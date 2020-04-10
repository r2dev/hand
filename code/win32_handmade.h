#if !defined(WIN32_HANDMADE_H)
#define WIN32_HANDMADE_H
struct win32_offscreen_buffer {
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};

struct win32_window_dimension {
	int Width;
	int Height;
};

struct win32_sound_output
{
	uint32 RunningSampleIndex;
	int SamplesPerSecond;
	int WavePeriod;
	int BytesPerSample;
	DWORD SecondaryBufferSize;
	DWORD SafetyBytes;
};

struct win32_debug_time_marker {
	DWORD PlayCursor;
	DWORD WriteCursor;
};

struct win32_game_code {
	HMODULE GameCodeDLL;
	FILETIME DLLLastWriteTime;
	game_update_and_render* UpdateAndRender;
	game_get_sound_samples* GetSoundSamples;
	
	bool32 IsValid;
};

struct win32_recorded_input {
	int InputCount;
	game_input* InputStream;
};


#define WIN32_STATE_FILE_NAME_COUNT MAX_PATH

struct win32_replay_buffer {
	void* MemoryBlock;
	char ReplayFileName[WIN32_STATE_FILE_NAME_COUNT];
};
struct win32_state {
	HANDLE RecordingHandle;
	int InputRecordingIndex;

	HANDLE PlaybackHandle;
	int InputPlayingIndex;

	void* GameMemoryBlock;
	uint64 TotalSize;

	win32_replay_buffer ReplayBuffers[4];

	char EXEFileName[WIN32_STATE_FILE_NAME_COUNT];
	char* OnePastLastEXEFileNameSlash;
};


#endif