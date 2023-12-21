#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // ���� ������ �ʴ� ������ Windows ������� �����մϴ�.
// Windows ��� ����
#include <windows.h>
#include <vector>
using namespace std;
#include <string>

// C ��Ÿ�� ��� �����Դϴ�.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>

// �޸𸮸�
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// d3d12 ���̺귯�� ��ũ
#pragma comment(lib,"d3dcompiler.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")

// d3d���
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <DirectXColors.h>

using namespace DirectX;

// ����� ���
#include "../Common/d3dx12.h"

// ����Ʈ ������
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// ���Ʈ
#include <cassert>

// IKnown ��ü�� GUID�� ���δٰ� �Ѵ�. �׷��� �װ� ���ϰ� �����ִ� ��ũ��
#define IID_PPV_ARGS(ppType) __uuidof(**(ppType)), IID_PPV_ARGS_Helper(ppType)