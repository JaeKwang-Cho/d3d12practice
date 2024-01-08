#pragma once

#include "../Common/MathHelper.h"
#include <fstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

#define PRAC3 (0)

// ����� Vertex ����
struct VertexSkull {
	XMFLOAT3 Pos;
	XMFLOAT3 Norm;
};

namespace Prac3
{
	void Prac3VerticesNIndicies(const wstring& _Path, vector<VertexSkull>& _outVertices, vector<uint32_t>& _outIndices)
	{
		ifstream fin;
		fin.open(_Path);

		string TrashBin;
		UINT VertexCount;
		UINT IndicesCount;

		float px, py, pz, nx, ny, nz;
		uint32_t index;

		if (fin.is_open())
		{
			fin >> TrashBin;
			fin >> VertexCount;

			fin >> TrashBin;
			fin >> IndicesCount;

			IndicesCount *= 3;

			_outVertices.reserve(VertexCount);
			_outIndices.reserve(IndicesCount);

			getline(fin, TrashBin);
			getline(fin, TrashBin);
			fin >> TrashBin;
			for (UINT i = 0; i < VertexCount; i++)
			{
				fin >> px >> py >> pz >> nx >> ny >> nz;
				XMFLOAT3 pos(px, py, pz);
				XMFLOAT3 norm(nx, ny, nz);
				VertexSkull skullVert = { pos, norm };
				_outVertices.push_back(skullVert);
			}
			fin >> TrashBin;
			fin >> TrashBin;
			fin >> TrashBin;

			for (UINT i = 0; i < IndicesCount; i++)
			{
				fin >> index;
				_outIndices.push_back(index);
			}
			fin >> TrashBin;
		}

		fin.close();
	}
}
