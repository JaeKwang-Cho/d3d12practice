// dllmain.cpp : DLL 애플리케이션의 진입점을 정의합니다.
#include "pch.h"
#include "D3D12Renderer.h"

#pragma comment(lib, "DXGI.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "D3DCompiler.lib")
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment( lib, "d3d11.lib" ) 

// D3D12 전역변수
D3D12_HEAP_PROPERTIES HEAP_PROPS_DEFAULT = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
D3D12_HEAP_PROPERTIES HEAP_PROPS_UPLOAD = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
D3D12_HEAP_PROPERTIES HEAP_PROPS_READBACK = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
#ifdef _DEBUG
        int	flags = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF;
        _CrtSetDbgFlag(flags);
#endif
    }
    break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
    {
#ifdef _DEBUG
        _ASSERT(_CrtCheckMemory());
#endif
    }
        break;
    }
    return TRUE;
}

STDAPI DllCreateInstance(void** ppv)
{
    HRESULT hr;
    IRenderer* pRenderer = new D3D12Renderer;

    if (!pRenderer)
    {
        hr = E_OUTOFMEMORY;
        goto lb_return;
    }
    hr = S_OK;
    *ppv = pRenderer;
lb_return:
    return hr;
}