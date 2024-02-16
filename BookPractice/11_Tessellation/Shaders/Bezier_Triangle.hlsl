//***************************************************************************************
// BezierTessellation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates hardware tessellating a Bezier patch.
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
struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

// �ƹ��͵� ���ϰ� �׳� �Ѱ��ش�.
VertexOut VS(VertexIn vin)
{
    VertexOut vout;
	
    vout.PosL = vin.PosL;

    return vout;
}
// ============== Hull Shader - Constant Hull Shader (input : Patch Control Point) =================
struct PatchTess
{
    float EdgeTess[3] : SV_TessFactor;
    float InsideTess[1] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 3> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
	
	// uniform tessellation�� 25�� ���ش�.

    pt.EdgeTess[0] = 16;
    pt.EdgeTess[1] = 16;
    pt.EdgeTess[2] = 16;
	
    pt.InsideTess[0] = 16;
	
    return pt;
}
// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};

// �� ���̴� ��Ʈ�� ���� ��ǥ���(�ຯȯ ��)�� �ٲٴµ� ���δ�.
// ���� ��� �׳� �簢���� ���� ť��(3�� ���ö��� ���)���� �ٲ� ��
[domain("tri")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(3)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 3> p,
           uint i : SV_OutputControlPointID,
           uint patchId : SV_PrimitiveID)
{
    HullOut hout;
	
    hout.PosL = p[i].PosL;
	
    return hout;
}
// ================ Hull Shader - Domain Shader =====================
struct DomainOut
{
    float4 PosH : SV_POSITION;
};

// 3�� ������ �ﰢ���� �����߽� ��ǥ�� �׸��� ���� ���� ���� ���̴�.
float3 CubicBezierTriangle(const OutputPatch<HullOut, 3> bezpatch, float3 centroidUVW)
{
    float3 sum = float3(0.f, 0.f, 0.f);
    float3 p0 = bezpatch[0].PosL;
    float3 p1 = bezpatch[1].PosL;
    float3 p2 = bezpatch[2].PosL;
    float cu = centroidUVW.x;
    float cv = centroidUVW.y;
    float cw = centroidUVW.z;
    
    sum = (p0 * p0 * p0 * cu * cu * cu) + (p1 * p1 * p1 * cv * cv * cv) + (p2 * p2 * p2 * cw * cw * cw);
    
    sum += 3.f * (p0 * p1 * p1 * cu * cv * cv);
    sum += 3.f * (p1 * p1 * p2 * cv * cv * cw);
    sum += 3.f * (p0 * p0 * p1 * cu * cu * cv);
    sum += 3.f * (p1 * p2 * p2 * cv * cw * cw);
    sum += 3.f * (p0 * p0 * p2 * cu * cu * cw);
    sum += 3.f * (p0 * p0 * p2 * cu * cw * cw);
    sum += 6.f * (p0 * p1 * p2 * cu * cv * cw);

    return sum;
}

[domain("tri")]
DomainOut DS(PatchTess patchTess,
             float3 uvw : SV_DomainLocation,
             const OutputPatch<HullOut, 3> bezPatch)
{
    DomainOut dout;
    
    float len0 = length(bezPatch[0].PosL);
    float len1 = length(bezPatch[1].PosL);
    float len2 = length(bezPatch[2].PosL);
    float sumLen = len0 + len1 + len2;
    
    float3 p = CubicBezierTriangle(bezPatch, uvw);
    p /= sumLen;
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	
    return dout;
}
// ============== Pixel Shader ==================
float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

