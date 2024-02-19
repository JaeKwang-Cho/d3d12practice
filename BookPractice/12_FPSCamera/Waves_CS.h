//***************************************************************************************
// Waves_CS.h by Frank Luna (C) 2011 All Rights Reserved.
//
// 파도 시뮬레이션은 GPU의 compute shader를 이요해서 처리한다.
// float Texture를 이용해서 데이터를 주고 받으며,
// App에서는 texture Srv 세팅과 CS 결과로 나온 값을 VS 쉐이더에 넘겨주어야 한다.
//***************************************************************************************

#pragma once

#include "../Common/d3dUtil.h"
#include "../Common/GameTimer.h"

class Waves_CS
{
public:
	// row 과 col은 16으로 나누어 떨어져야 한다. 
	Waves_CS(ID3D12Device* _device, ID3D12GraphicsCommandList* _cmdList, int _row, int _col, float _dx, float _dt, float _speed, float _damping);
	Waves_CS(const Waves_CS& _rhs) = delete;
	Waves_CS& operator=(const Waves_CS& _rhs) = delete;
	~Waves_CS() = default;

	void BuildDescriptors(CD3DX12_CPU_DESCRIPTOR_HANDLE _hCpuDescriptor, CD3DX12_GPU_DESCRIPTOR_HANDLE _hGpuDescriptor, UINT _descriptorSize);

	void Update(const GameTimer& _gt, ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso);

	void Disturb(ID3D12GraphicsCommandList* _cmdList, ID3D12RootSignature* _rootSig, ID3D12PipelineState* _pso, UINT _i, UINT _j, float _magnitude);

private:

	void BuildResources(ID3D12GraphicsCommandList* _cmdList);

private:
	// 프로퍼티
	UINT m_NumRows;
	UINT m_NumCols;
	UINT m_VertexCount;
	UINT m_TriangleCount;

	// 미리 계산 가능한 상수들
	float m_K[3];

	float m_TimeStep;
	float m_SpatialStep;

	ID3D12Device* m_d3dDevice = nullptr;

	// 텍스쳐 Srv 핸들
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_PrevSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrSolSrv;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NextSolSrv;
	// 텍스쳐 Uav 핸들
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_PrevSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrSolUav;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_NextSolUav;

	// 중간 과정을 저장하는 Texture
	Microsoft::WRL::ComPtr<ID3D12Resource> m_PrevSol = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_CurrSol = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_NextSol = nullptr;
	// 업로드용 버퍼
	Microsoft::WRL::ComPtr<ID3D12Resource> m_PrevUploadBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_CurrUploadBuffer = nullptr;

public:
	// 프로퍼티
	UINT GetNumRows() const
	{
		return m_NumRows;
	}

	UINT GetNumCols() const
	{
		return m_NumCols;
	}

	UINT GetVertexCount() const
	{
		return m_VertexCount;
	}

	UINT GetTriangleCount() const
	{
		return m_TriangleCount;
	}

	float GetWidth() const
	{
		return m_NumCols * m_SpatialStep;
	}

	float GetHeight() const
	{
		return m_NumRows * m_SpatialStep;
	}

	float GetSpatialStep() const
	{
		return m_SpatialStep;
	}

	CD3DX12_GPU_DESCRIPTOR_HANDLE GetDisplacementMap() const
	{
		return m_CurrSolSrv;
	}

	UINT GetDescriptorCount() const
	{
		return 6;
	}
};

