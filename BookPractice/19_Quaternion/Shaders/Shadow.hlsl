//***************************************************************************************
// Shadows.hlsl by Frank Luna (C) 2015 All Rights Reserved.
// 광원 시점에서 깊이 버퍼를 그리는 쉐이더 코드다.
//***************************************************************************************

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float2 TexC : TEXCOORD;
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