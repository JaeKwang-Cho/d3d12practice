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

struct Overlapped_IO_State
{
	WSAOVERLAPPED wsaOL;
	WSABUF wsabuf;
	SOCKADDR_IN addr;
};

struct ScreenImageHeader
{
	uint32_t currPacketNumber;
	uint32_t totalPacketsNumber;
	UINT64 uiSessionID;
	DWORD totalCompressedSize;
};

#define SERVER_PORT (4567)
#define CLIENT_PORT (7654)
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

#define IMAGE_NUM_FOR_BUFFERING (15)

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
};

struct CompressedTextures
{
	char* pCompressedData = nullptr;
	DWORD compressedSize;
	SRWLOCK lock;
};

struct ImageReceiveManager
{
public:
	void InitializeWinSock();
	bool StartReceiveManager(D3D12Renderer_Client* _renderer);

	D3D12Renderer_Client* const GetRenderer() {	return renderer;}

	UINT64 GetImagesNums();
	UINT64 IncreaseImageNums();
	UINT64 DecreaseImageNums();
	bool TryDecompressNCopyNextBuffer(void* _pUploadData, UINT64 _circularIndex);

	HANDLE GetHaltEvent() { return hHaltEvent; }
private:
	void CleanUpManager();
public:
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	CompressedTextures* rawTextureBuffer[IMAGE_NUM_FOR_BUFFERING];
	CompressedTextures* validTextureBuffer[IMAGE_NUM_FOR_BUFFERING];

	SOCKET hRecvSocket; // 수신 소켓
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	HANDLE hThread;
	HANDLE hHaltEvent;

	SRWLOCK updateCountLock;
	SRWLOCK renderCountLock;

	UINT64 renderedImageCount;
	UINT64 lastRenderedCircularIndex;

	UINT64 updatedImagesCount;
	UINT64 lastUpdatedCircularIndex;

	D3D12Renderer_Client* renderer;
public:
	ImageReceiveManager();
	virtual ~ImageReceiveManager();
};
