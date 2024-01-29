//***************************************************************************************
// GeoShaderApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************


#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include "Waves.h"
#include "FileReader.h"
#include <iomanip>

#define WAVE (0)
#define PRAC1 (0 && !WAVE)
#define PRAC2 (1 && !WAVE && !PRAC1)

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
	Mirrors,
	Reflected,
	Transparent,
	AlphaTested,
	AlphaTestedSprites,
	Shadow,
	Practice,
	Count
};

class GeoShaderApp : public D3DApp
{
public:
	GeoShaderApp(HINSTANCE hInstance);
	~GeoShaderApp();

	GeoShaderApp(const GeoShaderApp& _other) = delete;
	GeoShaderApp& operator=(const GeoShaderApp& _other) = delete;

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

	// ==== Update ====
	void UpdateCamera(const GameTimer& _gt);
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMaterialCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);
	void UpdateReflectedPassCB(const GameTimer& _gt);

	void UpdateWaves(const GameTimer& _gt);
	void AnimateMaterials(const GameTimer& _gt);

	// 연습 문제
	void BuildSamplerHeap();

	// ==== Init ====
	void BuildRootSignature();
	// 다시 Descriptor Heap을 사용한다. 테이플을 사용하기 때문에
	void BuildShadersAndInputLayout();

	// Wave 용이다.
	void LoadTextures();
	void BuildDescriptorHeaps();
	void BuildLandGeometry();
	void BuildFenceGeometry();
	void BuildWavesGeometryBuffers();
	void BuildTreeSpritesGeometry();
	void BuildMaterials();
	void BuildRenderItems();

	// Practice
	void Practice1();
	void Practice2();

	void BuildPSOs();
	void BuildFrameResources();

	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems);
	void DrawComplexityQuad(ID3D12GraphicsCommandList* _cmdList, int _StencilRef);

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
	// Sprite Layout은 다르기 때문에 따로 가지고 있는다.
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_TreeSpriteInputLayout;

	// practice 1
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_PracticeInputLayout;

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
	PassConstants m_ReflectedPassCB;

	XMFLOAT3 m_EyePos = { 0.f, 0.f, 0.f };
	XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	float m_Phi = 1.24f * XM_PI;
	float m_Theta = 0.42f * XM_PI;
#if !WAVE
	float m_Radius = 10.f;
#else
	float m_Radius = 50.f;
#endif

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
		GeoShaderApp theApp(hInstance);
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

GeoShaderApp::GeoShaderApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

GeoShaderApp::~GeoShaderApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool GeoShaderApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

#if PRAC1
	Practice1();
#elif PRAC2
	Practice2();
#else
	// 얘는 좀 복잡해서 따로 클래스로 만들었다.
	m_Waves = std::make_unique<Waves>(128, 128, 1.f, 0.03f, 4.f, 0.2f);

	BuildShadersAndInputLayout();
	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();
	// 지형을 만들고
	BuildLandGeometry();
	// 그 버퍼를 만들고
	BuildWavesGeometryBuffers();
	BuildFenceGeometry();

	BuildTreeSpritesGeometry();
	// 지형과 파도에 맞는 머테리얼을 만든다.
	BuildMaterials();
	// 아이템 화를 시킨다.
	BuildRenderItems();

	// 이제 FrameResource를 만들고
	BuildFrameResources();
	// 이제 PSO를 만들어서, 돌리면서 렌더링할 준비를 한다.
	BuildPSOs();
#endif

	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void GeoShaderApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 0.01f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void GeoShaderApp::Update(const GameTimer& _gt)
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
	UpdateWaves(_gt);
}

void GeoShaderApp::Draw(const GameTimer& _gt)
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
		(float*)&m_MainPassCB.FogColor,  // 근데 여기서 좀더 사실감을 살리기 위해 배경을 fog 색으로 채운다.
		0, 
		nullptr);
	// 뎁스를 1.f 스텐실을 0으로 초기화 한다.
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
	// 
	// 현재 PSO에 View Heap을 세팅해준다.
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_SrvDescriptorHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// PSO에 Root Signature를 세팅해준다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// 현재 Frame Constant Buffer를 세팅해준다..
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass는 2번

