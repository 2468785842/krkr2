#ifndef __psdparse_h__
#define __psdparse_h__

#include <algorithm>
#include <stack>
#include <boost/spirit/include/qi.hpp>
#include <boost/phoenix/bind.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/spirit/repository/include/qi_advance.hpp>

#include "psddata.h"

namespace psd {

    // 汎用版バッファコピー
    template <typename Iterator>
    inline void copyToBuffer(uint8_t *buffer, Iterator &start, int size) {
        Iterator end = start;
        std::advance(end, size);
#if defined(_MSC_VER) && _MSC_VER >= 1400
// std::copy の security warning 抑止
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
        std::copy(start, end, buffer);
#if defined(_MSC_VER) && _MSC_VER >= 1400
#pragma warning(pop)
#endif
        start = end;
    }

    template <typename Iterator>
    inline void getShortBE(uint8_t *var8, Iterator &cur) {
        var8[1] = *cur++;
        var8[0] = *cur++;
    }

    template <typename Iterator>
    inline void getShortLE(uint8_t *var8, Iterator &cur) {
        var8[0] = *cur++;
        var8[1] = *cur++;
    }

    template <typename Iterator>
    inline void getLongBE(uint8_t *var8, Iterator &cur) {
        var8[3] = *cur++;
        var8[2] = *cur++;
        var8[1] = *cur++;
        var8[0] = *cur++;
    }

    template <typename Iterator>
    inline void getLongLE(uint8_t *var8, Iterator &cur) {
        var8[0] = *cur++;
        var8[1] = *cur++;
        var8[2] = *cur++;
        var8[3] = *cur++;
    }

    template <typename Iterator>
    inline void getLongLongBE(uint8_t *var8, Iterator &cur) {
        var8[7] = *cur++;
        var8[6] = *cur++;
        var8[5] = *cur++;
        var8[4] = *cur++;
        var8[3] = *cur++;
        var8[2] = *cur++;
        var8[1] = *cur++;
        var8[0] = *cur++;
    }

    template <typename Iterator>
    inline void getLongLongLE(uint8_t *var8, Iterator &cur) {
        var8[0] = *cur++;
        var8[1] = *cur++;
        var8[2] = *cur++;
        var8[3] = *cur++;
        var8[4] = *cur++;
        var8[5] = *cur++;
        var8[6] = *cur++;
        var8[7] = *cur++;
    }

    // 汎用イテレータ参照
    template <typename Iterator>
    class IteratorData : public IteratorBase {
    public:
        typedef boost::iterator_range<Iterator> irange;
        IteratorData(irange range) : range(range) { init(); };
        IteratorBase *clone() { return new IteratorData(range); }
        IteratorBase *cloneOffset(int offset) {
            IteratorData *id = new IteratorData(range);
            if(offset != 0) {
                id->range.advance_begin(offset);
                id->init();
            }
            return id;
        }
        virtual void init() { cur = range.begin(); }
        virtual int getCh() {
            if(eoi()) {
                return -1;
            }
            return *cur++;
        }
        virtual int16_t getInt16(bool convToNative) {
            if(std::distance(cur, range.end()) < 2) {
                return -1;
            }
            union {
                uint8_t var8[2];
                int16_t var16;
            } ret;
#ifdef BOOST_LITTLE_ENDIAN
            bool swap = convToNative;
#else
            bool swap = !convToNative;
#endif
            if(swap) {
                getShortBE(ret.var8, cur);
            } else {
                getShortLE(ret.var8, cur);
            }
            return ret.var16;
        }
        virtual int32_t getInt32(bool convToNative) {
            if(std::distance(cur, range.end()) < 4) {
                return -1;
            }
            union {
                uint8_t var8[4];
                int32_t var32;
            } ret;
#ifdef BOOST_LITTLE_ENDIAN
            bool swap = convToNative;
#else
            bool swap = !convToNative;
#endif
            if(swap) {
                getLongBE(ret.var8, cur);
            } else {
                getLongLE(ret.var8, cur);
            }
            return ret.var32;
        }
        virtual int64_t getInt64(bool convToNative) {
            if(std::distance(cur, range.end()) < 8) {
                return -1;
            }
            union {
                uint8_t var8[8];
                int64_t var64;
            } ret;
#ifdef BOOST_LITTLE_ENDIAN
            bool swap = convToNative;
#else
            bool swap = !convToNative;
#endif
            if(swap) {
                getLongLongBE(ret.var8, cur);
            } else {
                getLongLongLE(ret.var8, cur);
            }
            return ret.var64;
        }
        virtual int getData(void *buffer, int size) {
            if(eoi()) {
                return 0;
            }
            int remain = (int)std::distance(cur, range.end());
            if(size > remain) {
                size = remain;
            }

            copyToBuffer((uint8_t *)buffer, cur, size);

            return size;
        }
        virtual bool eoi() { return cur == range.end(); }
        virtual void getUnicodeString(std::wstring &str,
                                      bool convToNative = true) {
            int size = getInt32(true);
            str.clear();
            for(int i = 0; i < size; i++) {
                str.push_back((wchar_t)getInt16(convToNative));
            }
        }
        virtual int size() { return std::distance(range.begin(), range.end()); }
        virtual int rest() { return std::distance(cur, range.end()); }
        virtual void advance(int size) {
            if(!eoi()) {
                int remain = (int)std::distance(cur, range.end());
                if(size > remain) {
                    size = remain;
                }
                std::advance(cur, size);
            }
        }

