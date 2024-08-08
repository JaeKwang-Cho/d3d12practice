#pragma once
#include <WinSock2.h>
#include <stdint.h>

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
	UINT64 sessionID;
};

#define SERVER_PORT (4567)
#define CLIENT_PORT (7654)
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)

#define THREAD_NUM_FOR_BUFFERING (5)

#if LZ4_COMPRESSION
static const size_t OriginalTextureSize = (1280 * 720 * 4);
#endif

static const UINT Receiver_Ports[THREAD_NUM_FOR_BUFFERING] = {7655, 7656, 7657, 7658, 7659 };
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
	SOCKADDR_IN addr;
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	UINT64 sessionID;
	SOCKET hRecvSocket;
	SOCKET hSendSocket;
};
#else
struct ThreadParam_Client
{
	void* pData;
	SOCKET socket;
	SOCKADDR_IN addr;
};
#endif
struct ThreadParam_Receiver
{
	void* pData;
#if LZ4_COMPRESSION
	char* pCompressed;
#endif
	SOCKADDR_IN addr;
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	UINT64 sessionID;
	SOCKET hRecvSocket;
	uint32_t totalPacketNumber;
	DWORD totalByteSize;
	HANDLE hHaltEvent;
};

struct ImageReceiver
{
public:
	void Initialize(const UINT _portNum);
	// true : Ready / false : Running
	bool CheckReceiverState();
	UINT GetPortNumber() { return portNum; }
	// 반환값 HANDLE로 스레드 작업 완료 판별, 파라미터로 들어가는 포인터는 lock과 같은 작업이 필요하다.
	HANDLE ReceiveImage(void* _pOutImage);
	void HaltThread();
private:
	SOCKADDR_IN addr;
	SOCKET hRecvSocket;
	UINT portNum;
	HANDLE hThread;
	HANDLE hHaltEvent;
	Overlapped_IO_Data* overlapped_IO_Data[MAXIMUM_WAIT_OBJECTS];
	ThreadParam_Receiver threadParam;
	char* compressedTexture;
public:
	ImageReceiver();
	virtual~ImageReceiver();
};


struct ImageReceiveManager
{
public:
	void InitializeWinSock();
	void StartReceiveManager();

	void GetImageData(void* _pData);
	bool CanReceiveData();
private:
	void CleanUpManager();
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	SOCKET hRecvSocket; // 수신 소켓
	SOCKET hSendSocket; // 송신 소켓

	HANDLE hThread;
	ThreadParam_Client threadParam;

#if OVERLAPPED_IO_VERSION
	Overlapped_IO_State* overlapped_IO_State;

	ImageReceiver* imageReceivers[THREAD_NUM_FOR_BUFFERING];
	UINT64 lastUpdatedCircularIndex;
#endif
#if LZ4_COMPRESSION
	char* decompressedTextures_circular[THREAD_NUM_FOR_BUFFERING];
#endif
public:
	ImageReceiveManager();
	virtual ~ImageReceiveManager();
};

/*
* 
렌더러에서는
무지성으로 ReceiveStream에다가 텍스쳐를 요청함.
-> ReceiveStream에서는 있다. 없다. 만 알려줌.

ReceiveStream은
1. 일단 반복해서 receive를 하는 thread가 있음
=> 서버에서는 얘 포트번호로 요청을 보낸다.
=> 얘는 클라이언트에서 서버가 보내주는 압축 이미지를 받을 수 있는지 확인함.
- 클라이언트가 압축 이미지를 받을 수 있느냐에 대한 조건은
- 생성한 스레드 풀에서, 노는 스레드가 있는 경우.
>> 가능
- Server에게 알맞게 이미지 데이터를 보낼 수 있도록
 스레드 + 버퍼 + 포트번호 등등을 담아서 보내줌.
- 그리고 이미지를 수신하는 스레드를 실행시키고, 해당 스레드를 또 관리함.
>> 불가능
- Server에게 안된다고 보냄
- 그리고 작업중인 스레드 중 두번째로 오래된 스레드를, 중지시킴

=> 마무리
- 한 프레임 17 기다린 다음에 다음 요청을 받음.


2. 이미지를 받는 스레드
- 얘는 여러개 존재하고, 각각 port, socket, buffer가 있음
- 여기서 다 받고, 압축을 해제해서 클래스 관리 버퍼에 넣어놓음.
- 작업이 다 끝났으면 스스로 종료하고, 관리스레드가 이것을 감지함.

3. 압축을 해제한 이미지는 클래스에서 관리를 하고,
- Session ID 순서대로 기록을 함.
- circular로 하든 뭐든 해서, 항상 최신화 하는 느낌
- Renderer 에서 요청하면, 맨 앞에 있는 것을 빼서 준다.


*/