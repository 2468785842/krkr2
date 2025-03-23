#include "../PSBFile.h"
#include "PimgType.h"

namespace PSB {


    bool PimgType::isThisType(const PSBFile &psb) {
        const auto *objects = psb.getObjects();
        if(psb.getObjects() == nullptr) {
            return false;
        }

        if(objects->find("layers") != objects->end() &&
           objects->find("height") != objects->end() &&
           objects->find("width") != objects->end()) {
            return true;
        }

        for(const auto &[k, v] : *objects) {
            if(k.find('.') != std::string::npos &&
                 dynamic_cast<PSBResource *>(v.get())) {
                return true;
            }
        }

        return false;
    }
} // namespace PSB