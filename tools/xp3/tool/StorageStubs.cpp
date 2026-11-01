#include "tjsCommHead.h"

#include "IFileBackend.h"
#include "Platform.h"
#include "StdFileBackend.h"
#include "StorageImpl.h"

#include <chrono>

// must after Platform.h (undef st_* macros)
#include <sys/stat.h>

#ifdef st_atime
#undef st_atime
#endif
#ifdef st_mtime
#undef st_mtime
#endif
#ifdef st_ctime
#undef st_ctime
#endif

//---------------------------------------------------------------------------
void TVPInitFileBackend() {
    if(!TVPGetFileBackend())
        TVPSetFileBackend(std::make_unique<StdFileBackend>());
}

//---------------------------------------------------------------------------
void TVPPreNormalizeStorageName(ttstr & /*name*/) {}

//---------------------------------------------------------------------------
static bool TVP_stat_impl(const char *name, tTVP_stat &s) {
    if(!name)
        return false;
    struct stat t{};
    if(::stat(name, &t) != 0)
        return false;
    s.st_mode = static_cast<uint16_t>(t.st_mode);
    s.st_size = static_cast<uint64_t>(t.st_size);
#if defined(__APPLE__)
    s.st_atime = static_cast<uint64_t>(t.st_atimespec.tv_sec);
    s.st_mtime = static_cast<uint64_t>(t.st_mtimespec.tv_sec);
    s.st_ctime = static_cast<uint64_t>(t.st_ctimespec.tv_sec);
#else
    s.st_atime = static_cast<uint64_t>(t.st_atime);
    s.st_mtime = static_cast<uint64_t>(t.st_mtime);
    s.st_ctime = static_cast<uint64_t>(t.st_ctime);
#endif
    return true;
}

//---------------------------------------------------------------------------
bool TVP_stat(const tjs_char *name, tTVP_stat &s) {
    return TVP_stat_impl(tTJSNarrowStringHolder(name).Buf, s);
}

//---------------------------------------------------------------------------
bool TVP_stat(const char *name, tTVP_stat &s) { return TVP_stat_impl(name, s); }

//---------------------------------------------------------------------------
tjs_uint64 TVPGetTickCount() {
    using clock = std::chrono::steady_clock;
    return static_cast<tjs_uint64>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            clock::now().time_since_epoch())
            .count());
}
