#pragma once
#include <WinSock2.h>

#pragma comment(lib, "ws2_32")

// ============ Client �� ���� ============ 
struct ScreenImageHeader
{
	uint32_t currPacketNumber;
	uint32_t totalPacketsNumber;
};

#define SERVER_PORT (4567) // client�� 7654
#define MAX_PACKET_SIZE (1200)
#define HEADER_SIZE sizeof(ScreenImageHeader)
#define DATA_SIZE (MAX_PACKET_SIZE - HEADER_SIZE)
#define THREAD_NUMBER_BY_FRAME (3)
// =======================================

// ScreenStreamer���� ����ϴ� �Լ���.
void ErrorHandler(const wchar_t* _pszMessage);

// Client���� �̹��� �����͸� ������ ������ �Լ�
DWORD WINAPI ThreadSendToClient(LPVOID _pParam);

struct ThreadParam
{
	void* data;
	SOCKET socket;
	size_t ulByteSize;
	SOCKADDR_IN addr;
};

struct WinSock_Properties
{
public:
	void InitializeWinsock();
	void SendData(void* _data, size_t _ulByteSize);
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	UINT m_uiThreadIndex;

	SOCKET hSocket; // �۽� ����
	HANDLE hThread[THREAD_NUMBER_BY_FRAME];
	ThreadParam threadParam[THREAD_NUMBER_BY_FRAME];
public:
	WinSock_Properties() = default;
	virtual ~WinSock_Properties();
};


