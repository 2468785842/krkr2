#include "tjsCommHead.h"
#include "ConfigManager/PreferenceStorage.h"

ttstr TVPGetDataPath() {
    return krkr::config::RequirePreferenceStorage()->WritableDirectory();
}
