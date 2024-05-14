//***************************************************************************************
// FbxTestApp.cpp by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>
#include <iomanip>
#include "ShadowMap.h"
#include "../Common/Camera.h"
#include "Ssao.h"

#include "FbxPractice.h"
#include "SkinnedData.h"
#include "LoadM3d.h"

#include <format>

const int g_NumFrameResources = 3;
// fbxPractice
const std::string danceFbxFilePath = "Fbxs\\Snake_Hip_Hop_Dance_none.fbx";

// 일단은 스키닝할 모델은 하나밖에 없다.
struct SkinnedModelInstance
{
	std::shared_ptr<SkinnedData> SkinnedInfo = nullptr;
	std::vector<DirectX::XMFLOAT4X4> FinalTransforms;
	std::string ClipName;
	float TimePos = 0.f;

	// 시간에 따라 현재 animation clip에 맞는 bone 위치를
	// 보간하고, 최종 transform을 계산한다.
	void UpdateSkinnedAnimation(float _dt)
	{
		TimePos += _dt;

		// 애니메이션 루프
		if (TimePos > SkinnedInfo->GetClipEndTime(ClipName))
		{
			TimePos = 0.f;
		}
		// 현재 시간에 맞는 final transform을 구한다.
		SkinnedInfo->GetFinalTransforms(ClipName, TimePos, FinalTransforms);

	}
};

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
	int NumFramesDirty = g_NumFrameResources;

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

	// 스키닝하는 아이템에만 사용하고
	UINT SkinnedCBIndex = -1;
	// 그렇지 않으면 nullptr로 둔다.
	std::shared_ptr<SkinnedModelInstance> SkinnedModelInst = nullptr;
};

// 스카이 맵을 그리는 lay를 따로 둔다. 
enum class RenderLayer : int
{
	Opaque = 0,
	SkinnedOpaque,
	Debug,
	Sky,
	Count
};

class FbxTestApp : public D3DApp
{
public:
	FbxTestApp(HINSTANCE hInstance);
	~FbxTestApp();

	FbxTestApp(const FbxTestApp& _other) = delete;
	FbxTestApp& operator=(const FbxTestApp& _other) = delete;

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
	void UpdateSkinnedCBs(const GameTimer& _gt); // 스키닝할 때 필요한 final bone을 넘겨주는 SkinnedCB를 업데이트 해준다.
	void UpdateMaterialBuffer(const GameTimer& _gt);
	void UpdateShadowTransform(const GameTimer& _gt); // 광원에서 텍스쳐로 넘어가는 행렬을 업데이트해준다.
	void UpdateMainPassCB(const GameTimer& _gt);
	void UpdateShadowPassCB(const GameTimer& _gt); // 디버깅용 화면을 그릴때 사용하는 PassCB를 업데이트 해준다.
	void UpdateSsaoCB(const GameTimer& _gt); // Ssao Map을 생성할 때 사용하는 SsaoCB를 업데이트 해준다.

	// ==== Init ====
	void LoadTextures();
	void BuildRootSignature();
	void BuildSsaoRootSignature();
	void BuildDescriptorHeaps();
	void BuildShadersAndInputLayout();
	void BuildFbxGeometry();
	void BuildMaterials();
	void BuildRenderItems();
	void BuildPSOs();
	void BuildFrameResources();

	// ==== Render ====
	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY  _Type = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED);
	void DrawSceneToShadowMap(); // 광원입장에서 깊이맵을 그려주는 함수이다.
	void DrawNormalsAndDepth(); // Screen 입장에서 노멀맵과 깊이맵을 그려주는 함수이다.

	// 정적 샘플러 구조체를 미리 만드는 함수이다.
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 11> GetStaticSamplers();

private:
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	// Ssao용 Root Signature를 따로 만들어야 편하다.
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	ComPtr<ID3D12RootSignature> m_SsaoRootSignature = nullptr;

	// Uav도 함께 쓰는 heap이므로 이름을 바꿔준다.
	ComPtr<ID3D12DescriptorHeap> m_CbvSrvUavDescriptorHeap = nullptr;

	// 도형, 쉐이더, PSO 등등 App에서 관리한다..
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_Materials;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

	// input layout도 백터로 가지고 있는다
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;
	// skinning 할때는 Vertex에 추가로 넘겨줘야할 것이 있다.
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_SkinnedInputLayout;

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

	// 쉐도우 맵을 관리해주는 클래스 인스턴스다.
	std::unique_ptr<ShadowMap> m_ShadowMap;
	// Ssao Map을 생성하고 블러를 먹여주는 클래스 인스턴스다.
	std::unique_ptr<Ssao> m_Ssao;

	// 필요한 부분에만 그림자를 그리도록 하는 BoundingSphere다.
	BoundingSphere m_SceneBounds;

	// Sky Texture가 위치한 view Handle Offset을 저장해 놓는다.
	UINT m_SkyTexHeapIndex = 0;
	// Shadow Texture가 위치한 view handle offset을 저장해 놓는다.
	UINT m_ShadowMapHeapIndex = 0;
	// Ssao를 생성할 때 사용하는 Normal Map, Depth Map, RandomVector Map이 연속적으로 있는 view offset이다.
	UINT m_SsaoHeapIndexStart;
	// Ssao 클래스에서 관리하고 있는, AmbientMap view offset이다.
	// (ambient0Map, ambient1Map, ambientNormalMap, ambientDepthMap, RandomVectorMap)
	UINT m_SsaoAmbientMapIndex = 0;

	// 디버깅용 큐브와 텍스쳐가 위치한 view handle offset을 저장해 놓는다.
	UINT m_NullCubeSrvIndex = 0;
	UINT m_NullTexSrvIndex0 = 0;
	UINT m_NullTexSrvIndex1 = 0;
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

	//fbx
	int m_CountOfMeshNode = 0;

	// Mesh Skinning 할 때 쓰이는 맴버들이다.
	UINT m_SkinnedSrvHeapStart = 0;
	std::vector<std::shared_ptr<SkinnedModelInstance>> m_SkinnedModelInsts;
	//std::unique_ptr<SkinnedModelInstance> m_SkinnedModelInst; 
	std::vector<std::shared_ptr<SkinnedData>> m_SkinnedInfos;
	//SkinnedData m_SkinnedInfo; 
	std::vector<M3DLoader::Subset> m_SkinnedSubsets;
	std::vector<M3DLoader::M3dMaterial> m_SkinnedMaterials;
	std::vector<std::string> m_SkinnedTextureNames;


