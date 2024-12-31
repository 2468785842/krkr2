#ifndef __psdlayer_h__
#define __psdlayer_h__

#include "psdbase.h"

namespace psd {
  bool loadLayerSectionDivider(LayerInfo &layer, AdditionalLayerInfo &additional);
  bool loadLayerUnicodeName(LayerInfo &layer, AdditionalLayerInfo &additional);
  bool loadLayerId(LayerInfo &layer, AdditionalLayerInfo &additional);
  bool loadLayerMetadata(LayerInfo &layer, AdditionalLayerInfo &additional);
  bool loadLayerFillOpacity(LayerInfo &layer, AdditionalLayerInfo &additional);
}
#endif //  __psdlayer_h__
