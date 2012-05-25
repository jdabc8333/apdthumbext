// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Original Sample(RecipeThumbnailProvider): Copyright (c) Microsoft Corporation. All rights reserved
// Apd Shell Extention: Copyright (c) teraapi.com

#include <shlwapi.h>
#include <thumbcache.h> // For IThumbnailProvider.
#include <new>
#include <math.h>
#include "..\\..\\zlib-1.2.7\\zlib.h"

#pragma comment(lib, "shlwapi.lib")

//offset:0
typedef struct _AZMAGICHEADER
{
	unsigned char magic[7];
	unsigned char version;
} AZMAGICHEADER;

//offset:8+14
typedef struct _AZP0HEADER
{
	unsigned short tmbw; //thumb width
	unsigned short tmbh; //thumb height
	unsigned int tmbsize;
} AZP0HEADER;

//offset:8
typedef struct _AZDW0HEADER
{
	unsigned short w;
	unsigned short h;
	unsigned short layerAmount;
	unsigned short layerNumCnt;
	unsigned short layerCurrentNum;
	unsigned char layerCompressionType;
	unsigned int tmbsize;
} AZDW0HEADER;

//offset:8
typedef struct _AZDW1HEADER
{
	unsigned short tmbw;
	unsigned short tmbh;
	unsigned int tmbsize;
} AZDW1HEADER;

enum AZFILETYPE { AZP0 , AZDW0 , AZDW1, Failed };

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
	HRESULT inf(void *dest, unsigned int destsize, unsigned int srcsize);

    long _cRef;
    IStream *_pStream;     // provided during initialization.
	AZMAGICHEADER magic;
	AZP0HEADER azp0;
	AZDW0HEADER azdw0;
	AZDW1HEADER azdw1;
	enum AZFILETYPE filetype;
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
//	OutputDebugStringA("Initalize\n");
	filetype = Failed;
    if (_pStream == NULL)
    {
        // take a reference to the stream if we have not been inited yet

		ULONG ulReaded=0;
		LARGE_INTEGER li;
		if(SUCCEEDED(pStream->QueryInterface(&_pStream)) && SUCCEEDED(_pStream->Read(reinterpret_cast<void*>(&magic), sizeof magic, &ulReaded)) && ulReaded==sizeof magic)
		{
			if(strncmp(reinterpret_cast<const char*>(magic.magic), "AZPDATA", 7)==0 && magic.version==0)
			{
				li.QuadPart = 8+14;
				if(SUCCEEDED(_pStream->Seek(li, STREAM_SEEK_SET, NULL)) && SUCCEEDED(_pStream->Read(reinterpret_cast<void*>(&azp0), sizeof azp0, &ulReaded)) && ulReaded==sizeof azp0 && azp0.tmbw<3000 && azp0.tmbw>0 && azp0.tmbh<3000 && azp0.tmbh>0 && azp0.tmbsize == azp0.tmbw * azp0.tmbh * 4)
				{
					filetype = AZP0;
//	OutputDebugStringA("AZP0init\n");
					return S_OK;
				}
			} else if(strncmp(reinterpret_cast<const char*>(magic.magic), "AZDWDAT", 7)==0)
			{
		//		li.QuadPart = 8;
		//		if(FAILED(_pStream->Seek(li, STREAM_SEEK_SET, NULL)))return E_FAIL;
				switch(magic.version)
				{
				case 0:
					if(SUCCEEDED(_pStream->Read(reinterpret_cast<void*>(&azdw0), sizeof azdw0, &ulReaded)) && ulReaded==sizeof azdw0 && azdw0.w<30000 && azdw0.w>0 && azdw0.h<30000 && azdw0.h>0 && azdw0.tmbsize < azdw0.w * azdw0.h * 4+500)
					{
						filetype = AZDW0;
//	OutputDebugStringA("AZDW0init\n");
						return S_OK;
					}
					break;
				case 1:
					if(SUCCEEDED(_pStream->Read(reinterpret_cast<void*>(&azdw1), sizeof azdw1, &ulReaded)) && ulReaded==sizeof azdw1 && azdw1.tmbw<3000 && azdw1.tmbw>0 && azdw1.tmbh<3000 && azdw1.tmbh>0 && azdw1.tmbsize < azdw1.tmbw * azdw1.tmbh * 4+500)
					{
						filetype = AZDW1;
//	OutputDebugStringA("AZDW1init\n");
						return S_OK;
					}
					break;
				}
			}
		}
    }
	return E_UNEXPECTED;
}

void CalcResizedSize(unsigned int *retw, unsigned int *reth, unsigned int maxwh)
{
	unsigned int srcw = *retw;
	unsigned int srch = *reth;
	unsigned int dstw = maxwh;
	unsigned int dsth = maxwh;

	if(maxwh/*maxw*/ < srcw || maxwh < srch)
	{
		double ratio = ((double)srcw) / ((double)srch);
		if(((double)maxwh) / ((double)maxwh) > ratio)
		{
			*retw = static_cast<unsigned int>(floor(((double)maxwh) * ratio));
			*reth = maxwh;
		} else {
			*retw = maxwh;
			*reth = static_cast<unsigned int>(floor(((double)maxwh) * ratio));
		}
	} else {
		*retw = srcw;
		*reth = srch;
	}
}
HBITMAP CreateMyDIB(unsigned int w, unsigned int h, void ** ppvBits)
{
	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = w;
	bmi.bmiHeader.biHeight = h;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	return CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
}



