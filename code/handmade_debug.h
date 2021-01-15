/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H
#define DEBUG_MAX_VARIABLE_STACK_SIZE 64

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

struct debug_stored_event {
    union {
        debug_stored_event *Next;
        debug_stored_event *NextFree;
    };
    debug_event Event;
    u32 FrameIndex;
};

struct debug_element {
    char *GUID;
    debug_element *NextInHash;
    debug_stored_event *OldestEvent;
    debug_stored_event *LatestEvent;
};

struct debug_variable_link {
    debug_variable_link *Next;
    debug_variable_link *Prev;
    debug_variable_group *Children;
    debug_element *Element;
};

struct debug_variable_group {
    u32 NameLength;
    char *Name;
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
    u32 FrameIndex;
    union {
        debug_frame *Next;
        debug_frame *NextFree;
    };
    
    r32 FrameBarScale;
};

struct open_debug_block {
    u32 StartingFrameIndex;
    debug_event *Event;
    debug_element *Element;
    union {
        open_debug_block *Parent;
        open_debug_block *NextFree;
    };
    
    // only data block
    debug_variable_group *Group;
};

struct debug_thread {
    u32 ID;
    u32 LaneIndex;
    open_debug_block *FirstOpenCodeBlock;
    open_debug_block *FirstOpenDataBlock;
    union {
        debug_thread *Next;
        debug_thread *NextFree;
    };
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
    DebugInteraction_ToggleExpansion,
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
    memory_arena PerFrameArena;
    
    b32 Compiling;
    debug_executing_process Compiler;
    
    u32 FrameBarLaneCount;
    debug_element *ElementHash[1024];
    debug_tree TreeSentinal;
    debug_variable_group *RootGroup;
    debug_view* ViewHash[4096];
    
    // wrapping after 2 years
    u32 TotalFrameCount;
    u32 FrameCount;
    debug_frame *OldestFrame;
    debug_frame *LatestFrame;
    
    debug_frame* CollationFrame;
    
    debug_thread *FirstThread;
    debug_thread *FirstFreeThread;
    open_debug_block *FirstFreeBlock;
    
    u32 SelectIDCount;
    debug_id Selected[64];
    
    b32 Paused;
    
    
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
    
    v2 LastMouseP;
    
    v2 HotMenuP;
    
    debug_frame *FirstFreeFrame;
    debug_stored_event *FirstFreeStoredEvent;
};

#endif //HANDMADE_DEBUG_H