public:
	// Heap에서 연속적으로 존재하는 View들이 많을 예정이라 이렇게 Get 함수를 만들었다.
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetCpuSrv(int index)const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE srv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CbvSrvUavDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
		srv.Offset(index, m_CbvSrvUavDescriptorSize);
		return srv;
	}
	CD3DX12_GPU_DESCRIPTOR_HANDLE GetGpuSrv(int index)const
	{
		CD3DX12_GPU_DESCRIPTOR_HANDLE srv = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		srv.Offset(index, m_CbvSrvUavDescriptorSize);
		return srv;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(int index)const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE dsv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_DsvHeap->GetCPUDescriptorHandleForHeapStart());
		dsv.Offset(index, m_DsvDescriptorSize);
		return dsv;
	}
	CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(int index)const
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RtvHeap->GetCPUDescriptorHandleForHeapStart());
		rtv.Offset(index, m_RtvDescriptorSize);
		return rtv;
	}
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
		FbxTestApp theApp(hInstance);
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

FbxTestApp::FbxTestApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
	// 어떤 레벨이 들어갈지 우리는 이미 알고 있으니, 
	// 생성자에서 BoundingSphere를 초기화 한다.
	m_SceneBounds.Center = XMFLOAT3(0.0f, 0.0f, 0.0f);
	m_SceneBounds.Radius = sqrtf(10.0f * 10.0f + 15.0f * 15.0f);
}

FbxTestApp::~FbxTestApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

void FbxTestApp::CreateRtvAndDsvDescriptorHeaps()
{
	// __super::CreateRtvAndDsvDescriptorHeaps();

	// 렌더타겟 힙 
	// 노멀맵 1개 + AO맵 2개 (핑퐁)
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = SwapChainBufferCount + 3;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&rtvHeapDesc,
		IID_PPV_ARGS(m_RtvHeap.GetAddressOf())
	));

	// 뎁스 스텐실 힙
	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	// 쉐도우 맵을 만드는 깊이 버퍼를 1개 더 만든다.
	dsvHeapDesc.NumDescriptors = 2;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsvHeapDesc.NodeMask = 0;

	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(
		&dsvHeapDesc, IID_PPV_ARGS(m_DsvHeap.GetAddressOf())));
}

bool FbxTestApp::Initialize()
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

	// Ssao 클래스 인스턴스를 생성한다.
	m_Ssao = std::make_unique<Ssao>(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		m_ClientWidth, m_ClientHeight
	);

	// ======== 초기화 ==========
	LoadTextures();
	BuildRootSignature();
	BuildSsaoRootSignature();
	BuildDescriptorHeaps();
	BuildShadersAndInputLayout();
	BuildFbxGeometry();
	BuildMaterials();
	BuildRenderItems();
	BuildFrameResources();
	BuildPSOs();
	// =========================

	// Ssao 내부 변수에 PSO를 세팅해준다.
	m_Ssao->SetPSOs(m_PSOs["ssao"].Get(), m_PSOs["ssaoBlur"].Get());

	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void FbxTestApp::OnResize()
{
	D3DApp::OnResize();

	m_Camera.SetFrustum(0.25f * MathHelper::Pi, GetAspectRatio(), 0.1f, 1000.f);
	// Screen 크기가 변해도 Ssao를 다시 생성한다.
	if (m_Ssao != nullptr)
	{
		m_Ssao->OnResize(m_ClientWidth, m_ClientHeight);
		// 리소스도 다시 만들었으니, view도 다시 설정한다.
		// (내부에서 depth buffer도 사용하기 때문에 넘겨준거다.)
		m_Ssao->RebuildDescriptors(m_DepthStencilBuffer.Get());
	}
}

void FbxTestApp::Update(const GameTimer& _gt)
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
	UpdateSkinnedCBs(_gt);
	UpdateMaterialBuffer(_gt);
	// PassCB에 Shadow Transform이 담기니까, 순서를 잘 맞춰줘야 한다.
	UpdateShadowTransform(_gt);
	UpdateMainPassCB(_gt);
	UpdateShadowPassCB(_gt);
	// Ssao CB도 업데이트 해준다.
	UpdateSsaoCB(_gt);
}

