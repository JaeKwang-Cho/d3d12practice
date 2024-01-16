#include "../Common/d3dApp.h"
#include "../Common/UploadBuffer.h"
#include <DirectXColors.h>

using namespace DirectX;

#define PRAC2 (0)
#define PRAC4 (0)
#define PRAC6 (0)
#define PRAC7 (0)
#define PRAC10 (0)
#define PRAC14 (0)
#define PRAC15 (0)
#define PRAC16 (0)

bool g_bPrac7 = false;

// 우리가 사용할 정보를 가진 Vertex 구조체
#if !PRAC10
struct Vertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};
#else
struct Vertex {
	XMFLOAT3 Pos;
	XMCOLOR Color;
};
#endif

// 연습 문제 2
#if PRAC2
struct VPosData {
	XMFLOAT3 Pos;
};

struct VColorData {
	XMFLOAT4 Color;
};
#endif

// Object에 매 프레임 마다 Shader에 입력될 친구
struct ObjectConstants 
{
	// Rendering Coords를 결정할 WVP mat
	XMFLOAT4X4 WVPmat = MathHelper::Identity4x4();
#if PRAC6 || PRAC14 || PRAC16
	float Time = 0.f;
#endif
#if PRAC16
	XMFLOAT4 PulseColor;
#endif
};

class RenderPracticeApp : public D3DApp
{
public:
	RenderPracticeApp(HINSTANCE hInstance);
	~RenderPracticeApp();

	// 뭐... delete 시키면 편하긴 하다.
	RenderPracticeApp(const RenderPracticeApp& _other) = delete;
	RenderPracticeApp& operator=(const RenderPracticeApp& _other) = delete;

	virtual bool Initialize() override;

private:
	virtual void OnResize() override;
	virtual void Update(const GameTimer& gt) override;
	virtual void Draw(const GameTimer& gt) override;

	// 돌려가면서 Box를 살펴보기 위해 Input을 오버라이딩 한다.
	virtual void OnMouseDown(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseUp(WPARAM _btnState, int _x, int _y) override;
	virtual void OnMouseMove(WPARAM _btnState, int _x, int _y) override;

	void BuildDescriptorHeaps();
	void BuildConstantBuffers();
	void BuildRootSignature();
	void BuildShadersAndInputLayout();
	void BuildBoxGeometry();
	void BuildPSO();

private:
	// Rendering Pipeline에 묶일 자원들을 정의하는데 사용되는 ID3D 변수
	ComPtr<ID3D12RootSignature> m_RootSignature = nullptr;

	// Constant Buffer를 Rendering Pipeline에 Bind 할 때 사용하는 View Heap
	ComPtr<ID3D12DescriptorHeap> m_CBViewHeap = nullptr;

	// 'UploadBuffer'라는 클래스를 이용해서, Constant 버퍼를 좀더 쉽게 관리한다.
	// 내부적으로 버퍼 자원의 생성, 파괴, 메모리 해제 등 처리해준다.
	// 그리고 값을 넣어 줄 수 있는 CopyData()도 제공한다.
	std::unique_ptr<UploadBuffer<ObjectConstants>> m_ObjectCBUpload = nullptr;

	// 'MeshGeometry'라는 클래스를 이용해서, Vertice, Indices, Stride, Size
	// 그리고 ID3DBlob(CPU)과 ID3DResource(GPU, Upload)도 맴버로 가진다.
	std::unique_ptr<MeshGeometry> m_BoxGeometry = nullptr;

	// 컴파일 한 Vertex Shader와 Pixel Shader를 Binary Large Object로 가지고 있는다
	ComPtr<ID3DBlob> m_vsByteCode = nullptr;
	ComPtr<ID3DBlob> m_psByteCode = nullptr;

	// Vertex 구조체를 Shader에게 넘겨주는 layout을 정의한다.
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_InputLayout;

	// Rendering 하는데 필요한 온갖 데이터와, 설정들을
	// 합쳐서 Rendering Pipeline을 최종적으로 제어하는
	// PipeLine State Object
	ComPtr<ID3D12PipelineState> m_PSO = nullptr;

	// WVP mat
	XMFLOAT4X4 m_WorldMat = MathHelper::Identity4x4();
	XMFLOAT4X4 m_ViewMat= MathHelper::Identity4x4();
	XMFLOAT4X4 m_ProjMat = MathHelper::Identity4x4();

	// 카메라 돌리는 용도
	float m_Theta = 1.5f * XM_PI; // 좌우
	float m_Phi = XM_PIDIV4; // 상하
	float m_Radius = 5.f;

	POINT m_LastMousePos = {0, 0};
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
		RenderPracticeApp theApp(hInstance);
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

RenderPracticeApp::RenderPracticeApp(HINSTANCE hInstance)
	: D3DApp(hInstance)
{
}

RenderPracticeApp::~RenderPracticeApp()
{
}

bool RenderPracticeApp::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	// 각종 렌더링 자원이나 속성들을 정의할때도
	// Command List를 사용한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr));

	BuildDescriptorHeaps();
	BuildConstantBuffers();
	BuildRootSignature();
	BuildShadersAndInputLayout();
	BuildBoxGeometry();
	BuildPSO();

	// 초기화 요청이 들어간 Command List를 Queue에 등록한다.
	ThrowIfFailed(m_CommandList->Close());
	ID3D12CommandList* cmdsLists[] = { m_CommandList.Get() };
	m_CommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// flush
	FlushCommandQueue();

	return true;
}

void RenderPracticeApp::OnResize()
{
	D3DApp::OnResize();

	// Projection Mat은 윈도우 종횡비에 영향을 받는다.
	XMMATRIX projMat = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, GetAspectRatio(), 1.f, 1000.f);
	XMStoreFloat4x4(&m_ProjMat, projMat);
}

