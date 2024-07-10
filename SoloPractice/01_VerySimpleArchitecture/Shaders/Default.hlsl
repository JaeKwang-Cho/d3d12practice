// Default.hlsl from "megayuchi"

#include "Commons.hlsl"

struct VSInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float3 tanU : TANGENT;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float TanW : TANGENT;
    float2 TexCoord : TEXCOORD0;
};

PSInput VS(VSInput _vin)
{
    PSInput vout;
    
    vout.posW = mul(float4(_vin.posL, 1.0f), g_matWorld).xyz;
    vout.posH = mul(float4(vout.posW, 1.0f), g_matViewProj);
    vout.normalW = mul(float4(_vin.normalL, 1.0f), g_invWorldTranspose).xyz;
    vout.TanW = mul(float4(_vin.tanU, 1.0f), g_invWorldTranspose).xyz;
    vout.TexCoord = _vin.TexCoord;
    
    return vout;
}

float4 PS(PSInput _pin) : SV_Target
{
    // 원래 색 저장
    float4 texColor = texDiffuse.Sample(samplerDiffuse, _pin.TexCoord);

    // 노멀 정규화 하기
    _pin.normalW = normalize(_pin.normalW);
    // 표면에서 카메라까지 벡터 구하기
    float3 toEyeW = normalize(g_eyePosW - _pin.posW);
    // 간접광 계산
    float4 ambientLight = g_ambientLight * texColor;
    // 일단 임의 Material (0.5 정도)
    Material tempMat =
    {
        float4(1.0f, 1.0f, 1.0f, 1.0f),
        float3(0.05f, 0.05f, 0.05f),
        0.5f
    };
    // (그림자는 나중에)
    float shadowFactor = 1.0f;
    float4 directionalLight = ComputeLighting(g_lights, tempMat, _pin.posW, _pin.normalW, toEyeW, shadowFactor);
    
    float4 litColor = directionalLight + ambientLight;
    litColor.a = 1.f;
    
    return litColor;

}