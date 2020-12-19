#include "handmade.h"
#include "handmade_world.cpp"
#include "handmade_sim_region.cpp"

#include "handmade_render_group.h"
#include "handmade_render_group.cpp"
#include "handmade_entity.cpp"
#include "handmade_random.h"
#include "handmade_asset.cpp"
#include "handmade_audio.h"
#include "handmade_audio.cpp"

inline world_position
ChunkPositionFromTilePosition(world* World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ, v3 AdditionalOffset = v3{ 0, 0, 0 }) {
	world_position BasePos = {};
	
	// @Note(ren): remove
	real32 TileSideInMeters = 1.4f;
	real32 TileDepthInMeters = 3.0f;
    
	v3 TileDim = v3{ TileSideInMeters , TileSideInMeters , TileDepthInMeters };
    
	v3 Offset = Hadamard(TileDim, v3{ (real32)AbsTileX, (real32)AbsTileY, (real32)AbsTileZ });
    
	world_position Result = MapIntoChunkSpace(World, BasePos, Offset + AdditionalOffset);
    
	return(Result);
}


internal task_with_memory*
BeginTaskWithMemory(transient_state* TranState) {
	task_with_memory* FoundTask = 0;
    
	for (uint32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); ++TaskIndex) {
		task_with_memory* Task = TranState->Tasks + TaskIndex;
		if (!Task->BeingUsed) {
			Task->BeingUsed = true;
			FoundTask = Task;
			Task->MemoryFlush = BeginTemporaryMemory(&Task->Arena);
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

struct add_low_entity_result {
	uint32 LowIndex;
	low_entity* Low;
};

inline add_low_entity_result
AddLowEntity(game_state* GameState, entity_type Type, world_position P) {
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
    
	uint32 EntityIndex = GameState->LowEntityCount++;
	low_entity* LowEntity = GameState->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Sim.Type = Type;
	LowEntity->Sim.Collision = GameState->NullCollision;
	LowEntity->P = NullPosition();
	ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, LowEntity, P);
    
	add_low_entity_result Result = {};
	Result.Low = LowEntity;
	Result.LowIndex = EntityIndex;
	return(Result);
}

inline add_low_entity_result
AddGroundedEntity(game_state* GameState, entity_type Type, world_position P, sim_entity_collision_volume_group * Collision) {
	add_low_entity_result Entity = AddLowEntity(GameState, Type, P);
	Entity.Low->Sim.Collision = Collision;
	return(Entity);
}

internal void
InitHitPoints(low_entity* EntityLow, uint32 HitPointCount) {
	Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoint));
	EntityLow->Sim.HitPointMax = HitPointCount;
	for (uint32 HitPointIndex = 0; HitPointIndex < EntityLow->Sim.HitPointMax; ++HitPointIndex) {
		hit_point* HitPoint = EntityLow->Sim.HitPoint + HitPointIndex;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
		HitPoint->Flag = 0;
	}
}

internal add_low_entity_result
AddStandardRoom(game_state* GameState, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Space, P);
	Entity.Low->Sim.Collision = GameState->StandardRoomCollision;
	AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);
	return(Entity);
}

internal add_low_entity_result
AddSword(game_state* GameState) {
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Sword, NullPosition());
	Entity.Low->Sim.Collision = GameState->SwordCollision;
    
	AddFlags(&Entity.Low->Sim, EntityFlag_Moveable);
	return(Entity);
}

internal add_low_entity_result
AddPlayer(game_state* GameState) {
	world_position P = GameState->CameraP;
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Hero, P, GameState->PlayerCollision);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	InitHitPoints(Entity.Low, 3);
    
	add_low_entity_result Sword = AddSword(GameState);
	Entity.Low->Sim.Sword.Index = Sword.LowIndex;
    
    
	if (GameState->CameraFollowingEntityIndex == 0) {
		GameState->CameraFollowingEntityIndex = Entity.LowIndex;
	}
	return(Entity);
}

internal add_low_entity_result
AddStair(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Stairwell, P, GameState->StairCollision);
	Entity.Low->Sim.WalkableHeight = GameState->TypicalFloorHeight;
	Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.xy;
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
	return(Entity);
}

internal add_low_entity_result
AddWall(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Wall, P, GameState->WallCollision);
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
	return(Entity);
}

internal add_low_entity_result
AddMonster(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Monster, P, GameState->MonstarCollision);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	
	InitHitPoints(Entity.Low, 2);
	return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameState, EntityType_Familiar, P, GameState->FamiliarCollision);
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	return(Entity);
}

internal void
DrawHitPoints(sim_entity* Entity, render_group* PieceGroup) {
	if (Entity->HitPointMax >= 1) {
		v2 HealthDim = { 0.2f, 0.2f };
		real32 SpacingX = 1.5f * HealthDim.x;
		v2 HitP = { -0.5f * (Entity->HitPointMax - 1) * SpacingX, -0.2f };
		v2 dHitP = { SpacingX, 0.0f };
		for (uint32 HealthIndex = 0; HealthIndex < Entity->HitPointMax; ++HealthIndex) {
			hit_point* HitPoint = Entity->HitPoint + HealthIndex;
			v4 Color = { 1.0f, 0.0f, 0.0f, 1.0f };
			if (HitPoint->FilledAmount == 0) {
				Color = { 0.2f, 0.2f, 0.2f, 1.0f };
			}
			PushRect(PieceGroup, V3(HitP, 0), HealthDim, Color);
			HitP += dHitP;
		}
	}
}

internal sim_entity_collision_volume_group*
MakeSimpleGroundedCollision(game_state* GameState, real32 DimX, real32 DimY, real32 DimZ) {
	sim_entity_collision_volume_group* Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
	Group->VolumeCount = 1;
	Group->Volumes = PushArray(&GameState->WorldArena, 1, sim_entity_collision_volume);
	Group->TotalVolume.Dim = v3{ DimX, DimY, DimZ };
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0.5f * DimZ };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal sim_entity_collision_volume_group*
MakeNullCollision(game_state* GameState) {
	sim_entity_collision_volume_group* Group = PushStruct(&GameState->WorldArena, sim_entity_collision_volume_group);
	Group->VolumeCount = 0;
	Group->Volumes = 0;
	Group->TotalVolume.Dim = v3{ 0, 0, 0};
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0 };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal void
ClearCollisionRulesFor(game_state* GameState, uint32 StorageIndex) {
	for (uint32 HashBucket = 0; HashBucket < ArrayCount(GameState->CollisionRuleHash); ++HashBucket) {
		for (pairwise_collision_rule** Rule = &GameState->CollisionRuleHash[HashBucket]; *Rule;) {
			if ((*Rule)->StorageIndexA == StorageIndex || (*Rule)->StorageIndexB == StorageIndex) {
				pairwise_collision_rule* RemovedRule = *Rule;
				*Rule = (*Rule)->NextHash;
                
				RemovedRule->NextHash = GameState->FirstFreeCollisionRule;
				GameState->FirstFreeCollisionRule = RemovedRule;
			}
			else {
				Rule = &(*Rule)->NextHash;
			}
		}
	}
    
}

