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
#ifndef _CRTDBG_MAP_ALLOC
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif
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

using namespace DirectX;

// ���� �ֽ� DirectX ����ϱ� (d3d + dxgi)
#include <dxgi1_6.h>
#include <d2d1_3.h>

// com ����Ʈ ������
#include <wrl.h>
using Microsoft::WRL::ComPtr;

// ���Ʈ
#include <cassert>

// �ڷ���
#include "typedef.h"

// ���̺� ����
#include "PSOKeys.h"

// Timer
#include "GameTimer.h"


// ǥ�� ������ ������� ���� ��
extern D3D12_HEAP_PROPERTIES HEAP_PROPS_DEFAULT;
extern D3D12_HEAP_PROPERTIES HEAP_PROPS_UPLOAD;
extern D3D12_HEAP_PROPERTIES HEAP_PROPS_READBACK;

// �⺻ ���µ�
extern TEXTURE_HANDLE* DEFAULT_WHITE_TEXTURE;
extern CONSTANT_BUFFER_MATERIAL DEFAULT_MATERIAL;

// const ����
const UINT SWAP_CHAIN_FRAME_COUNT = 3;
const UINT MAX_PENDING_FRAME_COUNT = SWAP_CHAIN_FRAME_COUNT - 1;

