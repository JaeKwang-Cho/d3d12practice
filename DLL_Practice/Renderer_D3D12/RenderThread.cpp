#include "pch.h"
#include "RenderThread.h"
#include "D3D12Renderer.h"

void RenderThreadMain(std::stop_token _StopToken, RENDER_THREAD_DESC* _pDesc)
{
	while (!_StopToken.stop_requested()) {
		_pDesc->ProcessSignal.acquire();
		if(_StopToken.stop_requested()) {
			break;
		}
		_pDesc->Renderer->ProcessByThread(_pDesc->uiThreadIndex);
	}
}