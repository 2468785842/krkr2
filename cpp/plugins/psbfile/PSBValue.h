//
// Created by lidong on 25-3-15.
//
#pragma once
#include <cstddef>
#include <optional>
#include <utility>
#include <unordered_map>

#include <fmt/format.h>

#include "tjs.h"
#include "BitConverter.h"
#include "PSBExtension.h"

namespace PSB {

    enum class PSBObjType : char {
        None = 0x0,
        Null = 0x1,
        False = 0x2,
        True = 0x3,

        // int
        NumberN0 = 0x4,
        NumberN1 = 0x5,
        NumberN2 = 0x6,
        NumberN3 = 0x7,
        NumberN4 = 0x8,
        NumberN5 = 0x9,
        NumberN6 = 0xA,
        NumberN7 = 0xB,
        NumberN8 = 0xC,

        // array N(NUMBER) is count mask
        ArrayN1 = 0xD,
        ArrayN2 = 0xE,
        ArrayN3 = 0xF,
        ArrayN4 = 0x10,
        ArrayN5 = 0x11,
        ArrayN6 = 0x12,
        ArrayN7 = 0x13,
        ArrayN8 = 0x14,

        // index of key name, only used in PSBv1 (according to GMMan's doc)
        KeyNameN1 = 0x11,
        KeyNameN2 = 0x12,
        KeyNameN3 = 0x13,
        KeyNameN4 = 0x14,

        // index of strings table
        StringN1 = 0x15,
        StringN2 = 0x16,
        StringN3 = 0x17,
        StringN4 = 0x18,

        // resource of thunk
        ResourceN1 = 0x19,
        ResourceN2 = 0x1A,
        ResourceN3 = 0x1B,
        ResourceN4 = 0x1C,

        // fpu value
        Float0 = 0x1D,
        Float = 0x1E,
        Double = 0x1F,

        // objects
        List = 0x20, // object list
        Objects = 0x21, // object dictionary

        ExtraChunkN1 = 0x22,
        ExtraChunkN2 = 0x23,
        ExtraChunkN3 = 0x24,
        ExtraChunkN4 = 0x25,

    };

    class IPSBValue {
    public:
        virtual ~IPSBValue() = default;
        virtual PSBObjType getType() = 0;
    };

    class IPSBCollection;
    class IPSBChild : public IPSBValue {
    public:
        std::shared_ptr<IPSBCollection> parent;

        std::string path;
    };

    class IPSBCollection : public IPSBChild {
    public:
        virtual std::shared_ptr<IPSBValue> operator[](int index) = 0;
        virtual std::shared_ptr<IPSBValue> operator[](const std::string &s) = 0;
    };

    class IPSBSingleton {
    public:
        std::vector<std::shared_ptr<IPSBCollection>> parents;
    };

    struct PSBNull : IPSBValue {
        PSBObjType getType() override { return PSB::PSBObjType::Null; }
    };

    struct PSBBool : IPSBValue {
        bool value{};
        explicit PSBBool(bool value = false) { this->value = value; }

        PSBObjType getType() override {
            return value ? PSB::PSBObjType::True : PSB::PSBObjType::False;
        }
    };

    enum class PSBNumberType {
        Int,
        Long,
        Float,
        Double,
    };

    struct PSBNumber : IPSBValue {

        std::vector<std::uint8_t> data;

        PSBNumberType numberType;

        explicit PSBNumber() : data(8), numberType(PSBNumberType::Long) {}

        explicit PSBNumber(int val) :
            data(BitConverter::toByteArray(val)),
            numberType(PSBNumberType::Int) {}

        explicit PSBNumber(std::vector<std::uint8_t> data, PSBNumberType type) :
            data(std::move(data)), numberType(type) {}
        explicit PSBNumber(PSBObjType objType, TJS::tTJSBinaryStream *stream);

        [[nodiscard]] std::int64_t getLongValue() const;

        [[nodiscard]] float getFloatValue() const;

        template <typename T>
        void setValue(T value) {
            data = BitConverter::toByteArray(value);
        }

