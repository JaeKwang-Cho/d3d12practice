//=============================================================================
// Sobel 연산자를 이용해서 Sobel Filtering을 한다.
//=============================================================================

Texture2D gInput : register(t0);
RWTexture2D<float4> gOutput : register(u0);


// 이건 그냥 실험결과 값이니 믿고 쓰면 된다.
float CalcLuminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

[numthreads(16, 16, 1)]
void SobelCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // 주변 픽셀 값을 가져오고
    float4 c[3][3];
    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            int2 xy = dispatchThreadID.xy + int2(-1 + j, -1 + i);
            c[i][j] = gInput[xy];
        }
    }

	// 가로세로 번갈아 가면서, 각 색채널에 대해서 + 주변 픽셀에 대해서 Sobel 공식을 적용 시킨다.
    float4 Gx = -1.0f * c[0][0] - 2.0f * c[1][0] - 1.0f * c[2][0] + 1.0f * c[0][2] + 2.0f * c[1][2] + 1.0f * c[2][2];
    
    float4 Gy = -1.0f * c[2][0] - 2.0f * c[2][1] - 1.0f * c[2][1] + 1.0f * c[0][0] + 2.0f * c[0][1] + 1.0f * c[0][2];

	// 주변 픽셀에 대한 각 채널에 대한 변화량을 구한다.
    float4 mag = sqrt(Gx * Gx + Gy * Gy);

	// 색이 많이 변하는 엣지는 검정색, 그렇지 않으면 하얀색으로 표시가 될 것이다.
    mag = 1.0f - saturate(CalcLuminance(mag.rgb));

    gOutput[dispatchThreadID.xy] = mag;
}
