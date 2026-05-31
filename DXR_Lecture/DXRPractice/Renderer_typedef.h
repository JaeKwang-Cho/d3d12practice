#pragma once
const UINT SWAP_CHAIN_FRAME_COUNT = 2;

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
	float Near;
	float Far;
	UINT MaxRadianceRayRecursionDepth;
	UINT RERSERVED0;
};

enum class CONSTANT_BUFFER_TYPE
{
	RAY_TRACING = 0,
	COUNT
};

struct CONSTANT_BUFFER_PROPERTY
{
	CONSTANT_BUFFER_TYPE cbType;
	UINT cbSize;
};

struct ROOT_ARG 
{
	D3D12_GPU_DESCRIPTOR_HANDLE srvVertexBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE srvIndexBuffer;
	D3D12_GPU_DESCRIPTOR_HANDLE srvTexBuffer;
};

const ULONG MAX_TRIANGLE_COUNT_PER_BLAS = 16;

struct BLAS_BUILD_TRIGROUP_INFO
{
	ID3D12Resource* pIndexBuffer;
	ID3D12Resource* pTexResource;
	ULONG ulIndexNum;
	bool bNotOpaque;
};

struct BLAS_INSTANCE
{
	void* pSrcMeshObj;
	ID3D12Resource* pBLAS;
	XMMATRIX matTransform;

	ULONG ulID;
	UINT uiShaderRecordIndex;
	ULONG ulVertexCount;
	ULONG ulTriGroupCount;
};