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

	//�۽��� ���� UDP ������ �ϳ� �� �����Ѵ�.
	SOCKET hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

	char* pData = (char*)pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;

	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	// 1200����Ʈ ���Ϸ� ����������, ������ ���ÿ��� �� �ذ��ҰŴ�.
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
	// winsock �ʱ�ȭ
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock�� �ʱ�ȭ �� �� �����ϴ�.");
	}

#if IOCP_VERSION
	// IOCP ����
	iocp = ::CreateIoCompletionPort(
		INVALID_HANDLE_VALUE,
		NULL,
		0,
		0);
	if (iocp == NULL) 
	{
		ErrorHandler(L"IOCP�� ������ �� �����ϴ�.");
	}

	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP,
		NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// IOCP�� UDP ���� ����
	if (CreateIoCompletionPort(
		(HANDLE)hSocket,
		iocp,
		(ULONG_PTR)hSocket,
		0
	) == NULL)
	{
		ErrorHandler(L"IOCP�� UDP�� ������ �� �����ϴ�.");
	}

	// ��Ŀ ������ ����
	for (int i = 0; i < THREAD_NUMBER_IOCP; i++)
	{
		::CreateThread(NULL, 0,	ThreadSentToClient_Worker, (LPVOID)iocp, 0,	NULL);
	}

#else
	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}
#endif
	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}
}

void WinSock_Properties::SendData(void* _data, size_t _ulByteSize)
{
#if IOCP_VERSION
	// �޽��� �۽� ������ ����
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
	else // Ȥ�ó� �ؼ� ��ŵ ��Ű�� ����
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
	}
#else
	// �޽��� �۽� ������ ����
	DWORD dwThreadID = 0;

	if (hThread == 0)
	{
		threadParam.data = _data;
		threadParam.socket = hSocket;
		threadParam.ulByteSize = _ulByteSize;

		threadParam.addr = addr;
		// ������ LoopBack���� �׽�Ʈ �Ѵ�.
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
	else // Ȥ�ó� �ؼ� ��ŵ ��Ű�� ����
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
