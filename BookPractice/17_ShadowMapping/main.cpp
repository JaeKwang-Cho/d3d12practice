//***************************************************************************************
// ShadowMappingApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>
#include "FileReader.h"
#include "ShadowMap.h"
#include "../Common/Camera.h"

#define PRAC1 (1)

const int g_NumFrameResources = 3;

// vertex, index, CB, PrimitiveType, DrawIndexedInstanced 등
// 요걸 묶어서 렌더링하기 좀 더 편하게 해주는 구조체이다.
struct RenderItem 
{
	RenderItem() = default;

	// item 마다 World를 가지고 있게 한다.
	XMFLOAT4X4 WorldMat = MathHelper::Identity4x4();

	// Texture Tile이나, Animation을 위한 Transform도 같이 넣어준다.
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();

	// 물체의 상태가 변해서 CB를 업데이트 해야 할 때
	// Dirty flag를 켜서 새로 업데이트를 한다. (디자인 패턴 관련)
	// 어쨌든 PassCB는 Frame 마다 갱신을 하므로, Frame 마다 업데이트를 해줘야한다.
	// 여기서는 g_NumFrameResources 값으로 세팅을 해줘서, 업데이트를 한다
	int NumFrameDirty = g_NumFrameResources;
	
	// Render Item과 GPU에 넘어간 CB가 함께 가지는 Index 값
	UINT ObjCBIndex = -1;

	MeshGeometry* Geo = nullptr;
	Material* Mat = nullptr;

	// 다른걸 쓸일은 잘 없을 듯
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	bool Visible = true;
	BoundingBox Bounds;
	// DrawIndexedInstanced 인자이다.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

// 스카이 맵을 그리는 lay를 따로 둔다. 
enum class RenderLayer : int
{
	Opaque = 0,
	Debug,
	Sky, 
	Count
};

class ShadowMappingApp : public D3DApp
{
public:
	ShadowMappingApp(HINSTANCE hInstance);
	~ShadowMappingApp();

	ShadowMappingApp(const ShadowMappingApp& _other) = delete;
	ShadowMappingApp& operator=(const ShadowMappingApp& _other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& _gt) override;
	virtual void Draw(const GameTimer& _gt) override;

	virtual void OnMouseDown(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseUp(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove(WPARAM _btnState, int _x, int _y) override;

	virtual void CreateRtvAndDsvDescriptorHeaps() override;
	
	// ==== Update ====
	void OnKeyboardInput(const GameTimer _gt);
	void AnimateMaterials(const GameTimer& _gt);
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMaterialBuffer(const GameTimer& _gt);
	void UpdateShadowTransform(const GameTimer& _gt); // 광원에서 텍스쳐로 넘어가는 행렬을 업데이트해준다.
	void UpdateMainPassCB(const GameTimer& _gt);
	void UpdateShadowPassCB(const GameTimer& _gt); // 디버깅용 화면을 그릴때 사용하는 PassCB를 업데이트 해준다.

	// ==== Init ====
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildStageGeometry();
	void BuildSkullGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildPSOs();
	void BuildFrameResources();

	// ==== Render ====
	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY  _Type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
	void DrawSceneToShadowMap(); // 디버깅용 그림자 화면을 그려주는 함수이다.

	// 지형의 높이와 노멀을 계산하는 함수다.
	float GetHillsHeight(float _x, float _z) const;
	XMFLOAT3 GetHillsNormal(float _x, float _z)const;

	// 정적 샘플러 구조체를 미리 만드는 함수이다.
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 11> GetStaticSamplers();

private:
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	// Compute Shader용 PSO가 따로 필요하다.
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	// Uav도 함께 쓰는 heap이므로 이름을 바꿔준다.
	ComPtr<ID3D12DescriptorHeap> m_CbvSrvUavDescriptorHeap = nullptr;

	// 테스트용 Sampler Heap이다.
	ComPtr<ID3D12DescriptorHeap> m_SamplerHeap = nullptr;

	// 도형, 쉐이더, PSO 등등 App에서 관리한다..
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

	// input layout도 백터로 가지고 있는다
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	//  App 단에서 RenderItem 포인터를 가지고 있는다
	RenderItem* m_WavesRenderItem = nullptr;

	// RenderItem 리스트
	std::vector<std::unique_ptr<RenderItem>> m_AllRenderItems;

	// 이건 나중에 공부한다.
	std::vector<RenderItem*> m_RenderItemLayer[(int)RenderLayer::Count];

	// Object 관계 없이, Render Pass 전체가 공유하는 값이다.
	PassConstants m_MainPassCB;
	// 디버깅용 화면에 쓰이는 PassCB이다.
	PassConstants m_ShadowPassCB;

	POINT m_LastMousePos = {};

	// Camera
	Camera m_Camera;
	Camera m_CubeMapCamera[6];
	
	// 쉐도우 맵을 관리해주는 클래스 인스턴스다.
	std::unique_ptr<ShadowMap> m_ShadowMap;
	// 필요한 부분에만 그림자를 그리도록 하는 BoundingSphere다.
	BoundingSphere m_SceneBounds;

	// Sky Texture가 위치한 view Handle Offset을 저장해 놓는다.
	UINT m_SkyTexHeapIndex = 0;
	// Shadow Texture가 위치한 view handle offset을 저장해 놓는다.
	UINT m_ShadowMapHeapIndex = 0;
	// 디버깅용 큐브와 텍스쳐가 위치한 view handle offset을 저장해 놓는다.
	UINT m_NullCubeSrvIndex = 0;
	UINT m_NullTexSrvIndex = 0;
	// 렌더타겟없이 뎁스 스텐실 뷰만 있는 Srv이다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NullSrv;


	// 그림자 투영을 하기 위해 필요한 속성들이다.
	// viewport 속성
	float m_LightNearZ = 0.0f;
	float m_LightFarZ = 0.0f;
	// 광원 위치
	XMFLOAT3 m_LightPosW;
	// 그걸로 만든 행렬들
	XMFLOAT4X4 m_LightViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_LightProjMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ShadowMat = MathHelper::Identity4x4();

	// 업데이트 하기 위해서 맴버로 가지고 있는다.
	float m_LightRotationAngle = 0.0f;
	XMFLOAT3 m_BaseLightDirections[3] = {
		XMFLOAT3(0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(-0.57735f, -0.57735f, 0.57735f),
		XMFLOAT3(0.0f, -0.707f, -0.707f)
	};
	XMFLOAT3 m_RotatedLightDirections[3];
};

int WINAPI WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE prevInstance,
				   _In_ PSTR cmdLine, _In_ int showCmd)
{
	// Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		ShadowMappingApp theApp(hInstance);
		if (!theApp.Initialize())
			return 0;
		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

ShadowMappingApp::ShadowMappingApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	// 어떤 레벨이 들어갈지 우리는 이미 알고 있으니, 
	// 생성자에서 BoundingSphere를 초기화 한다.
	m_SceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_SceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
}

ShadowMappingApp::~ShadowMappingApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

void ShadowMappingApp::CreateRtvAndDsvDescriptorHeaps()
{
	// __super::CreateRtvAndDsvDescriptorHeaps();

	// 렌더타겟 힙
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};

	rtvHeapDesc.NumDescriptors = SwapChainBufferCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(m_RtvHeap.GetAddressOf())
	));

