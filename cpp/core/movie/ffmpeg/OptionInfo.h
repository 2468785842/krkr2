//---------------------------------------------------------------------------
// option information for kirikiri configurator
//---------------------------------------------------------------------------

#ifndef __OptionInfoH__
#define __OptionInfoH__

#include <windows.h>
#include "tp_stub.h"
#include <tchar.h>

//---------------------------------------------------------------------------
// GetOptionInfoString: returns option information string for kirikiri
// conf.
//---------------------------------------------------------------------------
static inline const wchar_t *GetOptionInfoString() {
    return
        // for other languages; currently only English information is
        // available.
        L"Debug:ROT registration;Whether to register into "
        L"ROT(Running Object "
        L"Table) when "
        L"playbacking movies. Choosing 'Yes' enables you to inspect "
        L"the "
        L"trouble "
        L"related with movies, "
        L"using GraphEdit(available from DirectX SDK). 'Pause' not "
        L"only "
        L"enables "
        L"ROT registeration, "
        L"but makes the program also displaying message-box (this "
        L"interrupts "
        L"the "
        L"program and makes a pause) "
        L"after the graph is built.|"
        L"movie_reg_rot|select,*no;No,yes;Yes,pause;Pause\n";
}

#endif
