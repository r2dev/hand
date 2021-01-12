/* date = January 12th 2021 11:09 am */

#ifndef HANDMADE_DEBUG_INTERFACE_H
#define HANDMADE_DEBUG_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif
    struct debug_table;
#define DEBUG_FRAME_END(name) debug_table *name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
    typedef DEBUG_FRAME_END(debug_game_frame_end);
    
    struct debug_id {
        void *Value[2];
    };
#if HANDMADE_INTERNAL
    
    enum debug_type {
        DebugType_FrameMarker,
        DebugType_BeginBlock,
        DebugType_EndBlock,
        
        DebugType_R32,
        DebugType_U32,
        DebugType_S32,
        DebugType_V2,
        DebugType_V3,
        DebugType_V4,
        DebugType_B32,
        DebugType_Rectangle2,
        DebugType_Rectangle3,
        DebugType_BitmapID,
        DebugType_SoundID,
        DebugType_FontID,
        
        DebugType_OpenDataBlock,
        DebugType_CloseDataBlock,
        
        //
        DebugType_CounterThreadList,
    };
    
    struct debug_event {
        char* FileName;
        char* BlockName;
        
        u32 LineNumber;
        
        u64 Clock;
        u8 Type;
        
        u16 CoreIndex;
        u16 ThreadID;
        
        union {
            debug_id DebugID;
            b32 Bool32;
            s32 Int32;
            u32 UInt32;
            r32 Real32;
            v2 Vector2;
            v3 Vector3;
            v4 Vector4;
            rectangle2 Rect2;
            rectangle3 Rect3;
            bitmap_id BitmapID;
            sound_id SoundID;
            font_id FontID;
        };
    };
    
#define MAX_DEBUG_EVENT_COUNT (16 * 65536)
#define MAX_DEBUG_EVENT_ARRAY_COUNT 8
#define MAX_DEBUG_THREAD_COUNT 512
    
    struct debug_table {
        u32 CurrentEventArrayIndex;
        u64 volatile EventArrayIndex_EventIndex;
        u32 EventCounts[MAX_DEBUG_EVENT_ARRAY_COUNT];
        debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
    };
    
    extern debug_table *GlobalDebugTable;
#define RecordDebugEvent(EventType, Name) \
u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
u32 EventIndex = (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Assert(EventIndex < MAX_DEBUG_EVENT_COUNT); \
debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Event->Clock = __rdtsc(); \
Event->Type = (u8)EventType; \
Event->CoreIndex = 0; \
Event->ThreadID = (u16)GetThreadID(); \
Event->FileName = __FILE__; \
Event->BlockName = Name; \
Event->LineNumber = __LINE__; \
    
    
#define FRAME_MARKER(SecondsElapsedInit) { \
int Counter = __COUNTER__; \
RecordDebugEvent(DebugType_FrameMarker, "Frame Marker"); \
Event->Real32 = SecondsElapsedInit; \
}
    
#define TIMED_BLOCK__(BlockName, Number,  ...) timed_block TimeBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__);
    
#define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
    
#define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
    
#define TIMED_FUNCTION(...) TIMED_BLOCK_(__FUNCTION__, ## __VA_ARGS__)
    
#define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit) { \
RecordDebugEvent(DebugType_BeginBlock, BlockNameInit);}
    
#define BEGIN_BLOCK(Name) \
int Counter_##Name = __COUNTER__; \
BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name); \
    
#define END_BLOCK_(Counter) { \
RecordDebugEvent(DebugType_EndBlock, "End Block"); \
}
#define END_BLOCK(Name) END_BLOCK_(Counter_##Name)
    
    
    struct timed_block {
        int Counter;
        timed_block(int CounterInit, char *FileName, int LineNumber, char* BlockName, u32 HitCounterInit = 1)
        {
            Counter = CounterInit;
            BEGIN_BLOCK_(Counter, FileName, LineNumber, BlockName);
        }
        
        ~timed_block()
        {
            END_BLOCK_(Counter);
        }
    };
    
#else
#define TIMED_BLOCK__(BlockName, Number,  ...)
#define TIMED_BLOCK_(BlockName, Number, ...)
#define TIMED_BLOCK(BlockName, ...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit)
#define BEGIN_BLOCK(Name)
#define END_BLOCK_(Counter) 
#define END_BLOCK(Name)
#define FRAME_MARKER(SecondsElapsedInit)
#endif
    
#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && defined(HANDMADE_INTERNAL)

inline void
DEBUGValueSetEventData(debug_event *Event, r32 Value) {
    Event->Type = DebugType_R32;
    Event->Real32 = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, s32 Value) {
    Event->Type = DebugType_S32;
    Event->Int32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, u32 Value) {
    Event->Type = DebugType_U32;
    Event->UInt32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v2 Value) {
    Event->Type = DebugType_V2;
    Event->Vector2 = Value;
    
}
inline void
DEBUGValueSetEventData(debug_event *Event, v3 Value) {
    Event->Type = DebugType_V3;
    Event->Vector3 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v4 Value) {
    Event->Type = DebugType_V4;
    Event->Vector4 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle2 Value) {
    Event->Type = DebugType_Rectangle2;
    Event->Rect2 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, bitmap_id Value) {
    Event->Type = DebugType_BitmapID;
    Event->BitmapID = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, sound_id Value) {
    Event->Type = DebugType_SoundID;
    Event->SoundID = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, font_id Value) {
    Event->Type = DebugType_FontID;
    Event->FontID = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle3 Value) {
    Event->Type = DebugType_Rectangle3;
    Event->Rect3 = Value;
}

#define DEBUG_BEGIN_DATA_BLOCK(Name, ID) { \
RecordDebugEvent(DebugType_OpenDataBlock, Name); \
Event->DebugID = ID; \
}

#define DEBUG_END_DATA_BLOCK() { \
RecordDebugEvent(DebugType_CloseDataBlock, "End Data Block"); \
}

#define DEBUG_VALUE(Value) { \
RecordDebugEvent(DebugType_R32, #Value); \
DEBUGValueSetEventData(Event, Value); \
}

#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

inline debug_id
DEBUG_POINTER_ID(void *Pointer) {
    debug_id ID = {Pointer}; 
    return(ID);
} 
#define DEBUG_UI_ENABLED 1
internal void DEBUG_HIT(debug_id ID, r32 ZValue);

internal b32 DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color);

internal b32 DEBUG_REQUESTED(debug_id);
#else
global_variable debug_id NullID = {};

inline debug_id DEBUG_POINTER_ID(void *Pointer) {debug_id NullID = {}; return(NullID);} 
#define DEBUG_BEGIN_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

#define DEBUG_END_DATA_BLOCK(...)
#define DEBUG_UI_ENABLED 0
#define DEBUG_HIT(...)
#define DEBUG_HIGHLIGHTED(...) 0
#define DEBUG_REQUESTED(...)
#endif

#endif //HANDMADE_DEBUG_INTERFACE_H
