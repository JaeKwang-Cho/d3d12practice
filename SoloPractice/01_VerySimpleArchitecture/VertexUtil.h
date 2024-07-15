// VertexUtil.h from "megayuchi"

#pragma once

const int MAX_SHARED_TRIANGLES = 16;
const int EPSILON = 1e-6;

struct Float3ForKey
{
	float x;
	float y;
	float z;

	bool operator==(const Float3ForKey& _other) const;

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
	{}
};

struct Float3Hash
{
	size_t operator()(const Float3ForKey& _f3key) const;
};

struct TriFaceGroup
{
	XMFLOAT3 vertPos[3];
	uint32_t indice[3];
};

struct TriFaceGroupPerVertPos
{
	UINT numTriFaceGroups = 0;
	TriFaceGroup* triFaceGroups[MAX_SHARED_TRIANGLES];
};

// �� �鸶�� Texturing�� ������ �� �ִ� ť�긦 ������ش�
DWORD CreateBoxMesh(ColorVertex** _ppOutVertexList, uint16_t* _pOutIndexList, DWORD _dwMaxBufferCount, float _fHalfBoxLength);
void DeleteBoxMesh(ColorVertex* _pVertexList);

// triangle list�� mesh�� triangle list with adjacency�� �ٲ۴�.
int GenerateAdjacencyIndices(const std::vector<DirectX::XMFLOAT3>& _vertices, const std::vector<uint32_t>& _indices, std::vector<uint32_t>& _adjIndicies);