internal void
AddCollisionRule(game_state* GameState, uint32 StorageIndexA, uint32 StorageIndexB, bool32 CanCollide) {
	if (StorageIndexA > StorageIndexB) {
		uint32 Temp = StorageIndexA;
		StorageIndexA = StorageIndexB;
		StorageIndexB = Temp;
	}
	pairwise_collision_rule* Found = 0;
	uint32 HashBucket = StorageIndexA & (ArrayCount(GameState->CollisionRuleHash) - 1);
	for (pairwise_collision_rule* Rule = GameState->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
		if (Rule->StorageIndexA == StorageIndexA && Rule->StorageIndexB == StorageIndexB) {
			Found = Rule;
            
			break;
		}
	}
    
    
	if (!Found) {
		Found = GameState->FirstFreeCollisionRule;
		if (Found) {
			GameState->FirstFreeCollisionRule = Found->NextHash;
		}
		else {
			Found = PushStruct(&GameState->WorldArena, pairwise_collision_rule);
		}
		Found->NextHash = GameState->CollisionRuleHash[HashBucket];
		GameState->CollisionRuleHash[HashBucket] = Found;
	}
    
	if (Found) {
		Found->StorageIndexA = StorageIndexA;
		Found->StorageIndexB = StorageIndexB;
		Found->CanCollide = CanCollide;
	}
    
	Assert(Found);
}

struct fill_ground_chunk_work {
    transient_state *TranState;
    game_state* GameState;
    ground_buffer* GroundBuffer;
    world_position ChunkP;
	task_with_memory* Task;
	
};

PLATFORM_WORK_QUEUE_CALLBACK(FillGroundChunkWork) {
	fill_ground_chunk_work* Work = (fill_ground_chunk_work*)Data;
    loaded_bitmap* Buffer = &Work->GroundBuffer->Bitmap;
    Buffer->AlignPercentage = v2{ 0.5f, 0.5f };
    Buffer->WidthOverHeight = 1.0f;
    real32 Width = Work->GameState->World->ChunkDimInMeters.x;
    real32 Height =Work->GameState->World->ChunkDimInMeters.y;
    render_group* RenderGroup = AllocateRenderGroup(Work->TranState->Assets, &Work->Task->Arena, 0, true);
    BeginRender(RenderGroup);
    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);
    Clear(RenderGroup, v4{ 1.0f, 1.0f, 0.0f, 1.0f });
    
    v2 HalfDim = 0.5f * V2(Width, Height);
    HalfDim = 2.0f * HalfDim;
    for (int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
        for (int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
            
            int32 ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            int32 ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            int32 ChunkZ = Work->ChunkP.ChunkZ;
            random_series Series = RandomSeed(139 * ChunkX + 593 * ChunkY + 329 * ChunkZ);
            
            v2 Center = v2{ ChunkOffsetX * Width, ChunkOffsetY * Height };
            
            for (uint32 GrassIndex = 0; GrassIndex < 20; GrassIndex++) {
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets, RandomChoice(&Series, 2) ? Asset_Grass : Asset_Ground, &Series);
                v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 4.0f, V3(P, 0.0f));
            }
        }
    }
    for (int32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
        for (int32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
            
            int32 ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            int32 ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            int32 ChunkZ = Work->ChunkP.ChunkZ;
            random_series Series = RandomSeed(139 * ChunkX + 593 * ChunkY + 329 * ChunkZ);
            
            v2 Center = v2{ ChunkOffsetX * Width, ChunkOffsetY * Height };
            
            for (uint32 GrassIndex = 0; GrassIndex < 20; GrassIndex++) {
                
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets, Asset_Tuft, &Series);
                
                v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 0.4f, V3(P, 0.0f));
            }
        }
    }
    Assert(AllResoucePresent(RenderGroup));
	RenderGroupToOutput(RenderGroup, Buffer);
    EndRender(RenderGroup);
	EndTaskWithMemory(Work->Task);
}

internal void
FillGroundChunk(transient_state *TranState, game_state* GameState, ground_buffer* GroundBuffer, world_position *ChunkP) {
	task_with_memory* AvailableTask = BeginTaskWithMemory(TranState);
	if (AvailableTask) {
		fill_ground_chunk_work* Work = PushStruct(&AvailableTask->Arena, fill_ground_chunk_work);
		Work->Task = AvailableTask;
		
		Work->TranState = TranState;
        Work->GameState = GameState;
        Work->GroundBuffer = GroundBuffer;
		Work->ChunkP = *ChunkP;
        GroundBuffer->P = *ChunkP;
        Platform.AddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);
	}
}

internal void
ClearBitmap(loaded_bitmap* Bitmap) {
	if (Bitmap->Memory) {
		int32 TotalBitmapSize = Bitmap->Width * Bitmap->Height * BITMAP_BYTE_PER_PIXEL;
		ZeroSize(TotalBitmapSize, Bitmap->Memory);
	}
}

internal loaded_bitmap
MakeEmptyBitmap(memory_arena* Arena, int32 Width, int32 Height, bool32 ClearToZero = true) {
	loaded_bitmap Result = {};
	Result.Width = SafeTruncateToUInt16(Width);
	Result.Height = SafeTruncateToUInt16(Height);
    Result.AlignPercentage = v2{0.5f, 0.5f};
    Result.WidthOverHeight = SafeRatio1((r32)Width, (r32)Height);
	Result.Pitch = Result.Width * BITMAP_BYTE_PER_PIXEL;
	int32 TotalBitmapSize = Width * Height * BITMAP_BYTE_PER_PIXEL;
	Result.Memory = PushSize(Arena, TotalBitmapSize, 16);
	if (ClearToZero) {
		ClearBitmap(&Result);
	}
	return(Result);
}

