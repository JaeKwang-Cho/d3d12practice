//***************************************************************************************
// TessellationApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>

#define BASIC (0)
#define BEZIER (1)

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
	UINT ObjCBIndex = 1;

	// 이 예제에서는 Geometry 말고도
	MeshGeometry* Geo = nullptr;
	// Material 포인터도 가지고 있는다.
	Material* Mat = nullptr;

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
	Count
};

class TessellationApp : public D3DApp
{
public:
	TessellationApp(HINSTANCE hInstance);
	~TessellationApp();

	TessellationApp(const TessellationApp& _other) = delete;
	TessellationApp& operator=(const TessellationApp& _other) = delete;

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
	void UpdateCamera(const GameTimer& _gt);
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMaterialCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);
	void AnimateMaterials(const GameTimer& _gt);

	// ==== Init ====
	void LoadTextures();
	void BuildRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildQuadPatchGeometry_Basic();
	void BuildQuadPatchGeometry_Bezier();
	void BuildPSOs();
	void BuildFrameResources();
	void BuildMaterials();
	void BuildRenderItems();

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

	XMFLOAT3 m_EyePos = { 0.f, 0.f, 0.f };
	XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	float m_Phi = 1.24f * XM_PI;
	float m_Theta = 0.42f * XM_PI;
	float m_Radius = 50.f;

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
		TessellationApp theApp(hInstance);
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

TessellationApp::TessellationApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

TessellationApp::~TessellationApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool TessellationApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));
	
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
#if BASIC
	BuildQuadPatchGeometry_Basic();
#elif BEZIER
	BuildQuadPatchGeometry_Bezier();
#endif	
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();

	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void TessellationApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 0.01f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void TessellationApp::Update(const GameTimer& _gt)
{
	// 더 기능이 많아질테니, 함수로 쪼개서 넣는다.
	OnKeyboardInput(_gt);
	UpdateCamera(_gt);

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

void TessellationApp::Draw(const GameTimer& _gt)
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
	
	// 뷰 힙 설정
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CbvSrvUavDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// 루트 시그니처 설정
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());
	// Frame CB 넘겨주기
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());
	// 그리기 요청
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);
	// (PSO에서도 RenderItem 에서도, Primitive Topology 형식을 Patch를 가지고 있다.)

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

void TessellationApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void TessellationApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void TessellationApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
{
	// 왼쪽 마우스가 눌린 상태에서 움직인다면
	if ((_btnState & MK_LBUTTON) != 0)
	{
		// 마우스가 움직인 픽셀의 1/4 만큼 1 radian 으로 바꿔서 세타와 피에 적용시킨다.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(_x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(_y - m_LastMousePos.y));

		m_Phi += dx;
		m_Theta += dy;

		// Phi는 0 ~ Pi 구간을 유지하도록 한다.
		m_Theta = MathHelper::Clamp(m_Theta, 0.1f, MathHelper::Pi - 0.1f);
	}
	// 오른쪽 마우스가 눌린 상태에서 움직이면
	else if ((_btnState & MK_RBUTTON) != 0)
	{
		// 마우스가 움직인 만큼 구심좌표계의 반지름을 변화시킨다.
		float dx = 0.1f * static_cast<float>(_x - m_LastMousePos.x);
		float dy = 0.1f * static_cast<float>(_y - m_LastMousePos.y);

		m_Radius += dx - dy;

		// 반지름은 3 ~ 15 구간을 유지한다.
		m_Radius = MathHelper::Clamp(m_Radius, 5.f, 150.f);
	}

	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void TessellationApp::OnKeyboardInput(const GameTimer _gt)
{

}

void TessellationApp::UpdateCamera(const GameTimer& _gt)
{
	// 구심 좌표계 값에 따라 데카르트 좌표계로 변환한다.
	m_EyePos.x = m_Radius * sinf(m_Theta) * cosf(m_Phi);
	m_EyePos.z = m_Radius * sinf(m_Theta) * sinf(m_Phi);
	m_EyePos.y = m_Radius * cosf(m_Theta);

	// View(Camera) Mat을 초기화 한다.
	XMVECTOR pos = XMVectorSet(m_EyePos.x, m_EyePos.y, m_EyePos.z, 1.f); // 카메라 위치
	XMVECTOR target = XMVectorZero(); // 카메라 바라보는 곳
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f); // 월드(?) 업 벡터

	XMMATRIX viewMat = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_ViewMat, viewMat);
}

