// generated from gentext.pl Messages.xlsx
#ifndef __MSG_INTF_INC_H__
#define __MSG_INTF_INC_H__
#define TVP_MSG_DEFINE(name, msg) TVP_MSG_DECL_CONST(name, TJS_W(msg))
TVP_MSG_DEFINE(TVPVersionInformation,
               "Kirikiri 2 Executable core /%1 (Compiled on %DATE% "
               "%TIME%) TJS2/%2 "
               "Copyright (C) 1997-2013 W.Dee and contributors All "
               "rights reserved.")
TVP_MSG_DEFINE(TVPVersionInformation2,
               "The details of version information can be perused if -about is "
               "attached and started. ")
TVP_MSG_DEFINE(TVPDownloadPageURL, "")
TVP_MSG_DEFINE(TVPInternalError, "Internal error")
TVP_MSG_DEFINE(TVPInvalidParam, "Invalid argument")
TVP_MSG_DEFINE(TVPWarnDebugOptionEnabled, "Debug option enabled")
TVP_MSG_DEFINE(TVPCommandLineParamIgnoredAndDefaultUsed,
               "Command line parameter %1 = %2 is invalid. It's used default.")
TVP_MSG_DEFINE(TVPInvalidCommandLineParam,
               "Command line parameter %1 = %2 is invalid.")
TVP_MSG_DEFINE(TVPNotImplemented, "Called method is not implemented")
TVP_MSG_DEFINE(TVPCannotOpenStorage, "Cannot open storage %1")
TVP_MSG_DEFINE(TVPCannotFindStorage, "Cannot find storage %1")
TVP_MSG_DEFINE(TVPCannotOpenStorageForWrite,
               "Cannot open storage %1 for writing")
TVP_MSG_DEFINE(TVPStorageInArchiveNotFound,
               "Cannot find storage %1 in archive %2")
TVP_MSG_DEFINE(TVPInvalidPathName, "Invalid path name %1")
TVP_MSG_DEFINE(TVPUnsupportedMediaName, "Not supported media type \"%1\"")
TVP_MSG_DEFINE(TVPCannotUnbindXP3EXE, "Cannot unbind XP3 exe %1")
TVP_MSG_DEFINE(TVPCannotFindXP3Mark, "%1 is not XP3 archive or unsupported")
TVP_MSG_DEFINE(TVPMissingPathDelimiterAtLast, "Use path delimiter '>' or '/'")
TVP_MSG_DEFINE(TVPFilenameContainsSharpWarn,
               "(Attention) Filename \"%1\" contains '#'")
TVP_MSG_DEFINE(TVPCannotGetLocalName, "Cannot get local name from %1")
TVP_MSG_DEFINE(TVPReadError, "Read error")
TVP_MSG_DEFINE(TVPWriteError, "Write error")
TVP_MSG_DEFINE(TVPSeekError, "Seek error")
TVP_MSG_DEFINE(TVPTruncateError, "Truncate error")
TVP_MSG_DEFINE(TVPInsufficientMemory, "Insufficient memory")
TVP_MSG_DEFINE(TVPUncompressionFailed, "Uncompression failed")
TVP_MSG_DEFINE(TVPCompressionFailed, "Compression failed")
TVP_MSG_DEFINE(TVPCannotWriteToArchive, "Cannot write to archive")
TVP_MSG_DEFINE(TVPUnsupportedCipherMode, "%1 is unsupported cipher mode")
TVP_MSG_DEFINE(TVPUnsupportedEncoding, "%1 is unsupported encoding")
TVP_MSG_DEFINE(TVPUnsupportedModeString, "%1 is unsupported mode string")
TVP_MSG_DEFINE(TVPUnknownGraphicFormat, "Unknown graphic format %1")
TVP_MSG_DEFINE(TVPCannotSuggestGraphicExtension,
               "Cannot suggest graphics extension for %1")
