#include "Commons.hlsl"

// �Է����� Pos, Norm, TexC, TangentU �� �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
    float3 NormalL : NORMAL;
    float2 TexC : TEXCOORD;
    float3 TangentU : TANGENT;
};

// ������δ� World Pos�� ���� (Homogeneous clip space) Pos�� ������.
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
    
    // ���׸��� �����͸� �����´�
    MaterialData matData = gMaterialData[gMaterialIndex];
	
    // World Pos�� �ٲ۴�.
    float4 posW = mul(float4(vin.PosL, 1.0f), gWorld);
    vout.PosW = posW.xyz;

    // Uniform Scaling �̶�� �����ϰ� ����Ѵ�.
    // (�׷��� ������, ����ġ ����� ����ؾ� �Ѵ�.)
    //vout.NormalW = mul(vin.NormalL, (float3x3) gWorld);
    vout.NormalW = mul(vin.NormalL, (float3x3) gInvWorldTranspose);
    
    // ź��Ʈ�� ���������� ����ġ ����� ����Ͽ� ����� ��ȯ�Ѵ�.
    // ��� ���̰�, ���� ��ȯ�� ���� �̷��� ��������� ���� ���ص� �ȴ�.
    //vout.TangentW = mul(vin.TangentU, (float3x3) gWorld);
    vout.TangentW = mul(vin.TangentU, (float3x3) gInvWorldTranspose);

    // Render(Homogeneous clip space) Pos�� �ٲ۴�.
    vout.PosH = mul(posW, gViewProj);
    
    // Texture���� �־��� Transform�� �����Ų ����
    float4 texC = mul(float4(vin.TexC, 0.f, 1.f), gTexTransform);
    // World�� ���ؼ� PS�� �Ѱ��ش�.
    vout.TexC = mul(texC, matData.MaterialTransform).xy;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
    //return float4(1.f, 1.f, 1.f, 1.f);
    return float4(gScreenMap.Sample(gSamLinearWrap, pin.TexC).rgb, 1.0f);
}