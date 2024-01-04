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
	float cbPerObjectPad1 = 0.f;
	XMFLOAT2 RenderTargetSize = { 0.f, 0.f };
	XMFLOAT2 InvRenderTargetSize = { 0.f, 0.f };

	float NearZ = 0.f;
	float FarZ = 0.f;
	float TotalTime = 0.f;
	float DeltaTime = 0.f;
};

// ����� Vertex ����
struct Vertex {
	XMFLOAT3 Pos;
	XMFLOAT4 Color;
};

// CPU�� ������ ���� Command List�� �����ϴµ� �ʿ��� �������� �����Ѵ�.
struct FrameResource
{
public:
	FrameResource(ID3D12Device* _device, UINT _passCount, UINT _objectCount);
	FrameResource(const FrameResource& _other) = delete;
	FrameResource& operator=(const FrameResource& _other) = delete;
	~FrameResource();

	// FrameResource�� ������ �� ���̰�, GPU �۾��� ������ ���� 
	// �̸� �÷��� ���̱� ������ ���� Allocator�� ������.
	ComPtr<ID3D12CommandAllocator> CmdListAlloc;

	// GPU�� Commands�� �Ϸ��� ������, CB�� ������Ʈ���� �ʴ´�.
	// �׷��� �̰͵� ���� ������ �ִ´�.
	std::unique_ptr<UploadBuffer<PassConstants>> PassCB = nullptr;
	std::unique_ptr<UploadBuffer<ObjectConstants>> ObjectCB = nullptr;

	// Ȥ�ó� CPU�� �� ���� ���� �����, Fence�̴�.
	UINT64 Fence = 0;
};

