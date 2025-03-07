// generated from gentext.pl Messages.xlsx
#ifndef MsgImplH
#define MsgImplH

#include "tjsMessage.h"
#include "MsgIntf.h"

#ifndef TVP_MSG_DECL
#define TVP_MSG_DECL(name, msg) extern tTJSMessageHolder name;
#define TVP_MSG_DECL_CONST(name) extern tTJSMessageHolder name;
#endif
//---------------------------------------------------------------------------
// Message Strings
//---------------------------------------------------------------------------
#define TVP_MSG_DEFINE(name, msg) TVP_MSG_DECL_CONST(name, TJS_W(msg))
TVP_MSG_DEFINE(TVPScriptExceptionRaised, "Script exception raised")
TVP_MSG_DEFINE(TVPHardwareExceptionRaised, "Hardware exception raised")
TVP_MSG_DEFINE(TVPMainCDPName, "Script Editor (Main)")
TVP_MSG_DEFINE(TVPExceptionCDPName, "Script Editor (Exception)")
TVP_MSG_DEFINE(TVPCannnotLocateUIDLLForFolderSelection, "Not found krdevui.dll")
TVP_MSG_DEFINE(TVPInvalidUIDLL, "Invalid krdevui.dll")
TVP_MSG_DEFINE(TVPInvalidBPP, "Invalid bpp")
TVP_MSG_DEFINE(TVPCannotLoadPlugin, "Cannot load Plugin %1")
TVP_MSG_DEFINE(TVPNotValidPlugin, "Not valid plugin %1")
TVP_MSG_DEFINE(TVPPluginUninitFailed, "Faild to release plugin")
TVP_MSG_DEFINE(TVPCannnotLinkPluginWhilePluginLinking,
               "Cannot link plugin while plugin linking")
TVP_MSG_DEFINE(TVPNotSusiePlugin, "Not Susie Plugin")
TVP_MSG_DEFINE(TVPSusiePluginError, "Susie Plugin error : %1")
TVP_MSG_DEFINE(TVPCannotReleasePlugin, "Cannot release plugin")
TVP_MSG_DEFINE(TVPNotLoadedPlugin, "Not loaded plugin %1")
TVP_MSG_DEFINE(TVPCannotAllocateBitmapBits,
               "Cannot allocate memory for Bitmap : %1 (size=%2)")
TVP_MSG_DEFINE(TVPScanLineRangeOver, "Scan line %1 is range over (0 to %2)")
TVP_MSG_DEFINE(TVPPluginError, "Plugin Error : %1")
TVP_MSG_DEFINE(TVPInvalidCDDADrive, "Invalid CD-DA Drive")
TVP_MSG_DEFINE(TVPCDDADriveNotFound, "CD-DA Drive not found")
TVP_MSG_DEFINE(TVPMCIError, "MCI Error : %1")
TVP_MSG_DEFINE(TVPInvalidSMF, "Invalid SMF : %1")
TVP_MSG_DEFINE(TVPMalformedMIDIMessage, "Malformed MIDI Message")
TVP_MSG_DEFINE(TVPCannotInitDirectSound, "DirectSound Initialize file : %1")
TVP_MSG_DEFINE(TVPCannotCreateDSSecondaryBuffer,
               "Cannot create DirectSound secondary buffer : %1/%2")
TVP_MSG_DEFINE(TVPInvalidLoopInformation, "Invalid loop information %1")
TVP_MSG_DEFINE(TVPNotChildMenuItem, "Not child menu Item")
TVP_MSG_DEFINE(TVPCannotInitDirect3D, "Cannot initialize Direct3D : %1")
TVP_MSG_DEFINE(TVPCannotFindDisplayMode, "Cannot find display mode : %1")
TVP_MSG_DEFINE(TVPCannotSwitchToFullScreen, "Cannot switch to fullscreen : %1")
TVP_MSG_DEFINE(TVPInvalidPropertyInFullScreen,
               "Invalid property in fullscreen : %1")
