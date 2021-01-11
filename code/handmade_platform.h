#if !defined(HANDMADE_PLATFORM_H)
#define HANDMADE_PLATFORM_H

#include "zha_config.h"

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
        u32 ContentsSize;
        void* Contents;
    };
    
    typedef struct debug_executing_process {
        u64 OSHandle;
    } debug_executing_process;
    
    typedef struct debug_executing_state {
        b32 StartedSuccessfully;
        b32 IsRunning;
        s32 ReturnCode;
    } debug_executing_state;
    
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(void* Memory)
    typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);
    
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(char* Filename)
    typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);
    
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(char* Filename, u32 MemorySize, void *Memory)
    typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);
    
#define DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(name) debug_executing_process name(char* Path, char* Command, char* CommandLine)
    typedef DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(debug_platform_execute_system_command);
    
#define DEBUG_PLATFORM_GET_PROCESS_STATE(name) debug_executing_state name(debug_executing_process Process)
    typedef DEBUG_PLATFORM_GET_PROCESS_STATE(debug_platform_get_process_state);
    
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
        b32 EndedDown;
    };
    
    
    struct game_controller_input {
        b32 IsConnected;
        b32 IsAnalog;
        r32 StickAverageX;
        r32 StickAverageY;
        
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
    
    enum game_input_mouse_button {
        PlatformMouseButton_Left,
        PlatformMouseButton_Middle,
        PlatformMouseButton_Right,
        PlatformMouseButton_Extended0,
        PlatformMouseButton_Extended1,
        PlatformMouseButton_Count,
    };
    
    struct game_input {
        game_button_state MouseButtons[PlatformMouseButton_Count];
        r32 MouseX, MouseY, MouseZ;
        game_controller_input Controllers[5];
        
        r32 dtForFrame;
    };
    
    inline b32 WasPressed(game_button_state *State) {
        b32 Result = ((State->HalfTransitionCount > 1) ||
                      (State->HalfTransitionCount == 1 && State->EndedDown));
        return(Result);
    }
    
    
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
        
#if HANDMADE_INTERNAL
        debug_platform_read_entire_file* DEBUGReadEntireFile;
        debug_platform_write_entire_file* DEBUGWriteEntireFile;
        debug_platform_free_file_memory* DEBUGFreeFileMemory;
        debug_platform_execute_system_command* DEBUGExecuteSystemCommand;
        debug_platform_get_process_state* DEBUGGetProcessState;
#endif
    } platform_api;
    
    typedef struct game_memory {
        u64 PermanentStorageSize;
        void* PermanentStorage;
        
        uint64 TransientStorageSize;
        void* TransientStorage;
        
        u64 DebugStorageSize;
        void* DebugStorage;
        
        platform_work_queue* HighPriorityQueue;
        platform_work_queue* LowPriorityQueue;
        
        platform_api PlatformAPI;
        b32 ExecutableReloaded;
        
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
#define DEBUG_FRAME_END(name) debug_table *name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
    typedef DEBUG_FRAME_END(debug_game_frame_end);
    
    inline game_controller_input* GetController(game_input* Input, int unsigned ControllerIndex) {
        Assert(ControllerIndex < ArrayCount(Input->Controllers));
        game_controller_input* Result = &Input->Controllers[ControllerIndex];
        return(Result);
    }
    
    
    inline u32
        SafeTruncateUInt64(uint64 Value) {
        Assert(Value <= 0xFFFFFFFF);
        u32 Result = (u32)Value;
        return(Result);
    }
    
    inline u16
        SafeTruncateToUInt16(u32 Value) {
        Assert(Value <= 65535);
        Assert(Value >= 0);
        u16 Result = (u16)Value;
        return(Result);
    }
    
    inline s16
        SafeTruncateToInt16(u32 Value) {
        Assert(Value <= 32767);
        Assert(Value > 0);
        s16 Result = (s16)Value;
        return(Result);
    }
#if HANDMADE_INTERNAL
    struct debug_record
    {
        char* FileName;
        char* BlockName;
        
        u32 Reserved;
        u32 LineNumber;
        
        // u64 HitCount_CycleCount;
    };
    
    enum debug_event_type {
        DebugEvent_FrameMarker,
        DebugEvent_BeginBlock,
        DebugEvent_EndBlock,
        
        DebugEvent_R32,
        DebugEvent_U32,
        DebugEvent_S32,
        DebugEvent_V2,
        DebugEvent_V3,
        DebugEvent_V4,
        DebugEvent_Rectangle2,
        DebugEvent_Rectangle3,
        
        DebugEvent_OpenDataBlock,
        DebugEvent_CloseDataBlock,
    };
    
    struct threadid_coreindex {
        u16 CoreIndex;
        u16 ThreadID;
    };
    
    struct debug_event {
        u64 Clock;
        
        u16 DebugRecordIndex;
        u8 TranslationUnit;
        u8 Type;
        
        union {
            r32 SecondsElapsed;
            threadid_coreindex TC;
            s32 VecS32[6];
            u32 VecU32[6];
            r32 VecR32[6];
            void* VecPtr[3];
        };
    };
	
#define MAX_DEBUG_EVENT_COUNT (16 * 65536)
#define MAX_DEBUG_RECORD_COUNT (65536)
#define MAX_DEBUG_TRANSLATION_UNIT 3
#define MAX_DEBUG_EVENT_ARRAY_COUNT 8
#define MAX_DEBUG_THREAD_COUNT 512
    
    struct debug_table {
        u32 CurrentEventArrayIndex;
        u64 volatile EventArrayIndex_EventIndex;
        u32 EventCounts[MAX_DEBUG_EVENT_ARRAY_COUNT];
        debug_event Events[MAX_DEBUG_EVENT_ARRAY_COUNT][MAX_DEBUG_EVENT_COUNT];
        
        u32 RecordCounts[MAX_DEBUG_TRANSLATION_UNIT];
        debug_record Records[MAX_DEBUG_TRANSLATION_UNIT][MAX_DEBUG_RECORD_COUNT];
        
    };
    
    extern debug_table *GlobalDebugTable;
#define RecordDebugEventCommon(RecordIndex, EventType) \
u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugTable->EventArrayIndex_EventIndex, 1); \
u32 EventIndex = (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Assert(EventIndex < MAX_DEBUG_EVENT_COUNT); \
debug_event *Event = GlobalDebugTable->Events[ArrayIndex_EventIndex >> 32] + (ArrayIndex_EventIndex & 0xFFFFFFFF); \
Event->Clock = __rdtsc(); \
Event->DebugRecordIndex = (u16)RecordIndex; \
Event->TranslationUnit = TRANSLATION_UNIT_INDEX; \
Event->Type = (u8)EventType; \
    
    
#define RecordDebugEvent(RecordIndex, EventType) { \
RecordDebugEventCommon(RecordIndex, EventType); \
Event->TC.CoreIndex = 0; \
Event->TC.ThreadID = (u16)GetThreadID(); \
}
    
