//***************************************************************************************
// LitWavesApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// Use arrow keys to move light positions.
//
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include "Waves.h"
#include "FileReader.h"

#define PRAC1 (0)
#define PRAC2 (0)
#define SKULL (0)
#define PRAC3 (0)
#define PRAC4 (0)
#define PRAC5 (0)
#define PRAC6 (0)

const int g_NumFrameResources = 3;

// vertex, index, CB, PrimitiveType, DrawIndexedInstanced 등
// 요걸 묶어서 렌더링하기 좀 더 편하게 해주는 구조체이다.
struct RenderItem 
{
	RenderItem() = default;

	// item 마다 World를 가지고 있게 한다.
	XMFLOAT4X4 WorldMat = MathHelper::Identity4x4();

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

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class LightNMaterialApp : public D3DApp
{
public:
	LightNMaterialApp(HINSTANCE hInstance);
	~LightNMaterialApp();

	LightNMaterialApp(const LightNMaterialApp& _other) = delete;
	LightNMaterialApp& operator=(const LightNMaterialApp& _other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& _gt) override;
	virtual void Draw(const GameTimer& _gt) override;

	virtual void OnMouseDown(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseUp(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove(WPARAM _btnState, int _x, int _y) override;
	// 키보드에서 조명의 위치를 바꿀 것이다.
	void OnKeyboardInput(const GameTimer _gt);

	void UpdateCamera(const GameTimer& _gt);
	void ChangeRoughness(UINT _flag);

	void UpdateObjectCBs(const GameTimer& _gt);
	// Material CB 업데이트 함수를 추가한다.
	void UpdateMaterialCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);
	void UpdateMainPassCB_3PointLights(const GameTimer& _gt);
	void UpdateWaves(const GameTimer& _gt);

	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	// 지형을 렌더링 해볼 것이다.
	void BuildLandGeometry();
	void BuildWavesGeometryBuffers();

	// Prac3 용이다.
	void BuildShapeGeometry();
	void BuildSkull();
	void BuildMatForSkullNGeo();
	void BuildRenderItemForSkullNGeo();

	// 그리고 여기서는, Root Signature를
	// Root Constant로 사용할 것 이기 때문에
	// View Heap이 필요가 없다.
	void BuildPSOs();
	void BuildFrameResources();
	// Geometry 에 맞는 Material을 생성해준다.
	void BuildMaterials();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems);

	// 지형의 높이와 노멀을 계산하는 함수다.
	float GetHillsHeight(float _x, float _z) const;
	XMFLOAT3 GetHillsNormal(float _x, float _z)const;

private:
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	// Root Signature 만 사용한다. Constant Root 기 때문
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	// 도형, 쉐이더, PSO 등등 App에서 관리한다..
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	// Material 도 추가한다.
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	// Texture는 아직 사용하지 않는다.
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
	std::vector<RenderItem*> m_RItemLayer[(int)RenderLayer::Count];
	// 파도의 높이를 업데이트 해주는 클래스 인스턴스다.
	std::unique_ptr<Waves> m_Waves;

	// Object 관계 없이, Render Pass 전체가 공유하는 값이다.
	PassConstants m_MainPassCB;

	XMFLOAT3 m_EyePos = { 0.f, 0.f, 0.f };
	XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	float m_Phi = 1.5f * XM_PI;
	float m_Theta = XM_PIDIV2 - 0.1f;
#if SKULL
	float m_Radius = 20.f;
#else
	float m_Radius = 50.f;
#endif
	bool m_bKeyLight = false;
	bool m_bfillLight = false;
	bool m_bBackLight = false;

	XMFLOAT3 m_SpherePosArray[10];

	float m_SunPhi = 1.25f * XM_PI;
	float m_SunTheta = XM_PIDIV4;

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
		LightNMaterialApp theApp(hInstance);
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

LightNMaterialApp::LightNMaterialApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

LightNMaterialApp::~LightNMaterialApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool LightNMaterialApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	// 얘는 좀 복잡해서 따로 클래스로 만들었다.
	m_Waves = std::make_unique<Waves>(128, 128, 1.f, 0.03f, 4.f, 0.2f);

	// 여기서는 공동으로 쓰는 RootSignature를 먼저 정의한다.
	BuildRootSignature();
	BuildShadersAndInputLayout();

#if SKULL
	BuildShapeGeometry();
	BuildSkull();
	BuildMatForSkullNGeo();
	BuildRenderItemForSkullNGeo();
#else
	// 지형을 만들고
	BuildLandGeometry();
	// 그 버퍼를 만들고
	BuildWavesGeometryBuffers();
	// 지형과 파도에 맞는 머테리얼을 만든다.
	BuildMaterials();
	// 아이템 화를 시킨다.
	BuildRenderItems();
#endif

	// 이제 FrameResource를 만들고
	BuildFrameResources();
	// 이제 PSO를 만들어서, 돌리면서 렌더링할 준비를 한다.
	BuildPSOs();

	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void LightNMaterialApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void LightNMaterialApp::Update(const GameTimer& _gt)
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
	// CB를 업데이트 해준다.
	UpdateObjectCBs(_gt);
	UpdateMaterialCBs(_gt);
	UpdateMainPassCB(_gt);

	// 파도를 업데이트 해준다.
#if !SKULL
	UpdateWaves(_gt);
#endif
}

void LightNMaterialApp::Draw(const GameTimer& _gt)
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
	D3D12_RESOURCE_BARRIER bufferBarrier_RENDER_TARGET = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_RENDER_TARGET
	);

	m_CommandList->ResourceBarrier(
		1, 
		&bufferBarrier_RENDER_TARGET);

	// 백 버퍼와 깊이 버퍼를 초기화 한다.
	m_CommandList->ClearRenderTargetView(
		GetCurrentBackBufferView(), 
		Colors::LightSteelBlue, 
		0, 
		nullptr);
	m_CommandList->ClearDepthStencilView(
		GetDepthStencilView(), 
		D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 
		1.0f, 
		0, 
		0, 
		nullptr);

	// Rendering 할 백 버퍼와, 깊이 버퍼의 핸들을 OM 단계에 세팅을 해준다.
	D3D12_CPU_DESCRIPTOR_HANDLE BackBufferHandle = GetCurrentBackBufferView();
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilHandle = GetDepthStencilView();
	m_CommandList->OMSetRenderTargets(1, &BackBufferHandle, true, &DepthStencilHandle);

	// ==== 지형과 파도 그리기 ======
	
	// view와 직접 연결 되기 때문에, view heap 설정은 필요 없다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Frame 에 한번 넘겨주는 Constant를 bind 한다.
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	// 2번 (b2)에 연결 했다.
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); 
	
	// RenderItem을 그린다.
	DrawRenderItems(m_CommandList.Get(), m_RItemLayer[(int)RenderLayer::Opaque]);

	// =============================
	// 그림을 그릴 back buffer의 Resource Barrier의 Usage 를 D3D12_RESOURCE_STATE_PRESENT으로 바꾼다.
	D3D12_RESOURCE_BARRIER bufferBarrier_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);

	m_CommandList->ResourceBarrier(
		1, 
		&bufferBarrier_PRESENT
	);

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

void LightNMaterialApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void LightNMaterialApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void LightNMaterialApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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
		float dx = 0.005f * static_cast<float>(_x - m_LastMousePos.x);
		float dy = 0.005f * static_cast<float>(_y - m_LastMousePos.y);

		m_Radius += dx - dy;

		// 반지름은 3 ~ 15 구간을 유지한다.
		m_Radius = MathHelper::Clamp(m_Radius, 3.f, 15.f);
	}

	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void LightNMaterialApp::OnKeyboardInput(const GameTimer _gt)
{
	const float dt = _gt.GetDeltaTime();

	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_SunPhi -= 1.f * dt;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
	{
		m_SunPhi += 1.f * dt;
	}
	if (GetAsyncKeyState(VK_UP) & 0x8000)
	{
		m_SunTheta -= 1.f * dt;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
	{
		m_SunTheta += 1.f * dt;
	}
#if PRAC2
	if (GetAsyncKeyState('1') & 0x8000)
	{
		ChangeRoughness(1);
	}
	if (GetAsyncKeyState('2') & 0x8000)
	{
		ChangeRoughness(2);
	}
	if (GetAsyncKeyState('3') & 0x8000)
	{
		ChangeRoughness(3);
	}
#elif PRAC3
	if (GetAsyncKeyState('1') & 0x8000)
	{
		m_bKeyLight = true;
	}
	if (GetAsyncKeyState('2') & 0x8000)
	{
		m_bfillLight = true;
	}
	if (GetAsyncKeyState('3') & 0x8000)
	{
		m_bBackLight = true;
	}
	if (GetAsyncKeyState('4') & 0x8000)
	{
		m_bKeyLight = false;
		m_bfillLight = false;
		m_bBackLight = false;
	}
#endif

	m_SunTheta = MathHelper::Clamp(m_SunTheta, 0.1f, XM_PIDIV2);
}

void LightNMaterialApp::UpdateCamera(const GameTimer& _gt)
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

void LightNMaterialApp::ChangeRoughness(UINT _flag)
{
	std::unique_ptr<Material>& grassMat = m_Materials["grass"];
	std::unique_ptr<Material>& waterMat = m_Materials["water"];

	switch (_flag)
	{
	case 1:
	{
		grassMat->Roughness = 1.f;

		waterMat->Roughness = 0.f;
	}
		break;
	case 2:
	{
		grassMat->Roughness = 0.5f;

		waterMat->Roughness = 0.5f;
	}
		break;
	case 3:
	{
		grassMat->Roughness = 0.f;

		waterMat->Roughness = 1.f;
	}
		break;
	default:
		break;
	}

	grassMat->NumFramesDirty = g_NumFrameResources;
	waterMat->NumFramesDirty = g_NumFrameResources;
}

void LightNMaterialApp::UpdateObjectCBs(const GameTimer& _gt)
{
	UploadBuffer<ObjectConstants>* currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for (std::unique_ptr<RenderItem>& e : m_AllRenderItems)
	{
		// Constant Buffer가 변경됐을 때 Update를 한다.
		if (e->NumFrameDirty > 0)
		{
			// CPU에 있는 값을 가져와서
			XMMATRIX worldMat = XMLoadFloat4x4(&e->WorldMat);

			ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.WorldMat, XMMatrixTranspose(worldMat));
			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void LightNMaterialApp::UpdateMaterialCBs(const GameTimer& _gt)
{
	UploadBuffer<MaterialConstants>* currMaterialCB = m_CurrFrameResource->MaterialCB.get();	
	// std::pair<std::string, std::unique_ptr<Material>>
	// 
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

			currMaterialCB->CopyData(mat->MatCBIndex, matConstants);

			mat->NumFramesDirty--;
		}
	}
}

void LightNMaterialApp::UpdateMainPassCB(const GameTimer& _gt)
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
	m_MainPassCB.NearZ = 1.f;
	m_MainPassCB.FarZ = 1000.f;
	m_MainPassCB.TotalTime = _gt.GetTotalTime();
	m_MainPassCB.DeltaTime = _gt.GetDeltaTime();

	// 이제 Light를 채워준다. Shader와 App에서 정의한 Lights의 개수와 종류가 같아야 한다.

#if PRAC1
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.f };
	float sinWave = 0.5f * sinf(_gt.GetTotalTime()) + 0.5f;
	m_MainPassCB.Lights[0].Strength = { 1.f * sinWave, 1.f * sinWave, 0.9f * sinWave };
