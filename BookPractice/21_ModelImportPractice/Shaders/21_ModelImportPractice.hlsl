//***************************************************************************************
// 20_Animation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
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
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices : BONEINDICES;
#endif
};

// 출력으로는 World Pos와 동차 (Homogeneous clip space) Pos로 나눈다.
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
    
    // 머테리얼 데이터를 가져온다
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
        // Uniform scaling 이라고 가정한다.
        // 그렇지 않으면 inverse-transpose를 곱해야한다.

        posL += weights[i] * mul(float4(vin.PosL, 1.0f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        TangentU += weights[i] * mul(vin.TangentU.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
    }

    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentU.xyz = TangentU;
#endif
	
    // World Pos로 바꾼다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling 이라고 가정하고 계산한다.
    // (그렇지 않으면, 역전치 행렬을 사용해야 한다.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);
    
    // 탄젠트도 마찬가지로 역전치 행렬을 사용하여 월드로 변환한다.
    // 행렬 곱이고, 기저 변환에 따라 이렇게 월드행렬을 먼저 곱해도 된다.
    //vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos로 바꾼다.
    vout.PosH = mul(posW, gViewProj);
    
    // Texture에게 주어진 Transform을 적용시킨 다음
    float4 texC = mul(float4(vin.TexC, 0.f, 1.f), gTexTransform);
    // World를 곱해서 PS로 넘겨준다.
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
    
    // PassCB에서 넘겨준 행렬로, 투영공간으로 변환한다.
    vout.ShadowPosH = mul(posW, gShadowMat);
    
    // 생성된 Ssao Map위 값을 바로 찾을 수 있는 viewProjTexMat을 이용한다.
    vout.SsaoPosH = mul(posW, gViewProjTexMat);
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    // Object Constant에 있는 Index를 이용해서
    // Pipeline에 bind 된, Material 정보와 Texture를 가져와서 색을 뽑아낸다.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    float3 fresnelR0 = matData.FresnelR0;
    float roughness = matData.Roughness;
    uint diffuseTexIndex = matData.DiffuseMapIndex;
    uint normalMatIndex = matData.NormalMapIndex;
    
    // 텍스쳐 배열에서 동적으로 텍스쳐를 가져온다.
    diffuseAlbedo *= gTextureMaps[diffuseTexIndex].Sample(gSamAnisotropicWrap, pin.TexC);
        
#ifdef ALPHA_TEST
    // 알파 값이 일정값 이하면 그냥 잘라버린다.
    clip(diffuseAlbedo.a - 0.1f);
#endif
    
    // TBN을 world로 바꾸기 위한 Vertex Normal, Tangent를 가져온다.
    pin.NormalW = normalize(pin.NormalW);
    // UV에 맞는 Texture Normal 값을 가져온다.
    float4 normalMapSample = gTextureMaps[normalMatIndex].Sample(gSamAnisotropicWrap, pin.TexC);
    float3 bumpedNormalW = NormalSampleToWorldSpace(normalMapSample.rgb, pin.NormalW, pin.TangentW);
    
    // 표면에서 카메라 까지 벡터를 구한다.
    float3 toEyeW = normalize(gEyePosW - pin.PosW);

	// 이제 간접광을 Ssao의 결과로 바뀌게 해준다.
    // 일단 w로 나눠서(남아 있던 Homogenous공간의 Depth) 공간 변화를 마무리 해주고
    pin.SsaoPosH /= pin.SsaoPosH.w;
    // Ssao Map에서 결과를 가져온다.
    float ambientAccss = gSsaoMap.Sample(gSamLinearClamp, pin.SsaoPosH.xy, 0.f).r;
    
    float4 ambient = gAmbientLight * diffuseAlbedo * ambientAccss;
    
    // 일단은 첫번째 광원에 대해서만, 그림자를 설정한다.
    float3 shadowFactor = float3(1.f, 1.f, 1.f);
    shadowFactor[0] = CalcShadowFactor(pin.ShadowPosH);

    // 광택을 설정하고 (a 채널에 shininess 값이 들어있는 경우도 있다.)
    const float shininess = (1.0f - roughness) * normalMapSample.a;
    // 구조체를 채운 다음
    Material mat = { diffuseAlbedo, fresnelR0, shininess };
        
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
