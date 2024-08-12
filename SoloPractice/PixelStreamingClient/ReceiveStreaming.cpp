#include "pch.h"
#include "ReceiveStreaming.h"
#include <format>
#include <WS2tcpip.h>
#include "D3D12Renderer_Client.h"

#include <lz4.h>

void ErrorHandler(const wchar_t* _pszMessage)
{
	OutputDebugStringW(std::format(L"ERROR: {}\n", _pszMessage).c_str());
	::WSACleanup();
	__debugbreak();
	exit(1);
}

static ULONGLONG g_PrevUpdateTime = 0;

DWORD __stdcall ThreadManageReceiverAndSendToServer(LPVOID _pParam)
{
	ImageReceiveManager* imageReceiveMananger = (ImageReceiveManager*)_pParam;

	while (::WaitForSingleObject(imageReceiveMananger->GetHaltEvent(), 0) != WAIT_OBJECT_0)
	{
		ULONGLONG CurrTickTime = GetTickCount64();
		if (CurrTickTime - g_PrevUpdateTime > 66) // 15FPS
		{
			g_PrevUpdateTime = CurrTickTime;
			UINT64 index;
			UINT64 nums;
			imageReceiveMananger->GetIndexAndNums(index, nums);

			bool result = imageReceiveMananger->TryGetImageData_Threaded(index);

			if (result)
			{
				imageReceiveMananger->IncreaseImageNums();
			}
		}
	}
	return 0;
}

void ImageReceiveManager::InitializeWinSock()
{
	// winsock 초기화
	wsa = { 0 };
	if (::WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
	{
		ErrorHandler(L"WinSock을 초기화 할 수 없습니다.");
	}

	// 소켓 생성. SOCK_DGRAM, IPPROTO_UDP 으로 설정
	hRecvSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(CLIENT_PORT);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	hSendSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hSendSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 작업용 클래스와, 이미지 버퍼 미리 생성.
	for (UINT i = 0; i < IMAGE_NUM_FOR_BUFFERING; i++)
	{
		imageReceivers[i] = new ImageReceiver;
		imageReceivers[i]->Initialize(s_Receiver_Ports[i]);

		decompressedTextures_circular[i] = new DecompressedTextures;
		decompressedTextures_circular[i]->lock = SRWLOCK_INIT;
	}
}

bool ImageReceiveManager::StartReceiveManager(D3D12Renderer_Client* _renderer, char* _ppData[IMAGE_NUM_FOR_BUFFERING])
{
	renderer = _renderer;

	// mappedData 를 변수에 넣어놓고
	for (UINT i = 0; i < IMAGE_NUM_FOR_BUFFERING; i++)
	{
		decompressedTextures_circular[i]->pData = _ppData[i];
	}

	DWORD dwThreadID = 0;

	if (hThread != 0)
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
		else
		{
			return false;
		}
	}
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);
	hHaltEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	hThread = ::CreateThread(
		NULL,
		0,
		ThreadManageReceiverAndSendToServer,
		(LPVOID)(this),
		0,
		&dwThreadID
	);

	return hThread != NULL;
} 

bool ImageReceiveManager::CheckNextImageReady(UINT& _outIndex)
{
	if (numImages <= 15)
	{
		return false;
	}
	UINT curIndex = (lastUpdatedCircularIndex - 15 + IMAGE_NUM_FOR_BUFFERING) % IMAGE_NUM_FOR_BUFFERING;
	bool result = imageReceivers[curIndex]->CheckReceiverState();
	if (result = true)
	{
		_outIndex = curIndex;
	}
	return result;
}

bool ImageReceiveManager::TryGetImageData_Threaded(UINT64 _indexReceiver)
{
	bool result = imageReceivers[_indexReceiver]->CheckReceiverState() && renderer->CheckTextureDrawing(_indexReceiver);
	if (!result)
	{
		return false;
	}
	return imageReceivers[_indexReceiver]->ReceiveImage_Threaded(decompressedTextures_circular[_indexReceiver]);
}

void ImageReceiveManager::IncreaseImageNums()
{
	AcquireSRWLockExclusive(&countLock);
	numImages++;
	lastUpdatedCircularIndex = (lastUpdatedCircularIndex + 1) % IMAGE_NUM_FOR_BUFFERING;
	ReleaseSRWLockExclusive(&countLock);
}

void ImageReceiveManager::GetIndexAndNums(UINT64& _index, UINT64& _nums)
{
	_index = lastUpdatedCircularIndex;
	_nums = numImages;
}

