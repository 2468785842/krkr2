//
// Created by lidong on 25-3-15.
//
#pragma once

#include <optional>
#include <utility>
#include <unordered_map>

#include <fmt/format.h>

#include "tjs.h"
#include "BitConverter.h"
#include "Consts.h"
#include "PSBExtension.h"

namespace PSB {

    enum class PSBObjType : unsigned char {
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
        [[nodiscard]] virtual PSBObjType getType() const = 0;
        [[nodiscard]] virtual std::string toString() = 0;
        [[nodiscard]] virtual tTJSVariant toTJSVal() const = 0;
    };

    class IPSBCollection;
    class IPSBChild : public IPSBValue {
    public:
        std::shared_ptr<IPSBCollection> parent;

        std::string path;
    };

    class IPSBCollection : public IPSBChild {
    public:
        using K = std::string;
        using V = std::shared_ptr<IPSBValue>;

        virtual V operator[](int index) = 0;
        virtual V operator[](const K &key) = 0;

        virtual V operator[](int index) const = 0;
        virtual V operator[](const K &key) const = 0;
    };

    class IPSBSingleton {
    public:
        std::vector<std::shared_ptr<IPSBCollection>> parents;
    };

    struct PSBNull : IPSBValue {
        [[nodiscard]] PSBObjType getType() const override {
            return PSB::PSBObjType::Null;
        }
        std::string toString() override { return "null"; }

        [[nodiscard]] tTJSVariant toTJSVal() const override;
    };

    struct PSBBool : IPSBValue {
        bool value{};
        explicit PSBBool(bool value = false) { this->value = value; }

        [[nodiscard]] PSBObjType getType() const override {
            return value ? PSB::PSBObjType::True : PSB::PSBObjType::False;
        }

        std::string toString() override { return value ? "true" : "false"; }

        [[nodiscard]] tTJSVariant toTJSVal() const override;
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

        [[nodiscard]] tTJSVariant toTJSVal() const override;

        [[nodiscard]] std::string toString() override {
            switch(numberType) {
                case PSBNumberType::Int:
                    return std::to_string(getValue<int>());
                case PSBNumberType::Float:
                    return std::to_string(getValue<float>());
                case PSBNumberType::Double:
                    return std::to_string(getValue<double>());
                case PSBNumberType::Long:
                default:
                    return std::to_string(getValue<long>());
            }
        }

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

        [[nodiscard]] PSBObjType getType() const override;

        explicit operator int() const {
            if(this->numberType != PSBNumberType::Int) {
                throw std::runtime_error("not int type!");
            }
            return this->getValue<int>();
        }
    };

    struct PSBArray : IPSBValue {
        std::uint8_t entryLength{ 4 };
        std::vector<std::uint32_t> value{};
        explicit PSBArray() = default;
        explicit PSBArray(int n, TJS::tTJSBinaryStream *stream) {
            if(n < 0 || n > 8) {
                throw std::runtime_error("PSBArray bad length type size");
            }

            std::uint32_t count{};
            stream->ReadBuffer(&count, n);
            if(count > INT32_MAX) {
                throw std::runtime_error("Long array is not supported yet");
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

        [[nodiscard]] tTJSVariant toTJSVal() const override;

        std::string toString() override {
            return fmt::format("Array[{}]", value.size());
        }

        std::uint32_t operator[](int index) const { return value[index]; }

        [[nodiscard]] PSBObjType getType() const override {
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
                    throw std::runtime_error("Not a valid array");
            }
        }
    };

    struct PSBString : IPSBValue {
        std::optional<std::uint32_t> index{};

        std::string value;

        explicit PSBString(int n, TJS::tTJSBinaryStream *stream) {
            std::uint32_t tmp{};
            stream->Read(&tmp, n);
            index = tmp;
        }

        explicit PSBString(std::string value = "",
                           std::optional<std::uint32_t> index = {}) :
            index(index), value(std::move(value)) {}


        [[nodiscard]] tTJSVariant toTJSVal() const override;

        std::string toString() override { return value; }

        [[nodiscard]] PSBObjType getType() const override {

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
                    throw std::runtime_error("String index has wrong size");
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

        PSBResource(const PSBResource &) = default;

        [[nodiscard]] tTJSVariant toTJSVal() const override;

        std::string toString() override {
            return fmt::format("{{({})}}{{{}}}",
                               isExtra ? Consts::ExtraResourceIdentifier
                                       : Consts::ResourceIdentifier,
                               index.value_or(-1));
        }

        [[nodiscard]] PSBObjType getType() const override {

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
                    throw std::runtime_error("Not a valid resource");
            }
        }
    };

