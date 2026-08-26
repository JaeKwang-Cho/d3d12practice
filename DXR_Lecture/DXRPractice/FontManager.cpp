#include "pch.h"
#include "FontManager.h"
#include "D3D12Renderer.h"

bool FontManager::Initialize(D3D12Renderer* _pRenderer, D3D12CommandQueue_raw _pCommandQueue, UINT _Width, UINT _Height, bool _bEnableDebugLayer)
{
    D3D12Device_raw pD3D12Device = _pRenderer->INL_GetD3DDevice();
    CreateD2D(pD3D12Device, _pCommandQueue, _bEnableDebugLayer);

    // GDI와 달리, 모니터 DPI를 얻어서 virtual pixel을 이용해서 부드럽게 폰트 렌더링을 해준다.
    float fDPI = _pRenderer->INL_GetDPI();
	CreateDWrite(pD3D12Device, _Width, _Height, fDPI);

    return true;
}

FONT_HANDLE* FontManager::CreateFontObject_ITL(const WCHAR* _wchFontFamilyName, float _fFondSize)
{
	Microsoft::WRL::ComPtr<IDWriteTextFormat3> pTextFormat = nullptr;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> pTextFormatBase = nullptr;
	// factory가 있어야 font에 해당하는 IDWriteTextFormat을 만들 수 있다.
	Microsoft::WRL::ComPtr<IDWriteFactory8> pDWFactory = m_pDWFactory;
	Microsoft::WRL::ComPtr<IDWriteFontCollection3> pFontCollection = nullptr;

	// DIP(Device Independent Pixel) 단위로 폰트 크기를 계산한다. 96 DPI에서 1 DIP는 1 픽셀과 같다.

    HRESULT hr = S_OK;
    if (pDWFactory) {
        hr = pDWFactory->CreateTextFormat(
            _wchFontFamilyName,
			pFontCollection.Get(), // font 집합을 따로 넣어서 할 수 있지만, 여기서는 null로 해서 시스템 폰트 컬렉션에서 찾도록 한다.
            DWRITE_FONT_WEIGHT_REGULAR,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            _fFondSize,
            L"ko-kr",
            pTextFormatBase.GetAddressOf()
        );
        if (FAILED(hr)) {
            __debugbreak();
            return nullptr;
		}

        hr = pTextFormatBase.As(&pTextFormat);
        if (FAILED(hr)) {
            __debugbreak();
            return nullptr;
        }
    }

	FONT_HANDLE* pFontHandle = new FONT_HANDLE;
	wcsncpy_s(pFontHandle->wchFontFamilyName, _wchFontFamilyName, sizeof(pFontHandle->wchFontFamilyName) / sizeof(WCHAR));
	pFontHandle->fFontSize = _fFondSize;

    if (pTextFormat) {
		hr = pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        if (FAILED(hr)) {
            __debugbreak();
            return nullptr;
		}
        hr = pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        if (FAILED(hr)) {
            __debugbreak();
            return nullptr;
        }
    }

	pFontHandle->pTextFormat = pTextFormat.Detach();

	return pFontHandle;
}

void FontManager::DeleteFontObject_ITL(FONT_HANDLE* _pFontHandle)
{
	delete _pFontHandle;
}

bool FontManager::WriteTextToBitmap_ITL(BYTE* _pDestImage, UINT _DestWidth, UINT _DestHeight, UINT _DestPitch, int* _piOutWidth, int* _piOutHeight, FONT_HANDLE* _pFontHandle, const WCHAR* _wchString, DWORD _dwLen)
{
    int iTextWidth = 0;
    int iTextHeight = 0;

	bool bResult = CreateBitmapFromText(&iTextWidth, &iTextHeight, _pFontHandle->pTextFormat, _wchString, _dwLen);
    if (bResult) {
        //clamp
        if(iTextHeight > static_cast<int>(_DestHeight)) {
            iTextHeight = _DestHeight;
		}
        if (iTextWidth > static_cast<int>(_DestWidth)) {
            iTextWidth = _DestWidth;
        }

        D2D1_MAPPED_RECT mappedRect = {};
        if (FAILED(m_pD2DTargetBitmapReadable->Map(D2D1_MAP_OPTIONS_READ, &mappedRect))) {
            __debugbreak();
			return false;
        }

        BYTE* pDest = _pDestImage;
		char* pSrc = reinterpret_cast<char*>(mappedRect.bits);
        // Dynamic Texture에다가 복사 해준다.
        for(DWORD y = 0; y < static_cast<DWORD>(iTextHeight); ++y) {
			memcpy(pDest, pSrc, iTextWidth * 4);
            pDest += _DestPitch;
			pSrc += mappedRect.pitch;
		}
        if (FAILED(m_pD2DTargetBitmapReadable->Unmap())) {
			__debugbreak();
			return false;
        }
    }

    *_piOutWidth = iTextWidth;
    *_piOutHeight = iTextHeight;

    return true;
}

