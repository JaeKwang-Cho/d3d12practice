#pragma once

typedef Microsoft::WRL::ComPtr<ID3D12Device14> D3D12Device_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12CommandQueue1> D3D12CommandQueue_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12CommandAllocator> D3D12CommandAllocator_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList10> D3D12GraphicsCommandList_ptr;
typedef Microsoft::WRL::ComPtr<IDXGISwapChain4> DXGISwapChain_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12Resource> D3D12Resource_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> D3D12DescriptorHeap_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12PipelineState> D3D12PipelineState_ptr;
typedef Microsoft::WRL::ComPtr<ID3D12Fence1> D3D12Fence_ptr;

using D3D12Device_raw = ID3D12Device14*;
using D3D12CommandQueue_raw = ID3D12CommandQueue1*;
using D3D12CommandAllocator_raw = ID3D12CommandAllocator*;
using D3D12GraphicsCommandList_raw = ID3D12GraphicsCommandList10*;
using DXGISwapChain_raw = IDXGISwapChain4*;
using D3D12Resource_raw = ID3D12Resource*;
using D3D12DescriptorHeap_raw = ID3D12DescriptorHeap*;
using D3D12PipelineState_raw = ID3D12PipelineState*;
using D3D12Fence_raw = ID3D12Fence1*;