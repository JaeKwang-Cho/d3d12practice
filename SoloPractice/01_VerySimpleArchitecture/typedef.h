// typedef.h from "megayuchi"

#pragma once
#define MAX_LIGHT_COUNTS (16)
#define D3D_COMPILE_STANDARD_FILE_INCLUDE ((ID3DInclude*)(UINT_PTR)1)

struct ColorVertex {
	XMFLOAT3 position;
	XMFLOAT4 color;
	XMFLOAT2 texCoord;
};

struct TextureVertex 
{
	// 위치, 법선, 탄젠트, UV좌표
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 TexC;

	TextureVertex() = default;
	TextureVertex(
		const DirectX::XMFLOAT3& _p,
		const DirectX::XMFLOAT3& _n,
		const DirectX::XMFLOAT3& _t,
		const DirectX::XMFLOAT2& _uv) :
		Position(_p),
		Normal(_n),
		TangentU(_t),
		TexC(_uv) {}
	TextureVertex(
		float _px, float _py, float _pz,
		float _nx, float _ny, float _nz,
		float _tx, float _ty, float _tz,
		float _u, float _v) :
		Position(_px, _py, _pz),
		Normal(_nx, _ny, _nz),
		TangentU(_tx, _ty, _tz),
		TexC(_u, _v) {}
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

struct Light
{
	XMFLOAT3 strength;
	float falloffStart;  // point/spot light only
	XMFLOAT3 direction;	 // directional/spot light only
	float falloffEnd;	 // point/spot light only
	XMFLOAT3 position;	 // point/spot light only
	float spotPower;	 // spot light only
};

struct CONSTANT_BUFFER_OBJECT
{
	XMMATRIX matWorld;
	XMMATRIX invWorldTranspose;
};

struct CONSTANT_BUFFER_FRAME
{
	XMMATRIX matView;
	XMMATRIX intView;

	XMMATRIX matProj;
	XMMATRIX invProj;

	XMMATRIX matViewProj;
	XMMATRIX invViewProj;

	XMFLOAT3 eyePosW;
	float pad0;
	XMFLOAT2 renderTargetSize;
	XMFLOAT2 renderTargetSize_reciprocal;

	XMFLOAT4 ambientLight;
	Light lights[MAX_LIGHT_COUNTS];
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

enum class E_CONSTANT_BUFFER_TYPE : UINT
{
	DEFAULT = 0,
	SPRITE,
	END
};

struct CONSTANT_BUFFER_PROPERTY
{
	E_CONSTANT_BUFFER_TYPE type;
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

struct ColorMeshData
{
	// 점 데이터, 인덱스 데이터
	std::vector<ColorVertex> Vertices;
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

struct TextureMeshData
{
	// 점 데이터, 인덱스 데이터
	std::vector<TextureVertex> Vertices;
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

enum class E_RENDER_MESH_TYPE : UINT
{
	COLOR = 0,
	TEXTURE,
	END
};
