#include "handmade_platform.h"
#include <windows.h>
#include <Xinput.h>
#include <malloc.h>
#include <dsound.h>

#include "win32_handmade.h"

global_variable bool32 GlobalRunning;
global_variable bool32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable bool32 DEBUGGlobalShowCursor;
global_variable WINDOWPLACEMENT GlobalWindowPosition = { sizeof(GlobalWindowPosition) };

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state* XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state* XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

internal void
CatStrings(size_t SourceACount, char* SourceA,
	size_t SourceBCount, char* SourceB,
	size_t DestCount, char* Dest) {
	for (int Index = 0; Index < SourceACount; ++Index) {
		*Dest++ = *SourceA++;
	}
	for (int Index = 0; Index < SourceBCount; ++Index) {
		*Dest++ = *SourceB++;
	}
	*Dest++ = 0;
}

internal void
Win32GetEXEFileName(win32_state* State) {
	DWORD SizeOfFilename = GetModuleFileNameA(0, State->EXEFileName, sizeof(State->EXEFileName));
	State->OnePastLastEXEFileNameSlash = State->EXEFileName;
	for (char* Scan = State->EXEFileName; *Scan; ++Scan) {
		if (*Scan == '\\') {
			State->OnePastLastEXEFileNameSlash = Scan + 1;
		}
	}
}

internal int
StringLength(char* Str) {
	int Count = 0;
	while (*Str++) {
		++Count;
	}
	return(Count);
}

internal void
Win32BuildEXEPathFileName(win32_state* State, char* FileName,
	int DestCount, char* Dest)
{
	CatStrings(State->OnePastLastEXEFileNameSlash - State->EXEFileName, State->EXEFileName,
		StringLength(FileName), FileName,
		DestCount, Dest);
}

#if 0
internal void
Win32DebugDrawVertical(win32_offscreen_buffer* Backbuffer,
	int X, int Top, int Bottom, uint32 Color) {
	uint8* Pixel = (uint8*)Backbuffer->Memory +
		X * Backbuffer->BytesPerPixel +
		Top * Backbuffer->Pitch;
	for (int Y = Top; Y < Bottom; ++Y) {
		*(uint32*)Pixel = Color;
		Pixel += Backbuffer->Pitch;
	}
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer* Backbuffer, win32_sound_output* SoundOutput,
	real32 C, int PadX, int Top, int Bottom, DWORD Value, uint32 Color) {
	Assert(Value < SoundOutput->SecondaryBufferSize);
	real32 XReal32 = (C * (real32)Value);
	int X = PadX + (int)XReal32;
	Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer* Backbuffer, size_t MarkerCount,
	win32_debug_time_marker* Markers, win32_sound_output* SoundOutput, real32 TargetSecondsPerFrame) {
	int PadX = 16;
	int PadY = 16;
	int Top = PadY;
	int Bottom = Backbuffer->Height - PadY;

	real32 C = (real32)(Backbuffer->Width - 2 * PadX) / (real32)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex) {
		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,
			C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,
			C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);
	}
}
#endif
DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory) {
	if (Memory) {
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile) {
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER FileSize;
		if (GetFileSizeEx(FileHandle, &FileSize)) {
			uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents) {
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(BytesRead == FileSize32)) {
					Result.ContentsSize = FileSize32;
				}
				else {
					DEBUGPlatformFreeFileMemory(Thread, Result.Contents);
					Result.Contents = 0;
				}
			}
			else {

			}
		}
		else {

		}
		CloseHandle(FileHandle);
	}
	else {

	}
	return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile) {
	bool32 Result = false;
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (FileHandle != INVALID_HANDLE_VALUE) {
		DWORD BytesWritten;
		if (WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
			Result = (BytesWritten == MemorySize);
		}
		else {

		}
		CloseHandle(FileHandle);
	}
	else {

	}
	return(Result);
}

inline FILETIME
Win32GetLastWriteTime(char* SourceDLLName) {
	FILETIME LastWriteTime = {};
	WIN32_FILE_ATTRIBUTE_DATA Data;
	if (GetFileAttributesEx(SourceDLLName, GetFileExInfoStandard, &Data)) {
		LastWriteTime = Data.ftLastWriteTime;
	}
	return(LastWriteTime);
}