#elif PRAC3
	m_MainPassCB.AmbientLight = { 0.125f, 0.125f, 0.175f, 1.f };
	UpdateMainPassCB_3PointLights(_gt);
#elif PRAC4
	m_MainPassCB.AmbientLight = { 0.125f, 0.125f, 0.175f, 1.f };
	for (int i = 0; i < 10; i++)
	{
		m_MainPassCB.Lights[i].Strength = { 0.75f, 0.75f, 0.75f };
		// Point light 맴버를 채운다.
		m_MainPassCB.Lights[i].Position = m_SpherePosArray[i];
		m_MainPassCB.Lights[i].FalloffStart = 1.f;
		m_MainPassCB.Lights[i].FalloffEnd = 10.f;
	}
#elif PRAC5
	m_MainPassCB.AmbientLight = { 0.125f, 0.125f, 0.175f, 1.f };
	float switcher = -1.f;
	for (int i = 0; i < 10; i++)
	{
		m_MainPassCB.Lights[i].Strength = { 0.75f, 0.75f, 0.75f };
		// Spot light 맴버를 채운다.
		m_MainPassCB.Lights[i].Position = m_SpherePosArray[i];
		m_MainPassCB.Lights[i].Position.y += 1.f;
		m_MainPassCB.Lights[i].FalloffStart = 1.f;
		m_MainPassCB.Lights[i].FalloffEnd = 10.f;
		switcher *= -1.f;
		m_MainPassCB.Lights[i].Direction = { switcher, -1.f, 0.f };
		m_MainPassCB.Lights[i].SpotPower = 8.f;
	}
