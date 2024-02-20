//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

// 최대 점 개수는 16개로 제한
#define MaxLights 16

struct Light
{
    float3 Strength;
    float FalloffStart; // point/spot light only
    float3 Direction; // directional/spot light only
    float FalloffEnd; // point/spot light only
    float3 Position; // point light only
    float SpotPower; // spot light only
};

struct Material
{
    float4 DiffuseAlbedo;
    float3 FresnelR0;
    float Shininess;
};

float CalcAttenuation(float d, float falloffStart, float falloffEnd)
{
    // 선형 감쇠로 한다.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// 슐릭 공식으로 프레넬 반사를 근사한다.Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, (n은 굴절률)
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}
// specular를 구하는 간단화 된 퐁 계산 방법이다.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // Shininess(광택도 = 거칠기)에서 m을 유도한다.
    const float m = mat.Shininess * 256.0f;
    // 하프 벡터와 노멀과 차이가 얼마인지 계산하고, 
    float3 halfVec = normalize(toEye + lightVec);
    // 거칠기를 통한 반사율, 정규화 계수를 이용해서 roughness 광을 구한다.
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    
    // 프레넬 반사광은 슐릭 공식으로 구한다.
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    // 두개를 곱한 것이 스페큘러이다.
    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // HDR이 아니기 때문에, [0, 1]로 잘라준다.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// directional light에 의한 방정식을 계산해준다.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // 빛이 들어오는 반대 방향으로 벡터를 잡아준다.
    float3 lightVec = -L.Direction;

    // 램배트 - 코사인 법칙으로 감쇠해준다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// point light에 의한 방정식을 계산해준다.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // 표면에서 빛까지 가는 벡터를 구해준다.
    float3 lightVec = L.Position - pos;

    // 거리도 구해준다.
    float d = length(lightVec);

    // 유효거리를 테스트해주고
    if (d > L.FalloffEnd)
        return 0.0f;

    // 벡터 정규화도 시켜주고
    lightVec /= d;

    // 램버트 코사인 법칙을 이용해 빛의 세기도 감쇠해준다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // 거리에 의한 감쇠도 해준다.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// spot light에 의한 방정식을 계산해준다.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // 표면에서 광원으로 방향 벡터를 구한다.The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // 빛과 표면과 거리를 구한다.
    float d = length(lightVec);

    // 거리 유효성 테스트를 한다.
    if (d > L.FalloffEnd)
        return 0.0f;

    // 빛 벡터를 정규화 한다..
    lightVec /= d;

    // 빛의 세기를 램버트-코사인으로 감쇠한다.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // 빛을 거리로 감쇠한다.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // spot light 원 뿔 계수로 감쇠한다.
    float spotFactor = pow(max(dot(-lightVec, L.Direction), 0.0f), L.SpotPower);
    lightStrength *= spotFactor;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

float4 ComputeLighting(Light gLights[MaxLights], Material mat,
                       float3 pos, float3 normal, float3 toEye,
                       float3 shadowFactor)
{
    float3 result = 0.0f;

    int i = 0;

    // 다음 메크로는 이 유틸 쉐이더를 include 하는 친구에서 정의 한다.
    // 그리고 그 아래 바디도 그에 맞게 작성을 하는 것이다.
#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif
    // 이렇게 오프셋을 맞춰서, 알맞은 계산을 한뒤
#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif
    // 빛이 다 적용된 최종 색을 그려내는 것이다.
#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}