void ImageReceiveManager::CleanUpManager()
{
	::SetEvent(hHaltEvent);
	::WaitForSingleObject(hThread, INFINITE);
	::CloseHandle(hHaltEvent);

	for (UINT i = 0; i < IMAGE_NUM_FOR_BUFFERING; i++)
	{
		if (imageReceivers[i])
		{
			delete imageReceivers[i];
			imageReceivers[i] = nullptr;
		}
		if (decompressedTextures_circular[i])
		{
			delete decompressedTextures_circular[i];
			decompressedTextures_circular[i] = nullptr;
		}
	}

	CloseHandle(hThread);
	::closesocket(hRecvSocket);
	::closesocket(hSendSocket);
	::WSACleanup();
}

ImageReceiveManager::ImageReceiveManager()
	:wsa{ 0 }, addr{ 0 }, hRecvSocket(0), hSendSocket(0), hThread(0), hHaltEvent(0),
	imageReceivers{}, numImages(0), lastUpdatedCircularIndex(0), decompressedTextures_circular{},
	renderer(nullptr)
{
	countLock = SRWLOCK_INIT;
}

ImageReceiveManager::~ImageReceiveManager()
{
	CleanUpManager();
}

// ================= ImageReceiver ================= 

DWORD __stdcall ThreadReceiveImageFromServer(LPVOID _pParam)
{
	ImageReceiver* pImageReceiver = (ImageReceiver*)_pParam;

	uint32_t numTryToGetPackets = 0;
	uint32_t totalPacketNumber = 0;
	size_t totalByteSize = 0;

	// 송신측 정보
	SOCKADDR_IN serverAddr;
	int sizeAddr = sizeof(serverAddr);
	// 1200바이트로 넘어오는 패킷을 받아서 쌓는다.
	while (true)
	{
		if (::WaitForSingleObject(pImageReceiver->hHaltEvent, 0) == WAIT_OBJECT_0)
		{
			return 1;
		}

		UINT currOverlappedIOData_Index = numTryToGetPackets % MAXIMUM_WAIT_OBJECTS;
		Overlapped_IO_Data* curOverlapped_Param = pImageReceiver->overlapped_IO_Data[currOverlappedIOData_Index];

		// (과도하게 잘랐거나) 파일이 너무 큰 경우를 대비한다.
		if (curOverlapped_Param->wsaOL.hEvent != 0)
		{
			DWORD result = ::WaitForSingleObject(curOverlapped_Param->wsaOL.hEvent, 1);
			if (result != WAIT_TIMEOUT)
			{
				ScreenImageHeader header;
				memcpy(&header, curOverlapped_Param->pData, HEADER_SIZE);

				if (MAX_PACKET_SIZE >= curOverlapped_Param->ulByteSize)
				{
					int dataOffset = DATA_SIZE * header.currPacketNumber;
					char* pDataToWrite = (char*)curOverlapped_Param->wsabuf.buf;
					totalByteSize += curOverlapped_Param->ulByteSize;
					memcpy(pImageReceiver->compressedTexture + dataOffset, pDataToWrite + HEADER_SIZE, curOverlapped_Param->ulByteSize);

					if (header.currPacketNumber >= totalPacketNumber - 1)
					{
						break;
					}
				}
			}
		}

		curOverlapped_Param->wsabuf.buf = curOverlapped_Param->pData;
		curOverlapped_Param->wsabuf.len = MAX_PACKET_SIZE;

		// Overlapped I/O 요청을 건다.
		if (curOverlapped_Param->wsaOL.hEvent == 0)
		{
			curOverlapped_Param->wsaOL.hEvent = ::WSACreateEvent();
		}
		else
		{
			WSAResetEvent(curOverlapped_Param->wsaOL.hEvent);
		}
		DWORD flags = 0;
		int result = WSARecv(pImageReceiver->hRecvSocket, &curOverlapped_Param->wsabuf, 1, &curOverlapped_Param->ulByteSize, &flags, &curOverlapped_Param->wsaOL, nullptr);
		if (result == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			break;
		}
		numTryToGetPackets++;
		if (numTryToGetPackets >= totalPacketNumber - 1)
		{
			break;
		}
	}

	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (::WaitForSingleObject(pImageReceiver->hHaltEvent, 0) == WAIT_OBJECT_0)
		{
			return 1;
		}

		if (pImageReceiver->overlapped_IO_Data[i]->wsaOL.hEvent != 0)
		{
			::WaitForSingleObject(pImageReceiver->overlapped_IO_Data[i]->wsaOL.hEvent, 10);

			ScreenImageHeader header;
			memcpy(&header, pImageReceiver->overlapped_IO_Data[i]->pData, HEADER_SIZE);
			if (MAX_PACKET_SIZE >= pImageReceiver->overlapped_IO_Data[i]->ulByteSize)
			{
				int dataOffset = DATA_SIZE * header.currPacketNumber;
				char* pDataToWrite = (char*)pImageReceiver->overlapped_IO_Data[i]->pData;
				totalByteSize += pImageReceiver->overlapped_IO_Data[i]->ulByteSize;
				memcpy(pImageReceiver->compressedTexture + dataOffset, pDataToWrite + HEADER_SIZE, pImageReceiver->overlapped_IO_Data[i]->ulByteSize - HEADER_SIZE);
			}
			pImageReceiver->overlapped_IO_Data[i]->wsaOL.hEvent = 0;
		}
	}
	int decompressSize = LZ4_decompress_safe(pImageReceiver->compressedTexture, pImageReceiver->decompressedTexture->pData, totalByteSize, s_OriginalTextureSize);
	return 0;
}

