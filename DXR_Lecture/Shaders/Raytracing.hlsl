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

    float2 CurPixel = (float2) launchIndex.xy + float2(0.5f, 0.5f); // 픽셀 중심에서 레이 발사 (Raygen이 정규좌표계가 아니라 픽셀 좌표계에서 실행되기 때문에 0.5를 더해준다.)
    float2 Resolution = (float2) launchDim.xy;
    
    float4 ray_view = float4(
		(((2.0f * ((float) CurPixel.x) / (float) Resolution.x)) - 1.0f) * g_DecompProj.rcp_m11,
		-(((2.0f * ((float) CurPixel.y) / (float) Resolution.y)) - 1.0f) * g_DecompProj.rcp_m22,
		1.0, 0.0);
    
    float4 ray_world = mul(ray_view, g_matViewInv);
    ray_world.xyz = normalize(ray_world.xyz);
    
    float3 worldDir = ray_world.xyz;
    float3 worldOrigin = float3(g_vCameraPos.xyz);
    
    Ray ray =
    {
        worldOrigin,
        worldDir
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
void MyClosestHitShader_RadianceRay(inout RadiancePayload _rayPayload, in BuiltInTriangleIntersectionAttributes _attr)
{
    float3 hitPosition = HitWorldPosition();
    
    // 16-bit 인덱스 버퍼의 시작점을 얻어온다.
    uint InstID = InstanceID();
    //uint CustomInstIndex = GetInstanceIndex(InstID); // RaytracingManager에서 넣어주는 index-object index를 얻을 수 있다.
    // (자동 생성된) TLAS에서 현재 레이가 히트된 인스턴스의 인덱스를 얻어온다.
    uint SystemInstIndex = InstanceIndex();
    
    // (자동 상성된) BLAS에서 현재 레이가 히트된 삼각형의 인덱스를 얻어온다.
    // 충돌이 발생 했을때, LocalRootSignature에서 byte 단위로 전달 되는 데이터에서, 몇번째에 삼각형의 index가 있는지를 나타내는 값이 g_TriangleIndexStride이다.
    uint baseIndex = PrimitiveIndex() * g_TriangleIndexStride;
    
    
    // 삼각형의 세 점을 얻기 위해, 세 점의 index를 얻는다.
    uint3 indices = Load3x16BitIndices(baseIndex);
    float2 CurTexCoord = 0;
    float4 CurColor = float4(0, 0, 0, 1);
    float4 texDiffuse = float4(1, 1, 1, 0);
    
    float3 VertexNormals[3] =
    {
        l_Vertices[indices.x].Normal,
        l_Vertices[indices.y].Normal,
        l_Vertices[indices.z].Normal
    };
    
    float4 Color[3] =
    {
        l_Vertices[indices.x].Color,
        l_Vertices[indices.y].Color,
        l_Vertices[indices.z].Color
    };
    
    float2 TexCoord[3] =
    {
        l_Vertices[indices.x].TexCoord,
        l_Vertices[indices.y].TexCoord,
        l_Vertices[indices.z].TexCoord
    };
    
    CurColor = HitAttribute(Color, _attr); // 현재 hit된 지점의 색상. 삼각형의 세 점의 색상과 hit attribute로 보간해서 계산한다.
    CurTexCoord = HitAttribute(TexCoord, _attr);
    
    texDiffuse = l_texDiffuse.SampleLevel(samplerPoint, CurTexCoord, 0);
    
    // 오브젝트의 local 좌표계에서 normal
    float3 LocalNormal = HitAttribute(VertexNormals, _attr);
    // 오브젝트의 local 좌표계에서 normal을 월드 좌표계로
    // ObjectToWorld4x3: 오브젝트의 local 좌표계를 월드 좌표계로 변환하는 행렬. 
    // TLAS에서 BLAS를 변환할 때, 오브젝트의 위치, 회전, 크기 등의 정보를 이용해서 ObjectToWorld4x3 행렬이 만들어진다.
    float3 WorldNormal = normalize(mul(LocalNormal, (float3x3) ObjectToWorld4x3())); 
    
    
    _rayPayload.depth = hitPosition.z;
    _rayPayload.radiance = texDiffuse.rgb * CurColor.rgb;
}
[shader("closesthit")]
void MyClosestHitShader_ShadowRay(inout ShadowPayload _rayPayload, in BuiltInTriangleIntersectionAttributes _attr)
{
    _rayPayload.tHit = RayTCurrent(); // 레이가 히트된 지점까지의 거리를 저장. 그림자 계산에서는 이 값이 0보다 크면 그림자가 드리워진 것으로 간주할 수 있다.
}

[shader("miss")]
void MyMissShader_RadianceRay(inout RadiancePayload _rayPayload)
{
    _rayPayload.radiance = float3(0, 0, 1); // 임시로 파란색으로 표시. 나중에 환경광 계산이 들어가면 환경광 색상을 반환할 수 있다.
    _rayPayload.depth = 1.2;
}

[shader("anyhit")]
void MyAnyHitShader_RadianceRay(inout RadiancePayload _payload, in BuiltInTriangleIntersectionAttributes _attr)
{
    float3 hitPosition = HitWorldPosition();
    
    // 삼각형의 첫번째 16-bit index의 시작점을 얻어온다.
    uint InstID = InstanceID();
    // (자동 생성된) TLAS에서 현재 레이가 히트된 인스턴스의 인덱스를 얻어온다.
    uint SystemInstIndex = InstanceIndex();
}

#endif // RAYTRACING_HLSL
