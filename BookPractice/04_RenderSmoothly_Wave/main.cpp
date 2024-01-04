#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include "../Common/GeometryGenerator.h"
#include "FrameResource.h"
#include <DirectXColors.h>

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

	MeshGeometry* Geo = nullptr;

	// 다른걸 쓸일은 잘 없을 듯
	D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// DrawIndexedInstanced 인자이다.
	UINT IndexCount = 0;
	UINT StartIndexLocation = 0;
	int BaseVertexLocation = 0;
};

class RenderWaveApp : public D3DApp
{
public:
	RenderWaveApp(HINSTANCE hInstance);
	~RenderWaveApp();

	RenderWaveApp(const RenderWaveApp& _other) = delete;
	RenderWaveApp& operator=(const RenderWaveApp& _other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& _gt) override;
	virtual void Draw(const GameTimer& _gt) override;

	virtual void OnMouseDown(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseUp(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove(WPARAM _btnState, int _x, int _y) override;
	// 이번엔 키보드 입력도 받는다.
	void OnKeyboardInput(const GameTimer _gt);

	void UpdateCamera(const GameTimer& _gt);
	void UpdateObjectCBs(const GameTimer& _gt);
	void UpdateMainPassCB(const GameTimer& _gt);

	void BuildDescriptorHeaps();
	void BuildConstantBufferView();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	// 이번엔 박스가 아니라 좀 더 다양한 친구들을 렌더링한다.
	void BuildShapeGeometry();
	void BuildPSOs();
	// 그리고 FrameResource도 만들어서 사용할 것이다.
	void BuildFrameResources();
	// 렌더 아이템 구조체도 사용해서, 더 편하게
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems);


private:
	std::vector<std::unique_ptr<FrameResource>> m_FrameResources;
	FrameResource* m_CurrFrameResource = nullptr;
	int m_CurrFrameResourceIndex = 0;

	// 요걸 보니까 Root Signature나
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;
	// Descriptor 힙은, 하나씩만 있어도 되는 것 같다.
	ComPtr<ID3D12DescriptorHeap> m_CBViewHeap = nullptr;
	ComPtr<ID3D12DescriptorHeap> m_SRViewHeap = nullptr;

	// map으로 도형, 쉐이더, PSO를 관리한다.
	std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> m_Geometries;
	std::unordered_map<std::string, ComPtr<ID3DBlob>> m_Shaders;
	std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> m_PSOs;

	// input layout도 백터로 가지고 있는다
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	// RenderItem 리스트
	std::vector<std::unique_ptr<RenderItem>> m_AllItems;
	// PSO에 의해 구분된다.
	std::vector<RenderItem*> m_OpaqueItems;

	// Object 관계 없이, Render Pass 전체가 공유하는 값이다.
	PassConstants m_MainPassCB;
	// View Heap에 필요한 Offset 이다.
	UINT m_PassCBViewOffset = 0;

	bool m_bWireframe = false;

	XMFLOAT3 m_EyePos = { 0.f, 0.f, 0.f };
	XMFLOAT4X4 m_ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	float m_Phi = 1.5f * XM_PI;
	float m_Theta = 0.2f * XM_PI;
	float m_Radius = 15.f;

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
		RenderWaveApp theApp(hInstance);
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

RenderWaveApp::RenderWaveApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

RenderWaveApp::~RenderWaveApp()
{
	if (m_d3dDevice != nullptr)
	{
		FlushCommandQueue();
	}
}

bool RenderWaveApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	// 여기서는 공동으로 쓰는 RootSignature를 먼저 정의한다.
	BuildRootSignature();
	// 그리고 FrameResources 마다 쓰는 걸 정의하고
	BuildShadersAndInputLayout();
	// Item 마다 쓰는걸 정의한다.
	BuildShapeGeometry();
	BuildRenderItems();
	// 이제 FrameResource를 만들고
	BuildFrameResources();
	// ViewHeap에다가
	BuildDescriptorHeaps();
	// CBView를 끼워 넣는다.
	BuildConstantBufferView();
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

void RenderWaveApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void RenderWaveApp::Update(const GameTimer& _gt)
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
	UpdateMainPassCB(_gt);
}

void RenderWaveApp::Draw(const GameTimer& _gt)
{
	// 현재 FrameResource가 가지고 있는 allocator를 가지고 와서 초기화 한다.
	ComPtr<ID3D12CommandAllocator> CurrCommandAllocator = m_CurrFrameResource->CmdListAlloc;
	ThrowIfFailed(CurrCommandAllocator->Reset());

	// Command List도 초기화를 한다
	// 근데 Command List에 Reset()을 걸려면, 한번은 꼭 Command Queue에 
	// ExecuteCommandList()로 등록이 된적이 있어야 가능하다.

	// PSO별로 등록하면서, Allocator와 함께, Command List를 초기화 한다.
	if (m_bWireframe)
	{
		ThrowIfFailed(m_CommandList->Reset(CurrCommandAllocator.Get(), m_PSOs["opaque_wireframe"].Get()));
	}
	else
	{
		ThrowIfFailed(m_CommandList->Reset(CurrCommandAllocator.Get(), m_PSOs["opaque"].Get()));
	}
	
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

	// ==== 준비된 도형 그리기 ======
	
	// View heap과 
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBViewHeap.Get() };
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);
	// Signature를 세팅한다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Frame 당 공통으로 념겨주는 Pass CB를 넘겨준다.
	// Frame Offset의 View Heap Offset으로 Descriptor Handle을 구한다.
	int passCBViewIndex = m_PassCBViewOffset + m_CurrFrameResourceIndex;
	CD3DX12_GPU_DESCRIPTOR_HANDLE passCBViewHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBViewHeap->GetGPUDescriptorHandleForHeapStart());
	passCBViewHandle.Offset(passCBViewIndex, m_CbvSrvUavDescriptorSize);
	// 그리고 SetGraphicsRootDescriptorTable로 위치를 지정해주면
	// 거기에 있는 CB 값이 Shader로 넘어가게 된다.
	m_CommandList->SetGraphicsRootDescriptorTable(1, passCBViewHandle);

	DrawRenderItems(m_CommandList.Get(), m_OpaqueItems);

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

void RenderWaveApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void RenderWaveApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
}

void RenderWaveApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
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

void RenderWaveApp::OnKeyboardInput(const GameTimer _gt)
{
	if (GetAsyncKeyState('1') & 0x8000)
	{
		m_bWireframe = true;
	}
	else
	{
		m_bWireframe = false;
	}
}

void RenderWaveApp::UpdateCamera(const GameTimer& _gt)
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

void RenderWaveApp::UpdateObjectCBs(const GameTimer& _gt)
{
	UploadBuffer<ObjectConstants>* currObjectCB = m_CurrFrameResource->ObjectCB.get();
	for (std::unique_ptr<RenderItem>& e : m_AllItems)
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

void RenderWaveApp::UpdateMainPassCB(const GameTimer& _gt)
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
	
	UploadBuffer<PassConstants>* currPassCB = m_CurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, m_MainPassCB);
}

void RenderWaveApp::BuildDescriptorHeaps()
{
	// ===Constant Buffer를 Pipeline에 전달하기 위한==
	// =======Descriptor(View) Heap을 만드는 것=======

	// Object 개수마다 사용하기로 정한 Frame Resouces 개수만큼 CB Descriptor 가 필요하다.
	// 그리고 Pass CB용 Descriptor도 필요하다. 그러니깐
	UINT objCount = (UINT)m_OpaqueItems.size();
	// 이렇게 1을 더한뒤, 사용하기로한 프레임 리소스를 곱하면? ㅇㅇ
	UINT numDescriptors = (objCount + 1) * g_NumFrameResources;

	// 그리고 Pass View가 힙에 저장될 위치에 쓰일 Offset도 미리 구해놓는다.
	m_PassCBViewOffset = objCount * g_NumFrameResources;
	

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = numDescriptors;
	// 이게 Constant Buffer 용이다.
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; 
	// flag를 이렇게 해야, Shader가 참조할 수 있도록 Command List에 Bind 된다.
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; 
	cbvHeapDesc.NodeMask = 0; // 다중 어뎁터를 할 일이 생길까?

	// 그리고 어뎁터로 CB를 위한 View Heap 생성을 한다.
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBViewHeap)));
}

