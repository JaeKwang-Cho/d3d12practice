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
	
	// 시그마가 커질 수록 가우시안 커브가 넙적해진다.
	int blurRadius = (int)ceil(2.f * _sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);
	// 가우시간 커브를 따르는 가중치 값을 넣는다.
	float weightSum = 0.f;
	for (int i = -blurRadius; i <= blurRadius; i++)
	{
		float x = (float)i;
		
		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}
	// 정규화를 한다.
	for (int i = 0; i < weights.size(); i++)
	{
		weights[i] /= weightSum;
	}
	return weights;
}

void Ssao::BuildDescriptors(ID3D12Resource* _depthStencilBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuSrv, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuSrv, CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuRtv, UINT _cbvSrvUavDescriptorSize, UINT _rtvDescriptorSize)
{
	// Heap에서 연속된 위치에 있는 Srv들의 핸들을 저장한다.
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
	 
    // view를 만든다.
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

        // Screen의 절반 해상도의 AO 맵을 만든다.
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
    // 맴버 viewport, ScissorRect를 설정하고
    _cmdList->RSSetViewports(1, &m_Viewport);
    _cmdList->RSSetScissorRects(1, &m_ScissorRect);

    // 초기 AmientMap0에 그린다.

    // 맴버 리소스 베리어를 RENDER_TARGET으로 바꾸고
    CD3DX12_RESOURCE_BARRIER ambMap0_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(m_AmbientMap0.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    _cmdList->ResourceBarrier(1, &ambMap0_READ_RT);

    float clearValue[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    _cmdList->ClearRenderTargetView(m_hAmbientMap0CpuRtv, clearValue, 0, nullptr);

    // 렌더 타겟 세팅을 해주고
    _cmdList->OMSetRenderTargets(1, &m_hAmbientMap0CpuRtv, true, nullptr);

    // 사용할 passCB를 넘겨준다.
    D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress = _currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    _cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);
    _cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);

    // 노멀 맵과, 깊이 맵을 넣어주고 (연속되어 있으니 2개가 같이 올라간다.)
    _cmdList->SetGraphicsRootDescriptorTable(2, m_hNormalMapGpuSrv);

    // 랜덤 벡터 맵도 넘겨준다.
    _cmdList->SetGraphicsRootDescriptorTable(3, m_hRandomVectorMapGpuSrv);
    // PSO 세팅을 하고
    _cmdList->SetPipelineState(m_SsaoPSO);

    // 대충 점6개로 삼각형을 그린다고 요청을 하면.
    // 넘어간 노멀맵과 깊이맵으로 SSAO Map을 작성한다.
    _cmdList->IASetVertexBuffers(0, 0, nullptr);
    _cmdList->IASetIndexBuffer(nullptr);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->DrawInstanced(6, 1, 0, 0);

    // 이제 다시 GENERIC_READ 상태로 바꾼다..
    CD3DX12_RESOURCE_BARRIER ambMap0_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_AmbientMap0.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &ambMap0_RT_READ);
    // 블러를 먹인다.
    BlurAmbientMap(_cmdList, _currFrame, _blurCount);
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, FrameResource* _currFrame, int _blurCount)
{
    // Blur PSO를 세팅하고
    _cmdList->SetPipelineState(m_BlurPSO);
    // Blur PassCB를 세팅하고
    D3D12_GPU_VIRTUAL_ADDRESS ssaoCBAddress = _currFrame->SsaoCB->Resource()->GetGPUVirtualAddress();
    _cmdList->SetGraphicsRootConstantBufferView(0, ssaoCBAddress);

    // 정해진 횟수만큼, 수평수직 블러를 먹인다.
    for (int i = 0; i < _blurCount; i++)
    {
        BlurAmbientMap(_cmdList, true);
        BlurAmbientMap(_cmdList, false);
    }
}

void Ssao::BlurAmbientMap(ID3D12GraphicsCommandList* _cmdList, bool _bHorzBlur)
{
    // 수직 수평 블러를 먹일때 순서에 따라 입력/출력 역할이 달라진다.
    ID3D12Resource* output = nullptr;
    CD3DX12_GPU_DESCRIPTOR_HANDLE inputSrv;
    CD3DX12_CPU_DESCRIPTOR_HANDLE outputRtv;

    if (_bHorzBlur)
    {
        output = m_AmbientMap1.Get();
        inputSrv = m_hAmbientMap0GpuSrv;
        outputRtv = m_hAmbientMap1CpuRtv;
        // 직접 값을 설정한다.
        _cmdList->SetGraphicsRoot32BitConstant(1, 1, 0);
    }
    else
    {
        output = m_AmbientMap0.Get();
        inputSrv = m_hAmbientMap1GpuSrv;
        outputRtv = m_hAmbientMap0CpuRtv;
        _cmdList->SetGraphicsRoot32BitConstant(1, 0, 0);
    }
    // AO 맵 생성과 비슷하게 작동한다.
    // 베리어 설정하고
    CD3DX12_RESOURCE_BARRIER output_READ_RT = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_RENDER_TARGET);
    _cmdList->ResourceBarrier(1, &output_READ_RT);

    float clearValue[] = { 1.f, 1.f, 1.f, 1.f };
    // 렌더 타겟 설정하고
    _cmdList->ClearRenderTargetView(outputRtv, clearValue, 0, nullptr);
    _cmdList->OMSetRenderTargets(1, &outputRtv, true, nullptr);

    // Normal / Depth는 이미 bind가 되어있고
    // _cmdList->SetGraphicsRootDescriptorTable(2, m_hNormalMapGpuSrv);

    // 입력 역할을 하는 AmbientMap을 bind 한다.
    _cmdList->SetGraphicsRootDescriptorTable(3, inputSrv);

    // 똑같이 점6개 삼각형 2개로 screen에 대해서 작업을 진행한다.
    _cmdList->IASetVertexBuffers(0, 0, nullptr);
    _cmdList->IASetIndexBuffer(nullptr);
    _cmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _cmdList->DrawInstanced(6, 1, 0, 0);

    // RENDER_TARGET에서 READ로 바꾼다.
    CD3DX12_RESOURCE_BARRIER output_RT_READ = CD3DX12_RESOURCE_BARRIER::Transition(output, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &output_RT_READ);
}

