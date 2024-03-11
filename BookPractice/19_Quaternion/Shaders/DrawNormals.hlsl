//***************************************************************************************
// DrawNormals.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

	// 머테리얼 데이터를 가져온다
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    // Uniform Scaling 이라고 가정하고 계산한다.
    // (그렇지 않으면, 역전치 행렬을 사용해야 한다.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);
    
    // 탄젠트도 마찬가지로 역전치 행렬을 사용하여 월드로 변환한다.
    // 행렬 곱이고, 기저 변환에 따라 이렇게 월드행렬을 먼저 곱해도 된다.
    //vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos로 바꾼다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// 삼각형 따라 보간되도록 한다.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Object Constant에 있는 Index를 이용해서
    // Pipeline에 bind 된, Material 정보와 Texture를 가져와서 색을 뽑아낸다.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
    uint normalMapIndex = matData.NormalMapIndex;
	
    // 텍스쳐 배열에서 동적으로 텍스쳐를 가져온다.
    diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gSamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    // 알파 값이 일정값 이하면 그냥 잘라버린다.
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// 혹시 모르니 다시 정규화 해주고
    pin.NormalW = normalize(pin.NormalW);
	
    // 보간된 vertex의 normal을 SSAO에 사용할 것이다.

    // view 공간에 있는 normal을 기록한다. 
    float3 normalV = mul(pin.NormalW, (float3x3) gView);
    return float4(normalV, 0.0f);
}


