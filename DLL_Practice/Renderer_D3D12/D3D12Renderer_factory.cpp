#include "pch.h"
#include "../Common/IRenderer.h"
#include "D3D12Renderer.h"

std::unique_ptr<IRenderer> CreateRenderer()
{
	return std::make_unique<D3D12Renderer>();
}