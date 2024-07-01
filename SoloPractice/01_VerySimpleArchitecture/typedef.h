// typedef.h from "megayuchi"

#pragma once

struct BasicVertex {
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 texCoord;
};

union RGBA
{
	struct {
		BYTE r;
		BYTE g;
		BYTE b;
		BYTE a;
	};
	BYTE bColorFactor[4];
};

struct CONSTANT_BUFFER_DEFAULT
{
	XMMATRIX matWorld;
	XMMATRIX matView;
	XMMATRIX matProj;
	XMMATRIX matWVP;
};

struct CONSTANT_BUFFER_SPRITE 
{
	XMFLOAT2 screenResol;
	XMFLOAT2 pos;
	XMFLOAT2 scale;
	XMFLOAT2 texSize;
	XMFLOAT2 texSamplePos;
	XMFLOAT2 TexSampleSize;
	float z;
	float alpha;
	float pad0;
	float pad1;
};

enum class CONSTANT_BUFFER_TYPE : UINT
{
	DEFAULT = 0,
	SPRITE,
	END
};

struct CONSTANT_BUFFER_PROPERTY
{
	CONSTANT_BUFFER_TYPE type;
	UINT size;
};

struct TEXTURE_HANDLE
{
	Microsoft::WRL::ComPtr<ID3D12Resource> pTexResource;
	D3D12_CPU_DESCRIPTOR_HANDLE srv;
	TEXTURE_HANDLE() :pTexResource(nullptr), srv{} {}
};

struct SubRenderGeometry 
{
	UINT indexCount;
	UINT startIndexLocation;
	UINT baseVertexLocation;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_pIndexBuffer;
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView;

	TEXTURE_HANDLE* pTexHandle;

	SubRenderGeometry() 
		: indexCount(0), startIndexLocation(0), baseVertexLocation(0),
		m_pVertexBuffer(nullptr), m_VertexBufferView{},
		m_pIndexBuffer(nullptr), m_IndexBufferView{},
		pTexHandle(nullptr)
	{}
};

struct MeshData
{
	// 점 데이터, 인덱스 데이터
	std::vector<BasicVertex> Vertices;
	std::vector<uint32_t> Indices32;

	// 총 인덱스 개수가 적을 때 사용
	std::vector<uint16_t>& GetIndices16()
	{
		if (mIndices16.empty())
		{
			mIndices16.resize(Indices32.size());
			for (size_t i = 0; i < Indices32.size(); ++i)
				mIndices16[i] = static_cast<uint16_t>(Indices32[i]);
		}

		return mIndices16;
	}

private:
	std::vector<uint16_t> mIndices16;
};
