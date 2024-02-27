//***************************************************************************************
// NormalMappingApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>
#include "FileReader.h"
#include "../Common/Camera.h"
#include "CubeRenderTarget.h"

const int g_NumFrameResources = 3;
const UINT CubeMapSize = 512;

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
	OpaqueDynamicReflectors,
	Sky, 
	Count
};

class NormalMappingApp : public D3DApp
{
public:
	NormalMappingApp(HINSTANCE hInstance);
	~NormalMappingApp();

	NormalMappingApp(const NormalMappingApp& _other) = delete;
	NormalMappingApp& operator=(const NormalMappingApp& _other) = delete;

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

	// ==== Dynamic Cube ====
	virtual void CreateRtvAndDsvDescriptorHeaps() override;
	void UpdateCubeMapFacePassCB(const GameTimer& _gt);
	void BuildCubeDepthStencil();
	void DrawSceneToCubeMap();
	void BuildCubeFaceCamera(float _x, float _y, float _z);

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

	POINT m_LastMousePos = {};

	// ==== Dynamic Cube ====
	ComPtr<ID3D12Resource> m_CubeDepthStencilBuffer;

	UINT m_DynamicTexHeapIndex = 0;

	RenderItem* m_SkullRitem = nullptr;

	std::unique_ptr<CubeRenderTarget> m_DynamicCubeMap = nullptr;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CubeDsvHandle;

	// Camera
	Camera m_Camera;
	Camera m_CubeMapCamera[6];

	// Sky Texture가 위치한 view Handle Offset을 저장해 놓는다.
	UINT m_SkyTexHeapIndex = 0;

	float m_Phi = 1.24f * XM_PI;
	float m_Theta = 0.42f * XM_PI;
	float m_Radius = 20.f;
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
		NormalMappingApp theApp(hInstance);
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

NormalMappingApp::NormalMappingApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

NormalMappingApp::~NormalMappingApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool NormalMappingApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	m_Camera.SetPosition(0.0f, 2.0f, -15.0f);

#if DYNAMIC_CUBE
	BuildCubeFaceCamera(0.f, 2.f, 0.f);
	m_DynamicCubeMap = std::make_unique<CubeRenderTarget>(
		m_d3dDevice.Get(),
		CubeMapSize,
		CubeMapSize,
		DXGI_FORMAT_R8G8B8A8_UNORM
	);
#endif

	// ======== 초기화 ==========
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
#if DYNAMIC_CUBE
	BuildCubeDepthStencil();
#endif
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

void NormalMappingApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetFrustum(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
}

void NormalMappingApp::Update(const GameTimer& _gt)
{
	// 더 기능이 많아질테니, 함수로 쪼개서 넣는다.
	OnKeyboardInput(_gt);

#if DYNAMIC_CUBE
	XMMATRIX skullScale = XMMatrixScaling(0.2f, 0.2f, 0.2f);
	XMMATRIX skullOffset = XMMatrixTranslation(3.0f, 2.0f, 0.0f);
	XMMATRIX skullLocalRotate = XMMatrixRotationY(2.0f * _gt.GetTotalTime());
	XMMATRIX skullGlobalRotate = XMMatrixRotationY(0.5f * _gt.GetTotalTime());
	XMStoreFloat4x4(&m_SkullRitem->WorldMat, skullScale * skullLocalRotate * skullOffset * skullGlobalRotate);
	// 이걸 위에서 app에서 가지고 있었던 것이다.
	m_SkullRitem->NumFrameDirty = g_NumFrameResources;
#endif

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
#if DYNAMIC_CUBE
	UpdateCubeMapFacePassCB(_gt);
#endif
}

void NormalMappingApp::Draw(const GameTimer& _gt)
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

#if DYNAMIC_CUBE 
	// ==== 큐브맵 그리기 ====
	// 다이나믹 큐브맵을 사용할 친구는 하나니깐, Per Frame에서 CubeMap을 생성한다.
	// 만약 여러 물체가 사용하게 하려면 Per Object를 사용해야 할 것이다.
	m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor); // TextureCube는 3번
	
	// 함수로 묶어놨다.
	// 각 방향마다 카메라 설정하고, 물체를 그리고
	// 그 결과를 헬퍼 클래스가 관리하는 텍스쳐에 넣어놓는다.
	DrawSceneToCubeMap();