        template <typename T>
        [[nodiscard]] T getValue() const {
            return BitConverter::fromByteArray<T>(data);
        }

        PSBObjType getType() override;

        explicit operator int() const { return this->getValue<int>(); }
    };

    struct PSBArray : IPSBValue {
        std::uint8_t entryLength{ 4 };
        std::vector<std::uint32_t> value{};
        explicit PSBArray() = default;
        explicit PSBArray(int n, TJS::tTJSBinaryStream *stream) {
            if(n < 0 || n > 8) {
                throw "bad length type size";
            }

            std::uint32_t count{};
            stream->ReadBuffer(&count, n);
            if(count > INT32_MAX) {
                throw "Long array is not supported yet";
            }

            entryLength = stream->ReadI8LE() -
                static_cast<std::uint32_t>(PSBObjType::NumberN8);
            value.reserve(count);

            const auto shouldBeLength = entryLength * static_cast<int>(count);
            auto buffer = std::make_unique<std::uint8_t[]>(shouldBeLength);

            stream->Read(buffer.get(),
                         shouldBeLength); // WARN: the actual buffer.Length >=
                                          // shouldBeLength
            for(int i = 0; i < count; i++) {
                std::uint32_t result = 0;
                for(std::uint8_t j = 0; j < entryLength; ++j) {
                    result |= buffer.get()[i * entryLength + j] << j * 8;
                }
                value.push_back(result);
            }
        }

        std::uint32_t operator[](int index) const { return value[index]; }

        PSBObjType getType() override {
            switch(Extension::getSize(value.size())) {
                case 0:
                case 1:
                    return PSBObjType::ArrayN1;
                case 2:
                    return PSBObjType::ArrayN2;
                case 3:
                    return PSBObjType::ArrayN3;
                case 4:
                    return PSBObjType::ArrayN4;
                case 5:
                    return PSBObjType::ArrayN5;
                case 6:
                    return PSBObjType::ArrayN6;
                case 7:
                    return PSBObjType::ArrayN7;
                case 8:
                    return PSBObjType::ArrayN8;
                default:
                    throw "Not a valid array";
            }
        }
    };

    struct PSBString : IPSBValue {
        std::optional<std::uint32_t> index{};

        std::string value;

        explicit PSBString() = default;
        explicit PSBString(int n, TJS::tTJSBinaryStream *stream) {
            std::uint32_t tmp{};
            stream->Read(&tmp, n);
            index = tmp;
        }

        explicit PSBString(std::string value = "",
                           std::optional<std::uint32_t> index = {}) :
            index(index),
            value(std::move(value)) {}

        PSBObjType getType() override {

            switch(Extension::getSize(index.value_or(0))) {
                case 0:
                case 1:
                    return PSBObjType::StringN1;
                case 2:
                    return PSBObjType::StringN2;
                case 3:
                    return PSBObjType::StringN3;
                case 4:
                    return PSBObjType::StringN4;
                default:
                    throw "String index has wrong size";
            }
        }
    };

    struct PSBResource : IPSBValue, IPSBSingleton {
        bool isExtra = false;
        std::optional<std::uint32_t> index{};
        std::vector<std::uint8_t> data{};

        explicit PSBResource() = default;
        explicit PSBResource(int n, TJS::tTJSBinaryStream *stream) {
            std::uint32_t tmp{};
            stream->Read(&tmp, n);
            index = tmp;
        }

        PSBObjType getType() override {

            switch(Extension::getSize(index.value_or(0))) {
                case 0:
                case 1:
                    return isExtra ? PSBObjType::ExtraChunkN1
                                   : PSBObjType::ResourceN1;
                case 2:
                    return isExtra ? PSBObjType::ExtraChunkN2
                                   : PSBObjType::ResourceN2;
                case 3:
                    return isExtra ? PSBObjType::ExtraChunkN3
                                   : PSBObjType::ResourceN3;
                case 4:
                    return isExtra ? PSBObjType::ExtraChunkN4
                                   : PSBObjType::ResourceN4;
                default:
                    throw "Not a valid resource";
            }
        }
    };