void FbxTestApp::Draw(const GameTimer& _gt)
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
	m_CommandList->SetGraphicsRootShaderResourceView(3, matStructredBuffer->GetGPUVirtualAddress()); // Mat는 3번

	// 텍스쳐도 인덱스를 이용해서 공통으로 사용하니깐, 이렇게 넘겨준다.
	m_CommandList->SetGraphicsRootDescriptorTable(5, m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	// 배경을 그릴 skyTexture 핸들은 미리 만들어 놓는다.
	CD3DX12_GPU_DESCRIPTOR_HANDLE skyTexDescriptor(m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	skyTexDescriptor.Offset(m_SkyTexHeapIndex, m_CbvSrvUavDescriptorSize);

	// ==== 광원 입장에서 그림자 뎁스 그리기 ====

	// 4번에 Render Target이 nullptr인 Srv를 세팅한다.
	m_CommandList->SetGraphicsRootDescriptorTable(4, m_NullSrv);
	// 클래스 인스턴스 맴버인 텍스쳐에 그린다.
	DrawSceneToShadowMap();

	// ==== Screen 입장에서 노멀 / 뎁스 그리기 ====
	DrawNormalsAndDepth();

	// ==== Ssao Map 그리기 ====

	// Ssao용 Root Signature 세팅
	m_CommandList->SetGraphicsRootSignature(m_SsaoRootSignature.Get());
	// commandlist와 현재 FrameResource를 blur 횟수와 함께 넘겨준다.
	// 그러면 내부적으로 SsaoCB를 이용한다.
	m_Ssao->ComputeSsao(m_CommandList.Get(), m_CurrFrameResource, 2);

	// ==== 화면 그리기 ====

	// 다시 루트 시그니처 세팅을 하고
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// 이러면 다시 렌더링 자원을 bind 해줘야 한다.
	m_CommandList->SetGraphicsRootShaderResourceView(3, matStructredBuffer->GetGPUVirtualAddress()); // Mat는 3번

	// 텍스쳐도 인덱스를 이용해서 공통으로 사용하니깐, 이렇게 넘겨준다.
	m_CommandList->SetGraphicsRootDescriptorTable(5, m_CbvSrvUavDescriptorHeap->GetGPUDescriptorHandleForHeapStart());

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
	// 뎁스 스텐실 버퍼도 초기화 한다
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
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress()); // Pass는 2번

	// SkyMap을 그릴 TextureCube를 3번에 바인드해준다.
	m_CommandList->SetGraphicsRootDescriptorTable(4, skyTexDescriptor);

	// drawcall을 걸어준다.
	// 일단 불투명한 애들을 먼저 싹 그려준다.
	m_CommandList->SetPipelineState(m_PSOs["opaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	m_CommandList->SetPipelineState(m_PSOs["skinnedOpaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::SkinnedOpaque]);

	//m_CommandList->SetPipelineState(m_PSOs["debug"].Get());
	//DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Debug]);

	// 하늘을 맨 마지막에 그려주는 것이 depth test 성능상 좋다고 한다. 
	//m_CommandList->SetPipelineState(m_PSOs["sky"].Get());
	//DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Sky]);

	// ======================================
	
	D3D12_RESOURCE_BARRIER bufferBarrier_RT_PRESENT = CD3DX12_RESOURCE_BARRIER::Transition(
		GetCurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		D3D12_RESOURCE_STATE_PRESENT
	);
	m_CommandList->ResourceBarrier(1, &bufferBarrier_RT_PRESENT);

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

void FbxTestApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
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

void FbxTestApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void FbxTestApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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

void FbxTestApp::OnKeyboardInput(const GameTimer _gt)
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

void FbxTestApp::AnimateMaterials(const GameTimer& _gt)
{
}

void FbxTestApp::UpdateObjectCBs(const GameTimer& _gt)
{
	UploadBuffer<ObjectConstants>* currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for (std::unique_ptr<RenderItem>& e : m_AllRenderItems)
	{
		// Constant Buffer가 변경됐을 때 Update를 한다.
		if (e->NumFramesDirty > 0)
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
			e->NumFramesDirty--;
		}
	}
}

void FbxTestApp::UpdateSkinnedCBs(const GameTimer& _gt)
{
	if (m_SkinnedInfos.size() <= 0) {
		return;
	}

	for (int i = 0; i < m_SkinnedInfos.size(); i++) {
		UploadBuffer<SkinnedConstants>* currSkinnedCB = m_CurrFrameResource->SkinnedCB.get();

		m_SkinnedModelInsts[i]->UpdateSkinnedAnimation(_gt.GetDeltaTime());

		SkinnedConstants skinnedConstants;
		std::copy(
			std::begin(m_SkinnedModelInsts[i]->FinalTransforms),
			std::end(m_SkinnedModelInsts[i]->FinalTransforms),
			&skinnedConstants.BoneTransform[0]
		);

		currSkinnedCB->CopyData(i, skinnedConstants);
	}
}

void FbxTestApp::UpdateMaterialBuffer(const GameTimer& _gt)
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

void FbxTestApp::UpdateShadowTransform(const GameTimer& _gt)
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

