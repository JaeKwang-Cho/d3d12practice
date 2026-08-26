#include "pch.h"
#include "D3D12Renderer.h"
#include "RayTracingManager.h"
#include "ShaderManager.h"
#include "D3D12ResourceManager.h"
#include "ConstantBufferManager.h"
#include "SimpleConstantBufferPool.h"
#include "SingleDescriptorAllocator.h"
#include "DescriptorPool.h"
#include "FontManager.h"
#include "TextureManager.h"

#include "BasicMeshObject.h"
#include "SpriteObject.h"


bool D3D12Renderer::Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader, const WCHAR* _wchSahderPath, ULONG _ulMaxBlasCount)
{
	HRESULT hr = E_FAIL;
	// debug layer를 켜는데 사용하는 interface
	Microsoft::WRL::ComPtr<ID3D12Debug> pDebugController = nullptr;
	// DXGI 개체를 생성하는 interface
	Microsoft::WRL::ComPtr<IDXGIFactory7> pFactory = nullptr;
	// display subsystem의 스펙을 알아내는 interface
	Microsoft::WRL::ComPtr<IDXGIAdapter4> pAdaptor = nullptr;

	DXGI_ADAPTER_DESC3 AdaptorDesc = {};

	DWORD dwCreateFlags = 0;
	DWORD dwCreateFactoryFlags = 0;
	m_fDPI = static_cast<float>(GetDpiForWindow(_hWnd));

	// #1 GPU 디버그 레이어 설정
	if (_bEnableDebugLayer) {
		// 원래는 엄청 복잡한 모양으로 interface를 얻기가 어려웠는데,
		// 아래 헬퍼함수로 엄청 편해졌다고 한다.
		hr = D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugController));
		if (SUCCEEDED(hr)) {
			pDebugController->EnableDebugLayer();
		}

		// #2 GBV(GPU based Validation) 설정
		// 아래는 DXGI에 대해서 디버깅 플래그를 설정해주는 것이다.
		dwCreateFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
		// D3D12는 CPU 타임라인과, GPU 타임라인이 다르다.
		// (driver에서 돌아가는 것, shader에서 돌아가는 것이 다르다.)

		// 어쨌든 여기서 하는건, GPU에서 뭔가 문제가 생겨서 터질때 잡아준다는 것이다.
		if (_bEnableGBV) {
			// debug6라는 query interface를 가져온다.
			Microsoft::WRL::ComPtr<ID3D12Debug6> pDebugController6 = nullptr;
			if (S_OK == pDebugController->QueryInterface(IID_PPV_ARGS(pDebugController6.GetAddressOf()))) {
				// 그 interface로 GBV를 켜준다.
				pDebugController6->SetEnableGPUBasedValidation(TRUE);
				pDebugController6->SetEnableAutoName(TRUE);
			}
		}
	}

	// #3 DXGIFactory 생성
	// 옛날 DirectX는 두개(d3d + dxgi)가 하나에 있었는데, 그걸 2개로 분리했다.

	// dxgi를 사용하는 이유는 double-buffering을 하기 위함이다.
	CreateDXGIFactory2(dwCreateFactoryFlags, IID_PPV_ARGS(&pFactory));

	// GPU가 가지고 있는 기능들의 수준을 나타내는 것
	// (예전에는 GPU가 DirectX 표준에 맞춰서 기능을 제공하지 않았다. 그래서 모든 기능을 다 확인해야했다.)
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1, // 여기부터 레이트레이싱을 지원한다.
		D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0, // 기본 기능은 여기로도 충분하다.
	};

	DWORD FeatureLevelNum = _countof(featureLevels);

	// #4 GPU feature level에 맞는 D3DDevice 생성
	for (DWORD flIndex = 0; flIndex < FeatureLevelNum; flIndex++) {
		UINT adaptorIndex = 0;
		// DXGI가 가진 기능중에 그래픽 카드를 연결하는 기능도 있다.
		// DXGIFactory에서 어뎁터를 얻어와서
		IDXGIAdapter1** pTempAdaptor = reinterpret_cast<IDXGIAdapter1**>(pAdaptor.GetAddressOf());
		while (DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adaptorIndex, pTempAdaptor)) {
			pAdaptor->GetDesc3(&AdaptorDesc);
			// 그래픽 카드들을 확인해보면서 피쳐 레벨을 확인하는 것이다.
			// GPU에다가 D3DDevice를 생성해보고 성공하면 해당 feature level을 가지고 있는 것이다.
			if (SUCCEEDED(D3D12CreateDevice(pAdaptor.Get(), featureLevels[flIndex], IID_PPV_ARGS(&m_pD3DDevice)))) {
				goto EXIT;
			}
			// 형변환을 해서 그런가.. 스마트 포인터로 해제가 안된다.
			(*pTempAdaptor)->Release();
			(*pTempAdaptor) = nullptr;
			adaptorIndex++;
		}
	}
