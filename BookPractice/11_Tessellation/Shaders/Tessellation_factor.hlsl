//***************************************************************************************
// Tessellation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

// �׳� ��� �ؽ��ĸ� �����´�.
Texture2D gDiffuseMap : register(t0);

SamplerState gSamPointWrap : register(s0);
SamplerState gSamPointClamp : register(s1);
SamplerState gSamPointMirror : register(s2);
SamplerState gSamPointBorder : register(s3);
SamplerState gSamLinearWrap : register(s4);
SamplerState gSamLinearClamp : register(s5);
SamplerState gSamLinearMirror : register(s6);
SamplerState gSamLinearBorder : register(s7);
SamplerState gSamAnisotropicWrap : register(s8);
SamplerState gSamAnisotropicClamp : register(s9);

// Material ���� �޶����� Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvWorldTranspose;
    float4x4 gTexTransform;
};

// ������ ���� �޶����� Constant Buffer
cbuffer cbPass : register(b1)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    
    float3 gEyePosW;
    float cbPerPassad1;
    
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    
    float gNearZ;
    float gFarZ;
    float gTotalTime;
    float gDeltaTime;
    
    float4 gAmbientLight;
    
    // fog ������ pass�� �Ѿ�´�
    float4 gFogColor;
    
    float gFogStart;
    float gFogRange;
    float2 cbPerPassPad2;

    //Light gLights[MaxLights];
};

cbuffer cbMaterial : register(b2)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

// ============ Vertex Shader ===============

// Patch Point ������ �޴´�.
struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

// VS������ �ƹ��͵� ���� �ʴ´�.
VertexOut VS(VertexIn _vin)
{
    VertexOut vout;
    vout.PosL = _vin.PosL;

    return vout;
}

// ============== Hull Shader - Constant Hull Shader (input : Patch Control Point) =================

struct PatchTess
{
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(
        InputPatch<VertexOut, 4> patch,
        uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    pt.EdgeTess[0] = 3;
    pt.EdgeTess[1] = 3;
    pt.EdgeTess[2] = 3;
    pt.EdgeTess[3] = 3;
	
    pt.InsideTess[0] = 16;
    pt.InsideTess[1] = 16;
	
    return pt;
}

// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};
[domain("quad")] // tri, quad, isoline
[outputcontrolpoints(4)] // ��ġ�� ���� �潦�̴��� ������ ������ ����
[partitioning("integer")] // tess factor�� ������ ���� �۵�
[outputtopology("triangle_cw")] // �ð� ����
[patchconstantfunc("ConstantHS")] // �׼����̼� ����� ���� CHS �̸�
[maxtessfactor(64.f)] // �ִ� �׼����̼� ��
HullOut HS(
        InputPatch<VertexOut, 4> p,
        uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    // �ϴ� �ƹ��͵� ���ϰ� �ѱ��.
    HullOut hout;
    
    hout.PosL = p[i].PosL;
    
    return hout;
}

// ================ Hull Shader - Domain Shader =====================

struct DomainOut
{
    float4 PosH : SV_Position;
};

// �� ���̴����� ������ ��� �������� ������ �ȴ�.
// ���⼭ ȣ��������� ��ȯ�ϰ� �ȼ� ���̴��� �ѱ�� ���̴�.
[domain("quad")]
DomainOut DS(
                PatchTess patchTess,
                float2 uv : SV_DomainLocation, // quad�� ���߼��� ����
                const OutputPatch<HullOut, 4> quad
            )
{
    DomainOut dout;

    // ���߼�������
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);

	// ȣ�� �������� ��ȯ�� ����
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	// �ȼ� ���̴��� �ѱ��.
    return dout;
    
}

// ============== Pixel Shader ==================

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}