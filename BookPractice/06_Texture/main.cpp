//***************************************************************************************
// CrateApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include "Waves.h"
#include "FileReader.h"

#define BOX (1)
#define WAVE (0)
#define SKULL (0)
#define MIPMAPTEST (0)
#define MULTITEX (1)

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

enum class RenderLayer : int
{
	Opaque = 0,
	Count
};

class TextureApp : public D3DApp
{
public:
	TextureApp(HINSTANCE hInstance);
	~TextureApp();

	TextureApp(const TextureApp& _other) = delete;
	TextureApp& operator=(const TextureApp& _other) = delete;

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

	// ==== Update ====
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMaterialCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);
	void UpdateMainPassCB_3PointLights(const GameTimer& _gt);
	void UpdateWaves(const GameTimer& _gt);
	void AnimateMaterials(const GameTimer& _gt);

	// 연습 문제
	void BuildSamplerHeap();
	void LoadFireTextures();

	// ==== Init ====
	void BuildRootSignature();
	// 다시 Descriptor Heap을 사용한다. 테이플을 사용하기 때문에
	void BuildShadersAndInputLayout();
	// Box 용이다.
	void LoadBoxTextures();
	void BuildBoxDescriptorHeaps();
	void BuildBoxGeometry();
	void BuildBoxMaterials();
	void BuildBoxRenderItems();

	// Wave 용이다.
	void LoadWaveTextures();
	void BuildWaveDescriptorHeaps();
	void BuildLandGeometry();
	void BuildFenceGeometry();
	void BuildWavesGeometryBuffers();
	void BuildWaveMaterials();
	void BuildWaveRenderItems();

	// Skull 용이다.
	void LoadSkullTextures();
	void BuildSkullDescriptorHeaps();
	void BuildShapeGeometry();
	void BuildSkull();
	void BuildMatForSkullNGeo();
	void BuildRenderItemForSkullNGeo();

	void BuildPSOs();
	void BuildFrameResources();

	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems);

	// 지형의 높이와 노멀을 계산하는 함수다.
	float GetHillsHeight(float _x, float _z) const;
	XMFLOAT3 GetHillsNormal(float _x, float _z)const;

	// 정적 샘플러 구조체를 미리 만드는 함수이다.
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> GetStaticSamplers();

private:
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	// 이제 Descriptor Heap 도 사용한다.
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_SrvDescriptorHeap = nullptr;

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
	// 파도의 높이를 업데이트 해주는 클래스 인스턴스다.
	std::unique_ptr<Waves> m_Waves;

	// Object 관계 없이, Render Pass 전체가 공유하는 값이다.
	PassConstants m_MainPassCB;

	XMFLOAT3 m_EyePos = { 0.f, 0.f, 0.f };
	XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	float m_Phi = 1.3f * XM_PI;
	float m_Theta = 0.4f * XM_PI;
#if BOX
	float m_Radius = 2.5f;
#elif SKULL
	float m_Radius = 20.f;
#elif WAVE
	float m_Radius = 50.f;
#endif
	bool m_bKeyLight = false;
	bool m_bfillLight = false;
	bool m_bBackLight = false;

	XMFLOAT3 m_SpherePosArray[10] = {};

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
		TextureApp theApp(hInstance);
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

TextureApp::TextureApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

TextureApp::~TextureApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool TextureApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	// 얘는 좀 복잡해서 따로 클래스로 만들었다.
	m_Waves = std::make_unique<Waves>(128, 128, 1.f, 0.03f, 4.f, 0.2f);


	BuildShadersAndInputLayout();

#if MIPMAPTEST
	BuildSamplerHeap();
#endif

#if MULTITEX && BOX
	LoadFireTextures();
#elif BOX
	LoadBoxTextures();
#endif

#if BOX
	BuildRootSignature();
	BuildBoxDescriptorHeaps();
	BuildBoxGeometry();
	BuildBoxMaterials();
	BuildBoxRenderItems();
