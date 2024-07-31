#include "pch.h"
#include "D3D12Renderer_Client.h"
#include <DirectXColors.h>

bool D3D12Renderer_Client::Initialize(HWND _hWnd)
{
	// ������ ����
	m_hWnd = _hWnd;

	// SRWLock �ʱ�ȭ
	InitializeSRWLock(&m_srwLock);

	// debug layer�� �Ѵµ� ����ϴ� interface
	Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController = nullptr;
	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;

	// DXGI ��ü�� �����ϴ� interface
	Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory = nullptr;

	// display subsystem�� ������ �˾Ƴ��� interface
	Microsoft::WRL::ComPtr<IDXGIAdapter4> pAdaptor = nullptr;
	DXGI_ADAPTER_DESC3 AdaptorDesc = {};

	//#0 Debug Layer ����
	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController)))) {
		pDebugController->EnableDebugLayer();
	}
	dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
	Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController6 = nullptr;
	if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(pDebugController6.GetAddressOf()))) {
		// �� interface�� GBV�� ���ش�.
		pDebugController6->SetEnableGPUBasedValidation(TRUE);
		pDebugController6->SetEnableAutoName(TRUE);
	}

	// #1 DXGIFactory ����
	CreateDXGIFactory2(0, IID_PPV_ARGS(&pFactory));

	// #2 D3DDevice ����
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1, // ������� ����Ʈ���̽��� �����Ѵ�.
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, // �⺻ ����� ����ε� ����ϴ�.
	};

	DWORD FeatureLevelNum = _countof(featureLevels);

	for (DWORD flIndex = 0; flIndex < FeatureLevelNum; flIndex++) {
		UINT adaptorIndex = 0;
		IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(pAdaptor.GetAddressOf());
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adaptorIndex, pTempAdaptor)) {
			pAdaptor->GetDesc3(&AdaptorDesc);
			if (SUCCEEDED(D3D12CreateDevice(pAdaptor.Get(), featureLevels[flIndex], IID_PPV_ARGS(&m_pD3DDevice)))) {
				goto EXIT;
			}
			(*pTempAdaptor)->Release();
			(*pTempAdaptor) = nullptr;
			adaptorIndex++;
		}
	}
