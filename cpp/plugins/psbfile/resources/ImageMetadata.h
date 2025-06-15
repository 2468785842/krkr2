#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include "IResourceMetadata.h"
#include "../PSBValue.h"

namespace PSB {
    class ImageMetadata : public IResourceMetadata {
    public:
        explicit ImageMetadata() = default;

        ImageMetadata(const ImageMetadata &) = delete;
        ImageMetadata &operator=(const ImageMetadata &) = delete;

        ImageMetadata(ImageMetadata &&) = default;
        ImageMetadata &operator=(ImageMetadata &&) = default;

        inline static const std::string G_SupportedImageExt[]{ ".png", ".bmp",
                                                               ".jpg",
                                                               ".jpeg" };

        std::string getPart() const { return this->_part; }

        void setPart(std::string part) { this->_part = part; }

        std::string getName() const { return this->_name; }

        void setName(std::string name) { this->_name = name; }

        /**
         * Index is a value for tracking resource when compiling. For index
         * appeared in texture name
         * @see ImageMetadata::getTextureIndex()
         */
        std::uint32_t getIndex() const {
            return this->_resource->index.value_or(UINT32_MAX);
        }

        void setIndex(std::uint32_t index) {
            if(this->_resource != nullptr) {
                this->_resource->index = index;
            }
        }

        std::string getType() const noexcept { return this->_typeString.value; }

        std::string getPalType() const noexcept {
            return this->_paletteTypeString.value;
        }

        PSBPixelFormat getPalettePixelFormat() {
            PSBPixelFormat format =
                Extension::toPSBPixelFormat(getPalType(), _spec);
            if(format != PSBPixelFormat::None) {
                return format;
            }
            return Extension::defaultPalettePixelFormat(_spec);
        }

        /**
         * @brief The texture index
         * @code{.cpp}
         * "tex#001".TextureIndex = 1;
         * "tex".Index = 0;
         * @endcode
         */
        std::optional<std::uint32_t> getTextureIndex() {
            return getTextureIndex(this->_part);
        }

        std::vector<std::uint8_t> getData() { return this->_resource->data; }

        void setData(std::vector<uint8_t> data) {
            if(this->_resource == nullptr) {
                throw std::exception("Resource is null");
            }

            this->_resource->data = data;
        }


    private:
        /**
         * @brief The texture index. e.g.
         * @code{.cpp}
         * getTextureIndex("tex#001") = 1;
         * @endcode
         */
        std::optional<std::uint32_t>
        getTextureIndex(const std::string &texName);

    private:
        // Name 1
        std::string _part;
        // Name 2
        std::string _name;

        PSBCompressType _compress;

        bool _is2D = true;
        int _width;
        int _height;

        // [Type2]
        int _top;

        // [Type2]
        int _left;

        float _originX;
        float _originY;

        /// Pixel Format Type
        PSBString _typeString;
        Extension::RectangleF _clip;

        std::unique_ptr<PSBResource> _resource;

        // PIMG layer_type
        int _layerType;

        // Pal
        PSBResource _palette;

        PSBString _paletteTypeString;

        PSBSpec _spec = PSBSpec::Other;
    };

}; // namespace PSB