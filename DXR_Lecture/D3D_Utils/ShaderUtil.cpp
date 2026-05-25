#include "pch.h"
#include "ShaderUtil.h"
#include "../Utils/WriteDebugString.h"
#ifdef __INTELLISENSE__
#include "../DXRPractice/pch.h"
#endif


bool CreateShaderCodeFromFile(BYTE** _ppOutCodeBuffer, ULONG* _pulOutCodesize, SYSTEMTIME* _pOutLastWriteTime, const WCHAR* _wchFileName)
{
	ULONG ulOpenFlags = OPEN_EXISTING;
	ULONG ulAccessMode = GENERIC_READ;
	ULONG ulShare = FILE_SHARE_READ;

	WCHAR wchText[256] = {};

	CREATEFILE2_EXTENDED_PARAMETERS extendedParams = {};
	extendedParams.dwSize = sizeof(CREATEFILE2_EXTENDED_PARAMETERS);
	extendedParams.dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
	extendedParams.dwFileFlags = FILE_FLAG_SEQUENTIAL_SCAN;
	extendedParams.dwSecurityQosFlags = SECURITY_ANONYMOUS;
	extendedParams.lpSecurityAttributes = nullptr;
	extendedParams.hTemplateFile = nullptr;

	HANDLE hFile = CreateFile2(_wchFileName, ulAccessMode, ulShare, ulOpenFlags, &extendedParams);
	if (INVALID_HANDLE_VALUE == hFile) {
		swprintf_s(wchText, L"Failed to open file %s. Error code: %d\n",_wchFileName ,GetLastError());
		OutputDebugStringW(wchText);
		return false;
	}

	ULONG ulFileSize = GetFileSize(hFile, nullptr);
	if (ulFileSize > 1024 * 1024) {
		swprintf_s(wchText, L"File %s is too large. Size: %d bytes\n", _wchFileName, ulFileSize);
		OutputDebugStringW(wchText);
		CloseHandle(hFile);
		return false;
	}

	ULONG ulCodeSize = ulFileSize + 1;

	BYTE* pCodeBuffer = new BYTE[ulCodeSize];
	memset(pCodeBuffer, 0, ulCodeSize);

	ULONG ulBytesRead = 0;
	if(!ReadFile(hFile, pCodeBuffer, ulFileSize, &ulBytesRead, nullptr)) {
		swprintf_s(wchText, L"Failed to read file %s. Error code: %d\n", _wchFileName, GetLastError());
		OutputDebugStringW(wchText);
		delete[] pCodeBuffer;
		CloseHandle(hFile);
		return false;
	}

	FILETIME createTime, lastAccessTime, lastWriteTime;
	GetFileTime(hFile, &createTime, &lastAccessTime, &lastWriteTime);

	SYSTEMTIME sysLastWriteTime;
	FileTimeToSystemTime(&lastWriteTime, &sysLastWriteTime);

	*_ppOutCodeBuffer = pCodeBuffer;
	*_pulOutCodesize = ulCodeSize;
	*_pOutLastWriteTime = sysLastWriteTime;

	CloseHandle(hFile);

	return true;
}

void DeleteShaderCode(BYTE* _pCodeBuffer)
{
	delete[] _pCodeBuffer;
}

