//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

cbuffer cbPerObject : register(b0)
{
	float4x4 gWVPMat;
};

struct VS_Input
{
	float3 PosL : POSITION;
	float4 Color : COLOR;
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
	vout.PosH = mul(float4(_vin.PosL, 1.0f), gWVPMat);
	
	// Just passing Color
	vout.Color = _vin.Color;
	
	return vout;
}

float4 PS(VS_Output _pin) : SV_Target
{
	return _pin.Color;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.asm"