//***************************************************************************************
// Ssao.cpp by Frank Luna (C) 2011 All Rights Reserved.
//***************************************************************************************

#include "Ssao.h"
#include "directxpackedvector.h" // XMCOLOR

using namespace DirectX::PackedVector;

Ssao::Ssao(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, UINT _width, UINT _height)
{
	m_d3dDevice = _device;

	OnResize(_width, _height);

	BuildOffsetVectors();
	BuildRandomVectorTexture(_cmdList);
}

void Ssao::GetOffsetVectors(DirectX::XMFLOAT4 _offsets[14])
{
	std::copy(&m_Offsets[0], &m_Offsets[14], &_offsets[0]);
}

std::vector<float> Ssao::CalcGaussWeights(float _sigma)
{
	float twoSigma2 = 2.f * _sigma * _sigma;
	
	// �ñ׸��� Ŀ�� ���� ����þ� Ŀ�갡 ����������.
	int blurRadius = (int)ceil(2.f * _sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);
	// ����ð� Ŀ�긦 ������ ����ġ ���� �ִ´�.
	float weightSum = 0.f;
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = (float)i;
		
		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}
	// ����ȭ�� �Ѵ�.
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}
	return weights;
}

void Ssao::BuildDescriptors(ID3D12Resource* _depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv, UINT _cbvSrvUavDescriptorSize, UINT _rtvDescriptorSize)
{
	// Heap���� ���ӵ� ��ġ�� �ִ� Srv���� �ڵ��� �����Ѵ�.
	m_hAmbientMap0CpuSrv = _hCpuSrv;
    m_hAmbientMap1CpuSrv = _hCpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hNormalMapCpuSrv = _hCpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hDepthMapCpuSrv = _hCpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hRandomVectorMapCpuSrv = _hCpuSrv.Offset(1, _cbvSrvUavDescriptorSize);

    m_hAmbientMap0GpuSrv = _hGpuSrv;
    m_hAmbientMap1GpuSrv = _hGpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hNormalMapGpuSrv = _hGpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hDepthMapGpuSrv = _hGpuSrv.Offset(1, _cbvSrvUavDescriptorSize);
    m_hRandomVectorMapGpuSrv = _hGpuSrv.Offset(1, _cbvSrvUavDescriptorSize);

    m_hNormalMapCpuRtv = _hCpuRtv;
    m_hAmbientMap0CpuRtv = _hCpuRtv.Offset(1, _rtvDescriptorSize);
    m_hAmbientMap1CpuRtv = _hCpuRtv.Offset(1, _rtvDescriptorSize);
	 
    // view�� �����.
    RebuildDescriptors(_depthStencilBuffer);
}

void Ssao::RebuildDescriptors(ID3D12Resource* _depthStencilBuffer)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Format = NormalMapFormat;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.MipLevels = 1;
    m_d3dDevice->CreateShaderResourceView(m_NormalMap.Get(), &srvDesc, m_hNormalMapCpuSrv);

    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    m_d3dDevice->CreateShaderResourceView(_depthStencilBuffer, &srvDesc, m_hDepthMapCpuSrv);

    srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_d3dDevice->CreateShaderResourceView(m_RandomVectorMap.Get(), &srvDesc, m_hRandomVectorMapCpuSrv);

    srvDesc.Format = AmbientMapFormat;
    m_d3dDevice->CreateShaderResourceView(m_AmbientMap0.Get(), &srvDesc, m_hAmbientMap0CpuSrv);
    m_d3dDevice->CreateShaderResourceView(m_AmbientMap1.Get(), &srvDesc, m_hAmbientMap1CpuSrv);

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Format = NormalMapFormat;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;
    m_d3dDevice->CreateRenderTargetView(m_NormalMap.Get(), &rtvDesc, m_hNormalMapCpuRtv);

    rtvDesc.Format = AmbientMapFormat;
    m_d3dDevice->CreateRenderTargetView(m_AmbientMap0.Get(), &rtvDesc, m_hAmbientMap0CpuRtv);
    m_d3dDevice->CreateRenderTargetView(m_AmbientMap1.Get(), &rtvDesc, m_hAmbientMap1CpuRtv);
}