void RenderWaveApp::BuildConstantBufferView()
{
	// Descriptor(View)을 타고, Pipe line에 넘어갈 Buffer를 만든다.

	// 일단 CB Size를 Byte로 구한다.
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));
	// 물체 개수를 구하고
	UINT objCount = (UINT)m_OpaqueItems.size();

	// object 마다 FrameResource에서 사용할 CB Descriptor을 만든다.
	for (int frameIndex = 0; frameIndex < g_NumFrameResources; frameIndex++)
	{
		// 일단 만들기로한 프레임 리소스를 다 돌아다니면서,
		ID3D12Resource* objectCB = m_FrameResources[frameIndex]->ObjectCB->Resource();
		// 그리고 만들기로 한 오브젝트 마다
		for (UINT i = 0; i < objCount; i++)
		{
			// 그 친구가 가지고 있는 CB UploadBuffer에
			D3D12_GPU_VIRTUAL_ADDRESS cbAddress = objectCB->GetGPUVirtualAddress();

			// CB 사이즈 Offset으로 넘기고
			cbAddress += i * objCBByteSize;

			// 그 값을 View Heap에 연결 시켜준다. 
			int heapIndex = frameIndex * objCount + i;
			// 이 값은 CPU에서 갱신해주는 거니깐, CPU Handle에 연결 시킨다.
			CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBViewHeap->GetCPUDescriptorHandleForHeapStart());
			handle.Offset(heapIndex, m_CbvSrvUavDescriptorSize);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc; 
			// 이렇게 주소 연결과, 크기를 지정해준 다음에
			cbvDesc.BufferLocation = cbAddress;
			cbvDesc.SizeInBytes = objCBByteSize;
			// CB Descriptor(View)를 만들어준다.
			m_d3dDevice->CreateConstantBufferView(&cbvDesc, handle);
		}
	}

	// 이제 프레임에 한번 넘기는 Pass CB에 대해 Descriptor를 만들 차례

	// CB의 크기를 미리 구하고
	UINT passCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(PassConstants));

	for (int frameIndex = 0; frameIndex < g_NumFrameResources; frameIndex++)
	{
		ID3D12Resource* passCB = m_FrameResources[frameIndex]->PassCB->Resource();
		D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

		// Heap에서 위치는
		int heapIndex = m_PassCBViewOffset + frameIndex;
		CD3DX12_CPU_DESCRIPTOR_HANDLE handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBViewHeap->GetCPUDescriptorHandleForHeapStart());
		handle.Offset(heapIndex, m_CbvSrvUavDescriptorSize);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
		// 위치를 지정해주고, 사이즈도 넣어주고
		cbvDesc.BufferLocation = cbAddress;
		cbvDesc.SizeInBytes = passCBByteSize;
		// CB Descriptor (View)를 만들어준다.
		m_d3dDevice->CreateConstantBufferView(&cbvDesc, handle);
	}
}

void RenderWaveApp::BuildRootSignature()
{
	// Root Signature를 2개의 CB View Heap을 가지도록 만들 것이다.
	// 
	// 하나는 그냥 solid, 하나는 wireframe
	// view는 하나씩만 들어가고
	CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0); // 각각 레지스터 0번

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1); // 1번에 지정한다.
	
	// Root Signature parameter Slot에 View Range로 View를 등록해준다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[2];
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable0);
	slotRootParameter[1].InitAsDescriptorTable(1, &cbvTable1);

	// Root Signature parameter로 Root Signature를 만든다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(
		2,
		slotRootParameter,
		0,
		nullptr,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);
	
	// 하나의 CB로 만들어진 View Range를 가리키는 하나의 slot으로 root signature를 만든다.
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
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	// 직렬화 한 Binary를 가지고 Root Signature를 최종적으로 생성한다.
	ThrowIfFailed(m_d3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(m_RootSignature.GetAddressOf())
	));
}

