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

	// �۽��� ����
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
	�׳� ���� ���۸��� �õ��ϴ� ���ۿ�
	���Ἲ�� üũ�ؼ� �ϼ��� ���۸� �����ؼ� �õ��غ���.	
	*/

	while (::WaitForSingleObject(imageReceiveMananger->GetHaltEvent(), 0) != WAIT_OBJECT_0)
	{
		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = imageReceiveMananger->overlapped_IO_Data[currOverlappedIOData_Index];

		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			//DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 1000);
			// ������ �� ������. GetOverlappedResultEx()�� ����ؼ� �ٽ� �õ��غ���.
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
					if (bResult) // ���� ���ۿ� ���ο� session�� ���� ������ ���
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

					// Ȥ�� rederer���� drawing ���̸� ��ٸ���.
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
				// recv�� ���� �ʾҰų�, �� � Read�� OS�� ����ġ�� ���� ��츦 ���Ѵ�.
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
