// VertexUtil.h from "megayuchi"

#pragma once

// �� �鸶�� Texturing�� ������ �� �ִ� ť�긦 ������ش�
DWORD CreateBoxMesh(BasicVertex** _ppOutVertexList, uint16_t* _pOutIndexList, DWORD _dwMaxBufferCount, float _fHalfBoxLength);
void DeleteBoxMesh(BasicVertex* _pVertexList);

