#pragma once

class ShaderTable;
class D3D12Renderer;
struct SHADER_HANDLE;

class RayTracingManager
{
private:
	enum class COMMON_DESCRIPTOR_INDEX {
		OUTPUT_DIFFUSE_UAV = 0, // UAV - Output - Diffuse
		OUTPUT_DEPTH_UAV,   // UAV - Output - Depth
		Count,
	};

	enum class DISPATCH_DESCRIPTOR_INDEX {
		RAYTRACING_CBV = 0,
		OUTPUT_DIFFUSE,
		OUTPUT_DEPTH,
		Count,
	};

	static const DWORD MAX_RECURSION_DEPTH = 1;
	static const DWORD MAX_RADIANCE_RECURSION_DEPTH = std::min<DWORD>(MAX_RECURSION_DEPTH, 1);

public:
	bool Initialize(D3D12Renderer* _pRenderer, UINT _ulWidth, UINT _ulHeight);

	void DoRayTracing(D3D12GraphicsCommandList_raw _pCommandList);
	void UpdateWindowSize_forRayTracing(UINT _ulWidth, UINT _ulHeight);

private:
	void CreateCommandList_forRayTracing();

	void CreateFence_forRayTracing();
	UINT64 DoFence_forRayTracing();
	void WaitForFenceValue_forRayTracing();
	void CleanupFence_forRayTracing();

	void BuildShaderTable();

	void CreateRootSignatures();
	void CreateRaytracingPipelineStateObject();

	void CreateDescriptorHeapCBV_SRV_UAV();
	void CreateShaderVisibleHeap();

	bool CreateOutputDiffuseBuffer(UINT _uiWidth, UINT _uiHeight);
	void CleanupOutputDiffuseBuffer();
	bool CreateOutputDepthBuffer(UINT _uiWidth, UINT _uiHeight);
	void CleanupOutputDepthBuffer();

	void CleanupRayTracingManager();

private:
	// 다른데서 만든걸 참조
	D3D12Renderer* m_pRenderer = nullptr;
	D3D12Device_raw m_pD3DDevice = nullptr;

	// RayTracingManager이 소유 - start
	D3D12CommandQueue_ptr m_pCommandQueue = nullptr;
	D3D12CommandAllocator_ptr m_pCommandAllocator = nullptr;
	D3D12GraphicsCommandList_ptr m_pCommandList = nullptr;
	
	HANDLE m_hFenceEvent = nullptr;
	D3D12Fence_ptr m_pFence = nullptr;
	UINT64 m_ui64FenceValue = 0;

	// ray tracing 결과를 저장할 버퍼. UAV로 바인딩해서 사용한다.
	D3D12Resource_ptr m_pOutputDiffuseBuffer = nullptr; 
	D3D12Resource_ptr m_pOutputDepthBuffer = nullptr;
	UINT m_ulWidth = 0;
	UINT m_ulHeight = 0;

	SHADER_HANDLE* m_pRayShaderHandle = nullptr;

	// RootSignature과 StateObject도 RayTracingManager이 소유한다.
	D3D12RootSignature_ptr m_pRaytracingGlobalRootSignature = nullptr;
	D3D12StateObject_ptr m_pDXRStateObject = nullptr;

	D3D12DescriptorHeap_ptr m_pCommonDescriptorHeap = nullptr;
	D3D12DescriptorHeap_ptr m_pShaderVisibleDescriptorHeap = nullptr; // ID에 따라 srv, uav 위치 고정.
	UINT m_DescriptorSize = 0;

	// ShaderTable도 RayTracingManager이 소유한다.
	std::unique_ptr<ShaderTable> m_pRayGenShaderTable = nullptr;
	UINT m_ShaderIdentifierSize = D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;

	// RayTracingManager이 소유 - end
public:
	D3D12Resource_raw INL_GetOutputResource() { return m_pOutputDiffuseBuffer.Get(); }
	D3D12Resource_raw INL_GetDepthResource() { return m_pOutputDepthBuffer.Get(); }

	RayTracingManager();
	virtual ~RayTracingManager();

};