EXIT:
	// d3ddevice를 생성하고
	if (!m_pD3DDevice) {
		__debugbreak();
		return false;
	}
	m_pD3DDevice->SetName(L"device");
	m_adapterDesc = AdaptorDesc;

	// 참고 : goto 아래에 이렇게 바디를 새로 만드는 이유는 goto 때문에, 분기가 될때는 바디가 있어야 변수를 초기화 할 수 있다. -> "컨트롤 전송"

	// d3d debug 추가 설정을 할 수 있다.
	if (pDebugController) {
		// D3DUtil.h
		SetDebugLayerInfo(m_pD3DDevice.Get());
	}

	// #5 매 프레임 마다 작성한 Command List가 올라갈 Command Queue를 생성
	{
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

		hr = m_pD3DDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_pCommandQueue));
		if (FAILED(hr)) {
			__debugbreak();
			return false;
		}
	}
	m_pCommandQueue->SetName(L"Command Queue");
	// #6 RTV와 depth-stencil 전용 Descriptor Heap을 생성한다.
	CreateDescriptorHeapForRTV();
	CreateDescriptorHeapForDSV();

	// #7 swap chain과 그에 필요한 ID3DResource를 만든다.
	// swapchain 속성을 지정하고
	{
		RECT rect;
		GetClientRect(_hWnd, &rect);
		UINT uiWndWidth = rect.right - rect.left;
		UINT uiWndHeight = rect.bottom - rect.top;

		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.Width = uiWndWidth;
		swapChainDesc.Height = uiWndHeight;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = SWAP_CHAIN_FRAME_COUNT;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.Scaling = DXGI_SCALING_NONE;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		swapChainDesc.Flags |= DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING; // 티어링 현상을 막지 않는다.

		m_uiSwapChainFlags = swapChainDesc.Flags;

		DXGI_SWAP_CHAIN_FULLSCREEN_DESC fsSwapChainDesc = {};
		fsSwapChainDesc.Windowed = TRUE;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> pSwapChain1 = nullptr;
		// DXGI와 윈도우 핸들을 가지고 swapchain을 만든다.
		hr = pFactory->CreateSwapChainForHwnd(
			m_pCommandQueue.Get(), _hWnd, &swapChainDesc, &fsSwapChainDesc,
			nullptr, pSwapChain1.GetAddressOf());
		if (FAILED(hr))
		{
			__debugbreak();
		}
		pSwapChain1->QueryInterface(IID_PPV_ARGS(&m_pSwapChain));
		m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

		// Window에 맞춰서 Viewport와 Scissor Rect를 정의한다.
		m_viewport.Width = static_cast<float>(uiWndWidth);
		m_viewport.Height = static_cast<float>(uiWndHeight);
		m_viewport.MinDepth = 0.f;
		m_viewport.MaxDepth = 1.f;

		m_scissorRect.left = 0;
		m_scissorRect.right = uiWndWidth;
		m_scissorRect.top = 0;
		m_scissorRect.bottom = uiWndHeight;
		// 맴버도 채운다.
		m_ulWidth = static_cast<ULONG>(uiWndWidth);
		m_ulHeight = static_cast<ULONG>(uiWndHeight);
	}

	// #8 각각의 Frame에 대해 Frame Resource를 만든다.
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());

		for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
			// Descriptor Heap에 Render Target 역할을 하는 SwapChain의 프론트 버퍼를
			// ID3D12Resource으로 가져와서
			m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
			// 그리고 그것을 가리키는 view(일종의 포인터)를 설정함으로 렌더타겟으로 쓸 수 있게 한다.
			m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
			// RTV Heap handle을 다음으로 옮겨 백 버퍼도 똑같이 설정한다.
			rtvHandle.Offset(1, m_rtvDescriptorSize);

			m_pRenderTargets[i]->SetName(L"Render Target Resource");
		}

		m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_srvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		m_dsvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	}
	
	// #9
	CreateCommandList();


	// #10 fence를 정의한다.
	// synchronization objects가 필요한 이유는, d3d12는 GPU에서 리소스를 사용하기 전에
	// 그것을 해제해 버릴 수 있다. 그래서 이렇게 fence를 쳐줘서 없애기 전에 확인 해준다.
	// d3d12는 완전 비둥기(asynchronous) api다.
	CreateFence();

	// #11 Renderer에서 사용하는 Manager들을 초기화한다.
	m_pShaderManager = std::make_unique<ShaderManager>();
	m_pShaderManager->Initialize(this, _wchSahderPath, _bDebugShader);

	m_pFontManager = std::make_unique<FontManager>();
	m_pFontManager->Initialize(this, m_pCommandQueue.Get(), m_ulWidth, m_ulHeight, _bEnableDebugLayer);

	m_pResourceManager = std::make_unique<D3D12ResourceManager>();
	m_pResourceManager->Initialize(m_pD3DDevice.Get());

	m_pTextureManager = std::make_unique<TextureManager>();
	m_pTextureManager->Initialize(this);

	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		m_ppDescriptorPool[i] = std::make_unique<DescriptorPool>();
		m_ppDescriptorPool[i]->Initialize(m_pD3DDevice.Get(), MAX_DRAW_COUNT_PER_FRAME * BasicMeshObject::MAX_DESCRIPTOR_COUNT_FOR_DRAW);

		m_ppConstantBufferManager[i] = std::make_unique<ConstantBufferManager>();
		m_ppConstantBufferManager[i]->Initialize(m_pD3DDevice.Get(), MAX_DRAW_COUNT_PER_FRAME);
	}

	m_pSingleDescriptorAllocator = std::make_unique<SingleDescriptorAllocator>();
	m_pSingleDescriptorAllocator->Initialize(m_pD3DDevice.Get(), MAX_DESCRIPTOR_COUNT, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	CreateDepthStencilBuffer(m_ulWidth, m_ulHeight);

	// for ray tracing
	m_pRayTracingManager = std::make_unique<RayTracingManager>();
	m_pRayTracingManager->Initialize(this, m_ulWidth, m_ulHeight, _ulMaxBlasCount);

	InitCamera();

	return true;
}

