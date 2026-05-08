#pragma once

class D3D12Renderer;

enum class E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ
{
	CBV = 0,
	TEX,
	MAT,
	END
};

struct SubmeshRange
{
	UINT startPosIndex;
	UINT posIndexCount;
	UINT startIndexIndex;
	UINT indexIndexCount;
};

class TextureRenderMesh
{
public:
	static const UINT MAX_SUB_RENDER_GEO_COUNT = 8;
protected:
	// 상속받으면서 바꿔주면 되지 않을까?
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static DWORD m_dwInitRefCount;

public:
	bool Initialize(D3D12Renderer* _pRenderer, D3D_PRIMITIVE_TOPOLOGY _primitiveTopoloy = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	void Draw(D3D12GraphicsCommandList_ptr _pCommandList, const XMMATRIX* _pMatWorld);
	void DrawOutline(D3D12GraphicsCommandList_ptr _pCommandList, const XMMATRIX* _pMatWorld);

	void CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount);
	[[deprecated("Use CreateRenderAssetsFromSingleMesh(...) instead.")]]
	void CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount, std::vector<uint32_t>& _adjIndices);
	bool CreateRenderAssetsFromSingleMesh(
		const TextureMeshData& mesh,
		const std::vector<uint32_t>& adjIndices,
		const std::vector<SubmeshRange>& ranges);
	void BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex);
	void SetMaterial(CONSTANT_BUFFER_MATERIAL& _MaterialData, const UINT _subRenderAssetIndex);

protected:
	bool InitCommonResources();
	void CleanupSharedResources();

	virtual bool InitRootSignature();
	virtual bool InitPipelineState();
	bool InitPipelineState_Outline();

	bool BuildOutlineGeoFromMesh(const TextureMeshData& mesh, const std::vector<uint32_t>& adjIndices);
	void CleanUpAssets();
private:

public:
protected:
	D3D12Renderer* m_pRenderer;
	D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopoloy;

	// sub - render asset
	SubRenderGeometry* m_subRenderGeometries[MAX_SUB_RENDER_GEO_COUNT];

	// outline 전용
	SubRenderGeometry* m_pOutlineRenderGeo;

	UINT m_subRenderGeoCount;
	// PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState_Outline;
private:

public:
	TextureRenderMesh();
	virtual ~TextureRenderMesh();
};