internal void
MakeSphereDiffuseMap(loaded_bitmap* Bitmap) {
	real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
	real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);
	uint8* Row = (uint8*)Bitmap->Memory;
	for (int32 Y = 0; Y < Bitmap->Height; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int32 X = 0; X < Bitmap->Width; ++X) {
			v2 BitmapUV = { InvWidth * real32(X), InvHeight * real32(Y) };
			real32 Nx = 2.0f * BitmapUV.x - 1.0f;
			real32 Ny = 2.0f * BitmapUV.y - 1.0f;
			real32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
			real32 Alpha = 0.0f;
            
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
			
			*Pixel++ = (uint32)(Color.a + 0.5f) << 24 |
				(uint32)(Color.r + 0.5f) << 16 |
				(uint32)(Color.g + 0.5f) << 8 |
				(uint32)(Color.b + 0.5f) << 0;
		}
		Row += Bitmap->Pitch;
	}
}

internal void
MakeSphereNormalMap(loaded_bitmap *Bitmap, real32 Roughness) {
	real32 InvWidth = 1.0f / (real32)(Bitmap->Width - 1);
	real32 InvHeight = 1.0f / (real32)(Bitmap->Height - 1);
	uint8* Row = (uint8*)Bitmap->Memory;
	for (int32 Y = 0; Y < Bitmap->Height; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int32 X = 0; X < Bitmap->Width; ++X) {
			v2 BitmapUV = { InvWidth * real32(X), InvHeight * real32(Y) };
			real32 Nx = 2.0f * BitmapUV.x - 1.0f;
			real32 Ny = 2.0f * BitmapUV.y - 1.0f;
			real32 Nz = 0;
			real32 RootTerm = 1.0f - Square(Nx) - Square(Ny);
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
			*Pixel++ = (uint32)(Color.a + 0.5f) << 24 |
				(uint32)(Color.r + 0.5f) << 16 |
				(uint32)(Color.g + 0.5f) << 8 |
				(uint32)(Color.b + 0.5f) << 0;
		}
		Row += Bitmap->Pitch;
	}
}



inline rectangle2
GetCameraRectAtDistance(render_group* RenderGroup, real32 CameraDistance) {
	v2 HalfDimOnTarget = (CameraDistance / RenderGroup->Transform.FocalLength) * RenderGroup->MonitorHalfDimInMeters;
	rectangle2 Result = RectCenterHalfDim(v2{ 0, 0 }, HalfDimOnTarget);
	return(Result);
    
}

inline rectangle2
GetCameraRectAtTarget(render_group* RenderGroup) {
	rectangle2 Result = GetCameraRectAtDistance(RenderGroup, RenderGroup->Transform.DistanceAboveTarget);
	return(Result);
}

global_variable render_group *DEBUGRenderGroup;
global_variable r32 AtY;
global_variable r32 LeftEdge;
global_variable r32 FontScale;
global_variable font_id FontID;
internal void
DEBUGReset(game_assets *Assets, u32 Width, u32 Height) {
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    
    FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);
    
    FontScale = 80.0f / 255.0f;
    Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
    LeftEdge = -0.5f * (r32)Width;
    
    hha_font *Info = GetFontInfo(Assets, FontID);
    AtY = Height - (Info->Ascent + Info->Descent);
}

inline b32
IsHex(char Char) {
    b32 Result = (((Char >= '0') && (Char <= '9')) || ((Char >= 'A')&& (Char <= 'F')));
    return(Result);
}

inline u32
GetHex(char Char) {
    u32 Result = 0;
    if (Char >= '0' && Char <= '9') {
        Result = Char - '0';
    } else if (Char >= 'A' && Char <= 'F') {
        Result = 0xA + (Char - 'A');
    }
    return(Result);
}


internal void
DEBUGTextLine(char *String) {
    if (DEBUGRenderGroup) {
        render_group* RenderGroup = DEBUGRenderGroup;
        
        loaded_font* Font = PushFont(RenderGroup, FontID);
        if (Font) {
            hha_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
            u32 PrevCodePoint = 0;
            
            r32 AtX = LeftEdge;
            r32 CharScale = FontScale;
            
            for(char* At = String;
                *At;
                ++At) {
                
                u32 CodePoint = *At;
                r32 AdvancedX = 0;
                
                if (At[0] == '\\' && IsHex(At[1]) && IsHex(At[2]) && IsHex(At[3]) && IsHex(At[4])) {
                    CodePoint = ((GetHex(At[1]) << 12) | (GetHex(At[2]) << 8) | (GetHex(At[3]) << 4) | (GetHex(At[4]) << 0));
                    At += 4;
                }
                if(PrevCodePoint) {
                    AdvancedX = CharScale * GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                }
                AtX += AdvancedX;
                
                if (CodePoint != ' ') {
                    // proportional mode
                    // AdvancedX = CharScale * (r32)(Info->Dim[0] + 2);
                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                    hha_bitmap* BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);
                    PushBitmap(RenderGroup, BitmapID, CharScale * (r32)BitmapInfo->Dim[1], v3{AtX, AtY, 0}, v4{1.0f, 1.0f, 1.0f, 1});
                }
                // advance here
                
                PrevCodePoint = CodePoint;
            }
            AtY -= GetLineAdvancedFor(Info) * CharScale;
        } else {
            
        }
    }
}

internal void
OverlayCycleCounters(game_memory* Memory) {
#if HANDMADE_INTERNAL
    
    char* NameTable[] = {
        "GameUpdateAndRender",
        "RenderGroupToOutput",
        "DrawRectangleSlowly",
        "FillPixel",
        "TestPixel",
    };
    DEBUGTextLine("AVA WA aTa");
    DEBUGTextLine("$7$7$");
    DEBUGTextLine("\\6024\\7363");
	
	for (int CounterIndex = 0; CounterIndex < ArrayCount(Memory->Counters); ++CounterIndex) {
		debug_cycle_counter* Counter = Memory->Counters + CounterIndex;
		if (Counter->HitCount) {
            
#if 0
            
			char TextBuffer[256];
			_snprintf_s(TextBuffer, sizeof(TextBuffer), 
                        "  %d: %I64ucy %uh %I64ucy/h\n",
                        CounterIndex,
                        Counter->CycleCount,
                        Counter->HitCount,
                        Counter->CycleCount / Counter->HitCount);
            
			OutputDebugStringA(TextBuffer);
#else
            DEBUGTextLine(NameTable[CounterIndex]);
#endif
		}
	}
#endif
}