TVP_MSG_DEFINE(TVPInvalidMethodInFullScreen,
               "Invalid method in fullscreen : %1")
TVP_MSG_DEFINE(TVPCannotLoadCursor, "Cannot load mouse cursor : %1")
TVP_MSG_DEFINE(TVPCannotLoadKrMovieDLL, "Cannot load krmovie.dll")
TVP_MSG_DEFINE(TVPInvalidKrMovieDLL, "Invalid krmovie.dll")
TVP_MSG_DEFINE(TVPErrorInKrMovieDLL, "Error in krmovie.dll : %1")
TVP_MSG_DEFINE(TVPWindowAlreadyMissing, "Window already missing")
TVP_MSG_DEFINE(TVPPrerenderedFontMappingFailed,
               "Prerendered font mapping failed : %1")
TVP_MSG_DEFINE(TVPConfigFailOriginalFileCannotBeRewritten, "Cannot write %1.")
TVP_MSG_DEFINE(TVPConfigFailTempExeNotErased,
               "Did not exit %1. Cannot delete file.")
TVP_MSG_DEFINE(TVPExecutionFail, "Cannot execute %1.")
TVP_MSG_DEFINE(
    TVPPluginUnboundFunctionError,
    "Plugin require %1 funcion. Bat not found function in this file.")
TVP_MSG_DEFINE(TVPExceptionHadBeenOccured, " = (Exception)")
TVP_MSG_DEFINE(TVPConsoleResult, "Console : ")
TVP_MSG_DEFINE(TVPInfoListingFiles, "(info) Listing files in %1")
TVP_MSG_DEFINE(TVPInfoTotalPhysicalMemory, "(info) Total physical memory : %1")
TVP_MSG_DEFINE(TVPInfoSelectedProjectDirectory,
               "(info) Selected project directory : %1")
TVP_MSG_DEFINE(TVPTooSmallExecutableSize, "too small executable size.")
TVP_MSG_DEFINE(TVPInfoLoadingExecutableEmbeddedOptionsFailed,
               "(info) Loading executable embedded options failed "
               "(ignoring) : %1")
TVP_MSG_DEFINE(TVPInfoLoadingExecutableEmbeddedOptionsSucceeded,
               "(info) Loading executable embedded options succeeded.")
TVP_MSG_DEFINE(TVPFileNotFound, "file not found.")
TVP_MSG_DEFINE(
    TVPInfoLoadingConfigurationFileFailed,
    "(info) Loading configuration file \"%1\" failed (ignoring) : %2")
TVP_MSG_DEFINE(TVPInfoLoadingConfigurationFileSucceeded,
               "(info) Loading configuration file \"%1\" succeeded.")
TVP_MSG_DEFINE(TVPInfoDataPathDoesNotExistTryingToMakeIt,
               "(info) Data path does not exist, trying to make it ... %1")
TVP_MSG_DEFINE(TVPOk, "ok.")
TVP_MSG_DEFINE(TVPFaild, "failed.")
TVP_MSG_DEFINE(TVPInfoDataPath, "(info) Data path : %1")
TVP_MSG_DEFINE(TVPInfoSpecifiedOptionEarlierItemHasMorePriority,
               "(info) Specified option(s) (earlier item has more priority) :")
TVP_MSG_DEFINE(TVPNone, " (none)")
TVP_MSG_DEFINE(TVPInfoCpuClockRoughly, "(info) CPU clock (roughly) : %dMHz")
TVP_MSG_DEFINE(TVPProgramStartedOn, "Program started on %1 (%2)")
TVP_MSG_DEFINE(TVPKirikiri, "kirikiriz")
TVP_MSG_DEFINE(TVPUnknownError, "Unknown error!")
TVP_MSG_DEFINE(TVPExitCode, "Exit code: %d\n")
TVP_MSG_DEFINE(TVPFatalError, "Fatal Error")
TVP_MSG_DEFINE(TVPEnableDigitizer, "Enable Digitizer")
TVP_MSG_DEFINE(TVPTouchIntegratedTouch,
               "The device has an integrated touch digitizer. ")
