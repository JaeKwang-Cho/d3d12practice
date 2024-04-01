//***************************************************************************************
// Shadows.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// 광원 시점에서 깊이 버퍼를 그리는 쉐이더 코드다.
//***************************************************************************************

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
#ifdef SKINNED
    float3 BoneWeights : WEIGHTS;
    uint4 BoneIndices  : BONEINDICES;
#endif
};

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

    MaterialData matData = gMaterialData[gMaterialIndex];
    
#ifdef SKINNED
    float weights[4] = {0.f, 0.f, 0.f, 0.f};
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.f - weights[0] - weights[1] - weights[2];
    
    float3 posL = float3(0.f, 0.f, 0.f);
    for(int i = 0; i < 4; i++){
        // 변환할때 균등 변환임을 가정한다.
        // 그렇지 않으면 노멀을 바꿀때, inverse-transpose를 사용해야 한다.
        
        posL += weights[i] * mul(float4(vin.PosL, 1.f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
    }
    
    vin.PosL = posL;
#endif
    
    // World Pos로 바꾼다.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    // Render(Homogeneous clip space) Pos로 바꾼다.
    vout.PosH = mul(posW, gViewProj);
	
	// 출력 정점 속성은 나중에, 삼각형을 따라 보간된다.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
	
    return vout;
}

// 알파 클립에만 사용한다. 텍스쳐를 샘플링할 필요가 없는 도형은
// 픽셀 쉐이더가 없어도 된다.
void PS(VertexOut pin)
{
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
	
    diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gSamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    clip(diffuseAlbedo.a - 0.1f);
#endif
}