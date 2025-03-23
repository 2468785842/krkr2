#include "PSBFile.h"

#include <memory>

#define LOGGER spdlog::get("plugin")

namespace PSB {
    PSBFile::PSBFile() { LOGGER->info("PSBFile::constructor"); }

    void PSBFile::loadKeys(TJS::tTJSBinaryStream *stream) {
        const size_t len = nameIndexes.value.size();
        names.reserve(len);
        for(int i = 0; i < len; i++) {
            stream->SetPosition(_header.offsetNames + nameIndexes[i]);
            names.push_back(PSB::Extension::readStringZeroTrim(stream));
        }
    }

    void PSBFile::loadNames() {
        const size_t len = nameIndexes.value.size();
        names.reserve(len);
        for(int i = 0; i < len; i++) {
            auto list = std::vector<char>();
            const auto index = nameIndexes[i];
            auto chr = namesData[index];
            while(chr != 0) {
                const auto code = namesData[chr];
                const auto d = charset[code];
                const auto realChr = chr - d;
                chr = code;
                list.push_back(realChr);
            }

            list.resize(list.size());
            auto str = std::string(
                list.data(),
                list.size()); // That's why we don't use StringBuilder here.
            names.push_back(str);
        }
    }

    void PSBFile::loadString(std::unique_ptr<PSB::PSBString> &str,
                             TJS::tTJSBinaryStream *stream) {
        assert(str->index.has_value() && "Index can not be null");
        auto idx = str->index;
        const auto refStr = std::find_if(
            strings.begin(), strings.end(),
            [idx](const PSB::PSBString &s) { return s.index == idx; });

        /*if (refStr != strings.end() && Consts.FastMode) {
            str = refStr;
            return;
        }*/
        stream->SetPosition(_header.offsetStringsData +
                            stringOffsets[static_cast<int>(idx.value())]);
        auto strValue = PSB::Extension::readStringZeroTrim(stream);

        // Strict value equal check
        if(refStr != strings.end() && strValue == refStr->value) {
            str = std::make_unique<PSBString>(*refStr);
            return;
        }

        if(refStr != strings.end()) {
            LOGGER->info("{} does not match {}", refStr->value, strValue);
        }

        str->value = strValue;
        strings.emplace_back(*str); // FIXED
    }

    std::shared_ptr<PSB::PSBList>
    PSBFile::loadList(TJS::tTJSBinaryStream *stream, bool lazyLoad) {
        auto offsets = PSB::PSBList::loadIntoList(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream);
        const size_t pos = stream->GetPosition();
        auto list = std::make_shared<PSB::PSBList>(offsets.size());
        std::optional<std::uint32_t> maxOffset{};
        size_t endPos = pos;
        if(lazyLoad && !offsets.empty()) {
            maxOffset = *std::max_element(offsets.cbegin(), offsets.cend());
        }
        for(const auto offset : offsets) {
            stream->SetPosition(pos + offset);
            auto obj = unpack(stream);
            if(obj != nullptr) {
                if(typeid(obj.get()) == typeid(PSB::IPSBChild)) {
                    dynamic_cast<PSB::IPSBChild *>(obj.get())->parent = list;
                }
                if(typeid(obj.get()) == typeid(PSB::IPSBSingleton)) {
                    dynamic_cast<PSB::IPSBSingleton *>(obj.get())
                        ->parents.push_back(list);
                }

                list->push_back(std::move(obj));
            }

            if(lazyLoad && offset == maxOffset) {
                endPos = stream->GetPosition();
            }
        }

        if(lazyLoad) {
            stream->SetPosition(endPos);
        }
        return std::move(list);
    }

