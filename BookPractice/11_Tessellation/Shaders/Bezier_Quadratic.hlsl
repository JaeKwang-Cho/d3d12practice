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
    float EdgeTess[4] : SV_TessFactor;
    float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 9> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
	
	// uniform tessellation�� 25�� ���ش�.

    pt.EdgeTess[0] = 16;
    pt.EdgeTess[1] = 16;
    pt.EdgeTess[2] = 16;
    pt.EdgeTess[3] = 16;
	
    pt.InsideTess[0] = 16;
    pt.InsideTess[1] = 16;
	
    return pt;
}
// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};

// �� ���̴� ��Ʈ�� ���� ��ǥ���(�ຯȯ ��)�� �ٲٴµ� ���δ�.
// ���� ��� �׳� �簢���� ���� ť��(3�� ���ö��� ���)���� �ٲ� ��
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(9)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 9> p,
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
// 3�� ������ � ����� �� ���� ���ִ� ���� ��Ÿ�� ���̽ý� �̴�.
float3 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float3(invT * invT,
                   2.0f * t * invT,
                   t * t);
}
// 3�� ������ ��� ������, ����� ����� ���ؼ� �� ���� ���̽ý��� ������ ������ ��ġ�� �� ��ġ�� �̿��ؼ� �ش� ���� ���Ѵ�.
float3 QuadraticBezierSum(const OutputPatch<HullOut, 9> bezpatch, float3 basisU, float3 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum = basisV.x * (basisU.x * bezpatch[0].PosL + basisU.y * bezpatch[1].PosL + basisU.z * bezpatch[2].PosL);
    sum += basisV.y * (basisU.x * bezpatch[3].PosL + basisU.y * bezpatch[4].PosL + basisU.z * bezpatch[5].PosL);
    sum += basisV.z * (basisU.x * bezpatch[6].PosL + basisU.y * bezpatch[7].PosL + basisU.z * bezpatch[8].PosL);

    return sum;
}
// �̰� ��� ����� �������ִ� ���� ������ ���̽ý� ���Լ� �����̴�.
float3 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float3(-2 * invT,
                   2 * invT - 2 * t,
                   2 * t);
}

// ���� ���̽ý��� ������ �̿��ؼ� �� ���̴����� ������ ������ ������ ��鿡 ��ġ�ϰ� �Ѵ�.
[domain("quad")]
DomainOut DS(PatchTess patchTess,
             float2 uv : SV_DomainLocation,
             const OutputPatch<HullOut, 9> bezPatch)
{
    DomainOut dout;
	
    float3 basisU = BernsteinBasis(uv.x);
    float3 basisV = BernsteinBasis(uv.y);

    float3 p = QuadraticBezierSum(bezPatch, basisU, basisV);
	
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	
    return dout;
}
// ============== Pixel Shader ==================
float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