void RenderPracticeApp::Update(const GameTimer& gt)
{
	// 구심 좌표계 값에 따라 데카르트 좌표계로 변환한다.
	float x = m_Radius * sinf(m_Phi) * cosf(m_Theta);
	float z = m_Radius * sinf(m_Phi) * sinf(m_Theta);
	float y = m_Radius * cosf(m_Phi);

	// View(Camera) Mat을 초기화 한다.
	XMVECTOR pos = XMVectorSet(x, y, z, 1.f); // 카메라 위치
	XMVECTOR target = XMVectorZero(); // 카메라 바라보는 곳
	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f); // 월드(?) 업 벡터

	XMMATRIX viewMat = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_ViewMat, viewMat);

	// WVP mat을 Object Constant 용 Upload Buffer에 올려준다.
	XMMATRIX worldMat = XMLoadFloat4x4(&m_WorldMat);
	XMMATRIX projMat = XMLoadFloat4x4(&m_ProjMat);
	XMMATRIX wvp = worldMat * viewMat * projMat;

	ObjectConstants objConstants;
	XMStoreFloat4x4(&objConstants.WVPmat, XMMatrixTranspose(wvp));
#if PRAC6 || PRAC14 || PRAC16
	objConstants.Time = gt.GetTotalTime();
#endif
#if PRAC1
	objConstants.PulseColor = XMFLOAT4(Colors::DarkBlue);
#endif
	m_ObjectCBUpload->CopyData(0, objConstants);

}