#if HANDMADE_INTERNAL
game_memory* DebugGlobalMemory;
#endif
extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	
	Platform = Memory->PlatformAPI;
#if HANDMADE_INTERNAL
	DebugGlobalMemory = Memory;
#endif
	BEGIN_TIMED_BLOCK(GameUpdateAndRender);
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;
    
	uint32 GroundBufferWidth = 256;
	uint32 GroundBufferHeight = 256;
	
    
	if (!GameState->IsInitialized) {
		GameState->TypicalFloorHeight = 3.0f;
		real32 PixelsToMeters = 1.0f / 42.0f;
		
		v3 WorldChunkDimMeters = { PixelsToMeters * GroundBufferWidth, PixelsToMeters * GroundBufferHeight, GameState->TypicalFloorHeight };
        
		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
                        (uint8*)Memory->PermanentStorage + sizeof(game_state));
        
		InitializeAudioState(&GameState->AudioState, &GameState->WorldArena);
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world* World = GameState->World;
        
		InitializeWorld(World, WorldChunkDimMeters);
		
		int32 TileSideInPixels = 60;
        
		AddLowEntity(GameState, EntityType_Null, NullPosition());
		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
        
		real32 TileSideInMeters = 1.4f;
		real32 TileDepthInMeters = GameState->TypicalFloorHeight;
		
		GameState->NullCollision = MakeSimpleGroundedCollision(GameState, 0, 0, 0);
		GameState->SwordCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.1f);
		GameState->StairCollision = MakeSimpleGroundedCollision(GameState, 
                                                                TileSideInMeters, 2.0f * TileSideInMeters, 1.1f * TileDepthInMeters);
		GameState->PlayerCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 1.2f);
		GameState->MonstarCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
		GameState->WallCollision = MakeSimpleGroundedCollision(GameState, TileSideInMeters, TileSideInMeters, TileDepthInMeters);
		GameState->StandardRoomCollision = MakeSimpleGroundedCollision(GameState, TilesPerWidth * TileSideInMeters, TilesPerHeight * TileSideInMeters, 0.9f * TileDepthInMeters);
		GameState->FamiliarCollision = MakeSimpleGroundedCollision(GameState, 1.0f, 0.5f, 0.5f);
        
		random_series Series = RandomSeed(1234);
        GameState->EffectEntropy = RandomSeed(5456);
		
		
		uint32 ScreenBaseX = 0;
		uint32 ScreenBaseY = 0;
		uint32 ScreenBaseZ = 0;
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;
        
		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;
        
		for (uint32 ScreenIndex = 0; ScreenIndex < 30; ++ScreenIndex) {
			
#if 1
			uint32 DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown)? 2: 4);
#else
			uint32 DoorDirection = RandomChoice(&Series, 2);
#endif
            
			bool32 CreatedZDoor = false;
			if (DoorDirection == 3) {
				CreatedZDoor = true;
				DoorDown = true;
			}
			else if (DoorDirection == 2) {
				CreatedZDoor = true;
				DoorUp = true;
			}
			else if (DoorDirection == 1) {
				DoorRight = true;
			}
			else {
				DoorTop = true;
			}
			AddStandardRoom(GameState, 
                            (ScreenX * TilesPerWidth + TilesPerWidth / 2), 
                            (ScreenY * TilesPerHeight + TilesPerHeight / 2), 
                            AbsTileZ);
            
            
			for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
				for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
					uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
					uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;
                    
					bool32 ShouldBeDoor = false;
					if (TileX == 0 && (!DoorLeft || TileY != (TilesPerHeight / 2))) {
						ShouldBeDoor = true;
					}
                    
					if (TileX == TilesPerWidth - 1 && (!DoorRight || TileY != (TilesPerHeight / 2))) {
						ShouldBeDoor = true;
					}
                    
					if (TileY == 0 && (!DoorBottom || TileX != (TilesPerWidth / 2))) {
						ShouldBeDoor = true;
					}
                    
					if (TileY == TilesPerHeight - 1 && (!DoorTop || TileX != (TilesPerWidth / 2))) {
						ShouldBeDoor = true;
					}
                    
					if (ShouldBeDoor) {
						//if (ScreenIndex == 0) {
						
                        AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
						
						//}
					} else if (CreatedZDoor) {
						if (((AbsTileZ % 2) && (TileX == 10) && (TileY == 5)) ||
							(!(AbsTileZ % 2) && (TileX == 4) && (TileY == 5)))
						{
							AddStair(GameState, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
						}
						/**if (TileY == 5 && TileX == 10) {
							AddStair(GameState, AbsTileX, AbsTileY, DoorDown? AbsTileZ - 1: AbsTileZ);
						}*/
					}
                    
                    
				}
			}
			DoorLeft = DoorRight;
			DoorBottom = DoorTop;
            
			if (CreatedZDoor) {
				DoorDown = !DoorDown;
				DoorUp = !DoorUp;
			}
			else {
				DoorUp = false;
				DoorDown = false;
			}
            
			DoorRight = false;
			DoorTop = false;
            
			if (DoorDirection == 3) {
				AbsTileZ -= 1;
			}
#if 1
			else if (DoorDirection == 2) {
				AbsTileZ += 1;
			}
			else if (DoorDirection == 1) {
				ScreenX += 1;
			}
			else {
				ScreenY += 1;
			}
