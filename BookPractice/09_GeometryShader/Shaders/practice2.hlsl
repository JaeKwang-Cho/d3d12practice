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

void Subdivide(VertexOut invert0, VertexOut invert1, VertexOut invert2, out VertexOut outVerts[6])
{
    // ���� ���
    VertexOut m[3];
        
    m[0].PosL = 0.5f * (invert0.PosL + invert1.PosL);
    m[1].PosL = 0.5f * (invert1.PosL + invert2.PosL);
    m[2].PosL = 0.5f * (invert2.PosL + invert0.PosL);
    
    // �������� ����
    m[0].PosL = normalize(m[0].PosL);
    m[1].PosL = normalize(m[1].PosL);
    m[2].PosL = normalize(m[2].PosL);
    
    // ���� ���
    m[0].NormalL = m[0].PosL;
    m[1].NormalL = m[1].PosL;
    m[2].NormalL = m[2].PosL;
    
    // �ؽ��� ��ǥ ����
    m[0].TexC = 0.5f * (invert0.TexC + invert1.TexC);
    m[1].TexC = 0.5f * (invert1.TexC + invert2.TexC);
    m[2].TexC = 0.5f * (invert2.TexC + invert0.TexC);

    // ���� �� ���缭 �־�� �Ѵ�.
    outVerts[0] = invert0;
    outVerts[1] = m[0];
    outVerts[2] = m[2];
    outVerts[3] = m[1];
    outVerts[4] = invert2;
    outVerts[5] = invert1;
}

void OutPutSubdivision(VertexOut v[6], inout TriangleStream<GeoOut> triStream)
{
    GeoOut gout[6];
    
    [unroll]
    for (int i = 0; i < 6; i++)
    {
        // ���� ����
        gout[i].PosW = mul(float4(v[i].PosL, 1.f), gWorld).xyz;
        gout[i].NormalW = mul(v[i].NormalL, (float3x3) gInvWorldTranspose);
        // ȣ�� ����
        gout[i].PosH = mul(float4(gout[i].PosW, 1.f), gViewProj);
        // �ý���
        gout[i].TexC = v[i].TexC;

    }
    [unroll]
    for (int j = 0; j < 5; j++)
    {
        triStream.Append(gout[j]);
    }
    triStream.RestartStrip();
    triStream.Append(gout[1]);
    triStream.Append(gout[5]);
    triStream.Append(gout[3]);
}

// Geometry Shader�� ���� �Ǵ� ���� �ִ� ������ 32���� �����Ѵ�.
[maxvertexcount(32)]
void GS(
    triangle VertexOut gin[3], // �迭 ũ�⸦ 3�� �����ؼ� �ﰢ������ �Է¹ް�
    uint primID : SV_PrimitiveID,
    inout TriangleStream<GeoOut> triStream // �ﰢ����� ����� �Ѵ�.
        )
{
    VertexOut vout[6];
    VertexOut vin[6];
    [branch]
    switch (gDetail)
    {
        case 2:
            {
                Subdivide(gin[0], gin[1], gin[2], vout);  
                vin = vout;
                Subdivide(vin[0], vin[1], vin[2], vout);
                OutPutSubdivision(vout, triStream);
                triStream.RestartStrip();
                Subdivide(vin[1], vin[3], vin[2], vout);
                OutPutSubdivision(vout, triStream);
                triStream.RestartStrip();
                Subdivide(vin[2], vin[3], vin[4], vout);
                OutPutSubdivision(vout, triStream);
                triStream.RestartStrip();
                Subdivide(vin[1], vin[5], vin[3], vout);
                OutPutSubdivision(vout, triStream);
            }
            break;
        case 1:
            {
                Subdivide(gin[0], gin[1], gin[2], vout);
                OutPutSubdivision(vout, triStream);
            }
            break;
        default:
            {
                GeoOut gout[3];
                [unroll]
                for (int i = 0; i < 3; i++)
                {
                    // ���� ����
                    gout[i].PosW = mul(float4(gin[i].PosL, 1.f), gWorld).xyz;
                    gout[i].NormalW = mul(gin[i].NormalL, (float3x3) gInvWorldTranspose);
                    // ȣ�� ����
                    gout[i].PosH = mul(float4(gout[i].PosW, 1.f), gViewProj);
                    // �ý���
                    gout[i].TexC = gin[i].TexC;
                    
                }
                triStream.Append(gout[0]);
                triStream.Append(gout[2]);
                triStream.Append(gout[1]);
            }
            break;
    }
}

float4 PS(GeoOut pin) : SV_Target
{
    // Texture���� ���� �̾Ƴ���. (���÷��� ���׸��� ���̽� ���� �Բ� �����Ų��.)
    float4 diffuseAlbedo = gDiffuseMap.Sample(gSamLinearWrap, pin.TexC) * gDiffuseAlbedo;
    
#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
// �ϴ� ����ȭ�� �Ѵ�.
    pin.NormalW = normalize(pin.NormalW);

    // ǥ�鿡�� ī�޶� ���� ���͸� ���Ѵ�.
    float3 toEyeW = gEyePosW - pin.PosW;
    float distToEye = length(toEyeW); // ǥ��� ī�޶��� �Ÿ��� ���ϰ�
    toEyeW /= distToEye; // ����ȭ �Ѵ�.

	// �ϴ� ���� �������� �����Ѵ�.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    // ������ �����ϰ�
    const float shininess = 1.0f - gRoughness;
    // ����ü�� ä�� ����
    Material mat = { diffuseAlbedo, gFresnelR0, shininess };
    // (�׸��ڴ� ���߿� �� ���̴�.)
    float3 shadowFactor = 1.0f;
    // ������ ���� �ߴ� ���� �̿��ؼ� �ѱ��.
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    // ���� ���� �����ϰ�
    float4 litColor = ambient + directLight;
    
#ifdef FOG
    // ī�޶󿡼� �Ÿ��� �� ����
    float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
    // fog�� ������ ä������.
    litColor = lerp(litColor, gFogColor, fogAmount);
#endif

    // diffuse albedo���� alpha���� �����´�.
    litColor.a = diffuseAlbedo.a;

    
    // ����Ѵ�.
    return litColor;
}