TVP_MSG_DEFINE(TVPMaskSizeMismatch, "Mask size mismath")
TVP_MSG_DEFINE(TVPProvinceSizeMismatch, "Province image %1 size mismatch")
TVP_MSG_DEFINE(TVPImageLoadError, "Image Load Error /%1")
TVP_MSG_DEFINE(TVPJPEGLoadError, "JPEG Read Error /%1")
TVP_MSG_DEFINE(TVPPNGLoadError, "PNG Read Error /%1")
TVP_MSG_DEFINE(TVPERILoadError, "ERI Read Error /%1")
TVP_MSG_DEFINE(TVPTLGLoadError, "TLG Read Error /%1")
TVP_MSG_DEFINE(TVPInvalidImageSaveType, "Invalid image save type (%1)")
TVP_MSG_DEFINE(TVPInvalidOperationFor8BPP, "Invalid operation for 8bpp")
TVP_MSG_DEFINE(TVPInvalidOperationFor32BPP, "Invalid operation for 32bpp")
TVP_MSG_DEFINE(TVPSpecifyWindow, "Specify Window class object")
TVP_MSG_DEFINE(TVPSpecifyLayer, "Specify Layer class object")
TVP_MSG_DEFINE(TVPSpecifyLayerOrBitmap, "Specify Layer or Bitmap class object")
TVP_MSG_DEFINE(TVPCannotAcceptModeAuto, "Cannot accept omAuto mode")
TVP_MSG_DEFINE(TVPCannotCreateEmptyLayerImage,
               "Cannot create empty layer image")
TVP_MSG_DEFINE(TVPCannotSetPrimaryInvisible,
               "Cannot set primary layer invisible")
TVP_MSG_DEFINE(TVPCannotMovePrimary, "Cannot move primary layer")
TVP_MSG_DEFINE(TVPCannotSetParentSelf, "Cannot set parent self")
TVP_MSG_DEFINE(TVPCannotMoveNextToSelfOrNotSiblings,
               "Cannot move next to self or not siblings")
TVP_MSG_DEFINE(TVPCannotMovePrimaryOrSiblingless,
               "Cannot move primary or siblingless")
TVP_MSG_DEFINE(TVPCannotMoveToUnderOtherPrimaryLayer,
               "Cannot move to under Other primary layer")
TVP_MSG_DEFINE(TVPInvalidImagePosition, "Invalid Image position")
TVP_MSG_DEFINE(TVPCannotSetModeToDisabledOrModal,
               "Cannot set Mode to disabled or modal")
TVP_MSG_DEFINE(TVPNotDrawableLayerType, "Not drawable layer type")
TVP_MSG_DEFINE(TVPSourceLayerHasNoImage, "Source layer has no image")
TVP_MSG_DEFINE(TVPUnsupportedLayerType, "Unsupported layer type %1")
TVP_MSG_DEFINE(TVPNotDrawableFaceType, "Not drawble face type %1")
TVP_MSG_DEFINE(TVPCannotConvertLayerTypeUsingGivenDirection,
               "Cannot convert layer type using given direction")
TVP_MSG_DEFINE(TVPNegativeOpacityNotSupportedOnThisFace,
               "Negative opacity not supported on this face")
TVP_MSG_DEFINE(TVPSrcRectOutOfBitmap, "Source rectangle out of bitmap")
TVP_MSG_DEFINE(TVPBoxBlurAreaMustContainCenterPixel,
               "Box blur area must contain center pixel")
TVP_MSG_DEFINE(TVPBoxBlurAreaMustBeSmallerThan16Million,
               "Box blur area must be smaller than 16 million")
TVP_MSG_DEFINE(TVPCannotChangeFocusInProcessingFocus,
               "Cannot change focus in processing focus")
TVP_MSG_DEFINE(TVPWindowHasNoLayer, "Window has no layer")
TVP_MSG_DEFINE(TVPWindowHasAlreadyPrimaryLayer,
               "Window has already primary layer")
TVP_MSG_DEFINE(TVPSpecifiedEventNeedsParameter,
               "Specified event %1 needs parameter")
TVP_MSG_DEFINE(TVPSpecifiedEventNeedsParameter2,
               "Specified event %1 needs parameter %2")
TVP_MSG_DEFINE(TVPSpecifiedEventNameIsUnknown,
               "Specified event name %1 is unknown")
TVP_MSG_DEFINE(TVPOutOfRectangle, "Out of Rectangle")
TVP_MSG_DEFINE(TVPInvalidMethodInUpdating, "Invalid method in updating")
TVP_MSG_DEFINE(TVPCannotCreateInstance, "Cannot create instance")
TVP_MSG_DEFINE(TVPUnknownWaveFormat, "Unknown wave format %1")
TVP_MSG_DEFINE(TVPCurrentTransitionMustBeStopping,
               "Current transition must be stopping")
TVP_MSG_DEFINE(TVPTransHandlerError, "Transition handler error %1")
TVP_MSG_DEFINE(TVPTransAlreadyRegistered, "Transition %1 already registerd")
TVP_MSG_DEFINE(TVPCannotFindTransHander, "Cannot find transition handler %1")
TVP_MSG_DEFINE(TVPSpecifyTransitionSource, "Specify transition source")
TVP_MSG_DEFINE(TVPLayerCannotHaveImage, "This layer cannot have image")
TVP_MSG_DEFINE(TVPTransitionSourceAndDestinationMustHaveImage,
               "Transition source and destination must have image")
