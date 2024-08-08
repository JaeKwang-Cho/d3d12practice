#pragma once
#include <WinSock2.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32")

#define OVERLAPPED_IO_VERSION (1)
#define LZ4_COMPRESSION (1)
#define SINGLE_THREAD (1)

// ============ Server �� ���� ============ 
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

// D3D12Renderer_Client���� ����ϴ� �Լ���.
void ErrorHandler(const wchar_t* _pszMessage);

// Server���� �̹��� �����͸� �޴� ������ �Լ�
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
	// ��ȯ�� HANDLE�� ������ �۾� �Ϸ� �Ǻ�, �Ķ���ͷ� ���� �����ʹ� lock�� ���� �۾��� �ʿ��ϴ�.
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

	SOCKET hRecvSocket; // ���� ����
	SOCKET hSendSocket; // �۽� ����

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
������������
���������� ReceiveStream���ٰ� �ؽ��ĸ� ��û��.
-> ReceiveStream������ �ִ�. ����. �� �˷���.

ReceiveStream��
1. �ϴ� �ݺ��ؼ� receive�� �ϴ� thread�� ����
=> ���������� �� ��Ʈ��ȣ�� ��û�� ������.
=> ��� Ŭ���̾�Ʈ���� ������ �����ִ� ���� �̹����� ���� �� �ִ��� Ȯ����.
- Ŭ���̾�Ʈ�� ���� �̹����� ���� �� �ִ��Ŀ� ���� ������
- ������ ������ Ǯ����, ��� �����尡 �ִ� ���.
>> ����
- Server���� �˸°� �̹��� �����͸� ���� �� �ֵ���
 ������ + ���� + ��Ʈ��ȣ ����� ��Ƽ� ������.
- �׸��� �̹����� �����ϴ� �����带 �����Ű��, �ش� �����带 �� ������.
>> �Ұ���
- Server���� �ȵȴٰ� ����
- �׸��� �۾����� ������ �� �ι�°�� ������ �����带, ������Ŵ

=> ������
- �� ������ 17 ��ٸ� ������ ���� ��û�� ����.


2. �̹����� �޴� ������
- ��� ������ �����ϰ�, ���� port, socket, buffer�� ����
- ���⼭ �� �ް�, ������ �����ؼ� Ŭ���� ���� ���ۿ� �־����.
- �۾��� �� �������� ������ �����ϰ�, ���������尡 �̰��� ������.

3. ������ ������ �̹����� Ŭ�������� ������ �ϰ�,
- Session ID ������� ����� ��.
- circular�� �ϵ� ���� �ؼ�, �׻� �ֽ�ȭ �ϴ� ����
- Renderer ���� ��û�ϸ�, �� �տ� �ִ� ���� ���� �ش�.


*/