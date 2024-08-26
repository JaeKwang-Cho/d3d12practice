#include "pch.h"
#include "ReceiveStreaming.h"
#include <format>
#include <WS2tcpip.h>
#include "D3D12Renderer_Client.h"
#include <iostream>
#include <chrono>

#include <lz4.h>
#include <unordered_set>

SRWLOCK g_receiverInfoPerFrameLock = SRWLOCK_INIT;
DWORD g_overbufferSessionNum = 0;

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

static ULONGLONG g_PrevUpdateTime = 0;

DWORD __stdcall ThreadManageReceiverAndSendToServer(LPVOID _pParam)
{
	ImageReceiveManager* imageReceiveMananger = (ImageReceiveManager*)_pParam;

	// 송신측 정보
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);

	UINT64 uiSessionID = 0;
	UINT64 uiDroppedSessionNumber = 0;
	UINT circularSessionIndex = 0;
	uint32_t numTryToGetPackets = 0;

	UINT64 lastSessionID[IMAGE_NUM_FOR_BUFFERING];
	uint32_t totalPacketNums[IMAGE_NUM_FOR_BUFFERING];
	DWORD ulTotalByteSizes[IMAGE_NUM_FOR_BUFFERING];
	uint32_t sumPacketNums[IMAGE_NUM_FOR_BUFFERING];
	DWORD ulSumByteSizes[IMAGE_NUM_FOR_BUFFERING];
	memset(lastSessionID, 0xff, sizeof(lastSessionID));
	memset(totalPacketNums, 0, sizeof(totalPacketNums));
	memset(ulTotalByteSizes, 0, sizeof(ulTotalByteSizes));
	memset(sumPacketNums, 0, sizeof(sumPacketNums));
	memset(ulSumByteSizes, 0, sizeof(ulSumByteSizes));

	/*
	그냥 냅다 버퍼링을 시도하는 버퍼와
	무결성을 체크해서 완성된 버퍼를 구분해서 시도해보자.	
	*/

	while (::WaitForSingleObject(imageReceiveMananger->GetHaltEvent(), 0) != WAIT_OBJECT_0)
	{
		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = imageReceiveMananger->overlapped_IO_Data[currOverlappedIOData_Index];

		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			//DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 1000);
			// 사용법을 잘 몰랐다. GetOverlappedResultEx()을 사용해서 다시 시도해보자.
			DWORD receivedBytes = 0;
			BOOL result = GetOverlappedResultEx(
				(HANDLE)imageReceiveMananger->hRecvSocket,
				&curOverlapped_Param->wsaOL,
				&receivedBytes,
				1000,
				TRUE);
			if (result != 0)
			{
				ScreenImageHeader header;
				memcpy(&header, curOverlapped_Param->pData, HEADER_SIZE);
				if (MAX_PACKET_SIZE >= receivedBytes && receivedBytes > 0)
				{
					uiSessionID = header.uiSessionID;
					circularSessionIndex = uiSessionID % IMAGE_NUM_FOR_BUFFERING;
					bool bResult = lastSessionID[circularSessionIndex] < uiSessionID;
					if (bResult) // 현재 버퍼에 새로운 session이 들어올 차례인 경우
					{
						lastSessionID[circularSessionIndex] = uiSessionID;
						totalPacketNums[circularSessionIndex] = header.totalPacketsNumber;
						ulTotalByteSizes[circularSessionIndex] = header.totalCompressedSize;
						sumPacketNums[circularSessionIndex] = 0;
						ulSumByteSizes[circularSessionIndex] = 0;
					}

					lastSessionID[circularSessionIndex] = uiSessionID;
					int dataOffset = DATA_SIZE * header.currPacketNumber;
					char* pDataToWrite = (char*)curOverlapped_Param->wsabuf.buf;

					ulSumByteSizes[circularSessionIndex] += receivedBytes - HEADER_SIZE;
					sumPacketNums[circularSessionIndex]++;

					// 혹시 rederer에서 drawing 중이면 기다린다.
					AcquireSRWLockExclusive(&imageReceiveMananger->rawTextureBuffer[circularSessionIndex]->lock);
					memcpy(imageReceiveMananger->rawTextureBuffer[circularSessionIndex]->pCompressedData + dataOffset, pDataToWrite + HEADER_SIZE, receivedBytes - HEADER_SIZE);
					if (ulTotalByteSizes[circularSessionIndex] == ulSumByteSizes[circularSessionIndex])
					{
						UINT64 indexToUpdate = imageReceiveMananger->IncreaseImageNums();
						AcquireSRWLockExclusive(&imageReceiveMananger->validTextureBuffer[indexToUpdate]->lock);
						imageReceiveMananger->validTextureBuffer[indexToUpdate]->compressedSize = header.totalCompressedSize;
						memcpy(
							imageReceiveMananger->validTextureBuffer[indexToUpdate]->pCompressedData,
							imageReceiveMananger->rawTextureBuffer[circularSessionIndex]->pCompressedData,
							header.totalCompressedSize);
						ReleaseSRWLockExclusive(&imageReceiveMananger->validTextureBuffer[indexToUpdate]->lock);
					}
					ReleaseSRWLockExclusive(&imageReceiveMananger->rawTextureBuffer[circularSessionIndex]->lock);
				}
			}
			else if (result == 0 && GetLastError() == WAIT_TIMEOUT)
			{
				// recv가 오지 않았거나, 그 어떤 Read도 OS가 끝마치지 않은 경우를 말한다.
				CancelIoEx((HANDLE)imageReceiveMananger->hRecvSocket, &curOverlapped_Param->wsaOL);
				OutputDebugStringW(L"No Socket Input from Server\n");
			}
			else 
			{
				int lastError = GetLastError();
			}
		}
		ZeroMemory(curOverlapped_Param->pData, MAX_PACKET_SIZE);
		ZeroMemory(&curOverlapped_Param->wsaOL, sizeof(OVERLAPPED));

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
		int result = WSARecv(imageReceiveMananger->hRecvSocket, &curOverlapped_Param->wsabuf, 1, nullptr, &flags, &curOverlapped_Param->wsaOL, nullptr);
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
		rawTextureBuffer[i] = new CompressedTextures;

		rawTextureBuffer[i]->lock = SRWLOCK_INIT;
		rawTextureBuffer[i]->pCompressedData = new char[LZ4_compressBound(s_OriginalTextureSize)];
		rawTextureBuffer[i]->compressedSize = 0;

		validTextureBuffer[i] = new CompressedTextures;

		validTextureBuffer[i]->lock = SRWLOCK_INIT;
		validTextureBuffer[i]->pCompressedData = new char[LZ4_compressBound(s_OriginalTextureSize)];
		validTextureBuffer[i]->compressedSize = 0;
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

