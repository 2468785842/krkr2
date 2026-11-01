#include "ArchiveRegistry.h"

#include "IFileBackend.h"

#include <vector>

static std::vector<TVPArchiveCreatorFn> g_archiveCreators;

//---------------------------------------------------------------------------
void TVPRegisterArchiveCreator(TVPArchiveCreatorFn creator) {
    if(creator)
        g_archiveCreators.push_back(creator);
}

//---------------------------------------------------------------------------
void TVPClearArchiveCreators() { g_archiveCreators.clear(); }

//---------------------------------------------------------------------------
tTVPArchive *TVPOpenArchive(const ttstr &name, bool normalizeFileName) {
    if(g_archiveCreators.empty())
        return nullptr;

    tTJSBinaryStream *st = TVPCreateStream(name);
    if(!st)
        return nullptr;
    for(auto creator : g_archiveCreators) {
        tTVPArchive *archive = creator(name, st, normalizeFileName);
        if(archive)
            return archive;
        st->SetPosition(0);
    }

    return nullptr;
}
