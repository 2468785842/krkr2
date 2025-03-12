//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// System Initialization and Uninitialization
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "FilePathUtil.h"
// #include <delayimp.h>
// #include <mmsystem.h>
// #include <objbase.h>
// #include <commdlg.h>

#include "SysInitImpl.h"
#include "StorageIntf.h"
#include "StorageImpl.h"
#include "MsgIntf.h"
#include "GraphicsLoaderIntf.h"
#include "SystemControl.h"
#include "DebugIntf.h"
#include "tjsLex.h"
#include "LayerIntf.h"
#include "Random.h"
#include "DetectCPU.h"
#include "XP3Archive.h"
#include "ScriptMgnIntf.h"
#include "XP3Archive.h"
// #include "VersionFormUnit.h"
#include "EmergencyExit.h"

// #include "tvpgl_ia32_intf.h"

#include "BinaryStream.h"
#include "Application.h"
#include "Exception.h"
#include "ApplicationSpecialPath.h"
// #include "resource.h"
// #include "ConfigFormUnit.h"
#include "TickCount.h"

#ifdef IID
#undef IID
#endif
#define uint32_t unsigned int

#include <thread>

#undef uint32_t

#include "Platform.h"
#include "ConfigManager/IndividualConfigManager.h"

//---------------------------------------------------------------------------
// global data
//---------------------------------------------------------------------------
ttstr TVPNativeProjectDir;
ttstr TVPNativeDataPath;
bool TVPProjectDirSelected = false;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// System security options
//---------------------------------------------------------------------------
// system security options are held inside the executable, where
// signature checker will refer. This enables the signature checker
// (or other security modules like XP3 encryption module) to check
// the changes which is not intended by the contents author.
const static char TVPSystemSecurityOptions[] =
    "-- TVPSystemSecurityOptions "
    "disablemsgmap(0):forcedataxp3(0):acceptfilenameargument(0) --";

