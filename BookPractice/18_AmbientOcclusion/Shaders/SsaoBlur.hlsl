//=============================================================================
// SsaoBlur.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// ������ SSAO Map���� bilateral edge preserving blur�� ���δ�.
// ��ǻƮ ���̴� ��ſ� �ȼ� ���̴��� ����ϰ�
// ĳ�÷δ� �ؽ��ĸ� ����Ѵ�.
// SSAO���� 16��Ʈ format�̹Ƿ� �ؼ��� ũ�Ⱑ �۾Ƽ� ���� ���� �� �ִ�.
//=============================================================================
cbuffer cbSsao : register(b0)
{
    float4x4 gProjMat;
    float4x4 gInvProjMat;
    float4x4 gProjTexMat;
    float4 gOffsetVectors[14];
    
    // SsaoBlur ����ġ��
    float4 gBlurWeights[3];
    
    float2 gInvRenderTargetSize;
    
    // view ���� occlusion �Ӽ�����
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gSurfaceEpsilon;
};

cbuffer cbRootConstants : register(b1)
{
    bool gHorizontalBlur;
}

// ��ġ ��ǥ�� cbuffer�� ���� �� �ִ�.
Texture2D gNormalMap : register(t0);
Texture2D gDepthMap : register(t1);
Texture2D gInputMap : register(t2);

SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

static const int gBlurRadius = 5;

// ȭ�� �簢�� �ؽ��� ��ǥ
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

    // �ؽ��� ��ǥ�� NDC ��ǥ�� �ٲ㼭 �Ѱ��ش�.
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
    // ����ġ�� �ϳ��� �迭�� �ִ´�.
    float blurWeights[12] =
    {
        gBlurWeights[0].x, gBlurWeights[0].y, gBlurWeights[0].z, gBlurWeights[0].w,
        gBlurWeights[1].x, gBlurWeights[1].y, gBlurWeights[1].z, gBlurWeights[1].w,
        gBlurWeights[2].x, gBlurWeights[2].y, gBlurWeights[2].z, gBlurWeights[2].w,
    };
    
    // ȭ���� ũ�⸸ŭ �������� ���Ѵ�.
    float2 texOffset;
    // ���μ��� �ѹ��� �Ѵ�.
    // ĳ�ô� �ؽ��ĸ� �̿��ϰ�, cb�� �б⸦ �����Ѵ�.
    if (gHorizontalBlur)
    {
        texOffset = float2(gInvRenderTargetSize.x, 0.0f);
    }
    else
    {
        texOffset = float2(0.0f, gInvRenderTargetSize.y);
    }
    
    // Ÿ��(�߾�)���� �׻� ���տ� ���Եȴ�.
    float4 color = blurWeights[gBlurRadius] * gInputMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0);
    float totalWeight = blurWeights[gBlurRadius];
	// Ÿ���� ��ְ�, view���� ���̸� ���Ѵ�.
    float3 centerNormal = gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz;
    float centerDepth = NdcDepthToViewDepth(gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r);

    for (float i = -gBlurRadius; i <= gBlurRadius; ++i)
    {
		// Ÿ�� ���� �Ѿ��.
        if (i == 0)
            continue;
        
        float2 tex = pin.TexC + i * texOffset;
        // �ֺ� ��ְ� view ���̸� ���Ѵ�.
        float3 neighborNormal = gNormalMap.SampleLevel(gsamPointClamp, tex, 0.0f).xyz;
        float neighborDepth = NdcDepthToViewDepth(
            gDepthMap.SampleLevel(gsamDepthMap, tex, 0.0f).r);

		//
		// ����̳� ���� ���̰� ���ϸ�, ���� ��迡 ���� �ʴٰ�
        // �Ǵ��ؼ� blur ��꿡 ���Խ�Ű�� �ʴ´�.
		//
	
        if (dot(neighborNormal, centerNormal) >= 0.8f &&
		    abs(neighborDepth - centerDepth) <= 0.2f)
        {
            float weight = blurWeights[i + gBlurRadius];

			// blur ��꿡 �����Ѵ�.
            color += weight * gInputMap.SampleLevel(
                gsamPointClamp, tex, 0.0);
		
            totalWeight += weight;
        }
    }

	// ��꿡 ���Ե��� ���� ģ������ ����, ����ȭ�� ��Ų��.
    return color / totalWeight;
}