#endif
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

	// 현재 Frame Constant Buffer를 업데이트한다.
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(1, passCB->GetGPUVirtualAddress()); // Pass는 1번

#if DYNAMIC_CUBE
	// Dynamic Cube를 TextureCube에 바인드 해준다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE dynamicTexDescriptor(m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	dynamicTexDescriptor.Offset(m_SkyTexHeapIndex + 1, m_CbvSrvUavDescriptorSize);
	m_CommandList->SetGraphicsRootDescriptorTable(3, dynamicTexDescriptor);

	// 그리고 거울 공을 그린다.
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::OpaqueDynamicReflectors]);
#endif
	// SkyMap을 그릴 TextureCube를 바인드해준다.
	m_CommandList->SetGraphicsRootDescriptorTable(3, skyTexDescriptor);

	// drawcall을 걸어준다.
	// 일단 불투명한 애들을 먼저 싹 출력을 해준다.
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 하늘을 맨 마지막에 출력해주는 것이 depth test 성능상 좋다고 한다. 
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

void NormalMappingApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
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

void NormalMappingApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void NormalMappingApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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

void NormalMappingApp::OnKeyboardInput(const GameTimer _gt)
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

void NormalMappingApp::UpdateObjectCBs(const GameTimer& _gt)
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

void NormalMappingApp::UpdateMaterialCBs(const GameTimer& _gt)
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

void NormalMappingApp::UpdateMainPassCB(const GameTimer& _gt)
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

void NormalMappingApp::AnimateMaterials(const GameTimer& _gt)
{
}


