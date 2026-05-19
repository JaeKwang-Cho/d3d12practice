#include "pch.h"
#include "../Common/MeshGenerator.h"
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
	double xVal = std::round(_f3key.x * 1e5);
	double yVal = std::round(_f3key.y * 1e5);
	double zVal = std::round(_f3key.z * 1e5);

	boost::hash_combine(seed, xVal);
	boost::hash_combine(seed, yVal);
	boost::hash_combine(seed, zVal);

	return seed;
}

void CreateCube(float _width, float _height, float _depth, TextureMeshData& _outTextureMeshData, std::vector<uint32_t>& _outAdjIndices, std::vector<SubmeshRange>& _outSubmeshRange)
{
	std::vector<XMFLOAT3> posL;
	posL.resize(24);

	float w2 = 0.5f * _width;
	float h2 = 0.5f * _height;
	float d2 = 0.5f * _depth;

	_outTextureMeshData.Vertices.resize(24);

	// ¾Õ¸é
	_outTextureMeshData.Vertices[0] = TextureVertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[1] = TextureVertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[2] = TextureVertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	_outTextureMeshData.Vertices[3] = TextureVertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	posL[0] = XMFLOAT3(-w2, -h2, -d2);
	posL[1] = XMFLOAT3(-w2, +h2, -d2);
	posL[2] = XMFLOAT3(+w2, +h2, -d2);
	posL[3] = XMFLOAT3(+w2, -h2, -d2);

	// µÞ¸é
	_outTextureMeshData.Vertices[4] = TextureVertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	_outTextureMeshData.Vertices[5] = TextureVertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[6] = TextureVertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[7] = TextureVertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	posL[4] = XMFLOAT3(-w2, -h2, +d2);
	posL[5] = XMFLOAT3(+w2, -h2, +d2);
	posL[6] = XMFLOAT3(+w2, +h2, +d2);
	posL[7] = XMFLOAT3(-w2, +h2, +d2);

	// À­¸é
	_outTextureMeshData.Vertices[8] = TextureVertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[9] = TextureVertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[10] = TextureVertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	_outTextureMeshData.Vertices[11] = TextureVertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	posL[8] = XMFLOAT3(-w2, +h2, -d2);
	posL[9] = XMFLOAT3(-w2, +h2, +d2);
	posL[10] = XMFLOAT3(+w2, +h2, +d2);
	posL[11] = XMFLOAT3(+w2, +h2, -d2);

	// ¹Ø¸é
	_outTextureMeshData.Vertices[12] = TextureVertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	_outTextureMeshData.Vertices[13] = TextureVertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[14] = TextureVertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[15] = TextureVertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	posL[12] = XMFLOAT3(-w2, -h2, -d2);
	posL[13] = XMFLOAT3(+w2, -h2, -d2);
	posL[14] = XMFLOAT3(+w2, -h2, +d2);
	posL[15] = XMFLOAT3(-w2, -h2, +d2);

	// ¿Þ¸é
	_outTextureMeshData.Vertices[16] = TextureVertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[17] = TextureVertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[18] = TextureVertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	_outTextureMeshData.Vertices[19] = TextureVertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	posL[16] = XMFLOAT3(-w2, -h2, +d2);
	posL[17] = XMFLOAT3(-w2, +h2, +d2);
	posL[18] = XMFLOAT3(-w2, +h2, -d2);
	posL[19] = XMFLOAT3(-w2, -h2, -d2);

	// ¿À¸¥¸é
	_outTextureMeshData.Vertices[20] = TextureVertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	_outTextureMeshData.Vertices[21] = TextureVertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	_outTextureMeshData.Vertices[22] = TextureVertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	_outTextureMeshData.Vertices[23] = TextureVertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	posL[20] = XMFLOAT3(+w2, -h2, -d2);
	posL[21] = XMFLOAT3(+w2, +h2, -d2);
	posL[22] = XMFLOAT3(+w2, +h2, +d2);
	posL[23] = XMFLOAT3(+w2, -h2, +d2);

	_outTextureMeshData.Indices32.resize(36);

	_outTextureMeshData.Indices32[0] = 0; _outTextureMeshData.Indices32[1] = 1; _outTextureMeshData.Indices32[2] = 2;
	_outTextureMeshData.Indices32[3] = 0; _outTextureMeshData.Indices32[4] = 2; _outTextureMeshData.Indices32[5] = 3;

	_outTextureMeshData.Indices32[6] = 4; _outTextureMeshData.Indices32[7] = 5; _outTextureMeshData.Indices32[8] = 6;
	_outTextureMeshData.Indices32[9] = 4; _outTextureMeshData.Indices32[10] = 6; _outTextureMeshData.Indices32[11] = 7;

	_outTextureMeshData.Indices32[12] = 8; _outTextureMeshData.Indices32[13] = 9; _outTextureMeshData.Indices32[14] = 10;
	_outTextureMeshData.Indices32[15] = 8; _outTextureMeshData.Indices32[16] = 10; _outTextureMeshData.Indices32[17] = 11;

	_outTextureMeshData.Indices32[18] = 12; _outTextureMeshData.Indices32[19] = 13; _outTextureMeshData.Indices32[20] = 14;
	_outTextureMeshData.Indices32[21] = 12; _outTextureMeshData.Indices32[22] = 14; _outTextureMeshData.Indices32[23] = 15;

	_outTextureMeshData.Indices32[24] = 16; _outTextureMeshData.Indices32[25] = 17; _outTextureMeshData.Indices32[26] = 18;
	_outTextureMeshData.Indices32[27] = 16; _outTextureMeshData.Indices32[28] = 18; _outTextureMeshData.Indices32[29] = 19;

	_outTextureMeshData.Indices32[30] = 20; _outTextureMeshData.Indices32[31] = 21; _outTextureMeshData.Indices32[32] = 22;
	_outTextureMeshData.Indices32[33] = 20; _outTextureMeshData.Indices32[34] = 22; _outTextureMeshData.Indices32[35] = 23;

	_outAdjIndices.clear();
	GenerateAdjacencyIndices(posL, _outTextureMeshData.Indices32, _outAdjIndices);

	_outSubmeshRange.clear();
	_outSubmeshRange.reserve(6);
	for (UINT i = 0; i < 6; i++)
	{
		SubmeshRange range = {};
		range.startPosIndex = i * 4;
		range.posIndexCount = 4;
		range.startIndexIndex = i * 6;
		range.indexIndexCount = 6;
		_outSubmeshRange.push_back(range);
	}
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
	return -1;
}

