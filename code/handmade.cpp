#include "handmade.h"
#include "handmade_random.h"

#include "handmade_world.cpp"
#include "handmade_sim_region.cpp"


#include "handmade_render_group.h"
#include "handmade_render_group.cpp"
#include "handmade_entity.cpp"

#include "handmade_asset.cpp"

#include "handmade_audio.h"
#include "handmade_audio.cpp"
#include "handmade_world_mode.cpp"
#include "handmade_cutscene.cpp"

internal task_with_memory*
BeginTaskWithMemory(transient_state* TranState, b32 DependOnGameMode) {
	task_with_memory* FoundTask = 0;
    
	for (u32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex) {
		task_with_memory* Task = TranState->Tasks + TaskIndex;
		if (!Task->BeingUsed) {
			Task->BeingUsed = true;
			FoundTask = Task;
			Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
            Task->DependOnGameMode = DependOnGameMode;
			break;
		}
	}
	return(FoundTask);
}

inline void
EndTaskWithMemory(task_with_memory* Task) {
	EndTemporaryMemory(Task->MemoryFlush);
	_WriteBarrier();
	Task->BeingUsed = false;
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena* Arena, s32 Width, s32 Height, b32 ClearToZero = true) {
	loaded_bitmap Result = {};
	Result.Width = SafeTruncateToU16(Width);
	Result.Height = SafeTruncateToU16(Height);
    Result.AlignPercentage = v2{0.5f, 0.5f};
    Result.WidthOverHeight = SafeRatio1((r32)Width, (r32)Height);
	Result.Pitch = Result.Width * BITMAP_BYTE_PER_PIXEL;
	s32 TotalBitmapSize = Width * Height * BITMAP_BYTE_PER_PIXEL;
	Result.Memory = PushSize(Arena, TotalBitmapSize, Align(16, ClearToZero));
	return(Result);
}

internal void
MakeSphereDiffuseMap(loaded_bitmap* Bitmap) {
	r32 InvWidth = 1.0f / (r32)(Bitmap->Width - 1);
	r32 InvHeight = 1.0f / (r32)(Bitmap->Height - 1);
	u8* Row = (u8*)Bitmap->Memory;
	for (s32 Y = 0; Y < Bitmap->Height; ++Y) {
		u32* Pixel = (u32*)Row;
		for (s32 X = 0; X < Bitmap->Width; ++X) {
			v2 BitmapUV = { InvWidth * r32(X), InvHeight * r32(Y) };
			r32 Nx = 2.0f * BitmapUV.x - 1.0f;
			r32 Ny = 2.0f * BitmapUV.y - 1.0f;
			r32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
			r32 Alpha = 0.0f;
            
			if (RootTerm >= 0.0f) {
				Alpha = 1.0f;
			}
            
			v3 BasedColor = { 0.0f, 0.0f, 0.0f };
			Alpha *= 255.0f;
			v4 Color = {
				Alpha * BasedColor.r, 
				Alpha * BasedColor.g,
				Alpha * BasedColor.b,
				Alpha
			};
			
			*Pixel++ = (u32)(Color.a + 0.5f) << 24 |
				(u32)(Color.r + 0.5f) << 16 |
				(u32)(Color.g + 0.5f) << 8 |
				(u32)(Color.b + 0.5f) << 0;
		}
		Row += Bitmap->Pitch;
	}
}