#elif SKULL
	LoadSkullTextures();
	BuildSkullDescriptorHeaps();
	BuildRootSignature();
	BuildShapeGeometry();
	BuildSkull();
	BuildMatForSkullNGeo();
	BuildRenderItemForSkullNGeo();
#elif WAVE
	LoadWaveTextures();
	BuildRootSignature();
	BuildWaveDescriptorHeaps();
	// 지형을 만들고
	BuildLandGeometry();
	// 그 버퍼를 만들고
	BuildWavesGeometryBuffers();
	BuildFenceGeometry();
	// 지형과 파도에 맞는 머테리얼을 만든다.
	BuildWaveMaterials();
	// 아이템 화를 시킨다.
	BuildWaveRenderItems();
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

void TextureApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 0.01f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void TextureApp::Update(const GameTimer& _gt)
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

	// 파도를 업데이트 해준다.
#if WAVE
	UpdateWaves(_gt);
#endif
}

void TextureApp::Draw(const GameTimer& _gt)
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
	// ==== 그리기 ======

	// 현재 PSO에 View Heap을 세팅해준다.
#if MIPMAPTEST
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get(), m_SamplerHeap.Get()};
#else
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
#endif
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// PSO에 Root Signature를 세팅해준다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// 현재 Frame Constant Buffer를 업데이트한다.
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass는 2번

	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

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

void TextureApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void TextureApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void TextureApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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
		m_Radius = MathHelper::Clamp(m_Radius, 1.f, 15.f);
	}

	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void TextureApp::OnKeyboardInput(const GameTimer _gt)
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
	if (GetAsyncKeyState('5') & 0x8000)
	{
		m_Phi = -XM_PIDIV2;
		m_Theta = XM_PIDIV2;
	}

	m_SunTheta = MathHelper::Clamp(m_SunTheta, 0.1f, XM_PIDIV2);
}

void TextureApp::UpdateCamera(const GameTimer& _gt)
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

void TextureApp::UpdateObjectCBs(const GameTimer& _gt)
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
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));
			// Texture Tile이나, Animation을 위한 Transform도 같이 넣어준다.
			
			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void TextureApp::UpdateMaterialCBs(const GameTimer& _gt)
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

void TextureApp::UpdateMainPassCB(const GameTimer& _gt)
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

	// 이제 Light를 채워준다. Shader와 App에서 정의한 Lights의 개수와 종류가 같아야 한다.

#if 0
	m_MainPassCB.AmbientLight = { 0.25f, 0.25f, 0.35f, 1.f };
	float sinWave = 0.5f * sinf(_gt.GetTotalTime()) + 0.5f;
	m_MainPassCB.Lights[0].Strength = { 1.f * sinWave, 1.f * sinWave, 0.9f * sinWave };
#elif 1
	m_MainPassCB.AmbientLight = { 0.125f, 0.125f, 0.175f, 1.f };
	UpdateMainPassCB_3PointLights(_gt);
#elif 0
	m_MainPassCB.AmbientLight = { 0.125f, 0.125f, 0.175f, 1.f };
	for (int i = 0; i < 10; i++)
	{
		m_MainPassCB.Lights[i].Strength = { 0.75f, 0.75f, 0.75f };
		// Point light 맴버를 채운다.
		m_MainPassCB.Lights[i].Position = m_SpherePosArray[i];
		m_MainPassCB.Lights[i].FalloffStart = 1.f;
		m_MainPassCB.Lights[i].FalloffEnd = 10.f;
	}
#elif 0
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

void TextureApp::UpdateMainPassCB_3PointLights(const GameTimer& _gt)
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

