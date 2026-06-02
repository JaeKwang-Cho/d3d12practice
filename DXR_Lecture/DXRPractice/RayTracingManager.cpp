#include "pch.h"
#include "RayTracingManager.h"
#include "D3D12Renderer.h"
#include "ShaderTable.h"
#include "ShaderTable_Common.h"
#include "ShaderManager.h"
#include "SimpleConstantBufferPool.h"
#include "ConstantBufferManager.h"
#include "D3D12ResourceManager.h"

const wchar_t* c_raygenShaderName = { L"MyRaygenShader_RadianceRay" };
const wchar_t* c_closestHitShaderName[] = { L"MyClosestHitShader_RadianceRay" };
const wchar_t* c_missShaderName[] = { L"MyMissShader_RadianceRay" }; 
const wchar_t* c_anyHitShaderName[] = { L"MyAnyHitShader_RadianceRay" };

// Hit groups
const wchar_t* c_hitGroupName[] = { L"MyHitGroup_Triangle_RadianceRay" };

bool RayTracingManager::Initialize(D3D12Renderer* _pRenderer, UINT _ulWidth, UINT _ulHeight)
{
	m_pRenderer = _pRenderer;
	m_pD3DDevice = m_pRenderer->INL_GetD3DDevice();

	ShaderManager* pShaderManager = m_pRenderer->INL_GetShaderManager();

	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if(FAILED(m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(m_pCommandQueue.GetAddressOf()))))
	{
		__debugbreak();
		return false;
	}

	CreateCommandList_forRayTracing();
	CreateFence_forRayTracing();

	m_ulHeight = _ulHeight;	
	m_ulWidth = _ulWidth;

	CreateDescriptorHeapCBV_SRV_UAV();
	CreateShaderVisibleHeap();

	m_pRayShaderHandle = pShaderManager->CreateShaderDXC(L"Raytracing.hlsl", L"", L"lib_6_3", 0);

	CreateOutputDiffuseBuffer(m_ulWidth, m_ulHeight);
	CreateOutputDepthBuffer(m_ulWidth, m_ulHeight);

	CreateRootSignatures();
	CreateRaytracingPipelineStateObject();

	BuildShaderTable();

	// build geometry
	InitMesh();
	
	// build accelration structure
	InitAccelerationStructure();

	return true;
}

