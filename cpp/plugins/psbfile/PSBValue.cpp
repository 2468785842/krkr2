//
// Created by lidong on 25-3-18.
//
#include "PSBValue.h"

#include <spdlog/spdlog.h>

#include "PSBExtension.h"
#include "tjsArray.h"
#include "tjsDictionary.h"

#include "tjsObject.h"

namespace PSB {
    PSBNumber::PSBNumber(PSBObjType objType, TJS::tTJSBinaryStream *stream) {
        switch(objType) {
            case PSBObjType::NumberN0:
            case PSBObjType::NumberN1:
            case PSBObjType::NumberN2:
            case PSBObjType::NumberN3:
            case PSBObjType::NumberN4:
            case PSBObjType::KeyNameN1:
            case PSBObjType::KeyNameN2:
            case PSBObjType::KeyNameN3:
            case PSBObjType::KeyNameN4:
                numberType = PSBNumberType::Int;
                data.resize(4);
                break;
            case PSBObjType::NumberN5:
            case PSBObjType::NumberN6:
            case PSBObjType::NumberN7:
            case PSBObjType::NumberN8:
                numberType = PSBNumberType::Long;
                data.resize(8);
                break;
            case PSBObjType::Float0:
            case PSBObjType::Float:
                numberType = PSBNumberType::Float;
                break;
            case PSBObjType::Double:
                numberType = PSBNumberType::Double;
                break;
            default:
                break;
        }

        switch(objType) {
            case PSBObjType::NumberN0:
                setValue<int>(0);
                return;
            case PSBObjType::NumberN1:
            case PSBObjType::KeyNameN1:
                Extension::readAndUnzip(stream, 1, data);
                return;
            case PSBObjType::NumberN2:
            case PSBObjType::KeyNameN2:
                Extension::readAndUnzip(stream, 2, data);
                return;
            case PSBObjType::NumberN3:
            case PSBObjType::KeyNameN3:
                Extension::readAndUnzip(stream, 3, data);
                return;
            case PSBObjType::NumberN4:
            case PSBObjType::KeyNameN4:
                Extension::readAndUnzip(stream, 4, data);
                return;
            case PSBObjType::NumberN5:
                Extension::readAndUnzip(stream, 5, data);
                return;
            case PSBObjType::NumberN6:
                Extension::readAndUnzip(stream, 6, data);
                return;
            case PSBObjType::NumberN7:
                Extension::readAndUnzip(stream, 7, data);
                return;
            case PSBObjType::NumberN8:
                Extension::readAndUnzip(stream, 8, data);
                return;
            case PSBObjType::Float0:
                data = BitConverter::toByteArray(0.0f);
                return;
            case PSBObjType::Float:
                data = BitConverter::toByteArray(stream->ReadI32LE());
                return;
            case PSBObjType::Double:
                data = BitConverter::toByteArray(stream->ReadI64LE());
                return;
        }
    }


    std::int64_t PSBNumber::getLongValue() const {
        return BitConverter::fromByteArray<std::int64_t>(data);
    }

    float PSBNumber::getFloatValue() const {
        return BitConverter::fromByteArray<float>(data);
    }


    PSBObjType PSBNumber::getType() const {
        switch(numberType) {
            case PSBNumberType::Int:
            case PSBNumberType::Long:
                switch(Extension::getSize(getLongValue())) {
                    case 0:
                        return PSBObjType::NumberN0;
                    case 1:
                        if(getLongValue() == 0) {
                            return PSBObjType::NumberN0;
                        }
                        return PSBObjType::NumberN1;
                    case 2:
                        return PSBObjType::NumberN2;
                    case 3:
                        return PSBObjType::NumberN3;
                    case 4:
                        return PSBObjType::NumberN4;
                    case 5:
                        return PSBObjType::NumberN5;
                    case 6:
                        return PSBObjType::NumberN6;
                    case 7:
                        return PSBObjType::NumberN7;
                    case 8:
                        return PSBObjType::NumberN8;
                    default:
                        throw std::runtime_error("Not a valid Integer");
                }

            case PSBNumberType::Float:
                // TODO: Float0 or not
                if(std::fabs(getFloatValue()) <
                   std::numeric_limits<float>::epsilon()) { // should we just
                                                            // use 0?
                    return PSBObjType::Float0;
                }

                return PSBObjType::Float;
            case PSBNumberType::Double:
                return PSBObjType::Double;
            default:
                throw std::runtime_error("Unknown number type");
        }
    }

    tTJSVariant PSBNull::toTJSVal() const { return {}; }

    tTJSVariant PSBBool::toTJSVal() const { return { this->value }; }

    tTJSVariant PSBNumber::toTJSVal() const {
        switch(numberType) {
            case PSBNumberType::Int:
                return { getValue<int>() };
            case PSBNumberType::Float:
                return { getValue<float>() };
            case PSBNumberType::Double:
                return { getValue<double>() };
            case PSBNumberType::Long:
            default:
                return { getValue<tjs_int64>() };
        }
    }


    tTJSVariant PSBArray::toTJSVal() const {
        iTJSDispatch2 *array = TJSCreateArrayObject();
        for(auto i : this->value) {
            tTJSVariant tmp{ static_cast<tjs_int32>(i) };
            tTJSVariant *args[] = { &tmp };
            static tjs_uint addHint = 0;
            array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args, array);
        }

        tTJSVariant result(array, array);
        array->Release();
        return result;
    }

    tTJSVariant PSBString::toTJSVal() const { return ttstr{ this->value }; }

    tTJSVariant PSBResource::toTJSVal() const {
        tTJSVariant result(this->data.data(), this->data.size());
        return result;
    }

    tTJSVariant PSBDictionary::toTJSVal() const {
        iTJSDispatch2 *dsp = TJSCreateDictionaryObject();
        for(const auto &[k, v] : this->_map) {
            tTJSVariant tmp = v->toTJSVal();
            dsp->PropSet(TJS_MEMBERENSURE, ttstr{ k }.c_str(), nullptr, &tmp,
                         dsp);
        }
        tTJSVariant result(dsp, dsp);
        dsp->Release();
        return result;
    }

    tTJSVariant PSBList::toTJSVal() const {
        iTJSDispatch2 *array = TJSCreateArrayObject();
        for(const auto &v : this->_vec) {
            tTJSVariant tmp = v->toTJSVal();
            tTJSVariant *args[] = { &tmp };
            static tjs_uint addHint = 0;
            array->FuncCall(0, TJS_W("add"), &addHint, nullptr, 1, args, array);
        }

        tTJSVariant result(array, array);
        array->Release();
        return result;
    }

} // namespace PSB