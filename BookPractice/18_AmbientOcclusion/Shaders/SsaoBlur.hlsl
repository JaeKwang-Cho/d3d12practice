//=============================================================================
// SsaoBlur.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// 생성한 SSAO Map에서 bilateral edge preserving blur를 먹인다.
// 컴퓨트 쉐이더 대신에 픽셀 쉐이더를 사용하고
// 캐시로는 텍스쳐를 사용한다.
// SSAO맵은 16비트 format이므로 텍셀의 크기가 작아서 많이 담을 수 있다.
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
Texture2D gInputMap : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

static const int gBlurRadius = 5;

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
    float4 PosH : SV_Position;
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
    
    vout.TexC = gTexCoords[vid];

    // 텍스쳐 좌표를 NDC 좌표로 바꿔서 넘겨준다.
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
    
    return vout;
}

float NdcDepthToViewDepth(float z_ndc)
{
    float viewZ = gProjMat[3][2] / (z_ndc - gProjMat[2][2]);
    return viewZ;
}

float4 PS(VertexOut pin) : SV_Target
{
    // 가중치를 하나의 배열에 넣는다.
    float blurWeights[12] =
    {
        gBlurWeights[0].x, gBlurWeights[0].y, gBlurWeights[0].z, gBlurWeights[0].w,
        gBlurWeights[1].x, gBlurWeights[1].y, gBlurWeights[1].z, gBlurWeights[1].w,
        gBlurWeights[2].x, gBlurWeights[2].y, gBlurWeights[2].z, gBlurWeights[2].w,
    };
    
    // 화면의 크기만큼 오프셋을 정한다.
    float2 texOffset;
    // 가로세로 한번씩 한다.
    // 캐시는 텍스쳐를 이용하고, cb로 분기를 제어한다.
    if (gHorizontalBlur)
    {
        texOffset = float2(gInvRenderTargetSize.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0f, gInvRenderTargetSize.y);
    }
    
    // 타겟(중앙)값은 항상 총합에 포함된다.
    float4 color = blurWeights[gBlurRadius] * gInputMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0);
    float totalWeight = blurWeights[gBlurRadius];
	// 타겟의 노멀과, view에서 깊이를 구한다.
    float3 centerNormal = gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;
    float centerDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r);

    for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
		// 타겟 값은 넘어간다.
        if (i == 0)
            continue;
        
        float2 tex = pin.TexC + i * texOffset;
        // 주변 노멀과 view 깊이를 구한다.
        float3 neighborNormal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
        float neighborDepth = NdcDepthToViewDepth(
            gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r);

		//
		// 노멀이나 뎁스 차이가 심하면, 같은 경계에 있지 않다고
        // 판단해서 blur 계산에 포함시키지 않는다.
		//
	
        if (dot(neighborNormal, centerNormal) >= 0.8f &&
		    abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = blurWeights[i + gBlurRadius];

			// blur 계산에 포함한다.
            color += weight * gInputMap.SampleLevel(
                gsamPointClamp, tex, 0.0);
		
            totalWeight += weight;
        }
    }

	// 계산에 포함되지 않은 친구들은 빼서, 정규화를 시킨다.
    return color / totalWeight;
}
