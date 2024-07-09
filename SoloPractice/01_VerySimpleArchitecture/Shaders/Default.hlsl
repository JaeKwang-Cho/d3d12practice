// Default.hlsl from "megayuchi"

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

cbuffer CONSTANT_BUFFER_FRAME : register(b1)
{
    matrix g_matView;
    matrix g_matProj;
    matrix g_matViewProj;
};

cbuffer CONSTANT_BUFFER_OBJECT : register(b0)
{
    matrix g_matWorld;
    matrix g_invWorldTranspose;
};

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
    float4 texColor = texDiffuse.Sample(samplerDiffuse, _pin.TexCoord);
    return texColor;

}