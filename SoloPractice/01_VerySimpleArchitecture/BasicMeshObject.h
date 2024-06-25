// BasicMeshObject.h from "megayuchi"
#pragma once

// Descriptor Handle Offset을 이렇게 enum 값으로 정한다.
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

// 인덱싱 된 삼각형 그룹이다.
// 바로 Pipeline에 bind 할 수 있도록 맴버를 구성하고 있다.
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
	// 일단 지금은 단순히 테스트용으로 하기 때문에
	// 요롷게 클래스 맴버로 root signature와 PSO, RefCount를 관리한다.
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;

public:
	//static const UINT DESCRIPTOR_COUNT_FOR_DRAW = 2; // CB와 Tex를 가지고 있다.
	static const UINT DESCRIPTOR_COUNT_PER_OBJ = 1; // Object마다 들어가는 Descriptor 개수 -> 일단은 CB 하나이다.
	static const UINT DESCRIPTOR_COUNT_PER_TRI_GROUP = 1; // Triangle Group 마다 들어가는 Descriptor 개수 -> 일단은 SRV(Texture) 하나다.
	static const UINT MAX_TRI_GROUP_COUNT_PER_OBJ = 8; // Object 마다 가질 수 있는 최대 Triangle Group 개수
	static const UINT MAX_DESCRIPTOR_COUNT_FOR_DRAW = DESCRIPTOR_COUNT_PER_OBJ + (MAX_TRI_GROUP_COUNT_PER_OBJ * DESCRIPTOR_COUNT_PER_TRI_GROUP);

public:
	bool Initialize(D3D12Renderer* _pRenderer);
	// 외부에서 Texture를 받지 않는다. MeshObject를 생성할때 이미 맴버로 가지고 있는 것들을 bind한다.
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

	// Texture Resource를 Shader로 넘기기 위한 Table을 생성한다.
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

