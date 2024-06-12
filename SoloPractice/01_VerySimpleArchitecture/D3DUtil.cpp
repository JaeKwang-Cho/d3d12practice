// D3DUtil.cpp from "megayuchi"

#include "pch.h"
#include "D3DUtil.h"

void GetHardwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor)
{
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adaptor = nullptr;
	*_ppAdaptor = nullptr;

	IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(adaptor.GetAddressOf());
	for (UINT aI = 0; DXGI_ERROR_NOT_FOUND != _pFactory->EnumAdapters1(aI, pTempAdaptor); aI++) {
		DXGI_ADAPTER_DESC3 desc;
		adaptor->GetDesc3(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {
			// basic render driver adaptor�� �������� �ʴ´�.
			continue;
		}
		// adaptor�� d3d12�� �����ϴ��� Ȯ�θ� �Ѵ�. (device ������ ���Ѵ�.)
		if (SUCCEEDED(D3D12CreateDevice(adaptor.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
			break;
		}
	}
	*_ppAdaptor = adaptor.Get();
}

void GetSoftwareAdaptor(IDXGIFactory7* _pFactory, IDXGIAdapter4** _ppAdaptor)
{
	Microsoft::WRL::ComPtr<IDXGIAdapter4> adaptor = nullptr;
	*_ppAdaptor = nullptr;

	IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(adaptor.GetAddressOf());
	for (UINT aI = 0; DXGI_ERROR_NOT_FOUND != _pFactory->EnumAdapters1(aI, pTempAdaptor); aI++) {
		DXGI_ADAPTER_DESC3 desc;
		adaptor->GetDesc3(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) {		
			// adaptor�� d3d12�� �����ϴ��� Ȯ�θ� �Ѵ�. (device ������ ���Ѵ�.)
			if (SUCCEEDED(D3D12CreateDevice(adaptor.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) {
				*_ppAdaptor = adaptor.Get();
				break;
			}
		}
	}
}

void SetDebugLayerInfo(ID3D12Device* _pD3DDevice)
{
	// �굵 interface�� �� debug ������ ���ִ� ���̴�.
	Microsoft::WRL::ComPtr<ID3D12InfoQueue> pInfoQueue = nullptr;
	_pD3DDevice->QueryInterface(IID_PPV_ARGS(pInfoQueue.GetAddressOf()));

	if (pInfoQueue) {
		// GPU���� ������ �ؾ��ϴ� heap�� corruption �Ǿ��� ��, ���ܸ� �������� �����Ѵ�.
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		// API�� ����Ҷ� error�� �߻��ϸ�, ���ܸ� �������� �����Ѵ�.
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_INFO, TRUE);
		// (�� ó�� ����Ʈ����� ������ �����ϰ�, directx ��Ʈ�� ��ο����� ������ �����ϴ�.)

		// (�Ʒ� ��Ȳ�� ���, ������ �������� ����� ���̴�. from megayuchi)
		D3D12_MESSAGE_ID hide[] =
		{
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_EXECUTECOMMANDLISTS_WRONGSWAPCHAINBUFFERREFERENCE,
			//D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = (UINT)_countof(hide);
		filter.DenyList.pIDList = hide;

		pInfoQueue->AddStorageFilterEntries(&filter);
	}
}

void SetDefaultSamplerDesc(D3D12_STATIC_SAMPLER_DESC* _pOutSamplerDesc, UINT _RegisterIndex)
{
	D3D12_STATIC_SAMPLER_DESC sampler = {};
	//_pOutSamperDesc->Filter = D3D12_FILTER_ANISOTROPIC;
	_pOutSamplerDesc->Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	_pOutSamplerDesc->AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_pOutSamplerDesc->AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_pOutSamplerDesc->AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	_pOutSamplerDesc->MipLODBias = 0.0f;
	_pOutSamplerDesc->MaxAnisotropy = 16;
	_pOutSamplerDesc->ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	_pOutSamplerDesc->BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	_pOutSamplerDesc->MinLOD = -FLT_MAX;
	_pOutSamplerDesc->MaxLOD = D3D12_FLOAT32_MAX;
	_pOutSamplerDesc->ShaderRegister = _RegisterIndex;
	_pOutSamplerDesc->RegisterSpace = 0;
	_pOutSamplerDesc->ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
}

HRESULT CreateSimpleVertexBuffer(ID3D12Device* _pDevice, UINT _SizePerVertex, DWORD _dwVertexNum, D3D12_VERTEX_BUFFER_VIEW* _pOutVertexBufferView, ID3D12Resource** _ppOutBuffer)
{
	HRESULT hr = S_OK;

	D3D12_VERTEX_BUFFER_VIEW	VertexBufferView = {};
	Microsoft::WRL::ComPtr<ID3D12Resource> pVertexBuffer = nullptr;
	UINT VertexBufferSize = _SizePerVertex * _dwVertexNum;

	// Vertex Buffer�� �����Ѵ�.
	D3D12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC vbResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(VertexBufferSize);

	hr = _pDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&vbResourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&pVertexBuffer));

	if (FAILED(hr))
	{
		goto RETURN;
	}

	// vertex buffer view�� �ʱ�ȭ �Ѵ�.
	VertexBufferView.BufferLocation = pVertexBuffer->GetGPUVirtualAddress();
	VertexBufferView.StrideInBytes = _SizePerVertex;
	VertexBufferView.SizeInBytes = VertexBufferSize;

	*_pOutVertexBufferView = VertexBufferView;
	*_ppOutBuffer = pVertexBuffer.Get();

RETURN:
	return hr;
}
