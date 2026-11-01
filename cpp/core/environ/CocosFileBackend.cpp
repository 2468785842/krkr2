#include "CocosFileBackend.h"

#include "TVPPlatform.h"
#include "UtilStreams.h"

#include <cocos/cocos2d.h>

//---------------------------------------------------------------------------
std::string CocosFileBackend::ResolvePath(const ttstr &path) {
    auto *fileUtils = cocos2d::FileUtils::getInstance();
    const std::string narrow = path.AsNarrowStdString();
    const std::string full = fileUtils->fullPathForFilename(narrow);
    return full.empty() ? narrow : full;
}

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream>
CocosFileBackend::Open(const ttstr &storageName, const ttstr &localName,
                       tjs_uint32 flags) {
    if((flags & TJS_BS_ACCESS_MASK) != TJS_BS_READ)
        return nullptr;
    const ttstr &path = localName.IsEmpty() ? storageName : localName;
    return OpenRead(path);
}

//---------------------------------------------------------------------------
std::unique_ptr<tTJSBinaryStream>
CocosFileBackend::OpenRead(const ttstr &path) {
    auto *fileUtils = cocos2d::FileUtils::getInstance();
    const std::string resolved = ResolvePath(path);
    if(!fileUtils->isFileExist(resolved))
        return nullptr;

    cocos2d::Data data = fileUtils->getDataFromFile(resolved);
    if(data.isNull() || data.getSize() == 0)
        return nullptr;

    auto stream = std::make_unique<tTVPMemoryStream>();
    stream->Write(data.getBytes(), static_cast<tjs_uint>(data.getSize()));
    stream->SetPosition(0);
    return stream;
}

//---------------------------------------------------------------------------
bool CocosFileBackend::Exists(const ttstr &path) {
    auto *fileUtils = cocos2d::FileUtils::getInstance();
    const std::string resolved = ResolvePath(path);
    return fileUtils->isFileExist(resolved) ||
        fileUtils->isDirectoryExist(resolved);
}