#define CHUNK 16384

HRESULT CApdThumbProvider::inf(void *dest, unsigned int destsize, unsigned int srcsize)
{
	z_stream strm={0};
	void *in = malloc(CHUNK);
	ULONG ulReaded;
	unsigned int remain = srcsize;

    if(inflateInit(&strm)!=Z_OK)return E_FAIL;

	strm.avail_out = destsize;
	strm.next_out = reinterpret_cast<Bytef*>(dest);

	int ret;

    do {
   //     strm.avail_in = fread(in, 1, CHUNK, source);
		if(FAILED(_pStream->Read(in, (remain<CHUNK)?remain:CHUNK, &ulReaded)))
		{
            (void)inflateEnd(&strm);
			free(in);
			return E_FAIL;
		}
        strm.avail_in = ulReaded;
		remain -= ulReaded;
        if (strm.avail_in == 0)
            break;
        strm.next_in = reinterpret_cast<Bytef*>(in);

        /* run inflate() on input until output buffer not full */
        do {
			ret = inflate(&strm, Z_NO_FLUSH);
			if(ret == Z_STREAM_END)
				break;
			if(ret!=Z_OK)
			{
                (void)inflateEnd(&strm);
				free(in);
                return E_FAIL;
            }
        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (1);

    /* clean up and return */
    (void)inflateEnd(&strm);

	free(in);

	return (strm.total_out==destsize)?S_OK:E_FAIL;
}
//#include <stdio.h>
// IThumbnailProvider
IFACEMETHODIMP CApdThumbProvider::GetThumbnail(UINT cx, HBITMAP *phbmp, WTS_ALPHATYPE *pdwAlpha)
{
	unsigned int dstw, dsth, srcw, srch;
	void *srcImage;
//	OutputDebugStringA("GetThumbnail\n");
	switch(filetype)
	{
	case AZP0:
		dstw = srcw = azp0.tmbw;
		dsth = srch = azp0.tmbh;
		srcImage = malloc(azp0.tmbsize);
		break;
	case AZDW0:
		dstw = srcw = azdw0.w;
		dsth = srch = azdw0.h;
		srcImage = malloc(srcw*srch*4);
		break;
	case AZDW1:
		dstw = srcw = azdw1.tmbw;
		dsth = srch = azdw1.tmbh;
		srcImage = malloc(srcw*srch*4);
		break;
	default:
		return E_UNEXPECTED;
	}
	CalcResizedSize(&dstw, &dsth, cx);


	void *pBits;
	HBITMAP hbmp = CreateMyDIB(dstw, dsth, &pBits);
	if(hbmp==NULL)
	{
		free(srcImage);
		return E_UNEXPECTED;
	}

	ULONG ulReaded;
	
	switch(filetype)
	{
	case AZP0:
		if(FAILED(_pStream->Read(srcImage, azp0.tmbsize, &ulReaded)) || ulReaded!=azp0.tmbsize)
		{
			free(srcImage);
			DeleteObject(hbmp);
			return E_UNEXPECTED;
		}
//	OutputDebugStringA("AZP0inf\n");
		break;
	case AZDW0:
		if(FAILED(inf(srcImage, srcw*srch*4, azdw0.tmbsize)))
		{
			free(srcImage);
			DeleteObject(hbmp);
			return E_UNEXPECTED;
		}
//	OutputDebugStringA("AZDW0inf\n");
		break;
	case AZDW1:
		if(FAILED(inf(srcImage, srcw*srch*4, azdw1.tmbsize)))
		{
			free(srcImage);
			DeleteObject(hbmp);
			return E_UNEXPECTED;
		}
//	OutputDebugStringA("AZDW1inf\n");
		break;
	default:
		DeleteObject(hbmp);
		free(srcImage);
		return E_UNEXPECTED;
	}
	
	BITMAPINFO bmi = {0};
	bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth = srcw;
	bmi.bmiHeader.biHeight = srch;
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

/*	char d[150];
	sprintf(d, "srcw = %u, srch = %u, dstw = %u, dsth = %u, cx=%u\n", srcw, srch, dstw, dsth,cx);
	OutputDebugStringA(d);*/

	HDC hDC = CreateCompatibleDC(NULL);
	SelectObject(hDC, hbmp);
	StretchDIBits(hDC, 0, 0, dstw, dsth, 0, 0, srcw, srch, srcImage, &bmi, DIB_RGB_COLORS, SRCCOPY);
	hbmp=(HBITMAP)GetCurrentObject(hDC, OBJ_BITMAP);
	DeleteDC(hDC);
	free(srcImage);

	*phbmp = hbmp;
	*pdwAlpha = WTSAT_ARGB;
//	OutputDebugStringA("OKexit\n");
	return S_OK;
}