void RenderWaveApp::BuildShadersAndInputLayout()
{
#if 1 // 오프라인 컴파일 여부
	m_Shaders["standardVS"] = d3dUtil::CompileShader(L"Shaders\\04_RenderSmoothly_Wave_Shader.hlsl", nullptr, "VS", "vs_5_1");
	m_Shaders["opaquePS"] = d3dUtil::CompileShader(L"Shaders\\04_RenderSmoothly_Wave_Shader.hlsl", nullptr, "PS", "ps_5_1");
#else
	m_vsByteCode = d3dUtil::LoadBinary(L"Shaders\\03_RenderSmoothly_VS.cso");
	m_psByteCode = d3dUtil::LoadBinary(L"Shaders\\03_RenderSmoothly_PS.cso");
#endif
	// 쉐이더에 데이터를 전달해 주기 위한, 레이아웃을 정의한다.

	m_InputLayout =
	{
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
}

void RenderWaveApp::BuildShapeGeometry()
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
		vertices[k].Color = XMFLOAT4(DirectX::Colors::ForestGreen);
	}

	for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = grid.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::DarkGreen); 
	}

	for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = sphere.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::SteelBlue);
	}

	for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k)
	{
		vertices[k].Pos = cylinder.Vertices[i].Position;
		vertices[k].Color = XMFLOAT4(DirectX::Colors::Crimson);
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

void RenderWaveApp::BuildPSOs()
{
	// 일단은 그냥 불투명한 렌더링을 하는 PSO를 만든다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePSODesc;
	ZeroMemory(&opaquePSODesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	// 레이아웃을 넣어주고
	opaquePSODesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	// 2개의 view heap이 들어갈 Root Signature를 넣어주고
	opaquePSODesc.pRootSignature = m_RootSignature.Get();
	// 쉐이더를 설정해주고
	opaquePSODesc.VS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["standardVS"] -> GetBufferPointer()),
		m_Shaders["standardVS"]->GetBufferSize()
	};
	opaquePSODesc.PS =
	{
		reinterpret_cast<BYTE*>(m_Shaders["opaquePS"]->GetBufferPointer()),
		m_Shaders["opaquePS"]->GetBufferSize()
	};
	// 아직 사용하지 않는 렌더링 스테이트를 디폴트로 설정해주고
	opaquePSODesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePSODesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePSODesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePSODesc.SampleMask = UINT_MAX;
	opaquePSODesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePSODesc.NumRenderTargets = 1;
	opaquePSODesc.RTVFormats[0] = m_BackBufferFormat;
	opaquePSODesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	opaquePSODesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	opaquePSODesc.DSVFormat = m_DepthStencilFormat;
	// 만든다
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaquePSODesc, IID_PPV_ARGS(&m_PSOs["opaque"])));

	// 와이어 프레임으로 출력하는 PSO이다.
	// 그냥 대충 복사생성자로 한다.
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueWireframePSODesc = opaquePSODesc;
	opaqueWireframePSODesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(&opaqueWireframePSODesc, IID_PPV_ARGS(&m_PSOs["opaque_wireframe"])));
}

void RenderWaveApp::BuildFrameResources()
{
	// FrameResource를 만들어준다.
	for (int i = 0; i < g_NumFrameResources; i++)
	{
		m_FrameResources.push_back(
			std::make_unique<FrameResource>(
			m_d3dDevice.Get(),
			1,
			(UINT)m_AllItems.size()
		));
	}
}

