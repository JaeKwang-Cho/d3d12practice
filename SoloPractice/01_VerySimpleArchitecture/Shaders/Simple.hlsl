// Simple.hlsl from "megayuchi"

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
    
    float4 posW = mul(float4(_vin.pos, 1.0f), g_matWorld);
    vout.pos = mul(posW, g_matViewProj);
    //matrix matViewProj = mul(g_matView, g_matProj); // view x proj
    //matrix matWorldViewProj = mul(g_matWorld, matViewProj); // world x view x proj
    //vout.pos = mul(float4(_vin.pos, 1.0f), matWorldViewProj); // pojtected vertex = vertex x world x view x proj
    
    vout.color = _vin.color;
    vout.TexCoord = _vin.TexCoord;

    return vout;
}

float4 PS(PSInput _pin) : SV_Target
{
    float4 texColor = texDiffuse.Sample(samplerDiffuse, _pin.TexCoord);
    return texColor * _pin.color;
}