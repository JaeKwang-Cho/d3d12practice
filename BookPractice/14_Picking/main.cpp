//***************************************************************************************
// PickingApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>
#include "FileReader.h"
#include "../Common/Camera.h"

#define PRAC1 (0)

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
#if PRAC1
	BoundingSphere Bounds;
#else
	BoundingBox Bounds;
#endif
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
	Highlight,
	Count
};

class PickingApp : public D3DApp
{
public:
	PickingApp(HINSTANCE hInstance);
	~PickingApp();

	PickingApp(const PickingApp& _other) = delete;
	PickingApp& operator=(const PickingApp& _other) = delete;

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
	void BuildCarGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildPSOs();
	void BuildFrameResources();

	// ==== Func ====
	void Pick(int _sx, int _sy);

	// ==== Render ====
	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY  _Type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);

	// 지형의 높이와 노멀을 계산하는 함수다.
	float GetHillsHeight(float _x, float _z) const;
	XMFLOAT3 GetHillsNormal(float _x, float _z)const;

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

	// 편하게 하도록 App에서 관리한다.
	RenderItem* m_PickedRenderitem = nullptr;

	// 카메라 안에 들어오는 Bounding Frustum이다.
	BoundingFrustum m_CamFrustum;
	bool m_FrustumCullingEnabled = true;

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
		PickingApp theApp(hInstance);
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

PickingApp::PickingApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

PickingApp::~PickingApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool PickingApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_Camera.LookAt(
		XMFLOAT3(5.0f, 4.0f, -15.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f),
		XMFLOAT3(0.0f, 1.0f, 0.0f));

	// ======== 초기화 ==========
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildCarGeometry();
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

void PickingApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetFrustum(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
}

void PickingApp::Update(const GameTimer& _gt)
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
	UpdateObjectCBs(_gt);
	UpdateMaterialCBs(_gt);
	UpdateMainPassCB(_gt);
}

void PickingApp::Draw(const GameTimer& _gt)
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

	// Structured Buffer는 Heap 없이 그냥 Rood Descriptor로 넘겨줄 수 있다.
	// 현재 프레임에 사용하는 Material Buffer를 Srv로 바인드 해준다.
	ID3D12Resource* matStructredBuffer = m_CurrFrameResource->MaterialBuffer->Resource();
	m_CommandList->SetGraphicsRootShaderResourceView(2, matStructredBuffer->GetGPUVirtualAddress()); // Mat는 2번

	// 택스쳐가 올라가 있는 view heap에서 텍스쳐 첫번째 핸들을 설정해준다.
	// (첫번째 것만 설정해서 넘겨주면 된다. Root Signature에 이미 정보가 다 있다.)
	m_CommandList->SetGraphicsRootDescriptorTable(3, m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart()); // Tex arr는 3번

	// 일단 불투명한 애들을 먼저 싹 출력을 해준다.
	// 그래야 alpha 계산이 가능하다.
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 하이라이트를 출력해준다.
	m_CommandList->SetPipelineState(m_PSOs["highlight"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Highlight]);

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

void PickingApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	if ((_btnState & MK_LBUTTON) != 0)
	{
		// 마우스의 위치를 기억하고
		m_LastMousePos.x = _x;
		m_LastMousePos.y = _y;
		// 마우스를 붙잡는다.
		SetCapture(m_hMainWnd);
	}
	else if ((_btnState & MK_RBUTTON) != 0)
	{
		Pick(_x, _y);
	}
}

void PickingApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void PickingApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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

void PickingApp::OnKeyboardInput(const GameTimer _gt)
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

void PickingApp::UpdateObjectCBs(const GameTimer& _gt)
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

void PickingApp::UpdateMaterialCBs(const GameTimer& _gt)
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

			currMaterialCB->CopyData(mat->MatCBIndex, matData);

			mat->NumFramesDirty--;
		}
	}
}

void PickingApp::UpdateMainPassCB(const GameTimer& _gt)
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

void PickingApp::AnimateMaterials(const GameTimer& _gt)
{
}


