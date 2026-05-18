#pragma once

#include <Windows.h>
#include <DirectXMath.h>
#include <cstdint>
#include <vector>

struct TEXTURE_HANDLE;
struct FONT_HANDLE;
struct SPRITE_HANDLE;

struct TextureVertex
{
	DirectX::XMFLOAT3 Position;
	DirectX::XMFLOAT3 Normal;
	DirectX::XMFLOAT3 TangentU;
	DirectX::XMFLOAT2 TexC;

	TextureVertex() = default;

	TextureVertex(
		const DirectX::XMFLOAT3& _p,
		const DirectX::XMFLOAT3& _n,
		const DirectX::XMFLOAT3& _t,
		const DirectX::XMFLOAT2& _uv)
		: Position(_p),
		Normal(_n),
		TangentU(_t),
		TexC(_uv)
	{
	}

	TextureVertex(
		float _px, float _py, float _pz,
		float _nx, float _ny, float _nz,
		float _tx, float _ty, float _tz,
		float _u, float _v)
		: Position(_px, _py, _pz),
		Normal(_nx, _ny, _nz),
		TangentU(_tx, _ty, _tz),
		TexC(_u, _v)
	{
	}
};

struct TextureMeshData
{
	std::vector<TextureVertex> Vertices;
	std::vector<std::uint32_t> Indices32;

	std::vector<std::uint16_t>& GetIndices16()
	{
		if (m_Indices16.empty())
		{
			m_Indices16.resize(Indices32.size());
			for (size_t i = 0; i < Indices32.size(); ++i)
			{
				m_Indices16[i] = static_cast<std::uint16_t>(Indices32[i]);
			}
		}

		return m_Indices16;
	}

private:
	std::vector<std::uint16_t> m_Indices16;
};

struct SubmeshRange
{
	UINT startPosIndex = 0;
	UINT posIndexCount = 0;
	UINT startIndexIndex = 0;
	UINT indexIndexCount = 0;
};

struct CONSTANT_BUFFER_MATERIAL
{
	DirectX::XMFLOAT4 diffuseAlbedo;
	DirectX::XMFLOAT3 fresnelR0;
	float roughness;
	DirectX::XMMATRIX matTransform;

	CONSTANT_BUFFER_MATERIAL()
		: diffuseAlbedo(1.f, 1.f, 1.f, 1.f),
		fresnelR0(0.05f, 0.05f, 0.05f),
		roughness(0.5f),
		matTransform(DirectX::XMMatrixIdentity())
	{
	}

	explicit CONSTANT_BUFFER_MATERIAL(DirectX::XMFLOAT4 _diffuseAlbedo)
		: diffuseAlbedo(_diffuseAlbedo),
		fresnelR0(0.05f, 0.05f, 0.05f),
		roughness(0.5f),
		matTransform(DirectX::XMMatrixIdentity())
	{
	}

	CONSTANT_BUFFER_MATERIAL(
		DirectX::XMFLOAT4 _diffuseAlbedo,
		DirectX::XMFLOAT3 _fresnelR0,
		float _roughness,
		DirectX::XMMATRIX _matTransform = DirectX::XMMatrixIdentity())
		: diffuseAlbedo(_diffuseAlbedo),
		fresnelR0(_fresnelR0),
		roughness(_roughness),
		matTransform(_matTransform)
	{
	}
};