internal void
MakeSphereNormalMap(loaded_bitmap *Bitmap, r32 Roughness) {
	r32 InvWidth = 1.0f / (r32)(Bitmap->Width - 1);
	r32 InvHeight = 1.0f / (r32)(Bitmap->Height - 1);
	u8* Row = (u8*)Bitmap->Memory;
	for (s32 Y = 0; Y < Bitmap->Height; ++Y) {
		u32* Pixel = (u32*)Row;
		for (s32 X = 0; X < Bitmap->Width; ++X) {
			v2 BitmapUV = { InvWidth * r32(X), InvHeight * r32(Y) };
			r32 Nx = 2.0f * BitmapUV.x - 1.0f;
			r32 Ny = 2.0f * BitmapUV.y - 1.0f;
			r32 Nz = 0;
			r32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
			v3 Normal = v3{ 0, 0.7071067811865475f, 0.7071067811865475f };
			if (RootTerm >= 0.0f) {
				Nz = SquareRoot(RootTerm);
				Normal = v3{ Nx, Ny, Nz };
			}
			
			v4 Color = 255.0f * v4{
				(Normal.x + 1.0f) * 0.5f,
				(Normal.y + 1.0f) * 0.5f,
				(Normal.z + 1.0f) * 0.5f,
				Roughness
			};
			*Pixel++ = (u32)(Color.a + 0.5f) << 24 |
				(u32)(Color.r + 0.5f) << 16 |
				(u32)(Color.g + 0.5f) << 8 |
				(u32)(Color.b + 0.5f) << 0;
		}
		Row += Bitmap->Pitch;
	}
}

internal game_assets* 
DEBUGGetGameAsset(game_memory *Memory) {
    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    game_assets* Result = 0;
    if (TranState && TranState->IsInitialized) {
        Result = TranState->Assets;
    }
    return(Result);
}

internal u32
DEBUGGetMainGenerationID(game_memory *Memory) {
    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    u32 Result = 0;
    if (TranState && TranState->IsInitialized) {
        Result = TranState->MainGenerationID;
    }
    return(Result);
}


internal void
SetGameMode(game_state *GameState, transient_state *TranState, game_mode GameMode) {
    b32 WaitOnTask = false;
    for (u32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex) {
        task_with_memory *Task = TranState->Tasks + TaskIndex;
        if (Task->BeingUsed && Task->DependOnGameMode) {
            WaitOnTask = true;
            break;
        }
    }
    if (WaitOnTask) {
        Platform.CompleteAllWork(TranState->LowPriorityQueue);
    }
    Clear(&GameState->ModeArena);
    GameState->GameMode = GameMode;
}

internal b32
CheckForMetaInput(game_state *GameState, transient_state *TranState, game_input *Input) {
    b32 Rerun = false;
    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
        if (WasPressed(Controller->Back)) {
            Input->QuitRequested = true;
            Rerun = false;
            break;
        }
        else if (WasPressed(Controller->Start)) {
            EnterWorld(GameState, TranState);
            Rerun = true;
            break;
        }
    }
    
    return(Rerun);
}



#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	
	Platform = Memory->PlatformAPI;
#if HANDMADE_INTERNAL
	DebugGlobalMemory = Memory;
    
    {
        DEBUG_DATA_BLOCK("Renderer");
        DEBUG_B32(Global_Renderer_WeirdDrawBufferSize);
        {
            DEBUG_DATA_BLOCK("Camera");
            DEBUG_B32(Global_Renderer_UseDebugCamera);
            DEBUG_VALUE(Global_Renderer_Camera_DebugCameraDistance);
            DEBUG_B32(Global_Sim_RoomBaseCamera);
        }
    }
    {
        DEBUG_DATA_BLOCK("GroundChunks");
        DEBUG_VALUE(Global_GroundChunk_ReGenGroundChunkOnReload);
        DEBUG_VALUE(Global_GroundChunk_ShowGroundChunkOutlines);
        DEBUG_VALUE(Global_Sim_FamiliarFollowsHero);
        DEBUG_VALUE(Global_Renderer_Show_Space_Outline);
        DEBUG_VALUE(Global_Renderer_Camera);
    }
    {
        DEBUG_DATA_BLOCK("Particle");
        DEBUG_VALUE(Global_Particle_Demo);
        DEBUG_VALUE(Global_Particle_Grid);
    }
    {
        DEBUG_DATA_BLOCK("Profile");
        DEBUG_PROFILE(GameUpdateAndRender);
    }
    