bool FontManager::CreateD2D(D3D12Device_raw _pD3D12Device, D3D12CommandQueue_raw _pCommandQueue, bool _bEnableDebugLayer)
{
    // ID3D11On12Device을 이용해서 D3D11을 D3D12에서만든다.
	Microsoft::WRL::ComPtr<ID3D11On12Device> pD3D11On12Device = nullptr;
	// D3D12는 D2D가 없다. 그래서 D3D11을 이용해서 D2D를 만들기 위해서 D3D11Device과 D3D11DeviceContext가 필요하다.
	Microsoft::WRL::ComPtr<ID3D11Device> pD3D11Device = nullptr;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> pD3D11DeviceContext = nullptr;
	Microsoft::WRL::ComPtr<ID2D1Factory8> pD2DFactory = nullptr;

    // Direct3D 리소스와의 Direct2D 상호 운용성에 필요한 플래그를 켠다.
	UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
	D2D1_FACTORY_OPTIONS factoryOptions = {};

    if (_bEnableDebugLayer) {
        d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    }
	factoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
	// D3D11On12CreateDevice 함수를 이용해서 D3D11On12Device, D3D11Device, D3D11DeviceContext를 만든다.
    HRESULT hr = D3D11On12CreateDevice(
        _pD3D12Device,
        d3d11DeviceFlags,
        nullptr, 0,
        reinterpret_cast<IUnknown**>(&_pCommandQueue), 1,
        0,
        &pD3D11Device,
        &pD3D11DeviceContext,
        nullptr
	);

    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }

	// pD3D11On12Device으로 pD3D11Device을 이용할 수 있게 한다.
	hr = pD3D11Device->QueryInterface(IID_PPV_ARGS(&pD3D11On12Device));
    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}

	// D2D1CreateFactory 함수를 이용해서 D2D1Factory를 만든다.
    D2D1_DEVICE_CONTEXT_OPTIONS deviceOptions = D2D1_DEVICE_CONTEXT_OPTIONS_NONE;
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory8), &factoryOptions, static_cast<void**>(&pD2DFactory));
    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}

    // pDXGIDevice으로 pD3D11On12Device을 사용할 수 있게 한다.
    Microsoft::WRL::ComPtr<IDXGIDevice4> pDXGIDevice = nullptr;
    hr = pD3D11On12Device->QueryInterface(IID_PPV_ARGS(&pDXGIDevice));
    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }
	// 이제야 D2D1Factory를 이용해서 D2DDevice를 만들 수 있다. D2DDevice는 D3D11On12Device의 DXGI Device 인터페이스를 이용해서 만든다.
	hr = pD2DFactory->CreateDevice(pDXGIDevice.Get(), &m_pD2DDevice);
    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }
	// 단계가 많은데, 어쩔 수 없다. D2DDevice를 이용해서 D2DDeviceContext를 만든다.
	hr = m_pD2DDevice->CreateDeviceContext(deviceOptions, &m_pD2DDeviceContext);
    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}

    return true;
}

bool FontManager::CreateDWrite(D3D12Device_raw _pD3D12Device, UINT _TexWidth, UINT _TexHeight, float _fDPI)
{
    bool bResult = false;

	m_D2DBitmapWidth = _TexWidth;
    m_D2DBitmapHeight = _TexHeight;

	D2D1_SIZE_U size = { m_D2DBitmapWidth, m_D2DBitmapHeight };

	// 일종의 D2D용 Render Target이 될 Bitmap을 만든다.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = {};
    {
        bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
        bitmapProperties.pixelFormat = D2D1::PixelFormat(DXGI_FORMAT_R8G8B8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED);
		bitmapProperties.dpiX = bitmapProperties.dpiY = _fDPI;
    }

	HRESULT hr = m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, bitmapProperties, m_pD2DTargetBitmap.GetAddressOf());
    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }

	// CPU에서 읽을 수 있는 Bitmap을 만든다. 이 Bitmap은 나중에 텍스트가 그려진 Bitmap에서 텍스트 영역만큼 복사해서 CPU에서 읽어서 최종적으로 D3D12 Texture로 복사할 때 사용한다.
	bitmapProperties.bitmapOptions = D2D1_BITMAP_OPTIONS_CPU_READ | D2D1_BITMAP_OPTIONS_CANNOT_DRAW;
    hr = m_pD2DDeviceContext->CreateBitmap(size, nullptr, 0, bitmapProperties, m_pD2DTargetBitmapReadable.GetAddressOf());
    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}

	hr = m_pD2DDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_pWhiteBrush);
    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }
    // DWrite 객체를 생성할 때 필요하다. 그냥 표준이다.
    hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory8), &m_pDWFactory);
    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}

	bResult = true;
	return bResult;
}

