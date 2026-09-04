#include "pch.h"
#include <Windows.h>
#include <DirectXMath.h>
#include "VertexUtil.h"
#ifdef __INTELLISENSE__
#include "../DXRPractice/pch.h"
#endif

using namespace DirectX;

namespace
{
	XMVECTOR ComputeNormal(const XMVECTOR& _p0, const XMVECTOR& _p1, const XMVECTOR& _p2)
	{
		XMVECTOR v1 = XMVectorSubtract(_p1, _p0);
		XMVECTOR v2 = XMVectorSubtract(_p2, _p0);
		XMVECTOR normal = XMVector3Cross(v1, v2);
		return XMVector3Normalize(normal);
	}

	XMVECTOR ComputeTangent(const XMVECTOR& _p0, const XMVECTOR& _p1, const XMVECTOR& _p2, const XMFLOAT2& _uv0, const XMFLOAT2& _uv1, const XMFLOAT2& _uv2)
	{
		XMVECTOR edge1 = XMVectorSubtract(_p1, _p0);
		XMVECTOR edge2 = XMVectorSubtract(_p2, _p0);

		float du1 = _uv1.x - _uv0.x;
		float dv1 = _uv1.y - _uv0.y;
		float du2 = _uv2.x - _uv0.x;
		float dv2 = _uv2.y - _uv0.y;

		float f = 1.0f / (du1 * dv2 - du2 * dv1);

		XMVECTOR tangent = XMVectorSet(
			f * (dv2 * XMVectorGetX(edge1) - dv1 * XMVectorGetX(edge2)),
			f * (dv2 * XMVectorGetY(edge1) - dv1 * XMVectorGetY(edge2)),
			f * (dv2 * XMVectorGetZ(edge1) - dv1 * XMVectorGetZ(edge2)),
			0.0f
		);

		return XMVector3Normalize(tangent);
	}

	ULONG AddVertex(BasicVertex* _pVertexList, ULONG _maxVertexCount, ULONG* _pInOutVertexCount, const BasicVertex* _pVertex)
	{
		ULONG existVertexCount = *_pInOutVertexCount;
		for (ULONG i = 0; i < existVertexCount; ++i)
		{
			const BasicVertex* pExistVertex = _pVertexList + i;
			if (memcmp(pExistVertex, _pVertex, sizeof(BasicVertex)) == 0)
			{
				return i;
			}
		}
		if (existVertexCount + 1 > _maxVertexCount)
		{
			__debugbreak();
			return static_cast<ULONG>(-1);
		}
		
		ULONG newIndex = existVertexCount;
		_pVertexList[newIndex] = *_pVertex;
		*_pInOutVertexCount = existVertexCount + 1;
		return newIndex;
	}
}

