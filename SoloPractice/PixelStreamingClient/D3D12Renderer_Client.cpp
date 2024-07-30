#include "pch.h"
#include "D3D12Renderer_Client.h"

bool D3D12Renderer_Client::Initialize(HWND _hWnd)
{
	m_hWnd = _hWnd;

	// DXGI 개체를 생성하는 interface
	Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory = nullptr;
	// display subsystem의 스펙을 알아내는 interface
	Microsoft::WRL::ComPtr<IDXGIAdapter4> pAdaptor = nullptr;
	DXGI_ADAPTER_DESC3 AdaptorDesc = {};

	// #1 DXGIFactory 생성
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));

	// #2 D3DDevice 생성
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1, // 여기부터 레이트레이싱을 지원한다.
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, // 기본 기능은 여기로도 충분하다.
	};

	DWORD FeatureLevelNum = _countof(featureLevels);

	for (DWORD flIndex = 0; flIndex < FeatureLevelNum; flIndex++) {
		UINT adaptorIndex = 0;
		IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(pAdaptor.GetAddressOf());
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adaptorIndex, pTempAdaptor)) {
			pAdaptor->GetDesc3(&AdaptorDesc);
			if (SUCCEEDED(D3D12CreateDevice(pAdaptor.Get(), featureLevels[flIndex], IID_PPV_ARGS(&m_pD3DDevice)))) {
				__debugbreak();
			}
			(*pTempAdaptor)->Release();
			(*pTempAdaptor) = nullptr;
			adaptorIndex++;
		}
	}

	if (!m_pD3DDevice)
	{
		__debugbreak();
		return false;
	}
	m_pD3DDevice->SetName(L"Device");
	m_AdaptorDesc = AdaptorDesc;

	// #3 Command Queue 생성
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if (FAILED(m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)))) {
		__debugbreak();
	}
	m_pCommandQueue->SetName(L"Command Queue");

	// #4 RTV 용 Descriptor Heap 생성
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = 2;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; 
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf())))) {
		__debugbreak();
	}
	m_pRTVHeap->SetName(L"Render Target Heap");
	m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// #5 Swap Chain 생성
	RECT rect;
	GetClientRect(_hWnd, &rect);
	UINT uiWndWidth = rect.right - rect.left;
	UINT uiWndHeight = rect.bottom - rect.top;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = uiWndWidth;
	swapChainDesc.Height = uiWndHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // 티어링 현상을 막지 않는다.

	m_dwSwapChainFlags = swapChainDesc.Flags;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
	// DXGI와 윈도우 핸들을 가지고 swapchain을 만든다.
	if (FAILED(pFactory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
		nullptr, pSwapChain1.GetAddressOf()
	)))
	{
		__debugbreak();
	}
	pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	// Window에 맞춰서 Viewport와 Scissor Rect를 정의한다.
	m_Viewport.Width = static_cast<float>(uiWndWidth);
	m_Viewport.Height = static_cast<float>(uiWndHeight);
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;

	m_ScissorRect.left = 0;
	m_ScissorRect.right = uiWndWidth;
	m_ScissorRect.top = 0;
	m_ScissorRect.bottom = uiWndHeight;
	// 맴버도 채운다.
	m_dwWidth = static_cast<DWORD>(uiWndWidth);
	m_dwHeight = static_cast<DWORD>(uiWndHeight);

	// #6 각 Frame에 대해 Frame Resource를 만든다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < 2; i++) {
		// Descriptor Heap에 Render Target 역할을 하는 SwapChain의 프론트 버퍼를
		// ID3D12Resource으로 가져와서
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
		// 그리고 그것을 가리키는 view(일종의 포인터)를 설정함으로 렌더타겟으로 쓸 수 있게 한다.
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		// RTV Heap handle을 다음으로 옮겨 백 버퍼도 똑같이 설정한다.
		rtvHandle.Offset(1, m_rtvDescriptorSize);

		m_pRenderTargets[i]->SetName(L"Render Target Resource");
	}
	m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// #7 Command Allocator와 Command List 생성
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandAllocator->SetName(L"Command Allocator");
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandList->SetName(L"Command List");
	m_pCommandList->Close();
	
	// #8 Fence 생성
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}
	m_pFence->SetName(L"Fence");
	m_ui64FenceValue = 0;
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

	// #9 Root Signature 생성
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
	
	CD3DX12_DESCRIPTOR_RANGE rangeSrv[1] = {};
	rangeSrv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangeSrv), rangeSrv, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignatureBlob.GetAddressOf(), pErrorBlob.GetAddressOf()))) {
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}
	if (FAILED(m_pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf())))) {
		__debugbreak();
	}

	// #10 Pipeline State Object 생성
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

#if defined(_DEBUG)
	UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT compileFlas = 0;
#endif
	HRESULT hr;

	hr = D3DCompileFromFile(L".\\Screen.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VS", "vs_5_1", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}

	hr = D3DCompileFromFile(L".\\Screen.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PS", "ps_5_1", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
	if (FAILED(hr)) {
		if (pErrorBlob != nullptr)
			OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}

	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 3,D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

	psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	psoDesc.pRootSignature = m_pRootSignature.Get();
	psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
	psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);

	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	psoDesc.DepthStencilState.DepthEnable = false;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.SampleMask = UINT_MAX; // 지 혼자 0으로 초기회 된다.

	if (FAILED(m_pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
		__debugbreak();
	}

	// #11 디폴트/업로드 텍스쳐 리소스 생성
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = uiWndWidth;
	textureDesc.Height = uiWndHeight;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

	CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
		nullptr,
		IID_PPV_ARGS(m_pDefaultTexture.GetAddressOf())
	)))
	{
		__debugbreak();
	}

	const UINT uploadBufferSize = GetRequiredIntermediateSize(m_pDefaultTexture.Get(), 0, 1);
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_DEFAULT);
	CD3DX12_RESOURCE_DESC buffDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&buffDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(m_pUploadTexture.GetAddressOf())
	)))
	{
		__debugbreak();
	}

	// #12 SRV Heap 위 에 default texture SRV 만들기
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_pD3DDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pSRVHeap.GetAddressOf()));

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_pD3DDevice->CreateShaderResourceView(m_pDefaultTexture.Get(), &srvDesc, m_pSRVHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

void D3D12Renderer_Client::BeginRender()
{
}

void D3D12Renderer_Client::DrawStreamPixels(UINT8* _pPixels, UINT64 _ui64TotalBytes)
{
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = _pPixels;
	textureData.RowPitch = m_dwWidth * 4;
	textureData.SlicePitch = _ui64TotalBytes;

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pDefaultTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);

	UpdateSubresources(m_pCommandList.Get(), m_pDefaultTexture.Get(), m_pUploadTexture.Get(), 0, 0, 1, &textureData);

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pDefaultTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
}

void D3D12Renderer_Client::EndRender()
{
}

void D3D12Renderer_Client::Present()
{
}
