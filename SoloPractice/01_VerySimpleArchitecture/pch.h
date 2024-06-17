// pre-compiled header
//

#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.

// Windows ��� ����
#include <windows.h>

// C ��Ÿ�� ��� ����
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// �޸𸮸� üũ
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// D3D ��� ����
#include <initguid.h>
#include <d3d12.h>
#include <d3dcompiler.h>
#include <d3d11on12.h>
#include <dwrite.h>
#include <dxgi.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>
#include <dxgidebug.h>

#include <DirectXColors.h>
#include <DirectXMath.h>

// ���� �ֽ� DirectX ����ϱ� (d3d + dxgi)
#include <dxgi1_6.h>
#include <d2d1_3.h>

// com ����Ʈ ������
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// ���Ʈ
#include <cassert>

