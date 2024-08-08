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
	1. 일단 반복해서 receive를 하는 thread
	=> 서버에서는 얘 포트번호로 요청을 보낸다.
	=> 얘는 클라이언트에서 서버가 보내주는 압축 이미지를 받을 수 있는지 확인함.
	- 클라이언트가 압축 이미지를 받을 수 있느냐에 대한 조건은
	- 생성한 스레드 풀에서, 노는 스레드가 있는 경우.
	>> 가능
	- Server에게 알맞게 이미지 데이터를 보낼 수 있도록
	 스레드 + 버퍼 + 포트번호 등등을 담아서 보내줌.
	- 그리고 이미지를 수신하는 스레드를 실행시키고, 해당 스레드를 또 관리함.
	>> 불가능
	- Server에게 안된다고 보냄
	- 그리고 작업중인 스레드 중 두번째로 오래된 스레드를, 중지시킴

	=> 마무리 
	- 한 프레임 17 기다린 다음에 다음 요청을 받음.
	*/
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

#if OVERLAPPED_IO_VERSION
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
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	hSendSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hSendSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// Overlapped 구조체 미리 할당해놓기
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}
	overlapped_IO_State = new Overlapped_IO_State;
	// 압축 텍스쳐 버퍼 미리 생성
	compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
#else
	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hRecvSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
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
	현재 참조하는, 압축 해제된 이미지가
	lock이 걸려있지 않으면 (업데이트 중이 아니면)
	업데이트를 한다. 그냥 아래에 있는 함수랑 합칠 수도 있다.
	*/
}

bool ImageReceiveManager::CanReceiveData()
{
	/*
	현재 참조하는, 압축 해제된 이미지가 
	lock이 걸려있지 않으면 (업데이트 중이 아니면)
	true를 반환한다.
	*/
}

void ImageReceiveManager::CleanUpManager()
{
	/*
	맴버 이미지 스레드를 종료시키고
	수신 스레드를 종료하고,
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

	// 송신측 정보
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200바이트로 넘어오는 패킷을 받아서 쌓는다.
	while (true)
	{
		if (::WaitForSingleObject(hHaltEvent, 0) == WAIT_OBJECT_0)
		{
			return 1;
		}

		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (과도하게 잘랐거나) 파일이 너무 큰 경우를 대비한다.
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
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	// Overlapped 구조체 미리 할당해놓기
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}

	// 압축 텍스쳐 버퍼 미리 생성
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