TVP_MSG_DEFINE(TVPTouchExternalTouch,
               "The device has an external touch digitizer. ")
TVP_MSG_DEFINE(TVPTouchIntegratedPen,
               "The device has an integrated pen digitizer.")
TVP_MSG_DEFINE(TVPTouchExternalPen, "The device has an external pen digitizer.")
TVP_MSG_DEFINE(TVPTouchMultiInput,
               "The device supports multiple sources of digitizer input. ")
TVP_MSG_DEFINE(TVPTouchReady, "The device is ready to receive digitizer input.")
TVP_MSG_DEFINE(TVPCpuCheckFailure, "CPU check failure.")
TVP_MSG_DEFINE(TVPCpuCheckFailureCpuFamilyOrLesserIsNotSupported,
               "CPU check failure: CPU family 4 or lesser is not "
               "supported\r\n%1")
TVP_MSG_DEFINE(TVPInfoCpuNumber, "(info) CPU #%1 : ")
TVP_MSG_DEFINE(TVPCpuCheckFailureNotSupprtedCpu,
               "CPU check failure: Not supported CPU\r\n%1")
TVP_MSG_DEFINE(TVPInfoFinallyDetectedCpuFeatures,
               "(info) finally detected CPU features : %1")
TVP_MSG_DEFINE(TVPCpuCheckFailureNotSupportedCpu,
               "CPU check failure: Not supported CPU\r\n%1")
TVP_MSG_DEFINE(TVPInfoCpuClock, "(info) CPU clock : %.1fMHz")
TVP_MSG_DEFINE(TVPLayerBitmapBufferUnderrunDetectedCheckYourDrawingCode,
               "Layer bitmap: Buffer underrun detected. Check your drawing "
               "code!")
TVP_MSG_DEFINE(
    TVPLayerBitmapBufferOverrunDetectedCheckYourDrawingCode,
    "Layer bitmap: Buffer overrun detected. Check your drawing code!")
TVP_MSG_DEFINE(TVPFaildToCreateDirect3D, "Faild to create Direct3D9.")
TVP_MSG_DEFINE(TVPFaildToDecideBackbufferFormat,
               "Faild to decide backbuffer format.")
TVP_MSG_DEFINE(TVPFaildToCreateDirect3DDevice,
               "Faild to create Direct3D9 Device.")
TVP_MSG_DEFINE(TVPFaildToSetViewport, "Faild to set viewport.")
TVP_MSG_DEFINE(TVPFaildToSetRenderState, "Faild to set render state.")
TVP_MSG_DEFINE(TVPWarningImageSizeTooLargeMayBeCannotCreateTexture,
               "warning : Image size too large. May be cannot create texture.")
TVP_MSG_DEFINE(TVPUsePowerOfTwoSurface, "Use power of two surface.")
TVP_MSG_DEFINE(TVPCannotAllocateD3DOffScreenSurface,
               "Cannot allocate D3D off-screen surface/HR=%1")
TVP_MSG_DEFINE(TVPBasicDrawDeviceFailedToCreateDirect3DDevices,
               "BasicDrawDevice: Failed to create Direct3D devices: %1")
TVP_MSG_DEFINE(TVPBasicDrawDeviceFailedToCreateDirect3DDevicesUnknownReason,
               "BasicDrawDevice: Failed to create Direct3D devices: unknown "
               "reason")
TVP_MSG_DEFINE(TVPBasicDrawDeviceTextureHasAlreadyBeenLocked,
               "BasicDrawDevice: Texture has already been locked "
               "(StartBitmapCompletion() has been called twice without "
               "EndBitmapCompletion()),) unlocking the texture.")
