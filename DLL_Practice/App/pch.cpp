#include "pch.h"

std::filesystem::path GetExeDirectory()
{
    WCHAR wchModuleFileName[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, wchModuleFileName, MAX_PATH);
    return std::filesystem::path(wchModuleFileName).parent_path();
}