#endif
		}
        
		world_position NewCameraP = {};
		uint32 CameraTileX = ScreenBaseX * TilesPerWidth + 17 / 2;
		uint32 CameraTileY = ScreenBaseY * TilesPerHeight + 9 / 2;
		uint32 CameraTileZ = ScreenBaseZ;
		NewCameraP =
			ChunkPositionFromTilePosition(GameState->World, CameraTileX, CameraTileY, CameraTileZ);
		GameState->CameraP = NewCameraP;
		AddMonster(GameState, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
		AddFamiliar(GameState, CameraTileX - 2, CameraTileY - 2, CameraTileZ);
		GameState->IsInitialized = true;
	}
    
	Assert(sizeof(transient_state) <= Memory->TransientStorageSize);
	transient_state* TranState = (transient_state*)Memory->TransientStorage;
    
	if (!TranState->IsInitialized) {
        
		InitializeArena(&TranState->TranArena, Memory->TransientStorageSize - sizeof(transient_state),
                        (uint8*)Memory->TransientStorage + sizeof(transient_state));
		
		for (uint32 TaskIndex = 0; TaskIndex < ArrayCount(TranState->Tasks); TaskIndex++) {
			task_with_memory* Task = TranState->Tasks + TaskIndex;
			Task->BeingUsed = false;
			SubArena(&Task->Arena, &TranState->TranArena, Megabytes(1));
		}
		TranState->HighPriorityQueue = Memory->HighPriorityQueue;
		TranState->LowPriorityQueue = Memory->LowPriorityQueue;
        
		TranState->Assets = AllocateGameAssets(&TranState->TranArena, Megabytes(4), TranState);
        DEBUGRenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, Megabytes(16), false);
		
		TranState->GroundBufferCount = 256;
		TranState->GroundBuffers = PushArray(&TranState->TranArena, TranState->GroundBufferCount, ground_buffer);
		
		for (uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; GroundBufferIndex++) {
			ground_buffer* GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
			GroundBuffer->Bitmap = MakeEmptyBitmap(&TranState->TranArena, GroundBufferWidth, GroundBufferHeight, false);
			GroundBuffer->P = NullPosition();
		}		
		GameState->TestDiffuse = MakeEmptyBitmap(&TranState->TranArena, 256, 256, false);
		
		//DrawRectangle(&GameState->TestDiffuse, v2{ 0, 0 }, 
        //V2i(GameState->TestDiffuse.Width, GameState->TestDiffuse.Height), v4{ 0.5, 0.5, 0.5f, 1.0f });
        
		// normal map
		GameState->TestNormal = MakeEmptyBitmap(&TranState->TranArena, GameState->TestDiffuse.Width, GameState->TestDiffuse.Height, 0);
		MakeSphereNormalMap(&GameState->TestNormal, 0.0f);
		MakeSphereDiffuseMap(&GameState->TestDiffuse);
        
		TranState->EnvMapWidth = 512;
		TranState->EnvMapHeight = 256;
		for (uint32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
			environment_map* Map = TranState->EnvMaps + MapIndex;
			uint32 Width = TranState->EnvMapWidth;
			uint32 Height = TranState->EnvMapHeight;
			for (uint32 LODIndex = 0; LODIndex < ArrayCount(Map->LOD); ++LODIndex) {
				Map->LOD[LODIndex] = MakeEmptyBitmap(&TranState->TranArena, Width, Height, 0);
				Width >>= 1;
				Height >>= 1;
			}
		}
        
		GameState->Music = 0; //PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Music));
		//ChangePitch(GameState->Music, 0.8f);
        
		TranState->IsInitialized = true;
	}
    if (DEBUGRenderGroup) {
        BeginRender(DEBUGRenderGroup);
        DEBUGReset(DEBUGRenderGroup->Assets, Buffer->Width, Buffer->Height);
    }
    
	world* World = GameState->World;
    
	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		controlled_hero* ConHero = GameState->ControlledHeroes + ControllerIndex;
		if (ConHero->EntityIndex == 0) {
			if (Controller->Start.EndedDown) {
				*ConHero = {};
				ConHero->EntityIndex = AddPlayer(GameState).LowIndex;
			}
		}
		else {
			ConHero->ddP = {};
			ConHero->dZ = 0.0f;
			ConHero->dSword = {};
            
			if (Controller->IsAnalog) {
				ConHero->ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
			}
			else {
                
				if (Controller->MoveUp.EndedDown) {
					ConHero->ddP.y = 1.0f;
				}
				if (Controller->MoveDown.EndedDown) {
					ConHero->ddP.y = -1.0f;
				}
				if (Controller->MoveLeft.EndedDown) {
					ConHero->ddP.x = -1.0f;
				}
				if (Controller->MoveRight.EndedDown) {
					ConHero->ddP.x = 1.0f;
				}
			}
			if (Controller->Start.EndedDown) {
				ConHero->dZ = 3.0f;
			}
            
			ConHero->dSword = {};
#if 1
			if (Controller->ActionUp.EndedDown) {
				ConHero->dSword = v2{ 0.0f, 1.0f };
				ChangeVolume(GameState->Music, v2{ 1, 1 }, 2.0f);
			}
			else if (Controller->ActionDown.EndedDown) {
				ConHero->dSword = v2{ 0.0f, -1.0f };
				ChangeVolume(GameState->Music, v2{ 0, 0 }, 2.0f);
			}
			else if (Controller->ActionLeft.EndedDown) {
				ChangeVolume(GameState->Music, v2{ 1, 0 }, 2.0f);
				ConHero->dSword = v2{ -1.0f, 0.0f };
			}
			else if (Controller->ActionRight.EndedDown) {
				ChangeVolume(GameState->Music, v2{ 0, 1 }, 2.0f);
				ConHero->dSword = v2{ 1.0f, 0.0f };
			}
#else
			real32 ZoomRate = 0.0f;
			if (Controller->ActionUp.EndedDown) {
				ZoomRate = 1.0f;
			}
			else if (Controller->ActionDown.EndedDown) {
				ZoomRate = -1.0f;
			}
#endif
		}
	}
    
	temporary_memory RenderMemory = BeginTemporaryMemory(&TranState->TranArena);
    
	loaded_bitmap DrawBuffer_ = {};
	loaded_bitmap* DrawBuffer = &DrawBuffer_;
	DrawBuffer->Height = SafeTruncateToUInt16(Buffer->Height);
	DrawBuffer->Width = SafeTruncateToUInt16(Buffer->Width);
    DrawBuffer->Pitch = SafeTruncateToUInt16(Buffer->Pitch);
	DrawBuffer->Memory = Buffer->Memory;
    
	render_group* RenderGroup = AllocateRenderGroup(TranState->Assets, &TranState->TranArena, 
                                                    Megabytes(4), false);
    BeginRender(RenderGroup);
	real32 FocalLength = 0.6f;
	real32 DistanceAboveTarget = 9.0f;
    
	Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, DistanceAboveTarget);
    
    
	Clear(RenderGroup, v4{ 0.25f, 0.25f, 0.25f, 1.0f });
	
	v2 ScreenCenter = v2{ 0.5f * DrawBuffer->Width, 0.5f * DrawBuffer->Height };
    
	rectangle2 ScreenBound = GetCameraRectAtTarget(RenderGroup);
    
	rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBound.Min, 0.0f), V3( ScreenBound.Max, 0.0f));
	CameraBoundsInMeters.Min.z = -2.0f * GameState->TypicalFloorHeight;
	CameraBoundsInMeters.Max.z = 1.0f * GameState->TypicalFloorHeight;
	
    
