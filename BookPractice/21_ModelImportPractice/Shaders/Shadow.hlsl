//***************************************************************************************
// Shadows.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// ���� �������� ���� ���۸� �׸��� ���̴� �ڵ��.
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
        // ��ȯ�Ҷ� �յ� ��ȯ���� �����Ѵ�.
        // �׷��� ������ ����� �ٲܶ�, inverse-transpose�� ����ؾ� �Ѵ�.
        
        posL += weights[i] * mul(float4(vin.PosL, 1.f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
    }
    
    vin.PosL = posL;
#endif
    
    // World Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.
    vout.PosH = mul(posW, gViewProj);
	
	// ��� ���� �Ӽ��� ���߿�, �ﰢ���� ���� �����ȴ�.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
	
    return vout;
}

// ���� Ŭ������ ����Ѵ�. �ؽ��ĸ� ���ø��� �ʿ䰡 ���� ������
// �ȼ� ���̴��� ��� �ȴ�.
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