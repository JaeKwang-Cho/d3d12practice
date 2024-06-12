// pre-compiled header
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.

// Windows 헤더 파일
#include <windows.h>

// C 런타임 헤더 파일
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// 메모리릭 체크
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// D3D 라이브러리 링크
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// D3D 헤더 파일
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <DirectXColors.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>

// 괜히 최신 DirectX 사용하기 (d3d + dxgi)
#include <dxgi1_6.h>



// com 스마트 포인터
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// 어써트
#include <cassert>


