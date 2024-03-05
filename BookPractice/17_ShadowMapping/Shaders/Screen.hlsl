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
    float4 PosH : SV_POSITION;
    float3 PosW : POSITION1;
    float3 NormalW : NORMAL;
    float3 TangentW : TANGENT;
    float2 TexC : TEXCOORD;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
    VertexOut vout = (VertexOut) 0.0f;
    
    // 머테리얼 데이터를 가져온다
    MaterialData matData = gMaterialData[gMaterialIndex];
	
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
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //return float4(1.f, 1.f, 1.f, 1.f);
    return float4(gScreenMap.Sample(gSamLinearWrap, pin.TexC).rgb, 1.0f);
}