//***************************************************************************************
// Ssao.h by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"
#include "FrameResource.h"

class Ssao
{
public:
    Ssao(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, UINT _width, UINT _height);
    Ssao(const Ssao& _rhs) = delete;
    Ssao& operator=(const Ssao& _rhs) = delete;
    ~Ssao() = default;

    static const DXGI_FORMAT AmbientMapFormat = DXGI_FORMAT_R16_UNORM;
    static const DXGI_FORMAT NormalMapFormat = DXGI_FORMAT_R16G16B16A16_FLOAT;

    static const int MaxBlurRadius = 5;
    
    void GetOffsetVectors(DirectX::XMFLOAT4 _offsets[14]);
    std::vector<float> CalcGaussWeights(float _sigma);

    void BuildDescriptors(
        ID3D12Resource* _depthStencilBuffer,
        CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv,
        CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv,
        CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv,
        UINT _cbvSrvUavDescriptorSize,
        UINT _rtvDescriptorSize);

    void RebuildDescriptors(ID3D12Resource* _depthStencilBuffer);

    void SetPSOs(ID3D12PipelineState* _ssaoPso, ID3D12PipelineState* _ssaoBlurPso);

    ///<summary>
    /// resize용
    ///</summary>
    void OnResize(UINT _newWidth, UINT _newHeight);

    ///<summary>
    /// 내부 맴버 텍스쳐를 렌더 타겟으로 설정하고, 현재 화면에 대한 ambient를 계산해서 그린다.
    /// (AO에는 depth read/write가 필요 없어서 꺼놓는다.)
    ///</summary>
    void ComputeSsao(
        ID3D12GraphicsCommandList* _cmdList,
        FrameResource* _currFrame,
        int _blurCount);


private:
    ///<summary>
    /// 랜덤 벡터 몇개로 Occlusion factor를 계산하기 때문에, 주변 점을 이용해서 적당히 블러를 먹인다.
    /// edge-preserving blur를 해서, 불연속 적인 부분으로 판단이 되면, blur에 포함하지 않는다.
    ///</summary>
    void BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount);
    void BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, bool _horzBlur);

    void BuildResources();
    void BuildRandomVectorTexture(ID3D12GraphicsCommandList* _cmdList);

    void BuildOffsetVectors();


private:
    ID3D12Device* m_d3dDevice;

    Microsoft::WRL::ComPtr<ID3D12RootSignature> m_SsaoRootSig;

    ID3D12PipelineState* m_SsaoPso = nullptr;
    ID3D12PipelineState* m_BlurPso = nullptr;

    Microsoft::WRL::ComPtr<ID3D12Resource> m_RandomVectorMap;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_RandomVectorMapUploadBuffer;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_NormalMap;
    // 수평 수직 블러를 먹일 때 핑퐁용으로 2개가 필요하다.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_AmbientMap0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_AmbientMap1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hNormalMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hDepthMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hDepthMapGpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hRandomVectorMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hRandomVectorMapGpuSrv;

    // 수평 수직 블러를 먹일 때 핑퐁용으로 2개가 필요하다.
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap0CpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hAmbientMap0GpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap0CpuRtv;
                                   
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap1CpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hAmbientMap1GpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hAmbientMap1CpuRtv;

    UINT m_RenderTargetWidth;
    UINT m_RenderTargetHeight;

    DirectX::XMFLOAT4 m_Offsets[14];

    D3D12_VIEWPORT m_Viewport;
    D3D12_RECT mS_cissorRect;

public:
    UINT GetSsaoMapWidth() const
    {
        return m_RenderTargetWidth / 2.f;
    }

    UINT GetSsaoMapHeight() const
    {
        return m_RenderTargetHeight / 2.f;
    }

    ID3D12Resource* GetNormalMap()
    {
        return m_NormalMap.Get();
    }
    ID3D12Resource* GetAmbientMap()
    {
        return m_AmbientMap0.Get();
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE NormalMapRtv() const
    {
        return m_hNormalMapCpuRtv;
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE NormalMapSrv() const
    {
        return m_hNormalMapGpuSrv;
    }

    CD3DX12_GPU_DESCRIPTOR_HANDLE AmbientMapSrv() const
    {
        return m_hAmbientMap0GpuSrv;
    }
};

