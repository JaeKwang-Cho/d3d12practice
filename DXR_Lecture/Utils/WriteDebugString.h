#pragma once

enum class DEBUG_OUTPUT_TYPE
{
	NULL_TYPE,
	CONSOLE_TYPE,
	DEBUG_CONSOLE_TYPE
};

void WriteDebugStringW(DEBUG_OUTPUT_TYPE type, const WCHAR* wchFormat, ...);
void WriteDebugStringA(DEBUG_OUTPUT_TYPE type, const char* szFormat, ...);
