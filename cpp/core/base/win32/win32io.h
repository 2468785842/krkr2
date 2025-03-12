#pragma once
#ifdef WIN32
#include <io.h>
// posix io api
inline unsigned int lseek64(int  fileHandle,long offset,int  origin) {
    return _lseek(fileHandle, offset, origin);
}
// extern void *valloc(int n);
// extern void vfree(void *p);
// }
#endif
#ifdef CC_TARGET_OS_IPHONE
#define lseek64 lseek
#endif