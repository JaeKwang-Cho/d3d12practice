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
    float cbPerObjectPad1;
    
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
    float2 cbPerObjectPad2;

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
    float3 PosW : POSITION;
};

// ������δ� world Centher pos�� ��´�.
struct VertexOut
{
    float3 CenterW : POSITION;
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
    vout.CenterW = vin.PosW;
    
    return vout;
}

// Geometry Shader�� ���� �Ǵ� ���� �ִ� ������ 10���� �����Ѵ�.
[maxvertexcount(10)]
void GS(
    line VertexOut gin[2], // �迭 ũ�⸦ 2�� �����ؼ� ������ �Է¹ް�
    uint primID : SV_PrimitiveID,
    inout TriangleStream<GeoOut> triStream // �ﰢ����� ����� �Ѵ�.
        )
{
    float currY = -5.f; // �Ʒ����� ���� �ö�´�.
    float texCstep = 0.2f; // �׳� �ӽ� texcoords
    
    GeoOut gout1;
    GeoOut gout2;
    
	[unroll]
    for (int i = 0; i < 5; i++)
    {
        float4 v1;
        v1.xyz = gin[0].CenterW;
        v1.y = currY;
        v1.w = 1.f;
        
        gout1.PosH = mul(v1, gViewProj);
        gout1.PosW = v1.xyz;
        gout1.NormalW = gin[0].CenterW;
        gout1.TexC = float2(0.f, texCstep * i);
        gout1.PrimID = primID;
        
        float4 v2;
        v2.xyz = gin[1].CenterW;
        v2.y = currY;
        v2.w = 1.f;
        
        gout2.PosH = mul(v2, gViewProj);
        gout2.PosW = v2.xyz;
        gout2.NormalW = gin[1].CenterW;
        gout2.TexC = float2(1.f, texCstep * i);
        gout2.PrimID = primID;
		
        // app���� xz ��鿡�� rad�� �����ϵ��� �־��༭ �̷��� �ݴ�� �ؾ� ����� cull�� �ȴ�.
        triStream.Append(gout2);
        triStream.Append(gout1);
        
        currY += 2.f;
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