	// 뎁스 스텐실 힙
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	// 쉐도우 맵을 만드는 깊이 버퍼를 하나 더 만든다.
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())));
}

bool ShadowMappingApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_Camera.SetPosition(0.0f, 2.0f, -15.0f);
	// 사용할 shadow map의 해상도는 2048 * 2048으로 한다.
	m_ShadowMap = std::make_unique<ShadowMap>(
		m_d3dDevice.Get(), 2048, 2048);

	// ======== 초기화 ==========
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildStageGeometry();
	BuildSkullGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	// =========================


	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void ShadowMappingApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetFrustum(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
}

void ShadowMappingApp::Update(const GameTimer& _gt)
{
	// 더 기능이 많아질테니, 함수로 쪼개서 넣는다.
	OnKeyboardInput(_gt);

	// 원형 배열을 돌면서
	m_CurrFrameResourceIndex = (m_CurrFrameResourceIndex + 1) % g_NumFrameResources;
	m_CurrFrameResource = m_FrameResources[m_CurrFrameResourceIndex].get();

	// GPU가 너무 느려서, 한바꾸 돌았을 경우를 대비한다.
	if (m_CurrFrameResource->Fence != 0 && m_Fence->GetCompletedValue() < m_CurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_Fence->SetEventOnCompletion(m_CurrFrameResource->Fence, eventHandle));
		if (eventHandle != NULL)
		{
			DWORD result = WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}
		else
		{
			assert("eventHandle is NULL" && false);
			// need to terminate App.
		}
	}

	// 광원이 빙글빙글 돌게 만든다.
	m_LightRotationAngle += 0.1f * _gt.GetDeltaTime();

	XMMATRIX R = XMMatrixRotationY(m_LightRotationAngle);
	for (int i = 0; i < 3; i++)
	{
		XMVECTOR lightDir = XMLoadFloat3(&m_BaseLightDirections[i]);
		lightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&m_RotatedLightDirections[i], lightDir);
	}

	AnimateMaterials(_gt);
	UpdateObjectCBs(_gt);
	UpdateMaterialBuffer(_gt);
	// PassCB에 Shadow Transform이 담기니까, 순서를 잘 맞춰줘야 한다.
	UpdateShadowTransform(_gt);
	UpdateMainPassCB(_gt);
	UpdateShadowPassCB(_gt);
}

void ShadowMappingApp::Draw(const GameTimer& _gt)
{
	// 현재 FrameResource가 가지고 있는 allocator를 가지고 와서 초기화 한다.
	ComPtr<ID3D12CommandAllocator> CurrCommandAllocator = m_CurrFrameResource->CmdListAlloc;
	ThrowIfFailed(CurrCommandAllocator->Reset());

	// Command List도 초기화를 한다
	// 근데 Command List에 Reset()을 걸려면, 한번은 꼭 Command Queue에 
	// ExecuteCommandList()로 등록이 된적이 있어야 가능하다.

	// PSO별로 등록하면서, Allocator와 함께, Command List를 초기화 한다.
	ThrowIfFailed(m_CommandList->Reset(CurrCommandAllocator.Get(), m_PSOs["opaque"].Get()));

	// Descriptor heap과 Root Signature를 세팅해준다.
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvSrvUavDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// PSO에 Root Signature를 세팅해준다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Structured Buffer는 Heap 없이 그냥 Rood Descriptor로 넘겨줄 수 있다.
	// Material 정보는 공통으로 사용하기에 맨 먼저 바인드 해준다.
	// 현재 프레임에 사용하는 Material Buffer를 Srv로 바인드 해준다.
	ID3D12Resource* matStructredBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
	m_CommandList->SetGraphicsRootShaderResourceView(2, matStructredBuffer->GetGPUVirtualAddress()); // Mat는 2번

	// 텍스쳐도 인덱스를 이용해서 공통으로 사용하니깐, 이렇게 넘겨준다.
	m_CommandList->SetGraphicsRootDescriptorTable(4, m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// 배경을 그릴 skyTexture 핸들은 미리 만들어 놓는다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(m_SkyTexHeapIndex, m_CbvSrvUavDescriptorSize);

	// ==== 광원 입장에서 그림자 뎁스 그리기 ====
	
	// 3번에 Render Target이 nullptr인 Srv를 세팅한다.
	m_CommandList->SetGraphicsRootDescriptorTable(3, m_NullSrv);
	// 클래스 인스턴스 맴버인 텍스쳐에 그린다.
	DrawSceneToShadowMap();

	// ==== 화면 그리기 ====

	// 커맨드 리스트에서, Viewport와 ScissorRects 를 설정한다.
	m_CommandList->RSSetViewports(1, &m_ScreenViewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	// 그림을 그릴 resource인 back buffer의 Resource Barrier의 Usage를 D3D12_RESOURCE_STATE_RENDER_TARGET으로 바꾼다.
	D3D12_RESOURCE_BARRIER bufferBarrier_PRESENT_RT = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);
	m_CommandList->ResourceBarrier(1, &bufferBarrier_PRESENT_RT);

	// 백 버퍼와 깊이 버퍼를 초기화 한다.
	m_CommandList->ClearRenderTargetView(
		GetCurrentBackBufferView(),
		Colors::LightSteelBlue,  
		0, nullptr);	
	// 뎁스를 1.f 스텐실을 0으로 초기화 한다.
	m_CommandList->ClearDepthStencilView(
		GetDepthStencilView(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.0f, 0, 0, nullptr);

	// Rendering 할 백 버퍼와, 깊이 버퍼의 핸들을 OM 단계에 세팅을 해준다.
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = GetDepthStencilView();
	m_CommandList->OMSetRenderTargets(1, &BackBufferHandle, true, &DepthStencilHandle);

	// 현재 Frame Constant Buffer 0 offset을 업데이트한다.
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress()); // Pass는 1번

	// SkyMap을 그릴 TextureCube를 3번에 바인드해준다.
	m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	// drawcall을 걸어준다.
	// 일단 불투명한 애들을 먼저 싹 그려준다.
	m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// shadow debug layer를 그려준다.
	m_CommandList->SetPipelineState(m_PSOs["debug"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Debug]);

	// 하늘을 맨 마지막에 그려주는 것이 depth test 성능상 좋다고 한다. 
	m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Sky]);

	// ======================================

	D3D12_RESOURCE_BARRIER bufferBarrier_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_CommandList->ResourceBarrier(1,&bufferBarrier_RT_PRESENT);

	// Command List의 기록을 마치고
	ThrowIfFailed(m_CommandList->Close());

	// Command List를 Command Queue에 올려서, 작업을 기다린다.
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// 백 버퍼를 화면에 그려달라고 요청을 하고
	ThrowIfFailed(m_SwapChain->Present(0, 0));
	m_CurrBackBuffer = (m_CurrBackBuffer + 1) % SwapChainBufferCount;

	// 현재 fence point 까지 commands 들을 확인하도록 fence 값을 늘린다.
	m_CurrFrameResource->Fence = ++m_CurrentFence;

	// 새 GPU의 새 fence point를 세팅하는 명령을 큐에 넣는다.
	m_CommandQueue->Signal(m_Fence.Get(), m_CurrentFence);
}

void ShadowMappingApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	if ((_btnState & MK_LBUTTON) != 0)
	{
		// 마우스의 위치를 기억하고
		m_LastMousePos.x = _x;
		m_LastMousePos.y = _y;
		// 마우스를 붙잡는다.
		SetCapture(m_hMainWnd);
	}
}

void ShadowMappingApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void ShadowMappingApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
{
	// 왼쪽 마우스가 눌린 상태에서 움직인다면
	if ((_btnState & MK_LBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(_x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(_y - m_LastMousePos.y));

		m_Camera.AddPitch(dy);
		m_Camera.AddYaw(dx);
	}
	// 가운데 마우스가 눌린 상태에서 움직인다면
	/*
	if ((_btnState & MK_MBUTTON) != 0)
	{
		float dx = XMConvertToRadians(0.25f * static_cast<float>(_x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(_y - m_LastMousePos.y));

		m_Camera.AddRoll(dx);
	}
	*/
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void ShadowMappingApp::OnKeyboardInput(const GameTimer _gt)
{
	const float dt = _gt.GetDeltaTime();

	if (GetAsyncKeyState('W') & 0x8000)
		m_Camera.Walk(10.0f * dt);

	if (GetAsyncKeyState('S') & 0x8000)
		m_Camera.Walk(-10.0f * dt);

	if (GetAsyncKeyState('A') & 0x8000)
		m_Camera.Strafe(-10.0f * dt);

	if (GetAsyncKeyState('D') & 0x8000)
		m_Camera.Strafe(10.0f * dt);

	if (GetAsyncKeyState('Q') & 0x8000)
		m_Camera.Ascend(10.0f * dt);

	if (GetAsyncKeyState('E') & 0x8000)
		m_Camera.Ascend(-10.0f * dt);

	m_Camera.UpdateViewMatrix();
}

void ShadowMappingApp::AnimateMaterials(const GameTimer& _gt)
{
}

void ShadowMappingApp::UpdateObjectCBs(const GameTimer& _gt)
{
	UploadBuffer<ObjectConstants>* currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for (std::unique_ptr<RenderItem>& e : m_AllRenderItems)
	{
		// Constant Buffer가 변경됐을 때 Update를 한다.
		if (e->NumFrameDirty > 0)
		{
			// CPU에 있는 값을 가져와서
			XMMATRIX worldMat = XMLoadFloat4x4(&e->WorldMat);
			XMMATRIX texTransform = XMLoadFloat4x4(&e->TexTransform);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.WorldMat, XMMatrixTranspose(worldMat));
			XMVECTOR DetworldMat = XMMatrixDeterminant(worldMat);
			XMMATRIX InvworldMat = XMMatrixInverse(&DetworldMat, worldMat);
			XMStoreFloat4x4(&objConstants.InvWorldMat, InvworldMat);

			// Texture Tile이나, Animation을 위한 Transform도 같이 넣어준다.
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			// 머테리얼 인덱스를 넣어준다.
			objConstants.MaterialIndex = e->Mat->MatCBIndex;

			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void ShadowMappingApp::UpdateMaterialBuffer(const GameTimer& _gt)
{
	UploadBuffer<MaterialData>* currMaterialCB = m_CurrFrameResource->MaterialBuffer.get();	
	for (std::pair<const std::string, std::unique_ptr<Material>>& e : m_Materials)
	{
		// 이것도 dirty flag가 켜져있을 때만 업데이트를 한다.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialData matData;
			matData.DiffuseAlbedo = mat->DiffuseAlbedo;
			matData.FresnelR0 = mat->FresnelR0;
			matData.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matData.MaterialTransform, XMMatrixTranspose(matTransform));
			// Texture 인덱스를 넣어준다.
			matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
			matData.NormalMapIndex = mat->NormalSrvHeapIndex;
			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			mat->NumFramesDirty--;
		}
	}
}

void ShadowMappingApp::UpdateShadowTransform(const GameTimer& _gt)
{
	// 일단은 첫번째 디렉셔널 라이트에만 그림자를 만든다.
	XMVECTOR lightDir = XMLoadFloat3(&m_RotatedLightDirections[0]);
	// ViewMat을 생성하기 위한 속성을 정의하고
	XMVECTOR lightPos = -2.f * m_SceneBounds.Radius * lightDir;
	XMVECTOR targetPos = XMLoadFloat3(&m_SceneBounds.Center);
	XMVECTOR lightUp = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	// 광원에서 바라보는 ViewMat을 생성한다.
	XMMATRIX lightViewMat = XMMatrixLookAtLH(lightPos, targetPos, lightUp);
	
	// 맴버를 업데이트한다.
	XMStoreFloat3(&m_LightPosW, lightPos);

	// 월드의 중심을 광원 view 좌표계로 이동한다.
	XMFLOAT3 sphereCenterLightSpace;
	XMStoreFloat3(&sphereCenterLightSpace, XMVector3TransformCoord(targetPos, lightViewMat));

	// 직교 투영을 위한 viewport 속성을 설정한다.
	float left = sphereCenterLightSpace.x - m_SceneBounds.Radius;
	float right = sphereCenterLightSpace.x + m_SceneBounds.Radius;
	float bottom = sphereCenterLightSpace.y - m_SceneBounds.Radius;
	float top = sphereCenterLightSpace.y + m_SceneBounds.Radius;
	float nearZ = sphereCenterLightSpace.z - m_SceneBounds.Radius;
	float farZ = sphereCenterLightSpace.z + m_SceneBounds.Radius;

	// 멤버를 업데이트 한다.
	m_LightNearZ = nearZ;
	m_LightFarZ = farZ;

	// orthographic projection Mat을 생성한다.
	XMMATRIX lightProjMat = XMMatrixOrthographicOffCenterLH(left, right, bottom, top, nearZ, farZ);

	// NDC 공간을 [-1, 1] 텍스쳐 공간으로 [0, 1] 바꿔준다.
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f
	);
	// 그림자 변환 행렬을 맴버로 업데이트 한다.
	XMMATRIX ShadowMat = lightViewMat * lightProjMat * T;
	XMStoreFloat4x4(&m_LightViewMat, lightViewMat);
	XMStoreFloat4x4(&m_LightProjMat, lightProjMat);
	XMStoreFloat4x4(&m_ShadowMat, ShadowMat);
}

void ShadowMappingApp::UpdateMainPassCB(const GameTimer& _gt)
{
	XMMATRIX ViewMat = m_Camera.GetViewMat();
	XMMATRIX ProjMat = m_Camera.GetProjMat();

	XMMATRIX VPMat = XMMatrixMultiply(ViewMat, ProjMat);

	XMVECTOR DetViewMat = XMMatrixDeterminant(ViewMat);
	XMMATRIX InvViewMat = XMMatrixInverse(&DetViewMat, ViewMat);

	XMVECTOR DetProjMat = XMMatrixDeterminant(ProjMat);
	XMMATRIX InvProjMat = XMMatrixInverse(&DetProjMat, ProjMat);

	XMVECTOR DetVPMatMat = XMMatrixDeterminant(VPMat);
	XMMATRIX InvVPMat = XMMatrixInverse(&DetVPMatMat, VPMat);

	// 그림자 변환 행렬도 passCB로 넘겨준다.
	XMMATRIX shadowMat = XMLoadFloat4x4(&m_ShadowMat);

	XMStoreFloat4x4(&m_MainPassCB.ViewMat, XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&m_MainPassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
	XMStoreFloat4x4(&m_MainPassCB.ProjMat, XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&m_MainPassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
	XMStoreFloat4x4(&m_MainPassCB.VPMat, XMMatrixTranspose(VPMat));
	XMStoreFloat4x4(&m_MainPassCB.InvVPMat, XMMatrixTranspose(InvVPMat));
	XMStoreFloat4x4(&m_MainPassCB.ShadowMat, XMMatrixTranspose(shadowMat));

	m_MainPassCB.EyePosW = m_Camera.GetPosition3f();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.f / m_ClientWidth, 1.f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.f;
	m_MainPassCB.FarZ = 1000.f;
	m_MainPassCB.TotalTime = _gt.GetTotalTime();
	m_MainPassCB.DeltaTime = _gt.GetDeltaTime();

	// 사실 이미 값이 있긴 한데... 그냥 한번 더 써준거다.
	m_MainPassCB.FogColor = { 0.7f, 0.7f, 0.7f, 1.f };
	m_MainPassCB.FogStart = 5.f;
	m_MainPassCB.FogRange = 150.f;

	// 이제 Light를 채워준다. 멤버의 값을 채워준다.
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_MainPassCB.Lights[0].Direction = m_RotatedLightDirections[0];
	m_MainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
	m_MainPassCB.Lights[1].Direction = m_RotatedLightDirections[1];
	m_MainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.4f };
	m_MainPassCB.Lights[2].Direction = m_RotatedLightDirections[2];
	m_MainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.2f };
	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void ShadowMappingApp::UpdateShadowPassCB(const GameTimer& _gt)
{
	// 광원 입장에서 보이는 영역을 정의한다.
	XMMATRIX ViewMat = XMLoadFloat4x4(&m_LightViewMat);
	XMMATRIX ProjMat = XMLoadFloat4x4(&m_LightProjMat);

	XMMATRIX VPMat = XMMatrixMultiply(ViewMat, ProjMat);

	XMVECTOR DetViewMat = XMMatrixDeterminant(ViewMat);
	XMMATRIX InvViewMat = XMMatrixInverse(&DetViewMat, ViewMat);

	XMVECTOR DetProjMat = XMMatrixDeterminant(ProjMat);
	XMMATRIX InvProjMat = XMMatrixInverse(&DetProjMat, ProjMat);

	XMVECTOR DetVPMatMat = XMMatrixDeterminant(VPMat);
	XMMATRIX InvVPMat = XMMatrixInverse(&DetVPMatMat, VPMat);

	XMStoreFloat4x4(&m_MainPassCB.ViewMat, XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&m_MainPassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
	XMStoreFloat4x4(&m_MainPassCB.ProjMat, XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&m_MainPassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
	XMStoreFloat4x4(&m_MainPassCB.VPMat, XMMatrixTranspose(VPMat));
	XMStoreFloat4x4(&m_MainPassCB.InvVPMat, XMMatrixTranspose(InvVPMat));
	// 광원입장에서 그리기로한 정보를 넘겨줘야 한다.
	UINT w = m_ShadowMap->GetWidth();
	UINT h = m_ShadowMap->GetHeight();

	m_MainPassCB.EyePosW = m_LightPosW;
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.f / w, 1.f / h);
	m_MainPassCB.NearZ = m_LightNearZ;
	m_MainPassCB.FarZ = m_LightFarZ;

	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	// 두번째 offset에 넣어준다.
	currPassCB->CopyData(1, m_MainPassCB);
}

void ShadowMappingApp::LoadTextures()
{
	// 진작에 이렇게 할걸
	std::vector<std::string> texNames =
	{
		"bricksDiffuseMap",
		"bricksNormalMap",
		"tileDiffuseMap",
		"tileNormalMap",
		"defaultDiffuseMap",
		"defaultNormalMap",
		"skyCubeMap"
	};

	std::vector<std::wstring> texFilenames =
	{
		L"../Textures/bricks2.dds",
		L"../Textures/bricks2_nmap.dds",
		L"../Textures/tile.dds",
		L"../Textures/tile_nmap.dds",
		L"../Textures/white1x1.dds",
		L"../Textures/default_nmap.dds",
		L"../Textures/desertcube1024.dds"
	};

	for (int i = 0; i < (int)texNames.size(); ++i)
	{
		std::unique_ptr<Texture>  texMap = std::make_unique<Texture>();
		texMap->Name = texNames[i];
		texMap->Filename = texFilenames[i];
		ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_d3dDevice.Get(),
					  m_CommandList.Get(), texMap->Filename.c_str(),
					  texMap->Resource, texMap->UploadHeap));

		m_Textures[texMap->Name] = std::move(texMap);
	}
}


void ShadowMappingApp::BuildRootSignature()
{	
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	UINT numOfParameter = 5;

	// TextureCube와 ShadowMap이 Srv로 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	// Texture2D를 Index로 접근할 것 이기 때문에 이렇게 10개로 만들어 준다. 
	// 이제 노멀맵도 넘어간다.
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 10, 2, 0); // space0 / t2 - texture 정보

	// Srv가 넘어가는 테이블, Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[1].InitAsConstantBufferView(1); // b1 - PassConstants
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // t0 - Material Structred Buffer, Space도 1로 지정해준다.
	// D3D12_SHADER_VISIBILITY_ALL로 해주면 모든 쉐이더에서 볼 수 있다.
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t0 ~ t1 - TextureCube
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t2 - Texture2D Array

	// DirectX에서 제공하는 Heap을 만들지 않고, Sampler를 생성할 수 있게 해주는
	// Static Sampler 방법이다. 최대 2000개 생성 가능
	// (원래는 D3D12_DESCRIPTOR_HEAP_DESC 가지고 D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER막 해줘야 한다.)
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 11> staticSamplers = GetStaticSamplers();

	// IA에서 값이 들어가도록 설정한다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		numOfParameter,
		slotRootParameter,
		(UINT)staticSamplers.size(), // 드디어 여길 채워넣게 되었다.
		staticSamplers.data(), // 보아하니 구조체만 넣어주면 되는 듯 보인다.
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// Root Parameter 를 만든다.
	ComPtr<ID3DBlob> serializedRootSig = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(m_d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())));
}