void TextureApp::UpdateWaves(const GameTimer& _gt)
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
		
		// 얘는 좌측 위가 0,0이고 아래로 내려오니까
		// 좌표 변환을 시켜줘야 한다.
		v.TexC.x = 0.5f + v.Pos.x / m_Waves->Width();
		v.TexC.y = 0.5f - v.Pos.z / m_Waves->Depth();


		currWaveVB->CopyData(i, v);
	}
	// 그리고 그걸 Buffer에 업데이트 해준다.
	m_WavesRenderItem->Geo->VertexBufferGPU = currWaveVB->Resource();
}

void TextureApp::AnimateMaterials(const GameTimer& _gt)
{
#if WAVE
	Material* waterMat = m_Materials["water"].get();

	// 3행 0열에 있는걸 가져온다.
	float tu = waterMat->MatTransform(3, 0);
	float tv = waterMat->MatTransform(3, 1);
	// 이 위치는 Transform Matrix에서 Translate.xy 성분에 해당한다.
	tu += 0.1f * _gt.GetDeltaTime();
	tv += 0.02f * _gt.GetDeltaTime();

	if (tu >= 1.f)
	{
		tu -= 1.f;
	}
	if (tv >= 1.f)
	{
		tv -= 1.f;
	}

	waterMat->MatTransform(3, 0) = tu;
	waterMat->MatTransform(3, 1) = tv;
	// dirty flag를 켜준다.
	waterMat->NumFramesDirty = g_NumFrameResources;
#elif MULTITEX
	Material* flareMat = m_Materials["woodCrate"].get(); 
	XMMATRIX flareTexTransform = XMMatrixTranslation(-0.5f, -0.5f, 0.f);
	flareTexTransform *= XMMatrixRotationZ(_gt.GetTotalTime());
	flareTexTransform *= XMMatrixTranslation(0.5f, 0.5f, 0.f);

	XMStoreFloat4x4(&flareMat->MatTransform, flareTexTransform);

	flareMat->NumFramesDirty = g_NumFrameResources;
#endif
}

void TextureApp::BuildSamplerHeap()
{
	// 샘플러 뷰 힙을 만든다.
	D3D12_DESCRIPTOR_HEAP_DESC descHeapSampler = {};
	descHeapSampler.NumDescriptors = 3;
	descHeapSampler.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	descHeapSampler.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&descHeapSampler, IID_PPV_ARGS(&m_SamplerHeap)));

	// 샘플러를 만든다.
	UINT samplerIncrement = m_d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);
	CD3DX12_CPU_DESCRIPTOR_HANDLE samHeapHandle(m_SamplerHeap->GetCPUDescriptorHandleForHeapStart());
	
	// s10
	D3D12_SAMPLER_DESC pointWrapLOD3 = {};
	pointWrapLOD3.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	pointWrapLOD3.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pointWrapLOD3.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pointWrapLOD3.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pointWrapLOD3.MinLOD = 3;
	pointWrapLOD3.MaxLOD = D3D12_FLOAT32_MAX;
	pointWrapLOD3.MipLODBias = 0.f;
	pointWrapLOD3.MaxAnisotropy = 8;
	pointWrapLOD3.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	m_d3dDevice->CreateSampler(&pointWrapLOD3, samHeapHandle);

	// s11
	D3D12_SAMPLER_DESC linearWrapLOD3 = pointWrapLOD3;
	linearWrapLOD3.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;

	samHeapHandle.Offset(1, samplerIncrement);
	m_d3dDevice->CreateSampler(&linearWrapLOD3, samHeapHandle);

	// s12
	D3D12_SAMPLER_DESC anisotropicWrapLOD3 = linearWrapLOD3;
	anisotropicWrapLOD3.Filter = D3D12_FILTER_ANISOTROPIC;

	samHeapHandle.Offset(1, samplerIncrement);
	m_d3dDevice->CreateSampler(&anisotropicWrapLOD3, samHeapHandle);

}