void RenderPracticeApp::Draw(const GameTimer& gt)
{
	// 생성한 Command Allocator를 재사용 하기 위해서 Reset을 거는 것이다.
	// 하지만 Command List에 있는 모든 작업이 끝나야 Reset을 할 수 있다.
	ThrowIfFailed(m_CommandAllocator->Reset());

	// Command List도 초기화를 한다
	// 근데 Command List에 Reset()을 걸려면, 한번은 꼭 Command Queue에 
	// ExecuteCommandList()로 등록이 된적이 있어야 가능하다.

	// 이제는 PSO를 등록하면서 Command List를 초기화 한다.
	ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), m_PSO.Get()));

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

	// ==== 준비된 Box drawing ======
	
	// 초기화때 생성했던 CB 값을 넘겨주기 위한 ViewHeap으로 배열을 하나 만든다.
	ID3D12DescriptorHeap* descriptorHeaps[] = { m_CBViewHeap.Get() };

	// 그리고 그 배열을 GPU에 세팅하도록 Command List에 요청한다.
	m_CommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	// 초기화 때 만들었던 Root Signature도 GPU에 등록한다.
	m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

	// Descriptor Table의 0번 View Heap을
	// Graphics Root Signature로 세팅을 한다.
	m_CommandList->SetGraphicsRootDescriptorTable(0, m_CBViewHeap->GetGPUDescriptorHandleForHeapStart());

	// Vertex 와 Index 정보를 넣어서 Default Buffer로 만들어 놨던 걸
	// Input Assembly 단계에 넣는다.
	D3D12_VERTEX_BUFFER_VIEW VertexBuffView = m_BoxGeometry->VertexBufferView();
	D3D12_INDEX_BUFFER_VIEW IndexBuffView = m_BoxGeometry->IndexBufferView();
	m_CommandList->IASetVertexBuffers(0, 1, &VertexBuffView);
#if PRAC2
	m_CommandList->IASetVertexBuffers(1, 1, &VertexBuffView);
#endif
	m_CommandList->IASetIndexBuffer(&IndexBuffView);

	// 그리고 TRIANGLELIST으로 그리기로 설정한다.
	m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);


	
	// Render Target에 그린다.
	// Box
	if (!g_bPrac7)
	{

		m_CommandList->DrawIndexedInstanced(
			m_BoxGeometry->DrawArgs["Box"].IndexCount, // Indices 수
			1, // 인스턴스 갯수
			0, // 첫번째 인덱스 위치
			0, // Vertex 시작 검색위치
			0 // 인스턴싱 시작 위치
		);
	}
#if PRAC7
	// Pyramid
	else
	{
		m_CommandList->DrawIndexedInstanced(
			m_BoxGeometry->DrawArgs["Pyramid"].IndexCount, // Indices 수
			1, // 인스턴스 갯수
			m_BoxGeometry->DrawArgs["Pyramid"].StartIndexLocation,
			m_BoxGeometry->DrawArgs["Pyramid"].BaseVertexLocation,
			0 // 인스턴싱 시작 위치
		);
	}
		
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

	// GPU의 작업이 모두 끝날 때 까지 Fence를 이용해서 기다리는
	// Flush 함수를 통해 CPU, GPU 동기화를 한다.
	FlushCommandQueue();
}

void RenderPracticeApp::OnMouseDown(WPARAM _btnState, int _x, int _y)
{
	// 마우스의 위치를 기억하고
	m_LastMousePos.x = _x;
	m_LastMousePos.y = _y;
	// 마우스를 붙잡는다.
	SetCapture(m_hMainWnd);
}

void RenderPracticeApp::OnMouseUp(WPARAM _btnState, int _x, int _y)
{
	// 마우스를 놓는다.
	ReleaseCapture();
#if PRAC7
	g_bPrac7 = g_bPrac7 ? false : true;
#endif
}

void RenderPracticeApp::OnMouseMove(WPARAM _btnState, int _x, int _y)
{
	// 왼쪽 마우스가 눌린 상태에서 움직인다면
	if ((_btnState & MK_LBUTTON) != 0)
	{
		// 마우스가 움직인 픽셀의 1/4 만큼 1 radian 으로 바꿔서 세타와 피에 적용시킨다.
		float dx = XMConvertToRadians(0.25f * static_cast<float>(_x - m_LastMousePos.x));
		float dy = XMConvertToRadians(0.25f * static_cast<float>(_y - m_LastMousePos.y));

		m_Theta += dx;
		m_Phi += dy;

		// Phi는 0 ~ Pi 구간을 유지하도록 한다.
		m_Phi = MathHelper::Clamp(m_Phi, 0.1f, MathHelper::Pi - 0.1f);
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

void RenderPracticeApp::BuildDescriptorHeaps()
{
	// ===Constant Buffer를 Pipeline에 전달하기 위한==
	// =======Descriptor(View) Heap을 만드는 것=======

	D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {};
	cbvHeapDesc.NumDescriptors = 1;
	// 이게 Constant Buffer 용이다.
	cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV; 
	// Shader가 참조할 수 있도록 Command List에 Bind 된다.
	cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE; 
	cbvHeapDesc.NodeMask = 0; // 다중 어뎁터를 할 일이 생길까?

	// 그리고 어뎁터로 CB를 위한 View Heap 생성을 한다.
	ThrowIfFailed(m_d3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&m_CBViewHeap)));
}

