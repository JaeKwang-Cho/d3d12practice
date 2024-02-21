﻿//***************************************************************************************
// FPSCameraApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>
#include "FileReader.h"
#include "../Common/Camera.h"

const int g_NumFrameResources = 3;

#define PRAC3 (1)

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
	UINT ObjCBIndex = 1;

	MeshGeometry* Geo = nullptr;
	// Material 포인터 대신 인덱스만 가지고 있는다.
	UINT MatIndex = 0;

	// 다른걸 쓸일은 잘 없을 듯
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced 인자이다.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

// 드디어 불투명, 반투명, 알파자르기
// 이외에도 stencil을 사용하는
// Mirror, Reflected, Shadow가 추가 되었다.
enum class RenderLayer : int
{
	Opaque = 0,
	Transparent,
	AlphaTested,
	Count
};

class FPSCameraApp : public D3DApp
{
public:
	FPSCameraApp(HINSTANCE hInstance);
	~FPSCameraApp();

	FPSCameraApp(const FPSCameraApp& _other) = delete;
	FPSCameraApp& operator=(const FPSCameraApp& _other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& _gt) override;
	virtual void Draw(const GameTimer& _gt) override;

	virtual void OnMouseDown(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseUp(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove(WPARAM _btnState, int _x, int _y) override;
	
	// ==== Update ====
	void OnKeyboardInput(const GameTimer _gt);
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMaterialCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);
	void AnimateMaterials(const GameTimer& _gt);

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

	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY  _Type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
	void DrawAllSubRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY  _Type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

	// 지형의 높이와 노멀을 계산하는 함수다.
	float GetHillsHeight(float _x, float _z) const;
	XMFLOAT3 GetHillsNormal(float _x, float _z)const;

	// Practice 3
	void LoadTextures_Prac3();
	void BuildDescriptorHeaps_Prac3();
	void BuildShadersAndInputLayout_Prac3();
	void BuildBoxesGeometry_Prac3();
	void BuildMaterials_Prac3();
	void BuildRenderItems_Prac3();
	void InitPractice3()
	{
		LoadTextures_Prac3();
		BuildRootSignature();
		BuildDescriptorHeaps_Prac3();
		BuildShadersAndInputLayout_Prac3();
		BuildBoxesGeometry_Prac3();
		BuildMaterials_Prac3();
		BuildRenderItems_Prac3();
		BuildPSOs();
		BuildFrameResources();
	}

	// 정적 샘플러 구조체를 미리 만드는 함수이다.
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> GetStaticSamplers();

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
	// Material 도 추가한다.
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	// Texture도 추가한다.
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

	// Camera
	Camera m_Camera;

	float m_Phi = 1.24f * XM_PI;
	float m_Theta = 0.42f * XM_PI;
	float m_Radius = 20.f;

	POINT m_LastMousePos = {};
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
		FPSCameraApp theApp(hInstance);
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

FPSCameraApp::FPSCameraApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

FPSCameraApp::~FPSCameraApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool FPSCameraApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_Camera.SetPosition(0.0f, 2.0f, -15.0f);

	// ======== 초기화 ==========

#if PRAC3
	InitPractice3();
#else
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
#endif
	// =========================


	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void FPSCameraApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetFrustum(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
}

void FPSCameraApp::Update(const GameTimer& _gt)
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

	AnimateMaterials(_gt);
	// CB를 업데이트 해준다.
	UpdateObjectCBs(_gt);
	UpdateMaterialCBs(_gt);
	UpdateMainPassCB(_gt);
}

void FPSCameraApp::Draw(const GameTimer& _gt)
{
	// 현재 FrameResource가 가지고 있는 allocator를 가지고 와서 초기화 한다.
	ComPtr<ID3D12CommandAllocator> CurrCommandAllocator = m_CurrFrameResource->CmdListAlloc;
	ThrowIfFailed(CurrCommandAllocator->Reset());

	// Command List도 초기화를 한다
	// 근데 Command List에 Reset()을 걸려면, 한번은 꼭 Command Queue에 
	// ExecuteCommandList()로 등록이 된적이 있어야 가능하다.

	// PSO별로 등록하면서, Allocator와 함께, Command List를 초기화 한다.
	ThrowIfFailed(m_CommandList->Reset(CurrCommandAllocator.Get(), m_PSOs["opaque"].Get()));

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
		(float*)&m_MainPassCB.FogColor,  // 근데 여기서 좀더 사실감을 살리기 위해 배경을 fog 색으로 채운다.
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

	// ============== 그리기 =================
	
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvSrvUavDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// PSO에 Root Signature를 세팅해준다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// 현재 Frame Constant Buffer를 업데이트한다.
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress()); // Pass는 1번

