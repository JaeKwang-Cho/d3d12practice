#include "pch.h"
#include "RayTracingManager.h"
#include "D3D12Renderer.h"
#include "ShaderTable.h"
#include "ShaderTable_Common.h"
#include "ShaderManager.h"
#include "SimpleConstantBufferPool.h"
#include "ConstantBufferManager.h"

const wchar_t* c_raygenShaderName = { L"MyRaygenShader_RadianceRay" };

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
	
	// build accelration structure

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
	// Set Acceleration Structure
	// No-Implementation

	// Hit group shader table
	// No-Implementation

	// Miss shader table
	// No-Implementation

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
	void* pRayGenShaderIdentifier = pStateObjectProperties->GetShaderIdentifier(c_raygenShaderName);

	// Raygen Shader Table
	ShaderRecord rayGenShaderRecord = ShaderRecord(pRayGenShaderIdentifier, m_ShaderIdentifierSize, nullptr, 0);
	m_pRayGenShaderTable = std::make_unique<ShaderTable>();
	m_pRayGenShaderTable->Initialize(m_pD3DDevice, m_ShaderIdentifierSize, L"RayGenShaderTable");
	m_pRayGenShaderTable->CommitResource(1); // 지금	은 raygen shader record 하나만 있으므로 1로 설정한다.
	m_pRayGenShaderTable->InsertShaderRecord(&rayGenShaderRecord);

	// Miss Shader Table

	// Hit Group Shader Table
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
	CD3DX12_ROOT_PARAMETER GlobalRootParameters[1] = {};
	GlobalRootParameters[0].InitAsDescriptorTable(_countof(globalRanges), globalRanges, D3D12_SHADER_VISIBILITY_ALL);

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

	// 2) Triangle hit group Subobject
	// hit group은 geometry에서 Ray가 intersected 후, 어떤 shader가 실행 될 지 정의 한다.
	// No - Implemented. RayGen Shader만 있는 간단한 예제이므로, hit group은 만들지 않는다.

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

void RayTracingManager::CleanupRayTracingManager()
{
	ShaderManager* pShaderManager = m_pRenderer->INL_GetShaderManager();
	if(m_pRayShaderHandle)
	{
		pShaderManager->ReleaseShader(m_pRayShaderHandle);
		m_pRayShaderHandle = nullptr;
	}

}

RayTracingManager::RayTracingManager()
{
}

RayTracingManager::~RayTracingManager()
{
	CleanupRayTracingManager();
}