    std::shared_ptr<PSB::PSBDictionary>
    PSBFile::loadObjects(TJS::tTJSBinaryStream *stream, bool lazyLoad) {
        const auto names = PSB::PSBList::loadIntoList(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream);
        const auto offsets = PSB::PSBList::loadIntoList(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream);
        auto pos = stream->GetPosition();
        auto dictionary = std::make_shared<PSB::PSBDictionary>(names.size());
        std::optional<std::uint32_t> maxOffset{};
        auto endPos = pos;
        if(lazyLoad && !offsets.empty()) {
            maxOffset = *std::max_element(offsets.cbegin(), offsets.cend());
        }

        for(size_t i = 0; i < names.size(); i++) {
            const auto nameIdx = names[i];
            if(nameIdx >= PSBFile::names.size()) {
                LOGGER->warn("Bad PSB format: at position:{}, name index {} >= "
                             "Names count ({}), skipping.",
                             pos, nameIdx, PSBFile::names.size());
                continue;
            }
            auto name = PSBFile::names[nameIdx];
            std::shared_ptr<PSB::IPSBValue> obj = nullptr;
            std::uint32_t offset = 0;
            if(i < offsets.size()) {
                offset = offsets[i];
                stream->SetPosition(pos + offset);
                obj = unpack(stream, lazyLoad);
            } else {
                LOGGER->warn("Bad PSB format: at position:{}, offset index {} "
                             ">= offsets count ({}), skipping.",
                             pos, i, offsets.size());
            }

            if(obj != nullptr) {
                auto *c = dynamic_cast<IPSBChild *>(obj.get());
                if(c) {
                    c->parent = dictionary;
                }
                auto *s = dynamic_cast<IPSBSingleton *>(obj.get());

                if(s) {
                    s->parents.push_back(dictionary);
                }

                dictionary->emplace(name, std::move(obj));
            }

            if(lazyLoad && offset == maxOffset) {
                endPos = stream->GetPosition();
            }
        }

        if(lazyLoad) {
            stream->SetPosition(endPos);
        }
        return std::move(dictionary);
    }

    std::shared_ptr<PSBDictionary>
    PSBFile::loadObjectsV1(TJS::tTJSBinaryStream *stream, bool lazyLoad) {
        auto offsets = PSB::PSBList::loadIntoList(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSBObjType::ArrayN1) + 1,
            stream);
        std::optional<std::uint32_t> maxOffset{};
        if(lazyLoad && !offsets.empty()) {
            maxOffset = *std::max_element(offsets.cbegin(), offsets.cend());
        }
        const auto pos = stream->GetPosition();
        auto endPos = pos;
        auto dictionary = std::make_shared<PSBDictionary>(offsets.size());
        for(const auto offset : offsets) {
            stream->SetPosition(pos + offset);
            PSBNumber nameIdx(static_cast<PSBObjType>(stream->ReadI8LE()),
                              stream);
            auto name = PSBFile::names[static_cast<int>(nameIdx)];
            auto obj = unpack(stream, lazyLoad);
            if(obj != nullptr) {

                auto *c = dynamic_cast<IPSBChild *>(obj.get());
                if(c) {
                    c->parent = dictionary;
                }
                auto *s = dynamic_cast<IPSBSingleton *>(obj.get());

                if(s) {
                    s->parents.push_back(dictionary);
                }

                dictionary->emplace(name, std::move(obj));
            }

            if(lazyLoad && offset == maxOffset) {
                endPos = stream->GetPosition();
            }
        }

        if(lazyLoad) {
            stream->SetPosition(endPos);
        }

