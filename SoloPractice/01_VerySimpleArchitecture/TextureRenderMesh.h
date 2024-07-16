#pragma once

class D3D12Renderer;

enum class E_TEX_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ
{
	CBV = 0,
	TEX,
	END
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
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld);
	void DrawOutline(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld);

	void CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount);
	void CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount, std::vector<uint32_t>& _adjIndices);
	void BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex);

protected:
	bool InitCommonResources();
	void CleanupSharedResources();

	virtual bool InitRootSignature();
	virtual bool InitPipelineState();
	bool InitPipelineState_Outline();

	void CleanUpAssets();
private:

public:
protected:
	D3D12Renderer* m_pRenderer;
	D3D_PRIMITIVE_TOPOLOGY m_PrimitiveTopoloy;

	// sub - render asset
	SubRenderGeometry* m_subRenderGeometries[MAX_SUB_RENDER_GEO_COUNT];
	UINT m_subRenderGeoCount;
	// PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState_Outline;
private:

public:
	TextureRenderMesh();
	virtual ~TextureRenderMesh();
};
