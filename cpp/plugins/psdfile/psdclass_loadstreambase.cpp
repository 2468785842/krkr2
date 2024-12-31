#ifndef LOAD_MEMORY

#include "psdclass.h"

void
PSD::clearStream()
{
	if (pStream) {
		pStream->Release();
		pStream = 0;
	}
	mStreamSize = 0;
	mBufferPos  = 0;
	mBufferSize = 0;
}


unsigned char &
PSD::getStreamValue(const tTVInteger &pos)
{
	static unsigned char eof = 0;
	if (pos >=0 && pos < mStreamSize) {
		if (pos >= mBufferPos && pos < mBufferPos + mBufferSize) {
			return mBuffer[pos - mBufferPos];
		}
		mBufferPos = pos;
		LARGE_INTEGER n;
		n.QuadPart = pos;
		pStream->Seek(n, STREAM_SEEK_SET, 0);
		pStream->Read(mBuffer, sizeof mBuffer, &mBufferSize);
		return mBuffer[0];
	}
	return eof;
}

void
PSD::copyToBuffer(uint8_t *buf, tTVInteger pos, int size)
{
	if (pos >=0 && pos < mStreamSize) {
		if (size <= 16) {
			if (pos >= mBufferPos && pos+size < mBufferPos + mBufferSize) {
				memcpy(buf, &mBuffer[pos - mBufferPos], size);
				return;
			}
			mBufferPos = pos;
			LARGE_INTEGER n;
			n.QuadPart = pos;
			pStream->Seek(n, STREAM_SEEK_SET, 0);
			pStream->Read(mBuffer, sizeof mBuffer, &mBufferSize);
			memcpy(buf, mBuffer, size);
			return;
		}
		// 直接読み込む
		LARGE_INTEGER n;
		n.QuadPart = pos;
		pStream->Seek(n, STREAM_SEEK_SET, 0);
		ULONG rsize;
		pStream->Read(buf, size, &rsize);
	}
}

#endif