void RenderPracticeApp::BuildConstantBuffers()
{
	// Descriptor(View)을 타고, Pipe line에 넘어갈 Buffer를 만든다.

	m_ObjectCBUpload = std::make_unique<UploadBuffer<ObjectConstants>>(m_d3dDevice.Get(), 1, true);
	// CPU 에서도 접근 할 수 있는 Upload Heap에 Buffer를 만들었고,
	// 내부적으로 가지고 있는 ID3D12Resource 와 Map()로 
	// Resource의 Pointer를 얻어내서 (요것도 내부적으로 가진다. 변하지 않는 값이다.)
	// Constant Buffer의 값을 바꿀 수 있다.
	
	// 256 바이트 align을 하고
	UINT objCBByteSize = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// (ID3D12Resource::GetGPUVirtualAddress - Buffer Resource에만 유효하다.)
	// GPU buffer의 시작 주소를 저장하고
	D3D12_GPU_VIRTUAL_ADDRESS cbAddress = m_ObjectCBUpload->Resource()->GetGPUVirtualAddress();
	int boxCBIndex = 0;
	cbAddress += boxCBIndex * objCBByteSize; // 오프셋을 설정하고

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {0};
	cbvDesc.BufferLocation = cbAddress; // Buffer의 GPU 가상시작주소를 View Desc에 등록한다.
	cbvDesc.SizeInBytes = d3dUtil::CalcConstantBufferByteSize(sizeof(ObjectConstants));

	// 그리고 Buffer View를 만든다.
	m_d3dDevice->CreateConstantBufferView(
		&cbvDesc,
		m_CBViewHeap->GetCPUDescriptorHandleForHeapStart()
	);

	// 그니까... Constant Buffer의 View가 들어가는 Heap을 만들고.. 
	// 그 Buffer의 사이즈는 256 배수로 만들 수 있고,
	// https://learn.microsoft.com/ko-kr/windows/win32/direct3d12/hardware-support
	// 이걸 보면 Buffer View가 엄청 많이 들어간다는 걸 알 수 있다.
	// 보니깐 offset으로 접근하는 것 같다.
}

void RenderPracticeApp::BuildRootSignature()
{
	// Shader가 Resource를 받으려면, Rendering Pipeline에 알맞은 형태로 
	// Bind가 되어있어야 한다. Root Signature가 그 역할을 한다.
	// (Shader에게 어떤 Resource가 들어갈지 레지스터 번호로써 정의해준다.)

	// ROOT_PARAMETER의 타입은 Table, Contant, Descriptor가 가능하다.
	CD3DX12_ROOT_PARAMETER slotRootParameter[1];

	// 현재 D3D12_ROOT_PARAMETER_TYPE은 CBV가 하나들어가는 descriptor table로 정의한다.
	// D3D12_ROOT_DESCRIPTOR_TABLE
	CD3DX12_DESCRIPTOR_RANGE cbvTable;
	cbvTable.Init(
		D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 
		1, // view 개수
		0 // 등록할 Shader 레지스터 번호
	);
	// 그리고 파라미터를 CB view로 초기화 하면서 테이블에 등록한다.
	slotRootParameter[0].InitAsDescriptorTable(1, &cbvTable);

	// ROOT_SIGNATURE는 ROOT_PARAMETER으로 이루어진 배열이다.
	CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(
		1,
		slotRootParameter,
		0, // 샘플러는 
		nullptr, // 지금 안쓴다.
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT
	);

	// 이제 Constant Buffer가 하나 들어갈 descriptor range를 가리키는,
	// slot을 하나를 가지는 Root Signature에 넘겨줄 수 있는
	// 직렬화 된, Root Signature를 만든다.
	ComPtr<ID3DBlob> serializedRootSignature = nullptr;
	ComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(
		&rootSignatureDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSignature.GetAddressOf(),
		errorBlob.GetAddressOf()
	);

	if (errorBlob != nullptr)
	{
		OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	// 직렬화된 RootSignatrue를 넘겨준다.
	ThrowIfFailed(m_d3dDevice->CreateRootSignature(
		0, // 어뎁터는 하나다. (다중 어뎁터 지원)
		serializedRootSignature->GetBufferPointer(),
		serializedRootSignature->GetBufferSize(),
		IID_PPV_ARGS(&m_RootSignature)
	));
}

void RenderPracticeApp::BuildShadersAndInputLayout()
{
	// 쉐이더를 컴파일 해주고
	HRESULT hr = S_OK;

#if PRAC6 
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC6" , "1",
		NULL, NULL
	};

	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "PS", "ps_5_1");