void FbxTestApp::UpdateMainPassCB(const GameTimer& _gt)
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
	// View Proj Tex 행렬도 넘겨준다.
	XMMATRIX TexMat(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	XMMATRIX viewProjTex = XMMatrixMultiply(VPMat, TexMat);

	XMStoreFloat4x4(&m_MainPassCB.ViewMat, XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&m_MainPassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
	XMStoreFloat4x4(&m_MainPassCB.ProjMat, XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&m_MainPassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
	XMStoreFloat4x4(&m_MainPassCB.VPMat, XMMatrixTranspose(VPMat));
	XMStoreFloat4x4(&m_MainPassCB.InvVPMat, XMMatrixTranspose(InvVPMat));
	XMStoreFloat4x4(&m_MainPassCB.ShadowMat, XMMatrixTranspose(shadowMat));
	XMStoreFloat4x4(&m_MainPassCB.ViewProjTexMat, XMMatrixTranspose(viewProjTex));

	m_MainPassCB.EyePosW = m_Camera.GetPosition3f();
	m_MainPassCB.RenderTargetSize = XMFLOAT2((float)m_ClientWidth, (float)m_ClientHeight);
	m_MainPassCB.InvRenderTargetSize = XMFLOAT2(1.f / m_ClientWidth, 1.f / m_ClientHeight);
	m_MainPassCB.NearZ = 1.f;
	m_MainPassCB.FarZ = 1000.f;
	m_MainPassCB.TotalTime = _gt.GetTotalTime();
	m_MainPassCB.DeltaTime = _gt.GetDeltaTime();

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

void FbxTestApp::UpdateShadowPassCB(const GameTimer& _gt)
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

	XMStoreFloat4x4(&m_ShadowPassCB.ViewMat, XMMatrixTranspose(ViewMat));
	XMStoreFloat4x4(&m_ShadowPassCB.InvViewMat, XMMatrixTranspose(InvViewMat));
	XMStoreFloat4x4(&m_ShadowPassCB.ProjMat, XMMatrixTranspose(ProjMat));
	XMStoreFloat4x4(&m_ShadowPassCB.InvProjMat, XMMatrixTranspose(InvProjMat));
	XMStoreFloat4x4(&m_ShadowPassCB.VPMat, XMMatrixTranspose(VPMat));
	XMStoreFloat4x4(&m_ShadowPassCB.InvVPMat, XMMatrixTranspose(InvVPMat));
	// 광원입장에서 그리기로한 정보를 넘겨줘야 한다.
	UINT w = m_ShadowMap->GetWidth();
	UINT h = m_ShadowMap->GetHeight();

	m_ShadowPassCB.EyePosW = m_LightPosW;
	m_ShadowPassCB.RenderTargetSize = XMFLOAT2((float)w, (float)h);
	m_ShadowPassCB.InvRenderTargetSize = XMFLOAT2(1.f / w, 1.f / h);
	m_ShadowPassCB.NearZ = m_LightNearZ;
	m_ShadowPassCB.FarZ = m_LightFarZ;

	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	// 두번째 offset에 넣어준다.
	currPassCB->CopyData(1, m_ShadowPassCB);
}

void FbxTestApp::UpdateSsaoCB(const GameTimer& _gt)
{
	SsaoConstants ssaoCB;
	XMMATRIX P = m_Camera.GetProjMat();
	// NDC를 Texture 좌표로 바꾼다.
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);

	// 화면 기준이니까 그대로 가져온다.
	ssaoCB.ProjMat = m_MainPassCB.ProjMat;
	ssaoCB.InvProjMat = m_MainPassCB.InvProjMat;
	XMStoreFloat4x4(&ssaoCB.ProjTexMat, XMMatrixTranspose(P * T));
	// 균일한 벡터를 Shader에 넘겨준다.
	m_Ssao->GetOffsetVectors(ssaoCB.OffsetVectors);
	// blur 가중치를 세팅한다.
	std::vector<float> blurWeights = m_Ssao->CalcGaussWeights(2.5f);
	ssaoCB.BlurWeights[0] = XMFLOAT4(&blurWeights[0]);
	ssaoCB.BlurWeights[1] = XMFLOAT4(&blurWeights[4]);
	ssaoCB.BlurWeights[2] = XMFLOAT4(&blurWeights[8]);

	ssaoCB.InvRenderTargetSize = XMFLOAT2(1.0f / m_Ssao->GetSsaoMapWidth(), 1.0f / m_Ssao->GetSsaoMapHeight());

	// Occlusion을 먹일 threshold 값을 세팅한다.
	ssaoCB.OcclusionRadius = 0.5f;
	ssaoCB.OcclusionFadeStart = 0.2f;
	ssaoCB.OcclusionFadeEnd = 1.0f;
	ssaoCB.SurfaceEpsilon = 0.05f;

	UploadBuffer<SsaoConstants>* currSsaoCB = m_CurrFrameResource->SsaoCB.get();
	currSsaoCB->CopyData(0, ssaoCB);
}

void FbxTestApp::LoadTextures()
{
	std::vector<std::string> texNames =
	{
		"bricksDiffuseMap",
		"bricksNormalMap",
		"tileDiffuseMap",
		"tileNormalMap",
		"defaultDiffuseMap",
		"defaultNormalMap",
		"skyCubeMap",
	};

	std::vector<std::wstring> texFilenames =
	{
		L"../Textures/bricks2.dds",
		L"../Textures/bricks2_nmap.dds",
		L"../Textures/tile.dds",
		L"../Textures/tile_nmap.dds",
		L"../Textures/white1x1.dds",
		L"../Textures/default_nmap.dds",
		L"../Textures/desertcube1024.dds",
	};

	for (int i = 0; i < (int)texNames.size(); ++i)
	{
		// 중복해서 만들지 않게 한다.
		if (m_Textures.find(texNames[i]) != std::end(m_Textures))
		{
			// 팔다리가 하나씩 더 있다.
			// bad_alloc Exception
		}else{
			std::unique_ptr<Texture> texMap = std::make_unique<Texture>();
			texMap->Name = texNames[i];
			texMap->Filename = texFilenames[i];
			ThrowIfFailed(DirectX::CreateDDSTextureFromFile12(m_d3dDevice.Get(),
						  m_CommandList.Get(), texMap->Filename.c_str(),
						  texMap->Resource, texMap->UploadHeap));

			m_Textures[texMap->Name] = std::move(texMap);
		}
	}
}


void FbxTestApp::BuildRootSignature()
{
	// Table도 쓸거고, Constant도 쓸거다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[6];
	UINT numOfParameter = 6;

	// TextureCube, ShadowMap, SsaoMap이 Srv로 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, 0);

	// Pixel Shader에서 사용하는 Texture 정보가 넘어가는 Table
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	// Texture2D를 Index로 접근할 것 이기 때문에 이렇게 10개로 만들어 준다. 
	// 이제 노멀맵도 넘어간다.
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 48, 3, 0); // space0 / t2 - texture 정보

	// Srv가 넘어가는 테이블, Pass, Object, Material이 넘어가는 Constant
	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - PerObject
	slotRootParameter[1].InitAsConstantBufferView(1); // b1 - SkinnedConstants
	slotRootParameter[2].InitAsConstantBufferView(2); // b2 - PassConstants
	slotRootParameter[3].InitAsShaderResourceView(0, 1); // t0 - Material Structred Buffer, Space도 1로 지정해준다.
	// D3D12_SHADER_VISIBILITY_ALL로 해주면 모든 쉐이더에서 볼 수 있다.
	slotRootParameter[4].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t0 ~ t1 - TextureCube - Texture2D
	slotRootParameter[5].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t2 - Texture2D Array

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

void FbxTestApp::BuildSsaoRootSignature()
{
	
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];
	UINT numOfParameter = 4;

	// SsaoMap을 생성할 때 사용하는 NormalMap, DepthMap이 들어간다.
	CD3DX12_DESCRIPTOR_RANGE texTable0;
	texTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 0);

	// Random Vector Map 혹은 Input 역할을 하는 Ambient Map이 들어간다.
	CD3DX12_DESCRIPTOR_RANGE texTable1;
	texTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 2, 0);

	slotRootParameter[0].InitAsConstantBufferView(0); // b0 - Ssao CB
	slotRootParameter[1].InitAsConstants(1, 1); // b1 - bHorizontalBlur
	slotRootParameter[2].InitAsDescriptorTable(1, &texTable0, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t0 ~ t1 - NormalMap ~ DepthMap
	slotRootParameter[3].InitAsDescriptorTable(1, &texTable1, D3D12_SHADER_VISIBILITY_PIXEL); // space0, t2 - Random Vector Map / Ambient Map

	// Ssao.hlsl는 일단 4개의 샘플러만 보내준다.
	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		0, // 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_POINT, // 필터
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		1, // 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // 필터
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC depthMapSam(
		2, // 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // 필터
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_BORDER,  // addressW
		0.0f,
		0,
		D3D12_COMPARISON_FUNC_LESS_EQUAL,
		D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE);

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		3, // 레지스터 번호
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // 필터
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	std::array<CD3DX12_STATIC_SAMPLER_DESC, 4> staticSamplers =
	{
		pointClamp, linearClamp, depthMapSam, linearWrap
	};

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
		IID_PPV_ARGS(m_SsaoRootSignature.GetAddressOf())));
}

