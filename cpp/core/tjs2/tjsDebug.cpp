//---------------------------------------------------------------------------
/*
        TJS2 Script Engine
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// TJS Debugging support
//---------------------------------------------------------------------------
#include "tjsCommHead.h"

#include <algorithm>
#include "tjsDebug.h"
#include "tjsHashSearch.h"
#include "tjsInterCodeGen.h"
#include "tjsGlobalStringMap.h"

namespace TJS {

    //---------------------------------------------------------------------------
    // ObjectHashMap : hash map to track object
    // construction/destruction
    //--------------------------------------------------------------------------
    tTJSBinaryStream *TJSObjectHashMapLog = nullptr; // log file object
    //---------------------------------------------------------------------------
    enum tTJSObjectHashMapLogItemId {
        liiEnd, // 00 end of the log
        liiVersion, // 01 identify log structure version
        liiAdd, // 02 add a object
        liiRemove, // 03 remove a object
        liiSetType, // 04 set object type information
        liiSetFlag, // 05 set object flag
    };

    //---------------------------------------------------------------------------
    template <typename T>
    void TJSStoreLog(const T &object) {
        TJSObjectHashMapLog->Write(&object, sizeof(object));
    }

    //---------------------------------------------------------------------------
    template <typename T>
    T TJSRestoreLog() {
        T object;
        TJSObjectHashMapLog->Read(&object, sizeof(object));
        return object;
    }

    //---------------------------------------------------------------------------
    template <>
    void TJSStoreLog<ttstr>(const ttstr &which) {
        // store a string into log stream
        tjs_int length = which.GetLen();
        TJSObjectHashMapLog->Write(&length, sizeof(length));
        // Note that this function does not care what the endian is.
        TJSObjectHashMapLog->Write(which.c_str(), length * sizeof(tjs_char));
    }

    //---------------------------------------------------------------------------
    template <>
    ttstr TJSRestoreLog<ttstr>() {
        // restore a string from log stream
        tjs_int length;
        TJSObjectHashMapLog->Read(&length, sizeof(length));
        ttstr ret((tTJSStringBufferLength)(length));
        tjs_char *buf = ret.Independ();
        TJSObjectHashMapLog->Read(buf, length * sizeof(tjs_char));
        buf[length] = 0;
        ret.FixLen();
        return ret;
    }

    //---------------------------------------------------------------------------
    template <>
    void TJSStoreLog<tTJSObjectHashMapLogItemId>(
        const tTJSObjectHashMapLogItemId &id) {
        // log item id
        char cid = id;
        TJSObjectHashMapLog->Write(&cid, sizeof(char));
    }

    //---------------------------------------------------------------------------
    template <>
    tTJSObjectHashMapLogItemId TJSRestoreLog<tTJSObjectHashMapLogItemId>() {
        // restore item id
        char cid;
        if(TJSObjectHashMapLog->Read(&cid, sizeof(char)) != sizeof(char))
            return liiEnd;
        return (tTJSObjectHashMapLogItemId)(cid);
    }

    //---------------------------------------------------------------------------
    struct tTJSObjectHashMapRecord {
        ttstr History; // call history where the object was created
        ttstr Where; // a label which indicates where the object was
                     // created
        ttstr Type; // object type ("class Array" etc)
        tjs_uint32 Flags;

        tTJSObjectHashMapRecord() : Flags(TJS_OHMF_EXIST) { ; }

        void StoreLog() {
            // store the object into log stream
            TJSStoreLog(History);
            TJSStoreLog(Where);
            TJSStoreLog(Type);
            TJSStoreLog(Flags);
        }

        void RestoreLog() {
            // restore the object from log stream
            History = TJSRestoreLog<ttstr>();
            Where = TJSRestoreLog<ttstr>();
            Type = TJSRestoreLog<ttstr>();
            Flags = TJSRestoreLog<tjs_uint32>();
        }
    };

    //---------------------------------------------------------------------------
    class tTJSObjectHashMapRecordComparator_HistoryAndType {
    public:
        bool operator()(const tTJSObjectHashMapRecord &lhs,
                        const tTJSObjectHashMapRecord &rhs) const {
            if(lhs.Where == rhs.Where)
                return lhs.Type < rhs.Type;
            return lhs.History < rhs.History;
        }
    };

    class tTJSObjectHashMapRecordComparator_Type {
    public:
        bool operator()(const tTJSObjectHashMapRecord &lhs,
                        const tTJSObjectHashMapRecord &rhs) const {
            return lhs.Type < rhs.Type;
        }
    };

    //---------------------------------------------------------------------------
    class tTJSObjectHashMap;

    tTJSObjectHashMap *TJSObjectHashMap;

    class tTJSObjectHashMap {
        typedef tTJSHashTable<void *, tTJSObjectHashMapRecord,
                              tTJSHashFunc<void *>, 1024>
            tHash;
        tHash Hash;

        tjs_int RefCount;

    public:
        tTJSObjectHashMap() {
            RefCount = 1;
            TJSObjectHashMap = this;
        }

    protected:
        ~tTJSObjectHashMap() { TJSObjectHashMap = nullptr; }

    protected:
        void _AddRef() { RefCount++; }

        void _Release() {
            if(RefCount == 1)
                delete this;
            else
                RefCount--;
        }

    public:
        static void AddRef() {
            if(TJSObjectHashMap)
                TJSObjectHashMap->_AddRef();
            else
                new tTJSObjectHashMap();
        }

        static void Release() {
            if(TJSObjectHashMap)
                TJSObjectHashMap->_Release();
        }

        void Add(void *object, const tTJSObjectHashMapRecord &record) {
            Hash.Add(object, record);
        }

        void SetType(void *object, const ttstr &info) {
            tTJSObjectHashMapRecord *rec;
            rec = Hash.Find(object);
            if(!rec)
                return;
            rec->Type = TJSMapGlobalStringMap(info);
        }

        ttstr GetType(void *object) {
            tTJSObjectHashMapRecord *rec;
            rec = Hash.Find(object);
            if(!rec)
                return ttstr();
            return rec->Type;
        }

        void SetFlag(void *object, tjs_uint32 flags_to_change,
                     tjs_uint32 bits) {
            tTJSObjectHashMapRecord *rec;
            rec = Hash.Find(object);
            if(!rec)
                return;
            rec->Flags &= (~flags_to_change);
            rec->Flags |= (bits & flags_to_change);
        }

        tjs_uint32 GetFlag(void *object) {
            tTJSObjectHashMapRecord *rec;
            rec = Hash.Find(object);
            if(!rec)
                return 0;
            return rec->Flags;
        }

        void Remove(void *object) { Hash.Delete(object); }

        void WarnIfObjectIsDeleting(iTJSConsoleOutput *output, void *object) {
            tTJSObjectHashMapRecord *rec;
            rec = Hash.Find(object);
            if(!rec)
                return;

            if(rec->Flags & TJS_OHMF_DELETING) {
                // warn running code on deleting-in-progress object
                ttstr warn(TJSWarning);
                ttstr tmp{ fmt::format("{}", static_cast<void *>(object)) };

                ttstr info(TJSWarnRunningCodeOnDeletingObject);
                info.Replace(TJS_W("%1"), tmp);
                info.Replace(TJS_W("%2"), rec->Type);
                info.Replace(TJS_W("%3"), rec->Where);
                info.Replace(TJS_W("%4"), TJSGetStackTraceString(1));

                output->Print((warn + info).c_str());
            }
        }

        void ReportAllUnfreedObjects(iTJSConsoleOutput *output) {
            {
                ttstr msg = (const tjs_char *)TJSNObjectsWasNotFreed;
                msg.Replace(TJS_W("%1"), ttstr((tjs_int)Hash.GetCount()));
                output->Print(msg.c_str());
            }

            // list all unfreed objects
            tHash::tIterator i;
            for(i = Hash.GetFirst(); !i.IsNull(); i++) {
                ttstr addr{ fmt::format("0x{:p}",
                                        static_cast<void *>(i.GetKey())) };
                ttstr info = (const tjs_char *)TJSObjectWasNotFreed;
                info.Replace(TJS_W("%1"), addr);
                info.Replace(TJS_W("%2"), i.GetValue().Type);
                info.Replace(TJS_W("%3"), i.GetValue().History);
                output->Print(info.c_str());
            }

            // group by the history and object type
            output->Print(TJS_W("---"));
            output->Print((const tjs_char *)TJSGroupByObjectTypeAndHistory);
            std::vector<tTJSObjectHashMapRecord> items;
            for(i = Hash.GetFirst(); !i.IsNull(); i++)
                items.push_back(i.GetValue());

            std::stable_sort(
                items.begin(), items.end(),
                tTJSObjectHashMapRecordComparator_HistoryAndType());

            ttstr history, type;
            tjs_int count = 0;
            if(!items.empty()) {
                for(auto i = items.begin();; i++) {
                    if(i != items.begin() &&
                       (i == items.end() || history != i->History ||
                        type != i->Type)) {
                        ttstr tmp{ fmt::format("{}", static_cast<int>(count)) };
                        ttstr info = (const tjs_char *)
                            TJSObjectCountingMessageGroupByObjectTypeAndHistory;
                        info.Replace(TJS_W("%1"), tmp);
                        info.Replace(TJS_W("%2"), type);
                        info.Replace(TJS_W("%3"), history);
                        output->Print(info.c_str());

                        if(i == items.end())
                            break;

                        count = 0;
                    }

                    history = i->History;
                    type = i->Type;
                    count++;
                }
            }

            // group by object type
            output->Print(TJS_W("---"));
            output->Print((const tjs_char *)TJSGroupByObjectType);
            std::stable_sort(items.begin(), items.end(),
                             tTJSObjectHashMapRecordComparator_Type());

            type.Clear();
            count = 0;
            if(!items.empty()) {
                for(auto i = items.begin();; i++) {
                    if(i != items.begin() &&
                       (i == items.end() || type != i->Type)) {
                        ttstr tmp{ fmt::format("{}", static_cast<int>(count)) };
                        ttstr info = (const tjs_char *)
                            TJSObjectCountingMessageTJSGroupByObjectType;
                        info.Replace(TJS_W("%1"), tmp);
                        info.Replace(TJS_W("%2"), type);
                        output->Print(info.c_str());

                        if(i == items.end())
                            break;

                        count = 0;
                    }

                    type = i->Type;
                    count++;
                }
            }
        }

        bool AnyUnfreed() { return Hash.GetCount() != 0; }

        void WriteAllUnfreedObjectsToLog() {
            if(!TJSObjectHashMapLog)
                return;

            tHash::tIterator i;
            for(i = Hash.GetFirst(); !i.IsNull(); i++) {
                TJSStoreLog(liiAdd);
                TJSStoreLog(i.GetKey());
                i.GetValue().StoreLog();
            }
        }

        void ReplayLog() {
            // replay recorded log
            while(true) {
                tTJSObjectHashMapLogItemId id =
                    TJSRestoreLog<tTJSObjectHashMapLogItemId>();
                if(id == liiEnd) // 00 end of the log
                {
                    break;
                } else if(id == liiVersion) // 01 identify log
                                            // structure version
                {
                    tjs_int v = TJSRestoreLog<tjs_int>();
                    if(v != TJSVersionHex)
                        TJS_eTJSError(TJSObjectHashMapLogVersionMismatch);
                } else if(id == liiAdd) // 02 add a object
                {
                    void *object = TJSRestoreLog<void *>();
                    tTJSObjectHashMapRecord rec;
                    rec.RestoreLog();
                    Add(object, rec);
                } else if(id == liiRemove) // 03 remove a object
                {
                    void *object = TJSRestoreLog<void *>();
                    Remove(object);
                } else if(id == liiSetType) // 04 set object type
                                            // information
                {
                    void *object = TJSRestoreLog<void *>();
                    ttstr type = TJSRestoreLog<ttstr>();
                    SetType(object, type);
                } else if(id == liiSetFlag) // 05 set object flag
                {
                    void *object = TJSRestoreLog<void *>();
                    tjs_uint32 flags_to_change = TJSRestoreLog<tjs_uint32>();
                    tjs_uint32 bits = TJSRestoreLog<tjs_uint32>();
                    SetFlag(object, flags_to_change, bits);
                } else {
                    TJS_eTJSError(TJSCurruptedObjectHashMapLog);
                }
            }
        }
    };

    //---------------------------------------------------------------------------
    void TJSAddRefObjectHashMap() { tTJSObjectHashMap::AddRef(); }

    //---------------------------------------------------------------------------
    void TJSReleaseObjectHashMap() { tTJSObjectHashMap::Release(); }

    //---------------------------------------------------------------------------
    void TJSAddObjectHashRecord(void *object) {
        if(!TJSObjectHashMap && !TJSObjectHashMapLog)
            return;

        // create object record and log
        tTJSObjectHashMapRecord rec;
        ttstr hist(TJSGetStackTraceString(
            4, (const tjs_char *)(TJSObjectCreationHistoryDelimiter)));
        if(hist.IsEmpty())
            hist = TJSMapGlobalStringMap(
                (const tjs_char *)TJSCallHistoryIsFromOutOfTJS2Script);
        rec.History = hist;
        ttstr where(TJSGetStackTraceString(1));
        if(where.IsEmpty())
            where = TJSMapGlobalStringMap(
                (const tjs_char *)TJSCallHistoryIsFromOutOfTJS2Script);
        rec.Where = where;
        static ttstr InitialType(TJS_W("unknown type"));
        rec.Type = InitialType;

        if(TJSObjectHashMap) {
            TJSObjectHashMap->Add(object, rec);
        } else if(TJSObjectHashMapLog) {
            TJSStoreLog(liiAdd);
            TJSStoreLog(object);
            rec.StoreLog();
        }
    }

    //---------------------------------------------------------------------------
    void TJSRemoveObjectHashRecord(void *object) {
        if(TJSObjectHashMap) {
            TJSObjectHashMap->Remove(object);
        } else if(TJSObjectHashMapLog) {
            TJSStoreLog(liiRemove);
            TJSStoreLog(object);
        }
    }

    //---------------------------------------------------------------------------
    void TJSObjectHashSetType(void *object, const ttstr &type) {
        if(TJSObjectHashMap) {
            TJSObjectHashMap->SetType(object, type);
        } else if(TJSObjectHashMapLog) {
            TJSStoreLog(liiSetType);
            TJSStoreLog(object);
            TJSStoreLog(type);
        }
    }

    //---------------------------------------------------------------------------
    void TJSSetObjectHashFlag(void *object, tjs_uint32 flags_to_change,
                              tjs_uint32 bits) {
        if(TJSObjectHashMap) {
            TJSObjectHashMap->SetFlag(object, flags_to_change, bits);
        } else if(TJSObjectHashMapLog) {
            TJSStoreLog(liiSetFlag);
            TJSStoreLog(object);
            TJSStoreLog(flags_to_change);
            TJSStoreLog(bits);
        }
    }

    //---------------------------------------------------------------------------
    void TJSReportAllUnfreedObjects(iTJSConsoleOutput *output) {
        if(TJSObjectHashMap)
            TJSObjectHashMap->ReportAllUnfreedObjects(output);
    }

    //---------------------------------------------------------------------------
    bool TJSObjectHashAnyUnfreed() {
        if(TJSObjectHashMap)
            return TJSObjectHashMap->AnyUnfreed();
        return false;
    }

    //---------------------------------------------------------------------------
    void TJSObjectHashMapSetLog(tTJSBinaryStream *stream) {
        // Set log object. The log file object should not freed until
        // the program (the program is the Process, not RTL nor TJS2
        // framework).
        TJSObjectHashMapLog = stream;
        TJSStoreLog(liiVersion);
        TJSStoreLog(TJSVersionHex);
    }

    //---------------------------------------------------------------------------
    void TJSWriteAllUnfreedObjectsToLog() {
        if(TJSObjectHashMap && TJSObjectHashMapLog)
            TJSObjectHashMap->WriteAllUnfreedObjectsToLog();
    }

    //---------------------------------------------------------------------------
    void TJSWarnIfObjectIsDeleting(iTJSConsoleOutput *output, void *object) {
        if(TJSObjectHashMap)
            TJSObjectHashMap->WarnIfObjectIsDeleting(output, object);
    }

    //---------------------------------------------------------------------------
    void TJSReplayObjectHashMapLog() {
        if(TJSObjectHashMap && TJSObjectHashMapLog)
            TJSObjectHashMap->ReplayLog();
    }

    //---------------------------------------------------------------------------
    ttstr TJSGetObjectTypeInfo(void *object) {
        if(TJSObjectHashMap)
            return TJSObjectHashMap->GetType(object);
        return ttstr();
    }

    //---------------------------------------------------------------------------
    tjs_uint32 TJSGetObjectHashCheckFlag(void *object) {
        if(TJSObjectHashMap)
            return TJSObjectHashMap->GetFlag(object);
        return 0;
    }
    //---------------------------------------------------------------------------

    //---------------------------------------------------------------------------
    // StackTracer : stack to trace function call trace
    //---------------------------------------------------------------------------
    class tTJSStackTracer;

    tTJSStackTracer *TJSStackTracer;

    //---------------------------------------------------------------------------
    struct tTJSStackRecord {
        tTJSInterCodeContext *Context;
        const tjs_int *CodeBase;
        tjs_int *const *CodePtr;
        bool InTry;

        tTJSStackRecord(tTJSInterCodeContext *context, bool in_try) {
            CodeBase = nullptr;
            CodePtr = nullptr;
            InTry = in_try;
            Context = context;
            if(Context)
                Context->AddRef();
        }

        ~tTJSStackRecord() {
            if(Context)
                Context->Release();
        }

        tTJSStackRecord(const tTJSStackRecord &rhs) {
            Context = nullptr;
            *this = rhs;
        }

        void operator=(const tTJSStackRecord &rhs) {
            if(Context != rhs.Context) {
                if(Context)
                    Context->Release(), Context = nullptr;
                Context = rhs.Context;
                if(Context)
                    Context->AddRef();
            }
            CodeBase = rhs.CodeBase;
            CodePtr = rhs.CodePtr;
            InTry = rhs.InTry;
        }
    };

    //---------------------------------------------------------------------------
    class tTJSStackTracer {
        std::vector<tTJSStackRecord> Stack;

        tjs_int RefCount;

    public:
        tTJSStackTracer() {
            RefCount = 1;
            TJSStackTracer = this;
        }

    protected:
        ~tTJSStackTracer() { TJSStackTracer = nullptr; }

    protected:
        void _AddRef() { RefCount++; }

        void _Release() {
            if(RefCount == 1)
                delete this;
            else
                RefCount--;
        }

    public:
        static void AddRef() {
            if(TJSStackTracer)
                TJSStackTracer->_AddRef();
            else
                new tTJSStackTracer();
        }

        static void Release() {
            if(TJSStackTracer)
                TJSStackTracer->_Release();
        }

        void Push(tTJSInterCodeContext *context, bool in_try) {
            Stack.push_back(tTJSStackRecord(context, in_try));
        }

        void Pop() { Stack.pop_back(); }

        void SetCodePointer(const tjs_int32 *codebase,
                            tjs_int32 *const *codeptr) {
            tjs_uint size = (tjs_uint)Stack.size();
            if(size < 1)
                return;
            tjs_uint top = size - 1;
            Stack[top].CodeBase = codebase;
            Stack[top].CodePtr = codeptr;
        }

        ttstr GetTraceString(tjs_int limit, const tjs_char *delimiter) {
            // get stack trace string
            if(delimiter == nullptr)
                delimiter = TJS_W(" <-- ");

            ttstr ret;
            tjs_int top = (tjs_int)(Stack.size() - 1);
            while(top >= 0) {
                if(!ret.IsEmpty())
                    ret += delimiter;

                const tTJSStackRecord &rec = Stack[top];
                ttstr str;
                if(rec.CodeBase && rec.CodePtr) {
                    str = rec.Context->GetPositionDescriptionString(
                        (tjs_int)(*rec.CodePtr - rec.CodeBase));
                } else {
                    str = rec.Context->GetPositionDescriptionString(0);
                }

                ret += str;

                // skip try block stack.
                // 'try { } catch' blocks are implemented as
                // sub-functions in a parent function.
                while(top >= 0 && Stack[top].InTry)
                    top--;

                // check limit
                if(limit) {
                    limit--;
                    if(limit <= 0)
                        break;
                }

                top--;
            }

            return ret;
        }
    };

    //---------------------------------------------------------------------------
    void TJSAddRefStackTracer() { tTJSStackTracer::AddRef(); }

    //---------------------------------------------------------------------------
    void TJSReleaseStackTracer() { tTJSStackTracer::Release(); }

    //---------------------------------------------------------------------------
    void TJSStackTracerPush(tTJSInterCodeContext *context, bool in_try) {
        if(TJSStackTracer)
            TJSStackTracer->Push(context, in_try);
    }

    //---------------------------------------------------------------------------
    void TJSStackTracerSetCodePointer(const tjs_int32 *codebase,
                                      tjs_int32 *const *codeptr) {
        if(TJSStackTracer)
            TJSStackTracer->SetCodePointer(codebase, codeptr);
    }

    //---------------------------------------------------------------------------
    void TJSStackTracerPop() {
        if(TJSStackTracer)
            TJSStackTracer->Pop();
    }

    //---------------------------------------------------------------------------
    ttstr TJSGetStackTraceString(tjs_int limit, const tjs_char *delimiter) {
        if(TJSStackTracer)
            return TJSStackTracer->GetTraceString(limit, delimiter);
        else
            return ttstr();
    }

} // namespace TJS
