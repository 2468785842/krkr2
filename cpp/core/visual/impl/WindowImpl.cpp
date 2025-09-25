//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// "Window" TJS Class implementation
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

// #define DIRECTDRAW_VERSION 0x0300
// #include <ddraw.h>
// #include <d3d9.h>

#include <algorithm>
#include "MsgIntf.h"
#include "WindowIntf.h"
#include "LayerIntf.h"
// #include "WindowFormUnit.h"
#include "SysInitIntf.h"
#include "tjsHashSearch.h"
#include "StorageIntf.h"
#include "VideoOvlIntf.h"
#include "DebugIntf.h"
#include "PluginImpl.h"
#include "LayerManager.h"
#include "EventImpl.h"

#include "Application.h"
#include "TVPScreen.h"
#include "tjsDictionary.h"
#include "TVPWindow.h"
// #include "VSyncTimingThread.h"
// #include "MouseCursor.h"

iWindowLayer *TVPCreateAndAddWindow(tTJSNI_Window *w);
#define MK_SHIFT 4
#define MK_CONTROL 8
#define MK_ALT (0x20)
tjs_uint32 TVP_TShiftState_To_uint32(tjs_uint32 state) {
    tjs_uint32 result = 0;
    if(state & MK_SHIFT) {
        result |= ssShift;
    }
    if(state & MK_CONTROL) {
        result |= ssCtrl;
    }
    if(state & MK_ALT) {
        result |= ssAlt;
    }
    return result;
}
tjs_uint32 TVP_TShiftState_From_uint32(tjs_uint32 state) {
    tjs_uint32 result = 0;
    if(state & ssShift) {
        result |= MK_SHIFT;
    }
    if(state & ssCtrl) {
        result |= MK_CONTROL;
    }
    if(state & ssAlt) {
        result |= MK_ALT;
    }
    return result;
}

//---------------------------------------------------------------------------
// Mouse Cursor management
//---------------------------------------------------------------------------
static tTJSHashTable<ttstr, tjs_int> TVPCursorTable;
tjs_int TVPGetCursor(const ttstr &name) {
    // get placed path
    ttstr place(TVPSearchPlacedPath(name));

    // search in cache
    tjs_int *in_hash = TVPCursorTable.Find(place);
    if(in_hash)
        return *in_hash;
#if 0
	// not found
	tTVPLocalTempStorageHolder file(place);

	HCURSOR handle = ::LoadCursorFromFile(file.GetLocalName().c_str());

	if(!handle) TVPThrowExceptionMessage(TVPCannotLoadCursor, place);

	int id = MouseCursor::AddCursor( handle );

	TVPCursorTable.Add(place, id);

	return id;
#endif
    return 0;
}
//---------------------------------------------------------------------------

#if 0
//---------------------------------------------------------------------------
// Direct3D/Full Screen and priamary surface management
//---------------------------------------------------------------------------

//! @brief		Display resolution mode for full screen
enum tTVPFullScreenResolutionMode
{
	fsrAuto, //!< auto negotiation
	fsrProportional, //!< let screen resolution fitting neaest to the preferred resolution,
								//!< preserving the original aspect ratio
	fsrNearest, //!< let screen resolution fitting neaest to the preferred resolution.
				//!< There is no guarantee that the aspect ratio is preserved
	fsrNoChange,//!< no change resolution
	fsrExpandWindow	//!< expand window
};
static IDirect3D9 *TVPDirect3D=nullptr;

static IDirect3D9* (WINAPI * TVPDirect3DCreate)( UINT SDKVersion ) = nullptr;

static HMODULE TVPDirect3DDLLHandle=nullptr;
static bool TVPUseChangeDisplaySettings = false;
static tTVPScreenMode TVPDefaultScreenMode;

static bool TVPInFullScreen = false;
static HWND TVPFullScreenWindow = nullptr;
tTVPScreenModeCandidate TVPFullScreenMode;

static tjs_int TVPPreferredFullScreenBPP = 0;
static tTVPFullScreenResolutionMode TVPPreferredFullScreenResolutionMode = fsrNoChange;
enum tTVPFullScreenUsingEngineZoomMode
{
	fszmNone, //!< no zoom by the engine
	fszmInner, //!< inner fit on the monitor (uncovered areas may be filled with black)
	fszmOuter //!< outer fit on the monitor (primary layer may jut out of the monitor)
};
static tTVPFullScreenUsingEngineZoomMode TVPPreferredFullScreenUsingEngineZoomMode = fszmInner;


