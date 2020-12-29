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
    u32 LaneIndex;
};

struct debug_frame {
    u64 BeginClock;
    u64 EndClock;
    
    u32 RegionCount;
    debug_frame_region *Regions;
};

struct open_debug_block {
    u32 StartingFrameIndex;
    debug_event *Event;
    open_debug_block *Parent;
    open_debug_block *NextFree;
};

struct debug_thread {
    u32 ID;
    u32 LaneIndex;
    open_debug_block *FirstOpenBlock;
    debug_thread *Next;
};


struct debug_state {
    b32 IsInitialized;
    memory_arena CollateArena;
    temporary_memory CollateTemp;
    
    u32 FrameCount;
    u32 FrameBarLaneCount;
    r32 FrameBarScale;
    
    debug_frame *Frames;
    debug_thread *FirstThread;
    open_debug_block *FirstFreeBlock;
};

struct render_group;
struct game_assets;

global_variable render_group *DEBUGRenderGroup;

internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height);
internal void DEBUGOverlay(game_memory* Memory);

#endif //HANDMADE_DEBUG_H
