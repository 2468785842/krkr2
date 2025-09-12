#include "../PSBFile.h"
#include "PimgType.h"

namespace PSB {

#define LOGGER spdlog::get("plugin")

    bool PimgType::isThisType(const PSBFile &psb) {
        const auto objects = psb.getObjects();
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


    // 忽略大小写的字符串比较辅助函数
    static bool startsWithIgnoreCase(const std::string &str,
                                     const std::string &prefix) {
        if(prefix.size() > str.size())
            return false;

        return std::equal(
            prefix.begin(), prefix.end(), str.begin(), [](char a, char b) {
                return std::tolower(static_cast<unsigned char>(a)) ==
                    std::tolower(static_cast<unsigned char>(b));
            });
    }

    static void findPimgResources(
        const std::vector<std::unique_ptr<IResourceMetadata>> &list,
        IPSBValue *obj, bool deDuplication = true) {
        if(auto c = dynamic_cast<PSBList *>(obj)) {
            for(const auto &o : *c) {
                auto dic = dynamic_cast<PSBDictionary *>(o.get());
                if(!dic)
                    continue;
                if(dic->find("layer_id") != dic->end()) {
                    int layerId = 0;
                    if(auto sLayerId = dynamic_cast<PSBString *>(
                           (*dic)["layer_id"].get())) {
                        layerId = std::stoi(sLayerId->value);
                    } else if(auto nLayerId = dynamic_cast<PSBNumber *>(
                                  (*dic)["layer_id"].get())) {
                        layerId = static_cast<int>(*nLayerId);
                    } else {
                        LOGGER->critical("layer_id is wrong.");
                        continue;
                    }

                    // 1. 优先查找带点前缀的项
                    auto findWithDot =
                        [&](const std::unique_ptr<IResourceMetadata> &item) {
                            return startsWithIgnoreCase(
                                item->getName(), fmt::format("{}.", layerId));
                        };

                    // 2. 其次查找无点前缀的项
                    auto findWithoutDot =
                        [&](const std::unique_ptr<IResourceMetadata> &item) {
                            return startsWithIgnoreCase(
                                item->getName(), fmt::format("{}", layerId));
                        };

                    // 查找逻辑：优先带点前缀，若无则找无点前缀
                    IResourceMetadata *foundItem = nullptr;

                    auto it =
                        std::find_if(list.begin(), list.end(), findWithDot);
                    if(it != list.end()) {
                        foundItem = it->get();
                    } else {
                        it = std::find_if(list.begin(), list.end(),
                                          findWithoutDot);
                        if(it != list.end()) {
                            foundItem = it->get();
                        }
                    }

                    // 安全类型转换
                    ImageMetadata *res = nullptr;
                    if(foundItem) {
                        res = dynamic_cast<ImageMetadata *>(foundItem);
                        if(!res) {
                            continue;
                            LOGGER->warn("convert ImageMetadata to "
                                         "IResourceMetadata Failed!");
                        }
                    } else {
                        continue;
                    }

                    res->setIndex(static_cast<unsigned int>(layerId));

                    auto getIntValue = [deDuplication](const PSBNumber &num,
                                                       int ori) {
                        return deDuplication
                            ? std::max(static_cast<int>(num), ori)
                            : static_cast<int>(num);
                    };

                    if(auto nw =
                           dynamic_cast<PSBNumber *>((*dic)["width"].get())) {
                        res->setWidth(getIntValue(*nw, res->getWidth()));
                    }

                    if(auto nh =
                           dynamic_cast<PSBNumber *>((*dic)["height"].get())) {
                        res->setHeight(getIntValue(*nh, res->getHeight()));
                    }

                    if(auto nt =
                           dynamic_cast<PSBNumber *>((*dic)["top"].get())) {
                        res->setTop(getIntValue(*nt, res->getTop()));
                    }

                    if(auto nl =
                           dynamic_cast<PSBNumber *>((*dic)["left"].get())) {
                        res->setLeft(getIntValue(*nl, res->getLeft()));
                    }

                    if(auto no =
                           dynamic_cast<PSBNumber *>((*dic)["opacity"].get())) {
                        res->setOpacity(getIntValue(*no, res->getOpacity()));
                    }

                    if(auto gLayerId = dynamic_cast<PSBNumber *>(
                           (*dic)["group_layer_id"].get())) {
                        res->setPart(
                            fmt::format("{}", static_cast<int>(*gLayerId)));
                    }

                    if(auto nv =
                           dynamic_cast<PSBNumber *>((*dic)["visible"].get())) {
                        res->setVisible(static_cast<int>(*nv) != 0);
                    }

                    if(auto nn =
                           dynamic_cast<PSBString *>((*dic)["name"].get())) {
                        res->setLabel(nn->value);
                    }
                    if(auto lt = dynamic_cast<PSBNumber *>(
                           (*dic)["layer_type"].get())) {
                        res->setLayerType(static_cast<int>(*lt));
                    }
                }
            }
        }
    }

    bool endsWithCI(const std::string &str, const std::string &suffix) {
        if(suffix.size() > str.size())
            return false;
        return std::equal(
            suffix.rbegin(), suffix.rend(), str.rbegin(),
            [](char a, char b) { return std::tolower(a) == std::tolower(b); });
    }

    std::vector<std::unique_ptr<IResourceMetadata>>
    PimgType::collectResources(const PSBFile &psb, bool deDuplication) {
        std::vector<std::unique_ptr<IResourceMetadata>> resourceList;
        if(psb.resources.empty())
            resourceList.resize(psb.resources.size());
        auto objs = psb.getObjects();
        for(const auto &[k, v] : *objs) {

            if(const auto resource =
                   std::dynamic_pointer_cast<const PSBResource>(v)) {

                auto meta = std::make_unique<ImageMetadata>();
                meta->setName(k);
                meta->setResource(std::make_shared<PSBResource>(*resource));
                meta->setCompress(endsWithCI(k, ".tlg")
                                      ? PSBCompressType::Tlg
                                      : PSBCompressType::ByName);
                meta->setPSBType(PSBType::Pimg);
                meta->setSpec(psb.getPlatform());

                resourceList.push_back(std::move(meta));
            }
        }
        findPimgResources(resourceList, (*objs)[G_PimgSourceKey].get(),
                          deDuplication);

        return resourceList;
    }
} // namespace PSB