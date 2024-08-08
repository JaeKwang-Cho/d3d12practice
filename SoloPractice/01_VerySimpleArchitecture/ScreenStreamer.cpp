#include "pch.h"
#include "ScreenStreamer.h"
#include "D3D12Renderer.h"
#include "StreamingHeader.h"

DWORD WINAPI CreateFileFromTexture_Thread(LPVOID _pParam) 
{
	Image* pImage = (Image*)_pParam;
	HRESULT hr = SaveToWICFile(
		*pImage,
		WIC_FLAGS_NONE,
		GetWICCodec(WIC_CODEC_JPEG),
		L"Screen_Copy.png"
	);

	if (FAILED(hr)) {
		__debugbreak();
		OutputDebugString(L"SaveToWICFile Failed\n");
		return 1;
	}
	return 0;
}


void ScreenStreamer::Initialize(D3D12Renderer* _pRenderer, D3D12_RESOURCE_DESC _pResDescRT)
{
	Microsoft::WRL::ComPtr<ID3D12Device> pDevice = _pRenderer->INL_GetD3DDevice();
	m_TextureDesc = _pResDescRT;
	m_TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	// https://megayuchi.com/2016/01/03/d3d12%EC%97%90%EC%84%9C-texture%EC%9D%98-row-pitch%EA%B5%AC%ED%95%98%EA%B8%B0/
	UINT rows = 0;
	UINT64 rowSize = 0;
	UINT64 totalBytes = 0;

	pDevice->GetCopyableFootprints(&m_TextureDesc, 0, 1, 0, &m_footPrint, &rows, &rowSize, &totalBytes);

	m_Image.width = m_TextureDesc.Width;
	m_Image.height = m_TextureDesc.Height;
	m_Image.format = m_TextureDesc.Format;
	m_Image.rowPitch = m_footPrint.Footprint.RowPitch;
	m_Image.slicePitch = totalBytes;

	D3D12_RESOURCE_DESC buffSizeDesc = CD3DX12_RESOURCE_DESC::Buffer(totalBytes);

	HRESULT hr = E_NOTIMPL;
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++) {
		// 렌더 타겟을 가져올 친구
		hr = pDevice->CreateCommittedResource(
			&HEAP_PROPS_READBACK,
			D3D12_HEAP_FLAG_NONE,
			&buffSizeDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(m_pScreenTexture[i].GetAddressOf())
		);

		if (FAILED(hr)) {
			__debugbreak();
		}
		m_pScreenTexture[i]->Map(0, nullptr, (void**)(m_pMappedData + i));
	}

	// winsock 작업을 할 구조체를 초기화.
	m_winsockProps = new ImageSendManager;
	m_winsockProps->InitializeWinsock();
}

void ScreenStreamer::CreatFileFromTexture(DWORD _dwTexIndex)
{
	m_Image.pixels = m_pMappedData[_dwTexIndex];
	DWORD dwThreadID = 0;
	if (m_hThread == 0)
	{
		m_hThread = ::CreateThread(
			NULL,
			0,
			CreateFileFromTexture_Thread,
			(void*)(&m_Image),
			0,
			&dwThreadID
		);
	}
	else
	{
		DWORD result = WaitForSingleObject(m_hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(m_hThread);
			m_hThread = 0;
		}
	}
}

void ScreenStreamer::SendPixelsFromTexture(DWORD _dwTexIndex)
{
	if (m_winsockProps)
	{
		m_winsockProps->SendData(m_pMappedData[_dwTexIndex], m_Image.slicePitch);
	}
}

ScreenStreamer::ScreenStreamer()
	:m_pScreenTexture{}, m_pMappedData{}, 
	m_uiCurTextureIndex(0), m_TextureDesc{}, m_Image{},
	m_hThread(0), m_footPrint{}, m_winsockProps(nullptr)
{
}

ScreenStreamer::~ScreenStreamer()
{
	if (m_winsockProps) 
	{
		delete m_winsockProps;
	}
}

bool ScreenStreamer::CheckSendingThread(UINT _uiRenderTargetIndex)
{
	return m_winsockProps->CanSendData();
}
