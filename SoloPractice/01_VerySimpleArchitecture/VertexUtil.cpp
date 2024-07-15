// VertexUtil.cpp from "megayuchi"

#include "pch.h"
#include "VertexUtil.h"
#include <unordered_map>
#include <cstdlib>
#include <boost/functional/hash.hpp>

bool Float3ForKey::operator==(const Float3ForKey& _other) const
{
	return std::fabs(x - _other.x) < EPSILON &&
		std::fabs(y - _other.y) < EPSILON &&
		std::fabs(z - _other.z) < EPSILON;
}

size_t Float3Hash::operator()(const Float3ForKey& _f3key) const
{
	std::size_t seed = 0;
	int xVal = std::round(_f3key.x * 1e6);
	int yVal = std::round(_f3key.y * 1e6);
	int zVal = std::round(_f3key.z * 1e6);

	boost::hash_combine(seed, xVal);
	boost::hash_combine(seed, yVal);
	boost::hash_combine(seed, zVal);

	return seed;
}

uint16_t AddVertex(ColorVertex* _pVertexList, DWORD _dwMaxVertexCount, DWORD* _pdwInOutVertexCount, const ColorVertex* _pVertex) {
	uint16_t dwFoundIndex = -1;
	DWORD dwExistVertexCount = *_pdwInOutVertexCount;

	// 이미 존재하는 점이면 안 넣는다.
	for (DWORD i = 0; i < dwExistVertexCount; i++) {
		const ColorVertex* pExistVertex = _pVertexList + i;
		if (!memcmp(pExistVertex, _pVertex, sizeof(ColorVertex))) {
			dwFoundIndex = (uint16_t)i;
			goto RETURN;
		}
	}
	// 넘치면 안 넣는다.
	if (dwExistVertexCount + 1 > _dwMaxVertexCount) {
		__debugbreak();
		goto RETURN;
	}
	// 새로운 점 추가
	dwFoundIndex = (uint16_t)dwExistVertexCount;
	_pVertexList[dwFoundIndex] = *_pVertex;
	*_pdwInOutVertexCount = dwExistVertexCount + 1;

RETURN:
	return dwFoundIndex;
}

DWORD CreateBoxMesh(ColorVertex** _ppOutVertexList, uint16_t* _pOutIndexList, DWORD _dwMaxBufferCount, float _fHalfBoxLength)
{
	const DWORD INDEX_COUNT = 36;
	if (_dwMaxBufferCount < INDEX_COUNT) {
		__debugbreak();
	}

	const uint16_t pIndexList[INDEX_COUNT] =
	{
		// +z
		3, 0, 1,
		3, 1, 2,

		// -z
		4, 7, 6,
		4, 6, 5,

		// -x
		0, 4, 5,
		0, 5, 1,

		// +x
		7, 3, 2,
		7, 2, 6,

		// +y
		0, 3, 7,
		0, 7, 4,

		// -y
		2, 1, 5,
		2, 5, 6
	};

	XMFLOAT2 pTexCoordList[INDEX_COUNT] =
	{
		// +z
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -z
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -x
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// +x
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// +y
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f},

		// -y
		{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f},
		{0.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
	};

	XMFLOAT3 pWorldPosList[8];
	pWorldPosList[0] = { -_fHalfBoxLength, _fHalfBoxLength, _fHalfBoxLength };
	pWorldPosList[1] = { -_fHalfBoxLength, -_fHalfBoxLength, _fHalfBoxLength };
	pWorldPosList[2] = { _fHalfBoxLength, -_fHalfBoxLength, _fHalfBoxLength };
	pWorldPosList[3] = { _fHalfBoxLength, _fHalfBoxLength, _fHalfBoxLength };
	pWorldPosList[4] = { -_fHalfBoxLength, _fHalfBoxLength, -_fHalfBoxLength };
	pWorldPosList[5] = { -_fHalfBoxLength, -_fHalfBoxLength, -_fHalfBoxLength };
	pWorldPosList[6] = { _fHalfBoxLength, -_fHalfBoxLength, -_fHalfBoxLength };
	pWorldPosList[7] = { _fHalfBoxLength, _fHalfBoxLength, -_fHalfBoxLength };

	const DWORD MAX_WORKING_VERTEX_COUNT = 65536;
	ColorVertex* pWorkingVertexList = new ColorVertex[MAX_WORKING_VERTEX_COUNT];
	memset(pWorkingVertexList, 0, sizeof(ColorVertex) * MAX_WORKING_VERTEX_COUNT);
	DWORD dwBasicVertexCount = 0;

	for (DWORD i = 0; i < INDEX_COUNT; i++) {
		ColorVertex v;
		v.color = { 1.f, 1.f, 1.f, 1.f };
		v.position = pWorldPosList[pIndexList[i]];
		v.texCoord = pTexCoordList[i];

		_pOutIndexList[i] = AddVertex(pWorkingVertexList, MAX_WORKING_VERTEX_COUNT, &dwBasicVertexCount, &v);
	}

	ColorVertex* pNewVertexList = new ColorVertex[dwBasicVertexCount];
	memcpy(pNewVertexList, pWorkingVertexList, sizeof(ColorVertex) * dwBasicVertexCount);

	*_ppOutVertexList = pNewVertexList;

	delete[] pWorkingVertexList;
	pWorkingVertexList = nullptr;

	return dwBasicVertexCount;
}

