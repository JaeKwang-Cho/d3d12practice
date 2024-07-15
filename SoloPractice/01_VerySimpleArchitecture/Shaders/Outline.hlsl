#include "Commons.hlsl"

struct VSInput
{
    float3 posL : POSITION;
    float4 color : COLOR;
    float3 normalL : NORMAL;
};

struct VSOutput
{
    float3 posW : POSITION;
    float4 color : COLOR;
    float3 normalW : NORMAL;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VSOutput VS(VSInput _vin)
{
    VSOutput vout;
    
    vout.posW = mul(float4(_vin.posL, 1.0f), g_matWorld);
    vout.color = _vin.color;
    vout.normalW = mul(float4(_vin.normalL, 1.0f), g_invWorldTranspose);

    return vout;
}

//  축 수직 양방향으로 50개 그리기
[maxvertexcount(12)]
void GS(
    triangle VSOutput _gin[4],
    uint _primID : SV_PrimitiveID,
    inout LineStream<PSInput> _lineStream
)
{
    PSInput gout0;
    PSInput gout1;
    
    float4 v1 = float4(_gin[0].pos, 1.f);
    float4 v2 = float4(_gin[0].pos, 1.f);
   
    [branch]
    if (_primID % 2 == 0)
    {
        v1.z = -125.f;
        v2.z = 125.f;
    }
    else
    {
        v1.x = -125.f;
        v2.x = 125.f;
    }
    
    float4 posW = mul(v1, g_matWorld);
    gout0.pos = mul(posW, g_matViewProj);
    gout0.color = _gin[0].color;
    gout0.texCoord = _gin[0].texCoord;
    gout0.PrimID = _primID;

    posW = mul(v2, g_matWorld);
    gout1.pos = mul(posW, g_matViewProj);
    gout1.color = _gin[0].color;
    gout1.texCoord = _gin[0].texCoord;
    gout1.PrimID = _primID;
  
    _lineStream.Append(gout0);
    _lineStream.Append(gout1);
}

float4 PS(PSInput _pin) : SV_Target
{
    return _pin.color;
}