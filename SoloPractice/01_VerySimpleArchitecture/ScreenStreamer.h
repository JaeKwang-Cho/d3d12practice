#pragma once
#include "DirectXTex.h"

class D3D12Renderer;
struct WinSock_Properties;

class ScreenStreamer
{
public:
	void Initialize(D3D12Renderer* _pRenderer, D3D12_RESOURCE_DESC _pResDescRT);

	void CreatFileFromTexture(DWORD _dwTexIndex);
	void SendPixelsFromTexture(DWORD _dwTexIndex);
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

	// Mapped �� �����͸� Sending Thread�� �۾� ������ Ȯ���ϴ� ��
	bool CheckSendingThread(UINT _uiRenderTargetIndex);

	// �������� �Ϸ�� ����Ÿ���� ī���� �� �ֵ���, D3D12Renderer���� ȣ���� �� �ؾ� �Ѵ�.
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureToPaste(UINT _uiRenderTargetIndex)
	{
		return m_pScreenTexture[_uiRenderTargetIndex];
	}

	const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& GetFootPrint() {
		return m_footPrint;
	}
};

