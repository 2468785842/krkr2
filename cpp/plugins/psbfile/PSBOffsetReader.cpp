#include "PSBFile.h"

#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "tjsArray.h"
#include "tjsDictionary.h"
#include "ncbind.hpp"
#include "xp3filter.h"

#include "log/TVPLog.h"

namespace PSB {
    namespace {

        void parsePSBArray(std::vector<std::uint32_t> *target, int n,
                           TJS::tTJSBinaryStream *stream) {
            target->clear();
            std::uint32_t count = 0;
            stream->ReadBuffer(&count, n);
            const auto entryLength = static_cast<std::uint8_t>(
                stream->ReadI8LE() -
                static_cast<std::int8_t>(PSBObjType::NumberN8));
            target->reserve(count);
            for(std::uint32_t i = 0; i < count; ++i) {
                std::uint32_t val = 0;
                stream->ReadBuffer(&val, entryLength);
                target->push_back(val);
            }
        }

    } // namespace

    std::uint32_t PSBFile::readListInfo(std::vector<std::uint32_t> *target) {
        parsePSBArray(target,
                      _stream->ReadI8LE() -
                          static_cast<std::int8_t>(PSBObjType::ArrayN1) + 1,
                      _stream.get());
        return _stream->GetPosition();
    }

    void PSBFile::refreshListInfo(std::vector<std::uint32_t> *target1,
                                  std::vector<std::uint32_t> *target2) {
        target2->resize(target1->size());
        const std::uint32_t basePos = _stream->GetPosition();
        for(std::uint32_t i = 0; i < target1->size(); ++i) {
            _stream->SetPosition(basePos + target1->at(i));
            target2->at(i) = _stream->ReadI8LE();
            target1->at(i) += 4;
        }
    }

    bool PSBFile::parseObject(std::map<std::string, std::uint32_t> &output,
                              std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto typeByte = _stream->ReadI8LE();
        if(static_cast<PSBObjType>(typeByte) == PSBObjType::Null) {
            return false;
        }
        if(static_cast<PSBObjType>(typeByte) != PSBObjType::Objects) {
            G_PluginLog.warn("Invalidate Object at offset {}", objOffset);
            return false;
        }
        output.clear();

        std::vector<std::uint32_t> objsOffset;
        std::vector<std::uint32_t> objsNamesIdx;
        std::uint32_t tmpOffset = 0;
        if(_header.version == 1) {
            tmpOffset = readListInfo(&objsOffset);
            refreshListInfo(&objsOffset, &objsNamesIdx);
        } else {
            readListInfo(&objsNamesIdx);
            tmpOffset = readListInfo(&objsOffset);
        }

        for(std::int32_t i = 0;
            i < static_cast<std::int32_t>(objsNamesIdx.size()); ++i) {
            output.emplace(names.at(objsNamesIdx.at(i)),
                           tmpOffset + objsOffset.at(i));
        }
        return true;
    }

    bool PSBFile::parseNumber(std::int64_t &output, std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto type = static_cast<PSBObjType>(_stream->ReadI8LE());
        if(type == PSBObjType::Null) {
            return false;
        }
        if(readIntegerValue(type, _stream.get(), output)) {
            return true;
        }
        if(type == PSBObjType::True) {
            output = 1;
            return true;
        }
        if(type == PSBObjType::False) {
            output = 0;
            return true;
        }

        double realVal = 0;
        if(readRealValue(type, _stream.get(), realVal)) {
            output = static_cast<std::int64_t>(realVal);
            return true;
        }

        std::string retStr;
        if(parseString(retStr, objOffset)) {
            try {
                output = std::stoll(retStr);
                return true;
            } catch(...) {
                G_PluginLog.warn("Invalidate Number at offset {}", objOffset);
                return false;
            }
        }
        G_PluginLog.warn("Invalidate Number at offset {}", objOffset);
        return false;
    }

    bool PSBFile::parseReal(double &output, std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto type = static_cast<PSBObjType>(_stream->ReadI8LE());
        if(readRealValue(type, _stream.get(), output)) {
            return true;
        }

        std::string retStr;
        if(parseString(retStr, objOffset)) {
            try {
                output = std::stod(retStr);
                return true;
            } catch(...) {
                G_PluginLog.warn("Invalidate Real at offset {}", objOffset);
                return false;
            }
        }
        G_PluginLog.warn("Invalidate Real at offset {}", objOffset);
        return false;
    }

