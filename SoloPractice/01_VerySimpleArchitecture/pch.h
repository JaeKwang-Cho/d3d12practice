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

// D3D ���̺귯�� ��ũ
#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")

// D3D ��� ����
#include <d3d12.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <DirectXColors.h>
#include <d3dx12.h>
#include <d3d12sdklayers.h>

// ���� �ֽ� DirectX ����ϱ� (d3d + dxgi)
#include <dxgi1_6.h>



// com ����Ʈ ������
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// ���Ʈ
#include <cassert>


