//***************************************************************************************
// 20_Animation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

#include "Commons.hlsl"

// �Է����� Pos, Norm, TexC, TangentU �� �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
#endif
};

// ������δ� World Pos�� ���� (Homogeneous clip space) Pos�� ������.
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float4 ShadowPosH : POSITION0;
    float4 SsaoPosH : POSITION1;
    float3 PosW : POSITION2;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    // ���׸��� �����͸� �����´�
    MaterialData matData = gMaterialData[gMaterialIndex];
    
#ifdef SKINNED
        float weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.0f - weights[0] - weights[1] - weights[2];

    float3 posL = float3(0.0f, 0.0f, 0.0f);
    float3 normalL = float3(0.0f, 0.0f, 0.0f);
    float3 TangentU = float3(0.0f, 0.0f, 0.0f);
    for(int i = 0; i < 4; ++i)
    {
        // Uniform scaling �̶�� �����Ѵ�.
        // �׷��� ������ inverse-transpose�� ���ؾ��Ѵ�.

        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        TangentU += weights[i] * mul(vin.TangentU.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
    }

    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentU.xyz = TangentU;
#endif
	
    // World Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling �̶�� �����ϰ� ����Ѵ�.
    // (�׷��� ������, ����ġ ����� ����ؾ� �Ѵ�.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);
    
    // ź��Ʈ�� ���������� ����ġ ����� ����Ͽ� ����� ��ȯ�Ѵ�.
    // ��� ���̰�, ���� ��ȯ�� ���� �̷��� ��������� ���� ���ص� �ȴ�.
    //vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.
    vout.PosH = mul(posW, gViewProj);
    
    // Texture���� �־��� Transform�� �����Ų ����
    float4 texC = mul(float4(vin.TexC, 0.f, 1.f), gTexTransform);
    // World�� ���ؼ� PS�� �Ѱ��ش�.
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
    
    // PassCB���� �Ѱ��� ��ķ�, ������������ ��ȯ�Ѵ�.
    vout.ShadowPosH = mul(posW, gShadowMat);
    
    // ������ Ssao Map�� ���� �ٷ� ã�� �� �ִ� viewProjTexMat�� �̿��Ѵ�.
    vout.SsaoPosH = mul(posW, gViewProjTexMat);
    
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
    uint normalMatIndex = matData.NormalMapIndex;
    
    // �ؽ��� �迭���� �������� �ؽ��ĸ� �����´�.
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamAnisotropicWrap, pin.TexC);
        
#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    // TBN�� world�� �ٲٱ� ���� Vertex Normal, Tangent�� �����´�.
    pin.NormalW = normalize(pin.NormalW);
    // UV�� �´� Texture Normal ���� �����´�.
    float4 normalMapSample = gTextureMaps[normalMatIndex].Sample(gSamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    // ǥ�鿡�� ī�޶� ���� ���͸� ���Ѵ�.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// ���� �������� Ssao�� ����� �ٲ�� ���ش�.
    // �ϴ� w�� ������(���� �ִ� Homogenous������ Depth) ���� ��ȭ�� ������ ���ְ�
    pin.SsaoPosH /= pin.SsaoPosH.w;
    // Ssao Map���� ����� �����´�.
    float ambientAccss = gSsaoMap.Sample(gSamLinearClamp, pin.SsaoPosH.xy, 0.f).r;
    
    float4 ambient = gAmbientLight * diffuseAlbedo * ambientAccss;
    
    // �ϴ��� ù��° ������ ���ؼ���, �׸��ڸ� �����Ѵ�.
    float3 shadowFactor = float3(1.f, 1.f, 1.f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    // ������ �����ϰ� (a ä�ο� shininess ���� ����ִ� ��쵵 �ִ�.)
    const float shininess = (1.0f - roughness) * normalMapSample.a;
    // ����ü�� ä�� ����
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
        
    // ������ ���� �ߴ� ���� �̿��ؼ� �ѱ��.    
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);
    // ���� ���� �����ϰ�
    float4 litColor = ambient + directLight;

    // # �ֺ� ȯ�� �ݻ縦 �Ѵ�.
    // ī�޶� ��ġ����, ���� �ȼ��� ��ġ�Ѱ��� World ������ Ÿ��
    // ��� ������� reflect()�� �̿��� ���Ѵ�.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    // �� ���͸� �״�� lookup vector�� ����ϸ� ��Ȯ���� �ʴ�. 
    // ī�޶󿡼� ��� ��ó�� lookup�� �ϱ� �����̴�.
    // �ϴ��� �� �����̰� ũ�Ⱑ ��û Ŀ��(1000) ������� ���� ������.
    // ��� ���� ����� ������ �����̶�� ������� ���� �� �ִ�.
    float4 reflectionColor = gCubeMap.Sample(gSamLinearWrap, r);
    // �ݻ籤�� Fresnel�� ����Ͽ� �����´�.
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    // ��ĥ�⵵ �ݿ��Ѵ�.
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;
        
    // diffuse albedo���� alpha���� �����´�.
    litColor.a = diffuseAlbedo.a;
    
    
    // ����Ѵ�.
    return litColor;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blend.hlsl" /Od /D "ALPHA_TEST"="1"/Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.asm"