internal win32_game_code
Win32LoadGameCode(char* SourceDLLName, char* TempDLLName, char* LockFileName) {
	win32_game_code Result = {};
	WIN32_FILE_ATTRIBUTE_DATA Ignored;
	if (!GetFileAttributesEx(LockFileName, GetFileExInfoStandard, &Ignored)) {
		Result.DLLLastWriteTime = Win32GetLastWriteTime(SourceDLLName);
		CopyFile(SourceDLLName, TempDLLName, FALSE);

		Result.GameCodeDLL = LoadLibraryA(TempDLLName);
		if (Result.GameCodeDLL) {
			Result.UpdateAndRender = (game_update_and_render*)
				GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
			Result.GetSoundSamples = (game_get_sound_samples*)
				GetProcAddress(Result.GameCodeDLL, "GameGetSoundSamples");

			Result.IsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
		}
	}
	if (!Result.IsValid) {
		Result.UpdateAndRender = 0;
		Result.GetSoundSamples = 0;
	}
	return(Result);
}

internal void
Win32GetInputFileLocation(win32_state* State, bool32 InputStream, int SlotIndex, int DestCount, char* Dest) {
	char Temp[64];
	wsprintf(Temp, "loop_edit_%d_%s.hmi", SlotIndex, InputStream ? "input" : "state");
	Win32BuildEXEPathFileName(State, Temp, DestCount, Dest);
}

internal void
Win32UnloadGameCode(win32_game_code* GameCode) {
	if (GameCode->GameCodeDLL) {
		FreeLibrary(GameCode->GameCodeDLL);
		GameCode->GameCodeDLL = 0;
	}
	GameCode->IsValid = false;
	GameCode->UpdateAndRender = 0;
	GameCode->GetSoundSamples = 0;
}

internal win32_replay_buffer*
Win32GetReplayBuffer(win32_state* State, int unsigned Index) {
	Assert(Index < ArrayCount(State->ReplayBuffers));
	win32_replay_buffer* Result = &State->ReplayBuffers[Index];
	return(Result);
}

internal void
Win32BeginRecordingInput(win32_state* State, int InputRecordingIndex) {
	win32_replay_buffer* ReplayBuffer = Win32GetReplayBuffer(State, InputRecordingIndex);
	if (ReplayBuffer->MemoryBlock) {

		State->InputRecordingIndex = InputRecordingIndex;

		char Filename[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputRecordingIndex, sizeof(Filename), Filename);
		State->RecordingHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->RecordingHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(ReplayBuffer->MemoryBlock, State->GameMemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndRecordingInput(win32_state* State) {
	CloseHandle(State->RecordingHandle);
	State->InputRecordingIndex = 0;
}

internal void
Win32BeginInputPlayBack(win32_state* State, int InputPlayingIndex) {
	win32_replay_buffer* ReplayBuffer = Win32GetReplayBuffer(State, InputPlayingIndex);
	if (ReplayBuffer->MemoryBlock) {
		State->InputPlayingIndex = InputPlayingIndex;
		char Filename[WIN32_STATE_FILE_NAME_COUNT];
		Win32GetInputFileLocation(State, true, InputPlayingIndex, sizeof(Filename), Filename);
		//@todo check share here
		State->PlaybackHandle = CreateFileA(Filename, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);

		//DWORD BytesRead;
		//ReadFile(State->PlaybackHandle, State->GameMemoryBlock, BytesToRead, &BytesRead, 0);
#if 0
		LARGE_INTEGER FilePosition;
		FilePosition.QuadPart = State->TotalSize;
		SetFilePointerEx(State->PlaybackHandle, FilePosition, 0, FILE_BEGIN);
#endif
		CopyMemory(State->GameMemoryBlock, ReplayBuffer->MemoryBlock, State->TotalSize);
	}
}

internal void
Win32EndInputPlayBack(win32_state* State) {
	CloseHandle(State->PlaybackHandle);
	State->InputPlayingIndex = 0;
}


internal void
Win32RecordInput(win32_state* State, game_input* Input) {
	DWORD BytesWritten;
	WriteFile(State->RecordingHandle, Input, sizeof(*Input), &BytesWritten, 0);
}
internal void
Win32PlayBackInput(win32_state* State, game_input* Input) {
	DWORD BytesRead = 0;
	if (ReadFile(State->PlaybackHandle, Input, sizeof(*Input), &BytesRead, 0)) {
		if (BytesRead == 0) {
			int PlayingIndex = State->InputPlayingIndex;
			Win32EndInputPlayBack(State);
			Win32BeginInputPlayBack(State, PlayingIndex);
		}
	}
}



internal void
Win32LoadXInput() {
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary) {
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}
	if (!XInputLibrary) {
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibrary) {
		XInputGetState = (x_input_get_state*)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state*)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
	if (DSoundLibrary) {
		direct_sound_create* DirectSoundCreate = (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if (DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			if (SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
					HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
					if (SUCCEEDED(Error)) {
						OutputDebugString("Hello");
					}
					else {

					}
				}
			}
			else {

			}
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0))) {
				OutputDebugString("Hello");
			}

		}
		else {

		}
	}
}

