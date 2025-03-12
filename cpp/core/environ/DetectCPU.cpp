//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000-2007 W.Dee <dee@kikyou.info> and
   contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// CPU idetification / features detection routine
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "cpu_types.h"
#include "DebugIntf.h"
#include "SysInitIntf.h"

#include "ThreadIntf.h"
#include "Exception.h"

/*
        Note: CPU clock measuring routine is in EmergencyExit.cpp,
   reusing hot-key watching thread.
*/

//---------------------------------------------------------------------------
extern "C" {
tjs_uint32 TVPCPUType = 0; // CPU type
tjs_uint32 TVPCPUFeatures = 0;
}

static bool TVPCPUChecked = false;
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPGetCPUTypeForOne
//---------------------------------------------------------------------------
static void TVPGetCPUTypeForOne() {
    try {
        TVPCPUFeatures = 0;

        // TVPCheckCPU(); // in detect_cpu.nas
    } catch(... /*EXCEPTION_EXECUTE_HANDLER*/) {
        // exception had been ocured
        throw Exception("CPU check failure.");
    }

    // check OSFXSR
    // 	if(TVPCPUFeatures & TVP_CPU_HAS_SSE)
    // 	{
    // 		__try
    // 		{
    // 			__emit__(0x0f, 0x57, 0xc0); // xorps xmm0, xmm0 (SSE)
    // 		}
    // 		__except(EXCEPTION_EXECUTE_HANDLER)
    // 		{
    // 			// exception had been ocured
    // 			// execution of 'xorps' is failed (XMM registers not
    // available) 			TVPCPUFeatures &=~ TVP_CPU_HAS_SSE;
    // TVPCPUFeatures
    // &=~ TVP_CPU_HAS_SSE2;
    // 		}
    // 	}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
static ttstr TVPDumpCPUFeatures(tjs_uint32 features) {
    ttstr ret;
    // #define TVP_DUMP_CPU(x, n) { ret += TJS_W("  ") TJS_W(n);  \
// 	if(features & x) ret += TJS_W(":yes"); else ret += TJS_W(":no"); }
    //
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_FPU, "FPU");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_MMX, "MMX");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_3DN, "3DN");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_SSE, "SSE");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_CMOV, "CMOVcc");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_E3DN, "E3DN");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_EMMX, "EMMX");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_SSE2, "SSE2");
    // 	TVP_DUMP_CPU(TVP_CPU_HAS_TSC, "TSC");

    return ret;
}
//---------------------------------------------------------------------------
static ttstr TVPDumpCPUInfo(tjs_int cpu_num) {
    // dump detected cpu type
    ttstr features(TJS_W("(info) CPU #") + ttstr(cpu_num) + TJS_W(" : "));

    features += TVPDumpCPUFeatures(TVPCPUFeatures);

    tjs_uint32 vendor = TVPCPUFeatures & TVP_CPU_VENDOR_MASK;

    // #undef TVP_DUMP_CPU
    // #define TVP_DUMP_CPU(x, n) { \
// 	if(vendor == x) features += TJS_W("  ") TJS_W(n); }
    //
    // 	TVP_DUMP_CPU(TVP_CPU_IS_INTEL, "Intel");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_AMD, "AMD");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_IDT, "IDT");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_CYRIX, "Cyrix");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_NEXGEN, "NexGen");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_RISE, "Rise");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_UMC, "UMC");
    // 	TVP_DUMP_CPU(TVP_CPU_IS_TRANSMETA, "Transmeta");
    //
    // 	TVP_DUMP_CPU(TVP_CPU_IS_UNKNOWN, "Unknown");
    //
    // #undef TVP_DUMP_CPU

    //	features += TJS_W("(") + ttstr((const tjs_nchar
    //*)TVPCPUVendor) +
    // TJS_W(")");

    // 	if(TVPCPUName[0]!=0)
    // 		features += TJS_W(" [") + ttstr((const tjs_nchar
    // *)TVPCPUName) + TJS_W("]");

    // 	features += TJS_W("  CPUID(1)/EAX=") +
    // TJSInt32ToHex(TVPCPUID1_EAX); 	features += TJS_W("
    // CPUID(1)/EBX=") + TJSInt32ToHex(TVPCPUID1_EBX);

    TVPAddImportantLog(features);

    // 	if(((TVPCPUID1_EAX >> 8) & 0x0f) <= 4)
    // 		throw Exception("CPU check failure: CPU family 4 or lesser
    // is not supported\r\n"+ 		features.AsAnsiString());

    return features;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPDetectCPU
//---------------------------------------------------------------------------
static void TVPDisableCPU(tjs_uint32 featurebit, const tjs_char *name) {}

void TVPDetectCPU() {
    if(TVPCPUChecked)
        return;
    TVPCPUChecked = true;

#ifdef __APPLE__
    // must be iOS
    TVPCPUFeatures |= TVP_CPU_FAMILY_ARM | TVP_CPU_HAS_NEON;
#endif

    tjs_uint32 features = 0;
    features = (TVPCPUFeatures & TVP_CPU_FEATURE_MASK);
    TVPCPUType &= ~TVP_CPU_FEATURE_MASK;
    TVPCPUType |= features;

    TVPDisableCPU(TVP_CPU_HAS_NEON, TJS_W("-cpuneon"));
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// jpeg and png loader support functions
//---------------------------------------------------------------------------
unsigned long MMXReady = 0;
extern "C" {
void CheckMMX() {
    TVPDetectCPU();
    MMXReady = TVPCPUType & TVP_CPU_HAS_MMX;
}
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TVPGetCPUType
//---------------------------------------------------------------------------
tjs_uint32 TVPGetCPUType() {
    TVPDetectCPU();
    return TVPCPUType;
}
//---------------------------------------------------------------------------