void ShadowMappingApp::BuildDescriptorHeaps()
{
	// Descriptor Count는 좀 넘겨도 된다. 부족한게 문제가 생기는 것이다.
	const int textureDescriptorCount = 14;

	int viewCount = 0;

	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = textureDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CbvSrvUavDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	std::vector<ID3D12Resource*> tex2DList =
	{
		m_Textures["bricksDiffuseMap"]->Resource.Get(),
		m_Textures["bricksNormalMap"]->Resource.Get(),
		m_Textures["tileDiffuseMap"]->Resource.Get(),
		m_Textures["tileNormalMap"]->Resource.Get(),
		m_Textures["defaultDiffuseMap"]->Resource.Get(),
		m_Textures["defaultNormalMap"]->Resource.Get(),
	};

	ID3D12Resource* skyCubeMap = m_Textures["skyCubeMap"]->Resource.Get();

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

	for (UINT i = 0; i < (UINT)tex2DList.size(); i++)
	{
		// view를 만들면서 연결해준다.
		srvDesc.Format = tex2DList[i]->GetDesc().Format;
		srvDesc.Texture2D.MipLevels = tex2DList[i]->GetDesc().MipLevels;
		m_d3dDevice->CreateShaderResourceView(tex2DList[i], &srvDesc, viewHandle);

		viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
		viewCount++;
	}

	// TextureCube는 프로퍼티가 다르기때문에
	// 수정해주고 Resource View를 생성한다.
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = skyCubeMap->GetDesc().MipLevels;
	srvDesc.TextureCube.MostDetailedMip = 0;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.f;
	srvDesc.Format = skyCubeMap->GetDesc().Format;

	m_d3dDevice->CreateShaderResourceView(skyCubeMap, &srvDesc, viewHandle);
	m_SkyTexHeapIndex = viewCount++;

	// 미리 맴버에다가 offset을 저장하고.
	m_ShadowMapHeapIndex = viewCount++;
	m_NullCubeSrvIndex = viewCount++;
	m_NullTexSrvIndex = viewCount++;

	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart = m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCpuStart = m_DsvHeap->GetCPUDescriptorHandleForHeapStart();

	// 안쓰는 Cube Srv와 Tex Srv를 nullptr을 굳이 넣어주는 이유는 잘 모르겠지만...
	CD3DX12_CPU_DESCRIPTOR_HANDLE nullSrv = CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_NullCubeSrvIndex, m_CbvSrvUavDescriptorSize);
	m_NullSrv = CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_NullCubeSrvIndex, m_CbvSrvUavDescriptorSize);

	m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, m_CbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	// 클래스 인스턴스에 있는 텍스쳐 들을 view heap과 연결해준다.
	m_ShadowMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_ShadowMapHeapIndex, m_CbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_ShadowMapHeapIndex, m_CbvSrvUavDescriptorSize),
		CD3DX12_CPU_DESCRIPTOR_HANDLE(dsvCpuStart, 1, m_DsvDescriptorSize) // 두번째 dsv offset을 가진다.
	);
}

void ShadowMappingApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\17_ShadowMapping.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\17_ShadowMapping.hlsl", nullptr, "PS", "ps_5_1");
	//m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\17_ShadowMapping.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_Shaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", alphaTestDefines, "PS", "ps_5_1");
	 
	m_Shaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 2 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void ShadowMappingApp::BuildStageGeometry()
{
	// 박스, 그리드, 구, 원기둥을 하나씩 만들고
	GeometryGenerator geoGenerator;
	GeometryGenerator::MeshData box = geoGenerator.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGenerator.CreateGrid(20.f, 30.f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGenerator.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGenerator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);
	GeometryGenerator::MeshData quad = geoGenerator.CreateQuad(0.f, -1.f, 1.f, 1.f, 0.0f); // 그림자 디버깅용 창을 하나 만든다.
#if PRAC1
	GeometryGenerator::MeshData screen = geoGenerator.CreateGrid(10.f, 10.f, 4, 4);
#endif

	// 이거를 하나의 버퍼로 전부 연결한다.

	// 그 전에, 각종 속성 들을 저장해 놓는다.
	// vertex offset
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();
	UINT quadVertexOffset = cylinderVertexOffset + (UINT)cylinder.Vertices.size();
#if PRAC1
	UINT screenVertexOffset = quadVertexOffset + (UINT)quad.Vertices.size();
#endif

	// index offset
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();
	UINT quadIndexOffset = cylinderIndexOffset + (UINT)cylinder.Indices32.size();
#if PRAC1
	UINT screenIndexOffset = quadIndexOffset + (UINT)quad.Indices32.size();
#endif

	// 한방에 할꺼라서 MeshGeometry에 넣을
	// SubGeometry로 정의한다.
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = boxIndexOffset;
	boxSubmesh.BaseVertexLocation = boxVertexOffset;

	SubmeshGeometry gridSubmesh;
	gridSubmesh.IndexCount = (UINT)grid.Indices32.size();
	gridSubmesh.StartIndexLocation = gridIndexOffset;
	gridSubmesh.BaseVertexLocation = gridVertexOffset;

	SubmeshGeometry sphereSubmesh;
	sphereSubmesh.IndexCount = (UINT)sphere.Indices32.size();
	sphereSubmesh.StartIndexLocation = sphereIndexOffset;
	sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

	SubmeshGeometry cylinderSubmesh;
	cylinderSubmesh.IndexCount = (UINT)cylinder.Indices32.size();
	cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
	cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)quad.Indices32.size();
	quadSubmesh.StartIndexLocation = quadIndexOffset;
	quadSubmesh.BaseVertexLocation = quadVertexOffset;
#if PRAC1
	SubmeshGeometry screenSubmesh;
	screenSubmesh.IndexCount = (UINT)screen.Indices32.size();
	screenSubmesh.StartIndexLocation = screenIndexOffset;
	screenSubmesh.BaseVertexLocation = screenVertexOffset;
#endif

	// 이제 vertex 정보를 한곳에 다 옮기고, 색을 지정해준다.
	size_t totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size() +
		quad.Vertices.size();
#if PRAC1
	totalVertexCount += screen.Vertices.size();
