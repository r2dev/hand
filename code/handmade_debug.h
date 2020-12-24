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
    char* FunctionName;
    
    u32 LineNumber;
    debug_counter_snapshot Snapshots[DEBUG_SNAPSHOT_COUNT];
};

struct debug_state {
    u32 SnapIndex;
    u32 CounterCount;
    debug_counter_state CounterStates[512];
    debug_frame_end_info FrameEndInfos[DEBUG_SNAPSHOT_COUNT];
};

struct render_group;
struct game_assets;

global_variable render_group *DEBUGRenderGroup;

internal void DEBUGReset(game_assets *Assets, u32 Width, u32 Height);
internal void DEBUGOverlay(game_memory* Memory);


#define TIMED_BLOCK__(Number, ...) timed_block TimeBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__);

#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)



#endif //HANDMADE_DEBUG_H