//---------------------------------------------------------------------------
// Color Format Detection
//---------------------------------------------------------------------------
tjs_int TVPDisplayColorFormat = 0;
static tjs_int TVPGetDisplayColorFormat()
{
	// detect current 16bpp display color format
	// return value:
	// 555 : 16bit 555 mode
	// 565 : 16bit 565 mode
	// 0   : other modes

	if( TVPDirect3D ) {
		// まずは Direct3D を用いて 16bit color format 取得を試みる
		D3DDISPLAYMODE mode = {0};
		if( SUCCEEDED( TVPDirect3D->GetAdapterDisplayMode( D3DADAPTER_DEFAULT, &mode ) ) ) {
			if( mode.Format == D3DFMT_R5G6B5 ) {
				TVPDisplayColorFormat = 565;
				return 565;
			} else if( mode.Format == D3DFMT_X1R5G5B5 ) {
				TVPDisplayColorFormat = 555;
				return 555;
			} else {
				TVPDisplayColorFormat = 0;
				return 0;
			}
		}
	}
	// create temporary bitmap and device contexts
	HDC desktopdc = ::GetDC(0);
	HDC bitmapdc = ::CreateCompatibleDC(desktopdc);
	HBITMAP bmp = ::CreateCompatibleBitmap(desktopdc, 1, 1);
	HBITMAP oldbmp = ::SelectObject(bitmapdc, bmp);

	int count;
	int r, g, b;
	COLORREF lastcolor;

	// red
	count = 0;
	lastcolor = 0xffffff;
	for(int i = 0; i < 256; i++)
	{
		::SetPixel(bitmapdc, 0, 0, RGB(i, 0, 0));
		COLORREF rgb = ::GetPixel(bitmapdc, 0, 0);
		if(rgb != lastcolor) count ++;
		lastcolor = rgb;
	}
	r = count;

	// green
	count = 0;
	lastcolor = 0xffffff;
	for(int i = 0; i < 256; i++)
	{
		::SetPixel(bitmapdc, 0, 0, RGB(0, i, 0));
		COLORREF rgb = ::GetPixel(bitmapdc, 0, 0);
		if(rgb != lastcolor) count ++;
		lastcolor = rgb;
	}
	g = count;

	// blue
	count = 0;
	lastcolor = 0xffffff;
	for(int i = 0; i < 256; i++)
	{
		::SetPixel(bitmapdc, 0, 0, RGB(0, 0, i));
		COLORREF rgb = ::GetPixel(bitmapdc, 0, 0);
		if(rgb != lastcolor) count ++;
		lastcolor = rgb;
	}
	b = count;

	// free bitmap and device contexts
	::SelectObject(bitmapdc, oldbmp);
	::DeleteObject(bmp);
	::DeleteDC(bitmapdc);
	::ReleaseDC(0, desktopdc);

	// determine type
	if(r == 32 && g == 64 && b == 32)
	{
		TVPDisplayColorFormat = 565;
		return 565;
	}
	else if(r == 32 && g == 32 && b == 32)
	{
		TVPDisplayColorFormat = 555;
		return 555;
	}
	else
	{
		TVPDisplayColorFormat = 0;
		return 0;
	}
}
//---------------------------------------------------------------------------
static void TVPInitFullScreenOptions()
{
	tTJSVariant val;

	if(TVPGetCommandLine(TJS_W("-fsbpp"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("16"))
			TVPPreferredFullScreenBPP = 16;
		else if(str == TJS_W("24"))
			TVPPreferredFullScreenBPP = 24;
		else if(str == TJS_W("32"))
			TVPPreferredFullScreenBPP = 32;
		else
			TVPPreferredFullScreenBPP = 0; // means nochange
	}

	if(TVPGetCommandLine(TJS_W("-fsres"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("auto"))
			TVPPreferredFullScreenResolutionMode = fsrAuto;
		else if(str == TJS_W("prop") || str == TJS_W("proportional"))
			TVPPreferredFullScreenResolutionMode = fsrProportional;
		else if(str == TJS_W("nearest"))
			TVPPreferredFullScreenResolutionMode = fsrNearest;
		else if(str == TJS_W("nochange"))
			TVPPreferredFullScreenResolutionMode = fsrNoChange;
		else if(str == TJS_W("expandwindow"))
			TVPPreferredFullScreenResolutionMode = fsrExpandWindow;
	}

	if(TVPGetCommandLine(TJS_W("-fszoom"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("yes") || str == TJS_W("inner"))
			TVPPreferredFullScreenUsingEngineZoomMode = fszmInner;
		if(str == TJS_W("outer"))
			TVPPreferredFullScreenUsingEngineZoomMode = fszmOuter;
		else if(str == TJS_W("no"))
			TVPPreferredFullScreenUsingEngineZoomMode = fszmNone;
	}

	if(TVPGetCommandLine(TJS_W("-fsmethod"), &val) )
	{
		ttstr str(val);
		if(str == TJS_W("cds"))
			TVPUseChangeDisplaySettings = true;
		else
			TVPUseChangeDisplaySettings = false;
	}
}
//---------------------------------------------------------------------------
void TVPDumpDirect3DDriverInformation()
{
	if(TVPDirect3D)
	{
		IDirect3D9 *d3d9 = TVPDirect3D;
		static bool dumped = false;
		if(dumped) return;
		dumped = true;

		TVPAddImportantLog( (const tjs_char*)TVPInfoFoundDirect3DInterface );

		try
		{
			// dump direct3d information
			UINT numofadapter = d3d9->GetAdapterCount();
			ttstr infostart(TJS_W("(info)  "));
			ttstr log;
			log = infostart + TJS_W("Found ") + ttstr((tjs_int)numofadapter) + TJS_W(" Devices.");
			TVPAddImportantLog(log);

			for( UINT adapter = 0; adapter < numofadapter; adapter++ )
			{
				log = infostart + TJS_W("Device Number : ") + ttstr((tjs_int)adapter);
				TVPAddImportantLog(log);

				D3DADAPTER_IDENTIFIER9 D3DID = {0};
				if(SUCCEEDED(d3d9->GetAdapterIdentifier( adapter, 0, &D3DID)))
				{
					// driver string
					log = infostart + ttstr(D3DID.Description) + TJS_W(" [") + ttstr(D3DID.Driver) + TJS_W("]");
					TVPAddImportantLog(log);
					log = infostart + TJS_W(" [") + ttstr(D3DID.DeviceName) + TJS_W("]");
					TVPAddImportantLog(log);

					// driver version(reported)
					log = infostart + TJS_W("Driver version (reported) : ");
					wchar_t tmp[256];
					TJS_snprintf( tmp, 256, L"%d.%02d.%02d.%04d ",
							  HIWORD( D3DID.DriverVersion.HighPart ),
							  LOWORD( D3DID.DriverVersion.HighPart ),
							  HIWORD( D3DID.DriverVersion.LowPart  ),
							  LOWORD( D3DID.DriverVersion.LowPart  ) );
					log += tmp;
					TVPAddImportantLog(log);

					// driver version(actual)
					std::wstring driverName = ttstr(D3DID.Driver).AsStdString();
					wchar_t driverpath[1024];
					wchar_t *driverpath_filename = nullptr;
					bool success = 0!=SearchPath(nullptr, driverName.c_str(), nullptr, 1023, driverpath, &driverpath_filename);

					if(!success)
					{
						wchar_t syspath[1024];
						GetSystemDirectory(syspath, 1023);
						TJS_strcat(syspath, L"\\drivers"); // SystemDir\drivers
						success = 0!=SearchPath(syspath, driverName.c_str(), nullptr, 1023, driverpath, &driverpath_filename);
					}

					if(!success)
					{
						wchar_t syspath[1024];
						GetWindowsDirectory(syspath, 1023);
						TJS_strcat(syspath, L"\\system32"); // WinDir\system32
						success = 0!=SearchPath(syspath, driverName.c_str(), nullptr, 1023, driverpath, &driverpath_filename);
					}

					if(!success)
					{
						wchar_t syspath[1024];
						GetWindowsDirectory(syspath, 1023);
						TJS_strcat(syspath, L"\\system32\\drivers"); // WinDir\system32\drivers
						success = 0!=SearchPath(syspath, driverName.c_str(), nullptr, 1023, driverpath, &driverpath_filename);
					}

					if(success)
					{
						log = infostart + TJS_W("Driver version (") + ttstr(driverpath) + TJS_W(") : ");
						tjs_int major, minor, release, build;
						if(TVPGetFileVersionOf(driverpath, major, minor, release, build))
						{
							TJS_snprintf(tmp, 256, L"%d.%d.%d.%d", (int)major, (int)minor, (int)release, (int)build);
							log += tmp;
						}
						else
						{
							log += TJS_W("unknown");
						}
					}
					else
					{
						log = infostart + TJS_W("Driver ") + ttstr(D3DID.Driver) +
							TJS_W(" is not found in search path.");
					}
					TVPAddImportantLog(log);

					// device id
					TJS_snprintf(tmp, 256, L"VendorId:%08X  DeviceId:%08X  SubSysId:%08X  Revision:%08X",
						D3DID.VendorId, D3DID.DeviceId, D3DID.SubSysId, D3DID.Revision);
					log = infostart + TJS_W("Device ids : ") + tmp;
					TVPAddImportantLog(log);

					// Device GUID
					GUID *pguid = &D3DID.DeviceIdentifier;
					TJS_snprintf( tmp, 256, L"%08X-%04X-%04X-%02X%02X%02X%02X%02X%02X%02X%02X",
							  pguid->Data1,
							  pguid->Data2,
							  pguid->Data3,
							  pguid->Data4[0], pguid->Data4[1], pguid->Data4[2], pguid->Data4[3],
							  pguid->Data4[4], pguid->Data4[5], pguid->Data4[6], pguid->Data4[7] );
					log = infostart + TJS_W("Unique driver/device id : ") + tmp;
					TVPAddImportantLog(log);

					// WHQL level
					TJS_snprintf(tmp, 256, L"%08x", D3DID.WHQLLevel);
					log = infostart + TJS_W("WHQL level : ")  + tmp;
					TVPAddImportantLog(log);
				} else {
					TVPAddImportantLog( (const tjs_char*)TVPInfoFaild );
				}
			}
		}
		catch(...)
		{
		}
	}

}
//---------------------------------------------------------------------------
static void TVPUnloadDirect3D();
static void TVPInitDirect3D()
{
	if(!TVPDirect3DDLLHandle)
	{
		// load d3d9.dll
		TVPAddLog( (const tjs_char*)TVPInfoDirect3D );
		TVPDirect3DDLLHandle = ::LoadLibrary( L"d3d9.dll" );
		if(!TVPDirect3DDLLHandle)
			TVPThrowExceptionMessage(TVPCannotInitDirect3D, (const tjs_char*)TVPCannotLoadD3DDLL );
	}

	if(!TVPDirect3D)
	{
		try
		{
			// get Direct3DCreaet function
			TVPDirect3DCreate = (IDirect3D9*(WINAPI * )(UINT))GetProcAddress(TVPDirect3DDLLHandle, "Direct3DCreate9");
			if(!TVPDirect3DCreate)
				TVPThrowExceptionMessage(TVPCannotInitDirect3D, (const tjs_char*)TVPNotFoundDirect3DCreate );

			TVPDirect3D = TVPDirect3DCreate( D3D_SDK_VERSION );
			if( nullptr == TVPDirect3D )
				TVPThrowExceptionMessage( TVPFaildToCreateDirect3D );
		}
		catch(...)
		{
			TVPUnloadDirect3D();
			throw;
		}
	}

	TVPGetDisplayColorFormat();
}
//---------------------------------------------------------------------------
static void TVPUninitDirect3D()
{
	// release Direct3D object ( DLL will not be released )
}
//---------------------------------------------------------------------------
static void TVPUnloadDirect3D()
{
	// release Direct3D object and /*release it's DLL */
	TVPUninitDirect3D();
	if(TVPDirect3D) TVPDirect3D->Release(), TVPDirect3D = nullptr;

	TVPGetDisplayColorFormat();
}
//---------------------------------------------------------------------------
void TVPEnsureDirect3DObject()
{
	try
	{
		TVPInitDirect3D();
	}
	catch(...)
	{
	}
}
//---------------------------------------------------------------------------
IDirect3D9 * TVPGetDirect3DObjectNoAddRef()
{
	// retrieves IDirect3D9 interface
	return TVPDirect3D;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static tTVPAtExit
	TVPUnloadDirect3DAtExit(TVP_ATEXIT_PRI_RELEASE, TVPUnloadDirect3D);
//---------------------------------------------------------------------------



//---------------------------------------------------------------------------
//! @brief		Get tTVPFullScreenResolutionMode enumeration string
static ttstr TVPGetGetFullScreenResolutionModeString(tTVPFullScreenResolutionMode mode)
{
	switch(mode)
	{
	case fsrAuto:				return TJS_W("fsrAuto");
	case fsrProportional:		return TJS_W("fsrProportional");
	case fsrNearest:			return TJS_W("fsrNearest");
	case fsrNoChange:			return TJS_W("fsrNoChange");
	case fsrExpandWindow:		return TJS_W("fsrExpandWindow");
	}
	return ttstr();
}
//---------------------------------------------------------------------------
//! @brief	do reduction for numer over denom
static void TVPDoReductionNumerAndDenom(tjs_int &n, tjs_int &d)
{
	tjs_int a = n;
	tjs_int b = d;
	while(b)
	{
		tjs_int t = b;
		b = a % b;
		a = t;
	}
	n = n / a;
	d = d / a;
}
//---------------------------------------------------------------------------
static void TVPGetOriginalScreenMetrics()
{
	// retrieve original (un-fullscreened) information
	TVPDefaultScreenMode.Width = tTVPScreen::GetWidth();
	TVPDefaultScreenMode.Height = tTVPScreen::GetHeight();
	HDC dc = GetDC(0);
	TVPDefaultScreenMode.BitsPerPixel = GetDeviceCaps(dc, BITSPIXEL);
	ReleaseDC(0, dc);
}
//---------------------------------------------------------------------------
//! @brief	enumerate all display modes
void TVPEnumerateAllDisplayModes(std::vector<tTVPScreenMode> & modes)
{
	modes.clear();

	if(!TVPUseChangeDisplaySettings)
	{
		// if DisplaySettings APIs is not preferred
		// use Direct3D
		TVPEnsureDirect3DObject();
		IDirect3D9* d3d = TVPGetDirect3DObjectNoAddRef();
		if(d3d)
		{
			static const D3DFORMAT PixelFormatTypes[] = {
				//D3DFMT_A1R5G5B5, // not support display
				//D3DFMT_A2R10G10B10, // not support display
				//D3DFMT_A8R8G8B8, // not support display
				D3DFMT_R5G6B5,
				// D3DFMT_X1R5G5B5, // IDirect3D9::EnumAdapterModes では D3DFMT_R5G6B5 と同等と処理される
				D3DFMT_X8R8G8B8
			};
			static const int NumOfFormat = sizeof(PixelFormatTypes) / sizeof(PixelFormatTypes[0]);
			for( int f = 0; f < NumOfFormat; f++ )
			{
				D3DFORMAT format = PixelFormatTypes[f];
				UINT count = d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT,format);
				for( UINT a = 0; a < count; a++ )
				{
					D3DDISPLAYMODE mode;
					HRESULT hr = d3d->EnumAdapterModes( D3DADAPTER_DEFAULT, format, a, &mode );
					if( SUCCEEDED( hr ) )
					{
						tTVPScreenMode sm;
						sm.Width =  mode.Width;
						sm.Height = mode.Height;
						// modes.Refreshrate
						if( mode.Format == D3DFMT_R5G6B5 || mode.Format == D3DFMT_X1R5G5B5 ) {
							sm.BitsPerPixel = 16;
							modes.push_back(sm);
						} else if( mode.Format == D3DFMT_X8R8G8B8 ) {
							sm.BitsPerPixel = 32;
							modes.push_back(sm);
						} else {
							// unknown ここでは無視
						}
					}
				}
			}
		}
	}

	if(modes.size() == 0)
	{
		// try another API to retrieve screen sizes
		TVPUseChangeDisplaySettings = true;

		// attempt to use EnumDisplaySettings
		DWORD num = 0;
		do
		{
			DEVMODE dm;
			ZeroMemory(&dm, sizeof(DEVMODE));
			dm.dmSize = sizeof(DEVMODE);
			dm.dmDriverExtra = 0;
			if(::EnumDisplaySettings(nullptr, num, reinterpret_cast<DEVMODE*>(&dm)) == 0) break;
			tTVPScreenMode mode;
			mode.Width  = dm.dmPelsWidth;
			mode.Height = dm.dmPelsHeight;
			mode.BitsPerPixel = dm.dmBitsPerPel;
			modes.push_back(mode);
			num ++;
		} while(true);
	}

	TVPAddLog( TVPFormatMessage(TVPInfoEnvironmentUsing,
		(TVPUseChangeDisplaySettings?TJS_W("ChangeDisplaySettings API"):TJS_W("Direct3D"))) );
}
//---------------------------------------------------------------------------
//! @brief		make full screen mode candidates
//! @note		Always call this function *before* entering full screen mode
static void TVPMakeFullScreenModeCandidates(
	const tTVPScreenMode & preferred,
	tTVPFullScreenResolutionMode mode,
	tTVPFullScreenUsingEngineZoomMode zoom_mode,
	std::vector<tTVPScreenModeCandidate> & candidates)
{
	// adjust give parameter
	if(mode == fsrAuto && zoom_mode == fszmNone) zoom_mode = fszmInner;
		// fszmInner is ignored (as always be fszmInner) if mode == fsrAuto && zoom_mode == fszmNone

	// print debug information
	TVPAddLog((const tjs_char*)TVPInfoSearchBestFullscreenResolution);
	TVPAddLog(TVPFormatMessage(TVPInfoConditionPreferredScreenMode, preferred.Dump()));
	TVPAddLog(TVPFormatMessage(TVPInfoConditionMode, TVPGetGetFullScreenResolutionModeString(mode)));
	TVPAddLog(TVPFormatMessage(TVPInfoConditionZoomMode, ttstr(
		zoom_mode == fszmInner ? TJS_W("inner") :
		zoom_mode == fszmOuter ? TJS_W("outer") :
			TJS_W("none"))));

	// get original screen metrics
	TVPGetOriginalScreenMetrics();

	// decide preferred bpp
	tjs_int preferred_bpp = preferred.BitsPerPixel == 0 ?
			TVPDefaultScreenMode.BitsPerPixel : preferred.BitsPerPixel;

	// get original screen aspect ratio
	tjs_int screen_aspect_numer = TVPDefaultScreenMode.Width;
	tjs_int screen_aspect_denom = TVPDefaultScreenMode.Height;
	TVPDoReductionNumerAndDenom(screen_aspect_numer, screen_aspect_denom); // do reduction
	TVPAddLog(TVPFormatMessage(TVPInfoEnvironmentDefaultScreenMode, TVPDefaultScreenMode.Dump()));
	TVPAddLog(TVPFormatMessage(TVPInfoEnvironmentDefaultScreenAspectRatio,
		ttstr(screen_aspect_numer),ttstr(screen_aspect_denom)) );

	// clear destination array
	candidates.clear();

	// enumerate all display modes
	std::vector<tTVPScreenMode> modes;
	TVPEnumerateAllDisplayModes(modes);
	std::sort(modes.begin(), modes.end()); // sort by area, and bpp
	{	// 重複する項目を削除する(リフレッシュレートで重複する可能性がある)
		std::vector<tTVPScreenMode>::iterator new_end = std::unique(modes.begin(),modes.end());
		modes.erase(new_end, modes.end());
	}

	{
		tjs_int last_width = -1, last_height = -1;
		ttstr last_line;
		TVPAddLog( (const tjs_char*)TVPInfoEnvironmentAvailableDisplayModes );
		for(std::vector<tTVPScreenMode>::iterator i = modes.begin(); i != modes.end(); i++)
		{
			if(last_width != i->Width || last_height != i->Height)
			{
				if(!last_line.IsEmpty()) TVPAddLog(last_line);
				tjs_int w = i->Width, h = i->Height;
				TVPDoReductionNumerAndDenom(w, h);
				last_line = TJS_W("(info)  ") + i->DumpHeightAndWidth() +
					TJS_W(", AspectRatio=") + ttstr(w) + TJS_W(":") + ttstr(h) +
					TJS_W(", BitsPerPixel=") + ttstr(i->BitsPerPixel);
			}
			else
			{
				last_line += TJS_W("/") + ttstr(i->BitsPerPixel);
			}
			last_width = i->Width; last_height = i->Height;
		}
		if(!last_line.IsEmpty()) TVPAddLog(last_line);
	}

	if(mode != fsrNoChange && mode != fsrExpandWindow )
	{

		if(mode != fsrNearest)
		{
			// for fstAuto and fsrProportional, we need to see screen aspect ratio

			// reject screen mode which does not match the original screen aspect ratio
			for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
				i != modes.end(); /**/)
			{
				tjs_int aspect_numer = i->Width;
				tjs_int aspect_denom = i->Height;
				TVPDoReductionNumerAndDenom(aspect_numer, aspect_denom);
				if(aspect_numer != screen_aspect_numer || aspect_denom != screen_aspect_denom)
					i = modes.erase(i);
				else
					i++;
			}
		}

		if(zoom_mode == fszmNone)
		{
			// we cannot use resolution less than preferred resotution when
			// we do not use zooming, so reject them.
			for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
				i != modes.end(); /**/)
			{
				if(i->Width < preferred.Width || i->Height < preferred.Height)
					i = modes.erase(i);
				else
					i++;
			}
		}
	}
	else
	{
		// reject resolutions other than the original size
		for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
			i != modes.end(); /**/)
		{
			if(	i->Width  != TVPDefaultScreenMode.Width ||
				i->Height != TVPDefaultScreenMode.Height)
				i = modes.erase(i);
			else
				i++;
		}
	}

	// reject resolutions larger than the default screen mode
	for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
		i != modes.end(); /**/)
	{
		if(i->Width > TVPDefaultScreenMode.Width || i->Height > TVPDefaultScreenMode.Height)
			i = modes.erase(i);
		else
			i++;
	}

	// reject resolutions less than 16
	for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
		i != modes.end(); /**/)
	{
		if(i->BitsPerPixel < 16)
			i = modes.erase(i);
		else
			i++;
	}

	// check there is at least one candidate mode
	if(modes.size() == 0)
	{
		// panic! no candidates
		// this could be if the driver does not provide the screen
		// mode which is the same size as the default screen...
		// push the default screen mode
		TVPAddImportantLog( (const tjs_char*)TVPInfoNotFoundScreenModeFromDriver );
		tTVPScreenMode mode;
		mode.Width  = TVPDefaultScreenMode.Width;
		mode.Height = TVPDefaultScreenMode.Height;
		mode.BitsPerPixel = TVPDefaultScreenMode.BitsPerPixel;
		modes.push_back(mode);
	}

	// copy modes to candidation, with making zoom ratio and resolution rank
	for(std::vector<tTVPScreenMode>::iterator i = modes.begin();
		i != modes.end(); i++)
	{
		tTVPScreenModeCandidate candidate;
		candidate.Width = i->Width;
		candidate.Height = i->Height;
		candidate.BitsPerPixel = i->BitsPerPixel;
		if(zoom_mode != fszmNone)
		{
			double width_r  = (double)candidate.Width /  (double)preferred.Width;
			double height_r = (double)candidate.Height / (double)preferred.Height;

			// select width or height, to fit to target screen from preferred size
			if(zoom_mode == fszmInner ? (width_r < height_r) : (width_r > height_r))
			{
				candidate.ZoomNumer = candidate.Width;
				candidate.ZoomDenom = preferred.Width;
			}
			else
			{
				candidate.ZoomNumer = candidate.Height;
				candidate.ZoomDenom = preferred.Height;
			}

			// if the zooming range is between 1.00 and 1.034 we treat this as 1.00
			double zoom_r = (double)candidate.ZoomNumer / (double)candidate.ZoomDenom;
			if(zoom_r > 1.000 && zoom_r < 1.034)
				candidate.ZoomDenom = candidate.ZoomNumer = 1;
		}
		else
		{
			// zooming disabled
			candidate.ZoomDenom = candidate.ZoomNumer = 1;
		}
		TVPDoReductionNumerAndDenom(candidate.ZoomNumer, candidate.ZoomDenom);

		// make rank on each candidate

		// BPP
		// take absolute difference of preferred and candidate.
		// lesser bpp has less priority, so add 1000 to lesser bpp.
		candidate.RankBPP = std::abs(preferred_bpp - candidate.BitsPerPixel);
		if(candidate.BitsPerPixel < preferred_bpp) candidate.RankBPP += 1000;

		// Zoom-in
		// we usually use zoom-in, zooming out (this situation will occur if
		// the screen resolution is lesser than expected) has lesser priority.
		if(candidate.ZoomNumer < candidate.ZoomDenom)
			candidate.RankZoomIn = 1;
		else
			candidate.RankZoomIn = 0;

		// Zoom-Beauty
		if(mode == fsrAuto)
		{
			// 0: no zooming is the best.
			// 1: zooming using monitor's function is fastest and most preferable.
			// 2: zooming using kirikiri's zooming functions is somewhat slower but not so bad.
			// 3: zooming using monitor's function and kirikiri's function tends to be dirty
			//   because the zooming is applied twice. this is not preferable.
			tjs_int zoom_rank = 0;
			if(candidate.Width != TVPDefaultScreenMode.Width ||
				candidate.Height != TVPDefaultScreenMode.Height)
				zoom_rank += 1; // zoom by monitor

			if(candidate.ZoomNumer != 1 || candidate.ZoomDenom != 1)
				zoom_rank += 2; // zoom by the engine

			candidate.RankZoomBeauty = zoom_rank;
		}
		else
		{
			// Zoom-Beauty is not considered
			candidate.RankZoomBeauty = 0;
		}

		// Size
		// size rank is a absolute difference between area size of candidate and preferred.
		candidate.RankSize = std::abs(
			candidate.Width * candidate.Height - preferred.Width * preferred.Height);

		// push candidate into candidates array
		candidates.push_back(candidate);
	}

	// sort candidate by its rank
	std::sort(candidates.begin(), candidates.end());

	// dump all candidates to log
	TVPAddLog( (const tjs_char*)TVPInfoResultCandidates );
	for(std::vector<tTVPScreenModeCandidate>::iterator i = candidates.begin();
		i != candidates.end(); i++)
	{
		TVPAddLog(TJS_W("(info)  ") + i->Dump());
	}
}
//---------------------------------------------------------------------------
#if 0
tjs_uint TVPGetMonitorNumber( HWND window )
{
	if( TVPDirect3D == nullptr ) return D3DADAPTER_DEFAULT;
	HMONITOR windowMonitor = ::MonitorFromWindow( window, MONITOR_DEFAULTTOPRIMARY );
	UINT iCurrentMonitor = 0;
	UINT numOfMonitor = TVPDirect3D->GetAdapterCount();
	for( ; TVPDirect3D < numOfMonitor; ++iCurrentMonitor ) 	{
		if( IDirect3D9->GetAdapterMonitor(iCurrentMonitor) == windowMonitor )
			break;
	}
	if( iCurrentMonitor == numOfMonitor )
		iCurrentMonitor = D3DADAPTER_DEFAULT;
	return iCurrentMonitor;
}
#endif
//---------------------------------------------------------------------------
void TVPSwitchToFullScreen(HWND window, tjs_int w, tjs_int h, iTVPDrawDevice* drawdevice )
{
	if(TVPInFullScreen) return;

	TVPInitFullScreenOptions();

	//TVPReleaseVSyncTimingThread();

	if(!TVPUseChangeDisplaySettings)
	{
		try
		{
			TVPInitDirect3D();
		}
		catch(eTJS &e)
		{
			TVPAddLog(e.getMessage());
			TVPUseChangeDisplaySettings = true;
		}
		catch(...)
		{
			TVPUseChangeDisplaySettings = true;
		}
	}

	// get fullscreen mode candidates
	std::vector<tTVPScreenModeCandidate> candidates;
	tTVPScreenMode preferred;
	preferred.Width = w;
	preferred.Height = h;
	preferred.BitsPerPixel = TVPPreferredFullScreenBPP;
	TVPMakeFullScreenModeCandidates(
		preferred,
		TVPPreferredFullScreenResolutionMode,
		TVPPreferredFullScreenUsingEngineZoomMode,
		candidates);

	// try changing display mode
	bool success = false;
	for(std::vector<tTVPScreenModeCandidate>::iterator i = candidates.begin();
		i != candidates.end(); i++)
	{
		TVPAddLog( TVPFormatMessage(TVPInfoTryScreenMode, i->Dump()) );
		success = drawdevice->SwitchToFullScreen( window, i->Width, i->Height, i->BitsPerPixel, TVPDisplayColorFormat, TVPPreferredFullScreenResolutionMode != fsrExpandWindow );
		if( success )
		{
			TVPFullScreenMode = *i;
			break;
		}
	}

	if(!success)
	{
		TVPThrowExceptionMessage(TVPCannotSwitchToFullScreen, (const tjs_char*)TVPAllScreenModeError );
	}

	TVPAddLog( (const tjs_char*)TVPInfoChangeScreenModeSuccess );

	TVPInFullScreen = true;

	TVPGetDisplayColorFormat();
	//TVPEnsureVSyncTimingThread();
}
//---------------------------------------------------------------------------
void TVPRecalcFullScreen( tjs_int w, tjs_int h )
{
	if( TVPInFullScreen != true ) return;

	TVPInitFullScreenOptions();

	// get fullscreen mode candidates
	std::vector<tTVPScreenModeCandidate> candidates;
	tTVPScreenMode preferred;
	preferred.Width = w;
	preferred.Height = h;
	preferred.BitsPerPixel = TVPPreferredFullScreenBPP;
	TVPMakeFullScreenModeCandidates(
		preferred,
		TVPPreferredFullScreenResolutionMode,
		TVPPreferredFullScreenUsingEngineZoomMode,
		candidates);

	TVPFullScreenMode = *(candidates.begin());
	TVPGetDisplayColorFormat();
}
//---------------------------------------------------------------------------
void TVPRevertFromFullScreen(HWND window,tjs_uint w,tjs_uint h, iTVPDrawDevice* drawdevice)
{
	if(!TVPInFullScreen) return;

	//TVPReleaseVSyncTimingThread();

	drawdevice->RevertFromFullScreen( window, w, h, TVPDefaultScreenMode.BitsPerPixel, TVPDisplayColorFormat );

	TVPInFullScreen = false;

	TVPGetDisplayColorFormat();
	//TVPEnsureVSyncTimingThread();
}
//---------------------------------------------------------------------------





































