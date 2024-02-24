//***************************************************************************************
// 15_CubeMap.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

#include "Commons.hlsl"

// �Է����� Pos, Norm �� �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

// ������δ� World Pos�� ���� (Homogeneous clip space) Pos�� ������.
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    // ���׸��� �����͸� �����´�
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    // World Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling �̶�� �����ϰ� ����Ѵ�.
    // (�׷��� ������, ����ġ ����� ����ؾ� �Ѵ�.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.

    vout.PosH = mul(posW, gViewProj);
    
    // Texture���� �־��� Transform�� �����Ų ����
    float4 texC = mul(float4(vin.TexC, 0.f, 1.f), gTexTransform);
    // World�� ���ؼ� PS�� �Ѱ��ش�.
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Object Constant�� �ִ� Index�� �̿��ؼ�
    // Pipeline�� bind ��, Material ������ Texture�� �����ͼ� ���� �̾Ƴ���.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    
    // �ؽ��� �迭���� �������� �ؽ��ĸ� �����´�.
    diffuseAlbedo *= gDiffuseMap[diffuseTexIndex].Sample(gSamLinearWrap, pin.TexC);
    
#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
// �ϴ� ����ȭ�� �Ѵ�.
    pin.NormalW = normalize(pin.NormalW);

    // ǥ�鿡�� ī�޶� ���� ���͸� ���Ѵ�.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// �ϴ� ���� �������� �����Ѵ�.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    // ������ �����ϰ�
    const float shininess = 1.0f - roughness;
    // ����ü�� ä�� ����
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    // (�׸��ڴ� ���߿� �� ���̴�.)
    float3 shadowFactor = 1.0f;
    // ������ ���� �ߴ� ���� �̿��ؼ� �ѱ��.
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        pin.NormalW, toEyeW, shadowFactor);
    // ���� ���� �����ϰ�
    float4 litColor = ambient + directLight;
    
    // # �ֺ� ȯ�� �ݻ縦 �Ѵ�.
    // ī�޶� ��ġ����, ���� �ȼ��� ��ġ�Ѱ��� World ������ Ÿ��
    // ��� ������� reflect()�� �̿��� ���Ѵ�.
    float3 r = reflect(-toEyeW, pin.NormalW);
    // �� ���͸� �״�� lookup vector�� ����ϸ� ��Ȯ���� �ʴ�. 
    // ī�޶󿡼� ��� ��ó�� lookup�� �ϱ� �����̴�.
    // �ϴ��� �� �����̰� ũ�Ⱑ ��û Ŀ��(1000) ������� ���� ������.
    // ��� ���� ����� ������ �����̶�� ������� ���� �� �ִ�.
    float4 reflectionColor = gCubeMap.Sample(gSamLinearWrap, r);
    // �ݻ籤�� Fresnel�� ����Ͽ� �����´�.
    float3 fresnelFactor = SchlickFresnel(fresnelR0, pin.NormalW, r);
    // ��ĥ�⵵ �ݿ��Ѵ�.
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    // diffuse albedo���� alpha���� �����´�.
    litColor.a = diffuseAlbedo.a;

    
    // ����Ѵ�.
    return litColor;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blend.hlsl" /Od /D "ALPHA_TEST"="1"/Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.asm"
