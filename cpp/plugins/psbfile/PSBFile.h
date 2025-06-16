#pragma once

#include <spdlog/spdlog.h>

#include "tjs.h"
#include "PSB.h"
#include "PSBHeader.h"
#include "PSBValue.h"

namespace PSB {

    class PSBFile {
    public:
        PSB::PSBArray charset{};
        PSB::PSBArray namesData{};
        PSB::PSBArray nameIndexes{};
        std::vector<std::string> names{};
        PSB::PSBArray stringOffsets{};
        std::vector<PSB::PSBString> strings{};

        PSB::PSBArray chunkOffsets;
        PSB::PSBArray chunkLengths;

        std::vector<std::shared_ptr<PSBResource>> resources;

        PSB::PSBArray extraChunkOffsets{};
        PSB::PSBArray extraChunkLengths{};
        std::vector<std::shared_ptr<PSBResource>> extraResources;

        explicit PSBFile();

        void loadKeys(TJS::tTJSBinaryStream *stream);
        void loadNames();
        /**
         * file type: *.PIMG
         * @param filePath
         */
        bool loadPSBFile(const ttstr &filePath);
        /**
         * Load a string based on index, lift stream Position
         */
        void loadString(std::unique_ptr<PSB::PSBString> &str,
                        TJS::tTJSBinaryStream *stream);

        std::shared_ptr<PSB::PSBList> loadList(TJS::tTJSBinaryStream *stream,
                                               bool lazyLoad = false);
        std::shared_ptr<PSB::PSBDictionary>
        loadObjects(TJS::tTJSBinaryStream *stream, bool lazyLoad = false);

        std::shared_ptr<PSB::PSBDictionary>
        loadObjectsV1(TJS::tTJSBinaryStream *stream, bool lazyLoad = false);
        std::shared_ptr<PSB::IPSBValue> unpack(TJS::tTJSBinaryStream *stream,
                                               bool lazyLoad = false);
        void loadResource(PSBResource &res,
                          TJS::tTJSBinaryStream *stream) const;
        void loadExtraResource(PSBResource &res,
                               TJS::tTJSBinaryStream *stream) const;
        void afterLoad();

        [[nodiscard]] std::shared_ptr<const PSBDictionary> getObjects() const {
            return std::dynamic_pointer_cast<const PSBDictionary>(_root);
        }

        [[nodiscard]] PSBSpec getPlatform() const {
            auto spec = (*getObjects())["spec"];
            std::string specStr = !spec ? "" : spec->toString();
            if(specStr.empty()) {
                return PSBSpec::None;
            }

            // auto p = static_cast<PSBSpec>(spec);
            return /*p ? p : */ PSBSpec::Other;
        }

        [[nodiscard]] IPSBType *getTypeHandler() const {
            auto handler = TypeHandlers.find(_type);
            if(handler != TypeHandlers.end()) {
                return handler->second;
            }

            // return new MotionType();
            return nullptr;
        }

    private:
        PSB::PSBHeader _header{};
        std::shared_ptr<IPSBValue> _root{};
        PSBType _type = PSBType::PSB;

        PSBType inferType() {
            for(const auto &[type, handler] : TypeHandlers) {
                if(handler->isThisType(*this)) {
                    this->_type = type;
                    return this->_type;
                }
            }

            // foreach (var handler in FreeMount._.SpecialTypes)
            // {
            //     if (handler.Value.IsThisType(this))
            //     {
            //         TypeId = handler.Key;
            //         Type = PsbType.PSB;
            //         return PsbType.PSB;
            //     }
            // }

            return PSBType::PSB;
        }
    };
} // namespace PSB