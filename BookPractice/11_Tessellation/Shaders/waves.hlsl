//***************************************************************************************
// waves.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// UpdateWavesCS(): CPU���� �ϴ� �ĵ� ������ ó���� GPU�� ���� ������ �Ѵ�.
//
// DisturbWavesCS(): ó�� �ĵ��� ����Ű���� �ϴ� CS�̴�. �� �ڷδ� UpdateWavesCS�� ó���ȴ�.
//***************************************************************************************

// �ĵ� �����Ŀ� ���̴� ������̴�.
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
    // ���⼭�� ��� üũ�� �ʿ� ����.
    // ������ ������ �Ѿ�ų� 0���� ���� �Ŵ�,
    // ���� 0���� ó���ϱ� �����̴�.
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
    // ��� üũ�� �ʿ� ����.
    // ������ ������ �Ѿ�ų� 0���� ���� �Ŵ�,
    // ���� 0���� ó���ϱ� �����̴�.
    
    int x = gDisturbIndex.x;
    int y = gDisturbIndex.y;
    
    float halfMag = 0.5f * gDisturbMag;
    
    // RW���� ������ ���۴� += �����ڰ� �� �۵��Ѵ�.
    gOutput[int2(x, y)] += gDisturbMag;
    gOutput[int2(x, y + 1)] += halfMag;
    gOutput[int2(x + 1, y)] += halfMag;
    gOutput[int2(x, y - 1)] += halfMag;
    gOutput[int2(x - 1, y)] += halfMag;
}