#elif PRAC10
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC10" , "1",
		NULL, NULL
	};

	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "PS", "ps_5_1");
#elif PRAC14
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC14" , "1",
		NULL, NULL
	};

	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "PS", "ps_5_1");
#elif PRAC15
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC15" , "1",
		NULL, NULL
	};

	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "PS", "ps_5_1");
#elif PRAC16
	const D3D_SHADER_MACRO defines[] =
	{
		"PRAC16" , "1",
		NULL, NULL
	};

	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", defines, "PS", "ps_5_1");

#else
	m_vsByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", nullptr, "VS", "vs_5_1");
	m_psByteCode = d3dUtil::CompileShader(L"Shaders\\02_Rendering_Shader.hlsl", nullptr, "PS", "ps_5_1");
#endif

#if 0
	m_vsByteCode = d3dUtil::LoadBinary(L"Shaders\\02_Rendering_VS.cso");
	m_psByteCode = d3dUtil::LoadBinary(L"Shaders\\02_Rendering_PS.cso");
#endif`
	// 쉐이더에 데이터를 전달해 주기 위한, 레이아웃을 정의한다.

#if PRAC2
	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

#elif PRAC10
	m_InputLayout =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_B8G8R8A8_UNORM , 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
#else
	m_InputLayout =
	{
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};
#endif

	// 연습문제 1
	/*
	D3D12_INPUT_ELEMENT_DESC practice1[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3) * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 3, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) * 3 + sizeof(XMFLOAT2), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3) * 3 + sizeof(XMFLOAT2) * 2, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
	*/
}

void RenderPracticeApp::BuildBoxGeometry()
{
	// 우리가 그릴 도형를 정의한다.
#if PRAC4
	std::array<Vertex, 5> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT4(Colors::Red) }),
	};

	std::array<std::uint16_t, 18> indices =
	{
		// front face
		0, 4, 1,

		// back face
		2, 3, 4,

		// left face
		0, 2, 4,

		// right face
		1, 4, 3,

		// bottom face
		0, 1, 3,
		0, 3, 2
	};

#elif PRAC10
	uint32_t	ui32_white = 0xffffffff;
	uint32_t	ui32_Black = 0xff000000;
	uint32_t	ui32_Red = 0xffff0000;
	uint32_t	ui32_Green = 0xff00ff00;
	uint32_t	ui32_Blue = 0xff0000ff;
	uint32_t	ui32_Yellow = 0xffffff00;
	uint32_t	ui32_Cyan = 0xff00ffff;
	uint32_t	ui32_Magenta = 0xffff00ff;
	uint32_t	ui32_Test = 0xbb884400;


	std::array<Vertex, 8> vertices =
	{
		
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMCOLOR(ui32_white)}),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMCOLOR(ui32_Black)}),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMCOLOR(ui32_Red)}),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMCOLOR(ui32_Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMCOLOR(ui32_Blue)}),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMCOLOR(ui32_Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMCOLOR(ui32_Cyan)}),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMCOLOR(ui32_Magenta)})
	};		

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
#else
	std::array<Vertex, 8> vertices =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};

	std::array<std::uint16_t, 36> indices =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
	};
#endif

