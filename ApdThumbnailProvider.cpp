// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved
// Copyright (c) teraapi.com

#include <shlwapi.h>
//#include <Wincrypt.h>   // For CryptStringToBinary.
#include <thumbcache.h> // For IThumbnailProvider.
#include <wincodec.h>   // Windows Imaging Codecs
//#include <msxml6.h>
#include <new>
#include <math.h>

#pragma comment(lib, "shlwapi.lib")
#pragma comment(lib, "windowscodecs.lib")
//#pragma comment(lib, "Crypt32.lib")
//#pragma comment(lib, "msxml6.lib")

// this thumbnail provider implements IInitializeWithStream to enable being hosted
// in an isolated process for robustness

typedef struct _thumbheader
{
	unsigned short w;
	unsigned short h;
	unsigned int thumbsize;
} thumbheader;

class CApdThumbProvider : public IInitializeWithStream,
                             public IThumbnailProvider
{
public:
    CApdThumbProvider() : _cRef(1), _pStream(NULL)
    {
    }

    virtual ~CApdThumbProvider()
    {
        if (_pStream)
        {
            _pStream->Release();
        }
    }

    // IUnknown
    IFACEMETHODIMP QueryInterface(REFIID riid, void **ppv)
    {
        static const QITAB qit[] =
        {
            QITABENT(CApdThumbProvider, IInitializeWithStream),
            QITABENT(CApdThumbProvider, IThumbnailProvider),
            { 0 },
        };
        return QISearch(this, qit, riid, ppv);
    }

    IFACEMETHODIMP_(ULONG) AddRef()
    {
        return InterlockedIncrement(&_cRef);
    }

    IFACEMETHODIMP_(ULONG) Release()
    {
        ULONG cRef = InterlockedDecrement(&_cRef);
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    // IInitializeWithStream
    IFACEMETHODIMP Initialize(IStream *pStream, DWORD grfMode);

    // IThumbnailProvider
    IFACEMETHODIMP GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha);

private:

    long _cRef;
    IStream *_pStream;     // provided during initialization.
	thumbheader tmb;
};

HRESULT CApdThumbProvider_CreateInstance(REFIID riid, void **ppv)
{
    CApdThumbProvider *pNew = new (std::nothrow) CApdThumbProvider();
    HRESULT hr = pNew ? S_OK : E_OUTOFMEMORY;
    if (SUCCEEDED(hr))
    {
        hr = pNew->QueryInterface(riid, ppv);
        pNew->Release();
    }
    return hr;
}

// IInitializeWithStream
IFACEMETHODIMP CApdThumbProvider::Initialize(IStream *pStream, DWORD)
{
    if (_pStream == NULL)
    {
        // take a reference to the stream if we have not been inited yet

		if(SUCCEEDED(pStream->QueryInterface(&_pStream)))
		{
			unsigned char buf[8];
			ULONG ulReaded=0;
			LARGE_INTEGER li;
			li.QuadPart = 8+14;
			if(SUCCEEDED(_pStream->Read(buf, 8, &ulReaded)) && ulReaded==8 && strncmp(reinterpret_cast<const char*>(buf), "AZPDATA", 7)==0 && buf[7]==0 && SUCCEEDED(_pStream->Seek(li, STREAM_SEEK_SET, NULL)) && SUCCEEDED(_pStream->Read(reinterpret_cast<void*>(&tmb), 8, &ulReaded)) && ulReaded==8 && tmb.w<3000 && tmb.w>0 && tmb.h<3000 && tmb.h>0 && tmb.thumbsize == tmb.w * tmb.h * 4)
			{
//	OutputDebugStringA("OKAPDinit\n");
				return S_OK;
			}
		}
    }
	return E_UNEXPECTED;
}
#include <stdio.h>
// IThumbnailProvider
IFACEMETHODIMP CApdThumbProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	unsigned int dstw = cx;
	unsigned int dsth = cx;

	if(dstw/*maxw*/ < tmb.w || dsth < tmb.h)
	{
		double ratio = ((double)tmb.w) / ((double)tmb.h);
		if(((double)dstw) / ((double)dsth) > ratio)
		{
			dstw = static_cast<unsigned int>(floor(((double)dsth) * ratio));
		} else {
			dsth = static_cast<unsigned int>(floor(((double)dstw) * ratio));
		}
	} else {
		dstw = tmb.w;
		dsth = tmb.h;
	}

/*	char d[150];
	sprintf(d, "w = %u, h = %u, thumbsize = %u, dstw = %u, dsth = %u, cx = %u\n", tmb.w, tmb.h, tmb.thumbsize, dstw, dsth, cx);
	OutputDebugStringA(d);*/



	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = dstw;
	bmi.bmiHeader.biHeight = dsth;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	void *pBits;
	HBITMAP hbmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
	if(hbmp==NULL)return E_UNEXPECTED;

	ULONG ulReaded;
	if(FAILED(_pStream->Read(pBits, tmb.thumbsize, &ulReaded)) || ulReaded!=tmb.thumbsize)
	{
		DeleteObject(hbmp);
		return E_UNEXPECTED;
	}

	*phbmp = hbmp;
	*pdwAlpha = WTSAT_ARGB;
//	OutputDebugStringA("OKAPDthmb\n");
	return S_OK;
}
