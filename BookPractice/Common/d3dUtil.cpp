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

// 변하지 않는 값을 가진 Default Buffer를 만들기 위해서는,
// Default Heap에 데이터를 넣어야하는데
// CPU가 바꿀 수 없는 Default Heap에 데이터를 넣을 수 없으니
// CPU에서도 작업할 수 있는, Upload 용 buffer와 heap을 만들어서 데이터를 넣어준다음에
// GPU에서 넘겨주어야 한다.

Microsoft::WRL::ComPtr<ID3D12Resource> d3dUtil::CreateDefaultBuffer(
    ID3D12Device* device,
    ID3D12GraphicsCommandList* cmdList,
    const void* initData,
    UINT64 byteSize,
    Microsoft::WRL::ComPtr<ID3D12Resource>& uploadBuffer)
{
    ComPtr<ID3D12Resource> defaultBuffer;

    // Default 값의 Buffer를 byteSize 만큼 생성해준다..
    CD3DX12_HEAP_PROPERTIES cHeapDefaultProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC  bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);

    ThrowIfFailed(device->CreateCommittedResource(
        &cHeapDefaultProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
		D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(defaultBuffer.GetAddressOf())));

    // CPU 메모리 데이터를 default buffer에 넣어주기 위해서, 
    // 중간 단계의 Upload 힙을 만들 어준다.
    CD3DX12_HEAP_PROPERTIES cHeapUploadPros(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(device->CreateCommittedResource(
        &cHeapUploadPros,
		D3D12_HEAP_FLAG_NONE,
        &bufferDesc, // 크기는 같다.
		D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadBuffer.GetAddressOf())));

    // Default buffer에 넣고 싶은 데이터 속성을 작성한다.

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = initData;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    // (default buffer resource에 데이터를 카피하는 법)
    // 도우미 함수 UpdateSubresources가 CPU 메모리를 upload heap 에 저장을 한다.
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
    // 그리고 CopySubresourceRegion을 이용해서 Default Buffer에 upload buffer의 값이 복사가 된다.
    // 내부적으로 ID3D12Device::GetCopyableFootPrints가 호출이 된다.

    UpdateSubresources<1>(cmdList, defaultBuffer.Get(), uploadBuffer.Get(), 0, 0, 1, &subResourceData);

    D3D12_RESOURCE_BARRIER BufferBarrierTransition = CD3DX12_RESOURCE_BARRIER::Transition(
        defaultBuffer.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, 
        D3D12_RESOURCE_STATE_GENERIC_READ
    );
    
	cmdList->ResourceBarrier(
        1, 
        &BufferBarrierTransition);

    // Note: 'uploadBuffer' 는 Command List가 실제 복사를 하지 않았을 수 있기 때문에
    //  함수 호출 내내 살아 있어야 한다. 그래서 호출자는'uploadBuffer'를 copy가 완료 되었을 때만 삭제해야 한다.

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


