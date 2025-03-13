//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Conditional Compile Control
//---------------------------------------------------------------------------
#include <spdlog/spdlog.h>
#include "tjsCommHead.h"
#include "tjsCompileControl.h"
#include "tjsLex.h"
#include "tjsVariant.h"
#include "tjsError.h"

namespace TJSPP {
    //---------------------------------------------------------------------------

    int yylex(parser::value_type *yylex, tTJSPPExprParser *ptr) {
        tjs_int32 val;
        const tjs_int n = ptr->GetNext(val);
        if(n == parser::token_kind_type::PT_NUM)
            yylex->val = val;
        if(n == parser::token_kind_type::PT_SYMBOL)
            yylex->nv = val;
        return n;
    }

    void parser::error(const std::string &msg) {
        spdlog::get("tjs2")->critical(msg);
    }
    //---------------------------------------------------------------------------
    // TJS_iswspace
    static bool inline TJS_iswspace(tjs_char ch) {
        // the standard iswspace misses when non-zero page code

        if(ch & 0xff00) {
            return false;
        } else {
            return 0 != isspace(ch);
        }
    }

    //---------------------------------------------------------------------------
    static bool inline TJS_iswdigit(tjs_char ch) {
        // the standard iswdigit misses when non-zero page code

        if(ch & 0xff00) {
            return false;
        } else {
            return 0 != isdigit(ch);
        }
    }

    //---------------------------------------------------------------------------
    static bool inline TJS_iswalpha(tjs_char ch) {
        // the standard iswalpha misses when non-zero page code

        if(ch & 0xff00) {
            return true;
        } else {
            return 0 != isalpha(ch);
        }
    }

    //---------------------------------------------------------------------------
    //---------------------------------------------------------------------------
    // tTJSPPExprParser
    //---------------------------------------------------------------------------
    tTJSPPExprParser::tTJSPPExprParser(tTJS *tjs, const tjs_char *script) {
        // script pointed by "script" argument will be released by
        // this class via delete[]
        TJS = tjs;
        Script = script;
    }

    //---------------------------------------------------------------------------
    tTJSPPExprParser::~tTJSPPExprParser() { delete[] Script; }

    //---------------------------------------------------------------------------
    tjs_int32 tTJSPPExprParser::Parse() {
        Current = Script;
        Result = 0;
        auto bisonPpParser = parser{ this };
        if(bisonPpParser.parse()) {
            TJS_eTJSError(TJSPPError);
        }
        return Result;
    }

    //---------------------------------------------------------------------------
    tjs_int tTJSPPExprParser::GetNext(tjs_int32 &value) {
        // get next token

        while(TJS_iswspace(*Current) && *Current)
            Current++;
        if(!*Current)
            return 0;

        switch(*Current) {
            case TJS_W('('):
                Current++;
                return parser::token::PT_LPARENTHESIS;

            case TJS_W(')'):
                Current++;
                return parser::token::PT_RPARENTHESIS;

            case TJS_W(','):
                Current++;
                return parser::token::PT_COMMA;

            case TJS_W('='):
                if(*(Current + 1) == TJS_W('=')) {
                    Current += 2;
                    return parser::token::PT_EQUALEQUAL;
                }
                Current++;
                return parser::token::PT_EQUAL;

            case TJS_W('!'):
                if(*(Current + 1) == TJS_W('=')) {
                    Current += 2;
                    return parser::token::PT_NOTEQUAL;
                }
                Current++;
                return parser::token::PT_EXCLAMATION;

            case TJS_W('|'):
                if(*(Current + 1) == TJS_W('|')) {
                    Current += 2;
                    return parser::token::PT_LOGICALOR;
                }
                Current++;
                return parser::token::PT_VERTLINE;

            case TJS_W('&'):
                if(*(Current + 1) == TJS_W('&')) {
                    Current += 2;
                    return parser::token::PT_LOGICALAND;
                }
                Current++;
                return parser::token::PT_AMPERSAND;

            case TJS_W('^'):
                Current++;
                return parser::token::PT_CHEVRON;

            case TJS_W('+'):
                Current++;
                return parser::token::PT_PLUS;

            case TJS_W('-'):
                Current++;
                return parser::token::PT_MINUS;

            case TJS_W('*'):
                Current++;
                return parser::token::PT_ASTERISK;

            case TJS_W('/'):
                Current++;
                return parser::token::PT_SLASH;

            case TJS_W('%'):
                Current++;
                return parser::token::PT_PERCENT;

            case TJS_W('<'):
                if(*(Current + 1) == TJS_W('=')) {
                    Current += 2;
                    return parser::token::PT_LTOREQUAL;
                }
                Current++;
                return parser::token::PT_LT;

            case TJS_W('>'):
                if(*(Current + 1) == TJS_W('=')) {
                    Current += 2;
                    return parser::token::PT_GTOREQUAL;
                }
                Current++;
                return parser::token::PT_GT;

            case TJS_W('0'):
            case TJS_W('1'):
            case TJS_W('2'):
            case TJS_W('3'):
            case TJS_W('4'):
            case TJS_W('5'):
            case TJS_W('6'):
            case TJS_W('7'):
            case TJS_W('8'):
            case TJS_W('9'): {
                // number
                tTJSVariant val;
                try {
                    if(!TJSParseNumber(val, &Current))
                        return parser::token::PT_ERROR;
                } catch(...) {
                    return parser::token::PT_ERROR;
                }
                value = (tjs_int32)(tTVInteger)val;
                return parser::token::PT_NUM;
            }
        }

        if(!TJS_iswalpha(*Current) && *Current != TJS_W('_')) {
            return parser::token::PT_ERROR;
        }

        const tjs_char *st = Current;
        while((TJS_iswalpha(*Current) || TJS_iswdigit(*Current) ||
               *Current == TJS_W('_')) &&
              *Current)
            Current++;

        ttstr str(st, (int)(Current - st));

        IDs.push_back(str);
        value = (tjs_int32)(IDs.size() - 1);

        return parser::token::PT_SYMBOL;
    }

    //---------------------------------------------------------------------------
    const tjs_char *tTJSPPExprParser::GetString(tjs_int idx) const {
        return IDs[idx].c_str();
    }
} // namespace TJSPP