//---------------------------------------------------------------------------
void TVPMinimizeFullScreenWindowAtInactivation()
{
	// only works when TVPUseChangeDisplaySettings == true
	// (Direct3D framework does this)

	if(!TVPInFullScreen) return;
	if(!TVPUseChangeDisplaySettings) return;

	::ChangeDisplaySettings(nullptr, 0);

	::ShowWindow(TVPFullScreenWindow, SW_MINIMIZE);
}
//---------------------------------------------------------------------------
void TVPRestoreFullScreenWindowAtActivation()
{
	// only works when TVPUseChangeDisplaySettings == true
	// (Direct3D framework does this)

	if(!TVPInFullScreen) return;
	if(!TVPUseChangeDisplaySettings) return;

	DEVMODE dm;
	ZeroMemory(&dm, sizeof(DEVMODE));
	dm.dmSize = sizeof(DEVMODE);
	dm.dmPelsWidth = TVPFullScreenMode.Width;
	dm.dmPelsHeight = TVPFullScreenMode.Height;
	dm.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;
	dm.dmBitsPerPel = TVPFullScreenMode.BitsPerPixel;
	::ChangeDisplaySettings((DEVMODE*)&dm, CDS_FULLSCREEN);

	ShowWindow(TVPFullScreenWindow, SW_RESTORE);
	SetWindowPos(TVPFullScreenWindow, HWND_TOP,
		0, 0, TVPFullScreenMode.Width, TVPFullScreenMode.Height, SWP_SHOWWINDOW);
}
//---------------------------------------------------------------------------


