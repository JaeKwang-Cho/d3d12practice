//***************************************************************************************
// displacement.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Default shader, currently supports lighting.
//***************************************************************************************

#include "Commons.hlsl"

// 입력으로 Pos, Norm, TexC, TangentU 을 받는다.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

// 출력으로는 World Pos와 동차 (Homogeneous clip space) Pos로 나눈다.
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
    
    // patch의 월드 중점을 구한다.
    float3 centerL = 0.25f * (patch[0].PosL 
    + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    

    float3 centerW = mul(float4(centerL, 1.f), gWorld).xyz;
    // 카메라와 거리를 구한다.
    float d = distance(centerW, gEyePosW);
    
    // 카메라와의 거리에 따라서 테셀레이션 정도를 구한다.
    // 최대 64번 테셀레이션 한다. (거리 80이 제일 많이 할 때)
    const float d0 = 20.f;
    const float d1 = 100.f;
    
    //float tess = 64.f * saturate((d1 - d) / (d1 - d0));
    float tess = 64;
    // 다 똑같이 테셀레이트 한다.
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
[outputcontrolpoints(4)] // 패치에 대해 헐쉐이더를 적용할 제어점 개수
[partitioning("integer")] // tess factor가 정수일 때만 작동
[outputtopology("triangle_cw")] // 시계 방향
[patchconstantfunc("ConstantHS")] // 테셀레이션 계수를 받을 CHS 이름
[maxtessfactor(64.f)] // 최대 테셀레이션 양
HullOut HS(
        InputPatch<VertexOut, 4> p,
        uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    // 일단 아무것도 안하고 넘긴다.
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

// 헐 쉐이더에서 생성된 모든 정점마다 실행이 된다.
// 여기서 호모공간으로 변환하고 픽셀 쉐이더로 넘기는 것이다.
[domain("quad")]
DomainOut DS(
                PatchTess patchTess,
                float2 uv : SV_DomainLocation, // quad는 이중선형 보간
                const OutputPatch<HullOut, 4> quad
            )
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    DomainOut dout;
    
    // 이중선형보간

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
    // Object Constant에 있는 Index를 이용해서
    // Pipeline에 bind 된, Material 정보와 Texture를 가져와서 색을 뽑아낸다.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    uint normalMatIndex = matData.NormalMapIndex;
    
    // TBN을 world로 바꾸기 위한 Vertex Normal, Tangent를 가져온다.
    pin.NormalW = normalize(pin.NormalW);
    // UV에 맞는 Texture Normal 값을 가져온다.
    float4 normalMapSample = gTextureMaps[normalMatIndex].Sample(gSamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    
    // 텍스쳐 배열에서 동적으로 텍스쳐를 가져온다.
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamLinearWrap, pin.TexC);
    
#ifdef ALPHA_TEST
    // 알파 값이 일정값 이하면 그냥 잘라버린다.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    // 표면에서 카메라 까지 벡터를 구한다.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// 일단 냅다 간접광을 설정한다.
    float4 ambient = gAmbientLight * diffuseAlbedo;

    // 광택을 설정하고 (a 채널에 shininess 값이 들어있는 경우도 있다.)
    const float shininess = (1.0f - roughness) * normalMapSample.a;
    // 구조체를 채운 다음
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
    // (그림자는 나중에 할 것이다.)
    float3 shadowFactor = 1.0f;
    
    // 이전에 정의 했던 식을 이용해서 넘긴다.    
    float4 directLight = ComputeLighting(gLights, mat, pin.PosW,
        bumpedNormalW, toEyeW, shadowFactor);
    // 최종 색을 결정하고
    float4 litColor = ambient + directLight;
    

    // # 주변 환경 반사를 한다.
    // 카메라 위치에서, 현재 픽셀이 위치한곳의 World 법선을 타고
    // 어떻게 뻗어나갈지 reflect()를 이용해 구한다.
    float3 r = reflect(-toEyeW, bumpedNormalW);
    // 이 벡터를 그대로 lookup vector로 사용하면 정확하지 않다. 
    // 카메라에서 쏘는 것처럼 lookup을 하기 때문이다.
    // 하늘은 구 형태이고 크기가 엄청 커서(1000) 어색함이 거의 없지만.
    // 방과 같은 평면을 가지는 공간이라면 어색함이 생길 수 있다.
    float4 reflectionColor = gCubeMap.Sample(gSamLinearWrap, r);
    // 반사광은 Fresnel을 사용하여 가저온다.
    float3 fresnelFactor = SchlickFresnel(fresnelR0, bumpedNormalW, r);
    // 거칠기도 반영한다.
    litColor.rgb += shininess * fresnelFactor * reflectionColor.rgb;

    // diffuse albedo에서 alpha값을 가져온다.
    litColor.a = diffuseAlbedo.a;

    
    // 출력한다.
    return litColor;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blend.hlsl" /Od /D "ALPHA_TEST"="1"/Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\07_Blending\Shaders\07_Blending_PS_Alpha.asm"
