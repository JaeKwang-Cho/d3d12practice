#include "pch.h"
#include "Grid_RenderMesh.h"
#include "D3D12Renderer.h"

bool Grid_RenderMesh::InitRootSignature()
{
	return ColorRenderMesh::InitRootSignature();
}

bool Grid_RenderMesh::InitPipelineState()
{
	//D3D12PSOCache* pD3DPSOCache = m_pRenderer->INL_GetD3D12PSOCache();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pPipelineState = nullptr;
	std::string psoKey = g_PSOKeys[(UINT)E_PSO_KEYS_INDEX::DEFAULT_WIREFRAME];
	pPipelineState = m_pRenderer->GetPSO(psoKey);
	if (pPipelineState != nullptr) {
		m_pPipelineState = pPipelineState;
		goto RETURN;
	}
	else {
		Microsoft::WRL::ComPtr<ID3D12Device14> pD3DDevice = m_pRenderer->INL_GetD3DDevice();

		Microsoft::WRL::ComPtr<ID3DBlob> pVertexShaderBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pGeoShaderBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pPixelShaderBlob = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> pErrorBlob = nullptr;

#if defined(_DEBUG)
		// ���̴��� ������ �Ҷ�, ����ȭ�� ���ϰ� ������ϱ� ���ϵ��� �ϴ� ���̴�.
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlas = 0;
#endif
		HRESULT hr;

		// vertex shader�� �������ϰ�
		hr = D3DCompileFromFile(L".\\Shaders\\GridTile.hlsl", nullptr, nullptr, "VS", "vs_5_0", compileFlags, 0, pVertexShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			// �޸� : �� �������� D3DCompiler_47.dll �ε� ������ ���.
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		// Geometry shader �������ϰ�
		hr = D3DCompileFromFile(L".\\Shaders\\GridTile.hlsl", nullptr, nullptr, "GS", "gs_5_0", compileFlags, 0, pGeoShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			// �޸� : �� �������� D3DCompiler_47.dll �ε� ������ ���.
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}
		// pixel shader�� ������ �Ѵ�.
		hr = D3DCompileFromFile(L".\\Shaders\\GridTile.hlsl", nullptr, nullptr, "PS", "ps_5_0", compileFlags, 0, pPixelShaderBlob.GetAddressOf(), pErrorBlob.GetAddressOf());
		if (FAILED(hr)) {
			// �޸� : �� �������� D3DCompiler_47.dll �ε� ������ ���.
			if (pErrorBlob != nullptr)
				OutputDebugStringA((char*)pErrorBlob->GetBufferPointer());
			__debugbreak();
		}

		// input layout�� �����ϰ�
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, sizeof(XMFLOAT3),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(XMFLOAT3) + sizeof(XMFLOAT4),D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
		};

		// Pipeline State Object (PSO)�� �����.
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = m_pRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(pVertexShaderBlob->GetBufferPointer(), pVertexShaderBlob->GetBufferSize());
		psoDesc.GS = CD3DX12_SHADER_BYTECODE(pGeoShaderBlob->GetBufferPointer(), pGeoShaderBlob->GetBufferSize());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pPixelShaderBlob->GetBufferPointer(), pPixelShaderBlob->GetBufferSize());

		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = true; // ���� depth ���۸� ����Ѵ�.
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL; // depth���� ���� ��(z�� �۾Ƽ� �տ� �ִ� ��)�� �׸���.
		psoDesc.DepthStencilState.StencilEnable = false;

		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT; // �����ߴ� ���ҽ��� ������ ������ �̿��Ѵ�.

		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleMask = UINT_MAX; // �� ȥ�� 0���� �ʱ�ȸ �ȴ�.

		if (FAILED(pD3DDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(m_pPipelineState.GetAddressOf())))) {
			__debugbreak();
		}

		m_pRenderer->CachePSO(psoKey, m_pPipelineState);
	}

RETURN:
	return true;
}

Grid_RenderMesh::Grid_RenderMesh()
{
}

Grid_RenderMesh::~Grid_RenderMesh()
{
}
