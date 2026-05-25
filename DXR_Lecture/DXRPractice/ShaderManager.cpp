#include "pch.h"
#include "ShaderManager.h"

bool ShaderManager::Initialize(D3D12Renderer* _pRenderer, const WCHAR* _wchShaderPath, bool _bDisableOptimize)
{
	return false;
}

SHADER_HANDLE* ShaderManager::CreateShaderDXC(const WCHAR* _wchShaderFileName, const WCHAR* _wchEntryPoint, const WCHAR* _wchShaderModel, DWORD _dwFlags)
{
	return nullptr;
}

void ShaderManager::ReleaseShader(SHADER_HANDLE* _pShaderHandle)
{
}

bool ShaderManager::InitDXC()
{
	return false;
}

void ShaderManager::CleanupDXC()
{
}

void ShaderManager::CleanupShaderManager()
{
}

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
}
