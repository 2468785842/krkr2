// generated from gentext.pl Messages.xlsx
#ifndef __TJS_ERROR_INC_H__
#define __TJS_ERROR_INC_H__
TJS_MSG_DECL(TJSInternalError, TJS_W("Internal error"))
TJS_MSG_DECL(TJSWarning, TJS_W("Warning: "))
TJS_MSG_DECL(TJSWarnEvalOperator,
             TJS_W("Non-global post-! operator is used (note that the post-! "
                   "operator behavior is changed on TJS2 version 2.4.1)"))
TJS_MSG_DECL(TJSNarrowToWideConversionError,
             TJS_W("Cannot convert given narrow string to wide string"))
TJS_MSG_DECL(TJSVariantConvertError,
             TJS_W("Cannot convert the variable type (%1 to %2)"))
TJS_MSG_DECL(TJSVariantConvertErrorToObject,
             TJS_W("Cannot convert the variable type (%1 to Object)"))
TJS_MSG_DECL(TJSIDExpected, TJS_W("Specify an ID"))
TJS_MSG_DECL(TJSSubstitutionInBooleanContext,
             TJS_W("Substitution in boolean context (use form of '(A=B)!=0' to "
                   "compare to zero)"));
TJS_MSG_DECL(TJSCannotModifyLHS,
             TJS_W("This expression cannot be used as a lvalue"))
TJS_MSG_DECL(TJSInsufficientMem, TJS_W("Insufficient memory"))
TJS_MSG_DECL(TJSCannotGetResult,
             TJS_W("Cannot get the value of this expression"))
TJS_MSG_DECL(TJSNullAccess, TJS_W("Accessing to nullptr object"))
TJS_MSG_DECL(TJSMemberNotFound, TJS_W("Member \"%1\" does not exist"))
TJS_MSG_DECL(TJSMemberNotFoundNoNameGiven, TJS_W("Member does not exist"))
TJS_MSG_DECL(TJSNotImplemented, TJS_W("Called method is not implemented"))
TJS_MSG_DECL(TJSInvalidParam, TJS_W("Invalid argument"))
TJS_MSG_DECL(TJSBadParamCount, TJS_W("Invalid argument count"))
TJS_MSG_DECL(TJSInvalidType,
             TJS_W("Not a function or invalid method/property type"))
TJS_MSG_DECL(TJSSpecifyDicOrArray,
             TJS_W("Specify a Dictionary object or an Array object"));
TJS_MSG_DECL(TJSSpecifyArray, TJS_W("Specify an Array object"));
TJS_MSG_DECL(TJSStringDeallocError,
             TJS_W("Cannot free the string memory block"))
TJS_MSG_DECL(TJSStringAllocError,
             TJS_W("Cannot allocate the string memory block"))
TJS_MSG_DECL(TJSMisplacedBreakContinue,
             TJS_W("Cannot place \"break\" or \"continue\" here"))
TJS_MSG_DECL(TJSMisplacedCase, TJS_W("Cannot place \"case\" here"))
TJS_MSG_DECL(TJSMisplacedReturn, TJS_W("Cannot place \"return\" here"))
TJS_MSG_DECL(TJSStringParseError,
             TJS_W("Un-terminated string/regexp/octet literal"))
TJS_MSG_DECL(TJSNumberError, TJS_W("Cannot be parsed as a number"))
TJS_MSG_DECL(TJSUnclosedComment, TJS_W("Un-terminated comment"))
TJS_MSG_DECL(TJSInvalidChar, TJS_W("Invalid character \'%1\'"))
TJS_MSG_DECL(TJSExpected, TJS_W("Expected %1"))
TJS_MSG_DECL(TJSSyntaxError, TJS_W("Syntax error (%1)"))
TJS_MSG_DECL(TJSPPError, TJS_W("Error in conditional compiling expression"))
TJS_MSG_DECL(TJSCannotGetSuper,
             TJS_W("Super class does not exist or cannot specify the "
                   "super class"))
TJS_MSG_DECL(TJSInvalidOpecode, TJS_W("Invalid VM code"))
TJS_MSG_DECL(TJSRangeError, TJS_W("The value is out of the range"))
TJS_MSG_DECL(TJSAccessDenyed,
             TJS_W("Invalid operation for Read-only or Write-only property"))