#endif

	std::vector<Vertex> vertices;
	vertices.resize(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
		vertices[k].TangentU = box.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
		vertices[k].TangentU = grid.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
		vertices[k].TangentU = sphere.Vertices[i].TangentU;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
		vertices[k].TangentU = cylinder.Vertices[i].TangentU;
	}

	for (int i = 0; i < quad.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = quad.Vertices[i].Position;
		vertices[k].Normal = quad.Vertices[i].Normal;
		vertices[k].TexC = quad.Vertices[i].TexC;
		vertices[k].TangentU = quad.Vertices[i].TangentU;
	}
#if PRAC1
	for (int i = 0; i < screen.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = screen.Vertices[i].Position;
		vertices[k].Normal = screen.Vertices[i].Normal;
		vertices[k].TexC = screen.Vertices[i].TexC;
		vertices[k].TangentU = screen.Vertices[i].TangentU;
	}
#endif

	// 이제 index 정보도 한곳에 다 옮긴다.
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));
	indices.insert(indices.end(), std::begin(quad.GetIndices16()), std::end(quad.GetIndices16()));
#if PRAC1
	indices.insert(indices.end(), std::begin(screen.GetIndices16()), std::end(screen.GetIndices16()));
#endif

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "shapeGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		vertices.data(),
		vbByteSize,
		geo->VertexBufferUploader
	);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	geo->DrawArgs["box"] = boxSubmesh;
	geo->DrawArgs["grid"] = gridSubmesh;
	geo->DrawArgs["sphere"] = sphereSubmesh;
	geo->DrawArgs["cylinder"] = cylinderSubmesh;
	geo->DrawArgs["quad"] = quadSubmesh;
#if PRAC1
	geo->DrawArgs["screen"] = screenSubmesh;
#endif

	m_Geometries[geo->Name] = std::move(geo);
}

void ShadowMappingApp::BuildSkullGeometry()
{
	vector<Vertex> SkullVertices;
	vector<uint32_t> SkullIndices;

	BoundingBox bounds;
	Dorasima::GetMeshFromFile(L"..\\04_RenderSmoothly_Wave\\Skull.txt", SkullVertices, SkullIndices, bounds);

	const UINT vbByteSize = (UINT)SkullVertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)SkullIndices.size() * sizeof(std::uint32_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "SkullGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), SkullVertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), SkullIndices.data(), ibByteSize);

	// Vertex 와 Index 정보를 Default Buffer로 만들고
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		SkullVertices.data(),
		vbByteSize,
		geo->VertexBufferUploader
	);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		SkullIndices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	// 서브 메쉬 설정을 해준다.
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)SkullIndices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["skull"] = submesh;

	m_Geometries["SkullGeo"] = std::move(geo);
}

void ShadowMappingApp::BuildPSOs()
{
	// 불투명 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSODesc;

	ZeroMemory(&opaquePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePSODesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	opaquePSODesc.pRootSignature = m_RootSignature.Get();
	opaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	opaquePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	opaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePSODesc.SampleMask = UINT_MAX;
	opaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePSODesc.NumRenderTargets = 1;
	opaquePSODesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePSODesc.DSVFormat = m_DepthStencilFormat;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaquePSODesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	// Shadow Map을 생성할 때 사용하는 PSO다
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDrawPSODesc = opaquePSODesc;
	// 생성되는 삼각형 마다, (여기서는 광원에서 바라보는)카메라와의 각도를 계산해서
	// bias를 속성 값에 따라 계산해서 적용해준다. 이거는 예쁘게 나오는 값들을 실험적으로 찾아야 한다.
	// 아래 속성 값들의 사용법 : https://learn.microsoft.com/ko-kr/windows/win32/direct3d11/d3d10-graphics-programming-guide-output-merger-stage-depth-bias
	shadowDrawPSODesc.RasterizerState.DepthBias = 100000;
	shadowDrawPSODesc.RasterizerState.DepthBiasClamp = 0.f;
	shadowDrawPSODesc.RasterizerState.SlopeScaledDepthBias = 1.f;
	shadowDrawPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["shadowVS"]->GetBufferPointer()),
		m_Shaders["shadowVS"]->GetBufferSize()
	};
	shadowDrawPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["shadowOpaquePS"]->GetBufferPointer()),
		m_Shaders["shadowOpaquePS"]->GetBufferSize()
	};
	// 얘는 Render Target이 없다.
	shadowDrawPSODesc.RTVFormats[0] = DXGI_FORMAT_UNKNOWN;
	shadowDrawPSODesc.NumRenderTargets = 0;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&shadowDrawPSODesc, IID_PPV_ARGS(&m_PSOs["shadow_opaque"])));

	// debug Layer용 PSO다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPSODesc = opaquePSODesc;
	debugPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["debugVS"]->GetBufferPointer()),
		m_Shaders["debugVS"]->GetBufferSize()
	};
	debugPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["debugPS"]->GetBufferPointer()),
		m_Shaders["debugPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&debugPSODesc, IID_PPV_ARGS(&m_PSOs["debug"])));


	// Sky용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPSODesc = opaquePSODesc;

	// 커다란 구로 표현을 할거고, 안쪽에서 바라보기 때문에 cull을 반대로 해줘야한다.
	skyPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	// z가 항상 1로 고정이 되기 때문에 LESS_EQUAL로 걸어줘야 한다.
	// Init()에서 Clear를 할때 Depth Clear를 1.f로 한다.
	// 안그러면 초기 화면 값만 보이게 된다.
	skyPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	skyPSODesc.pRootSignature = m_RootSignature.Get();
	skyPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyVS"]->GetBufferPointer()),
		m_Shaders["skyVS"]->GetBufferSize()
	};
	skyPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skyPS"]->GetBufferPointer()),
		m_Shaders["skyPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&skyPSODesc, IID_PPV_ARGS(&m_PSOs["sky"])));
}

void ShadowMappingApp::BuildFrameResources()
{
	// Shadow Map 용으로 하나 더 필요하다.
	UINT passCBCount = 2;

	for (int i = 0; i < g_NumFrameResources; ++i)
	{
		m_FrameResources.push_back(
			std::make_unique<FrameResource>(m_d3dDevice.Get(),
			passCBCount,
			(UINT)m_AllRenderItems.size(),
			(UINT)m_Materials.size()
		));
	}
}