void D3D12Renderer::BeginRender()
{
	D3D12CommandAllocator_raw pCommandAllocator = m_ppCommandAllocator[m_ulCurContextIndex].Get();
	D3D12GraphicsCommandList_raw pCommandList = m_ppCommandList[m_ulCurContextIndex].Get();

	if(FAILED(pCommandAllocator->Reset())) {
		__debugbreak();
	}
	if (FAILED(pCommandList->Reset(pCommandAllocator, nullptr))) {
		__debugbreak();
	}

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart(), m_uiRenderTargetIndex, m_rtvDescriptorSize);

	auto barrier_PresentToRT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_pRenderTargets[m_uiRenderTargetIndex].Get(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &barrier_PresentToRT);

	pCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
	pCommandList->ClearDepthStencilView(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);

	pCommandList->RSSetViewports(1, &m_viewport);
	pCommandList->RSSetScissorRects(1, &m_scissorRect);
	pCommandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
}

void D3D12Renderer::EndRender()
{
	D3D12GraphicsCommandList_raw pCommandList = m_ppCommandList[m_ulCurContextIndex].Get();

	if (m_bDXREnabled) {
		if (m_pRayTracingManager->IsUpdatedAccelerationStructure())
		{
			for (DWORD i = 0; i < MAX_PENDING_FRAME_COUNT; i++)
			{
				WaitForFenceValue(m_pui64FenceValue[i]);
			}
			m_pRayTracingManager->UpdateAccelerationStructure();
		}
		m_pRayTracingManager->DoRayTracing(pCommandList);

		D3D12Resource_raw pRayTracingOutputResource = m_pRayTracingManager->INL_GetOutputResource();

		D3D12_RESOURCE_BARRIER preCopyBarrier[2];
		preCopyBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pRenderTargets[m_uiRenderTargetIndex].Get(),
			D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_DEST);
		preCopyBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(pRayTracingOutputResource, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		pCommandList->ResourceBarrier(ARRAYSIZE(preCopyBarrier), preCopyBarrier);

		pCommandList->CopyResource(m_pRenderTargets[m_uiRenderTargetIndex].Get(), pRayTracingOutputResource);

		D3D12_RESOURCE_BARRIER postCopyBarrier[2];
		postCopyBarrier[0] = CD3DX12_RESOURCE_BARRIER::Transition(
			m_pRenderTargets[m_uiRenderTargetIndex].Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		postCopyBarrier[1] = CD3DX12_RESOURCE_BARRIER::Transition(pRayTracingOutputResource, D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);

		pCommandList->ResourceBarrier(ARRAYSIZE(postCopyBarrier), postCopyBarrier);
	}
	else {
		D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_pRenderTargets[m_uiRenderTargetIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		pCommandList->ResourceBarrier(1, &barrier);
	}
	pCommandList->Close();

	ID3D12CommandList* ppCommandLists[] = { pCommandList };
	m_pCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

}

void D3D12Renderer::Present()
{
	DoFence();

	UINT uiSyncInterval = 0; // VSync Off / 1이면 VSync On
	UINT uiPresentFlags = 0;
	
	if (!uiSyncInterval) {
		uiPresentFlags = DXGI_PRESENT_ALLOW_TEARING;
	}

	HRESULT hr = m_pSwapChain->Present(uiSyncInterval, uiPresentFlags);

	if (DXGI_ERROR_DEVICE_REMOVED == hr) {
		__debugbreak();
	}
	
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	ULONG ulNextContextIndex = (m_ulCurContextIndex + 1) % MAX_PENDING_FRAME_COUNT;
	WaitForFenceValue(m_pui64FenceValue[ulNextContextIndex]);

	// reset resources per frame
	m_ppConstantBufferManager[ulNextContextIndex]->Reset_ConstantBufferManager();
	m_ppDescriptorPool[ulNextContextIndex]->Reset();
	m_ulCurContextIndex = ulNextContextIndex;
}

bool D3D12Renderer::UpdateWindowSize(ULONG _width, ULONG _height)
{
	if (!(_width * _height))
		return false;
	if(m_ulHeight == _width && m_ulHeight == _height)
		return false;

	DoFence();

	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64FenceValue[i]);
	}

	CleanupDepthStencilBuffer();

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	HRESULT hr = m_pSwapChain->GetDesc1(&swapChainDesc);
	if (FAILED(hr)) {
		__debugbreak();
		return false;
	}

	for(UINT n = 0; n < SWAP_CHAIN_FRAME_COUNT; n++) {
		m_pRenderTargets[n].Reset();
	}

	if(FAILED(m_pSwapChain->ResizeBuffers(SWAP_CHAIN_FRAME_COUNT, _width, _height, DXGI_FORMAT_R8G8B8A8_UNORM, m_uiSwapChainFlags))) {
		__debugbreak();
		return false;
	}
	m_uiRenderTargetIndex = m_pSwapChain->GetCurrentBackBufferIndex();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_pRTVHeap->GetCPUDescriptorHandleForHeapStart());
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		m_pSwapChain->GetBuffer(i, IID_PPV_ARGS(m_pRenderTargets[i].GetAddressOf()));
		m_pD3DDevice->CreateRenderTargetView(m_pRenderTargets[i].Get(), nullptr, rtvHandle);
		rtvHandle.Offset(1, m_rtvDescriptorSize);
		m_pRenderTargets[i]->SetName(L"Render Target Resource");
	}

	m_ulHeight = _height;
	m_ulWidth = _width;
	m_viewport.Width = static_cast<float>(m_ulWidth);
	m_viewport.Height = static_cast<float>(m_ulHeight);
	m_scissorRect.left = 0;
	m_scissorRect.top = 0;
	m_scissorRect.right = m_ulWidth;
	m_scissorRect.bottom = m_ulHeight;

	CreateDepthStencilBuffer(m_ulWidth, m_ulHeight);

	m_pRayTracingManager->UpdateWindowSize_forRayTracing(m_ulWidth, m_ulHeight);

	return true;
}

