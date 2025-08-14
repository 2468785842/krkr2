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

    /*==================== 文件基础操作 ====================*/

    static tjs_error exportFile(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 2) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取源文件路径和目标路径
        ttstr srcPath(*param[0]);
        ttstr dstPath(*param[1]);

        // 打开源文件
        tTJSBinaryStream* srcStream = TVPCreateStream(srcPath, TJS_BS_READ);
        if (!srcStream) {
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 创建目标文件
        tTJSBinaryStream* dstStream = TVPCreateStream(dstPath, TJS_BS_WRITE);
        if (!dstStream) {
            delete srcStream;
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 读取并写入数据
        const tjs_uint bufferSize = 4096;
        char buffer[bufferSize];
        tjs_uint bytesRead;
        while ((bytesRead = srcStream->Read(buffer, bufferSize)) > 0) {
            dstStream->Write(buffer, bytesRead);
        }

        // 释放资源
        delete srcStream;
        delete dstStream;

        if (r) *r = true;
        return TJS_S_OK;
    }

    static tjs_error deleteFile(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 1) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取文件路径
        ttstr filePath(*param[0]);

        // 删除文件
        bool success = TVPRemoveFile(filePath);

        if (r) *r = success;
        return TJS_S_OK;
    }

    static tjs_error truncateFile(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 2) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取文件路径和截断大小
        ttstr filePath(*param[0]);
        tjs_uint64 size = param[1]->AsInteger();

        // 打开文件
        tTJSBinaryStream* stream = TVPCreateStream(filePath, TJS_BS_READ);
        if (!stream) {
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 截断文件
        stream->Seek(size, TJS_BS_SEEK_SET);
        stream->SetEndOfStorage();

        // 释放资源
        delete stream;

        if (r) *r = true;
        return TJS_S_OK;
    }

    static tjs_error moveFile(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 2) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取源文件路径和目标路径
        ttstr srcPath(*param[0]);
        ttstr dstPath(*param[1]);

        // 复制文件
        bool success = TVPCreateStream(dstPath, TJS_BS_WRITE);
        if (!success) {
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 删除源文件
        success = TVPRemoveFile(srcPath);

        if (r) *r = success;
        return TJS_S_OK;
    }


    static tjs_error copyFile(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 2) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取源文件路径和目标路径
        ttstr srcPath(*param[0]);
        ttstr dstPath(*param[1]);

        // 打开源文件
        tTJSBinaryStream* srcStream = TVPCreateStream(srcPath, TJS_BS_READ);
        if (!srcStream) {
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 创建目标文件
        tTJSBinaryStream* dstStream = TVPCreateStream(dstPath, TJS_BS_WRITE );
        if (!dstStream) {
            delete srcStream; // 释放源文件流
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 读取并写入数据
        const tjs_uint bufferSize = 4096;
        char buffer[bufferSize];
        tjs_uint bytesRead;
        while ((bytesRead = srcStream->Read(buffer, bufferSize)) > 0) {
            dstStream->Write(buffer, bytesRead);
        }

        // 释放资源
        delete srcStream; // 释放源文件流
        delete dstStream; // 释放目标文件流

        if (r) *r = true;
        return TJS_S_OK;
    }

    static tjs_error copyFileNoNormalize(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 2) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取源文件路径和目标路径
        ttstr srcPath(*param[0]);
        ttstr dstPath(*param[1]);

        // 打开源文件
        tTJSBinaryStream* srcStream = TVPCreateStream(srcPath, TJS_BS_READ);
        if (!srcStream) {
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 创建目标文件
        tTJSBinaryStream* dstStream = TVPCreateStream(dstPath, TJS_BS_WRITE );
        if (!dstStream) {
            delete srcStream; // 释放源文件流
            if (r) *r = false;
            return TJS_S_OK;
        }

        // 读取并写入数据
        const tjs_uint bufferSize = 4096;
        char buffer[bufferSize];
        tjs_uint bytesRead;
        while ((bytesRead = srcStream->Read(buffer, bufferSize)) > 0) {
            dstStream->Write(buffer, bytesRead);
        }

        // 释放资源
        delete srcStream; // 释放源文件流
        delete dstStream; // 释放目标文件流

        if (r) *r = true;
        return TJS_S_OK;
    }

    /*==================== 目录列表 ====================*/
    static tjs_error dirlist(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        // 参数检查
        if (numparams < 1)
            return TJS_E_BADPARAMCOUNT;

        // 获取目录路径
        ttstr dir(*param[0]);

        // 检查目录路径是否以 '/' 结尾
        if (dir.GetLastChar() != TJS_W('/'))
            TVPThrowExceptionMessage(TJS_W("'/' must be specified at the end of given directory name."));

        // 将目录路径转换为 OS 原生格式
        dir = TVPNormalizeStorageName(dir);

        // 创建数组对象
        iTJSDispatch2* array = TJSCreateArrayObject();
        if (!r)
            return TJS_S_OK;

        try {
            tTJSArrayNI* ni;
            array->NativeInstanceSupport(TJS_NIS_GETINSTANCE, TJSGetArrayClassID(), (iTJSNativeInstance**)&ni);

            // 获取目录中的文件和子目录
            TVPGetLocalName(dir);
            TVPGetLocalFileListAt(dir, [ni](const ttstr& name, tTVPLocalFileInfo* s) {
                if (s->Mode & (S_IFREG | S_IFDIR)) {
                    ni->Items.emplace_back(name);
                }
            });

            // 将数组对象赋值给结果
            *r = tTJSVariant(array, array);
            array->Release();
        } catch (...) {
            array->Release();
            throw;
        }

        // 返回成功
        return TJS_S_OK;
    }
    static tjs_error dirlistEx(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*){
        return TJS_S_OK;
    }

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

    // selectDirectory: 设置当前工作目录
    static tjs_error selectDirectory(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 1) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取目录路径
        ttstr dirPath(*param[0]);

        // 规范化路径
        TVPPreNormalizeStorageName(dirPath);

        // 设置当前工作目录
        TVPSetCurrentDirectory(dirPath);

        return TJS_S_OK;
    }

    // isExistentDirectory: 检查目录是否存在
    static tjs_error isExistentDirectory(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 1) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取目录路径
        ttstr dirPath(*param[0]);

        // 规范化路径
        TVPPreNormalizeStorageName(dirPath);

        // 检查目录是否存在
        bool exists = TVPCheckExistentLocalFolder(dirPath);

        // 将结果存储在 r 中
        if (r) {
            *r = exists;
        }

        return TJS_S_OK;
    }

    // isExistentStorageNoSearchNoNormalize: 检查存储路径是否存在（不进行路径规范化和自动搜索路径的查找）
    static tjs_error isExistentStorageNoSearchNoNormalize(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
    {
        if (numparams < 1) {
            return TJS_E_BADPARAMCOUNT;
        }

        // 获取存储路径
        ttstr path(*param[0]);

        // 检查存储路径是否存在
        bool exists = TVPIsExistentStorageNoSearchNoNormalize(path);

        // 将结果存储在 r 中
        if (r) {
            *r = exists;
        }

        return TJS_S_OK;
    }

    /*==================== 时间戳 ====================*/
    static tjs_error  getTime(tTJSVariant* r, tjs_int, tTJSVariant**, iTJSDispatch2*)
    {
        std::string time = std::to_string(getTick());
        // 将结果存储在 r 中
        if (r) {
            *r = tTJSVariant(time.c_str());
        }
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
    //Property(TJS_W("currentPath"), &Storages::get_currentPath, &Storages::set_currentPath);
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