EXIT:
	if (!m_pD3DDevice)
	{
		__debugbreak();
		return false;
	}
	m_pD3DDevice->SetName(L"Device");
	m_AdaptorDesc = AdaptorDesc;

	// #3 Command Queue ����
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	if (FAILED(m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue)))) {
		__debugbreak();
	}
	m_pCommandQueue->SetName(L"Command Queue");

	// #4 RTV �� Descriptor Heap ����
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = THREAD_NUMBER_BY_FRAME;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV; 
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if (FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf())))) {
		__debugbreak();
	}
	m_pRTVHeap->SetName(L"Render Target Heap");
	m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// #5 Swap Chain ����
	RECT rect;
	GetClientRect(_hWnd, &rect);
	UINT uiWndWidth = rect.right - rect.left;
	UINT uiWndHeight = rect.bottom - rect.top;

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = uiWndWidth;
	swapChainDesc.Height = uiWndHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = THREAD_NUMBER_BY_FRAME;
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // Ƽ� ������ ���� �ʴ´�.

	m_dwSwapChainFlags = swapChainDesc.Flags;

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
	fsSwapChainDesc.Windowed = TRUE;

	Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
	// DXGI�� ������ �ڵ��� ������ swapchain�� �����.
	if (FAILED(pFactory->CreateSwapChainForHwnd(
		m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
		nullptr, pSwapChain1.GetAddressOf()
	)))
	{
		__debugbreak();
	}
	pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
	m_uiTextureIndexByThread = m_pSwapChain->GetCurrentBackBufferIndex();

	// Window�� ���缭 Viewport�� Scissor Rect�� �����Ѵ�.
	m_Viewport.Width = static_cast<float>(uiWndWidth);
	m_Viewport.Height = static_cast<float>(uiWndHeight);
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;

	m_ScissorRect.left = 0;
	m_ScissorRect.right = uiWndWidth;
	m_ScissorRect.top = 0;
	m_ScissorRect.bottom = uiWndHeight;
	// �ɹ��� ä���.
	m_dwWidth = static_cast<DWORD>(uiWndWidth);
	m_dwHeight = static_cast<DWORD>(uiWndHeight);

	// #6 �� Frame�� ���� Frame Resource�� �����.
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

	for (UINT i = 0; i < THREAD_NUMBER_BY_FRAME; i++) {
		// Descriptor Heap�� Render Target ������ �ϴ� SwapChain�� ����Ʈ ���۸�
		// ID3D12Resource���� �����ͼ�
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
		// �׸��� �װ��� ����Ű�� view(������ ������)�� ���������� ����Ÿ������ �� �� �ְ� �Ѵ�.
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		// RTV Heap handle�� �������� �Ű� �� ���۵� �Ȱ��� �����Ѵ�.
		rtvHandle.Offset(1, m_rtvDescriptorSize);

		m_pRenderTargets[i]->SetName(L"Render Target Resource");
	}
	//m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// #7 Command Allocator�� Command List ����
	if (FAILED(m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_pCommandAllocator.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandAllocator->SetName(L"Command Allocator");
	if (FAILED(m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(m_pCommandList.GetAddressOf())))) {
		__debugbreak();
	}
	m_pCommandList->SetName(L"Command List");
	m_pCommandList->Close();
	
	// #8 Fence ����
	if (FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}
	m_pFence->SetName(L"Fence");
	m_ui64FenceValue = 0;
	m_hCopyFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	/*
	// #9 Root Signature ����
	Microsoft::WRL::ComPtr<ID3DBlob> pSignatureBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;
	
	CD3DX12_DESCRIPTOR_RANGE rangeSrv[1] = {};
	rangeSrv[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER rootParameters[1] = {};
	rootParameters[0].InitAsDescriptorTable(_countof(rangeSrv), rangeSrv, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 0, nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, pSignatureBlob.GetAddressOf(), pErrorBlob.GetAddressOf()))) {
		OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
		__debugbreak();
	}
	if (FAILED(m_pD3DDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), IID_PPV_ARGS(m_pRootSignature.GetAddressOf())))) {
		__debugbreak();
	}

	// #10 Pipeline State Object ����
	Microsoft::WRL::ComPtr<ID3DBlob> pVertexShaderBlob = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> pPixelShaderBlob = nullptr;

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
	psoDesc.SampleMask = UINT_MAX; // �� ȥ�� 0���� �ʱ�ȸ �ȴ�.

	if (FAILED(m_pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
		__debugbreak();
	}

	// #11 SRV Heap ����
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = THREAD_NUMBER_BY_FRAME;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	m_pD3DDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(m_pSRVHeap.GetAddressOf()));
	*/

	// #12 ���ε� �ؽ��� ���ҽ� ����
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
	CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

	for (UINT i = 0; i < THREAD_NUMBER_BY_FRAME; i++) 
	{
		m_pD3DDevice->CreateCommittedResource(
			&defaultHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&textureDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_pDefaultTexture[i].GetAddressOf())
		);

		const UINT uploadBufferSize = GetRequiredIntermediateSize(m_pRenderTargets[i].Get(), 0, 1);
		CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

		if (FAILED(m_pD3DDevice->CreateCommittedResource(
			&uploadHeapProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(m_pUploadTexture[i].GetAddressOf())
		)))
		{
			__debugbreak();
		}

		CD3DX12_RANGE readRange(0, 0);
		m_pUploadTexture[i]->Map(0, &readRange, reinterpret_cast<void**>(m_ppMappedData + i));
	}


	return true;
}

/*

0. �ϴ� UDP�� �Ѿ�� �ؽ��ĸ� �ϼ��ؼ� �޸𸮿� ������ �ִ´�.
1. draw�� ������, �ؽ��� ���ε带 command list�� �Ǵ�.
2. �ؽ��� ���ε尡 ������ ����ü�ο� ���縦 �Ǵ�.
---
Command List�� ������ �ΰ�, ��Ƽ�������� �ϴ� ���� �ʹ� �����ϴ�,
��� �ϼ����� ���Ѿ߰ڴ�.
*/

void D3D12Renderer_Client::DrawStreamPixels(UINT8* _pPixels, UINT64 _ui64TotalBytes)
{
	BeginRender();
	UploadStreamPixels(_pPixels, _ui64TotalBytes);
	EndRender();
	Present();
}

void D3D12Renderer_Client::BeginRender()
{
	WaitForCopyFenceValue(m_pui64CopyFenceValue[m_uiTextureIndexByThread]);

	if (FAILED(m_pCommandAllocator->Reset())) {
		__debugbreak();
	}

	if (FAILED(m_pCommandList->Reset(m_pCommandAllocator.Get(), nullptr))) {
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiTextureIndexByThread, m_rtvDescriptorSize);
	D3D12_RESOURCE_BARRIER trans_PRESENT_DEST = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiTextureIndexByThread].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->ResourceBarrier(1, &trans_PRESENT_DEST);
}

