#pragma once
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

#define OVERLAPPED_IO_VERSION (1)

// ============ Client 와 공유 ============ 
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
// =======================================

// ScreenStreamer에서 사용하는 함수들.
void ErrorHandler(const wchar_t* _pszMessage);

// Client에게 이미지 데이터를 보내는 스레드 함수
DWORD WINAPI ThreadSendToClient(LPVOID _pParam);

#if OVERLAPPED_IO_VERSION
struct Overlapped_IO_Data
{
	WSAOVERLAPPED wsaOL;
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
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
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

	SOCKET hSocket; // 송신 소켓
	HANDLE hThread;
	ThreadParam threadParam;
	
#if OVERLAPPED_IO_VERSION
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	UINT64 sessionID;
#endif
public:
	WinSock_Properties();
	virtual ~WinSock_Properties();
};


