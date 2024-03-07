//=============================================================================
// Ssao.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//=============================================================================

cbuffer cbSsao : register(b0)
{
    float4x4 gProjMat;
    float4x4 gInvProjMat;
    float4x4 gProjTexMat;
    float4 gOffsetVectors[14];
    
    // SsaoBlur 가중치들
    float4 gBlurWeights[3];
    
    float2 gInvRenderTargetSize;
    
    // view 공간 occlusion 속성값들
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gSurfaceEpsilon;
};

cbuffer cbRootConstants : register(b1)
{
    bool gHorizontalBlur;
}

// 수치 좌표만 cbuffer에 넣을 수 있다.
Texture2D gNormalMap : register(t0);
Texture2D gDepthMap : register(t1);
Texture2D gRandomVecMap : register(t2);

// 샘플러는 다음 친구들을 사용하는 것 같은데...
SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

static const int gSampleCount = 14;

// 화면 사각형 텍스쳐 좌표
static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};

// NDC 공간에서 ProjMat을 거꾸로 타고가서 view 공간의 점을 얻는 것이다.
VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // 일단 텍스쳐 좌표를 NDC 좌표로 바꾸고
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
 
    // NDC 좌표를 InvProjMat을 이용해서
    // view 공간에 해당하는 가장 가까운 점으로 대응 시킨다.
    float4 ph = mul(vout.PosH, gInvProjMat);
    vout.PosV = ph.xyz / ph.w;

    return vout;
}

// 무작위로 선택된 q가 p를 얼마나 가리는지 distZ(p - q)를 계산하는 함수로 계산한다.
float OcclusionFunction(float distZ)
{
	// 1. 깊이 맵에서 q의 값이 p의 값보다 크다면(뒤에 있다면) p는 가려지지 않은 것이다.
    // 2. q와 p의 값이 충분히 가깝다면 (두 깊이의 차가 입실론 보다 작다면) p는 가려지지 않은 것이다.
    //    같은 평면에 있다고 가정한다. (그냥 둘다 다른 친구에 의해 가려진다고 생각한다.)
	//
	//  Eps 부터 occlusion이 일어나고
    //  z0는 occlusion이 점점 약해지는 시작하는 거리
	//  z1는 occlusion이 일어나기에는 너무 먼 거리
	//
	//       1.0     -------------\
	//               |           |  \
	//               |           |    \
	//               |           |      \ 
	//               |           |        \
	//               |           |          \
	//               |           |            \
	//  ------|------|-----------|-------------|---------|--> zv
	//        0     Eps          z0            z1        
	//
	
    float occlusion = 0.0f;
    if (distZ > gSurfaceEpsilon)
    {
        float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
		
		// z0 에서 z1으로 선형으로 감소한다.
        occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
    }
	
    return occlusion;
}

float NdcDepthToViewDepth(float z_ndc)
{
    // z_ndc = A + B/viewZ, where gProj[2,2]=A and gProj[3,2]=B.
    // ProjMat을 계산하고, Homogenous에서 저장된 viewZ(깊이)로 
    // 한번 더 나눠주는 것이 NDC 좌표인데, 그걸 거꾸로 한 것이다.
    float viewZ = gProjMat[3][2] / (z_ndc - gProjMat[2][2]);
	// float viewZ = (z_ndc - gProj[2][2])/gProj[3][2] 
    return viewZ;
}

