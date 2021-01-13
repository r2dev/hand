/* date = January 7th 2021 11:39 am */

#ifndef ZHA_DATA_STRUCTURE_H
#define ZHA_DATA_STRUCTURE_H


#define DLIST_INSERT(Sentinal, Entry) \
(Entry)->Prev = (Sentinal); \
(Entry)->Next = (Sentinal)->Next; \
(Entry)->Prev->Next = (Entry); \
(Entry)->Next->Prev = (Entry);

#define DLIST_INIT(Sentinal) \
(Sentinal)->Prev = (Sentinal); \
(Sentinal)->Next = (Sentinal);



#define FREELIST_ALLOC(FreeListPointer, Result, AllocationCode) \
Result = FreeListPointer; \
if (!Result) { \
Result = AllocationCode; \
} else { \
FreeListPointer = Result->NextFree; \
}

#define FREELIST_DEALLOC(FreeListPointer, Pointer) \
if (Pointer) { \
Pointer->NextFree = FreeListPointer; \
FreeListPointer = Pointer; \
}



#endif //ZHA_DATA_STRUCTURE_H
