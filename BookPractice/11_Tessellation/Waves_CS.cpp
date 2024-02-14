//***************************************************************************************
// Waves_CS.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Waves_CS.h"
//#include "Effects.h"
#include <algorithm>
#include <vector>
#include <cassert>

Waves_CS::Waves_CS(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, int _row, int _col, float _dx, float _dt, float _speed, float _damping)
{
	m_d3dDevice = _device;

	m_NumRows = _row;
	m_NumCols = _col;

	assert((_row * _col) % 256 == 0);

	m_VertexCount = _row * _col;
	m_TriangleCount = (_row - 1) * (_col - 1) * 2;

	m_TimeStep = _dt;
	m_SpatialStep = _dx;

	float d = _damping * _dt + 2.f;
	float e = (_speed * _speed) * (_dt * _dt) / (_dx * _dx);
	m_K[0] = (_damping * _dt - 2.f) / d;
	m_K[1] = (4.f - 8.f * e) / d;
	m_K[2] = (2.f * e) / d;

	BuildResources(_cmdList);
}

void Waves_CS::BuildResources(ID3D12GraphicsCommandList* _cmdList)
{
	// Srv로 입력하고, Uav로 출력을 받도록
	// 맴버 Resource들을 생성한다.

	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_NumCols;
	texDesc.Height = m_NumRows;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = DXGI_FORMAT_R32_FLOAT; // float으로 Texture를 만든다.
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_PrevSol)
	));

	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_CurrSol)
	));

	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_NextSol)
	));

	// resource를 Default Buffer로 만들어 넘겨주기 위해서, 중간 단계 upload heap 버퍼를 만든다.

	// 하위 리소스 개수와 GetRequiredIntermediateSize으로 필요한 버퍼의 크기를 구한다.
	const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
	const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_CurrSol.Get(), 0, num2DSubresources);

	D3D12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
	D3D12_RESOURCE_DESC resDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	// 업로드 버퍼를 만든다.
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_CurrUploadBuffer.GetAddressOf())
	));

	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&resDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_PrevUploadBuffer.GetAddressOf())
	));

	// 업로드 버퍼를 초기화 한다.
	std::vector<float> initData(m_NumRows * m_NumCols, 0.f);
	for (int i = 0; i < initData.size(); i++)
	{
		initData[i] = 0.f;
	}
	
	// 업로드 버퍼 속성값
	D3D12_SUBRESOURCE_DATA subResourceData = {};
	subResourceData.pData = initData.data();
	subResourceData.RowPitch = m_NumCols * sizeof(float);
	subResourceData.SlicePitch = m_NumRows * subResourceData.RowPitch;

	// GPU에 업로드 버퍼 데이터를 디폴트 버퍼에 복사하도록 요청한다.
	D3D12_RESOURCE_BARRIER prev_COMM_DEST = CD3DX12_RESOURCE_BARRIER::Transition(m_PrevSol.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	_cmdList->ResourceBarrier(1, &prev_COMM_DEST);
	UpdateSubresources(_cmdList, m_PrevSol.Get(), m_PrevUploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData);
	D3D12_RESOURCE_BARRIER prev_DEST_UA = CD3DX12_RESOURCE_BARRIER::Transition(m_PrevSol.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	_cmdList->ResourceBarrier(1, &prev_DEST_UA);
	m_PrevSol->SetName(L"prevSol");

	// Curr 이 친구는 초기 상태를 GENERIC_READ state로 만들어서 쉐이더에서 바로 쓸 수 있게 만든다.
	D3D12_RESOURCE_BARRIER curr_COMM_DEST = CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
	_cmdList->ResourceBarrier(1, &curr_COMM_DEST);
	UpdateSubresources(_cmdList, m_CurrSol.Get(), m_CurrUploadBuffer.Get(), 0, 0, num2DSubresources, &subResourceData);
	D3D12_RESOURCE_BARRIER curr_DEST_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
	_cmdList->ResourceBarrier(1, &curr_DEST_READ);
	m_CurrSol->SetName(L"currSol");

	// CS를 돌릴 준비를 마무리 한다.
	D3D12_RESOURCE_BARRIER next_COMM_UA = CD3DX12_RESOURCE_BARRIER::Transition(m_NextSol.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_UNORDERED_ACCESS );
	_cmdList->ResourceBarrier(1, &next_COMM_UA);
	m_NextSol->SetName(L"nextSol");
}

void Waves_CS::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor, UINT _descriptorSize)
{
	// 클래스 인스턴스가 내부적으로 뷰를 생성하고 핸들을 가지고 있는다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R32_FLOAT;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	m_d3dDevice->CreateShaderResourceView(m_PrevSol.Get(), &srvDesc, _hCpuDescriptor);
	m_d3dDevice->CreateShaderResourceView(m_CurrSol.Get(), &srvDesc, _hCpuDescriptor.Offset(1, _descriptorSize));
	m_d3dDevice->CreateShaderResourceView(m_NextSol.Get(), &srvDesc, _hCpuDescriptor.Offset(1, _descriptorSize));

	m_d3dDevice->CreateUnorderedAccessView(m_PrevSol.Get(), nullptr, &uavDesc, _hCpuDescriptor.Offset(1, _descriptorSize));
	m_d3dDevice->CreateUnorderedAccessView(m_CurrSol.Get(), nullptr, &uavDesc, _hCpuDescriptor.Offset(1, _descriptorSize));
	m_d3dDevice->CreateUnorderedAccessView(m_NextSol.Get(), nullptr, &uavDesc, _hCpuDescriptor.Offset(1, _descriptorSize));

	m_PrevSolSrv = _hGpuDescriptor;
	m_CurrSolSrv = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_NextSolSrv = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_PrevSolUav = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_CurrSolUav = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_NextSolUav = _hGpuDescriptor.Offset(1, _descriptorSize);
}

