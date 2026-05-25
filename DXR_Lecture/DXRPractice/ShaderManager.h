#pragma once

#define		MAX_SHADER_NAME_BUFFER_LEN		256
#define		MAX_SHADER_NAME_LEN				(MAX_SHADER_NAME_BUFFER_LEN-1)
#define		MAX_SHADER_NUM					2048
#define		MAX_CODE_SIZE					(1024*1024)

struct SHADER_HANDLE
{
	DWORD dwFlags;
	DWORD dwCodeSize;
	DWORD dwShaderNameLen;
	WCHAR wchShaderName[MAX_SHADER_NAME_BUFFER_LEN];
	DWORD pCodeBuffer[1];
};

class D3D12Renderer;

class ShaderManager
{
public:
	bool Initialize(D3D12Renderer* _pRenderer, const WCHAR* _wchShaderPath, bool _bDisableOptimize);
	SHADER_HANDLE* CreateShaderDXC(const WCHAR* _wchShaderFileName, const WCHAR* _wchEntryPoint, const WCHAR* _wchShaderModel, DWORD _dwFlags);
	void ReleaseShader(SHADER_HANDLE* _pShaderHandle);

private:
	bool InitDXC();
	void CleanupDXC();
	void CleanupShaderManager();

private:
	HMODULE m_hDXL = nullptr;
	IDxcUtils* m_pDxcUtils = nullptr;
	IDxcCompiler3* m_pDxcCompiler = nullptr;
	IDxcIncludeHandler* m_pDxcIncludeHandler = nullptr;

	bool m_bDisableOptimize = false;
	WCHAR m_wchDefaultShaderPath[_MAX_PATH] = {};

public:
	ShaderManager();
	virtual ~ShaderManager();
};