#if PRAC1
	m_CommandList->SetPipelineState(m_PSOs["prac1PSO"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Practice]);
#elif PRAC2

	m_CommandList->SetPipelineState(m_PSOs["prac2PSO"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Practice]);
#else
	// 일단 불투명한 애들을 먼저 싹 출력을 해준다.
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 알파 자르기하는 친구들을 출력해주고
	m_CommandList->SetPipelineState(m_PSOs["alphaTested"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::AlphaTested]);

	// Sprite를 출력해주고,
	m_CommandList->SetPipelineState(m_PSOs["treeSprites"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::AlphaTestedSprites]);

	// 이제 반투명한 애들을 출력해준다.
	m_CommandList->SetPipelineState(m_PSOs["transparent"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Transparent]);
#endif

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

void GeoShaderApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void GeoShaderApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void GeoShaderApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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


		UploadBuffer<ObjectConstants>* currObjectCB = m_CurrFrameResource->ObjectCB.get();
		for (auto e : m_RenderItemLayer[(int)RenderLayer::Practice])
		{
			e->NumFrameDirty = g_NumFrameResources;
		}
	}

	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
}

void GeoShaderApp::OnKeyboardInput(const GameTimer _gt)
{

}

void GeoShaderApp::UpdateCamera(const GameTimer& _gt)
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

void GeoShaderApp::UpdateObjectCBs(const GameTimer& _gt)
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
			XMStoreFloat4x4(&objConstants.TexTransform, XMMatrixTranspose(texTransform));

			XMVECTOR subVecfromEye = XMVectorSet(
				objConstants.WorldMat._41 - m_EyePos.x,
				objConstants.WorldMat._42 - m_EyePos.y, 
				objConstants.WorldMat._43 - m_EyePos.z, 
				0.f);

			float distFromEye = XMVector3Length(subVecfromEye).m128_f32[0];

			if (distFromEye < 15)
			{
				objConstants.Detail = 2;
				// 등차 수열합
				//objConstants.NumOfGeneratedVertices = (UINT)((exp2(2) + 1) * (exp2(1) +1));
				//objConstants.IndexOffset = (UINT)((exp2(2)));
			}
			else if (distFromEye < 30)
			{
				objConstants.Detail = 1;
				//objConstants.NumOfGeneratedVertices = 6;
				//objConstants.IndexOffset = 2;
			}
			else
			{
				objConstants.Detail = 0;
				//objConstants.NumOfGeneratedVertices = 3;
				//objConstants.IndexOffset = 1;
			}

			// Texture Tile이나, Animation을 위한 Transform도 같이 넣어준다.
			
			// CB에 넣어주고
			currObjectCB->CopyData(e->ObjCBIndex, objConstants);

			// 다음 FrameResource에서 업데이트를 하도록 설정한다.
			e->NumFrameDirty--;
		}
	}
}

void GeoShaderApp::UpdateMaterialCBs(const GameTimer& _gt)
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

void GeoShaderApp::UpdateMainPassCB(const GameTimer& _gt)
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

void GeoShaderApp::UpdateReflectedPassCB(const GameTimer& _gt)
{
	// 거울에 그려줄 친구가 쓰는 PassCB이다.
	m_ReflectedPassCB = m_MainPassCB;

	XMVECTOR mirrorPlane = XMVectorSet(0.f, 0.f, 1.f, 0.f);
	XMMATRIX R = XMMatrixReflect(mirrorPlane);

	// 반사된 조명을 구한다.
	for (int i = 0; i < 3; i++)
	{
		// Directional Light이기 때문에 direction만 바꿔준다.
		XMVECTOR lightDir = XMLoadFloat3(&m_MainPassCB.Lights[i].Direction);
		XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
		XMStoreFloat3(&m_ReflectedPassCB.Lights[i].Direction, reflectedLightDir);
	}
	// 얘는 Buffer index 1번에 저장한다.
	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, m_ReflectedPassCB);
}

