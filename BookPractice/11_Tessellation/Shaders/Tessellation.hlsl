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
#if PRAC1 || PRAC2
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess[1] : SV_InsideTessFactor;
#else
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
#endif
};

PatchTess ConstantHS(
#if PRAC1 || PRAC2
        InputPatch<VertexOut, 3> patch, 
#else
        InputPatch<VertexOut, 4> patch,
#endif
        uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
#if PRAC1 || PRAC2
    // patch�� ���� ������ ���Ѵ�.
    float3 centerL = 0.3334f * (patch[0].PosL + patch[1].PosL + patch[2].PosL);
#else
    // patch�� ���� ������ ���Ѵ�.
    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
#endif
    

    float3 centerW = mul(float4(centerL, 1.f), gWorld).xyz;
    // ī�޶�� �Ÿ��� ���Ѵ�.
    float d = distance(centerW, gEyePosW);
    
    // ī�޶���� �Ÿ��� ���� �׼����̼� ������ ���Ѵ�.
    // �ִ� 64�� �׼����̼� �Ѵ�. (�Ÿ� 80�� ���� ���� �� ��)
    const float d0 = 20.f;
    const float d1 = 100.f;
    
    float tess = 64.f * saturate((d1 - d) / (d1 - d0));
    
    // �� �Ȱ��� �׼�����Ʈ �Ѵ�.
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
	
    pt.InsideTess[0] = tess;
    
#if !PRAC1 && !PRAC2
    pt.EdgeTess[3] = tess;
    pt.InsideTess[1] = tess;
#endif
	
    return pt;
}

// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};
#if PRAC1 || PRAC2
[domain("tri")]
[outputcontrolpoints(3)] // ��ġ�� ���� �潦�̴��� ������ ������ ����
#else
[domain("quad")] // tri, quad, isoline
[outputcontrolpoints(4)] // ��ġ�� ���� �潦�̴��� ������ ������ ����
#endif    
[partitioning("integer")] // tess factor�� ������ ���� �۵�
[outputtopology("triangle_cw")] // �ð� ����
[patchconstantfunc("ConstantHS")] // �׼����̼� ����� ���� CHS �̸�
[maxtessfactor(64.f)] // �ִ� �׼����̼� ��
HullOut HS(
#if PRAC1 || PRAC2
        InputPatch<VertexOut, 3> p, 
#else
        InputPatch<VertexOut, 4> p,
#endif
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
#if PRAC1 || PRAC2
[domain("tri")]
#else
[domain("quad")]
#endif
DomainOut DS(
                PatchTess patchTess,
#if PRAC1 || PRAC2
                float3 uvw : SV_DomainLocation, // tri�� �����߽� ��ǥ
                const OutputPatch<HullOut, 3> quad
#else
                float2 uv : SV_DomainLocation, // quad�� ���߼��� ����
                const OutputPatch<HullOut, 4> quad
#endif
            )
{
    DomainOut dout;
    
#if PRAC1 || PRAC2
    // �����߽� ��ǥ
    float3 p = (quad[0].PosL * uvw.x + quad[1].PosL * uvw.y + quad[2].PosL * uvw.z);
#else
    // ���߼�������
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);
#endif
    
#if PRAC2
    p = normalize(p);
    float patchLen = length(quad[0].PosL);
    p *= patchLen;
#else
	// ���⼭ ������ ���� ���ó�� ������ش�.
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
#endif
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