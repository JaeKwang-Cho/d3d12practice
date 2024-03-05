//***************************************************************************************
// GeometryGenerator.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "GeometryGenerator.h"
#include <algorithm>

using namespace DirectX;

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float _width, float _height, float _depth, uint32 _numSubdivisions)
{
    MeshData meshData;

	// �ϴ� ��ü �ڽ��� �����.

	Vertex v[24];

	float w2 = 0.5f*_width;
	float h2 = 0.5f*_height;
	float d2 = 0.5f*_depth;
    
	// �ո� (pos, norm, tangent, UVc ��)
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// �޸� (pos, norm, tangent, UVc ��)
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// ���� (pos, norm, tangent, UVc ��)
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// �ظ� (pos, norm, tangent, UVc ��)
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// �޸� (pos, norm, tangent, UVc ��)
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// ������ (pos, norm, tangent, UVc ��)
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);
 
	// Index ���۸� �����.

	uint32 i[36];

	// ���� �ε���
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// �ĸ� �ε���
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// ���� �ε���
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// �ظ� �ε���
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// �޸� �ε���
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// ������ �ε���
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices32.assign(&i[0], &i[36]);

    // �ϴ� ���� ���� 6 ���Ϸ� ������ �δ� �����̴�.
    _numSubdivisions = std::min<uint32>(_numSubdivisions, 6u);

	// ���� �� ��ŭ ���� �Լ��� ȣ�����ش�.
    for(uint32 i = 0; i < _numSubdivisions; ++i)
        Subdivide(meshData);

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float _radius, uint32 _sliceCount, uint32 _stackCount)
{
    MeshData meshData;

	// Top Poles(�ϱ�)���� �ְ�, ������ (Stack)�� ���� �������� ���� ����Ѵ�.
	// Compute the vertices stating at the top pole and moving down the stacks.

	// Poles: ���簢�� �ؽ��ĸ� ���� ������ ��, 
	// ��(poles)�� �Ҵ�� ������ TexCoords�� ���� ������ �ְ��� �߻��մϴ�.
	Vertex topVertex(0.0f, +_radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -_radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	// �ϴ� Top Pole Vertex�� �־��ְ�
	meshData.Vertices.push_back( topVertex );

	// �� �������� PI�� ������ ������ ������
	float thetaStep   = XM_PI/_stackCount;
	// �� �������� 2PI�� ������ ������ ������.
	float phiStep = 2.0f*XM_PI/_sliceCount;

	// Stack Ring ���� �������鼭 Vertex�� ����Ѵ�. (pole�� ������� �ʴ´�.)
	// ���� ���η� �߶� ����� �ܸ� ���� ring�̴�.
	for(uint32 i = 1; i <= _stackCount-1; ++i)
	{
		float theta = i*thetaStep;

		// ring �� ����
        for(uint32 j = 0; j <= _sliceCount; ++j)
		{
			// ���� ��
			float phi = j*phiStep;

			Vertex v;

			// ���� ��ǥ�踦 ����ؼ� ���Ѵ�.
			v.Position.x = _radius*sinf(theta)*cosf(phi);
			v.Position.y = _radius*cosf(theta);
			v.Position.z = _radius*sinf(theta)*sinf(phi);

			// Phi�� ���ؼ� �κ� ���Լ��� �ɸ� TangentU�� ���� �� �ִ�.
			v.TangentU.x = -_radius*sinf(theta)*sinf(phi); // consf(Phi)�� ���ؼ��� �̺�
			v.TangentU.y = 0.0f; // ��� ����ϱ� ��������
			v.TangentU.z = +_radius*sinf(theta)*cosf(phi); // sinf(Phi)�� ���ؼ��� �̺�

			// TangentU ��� ������... �Ƹ��� Tangent Plane���� UV ��ǥ�迡�� U�� Vector�� ���ϴ� �� ����.
			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			// === �𸣴� �� ===
			// �׷��� �� TangentV�� ������?
			// Plane�� ������ Normal�� �����Ű�...�������� ���ϳ�?

			// �ϴ� Origin�� ����� �Ŵϱ�, ���� ������ ��ǥ ��ü�� normal�� ���� ���̴�.
			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			// �ؽ��� ��ǥ�� �׳�, ���� ������ ������ ������ ������ �������� ����.
			v.TexC.x = phi / XM_2PI;
			v.TexC.y = theta / XM_PI;

			// �׸��� �ִ´�.
			meshData.Vertices.push_back( v );
		}
	}

	// �׸��� ���� ���� �ִ´�.
	meshData.Vertices.push_back( bottomVertex );

	//
	// �ϱ����� ó���� �ְ�, ������ ���ð�
	// �����Ѵ�.

    for(uint32 i = 1; i <= _sliceCount; ++i)
	{
		meshData.Indices32.push_back(0); // �ϱ����� �Ź� �־
		meshData.Indices32.push_back(i+1); // �ٷξƷ� ring��
		meshData.Indices32.push_back(i); // �����ϰ� �ȴ�.
	}

	// ���� �� �Ʒ� ���ʴ�� �������鼭 stack�� �� �Ʒ� stack�� 
	// �ε����� �����ϱ� �����Ѵ�.	

	// (���� pole �ǳ� �ٰ�) 
	// ���� ring���� 2��, �Ʒ����� 1�� �ﰢ�� �ϳ�
	// ���� ring���� 1��, �Ʒ����� 2�� �ﰢ�� �ϳ���
	// �簢���� ���鵵�� �ϴ� index�� �ִ´�.
    uint32 baseIndex = 1;
    uint32 ringVertexCount = _sliceCount + 1;
	for(uint32 i = 0; i < _stackCount-2; ++i)
	{
		for(uint32 j = 0; j < _sliceCount; ++j)
		{
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j+1);
			meshData.Indices32.push_back(baseIndex + (i+1)*ringVertexCount + j);

			meshData.Indices32.push_back(baseIndex + (i+1)*ringVertexCount + j);
			meshData.Indices32.push_back(baseIndex + i*ringVertexCount + j+1);
			meshData.Indices32.push_back(baseIndex + (i+1)*ringVertexCount + j+1);
		}
	}

	//
	// ���� �� �Ʒ� ������ ����Ѵ�. ���� ���̶� �� �Ʒ� ���̶� �����Ѵ�.
	//

	// �ϴ� ���� ���ؽ� ��ȣ�� �̷���
	uint32 southPoleIndex = (uint32)meshData.Vertices.size()-1;

	// �ű⼭ ���� ���ؽ� ������ŭ ����, �� �Ʒ� ���� ù��° ���ý� ��ȣ�� ���� ���̴�.
	baseIndex = southPoleIndex - ringVertexCount;
	
	// �װɷ� ���� ���ư��鼭 �ε����� �������ָ� �ȴ�.
	for(uint32 i = 0; i < _sliceCount; ++i)
	{
		meshData.Indices32.push_back(southPoleIndex);
		meshData.Indices32.push_back(baseIndex+i);
		meshData.Indices32.push_back(baseIndex+i+1);
	}

    return meshData;
}
 