#if PRAC7
	std::array<Vertex, 5> vertices_Pyramid =
	{
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT4(Colors::Red) }),
	};

	std::array<std::uint16_t, 18> indices_Pyramid =
	{
		// front face
		0, 4, 1,

		// back face
		2, 3, 4,

		// left face
		0, 2, 4,

		// right face
		1, 4, 3,

		// bottom face
		0, 1, 3,
		0, 3, 2
	};
#endif

	// 내부적으로 Resource 관리를 편하게 하고, Rendering Pipeline에 등록을 편하게 해주는
	// MeshGeometry 클래스를 이용한다.
	m_BoxGeometry = std::make_unique<MeshGeometry>();
	m_BoxGeometry->Name = "Box_Geo";

	// ByteSize를 구하고
	const UINT vbByteSize = (UINT)vertices.size() * sizeof(Vertex);
	const UINT ibByteSize = (UINT)indices.size() * sizeof(std::uint16_t);

	// CPU에 올릴 Buffer를 Blob으로 만들어주고
	UINT vbByteSize_Total = vbByteSize;
	UINT ibByteSize_Total = ibByteSize;
#if PRAC7
	const UINT vbByteSize_Pyramid = (UINT)vertices_Pyramid.size() * sizeof(Vertex);
	const UINT ibByteSize_Pyramid = (UINT)indices_Pyramid.size() * sizeof(std::uint16_t);

	vbByteSize_Total += vbByteSize_Pyramid;
	ibByteSize_Total += ibByteSize_Pyramid;
#endif

	ThrowIfFailed(D3DCreateBlob(vbByteSize_Total, &(m_BoxGeometry->VertexBufferCPU)));
	ThrowIfFailed(D3DCreateBlob(ibByteSize_Total, &(m_BoxGeometry->IndexBufferCPU)));

	// memcpy를 이용해, Blob에 진짜로 값을 복사해준다.
	BYTE* VertexBufferPointer = static_cast<BYTE*>(m_BoxGeometry->VertexBufferCPU->GetBufferPointer());
	BYTE* IndexBufferPointer = static_cast<BYTE*>(m_BoxGeometry->IndexBufferCPU->GetBufferPointer());

	CopyMemory(VertexBufferPointer, vertices.data(), vbByteSize);
	CopyMemory(VertexBufferPointer, indices.data(), ibByteSize);

#if PRAC7
	CopyMemory(VertexBufferPointer + vbByteSize, vertices_Pyramid.data(), vbByteSize_Pyramid);
	CopyMemory(VertexBufferPointer + ibByteSize, indices_Pyramid.data(), ibByteSize_Pyramid);

	std::array<Vertex, (UINT)vertices.size() + (UINT)vertices_Pyramid.size()> vertices_Total;
	std::array<std::uint16_t, (UINT)indices.size() + (UINT)indices_Pyramid.size()> Indices_Total;

	auto verIter = std::copy(vertices.cbegin(), vertices.cend(), vertices_Total.begin());
	std::copy(vertices_Pyramid.cbegin(), vertices_Pyramid.cend(), verIter);

	auto IdxIter = std::copy(indices.cbegin(), indices.cend(), Indices_Total.begin());
	std::copy(indices_Pyramid.cbegin(), indices_Pyramid.cend(), IdxIter);
#else
	std::array<Vertex, (UINT)vertices.size()> vertices_Total = vertices;
	std::array<std::uint16_t, (UINT)indices.size()> Indices_Total = indices;
