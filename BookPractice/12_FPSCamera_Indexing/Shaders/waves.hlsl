//***************************************************************************************
// waves.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// UpdateWavesCS(): CPU에서 하던 파도 방정식 처리를 GPU의 힘을 빌려서 한다.
//
// DisturbWavesCS(): 처음 파도를 일으키도록 하는 CS이다. 그 뒤로는 UpdateWavesCS로 처리된다.
//***************************************************************************************

// 파도 방정식에 쓰이는 상수들이다.
cbuffer cbUpdateSettings
{
    float gWaveConstant0;
    float gWaveConstant1;
    float gWaveConstant2;
    
    float gDisturbMag;
    int2 gDisturbIndex;
};

RWTexture2D<float> gPrevSolInput : register(u0);
RWTexture2D<float> gCurrSolInput : register(u1);
RWTexture2D<float> gOutput : register(u2);

[numthreads(16, 16, 1)]
void UpdateWavesCS(int3 dispatchThreadID : SV_DispatchThreadID)
{
    // 여기서는 경계 체크가 필요 없다.
    // 스레드 개수를 넘어가거나 0보다 작은 거는,
    // 전부 0으로 처리하기 때문이다.
    int x = dispatchThreadID.x;
    int y = dispatchThreadID.y;
    
    gOutput[int2(x, y)] =
    gWaveConstant0 * gPrevSolInput[int2(x, y)].r +
    gWaveConstant1 * gCurrSolInput[int2(x, y)].r +
    gWaveConstant2 * (
        gCurrSolInput[int2(x, y + 1)].r +
        gCurrSolInput[int2(x, y - 1)].r +
        gCurrSolInput[int2(x + 1, y)].r +
        gCurrSolInput[int2(x - 1, y)].r);
}

[numthreads(1, 1, 1)]
void DisturbWavesCS(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{
    // 경계 체크가 필요 없다.
    // 스레드 개수를 넘어가거나 0보다 작은 거는,
    // 전부 0으로 처리하기 때문이다.
    
    int x = gDisturbIndex.x;
    int y = gDisturbIndex.y;
    
    float halfMag = 0.5f * gDisturbMag;
    
    // RW으로 지정된 버퍼는 += 연산자가 잘 작동한다.
    gOutput[int2(x, y)] += gDisturbMag;
    gOutput[int2(x, y + 1)] += halfMag;
    gOutput[int2(x + 1, y)] += halfMag;
    gOutput[int2(x, y - 1)] += halfMag;
    gOutput[int2(x - 1, y)] += halfMag;
}