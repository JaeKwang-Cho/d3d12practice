//=============================================================================
// Structured Data �����ϴ� Vector �����̴�.
//=============================================================================

struct VectorPrac_t
{
    float3 vec;
};

[numthreads(64, 1, 1)]
void VectorPractice(int3 groupThreadID : SV_GroupThreadID, int3 dispatchThreadID : SV_DispatchThreadID)
{

}