	// 현재 프레임에 사용하는 Material Buffer를 Srv로 바인드 해준다.
	ID3D12Resource* matStructredBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
	m_CommandList->SetGraphicsRootShaderResourceView(2, matStructredBuffer->GetGPUVirtualAddress()); // Mat는 2번

	// 택스쳐가 올라가 있는 view heap에서 텍스쳐 첫번째 핸들을 설정해준다.
	// (첫번째 것만 설정해서 넘겨주면 된다. Root Signature에 이미 정보가 다 있다.)
	m_CommandList->SetGraphicsRootDescriptorTable(3, m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); // Tex arr는 3번

	// 일단 불투명한 애들을 먼저 싹 출력을 해준다.
	// 그래야 alpha 계산이 가능하다.
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 알파 자르기하는 친구들을 출력해주고
	m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::AlphaTested]);

	// 이제 반투명한 애들을 출력해준다.
	m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Transparent]);

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

void FPSCameraApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void FPSCameraApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void FPSCameraApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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

void FPSCameraApp::OnKeyboardInput(const GameTimer _gt)
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

void FPSCameraApp::UpdateObjectCBs(const GameTimer& _gt)
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
			objConstants.MaterialIndex = e->MatIndex;
			
			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void FPSCameraApp::UpdateMaterialCBs(const GameTimer& _gt)
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
			matData.DiffuseMapIndex = mat->MatCBIndex;

			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			mat->NumFramesDirty--;
		}
	}
}

void FPSCameraApp::UpdateMainPassCB(const GameTimer& _gt)
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

	XMStoreFloat4x4(&m_MainPassCB.ViewMat, XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&m_MainPassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
	XMStoreFloat4x4(&m_MainPassCB.ProjMat, XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&m_MainPassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
	XMStoreFloat4x4(&m_MainPassCB.VPMat, XMMatrixTranspose(VPMat));
	XMStoreFloat4x4(&m_MainPassCB.InvVPMat, XMMatrixTranspose(InvVPMat));

	m_MainPassCB.EyePosW = m_Camera.GetPosition3f();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.f / m_ClientWidth, 1.f / m_ClientHeight);
	m_MainPassCB.NearZ = 0.01f;
	m_MainPassCB.FarZ = 1000.f;
	m_MainPassCB.TotalTime = _gt.GetTotalTime();
	m_MainPassCB.DeltaTime = _gt.GetDeltaTime();

	// 사실 이미 값이 있긴 한데... 그냥 한번 더 써준거다.
	m_MainPassCB.FogColor = { 0.7f, 0.7f, 0.7f, 1.f };
	m_MainPassCB.FogStart = 5.f;
	m_MainPassCB.FogRange = 150.f;

	// 이제 Light를 채워준다. Shader와 App에서 정의한 Lights의 개수와 종류가 같아야 한다.
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.0f };
	m_MainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[0].Strength = { 0.9f, 0.9f, 0.8f };
	m_MainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	m_MainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	m_MainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	m_MainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };
	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void FPSCameraApp::AnimateMaterials(const GameTimer& _gt)
{
}


