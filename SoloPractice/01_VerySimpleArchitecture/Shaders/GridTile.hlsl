#include "Commons.hlsl"

struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
};

struct VSOutput
{
    float3 pos : POSITION;
    float4 color : COLOR;
    float2 texCoord : TEXCOORD;
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
    
    vout.pos = _vin.pos;
    vout.color = _vin.color;
    vout.texCoord = _vin.texCoord;

    return vout;
}

//  축 수직 양방향으로 50개 그리기
[maxvertexcount(2)]
void GS(
    point VSOutput _gin[1],
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