internal win32_window_dimension
Win32GetWindowDimension(HWND Window) {
	win32_window_dimension Result;
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;
	return(Result);
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer* Buffer, int Width, int Height) {
	if (Buffer->Memory) {
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	Buffer->Width = Width;
	Buffer->Height = Height;

	Buffer->BytesPerPixel = 4;

	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;

	int BitmapMemorySize = (Buffer->Width * Buffer->Height) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
	Buffer->Pitch = Width * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferInWindow(
	win32_offscreen_buffer* Buffer,
	HDC DeviceContext, int WindowWidth, int WindowHeight) {
	
	if (WindowWidth >= Buffer->Width * 2 && WindowHeight >= Buffer->Height * 2) {
		StretchDIBits(DeviceContext,
			0, 0, Buffer->Width * 2, Buffer->Height * 2,
			0, 0, Buffer->Width, Buffer->Height,
			Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
	}
	else {
		int OffsetX = 10;
		int OffsetY = 10;
		PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
		PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
		PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

		StretchDIBits(DeviceContext,
			OffsetX, OffsetY, Buffer->Width, Buffer->Height,
			0, 0, Buffer->Width, Buffer->Height,
			Buffer->Memory, &Buffer->Info, DIB_RGB_COLORS, SRCCOPY);
	}
}

internal void
Win32ClearBuffer(win32_sound_output* SoundOutput) {
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(0, SoundOutput->SecondaryBufferSize,
		&Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		uint8* DestSample = (uint8*)Region1;
		for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
			*DestSample++ = 0;
		}
		DestSample = (uint8*)Region2;
		for (DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}


internal void
Win32FillSoundBuffer(win32_sound_output* SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer* SourceBuffer) {
	VOID* Region1;
	DWORD Region1Size;
	VOID* Region2;
	DWORD Region2Size;
	if (SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
		&Region1, &Region1Size, &Region2, &Region2Size, 0))) {
		DWORD Region1SampleCount = Region1Size / SoundOutput->BytesPerSample;
		int16* DestSample = (int16*)Region1;
		int16* SourceSample = SourceBuffer->Samples;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
		DestSample = (int16*)Region2;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {

			*DestSample++ = *SourceSample++;
			*DestSample++ = *SourceSample++;
			++SoundOutput->RunningSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

internal real32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold) {
	real32 Result = 0;
	if (Value < -DeadZoneThreshold)
	{
		Result = (real32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
	}
	else if (Value > DeadZoneThreshold)
	{
		Result = (real32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
	}
	return(Result);
}

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state* OldState,
	DWORD ButtonBit, game_button_state* NewState) {
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount =
		(OldState->EndedDown != NewState->EndedDown) ? 1 : 0;

}

internal void
Win32ProcessKeyboardMessage(game_button_state* NewState, bool32 IsDown) {
	if (NewState->EndedDown != IsDown) {
		NewState->EndedDown = IsDown;
		++NewState->HalfTransitionCount;
	}
}

internal void
ToggleFullScreen(HWND Window) {
	DWORD Style = GetWindowLong(Window, GWL_STYLE);
	if (Style & WS_OVERLAPPEDWINDOW) {
		MONITORINFO MonitorInfo = { sizeof(MonitorInfo) };
		if (GetWindowPlacement(Window, &GlobalWindowPosition) &&
			GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo)) {
			SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
			SetWindowPos(Window, HWND_TOP,
				MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
				MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
				MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
	}
	else {
		SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
		SetWindowPlacement(Window, &GlobalWindowPosition);
		SetWindowPos(Window, NULL, 0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
}

internal void
Win32ProcessPendingMessages(win32_state* State, game_controller_input* KeyboardController) {
	MSG Message;
	while (PeekMessageA(&Message, 0, 0, 0, PM_REMOVE)) {
		if (Message.message == WM_QUIT) {
			GlobalRunning = false;
		}
		switch (Message.message) {
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYUP:
		case WM_KEYDOWN: {
			uint32 VKCode = (uint32)Message.wParam;
			bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
			bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
			if (WasDown != IsDown) {
				if (VKCode == VK_UP) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
				}
				else if (VKCode == VK_DOWN) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
				}
				else if (VKCode == VK_LEFT) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
				}
				else if (VKCode == VK_RIGHT) {
					Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
				}
				else if (VKCode == 'W') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
				}
				else if (VKCode == 'S') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
				}
				else if (VKCode == 'A') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
				}
				else if (VKCode == 'D') {
					Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
				}
				else if (VKCode == 'Q') {
					Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
				}
				else if (VKCode == 'E') {
					Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
				}
				else if (VKCode == VK_SPACE) {
					Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
				}
				else if (VKCode == VK_ESCAPE) {
					Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
				}
#if HANDMADE_INTERNAL
				else if (VKCode == 'P') {
					if (IsDown) {
						GlobalPause = !GlobalPause;
					}
				}
				else if (VKCode == 'L') {
					if (IsDown) {
						if (State->InputPlayingIndex == 0) {
							if (State->InputRecordingIndex == 0) {
								Win32BeginRecordingInput(State, 1);
							}
							else {
								Win32EndRecordingInput(State);
								Win32BeginInputPlayBack(State, 1);
							}
						}
						else {
							Win32EndInputPlayBack(State);
						}
					}
				}

#endif
				if (IsDown) {
					bool32 AltKeyWasDown = (Message.lParam & (1 << 29));
					if ((VKCode == VK_F4) && AltKeyWasDown) {
						GlobalRunning = false;
					}
					if ((VKCode == VK_RETURN) && AltKeyWasDown) {
						if (Message.hwnd) {
							ToggleFullScreen(Message.hwnd);
						}
					}
				}
			}

		} break;
		default: {
			TranslateMessage(&Message);
			DispatchMessageA(&Message);
		} break;
		}

	}
}

internal LRESULT CALLBACK
Win32MainWindowCallback(
	HWND   Window,
	UINT   Message,
	WPARAM WParam,
	LPARAM LParam
	) {
	LRESULT Result = 0;
	switch (Message) {
	case WM_SIZE: {

	} break;
	case WM_SETCURSOR: {
		if (DEBUGGlobalShowCursor) {
			Result = DefWindowProc(Window, Message, WParam, LParam);
		}
		else {
			SetCursor(0);
		}
	} break;
	case WM_DESTROY: {
		GlobalRunning = false;
	} break;
	case WM_CLOSE: {
		GlobalRunning = false;
	} break;
	case WM_ACTIVATEAPP: {
#if 0
		if (WParam == TRUE) {
			SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 255, LWA_ALPHA);
		}
		else {
			SetLayeredWindowAttributes(Window, RGB(0, 0, 0), 64, LWA_ALPHA);
		}
#endif

	} break;
	case WM_PAINT: {
		PAINTSTRUCT Paint;
		HDC DeviceContext = BeginPaint(Window, &Paint);
		int X = Paint.rcPaint.left;
		int Y = Paint.rcPaint.top;
		int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
		int Width = Paint.rcPaint.right - Paint.rcPaint.left;
		win32_window_dimension Dimension = Win32GetWindowDimension(Window);
		Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
		EndPaint(Window, &Paint);
	} break;
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_KEYUP:
	case WM_KEYDOWN: {
		Assert(!"Keyboard input came in through");
	} break;
	default: {
		Result = DefWindowProc(Window, Message, WParam, LParam);
	} break;
	}
	return(Result);
}



inline LARGE_INTEGER
Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return(Result);
}