TVP_MSG_DEFINE(TVPInternalErrorResult, "Internal error/HR=%1")
TVP_MSG_DEFINE(TVPBasicDrawDeviceInfPolygonDrawingFailed,
               "BasicDrawDevice: (inf) Polygon drawing failed/HR=%1")
TVP_MSG_DEFINE(TVPBasicDrawDeviceInfDirect3DDevicePresentFailed,
               "BasicDrawDevice: (inf) IDirect3DDevice::Present failed/HR=%1")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeRestart,
               "ChangeDisplaySettings failed: DISP_CHANGE_RESTART")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeBadFlags,
               "ChangeDisplaySettings failed: DISP_CHANGE_BADFLAGS")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeBadParam,
               "ChangeDisplaySettings failed: DISP_CHANGE_BADPARAM")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeFailed,
               "ChangeDisplaySettings failed: DISP_CHANGE_FAILED")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeBadMode,
               "ChangeDisplaySettings failed: DISP_CHANGE_BADMODE")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedDispChangeNotUpdated,
               "ChangeDisplaySettings failed: DISP_CHANGE_NOTUPDATED")
TVP_MSG_DEFINE(TVPChangeDisplaySettingsFailedUnknownReason,
               "ChangeDisplaySettings failed: unknown reason (%1)")
TVP_MSG_DEFINE(TVPFailedToCreateScreenDC, "Failed to create screen DC")
TVP_MSG_DEFINE(TVPFailedToCreateOffscreenBitmap,
               "Failed to create offscreen bitmap")
TVP_MSG_DEFINE(TVPFailedToCreateOffscreenDC, "Failed to create offscreen DC")
TVP_MSG_DEFINE(TVPInfoSusiePluginInfo, "(info) Susie plugin info : %1")
TVP_MSG_DEFINE(TVPSusiePluginUnsupportedBitmapHeader,
               "Non-supported bitmap header was given from susie plug-in.")
TVP_MSG_DEFINE(TVPBasicDrawDeviceFailedToCreateDirect3DDevice,
               "BasicDrawDevice: Failed to create Direct3D Device: %1")
TVP_MSG_DEFINE(TVPBasicDrawDeviceFailedToCreateDirect3DDeviceUnknownReason,
               "BasicDrawDevice: Failed to create Direct3D Device: unknown "
               "reason")
TVP_MSG_DEFINE(TVPCouldNotCreateAnyDrawDevice,
               "Fatal: Could not create any drawer objects.")
TVP_MSG_DEFINE(TVPBasicDrawDeviceDoesNotSupporteLayerManagerMoreThanOne,
               "\"basic draw\" device does not support layer manager more than "
               "1")
TVP_MSG_DEFINE(TVPInvalidVideoSize, "Invalid video size")
TVP_MSG_DEFINE(TVPRoughVsyncIntervalReadFromApi,
               "Rough VSync interval read from API : %1")
TVP_MSG_DEFINE(TVPRoughVsyncIntervalStillSeemsWrong,
               "Rough VSync interval still seems wrong, assuming "
               "default value (16)")
TVP_MSG_DEFINE(TVPInfoFoundDirect3DInterface,
               "(info) IDirect3D9 or higher detected. Retrieving current "
               "Direct3D driver information...")
TVP_MSG_DEFINE(TVPInfoFaild, "(info) Failed.")
TVP_MSG_DEFINE(TVPInfoDirect3D, "(info) Loading Direct3D ...")
TVP_MSG_DEFINE(TVPCannotLoadD3DDLL, "Cannot load d3d9.dll")
TVP_MSG_DEFINE(TVPNotFoundDirect3DCreate, "Missing Direct3DCreate9 in d3d9.dll")
TVP_MSG_DEFINE(TVPInfoEnvironmentUsing, "(info) environment: using %1")
TVP_MSG_DEFINE(TVPInfoSearchBestFullscreenResolution,
               "(info) Searching best fullscreen resolution ...")
