// BasicMeshObject.h from "megayuchi"
#pragma once

// Descriptor Handle Offset�� �̷��� enum ������ ���Ѵ�.
/*
enum class BASIC_MESH_DESCRIPTOR_INDEX
{
	CBV = 0, // Contant Buffer View
	TEX = 1, 
	END
};
*/
enum class BASIC_MESH_DESCRIPTOR_INDEX_PER_OBJ
{
	CBV = 0,
	END
};

enum class BASIC_MESH_DESCRIPTOR_INDEX_PER_TRI_GROUP
{
	TEX = 0,
	END
};

// �ε��� �� �ﰢ�� �׷��̴�.
// �ٷ� Pipeline�� bind �� �� �ֵ��� �ɹ��� �����ϰ� �ִ�.
struct INDEXED_TRI_GROUP
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pIndexBuffer = nullptr;
	D3D12_INDEX_BUFFER_VIEW indexBufferView = {};
	DWORD dwTriCount;
	TEXTURE_HANDLE* pTexHandle;
};

class D3D12Renderer;

class BasicMeshObject
{
private:
	// �ϴ� ������ �ܼ��� �׽�Ʈ������ �ϱ� ������
	// ���� Ŭ���� �ɹ��� root signature�� PSO, RefCount�� �����Ѵ�.
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;

public:
	//static const UINT DESCRIPTOR_COUNT_FOR_DRAW = 2; // CB�� Tex�� ������ �ִ�.
	static const UINT DESCRIPTOR_COUNT_PER_OBJ = 1; // Object���� ���� Descriptor ���� -> �ϴ��� CB �ϳ��̴�.
	static const UINT DESCRIPTOR_COUNT_PER_TRI_GROUP = 1; // Triangle Group ���� ���� Descriptor ���� -> �ϴ��� SRV(Texture) �ϳ���.
	static const UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8; // Object ���� ���� �� �ִ� �ִ� Triangle Group ����
	static const UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * DESCRIPTOR_COUNT_PER_TRI_GROUP);

public:
	bool Initialize(D3D12Renderer* _pRenderer);
	// �ܺο��� Texture�� ���� �ʴ´�. MeshObject�� �����Ҷ� �̹� �ɹ��� ������ �ִ� �͵��� bind�Ѵ�.
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMMATRIX* _pMatWorld);

	bool BeginCreateMesh(const BasicVertex* _pVertexList, DWORD _dwVertexNum, DWORD _dwTriGroupCount);
	bool InsertIndexedTriList(const uint16_t* _pIndexList, DWORD _dwTriCount, const WCHAR* _wchTexFileName);
	void EndCreateMesh();

	//bool CreateMesh();
	//bool CreateMesh_UploadHeap();
	//bool CreateMesh_DefaultHeap();
	//bool CreateMesh_WithIndex();
	//bool CreateMesh_WithTexture();
	//bool CreateMesh_WithCB();
protected:
private:
	bool InitCommonResources();
	void CleanupSharedResources();

	bool InitRootSignature();
	bool InitPipelineState();

	// Texture Resource�� Shader�� �ѱ�� ���� Table�� �����Ѵ�.
	//bool CreateDescriptorTable();

	void CleanUpMesh();
public:


protected:
private:
	D3D12Renderer* m_pRenderer;

	// vertex data
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	// index data
	INDEXED_TRI_GROUP* m_pTriGroupList;
	DWORD m_dwTriGroupCount;
	DWORD m_dwMaxTriGroupCount;



public:
	BasicMeshObject();
	~BasicMeshObject();

};

