#pragma once

class D3D12Renderer;

enum class BASIC_RENDERASSET_DESCRIPTOR_INDEX_PER_OBJ
{
	CBV = 0,
	TEX,
	END
};

class BasicRenderMesh
{
public:
	static const UINT MAX_SUB_RENDER_GEO_COUNT = 8;
protected:
	// 상속받으면서 바꿔주면 되지 않을까?
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;

	static DWORD m_dwInitRefCount;

public:
	bool Initialize(D3D12Renderer* _pRenderer);
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX*  _pMatWorld);

	void CreateRenderAssets(MeshData** _ppMeshData, const UINT _meshDataCount);
	void BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex);

protected:
	bool InitCommonResources();
	void CleanupSharedResources();

	virtual bool InitRootSignature();
	virtual bool InitPipelineState();

	void CleanUpAssets();
private:

public:


protected:
private:
	D3D12Renderer* m_pRenderer;

	// sub - render asset
	SubRenderGeometry* subRenderGeometries[MAX_SUB_RENDER_GEO_COUNT];
	UINT m_subRenderGeoCount;
	// PSO
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;

public:
	BasicRenderMesh();
	virtual ~BasicRenderMesh();
};

