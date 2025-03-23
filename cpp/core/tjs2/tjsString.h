//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// tTJSVariant friendly string class implementation
//---------------------------------------------------------------------------

#ifndef tjsStringH
#define tjsStringH

#include <boost/locale.hpp>
#include <fmt/printf.h>

#include <string>
#include "tjsConfig.h"

#include "tjsVariantString.h"

namespace TJS {

    class tTJSVariant;
    class tTJSVariantString;

    static const tjs_char *TJSNullStrPtr = TJS_W("");

    class tTJSStringBufferLength {
    public:
        tjs_int n;

        tTJSStringBufferLength(tjs_int n) // NOLINT(*-explicit-constructor)
            :
            n(n) {}
    };

    struct tTJSString_S {
        tTJSVariantString *Ptr{};
    };

    class tTJSString : protected tTJSString_S {
    public:
        tTJSString() : tTJSString_S{ nullptr } {}

        tTJSString(const tTJSString &rhs) : tTJSString_S{ rhs.Ptr } {
            if(rhs.Ptr)
                rhs.Ptr->AddRef();
        }

        tTJSString(tTJSVariantString *vstr) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ vstr } {
            if(vstr)
                vstr->AddRef();
        }

        tTJSString(const tjs_char *str) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ TJSAllocVariantString(str) } {}

        tTJSString(const tjs_nchar *str) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ TJSAllocVariantString(str) } {}

        tTJSString(
            const tTJSStringBufferLength len) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ TJSAllocVariantStringBuffer(len.n) } {}

        tTJSString(tjs_char rch) // NOLINT(*-explicit-constructor)
        {
            tjs_char tmp[]{ rch, TJS_W('\0') };
            this->Ptr = TJSAllocVariantString(tmp);
        } // NOLINT(google-explicit-constructor)

        // construct with first n chars of str
        tTJSString(const tTJSString &str, size_t n) :
            tTJSString_S{ TJSAllocVariantString(str.c_str(), n) } {}

        // same as above except for str's type
        tTJSString(const tjs_char *str, size_t n) :
            tTJSString_S{ TJSAllocVariantString(str, n) } {}

        tTJSString(const std::basic_string<tjs_char>
                       &str) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ TJSAllocVariantString(str.c_str()) } {}

        tTJSString(const std::string &str) // NOLINT(*-explicit-constructor)
            :
            tTJSString_S{ TJSAllocVariantString(str.c_str()) } {}

        tTJSString(const tTJSVariant &val); // NOLINT(*-explicit-constructor)

        tTJSString(tjs_int n); // NOLINT(*-explicit-constructor)
        tTJSString(tjs_int64 n); // NOLINT(*-explicit-constructor)

        ~tTJSString() {
            if(this->Ptr)
                this->Ptr->Release();
        }

        [[nodiscard]] const tjs_char *c_str() const {
            return this->Ptr ? this->Ptr->operator const tjs_char *()
                             : TJSNullStrPtr;
        }

        [[nodiscard]] std::string toString() const {
            return AsNarrowStdString();
        }

        [[nodiscard]] std::wstring toWString() const {
            return boost::locale::conv::utf_to_utf<wchar_t>(
                AsNarrowStdString());
        }

        [[nodiscard]] std::string AsStdString() const {
            if(!Ptr)
                return "";
            // this constant string value must match std::string in
            // type
            tTJSNarrowStringHolder holder(Ptr->operator const tjs_char *());
            return { holder.operator const char *() };
        }

        [[nodiscard]] std::string AsNarrowStdString() const {
            if(!Ptr)
                return "";
            // this constant string value must match std::string in
            // type
            tTJSNarrowStringHolder holder(Ptr->operator const tjs_char *());
            return { holder.operator const char *() };
        }

        [[nodiscard]] tTJSVariantString *AsVariantStringNoAddRef() const {
            return Ptr;
        }

        [[nodiscard]] tjs_int64 AsInteger() const;

        //-------------------------------------------------------
        // substitution --
        tTJSString &operator=(const tTJSString &rhs) {
            if(rhs.Ptr)
                rhs.Ptr->AddRef();
            if(Ptr)
                Ptr->Release();
            Ptr = rhs.Ptr;
            return *this;
        }

        tTJSString &operator=(const tjs_char *rhs) {
            if(Ptr) {
                Independ();
                if(rhs && rhs[0])
                    Ptr->ResetString(rhs);
                else
                    Ptr->Release(), Ptr = nullptr;
            } else {
                Ptr = TJSAllocVariantString(rhs);
            }
            return *this;
        }

        tTJSString &operator=(const tjs_nchar *rhs) {
            if(Ptr)
                Ptr->Release();
            Ptr = TJSAllocVariantString(rhs);
            return *this;
        }

        bool operator==(const tTJSString &ref) const {
            if(this->Ptr == ref.Ptr)
                return true;
            if(!this->Ptr || !ref.Ptr)
                return false;
            if(this->Ptr->Length != ref.Ptr->Length)
                return false;
            return !TJS_strcmp(*this->Ptr, *ref.Ptr);
        }

        bool operator!=(const tTJSString &ref) const {
            return !this->operator==(ref);
        }

        [[nodiscard]] tjs_int CompareIC(const tTJSString &ref) const {
            if(!this->Ptr && !ref.Ptr)
                return true;
            if(!this->Ptr || !ref.Ptr)
                return false;
            return TJS_stricmp(*this->Ptr, *ref.Ptr);
        }

        bool operator==(const tjs_char *ref) const {
            bool rnemp = ref && ref[0];
            if(!this->Ptr && !rnemp)
                return true;
            if(!this->Ptr || !rnemp)
                return false;
            return !TJS_strcmp(*this->Ptr, ref);
        }

        bool operator!=(const tjs_char *ref) const {
            return !this->operator==(ref);
        }

        tjs_int CompareIC(const tjs_char *ref) const {
            bool rnemp = ref && ref[0];
            if(!this->Ptr && !rnemp)
                return true;
            if(!this->Ptr || !rnemp)
                return false;
            return TJS_strcmp(*this->Ptr, ref);
        }

        bool operator<(const tTJSString &ref) const {
            if(!this->Ptr && !ref.Ptr)
                return false;
            if(!this->Ptr)
                return true;
            if(!ref.Ptr)
                return false;
            return TJS_strcmp(*this->Ptr, *ref.Ptr) < 0;
        }

        bool operator>(const tTJSString &ref) const {
            if(!this->Ptr && !ref.Ptr)
                return false;
            if(!this->Ptr)
                return false;
            if(!ref.Ptr)
                return true;
            return TJS_strcmp(*this->Ptr, *ref.Ptr) > 0;
        }

        //----------------------------------------------------------
        // operation --
        void operator+=(const tTJSString &ref) {
            if(!ref.Ptr)
                return;
            Independ();
            Ptr = TJSAppendVariantString(Ptr, *ref.Ptr);
        }

        void operator+=(const tTJSVariantString *ref) {
            if(!ref)
                return;
            Independ();
            Ptr = TJSAppendVariantString(Ptr, ref);
        }

        void operator+=(const tjs_char *ref) {
            if(!ref)
                return;
            Independ();
            Ptr = TJSAppendVariantString(Ptr, ref);
        }

        void operator+=(tjs_char rch) {
            Independ();
            tjs_char ch[2];
            ch[0] = rch;
            ch[1] = 0;
            Ptr = TJSAppendVariantString(Ptr, ch);
        }

        tTJSString operator+(const tTJSString &ref) const {
            if(!ref.Ptr && !Ptr)
                return {};
            if(!ref.Ptr)
                return *this;
            if(!Ptr)
                return ref;

            tTJSString newstr;
            newstr.Ptr = TJSAllocVariantString(*this->Ptr, *ref.Ptr);
            return newstr;
        }

        tTJSString operator+(const tjs_char *ref) const {
            if(!ref && !Ptr)
                return {};
            if(!ref)
                return *this;
            if(!Ptr)
                return { ref };

            tTJSString newstr;
            newstr.Ptr = TJSAllocVariantString(*this->Ptr, ref);
            return newstr;
        }

        tTJSString operator+(tjs_char rch) const {
            if(!Ptr)
                return { rch };
            tjs_char ch[2];
            ch[0] = rch;
            ch[1] = 0;
            tTJSString newstr;
            newstr.Ptr = TJSAllocVariantString(*this->Ptr, ch);
            return newstr;
        }

        friend tTJSString operator+(const tjs_char *lhs, const tTJSString &rhs);

        tjs_char operator[](tjs_uint i) const {
            // returns character at i. this function does not check
            // the range.
            if(!Ptr)
                return 0;
            return Ptr->operator const tjs_char *()[i];
        }

        void Clear() {
            if(Ptr)
                Ptr->Release(), Ptr = nullptr;
        }

        tjs_char *AllocBuffer(tjs_uint len) {
            /* you must call FixLen when you allocate larger buffer
             * than actual string length */

            if(Ptr)
                Ptr->Release();
            Ptr = TJSAllocVariantStringBuffer(len);
            return const_cast<tjs_char *>(Ptr->operator const tjs_char *());
        }

        tjs_char *AppendBuffer(tjs_uint len) {
            /* you must call FixLen when you allocate larger buffer
             * than actual string length */

            if(!Ptr)
                return AllocBuffer(len);
            Independ();
            Ptr->AppendBuffer(len);
            return const_cast<tjs_char *>(Ptr->operator const tjs_char *());
        }

        void FixLen() {
            Independ();
            if(Ptr)
                Ptr = Ptr->FixLength();
        }

        void Replace(const tTJSString &from, const tTJSString &to,
                     bool forall = true);

        [[nodiscard]] tTJSString AsLowerCase() const;

        [[nodiscard]] tTJSString AsUpperCase() const;

        void AsLowerCase(tTJSString &dest) const { dest = AsLowerCase(); }

        void AsUpperCase(tTJSString &dest) const { dest = AsUpperCase(); }

        void ToLowerCase();

        void ToUpperCase();

        template <typename... T>
        size_t printf(const char *format, const T &...args) {
            std::u16string utf16_string =
                boost::locale::conv::utf_to_utf<char16_t>(
                    fmt::sprintf(format, args...));
            size_t len = utf16_string.length();
            AllocBuffer(len);
            TJS_strcpy(const_cast<tjs_char *>(c_str()), utf16_string.c_str());
            FixLen();
            return len;
        }

        [[nodiscard]] tTJSString
        EscapeC() const; // c-style string escape/unescaep
        [[nodiscard]] tTJSString UnescapeC() const;

        void EscapeC(tTJSString &dest) const { dest = EscapeC(); }

        void UnescapeC(tTJSString &dest) const { dest = UnescapeC(); }

        bool StartsWith(const tjs_char *string) const;

        [[nodiscard]] bool StartsWith(const tTJSString &string) const {
            return StartsWith(string.c_str());
        }

        tjs_uint32 *GetHint() { return Ptr ? Ptr->GetHint() : nullptr; }

        [[nodiscard]] tjs_int GetNarrowStrLen() const;

        void ToNarrowStr(tjs_nchar *dest, tjs_int destmaxlen) const;

        //-------------------------------------------------------------
        // others --
        [[nodiscard]] bool IsEmpty() const { return Ptr == nullptr; }

    private:
        tjs_char *InternalIndepend();

    public:
        tjs_char *Independ() {
            // severs sharing of the string instance
            // and returns independent internal buffer

            // note that you must call FixLen after making
            // modification of the buffer if you shorten the string
            // using this method's return value. USING THIS METHOD'S
            // RETURN VALUE AND MODIFYING THE INTERNAL BUFFER IS VERY
            // DANGER.

            if(!Ptr)
                return nullptr;

            if(Ptr->GetRefCount() == 0) {
                // already indepentent
                return const_cast<tjs_char *>(Ptr->operator const tjs_char *());
            }
            return InternalIndepend();
        }

        [[nodiscard]] tjs_int GetLen() const { return Ptr->GetLength(); }

        [[nodiscard]] tjs_int length() const { return GetLen(); }

        [[nodiscard]] tjs_char GetLastChar() const {
            if(!Ptr)
                return (tjs_char)0;
            const tjs_char *p = Ptr->operator const tjs_char *();
            return p[Ptr->GetLength() - 1];
        }

        //----------------------------------------------
        // allocator/deallocater --
        static void *operator new(size_t size) { return new char[size]; }

        void operator delete(void *p) { delete[]((char *)p); }

        void *operator new[](size_t size) { return new char[size]; }

        void operator delete[](void *p) { delete[]((char *)p); }

        void *operator new(size_t size, void *buf) { return buf; }

        //--------------------------------------------- indexer /
        // finder --
        [[nodiscard]] int IndexOf(const tTJSString &str,
                                  unsigned int pos = 0) const;

        int IndexOf(const char *s, unsigned int pos = 0,
                    unsigned int n = -1) const {
            return IndexOf(tTJSString(s, n), pos);
        }

        [[nodiscard]] int IndexOf(tjs_char c, unsigned int pos = 0) const {
            return IndexOf(tTJSString(&c, 1), pos);
        }

        [[nodiscard]] tTJSString SubString(unsigned int pos,
                                           unsigned int len) const;

        [[nodiscard]] tTJSString Trim() const;
    }; // class tTJSString
    extern tTJSString operator+(const tjs_char *lhs, const tTJSString &rhs);

    extern tTJSString TJSInt32ToHex(tjs_uint32 num, int zeropad = 8);

    typedef tTJSString ttstr;

    //---------------------------------------------------------------------------
} // namespace TJS

static TJS::tTJSString operator"" _tss(const char *str, size_t len) {
    return TJS::tTJSString{ str, len };
}

#endif