//---------------------------------------------------------------------------
int GetSystemSecurityOption(const char *name) {
    size_t namelen = TJS_nstrlen(name);
    const char *p = TJS_nstrstr(TVPSystemSecurityOptions, name);
    if(!p)
        return 0;
    if(p[namelen] == '(' && p[namelen + 2] == ')')
        return p[namelen + 1] - '0';
    return 0;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// delayed DLL load procedure hook
//---------------------------------------------------------------------------
// for supporting of "_inmm.dll" (C) irori
// http://www.geocities.co.jp/Playtown-Domino/8282/
//---------------------------------------------------------------------------
/*
note:
        _inmm.dll is a replacement of winmm.dll ( windows multimedia
system dll
). _inmm.dll enables "MCI CD-DA supporting applications" to play
musics using various way, including midi, mp3, wave or digital CD-DA,
by applying a patch on those applications.

        TVP(kirikiri) system has a special structure of executable
file -- delayed loading of winmm.dll, in addition to compressed
code/data area by the UPX executable packer. _inmm.dll's patcher can
not recognize TVP's import area.

        So we must implement supporting of _inmm.dll alternatively.

        This function only works when -_inmm=yes or -inmm=yes option
is specified at command line or embeded options area.
*/

void TVPDumpHWException() {
    // dummy
}

//---------------------------------------------------------------------------
static void TVPInitRandomGenerator() {
    tjs_uint32 tick = TVPGetRoughTickCount32();
    TVPPushEnvironNoise(&tick, sizeof(tick));
    std::thread::id tid = std::this_thread::get_id();
    TVPPushEnvironNoise(&tid, sizeof(tid));
    time_t curtime = time(nullptr);
    TVPPushEnvironNoise(&curtime, sizeof(curtime));
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPInitializeBaseSystems
//---------------------------------------------------------------------------
void TVPInitializeBaseSystems() {
    // set system archive delimiter
    tTJSVariant v;
    if(TVPGetCommandLine(TJS_W("-arcdelim"), &v))
        TVPArchiveDelimiter = ttstr(v)[0];

    // set default current directory
    {
        TVPSetCurrentDirectory(
            IncludeTrailingBackslash(ExtractFileDir(ExePath())));
    }

    // load message map file
    bool load_msgmap = GetSystemSecurityOption("disablemsgmap") == 0;

    if(load_msgmap) {
        const tjs_char name_msgmap[] = TJS_W("msgmap.tjs");
        if(TVPIsExistentStorage(name_msgmap))
            TVPExecuteStorage(name_msgmap, nullptr, false, TJS_W(""));
    }
}

static tjs_uint64 TVPTotalPhysMemory = 0;

static void TVPInitProgramArgumentsAndDataPath(bool stop_after_datapath_got);

void TVPBeforeSystemInit() {
    // RegisterDllLoadHook();
    //  register DLL delayed import hook to support _inmm.dll

    TVPInitProgramArgumentsAndDataPath(false); // ensure command line

    // set system archive delimiter after patch.tjs specified
    tTJSVariant v;
    if(TVPGetCommandLine(TJS_W("-arcdelim"), &v))
        TVPArchiveDelimiter = ttstr(v)[0];

    if(TVPIsExistentStorageNoSearchNoNormalize(TVPProjectDir)) {
        TVPProjectDir += TVPArchiveDelimiter;
    } else {
        TVPProjectDir += TJS_W("/");
    }
    TVPSetCurrentDirectory(TVPProjectDir);
    // randomize
    TVPInitRandomGenerator();

    // memory usage
    {
        TVPMemoryInfo meminf;
        TVPGetMemoryInfo(meminf);
        TVPPushEnvironNoise(&meminf, sizeof(meminf));

        TVPTotalPhysMemory = meminf.MemTotal * 1024;
        if(TVPTotalPhysMemory > 768 * 1024 * 1024) {
            TVPTotalPhysMemory -=
                512 * 1024 * 1024; // assume that system reserved 512M memory
        } else {
            TVPTotalPhysMemory /= 2; // use half memory in small memory devices
        }

        TVPAddImportantLog(TVPFormatMessage(TVPInfoTotalPhysicalMemory,
                                            tjs_int64(TVPTotalPhysMemory)));
        if(TVPTotalPhysMemory > 256 * 1024 * 1024) {
            std::string str =
                IndividualConfigManager::GetInstance()->GetValue<std::string>(
                    "memusage", "unlimited");
            if(str == ("low"))
                TVPTotalPhysMemory = 0; // assumes zero
            else if(str == ("medium"))
                TVPTotalPhysMemory = 128 * 1024 * 1024;
            else if(str == ("high"))
                TVPTotalPhysMemory = 256 * 1024 * 1024;
        } else { // use minimum memory usage if less than 256M(512M
                 // physics)
            TVPTotalPhysMemory = 0;
        }

        if(TVPTotalPhysMemory < 128 * 1024 * 1024) {
            // very very low memory, forcing to assume zero memory
            TVPTotalPhysMemory = 0;
        }

        if(TVPTotalPhysMemory < 128 * 1024 * 1024) {
            // extra low memory
            if(TJSObjectHashBitsLimit > 0)
                TJSObjectHashBitsLimit = 0;
            TVPSegmentCacheLimit = 0;
            TVPFreeUnusedLayerCache = true; // in LayerIntf.cpp
        } else if(TVPTotalPhysMemory < 256 * 1024 * 1024) {
            // low memory
            if(TJSObjectHashBitsLimit > 4)
                TJSObjectHashBitsLimit = 4;
        }
    }
}

//---------------------------------------------------------------------------
static void TVPDumpOptions();

//---------------------------------------------------------------------------
extern bool TVPEnableGlobalHeapCompaction;

extern void TVPGL_SSE2_Init();

extern "C" void TVPGL_ASM_Init();
extern bool TVPAutoSaveBookMark;
static bool TVPHighTimerPeriod = false;
static uint32_t TVPTimeBeginPeriodRes = 0;

//---------------------------------------------------------------------------
void TVPAfterSystemInit() {
    // check CPU type
    TVPDetectCPU();

    TVPAllocGraphicCacheOnHeap = false; // always false since beta 20

    // determine maximum graphic cache limit
    tTJSVariant opt;
    tjs_int64 limitmb = -1;
    if(TVPGetCommandLine(TJS_W("-gclim"), &opt)) {
        ttstr str(opt);
        if(str == TJS_W("auto"))
            limitmb = -1;
        else
            limitmb = opt.AsInteger();
    }

    if(limitmb == -1) {
        if(TVPTotalPhysMemory <= 32 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 0;
        else if(TVPTotalPhysMemory <= 48 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 0;
        else if(TVPTotalPhysMemory <= 64 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 0;
        else if(TVPTotalPhysMemory <= 96 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 4;
        else if(TVPTotalPhysMemory <= 128 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 8;
        else if(TVPTotalPhysMemory <= 192 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 12;
        else if(TVPTotalPhysMemory <= 256 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 20;
        else if(TVPTotalPhysMemory <= 512 * 1024 * 1024)
            TVPGraphicCacheSystemLimit = 40;
        else
            TVPGraphicCacheSystemLimit =
                tjs_uint64(TVPTotalPhysMemory /
                           (1024 * 1024 * 10)); // cachemem = physmem / 10
        TVPGraphicCacheSystemLimit *= 1024 * 1024;
    } else {
        TVPGraphicCacheSystemLimit = limitmb * 1024 * 1024;
    }
    // 32bit なので 512MB までに制限
    if(TVPGraphicCacheSystemLimit >= 512 * 1024 * 1024)
        TVPGraphicCacheSystemLimit = 512 * 1024 * 1024;

    if(TVPTotalPhysMemory <= 64 * 1024 * 1024)
        TVPSetFontCacheForLowMem();

    //	TVPGraphicCacheSystemLimit = 1*1024*1024; // DEBUG

    if(TVPGetCommandLine(TJS_W("-autosave"), &opt)) {
        ttstr str(opt);
        if(str == TJS_W("yes")) {
            TVPAutoSaveBookMark = true;
        }
    }
    // check TVPGraphicSplitOperation option
    std::string _val =
        IndividualConfigManager::GetInstance()->GetValue<std::string>(
            "renderer", "software");
    if(_val != "software") {
        TVPGraphicSplitOperationType = gsotNone;
    } else {
        TVPDrawThreadNum =
            IndividualConfigManager::GetInstance()->GetValue<int>(
                "software_draw_thread", 0);
        if(TVPGetCommandLine(TJS_W("-gsplit"), &opt)) {
            ttstr str(opt);
            if(str == TJS_W("no"))
                TVPGraphicSplitOperationType = gsotNone;
            else if(str == TJS_W("int"))
                TVPGraphicSplitOperationType = gsotInterlace;
            else if(str == TJS_W("yes") || str == TJS_W("simple"))
                TVPGraphicSplitOperationType = gsotSimple;
            else if(str == TJS_W("bidi"))
                TVPGraphicSplitOperationType = gsotBiDirection;
        }
    }

    // check TVPDefaultHoldAlpha option
    if(TVPGetCommandLine(TJS_W("-holdalpha"), &opt)) {
        ttstr str(opt);
        if(str == TJS_W("yes") || str == TJS_W("true"))
            TVPDefaultHoldAlpha = true;
        else
            TVPDefaultHoldAlpha = false;
    }

    // check TVPJPEGFastLoad option
    if(TVPGetCommandLine(TJS_W("-jpegdec"),
                         &opt)) // this specifies precision for JPEG decoding
    {
        ttstr str(opt);
        if(str == TJS_W("normal"))
            TVPJPEGLoadPrecision = jlpMedium;
        else if(str == TJS_W("low"))
            TVPJPEGLoadPrecision = jlpLow;
        else if(str == TJS_W("high"))
            TVPJPEGLoadPrecision = jlpHigh;
    }

    // dump option
    TVPDumpOptions();

    // initilaize x86 graphic routines
#if 0
#ifndef TJS_64BIT_OS
    TVPGL_IA32_Init();
#endif
    TVPGL_SSE2_Init();
#endif
    //	TVPGL_ASM_Init();

    // timer precision
    uint32_t prectick = 1;
    if(TVPGetCommandLine(TJS_W("-timerprec"), &opt)) {
        ttstr str(opt);
        if(str == TJS_W("high"))
            prectick = 1;
        if(str == TJS_W("higher"))
            prectick = 5;
        if(str == TJS_W("normal"))
            prectick = 10;
    }

    // draw thread num
    tjs_int drawThreadNum = 0;
    if(TVPGetCommandLine(TJS_W("-drawthread"), &opt)) {
        ttstr str(opt);
        if(str == TJS_W("auto"))
            drawThreadNum = 0;
        else
            drawThreadNum = (tjs_int)opt;
    }
    TVPDrawThreadNum = drawThreadNum;
}

//---------------------------------------------------------------------------
void TVPBeforeSystemUninit() {
    // TVPDumpHWException(); // dump cached hw exceptoin
}

//---------------------------------------------------------------------------
void TVPAfterSystemUninit() {}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
bool TVPTerminated = false;
bool TVPTerminateOnWindowClose = true;
bool TVPTerminateOnNoWindowStartup = true;
int TVPTerminateCode = 0;

//---------------------------------------------------------------------------
void TVPTerminateAsync(int code) {
    // do "A"synchronous temination of application
    TVPTerminated = true;
    TVPTerminateCode = code;

    // posting dummy message will prevent "missing WM_QUIT bug" in
    // Direct3D framework.
    if(TVPSystemControl)
        TVPSystemControl->CallDeliverAllEventsOnIdle();

    Application->Terminate();

    if(TVPSystemControl)
        TVPSystemControl->CallDeliverAllEventsOnIdle();
}

//---------------------------------------------------------------------------
void TVPTerminateSync(int code) {
    // do synchronous temination of application (never return)
    TVPSystemUninit();
    TVPExitApplication(code);
}

//---------------------------------------------------------------------------
void TVPMainWindowClosed() {
    // called from WindowIntf.cpp, caused by closing all window.
    if(TVPTerminateOnWindowClose)
        TVPTerminateAsync();
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// GetCommandLine
//---------------------------------------------------------------------------
static std::vector<std::string> *TVPGetEmbeddedOptions() { return nullptr; }

//---------------------------------------------------------------------------
static std::vector<std::string> *
TVPGetConfigFileOptions(const ttstr &filename) {
    return nullptr;
}

static ttstr TVPParseCommandLineOne(const ttstr &i) {
    // value is specified
    const tjs_char *p, *o;
    p = o = i.c_str();
    p = TJS_strchr(p, '=');

    if(p == nullptr) {
        return i + TJS_W("=yes");
    }

    p++;

    ttstr optname(o, (int)(p - o));

    if(*p == TJS_W('\'') || *p == TJS_W('\"')) {
        // as an escaped string
        tTJSVariant v;
        TJSParseString(v, &p);

        return optname + ttstr(v);
    } else {
        // as a string
        return optname + p;
    }
}

//---------------------------------------------------------------------------
std::vector<ttstr> TVPProgramArguments;
static bool TVPProgramArgumentsInit = false;
static tjs_int TVPCommandLineArgumentGeneration = 0;
static bool TVPDataPathDirectoryEnsured = false;

//---------------------------------------------------------------------------
tjs_int TVPGetCommandLineArgumentGeneration() {
    return TVPCommandLineArgumentGeneration;
}

//---------------------------------------------------------------------------
void TVPEnsureDataPathDirectory() {
    if(!TVPDataPathDirectoryEnsured) {
        TVPDataPathDirectoryEnsured = true;
        // ensure data path existence
        if(!TVPCheckExistentLocalFolder(TVPNativeDataPath.c_str())) {
            if(TVPCreateFolders(TVPNativeDataPath.c_str()))
                TVPAddImportantLog(
                    TVPFormatMessage(TVPInfoDataPathDoesNotExistTryingToMakeIt,
                                     (const tjs_char *)TVPOk));
            else
                TVPAddImportantLog(
                    TVPFormatMessage(TVPInfoDataPathDoesNotExistTryingToMakeIt,
                                     (const tjs_char *)TVPFaild));
        }
    }
}

//---------------------------------------------------------------------------
static void PushAllCommandlineArguments() {}

//---------------------------------------------------------------------------
static void PushConfigFileOptions(const std::vector<std::string> *options) {
    if(!options)
        return;
    for(unsigned int j = 0; j < options->size(); j++) {
        if((*options)[j].c_str()[0] != ';') // unless comment
            TVPProgramArguments.push_back(TVPParseCommandLineOne(
                TJS_W("-") + ttstr((*options)[j].c_str())));
    }
}

//---------------------------------------------------------------------------
static void TVPInitProgramArgumentsAndDataPath(bool stop_after_datapath_got) {
    if(!TVPProgramArgumentsInit) {
        TVPProgramArgumentsInit = true;

        // find options from self executable image
        const int num_option_layers = 3;
        std::vector<std::string> *options[num_option_layers];
        for(int i = 0; i < num_option_layers; i++)
            options[i] = nullptr;
        try {
            // read embedded options and default configuration file
            options[0] = TVPGetEmbeddedOptions();
            //			options[1] =
            // TVPGetConfigFileOptions(ApplicationSpecialPath::GetConfigFileName(ExePath()));

            // at this point, we need to push all exsting known
            // options to be able to see datapath
            PushAllCommandlineArguments();
            PushConfigFileOptions(options[1]); // has more priority
            PushConfigFileOptions(options[0]); // has lesser priority

            // read datapath
            tTJSVariant val;
            ttstr config_datapath;
            // 			if(TVPGetCommandLine(TJS_W("-datapath"),
            // &val)) 				config_datapath =
            // ((ttstr)val).AsStdString();
            TVPNativeDataPath = ApplicationSpecialPath::GetDataPathDirectory(
                config_datapath, ExePath());

            if(stop_after_datapath_got)
                return;

            // read per-user configuration file
            //			options[2] =
            // TVPGetConfigFileOptions(ApplicationSpecialPath::GetUserConfigFileName(config_datapath,
            // ExePath()));

            // push each options into option stock
            // we need to clear TVPProgramArguments first because of
            // the option priority order.
            TVPProgramArguments.clear();
            PushAllCommandlineArguments();
            PushConfigFileOptions(options[2]); // has more priority
            PushConfigFileOptions(options[1]); // has more priority
            PushConfigFileOptions(options[0]); // has lesser priority
        } catch(...) {
            for(int i = 0; i < num_option_layers; i++)
                if(options[i])
                    delete options[i];
            throw;
        }
        for(int i = 0; i < num_option_layers; i++)
            if(options[i])
                delete options[i];

        // set data path
        TVPDataPath = TVPNormalizeStorageName(TVPNativeDataPath);
        TVPAddImportantLog(TVPFormatMessage(TVPInfoDataPath, TVPDataPath));

        // set log output directory
        TVPSetLogLocation(TVPNativeDataPath);

        // increment TVPCommandLineArgumentGeneration
        TVPCommandLineArgumentGeneration++;
    }
}

//---------------------------------------------------------------------------
static void TVPDumpOptions() {
    std::vector<ttstr>::const_iterator i;
    ttstr options(TVPInfoSpecifiedOptionEarlierItemHasMorePriority);
    if(TVPProgramArguments.size()) {
        for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end();
            i++) {
            options += TJS_W(" ");
            options += *i;
        }
    } else {
        options += (const tjs_char *)TVPNone;
    }
    TVPAddImportantLog(options);
}

//---------------------------------------------------------------------------
bool TVPGetCommandLine(const tjs_char *name, tTJSVariant *value) {
    TVPInitProgramArgumentsAndDataPath(false);

    tjs_int namelen = (tjs_int)TJS_strlen(name);
    std::vector<ttstr>::const_iterator i;
    for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end(); i++) {
        if(!TJS_strncmp(i->c_str(), name, namelen)) {
            if(i->c_str()[namelen] == TJS_W('=')) {
                // value is specified
                const tjs_char *p = i->c_str() + namelen + 1;
                if(value)
                    *value = p;
                return true;
            } else if(i->c_str()[namelen] == 0) {
                // value is not specified
                if(value)
                    *value = TJS_W("yes");
                return true;
            }
        }
    }
    return false;
}

//---------------------------------------------------------------------------
void TVPSetCommandLine(const tjs_char *name, const ttstr &value) {
    //	TVPInitProgramArgumentsAndDataPath(false);

    tjs_int namelen = (tjs_int)TJS_strlen(name);
    std::vector<ttstr>::iterator i;
    for(i = TVPProgramArguments.begin(); i != TVPProgramArguments.end(); i++) {
        if(!TJS_strncmp(i->c_str(), name, namelen)) {
            if(i->c_str()[namelen] == TJS_W('=') || i->c_str()[namelen] == 0) {
                // value found
                *i = ttstr(i->c_str(), namelen) + TJS_W("=") + value;
                TVPCommandLineArgumentGeneration++;
                if(TVPCommandLineArgumentGeneration == 0)
                    TVPCommandLineArgumentGeneration = 1;
                return;
            }
        }
    }

    // value not found; insert argument into front
    TVPProgramArguments.insert(TVPProgramArguments.begin(),
                               ttstr(name) + TJS_W("=") + value);
    TVPCommandLineArgumentGeneration++;
    if(TVPCommandLineArgumentGeneration == 0)
        TVPCommandLineArgumentGeneration = 1;
}

bool TVPCheckPrintDataPath() { return false; }

bool TVPCheckAbout() { return false; }

static void TVPExecuteAsync(const std::wstring &progname) {}

static bool TVPWaitWritePermit(const std::wstring &fn) { return false; }

bool TVPExecuteUserConfig() { return false; }