void TextureApp::LoadFireTextures()
{
	std::unique_ptr<Texture> flareTex = std::make_unique<Texture>();
	flareTex->Name = "flareTex";
	flareTex->Filename = L"../Textures/flare.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		flareTex->Filename.c_str(),
		flareTex->Resource,
		flareTex->UploadHeap
	));

	m_Textures[flareTex->Name] = std::move(flareTex);

	std::unique_ptr<Texture> flareAlphaTex = std::make_unique<Texture>();
	flareAlphaTex->Name = "flareAlphaTex";
	flareAlphaTex->Filename = L"../Textures/flarealpha.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		flareAlphaTex->Filename.c_str(),
		flareAlphaTex->Resource,
		flareAlphaTex->UploadHeap
	));

	m_Textures[flareAlphaTex->Name] = std::move(flareAlphaTex);
}

void TextureApp::BuildRootSignature()
{
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[5];
	
#if MIPMAPTEST
	// Sampler 정보가 넘어가는 테이블
	UINT numOfParameter = 5;
	CD3DX12_DESCRIPTOR_RANGE samTable;
	samTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER, 3, 10); // s10 ~ s12 - Lod3 Sampler 정보
	slotRootParameter[4].InitAsDescriptorTable(1, &samTable, D3D12_SHADER_VISIBILITY_PIXEL);
#else
	UINT numOfParameter = 4; 
#endif
	// Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable;
#if MULTITEX
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // t0 ~ t1 - texture 정보
#else
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0); // t0 - texture 정보
#endif
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
	// Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[1].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[2].InitAsConstantBufferView(1); // b1 - PassConstants
	slotRootParameter[3].InitAsConstantBufferView(2); // b2 - Material

	// DirectX에서 제공하는 Heap을 만들지 않고, Sampler를 생성할 수 있게 해주는
	// Static Sampler 방법이다. 최대 2000개 생성 가능
	// (원래는 D3D12_DESCRIPTOR_HEAP_DESC 가지고 D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER막 해줘야 한다.)
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> staticSamplers = GetStaticSamplers();

	// IA에서 값이 들어가도록 설정한다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		numOfParameter, // table 1개, constant view 3개 (+ Sampler 1개)
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

void TextureApp::BuildBoxDescriptorHeaps()
{
	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 2;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
#if MULTITEX
	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* flareTex = m_Textures["flareTex"].get()->Resource.Get();
	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = flareTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = flareTex->GetDesc().MipLevels;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(flareTex, &srcDesc, viewHandle);

	ID3D12Resource* flareAlphaTex = m_Textures["flareAlphaTex"].get()->Resource.Get();
	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = flareAlphaTex->GetDesc().Format;
	m_d3dDevice->CreateShaderResourceView(flareAlphaTex, &srcDesc, viewHandle);

#else
	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* woodCrateTex = m_Textures["woodCrateTex"].get()->Resource.Get();
	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = woodCrateTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = woodCrateTex->GetDesc().MipLevels;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	// view를 만들면서 연결해준다.
	m_d3dDevice->CreateShaderResourceView(woodCrateTex, &srcDesc, viewHandle);
#endif
}

void TextureApp::BuildShadersAndInputLayout()
{
#if MULTITEX 
	const D3D_SHADER_MACRO defines[] =
	{
		"MULTITEX" , "1",
		NULL, NULL
	};

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\06_Texture.hlsl", defines, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\06_Texture.hlsl", defines, "PS", "ps_5_1");
#else
	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\06_Texture.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\06_Texture.hlsl", nullptr, "PS", "ps_5_1");

#endif

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) *2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void TextureApp::LoadBoxTextures()
{
	std::unique_ptr<Texture> woodCrateTex = std::make_unique<Texture>();
	woodCrateTex->Name = "woodCrateTex";
	woodCrateTex->Filename = L"../Textures/WoodCrate01.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		woodCrateTex->Filename.c_str(),
		woodCrateTex->Resource,
		woodCrateTex->UploadHeap
	));

	m_Textures[woodCrateTex->Name] = std::move(woodCrateTex);
}

