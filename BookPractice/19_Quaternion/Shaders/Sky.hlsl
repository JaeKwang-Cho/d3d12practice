//=============================================================================
// Sky.hlsl by Frank Luna (C) 2011 All Rights Reserved.
// �� �ָ�(?) �ϴ��� �׸��� ������ �ϴ� ���̴���.
//=============================================================================

#include "Commons.hlsl"

struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
};

struct VertexOut
{
    float4 PosH : SV_Position;
    float3 PosL : POSITION;
};

VertexOut VS(VertexIn _vin)
{
    VertexOut vout;
    
    // ������ġ�� ��� ���ͷ� �״�� ����Ѵ�.
    vout.PosL = _vin.PosL;
    
    // ������� ã�� �ȼ��� ���� ������, ����� �ٲٰ�
    float4 posW = mul(float4(_vin.PosL, 1.f), gWorld);
    
    // �װ��� ī�޶� ��ġ�� ���� �����̵��� �����Ѵ�.
    posW.xyz += gEyePosW;
    
    // ȭ�鿡 ������ z����(depth�� ���߿� ������� ������
    // w ������ ������ ��) �׻� 1(����ָ�) �������� �Ѵ�.
    vout.PosH = mul(posW, gViewProj).xyww;
    
    return vout;
}

float4 PS(VertexOut _pin) : SV_Target
{
    // �����س��� ��� ���ͷ� �ȼ��� ���Ƴ���.
    return gCubeMap.Sample(gSamLinearWrap, _pin.PosL);
}