void Waves_CS::Update(const GameTimer& _gt, ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso)
{
	static float t = 0.f;

	t += _gt.GetDeltaTime();

	// CS를 돌린다.
	_cmdList->SetPipelineState(_pso);
	_cmdList->SetComputeRootSignature(_rootSig);

	if (t >= m_TimeStep)
	{
		// 방정식에 쓰이는 상수를 세팅하고
		_cmdList->SetComputeRoot32BitConstants(0, 3, m_K, 0); // 0 ~ 2 번째
		// Texture를 넣어준다.
		_cmdList->SetComputeRootDescriptorTable(1, m_PrevSolUav);
		_cmdList->SetComputeRootDescriptorTable(2, m_CurrSolUav);
		_cmdList->SetComputeRootDescriptorTable(3, m_NextSolUav);

		// 16개로 스레드 그룹을 하나씩 만든다.
		UINT numGroupsX = m_NumCols / 16;
		UINT numGroupsY = m_NumRows / 16;
		_cmdList->Dispatch(numGroupsX, numGroupsY, 1);

		// 업데이트가 끝나면 클래스 내부에서
		// 자리를 바꿔주면서 버퍼와 뷰들을 ping-pong 시켜준다.

		Microsoft::WRL::ComPtr<ID3D12Resource> resTemp = m_PrevSol;
		m_PrevSol = m_CurrSol;
		m_CurrSol = m_NextSol;
		m_NextSol = resTemp;

		CD3DX12_GPU_DESCRIPTOR_HANDLE srvTemp = m_PrevSolSrv;
		m_PrevSolSrv = m_CurrSolSrv;
		m_CurrSolSrv = m_NextSolSrv;
		m_NextSolSrv = srvTemp;

		CD3DX12_GPU_DESCRIPTOR_HANDLE uavTemp = m_PrevSolUav;
		m_PrevSolUav = m_CurrSolUav;
		m_CurrSolUav = m_NextSolUav;
		m_NextSolUav = uavTemp;

		t = 0.f; // 시간 초기화
		
		// Vertex Shader에서 읽을 수 있도록 Curr을 GENERIC_READ로 바꿔준다.
		D3D12_RESOURCE_BARRIER currsol_UA_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_GENERIC_READ);
		_cmdList->ResourceBarrier(1, &currsol_UA_READ);

		// 이전에 아마도 읽었던 Prev를 다시 UNORDERED_ACCESS로 바꾼다.
		D3D12_RESOURCE_BARRIER prevsol_READ_UA = CD3DX12_RESOURCE_BARRIER::Transition(m_PrevSol.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		_cmdList->ResourceBarrier(1, &prevsol_READ_UA);
	}
}

void Waves_CS::Disturb(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso, UINT _i, UINT _j, float _magnitude)
{
	// CS를 돌릴 준비한다.
	_cmdList->SetPipelineState(_pso);
	_cmdList->SetComputeRootSignature(_rootSig);

	UINT disturbIndex[2] = { _j, _i };
	_cmdList->SetComputeRoot32BitConstants(0, 1, &_magnitude, 3); // 3번째
	_cmdList->SetComputeRoot32BitConstants(0, 2, disturbIndex, 4); // 4번째

	_cmdList->SetComputeRootDescriptorTable(3, m_CurrSolUav);

	// CS가 값을 읽고, 결과를 바로 뱉을 수 있게, UA state로 바꾼다.
	//D3D12_RESOURCE_BARRIER currsol_READ_UA = CD3DX12_RESOURCE_BARRIER::Transition(m_CurrSol.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
	//_cmdList->ResourceBarrier(1, &currsol_READ_UA);

	// 스레드를 생성한다.
	_cmdList->Dispatch(1, 1, 1);
}
