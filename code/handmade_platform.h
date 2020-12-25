#if !defined(HANDMADE_PLATFORM_H)
#define HANDMADE_PLATFORM_H

#ifdef __cplusplus
extern "C" {
#endif
    
    
#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif
    
#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif
    
#if !COMPILER_LLVM && !COMPILER_MSVC
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif
    
#if COMPILER_MSVC
#include <intrin.h>
#pragma intrinsic(_BitScanForward)
#endif
    
#include <stdint.h>
#include <float.h>
    
    typedef uint8_t uint8;
    typedef uint16_t uint16;
    typedef uint32_t uint32;
    typedef uint64_t uint64;
    
    typedef int8_t int8;
    typedef int16_t int16;
    typedef int32_t int32;
    typedef int64_t int64;
    typedef int32 bool32;
    
    typedef uintptr_t uintptr;
    typedef intptr_t intptr;
    typedef float real32;
    typedef double real64;
    
    typedef int8 s8;
    typedef int16 s16;
    typedef int32 s32;
    typedef int64 s65;
    typedef bool32 b32;
    
    typedef uint8 u8;
    typedef uint16 u16;
    typedef uint32 u32;
    typedef uint64 u64;
    
    typedef real32 r32;
    typedef real64 r64;
    
    typedef size_t memory_index;
    
#if !defined(internal)
#define internal static
#endif
    
#define local_persist static
#define global_variable static
    
#define Pi32 3.14159265359f
#define Tau32 6.28318530718f
    
#define Real32Maximum FLT_MAX
    
#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) { *(int*)0 = 0; }
#else
#define Assert(Expression)
#endif
    
#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break
    
#include <math.h>
    
#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)
    
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))
    
#define Align4(Value) (Value + 3 & ~3);
#define Align8(Value) (Value + 7 & ~7);
#define Align16(Value) (Value + 15 & ~15);
    
    
#if HANDMADE_INTERNAL
    struct debug_read_file_result {
        uint32 ContentsSize;
        void* Contents;
    };
    
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void* Memory)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
    
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* Filename)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
    
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name( char* Filename, uint32 MemorySize, void *Memory)
    typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
    
    extern struct game_memory* DebugGlobalMemory;
    
    
#endif
    
#define BITMAP_BYTE_PER_PIXEL 4
    
    struct game_offscreen_buffer {
        void* Memory;
        int Width;
        int Height;
        int Pitch;
    };
    
    struct game_sound_output_buffer {
        int16* Samples;
        int SampleCount;
        int SamplesPerSecond;
    };
    
    struct game_button_state {
        int HalfTransitionCount;
        bool32 EndedDown;
    };
    
    
    struct game_controller_input {
        bool32 IsConnected;
        bool32 IsAnalog;
        real32 StickAverageX;
        real32 StickAverageY;
        
        union {
            game_button_state Buttons[12];
            struct {
                game_button_state MoveUp;
                game_button_state MoveDown;
                game_button_state MoveLeft;
                game_button_state MoveRight;
                
                game_button_state ActionUp;
                game_button_state ActionDown;
                game_button_state ActionLeft;
                game_button_state ActionRight;
                
                game_button_state LeftShoulder;
                game_button_state RightShoulder;
                
                game_button_state Start;
                game_button_state Back;
            };
        };
        
    };
    
    struct game_input {
        game_button_state MouseBottons[5];
        int32 MouseX, MouseY, MouseZ;
        game_controller_input Controllers[5];
        
        bool32 ExecutableReloaded;
        real32 dtForFrame;
    };
    
    
    typedef struct platform_file_handle {
        b32 NoErrors;
        void *Platform;
    } platform_file_handle;
    
    typedef struct platform_file_group {
        u32 FileCount;
        void* Platform;
    } platform_file_group;
    
    typedef enum platform_file_type {
        PlatformFileType_AssetFile,
        PlatformFileType_SaveGameFile
    } platform_file_type;
    
#define PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(name) platform_file_group name(platform_file_type Type)
    typedef PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(platform_get_all_file_of_type_begin);
    
#define PLATFORM_GET_ALL_FILE_OF_TYPE_END(name) void name(platform_file_group *FileGroup)
    typedef PLATFORM_GET_ALL_FILE_OF_TYPE_END(platform_get_all_file_of_type_end);
    
#define PLATFORM_OPEN_FILE(name) platform_file_handle name(platform_file_group *FileGroup)
    typedef PLATFORM_OPEN_FILE(platform_open_file);
    