    bool PSBFile::parseString(std::string &output, std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto typeByte = _stream->ReadI8LE();
        switch(static_cast<PSBObjType>(typeByte)) {
            case PSBObjType::StringN1:
            case PSBObjType::StringN2:
            case PSBObjType::StringN3:
            case PSBObjType::StringN4: {
                std::int32_t idx = 0;
                _stream->ReadBuffer(
                    &idx,
                    typeByte - static_cast<std::int8_t>(PSBObjType::StringN1) +
                        1);
                _stream->SetPosition(_header.offsetStringsData +
                                     stringOffsets.value.at(idx));
                output = Extension::readStringZeroTrim(_stream.get());
                return true;
            }
            default:
                G_PluginLog.warn("Invalidate String at offset {}", objOffset);
                return false;
        }
    }

    bool PSBFile::parseList(std::vector<std::uint32_t> &output,
                            std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto typeByte = _stream->ReadI8LE();
        if(static_cast<PSBObjType>(typeByte) == PSBObjType::Null) {
            return false;
        }
        if(static_cast<PSBObjType>(typeByte) != PSBObjType::List) {
            G_PluginLog.warn("Invalidate List at offset {}", objOffset);
            return false;
        }
        std::vector<std::uint32_t> objsOffset;
        const std::uint32_t tmpOffset = readListInfo(&objsOffset);
        output.clear();
        output.reserve(objsOffset.size());
        for(const auto offset : objsOffset) {
            output.push_back(tmpOffset + offset);
        }
        return true;
    }

    bool PSBFile::parseResource(std::int32_t &id, std::uint32_t objOffset) {
        _stream->SetPosition(objOffset);
        const auto type = static_cast<PSBObjType>(_stream->ReadI8LE());
        if(readResourceIndex(type, _stream.get(), id)) {
            return true;
        }
        G_PluginLog.warn("Invalidate Resource at offset {}", objOffset);
        return false;
    }

