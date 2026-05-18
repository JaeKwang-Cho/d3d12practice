#pragma once
#include <thread>
#include <semaphore>

class D3D12Renderer;

struct RENDER_THREAD_DESC
{
	D3D12Renderer* Renderer = nullptr;
	ULONG uiThreadIndex = 0;
	std::jthread Thread;
	std::binary_semaphore ProcessSignal{ 0 };
	std::binary_semaphore FinishSignal{ 0 };
};

void RenderThreadMain(std::stop_token _StopToken, RENDER_THREAD_DESC* _pDesc);


