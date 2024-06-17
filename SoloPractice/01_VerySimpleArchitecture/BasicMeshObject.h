// BasicMeshObject.h from "megayuchi"

#pragma once
class D3D12Renderer;

class BasicMeshObject
{
	// 일단 지금은 단순히 테스트용으로 하기 때문에
	// 요롷게 클래스 맴버로 root signature와 PSO, RefCount를 관리한다.
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;
public:
	bool Initialize(D3D12Renderer* _pRenderer);
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _pCommandList);
	bool CreateMesh();
protected:
private:
	bool InitCommonResources();
	void CleanupSharedResources();

	bool InitRootSignature();
	bool InitPipelineState();

	void CleanUpMesh();
public:


protected:
private:
	D3D12Renderer* m_pRenderer;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
public:
	BasicMeshObject();
	~BasicMeshObject();

};

