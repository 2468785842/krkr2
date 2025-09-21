/*---------------------------------------------------------------------------*/
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
/*---------------------------------------------------------------------------*/
/* "TJS2" type definitions */
/*---------------------------------------------------------------------------*/
#pragma once

#include <cstdint>
#include <stddef.h> // ptrdiff_t

#if !defined(_WIN32)
#include <sys/types.h>
#endif

/* Functions that needs to be exported ( for non-class-member
 * functions ) */
/* This should only be applyed for function declaration in headers (
 * not body )
 */
#define TJS_EXP_FUNC_DEF(rettype, name, arg) extern rettype name arg

/* Functions that needs to be exported ( for class-member functions )
 */
#define TJS_METHOD_DEF(rettype, name, arg) rettype name arg
#define TJS_CONST_METHOD_DEF(rettype, name, arg) rettype name arg const
#define TJS_STATIC_METHOD_DEF(rettype, name, arg) static rettype name arg
#define TJS_METHOD_RET_EMPTY
#define TJS_METHOD_RET(type)

typedef std::int8_t tjs_int8;
typedef std::uint8_t tjs_uint8;
typedef std::int16_t tjs_int16;
typedef std::uint16_t tjs_uint16;
typedef std::int32_t tjs_int32;
typedef std::uint32_t tjs_uint32;
typedef std::int64_t tjs_int64;
typedef std::uint64_t tjs_uint64;

typedef char16_t tjs_char;
#define TJS_W(X) u##X
#define TJS_N(X) X

typedef char tjs_nchar;
typedef double tjs_real;

typedef int tjs_int;
typedef unsigned int tjs_uint;

typedef intptr_t tjs_intptr_t;
typedef uintptr_t tjs_uintptr_t;

typedef tjs_int32 tjs_error;

typedef tjs_int64 tTVInteger;
typedef tjs_real tTVReal;

typedef size_t tjs_size;
typedef ptrdiff_t tjs_offset;

#ifdef WORDS_BIGENDIAN
#define TJS_HOST_IS_BIG_ENDIAN 1
#define TJS_HOST_IS_LITTLE_ENDIAN 0
#else
#define TJS_HOST_IS_BIG_ENDIAN 0
#define TJS_HOST_IS_LITTLE_ENDIAN 1

#if defined(_WIN32)

#define TJS_I64_VAL(x) ((tjs_int64)(x##i64))
#define TJS_UI64_VAL(x) ((tjs_uint64)(x##i64))

#ifdef _M_X64
#define TJS_64BIT_OS /* 64bit windows */
#endif

#if defined(_MSC_VER)
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#endif

#else

#define TJS_I64_VAL(x) ((tjs_int64)(x##LL))
#define TJS_UI64_VAL(x) ((tjs_uint64)(x##LL))

#endif

#define TJS_USERENTRY

#endif

/* IEEE double manipulation support
 (TJS requires IEEE double(64-bit float) native support on machine or
 C++ compiler) */

/*

63 62       52 51                         0
+-+-----------+---------------------------+
|s|    exp    |         significand       |
+-+-----------+---------------------------+

s = sign,  negative if this is 1, otherwise positive.

*/

/* double related constants */
#define TJS_IEEE_D_EXP_MAX 1023
#define TJS_IEEE_D_EXP_MIN (-1022)
#define TJS_IEEE_D_SIGNIFICAND_BITS 52

#define TJS_IEEE_D_EXP_BIAS 1023

/* component extraction */
#define TJS_IEEE_D_SIGN_MASK (TJS_UI64_VAL(0x8000000000000000))
#define TJS_IEEE_D_EXP_MASK (TJS_UI64_VAL(0x7ff0000000000000))
#define TJS_IEEE_D_SIGNIFICAND_MASK (TJS_UI64_VAL(0x000fffffffffffff))
#define TJS_IEEE_D_SIGNIFICAND_MSB_MASK (TJS_UI64_VAL(0x0008000000000000))

#define TJS_IEEE_D_GET_SIGN(x) (0 != ((x) & TJS_IEEE_D_SIGN_MASK))
#define TJS_IEEE_D_GET_EXP(x)                                                  \
    ((tjs_int)((((x) & TJS_IEEE_D_EXP_MASK) >> TJS_IEEE_D_SIGNIFICAND_BITS) -  \
               TJS_IEEE_D_EXP_BIAS))
#define TJS_IEEE_D_GET_SIGNIFICAND(x) ((x) & TJS_IEEE_D_SIGNIFICAND_MASK)

/* component composition */
#define TJS_IEEE_D_MAKE_SIGN(x)                                                \
    ((x) ? TJS_UI64_VAL(0x8000000000000000) : TJS_UI64_VAL(0))
#define TJS_IEEE_D_MAKE_EXP(x) ((tjs_uint64)((x) + TJS_IEEE_D_EXP_BIAS) << 52)
#define TJS_IEEE_D_MAKE_SIGNIFICAND(x) ((tjs_uint64)(x))

/* special expression */
/* (quiet) NaN */
#define TJS_IEEE_D_P_NaN                                                       \
    (tjs_uint64)(TJS_IEEE_D_EXP_MASK | TJS_IEEE_D_SIGNIFICAND_MSB_MASK)
#define TJS_IEEE_D_N_NaN (tjs_uint64)(TJS_IEEE_D_SIGN_MASK | TJS_IEEE_D_P_NaN)
/* infinite */
#define TJS_IEEE_D_P_INF (tjs_uint64)(TJS_IEEE_D_EXP_MASK)
#define TJS_IEEE_D_N_INF (tjs_uint64)(TJS_IEEE_D_SIGN_MASK | TJS_IEEE_D_P_INF)

/* special expression check */
#define TJS_IEEE_D_IS_NaN(x)                                                   \
    ((TJS_IEEE_D_EXP_MASK & (x)) == TJS_IEEE_D_EXP_MASK) &&                    \
        (((x) & TJS_IEEE_D_SIGNIFICAND_MSB_MASK) ||                            \
         (!((x) & TJS_IEEE_D_SIGNIFICAND_MSB_MASK) &&                          \
          ((x) &                                                               \
           (TJS_IEEE_D_SIGNIFICAND_MASK ^ TJS_IEEE_D_SIGNIFICAND_MSB_MASK))))
#define TJS_IEEE_D_IS_INF(x)                                                   \
    (((TJS_IEEE_D_EXP_MASK & (x)) == TJS_IEEE_D_EXP_MASK) &&                   \
     (!((x) & TJS_IEEE_D_SIGNIFICAND_MASK)))
