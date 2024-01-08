//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************


cbuffer cbPerObject : register(b0)
{
    float4x4 gWorldMat;
};

cbuffer cbPass : register(b1)
{
    float4x4 gViewMat;
    float4x4 gInvView;
    
    float4x4 gProjMat;
    float4x4 gInvProjMat;
    
    float4x4 gVPMat;
    float4x4 gInvVPMat;
    
    float3 gEyePosW;
    
    float cbPerObjectPad1;
    
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    
    float gNearZ;
    float gFarZ;
    
    float gTotalTime;
    float gDeltaTime;
};

struct VS_Input
{
    float3 PosL : POSITION;
    float3 Norm : NORMAL;
};

struct VS_Output
{
    float4 PosH : SV_POSITION;
    float3 Norm : NORMAL;
};

VS_Output VS(VS_Input _vin)
{
    VS_Output vout;
	
	// Transform to Render Coords
    float4 posW = mul(float4(_vin.PosL, 1.f), gWorldMat);
    vout.PosH = mul(posW, gVPMat);
    vout.Norm = _vin.Norm;
    
    return vout;
}

float4 PS(VS_Output _pin) : SV_Target
{
    return float4(0.8f, 0.5f, 0.3f, 1.f);
}