#define PLATFORM_READ_DATA_FROM_FILE(name) void name(platform_file_handle *Source, u64 Offset, u64 Size, void* Dest)
    typedef PLATFORM_READ_DATA_FROM_FILE(platform_read_data_from_file);
    
#define PLATFORM_FILE_ERROR(name) void name(platform_file_handle* Handle, char* Msg)
    typedef PLATFORM_FILE_ERROR(platform_file_error);
    
#define PlatformNoFileErrors(Handle) ((Handle)->NoErrors)
    
    
    struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
    typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);
    
    typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
    typedef void platform_complete_all_work(platform_work_queue* Queue);
    
    
#define PLATFORM_ALLOCATE_MEMORY(name) void* name(memory_index Size)
    typedef PLATFORM_ALLOCATE_MEMORY(platform_allocate_memory);
    
#define PLATFORM_DEALLOCATE_MEMORY(name) void name(void* Memory)
    typedef PLATFORM_DEALLOCATE_MEMORY(platform_deallocate_memory);
    
    typedef struct platform_api {
        platform_add_entry* AddEntry;
        platform_complete_all_work* CompleteAllWork;
        
        platform_get_all_file_of_type_begin* GetAllFileOfTypeBegin;
        platform_get_all_file_of_type_end* GetAllFileOfTypeEnd;
        platform_open_file* OpenNextFile;
        platform_read_data_from_file* ReadDataFromFile;
        platform_file_error* FileError;
        
        platform_allocate_memory *AllocateMemory;
        platform_deallocate_memory *DeallocateMemory;
        
        debug_platform_read_entire_file* DEBUGReadEntireFile;
        debug_platform_write_entire_file* DEBUGWriteEntireFile;
        debug_platform_free_file_memory* DEBUGFreeFileMemory;
        
    } platform_api;
    
    
    
    typedef struct game_memory {
        uint64 PermanentStorageSize;
        void* PermanentStorage;
        
        uint64 TransientStorageSize;
        void* TransientStorage;
        
        u64 DebugStorageSize;
        void* DebugStorage;
        
        platform_work_queue* HighPriorityQueue;
        platform_work_queue* LowPriorityQueue;
        
        platform_api PlatformAPI;
        
    } game_memory;
    
    
#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
    typedef GAME_UPDATE_AND_RENDER(game_update_and_render);
    
#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory* Memory, game_sound_output_buffer* SoundBuffer)
    typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);
    
#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
    inline u32 AtomicCompareExchangeUInt32(u32 volatile* Value, u32 New, u32 Expect) {
        u32 Result = _InterlockedCompareExchange((long *)Value, New, Expect);
        return(Result);
    }
    
    inline u64 AtomicExchangeU64(u64 volatile* Value, u64 New) {
        u64 Result = _InterlockedExchange64((__int64 *)Value, New);
        return(Result);
        
    }
    
    inline u32 AtomicAddU32(u32 volatile* Value, u32 Addend) {
        u32 Result = _InterlockedExchangeAdd((long *)Value, Addend);
        return(Result);
    }
    
    inline u64 AtomicAddU64(u64 volatile* Value, u64 Addend) {
        u64 Result = _InterlockedExchangeAdd64((__int64 *)Value, Addend);
        return(Result);
    }
    
    inline u32 GetThreadID() {
        u8* gs = (u8*)__readgsqword(0x30);
        u32 Result = *((u32*)(gs + 0x48));
        return(Result);
    }
    
#define AtomicIncrement(u);
#else
#define CompletePreviousReadsBeforeFutureReads
#define CompletePreviousWritesBeforeFutureWrites
#endif
    
    struct debug_table;
