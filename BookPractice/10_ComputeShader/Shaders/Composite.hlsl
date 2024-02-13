//***************************************************************************************
// Composite.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// 두개의 Texture를 합친다.
//***************************************************************************************

Texture2D gBaseMap : register(t0);
Texture2D gEdgeMap : register(t1);

SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamPointMirror : register(s2);
SamplerState gSamPointBorder : register(s3);
SamplerState gSamLinearWrap : register(s4);
SamplerState gSamLinearClamp : register(s5);
SamplerState gSamLinearMirror : register(s6);
SamplerState gSamLinearBorder : register(s7);
SamplerState gSamAnisotropicWrap : register(s8);
SamplerState gSamAnisotropicClamp : register(s9);


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
    float2 TexC : TEXCOORD;
};

VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;
	// 드디어 SV_VertexID를 사용한다.
    // App에서 보면 냅다 DrawInstanced를 건다.
    vout.TexC = gTexCoords[vid];
    // 그대로 화면 공간 NDC Coords로 그린다.
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // 픽셀 쉐이더에서, 텍스쳐를 곱한다.
    float4 c = gBaseMap.SampleLevel(gSamPointClamp, pin.TexC, 0.0f);
    float4 e = gEdgeMap.SampleLevel(gSamPointClamp, pin.TexC, 0.0f);
	
    return c * e;
}