void NormalMappingApp::LoadTextures()
{
	// 진작에 이렇게 할걸
	std::vector<std::string> texNames =
	{
		"bricksDiffuseMap",
		"tileDiffuseMap",
		"defaultDiffuseMap",
		"skyCubeMap"
	};

	std::vector<std::wstring> texFilenames =
	{
		L"../Textures/bricks2.dds",
		L"../Textures/tile.dds",
		L"../Textures/white1x1.dds",
		L"../Textures/grasscube1024.dds"
		//L"../Textures/skymap.dds"
		// image source: https://www.cleanpng.com/png-skybox-texture-mapping-cube-mapping-desktop-wallpa-6020000/
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


void NormalMappingApp::BuildRootSignature()
{	
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	UINT numOfParameter = 5;

	// TextureCube가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0);

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	// Texture2D를 Index로 접근할 것 이기 때문에 이렇게 5개로 만들어 준다.
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 5, 1, 0); // space0 / t1 - texture 정보

	// Srv가 넘어가는 테이블, Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[1].InitAsConstantBufferView(1); // b1 - PassConstants
	slotRootParameter[2].InitAsShaderResourceView(0, 1); // t0 - Material Structred Buffer, Space도 1로 지정해준다.
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t0 - TextureCube
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t1 - Texture2D Array

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

void NormalMappingApp::BuildDescriptorHeaps()
{
#if DYNAMIC_CUBE
	const int textureDescriptorCount = 6;
#else
	const int textureDescriptorCount = 4;
#endif

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
	ID3D12Resource* bricksTex = m_Textures["bricksDiffuseMap"].get()->Resource.Get();
	ID3D12Resource* tileTex = m_Textures["tileDiffuseMap"].get()->Resource.Get();
	ID3D12Resource* whiteTex = m_Textures["defaultDiffuseMap"].get()->Resource.Get();
	ID3D12Resource* skyTex = m_Textures["skyCubeMap"].get()->Resource.Get();

	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = bricksTex->GetDesc().Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(bricksTex, &srvDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	viewCount++;
	srvDesc.Format = tileTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = tileTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(tileTex, &srvDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	viewCount++;
	srvDesc.Format = whiteTex->GetDesc().Format;
	srvDesc.Texture2D.MipLevels = whiteTex->GetDesc().MipLevels;
	m_d3dDevice->CreateShaderResourceView(whiteTex, &srvDesc, viewHandle);

	// TextureCube는 프로퍼티가 다르기때문에
	// 수정해주고 Resource View를 생성한다.
	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	viewCount++;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
	srvDesc.TextureCube.MipLevels = skyTex->GetDesc().MipLevels;
	srvDesc.TextureCube.ResourceMinLODClamp = 0.f;
	srvDesc.Format = skyTex->GetDesc().Format;

	m_d3dDevice->CreateShaderResourceView(skyTex, &srvDesc, viewHandle);
	m_SkyTexHeapIndex = viewCount;

#if DYNAMIC_CUBE
	m_DynamicTexHeapIndex = m_SkyTexHeapIndex + 1;

	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuStart = m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuStart = m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCpuStart = m_RtvHeap->GetCPUDescriptorHandleForHeapStart();

	// swap chain 다음에 6개가 추가로 달려있는 거에 대해
	// view를 생성해준다.
	int rtvOffset = SwapChainBufferCount;
	// 핸들을 배열로 만들어 놓고
	CD3DX12_CPU_DESCRIPTOR_HANDLE cubeRtvHandle[6];
	for (int i = 0; i < 6; i++)
	{
		cubeRtvHandle[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(rtvCpuStart, rtvOffset + i, m_RtvDescriptorSize);
	}
	// 함수로 한번에 6개 view를 만든다.
	m_DynamicCubeMap->BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(srvCpuStart, m_DynamicTexHeapIndex, m_CbvSrvUavDescriptorSize),
		CD3DX12_GPU_DESCRIPTOR_HANDLE(srvGpuStart, m_DynamicTexHeapIndex, m_CbvSrvUavDescriptorSize),
		cubeRtvHandle
	);
#endif
}

void NormalMappingApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\16_NormalMapping.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\16_NormalMapping.hlsl", nullptr, "PS", "ps_5_1");
	//m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\16_NormalMapping.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_Shaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void NormalMappingApp::BuildStageGeometry()
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

void NormalMappingApp::BuildSkullGeometry()
{
	vector<Vertex> SkullVertices;
	vector<uint32_t> SkullIndices;

	BoundingBox bounds;
	Dorasima::Prac3VerticesNIndicies(L"..\\04_RenderSmoothly_Wave\\Skull.txt", SkullVertices, SkullIndices, bounds);

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

void NormalMappingApp::BuildPSOs()
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

void NormalMappingApp::BuildFrameResources()
{
#if DYNAMIC_CUBE
	// 메인 1 + 다이나믹 큐브 생성 6
	UINT passCBCount = 7;
#else 
	UINT passCBCount = 1;
#endif

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

void NormalMappingApp::BuildMaterials()
{
	std::unique_ptr<Material> brickMat = std::make_unique<Material>();
	brickMat->Name = "brickMat";
	brickMat->MatCBIndex = 0;
	brickMat->DiffuseSrvHeapIndex = 0;
	brickMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	brickMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	brickMat->Roughness = 0.3f;

	std::unique_ptr<Material> tileMat = std::make_unique<Material>();
	tileMat->Name = "tileMat";
	tileMat->MatCBIndex = 1;
	tileMat->DiffuseSrvHeapIndex = 1;
	tileMat->DiffuseAlbedo = XMFLOAT4(0.9f, 0.9f, 0.9f, 1.0f);
	tileMat->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	tileMat->Roughness = 0.3f;

	std::unique_ptr<Material> mirrorMat = std::make_unique<Material>();
	mirrorMat->Name = "mirrorMat";
	mirrorMat->MatCBIndex = 2;
	mirrorMat->DiffuseSrvHeapIndex = 2;
	mirrorMat->DiffuseAlbedo = XMFLOAT4(0.0f, 0.0f, 0.1f, 1.0f);
	mirrorMat->FresnelR0 = XMFLOAT3(0.98f, 0.97f, 0.95f);
	mirrorMat->Roughness = 0.1f;

	std::unique_ptr<Material> whiteMat = std::make_unique<Material>();
	whiteMat->Name = "whiteMat";
	whiteMat->MatCBIndex = 3;
	whiteMat->DiffuseSrvHeapIndex = 2;
	whiteMat->DiffuseAlbedo = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	whiteMat->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	whiteMat->Roughness = 0.2f;

	std::unique_ptr<Material> skyMat = std::make_unique<Material>();
	skyMat->Name = "skyMat";
	skyMat->MatCBIndex = 4;
	skyMat->DiffuseSrvHeapIndex = 3;
	skyMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);;
	skyMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	skyMat->Roughness = 1.0f;

	m_Materials["brickMat"] = std::move(brickMat);
	m_Materials["mirrorMat"] = std::move(mirrorMat);
	m_Materials["tileMat"] = std::move(tileMat);
	m_Materials["whiteMat"] = std::move(whiteMat);
	m_Materials["skyMat"] = std::move(skyMat);
}

void NormalMappingApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> skyRitem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&skyRitem->WorldMat, XMMatrixScaling(5000.0f, 5000.0f, 5000.0f));
	skyRitem->TexTransform = MathHelper::Identity4x4();
	skyRitem->ObjCBIndex = 0;
	skyRitem->Mat = m_Materials["skyMat"].get();
	skyRitem->Geo = m_Geometries["shapeGeo"].get();
	skyRitem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skyRitem->IndexCount = skyRitem->Geo->DrawArgs["sphere"].IndexCount;
	skyRitem->StartIndexLocation = skyRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
	skyRitem->BaseVertexLocation = skyRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Sky].push_back(skyRitem.get());
	m_AllRenderItems.push_back(std::move(skyRitem));

	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = 1;
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
	XMStoreFloat4x4(&gridRitem->TexTransform, XMMatrixScaling(8.0f, 8.0f, 1.0f));
	gridRitem->WorldMat = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 2;
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->Mat = m_Materials["tileMat"].get();
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
#if DYNAMIC_CUBE
	skullRenderItem->WorldMat = MathHelper::Identity4x4();
#else
	XMMATRIX ScaleHalfMat = XMMatrixScaling(0.5f, 0.5f, 0.5f);
	XMMATRIX TranslateDown5Units = XMMatrixTranslation(0.f, 1.f, 0.f);
	XMMATRIX SkullWorldMat = ScaleHalfMat * TranslateDown5Units;
	XMStoreFloat4x4(&skullRenderItem->WorldMat, SkullWorldMat);
#endif
	skullRenderItem->TexTransform = MathHelper::Identity4x4();
	skullRenderItem->ObjCBIndex = objCBIndex++;
	skullRenderItem->Geo = m_Geometries["SkullGeo"].get();
	skullRenderItem->Mat = m_Materials["whiteMat"].get();
	skullRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->IndexCount = skullRenderItem->Geo->DrawArgs["skull"].IndexCount;
	skullRenderItem->StartIndexLocation = skullRenderItem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRenderItem->BaseVertexLocation = skullRenderItem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_SkullRitem = skullRenderItem.get();

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());

	m_AllRenderItems.push_back(std::move(skullRenderItem));

#if DYNAMIC_CUBE
	// Dynamic Cube를 이용하는 미러볼을 생성한다.
	std::unique_ptr<RenderItem>  globeRenderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&globeRenderItem->WorldMat, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 2.0f, 0.0f));
	XMStoreFloat4x4(&globeRenderItem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	globeRenderItem->ObjCBIndex = objCBIndex++;
	globeRenderItem->Mat = m_Materials["mirrorMat"].get();
	globeRenderItem->Geo = m_Geometries["shapeGeo"].get();
	globeRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	globeRenderItem->IndexCount = globeRenderItem->Geo->DrawArgs["sphere"].IndexCount;
	globeRenderItem->StartIndexLocation = globeRenderItem->Geo->DrawArgs["sphere"].StartIndexLocation;
	globeRenderItem->BaseVertexLocation = globeRenderItem->Geo->DrawArgs["sphere"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::OpaqueDynamicReflectors].push_back(globeRenderItem.get());
	m_AllRenderItems.push_back(std::move(globeRenderItem));
#endif
}