void RayTracingManager::DoRayTracing(D3D12GraphicsCommandList_raw _pCommandList)
{
	CD3DX12_CPU_DESCRIPTOR_HANDLE dispatchHeapHandleCPU(m_pShaderVisibleDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle = {};
	SimpleConstantBufferPool* pCBPool = m_pRenderer->INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE::RAY_TRACING);
	CB_CONTAINER* pCBContainer = pCBPool->AllocCBContainer();
	if (!pCBContainer) {
		__debugbreak();
		return;
	}

	// RayTracing에 필요한 상수 버퍼 데이터를 채운다.
	CONSTANT_BUFFER_RAY_TRACING* pConstBuffer = reinterpret_cast<CONSTANT_BUFFER_RAY_TRACING*>(pCBContainer->pSysMemAddress);
	pConstBuffer->MaxRadianceRayRecursionDepth = MAX_RADIANCE_RECURSION_DEPTH;
	pConstBuffer->Near = NEAR_PLANE;
	pConstBuffer->Far = FAR_PLANE;

	// (0) CBV - RayTracing
	m_pD3DDevice->CopyDescriptorsSimple(1, dispatchHeapHandleCPU, pCBContainer->CBVHandle, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dispatchHeapHandleCPU.Offset(1, m_DescriptorSize);

	// (1) UAV - Output Diffuse
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavDiffuse(m_pCommonDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(COMMON_DESCRIPTOR_INDEX::OUTPUT_DIFFUSE_UAV), m_DescriptorSize);
	m_pD3DDevice->CopyDescriptorsSimple(1, dispatchHeapHandleCPU, uavDiffuse, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dispatchHeapHandleCPU.Offset(1, m_DescriptorSize);

	// (2) UAV - Output Depth
	CD3DX12_CPU_DESCRIPTOR_HANDLE uavDepth(m_pCommonDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(COMMON_DESCRIPTOR_INDEX::OUTPUT_DEPTH_UAV), m_DescriptorSize);
	m_pD3DDevice->CopyDescriptorsSimple(1, dispatchHeapHandleCPU, uavDepth, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	dispatchHeapHandleCPU.Offset(1, m_DescriptorSize);

	CD3DX12_RESOURCE_BARRIER rcBarrier[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(m_pOutputDiffuseBuffer.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS),
		CD3DX12_RESOURCE_BARRIER::Transition(m_pOutputDepthBuffer.Get(), D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS)
	};

	_pCommandList->ResourceBarrier(static_cast<UINT>(_countof(rcBarrier)), rcBarrier);

	// Ray들이 사용할 global root signature을 설정한다.
	_pCommandList->SetComputeRootSignature(m_pRaytracingGlobalRootSignature.Get());

	// acceleration structure와 dispatch rays를 위한 descriptor heap을 설정한다.
	D3D12DescriptorHeap_raw ppHeaps[] = { m_pShaderVisibleDescriptorHeap.Get() };
	_pCommandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	// heap을 table로 설정한다. root param 0에 heap이 바인딩된다고 가정한다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE dispatchHeapHandleGPU(m_pShaderVisibleDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	_pCommandList->SetComputeRootDescriptorTable(0, dispatchHeapHandleGPU);

	// 각 Ray들은 D3D12_DISPATCH_RAYS_DESC에 값을	 채워서 DispatchRays()로 실행된다.
	D3D12_DISPATCH_RAYS_DESC dispatchDesc = {};
	// t0에 TLAS를 바인딩한다. Ray들이 TLAS를 참조해서 씬의 geometry에 대한 정보를 얻는다.
	_pCommandList->SetComputeRootShaderResourceView(1, m_pTLAS->GetGPUVirtualAddress());

	// Hit group shader table
	D3D12Resource_raw pHitGroupShaderTableResource = m_pHitGroupShaderTable->GetResource();
	dispatchDesc.HitGroupTable.StartAddress = pHitGroupShaderTableResource->GetGPUVirtualAddress();
	dispatchDesc.HitGroupTable.SizeInBytes = m_pHitGroupShaderTable->GetHitGroupShaderTableSize();
	dispatchDesc.HitGroupTable.StrideInBytes = m_pHitGroupShaderTable->GetShaderRecordSize();

	// Miss shader table
	D3D12Resource_raw pMissShaderTableResource = m_pMissShaderTable->GetResource();
	dispatchDesc.MissShaderTable.StartAddress = pMissShaderTableResource->GetGPUVirtualAddress();
	dispatchDesc.MissShaderTable.SizeInBytes = m_pMissShaderTable->GetShaderRecordSize();
	dispatchDesc.HitGroupTable.StrideInBytes = m_pMissShaderTable->GetShaderRecordSize();

	// Raygen shader table
	D3D12Resource_raw pRayGenShaderTableResource = m_pRayGenShaderTable->GetResource();
	dispatchDesc.RayGenerationShaderRecord.StartAddress = pRayGenShaderTableResource->GetGPUVirtualAddress();
	dispatchDesc.RayGenerationShaderRecord.SizeInBytes = m_pRayGenShaderTable->GetShaderRecordSize();

	// Ray dimension
	dispatchDesc.Width = m_ulWidth;
	dispatchDesc.Height = m_ulHeight;
	dispatchDesc.Depth = 1;

	// Object를 세팅해주고, Ray를 Dispatch한다.
	_pCommandList->SetPipelineState1(m_pDXRStateObject.Get());
	_pCommandList->DispatchRays(&dispatchDesc);

	CD3DX12_RESOURCE_BARRIER rcBarrier2[] = {
		CD3DX12_RESOURCE_BARRIER::Transition(m_pOutputDiffuseBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE),
		CD3DX12_RESOURCE_BARRIER::Transition(m_pOutputDepthBuffer.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE)
	};

	_pCommandList->ResourceBarrier(static_cast<UINT>(_countof(rcBarrier2)), rcBarrier2);
}

void RayTracingManager::UpdateWindowSize_forRayTracing(UINT _ulWidth, UINT _ulHeight)
{
	CleanupOutputDiffuseBuffer();
	CleanupOutputDepthBuffer();

	m_ulHeight = _ulHeight;
	m_ulWidth = _ulWidth;

	CreateOutputDiffuseBuffer(m_ulWidth, m_ulHeight);
	CreateOutputDepthBuffer(m_ulWidth, m_ulHeight);
}

BLAS_INSTANCE* RayTracingManager::AllocBLAS(D3D12Resource_raw _pVertexBuffer, UINT _VertexSize, ULONG _ulVertexCount, const BLAS_BUILD_TRIGROUP_INFO* _pTriGroupInfoList, ULONG _ulTriGroupCount, bool _bAllowUpdate)
{
	std::unique_ptr<BLAS_INSTANCE> pBlasInstance = BuildBLAS(_pVertexBuffer, _VertexSize, _ulVertexCount, _pTriGroupInfoList, _ulTriGroupCount, _bAllowUpdate);
	BLAS_INSTANCE* pBlasInstanceRaw = pBlasInstance.get();
	m_arrBLASInstance.push_back(std::move(pBlasInstance));

	return pBlasInstanceRaw;
}

void RayTracingManager::CreateCommandList_forRayTracing()
{
	if(FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}

	if(FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}
	m_pCommandList->Close();
}


void RayTracingManager::CreateFence_forRayTracing()
{
	if(FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}
	m_ui64FenceValue = 0;

	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

UINT64 RayTracingManager::DoFence_forRayTracing()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);	
	return m_ui64FenceValue;
}

void RayTracingManager::WaitForFenceValue_forRayTracing()
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;
	if(m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void RayTracingManager::CleanupFence_forRayTracing()
{
	if (m_hFenceEvent)
	{
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
	if (m_pFence)
	{
		m_pFence.Reset();
	}
}

void RayTracingManager::BuildShaderTable()
{
	// RayGen Shader에 대한 shader identifier가 들어가는, shader table이 있어야 한다.
	Microsoft::WRL::ComPtr<ID3D12StateObjectProperties> pStateObjectProperties = nullptr;
	m_pDXRStateObject->QueryInterface(IID_PPV_ARGS(pStateObjectProperties.GetAddressOf()));

	// entry 이름으로 identifier를 얻고, shader table에 넣는다. Shader table은 raygen, miss, hit group shader 각각에 대해 만들어야 한다.
	// 
	// Raygen Shader Table
	void* pRayGenShaderIdentifier = pStateObjectProperties->GetShaderIdentifier(c_raygenShaderName);
	ShaderRecord rayGenShaderRecord = ShaderRecord(pRayGenShaderIdentifier, m_ShaderIdentifierSize, nullptr, 0);

	m_pRayGenShaderTable = std::make_unique<ShaderTable>();
	m_pRayGenShaderTable->Initialize(m_pD3DDevice, m_ShaderIdentifierSize, L"RayGenShaderTable");
	m_pRayGenShaderTable->CommitResource(1); // 지금	은 raygen shader record 하나만 있으므로 1로 설정한다.
	m_pRayGenShaderTable->InsertShaderRecord(&rayGenShaderRecord);

	// Miss Shader Table
	void* pMissShaderIdentifier = pStateObjectProperties->GetShaderIdentifier(c_missShaderName[0]);
	ShaderRecord missShaderRecord = ShaderRecord(pMissShaderIdentifier, m_ShaderIdentifierSize);

	m_pMissShaderTable = std::make_unique<ShaderTable>();
	m_pMissShaderTable->Initialize(m_pD3DDevice, m_ShaderIdentifierSize, L"MissShaderTable");
	m_pMissShaderTable->CommitResource(1);
	m_pMissShaderTable->InsertShaderRecord(&missShaderRecord);
	m_MissShaderTableStrideInBytes = m_pMissShaderTable->GetShaderRecordSize();

	// Hit Group Shader Table
	// 여기서 Hit Group Name을 사용해서 export 했던 hit group subobject를 참조해서 shader identifier를 얻는다. 
	// Hit Group Shader Table의 각 레코드는 shader identifier과 root argument로 구성된다. 
	// Root argument는 shader에서 접근할 수 있는 GPU descriptor handle이다.
	void* pHitGroupShaderIdentifier = pStateObjectProperties->GetShaderIdentifier(c_hitGroupName[0]);
	m_pHitGroupShaderTable = std::make_unique<ShaderTable>();
	m_pHitGroupShaderTable->Initialize(m_pD3DDevice, m_ShaderIdentifierSize + sizeof(ROOT_ARG), L"HitGroupShaderTable");
	m_pHitGroupShaderTable->CommitResource(1);
	// Hit Group Shader Table의 각 레코드는 shader identifier과 root argument로 구성된다. Root argument는 shader에서 접근할 수 있는 GPU descriptor handle이다.
	ROOT_ARG rootArg = {}; // 여기서는 hit에 전달할 파라미터가 없으므로, root argument는 비워둔다.
	// ShaderRecord 내부적으로 shader identifier과 root argument가 결합된 레코드 버퍼를 만들어서, shader table에 넣는다.
	// D3D12_RAYTRACING_SHADER_RECORD_BYTE_ALIGNMENT 사이즈 분량으로 shader identifier과 root argument가 결합된 레코드 버퍼가 만들어진다.
	ShaderRecord hitGroupShaderRecord = ShaderRecord(pHitGroupShaderIdentifier, m_ShaderIdentifierSize, &rootArg, sizeof(ROOT_ARG));
	m_pHitGroupShaderTable->InsertShaderRecord(&hitGroupShaderRecord);
	m_HitGroupShaderTableStrideInBytes = m_pHitGroupShaderTable->GetShaderRecordSize();
	m_ulHitGroupShaderRecordNum = m_pHitGroupShaderTable->GetShaderRecordCount();
}

void RayTracingManager::CreateRootSignatures()
{
	// 모든 ray tracing shader가 공통으로 사용하는 global root signature을 만든다.
	// DispatchRays()에서 사용할 root signature이기도 하다.

	// root param 0
	// output-diffuse(uav) | output-depth(uav)

	// root param 1
	// AccelerationStructure

	CD3DX12_DESCRIPTOR_RANGE globalRanges[2] = {};
	globalRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);  // b0 - CBV
	globalRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 2, 0);  // u0 - Diffuse UAV, u1 - Depth UAV

	// b0 : RayTracingCB | u0 : Output Diffuse | u1 : Output Depth | t0 : AccelerationStructure
	CD3DX12_ROOT_PARAMETER GlobalRootParameters[2] = {};
	GlobalRootParameters[0].InitAsDescriptorTable(_countof(globalRanges), globalRanges, D3D12_SHADER_VISIBILITY_ALL);
	GlobalRootParameters[1].InitAsShaderResourceView(0); // t0 - AccelerationStructure

	// Sampler
	D3D12_STATIC_SAMPLER_DESC samplers[4] = {};
	SetSamplerDesc_Wrap(&samplers[0], 0); // s0 - Wrap Sampler
	SetSamplerDesc_Clamp(&samplers[1], 1); // s1 - Clamp Sampler
	SetSamplerDesc_Wrap(&samplers[2], 2); // s2 - Wrap Sampler
	samplers[2].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	SetSamplerDesc_Mirror(&samplers[3], 3); // s3 - Mirror Sampler
	samplers[3].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	for (size_t i = 0; i < _countof(samplers); ++i) {
		samplers[i].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
	}

	CD3DX12_ROOT_SIGNATURE_DESC globalRootSignatureDesc(
		ARRAYSIZE(GlobalRootParameters),
		GlobalRootParameters,
		static_cast<UINT>(_countof(samplers)),
		samplers
	);

	SerializeAndCreateRaytracingRootSignature(m_pD3DDevice, &globalRootSignatureDesc, m_pRaytracingGlobalRootSignature.GetAddressOf());
}

void RayTracingManager::CreateRaytracingPipelineStateObject()
{
	// 총 7개의 Subobject를 만들어 RTPSO(RayTracing Pipeline State Object)를 만든다.
	// Subobject는 각각의 DXIL export(= shader entry point)에 기본적 또는 명시적으로 연결된다.

	// 구성:
	// 1. DXIL(DirectX Intermediate Language) Library
	// 2. Triangle hit group
	// 3. Shader Config (payload, attribute 크기)
	// 4 ~ 5. Local Root Signature와 Association
	// 6. Global Root Signature
	// 7. Pipeline Config (최대 recursion depth)
	CD3DX12_STATE_OBJECT_DESC raytracingPipeline{ D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE };


	// 1) DXIL Library Subobject
	// Shader는 Subobject로 간주되지 않는다. Shader는 DXIL로 컴파일된 후, DXIL 라이브러리 Subobject에 포함되어야 한다.
	CD3DX12_DXIL_LIBRARY_SUBOBJECT* pLib = raytracingPipeline.CreateSubobject<CD3DX12_DXIL_LIBRARY_SUBOBJECT>();
	// Shader ByteCode를 Subobject에 설정한다.
	D3D12_SHADER_BYTECODE libdxil = CD3DX12_SHADER_BYTECODE(m_pRayShaderHandle->pCodeBuffer, m_pRayShaderHandle->ullCodeSize);
	pLib->SetDXILLibrary(&libdxil);
	// RayGen Shader인 "MyRaygenShader_RadianceRay"를 Export한다. Export된 이름은 나중에 ShaderTable에서 참조된다.
	pLib->DefineExport(c_raygenShaderName);

	// HitGroup에서 import 할 수 있도록 export
	// 셰이더 타입별(radiance/shadow)로 ClosestHit, Miss, AnyHit 셰이더를 export한다.
	pLib->DefineExport(c_closestHitShaderName[0]);
	pLib->DefineExport(c_anyHitShaderName[0]);
	pLib->DefineExport(c_missShaderName[0]);

	// 2) Triangle hit group Subobject
	// hit group은 geometry에서 Ray가 intersected 후, 어떤 shader가 실행 될 지 정의 한다..
	CD3DX12_HIT_GROUP_SUBOBJECT* pHitGroup = raytracingPipeline.CreateSubobject<CD3DX12_HIT_GROUP_SUBOBJECT>();
	pHitGroup->SetClosestHitShaderImport(c_closestHitShaderName[0]);
	pHitGroup->SetAnyHitShaderImport(c_anyHitShaderName[0]);
	pHitGroup->SetHitGroupExport(c_hitGroupName[0]); // 이 HitGroup을 참조할 때 사용할 이름이다. ShaderTable에서 이 이름으로 참조된다.
	pHitGroup->SetHitGroupType(D3D12_HIT_GROUP_TYPE_TRIANGLES);
	//pHitGroup->SetIntersectionShaderImport(L""); // Triangle hit group이므로, intersection shader는 필요 없다.

	// 3) Shader Config Subobject
	// ray payload와 attribute의 크기를 정의한다.
	CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT* pShaderConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_SHADER_CONFIG_SUBOBJECT>();
	UINT payloadSize = PAYLOAD_SIZE;
	UINT attributeSize = 2 * static_cast<UINT>(sizeof(float)); // ray attribute는 ray와 intersection에 대한 정보를 담는다. (예시에서는 barycentrics)
	pShaderConfig->Config(payloadSize, attributeSize);

	// 4 ~ 5) Local Root Signature Subobject와 Association
	// Local Root Signature 및 연결 객체 설정
	// Shader Table에서 각 Shader가 고유한 parameter를 사용할 수 있게 한다.
	// No - Implemented. RayGen Shader만 있는 간단한 예제이므로, Local Root Signature는 만들지 않는다.

	// 6) Global Root Signature Subobject
	// DispatchRays()에서 나온 결과를 저장할 UAV 텍스쳐를 넘겨줄, Global Root Signature를 설정한다.
	CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT* pGlobalRootSignature = raytracingPipeline.CreateSubobject<CD3DX12_GLOBAL_ROOT_SIGNATURE_SUBOBJECT>();
	pGlobalRootSignature->SetRootSignature(m_pRaytracingGlobalRootSignature.Get());

	// 7) Pipeline Config Subobject
	// TraceRay() 함수의 최대 재귀 깊이를 설정한다.
	CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT* pPipelineConfig = raytracingPipeline.CreateSubobject<CD3DX12_RAYTRACING_PIPELINE_CONFIG_SUBOBJECT>();
	UINT maxRecursionDepth = MAX_RECURSION_DEPTH;
	pPipelineConfig->Config(maxRecursionDepth);

	// Subobject 간의 연결 설정
	const D3D12_STATE_OBJECT_DESC* pRaytracingPipelineDesc = raytracingPipeline;
	if(FAILED(m_pD3DDevice->CreateStateObject(pRaytracingPipelineDesc, IID_PPV_ARGS(m_pDXRStateObject.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}
	// 이렇게 이름을 정해주면(그리고 컴파일 할 때 디버그 정보가 포함되어 있으면), PIX에서 RayTracing Pipeline State Object의 이름으로 나온다.
	m_pDXRStateObject->SetName(L"RayTracingManager::m_pDXRStateObject");
}

void RayTracingManager::CreateDescriptorHeapCBV_SRV_UAV()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = static_cast<UINT>(COMMON_DESCRIPTOR_INDEX::Count);
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pCommonDescriptorHeap.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}
	m_pCommonDescriptorHeap->SetName(L"RayTracingManager::m_pCommonDescriptorHeap");

	m_DescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
}

void RayTracingManager::CreateShaderVisibleHeap()
{
	D3D12_DESCRIPTOR_HEAP_DESC HeapDesc = {};
	HeapDesc.NumDescriptors = static_cast<UINT>(DISPATCH_DESCRIPTOR_INDEX::Count);
	HeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	HeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&HeapDesc, IID_PPV_ARGS(m_pShaderVisibleDescriptorHeap.GetAddressOf()))))
	{
		__debugbreak();
		return;
	}
}