size_t GenerateAdjacencyIndices(const std::vector<XMFLOAT3>& _vertices, const std::vector<uint32_t>& _indices, std::vector<uint32_t>& _adjIndicies)
{
	size_t numVerts = _vertices.size();
	size_t numFaces = _indices.size() / 3;

	_adjIndicies.resize(numFaces * 6);
	TriFaceGroup* triFaceGroups = new TriFaceGroup[numFaces];
	std::unordered_map<Float3ForKey, TriFaceGroupPerVertPos, Float3Hash> findFaceGroupByPos;

	// vertex pos·Î face groupÀ» Ã£À» ¼ö ÀÖ°Ô ¸¸µê.
	for (size_t f = 0; f < numFaces; f++)
	{
		triFaceGroups[f].indice[0] = _indices[(f * 3)];
		triFaceGroups[f].indice[1] = _indices[(f * 3) + 1];
		triFaceGroups[f].indice[2] = _indices[(f * 3) + 2];

		triFaceGroups[f].vertPos[0] = _vertices[_indices[(f * 3)]];
		triFaceGroups[f].vertPos[1] = _vertices[_indices[(f * 3) + 1]];
		triFaceGroups[f].vertPos[2] = _vertices[_indices[(f * 3) + 2]];

		// vert pos·Î index Ã£´Â map
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