//---------------------------------------------------------------------------
static void TVPRestoreDisplayMode()
{
	// only works when TVPUseChangeDisplaySettings == true
	if(!TVPUseChangeDisplaySettings) return;
	if(!TVPInFullScreen) return;
	::ChangeDisplaySettings(nullptr, 0);
}
//---------------------------------------------------------------------------
static tTVPAtExit
	TVPRestoreDisplayModeAtExit(TVP_ATEXIT_PRI_CLEANUP, TVPUnloadDirect3D);
//---------------------------------------------------------------------------
















//---------------------------------------------------------------------------
// TVPGetModalWindowOwner
//---------------------------------------------------------------------------
HWND TVPGetModalWindowOwnerHandle()
{
	if(TVPFullScreenedWindow)
		return TVPFullScreenedWindow->GetHandle();
	else
		return Application->GetHandle();
}
//---------------------------------------------------------------------------
#endif

//---------------------------------------------------------------------------
// tTJSNI_Window
//---------------------------------------------------------------------------
tTJSNI_Window::tTJSNI_Window() {
    // TVPEnsureVSyncTimingThread();
    //	VSyncTimingThread = nullptr;
    _form = nullptr;
}
//---------------------------------------------------------------------------
tjs_error tTJSNI_Window::Construct(tjs_int numparams, tTJSVariant **param,
                                   iTJSDispatch2 *tjs_obj) {
    tjs_error hr = tTJSNI_BaseWindow::Construct(numparams, param, tjs_obj);
    if(TJS_FAILED(hr))
        return hr;
    if(numparams >= 1 && param[0]->Type() == tvtObject) {
        tTJSVariantClosure clo = param[0]->AsObjectClosureNoAddRef();
        tTJSNI_Window *win = nullptr;
        if(clo.Object != nullptr) {
            if(TJS_FAILED(clo.Object->NativeInstanceSupport(
                   TJS_NIS_GETINSTANCE, tTJSNC_Window::ClassID,
                   (iTJSNativeInstance **)&win)))
                TVPThrowExceptionMessage(TVPSpecifyWindow);
            if(!win)
                TVPThrowExceptionMessage(TVPSpecifyWindow);
        }
#if 0
		_form = new TTVPWindowForm(Application, this, win);
	} else {
		_form = new TTVPWindowForm(Application, this);
#endif
    }
    _form = TVPCreateAndAddWindow(this);
    return TJS_S_OK;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::Invalidate() {
    tTJSNI_BaseWindow::Invalidate();
#if 0
	if( VSyncTimingThread )
	{
		delete VSyncTimingThread;
		VSyncTimingThread = nullptr;
	}
#endif
    if(_form) {
        _form->InvalidateClose();
        _form = nullptr;
    }

    // remove all events
    TVPCancelSourceEvents(Owner);
    TVPCancelInputEvents(this);

    // Set Owner nullptr
    Owner = nullptr;
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::CanDeliverEvents() const {
    if(!_form)
        return false;
    return GetVisible() && _form->GetFormEnabled();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::NotifyWindowClose() { _form = nullptr; }
//---------------------------------------------------------------------------
void tTJSNI_Window::SendCloseMessage() {
    if(_form)
        _form->SendCloseMessage();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::TickBeat() {
    if(_form)
        _form->TickBeat();
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetWindowActive() {
    if(_form)
        return _form->GetWindowActive();
    return false;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ResetDrawDevice() {
    if(_form)
        _form->ResetDrawDevice();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::FullScreenGuard() const {
    if(_form) {
        if(_form->GetFullScreenMode())
            TVPThrowExceptionMessage(TVPInvalidPropertyInFullScreen);
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::PostInputEvent(const ttstr &name, iTJSDispatch2 *params) {
    // posts input event
    if(!_form)
        return;

    static ttstr key_name(TJS_W("key"));
    static ttstr shift_name(TJS_W("shift"));

    // check input event name
    enum tEventType { etUnknown, etOnKeyDown, etOnKeyUp, etOnKeyPress } type;

    if(name == TJS_W("onKeyDown"))
        type = etOnKeyDown;
    else if(name == TJS_W("onKeyUp"))
        type = etOnKeyUp;
    else if(name == TJS_W("onKeyPress"))
        type = etOnKeyPress;
    else
        type = etUnknown;

    if(type == etUnknown)
        TVPThrowExceptionMessage(TVPSpecifiedEventNameIsUnknown, name);

    if(type == etOnKeyDown || type == etOnKeyUp) {
        // this needs params, "key" and "shift"
        if(params == nullptr)
            TVPThrowExceptionMessage(TVPSpecifiedEventNeedsParameter, name);

        tjs_uint key;
        tjs_uint32 shift = 0;

        tTJSVariant val;
        if(TJS_SUCCEEDED(params->PropGet(0, key_name.c_str(),
                                         key_name.GetHint(), &val, params)))
            key = (tjs_int)val;
        else
            TVPThrowExceptionMessage(TVPSpecifiedEventNeedsParameter2, name,
                                     TJS_W("key"));

        if(TJS_SUCCEEDED(params->PropGet(0, shift_name.c_str(),
                                         shift_name.GetHint(), &val, params)))
            shift = (tjs_int)val;
        else
            TVPThrowExceptionMessage(TVPSpecifiedEventNeedsParameter2, name,
                                     TJS_W("shift"));

        uint16_t vcl_key = key;
        if(type == etOnKeyDown)
            _form->InternalKeyDown(key, shift);
        else if(type == etOnKeyUp)
            // Form->OnKeyUp(Form, vcl_key,
            // TVP_TShiftState_From_uint32(shift));
            _form->OnKeyUp(vcl_key, TVP_TShiftState_From_uint32(shift));
    } else if(type == etOnKeyPress) {
        // this needs param, "key"
        if(params == nullptr)
            TVPThrowExceptionMessage(TVPSpecifiedEventNeedsParameter, name);

        tjs_uint key;

        tTJSVariant val;
        if(TJS_SUCCEEDED(params->PropGet(0, key_name.c_str(),
                                         key_name.GetHint(), &val, params)))
            key = (tjs_int)val;
        else
            TVPThrowExceptionMessage(TVPSpecifiedEventNeedsParameter2, name,
                                     TJS_W("key"));

        char vcl_key = key;
        // Form->OnKeyPress(Form, vcl_key);
        _form->OnKeyPress(vcl_key, 0, false, false);
    }
}

//---------------------------------------------------------------------------
void tTJSNI_Window::NotifySrcResize() {
    tTJSNI_BaseWindow::NotifySrcResize();

    // is called from primary layer
    // ( or from TWindowForm to reset paint box's size )
    tjs_int w, h;
    DrawDevice->GetSrcSize(w, h);
    if(_form)
        _form->SetPaintBoxSize(w, h);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetDefaultMouseCursor() {
    // set window mouse cursor to default
    if(_form)
        _form->SetDefaultMouseCursor();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMouseCursor(tjs_int handle) {
    // set window mouse cursor
    if(_form)
        _form->SetMouseCursor(handle);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::GetCursorPos(tjs_int &x, tjs_int &y) {
    // get cursor pos in primary layer's coordinates
    if(_form)
        _form->GetCursorPos(x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetCursorPos(tjs_int x, tjs_int y) {
    // set cursor pos in primar layer's coordinates
    if(_form)
        _form->SetCursorPos(x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::WindowReleaseCapture() {
    //::ReleaseCapture(); // Windows API
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetHintText(iTJSDispatch2 *sender, const ttstr &text) {
    // set hint text to window
    if(_form)
        _form->SetHintText(sender, text);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetAttentionPoint(tTJSNI_BaseLayer *layer, tjs_int l,
                                      tjs_int t) {
    // set attention point to window
    if(_form) {
        // class TFont * font = nullptr;
        const tTVPFont *font = nullptr;
        if(layer) {
            iTVPBaseBitmap *bmp = layer->GetMainImage();
            if(bmp) {
                // font = bmp->GetFontCanvas()->GetFont(); =
                //  font = bmp->GetFontCanvas();
                const tTVPFont &finfo = bmp->GetFont();
                font = &finfo;
            }
        }

        _form->SetAttentionPoint(l, t, font);
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::DisableAttentionPoint() {
    // disable attention point
    if(_form)
        _form->DisableAttentionPoint();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetImeMode(tTVPImeMode mode) {
    // set ime mode
    if(_form)
        _form->SetImeMode(mode);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetDefaultImeMode(tTVPImeMode mode) {
    // set default ime mode
    if(_form) {
        //		Form->SetDefaultImeMode(mode,
        // LayerManager->GetFocusedLayer() == nullptr);
    }
}
//---------------------------------------------------------------------------
tTVPImeMode tTJSNI_Window::GetDefaultImeMode() const {
    if(_form)
        return _form->GetDefaultImeMode();
    return ::imDisable;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ResetImeMode() {
    // set default ime mode ( default mode is imDisable; IME is
    // disabled )
    if(_form)
        _form->ResetImeMode();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::BeginUpdate(const tTVPComplexRect &rects) {
    tTJSNI_BaseWindow::BeginUpdate(rects);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::EndUpdate() { tTJSNI_BaseWindow::EndUpdate(); }
//---------------------------------------------------------------------------
#if 0
HWND tTJSNI_Window::GetSurfaceWindowHandle()
{
	if(!_form) return nullptr;
	return _form->GetSurfaceWindowHandle();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::ZoomRectangle(tjs_int &left, tjs_int &top, tjs_int &right,
                                  tjs_int &bottom) {
    if(!_form)
        return;
    _form->ZoomRectangle(left, top, right, bottom);
}
//---------------------------------------------------------------------------
#if 0
HWND tTJSNI_Window::GetWindowHandle()
{
	if(!_form) return nullptr;
	return _form->GetWindowHandle();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::GetVideoOffset(tjs_int &ofsx, tjs_int &ofsy) {
    if(!_form) {
        ofsx = 0;
        ofsy = 0;
    } else {
        _form->GetVideoOffset(ofsx, ofsy);
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ReadjustVideoRect() {
    if(!_form)
        return;

    // re-adjust video rectangle.
    // this reconnects owner window and video offsets.

    tObjectListSafeLockHolder<tTJSNI_BaseVideoOverlay> holder(VideoOverlay);
    tjs_int count = VideoOverlay.GetSafeLockedObjectCount();

    for(tjs_int i = 0; i < count; i++) {
        tTJSNI_VideoOverlay *item =
            (tTJSNI_VideoOverlay *)VideoOverlay.GetSafeLockedObjectAt(i);
        if(!item)
            continue;
        item->ResetOverlayParams();
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::WindowMoved() {
    // inform video overlays that the window has moved.
    // video overlays typically owns Direct3D surface which is not a
    // part of normal window systems and does not matter where the
    // owner window is. so we must inform window moving to overlay
    // window.

    tObjectListSafeLockHolder<tTJSNI_BaseVideoOverlay> holder(VideoOverlay);
    tjs_int count = VideoOverlay.GetSafeLockedObjectCount();
    for(tjs_int i = 0; i < count; i++) {
        tTJSNI_VideoOverlay *item =
            (tTJSNI_VideoOverlay *)VideoOverlay.GetSafeLockedObjectAt(i);
        if(!item)
            continue;
        item->SetRectangleToVideoOverlay();
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::DetachVideoOverlay() {
    // detach video overlay window
    // this is done before the window is being fullscreened or
    // un-fullscreened.
    tObjectListSafeLockHolder<tTJSNI_BaseVideoOverlay> holder(VideoOverlay);
    tjs_int count = VideoOverlay.GetSafeLockedObjectCount();
    for(tjs_int i = 0; i < count; i++) {
        tTJSNI_VideoOverlay *item =
            (tTJSNI_VideoOverlay *)VideoOverlay.GetSafeLockedObjectAt(i);
        if(!item)
            continue;
        item->DetachVideoOverlay();
    }
}
//---------------------------------------------------------------------------
#if 0
HWND tTJSNI_Window::GetWindowHandleForPlugin()
{
	if(!_form) return nullptr;
	return _form->GetWindowHandleForPlugin();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::RegisterWindowMessageReceiver(tTVPWMRRegMode mode,
                                                  void *proc,
                                                  const void *userdata) {
    if(!_form)
        return;
    _form->RegisterWindowMessageReceiver(mode, proc, userdata);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::Close() {
    if(_form)
        _form->Close();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::OnCloseQueryCalled(bool b) {
    if(_form)
        _form->OnCloseQueryCalled(b);
}
//---------------------------------------------------------------------------
#ifdef USE_OBSOLETE_FUNCTIONS
void tTJSNI_Window::BeginMove() {
    FullScreenGuard();
    if(_form)
        _form->BeginMove();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::BringToFront() {
    if(_form)
        _form->BringToFront();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::Update(tTVPUpdateType type) {
    if(_form)
        _form->UpdateWindow(type);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ShowModal() {
    FullScreenGuard();
    if(_form) {
        TVPClearAllWindowInputEvents();
        // cancel all input events that can cause delayed operation
        _form->ShowWindowAsModal();
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::HideMouseCursor() {
    if(_form)
        _form->HideMouseCursor();
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetVisible() const {
    if(!_form)
        return false;
    return _form->GetVisible();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetVisible(bool s) {
    FullScreenGuard();
    if(_form)
        _form->SetVisibleFromScript(s);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::GetCaption(ttstr &v) const {
    if(_form)
        v = _form->GetCaption();
    else
        v.Clear();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetCaption(const ttstr &v) {
    if(_form)
        _form->SetCaption(v.AsStdString());
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetWidth(tjs_int w) {
    FullScreenGuard();
    if(_form)
        _form->SetWidth(w);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetWidth() const {
    if(!_form)
        return 0;
    return _form->GetWidth();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetHeight(tjs_int h) {
    FullScreenGuard();
    if(_form)
        _form->SetHeight(h);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetHeight() const {
    if(!_form)
        return 0;
    return _form->GetHeight();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetLeft(tjs_int l) {
    FullScreenGuard();
    if(_form)
        _form->SetLeft(l);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetLeft() const {
    if(!_form)
        return 0;
    return _form->GetLeft();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetTop(tjs_int t) {
    FullScreenGuard();
    if(_form)
        _form->SetTop(t);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetTop() const {
    if(!_form)
        return 0;
    return _form->GetTop();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetSize(tjs_int w, tjs_int h) {
    FullScreenGuard();
    if(_form)
        _form->SetSize(w, h);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMinWidth(int v) {
    FullScreenGuard();
    if(_form)
        _form->SetMinWidth(v);
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetMinWidth() const {
    if(_form)
        return _form->GetMinWidth();
    else
        return 0;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMinHeight(int v) {
    FullScreenGuard();
    if(_form)
        _form->SetMinHeight(v);
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetMinHeight() const {
    if(_form)
        return _form->GetMinHeight();
    else
        return 0;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMinSize(int w, int h) {
    FullScreenGuard();
    if(_form)
        _form->SetMinSize(w, h);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMaxWidth(int v) {
    FullScreenGuard();
    if(_form)
        _form->SetMaxWidth(v);
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetMaxWidth() const {
    if(_form)
        return _form->GetMaxWidth();
    else
        return 0;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMaxHeight(int v) {
    FullScreenGuard();
    if(_form)
        _form->SetMaxHeight(v);
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetMaxHeight() const {
    if(_form)
        return _form->GetMaxHeight();
    else
        return 0;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMaxSize(int w, int h) {
    FullScreenGuard();
    if(_form)
        _form->SetMaxSize(w, h);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetPosition(tjs_int l, tjs_int t) {
    FullScreenGuard();
    if(_form)
        _form->SetPosition(l, t);
}
//---------------------------------------------------------------------------
#ifdef USE_OBSOLETE_FUNCTIONS
void tTJSNI_Window::SetLayerLeft(tjs_int l) {
    if(_form)
        _form->SetLayerLeft(l);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetLayerLeft() const {
    if(!_form)
        return 0;
    return _form->GetLayerLeft();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetLayerTop(tjs_int t) {
    if(_form)
        _form->SetLayerTop(t);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetLayerTop() const {
    if(!_form)
        return 0;
    return _form->GetLayerTop();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetLayerPosition(tjs_int l, tjs_int t) {
    if(_form)
        _form->SetLayerPosition(l, t);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetInnerSunken(bool b) {
    FullScreenGuard();
    if(_form)
        _form->SetInnerSunken(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetInnerSunken() const {
    if(!_form)
        return true;
    return _form->GetInnerSunken();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::SetInnerWidth(tjs_int w) {
    FullScreenGuard();
    if(_form)
        _form->SetInnerWidth(w);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetInnerWidth() const {
    if(!_form)
        return 0;
    return _form->GetInnerWidth();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetInnerHeight(tjs_int h) {
    FullScreenGuard();
    if(_form)
        _form->SetInnerHeight(h);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetInnerHeight() const {
    if(!_form)
        return 0;
    return _form->GetInnerHeight();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetInnerSize(tjs_int w, tjs_int h) {
    FullScreenGuard();
    if(_form)
        _form->SetInnerSize(w, h);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetBorderStyle(tTVPBorderStyle st) {
    FullScreenGuard();
    if(_form)
        _form->SetBorderStyle(st);
}
//---------------------------------------------------------------------------
tTVPBorderStyle tTJSNI_Window::GetBorderStyle() const {
    if(!_form)
        return (tTVPBorderStyle)0;
    return _form->GetBorderStyle();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetStayOnTop(bool b) {
    if(!_form)
        return;
    _form->SetStayOnTop(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetStayOnTop() const {
    if(!_form)
        return false;
    return _form->GetStayOnTop();
}
//---------------------------------------------------------------------------
#ifdef USE_OBSOLETE_FUNCTIONS
void tTJSNI_Window::SetShowScrollBars(bool b) {
    if(_form)
        _form->SetShowScrollBars(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetShowScrollBars() const {
    if(!_form)
        return true;
    return _form->GetShowScrollBars();
}
#endif
//---------------------------------------------------------------------------
void tTJSNI_Window::SetFullScreen(bool b) {
    if(!_form)
        return;
    _form->SetFullScreenMode(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetFullScreen() const {
    if(!_form)
        return false;
    return _form->GetFullScreenMode();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetUseMouseKey(bool b) {
    if(!_form)
        return;
    _form->SetUseMouseKey(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetUseMouseKey() const {
    if(!_form)
        return false;
    return _form->GetUseMouseKey();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetTrapKey(bool b) {
    if(!_form)
        return;
    _form->SetTrapKey(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetTrapKey() const {
    if(!_form)
        return false;
    return _form->GetTrapKey();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMaskRegion(tjs_int threshold) {
    if(!_form)
        return;

    if(!DrawDevice)
        TVPThrowExceptionMessage(TVPWindowHasNoLayer);
    tTJSNI_BaseLayer *lay = DrawDevice->GetPrimaryLayer();
    if(!lay)
        TVPThrowExceptionMessage(TVPWindowHasNoLayer);
    //	Form->SetMaskRegion(
    //((tTJSNI_Layer*)lay)->CreateMaskRgn((tjs_uint)threshold) );
}
//---------------------------------------------------------------------------
void tTJSNI_Window::RemoveMaskRegion() {
    if(!_form)
        return;
    _form->RemoveMaskRegion();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMouseCursorState(tTVPMouseCursorState mcs) {
    if(!_form)
        return;
    _form->SetMouseCursorState(mcs);
}
//---------------------------------------------------------------------------
tTVPMouseCursorState tTJSNI_Window::GetMouseCursorState() const {
    if(!_form)
        return mcsVisible;
    return _form->GetMouseCursorState();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetFocusable(bool b) {
    if(!_form)
        return;
    _form->SetFocusable(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetFocusable() {
    if(!_form)
        return true;
    return _form->GetFocusable();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetZoom(tjs_int numer, tjs_int denom) {
    if(!_form)
        return;
    _form->SetZoom(numer, denom);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetZoomNumer(tjs_int n) {
    if(!_form)
        return;
    _form->SetZoomNumer(n);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetZoomNumer() const {
    if(!_form)
        return 1;
    return _form->GetZoomNumer();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetZoomDenom(tjs_int n) {
    if(!_form)
        return;
    _form->SetZoomDenom(n);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetZoomDenom() const {
    if(!_form)
        return 1;
    return _form->GetZoomDenom();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetTouchScaleThreshold(tjs_real threshold) {
    if(!_form)
        return;
    _form->SetTouchScaleThreshold(threshold);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchScaleThreshold() const {
    if(!_form)
        return 0;
    return _form->GetTouchScaleThreshold();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetTouchRotateThreshold(tjs_real threshold) {
    if(!_form)
        return;
    _form->SetTouchRotateThreshold(threshold);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchRotateThreshold() const {
    if(!_form)
        return 0;
    return _form->GetTouchRotateThreshold();
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchPointStartX(tjs_int index) {
    if(!_form)
        return 0;
    return _form->GetTouchPointStartX(index);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchPointStartY(tjs_int index) {
    if(!_form)
        return 0;
    return _form->GetTouchPointStartY(index);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchPointX(tjs_int index) {
    if(!_form)
        return 0;
    return _form->GetTouchPointX(index);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchPointY(tjs_int index) {
    if(!_form)
        return 0;
    return _form->GetTouchPointY(index);
}
//---------------------------------------------------------------------------
tjs_real tTJSNI_Window::GetTouchPointID(tjs_int index) {
    if(!_form)
        return 0;
    return _form->GetTouchPointID(index);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetTouchPointCount() {
    if(!_form)
        return 0;
    return _form->GetTouchPointCount();
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetTouchVelocity(tjs_int id, float &x, float &y,
                                     float &speed) const {
    if(!_form)
        return false;
    return _form->GetTouchVelocity(id, x, y, speed);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetMouseVelocity(float &x, float &y, float &speed) const {
    if(!_form)
        return false;
    return _form->GetMouseVelocity(x, y, speed);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ResetMouseVelocity() {
    if(!_form)
        return;
    return _form->ResetMouseVelocity();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetHintDelay(tjs_int delay) {
    if(!_form)
        return;
    _form->SetHintDelay(delay);
}
//---------------------------------------------------------------------------
tjs_int tTJSNI_Window::GetHintDelay() const {
    if(!_form)
        return 0;
    return _form->GetHintDelay();
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetEnableTouch(bool b) {
    if(!_form)
        return;
    _form->SetEnableTouch(b);
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::GetEnableTouch() const {
    if(!_form)
        return false;
    return _form->GetEnableTouch();
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetDisplayOrientation() {
    if(!_form)
        return orientUnknown;
    return _form->GetDisplayOrientation();
}
//---------------------------------------------------------------------------
int tTJSNI_Window::GetDisplayRotate() {
    if(!_form)
        return -1;
    return _form->GetDisplayRotate();
}
//---------------------------------------------------------------------------
bool tTJSNI_Window::WaitForVBlank(tjs_int *in_vblank, tjs_int *delayed) {
    if(DrawDevice)
        return DrawDevice->WaitForVBlank(in_vblank, delayed);
    return false;
}
//---------------------------------------------------------------------------
void tTJSNI_Window::UpdateVSyncThread() {
#if 0
	if( WaitVSync ) {
		if( VSyncTimingThread == nullptr ) {
			VSyncTimingThread = new tTVPVSyncTimingThread(this);
		}
	} else {
		if( VSyncTimingThread ) {
			delete VSyncTimingThread;
		}
		VSyncTimingThread = nullptr;
	}
#endif
}
//---------------------------------------------------------------------------
void tTJSNI_Window::StartBitmapCompletion(iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->StartBitmapCompletion(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::NotifyBitmapCompleted(class iTVPLayerManager *manager,
                                          tjs_int x, tjs_int y,
                                          tTVPBaseTexture *bmp,
                                          const tTVPRect &cliprect,
                                          tTVPLayerType type, tjs_int opacity) {
    if(DrawDevice) {
#if 0
		DrawDevice->NotifyBitmapCompleted(manager,x,y,bits,bitmapinfo->GetBITMAPINFO(), cliprect, type, opacity );
#endif
        DrawDevice->NotifyBitmapCompleted(manager, x, y, bmp, cliprect, type,
                                          opacity);
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::EndBitmapCompletion(iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->EndBitmapCompletion(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetMouseCursor(class iTVPLayerManager *manager,
                                   tjs_int cursor) {
    if(DrawDevice) {
        if(cursor == 0)
            DrawDevice->SetDefaultMouseCursor(manager);
        else
            DrawDevice->SetMouseCursor(manager, cursor);
    }
}
//---------------------------------------------------------------------------
void tTJSNI_Window::GetCursorPos(class iTVPLayerManager *manager, tjs_int &x,
                                 tjs_int &y) {
    if(DrawDevice)
        DrawDevice->GetCursorPos(manager, x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetCursorPos(class iTVPLayerManager *manager, tjs_int x,
                                 tjs_int y) {
    if(DrawDevice)
        DrawDevice->SetCursorPos(manager, x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ReleaseMouseCapture(class iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->WindowReleaseCapture(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetHint(class iTVPLayerManager *manager,
                            iTJSDispatch2 *sender, const ttstr &hint) {
    if(DrawDevice)
        DrawDevice->SetHintText(manager, sender, hint);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::NotifyLayerResize(class iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->NotifyLayerResize(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::NotifyLayerImageChange(class iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->NotifyLayerImageChange(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetAttentionPoint(class iTVPLayerManager *manager,
                                      tTJSNI_BaseLayer *layer, tjs_int x,
                                      tjs_int y) {
    if(DrawDevice)
        DrawDevice->SetAttentionPoint(manager, layer, x, y);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::DisableAttentionPoint(class iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->DisableAttentionPoint(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::SetImeMode(class iTVPLayerManager *manager,
                               tjs_int mode) // mode == tTVPImeMode
{
    if(DrawDevice)
        DrawDevice->SetImeMode(manager, (tTVPImeMode)mode);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::ResetImeMode(class iTVPLayerManager *manager) {
    if(DrawDevice)
        DrawDevice->ResetImeMode(manager);
}
//---------------------------------------------------------------------------
void tTJSNI_Window::OnTouchUp(tjs_real x, tjs_real y, tjs_real cx, tjs_real cy,
                              tjs_uint32 id) {
    tTJSNI_BaseWindow::OnTouchUp(x, y, cx, cy, id);
    if(_form) {
        _form->ResetTouchVelocity(id);
    }
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// tTJSNC_Window::CreateNativeInstance : returns proper instance
// object
//---------------------------------------------------------------------------
tTJSNativeInstance *tTJSNC_Window::CreateNativeInstance() {
    return new tTJSNI_Window();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPCreateNativeClass_Window
//---------------------------------------------------------------------------
tTJSNativeClass *TVPCreateNativeClass_Window() {
    tTJSNativeClass *cls = new tTJSNC_Window();
    static tjs_uint32 TJS_NCM_CLASSID;
    TJS_NCM_CLASSID = tTJSNC_Window::ClassID;
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(findFullScreenCandidates) {
        if(numparams < 5)
            return TJS_E_BADPARAMCOUNT;
#if 0
	std::vector<tTVPScreenModeCandidate> candidates;

	tTVPScreenMode preferred;
	preferred.Width = *param[0];
	preferred.Height = *param[1];
	preferred.BitsPerPixel = *param[2];
	tjs_int mode = *param[3];
	tjs_int zoom_mode = *param[4];

	TVPMakeFullScreenModeCandidates(preferred, (tTVPFullScreenResolutionMode)mode,
		(tTVPFullScreenUsingEngineZoomMode)zoom_mode, candidates);
#endif

        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, findFullScreenCandidates)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(registerMessageReceiver) {
        TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                                /*var. type*/ tTJSNI_Window);
        if(numparams < 3)
            return TJS_E_BADPARAMCOUNT;

        _this->RegisterWindowMessageReceiver(
            (tTVPWMRRegMode)((tjs_int)*param[0]),
            reinterpret_cast<void *>(param[1]->AsInteger()),
            reinterpret_cast<const void *>(param[2]->AsInteger()));

        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, registerMessageReceiver)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(getTouchPoint) {
        TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                                /*var. type*/ tTJSNI_Window);
        if(numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        tjs_int index = (tjs_int)*param[0];
        if(index < _this->GetTouchPointCount()) {
            if(result) {
                iTJSDispatch2 *object = TJSCreateDictionaryObject();

                static ttstr startX_name(TJS_W("startX"));
                static ttstr startY_name(TJS_W("startY"));
                static ttstr X_name(TJS_W("x"));
                static ttstr Y_name(TJS_W("y"));
                static ttstr ID_name(TJS_W("ID"));
                {
                    tTJSVariant val(_this->GetTouchPointStartX(index));
                    if(TJS_FAILED(object->PropSet(
                           TJS_MEMBERENSURE | TJS_IGNOREPROP,
                           startX_name.c_str(), startX_name.GetHint(), &val,
                           object)))
                        TVPThrowInternalError;
                }
                {
                    tTJSVariant val(_this->GetTouchPointStartY(index));
                    if(TJS_FAILED(object->PropSet(
                           TJS_MEMBERENSURE | TJS_IGNOREPROP,
                           startY_name.c_str(), startY_name.GetHint(), &val,
                           object)))
                        TVPThrowInternalError;
                }
                {
                    tTJSVariant val(_this->GetTouchPointX(index));
                    if(TJS_FAILED(object->PropSet(
                           TJS_MEMBERENSURE | TJS_IGNOREPROP, X_name.c_str(),
                           X_name.GetHint(), &val, object)))
                        TVPThrowInternalError;
                }
                {
                    tTJSVariant val(_this->GetTouchPointY(index));
                    if(TJS_FAILED(object->PropSet(
                           TJS_MEMBERENSURE | TJS_IGNOREPROP, Y_name.c_str(),
                           Y_name.GetHint(), &val, object)))
                        TVPThrowInternalError;
                }
                {
                    tTJSVariant val(_this->GetTouchPointID(index));
                    if(TJS_FAILED(object->PropSet(
                           TJS_MEMBERENSURE | TJS_IGNOREPROP, ID_name.c_str(),
                           ID_name.GetHint(), &val, object)))
                        TVPThrowInternalError;
                }
                tTJSVariant objval(object, object);
                object->Release();
                *result = objval;
            }
        } else {
            return TJS_E_INVALIDPARAM;
        }
        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, getTouchPoint)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(getTouchVelocity) {
        TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                                /*var. type*/ tTJSNI_Window);
        if(numparams < 4)
            return TJS_E_BADPARAMCOUNT;

        tjs_int id = (tjs_int)*param[0];
        float x, y, speed;
        bool ret = _this->GetTouchVelocity(id, x, y, speed);
        if(result) {
            *result = ret ? (tjs_int)1 : (tjs_int)0;
        }
        if(ret) {
            (*param[1]) = (tjs_real)x;
            (*param[2]) = (tjs_real)y;
            (*param[3]) = (tjs_real)speed;
        } else {
            (*param[1]) = (tjs_real)0.0;
            (*param[2]) = (tjs_real)0.0;
            (*param[3]) = (tjs_real)0.0;
        }

        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, getTouchVelocity)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(getMouseVelocity) {
        TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                                /*var. type*/ tTJSNI_Window);
        if(numparams < 3)
            return TJS_E_BADPARAMCOUNT;

        float x, y, speed;
        bool ret = _this->GetMouseVelocity(x, y, speed);
        if(result) {
            *result = ret ? (tjs_int)1 : (tjs_int)0;
        }
        if(ret) {
            (*param[0]) = (tjs_real)x;
            (*param[1]) = (tjs_real)y;
            (*param[2]) = (tjs_real)speed;
        } else {
            (*param[0]) = (tjs_real)0.0;
            (*param[1]) = (tjs_real)0.0;
            (*param[2]) = (tjs_real)0.0;
        }

        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, getMouseVelocity)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_METHOD_DECL(resetMouseVelocity) {
        TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                                /*var. type*/ tTJSNI_Window);
        _this->ResetMouseVelocity();
        return TJS_S_OK;
    }
    TJS_END_NATIVE_METHOD_DECL_OUTER(cls, resetMouseVelocity)
    //---------------------------------------------------------------------------
    TJS_BEGIN_NATIVE_PROP_DECL(HWND){
        TJS_BEGIN_NATIVE_PROP_GETTER{ TJS_GET_NATIVE_INSTANCE(
            /*var. name*/ _this, /*var. type*/ tTJSNI_Window);
    *result = (tTVInteger)(tjs_intptr_t)_this /*->GetWindowHandleForPlugin()*/;
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, HWND)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(drawDevice){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetDrawDeviceObject();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_BEGIN_NATIVE_PROP_SETTER {
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
    _this->SetDrawDeviceObject(*param);
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, drawDevice)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(touchScaleThreshold){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetTouchScaleThreshold();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_BEGIN_NATIVE_PROP_SETTER {
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
    _this->SetTouchScaleThreshold(*param);
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, touchScaleThreshold)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(touchRotateThreshold){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetTouchRotateThreshold();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_BEGIN_NATIVE_PROP_SETTER {
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
    _this->SetTouchRotateThreshold(*param);
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, touchRotateThreshold)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(touchPointCount){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetTouchPointCount();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, touchPointCount)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(hintDelay){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetHintDelay();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_BEGIN_NATIVE_PROP_SETTER {
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
    _this->SetHintDelay(*param);
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, hintDelay)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(enableTouch){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetEnableTouch() ? 1 : 0;
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_BEGIN_NATIVE_PROP_SETTER {
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
    _this->SetEnableTouch(((tjs_int)*param) ? true : false);
    return TJS_S_OK;
}
TJS_END_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, enableTouch)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(displayOrientation){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetDisplayOrientation();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, displayOrientation)
//---------------------------------------------------------------------------
TJS_BEGIN_NATIVE_PROP_DECL(displayRotate){ TJS_BEGIN_NATIVE_PROP_GETTER{
    TJS_GET_NATIVE_INSTANCE(/*var. name*/ _this,
                            /*var. type*/ tTJSNI_Window);
*result = _this->GetDisplayRotate();
return TJS_S_OK;
}
TJS_END_NATIVE_PROP_GETTER

TJS_DENY_NATIVE_PROP_SETTER
}
TJS_END_NATIVE_PROP_DECL_OUTER(cls, displayRotate)
//---------------------------------------------------------------------------

// TVPGetDisplayColorFormat(); // this will be ran only once here

return cls;
}
//---------------------------------------------------------------------------
