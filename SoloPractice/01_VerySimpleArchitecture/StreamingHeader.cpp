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

	// Overlapped I/O 요청
	for (UINT i = 0; i < uiSendCount; i++)
	{
		UINT currOverlappedIOData_Index = i % MAXIMUM_WAIT_OBJECTS;

		Overlapped_IO_Data* curOverlapped_Param = pThreadParam->overlapped_IO_Data[currOverlappedIOData_Index];

		// (과도하게 잘랐거나) 파일이 너무 큰 경우를 대비한다.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, INFINITE);
		}

		// 보낼 데이터를 준비하고
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

		// Overlapped I/O 요청을 건다.
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
	// winsock 초기화
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock을 초기화 할 수 없습니다.");
	}

	// 송신 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hSendSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP,	NULL, 0, WSA_FLAG_OVERLAPPED);
	if (hSendSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// Overlapped 구조체 미리 할당해놓기
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}
	overlapped_IO_State = new Overlapped_IO_State;
	// 압축 텍스쳐 버퍼 미리 할당
	compressedTexture = new char[LZ4_compressBound(OriginalTextureSize)];
}

void ImageSendManager::SendData(void* _data, size_t _ulByteSize, UINT _uiThreadIndex)
{
	// 메시지 송신 스레드 생성
	DWORD dwThreadID = 0;

	if (hThread[_uiThreadIndex] != 0) // 혹시나 해서 스킵 시키는 로직
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
	// 클라이언트 정보
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
