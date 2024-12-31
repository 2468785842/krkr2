#include "psdparse.h"
#include "psdfile.h"

namespace psd {

// psdファイルをロードする
bool
PSDFile::load(const char *filename)
{ 
	clearData();

	namespace spirit = boost::spirit;
  namespace fs = boost::filesystem;

  isLoaded = false;

  const fs::path file(filename);
  boost::system::error_code error;
  const bool result = fs::exists(file, error);
  if (!result || error) {
    std::cerr << "file not found!: '" << filename << "'" << std::endl;
    return false;
  }

  in.open(filename);
  if (!in.is_open()) {
    std::cerr << "could not open input file: '" << filename << "'" << std::endl;
    return false;
  }

  typedef boost::iostreams::mapped_file_source::iterator iterator_type;
  iterator_type iter = in.begin();
  iterator_type end  = in.end();

  psd::Parser<iterator_type> parser(*this);
  bool r = parse(iter, end, parser);
  if (r && iter == end) {
    dprint("succeeded\n");
    isLoaded = processParsed();
  } else {
    std::cerr << "failed\n";
    in.close();
  }

  return isLoaded;
}
} // namespace psd

/**
 * 単体デバッグ用メイン
 */
int
// _tmain(int argc, _TCHAR* argv[])
main(int argc, const char* argv[])
{
  if (argc > 1) {
    psd::PSDFile psd;
    psd.load(argv[1]);

    for (int i = 0; i < (int)psd.layerList.size(); i++) {
      psd::LayerInfo &layer = psd.layerList[i];
      char *buf = new char[layer.width * layer.height * 4];
      psd.getLayerImage(layer, buf, psd::BGRA_LE, layer.width * 4, psd::IMAGE_MODE_MASKEDIMAGE);
      delete [] buf;
    }

    char *buf = new char[psd.header.width * psd.header.height * 4];
    psd.getMergedImage(buf, psd::BGRA_LE, 0);
    delete [] buf;
  }
  return 0;
}
