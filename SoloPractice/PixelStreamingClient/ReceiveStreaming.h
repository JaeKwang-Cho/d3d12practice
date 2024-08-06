#pragma once
#include <WinSock2.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32")

#define OVERLAPPED_IO_VERSION (1)
#define LZ4_COMPRESSION (1)
#define SINGLE_THREAD (1)

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

#if LZ4_COMPRESSION
static const size_t OriginalTextureSize = (1280 * 720 * 4);
#endif
// =======================================

// D3D12Renderer_Client에서 사용하는 함수들.
void ErrorHandler(const wchar_t* _pszMessage);

// Server에서 이미지 데이터를 받는 스레드 함수
DWORD WINAPI ThreadReceiveFromServer(LPVOID _pParam);

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

struct ThreadParam_Client
{
	void* pData;
#if LZ4_COMPRESSION
	char* pCompressed;
#endif
	SOCKET socket;
	SOCKADDR_IN addr;
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	UINT64 sessionID;
};
#else
struct ThreadParam_Client
{
	void* pData;
	SOCKET socket;
	SOCKADDR_IN addr;
};
#endif
struct WinSock_Props
{
public:
	void InitializeWinSock();
	void ReceiveData(void* _pData);
	bool CanReceiveData();
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	SOCKET hSocket; // 송신 소켓
	bool bDrawing;
	HANDLE hThread;
	ThreadParam_Client threadParam;

#if OVERLAPPED_IO_VERSION
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	UINT64 sessionID;
#endif
#if LZ4_COMPRESSION
	char* compressedTexture;
#endif
public:
	WinSock_Props();
	virtual ~WinSock_Props();
};