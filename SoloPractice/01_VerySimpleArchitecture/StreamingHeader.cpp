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

	SOCKET hSocket = pThreadParam->socket;
	void* pData = pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;
	SOCKADDR_IN addr = pThreadParam->addr;

	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	// 1200바이트 이하로 보낼꺼여서, 스레드 스택에서 다 해결할거다.
	for (UINT i = 0; i < uiSendCount; i++) 
	{
		UINT8 buffer[MAX_PACKET_SIZE];
		ScreenImageHeader header = { i, uiSendCount };
		memcpy(buffer, &header, HEADER_SIZE);

		int startOffset = i * DATA_SIZE;
		int endOffset = min(startOffset + DATA_SIZE, ulByteSize);
		memcpy(buffer + HEADER_SIZE, pData, endOffset - startOffset);

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

	UINT curThreadIndex = m_uiThreadIndex;
	m_uiThreadIndex = (m_uiThreadIndex + 1) % THREAD_NUMBER_BY_FRAME;

	if (hThread == 0)
	{
		threadParam[curThreadIndex].data = _data;
		threadParam[curThreadIndex].socket = hSocket;
		threadParam[curThreadIndex].ulByteSize = _ulByteSize;

		threadParam[curThreadIndex].addr = addr;
		// 지금은 LoopBack으로 테스트 한다.
		// threadParam[curThreadIndex].addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
		::InetPton(AF_INET, _T("127.0.0.1"), &threadParam[curThreadIndex].addr.sin_addr);

		hThread[curThreadIndex] = ::CreateThread(
			NULL,
			0,
			ThreadSendToClient,
			(LPVOID)(&threadParam[curThreadIndex]),
			0,
			&dwThreadID
		);
	}
	else // 스레드 안 끝났으면 그냥 스킵. 그냥 화면만 보여주는 거니까 ㄱㅊ
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread[m_uiThreadIndex] = 0;
		}
	}
}

WinSock_Properties::~WinSock_Properties()
{
	for (UINT i = 0; i < THREAD_NUMBER_BY_FRAME; i++) {
		CloseHandle(hThread[i]);
	}
	::closesocket(hSocket);
	::WSACleanup();
}
