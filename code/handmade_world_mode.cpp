struct add_low_entity_result {
	u32 LowIndex;
	low_entity* Low;
};

inline world_position
ChunkPositionFromTilePosition(world* World, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ, v3 AdditionalOffset = v3{ 0, 0, 0 }) {
	world_position BasePos = {};
	
	// @Note(ren): remove
	r32 TileSideInMeters = 1.4f;
	r32 TileDepthInMeters = 3.0f;
    
	v3 TileDim = v3{ TileSideInMeters , TileSideInMeters , TileDepthInMeters };
    
	v3 Offset = Hadamard(TileDim, v3{ (r32)AbsTileX, (r32)AbsTileY, (r32)AbsTileZ });
    
	world_position Result = MapIntoChunkSpace(World, BasePos, Offset + AdditionalOffset);
    
	return(Result);
}


inline add_low_entity_result
AddLowEntity(game_mode_world* GameWorld, entity_type Type, world_position P) {
	Assert(GameWorld->LowEntityCount < ArrayCount(GameWorld->LowEntities));
    
	u32 EntityIndex = GameWorld->LowEntityCount++;
	low_entity* LowEntity = GameWorld->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Sim.Type = Type;
	LowEntity->Sim.Collision = GameWorld->NullCollision;
	LowEntity->P = NullPosition();
    ChangeEntityLocation(&GameWorld->World->Arena, GameWorld->World, EntityIndex, LowEntity, P);
    
	add_low_entity_result Result = {};
	Result.Low = LowEntity;
	Result.LowIndex = EntityIndex;
	return(Result);
}

inline void
DeleteLowEntity(game_state* GameState, u32 Index) {
    // TODO(NAME): 
}

inline add_low_entity_result
AddGroundedEntity(game_mode_world* GameWorld, entity_type Type, world_position P, sim_entity_collision_volume_group * Collision) {
	add_low_entity_result Entity = AddLowEntity(GameWorld, Type, P);
	Entity.Low->Sim.Collision = Collision;
	return(Entity);
}


internal void
InitHitPoints(low_entity* EntityLow, u32 HitPointCount) {
	Assert(HitPointCount <= ArrayCount(EntityLow->Sim.HitPoint));
	EntityLow->Sim.HitPointMax = HitPointCount;
	for (u32 HitPointIndex = 0; HitPointIndex < EntityLow->Sim.HitPointMax; ++HitPointIndex) {
		hit_point* HitPoint = EntityLow->Sim.HitPoint + HitPointIndex;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
		HitPoint->Flag = 0;
	}
}

internal add_low_entity_result
AddStandardRoom(game_mode_world* GameWorld, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameWorld->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameWorld, EntityType_Space, P);
	Entity.Low->Sim.Collision = GameWorld->StandardRoomCollision;
	AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);
	return(Entity);
}

internal add_low_entity_result
AddSword(game_mode_world *GameWorld) {
	add_low_entity_result Entity = AddLowEntity(GameWorld, EntityType_Sword, NullPosition());
	Entity.Low->Sim.Collision = GameWorld->SwordCollision;
    
	AddFlags(&Entity.Low->Sim, EntityFlag_Moveable);
	return(Entity);
}

internal add_low_entity_result
AddPlayer(game_mode_world *GameWorld) {
	world_position P = GameWorld->CameraP;
	add_low_entity_result Entity = AddGroundedEntity(GameWorld, EntityType_Hero, P, GameWorld->PlayerCollision);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	InitHitPoints(Entity.Low, 3);
    
	add_low_entity_result Sword = AddSword(GameWorld);
	Entity.Low->Sim.Sword.Index = Sword.LowIndex;
    
    
	if (GameWorld->CameraFollowingEntityIndex == 0) {
		GameWorld->CameraFollowingEntityIndex = Entity.LowIndex;
	}
	return(Entity);
}

internal add_low_entity_result
AddStair(game_mode_world* GameWorld, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameWorld->World, AbsTileX, AbsTileY, AbsTileZ);
	
	add_low_entity_result Entity = AddGroundedEntity(GameWorld, EntityType_Stairwell, P, GameWorld->StairCollision);
	Entity.Low->Sim.WalkableHeight = GameWorld->TypicalFloorHeight;
	Entity.Low->Sim.WalkableDim = Entity.Low->Sim.Collision->TotalVolume.Dim.xy;
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
	return(Entity);
}