bool RayTracingManager::CreateOutputDiffuseBuffer(UINT _uiWidth, UINT _uiHeight)
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	texDesc.Width = _uiWidth;
	texDesc.Height = _uiHeight;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAV로 사용할 것이므로, Unordered Access 플래그를 설정한다.
	texDesc.DepthOrArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	auto defaultHeapProps =  CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pOutputDiffuseBuffer.GetAddressOf()))))
	{
		__debugbreak();
		return false;
	}
	m_pOutputDiffuseBuffer->SetName(L"RayTracingManager::m_pOutputDiffuseBuffer");

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(
		m_pCommonDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		static_cast<UINT>(COMMON_DESCRIPTOR_INDEX::OUTPUT_DIFFUSE_UAV), m_DescriptorSize);
	m_pD3DDevice->CreateUnorderedAccessView(m_pOutputDiffuseBuffer.Get(), nullptr, nullptr, uavHandle);

	return true;
}

void RayTracingManager::CleanupOutputDiffuseBuffer()
{
	if(m_pOutputDiffuseBuffer)
	{
		m_pOutputDiffuseBuffer.Reset();
	}
}

bool RayTracingManager::CreateOutputDepthBuffer(UINT _uiWidth, UINT _uiHeight)
{
	D3D12_RESOURCE_DESC texDesc = {};
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32_TYPELESS;
	texDesc.Width = _uiWidth;
	texDesc.Height = _uiHeight;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS; // UAV로 사용할 것이므로, Unordered Access 플래그를 설정한다.
	texDesc.DepthOrArraySize = 1;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pOutputDepthBuffer.GetAddressOf()))))
	{
		__debugbreak();
		return false;
	}
	m_pOutputDepthBuffer->SetName(L"RayTracingManager::m_pOutputDepthBuffer");

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Buffer.StructureByteStride = sizeof(float);
	uavDesc.Buffer.NumElements = _uiWidth * _uiHeight;
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

	CD3DX12_CPU_DESCRIPTOR_HANDLE uavHandle(
		m_pCommonDescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		static_cast<UINT>(COMMON_DESCRIPTOR_INDEX::OUTPUT_DEPTH_UAV), m_DescriptorSize);
	m_pD3DDevice->CreateUnorderedAccessView(m_pOutputDepthBuffer.Get(), nullptr, &uavDesc, uavHandle);

	return true;
}

