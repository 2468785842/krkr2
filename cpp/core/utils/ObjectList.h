//---------------------------------------------------------------------------
/*
        TVP2 ( T Visual Presenter 2 )  A script authoring tool
        Copyright (C) 2000 W.Dee <dee@kikyou.info> and contributors

        See details of license at "license.txt"
*/
//---------------------------------------------------------------------------
// Safe Object List
//---------------------------------------------------------------------------
#ifndef ObjectListH
#define ObjectListH

#include "tjs.h"

using namespace TJS;

//---------------------------------------------------------------------------
/*
        Simple implementation of auto array for objects.
        This manages neither object's construction nor destruction.
*/
//---------------------------------------------------------------------------
#define TOBJECTLIST_INC_COUNT 64

template <typename ObjT>
class tVoidObjectList {
private:
    ObjT **Objects;
    ObjT **BackupedObjects;
    // void * * Objects; // array of the object
    // void * * BackupedObjects; // backuped objects
    tjs_int BackupedCount; // item count in BackupedObjects
    tjs_int Capacity; // capacity of the array
    tjs_int Count; // actual count
    tjs_int SafeLockCount; // safe lock reference count
    bool Backuped; // whether "Object" is backuped to "BackupedObject"

    void InternalAssign(const tVoidObjectList &ref) {
        // this internal function does not free the array previously
        // allocated. only "Objects" ( not "BackupedObjects" ) is
        // copied.
        if(ref.Count) {
            // Objects = (void **)TJS_malloc(ref.Count * sizeof(void
            // *) );
            Objects = new ObjT *[ref.Count];
            memcpy(Objects, ref.Objects, ref.Count * sizeof(ObjT *));
        } else {
            Objects = nullptr;
        }
        Capacity = Count = ref.Count;
    }

    void Backup() {
        // backup current "Objects" to "BackupedObjects"
        if(Count) {
            // BackupedObjects = (void**)TJS_malloc(Count *
            // sizeof(void *) );
            BackupedObjects = new ObjT *[Count];
            memcpy(BackupedObjects, Objects, Count * sizeof(ObjT *));
        } else {
            BackupedObjects = nullptr;
        }
        BackupedCount = Count;
        Backuped = true;
    }

    void Commit() {
        // commit the current array
        // this simply free BackupedObjects ( and its related things )
        if(BackupedObjects)
            delete[](BackupedObjects);
        BackupedObjects = nullptr;
        BackupedCount = 0;
        Backuped = false;
    }

public:
    tVoidObjectList() {
        Objects = nullptr;
        BackupedObjects = nullptr;
        Capacity = 0;
        Count = 0;
        Backuped = false;
        SafeLockCount = 0;
    }

    tVoidObjectList(const tVoidObjectList &ref) {
        SafeLockCount = 0;
        BackupedObjects = nullptr;
        Backuped = false;
        InternalAssign(ref);
    }

    ~tVoidObjectList() {
        if(Objects)
            delete[] Objects;
        if(BackupedObjects)
            delete[] BackupedObjects;
    }

    void operator=(const tVoidObjectList &ref) { Assign(ref); }

    void Assign(const tVoidObjectList &ref) {
        // note that this function does not change any safe locking
        // effects ( locking count is not changed )
        if(Objects)
            delete[] Objects;
        InternalAssign(ref);
    }

    void SafeLock() {
        // lock array safely
        // safe locking is managed by reference counter;
        if(SafeLockCount == 0)
            Compact(); // this is needed
        SafeLockCount++;
    }

    void SafeUnlock() {
        SafeLockCount--;
        if(SafeLockCount == 0)
            Commit();
    }

    tjs_int GetSafeLockedObjectCount() const {
        // this function only valid in safe locking
        if(Backuped)
            return BackupedCount;
        return Count;
    }

    ObjT *GetSafeLockedObjectAt(tjs_int index) const {
        // this migight contain nullptr pointer
        if(Backuped) {
            return BackupedObjects[index];
        }
        return Objects[index];
    }

    tjs_int GetCount() const {
        // note that if you want to ensure the result is
        // actual object count in the array,
        // call Compact() before GetCount().
        return Count;
    }

    tjs_int GetActualCount() {
        // this does Compact() before returning current Count,
        // ensuring returned count is actual object count in the
        // array.
        Compact();
        return Count;
    }

    void SetCount(tjs_int count) {
        if(SafeLockCount && !Backuped)
            Backup();
        Reserve(count);
        Count = count;
    }

    void Reserve(tjs_int count) {
        if(SafeLockCount && !Backuped)
            Backup();
        if(Capacity < count) {
            Capacity = count + TOBJECTLIST_INC_COUNT;
            if(Capacity) {
                ObjT **tmp = new ObjT *[Capacity];
                if(Count > 0) {
                    memcpy(tmp, Objects, Count * sizeof(ObjT *));
                }
                delete[] Objects;
                Objects = tmp;
                // Objects = (void**)TJS_realloc(Objects, Capacity *
                // sizeof(void
                // *));
            } else if(Objects)
                delete[] Objects, Objects = nullptr;
        }
    }

