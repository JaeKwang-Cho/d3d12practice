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