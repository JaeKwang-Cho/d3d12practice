//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
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


// Material ���� �޶����� Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvWorldTranspose;
    float4x4 gTexTransform;
    uint gDetail;
    float3 cbPerObjectPad1;
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
    float cbPerPassPad1;
    
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

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

 
// �Է����� Pos�� �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// ������δ� world Centher pos�� ��´�.
struct VertexOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// ������Ʈ�� ���̴��� ��� ����ü�̴�.
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
    
    // VS������ �ƹ��͵� ���Ѵ�.
    vout = vin;
    
    return vout;
}


// Geometry Shader�� ���� �Ǵ� ���� �ִ� ������ 2���� �����Ѵ�.
[maxvertexcount(2)]
void GS(
    triangle VertexOut gin[3], // �迭 ũ�⸦ 1�� �����ؼ� ������ �Է¹ް�
    uint primID : SV_PrimitiveID,
    inout LineStream<GeoOut> lineStream // ���н�Ʈ�� ����� �Ѵ�.
        )
{
    float3 verts[3];
    // ���� ����
    verts[0] = mul(float4(gin[0].PosL, 1.f), gWorld).xyz;
    verts[1] = mul(float4(gin[1].PosL, 1.f), gWorld).xyz;
    verts[2] = mul(float4(gin[2].PosL, 1.f), gWorld).xyz;
    
    float3 vec0 = verts[1] - verts[0];
    float3 vec1 = verts[2] - verts[0];
    float3 triNorm = cross(vec0, vec1);
    triNorm = normalize(triNorm);
    
    GeoOut gout[2];
    gout[0].PosW = (verts[0] + verts[1] + verts[2]) / 3;
    
    verts[0] = mul(gin[0].NormalL, (float3x3) gInvWorldTranspose).xyz;
    verts[1] = mul(gin[1].NormalL, (float3x3) gInvWorldTranspose).xyz;
    verts[2] = mul(gin[2].NormalL, (float3x3) gInvWorldTranspose).xyz;
    gout[0].NormalW = (verts[0] + verts[1] + verts[2]) / 3;
    // ȣ�� ����
    gout[0].PosH = mul(float4(gout[0].PosW, 1.f), gViewProj);
    gout[0].TexC = (gin[0].TexC + gin[1].TexC + gin[2].TexC);
    
    gout[1].PosW = gout[0].PosW + (triNorm * 3.f);
    gout[1].NormalW = gout[0].NormalW;
    gout[1].PosH = mul(float4(gout[1].PosW, 1.f), gViewProj);
    gout[1].TexC = gout[0].TexC;
    
    lineStream.Append(gout[0]);
    lineStream.Append(gout[1]);
}

float4 PS(GeoOut pin) : SV_Target
{
    // Texture���� ���� �̾Ƴ���. (���÷��� ���׸��� ���̽� ���� �Բ� �����Ų��.)
    float4 diffuseAlbedo = float4(1.f, 1.f, 0.f, 1.f);
    
    // ����Ѵ�.
    return diffuseAlbedo;
}
