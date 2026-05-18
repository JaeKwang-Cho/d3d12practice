// VertexUtil.cpp from "megayuchi"

#include "pch.h"
#include "VertexUtil.h"

const std::vector<ColorMeshData> CreateTileGrid(UINT _gridCellGapOffest)
{
	std::vector<ColorMeshData> meshData;
	meshData.push_back(ColorMeshData());

	int vertexCount = 11;

	// -x+, -y+ 번갈아 가면서 넣어주고
	ColorMeshData& refMeshData = meshData[0];

	refMeshData.Vertices.resize(vertexCount * 2);
	refMeshData.Indices32.resize(vertexCount * 2);
	for (int i = 0; i < vertexCount; i++)
	{
		int curIndex = i * 2;
		refMeshData.Vertices[curIndex].position = XMFLOAT3(float(i - vertexCount / 2) * _gridCellGapOffest, 0.f, 0.f);
		refMeshData.Vertices[curIndex].color = XMFLOAT4(DirectX::Colors::DarkRed);
		refMeshData.Vertices[curIndex].texCoord = XMFLOAT2(0.f, 0.f); // 텍스쳐는 입히지 않는다.

		refMeshData.Vertices[curIndex + 1].position = XMFLOAT3(0.f, 0.f, float(i - vertexCount / 2) * _gridCellGapOffest);
		refMeshData.Vertices[curIndex + 1].color = XMFLOAT4(DirectX::Colors::DarkGreen);
		refMeshData.Vertices[curIndex + 1].texCoord = XMFLOAT2(0.f, 0.f); // 텍스쳐는 입히지 않는다.

		// 인덱스도 적당히 짝지어주는 거로 넘긴다.
		refMeshData.Indices32[curIndex] = curIndex;
		refMeshData.Indices32[curIndex + 1] = curIndex + 1;
	}

	return meshData;
}