void GeoShaderApp::UpdateWaves(const GameTimer& _gt)
{
#if WAVE
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
#endif
}

void GeoShaderApp::AnimateMaterials(const GameTimer& _gt)
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
#endif
}

void GeoShaderApp::BuildSamplerHeap()
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

void GeoShaderApp::BuildRootSignature()
{	
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	UINT numOfParameter = 4; 

	// Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable;

	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0); // t0 - texture 정보
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

void GeoShaderApp::BuildShadersAndInputLayout()
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

	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\09_GeometryShader.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\09_GeometryShader.hlsl", defines, "PS", "ps_5_1");
	m_Shaders["alphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\09_GeometryShader.hlsl", alphaTestDefines, "PS", "ps_5_1");

	// GS도 비슷하게 인트로를 정해주고 컴파일을 하면 된다.
	m_Shaders["treeSpriteVS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["treeSpriteGS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", nullptr, "GS", "gs_5_1");
	m_Shaders["treeSpritePS"] = d3dUtil::CompileShader(L"Shaders\\TreeSprite.hlsl", alphaTestDefines, "PS", "ps_5_1");


	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) *2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	// Sprite는 layout을 따로 가진다.
	m_TreeSpriteInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"SIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void GeoShaderApp::LoadTextures()
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
	fenceTex->Filename = L"../Textures/WireFence.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		fenceTex->Filename.c_str(),
		fenceTex->Resource,
		fenceTex->UploadHeap
	));

	std::unique_ptr<Texture> treeArrayTex = std::make_unique<Texture>();
	treeArrayTex->Name = "treeArrayTex";
	treeArrayTex->Filename = L"../Textures/treeArray2.dds";
	ThrowIfFailed(CreateDDSTextureFromFile12(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		treeArrayTex->Filename.c_str(),
		treeArrayTex->Resource,
		treeArrayTex->UploadHeap
	));

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

	m_Textures[fenceTex->Name] = std::move(fenceTex);
	m_Textures[grassTex->Name] = std::move(grassTex);
	m_Textures[waterTex->Name] = std::move(waterTex);
	m_Textures[treeArrayTex->Name] = std::move(treeArrayTex);
}

void GeoShaderApp::BuildDescriptorHeaps()
{
	// Texture는 Shader Resource View Heap을 사용한다. (SRV Heap)
	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 5;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_SrvDescriptorHeap)));

	// 이제 텍스쳐를 View로 만들면서 Heap과 연결해준다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE viewHandle(m_SrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	// 텍스쳐 리소스를 얻은 다음
	ID3D12Resource* grassTex = m_Textures["grassTex"].get()->Resource.Get();
	ID3D12Resource* waterTex = m_Textures["waterTex"].get()->Resource.Get();
	ID3D12Resource* fenceTex = m_Textures["fenceTex"].get()->Resource.Get();
	ID3D12Resource* treeArrayTex = m_Textures["treeArrayTex"].get()->Resource.Get();
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

	// Texture2DArray는 Desc가 다르다.
	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	D3D12_SHADER_RESOURCE_VIEW_DESC arrSrcDesc = srcDesc;
	arrSrcDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
	arrSrcDesc.Format = treeArrayTex->GetDesc().Format;
	arrSrcDesc.Texture2DArray.MostDetailedMip = 0;
	arrSrcDesc.Texture2DArray.MipLevels = -1;
	arrSrcDesc.Texture2DArray.FirstArraySlice = 0;
	arrSrcDesc.Texture2DArray.ArraySize = treeArrayTex->GetDesc().DepthOrArraySize;
	m_d3dDevice->CreateShaderResourceView(treeArrayTex, &arrSrcDesc, viewHandle);

	viewHandle.Offset(1, m_CbvSrvUavDescriptorSize);
	srcDesc.Format = whiteTex->GetDesc().Format;
	m_d3dDevice->CreateShaderResourceView(whiteTex, &srcDesc, viewHandle);

}

void GeoShaderApp::BuildLandGeometry()
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

void GeoShaderApp::BuildFenceGeometry()
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

