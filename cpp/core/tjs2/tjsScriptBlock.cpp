//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Script Block Management
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include "tjsScriptBlock.h"
#include "tjsInterCodeGen.h"
#include "tjsConstArrayData.h"
#include "tjs.h"

namespace TJS {
    //---------------------------------------------------------------------------
    // tTJSScriptBlock
    //---------------------------------------------------------------------------
    tTJSScriptBlock::tTJSScriptBlock(tTJS *owner) {
        RefCount = 1;
        Owner = owner;
        Owner->AddRef();

        Script = nullptr;
        Name = nullptr;

        InterCodeContext = nullptr;
        TopLevelContext = nullptr;
        LexicalAnalyzer = nullptr;

        UsingPreProcessor = false;

        LineOffset = 0;

        Owner->AddScriptBlock(this);
    }

    //---------------------------------------------------------------------------
    // for Bytecode
    tTJSScriptBlock::tTJSScriptBlock(tTJS *owner, const tjs_char *name,
                                     tjs_int lineoffset) {
        RefCount = 1;
        Owner = owner;
        Owner->AddRef();
        Name = nullptr;
        if(name) {
            Name = new tjs_char[TJS_strlen(name) + 1];
            TJS_strcpy(Name, name);
        }
        LineOffset = lineoffset;
        Script = nullptr;

        InterCodeContext = nullptr;
        TopLevelContext = nullptr;
        LexicalAnalyzer = nullptr;

        UsingPreProcessor = false;

        Owner->AddScriptBlock(this);
    }

