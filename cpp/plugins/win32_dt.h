//
// Created by lidong on 2025/1/5.
// more info:
// https://learn.microsoft.com/en-us/windows/win32/winprog/windows-data-types
//

#ifndef KRKR2_WIN32_DT_H
#define KRKR2_WIN32_DT_H
#ifndef _WIN32
typedef void *PVOID;
typedef PVOID HANDLE;
typedef HANDLE HMENU;
typedef HANDLE HWND;
typedef HANDLE HICON;
typedef HANDLE HDC;
typedef HANDLE HBITMAP;
typedef HANDLE HGLOBAL;

// typedef unsigned long DWORD;

typedef unsigned short WORD;
typedef unsigned int UINT;
// typedef uintptr_t ULONG_PTR;
#else
#include <BaseTsd.h>
#endif
#endif // KRKR2_WIN32_DT_H