void ShadowMappingApp::BuildMaterials()
{
	//0		m_Textures["bricksDiffuseMap"]->Resource.Get(),
	//1		m_Textures["bricksNormalMap"]->Resource.Get(),
	//2		m_Textures["tileDiffuseMap"]->Resource.Get(),
	//3		m_Textures["tileNormalMap"]->Resource.Get(),
	//4		m_Textures["defaultDiffuseMap"]->Resource.Get(),
	//5		m_Textures["defaultNormalMap"]->Resource.Get(),

	std::unique_ptr<Material> brickMat = std::make_unique<Material>();
	brickMat->Name = "brickMat";
	brickMat->MatCBIndex = 0;
	brickMat->DiffuseSrvHeapIndex = 0;
	brickMat->NormalSrvHeapIndex = 1;
	brickMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brickMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	brickMat->Roughness = 0.3f;

	std::unique_ptr<Material> tileMat = std::make_unique<Material>();
	tileMat->Name = "tileMat";
	tileMat->MatCBIndex = 1;
	tileMat->DiffuseSrvHeapIndex = 2;
	tileMat->NormalSrvHeapIndex = 3;
	tileMat->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tileMat->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tileMat->Roughness = 0.1f;

	std::unique_ptr<Material> mirrorMat = std::make_unique<Material>();
	mirrorMat->Name = "mirrorMat";
	mirrorMat->MatCBIndex = 2;
	mirrorMat->DiffuseSrvHeapIndex = 4;
	mirrorMat->NormalSrvHeapIndex = 5;
	mirrorMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	mirrorMat->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirrorMat->Roughness = 0.1f;

	std::unique_ptr<Material> skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 3;
	skullMat->DiffuseSrvHeapIndex = 4;
	skullMat->NormalSrvHeapIndex = 5;
	skullMat->DiffuseAlbedo = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.7f, 0.7f, 0.7f);
	skullMat->Roughness = 0.2f;

	std::unique_ptr<Material> skyMat = std::make_unique<Material>();
	skyMat->Name = "skyMat";
	skyMat->MatCBIndex = 4;
	skyMat->DiffuseSrvHeapIndex = 4;
	skyMat->NormalSrvHeapIndex = 5;
	skyMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);;
	skyMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	skyMat->Roughness = 1.0f;

	m_Materials["brickMat"] = std::move(brickMat);
	m_Materials["mirrorMat"] = std::move(mirrorMat);
	m_Materials["tileMat"] = std::move(tileMat);
	m_Materials["skullMat"] = std::move(skullMat);
	m_Materials["skyMat"] = std::move(skyMat);
}

