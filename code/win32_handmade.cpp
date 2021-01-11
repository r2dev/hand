#include "handmade_platform.h"
#include <windows.h>
#include <Xinput.h>
#include <malloc.h>
#include <stdio.h>
#include <dsound.h>

#include "win32_handmade.h"

global_variable b32 GlobalRunning;
global_variable b32 GlobalPause;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
global_variable int64 GlobalPerfCountFrequency;
global_variable b32 DEBUGGlobalShowCursor;
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
                       int X, int Top, int Bottom, u32 Color) {
	u8* Pixel = (u8*)Backbuffer->Memory +
		X * Backbuffer->BytesPerPixel +
		Top * Backbuffer->Pitch;
	for (int Y = Top; Y < Bottom; ++Y) {
		*(u32*)Pixel = Color;
		Pixel += Backbuffer->Pitch;
	}
}

inline void
Win32DrawSoundBufferMarker(win32_offscreen_buffer* Backbuffer, win32_sound_output* SoundOutput,
                           r32 C, int PadX, int Top, int Bottom, DWORD Value, u32 Color) {
	Assert(Value < SoundOutput->SecondaryBufferSize);
	r32 XReal32 = (C * (r32)Value);
	int X = PadX + (int)XReal32;
	Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}

internal void
Win32DebugSyncDisplay(win32_offscreen_buffer* Backbuffer, size_t MarkerCount,
                      win32_debug_time_marker* Markers, win32_sound_output* SoundOutput, r32 TargetSecondsPerFrame) {
	int PadX = 16;
	int PadY = 16;
	int Top = PadY;
	int Bottom = Backbuffer->Height - PadY;
    
	r32 C = (r32)(Backbuffer->Width - 2 * PadX) / (r32)SoundOutput->SecondaryBufferSize;
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex) {
		win32_debug_time_marker* ThisMarker = &Markers[MarkerIndex];
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,
                                   C, PadX, Top, Bottom, ThisMarker->PlayCursor, 0xFFFFFFFF);
		Win32DrawSoundBufferMarker(Backbuffer, SoundOutput,
                                   C, PadX, Top, Bottom, ThisMarker->WriteCursor, 0xFFFF0000);
	}
}
#endif
#if HANDMADE_INTERNAL
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
			u32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
			Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
			if (Result.Contents) {
				DWORD BytesRead;
				if (ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) &&
					(BytesRead == FileSize32)) {
					Result.ContentsSize = FileSize32;
				}
				else {
					DEBUGPlatformFreeFileMemory(Result.Contents);
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
	b32 Result = false;
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

DEBUG_PLATFORM_EXECUTE_SYSTEM_COMMAND(DEBUGPlatformExecuteSystemCommand) {
    debug_executing_process Result = {};
    STARTUPINFO StartupInfo = {};
    StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
    StartupInfo.cb = sizeof(STARTUPINFO);
    StartupInfo.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION ProcessInfo = {};
    if (CreateProcess(Command, CommandLine, 0, 0, FALSE, 0, 0, Path, &StartupInfo, &ProcessInfo)) {
        Assert(sizeof(Result.OSHandle) >= sizeof(ProcessInfo.hProcess));
        *(HANDLE *)&Result.OSHandle = ProcessInfo.hProcess;
    } else {
        DWORD ErrorCode = GetLastError();
        *(HANDLE *)&Result.OSHandle = INVALID_HANDLE_VALUE;
    }
    return(Result);
}

DEBUG_PLATFORM_GET_PROCESS_STATE(DEBUGPlatformGetProcessState) {
    debug_executing_state Result = {};
    HANDLE hProcess = *(HANDLE *)&Process.OSHandle;
    if (hProcess != INVALID_HANDLE_VALUE) {
        Result.StartedSuccessfully = true;
        if (WaitForSingleObject(hProcess, 0) == WAIT_OBJECT_0) {
            DWORD ReturnCode = 0;
            GetExitCodeProcess(hProcess, &ReturnCode);
            Result.ReturnCode = ReturnCode;
            CloseHandle(hProcess);
        } else {
            Result.IsRunning = true;
        }
    }
    return(Result);
}
#endif

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
            Result.DEBUGFrameEnd = (debug_game_frame_end*)
                GetProcAddress(Result.GameCodeDLL, "DEBUGGameFrameEnd");
            
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
Win32GetInputFileLocation(win32_state* State, b32 InputStream, int SlotIndex, int DestCount, char* Dest) {
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
Win32InitDSound(HWND Window, s32 SamplesPerSecond, s32 BufferSize) {
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
            BufferDescription.dwFlags = DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_GLOBALFOCUS;
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
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    
    Buffer->Pitch = Align16(Width * Buffer->BytesPerPixel);
    int BitmapMemorySize = Buffer->Pitch * Buffer->Height;
    
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    
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
#if 0
        int OffsetX = 10;
        int OffsetY = 10;
        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);
#else
        int OffsetX = 0;
        int OffsetY = 0;
#endif
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
        u8* DestSample = (u8*)Region1;
        for (DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
            *DestSample++ = 0;
        }
        DestSample = (u8*)Region2;
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
        s16* DestSample = (s16*)Region1;
        s16* SourceSample = SourceBuffer->Samples;
        for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        DWORD Region2SampleCount = Region2Size / SoundOutput->BytesPerSample;
        DestSample = (s16*)Region2;
        for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
            
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }
        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal r32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold) {
    r32 Result = 0;
    if (Value < -DeadZoneThreshold)
    {
        Result = (r32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if (Value > DeadZoneThreshold)
    {
        Result = (r32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
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
Win32ProcessKeyboardMessage(game_button_state* NewState, b32 IsDown) {
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
                u32 VKCode = (u32)Message.wParam;
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
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
                        b32 AltKeyWasDown = (Message.lParam & (1 << 29));
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

inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End) {
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart) / (r32)GlobalPerfCountFrequency);
    return(Result);
}

#if 0
internal void
HandleDebugCycleCounters(game_memory* Memory) {
#if HANDMADE_INTERNAL
    
    OutputDebugStringA("Debug cycle count: \n");
    for (int CounterIndex = 0; CounterIndex < ArrayCount(Memory->Counters); ++CounterIndex) {
        debug_cycle_counter* Counter = Memory->Counters + CounterIndex;
        if (Counter->HitCount) {
            char TextBuffer[256];
            _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                        "  %d: %I64ucy %uh %I64ucy/h\n",
                        CounterIndex,
                        Counter->CycleCount,
                        Counter->HitCount,
                        Counter->CycleCount / Counter->HitCount);
            
            OutputDebugStringA(TextBuffer);
            Counter->HitCount = 0;
            Counter->CycleCount = 0;
        }
    }
    
#endif
}
#endif

struct platform_work_queue_entry {
    platform_work_queue_callback* Callback;
    void* Data;
};

struct platform_work_queue {
    volatile u32 CompletedGoal;
    volatile u32 CompletedCount;
    
    volatile u32 NextEntryToWrite;
    volatile u32 NextEntryToRead;
    
    platform_work_queue_entry Entries[256];
    HANDLE SemaphoreHandle;
};

internal void
Win32AddEntry(platform_work_queue* Queue, platform_work_queue_callback* Callback, void* Data) {
    u32 NextNewEntryToWrite = (Queue->NextEntryToWrite + 1) % ArrayCount(Queue->Entries);
    Assert(NextNewEntryToWrite != Queue->NextEntryToRead);
    platform_work_queue_entry* Entry = Queue->Entries + Queue->NextEntryToWrite;
    Entry->Data = Data;
    Entry->Callback = Callback;
    ++Queue->CompletedGoal;
    _WriteBarrier();
    Queue->NextEntryToWrite = NextNewEntryToWrite;
    ReleaseSemaphore(Queue->SemaphoreHandle, 1, 0);
    
}

internal b32
Win32DoNextQueueEntry(platform_work_queue* Queue) {
    b32 WeShouldSleep = false;
    u32 OriginalNextEntryToRead = Queue->NextEntryToRead;
    u32 NewNextEntryToRead = (OriginalNextEntryToRead + 1) % ArrayCount(Queue->Entries);
    if (OriginalNextEntryToRead != Queue->NextEntryToWrite) {
        u32 Index = InterlockedCompareExchange((LONG volatile*)&Queue->NextEntryToRead, NewNextEntryToRead, OriginalNextEntryToRead);
        if (Index == OriginalNextEntryToRead) {
            platform_work_queue_entry Entry = Queue->Entries[Index];
            Entry.Callback(Queue, Entry.Data);
            InterlockedIncrement((LONG volatile*)&(Queue->CompletedCount));
        }
    }
    else {
        WeShouldSleep = true;
    }
    return(WeShouldSleep);
}

internal void
Win32CompleteAllWork(platform_work_queue* Queue) {
    while (Queue->CompletedGoal != Queue->CompletedCount) {
        Win32DoNextQueueEntry(Queue);
    }
    Queue->CompletedGoal = 0;
    Queue->CompletedCount = 0;
}

internal PLATFORM_WORK_QUEUE_CALLBACK(DoWorkerWork) {
    char Buffer[256];
    wsprintf(Buffer, "Thread %u: %s\n", GetCurrentThreadId(), (char*)Data);
    OutputDebugStringA(Buffer);
}


DWORD WINAPI ThreadProc(LPVOID lparam) {
    platform_work_queue* Queue = (platform_work_queue*)(lparam);
    
    for (;;) {
        if (Win32DoNextQueueEntry(Queue)) {
            WaitForSingleObjectEx(Queue->SemaphoreHandle, INFINITE, 0);
        }
    }
}

internal void
Win32MakeQueue(platform_work_queue* Queue, u32 ThreadCount) {
    Queue->CompletedCount = 0;
    Queue->CompletedGoal = 0;
    Queue->NextEntryToRead = 0;
    Queue->NextEntryToWrite = 0;
    u32 InitialCount = 0;
    Queue->SemaphoreHandle = CreateSemaphoreEx(0, InitialCount, ThreadCount, 0, 0, SEMAPHORE_ALL_ACCESS);
    for (u32 ThreadIndex = 0; ThreadIndex < ThreadCount; ++ThreadIndex) {
        HANDLE handle = CreateThread(0, 0, ThreadProc, Queue, 0, 0);
        CloseHandle(handle);
    }
}

struct win32_platform_file_handle {
    HANDLE Win32Handle;
};

struct win32_platform_file_group {
    HANDLE FindHandle;
    WIN32_FIND_DATAW FindData;
};
PLATFORM_GET_ALL_FILE_OF_TYPE_BEGIN(Win32GetAllFileOfTypeBegin) {
    
    platform_file_group Result = {};
    win32_platform_file_group *Win32FileGroup = (win32_platform_file_group *)VirtualAlloc(0, sizeof(win32_platform_file_group), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Result.Platform = Win32FileGroup;
    
    wchar_t *WildCard = L"*.*";
    
    switch(Type) {
        case PlatformFileType_AssetFile: {
            WildCard = L"*.hha";
        } break;
        case PlatformFileType_SaveGameFile: {
            WildCard = L"*.hhs";
        } break;
        
        InvalidDefaultCase;
    }
    Result.FileCount = 0;
    WIN32_FIND_DATAW FindData;
    HANDLE FindHandle = FindFirstFileW(WildCard, &FindData);
    while(FindHandle != INVALID_HANDLE_VALUE) {
        ++Result.FileCount;
        if (!FindNextFileW(FindHandle, &FindData)) {
            break;
        };
    }
    FindClose(FindHandle);
    Win32FileGroup->FindHandle = FindFirstFileW(WildCard, &Win32FileGroup->FindData);
    return(Result);
}

PLATFORM_GET_ALL_FILE_OF_TYPE_END(Win32GetAllFileOfTypeEnd) {
    win32_platform_file_group* Win32FileGroup = (win32_platform_file_group*)FileGroup->Platform;
    if (Win32FileGroup) {
        FindClose(Win32FileGroup->FindHandle);
        VirtualFree(Win32FileGroup, 0, MEM_RELEASE);
    }
    
}


PLATFORM_OPEN_FILE(Win32OpenNextFile) {
    platform_file_handle Result = {};
    win32_platform_file_group* Win32FileGroup = (win32_platform_file_group*)FileGroup->Platform;
    
    if (Win32FileGroup->FindHandle != INVALID_HANDLE_VALUE) {
        win32_platform_file_handle* Win32Handle= (win32_platform_file_handle* )VirtualAlloc(0, sizeof(win32_platform_file_handle), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
        Result.Platform = Win32Handle;
        if (Win32Handle) {
            wchar_t* FileName = Win32FileGroup->FindData.cFileName;
            Win32Handle->Win32Handle = CreateFileW(FileName, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
            Result.NoErrors = (Win32Handle->Win32Handle != INVALID_HANDLE_VALUE);
            
        }
        
        if (!FindNextFileW(Win32FileGroup->FindHandle, &Win32FileGroup->FindData)) {
            FindClose(Win32FileGroup->FindHandle);
            Win32FileGroup->FindHandle = INVALID_HANDLE_VALUE;
        }
    }
    return(Result);
}

PLATFORM_FILE_ERROR(Win32FileError) {
#if HANDMADE_INTERNAL
    OutputDebugStringA(Msg);
#endif
    Handle->NoErrors = false;
}

PLATFORM_READ_DATA_FROM_FILE(Win32ReadDataFromFile) {
    if (PlatformNoFileErrors(Source)) {
        win32_platform_file_handle* Handle = (win32_platform_file_handle*)Source->Platform;
        OVERLAPPED Overlapped = {};
        Overlapped.Offset = (u32)(((Offset >> 0) & 0xFFFFFFFF));
        Overlapped.OffsetHigh = (u32)((Offset >> 32) & 0xFFFFFFFF);
        u32 FileSize32 = SafeTruncateUInt64(Size);
        DWORD BytesRead;
        if (ReadFile(Handle->Win32Handle, Dest, FileSize32, &BytesRead, &Overlapped) && (FileSize32 == BytesRead)) {
        } else {
            Win32FileError(Source, "Read file failed");
        }
    }
}

PLATFORM_ALLOCATE_MEMORY(Win32AllocateMemory) {
    void* Result = VirtualAlloc(0, Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    return(Result);
}
PLATFORM_DEALLOCATE_MEMORY(Win32DeallocateMemory) {
    if (Memory) {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}
#if HANDMADE_INTERNAL
global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;
#endif

int CALLBACK WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CommandLine, int showCode) {
    
    platform_work_queue HighPriorityQueue;
    Win32MakeQueue(&HighPriorityQueue, 6);
    platform_work_queue LowPriorityQueue;
    Win32MakeQueue(&LowPriorityQueue, 2);
    
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
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);
    
    Win32LoadXInput();
#if HANDMADE_INTERNAL
    DEBUGGlobalShowCursor = true;
#endif
    WNDCLASS WindowClass = {};
    //Win32ResizeDIBSection(&GlobalBackbuffer, 960, 540);
    Win32ResizeDIBSection(&GlobalBackbuffer, 1920, 1080);
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
            int MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            int Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if (Win32RefreshRate > 1) {
                MonitorRefreshHz = Win32RefreshRate;
            }
            r32 GameUpdateHz = MonitorRefreshHz / 2.0f;
            r32 TargetSecondsPerFrame = 1.0f / GameUpdateHz;
            
            
            SoundOutput.SamplesPerSecond = 48000;
            SoundOutput.BytesPerSample = sizeof(s16) * 2;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
            SoundOutput.SafetyBytes = (int)(((r32)SoundOutput.SamplesPerSecond * (r32)SoundOutput.BytesPerSample / GameUpdateHz) / 3.0f);
            
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearBuffer(&SoundOutput);
            
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            u32 MaxPossibleOverrun = 2 * 8 * sizeof(u16);
            s16* Samples = (s16*)VirtualAlloc(0, SoundOutput.SecondaryBufferSize + MaxPossibleOverrun,
                                              MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
#if HANDMADE_INTERNAL
            LPVOID BaseAddress = (LPVOID)Terabytes((u64)2);
#else
            LPVOID BaseAddress = 0;
#endif
            
            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(256);
            GameMemory.TransientStorageSize = Gigabytes(1);
            GameMemory.DebugStorageSize = Megabytes(64);
            
            GameMemory.HighPriorityQueue = &HighPriorityQueue;
            GameMemory.LowPriorityQueue = &LowPriorityQueue;
            
#if HANDMADE_INTERNAL
            GameMemory.PlatformAPI.DEBUGReadEntireFile = DEBUGPlatformReadEntireFile;
            GameMemory.PlatformAPI.DEBUGWriteEntireFile = DEBUGPlatformWriteEntireFile;
            GameMemory.PlatformAPI.DEBUGFreeFileMemory = DEBUGPlatformFreeFileMemory;
            GameMemory.PlatformAPI.DEBUGExecuteSystemCommand = DEBUGPlatformExecuteSystemCommand;
            GameMemory.PlatformAPI.DEBUGGetProcessState = DEBUGPlatformGetProcessState;
#endif
            
            GameMemory.PlatformAPI.AddEntry = Win32AddEntry;
            GameMemory.PlatformAPI.CompleteAllWork = Win32CompleteAllWork;
            GameMemory.PlatformAPI.GetAllFileOfTypeBegin = Win32GetAllFileOfTypeBegin;
            GameMemory.PlatformAPI.GetAllFileOfTypeEnd = Win32GetAllFileOfTypeEnd;
            GameMemory.PlatformAPI.OpenNextFile = Win32OpenNextFile;
            GameMemory.PlatformAPI.ReadDataFromFile = Win32ReadDataFromFile;
            GameMemory.PlatformAPI.FileError = Win32FileError;
            GameMemory.PlatformAPI.AllocateMemory = Win32AllocateMemory;
            GameMemory.PlatformAPI.DeallocateMemory = Win32DeallocateMemory;
            
            Win32State.TotalSize = (GameMemory.PermanentStorageSize + 
                                    GameMemory.TransientStorageSize +
                                    GameMemory.DebugStorageSize);
            Win32State.GameMemoryBlock = VirtualAlloc(BaseAddress, (size_t)Win32State.TotalSize,
                                                      MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            
            GameMemory.PermanentStorage = Win32State.GameMemoryBlock;
            GameMemory.TransientStorage = ((u8*)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
            GameMemory.DebugStorage = ((u8*)GameMemory.TransientStorage + GameMemory.TransientStorageSize);
            
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
                r32 AudioLatencySeconds = 0;
                b32 SoundIsValid = false;
                
                win32_game_code Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
                
                while (GlobalRunning) {
                    ///
                    ///
                    ///
                    BEGIN_BLOCK(ExecutableRefresh);
                    
                    NewInput->dtForFrame = TargetSecondsPerFrame;
                    GameMemory.ExecutableReloaded = false;
                    FILETIME NewDLLWriteTime = Win32GetLastWriteTime(SourceGameCodeDLLFullPath);
                    
                    if (CompareFileTime(&NewDLLWriteTime, &Game.DLLLastWriteTime) != 0) {
                        Win32CompleteAllWork(&HighPriorityQueue);
                        Win32CompleteAllWork(&LowPriorityQueue);
                        
                        Win32UnloadGameCode(&Game);
#if HANDMADE_INTERNAL
                        GlobalDebugTable = &GlobalDebugTable_;
#endif
                        
                        Game = Win32LoadGameCode(SourceGameCodeDLLFullPath, TempGameCodeDLLFullPath, GameCodeLockFullPath);
                        GameMemory.ExecutableReloaded = true;
                    }
                    END_BLOCK(ExecutableRefresh);
                    
                    
                    
                    ///
                    ///
                    ///
                    BEGIN_BLOCK(ProcessInput);
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
                        NewInput->MouseX = (r32)MouseP.x ;
                        NewInput->MouseY = (r32)((GlobalBackbuffer.Height - 1.0f) - (r32)MouseP.y);
                        NewInput->MouseZ = 0;
                        
                        DWORD Win32BottomID[PlatformMouseButton_Count] = {
                            VK_LBUTTON,
                            VK_MBUTTON,
                            VK_RBUTTON,
                            VK_XBUTTON1,
                            VK_XBUTTON2
                        };
                        for (u32 MouseButtonIndex = 0; MouseButtonIndex < PlatformMouseButton_Count; ++MouseButtonIndex) {
                            NewInput->MouseButtons[MouseButtonIndex] = OldInput->MouseButtons[MouseButtonIndex];
                            NewInput->MouseButtons[MouseButtonIndex].HalfTransitionCount = 0;
                            Win32ProcessKeyboardMessage(&NewInput->MouseButtons[MouseButtonIndex], GetKeyState(Win32BottomID[MouseButtonIndex]) & (1 << 15));
                        }
                        
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
                                
                                r32 Threshold = 0.5f;
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
                    }
                    
                    END_BLOCK(ProcessInput);
                    
                    ///
                    ///
                    ///
                    BEGIN_BLOCK(GameUpdate);
                    
                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Width = GlobalBackbuffer.Width;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
                    if (!GlobalPause) {
                        if (Win32State.InputRecordingIndex) {
                            Win32RecordInput(&Win32State, NewInput);
                        }
                        if (Win32State.InputPlayingIndex) {
                            game_input Temp = *NewInput;
                            Win32PlayBackInput(&Win32State, NewInput);
                            for (u32 MouseButtonIndex = 0; MouseButtonIndex < PlatformMouseButton_Count; ++MouseButtonIndex) {
                                NewInput->MouseButtons[MouseButtonIndex] = Temp.MouseButtons[MouseButtonIndex];
                            }
                            NewInput->MouseX = Temp.MouseX;
                            NewInput->MouseY = Temp.MouseY;
                            NewInput->MouseZ = Temp.MouseZ;
                        }
                        if (Game.UpdateAndRender) {
                            Game.UpdateAndRender(&GameMemory, NewInput, &Buffer);
                            //HandleDebugCycleCounters(&GameMemory);
                        }
                        
                    }
                    END_BLOCK(GameUpdate);
                    
                    
                    ///
                    ///
                    ///
                    BEGIN_BLOCK(UpdateAudio);
                    if (!GlobalPause)
                    {
                        LARGE_INTEGER AudioWallClock = Win32GetWallClock();
                        r32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);
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
                                (int)(((r32)SoundOutput.SamplesPerSecond * (r32)SoundOutput.BytesPerSample) / GameUpdateHz);
                            
                            r32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                            DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (r32)ExpectedSoundBytesPerFrame);
                            
                            DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
                            
                            DWORD SafeWriteCursor = WriteCursor;
                            if (SafeWriteCursor < PlayCursor) {
                                SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                            }
                            Assert(SafeWriteCursor >= PlayCursor);
                            SafeWriteCursor += SoundOutput.SafetyBytes;
                            
                            b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);
                            
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
                            SoundBuffer.SampleCount = Align4(BytesToWrite / SoundOutput.BytesPerSample);
                            BytesToWrite = SoundBuffer.SampleCount * SoundOutput.BytesPerSample;
                            SoundBuffer.Samples = Samples;
                            if (Game.GetSoundSamples) {
                                Game.GetSoundSamples(&GameMemory, &SoundBuffer);
                            }
                            
                            
                            Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                        }
                        else {
                            SoundIsValid = false;
                        }
                        
                    }
                    END_BLOCK(UpdateAudio);
                    
                    ///
                    ///
                    ///debug colate
#if HANDMADE_INTERNAL
                    BEGIN_BLOCK(DebugCollation);
                    if (Game.DEBUGFrameEnd) {
                        GlobalDebugTable = Game.DEBUGFrameEnd(&GameMemory, NewInput, &Buffer);
                    }
                    GlobalDebugTable_.EventArrayIndex_EventIndex = 0;
                    END_BLOCK(DebugCollation);
#endif
                    ///
#if 1
                    BEGIN_BLOCK(WaitFrameComplete);
                    if (!GlobalPause) {
                        LARGE_INTEGER WorkCounter = Win32GetWallClock();
                        r32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);
                        
                        r32 SecondsElapsedForFrame = WorkSecondsElapsed;
                        if (SecondsElapsedForFrame < TargetSecondsPerFrame) {
                            while (SecondsElapsedForFrame < TargetSecondsPerFrame) {
                                if (SleepIsGranular) {
                                    DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
                                    if (SleepMS > 0) {
                                        Sleep(SleepMS);
                                    }
                                }
                                
                                r32 TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                                if (TestSecondsElapsedForFrame < TargetSecondsPerFrame) {
                                    
                                }
                                while (SecondsElapsedForFrame < TargetSecondsPerFrame) {
                                    SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
                                }
                            }
                        }
                        else {
                            
                        }
                        
                    }
                    END_BLOCK(WaitFrameComplete);
#endif
                    
                    ///
                    
                    BEGIN_BLOCK(EndOfFrame);
                    
                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
                    ReleaseDC(Window, DeviceContext);
                    
                    // note music syncing
                    FlipWallClock = Win32GetWallClock();
                    
                    game_input* Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    
                    
                    END_BLOCK(EndOfFrame);
                    
                    LARGE_INTEGER EndCounter = Win32GetWallClock();
                    
                    FRAME_MARKER(Win32GetSecondsElapsed(LastCounter, EndCounter));
                    LastCounter = EndCounter;
                    
#if HANDMADE_INTERNAL
                    if (GlobalDebugTable) {
                        GlobalDebugTable->RecordCounts[TRANSLATION_UNIT_INDEX] = __COUNTER__;
                    }
#endif
                    
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