void TessellationApp::UpdateObjectCBs(const GameTimer& _gt)
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
			
			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void TessellationApp::UpdateMaterialCBs(const GameTimer& _gt)
{
	UploadBuffer<MaterialConstants>* currMaterialCB = m_CurrFrameResource->MaterialCB.get();	
	for (std::pair<const std::string, std::unique_ptr<Material>>& e : m_Materials)
	{
		// 이것도 dirty flag가 켜져있을 때만 업데이트를 한다.
		Material* mat = e.second.get();
		if (mat->NumFramesDirty > 0)
		{
			XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);

			MaterialConstants matConstants;
			matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
			matConstants.FresnelR0 = mat->FresnelR0;
			matConstants.Roughness = mat->Roughness;
			XMStoreFloat4x4(&matConstants.MatTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void TessellationApp::UpdateMainPassCB(const GameTimer& _gt)
{
	XMMATRIX ViewMat = XMLoadFloat4x4(&m_ViewMat);
	XMMATRIX ProjMat = XMLoadFloat4x4(&m_ProjMat);

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

	m_MainPassCB.EyePosW = m_EyePos;
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

void TessellationApp::AnimateMaterials(const GameTimer& _gt)
{
}


void TessellationApp::LoadTextures()
{
	std::unique_ptr<Texture> grassTex = std::make_unique<Texture>();
	grassTex->Name = "grassTex";
	grassTex->Filename = L"../Textures/grass.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		grassTex->Filename.c_str(),
		grassTex->Resource,
		grassTex->UploadHeap
	));
	m_Textures[grassTex->Name] = std::move(grassTex);

	std::unique_ptr<Texture> waterTex = std::make_unique<Texture>();
	waterTex->Name = "waterTex";
	waterTex->Filename = L"../Textures/water1.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		waterTex->Filename.c_str(),
		waterTex->Resource,
		waterTex->UploadHeap
	));
	m_Textures[waterTex->Name] = std::move(waterTex);

	std::unique_ptr<Texture> fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "fenceTex";
	fenceTex->Filename = L"../Textures/WireFence.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		fenceTex->Filename.c_str(),
		fenceTex->Resource,
		fenceTex->UploadHeap
	));
	m_Textures[fenceTex->Name] = std::move(fenceTex);

	// 텍스쳐를 사용하지 않을 때 쓰는 친구다. (연습문제 용)
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


