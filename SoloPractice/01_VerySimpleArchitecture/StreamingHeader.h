#pragma once
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

#define IOCP_VERSION (1)

// ============ Client �� ���� ============ 
struct ScreenImageHeader
{
	uint32_t currPacketNumber;
	uint32_t totalPacketsNumber;
	UINT64 sessionID;
};

#define SERVER_PORT (4567)
#define CLIENT_PORT (7654)
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)
#define THREAD_NUMBER_IOCP (3)
// =======================================

// ScreenStreamer���� ����ϴ� �Լ���.
void ErrorHandler(const wchar_t* _pszMessage);

// Client���� �̹��� �����͸� ������ ������ �Լ�
DWORD WINAPI ThreadSendToClient(LPVOID _pParam);
DWORD WINAPI ThreadSentToClient_Worker(LPVOID _pParam);

#if IOCP_VERSION
struct WorkerThreadParam
{
	WSAOVERLAPPED emptyOL;
	WSABUF wsabuf;
	char pData[MAX_PACKET_SIZE];
	SOCKADDR_IN addr;
	size_t ulByteSize;
	UINT64 sessionID;
};

struct ThreadParam
{
	void* data;
	SOCKADDR_IN addr;
	HANDLE hWorkerThreads[THREAD_NUMBER_IOCP];
	size_t ulByteSize;
	UINT64 sessionID;
	SOCKET hSocket;
};

#else
struct ThreadParam
{
	void* data;
	SOCKET socket;
	size_t ulByteSize;
	SOCKADDR_IN addr;
};
#endif
struct WinSock_Properties
{
public:
	void InitializeWinsock();
	void SendData(void* _data, size_t _ulByteSize);
	bool CanSendData();
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	SOCKET hSocket; // �۽� ����
	HANDLE hThread;
	ThreadParam threadParam;
	
#if IOCP_VERSION
	HANDLE iocp;
	HANDLE hWorkerThreads[THREAD_NUMBER_IOCP];
	UINT64 sessionID;
#endif
public:
	WinSock_Properties();
	virtual ~WinSock_Properties();
};