void RayTracingManager::CleanupOutputDepthBuffer()
{
	if(m_pOutputDepthBuffer)
	{
		m_pOutputDepthBuffer.Reset();
	}
}

std::unique_ptr<BLAS_INSTANCE> RayTracingManager::BuildBLAS(D3D12Resource_raw _pVertexBuffer, UINT _VertexSize, ULONG _ulVertexCount, const BLAS_BUILD_TRIGROUP_INFO* _pTriGroupInfoList, ULONG _ulTriGroupCount, bool _bAllowUpdate)
{
	D3D12Resource_raw pBLASResource = nullptr; // BLAS_INSTANCE에 들어갈 BLAS 리소스

	// 일단은 VB 하나에 IB 여러개 (물론 나중에 구조를 바꿀 수 있다.)
	if (_ulTriGroupCount > MAX_TRIANGLE_COUNT_PER_BLAS) {
		__debugbreak();
		return nullptr;
	}
	// 가변 메모리 할당인데, unique_ptr는 상관이 없다.
	ULONG ulBlasInstanceMemSize = static_cast<ULONG>(sizeof(BLAS_INSTANCE) - sizeof(ROOT_ARG)) * _ulTriGroupCount;

	std::unique_ptr<BLAS_INSTANCE> pBlasInstance = std::make_unique<BLAS_INSTANCE>();
	pBlasInstance->ulID = 0;
	pBlasInstance->matTransform = XMMatrixIdentity();
	pBlasInstance->ulVertexCount = _ulVertexCount;

	// BLAS에 들어갈 geometry description을 만든다. 여러개의 trigroup이 있다면, 각 그룹마다 geometry description이 필요하다. (예시에서는 하나의 그룹만 있지만, 일반적으로는 여러 그룹이 있을 수 있다.)
	D3D12_RAYTRACING_GEOMETRY_DESC pGeomDescList[MAX_TRIANGLE_COUNT_PER_BLAS] = {};
	D3D12_GPU_VIRTUAL_ADDRESS VB_GPU_Ptr = _pVertexBuffer->GetGPUVirtualAddress();
	for (ULONG i = 0; i < _ulTriGroupCount; i++) {
		D3D12_GPU_VIRTUAL_ADDRESS IB_GPU_Ptr = _pTriGroupInfoList[i].pIndexBuffer->GetGPUVirtualAddress();
		// 지금은 triangle geometry만 지원하지만, AABB geometry도 지원할 수 있다.
		pGeomDescList[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES; 
		// IndexBuffer와 VertexBuffer의 GPU virtual address를 설정한다. 그리고 index와 vertex의 개수, vertex 포맷 등도 설정한다.
		pGeomDescList[i].Triangles.IndexBuffer = IB_GPU_Ptr;
		pGeomDescList[i].Triangles.IndexCount = _pTriGroupInfoList[i].ulIndexNum;
		pGeomDescList[i].Triangles.IndexFormat = DXGI_FORMAT_R16_UINT;
		pGeomDescList[i].Triangles.Transform3x4 = 0; // BLAS 변환은 하지 않고
		pGeomDescList[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
		pGeomDescList[i].Triangles.VertexCount = _ulVertexCount;
		pGeomDescList[i].Triangles.VertexBuffer.StartAddress = VB_GPU_Ptr;
		pGeomDescList[i].Triangles.VertexBuffer.StrideInBytes = _VertexSize;
		// 불투명이 아니라면, AnyHit Shader가 실행되어야 하므로, D3D12_RAYTRACING_GEOMETRY_FLAG_NONE 플래그를 설정한다.
		// 불투명하다면, AnyHit Shader가 실행되지 않고 바로 ClosestHit Shader가 실행되어야 하므로, D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE 플래그를 설정한다.
		if (_pTriGroupInfoList[i].bNotOpaque)
		{
			pGeomDescList[i].Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_NONE;
		}
		else 
		{
			pGeomDescList[i].Flags |= D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}
	}

	// Build BLAS with pGeomDescList and store the result in pBlasInstance->pBLASResource
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	if (_bAllowUpdate) { // 애니메이션이 들어가면 skinning이 필요한데, 그러면 BLAS를 업데이트 해야 한다.
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}
	else { 
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	}
	inputs.NumDescs = _ulTriGroupCount;
	inputs.pGeometryDescs = pGeomDescList;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL; // BLAS를 만드는 중이다.

	// 사전 빌드 정보를 얻는다. 이 정보에는 GPU에서 BLAS를 빌드하는 데 필요한 
	// scratch buffer와 BLAS buffer의 크기가 포함되어 있다.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	m_pD3DDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12Resource_ptr pScratchResource = nullptr;
	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	// UAV 타입으로 사이즈를 얻어서 GPU 리소스를 할당한다. Scratch buffer는 BLAS를 빌드하는 동안에 필요한 임시 버퍼이다. 
	HRESULT hr = m_pD3DDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uavResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(pScratchResource.GetAddressOf()));

	if (FAILED(hr)) {
		__debugbreak();
		return nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS pScratchGPUAddress = pScratchResource->GetGPUVirtualAddress();
	if(!pScratchGPUAddress){
		__debugbreak();
		return nullptr;
	}
	// acceleration structure를 위한 리소스 할당하기
	// Acceleration Structure는 오직 default heap(혹은 custom heap)에만 생성될 수 있다.
	// Default heap은 CPU에서 접근할 수 없다.
	// Acceleration Structure를 가질 리소스는 D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE 상태로 생성되어야 한다.
	// 그리고 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS 플래그도 설정되어야 한다. 

	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	// BLAS로 사용할 리소스를 할당한다. UAV 버퍼로 BLAS 버퍼의 크기를 얻어서 GPU 리소스를 할당한다. BLAS 버퍼는 최종 BLAS가 저장될 버퍼이다.
	hr = CreateUAVBuffer(m_pD3DDevice, info.ResultDataMaxSizeInBytes, &pBLASResource, initialResourceState, L"BottomLevelAccelerationStructure");
	if (FAILED(hr)) {
		__debugbreak();
		return nullptr;
	}

	// BLAS desc을 이용해서 (scratch buffer와 BLAS 버퍼를 포함해서) BLAS를 빌드한다. BLAS 빌드는 GPU에서 실행되어야 한다.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC blasDesc = {};
	blasDesc.Inputs = inputs;
	blasDesc.ScratchAccelerationStructureData = pScratchGPUAddress;
	blasDesc.DestAccelerationStructureData = pBLASResource->GetGPUVirtualAddress();

	if(FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
		return nullptr;
	}
	if(FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
		return nullptr;
	}

	m_pCommandList->BuildRaytracingAccelerationStructure(&blasDesc, 0, nullptr);
	// RayTracing을 하기 전에, UAV Barrier를 넣어준다. (BLAS 버퍼에 대한 UAV Barrier)
	// (지금은 뒤에 fence가 있어서 필요없지만, 나중에 성능을 올리기 위해 구조를 바꾼다면 필요하다.)
	auto uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(pBLASResource);
	m_pCommandList->ResourceBarrier(1, &uavBarrier);

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	DoFence_forRayTracing();
	WaitForFenceValue_forRayTracing();

	pBlasInstance->pBLAS = pBLASResource;
	pBlasInstance->ulTriGroupCount = _ulTriGroupCount;

	return pBlasInstance;
}

D3D12Resource_ptr RayTracingManager::BuildTLAS(D3D12Resource_raw _pInstanceDescResource, BLAS_INSTANCE** _ppInstanceList, ULONG _ulBlasInstanceNum, bool _bAllowUpdate, UINT _CurrContextIndex)
{
	D3D12Resource_ptr pTLASResource = nullptr; // TLAS를 위한 리소스

	// TLAS도 BLAS와 비슷한 방식으로 만들어진다. TLAS는 instance desc로부터 만들어지는데, instance desc는 GPU에서 접근할 수 있는 리소스에 있어야 한다.
	// TLAS 버퍼의 크기를 얻고, TLAS를 위한 리소스를 할당한다. TLAS도 D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS 플래그가 설정된 default heap에 생성되어야 한다.

	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE;
	if(_bAllowUpdate) {
		inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_UPDATE;
	}
	else { // TLAS는 일반적으로 업데이트가 필요 없으므로, 빠른 추적을 선호하는 플래그를 설정한다.
		inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE | D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_ALLOW_COMPACTION;
	}
	inputs.NumDescs = _ulBlasInstanceNum; // BLAS 인스턴스의 개수로 instance desc의 개수를 설정한다.
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL; // TLAS를 만드는 중이다.
	
	// 역시 사전 빌드 정보를 얻는다. 이 정보에는 GPU에서 TLAS를 빌드하는 데 필요한 scratch buffer와 TLAS buffer의 크기가 포함되어 있다.
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	m_pD3DDevice->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	D3D12Resource_ptr pScratchResource = nullptr;

	auto defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	auto uavResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
	// GPU에 TLAS 컴파일을 위한 scratch 메모리를 할당한다. (UAV)
	HRESULT hr = m_pD3DDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&uavResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr, IID_PPV_ARGS(pScratchResource.GetAddressOf()));
	if (FAILED(hr)) {
		__debugbreak();
		return nullptr;
	}

	D3D12_GPU_VIRTUAL_ADDRESS pScratchGPUAddress = pScratchResource->GetGPUVirtualAddress();

	D3D12_RESOURCE_STATES initialResourceState = D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE;
	hr = CreateUAVBuffer(m_pD3DDevice, info.ResultDataMaxSizeInBytes, pTLASResource.GetAddressOf(), initialResourceState, L"TopLevelAccelerationStructure");

	// 인자로 넘어온 instance desc 리스트와, 그 크기의 upload buffer가 들어왔으니
	// instance desc 리스트를 GPU에서 접근할 수 있는 리소스에 복사한다.
	// (instance desc 리스트는 CPU에서 만들어지지만, GPU에서 접근할 수 있는 리소스에 있어야 한다.)
	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDescLists = nullptr;

	CD3DX12_RANGE readRange(0, 0);
	_pInstanceDescResource->Map(0, &readRange, reinterpret_cast<void**>(&pInstanceDescLists));
	// 차례대로 upload 버퍼에 넣는다. instance desc 리스트의 각 entry는 BLAS 인스턴스 하나에 해당한다. 
	// instance desc 리스트의 각 entry에는 BLAS 인스턴스의 변환 행렬, B
	// LAS 인스턴스가 참조하는 BLAS의 GPU virtual address, 그리고 shader record index 등이 포함되어 있다.
	ULONG ulTLAS_ElementCount = 0;
	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDescEntry = pInstanceDescLists;
	for(ULONG i = 0; i<_ulBlasInstanceNum; i++)
	{
		const BLAS_INSTANCE* pInstanceSrc = _ppInstanceList[i];
		XMMATRIX matTranpose = XMMatrixTranspose(pInstanceSrc->matTransform);
		memcpy(pInstanceDescEntry->Transform, &matTranpose, sizeof(pInstanceDescEntry->Transform));

		pInstanceDescEntry->InstanceID = pInstanceSrc->ulID; // 나중에 Shader에서 InstanceID()으로 노출이 되는 값이다.
		pInstanceDescEntry->InstanceContributionToHitGroupIndex = pInstanceSrc->uiShaderRecordIndex; // **** 중요! ****
		// Ray가 hit group shader table에서 어떤 레코드를 참조할 지 결정하는 값이다.
		// Ray가 hit group shader table에서 참조할 레코드는 InstanceContributionToHitGroupIndex + HitGroupIndexInShaderTable이다.
		// (HitGroupIndexInShaderTable은 ray tracing dispatch할 때 설정된다.)
		// 오브젝트 갯수가 줄거나 늘어날 때, InstanceContributionToHitGroupIndex를 compact 하게 유지하는 것이 성능에 좋다.
		// 현재 샘플은 그렇지 않지만. uiShaderRecordIndex가 그 역할을 한다. uiShaderRecordIndex는 BLAS 인스턴스가 참조하는 hit group shader record의 index이다. 
		// (예시에서는 하나의 hit group shader record만 있지만, 일반적으로는 여러 개가 있을 수 있다.)
		pInstanceDescEntry->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
		pInstanceDescEntry->AccelerationStructure = pInstanceSrc->pBLAS->GetGPUVirtualAddress(); // BLAS 인스턴스가 참조하는 BLAS의 GPU virtual address이다. Ray가 이 BLAS 인스턴스와 교차할 때, 이 주소를 참조해서 BLAS에 접근한다.
		pInstanceDescEntry->InstanceMask = 0xFF;
		pInstanceDescEntry++;
		ulTLAS_ElementCount++;
	}
	_pInstanceDescResource->Unmap(0, nullptr);

	// 이제 GPU를 통해서 TLAS desc으로 TLAS를 빌드한다. 
	// TLAS 빌드도 BLAS 빌드와 비슷하지만, BLAS가 geometry desc로 만들어지는 반면에,
	// TLAS는 instance desc로 만들어진다는 점이 다르다.
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC tlasDesc = {};
	tlasDesc.Inputs = inputs;
	tlasDesc.Inputs.NumDescs = ulTLAS_ElementCount;
	tlasDesc.Inputs.InstanceDescs = _pInstanceDescResource->GetGPUVirtualAddress();
	tlasDesc.DestAccelerationStructureData = pTLASResource->GetGPUVirtualAddress();
	tlasDesc.ScratchAccelerationStructureData = pScratchGPUAddress;

	if(FAILED(m_pCommandAllocator->Reset()))
	{
		__debugbreak();
		return nullptr;
	}
	
	if(FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr)))
	{
		__debugbreak();
		return nullptr;
	}

	m_pCommandList->BuildRaytracingAccelerationStructure(&tlasDesc, 0, nullptr);

	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = pTLASResource.Get();
	m_pCommandList->ResourceBarrier(1, &uavBarrier);

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	DoFence_forRayTracing();
	WaitForFenceValue_forRayTracing();

	return pTLASResource;
}

