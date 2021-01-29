/* date = January 12th 2021 11:09 am */

#ifndef HANDMADE_DEBUG_INTERFACE_H
#define HANDMADE_DEBUG_INTERFACE_H

#ifdef __cplusplus
extern "C" {
#endif
    struct debug_table;
#define DEBUG_FRAME_END(name) debug_table *name(game_memory* Memory, game_input* Input, game_render_commands *RenderCommands)
    typedef DEBUG_FRAME_END(debug_game_frame_end);
    
    struct debug_id {
        void *Value[2];
    };
#if HANDMADE_INTERNAL
    
    enum debug_type {
        DebugType_Unknown,
        DebugType_FrameMarker,
        
        DebugType_BeginBlock,
        DebugType_EndBlock,
        
        DebugType_r32,
        DebugType_u32,
        DebugType_s32,
        DebugType_v2,
        DebugType_v3,
        DebugType_v4,
        DebugType_b32,
        DebugType_rectangle2,
        DebugType_rectangle3,
        DebugType_bitmap_id,
        DebugType_sound_id,
        DebugType_font_id,
        
        DebugType_OpenDataBlock,
        DebugType_CloseDataBlock,
        
        //
        DebugType_CounterThreadList,
        DebugType_CounterFunctionList,
    };
    
    struct debug_event {
        char* GUID;
        u64 Clock;
        u8 Type;
        
        u16 CoreIndex;
        u16 ThreadID;
        
        union {
            debug_id DebugID;
            debug_event *Value_debug_event;
            b32 Value_b32;
            s32 Value_s32;
            u32 Value_u32;
            r32 Value_r32;
            v2 Value_v2;
            v3 Value_v3;
            v4 Value_v4;
            rectangle2 Value_rectangle2;
            rectangle3 Value_rectangle3;
            bitmap_id Value_bitmap_id;
            sound_id Value_sound_id;
            font_id Value_font_id;
        };
    };
    
    struct debug_table {
        u32 CurrentEventArrayIndex;
        u64 volatile EventArrayIndex_EventIndex;
        //ping pong buffer
        debug_event Events[2][16 * 65536];
    };
    
    extern debug_table *GlobalDebugTable;
    
#define UniqueFileCounterString__(A, B, C, Name) A "|" #B "|" #C "|" Name
#define UniqueFileCounterString_(A, B, C, Name) UniqueFileCounterString__(A, B, C, Name)
#define DEBUG_NAME(Name) UniqueFileCounterString_(__FILE__, __LINE__,  __COUNTER__, Name)
    
#define RecordDebugEvent(EventType, GUIDInit) \
u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
u32 EventIndex = (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Assert(EventIndex < ArrayCount(GlobalDebugTable->Events[0])); \
debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Event->Clock = __rdtsc(); \
Event->Type = (u8)EventType; \
Event->CoreIndex = 0; \
Event->ThreadID = (u16)GetThreadID(); \
Event->GUID = GUIDInit \
    
    
#define FRAME_MARKER(SecondsElapsedInit) { \
RecordDebugEvent(DebugType_FrameMarker, DEBUG_NAME("Frame Marker")); \
Event->Value_r32 = SecondsElapsedInit; \
}
    
#define TIMED_BLOCK__(Name, Number,  ...) timed_block TimeBlock_##Number(Name)
    
#define TIMED_BLOCK_(Name, Number, ...) TIMED_BLOCK__(Name, Number, ## __VA_ARGS__)
    
#define TIMED_BLOCK(Name, ...) TIMED_BLOCK_(DEBUG_NAME(Name), __COUNTER__, ## __VA_ARGS__)
    
#define TIMED_FUNCTION(...) TIMED_BLOCK_(DEBUG_NAME(__FUNCTION__), ## __VA_ARGS__)
    
#define BEGIN_BLOCK_(GUID) {RecordDebugEvent(DebugType_BeginBlock, GUID);}
#define BEGIN_BLOCK(Name) BEGIN_BLOCK_(DEBUG_NAME(Name))
    
#define END_BLOCK_(GUID) {RecordDebugEvent(DebugType_EndBlock, GUID);}
#define END_BLOCK() END_BLOCK_(DEBUG_NAME("End_block"))
    
    
    struct timed_block {
        timed_block(char* GUID)
        {
            BEGIN_BLOCK_(GUID);
        }
        
        ~timed_block()
        {
            END_BLOCK();
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

#if defined(__cplusplus) && HANDMADE_INTERNAL

inline void
DEBUGValueSetEventData(debug_event *Event, r32 Value) {
    Event->Type = DebugType_r32;
    Event->Value_r32 = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, s32 Value) {
    Event->Type = DebugType_s32;
    Event->Value_s32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, u32 Value) {
    Event->Type = DebugType_u32;
    Event->Value_u32 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v2 Value) {
    Event->Type = DebugType_v2;
    Event->Value_v2 = Value;
    
}
inline void
DEBUGValueSetEventData(debug_event *Event, v3 Value) {
    Event->Type = DebugType_v3;
    Event->Value_v3 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, v4 Value) {
    Event->Type = DebugType_v4;
    Event->Value_v4 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle2 Value) {
    Event->Type = DebugType_rectangle2;
    Event->Value_rectangle2 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, rectangle3 Value) {
    Event->Type = DebugType_rectangle3;
    Event->Value_rectangle3 = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, bitmap_id Value) {
    Event->Type = DebugType_bitmap_id;
    Event->Value_bitmap_id = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, sound_id Value) {
    Event->Type = DebugType_sound_id;
    Event->Value_sound_id = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, font_id Value) {
    Event->Type = DebugType_font_id;
    Event->Value_font_id = Value;
}

struct debug_data_block {
    debug_data_block(char *Name) {
        RecordDebugEvent(DebugType_OpenDataBlock, Name);
        // TODO(NAME): 
        //Event->DebugID = ID;
    }
    
    ~debug_data_block() {
        RecordDebugEvent(DebugType_CloseDataBlock, DEBUG_NAME("End Data Block"));
    }
};
#define DEBUG_DATA_BLOCK__(Name, Number) debug_data_block DataBlock__##Number(Name)
#define DEBUG_DATA_BLOCK_(Name, Number) DEBUG_DATA_BLOCK__(Name, Number)
#define DEBUG_DATA_BLOCK(Name) DEBUG_DATA_BLOCK_(DEBUG_NAME(Name), __LINE__)

#define DEBUG_VALUE(Value) { \
RecordDebugEvent(DebugType_Unknown, DEBUG_NAME(#Value)); \
DEBUGValueSetEventData(Event, Value); \
}

#define DEBUG_PROFILE(FunctionName) { \
RecordDebugEvent(DebugType_CounterFunctionList, DEBUG_NAME(#FunctionName)); \
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
inline debug_id DEBUG_POINTER_ID(void *Pointer) {debug_id NullID = {}; return(NullID);} 
#define DEBUG_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

#define DEBUG_UI_ENABLED 0
#define DEBUG_HIT(...)
#define DEBUG_HIGHLIGHTED(...) 0
#define DEBUG_REQUESTED(...) 0

#endif
#endif //HANDMADE_DEBUG_INTERFACE_H