namespace VertexUtil
{
	ULONG CreateGridPerPlane(BasicVertex* _pOutVertexList, ULONG _maxVertexBufferCount, USHORT* _pOutIndexList, ULONG _maxIndexBufferCount, const XMFLOAT3* _pStart, const XMFLOAT3* _pEnd,
							 ULONG _startVertexIndex,
							 int _width, int _height,
							 int _u_index, int _v_index,
							 const XMFLOAT4* _pColor,
							 ULONG* _pOutIndexCount)
	{
		ULONG vertexCount = 0;
		ULONG indexCount = 0;

		ULONG requiredVertexCount = (_width + 1) * (_height + 1);
		ULONG requiredIndexCount = _width * _height * 2 * 3;

		if (_maxVertexBufferCount < requiredVertexCount)
			__debugbreak();

		if (_maxIndexBufferCount < requiredIndexCount)
			__debugbreak();

		const float* p_start_u = &_pStart->x + _u_index;
		const float* p_start_v = &_pStart->x + _v_index;
		const float* p_end_u = &_pEnd->x + _u_index;
		const float* p_end_v = &_pEnd->x + _v_index;

		float pos_offset_u = (*p_end_u - *p_start_u) / static_cast<float>(_width);
		float pos_offset_v = (*p_end_v - *p_start_v) / static_cast<float>(_height);

		int width_vertex_count = _width + 1;
		int height_vertex_count = _height + 1;
		
		float tex_coord_offset_u = 1.0f / static_cast<float>(_width);
		float tex_coord_offset_v = 1.0f / static_cast<float>(_height);

		for (int v = 0; v < height_vertex_count; ++v)
		{
			for (int u = 0; u < width_vertex_count; ++u)
			{
				BasicVertex* pDestVertex = _pOutVertexList + (u + v * width_vertex_count);
				pDestVertex->position = *_pStart;
				pDestVertex->color = *_pColor;
				
				float* p_dest_u = &pDestVertex->position.x + _u_index;
				float* p_dest_v = &pDestVertex->position.x + _v_index;
				*p_dest_u += pos_offset_u * static_cast<float>(u);
				*p_dest_v += pos_offset_v * static_cast<float>(v);

				pDestVertex->texCoord.x = tex_coord_offset_u * static_cast<float>(u);
				pDestVertex->texCoord.y = tex_coord_offset_v * static_cast<float>(v);
				vertexCount++;
			}
		}

		for (int v = 0; v < _height; ++v)
		{
			for (int u = 0; u < _width; ++u)
			{
				USHORT* pDestIndex = _pOutIndexList + ((u * 2 * 3) + v * _width * (2 * 3));
				pDestIndex[0] = static_cast<USHORT>(u + (v * width_vertex_count));
				pDestIndex[1] = static_cast<USHORT>((u + 1) + (v * width_vertex_count));
				pDestIndex[2] = static_cast<USHORT>((u + 1) + ((v + 1) * width_vertex_count));
				pDestIndex[3] = pDestIndex[0];
				pDestIndex[4] = pDestIndex[2];
				pDestIndex[5] = static_cast<USHORT>((u) + ((v + 1) * width_vertex_count));
				indexCount += 6;
			}
		}
		
		XMVECTOR p0 = { _pOutVertexList[_pOutIndexList[0]].position.x, _pOutVertexList[_pOutIndexList[0]].position.y, _pOutVertexList[_pOutIndexList[0]].position.z };
		XMVECTOR p1 = { _pOutVertexList[_pOutIndexList[1]].position.x, _pOutVertexList[_pOutIndexList[1]].position.y, _pOutVertexList[_pOutIndexList[1]].position.z };
		XMVECTOR p2 = { _pOutVertexList[_pOutIndexList[2]].position.x, _pOutVertexList[_pOutIndexList[2]].position.y, _pOutVertexList[_pOutIndexList[2]].position.z };

		XMFLOAT2 uv0 = { _pOutVertexList[_pOutIndexList[0]].texCoord.x, _pOutVertexList[_pOutIndexList[0]].texCoord.y };
		XMFLOAT2 uv1 = { _pOutVertexList[_pOutIndexList[1]].texCoord.x, _pOutVertexList[_pOutIndexList[1]].texCoord.y };
		XMFLOAT2 uv2 = { _pOutVertexList[_pOutIndexList[2]].texCoord.x, _pOutVertexList[_pOutIndexList[2]].texCoord.y };
		
		XMVECTOR normal = ComputeNormal(p0, p1, p2);
		XMVECTOR tangent = ComputeTangent(p0, p1, p2, uv0, uv1, uv2);
		for (ULONG i = 0; i < vertexCount; ++i)
		{
			_pOutVertexList[i].normal = { normal.m128_f32[0], normal.m128_f32[1], normal.m128_f32[2] };
			_pOutVertexList[i].tangent = { tangent.m128_f32[0], tangent.m128_f32[1], tangent.m128_f32[2] };
		}

		for (ULONG i = 0; i < indexCount; ++i)
		{
			_pOutIndexList[i] += static_cast<USHORT>(_startVertexIndex);
		}

		*_pOutIndexCount = indexCount;
		return vertexCount;
	}