inline real32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
	real32 Result = ((real32)(End.QuadPart - Start.QuadPart) / (real32)GlobalPerfCountFrequency);
	return(Result);
}

int CALLBACK WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR     CommandLine,
	int       showCode
	) {
	win32_state Win32State = {};
	LARGE_INTEGER PerfCountFrequencyResult;
	QueryPerformanceFrequency(&PerfCountFrequencyResult);
	GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;


	Win32GetEXEFileName(&Win32State);
	char SourceGameCodeDLLFilename[] = "handmade.dll";
	char SourceGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, SourceGameCodeDLLFilename,
		sizeof(SourceGameCodeDLLFullPath), SourceGameCodeDLLFullPath);

	char TempGameCodeDLLFilename[] = "handmade_temp.dll";
	char TempGameCodeDLLFullPath[WIN32_STATE_FILE_NAME_COUNT];

	Win32BuildEXEPathFileName(&Win32State, TempGameCodeDLLFilename,
		sizeof(TempGameCodeDLLFullPath), TempGameCodeDLLFullPath);

	char GameCodeLockFilename[] = "lock.tmp";
	char GameCodeLockFullPath[WIN32_STATE_FILE_NAME_COUNT];
	Win32BuildEXEPathFileName(&Win32State, GameCodeLockFilename,
		sizeof(GameCodeLockFullPath), GameCodeLockFullPath);


	UINT DesiredSchedulerMS = 1;
	bool32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();
