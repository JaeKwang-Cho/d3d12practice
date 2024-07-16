// VertexUtil.cpp from "megayuchi"

#include "pch.h"
#include "VertexUtil.h"
#include <unordered_map>
#include <cstdlib>
#include <boost/functional/hash.hpp>

bool Float3ForKey::operator==(const Float3ForKey& _other) const
{
	bool bResult = std::fabs(x - _other.x) < EPSILON &&
		std::fabs(y - _other.y) < EPSILON &&
		std::fabs(z - _other.z) < EPSILON;
	return bResult;
}

bool Float3ForKey::operator!=(const Float3ForKey& _other) const
{
	bool bResult = std::fabs(x - _other.x) > EPSILON ||
		std::fabs(y - _other.y) > EPSILON ||
		std::fabs(z - _other.z) > EPSILON;
	return bResult;
}

size_t Float3Hash::operator()(const Float3ForKey& _f3key) const
{
	std::size_t seed = 0;
	int xVal = std::round(_f3key.x * 1e5);
	int yVal = std::round(_f3key.y * 1e5);
	int zVal = std::round(_f3key.z * 1e5);

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

uint32_t FindOtherOneIndex(
	Float3ForKey& _vertPos0,
	Float3ForKey& _vertPos1,
	Float3ForKey& _vertPos2_Excluded,
	std::unordered_map<Float3ForKey, TriFaceGroupPerVertPos, Float3Hash>& _findFaceGroupByPos)
{	
	// first
	auto iter0 = _findFaceGroupByPos.find(_vertPos0);
	UINT numTriFaceGroups0 = iter0->second.numTriFaceGroups;
	// second
	auto iter1 = _findFaceGroupByPos.find(_vertPos1);
	UINT numTriFaceGroups1 = iter1->second.numTriFaceGroups;

	int includedVert = -1;
	int otherVert = -1;
	bool bItself = false;
	for (UINT i = 0; i < numTriFaceGroups0; i++) {
		TriFaceGroup* pFaceGroup0 = iter0->second.triFaceGroups[i];
		for (int v = 0; v < 3; v++) {
			if (pFaceGroup0->vertPos[v] == _vertPos1) {
				includedVert = v;
			}
			else if (pFaceGroup0->vertPos[v] == _vertPos2_Excluded) 
			{
				bItself = true;
			}
			else if (pFaceGroup0->vertPos[v] != _vertPos0)
			{
				otherVert = v;
			}
		}
		if (includedVert >= 0 && !bItself)
		{
			return pFaceGroup0->indice[otherVert];
		}
		includedVert = -1;
		otherVert = -1;
		bItself = false;
	}
	
	for (UINT j = 0; j < numTriFaceGroups1; j++) {
		TriFaceGroup* pFaceGroup1 = iter1->second.triFaceGroups[j];
		for (int v = 0; v < 3; v++) {
			if (pFaceGroup1->vertPos[v] == _vertPos0) {
				includedVert = v;
			}
			else if (pFaceGroup1->vertPos[v] == _vertPos2_Excluded)
			{
				bItself = true;
			}
			else if (pFaceGroup1->vertPos[v] != _vertPos1)
			{
				otherVert = v;
			}
		}
		if (includedVert >= 0 && !bItself)
		{
			return pFaceGroup1->indice[otherVert];
		}
		includedVert = -1;
		otherVert = -1;
		bItself = false;
	}
	__debugbreak();
}

int GenerateAdjacencyIndices(const std::vector<XMFLOAT3>& _vertices, const std::vector<uint32_t>& _indices, std::vector<uint32_t>& _adjIndicies)
{
	size_t numVerts = _vertices.size();
	size_t numFaces = _indices.size() / 3;

	_adjIndicies.resize(numFaces * 6);
	TriFaceGroup* triFaceGroups = new TriFaceGroup[numFaces];
	std::unordered_map<Float3ForKey, TriFaceGroupPerVertPos, Float3Hash> findFaceGroupByPos;

	// vertex pos로 face group을 찾을 수 있게 만듦.
	for (size_t f = 0; f < numFaces;f++)
	{
		triFaceGroups[f].indice[0] = _indices[(f * 3)];
		triFaceGroups[f].indice[1] = _indices[(f * 3) + 1];
		triFaceGroups[f].indice[2] = _indices[(f * 3) + 2];

		triFaceGroups[f].vertPos[0] = _vertices[_indices[(f * 3)]];
		triFaceGroups[f].vertPos[1] = _vertices[_indices[(f * 3) + 1]];
		triFaceGroups[f].vertPos[2] = _vertices[_indices[(f * 3) + 2]];

		// vert pos로 index 찾는 map
		for (size_t i = f * 3; i < (f + 1) * 3; i++) {
			Float3ForKey vertPos = _vertices[_indices[i]];
			auto iter = findFaceGroupByPos.find(vertPos);

			Float3Hash float3Hash;
			size_t tmpHash0 = float3Hash(vertPos);

			if (iter == findFaceGroupByPos.end()) {
				TriFaceGroupPerVertPos triFaceGroupPerVertPos;
				triFaceGroupPerVertPos.numTriFaceGroups = 1;
				triFaceGroupPerVertPos.triFaceGroups[0] = triFaceGroups + f;
				findFaceGroupByPos.insert({ vertPos, triFaceGroupPerVertPos });
			}
			else {
				UINT numTriFaceGroup = iter->second.numTriFaceGroups;
				if (numTriFaceGroup >= MAX_SHARED_TRIANGLES - 1) {
					__debugbreak();
					exit(1);
				}
				iter->second.triFaceGroups[numTriFaceGroup] = triFaceGroups + f;
				iter->second.numTriFaceGroups = numTriFaceGroup + 1;
			}
		}
	}

	for (size_t f = 0; f < numFaces; f++)
	{
		size_t i = f * 3;
		Float3ForKey vertPos0 = _vertices[_indices[i]];
		Float3ForKey vertPos1 = _vertices[_indices[i + 1]];
		Float3ForKey vertPos2 = _vertices[_indices[i + 2]];

		size_t adjI = f * 6;
		//_adjIndicies[adjI] = _indices[i];
		//_adjIndicies[adjI + 1] = _indices[i + 1];
		//_adjIndicies[adjI + 2] = _indices[i + 2];
		//_adjIndicies[adjI + 3] = FindOtherOneIndex(vertPos0, vertPos1, vertPos2, findFaceGroupByPos);
		//_adjIndicies[adjI + 4] = FindOtherOneIndex(vertPos1, vertPos2, vertPos0, findFaceGroupByPos);
		//_adjIndicies[adjI + 5] = FindOtherOneIndex(vertPos2, vertPos0, vertPos1, findFaceGroupByPos);
		_adjIndicies[adjI] = _indices[i];
		_adjIndicies[adjI + 1] = FindOtherOneIndex(vertPos0, vertPos1, vertPos2, findFaceGroupByPos);
		_adjIndicies[adjI + 2] = _indices[i + 1];
		_adjIndicies[adjI + 3] = FindOtherOneIndex(vertPos1, vertPos2, vertPos0, findFaceGroupByPos);
		_adjIndicies[adjI + 4] = _indices[i + 2];
		_adjIndicies[adjI + 5] = FindOtherOneIndex(vertPos2, vertPos0, vertPos1, findFaceGroupByPos);
	}

	delete[] triFaceGroups;
	triFaceGroups = nullptr;
	return _indices.size();
}

