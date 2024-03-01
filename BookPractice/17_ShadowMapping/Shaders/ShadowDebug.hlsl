//***************************************************************************************
// ShadowDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// 광원 입장에서 깊이 버퍼가 어떻게 보이는지 그리는 쉐이더다.
//***************************************************************************************

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    // App에서 이미 호모 공간으로 변형되어 넘어온다.
    vout.PosH = float4(vin.PosL, 1.0f);
	
    vout.TexC = vin.TexC;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    return float4(gShadowMap.Sample(gSamLinearWrap, pin.TexC).rrr, 1.0f);
}