void D3D12Renderer::SetCameraPos(const float _x, const float _y, const float _z)
{
	m_vCamPos = XMVectorSet(_x, _y, _z, 1.f);
	UpdateCamera();
}

void D3D12Renderer::MoveCamera(const float _x, const float _y, const float _z)
{
	XMVECTOR CamMoveForward = XMVectorScale(m_vCamDir, _z);
	XMVECTOR CamMoveRight = XMVectorScale(m_vCamRight, _x);
	XMVECTOR CamMoveUp = XMVectorScale(m_vCamUp, _y);

	m_vCamPos = XMVectorAdd(m_vCamPos, CamMoveForward);
	m_vCamPos = XMVectorAdd(m_vCamPos, CamMoveRight);
	m_vCamPos = XMVectorAdd(m_vCamPos, CamMoveUp);
	m_vCamPos.m128_f32[3] = 1.f;

	UpdateCamera();
}

void D3D12Renderer::GetCameraPos(float& _outX, float& _outY, float& _outZ)
{
	_outX = m_vCamPos.m128_f32[0];
	_outY = m_vCamPos.m128_f32[1];
	_outZ = m_vCamPos.m128_f32[2];
}

void D3D12Renderer::ApplyCameraRot(const float _yaw, const float _pitch, const float _roll)
{
	m_fCamYaw += _yaw;
	m_fCamPitch += _pitch;
	m_fCamRoll += _roll;

	UpdateCamera();
}
void D3D12Renderer::EnableDXR(bool _bEnable)
{
	m_bDXREnabled = _bEnable;
}
bool D3D12Renderer::IsEnabledDXR()
{
	return m_bDXREnabled;
}
void* D3D12Renderer::CreateBasicMeshObject()
{
	BasicMeshObject* pMeshObj = new BasicMeshObject();
	pMeshObj->Initialize(this);
	return pMeshObj;
}

BOOL D3D12Renderer::BeginCreateMesh(void* _pMeshObjHandle, const BasicVertex* _pVertexList, ULONG _ulVertexCount, ULONG _ulTriGroupCount)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	BOOL bResult = pMeshObj->BeginCreateMesh(_pVertexList, _ulVertexCount, _ulTriGroupCount);
	return bResult;
}