        return std::move(dictionary);
    }

    std::shared_ptr<PSB::IPSBValue>
    PSBFile::unpack(TJS::tTJSBinaryStream *stream, bool lazyLoad) {

        auto typeByte = stream->ReadI8LE();
        // There is no need to check this, and it's slow
        // if (!Enum.IsDefined(typeof(PsbObjType), typeByte))
        //{
        //     return null;
        //     //throw new ArgumentOutOfRangeException($"0x{type:X2} is not a
        //     known type.");
        // }

        auto type = static_cast<PSB::PSBObjType>(typeByte);

        switch(type) {
            case PSB::PSBObjType::None:
                return nullptr;
            case PSB::PSBObjType::Null:
                return std::make_shared<PSB::PSBNull>();
            case PSB::PSBObjType::False:
            case PSB::PSBObjType::True:
                return std::make_shared<PSB::PSBBool>(type ==
                                                      PSB::PSBObjType::True);
            case PSB::PSBObjType::NumberN0:
                return std::make_shared<PSB::PSBNumber>(
                    0); // PsbNumber is not comparable!
            case PSB::PSBObjType::NumberN1:
            case PSB::PSBObjType::NumberN2:
            case PSB::PSBObjType::NumberN3:
            case PSB::PSBObjType::NumberN4:
            case PSB::PSBObjType::NumberN5:
            case PSB::PSBObjType::NumberN6:
            case PSB::PSBObjType::NumberN7:
            case PSB::PSBObjType::NumberN8:
            case PSB::PSBObjType::Float0:
            case PSB::PSBObjType::Float:
            case PSB::PSBObjType::Double:
                return std::make_shared<PSB::PSBNumber>(type, stream);
            case PSB::PSBObjType::ArrayN1:
            case PSB::PSBObjType::ArrayN2:
            case PSB::PSBObjType::ArrayN3:
            case PSB::PSBObjType::ArrayN4:
            case PSB::PSBObjType::ArrayN5:
            case PSB::PSBObjType::ArrayN6:
            case PSB::PSBObjType::ArrayN7:
            case PSB::PSBObjType::ArrayN8:
                return std::make_shared<PSB::PSBArray>(
                    typeByte -
                        static_cast<std::int8_t>(PSB::PSBObjType::ArrayN1) + 1,
                    stream);
            case PSB::PSBObjType::StringN1:
            case PSB::PSBObjType::StringN2:
            case PSB::PSBObjType::StringN3:
            case PSB::PSBObjType::StringN4: {
                auto str = std::make_unique<PSB::PSBString>(
                    typeByte -
                        static_cast<std::int8_t>(PSB::PSBObjType::StringN1) + 1,
                    stream);
                if(lazyLoad) {
                    const auto foundStr = std::find_if(
                        strings.begin(), strings.end(),
                        [&str](const PSB::PSBString &s) {
                            return s.index.has_value() && s.index == str->index;
                        });
                    if(foundStr == strings.end()) {
                        strings.emplace_back(*str);
                    } else {
                        str = std::make_unique<PSB::PSBString>(*foundStr);
                    }
                } else {
                    loadString(str, stream);
                }

                return std::move(str);
            }
            case PSB::PSBObjType::ResourceN1:
            case PSB::PSBObjType::ResourceN2:
            case PSB::PSBObjType::ResourceN3:
            case PSB::PSBObjType::ResourceN4:
            case PSB::PSBObjType::ExtraChunkN1:
            case PSB::PSBObjType::ExtraChunkN2:
            case PSB::PSBObjType::ExtraChunkN3:
            case PSB::PSBObjType::ExtraChunkN4: {
                const bool isExtra = type >= PSB::PSBObjType::ExtraChunkN1;
                auto &resList = isExtra ? extraResources : resources;
                PSB::PSBResource res{
                    typeByte -
                        static_cast<std::uint8_t>(
                            isExtra ? PSB::PSBObjType::ExtraChunkN1
                                    : PSB::PSBObjType::ResourceN1) +
                        1,
                    stream
                };
                res.isExtra = isExtra;
                // LoadResource(ref res, br); //No longer load Resources here
                const auto foundRes =
                    std::find_if(resList.begin(), resList.end(),
                                 [&res](const PSB::PSBResource &r) {
                                     return r.index == res.index;
                                 });

                if(foundRes == resList.end()) {
                    resList.push_back(res);
                } else {
                    res = *foundRes;
                }

                return std::make_shared<PSB::PSBResource>(std::move(res));
            }
            case PSB::PSBObjType::List:
                return loadList(stream, lazyLoad);
            case PSB::PSBObjType::Objects:
                return _header.version != 1 ? loadObjects(stream, lazyLoad)
                                            : loadObjectsV1(stream, lazyLoad);
            default:
                LOGGER->error("unknown psbObjType");
                return nullptr;
        }
    }

    bool PSBFile::loadPSBFile(const ttstr &filePath) {
        LOGGER->info("PSBFile::load path: {}", filePath.AsStdString());
        const std::unique_ptr<TJS::tTJSBinaryStream> stream{ TVPCreateStream(
            filePath) };
        if(!stream) {
            return false;
        }
        const size_t readSize = stream->GetSize();
        if(readSize < 9) {
            return false;
        }

        constexpr int signSize = 4;
        char sign[signSize];
        stream->Read(sign, signSize);

        if(10 < readSize && std::strcmp(sign, "MDF") == 0) {
            // auto originalLen = readSize - 8;
            // uLong compressedLen = compressBound(originalLen);
            // auto *compressed = new Bytef[compressedLen];
            // stream = uncompress(compressed, &compressedLen,
            //                       reinterpret_cast<const Bytef *>(buffer[2]),
            //                       originalLen);
            // if(code == 0) {
            //     delete[] buffer;
            //     return false;
            // }
            LOGGER->info("PSBFile::load MDF not implement!");
        }
        stream->SetPosition(0);
        _header = PSB::parsePSBHeader(stream.get());
        if(std::strcmp(_header.signature, PSB::PsbSignature) != 0) {
            return false;
        }

        if(_header.isEncrypted()) {
            LOGGER->critical("psb file is encrypted");
            return false;
        }

        if(_header.version > 3) {
            LOGGER->critical("not support psb file format version > 3");
            return false;
        }

        // Pre Load Strings
        stream->SetPosition(_header.offsetStrings);
        stringOffsets = PSB::PSBArray(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream.get());

        // Load Names
        if(_header.version == 1) {
            // don't believe HeaderLength
            if(_header.length >= stream->GetSize()) {
                _header.length = _header.GetHeaderLength();
            }
            stream->SetPosition(_header.length);
            nameIndexes = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            loadKeys(stream.get());
        } else {
            stream->SetPosition(_header.offsetNames);
            charset = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            namesData = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            nameIndexes = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            loadNames();
        }

        // Pre Load Resources (Chunks)
        stream->SetPosition(_header.offsetChunkOffsets);
        chunkOffsets = PSB::PSBArray(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream.get());
        stream->SetPosition(_header.offsetChunkLengths);
        chunkLengths = PSB::PSBArray(
            stream->ReadI8LE() -
                static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
            stream.get());

        resources.reserve(chunkLengths.value.size());

        if(_header.version >= 4) {
            // Pre Load Extra Resources (Chunks)
            stream->SetPosition(_header.offsetExtraChunkOffsets);
            extraChunkOffsets = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            stream->SetPosition(_header.offsetExtraChunkLengths);
            extraChunkLengths = PSB::PSBArray(
                stream->ReadI8LE() -
                    static_cast<std::uint8_t>(PSB::PSBObjType::ArrayN1) + 1,
                stream.get());
            extraResources.reserve(extraChunkLengths.value.size());
        }
        // Load Entries
        stream->SetPosition(_header.offsetEntries);
        auto obj = unpack(stream.get());
        if(!obj) {
            LOGGER->error("Can not parse objects");
        }

        _root = std::move(obj);
        // Load Resource
        for(auto &res : resources) {
            loadResource(res, stream.get());
        }

        if(_header.version >= 4) {
            for(auto &res : extraResources) {
                loadExtraResource(res, stream.get());
            }
        }

        afterLoad();

        return true;
    }

    void PSBFile::loadResource(PSBResource &res,
                               TJS::tTJSBinaryStream *stream) {
        if(!res.index.has_value()) {
            throw "Resource Index invalid";
        }

        auto index = static_cast<int>(res.index.value());
        auto offset = chunkOffsets[index];
        auto length = chunkLengths[index];
        stream->SetPosition(_header.offsetChunkData + offset);
        std::vector<std::uint8_t> tmp(length);
        stream->ReadBuffer(tmp.data(), length);
        res.data = std::move(tmp);
    }

    void PSBFile::loadExtraResource(PSBResource &res,
                                    TJS::tTJSBinaryStream *stream) {
        if(!res.index.has_value()) {
            throw "Extra Resource Index invalid";
        }

        auto index = static_cast<int>(res.index.value());
        auto offset = extraChunkOffsets[index];
        auto length = extraChunkLengths[index];
        stream->SetPosition(_header.offsetExtraChunkData + offset);
        std::vector<std::uint8_t> tmp(length);
        stream->ReadBuffer(tmp.data(), length);
        res.data = std::move(tmp);
    }

    void PSBFile::afterLoad() {
        const auto intMax = static_cast<std::uint32_t>(INT_MAX);
        std::sort(strings.begin(), strings.end(),
                  [intMax](const PSB::PSBString &r1, const PSB::PSBString &r2) {
                      return r1.index.value_or(intMax) <
                          r2.index.value_or(intMax);
                  });
        std::sort(
            resources.begin(), resources.end(),
            [intMax](const PSB::PSBResource &r1, const PSB::PSBResource &r2) {
                return r1.index.value_or(intMax) < r2.index.value_or(intMax);
            });
        std::sort(
            extraResources.begin(), extraResources.end(),
            [intMax](const PSB::PSBResource &r1, const PSB::PSBResource &r2) {
                return r1.index.value_or(intMax) < r2.index.value_or(intMax);
            });
        inferType();
    }
} // namespace PSB