void ShadowMappingApp::BuildRenderItems()
{
	UINT objCBIndex = 0;
	std::unique_ptr<RenderItem> skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->WorldMat, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = objCBIndex++;
	skyRitem->Mat = m_Materials["skyMat"].get();
	skyRitem->Geo = m_Geometries["shapeGeo"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	m_AllRenderItems.push_back(std::move(skyRitem));

	std::unique_ptr<RenderItem> quadRitem = std::make_unique<RenderItem>();
	quadRitem->WorldMat = MathHelper::Identity4x4();
	quadRitem->TexTransform = MathHelper::Identity4x4();
	quadRitem->ObjCBIndex = objCBIndex++;
	quadRitem->Mat = m_Materials["brickMat"].get();
	quadRitem->Geo = m_Geometries["shapeGeo"].get();
	quadRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	quadRitem->IndexCount = quadRitem->Geo->DrawArgs["quad"].IndexCount;
	quadRitem->StartIndexLocation = quadRitem->Geo->DrawArgs["quad"].StartIndexLocation;
	quadRitem->BaseVertexLocation = quadRitem->Geo->DrawArgs["quad"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Debug].push_back(quadRitem.get());
	m_AllRenderItems.push_back(std::move(quadRitem));

#if PRAC1 
	std::unique_ptr<RenderItem> screenRitem = std::make_unique<RenderItem>();
	XMMATRIX screenTransformMat = XMMatrixScaling(0.8f, 1.f, 1.5f) * XMMatrixRotationZ(- XM_PIDIV2) * XMMatrixTranslation(-10.f, 4.f, 7.5f);
	XMStoreFloat4x4(&screenRitem->WorldMat, screenTransformMat);
	XMMATRIX screenTexMat = XMMatrixRotationZ(-XM_PIDIV2);
	//XMStoreFloat4x4(&screenRitem->TexTransform, screenTexMat);
	//screenRitem->WorldMat = MathHelper::Identity4x4();
	screenRitem->TexTransform = MathHelper::Identity4x4();
	screenRitem->ObjCBIndex = objCBIndex++;
	screenRitem->Mat = m_Materials["brickMat"].get();
	screenRitem->Geo = m_Geometries["shapeGeo"].get();
	screenRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	screenRitem->IndexCount = screenRitem->Geo->DrawArgs["screen"].IndexCount;
	screenRitem->StartIndexLocation = screenRitem->Geo->DrawArgs["screen"].StartIndexLocation;
	screenRitem->BaseVertexLocation = screenRitem->Geo->DrawArgs["screen"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(screenRitem.get());
	m_AllRenderItems.push_back(std::move(screenRitem));
#endif

	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = objCBIndex++;
	boxRitem->Geo = m_Geometries["shapeGeo"].get();
	boxRitem->Mat = m_Materials["brickMat"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	m_AllRenderItems.push_back(std::move(boxRitem));

	// 그리드 아이템
	std::unique_ptr<RenderItem> gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(6.0f, 9.0f, 1.0f));
	gridRitem->WorldMat = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = objCBIndex++;
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->Mat = m_Materials["tileMat"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	m_AllRenderItems.push_back(std::move(gridRitem));

	// 원기둥과 구 5개씩 2줄
	for (int i = 0; i < 5; ++i)
	{
		std::unique_ptr<RenderItem> leftCylRitem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightCylRitem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> leftSphereRitem = std::make_unique<RenderItem>();
		std::unique_ptr<RenderItem> rightSphereRitem = std::make_unique<RenderItem>();

		//  z 축으로 10칸씩 옮긴다.
		XMMATRIX leftCylWorld = XMMatrixTranslation(-5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX rightCylWorld = XMMatrixTranslation(+5.0f, 1.5f, -10.0f + i * 5.0f);
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->WorldMat, rightCylWorld);
		XMStoreFloat4x4(&leftCylRitem->TexTransform, XMMatrixIdentity());
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = m_Geometries["shapeGeo"].get();
		leftCylRitem->Mat = m_Materials["brickMat"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->WorldMat, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, XMMatrixIdentity());
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = m_Geometries["shapeGeo"].get();
		rightCylRitem->Mat = m_Materials["brickMat"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->WorldMat, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		leftSphereRitem->Mat = m_Materials["mirrorMat"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->WorldMat, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->Mat = m_Materials["mirrorMat"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
		m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

		m_AllRenderItems.push_back(std::move(leftCylRitem));
		m_AllRenderItems.push_back(std::move(rightCylRitem));
		m_AllRenderItems.push_back(std::move(leftSphereRitem));
		m_AllRenderItems.push_back(std::move(rightSphereRitem));
	}

	std::unique_ptr<RenderItem> skullRenderItem = std::make_unique<RenderItem>();
	XMMATRIX ScaleHalfMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX TranslateDown5Units = XMMatrixTranslation(0.f, 1.f, 0.f);
	XMMATRIX SkullWorldMat = ScaleHalfMat * TranslateDown5Units;
	XMStoreFloat4x4(&skullRenderItem->WorldMat, SkullWorldMat);
	skullRenderItem->TexTransform = MathHelper::Identity4x4();
	skullRenderItem->ObjCBIndex = objCBIndex++;
	skullRenderItem->Geo = m_Geometries["SkullGeo"].get();
	skullRenderItem->Mat = m_Materials["skullMat"].get();
	skullRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->IndexCount = skullRenderItem->Geo->DrawArgs["skull"].IndexCount;
	skullRenderItem->StartIndexLocation = skullRenderItem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRenderItem->BaseVertexLocation = skullRenderItem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());

	m_AllRenderItems.push_back(std::move(skullRenderItem));
}

void ShadowMappingApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < _renderItems.size(); i++)
	{
		RenderItem* ri = _renderItems[i];
		if (ri->Visible == false)
		{
			continue;
		}

		// 일단 Vertex, Index를 IA에 세팅해주고
		D3D12_VERTEX_BUFFER_VIEW VBView = ri->Geo->VertexBufferView();
		_cmdList->IASetVertexBuffers(0, 1, &VBView);
		D3D12_INDEX_BUFFER_VIEW IBView = ri->Geo->IndexBufferView();
		_cmdList->IASetIndexBuffer(&IBView);
		if (_Type == D3D_PRIMITIVE_TOPOLOGY_UNDEFINED)
		{
			_cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
		}
		else
		{
			_cmdList->IASetPrimitiveTopology(_Type);
		}
		// Object Constant와
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;

		// 이제 Item은 Object Buffer만 넘겨주어도 된다.
		_cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);

		_cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void ShadowMappingApp::DrawSceneToShadowMap()
{
	// 인스턴스 맴버 값을 이용해서, Rasterization 속성 값을 설정한다.
	D3D12_VIEWPORT shadowViewPort = m_ShadowMap->GetViewport();
	m_CommandList->RSSetViewports(1, &shadowViewPort);
	D3D12_RECT shadowRect = m_ShadowMap->GetScissorRect();
	m_CommandList->RSSetScissorRects(1, &shadowRect);

	// Barrier 설정을 하고
	CD3DX12_RESOURCE_BARRIER shadowDS_READ_WRITE = CD3DX12_RESOURCE_BARRIER::Transition(
		m_ShadowMap->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_CommandList->ResourceBarrier(1, &shadowDS_READ_WRITE);

	// Depth view를 clear해주고
	m_CommandList->ClearDepthStencilView(
		m_ShadowMap->GetDsv(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
		1.f, 0, 0, nullptr);

	// Render Target은 nullptr로 한다. depth 판정 값만 기록할 것이라, Depth-Stencil View만 넘겨준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE shadowDsvHandle = m_ShadowMap->GetDsv();
	m_CommandList->OMSetRenderTargets(0, nullptr, false, &shadowDsvHandle);

	// pass CB을 frame resource의 두번째 CBV에 바인드해주고
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + passCBByteSize;
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCBAddress);

	// 광원 입장에서 보는 물체들을 그린다.
	m_CommandList->SetPipelineState(m_PSOs["shadow_opaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// Dsv Barrier를 다시 Read로 바꾼다.
	CD3DX12_RESOURCE_BARRIER shadowDS_WRITE_READ = CD3DX12_RESOURCE_BARRIER::Transition(
		m_ShadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_CommandList->ResourceBarrier(1, &shadowDS_WRITE_READ);
}

float ShadowMappingApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 ShadowMappingApp::GetHillsNormal(float _x, float _z) const
{
	// n = (-df/dx, 1, -df/dz)
	// 이게 왜그러냐면
	// https://en.wikipedia.org/wiki/Normal_(geometry)
	// 각 축에 대해서 부분 도함수를 구한 다음에
	// 외적 한것이 노멀인데, 그걸 식으로 정리한 것이다.
	XMFLOAT3 n(
		-0.03f * _z * cosf(0.1f * _x) - 0.3f * cosf(0.1f * _z),
		1.0f,
		-0.3f * sinf(0.1f * _x) + 0.03f * _x * sinf(0.1f * _z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 11> ShadowMappingApp::GetStaticSamplers()
{
	// 일반적인 앱에서는 쓰는 샘플러만 사용한다.
	// 그래서 미리 만들어 놓고 루트서명에 넣어둔다.
	
	// 근접점 필터링 - 반복
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // s0 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // 필터 종류
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // u 방향 텍스쳐 지정 모드
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, // w 방향 텍스쳐 지정 모드
		D3D12_TEXTURE_ADDRESS_MODE_WRAP // w 방향 텍스쳐 지정 모드
	);

	// 근접점 필터링 - 자르고 늘이기
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // s1 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	// 근접점 필터링 - 미러
	const CD3DX12_STATIC_SAMPLER_DESC pointMirror(
		2, // s3 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR); // addressW
	// 근접점 필터링 - 색 채우기
	CD3DX12_STATIC_SAMPLER_DESC pointBorder(
		3, // s4 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER);	// addressW
	pointBorder.BorderColor = D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	// 주변선형 보간 필터링 - 반복
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		4, // s5 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// 주변선형 보간 필터링 - 자르고 늘이기
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		5, // s6 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
	// 주변선형 보간 필터링 - 미러
	const CD3DX12_STATIC_SAMPLER_DESC linearMirror(
		6, // s7 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR); // addressW
	// 주변선형 보간 필터링 - 색채우기
	CD3DX12_STATIC_SAMPLER_DESC linearBorder(
		7, // s8 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER); // addressW
	linearBorder.BorderColor = D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		8, // s9 - 레지스터 번호
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                              // LOD를 계산하는 offset(bias) 값이라고 한다.
		8);                                // 최대 비등방 값 (높을 수록 비싸고, 예뻐진다.)

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		9, // s10 - 레지스터 번호
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // LOD를 계산하는 offset(bias) 값이라고 한다.
		8);                                // 최대 비등방 값 (높을 수록 비싸고, 예뻐진다.)

	const CD3DX12_STATIC_SAMPLER_DESC SamplerComparisonStateForShadow(
		10, // s11 - 레지스터 번호
		D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // hlsl에서 SampleCmp를 사용할 수 있게하는 filter이다.
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU (테두리 색으로 지정한다.)
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,                              // LOD를 계산하는 offset(bias) 값이라고 한다.
		16,									// 최대 비등방 값 (높을 수록 비싸고, 예뻐진다.)
		D3D12_COMPARISON_FUNC_LESS_EQUAL, // 샘플링된 데이터 비교 옵션이다. (원본이 비교보다 작거나 같으면 넘겨준다.)
		D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);                                 

	return {
		pointWrap, pointClamp, pointMirror, pointBorder,
		linearWrap, linearClamp, linearMirror, linearBorder,
		anisotropicWrap, anisotropicClamp, SamplerComparisonStateForShadow };
}
