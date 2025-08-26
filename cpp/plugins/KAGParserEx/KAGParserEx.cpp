#include "KAGParserEx.hpp"

const tjs_char* TVPKAGNoLine = TJS_W("読み込もうとしたシナリオファイル %1 は空です");
const tjs_char* TVPKAGCannotOmmitFirstLabelName = TJS_W("シナリオファイルの最初のラベル名は省略できません");
//const tjs_char* TVPInternalError = TJS_W("内部エラーが発生しました: at %1 line %2");
const tjs_char* TVPKAGMalformedSaveData = TJS_W("栞データが異常です。データが破損している可能性があります");
const tjs_char* TVPKAGLabelNotFound = TJS_W("シナリオファイル %1 内にラベル %2 が見つかりません");
const tjs_char* TVPLabelOrScriptInMacro = TJS_W("ラベルや iscript はマクロ中に記述できません");
const tjs_char* TVPKAGInlineScriptNotEnd = TJS_W("[endscript] または @endscript が見つかりません");
const tjs_char* TVPKAGSyntaxError = TJS_W("タグの文法エラーです。'[' や ']' の対応、\" と \" の対応、スペースの入れ忘れ、余分な改行、macro ～ endmacro の対応、必要な属性の不足などを確認してください");
const tjs_char* TVPKAGCallStackUnderflow = TJS_W("return タグが call タグと対応していません ( return タグが多い )");
const tjs_char* TVPKAGReturnLostSync = TJS_W("シナリオファイルに変更があったため return の戻り先位置を特定できません");
const tjs_char* TVPKAGSpecifyKAGParser = TJS_W("KAGParser クラスのオブジェクトを指定してください");
const tjs_char* TVPUnknownMacroName = TJS_W("マクロ \"%1\" は登録されていません");

#include "KAGParser.cpp"
