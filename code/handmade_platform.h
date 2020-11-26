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

typedef size_t memory_index;

#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

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

enum {
	DebugCycleCounter_GameUpdateAndRender,
	DebugCycleCounter_RenderGroupToOutput,
	DebugCycleCounter_DrawRectangleSlowly,
	DebugCycleCounter_FillPixel,
	DebugCycleCounter_TestPixel,
	
	DebugCycleCounter_Count
};


typedef struct debug_cycle_counter {
	uint64 CycleCount;
	uint32 HitCount;
} debug_cycle_counter;

extern struct game_memory* DebugGlobalMemory;

#if _MSC_VER
#define BEGIN_TIMED_BLOCK(ID) uint64 StartCycleCount##ID = __rdtsc();
#define END_TIMED_BLOCK(ID) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount = __rdtsc() - StartCycleCount##ID; ++DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount;
#define END_TIMED_BLOCK_COUNTED(ID, COUNT) DebugGlobalMemory->Counters[DebugCycleCounter_##ID].CycleCount = __rdtsc() - StartCycleCount##ID; DebugGlobalMemory->Counters[DebugCycleCounter_##ID].HitCount += COUNT;
#else
#define BEGIN_TIMED_BLOCK(ID)
#define END_TIMED_BLOCK(ID)
#endif

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

struct platform_work_queue;
#define PLATFORM_WORK_QUEUE_CALLBACK(name) void name(platform_work_queue* Queue, void* Data)
typedef PLATFORM_WORK_QUEUE_CALLBACK(platform_work_queue_callback);

typedef void platform_add_entry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data);
typedef void platform_complete_all_work(platform_work_queue* Queue);

typedef struct game_memory {
	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;

	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
	debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;

	platform_work_queue* HighPriorityQueue;
	platform_work_queue* LowPriorityQueue;
	platform_add_entry* PlatformAddEntry;
	platform_complete_all_work* PlatformCompleteAllWork;

#if HANDMADE_INTERNAL
	debug_cycle_counter Counters[DebugCycleCounter_Count];
#endif
} game_memory;


#define GAME_UPDATE_AND_RENDER(name) void name(game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);

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
	
#ifdef __cplusplus
}
#endif

#endif
