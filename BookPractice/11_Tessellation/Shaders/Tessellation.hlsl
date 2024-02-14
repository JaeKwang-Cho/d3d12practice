//***************************************************************************************
// Tessellation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
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

// Patch Point 정보만 받는다.
struct VertexIn
{
    float3 PosL : POSITION;
};

struct VertexOut
{
    float3 PosL : POSITION;
};

// VS에서는 아무것도 하지 않는다.
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

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
    PatchTess pt;
    
    // patch의 월드 중점을 구한다.
    float3 centerL = 0.25f * (patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
    float3 centerW = mul(float4(centerL, 1.f), gWorld).xyz;
    // 카메라와 거리를 구한다.
    float d = distance(centerW, gEyePosW);
    
    // 카메라와의 거리에 따라서 테셀레이션 정도를 구한다.
    // 최대 64번 테셀레이션 한다. (거리 80이 제일 많이 할 때)
    const float d0 = 20.f;
    const float d1 = 100.f;
    float tess = 64.f * saturate((d1 - d) / (d1 - d0));
    
    // 다 똑같이 테셀레이트 한다.
    pt.EdgeTess[0] = tess;
    pt.EdgeTess[1] = tess;
    pt.EdgeTess[2] = tess;
    pt.EdgeTess[3] = tess;
	
    pt.InsideTess[0] = tess;
    pt.InsideTess[1] = tess;
	
    return pt;
}

// ================ Hull Shader - Hull Shader =======================
struct HullOut
{
    float3 PosL : POSITION;
};

[domain("quad")] // tri, quad, isoline
[partitioning("integer")] // tess factor가 정수일 때만 작동
[outputtopology("triangle_cw")] // 시계 방향
[outputcontrolpoints(4)] // 패치에 대해 헐쉐이더를 적용할 제어점 개수
[patchconstantfunc("ConstantHS")] // 테셀레이션 계수를 받을 CHS 이름
[maxtessfactor(64.f)] // 최대 테셀레이션 양
HullOut HS(InputPatch<VertexOut, 4> p, uint i : SV_OutputControlPointID, uint patchId : SV_PrimitiveID)
{
    // 일단 아무것도 안하고 넘긴다.
    HullOut hout;
    
    hout.PosL = p[i].PosL;
    
    return hout;
}

// ================ Hull Shader - Domain Shader =====================

struct DomainOut
{
    float4 PosH : SV_Position;
};

// 헐 쉐이더에서 생성된 모든 정점마다 실행이 된다.
// 여기서 호모공간으로 변환하고 픽셀 쉐이더로 넘기는 것이다.
[domain("quad")]
DomainOut DS(
                PatchTess patchTess, 
                float2 uv : SV_DomainLocation, // quad는 이중선형 보간, tri는 무게중심 좌표이다.
                const OutputPatch<HullOut, 4> quad)
{
    DomainOut dout;
    
    // 이중선형보간
    float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
    float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
    float3 p = lerp(v1, v2, uv.y);
	
	// 여기서 적절히 산의 모양처럼 만들어준다.
    p.y = 0.3f * (p.z * sin(p.x) + p.x * cos(p.z));
	// 호모 공간으로 변환한 다음
    float4 posW = mul(float4(p, 1.0f), gWorld);
    dout.PosH = mul(posW, gViewProj);
	// 픽셀 쉐이더로 넘긴다.
    return dout;
}

// ============== Pixel Shader ==================

float4 PS(DomainOut pin) : SV_Target
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}