void GeometryGenerator::Subdivide(MeshData& _meshData)
{
	// �ϴ� ī�Ǹ� �س��´�.
	MeshData inputCopy = _meshData;

	// �׸��� ������ 0���� ���ֹ�����.
	_meshData.Vertices.resize(0);
	_meshData.Indices32.resize(0);

	//       v1
	//       *
	//      / \
	//     /   \
	//  m0*-----*m1
	//   / \   / \
	//  /   \ /   \
	// *-----*-----*
	// v0    m2     v2

	// �ϴ� ��ü ������ �ﰢ�� ������ �ε��� 3���� �𿩼� �����, �װɷ� ���Ѵ�.
	uint32 numTris = (uint32)inputCopy.Indices32.size()/3;
	for(uint32 i = 0; i < numTris; ++i)
	{
		// �ﰢ�� ���� ��ŭ ������ ���鼭
		Vertex v0 = inputCopy.Vertices[ inputCopy.Indices32[i*3+0] ];
		Vertex v1 = inputCopy.Vertices[ inputCopy.Indices32[i*3+1] ];
		Vertex v2 = inputCopy.Vertices[ inputCopy.Indices32[i*3+2] ];

		//
		// �ε����� ����Ű�� ���ؽ��� ���ϰ�
		// �װɷ� ���� �ﰢ���� �߰����� �������ν� �����.
		//

        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

		//
		// ���ο� ������ ���� �־��ش�.
		// �ϴ� ��ü �ְ�
		// �� ����, �̵� ����Ʈ�� �ְ�
		//

		_meshData.Vertices.push_back(v0); // 0
		_meshData.Vertices.push_back(v1); // 1
		_meshData.Vertices.push_back(v2); // 2
		_meshData.Vertices.push_back(m0); // 3
		_meshData.Vertices.push_back(m1); // 4
		_meshData.Vertices.push_back(m2); // 5
 
		// �׿� �´� �ε����� �־��ش�.
		_meshData.Indices32.push_back(i*6+0);
		_meshData.Indices32.push_back(i*6+3);
		_meshData.Indices32.push_back(i*6+5);

		_meshData.Indices32.push_back(i*6+3);
		_meshData.Indices32.push_back(i*6+4);
		_meshData.Indices32.push_back(i*6+5);

		_meshData.Indices32.push_back(i*6+5);
		_meshData.Indices32.push_back(i*6+4);
		_meshData.Indices32.push_back(i*6+2);

		_meshData.Indices32.push_back(i*6+3);
		_meshData.Indices32.push_back(i*6+1);
		_meshData.Indices32.push_back(i*6+4);
	}
}

