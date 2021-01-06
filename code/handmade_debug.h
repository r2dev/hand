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
    
    DebugVariableType_CounterThreadList,
    DebugVariableType_ProfileSetting,
    DebugVariableType_BitmapDiplay,
};

inline b32
DEBUGShouldBeWritten(debug_variable_type Type) {
    b32 Result = (Type != DebugVariableType_CounterThreadList && Type != DebugVariableType_BitmapDiplay);
    return(Result);
}

struct debug_variable;
struct debug_variable_reference;

struct debug_variable_group {
    b32 Expanded;
    debug_variable_reference* FirstChild;
    debug_variable_reference* LastChild;
};

struct debug_variable_hierarchy {
    debug_variable_reference* Group;
    debug_variable_hierarchy* Next;
    debug_variable_hierarchy* Prev;
    v2 UIP;
};

struct debug_profile_setting {
    v2 Dimension;
};

struct debug_variable_reference {
    debug_variable *Var;
    debug_variable_reference* Next;
    debug_variable_reference* Parent;
};

struct debug_bitmap_display {
    bitmap_id ID;
    v2 Dim;
    b32 Alpha;
};

struct debug_variable {
    debug_variable_type Type;
    char* Name;
    union {
        b32 Bool32;
        s32 Int32;
        u32 UInt32;
        r32 Real32;
        v2 Vector2;
        v3 Vector3;
        v4 Vector4;
        debug_variable_group Group;
        debug_profile_setting ProfileSetting;
        debug_bitmap_display BitmapDisplay;
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

enum debug_interaction_type {
    DebugInteraction_None,
    DebugInteraction_NOP,
    DebugInteraction_AutoModifyVariable,
    DebugInteraction_DragValue,
    DebugInteraction_ToggleValue,
    DebugInteraction_TearValue,
    DebugInteraction_Resize,
    DebugInteraction_Move,
};

struct debug_interaction {
    debug_interaction_type Type;
    union {
        void* Generic;
        debug_variable *Var;
        debug_variable_hierarchy *Hierarchy;
        v2 *P;
    };
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
    r32 RightEdge;
    r32 FontScale;
    font_id FontID;
    r32 GlobalWidth;
    r32 GlobalHeight;
    render_group *RenderGroup;
    
    loaded_font *Font;
    hha_font *FontInfo;
    
    debug_interaction Interaction;
    debug_interaction HotInteraction;
    debug_interaction NextHotInteraction;
    
    debug_variable_hierarchy HierarchySentinal;
    debug_variable_reference *RootGroup;
    
    v2 LastMouseP;
    
    v2 HotMenuP;
};

#endif //HANDMADE_DEBUG_H
