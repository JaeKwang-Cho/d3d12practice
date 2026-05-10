#include "pch.h"
#include "TextureManager.h"
#include "D3D12Renderer.h"
#include "D3D12ResourceManager.h"
#include "SingleDescriptorAllocator.h"

TextureManager::TextureManager()
	: m_pRenderer(nullptr), m_pResourceManager(nullptr)
{
}

TextureManager::~TextureManager()
{
	CleanUpTextureManager();
}

bool TextureManager::Initalize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;
	m_pResourceManager = m_pRenderer->INL_GetResourceManager();

	m_pTextureHashTable.clear();

	return true;
}

TEXTURE_HANDLE* TextureManager::CreateTextureFromFile_ITL(const WCHAR* _wchFileName)
{
	TEXTURE_HANDLE* pTexHandle = nullptr;

	std::wstring strFileName(_wchFileName);
	std::unordered_map<std::wstring, TEXTURE_HANDLE*>::iterator iter =  m_pTextureHashTable.find(strFileName);
	if (iter != m_pTextureHashTable.end()) {
		iter->second->dwRefCount++;
		pTexHandle = iter->second;

		return pTexHandle;
	}

	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();

	D3D12Resource_ptr pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};
	D3D12_RESOURCE_DESC desc = {};

	if(FAILED(m_pResourceManager->CreateTextureFromFile(&pTexResource, &desc, _wchFileName))) {
		__debugbreak();
		return nullptr;
	}
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = desc.Format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;

	if (!pSingleDescriptorAllocator->AllocDescriptorHandle(&srv)) {
		pTexResource->Release();
		pTexResource = nullptr;
	}

	pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &srvDesc, srv);

	pTexHandle = AllocTextureHandle_ITL();
	pTexHandle->pTexResource = pTexResource;
	pTexHandle->bFromFile = true;
	pTexHandle->srv = srv;
	pTexHandle->dwRefCount = 1;

	m_pTextureHashTable.insert(std::make_pair(strFileName, pTexHandle));

	return pTexHandle;
}

TEXTURE_HANDLE* TextureManager::CreateDynamicTexture_ITL(UINT _TexWidth, UINT _TexHeight)
{
	
}

TEXTURE_HANDLE* TextureManager::CreateImmutableTexture(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage)
{
	return nullptr;
}

void TextureManager::DeleteTexture(TEXTURE_HANDLE* _pTexHandle)
{
}

TEXTURE_HANDLE* TextureManager::AllocTextureHandle_ITL()
{
	return nullptr;
}

void TextureManager::CleanUpTextureManager()
{
}