void PickingApp::LoadTextures()
{
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


void PickingApp::BuildRootSignature()
{	
	// Table도 쓸거고, Srv쓸거고 Cbv도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	UINT numOfParameter = 4;

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable;
	// Texture2D를 Index로 접근할 것 이기 때문에 이렇게 5개로 만들어 준다.
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0); // t0 - texture 정보

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

void PickingApp::BuildDescriptorHeaps()
{
	const int textureDescriptorCount = 1;

	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = textureDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CbvSrvUavDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* whiteTex = m_Textures["white1x1Tex"].get()->Resource.Get();

	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = whiteTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = -1;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(whiteTex, &srcDesc, viewHandle);
}

void PickingApp::BuildShadersAndInputLayout()
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

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\14_Picking.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\14_Picking.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\14_Picking.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void PickingApp::BuildCarGeometry()
{
	vector<Vertex> CarVertices;
	vector<uint32_t> CarIndices;

#if PRAC1
	BoundingSphere bounds;
#else
	BoundingBox bounds;
#endif
	Dorasima::Prac3VerticesNIndicies(L"..\\14_Picking\\Models\\car.txt", CarVertices, CarIndices, bounds);

	const UINT vbByteSize = (UINT)CarVertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)CarIndices.size() * sizeof(std::uint32_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "CarGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), CarVertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), CarIndices.data(), ibByteSize);

	// Vertex 와 Index 정보를 Default Buffer로 만들고
	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		CarVertices.data(),
		vbByteSize,
		geo->VertexBufferUploader
	);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		CarIndices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R32_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	// 서브 메쉬 설정을 해준다.
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)CarIndices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

#if PRAC1
	submesh.BoundSphere = bounds;
#else
	// 구해놓은 바운딩박스도 설정해준다.
	submesh.BoundBox = bounds;
#endif

	geo->DrawArgs["car"] = submesh;

	m_Geometries["CarGeo"] = std::move(geo);
}

void PickingApp::BuildPSOs()
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

	// 하이라이트 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC highlighPSODesc = opaquePSODesc;
	// 선택된 삼각형 위에 노란삼각형을 하나 더 그리는 것이다.
	highlighPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // 이렇게 해야 잘 보인다.
	// 전형적인 Transparency Blending을 사용한다. (이미 위에 있다.)
	highlighPSODesc.BlendState.RenderTarget[0] = transparencyBlendDesc;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&highlighPSODesc, IID_PPV_ARGS(&m_PSOs["highlight"])));
}

void PickingApp::BuildFrameResources()
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

void PickingApp::Pick(int _sx, int _sy)
{
	XMFLOAT4X4 p = m_Camera.GetProj4x4f();
	// 카메라에 있는 프로젝션 매트릭스를 위해서,
	// Projection Plane의 (0, 0)에서 view frustum으로 뻣어나가는 Ray를 구한다.

	// 교재 공식을 사용한다.
	float vx = (2.f * _sx / m_ClientWidth - 1.f) / p(0, 0);
	float vy = (-2.f * _sy / m_ClientHeight + 1.f) / p(1, 1);

	// Ray 벡터를 정의한다.
	XMVECTOR rayOrigin = XMVectorSet(0.f, 0.f, 0.f, 1.f); // 점이니까 w 성분이 1
	XMVECTOR rayDirection = XMVectorSet(vx, vy, 1.f, 0.f); // 방향이니까 w 성분이 0

	// model을 옮기는 것보다 Ray를 view 공간으로 변환해서 비교하는게 더 효율적이다.
	XMMATRIX v = m_Camera.GetViewMat();
	XMVECTOR vDet = XMMatrixDeterminant(v);
	XMMATRIX invV = XMMatrixInverse(&vDet, v);

	// 일단 초기화
	m_PickedRenderitem->Visible = false;
	// 일단 여기선 '선택가능한 리스트'가 아니라 불투명한 애들로 루프를 돈다.
	for (RenderItem* ri : m_RenderItemLayer[(int)RenderLayer::Opaque])
	{
		if (ri->Visible == false)
		{
			continue;
		}

		MeshGeometry* geo = ri->Geo;
		// 물체의 월드 위치도 역으로 Ray가 움직여서 계산한다.
		XMMATRIX w = XMLoadFloat4x4(&ri->WorldMat);
		XMVECTOR detW = XMMatrixDeterminant(w);
		XMMATRIX invW = XMMatrixInverse(&detW, w);

		XMMATRIX toLocalMat = XMMatrixMultiply(invV, invW);

		// 점으로서 행렬변환하고
		rayOrigin = XMVector3TransformCoord(rayOrigin, toLocalMat);
		// 벡터로서 행렬변환한다.
		rayDirection = XMVector3TransformNormal(rayDirection, toLocalMat);
		rayDirection = XMVector3Normalize(rayDirection);

		// 모델이 선택이 되었으면, 해당 모델에 모든 삼각형에 대해 
		// Ray intersection 테스트를 하고 그 중에서 가장 가까운 삼각형을 선택한다.
		float tmin = 0.f;
		if (ri->Bounds.Intersects(rayOrigin, rayDirection, tmin))
		{
			// 이것도 만약 geometry 마다 구성이 다르다면,
			// intersection을 테스트하는 또다른 메타데이터를 정의해야할 것이다.
			Vertex* vertices = (Vertex*)geo->VertexBufferCPU->GetBufferPointer();
			std::uint32_t* indices = (std::uint32_t*)geo->IndexBufferCPU->GetBufferPointer();
			UINT triCount = ri->IndexCount / 3;

			// 가장 가까이서 겹치는 삼각형을 찾는다.
			tmin = MathHelper::Infinity;
			for (UINT i = 0; i < triCount; i++)
			{
				// 모든 삼각형을 돌면서
				UINT i0 = indices[i * 3 + 0];
				UINT i1 = indices[i * 3 + 1];
				UINT i2 = indices[i * 3 + 2];

				XMVECTOR v0 = XMLoadFloat3(&vertices[i0].Pos);
				XMVECTOR v1 = XMLoadFloat3(&vertices[i1].Pos);
				XMVECTOR v2 = XMLoadFloat3(&vertices[i2].Pos);

				// 출력용 t이다. 거리를 받아온다.
				float t = 0.f; 
				if (DirectX::TriangleTests::Intersects(rayOrigin, rayDirection, v0, v1, v2, t))
				{
					if (t < tmin)
					{
						tmin = t;

						m_PickedRenderitem->Visible = true;
						m_PickedRenderitem->IndexCount = 3;
						m_PickedRenderitem->StartIndexLocation = 3 * i;
						m_PickedRenderitem->BaseVertexLocation = 0;
						
						// 모델과 똑같은 World를 넘겨준다.
						m_PickedRenderitem->WorldMat = ri->WorldMat;
						m_PickedRenderitem->NumFrameDirty = g_NumFrameResources;
					}
				}
			}
		}
	}

}

