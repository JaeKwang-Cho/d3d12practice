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

Texture2DArray gTreeMapArray: register(t0);

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

 
// �Է����� Pos, Size�� �޴´�.
struct VertexIn
{
    float3 PosW : POSITION;
    float2 SizeW : SIZE;
};

// ������δ� world Centher pos�� world size�� ��´�.
struct VertexOut
{
    float3 CenterW : POSITION;
    float2 SizeW : SIZE;
};

// ������Ʈ�� ���̴��� ��� ����ü�̴�.
struct GeoOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
    uint PrimID : SV_PrimitiveID; // � primitive���� ����ϴ� �ɹ��� �ִ�.
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout;
    
    // VS������ �ƹ��͵� ���Ѵ�.
    vout.CenterW = vin.PosW;
    vout.SizeW = vin.SizeW;
    
    return vout;
}

// Geometry Shader�� ���� �Ǵ� ���� �ִ� ������ 4���� �����Ѵ�.
[maxvertexcount(4)]
void GS(
    point VertexOut gin[1], // �迭 ũ�⸦ 1�� �����ؼ� ������ �Է¹ް�
    uint primID : SV_PrimitiveID, 
    inout TriangleStream<GeoOut> triStream // �ﰢ����� ����� �Ѵ�.
        )
{
    // Sprite �� ī�޶� y-axis align ��ų �� �ֵ��� ����� �Ѵ�.

    // local ��ǥ���� ���Ѵ�.
    float3 up = float3(0.0f, 1.0f, 0.0f);
    float3 look = gEyePosW - gin[0].CenterW;
    look.y = 0.0f; // xz-��鿡 �翵��Ų��.
    look = normalize(look);
    float3 right = cross(up, look);

	// ������ Vertex�� world�� ǥ���Ѵ�.
    float halfWidth = 0.5f * gin[0].SizeW.x;
    float halfHeight = 0.5f * gin[0].SizeW.y;
	
    // local ��ǥ ���, ũ�� ���� �̿��ؼ�, Billboard�� ��������  world ��ġ�� ���Ѵ�.
    float4 v[4];
    v[0] = float4(gin[0].CenterW + halfWidth * right - halfHeight * up, 1.0f);
    v[1] = float4(gin[0].CenterW + halfWidth * right + halfHeight * up, 1.0f);
    v[2] = float4(gin[0].CenterW - halfWidth * right - halfHeight * up, 1.0f);
    v[3] = float4(gin[0].CenterW - halfWidth * right + halfHeight * up, 1.0f);

	// ���� triangle strip���μ� ������ش�.
	
    float2 texC[4] =
    {
        float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
    };
	
    GeoOut gout;
	[unroll]
    for (int i = 0; i < 4; ++i)
    {
        gout.PosH = mul(v[i], gViewProj);
        gout.PosW = v[i].xyz;
        gout.NormalW = look;
        gout.TexC = texC[i];
        gout.PrimID = primID;
		
        triStream.Append(gout);
    }
}

float4 PS(GeoOut pin) : SV_Target
{
    // w ���е� �̿��ؼ� Texture2DArray����  Sample�� �̾ƿ´�.
    float3 uvw = float3(pin.TexC, pin.PrimID % 2);
    // Texture���� ���� �̾Ƴ���. (���÷��� ���׸��� ���̽� ���� �Բ� �����Ų��.)
    float4 diffuseAlbedo = gTreeMapArray.Sample(gSamAnisotropicWrap, uvw) * gDiffuseAlbedo;
    
#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    //clip(diffuseAlbedo.a - 0.1f);
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