#define DEBUG_FRAME_END(name) debug_table *name(game_memory* Memory)
    typedef DEBUG_FRAME_END(debug_game_frame_end);
    
    inline game_controller_input* GetController(game_input* Input, int unsigned ControllerIndex) {
        Assert(ControllerIndex < ArrayCount(Input->Controllers));
        game_controller_input* Result = &Input->Controllers[ControllerIndex];
        return(Result);
    }
    
    
    inline uint32
        SafeTruncateUInt64(uint64 Value) {
        Assert(Value <= 0xFFFFFFFF);
        uint32 Result = (uint32)Value;
        return(Result);
    }
    
    inline u16
        SafeTruncateToUInt16(uint32 Value) {
        Assert(Value <= 65535);
        Assert(Value >= 0);
        u16 Result = (u16)Value;
        return(Result);
    }
    
    inline s16
        SafeTruncateToInt16(uint32 Value) {
        Assert(Value <= 32767);
        Assert(Value > 0);
        s16 Result = (s16)Value;
        return(Result);
    }
    
    struct debug_record
    {
        char* FileName;
        char* BlockName;
        
        u32 Reserved;
        u32 LineNumber;
        
        // u64 HitCount_CycleCount;
    };
    
    enum debug_event_type {
        DebugEvent_BeginBlock,
        DebugEvent_EndBlock,
        DebugEvent_FrameMarker,
    };
    
    struct debug_event {
        u64 Clock;
        u16 CoreIndex;
        u16 ThreadIndex;
        u16 DebugRecordIndex;
        u8 TranslationUnit;
        u8 Type;
    };
	
#define MAX_DEBUG_EVENT_COUNT (16 * 65536)
#define MAX_DEBUG_RECORD_COUNT (65536)
#define MAX_DEBUG_TRANSLATION_UNIT 3
    
    struct debug_table {
        u64 CurrentEventArrayIndex;
        u64 volatile EventArrayIndex_EventIndex;
        debug_event Events[32][MAX_DEBUG_EVENT_COUNT];
        debug_record Records[MAX_DEBUG_TRANSLATION_UNIT][MAX_DEBUG_RECORD_COUNT];
        u32 RecordCounts[MAX_DEBUG_TRANSLATION_UNIT];
    };
    
    extern debug_table *GlobalDebugTable;
    
    inline void
        RecordDebugEvent(int RecordIndex, debug_event_type EventType) {
        u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1);
        u32 EventIndex = (ArrayIndex_EventIndex & 0xFFFFFFFF);
        Assert(EventIndex < MAX_DEBUG_EVENT_COUNT);
        debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + (ArrayIndex_EventIndex & 0xFFFFFFFF);
        Event->Clock = __rdtsc();
        Event->ThreadIndex = (u16)GetThreadID();
        Event->CoreIndex = 0;
        Event->DebugRecordIndex = (u16)RecordIndex;
        Event->TranslationUnit = TRANSLATION_UNIT_INDEX;
        Event->Type = (u8)EventType;
    }
    
    
#define FRAME_MARKER() { \
int Counter = __COUNTER__; \
RecordDebugEvent(__COUNTER__, DebugEvent_FrameMarker); \
debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
Record->FileName = __FILE__; \
Record->BlockName = "FrameMarker"; \
Record->LineNumber = __LINE__; \
}
    
#define TIMED_BLOCK__(BlockName, Number,  ...) timed_block TimeBlock_##Number(__COUNTER__, __FILE__, __LINE__, BlockName, ## __VA_ARGS__);
    
#define TIMED_BLOCK_(BlockName, Number, ...) TIMED_BLOCK__(BlockName, Number, ## __VA_ARGS__)
    
#define TIMED_BLOCK(BlockName, ...) TIMED_BLOCK_(#BlockName, __LINE__, ## __VA_ARGS__)
    
#define TIMED_FUNCTION(...) TIMED_BLOCK_(__FUNCTION__, ## __VA_ARGS__)
    
    
#define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit) \
{debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
Record->FileName = FileNameInit; \
Record->BlockName = BlockNameInit; \
Record->LineNumber = LineNumberInit; \
RecordDebugEvent(Counter, DebugEvent_BeginBlock);} \
    
#define BEGIN_BLOCK(Name) \
int Counter_##Name = __COUNTER__; \
BEGIN_BLOCK_(Counter_##Name, __FILE__, __LINE__, #Name); \
    
#define END_BLOCK_(Counter) RecordDebugEvent(Counter, DebugEvent_EndBlock);
#define END_BLOCK(Name) END_BLOCK_(Counter_##Name)
    
    
    struct timed_block {
        int Counter;
        timed_block(int CounterInit, char *FileName, int LineNumber, char* BlockName, u32 HitCounterInit = 1)
        {
            Counter = CounterInit;
            BEGIN_BLOCK_(Counter, FileName, LineNumber, BlockName);
        }
        
        ~timed_block()
        {
            END_BLOCK_(Counter);
        }
    };
    
    
#ifdef __cplusplus
}
#endif

#endif