GeometryGenerator::Vertex GeometryGenerator::MidPoint(const Vertex& _v0, const Vertex& _v1)
{
    XMVECTOR p0 = XMLoadFloat3(&_v0.Position);
    XMVECTOR p1 = XMLoadFloat3(&_v1.Position);

    XMVECTOR n0 = XMLoadFloat3(&_v0.Normal);
    XMVECTOR n1 = XMLoadFloat3(&_v1.Normal);

    XMVECTOR tan0 = XMLoadFloat3(&_v0.TangentU);
    XMVECTOR tan1 = XMLoadFloat3(&_v1.TangentU);

    XMVECTOR tex0 = XMLoadFloat2(&_v0.TexC);
    XMVECTOR tex1 = XMLoadFloat2(&_v1.TexC);

    // Pos, Norm, tangU, TexC �� ����� ���� �־��ش�.
    XMVECTOR pos = 0.5f*(p0 + p1);
    XMVECTOR normal = XMVector3Normalize(0.5f*(n0 + n1));
    XMVECTOR tangent = XMVector3Normalize(0.5f*(tan0+tan1));
    XMVECTOR tex = 0.5f*(tex0 + tex1);

    Vertex v;
    XMStoreFloat3(&v.Position, pos);
    XMStoreFloat3(&v.Normal, normal);
    XMStoreFloat3(&v.TangentU, tangent);
    XMStoreFloat2(&v.TexC, tex);

    return v;
}

