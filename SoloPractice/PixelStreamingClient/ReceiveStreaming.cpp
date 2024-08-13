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
1. ImageReceiveManager에서
- 압축 안 된텍스쳐 버퍼를 가지고 있고,
- session 별로 받으면서 압축된 텍스쳐를 ImageDecompressor에게 copy해서 넘겨주면서 요청을 한다. (여기서 스레드는 분리된다.)
- buffer마다 lock을 가지고 있는데, 이 lock은 uploadbuffer로 copy할 때나, 압축을 해제할 때 사용한다.

2. ImageDecompressor에서
- 압축을 해제한 다음 ImageReceiveManager에 있는 버퍼에 넣는다.
- 이 과정에서 버퍼를 lock 한다.

3. renderer
ImageReceiveManager에서 버퍼를 upload buffer에 복사해서 올린다.
 이 과정에서 복사가 끝날 때 가지 lock을 한다.
*/

DWORD __stdcall ThreadManageReceiverAndSendToServer(LPVOID _pParam)
{
	ImageReceiveManager* imageReceiveMananger = (ImageReceiveManager*)_pParam;

	// 송신측 정보
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
						// 드롭하는 것.
						goto DROP_PACKET;
					}

					bool bResult = lastSessionID[circularSessionIndex] < uiSessionID;
					if (bResult) // 현재 버퍼에 새로운 session이 들어올 차례인 경우
					{
						// 버퍼에서 현재 세션에 대해 받는 것을 멈추고, 압축 해제 스레드로 옮긴다.
						if (imageReceiveMananger->compressedTextureBuffer[circularSessionIndex]->bRendered == false)
						{
							// 렌더링 된 적이 없다면, 더 이상 버퍼링을 해서는 안된다. 일단 멈춰야 한다.
							// 서버와 화면에 갭이 생기겠지만, 어쩔 수 없다. 동영상이 아니기 때문에 그냥 버리는 수 밖에 없다.
							uiDroppedSessionID = uiSessionID;
							uiDroppedSessionNumber++;
							goto DROP_PACKET;
						}
						else 
						{
							// 새로 받을 준비를 한다.
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

					// 혹시 rederer에서 drawing 중이면 기다린다.
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
				// recv가 오지 않았거나, 그 어떤 Read도 OS가 끝마치지 않은 경우를 말한다.
				CancelIoEx((HANDLE)imageReceiveMananger->hRecvSocket, &curOverlapped_Param->wsaOL);
			}
		}

DROP_PACKET:
		curOverlapped_Param->wsabuf.buf = curOverlapped_Param->pData;
		curOverlapped_Param->wsabuf.len = MAX_PACKET_SIZE;

		// Overlapped I/O 요청을 건다.
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
	// winsock 초기화
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock을 초기화 할 수 없습니다.");
	}

	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hRecvSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	// overlapped_IO_Data 미리 생성
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		memset(overlapped_IO_Data[i]->pData, 0, sizeof(MAX_PACKET_SIZE));
	}

	// 작업용 클래스와, 이미지 버퍼 미리 생성.
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