HRESULT CompileShaderFromFileWithDXC(IDxcUtils* _pUtils, IDxcCompiler3* _pCompiler, IDxcIncludeHandler* _pIncludeHandler, const WCHAR* _wchFileName, const WCHAR* _wchEntryPoint, const WCHAR* _wchShaderModel, IDxcBlob** _ppOutCodeBlob, bool _bDisableOptimize, SYSTEMTIME* _pOutLastWriteTime, ULONG _ulFlags)
{
	(void)_ulFlags;

	if (!_pUtils || !_pCompiler || !_wchFileName || !_wchShaderModel || !_ppOutCodeBlob || !_pOutLastWriteTime) {
		return E_INVALIDARG;
	}

	*_ppOutCodeBlob = nullptr;

	HRESULT hr = E_FAIL;
	HRESULT hrCompile = E_FAIL;

	SYSTEMTIME lastWriteTime = {};
	BYTE* pCodeBuffer = nullptr;
	ULONG ulCodeSize = 0;

	if (!CreateShaderCodeFromFile(&pCodeBuffer, &ulCodeSize, &lastWriteTime, _wchFileName)) {
		return hr;
	}

	const WCHAR* pArg[16] = {};
	ULONG ulArgCount = 0;

	pArg[ulArgCount++] = DXC_ARG_ENABLE_STRICTNESS; // 엄격한 컴파일을 활성화하여 잠재적인 문제를 조기에 발견
	pArg[ulArgCount++] = DXC_ARG_WARNINGS_ARE_ERRORS; // 모든 경고를 오류로 처리하여 코드 품질을 높임
	pArg[ulArgCount++] = DXC_ARG_PACK_MATRIX_ROW_MAJOR; // Row Major로 행렬을 패킹하여 쉐이더에서 사용할 때 호환성을 높임

	if (_bDisableOptimize) {
		pArg[ulArgCount++] = DXC_ARG_DEBUG; // 디버깅 정보를 포함하여 컴파일
		pArg[ulArgCount++] = L"-Qembed_debug"; // 디버깅 정보를 쉐이더 코드에 포함
		pArg[ulArgCount++] = DXC_ARG_SKIP_OPTIMIZATIONS; // 디버깅을 위해 최적화 스킵
	}
	else {
		pArg[ulArgCount++] = DXC_ARG_OPTIMIZATION_LEVEL3;
	}

	IDxcCompilerArgs* pCompilerArgs = nullptr;
	hr = _pUtils->BuildArguments(
		_wchFileName,
		_wchEntryPoint,
		_wchShaderModel,
		pArg,
		ulArgCount,
		nullptr,
		0,
		&pCompilerArgs);

	if (FAILED(hr)) {
		DeleteShaderCode(pCodeBuffer);
		return hr;
	}

	DxcBuffer sourceBuffer = {};
	sourceBuffer.Ptr = pCodeBuffer;
	sourceBuffer.Size = ulCodeSize > 0 ? static_cast<SIZE_T>(ulCodeSize - 1) : 0;
	sourceBuffer.Encoding = DXC_CP_ACP;

	IDxcResult* pCompileResult = nullptr;
	hr = _pCompiler->Compile(
		&sourceBuffer,
		pCompilerArgs->GetArguments(),
		pCompilerArgs->GetCount(),
		_pIncludeHandler,
		IID_PPV_ARGS(&pCompileResult));

	if (SUCCEEDED(hr) && pCompileResult) {
		hr = pCompileResult->GetStatus(&hrCompile);
		if (FAILED(hr)) {
			hrCompile = hr;
		}
	}
	else {
		hrCompile = hr;
	}

	if (SUCCEEDED(hrCompile)) {
		hr = pCompileResult->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(_ppOutCodeBlob), nullptr);
		if (FAILED(hr)) {
			hrCompile = hr;
		}
	}
	else if (pCompileResult) {
		IDxcBlobUtf8* pErrorBlob = nullptr;
		if (SUCCEEDED(pCompileResult->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&pErrorBlob), nullptr)) && pErrorBlob) {
			const char* pErrorMessage = pErrorBlob->GetStringPointer();
			if (pErrorMessage && pErrorBlob->GetStringLength() > 0) {
				WriteDebugStringA(DEBUG_OUTPUT_TYPE::DEBUG_CONSOLE_TYPE, "%s", pErrorMessage);
			}

			pErrorBlob->Release();
			pErrorBlob = nullptr;
		}
	}

	if (pCompileResult) {
		pCompileResult->Release();
		pCompileResult = nullptr;
	}
	if (pCompilerArgs) {
		pCompilerArgs->Release();
		pCompilerArgs = nullptr;
	}

	DeleteShaderCode(pCodeBuffer);
	*_pOutLastWriteTime = lastWriteTime;

	return hrCompile;
}
