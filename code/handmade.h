#if !defined(HANDMADE_H)
#define HANDMADE_H
#if HANDMADE_SLOW
#define Assert(Expression) if(!(Expression)) { *(int*)0 = 0; }
#else
#define Assert(Expression)
#endif

#include <stdint.h>
#include <math.h>
#define internal static
#define local_persist static
#define global_variable static

#define Pi32 3.14159265359f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;
typedef int32 bool32;

typedef float real32;
typedef double real64;

inline uint32
SafeTruncateUInt64(uint64 Value) {
	Assert(Value <= 0xFFFFFFFF);
	uint32 Result = (uint32)Value;
	return(Result);
}

#define Kilobytes(Value) ((Value)*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define Terabytes(Value) (Gigabytes(Value)*1024)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))


struct thread_context {
	int Placeholder;
};


#if HANDMADE_INTERNAL
struct debug_read_file_result {
	uint32 ContentsSize;
	void* Contents;
};

#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(thread_context *Thread, void* Memory)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platform_free_file_memory);

#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) debug_read_file_result name(thread_context *Thread, char* Filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platform_read_entire_file);

#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) bool32 name(thread_context *Thread, char* Filename, uint32 MemorySize, void *Memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platform_write_entire_file);

#endif


struct game_offscreen_buffer {
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
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
	real32 dtForFrame;
};

inline game_controller_input* GetController(game_input* Input, int unsigned ControllerIndex) {
	Assert(ControllerIndex < ArrayCount(Input->Controllers));
	game_controller_input* Result = &Input->Controllers[ControllerIndex];
	return(Result);
}
struct canonical_position {
#if 1
	int32 TileMapX;
	int32 TileMapY;

	int32 TileX;
	int32 TileY;
#else
	uint32 _TileX;
	uint32 _TileY;
#endif
	real32 TileRelX;
	real32 TileRelY;
};

struct game_state {
	canonical_position PlayerP;
};

struct tile_map
{
	uint32* Tiles;
};

struct world
{
	real32 TileSideInMeters;
	int32 TileSideInPixels;
	real32 MetersToPixels;

	int32 CountX;
	int32 CountY;
	real32 UpperLeftX;
	real32 UpperLeftY;

	tile_map* TileMaps;
	int32 TileMapCountX;
	int32 TileMapCountY;
};

struct game_memory {
	bool32 IsInitialized;

	uint64 PermanentStorageSize;
	void* PermanentStorage;

	uint64 TransientStorageSize;
	void* TransientStorage;

	debug_platform_read_entire_file* DEBUGPlatformReadEntireFile;
	debug_platform_write_entire_file* DEBUGPlatformWriteEntireFile;
	debug_platform_free_file_memory* DEBUGPlatformFreeFileMemory;
};

#define GAME_UPDATE_AND_RENDER(name) void name(thread_context *Thread, game_memory* Memory, game_input* Input, game_offscreen_buffer* Buffer)
typedef GAME_UPDATE_AND_RENDER(game_update_and_render);

#define GAME_GET_SOUND_SAMPLES(name) void name(thread_context *Thread, game_memory* Memory, game_sound_output_buffer* SoundBuffer)
typedef GAME_GET_SOUND_SAMPLES(game_get_sound_samples);



#endif
