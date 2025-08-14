//
// Created by 33298 on 2025/8/14.
//
#define NCB_MODULE_NAME TJS_W("krkrsteam.dll")

#include "ncbind.hpp"
#include "tjs.h"

struct SteamEx {
    /* Steam Cloud */
    static tjs_error getCloudQuota   (tTJSVariant *r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error getCloudFileCount(tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error getCloudFileInfo(tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=tTJSVariant();}return TJS_S_OK;}
    static tjs_error deleteCloudFile (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error copyCloudFile   (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}

    /* Screenshots */
    static tjs_error triggerScreenshot(tTJSVariant*,tjs_int,tTJSVariant**,iTJSDispatch2*){return TJS_S_OK;}
    static tjs_error hookScreenshots   (tTJSVariant*,tjs_int,tTJSVariant**,iTJSDispatch2*){return TJS_S_OK;}
    static tjs_error writeScreenshot   (tTJSVariant*,tjs_int,tTJSVariant**,iTJSDispatch2*){return TJS_S_OK;}

    /* Broadcast */
    static tjs_error isBroadcasting    (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error hookBroadcasting  (tTJSVariant*,tjs_int,tTJSVariant**,iTJSDispatch2*){return TJS_S_OK;}

    /* DLC */
    static tjs_error isIsSubscribedApp (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error ssDlcInstalled    (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error getDLCCount       (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=(tTVInteger)0;}return TJS_S_OK;}
    static tjs_error getDLCData        (tTJSVariant*r,tjs_int,tTJSVariant**,iTJSDispatch2*){if(r){*r=tTJSVariant();}return TJS_S_OK;}
};

NCB_ATTACH_CLASS(SteamEx, Steam) {
    RawCallback("getCloudQuota",    &SteamEx::getCloudQuota,    TJS_STATICMEMBER);
    RawCallback("getCloudFileCount",&SteamEx::getCloudFileCount,TJS_STATICMEMBER);
    RawCallback("getCloudFileInfo", &SteamEx::getCloudFileInfo, TJS_STATICMEMBER);
    RawCallback("deleteCloudFile",  &SteamEx::deleteCloudFile,  TJS_STATICMEMBER);
    RawCallback("copyCloudFile",    &SteamEx::copyCloudFile,    TJS_STATICMEMBER);

    RawCallback("triggerScreenshot",&SteamEx::triggerScreenshot,TJS_STATICMEMBER);
    RawCallback("hookScreenshots",  &SteamEx::hookScreenshots,  TJS_STATICMEMBER);
    RawCallback("writeScreenshot",  &SteamEx::writeScreenshot,  TJS_STATICMEMBER);

    RawCallback("isBroadcasting",   &SteamEx::isBroadcasting,   TJS_STATICMEMBER);
    RawCallback("hookBroadcasting", &SteamEx::hookBroadcasting, TJS_STATICMEMBER);

    RawCallback("isIsSubscribedApp",&SteamEx::isIsSubscribedApp,TJS_STATICMEMBER);
    RawCallback("ssDlcInstalled",   &SteamEx::ssDlcInstalled,   TJS_STATICMEMBER);
    RawCallback("getDLCCount",      &SteamEx::getDLCCount,      TJS_STATICMEMBER);
    RawCallback("getDLCData",       &SteamEx::getDLCData,       TJS_STATICMEMBER);
}