void Ssao::BuildResources()
{
    // 스마트 포인터를 이용해서 원래 맴버들이 가지고 있던 리소스를 해제해준다.
    m_NormalMap = nullptr;
    m_AmbientMap0 = nullptr;
    m_AmbientMap1 = nullptr;
    // 속성값을 정의해주고
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

    // normal map은 일단 냅다 z 방향으로 초기화 한다.
    float normalClearColor[] = { 0.0f, 0.0f, 1.0f, 0.0f };
    CD3DX12_CLEAR_VALUE optClear(NormalMapFormat, normalClearColor);
    ThrowIfFailed(m_d3dDevice->CreateCommittedResource(
        &defaultHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        &optClear,
        IID_PPV_ARGS(&m_NormalMap)));

    // Ambient occlusion map은 절반의 해상도를 가지게 한다.
    texDesc.Width = m_RenderTargetWidth / 2;
    texDesc.Height = m_RenderTargetHeight / 2;
    texDesc.Format = Ssao::AmbientMapFormat;
    // 포맷은 R16 단일 성분을 가지는 텍스쳐이긴 하다.
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
    // Upload Heap을 이용해서 CPU 값으로 Default Buffer를 만든다.
    //
    // ** Buffer의 크기를 구하기
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
            // [0, 1] 값이 나오지만 쉐이더에서 [-1, 1]로 확장된다.
            XMFLOAT3 v(MathHelper::RandF(), MathHelper::RandF(), MathHelper::RandF());

            initData[i * 256 + j] = XMCOLOR(v.x, v.y, v.z, 0.0f);
        }
    }
    // 생성된 값들이 들어갈 수 있도록 subresource를 설정해주고
    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = 256 * sizeof(XMCOLOR);
    subResourceData.SlicePitch = subResourceData.RowPitch * 256;

    //
    // 역시 언제 GPU로 업로드가 완료될지 모르니 Upload buffer는 맴버로 가지고 있는다.
    //
    CD3DX12_RESOURCE_BARRIER vectorMap_READ_DEST = CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorMap.Get(), D3D12_RESOURCE_STATE_GENERIC_READ, D3D12_RESOURCE_STATE_COPY_DEST);
    _cmdList->ResourceBarrier(1, &vectorMap_READ_DEST);
    UpdateSubresources(_cmdList, m_RandomVectorMap.Get(), m_RandomVectorMapUploadBuffer.Get(),0, 0, num2DSubresources, &subResourceData);
    CD3DX12_RESOURCE_BARRIER vectorMap_DEST_READ = CD3DX12_RESOURCE_BARRIER::Transition(m_RandomVectorMap.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    _cmdList->ResourceBarrier(1, &vectorMap_DEST_READ);
}

void Ssao::BuildOffsetVectors()
{
    // 큐브의 면, 큐브의 엣지 방향으로 골고루 뻗어나가는
    // 14개의 벡터를 Shader에 넘겨주어서 q (혹은 r)을 선정해서
    // 검사할 때 골고루 검사하도록 만든다.

    // 8개 큐브 엣지
    m_Offsets[0] = XMFLOAT4(+1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[1] = XMFLOAT4(-1.0f, -1.0f, -1.0f, 0.0f);
     
    m_Offsets[2] = XMFLOAT4(-1.0f, +1.0f, +1.0f, 0.0f);
    m_Offsets[3] = XMFLOAT4(+1.0f, -1.0f, -1.0f, 0.0f);
     
    m_Offsets[4] = XMFLOAT4(+1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[5] = XMFLOAT4(-1.0f, -1.0f, +1.0f, 0.0f);
     
    m_Offsets[6] = XMFLOAT4(-1.0f, +1.0f, -1.0f, 0.0f);
    m_Offsets[7] = XMFLOAT4(+1.0f, -1.0f, +1.0f, 0.0f);
     
    // 6개 큐브 면
    m_Offsets[8] = XMFLOAT4(-1.0f, 0.0f, 0.0f, 0.0f);
    m_Offsets[9] = XMFLOAT4(+1.0f, 0.0f, 0.0f, 0.0f);
     
    m_Offsets[10] = XMFLOAT4(0.0f, -1.0f, 0.0f, 0.0f);
    m_Offsets[11] = XMFLOAT4(0.0f, +1.0f, 0.0f, 0.0f);
     
    m_Offsets[12] = XMFLOAT4(0.0f, 0.0f, -1.0f, 0.0f);
    m_Offsets[13] = XMFLOAT4(0.0f, 0.0f, +1.0f, 0.0f);

    for (int i = 0; i < 14; ++i)
    {
        // 길이도 랜덤으로 만든다.
        float s = MathHelper::RandF(0.25f, 1.0f);

        XMVECTOR v = s * XMVector4Normalize(XMLoadFloat4(&m_Offsets[i]));

        XMStoreFloat4(&m_Offsets[i], v);
    }
}

