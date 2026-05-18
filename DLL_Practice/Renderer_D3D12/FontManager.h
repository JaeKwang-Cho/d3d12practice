#pragma once


typedef Microsoft::WRL::ComPtr<ID2D1Device7> D2D1Device_ptr;
typedef Microsoft::WRL::ComPtr<ID2D1DeviceContext7> D2D1DeviceContext_ptr;
typedef Microsoft::WRL::ComPtr<ID2D1Bitmap1> D2D1Bitmap_ptr;
typedef Microsoft::WRL::ComPtr<IDWriteFontCollection3> DWriteFontCollection_ptr;
typedef Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> D2D1SolidColorBrush_ptr;
typedef Microsoft::WRL::ComPtr<IDWriteFactory8> DWriteFactory_ptr;

using D2D1Device_raw = ID2D1Device7*;
using D2D1DeviceContext_raw = ID2D1DeviceContext7*;
using D2D1Bitmap_raw = ID2D1Bitmap1*;
using DWriteFontCollection_raw = IDWriteFontCollection3*;
using D2D1SolidColorBrush_raw = ID2D1SolidColorBrush*;
using DWriteFactory_raw = IDWriteFactory8*;

class D3D12Renderer;

class FontManager
{
public:
	bool Initialize(D3D12Renderer* _pRenderer, D3D12CommandQueue_raw _pCommandQueue, UINT _Width, UINT _Height, bool _bEnableDebugLayer);
	FONT_HANDLE* CreateFontObject_ITL(const WCHAR* _wchFontFamilyName, float _fFondSize);
	void DeleteFontObject_ITL(FONT_HANDLE* _pFontHandle);

	bool WriteTextToBitmap_ITL(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, FONT_HANDLE* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen);

private:
	bool CreateD2D(D3D12Device_raw _pD3D12Device, D3D12CommandQueue_raw _pCommandQueue, bool _bEnableDebugLayer);
	bool CreateDWrite(D3D12Device_raw _pD3D12Device, UINT _TexWidth, UINT _TexHeight, float _fDPI);
	bool CreateBitmapFromText(int* _piOutWidth, int* _piOutHeight, IDWriteTextFormat* _pTextFormat, const WCHAR* _wchString, DWORD _dwLen);
	
	void CleanupDWrite();
	void CleanupD2D();
	void Cleanup();

private:
	D3D12Renderer* m_pRenderer;

	// 폰트 렌더링 용 D2D
	D2D1Device_ptr m_pD2DDevice;
	D2D1DeviceContext_ptr m_pD2DDeviceContext;
	
	// 폰트 렌더링 용 DWrite 그리고 폰트 컬렉션
	D2D1Bitmap_ptr m_pD2DTargetBitmap;
	D2D1Bitmap_ptr m_pD2DTargetBitmapReadable; // Readback용으로 CPU에서 읽을 수 있는 Bitmap
	DWriteFontCollection_ptr m_pFontCollection; // Font를 생성하고, 렌더링을 하기위한 Device 객체
	D2D1SolidColorBrush_ptr m_pWhiteBrush; // 폰트 렌더링을 위한 SolidColorBrush 객체. 일단은 흰색으로 고정한다.

	
	DWriteFactory_ptr m_pDWFactory;
	std::unique_ptr<DWRITE_LINE_METRICS1> m_pLineMetrics ;
	DWORD m_dwMaxLineMetricsNum = 0;
	UINT m_D2DBitmapWidth;
	UINT m_D2DBitmapHeight;

public:
	FontManager();
	virtual ~FontManager();
};

