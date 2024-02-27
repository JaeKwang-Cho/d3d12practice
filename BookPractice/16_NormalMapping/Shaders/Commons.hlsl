//***************************************************************************************
// Commons.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// �������� ���� �����, ������ Ÿ�� ���� �͵��� �̸� ������ ���� ���̴�.
//***************************************************************************************

// LightingUtil.hlsl�� include ���ֱ� ���� �̷��� �ؾ�, 
// ����� �˸°� �ȴ�.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// �� ����� �ϴµ� �ʿ��� �Լ��� ��� ���� ���̴�.
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
    uint MatPad0;
    uint MatPad1;
};

// 6���� Texture�� ������ �ְ�, �Ͼ� ���͸� �̿��ؼ�
// �ȼ� ���� �����ϴ� ���ҽ��̴�.
TextureCube gCubeMap : register(t0);

// ���� ��ָʵ� �Բ� ����� ���̱� ������, ������ �÷��ְ�, �Ϲ����� �̸����� �ٲ��ش�.
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

// ��ü���� ������ �ִ� Constant Buffer
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

// ������ ���� �޶����� Constant Buffer
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
    
    // fog ������ pass�� �Ѿ�´�
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
 
// normal map sample�� world ��ǥ��� ��ȯ�Ѵ�.
float3 NormalSampleToWorldSpace(float3 _normalMapSample, float3 _unitNormalW, float3 tangentW)
{
    // ������ [-1, 1]�� �����Ѵ�.
    float3 normalT = 2.f * _normalMapSample - 1.f;
    
    // TBN basis�� �����Ѵ�.
    float3 N = _unitNormalW;
    // tangentW�� �����Ǿ� �Ѿ�� ���̱� ������, T���� N ������ �����ؾ�, ���� ������ �ȴ�.
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    // �������� B�� ���� ���Ѵ�.
    float3 B = cross(N, T);
    
    // basis - transform ����� �����
    float3x3 TBN = float3x3(T, B, N);
    // world ��ǥ��� �ٲ۴�.
    float3 bumpedNormalW = mul(normalT, TBN);
    
    return bumpedNormalW;
}

float3 WorldSpaceToTangentSpace(float3 _worldPos, float3 _unitNormalW, float3 tangentW)
{
    // TBN basis�� �����Ѵ�.
    float3 N = _unitNormalW;
    // tangentW�� �����Ǿ� �Ѿ�� ���̱� ������, T���� N ������ �����ؾ�, ���� ������ �ȴ�.
    float3 T = normalize(tangentW - dot(tangentW, N) * N);
    // �������� B�� ���� ���Ѵ�.
    float3 B = cross(N, T);
    
    // World to Tangent ����� Tangent to World�� ������̴�.
    // �ٵ� ��ǥ���� T, B, N�� �� ���� �̷�� ���Ͱ�, ���� �����Ѵ�.
    // ��������� �������? �� ����� ��ġ����̴�.
    float3 row0 = float3(T.r, B.r, N.r);
    float3 row1 = float3(T.g, B.g, N.g);
    float3 row2 = float3(T.b, B.b, N.b);
    float3x3 invTBN = float3x3(row0, row1, row2);
    // world ��ǥ��� �ٲ۴�.
    float3 bumpedNormalW = mul(_worldPos, invTBN);
    
    return bumpedNormalW;
}