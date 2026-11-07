#include "tjsCommHead.h"

#include <dirent.h>
#include <functional>
#include <sys/stat.h>

#include "IFileBackend.h"
#include "MsgIntf.h"
#include "TVPPlatform.h"
#include "Platform.h"
#include "StorageImpl.h"
#include "SysInitIntf.h"

#ifdef WIN32
#include <io.h>
#endif

//---------------------------------------------------------------------------
// tTVPFileMedia
//---------------------------------------------------------------------------
class tTVPFileMedia : public iTVPStorageMedia {
    tjs_uint RefCount;

public:
    tTVPFileMedia() { RefCount = 1; }

    ~tTVPFileMedia() override {}

    void AddRef() override { RefCount++; }

    void Release() override {
        if(RefCount == 1)
            delete this;
        else
            RefCount--;
    }

    void GetName(ttstr &name) override { name = TJS_W("file"); }

    void NormalizeDomainName(ttstr &name) override;

    void NormalizePathName(ttstr &name) override;

    bool CheckExistentStorage(const ttstr &name) override;

    tTJSBinaryStream *Open(const ttstr &name, tjs_uint32 flags) override;

    void GetListAt(const ttstr &name, iTVPStorageLister *lister) override;

    void GetLocallyAccessibleName(ttstr &name) override;

    void GetLocalName(ttstr &name);
};

//---------------------------------------------------------------------------
void tTVPFileMedia::NormalizeDomainName(ttstr &name) {
    // normalize domain name
    // make all characters small
    tjs_char *p = name.Independ();
    while(*p) {
        if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
            *p += TJS_W('a') - TJS_W('A');
        p++;
    }
}

//---------------------------------------------------------------------------
void tTVPFileMedia::NormalizePathName(ttstr &name) {
    // normalize path name
    // make all characters small
    tjs_char *p = name.Independ();
    while(*p) {
        if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
            *p += TJS_W('a') - TJS_W('A');
        p++;
    }
}

//---------------------------------------------------------------------------
bool tTVPFileMedia::CheckExistentStorage(const ttstr &name) {
    if(name.IsEmpty())
        return false;

    if(!TVPGetFileBackend())
        TVPInitFileBackend();

    IFileBackend *backend = TVPGetFileBackend();
    // 与 Open 一致：优先 IFileBackend（Cocos 打包资源等）
    if(backend && backend->Exists(name))
        return true;

    ttstr localName(name);
    GetLocalName(localName);

    if(backend && backend->Exists(localName))
        return true;

    return TVPCheckExistentLocalPath(localName);
}

//---------------------------------------------------------------------------
tTJSBinaryStream *tTVPFileMedia::Open(const ttstr &name, tjs_uint32 flags) {
    // open storage named "name".
    // currently only local/network(by OS) storage systems are
    // supported.
    if(name.IsEmpty())
        TVPThrowExceptionMessage(TVPCannotOpenStorage, TJS_W("\"\""));

    ttstr origname = name;
    ttstr _name(name);
    GetLocalName(_name);

    if(!TVPGetFileBackend())
        TVPInitFileBackend();

    if(auto stream = TVPGetFileBackend()->Open(origname, _name, flags))
        return stream.release();

    TVPThrowExceptionMessage(TVPCannotOpenStorage, origname);
    return nullptr;
}

//---------------------------------------------------------------------------
void TVPGetLocalFileListAt(
    const ttstr &name,
    const std::function<void(const ttstr &, tTVPLocalFileInfo *)> &cb) {
    DIR *dirp;
    dirent *direntp;
    tTVP_stat stat_buf;
    std::string folder(name.AsStdString());
    if((dirp = opendir(folder.c_str()))) {
        while((direntp = readdir(dirp)) != nullptr) {
            std::string fullpath = folder + "/" + direntp->d_name;
            if(!TVP_stat(fullpath.c_str(), stat_buf))
                continue;
            ttstr file(direntp->d_name);
            if(file.length() <= 2) {
                if(file == TJS_W(".") || file == TJS_W(".."))
                    continue;
            }
            tjs_char *p = file.Independ();
            while(*p) {
                // make all characters small
                if(*p >= TJS_W('A') && *p <= TJS_W('Z'))
                    *p += TJS_W('a') - TJS_W('A');
                p++;
            }
            tTVPLocalFileInfo info;
            info.NativeName = direntp->d_name;
            info.Mode = stat_buf.st_mode;
            info.Size = stat_buf.st_size;
            info.AccessTime = stat_buf.st_atime;
            info.ModifyTime = stat_buf.st_mtime;
            info.CreationTime = stat_buf.st_ctime;
            cb(file, &info);
        }
        closedir(dirp);
    }
}

//---------------------------------------------------------------------------
void tTVPFileMedia::GetListAt(const ttstr &_name, iTVPStorageLister *lister) {
    ttstr name(_name);
    GetLocalName(name);
    TVPGetLocalFileListAt(name,
                          [lister](const ttstr &name, tTVPLocalFileInfo *s) {
                              if(s->Mode & (S_IFREG)) {
                                  lister->Add(name);
                              }
                          });
}
static int _utf8_strcasecmp(const char *a, const char *b) {
    for(; *a && *b; ++a, ++b) {
        int ca = *a, cb = *b;
        if('A' <= ca && ca <= 'Z')
            ca += 'a' - 'A';
        if('A' <= cb && cb <= 'Z')
            cb += 'a' - 'A';
        int ret = ca - cb;
        if(ret)
            return ret;
    }
    return *a - *b;
}

