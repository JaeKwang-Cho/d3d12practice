//***************************************************************************************
// BezierTessellation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Demonstrates hardware tessellating a Bezier patch.
//***************************************************************************************

// 그냥 흰색 텍스쳐를 가져온다.
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

// Material 마다 달라지는 Constant Buffer
cbuffer cbPerObject : register(b0)
{
    float4x4 gWorld;
    float4x4 gInvWorldTranspose;
    float4x4 gTexTransform;
};

// 프레임 마다 달라지는 Constant Buffer
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
    
    // fog 정보는 pass로 넘어온다
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

// 아무것도 안하고 그냥 넘겨준다.
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

PatchTess ConstantHS(InputPatch<VertexOut, 16> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
	
	// uniform tessellation을 25번 해준다.

    pt.EdgeTess[0] = 25;
    pt.EdgeTess[1] = 25;
    pt.EdgeTess[2] = 25;
    pt.EdgeTess[3] = 25;
	
    pt.InsideTess[0] = 25;
    pt.InsideTess[1] = 25;
	
    return pt;
}
// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};

// 헐 쉐이더 파트는 보통 좌표기반(축변환 등)을 바꾸는데 쓰인다.
// 예를 들면 그냥 사각형을 바이 큐빅(3차 스플라인 평면)으로 바꿀 때
[domain("quad")]
[partitioning("integer")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(16)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 16> p,
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
// 3차 베지어 곡선 계산을 더 쉽게 해주는 베른 슈타인 베이시스 이다.
float4 BernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(invT * invT * invT,
                   3.0f * t * invT * invT,
                   3.0f * t * t * invT,
                   t * t * t);
}
// 3차 베지어 곡선을 가지고, 곡면을 만들기 위해서 축 별로 베이시스와 생성된 점들의 패치위 축 위치을 이용해서 해당 점을 구한다.
float3 CubicBezierSum(const OutputPatch<HullOut, 16> bezpatch, float4 basisU, float4 basisV)
{
    float3 sum = float3(0.0f, 0.0f, 0.0f);
    sum = basisV.x * (basisU.x * bezpatch[0].PosL + basisU.y * bezpatch[1].PosL + basisU.z * bezpatch[2].PosL + basisU.w * bezpatch[3].PosL);
    sum += basisV.y * (basisU.x * bezpatch[4].PosL + basisU.y * bezpatch[5].PosL + basisU.z * bezpatch[6].PosL + basisU.w * bezpatch[7].PosL);
    sum += basisV.z * (basisU.x * bezpatch[8].PosL + basisU.y * bezpatch[9].PosL + basisU.z * bezpatch[10].PosL + basisU.w * bezpatch[11].PosL);
    sum += basisV.w * (basisU.x * bezpatch[12].PosL + basisU.y * bezpatch[13].PosL + basisU.z * bezpatch[14].PosL + basisU.w * bezpatch[15].PosL);

    return sum;
}
// 이건 노멀 계산을 쉽게해주는 베른 슈탸인 베이시스 도함수 버전이다.
float4 dBernsteinBasis(float t)
{
    float invT = 1.0f - t;

    return float4(-3 * invT * invT,
                   3 * invT * invT - 6 * t * invT,
                   6 * t * invT - 3 * t * t,
                   3 * t * t);
}

// 위에 베이시스와 계산식을 이용해서 헐 쉐이더에서 생성된 점들을 베지어 곡면에 위치하게 한다.
[domain("quad")]
DomainOut DS(PatchTess patchTess,
             float2 uv : SV_DomainLocation,
             const OutputPatch<HullOut, 16> bezPatch)
{
    DomainOut dout;
	
    float4 basisU = BernsteinBasis(uv.x);
    float4 basisV = BernsteinBasis(uv.y);

    float3 p = CubicBezierSum(bezPatch, basisU, basisV);
	
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	
    return dout;
}
// ============== Pixel Shader ==================
float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}