BOOL D3D12Renderer::InsertTriGroup(void* _pMeshObjHandle, const USHORT* _pIndexList, ULONG _ulTriCount, const WCHAR* _wchTexFileName)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	BOOL bResult = pMeshObj->InsertIndexedTriList(_pIndexList, _ulTriCount, _wchTexFileName);
	return bResult;
}

void D3D12Renderer::EndCreateMesh(void* _pMeshObjHandle)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	pMeshObj->EndCreateMesh();
}

void D3D12Renderer::DeleteBasicMeshObject(void* _pMeshObjHandle)
{
	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64FenceValue[i]);
	}
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	delete pMeshObj;
}

void* D3D12Renderer::CreateBLAS(void* _pMeshObjHandle)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	void* pBlasHandle = pMeshObj->CreateBLAS();
	return pBlasHandle;
}

void D3D12Renderer::DeleteBLAS(void* _pMeshObjHandle, void* _pBlasHandle)
{
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	pMeshObj->DeleteBLAS(_pBlasHandle);
}

void D3D12Renderer::UpdateBLASTransform(void* _pBlasHandle, const XMMATRIX* _pMatWorld)
{
	BLAS_INSTANCE* pBlasInstance = reinterpret_cast<BLAS_INSTANCE*>(_pBlasHandle);
	m_pRayTracingManager->UpdateBLASTransform(pBlasInstance, _pMatWorld);
}

void* D3D12Renderer::CreateSpriteObject()
{
	SpriteObject* pSprObj = new SpriteObject();
	pSprObj->Initialize(this);
	return pSprObj;
}

void* D3D12Renderer::CreateSpriteObject(const WCHAR* _wchTexFileName, int _PosX, int _PosY, int _Width, int _Height)
{
	SpriteObject* pSprObj = new SpriteObject();
	RECT rect;
	rect.left = _PosX;
	rect.top = _PosY;
	rect.right = _Width;
	rect.bottom = _Height;
	pSprObj->Initialize(this, _wchTexFileName, &rect);
	return pSprObj;
}

void D3D12Renderer::DeleteSpriteObject(void* _pSpriteObjHandle)
{
	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64FenceValue[i]);
	}
	SpriteObject* pSprObj = reinterpret_cast<SpriteObject*>(_pSpriteObjHandle);
	delete pSprObj;
}

void* D3D12Renderer::CreateTiledTexture(UINT _TexWidth, UINT _TexHeight, ULONG _r, ULONG _g, ULONG _b)
{
	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	BYTE* pImage = reinterpret_cast<BYTE*>(malloc(_TexWidth * _TexHeight * 4));
	memset(pImage, 0, _TexWidth * _TexHeight * 4);

	BOOL bFirstColorIsWhite = TRUE;
	for (UINT y = 0; y < _TexHeight; y++) {
		for (UINT x = 0; x < _TexWidth; x++) {
			RGBA* pDest = reinterpret_cast<RGBA*>(pImage + (x + y * _TexWidth) * 4);
			if ((bFirstColorIsWhite + x) % 2) {
				pDest->r = static_cast<BYTE>(_r);
				pDest->g = static_cast<BYTE>(_g);
				pDest->b = static_cast<BYTE>(_b);
			}
			else {
				pDest->r = 0;
				pDest->g = 0;
				pDest->b = 0;
			}
			pDest->a = 255;
		}
		bFirstColorIsWhite++;
		bFirstColorIsWhite %= 2;
	}

	TEXTURE_HANDLE* pTexHandle = m_pTextureManager->CreateImmutableTexture_ITL(_TexWidth, _TexHeight, TexFormat, pImage);
	free(pImage);
	pImage = nullptr;
	return pTexHandle;
}

void* D3D12Renderer::CreateDynamicTexture(UINT _TexWidth, UINT _TexHeight)
{
	TEXTURE_HANDLE* pTexHandle = m_pTextureManager->CreateDynamicTexture_ITL(_TexWidth, _TexHeight);
	return pTexHandle;
}

void* D3D12Renderer::CreateTextureFromFile(const WCHAR* _wchFileName)
{
	TEXTURE_HANDLE* pTexHandle = m_pTextureManager->CreateTextureFromFile_ITL(_wchFileName);
	return pTexHandle;
}

void* D3D12Renderer::CreateImmutableTexture(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage)
{
	TEXTURE_HANDLE* pTexHandle = m_pTextureManager->CreateImmutableTexture_ITL(_TexWidth, _TexHeight, _format, _pInitImage);
	return pTexHandle;
}

void D3D12Renderer::DeleteTexture(void* _pTexHandle)
{
	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		WaitForFenceValue(m_pui64FenceValue[i]);
	}
	m_pTextureManager->DeleteTexture_ITL(reinterpret_cast<TEXTURE_HANDLE*>(_pTexHandle));
}

