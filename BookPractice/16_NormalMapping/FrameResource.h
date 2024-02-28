#pragma once
#include "../Common/d3dUtil.h"
#include "../Common/UploadBuffer.h"
#include "../Common/MathHelper.h"

#define PRAC5 (1)

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// �ν��Ͻ����� �Ѱ��ִ� ģ���̴�.
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

	// Pass���� Fog ������ �Ѱ��ش�.
	DirectX::XMFLOAT4 FogColor = { 0.7f, 0.7f, 0.7f, 1.f };

	float FogStart = 5.f;
	float FogRange = 150.f;
	DirectX::XMFLOAT2 cbPerPassPad2 = { 0.f, 0.f };

	// ���� ������ �־��ش�.
	// ������ Directional -> Point -> Spot ���̰�
	// �ε����� ������ �ϰ� �ȴ�.
	Light Lights[MaxLights];
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

	// ���� Constant Buffer�� �ƴ϶� StructuredBuffer���� �Ѱ��ְ�
	// Material ������ Transform�� Index�� �߰��� ����ü�� �Ѱ��ش�.
	std::unique_ptr<UploadBuffer<MaterialData>> MaterialBuffer = nullptr;

	// Ȥ�ó� CPU�� �� ���� ���� �����, Fence�̴�.
	UINT64 Fence = 0;
};