void GeoShaderApp::BuildWavesGeometryBuffers()
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

void GeoShaderApp::BuildTreeSpritesGeometry()
{
	// 각종 계산은 GPU에서 해주기 때문에
	// 이렇게 로컬하게 구조체를 정의해줘도 된다.
	struct TreeSpriteVertex
	{
		XMFLOAT3 Pos;
		XMFLOAT2 Size;
	};

	static const int treeCount = 16;
	std::array<TreeSpriteVertex, 16> vertices;
	for (UINT i = 0; i < treeCount; i++)
	{
		// 랜덤으로 트리의 위치를 정하고
		float x = MathHelper::RandF(-45.f, 45.f);
		float z = MathHelper::RandF(-45.f, 45.f);
		float y = GetHillsHeight(x, z);
		// 살짝 띄운다.
		y += 8.f;
		
		vertices[i].Pos = XMFLOAT3(x, y, z);
		vertices[i].Size = XMFLOAT2(20.f, 20.f);
	}
	
	// 어처피 quad는 shader에서 만들어줄 것이기 때문에 주르륵 넘긴다.
	std::array<std::uint16_t, 16> indices = { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(TreeSpriteVertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "treeSpritesGeo";

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
		ibByteSize, geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(TreeSpriteVertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	m_Geometries["treeSpritesGeo"] = std::move(geo);
}

void GeoShaderApp::Practice1()
{
	// GS도 비슷하게 인트로를 정해주고 컴파일을 하면 된다.
	m_Shaders["practice1VS"] = d3dUtil::CompileShader(L"Shaders\\practice1.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["practice1GS"] = d3dUtil::CompileShader(L"Shaders\\practice1.hlsl", nullptr, "GS", "gs_5_1");
	m_Shaders["practice1PS"] = d3dUtil::CompileShader(L"Shaders\\practice1.hlsl", nullptr, "PS", "ps_5_1");

	m_PracticeInputLayout =
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();

	struct Practice1Vertex
	{
		XMFLOAT3 Pos;
	};

	static const int PizzaSlices = 16;
	std::array<Practice1Vertex, 16> vertices;
	const float sliceRadian = XM_2PI / (PizzaSlices - 1);

	for (UINT i = 0; i < PizzaSlices; i++)
	{
		float currRad = sliceRadian * i;
		float x = cosf(currRad);
		float z = sinf(currRad);
		vertices[i].Pos = XMFLOAT3(x, 0.f, z);
	}

	// 어처피 quad는 shader에서 만들어줄 것이기 때문에 주르륵 넘긴다.
	std::array<std::uint16_t, 30> indices = { 0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9,10,10,11,11,12,12,13,13,14,14,15 };

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Practice1Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "practice1Geo";

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
		ibByteSize, geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(Practice1Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	m_Geometries["practice1Geo"] = std::move(geo);

	BuildMaterials();

	std::unique_ptr<RenderItem> prac1RenderItem = std::make_unique<RenderItem>();
	prac1RenderItem->WorldMat = MathHelper::Identity4x4();
	prac1RenderItem->ObjCBIndex = 0;
	prac1RenderItem->Mat = m_Materials["practiceMat"].get();
	prac1RenderItem->Geo = m_Geometries["practice1Geo"].get();
	prac1RenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	prac1RenderItem->IndexCount = prac1RenderItem->Geo->DrawArgs["points"].IndexCount;
	prac1RenderItem->StartIndexLocation = prac1RenderItem->Geo->DrawArgs["points"].StartIndexLocation;
	prac1RenderItem->BaseVertexLocation = prac1RenderItem->Geo->DrawArgs["points"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Practice].push_back(prac1RenderItem.get());

	m_AllRenderItems.push_back(std::move(prac1RenderItem));

	BuildFrameResources();

	// practice 1 전용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC prac1PSODesc;
	ZeroMemory(&prac1PSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	prac1PSODesc.pRootSignature = m_RootSignature.Get();
	prac1PSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice1VS"]->GetBufferPointer()),
		m_Shaders["practice1VS"]->GetBufferSize()
	};
	prac1PSODesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice1GS"]->GetBufferPointer()),
		m_Shaders["practice1GS"]->GetBufferSize()
	};
	prac1PSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice1PS"]->GetBufferPointer()),
		m_Shaders["practice1PS"]->GetBufferSize()
	};
	prac1PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//prac1PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	prac1PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	prac1PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	prac1PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	prac1PSODesc.SampleMask = UINT_MAX;
	prac1PSODesc.NumRenderTargets = 1;
	prac1PSODesc.RTVFormats[0] = m_BackBufferFormat;
	prac1PSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	prac1PSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	prac1PSODesc.DSVFormat = m_DepthStencilFormat;

	prac1PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE; // 점으로 넘겨준다.
	prac1PSODesc.InputLayout = { m_PracticeInputLayout.data(), (UINT)m_PracticeInputLayout.size() };

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&prac1PSODesc, IID_PPV_ARGS(&m_PSOs["prac1PSO"])));
}

void GeoShaderApp::Practice2()
{
	// GS도 비슷하게 인트로를 정해주고 컴파일을 하면 된다.
	m_Shaders["practice2VS"] = d3dUtil::CompileShader(L"Shaders\\practice2.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["practice2GS"] = d3dUtil::CompileShader(L"Shaders\\practice2.hlsl", nullptr, "GS", "gs_5_1");
	m_Shaders["practice2PS"] = d3dUtil::CompileShader(L"Shaders\\practice2.hlsl", nullptr, "PS", "ps_5_1");

	m_PracticeInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	LoadTextures();
	BuildRootSignature();
	BuildDescriptorHeaps();

	// 일단 제너레이터로 박스를 만들고
	GeometryGenerator geoGen;
	GeometryGenerator::MeshData geoSphere = geoGen.CreateGeosphere(1.f, 0);
	// 속성도 뽑아 먹고
	SubmeshGeometry geoSphereSubmesh;
	geoSphereSubmesh.IndexCount = (UINT)geoSphere.Indices32.size();
	geoSphereSubmesh.StartIndexLocation = 0;
	geoSphereSubmesh.BaseVertexLocation = 0;
	// 데이터도 뽑아 먹는다.
	std::vector<Vertex> vertices(geoSphere.Vertices.size());

	for (size_t i = 0; i < geoSphere.Vertices.size(); i++)
	{
		vertices[i].Pos = geoSphere.Vertices[i].Position;
		vertices[i].Normal = geoSphere.Vertices[i].Normal;
		vertices[i].TexC = geoSphere.Vertices[i].TexC;
	}

	std::vector<std::uint16_t> indices = geoSphere.GetIndices16();

	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(uint16_t);

	auto geo = std::make_unique<MeshGeometry>();
	geo->Name = "practice2Geo";

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
		ibByteSize, geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(Vertex);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;
	geo->IndexBufferByteSize = ibByteSize;

	SubmeshGeometry submesh;
	submesh.IndexCount = (UINT)indices.size();
	submesh.StartIndexLocation = 0;
	submesh.BaseVertexLocation = 0;

	geo->DrawArgs["points"] = submesh;

	m_Geometries["practice2Geo"] = std::move(geo);

	BuildMaterials();

	std::unique_ptr<RenderItem> prac2RenderItem = std::make_unique<RenderItem>();
	prac2RenderItem->WorldMat = MathHelper::Identity4x4();
	prac2RenderItem->ObjCBIndex = 0;
	prac2RenderItem->Mat = m_Materials["practiceMat"].get();
	prac2RenderItem->Geo = m_Geometries["practice2Geo"].get();
	prac2RenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	prac2RenderItem->IndexCount = prac2RenderItem->Geo->DrawArgs["points"].IndexCount;
	prac2RenderItem->StartIndexLocation = prac2RenderItem->Geo->DrawArgs["points"].StartIndexLocation;
	prac2RenderItem->BaseVertexLocation = prac2RenderItem->Geo->DrawArgs["points"].BaseVertexLocation;

	m_RenderItemLayer[(int)RenderLayer::Practice].push_back(prac2RenderItem.get());

	m_AllRenderItems.push_back(std::move(prac2RenderItem));

	BuildFrameResources();

	// practice 2 전용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC prac2PSODesc;
	ZeroMemory(&prac2PSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	prac2PSODesc.pRootSignature = m_RootSignature.Get();
	prac2PSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice2VS"]->GetBufferPointer()),
		m_Shaders["practice2VS"]->GetBufferSize()
	};
	prac2PSODesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice2GS"]->GetBufferPointer()),
		m_Shaders["practice2GS"]->GetBufferSize()
	};
	prac2PSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["practice2PS"]->GetBufferPointer()),
		m_Shaders["practice2PS"]->GetBufferSize()
	};
	prac2PSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	prac2PSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	prac2PSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	prac2PSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	prac2PSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	prac2PSODesc.SampleMask = UINT_MAX;
	prac2PSODesc.NumRenderTargets = 1;
	prac2PSODesc.RTVFormats[0] = m_BackBufferFormat;
	prac2PSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	prac2PSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	prac2PSODesc.DSVFormat = m_DepthStencilFormat;

	prac2PSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	prac2PSODesc.InputLayout = { m_PracticeInputLayout.data(), (UINT)m_PracticeInputLayout.size() };

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&prac2PSODesc, IID_PPV_ARGS(&m_PSOs["prac2PSO"])));
}

