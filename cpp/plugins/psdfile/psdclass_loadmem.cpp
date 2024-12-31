#include "psdclass.h"

#ifdef LOAD_MEMORY

#include "psdparse/psdparse.h"

void
PSD::clearMemory()
{
	if (hBuffer) {
		::GlobalUnlock(hBuffer);
		::GlobalFree(hBuffer);
		hBuffer = 0;
	}
}

bool
PSD::loadMemory(const ttstr &filename)
{
	clearData();

	// まるごとメモリに読み込んで処理
	isLoaded = false;
	IStream *stream = TVPCreateIStream(filename, TJS_BS_READ);
	if (stream) {
		try {
			// 全部メモリに読み込む
			STATSTG stat;
			stream->Stat(&stat, STATFLAG_NONAME);
			tjs_uint64 qsize = (tjs_uint64)stat.cbSize.QuadPart;
			if (qsize < 0xFFFFFFFF) {
				DWORD size = (DWORD)qsize;
				hBuffer = ::GlobalAlloc(GMEM_MOVEABLE, size);
				unsigned char* pBuffer = (unsigned char*)::GlobalLock(hBuffer);
				if (pBuffer) {
					ULONG retsize;
					stream->Read(pBuffer, size, &retsize);
					psd::Parser<unsigned char*> parser(*this);
					unsigned char *begin = pBuffer;
					unsigned char *end   = begin + size;
					bool r = parse(begin , end,  parser);
					if (r && begin == end) {
						dprint("succeeded\n");
						isLoaded = processParsed();
					}
					if (!isLoaded) {
						clearData();
					}
				}
			}
		} catch(...) {
			clearData();
			stream->Release();
			throw;
		}
		stream->Release();
	}
	return isLoaded;	
}

#endif