void TextureApp::BuildBoxGeometry()
{
	// 일단 제너레이터로 박스를 만들고
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(1.f, 1.f, 1.f, 3);
	// 속성도 뽑아 먹고
	SubmeshGeometry boxSubmesh;
	boxSubmesh.IndexCount = (UINT)box.Indices32.size();
	boxSubmesh.StartIndexLocation = 0;
	boxSubmesh.BaseVertexLocation = 0;
	// 데이터도 뽑아 먹는다.
	std::vector<Vertex> vertices(box.Vertices.size());

	for (size_t i = 0; i < box.Vertices.size(); i++)
	{
		vertices[i].Pos = box.Vertices[i].Position;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = box.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
	geo->Name = "boxGeo";

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

	m_Geometries[geo->Name] = std::move(geo);
}

void TextureApp::BuildBoxMaterials()
{
	std::unique_ptr<Material> woodCrate = std::make_unique<Material>();
	woodCrate->Name = "woodCrate";
	woodCrate->MatCBIndex = 0;
	woodCrate->DiffuseSrvHeapIndex = 0;
	woodCrate->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	woodCrate->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
	woodCrate->Roughness = 0.2f;

	m_Materials[woodCrate->Name] = std::move(woodCrate);
}

void TextureApp::BuildBoxRenderItems()
{
	std::unique_ptr<RenderItem> boxRenderItem = std::make_unique<RenderItem>();

	boxRenderItem->ObjCBIndex = 0;
	XMMATRIX texTransform = XMMatrixIdentity();
	//texTransform = XMMatrixScaling(3.f, 3.f, 1.f) * XMMatrixTranslation(-0.7f, -0.7f, 0.f);
	XMStoreFloat4x4(&boxRenderItem->TexTransform, texTransform);
	boxRenderItem->Mat = m_Materials["woodCrate"].get();
	boxRenderItem->Geo = m_Geometries["boxGeo"].get();
	boxRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRenderItem->IndexCount = boxRenderItem->Geo->DrawArgs["box"].IndexCount;
	boxRenderItem->StartIndexLocation = boxRenderItem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRenderItem->BaseVertexLocation = boxRenderItem->Geo->DrawArgs["box"].BaseVertexLocation;

	m_AllRenderItems.push_back(std::move(boxRenderItem));

	// 일단은 opaque 한 친구들 말고는 없다.
	for (std::unique_ptr<RenderItem>& e : m_AllRenderItems)
	{
		m_RenderItemLayer[(UINT)RenderLayer::Opaque].push_back(e.get());
	}
}

void TextureApp::LoadWaveTextures()
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

	std::unique_ptr<Texture> fenceTex = std::make_unique<Texture>();
	fenceTex->Name = "fenceTex";
	fenceTex->Filename = L"../Textures/WoodCrate01.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		fenceTex->Filename.c_str(),
		fenceTex->Resource,
		fenceTex->UploadHeap
	));

	m_Textures[grassTex->Name] = std::move(grassTex);
	m_Textures[waterTex->Name] = std::move(waterTex);
	m_Textures[fenceTex->Name] = std::move(fenceTex);
}

void TextureApp::BuildWaveDescriptorHeaps()
{
	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 3;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* grassTex = m_Textures["grassTex"].get()->Resource.Get();
	ID3D12Resource* waterTex = m_Textures["waterTex"].get()->Resource.Get();
	ID3D12Resource* fenceTex = m_Textures["fenceTex"].get()->Resource.Get();


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
}

void TextureApp::BuildLandGeometry()
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
		vertices[i].TexC = grid.Vertices[i].TexC;
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

void TextureApp::BuildFenceGeometry()
{
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData box = geoGen.CreateBox(8.0f, 8.0f, 8.0f, 3);

	std::vector<Vertex> vertices(box.Vertices.size());
	for (size_t i = 0; i < box.Vertices.size(); ++i)
	{
		auto& p = box.Vertices[i].Position;
		vertices[i].Pos = p;
		vertices[i].Normal = box.Vertices[i].Normal;
		vertices[i].TexC = box.Vertices[i].TexC;
	}

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);

	std::vector<std::uint16_t> indices = box.GetIndices16();
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "fenceGeo";

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

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["fence"] = submesh;

	m_Geometries["fenceGeo"] = std::move(geo);
}

