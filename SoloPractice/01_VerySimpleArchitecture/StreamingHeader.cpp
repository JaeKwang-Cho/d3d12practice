#include "pch.h"
#include "StreamingHeader.h"
#include <format>
#include <WS2tcpip.h>
#include <lz4.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

DWORD WINAPI ThreadSendToClient(LPVOID _pParam)
{
	ThreadParam* pThreadParam = (ThreadParam*)_pParam;

	char* pData = (char*)pThreadParam->data;
	size_t ulByteSize = pThreadParam->ulByteSize;
	UINT64 uiSessionID = pThreadParam->uiSessionID;
	size_t uiSendCount = (ulByteSize + DATA_SIZE - 1) / DATA_SIZE;

	// Overlapped I/O ��û
	for (UINT i = 0; i < uiSendCount; i++)
	{
		UINT currOverlappedIOData_Index = i % MAXIMUM_WAIT_OBJECTS;

		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (�����ϰ� �߶��ų�) ������ �ʹ� ū ��츦 ����Ѵ�.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, INFINITE);
		}

		// ���� �����͸� �غ��ϰ�
		curOverlapped_Param->addr = pThreadParam->addr;
		curOverlapped_Param->ulByteSize = pThreadParam->ulByteSize;

		int startOffset = i * DATA_SIZE;
		int endOffset = min(startOffset + DATA_SIZE, ulByteSize);
		memcpy(curOverlapped_Param->pData + HEADER_SIZE, pData + startOffset, endOffset - startOffset);

		ScreenImageHeader header = { i, uiSendCount, uiSessionID };
		memcpy(curOverlapped_Param->pData, &header, HEADER_SIZE);

		curOverlapped_Param->wsabuf.buf = curOverlapped_Param->pData;
		curOverlapped_Param->wsabuf.len = endOffset - startOffset + HEADER_SIZE;
		int addrLen = sizeof(curOverlapped_Param->addr);

		// Overlapped I/O ��û�� �Ǵ�.
		if (curOverlapped_Param->wsaOL.hEvent == 0)
		{
			curOverlapped_Param->wsaOL.hEvent = ::WSACreateEvent();
		}
		else
		{
			WSAResetEvent(curOverlapped_Param->wsaOL.hEvent);
		}

		int result = WSASendTo(
			pThreadParam->hSendSocket,
			&curOverlapped_Param->wsabuf, 1,
			nullptr, 0,
			(sockaddr*)&curOverlapped_Param->addr, addrLen,
			&curOverlapped_Param->wsaOL, NULL);

		int lastError = WSAGetLastError();
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			ErrorHandler(L"WSASendTo Failed pending.\n");
		}
	}

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent, INFINITE);
			pThreadParam->overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		}
	}
	return 0;
}

void ImageSendManager::InitializeWinsock()
{
	// winsock �ʱ�ȭ
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock�� �ʱ�ȭ �� �� �����ϴ�.");
	}

	// �۽� ���� ����. SOCK_DGRAM, IPPROTO_UDP ���� ����
	hSendSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP,	NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSendSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP ������ ������ �� �����ϴ�.");
	}

	// Overlapped ����ü �̸� �Ҵ��س���
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}
	overlapped_IO_State = new Overlapped_IO_State;
	// ���� �ؽ��� ���� �̸� �Ҵ�
	compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
}

void ImageSendManager::SendData(void* _data, size_t _ulByteSize, UINT _uiThreadIndex)
{
	// �޽��� �۽� ������ ����
	DWORD dwThreadID = 0;

	if (hThread[_uiThreadIndex] != 0) // Ȥ�ó� �ؼ� ��ŵ ��Ű�� ����
	{
		DWORD result = WaitForSingleObject(hThread[_uiThreadIndex], 0);
		if (result == WAIT_OBJECT_0)
		{
			DWORD dwExitCode = -1;
			GetExitCodeThread(hThread[_uiThreadIndex], &dwExitCode);
			if (dwExitCode == 0)
			{
				sessionID++;
			}
			CloseHandle(hThread[_uiThreadIndex]);
			hThread[_uiThreadIndex] = 0;
		}
		else
		{
			return;
		}
	}
	// Ŭ���̾�Ʈ ����
	SOCKADDR_IN addr = { 0 };
	addr.sin_family = AF_INET;

	addr.sin_port = htons(CLIENT_PORT);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

	int inputSize = _ulByteSize;
	int compressSize = LZ4_compress_fast((char*)_data, compressedTexture, inputSize, LZ4_compressBound(inputSize), 1);

	threadParam.data = compressedTexture;
	threadParam.ulByteSize = compressSize;
	//threadParam.data = _data;
	//threadParam.ulByteSize = _ulByteSize;
	threadParam.addr = addr;
	threadParam.hSendSocket = hSendSocket;
	threadParam.uiSessionID = uiSessionID++;
	memcpy(threadParam.overlapped_IO_Data, overlapped_IO_Data, sizeof(overlapped_IO_Data));

	//ThreadSendToClient((LPVOID)(&threadParam));
	hThread[_uiThreadIndex] = ::CreateThread(
		NULL,
		0,
		ThreadSendToClient,
		(LPVOID)(&threadParam),
		0,
		&dwThreadID
	);
}

bool ImageSendManager::CanSendData(UINT _uiThreadIndex)
{
	if (hThread[_uiThreadIndex] == 0)
	{
		return true;
	}
	DWORD result = WaitForSingleObject(hThread[_uiThreadIndex], 0);
	if (result == WAIT_OBJECT_0)
	{
		CloseHandle(hThread[_uiThreadIndex]);
		hThread[_uiThreadIndex] = 0;
		return true;
	}
	else if (result == WAIT_TIMEOUT)
	{
		return false;
	}
	else
	{
		__debugbreak();
		return false;
	}
}

ImageSendManager::ImageSendManager()
	:wsa{ 0 }, addr{ 0 }, hSendSocket(0), hThread(0), threadParam{ {0}, {0}, {0} }, uiSessionID(0),
	overlapped_IO_Data{}, overlapped_IO_State(nullptr), sessionID(0),compressedTexture(nullptr)
{
}

ImageSendManager::~ImageSendManager()
{
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			if (overlapped_IO_Data[i]->wsaOL.hEvent != 0)
			{
				::WaitForSingleObject(overlapped_IO_Data[i]->wsaOL.hEvent, INFINITE);
				overlapped_IO_Data[i]->wsaOL.hEvent = 0;
			}
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}
	if (overlapped_IO_State)
	{
		delete overlapped_IO_State;
		overlapped_IO_State = nullptr;
	}
	if (compressedTexture)
	{
		delete[] compressedTexture;
		compressedTexture = nullptr;
	}
	for (UINT i = 0; i < SWAP_CHAIN_FRAME_COUNT; i++)
	{
		CloseHandle(hThread[i]);
	}
	::closesocket(hSendSocket);
	::WSACleanup();
}
