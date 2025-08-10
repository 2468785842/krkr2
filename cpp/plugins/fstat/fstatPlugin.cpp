#include "ncbind/ncbind.hpp"
#include "utils/TickCount.h"
#include <sys/stat.h>
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#   include <windows.h>
#   include <direct.h>
#   include <Shlobj.h>
#else
#   include <unistd.h>
#   include <dirent.h>
#endif
#define NCB_MODULE_NAME TJS_W("fstat.dll")
// ------------- 工具函数 -------------
static ttstr TVPGetPlacedPath(const ttstr& f) { return TVPGetPlacedPath(f); } // 官方已有
static tjs_int64 getTick() { return TVPGetTickCount(); }                     // 官方已有

struct Storages
{
    /*==================== 属性 ====================*/
    static tjs_error  get_currentPath(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    {
        if (r) *r = TJS_W("");   // 空字符串占位
        return TJS_S_OK;
    }
    static tjs_error  set_currentPath(tTJSVariant*, tjs_int n, tTJSVariant** p, iTJSDispatch2*)
    {
        return TJS_S_OK;         // 假装成功
    }

    /*==================== 文件信息 ====================*/
    static tjs_error  fstat(tTJSVariant* r, tjs_int n, tTJSVariant** p, iTJSDispatch2*)
    {
        // if (r) *r = TJSObject::CreateDictionary();   // 空字典
        return TJS_S_OK;
    }

    /*==================== 文件/目录基础操作 ====================*/
    static tjs_error  exportFile(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    static tjs_error  deleteFile(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  truncateFile(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  moveFile(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  copyFile(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  copyFileNoNormalize(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    /*==================== 目录列表 ====================*/
    static tjs_error  dirlist(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJSCreateArrayObject(); return TJS_S_OK; }

    static tjs_error  dirlistEx(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJSCreateArrayObject(); return TJS_S_OK; }

    static tjs_error  dirtree(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJSCreateArrayObject(); return TJS_S_OK; }

    /*==================== 目录增删 ====================*/
    static tjs_error  removeDirectory(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  createDirectory(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  createDirectoryNoNormalize(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    static tjs_error  changeDirectory(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = true; return TJS_S_OK; }

    /*==================== 属性操作 ====================*/
    static tjs_error  setFileAttributes(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    static tjs_error  resetFileAttributes(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    static tjs_error  getFileAttributes(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = 0; return TJS_S_OK; }

    /*==================== 对话框 / 存在检查 ====================*/
    static tjs_error  selectDirectory(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    static tjs_error  isExistentDirectory(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = -1; return TJS_S_OK; }

    static tjs_error  isExistentStorageNoSearchNoNormalize(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = false; return TJS_S_OK; }

    /*==================== 时间戳 ====================*/
    static tjs_error  getTime(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    {
        // if (r) *r = TJSObject::CreateDictionary(); return TJS_S_OK;
        return TJS_S_OK;
    }

    static tjs_error  setTime(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    static tjs_error  getLastModifiedFileTime(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = (tjs_int64)0; return TJS_S_OK; }

    static tjs_error  setLastModifiedFileTime(tTJSVariant*, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { return TJS_S_OK; }

    /*==================== 杂项 ====================*/
    static tjs_error  getDisplayName(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJS_W(""); return TJS_S_OK; }

    static tjs_error  getMD5HashString(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJS_W("d41d8cd98f00b204e9800998ecf8427e"); return TJS_S_OK; }

    static tjs_error  searchPath(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJS_W(""); return TJS_S_OK; }

    static tjs_error  getTemporaryName(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    { if (r) *r = TJS_W("temp.tmp"); return TJS_S_OK; }
};

/* ---------- 注册 ---------- */
NCB_ATTACH_CLASS(Storages, Storages)
{
    Property(TJS_W("currentPath"), &Storages::get_currentPath, &Storages::set_currentPath);
    RawCallback(TJS_W("fstat"),            &Storages::fstat,            TJS_STATICMEMBER);
    RawCallback(TJS_W("exportFile"),       &Storages::exportFile,       TJS_STATICMEMBER);
    RawCallback(TJS_W("deleteFile"),       &Storages::deleteFile,       TJS_STATICMEMBER);
    RawCallback(TJS_W("truncateFile"),     &Storages::truncateFile,     TJS_STATICMEMBER);
    RawCallback(TJS_W("moveFile"),         &Storages::moveFile,         TJS_STATICMEMBER);
    RawCallback(TJS_W("dirlist"),          &Storages::dirlist,          TJS_STATICMEMBER);
    RawCallback(TJS_W("dirlistEx"),        &Storages::dirlistEx,        TJS_STATICMEMBER);
    RawCallback(TJS_W("dirtree"),          &Storages::dirtree,          TJS_STATICMEMBER);
    RawCallback(TJS_W("removeDirectory"),  &Storages::removeDirectory,  TJS_STATICMEMBER);
    RawCallback(TJS_W("createDirectory"),  &Storages::createDirectory,  TJS_STATICMEMBER);
    RawCallback(TJS_W("createDirectoryNoNormalize"), &Storages::createDirectoryNoNormalize, TJS_STATICMEMBER);
    RawCallback(TJS_W("changeDirectory"),  &Storages::changeDirectory,  TJS_STATICMEMBER);
    RawCallback(TJS_W("setFileAttributes"),   &Storages::setFileAttributes,   TJS_STATICMEMBER);
    RawCallback(TJS_W("resetFileAttributes"), &Storages::resetFileAttributes, TJS_STATICMEMBER);
    RawCallback(TJS_W("getFileAttributes"),   &Storages::getFileAttributes,   TJS_STATICMEMBER);
    RawCallback(TJS_W("selectDirectory"),     &Storages::selectDirectory,     TJS_STATICMEMBER);
    RawCallback(TJS_W("isExistentDirectory"), &Storages::isExistentDirectory, TJS_STATICMEMBER);
    RawCallback(TJS_W("isExistentStorageNoSearchNoNormalize"), &Storages::isExistentStorageNoSearchNoNormalize, TJS_STATICMEMBER);
    RawCallback(TJS_W("getTime"),                 &Storages::getTime,                 TJS_STATICMEMBER);
    RawCallback(TJS_W("setTime"),                 &Storages::setTime,                 TJS_STATICMEMBER);
    RawCallback(TJS_W("getLastModifiedFileTime"), &Storages::getLastModifiedFileTime, TJS_STATICMEMBER);
    RawCallback(TJS_W("setLastModifiedFileTime"), &Storages::setLastModifiedFileTime, TJS_STATICMEMBER);
    RawCallback(TJS_W("getDisplayName"),        &Storages::getDisplayName,        TJS_STATICMEMBER);
    RawCallback(TJS_W("getMD5HashString"),      &Storages::getMD5HashString,      TJS_STATICMEMBER);
    RawCallback(TJS_W("searchPath"),            &Storages::searchPath,            TJS_STATICMEMBER);
    RawCallback(TJS_W("getTemporaryName"),      &Storages::getTemporaryName,      TJS_STATICMEMBER);
}

