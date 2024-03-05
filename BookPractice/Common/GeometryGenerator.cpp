//***************************************************************************************
// GeometryGenerator.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "GeometryGenerator.h"
#include <algorithm>

using namespace DirectX;

GeometryGenerator::MeshData GeometryGenerator::CreateBox(float _width, float _height, float _depth, uint32 _numSubdivisions)
{
    MeshData meshData;

	// 일단 모체 박스를 만든다.

	Vertex v[24];

	float w2 = 0.5f*_width;
	float h2 = 0.5f*_height;
	float d2 = 0.5f*_depth;
    
	// 앞면 (pos, norm, tangent, UVc 순)
	v[0] = Vertex(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[1] = Vertex(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[2] = Vertex(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[3] = Vertex(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// 뒷면 (pos, norm, tangent, UVc 순)
	v[4] = Vertex(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[5] = Vertex(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[6] = Vertex(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[7] = Vertex(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// 윗면 (pos, norm, tangent, UVc 순)
	v[8]  = Vertex(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[9]  = Vertex(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vertex(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vertex(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// 밑면 (pos, norm, tangent, UVc 순)
	v[12] = Vertex(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vertex(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vertex(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vertex(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	// 왼면 (pos, norm, tangent, UVc 순)
	v[16] = Vertex(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[17] = Vertex(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[18] = Vertex(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[19] = Vertex(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// 오른면 (pos, norm, tangent, UVc 순)
	v[20] = Vertex(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[21] = Vertex(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[22] = Vertex(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);
	v[23] = Vertex(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);

	meshData.Vertices.assign(&v[0], &v[24]);
 
	// Index 버퍼를 만든다.

	uint32 i[36];

	// 전면 인덱스
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// 후면 인덱스
	i[6] = 4; i[7]  = 5; i[8]  = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// 윗면 인덱스
	i[12] = 8; i[13] =  9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// 밑면 인덱스
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// 왼면 인덱스
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// 오른면 인덱스
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	meshData.Indices32.assign(&i[0], &i[36]);

    // 일단 분할 수를 6 이하로 제한을 두는 느낌이다.
    _numSubdivisions = std::min<uint32>(_numSubdivisions, 6u);

	// 분할 수 만큼 분할 함수를 호출해준다.
    for(uint32 i = 0; i < _numSubdivisions; ++i)
        Subdivide(meshData);

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateSphere(float _radius, uint32 _sliceCount, uint32 _stackCount)
{
    MeshData meshData;

	// Top Poles(북극)에서 있고, 세로줄 (Stack)을 따라 내려가는 점을 계산한다.
	// Compute the vertices stating at the top pole and moving down the stacks.

	// Poles: 직사각형 텍스쳐를 구에 매핑할 때, 
	// 극(poles)에 할당된 고유한 TexCoords가 없기 때문에 왜곡이 발생합니다.
	Vertex topVertex(0.0f, +_radius, 0.0f, 0.0f, +1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	Vertex bottomVertex(0.0f, -_radius, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);

	// 일단 Top Pole Vertex를 넣어주고
	meshData.Vertices.push_back( topVertex );

	// 고각 오프셋은 PI를 세로줄 개수로 나누고
	float thetaStep   = XM_PI/_stackCount;
	// 편각 오프셋은 2PI를 가로줄 개수로 나눈다.
	float phiStep = 2.0f*XM_PI/_sliceCount;

	// Stack Ring 마다 내려오면서 Vertex를 계산한다. (pole은 계산하지 않는다.)
	// 구는 가로로 잘라서 생기는 단면 원이 ring이다.
	for(uint32 i = 1; i <= _stackCount-1; ++i)
	{
		float theta = i*thetaStep;

		// ring 의 점들
        for(uint32 j = 0; j <= _sliceCount; ++j)
		{
			// 현재 편각
			float phi = j*phiStep;

			Vertex v;

			// 구면 좌표계를 사용해서 구한다.
			v.Position.x = _radius*sinf(theta)*cosf(phi);
			v.Position.y = _radius*cosf(theta);
			v.Position.z = _radius*sinf(theta)*sinf(phi);

			// Phi에 대해서 부분 도함수를 걸면 TangentU를 구할 수 있다.
			v.TangentU.x = -_radius*sinf(theta)*sinf(phi); // consf(Phi)에 대해서만 미분
			v.TangentU.y = 0.0f; // 요건 상수니까 없어지고
			v.TangentU.z = +_radius*sinf(theta)*cosf(phi); // sinf(Phi)에 대해서만 미분

			// TangentU 라고 적힌건... 아마도 Tangent Plane에서 UV 좌표계에서 U축 Vector를 말하는 것 같다.
			XMVECTOR T = XMLoadFloat3(&v.TangentU);
			XMStoreFloat3(&v.TangentU, XMVector3Normalize(T));

			// === 모르는 점 ===
			// 그러면 왜 TangentV는 없을까?
			// Plane의 법선은 Normal과 같을거고...외적으로 구하나?

			// 일단 Origin에 만드는 거니까, 점이 찍히는 좌표 자체가 normal과 같을 것이다.
			XMVECTOR p = XMLoadFloat3(&v.Position);
			XMStoreFloat3(&v.Normal, XMVector3Normalize(p));

			// 텍스쳐 좌표도 그냥, 편각과 고각으로 비율을 나눠서 가지는 느낌으로 간다.
			v.TexC.x = phi / XM_2PI;
			v.TexC.y = theta / XM_PI;

			// 그리고 넣는다.
			meshData.Vertices.push_back( v );
		}
	}

	// 그리고 남극 점을 넣는다.
	meshData.Vertices.push_back( bottomVertex );

	//
	// 북극점을 처음에 넣고, 맨위의 스택과
	// 연결한다.

    for(uint32 i = 1; i <= _sliceCount; ++i)
	{
		meshData.Indices32.push_back(0); // 북극점을 매번 넣어서
		meshData.Indices32.push_back(i+1); // 바로아래 ring과
		meshData.Indices32.push_back(i); // 연결하게 된다.
	}

	// 이제 그 아래 차례대로 내려가면서 stack과 그 아래 stack을 
	// 인덱스로 연결하기 시작한다.	

	// (이제 pole 건너 뛰고) 
	// 위에 ring에서 2개, 아래에서 1개 삼각형 하나
	// 위에 ring에서 1개, 아래에서 2개 삼각형 하나로
	// 사각형을 만들도록 하는 index를 넣는다.
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
	// 이제 맨 아래 스택을 계산한다. 남극 남이랑 맨 아래 링이랑 연결한다.
	//

	// 일단 남극 버텍스 번호는 이렇고
	uint32 southPoleIndex = (uint32)meshData.Vertices.size()-1;

	// 거기서 링의 버텍스 개수만큼 빼면, 맨 아래 링의 첫번째 버택스 번호가 나올 것이다.
	baseIndex = southPoleIndex - ringVertexCount;
	
	// 그걸로 이제 돌아가면서 인덱스를 지정해주면 된다.
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
	// 일단 카피를 해놓는다.
	MeshData inputCopy = _meshData;

	// 그리고 원본을 0으로 없애버린다.
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

	// 일단 모체 도형의 삼각형 개수는 인덱스 3개가 모여서 만드니, 그걸로 구한다.
	uint32 numTris = (uint32)inputCopy.Indices32.size()/3;
	for(uint32 i = 0; i < numTris; ++i)
	{
		// 삼각형 개수 만큼 루프를 돌면서
		Vertex v0 = inputCopy.Vertices[ inputCopy.Indices32[i*3+0] ];
		Vertex v1 = inputCopy.Vertices[ inputCopy.Indices32[i*3+1] ];
		Vertex v2 = inputCopy.Vertices[ inputCopy.Indices32[i*3+2] ];

		//
		// 인덱스가 가리키는 버텍스를 구하고
		// 그걸로 하위 삼각형을 중간점을 구함으로써 만든다.
		//

        Vertex m0 = MidPoint(v0, v1);
        Vertex m1 = MidPoint(v1, v2);
        Vertex m2 = MidPoint(v0, v2);

		//
		// 새로운 도형을 만들어서 넣어준다.
		// 일단 모체 넣고
		// 그 다음, 미드 포인트를 넣고
		//

		_meshData.Vertices.push_back(v0); // 0
		_meshData.Vertices.push_back(v1); // 1
		_meshData.Vertices.push_back(v2); // 2
		_meshData.Vertices.push_back(m0); // 3
		_meshData.Vertices.push_back(m1); // 4
		_meshData.Vertices.push_back(m2); // 5
 
		// 그에 맞는 인덱스를 넣어준다.
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

    // Pos, Norm, tangU, TexC 다 평균을 내서 넣어준다.
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

	// 최대 부분 개수를 6개 이하로 한다.
    _numSubdivisions = std::min<uint32>(_numSubdivisions, 6u);

	// 일단은 모체를 원을 20면체로 근사한다.

	// 20면체를 구하는데 쓰이는, 이미 정해진 값이다.
	const float X = 0.525731f; 
	const float Z = 0.850651f;

	// 정 20면체는 점이 12개다
	XMFLOAT3 pos[12] = 
	{
		XMFLOAT3(-X, 0.0f, Z),  XMFLOAT3(X, 0.0f, Z),  
		XMFLOAT3(-X, 0.0f, -Z), XMFLOAT3(X, 0.0f, -Z),    
		XMFLOAT3(0.0f, Z, X),   XMFLOAT3(0.0f, Z, -X), 
		XMFLOAT3(0.0f, -Z, X),  XMFLOAT3(0.0f, -Z, -X),    
		XMFLOAT3(Z, X, 0.0f),   XMFLOAT3(-Z, X, 0.0f), 
		XMFLOAT3(Z, -X, 0.0f),  XMFLOAT3(-Z, -X, 0.0f)
	};

	// 인덱스는 60개가 나온다
    uint32 k[60] =
	{
		1,4,0,  4,9,0,  4,5,9,  8,5,4,  1,8,4,    
		1,10,8, 10,3,8, 8,3,5,  3,2,5,  3,7,2,    
		3,10,7, 10,6,7, 6,11,7, 6,0,11, 6,1,0, 
		10,1,6, 11,0,9, 2,11,9, 5,2,9,  11,2,7 
	};

	// 일단 데이터에 넣고
    meshData.Vertices.resize(12);
    meshData.Indices32.assign(&k[0], &k[60]);

	for(uint32 i = 0; i < 12; ++i)
		meshData.Vertices[i].Position = pos[i];

	for(uint32 i = 0; i < _numSubdivisions; ++i)
		Subdivide(meshData);

	// 각 점들을 20면체에 외접하도록 사영하고, 스케일링 한다.
	for(uint32 i = 0; i < meshData.Vertices.size(); ++i)
	{
		// 일단 단위 길이로 만들고.
		XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&meshData.Vertices[i].Position));

		// 반지름을 곱해준다.
		XMVECTOR p = _radius * n;

		XMStoreFloat3(&meshData.Vertices[i].Position, p);
		XMStoreFloat3(&meshData.Vertices[i].Normal, n);

		// TexCoords는 구면 좌표계에서 구한다.
		// 피를 구하고 
		float phi = atan2f(meshData.Vertices[i].Position.z, meshData.Vertices[i].Position.x);

		// 구간을 제한한다. [0, 2pi].
		if (phi < 0.0f)
			phi += XM_2PI;

		// 세타를 구한다.
		float theta = acosf(meshData.Vertices[i].Position.y / _radius);

		meshData.Vertices[i].TexC.x = phi / XM_2PI;
		meshData.Vertices[i].TexC.y = theta / XM_PI;

		// 피에 대해서 부분 도함수를 구해준다.
		meshData.Vertices[i].TangentU.x = -_radius * sinf(theta) * sinf(phi);
		meshData.Vertices[i].TangentU.y = 0.0f;
		meshData.Vertices[i].TangentU.z = +_radius * sinf(theta) * cosf(phi);

		// 그리고 점의 정보를 넣어준다.
		XMVECTOR T = XMLoadFloat3(&meshData.Vertices[i].TangentU);
		XMStoreFloat3(&meshData.Vertices[i].TangentU, XMVector3Normalize(T));
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateCylinder(float _bottomRadius, float _topRadius, float _height, uint32 _sliceCount, uint32 _stackCount)
{
    MeshData meshData;

	// 일단 stack 정보를 만든다.

	// 스택별 높이를 구하고
	float stackHeight = _height / _stackCount;

	// 위뚜껑 아래받침의 차이와 스택개수를 이용해서, 반지름 증가량을 구한다.
	float radiusStep = (_topRadius - _bottomRadius) / _stackCount;

	uint32 ringCount = _stackCount+1;

	// 바닥 ring에서 올라가면서 stack을 만들어 간다.
	for(uint32 i = 0; i < ringCount; ++i)
	{
		float y = -0.5f*_height + i*stackHeight;
		float r = _bottomRadius + i*radiusStep;

		// 일단 ring의 점들을 구한다.
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

			// TanU 는 세타에 대해서 부분도함수를 구하면 된다.
			// 크기는 단위 벡터이고
			vertex.TangentU = XMFLOAT3(-s, 0.0f, c);

			// Normal은 약간 복잡하다.
			// TanU는 ring을 따라가는 느낌이니깐, y 성분이 0인데
			float dr = _bottomRadius-_topRadius;
			// BiTan 그러니깐 TanV는 위뚜껑과, 아래 뚜껑에 차이가나면, 살짝 기울게 된다.
			// 그것을 적절히, 그 차이와, 전체 높이를 비율로 나타낸 것이다. 
			XMFLOAT3 bitangent(dr*c, -_height, dr*s);

			// 이게 그걸 TBN 뭐시기로 계산해서
			XMVECTOR T = XMLoadFloat3(&vertex.TangentU);
			XMVECTOR B = XMLoadFloat3(&bitangent);
			// Normal을 구하게 된다.
			XMVECTOR N = XMVector3Normalize(XMVector3Cross(T, B));
			XMStoreFloat3(&vertex.Normal, N);

			meshData.Vertices.push_back(vertex);
		}
	}

	// 1을 더해주는 이유는, ring 마다 첫번째 점과, 두번째 점이
	// 중복이 되는데, TexCoords 는 달라야 제대로 그려지기 때문이다.
	uint32 ringVertexCount = _sliceCount+1;

	// 각 스택에 대해서 인덱스를 설정한다.
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

	// 이제 뚜껑과 바닥을 덮어준다.
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

	// 꼭대기 ring의 점들을 복사한다, Normal과 TexCoords가 다르기 때문에 따로 넣어줘야 한다.
	for(uint32 i = 0; i <= _sliceCount; ++i)
	{
		float x = _topRadius*cosf(i*dTheta);
		float z = _topRadius*sinf(i*dTheta);

		// 뚜껑 버텍스의 x, z 좌표를 height로 나눠주고, 0.5f를 더해주는게
		// 왜 뚜껑에 비례해서 TexCoords를 만들어준다는데...
		// 높이가 낮고, 뚜껑이 커서 납작한 모양이면, TexCoords의 값을 크게 해서,
		// 택스쳐를 많이 펴발라야하고
		// 높이가 높고, 뚜껑이 작아서 길쭉한 모양이면, TexCoords 값이 작아져서,
		// 뚜껑에 그려지는 택스쳐 부분이 작을 것이다.
		float u = x/_height + 0.5f;
		float v = z/_height + 0.5f;

		_meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
	}

	// 뚜껑의 가운데 점을 추가한다.
	_meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// 뚜껑의 인덱스를 정의한다.
	uint32 centerIndex = (uint32)_meshData.Vertices.size()-1;
	// 가운데 점을 기준으로 삼각형을 그린다.
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

	// 받침의 ring의 점들을 복사한다, Normal과 TexCoords가 다르기 때문에 따로 넣어줘야 한다.
	float dTheta = 2.0f*XM_PI/_sliceCount;
	for(uint32 i = 0; i <= _sliceCount; ++i)
	{
		float x = _bottomRadius*cosf(i*dTheta);
		float z = _bottomRadius*sinf(i*dTheta);

		// 높이가 낮고, 바닥이 커서 납작한 모양이면, TexCoords의 값을 크게 해서,
		// 택스쳐를 많이 펴발라야하고
		// 높이가 높고, 바닥이 작아서 길쭉한 모양이면, TexCoords 값이 작아져서,
		// 바닥에 그려지는 택스쳐 부분이 작을 것이다.
		float u = x/_height + 0.5f;
		float v = z/_height + 0.5f;

		_meshData.Vertices.push_back( Vertex(x, y, z, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, u, v) );
	}

	// 받침의 가운데 점을 추가한다.
	_meshData.Vertices.push_back( Vertex(0.0f, y, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.5f, 0.5f) );

	// 받침의 인덱스를 정의한다.
	uint32 centerIndex = (uint32)_meshData.Vertices.size()-1;
	// 가운데 점을 기준으로 삼각형을 그린다.
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

	// 점개수와
	uint32 vertexCount = _m*_n;
	// 면 개수를 구한다.
	// 삼각형 2개로 만들어지니까 2를 곱한다.
	uint32 faceCount   = (_m-1)*(_n-1)*2;

	//  중간을 구한다.
	float halfWidth = 0.5f*_width;
	float halfDepth = 0.5f*_depth;

	// 부분의 크기를 구한다.
	// n이 x 축, m이 z 축이다.
	float dx = _width / (_n-1);
	float dz = _depth / (_m-1);
	// TexCoords의 부분 크기를 구한다.
	float du = 1.0f / (_n-1);
	float dv = 1.0f / (_m-1);

	meshData.Vertices.resize(vertexCount);
	for(uint32 i = 0; i < _m; ++i)
	{
		// z는 안쪽(+) 부터
		float z = halfDepth - i*dz;
		for(uint32 j = 0; j < _n; ++j)
		{
			// x는 왼쪽(-) 부터
			float x = -halfWidth + j*dx;

			meshData.Vertices[i*_n+j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i*_n+j].Normal   = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i*_n+j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// TexCoords를 펼쳐서 삽입한다.
			meshData.Vertices[i*_n+j].TexC.x = j*du;
			meshData.Vertices[i*_n+j].TexC.y = i*dv;
		}
	}
 
    // 인덱스를 설정한다.
	meshData.Indices32.resize(faceCount*3); 

	// 칸마다 돌아다니면서 index를 계산한다.
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

			k += 6; // 다음 칸
		}
	}

    return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreatePatchQuad(float _width, float _depth, uint32 _m, uint32 _n)
{
	MeshData meshData;

	// 점개수와
	uint32 vertexCount = _m * _n;
	// 면 개수를 구한다.
	// 사각형 하나에 인덱스 4개씩 보낼 것이다.
	uint32 faceCount = (_m - 1) * (_n - 1);

	//  중간을 구한다.
	float halfWidth = 0.5f * _width;
	float halfDepth = 0.5f * _depth;

	// 부분의 크기를 구한다.
	// n이 x 축, m이 z 축이다.
	float dx = _width / (_n - 1);
	float dz = _depth / (_m - 1);
	// TexCoords의 부분 크기를 구한다.
	float du = 1.0f / (_n - 1);
	float dv = 1.0f / (_m - 1);

	meshData.Vertices.resize(vertexCount);
	for (uint32 i = 0; i < _m; ++i)
	{
		// z는 안쪽(+) 부터
		float z = halfDepth - i * dz;
		for (uint32 j = 0; j < _n; ++j)
		{
			// x는 왼쪽(-) 부터
			float x = -halfWidth + j * dx;

			meshData.Vertices[i * _n + j].Position = XMFLOAT3(x, 0.0f, z);
			meshData.Vertices[i * _n + j].Normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			meshData.Vertices[i * _n + j].TangentU = XMFLOAT3(1.0f, 0.0f, 0.0f);

			// TexCoords를 펼쳐서 삽입한다.
			meshData.Vertices[i * _n + j].TexC.x = j * du;
			meshData.Vertices[i * _n + j].TexC.y = i * dv;
		}
	}

	// 인덱스를 설정한다.
	meshData.Indices32.resize(faceCount * 4);

	// 칸마다 돌아다니면서 index를 계산한다.
	uint32 k = 0;
	for (uint32 i = 0; i < _m - 1; ++i)
	{
		for (uint32 j = 0; j < _n - 1; ++j)
		{
			// 그냥 냅다 4개를 그어 버린다.
			meshData.Indices32[k] = i * _n + j;
			meshData.Indices32[k + 1] = i * _n + j + 1;
			meshData.Indices32[k + 2] = (i + 1) * _n + j;
			meshData.Indices32[k + 3] = (i + 1) * _n + j + 1;

			k += 4; // 다음 칸
		}
	}

	return meshData;
}

GeometryGenerator::MeshData GeometryGenerator::CreateQuad(float _x, float _y, float _w, float _h, float _depth)
{
    MeshData meshData;

	meshData.Vertices.resize(4);
	meshData.Indices32.resize(6);

	// 화면을 그리는 좌표계인
	// NDC(Normalize Device Coordinate)로 나타낸다.
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