void FbxTestApp::BuildDescriptorHeaps()
{
	const int textureDescriptorCount = 64;

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
	std::vector<ComPtr<ID3D12Resource>> tex2DList =
	{
		m_Textures["bricksDiffuseMap"]->Resource,
		m_Textures["bricksNormalMap"]->Resource,
		m_Textures["tileDiffuseMap"]->Resource,
		m_Textures["tileNormalMap"]->Resource,
		m_Textures["defaultDiffuseMap"]->Resource,
		m_Textures["defaultNormalMap"]->Resource,
	};

	ComPtr<ID3D12Resource> skyCubeMap = m_Textures["skyCubeMap"]->Resource;

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
		m_d3dDevice->CreateShaderResourceView(tex2DList[i].Get(), &srvDesc, viewHandle);

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

	m_d3dDevice->CreateShaderResourceView(skyCubeMap.Get(), &srvDesc, viewHandle);
	m_SkyTexHeapIndex = viewCount++;

	// 미리 맴버에다가 offset을 저장하고.
	m_ShadowMapHeapIndex = viewCount++;
	m_SsaoHeapIndexStart = viewCount++;
	viewCount += 2; // (연속 3개 - NormalMap, DepthMap, RandomVectorMap)
	m_SsaoAmbientMapIndex = viewCount++;
	viewCount += 1; // (연속 3 + 2개 - ambient0Map, ambient1Map)
	m_NullCubeSrvIndex = viewCount++;
	m_NullTexSrvIndex0 = viewCount++;
	m_NullTexSrvIndex1 = viewCount++;

	// 안쓰는 Cube Srv와 Tex Srv를 nullptr을 굳이 넣어주는 이유는 잘 모르겠지만...
	CD3DX12_CPU_DESCRIPTOR_HANDLE nullSrv = GetCpuSrv(m_NullCubeSrvIndex);
	m_NullSrv = GetGpuSrv(m_NullCubeSrvIndex);

	m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);
	nullSrv.Offset(1, m_CbvSrvUavDescriptorSize);

	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = 1;
	srvDesc.Texture2D.ResourceMinLODClamp = 0.f;
	m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	nullSrv.Offset(1, m_CbvSrvUavDescriptorSize);
	m_d3dDevice->CreateShaderResourceView(nullptr, &srvDesc, nullSrv);

	// 클래스 인스턴스에 있는 텍스쳐 들을 view heap과 연결해준다.
	m_ShadowMap->BuildDescriptors(
		GetCpuSrv(m_ShadowMapHeapIndex),
		GetGpuSrv(m_ShadowMapHeapIndex),
		GetDsv(1) // 두번째 dsv offset을 가진다.
	);

	m_Ssao->BuildDescriptors(
		m_DepthStencilBuffer.Get(), // 내부적으로 depth buffer를 사용한다.
		GetCpuSrv(m_SsaoHeapIndexStart), // 내부적으로 5개의 view를 가지고 있는다.
		GetGpuSrv(m_SsaoHeapIndexStart),
		GetRtv(SwapChainBufferCount), // Ssao를 그릴 RT
		m_CbvSrvUavDescriptorSize, 
		m_RtvDescriptorSize
	);
}

