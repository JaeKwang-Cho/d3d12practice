//=============================================================================
// Sky.hlsl by Frank Luna (C) 2011 All Rights Reserved.
// 저 멀리(?) 하늘을 그리는 역할을 하는 쉐이더다.
//=============================================================================

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn _vin)
{
    VertexOut vout;
    
    // 로컬위치를 룩업 벡터로 그대로 사용한다.
    vout.PosL = _vin.PosL;
    
    // 룩업으로 찾은 픽셀을 찍을 공간은, 월드로 바꾸고
    float4 posW = mul(float4(_vin.PosL, 1.f), gWorld);
    
    // 그것을 카메라 위치에 따라 움직이도록 고정한다.
    posW.xyz += gEyePosW;
    
    // 화면에 찍힐때 z값이(depth는 나중에 사원수로 빼놓은
    // w 값으로 나눴을 때) 항상 1(가장멀리) 찍히도록 한다.
    vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

float4 PS(VertexOut _pin) : SV_Target
{
    // 저장해놨던 룩업 벡터로 픽셀을 봅아낸다.
    return gCubeMap.Sample(gSamLinearWrap, _pin.PosL);
}