#endif
	TIMED_FUNCTION();
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;
    
	if (!GameState->IsInitialized) {
        memory_arena TotalArena;
        InitializeArena(&TotalArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (u8*)Memory->PermanentStorage + sizeof(game_state));
        SubArena(&GameState->AudioState.AudioArena, &TotalArena, Megabytes(1));
		InitializeAudioState(&GameState->AudioState);
        SubArena(&GameState->ModeArena, &TotalArena, GetArenaSizeRemaining(&TotalArena), NoClear());
        
		GameState->IsInitialized = true;
	}
    
	Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
	transient_state* TranState = (transient_state*)Memory->TransientStorage;
    
	if (!TranState->IsInitialized) {
        
		InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (u8*)Memory->TransientStorage + sizeof(transient_state));
		
		for (u32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); TaskIndex++) {
			task_with_memory* Task = TranState->Tasks + TaskIndex;
			Task->BeingUsed = false;
			SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
		}
		TranState->HighPriorityQueue = Memory->HighPriorityQueue;
		TranState->LowPriorityQueue = Memory->LowPriorityQueue;
        
		TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(512), TranState);
		
		GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
		
		//DrawRectangle(&GameState->TestDiffuse, v2{ 0, 0 }, 
        //V2i(GameState->TestDiffuse.Width, GameState->TestDiffuse.Height), v4{ 0.5, 0.5, 0.5f, 1.0f });
        
		// normal map
		GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->TestDiffuse.Width, GameState->TestDiffuse.Height, 0);
		MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
		MakeSphereDiffuseMap(&GameState->TestDiffuse);
        
		TranState->EnvMapWidth = 512;
		TranState->EnvMapHeight = 256;
		for (u32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
			environment_map* Map = TranState->EnvMaps + MapIndex;
			u32 Width = TranState->EnvMapWidth;
			u32 Height = TranState->EnvMapHeight;
			for (u32 LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex) {
				Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, 0);
				Width >>= 1;
				Height >>= 1;
			}
		}
        
		GameState->Music = 0; //PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));
		//ChangePitch(GameState->Music, 0.8f);
		TranState->IsInitialized = true;
	}
    
    if (TranState->MainGenerationID) {
        EndGeneration(TranState->Assets, TranState->MainGenerationID);
    }
    TranState->MainGenerationID = BeginGeneration(TranState->Assets);
    
    if(GameState->GameMode == GameMode_None) {
        PlayIntroCutScene(GameState, TranState);
        
        game_controller_input *Controller = GetController(Input, 0);
        Controller->Start.EndedDown = true;
        Controller->Start.HalfTransitionCount = 1;
    }
    
	game_mode_world* GameWorld = GameState->WorldMode;
    
	temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    
    // TODO(NAME): remove this
    
    loaded_bitmap DrawBuffer_ = {};
    DrawBuffer_.Width = RenderCommands->Width;
    DrawBuffer_.Height = RenderCommands->Height;
    
    loaded_bitmap* DrawBuffer = &DrawBuffer_;
    
	render_group RenderGroup_ = BeginRenderGroup(TranState->Assets, RenderCommands, TranState->MainGenerationID, false);
    render_group *RenderGroup = &RenderGroup_;
    b32 Rerun = true;
    do {
        switch (GameState->GameMode) {
            case GameMode_TitleScreen: {
                Rerun = UpdateAndRenderTitleScreen(GameState, TranState, RenderGroup, DrawBuffer, GameState->TitleScreen, Input);
            } break;
            case GameMode_CutScrene: {
                Rerun = UpdateAndRenderCutScene(GameState, TranState, RenderGroup, DrawBuffer, GameState->CutScene, Input);
            } break;
            case GameMode_World: {
                Rerun = UpdateAndRenderWorld(GameState, GameWorld, TranState, Input, RenderGroup, DrawBuffer);
            } break;
            InvalidDefaultCase;
        }
    } while(Rerun);
    
    EndTemporaryMemory(RenderMemory);
    
    EndRenderGroup(RenderGroup);
    
#if 0    
    if (QuitRequested && !HeroExist) {
        Memory->QuitRequested = true;
    }
#endif
    
    CheckArena(&TranState->TranArena);
    CheckArena(&GameState->ModeArena);
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    OutputPlayingSounds(&GameState->AudioState, SoundBuffer, TranState->Assets, &TranState->TranArena);
}

#if HANDMADE_INTERNAL
#include "handmade_debug.cpp"
#else
extern "C" extern "C" DEBUG_FRAME_END(DEBUGGameFrameEnd) {
    return(0);
}
#endif