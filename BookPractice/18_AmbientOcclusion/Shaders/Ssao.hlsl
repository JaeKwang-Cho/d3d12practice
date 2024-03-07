//=============================================================================
// Ssao.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//=============================================================================

cbuffer cbSsao : register(b0)
{
    float4x4 gProjMat;
    float4x4 gInvProjMat;
    float4x4 gProjTexMat;
    float4 gOffsetVectors[14];
    
    // SsaoBlur ����ġ��
    float4 gBlurWeights[3];
    
    float2 gInvRenderTargetSize;
    
    // view ���� occlusion �Ӽ�����
    float gOcclusionRadius;
    float gOcclusionFadeStart;
    float gOcclusionFadeEnd;
    float gSurfaceEpsilon;
};

cbuffer cbRootConstants : register(b1)
{
    bool gHorizontalBlur;
}

// ��ġ ��ǥ�� cbuffer�� ���� �� �ִ�.
Texture2D gNormalMap : register(t0);
Texture2D gDepthMap : register(t1);
Texture2D gRandomVecMap : register(t2);

// ���÷��� ���� ģ������ ����ϴ� �� ������...
SamplerState gsamPointClamp : register(s0);
SamplerState gsamLinearClamp : register(s1);
SamplerState gsamDepthMap : register(s2);
SamplerState gsamLinearWrap : register(s3);

static const int gSampleCount = 14;

// ȭ�� �簢�� �ؽ��� ��ǥ
static const float2 gTexCoords[6] =
{
    float2(0.0f, 1.0f),
    float2(0.0f, 0.0f),
    float2(1.0f, 0.0f),
    float2(0.0f, 1.0f),
    float2(1.0f, 0.0f),
    float2(1.0f, 1.0f)
};
 
struct VertexOut
{
    float4 PosH : SV_POSITION;
    float3 PosV : POSITION;
    float2 TexC : TEXCOORD0;
};

// NDC �������� ProjMat�� �Ųٷ� Ÿ���� view ������ ���� ��� ���̴�.
VertexOut VS(uint vid : SV_VertexID)
{
    VertexOut vout;

    vout.TexC = gTexCoords[vid];

    // �ϴ� �ؽ��� ��ǥ�� NDC ��ǥ�� �ٲٰ�
    vout.PosH = float4(2.0f * vout.TexC.x - 1.0f, 1.0f - 2.0f * vout.TexC.y, 0.0f, 1.0f);
 
    // NDC ��ǥ�� InvProjMat�� �̿��ؼ�
    // view ������ �ش��ϴ� ���� ����� ������ ���� ��Ų��.
    float4 ph = mul(vout.PosH, gInvProjMat);
    vout.PosV = ph.xyz / ph.w;

    return vout;
}

// �������� ���õ� q�� p�� �󸶳� �������� distZ(p - q)�� ����ϴ� �Լ��� ����Ѵ�.
float OcclusionFunction(float distZ)
{
	// 1. ���� �ʿ��� q�� ���� p�� ������ ũ�ٸ�(�ڿ� �ִٸ�) p�� �������� ���� ���̴�.
    // 2. q�� p�� ���� ����� �����ٸ� (�� ������ ���� �ԽǷ� ���� �۴ٸ�) p�� �������� ���� ���̴�.
    //    ���� ��鿡 �ִٰ� �����Ѵ�. (�׳� �Ѵ� �ٸ� ģ���� ���� �������ٰ� �����Ѵ�.)
	//
	//  Eps ���� occlusion�� �Ͼ��
    //  z0�� occlusion�� ���� �������� �����ϴ� �Ÿ�
	//  z1�� occlusion�� �Ͼ�⿡�� �ʹ� �� �Ÿ�
	//
	//       1.0     -------------\
	//               |           |  \
	//               |           |    \
	//               |           |      \ 
	//               |           |        \
	//               |           |          \
	//               |           |            \
	//  ------|------|-----------|-------------|---------|--> zv
	//        0     Eps          z0            z1        
	//
	
    float occlusion = 0.0f;
    if (distZ > gSurfaceEpsilon)
    {
        float fadeLength = gOcclusionFadeEnd - gOcclusionFadeStart;
		
		// z0 ���� z1���� �������� �����Ѵ�.
        occlusion = saturate((gOcclusionFadeEnd - distZ) / fadeLength);
    }
	
    return occlusion;
}

float NdcDepthToViewDepth(float z_ndc)
{
    // z_ndc = A + B/viewZ, where gProj[2,2]=A and gProj[3,2]=B.
    // ProjMat�� ����ϰ�, Homogenous���� ����� viewZ(����)�� 
    // �ѹ� �� �����ִ� ���� NDC ��ǥ�ε�, �װ� �Ųٷ� �� ���̴�.
    float viewZ = gProjMat[3][2] / (z_ndc - gProjMat[2][2]);
	// float viewZ = (z_ndc - gProj[2][2])/gProj[3][2] 
    return viewZ;
}

