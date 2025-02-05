#include "psdparse.h"
#include "psdlayer.h"
#include "psdresource.h"

namespace psd {

    // パース完了した生データから必要な構造を展開する
    bool Data::processParsed() {
        bool success = true;

        // イメージリソースを展開
        for(auto &res : imageResourceList) {
            switch(res.id) {
                case 1032: // 0x0408 -- (Photoshop 4.0) Grid and
                           // guides information. See See Grid and
                           // guides resource format.
                    success = loadResourceGridAndGuide(*this, res);
                    break;
                case 1050: // 0x041A -- (Photoshop 6.0) Slices. See
                           // See Slices resource format.
                    success = loadResourceSlice(*this, res);
                    break;
                case 1046: // 0x0416 -- (Photoshop 6.0) Indexed Color
                           // Table Count. 2 bytes for the number of
                           // colors in table that are actually
                           // defined
                    success = loadResourceColorTableCount(*this, res);
                    break;
                case 1047: // 0x0417 -- (Photoshop 6.0) Transparency
                           // Index. 2 bytes for the index of
                           // transparent color, if any.
                    success = loadResourceTransparencyIndex(*this, res);
                    break;
                case 1065: // 0x0429 -- (Photoshop CS) Layer Comps. 4
                           // bytes (descriptor version = 16),
                           // Descriptor (see See Descriptor
                           // structure)
                    success = loadResourceLayerComps(*this, res);
                    break;

                default: // 未実装はそのままスルー
                case 1000: // 0x03E8 -- (Obsolete--Photoshop 2.0 only
                           // ) Contains five 2-byte values: number of
                           // channels, rows, columns, depth, and mode
                case 1001: // 0x03E9 -- Macintosh print manager print
                           // info record
                case 1003: // 0x03EB -- (Obsolete--Photoshop 2.0 only
                           // ) Indexed color table
                case 1005: // 0x03ED -- ResolutionInfo structure. See
                           // Appendix A in Photoshop API Guide.pdf.
                case 1006: // 0x03EE -- Names of the alpha channels as
                           // a series of Pascal strings.
                case 1007: // 0x03EF -- (Obsolete) See ID
                           // 1077DisplayInfo structure. See Appendix
                           // A in Photoshop API Guide.pdf.
                case 1008: // 0x03F0 -- The caption as a Pascal
                           // string.
                case 1009: // 0x03F1 -- Border information. Contains a
                           // fixed number (2 bytes real, 2 bytes
                           // fraction) for the border width, and 2
                           // bytes for border units (1 = inches, 2 =
                           // cm, 3 = points, 4 = picas, 5 = columns).
                case 1010: // 0x03F2 -- Background color. See See
                           // Color structure.
                case 1011: // 0x03F3 -- Print flags. A series of
                           // one-byte boolean values (see Page Setup
                           // dialog): labels, crop marks, color bars,
                           // registration marks, negative, flip,
                           // interpolate, caption, print flags.
                case 1012: // 0x03F4 -- Grayscale and multichannel
                           // halftoning information
                case 1013: // 0x03F5 -- Color halftoning information
                case 1014: // 0x03F6 -- Duotone halftoning information
                case 1015: // 0x03F7 -- Grayscale and multichannel
                           // transfer function
                case 1016: // 0x03F8 -- Color transfer functions
                case 1017: // 0x03F9 -- Duotone transfer functions
                case 1018: // 0x03FA -- Duotone image information
                case 1019: // 0x03FB -- Two bytes for the effective
                           // black and white values for the dot range
                case 1020: // 0x03FC -- (Obsolete)
                case 1021: // 0x03FD -- EPS options
                case 1022: // 0x03FE -- Quick Mask information. 2
                           // bytes containing Quick Mask channel ID;
                           // 1- byte boolean indicating whether the
                           // mask was initially empty.
                case 1023: // 0x03FF -- (Obsolete)
                case 1024: // 0x0400 -- Layer state information. 2
                           // bytes containing the index of target
                           // layer (0 = bottom layer).
                case 1025: // 0x0401 -- Working path (not saved). See
                           // See Path resource format.
                case 1026: // 0x0402 -- Layers group information. 2
                           // bytes per layer containing a group ID
                           // for the dragging groups. Layers in a
                           // group have the same group ID.
                case 1027: // 0x0403 -- (Obsolete)
                case 1028: // 0x0404 -- IPTC-NAA record. Contains the
                           // File Info... information. See the
                           // documentation in the IPTC folder of the
                           // Documentation folder.
                case 1029: // 0x0405 -- Image mode for raw format
                           // files
                case 1030: // 0x0406 -- JPEG quality. Private.
                case 1033: // 0x0409 -- (Photoshop 4.0) Thumbnail
                           // resource for Photoshop 4.0 only. See See
                           // Thumbnail resource format.
                case 1034: // 0x040A -- (Photoshop 4.0) Copyright
                           // flag. Boolean indicating whether image
                           // is copyrighted. Can be set via Property
                           // suite or by user in File Info...
                case 1035: // 0x040B -- (Photoshop 4.0) URL. Handle of
                           // a text string with uniform resource
                           // locator. Can be set via Property suite
                           // or by user in File Info...
                case 1036: // 0x040C -- (Photoshop 5.0) Thumbnail
                           // resource (supersedes resource 1033). See
                           // See Thumbnail resource format.
                case 1037: // 0x040D -- (Photoshop 5.0) Global Angle.
                           // 4 bytes that contain an integer between
                           // 0 and 359, which is the global lighting
                           // angle for effects layer. If not present,
                           // assumed to be 30.
                case 1038: // 0x040E -- (Obsolete) See ID 1073 below.
                           // (Photoshop 5.0) Color samplers resource.
                           // See See Color samplers resource format.
                case 1039: // 0x040F -- (Photoshop 5.0) ICC Profile.
                           // The raw bytes of an ICC (International
                           // Color Consortium) format profile. See
                           // ICC1v42_2006-05.pdf in the Documentation
                           // folder and icProfileHeader.h in Sample
                           // Code\Common\Includes .
                case 1040: // 0x0410 -- (Photoshop 5.0) Watermark. One
                           // byte.
                case 1041: // 0x0411 -- (Photoshop 5.0) ICC Untagged
                           // Profile. 1 byte that disables any
                           // assumed profile handling when opening
                           // the file. 1 = intentionally untagged.
                case 1042: // 0x0412 -- (Photoshop 5.0) Effects
                           // visible. 1-byte global flag to show/hide
                           // all the effects layer. Only present when
                           // they are hidden.
                case 1043: // 0x0413 -- (Photoshop 5.0) Spot Halftone.
                           // 4 bytes for version, 4 bytes for length,
                           // and the variable length data.
                case 1044: // 0x0414 -- (Photoshop 5.0)
                           // Document-specific IDs seed number. 4
                           // bytes: Base value, starting at which
                           // layer IDs will be generated (or a
                           // greater value if existing IDs already
                           // exceed it). Its purpose is to avoid the
                           // case where we add layers, flatten, save,
                           // open, and then add more layers that end
                           // up with the same IDs as the first set.
                case 1045: // 0x0415 -- (Photoshop 5.0) Unicode Alpha
                           // Names. Unicode string
                case 1049: // 0x0419 -- (Photoshop 6.0) Global
                           // Altitude. 4 byte entry for altitude
                case 1051: // 0x041B -- (Photoshop 6.0) Workflow URL.
                           // Unicode string
                case 1052: // 0x041C -- (Photoshop 6.0) Jump To XPEP.
                           // 2 bytes major version, 2 bytes minor
                           // version, 4 bytes count. Following is
                           // repeated for count: 4 bytes block size,
                           // 4 bytes key, if key = 'jtDd' , then next
                           // is a Boolean for the dirty flag;
                           // otherwise it's a 4 byte entry for the
                           // mod date.
                case 1053: // 0x041D -- (Photoshop 6.0) Alpha
                           // Identifiers. 4 bytes of length, followed
                           // by 4 bytes each for every alpha
                           // identifier.
                case 1054: // 0x041E -- (Photoshop 6.0) URL List. 4
                           // byte count of URLs, followed by 4 byte
                           // long, 4 byte ID, and Unicode string for
                           // each count.
                case 1057: // 0x0421 -- (Photoshop 6.0) Version Info.
                           // 4 bytes version, 1 byte
                           // hasRealMergedData , Unicode string:
                           // writer name, Unicode string: reader
                           // name, 4 bytes file version.
                case 1058: // 0x0422 -- (Photoshop 7.0) EXIF data 1.
                           // See
                           // http://www.kodak.com/global/plugins/acrobat/en/service/digCam/exifStandard2.pdf
                case 1059: // 0x0423 -- (Photoshop 7.0) EXIF data 3.
                           // See
                           // http://www.kodak.com/global/plugins/acrobat/en/service/digCam/exifStandard2.pdf
                case 1060: // 0x0424 -- (Photoshop 7.0) XMP metadata.
                           // File info as XML description. See
                           // http://www.adobe.com/devnet/xmp/
                case 1061: // 0x0425 -- (Photoshop 7.0) Caption
                           // digest. 16 bytes: RSA Data Security, MD5
                           // message-digest algorithm
                case 1062: // 0x0426 -- (Photoshop 7.0) Print scale. 2
                           // bytes style (0 = centered, 1 = size to
                           // fit, 2 = user defined). 4 bytes x
                           // location (floating point). 4 bytes y
                           // location (floating point). 4 bytes scale
                           // (floating point)
                case 1064: // 0x0428 -- (Photoshop CS) Pixel Aspect
                           // Ratio. 4 bytes (version = 1 or 2), 8
                           // bytes double, x / y of a pixel. Version
                           // 2, attempting to correct values for NTSC
                           // and PAL, previously off by a factor of
                           // approx. 5%.
                case 1066: // 0x042A -- (Photoshop CS) Alternate
                           // Duotone Colors. 2 bytes (version = 1), 2
                           // bytes count, following is repeated for
                           // each count: [ Color: 2 bytes for space
                           // followed by 4 * 2 byte color component
                           // ], following this is another 2 byte
                           // count, usually 256, followed by Lab
                           // colors one byte each for L, a, b. This
                           // resource is not read or used by
                           // Photoshop.
                case 1067: // 0x042B -- (Photoshop CS)Alternate Spot
                           // Colors. 2 bytes (version = 1), 2 bytes
                           // channel count, following is repeated for
                           // each count: 4 bytes channel ID, Color: 2
                           // bytes for space followed by 4 * 2 byte
                           // color component. This resource is not
                           // read or used by Photoshop.
                case 1069: // 0x042D -- (Photoshop CS2) Layer
                           // Selection ID(s). 2 bytes count,
                           // following is repeated for each count: 4
                           // bytes layer ID
                case 1070: // 0x042E -- (Photoshop CS2) HDR Toning
                           // information
                case 1071: // 0x042F -- (Photoshop CS2) Print info
                case 1072: // 0x0430 -- (Photoshop CS2) Layer Group(s)
                           // Enabled ID. 1 byte for each layer in the
                           // document, repeated by length of the
                           // resource. NOTE: Layer groups have start
                           // and end markers
                case 1073: // 0x0431 -- (Photoshop CS3) Color samplers
                           // resource. Also see ID 1038 for old
                           // format. See See Color samplers resource
                           // format.
                case 1074: // 0x0432 -- (Photoshop CS3) Measurement
                           // Scale. 4 bytes (descriptor version =
                           // 16), Descriptor (see See Descriptor
                           // structure)
                case 1075: // 0x0433 -- (Photoshop CS3) Timeline
                           // Information. 4 bytes (descriptor version
                           // = 16), Descriptor (see See Descriptor
                           // structure)
                case 1076: // 0x0434 -- (Photoshop CS3) Sheet
                           // Disclosure. 4 bytes (descriptor version
                           // = 16), Descriptor (see See Descriptor
                           // structure)
                case 1077: // 0x0435 -- (Photoshop CS3) DisplayInfo
                           // structure to support floating point
                           // clors. Also see ID 1007. See Appendix A
                           // in Photoshop API Guide.pdf .
                case 1078: // 0x0436 -- (Photoshop CS3) Onion Skins. 4
                           // bytes (descriptor version = 16),
                           // Descriptor (see See Descriptor
                           // structure)
                case 1080: // 0x0438 -- (Photoshop CS4) Count
                           // Information. 4 bytes (descriptor version
                           // = 16), Descriptor (see See Descriptor
                           // structure) Information about the count
                           // in the document. See the Count Tool.
                case 1082: // 0x043A -- (Photoshop CS5) Print
                           // Information. 4 bytes (descriptor version
                           // = 16), Descriptor (see See Descriptor
                           // structure) Information about the current
                           // print settings in the document. The
                           // color management options.
                case 1083: // 0x043B -- (Photoshop CS5) Print Style. 4
                           // bytes (descriptor version = 16),
                           // Descriptor (see See Descriptor
                           // structure) Information about the current
                           // print style in the document. The
                           // printing marks, labels, ornaments, etc.
                case 1084: // 0x043C -- (Photoshop CS5) Macintosh
                           // NSPrintInfo. Variable OS specific info
                           // for Macintosh. NSPrintInfo. It is
                           // recommened that you do not interpret or
                           // use this data.
                case 1085: // 0x043D -- (Photoshop CS5) Windows
                           // DEVMODE. Variable OS specific info for
                           // Windows. DEVMODE. It is recommened that
                           // you do not interpret or use this data.
                case 1086: // 0x043E -- (Photoshop CS6) Auto Save File
                           // Path. Unicode string. It is recommened
                           // that you do not interpret or use this
                           // data.
                case 1087: // 0x043F -- (Photoshop CS6) Auto Save
                           // Format. Unicode string. It is recommened
                           // that you do not interpret or use this
                           // data.
                case 1088: // 0x0440 -- (Photoshop CC) Path Selection
                           // State. 4 bytes (descriptor version =
                           // 16), Descriptor (see See Descriptor
                           // structure) Information about the current
                           // path selection state.
                // case 2000-2997:  // 0x07D0-0x0BB6 -- Path
                // Information (saved paths). See See Path resource
                // format.
                case 2999: // 0x0BB7 -- Name of clipping path. See See
                           // Path resource format.
                case 3000: // 0x0BB8 -- (Photoshop CC) Origin Path
                           // Info. 4 bytes (descriptor version = 16),
                           // Descriptor (see See Descriptor
                           // structure) Information about the origin
                           // path data.
                // case 4000-4999:  // 0x0FA0-0x1387 -- Plug-In
                // resource(s). Resources added by a plug-in. See the
                // plug-in API found in the SDK documentation
                case 7000: // 0x1B58 -- Image Ready variables. XML
                           // representation of variables definition
                case 7001: // 0x1B59 -- Image Ready data sets
                case 8000: // 0x1F40 -- (Photoshop CS3) Lightroom
                           // workflow, if present the document is in
                           // the middle of a Lightroom workflow.
                case 10000: // 0x2710 -- Print flags information. 2
                            // bytes version ( = 1), 1 byte center
                            // crop marks, 1 byte ( = 0), 4 bytes
                            // bleed width value, 2 bytes bleed width
                            // scale.
                    if(res.id >= 2000 && res.id <= 2997) {
                    }
                    if(res.id >= 4000 && res.id <= 4999) {
                    }
                    break;
            }
        }

        // カラーテーブル
        if(header.mode == COLOR_MODE_INDEXED && colorModeIterator &&
           colorModeSize == 768) {
            for(int i = 0; i < 256; i++) {
                colorTable.colors.emplace_back();
                colorTable.colors[i].r = colorModeIterator->getCh();
                colorTable.colors[i].a = 0xff;
            }
            for(int i = 0; i < 256; i++) {
                colorTable.colors[i].g = colorModeIterator->getCh();
            }
            for(int i = 0; i < 256; i++) {
                colorTable.colors[i].b = colorModeIterator->getCh();
            }
            if(colorTable.transparencyIndex >= 0 &&
               colorTable.transparencyIndex < 256) {
                colorTable.colors[colorTable.transparencyIndex].a = 0x0;
            }
        }

        // レイヤを展開
        int offset = 0;
        for(auto &i : layerList) {
            LayerInfo &layer = i;

            // 基本情報のセットアップ
            layer.owner = this;
            layer.layerName = layer.extraData.layerName;
            layer.layerType = LAYER_TYPE_NORMAL;
            layer.layerId = -1;
            layer.fill_opacity = 255;

            // チャネルイメージイテレータをチャネル毎に頭出しコピー
            for(auto &channel : layer.channels) {
                channel.imageData = channelImageData->cloneOffset(offset);
                offset += channel.length;
            }

            // 追加レイヤ情報を展開
            LayerExtraData &extra = i.extraData;
            for(auto &additional : extra.additionalLayers) {
                if(additional.sigType != 0) { // !8BIM
                    std::cerr << "not support: additional layer sig "
                                 "type = '8B64'\n";
                    continue;
                }

                switch(additional.key) {
                    // --- 調整レイヤ ---
                    case 'grdm': // Gradient settings (Photoshop 6.0)
                    case 'levl': // Levels
                    case 'curv': // Curves
                    case 'hue ': // Old Hue/saturation, Photoshop 4.0
                    case 'hue2': // New Hue/saturation, Photoshop 5.0
                    case 'blnc': // Color Balance
                    case 'nvrt': // Invert
                    case 'post': // Posterize
                    case 'thrs': // Threshold
                    case 'selc': // Selective color
                    case 'brit': // Brightness and Contrast
                    case 'mixr': // Channel Mixer
                    case 'clrL': // Color Lookup (Photoshop CS6)
                    case 'phfl': // Photo Filter
                    case 'blwh': // Black White (Photoshop CS3)
                    case 'vibA': // Vibrance (Photoshop CS3)
                    case 'expA': // Exposure
                        // TODO success = loadLayerXXXX();
                        layer.layerType = LAYER_TYPE_ADJUST;
                        break;

                    // --- 塗りつぶしレイヤ ---
                    case 'SoCo': // Solid color sheet setting
                                 // (Photoshop 6.0)
                    case 'GdFl': // Gradient fill setting
                                 // (Photoshop 6.0)
                    case 'PtFl': // Pattern fill setting
                                 // (Photoshop 6.0)
                        // TODO success = loadLayerXXXX();
                        layer.layerType = LAYER_TYPE_FILL;
                        break;

                    // --- 基本的なレイヤ情報 ---
                    case 'lsct': // Section divider setting
                                 // (Photoshop 6.0)
                                 //        success =
                                 //        loadLayerSectionDivider(layer,
                                 //        additional); break;
                    case 'lsdk': // *UNDOCUMENTED* key (may be
                                 // incorrect..)
                        success = loadLayerSectionDivider(layer, additional);
                        break;
                    case 'luni': // Unicode layer name (Photoshop 5.0)
                        success = loadLayerUnicodeName(layer, additional);
                        break;
                    case 'lyid': // Layer ID (Photoshop 5.0)
                        success = loadLayerId(layer, additional);
                        break;
                    case 'shmd': // Metadata setting (Photoshop 6.0)
                        success = loadLayerMetadata(layer, additional);
                        break;
                    case 'iOpa': // fill opacity
                        success = loadLayerFillOpacity(layer, additional);
                        break;

                    // --- 未対応 ---
                    case 'lrFX': // Effects Layer (Photoshop 5.0)
                    case 'tySh': // Type Tool Info (Photoshop 5.0
                                 // and 5.5 only)
                    case 'lfx2': // Object-based effects layer info
                                 // (Photoshop 6.0)
                    case 'Patt': // Patterns (Photoshop 6.0 and CS
                                 // (8.0))
                    case 'Pat2':
                    case 'Pat3':
                    case 'Anno': // Annotations (Photoshop 6.0)
                    case 'clbl': // Blend clipping elements
                                 // (Photoshop 6.0)
                    case 'infx': // Blend interior elements
                                 // (Photoshop 6.0)
                    case 'knko': // Knockout setting (Photoshop 6.0)
                    case 'lspf': // Protected setting (Photoshop 6.0)
                    case 'lclr': // Sheet color setting
                                 // (Photoshop 6.0)
                    case 'fxrp': // Reference point (Photoshop 6.0)
                    case 'brst': // Channel blending restrictions
                                 // setting (Photoshop 6.0)
                    case 'vmsk': // Vector mask setting
                                 // (Photoshop 6.0)
                    case 'vsms':
                    case 'TySh': // Type tool object setting
                                 // (Photoshop 6.0)
                    case 'ffxi': // Foreign effect ID (Photoshop 6.0)
                    case 'lnsr': // Layer name source setting
                                 // (Photoshop 6.0)
                    case 'shpa': // Pattern data (Photoshop 6.0)
                    case 'lyvr': // Layer version (Photoshop 7.0)
                    case 'tsly': // Transparency shapes layer
                                 // (Photoshop 7.0)
                    case 'lmgm': // Layer mask as global mask
                                 // (Photoshop 7.0)
                    case 'vmgm': // Vector mask as global mask
                                 // (Photoshop 7.0)
                    case 'plLd': // Placed Layer (replaced by SoLd in
                                 // Photoshop CS3)
                    case 'lnkD': // Linked Layer
                    case 'lnk2':
                    case 'lnk3':
                    case 'CgEd': // Content Generator Extra Data
                                 // (Photoshop CS5)
                    case 'Txt2': // Text Engine Data (Photoshop CS3)
                    case 'pths': // Unicode Path Name (Photoshop CS6)
                    case 'anFX': // Animation Effects (Photoshop CS6)
                    case 'FMsk': // Filter Mask (Photoshop CS3)
                    case 'SoLd': // Placed Layer Data (Photoshop CS3)
                    case 'vstk': // Vector Stroke Data (Photoshop CS6)
                    case 'vscg': // Vector Stroke Content Data
                                 // (Photoshop CS6)
                    case 'sn2P': // Using Aligned Rendering (Photoshop
                                 // CS6)
                    case 'vogk': // Vector Origination Data (Photoshop
                                 // CC)
                    case 'Mtrn': // Saving Merged Transparency
                    case 'Mt16':
                    case 'Mt32':
                    case 'LMsk': // User Mask
                    case 'FXid': // Filter Effects
                    case 'FEid':
                        break;
                }
            }
        } // end of additionals

        // グループ情報をリンク
        std::stack<LayerInfo *> parent;
        parent.push(nullptr);
        for(int i = (int)layerList.size() - 1; i >= 0; i--) {
            LayerInfo *layer = &layerList[i];
            layer->parent = parent.top();
            switch(layer->layerType) {
                case LAYER_TYPE_FOLDER:
                    parent.push(layer);
                    break;
                case LAYER_TYPE_HIDDEN:
                    parent.pop();
                    break;
                default:
                    break;
            }
        }

        return success;
    }

    // レイヤIDからレイヤを取得
    LayerInfo *Data::getLayerById(int layerId) {
        for(auto &i : layerList) {
            if(i.layerId == layerId) {
                return &i;
            }
        }
        return nullptr;
    }

} // namespace psd