#if HANDMADE_INTERNAL
	DEBUGGlobalShowCursor = true;
#endif
	WNDCLASS WindowClass = {};
	Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW;
	WindowClass.lpfnWndProc = Win32MainWindowCallback;
	WindowClass.hInstance = Instance;
	WindowClass.lpszClassName = "HandmadeHeroWindowClass";
	WindowClass.hCursor = LoadCursor(0, IDC_ARROW);

	if (RegisterClass(&WindowClass)) {
		HWND Window = CreateWindowEx(
			0, WindowClass.lpszClassName,
			"Test", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, Instance, 0);
		if (Window) {
			GlobalRunning = true;

			win32_sound_output SoundOutput = {};
			int	MonitorRefreshHz = 60;
			HDC RefreshDC = GetDC(Window);
			int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
			ReleaseDC(Window, RefreshDC);
			if (Win32RefreshRate > 1) {
				MonitorRefreshHz = Win32RefreshRate;
			}
			real32 GameUpdateHz = MonitorRefreshHz / 2.0f;
			real32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;


			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.SafetyBytes = (int)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);

			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16* Samples = (int16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes((uint64)2);
#else
			LPVOID BaseAddress = 0;
#endif

			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
#if 1
			GameMemory.TransientStorageSize = Gigabytes(1);
#else
			GameMemory.TransientStorageSize = Megabytes(20);
#endif
			GameMemory.DEBUGPlatformReadEntireFile = DEBUGPlatformReadEntireFile;
			GameMemory.DEBUGPlatformWriteEntireFile = DEBUGPlatformWriteEntireFile;
			GameMemory.DEBUGPlatformFreeFileMemory = DEBUGPlatformFreeFileMemory;

			Win32State.TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
			Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
				MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
			GameMemory.TransientStorage = ((uint8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			for (int ReplayIndex = 0; ReplayIndex < ArrayCount(Win32State.ReplayBuffers); ++ReplayIndex) {
				win32_replay_buffer* ReplayBuffer = &Win32State.ReplayBuffers[ReplayIndex];
				ReplayBuffer->MemoryBlock = VirtualAlloc(0, (size_t)Win32State.TotalSize,
					MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
				if (ReplayBuffer->MemoryBlock) {

				}
			}


			if (Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {
				game_input Input[2] = {};
				game_input* NewInput = &Input[0];
				game_input* OldInput = &Input[1];


				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();

				int DebugTimeMarkerIndex = 0;
				win32_debug_time_marker DebugTimeMarkers[30] = { 0 };

				DWORD AudioLatencyBytes = 0;
				real32 AudioLatencySeconds = 0;
				bool32 SoundIsValid = false;

				win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);

				int64 LastCycleCount = __rdtsc();
				while (GlobalRunning) {
					NewInput->dtForFrame = TargetSecondsPerFrame;

					FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);

					if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0) {
						Win32UnloadGameCode(&Game);
						Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
					}

					game_controller_input* OldKeyboardController = GetController(OldInput, 0);
					game_controller_input* NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;

					for (int ButtonIndex = 0; ButtonIndex < ArrayCount(NewKeyboardController->Buttons); ++ButtonIndex) {
						NewKeyboardController->Buttons[ButtonIndex].EndedDown = OldKeyboardController->Buttons[ButtonIndex].EndedDown;
					}

					Win32ProcessPendingMessages(&Win32State, NewKeyboardController);
					if (!GlobalPause) {
						POINT MouseP;
						GetCursorPos(&MouseP);
						ScreenToClient(Window, &MouseP);
						NewInput->MouseX = MouseP.x;
						NewInput->MouseY = MouseP.y;
						NewInput->MouseZ = 0;
						Win32ProcessKeyboardMessage(&NewInput->MouseBottons[0], GetKeyState(VK_LBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseBottons[1], GetKeyState(VK_MBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseBottons[2], GetKeyState(VK_RBUTTON) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseBottons[3], GetKeyState(VK_XBUTTON1) & (1 << 15));
						Win32ProcessKeyboardMessage(&NewInput->MouseBottons[4], GetKeyState(VK_XBUTTON2) & (1 << 15));

						DWORD MaxControllerCount = XUSER_MAX_COUNT;
						if (MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1)) {
							MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
						}
						for (DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex) {

							DWORD OurControllerIndex = ControllerIndex + 1;
							game_controller_input* OldController = GetController(OldInput, OurControllerIndex);
							game_controller_input* NewController = GetController(NewInput, OurControllerIndex);
							XINPUT_STATE ControllerState;
							if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
								NewController->IsConnected = true;
								NewController->IsAnalog = OldController->IsAnalog;
								XINPUT_GAMEPAD* Pad = &ControllerState.Gamepad;
								//NewController->IsAnalog = true;

								NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								if ((NewController->StickAverageX != 0.0f) || (NewController->StickAverageY != 0.0f)) {
									NewController->IsAnalog = true;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP) {
									NewController->StickAverageY = 1.0f;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) {
									NewController->StickAverageY = -1.0f;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) {
									NewController->StickAverageX = -1.0f;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) {
									NewController->StickAverageX = 1.0f;
									NewController->IsAnalog = false;
								}

								real32 Threshold = 0.5f;
								Win32ProcessXInputDigitalButton((NewController->StickAverageX < -Threshold) ? 1 : 0,
									&OldController->MoveLeft, 1, &NewController->MoveLeft);
								Win32ProcessXInputDigitalButton((NewController->StickAverageX > Threshold) ? 1 : 0,
									&OldController->MoveRight, 1, &NewController->MoveRight);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY < -Threshold) ? 1 : 0,
									&OldController->MoveDown, 1, &NewController->MoveDown);
								Win32ProcessXInputDigitalButton((NewController->StickAverageY > Threshold) ? 1 : 0,
									&OldController->MoveUp, 1, &NewController->MoveUp);

								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionDown, XINPUT_GAMEPAD_A,
									&NewController->ActionDown);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionRight, XINPUT_GAMEPAD_B,
									&NewController->ActionRight);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionLeft, XINPUT_GAMEPAD_X,
									&NewController->ActionLeft);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->ActionUp, XINPUT_GAMEPAD_Y,
									&NewController->ActionUp);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
									&NewController->LeftShoulder);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
									&NewController->RightShoulder);

								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Start, XINPUT_GAMEPAD_START,
									&NewController->Start);
								Win32ProcessXInputDigitalButton(Pad->wButtons,
									&OldController->Back, XINPUT_GAMEPAD_BACK,
									&NewController->Back);
							}
							else {
								NewController->IsConnected = false;
							}
						}

						thread_context Thread = {};

						game_offscreen_buffer Buffer = {};
						Buffer.Memory = GlobalBackbuffer.Memory;
						Buffer.Height = GlobalBackbuffer.Height;
						Buffer.Width = GlobalBackbuffer.Width;
						Buffer.Pitch = GlobalBackbuffer.Pitch;
						Buffer.BytesPerPixel = GlobalBackbuffer.BytesPerPixel;

						if (Win32State.InputRecordingIndex) {
							Win32RecordInput(&Win32State, NewInput);
						}
						if (Win32State.InputPlayingIndex) {
							Win32PlayBackInput(&Win32State, NewInput);
						}
						if (Game.UpdateAndRender) {
							Game.UpdateAndRender(&Thread, &GameMemory, NewInput, &Buffer);
						}

						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						real32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
						DWORD PlayCursor;
						DWORD WriteCursor;
						if (GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK) {
							if (!SoundIsValid) {
								SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								SoundIsValid = true;
							}

							DWORD ByteToLock = (SoundOutput.RunningSampleIndex * SoundOutput.BytesPerSample) %
								SoundOutput.SecondaryBufferSize;
							DWORD ExpectedSoundBytesPerFrame =
								(int)(((real32)SoundOutput.SamplesPerSecond * (real32)SoundOutput.BytesPerSample) / GameUpdateHz);

							real32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
							DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (real32)ExpectedSoundBytesPerFrame);

							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;

							DWORD SafeWriteCursor = WriteCursor;
							if (SafeWriteCursor < PlayCursor) {
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							Assert(SafeWriteCursor >= PlayCursor);
							SafeWriteCursor += SoundOutput.SafetyBytes;

							bool32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);

							DWORD TargetCursor = 0;
							if (AudioCardIsLowLatency) {
								TargetCursor = ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame;
							}
							else {
								TargetCursor = WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes;
							}
							TargetCursor = TargetCursor % SoundOutput.SecondaryBufferSize;

							DWORD BytesToWrite = 0;
							if (ByteToLock > TargetCursor) {
								BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
								BytesToWrite += TargetCursor;
							}
							else {
								BytesToWrite = TargetCursor - ByteToLock;
							}

							game_sound_output_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
							SoundBuffer.Samples = Samples;
							if (Game.GetSoundSamples) {
								Game.GetSoundSamples(&Thread, &GameMemory, &SoundBuffer);
							}


							Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
						}
						else {
							SoundIsValid = false;
						}

						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						real32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						real32 SecondsElapsedForFrame = WorkSecondsElapsed;
						if (SecondsElapsedForFrame < TargetSecondsPerFrame) {
							while (SecondsElapsedForFrame < TargetSecondsPerFrame) {
								if (SleepIsGranular) {
									DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
									if (SleepMS > 0) {
										Sleep(SleepMS);
									}
								}

								real32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
								if (TestSecondsElapsedForFrame < TargetSecondsPerFrame) {

								}
								while (SecondsElapsedForFrame < TargetSecondsPerFrame) {
									SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
								}
							}
						}
						else {

						}

						LARGE_INTEGER EndCounter = Win32GetWallClock();
						LastCounter = EndCounter;

						win32_window_dimension Dimension = Win32GetWindowDimension(Window);

						HDC DeviceContext = GetDC(Window);
						Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
						ReleaseDC(Window, DeviceContext);
						FlipWallClock = Win32GetWallClock();

#if 0
						{

							Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
							win32_debug_time_marker* Marker = &DebugTimeMarkers[DebugTimeMarkerIndex++];
							if (DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers)) {
								DebugTimeMarkerIndex = 0;
							}
							Marker->PlayCursor = PlayCursor;
							Marker->WriteCursor = WriteCursor;
						}
#endif



						game_input* Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;
					}
				}
			}
			else {

			}
		}
		else {

		}

	}
	else {

	}
	return(0);
}