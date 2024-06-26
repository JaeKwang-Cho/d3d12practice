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
    float4 Color : COLOR;
    float3 PosL : POSITION;
};

struct VS_Output
{
	float4 PosH : SV_POSITION;
	float4 Color : COLOR;
};

VS_Output VS(VS_Input _vin)
{
	VS_Output vout;
	
	// Transform to Render Coords
    float4 posW = mul(float4(_vin.PosL, 1.f), gWorldMat);
    vout.PosH = mul(posW, gVPMat);
	
    vout.Color = _vin.Color;
    
	return vout;
}

float4 PS(VS_Output _pin) : SV_Target
{
	return _pin.Color;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.asm"