void* D3D12Renderer::CreateFontObject(const WCHAR* _wchFontFamilyName, float _fFontSize)
{
	FONT_HANDLE* pFontHandle = m_pFontManager->CreateFontObject_ITL(_wchFontFamilyName, _fFontSize);
	return pFontHandle;
}

void D3D12Renderer::DeleteFontObject(void* _pFontHandle)
{
	m_pFontManager->DeleteFontObject_ITL(reinterpret_cast<FONT_HANDLE*>(_pFontHandle));
}

bool D3D12Renderer::WriteTextToBitmap(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, void* _pFontObjHandle, const WCHAR* _wchString, ULONG _ulLen)
{
	bool bResult = m_pFontManager->WriteTextToBitmap_ITL(_pDestImage, _DestWidth, _DestHeight, _DestPitch, _piOutWidth, _piOutHeight, reinterpret_cast<FONT_HANDLE*>(_pFontObjHandle), _wchString, _ulLen);
	return bResult;
}
void D3D12Renderer::RenderMeshObject(void* _pMeshObjHandle, const XMMATRIX* _pMatWorld)
{
	D3D12GraphicsCommandList_raw pCommandList = m_ppCommandList[m_ulCurContextIndex].Get();
	BasicMeshObject* pMeshObj = reinterpret_cast<BasicMeshObject*>(_pMeshObjHandle);
	pMeshObj->Draw(pCommandList, _pMatWorld);
}
void D3D12Renderer::RenderSpriteWithTex(void* _pSprObjHandle, int _iPosX, int _iPosY, float _fScaleX, float _fScaleY, const RECT* _pRect, float _Z, void* _pTexHandle)
{
	D3D12GraphicsCommandList_raw pCommandList = m_ppCommandList[m_ulCurContextIndex].Get();
	TEXTURE_HANDLE* pTexHandle = reinterpret_cast<TEXTURE_HANDLE*>(_pTexHandle);

	SpriteObject* pSpriteObj = reinterpret_cast<SpriteObject*>(_pSprObjHandle);

	XMFLOAT2 Pos(static_cast<float>(_iPosX), static_cast<float>(_iPosY));
	XMFLOAT2 Scale(_fScaleX, _fScaleY);

	if (pTexHandle->pUploadBuffer) {
		if (pTexHandle->bUpdated) {
			UpdateTexture(m_pD3DDevice, pCommandList, pTexHandle->pTexResource, pTexHandle->pUploadBuffer);
		}
		pTexHandle->bUpdated = false;
	}
	pSpriteObj->DrawWithTex(pCommandList, &Pos, &Scale, _pRect, _Z, pTexHandle);
}
void D3D12Renderer::RenderSprite(void* _pSprObjHandle, int _iPosX, int _iPosY, float _fScaleX, float _fScaleY, float _Z)
{
	D3D12GraphicsCommandList_raw pCommandList = m_ppCommandList[m_ulCurContextIndex].Get();
	SpriteObject* pSpriteObj = reinterpret_cast<SpriteObject*>(_pSprObjHandle);

	XMFLOAT2 Pos = { static_cast<float>(_iPosX), static_cast<float>(_iPosY) };
	XMFLOAT2 Scale = { _fScaleX, _fScaleY };
	pSpriteObj->Draw(pCommandList, &Pos, &Scale, _Z);
}
void D3D12Renderer::UpdateTextureWithImage(void* _pTexHandle, const BYTE* _pSrcBits, UINT _SrcWidth, UINT _SrcHeight)
{
	TEXTURE_HANDLE* pTextureHandle = reinterpret_cast<TEXTURE_HANDLE*>(_pTexHandle);
	D3D12Resource_raw pDestTexResource = pTextureHandle->pTexResource.Get();
	D3D12Resource_raw pUploadBuffer = pTextureHandle->pUploadBuffer.Get();

	D3D12_RESOURCE_DESC Desc = pDestTexResource->GetDesc();
	if (_SrcWidth > Desc.Width)
	{
		__debugbreak();
	}
	if (_SrcHeight > Desc.Height)
	{
		__debugbreak();
	}
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT Footprint;
	UINT	Rows = 0;
	UINT64	RowSize = 0;
	UINT64	TotalBytes = 0;

	m_pD3DDevice->GetCopyableFootprints(&Desc, 0, 1, 0, &Footprint, &Rows, &RowSize, &TotalBytes);

	BYTE* pMappedPtr = nullptr;
	CD3DX12_RANGE readRange(0, 0);

	HRESULT hr = pUploadBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pMappedPtr));
	if (FAILED(hr))
		__debugbreak();

	const BYTE* pSrc = _pSrcBits;
	BYTE* pDest = pMappedPtr;
	for (UINT y = 0; y < _SrcWidth; y++)
	{
		memcpy(pDest, pSrc, _SrcHeight * 4);
		pSrc += (_SrcWidth * 4);
		pDest += Footprint.Footprint.RowPitch;
	}
	// Unmap
	pUploadBuffer->Unmap(0, nullptr);

	pTextureHandle->bUpdated = TRUE;
}
void D3D12Renderer::CreateCommandList()
{
	for (ULONG i = 0; i < MAX_PENDING_FRAME_COUNT; i++) {
		D3D12CommandAllocator_ptr pCommandAllocator = nullptr;
		D3D12GraphicsCommandList_ptr pCommandList = nullptr;

		HRESULT hr = m_pD3DDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(pCommandAllocator.GetAddressOf()));
		if (FAILED(hr)) {
			OutputDebugString(L"Failed to create command allocator\n");
			__debugbreak();
		}

		hr = m_pD3DDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), nullptr, IID_PPV_ARGS(pCommandList.GetAddressOf()));
		if (FAILED(hr)) {
			OutputDebugString(L"Failed to create command list\n");
			__debugbreak();
		}
		pCommandList->Close();

		m_ppCommandAllocator[i] = std::move(pCommandAllocator);
		m_ppCommandList[i] = std::move(pCommandList);
	}
}

