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

        [[nodiscard]] std::string getPart() const { return this->_part; }

        void setPart(const std::string &part) { this->_part = part; }

        [[nodiscard]] std::string getName() const override {
            return this->_name;
        }

        void setName(std::string name) override { this->_name = name; }

        /**
         * Index is a value for tracking resource when compiling. For index
         * appeared in texture name
         * @see ImageMetadata::getTextureIndex()
         */
        [[nodiscard]] std::uint32_t getIndex() const override {
            return this->_resource->index.value_or(UINT32_MAX);
        }

        void setIndex(std::uint32_t index) override {
            if(this->_resource != nullptr) {
                this->_resource->index = index;
            }
        }

        [[nodiscard]] std::string getType() const noexcept {
            return this->_typeString.value;
        }

        [[nodiscard]] std::string getPalType() const noexcept {
            return this->_paletteTypeString.value;
        }

        [[nodiscard]] PSBPixelFormat getPalettePixelFormat() const {
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

        [[nodiscard]] std::vector<std::uint8_t> getData() const {
            return this->_resource->data;
        }

        void setData(const std::vector<uint8_t> &data) const {
            if(this->_resource == nullptr) {
                throw std::runtime_error("Resource is null");
            }

            this->_resource->data = data;
        }

        [[nodiscard]] int getWidth() const { return this->_width; }
        void setWidth(int width) { this->_width = width; }

        [[nodiscard]] int getHeight() const { return this->_height; }
        void setHeight(int height) { this->_height = height; }

        [[nodiscard]] int getTop() const { return this->_top; }
        void setTop(int top) { this->_top = top; }

        [[nodiscard]] int getLeft() const { return this->_left; }
        void setLeft(int left) { this->_left = left; }

        [[nodiscard]] int getOpacity() const { return this->_opacity; }
        void setOpacity(int opacity) { this->_opacity = opacity; }

        [[nodiscard]] bool getVisible() const { return this->_visible; }
        void setVisible(bool visible) { this->_visible = visible; }

        [[nodiscard]] std::string getLabel() const { return this->_label; }
        void setLabel(const std::string &label) { this->_label = label; }

        [[nodiscard]] int getLayerType() const { return this->_layerType; }
        void setLayerType(int layerType) { this->_layerType = layerType; }

        [[nodiscard]] std::shared_ptr<PSBResource> getResource() const {
            return this->_resource;
        }
        void setResource(const std::shared_ptr<PSBResource> &resource) {
            this->_resource = resource;
        }

        [[nodiscard]] PSBCompressType getCompress() const {
            return this->_compress;
        }
        void setCompress(PSBCompressType compressType) {
            this->_compress = compressType;
        }

        [[nodiscard]] PSBSpec getSpec() const override { return this->_spec; }

        void setSpec(PSBSpec spec) override { this->_spec = spec; }

        [[nodiscard]] PSBType getPSBType() const override {
            return this->_psbType;
        }

        void setPSBType(PSBType psbType) override { this->_psbType = psbType; }

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

        std::string _label;

        int _opacity{ 10 };
        bool _visible{ true };

        /// Pixel Format Type
        PSBString _typeString;
        Extension::RectangleF _clip;

        std::shared_ptr<PSBResource> _resource;

        // PIMG layer_type
        int _layerType;

        // Pal
        PSBResource _palette;

        PSBString _paletteTypeString;

        PSBSpec _spec{ PSBSpec::Other };

        PSBType _psbType{ PSBType::Motion };
    };

}; // namespace PSB