    tTJSVariant PSBFile::readAllObjs(const ttstr & /*key*/,
                                     tjs_uint32 objOffset) {
        _stream->SetPosition(objOffset);
        const auto typeByte = _stream->ReadI8LE();
        switch(static_cast<PSBObjType>(typeByte)) {
            case PSBObjType::None:
            case PSBObjType::Null:
                return tTJSVariant();
            case PSBObjType::False:
            case PSBObjType::True:
                return tTJSVariant(typeByte ==
                                   static_cast<std::int8_t>(PSBObjType::True));
            case PSBObjType::NumberN0:
                return tTJSVariant(0);
            case PSBObjType::NumberN1: {
                tjs_int8 val8 = 0;
                _stream->ReadBuffer(&val8, 1);
                return tTJSVariant(val8);
            }
            case PSBObjType::NumberN2: {
                tjs_int16 val16 = 0;
                _stream->ReadBuffer(&val16, 2);
                return tTJSVariant(val16);
            }
            case PSBObjType::NumberN3: {
                tjs_int32 val32 = 0;
                _stream->ReadBuffer(&val32, 3);
                val32 |= ((val32 & 0x800000) == 0 ? 0 : 0xFFFF0000);
                return tTJSVariant(val32);
            }
            case PSBObjType::NumberN4: {
                tjs_int32 val32 = 0;
                _stream->ReadBuffer(&val32, 4);
                return tTJSVariant(val32);
            }
            case PSBObjType::NumberN5: {
                tjs_int64 val64 = 0;
                _stream->ReadBuffer(&val64, 5);
                val64 |=
                    ((val64 & 0x8000000000LL) == 0 ? 0 : 0xFFFFFFFF00000000LL);
                return tTJSVariant(val64);
            }
            case PSBObjType::NumberN6: {
                tjs_int64 val64 = 0;
                _stream->ReadBuffer(&val64, 6);
                val64 |=
                    ((val64 & 0x800000000000LL) == 0 ? 0
                                                     : 0xFFFFFF0000000000LL);
                return tTJSVariant(val64);
            }
            case PSBObjType::NumberN7: {
                tjs_int64 val64 = 0;
                _stream->ReadBuffer(&val64, 7);
                val64 |=
                    ((val64 & 0x80000000000000LL) == 0 ? 0
                                                       : 0xFFFF000000000000LL);
                return tTJSVariant(val64);
            }
            case PSBObjType::NumberN8: {
                tjs_int64 val64 = 0;
                _stream->ReadBuffer(&val64, 8);
                return tTJSVariant(val64);
            }
            case PSBObjType::Float0:
                return tTJSVariant(0.0f);
            case PSBObjType::Float: {
                tjs_uint8 buffer[4];
                _stream->ReadBuffer(buffer, 4);
                float tmp = 0.0f;
                std::memcpy(&tmp, buffer, 4);
                return tTJSVariant(static_cast<tjs_real>(tmp));
            }
            case PSBObjType::Double: {
                tjs_uint8 buffer[8];
                _stream->ReadBuffer(buffer, 8);
                double tmp = 0.0;
                std::memcpy(&tmp, buffer, 8);
                return tTJSVariant(static_cast<tjs_real>(tmp));
            }
            case PSBObjType::ArrayN1:
            case PSBObjType::ArrayN2:
            case PSBObjType::ArrayN3:
            case PSBObjType::ArrayN4:
            case PSBObjType::ArrayN5:
            case PSBObjType::ArrayN6:
            case PSBObjType::ArrayN7:
            case PSBObjType::ArrayN8: {
                std::vector<tjs_uint32> tmp;
                parsePSBArray(
                    &tmp,
                    typeByte - static_cast<tjs_int8>(PSBObjType::ArrayN1) + 1,
                    _stream.get());
                iTJSDispatch2 *array = TJSCreateArrayObject();
                for(const auto i : tmp) {
                    tTJSVariant item(static_cast<tjs_int32>(i));
                    tTJSVariant *args[] = { &item };
                    static tjs_uint addHint = 0;
                    array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args,
                                    array);
                }
                tTJSVariant result(array, array);
                array->Release();
                return result;
            }
            case PSBObjType::StringN1:
            case PSBObjType::StringN2:
            case PSBObjType::StringN3:
            case PSBObjType::StringN4: {
                std::string str;
                if(parseString(str, objOffset)) {
                    return tTJSVariant(str);
                }
                return tTJSVariant("");
            }
            case PSBObjType::ResourceN1:
            case PSBObjType::ResourceN2:
            case PSBObjType::ResourceN3:
            case PSBObjType::ResourceN4:
            case PSBObjType::ExtraChunkN1:
            case PSBObjType::ExtraChunkN2:
            case PSBObjType::ExtraChunkN3:
            case PSBObjType::ExtraChunkN4:
                return tTJSVariant();
            case PSBObjType::List: {
                std::vector<std::uint32_t> objsOffset;
                const std::uint32_t tmpOffset = readListInfo(&objsOffset);
                iTJSDispatch2 *array = TJSCreateArrayObject();
                for(const auto offset : objsOffset) {
                    tTJSVariant obj = readAllObjs(ttstr(), tmpOffset + offset);
                    tTJSVariant *args[] = { &obj };
                    static tjs_uint addHint = 0;
                    array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args,
                                    array);
                }
                tTJSVariant result(array, array);
                array->Release();
                return result;
            }
            case PSBObjType::Objects: {
                std::vector<std::uint32_t> objsOffset;
                std::vector<std::uint32_t> objsNamesIdx;
                std::uint32_t tmpOffset = 0;
                if(_header.version == 1) {
                    tmpOffset = readListInfo(&objsOffset);
                    refreshListInfo(&objsOffset, &objsNamesIdx);
                } else {
                    readListInfo(&objsNamesIdx);
                    tmpOffset = readListInfo(&objsOffset);
                }

                iTJSDispatch2 *dsp = TJSCreateDictionaryObject();
                for(tjs_int i = 0;
                    i < static_cast<tjs_int>(objsNamesIdx.size()); ++i) {
                    const ttstr keyName = names.at(objsNamesIdx.at(i));
                    tTJSVariant obj =
                        readAllObjs(keyName, tmpOffset + objsOffset.at(i));
                    dsp->PropSet(TJS_MEMBERENSURE, keyName.c_str(), nullptr,
                                 &obj, dsp);
                }
                tTJSVariant result(dsp, dsp);
                dsp->Release();
                return result;
            }
            default:
                G_PluginLog.error("unknown psbObjType at offset {}", objOffset);
                return tTJSVariant();
        }
    }

    std::uint64_t PSBFile::getDataSize() const {
        return _stream ? _stream->GetSize() : 0;
    }

    void PSBFile::readAllData(std::uint8_t *output,
                              std::uint32_t outputLen) const {
        if(!_stream) {
            return;
        }
        _stream->SetPosition(0);
        _stream->ReadBuffer(output, outputLen);
    }

    bool PSBFile::readChunkData(std::int32_t chunkId, std::uint8_t *buffer,
                                std::uint32_t size) const {
        if(chunkId < 0 ||
           chunkId >= static_cast<std::int32_t>(chunkLengths.value.size())) {
            return false;
        }
        const auto length = chunkLengths.value.at(chunkId);
        if(size < length) {
            return false;
        }
        _stream->SetPosition(_header.offsetChunkData +
                             chunkOffsets.value.at(chunkId));
        _stream->ReadBuffer(buffer, length);
        return true;
    }

    const PSBArray &PSBFile::getChunkOffsets() const { return chunkOffsets; }

    const PSBArray &PSBFile::getChunkLengths() const { return chunkLengths; }

} // namespace PSB