void TextureApp::BuildWavesGeometryBuffers()
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

void TextureApp::LoadSkullTextures()
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
}

void TextureApp::BuildSkullDescriptorHeaps()
{
	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 4;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* skullTex = m_Textures["skullTex"].get()->Resource.Get();
	ID3D12Resource* bricksTex = m_Textures["bricksTex"].get()->Resource.Get();
	ID3D12Resource* stoneTex = m_Textures["stoneTex"].get()->Resource.Get();
	ID3D12Resource* tileTex = m_Textures["tileTex"].get()->Resource.Get();


	// view를 만들어주고
	D3D12_SHADER_RESOURCE_VIEW_DESC srcDesc = {};
	srcDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srcDesc.Format = skullTex->GetDesc().Format;
	srcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srcDesc.Texture2D.MostDetailedMip = 0;
	srcDesc.Texture2D.MipLevels = skullTex->GetDesc().MipLevels;
	srcDesc.Texture2D.ResourceMinLODClamp = 0.f;

	m_d3dDevice->CreateShaderResourceView(skullTex, &srcDesc, viewHandle);

	// view를 만들면서 연결해준다.
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
}

void TextureApp::BuildShapeGeometry()
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

void TextureApp::BuildSkull()
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

void TextureApp::BuildMatForSkullNGeo()
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

	m_Materials["skullMat"] = std::move(skullMat);
	m_Materials["brickMat"] = std::move(brickMat);
	m_Materials["stoneMat"] = std::move(stoneMat);
	m_Materials["tileMat"] = std::move(tileMat);
}

