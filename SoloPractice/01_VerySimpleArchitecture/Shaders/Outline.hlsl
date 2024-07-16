#include "Commons.hlsl"

struct VSInput
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float3 tanU : TANGENT;
    float2 TexCoord : TEXCOORD;
};

struct VSOutput
{
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 TexCoord : TEXCOORD0;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    uint PrimID : SV_PrimitiveID;
};

VSOutput VS(VSInput _vin)
{
    VSOutput vout;
    
    vout.posW = mul(float4(_vin.posL, 1.0f), g_matWorld).xyz;
    vout.normalW = mul(float4(_vin.normalL, 1.0f), g_invWorldTranspose).xyz;
    vout.TexCoord = _vin.TexCoord;

    return vout;
}

[maxvertexcount(12)]
void GS(
    triangleadj VSOutput _gin[6],
    uint _primID : SV_PrimitiveID,
    inout LineStream<PSInput> _lineStream
    //inout TriangleStream<PSInput> _triStream
)
{
    PSInput v0;
    PSInput v1;

    int v0Index;
    int v1Index;
    
    float3 triCenter = (_gin[0].posW + _gin[2].posW + _gin[4].posW) / 3.0f;
    float3 viewDir = normalize(g_eyePosW - triCenter);
    
    
    for (int i = 0; i < 3; i++)
    {
        float3 triNormal = _gin[(i * 2)].normalW;
        float3 adjNormal = _gin[(i * 2) + 1].normalW;
        
        float triDotV = dot(triNormal, viewDir);
        float adjDotV = dot(adjNormal, viewDir);
        
        if (triDotV * adjDotV < 0.0f)
        {
            int v0Index = (i * 2);
            int v1Index = ((i + 1) * 2) % 6;
            
            v0.pos = mul(float4(_gin[v0Index].posW, 1.0f), g_matViewProj);
            v0.PrimID = _primID;
            v0.texCoord = _gin[v0Index].TexCoord;
            v1.pos = mul(float4(_gin[v1Index].posW, 1.0f), g_matViewProj);
            v1.PrimID = _primID;
            v1.texCoord = _gin[v1Index].TexCoord;
            
            _lineStream.Append(v0);
            _lineStream.Append(v1);
        }
    }
}

float4 PS(PSInput _pin) : SV_Target
{
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}