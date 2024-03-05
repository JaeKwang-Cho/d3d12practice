//***************************************************************************************
// GeometryGenerator.h by Frank Luna (C) 2011 All Rights Reserved.
//   
// Defines a static class for procedurally generating the geometry of 
// common mathematical objects.
//
// ��� �ﰢ���� �ٱ��� �ٶ󺸵��� �����ȴ�.
// ���� ������ �ٶ󺸵��� ����� �ʹٸ�, 
//	1. Cull Mode�� Winding ������ �ٲٰ�
//	2. �ﰢ���� normal�� �ݴ�� �ٲٰ�
//	3. TexCoords�� ź��Ʈ ���͸� �ٲ�� �Ѵ�.
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

	// �� �����͸� ���� ���� �Ѵ�.
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
		// ��ġ, ����, ź��Ʈ, UV��ǥ
        DirectX::XMFLOAT3 Position;
        DirectX::XMFLOAT3 Normal;
        DirectX::XMFLOAT3 TangentU;
        DirectX::XMFLOAT2 TexC;
	};

	struct MeshData
	{
		// �� ������, �ε��� ������
		std::vector<Vertex> Vertices;
        std::vector<uint32> Indices32;

		// �� �ε��� ������ ���� �� ���
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
	/// �ڽ��� origin�� �־��� ���� �Ӽ����� �����ؼ� �����.
	/// �ʺ�, ����, ����, �κ��� ��
	///</summary>
    MeshData CreateBox(float _width, float _height, float _depth, uint32 _numSubdivisions);

	///<summary>
	/// ���� origion�� �־��� �Ӽ����� �����ؼ� �����.
	/// ������, ���� ��, ���� ��
	/// (���� ��, ���� ���� �׼����̼��� �����Ѵ�.)
	///</summary>
    MeshData CreateSphere(float _radius, uint32 _sliceCount, uint32 _stackCount);

	///<summary>
	/// �������� ��(������)�� origion�� �־��� �Ӽ����� �����.
	/// ������, �κ��� ��
	/// (depth�� �׼����̼��� �����Ѵ�.)
	///</summary>
    MeshData CreateGeosphere(float _radius, uint32 _numSubdivisions);

	///<summary>
	/// ������� y �࿡ �����ϰ�, origin�� �����.
	/// �ٴ� ������, ���� ������, ����, ���� ��, ���� ��
	/// (���μ�, ���μ��� �׼����̼��� �����Ѵ�.)
	///</summary>
    MeshData CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount);

	///<summary>
	/// XZ ����� �־��� �Ӽ����� �簢���� �����.
	/// ����, ����, ���� �κ�, ���� �κ�
	///</summary>
    MeshData CreateGrid(float _width, float _depth, uint32 _m, uint32 _n);

	///<summary>
	/// XZ ����� �־��� �Ӽ����� �׼����̼� �� ��ġ�� �����.
	/// ����, ����, ���� �κ�, ���� �κ�
	///</summary>
	MeshData CreatePatchQuad(float _width, float _depth, uint32 _m, uint32 _n);

	///<summary>
	/// ȭ�鿡 ���ĵǴ� �簢���� �׸���.
	/// Post Processing�� �ϴµ� �ַ� ���δ�.
	/// ���� x, ���� y, �ʺ�(����������), ����(����), ����
	///</summary>
    MeshData CreateQuad(float _x, float _y, float _w, float _h, float _depth);

private:
	// ��ü ������ �κ����� ������ ������ �Ѵ�.
	void Subdivide(MeshData& _meshData);

	// ���ڷ� ���� �� �� ���� Vertex�� �������ش�.
    Vertex MidPoint(const Vertex& _v0, const Vertex& _v1);

	// ������� �� �Ѳ��� ������ش�.
    void BuildCylinderTopCap(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData);

	// ������� �Ʒ� ��ħ�� ������ش�.
    void BuildCylinderBottomCap(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData);
};

