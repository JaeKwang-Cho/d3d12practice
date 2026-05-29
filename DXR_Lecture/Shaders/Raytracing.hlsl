#ifndef RAYTRACING_HLSL
#define RAYTRACING_HLSL

#include "Raytracing_common.hlsl"

RadiancePayload TraceRadianceRay(in Ray _ray, in uint _CurRayRecursionDepth, in uint _MaxRescursionDepth,
                                float _tMin, float _tMax, bool _cullNonOpaque, bool _cullBackFace)
{
    RadiancePayload rayPayload = (RadiancePayload) 0;
    
    rayPayload.rayRecursionDepth = _CurRayRecursionDepth + 1;
    rayPayload.radiance = 0;
    
    if (_CurRayRecursionDepth >= _MaxRescursionDepth)
    {
        rayPayload.radiance = float3(1, 1, 1);
        return rayPayload;
    }

    RayDesc rayDesc;
    rayDesc.Origin = _ray.origin;
    rayDesc.Direction = _ray.direction;
    rayDesc.TMin = _tMin;
    rayDesc.TMax = _tMax;
    
    uint rayFlags = 0;
    if(_cullNonOpaque)
    {
        rayFlags |= RAY_FLAG_CULL_NON_OPAQUE; // 0x80
    }
    if(_cullBackFace)
    {
        rayFlags |= RAY_FLAG_CULL_BACK_FACING_TRIANGLES; // 0x10
    }
    
    // 나중에 광도(radiance) 계산과 그림자(Shadow) 계산을 분리 할 때
    // TraceRay 함수의 HitGroupIndex와 Multiplier를 이용해서 광도 계산과 그림자 계산을 분리할 수 있다.
    // TraceRay(Scene, rayFlags, 0xFFFFFFFFu, 0, 2, 0, rayDesc, rayPayload);
    // TraceRay(Scene, rayFlags, 0xFFFFFFFFu, 1, 2, 1, rayDesc, shadowPayload);
    // https://learn.microsoft.com/ko-kr/windows/win32/direct3d12/traceray-function 
    
    TraceRay(Scene, rayFlags, 0xFFFFFFFFu, 0, 1, 0, rayDesc, rayPayload);

    return rayPayload;
}

[shader("raygeneration")]
void MyRaygenShader_RadianceRay()
{
    uint2 launchIndex = DispatchRaysIndex().xy;
    uint2 launchDim = DispatchRaysDimensions().xy;

    float2 xy = launchIndex.xy + 0.5f; // 픽셀 중심에서 레이 발사 (Raygen이 정규좌표계가 아니라 픽셀 좌표계에서 실행되기 때문에 0.5를 더해준다.)
    float2 screenPos = xy / launchDim.xy * 2.0 - 1.0f; // 화면 좌표계로 변환 (0, 0) ~ (width, height) -> (-1, -1) ~ (1, 1)
    
    float3 origin = float3(screenPos.xy, 0);
    float3 direction = float3(0, 0, 1);
    
    Ray ray =
    {
        origin,
        direction
    };
    
    uint CurrRayRecursionDepth = 0;
    bool cullNonOpaque = false;
    bool cullBackFace = false;
    RadiancePayload rayPayload = TraceRadianceRay(ray, CurrRayRecursionDepth, 
                                                g_MaxRadianceRayRecursionDepth, NEAR_PLANE, FAR_PLANE,
                                                cullNonOpaque, cullBackFace);
    
    g_OutputDiffuse[launchIndex.xy] = float4(rayPayload.radiance, 1);
    g_OutputDepth[launchIndex.xy] = rayPayload.depth;
}
[shader("closesthit")]
void MyClosestHitShader_RadianceRay(inout RadiancePayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 hitPosition = HitWorldPosition();
    
    rayPayload.depth = hitPosition.z;
    rayPayload.radiance = float3(1, 0, 0); // 임시로 빨간색으로 표시. 나중에 광도 계산이 들어가면 hitPosition과 hitNormal을 이용해서 광도 계산을 할 수 있다.
}
[shader("closesthit")]
void MyClosestHitShader_ShadowRay(inout ShadowPayload rayPayload, in BuiltInTriangleIntersectionAttributes attr)
{
}


[shader("miss")]
void MyMissShader_RadianceRay(inout RadiancePayload rayPayload)
{
    rayPayload.radiance = float3(0, 0, 0); // 임시로 검은색으로 표시. 나중에 환경광 계산이 들어가면 환경광 색상을 반환할 수 있다.
    rayPayload.depth = 1.2;
}

[shader("anyhit")]
void MyAnyHitShader_RadianceRay(inout RadiancePayload payload, in BuiltInTriangleIntersectionAttributes attr)
{
    float3 hitPosition = HitWorldPosition();
    
    // 삼각형의 첫번째 16-bit index의 시작점을 얻어온다.
    uint InstID = InstanceID();
    // (자동 생성된) TLAS에서 현재 레이가 히트된 인스턴스의 인덱스를 얻어온다.
    uint SystemInstIndex = InstanceIndex();
}

#endif // RAYTRACING_HLSL