void FPSCameraApp::LoadTextures()
{
	std::unique_ptr<Texture> skullTex = std::make_unique<Texture>();
	skullTex->Name = "skullTex";
	skullTex->Filename = L"../Textures/ice.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		skullTex->Filename.c_str(),
		skullTex->Resource,
		skullTex->UploadHeap
	));

	m_Textures[skullTex->Name] = std::move(skullTex);

	std::unique_ptr<Texture> bricksTex = std::make_unique<Texture>();
	bricksTex->Name = "bricksTex";
	bricksTex->Filename = L"../Textures/bricks.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		bricksTex->Filename.c_str(),
		bricksTex->Resource,
		bricksTex->UploadHeap
	));

	m_Textures[bricksTex->Name] = std::move(bricksTex);

	std::unique_ptr<Texture> stoneTex = std::make_unique<Texture>();
	stoneTex->Name = "stoneTex";
	stoneTex->Filename = L"../Textures/stone.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		stoneTex->Filename.c_str(),
		stoneTex->Resource,
		stoneTex->UploadHeap
	));

	m_Textures[stoneTex->Name] = std::move(stoneTex);

	std::unique_ptr<Texture> tileTex = std::make_unique<Texture>();
	tileTex->Name = "tileTex";
	tileTex->Filename = L"../Textures/tile.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		tileTex->Filename.c_str(),
		tileTex->Resource,
		tileTex->UploadHeap
	));

	m_Textures[tileTex->Name] = std::move(tileTex);

	// 텍스쳐를 사용하지 않을 때 쓰는 친구다.
	std::unique_ptr<Texture> white1x1Tex = std::make_unique<Texture>();
	white1x1Tex->Name = "white1x1Tex";
	white1x1Tex->Filename = L"../Textures/white1x1.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		white1x1Tex->Filename.c_str(),
		white1x1Tex->Resource,
		white1x1Tex->UploadHeap
	));
	m_Textures[white1x1Tex->Name] = std::move(white1x1Tex);
}


void FPSCameraApp::BuildRootSignature()
{	
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	UINT numOfParameter = 4;

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable;
	// Texture2D를 Index로 접근할 것 이기 때문에 이렇게 5개로 만들어 준다.
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 0); // t0 - texture 정보
	
	// Srv가 넘어가는 테이블, Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[1].InitAsConstantBufferView(1); // b1 - PassConstants
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // t0 - Material Structred Buffer, Space도 1로 지정해준다.
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL); // t0 - Texture2D Array

	// DirectX에서 제공하는 Heap을 만들지 않고, Sampler를 생성할 수 있게 해주는
	// Static Sampler 방법이다. 최대 2000개 생성 가능
	// (원래는 D3D12_DESCRIPTOR_HEAP_DESC 가지고 D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER막 해줘야 한다.)
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> staticSamplers = GetStaticSamplers();

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