void GeoShaderApp::BuildPSOs()
{
	// #1 불투명 PSO
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

	// #2 반투명 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC transparentPSODesc = opaquePSODesc;
	// 이전 예제랑 똑같이 해준다.
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

	// #3 Stencil 마킹하는 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC markMirrorsPSODesc = opaquePSODesc;

	// 얘는 RenderTargetWriteMask를 0으로 해줘서 RGBA를 기록하지 않는다.
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0; 
	// Stencil의 Depth도 기록하지 않는다.
	D3D12_DEPTH_STENCIL_DESC mirrorDSS;
	mirrorDSS.DepthEnable = true; // depth 판정을 하는데
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO; // 기록은 하지 않는다.
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true; // 스텐실 테스트를 켜주고
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff; // 마스크도 켜준다

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&markMirrorsPSODesc, IID_PPV_ARGS(&m_PSOs["markStencilMirrors"])));

	// 깊이 판정을 실패 했을 때는, 스텐실을 쓰지 않는다.
	// 그 이외 조건이 없으니, 스탠실을 항상 작성한다.
	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// PSO desc에 넣어주고 만든다.
	markMirrorsPSODesc.BlendState = mirrorBlendState;
	markMirrorsPSODesc.DepthStencilState = mirrorDSS;

	// #4 스텐실 값을 판정해서 반사된 skull을 그릴 PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawReflectionsPSODesc = opaquePSODesc;

	D3D12_DEPTH_STENCIL_DESC reflectionsDSS;
	reflectionsDSS.DepthEnable = true;
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionsDSS.StencilEnable = true;
	reflectionsDSS.StencilReadMask = 0xff;
	reflectionsDSS.StencilWriteMask = 0xff;

	// 스텐실 판정이 1값일 때, 통과한 것으로 본다.
	reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
	//reflectionsDDS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	// 라스터라이저 상태를 바꿔줘서, 반대로 삼각형이 그려지게 만들어서 앞뒤를 바꾼다.
	drawReflectionsPSODesc.DepthStencilState = reflectionsDSS;
	drawReflectionsPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
	drawReflectionsPSODesc.RasterizerState.FrontCounterClockwise = true;

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&drawReflectionsPSODesc, IID_PPV_ARGS(&m_PSOs["drawStencilReflections"])));

	// #5 그림자를 그리는 PSO
	// 얘는 Transparent한 친구다. 아예 시커멓지는 않지 않은가?

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPSODesc = transparentPSODesc;
	D3D12_DEPTH_STENCIL_DESC shadowDSS;

	shadowDSS.DepthEnable = true;
	shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowDSS.StencilEnable = true;
	shadowDSS.StencilReadMask = 0xff;
	shadowDSS.StencilWriteMask = 0xff;

	// Stencil 판정을 통과하면 1을 늘려서, 그림자 여러번 그리는 걸 방지한다.
	shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	//shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	shadowPSODesc.DepthStencilState = shadowDSS;

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&shadowPSODesc, IID_PPV_ARGS(&m_PSOs["shadow"])));

	// #6 알파 자르기 PSO

	D3D12_GRAPHICS_PIPELINE_STATE_DESC alphaTestPSODesc = opaquePSODesc;
	alphaTestPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["alphaTestedPS"]->GetBufferPointer()),
		m_Shaders["alphaTestedPS"]->GetBufferSize()
	};
	alphaTestPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&alphaTestPSODesc, IID_PPV_ARGS(&m_PSOs["alphaTested"])));

	// #7 Tree Sprite 전용 PSO
	D3D12_GRAPHICS_PIPELINE_STATE_DESC treeSpritePSODesc = opaquePSODesc;
	treeSpritePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpriteVS"]->GetBufferPointer()),
		m_Shaders["treeSpriteVS"]->GetBufferSize()
	};
	treeSpritePSODesc.GS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpriteGS"]->GetBufferPointer()),
		m_Shaders["treeSpriteGS"]->GetBufferSize()
	};
	treeSpritePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["treeSpritePS"]->GetBufferPointer()),
		m_Shaders["treeSpritePS"]->GetBufferSize()
	};
	treeSpritePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT; // 점으로 넘겨준다.
	treeSpritePSODesc.InputLayout = { m_TreeSpriteInputLayout.data(), (UINT)m_TreeSpriteInputLayout.size() };
	treeSpritePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

	// alpha를 coverage로 사용하는 기법인데... 뭔소린지 모르겠다.
	// https://learn.microsoft.com/ko-kr/windows/win32/direct3d11/d3d10-graphics-programming-guide-blend-state#alpha-to-coverage
	treeSpritePSODesc.BlendState.AlphaToCoverageEnable = false;
	
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&treeSpritePSODesc, IID_PPV_ARGS(&m_PSOs["treeSprites"])));
}

