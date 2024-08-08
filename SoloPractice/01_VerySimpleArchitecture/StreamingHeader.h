#pragma once
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

#define OVERLAPPED_IO_VERSION (1)
#define LZ4_COMPRESSION (1)

// ============ Client �� ���� ============ 
enum class E_ClientState : UINT8
{
	FULL = 0,
	READY = 1,
	END
};

struct StateExchange
{
	uint32_t totalPacketsNumber;
	size_t compressedSize;
	E_ClientState eClientState;
};

struct Overlapped_IO_State
{
	WSAOVERLAPPED wsaOL;
	WSABUF wsabuf;
	StateExchange state;
	SOCKADDR_IN addr;
	DWORD ulByteSize;
	UINT64 sessionID;
};

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

#if LZ4_COMPRESSION
static const size_t OriginalTextureSize = (1280 * 720 * 4);
#endif
// =======================================

// ScreenStreamer���� ����ϴ� �Լ���.
void ErrorHandler(const wchar_t* _pszMessage);

// Client���� �̹��� �����͸� ������ ������ �Լ�
DWORD WINAPI ThreadSendToClient(LPVOID _pParam);

#if OVERLAPPED_IO_VERSION
struct Overlapped_IO_Data
{
	WSAOVERLAPPED wsaOL;
	WSABUF wsabuf;
	char pData[MAX_PACKET_SIZE];
	SOCKADDR_IN addr;
	DWORD ulByteSize;
	UINT64 sessionID;
};

struct ThreadParam
{
	void* data;
	SOCKADDR_IN addr;
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	Overlapped_IO_State* overlapped_IO_State;
	DWORD ulByteSize;
	UINT64 sessionID;
	SOCKET hSendSocket;
	SOCKET hRecvSocket;
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

	SOCKET hSendSocket; // �۽� ����
	SOCKET hRecvSocket; // ���� ����
	HANDLE hThread;
	ThreadParam threadParam;
	
#if OVERLAPPED_IO_VERSION
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	Overlapped_IO_State* overlapped_IO_State;
	UINT64 sessionID;
#endif

#if LZ4_COMPRESSION
	char* compressedTexture;
#endif

public:
	WinSock_Properties();
	virtual ~WinSock_Properties();
};


