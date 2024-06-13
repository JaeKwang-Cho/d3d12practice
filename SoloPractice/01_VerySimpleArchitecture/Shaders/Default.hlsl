// Default Shader
// position, color

/*
struct PSInput
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

PSInput VSMain(float4 position : POSITION, float4 color : COLOR)
{
    PSInput result;

    result.position = position;
    result.color = color;

    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
*/
struct VSInput
{
    float3 pos : POSITION;
    float4 color : COLOR;
};

struct PSInput
{
    float4 pos : SV_POSITION;
    float4 color : COLOR;
};

// VSInput VS(float4 _pos : POSITION, float4 _color : COLOR)
PSInput VS(VSInput _vin)
{
    PSInput vout;
    
    vout.color = _vin.color;
    //vout.color = _color;
    vout.pos = float4(_vin.pos, 1.0f);
    //vout.pos = _pos;

    return vout;
}

float4 PS(PSInput _pin) : SV_Target
{
    return _pin.color;
}