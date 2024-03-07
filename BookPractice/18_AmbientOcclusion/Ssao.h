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
    /// resize��
    ///</summary>
    void OnResize(UINT _newWidth, UINT _newHeight);

    ///<summary>
    /// ���� �ɹ� �ؽ��ĸ� ���� Ÿ������ �����ϰ�, ���� ȭ�鿡 ���� ambient�� ����ؼ� �׸���.
    /// (AO���� depth read/write�� �ʿ� ��� �����´�.)
    ///</summary>
    void ComputeSsao(
        ID3D12GraphicsCommandList* _cmdList,
        FrameResource* _currFrame,
        int _blurCount);


private:
    ///<summary>
    /// ���� ���� ��� Occlusion factor�� ����ϱ� ������, �ֺ� ���� �̿��ؼ� ������ ���� ���δ�.
    /// edge-preserving blur�� �ؼ�, �ҿ��� ���� �κ����� �Ǵ��� �Ǹ�, blur�� �������� �ʴ´�.
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
    // ���� ���� ���� ���� �� ���������� 2���� �ʿ��ϴ�.
    Microsoft::WRL::ComPtr<ID3D12Resource> m_AmbientMap0;
    Microsoft::WRL::ComPtr<ID3D12Resource> m_AmbientMap1;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hNormalMapGpuSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hNormalMapCpuRtv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hDepthMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hDepthMapGpuSrv;

    CD3DX12_CPU_DESCRIPTOR_HANDLE m_hRandomVectorMapCpuSrv;
    CD3DX12_GPU_DESCRIPTOR_HANDLE m_hRandomVectorMapGpuSrv;

    // ���� ���� ���� ���� �� ���������� 2���� �ʿ��ϴ�.
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