void PickingApp::BuildMaterials()
{
	std::unique_ptr<Material> whiteMat = std::make_unique<Material>();
	whiteMat->Name = "grayMat";
	whiteMat->MatCBIndex = 0;
	whiteMat->DiffuseSrvHeapIndex = 0;
	whiteMat->DiffuseAlbedo = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.f);
	whiteMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	whiteMat->Roughness = 0.3f;

	m_Materials["grayMat"] = std::move(whiteMat);

	std::unique_ptr<Material> highlightMat = std::make_unique<Material>();
	highlightMat->Name = "highlightMat";
	highlightMat->MatCBIndex = 1;
	highlightMat->DiffuseSrvHeapIndex = 0;
	highlightMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.6f);
	highlightMat->FresnelR0 = XMFLOAT3(0.06f, 0.06f, 0.06f);
	highlightMat->Roughness = 0.0f;

	m_Materials["highlightMat"] = std::move(highlightMat);
}

void PickingApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> carRenderItem = std::make_unique<RenderItem>();
	carRenderItem->WorldMat = MathHelper::Identity4x4();
	carRenderItem->TexTransform = MathHelper::Identity4x4();
	carRenderItem->ObjCBIndex = 0;
	carRenderItem->Geo = m_Geometries["CarGeo"].get();
	carRenderItem->Mat = m_Materials["grayMat"].get();
	carRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	carRenderItem->IndexCount = carRenderItem->Geo->DrawArgs["car"].IndexCount;
	carRenderItem->StartIndexLocation = carRenderItem->Geo->DrawArgs["car"].StartIndexLocation;
	carRenderItem->BaseVertexLocation = carRenderItem->Geo->DrawArgs["car"].BaseVertexLocation;
#if PRAC1
	carRenderItem->Bounds = carRenderItem->Geo->DrawArgs["car"].BoundSphere;
#else
	carRenderItem->Bounds = carRenderItem->Geo->DrawArgs["car"].BoundBox;
#endif

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(carRenderItem.get());
	m_AllRenderItems.push_back(std::move(carRenderItem));

	std::unique_ptr<RenderItem> pickedRenderItem = std::make_unique<RenderItem>();
	pickedRenderItem->WorldMat = MathHelper::Identity4x4();
	pickedRenderItem->TexTransform = MathHelper::Identity4x4();
	pickedRenderItem->ObjCBIndex = 1;
	pickedRenderItem->Geo = m_Geometries["CarGeo"].get();
	pickedRenderItem->Mat = m_Materials["highlightMat"].get();
	pickedRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// 선택되기 전 까지 세팅하지 않는다.
	pickedRenderItem->Visible = false;
	pickedRenderItem->IndexCount = 0;
	pickedRenderItem->StartIndexLocation = 0;
	pickedRenderItem->BaseVertexLocation = 0;
	m_PickedRenderitem = pickedRenderItem.get();

	m_RenderItemLayer[(int)RenderLayer::Highlight].push_back(pickedRenderItem.get());
	m_AllRenderItems.push_back(std::move(pickedRenderItem));
}

void PickingApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
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

float PickingApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 PickingApp::GetHillsNormal(float _x, float _z) const
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> PickingApp::GetStaticSamplers()
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