    class PSBDictionary
        : public std::unordered_map<std::string, std::shared_ptr<IPSBValue>>,
          public IPSBCollection {
        using inherit =
            std::unordered_map<std::string, std::shared_ptr<IPSBValue>>;

    public:
        explicit PSBDictionary(int capacity) : inherit(capacity) {}
        explicit PSBDictionary() : inherit() {}

        template <typename T>
        bool tryGetPsbValue(const std::string &key, T *&value) {
            auto it = inherit::find(key);
            if(it != inherit::end()) {
                value = dynamic_cast<T *>(it->second);
                return (value != nullptr);
            }
            value = nullptr;
            return false;
        }

        IPSBCollection *parent = nullptr;

        std::shared_ptr<IPSBValue> operator[](const std::string &s) override {
            const auto tmp = inherit::find(s);
            return tmp != inherit::end() ? tmp->second : nullptr;
        }

        std::shared_ptr<IPSBValue> operator[](int i) override {
            return operator[](fmt::format("{}", i));
        }

        std::shared_ptr<IPSBValue> operator[](const std::string &s) const {
            const auto tmp = inherit::find(s);
            return tmp != inherit::end() ? tmp->second : nullptr;
        }

        std::shared_ptr<IPSBValue> operator[](int i) const {
            return operator[](fmt::format("{}", i));
        }

        PSBObjType getType() override { return PSBObjType::Objects; }

        void unionWith(const PSBDictionary &dic) {
            for(const auto &[key, _] : dic) {
                if(inherit::find(key) != inherit::end()) {
                    auto *childDic = dynamic_cast<PSBDictionary *>(
                        inherit::operator[](key).get());
                    auto *otherDic =
                        dynamic_cast<PSBDictionary *>(dic[key].get());
                    if(childDic && otherDic) {
                        childDic->unionWith(*otherDic);
                    }
                } else {
                    inherit::emplace(key, dic[key]);
                }
            }
        }
    };

    class PSBList : public IPSBCollection,
                    public std::vector<std::shared_ptr<IPSBValue>> {
        using inherit = std::vector<std::shared_ptr<IPSBValue>>;

    public:
        std::uint8_t entryLength{ 4 };

        explicit PSBList(size_t capacity) : inherit(capacity) {}

        std::shared_ptr<IPSBValue> operator[](int index) override {
            return inherit::operator[](index);
        }

        std::shared_ptr<IPSBValue> operator[](const std::string &s) override {
            assert(false && "not implement method: operator[](std::string)!");
            return nullptr;
        }

        PSBObjType getType() override { return PSBObjType::List; }

        static std::vector<std::uint32_t>
        loadIntoList(int n, TJS::tTJSBinaryStream *stream) {
            if(n < 0 || n > 8) {
                throw "Long array is not supported yet";
            }

            std::uint32_t count{};
            stream->ReadBuffer(&count, n);
            if(count > INT32_MAX) {
                throw "Long array is not supported yet";
            }

            const std::uint8_t entryLength = stream->ReadI8LE() -
                static_cast<std::uint32_t>(PSBObjType::NumberN8);
            std::vector<std::uint32_t> list(count);
            // for (int i = 0; i < count; i++)
            //{
            //     list.Add(br.ReadBytes(entryLength).UnzipUInt());
            // }

            const auto shouldBeLength = entryLength * static_cast<int>(count);
            const auto buffer =
                std::make_unique<std::uint8_t[]>(shouldBeLength);

            stream->Read(buffer.get(),
                         shouldBeLength); // WARN: the actual buffer.Length >=
                                          // shouldBeLength
            for(int i = 0; i < count; i++) {
                std::uint32_t result = 0;
                for(std::uint8_t j = 0; j < entryLength; ++j) {
                    result |= buffer.get()[i * entryLength + j] << j * 8;
                }
                list.push_back(result);
            }

            return list;
        }
    };
} // namespace PSB