//=============================================================================
// Structured Data 연습하는 Vector 예제이다.
//=============================================================================

/*
struct VectorPrac
{
    float3 vec;
};

struct VectorLen
{
    float result;
};

StructuredBuffer<VectorPrac> gInput : register(t0);
RWStructuredBuffer<VectorPrac> gOutput : register(u0);
*/

StructuredBuffer<float3> gInput : register(t0);
RWStructuredBuffer<float> gOutput : register(u0);


[numthreads(64, 1, 1)]
void VectorPractice(int3 groupThreadID : SV_GroupThreadID)
{
    gOutput[groupThreadID.x] = length(gInput[groupThreadID.x]);
}
