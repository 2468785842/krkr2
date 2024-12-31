#include "psdclass.h"

#ifndef LOAD_MEMORY

#include "psdparse/psdparse.h"

/**
 * Stream 用の Iterator
 */
class PSDIterator : public std::iterator<std::random_access_iterator_tag, const unsigned char>
{
public:

	typedef size_t diff_t;

	PSDIterator() : _psd(0), _pos(0), _size(0) {
	}

	/**
	 * コンストラクタ
	 * @param srream 参照ストリーム
	 * @param begin true なら冒頭、false なら末尾
	 */
	PSDIterator(PSD* psd, bool begin) : _psd(psd), _pos(0), _size(0) {
		_size = psd->mStreamSize;
		_pos = begin ? 0 : _size;
	}

	// コピーコンストラクタ
	PSDIterator(const PSDIterator& o) {
		_psd = o._psd;
		_pos = o._pos;
		_size = o._size;
	}

	// 代入演算子
	PSDIterator &operator=(const PSDIterator &o) {
		_psd  = o._psd;
		_pos  = o._pos;
		_size = o._size;
		return *this;
	}
	
	// デストラクタ
	virtual ~PSDIterator() {
	}

	// イテレータを進める(前置)
	PSDIterator& operator++() {
		_pos++;
		if (_pos > _size) _pos = _size;
		return *this;
	}

	// イテレータを進める（後置)
	PSDIterator operator++(int) {
		PSDIterator ret = *this;
		_pos++;
		if (_pos > _size) _pos = _size;
		return ret;
	}

	PSDIterator& operator+=(diff_t n) {
		_pos += n;
		if (_pos > _size) _pos = _size;
		return *this;
	}

	PSDIterator operator+(diff_t n) {
		PSDIterator ret = *this;
		ret += n;
		return ret;
	}
	
	// イテレータを戻す(前置)
	PSDIterator& operator--() {
		_pos--;
		if (_pos < 0) _pos = 0;
		return *this;
	}

	// イテレータを戻す（後置)
	PSDIterator operator--(int) {
		PSDIterator ret = *this;
		_pos--;
		if (_pos < 0) _pos = 0;
		return ret;
	}

	PSDIterator& operator-=(diff_t n) {
		_pos -= n;
		if (_pos < 0) _pos = 0;
		return *this;
	}

	PSDIterator operator-(diff_t n) {
		PSDIterator ret = *this;
		ret -= n;
		return ret;
	}

	// 読み取り
	const unsigned char& operator*() {
		return _psd->getStreamValue(_pos);
	}

	void copyToBuffer(uint8_t *buf, int size) {
		_psd->copyToBuffer(buf, _pos, size);
		_pos += size;
	}
	
	// 差分
	diff_t operator-(const PSDIterator& b) const {
		return (diff_t)(_pos - b._pos);
	}
	
	// イテレータの一致判定
	bool operator==(const PSDIterator& o) const {
		return o._pos == _pos;
	}
	// イテレータの一致判定
	bool operator!=(const PSDIterator& o) const {
		return o._pos != _pos;
	}

	bool operator<(const PSDIterator& o) const {
		return _pos < o._pos;
	}
	bool operator>(const PSDIterator& o) const {
		return _pos > o._pos;
	}
	bool operator<=(const PSDIterator& o) const {
		return _pos <= o._pos;
	}
	bool operator>=(const PSDIterator& o) const {
		return _pos >= o._pos;
	}
	

	
private:
	PSD *_psd;
	tTVInteger _size; //< ストリームサイズ保持用
	tTVInteger _pos;  //< 参照位置
};

namespace psd {

	inline void copyToBuffer(uint8_t *buffer, PSDIterator &cur, int size) {
		cur.copyToBuffer(buffer, size);
	}

	inline void getShortLE(uint8_t *buffer, PSDIterator &cur) {
		cur.copyToBuffer(buffer, 2);
	}
	
	inline void getLongLE(uint8_t *buffer, PSDIterator &cur) {
		cur.copyToBuffer(buffer, 4);
	}

	inline void getLongLongLE(uint8_t *buffer, PSDIterator &cur) {
		cur.copyToBuffer(buffer, 8);
	}
}

bool
PSD::loadStream(const ttstr &filename)
{
	clearData();

	// ストリームのまま
	isLoaded = false;
	pStream = TVPCreateIStream(filename, TJS_BS_READ);
	if (pStream) {

		STATSTG stat;
		pStream->Stat(&stat, STATFLAG_NONAME);
		mStreamSize = stat.cbSize.QuadPart;

		PSDIterator begin(this, true);
		PSDIterator end(this, false);
		psd::Parser<PSDIterator> parser(*this);
		bool r = parse(begin , end,  parser);
		if (r && begin == end) {
			dprint("succeeded\n");
			isLoaded = processParsed();
		}
		if (!isLoaded) {
			clearData();
		}
	}
	return isLoaded;	
}

#endif
