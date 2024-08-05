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
#if OVERLAPPED_IO_VERSION
	ThreadParam* pThreadParam = (ThreadParam*)_pParam;

	char* pData = (char*)pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;

	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	// Overlapped I/O ��û
	for (UINT i = 0; i < uiSendCount; i++)
	{
		UINT currOverlappedIOData_Index = i % MAXIMUM_WAIT_OBJECTS;

		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (�����ϰ� �߶��ų�) ������ �ʹ� ū ��츦 ����Ѵ�.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, INFINITE);
		}

		// ���� �����͸� �غ��ϰ�
		curOverlapped_Param->addr = pThreadParam->addr;
		curOverlapped_Param->sessionID = pThreadParam->sessionID;
		curOverlapped_Param->ulByteSize = pThreadParam->ulByteSize;

		int startOffset = i * DATA_SIZE;
		int endOffset = min(startOffset + DATA_SIZE, ulByteSize);
		memcpy(curOverlapped_Param->pData + HEADER_SIZE, pData + startOffset, endOffset - startOffset);

		ScreenImageHeader header = { i, uiSendCount, curOverlapped_Param->sessionID };
		memcpy(curOverlapped_Param->pData, &header, HEADER_SIZE);

		curOverlapped_Param->wsabuf.buf = curOverlapped_Param->pData;
		curOverlapped_Param->wsabuf.len = endOffset - startOffset + HEADER_SIZE;
		int addrLen = sizeof(curOverlapped_Param->addr);

		// Overlapped I/O ��û�� �Ǵ�.
		if (curOverlapped_Param->wsaOL.hEvent == 0)
		{
			curOverlapped_Param->wsaOL.hEvent = ::WSACreateEvent();
		}
		else
		{
			WSAResetEvent(curOverlapped_Param->wsaOL.hEvent);
		}
		
	
		int result = WSASendTo(
			pThreadParam->hSocket,
			&curOverlapped_Param->wsabuf, 1,
			nullptr, 0,
			(sockaddr*)&curOverlapped_Param->addr, addrLen,
			&curOverlapped_Param->wsaOL, NULL);

		int lastError = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			ErrorHandler(L"WSASendTo Failed pending.\n");
		}
	}

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent, INFINITE);
			pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent = 0;
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

void WinSock_Properties::InitializeWinsock()
{
	// winsock �ʱ�ȭ
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock�� �ʱ�ȭ �� �� �����ϴ�.");
	}

#if OVERLAPPED_IO_VERSION
	/*
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
	*/
	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP,	NULL, 0, WSA_FLAG_OVERLAPPED);
	//hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}
	// Overlapped ����ü �̸� �Ҵ��س���
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}

#else
	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(SERVER_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}
#endif
}

void WinSock_Properties::SendData(void* _data, size_t _ulByteSize)
{
#if OVERLAPPED_IO_VERSION
	// �޽��� �۽� ������ ����
	DWORD dwThreadID = 0;

	if (hThread != 0) // Ȥ�ó� �ؼ� ��ŵ ��Ű�� ����
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
	// Ŭ���̾�Ʈ ����
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

	threadParam.data = _data;
	threadParam.ulByteSize = _ulByteSize;
	threadParam.addr = addr;
	threadParam.sessionID = sessionID;
	threadParam.hSocket = hSocket;
	memcpy(threadParam.overlapped_IO_Data, overlapped_IO_Data, sizeof(overlapped_IO_Data));

	//ThreadSendToClient((LPVOID)(&threadParam));
	hThread = ::CreateThread(
		NULL,
		0,
		ThreadSendToClient,
		(LPVOID)(&threadParam),
		0,
		&dwThreadID
	);
	sessionID++;

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
#if OVERLAPPED_IO_VERSION
	/*, iocp(0)*/, overlapped_IO_Data{}
#endif
{
}

WinSock_Properties::~WinSock_Properties()
{
#if OVERLAPPED_IO_VERSION
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			if (overlapped_IO_Data[i]->wsaOL.hEvent != 0)
			{
				::WaitForSingleObject(overlapped_IO_Data[i]->wsaOL.hEvent, INFINITE);
				overlapped_IO_Data[i]->wsaOL.hEvent = 0;
			}
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}
#endif
	CloseHandle(hThread);
	::closesocket(hSocket);
	::WSACleanup();
}
