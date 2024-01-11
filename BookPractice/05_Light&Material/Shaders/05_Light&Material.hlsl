//***************************************************************************************
// Default.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

// LightingUtil.hlsl�� include ���ֱ� ���� �̷��� �ؾ�, 
// ����� �˸°� �ȴ�.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#define PRAC6 (0)

// �� ����� �ϴµ� �ʿ��� �Լ��� ��� ���� ���̴�.
#include "LightingUtil.hlsl"


// Material ���� �޶����� Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
};

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

// ������ ���� �޶����� Constant Buffer
cbuffer cbPass : register(b2)
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

    // Indices [0, NUM_DIR_LIGHTS) are directional lights;
    // indices [NUM_DIR_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHTS) are point lights;
    // indices [NUM_DIR_LIGHTS+NUM_POINT_LIGHTS, NUM_DIR_LIGHTS+NUM_POINT_LIGHT+NUM_SPOT_LIGHTS)
    // are spot lights for a maximum of MaxLights per object.
    Light gLights[MaxLights];
};
 
// �Է����� Pos, Norm �� �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
};

// ������δ� World Pos�� Render(Homogeneous clip space) Pos�� ������.
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;
	
    // World Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling �̶�� �����ϰ� ����Ѵ�.
    // (�׷��� ������, ����ġ ����� ����ؾ� �Ѵ�.)
    vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.
    vout.PosH = mul(posW, gViewProj);

    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // �ϴ� ����ȭ�� �Ѵ�.
    pin.NormalW = normalize(pin.NormalW);

    // ǥ�鿡�� ī�޶� ���� ���͸� ���Ѵ�.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// �ϴ� ���� �������� �����Ѵ�.
    float4 ambient = gAmbientLight * gDiffuseAlbedo;

    // ������ �����ϰ�
    const float shininess = 1.0f - gRoughness;
    // ����ü�� ä�� ����
    Material mat = { gDiffuseAlbedo, gFresnelR0, shininess };
    // (�׸��ڴ� ���߿� �� ���̴�.)
    float3 shadowFactor = 1.0f;
    // ������ ���� �ߴ� ���� �̿��ؼ� �ѱ��.
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    // ���� ���� �����ϰ�
    float4 litColor = ambient + directLight;
    
#if PRAC6 // �� ���̵�
    float redVal = litColor.r;
    redVal *= 10.f;
    int redCeil = ceil(redVal);
    litColor.r = (float) redCeil / 10.f;
    
    float greenVal = litColor.g;
    greenVal *= 10.f;
    int greenCeil = ceil(greenVal);
    litColor.g = (float) greenCeil / 10.f;
    
    float blueVal = litColor.b;
    blueVal *= 10.f;
    int blueCeil = ceil(blueVal);
    litColor.b = (float) blueCeil / 10.f;
#endif

    // (�ϴ� diffuse�� ���ĸ� �״�� �����´�.)
    litColor.a = gDiffuseAlbedo.a;
    // ����Ѵ�.
    return litColor;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.asm"