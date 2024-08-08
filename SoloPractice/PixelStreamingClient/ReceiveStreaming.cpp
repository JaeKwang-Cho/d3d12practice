#include "pch.h"
#include "ReceiveStreaming.h"
#include <format>
#include <WS2tcpip.h>

#include <lz4.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

DWORD __stdcall ThreadReceiveFromServer(LPVOID _pParam)
{
	/*
	1. �ϴ� �ݺ��ؼ� receive�� �ϴ� thread
	=> ���������� �� ��Ʈ��ȣ�� ��û�� ������.
	=> ��� Ŭ���̾�Ʈ���� ������ �����ִ� ���� �̹����� ���� �� �ִ��� Ȯ����.
	- Ŭ���̾�Ʈ�� ���� �̹����� ���� �� �ִ��Ŀ� ���� ������
	- ������ ������ Ǯ����, ��� �����尡 �ִ� ���.
	>> ����
	- Server���� �˸°� �̹��� �����͸� ���� �� �ֵ���
	 ������ + ���� + ��Ʈ��ȣ ����� ��Ƽ� ������.
	- �׸��� �̹����� �����ϴ� �����带 �����Ű��, �ش� �����带 �� ������.
	>> �Ұ���
	- Server���� �ȵȴٰ� ����
	- �׸��� �۾����� ������ �� �ι�°�� ������ �����带, ������Ŵ

	=> ������ 
	- �� ������ 17 ��ٸ� ������ ���� ��û�� ����.
	*/
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

#if OVERLAPPED_IO_VERSION
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
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}

	hSendSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hSendSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// Overlapped ����ü �̸� �Ҵ��س���
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}
	overlapped_IO_State = new Overlapped_IO_State;
	// ���� �ؽ��� ���� �̸� ����
	compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
#else
	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hRecvSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}
#endif
}

void ImageReceiveManager::StartReceiveManager()
{
	// 
}

void ImageReceiveManager::GetImageData(void* _pData)
{
	/*
	���� �����ϴ�, ���� ������ �̹�����
	lock�� �ɷ����� ������ (������Ʈ ���� �ƴϸ�)
	������Ʈ�� �Ѵ�. �׳� �Ʒ��� �ִ� �Լ��� ��ĥ ���� �ִ�.
	*/
}

bool ImageReceiveManager::CanReceiveData()
{
	/*
	���� �����ϴ�, ���� ������ �̹����� 
	lock�� �ɷ����� ������ (������Ʈ ���� �ƴϸ�)
	true�� ��ȯ�Ѵ�.
	*/
}

void ImageReceiveManager::CleanUpManager()
{
	/*
	�ɹ� �̹��� �����带 �����Ű��
	���� �����带 �����ϰ�,
	*/
}

ImageReceiveManager::ImageReceiveManager()
	:wsa{ 0 }, addr{ 0 }, hRecvSocket(0), hSendSocket(0), hThread(0), threadParam{ {0}, {0}, {0} }
#if OVERLAPPED_IO_VERSION
	, imageReceivers{}, overlapped_IO_State(nullptr), lastUpdatedCircularIndex(0)
#endif
#if LZ4_COMPRESSION
	, decompressedTextures_circular{}
#endif
{
}

ImageReceiveManager::~ImageReceiveManager()
{
#if OVERLAPPED_IO_VERSION
	if (overlapped_IO_State)
	{
		delete overlapped_IO_State;
		overlapped_IO_State = nullptr;
	}
#endif
#if LZ4_COMPRESSION
	for (UINT i = 0; i < THREAD_NUM_FOR_BUFFERING; i++)
	{
		if (decompressedTextures_circular[i])
		{
			delete decompressedTextures_circular[i];
			decompressedTextures_circular[i] = nullptr;
		}
	}
#endif
	CloseHandle(hThread);
	::closesocket(hRecvSocket);
	::closesocket(hSendSocket);
	::WSACleanup();
}

// ================= ImageReceiver ================= 

