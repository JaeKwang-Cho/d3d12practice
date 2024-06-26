// SpriteObject.h from "megayuchi"
#pragma once

enum class SPRITE_DESCRIPTOR_INDEX : UINT
{
	CBV = 0,
	TEX = 1
};

class D3D12Renderer;

class SpriteObject
{
public:
	static const UINT DESCRIPTOR_COUNT_FOR_DRAW = 2; // CB와 Texture를 가진다.
private:
	// (BasicMeshObject와 마찬가지로) 그리는 방법과 도형정보는 인스턴싱을 위해 클래스 맴버로 공유한다.
	static Microsoft::WRL::ComPtr<ID3D12RootSignature> m_pRootSignature;
	static Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pPipelineState;
	static DWORD m_dwInitRefCount;

	// Vertex 정보와 Index 정보도 static으로 한다. 아마 Quad로만 그리기 때문일 것이다.
	static Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	static D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	static Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
	static D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

public:
	bool Initialize(D3D12Renderer* _pRenderer);
	bool Initialize(D3D12Renderer* _pRenderer, const WCHAR* _wchTexFileName, const RECT* _pRect);
	void DrawWithTex(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMFLOAT2* _pPos, const XMFLOAT2* _pScale, const RECT* _pRect, float _z, TEXTURE_HANDLE* _pTexHandle);
	void Draw(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> _pCommandList, const XMFLOAT2* _pPos, const XMFLOAT2* _pScale, float _z);
protected:
private:
	bool InitCommonResources();
	void CleanUpSharedResources();

	bool InitRootSignature();
	bool InitPipelineState();
	bool InitMesh();

	void CleanUp();

public:
protected:
private:
	TEXTURE_HANDLE* m_pTexHandle;
	D3D12Renderer* m_pRenderer;
	RECT m_Rect;
	XMFLOAT2 m_Scale;

	DWORD m_dwTriGroupCount;
	DWORD m_dwMaxTriGroupCount;

public:
	SpriteObject();
	~SpriteObject();
};