TVP_MSG_DEFINE(TVPInfoConditionPreferredScreenMode,
               "(info) condition: preferred screen mode: %1")
TVP_MSG_DEFINE(TVPInfoConditionMode, "(info) condition: mode: %1")
TVP_MSG_DEFINE(TVPInfoConditionZoomMode, "(info) condition: zoom mode: %1")
TVP_MSG_DEFINE(TVPInfoEnvironmentDefaultScreenMode,
               "(info) environment: default screen mode: %1")
TVP_MSG_DEFINE(TVPInfoEnvironmentDefaultScreenAspectRatio,
               "(info) environment: default screen aspect ratio: %1 : %2")
TVP_MSG_DEFINE(TVPInfoEnvironmentAvailableDisplayModes,
               "(info) environment: available display modes:")
TVP_MSG_DEFINE(TVPInfoNotFoundScreenModeFromDriver,
               "(info) Panic! There is no reasonable candidate "
               "screen mode provided from "
               "the driver ... trying to use the default screen size "
               "and color depth ...")
TVP_MSG_DEFINE(TVPInfoResultCandidates, "(info) result: candidates:")
TVP_MSG_DEFINE(TVPInfoTryScreenMode, "(info) Trying screen mode: %1")
TVP_MSG_DEFINE(
    TVPAllScreenModeError,
    "All screen mode has been tested, but no modes available at all.")
TVP_MSG_DEFINE(TVPInfoChangeScreenModeSuccess,
               "(info) Changing screen mode succeeded")
TVP_MSG_DEFINE(TVPSelectXP3FileOrFolder, "Select XP3 file or folder")
TVP_MSG_DEFINE(TVPD3dErrDeviceLost,
               "D3D : The device has been lost but cannot be reset "
               "at this time.")
TVP_MSG_DEFINE(TVPD3dErrDriverIinternalError,
               "D3D : Internal driver error. Applications should destroy and "
               "recreate the device when receiving this error. ")
TVP_MSG_DEFINE(TVPD3dErrInvalidCall,
               "D3D : The method call is invalid. For example, a method's "
               "parameter may not be a valid pointer.")
TVP_MSG_DEFINE(TVPD3dErrOutOfVideoMemory,
               "D3D : Direct3D does not have enough display memory to perform "
               "the operation. ")
TVP_MSG_DEFINE(TVPD3dErrOutOfMemory,
               "D3D : Direct3D could not allocate sufficient memory "
               "to complete the call.")
TVP_MSG_DEFINE(TVPD3dErrWrongTextureFormat,
               "D3D : The pixel format of the texture surface is not valid.")
TVP_MSG_DEFINE(TVPD3dErrUnsuportedColorOperation,
               "D3D : The device does not support a specified texture-blending "
               "operation for color values.")
TVP_MSG_DEFINE(TVPD3dErrUnsuportedColorArg,
               "D3D : The device does not support a specified texture-blending "
               "argument for color values.")
TVP_MSG_DEFINE(TVPD3dErrUnsuportedAalphtOperation,
               "D3D : The device does not support a specified texture-blending "
               "operation for the alpha channel.")
TVP_MSG_DEFINE(TVPD3dErrUnsuportedAlphaArg,
               "D3D : The device does not support a specified texture-blending "
               "argument for the alpha channel.")
TVP_MSG_DEFINE(TVPD3dErrTooManyOperations,
               "D3D : The application is requesting more texture-filtering "
               "operations than the device supports.")
TVP_MSG_DEFINE(TVPD3dErrConflictioningTextureFilter,
               "D3D : The current texture filters cannot be used together.")
TVP_MSG_DEFINE(TVPD3dErrUnsuportedFactorValue,
               "D3D : The device does not support the specified texture factor "
               "value. Not used; provided only to support older drivers.")