#if 1
	for (uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; GroundBufferIndex++) {
        
		ground_buffer* GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        
		if (IsValid(GroundBuffer->P)) {
			loaded_bitmap* Bitmap = &GroundBuffer->Bitmap;
			v3 Delta = Subtract(GameState->World, &GroundBuffer->P, &GameState->CameraP);
			if (Delta.z >= -1.0f && Delta.z < 1.0f) {
				PushBitmap(RenderGroup, Bitmap, World->ChunkDimInMeters.y, Delta);
				//PushRectOutline(RenderGroup, Delta, World->ChunkDimInMeters.xy, v4{ 1.0f, 1.0f, 1.0f, 1.0f });
			}
		}
	}
    
	{
		world_position MinChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMinCorner(CameraBoundsInMeters));
		world_position MaxChunkP = MapIntoChunkSpace(World, GameState->CameraP, GetMaxCorner(CameraBoundsInMeters));
        
		for (int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ) {
			for (int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
				for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
					world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
					v3 RelP = Subtract(World, &ChunkCenterP, &GameState->CameraP);
					real32 FurthestBufferLengthSq = 0.0f;
					ground_buffer* FurthestBuffer = 0;
					for (uint32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex) {
						ground_buffer* GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
						if (AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP)) {
							FurthestBuffer = 0;
							break;
						}
						else if (IsValid(GroundBuffer->P)) {
							v3 RelativeP = Subtract(World, &GroundBuffer->P, &GameState->CameraP);
                            
							real32 BufferLengthSq = LengthSq(RelativeP.xy);
							if (FurthestBufferLengthSq < BufferLengthSq) {
								FurthestBufferLengthSq = BufferLengthSq;
								FurthestBuffer = GroundBuffer;
							}
						}
						else {
							FurthestBufferLengthSq = Real32Maximum;
							FurthestBuffer = GroundBuffer;
						}
					}
                    
					if (FurthestBuffer) {
						FillGroundChunk(TranState, GameState, FurthestBuffer, &ChunkCenterP);
					}
				}
			}
		}
	}