	ULONG CreateGridBox(BasicVertex** _ppOutVertexList, USHORT** _ppOutIndexList, ULONG* _pOutIndexCount, int _width, int _height, float _halfBoxLen)
	{
		ULONG vertexCount = 0;
		ULONG indexCount = 0;

		XMFLOAT3 pWorldPosList[8];
		pWorldPosList[0] = { -_halfBoxLen, _halfBoxLen, _halfBoxLen };
		pWorldPosList[1] = { -_halfBoxLen, -_halfBoxLen, _halfBoxLen };
		pWorldPosList[2] = { _halfBoxLen, -_halfBoxLen, _halfBoxLen };
		pWorldPosList[3] = { _halfBoxLen, _halfBoxLen, _halfBoxLen };
		pWorldPosList[4] = { -_halfBoxLen, _halfBoxLen, -_halfBoxLen };
		pWorldPosList[5] = { -_halfBoxLen, -_halfBoxLen, -_halfBoxLen };
		pWorldPosList[6] = { _halfBoxLen, -_halfBoxLen, -_halfBoxLen };
		pWorldPosList[7] = { _halfBoxLen, _halfBoxLen, -_halfBoxLen };

		// +z
		XMFLOAT3 p_z_start = pWorldPosList[3];
		XMFLOAT3 p_z_end = pWorldPosList[1];
		XMFLOAT4 p_z_color = { 0.0f, 0.0f, 1.0f, 1.0f };
		
		// -z
		XMFLOAT3 n_z_start = pWorldPosList[4];
		XMFLOAT3 n_z_end = pWorldPosList[6];
		XMFLOAT4 n_z_color = { 0.5f, 0.5f, 0.0f, 1.0f };

		// -x
		XMFLOAT3 n_x_start = pWorldPosList[0];
		XMFLOAT3 n_x_end = pWorldPosList[5];
		XMFLOAT4 n_x_color = { 0.0f, 0.5f, 0.5f, 1.0f };

		// +x
		XMFLOAT3 p_x_start = pWorldPosList[7];
		XMFLOAT3 p_x_end = pWorldPosList[2];
		XMFLOAT4 p_x_color = { 1.0f, 0.0f, 0.0f, 1.0f };

		// +y
		XMFLOAT3 p_y_start = pWorldPosList[0];
		XMFLOAT3 p_y_end = pWorldPosList[7];
		XMFLOAT4 p_y_color = { 0.0f, 1.0f, 0.0f, 1.0f };

		// -y
		XMFLOAT3 n_y_start = pWorldPosList[2];
		XMFLOAT3 n_y_end = pWorldPosList[5];
		XMFLOAT4 n_y_color = { 0.5f, 0.0f, 0.5f, 1.0f };

		ULONG maxVertexCountPerPlane = (_width + 1) * (_height + 1);
		ULONG maxVertexCount = maxVertexCountPerPlane * 6;
		BasicVertex* pVertexList = new BasicVertex[maxVertexCount];
		memset(pVertexList, 0, sizeof(BasicVertex) * maxVertexCount);

		ULONG maxIndexCountPerPlane = _width * _height * 2 * 3;
		ULONG maxIndexCount = maxIndexCountPerPlane * 6;
		USHORT* pIndexList = new USHORT[maxIndexCount];
		memset(pIndexList, 0, sizeof(USHORT) * maxIndexCount);
		
		// -z
		ULONG indexCountPerPlane = 0;
		ULONG basicVertexCountPerPlane = CreateGridPerPlane(pVertexList, maxVertexCount, pIndexList, maxIndexCount, &n_z_start, &n_z_end, vertexCount, _width, _height, 0, 1, &n_z_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		// +z
		basicVertexCountPerPlane = CreateGridPerPlane(pVertexList + vertexCount, maxVertexCount - vertexCount, pIndexList + indexCount, maxIndexCount - indexCount, &p_z_start, &p_z_end, vertexCount, _width, _height, 0, 1, &p_z_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		// -x
		basicVertexCountPerPlane = CreateGridPerPlane(pVertexList + vertexCount, maxVertexCount - vertexCount, pIndexList + indexCount, maxIndexCount - indexCount, &n_x_start, &n_x_end, vertexCount, _width, _height, 2, 1, &n_x_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		// +x
		basicVertexCountPerPlane = CreateGridPerPlane(pVertexList + vertexCount, maxVertexCount - vertexCount, pIndexList + indexCount, maxIndexCount - indexCount, &p_x_start, &p_x_end, vertexCount, _width, _height, 2, 1, &p_x_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		// +y
		basicVertexCountPerPlane = CreateGridPerPlane(pVertexList + vertexCount, maxVertexCount - vertexCount, pIndexList + indexCount, maxIndexCount - indexCount, &p_y_start, &p_y_end, vertexCount, _width, _height, 0, 2, &p_y_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		// -y
		basicVertexCountPerPlane = CreateGridPerPlane(pVertexList + vertexCount, maxVertexCount - vertexCount, pIndexList + indexCount, maxIndexCount - indexCount, &n_y_start, &n_y_end, vertexCount, _width, _height, 0, 2, &n_y_color, &indexCountPerPlane);
		vertexCount += basicVertexCountPerPlane;
		indexCount += indexCountPerPlane;

		*_ppOutVertexList = pVertexList;
		*_pOutIndexCount = indexCount;
		*_ppOutIndexList = pIndexList;

		return vertexCount;
	}

	void DeleteGridBox(BasicVertex** _ppInOutVertexList, USHORT** _ppInOutIndexList)
	{
		BasicVertex* pVertexList = *_ppInOutVertexList;
		if (pVertexList)
		{
			delete[] pVertexList;
			*_ppInOutVertexList = nullptr;
		}
		
		USHORT* pIndexList = *_ppInOutIndexList;
		if (pIndexList)
		{
			delete[] pIndexList;
			*_ppInOutIndexList = nullptr;
		}
	}

	ULONG CreateBoxMesh(BasicVertex** _ppOutVertexList, USHORT* _pOutIndexList, ULONG _maxBufferCount, float _halfBoxLen)
	{
		const ULONG INDEX_COUNT = 36;
		if (_maxBufferCount < INDEX_COUNT)
			__debugbreak();

		const USHORT pIndexList[INDEX_COUNT] =
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
		
		TVERTEX pTexCoordList[INDEX_COUNT] =
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
		FLOAT3 pNormalList[INDEX_COUNT] =
		{
			// +z
			{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
			{0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f, 1.0f},
			
			// -z
			{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},
			{0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f, -1.0f},

			// -x
			{-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},
			{-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f},

			// +x
			{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},
			{1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f},

			// +y
			{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			{0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f},
			
			// -y
			{0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f},
			{0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}
		};
		FLOAT3 pTangentList[INDEX_COUNT] = {};
		
		FLOAT3 pWorldPosList[8];
		pWorldPosList[0] = { -_halfBoxLen, _halfBoxLen, _halfBoxLen };
		pWorldPosList[1] = { -_halfBoxLen, -_halfBoxLen, _halfBoxLen };
		pWorldPosList[2] = { _halfBoxLen, -_halfBoxLen, _halfBoxLen };
		pWorldPosList[3] = { _halfBoxLen, _halfBoxLen, _halfBoxLen };
		pWorldPosList[4] = { -_halfBoxLen, _halfBoxLen, -_halfBoxLen };
		pWorldPosList[5] = { -_halfBoxLen, -_halfBoxLen, -_halfBoxLen };
		pWorldPosList[6] = { _halfBoxLen, -_halfBoxLen, -_halfBoxLen };
		pWorldPosList[7] = { _halfBoxLen, _halfBoxLen, -_halfBoxLen };

		
		for (ULONG i = 0; i < INDEX_COUNT / 3; ++i)
		{
			XMVECTOR p0 = { pWorldPosList[pIndexList[i * 3 + 0]].x, pWorldPosList[pIndexList[i * 3 + 0]].y, pWorldPosList[pIndexList[i * 3 + 0]].z, 1.0f };
			XMVECTOR p1 = { pWorldPosList[pIndexList[i * 3 + 1]].x, pWorldPosList[pIndexList[i * 3 + 1]].y, pWorldPosList[pIndexList[i * 3 + 1]].z, 1.0f };
			XMVECTOR p2 = { pWorldPosList[pIndexList[i * 3 + 2]].x, pWorldPosList[pIndexList[i * 3 + 2]].y, pWorldPosList[pIndexList[i * 3 + 2]].z, 1.0f };

			XMFLOAT2 uv0 = { pTexCoordList[i * 3 + 0].u, pTexCoordList[i * 3 + 0].v };
			XMFLOAT2 uv1 = { pTexCoordList[i * 3 + 1].u, pTexCoordList[i * 3 + 1].v };
			XMFLOAT2 uv2 = { pTexCoordList[i * 3 + 2].u, pTexCoordList[i * 3 + 2].v };

			XMVECTOR tangent = ComputeTangent(p0, p1, p2, uv0, uv1, uv2);

			pTangentList[i * 3 + 0].x = tangent.m128_f32[0];
			pTangentList[i * 3 + 0].y = tangent.m128_f32[1];
			pTangentList[i * 3 + 0].z = tangent.m128_f32[2];

			pTangentList[i * 3 + 1].x = tangent.m128_f32[0];
			pTangentList[i * 3 + 1].y = tangent.m128_f32[1];
			pTangentList[i * 3 + 1].z = tangent.m128_f32[2];

			pTangentList[i * 3 + 2].x = tangent.m128_f32[0];
			pTangentList[i * 3 + 2].y = tangent.m128_f32[1];
			pTangentList[i * 3 + 2].z = tangent.m128_f32[2];
		}

		const ULONG MAX_WORKING_VERTEX_COUNT = 65536;
		BasicVertex* pWorkingVertexList = new BasicVertex[MAX_WORKING_VERTEX_COUNT];
		memset(pWorkingVertexList, 0, sizeof(BasicVertex) * MAX_WORKING_VERTEX_COUNT);
		ULONG dwBasicVertexCount = 0;

		for (ULONG i = 0; i < INDEX_COUNT; ++i)
		{
			BasicVertex v;
			v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
			v.position = { pWorldPosList[pIndexList[i]].x, pWorldPosList[pIndexList[i]].y, pWorldPosList[pIndexList[i]].z };
			v.normal = {pNormalList[i].x, pNormalList[i].y, pNormalList[i].z };
			v.tangent = {pTangentList[i].x, pTangentList[i].y, pTangentList[i].z };
			v.texCoord = { pTexCoordList[i].u, pTexCoordList[i].v };

			_pOutIndexList[i] = static_cast<USHORT>(AddVertex(pWorkingVertexList, MAX_WORKING_VERTEX_COUNT, &dwBasicVertexCount, &v));
		}
		BasicVertex* pNewVertexList = new BasicVertex[dwBasicVertexCount];
		memcpy(pNewVertexList, pWorkingVertexList, sizeof(BasicVertex) * dwBasicVertexCount);

		*_ppOutVertexList = pNewVertexList;

		delete[] pWorkingVertexList;
		return dwBasicVertexCount;
	}

	ULONG CreateBottomMesh(BasicVertex* _pOutVertexList, ULONG _maxVertexCount, USHORT* _pOutIndexList, ULONG _maxIndexCount, float _fHalfWidthDepth, float _fHeight)
	{
		if (_maxVertexCount < 4)
			__debugbreak();

		if (_maxIndexCount < 6)
			__debugbreak();

		const USHORT pIndexList[6] =
		{
			0, 1, 2,
			0, 2, 3
		};
		
		FLOAT3 pWorldPosList[4];
		pWorldPosList[0] = { -_fHalfWidthDepth, _fHeight, _fHalfWidthDepth };
		pWorldPosList[1] = { _fHalfWidthDepth, _fHeight, _fHalfWidthDepth };
		pWorldPosList[2] = { _fHalfWidthDepth, _fHeight, -_fHalfWidthDepth };
		pWorldPosList[3] = { -_fHalfWidthDepth, _fHeight, -_fHalfWidthDepth };

		TVERTEX pTexCoordList[4] =
		{
			{0.0f, 0.0f}, 
			{4.0f, 0.0f},
			{4.0f, 4.0f},
			{0.0f, 4.0f}
		};

		XMVECTOR p0 = { pWorldPosList[0].x, pWorldPosList[0].y, pWorldPosList[0].z };
		XMVECTOR p1 = { pWorldPosList[1].x, pWorldPosList[1].y, pWorldPosList[1].z };
		XMVECTOR p2 = { pWorldPosList[2].x, pWorldPosList[2].y, pWorldPosList[2].z };
		XMFLOAT2 t0 = { pTexCoordList[0].u, pTexCoordList[0].v };
		XMFLOAT2 t1 = { pTexCoordList[1].u, pTexCoordList[1].v };
		XMFLOAT2 t2 = { pTexCoordList[2].u, pTexCoordList[2].v };

		XMVECTOR tangent = ComputeTangent(p0, p1, p2, t0, t1, t2);
		for (ULONG i = 0; i < 4; ++i)
		{
			BasicVertex v;
			v.position = { pWorldPosList[i].x, pWorldPosList[i].y, pWorldPosList[i].z };
			v.texCoord = { pTexCoordList[i].u, pTexCoordList[i].v };
			v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
			v.normal = { 0.0f, 1.0f, 0.0f };
			v.tangent = { tangent.m128_f32[0], tangent.m128_f32[1], tangent.m128_f32[2] };
			
			_pOutVertexList[i] = v;
		}
		memcpy(_pOutIndexList, pIndexList, sizeof(USHORT) * 6);

		return 4;
	}

	ULONG CreateWallMesh(BasicVertex* _pOutVertexList, ULONG _maxVertexCount, USHORT* _pOutIndexList, ULONG _maxIndexCount, float _fHalfWidthDepth, float _fHeight)
	{
		if (_maxVertexCount < 4)
			__debugbreak();

		if (_maxIndexCount < 6)
			__debugbreak();

		const USHORT pIndexList[6] =
		{
			0, 1, 2,
			0, 2, 3
		};
		
		FLOAT3 pWorldPosList[4];
		pWorldPosList[0] = { -_fHalfWidthDepth, _fHalfWidthDepth, 0.0f };
		pWorldPosList[1] = { _fHalfWidthDepth, _fHalfWidthDepth, 0.0f };
		pWorldPosList[2] = { _fHalfWidthDepth, -_fHalfWidthDepth, 0.0f };
		pWorldPosList[3] = { -_fHalfWidthDepth, -_fHalfWidthDepth, 0.0f };

		TVERTEX pTexCoordList[4] =
		{
			{0.0f, 0.0f}, 
			{4.0f, 0.0f},
			{4.0f, 4.0f},
			{0.0f, 4.0f}
		};

		XMVECTOR p0 = { pWorldPosList[0].x, pWorldPosList[0].y, pWorldPosList[0].z };
		XMVECTOR p1 = { pWorldPosList[1].x, pWorldPosList[1].y, pWorldPosList[1].z };
		XMVECTOR p2 = { pWorldPosList[2].x, pWorldPosList[2].y, pWorldPosList[2].z };
		XMFLOAT2 t0 = { pTexCoordList[0].u, pTexCoordList[0].v };
		XMFLOAT2 t1 = { pTexCoordList[1].u, pTexCoordList[1].v };
		XMFLOAT2 t2 = { pTexCoordList[2].u, pTexCoordList[2].v };

		XMVECTOR tangent = ComputeTangent(p0, p1, p2, t0, t1, t2);
		for (ULONG i = 0; i < 4; ++i)
		{
			BasicVertex v;
			v.position = { pWorldPosList[i].x, pWorldPosList[i].y, pWorldPosList[i].z };
			v.texCoord = { pTexCoordList[i].u, pTexCoordList[i].v };
			v.color = { 1.0f, 1.0f, 1.0f, 1.0f };
			v.normal = { 0.0f, 0.0f, -1.0f };
			v.tangent = { tangent.m128_f32[0], tangent.m128_f32[1], tangent.m128_f32[2] };
			
			_pOutVertexList[i] = v;
		}
		memcpy(_pOutIndexList, pIndexList, sizeof(USHORT) * 6);

		return 4;
	}

	void DeleteBoxMesh(BasicVertex* _pVertexList)
	{
		delete[] _pVertexList;
	}
}