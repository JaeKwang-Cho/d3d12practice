//=============================================================================
// blur radius 5���� ����þ� ���� ���δ�. 
//=============================================================================

cbuffer cbSettings : register(b0)
{
    // Root Constants�� ������ �Ǵ� CB�� Array�� ����� �� ����.
    // �׷��� �̷��� �¸��� ��� �Ѵ�.
    int gBlurRadius;
    
    // 11���� blur ����ġ�� ������.
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

// �ִ� 5�� �����Ѵ�.
static const int gMaxBlurRadius = 5;

// �Է�
Texture2D gInput : register(t0);
// ���
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
    
    // �� weight�� �迭�� �ִ´�.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // �����޸𸮸� �̿��ؼ�, �����尡 �ؽ��� ǥ�� ������ ���ΰ� ���� �������
    // Ÿ���ȼ��� �ֹ��ȼ��� �����ϸ�, �ٸ� ������� ���� �ȼ��� ������ �������ԵǴµ�,
    // �ؽ��� ���ø��� �����޸� ���� ���� �� ���ſ� �۾��̴�.
    
    // �׸��� ������ ������ �ִµ�, �ϴ� N���� �����带 �������
    // �� �����尡 �ȼ��� �����鼭, N���� �� ���� Blur Radius * 2 ��ŭ �о�� �Ѵ�.
    // �׷��� border�� �ִ� ģ������ �׳� �����ڸ��� �ִ°� �״�� �ٽ� �־��ش�.
    
    // �ϴ� �� ������ ���� sample�� �ؼ� �ڱ� �ȼ��� gCache�� �� �־��ش�.
    
    // �����̴�.
    if (groupThreadID.x < gBlurRadius)
    {
        int x = max(dispatchThreadID.x - gBlurRadius, 0);
        // �׷��� �� ���ʿ� �ִ� �ȼ������� �־������.
        gCache[groupThreadID.x] = gInput[int2(x, dispatchThreadID.y)];
    }
    // �������̴�.
    if (groupThreadID.x >= N - gBlurRadius)
    {
        int x = min(dispatchThreadID.x + gBlurRadius, inputXLen - 1);
        // �׷��� �ش� �ؽ��� ����, �� �����ʿ� �ִ� �ȼ������� �־������.
        gCache[groupThreadID.x + 2 * gBlurRadius] = gInput[int2(x, dispatchThreadID.y)];
    }
    
    // �������� �׳� �ڱⲨ �ִ´�.
    uint2 inputXYLen = { inputXLen, inputYLen };
    gCache[groupThreadID.x + gBlurRadius] = gInput[min(dispatchThreadID.xy, inputXYLen - 1)];
    
    // ��� �����尡 �����޸𸮸� ä������ ��ٸ���,
    GroupMemoryBarrierWithGroupSync();
    
    // ���� �� �ȼ��� ���� ���δ�.
    float4 blurColor = float4(0, 0, 0, 0);
    
    // ����x ���� ���δ�.
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
    
    // �� weight�� �迭�� �ִ´�.
    float weights[11] = { w0, w1, w2, w3, w4, w5, w6, w7, w8, w9, w10 };
    
    // �����޸𸮸� �̿��ؼ�, �����尡 �ؽ��� ǥ�� ������ ���ΰ� ���� �������
    // Ÿ���ȼ��� �ֹ��ȼ��� �����ϸ�, �ٸ� ������� ���� �ȼ��� ������ �������ԵǴµ�,
    // �ؽ��� ���ø��� �����޸� ���� ���� �� ���ſ� �۾��̴�.
    
    // �׸��� ������ ������ �ִµ�, �ϴ� N���� �����带 �������
    // �� �����尡 �ȼ��� �����鼭, N���� �� ���� Blur Radius * 2 ��ŭ �о�� �Ѵ�.
    // �׷��� border�� �ִ� ģ������ �׳� �����ڸ��� �ִ°� �״�� �ٽ� �־��ش�.
    
    // �ϴ� �� ������ ���� sample�� �ؼ� �ڱ� �ȼ��� gCache�� �� �־��ش�.
    
    // �����̴�.
    if (groupThreadID.y < gBlurRadius)
    {
        int y = max(dispatchThreadID.y - gBlurRadius, 0);
        // �׷��� �� ���ʿ� �ִ� �ȼ������� �־������.
        gCache[groupThreadID.y] = gInput[int2(dispatchThreadID.x, y)];
    }
    // �Ʒ����̴�.
    if (groupThreadID.y >= N - gBlurRadius)
    {
        int y = min(dispatchThreadID.y + gBlurRadius, inputYLen - 1);
        // �׷��� �ش� �ؽ��� ����, �� �Ʒ��ʿ� �ִ� �ȼ������� �־������.
        gCache[groupThreadID.y + 2 * gBlurRadius] = gInput[int2(dispatchThreadID.x, y)];
    }
    
    // �������� �׳� �ڱⲨ �ִ´�.
    uint2 inputXYLen = { inputXLen, inputYLen };
    gCache[groupThreadID.y + gBlurRadius] = gInput[min(dispatchThreadID.xy, inputXYLen - 1)];
    
    // ��� �����尡 �����޸𸮸� ä������ ��ٸ���,
    GroupMemoryBarrierWithGroupSync();
    
    // ���� �� �ȼ��� ���� ���δ�.
    float4 blurColor = float4(0, 0, 0, 0);
    
    // ����x ���� ���δ�.
    for (int i = -gBlurRadius; i <= gBlurRadius; i++)
    {
        int k = groupThreadID.y + gBlurRadius + i;
        blurColor += weights[i + gBlurRadius] * gCache[k];
    }
    
    gOutput[dispatchThreadID.xy] = blurColor;
}