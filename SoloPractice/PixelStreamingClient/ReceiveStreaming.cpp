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
	ThreadParam_Client* pThreadParam = (ThreadParam_Client*)_pParam;

	SOCKET hSocket = pThreadParam->socket;
	char* pTexturePointer = (char*)pThreadParam->pData;
	SOCKADDR_IN addr = pThreadParam->addr;
	uint32_t numPackets = 0;

	// �۽��� ����
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200����Ʈ�� �Ѿ���� ��Ŷ�� �޾Ƽ� �״´�.
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
	return 0;
}

void WinSock_Props::InitializeWinSock()
{
	// winsock �ʱ�ȭ
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock�� �ʱ�ȭ �� �� �����ϴ�.");
	}

	// ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (hSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// ��Ʈ ���ε�
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"���Ͽ� IP�ּҿ� ��Ʈ�� ���ε� �� �� �����ϴ�.");
	}
}

void WinSock_Props::ReceiveData(void* _pData)
{
	// �޽��� �۽� ������ ����
	DWORD dwThreadID = 0;

	if (hThread == 0)
	{
		threadParam.pData = _pData;
		threadParam.socket = hSocket;
		threadParam.addr = addr;
		// ������ LoopBack���� �׽�Ʈ �Ѵ�.
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
	else // Ȥ�ó� �ؼ� ��ŵ ��Ű�� ����
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
	}
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
{
}

WinSock_Props::~WinSock_Props()
{
	CloseHandle(hThread);
	::closesocket(hSocket);
	::WSACleanup();
}
