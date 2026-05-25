#pragma once

bool CreateShaderCodeFromFile(BYTE** _ppOutCodeBuffer, ULONG* _pulOutCodesize, SYSTEMTIME* _pOutLastWriteTime, const WCHAR* _wchFileName);

void DeleteShaderCode(BYTE* _pCodeBuffer);

HRESULT CompileShaderFromFileWithDXC(
	IDxcUtils* _pUtils,
	IDxcCompiler3* _pCompiler,
	IDxcIncludeHandler* _pIncludeHandler,
	const WCHAR* _wchFileName,
	const WCHAR* _wchEntryPoint,
	const WCHAR* _wchShaderModel,
	IDxcBlob** _ppOutCodeBlob,
	bool _bDisableOptimize,
	SYSTEMTIME* _pOutLastWriteTime,
	ULONG _ulFlags);