    class PSBDictionary : public IPSBCollection {
        using Map = std::unordered_map<K, V>;

    public:
        explicit PSBDictionary() = default;
        explicit PSBDictionary(int capacity) : _map(capacity) {}

        template <typename T>
        bool tryGetPsbValue(const K &key, T *&val) {
            auto it = _map.find(key);
            if(it != _map.end()) {
                val = dynamic_cast<T *>(it->second);
                return val != nullptr;
            }
            val = nullptr;
            return false;
        }

        std::string toString() override {
            return fmt::format("Dictionary[{}]", _map.size());
        }

        void emplace(const K &key, const V &val) { _map.emplace(key, val); }

        [[nodiscard]] auto begin() const { return _map.begin(); }
        [[nodiscard]] auto end() const { return _map.end(); }
        [[nodiscard]] auto find(const K &key) const { return _map.find(key); }

        [[nodiscard]] V operator[](int index) override { return get(index); }
        [[nodiscard]] V operator[](const K &key) override { return get(key); }
        [[nodiscard]] V operator[](int index) const override {
            return get(index);
        }
        [[nodiscard]] V operator[](const K &key) const override {
            return get(key);
        }

        [[nodiscard]] PSBObjType getType() const override {
            return PSBObjType::Objects;
        }

        void unionWith(const PSBDictionary &dic) {
            for(const auto &[key, val] : dic) {
                if(_map.find(key) != _map.end()) {
                    auto *childDic =
                        dynamic_cast<PSBDictionary *>(_map[key].get());
                    auto *otherDic = dynamic_cast<PSBDictionary *>(val.get());
                    if(childDic && otherDic) {
                        childDic->unionWith(*otherDic);
                    }
                } else {
                    _map.emplace(key, val);
                }
            }
        }

        [[nodiscard]] tTJSVariant toTJSVal() const override;

    private:
        [[nodiscard]] V get(const K &key) const {
            auto it = _map.find(key);
            return it != _map.end() ? it->second : nullptr;
        }

        [[nodiscard]] V get(int index) const {
            return operator[](fmt::format("{}", index));
        }

    private:
        Map _map{};
        // IPSBCollection *parent = nullptr;
    };

    class PSBList : public IPSBCollection {

        using V = std::shared_ptr<IPSBValue>;
        using Vec = std::vector<V>;

    public:
        std::uint8_t entryLength{ 4 };

        explicit PSBList(size_t capacity) { _vec.reserve(capacity); }

        [[nodiscard]] tTJSVariant toTJSVal() const override;

        std::string toString() override {
            return fmt::format("List[{}]", _vec.size());
        }

        void push_back(const V &val) { _vec.push_back(val); }
        [[nodiscard]] auto begin() const { return _vec.begin(); }
        [[nodiscard]] auto end() const { return _vec.end(); }

        [[nodiscard]] V operator[](int index) override { return get(index); }
        [[nodiscard]] V operator[](const K &key) override { return get(key); }
        [[nodiscard]] V operator[](int index) const override {
            return get(index);
        }
        [[nodiscard]] V operator[](const K &key) const override {
            return get(key);
        }

        [[nodiscard]] PSBObjType getType() const override {
            return PSBObjType::List;
        }

        [[nodiscard]] size_t size() const { return _vec.size(); }

        static std::vector<std::uint32_t>
        loadIntoList(int n, TJS::tTJSBinaryStream *stream) {
            if(n < 0 || n > 8) {
                throw std::runtime_error("Long array is not supported yet");
            }

            std::uint32_t count{};
            stream->ReadBuffer(&count, n);
            if(count > INT32_MAX) {
                throw std::runtime_error("Long array is not supported yet");
            }

            const std::uint8_t entryLength = stream->ReadI8LE() -
                static_cast<std::uint32_t>(PSBObjType::NumberN8);
            std::vector<std::uint32_t> list;
            list.reserve(count);

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

    private:
        [[nodiscard]] V get(const K &key) const {
            assert(false && "not implement method: operator[](std::string)!");
            return nullptr;
        }

        [[nodiscard]] V get(int index) const { return _vec[index]; }

    private:
        Vec _vec{};
    };
} // namespace PSB