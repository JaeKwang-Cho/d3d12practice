#include "pch.h"
#include "typedef.h"
#include "SingleDescriptorAllocator.h"

TEXTURE_HANDLE::~TEXTURE_HANDLE()
{
	if (!OuterAllocator) {
		__debugbreak();
	}
	OuterAllocator->FreeDescriptorHandle(srv);
}