void Ssao::OnResize(UINT _newWidth, UINT _newHeight)
{
    if (m_RenderTargetWidth != _newWidth || m_RenderTargetHeight != _newHeight)
    {
        m_RenderTargetWidth = _newWidth;
        m_RenderTargetHeight = _newHeight;

        // Screen�� ���� �ػ��� AO ���� �����.
        m_Viewport.TopLeftX = 0.0f;
        m_Viewport.TopLeftY = 0.0f;
        m_Viewport.Width = m_RenderTargetWidth / 2.0f;
        m_Viewport.Height = m_RenderTargetHeight / 2.0f;
        m_Viewport.MinDepth = 0.0f;
        m_Viewport.MaxDepth = 1.0f;

        m_ScissorRect = { 0, 0, (int)m_RenderTargetWidth / 2, (int)m_RenderTargetHeight / 2 };

        BuildResources();
    }
}

void Ssao::ComputeSsao(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount)
{
    // �ɹ� viewport, ScissorRect�� �����ϰ�
    _cmdList->RSSetViewports(1, &m_Viewport);
    _cmdList->RSSetScissorRects(1, &m_ScissorRect);

    // �ʱ� AmientMap0�� �׸���.

    // �ɹ� ���ҽ� ����� RENDER_TARGET���� �ٲٰ�
    CD3DX12_RESOURCE_BARRIER ambMap0_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_AmbientMap0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    _cmdList->ResourceBarrier(1, &ambMap0_READ_RT);

    float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    _cmdList->ClearRenderTargetView(m_hAmbientMap0CpuRtv, clearValue, 0, nullptr);

    // ���� Ÿ�� ������ ���ְ�
    _cmdList->OMSetRenderTargets(1, &m_hAmbientMap0CpuRtv, true, nullptr);

    // ����� passCB�� �Ѱ��ش�.
    D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress = _currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    _cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
    _cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    // ��� �ʰ�, ���� ���� �־��ְ� (���ӵǾ� ������ 2���� ���� �ö󰣴�.)
    _cmdList->SetGraphicsRootDescriptorTable(2, m_hNormalMapGpuSrv);

    // ���� ���� �ʵ� �Ѱ��ش�.
    _cmdList->SetGraphicsRootDescriptorTable(3, m_hRandomVectorMapGpuSrv);
    // PSO ������ �ϰ�
    _cmdList->SetPipelineState(m_SsaoPSO);

    // ���� ��6���� �ﰢ���� �׸��ٰ� ��û�� �ϸ�.
    // �Ѿ ��ָʰ� ���̸����� SSAO Map�� �ۼ��Ѵ�.
    _cmdList->IASetVertexBuffers(0, 0, nullptr);
    _cmdList->IASetIndexBuffer(nullptr);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->DrawInstanced(6, 1, 0, 0);

    // ���� �ٽ� GENERIC_READ ���·� �ٲ۴�..
    CD3DX12_RESOURCE_BARRIER ambMap0_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_AmbientMap0.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &ambMap0_RT_READ);
    // ���� ���δ�.
    BlurAmbientMap(_cmdList, _currFrame, _blurCount);
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount)
{
    // Blur PSO�� �����ϰ�
    _cmdList->SetPipelineState(m_BlurPSO);
    // Blur PassCB�� �����ϰ�
    D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress = _currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    _cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

    // ������ Ƚ����ŭ, ������� ���� ���δ�.
    for (int i = 0; i < _blurCount; i++)
    {
        BlurAmbientMap(_cmdList, true);
        BlurAmbientMap(_cmdList, false);
    }
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, bool _bHorzBlur)
{
    // ���� ���� ���� ���϶� ������ ���� �Է�/��� ������ �޶�����.
    ID3D12Resource* output = nullptr;
    CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

    if (_bHorzBlur)
    {
        output = m_AmbientMap1.Get();
        inputSrv = m_hAmbientMap0GpuSrv;
        outputRtv = m_hAmbientMap1CpuRtv;
        // ���� ���� �����Ѵ�.
        _cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
    }
    else
    {
        output = m_AmbientMap0.Get();
        inputSrv = m_hAmbientMap1GpuSrv;
        outputRtv = m_hAmbientMap0CpuRtv;
        _cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
    }
    // AO �� ������ ����ϰ� �۵��Ѵ�.
    // ������ �����ϰ�
    CD3DX12_RESOURCE_BARRIER output_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    _cmdList->ResourceBarrier(1, &output_READ_RT);

    float clearValue[] = { 1.f, 1.f, 1.f, 1.f };
    // ���� Ÿ�� �����ϰ�
    _cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);
    _cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

    // Normal / Depth�� �̹� bind�� �Ǿ��ְ�
    // _cmdList->SetGraphicsRootDescriptorTable(2, m_hNormalMapGpuSrv);

    // �Է� ������ �ϴ� AmbientMap�� bind �Ѵ�.
    _cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);

    // �Ȱ��� ��6�� �ﰢ�� 2���� screen�� ���ؼ� �۾��� �����Ѵ�.
    _cmdList->IASetVertexBuffers(0, 0, nullptr);
    _cmdList->IASetIndexBuffer(nullptr);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->DrawInstanced(6, 1, 0, 0);

    // RENDER_TARGET���� READ�� �ٲ۴�.
    CD3DX12_RESOURCE_BARRIER output_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &output_RT_READ);
}

