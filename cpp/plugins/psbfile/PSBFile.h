#pragma once

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include <spdlog/spdlog.h>

#include "tjs.h"
#include "PSB.h"
#include "PSBHeader.h"
#include "PSBValue.h"

class tTVPMemoryStream;

namespace PSB {

    class PSBFile {
    public:
        PSBArray charset{};
        PSBArray namesData{};
        PSBArray nameIndexes{};
        std::vector<std::string> names{};
        PSBArray stringOffsets{};
        std::vector<PSBString> strings{};

        PSBArray chunkOffsets;
        PSBArray chunkLengths;

        std::vector<std::shared_ptr<PSBResource>> resources;

        PSBArray extraChunkOffsets{};
        PSBArray extraChunkLengths{};
        std::vector<std::shared_ptr<PSBResource>> extraResources;

        PSBFile();
        ~PSBFile();
        PSBFile(PSBFile &&) noexcept;
        PSBFile &operator=(PSBFile &&) noexcept;

        void loadKeys(TJS::tTJSBinaryStream *stream);
        void loadNames();

        void setSeed(int seed) { this->_seed = seed; }
        int getSeed() const { return this->_seed; }

        void setDecryptCallback(tTJSVariantClosure decryptClo) {
            _decryptClo = decryptClo;
        }

        /**
         * file type: *.PIMG
         * @param filePath
         */
        bool loadPSBFile(const ttstr &filePath);
        bool loadPSBStream(TJS::tTJSBinaryStream *stream);

        // Offset-based readers used by emotefile / motionplayer.
        tTJSVariant readAllObjs(const ttstr &key, tjs_uint32 objOffset);
        std::uint32_t readListInfo(std::vector<std::uint32_t> *target);
        void refreshListInfo(std::vector<std::uint32_t> *target1,
                             std::vector<std::uint32_t> *target2);
        bool parseObject(std::map<std::string, std::uint32_t> &output,
                         std::uint32_t objOffset);
        bool parseNumber(std::int64_t &output, std::uint32_t objOffset);
        bool parseReal(double &output, std::uint32_t objOffset);
        bool parseString(std::string &output, std::uint32_t objOffset);
        bool parseList(std::vector<std::uint32_t> &output,
                       std::uint32_t objOffset);
        bool parseResource(std::int32_t &id, std::uint32_t objOffset);
        [[nodiscard]] std::uint64_t getDataSize() const;
        void readAllData(std::uint8_t *output, std::uint32_t outputLen) const;
        bool readChunkData(std::int32_t chunkId, std::uint8_t *buffer,
                           std::uint32_t size) const;
        [[nodiscard]] const PSBArray &getChunkOffsets() const;
        [[nodiscard]] const PSBArray &getChunkLengths() const;
        /**
         * Load a string based on index, lift stream Position
         */
        void loadString(std::unique_ptr<PSBString> &str,
                        TJS::tTJSBinaryStream *stream);

        std::shared_ptr<PSBList> loadList(TJS::tTJSBinaryStream *stream,
                                          bool lazyLoad = false);
        std::shared_ptr<PSBDictionary>
        loadObjects(TJS::tTJSBinaryStream *stream, bool lazyLoad = false);

        std::shared_ptr<PSBDictionary>
        loadObjectsV1(TJS::tTJSBinaryStream *stream, bool lazyLoad = false);
        std::shared_ptr<IPSBValue> unpack(TJS::tTJSBinaryStream *stream,
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
            auto objects = getObjects();
            auto spec = objects ? (*objects)["spec"] : nullptr;
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
                return handler->second.get();
            }

            return TypeHandlers.at(PSBType::Motion).get();
        }

        PSBHeader getPSBHeader() const { return this->_header; }

        PSBType getType() const { return _type; }

    private:
        bool loadFromStream(TJS::tTJSBinaryStream *stream);

        unsigned _parseDepth = 0;
        int _seed = 0;
        PSBHeader _header{};
        std::unique_ptr<tTVPMemoryStream> _stream{};
        tTJSVariantClosure _decryptClo = NULL;
        std::shared_ptr<IPSBValue> _root{};
        PSBType _type{ PSBType::PSB };

        PSBType inferType() {
            for(const auto &[type, handler] : TypeHandlers) {
                if(handler->isThisType(*this)) {
                    this->_type = type;
                    break;
                }
            }

            return this->_type;
        }
    };
} // namespace PSB
