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

    static void
    findPimgResources(std::vector<std::unique_ptr<IResourceMetadata>> list,
                      IPSBValue *obj, bool deDuplication = true) {}

    bool endsWithCI(const std::string &str, const std::string &suffix) {
        if(suffix.size() > str.size())
            return false;
        return std::equal(
            suffix.rbegin(), suffix.rend(), str.rbegin(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    PimgType::collectResources(const PSBFile &psb, bool deDuplication) {
        auto resourceList = psb.resources.empty()
            ? std::vector<std::unique_ptr<IResourceMetadata>>()
            : std::vector<std::unique_ptr<IResourceMetadata>>(
                  psb.resources.size());
        auto objs = psb.getObjects();
        for(const auto &[k, v] : *objs) {

            const auto *resource = dynamic_cast<const PSBResource *>(v.get());
            if(resource) {
                ImageMetadata *meta = new ImageMetadata();
                meta->name = k;
                // meta->resource = resource;
                // meta->compress = endsWithCI(k, ".tlg")
                    // ? PSBCompressType::Tlg
                    // : PSBCompressType::ByName;
                meta->psbType = PSBType::Pimg;
                // meta->spec = psb.Platform;

                resourceList.push_back(
                    std::make_unique<IResourceMetadata>(*meta));
            }
        }
        findPimgResources(resourceList, objs[PimgSourceKey], deDuplication);

        return resourceList;
    }
} // namespace PSB