void Ssao::BuildResources()
{
    // ����Ʈ �����͸� �̿��ؼ� ���� �ɹ����� ������ �ִ� ���ҽ��� �������ش�.
    m_NormalMap = nullptr;
    m_AmbientMap0 = nullptr;
    m_AmbientMap1 = nullptr;
    // �Ӽ����� �������ְ�
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = m_RenderTargetWidth;
    texDesc.Height = m_RenderTargetHeight;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = Ssao::NormalMapFormat;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

    // normal map�� �ϴ� ���� z �������� �ʱ�ȭ �Ѵ�.
    float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE optClear(NormalMapFormat, normalClearColor);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&m_NormalMap)));

    // Ambient occlusion map�� ������ �ػ󵵸� ������ �Ѵ�.
    texDesc.Width = m_RenderTargetWidth / 2;
    texDesc.Height = m_RenderTargetHeight / 2;
    texDesc.Format = Ssao::AmbientMapFormat;
    // ������ R16 ���� ������ ������ �ؽ����̱� �ϴ�.
    float ambientClearColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    optClear = CD3DX12_CLEAR_VALUE(AmbientMapFormat, ambientClearColor);

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&m_AmbientMap0)));

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&m_AmbientMap1)));
}

void Ssao::BuildRandomVectorTexture(ID3D12GraphicsCommandList* _cmdList)
{
    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = 256;
    texDesc.Height = 256;
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    CD3DX12_HEAP_PROPERTIES defaultHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_RandomVectorMap)));

    //
    // Upload Heap�� �̿��ؼ� CPU ������ Default Buffer�� �����.
    //
    // ** Buffer�� ũ�⸦ ���ϱ�
    const UINT num2DSubresources = texDesc.DepthOrArraySize * texDesc.MipLevels;
    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(m_RandomVectorMap.Get(), 0, num2DSubresources);

    CD3DX12_HEAP_PROPERTIES uploadHeapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &uploadHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(m_RandomVectorMapUploadBuffer.GetAddressOf())));

    XMCOLOR initData[256 * 256];
    for (int i = 0; i < 256; ++i)
    {
        for (int j = 0; j < 256; ++j)
        {
            // [0, 1] ���� �������� ���̴����� [-1, 1]�� Ȯ��ȴ�.
            XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

            initData[i * 256 + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
        }
    }
    // ������ ������ �� �� �ֵ��� subresource�� �������ְ�
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
    subResourceData.SlicePitch = subResourceData.RowPitch * 256;

    //
    // ���� ���� GPU�� ���ε尡 �Ϸ���� �𸣴� Upload buffer�� �ɹ��� ������ �ִ´�.
    //
    CD3DX12_RESOURCE_BARRIER vectorMap_READ_DEST = CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorMap.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    _cmdList->ResourceBarrier(1, &vectorMap_READ_DEST);
    UpdateSubresources(_cmdList, m_RandomVectorMap.Get(), m_RandomVectorMapUploadBuffer.Get(),0, 0, num2DSubresources, &subResourceData);
    CD3DX12_RESOURCE_BARRIER vectorMap_DEST_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorMap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &vectorMap_DEST_READ);
}

void Ssao::BuildOffsetVectors()
{
    // ť���� ��, ť���� ���� �������� ���� �������
    // 14���� ���͸� Shader�� �Ѱ��־ q (Ȥ�� r)�� �����ؼ�
    // �˻��� �� ���� �˻��ϵ��� �����.

    // 8�� ť�� ����
    m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);
     
    m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);
     
    m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);
     
    m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);
     
    // 6�� ť�� ��
    m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
    m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);
     
    m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
    m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);
     
    m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
    m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

    for (int i = 0; i < 14; ++i)
    {
        // ���̵� �������� �����.
        float s = MathHelper::RandF(0.25f, 1.0f);

        XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

        XMStoreFloat4(&m_Offsets[i], v);
    }
}

