#include <cstring>
#include "psddata.h"
#include "psddesc.h"

namespace psd {
    bool loadLayerSectionDivider(LayerInfo &layer,
                                 AdditionalLayerInfo &additional) {
        int type = additional.data->getInt32();
        switch(type) {
            default:
            case 0: // フォルダ関係以外の場合は他で適切なものがセットされるので無視
                break;
            case 1:
            case 2:
                layer.layerType = LAYER_TYPE_FOLDER;
                break;
            case 3:
                layer.layerType = LAYER_TYPE_HIDDEN;
                break;
        }

        if(additional.size >= 12) {
            int signature = additional.data->getInt32(); // 読み捨て
            layer.blendMode = blendKeyToMode(additional.data->getInt32());
            if(additional.size >= 16) {
                int subType = additional.data->getInt32(); // 読み捨て
            }
        }
        return true;
    }

    bool loadLayerUnicodeName(LayerInfo &layer,
                              AdditionalLayerInfo &additional) {
        additional.data->getUnicodeString(layer.layerNameUnicode);
        return true;
    }

    bool loadLayerId(LayerInfo &layer, AdditionalLayerInfo &additional) {
        layer.layerId = additional.data->getInt32();
        return true;
    }

    bool loadLayerFillOpacity(LayerInfo &layer,
                              AdditionalLayerInfo &additional) {
        layer.fill_opacity = additional.data->getCh();
        return true;
    }

    bool loadLayerMetadata(LayerInfo &layer, AdditionalLayerInfo &additional) {
        int count = additional.data->getInt32();
        for(int i = 0; i < count; i++) {
            int signature = additional.data->getInt32();
            int key = additional.data->getInt32();
            bool copyOnSheetDup = (additional.data->getCh() != 0);
            additional.data->advance(3);
            int len = additional.data->getInt32();

            // レイヤーカンプ
            if(key == 'cmls') {
                int ver = additional.data->getInt32();
                if(ver != 16) {
                    continue; // not support
                }
                Descriptor &dsc = layer.layerCompDesc;
                if(!dsc.load(additional.data)) {
                    continue; // invalid data
                }
                // dprint("----\n");
                // dsc.dump();
                DescriptorList *settings =
                    (DescriptorList *)dsc.findItem("layerSettings");
                if(settings) {
                    bool lastEnabled = true;
                    for(int i = 0; i < (int)settings->items.size(); i++) {
                        Descriptor *comp = settings->item(i);
                        DescriptorList *compList = comp->item("compList");
                        if(compList) {
                            LayerCompInfo ci{};
                            memset(&ci, 0, sizeof(ci));

                            int compId =
                                ((DescriptorInteger *)compList->items[0])->val;
                            ci.id = compId;

                            Descriptor *offset = comp->item("Ofst");
                            if(offset) {
                                DescriptorInteger *h = comp->item("Hrzn");
                                DescriptorInteger *v = comp->item("Hrzn");
                                ci.offsetX = (h) ? h->val : 0;
                                ci.offsetY = (v) ? v->val : 0;
                            }

                            DescriptorBoolean *enable = comp->item("enab");
                            if(enable) {
                                ci.isEnabled = lastEnabled = enable->val;
                            } else {
                                ci.isEnabled = lastEnabled;
                            }

                            dprint("Comp: id:0x%08x, offX:%d, "
                                   "offY:%d, enabled:%d\n",
                                   ci.id, ci.offsetX, ci.offsetY, ci.isEnabled);

                            layer.layerComps[compId] = ci;
                        }
                    }
                }
            } else {
                // additional.data->getData(, len);
            }
        }

        return true;
    }

} // namespace psd