    //---------------------------------------------------------------------------
    tTJSScriptBlock::~tTJSScriptBlock() {
        if(TopLevelContext)
            TopLevelContext->Release(), TopLevelContext = nullptr;
        while(!ContextStack.empty()) {
            ContextStack.top()->Release();
            ContextStack.pop();
        }

        Owner->RemoveScriptBlock(this);

        delete LexicalAnalyzer;
        delete[] Script;
        delete[] Name;

        Owner->Release();
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::AddRef() { RefCount++; }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Release() {
        if(RefCount <= 1)
            delete this;
        else
            RefCount--;
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::SetName(const tjs_char *name, tjs_int lineofs) {
        if(Name)
            delete[] Name, Name = nullptr;
        if(name) {
            LineOffset = lineofs;
            Name = new tjs_char[TJS_strlen(name) + 1];
            TJS_strcpy(Name, name);
        }
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Add(tTJSInterCodeContext *cntx) {
        InterCodeContextList.push_back(cntx);
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Remove(tTJSInterCodeContext *cntx) {
        InterCodeContextList.remove(cntx);
    }

    //---------------------------------------------------------------------------
    tjs_uint tTJSScriptBlock::GetTotalVMCodeSize() const {
        tjs_uint size = 0;

        std::list<tTJSInterCodeContext *>::const_iterator i;
        for(i = InterCodeContextList.begin(); i != InterCodeContextList.end();
            i++) {
            size += (*i)->GetCodeSize();
        }
        return size;
    }

    //---------------------------------------------------------------------------
    tjs_uint tTJSScriptBlock::GetTotalVMDataSize() const {
        tjs_uint size = 0;

        std::list<tTJSInterCodeContext *>::const_iterator i;
        for(i = InterCodeContextList.begin(); i != InterCodeContextList.end();
            i++) {
            size += (*i)->GetDataSize();
        }
        return size;
    }

    //---------------------------------------------------------------------------
    const tjs_char *tTJSScriptBlock::GetLine(tjs_int line,
                                             tjs_int *linelength) const {
        if(Script == nullptr) {
            *linelength = 10;
            return TJS_W("Bytecode.");
        }
        // note that this function DOES matter LineOffset
        line -= LineOffset;
        if(linelength)
            *linelength = LineLengthVector[line];
        return Script + LineVector[line];
    }

    //---------------------------------------------------------------------------
    tjs_int tTJSScriptBlock::SrcPosToLine(tjs_int pos) const {
        tjs_uint s = 0;
        auto e = (tjs_uint)LineVector.size();
        while(true) {
            if(e - s <= 1)
                return s + LineOffset; // LineOffset is added
            tjs_uint m = s + (e - s) / 2;
            if(LineVector[m] > pos)
                e = m;
            else
                s = m;
        }
    }

    //---------------------------------------------------------------------------
    tjs_int tTJSScriptBlock::LineToSrcPos(tjs_int line) const {
        // assumes line is added by LineOffset
        line -= LineOffset;
        return LineVector[line];
    }

    //---------------------------------------------------------------------------
    ttstr tTJSScriptBlock::GetLineDescriptionString(tjs_int pos) const {
        // get short description, like "mainwindow.tjs(321)"
        // pos is in character count from the first of the script
        tjs_int line = SrcPosToLine(pos) + 1;
        ttstr name;
        if(Name) {
            name = Name;
        } else {
            ttstr ptr{ fmt::format("0x{:p}", static_cast<const void *>(this)) };
            name = ttstr(TJS_W("anonymous@")) + ptr;
        }

        return name + TJS_W("(") + ttstr(line) + TJS_W(")");
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::ConsoleOutput(const tjs_char *msg, void *data) {
        auto *blk = (tTJSScriptBlock *)data;
        blk->Owner->OutputToConsole(msg);
    }
    //---------------------------------------------------------------------------

    void tTJSScriptBlock::SetText(tTJSVariant *result, const tjs_char *text,
                                  iTJSDispatch2 *context, bool isexpression) {

        // compiles text and executes its global level scripts.
        // the script will be compiled as an expression if isexpressn
        // is true.
        if(!text)
            return;
        if(!text[0])
            return;

        TJS_D((TJS_W("Counting lines ...\n")))

        Script = new tjs_char[TJS_strlen(text) + 1];
        TJS_strcpy(Script, text);

        // calculation of line-count
        tjs_char *ls = Script;
        tjs_char *p = Script;
        while(*p) {
            if(*p == TJS_W('\r') || *p == TJS_W('\n')) {
                LineVector.push_back(int(ls - Script));
                LineLengthVector.push_back(int(p - ls));
                if(*p == TJS_W('\r') && p[1] == TJS_W('\n'))
                    p++;
                p++;
                ls = p;
            } else {
                p++;
            }
        }

        if(p != ls) {
            LineVector.push_back(int(ls - Script));
            LineLengthVector.push_back(int(p - ls));
        }

        try {

            // parse and execute
#ifdef TJS_DEBUG_PROFILE_TIME
            {
                tTJSTimeProfiler p(parsetime);
#endif

                Parse(text, isexpression, result != nullptr);

#ifdef TJS_DEBUG_PROFILE_TIME
            }

            {
                char buf[256];
                TJS_nsprintf(buf, "parsing : %d", parsetime);
                OutputDebugString(buf);
                if(parsetime) {
                    TJS_nsprintf(buf, "Commit : %d (%d%%)", time_Commit,
                                 time_Commit * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "yylex : %d (%d%%)", time_yylex,
                                 time_yylex * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "MakeNP : %d (%d%%)", time_make_np,
                                 time_make_np * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "GenNodeCode : %d (%d%%)",
                                 time_GenNodeCode,
                                 time_GenNodeCode * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "  PutCode : %d (%d%%)", time_PutCode,
                                 time_PutCode * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "  PutData : %d (%d%%)", time_PutData,
                                 time_PutData * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "  this_proxy : %d (%d%%)",
                                 time_this_proxy,
                                 time_this_proxy * 100 / parsetime);
                    OutputDebugString(buf);

                    TJS_nsprintf(buf, "ns::Push : %d (%d%%)", time_ns_Push,
                                 time_ns_Push * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "ns::Pop : %d (%d%%)", time_ns_Pop,
                                 time_ns_Pop * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "ns::Find : %d (%d%%)", time_ns_Find,
                                 time_ns_Find * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "ns::Remove : %d (%d%%)", time_ns_Remove,
                                 time_ns_Remove * 100 / parsetime);
                    OutputDebugString(buf);
                    TJS_nsprintf(buf, "ns::Commit : %d (%d%%)", time_ns_Commit,
                                 time_ns_Commit * 100 / parsetime);
                    OutputDebugString(buf);
                }
            }
#endif

#ifdef TJS_DEBUG_DISASM
            std::list<tTJSInterCodeContext *>::iterator i =
                InterCodeContextList.begin();
            while(i != InterCodeContextList.end()) {
                ConsoleOutput(TJS_W(""), (void *)this);
                ConsoleOutput((*i)->GetName(), (void *)this);
                (*i)->Disassemble(ConsoleOutput, (void *)this);
                i++;
            }
#endif

            // execute global level script
            ExecuteTopLevelScript(result, context);
        } catch(...) {
            if(InterCodeContextList.size() != 1) {
                if(TopLevelContext)
                    TopLevelContext->Release(), TopLevelContext = nullptr;
                while(ContextStack.size()) {
                    ContextStack.top()->Release();
                    ContextStack.pop();
                }
            }
            throw;
        }

        if(InterCodeContextList.size() != 1) {
            // this is not a single-context script block
            // (may hook itself)
            // release all contexts and global at this time
            if(TopLevelContext)
                TopLevelContext->Release(), TopLevelContext = nullptr;
            while(ContextStack.size()) {
                ContextStack.top()->Release();
                ContextStack.pop();
            }
        }
    }

    //---------------------------------------------------------------------------
    // for Bytecode
    void tTJSScriptBlock::ExecuteTopLevel(tTJSVariant *result,
                                          iTJSDispatch2 *context) {
        try {
#ifdef TJS_DEBUG_DISASM
            std::list<tTJSInterCodeContext *>::iterator i =
                InterCodeContextList.begin();
            while(i != InterCodeContextList.end()) {
                ConsoleOutput(TJS_W(""), (void *)this);
                ConsoleOutput((*i)->GetName(), (void *)this);
                (*i)->Disassemble(ConsoleOutput, (void *)this);
                i++;
            }
#endif

            // execute global level script
            ExecuteTopLevelScript(result, context);
        } catch(...) {
            if(InterCodeContextList.size() != 1) {
                if(TopLevelContext)
                    TopLevelContext->Release(), TopLevelContext = nullptr;
                while(ContextStack.size()) {
                    ContextStack.top()->Release();
                    ContextStack.pop();
                }
            }
            throw;
        }

        if(InterCodeContextList.size() != 1) {
            // this is not a single-context script block
            // (may hook itself)
            // release all contexts and global at this time
            if(TopLevelContext)
                TopLevelContext->Release(), TopLevelContext = nullptr;
            while(ContextStack.size()) {
                ContextStack.top()->Release();
                ContextStack.pop();
            }
        }
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::ExecuteTopLevelScript(tTJSVariant *result,
                                                iTJSDispatch2 *context) {
        if(TopLevelContext) {
#ifdef TJS_DEBUG_PROFILE_TIME
            clock_t start = clock();
#endif
            TopLevelContext->FuncCall(0, nullptr, nullptr, result, 0, nullptr,
                                      context);
#ifdef TJS_DEBUG_PROFILE_TIME
            tjs_char str[100];
            TJS_sprintf(str, TJS_W("%d"), clock() - start);
            ConsoleOutput(str, (void *)this);
#endif
        }
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::PushContextStack(const tjs_char *name,
                                           tTJSContextType type) {
        tTJSInterCodeContext *cntx;
        cntx = new tTJSInterCodeContext(InterCodeContext, name, this, type);
        if(InterCodeContext == nullptr) {
            if(TopLevelContext)
                TJS_eTJSError(TJSInternalError);
            TopLevelContext = cntx;
            TopLevelContext->AddRef();
        }
        ContextStack.push(cntx);
        InterCodeContext = cntx;
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::PopContextStack() {
        InterCodeContext->Commit();
        InterCodeContext->Release();
        ContextStack.pop();
        if(!ContextStack.empty())
            InterCodeContext = ContextStack.top();
        else
            InterCodeContext = nullptr;
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Parse(const tjs_char *script, bool isexpr,
                                bool resultneeded) {
        TJS_F_TRACE("tTJSScriptBlock::Parse");

        if(!script)
            return;

        CompileErrorCount = 0;

        LexicalAnalyzer =
            new tTJSLexicalAnalyzer(this, script, isexpr, resultneeded);

        try {
            parser{ this }.parse();
        } catch(...) {
            delete LexicalAnalyzer;
            LexicalAnalyzer = nullptr;
            throw;
        }

        delete LexicalAnalyzer;
        LexicalAnalyzer = nullptr;

        if(CompileErrorCount) {
            TJS_eTJSScriptError(FirstError, this, FirstErrorPos);
        }
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::SetFirstError(const tjs_char *error, tjs_int pos) {
        if(CompileErrorCount == 0) {
            FirstError = error;
            FirstErrorPos = pos;
        }
    }

    //---------------------------------------------------------------------------
    ttstr tTJSScriptBlock::GetNameInfo() const {
        if(LineOffset == 0) {
            return { Name };
        }
        return ttstr(Name) + TJS_W("(line +") + ttstr(LineOffset) + TJS_W(")");
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Dump() const {
        auto i = InterCodeContextList.begin();
        while(i != InterCodeContextList.end()) {
            ConsoleOutput(TJS_W(""), (void *)this);
            ttstr ptr{ fmt::format(" {}", static_cast<void *>((*i))) };
            ConsoleOutput((ttstr(TJS_W("(")) +
                           ttstr((*i)->GetContextTypeName()) + TJS_W(") ") +
                           ttstr((*i)->GetName()) + ptr)
                              .c_str(),
                          (void *)this);
            (*i)->Disassemble(ConsoleOutput, (void *)this);
            ++i;
        }
    }

    void tTJSScriptBlock::Dump(tTJSBinaryStream *stream) const {
        struct TmpTJSConsoleOutput : public iTJSConsoleOutput {
            tTJSBinaryStream *_stream;
            explicit TmpTJSConsoleOutput(tTJSBinaryStream *s) : _stream(s) {}

            void ExceptionPrint(const tjs_char *msg) override { Print(msg); }

            void Print(const tjs_char *msg) override {
                _stream->Write(msg, TJS_strlen(msg));
            }
        } output{ stream };
        auto i = InterCodeContextList.begin();
        while(i != InterCodeContextList.end()) {
            output.Print(TJS_W(""));
            ttstr ptr{ fmt::format(" {}", static_cast<void *>((*i))) };
            output.Print((ttstr(TJS_W("(")) +
                          ttstr((*i)->GetContextTypeName()) + TJS_W(") ") +
                          ttstr((*i)->GetName()) + ptr)
                             .c_str());
            (*i)->Disassemble([&](const tjs_char *msg,
                                  void *data) -> void { output.Print(msg); },
                              (void *)this);
            ++i;
        }
    }

    //---------------------------------------------------------------------------
    // for Bytecode
    tjs_int
    tTJSScriptBlock::GetCodeIndex(const tTJSInterCodeContext *ctx) const {
        tjs_int index = 0;
        for(auto i = InterCodeContextList.begin();
            i != InterCodeContextList.end(); ++i, index++) {
            if((*i) == ctx) {
                return index;
            }
        }
        return -1;
    }

    //---------------------------------------------------------------------------
    const tjs_uint8
        tTJSScriptBlock::BYTECODE_FILE_TAG[BYTECODE_FILE_TAG_SIZE] = {
            'T', 'J', 'S', '2', '1', '0', '0', 0
        };
    const tjs_uint8 tTJSScriptBlock::BYTECODE_CODE_TAG[BYTECODE_TAG_SIZE] = {
        'T', 'J', 'S', '2'
    };
    const tjs_uint8 tTJSScriptBlock::BYTECODE_OBJ_TAG[BYTECODE_TAG_SIZE] = {
        'O', 'B', 'J', 'S'
    };
    const tjs_uint8 tTJSScriptBlock::BYTECODE_DATA_TAG[BYTECODE_TAG_SIZE] = {
        'D', 'A', 'T', 'A'
    };

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::ExportByteCode(bool outputdebug,
                                         tTJSBinaryStream *output) {
        const int count = (int)InterCodeContextList.size();
        std::vector<std::vector<tjs_uint8> *> objarray;
        objarray.reserve(count * 2);
        auto *constarray = new tjsConstArrayData();
        int objsize = 0;
        for(auto obj : InterCodeContextList) {
            std::vector<tjs_uint8> *buf =
                obj->ExportByteCode(outputdebug, this, *constarray);
            objarray.push_back(buf);
            objsize += (int)buf->size() + BYTECODE_TAG_SIZE +
                BYTECODE_CHUNK_SIZE_LEN; // tag + size
        }

        objsize += BYTECODE_TAG_SIZE + BYTECODE_CHUNK_SIZE_LEN + 4 +
            4; // OBJS tag + size + toplevel + count
        std::vector<tjs_uint8> *dataarea = constarray->ExportBuffer();
        int datasize = (int)dataarea->size() + BYTECODE_TAG_SIZE +
            BYTECODE_CHUNK_SIZE_LEN; // DATA tag + size
        int filesize = objsize + datasize + BYTECODE_FILE_TAG_SIZE +
            BYTECODE_CHUNK_SIZE_LEN; // TJS2 tag + file size
        int toplevel = -1;
        if(TopLevelContext != nullptr) {
            toplevel = GetCodeIndex(TopLevelContext);
        }
        tjs_uint8 tmp[4];
        output->Write(BYTECODE_FILE_TAG, 8);
        Write4Byte(tmp, filesize);
        output->Write(tmp, 4);
        output->Write(BYTECODE_DATA_TAG, 4);
        Write4Byte(tmp, datasize);
        output->Write(tmp, 4);
        output->Write(&((*dataarea)[0]), (tjs_uint)dataarea->size());
        output->Write(BYTECODE_OBJ_TAG, 4);
        Write4Byte(tmp, objsize);
        output->Write(tmp, 4);
        Write4Byte(tmp, toplevel);
        output->Write(tmp, 4);
        Write4Byte(tmp, count);
        output->Write(tmp, 4);
        for(int i = 0; i < count; i++) {
            std::vector<tjs_uint8> *buf = objarray[i];
            int size = (int)buf->size();
            output->Write(BYTECODE_CODE_TAG, 4);
            Write4Byte(tmp, size);
            output->Write(tmp, 4);
            output->Write(&((*buf)[0]), size);
            delete buf;
            objarray[i] = nullptr;
        }
        objarray.clear();
        delete constarray;
        delete dataarea;
    }

    //---------------------------------------------------------------------------
    void tTJSScriptBlock::Compile(const tjs_char *text, bool isexpression,
                                  bool isresultneeded, bool outputdebug,
                                  tTJSBinaryStream *output) {
        if(!text)
            return;
        if(!text[0])
            return;

        tTJSInterCodeContext::IsBytecodeCompile = true;
        try {
            Script = new tjs_char[TJS_strlen(text) + 1];
            TJS_strcpy(Script, text);

            // calculation of line-count
            tjs_char *ls = Script;
            tjs_char *p = Script;
            while(*p) {
                if(*p == TJS_W('\r') || *p == TJS_W('\n')) {
                    LineVector.push_back(int(ls - Script));
                    LineLengthVector.push_back(int(p - ls));
                    if(*p == TJS_W('\r') && p[1] == TJS_W('\n'))
                        p++;
                    p++;
                    ls = p;
                } else {
                    p++;
                }
            }
            if(p != ls) {
                LineVector.push_back(int(ls - Script));
                LineLengthVector.push_back(int(p - ls));
            }

            Parse(text, isexpression, isresultneeded);

            ExportByteCode(outputdebug, output);
        } catch(...) {
            if(InterCodeContextList.size() != 1) {
                if(TopLevelContext)
                    TopLevelContext->Release(), TopLevelContext = nullptr;
                while(!ContextStack.empty()) {
                    ContextStack.top()->Release();
                    ContextStack.pop();
                }
            }
            tTJSInterCodeContext::IsBytecodeCompile = false;
            throw;
        }
        if(InterCodeContextList.size() != 1) {
            if(TopLevelContext)
                TopLevelContext->Release(), TopLevelContext = nullptr;
            while(!ContextStack.empty()) {
                ContextStack.top()->Release();
                ContextStack.pop();
            }
        }
        tTJSInterCodeContext::IsBytecodeCompile = false;
    }

    inline void OffsetReg(tjs_int32 &x) { x = TJS_FROM_VM_REG_ADDR(x); }

    inline void OffsetCode(tjs_int32 &x) { x = TJS_FROM_VM_CODE_ADDR(x); }

    // Safe index helper: returns true if code[i + offset] is a valid access
    // (0-based offset)
    inline bool SafeIndex(const tjs_int i, const int offset,
                          const int codeSize) {
        return i + offset < codeSize;
    }

    int HandleOp2(tjs_int32 *code, const int i, const int opcode,
                  const int codeSize) {
        switch(code[i] - opcode) {
            case 0: // base
                if(!SafeIndex(i, 2, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                OffsetReg(code[i + 2]);
                return 3;
            case 1:
            case 2:
                if(!SafeIndex(i, 4, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                OffsetReg(code[i + 2]);
                OffsetReg(code[i + 3]);
                OffsetReg(code[i + 4]);
                return 5;
            case 3:
                if(!SafeIndex(i, 3, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                OffsetReg(code[i + 2]);
                OffsetReg(code[i + 3]);
                return 4;
            default:
                return 1; // fallback
        }
    }

    int HandleIncDec(tjs_int32 *code, const int i, const int opcode,
                     const int codeSize) {
        switch(code[i] - opcode) {
            case 0:
                if(!SafeIndex(i, 1, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                return 2;
            case 1:
            case 2:
                if(!SafeIndex(i, 3, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                OffsetReg(code[i + 2]);
                OffsetReg(code[i + 3]);
                return 4;
            case 3:
                if(!SafeIndex(i, 2, codeSize))
                    return 1;
                OffsetReg(code[i + 1]);
                OffsetReg(code[i + 2]);
                return 3;
            default:
                return 1;
        }
    }

    int HandleCallOp(tjs_int32 *code, const int i, const int opcode,
                     const int codeSize) {
        // ensure at least i+2 exists (to read code[i+1], code[i+2])
        if(!SafeIndex(i, 2, codeSize))
            return 1;
        OffsetReg(code[i + 1]);
        OffsetReg(code[i + 2]);

        int st; // start of arguments
        if(opcode == VM_CALLD || opcode == VM_CALLI) {
            if(!SafeIndex(i, 3, codeSize))
                return 1;
            OffsetReg(code[i + 3]);
            st = 5;
        } else {
            st = 4;
        }

        // we need code[i + st - 1] to exist to get argument count
        if(!SafeIndex(i, st - 1, codeSize))
            return st;

        const int num = code[i + st - 1]; // argument count or special flag
        if(num >= 0) {
            // normal: st + num bytes (each argument is a single reg)
            if(!SafeIndex(i, st + num - 1, codeSize))
                return st;
            for(int j = 0; j < num; ++j) {
                OffsetReg(code[i + st + j]);
            }
            return st + num;
        }
        if(num == -1) {
            // -1: no arguments, size = st
            return st;
        }
        if(num == -2) {
            // expanded args: next byte contains argCount, followed by argCount
            // * 2 pairs
            st++;
            if(!SafeIndex(i, st - 1, codeSize))
                return st;
            const int argCount = code[i + st - 1];
            if(!SafeIndex(i, st + argCount * 2 - 1, codeSize))
                return st;
            for(int j = 0; j < argCount; ++j) {
                if(const int flag = code[i + st + j * 2];
                   flag == fatNormal || flag == fatExpand) {
                    OffsetReg(code[i + st + j * 2 + 1]);
                }
            }
            return st + argCount * 2;
        }

        return st;
    }

    // =======================================
    // 指令信息表
    // =======================================
    struct OpInfo {
        int size; // 基本指令长度（不含可变部分）
        int regCount; // 固定寄存器参数数
        bool hasCodeAddr; // 是否包含 code 地址
        std::function<int(tjs_int32 *, int, int, int)> handler; // 特殊处理函数
    };

    // ------------------ opTable construction ------------------
    static std::unordered_map<int, OpInfo> BuildOpTable() {
        std::unordered_map<int, OpInfo> m;

        // fixed simple ops
        m[VM_NOP] = { 1, 0, false, nullptr };
        m[VM_NF] = { 1, 0, false, nullptr };
        m[VM_RET] = { 1, 0, false, nullptr };
        m[VM_EXTRY] = { 1, 0, false, nullptr };
        m[VM_REGMEMBER] = { 1, 0, false, nullptr };
        m[VM_DEBUGGER] = { 1, 0, false, nullptr };

        // jumps (code address at i+1)
        m[VM_JMP] = { 2, 0, true, nullptr };
        m[VM_JF] = { 2, 0, true, nullptr };
        m[VM_JNF] = { 2, 0, true, nullptr };

        // single-reg unary ops (size=2)
        constexpr int singleRegs[] = { VM_TT,     VM_TF,     VM_SETF,  VM_SETNF,
                                       VM_LNOT,   VM_BNOT,   VM_ASC,   VM_CHR,
                                       VM_NUM,    VM_CHS,    VM_CL,    VM_INV,
                                       VM_CHKINV, VM_TYPEOF, VM_EVAL,  VM_EEXP,
                                       VM_INT,    VM_REAL,   VM_STR,   VM_OCTET,
                                       VM_SRV,    VM_THROW,  VM_GLOBAL };
        for(int op : singleRegs)
            m[op] = { 2, 1, false, nullptr };

        // CCL (size=3, 1 reg)
        m[VM_CCL] = { 3, 1, false, nullptr };

        // two-reg ops (size=3)
        constexpr int twoRegs[] = { VM_CP,   VM_CEQ,    VM_CDEQ,  VM_CLT,
                                    VM_CGT,  VM_CHKINS, VM_CONST, VM_SETP,
                                    VM_GETP, VM_ADDCI };
        for(int op : twoRegs)
            m[op] = { 3, 2, false, nullptr };

        // three-reg ops (size=4)
        constexpr int threeRegs[] = { VM_GPD,     VM_GPDS, VM_SPD,    VM_SPDE,
                                      VM_SPDEH,   VM_SPDS, VM_GPI,    VM_GPIS,
                                      VM_SPI,     VM_SPIE, VM_SPIS,   VM_DELD,
                                      VM_TYPEOFD, VM_DELI, VM_TYPEOFI };
        for(int op : threeRegs)
            m[op] = { 4, 3, false, nullptr };

        // CHGTHIS (3 bytes, 2 regs)
        m[VM_CHGTHIS] = { 3, 2, false, nullptr };

        // ENTRY: original implementation did OffsetCode(code[i+1]) then
        // OffsetReg(code[i+2]) to preserve exact behavior we won't rely on
        // regCount/hasCodeAddr auto handling here.
        m[VM_ENTRY] = { 0, 0, false,
                        [](tjs_int32 *code, const int i, int,
                           const int codeSize) -> int {
                            if(!SafeIndex(i, 2, codeSize))
                                return 1;
                            // preserve original order: first code address, then
                            // reg
                            OffsetCode(code[i + 1]);
                            OffsetReg(code[i + 2]);
                            return 3;
                        } };

        // INC/DEC family -> handler
        m[VM_INC] = { 0, 0, false, HandleIncDec };
        m[VM_DEC] = { 0, 0, false, HandleIncDec };

        // for simplicity assume the OP2 group members are represented by
        // distinct enum values
        int op2_group[] = { VM_LOR, VM_LAND, VM_BOR,  VM_BXOR, VM_BAND,
                            VM_SAR, VM_SAL,  VM_SR,   VM_ADD,  VM_SUB,
                            VM_MOD, VM_DIV,  VM_IDIV, VM_MUL };
        for(int op : op2_group)
            m[op] = { 0, 0, false, HandleOp2 };

        // CALL/NEW family use HandleCallOp
        m[VM_CALL] = { 0, 0, false, HandleCallOp };
        m[VM_CALLD] = { 0, 0, false, HandleCallOp };
        m[VM_CALLI] = { 0, 0, false, HandleCallOp };
        m[VM_NEW] = { 0, 0, false, HandleCallOp };

        return m;
    }

    // Build table once (static)
    static std::unordered_map<int, OpInfo> opTable = BuildOpTable();


    void tTJSScriptBlock::TranslateCodeAddress(tjs_int32 *code,
                                               const tjs_int32 codeSize) {
        tjs_int i = 0;
        while(i < codeSize) {
            int opcode = code[i];
            auto it = opTable.find(opcode);
            if(it == opTable.end()) {
                // unknown opcode: behave like original default (advance by 1)
                i += 1;
                continue;
            }

            auto &[size, regCount, hasCodeAddr, handler] = it->second;
            if(handler) {
                size = handler(code, i, opcode, codeSize);
            } else {
                // fixed-size handling: do bounds checks before touching fields
                if(hasCodeAddr) {
                    // need code[i+1]
                    if(!SafeIndex(i, 1, codeSize)) {
                        // malformed bytecode: bail out similarly to original
                        // fallback
                        i = codeSize;
                        break;
                    }
                    OffsetCode(code[i + 1]);
                }
                for(int r = 0; r < regCount; ++r) {
                    if(!SafeIndex(i, 1 + r, codeSize)) {
                        i = codeSize;
                        break;
                    }
                    OffsetReg(code[i + 1 + r]);
                }
            }

            i += size;
        }

        if(codeSize != i) {
            // keep same error behavior as original
            TJS_eTJSScriptError(TJSInternalError, this, 0);
        }
    }

} // namespace TJS
