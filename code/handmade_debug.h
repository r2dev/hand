/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H


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

struct debug_state {
    b32 IsInitialized;
    platform_work_queue* HighPriorityQueue;
    memory_arena DebugArena;
    memory_arena CollateArena;
    temporary_memory CollateTemp;
    
    u32 FrameCount;
    u32 FrameBarLaneCount;
    r32 FrameBarScale;
    
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
    
    rectangle2 ProfileRect;
};

internal void DEBUGEnd(game_input* Input, loaded_bitmap* DrawBuffer);
internal void DEBUGStart(game_assets *Assets, u32 Width, u32 Height);
internal void RefreshCollation(debug_state* DebugState);
#endif //HANDMADE_DEBUG_H
