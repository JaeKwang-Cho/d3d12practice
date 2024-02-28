//***************************************************************************************
// Commons.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// 공통으로 쓰는 헤더나, 데이터 타입 같은 것들을 미리 정의해 놓은 것이다.
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

/*
struct InstanceData
{
    float4x4 WorldMat;
    float4x4 InvWorldTransposeMat;
    float4x4 TexTransform;
    uint MaterialIndex;
    uint InstPad0;
    uint InstPad1;
    uint InstPad2;
};
*/
struct MaterialData
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Roughness;
    float4x4 MaterialTransform;
    uint DiffuseMapIndex;
    uint NormalMapIndex;
    uint DispMapIndex;
    uint MatPad0;
};

// 6개의 Texture를 가지고 있고, 록업 벡터를 이용해서
// 픽셀 값을 참조하는 리소스이다.
TextureCube gCubeMap : register(t0);

// 이제 노멀맵도 함께 사용할 것이기 때문에, 개수도 늘려주고, 일반적인 이름으로 바꿔준다.
Texture2D gTextureMaps[16] : register(t1);

//StructuredBuffer<InstanceData> gInstanceData : register(t0, space1);
StructuredBuffer<MaterialData> gMaterialData : register(t0, space1);

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

// 물체마다 가지고 있는 Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvWorldTranspose;
    float4x4 gTexTransform;
    uint gMaterialIndex;
    uint gObjPad0;
    uint gObjPad1;
    uint gObjPad2;
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
 
// normal map sample을 world 좌표계로 변환한다.
float3 NormalSampleToWorldSpace(float3 _normalMapSample, float3 _unitNormalW, float3 tangentW)
{
    // 구간을 [-1, 1]로 매핑한다.
    float3 normalT = 2.f * _normalMapSample - 1.f;
    
    // TBN basis를 생성한다.
    float3 N = _unitNormalW;
    // tangentW는 보간되어 넘어온 값이기 때문에, T에서 N 성분을 제거해야, 서로 직교가 된다.
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    // 외적으로 B을 마저 구한다.
    float3 B = cross(N, T);
    
    // basis - transform 행렬을 만들고
    float3x3 TBN = float3x3(T, B, N);
    // world 좌표계로 바꾼다.
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW;
}

float3 WorldSpaceToTangentSpace(float3 _worldPos, float3 _unitNormalW, float3 tangentW)
{
    // TBN basis를 생성한다.
    float3 N = _unitNormalW;
    // tangentW는 보간되어 넘어온 값이기 때문에, T에서 N 성분을 제거해야, 서로 직교가 된다.
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    // 외적으로 B을 마저 구한다.
    float3 B = cross(N, T);
    
    // World to Tangent 행렬은 Tangent to World의 역행렬이다.
    // 근데 좌표계의 T, B, N은 각 축을 이루는 벡터고, 서로 직교한다.
    // 직교행렬의 역행렬은? 그 행렬의 전치행렬이다.
    float3 row0 = float3(T.r, B.r, N.r);
    float3 row1 = float3(T.g, B.g, N.g);
    float3 row2 = float3(T.b, B.b, N.b);
    float3x3 invTBN = float3x3(row0, row1, row2);
    // world 좌표계로 바꾼다.
    float3 bumpedNormalW = mul(_worldPos, invTBN);
    
    return bumpedNormalW;
}