void FPSCameraApp::BuildDescriptorHeaps()
{
	const int textureDescriptorCount = 5;

	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = textureDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CbvSrvUavDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* skullTex = m_Textures["skullTex"].get()->Resource.Get();
	ID3D12Resource* bricksTex = m_Textures["bricksTex"].get()->Resource.Get();
	ID3D12Resource* stoneTex = m_Textures["stoneTex"].get()->Resource.Get();
	ID3D12Resource* tileTex = m_Textures["tileTex"].get()->Resource.Get();
	ID3D12Resource* whiteTex = m_Textures["white1x1Tex"].get()->Resource.Get();

	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = skullTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = -1;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(skullTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = bricksTex->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = bricksTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(bricksTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = stoneTex->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = stoneTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(stoneTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = tileTex->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(tileTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = whiteTex->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = whiteTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(whiteTex, &srcDesc, viewHandle);
}

void FPSCameraApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\12_FPSCamera_Indexing.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\12_FPSCamera_Indexing.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\12_FPSCamera_Indexing.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void FPSCameraApp::BuildStageGeometry()
{
	// 박스, 그리드, 구, 원기둥을 하나씩 만들고
	GeometryGenerator geoGenerator;
	GeometryGenerator::MeshData box = geoGenerator.CreateBox(1.5f, 0.5f, 1.5f, 3);
	GeometryGenerator::MeshData grid = geoGenerator.CreateGrid(20.f, 30.f, 60, 40);
	GeometryGenerator::MeshData sphere = geoGenerator.CreateSphere(0.5f, 20, 20);
	GeometryGenerator::MeshData cylinder = geoGenerator.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

	// 이거를 하나의 버퍼로 전부 연결한다.

	// 그 전에, 각종 속성 들을 저장해 놓는다.
	// vertex offset
	UINT boxVertexOffset = 0;
	UINT gridVertexOffset = (UINT)box.Vertices.size();
	UINT sphereVertexOffset = gridVertexOffset + (UINT)grid.Vertices.size();
	UINT cylinderVertexOffset = sphereVertexOffset + (UINT)sphere.Vertices.size();

	// index offset
	UINT boxIndexOffset = 0;
	UINT gridIndexOffset = (UINT)box.Indices32.size();
	UINT sphereIndexOffset = gridIndexOffset + (UINT)grid.Indices32.size();
	UINT cylinderIndexOffset = sphereIndexOffset + (UINT)sphere.Indices32.size();

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

	// 이제 vertex 정보를 한곳에 다 옮기고, 색을 지정해준다.
	size_t totalVertexCount =
		box.Vertices.size() +
		grid.Vertices.size() +
		sphere.Vertices.size() +
		cylinder.Vertices.size();

	std::vector<Vertex> vertices;
	vertices.resize(totalVertexCount);

	UINT k = 0;
	for (size_t i = 0; i < box.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = box.Vertices[i].Position;
		vertices[k].Normal = box.Vertices[i].Normal;
		vertices[k].TexC = box.Vertices[i].TexC;
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
		vertices[k].TexC = grid.Vertices[i].TexC;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
		vertices[k].TexC = sphere.Vertices[i].TexC;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
		vertices[k].TexC = cylinder.Vertices[i].TexC;
	}

	// 이제 index 정보도 한곳에 다 옮긴다.
	std::vector<std::uint16_t> indices;
	indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
	indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
	indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
	indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

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

	m_Geometries[geo->Name] = std::move(geo);
}

void FPSCameraApp::BuildSkullGeometry()
{
	vector<Vertex> SkullVertices;
	vector<uint32_t> SkullIndices;

	Dorasima::Prac3VerticesNIndicies(L"..\\04_RenderSmoothly_Wave\\Skull.txt", SkullVertices, SkullIndices);

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

void FPSCameraApp::BuildPSOs()
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

	// 반투명 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPSODesc = opaquePSODesc;

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	// transparencyBlendDesc.SrcBlend = D3D12_BLEND_BLEND_FACTOR;
	// transparencyBlendDesc.DestBlend = D3D12_BLEND_BLEND_FACTOR;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparentPSODesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&transparentPSODesc, IID_PPV_ARGS(&m_PSOs["transparent"])));

	// 알파 자르기 PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPSODesc = opaquePSODesc;
	alphaTestPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["alphaTestedPS"]->GetBufferPointer()),
		m_Shaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&alphaTestPSODesc, IID_PPV_ARGS(&m_PSOs["alphaTested"])));
}

void FPSCameraApp::BuildFrameResources()
{
	UINT passCBCount = 1;
	
	// 반사상 용 PassCB가 하나 추가 된다.
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

void FPSCameraApp::BuildMaterials()
{
	std::unique_ptr<Material> skullMat = std::make_unique<Material>();
	skullMat->Name = "skullMat";
	skullMat->MatCBIndex = 0;
	skullMat->DiffuseSrvHeapIndex = 0;
	skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	skullMat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	skullMat->Roughness = 0.125f;

	std::unique_ptr<Material> brickMat = std::make_unique<Material>();
	brickMat->Name = "brickMat";
	brickMat->MatCBIndex = 1;
	brickMat->DiffuseSrvHeapIndex = 1;
	brickMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brickMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	brickMat->Roughness = 0.3f;

	std::unique_ptr<Material> stoneMat = std::make_unique<Material>();
	stoneMat->Name = "stoneMat";
	stoneMat->MatCBIndex = 2;
	stoneMat->DiffuseSrvHeapIndex = 2;
	stoneMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	stoneMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	stoneMat->Roughness = 0.3f;

	std::unique_ptr<Material> tileMat = std::make_unique<Material>();
	tileMat->Name = "tileMat";
	tileMat->MatCBIndex = 3;
	tileMat->DiffuseSrvHeapIndex = 3;
	tileMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	tileMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	tileMat->Roughness = 0.3f;

	std::unique_ptr<Material> whiteMat = std::make_unique<Material>();
	whiteMat->Name = "whiteMat";
	whiteMat->MatCBIndex = 4;
	whiteMat->DiffuseSrvHeapIndex = 4;
	whiteMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	whiteMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	whiteMat->Roughness = 0.3f;

	m_Materials["skullMat"] = std::move(skullMat);
	m_Materials["brickMat"] = std::move(brickMat);
	m_Materials["stoneMat"] = std::move(stoneMat);
	m_Materials["tileMat"] = std::move(tileMat);
	m_Materials["whiteMat"] = std::move(whiteMat);
}

void FPSCameraApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Geo = m_Geometries["shapeGeo"].get();
	boxRitem->MatIndex = m_Materials["stoneMat"].get()->MatCBIndex;
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	m_AllRenderItems.push_back(std::move(boxRitem));

	// 그리드 아이템
	std::unique_ptr<RenderItem> gridRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->WorldMat = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 2;
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->MatIndex = m_Materials["tileMat"].get()->MatCBIndex;
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
	m_AllRenderItems.push_back(std::move(gridRitem));

	// 원기둥과 구 5개씩 2줄
	UINT objCBIndex = 3;
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
		leftCylRitem->MatIndex = m_Materials["brickMat"].get()->MatCBIndex;
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->WorldMat, leftCylWorld);
		XMStoreFloat4x4(&rightCylRitem->TexTransform, XMMatrixIdentity());
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = m_Geometries["shapeGeo"].get();
		rightCylRitem->MatIndex = m_Materials["brickMat"].get()->MatCBIndex;
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->WorldMat, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		leftSphereRitem->MatIndex = m_Materials["stoneMat"].get()->MatCBIndex;
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->WorldMat, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->MatIndex = m_Materials["stoneMat"].get()->MatCBIndex;
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
	// skullRenderItem->WorldMat = MathHelper::Identity4x4();
	XMStoreFloat4x4(&skullRenderItem->WorldMat, SkullWorldMat);
	skullRenderItem->ObjCBIndex = 0;
	skullRenderItem->Geo = m_Geometries["SkullGeo"].get();
	skullRenderItem->MatIndex = m_Materials["whiteMat"].get()->MatCBIndex;
	skullRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->IndexCount = skullRenderItem->Geo->DrawArgs["skull"].IndexCount;
	skullRenderItem->StartIndexLocation = skullRenderItem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRenderItem->BaseVertexLocation = skullRenderItem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());

	m_AllRenderItems.push_back(std::move(skullRenderItem));
}

void FPSCameraApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
{
	// 이제 Material도 물체마다 업데이트 해줘야 한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < _renderItems.size(); i++)
	{
		RenderItem* ri = _renderItems[i];
		
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

void FPSCameraApp::DrawAllSubRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
{
	// 이제 Material도 물체마다 업데이트 해줘야 한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();

	for (size_t i = 0; i < _renderItems.size(); i++)
	{
		RenderItem* ri = _renderItems[i];

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

		MeshGeometry* meshGeo = ri->Geo;
		std::unordered_map<std::string, SubmeshGeometry>::iterator iter = meshGeo->DrawArgs.begin();
		for (; iter != meshGeo->DrawArgs.end(); iter++)
		{
			// 이제 Item은 Object Buffer만 넘겨주어도 된다
			_cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
			_cmdList->DrawIndexedInstanced(iter->second.IndexCount, 1, iter->second.StartIndexLocation, iter->second.BaseVertexLocation, 0);
		}
	}
}

float FPSCameraApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 FPSCameraApp::GetHillsNormal(float _x, float _z) const
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

void FPSCameraApp::LoadTextures_Prac3()
{
	std::unique_ptr<Texture> boxTex0 = std::make_unique<Texture>();
	boxTex0->Name = "boxTex0";
	boxTex0->Filename = L"../Textures/ice.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		boxTex0->Filename.c_str(),
		boxTex0->Resource,
		boxTex0->UploadHeap
	));

	m_Textures[boxTex0->Name] = std::move(boxTex0);

	std::unique_ptr<Texture> boxTex1 = std::make_unique<Texture>();
	boxTex1->Name = "boxTex1";
	boxTex1->Filename = L"../Textures/bricks2.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		boxTex1->Filename.c_str(),
		boxTex1->Resource,
		boxTex1->UploadHeap
	));

	m_Textures[boxTex1->Name] = std::move(boxTex1);

	std::unique_ptr<Texture> boxTex2 = std::make_unique<Texture>();
	boxTex2->Name = "boxTex2";
	boxTex2->Filename = L"../Textures/bricks3.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		boxTex2->Filename.c_str(),
		boxTex2->Resource,
		boxTex2->UploadHeap
	));

	m_Textures[boxTex2->Name] = std::move(boxTex2);

	std::unique_ptr<Texture> boxTex3 = std::make_unique<Texture>();
	boxTex3->Name = "boxTex3";
	boxTex3->Filename = L"../Textures/checkboard.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		boxTex3->Filename.c_str(),
		boxTex3->Resource,
		boxTex3->UploadHeap
	));

	m_Textures[boxTex3->Name] = std::move(boxTex3);

	// 텍스쳐를 사용하지 않을 때 쓰는 친구다.
	std::unique_ptr<Texture> boxTex4 = std::make_unique<Texture>();
	boxTex4->Name = "boxTex4";
	boxTex4->Filename = L"../Textures/flare.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		boxTex4->Filename.c_str(),
		boxTex4->Resource,
		boxTex4->UploadHeap
	));
	m_Textures[boxTex4->Name] = std::move(boxTex4);
}

