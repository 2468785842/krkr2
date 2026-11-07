#include "ncbind.hpp"

#define NCB_MODULE_NAME TJS_W("TextRender.dll")

// #define dbg_print TVPAddLog
#define dbg_print

// use Kirikiri-Z rasterizer for layouting
#include "FontRasterizer.h"
#include "LayerBitmapIntf.h"

#include "log/TVPLog.h"

extern FontRasterizer *GetCurrentRasterizer();

using RgbColor = uint32_t;

#define setprop_t(d, p, ty)                                                    \
    {                                                                          \
        tTJSVariant v(ty(p));                                                  \
        d->PropSet(TJS_MEMBERENSURE, TJS_W(#p), nullptr, &v, d);               \
    }

#define setprop_opt_t(d, p, ty)                                                \
    if(NULL != p) {                                                            \
        tTJSVariant v(ty(p));                                                  \
        d->PropSet(TJS_MEMBERENSURE, TJS_W(#p), nullptr, &v, d);               \
    } else {                                                                   \
        tTJSVariant v;                                                         \
        d->PropSet(TJS_MEMBERENSURE, TJS_W(#p), nullptr, &v, d);               \
    }

#define setprop(d, p) setprop_t(d, p, )

#define setprop_opt(d, p) setprop_opt_t(d, p, )

#define getprop_opt(d, p) getprop_opt_t(d, p, )

#define getprop_t(d, p, ty)                                                    \
    {                                                                          \
        tTJSVariant v;                                                         \
        if(TJS_SUCCEEDED(d->PropGet(0, TJS_W(#p), nullptr, &v, d)) &&          \
           v.Type() != tvtVoid) {                                              \
            p = ty(v);                                                         \
        }                                                                      \
    }

#define getprop_opt_t(d, p, ty)                                                \
    {                                                                          \
        tTJSVariant v;                                                         \
        if(TJS_SUCCEEDED(d->PropGet(0, TJS_W(#p), nullptr, &v, d)) &&          \
           v.Type() != tvtVoid) {                                              \
            p = ty(v);                                                         \
        } else {                                                               \
            p = NULL;                                                          \
        }                                                                      \
    }

#define getprop_ensure_opt_deref(d, p, e)                                      \
    {                                                                          \
        tTJSVariant v;                                                         \
        if(TJS_SUCCEEDED(d->PropGet(0, TJS_W(#p), nullptr, &v, d)) &&          \
           v.Type() != tvtVoid) {                                              \
            auto ens = v.e;                                                    \
            if(ens)                                                            \
                p = *ens;                                                      \
            else                                                               \
                p = std::nullopt;                                              \
        } else {                                                               \
            p = std::nullopt;                                                  \
        }                                                                      \
    }

#define getprop(d, p) getprop_t(d, p, )

#define getprop_ensure_deref(d, p, e)                                          \
    {                                                                          \
        tTJSVariant v;                                                         \
        if(TJS_SUCCEEDED(d->PropGet(0, TJS_W(#p), nullptr, &v, d)) &&          \
           v.Type() != tvtVoid) {                                              \
            auto ens = v.e;                                                    \
            if(ens)                                                            \
                p = *ens;                                                      \
        }                                                                      \
    }

struct TextRenderState {
    bool bold = false;
    bool italic = false;
    ttstr face = TJS_W("user");
    int fontSize = 24;
    double fontScale = 1.0;
    RgbColor chColor = 0xffffff;
    int rubySize = 10;
    int rubyOffset = -2;
    bool shadow = true;
    RgbColor shadowColor = 0x000000;
    bool edge = false;
    RgbColor edgeColor = 0x0080ff;
    int lineSpacing = 6;
    int pitch = 0;
    int lineSize = 0;

    bool renderOver = false;
    int renderDelay = 1000;
    ttstr renderText = TJS_W("");

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, bold);
        setprop(dict, italic);
        setprop(dict, fontSize);
        setprop(dict, fontScale);
        setprop(dict, face);
        setprop_t(dict, chColor, static_cast<tjs_int>);
        setprop(dict, rubySize);
        setprop(dict, rubyOffset);
        setprop(dict, shadow);
        setprop_t(dict, shadowColor, static_cast<tjs_int>);
        setprop(dict, edge);
        setprop_t(dict, edgeColor, static_cast<tjs_int>);
        setprop(dict, lineSpacing);
        setprop(dict, pitch);
        setprop(dict, lineSize);

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }

        getprop_t(dict, bold, static_cast<tjs_int>);
        getprop_t(dict, italic, static_cast<tjs_int>);
        getprop(dict, fontSize);
        getprop(dict, fontScale);
        getprop_ensure_deref(dict, face, AsStringNoAddRef());
        getprop_t(dict, chColor, static_cast<tjs_int>);
        getprop(dict, rubySize);
        getprop(dict, rubyOffset);
        getprop_t(dict, shadow, static_cast<tjs_int>);
        getprop_t(dict, shadowColor, static_cast<tjs_int>);
        getprop_t(dict, edge, static_cast<tjs_int>);
        getprop_t(dict, edgeColor, static_cast<tjs_int>);
        getprop(dict, lineSpacing);
        getprop(dict, pitch);
        getprop(dict, lineSize);
    }

    static TextRenderState from(tTJSVariant t) {
        TextRenderState state{};
        state.deserialize(t);
        return state;
    }
};

struct TextRenderOptions {
    const tjs_char *following = TJS_W(
        "%),:;]}????。，、．：；゛゜ヽヾゝゞ々’”）〕］｝〉》」』】°′″℃￠％‰　!."
        "?=?????????????？！ーぁぃぅぇぉっゃゅょゎァィゥェォッャュョヮヵヶ");
    const tjs_char *leading = TJS_W("\\$([{?‘“（〔［｛〈《「『【￥＄￡");
    const tjs_char *begin = TJS_W("「『（‘“〔［｛〈《");
    const tjs_char *end = TJS_W("」』）’”〕］｝〉》");

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, following);
        setprop(dict, leading);
        setprop(dict, begin);
        setprop(dict, end);

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }
    }

    static TextRenderState from(tTJSVariant t) {
        TextRenderState state{};
        state.deserialize(t);
        return state;
    }
};

struct CharacterInfo {
    bool bold = false;
    bool italic = false;
    bool graph = false;
    bool vertical = false;
    ttstr face = TJS_W("user");

    int x = 0;
    int y = 0;
    int cw = 0;
    int size = 0;

    RgbColor color = 0xffffff;
    RgbColor edge = NULL; //
    RgbColor shadow = NULL; //

    ttstr text = TJS_W("");

    // -------------------------------------------------------------- //

    tTJSVariant serialize() const {
        auto dict = TJSCreateDictionaryObject();

        setprop(dict, bold);
        setprop(dict, italic);
        setprop(dict, graph);
        setprop(dict, vertical);
        setprop(dict, x);
        setprop(dict, y);
        setprop(dict, cw);
        setprop(dict, size);
        setprop(dict, face);

        setprop_t(dict, color, static_cast<tjs_int>);
        setprop_opt_t(dict, edge, static_cast<tjs_int>);
        setprop_opt_t(dict, shadow, static_cast<tjs_int>);

        setprop(dict, text);

        auto res = tTJSVariant(dict, dict);
        dict->Release();

        return res;
    }

    void deserialize(tTJSVariant t) {
        auto dict = t.AsObjectNoAddRef();
        if(!dict) {
            return;
        }

        getprop(dict, bold);
        getprop(dict, italic);
        getprop(dict, graph);
        getprop(dict, vertical);
        getprop(dict, x);
        getprop(dict, y);
        getprop(dict, cw);
        getprop(dict, size);
        getprop_ensure_deref(dict, face, AsStringNoAddRef());

        getprop_t(dict, color, static_cast<tjs_int>);
        getprop_opt_t(dict, edge, static_cast<tjs_int>);
        getprop_opt_t(dict, shadow, static_cast<tjs_int>);

        getprop_ensure_deref(dict, text, AsStringNoAddRef());
    }

    static TextRenderState from(tTJSVariant t) {
        TextRenderState state{};
        state.deserialize(t);
        return state;
    }
};

#define property_accessor(name, type, storage)                                 \
    type get_##name() const { return storage; }                                \
    void set_##name(type v) { storage = v; }

#define property_accessor_cast(name, type, cast, storage)                      \
    cast get_##name() const { return cast(storage); }                          \
    void set_##name(cast v) { storage = type(v); }

#define property_accessor_string(name, storage)                                \
    tTJSVariant get_##name() const { return tTJSVariant(storage); }            \
    void set_##name(tTJSVariant v) {                                           \
        auto s = v.AsStringNoAddRef();                                         \
        storage = s->LongString ? s->LongString : s->ShortString;              \
    }

#define property_delegate(name) NCB_PROPERTY(name, get_##name, set_##name);

class TextRenderBase {
public:
    TextRenderBase();
    virtual ~TextRenderBase();
    bool render(tTJSString text, int autoIndent, int diff, int all,
                bool _reserved);
    void setRenderSize(int width, int height);
    void setDefault(tTJSVariant defaultSettings);
    void setOption(tTJSVariant options);
    tTJSVariant getCharacters(int start, int end);
    void clear();
    void done();

    void newline() { G_PluginLog.warn("no support newline"); }

    void resetFont();
    void resetStyle();
    tTJSVariant getKeyWait();
    void setKeyWait(tTJSVariant) { throw "no support setKeyWait"; };
    int calcShowCount();

    // property accessor
    property_accessor(vertical, bool, m_vertical);

    property_accessor(bold, bool, m_state.bold);
    property_accessor(italic, bool, m_state.italic);
    property_accessor_string(face, m_state.face);
    property_accessor(fontSize, int, m_state.fontSize);
    property_accessor(fontScale, double, m_state.fontScale);
    property_accessor_cast(chColor, RgbColor, tjs_int, m_state.chColor);
    property_accessor(rubySize, int, m_state.rubySize);
    property_accessor(rubyOffset, int, m_state.rubyOffset);
    property_accessor(shadow, bool, m_state.shadow);
    property_accessor_cast(shadowColor, RgbColor, tjs_int, m_state.shadowColor);
    property_accessor(edge, bool, m_state.edge);
    property_accessor(lineSpacing, int, m_state.lineSpacing);
    property_accessor(pitch, int, m_state.pitch);
    property_accessor(lineSize, int, m_state.lineSize);

    property_accessor(renderOver, bool, m_state.renderOver);
    property_accessor(renderDelay, int, m_state.renderDelay);
    property_accessor(renderLeft, int, m_x);
    int get_renderRight() const { return m_x + m_boxWidth; }
    void set_renderRight(int v) { m_boxWidth = v - m_x; };
    property_accessor(renderTop, int, m_y);
    int get_renderBottom() const { return m_y + m_boxHeight; }
    void set_renderBottom(int v) { m_boxHeight = v - m_y; };
    property_accessor(renderText, ttstr, m_state.renderText);
    int get_renderCount() const { return m_state.renderText.length(); }
    void set_renderCount(int v) { throw "avoid to set renderCount"; };

    property_accessor(defaultBold, bool, m_default.bold);
    property_accessor(defaultItalic, bool, m_default.italic);
    property_accessor_string(defaultFace, m_default.face);
    property_accessor(defaultFontSize, int, m_default.fontSize);
    property_accessor(defaultFontScale, float, m_default.fontScale);
    property_accessor_cast(defaultChColor, RgbColor, tjs_int,
                           m_default.chColor);
    property_accessor(defaultRubySize, int, m_default.rubySize);
    property_accessor(defaultRubyOffset, int, m_default.rubyOffset);
    property_accessor(defaultShadow, bool, m_default.shadow);
    property_accessor_cast(defaultShadowColor, RgbColor, tjs_int,
                           m_default.shadowColor);
    property_accessor(defaultEdge, bool, m_default.edge);
    property_accessor(defaultLineSpacing, int, m_default.lineSpacing);
    property_accessor(defaultPitch, int, m_default.pitch);
    property_accessor(defaultLineSize, int, m_default.lineSize);

private:
    int m_boxWidth = 0;
    int m_boxHeight = 0;

    int m_x = 0;
    int m_y = 0;

    int m_indent = 0;
    int m_autoIndent = 0;
    bool m_overflow = false;
    bool m_isBeginningOfLine = true;

    bool m_vertical = false;

    TextRenderOptions m_options{};
    TextRenderState m_default{};
    TextRenderState m_state{};

    std::vector<CharacterInfo> m_characters{};
    std::vector<CharacterInfo> m_buffer{};
    uint32_t m_mode = 0;

    void pushCharacter(tjs_char ch);
    void pushGraphicalCharacter(ttstr const &graph);
    void performLinebreak();
    void flush(bool force = false);
    void updateFont();
};

enum TextRenderAlignment {
    kTextRenderAlignmentLeft = -1,
    kTextRenderAlignmentCenter = 0,
    kTextRenderAlignmentRight = 1,
};

enum TextRenderMode {
    kTextRenderModeLeading = 0,
    kTextRenderModeNormal,
    kTextRenderModeFollowing,
};

// -------------------------------------------------------------------

TextRenderBase::TextRenderBase() {}

TextRenderBase::~TextRenderBase() {}

static bool readchar(tTJSString const &str, size_t &i, tjs_char &c) {
    auto const len = str.GetLen();

    if(++i >= len) {
        return false;
    }

    c = str[i];
    return true;
}

static void read_integer(tTJSString const &str, size_t &i, int &value) {
    tjs_char ch;
    bool is_negative = false;

    while(true) {
        if(!readchar(str, i, ch)) {
            TVPThrowExceptionMessage(
                TJS_W("TextRenderBase::render() failed to "
                      "parse: expected either integer or ';', found EOF"));
        }

        if('0' <= ch && ch <= '9') {
            value = value * 10 + (ch - '0');
            continue;
        } else if(ch == '-') {
            is_negative = !is_negative;
            continue;
        } else if(ch == ';') {
            if(is_negative) {
                value = -value;
            }
            return;
        }

        TVPThrowExceptionMessage(
            TJS_W("TextRenderBase::render() failed to "
                  "parse: expected either integer or ';', found '%1'"),
            ch);
    }
}

bool TextRenderBase::render(tTJSString text, int autoIndent, int diff, int all,
                            bool same) {

    auto const len = text.GetLen();

    for(size_t i = 0; i < len; ++i) {
        auto ch = text[i];

        switch(ch) {
            case '%':
                if(!readchar(text, i, ch)) {
                    TVPThrowExceptionMessage(
                        TJS_W("TextRenderBase::render() failed to "
                              "parse: expected character, found EOF"));
                }

                switch(ch) {
                    case 't': {
                        ttstr faceName{};

                        while(true) {
                            if(!readchar(text, i, ch)) {
                                TVPThrowExceptionMessage(TJS_W(
                                    "TextRenderBase::render() failed to "
                                    "parse: expected character, found EOF"));
                            }

                            if(ch == ';')
                                break;

                            faceName += ch;
                        }
                        break;
                    }
                    case 'f': {
                        ttstr faceName{};

                        while(true) {
                            if(!readchar(text, i, ch)) {
                                TVPThrowExceptionMessage(TJS_W(
                                    "TextRenderBase::render() failed to "
                                    "parse: expected character, found EOF"));
                            }

                            if(ch == ';')
                                break;

                            faceName += ch;
                        }

                        dbg_print(TVPFormatMessage(
                            TJS_W("change font face name: %1"), faceName));

                        m_state.face = faceName;

                        break;
                    }
                    case 'b': {
                        if(!readchar(text, i, ch) && (ch == '0' || ch == '1')) {
                            TVPThrowExceptionMessage(
                                TJS_W("TextRenderBase::render() failed to "
                                      "parse %%b: expected either '0' or '1', "
                                      "found EOF"));
                        }
                        bool flag = ch == '1';
                        if(flag)
                            dbg_print(TJS_W("set bold"));
                        else
                            dbg_print(TJS_W("unset bold"));

                        m_state.bold = flag;

                        break;
                    }
                    case 'i': {
                        if(!readchar(text, i, ch) && (ch == '0' || ch == '1')) {
                            TVPThrowExceptionMessage(
                                TJS_W("TextRenderBase::render() failed to "
                                      "parse %%i: expected either '0' or '1', "
                                      "found EOF"));
                        }
                        bool flag = ch == '1';
                        if(flag)
                            dbg_print(TJS_W("set italic (oblique)"));
                        else
                            dbg_print(TJS_W("unset italic (oblique)"));

                        m_state.italic = flag;

                        break;
                    }
                    case 's': {
                        if(!readchar(text, i, ch) && (ch == '0' || ch == '1')) {
                            TVPThrowExceptionMessage(
                                TJS_W("TextRenderBase::render() failed to "
                                      "parse %%s: expected either '0' or '1', "
                                      "found EOF"));
                        }
                        bool flag = ch == '1';
                        if(flag)
                            dbg_print(TJS_W("set shadow"));
                        else
                            dbg_print(TJS_W("unset shadow"));

                        m_state.shadow = flag;

                        break;
                    }
                    case 'e': {
                        if(!readchar(text, i, ch) && (ch == '0' || ch == '1')) {
                            TVPThrowExceptionMessage(
                                TJS_W("TextRenderBase::render() failed to "
                                      "parse %%e: expected either '0' or '1', "
                                      "found '%1'"),
                                ch);
                        }
                        bool flag = ch == '1';
                        if(flag)
                            dbg_print(TJS_W("set edge"));
                        else
                            dbg_print(TJS_W("unset edge"));

                        m_state.edge = flag;

                        break;
                    }
                    case 'B': // TODO: Big
                        dbg_print(TJS_W("big font"));
                        break;
                    case 'S': // TODO: Small
                        dbg_print(TJS_W("small font"));
                        break;
                    case 'r': // Reset
                        dbg_print(TJS_W("reset"));
                        resetFont();
                        break;
                    case 'C': // TODO: Centre
                        dbg_print(TJS_W("centre"));
                        break;
                    case 'R': // TODO: Right
                        dbg_print(TJS_W("right"));
                        break;
                    case 'L': // TODO: Left
                        dbg_print(TJS_W("left"));
                        break;
                    case 'l': {
                        dbg_print(TJS_W("left"));
                        while(true) {
                            if(!readchar(text, i, ch)) {
                                TVPThrowExceptionMessage(TJS_W(
                                    "TextRenderBase::render() failed to "
                                    "parse: expected character, found EOF"));
                            }
                            if(ch == ';')
                                break;
                        }
                        break;
                    }
                    case 'p': {
                        int value = 0;
                        read_integer(text, i, value);
                        m_state.pitch = value;
                        break;
                    }
                    case 'd': {
                        int value = 0;
                        read_integer(text, i, value);
                        // TODO: text speed
                        break;
                    }
                    case 'w': {
                        int value = 0;
                        read_integer(text, i, value);
                        // TODO: wait
                        break;
                    }
                    case 'D': // %D[0-9]+; || %D$.+;
                    {
                        if(ch == '$') {
                            ttstr labelName{};

                            while(true) {
                                if(!readchar(text, i, ch)) {
                                    TVPThrowExceptionMessage(TJS_W(
                                        "TextRenderBase::render() failed to "
                                        "parse: expected character, found "
                                        "EOF"));
                                }

                                if(ch == ';')
                                    break;

                                labelName += ch;
                            }

                            // TODO: labelName

                        } else {
                            int value = 0;
                            read_integer(text, i, value);

                            // TODO: label?
                        }
                        //
                        break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9': {
                        int value = static_cast<int>(ch - '0');
                        read_integer(text, i, value);

                        // font size
                        m_state.fontSize = m_default.fontSize * value / 100;

                        dbg_print(TVPFormatMessage(
                            TJS_W("new font size: %1 px"), m_state.fontSize));

                        updateFont();

                        break;
                    }
                    default:
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected any of "
                                  "'fbiseBSrCRLpdwD0123456789', found "
                                  "'%1'"),
                            ch);
                        break;
                }
                break;
            case '\\':
                if(!readchar(text, i, ch)) {
                    TVPThrowExceptionMessage(
                        TJS_W("TextRenderBase::render() failed to "
                              "parse: expected character, found EOF"));
                }

                switch(ch) {
                    case 'n':
                        flush();
                        performLinebreak();
                        break;
                    case 't':
                        pushCharacter('\t');
                        break;
                    case 'i':
                        m_indent = m_x;
                        break;
                    case 'r':
                        m_indent = 0;
                        break;
                    case 'w':
                        pushCharacter(' ');
                        break;
                    case 'k':
                        break;
                    case 'x':
                        break;
                    default:
                        goto __draw_normal;
                        break;
                }
                break;
            case '[': {
                // [ .* ]
                // [ .*, [0-9]+ ]
                ttstr ruby{};

                while(true) {
                    if(!readchar(text, i, ch)) {
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected character, found EOF"));
                    }

                    if(ch == ']')
                        break;

                    ruby += ch;
                }

                // TODO: implement ruby

                break;
            }
            case '#': {
                // [ .* ]
                // [ .*, [0-9]+ ]
                RgbColor colour = 0x00000000;
                bool isNull = true;

                while(true) {
                    if(!readchar(text, i, ch)) {
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected character, found EOF"));
                    }

                    if(ch == ';')
                        break;
                    isNull = false;
                    RgbColor c = 0;
                    if('0' <= ch && ch <= '9') {
                        c = static_cast<RgbColor>(ch - '0');
                    } else if('A' <= ch && ch <= 'F') {
                        c = 0x0a + static_cast<RgbColor>(ch - 'A');
                    } else if('a' <= ch && ch <= 'f') {
                        c = 0x0a + static_cast<RgbColor>(ch - 'a');
                    } else {
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected hexadecimal number, found "
                                  "'%1'"),
                            ch);
                    }

                    colour = (colour << 4) | c;
                }
                if(isNull)
                    colour = 0xffffff;
                m_state.chColor = colour;

                break;
            }
            case '&': {
                // '&' .+ ';'
                ttstr graph{};

                while(true) {
                    if(!readchar(text, i, ch)) {
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected character, found EOF"));
                    }

                    if(ch == ';')
                        break;

                    graph += ch;
                }

                pushGraphicalCharacter(graph);
                break;
            }
            case '$': {
                // '&' .+ ';'
                ttstr varName{};

                while(true) {
                    if(!readchar(text, i, ch)) {
                        TVPThrowExceptionMessage(
                            TJS_W("TextRenderBase::render() failed to "
                                  "parse: expected character, found EOF"));
                    }

                    if(ch == ';')
                        break;

                    varName += ch;
                }

                // TODO: implement eval

                break;
            }
            default:
            __draw_normal:
                // TODO: character should include format options;
                //       as the font is lazy-evaluated/drawn
                //       (restrictions for line-breaking algorithm)
                pushCharacter(ch);
                break;
        }
    }

    if(m_y > m_boxHeight) {
        m_overflow = true;
        set_renderOver(true);
    } else {
        m_overflow = false;
        set_renderOver(false);
    }

    return !m_overflow;
}

void TextRenderBase::performLinebreak() {
    auto rasterizer = GetCurrentRasterizer();
    m_x = m_indent;
    m_isBeginningOfLine = true;
    m_y += rasterizer->GetAscentHeight() + m_state.lineSpacing;
}

void TextRenderBase::pushGraphicalCharacter(ttstr const &graph) {
    // TODO: implement graphical characters
}

static bool findchInChars(const tjs_char *chz, tjs_char ch) {
    for(int i = 0; chz[i] != L'\0'; i++) {
        if(chz[i] == ch) {
            return true;
        }
    }
    return false;
}

void TextRenderBase::pushCharacter(tjs_char ch) {
    if((0xD800 <= ch && ch <= 0xDBFF) /* upper surrogate-pair */
       || (0xDC00 <= ch && ch <= 0xDFFF) /* lower surrogate-pair */) {
        TVPThrowExceptionMessage(TJS_W("unexpected character: surrogate pair"));
    }

    auto isLeadingChar = findchInChars(m_options.leading, ch);
    auto isFollowingChar = findchInChars(m_options.following, ch);
    auto isIndent = findchInChars(m_options.begin, ch);
    auto isIndentDecr = findchInChars(m_options.end, ch);

    uint32_t current;

    if(isLeadingChar) {
        current = kTextRenderModeLeading;
    } else if(isFollowingChar) {
        current = kTextRenderModeFollowing;
    } else {
        current = kTextRenderModeNormal;
    }

    if(m_mode == kTextRenderModeFollowing || m_mode != kTextRenderModeLeading) {
        flush();
    }

    auto rasterizer = GetCurrentRasterizer();
    auto text_height = rasterizer->GetAscentHeight();

    int advance_width = 0, advance_height = 0;

    rasterizer->GetTextExtent(ch, advance_width, advance_height);

    CharacterInfo info{
        m_state.bold,
        m_state.italic,
        false,
        false,
        m_state.face,
        0,
        0,
        advance_width,
        text_height,
        m_state.chColor,
        (m_state.edge ? m_state.edgeColor : 0),
        (m_state.shadow ? m_state.shadowColor : 0),
        (ttstr() + ch),
    };

    m_buffer.push_back(std::move(info));

    if(m_autoIndent) {
        // pre-indent
        if(m_isBeginningOfLine && m_autoIndent < 0) {
            m_x -= advance_width;
        }

        if(isIndent) {
            m_indent = m_x + advance_width;
            // TODO: register pair
        }

        if(isIndentDecr && m_indent > 0) {
            flush(); // FIXME: not safe?
            m_indent = 0;
        }
    }

    m_mode = current;
    m_isBeginningOfLine = false;
}

void TextRenderBase::flush(bool force) {
    if(m_buffer.empty()) {
        return;
    }

    // try place all characters in the same line

    auto x = m_x;

    for(auto &ch : m_buffer) {
        auto advance_width = ch.cw;
        auto new_x = advance_width + x + m_state.pitch;

        if(m_boxWidth < new_x) {
            if(force) {
                performLinebreak();
                x = m_x;
                new_x = advance_width + x + m_state.pitch;
            } else {
                performLinebreak();
                flush(true);
                return;
            }
        }

        ch.x = x;
        ch.y = m_y;

        x = new_x;
    }

    m_x = x;
    m_characters.insert(m_characters.end(), m_buffer.begin(), m_buffer.end());
    for(size_t i = 0; i < m_buffer.size(); i++) {
        m_state.renderText += m_buffer.at(i).text;
    }
    m_buffer.clear();
}

void TextRenderBase::setRenderSize(int width, int height) {
    m_boxWidth = width;
    m_boxHeight = height;

    dbg_print(
        TVPFormatMessage(TJS_W("set render size: (%1, %2)"), width, height));

    clear();
}

void TextRenderBase::setDefault(tTJSVariant defaultSettings) {
    dbg_print(TJS_W("set default format"));
    m_default.deserialize(defaultSettings);
}

void TextRenderBase::setOption(tTJSVariant options) {
    dbg_print(TJS_W("set option"));
    m_options.deserialize(options);
}

tTJSVariant TextRenderBase::getCharacters(int start, int end) {
    // TODO: only (0, 0) is observed
    auto array = TJSCreateArrayObject();
    dbg_print(TVPFormatMessage(TJS_W("get characters: [%1, %2]"), start, end));

    if((end < start) || (start == 0 && end == 0)) {
        for(size_t i = 0, cnt = m_characters.size(); i < cnt; ++i) {
            auto ch = m_characters[i].serialize();
            array->PropSetByNum(TJS_MEMBERENSURE, i, &ch, array);
        }
    } else {
        // TODO: unknown behaviour
    }

    return tTJSVariant(array, array);
}

void TextRenderBase::clear() {
    dbg_print(TJS_W("clear character buffer and format"));

    m_characters.clear();

    m_state = m_default;
    m_overflow = false;

    m_x = 0;
    m_y = 0;
    m_indent = 0;

    m_isBeginningOfLine = true;

    updateFont();
}

void TextRenderBase::updateFont() {
    auto rasterizer = GetCurrentRasterizer();
    auto font = tTVPFont{
        m_state.fontSize, // height of text
        static_cast<tjs_uint32>((m_state.bold ? TVP_TF_BOLD : 0) |
                                (m_state.italic ? TVP_TF_ITALIC : 0)),
        0,
        m_state.face, // TODO: this may fuck up the font settings by
                      // forcing the fallback font (in most cases)
    };

    rasterizer->ApplyFont(font);
}

void TextRenderBase::done() {
    dbg_print(TJS_W("flush character buffer"));
    flush();
}

void TextRenderBase::resetFont() { m_state = m_default; }

void TextRenderBase::resetStyle() { m_state = m_default; }

tTJSVariant TextRenderBase::getKeyWait() {
    iTJSDispatch2 *dsp = TJSCreateArrayObject();
    tTJSVariant ret(dsp, dsp);
    dsp->Release();
    return ret;
}

int TextRenderBase::calcShowCount() { return m_characters.size(); }

// register the class
NCB_REGISTER_CLASS(TextRenderBase) {
    Constructor();

    NCB_METHOD(render);
    NCB_METHOD(setRenderSize);
    NCB_METHOD(setDefault);
    NCB_METHOD(setOption);
    NCB_METHOD(getCharacters);
    NCB_METHOD(clear);
    NCB_METHOD(done);
    NCB_METHOD(newline);

    NCB_METHOD(resetFont);
    NCB_METHOD(resetStyle);
    NCB_METHOD(getKeyWait);
    NCB_METHOD(calcShowCount);
    Property(TJS_W("keyWait"), &Class::getKeyWait, &Class::setKeyWait);
    property_delegate(renderCount);
    property_delegate(renderOver);
    property_delegate(renderDelay);
    property_delegate(renderText);
    property_delegate(renderLeft);
    property_delegate(renderRight);
    property_delegate(renderTop);
    property_delegate(renderBottom);

    property_delegate(vertical);
    property_delegate(bold);
    property_delegate(italic);
    property_delegate(face);
    property_delegate(fontSize);
    property_delegate(fontScale);
    property_delegate(chColor);
    property_delegate(rubySize);
    property_delegate(rubyOffset);
    property_delegate(shadow);
    property_delegate(edge);
    property_delegate(lineSpacing);
    property_delegate(pitch);
    property_delegate(lineSize);

    property_delegate(defaultBold);
    property_delegate(defaultItalic);
    property_delegate(defaultFace);
    property_delegate(defaultFontSize);
    property_delegate(defaultFontScale);
    property_delegate(defaultChColor);
    property_delegate(defaultRubySize);
    property_delegate(defaultRubyOffset);
    property_delegate(defaultShadow);
    property_delegate(defaultEdge);
    property_delegate(defaultLineSpacing);
    property_delegate(defaultPitch);
    property_delegate(defaultLineSize);
};