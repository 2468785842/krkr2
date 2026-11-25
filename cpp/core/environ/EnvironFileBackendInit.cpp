#include "EnvironFileBackendInit.h"

#include "IFileBackend.h"


//---------------------------------------------------------------------------
void TVPInitEnvironFileBackends() {
    if(TVPGetFileBackend())
        return;

    TVPInitFileBackend();
}
