//***************************************************************************************
// LightingUtil.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Contains API for shader lighting.
//***************************************************************************************

// �ִ� �� ������ 16���� ����
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
    // ���� ����� �Ѵ�.
    return saturate((falloffEnd - d) / (falloffEnd - falloffStart));
}

// ���� �������� ������ �ݻ縦 �ٻ��Ѵ�.Schlick gives an approximation to Fresnel reflectance (see pg. 233 "Real-Time Rendering 3rd Ed.").
// R0 = ( (n-1)/(n+1) )^2, (n�� ������)
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVec)
{
    float cosIncidentAngle = saturate(dot(normal, lightVec));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}
// specular�� ���ϴ� ����ȭ �� �� ��� ����̴�.
float3 BlinnPhong(float3 lightStrength, float3 lightVec, float3 normal, float3 toEye, Material mat)
{
    // Shininess(���õ� = ��ĥ��)���� m�� �����Ѵ�.
    const float m = mat.Shininess * 256.0f;
    // ���� ���Ϳ� ��ְ� ���̰� ������ ����ϰ�, 
    float3 halfVec = normalize(toEye + lightVec);
    // ��ĥ�⸦ ���� �ݻ���, ����ȭ ����� �̿��ؼ� roughness ���� ���Ѵ�.
    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVec, normal), 0.0f), m) / 8.0f;
    
    // ������ �ݻ籤�� ���� �������� ���Ѵ�.
    float3 fresnelFactor = SchlickFresnel(mat.FresnelR0, halfVec, lightVec);
    // �ΰ��� ���� ���� ����ŧ���̴�.
    float3 specAlbedo = fresnelFactor * roughnessFactor;

    // HDR�� �ƴϱ� ������, [0, 1]�� �߶��ش�.
    specAlbedo = specAlbedo / (specAlbedo + 1.0f);

    return (mat.DiffuseAlbedo.rgb + specAlbedo) * lightStrength;
}

//---------------------------------------------------------------------------------------
// directional light�� ���� �������� ������ش�.
//---------------------------------------------------------------------------------------
float3 ComputeDirectionalLight(Light L, Material mat, float3 normal, float3 toEye)
{
    // ���� ������ �ݴ� �������� ���͸� ����ش�.
    float3 lightVec = -L.Direction;

    // ����Ʈ - �ڻ��� ��Ģ���� �������ش�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// point light�� ���� �������� ������ش�.
//---------------------------------------------------------------------------------------
float3 ComputePointLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� ������ ���� ���͸� �����ش�.
    float3 lightVec = L.Position - pos;

    // �Ÿ��� �����ش�.
    float d = length(lightVec);

    // ��ȿ�Ÿ��� �׽�Ʈ���ְ�
    if (d > L.FalloffEnd)
        return 0.0f;

    // ���� ����ȭ�� �����ְ�
    lightVec /= d;

    // ����Ʈ �ڻ��� ��Ģ�� �̿��� ���� ���⵵ �������ش�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // �Ÿ��� ���� ���赵 ���ش�.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    return BlinnPhong(lightStrength, lightVec, normal, toEye, mat);
}

//---------------------------------------------------------------------------------------
// spot light�� ���� �������� ������ش�.
//---------------------------------------------------------------------------------------
float3 ComputeSpotLight(Light L, Material mat, float3 pos, float3 normal, float3 toEye)
{
    // ǥ�鿡�� �������� ���� ���͸� ���Ѵ�.The vector from the surface to the light.
    float3 lightVec = L.Position - pos;

    // ���� ǥ��� �Ÿ��� ���Ѵ�.
    float d = length(lightVec);

    // �Ÿ� ��ȿ�� �׽�Ʈ�� �Ѵ�.
    if (d > L.FalloffEnd)
        return 0.0f;

    // �� ���͸� ����ȭ �Ѵ�..
    lightVec /= d;

    // ���� ���⸦ ����Ʈ-�ڻ������� �����Ѵ�.
    float ndotl = max(dot(lightVec, normal), 0.0f);
    float3 lightStrength = L.Strength * ndotl;

    // ���� �Ÿ��� �����Ѵ�.
    float att = CalcAttenuation(d, L.FalloffStart, L.FalloffEnd);
    lightStrength *= att;

    // spot light �� �� ����� �����Ѵ�.
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

    // ���� ��ũ�δ� �� ��ƿ ���̴��� include �ϴ� ģ������ ���� �Ѵ�.
    // �׸��� �� �Ʒ� �ٵ� �׿� �°� �ۼ��� �ϴ� ���̴�.
#if (NUM_DIR_LIGHTS > 0)
    for(i = 0; i < NUM_DIR_LIGHTS; ++i)
    {
        result += shadowFactor[i] * ComputeDirectionalLight(gLights[i], mat, normal, toEye);
    }
#endif
    // �̷��� �������� ���缭, �˸��� ����� �ѵ�
#if (NUM_POINT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS; i < NUM_DIR_LIGHTS+NUM_POINT_LIGHTS; ++i)
    {
        result += ComputePointLight(gLights[i], mat, pos, normal, toEye);
    }
#endif
    // ���� �� ����� ���� ���� �׷����� ���̴�.
#if (NUM_SPOT_LIGHTS > 0)
    for(i = NUM_DIR_LIGHTS + NUM_POINT_LIGHTS; i < NUM_DIR_LIGHTS + NUM_POINT_LIGHTS + NUM_SPOT_LIGHTS; ++i)
    {
        result += ComputeSpotLight(gLights[i], mat, pos, normal, toEye);
    }
#endif 

    return float4(result, 0.0f);
}


