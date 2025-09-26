#pragma once
#include "tjs.h"
#include "StorageIntf.h"

class ApplicationSpecialPath {
public:
    static ttstr ReplaceStringAll(ttstr src, const ttstr &target,
                                  const ttstr &dest) {
        src.Replace(target, dest);
        return src;
    }

    static ttstr GetDataPathDirectory(const ttstr &datapath,
                                      const ttstr &exename) {
        ttstr nativeDataPath = TVPGetAppPath();
        TVPGetLocalName(nativeDataPath);
        nativeDataPath += "/savedata/";
        return nativeDataPath;
    }
};