void RenderWaveApp::BuildRenderItems()
{
	// 이제 원래 있던 Geometry 정보를 가지고,
	// Item 구조체를 이용해서 중복해서 다르게 그려볼 것이다.
	// RenderItem 구조체는 
	// 1. Geometry 포인터와
	// 2. World Mat
	// 3. DrawIndexedInstance 에 필요한 속성을 가지고 있다.

	// 박스 아이템
	std::unique_ptr<RenderItem> boxRitem = std::make_unique<RenderItem>();

	XMStoreFloat4x4(&boxRitem->WorldMat, XMMatrixScaling(2.f, 2.f, 2.f) * XMMatrixTranslation(0.f, 0.5f, 0.f));
	boxRitem->ObjCBIndex = 0;
	boxRitem->Geo = m_Geometries["shapeGeo"].get();
	boxRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRitem->IndexCount = boxRitem->Geo->DrawArgs["box"].IndexCount;
	boxRitem->StartIndexLocation = boxRitem->Geo->DrawArgs["box"].StartIndexLocation;
	boxRitem->BaseVertexLocation = boxRitem->Geo->DrawArgs["box"].BaseVertexLocation;
	m_AllItems.push_back(std::move(boxRitem));

	// 그리드 아이템
	std::unique_ptr<RenderItem> gridRitem = std::make_unique<RenderItem>();

	gridRitem->WorldMat = MathHelper::Identity4x4();
	gridRitem->ObjCBIndex = 1;
	gridRitem->Geo = m_Geometries["shapeGeo"].get();
	gridRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	gridRitem->IndexCount = gridRitem->Geo->DrawArgs["grid"].IndexCount;
	gridRitem->StartIndexLocation = gridRitem->Geo->DrawArgs["grid"].StartIndexLocation;
	gridRitem->BaseVertexLocation = gridRitem->Geo->DrawArgs["grid"].BaseVertexLocation;
	m_AllItems.push_back(std::move(gridRitem));

	// 원기둥과 구 5개씩 2줄
	UINT objCBIndex = 2;
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
		leftCylRitem->ObjCBIndex = objCBIndex++;
		leftCylRitem->Geo = m_Geometries["shapeGeo"].get();
		leftCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftCylRitem->IndexCount = leftCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		leftCylRitem->StartIndexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		leftCylRitem->BaseVertexLocation = leftCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&rightCylRitem->WorldMat, leftCylWorld);
		rightCylRitem->ObjCBIndex = objCBIndex++;
		rightCylRitem->Geo = m_Geometries["shapeGeo"].get();
		rightCylRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightCylRitem->IndexCount = rightCylRitem->Geo->DrawArgs["cylinder"].IndexCount;
		rightCylRitem->StartIndexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].StartIndexLocation;
		rightCylRitem->BaseVertexLocation = rightCylRitem->Geo->DrawArgs["cylinder"].BaseVertexLocation;

		XMStoreFloat4x4(&leftSphereRitem->WorldMat, leftSphereWorld);
		leftSphereRitem->ObjCBIndex = objCBIndex++;
		leftSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		leftSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		leftSphereRitem->IndexCount = leftSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		leftSphereRitem->StartIndexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		leftSphereRitem->BaseVertexLocation = leftSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		XMStoreFloat4x4(&rightSphereRitem->WorldMat, rightSphereWorld);
		rightSphereRitem->ObjCBIndex = objCBIndex++;
		rightSphereRitem->Geo = m_Geometries["shapeGeo"].get();
		rightSphereRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		rightSphereRitem->IndexCount = rightSphereRitem->Geo->DrawArgs["sphere"].IndexCount;
		rightSphereRitem->StartIndexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].StartIndexLocation;
		rightSphereRitem->BaseVertexLocation = rightSphereRitem->Geo->DrawArgs["sphere"].BaseVertexLocation;

		m_AllItems.push_back(std::move(leftCylRitem));
		m_AllItems.push_back(std::move(rightCylRitem));
		m_AllItems.push_back(std::move(leftSphereRitem));
		m_AllItems.push_back(std::move(rightSphereRitem));
	}

	// 지금은 opaque한 친구들 말고 없다.
	for (auto& e : m_AllItems)
		m_OpaqueItems.push_back(e.get());
}

void RenderWaveApp::DrawRenderItems(ID3D12GraphicsCommandList* _cmdList, const std::vector<RenderItem*>& _renderItems)
{
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	ID3D12Resource* objectCB = m_CurrFrameResource->ObjectCB->Resource();

	// 아이템을 렌더링 한다.
	for (size_t i = 0; i < _renderItems.size(); i++)
	{
		RenderItem* rItem = _renderItems[i];
		// 각 Geo 가 가지고 있는 VertexBuffer와
		D3D12_VERTEX_BUFFER_VIEW geoVBuffView = rItem->Geo->VertexBufferView();
		_cmdList->IASetVertexBuffers(0, 1, &geoVBuffView);
		// IndexBuffer를 IA 단계에 넘겨준다.
		D3D12_INDEX_BUFFER_VIEW geoIBuffView = rItem->Geo->IndexBufferView();
		_cmdList->IASetIndexBuffer(&geoIBuffView);
		_cmdList->IASetPrimitiveTopology(rItem->PrimitiveType);

		// Object 와 FrameResource에 대한 heap에서의 CB View의 오프셋을 구한다.
		// 위에서 힙을 만들 때, (물체 개수 + 1) * 프레임 리소스 개수 만큼 버퍼가 들어간 힙을 만들었던게 기억이 나야한다.
		UINT CBVIndex = m_CurrFrameResourceIndex * (UINT)m_OpaqueItems.size() + rItem->ObjCBIndex;
		CD3DX12_GPU_DESCRIPTOR_HANDLE CBVHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBViewHeap->GetGPUDescriptorHandleForHeapStart());
		CBVHandle.Offset(CBVIndex, m_CbvSrvUavDescriptorSize);

		// offset으로 구한 핸들로 Root Signature를 설정해줘서, CB 값이 넘어가도록 해주고
		_cmdList->SetGraphicsRootDescriptorTable(0, CBVHandle);
		// 그린다.
		_cmdList->DrawIndexedInstanced(rItem->IndexCount, 1, rItem->StartIndexLocation, rItem->BaseVertexLocation, 0);
	}
}