bool FontManager::CreateBitmapFromText(int* _piOutWidth, int* _piOutHeight, IDWriteTextFormat* _pTextFormat, const WCHAR* _wchString, DWORD _dwLen)
{
	bool bResult = false;
	HRESULT hr = S_OK;

    D2D1DeviceContext_raw pD2DDeviceContext = m_pD2DDeviceContext.Get();
	DWriteFactory_raw pDWFactory = m_pDWFactory.Get();

    D2D1_SIZE_F max_size = pD2DDeviceContext->GetSize();
    max_size.width = static_cast<float>(m_D2DBitmapWidth);
	max_size.height = static_cast<float>(m_D2DBitmapHeight);

	// IDwriteTextFormat 을 이용해서 IDWriteTextLayout을 만든다. 렌더링에 바로 써먹을 수 있는 객체이다.
    // 일단은 스마트 포인터로 바디를 나가면 없어지게 했는데,
	// 나중에 캐시기능을 넣어서, 같은 폰트 설정이면 IDWriteTextLayout을 재사용할 수 있게 바꿔보자.
	Microsoft::WRL::ComPtr<IDWriteTextLayout> pTextLayout = nullptr;
    if (pDWFactory && _pTextFormat) {
        HRESULT hr = pDWFactory->CreateTextLayout(
            _wchString,
            _dwLen,
            _pTextFormat,
            max_size.width,
            max_size.height,
            pTextLayout.GetAddressOf()
        );
        if (FAILED(hr)) {
            __debugbreak();
            return false;
		}
    }
	// 텍스트 레이아웃에서 텍스트의 실제 크기를 얻는다. 이 크기를 이용해서 나중에 텍스트가 그려진 Bitmap에서 텍스트 영역만큼 복사해서 CPU에서 읽어서 최종적으로 D3D12 Texture로 복사할 때 사용한다.
	// 실제로 사용할 수 있게 고도화 시키기 쉽지 않다. 글자마다 자간이 다 다르기에, 한 줄에 몇 글자가 들어갈 지 구하는게 쉽지 않다.
    // 지금은 기본 값으로 한다.
	DWRITE_TEXT_METRICS1 metrics = {};
    if(!pTextLayout){
        __debugbreak();
        return false;
	}

    pTextLayout->GetMetrics(&metrics);
    // Target 설정
    pD2DDeviceContext->SetTarget(m_pD2DTargetBitmap.Get());
    pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);

	// 나중에 d3d12에서 그리도록 바꾸자. 지금은 d3d9 스타일로 그냥 그려보자.
	pD2DDeviceContext->BeginDraw();
    pD2DDeviceContext->Clear(D2D1::ColorF(D2D1::ColorF::Black));
	pD2DDeviceContext->SetTransform(D2D1::Matrix3x2F::Identity());
	// 이제 brush를 적용시키면서, TextLayout을 이용해서 텍스트를 그린다.
    pD2DDeviceContext->DrawTextLayout(
        D2D1::Point2F(0, 0),
        pTextLayout.Get(),
        m_pWhiteBrush.Get()
    );

    hr = pD2DDeviceContext->EndDraw();

    if (FAILED(hr)) {
        __debugbreak();
        return false;
	}
    // 다시 복구 시키고
    pD2DDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_DEFAULT);
    pD2DDeviceContext->SetTarget(nullptr);

	int width = static_cast<int>(metrics.width);
    int height = static_cast<int>(metrics.height);

	D2D1_POINT_2U destPoint = { 0, 0 };
	D2D1_RECT_U srcRect = { 0, 0, static_cast<UINT>(width), static_cast<UINT>(height) };

    // 시스템 메모리로 가져온다.
	hr = m_pD2DTargetBitmapReadable->CopyFromBitmap(&destPoint, m_pD2DTargetBitmap.Get(), &srcRect);
    if (FAILED(hr)) {
        __debugbreak();
        return false;
    }
    // Dynamic Texture에 복사하기 위해 텍스쳐 정보도 가져온다.
    *_piOutWidth = width;
    *_piOutHeight = height;

	bResult = true;
	return bResult;
}

void FontManager::CleanupDWrite()
{
}

void FontManager::CleanupD2D()
{		
}

void FontManager::Cleanup()
{
    CleanupDWrite();
    CleanupD2D();
}

FontManager::FontManager():
    m_pRenderer(nullptr),
    m_pD2DDevice(nullptr),
    m_pD2DDeviceContext(nullptr),
    m_pD2DTargetBitmap(nullptr),
    m_pD2DTargetBitmapReadable(nullptr),
    m_pFontCollection(nullptr),
    m_pWhiteBrush(nullptr),
    m_pDWFactory(nullptr),
    m_pLineMetrics(nullptr),
    m_dwMaxLineMetricsNum(0),
    m_D2DBitmapWidth(0),
	m_D2DBitmapHeight(0)
{
}

FontManager::~FontManager()
{
    Cleanup();
}
