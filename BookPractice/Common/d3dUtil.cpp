//***************************************************************************************
// d3dUtil.cpp by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************


#include "d3dUtil.h"
#include <comdef.h>
#include <fstream>

using Microsoft::WRL::ComPtr;

DxException::DxException(HRESULT hr, const std::wstring& functionName, const std::wstring& filename, int lineNumber) :
    ErrorCode(hr),
    FunctionName(functionName),
    Filename(filename),
    LineNumber(lineNumber)
{
}

bool d3dUtil::IsKeyDown(int vkeyCode)
{
    return (GetAsyncKeyState(vkeyCode) & 0x8000) != 0;
}

ComPtr<ID3DBlob> d3dUtil::LoadBinary(const std::wstring& filename)
{
    std::ifstream fin(filename, std::ios::binary);

    fin.seekg(0, std::ios_base::end);
    std::ifstream::pos_type size = (int)fin.tellg();
    fin.seekg(0, std::ios_base::beg);

    ComPtr<ID3DBlob> blob;
    ThrowIfFailed(D3DCreateBlob(size, blob.GetAddressOf()));

    fin.read((char*)blob->GetBufferPointer(), size);
    fin.close();

    return blob;
}

// ������ �ʴ� ���� ���� Default Buffer�� ����� ���ؼ���,
// Default Heap�� �����͸� �־���ϴµ�
// CPU�� �ٲ� �� ���� Default Heap�� �����͸� ���� �� ������
// CPU������ �۾��� �� �ִ�, Upload �� buffer�� heap�� ���� �����͸� �־��ش�����
// GPU���� �Ѱ��־�� �Ѵ�.

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Default ���� Buffer�� byteSize ��ŭ �������ش�..
    CD3DX12_HEAP_PROPERTIES cHeapDefaultProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC  bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &cHeapDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // CPU �޸� �����͸� default buffer�� �־��ֱ� ���ؼ�, 
    // �߰� �ܰ��� Upload ���� ���� ���ش�.
    CD3DX12_HEAP_PROPERTIES cHeapUploadPros(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(device->CreateCommittedResource(
        &cHeapUploadPros,
		D3D12_HEAP_FLAG_NONE,
        &bufferDesc, // ũ��� ����.
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    // Default buffer�� �ְ� ���� ������ �Ӽ��� �ۼ��Ѵ�.

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // (default buffer resource�� �����͸� ī���ϴ� ��)
    // ����� �Լ� UpdateSubresources�� CPU �޸𸮸� upload heap �� ������ �Ѵ�.
    D3D12_RESOURCE_BARRIER UploadBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COMMON,
        D3D12_RESOURCE_STATE_COPY_DEST
    );
	cmdList->ResourceBarrier(
        1, 
        &UploadBarrierTransition
    );

    // (d3dx12.h)
    // �׸��� CopySubresourceRegion�� �̿��ؼ� Default Buffer�� upload buffer�� ���� ���簡 �ȴ�.
    // ���������� ID3D12Device::GetCopyableFootPrints�� ȣ���� �ȴ�.

    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    D3D12_RESOURCE_BARRIER BufferBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, 
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    
	cmdList->ResourceBarrier(
        1, 
        &BufferBarrierTransition);

    // Note: 'uploadBuffer' �� Command List�� ���� ���縦 ���� �ʾ��� �� �ֱ� ������
    //  �Լ� ȣ�� ���� ��� �־�� �Ѵ�. �׷��� ȣ���ڴ�'uploadBuffer'�� copy�� �Ϸ� �Ǿ��� ���� �����ؾ� �Ѵ�.

    return defaultBuffer;
}

ComPtr<ID3DBlob> d3dUtil::CompileShader(
	const std::wstring& filename,
	const D3D_SHADER_MACRO* defines,
	const std::string& entrypoint,
	const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	HRESULT hr = S_OK;

	ComPtr<ID3DBlob> byteCode = nullptr;
	ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(), target.c_str(), compileFlags, 0, &byteCode, &errors);

	if(errors != nullptr)
		OutputDebugStringA((char*)errors->GetBufferPointer());

	ThrowIfFailed(hr);

	return byteCode;
}

std::wstring DxException::ToString()const
{
    // Get the string description of the error code.
    _com_error err(ErrorCode);
    std::wstring msg = err.ErrorMessage();

    return FunctionName + L" failed in " + Filename + L"; line " + std::to_wstring(LineNumber) + L"; error: " + msg;
}


