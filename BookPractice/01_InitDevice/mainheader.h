#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
// Windows 헤더 파일
#include <windows.h>
#include <vector>
using namespace std;
#include <string>

// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// 메모리릭
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// d3d12 라이브러리 링크
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// d3d헤더
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <DirectXColors.h>

using namespace DirectX;

// 도우미 헤더
#include "../Common/d3dx12.h"

// 스마트 포인터
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// 어써트
#include <cassert>

// IKnown 객체의 GUID가 쓰인다고 한다. 그래서 그걸 편하게 구해주는 메크로
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)