void FbxTestApp::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO skinnedDefines[] =
	{
		"SKINNED", "1",
		NULL, NULL
	};


	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\21_ModelImportPractice.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skinnedVS"] = d3dUtil::CompileShader(L"Shaders\\21_ModelImportPractice.hlsl", skinnedDefines, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\21_ModelImportPractice.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["shadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skinnedShadowVS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", skinnedDefines, "VS", "vs_5_1");
	m_Shaders["shadowOpaquePS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", nullptr, "PS", "ps_5_1");
	m_Shaders["shadowAlphaTestedPS"] = d3dUtil::CompileShader(L"Shaders\\Shadow.hlsl", alphaTestDefines, "PS", "ps_5_1");

	m_Shaders["debugVS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skinnedDrawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", skinnedDefines, "VS", "vs_5_1");
	m_Shaders["debugPS"] = d3dUtil::CompileShader(L"Shaders\\ShadowDebug.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["drawNormalsVS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["drawNormalsPS"] = d3dUtil::CompileShader(L"Shaders\\DrawNormals.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["ssaoVS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["ssaoPS"] = d3dUtil::CompileShader(L"Shaders\\Ssao.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["ssaoBlurVS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["ssaoBlurPS"] = d3dUtil::CompileShader(L"Shaders\\SsaoBlur.hlsl", nullptr, "PS", "ps_5_1");

	m_Shaders["skyVS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["skyPS"] = d3dUtil::CompileShader(L"Shaders\\Sky.hlsl", nullptr, "PS", "ps_5_1");

	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 2 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};

	m_SkinnedInputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 2 , D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 2 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 3 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "BONEINDICES", 0, DXGI_FORMAT_R8G8B8A8_UINT, 0, sizeof(XMFLOAT3) * 4 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}

void FbxTestApp::BuildFbxGeometry()
{
	std::unique_ptr<FbxPractice> fbxPractice = std::make_unique<FbxPractice>();
	fbxPractice->Init();
	fbxPractice->ImportFile(danceFbxFilePath.c_str());
	//fbxPractice->TestTraverseScene();
	//fbxPractice->TestTraverseSkin(); 
	//fbxPractice->TestTraverseAnimation();

	FbxScene* pRootScene = fbxPractice->GetRootScene();
	FbxNode* pRootNode = pRootScene->GetRootNode();

	m_CountOfMeshNode = 0;
	std::vector<FbxNode*> MeshNodeArr;
	FbxNode* skeletonNode = nullptr;

	//OutputDebugStringA(std::format("***** count of ChildCount : '{}' *****\n", pRootNode->GetChildCount()).c_str());
	if (pRootNode != nullptr) {
		for (int i = 0; i < pRootNode->GetChildCount(); i++) {
			FbxNodeAttribute* nodeAttrib = pRootNode->GetChild(i)->GetNodeAttribute();
			if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eMesh) {
				m_CountOfMeshNode++;
				MeshNodeArr.push_back(pRootNode->GetChild(i));
				//break;
			}
			else if (nodeAttrib->GetAttributeType() == FbxNodeAttribute::eSkeleton) {
				skeletonNode = pRootNode->GetChild(i);
			}
		}
	}

	int fbxMatCBIndex = 5;
	for (int i = 0; i < m_CountOfMeshNode; i++)
	{
		std::vector<M3DLoader::SkinnedVertex> vertices;
		//std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		OutFbxMaterial outFbxMat;

		m_SkinnedInfos.push_back(std::make_shared<SkinnedData>());
		fbxPractice->GetFbxPerMeshToApp(skeletonNode, MeshNodeArr[i], vertices, indices, outFbxMat, *m_SkinnedInfos[i].get());

		m_SkinnedModelInsts.push_back(std::make_shared<SkinnedModelInstance>());
		m_SkinnedModelInsts[i]->SkinnedInfo = m_SkinnedInfos[i];
		m_SkinnedModelInsts[i]->FinalTransforms.resize(m_SkinnedInfos[i]->BoneCount());
		m_SkinnedModelInsts[i]->ClipName = "Take 001";
		m_SkinnedModelInsts[i]->TimePos = 0.f;

		/*
		std::vector<Vertex> vertices;
		std::vector<std::uint32_t> indices;

		OutFbxMaterial outFbxMat;

		fbxPractice->GetMeshToApp(MeshNodeArr[i]->GetMesh(), vertices, indices);
		fbxPractice->GetMaterialToApp(MeshNodeArr[i]->GetMaterial(0), outFbxMat);
		*/

		std::unique_ptr<Material> fbxMat = std::make_unique<Material>();
		fbxMat->Name = "fbxMat";
		fbxMat->MatCBIndex = fbxMatCBIndex++;
		fbxMat->DiffuseSrvHeapIndex = 4;
		//fbxMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		fbxMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
		//fbxMat->Roughness = 0.3f;

		fbxMat->DiffuseAlbedo = XMFLOAT4(
			outFbxMat.diffuseColor.color.x, 
			outFbxMat.diffuseColor.color.y, 
			outFbxMat.diffuseColor.color.z, 
			1.f);
		fbxMat->Roughness = 1.f - outFbxMat.shininess;

		m_Materials["fbxMat" + i] = std::move(fbxMat);

		const UINT vbByteSize = (UINT)vertices.size() * sizeof(M3DLoader::SkinnedVertex);
		//const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
		const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint32_t);

		std::unique_ptr<MeshGeometry> geo = std::make_unique<MeshGeometry>();
		geo->Name = "fbxGeo_" + i;

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

		geo->VertexByteStride = sizeof(M3DLoader::SkinnedVertex);
		//geo->VertexByteStride = sizeof(Vertex);
		geo->VertexBufferByteSize = vbByteSize;
		geo->IndexFormat = DXGI_FORMAT_R32_UINT;
		geo->IndexBufferByteSize = ibByteSize;

		SubmeshGeometry fbxSubMesh;
		fbxSubMesh.IndexCount = (UINT)indices.size();
		fbxSubMesh.StartIndexLocation = 0;
		fbxSubMesh.BaseVertexLocation = 0;

		geo->DrawArgs["fbx"] = fbxSubMesh;

		m_Geometries[geo->Name] = std::move(geo);
	}
}

