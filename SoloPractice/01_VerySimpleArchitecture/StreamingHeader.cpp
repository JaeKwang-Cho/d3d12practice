#include "pch.h"
#include "StreamingHeader.h"
#include <format>
#include <WS2tcpip.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

DWORD WINAPI ThreadSendToClient(LPVOID _pParam)
{
	ThreadParam* pThreadParam = (ThreadParam*)_pParam;

	//송신을 위한 UDP 소켓을 하나 더 개방한다.
	SOCKET hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

	char* pData = (char*)pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;

	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	// 1200바이트 이하로 보낼꺼여서, 스레드 스택에서 다 해결할거다.
	for (UINT i = 0; i < uiSendCount; i++) 
	{
		UINT8 buffer[MAX_PACKET_SIZE];
		ScreenImageHeader header = { i, uiSendCount };
		memcpy(buffer, &header, HEADER_SIZE);

		int startOffset = i * DATA_SIZE;
		int endOffset = min(startOffset + DATA_SIZE, ulByteSize);
		memcpy(buffer + HEADER_SIZE, pData + startOffset, endOffset - startOffset);

		::sendto(hSocket, (char*)buffer,
			endOffset - startOffset + HEADER_SIZE, 0,
			(sockaddr*)&addr, sizeof(addr)
		);
	}

	return 0;
}

void WinSock_Properties::InitializeWinsock()
{
	// winsock 초기화
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock을 초기화 할 수 없습니다.");
	}

	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}
}

void WinSock_Properties::SendData(void* _data, size_t _ulByteSize)
{
	// 메시지 송신 스레드 생성
	DWORD dwThreadID = 0;

	if (hThread == 0)
	{
		threadParam.data = _data;
		threadParam.socket = hSocket;
		threadParam.ulByteSize = _ulByteSize;

		threadParam.addr = addr;
		// 지금은 LoopBack으로 테스트 한다.
		// threadParam[_uiThreadIndex].addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		::InetPton(AF_INET, _T("127.0.0.1"), &threadParam.addr.sin_addr);

		hThread = ::CreateThread(
			NULL,
			0,
			ThreadSendToClient,
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
}

bool WinSock_Properties::CanSendData()
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
	else
	{
		__debugbreak();
		return false;
	}
}

WinSock_Properties::WinSock_Properties()
	:wsa{ 0 }, addr{ 0 }, hSocket(0), hThread(0), threadParam{{0}, {0}, {0}}
{
}

WinSock_Properties::~WinSock_Properties()
{
	CloseHandle(hThread);
	::closesocket(hSocket);
	::WSACleanup();
}