#define FRAME_MARKER(SecondsElapsedInit) { \
int Counter = __COUNTER__; \
RecordDebugEventCommon(Counter, DebugEvent_FrameMarker); \
Event->SecondsElapsed = SecondsElapsedInit; \
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
    
#else
#define TIMED_BLOCK__(BlockName, Number,  ...)
#define TIMED_BLOCK_(BlockName, Number, ...)
#define TIMED_BLOCK(BlockName, ...)
#define TIMED_FUNCTION(...)
#define BEGIN_BLOCK_(Counter, FileNameInit, LineNumberInit, BlockNameInit)
#define BEGIN_BLOCK(Name)
#define END_BLOCK_(Counter) 
#define END_BLOCK(Name)
#define FRAME_MARKER(SecondsElapsedInit)
#endif
    //utility for platform and game
    inline u32
        StringLength(char* Str) {
        u32 Count = 0;
        while (*Str++) {
            ++Count;
        }
        return(Count);
    }
    
#ifdef __cplusplus
}
#endif

#if defined(__cplusplus) && defined(HANDMADE_INTERNAL)



inline void
DEBUGValueSetEventData(debug_event *Event, r32 Value) {
    Event->Type = DebugEvent_R32;
    Event->VecR32[0] = Value;
}
inline void
DEBUGValueSetEventData(debug_event *Event, s32 Value) {
    Event->Type = DebugEvent_S32;
    Event->VecS32[0] = Value;
}

inline void
DEBUGValueSetEventData(debug_event *Event, u32 Value) {
    Event->Type = DebugEvent_U32;
    Event->VecU32[0] = Value;
}

#define DEBUG_BEGIN_DATA_BLOCK(Name, Ptr0, Ptr1) { \
int Counter = __COUNTER__; \
RecordDebugEventCommon(Counter, DebugEvent_OpenDataBlock); \
Event->VecPtr[0] = Ptr0; \
Event->VecPtr[1] = Ptr1; \
debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
Record->FileName = __FILE__; \
Record->LineNumber = __LINE__; \
Record->BlockName = Name; \
}

#define DEBUG_END_DATA_BLOCK() { \
int Counter = __COUNTER__; \
RecordDebugEventCommon(Counter, DebugEvent_CloseDataBlock); \
debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
Record->FileName = __FILE__; \
Record->LineNumber = __LINE__; \
}

#define DEBUG_VALUE(Value) { \
int Counter = __COUNTER__; \
RecordDebugEventCommon(Counter, DebugEvent_R32); \
DEBUGValueSetEventData(Event, Value); \
debug_record* Record = GlobalDebugTable->Records[TRANSLATION_UNIT_INDEX] + Counter; \
Record->FileName = __FILE__; \
Record->LineNumber = __LINE__; \
Record->BlockName = "Value"; \
}

#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)

#else
#define DEBUG_BEGIN_DATA_BLOCK(...)
#define DEBUG_VALUE(...)
#define DEBUG_BEGIN_ARRAY(...)
#define DEBUG_END_ARRAY(...)
#define DEBUG_END_DATA_BLOCK(...)
#endif


#endif