DWORD __stdcall ThreadReceiveImageFromServer(LPVOID _pParam)
{
	ThreadParam_Receiver* pThreadParam = (ThreadParam_Receiver*)_pParam;
	char* pTexturePointer = pThreadParam->pCompressed;
	SOCKADDR_IN addr = pThreadParam->addr;
	uint32_t totalPacketNumber = pThreadParam->totalPacketNumber;
	HANDLE hHaltEvent = pThreadParam->hHaltEvent;

	uint32_t numTryToGetPackets = 0;
	size_t totalByteSize = 0;

	// �۽��� ����
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200����Ʈ�� �Ѿ���� ��Ŷ�� �޾Ƽ� �״´�.
	while (true)
	{
		if (::WaitForSingleObject(hHaltEvent, 0) == WAIT_OBJECT_0)
		{
			return 1;
		}

		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (�����ϰ� �߶��ų�) ������ �ʹ� ū ��츦 ����Ѵ�.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 1);
			if (result != WAIT_TIMEOUT)
			{
				ScreenImageHeader header;
				memcpy(&header, curOverlapped_Param->pData, HEADER_SIZE);

				if (MAX_PACKET_SIZE >= curOverlapped_Param->ulByteSize)
				{
					int dataOffset = DATA_SIZE * header.currPacketNumber;
					char* pDataToWrite = (char*)curOverlapped_Param->wsabuf.buf;
					totalByteSize += curOverlapped_Param->ulByteSize;
					memcpy(pTexturePointer + dataOffset, pDataToWrite + HEADER_SIZE, curOverlapped_Param->ulByteSize);

					if (header.currPacketNumber >= totalPacketNumber - 1)
					{
						break;
					}
				}
			}
		}

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
		int result = WSARecv(pThreadParam->hRecvSocket, &curOverlapped_Param->wsabuf, 1, &curOverlapped_Param->ulByteSize, &flags, &curOverlapped_Param->wsaOL, nullptr);
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			break;
		}
		numTryToGetPackets++;
		if (numTryToGetPackets >= totalPacketNumber - 1)
		{
			break;
		}
	}

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (::WaitForSingleObject(hHaltEvent, 0) == WAIT_OBJECT_0)
		{
			return 1;
		}

		if (pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent, 10);

			ScreenImageHeader header;
			memcpy(&header, pThreadParam->overlapped_IO_Data[i]->pData, HEADER_SIZE);
			if (MAX_PACKET_SIZE >= pThreadParam->overlapped_IO_Data[i]->ulByteSize)
			{
				int dataOffset = DATA_SIZE * header.currPacketNumber;
				char* pDataToWrite = (char*)pThreadParam->overlapped_IO_Data[i]->pData;
				totalByteSize += pThreadParam->overlapped_IO_Data[i]->ulByteSize;
				memcpy(pTexturePointer + dataOffset, pDataToWrite + HEADER_SIZE, pThreadParam->overlapped_IO_Data[i]->ulByteSize - HEADER_SIZE);
			}
			pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		}
	}
	int decompressSize = LZ4_decompress_safe(pTexturePointer, (char*)pThreadParam->pData, totalByteSize, OriginalTextureSize);
	return 0;
}

void ImageReceiver::Initialize(const UINT _portNum)
{
	portNum = _portNum;

	hRecvSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}

	// Overlapped ����ü �̸� �Ҵ��س���
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}

	// ���� �ؽ��� ���� �̸� ����
	compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
}

bool ImageReceiver::CheckReceiverState()
{
	if (hThread == 0)
	{
		return true;
	}
	DWORD result = WaitForSingleObject(hThread, 0);
	if (result == WAIT_OBJECT_0)
	{
		CloseHandle(hThread);
		hThread = 0;
		return true;
	}
	else if (result == WAIT_TIMEOUT)
	{
		return false;
	}
	__debugbreak();
	return false;
}

HANDLE ImageReceiver::ReceiveImage(void* _pOutImage)
{
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
			return;
		}
	}
	threadParam.pData = _pOutImage;
	threadParam.pCompressed = compressedTexture;
	threadParam.hRecvSocket = hRecvSocket;
	threadParam.addr = addr;
	if (hHaltEvent != 0)
	{
		::CloseHandle(hHaltEvent);
		hHaltEvent = 0;
	}
	hHaltEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	threadParam.hHaltEvent = hHaltEvent;
	memcpy(threadParam.overlapped_IO_Data, overlapped_IO_Data, sizeof(overlapped_IO_Data));
	::InetPton(AF_INET, _T("127.0.0.1"), &threadParam.addr.sin_addr);

	hThread = ::CreateThread(
		NULL,
		0,
		ThreadReceiveImageFromServer,
		(LPVOID)(&threadParam),
		0,
		&dwThreadID
	);

	return hThread;
}

void ImageReceiver::HaltThread()
{
	::SetEvent(hHaltEvent);
	::WaitForSingleObject(hThread, INFINITE);

	::CloseHandle(hThread);
	::CloseHandle(hHaltEvent);
}

ImageReceiver::ImageReceiver()
	:addr{}, hRecvSocket(0), portNum(0), hThread(0), hHaltEvent(0), overlapped_IO_Data{}, threadParam{}, compressedTexture(nullptr)
{
}

ImageReceiver::~ImageReceiver()
{
	HaltThread();
	::closesocket(hRecvSocket);
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}
	if (compressedTexture)
	{
		delete compressedTexture;
		compressedTexture = nullptr;
	}
}

// =================================================== 