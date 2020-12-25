/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H

#define DEBUG_SNAPSHOT_COUNT 120

struct debug_counter_snapshot {
    u32 HitCount;
    u64 CycleCount;
};

struct debug_counter_state {
    char* FileName;
    char* BlockName;
    
    u32 LineNumber;
    debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
};

struct debug_state {
    u32 SnapIndex;
    u32 CounterCount;
    debug_counter_state CounterStates[512];
};

struct render_group;
struct game_assets;

global_variable render_group *DEBUGRenderGroup;

internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height);
internal void DEBUGOverlay(game_memory* Memory);

#endif //HANDMADE_DEBUG_H