GeometryGenerator::MeshData GeometryGenerator::CreateGeosphere(float _radius, uint32 _numSubdivisions)
{
    MeshData meshData;

	// �ִ� �κ� ������ 6�� ���Ϸ� �Ѵ�.
    _numSubdivisions = std::min<uint32>(_numSubdivisions, 6u);

	// �ϴ��� ��ü�� ���� 20��ü�� �ٻ��Ѵ�.

	// 20��ü�� ���ϴµ� ���̴�, �̹� ������ ���̴�.
	const float X = 0.525731f; 
	const float Z = 0.850651f;

	// �� 20��ü�� ���� 12����
	XMFLOAT3 pos[12] = 
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),  
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),    
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X), 
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),    
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f), 
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	// �ε����� 60���� ���´�
    uint32 k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,    
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,    
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0, 
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7 
	};

	// �ϴ� �����Ϳ� �ְ�
    meshData.Vertices.resize(12);
    meshData.Indices32.assign(&k[0], &k[60]);

	for(uint32 i = 0; i < 12; ++i)
		meshData.Vertices[i].Position = pos[i];

	for(uint32 i = 0; i < _numSubdivisions; ++i)
		Subdivide(meshData);

	// �� ������ 20��ü�� �����ϵ��� �翵�ϰ�, �����ϸ� �Ѵ�.
	for(uint32 i = 0; i < meshData.Vertices.size(); ++i)
	{
		// �ϴ� ���� ���̷� �����.
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));

		// �������� �����ش�.
		XMVECTOR p = _radius * n;

		XMStoreFloat3(&meshData.Vertices[i].Position, p);
		XMStoreFloat3(&meshData.Vertices[i].Normal, n);

		// TexCoords�� ���� ��ǥ�迡�� ���Ѵ�.
		// �Ǹ� ���ϰ� 
		float phi = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);

		// ������ �����Ѵ�. [0, 2pi].
		if (phi < 0.0f)
			phi += XM_2PI;

		// ��Ÿ�� ���Ѵ�.
		float theta = acosf(meshData.Vertices[i].Position.y / _radius);

		meshData.Vertices[i].TexC.x = phi / XM_2PI;
		meshData.Vertices[i].TexC.y = theta / XM_PI;

		// �ǿ� ���ؼ� �κ� ���Լ��� �����ش�.
		meshData.Vertices[i].TangentU.x = -_radius * sinf(theta) * sinf(phi);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = +_radius * sinf(theta) * cosf(phi);

		// �׸��� ���� ������ �־��ش�.
		XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount)
{
    MeshData meshData;

	// �ϴ� stack ������ �����.

	// ���ú� ���̸� ���ϰ�
	float stackHeight = _height / _stackCount;

	// ���Ѳ� �Ʒ���ħ�� ���̿� ���ð����� �̿��ؼ�, ������ �������� ���Ѵ�.
	float radiusStep = (_topRadius - _bottomRadius) / _stackCount;

	uint32 ringCount = _stackCount+1;

	// �ٴ� ring���� �ö󰡸鼭 stack�� ����� ����.
	for(uint32 i = 0; i < ringCount; ++i)
	{
		float y = -0.5f*_height + i*stackHeight;
		float r = _bottomRadius + i*radiusStep;

		// �ϴ� ring�� ������ ���Ѵ�.
		float dTheta = 2.0f*XM_PI/_sliceCount;
		for(uint32 j = 0; j <= _sliceCount; ++j)
		{
			Vertex vertex;

			float c = cosf(j*dTheta);
			float s = sinf(j*dTheta);

			vertex.Position = XMFLOAT3(r*c, y, r*s);

			vertex.TexC.x = (float)j/_sliceCount;
			vertex.TexC.y = 1.0f - (float)i/_stackCount;

			// Cylinder can be parameterized as follows, where we introduce v
			// parameter that goes in the same direction as the v tex-coord
			// so that the bitangent goes in the same direction as the v tex-coord.
			//   Let r0 be the bottom radius and let r1 be the top radius.
			//   y(v) = h - hv for v in [0,1].
			//   r(v) = r1 + (r0-r1)v
			//
			//   x(t, v) = r(v)*cos(t)
			//   y(t, v) = h - hv
			//   z(t, v) = r(v)*sin(t)
			// 
			//  dx/dt = -r(v)*sin(t)
			//  dy/dt = 0
			//  dz/dt = +r(v)*cos(t)
			//
			//  dx/dv = (r0-r1)*cos(t)
			//  dy/dv = -h
			//  dz/dv = (r0-r1)*sin(t)

			// TanU �� ��Ÿ�� ���ؼ� �κе��Լ��� ���ϸ� �ȴ�.
			// ũ��� ���� �����̰�
			vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

			// Normal�� �ణ �����ϴ�.
			// TanU�� ring�� ���󰡴� �����̴ϱ�, y ������ 0�ε�
			float dr = _bottomRadius-_topRadius;
			// BiTan �׷��ϱ� TanV�� ���Ѳ���, �Ʒ� �Ѳ��� ���̰�����, ��¦ ���� �ȴ�.
			// �װ��� ������, �� ���̿�, ��ü ���̸� ������ ��Ÿ�� ���̴�. 
			XMFLOAT3 bitangent(dr*c, -_height, dr*s);

			// �̰� �װ� TBN ���ñ�� ����ؼ�
			XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			// Normal�� ���ϰ� �ȴ�.
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);

			meshData.Vertices.push_back(vertex);
		}
	}

	// 1�� �����ִ� ������, ring ���� ù��° ����, �ι�° ����
	// �ߺ��� �Ǵµ�, TexCoords �� �޶�� ����� �׷����� �����̴�.
	uint32 ringVertexCount = _sliceCount+1;

	// �� ���ÿ� ���ؼ� �ε����� �����Ѵ�.
	for(uint32 i = 0; i < _stackCount; ++i)
	{
		for(uint32 j = 0; j < _sliceCount; ++j)
		{
			meshData.Indices32.push_back(i*ringVertexCount + j);
			meshData.Indices32.push_back((i+1)*ringVertexCount + j);
			meshData.Indices32.push_back((i+1)*ringVertexCount + j+1);

			meshData.Indices32.push_back(i*ringVertexCount + j);
			meshData.Indices32.push_back((i+1)*ringVertexCount + j+1);
			meshData.Indices32.push_back(i*ringVertexCount + j+1);
		}
	}

	// ���� �Ѳ��� �ٴ��� �����ش�.
	BuildCylinderTopCap(_bottomRadius, _topRadius, _height, _sliceCount, _stackCount, meshData);
	BuildCylinderBottomCap(_bottomRadius, _topRadius, _height, _sliceCount, _stackCount, meshData);

    return meshData;
}

