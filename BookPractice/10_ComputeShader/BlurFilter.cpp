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
	// �̸� �� �ڵ��� ��� ���´�.
	m_Blur0CpuSrv = _hCpuDescriptor;
	m_Blur0CpuUav = _hCpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1CpuSrv = _hCpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1CpuUav = _hCpuDescriptor.Offset(1, _descriptorSize);

	m_Blur0GpuSrv = _hGpuDescriptor;		 
	m_Blur0GpuUav = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1GpuSrv = _hGpuDescriptor.Offset(1, _descriptorSize);
	m_Blur1GpuUav = _hGpuDescriptor.Offset(1, _descriptorSize);

	// �׸��� �並 �����.
	BuildDescriptors();
}

void BlurFilter::OnResize(UINT _newWidth, UINT _newHeight)
{
	if ((m_Width != _newWidth) || (m_Height != _newHeight))
	{
		m_Width = _newWidth;
		m_Height = _newHeight;

		// �ؽ��ĸ� ���� �����
		BuildResources();
		// �䵵 ���� �����.
		BuildDescriptors();
	}
}

void BlurFilter::Execute(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _horzBlurPSO, ID3D12PipelineState* _vertBlurPSO, ID3D12Resource* _input, int _blurCount)
{
	// �ϴ��� �ñ׸��� 2.5�� ����þ� ���� �Ǵ�.
	std::vector<float> weights = CalcGaussWeights(2.5f);
	int blurRadius = (int)weights.size() / 2;

	// GPU�� �۾��� �� ���ʴ�.
	_cmdList->SetComputeRootSignature(_rootSig);

	// #0 �ɹ��� ������ blur �Ӽ����� ���̴��� �Ѱ��ش�.
	_cmdList->SetComputeRoot32BitConstants(0, 1, &blurRadius, 0);
	_cmdList->SetComputeRoot32BitConstants(0, (UINT)weights.size(), weights.data(), 1);

	// �ϴ� blur �ϱ� ���� �׷��� �� ���� �ؽ��ĸ� ������ �غ� �ϰ�
	D3D12_RESOURCE_BARRIER inputBarrier_RT_COPYS = CD3DX12_RESOURCE_BARRIER::Transition(
		_input, 
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);
	_cmdList->ResourceBarrier(1, &inputBarrier_RT_COPYS);
	// �� ���� �ؽ��ĸ� �ɹ� �ؽ��Ŀ� �ٿ����� �غ� �ϰ�
	D3D12_RESOURCE_BARRIER blur0Barrier_COMM_COPYD = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_COPY_DEST
	);
	_cmdList->ResourceBarrier(1, &blur0Barrier_COMM_COPYD);
	// #1 ù��° �ؽ��Ŀ� �����Ѵ�.
	_cmdList->CopyResource(m_BlurMap0.Get(), _input);

	// ���� srv�� uav�� �̿��ؼ� blur�� ���� ���ʴ�
	// D3D12_RESOURCE_STATE_GENERIC_READ �� ���ε� ���� ����� �� �ִ� ���·� �ϴ� ����� ���̴�.
	D3D12_RESOURCE_BARRIER blur0Barrier_COPYD_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap0.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_GENERIC_READ
	);
	_cmdList->ResourceBarrier(1, &blur0Barrier_COPYD_GREAD);
	// CS�� ������� ����� �����ϵ��� UAV ���·� ���� ���̴�.
	D3D12_RESOURCE_BARRIER blur1Barrier_COMM_UODER= CD3DX12_RESOURCE_BARRIER::Transition(
		m_BlurMap1.Get(),
		D3D12_RESOURCE_STATE_COMMON,
		D3D12_RESOURCE_STATE_UNORDERED_ACCESS
	);
	_cmdList->ResourceBarrier(1, &blur1Barrier_COMM_UODER);

	// �� ������ ���� ���� �� ����� ���̴�.
	for (int i = 0; i < _blurCount; i++)
	{
		// #2 �ϴ� ���� ���� ���� ���δ�.
		// PSO�� �����ϰ�
		_cmdList->SetPipelineState(_horzBlurPSO);
		// Srv�� Uav�� �����Ѵ�.
		// 0�� srv���� �����ͼ� CS�� ���ļ� 1�� uav�� ����ϴ� ���̴�.
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur0GpuSrv);
		_cmdList->SetComputeRootDescriptorTable(2, m_Blur1GpuUav);

		// #3 CS�� �۵���ų ������ �׷��� �����Ѵ�.
		// (���̴� ���� ���ο��� �����尡 256�� ������, Ŭ���̾�Ʈ ������ Ŀ���ϱ�� �Ͽ���.)
		// ���� ���� ���϶� ����� ������ �׷� �� ��ŭ dispatch�� �Ǵ�.
		UINT numGroupsX = (UINT)ceilf(m_Width / 256.f);
		_cmdList->Dispatch(numGroupsX, m_Height, 1);

		// �Է��� �ϴ� ù��° �ؽ����� ���ε� ��(GENERIC_READ)���� ����ϴ� ����(srv)��, UNORDERED_ACCESS ����(uav)�� �ٲ۴�.
		D3D12_RESOURCE_BARRIER blur0_GREAD_UODER = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS
		);
		_cmdList->ResourceBarrier(1, &blur0_GREAD_UODER);
		// CS�� ���(uav)�� �޴� �ι�° �ý��ĸ� ���� ���ε� ��(GENERIC_READ) ���·�(srv)�� �ٲ۴�.
		D3D12_RESOURCE_BARRIER blur1_UODER_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap1.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		_cmdList->ResourceBarrier(1, &blur1_UODER_GREAD);

		// #3 ���� ���� ���� ���δ�.
		// PSO�� �����ϰ�
		_cmdList->SetPipelineState(_vertBlurPSO);
		// Srv�� Uav�� �����Ѵ�.
		// 1�� srv���� �����ͼ� CS�� ���ļ� 0�� uav�� ����ϴ� ���̴�.
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur1GpuSrv);
		_cmdList->SetComputeRootDescriptorTable(1, m_Blur0GpuUav);

		// ���� ���� ���� ������ ������ ���ؼ�, ������ �׷��� �����Ѵ�.
		UINT numGroupY = (UINT)ceilf(m_Height / 256.f);
		_cmdList->Dispatch(m_Width, numGroupY, 1);

		// ���� 0�� �ؽ��ĸ� Uav ���¿���, GENERIC_READ ���·� �ٲ۴�.
		D3D12_RESOURCE_BARRIER blur0_UODER_GREAD = CD3DX12_RESOURCE_BARRIER::Transition(
			m_BlurMap0.Get(),
			D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
			D3D12_RESOURCE_STATE_GENERIC_READ
		);
		_cmdList->ResourceBarrier(1, &blur0_UODER_GREAD);
		// ���� ������ ���� ���� ����ؼ�, 1���� �̸� UAV�� ����� �����´�.
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

	// �ֺ� �ȼ��� Ÿ�� �ȼ��� ����ġ�� �̿��ؼ� blur�� ���̴µ�
	// ���⼭�� ����þ� Ŀ�긦 �̿��ؼ� �� ������ ���Ѵ�.

	// �ñ׸��� Ŭ ���� ���� �ȼ����� ����.
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

	// ����ġ���� ���� 1�� �ǵ��� �����.
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}

	return weights;
}

void BlurFilter::BuildDescriptors()
{
	// ���� �������� Srv, Uav�� �ϳ��� �����.
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

	// ���� ���ҽ��� ���ؼ� View�� �ۼ��ص� �ȴ�.
	// Srv�� �Է�����, Uav�� ������� �۵��Ѵ�.
	m_d3dDevice->CreateShaderResourceView(m_BlurMap0.Get(), &srvDesc, m_Blur0CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap0.Get(), nullptr, &uavDesc, m_Blur0CpuUav);
	// ī���� �������� �ǹ̴�... ���� �����ϴµ� ���̴� ���ҽ���..���ϴ� �ɱ�?
	m_d3dDevice->CreateShaderResourceView(m_BlurMap1.Get(), &srvDesc, m_Blur0CpuSrv);
	m_d3dDevice->CreateUnorderedAccessView(m_BlurMap1.Get(), nullptr, &uavDesc, m_Blur1CpuUav);
}

void BlurFilter::BuildResources()
{
	// ���� : ���� ������ Uav���� ������ ��´�. 

	// �ɹ��� ���� ��������, Blur ����� ����� �ɹ� �ؽ��ĸ� �����Ѵ�.
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
