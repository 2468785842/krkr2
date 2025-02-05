#include "CustomFileUtils.h"
#include "base/win32/StorageImpl.h"
#include "environ/ConfigManager/LocaleConfigManager.h"

static bool TVPCopyFolder(const std::string &from, const std::string &to) {
    if(!TVPCheckExistentLocalFolder(to) && !TVPCreateFolders(to)) {
        return false;
    }

    bool success = true;
    TVPListDir(from, [&](const std::string &_name, int mask) {
        if(_name == "." || _name == "..")
            return;
        if(!success)
            return;
        if(mask & S_IFREG) {
            success = TVPCopyFile(from + "/" + _name, to + "/" + _name);
        } else if(mask & S_IFDIR) {
            success = TVPCopyFolder(from + "/" + _name, to + "/" + _name);
        }
    });
    return success;
}

bool TVPCopyFile(const std::string &from, const std::string &to) {
    FILE *fFrom = fopen(from.c_str(), "rb");
    if(!fFrom) { // try folder copy
        return TVPCopyFolder(from, to);
    }
    FILE *fTo = fopen(to.c_str(), "wb");
    if(!fTo) {
        fclose(fFrom);
        return false;
    }
    const int bufSize = 1 * 1024 * 1024;
    std::vector<char> buffer;
    buffer.resize(bufSize);
    size_t index = 0;
    while((index = fread(&buffer.front(), 1, bufSize, fFrom))) {
        fwrite(&buffer.front(), 1, index, fTo);
    }
    fclose(fFrom);
    fclose(fTo);
    return true;
}
