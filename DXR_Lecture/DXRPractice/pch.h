#pragma once

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers


#include <windows.h>
#include <initguid.h>

// d3d
#include <d3d12.h>
#include <d3dx12.h>
#include <dxgi1_4.h>
#include <d3d11on12.h>
#include <dwrite.h>
#include <d2d1_3.h>
#include <D3DCompiler.h>
#include <dxcapi.h>
#include <d3d12sdklayers.h>
#include <dxgidebug.h>

#include <DirectXColors.h>
#include <DirectXMath.h>
using namespace DirectX;

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// 괜히 최신 DirectX 사용하기 (d3d + dxgi)
#include <dxgi1_6.h>

// com 스마트 포인터
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// 어써트
#include <cassert>

#include "typedef.h"
#include "Renderer_typedef.h"

#ifdef _DEBUG
#define _CRTDBG_MAP_ALLOC
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
#endif

using namespace std;

#include "D3D12_SmartPointer_typedef.h"
#include "../Utils/IndexCreator.h"
#include "../Utils/WriteDebugString.h"
#include "../D3D_Utils/D3DUtil.h"
#include "../D3D_Utils/ShaderUtil.h"