void FPSCameraApp::BuildDescriptorHeaps_Prac3()
{
	const int textureDescriptorCount = 5;

	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = textureDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CbvSrvUavDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* boxTex0 = m_Textures["boxTex0"].get()->Resource.Get();
	ID3D12Resource* boxTex1 = m_Textures["boxTex1"].get()->Resource.Get();
	ID3D12Resource* boxTex2 = m_Textures["boxTex2"].get()->Resource.Get();
	ID3D12Resource* boxTex3 = m_Textures["boxTex3"].get()->Resource.Get();
	ID3D12Resource* boxTex4 = m_Textures["boxTex4"].get()->Resource.Get();

	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = boxTex0->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = -1;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(boxTex0, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = boxTex1->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = boxTex1->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(boxTex1, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = boxTex2->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = boxTex2->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(boxTex2, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = boxTex3->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = boxTex3->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(boxTex3, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = boxTex4->GetDesc().Format;
	srcDesc.Texture2D.MipLevels = boxTex4->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(boxTex4, &srcDesc, viewHandle);
}

void FPSCameraApp::BuildShadersAndInputLayout_Prac3()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\Prac3.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\Prac3.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Prac3.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "INDEX", 0, DXGI_FORMAT_R32_UINT, 0, sizeof(XMFLOAT3) * 2 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void FPSCameraApp::BuildBoxesGeometry_Prac3()
{
	struct Vertex_Prac3 
	{
		XMFLOAT3 Pos;
		XMFLOAT3 Normal;
		XMFLOAT2 TexC;
		UINT Index;
		Vertex_Prac3() = default;
		Vertex_Prac3(const GeometryGenerator::Vertex& _vert, UINT _index)
			:Pos(_vert.Position), Normal(_vert.Normal), TexC(_vert.TexC), Index(_index)
		{ }
	};

	GeometryGenerator geoGenerator;
	GeometryGenerator::MeshData box = geoGenerator.CreateBox(1.f, 1.f, 1.f, 2);

	UINT boxVertCount = (UINT)box.Vertices.size();
	UINT boxIndexCount = (UINT)box.Indices32.size();
	
	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

	std::vector<Vertex_Prac3> vertices;
	vertices.reserve(boxVertCount * 5);

	std::vector<uint32_t> indices;
	indices.reserve(boxIndexCount * 5);

	for (UINT b = 0; b < 5; b++)
	{
		for (UINT i = 0; i < boxVertCount; i++)
		{
			vertices.push_back(Vertex_Prac3(box.Vertices[i], b));
		}
		indices.insert(indices.end(), std::begin(box.Indices32), std::end(box.Indices32));

		SubmeshGeometry boxSubmesh;
		boxSubmesh.IndexCount = boxIndexCount;
		boxSubmesh.StartIndexLocation = boxIndexCount * b;
		boxSubmesh.BaseVertexLocation = boxVertCount * b;

		std::string boxArgs = "box" + to_string(b);
		geo->DrawArgs[boxArgs] = boxSubmesh;
	}
	const UINT vbByteSize = boxVertCount * sizeof(Vertex_Prac3) * 5;
	const UINT ibByteSize = boxIndexCount * sizeof(std::uint32_t) * 5;

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	// Vertex 와 Index 정보를 Default Buffer로 만들고
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

	geo->VertexByteStride = sizeof(Vertex_Prac3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	m_Geometries["boxGeo"] = std::move(geo);
}

void FPSCameraApp::BuildMaterials_Prac3()
{
	std::unique_ptr<Material> box0Mat = std::make_unique<Material>();
	box0Mat->Name = "box0Mat";
	box0Mat->MatCBIndex = 0;
	box0Mat->DiffuseSrvHeapIndex = 0;
	box0Mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box0Mat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	box0Mat->Roughness = 0.125f;

	std::unique_ptr<Material> box1Mat = std::make_unique<Material>();
	box1Mat->Name = "box1Mat";
	box1Mat->MatCBIndex = 1;
	box1Mat->DiffuseSrvHeapIndex = 1;
	box1Mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box1Mat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	box1Mat->Roughness = 0.3f;

	std::unique_ptr<Material> box2Mat = std::make_unique<Material>();
	box2Mat->Name = "box2Mat";
	box2Mat->MatCBIndex = 2;
	box2Mat->DiffuseSrvHeapIndex = 2;
	box2Mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box2Mat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	box2Mat->Roughness = 0.3f;

	std::unique_ptr<Material> box3Mat = std::make_unique<Material>();
	box3Mat->Name = "box3Mat";
	box3Mat->MatCBIndex = 3;
	box3Mat->DiffuseSrvHeapIndex = 3;
	box3Mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box3Mat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	box3Mat->Roughness = 0.3f;

	std::unique_ptr<Material> box4Mat = std::make_unique<Material>();
	box4Mat->Name = "box4Mat";
	box4Mat->MatCBIndex = 4;
	box4Mat->DiffuseSrvHeapIndex = 4;
	box4Mat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	box4Mat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	box4Mat->Roughness = 0.3f;

	m_Materials["box0Mat"] = std::move(box0Mat);
	m_Materials["box1Mat"] = std::move(box1Mat);
	m_Materials["box2Mat"] = std::move(box2Mat);
	m_Materials["box3Mat"] = std::move(box3Mat);
	m_Materials["box4Mat"] = std::move(box4Mat);
}

void FPSCameraApp::BuildRenderItems_Prac3()
{
	for (UINT i = 0; i < 5; i++)
	{
		std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

		XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixTranslation(2.f, 0.f, 0.f) * XMMatrixRotationY((XM_2PI  * i) / 5));
		XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		boxRitem->ObjCBIndex = i;
		boxRitem->Geo = m_Geometries["boxGeo"].get();
		boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		string subGeoArgs = "box" + to_string(i);
		boxRitem->IndexCount = boxRitem->Geo->DrawArgs[subGeoArgs].IndexCount;
		boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs[subGeoArgs].StartIndexLocation;
		boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs[subGeoArgs].BaseVertexLocation;

		m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
		m_AllRenderItems.push_back(std::move(boxRitem));
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> FPSCameraApp::GetStaticSamplers()
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
		3, // s3 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER);	// addressW
	pointBorder.BorderColor = D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	// 주변선형 보간 필터링 - 반복
	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		4, // s2 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	// 주변선형 보간 필터링 - 자르고 늘이기
	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		5, // s3 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW
	// 주변선형 보간 필터링 - 미러
	const CD3DX12_STATIC_SAMPLER_DESC linearMirror(
		6, // s3 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_MIRROR); // addressW
	// 주변선형 보간 필터링 - 색채우기
	CD3DX12_STATIC_SAMPLER_DESC linearBorder(
		7, // s3 - 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER); // addressW
	linearBorder.BorderColor = D3D12_STATIC_BORDER_COLOR::D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		8, // s4 - 레지스터 번호
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                              // LOD를 계산하는 offset(bias) 값이라고 한다.
		8);                                // 최대 비등방 값 (높을 수록 비싸고, 예뻐진다.)

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		9, // s5 - 레지스터 번호
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // LOD를 계산하는 offset(bias) 값이라고 한다.
		8);                                // 최대 비등방 값 (높을 수록 비싸고, 예뻐진다.)

	return {
		pointWrap, pointClamp, pointMirror, pointBorder,
		linearWrap, linearClamp, linearMirror, linearBorder,
		anisotropicWrap, anisotropicClamp };
}