#endif
    
	v3 SimBoundsExpansion = { 15.0f, 15.0f, 0 };
	rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
    
	temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    
	world_position SimCenterP = GameState->CameraP;
    
	sim_region* SimRegion = BeginSim(&TranState->TranArena, GameState, GameState->World,
                                     SimCenterP, SimBounds, Input->dtForFrame);
	v3 CameraP = Subtract(World, &GameState->CameraP, &SimCenterP);
    
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(ScreenBound), v4{ 1.0f, 1.0f, 0.0f, 1.0f });
    
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(SimBounds).xy, v4{ 0.0f, 1.0f, 1.0f, 1.0f });
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(SimRegion->Bounds).xy, v4{ 1.0f, 0.0f, 0.0f, 1.0f });
    
	sim_entity* Entity = SimRegion->Entities;
	for (uint32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity) {
		if (Entity->Updatable) {
            
			real32 dt = Input->dtForFrame;
            
			real32 ShadowAlpha = 1.0f - 0.5f * Entity->P.z;
			if (ShadowAlpha < 0) {
				ShadowAlpha = 0.0f;
			}
            
			move_spec MoveSpec = DefaultMoveSpec();
			v3 ddP = {};
            
			v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
			real32 FadeTopEnd = 0.75f * GameState->TypicalFloorHeight;
			real32 FadeTopStart = 0.5f * GameState->TypicalFloorHeight;
            
			real32 FadeBottomStart = -2.0f * GameState->TypicalFloorHeight;
			real32 FadeBottomEnd = -2.25f * GameState->TypicalFloorHeight;
            
			RenderGroup->GlobalAlpha = 1.0f;
            
            
			if (CameraRelativeGroundP.z > FadeTopStart) {
                
				RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeTopStart, CameraRelativeGroundP.z, FadeTopEnd);
                
			}
			else if (CameraRelativeGroundP.z < FadeBottomStart) {
				RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeBottomStart, CameraRelativeGroundP.z, FadeBottomEnd);
			}
            
			// hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[Entity->FacingDirection];
			switch (Entity->Type) {
                case EntityType_Hero: {
                    
                    for (uint32 ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeroes); ++ControlIndex) {
                        controlled_hero* ConHero = GameState->ControlledHeroes + ControlIndex;
                        if (Entity->StorageIndex == ConHero->EntityIndex) {
                            if (ConHero->dZ != 0.0f) {
                                Entity->dP.z = ConHero->dZ;
                            }
                            MoveSpec.UnitMaxAccelVector = true;
                            MoveSpec.Speed = 50.0f;
                            MoveSpec.Drag = 9.0f;
                            ddP = V3(ConHero->ddP, 0);
                            
                            
                            if (ConHero->dSword.x != 0.0f || ConHero->dSword.y != 0.0f) {
                                sim_entity* Sword = Entity->Sword.Ptr;
                                if (Sword && IsSet(Sword, EntityFlag_Nonspatial)) {
                                    Sword->DistanceLimit = 5.0f;
                                    MakeEntitySpatial(Sword, Entity->P, 5.0f * V3(ConHero->dSword, 0));
                                    AddCollisionRule(GameState, Sword->StorageIndex, Entity->StorageIndex, false);
                                    PlaySound(&GameState->AudioState, GetFirstSoundFrom(TranState->Assets, Asset_Bloop));
                                }
                            }
                        }
                    }
                    
                } break;
                
                case EntityType_Sword: {
                    
                    MoveSpec.UnitMaxAccelVector = false;
                    MoveSpec.Speed = 0.0f;
                    MoveSpec.Drag = 0.0f;
                    
                    if (Entity->DistanceLimit == 0.0f) {
                        ClearCollisionRulesFor(GameState, Entity->StorageIndex);
                        MakeEntityNonSpatial(Entity);
                    }
                    
                } break;
                case EntityType_Familiar: {
                    
                    sim_entity* ClosestHero = 0;
                    real32 ClosestHeroSq = Square(10.0f);
                    sim_entity* TestEntity = SimRegion->Entities;
                    
#if 0
                    for (uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
                        if (TestEntity->Type == EntityType_Hero) {
                            real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
                            
                            if (ClosestHeroSq > TestDSq) {
                                ClosestHero = TestEntity;
                                ClosestHeroSq = TestDSq;
                            }
                        }
                    }
#endif
                    if (ClosestHero && (ClosestHeroSq > Square(3.0f))) {
                        real32 Acceleration = 0.5f;
                        real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
                        ddP = OneOverLength * (ClosestHero->P - Entity->P);
                    }
                    
                    MoveSpec.UnitMaxAccelVector = true;
                    MoveSpec.Speed = 50.0f;
                    MoveSpec.Drag = 0.2f;
                }
                
			}
			if (!IsSet(Entity, EntityFlag_Nonspatial) && IsSet(Entity, EntityFlag_Moveable)) {
				MoveEntity(GameState, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
			}			
            
			RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);
			
			asset_vector MatchVector = {};
			MatchVector.E[Tag_FaceDirection] = (real32)Entity->FacingDirection;
			asset_vector WeightVector = {};
			WeightVector.E[Tag_FaceDirection] = 1.0f;
            
			switch (Entity->Type) {
                case EntityType_Hero: {
                    real32 HeroSizeC = 2.5f;
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), HeroSizeC * 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    DrawHitPoints(Entity, RenderGroup);
                    
                    
                    for (u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex) {
                        particle *Particle = GameState->Particle + GameState->NextParticle++;
                        if (GameState->NextParticle >= ArrayCount(GameState->Particle)) {
                            GameState->NextParticle = 0;
                        }
                        
                        Particle->P = v3{RandomBetween(&GameState->EffectEntropy, -0.1f, 0.1f), 0, 0};
                        Particle->dP = v3{ 
                            RandomBetween(&GameState->EffectEntropy, -0.1f, 0.1f), 
                            7.0f * RandomBetween(&GameState->EffectEntropy, 0.7f, 1.0f), 
                            0.0f};
                        Particle->ddP = v3{0, -9.8f, 0};
                        Particle->dColor = v4{0, 0, 0, -0.5f};
                        Particle->Color = v4{
                            RandomBetween(&GameState->EffectEntropy, 0.7f, 1.0f),
                            RandomBetween(&GameState->EffectEntropy, 0.7f, 1.0f),
                            RandomBetween(&GameState->EffectEntropy, 0.7f, 1.0f),
                            1.0f
                        };
                        
                    }
                    
                    ZeroStruct(GameState->ParticleCels);
                    
                    r32 GridScale = 0.5f;
                    v3 GridOrigin = {-0.5f * GridScale * PARTICEL_CEL_DIM, 0, 0};
                    r32 InvGridScale = 1.0f / GridScale;
                    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particle); ++ParticleIndex) {
                        particle *Particle = GameState->Particle + ParticleIndex;
                        
                        v3 P = InvGridScale * (Particle->P - GridOrigin);
                        s32 X = TruncateReal32ToInt32(P.x);
                        s32 Y = TruncateReal32ToInt32(P.y);
                        
                        if (X < 0) {X = 0;}
                        if (X > PARTICEL_CEL_DIM - 1) { X = PARTICEL_CEL_DIM - 1;}
                        if (Y < 0) {Y = 0;}
                        if (Y > PARTICEL_CEL_DIM - 1) { Y = PARTICEL_CEL_DIM - 1;}
                        particle_cel *Cel = &GameState->ParticleCels[Y][X];
                        real32 Density = Particle->Color.a;
                        Cel->Density += Density;
                        Cel->VelocityTimesDensity += Particle->dP * Density;
                    }
                    
                    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameState->Particle); ++ParticleIndex) {
                        particle *Particle = GameState->Particle + ParticleIndex;
                        
                        v3 P = InvGridScale * (Particle->P - GridOrigin);
                        
                        s32 X = TruncateReal32ToInt32(P.x);
                        s32 Y = TruncateReal32ToInt32(P.y);
                        
                        if (X < 1) {X = 1;}
                        if (X > PARTICEL_CEL_DIM - 2) { X = PARTICEL_CEL_DIM - 2;}
                        if (Y < 1) {Y = 1;}
                        if (Y > PARTICEL_CEL_DIM - 2) { Y = PARTICEL_CEL_DIM - 2;}
                        
                        particle_cel *CelCenter = &GameState->ParticleCels[Y][X];
                        particle_cel *CelLeft = &GameState->ParticleCels[Y][X - 1];
                        particle_cel *CelRight = &GameState->ParticleCels[Y][X + 1];
                        particle_cel *CelDown = &GameState->ParticleCels[Y - 1][X];
                        particle_cel *CelUp = &GameState->ParticleCels[Y + 1][X];
                        
                        v3 Dispersion = {};
                        r32 Dc = 0.02f;
                        Dispersion += Dc * (CelCenter->Density - CelLeft->Density) * v3{-1, 0, 0};
                        Dispersion += Dc * (CelCenter->Density - CelRight->Density) * v3{1, 0, 0};
                        Dispersion += Dc * (CelCenter->Density - CelUp->Density) * v3{0, -1, 0};
                        Dispersion += Dc * (CelCenter->Density - CelDown->Density) * v3{0, 1, 0};
                        
                        Particle->ddP += Dispersion;
                        
                        Particle->P += 0.5f * Square(Input->dtForFrame) * Particle->ddP + Particle->dP * Input->dtForFrame;
                        Particle->dP += Input->dtForFrame * Particle->ddP;
                        Particle->Color += Input->dtForFrame * Particle->dColor;
                        
                        if (Particle->P.y < 0) {
                            r32 CoefficientRestitution = 0.3f;
                            Particle->P.y = -Particle->P.y;
                            Particle->dP.y = - CoefficientRestitution * Particle->dP.y;
                        }
                        
                        Particle->ddP += Dispersion;
                        
                        v4 Color;
                        Color.r = Clamp01(Particle->Color.r);
                        Color.g = Clamp01(Particle->Color.g);
                        Color.b = Clamp01(Particle->Color.b);
                        Color.a = Clamp01(Particle->Color.a);
                        
                        if (Color.a > 0.9f) {
                            Color.a = 0.9f * Clamp01MapToRange(1.0f, Color.a, 0.9f);
                        }
                        
                        asset_vector MatchV = {};
                        asset_vector WeightV = {};
                        MatchV.E[Tag_UnicodeCodepoint] = (r32)'R';
                        
                        WeightV.E[Tag_UnicodeCodepoint] = 1.0f;
                        //PushBitmap(RenderGroup, GetBestMatchFontFrom(TranState->Assets, Asset_Font, &MatchV, &WeightV), 0.5f, Particle->P, Color);
                        
                        //PushBitmap(RenderGroup, GetRandomBitmapFrom(TranState->Assets, Asset_Font, &GameState->EffectEntropy), 0.5f, Particle->P, Color);
                        //PushBitmap(RenderGroup, GetRandomBitmapFrom(TranState->Assets, Asset_Head, &GameState->EffectEntropy), 1.0f, Particle->P, Color);
                        //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    }
                    
                } break;
                case EntityType_Wall: {
                    
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Tree), 2.5f, v3{ 0, 0, 0 });
                } break;
                case EntityType_Sword: {
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 0.4f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Sword), 0.5f, v3{ 0, 0, 0 });
                } break;
                case EntityType_Familiar: {
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 0.5f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector), 1.0f, v3{ 0, 0, 0 });
                }
                case EntityType_Monster: {
                    
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector), 1.2f, v3{ 0, 0, 0 });
                    DrawHitPoints(Entity, RenderGroup);
                } break;
                
                case EntityType_Stairwell: {
                    
                    PushRect(RenderGroup, v3{ 0, 0, 0 }, Entity->WalkableDim, v4{ 1.0f, 0.5f, 0, 1.0f });
                    PushRect(RenderGroup, v3{ 0, 0, Entity->WalkableHeight }, Entity->WalkableDim, v4{ 1.0f, 1.0f, 0, 1.0f });
                    
                    //PushBitmap(&PieceGroup, &GameState->Stairwell, v2{ 0, 0 }, 0, v2{ 37, 37 });
                } break;
                case EntityType_Space: {
#if 0
                    for (uint32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                        sim_entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                        PushRectOutline(RenderGroup, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, v4{ 0, 0.5f, 1.0f, 1.0f });
                    }
#endif
                } break;
                default: {
                    InvalidCodePath
                } break;
            }
        }
    }
    RenderGroup->GlobalAlpha = 1.0;
    
    
