#ifndef RAYTRACING_COMMON_HLSL
#define RAYTRACING_COMMON_HLSL


#include "Raytracing_typedef.hlsl"

// Global Root Parameter
RWTexture2D<float4> g_OutputDiffuse : register(u0);
RWTexture2D<float4> g_OutputDepth : register(u1);
RaytracingAccelerationStructure Scene : register(t0, space0);

cbuffer CONSTANT_BUFFER_RAY_TRACING : register(b0)
{
    float g_Near;
    float g_Far;
    uint g_MaxRadianceRayRecursionDepth;
    uint Reserved0;
};

float3 HitAttribute(float3 _vertexAttribute[3], BuiltInTriangleIntersectionAttributes _attr)
{
    return _vertexAttribute[0] + 
        _attr.barycentrics.x * (_vertexAttribute[1] - _vertexAttribute[0]) + 
        _attr.barycentrics.y * (_vertexAttribute[2] - _vertexAttribute[0]);
}

float2 HitAttribute(float2 _vertexAttribute[3], BuiltInTriangleIntersectionAttributes _attr)
{
    return _vertexAttribute[0] + 
        _attr.barycentrics.x * (_vertexAttribute[1] - _vertexAttribute[0]) + 
        _attr.barycentrics.y * (_vertexAttribute[2] - _vertexAttribute[0]);
}

// Retrieve the world space
float3 HitWorldPosition()
{
    return WorldRayOrigin() + RayTCurrent() + WorldRayDirection();
    // WorldRayOrigin: 레이의 시작점(월드 좌표계)
    // RayTCurrent: 현재 레이의 길이
    // WorldRayDirection: 레이의 방향(월드 좌표계)
    
    // 레이의 시작점에서 현재 레이의 길이만큼 레이의 방향으로 이동한 위치가 히트된 지점의 월드 좌표입니다.
}


#endif // RAYTRACING_COMMON_HLSL