void NormalMappingApp::CreateRtvAndDsvDescriptorHeaps()
{
#if DYNAMIC_CUBE
	// 랜더 타겟을 6개 더 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc;
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 6;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc, IID_PPV_ARGS(m_RtvHeap.GetAddressOf())
	));

	// DepthStencil도 1개 더 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc;
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())
	));
	// app에서 미리 핸들을 가지고 있는다.
	m_CubeDsvHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(
		m_DsvHeap->GetCPUDescriptorHandleForHeapStart(),
		1,
		m_DsvDescriptorSize
	);
#else
	__super::CreateRtvAndDsvDescriptorHeaps();
#endif
}

void NormalMappingApp::UpdateCubeMapFacePassCB(const GameTimer& _gt)
{
	for (int i = 0; i < 6; ++i)
	{
		// 각 텍스쳐마다 카메라 방향이 다르기 때문에
		// pass도 따로 넘겨줘야 한다.
		PassConstants cubeFacePassCB = m_MainPassCB;

		XMMATRIX ViewMat = m_CubeMapCamera[i].GetViewMat();
		XMMATRIX ProjMat = m_CubeMapCamera[i].GetProjMat();

		XMMATRIX VPMat = XMMatrixMultiply(ViewMat, ProjMat);

		XMVECTOR DetViewMat = XMMatrixDeterminant(ViewMat);
		XMMATRIX InvViewMat = XMMatrixInverse(&DetViewMat, ViewMat);

		XMVECTOR DetProjMat = XMMatrixDeterminant(ProjMat);
		XMMATRIX InvProjMat = XMMatrixInverse(&DetProjMat, ProjMat);

		XMVECTOR DetVPMatMat = XMMatrixDeterminant(VPMat);
		XMMATRIX InvVPMat = XMMatrixInverse(&DetVPMatMat, VPMat);

		XMStoreFloat4x4(&cubeFacePassCB.ViewMat, XMMatrixTranspose(ViewMat));
		XMStoreFloat4x4(&cubeFacePassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
		XMStoreFloat4x4(&cubeFacePassCB.ProjMat, XMMatrixTranspose(ProjMat));
		XMStoreFloat4x4(&cubeFacePassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
		XMStoreFloat4x4(&cubeFacePassCB.VPMat, XMMatrixTranspose(VPMat));
		XMStoreFloat4x4(&cubeFacePassCB.InvVPMat, XMMatrixTranspose(InvVPMat));
		cubeFacePassCB.EyePosW = m_CubeMapCamera[i].GetPosition3f();
		cubeFacePassCB.RenderTargetSize = XMFLOAT2((float)CubeMapSize, (float)CubeMapSize);
		cubeFacePassCB.InvRenderTargetSize = XMFLOAT2(1.0f / CubeMapSize, 1.0f / CubeMapSize);

		UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
		// 오프셋에 맞춰서 넣어준다.
		currPassCB->CopyData(1 + i, cubeFacePassCB);
	}
}

void NormalMappingApp::BuildCubeDepthStencil()
{
	// DynamicTexture Cube용이다.
	// 한 장면씩 그리기 때문에, 하나만 있어도 된다.
	D3D12_RESOURCE_DESC depthStencilDesc;
	depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depthStencilDesc.Alignment = 0;
	depthStencilDesc.Width = CubeMapSize; // 사이즈를 맞춰추고
	depthStencilDesc.Height = CubeMapSize;
	depthStencilDesc.DepthOrArraySize = 1;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.Format = m_DepthStencilFormat;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_CLEAR_VALUE optClear;
	optClear.Format = m_DepthStencilFormat;
	optClear.DepthStencil.Depth = 1.f;
	optClear.DepthStencil.Stencil = 0;

	// 리소스를 만들고
	CD3DX12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
		&defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&depthStencilDesc,
		D3D12_RESOURCE_STATE_COMMON,
		&optClear,
		IID_PPV_ARGS(m_CubeDepthStencilBuffer.GetAddressOf())
	));
	// 뷰를 만들어준다.
	m_d3dDevice->CreateDepthStencilView(m_CubeDepthStencilBuffer.Get(), nullptr, m_CubeDsvHandle);
	// 사용할 수 있도록 베리어를 바꿔준다.
	CD3DX12_RESOURCE_BARRIER cubeDSV_COMM_WRITE = CD3DX12_RESOURCE_BARRIER::Transition(
		m_CubeDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
	m_CommandList->ResourceBarrier(1, &cubeDSV_COMM_WRITE);
}

void NormalMappingApp::DrawSceneToCubeMap()
{
	// 헬퍼 클래스로 관리하던, viewport와 scissorrRect 정보를 이용해서 세팅한다.
	D3D12_VIEWPORT dynaCubeMapViewport = m_DynamicCubeMap->GetViewport();
	D3D12_RECT dynaCubeMapScissorRect = m_DynamicCubeMap->GetScissorRect();
	m_CommandList->RSSetViewports(1, &dynaCubeMapViewport);
	m_CommandList->RSSetScissorRects(1, &dynaCubeMapScissorRect);

	// 렌더 타겟도 클래스 인스턴스에 있는 친구로 설정한다.
	CD3DX12_RESOURCE_BARRIER dynaCubeMap_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DynamicCubeMap->GetResource(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &dynaCubeMap_READ_RT);

	// 각 방향 별로 (상하전후좌우) 렌더링을 하기에, pass constanct도 그때마다 넘겨주는 것이다.
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	for (int i = 0; i < 6; i++)
	{
		// 백버퍼역할을 하는 친구와 뎁스버퍼를 초기화한다.
		m_CommandList->ClearRenderTargetView(m_DynamicCubeMap->GetRtv(i), Colors::LightSteelBlue, 0, nullptr);
		m_CommandList->ClearDepthStencilView(m_CubeDsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.f, 0, 0, nullptr);

		// 렌더 타겟을 설정한다. (미리 가지고 있던, CubeMap 용 depth-stencil view도 설정한다.)
		CD3DX12_CPU_DESCRIPTOR_HANDLE curRtv = m_DynamicCubeMap->GetRtv(i);
		m_CommandList->OMSetRenderTargets(1, &curRtv, true, &m_CubeDsvHandle);

		// 각 방향 마다 pass constant를 넘겨준다. (방향마다 camera 방향이 다르기 때문)
		ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS passCBAddress = passCB->GetGPUVirtualAddress() + (1 + i) * passCBByteSize;
		m_CommandList->SetGraphicsRootConstantBufferView(1, passCBAddress); // PassConstant는 1번

		// 불투명한 친구 먼저
		DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);
		// 그 다음에 하늘을 그려야 성능이 낭비가 안된다.
		m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
		DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Sky]);

		m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
	}

	// 이제 다 그린 텍스쳐를 사용할 수 있게 Read로 바꾼다.
	CD3DX12_RESOURCE_BARRIER dynaCubeMap_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(
		m_DynamicCubeMap->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_CommandList->ResourceBarrier(1, &dynaCubeMap_RT_READ);
}

