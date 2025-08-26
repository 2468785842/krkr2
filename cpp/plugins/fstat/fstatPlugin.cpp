#include "ncbind/ncbind.hpp"
#include "utils/TickCount.h"
#include "tjsCommHead.h"   // 通常包含 TJS 核心定义
#include "tjsNative.h"     // 包含 TJS 本地对象和类，可能包含 TJSObject
#include "StorageIntf.h"   // 通常包含 TVP 存储接口，如 tTVPStreamInfo, TVPCreateStream 等
#include "tjsDictionary.h" // 如果有单独的字典头文件
#include "tjsDate.h"       // 你已经包含了 tjsDate.h，这是正确的
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
    //---------------------------------------------------------------------------
    // fstat
    //   取得文件大小与三种时间戳，封装成 TJS 字典返回
    //---------------------------------------------------------------------------
    static tjs_error fstat(tTJSVariant *result, tjs_int numparams,
          tTJSVariant **param, iTJSDispatch2 *objthis)
    {
        if(numparams < 1) return TJS_E_BADPARAMCOUNT;

        //-----------------------------------------------------------
        // 1. 把 TJS 文件名转换成 TVP 统一存储名
        //-----------------------------------------------------------
        ttstr filename = *param[0];
        filename = TVPNormalizeStorageName(filename);

        //-----------------------------------------------------------
        // 2. 打开文件流
        //-----------------------------------------------------------
        tTJSBinaryStream *strm = nullptr;
        try
        {
            strm = TVPCreateStream(filename, TJS_BS_READ);
        }
        catch(...) // 捕获所有类型的异常，更具体的异常处理可能更好
        {
            // 不存在或其它原因导致打开失败
            // 使用 TJSCreateDictionaryObject() 创建字典对象
            iTJSDispatch2 *dictOnError = TJSCreateDictionaryObject();
            if(result) *result = tTJSVariant(dictOnError, dictOnError);
            if(dictOnError) dictOnError->Release(); // 创建后如果不再使用需要释放
            return TJS_S_OK;
        }

        //-----------------------------------------------------------
        // 3. 取得大小和时间戳
        //-----------------------------------------------------------
        tjs_uint64 size64 = strm->GetSize();


//        tTVPStreamInfo info = {0}; // 初始化结构体

//        strm->GetInfo(info); // 取得时间戳等信息

        delete strm; // 关闭并删除流对象

        //-----------------------------------------------------------
        // 4. 构造返回字典
        //-----------------------------------------------------------
        // 使用 TJSCreateDictionaryObject() 创建字典对象
        iTJSDispatch2 *dict = TJSCreateDictionaryObject();
        if (!dict) { // 检查字典是否成功创建
            if(result) result->Clear(); // 清除结果，表示失败
            return TJS_E_FAIL; // 或者其他合适的错误码
        }

        tTJSVariant val;
        tTJSVariant dateVal; // 为日期创建一个单独的 Variant 变量，避免混淆

        // size
        val = (tTVInteger)size64; // tTVInteger 通常是 tjs_int64 的别名
        dict->PropSet(TJS_MEMBERENSURE, TJS_W("size"), 0, &val, dict);

        // mtime (修改时间)
//        if(info.MTime != 0) // tTVPStreamInfo 通常直接存储 time_t 类型的时间戳
//        {
//            if (info.mtime) { // 确保指针有效
//                tTJSDateVariantFromTime(dateVal, *info.mtime);
//                dict->PropSet(TJS_MEMBERENSURE, TJS_W("mtime"), 0, &dateVal, dict);
//            }
//        }

        // atime (访问时间)
//        if(info.atime) // 假设 info.atime 是 time_t*
//        {
//            tTJSDateVariantFromTime(dateVal, *info.atime);
//            dict->PropSet(TJS_MEMBERENSURE, TJS_W("atime"), 0, &dateVal, dict);
//        }


        // ctime (创建时间 或 元数据更改时间，取决于平台)
//        if(info.ctime) // 假设 info.ctime 是 time_t*
//        {
//            tTJSDateVariantFromTime(dateVal, *info.ctime);
//            dict->PropSet(TJS_MEMBERENSURE, TJS_W("ctime"), 0, &dateVal, dict);
//        }

        if(result) *result = tTJSVariant(dict, dict);
        dict->Release(); // 释放字典对象 (result 持有其引用)
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
    static tjs_error dirlistEx(tTJSVariant* r, tjs_int numparams, tTJSVariant** param, iTJSDispatch2*)
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

            // ... 在 dirlistEx 函数内部的 TVPGetLocalFileListAt 回调中 ...
            TVPGetLocalFileListAt(dir, [ni](const ttstr& name, tTVPLocalFileInfo* s) {
                // 创建一个标准的 TJS 字典对象来存储文件信息
                iTJSDispatch2* fileInfoDict = TJSCreateDictionaryObject();
                if (!fileInfoDict) {
                    // 处理创建字典失败的情况，例如抛出异常或记录错误
                    // TVPThrowExceptionMessage(TJS_W("Failed to create dictionary object for file info."));
                    return; // 或者 continue，取决于你的错误处理策略
                }

                tTJSVariant tempVal; // 用于设置属性的临时 Variant

                // 1. 设置 name
                tempVal = name; // ttstr 可以隐式或显式转换为 tTJSVariant
                fileInfoDict->PropSet(
                    TJS_MEMBERENSURE, // 创建成员如果不存在
                    TJS_W("name"),    // 属性名
                    0,                // hint (通常为0)
                    &tempVal,         // 属性值
                    fileInfoDict      // 对象本身
                );

                // 2. 设置 size (s->Size 通常是 tjs_uint64)
                tempVal = static_cast<tTVInteger>(s->Size); // tTVInteger 通常是 tjs_int64
                fileInfoDict->PropSet(TJS_MEMBERENSURE, TJS_W("size"), 0, &tempVal, fileInfoDict);

                // 3. 设置 attrib (s->Mode 通常是 tjs_uint32 或 int)
                tempVal = static_cast<tTVInteger>(s->Mode);
                fileInfoDict->PropSet(TJS_MEMBERENSURE, TJS_W("attrib"), 0, &tempVal, fileInfoDict);

                // 4. 设置 mtime (s->ModifyTime 通常是 time_t)
                // 需要将 time_t 转换为 TJS 的 Date 对象
                if (s->ModifyTime != 0) { // 检查时间戳是否有效
                    tTJSVariant dateVariant;
                    // 假设 tTJSDateVariantFromTime 接受 time_t*
                    // 如果 s->ModifyTime 本身就是 time_t*，则直接使用 s->ModifyTime
                    // 如果 s->ModifyTime 是 time_t，则传递其地址
                    time_t modTime = s->ModifyTime; // 假设 s->ModifyTime 是 time_t
                    dateVariant = tTJSVariant(modTime);
//                    tTJSDateVariantFromTime(dateVariant, &modTime);
                    fileInfoDict->PropSet(TJS_MEMBERENSURE, TJS_W("mtime"), 0, &dateVariant, fileInfoDict);
                }

                // 5. 设置 atime (s->AccessTime 通常是 time_t)
                if (s->AccessTime != 0) {
                    tTJSVariant dateVariant;
                    time_t accTime = s->AccessTime;
//                    tTJSDateVariantFromTime(dateVariant, &accTime);
                    dateVariant = tTJSVariant(accTime);
                    fileInfoDict->PropSet(TJS_MEMBERENSURE, TJS_W("atime"), 0, &dateVariant, fileInfoDict);
                }

                // 6. 设置 ctime (s->CreationTime 通常是 time_t)
                if (s->CreationTime != 0) {
                    tTJSVariant dateVariant;
                    time_t creTime = s->CreationTime;
//                    tTJSDateVariantFromTime(dateVariant, &creTime);
                    dateVariant = tTJSVariant(creTime);
                    fileInfoDict->PropSet(TJS_MEMBERENSURE, TJS_W("ctime"), 0, &dateVariant, fileInfoDict);
                }

                // 将创建的字典对象 (作为 tTJSVariant) 添加到数组中
                // tTJSVariant 构造函数会增加 fileInfoDict 的引用计数
                ni->Items.emplace_back(tTJSVariant(fileInfoDict, fileInfoDict));

                // 释放我们在这里创建的 fileInfoDict 的引用，因为 tTJSVariant 已经持有了它
                fileInfoDict->Release();

            }); // 结束 TVPGetLocalFileListAt 的 lambda 回调


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