void GeoShaderApp::BuildFrameResources()
{
	UINT waveVerCount = 0;
	UINT passCBCount = 1;
#if !WAVE
	waveVerCount = 0;
#else
	waveVerCount = m_Waves->VertexCount();
#endif
	
	// 반사상 용 PassCB가 하나 추가 된다.
	for (int i = 0; i < g_NumFrameResources; ++i)
	{
		m_FrameResources.push_back(
			std::make_unique<FrameResource>(m_d3dDevice.Get(),
			passCBCount,
			(UINT)m_AllRenderItems.size(),
			(UINT)m_Materials.size(),
			waveVerCount
		));
	}
}

void GeoShaderApp::BuildMaterials()
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
	water->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.5f);
	water->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	water->Roughness = 0.f;

	std::unique_ptr<Material> fence = std::make_unique<Material>();
	fence->Name = "fence";
	fence->MatCBIndex = 2;
	fence->DiffuseSrvHeapIndex = 2;
	fence->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	fence->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	fence->Roughness = 0.25f;

	std::unique_ptr<Material> treeSprites = std::make_unique<Material>();
	treeSprites->Name = "treeSprites";
	treeSprites->MatCBIndex = 3;
	treeSprites->DiffuseSrvHeapIndex = 3;
	treeSprites->DiffuseAlbedo = XMFLOAT4(1.f, 1.f, 1.f, 1.f);
	treeSprites->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	treeSprites->Roughness = 0.125;

	std::unique_ptr<Material> practiceMat = std::make_unique<Material>();
	practiceMat->Name = "practice1Mat";
	practiceMat->MatCBIndex = 4;
	practiceMat->DiffuseSrvHeapIndex = 4;
	practiceMat->DiffuseAlbedo = XMFLOAT4(5.f, 4.f, 0.7f, 1.f);
	practiceMat->FresnelR0 = XMFLOAT3(0.01f, 0.01f, 0.01f);
	practiceMat->Roughness = 0.125;

	m_Materials["practiceMat"] = std::move(practiceMat);

	m_Materials["grass"] = std::move(grass);
	m_Materials["water"] = std::move(water);
	m_Materials["fence"] = std::move(fence);
	m_Materials["treeSprites"] = std::move(treeSprites);
}

