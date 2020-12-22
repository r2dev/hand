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

struct debug_counter_state {
    char* FileName;
    char* FunctionName;
    
    u32 LineNumber;
    debug_counter_snapshot Snapshots[120];
};

struct debug_state {
    u32 CounterCount;
    debug_counter_state CounterStates[512];
};


#define TIMED_BLOCK__(Number, ...) timed_block TimeBlock_##Number(__COUNTER__, __FILE__, __LINE__, __FUNCTION__, ## __VA_ARGS__);

#define TIMED_BLOCK_(Number, ...) TIMED_BLOCK__(Number, ## __VA_ARGS__)
#define TIMED_BLOCK(...) TIMED_BLOCK_(__LINE__, ## __VA_ARGS__)

#endif //HANDMADE_DEBUG_H