void FbxTestApp::BuildPSOs()
{
	// 이렇게 기본 PSO를 따로 뺀 이유는
	// opaque Rendering을 할 때, ssao map을 작성하면스 depth buffer가 이미
	// 작성이 되어있어서 또 작성할 필요가 없기 때문에 그런 것이다.
	// 그래서 depth 판정을 할 때, D3D12_COMPARISON_FUNC_EQUAL 같은 친구들을 렌더링하면
	// 평소 처럼 나오게 되는 것이다.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC basePSODesc;

	ZeroMemory(&basePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	basePSODesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	basePSODesc.pRootSignature = m_RootSignature.Get();
	basePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"]->GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	basePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	basePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	basePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	basePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	basePSODesc.SampleMask = UINT_MAX;
	basePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	basePSODesc.NumRenderTargets = 1;
	basePSODesc.RTVFormats[0] = m_BackBufferFormat;
	basePSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	basePSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	basePSODesc.DSVFormat = m_DepthStencilFormat;

	//
	// 불투명 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSODesc = basePSODesc;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaquePSODesc, IID_PPV_ARGS(&m_PSOs["opaque"])));
	
	//
	// Shadow Map을 생성할 때 사용하는 PSO다
	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowDrawPSODesc = basePSODesc;
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

	//
	// Skinned 불투명 PSO
	// 
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedOpaquePSODesc = opaquePSODesc;
	skinnedOpaquePSODesc.InputLayout = { m_SkinnedInputLayout.data(), (UINT)m_SkinnedInputLayout.size() };
	skinnedOpaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skinnedVS"]->GetBufferPointer()),
		m_Shaders["skinnedVS"]->GetBufferSize()
	};
	//skinnedOpaquePSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&skinnedOpaquePSODesc, IID_PPV_ARGS(&m_PSOs["skinnedOpaque"])));

	//
	// Skinned Shadow Map 생성 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedShadowMapPSODesc = shadowDrawPSODesc;
	skinnedShadowMapPSODesc.InputLayout = { m_SkinnedInputLayout.data(), (UINT)m_SkinnedInputLayout.size() };
	skinnedShadowMapPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skinnedShadowVS"]->GetBufferPointer()),
		m_Shaders["skinnedShadowVS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&skinnedShadowMapPSODesc, IID_PPV_ARGS(&m_PSOs["skinnedShadow_opaque"])));
	
	//
	// debug Layer용 PSO다.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC debugPSODesc = basePSODesc;
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

	//
	// Screen에서 보이는 Normal을 그리는 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC drawNormalsPSODesc = basePSODesc;
	drawNormalsPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["drawNormalsVS"]->GetBufferPointer()),
		m_Shaders["drawNormalsVS"]->GetBufferSize()
	};
	drawNormalsPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["drawNormalsPS"]->GetBufferPointer()),
		m_Shaders["drawNormalsPS"]->GetBufferSize()
	};
	drawNormalsPSODesc.RTVFormats[0] = Ssao::NormalMapFormat;
	drawNormalsPSODesc.SampleDesc.Count = 1;
	drawNormalsPSODesc.SampleDesc.Quality = 0;
	drawNormalsPSODesc.DSVFormat = m_DepthStencilFormat;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&drawNormalsPSODesc, IID_PPV_ARGS(&m_PSOs["drawNormals"])));

	//
	// Skinned Normal을 그리는 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skinnedDrawNormalsPSODesc = drawNormalsPSODesc;
	skinnedDrawNormalsPSODesc.InputLayout = { m_SkinnedInputLayout.data(), (UINT)m_SkinnedInputLayout.size() };
	skinnedDrawNormalsPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["skinnedDrawNormalsVS"]->GetBufferPointer()),
		m_Shaders["skinnedDrawNormalsVS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&skinnedDrawNormalsPSODesc, IID_PPV_ARGS(&m_PSOs["skinnedDrawNormals"])));

	//
	// SSAO Map을 그리는 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoPSODesc = basePSODesc;
	// SSAO는 내부적으로 따로 좌표계 변환을 하고 쉐이더 리소스로 작업을 하기 때문에
	// 입력으로 들어가는 Vertex, Index 값이 없다.
	ssaoPSODesc.InputLayout = { nullptr, 0 };
	ssaoPSODesc.pRootSignature = m_SsaoRootSignature.Get();
	ssaoPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["ssaoVS"]->GetBufferPointer()),
		m_Shaders["ssaoVS"]->GetBufferSize()
	};
	ssaoPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["ssaoPS"]->GetBufferPointer()),
		m_Shaders["ssaoPS"]->GetBufferSize()
	};
	// 뎁스 버퍼도 이용하지 않는다.
	ssaoPSODesc.DepthStencilState.DepthEnable = false;
	ssaoPSODesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	ssaoPSODesc.RTVFormats[0] = Ssao::AmbientMapFormat;
	ssaoPSODesc.SampleDesc.Count = 1;
	ssaoPSODesc.SampleDesc.Quality = 0;
	ssaoPSODesc.DSVFormat = DXGI_FORMAT_UNKNOWN;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&ssaoPSODesc, IID_PPV_ARGS(&m_PSOs["ssao"])));

	//
	// SSAO에 blur를 먹이는 PSO다.
	// 쉐이더만 바꿔주면 된다.
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC ssaoBlurPSODesc = ssaoPSODesc;
	ssaoBlurPSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["ssaoBlurVS"]->GetBufferPointer()),
		m_Shaders["ssaoBlurVS"]->GetBufferSize()
	};
	ssaoBlurPSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["ssaoBlurPS"]->GetBufferPointer()),
		m_Shaders["ssaoBlurPS"]->GetBufferSize()
	};
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&ssaoBlurPSODesc, IID_PPV_ARGS(&m_PSOs["ssaoBlur"])));

	//
	// Sky용 PSO
	//
	D3D12_GRAPHICS_PIPELINE_STATE_DESC skyPSODesc = basePSODesc;

	// 커다란 구로 표현을 할거고, 안쪽에서 바라보기 때문에 cull을 반대로 해줘야한다.
	skyPSODesc.RasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	// z가 항상 1로 고정이 되기 때문에 LESS_EQUAL로 걸어줘야 한다.
	// Init()에서 Clear를 할때 Depth Clear를 1.f로 한다.
	// 안그러면 초기 화면 값만 보이게 된다.
	skyPSODesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
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

void FbxTestApp::BuildFrameResources()
{
	// Shadow Map 용으로 하나 더 필요하다.
	UINT passCBCount = 2;
	UINT skinnedObjectCount = m_CountOfMeshNode;

	for (int i = 0; i < g_NumFrameResources; ++i)
	{
		m_FrameResources.push_back(
			std::make_unique<FrameResource>(m_d3dDevice.Get(),
			passCBCount,
			(UINT)m_AllRenderItems.size(),
			skinnedObjectCount,
			(UINT)m_Materials.size()
		));
	}
}

void FbxTestApp::BuildMaterials()
{
	//0		m_Textures["bricksDiffuseMap"]->Resource.Get(),
	//1		m_Textures["bricksNormalMap"]->Resource.Get(),
	//2		m_Textures["tileDiffuseMap"]->Resource.Get(),
	//3		m_Textures["tileNormalMap"]->Resource.Get(),
	//4		m_Textures["defaultDiffuseMap"]->Resource.Get(),
	//5		m_Textures["defaultNormalMap"]->Resource.Get(),
	//6		~ skinning Texture

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

	std::unique_ptr<Material> skyMat = std::make_unique<Material>();
	skyMat->Name = "skyMat";
	skyMat->MatCBIndex = 3;
	skyMat->DiffuseSrvHeapIndex = 4;
	skyMat->NormalSrvHeapIndex = 5;
	skyMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);;
	skyMat->FresnelR0 = XMFLOAT3(0.1f, 0.1f, 0.1f);
	skyMat->Roughness = 1.0f;

	std::unique_ptr<Material> whiteMat = std::make_unique<Material>();
	whiteMat->Name = "whiteMat";
	whiteMat->MatCBIndex = 4;
	whiteMat->DiffuseSrvHeapIndex = 5;
	whiteMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	whiteMat->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
	whiteMat->Roughness = 0.3f;

	m_Materials["brickMat"] = std::move(brickMat);
	m_Materials["mirrorMat"] = std::move(mirrorMat);
	m_Materials["tileMat"] = std::move(tileMat);
	m_Materials["whiteMat"] = std::move(whiteMat);
	m_Materials["skyMat"] = std::move(skyMat);
}

