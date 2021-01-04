/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H

enum debug_variable_type {
    DebugVariableType_Bool32,
    DebugVariableType_Int32,
    DebugVariableType_UInt32,
    DebugVariableType_Real32,
    DebugVariableType_V2,
    DebugVariableType_V3,
    DebugVariableType_V4,
    DebugVariableType_Group,
};

struct debug_variable;
struct debug_variable_group {
    b32 Expanded;
    debug_variable* FirstChild;
    debug_variable* LastChild;
};
struct debug_variable_hierarchy {
    debug_variable* Group;
    v2 UIP;
};

struct debug_variable {
    debug_variable_type Type;
    char* Name;
    debug_variable* Next;
    debug_variable* Parent;
    
    union {
        b32 Bool32;
        s32 Int32;
        u32 UInt32;
        r32 Real32;
        v2 Vector2;
        v3 Vector3;
        v4 Vector4;
        debug_variable_group Group;
    };
};

enum debug_text_op {
    DEBUGTextOp_DrawText,
    DEBUGTextOp_SizeText,
};

struct debug_counter_snapshot {
    u32 HitCount;
    u64 CycleCount;
};

struct debug_counter_state {
    char* FileName;
    char* BlockName;
    
    u32 LineNumber;
};
// can be improve with chaining
#define MAX_REGIONS_PER_FRAME 1024
struct debug_frame_region {
    r32 MinT;
    r32 MaxT;
    u16 LaneIndex;
    u16 ColorIndex;
    debug_record* Record;
    
    u64 CycleCount;
};

struct debug_frame {
    u64 BeginClock;
    u64 EndClock;
    r32 WallSecondsElapsed;
    u32 RegionCount;
    debug_frame_region *Regions;
    
};

struct open_debug_block {
    u32 StartingFrameIndex;
    debug_event *Event;
    open_debug_block *Parent;
    open_debug_block *NextFree;
    debug_record* Source;
};

struct debug_thread {
    u32 ID;
    u32 LaneIndex;
    open_debug_block *FirstOpenBlock;
    debug_thread *Next;
};

struct render_group;
struct game_assets;
struct loaded_bitmap;
struct loaded_font;

enum debug_interaction {
    DebugInteraction_None,
    DebugInteraction_NOP,
    DebugInteraction_DragValue,
    DebugInteraction_ToggleValue,
    DebugInteraction_TearValue,
};

struct debug_state {
    b32 IsInitialized;
    platform_work_queue* HighPriorityQueue;
    memory_arena DebugArena;
    memory_arena CollateArena;
    temporary_memory CollateTemp;
    
    u32 FrameCount;
    u32 FrameBarLaneCount;
    r32 FrameBarScale;
    b32 ProfileOn;
    
    b32 Compiling;
    debug_executing_process Compiler;
    
    debug_frame *Frames;
    debug_thread *FirstThread;
    open_debug_block *FirstFreeBlock;
    
    b32 Paused;
    u32 CollationArrayIndex;
    debug_frame* CollationFrame;
    
    debug_record* RecordToScope;
    
    r32 AtY;
    r32 LeftEdge;
    r32 FontScale;
    font_id FontID;
    r32 GlobalWidth;
    r32 GlobalHeight;
    render_group *RenderGroup;
    
    loaded_font *Font;
    hha_font *FontInfo;
    
    rectangle2 ProfileRect;
    
    debug_variable *Hot;
    debug_variable *InteractingWith;
    debug_variable *NextHot;
    debug_interaction Interaction;
    debug_variable_hierarchy Hierarchy;
    debug_variable *RootGroup;
    v2 LastMouseP;
    
    v2 HotMenuP;
};

internal void DEBUGEnd(game_input* Input, loaded_bitmap* DrawBuffer);
internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height);
internal void RefreshCollation(debug_state* DebugState);
#endif //HANDMADE_DEBUG_H