    private:
        irange range;
        Iterator cur;
    };

    namespace spirit = boost::spirit;
    namespace qi = spirit::qi;
    namespace phx = boost::phoenix;
    namespace repos = boost::spirit::repository;

    template <typename Iterator>
    struct HeaderParser : qi::grammar<Iterator> {
        HeaderParser(Header &data) :
            HeaderParser::base_type(start), data(data) {
            start = (qi::lit("8BPS") >> qi::big_word >> repos::qi::advance(6) >>
                     qi::big_word >> qi::big_dword >> qi::big_dword >>
                     qi::big_word >> qi::big_word)[phx::bind(
                &HeaderParser::setHeader, this, qi::_1, qi::_2, qi::_3, qi::_4,
                qi::_5, qi::_6)];
        }

        void setHeader(int version, int channels, int height, int width,
                       int depth, int mode) {
            data.version = version;
            data.channels = channels;
            data.height = height;
            data.width = width;
            data.depth = depth;
            data.mode = mode;
        }

        qi::rule<Iterator> start;
        Header &data;
    };

    /**
     * イメージリソースパーサ
     */
    template <typename Iterator>
    struct ImageResourceParser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        ImageResourceParser(Data &data) :
            ImageResourceParser::base_type(start), data(data) {
            // カッコ内で式が使えないadvance用に事前にパディング等を計算しておくための一時変数
            int paddedSize = 0;

            // イメージリソース単体
            anImageResource =
                (qi::lit("8BIM") >> // signature
                 qi::big_word >> // resource ID
                 qi::byte_[qi::_a = qi::_1] >> // name size
                 spirit::repeat(qi::_a)[qi::char_] >> // name
                 spirit::repeat((qi::_a + 1) %
                                2)[qi::byte_] >> // パディング読み飛ばし
                 qi::big_dword[qi::_a = qi::_1]
                              [phx::ref(paddedSize) =
                                   (qi::_a + 1) / 2 * 2] >> // サイズ
                 qi::raw[repos::qi::advance(
                     phx::ref(paddedSize))] // データ(パディング含む)
                 )[phx::bind(&ImageResourceParser::addImageResource, this,
                             qi::_1, qi::_3, qi::_5, qi::_6)];
            start = *anImageResource;
        }
        qi::rule<Iterator, qi::locals<boost::uint32_t>> anImageResource;
        qi::rule<Iterator> start;
        Data &data;