bool D3D12Renderer::CreateDescriptorHeapForRTV()
{
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SWAP_CHAIN_FRAME_COUNT;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if(FAILED(m_pD3DDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(m_pRTVHeap.GetAddressOf())))) {
		__debugbreak();
		return false;
	}

	m_rtvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	return true;
}

bool D3D12Renderer::CreateDescriptorHeapForDSV()
{
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	if(FAILED(m_pD3DDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(m_pDSVHeap.GetAddressOf())))) {
		__debugbreak();
		return false;
	}

	m_dsvDescriptorSize = m_pD3DDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	return true;
}

bool D3D12Renderer::CreateDepthStencilBuffer(UINT _Width, UINT _Height)
{
	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
	depthStencilDesc.Format = DXGI_FORMAT_D32_FLOAT;
	depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
	depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
	depthOptimizedClearValue.DepthStencil.Depth = 1.f;
	depthOptimizedClearValue.DepthStencil.Stencil = 0;

	CD3DX12_RESOURCE_DESC depthDesc(
		D3D12_RESOURCE_DIMENSION_TEXTURE2D,
		0,
		_Width,
		_Height,
		1,
		1,
		DXGI_FORMAT_R32_TYPELESS,
		1,
		0,
		D3D12_TEXTURE_LAYOUT_UNKNOWN,
		D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
	);

	auto depthHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	if (FAILED(m_pD3DDevice->CreateCommittedResource(
		&depthHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&depthDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthOptimizedClearValue,
		IID_PPV_ARGS(m_pDepthStencilBuffer.GetAddressOf())))) 
	{
		__debugbreak();
		return false;
	}
	m_pDepthStencilBuffer->SetName(L"D3D12Renderer::m_pDepthStencilBuffer");

	CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_pDSVHeap->GetCPUDescriptorHandleForHeapStart());
	m_pD3DDevice->CreateDepthStencilView(m_pDepthStencilBuffer.Get(), &depthStencilDesc, dsvHandle);

	return true;
}

void D3D12Renderer::CleanupDepthStencilBuffer()
{
	if(m_pDepthStencilBuffer) {
		m_pDepthStencilBuffer.Reset();
	}
}

void D3D12Renderer::CreateFence()
{
	if(FAILED(m_pD3DDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_pFence.GetAddressOf())))) {
		__debugbreak();
	}

	m_ui64FenceValue = 0;
	m_hFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

UINT64 D3D12Renderer::DoFence()
{
	m_ui64FenceValue++;
	m_pCommandQueue->Signal(m_pFence.Get(), m_ui64FenceValue);
	return m_ui64FenceValue;
}

void D3D12Renderer::WaitForFenceValue(UINT64 _ExpectedFenceValue)
{
	const UINT64 ExpectedFenceValue = m_ui64FenceValue;

	// Wait until the previous frame is finished.
	if (m_pFence->GetCompletedValue() < ExpectedFenceValue)
	{
		m_pFence->SetEventOnCompletion(ExpectedFenceValue, m_hFenceEvent);
		WaitForSingleObject(m_hFenceEvent, INFINITE);
	}
}

void D3D12Renderer::CleanUpFence()
{
	if(m_hFenceEvent) {
		CloseHandle(m_hFenceEvent);
		m_hFenceEvent = nullptr;
	}
}