#if defined(TVP_PLATFORM_IOS)
const std::vector<std::string> &TVPGetApplicationHomeDirectory();
const std::vector<ttstr> &_getPrefixPath() {
    static std::vector<ttstr> ret;
    if(ret.empty()) {
        for(const std::string &path : TVPGetApplicationHomeDirectory()) {
            ret.emplace_back(path);
        }
    }
    return ret;
}
const std::vector<std::string> &_getHomeDir() {
    static std::vector<std::string> ret;
    if(ret.empty()) {
        for(const std::string &path : TVPGetApplicationHomeDirectory()) {
            ret.emplace_back(path + "/");
        }
    }
    return ret;
}
#endif

//---------------------------------------------------------------------------
void tTVPFileMedia::GetLocallyAccessibleName(ttstr &name) {
    ttstr newname;

    const tjs_char *ptr = name.c_str();

#ifdef WIN32
    if(TJS_strncmp(ptr, TJS_W("./"), 2)) {
        // differs from "./",
        // this may be a UNC file name.
        // UNC first two chars must be "\\\\" ?
        // AFAIK 32-bit version of Windows assumes that '/' can be
        // used as a path delimiter. Can UNC "\\\\" be replaced by
        // "//" though ?

        newname = ttstr(TJS_W("\\\\")) + ptr;
    } else {
        ptr += 2; // skip "./"
        if(!*ptr) {
            newname = TJS_W("");
        } else {
            tjs_char dch = *ptr;
            if(*ptr < TJS_W('a') || *ptr > TJS_W('z')) {
                newname = TJS_W("");
            } else {
                ptr++;
                if(*ptr != TJS_W('/')) {
                    newname = TJS_W("");
                } else {
                    newname = ttstr(dch) + TJS_W(":") + ptr;
                }
            }
        }
    }

    // change path delimiter to '\\'
    // tjs_char *pp = newname.Independ();
    // while(*pp) {
    //     if(*pp == TJS_W('/'))
    //         *pp = TJS_W('\\');
    //     pp++;
    // }
#else // posix
    if(!TJS_strncmp(ptr, TJS_W("./"), 2)) {
        ptr += 2; // skip "./"
        newname.Clear();
    }
#if defined(TVP_PLATFORM_IOS)
    {
        std::string prefix = "/";
        prefix += tTJSNarrowStringHolder(ptr).Buf;
        static const std::vector<ttstr> &prefixPath = _getPrefixPath();
        static const std::vector<std::string> &homeDir = _getHomeDir();
        for(int i = 0; i < prefixPath.size(); ++i) {
            const std::string &dir = homeDir[i];
            if(prefix.length() < dir.length())
                continue;
            std::string actualPrefix = prefix.substr(0, dir.length());
            if(!_utf8_strcasecmp(actualPrefix.c_str(), dir.c_str())) {
                newname = prefixPath[i];
                ptr += prefixPath[i].length();
                while(*ptr && *ptr == TJS_W('/'))
                    ++ptr;
                break;
            }
        }
    }
#endif
    while(*ptr) {
        const tjs_char *ptr_end = ptr;
        while(*ptr_end && *ptr_end != TJS_W('/'))
            ++ptr_end;
        if(ptr_end == ptr)
            break;
        const tjs_char *ptr_cur = ptr;
        tTJSNarrowStringHolder walker(ttstr(ptr, ptr_end - ptr).c_str());
        while(*ptr_end && *ptr_end == TJS_W('/'))
            ++ptr_end;
        ptr = ptr_end;

        DIR *dirp;
        struct dirent *direntp;
        newname += "/";
        if((dirp = opendir(tTJSNarrowStringHolder(newname.c_str())))) {
            bool found = false;
            while((direntp = readdir(dirp)) != nullptr) {
                if(!_utf8_strcasecmp(walker, direntp->d_name)) {
                    newname += direntp->d_name;
                    found = true;
                    break;
                }
            }
            closedir(dirp);
            if(!found) {
                newname += ptr_cur;
                break;
            }
        } else {
            newname += ptr_cur;
            break;
        }
    }

#endif
    name = newname;
}

//---------------------------------------------------------------------------
void tTVPFileMedia::GetLocalName(ttstr &name) {
    ttstr tmp = name;
    GetLocallyAccessibleName(tmp);
    if(tmp.IsEmpty())
        TVPThrowExceptionMessage(TVPCannotGetLocalName, name);
    name = tmp;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
bool TVPCheckExistentLocalPath(const ttstr &name) {
    if(name.IsEmpty())
        return false;
    tTVP_stat s{};
    if(!TVP_stat(name.c_str(), s))
        return false;
    return (s.st_mode & S_IFREG) || (s.st_mode & S_IFDIR);
}

//---------------------------------------------------------------------------
iTVPStorageMedia *TVPCreateFileMedia() { return new tTVPFileMedia; }