        // イメージリソースを一つ追加する
        void addImageResource(uint16_t id, std::vector<char> &name, int size,
                              irange range) {
            std::string strname;
            if(name.size() > 0) {
                strname.assign(&name[0], name.size());
            }
            ImageResourceInfo info(id, strname, size,
                                   new IteratorData<Iterator>(range));
            data.imageResourceList.push_back(info);
        }
    };

    /**
     * レイヤ extra data パーサ
     */
    template <typename Iterator>
    struct LayerMaskParser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        LayerMaskParser(LayerMask &data, int size) :
            LayerMaskParser::base_type(start), data(data) {
            start = (qi::big_dword >> qi::big_dword >> qi::big_dword >>
                     qi::big_dword >> qi::byte_ >> qi::byte_ >>
                     qi::byte_)[phx::bind(&LayerMaskParser::setLayerMask, this,
                                          qi::_1, qi::_2, qi::_3, qi::_4,
                                          qi::_5, qi::_6)];

            if(size > 20) {
                start =
                    (start.copy() >> qi::byte_ >> qi::byte_ >> qi::big_dword >>
                     qi::big_dword >> qi::big_dword >> qi::big_dword)[phx::bind(
                        &LayerMaskParser::setLayerMaskExtra, this, qi::_1,
                        qi::_2, qi::_3, qi::_4, qi::_5, qi::_6)];
            }
        }

        void setLayerMask(int top, int left, int bottom, int right,
                          int defaultColor, int flags) {
            data.top = top;
            data.left = left;
            data.bottom = bottom;
            data.right = right;
            data.defaultColor = defaultColor;
            data.flags = flags;
        }

        void setLayerMaskExtra(int realFlags, int realUserMaskBackground,
                               int enclosingTop, int enclosingLeft,
                               int enclosingBottom, int enclosingRight) {
            data.realFlags = realFlags;
            data.realUserMaskBackground = realUserMaskBackground;
            data.enclosingTop = enclosingTop;
            data.enclosingLeft = enclosingLeft;
            data.enclosingBottom = enclosingBottom;
            data.enclosingRight = enclosingRight;
        }

        qi::rule<Iterator> start;
        LayerMask &data;
    };

    /**
     * レイヤ blending range パーサ
     */
    template <typename Iterator>
    struct LayerBlendingRangeParser : qi::grammar<Iterator> {
        LayerBlendingRangeParser(LayerBlendingRange &data) :
            LayerBlendingRangeParser::base_type(start), data(data) {
            channel = (qi::big_dword >> qi::big_dword)[phx::bind(
                &LayerBlendingRangeParser::addChannel, this, qi::_1, qi::_2)];
            start = (qi::big_dword >> qi::big_dword >> *channel)[phx::bind(
                &LayerBlendingRangeParser::setgrayBlend, this, qi::_1, qi::_2)];
        }
        void addChannel(int source, int dest) {
            LayerBlendingChannel ch = { source, dest };
            data.channels.push_back(ch);
        }
        void setgrayBlend(int src, int dest) {
            data.grayBlendSource = src;
            data.grayBlendDest = dest;
        }
        qi::rule<Iterator> channel;
        qi::rule<Iterator> start;
        LayerBlendingRange &data;
    };

    /**
     * レイヤ extra data パーサ
     */
    template <typename Iterator>
    struct LayerExtraDataParser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        LayerExtraDataParser(LayerExtraData &data) :
            LayerExtraDataParser::base_type(start), data(data) {

            layerMask = (qi::big_dword[qi::_a = qi::_1] >>
                         qi::raw[repos::qi::advance(qi::_a)])[phx::bind(
                &LayerExtraDataParser::setLayerMask, this, qi::_1, qi::_2)];

            layerBlendingRange =
                (qi::big_dword[qi::_a = qi::_1] >>
                 qi::raw[repos::qi::advance(qi::_a)])[phx::bind(
                    &LayerExtraDataParser::setLayerBlendingRange, this, qi::_1,
                    qi::_2)];

            layerName =
                (qi::byte_[qi::_a = qi::_1] >> // name size
                 spirit::repeat(qi::_a)[qi::char_][phx::bind(
                     &LayerExtraDataParser::setLayerName, this, qi::_1)] >>
                 spirit::repeat((4 - (qi::_a + 1) & 3) &
                                3)[qi::byte_] // padding
                );

            // 追加レイヤ情報
            additional =
                ((qi::lit("8BIM")[qi::_a = 0] | qi::lit("8B64")[qi::_a = 1]) >>
                 qi::big_dword >> // key
                 qi::big_dword[qi::_b = qi::_1] >> // data length
                 qi::raw[repos::qi::advance(qi::_b)] // data
                 )[phx::bind(&LayerExtraDataParser::addAdditional, this, qi::_a,
                             qi::_1, qi::_2, qi::_3)];

            start =
                (layerMask >> layerBlendingRange >> layerName >> *additional);
        }
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerMask;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerBlendingRange;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerName;
        qi::rule<Iterator, qi::locals<int, boost::uint32_t>> additional;
        qi::rule<Iterator> start;

        void setLayerMask(int size, irange range) {
            if(size > 0) {
                LayerMaskParser<Iterator> parser(data.layerMask, size);
                bool r = qi::parse(range.begin(), range.end(), parser);
                data.layerMask.width =
                    data.layerMask.right - data.layerMask.left;
                data.layerMask.height =
                    data.layerMask.bottom - data.layerMask.top;
            } else {
                std::memset(&data.layerMask, 0, sizeof(LayerMask));
            }
        }

        void setLayerBlendingRange(int size, irange range) {
            if(size > 0) {
                LayerBlendingRangeParser<Iterator> parser(
                    data.layerBlendingRange);
                bool r = qi::parse(range.begin(), range.end(), parser);
            } else {
                std::memset(&data.layerBlendingRange, 0,
                            sizeof(LayerBlendingRange));
            }
        }

        void setLayerName(std::vector<char> &name) {
            if(name.size() > 0) {
                data.layerName.assign(&name[0], name.size());
            }
        }

        void addAdditional(int sigType, int key, int size, irange range) {
            data.additionalLayers.push_back(AdditionalLayerInfo(
                sigType, key, size, new IteratorData<Iterator>(range)));
        }

        LayerExtraData &data;
    };

    /**
     * レイヤ情報パーサ
     */
    template <typename Iterator>
    struct LayerInfoParser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        LayerInfoParser(Data &data) :
            LayerInfoParser::base_type(start), data(data) {

            // チャンネル情報
            channelInfo = (qi::big_word >> // id
                           qi::big_dword // length
                           )[phx::bind(&LayerInfoParser::addChannel, this,
                                       qi::_1, qi::_2)];

            // エクストラデータ用処理
            extraData = (qi::big_dword[qi::_a = qi::_1] >> // extra data size
                         qi::raw[repos::qi::advance(qi::_a)])[phx::bind(
                &LayerInfoParser::setExtraData, this, qi::_1, qi::_2)];

            // レイヤ情報
            layerRecord = (qi::eps[phx::bind(&LayerInfoParser::addLayer,
                                             this)] >>
                           qi::big_dword >> // top
                           qi::big_dword >> // left
                           qi::big_dword >> // bottom
                           qi::big_dword // right
                           )[phx::bind(&LayerInfoParser::setLayerSize, this,
                                       qi::_1, qi::_2, qi::_3, qi::_4)] >>
                qi::big_word[qi::_a = qi::_1] >> // channel nums
                spirit::repeat(qi::_a)[channelInfo] >> // channel information
                qi::lit("8BIM") >> // signature
                (qi::big_dword >> // blend mode key
                 qi::byte_ >> // opacity
                 qi::byte_ >> // clipping
                 qi::byte_ // flag
                 )[phx::bind(&LayerInfoParser::setLayerData, this, qi::_1,
                             qi::_2, qi::_3, qi::_4)] >>
                qi::byte_ >> // filler(zero)
                extraData;
            // パース開始部
            start = (qi::big_word[phx::bind(&LayerInfoParser::setLayerCount,
                                            this, qi::_1)] >>
                     layerRecords >>
                     qi::raw[*qi::byte_][phx::bind(
                         &LayerInfoParser::setChannelImageData, this, qi::_1)]);
        }

        // レイヤ数指定
        void setLayerCount(boost::int16_t count) {
            data.mergedAlpha = count < 0;
            if(count == 0) {
                layerRecords = qi::eps;
            } else {
                layerRecords = spirit::repeat(abs(count))[layerRecord];
            }
        }

        void addChannel(int16_t id, int length) {
            data.layerList.back().channels.push_back(ChannelInfo(id, length));
        }

        void addLayer() { data.layerList.push_back(LayerInfo()); }

        // チャンネルイメージデータの場所を記録
        void setExtraData(int size, irange range) {
            if(size > 0) {
                LayerExtraDataParser<Iterator> parser(
                    data.layerList.back().extraData);
                bool r = qi::parse(range.begin(), range.end(), parser);
            }
        }

        // レイヤ基本情報を設定
        void setLayerSize(int top, int left, int bottom, int right) {
            LayerInfo &info = data.layerList.back();
            info.top = top;
            info.left = left;
            info.bottom = bottom;
            info.right = right;
            info.width = right - left;
            info.height = bottom - top;
        }

        // レイヤのその他のデータを設定
        void setLayerData(int blendModeKey, int opacity, int clipping,
                          int flag) {
            LayerInfo &info = data.layerList.back();
            info.blendModeKey = blendModeKey;
            info.blendMode = blendKeyToMode(blendModeKey);
            info.opacity = opacity;
            info.clipping = clipping;
            info.flag = flag;
        }

        // チャンネルイメージデータの場所を記録
        void setChannelImageData(irange range) {
            data.channelImageData = new IteratorData<Iterator>(range);
        }

        qi::rule<Iterator> channelInfo;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> extraData;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerRecord;
        qi::rule<Iterator> layerRecords;
        qi::rule<Iterator> start;

        Data &data;
    };

    // Global layer mask info parser
    template <typename Iterator>
    struct GlobalLayerMaskInfoParser : qi::grammar<Iterator> {
        GlobalLayerMaskInfoParser(GlobalLayerMaskInfo &data) :
            GlobalLayerMaskInfoParser::base_type(start), data(data) {
            start = (qi::big_word >> qi::big_word >> qi::big_word >>
                     qi::big_word >> qi::big_word >> qi::big_word >>
                     qi::byte_ >> *qi::byte_)[phx::bind(
                &GlobalLayerMaskInfoParser::setLayerMaskInfo, this, qi::_1,
                qi::_2, qi::_3, qi::_4, qi::_5, qi::_6, qi::_7)];
        }

        void setLayerMaskInfo(int overlayColorSpace, int color1, int color2,
                              int color3, int color4, int opacity, int kind) {
            data.overlayColorSpace = overlayColorSpace;
            data.color1 = color1;
            data.color2 = color2;
            data.color3 = color3;
            data.color4 = color4;
            data.opacity = opacity;
            data.kind = kind;
        }

        qi::rule<Iterator> start;
        GlobalLayerMaskInfo &data;
    };

    /**
     * layer and mask parser
     */
    template <typename Iterator>
    struct LayerAndMaskParser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        LayerAndMaskParser(Data &data) :
            LayerAndMaskParser::base_type(start), data(data) {

            layerInfo = (qi::big_dword[qi::_a = qi::_1] >> // size
                         qi::raw[repos::qi::advance(qi::_a)] // data
                         )[phx::bind(&LayerAndMaskParser::setLayerInfo, this,
                                     qi::_1, qi::_2)];

            globalLayerMaskInfo =
                (qi::big_dword[qi::_a = qi::_1] >> // size
                 qi::raw[repos::qi::advance(qi::_a)] // data
                 )[phx::bind(&LayerAndMaskParser::setGlobalLayerMaskInfo, this,
                             qi::_1, qi::_2)];

            layerInfo2 = qi::lit("8BIM") >>
                (qi::lit("Lr16") | qi::lit("Lr32")) >> // TODO Txt2, Patt, Pat2
                layerInfo;

            start = layerInfo >> // レイヤ情報
                globalLayerMaskInfo >> // グローバルレイヤマスク
                -layerInfo2;
        }

        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerInfo;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerInfo2;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> globalLayerMaskInfo;
        qi::rule<Iterator> start;

        void setLayerInfo(int size, irange range) {
            if(size > 0) {
                LayerInfoParser<Iterator> parser(data);
                bool r = qi::parse(range.begin(), range.end(), parser);
            }
        }

        void setGlobalLayerMaskInfo(int size, irange range) {
            if(size > 0) {
                GlobalLayerMaskInfoParser<Iterator> parser(
                    data.globalLayerMaskInfo);
                bool r = qi::parse(range.begin(), range.end(), parser);
            }
        }

        Data &data;
    };

    /**
     * PSD Parser
     */
    template <typename Iterator>
    struct Parser : qi::grammar<Iterator> {

        typedef boost::iterator_range<Iterator> irange;

        Parser(Data &data) :
            Parser::base_type(start), data(data), headerParser(data.header) {

            // カラーモード情報
            colorMode =
                (qi::big_dword[qi::_a = qi::_1] >> // size
                 qi::raw[repos::qi::advance(qi::_a)] // data
                 )[phx::bind(&Parser::setColorModeData, this, qi::_1, qi::_2)];

            // イメージリソース処理
            imageResource =
                (qi::big_dword[qi::_a = qi::_1] >> // size
                 qi::raw[repos::qi::advance(qi::_a)] // data
                 )[phx::bind(&Parser::setImageResource, this, qi::_1, qi::_2)];

            // レイヤとマスク
            layerAndMask =
                (qi::big_dword[qi::_a = qi::_1] >> // size
                 qi::raw[repos::qi::advance(qi::_a)] // data
                 )[phx::bind(&Parser::setLayerAndMask, this, qi::_1, qi::_2)];

            // 画像データ
            imageData = qi::raw[+qi::byte_]
                               [phx::bind(&Parser::setImageData, this, qi::_1)];

            start = headerParser >> colorMode >> imageResource >>
                layerAndMask >> -imageData >> qi::eoi;
        }

        HeaderParser<Iterator> headerParser;

        // rules
        qi::rule<Iterator, qi::locals<boost::uint32_t>> colorMode;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> imageResource;
        qi::rule<Iterator, qi::locals<boost::uint32_t>> layerAndMask;
        qi::rule<Iterator> imageData;
        qi::rule<Iterator> start;

        // カラーモードデータを登録
        void setColorModeData(int size, irange range) {
            data.colorModeSize = size;
            data.colorModeIterator = new IteratorData<Iterator>(range);
        }

        /**
         * イメージリソースを解析
         * @param size サイズ
         * @param range データ領域
         */
        void setImageResource(int size, irange range) {
            if(size > 0) {
                ImageResourceParser<Iterator> parser(data);
                bool r = qi::parse(range.begin(), range.end(), parser);
            }
        }

        /**
         * レイヤ・マスク情報を解析
         * @param size サイズ
         * @param データ領域
         */
        void setLayerAndMask(int size, irange range) {
            if(size > 0) {
                LayerAndMaskParser<Iterator> parser(data);
                bool r = qi::parse(range.begin(), range.end(), parser);
            }
        }

        // 合成済み画像データの先頭位置を登録
        void setImageData(irange range) {
            data.imageData = new IteratorData<Iterator>(range);
        }

        Data &data;
    };
} // namespace psd

#endif
