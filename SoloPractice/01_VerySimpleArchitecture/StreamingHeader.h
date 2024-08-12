#pragma once
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

// ============ Client 와 공유 ============ 

#define IMAGE_NUM_FOR_BUFFERING (16)

static const UINT s_Receiver_Ports[IMAGE_NUM_FOR_BUFFERING] =
{ 7601, 7602, 7603, 7604, 7605,
7606, 7607, 7608, 7609, 7610,
7611, 7612, 7613, 7614, 7615, 7616 };

struct Overlapped_IO_State
{
	WSAOVERLAPPED wsaOL;
	WSABUF wsabuf;
	SOCKADDR_IN addr;
	DWORD ulByteSize;
	UINT64 sessionID;
};

struct ScreenImageHeader
{
	uint32_t currPacketNumber;
	uint32_t totalPacketsNumber;
};

#define SERVER_PORT (4567)
#define CLIENT_PORT (7654)
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

static const size_t OriginalTextureSize = (1280 * 720 * 4);
// =======================================

// ScreenStreamer에서 사용하는 함수들.
void ErrorHandler(const wchar_t* _pszMessage);

// Client에게 이미지 데이터를 보내는 스레드 함수
DWORD WINAPI ThreadSendToClient(LPVOID _pParam);

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
	DWORD ulByteSize;
	SOCKET hSendSocket;
};
struct ImageSendManager
{
public:
	void InitializeWinsock();
	void SendData(void* _data, size_t _ulByteSize);
	bool CanSendData();
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	SOCKET hSendSocket; // 송신 소켓
	HANDLE hThread;
	ThreadParam threadParam;
	UINT textureIndexByThread;
	
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	Overlapped_IO_State* overlapped_IO_State;
	UINT64 sessionID;

	char* compressedTexture;

public:
	ImageSendManager();
	virtual ~ImageSendManager();
};