UINT64 ImageReceiveManager::IncreaseImageNums()
{
	if (updatedImagesCount - renderedImageCount < IMAGE_NUM_FOR_BUFFERING)
	{
		AcquireSRWLockExclusive(&countLock);
		updatedImagesCount++;
		lastUpdatedCircularIndex = (lastUpdatedCircularIndex + 1) % IMAGE_NUM_FOR_BUFFERING;
		ReleaseSRWLockExclusive(&countLock);
		return (lastUpdatedCircularIndex - 1 + IMAGE_NUM_FOR_BUFFERING) % IMAGE_NUM_FOR_BUFFERING;
	}
	else 
	{
		AcquireSRWLockExclusive(&g_receiverInfoPerFrameLock);
		g_overbufferSessionNum++;
		ReleaseSRWLockExclusive(&g_receiverInfoPerFrameLock);
		return lastUpdatedCircularIndex;
	}
}

bool ImageReceiveManager::DecompressNCopyNextBuffer(void* _pUploadData)
{
	if (updatedImagesCount < 5 || updatedImagesCount <= renderedImageCount)
	{
		return false;
	}
	AcquireSRWLockExclusive(&validTextureBuffer[lastRenderedCircularIndex]->lock);

	LZ4_decompress_safe(validTextureBuffer[lastRenderedCircularIndex]->pCompressedData, (char*)_pUploadData,
		validTextureBuffer[lastRenderedCircularIndex]->compressedSize, s_OriginalTextureSize);

	ReleaseSRWLockExclusive(&validTextureBuffer[lastRenderedCircularIndex]->lock);

	lastRenderedCircularIndex = (lastRenderedCircularIndex + 1) % IMAGE_NUM_FOR_BUFFERING;
	renderedImageCount++;
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
		if (rawTextureBuffer[i])
		{
			if (rawTextureBuffer[i]->pCompressedData)
			{
				delete[] rawTextureBuffer[i]->pCompressedData;
				rawTextureBuffer[i]->pCompressedData = nullptr;
			}
			delete rawTextureBuffer[i];
			rawTextureBuffer[i] = nullptr;
		}

		if (validTextureBuffer[i])
		{
			if (validTextureBuffer[i]->pCompressedData)
			{
				delete[] validTextureBuffer[i]->pCompressedData;
				validTextureBuffer[i]->pCompressedData = nullptr;
			}
			delete validTextureBuffer[i];
			validTextureBuffer[i] = nullptr;
		}
	}

	CloseHandle(hThread);
	::closesocket(hRecvSocket);
	::WSACleanup();
}

ImageReceiveManager::ImageReceiveManager()
	:wsa{ 0 }, addr{ 0 }, hRecvSocket(0),  hThread(0), hHaltEvent(0),
	renderedImageCount(0), lastRenderedCircularIndex(0), 
	lastUpdatedCircularIndex(0), updatedImagesCount(0),
	overlapped_IO_Data{}, rawTextureBuffer {}, validTextureBuffer{},
	renderer(nullptr)
{
	countLock = SRWLOCK_INIT;
}

ImageReceiveManager::~ImageReceiveManager()
{
	CleanUpManager();
}
