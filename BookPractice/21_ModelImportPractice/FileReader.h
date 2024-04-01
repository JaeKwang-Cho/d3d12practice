#pragma once

#include "../Common/MathHelper.h"
#include "FrameResource.h"
#include <fstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using namespace std;

namespace Dorasima
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

	void GetMeshFromFile(const wstring& _Path, vector<Vertex>& _outVertices, vector<uint32_t>& _outIndices, BoundingBox& _outBounds)
	{
		ifstream fin;
		fin.open(_Path);

		string TrashBin;
		UINT VertexCount;
		UINT IndicesCount;

		float px, py, pz, nx, ny, nz;
		uint32_t index;

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

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
				XMFLOAT2 uv = VertexToApproxSphericalRadian(pos);
				XMFLOAT3 tanU;

				XMVECTOR vPos = XMLoadFloat3(&pos);
				XMVECTOR vNorm = XMLoadFloat3(&norm);

				// skull 이 친구는 vertex도 많고, normal이 꽤나 잘 되어 있어서 따로 normal map을 사용하지 않는다. (diffuse map도 그냥 흰색 쓴다.)
				// 대충 Normal과 수직인 친구만 구하면 되니깐 Up 벡터를 이용해서 TangentU를 구한다. 
				// else는 Normal 너무 Up 벡터와 비슷해서 값을 제대로 구할 수 없을때 +z 벡터를 사용해서 TangentU를 구한다. 
				XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				if (fabsf(XMVectorGetX(XMVector3Dot(vNorm, up))) < 1.0f - 0.001f)
				{
					XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(up, vNorm));
					XMStoreFloat3(&tanU, vTanU);
				}
				else
				{
					up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
					XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(vNorm, up));
					XMStoreFloat3(&tanU, vTanU);
				}

				//XMFLOAT2 uv(0.f, 0.f);
				Vertex skullVert = { pos, norm, uv, tanU };
				_outVertices.push_back(skullVert);

				// XMVECTOR를 이용해서 성분별로 최대최소를 쉽게 구한다.
				XMVECTOR P = XMLoadFloat3(&pos);

				vMin = XMVectorMin(vMin, P);
				vMax = XMVectorMax(vMax, P);
			}
			XMStoreFloat3(&_outBounds.Center, 0.5f * (vMin + vMax));
			XMStoreFloat3(&_outBounds.Extents, 0.5f * (vMax - vMin));

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

	void GetMeshFromFile(const wstring& _Path, vector<Vertex>& _outVertices, vector<uint32_t>& _outIndices, BoundingSphere& _outBounds)
	{
		ifstream fin;
		fin.open(_Path);

		string TrashBin;
		UINT VertexCount;
		UINT IndicesCount;

		float px, py, pz, nx, ny, nz;
		uint32_t index;

		XMFLOAT3 vMinf3(+MathHelper::Infinity, +MathHelper::Infinity, +MathHelper::Infinity);
		XMFLOAT3 vMaxf3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);

		XMVECTOR vMin = XMLoadFloat3(&vMinf3);
		XMVECTOR vMax = XMLoadFloat3(&vMaxf3);

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
				XMFLOAT2 uv = VertexToApproxSphericalRadian(pos);
				XMFLOAT3 tanU;
				//XMFLOAT2 uv(0.f, 0.f);

				XMVECTOR vPos = XMLoadFloat3(&pos);
				XMVECTOR vNorm = XMLoadFloat3(&norm);

				// skull 이 친구는 vertex도 많고, normal이 꽤나 잘 되어 있어서 따로 normal map을 사용하지 않는다. (diffuse map도 그냥 흰색 쓴다.)
				// 대충 Normal과 수직인 친구만 구하면 되니깐 Up 벡터를 이용해서 TangentU를 구한다. 
				// else는 Normal 너무 Up 벡터와 비슷해서 값을 제대로 구할 수 없을때 +z 벡터를 사용해서 TangentU를 구한다. 
				XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
				if (fabsf(XMVectorGetX(XMVector3Dot(vNorm, up))) < 1.0f - 0.001f)
				{
					XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(up, vNorm));
					XMStoreFloat3(&tanU, vTanU);
				}
				else
				{
					up = XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f);
					XMVECTOR vTanU = XMVector3Normalize(XMVector3Cross(vNorm, up));
					XMStoreFloat3(&tanU, vTanU);
				}

				Vertex skullVert = { pos, norm, uv, tanU };
				_outVertices.push_back(skullVert);

				// XMVECTOR를 이용해서 성분별로 최대최소를 쉽게 구한다.
				XMVECTOR P = XMLoadFloat3(&pos);

				vMin = XMVectorMin(vMin, P);
				vMax = XMVectorMax(vMax, P);
			}
			// 일단 BoundingSphere의 중심을 정해놓고
			XMVECTOR vCenter = 0.5f * (vMin + vMax);
			XMStoreFloat3(&_outBounds.Center, vCenter);
			// 다시 루프를 돌면서 최소 반지름을 구한다.
			XMVECTOR vZero = XMVectorZero();
			for (UINT i = 0; i < VertexCount; i++)
			{
				XMVECTOR vPos = XMLoadFloat3(&_outVertices[i].Pos);
				XMVECTOR vSub = vPos - vCenter;
				XMVECTOR length = XMVector3Length(vSub);

				vZero = XMVectorMax(vZero, length);
			}
			XMStoreFloat(&_outBounds.Radius, vZero);

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