void GeometryGenerator::BuildCylinderTopCap(float _bottomRadius, float _topRadius, float _height,
											uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData)
{
	uint32 baseIndex = (uint32)_meshData.Vertices.size();

	float y = 0.5f*_height;
	float dTheta = 2.0f*XM_PI/_sliceCount;

	// ����� ring�� ������ �����Ѵ�, Normal�� TexCoords�� �ٸ��� ������ ���� �־���� �Ѵ�.
	for(uint32 i = 0; i <= _sliceCount; ++i)
	{
		float x = _topRadius*cosf(i*dTheta);
		float z = _topRadius*sinf(i*dTheta);

		// �Ѳ� ���ؽ��� x, z ��ǥ�� height�� �����ְ�, 0.5f�� �����ִ°�
		// �� �Ѳ��� ����ؼ� TexCoords�� ������شٴµ�...
		// ���̰� ����, �Ѳ��� Ŀ�� ������ ����̸�, TexCoords�� ���� ũ�� �ؼ�,
		// �ý��ĸ� ���� ��߶���ϰ�
		// ���̰� ����, �Ѳ��� �۾Ƽ� ������ ����̸�, TexCoords ���� �۾�����,
		// �Ѳ��� �׷����� �ý��� �κ��� ���� ���̴�.
		float u = x/_height + 0.5f;
		float v = z/_height + 0.5f;

		_meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
	}

	// �Ѳ��� ��� ���� �߰��Ѵ�.
	_meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// �Ѳ��� �ε����� �����Ѵ�.
	uint32 centerIndex = (uint32)_meshData.Vertices.size()-1;
	// ��� ���� �������� �ﰢ���� �׸���.
	for(uint32 i = 0; i < _sliceCount; ++i)
	{
		_meshData.Indices32.push_back(centerIndex);
		_meshData.Indices32.push_back(baseIndex + i+1);
		_meshData.Indices32.push_back(baseIndex + i);
	}
}

