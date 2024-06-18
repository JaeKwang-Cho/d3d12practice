// BasicMeshObject.h from "megayuchi"
#pragma once

// Descriptor Handle Offset�� �̷��� enum ������ ���Ѵ�.
enum class BASIC_MESH_DESCRIPTOR_INDEX
{
	TEX = 0
};

class D3D12Renderer;

class BasicMeshObject
{
	// �ϴ� ������ �ܼ��� �׽�Ʈ������ �ϱ� ������
	// ���� Ŭ���� �ɹ��� root signature�� PSO, RefCount�� �����Ѵ�.
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;

	static const UINT DESCRIPTOR_COUNT_FOR_DRAW = 1; // �ϴ��� �ؽ��� �Ѱ���.
public:
	bool Initialize(D3D12Renderer* _pRenderer);
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _pCommandList);
	
	bool CreateMesh_UploadHeap();
	bool CreateMesh_DefaultHeap();
	bool CreateMesh_WithIndex();
	bool CreateMesh_WithTexture();
protected:
private:
	bool InitCommonResources();
	void CleanupSharedResources();

	bool InitRootSignature();
	bool InitPipelineState();

	// Texture Resource�� Shader�� �ѱ�� ���� Table�� �����Ѵ�.
	bool CreateDescriptorTable();

	void CleanUpMesh();
public:


protected:
private:
	D3D12Renderer* m_pRenderer;

	// vertex data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// index data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;
	// texture data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pTexResource;

	// for Descriptor Table
	UINT m_srvDescriptorSize;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pDescriptorHeap;
public:
	BasicMeshObject();
	~BasicMeshObject();

};