#else
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.f };
	// 태양의 위치를 구심좌표계로 구하고, 그걸 뒤집어서 벡터로 표현해준다.
	XMVECTOR lightDir = -MathHelper::SphericalToCartesian(1.f, m_SunPhi, m_SunTheta);
	XMStoreFloat3(&m_MainPassCB.Lights[0].Direction, lightDir);
	m_MainPassCB.Lights[0].Strength = { 1.f, 1.f, 0.9f };
#endif
	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void LightNMaterialApp::UpdateMainPassCB_3PointLights(const GameTimer& _gt)
{
	// Key Light
	XMVECTOR keyLightDir = -MathHelper::SphericalToCartesian(1.f, m_SunPhi, m_SunTheta);
	XMStoreFloat3(&m_MainPassCB.Lights[0].Direction, keyLightDir);
	if (m_bKeyLight)
	{
		m_MainPassCB.Lights[0].Strength = { 1.f, 1.f, 0.9f };
	}
	else
	{
		m_MainPassCB.Lights[0].Strength = { 0.f, 0.f, 0.f };
	}
	

	// Fill Light
	XMVECTOR fillLightDir = -MathHelper::SphericalToCartesian(1.f, m_SunPhi + XM_PIDIV2, m_SunTheta);
	XMStoreFloat3(&m_MainPassCB.Lights[1].Direction, fillLightDir);

	if (m_bfillLight)
		m_MainPassCB.Lights[1].Strength = { 0.4f, 0.4f, 0.36f };
	else
		m_MainPassCB.Lights[1].Strength = { 0.f, 0.f, 0.f };

	// Back Light
	XMVECTOR backLightDir = -MathHelper::SphericalToCartesian(1.f, m_SunPhi + XM_PI, m_SunTheta + XM_PIDIV4);
	XMStoreFloat3(&m_MainPassCB.Lights[2].Direction, backLightDir);

	if (m_bBackLight)
		m_MainPassCB.Lights[2].Strength = { 0.2f, 0.2f, 0.18f };
	else
	{
		m_MainPassCB.Lights[2].Strength = { 0.f, 0.f, 0.f };
	}
}

void LightNMaterialApp::UpdateWaves(const GameTimer& _gt)
{
	// 파도는 매 프레임 바꿔줘서 올려야하니까
	// dynamic vertex buffer 에 올려줘야 한다.

	// 1/4 초 마다 무작위 파도를 생성한다.
	static float timeBase = 0.f;
	if ((_gt.GetTotalTime() - timeBase) >= 0.25f)
	{
		timeBase += 0.25f;
		// 랜덤 위치
		int i = MathHelper::Rand(4, m_Waves->RowCount() - 5);
		int j = MathHelper::Rand(4, m_Waves->ColumnCount() - 5);
		// 랜덤 세기
		float r = MathHelper::RandF(0.2f, 0.5f);
		// 파도 일으키기
		m_Waves->Disturb(i, j, r);
	}

	// 파도 업데이트하기
	m_Waves->Update(_gt.GetDeltaTime());

	// 새로운 파원으로 파도 vertex buffer를 업데이트한다.
	UploadBuffer<Vertex>* currWaveVB = m_CurrFrameResource->WaveVB.get();
	for (int i = 0; i < m_Waves->VertexCount(); i++)
	{
		Vertex v;
		// 위치 (높이)를 넣고
		v.Pos = m_Waves->GetSolutionPos(i);
		// 노멀도 넣어준다.
		v.Normal = m_Waves->GetSolutionNorm(i);
		// 버퍼에 복사해준다.
		currWaveVB->CopyData(i, v);
	}
	// 그리고 그걸 Buffer에 업데이트 해준다.
	m_WavesRenderItem->Geo->VertexBufferGPU = currWaveVB->Resource();
}