void GeometryGenerator::BuildCylinderBottomCap(float _bottomRadius, float _topRadius, float _height,
											   uint32 _sliceCount, uint32 _stackCount, MeshData& _meshData)
{
	uint32 baseIndex = (uint32)_meshData.Vertices.size();
	float y = -0.5f*_height;

	// ��ħ�� ring�� ������ �����Ѵ�, Normal�� TexCoords�� �ٸ��� ������ ���� �־���� �Ѵ�.
	float dTheta = 2.0f*XM_PI/_sliceCount;
	for(uint32 i = 0; i <= _sliceCount; ++i)
	{
		float x = _bottomRadius*cosf(i*dTheta);
		float z = _bottomRadius*sinf(i*dTheta);

		// ���̰� ����, �ٴ��� Ŀ�� ������ ����̸�, TexCoords�� ���� ũ�� �ؼ�,
		// �ý��ĸ� ���� ��߶���ϰ�
		// ���̰� ����, �ٴ��� �۾Ƽ� ������ ����̸�, TexCoords ���� �۾�����,
		// �ٴڿ� �׷����� �ý��� �κ��� ���� ���̴�.
		float u = x/_height + 0.5f;
		float v = z/_height + 0.5f;

		_meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
	}

	// ��ħ�� ��� ���� �߰��Ѵ�.
	_meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// ��ħ�� �ε����� �����Ѵ�.
	uint32 centerIndex = (uint32)_meshData.Vertices.size()-1;
	// ��� ���� �������� �ﰢ���� �׸���.
	for(uint32 i = 0; i < _sliceCount; ++i)
	{
		_meshData.Indices32.push_back(centerIndex);
		_meshData.Indices32.push_back(baseIndex + i);
		_meshData.Indices32.push_back(baseIndex + i+1);
	}
}

