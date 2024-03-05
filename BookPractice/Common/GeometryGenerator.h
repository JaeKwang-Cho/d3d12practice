//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// 모든 삼각형은 바깥을 바라보도록 생성된다.
// 만약 안쪽을 바라보도록 만들고 싶다면, 
//	1. Cull Mode의 Winding 방향을 바꾸고
//	2. 삼각형의 normal을 반대로 바꾸고
//	3. TexCoords와 탄젠트 벡터를 바꿔야 한다.
//***************************************************************************************

#pragma once

#include <cstdint>
#include <DirectXMath.h>
#include <vector>

class GeometryGenerator
{
public:

    using uint16 = std::uint16_t;
    using uint32 = std::uint32_t;

	// 점 데이터를 새로 정의 한다.
	struct Vertex
	{
		Vertex() = default;
        Vertex(
            const DirectX::XMFLOAT3& _p, 
            const DirectX::XMFLOAT3& _n, 
            const DirectX::XMFLOAT3& _t, 
            const DirectX::XMFLOAT2& _uv) :
            Position(_p), 
            Normal(_n), 
            TangentU(_t), 
            TexC(_uv){}
		Vertex(
			float _px, float _py, float _pz, 
			float _nx, float _ny, float _nz,
			float _tx, float _ty, float _tz,
			float _u, float _v) : 
            Position(_px,_py,_pz), 
            Normal(_nx,_ny,_nz),
			TangentU(_tx, _ty, _tz), 
            TexC(_u,_v){}
		// 위치, 법선, 탄젠트, UV좌표
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		// 점 데이터, 인덱스 데이터
		std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;

		// 총 인덱스 개수가 적을 때 사용
        std::vector<uint16>& GetIndices16()
        {
			if(mIndices16.empty())
			{
				mIndices16.resize(Indices32.size());
				for(size_t i = 0; i < Indices32.size(); ++i)
					mIndices16[i] = static_cast<uint16>(Indices32[i]);
			}

			return mIndices16;
        }

	private:
		std::vector<uint16> mIndices16;
	};

	///<summary>
	/// 박스를 origin에 주어진 차원 속성으로 구성해서 만든다.
	/// 너비, 높이, 깊이, 부분의 수
	///</summary>
    MeshData CreateBox(float _width, float _height, float _depth, uint32 _numSubdivisions);

	///<summary>
	/// 구를 origion에 주어진 속성으로 구성해서 만든다.
	/// 반지름, 세로 선, 가로 선
	/// (가로 선, 세로 선이 테셀레이션을 제어한다.)
	///</summary>
    MeshData CreateSphere(float _radius, uint32 _sliceCount, uint32 _stackCount);

	///<summary>
	/// 지오데식 구(측지구)를 origion에 주어진 속성으로 만든다.
	/// 반지름, 부분의 수
	/// (depth가 테셀레이션을 제어한다.)
	///</summary>
    MeshData CreateGeosphere(float _radius, uint32 _numSubdivisions);

	///<summary>
	/// 원기둥을 y 축에 평행하게, origin에 만든다.
	/// 바닥 반지름, 지붕 반지름, 높이, 세로 선, 가로 선
	/// (세로선, 가로선이 테셀레이션을 제어한다.)
	///</summary>
    MeshData CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount);

	///<summary>
	/// XZ 평면을 주어진 속성으로 사각형을 만든다.
	/// 가로, 세로, 가로 부분, 세로 부분
	///</summary>
    MeshData CreateGrid(float _width, float _depth, uint32 _m, uint32 _n);

	///<summary>
	/// XZ 평면을 주어진 속성으로 테셀레이션 용 패치를 만든다.
	/// 가로, 세로, 가로 부분, 세로 부분
	///</summary>
	MeshData CreatePatchQuad(float _width, float _depth, uint32 _m, uint32 _n);

	///<summary>
	/// 화면에 정렬되는 사각형을 그린다.
	/// Post Processing을 하는데 주로 쓰인다.
	/// 시작 x, 시작 y, 너비(오른쪽으로), 높이(위로), 깊이
	///</summary>
    MeshData CreateQuad(float _x, float _y, float _w, float _h, float _depth);

private:
	// 모체 도형을 부분으로 나누는 역할을 한다.
	void Subdivide(MeshData& _meshData);

	// 인자로 들어온 두 점 사이 Vertex를 정의해준다.
    Vertex MidPoint(const Vertex& _v0, const Vertex& _v1);

	// 원기둥의 윗 뚜껑을 만들어준다.
    void BuildCylinderTopCap(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData);

	// 원기둥의 아래 받침을 만들어준다.
    void BuildCylinderBottomCap(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData);
};

