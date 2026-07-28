#ifndef RAYTRACING_COMMON_HLSL
#define RAYTRACING_COMMON_HLSL


#include "Raytracing_typedef.hlsl"

// Global Root Parameter
RWTexture2D<float4> g_OutputDiffuse : register(u0);
RWTexture2D<float4> g_OutputDepth : register(u1);
RaytracingAccelerationStructure Scene : register(t0, space0);
SamplerState samplerWrap : register(s0);
SamplerState samplerClamp : register(s1);
SamplerState samplerPoint : register(s2);
SamplerState samplerMirror : register(s3);

// Local Root Parameter
ConstantBuffer<CONSTANT_BUFFER_RT_TRIGROUP> l_rayGeomCB : register(b0, space1);
StructuredBuffer<BasicVertex> l_Vertices : register(t0, space1); // read-only uav
ByteAddressBuffer l_Indices : register(t1, space1); // read-only binary
Texture2D<float> l_texDiffuse : register(t2, space1);

cbuffer CONSTANT_BUFFER_RAY_TRACING : register(b0)
{
    matrix g_matViewProj;
    matrix g_matViewInv;
    DECOMP_PROJ g_DecompProj;
    float4 g_vCameraPos;
    float g_Near;
    float g_Far;
    uint g_MaxRadianceRayRecursionDepth;
    uint Reserved0;
};

float4 HitAttribute(float4 _vertexAttribute[3], BuiltInTriangleIntersectionAttributes _attr)
{
    return _vertexAttribute[0] + 
        _attr.barycentrics.x * (_vertexAttribute[1] - _vertexAttribute[0]) + 
        _attr.barycentrics.y * (_vertexAttribute[2] - _vertexAttribute[0]);
}

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

// Load three 16-bit-Indices
static uint3 Load3x16BitIndices(uint _offsetBytes)
{
    uint3 indices;
    
    // ByteAddressBuffer는 4-byte 단위로 align이 되어야 한다.
    // 현재 구조에서는 16-bit 인덱스 3개를 읽어야 한다.
    // 4-byte 단위로 align된 오프셋을 계산하여 4개의 16-bit 인덱스를 읽어옵니다.
    // align된 예시는  {0, 1, 2, ?}이고 {?, 0, 1, 2}는 align이 되지 않은 예시입니다.
    
    const uint dwordAlignedOffset = _offsetBytes & ~0x3; // 4-byte 단위로 align된 오프셋 계산
    const uint2 four16BitIndices = l_Indices.Load2(dwordAlignedOffset); // 4개의 16-bit 인덱스 읽기 (총 8 bytes)
    
    if (dwordAlignedOffset == _offsetBytes)
    {
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else
    {
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }

    return indices;

}

#endif // RAYTRACING_COMMON_HLSL
