// BasicMeshObject.h from "megayuchi"

#pragma once
class D3D12Renderer;

class BasicMeshObject
{
	// �ϴ� ������ �ܼ��� �׽�Ʈ������ �ϱ� ������
	// ���� Ŭ���� �ɹ��� root signature�� PSO, RefCount�� �����Ѵ�.
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

