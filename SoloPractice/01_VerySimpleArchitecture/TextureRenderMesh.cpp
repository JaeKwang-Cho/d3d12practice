#include "pch.h"
#include "TextureRenderMesh.h"

bool TextureRenderMesh::Initialize(D3D12Renderer* _pRenderer, D3D_PRIMITIVE_TOPOLOGY _primitiveTopoloy)
{
	m_pRenderer = _pRenderer;
	m_PrimitiveTopoloy = _primitiveTopoloy;

	bool bResult = InitCommonResources();

	// 일단은 인스턴스마다 PSO를 갖도록 한다.
	bResult = InitPipelineState();
	return bResult;
}

void TextureRenderMesh::Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld)
{
}

void TextureRenderMesh::CreateRenderAssets(std::vector<TextureMeshData>& _ppMeshData, const UINT _meshDataCount)
{
}

void TextureRenderMesh::BindTextureAssets(TEXTURE_HANDLE* _pTexHandle, const UINT _subRenderAssetIndex)
{
}

bool TextureRenderMesh::InitCommonResources()
{
	return false;
}

void TextureRenderMesh::CleanupSharedResources()
{
}

bool TextureRenderMesh::InitRootSignature()
{
	return false;
}

bool TextureRenderMesh::InitPipelineState()
{
	return false;
}

void TextureRenderMesh::CleanUpAssets()
{
}

TextureRenderMesh::TextureRenderMesh()
	:m_pRenderer(nullptr), m_PrimitiveTopoloy(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST),
	subRenderGeometries{ nullptr, },
	m_subRenderGeoCount(0), m_pPipelineState(nullptr)
{
}

TextureRenderMesh::~TextureRenderMesh()
{
}
