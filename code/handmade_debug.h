/* date = December 20th 2020 1:51 am */

#ifndef HANDMADE_DEBUG_H
#define HANDMADE_DEBUG_H

struct debug_record
{
    char* FileName;
    char* FunctionName;
    
    u32 Reserved;
    u32 LineNumber;
    
    u64 HitCount_CycleCount;
    
};

debug_record DebugRecordArray[];

struct timed_block
{
    
    debug_record* Record;
    u64 StartCycles;
    u32 HitCount;
    timed_block(int Counter, char *FileName, int LineNumber, char* FunctionName, int HitCountInit = 1)
    {
        Record = DebugRecordArray + Counter;
        Record->FileName = FileName;
        Record->FunctionName = FunctionName;
        Record->LineNumber = LineNumber;
        HitCount = HitCountInit;
        StartCycles = __rdtsc();
    }
    
    ~timed_block()
    {
        u64 Delta = (u64)((__rdtsc() - StartCycles) | ((u64)HitCount << 32));
        AtomicAddU64(&Record->HitCount_CycleCount, Delta);
    }
};

struct debug_counter_snapshot {
    u32 HitCount;
    u32 CycleCount;
};

#define DEBUG_SNAPSHOT_COUNT 120
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
