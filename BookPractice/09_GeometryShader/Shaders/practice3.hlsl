//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// LightingUtil.hlsl을 include 해주기 전에 이렇개 해야, 
// 계산이 알맞게 된다.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// 빛 계산을 하는데 필요한 함수를 모아 놓은 것이다.
#include "LightingUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

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


// Material 마다 달라지는 Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvWorldTranspose;
    float4x4 gTexTransform;
    uint gDetail;
    float3 cbPerObjectPad1;
};

// 프레임 마다 달라지는 Constant Buffer
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    
    float3 gEyePosW;
    float cbPerPassPad1;
    
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    
    float4 gAmbientLight;
    
    // fog 정보는 pass로 넘어온다
    float4 gFogColor;
    
    float gFogStart;
    float gFogRange;
    float2 cbPerPassPad2;

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

 
// 입력으로 Pos를 받는다.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// 출력으로는 world Centher pos를 뱉는다.
struct VertexOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// 지오메트리 쉐이더의 출력 구조체이다.
struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // VS에서는 아무것도 안한다.
    vout = vin;
    
    return vout;
}


// Geometry Shader로 생성 되는 점의 최대 개수를 3개로 설정한다.
[maxvertexcount(3)]
void GS(
    triangle VertexOut gin[3], // 배열 크기를 3로 설정해서 삼각형으로 입력받고
    uint primID : SV_PrimitiveID,
    inout TriangleStream<GeoOut> triStream // 삼각형띠로 출력을 한다.
        )
{
    GeoOut gout[3];
    // 월드 공간
    gout[0].PosW = mul(float4(gin[0].PosL, 1.f), gWorld).xyz;
    gout[1].PosW = mul(float4(gin[1].PosL, 1.f), gWorld).xyz;
    gout[2].PosW = mul(float4(gin[2].PosL, 1.f), gWorld).xyz;
    
    float3 vec0 = gout[1].PosW - gout[0].PosW;
    float3 vec1 = gout[2].PosW - gout[0].PosW;
    float3 triNorm = cross(vec0, vec1);
    triNorm = normalize(triNorm);
    
    [unroll]
    for (int i = 0; i < 3; i++)
    {
        gout[i].PosW += triNorm * gTotalTime * primID * 0.1f;
        gout[i].NormalW = mul(gin[i].NormalL, (float3x3) gInvWorldTranspose);
        // 호모 공간
        gout[i].PosH = mul(float4(gout[i].PosW, 1.f), gViewProj);
        // 택스쳐
        gout[i].TexC = gin[i].TexC;

    }
    [unroll]
    for (int j = 0; j < 3; j++)
    {
        triStream.Append(gout[j]);
    }
}

float4 PS(GeoOut pin) : SV_Target
{
    // Texture에서 색을 뽑아낸다. (샘플러와 머테리얼 베이스 색도 함께 적용시킨다.)
    float4 diffuseAlbedo = gDiffuseMap.Sample(gSamLinearWrap, pin.TexC) * gDiffuseAlbedo;
    
#ifdef ALPHA_TEST
    // 알파 값이 일정값 이하면 그냥 잘라버린다.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
// 일단 정규화를 한다.
    pin.NormalW = normalize(pin.NormalW);

    // 표면에서 카메라 까지 벡터를 구한다.
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW); // 표면과 카메라의 거리를 구하고
    toEyeW /= distToEye; // 정규화 한다.

	// 일단 냅다 간접광을 설정한다.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    // 광택을 설정하고
    const float shininess = 1.0f - gRoughness;
    // 구조체를 채운 다음
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    // (그림자는 나중에 할 것이다.)
    float3 shadowFactor = 1.0f;
    // 이전에 정의 했던 식을 이용해서 넘긴다.
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    // 최종 색을 결정하고
    float4 litColor = ambient + directLight;
    
#ifdef FOG
    // 카메라에서 거리가 멀 수록
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    // fog의 색으로 채워진다.
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // diffuse albedo에서 alpha값을 가져온다.
    litColor.a = diffuseAlbedo.a;

    
    // 출력한다.
    return litColor;
}
