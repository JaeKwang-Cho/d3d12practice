//***************************************************************************************
// DrawNormals.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// Defaults for number of lights.
#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 0
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

#include "Commons.hlsl"

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

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
    VertexOut vout = (VertexOut) 0.0f;

	// ���׸��� �����͸� �����´�
    MaterialData matData = gMaterialData[gMaterialIndex];
    
#ifdef SKINNED
    float weights[4] = {0.f, 0.f, 0.f, 0.f};
    weights[0] = vin.BoneWeights.x;
    weights[1] = vin.BoneWeights.y;
    weights[2] = vin.BoneWeights.z;
    weights[3] = 1.f - weights[0] - weights[1] - weights[2];
    
    float3 posL = float3(0.f, 0.f, 0.f);
    float3 normalL = float3(0.f, 0.f, 0.f);
    float3 tangentU = float3(0.f, 0.f, 0.f);
    for(int i = 0; i < 4; i++){
        // ��ȯ�Ҷ� �յ� ��ȯ���� �����Ѵ�.
        // �׷��� ������ ����� �ٲܶ�, inverse-transpose�� ����ؾ� �Ѵ�.
        
        posL += weights[i] * mul(float4(vin.PosL, 1.f), gBoneTransforms[vin.BoneIndices[i]]).xyz;
        normalL += weights[i] * mul(vin.NormalL, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
        tangentU += weights[i] * mul(vin.TangentU.xyz, (float3x3)gBoneTransforms[vin.BoneIndices[i]]);
    }
    
    vin.PosL = posL;
    vin.NormalL = normalL;
    vin.TangentU.xyz = tangentU;
#endif
	
    // Uniform Scaling �̶�� �����ϰ� ����Ѵ�.
    // (�׷��� ������, ����ġ ����� ����ؾ� �Ѵ�.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);
    
    // ź��Ʈ�� ���������� ����ġ ����� ����Ͽ� ����� ��ȯ�Ѵ�.
    // ��� ���̰�, ���� ��ȯ�� ���� �̷��� ��������� ���� ���ص� �ȴ�.
    //vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosH = mul(posW, gViewProj);
	
	// �ﰢ�� ���� �����ǵ��� �Ѵ�.
    float4 texC = mul(float4(vin.TexC, 0.0f, 1.0f), gTexTransform);
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
	
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	// Object Constant�� �ִ� Index�� �̿��ؼ�
    // Pipeline�� bind ��, Material ������ Texture�� �����ͼ� ���� �̾Ƴ���.
    MaterialData matData = gMaterialData[gMaterialIndex];
    float4 diffuseAlbedo = matData.DiffuseAlbedo;
    uint diffuseMapIndex = matData.DiffuseMapIndex;
    uint normalMapIndex = matData.NormalMapIndex;
	
    // �ؽ��� �迭���� �������� �ؽ��ĸ� �����´�.
    diffuseAlbedo *= gTextureMaps[diffuseMapIndex].Sample(gSamAnisotropicWrap, pin.TexC);

#ifdef ALPHA_TEST
    // ���� ���� ������ ���ϸ� �׳� �߶������.
    clip(diffuseAlbedo.a - 0.1f);
#endif

	// Ȥ�� �𸣴� �ٽ� ����ȭ ���ְ�
    pin.NormalW = normalize(pin.NormalW);
	
    // ������ vertex�� normal�� SSAO�� ����� ���̴�.

    // view ������ �ִ� normal�� ����Ѵ�. 
    float3 normalV = mul(pin.NormalW, (float3x3) gView);
    return float4(normalV, 0.0f);
}


