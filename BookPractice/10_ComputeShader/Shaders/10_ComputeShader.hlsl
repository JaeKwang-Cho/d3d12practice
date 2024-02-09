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
Texture2D gDisplacementMap : register(t1);

SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamPointMirror : register(s2);
SamplerState gSamPointBorder : register(s3);
SamplerState gSamLinearWrap : register(s4);
SamplerState gSamLinearClamp: register(s5);
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
#ifdef DISPLACEMENT_MAP
    float2 gDisplacementMapTexelSize;
    float gGridSpatialStep;
    float cbPerObjectPad1;
#endif       
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
    float cbPerPassad1;
    
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

 
// 입력으로 Pos, Norm 을 받는다.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// 출력으로는 World Pos와 동차 (Homogeneous clip space) Pos로 나눈다.
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
    
#ifdef DISPLACEMENT_MAP
    // 월드로 변형하지 않은 값을 가져와서 작업한다. (당연한 소리)
    vin.PosL.y += gDisplacementMap.SampleLevel(gSamLinearWrap, vin.TexC, 1.f).r;
    
    // 노멀은 유한 차분으로 구한다.
    float du = gDisplacementMapTexelSize.x;
    float dv = gDisplacementMapTexelSize.y;
    float l = gDisplacementMap.SampleLevel(gSamPointClamp, vin.TexC - float2(du, 0.f), 0.f).r;
    float r = gDisplacementMap.SampleLevel(gSamPointClamp, vin.TexC + float2(du, 0.f), 0.f).r;
    float t = gDisplacementMap.SampleLevel(gSamPointClamp, vin.TexC - float2(0.f, dv), 0.f).r;
    float b = gDisplacementMap.SampleLevel(gSamPointClamp, vin.TexC + float2(0.f, dv), 0.f).r;
    vin.NormalL = normalize(float3(-r + l, 2.f * gGridSpatialStep, b - t));
#endif
	
    // World Pos로 바꾼다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling 이라고 가정하고 계산한다.
    // (그렇지 않으면, 역전치 행렬을 사용해야 한다.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos로 바꾼다.

    vout.PosH = mul(posW, gViewProj);
    
    // Texture에게 주어진 Transform을 적용시킨 다음
    float4 texC = mul(float4(vin.TexC, 0.f, 1.f), gTexTransform);
    // World를 곱해서 PS로 넘겨준다.
    vout.TexC = mul(texC, gMatTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
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
    //float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    // fog의 색으로 채워진다.
    //litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // diffuse albedo에서 alpha값을 가져온다.
    litColor.a = diffuseAlbedo.a;

    
    // 출력한다.
    return litColor;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blend.hlsl" /Od /D "ALPHA_TEST"="1"/Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.asm"