float4 PS(VertexOut pin) : SV_Target
{
	// p -- occlusion ��ġ�� ���� �ȼ�
	// n -- p�� ���
	// q -- �������� ���õ� q
	// r -- q�� ������ p�� �����ͱ�� ���Ǵ� ���̹��۷� ���� ��.

	// view �������� p�� ����� ���ϰ�.  
    float3 n = normalize(gNormalMap.SampleLevel(gsamPointClamp, pin.TexC, 0.0f).xyz);
    // p�� depth�� ���Ѵ�.
    float pz = gDepthMap.SampleLevel(gsamDepthMap, pin.TexC, 0.0f).r;
    pz = NdcDepthToViewDepth(pz);

	// ������ ������ �̿��ؼ� view������ p(x, y, z)�� ���Ѵ�.
    // Vertex Shader���� InvProjMat���� ���ߴ� PosV�� p�� ������ �̷�ٰ� �����Ѵ�.
	// p = t * pin.PosV �̰�, z ������ �˰� ������  t = p.z / pin.PosV.z���� 
    // t�� ���ؼ� p�� ���� ���� �ٽ� t�� pin.PosV�� ��Ÿ�� �� �ִ�.
    float3 p = (pz / pin.PosV.z) * pin.PosV;
	
	// App���� ����� �ξ��� ���� ���͸� [0, 1] -> [-1, +1]���� ����Ѵ�. (�ؽ��� ũ�⸦ ���̱� �̷��� �ߴ�.)
    float3 randVec = 2.0f * gRandomVecMap.SampleLevel(gsamLinearWrap, 4.0f * pin.TexC, 0.0f).rgb - 1.0f;

    float occlusionSum = 0.0f;
	
	// p�� �߽����� n�� �������� ���� �ݱ����� ���� ���ؼ� ���캻��..
    for (int i = 0; i < gSampleCount; ++i)
    {
		// �̸� ���� cb�� �Ѿ�� ������ ���͵��� �̿��ؼ�,
        // ���� �����ϰ� �ֺ��� ���캼 �� �ִ�.
        // ������ ���� ���� * ������ ���� ����
        float3 offset = reflect(gOffsetVectors[i].xyz, randVec);
	
		// ���� ���ø��� ���� p�� n�� �̷�� ����� �ݴ�� �����̶�� �����´�. (����)
        float flip = sign(dot(offset, n)); // 0���� ������ -1 ��ȯ
		
		// ���� ���Ϳ� ���α׷��Ӱ� ���� ���������� ���ؼ�, ��¥ q�� ���Ѵ�.
        float3 q = p + flip * gOcclusionRadius * offset;
		
		// view ���� �� q�� ������Ų ���� �ؽ��� ��ǥ�� �ٲ۴�.
        float4 projQ = mul(float4(q, 1.0f), gProjTexMat);
        projQ /= projQ.w;

		// �̸� Projection �ؼ� depth�� ����� ���� �ؽ��Ŀ���
        // q�� xy�� �̿��ؼ� q�� ������ ���������� ���� ����� �ȼ��� ���̸� ���Ѵ�.
		// (q�� ���̰� �ƴϰ�, �ػ� ���� ������ ������ �߻��� �� �ִ�. �� ������ ���� �ִ�.)
        float rz = gDepthMap.SampleLevel(gsamDepthMap, projQ.xy, 0.0f).r;
        // �� ���� ���� rz�̶�� �ϰ�, �ٽ� view ���������� ���̷� �ٲ۴�.
        rz = NdcDepthToViewDepth(rz);

		// ���� �Ȱ��� ��¥ r�� ��ġ�� q�� ������ �������̶�� ���� �̿��ؼ� ���Ѵ�.
        // r = (rx, ry, rz) / r = t * q
        // r.z = t * q.z -> t = r.z / q.z
        float3 r = (rz / q.z) * q;
		
		//
		// ��¥ �����ϴ� r�� p�� �������� �����Ѵ�.
		//   * dot(n, normalize(r - p))�� ����� r�� ��� (p, n)���� 
        //     �󸶳� ������ ������ �ִ��� ��Ÿ����. 
        //     ��� (p, n)�� �̷�� ���� Ŭ ����(�� ����) ���� Ŀ����,
        //     �̷�� ���� 0�� �Ǹ�(r�� ���(p, n)���� ������ ��) 
        //     ���� 0���� �Ǿ, ���� ������ ���� �� �ִ�.
		//   * ���� �Լ������� 0, Eps, FadeStart, FadeEnd ������ ������
        //     ������ occlusion weight�� �������ش�.
		// 
		
        float distZ = p.z - r.z;
        float dp = max(dot(n, normalize(r - p)), 0.0f);

        float occlusion = dp * OcclusionFunction(distZ);

        occlusionSum += occlusion;
    }
	
    occlusionSum /= gSampleCount;
	
    float access = 1.0f - occlusionSum;

	// power�� ���༭ �� �� Ȯ���� occlusion ȿ���� �ش�.
    return saturate(pow(access, 6.0f));
}
