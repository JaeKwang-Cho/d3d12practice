//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

#define PRAC6 (0)
#define PRAC10 (0)
#define PRAC14 (0)
#define PRAC15 (0)
#define PRAC16 (0)

cbuffer cbPerObject : register(b0)
{
	float4x4 gWVPMat;
#if PRAC6 || PRAC14 || PRAC16
    float gTime;
#endif
#if PRAC16
    float4 gPulseColor;
#endif
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
	
#if PRAC6
    _vin.PosL.xy += 0.5f * sin(_vin.PosL.x) * sin(3.f * gTime);
    _vin.PosL.z *= 0.6 + 0.4f * sin(2.f * gTime);
#endif
	// Transform to Render Coords
	vout.PosH = mul(float4(_vin.PosL, 1.0f), gWVPMat);
	
    vout.Color = _vin.Color;
#if PRAC14
    vout.Color.r = 0.5f + _vin.Color.r * sin(2.f * gTime);
#endif
    
	return vout;
}

float4 PS(VS_Output _pin) : SV_Target
{
#if PRAC14
    _pin.Color.b = 0.5f + _pin.Color.b * sin(5.f * gTime);
#elif PRAC15	
    clip(_pin.Color.r - 0.5f);
#elif PRAC16
    const float pi = 3.14159;
    // 0과 1사이 진동하는 값으로 알파값을 정하고
    float s = 0.5f * sin(2 * gTime - 0.25f * pi) + 0.5f;
    // 그것으로 보간한 Color를 그린다.
    _pin.Color = lerp(_pin.Color, gPulseColor, s);
#endif
	return _pin.Color;
}

// fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T vs_5_1 /E "VS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shader\02_Rendering_VS.asm"

//fxc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_Shader.hlsl" /Od /Zi /T ps_5_1 /E "PS" /Fo "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.cso" /Fc "A:\[DirectX]\MyDirectX\PracticeD3D12\BookPractice\02_Rendering\Shaders\02_Rendering_PS.asm"