#include "ncbind/ncbind.hpp"

void TVPLoadInternalPlugins() {
    ncbAutoRegister::AllRegist();
    ncbAutoRegister::LoadModule(TJS_W("xp3filter.dll"));
}

[[maybe_unused]] void TVPUnloadInternalPlugins() {
    ncbAutoRegister::AllUnregist();
}

bool TVPLoadInternalPlugin(const ttstr &_name) {
    return ncbAutoRegister::LoadModule(TVPExtractStorageName(_name));
}