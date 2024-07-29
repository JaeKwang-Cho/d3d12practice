#pragma once
#include "DirectXTex.h"

class D3D12Renderer;
struct WinSock_Properties;

class ScreenStreamer
{
public:
	void Initialize(D3D12Renderer* _pRenderer, D3D12_RESOURCE_DESC _pResDescRT);

	void CreatFileFromTexture(DWORD _dwTexIndex);
	void SendFileFromTexture(DWORD _dwTexIndex);
protected:
private:

public:
protected:
private:
	Microsoft::WRL::ComPtr<ID3D12Resource> m_pScreenTexture[SWAP_CHAIN_FRAME_COUNT];
	UINT8* m_pMappedData[SWAP_CHAIN_FRAME_COUNT];
	UINT m_uiCurTextureIndex;

	D3D12_RESOURCE_DESC m_TextureDesc;
	Image m_Image;
	HANDLE m_hThread;
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT m_footPrint;

	WinSock_Properties* m_winsockProps;

public:
	ScreenStreamer();
	virtual ~ScreenStreamer();

	// 렌더링이 완료된 렌더타켓을 카피할 수 있도록, D3D12Renderer에서 호출을 잘 해야 한다.
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureToPaste()
	{
		UINT curIndex = m_uiCurTextureIndex;
		m_uiCurTextureIndex = (m_uiCurTextureIndex + 1) % SWAP_CHAIN_FRAME_COUNT;
		return m_pScreenTexture[curIndex];
	}

	const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& GetFootPrint() {
		return m_footPrint;
	}
};