void TextureApp::BuildRenderItemForSkullNGeo()
{
	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	XMStoreFloat4x4(&boxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
	boxRitem->ObjCBIndex = 1;
	boxRitem->Geo = m_Geometries["shapeGeo"].get();
	boxRitem->Mat = m_Materials["stoneMat"].get();
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

#if 0
		m_SpherePosArray[i * 2] = { -5.0f, 3.5f, -10.0f + i * 5.0f };
		m_SpherePosArray[i * 2 + 1] = { +5.0f, 3.5f, -10.0f + i * 5.0f };
#endif
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
		leftSphereRitem->Mat = m_Materials["stoneMat"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->WorldMat, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->Mat = m_Materials["stoneMat"].get();
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
	skullRenderItem->Mat = m_Materials["skullMat"].get();
	skullRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	skullRenderItem->IndexCount = skullRenderItem->Geo->DrawArgs["skull"].IndexCount;
	skullRenderItem->StartIndexLocation = skullRenderItem->Geo->DrawArgs["skull"].StartIndexLocation;
	skullRenderItem->BaseVertexLocation = skullRenderItem->Geo->DrawArgs["skull"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(skullRenderItem.get());

	m_AllRenderItems.push_back(std::move(skullRenderItem));
}

void TextureApp::BuildPSOs()
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

void TextureApp::BuildFrameResources()
{
	UINT waveVerCount = 0;
#if WAVE
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

void TextureApp::BuildWaveMaterials()
{
	// 일단 land에 씌울 grass Mat을 만든다.
	std::unique_ptr<Material> grass = std::make_unique<Material>();
	grass->Name = "grass";
	grass->MatCBIndex = 0;
	grass->DiffuseSrvHeapIndex = 0;
	grass->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	grass->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	grass->Roughness = 0.125f;

	// wave에 씌울 water Mat을 만든다.
	std::unique_ptr<Material> water = std::make_unique<Material>();
	water->Name = "water";
	water->MatCBIndex = 1;
	water->DiffuseSrvHeapIndex = 1;
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	water->FresnelR0 = XMFLOAT3(0.2f, 0.2f, 0.2f);
	water->Roughness = 0.f;

	std::unique_ptr<Material> fence = std::make_unique<Material>();
	fence->Name = "fence";
	fence->MatCBIndex = 2;
	fence->DiffuseSrvHeapIndex = 2;
	fence->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	fence->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	fence->Roughness = 0.25f;


	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
	m_Materials["fence"] = std::move(fence);
}

void TextureApp::BuildWaveRenderItems()
{
	// 파도를 아이템화 시키기
	std::unique_ptr<RenderItem> wavesRenderItem = std::make_unique<RenderItem>();
	wavesRenderItem->WorldMat = MathHelper::Identity4x4();
	XMStoreFloat4x4(&wavesRenderItem->TexTransform, XMMatrixScaling(5.0f, 5.0f, 1.0f));
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
	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(wavesRenderItem.get());

	// 지형을 아이템화 시키기
	std::unique_ptr<RenderItem> gridRenderItem = std::make_unique<RenderItem>();
	gridRenderItem->WorldMat = MathHelper::Identity4x4();
	XMStoreFloat4x4(&gridRenderItem->TexTransform, XMMatrixScaling(5.f, 5.f, 1.f));
	gridRenderItem->ObjCBIndex = 1;
	// 여기도 mat을 추가해준다.
	gridRenderItem->Mat = m_Materials["grass"].get();
	gridRenderItem->Geo = m_Geometries["LandGeo"].get();
	gridRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRenderItem->IndexCount = gridRenderItem->Geo->DrawArgs["grid"].IndexCount;
	gridRenderItem->StartIndexLocation = gridRenderItem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRenderItem->BaseVertexLocation = gridRenderItem->Geo->DrawArgs["grid"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());

	std::unique_ptr<RenderItem> fenceRenderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&fenceRenderItem->WorldMat, XMMatrixTranslation(3.f, 2.f, -9.f));
	fenceRenderItem->ObjCBIndex = 2;
	fenceRenderItem->Mat = m_Materials["fence"].get();
	fenceRenderItem->Geo = m_Geometries["fenceGeo"].get();
	fenceRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fenceRenderItem->IndexCount = fenceRenderItem->Geo->DrawArgs["fence"].IndexCount;
	fenceRenderItem->StartIndexLocation = fenceRenderItem->Geo->DrawArgs["fence"].StartIndexLocation;
	fenceRenderItem->BaseVertexLocation = fenceRenderItem->Geo->DrawArgs["fence"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(fenceRenderItem.get());

	m_AllRenderItems.push_back(std::move(wavesRenderItem));
	m_AllRenderItems.push_back(std::move(gridRenderItem));
	m_AllRenderItems.push_back(std::move(fenceRenderItem));
}

void TextureApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems)
{
	// 이제 Material도 물체마다 업데이트 해줘야 한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();
	ID3D12Resource* matCB = m_CurrFrameResource->MaterialCB->Resource();

#if MIPMAPTEST
	CD3DX12_GPU_DESCRIPTOR_HANDLE sam(m_SamplerHeap->GetGPUDescriptorHandleForHeapStart());
	_cmdList->SetGraphicsRootDescriptorTable(4, sam);
#endif

	for (size_t i = 0; i < _renderItems.size(); i++)
	{
		RenderItem* ri = _renderItems[i];
		
		// 일단 Vertex, Index를 IA에 세팅해주고
		D3D12_VERTEX_BUFFER_VIEW VBView = ri->Geo->VertexBufferView();
		_cmdList->IASetVertexBuffers(0, 1, &VBView);
		D3D12_INDEX_BUFFER_VIEW IBView = ri->Geo->IndexBufferView();
		_cmdList->IASetIndexBuffer(&IBView);
		_cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(m_SrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
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

float TextureApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 TextureApp::GetHillsNormal(float _x, float _z) const
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> TextureApp::GetStaticSamplers()
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
