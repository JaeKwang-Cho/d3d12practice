#pragma once
#include "D3D12_SmartPointer_typedef.h"

const UINT SWAP_CHAIN_FRAME_COUNT = 3;
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;

// RadiancePayload (Raytracing_typedef.hlsl) 의 C++ 미러 구조체
// HLSL의 RadiancePayload와 레이아웃을 반드시 동일하게 유지해야 한다.
struct RadiancePayload_Mirror
{
	float radiance[3];          // float3 radiance
	float depth;                // float depth
	UINT  rayRecursionDepth;    // uint rayRecursionDepth
};

static_assert(sizeof(RadiancePayload_Mirror) == 20,
	"RadiancePayload_Mirror size mismatch! At Raytracing_typedef.hlsl, Size must be equal to RadiancePayload.");

const UINT PAYLOAD_SIZE = sizeof(RadiancePayload_Mirror);

struct CONSTANT_BUFFER_RAY_TRACING
{
	XMMATRIX matViewProj;
	XMMATRIX matViewInv;
	DECOMP_PROJ DecompProj;
	XMVECTOR vCameraPos;
	float Near;
	float Far;
	UINT MaxRadianceRayRecursionDepth;
	UINT RERSERVED0;
};

struct CONSTANT_BUFFER_DEFAULT
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProj;
};

struct CONSTANT_BUFFER_SPRITE
{
	XMFLOAT2 ScreenRes;
	XMFLOAT2 Pos;
	XMFLOAT2 Scale;
	XMFLOAT2 TexSize;
	XMFLOAT2 TexSamplePos;
	XMFLOAT2 TexSampleSize;

	float Z;
	float Alpha;
	float Reserved0;
	float Reserved1;
};

enum class CONSTANT_BUFFER_TYPE
{
	DEFAULT = 0,
	SPRITE,
	RAY_TRACING,
	COUNT
};

struct CONSTANT_BUFFER_PROPERTY
{
	CONSTANT_BUFFER_TYPE cbType;
	UINT cbSize;
};

struct TEXTURE_HANDLE
{
	D3D12Resource_ptr pTexResource;
	D3D12Resource_ptr pUploadBuffer;
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle;
	bool bUpdated;
	bool bFromFile;
	ULONG ulRefCount;
	void* pSearchHandle;
};

struct FONT_HANDLE
{
	IDWriteTextFormat* pTextFormat;
	float fFontSize;
	WCHAR wchFontFamilyName[512];
};

struct CONSTANT_BUFFER_RT_TRIGROUP
{
	float Reserved0;
	float Reserved1;
	float Reserved2;
	float Reserved3;
	float Reserved4;
	float Reserved5;
	float Reserved6;
	float Reserved7;
};

// Geometry에 대해서 Hit Group Shader가 실행 될 때, Local Root Signature에 전달할 정보 구조체.
struct ROOT_ARG 
{
	CONSTANT_BUFFER_RT_TRIGROUP cbTrigroup;
	D3D12_GPU_DESCRIPTOR_HANDLE srvVertexBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE srvIndexBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE srvTexBuffer;
};

const ULONG MAX_TRIGROUP_COUNT_PER_BLAS = 16;

// BLAS를 빌드 할 때, 전달해야할	정보 구조체. 
// BLAS 빌드 함수에 전달된다.
// 일단 기본은 BLAS를 만들때는 index buffer와 vertex buffer가 필요하다.
struct BLAS_BUILD_TRIGROUP_INFO
{
	ID3D12Resource* pIndexBuffer;
	TEXTURE_HANDLE* pDiffuseTexHandle;
	ULONG ulIndexNum;
	bool bNotOpaque;
};
// RayTracing Manager가 계속 들고있으면서 관리한다.
// Acceleration Structure와 Geometry의 연결정보를 담는다.
struct BLAS_INSTANCE
{
	void* pSrcMeshObj; // 메쉬
	ID3D12Resource* pBLAS; // BLAS는 ID3D12Resource로 만들어진다. (TLAS도 마찬가지)
	XMMATRIX matTransform; // TLAS에서 instance desc를 만들 때, BLAS에 적용할 월드 변환 행렬.

	ULONG ulID; // (지금은 쓰이지 않지만) 오브젝트 별로 머테리얼을 다르게 적용한다던가, 특정 프로퍼티를 주고 싶을 때 사용한다.
	UINT uiShaderRecordIndex; // 렌더링할 때, BLAS의 총 갯수가 바뀌거나 , BLAS의 순서가 바뀌는 경우가 있을 수 있다. 그럴 때, ShaderTable에서 BLAS에 해당하는 ShaderRecord의 인덱스가 바뀌게 된다. uiShaderRecordIndex는 BLAS와 ShaderRecord의 연결고리 역할을 한다.
	ULONG ulVertexCount;
	ULONG ulTriGroupCount; // Tri라고 되어있지만, 같은 텍스쳐를 사용하는 geometry의 그룹이 여러 개일 수 있다.

	// Local Parameter
	D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle; // BLAS에 대한 SRV가 바인딩된 CPU descriptor handle. Local Root Signature에 전달된다.
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle; // BLAS에 대한 SRV가 바인딩된 GPU descriptor handle. Local Root Signature에 전달된다.
	std::vector<ROOT_ARG> rootArgs; // BLAS가 참조하는 geometry description이 여러 개일 수 있다. (예시에서는 하나의 geometry description만 있지만, 일반적으로는 여러 개가 있을 수 있다.) geometry description마다 root argument가 필요하다. 그래서 배열로 만들어준다. (예시에서는 1개지만, 일반적으로는 여러 개가 있을 수 있다.)

	virtual ~BLAS_INSTANCE() { if (pBLAS) pBLAS->Release(); }
};