void LightNMaterialApp::BuildRootSignature()
{
	// 여기서는 Table을 사용하는 것이 아니라
	// Constant Buffer View를 직접 연결한다.
	// 그에 맞는 Root Signature 설정을 해준다.

	CD3DX12_ROOT_PARAMETER slotRootParameter[3];
	// 루트 파라미터를 Root Constant 설정할 것이다.
	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[1].InitAsConstantBufferView(1); // b1 - Material
	slotRootParameter[2].InitAsConstantBufferView(2); // b2 - PassConstants

	// IA에서 값이 들어가도록 설정한다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		3,
		slotRootParameter,
		0,
		nullptr,
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

void LightNMaterialApp::BuildShadersAndInputLayout()
{
#if PRAC6
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC6" , "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\05_Light&Material.hlsl", defines, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\05_Light&Material.hlsl", defines, "PS", "ps_5_1");
#else
	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\05_Light&Material.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\05_Light&Material.hlsl", nullptr, "PS", "ps_5_1");
#endif
	

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void LightNMaterialApp::BuildLandGeometry()
{
	// Grid를 만들고, 높이(y)를 바꿔주어서 지형을 만든다.
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData grid = geoGen.CreateGrid(160.f, 160.f, 50, 50);

	// x, z에 따라 y가 적절히 변하는 함수를 이용해서
	// Vertex의 y값을 바꿔주고, 노말도 넣어준다.
	std::vector<Vertex> vertices(grid.Vertices.size());

	for (size_t i = 0; i < grid.Vertices.size(); i++)
	{
		XMFLOAT3 p = grid.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Pos.y = GetHillsHeight(p.x, p.z);
		vertices[i].Normal = GetHillsNormal(p.x, p.z);
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = grid.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "LandGeo";

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

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	// 서브 메쉬 설정을 해준다.
	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	m_Geometries["LandGeo"] = std::move(geo);
}

void LightNMaterialApp::BuildWavesGeometryBuffers()
{
	// Wave 의 Vertex 정보는 Waves 클래스에서 만들었지만
	// index는 아직 안 만들었다.
	// 이제 만들어야 한다.
	std::vector<std::uint16_t> indices(3 * m_Waves->TriangleCount());
	// 16 비트 숫자보다 크면 안된다.
	assert(m_Waves->VertexCount() < 0x0000ffff);

	// 이전에 봤던 공식처럼 quad를 만든다.
	int Row = m_Waves->RowCount();
	int Col = m_Waves->ColumnCount();
	int k = 0;
	for (int i = 0; i < Row - 1; i++)
	{
		for (int j = 0; j < Col - 1; j++)
		{
			indices[k] = i * Col + j;
			indices[k + 1] = i * Col + j + 1;
			indices[k + 2] = (i + 1) * Col + j;

			indices[k + 3] = (i + 1) * Col + j;
			indices[k + 4] = i * Col + j + 1;
			indices[k + 5] = (i + 1) * Col + j + 1;

			k += 6; // 다음 사각형
		}
	}

	// 이제 버퍼를 만들어 준다.
	UINT vbByteSize = m_Waves->VertexCount() * sizeof(Vertex);
	UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "WaterGeo";

	// Dynamic Buffer여서 Blob이나 Default Buffer를 만들어도 안 쓰인다.
	geo->VertexBufferCPU = nullptr;
	geo->VertexBufferGPU = nullptr;

	// 하지만 Index는 바뀌지 않는다. 그래서 Default 버퍼로 만들어준다.
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	// Geometry 속성 정보를 넣어준다.
	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["grid"] = submesh;

	m_Geometries["WaterGeo"] = std::move(geo);
}

void LightNMaterialApp::BuildShapeGeometry()
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
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Normal = grid.Vertices[i].Normal;
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Normal = sphere.Vertices[i].Normal;
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Normal = cylinder.Vertices[i].Normal;
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

void LightNMaterialApp::BuildSkull()
{
	vector<Vertex> SkullVertices;
	vector<uint32_t> SkullIndices;

	Prac3::Prac3VerticesNIndicies(L"..\\04_RenderSmoothly_Wave\\Skull.txt", SkullVertices, SkullIndices);

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

void LightNMaterialApp::BuildMatForSkullNGeo()
{
	std::unique_ptr<Material> skull = std::make_unique<Material>();
	skull->Name = "skull";
	skull->MatCBIndex = 0;
	skull->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
	skull->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	skull->Roughness = 0.125f;

	std::unique_ptr<Material> box = std::make_unique<Material>();
	box->Name = "box";
	box->MatCBIndex = 1;
	box->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
	box->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	box->Roughness = 0.25f;

	std::unique_ptr<Material> grid = std::make_unique<Material>();
	grid->Name = "grid";
	grid->MatCBIndex = 2;
	grid->DiffuseAlbedo = XMFLOAT4(Colors::DarkGreen);
	grid->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grid->Roughness = 0.375f;

	std::unique_ptr<Material> sphere = std::make_unique<Material>();
	sphere->Name = "sphere";
	sphere->MatCBIndex = 3;
	sphere->DiffuseAlbedo = XMFLOAT4(Colors::Crimson);
	sphere->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	sphere->Roughness = 0.0f;

	std::unique_ptr<Material> cylinder = std::make_unique<Material>();
	cylinder->Name = "cylinder";
	cylinder->MatCBIndex = 4;
	cylinder->DiffuseAlbedo = XMFLOAT4(Colors::SteelBlue);
	cylinder->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	cylinder->Roughness = 0.75f;

	m_Materials["skull"] = std::move(skull);
	m_Materials["box"] = std::move(box);
	m_Materials["grid"] = std::move(grid);
	m_Materials["sphere"] = std::move(sphere);
	m_Materials["cylinder"] = std::move(cylinder);
}

void LightNMaterialApp::BuildRenderItemForSkullNGeo()
{
	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Geo = m_Geometries["shapeGeo"].get();
	boxRitem->Mat = m_Materials["box"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;

	m_RItemLayer[(int)RenderLayer::Opaque].push_back(boxRitem.get());
	m_AllRenderItems.push_back(std::move(boxRitem));

	// 그리드 아이템
	std::unique_ptr<RenderItem> gridRitem = std::make_unique<RenderItem>();

	gridRitem->WorldMat = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 2;
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->Mat = m_Materials["grid"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RItemLayer[(int)RenderLayer::Opaque].push_back(gridRitem.get());
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

#if PRAC4 || PRAC5
		m_SpherePosArray[i * 2] = { -5.0f, 3.5f, -10.0f + i * 5.0f };
		m_SpherePosArray[i * 2 + 1] = { +5.0f, 3.5f, -10.0f + i * 5.0f };
#endif
		XMMATRIX leftSphereWorld = XMMatrixTranslation(-5.0f, 3.5f, -10.0f + i * 5.0f);
		XMMATRIX rightSphereWorld = XMMatrixTranslation(+5.0f, 3.5f, -10.0f + i * 5.0f);

		XMStoreFloat4x4(&leftCylRitem->WorldMat, rightCylWorld);
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = m_Geometries["shapeGeo"].get();
		leftCylRitem->Mat = m_Materials["cylinder"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->WorldMat, leftCylWorld);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = m_Geometries["shapeGeo"].get();
		rightCylRitem->Mat = m_Materials["cylinder"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->WorldMat, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		leftSphereRitem->Mat = m_Materials["sphere"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->WorldMat, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->Mat = m_Materials["sphere"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		m_RItemLayer[(int)RenderLayer::Opaque].push_back(leftCylRitem.get());
		m_RItemLayer[(int)RenderLayer::Opaque].push_back(rightCylRitem.get());
		m_RItemLayer[(int)RenderLayer::Opaque].push_back(leftSphereRitem.get());
		m_RItemLayer[(int)RenderLayer::Opaque].push_back(rightSphereRitem.get());

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
	skullRenderItem->Mat = m_Materials["skull"].get();
	skullRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->IndexCount = skullRenderItem->Geo->DrawArgs["skull"].IndexCount;
	skullRenderItem->StartIndexLocation = skullRenderItem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRenderItem->BaseVertexLocation = skullRenderItem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_RItemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());

	m_AllRenderItems.push_back(std::move(skullRenderItem));
}

void LightNMaterialApp::BuildPSOs()
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
}

void LightNMaterialApp::BuildFrameResources()
{
	UINT waveVerCount = 0;
#if !PRAC3
	waveVerCount = m_Waves->VertexCount();
#endif

	for (int i = 0; i < g_NumFrameResources; ++i)
	{
		m_FrameResources.push_back(
			std::make_unique<FrameResource>(m_d3dDevice.Get(),
			1,
			(UINT)m_AllRenderItems.size(),
			(UINT)m_Materials.size(),
			waveVerCount
		));
	}
}

void LightNMaterialApp::BuildMaterials()
{
	// 일단 land에 씌울 grass Mat을 만든다.
	std::unique_ptr<Material> grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(0.2f, 0.6f, 0.2f, 1.f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// wave에 씌울 water Mat을 만든다.
	std::unique_ptr<Material> water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(0.0f, 0.2f, 0.6f, 1.f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.f;

	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
}

void LightNMaterialApp::BuildRenderItems()
{
	// 파도를 아이템화 시키기
	std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>();
	wavesRenderItem->WorldMat = MathHelper::Identity4x4();
	wavesRenderItem->ObjCBIndex = 0;
	// 아이템에 Mat을 추가시켜준다.
	wavesRenderItem->Mat = m_Materials["water"].get();
	wavesRenderItem->Geo = m_Geometries["WaterGeo"].get();
	wavesRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wavesRenderItem->IndexCount = wavesRenderItem->Geo->DrawArgs["grid"].IndexCount;
	wavesRenderItem->StartIndexLocation = wavesRenderItem->Geo->DrawArgs["grid"].StartIndexLocation;
	wavesRenderItem->BaseVertexLocation = wavesRenderItem->Geo->DrawArgs["grid"].BaseVertexLocation;
	// 업데이트를 위해 맴버로 관리해준다.
	m_WavesRenderItem = wavesRenderItem.get();
	m_RItemLayer[(int)RenderLayer::Opaque].push_back(wavesRenderItem.get());

	// 지형을 아이템화 시키기
	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
	gridRenderItem->WorldMat = MathHelper::Identity4x4();
	gridRenderItem->ObjCBIndex = 1;
	// 여기도 mat을 추가해준다.
	gridRenderItem->Mat = m_Materials["grass"].get();
	gridRenderItem->Geo = m_Geometries["LandGeo"].get();
	gridRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geo->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());

	m_AllRenderItems.push_back(std::move(wavesRenderItem));
	m_AllRenderItems.push_back(std::move(gridRenderItem));
}

void LightNMaterialApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems)
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
		_cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		// Object Constant와
		D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress();
		objCBAddress += ri->ObjCBIndex * objCBByteSize;
		// Material Constant를
		D3D12_GPU_VIRTUAL_ADDRESS matCBAddress = matCB->GetGPUVirtualAddress();
		matCBAddress += ri->Mat->MatCBIndex * matCBByteSize;

		// PSO에 연결해준다.
		_cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
		_cmdList->SetGraphicsRootConstantBufferView(1, matCBAddress);

		_cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

float LightNMaterialApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 LightNMaterialApp::GetHillsNormal(float _x, float _z) const
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
