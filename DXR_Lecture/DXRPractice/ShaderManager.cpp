#include "pch.h"
#include "D3D12Renderer.h"
#include "ShaderManager.h"

typedef DXC_API_IMPORT HRESULT(__stdcall *DxcCreateInstanceT)(_In_ REFCLSID rclsid, _In_ REFIID riid, _Out_ LPVOID* ppv);

bool ShaderManager::Initialize(D3D12Renderer* _pRenderer, const WCHAR* _wchShaderPath, bool _bDisableOptimize)
{
	m_bDisableOptimize = _bDisableOptimize;

	wcscpy_s(m_wchDefaultShaderPath, _wchShaderPath);

	if(!InitDXC())
	{
		__debugbreak();
		return false;
	}

	return true;
}

SHADER_HANDLE* ShaderManager::CreateShaderDXC(const WCHAR* _wchShaderFileName, const WCHAR* _wchEntryPoint, const WCHAR* _wchShaderModel, DWORD _dwFlags)
{
	SYSTEMTIME CreationTime = {};
	SHADER_HANDLE* pNewShaderHandle = nullptr;

	WCHAR wchOldPath[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, wchOldPath);

	Microsoft::WRL::ComPtr<IDxcBlob> pShaderBlob = nullptr;

	//	case DXIL::ShaderKind::Vertex:    entry = L"VSMain"; profile = L"vs_6_1"; break;
	// case DXIL::ShaderKind::Pixel:     entry = L"PSMain"; profile = L"ps_6_1"; break;
	// case DXIL::ShaderKind::Geometry:  entry = L"GSMain"; profile = L"gs_6_1"; break;
	// case DXIL::ShaderKind::Hull:      entry = L"HSMain"; profile = L"hs_6_1"; break;
	// case DXIL::ShaderKind::Domain:    entry = L"DSMain"; profile = L"ds_6_1"; break;
	// case DXIL::ShaderKind::Compute:   entry = L"CSMain"; profile = L"cs_6_1"; break;
	// case DXIL::ShaderKind::Mesh:      entry = L"MSMain"; profile = L"ms_6_5"; break;
	// case DXIL::ShaderKind::Amplification: entry = L"ASMain"; profile = L"as_6_5"; break;

	//"vs_6_0"
	//"ps_6_0"
	//"cs_6_0"
	//"gs_6_0"
	//"ms_6_5"
	//"as_6_5"
	//"hs_6_0"
	//"lib_6_3"

	SetCurrentDirectoryW(m_wchDefaultShaderPath);
	HRESULT hr = CompileShaderFromFileWithDXC(m_pDxcUtils.Get(), m_pDxcCompiler.Get(), m_pDxcIncludeHandler.Get(), _wchShaderFileName, _wchEntryPoint, _wchShaderModel, &pShaderBlob, m_bDisableOptimize, &CreationTime, 0);
	if (FAILED(hr)) {
		WriteDebugStringW(DEBUG_OUTPUT_TYPE::DEBUG_CONSOLE_TYPE, L"Failed to compile shader %s. Error Code : %s", _wchShaderFileName, GetLastError());
		return nullptr;
	}

	size_t ullShaderSize = pShaderBlob->GetBufferSize();
	const char* pCodeBuffer = static_cast<const char*>(pShaderBlob->GetBufferPointer());

	size_t ullShaderHandleSize = sizeof(SHADER_HANDLE) - sizeof(DWORD) + ullShaderSize;
	pNewShaderHandle = (SHADER_HANDLE*)malloc(ullShaderHandleSize);
	memset(pNewShaderHandle, 0, ullShaderHandleSize);

	memcpy(pNewShaderHandle->pCodeBuffer, pCodeBuffer, ullShaderSize);
	pNewShaderHandle->ullCodeSize = ullShaderSize;
	pNewShaderHandle->dwShaderNameLen = static_cast<DWORD>(swprintf_s(pNewShaderHandle->wchShaderName, L"%s-%s", _wchShaderFileName, _wchEntryPoint));

	SetCurrentDirectoryW(wchOldPath);

	return pNewShaderHandle;
}

void ShaderManager::ReleaseShader(SHADER_HANDLE* _pShaderHandle)
{
	free(_pShaderHandle);
}

bool ShaderManager::InitDXC()
{
	const WCHAR* wchDLLPath = nullptr;
#if defined(_M_AMD64)
	wchDLLPath = L"./DXC/x64";
#elif defined(_M_IX86)
	wchDLLPath = L"./DXC/x86";
#elif defined(_M_ARM64)
	wchDLLPath = L"./DXC/arm64";
#elif defined(_M_ARM64EC)
	wchDLLPath = L"./DXC/arm64";
#else
	#error Unsupported platform
#endif

	WCHAR wchOldPath[MAX_PATH];
	GetCurrentDirectoryW(MAX_PATH, wchOldPath);
	SetCurrentDirectoryW(wchDLLPath);

	m_hDXL = LoadLibrary(L"dxcompiler.dll");
	if (!m_hDXL) {
		__debugbreak();
		return false;
	}

	DxcCreateInstanceT DxcCreateInstanceFunc = (DxcCreateInstanceT)GetProcAddress(m_hDXL, "DxcCreateInstance");
	HRESULT hr = DxcCreateInstanceFunc(CLSID_DxcUtils, IID_PPV_ARGS(&m_pDxcUtils));
	if(FAILED(hr)) {
		__debugbreak();
		return false;
	}

	hr = DxcCreateInstanceFunc(CLSID_DxcCompiler, IID_PPV_ARGS(&m_pDxcCompiler));
	if(FAILED(hr)) {
		__debugbreak();
		return false;
	}

	m_pDxcUtils->CreateDefaultIncludeHandler(&m_pDxcIncludeHandler);

	SetCurrentDirectoryW(wchOldPath);

	return true;
}

void ShaderManager::CleanupDXC()
{
	if (m_hDXL)
	{
		FreeLibrary(m_hDXL);
		m_hDXL = nullptr;
	}
}

void ShaderManager::CleanupShaderManager()
{
	CleanupDXC();
}

ShaderManager::ShaderManager()
{
}

ShaderManager::~ShaderManager()
{
	CleanupShaderManager();
}
