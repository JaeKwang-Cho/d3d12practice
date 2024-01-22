#pragma once

#include "../Common/MathHelper.h"
#include "FrameResource.h"
#include <fstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

namespace Prac3
{
	XMFLOAT2 VertexToApproxSphericalRadian(const XMFLOAT3& _ver)
	{
		XMFLOAT2 uv = { 0.f, 0.f };

		XMVECTOR vec = { _ver.x, _ver.y, _ver.z, 0.f };
		vec = XMVector3Normalize(vec);
		XMFLOAT3 ver;
		XMStoreFloat3(&ver, vec);

		float theta = acosf(ver.y);
		if (theta > 0.0f)
		{
			float sinPhi = ver.z / sin(theta);
			float phi = asinf(sinPhi);

			uv.x = phi;
			uv.y = theta;
		}
		
		return uv;
	}

	void Prac3VerticesNIndicies(const wstring& _Path, vector<Vertex>& _outVertices, vector<uint32_t>& _outIndices)
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
				// XMFLOAT2 uv = VertexToApproxSphericalRadian(pos);
				XMFLOAT2 uv(0.f, 0.f);
				Vertex skullVert = { pos, norm, uv };
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