float4 PS(VertexOut pin) : SV_Target
{
	// p -- occlusion 수치를 정할 픽셀
	// n -- p의 노멀
	// q -- 랜덤으로 선택된 q
	// r -- q를 지나고 p를 가릴것기아 기대되는 깊이버퍼로 얻은 점.

	// view 공간에서 p의 노멀을 구하고.  
    float3 n = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);
    // p의 depth를 구한다.
    float pz = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);

	// 반직선 성질을 이용해서 view에서의 p(x, y, z)를 구한다.
    // Vertex Shader에서 InvProjMat으로 구했던 PosV와 p가 직선을 이룬다고 생각한다.
	// p = t * pin.PosV 이고, z 성분을 알고 있으니  t = p.z / pin.PosV.z으로 
    // t를 구해서 p에 대한 식을 다시 t와 pin.PosV로 나타낼 수 있다.
    float3 p = (pz / pin.PosV.z) * pin.PosV;
	
	// App에서 만들어 두었던 랜덤 백터를 [0, 1] -> [-1, +1]으로 사상한다. (텍스쳐 크기를 줄이기 이렇게 했다.)
    float3 randVec = 2.0f * gRandomVecMap.SampleLevel(gsamLinearWrap, 4.0f * pin.TexC, 0.0f).rgb - 1.0f;

    float occlusionSum = 0.0f;
	
	// p를 중심으로 n을 방향으로 만든 반구위의 점에 대해서 살펴본다..
    for (int i = 0; i < gSampleCount; ++i)
    {
		// 미리 만들어서 cb로 넘어온 오프셋 백터들을 이용해서,
        // 나름 균일하게 주변을 살펴볼 수 있다.
        // 무작위 벡터 분포 * 균일한 벡터 분포
        float3 offset = reflect(gOffsetVectors[i].xyz, randVec);
	
		// 만약 샘플링된 점이 p와 n이 이루는 평면의 반대로 방향이라면 뒤집는다. (내적)
        float flip = sign(dot(offset, n)); // 0보다 작으면 -1 반환
		
		// 이제 벡터에 프로그래머가 정한 반지름까지 곱해서, 진짜 q를 구한다.
        float3 q = p + flip * gOcclusionRadius * offset;
		
		// view 공간 위 q를 투영시킨 다음 텍스쳐 좌표로 바꾼다.
        float4 projQ = mul(float4(q, 1.0f), gProjTexMat);
        projQ /= projQ.w;

		// 미리 Projection 해서 depth를 기록해 놓은 텍스쳐에서
        // q의 xy를 이용해서 q를 지나는 반직선에서 가장 가까운 픽셀의 깊이를 구한다.
		// (q의 깊이가 아니고, 해상도 차이 때문에 오차가 발생할 수 있다. 빈 공간일 수도 있다.)
        float rz = gDepthMap.SampleLevel(gsamDepthMap, projQ.xy, 0.0f).r;
        // 그 깊이 값을 rz이라고 하고, 다시 view 공간에서의 깊이로 바꾼다.
        rz = NdcDepthToViewDepth(rz);

		// 위와 똑같이 진짜 r의 위치를 q를 지나는 반직선이라는 것을 이용해서 구한다.
        // r = (rx, ry, rz) / r = t * q
        // r.z = t * q.z -> t = r.z / q.z
        float3 r = (rz / q.z) * q;
		
		//
		// 진짜 존재하는 r이 p를 가리는지 판정한다.
		//   * dot(n, normalize(r - p))의 결과는 r이 평면 (p, n)에서 
        //     얼마나 앞으로 떨어져 있는지 나타낸다. 
        //     평면 (p, n)와 이루는 각이 클 수록(멀 수록) 값이 커지고,
        //     이루는 각이 0이 되면(r이 평면(p, n)위에 존재할 때) 
        //     값이 0ㅇ이 되어서, 판정 오류도 막을 수 있다.
		//   * 내부 함수에서는 0, Eps, FadeStart, FadeEnd 구간을 가지고
        //     적절히 occlusion weight를 설정해준다.
		// 
		
        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);

        float occlusion = dp * OcclusionFunction(distZ);

        occlusionSum += occlusion;
    }
	
    occlusionSum /= gSampleCount;
	
    float access = 1.0f - occlusionSum;

	// power를 해줘서 좀 더 확실한 occlusion 효과를 준다.
    return saturate(pow(access, 6.0f));
}
