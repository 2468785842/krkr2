#include "psdbase.h"
#include "psddata.h"
#include <boost/iostreams/device/mapped_file.hpp>

namespace psd {
  // イメージ取得モード
  enum ImageMode {
    IMAGE_MODE_IMAGE,       // マスクをくりこまないイメージデータ
    IMAGE_MODE_MASK,        // マスク情報のみのイメージデータ(グレー)
    IMAGE_MODE_MASKEDIMAGE, // マスクをアルファに繰り込んだイメージデータ
  };
  
  /**
   * PSDファイルクラス
   */
  class PSDFile : public Data {
  public:
    PSDFile() : isLoaded(false) {}
    ~PSDFile() {}

    // 読み込み済みフラグ
    bool isLoaded;
    
    // ファイルロードエントリ
    bool load(const char *filename);

		// 画像データ取得インタフェース(バッファピッチが０の場合はfull fillされます)
    // 合成済み画像(PSDに保持されている場合のみ)
    bool getMergedImage(void *buf, const ColorFormat &format, int bufPitchByte);
    // レイヤ画像
    bool getLayerImage(LayerInfo &layer, void *buf, const ColorFormat &format,
                       int bufPitchByte, ImageMode mode);
    bool getLayerImageById(int layerId, void *buf, const ColorFormat &format,
                           int bufPitchByte, ImageMode mode);

  private:
		// loadFileで使用するメモリマップドファイル
    // (画像の遅延読み込みの関係上 Data 生存期間中は開きっぱなし)
    boost::iostreams::mapped_file_source in;
  };

} // namespace psd