TVP_MSG_DEFINE(TVPD3dErrConflictioningRenderState,
               "D3D : The currently set render states cannot be used together.")
TVP_MSG_DEFINE(
    TVPD3dErrUnsupportedTextureFilter,
    "D3D : The device does not support the specified texture filter.")
TVP_MSG_DEFINE(TVPD3dErrConflictioningTexturePalette,
               "D3D : The current textures cannot be used simultaneously.")
TVP_MSG_DEFINE(TVPD3dErrNotFound, "D3D : The requested item was not found.")
TVP_MSG_DEFINE(TVPD3dErrMoreData,
               "D3D : There is more data available than the "
               "specified buffer size can hold.")
TVP_MSG_DEFINE(TVPD3dErrDeviceNotReset,
               "D3D : The device has been lost but can be reset at this time.")
TVP_MSG_DEFINE(TVPD3dErrNotAvailable,
               "D3D : This device does not support the queried technique.")
TVP_MSG_DEFINE(TVPD3dErrInvalidDevice,
               "D3D : The requested device type is not valid.")
TVP_MSG_DEFINE(TVPD3dErrDriverInvalidCall, "D3D : Not used.")
TVP_MSG_DEFINE(TVPD3dErrWasStillDrawing,
               "D3D : The previous blit operation that is transferring "
               "information to or from this surface is incomplete.")
TVP_MSG_DEFINE(TVPD3dErrDeviceHung,
               "D3D : The device that returned this code caused the hardware "
               "adapter to be reset by the OS.")
TVP_MSG_DEFINE(TVPD3dErrUnsupportedOverlay,
               "D3D : The device does not support overlay for the specified "
               "size or display mode. ")
TVP_MSG_DEFINE(TVPD3dErrUnsupportedOverlayFormat,
               "D3D : The device does not support overlay for the specified "
               "surface format. ")
TVP_MSG_DEFINE(TVPD3dErrCannotProtectContent,
               "D3D : The specified content cannot be protected.")
TVP_MSG_DEFINE(TVPD3dErrUnsupportedCrypto,
               "D3D : The specified cryptographic algorithm is not supported.")
TVP_MSG_DEFINE(TVPD3dErrPresentStatisticsDisJoint,
               "D3D : The present statistics have no orderly sequence.")
TVP_MSG_DEFINE(TVPD3dErrDeviceRemoved,
               "D3D : The hardware adapter has been removed.")
TVP_MSG_DEFINE(TVPD3dOkNoAutoGen,
               "D3D : This is a success code. However,) the autogeneration of "
               "mipmaps is not supported for this format.")
TVP_MSG_DEFINE(TVPD3dErrFail,
               "D3D : An undetermined error occurred inside the "
               "Direct3D subsystem.")
TVP_MSG_DEFINE(TVPD3dErrInvalidArg,
               "D3D : An invalid parameter was passed to the "
               "returning function.")
TVP_MSG_DEFINE(TVPD3dUnknownError, "D3D : Unknown error.")
TVP_MSG_DEFINE(TVPExceptionAccessViolation,
               "Access Violation: The thread attempts to read from or write to "
               "a virtual address for which it does not have access.")
TVP_MSG_DEFINE(TVPExceptionBreakpoint,
               "Break Point: A breakpoint is encountered.")
TVP_MSG_DEFINE(TVPExceptionDatatypeMisalignment,
               "Data Type Misalignment: The thread attempts to read "
               "or write data that is "
               "misaligned on hardware that does not provide "
               "alignment. For example, "
               "16-bit values must be aligned on 2-byte boundaries,) "
               "32-bit values on "
               "4-byte boundaries,) and so on.")
TVP_MSG_DEFINE(TVPExceptionSingleStep,
               "Single Step: A trace trap or other single instruction "
               "mechanism signals that one instruction is executed.")
TVP_MSG_DEFINE(TVPExceptionArrayBoundsExceeded,
               "Array Bounds Exceede: The thread attempts to access "
               "an array element that "
               "is out of bounds, and the underlying hardware "
               "supports bounds checking.")