void ImageReceiver::Initialize(const UINT _portNum)
{
	portNum = _portNum;

	hRecvSocket = ::WSASocket(AF_INET, SOCK_DGRAM, IPPROTO_UDP, nullptr, 0, WSA_FLAG_OVERLAPPED);
	if (hRecvSocket == INVALID_SOCKET)
	{
		ErrorHandler(L"UDP 소켓을 생성할 수 없습니다.");
	}

	// 포트 바인딩
	addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(portNum);
	addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	if (::bind(hRecvSocket, (SOCKADDR*)&addr, sizeof(addr)) == SOCKET_ERROR)
	{
		ErrorHandler(L"소켓에 IP주소와 포트를 바인드 할 수 없습니다.");
	}

	// Overlapped 구조체 미리 할당해놓기
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		overlapped_IO_Data[i] = new Overlapped_IO_Data;
		overlapped_IO_Data[i]->wsaOL.hEvent = 0;
	}

	// 압축 텍스쳐 버퍼 미리 생성
	compressedTexture = new char[LZ4_compressBound(s_OriginalTextureSize)];
}

bool ImageReceiver::CheckReceiverState()
{
	if (hThread == 0)
	{
		return true;
	}
	DWORD result = WaitForSingleObject(hThread, 0);
	if (result == WAIT_OBJECT_0)
	{
		CloseHandle(hThread);
		hThread = 0;
		return true;
	}
	else if (result == WAIT_TIMEOUT)
	{
		return false;
	}
	__debugbreak();
	return false;
}

bool ImageReceiver::ReceiveImage_Threaded(DecompressedTextures* _decompTex)
{
	DWORD dwThreadID = 0;

	if (hThread != 0)
	{
		DWORD result = WaitForSingleObject(hThread, 0);
		if (result == WAIT_OBJECT_0)
		{
			CloseHandle(hThread);
			hThread = 0;
		}
		else
		{
			return false;
		}
	}
	if (hHaltEvent != 0)
	{
		::CloseHandle(hHaltEvent);
		hHaltEvent = 0;
	}
	hHaltEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	::InetPton(AF_INET, _T("127.0.0.1"), &addr.sin_addr);

	hThread = ::CreateThread(
		NULL,
		0,
		ThreadReceiveImageFromServer,
		(LPVOID)(this),
		0,
		&dwThreadID
	);

	return hThread != NULL;
}

void ImageReceiver::HaltThread()
{
	::SetEvent(hHaltEvent);
	::WaitForSingleObject(hThread, INFINITE);

	::CloseHandle(hThread);
	::CloseHandle(hHaltEvent);
}

ImageReceiver::ImageReceiver()
	:addr{}, hRecvSocket(0), portNum(0), hThread(0), hHaltEvent(0), overlapped_IO_Data{}, compressedTexture(nullptr), decompressedTexture(nullptr)
{
}

ImageReceiver::~ImageReceiver()
{
	HaltThread();
	::closesocket(hRecvSocket);
	for (UINT i = 0; i < MAXIMUM_WAIT_OBJECTS; i++)
	{
		if (overlapped_IO_Data[i])
		{
			delete overlapped_IO_Data[i];
			overlapped_IO_Data[i] = nullptr;
		}
	}
	if (compressedTexture)
	{
		delete compressedTexture;
		compressedTexture = nullptr;
	}
}

// =================================================== 