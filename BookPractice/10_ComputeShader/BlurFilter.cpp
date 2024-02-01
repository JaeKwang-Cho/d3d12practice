//***************************************************************************************
// BlurFilter.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "BlurFilter.h"

BlurFilter::BlurFilter(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format)
	:m_d3dDevice(_device), m_Width(_width), m_Height(_height), m_Format(_format)
{
	BuildResources();
}

ID3D12Resource* BlurFilter::Output()
{
	return m_BlurMap0.Get();
}

void BlurFilter::BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor, UINT _descriptorSize)
{
	// 미리 뷰 핸들을 얻어 놓는다.
	m_Blur0CpuSrv = _hCpuDescriptor;
	m_Blur0CpuUav = _hCpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1CpuSrv = _hCpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1CpuUav = _hCpuDescriptor.Offset(1, _descriptorSize);

	m_Blur0GpuSrv = _hGpuDescriptor;		 
	m_Blur0GpuUav = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1GpuSrv = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1GpuUav = _hGpuDescriptor.Offset(1, _descriptorSize);

	// 그리고 뷰를 만든다.
	BuildDescriptors();
}

void BlurFilter::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		// 텍스쳐를 새로 만들고
		BuildResources();
		// 뷰도 새로 만든다.
		BuildDescriptors();
	}
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _horzBlurPSO, ID3D12PipelineState* _vertBlurPSO, ID3D12Resource* _input, int _blurCount)
{
	// 일단은 시그마를 2.5로 가우시안 블러를 건다.
	std::vector<float> weights = CalcGaussWeights(2.5f);
	int blurRadius = (int)weights.size() / 2;

	// GPU가 작업을 할 차례다.
	_cmdList->SetComputeRootSignature(_rootSig);

	// #0 맴버로 가지는 blur 속성들을 쉐이더에 넘겨준다.
	_cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	_cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

	// 일단 blur 하기 전에 그려진 백 버퍼 텍스쳐를 가져올 준비를 하고
	D3D12_RESOURCE_BARRIER inputBarrier_RT_COPYS = CD3DX12_RESOURCE_BARRIER::Transition(
		_input, 
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	_cmdList->ResourceBarrier(1, &inputBarrier_RT_COPYS);
	// 백 버퍼 텍스쳐를 맴버 텍스쳐에 붙여넣을 준비를 하고
	D3D12_RESOURCE_BARRIER blur0Barrier_COMM_COPYD = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	_cmdList->ResourceBarrier(1, &blur0Barrier_COMM_COPYD);
	// #1 첫번째 텍스쳐에 복사한다.
	_cmdList->CopyResource(m_BlurMap0.Get(), _input);

	// 이제 srv와 uav를 이용해서 blur를 먹일 차례다
	// D3D12_RESOURCE_STATE_GENERIC_READ 는 업로드 힙에 사용할 수 있는 상태로 일단 만드는 것이다.
	D3D12_RESOURCE_BARRIER blur0Barrier_COPYD_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);
	_cmdList->ResourceBarrier(1, &blur0Barrier_COPYD_GREAD);
	// CS의 출력으로 사용이 가능하도록 UAV 상태로 만든 것이다.
	D3D12_RESOURCE_BARRIER blur1Barrier_COMM_UODER= CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap1.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	_cmdList->ResourceBarrier(1, &blur1Barrier_COMM_UODER);

	// 블러 여러번 먹일 수록 더 흐려질 것이다.
	for (int i = 0; i < _blurCount; i++)
	{
		// #2 일단 수평 블러를 먼저 먹인다.
		// PSO를 세팅하고
		_cmdList->SetPipelineState(_horzBlurPSO);
		// Srv와 Uav를 세팅한다.
		// 0번 srv에서 가져와서 CS를 거쳐서 1번 uav에 출력하는 것이다.
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur0GpuSrv);
		_cmdList->SetComputeRootDescriptorTable(2, m_Blur1GpuUav);

		// #3 CS를 작동시킬 스레드 그룹을 생성한다.
		// (쉐이더 파일 내부에서 스레드가 256개 단위로, 클라이언트 영역을 커버하기로 하였다.)
		// 수평 블러를 먹일때 사용할 스레드 그룹 수 만큼 dispatch를 건다.
		UINT numGroupsX = (UINT)ceilf(m_Width / 256.f);
		_cmdList->Dispatch(numGroupsX, m_Height, 1);

		// 입력을 하던 첫번째 텍스쳐의 업로드 힙(GENERIC_READ)에서 사용하던 상태(srv)를, UNORDERED_ACCESS 상태(uav)로 바꾼다.
		D3D12_RESOURCE_BARRIER blur0_GREAD_UODER = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		_cmdList->ResourceBarrier(1, &blur0_GREAD_UODER);
		// CS의 출력(uav)을 받던 두번째 택스쳐를 이제 업로드 힙(GENERIC_READ) 상태로(srv)로 바꾼다.
		D3D12_RESOURCE_BARRIER blur1_UODER_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		_cmdList->ResourceBarrier(1, &blur1_UODER_GREAD);

		// #3 이제 수직 블러를 먹인다.
		// PSO를 세팅하고
		_cmdList->SetPipelineState(_vertBlurPSO);
		// Srv와 Uav를 세팅한다.
		// 1번 srv에서 가져와서 CS를 거쳐서 0번 uav에 출력하는 것이다.
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur1GpuSrv);
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur0GpuUav);

		// 수직 블러를 먹일 스레드 개수를 정해서, 스레드 그룹을 생성한다.
		UINT numGroupY = (UINT)ceilf(m_Height / 256.f);
		_cmdList->Dispatch(m_Width, numGroupY, 1);

		// 이제 0번 텍스쳐를 Uav 상태에서, GENERIC_READ 상태로 바꾼다.
		D3D12_RESOURCE_BARRIER blur0_UODER_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		_cmdList->ResourceBarrier(1, &blur0_UODER_GREAD);
		// 블러를 여러번 먹일 때를 대비해서, 1번은 미리 UAV로 만들어 놓ㄴ는다.
		D3D12_RESOURCE_BARRIER blur1_GREAD_UODER = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap1.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		_cmdList->ResourceBarrier(1, &blur1_GREAD_UODER);
	}
}

