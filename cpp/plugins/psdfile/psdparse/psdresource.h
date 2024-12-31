#include "psddata.h"

#include <string>

namespace psd {
  // リソースローダ
  bool loadResourceSlice(Data &data, ImageResourceInfo &res);
  bool loadResourceGridAndGuide(Data &data, ImageResourceInfo &res);
  bool loadResourceColorTableCount(Data &data, ImageResourceInfo &res);
  bool loadResourceTransparencyIndex(Data &data, ImageResourceInfo &res);
  bool loadResourceLayerComps(Data &data, ImageResourceInfo &res);
} // namespace psd
