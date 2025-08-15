#include "KAGParserEx.hpp"
#include "ncbind.hpp"

static iTJSDispatch2 *origKAGParser = NULL;

void kagparserex_init()
{
	tTJSNI_KAGParser::initMethod();
	
	iTJSDispatch2 * global = TVPGetScriptDispatch();
	if (global) {
		tTJSVariant val;
		if (TJS_SUCCEEDED(global->PropGet(0, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global))) {
			origKAGParser = val.AsObject();
			val.Clear();
		}
		iTJSDispatch2 * tjsclass = tTJSNC_KAGParser::CreateNativeClass();
		val = tTJSVariant(tjsclass);
		tjsclass->Release();
		global->PropSet(TJS_MEMBERENSURE, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global);
		global->Release();
	}
}

void kagparserex_done(){

    iTJSDispatch2 * global = TVPGetScriptDispatch();
    if (global)	{
        global->DeleteMember(0, TVP_KAGPARSER_EX_CLASSNAME, NULL, global);
        if (origKAGParser) {
            tTJSVariant val(origKAGParser);
            origKAGParser->Release();
            origKAGParser = NULL;
            global->PropSet(TJS_MEMBERENSURE, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global);
        }
        global->Release();
    }

    tTJSNI_KAGParser::doneMethod();
}

NCB_PRE_REGIST_CALLBACK(kagparserex_init);
NCB_POST_UNREGIST_CALLBACK(kagparserex_done);
#ifndef NO_V2LINK

#define EXPORT(hr) extern "C" __declspec(dllexport) hr __stdcall

#ifdef _MSC_VER
# if defined(_M_AMD64) || defined(_M_X64)
#  pragma comment(linker, "/EXPORT:V2Link")
#  pragma comment(linker, "/EXPORT:V2Unlink")
# else
#pragma comment(linker, "/EXPORT:V2Link=_V2Link@4")
#pragma comment(linker, "/EXPORT:V2Unlink=_V2Unlink@0")
#endif
#endif

int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void* lpReserved)
{
	return 1;
}


//---------------------------------------------------------------------------
static tjs_int GlobalRefCountAtInit = 0;
EXPORT(HRESULT) V2Link(iTVPFunctionExporter *exporter)
{
	TVPInitImportStub(exporter);

	kagparserex_init();

	GlobalRefCountAtInit = TVPPluginGlobalRefCount;
	return S_OK;
}
//---------------------------------------------------------------------------
EXPORT(HRESULT) V2Unlink()
{
	if(TVPPluginGlobalRefCount > GlobalRefCountAtInit) return E_FAIL;

	iTJSDispatch2 * global = TVPGetScriptDispatch();
	if (global)	{
		global->DeleteMember(0, TVP_KAGPARSER_EX_CLASSNAME, NULL, global);
		if (origKAGParser) {
			tTJSVariant val(origKAGParser);
			origKAGParser->Release();
			origKAGParser = NULL;
			global->PropSet(TJS_MEMBERENSURE, TVP_KAGPARSER_EX_CLASSNAME, NULL, &val, global);
		}
		global->Release();
	}

	tTJSNI_KAGParser::doneMethod();
	
	TVPUninitImportStub();
	return S_OK;
}

#endif // NO_V2LINK
