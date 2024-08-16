#include "pch.h"
#include "ScreenStreamer.h"
#include "D3D12Renderer.h"
#include "StreamingHeader.h"
#include <lz4.h>

DWORD WINAPI CreateFileFromTexture_Thread(LPVOID _pParam) 
{
	ImageFile* pImage = (ImageFile*)_pParam;

	LZ4_decompress_safe(pImage->compressedTexture, pImage->decompressedTexture, pImage->compressSize, pImage->image.slicePitch);
	pImage->image.pixels = (UINT8*)pImage->decompressedTexture;
	HRESULT hr = SaveToWICFile(
		pImage->image,
		WIC_FLAGS_NONE,
		GetWICCodec(WIC_CODEC_JPEG),
		pImage->fileName
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
	m_imageSendManager = new ImageSendManager;
	m_imageSendManager->InitializeWinsock();

	// test용
	m_imageFile = new ImageFile;
	m_compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
	m_decompressedTexture = new char[OriginalTextureSize];
}

void ScreenStreamer::CreatFileFromTexture(DWORD _dwTexIndex)
{
	//m_Image.pixels = m_pMappedData[_dwTexIndex];
	
	DWORD dwThreadID = 0;
	if (m_hThread == 0)
	{
		m_decompressedTexture = (char*)m_pMappedData[_dwTexIndex];

		m_imageFile->image = m_Image;
		m_imageFile->compressedTexture = m_compressedTexture;
		m_imageFile->decompressedTexture = m_decompressedTexture;

		int compressSize = LZ4_compress_fast((char*)m_pMappedData[_dwTexIndex], m_compressedTexture, m_Image.slicePitch, LZ4_compressBound(m_Image.slicePitch), 1);
		m_imageFile->compressSize = compressSize;
		m_testIndex++;
		swprintf_s(m_imageFile->fileName, L"testcapturescreen\\screen_copy_%04d.png", m_testIndex);

		if (m_testIndex >= 300)
		{
			return;
		}
		m_hThread = ::CreateThread(
			NULL,
			0,
			CreateFileFromTexture_Thread,
			(void*)(m_imageFile),
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

void ScreenStreamer::SendPixelsFromTexture()
{
	if (m_imageSendManager)
	{
		m_imageSendManager->SendData(m_pMappedData[m_indexToCopyDest_Circular], m_Image.slicePitch, m_indexToCopyDest_Circular);
	}
}

ScreenStreamer::ScreenStreamer()
	:m_pScreenTexture{}, m_pMappedData{}, 
	m_uiCurTextureIndex(0), m_TextureDesc{}, m_Image{},
	m_hThread(0), m_footPrint{}, m_imageSendManager(nullptr), m_indexToCopyDest_Circular(0), m_testIndex(0), 
	m_imageFile(nullptr), m_compressedTexture(nullptr), m_decompressedTexture(nullptr)
{
}

ScreenStreamer::~ScreenStreamer()
{
	if (m_hThread != 0)
	{
		WaitForSingleObject(m_hThread, INFINITE);
	}
	if (m_imageSendManager) 
	{
		delete m_imageSendManager;
	}
	if (m_compressedTexture)
	{
		delete[] m_compressedTexture;
	}
	if (m_imageFile)
	{
		delete m_imageFile;
	}
}

bool ScreenStreamer::CheckSendable()
{
	return m_imageSendManager->CanSendData(m_indexToCopyDest_Circular);
}
