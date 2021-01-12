/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H

struct debug_tree;

struct debug_view_inline_block {
    v2 Dim;
};

struct debug_view_collapsible {
    b32 ExpandedAlways;
    b32 ExpandedAltView;
};

enum debug_view_type {
    DebugViewType_Unknown,
    DebugViewType_Basic,
    DebugViewType_InlineBlock,
    DebugViewType_Collapsible,
};

struct debug_view {
    debug_id ID;
    debug_view_type Type;
    debug_view *NextInHash;
    union {
        debug_view_inline_block InlineBlock;
        debug_view_collapsible Collapsible;
    };
};
struct debug_variable_group;

struct debug_tree {
    v2 UIP;
    debug_variable_group* Group;
    debug_tree* Next;
    debug_tree* Prev;
};

struct debug_variable_link {
    debug_variable_link *Next;
    debug_variable_link *Prev;
    debug_variable_group *Children;
    debug_event *Event;
};

struct debug_variable_group {
    debug_variable_link Sentinal;
};

struct debug_variable_array {
    u32 Count;
    debug_event *Events;
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
    debug_event* Event;
    
    u64 CycleCount;
};

struct debug_frame {
    u64 BeginClock;
    u64 EndClock;
    r32 WallSecondsElapsed;
    
    debug_variable_group *RootGroup;
    
    u32 RegionCount;
    debug_frame_region *Regions;
    
};

struct open_debug_block {
    u32 StartingFrameIndex;
    debug_event *Event;
    open_debug_block *Parent;
    open_debug_block *NextFree;
    
    // only data block
    debug_variable_group *Group;
};

struct debug_thread {
    u32 ID;
    u32 LaneIndex;
    open_debug_block *FirstOpenCodeBlock;
    open_debug_block *FirstOpenDataBlock;
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
    DebugInteraction_Select,
};

struct debug_interaction {
    debug_id ID;
    debug_interaction_type Type;
    
    union {
        void* Generic;
        debug_event *Event;
        debug_tree *Tree;
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
    
    u32 SelectIDCount;
    debug_id Selected[64];
    
    b32 Paused;
    u32 CollationArrayIndex;
    debug_frame* CollationFrame;
    
    char* RecordToScope;
    
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
    
    debug_tree TreeSentinal;
    debug_variable_group *RootGroup;
    debug_view* ViewHash[4096];
    
    v2 LastMouseP;
    
    v2 HotMenuP;
};

#endif //HANDMADE_DEBUG_H
