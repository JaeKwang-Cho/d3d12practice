#pragma once
#include <WinSock2.h>
#include <stdint.h>
#include <Windows.h>

class D3D12Renderer_Client;

#pragma comment(lib, "ws2_32")

#define OVERLAPPED_IO_VERSION (1)
#define LZ4_COMPRESSION (1)
#define SINGLE_THREAD (1)

// ============ Server 와 공유 ============ 
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
	UINT64 uiSessionID;
};

#define SERVER_PORT (4567)
#define CLIENT_PORT (7654)
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

#define IMAGE_NUM_FOR_BUFFERING (30)

#if LZ4_COMPRESSION
static const size_t s_OriginalTextureSize = (1280 * 720 * 4);
#endif
// =======================================

// D3D12Renderer_Client에서 사용하는 함수들.
void ErrorHandler(const wchar_t* _pszMessage);

// Server에서 이미지 데이터를 받는 스레드 함수
DWORD WINAPI ThreadManageReceiverAndSendToServer(LPVOID _pParam);

struct Overlapped_IO_Data
{
	WSAOVERLAPPED wsaOL;
	WSABUF wsabuf;
	char pData[MAX_PACKET_SIZE];
	SOCKADDR_IN addr;
	DWORD ulByteSize;
	UINT64 sessionID;
};

struct CompressedTextures
{
	char* pCompressedData = nullptr;
	DWORD compressedSize;
	SRWLOCK lock;
	bool bRendered = false;
};

struct ImageReceiveManager
{
public:
	void InitializeWinSock();
	bool StartReceiveManager(D3D12Renderer_Client* _renderer);

	void IncreaseImageNums();
	bool DecompressNCopyNextBuffer(void* _pUploadData);

	HANDLE GetHaltEvent() { return hHaltEvent; }
private:
	void CleanUpManager();
public:
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	CompressedTextures* compressedTextureBuffer[IMAGE_NUM_FOR_BUFFERING];

	SOCKET hRecvSocket; // 수신 소켓
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	HANDLE hThread;
	HANDLE hHaltEvent;

	SRWLOCK countLock;

	UINT64 lastRenderedCircularIndex;
	UINT64 lastUpdatedCircularIndex;
	UINT64 numImages;

	D3D12Renderer_Client* renderer;
public:
	ImageReceiveManager();
	virtual ~ImageReceiveManager();
};