void D3D12Renderer::CleanupRenderer()
{
	DoFence();

	// ① RayTracingManager 먼저 (내부에서 CommandQueue 등 사용)
	m_pRayTracingManager = nullptr;
	m_pResourceManager = nullptr;
	m_pShaderManager = nullptr;

	// ② Depth Stencil
	m_pDepthStencilBuffer = nullptr;

	// ③ RenderTarget, DescriptorHeap
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
		m_pRenderTargets[i] = nullptr;
	m_pRTVHeap = nullptr;
	m_pDSVHeap = nullptr;

	// ⑤ SwapChain을 CommandQueue보다 먼저! (SwapChain이 CommandQueue에 AddRef함)
	m_pSwapChain = nullptr;

	// ⑥ CommandQueue
	m_pCommandQueue = nullptr;

	// ⑦ Device를 QueryInterface 후 Release, 그 다음 Report
	{
		Microsoft::WRL::ComPtr<ID3D12DebugDevice> pDebugDevice;
		m_pD3DDevice->QueryInterface(IID_PPV_ARGS(pDebugDevice.GetAddressOf()));
		m_pD3DDevice.Reset(); // ← Device ComPtr 먼저 해제

		if (pDebugDevice)
		{
			// 이 시점에 pDebugDevice만 Device를 물고 있어야 정상
			pDebugDevice->ReportLiveDeviceObjects(
				static_cast<D3D12_RLDO_FLAGS>(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL)
			);
		}
	}
}

void D3D12Renderer::InitCamera()
{
	m_fCamPitch = 0.f;
	m_fCamRoll = 0.f;
	m_fCamYaw = 0.f;
}

void D3D12Renderer::UpdateCamera()
{
	XMVECTOR xAxis = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	XMVECTOR yAxis = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR zAxis = XMVectorSet(0.f, 0.f, 1.f, 0.f);

	XMMATRIX matRotPitch = XMMatrixRotationX(m_fCamPitch);
	XMMATRIX matRotYaw = XMMatrixRotationY(m_fCamYaw);

	XMMATRIX matCamRot = XMMatrixMultiply(matRotYaw, matRotPitch);

	m_vCamDir = XMVector3Transform(zAxis, matCamRot);
	m_vCamRight = XMVector3Cross(yAxis, m_vCamDir);
	m_vCamUp = XMVector3Cross(m_vCamDir, m_vCamRight);

	// view matrix
	m_matView = XMMatrixLookAtLH(m_vCamPos, m_vCamPos + m_vCamDir, m_vCamUp);

	// Fov (Radian)
	float fovY = XM_PIDIV4; // 90도

	// proj matrix
	float fAspectRatio = static_cast<float>(m_ulWidth) / static_cast<float>(m_ulHeight);
	float fNearZ = 0.1f;
	float fFarZ = 1000.f;
	m_matProj = XMMatrixPerspectiveFovLH(fovY, fAspectRatio, fNearZ, fFarZ);

	XMVECTOR determinant;
	m_matViewInv = XMMatrixInverse(&determinant, m_matView);

}

SimpleConstantBufferPool* D3D12Renderer::INL_GetConstantBufferPool(CONSTANT_BUFFER_TYPE _cbType)
{
	return m_ppConstantBufferManager[m_ulCurContextIndex]->GetConstantBufferPool(_cbType);
}

void D3D12Renderer::FillProjDecompConstant(DECOMP_PROJ* _pOutConstBuffer)
{
	_pOutConstBuffer->rcp_m11 = 1.f / XMMatrixExtract(&m_matProj, 1, 1);
	_pOutConstBuffer->rcp_m22 = 1.f / XMMatrixExtract(&m_matProj, 2, 2);

	_pOutConstBuffer->m21 = XMMatrixExtract(&m_matProj, 2, 1);
	_pOutConstBuffer->m31 = XMMatrixExtract(&m_matProj, 3, 1);
	_pOutConstBuffer->m32 = XMMatrixExtract(&m_matProj, 3, 2);
	_pOutConstBuffer->m33 = XMMatrixExtract(&m_matProj, 3, 3);
	_pOutConstBuffer->m43 = XMMatrixExtract(&m_matProj, 4, 3);
}

void D3D12Renderer::FillRayTraceConstant(CONSTANT_BUFFER_RAY_TRACING* _pOutBuffer)
{
	FillProjDecompConstant(&_pOutBuffer->DecompProj);
	_pOutBuffer->matViewProj = XMMatrixMultiplyTranspose(m_matView, m_matProj);
	_pOutBuffer->matViewInv = XMMatrixTranspose(m_matViewInv);

	_pOutBuffer->vCameraPos = { m_vCamPos.m128_f32[0], m_vCamPos.m128_f32[1], m_vCamPos.m128_f32[2], 1.f };
	_pOutBuffer->Near = 0.1f;
	_pOutBuffer->Far = 1000.f;
}

void D3D12Renderer::INL_GetViewProjMatrix(XMMATRIX& _outView, XMMATRIX& _outProj)
{
	_outView = XMMatrixTranspose(m_matView);
	_outProj = XMMatrixTranspose(m_matProj);
}

D3D12Renderer::D3D12Renderer()
{
}

D3D12Renderer::~D3D12Renderer()
{
	CleanupRenderer();
}
