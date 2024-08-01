#pragma once
#include <WinSock2.h>
#include <stdint.h>

#pragma comment(lib, "ws2_32")

// ============ Client �� ���� ============ 
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
#define THREAD_NUMBER_BY_FRAME (3)
// =======================================

// D3D12Renderer_Client���� ����ϴ� �Լ���.
void ErrorHandler(const wchar_t* _pszMessage);

// Server���� �̹��� �����͸� �޴� ������ �Լ�
DWORD WINAPI ThreadReceiveFromServer(LPVOID _pParam);

struct ThreadParam_Client
{
	void* pData;
	SOCKET socket;
	SOCKADDR_IN addr;
};

struct WinSock_Props
{
public:
	void InitializeWinSock();
	void ReceiveData(UINT _uiThreadIndex, void* _pData);
	bool CanReceiveData(UINT _uiThreadIndex);
private:
	WSADATA wsa;
	SOCKADDR_IN addr;

	SOCKET hSocket; // �۽� ����
	HANDLE hThread[THREAD_NUMBER_BY_FRAME];
	ThreadParam_Client threadParam[THREAD_NUMBER_BY_FRAME];
public:
	WinSock_Props();
	virtual ~WinSock_Props();
};