GeometryGenerator::MeshData GeometryGenerator::CreateGrid(float _width, float _depth, uint32 _m, uint32 _n)
{
    MeshData meshData;

	// ��������
	uint32 vertexCount = _m*_n;
	// �� ������ ���Ѵ�.
	// �ﰢ�� 2���� ��������ϱ� 2�� ���Ѵ�.
	uint32 faceCount   = (_m-1)*(_n-1)*2;

	//  �߰��� ���Ѵ�.
	float halfWidth = 0.5f*_width;
	float halfDepth = 0.5f*_depth;

	// �κ��� ũ�⸦ ���Ѵ�.
	// n�� x ��, m�� z ���̴�.
	float dx = _width / (_n-1);
	float dz = _depth / (_m-1);
	// TexCoords�� �κ� ũ�⸦ ���Ѵ�.
	float du = 1.0f / (_n-1);
	float dv = 1.0f / (_m-1);

	meshData.Vertices.resize(vertexCount);
	for(uint32 i = 0; i < _m; ++i)
	{
		// z�� ����(+) ����
		float z = halfDepth - i*dz;
		for(uint32 j = 0; j < _n; ++j)
		{
			// x�� ����(-) ����
			float x = -halfWidth + j*dx;

			meshData.Vertices[i*_n+j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i*_n+j].Normal   = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i*_n+j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// TexCoords�� ���ļ� �����Ѵ�.
			meshData.Vertices[i*_n+j].TexC.x = j*du;
			meshData.Vertices[i*_n+j].TexC.y = i*dv;
		}
	}
 
    // �ε����� �����Ѵ�.
	meshData.Indices32.resize(faceCount*3); 

	// ĭ���� ���ƴٴϸ鼭 index�� ����Ѵ�.
	uint32 k = 0;
	for(uint32 i = 0; i < _m-1; ++i)
	{
		for(uint32 j = 0; j < _n-1; ++j)
		{
			// z0, z0, z1
			meshData.Indices32[k]   = i*_n+j;
			meshData.Indices32[k+1] = i*_n+j+1;
			meshData.Indices32[k+2] = (i+1)*_n+j;
			// z1, z0, z1
			meshData.Indices32[k+3] = (i+1)*_n+j;
			meshData.Indices32[k+4] = i*_n+j+1;
			meshData.Indices32[k+5] = (i+1)*_n+j+1;

			k += 6; // ���� ĭ
		}
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreatePatchQuad(float _width, float _depth, uint32 _m, uint32 _n)
{
	MeshData meshData;

	// ��������
	uint32 vertexCount = _m * _n;
	// �� ������ ���Ѵ�.
	// �簢�� �ϳ��� �ε��� 4���� ���� ���̴�.
	uint32 faceCount = (_m - 1) * (_n - 1);

	//  �߰��� ���Ѵ�.
	float halfWidth = 0.5f * _width;
	float halfDepth = 0.5f * _depth;

	// �κ��� ũ�⸦ ���Ѵ�.
	// n�� x ��, m�� z ���̴�.
	float dx = _width / (_n - 1);
	float dz = _depth / (_m - 1);
	// TexCoords�� �κ� ũ�⸦ ���Ѵ�.
	float du = 1.0f / (_n - 1);
	float dv = 1.0f / (_m - 1);

	meshData.Vertices.resize(vertexCount);
	for (uint32 i = 0; i < _m; ++i)
	{
		// z�� ����(+) ����
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < _n; ++j)
		{
			// x�� ����(-) ����
			float x = -halfWidth + j * dx;

			meshData.Vertices[i * _n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i * _n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i * _n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// TexCoords�� ���ļ� �����Ѵ�.
			meshData.Vertices[i * _n + j].TexC.x = j * du;
			meshData.Vertices[i * _n + j].TexC.y = i * dv;
		}
	}

	// �ε����� �����Ѵ�.
	meshData.Indices32.resize(faceCount * 4);

	// ĭ���� ���ƴٴϸ鼭 index�� ����Ѵ�.
	uint32 k = 0;
	for (uint32 i = 0; i < _m - 1; ++i)
	{
		for (uint32 j = 0; j < _n - 1; ++j)
		{
			// �׳� ���� 4���� �׾� ������.
			meshData.Indices32[k] = i * _n + j;
			meshData.Indices32[k + 1] = i * _n + j + 1;
			meshData.Indices32[k + 2] = (i + 1) * _n + j;
			meshData.Indices32[k + 3] = (i + 1) * _n + j + 1;

			k += 4; // ���� ĭ
		}
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float _x, float _y, float _w, float _h, float _depth)
{
    MeshData meshData;

	meshData.Vertices.resize(4);
	meshData.Indices32.resize(6);

	// ȭ���� �׸��� ��ǥ����
	// NDC(Normalize Device Coordinate)�� ��Ÿ����.
	meshData.Vertices[0] = Vertex(
        _x, _y , _depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 1.0f);

	meshData.Vertices[1] = Vertex(
		_x, _y + _h, _depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		0.0f, 0.0f);

	meshData.Vertices[2] = Vertex(
		_x+_w, _y + _h, _depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 0.0f);

	meshData.Vertices[3] = Vertex(
		_x+_w, _y, _depth,
		0.0f, 0.0f, -1.0f,
		1.0f, 0.0f, 0.0f,
		1.0f, 1.0f);

	if (_w * _h < 0.f)
	{
		meshData.Indices32[0] = 0;
		meshData.Indices32[1] = 2;
		meshData.Indices32[2] = 1;

		meshData.Indices32[3] = 0;
		meshData.Indices32[4] = 3;
		meshData.Indices32[5] = 2;
	}
	else
	{
		meshData.Indices32[0] = 0;
		meshData.Indices32[1] = 1;
		meshData.Indices32[2] = 2;

		meshData.Indices32[3] = 0;
		meshData.Indices32[4] = 2;
		meshData.Indices32[5] = 3;
	}

    return meshData;
}