void GeoShaderApp::BuildRenderItems()
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

	std::unique_ptr<RenderItem> fenceRenderItem = std::make_unique<RenderItem>();
	XMStoreFloat4x4(&fenceRenderItem->WorldMat, XMMatrixTranslation(3.f, 2.f, -9.f));
	fenceRenderItem->ObjCBIndex = 2;
	fenceRenderItem->Mat = m_Materials["fence"].get();
	fenceRenderItem->Geo = m_Geometries["fenceGeo"].get();
	fenceRenderItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	fenceRenderItem->IndexCount = fenceRenderItem->Geo->DrawArgs["fence"].IndexCount;
	fenceRenderItem->StartIndexLocation = fenceRenderItem->Geo->DrawArgs["fence"].StartIndexLocation;
	fenceRenderItem->BaseVertexLocation = fenceRenderItem->Geo->DrawArgs["fence"].BaseVertexLocation;

	std::unique_ptr<RenderItem> treeSpritesRenderItem = std::make_unique<RenderItem>();
	treeSpritesRenderItem->WorldMat = MathHelper::Identity4x4();
	treeSpritesRenderItem->ObjCBIndex = 3;
	treeSpritesRenderItem->Mat = m_Materials["treeSprites"].get();
	treeSpritesRenderItem->Geo = m_Geometries["treeSpritesGeo"].get();
	treeSpritesRenderItem->PrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_POINTLIST;
	treeSpritesRenderItem->IndexCount = treeSpritesRenderItem->Geo->DrawArgs["points"].IndexCount;
	treeSpritesRenderItem->StartIndexLocation = treeSpritesRenderItem->Geo->DrawArgs["points"].StartIndexLocation;
	treeSpritesRenderItem->BaseVertexLocation = treeSpritesRenderItem->Geo->DrawArgs["points"].BaseVertexLocation;

	// 파도는 반투명한 친구이다.
	m_RenderItemLayer[(int)RenderLayer::Transparent].push_back(wavesRenderItem.get());
	// 얘는 불투명한 친구
	m_RenderItemLayer[(int)RenderLayer::Opaque].push_back(gridRenderItem.get());
	// Fence는 알파 자르기를 하는 Render Item이다.
	m_RenderItemLayer[(int)RenderLayer::AlphaTested].push_back(fenceRenderItem.get());
	// Tree는 다른 VS, GS, PS를 가진다.
	m_RenderItemLayer[(int)RenderLayer::AlphaTestedSprites].push_back(treeSpritesRenderItem.get());

	m_AllRenderItems.push_back(std::move(wavesRenderItem));
	m_AllRenderItems.push_back(std::move(gridRenderItem));
	m_AllRenderItems.push_back(std::move(fenceRenderItem));
	m_AllRenderItems.push_back(std::move(treeSpritesRenderItem));
}

void GeoShaderApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems)
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

void GeoShaderApp::DrawComplexityQuad(ID3D12GraphicsCommandList* _cmdList, int _StencilRef)
{
	// 이제 Material도 물체마다 업데이트 해줘야 한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT matCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(MaterialConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();
	ID3D12Resource* matCB = m_CurrFrameResource->MaterialCB->Resource();

	RenderItem* ri = m_RenderItemLayer[(int)RenderLayer::Practice][_StencilRef];

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

float GeoShaderApp::GetHillsHeight(float _x, float _z) const
{
	return 0.3f * (_z * sinf(0.1f * _x) + _x * cosf(0.1f * _z));
}

XMFLOAT3 GeoShaderApp::GetHillsNormal(float _x, float _z) const
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

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 10> GeoShaderApp::GetStaticSamplers()
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