#endif


	// 이제 GPU에도 Upload heap을 타고가서 Default Buffer로
	// Geometry 정보를 올려준다.
	m_BoxGeometry->VertexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		vertices_Total.data(),
		vbByteSize_Total,
		// 이렇게 Upload Resource를 따로 가지고 있는 이유는
		m_BoxGeometry->VertexBufferUploader 
		// command list 로 작업이 이뤄지고, GPU가 언제 복사를
		// 마칠지 모르기 때문에 함부로 파괴가 안되도록 가지고 있는 것이다.
	);

	m_BoxGeometry->IndexBufferGPU = d3dUtil::CreateDefaultBuffer(
		m_d3dDevice.Get(),
		m_CommandList.Get(),
		Indices_Total.data(),
		ibByteSize_Total,
		// 요것도 마찬가지로, 함부로 파괴가 되지 않도록 가지고 있는 것
		m_BoxGeometry->IndexBufferUploader
	);

	// GPU에서 정보를 잘 읽기 위한
	// 속성 정보를 등록해주고
	m_BoxGeometry->VertexByteStride = sizeof(Vertex);
	m_BoxGeometry->VertexBufferByteSize = vbByteSize_Total;
	m_BoxGeometry->IndexFormat = DXGI_FORMAT_R16_UINT;
	m_BoxGeometry->IndexBufferByteSize = ibByteSize_Total;


	// 현재 버퍼에는 메쉬가 하나만 들어있기 때문에
	// 따로 지정할 오프셋은 존재하지 않는다.
	SubmeshGeometry subMesh;
	subMesh.IndexCount = (UINT)indices.size();
	subMesh.StartIndexLocation = 0;
	subMesh.BaseVertexLocation = 0;

	// 서브 메쉬는 이렇게 Map에 저장을 하게 된다.
	m_BoxGeometry->DrawArgs["Box"] = subMesh;

#if PRAC7
	SubmeshGeometry subMesh_Pyramid;
	subMesh_Pyramid.IndexCount = (UINT)indices_Pyramid.size();
	subMesh_Pyramid.StartIndexLocation = (UINT)indices.size();
	subMesh_Pyramid.BaseVertexLocation = (UINT)vertices.size();
	
	m_BoxGeometry->DrawArgs["Pyramid"] = subMesh_Pyramid;
#endif
}

void RenderPracticeApp::BuildPSO()
{
	// 이제 각종 리소스와, 쉐이더, 상태 등등을
	// 한번에 제어하는 Pipeline State Object를 정의한다.

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc;
	memset(&psoDesc, 0, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	// input layout
	psoDesc.InputLayout = { m_InputLayout.data(), (UINT)m_InputLayout.size() };
	// Root Signature (CB)
	psoDesc.pRootSignature = m_RootSignature.Get();
	// Vertex Shader
	psoDesc.VS = {
		reinterpret_cast<BYTE*>(m_vsByteCode->GetBufferPointer()),
		m_vsByteCode->GetBufferSize()
	};
	// Pixel Shader
	psoDesc.PS = {
		reinterpret_cast<BYTE*>(m_psByteCode->GetBufferPointer()),
		m_psByteCode->GetBufferSize()
	};
	// Rasterizer State (일단은 디폴트)
	D3D12_RASTERIZER_DESC rasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	//rasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
	//rasterizerState.CullMode = D3D12_CULL_MODE_FRONT;
	psoDesc.RasterizerState = rasterizerState;
	// Blend State (일단은 디폴트)
	psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	// Depth-Stencil State (일단은 디폴트)
	psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	// Sampling Mask 기본값 (어떤 것도 비활성화 하지 않는다.)
	psoDesc.SampleMask = UINT_MAX;
	// 삼각형으로 그리기
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	// 동시에 그리는 Rendertarget 개수
	psoDesc.NumRenderTargets = 1;
	// Render Target Format (지정한 것과 같은 포맷을 가져야 한다.)
	psoDesc.RTVFormats[0] = m_BackBufferFormat;
	// multi sampling 수준 (지정한 것과 같은 포맷을 가져야 한다.)
	psoDesc.SampleDesc.Count = m_4xMsaaState ? 4 : 1;
	psoDesc.SampleDesc.Quality = m_4xMsaaState ? (m_4xMsaaQuality - 1) : 0;
	// Depth-Stencil Format (지정한 것과 같은 포맷을 가져야 한다.)
	psoDesc.DSVFormat = m_DepthStencilFormat;

	// 어뎁터에서 위 정보를 가지고 PSO를 생성한다.
	ThrowIfFailed(m_d3dDevice->CreateGraphicsPipelineState(
		&psoDesc,
		IID_PPV_ARGS(&m_PSO)
	));
}
