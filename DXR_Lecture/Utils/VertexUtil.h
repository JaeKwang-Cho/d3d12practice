#pragma once
#include <DirectXMath.h>
#include "typedef.h"

namespace VertexUtil
{
	ULONG CreateBoxMesh(BasicVertex** _ppOutVertexList, WORD* _pOutIndexList, ULONG _maxBufferCount, float _halfBoxLen);
	void DeleteBoxMesh(BasicVertex* _pVertexList);

	ULONG CreateBottomMesh(BasicVertex* _pOutVertexList, ULONG _maxVertexCount, WORD* _pOutIndexList, ULONG _maxIndexCount, float _fHalfWidthDepth, float _fHeight);
	ULONG CreateWallMesh(BasicVertex* _pOutVertexList, ULONG _maxVertexCount, WORD* _pOutIndexList, ULONG _maxIndexCount, float _fHalfWidthDepth, float _fHeight);

	ULONG CreateGridPerPlane(BasicVertex* _pOutVertexList, ULONG _maxVertexBufferCount, WORD* _pOutIndexList, ULONG _maxIndexBufferCount, const DirectX::XMFLOAT3* _pStart, const DirectX::XMFLOAT3* _pEnd,
							 ULONG _startVertexIndex,
							 int _width, int _height,
							 int _u_index, int _v_index,
							 const DirectX::XMFLOAT4* _pColor,
							 ULONG* _pOutIndexCount);

	ULONG CreateGridBox(BasicVertex** _ppOutVertexList, WORD** _ppOutIndexList, ULONG* _pOutIndexCount, int _width, int _height, float _halfBoxLen);
	void DeleteGridBox(BasicVertex** _ppInOutVertexList, WORD** _ppInOutIndexList);
}