void TessellationApp::BuildRootSignature()
{	
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	UINT numOfParameter = 4;

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 - texture 정보

	// Srv가 넘어가는 테이블, Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	slotRootParameter[1].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[2].InitAsConstantBufferView(1); // b1 - PassConstants
	slotRootParameter[3].InitAsConstantBufferView(2); // b2 - Material

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

void TessellationApp::BuildDescriptorHeaps()
{
 	const int textureDescriptorCount = 4;

	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = textureDescriptorCount;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CbvSrvUavDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* grassTex = m_Textures["grassTex"].get()->Resource.Get();
	ID3D12Resource* waterTex = m_Textures["waterTex"].get()->Resource.Get();
	ID3D12Resource* fenceTex = m_Textures["fenceTex"].get()->Resource.Get();
	ID3D12Resource* whiteTex = m_Textures["white1x1Tex"].get()->Resource.Get();

	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = grassTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = -1;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(grassTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = waterTex->GetDesc().Format;
	m_d3dDevice->CreateShaderResourceView(waterTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = fenceTex->GetDesc().Format;
	m_d3dDevice->CreateShaderResourceView(fenceTex, &srcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = whiteTex->GetDesc().Format;
	m_d3dDevice->CreateShaderResourceView(whiteTex, &srcDesc, viewHandle);
}

void TessellationApp::BuildShadersAndInputLayout()
{
#if BASIC
	m_Shaders["tessVS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["tessHS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "HS", "hs_5_1");
	m_Shaders["tessDS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "DS", "ds_5_1");
	m_Shaders["tessPS"] = d3dUtil::CompileShader(L"Shaders\\Tessellation.hlsl", nullptr, "PS", "ps_5_1");
#elif BEZIER
	m_Shaders["tessVS"] = d3dUtil::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["tessHS"] = d3dUtil::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "HS", "hs_5_1");
	m_Shaders["tessDS"] = d3dUtil::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "DS", "ds_5_1");
	m_Shaders["tessPS"] = d3dUtil::CompileShader(L"Shaders\\BezierTessellation.hlsl", nullptr, "PS", "ps_5_1");
#endif

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void TessellationApp::BuildQuadPatchGeometry_Basic()
{
	std::array<XMFLOAT3, 4> vertices =
	{
		XMFLOAT3(-10.0f, 0.0f, +10.0f),
		XMFLOAT3(+10.0f, 0.0f, +10.0f),
		XMFLOAT3(-10.0f, 0.0f, -10.0f),
		XMFLOAT3(+10.0f, 0.0f, -10.0f)
	};

	std::array<std::int16_t, 4> indices = { 0, 1, 2, 3 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "quadpatchGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),m_CommandList.Get(), 
		vertices.data(), vbByteSize, 
		geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(), m_CommandList.Get(), 
		indices.data(), ibByteSize, 
		geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(XMFLOAT3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = 4;
	quadSubmesh.StartIndexLocation = 0;
	quadSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["quadpatch"] = quadSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void TessellationApp::BuildQuadPatchGeometry_Bezier()
{
	std::array<XMFLOAT3, 16> vertices =
	{
		// Row 0
		XMFLOAT3(-10.0f, -10.0f, +15.0f),
		XMFLOAT3(-5.0f,  0.0f, +15.0f),
		XMFLOAT3(+5.0f,  0.0f, +15.0f),
		XMFLOAT3(+10.0f, 0.0f, +15.0f),

		// Row 1
		XMFLOAT3(-15.0f, 0.0f, +5.0f),
		XMFLOAT3(-5.0f,  0.0f, +5.0f),
		XMFLOAT3(+5.0f,  20.0f, +5.0f),
		XMFLOAT3(+15.0f, 0.0f, +5.0f),

		// Row 2
		XMFLOAT3(-15.0f, 0.0f, -5.0f),
		XMFLOAT3(-5.0f,  0.0f, -5.0f),
		XMFLOAT3(+5.0f,  0.0f, -5.0f),
		XMFLOAT3(+15.0f, 0.0f, -5.0f),

		// Row 3
		XMFLOAT3(-10.0f, 10.0f, -15.0f),
		XMFLOAT3(-5.0f,  0.0f, -15.0f),
		XMFLOAT3(+5.0f,  0.0f, -15.0f),
		XMFLOAT3(+25.0f, 10.0f, -15.0f)
	};

	std::array<std::int16_t, 16> indices =
	{
		0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15
	};

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "quadpatchGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(), m_CommandList.Get(), 
		vertices.data(), vbByteSize, 
		geo->VertexBufferUploader);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(), m_CommandList.Get(), 
		indices.data(), ibByteSize, 
		geo->IndexBufferUploader);

	geo->VertexByteStride = sizeof(XMFLOAT3);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry quadSubmesh;
	quadSubmesh.IndexCount = (UINT)indices.size();
	quadSubmesh.StartIndexLocation = 0;
	quadSubmesh.BaseVertexLocation = 0;

	geo->DrawArgs["quadpatch"] = quadSubmesh;

	m_Geometries[geo->Name] = std::move(geo);
}

void TessellationApp::BuildPSOs()
{
	// 불투명 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSODesc;

	ZeroMemory(&opaquePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePSODesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	opaquePSODesc.pRootSignature = m_RootSignature.Get();
	opaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["tessVS"]->GetBufferPointer()),
		m_Shaders["tessVS"]->GetBufferSize()
	};
	opaquePSODesc.HS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["tessHS"]->GetBufferPointer()),
		m_Shaders["tessHS"]->GetBufferSize()
	};
	opaquePSODesc.DS =
	{
	reinterpret_cast<BYTE*>(m_Shaders["tessDS"]->GetBufferPointer()),
		m_Shaders["tessDS"]->GetBufferSize()
	};
	opaquePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["tessPS"]->GetBufferPointer()),
		m_Shaders["tessPS"]->GetBufferSize()
	};
	opaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	opaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePSODesc.SampleMask = UINT_MAX;
	// 여기가 중요하다. TYPE_PATCH로 하는 것이 중요하다.
	opaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_PATCH;
	opaquePSODesc.NumRenderTargets = 1;
	opaquePSODesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePSODesc.DSVFormat = m_DepthStencilFormat;

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaquePSODesc, IID_PPV_ARGS(&m_PSOs["opaque"])));
}

