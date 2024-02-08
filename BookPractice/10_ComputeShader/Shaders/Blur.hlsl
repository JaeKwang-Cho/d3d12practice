//=============================================================================
// blur radius 5으로 가우시안 블러를 먹인다. 
//=============================================================================

cbuffer cbSettings : register(b0)
{
    // Root Constants로 매핑이 되는 CB는 Array로 사용할 수 없다.
    // 그래서 이렇게 좌르륵 써야 한다.
    int gBlurRadius;
    
    // 11개의 blur 가중치를 가진다.
    float w0;
    float w1;
    float w2;
    float w3;
    float w4;
    float w5;
    float w6;
    float w7;
    float w8;
    float w9;
    float w10;
};

// 최대 5로 제한한다.
static const int gMaxBlurRadius = 5;

// 입력
Texture2D gInput : register(t0);
// 출력
RWTexture2D<float4> gOutput : register(u0);

#define N (256)
#define CacheSize (N + 2 * gMaxBlurRadius)
groupshared float4 gCache[CacheSize];

[numthreads(N, 1, 1)]
void HorzBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    uint inputXLen;
    uint inputYLen;
    uint inputLevel;
    
    gInput.GetDimensions(0, inputXLen, inputYLen, inputLevel);
    
    // 각 weight를 배열에 넣는다.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // 공유메모리를 이용해서, 스레드가 텍스쳐 표본 추출을 색인과 같은 방법으로
    // 타겟픽셀과 주번픽셀을 접근하면, 다른 스레드와 같은 픽셀을 여러번 가져오게되는데,
    // 텍스쳐 샘플링은 공유메모리 접근 보다 더 무거운 작업이다.
    
    // 그리고 가생이 문제가 있는데, 일단 N개의 스레드를 만들었고
    // 각 스레드가 픽셀을 지나면서, N개와 양 끝의 Blur Radius * 2 만큼 읽어야 한다.
    // 그래서 border에 있는 친구들은 그냥 가장자리에 있는걸 그대로 다시 넣어준다.
    
    // 일단 각 스레드 들이 sample을 해서 자기 픽셀을 gCache에 다 넣어준다.
    
    // 왼쪽이다.
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        // 그러면 맨 왼쪽에 있는 픽셀값으로 넣어버린다.
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }
    // 오른쪽이다.
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, inputXLen - 1);
        // 그러면 해당 텍스쳐 구역, 맨 오른쪽에 있는 픽셀값으로 넣어버린다.
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
    }
    
    // 나머지는 그냥 자기꺼 넣는다.
    uint2 inputXYLen = { inputXLen, inputYLen };
    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, inputXYLen - 1)];
    
    // 모든 스레드가 공유메모리를 채우기까지 기다리고,
    GroupMemoryBarrierWithGroupSync();
    
    // 이제 각 픽셀에 블러를 먹인다.
    float4 blurColor = float4(0, 0, 0, 0);
    
    // 가로x 블러를 먹인다.
    for (int i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        int k = groupThreadID.x + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
    
    gOutput[dispatchThreadID.xy] = blurColor;
}

[numthreads(1, N, 1)]
void VertBlurCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    uint inputXLen;
    uint inputYLen;
    uint inputLevel;
    
    gInput.GetDimensions(0, inputXLen, inputYLen, inputLevel);
    
    // 각 weight를 배열에 넣는다.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // 공유메모리를 이용해서, 스레드가 텍스쳐 표본 추출을 색인과 같은 방법으로
    // 타겟픽셀과 주번픽셀을 접근하면, 다른 스레드와 같은 픽셀을 여러번 가져오게되는데,
    // 텍스쳐 샘플링은 공유메모리 접근 보다 더 무거운 작업이다.
    
    // 그리고 가생이 문제가 있는데, 일단 N개의 스레드를 만들었고
    // 각 스레드가 픽셀을 지나면서, N개와 양 끝의 Blur Radius * 2 만큼 읽어야 한다.
    // 그래서 border에 있는 친구들은 그냥 가장자리에 있는걸 그대로 다시 넣어준다.
    
    // 일단 각 스레드 들이 sample을 해서 자기 픽셀을 gCache에 다 넣어준다.
    
    // 위쪽이다.
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        // 그러면 맨 위쪽에 있는 픽셀값으로 넣어버린다.
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }
    // 아래쪽이다.
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, inputYLen - 1);
        // 그러면 해당 텍스쳐 구역, 맨 아래쪽에 있는 픽셀값으로 넣어버린다.
        gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
    }
    
    // 나머지는 그냥 자기꺼 넣는다.
    uint2 inputXYLen = { inputXLen, inputYLen };
    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, inputXYLen - 1)];
    
    // 모든 스레드가 공유메모리를 채우기까지 기다리고,
    GroupMemoryBarrierWithGroupSync();
    
    // 이제 각 픽셀에 블러를 먹인다.
    float4 blurColor = float4(0, 0, 0, 0);
    
    // 가로x 블러를 먹인다.
    for (int i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        int k = groupThreadID.y + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
    
    gOutput[dispatchThreadID.xy] = blurColor;
}