    void Compact() {
        // this eliminates nullptr pointer from the array
        if(SafeLockCount && !Backuped)
            Backup();
        ObjT **s, **d, **slim;
        slim = Objects + Count;
        s = d = Objects;
        while(s < slim) {
            if(*s == nullptr) {
                s++;
                continue;
            }
            if(s != d)
                *d = *s;
            s++, d++;
        }
        Count = (tjs_int)(d - Objects);
        Capacity = Count;
        if(Count) {
            // Objects = (void**)TJS_realloc(Objects, Count *
            // sizeof(void *));
            ObjT **tmp = new ObjT *[Count];
            memcpy(tmp, Objects, Count * sizeof(ObjT *));
            delete[] Objects;
            Objects = tmp;
        } else if(Objects)
            delete[] Objects, Objects = nullptr;
    }

    ObjT *&operator[](tjs_int index) {
        // this might return a nullptr pointer
        if(SafeLockCount && !Backuped)
            Backup();
        return Objects[index];
    }

    ObjT *const &operator[](tjs_int index) const {
        // this might return a nullptr pointer
        return Objects[index];
    }

    tjs_int Find(ObjT *object) const {
        // find "object" from array
        // return -1 if "object" does not exist
        if(!object)
            return -1; // nullptr cannot be finded
        ObjT *const *s = Objects;
        ObjT *const *slim = Objects + Count;
        while(s < slim) {
            if(*s == object)
                return (tjs_int)(s - Objects);
            s++;
        }
        return -1;
    }

    bool Add(ObjT *object) {
        // add "object" to array
        // this does not allow duplicates
        if(SafeLockCount && !Backuped)
            Backup();
        if(!object)
            return false; // nullptr cannot be added
        if(Find(object) == -1) {
            tjs_int orgcount = Count;
            SetCount(Count + 1);
            Objects[orgcount] = object;
            return true;
        }
        return false;
    }

    bool Remove(ObjT *object) {
        // remove object from array
        // (this only set the pointer to nullptr)
        if(SafeLockCount && !Backuped)
            Backup();
        tjs_int index = Find(object);
        bool ret;
        if(index != -1) {
            Objects[index] = nullptr;
            ret = true;
        } else {
            ret = false;
        }
        if(Backuped && object) {
            // remove also from BackupedObjects
            ObjT **s = BackupedObjects,
                 **slim = BackupedObjects + BackupedCount;
            while(s < slim) {
                if(*s == object) {
                    *s = nullptr;
                    break;
                }
                s++;
            }
        }
        return ret;
    }

    void Remove(tjs_int index) {
        if(SafeLockCount && !Backuped)
            Backup();
        ObjT *object = Objects[index];
        Objects[index] = nullptr;
        if(Backuped && object) {
            // remove also from BackupedObjects
            ObjT **s = BackupedObjects,
                 **slim = BackupedObjects + BackupedCount;
            while(s < slim) {
                if(*s == object) {
                    *s = nullptr;
                    break;
                }
                s++;
            }
        }
    }
};

//---------------------------------------------------------------------------
// template wrappter class for tVoidObjectList
template <typename ObjT>
class tObjectList {
private:
    tVoidObjectList<ObjT> List;

public:
    tObjectList() : List() {}

    tObjectList(const tObjectList<ObjT> &ref) : List(ref.List) {}

    ~tObjectList() {}

    void operator=(const tObjectList<ObjT> &ref) { List.operator=(ref.List); }

    void Assign(const tObjectList<ObjT> &ref) { List.Assign(ref.List); }

    void SafeLock() { List.SafeLock(); }

    void SafeUnlock() { List.SafeUnlock(); }

    tjs_int GetSafeLockedObjectCount() const {
        return List.GetSafeLockedObjectCount();
    }

    ObjT *GetSafeLockedObjectAt(tjs_int index) const {
        return List.GetSafeLockedObjectAt(index);
    }

    tjs_int GetCount() const { return List.GetCount(); }

    tjs_int GetActualCount() { return List.GetActualCount(); }

    void SetCount(tjs_int count) { List.SetCount(count); }

    void Reserve(tjs_int count) { List.Reserve(count); }

    void Compact() { List.Compact(); }

    ObjT *&operator[](tjs_int index) { return List[index]; }

    ObjT *const &operator[](tjs_int index) const { return List[index]; }

    tjs_int Find(ObjT *object) const { return List.Find(object); }

    bool Add(ObjT *object) { return List.Add(object); }

    bool Remove(ObjT *object) { return List.Remove(object); }

    void Remove(tjs_int index) { List.Remove(index); }
};

//---------------------------------------------------------------------------
template <typename ObjT>
class tVoidObjectListSafeLockHolder {
private:
    tVoidObjectList<ObjT> &List;

public:
    tVoidObjectListSafeLockHolder(tVoidObjectList<ObjT> &list) : List(list) {
        List.SafeLock();
    }

    ~tVoidObjectListSafeLockHolder() { List.SafeUnlock(); }
};

//---------------------------------------------------------------------------
template <typename ObjT>
class tObjectListSafeLockHolder {
private:
    tObjectList<ObjT> &List;

public:
    tObjectListSafeLockHolder(tObjectList<ObjT> &list) : List(list) {
        List.SafeLock();
    }

    ~tObjectListSafeLockHolder() { List.SafeUnlock(); }
};
//---------------------------------------------------------------------------

#endif
