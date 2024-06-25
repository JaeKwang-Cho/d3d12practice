// VertexUtil.h from "megayuchi"

#pragma once

// 한 면마다 Texturing을 연습할 수 있는 큐브를 만들어준다
DWORD CreateBoxMesh(BasicVertex** _ppOutVertexList, uint16_t* _pOutIndexList, DWORD _dwMaxBufferCount, float _fHalfBoxLength);
void DeleteBoxMesh(BasicVertex* _pVertexList);