void DeleteBoxMesh(ColorVertex* _pVertexList)
{
	delete[] _pVertexList;
}

int GenerateAdjacencyIndices(const std::vector<XMFLOAT3>& _vertices, const std::vector<uint32_t>& _indices, std::vector<uint32_t>& _adjIndicies)
{
	size_t numVerts = _vertices.size();
	size_t numFaces = _indices.size() / 3;

	_adjIndicies.resize(numFaces * 6);
	TriFaceGroup* triFaceGroups = new TriFaceGroup[numFaces];
	std::unordered_map<Float3ForKey, TriFaceGroupPerVertPos, Float3Hash> findIndicesByPos;

	// vertex pos로 face group을 찾을 수 있게 만듦.
	for (size_t f = 0; f < numFaces; f++)
	{
		triFaceGroups[f].indice[0] = _indices[f];
		triFaceGroups[f].indice[1] = _indices[f + 1];
		triFaceGroups[f].indice[2] = _indices[f + 2];

		triFaceGroups[f].vertPos[0] = _vertices[_indices[f]];
		triFaceGroups[f].vertPos[1] = _vertices[_indices[f + 1]];
		triFaceGroups[f].vertPos[2] = _vertices[_indices[f + 2]];

		// vert pos로 index 찾는 map
		for (size_t i = f * 3; i < (f + 1) * 3; i++) {
			Float3ForKey vertPos = _vertices[_indices[i]];
			auto iter = findIndicesByPos.find(vertPos);

			if (iter == findIndicesByPos.end()) {
				TriFaceGroupPerVertPos triFaceGroupPerVertPos;
				triFaceGroupPerVertPos.numTriFaceGroups = 0;
				triFaceGroupPerVertPos.triFaceGroups[0] = triFaceGroups + f;
				findIndicesByPos.insert({ vertPos, triFaceGroupPerVertPos });
			}
			else {
				UINT numTriFaceGroup = iter->second.numTriFaceGroups++;
				if (numTriFaceGroup >= MAX_SHARED_TRIANGLES) {
					__debugbreak();
					exit(1);
				}
				iter->second.triFaceGroups[numTriFaceGroup] = triFaceGroups + f;
				iter->second.numTriFaceGroups = numTriFaceGroup + 1;
			}
		}
	}
	delete[] triFaceGroups;
	triFaceGroups = nullptr;
	return _indices.size();
}