TVP_MSG_DEFINE(TVPCannotLoadRuleGraphic, "Cannot load rule graphics %1")
TVP_MSG_DEFINE(TVPSpecifyOption, "Specify option %1")
TVP_MSG_DEFINE(TVPTransitionLayerSizeMismatch,
               "Transition layer size mismatch %1 and %2")
TVP_MSG_DEFINE(TVPTransitionMutualSource, "Transition mutual source")
TVP_MSG_DEFINE(TVPHoldDestinationAlphaParameterIsNowDeprecated,
               "Warring : Method %1 %2th parameter had is ignore. Hold "
               "destination alpha parameter is now deprecated.")
TVP_MSG_DEFINE(TVPCannotConnectMultipleWaveSoundBufferAtOnce,
               "Cannot connect multiple wave sound buffer at once")
TVP_MSG_DEFINE(TVPInvalidWindowSizeMustBeIn64to32768,
               "Invalid window size must be in 64 to 32768")
TVP_MSG_DEFINE(TVPInvalidOverlapCountMustBeIn2to32,
               "Invalid overlap count must be in 2 to 32")
TVP_MSG_DEFINE(TVPCurrentlyAsyncLoadBitmap, "Currently async load bitmap")
TVP_MSG_DEFINE(TVPFaildClipboardCopy, "copying to clipboard failed.")
TVP_MSG_DEFINE(TVPInvalidUTF16ToUTF8, "Invalid UTF-16 to UTF-8")
TVP_MSG_DEFINE(TVPInfoLoadingStartupScript, "(info) Loading startup script : ")
TVP_MSG_DEFINE(TVPInfoStartupScriptEnded, "(info) Startup script ended.")
TVP_MSG_DEFINE(TVPTjsCharMustBeTwoOrFour, "sizeof(tjs_char) must be 2 or 4.")
TVP_MSG_DEFINE(TVPMediaNameHadAlreadyBeenRegistered,
               "Media name \"%1\" had already been registered")
TVP_MSG_DEFINE(TVPMediaNameIsNotRegistered,
               "Media name \"%1\" is not registered")
TVP_MSG_DEFINE(TVPInfoRebuildingAutoPath,
               "(info) Rebuilding Auto Path Table ...")
TVP_MSG_DEFINE(TVPInfoTotalFileFoundAndActivated,
               "(info) Total %1 file(s) found, %2 file(s) activated. (%3ms)")
TVP_MSG_DEFINE(TVPErrorInRetrievingSystemOnActivateOnDeactivate,
               "Error in retrieving System.onActivate/onDeactivate : %1")
TVP_MSG_DEFINE(TVPTheHostIsNotA16BitUnicodeSystem,
               "The host is not a 16-bit unicode system.")
TVP_MSG_DEFINE(TVPInfoTryingToReadXp3VirtualFileSystemInformationFrom,
               "(info) Trying to read XP3 virtual file system "
               "information from : %1")
TVP_MSG_DEFINE(TVPSpecifiedStorageHadBeenProtected,
               "Specified storage had been protected!")
TVP_MSG_DEFINE(TVPInfoFailed, "(info) Failed.")
TVP_MSG_DEFINE(TVPInfoDoneWithContains,
               "(info) Done. (contains %1 file(s), %2 segment(s))")
TVP_MSG_DEFINE(TVPSeparatorCRLF,
               "\r\n\r\n\r\n==================================================="
               "===========================\r\n================================"
               "==============================================\r\n")
TVP_MSG_DEFINE(TVPSeparatorCR,
               "\n\n\n========================================================="
               "=====================\n========================================"
               "======================================\n")
// TVP_MSG_DEFINE(TVPDefaultFontName, "DEFAULT_GUI_FONT")
TVP_MSG_DEFINE(TVPCannotOpenFontFile, "Can't open font file '%1$s'")
TVP_MSG_DEFINE(TVPFontCannotBeUsed, "Font '%1$s' cannot be used")
TVP_MSG_DEFINE(TVPFontRasterizeError, "Font Rasterize error.")
TVP_MSG_DEFINE(TVPBitFieldsNotSupported, "BITFIELDS not supported.")
TVP_MSG_DEFINE(TVPCompressedBmpNotSupported, "Compressed BMP not supported.")
TVP_MSG_DEFINE(TVPUnsupportedColorModeForPalettImage,
               "Unsupported color mode for palettized image.")
