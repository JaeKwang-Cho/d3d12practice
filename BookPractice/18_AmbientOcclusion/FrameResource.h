#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/MathHelper.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// ������Ʈ���� �Ѱ��ִ� ģ���̴�.
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

// ���������� �ѹ����� �Ѱ��ִ� ģ���̴�.
struct PassConstants {

	XMFLOAT4X4 ViewMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvViewMat = MathHelper::Identity4x4();

	XMFLOAT4X4 ProjMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvProjMat = MathHelper::Identity4x4();

	XMFLOAT4X4 VPMat = MathHelper::Identity4x4();
	XMFLOAT4X4 InvVPMat = MathHelper::Identity4x4();

	// App���� ����ؼ� �Ѱ��ִ� �������� �׸��� �ؽ��ķ� �Ѿ�� �׸��� ��ȯ ����̴�.
	XMFLOAT4X4 ShadowMat = MathHelper::Identity4x4();
	// View - Proj - Tex �ѹ濡 ���ִ� ģ����.
	XMFLOAT4X4 ViewProjTexMat = MathHelper::Identity4x4();

	XMFLOAT3 EyePosW = { 0.f, 0.f, 0.f };
	float cbPerPassPad1 = 0.f;
	XMFLOAT2 RenderTargetSize = { 0.f, 0.f };
	XMFLOAT2 InvRenderTargetSize = { 0.f, 0.f };

	float NearZ = 0.f;
	float FarZ = 0.f;
	float TotalTime = 0.f;
	float DeltaTime = 0.f;

	// Ambient Light��
	DirectX::XMFLOAT4 AmbientLight = { 0.0f, 0.0f, 0.0f, 1.0f };

	// ���� ������ �־��ش�.
	// ������ Directional -> Point -> Spot ���̰�
	// �ε����� ������ �ϰ� �ȴ�.
	Light Lights[MaxLights];
};
// Ssao Map �۾��� �� �ʿ��� ģ�����̴�.
struct SsaoConstants 
{
	DirectX::XMFLOAT4X4 ProjMat;
	DirectX::XMFLOAT4X4 InvProjMat;
	DirectX::XMFLOAT4X4 ProjTexMat;
	DirectX::XMFLOAT4   OffsetVectors[14];

	// SsaoBlur ���϶� ���
	DirectX::XMFLOAT4 BlurWeights[3];

	DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };

	// view ���� occlusion �Ӽ�����
	float OcclusionRadius = 0.5f;
	float OcclusionFadeStart = 0.2f;
	float OcclusionFadeEnd = 2.0f;
	float SurfaceEpsilon = 0.05f;
};

// ��ü���� ����ȭ�� �� �� �ֱ� ������,
// ����ü�� ���� �Ѱ��༭, ������ ���ۿ��� �����ϰ� �Ѵ�.
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

// ����� Vertex ����
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

// CPU�� ������ ���� Command List�� �����ϴµ� �ʿ��� �������� �����Ѵ�.
struct FrameResource
{
public:
	FrameResource(ID3D12Device* _device, UINT _passCount, UINT _instanceCount, UINT _materialCount);
	FrameResource(const FrameResource& _other) = delete;
	FrameResource& operator=(const FrameResource& _other) = delete;
	~FrameResource();

	// FrameResource�� ������ �� ���̰�, GPU �۾��� ������ ���� 
	// �̸� �÷��� ���̱� ������ ���� Allocator�� ������.
	ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// GPU�� Commands�� �Ϸ��� ������, CB�� ������Ʈ���� �ʴ´�.
	// �׷��� �̰͵� ���� ������ �ִ´�.
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;

	// RenderItem�� �ϳ����� �ϳ��� ������ ������, ���� �������� �ϰ� �ʹٸ�,
	// �̰� ������ ��������, �׿� �˸´� ������ ������ �� ���̴�.
	// std::unique_ptr<UploadBuffer<InstanceData>> InstanceBuffer = nullptr;

	// �ٽ� Object Constant Buffer�� ���ƿԴ�.
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// Ssao Map�� �ۼ��ϰ�, Ssao Blur�� ���̴µ� ����ϴ� Buffer�̴�.
	std::unique_ptr<UploadBuffer<SsaoConstants>> SsaoCB = nullptr;

	// ���� Constant Buffer�� �ƴ϶� StructuredBuffer���� �Ѱ��ְ�
	// Material ������ Transform�� Index�� �߰��� ����ü�� �Ѱ��ش�.
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	// Ȥ�ó� CPU�� �� ���� ���� �����, Fence�̴�.
	UINT64 Fence = 0;
};