TVP_MSG_DEFINE(TVPExceptionFltDenormalOperand,
               "Denormal Operand: One of the operands in a floating point "
               "operation is denormal. A denormal value is one that is too "
               "small to represent as a standard floating point value.")
TVP_MSG_DEFINE(TVPExceptionFltDivideByZero,
               "Divide By Zero: The thread attempts to divide a floating point "
               "value by a floating point divisor of 0 (zero).")
TVP_MSG_DEFINE(TVPExceptionFltInexactResult,
               "Inexact Result: The result of a floating point operation "
               "cannot be represented exactly as a decimal fraction.")
TVP_MSG_DEFINE(TVPExceptionFltInvalidOperation,
               "Invalid Operation: A floating point exception that is not "
               "included in this list.")
TVP_MSG_DEFINE(TVPExceptionFltOverflow,
               "Overflow: The exponent of a floating point operation is "
               "greater than the magnitude allowed by the corresponding type.")
TVP_MSG_DEFINE(TVPExceptionFltStackCheck,
               "Stack Check: The stack has overflowed or underflowed, because "
               "of a floating point operation.")
TVP_MSG_DEFINE(TVPExceptionFltUnderflow,
               "Underflow: The exponent of a floating point operation is less "
               "than the magnitude allowed by the corresponding type.")
TVP_MSG_DEFINE(TVPExceptionIntDivideByZero,
               "Divide By Zero: The thread attempts to divide an integer value "
               "by an integer divisor of 0 (zero).")
TVP_MSG_DEFINE(TVPExceptionIntOverflow,
               "Overflow: The result of an integer operation creates "
               "a value that is too "
               "large to be held by the destination register. In "
               "some cases, this will "
               "result in a carry out of the most significant bit of "
               "the result. Some "
               "operations do not set the carry flag.")
TVP_MSG_DEFINE(
    TVPExceptionPrivInstruction,
    "Priv Instruction: The thread attempts to execute an instruction "
    "with an "
    "operation that is not allowed in the current computer mode.")
TVP_MSG_DEFINE(TVPExceptionNoncontinuableException,
               "Noncontinuable Exception: The thread attempts to continue "
               "execution after a non-continuable exception occurs.")
TVP_MSG_DEFINE(TVPExceptionGuardPage,
               "Guard Page: The thread accessed memory "
               "allocated with the PAGE_GUARD modifier.")
TVP_MSG_DEFINE(TVPExceptionIllegalInstruction,
               "Illegal Instruction: The thread tries to execute an "
               "invalid instruction.")
TVP_MSG_DEFINE(TVPExceptionInPageError,
               "In Page Error: The thread tries to access a page that is not "
               "present, and the system is unable to load the page. For "
               "example, this exception might occur if a network connection is "
               "lost while running a program over a network.")
TVP_MSG_DEFINE(TVPExceptionInvalidDisposition,
               "Invalid Disposition: An exception handler returns an "
               "invalid disposition "
               "to the exception dispatcher. Programmers using a "
               "high-level language such "
               "as C should never encounter this exception.")
TVP_MSG_DEFINE(TVPExceptionInvalidHandle,
               "Invalid Handle: The thread used a handle to a kernel object "
               "that was invalid (probably because it had been closed.)")
TVP_MSG_DEFINE(TVPExceptionStackOverflow,
               "Stack Overflow: The thread uses up its stack.")
TVP_MSG_DEFINE(TVPExceptionUnwindCconsolidate,
               "Unwind Consolidate: A frame consolidation has been executed.")
TVP_MSG_DEFINE(TVPCannotShowModalAreadyShowed, "Cannot Show Modal.")
TVP_MSG_DEFINE(TVPCannotShowModalSingleWindow,
               "Cannot Show Modal. When it is single window.")
#endif
