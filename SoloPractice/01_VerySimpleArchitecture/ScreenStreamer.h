#pragma once
#include "DirectXTex.h"

class D3D12Renderer;
struct ImageSendManager;

struct ImageFile
{
	Image image;
	wchar_t fileName[64];
	char* compressedTexture;
	char* decompressedTexture;
	int compressSize;
};

class ScreenStreamer
{
public:
	void Initialize(D3D12Renderer* _pRenderer, D3D12_RESOURCE_DESC _pResDescRT);

	void CreatFileFromTexture(DWORD _dwTexIndex);
	void SendPixelsFromTexture();
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

	ImageSendManager* m_imageSendManager;
	UINT m_indexToCopyDest_Circular;

	// test
	UINT64 m_testIndex;
	ImageFile* m_imageFile;
	char* m_compressedTexture;
	char* m_decompressedTexture;
public:
	ScreenStreamer();
	virtual ~ScreenStreamer();

	// Mapped 된 데이터를 Sending Thread가 작업 중인지 확인하는 것
	bool CheckSendable();

	// 렌더링이 완료된 렌더타켓을 카피할 수 있도록, D3D12Renderer에서 호출을 잘 해야 한다.
	Microsoft::WRL::ComPtr<ID3D12Resource> GetTextureToPaste()
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> copyDestTexture = m_pScreenTexture[m_indexToCopyDest_Circular];
		m_indexToCopyDest_Circular = (m_indexToCopyDest_Circular + 1) % SWAP_CHAIN_FRAME_COUNT;
		return copyDestTexture;
	}

	const D3D12_PLACED_SUBRESOURCE_FOOTPRINT& GetFootPrint() {
		return m_footPrint;
	}
};