TVP_MSG_DEFINE(TVPNotWindowsBmp,
               "This is not a Windows BMP file or an OS/2 BMP file.")
TVP_MSG_DEFINE(TVPUnsupportedHeaderVersion, "Non-supported header version.")
TVP_MSG_DEFINE(TVPInfoTouching, "(info) Touching ")
TVP_MSG_DEFINE(TVPAbortedTimeOut, " ... aborted [timed out]")
TVP_MSG_DEFINE(TVPAbortedLimitByte, " ... aborted [limit bytes exceeded]")
TVP_MSG_DEFINE(TVPFaildGlyphForDrawGlyph, "Faild glyph for DrawGlyph.")
TVP_MSG_DEFINE(TVPLayerObjectIsNotProperlyConstructed,
               "Panic! Layer object is not properly constructed. The "
               "constructor was not called??")
TVP_MSG_DEFINE(TVPUnknownUpdateType, "Unknown update type")
TVP_MSG_DEFINE(TVPUnknownTransitionType, "Unknown transition type")
TVP_MSG_DEFINE(TVPUnsupportedUpdateTypeTutGiveUpdate,
               "Update type of tutGiveUpdate is not yet supported")
TVP_MSG_DEFINE(TVPErrorCode, "error code : ")
TVP_MSG_DEFINE(TVPUnsupportedJpegPalette,
               "JPEG does not support palettized image")
TVP_MSG_DEFINE(TVPLibpngError, "libpng error.")
TVP_MSG_DEFINE(TVPUnsupportedColorTypePalette,
               "Unsupported color type for palattized image")
TVP_MSG_DEFINE(TVPUnsupportedColorType, "Unsupported color type")
TVP_MSG_DEFINE(TVPTooLargeImage, "Too large image")
TVP_MSG_DEFINE(TVPPngSaveError, "PNG save error.")
TVP_MSG_DEFINE(TVPTlgUnsupportedUniversalTransitionRule,
               "TLG cannot be used as universal transition rule, province(_p) "
               "or mask(_m) images.")
TVP_MSG_DEFINE(TVPUnsupportedColorCount, "Unsupported color count : ")
TVP_MSG_DEFINE(TVPDataFlagMustBeZero,
               "Data flag must be 0 (any flags are not yet supported)")
TVP_MSG_DEFINE(TVPUnsupportedColorTypeColon, "Unsupported color type : ")
TVP_MSG_DEFINE(TVPUnsupportedExternalGolombBitLengthTable,
               "External golomb bit length table is not yet supported.")
TVP_MSG_DEFINE(TVPUnsupportedEntropyCodingMethod,
               "Unsupported entropy coding method")
TVP_MSG_DEFINE(TVPInvalidTlgHeaderOrVersion,
               "Invalid TLG header or unsupported TLG version.")
TVP_MSG_DEFINE(TVPTlgMalformedTagMissionColonAfterNameLength,
               "Malformed TLG SDS tag structure, missing colon after "
               "name length")
TVP_MSG_DEFINE(TVPTlgMalformedTagMissionEqualsAfterName,
               "Malformed TLG SDS tag structure, missing equals after name")
TVP_MSG_DEFINE(TVPTlgMalformedTagMissionColonAfterVaueLength,
               "Malformed TLG SDS tag structure, missing colon after "
               "value length")
TVP_MSG_DEFINE(TVPTlgMalformedTagMissionCommaAfterTag,
               "Malformed TLG SDS tag structure, missing comma after a tag")
TVP_MSG_DEFINE(TVPFileSizeIsZero, "File size is zero.")
TVP_MSG_DEFINE(TVPMemoryAllocationError, "Memory allocation error.")
TVP_MSG_DEFINE(TVPFileReadError, "File read error.")
TVP_MSG_DEFINE(TVPInvalidPrerenderedFontFile,
               "Signature not found or invalid pre-rendered font file.")
TVP_MSG_DEFINE(TVPNot16BitUnicodeFontFile, "Not a 16-bit UNICODE font file.")
TVP_MSG_DEFINE(TVPInvalidHeaderVersion, "Invalid header version.")
TVP_MSG_DEFINE(TVPTlgInsufficientMemory, "SaveTLG6: Insufficient memory")
TVP_MSG_DEFINE(TVPTlgTooLargeBitLength,
               "SaveTLG6: Too large bit length (given image may be too large)")
TVP_MSG_DEFINE(TVPCannotRetriveInterfaceFromDrawDevice,
               "Could not retrive interface from given draw device")
#endif