TJS_MSG_DECL(TJSNativeClassCrash, TJS_W("Invalid object context"))
TJS_MSG_DECL(TJSInvalidObject, TJS_W("The object is already invalidated"))
TJS_MSG_DECL(TJSDuplicatedPropHandler,
             TJS_W("Duplicated \"setter\" or \"getter\""))
TJS_MSG_DECL(TJSCannotOmit, TJS_W("\"...\" is used out of functions"))
TJS_MSG_DECL(TJSCannotParseDate, TJS_W("Invalid date format"))
TJS_MSG_DECL(TJSInvalidValueForTimestamp, TJS_W("Invalid value for date/time"))
TJS_MSG_DECL(TJSExceptionNotFound,
             TJS_W("Cannot convert exception because \"Exception\" "
                   "does not exist"))
TJS_MSG_DECL(TJSInvalidFormatString, TJS_W("Invalid format string"))
TJS_MSG_DECL(TJSDivideByZero, TJS_W("Division by zero"))
TJS_MSG_DECL(TJSNotReconstructiveRandomizeData,
             TJS_W("Cannot reconstruct random seeds"))
TJS_MSG_DECL(TJSSymbol, TJS_W("ID"))
TJS_MSG_DECL(TJSCallHistoryIsFromOutOfTJS2Script, TJS_W("[out of TJS2 script]"))
TJS_MSG_DECL(TJSNObjectsWasNotFreed, TJS_W("Total %1 Object(s) was not freed"))
TJS_MSG_DECL(TJSObjectCreationHistoryDelimiter, TJS_W(" <-- "));
TJS_MSG_DECL(TJSObjectWasNotFreed,
             TJS_W("Object %1 [%2] wes not freed / The object was "
                   "created at : %2"))
TJS_MSG_DECL(TJSGroupByObjectTypeAndHistory,
             TJS_W("Group by object type and location where the "
                   "object was created"))
TJS_MSG_DECL(TJSGroupByObjectType, TJS_W("Group by object type"))
TJS_MSG_DECL(TJSObjectCountingMessageGroupByObjectTypeAndHistory,
             TJS_W("%1 time(s) : [%2] %3"))
TJS_MSG_DECL(TJSObjectCountingMessageTJSGroupByObjectType,
             TJS_W("%1 time(s) : [%2]"))
TJS_MSG_DECL(TJSWarnRunningCodeOnDeletingObject,
             TJS_W("%4: Running code on deleting-in-progress object %1[%2] / "
                   "The object was created at : %3"))
TJS_MSG_DECL(TJSWriteError, TJS_W("Write error"))
TJS_MSG_DECL(TJSReadError, TJS_W("Read error"))
TJS_MSG_DECL(TJSSeekError, TJS_W("Seek error"))
TJS_MSG_DECL(TJSByteCodeBroken,
             TJS_W("Bytecode read error. File is broken or it's not "
                   "bytecode file."))
TJS_MSG_DECL(TJSObjectHashMapLogVersionMismatch,
             TJS_W("Object Hash Map log version mismatch"))
TJS_MSG_DECL(TJSCurruptedObjectHashMapLog,
             TJS_W("Currupted Object Hash Map log"))
TJS_MSG_DECL(TJSUnknownFailure, TJS_W("Unknown failure : %08X"))
TJS_MSG_DECL(TJSUnknownPackUnpackTemplateCharcter,
             TJS_W("Unknown pack/unpack TEMPLATE charcter"))
TJS_MSG_DECL(TJSUnknownBitStringCharacter, TJS_W("Unknown bit string charcter"))
TJS_MSG_DECL(TJSUnknownHexStringCharacter, TJS_W("Unknown Hex string charcter"))
TJS_MSG_DECL(TJSNotSupportedUuencode, TJS_W("Not supported uuencode"))
TJS_MSG_DECL(TJSNotSupportedBER, TJS_W("Not supported BER"))
TJS_MSG_DECL(TJSNotSupportedUnpackLP, TJS_W("Not supported unpack 'lp'"))
TJS_MSG_DECL(TJSNotSupportedUnpackP, TJS_W("Not supported unpack 'p'"))
#endif