void FbxTestApp::BuildRenderItems()
{
	UINT objCBIndex = 0;
	// fbx 아이템

	for (int i = 0; i < m_CountOfMeshNode; i++) {

		std::unique_ptr<RenderItem> fbxRitem = std::make_unique<RenderItem>();

		XMStoreFloat4x4(&fbxRitem->WorldMat, XMMatrixScaling(0.2f, 0.2f, 0.2f) * XMMatrixTranslation(0.f, 0.f, 0.f));
		XMStoreFloat4x4(&fbxRitem->TexTransform, XMMatrixScaling(1.0f, 1.0f, 1.0f));
		fbxRitem->ObjCBIndex = objCBIndex++;
		std::string geoName = "fbxGeo_" + i;
		fbxRitem->Geo = m_Geometries[geoName].get();
		fbxRitem->Mat = m_Materials["fbxMat" + i].get();
		fbxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		fbxRitem->IndexCount = fbxRitem->Geo->DrawArgs["fbx"].IndexCount;
		fbxRitem->StartIndexLocation = fbxRitem->Geo->DrawArgs["fbx"].StartIndexLocation;
		fbxRitem->BaseVertexLocation = fbxRitem->Geo->DrawArgs["fbx"].BaseVertexLocation;

		fbxRitem->SkinnedCBIndex = i;
		fbxRitem->SkinnedModelInst = m_SkinnedModelInsts[i];

		m_RenderItemLayer[(int)RenderLayer::SkinnedOpaque].push_back(fbxRitem.get());
		m_AllRenderItems.push_back(std::move(fbxRitem));
	}
}

void FbxTestApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems, D3D_PRIMITIVE_TOPOLOGY _Type)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	UINT skinnedCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(SkinnedConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();
	ID3D12Resource* skinnedCB = m_CurrFrameResource->SkinnedCB->Resource();

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

		// skinnning을 하는 렌더 아이템이면 그에 맞는 CB view 값을 bind 해준다.
		if (ri->SkinnedModelInst != nullptr)
		{
			D3D12_GPU_VIRTUAL_ADDRESS skinnedCBAddress = skinnedCB->GetGPUVirtualAddress() + ri->SkinnedCBIndex * skinnedCBByteSize;
			_cmdList->SetGraphicsRootConstantBufferView(1, skinnedCBAddress);
		}
		else
		{
			_cmdList->SetGraphicsRootConstantBufferView(1, 0);
		}

		_cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
	}
}

void FbxTestApp::DrawSceneToShadowMap()
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
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCBAddress);

	// 광원 입장에서 보는 물체들을 그린다.
	m_CommandList->SetPipelineState(m_PSOs["shadow_opaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 스키닝을 하는 물체에 대해서도 Shadow Map을 그려준다.
	m_CommandList->SetPipelineState(m_PSOs["skinnedShadow_opaque"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::SkinnedOpaque]);

	// Dsv Barrier를 다시 Read로 바꾼다.
	CD3DX12_RESOURCE_BARRIER shadowDS_WRITE_READ = CD3DX12_RESOURCE_BARRIER::Transition(
		m_ShadowMap->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_CommandList->ResourceBarrier(1, &shadowDS_WRITE_READ);
}

void FbxTestApp::DrawNormalsAndDepth()
{
	// 화면에 보이는 normal을 뽑는 것이다.
	m_CommandList->RSSetViewports(1, &m_ScreenViewport);
	m_CommandList->RSSetScissorRects(1, &m_ScissorRect);

	// 렌더타겟이 될 리소스와 뷰를 가져온다.
	ID3D12Resource* normalMap = m_Ssao->GetNormalMap();
	CD3DX12_CPU_DESCRIPTOR_HANDLE normalMapRtv = m_Ssao->GetNormalMapRtv();

	// 베리어를 렌더 타겟으로 바꾸고
	CD3DX12_RESOURCE_BARRIER normalMap_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
	m_CommandList->ResourceBarrier(1, &normalMap_READ_RT);

	// 노멀은 z = 1로, depth는 1로 초기화 시킨다.
	float clearValue[] = { 0.0f, 0.0f, 1.0f, 0.0f };
	m_CommandList->ClearRenderTargetView(normalMapRtv, clearValue, 0, nullptr);
	m_CommandList->ClearDepthStencilView(GetDepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	// 렌더 타겟을 바인드하고
	D3D12_CPU_DESCRIPTOR_HANDLE depthStencilView = GetDepthStencilView();
	m_CommandList->OMSetRenderTargets(1, &normalMapRtv, true, &depthStencilView);

	// 화면과 똑같은 passCB를 넘겨준다..
	ID3D12Resource* passCB = m_CurrFrameResource->PassCB->Resource();
	m_CommandList->SetGraphicsRootConstantBufferView(2, passCB->GetGPUVirtualAddress());

	m_CommandList->SetPipelineState(m_PSOs["drawNormals"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::Opaque]);

	// 여기서도 스키닝하는 친구에 대한 Normal Map을 그려준다.
	m_CommandList->SetPipelineState(m_PSOs["skinnedDrawNormals"].Get());
	DrawRenderItems(m_CommandList.Get(), m_RenderItemLayer[(int)RenderLayer::SkinnedOpaque]);

	// 다시 베리어를 Read로 바꾼다.
	CD3DX12_RESOURCE_BARRIER normalMap_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(normalMap, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
	m_CommandList->ResourceBarrier(1, &normalMap_RT_READ);
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 11> FbxTestApp::GetStaticSamplers()
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
