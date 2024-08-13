#include "pch.h"
#include "ReceiveStreaming.h"
#include <format>
#include <WS2tcpip.h>
#include "D3D12Renderer_Client.h"
#include <iostream>
#include <chrono>

#include <lz4.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

static ULONGLONG g_PrevUpdateTime = 0;

/*
1. ImageReceiveManager����
- ���� �� ���ؽ��� ���۸� ������ �ְ�,
- session ���� �����鼭 ����� �ؽ��ĸ� ImageDecompressor���� copy�ؼ� �Ѱ��ָ鼭 ��û�� �Ѵ�. (���⼭ ������� �и��ȴ�.)
- buffer���� lock�� ������ �ִµ�, �� lock�� uploadbuffer�� copy�� ����, ������ ������ �� ����Ѵ�.

2. ImageDecompressor����
- ������ ������ ���� ImageReceiveManager�� �ִ� ���ۿ� �ִ´�.
- �� �������� ���۸� lock �Ѵ�.

3. renderer
ImageReceiveManager���� ���۸� upload buffer�� �����ؼ� �ø���.
 �� �������� ���簡 ���� �� ���� lock�� �Ѵ�.
*/

DWORD __stdcall ThreadManageReceiverAndSendToServer(LPVOID _pParam)
{
	ImageReceiveManager* imageReceiveMananger = (ImageReceiveManager*)_pParam;

	// �۽��� ����
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);

	UINT64 uiSessionID = 0;
	UINT64 uiDroppedSessionID = 0;
	UINT64 uiDroppedSessionNumber = 0;
	UINT circularSessionIndex = 0;
	uint32_t numTryToGetPackets = 0;

	UINT64 lastSessionID[IMAGE_NUM_FOR_BUFFERING];
	uint32_t totalPacketNums[IMAGE_NUM_FOR_BUFFERING];
	uint32_t sumPacketNums[IMAGE_NUM_FOR_BUFFERING];
	DWORD ulByteSizes[IMAGE_NUM_FOR_BUFFERING];
	memset(lastSessionID, 0xff, sizeof(lastSessionID));
	memset(totalPacketNums, 0, sizeof(totalPacketNums));
	memset(sumPacketNums, 0, sizeof(sumPacketNums));
	memset(ulByteSizes, 0, sizeof(ulByteSizes));

	ULONGLONG CurrTickTime = GetTickCount64();

	if (CurrTickTime - g_PrevUpdateTime > 66) // 15FPS
	{
		g_PrevUpdateTime = CurrTickTime;
	}

	while (::WaitForSingleObject(imageReceiveMananger->GetHaltEvent(), 0) != WAIT_OBJECT_0)
	{
		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = imageReceiveMananger->overlapped_IO_Data[currOverlappedIOData_Index];

		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 0);
			if (result != WAIT_TIMEOUT)
			{
				ScreenImageHeader header;
				memcpy(&header, curOverlapped_Param->pData, HEADER_SIZE);

				if (MAX_PACKET_SIZE >= curOverlapped_Param->ulByteSize)
				{
					uiSessionID = header.uiSessionID;
					circularSessionIndex = (uiSessionID - uiDroppedSessionNumber + IMAGE_NUM_FOR_BUFFERING) % IMAGE_NUM_FOR_BUFFERING;
					if (uiSessionID <= uiDroppedSessionID) 
					{
						// ����ϴ� ��.
						goto DROP_PACKET;
					}

					bool bResult = lastSessionID[circularSessionIndex] < uiSessionID;
					if (bResult) // ���� ���ۿ� ���ο� session�� ���� ������ ���
					{
						// ���ۿ��� ���� ���ǿ� ���� �޴� ���� ���߰�, ���� ���� ������� �ű��.
						if (imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->bRendered == false)
						{
							// ������ �� ���� ���ٸ�, �� �̻� ���۸��� �ؼ��� �ȵȴ�. �ϴ� ����� �Ѵ�.
							// ������ ȭ�鿡 ���� ���������, ��¿ �� ����. �������� �ƴϱ� ������ �׳� ������ �� �ۿ� ����.
							uiDroppedSessionID = uiSessionID;
							uiDroppedSessionNumber++;
							goto DROP_PACKET;
						}
						else 
						{
							// ���� ���� �غ� �Ѵ�.
							lastSessionID[circularSessionIndex] = uiSessionID;
							totalPacketNums[circularSessionIndex] = header.totalPacketsNumber;
							sumPacketNums[circularSessionIndex] = 0;
							ulByteSizes[circularSessionIndex] = 0;
						}
					}

					lastSessionID[circularSessionIndex] = uiSessionID;
					int dataOffset = DATA_SIZE * header.currPacketNumber;
					char* pDataToWrite = (char*)curOverlapped_Param->wsabuf.buf;

					ulByteSizes[circularSessionIndex] += curOverlapped_Param->ulByteSize - HEADER_SIZE;
					sumPacketNums[circularSessionIndex]++;

					// Ȥ�� rederer���� drawing ���̸� ��ٸ���.
					AcquireSRWLockExclusive(&imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->lock);
					memcpy(imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->pCompressedData + dataOffset, pDataToWrite + HEADER_SIZE, curOverlapped_Param->ulByteSize - HEADER_SIZE);
					ReleaseSRWLockExclusive(&imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->lock);
					if (sumPacketNums[circularSessionIndex] >= header.totalPacketsNumber || header.currPacketNumber >= header.totalPacketsNumber - 1)
					{
						imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->compressedSize = ulByteSizes[circularSessionIndex];
						imageReceiveMananger->IncreaseImageNums();
					}
				}
			}
			else
			{
				// recv�� ���� �ʾҰų�, �� � Read�� OS�� ����ġ�� ���� ��츦 ���Ѵ�.
				CancelIoEx((HANDLE)imageReceiveMananger->hRecvSocket, &curOverlapped_Param->wsaOL);
			}
		}

