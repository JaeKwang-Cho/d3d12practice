#ifndef NUM_DIR_LIGHTS
#define NUM_DIR_LIGHTS 1
#endif

#ifndef NUM_POINT_LIGHTS
#define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
#define NUM_SPOT_LIGHTS 0
#endif

// 빛 계산을 하는데 필요한 함수를 모아 놓은 것이다.
#include "LightingUtil.hlsl"

Texture2D texDiffuse : register(t0);
SamplerState samplerDiffuse : register(s0);

cbuffer CONSTANT_BUFFER_FRAME : register(b1)
{
    matrix g_matView;
    matrix g_intView;

    matrix g_matProj;
    matrix g_invProj;

    matrix g_matViewProj;
    matrix g_invViewProj;

    float3 g_eyePosW;
    float pad0;
    float2 g_renderTargetSize;
    float2 g_renderTargetSize_reciprocal;

    float4 g_ambientLight;
    Light g_lights[MAX_LIGHT_COUNTS];
};

cbuffer CONSTANT_BUFFER_OBJECT : register(b0)
{
    matrix g_matWorld;
    matrix g_invWorldTranspose;
};