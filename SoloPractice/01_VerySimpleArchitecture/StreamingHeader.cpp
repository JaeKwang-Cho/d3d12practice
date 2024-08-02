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
#if IOCP_VERSION
	ThreadParam* pThreadParam = (ThreadParam*)_pParam;

	char* pData = (char*)pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;

	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	for (UINT i = 0; i < uiSendCount; i++)
	{
		WorkerThreadParam* workerThreadParam = new WorkerThreadParam;

		workerThreadParam->addr = pThreadParam->addr;
		workerThreadParam->sessionID = pThreadParam->sessionID;
		workerThreadParam->ulByteSize = pThreadParam->ulByteSize;

		int startOffset = i * DATA_SIZE;
		int endOffset = min(startOffset + DATA_SIZE, ulByteSize);
		memcpy(workerThreadParam->pData + HEADER_SIZE, pThreadParam->data, endOffset - startOffset);

		ScreenImageHeader header = { i, uiSendCount, workerThreadParam->sessionID };
		memcpy(workerThreadParam->pData, &header, HEADER_SIZE);

		workerThreadParam->wsabuf.buf = workerThreadParam->pData;
		workerThreadParam->wsabuf.len = endOffset - startOffset + HEADER_SIZE;
	
		int result = WSASendTo(
			pThreadParam->hSocket,
			&workerThreadParam->wsabuf, 1,
			nullptr, 0,
			(sockaddr*)&workerThreadParam->addr, sizeof(sockaddr),
			&workerThreadParam->emptyOL, NULL);
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			delete workerThreadParam;
			workerThreadParam = nullptr;
			ErrorHandler(L"WSASendTo Failed pending.\n");
		}
	}
#else
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
#endif
	return 0;
}

DWORD __stdcall ThreadSentToClient_Worker(LPVOID _pParam)
{
	HANDLE iocp = (HANDLE)_pParam;
	DWORD bytesTransferred;
	ULONG_PTR completionKey;
	ThreadParam* threadParam;

	while (true) {
		BOOL result = GetQueuedCompletionStatus(iocp, &bytesTransferred, &completionKey, (LPOVERLAPPED*)&threadParam, INFINITE);
		if (!result) {
			OutputDebugStringW(std::format(L"GQCS failed with error: {}\n", GetLastError()).c_str());
			continue;
		}
		delete threadParam;
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

#if IOCP_VERSION
	// IOCP 생성
	iocp = ::CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		0,
		0);
	if (iocp == NULL) 
	{
		ErrorHandler(L"IOCP를 생성할 수 없습니다.");
	}

	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// IOCP와 UDP 소켓 연결
	if (CreateIoCompletionPort(
		(HANDLE)hSocket,
		iocp,
		(ULONG_PTR)hSocket,
		0
	) == NULL)
	{
		ErrorHandler(L"IOCP와 UDP를 연결할 수 없습니다.");
	}

	// 워커 스레드 생성
	for (int i = 0; i < THREAD_NUMBER_IOCP; i++)
	{
		::CreateThread(NULL, 0,	ThreadSentToClient_Worker, (LPVOID)iocp, 0,	NULL);
	}

#else
	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}
#endif
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
#if IOCP_VERSION
	// 메시지 송신 스레드 생성
	DWORD dwThreadID = 0;

	if (hThread == 0)
	{
		SOCKADDR_IN addr = { 0 };
		addr.sin_family = AF_INET;
		addr.sin_port = htons(CLIENT_PORT);
		::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

		threadParam.data = _data;
		threadParam.ulByteSize = _ulByteSize;
		threadParam.addr = addr;
		threadParam.sessionID = sessionID;
		threadParam.hSocket = hSocket;
		memcpy(threadParam.hWorkerThreads, hWorkerThreads, sizeof(hWorkerThreads));

		hThread = ::CreateThread(
			NULL,
			0,
			ThreadSendToClient,
			(LPVOID)(&threadParam),
			0,
			&dwThreadID
		);

		sessionID++;
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
#else
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
#endif
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
	:wsa{ 0 }, addr{ 0 }, hSocket(0), hThread(0), threadParam{ {0}, {0}, {0} }, sessionID(0)
#if IOCP_VERSION
	,iocp(0), hWorkerThreads{}
#endif
{
}

WinSock_Properties::~WinSock_Properties()
{
	CloseHandle(hThread);
	::closesocket(hSocket);
#if IOCP_VERSION
	CloseHandle(iocp);
#endif
	::WSACleanup();
}
