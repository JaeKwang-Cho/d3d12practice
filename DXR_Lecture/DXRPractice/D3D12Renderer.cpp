#include "pch.h"
#include "D3D12Renderer.h"
#include "RayTracingManager.h"
#include "ShaderManager.h"

bool D3D12Renderer::Initialize(HWND _hWnd, bool _bEnableDebugLayer, bool _bEnableGBV, bool _bDebugShader, const WCHAR* _wchSahderPath)
{
	return false;
}

void D3D12Renderer::BeginRender()
{
}

void D3D12Renderer::EndRender()
{
}

void D3D12Renderer::Present()
{
}

bool D3D12Renderer::UpdateWindowSize(ULONG _width, ULONG _Height)
{
	return false;
}

void D3D12Renderer::CreateCommandList()
{
}

void D3D12Renderer::CleanupCommandList()
{
}

bool D3D12Renderer::CreateDescriptorHeapForRTV()
{
	return false;
}

void D3D12Renderer::CleanupDescriptorHeapForRTV()
{
}

bool D3D12Renderer::CreateDescriptorHeapForDSV()
{
	return false;
}

void D3D12Renderer::CleanupDescriptorHeapForDSV()
{
}

bool D3D12Renderer::CreateDepthStencilBuffer(UINT _Width, UINT _Height)
{
	return false;
}

void D3D12Renderer::CleanupDepthStencilBuffer()
{
}

void D3D12Renderer::CreateFence()
{
}

UINT64 D3D12Renderer::DoFence()
{
	return UINT64();
}

void D3D12Renderer::WaitForFenceValue()
{
}

void D3D12Renderer::CleanUpFence()
{
}

void D3D12Renderer::CleanupRenderer()
{
}

D3D12Renderer::D3D12Renderer()
{
}

D3D12Renderer::~D3D12Renderer()
{
}