void NormalMappingApp::BuildCubeFaceCamera(float _x, float _y, float _z)
{
	// 인자로 들어온 위치에서 6방향으로 카메라를 만든다.
	XMFLOAT3 center(_x, _y, _z);
	XMFLOAT3 worldUp(0.0f, 1.0f, 0.0f);

	XMFLOAT3 targets[6] =
	{
		XMFLOAT3(_x + 1.0f, _y, _z), // +X
		XMFLOAT3(_x - 1.0f, _y, _z), // -X
		XMFLOAT3(_x, _y + 1.0f, _z), // +Y
		XMFLOAT3(_x, _y - 1.0f, _z), // -Y
		XMFLOAT3(_x, _y, _z + 1.0f), // +Z
		XMFLOAT3(_x, _y, _z - 1.0f)  // -Z
	};

	// 카메라 위 아래를 볼때는, 업벡터를 +Z/-Z를 대신 사용한다.
	XMFLOAT3 ups[6] =
	{
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // +X
		XMFLOAT3(0.0f, 1.0f, 0.0f),  // -X
		XMFLOAT3(0.0f, 0.0f, -1.0f), // +Y
		XMFLOAT3(0.0f, 0.0f, +1.0f), // -Y
		XMFLOAT3(0.0f, 1.0f, 0.0f),	 // +Z
		XMFLOAT3(0.0f, 1.0f, 0.0f)	 // -Z
	};
	// app에서 관리하도록 카메라를 생성한다.
	for (int i = 0; i < 6; ++i)
	{
		m_CubeMapCamera[i].LookAt(center, targets[i], ups[i]);
		m_CubeMapCamera[i].SetFrustum(0.5f * XM_PI, 1.0f, 0.1f, 1000.0f);
		m_CubeMapCamera[i].UpdateViewMatrix();
	}
}

void NormalMappingApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
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

float NormalMappingApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 NormalMappingApp::GetHillsNormal(float _x, float _z) const
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> NormalMappingApp::GetStaticSamplers()
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
