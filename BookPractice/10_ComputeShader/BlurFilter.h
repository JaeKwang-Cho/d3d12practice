//***************************************************************************************
// BlurFilter.h by Frank Luna (C) 2011 All Rights Reserved.
//
// Performs a blur operation on the topmost mip level of an input texture.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"

class BlurFilter
{
public:
	// �ؽ��Ŀ� ���� ���̱� ���ؼ���, �Է� �ؽ����� dimension�� width, height�� ��ġ�ؾ� �Ѵ�.
	// �׷��� resize�� �Ͼ�� ��, Recreate�� �ؾ� �Ѵ�.
	BlurFilter(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format);

	BlurFilter(const BlurFilter& _rhs) = delete;
	BlurFilter& operator=(const BlurFilter& _rhs) = delete;

	~BlurFilter() = default;

	// ������ ���(0��° �ɹ� �ؽ���)�� �������� �Լ�
	ID3D12Resource* Output();

	// CS�� ������ Texture�� �ѱ涧 �ʿ��� ģ���� �ɹ����� �����Ѵ�.
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor,
		UINT _descriptorSize
	);

	// ������ ����ߵ���, Ŭ���̾�Ʈ ������ �����̷��� Ŭ�������� ����ϴ� �ؽ��ĸ� �ٽ� �����ؾ� �Ѵ�.
	void OnResize(UINT _newWidth, UINT _newHeight);

	// ���� ID3DResource �ɹ����� �̿��ؼ� ����, ���� Blur�� ���δ�.
	void Execute(
		ID3D12GraphicsCommandList* _cmdList,
		ID3D12RootSignature* _rootSig,
		ID3D12PipelineState* _horzBlurPSO,
		ID3D12PipelineState* _vertBlurPSO,
		ID3D12Resource* _input,
		int _blurCount
	);

private:
	// �ñ׸� ���� ���� ���� ���� ���� �Դ´�.
	std::vector<float> CalcGaussWeights(float _sigma);

	void BuildDescriptors();
	void BuildResources();

private:
	const int MaxBlurRadius = 5;

	ID3D12Device* m_d3dDevice = nullptr;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// �̷��� ����� 2�� ������ ������,
	// ��� �ȼ��� ���ؼ� radius�� ���� �ϴ� �ͺ���
	// ���� blur, ���� blur�� �ѹ��� ���̴� ���� ������ �����̴�.
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur0CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur0CpuUav;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur1CpuSrv;
	CD3DX12_CPU_DESCRIPTOR_HANDLE m_Blur1CpuUav;

	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur0GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur0GpuUav;
										 
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur1GpuSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_Blur1GpuUav;

	Microsoft::WRL::ComPtr<ID3D12Resource> m_BlurMap0 = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_BlurMap1 = nullptr;
};

