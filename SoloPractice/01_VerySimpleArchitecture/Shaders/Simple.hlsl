// Simple.hlsl from "megayuchi"

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

cbuffer CONSTANT_BUFFER_DEFAULT : register(b0)
{
    float4 g_Offset;
};

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 TexCoord : TEXCOORD0;
};

PSInput VS(VSInput _vin)
{
    PSInput vout;
    
    vout.color = _vin.color;
    vout.pos = float4(_vin.pos, 1.0f);
    vout.pos.xy += g_Offset.xy;
    vout.TexCoord = _vin.TexCoord;

    return vout;
}

float4 PS(PSInput _pin) : SV_Target
{
    float4 texColor = texDiffuse.Sample(samplerDiffuse, _pin.TexCoord);
    return texColor * _pin.color;
}