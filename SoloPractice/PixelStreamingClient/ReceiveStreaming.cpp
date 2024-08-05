#include "pch.h"
#include "ReceiveStreaming.h"
#include <format>
#include <WS2tcpip.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

DWORD __stdcall ThreadReceiveFromServer(LPVOID _pParam)
{
#if OVERLAPPED_IO_VERSION
	ThreadParam_Client* pThreadParam = (ThreadParam_Client*)_pParam;

	SOCKET hSocket = pThreadParam->socket;
	char* pTexturePointer = (char*)pThreadParam->pData;
	SOCKADDR_IN addr = pThreadParam->addr;
	uint32_t numPackets = 0;

	UINT64 curSessionID = pThreadParam->sessionID;
	bool bGetFirstSessionFlags = false;

	// 송신측 정보
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200바이트로 넘어오는 패킷을 받아서 쌓는다.
	while (true)
	{
		UINT currOverlappedIOData_Index = numPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (과도하게 잘랐거나) 파일이 너무 큰 경우를 대비한다.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 1);
			if (result != WAIT_TIMEOUT)
			{
				ScreenImageHeader header;
				memcpy(&header, curOverlapped_Param->pData, HEADER_SIZE);

				if (bGetFirstSessionFlags == false)
				{
					curSessionID = header.sessionID;
					bGetFirstSessionFlags = true;
				}

				if (MAX_PACKET_SIZE >= curOverlapped_Param->ulByteSize)
				{
					int dataOffset = DATA_SIZE * header.currPacketNumber;
					char* pDataToWrite = (char*)curOverlapped_Param->wsabuf.buf;
					memcpy(pTexturePointer + dataOffset, pDataToWrite + HEADER_SIZE, curOverlapped_Param->ulByteSize);

					if (numPackets >= header.totalPacketsNumber - 1)
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
		int result = WSARecv(hSocket, &curOverlapped_Param->wsabuf, 1, &curOverlapped_Param->ulByteSize, &flags, &curOverlapped_Param->wsaOL, nullptr);
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			break;
		}
		numPackets++;
	}

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent, INFINITE);

			ScreenImageHeader header;
			memcpy(&header, pThreadParam->overlapped_IO_Data[i]->pData, HEADER_SIZE);
			if (MAX_PACKET_SIZE >= pThreadParam->overlapped_IO_Data[i]->ulByteSize)
			{
				int dataOffset = DATA_SIZE * header.currPacketNumber;
				char* pDataToWrite = (char*)pThreadParam->overlapped_IO_Data[i]->pData;
				memcpy(pTexturePointer + dataOffset, pDataToWrite + HEADER_SIZE, pThreadParam->overlapped_IO_Data[i]->ulByteSize - HEADER_SIZE);
			}
			pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		}
	}
#else
	ThreadParam_Client* pThreadParam = (ThreadParam_Client*)_pParam;

	SOCKET hSocket = pThreadParam->socket;
	char* pTexturePointer = (char*)pThreadParam->pData;
	SOCKADDR_IN addr = pThreadParam->addr;
	uint32_t numPackets = 0;

	// 송신측 정보
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200바이트로 넘어오는 패킷을 받아서 쌓는다.
	while (true)
	{
		UINT8 buffer[MAX_PACKET_SIZE];
		int bytesReceived = recvfrom(hSocket, (char*)buffer, MAX_PACKET_SIZE, 0, (sockaddr*)&serverAddr, &sizeAddr);
		if (bytesReceived == SOCKET_ERROR)
		{
			break;
		}

		ScreenImageHeader header;
		memcpy(&header, buffer, HEADER_SIZE);

		int dataOffset = DATA_SIZE * header.currPacketNumber;
		char* pDataToWrite = (char*)buffer;
		memcpy(pTexturePointer + dataOffset, pDataToWrite + HEADER_SIZE, bytesReceived - HEADER_SIZE);
		numPackets++;
		if (numPackets >= header.totalPacketsNumber - 1)
		{
			break;
		}
	}
#endif
	return 0;
}

void WinSock_Props::InitializeWinSock()
{
	// winsock 초기화
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock을 초기화 할 수 없습니다.");
	}

#if OVERLAPPED_IO_VERSION
	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	// Overlapped 구조체 미리 할당해놓기
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}
#else
	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}
#endif
}

void WinSock_Props::ReceiveData(void* _pData)
{
#if OVERLAPPED_IO_VERSION
	// 메시지 송신 스레드 생성
	DWORD dwThreadID = 0;

	if (hThread != 0) // 혹시나 해서 스킵 시키는 로직
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
	threadParam.pData = _pData;
	threadParam.socket = hSocket;
	threadParam.addr = addr;
	threadParam.sessionID = sessionID;
	memcpy(threadParam.overlapped_IO_Data, overlapped_IO_Data, sizeof(overlapped_IO_Data));
	// 지금은 LoopBack으로 테스트 한다.
	::InetPton(AF_INET, _T("127.0.0.1"), &threadParam.addr.sin_addr);

	//ThreadReceiveFromServer((LPVOID)(&threadParam));

	hThread = ::CreateThread(
		NULL,
		0,
		ThreadReceiveFromServer,
		(LPVOID)(&threadParam),
		0,
		&dwThreadID
	);
	
	sessionID++;
#else
	// 메시지 송신 스레드 생성
	DWORD dwThreadID = 0;

	if (hThread == 0)
	{
		threadParam.pData = _pData;
		threadParam.socket = hSocket;
		threadParam.addr = addr;
		// 지금은 LoopBack으로 테스트 한다.
		// threadParam[_uiThreadIndex].addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		::InetPton(AF_INET, _T("127.0.0.1"), &threadParam.addr.sin_addr);

		hThread= ::CreateThread(
			NULL,
			0,
			ThreadReceiveFromServer,
			(LPVOID)(&threadParam),
			0,
			&dwThreadID
		);
	}
	else // 혹시나 해서 스킵 시키는 로직
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
	}
#endif
}

bool WinSock_Props::CanReceiveData()
{
	if(hThread == 0)
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
	else
	{
		__debugbreak();
		return false;
	}
}

WinSock_Props::WinSock_Props()
	:wsa{ 0 }, addr{ 0 }, hSocket(0), hThread(0), threadParam{ {0}, {0}, {0} }
#if OVERLAPPED_IO_VERSION
	, overlapped_IO_Data{}, sessionID(0)
#endif
{
}

WinSock_Props::~WinSock_Props()
{
#if OVERLAPPED_IO_VERSION
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}
#endif
	CloseHandle(hThread);
	::closesocket(hSocket);
	::WSACleanup();
}
