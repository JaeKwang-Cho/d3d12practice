#pragma once
#include "IRenderer_typedef.h"

const int MAX_SHARED_TRIANGLES = 16;
const float EPSILON = 1e-5f;

struct Float3ForKey
{
	float x;
	float y;
	float z;

	bool operator==(const Float3ForKey& _other) const;
	bool operator!=(const Float3ForKey& _other) const;

	Float3ForKey& operator=(const Float3ForKey& _other) = default;
	Float3ForKey& operator=(const XMFLOAT3& _other)
	{
		x = _other.x;
		y = _other.y;
		z = _other.z;

		return *this;
	}
	Float3ForKey() = default;
	Float3ForKey(const XMFLOAT3& _other)
		:x(_other.x), y(_other.y), z(_other.z)
	{
	}
};

struct Float3Hash
{
	size_t operator()(const Float3ForKey& _f3key) const;
};

struct TriFaceGroup
{
	Float3ForKey vertPos[3];
	uint32_t indice[3];
};

struct TriFaceGroupPerVertPos
{
	UINT numTriFaceGroups = 0;
	TriFaceGroup* triFaceGroups[MAX_SHARED_TRIANGLES];
	TriFaceGroupPerVertPos() {
		for (UINT i = 0; i < MAX_SHARED_TRIANGLES; i++) {
			triFaceGroups[i] = nullptr;
		}
	};
};

// Outline 을 그릴 수 있는 Cube를 그린다.
void CreateCube(float _width, float _height, float _depth, struct TextureMeshData& _outTextureMeshData, std::vector<uint32_t>& _outAdjIndices, std::vector<struct SubmeshRange>& _outSubmeshRange);

// triangle list인 mesh를 triangle list with adjacency로 바꾼다.
size_t GenerateAdjacencyIndices(const std::vector<DirectX::XMFLOAT3>& _vertices, const std::vector<uint32_t>& _indices, std::vector<uint32_t>& _adjIndicies);