void D3D12Renderer_Client::UploadStreamPixels(UINT8* _pPixels, UINT64 _ui64TotalBytes)
{
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = _pPixels;
	textureData.RowPitch = m_dwWidth * 4;
	textureData.SlicePitch = _ui64TotalBytes;

	UpdateSubresources(m_pCommandList.Get(), m_pDefaultTexture[m_uiTextureIndexByThread].Get(), m_pUploadTexture[m_uiTextureIndexByThread].Get(), 0, 0, 1, &textureData);

	//memcpy(m_ppMappedData[m_uiTextureIndexByThread], _pPixels, _ui64TotalBytes);

	CD3DX12_RESOURCE_BARRIER barrier_READ_SRC = CD3DX12_RESOURCE_BARRIER::Transition(m_pDefaultTexture[m_uiTextureIndexByThread].Get(), D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_COPY_SOURCE);
	m_pCommandList->ResourceBarrier(1, &barrier_READ_SRC);

	m_pCommandList->CopyResource(m_pRenderTargets[m_uiTextureIndexByThread].Get(), m_pDefaultTexture[m_uiTextureIndexByThread].Get());

	CD3DX12_RESOURCE_BARRIER barrier_SRC_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_pDefaultTexture[m_uiTextureIndexByThread].Get(), D3D12_RESOURCE_STATE_COPY_SOURCE,
		D3D12_RESOURCE_STATE_COPY_DEST);
	m_pCommandList->ResourceBarrier(1, &barrier_SRC_READ);
}

void D3D12Renderer_Client::EndRender()
{
	CD3DX12_RESOURCE_BARRIER barrier_DEST_SRV = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiTextureIndexByThread].Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
	m_pCommandList->ResourceBarrier(1, &barrier_DEST_SRV);

	m_pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { m_pCommandList.Get() };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
}

void D3D12Renderer_Client::Present()
{
	DoCopyFence();

	UINT SyncInterval = 0; // Vsync on , 0�̸� Vsync�� off �ϴ� ���̴�.
	// �̰� Ű�� �������� ��������.

	UINT uiSyncInterval = SyncInterval;
	UINT uiPresentFlags = 0;

	// ������� �ֻ����� GPU Rendering �ֱ���� ���̿��� ����� ȭ���� �������� �����̴�.
	if (!uiSyncInterval) {
		// �̷��� �ؾ� Tearing(ȭ�� ������)�� �����ϰ�
		// Vsync�� ���ش�.
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	// ���簡 �Ϸ�Ǹ� ����۸� �ٲ۴�.
	WaitForCopyFenceValue(m_pui64CopyFenceValue[m_uiTextureIndexByThread]);
	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr)
	{
		__debugbreak();
	}
	m_uiTextureIndexByThread = m_pSwapChain->GetCurrentBackBufferIndex();
}

UINT64 D3D12Renderer_Client::DoCopyFence()
{
	IncreaseFenceValue_Wait();
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	m_pui64CopyFenceValue[m_uiTextureIndexByThread] = m_ui64FenceValue;

	return m_ui64FenceValue;
}

void D3D12Renderer_Client::WaitForCopyFenceValue(UINT64 _expectedFenceValue)
{
	if (m_pFence->GetCompletedValue() < _expectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(_expectedFenceValue, m_hCopyFenceEvent);
		WaitForSingleObject(m_hCopyFenceEvent, INFINITE);
	}
}

void D3D12Renderer_Client::IncreaseFenceValue_Wait()
{
	AcquireSRWLockExclusive(&m_srwLock);
	m_ui64FenceValue++;
	ReleaseSRWLockExclusive(&m_srwLock);
}

void D3D12Renderer_Client::CleanUpRenderer()
{
	DoCopyFence();

	for (DWORD i = 0; i < THREAD_NUMBER_BY_FRAME; i++) {
		WaitForCopyFenceValue(m_pui64CopyFenceValue[i]);
	}
}

D3D12Renderer_Client::D3D12Renderer_Client()
	: m_hWnd(nullptr), m_pD3DDevice(nullptr), m_pCommandQueue(nullptr), m_pCommandAllocator(nullptr),m_pCommandList(nullptr),
	m_ui64FenceValue(0), m_pui64CopyFenceValue{}, 
	m_FeatureLevel(D3D_FEATURE_LEVEL_11_0),
	m_AdaptorDesc{}, m_pSwapChain(nullptr), m_pRenderTargets{}, 
	m_pRTVHeap(nullptr), m_uiTextureIndexByThread(0),
	m_rtvDescriptorSize(0), m_pUploadTexture{}, m_ppMappedData{},
	m_dwSwapChainFlags(0),
	m_hCopyFenceEvent(nullptr),	m_pFence(nullptr),	
	m_Viewport{}, m_ScissorRect{}, m_dwWidth(0), m_dwHeight(0),
	m_srwLock()
	// m_pui64DrawFenceValue{}, m_pSRVHeap(nullptr), m_srvDescriptorSize(0), m_hDrawFenceEvent(nullptr)
{
}

D3D12Renderer_Client::~D3D12Renderer_Client()
{
	CleanUpRenderer();
}
