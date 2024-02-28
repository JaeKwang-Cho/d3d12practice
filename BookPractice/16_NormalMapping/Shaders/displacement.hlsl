//***************************************************************************************
// displacement.hlsl by Frank Luna (C) 2015 All Rights Reserved.
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
};

// ������δ� World Pos�� ���� (Homogeneous clip space) Pos�� ������.
struct VertexOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

// ============ Vertex Shader ===============
VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout;
    vout.PosL = vin.PosL;
    vout.NormalL = vin.NormalL;
    vout.TexC = vin.TexC;
    vout.TangentU = vin.TangentU;

    return vout;
}

// ============== Hull Shader - Constant Hull Shader (input : Patch Control Point) =================
struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(
        InputPatch<VertexOut, 4> patch,
        uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // patch�� ���� ������ ���Ѵ�.
    float3 centerL = 0.25f * (patch[0].PosL 
    + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    

    float3 centerW = mul(float4(centerL, 1.f), gWorld).xyz;
    // ī�޶�� �Ÿ��� ���Ѵ�.
    float d = distance(centerW, gEyePosW);
    
    // ī�޶���� �Ÿ��� ���� �׼����̼� ������ ���Ѵ�.
    // �ִ� 64�� �׼����̼� �Ѵ�. (�Ÿ� 80�� ���� ���� �� ��)
    const float d0 = 20.f;
    const float d1 = 100.f;
    
    //float tess = 64.f * saturate((d1 - d) / (d1 - d0));
    float tess = 64;
    // �� �Ȱ��� �׼�����Ʈ �Ѵ�.
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;
	
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
	
    return pt;
}

// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float3 TangentU : TANGENT;
    float2 TexC : TEXCOORD;
};

[domain("quad")] // tri, quad, isoline
[outputcontrolpoints(4)] // ��ġ�� ���� �潦�̴��� ������ ������ ����
[partitioning("integer")] // tess factor�� ������ ���� �۵�
[outputtopology("triangle_cw")] // �ð� ����
[patchconstantfunc("ConstantHS")] // �׼����̼� ����� ���� CHS �̸�
[maxtessfactor(64.f)] // �ִ� �׼����̼� ��
HullOut HS(
        InputPatch<VertexOut, 4> p,
        uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    // �ϴ� �ƹ��͵� ���ϰ� �ѱ��.
    HullOut hout;
    
    hout.PosL = p[i].PosL;
    hout.NormalL = p[i].NormalL;
    hout.TangentU = p[i].TangentU;
    hout.TexC = p[i].TexC;
    
    return hout;
}

// ================ Hull Shader - Domain Shader =====================

struct DomainOut
{
    float4 PosH : SV_Position;
    float3 PosW : POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

// �� ���̴����� ������ ��� �������� ������ �ȴ�.
// ���⼭ ȣ��������� ��ȯ�ϰ� �ȼ� ���̴��� �ѱ�� ���̴�.
[domain("quad")]
DomainOut DS(
                PatchTess patchTess,
                float2 uv : SV_DomainLocation, // quad�� ���߼��� ����
                const OutputPatch<HullOut, 4> quad
            )
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    DomainOut dout;
    
    // ���߼�������

    //TexC
    float2 tex1 = lerp(quad[0].TexC, quad[1].TexC, uv.x);
    float2 tex2 = lerp(quad[2].TexC, quad[3].TexC, uv.x);
    float2 texC = lerp(tex1, tex2, uv.y);
    float4 texCf4 = mul(float4(texC, 0.f, 1.f), gTexTransform);
    texC = mul(texCf4, matData.MaterialTransform).xy;
    
    dout.TexC = texC;
    
    //NormalW 
    float3 norm1 = lerp(quad[0].NormalL, quad[1].NormalL, uv.x);
    float3 norm2 = lerp(quad[2].NormalL, quad[3].NormalL, uv.x);
    float3 normalL = lerp(norm1, norm2, uv.y);
    dout.NormalW = mul(normalL, (float3x3) gInvWorldTranspose);
    
    //TangentW
    float3 tan1 = lerp(quad[0].TangentU, quad[1].TangentU, uv.x);
    float3 tan2 = lerp(quad[2].TangentU, quad[3].TangentU, uv.x);
    float3 tangentL = lerp(tan1, tan2, uv.y);
    
    dout.TangentW = mul(tangentL, (float3x3) gInvWorldTranspose);
    
    //PosW
    float3 pos1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 pos2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 pos = lerp(pos1, pos2, uv.y);
    
    uint dispMapIndex = matData.DispMapIndex;
    float4 dispValue = gTextureMaps[dispMapIndex].SampleLevel(gSamLinearWrap, texC, 0.f);
    pos.y += dispValue.z * 0.1f;
    
    
    float4 posW = mul(float4(pos, 1.0f), gWorld);
    dout.PosW = posW.xyz;
    
    //PosH
    dout.PosH = mul(posW, gViewProj);
    
    return dout;
}

float4 PS(DomainOut pin) : SV_Target
{
    // Object Constant�� �ִ� Index�� �̿��ؼ�
    // Pipeline�� bind ��, Material ������ Texture�� �����ͼ� ���� �̾Ƴ���.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    uint normalMatIndex = matData.NormalMapIndex;
    
    // TBN�� world�� �ٲٱ� ���� Vertex Normal, Tangent�� �����´�.
    pin.NormalW = normalize(pin.NormalW);
    // UV�� �´� Texture Normal ���� �����´�.
    float4 normalMapSample = gTextureMaps[normalMatIndex].Sample(gSamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    
    // �ؽ��� �迭���� �������� �ؽ��ĸ� �����´�.
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamLinearWrap, pin.TexC);
    
#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    // ǥ�鿡�� ī�޶� ���� ���͸� ���Ѵ�.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// �ϴ� ���� �������� �����Ѵ�.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    // ������ �����ϰ� (a ä�ο� shininess ���� ����ִ� ��쵵 �ִ�.)
    const float shininess = (1.0f - roughness) * normalMapSample.a;
    // ����ü�� ä�� ����
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    // (�׸��ڴ� ���߿� �� ���̴�.)
    float3 shadowFactor = 1.0f;
    
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
