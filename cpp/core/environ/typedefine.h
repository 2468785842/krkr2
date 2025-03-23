#pragma once
#include <stdint.h>

// 平台检测宏
#if defined(_WIN32) || defined(_WIN64)
#define TARGET_WINDOWS 1
#include <windows.h>
#else
#define TARGET_WINDOWS 0
#include "tjsTypes.h"
#include <sys/types.h>
#endif

/* 公共宏定义 */
// 仅在非Windows或未定义时声明MAX_PATH
#if !TARGET_WINDOWS || !defined(MAX_PATH)
#define MAX_PATH 260
#endif

#ifndef LF_FACESIZE
#define LF_FACESIZE 32
#endif

/* 仅定义Windows原生API未提供的类型 */
#if !TARGET_WINDOWS

/* ========== 类型定义部分 ========== */
typedef int32_t LONG;
typedef uint32_t ULONG;
typedef uint32_t DWORD;
typedef uint16_t WORD;
typedef uint8_t BYTE;
typedef int16_t SHORT;
typedef int32_t INT32;
typedef uint32_t UINT32;
typedef uint64_t ULONGLONG;
typedef void *LPVOID;
typedef uint32_t *LPDWORD;

typedef char *PSTR, *LPSTR;
typedef const char *LPCSTR;

/* ========== 句柄类型模拟 ========== */
typedef void *HBITMAP;
typedef void *HDC;
typedef void *HENHMETAFILE;
typedef void *HFONT;
typedef void *HICON;
typedef void *HINSTANCE;
typedef void *HMETAFILE;
typedef void *HPALETTE;
typedef LONG HRESULT;

/* ========== 结构体定义 ========== */
typedef struct _FILETIME {
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
} FILETIME;

typedef union _ULARGE_INTEGER {
    struct {
        DWORD LowPart;
        DWORD HighPart;
    } u;
    ULONGLONG QuadPart;
} ULARGE_INTEGER;

typedef union _LARGE_INTEGER {
    struct {
        DWORD LowPart;
        LONG HighPart;
    } u;
    int64_t QuadPart; // 更明确的类型
} LARGE_INTEGER;

/* ========== 兼容性宏 ========== */
#define IN
#define OUT
#define CONST const

/* ========== GDI+相关常量 ========== */
// 映射模式
#define MM_TEXT 1
#define MM_LOMETRIC 2
#define MM_HIMETRIC 3
#define MM_LOENGLISH 4
#define MM_HIENGLISH 5
#define MM_TWIPS 6
#define MM_ISOTROPIC 7
#define MM_ANISOTROPIC 8

// 画笔样式
#define PS_NULL 0x00000005
#define PS_STYLE_MASK 0x0000000F

typedef tjs_char *LPWSTR;
typedef const tjs_char *LPCWSTR;

#endif // !TARGET_WINDOWS

/* ========== 跨平台通用部分 ========== */
// 确保基础类型存在（所有平台）
typedef uint8_t UINT8;
typedef uint16_t UINT16;
typedef uint32_t UINT32;
typedef uint64_t UINT64;
typedef int8_t INT8;
typedef int16_t INT16;
typedef int32_t INT32;
typedef int64_t INT64;

// 浮点类型
typedef float REAL;

#undef TARGET_WINDOWS