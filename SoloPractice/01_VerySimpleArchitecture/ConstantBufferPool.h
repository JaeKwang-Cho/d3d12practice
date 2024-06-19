// ConstantBufferPool.h from "megayuchi"

#pragma once

// Object가 렌더링 될 때 같이 shader에 넘어갈 친구들
struct CB_CONTAINER {
	D3D12_CPU_DESCRIPTOR_HANDLE cbvHandle;
	D3D12_GPU_VIRTUAL_ADDRESS pGPUMemAddr; // table에 넘기는용
	UINT8* pSystemMemAddr; // 업데이트용
};

// 이 클래스의 목적은 하나의 Object를 한 프레임에 여러번 다르게 렌더링 하기 위함이다.
// 이유 1: D3D12가 비동기이기 때문에 Drawcall을 하고 CB값을 바꿔서 Drawcall을 할 수 없음
// 이유 2: CB만 여러개 가지고 있어도 부족하다. Drawcall을 할 때 DescriptorHeap도 Bind를 한다.
// 방법 : Constant Buffer View를 여러개를 만들고, Drawcall 마다 CBV를 Pool에 있는 heap 중 하나에 복사해서 bind를 하자.

class ConstantBufferPool
{
public:
	bool Initialize(Microsoft::WRL::ComPtr<ID3D12Device14> _pD3DDevice, UINT _sizePerCBV, UINT _maxCBVNums);
	CB_CONTAINER* Alloc(); // 해제하면 안되는 포인터를 반환한다.
	void Reset();

protected:
private:
	void CleanUpPool();
public:
protected:
private:
	// 배열로 관리한다.
	CB_CONTAINER* m_pCBContainerList;
	// CBV가 올라갈 Heap이다.
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_pCBVHeap;
	// CB resource
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pResource;
	// 업로드용 CPU memeory (typedef unsigned char UINT8)
	UINT8* m_pSystemMemAddr;
	// 초기화 할 때 pool을 어느정도로 만들지 정한다.
	UINT m_sizePerCBV;
	UINT m_maxCBVNum;
	UINT m_allocatedCBVNum;
public:
	ConstantBufferPool();
	~ConstantBufferPool();
};