void TessellationApp::BuildFrameResources()
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

void TessellationApp::BuildMaterials()
{
	std::unique_ptr<Material> practiceMat = std::make_unique<Material>();
	practiceMat->Name = "practice1Mat";
	practiceMat->MatCBIndex = 0;
	practiceMat->DiffuseSrvHeapIndex = 3;
	practiceMat->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	practiceMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	practiceMat->Roughness = 0.5;

	m_Materials["practiceMat"] = std::move(practiceMat);
}

void TessellationApp::BuildRenderItems()
{
	std::unique_ptr<RenderItem> quadPatchRenderItem = std::make_unique<RenderItem>();
	quadPatchRenderItem->WorldMat = MathHelper::Identity4x4();
	XMStoreFloat4x4(&quadPatchRenderItem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
	quadPatchRenderItem->ObjCBIndex = 0;
	quadPatchRenderItem->Mat = m_Materials["practiceMat"].get();
	quadPatchRenderItem->Geo = m_Geometries["quadpatchGeo"].get();
	// PSO와 마찬가지로 여기도 POINT_PATCHLIST로 넘겨주어야 한다.
#if BASIC
	quadPatchRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST; // 꼭짓점 4개의 Patch를 list로 넘긴다.
#elif BEZIER
	quadPatchRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_16_CONTROL_POINT_PATCHLIST; // 꼭짓점 16개의 Patch를 list로 넘긴다.
#endif
	quadPatchRenderItem->IndexCount = quadPatchRenderItem->Geo->DrawArgs["quadpatch"].IndexCount;
	quadPatchRenderItem->StartIndexLocation = quadPatchRenderItem->Geo->DrawArgs["quadpatch"].StartIndexLocation;
	quadPatchRenderItem->BaseVertexLocation = quadPatchRenderItem->Geo->DrawArgs["quadpatch"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(quadPatchRenderItem.get());
	m_AllRenderItems.push_back(std::move(quadPatchRenderItem));
}

void TessellationApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
{
	// 이제 Material도 물체마다 업데이트 해줘야 한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();
	ID3D12Resource* matCB = m_CurrFrameResource->MaterialCB->Resource();

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
		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(ri->Mat->DiffuseSrvHeapIndex, m_CbvSrvUavDescriptorSize);

		// Object Constant와
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;
		// Material Constant를
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress();
		matCBAddress += ri->Mat->MatCBIndex * matCBByteSize;

		// PSO에 연결해준다.
		_cmdList->SetGraphicsRootDescriptorTable(0, tex);
		_cmdList->SetGraphicsRootConstantBufferView(1, objCBAddress);
		_cmdList->SetGraphicsRootConstantBufferView(3, matCBAddress);

		_cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

float TessellationApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 TessellationApp::GetHillsNormal(float _x, float _z) const
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> TessellationApp::GetStaticSamplers()
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