DROP_PACKET:
		curOverlapped_Param->wsabuf.buf = curOverlapped_Param->pData;
		curOverlapped_Param->wsabuf.len = MAX_PACKET_SIZE;

		// Overlapped I/O ��û�� �Ǵ�.
		if (curOverlapped_Param->wsaOL.hEvent == 0)
		{
			curOverlapped_Param->wsaOL.hEvent = ::WSACreateEvent();
		}
		else
		{
			WSAResetEvent(curOverlapped_Param->wsaOL.hEvent);
		}
		DWORD flags = 0;
		int result = WSARecv(imageReceiveMananger->hRecvSocket, &curOverlapped_Param->wsabuf, 1, &curOverlapped_Param->ulByteSize, &flags, &curOverlapped_Param->wsaOL, nullptr);
		int lastError = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			__debugbreak();
			break;
		}
		numTryToGetPackets++;
	}

	return 0;
}

void ImageReceiveManager::InitializeWinSock()
{
	// winsock �ʱ�ȭ
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock�� �ʱ�ȭ �� �� �����ϴ�.");
	}

	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hRecvSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}

	// overlapped_IO_Data �̸� ����
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		memset(overlapped_IO_Data[i]->pData, 0, sizeof(MAX_PACKET_SIZE));
	}

	// �۾��� Ŭ������, �̹��� ���� �̸� ����.
	for (UINT i = 0; i < IMAGE_NUM_FOR_BUFFERING; i++)
	{
		compressedTextureBuffer[i] = new CompressedTextures;

		compressedTextureBuffer[i]->lock = SRWLOCK_INIT;
		compressedTextureBuffer[i]->pCompressedData = new char[LZ4_compressBound(s_OriginalTextureSize)];
		compressedTextureBuffer[i]->bRendered = false;
		compressedTextureBuffer[i]->compressedSize = 0;
	}
}

bool ImageReceiveManager::StartReceiveManager(D3D12Renderer_Client* _renderer)
{
	renderer = _renderer;
	DWORD dwThreadID = 0;

	if (hThread != 0)
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
		else
		{
			return false;
		}
	}
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);
	hHaltEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	hThread = ::CreateThread(
		NULL,
		0,
		ThreadManageReceiverAndSendToServer,
		(LPVOID)(this),
		0,
		&dwThreadID
	);

	return hThread != NULL;
} 

void ImageReceiveManager::IncreaseImageNums()
{
	AcquireSRWLockExclusive(&countLock);
	numImages++;
	lastUpdatedCircularIndex = (lastUpdatedCircularIndex + 1) % IMAGE_NUM_FOR_BUFFERING;
	ReleaseSRWLockExclusive(&countLock);
}

bool ImageReceiveManager::DecompressNCopyNextBuffer(void* _pUploadData)
{
	if (numImages < 5)
	{
		return false;
	}
	AcquireSRWLockExclusive(&compressedTextureBuffer[lastRenderedCircularIndex]->lock);

	LZ4_decompress_safe(compressedTextureBuffer[lastRenderedCircularIndex]->pCompressedData, (char*)_pUploadData,
		compressedTextureBuffer[lastRenderedCircularIndex]->compressedSize, s_OriginalTextureSize);
	compressedTextureBuffer[lastRenderedCircularIndex]->bRendered = true;

	ReleaseSRWLockExclusive(&compressedTextureBuffer[lastRenderedCircularIndex]->lock);
	lastRenderedCircularIndex = (lastRenderedCircularIndex + 1) % IMAGE_NUM_FOR_BUFFERING;
	return true;
}

void ImageReceiveManager::CleanUpManager()
{
	::SetEvent(hHaltEvent);
	::WaitForSingleObject(hThread, INFINITE);
	::CloseHandle(hHaltEvent);

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}

	for (UINT i = 0; i < IMAGE_NUM_FOR_BUFFERING; i++)
	{
		if (compressedTextureBuffer[i])
		{
			if (compressedTextureBuffer[i]->pCompressedData)
			{
				delete[] compressedTextureBuffer[i]->pCompressedData;
				compressedTextureBuffer[i]->pCompressedData = nullptr;
			}
			delete compressedTextureBuffer[i];
			compressedTextureBuffer[i] = nullptr;
		}
	}

	CloseHandle(hThread);
	::closesocket(hRecvSocket);
	::WSACleanup();
}

ImageReceiveManager::ImageReceiveManager()
	:wsa{ 0 }, addr{ 0 }, hRecvSocket(0),  hThread(0), hHaltEvent(0),
	numImages(0), lastRenderedCircularIndex(0), lastUpdatedCircularIndex(0), 
	overlapped_IO_Data{}, compressedTextureBuffer {},
	renderer(nullptr)
{
	countLock = SRWLOCK_INIT;
}

ImageReceiveManager::~ImageReceiveManager()
{
	CleanUpManager();
}