#if 0
    GameState->time += Input->dtForFrame;
    
    real32 Angle = GameState->time;
    v2 Origin = ScreenCenter;
    v2 Offset = v2{ 0.5f + 10.0f  * (Sin(GameState->time) + 0.5f), 0 };
    v2 AxisX = 200 * v2{ Sin(GameState->time), Cos(GameState->time) };
    v2 AxisY = Perp(AxisX);
    v4 Color = v4{ 1.0f, 1.0f, 1.0f, 1.0f };
    
    v3 MapColor[] = {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1}
    };
    
    for (uint32 MapIndex = 0;
         MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
        environment_map* Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap* LOD = Map->LOD + 0;
        bool32 RowCheckerOn = false;
        int32 CheckerWidth = 16;
        int32 CheckerHeight = 16;
        for (int32 Y = 0; Y < LOD->Height; Y += CheckerHeight) {
            bool32 CheckerOn = RowCheckerOn;
            for (int32 X = 0; X < LOD->Width; X += CheckerWidth) {
                v4 Color = CheckerOn ? ToV4(MapColor[MapIndex], 1.0f) : v4{ 0, 0, 0, 1.0f };
                v2 MinP = V2i(X, Y);
                v2 MaxP = MinP + V2i(CheckerWidth, CheckerHeight);
                DrawRectangle(LOD, MinP, MaxP, Color);
                CheckerOn = !CheckerOn;
            }
            RowCheckerOn = !RowCheckerOn;
        }
    }
    
    TranState->EnvMaps[0].Pz = -1.5f;
    TranState->EnvMaps[1].Pz = 0.0f;
    TranState->EnvMaps[2].Pz = 1.5f;
    
    CoordinateSystem(RenderGroup, Origin + Offset - 0.5f * v2{ AxisX.x, AxisY.y }, AxisX, AxisY, Color, 
                     &GameState->TestDiffuse, &GameState->TestNormal, TranState->EnvMaps + 2, TranState->EnvMaps + 1, TranState->EnvMaps + 0);
    v2 MapP = { 0.0f, 0.0f };
    for (uint32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
        environment_map* Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap* LOD = Map->LOD + 0;
        
        v2 XAxis = 0.5f * v2{ (real32)LOD->Width, 0 };
        v2 YAxis = 0.5f * v2{ 0, (real32)LOD->Height };
        CoordinateSystem(RenderGroup, MapP, XAxis, YAxis, v4{ 1.0f, 1.0f, 1.0f, 1.0f },
                         LOD, 0, 0, 0, 0);
        MapP += YAxis + v2{ 0.0f, 6.0f };
    }
    
#endif
    rectangle2i d = {4, 4, 120, 120};
    
    TiledRenderGroupToOutput(TranState->HighPriorityQueue, RenderGroup, DrawBuffer);
    EndRender(RenderGroup);
    
    EndSim(SimRegion, GameState);
    EndTemporaryMemory(SimMemory);
    EndTemporaryMemory(RenderMemory);
    //EvictAssetsAsNecessary(TranState->Assets);
    CheckArena(&TranState->TranArena);
    CheckArena(&GameState->WorldArena);
    END_TIMED_BLOCK(GameUpdateAndRender);
    OverlayCycleCounters(Memory);
    
    if (DEBUGRenderGroup) {
        TiledRenderGroupToOutput(TranState->HighPriorityQueue, DEBUGRenderGroup, DrawBuffer);
        EndRender(DEBUGRenderGroup);
        
    }
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
    game_state* GameState = (game_state*)Memory->PermanentStorage;
    transient_state* TranState = (transient_state*)Memory->TransientStorage;
    OutputPlayingSounds(&GameState->AudioState, SoundBuffer, TranState->Assets, &TranState->TranArena);
}