internal add_low_entity_result
AddWall(game_mode_world* GameWorld, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameWorld->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameWorld, EntityType_Wall, P, GameWorld->WallCollision);
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides);
	return(Entity);
}

internal add_low_entity_result
AddMonster(game_mode_world* GameWorld, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameWorld->World, AbsTileX, AbsTileY, AbsTileZ);
	
	add_low_entity_result Entity = AddGroundedEntity(GameWorld, EntityType_Monster, P, GameWorld->MonstarCollision);
	
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	
	InitHitPoints(Entity.Low, 2);
	return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_mode_world* GameWorld, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameWorld->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddGroundedEntity(GameWorld, EntityType_Familiar, P, GameWorld->FamiliarCollision);
	AddFlags(&Entity.Low->Sim, EntityFlag_Collides | EntityFlag_Moveable);
	return(Entity);
}

internal void
DrawHitPoints(sim_entity* Entity, render_group* PieceGroup) {
	if (Entity->HitPointMax >= 1) {
		v2 HealthDim = { 0.2f, 0.2f };
		r32 SpacingX = 1.5f * HealthDim.x;
		v2 HitP = { -0.5f * (Entity->HitPointMax - 1) * SpacingX, -0.2f };
		v2 dHitP = { SpacingX, 0.0f };
		for (u32 HealthIndex = 0; HealthIndex < Entity->HitPointMax; ++HealthIndex) {
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
MakeSimpleGroundedCollision(game_mode_world* GameWorld, r32 DimX, r32 DimY, r32 DimZ) {
	sim_entity_collision_volume_group* Group = PushStruct(&GameWorld->World->Arena, sim_entity_collision_volume_group);
	Group->VolumeCount = 1;
	Group->Volumes = PushArray(&GameWorld->World->Arena, 1, sim_entity_collision_volume);
	Group->TotalVolume.Dim = v3{ DimX, DimY, DimZ };
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0.5f * DimZ };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal sim_entity_collision_volume_group*
MakeNullCollision(game_mode_world* GameWorld) {
	sim_entity_collision_volume_group* Group = PushStruct(&GameWorld->World->Arena, sim_entity_collision_volume_group);
	Group->VolumeCount = 0;
	Group->Volumes = 0;
	Group->TotalVolume.Dim = v3{ 0, 0, 0};
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0 };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal void
ClearCollisionRulesFor(game_mode_world* GameWorld, u32 StorageIndex) {
	for (u32 HashBucket = 0; HashBucket < ArrayCount(GameWorld->CollisionRuleHash); ++HashBucket) {
		for (pairwise_collision_rule** Rule = &GameWorld->CollisionRuleHash[HashBucket]; *Rule;) {
			if ((*Rule)->StorageIndexA == StorageIndex || (*Rule)->StorageIndexB == StorageIndex) {
				pairwise_collision_rule* RemovedRule = *Rule;
				*Rule = (*Rule)->NextHash;
                
				RemovedRule->NextHash = GameWorld->FirstFreeCollisionRule;
				GameWorld->FirstFreeCollisionRule = RemovedRule;
			}
			else {
				Rule = &(*Rule)->NextHash;
			}
		}
	}
    
}

internal void
AddCollisionRule(game_mode_world* GameWorld, u32 StorageIndexA, u32 StorageIndexB, b32 CanCollide) {
	if (StorageIndexA > StorageIndexB) {
		u32 Temp = StorageIndexA;
		StorageIndexA = StorageIndexB;
		StorageIndexB = Temp;
	}
	pairwise_collision_rule* Found = 0;
	u32 HashBucket = StorageIndexA & (ArrayCount(GameWorld->CollisionRuleHash) - 1);
	for (pairwise_collision_rule* Rule = GameWorld->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
		if (Rule->StorageIndexA == StorageIndexA && Rule->StorageIndexB == StorageIndexB) {
			Found = Rule;
            
			break;
		}
	}
    
    
	if (!Found) {
		Found = GameWorld->FirstFreeCollisionRule;
		if (Found) {
			GameWorld->FirstFreeCollisionRule = Found->NextHash;
		}
		else {
			Found = PushStruct(&GameWorld->World->Arena, pairwise_collision_rule);
		}
		Found->NextHash = GameWorld->CollisionRuleHash[HashBucket];
		GameWorld->CollisionRuleHash[HashBucket] = Found;
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
    game_mode_world* GameWorld;
    ground_buffer* GroundBuffer;
    world_position ChunkP;
	task_with_memory* Task;
	
};

PLATFORM_WORK_QUEUE_CALLBACK(FillGroundChunkWork) {
	fill_ground_chunk_work* Work = (fill_ground_chunk_work*)Data;
    
#if 0    
    loaded_bitmap* Buffer = &Work->GroundBuffer->Bitmap;
    Buffer->AlignPercentage = v2{ 0.5f, 0.5f };
    Buffer->WidthOverHeight = 1.0f;
    r32 Width = Work->GameState->World->ChunkDimInMeters.x;
    r32 Height =Work->GameState->World->ChunkDimInMeters.y;
    render_group* RenderGroup = AllocateRenderGroup(Work->TranState->Assets, &Work->Task->Arena, 0, true);
    BeginRender(RenderGroup);
    Orthographic(RenderGroup, Buffer->Width, Buffer->Height, (Buffer->Width - 2) / Width);
    Clear(RenderGroup, v4{ 1.0f, 1.0f, 0.0f, 1.0f });
    
    v2 HalfDim = 0.5f * V2(Width, Height);
    HalfDim = 2.0f * HalfDim;
    for (s32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
        for (s32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
            
            s32 ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            s32 ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            s32 ChunkZ = Work->ChunkP.ChunkZ;
            random_series Series = RandomSeed(139 * ChunkX + 593 * ChunkY + 329 * ChunkZ);
            
            v2 Center = v2{ ChunkOffsetX * Width, ChunkOffsetY * Height };
            
            for (u32 GrassIndex = 0; GrassIndex < 20; GrassIndex++) {
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets, RandomChoice(&Series, 2) ? Asset_Grass : Asset_Ground, &Series);
                v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 4.0f, V3(P, 0.0f));
            }
        }
    }
    for (s32 ChunkOffsetY = -1; ChunkOffsetY <= 1; ++ChunkOffsetY) {
        for (s32 ChunkOffsetX = -1; ChunkOffsetX <= 1; ++ChunkOffsetX) {
            
            s32 ChunkX = Work->ChunkP.ChunkX + ChunkOffsetX;
            s32 ChunkY = Work->ChunkP.ChunkY + ChunkOffsetY;
            s32 ChunkZ = Work->ChunkP.ChunkZ;
            random_series Series = RandomSeed(139 * ChunkX + 593 * ChunkY + 329 * ChunkZ);
            
            v2 Center = v2{ ChunkOffsetX * Width, ChunkOffsetY * Height };
            
            for (u32 GrassIndex = 0; GrassIndex < 20; GrassIndex++) {
                
                bitmap_id Stamp = GetRandomBitmapFrom(Work->TranState->Assets, Asset_Tuft, &Series);
                
                v2 P = Center + Hadamard(HalfDim, V2(RandomBilateral(&Series), RandomBilateral(&Series)));
                PushBitmap(RenderGroup, Stamp, 0.4f, V3(P, 0.0f));
            }
        }
    }
    Assert(AllResoucePresent(RenderGroup));
	RenderGroupToOutput(RenderGroup, Buffer);
    EndRender(RenderGroup);
#endif
    
	EndTaskWithMemory(Work->Task);
}

internal void
FillGroundChunk(transient_state *TranState, game_mode_world* GameWorld, ground_buffer* GroundBuffer, world_position *ChunkP) {
	task_with_memory* AvailableTask = BeginTaskWithMemory(TranState);
	if (AvailableTask) {
		fill_ground_chunk_work* Work = PushStruct(&AvailableTask->Arena, fill_ground_chunk_work);
		Work->Task = AvailableTask;
		
		Work->TranState = TranState;
        Work->GameWorld = GameWorld;
        Work->GroundBuffer = GroundBuffer;
		Work->ChunkP = *ChunkP;
        GroundBuffer->P = *ChunkP;
        Platform.AddEntry(TranState->LowPriorityQueue, FillGroundChunkWork, Work);
	}
}


inline rectangle2
GetCameraRectAtDistance(render_group* RenderGroup, r32 CameraDistance) {
	v2 HalfDimOnTarget = (CameraDistance / RenderGroup->Transform.FocalLength) * RenderGroup->MonitorHalfDimInMeters;
	rectangle2 Result = RectCenterHalfDim(v2{ 0, 0 }, HalfDimOnTarget);
	return(Result);
    
}

inline rectangle2
GetCameraRectAtTarget(render_group* RenderGroup) {
	rectangle2 Result = GetCameraRectAtDistance(RenderGroup, RenderGroup->Transform.DistanceAboveTarget);
	return(Result);
}



internal void
EnterWorld(game_state *GameState) {
    
    SetGameMode(GameState, GameMode_World);
    game_mode_world *GameWorld = PushStruct(&GameState->ModeArena, game_mode_world);
    
    
    GameWorld->TypicalFloorHeight = 3.0f;
    
    r32 PixelsToMeters = 1.0f / 42.0f;
    v3 WorldChunkDimMeters = { PixelsToMeters * GroundBufferWidth, PixelsToMeters * GroundBufferHeight, GameWorld->TypicalFloorHeight };
    GameWorld->World = PushStruct(&GameState->ModeArena, world);
    world* World = GameWorld->World;
    InitializeWorld(World, WorldChunkDimMeters, &GameState->ModeArena);
    
    s32 TileSideInPixels = 60;
    
    AddLowEntity(GameWorld, EntityType_Null, NullPosition());
    u32 TilesPerWidth = 17;
    u32 TilesPerHeight = 9;
    
    r32 TileSideInMeters = 1.4f;
    r32 TileDepthInMeters = GameWorld->TypicalFloorHeight;
    
    GameWorld->NullCollision = MakeSimpleGroundedCollision(GameWorld, 0, 0, 0);
    GameWorld->SwordCollision = MakeSimpleGroundedCollision(GameWorld, 1.0f, 0.5f, 0.1f);
    GameWorld->StairCollision = MakeSimpleGroundedCollision(GameWorld, 
                                                            TileSideInMeters, 2.0f * TileSideInMeters, 1.1f * TileDepthInMeters);
    GameWorld->PlayerCollision = MakeSimpleGroundedCollision(GameWorld, 1.0f, 0.5f, 1.2f);
    GameWorld->MonstarCollision = MakeSimpleGroundedCollision(GameWorld, 1.0f, 0.5f, 0.5f);
    GameWorld->WallCollision = MakeSimpleGroundedCollision(GameWorld, TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    GameWorld->StandardRoomCollision = MakeSimpleGroundedCollision(GameWorld, TilesPerWidth * TileSideInMeters, TilesPerHeight * TileSideInMeters, 0.9f * TileDepthInMeters);
    GameWorld->FamiliarCollision = MakeSimpleGroundedCollision(GameWorld, 1.0f, 0.5f, 0.5f);
    
    random_series Series = RandomSeed(1234);
    GameWorld->EffectEntropy = RandomSeed(5456);
    
    
    u32 ScreenBaseX = 0;
    u32 ScreenBaseY = 0;
    u32 ScreenBaseZ = 0;
    u32 ScreenX = ScreenBaseX;
    u32 ScreenY = ScreenBaseY;
    u32 AbsTileZ = ScreenBaseZ;
    
    b32 DoorLeft = false;
    b32 DoorRight = false;
    b32 DoorTop = false;
    b32 DoorBottom = false;
    b32 DoorUp = false;
    b32 DoorDown = false;
    
    for (u32 ScreenIndex = 0; ScreenIndex < 30; ++ScreenIndex) {
        
#if 1
        u32 DoorDirection = RandomChoice(&Series, (DoorUp || DoorDown)? 2: 4);
#else
        u32 DoorDirection = RandomChoice(&Series, 2);
#endif
        
        b32 CreatedZDoor = false;
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
        AddStandardRoom(GameWorld, 
                        (ScreenX * TilesPerWidth + TilesPerWidth / 2), 
                        (ScreenY * TilesPerHeight + TilesPerHeight / 2), 
                        AbsTileZ);
        
        
        for (u32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
            for (u32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
                u32 AbsTileX = ScreenX * TilesPerWidth + TileX;
                u32 AbsTileY = ScreenY * TilesPerHeight + TileY;
                
                b32 ShouldBeDoor = false;
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
                    
                    AddWall(GameWorld, AbsTileX, AbsTileY, AbsTileZ);
                    
                    //}
                } else if (CreatedZDoor) {
                    if (((AbsTileZ % 2) && (TileX == 10) && (TileY == 5)) ||
                        (!(AbsTileZ % 2) && (TileX == 4) && (TileY == 5)))
                    {
                        AddStair(GameWorld, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
                    }
                    /**if (TileY == 5 && TileX == 10) {
                        AddStair(GameWorld, AbsTileX, AbsTileY, DoorDown? AbsTileZ - 1: AbsTileZ);
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
    u32 CameraTileX = ScreenBaseX * TilesPerWidth + 17 / 2;
    u32 CameraTileY = ScreenBaseY * TilesPerHeight + 9 / 2;
    u32 CameraTileZ = ScreenBaseZ;
    NewCameraP =
        ChunkPositionFromTilePosition(GameWorld->World, CameraTileX, CameraTileY, CameraTileZ);
    GameWorld->CameraP = NewCameraP;
    AddMonster(GameWorld, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
    AddFamiliar(GameWorld, CameraTileX - 2, CameraTileY - 2, CameraTileZ);
}



internal void
UpdateAndRenderWorld(game_state *GameState, game_mode_world *GameWorld, transient_state *TranState, game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
{
    
    world *World = GameWorld->World;
    v2 MouseP = {Input->MouseX, Input->MouseY};
    r32 FocalLength = 0.6f;
	r32 DistanceAboveTarget = 9.0f;
    
	Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, DistanceAboveTarget);
    Clear(RenderGroup, v4{ 0.25f, 0.25f, 0.25f, 1.0f });
	
	v2 ScreenCenter = v2{ 0.5f * DrawBuffer->Width, 0.5f * DrawBuffer->Height };
    
	rectangle2 ScreenBound = GetCameraRectAtTarget(RenderGroup);
    
	rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBound.Min, 0.0f), V3( ScreenBound.Max, 0.0f));
	CameraBoundsInMeters.Min.z = -2.0f * GameWorld->TypicalFloorHeight;
	CameraBoundsInMeters.Max.z = 1.0f * GameWorld->TypicalFloorHeight;
	
    
#if 1
	for (u32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; GroundBufferIndex++) {
        
		ground_buffer* GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
        
		if (IsValid(GroundBuffer->P)) {
			loaded_bitmap* Bitmap = &GroundBuffer->Bitmap;
			v3 Delta = Subtract(GameWorld->World, &GroundBuffer->P, &GameWorld->CameraP);
			if (Delta.z >= -1.0f && Delta.z < 1.0f) {
				PushBitmap(RenderGroup, Bitmap, World->ChunkDimInMeters.y, Delta);
                DEBUG_IF(GroundChunk_ShowGroundChunkOutlines)
                {
                    PushRectOutline(RenderGroup, Delta, World->ChunkDimInMeters.xy, v4{ 1.0f, 1.0f, 1.0f, 1.0f });
                }
			}
		}
	}
    
	{
		world_position MinChunkP = MapIntoChunkSpace(World, GameWorld->CameraP, GetMinCorner(CameraBoundsInMeters));
		world_position MaxChunkP = MapIntoChunkSpace(World, GameWorld->CameraP, GetMaxCorner(CameraBoundsInMeters));
        
		for (s32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ) {
			for (s32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
				for (s32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
					world_position ChunkCenterP = CenteredChunkPoint(ChunkX, ChunkY, ChunkZ);
					v3 RelP = Subtract(World, &ChunkCenterP, &GameWorld->CameraP);
					r32 FurthestBufferLengthSq = 0.0f;
					ground_buffer* FurthestBuffer = 0;
					for (u32 GroundBufferIndex = 0; GroundBufferIndex < TranState->GroundBufferCount; ++GroundBufferIndex) {
						ground_buffer* GroundBuffer = TranState->GroundBuffers + GroundBufferIndex;
						if (AreInSameChunk(World, &GroundBuffer->P, &ChunkCenterP)) {
							FurthestBuffer = 0;
							break;
						}
						else if (IsValid(GroundBuffer->P)) {
							v3 RelativeP = Subtract(World, &GroundBuffer->P, &GameWorld->CameraP);
                            
							r32 BufferLengthSq = LengthSq(RelativeP.xy);
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
						FillGroundChunk(TranState, GameWorld, FurthestBuffer, &ChunkCenterP);
					}
				}
			}
		}
	}
#endif
    
	v3 SimBoundsExpansion = { 15.0f, 15.0f, 0 };
	rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
    
	temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    
	world_position SimCenterP = GameWorld->CameraP;
    
	sim_region* SimRegion = BeginSim(&TranState->TranArena, GameWorld, GameWorld->World,
                                     SimCenterP, SimBounds, Input->dtForFrame);
	v3 CameraP = Subtract(World, &GameWorld->CameraP, &SimCenterP);
    
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(ScreenBound), v4{ 1.0f, 1.0f, 0.0f, 1.0f });
    
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(SimBounds).xy, v4{ 0.0f, 1.0f, 1.0f, 1.0f });
	PushRectOutline(RenderGroup, v3{ 0, 0, 0 }, GetDim(SimRegion->Bounds).xy, v4{ 1.0f, 0.0f, 0.0f, 1.0f });
    
	sim_entity* Entity = SimRegion->Entities;
	for (u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity) {
		if (Entity->Updatable) {
            
			r32 dt = Input->dtForFrame;
            
			r32 ShadowAlpha = 1.0f - 0.5f * Entity->P.z;
			if (ShadowAlpha < 0) {
				ShadowAlpha = 0.0f;
			}
            
			move_spec MoveSpec = DefaultMoveSpec();
			v3 ddP = {};
            
			v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
			r32 FadeTopEnd = 0.75f * GameWorld->TypicalFloorHeight;
			r32 FadeTopStart = 0.5f * GameWorld->TypicalFloorHeight;
            
			r32 FadeBottomStart = -2.0f * GameWorld->TypicalFloorHeight;
			r32 FadeBottomEnd = -2.25f * GameWorld->TypicalFloorHeight;
            
			RenderGroup->GlobalAlpha = 1.0f;
            
            
			if (CameraRelativeGroundP.z > FadeTopStart) {
                
				RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeTopStart, CameraRelativeGroundP.z, FadeTopEnd);
                
			}
			else if (CameraRelativeGroundP.z < FadeBottomStart) {
				RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeBottomStart, CameraRelativeGroundP.z, FadeBottomEnd);
			}
            
            //hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[Entity->FacingDirection];
			switch (Entity->Type) {
                case EntityType_Hero: {
                    
                    for (u32 ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeroes); ++ControlIndex) {
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
                                    AddCollisionRule(GameWorld, Sword->StorageIndex, Entity->StorageIndex, false);
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
                        ClearCollisionRulesFor(GameWorld, Entity->StorageIndex);
                        MakeEntityNonSpatial(Entity);
                    }
                } break;
                case EntityType_Familiar: {
                    
                    sim_entity* ClosestHero = 0;
                    r32 ClosestHeroSq = Square(10.0f);
                    sim_entity* TestEntity = SimRegion->Entities;
                    
                    DEBUG_IF(Sim_FamiliarFollowsHero)
                    {
                        for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
                            if (TestEntity->Type == EntityType_Hero) {
                                r32 TestDSq = LengthSq(TestEntity->P - Entity->P);
                                
                                if (ClosestHeroSq > TestDSq) {
                                    ClosestHero = TestEntity;
                                    ClosestHeroSq = TestDSq;
                                }
                            }
                        }
                    }
                    
                    if (ClosestHero && (ClosestHeroSq > Square(3.0f))) {
                        r32 Acceleration = 0.5f;
                        r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
                        ddP = OneOverLength * (ClosestHero->P - Entity->P);
                    }
                    
                    MoveSpec.UnitMaxAccelVector = true;
                    MoveSpec.Speed = 50.0f;
                    MoveSpec.Drag = 0.2f;
                }
			}
			if (!IsSet(Entity, EntityFlag_Nonspatial) && IsSet(Entity, EntityFlag_Moveable)) {
				MoveEntity(GameWorld, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
			}			
            
			RenderGroup->Transform.OffsetP = GetEntityGroundPoint(Entity);
			
			asset_vector MatchVector = {};
			MatchVector.E[Tag_FaceDirection] = (r32)Entity->FacingDirection;
			asset_vector WeightVector = {};
			WeightVector.E[Tag_FaceDirection] = 1.0f;
            
			switch (Entity->Type) {
                case EntityType_Hero: {
                    r32 HeroSizeC = 2.5f;
                    PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), HeroSizeC * 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    PushBitmap(RenderGroup, GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, 0, 0 });
                    DrawHitPoints(Entity, RenderGroup);
                    DEBUG_IF(Particle_Demo)
                    {
                        for (u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex) {
                            particle *Particle = GameWorld->Particle + GameWorld->NextParticle++;
                            if (GameWorld->NextParticle >= ArrayCount(GameWorld->Particle)) {
                                GameWorld->NextParticle = 0;
                            }
                            
                            Particle->P = v3{RandomBetween(&GameWorld->EffectEntropy, -0.1f, 0.1f), 0, 0};
                            Particle->dP = v3{ 
                                RandomBetween(&GameWorld->EffectEntropy, -0.1f, 0.1f), 
                                7.0f * RandomBetween(&GameWorld->EffectEntropy, 0.7f, 1.0f), 
                                0.0f};
                            Particle->ddP = v3{0, -9.8f, 0};
                            Particle->dColor = v4{0, 0, 0, -0.5f};
                            Particle->Color = v4{
                                RandomBetween(&GameWorld->EffectEntropy, 0.7f, 1.0f),
                                RandomBetween(&GameWorld->EffectEntropy, 0.7f, 1.0f),
                                RandomBetween(&GameWorld->EffectEntropy, 0.7f, 1.0f),
                                1.0f
                            };
                            
                        }
                        
                        ZeroStruct(GameWorld->ParticleCels);
                        
                        r32 GridScale = 0.5f;
                        v3 GridOrigin = {-0.5f * GridScale * PARTICEL_CEL_DIM, 0, 0};
                        r32 InvGridScale = 1.0f / GridScale;
                        for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameWorld->Particle); ++ParticleIndex) {
                            particle *Particle = GameWorld->Particle + ParticleIndex;
                            
                            v3 P = InvGridScale * (Particle->P - GridOrigin);
                            s32 X = TruncateReal32ToInt32(P.x);
                            s32 Y = TruncateReal32ToInt32(P.y);
                            
                            if (X < 0) {X = 0;}
                            if (X > PARTICEL_CEL_DIM - 1) { X = PARTICEL_CEL_DIM - 1;}
                            if (Y < 0) {Y = 0;}
                            if (Y > PARTICEL_CEL_DIM - 1) { Y = PARTICEL_CEL_DIM - 1;}
                            particle_cel *Cel = &GameWorld->ParticleCels[Y][X];
                            r32 Density = Particle->Color.a;
                            Cel->Density += Density;
                            Cel->VelocityTimesDensity += Particle->dP * Density;
                        }
                        DEBUG_IF(Particle_Grid)
                        {
                            for(u32 Y = 0; Y < PARTICEL_CEL_DIM; ++Y) {
                                for (u32 X = 0; X < PARTICEL_CEL_DIM; ++X) {
                                    particle_cel *Cel = &GameWorld->ParticleCels[Y][X];
                                    r32 Alpha = Clamp01(0.1f * Cel->Density);
                                    PushRect(RenderGroup, GridScale * v3{(r32)X, (r32)Y, 0} + GridOrigin, GridScale * v2{1.0f, 1.0f}, v4{Alpha, Alpha, Alpha, 1.0f});
                                }
                            }
                        }
                        
                        for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(GameWorld->Particle); ++ParticleIndex) {
                            particle *Particle = GameWorld->Particle + ParticleIndex;
                            
                            v3 P = InvGridScale * (Particle->P - GridOrigin);
                            
                            s32 X = TruncateReal32ToInt32(P.x);
                            s32 Y = TruncateReal32ToInt32(P.y);
                            
                            if (X < 1) {X = 1;}
                            if (X > PARTICEL_CEL_DIM - 2) { X = PARTICEL_CEL_DIM - 2;}
                            if (Y < 1) {Y = 1;}
                            if (Y > PARTICEL_CEL_DIM - 2) { Y = PARTICEL_CEL_DIM - 2;}
                            
                            particle_cel *CelCenter = &GameWorld->ParticleCels[Y][X];
                            particle_cel *CelLeft = &GameWorld->ParticleCels[Y][X - 1];
                            particle_cel *CelRight = &GameWorld->ParticleCels[Y][X + 1];
                            particle_cel *CelDown = &GameWorld->ParticleCels[Y - 1][X];
                            particle_cel *CelUp = &GameWorld->ParticleCels[Y + 1][X];
                            
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
                            PushBitmap(RenderGroup, GetRandomBitmapFrom(TranState->Assets, Asset_Head, &GameWorld->EffectEntropy), 1.0f, Particle->P, Color);
                            //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                            
                        }
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
                    
                    DEBUG_IF(Renderer_Show_Space_Outline)
                    {
                        for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                            sim_entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                            PushRectOutline(RenderGroup, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, v4{ 0, 0.5f, 1.0f, 1.0f });
                        }
                    }
                } break;
                default: {
                    InvalidCodePath
                } break;
                
            }
            if (DEBUG_UI_ENABLED) {
                debug_id EntityDebugID = DEBUG_POINTER_ID(GameWorld->LowEntities + Entity->StorageIndex);
                for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                    sim_entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                    v3 WorldMousePoint = Unproject(RenderGroup, MouseP);
#if 0
                    PushRect(RenderGroup, V3(WorldMousePoint, 0.0f), v2{1.0f, 1.0f}, v4{1.0f, 0.0f, 1.0f, 1.0f});
#endif
                    if ((WorldMousePoint.x > -0.5f * Volume->Dim.x && WorldMousePoint.x < 0.5f * Volume->Dim.x) &&
                        (WorldMousePoint.y > -0.5f * Volume->Dim.y && WorldMousePoint.y < 0.5f * Volume->Dim.y)){
                        
                        DEBUG_HIT(EntityDebugID, WorldMousePoint.z);
                    }
                    v4 EntityOutlineColor;
                    if (DEBUG_HIGHLIGHTED(EntityDebugID, &EntityOutlineColor)) {
                        PushRectOutline(RenderGroup, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, EntityOutlineColor, 0.05f);
                    }
                    
                }
                if (DEBUG_REQUESTED(EntityDebugID)) {
                    DEBUG_BEGIN_DATA_BLOCK(Simulation_Entity, EntityDebugID);
                    DEBUG_VALUE(Entity->P);
                    DEBUG_VALUE(Entity->dP);
                    DEBUG_VALUE(Entity->FacingDirection);
                    DEBUG_VALUE(Entity->WalkableDim);
                    DEBUG_VALUE(Entity->WalkableHeight);
                    DEBUG_VALUE(Entity->StorageIndex);
                    
                    DEBUG_END_DATA_BLOCK();
                }
            }
        }
    }
    RenderGroup->GlobalAlpha = 1.0;
    
    
#if 0
    GameState->time += Input->dtForFrame;
    
    r32 Angle = GameState->time;
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
    
    for (u32 MapIndex = 0;
         MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
        environment_map* Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap* LOD = Map->LOD + 0;
        b32 RowCheckerOn = false;
        s32 CheckerWidth = 16;
        s32 CheckerHeight = 16;
        rectangle2i ClipRect = {0, 0, LOD->Width, LOD->Height};
        for (s32 Y = 0; Y < LOD->Height; Y += CheckerHeight) {
            b32 CheckerOn = RowCheckerOn;
            for (s32 X = 0; X < LOD->Width; X += CheckerWidth) {
                v4 CheckColor = CheckerOn ? ToV4(MapColor[MapIndex], 1.0f) : v4{ 0, 0, 0, 1.0f };
                v2 MinP = V2i(X, Y);
                v2 MaxP = MinP + V2i(CheckerWidth, CheckerHeight);
                DrawRectangle(LOD, MinP, MaxP, CheckColor, ClipRect, true);
                DrawRectangle(LOD, MinP, MaxP, CheckColor, ClipRect, false);
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
    for (u32 MapIndex = 0; MapIndex < ArrayCount(TranState->EnvMaps); ++MapIndex) {
        environment_map* Map = TranState->EnvMaps + MapIndex;
        loaded_bitmap* LOD = Map->LOD + 0;
        
        v2 XAxis = 0.5f * v2{ (r32)LOD->Width, 0 };
        v2 YAxis = 0.5f * v2{ 0, (r32)LOD->Height };
        CoordinateSystem(RenderGroup, MapP, XAxis, YAxis, v4{ 1.0f, 1.0f, 1.0f, 1.0f },
                         LOD, 0, 0, 0, 0);
        MapP += YAxis + v2{ 0.0f, 6.0f };
    }
    
#endif
    
    Orthographic(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, 1.0f);
    
    
    rectangle2i d = {4, 4, 120, 120};
    
    EndSim(SimRegion, GameWorld);
    EndTemporaryMemory(SimMemory);
}