std::vector<float> BlurFilter::CalcGaussWeights(float _sigma)
{
	float twoSigma2 = 2.f * _sigma * _sigma;

	// 주변 픽셀과 타겟 픽셀의 가중치를 이용해서 blur를 먹이는데
	// 여기서는 가우시안 커브를 이용해서 그 정도를 구한다.

	// 시그마가 클 수록 섞는 픽셀들이 많다.
	int blurRadius = (int)ceil(2.f * _sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);


	float weightSum = 0.f;
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = (float)i;
		
		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// 가중치들의 합이 1이 되도록 만든다.
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void BlurFilter::BuildDescriptors()
{
	// 같은 포멧으로 Srv, Uav를 하나씩 만든다.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = m_Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.Format = m_Format;
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Texture2D.MipSlice = 0;

	// 같은 리소스에 대해서 View를 작성해도 된다.
	// Srv는 입력으로, Uav는 출력으로 작동한다.
	m_d3dDevice->CreateShaderResourceView(m_BlurMap0.Get(), &srvDesc, m_Blur0CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap0.Get(), nullptr, &uavDesc, m_Blur0CpuUav);
	// 카운터 오프셋의 의미는... 힙을 점프하는데 쓰이는 리소스를..뜻하는 걸까?
	m_d3dDevice->CreateShaderResourceView(m_BlurMap1.Get(), &srvDesc, m_Blur0CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap1.Get(), nullptr, &uavDesc, m_Blur1CpuUav);
}

void BlurFilter::BuildResources()
{
	// 참고 : 압축 포멧은 Uav에서 에러를 뱉는다. 

	// 맴버로 받은 포멧으로, Blur 결과를 기록할 맴버 텍스쳐를 생성한다.
	D3D12_RESOURCE_DESC texDesc;
	ZeroMemory(&texDesc, sizeof(texDesc));
	texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	texDesc.Alignment = 0;
	texDesc.Width = m_Width;
	texDesc.Height = m_Height;
	texDesc.DepthOrArraySize = 1;
	texDesc.MipLevels = 1;
	texDesc.Format = m_Format;
	texDesc.SampleDesc.Count = 1;
	texDesc.SampleDesc.Quality = 0;
	texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heapDefaultProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&heapDefaultProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_BlurMap0)
	));

	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&heapDefaultProps,
		D3D12_HEAP_FLAG_NONE,
		&texDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&m_BlurMap1)
	));
}
