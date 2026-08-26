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

bool TextureManager::Initialize(D3D12Renderer* _pRenderer)
{
	m_pRenderer = _pRenderer;
	m_pResourceManager = m_pRenderer->INL_GetResourceManager();

	m_TextureHashTable.clear();
	m_TextureReverseHashTable.clear();
	m_TextureHashSet.clear();

	return true;
}

TEXTURE_HANDLE* TextureManager::CreateTextureFromFile_ITL(const WCHAR* _wchFileName)
{
	TEXTURE_HANDLE* pTexHandle = nullptr;

	std::wstring strFileName(_wchFileName);
	std::map<std::wstring, std::unique_ptr<TEXTURE_HANDLE>>::iterator iter =  m_TextureHashTable.find(strFileName);
	if (iter != m_TextureHashTable.end()) {
		iter->second->ulRefCount++;
		return iter->second.get();
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
	pTexHandle->srvCpuHandle = srv;

	m_TextureReverseHashTable.insert(std::make_pair(pTexHandle, strFileName));
	m_TextureHashTable.insert(std::make_pair(strFileName, std::move(pTexHandle)));

	return m_TextureHashTable.find(strFileName)->second.get();
}

TEXTURE_HANDLE* TextureManager::CreateDynamicTexture_ITL(UINT _TexWidth, UINT _TexHeight)
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();
	TEXTURE_HANDLE* pTexHandle = nullptr;

	D3D12Resource_ptr pTexResource = nullptr;
	D3D12Resource_ptr pUploadBuffer = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	DXGI_FORMAT TexFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

	if (FAILED(m_pResourceManager->CreateTexturePair(&pTexResource, &pUploadBuffer, _TexWidth, _TexHeight, TexFormat))) {
		__debugbreak();
		return nullptr;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = TexFormat;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	if (pSingleDescriptorAllocator->AllocDescriptorHandle(&srv))
	{
		pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

		pTexHandle = AllocTextureHandle_ITL();
		pTexHandle->pTexResource = pTexResource;
		pTexHandle->pUploadBuffer = pUploadBuffer;
		pTexHandle->srvCpuHandle = srv;
	}
	else
	{
		pTexResource->Release();
		pTexResource = nullptr;

		pUploadBuffer->Release();
		pUploadBuffer = nullptr;
	}


	return pTexHandle;
}

TEXTURE_HANDLE* TextureManager::CreateImmutableTexture_ITL(UINT _TexWidth, UINT _TexHeight, DXGI_FORMAT _format, const BYTE* _pInitImage)
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();
	TEXTURE_HANDLE* pTexHandle = nullptr;

	D3D12Resource_ptr pTexResource = nullptr;
	D3D12_CPU_DESCRIPTOR_HANDLE srv = {};

	if (FAILED(m_pResourceManager->CreateTexture(&pTexResource, _TexWidth, _TexHeight, _format, _pInitImage)))
	{
		__debugbreak();
		return nullptr;
	}

	D3D12_SHADER_RESOURCE_VIEW_DESC SRVDesc = {};
	SRVDesc.Format = _format;
	SRVDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	SRVDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = 1;

	if (pSingleDescriptorAllocator->AllocDescriptorHandle(&srv))
	{
		pD3DDevice->CreateShaderResourceView(pTexResource.Get(), &SRVDesc, srv);

		pTexHandle = AllocTextureHandle_ITL();
		pTexHandle->pTexResource = pTexResource;
		pTexHandle->srvCpuHandle = srv;
	}
	else
	{
		pTexResource->Release();
		pTexResource = nullptr;
	}

	return pTexHandle;
}

void TextureManager::DeleteTexture_ITL(TEXTURE_HANDLE* _pTexHandle)
{
	D3D12Device_raw pD3DDevice = m_pRenderer->INL_GetD3DDevice();
	SingleDescriptorAllocator* pSingleDescriptorAllocator = m_pRenderer->INL_GetSingleDescriptorAllocator();

	if (_pTexHandle->ulRefCount <= 0) {
		__debugbreak();
		return;
	}

	ULONG ref_Count = --_pTexHandle->ulRefCount;
	if (ref_Count <= 0) {
		std::map<TEXTURE_HANDLE*, std::wstring>::iterator iter =  m_TextureReverseHashTable.find(_pTexHandle);
		if(iter != m_TextureReverseHashTable.end()) {
			std::wstring strFileName = iter->second;
			m_TextureReverseHashTable.erase(iter);
			std::map<std::wstring, std::unique_ptr<TEXTURE_HANDLE>>::iterator iter2 = m_TextureHashTable.find(strFileName);
			if(iter2 != m_TextureHashTable.end()) {
				m_TextureHashTable.erase(iter2);
			}
		}
	}
} 

TEXTURE_HANDLE* TextureManager::AllocTextureHandle_ITL()
{
	std::unique_ptr<TEXTURE_HANDLE> pTexHandle = std::make_unique<TEXTURE_HANDLE>();
	//memset(pTexHandle.get(), 0, sizeof(TEXTURE_HANDLE));

	pTexHandle->ulRefCount = 1;
	m_TextureHashSet.insert(std::make_pair(pTexHandle.get(), std::move(pTexHandle)));

	return pTexHandle.get();
}

void TextureManager::CleanUpTextureManager()
{
	m_TextureHashTable.clear();
	m_TextureReverseHashTable.clear();
	m_TextureHashSet.clear();
}
