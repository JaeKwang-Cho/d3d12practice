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
	// 텍스쳐에 블러를 먹이기 위해서는, 입력 텍스쳐의 dimension과 width, height가 일치해야 한다.
	// 그래서 resize가 일어났을 때, Recreate를 해야 한다.
	BlurFilter(ID3D12Device* _device, UINT _width, UINT _height, DXGI_FORMAT _format);

	BlurFilter(const BlurFilter& _rhs) = delete;
	BlurFilter& operator=(const BlurFilter& _rhs) = delete;

	~BlurFilter() = default;

	// 블러먹인 결과(0번째 맴버 텍스쳐)를 내보내는 함수
	ID3D12Resource* Output();

	// CS에 적절한 Texture를 넘길때 필요한 친구를 맴버에서 관리한다.
	void BuildDescriptors(
		CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor,
		CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor,
		UINT _descriptorSize
	);

	// 위에서 언급했듯이, 클라이언트 영역을 블러먹이려면 클래스에서 사용하는 텍스쳐를 다시 생성해야 한다.
	void OnResize(UINT _newWidth, UINT _newHeight);

	// 내부 ID3DResource 맴버들을 이용해서 수평, 수직 Blur를 먹인다.
	void Execute(
		ID3D12GraphicsCommandList* _cmdList,
		ID3D12RootSignature* _rootSig,
		ID3D12PipelineState* _horzBlurPSO,
		ID3D12PipelineState* _vertBlurPSO,
		ID3D12Resource* _input,
		int _blurCount
	);

private:
	// 시그마 값이 강할 수록 블러가 세게 먹는다.
	std::vector<float> CalcGaussWeights(float _sigma);

	void BuildDescriptors();
	void BuildResources();

private:
	const int MaxBlurRadius = 5;

	ID3D12Device* m_d3dDevice = nullptr;

	UINT m_Width = 0;
	UINT m_Height = 0;
	DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// 이렇게 멤버를 2개 가지는 이유는,
	// 모든 픽셀에 대해서 radius로 블러를 하는 것보다
	// 수직 blur, 수평 blur를 한번씩 먹이는 것이 빠르기 때문이다.
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

