//
// Created by lidong on 25-3-18.
//
#include "PSBValue.h"
#include "PSBExtension.h"

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


    PSBObjType PSBNumber::getType() {
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
                        throw "Not a valid Integer";
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
                throw "Unknown number type";
        }
    }
} // namespace PSB