bool RayTracingManager::InitMesh()
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// Create the vertex buffer.
	BasicVertex Vertices[] =
	{
		{ { -0.25f, 0.25f, 0.1f }, { 0.0f, 0.0f, 1.0f, 1.0f } },
		{ { 0.25f, 0.25f, 0.1f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
		{ { 0.25f, -0.25f, 0.1f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
		{ { -0.25f, -0.25f, 0.1f }, { 0.0f, 0.5f, 0.5f, 1.0f } }
	};
	WORD Indices[] =
	{
		0, 1, 2,
		0, 2, 3
	};

	D3D12_VERTEX_BUFFER_VIEW VertexBufferView = {};
	D3D12_INDEX_BUFFER_VIEW IndexBufferView = {};

	const UINT VertexBufferSize = sizeof(Vertices);

	if (FAILED(pResourceManager->CreateVertexBuffer(sizeof(BasicVertex), static_cast<ULONG>(_countof(Vertices)), &VertexBufferView, &m_pVertexBuffer, Vertices)))
	{
		__debugbreak();
		return false;
	}
	const DWORD ulFacesNum = 2;
	ULONG ulIndicesSize = ulFacesNum * 3 * static_cast<ULONG>(sizeof(USHORT));
	ULONG ulAlignedIndicesSize = (ulIndicesSize / 16 + ((ulIndicesSize % 16) != 0)) * 16; 
	// GPU에서 Access 하기 편하도록 16바이트 Align을 해준다.
	ULONG ulAlignedIndexNum = ulAlignedIndicesSize / sizeof(USHORT);
	if (FAILED(pResourceManager->CreateIndexBuffer(ulAlignedIndexNum, &IndexBufferView, &m_pIndexBuffer, Indices, sizeof(Indices))))
	{
		__debugbreak();
		return false;
	}

	return true;
} 

bool RayTracingManager::InitAccelerationStructure()
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	D3D12ResourceManager* pResourceManager = m_pRenderer->INL_GetResourceManager();

	// Build BLAS
	BLAS_BUILD_TRIGROUP_INFO BuildInfo = {};
	// 사용할 Primitive type을 넣어준다.
	// rasterization에서는 index buffer가 하나만 필요하지만, 
	// ray tracing에서는 geometry description마다 index buffer가 필요하다. 
	// (예시에서는 하나의 geometry description만 있지만, 일반적으로는 여러 개가 있을 수 있다.)
	BuildInfo.pIndexBuffer = m_pIndexBuffer.Get(); 
	BuildInfo.bNotOpaque = false;
	BuildInfo.ulIndexNum = 6;
	
	// 사각형 하나에 대한 BLAS를 만든다.
	// Vertex 버퍼와, 그것을 사용하는 index buffer들을	 BLAS_BUILD_TRIGROUP_INFO에 담아서 BLAS를 만든다.
	m_pBLASInstance = AllocBLAS(m_pVertexBuffer.Get(), sizeof(BasicVertex), 4, &BuildInfo, 1, false);
	
	// Shader Table을 만든다. Shader Table은 RayGen, Miss, Hit Group shader 각각에 대해서 만들어야 한다. 
	// Shader Table을 만들 때, shader identifier과 root argument를 넣어준다. 
	// Shader identifier는 RayTracing Pipeline State Object를 만들 때 export했던 이름으로 참조할 수 있다. 
	// Root argument는 shader에서 접근할 수 있는 GPU descriptor handle이다.
	BuildShaderTable();

	// BLAS update를 바뀐 친구들만 모아서 하기 위해, BLAS instance들을 리스트로 만들어서,
	// 그리고 TLAS를 만들 때 같이 넘겨준다.
	BLAS_INSTANCE* ppBLASInstanceList[256] = {};
	ULONG ulBLASInstanceCount = 0;

	for(auto iter = m_arrBLASInstance.begin(); iter != m_arrBLASInstance.end(); iter++)
	{
		ppBLASInstanceList[ulBLASInstanceCount++] = iter->get();
	}
	// TLAS를 만들 때, BLAS instance 리스트와, 그리고 BLAS instance 리스트가 들어있는 GPU 리소스를 같이 넘겨준다.
	// 그러기 위해서 BLAS instance 리스트를 GPU에서 접근할 수 있는 리소스에 복사한다. BLAS instance 리스트는 TLAS desc로 사용된다.
	CreateUploadBuffer(m_pD3DDevice, nullptr, sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * ulBLASInstanceCount, m_pBLASInstanceDescResource.GetAddressOf(), L"InstanceDescs");

	m_pTLAS = BuildTLAS(m_pBLASInstanceDescResource.Get(), ppBLASInstanceList, ulBLASInstanceCount, false, 0);

	return true;
}

void RayTracingManager::CleanupRayTracingManager()
{
	WaitForFenceValue_forRayTracing();

	ShaderManager* pShaderManager = m_pRenderer->INL_GetShaderManager();
	if(m_pRayShaderHandle)
	{
		pShaderManager->ReleaseShader(m_pRayShaderHandle);
		m_pRayShaderHandle = nullptr;
	}
	CleanupFence_forRayTracing();
}

RayTracingManager::RayTracingManager()
{
}

RayTracingManager::~RayTracingManager()
{
	CleanupRayTracingManager();
}