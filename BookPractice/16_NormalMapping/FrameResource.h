#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/MathHelper.h"

#define PRAC5 (1)

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// 인스턴스마다 넘겨주는 친구이다.
/*
struct InstanceData 
{
	XMFLOAT4X4 WorldMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvWorldMat = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex;
	UINT instPad0;
	UINT instPad1;
	UINT instPad2;
};
*/

// 오브젝트마다 넘겨주는 친구이다.
struct ObjectConstants
{
	XMFLOAT4X4 WorldMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvWorldMat = MathHelper::Identity4x4();
	XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	UINT MaterialIndex;
	UINT objPad0;
	UINT objPad1;
	UINT objPad2;
};

// 렌더링마다 한번씩만 넘겨주는 친구이다.
struct PassConstants {

	XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvViewMat = MathHelper::Identity4x4();

	XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProjMat = MathHelper::Identity4x4();

	XMFLOAT4X4 VPMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvVPMat = MathHelper::Identity4x4();

	XMFLOAT3 EyePosW = { 0.f, 0.f, 0.f };
	float cbPerPassPad1 = 0.f;
	XMFLOAT2 RenderTargetSize = { 0.f, 0.f };
	XMFLOAT2 InvRenderTargetSize = { 0.f, 0.f };

	float NearZ = 0.f;
	float FarZ = 0.f;
	float TotalTime = 0.f;
	float DeltaTime = 0.f;

	// Ambient Light와
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// Pass에서 Fog 정보를 넘겨준다.
	DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.f };

	float FogStart = 5.f;
	float FogRange = 150.f;
	DirectX::XMFLOAT2 cbPerPassPad2 = { 0.f, 0.f };

	// 조명 정보를 넣어준다.
	// 순서는 Directional -> Point -> Spot 순이고
	// 인덱스로 구분을 하게 된다.
	Light Lights[MaxLights];
};

// 물체별로 색인화를 할 수 있기 때문에,
// 구조체를 만들어서 넘겨줘서, 구조적 버퍼에서 참조하게 한다.
struct MaterialData
{
	DirectX::XMFLOAT4 DiffuseAlbedo = { 1.f, 1.f, 1.f, 1.f };
	DirectX::XMFLOAT3 FresnelR0 = { 0.01f, 0.01f, 0.01f };
	float Roughness = 64.f;

	DirectX::XMFLOAT4X4 MaterialTransform = MathHelper::Identity4x4();

	UINT DiffuseMapIndex = 0;
	UINT NormalMapIndex = 0;
	UINT DispMapIndex = 0;
	UINT MaterialPad0;
};

// 사용할 Vertex 정보
struct Vertex {
	Vertex() = default;
	Vertex(float x, float y, float z, float nx, float ny, float nz, float u, float v, float tx, float ty, float tz) :
		Pos(x, y, z),
		Normal(nx, ny, nz),
		TexC(u, v),
		TangentU(tx, ty, tz)
	{}
	Vertex(XMFLOAT3 _Pos, XMFLOAT3 _Normal, XMFLOAT2 _TexC, XMFLOAT3 _TangentU)
		: Pos(_Pos), Normal(_Normal), TexC(_TexC), TangentU(_TangentU)
	{ }
	XMFLOAT3 Pos;
	XMFLOAT3 Normal;
	XMFLOAT2 TexC;
	XMFLOAT3 TangentU;
};

// CPU가 프레임 마다 Command List를 구성하는데 필요한 정보들을 저장한다.
struct FrameResource
{
public:
	FrameResource(ID3D12Device* _device, UINT _passCount, UINT _instanceCount, UINT _materialCount);
	FrameResource(const FrameResource& _other) = delete;
	FrameResource& operator=(const FrameResource& _other) = delete;
	~FrameResource();

	// FrameResource를 여러개 둘 것이고, GPU 작업이 끝나기 전에 
	// 미리 올려둘 것이기 때문에 각자 Allocator를 가진다.
	ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// GPU가 Commands를 완료할 때까지, CB를 업데이트하지 않는다.
	// 그래서 이것도 각자 가지고 있는다.
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;

	// RenderItem이 하나여서 하나만 가지고 있지만, 만약 여러개를 하고 싶다면,
	// 이걸 여러개 가지던가, 그에 알맞는 구조를 가져야 할 것이다.
	// std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;

	// 다시 Object Constant Buffer로 돌아왔다.
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// 이제 Constant Buffer가 아니라 StructuredBuffer으로 넘겨주고
	// Material 정보에 Transform과 Index를 추가한 구조체를 넘겨준다.
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	// 혹시나 CPU가 더 빠를 때를 대비한, Fence이다.
	UINT64 Fence = 0;
};

