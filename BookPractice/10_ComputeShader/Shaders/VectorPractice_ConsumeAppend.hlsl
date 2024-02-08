//=============================================================================
// Structured Data 연습하는 Vector 예제이다.
//=============================================================================

struct VectorPrac
{
    float3 vec;
};

struct VectorLen
{
    float result;
};

ConsumeStructuredBuffer<VectorPrac> gInput : register(u0);
AppendStructuredBuffer<VectorLen> gOutput : register(u1);

//RWStructuredBuffer<VectorPrac> gInput : register(u0);
//RWStructuredBuffer<VectorLen> gOutput : register(u1);

[numthreads(64, 1, 1)]
void VectorPractice(int3 groupThreadID : SV_GroupThreadID)
{
    VectorLen res;
    res.result = length(gInput.Consume().vec);
    gOutput.Append(res);
    //gOutput[groupThreadID.x].result = length(gInput[groupThreadID.x].vec);
}
