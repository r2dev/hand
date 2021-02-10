internal entity *
BeginLowEntity(game_mode_world* WorldMode, entity_type Type) {
    Assert(WorldMode->CreationBufferIndex < ArrayCount(WorldMode->CreationBuffer));
    entity *Result = WorldMode->CreationBuffer + WorldMode->CreationBufferIndex++;
    ZeroStruct(*Result);
    
    Result->ID.Value = ++WorldMode->LastUsedEntityStorageIndex;
	Result->Type = Type;
	Result->Collision = WorldMode->NullCollision;
    return(Result);
}

internal void
EndEntity(game_mode_world* WorldMode, entity *Entity, world_position P) {
    --WorldMode->CreationBufferIndex;
    Assert(Entity == WorldMode->CreationBuffer + WorldMode->CreationBufferIndex);
    Entity->P = P.Offset_;
    PackEntityIntoWorld(WorldMode->World, Entity, P);
}

inline entity *
BeginGroundedEntity(game_mode_world* WorldMode, entity_type Type, entity_collision_volume_group * Collision) {
    entity *Entity = BeginLowEntity(WorldMode, Type);
	Entity->Collision = Collision;
	return(Entity);
}

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

internal void
InitHitPoints(entity* Entity, u32 HitPointCount) {
	Assert(HitPointCount <= ArrayCount(Entity->HitPoint));
	Entity->HitPointMax = HitPointCount;
	for (u32 HitPointIndex = 0; HitPointIndex < Entity->HitPointMax; ++HitPointIndex) {
		hit_point* HitPoint = Entity->HitPoint + HitPointIndex;
		HitPoint->FilledAmount = HIT_POINT_SUB_COUNT;
		HitPoint->Flag = 0;
	}
}

internal void
AddStandardRoom(game_mode_world* WorldMode, u32 AbsTileX, u32 AbsTileY, u32 AbsTileZ, random_series *Series) {
    for (s32 OffsetY = -4; OffsetY <= 4; ++OffsetY) {
        for (s32 OffsetX = -8; OffsetX <= 8; ++OffsetX) {
            world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX + OffsetX, AbsTileY + OffsetY, AbsTileZ);
            
            P.Offset_.z = 0.2f * (r32)(OffsetX + OffsetY);
            
            entity *Entity = BeginLowEntity(WorldMode, EntityType_Floor);
            Entity->Collision = WorldMode->FloorCollision;
            EndEntity(WorldMode, Entity, P);
            //AddFlags(&Entity.Low->Sim, EntityFlag_Traversable);
        }
    }
	
}

internal entity_id
AddPlayer(game_mode_world *WorldMode) {
	world_position P = WorldMode->CameraP;
    entity_id Result;
    entity *Body = BeginGroundedEntity(WorldMode, EntityType_HeroBody, WorldMode->HeroBodyCollision);
    AddFlags(Body, EntityFlag_Collides | EntityFlag_Moveable);
    InitHitPoints(Body, 3);
    
    
    entity *Head = BeginGroundedEntity(WorldMode, EntityType_HeroHead, WorldMode->HeroHeadCollision);
	AddFlags(Head, EntityFlag_Collides | EntityFlag_Moveable);
	
    
    Body->Head.ID = Head->ID;
    Head->Head.ID = Body->ID;
	if (WorldMode->CameraFollowingEntityID.Value == 0) {
		WorldMode->CameraFollowingEntityID = Head->ID;
	}
    Result = Head->ID;
    EndEntity(WorldMode, Head, P);
    EndEntity(WorldMode, Body, P);
	return(Result);
}

internal void
AddStair(game_mode_world* WorldMode, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Stairwell, WorldMode->StairCollision);
	Entity->WalkableHeight = WorldMode->TypicalFloorHeight;
	Entity->WalkableDim = Entity->Collision->TotalVolume.Dim.xy;
    AddFlags(Entity, EntityFlag_Collides);
    EndEntity(WorldMode, Entity, P);
}

internal void
AddWall(game_mode_world* WorldMode, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Wall, WorldMode->WallCollision);
	AddFlags(Entity, EntityFlag_Collides);
    EndEntity(WorldMode, Entity, P);
}

internal void
AddMonster(game_mode_world* WorldMode, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
	
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Monster, WorldMode->MonstarCollision);
	AddFlags(Entity, EntityFlag_Collides | EntityFlag_Moveable);
	InitHitPoints(Entity, 2);
    EndEntity(WorldMode, Entity, P);
}

internal void
AddFamiliar(game_mode_world* WorldMode, s32 AbsTileX, s32 AbsTileY, s32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(WorldMode->World, AbsTileX, AbsTileY, AbsTileZ);
    entity *Entity = BeginGroundedEntity(WorldMode, EntityType_Familiar, WorldMode->FamiliarCollision);
	AddFlags(Entity, EntityFlag_Collides | EntityFlag_Moveable);
    EndEntity(WorldMode, Entity, P);
}

internal void
DrawHitPoints(entity* Entity, render_group* PieceGroup, object_transform ObjectTransform) {
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
			PushRect(PieceGroup, ObjectTransform, V3(HitP, 0), HealthDim, Color);
			HitP += dHitP;
		}
	}
}

internal entity_collision_volume_group*
MakeSimpleGroundedCollision(game_mode_world* WorldMode, r32 DimX, r32 DimY, r32 DimZ, r32 OffsetZ = 0.0f) {
	entity_collision_volume_group* Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
	Group->VolumeCount = 1;
	Group->Volumes = PushArray(&WorldMode->World->Arena, 1, entity_collision_volume);
	Group->TotalVolume.Dim = v3{ DimX, DimY, DimZ };
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0.5f * DimZ + OffsetZ };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal entity_collision_volume_group*
MakeSimpleFloorCollision(game_mode_world* WorldMode, r32 DimX, r32 DimY, r32 DimZ) {
	entity_collision_volume_group* Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
	Group->VolumeCount = 0;
    Group->TraversableCount = 1;
    Group->Traversables = PushArray(&WorldMode->World->Arena, Group->TraversableCount, entity_traversable_point);
	Group->TotalVolume.Dim = v3{ DimX, DimY, DimZ };
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0 };
	Group->Traversables[0].P = v3{0, 0, 0};
	return(Group);
}

internal entity_collision_volume_group*
MakeNullCollision(game_mode_world* WorldMode) {
	entity_collision_volume_group* Group = PushStruct(&WorldMode->World->Arena, entity_collision_volume_group);
	Group->VolumeCount = 0;
	Group->Volumes = 0;
	Group->TotalVolume.Dim = v3{ 0, 0, 0};
	Group->TotalVolume.OffsetP = v3{ 0, 0, 0 };
	Group->Volumes[0] = Group->TotalVolume;
	return(Group);
}

internal void
ClearCollisionRulesFor(game_mode_world* WorldMode, entity_id ID) {
	for (u32 HashBucket = 0; HashBucket < ArrayCount(WorldMode->CollisionRuleHash); ++HashBucket) {
		for (pairwise_collision_rule** Rule = &WorldMode->CollisionRuleHash[HashBucket]; *Rule;) {
			if ((*Rule)->EntityIDA == ID.Value || (*Rule)->EntityIDB == ID.Value) {
				pairwise_collision_rule* RemovedRule = *Rule;
				*Rule = (*Rule)->NextHash;
                
				RemovedRule->NextHash = WorldMode->FirstFreeCollisionRule;
				WorldMode->FirstFreeCollisionRule = RemovedRule;
			}
			else {
				Rule = &(*Rule)->NextHash;
			}
		}
	}
    
}

internal void
AddCollisionRule(game_mode_world* WorldMode, u32 IDA, u32 IDB, b32 CanCollide) {
	if (IDA > IDB) {
		u32 Temp = IDA;
		IDA = IDB;
		IDB = Temp;
	}
	pairwise_collision_rule* Found = 0;
	u32 HashBucket = IDA & (ArrayCount(WorldMode->CollisionRuleHash) - 1);
	for (pairwise_collision_rule* Rule = WorldMode->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
		if (Rule->EntityIDA == IDA && Rule->EntityIDB == IDB) {
			Found = Rule;
            
			break;
		}
	}
    
	if (!Found) {
		Found = WorldMode->FirstFreeCollisionRule;
		if (Found) {
			WorldMode->FirstFreeCollisionRule = Found->NextHash;
		}
		else {
			Found = PushStruct(&WorldMode->World->Arena, pairwise_collision_rule);
		}
		Found->NextHash = WorldMode->CollisionRuleHash[HashBucket];
		WorldMode->CollisionRuleHash[HashBucket] = Found;
	}
    
	if (Found) {
		Found->EntityIDA = IDA;
		Found->EntityIDB = IDB;
		Found->CanCollide = CanCollide;
	}
    
	Assert(Found);
}


inline rectangle2
GetCameraRectAtDistance(render_group* RenderGroup, r32 CameraDistance) {
	v2 HalfDimOnTarget = (CameraDistance / RenderGroup->CameraTransform.FocalLength) * RenderGroup->MonitorHalfDimInMeters;
	rectangle2 Result = RectCenterHalfDim(v2{ 0, 0 }, HalfDimOnTarget);
	return(Result);
    
}

inline rectangle2
GetCameraRectAtTarget(render_group* RenderGroup) {
	rectangle2 Result = GetCameraRectAtDistance(RenderGroup, RenderGroup->CameraTransform.DistanceAboveTarget);
	return(Result);
}



internal void
EnterWorld(game_state *GameState, transient_state *TranState) {
    SetGameMode(GameState, TranState, GameMode_World);
    game_mode_world *WorldMode = PushStruct(&GameState->ModeArena, game_mode_world);
    
    WorldMode->TypicalFloorHeight = 3.0f;
    
    r32 PixelsToMeters = 1.0f / 42.0f;
    v3 WorldChunkDimMeters = { PixelsToMeters * GroundBufferWidth, PixelsToMeters * GroundBufferHeight, WorldMode->TypicalFloorHeight };
    WorldMode->World = CreateWorld(WorldChunkDimMeters, &GameState->ModeArena);
    world* World = WorldMode->World;
    s32 TileSideInPixels = 60;
    
    u32 TilesPerWidth = 17;
    u32 TilesPerHeight = 9;
    
    r32 TileSideInMeters = 1.4f;
    r32 TileDepthInMeters = WorldMode->TypicalFloorHeight;
    
    WorldMode->NullCollision = MakeSimpleGroundedCollision(WorldMode, 0, 0, 0);
    WorldMode->StairCollision = MakeSimpleGroundedCollision(WorldMode, 
                                                            TileSideInMeters, 2.0f * TileSideInMeters, 1.1f * TileDepthInMeters);
    WorldMode->HeroHeadCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.4f, 0.7f);
    WorldMode->HeroBodyCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.6f);
    WorldMode->MonstarCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f);
    WorldMode->WallCollision = MakeSimpleGroundedCollision(WorldMode, TileSideInMeters, TileSideInMeters, TileDepthInMeters);
    WorldMode->FamiliarCollision = MakeSimpleGroundedCollision(WorldMode, 1.0f, 0.5f, 0.5f);
    
    WorldMode->FloorCollision = MakeSimpleFloorCollision(WorldMode, TileSideInMeters, TileSideInMeters, 0.9f * TileDepthInMeters);
    
    random_series Series = RandomSeed(1234);
    WorldMode->EffectEntropy = RandomSeed(5456);
    
    
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
    
    for (u32 ScreenIndex = 0; ScreenIndex < 4; ++ScreenIndex) {
#if 0
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
        AddStandardRoom(WorldMode, 
                        (ScreenX * TilesPerWidth + TilesPerWidth / 2), 
                        (ScreenY * TilesPerHeight + TilesPerHeight / 2), 
                        AbsTileZ, &Series);
        
        
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
                    
                    AddWall(WorldMode, AbsTileX, AbsTileY, AbsTileZ);
                    
                    //}
                } else if (CreatedZDoor) {
                    if (((AbsTileZ % 2) && (TileX == 10) && (TileY == 5)) ||
                        (!(AbsTileZ % 2) && (TileX == 4) && (TileY == 5)))
                    {
                        AddStair(WorldMode, AbsTileX, AbsTileY, DoorDown ? AbsTileZ - 1 : AbsTileZ);
                    }
                    /**if (TileY == 5 && TileX == 10) {
                        AddStair(WorldMode, AbsTileX, AbsTileY, DoorDown? AbsTileZ - 1: AbsTileZ);
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
        ChunkPositionFromTilePosition(WorldMode->World, CameraTileX, CameraTileY, CameraTileZ);
    WorldMode->CameraP = NewCameraP;
    AddMonster(WorldMode, CameraTileX - 3, CameraTileY + 2, CameraTileZ);
    AddFamiliar(WorldMode, CameraTileX - 2, CameraTileY - 2, CameraTileZ);
    GameState->WorldMode = WorldMode;
}

internal b32
GetClosestTraversable(sim_region *SimRegion, v3 FromP, v3 *Result) {
    b32 Found = false;
    r32 BestDistanceSq = Square(1000.0f);
    entity* TestEntity = SimRegion->Entities;
    for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
        for (u32 PIndex = 0; PIndex < TestEntity->Collision->TraversableCount; ++PIndex) {
            entity_traversable_point P = GetSimEntityTraversable(TestEntity, PIndex);
            v3 HeadToPoint = P.P - FromP;
            // HeadToPoint.z 
            r32 TestDSq = LengthSq(HeadToPoint);
            if (BestDistanceSq > TestDSq) {
                *Result = P.P;
                BestDistanceSq = TestDSq;
                Found = true;
            }
        }
    }
    return(Found);
}


internal b32
UpdateAndRenderWorld(game_state *GameState, game_mode_world *WorldMode, transient_state *TranState, game_input *Input, render_group *RenderGroup, loaded_bitmap *DrawBuffer)
{
    
    b32 Result = false;
    world *World = WorldMode->World;
    v2 MouseP = {Input->MouseX, Input->MouseY};
    r32 FocalLength = 0.6f;
    r32 DistanceAboveTarget = 9.0f;
    
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, DistanceAboveTarget);
    Clear(RenderGroup, v4{ 0.25f, 0.25f, 0.25f, 1.0f });
    
    v2 ScreenCenter = v2{ 0.5f * DrawBuffer->Width, 0.5f * DrawBuffer->Height };
    
    rectangle2 ScreenBound = GetCameraRectAtTarget(RenderGroup);
    
    rectangle3 CameraBoundsInMeters = RectMinMax(V3(ScreenBound.Min, 0.0f), V3( ScreenBound.Max, 0.0f));
    CameraBoundsInMeters.Min.z = -2.0f * WorldMode->TypicalFloorHeight;
    CameraBoundsInMeters.Max.z = 1.0f * WorldMode->TypicalFloorHeight;
    b32 HeroExist = false;
    b32 QuitRequested = false;
    for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
        game_controller_input* Controller = GetController(Input, ControllerIndex);
        controlled_hero* ConHero = GameState->ControlledHeroes + ControllerIndex;
        if (ConHero->ID.Value == 0) {
            if (WasPressed(Controller->Back)) {
                QuitRequested = true;
            }
            else if (WasPressed(Controller->Start)) {
                *ConHero = {};
                ConHero->ID = AddPlayer(WorldMode);
            }
        }
        if (ConHero->ID.Value) {
            HeroExist = true;
            ConHero->dZ = 0.0f;
            ConHero->dSword = {};
            
            if (Controller->IsAnalog) {
                ConHero->ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
            }
            else {
                r32 RecenterTimer = 0.2f;
                if (WasPressed(Controller->MoveUp)) {
                    ConHero->ddP.x = 0;
                    ConHero->ddP.y = 1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveDown)) {
                    ConHero->ddP.x = 0;
                    ConHero->ddP.y = -1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveLeft)) {
                    ConHero->ddP.x = -1.0f;
                    ConHero->ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveRight)) {
                    ConHero->ddP.x = 1.0f;
                    ConHero->ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (!IsDown(Controller->MoveUp) && !IsDown(Controller->MoveDown)) {
                    ConHero->ddP.y = 0;
                    if (IsDown(Controller->MoveRight)) {
                        ConHero->ddP.x = 1;
                    }
                    if (IsDown(Controller->MoveLeft)) {
                        ConHero->ddP.x = -1;
                    }
                }
                
                if (!IsDown(Controller->MoveRight) && !IsDown(Controller->MoveLeft)) {
                    ConHero->ddP.x = 0;
                    if (IsDown(Controller->MoveUp)) {
                        ConHero->ddP.y = 1;
                    }
                    if (IsDown(Controller->MoveDown)) {
                        ConHero->ddP.y = -1;
                    }
                }
            }
#if 0
            if (Controller->Start.EndedDown) {
                ConHero->dZ = 3.0f;
            }
#endif
            ConHero->dSword = {};
#if 1
            if (Controller->ActionUp.EndedDown) {
                ConHero->dSword = v2{ 0.0f, 1.0f };
                ChangeVolume(GameState->Music, v2{ 1, 1 }, 2.0f);
            }
            if (Controller->ActionDown.EndedDown) {
                ConHero->dSword = v2{ 0.0f, -1.0f };
                ChangeVolume(GameState->Music, v2{ 0, 0 }, 2.0f);
            }
            if (Controller->ActionLeft.EndedDown) {
                ChangeVolume(GameState->Music, v2{ 1, 0 }, 2.0f);
                ConHero->dSword = v2{ -1.0f, 0.0f };
            }
            if (Controller->ActionRight.EndedDown) {
                ChangeVolume(GameState->Music, v2{ 0, 1 }, 2.0f);
                ConHero->dSword = v2{ 1.0f, 0.0f };
            }
            if (WasPressed(Controller->Back)) {
                ConHero->Exited = true;
            }
#else
            r32 ZoomRate = 0.0f;
            if (Controller->ActionUp.EndedDown) {
                ZoomRate = 1.0f;
            }
            else if (Controller->ActionDown.EndedDown) {
                ZoomRate = -1.0f;
            }
#endif
        }
    }
    v3 SimBoundsExpansion = { 15.0f, 15.0f, 15.0f };
    rectangle3 SimBounds = AddRadiusTo(CameraBoundsInMeters, SimBoundsExpansion);
    
    //temporary_memory SimMemory = BeginTemporaryMemory(&TranState->TranArena);
    
    world_position SimCenterP = WorldMode->CameraP;
    sim_region* SimRegion = BeginSim(&TranState->TranArena, WorldMode, WorldMode->World, SimCenterP, SimBounds, Input->dtForFrame);
    v3 CameraP = Subtract(World, &WorldMode->CameraP, &SimCenterP) + WorldMode->CameraOffset;
    
    object_transform WorldTransform = DefaultUprightTransform();
    WorldTransform.OffsetP = -CameraP;
    PushRectOutline(RenderGroup, WorldTransform, v3{ 0, 0, 0 }, GetDim(ScreenBound), v4{ 1.0f, 1.0f, 0.0f, 1.0f });
    PushRectOutline(RenderGroup, WorldTransform, v3{ 0, 0, 0 }, GetDim(SimBounds).xy, v4{ 0.0f, 1.0f, 1.0f, 1.0f });
    PushRectOutline(RenderGroup, WorldTransform, v3{ 0, 0, 0 }, GetDim(SimRegion->Bounds).xy, v4{ 1.0f, 0.0f, 0.0f, 1.0f });
    
    for (u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
        entity* Entity = SimRegion->Entities + EntityIndex;
        Entity->XAxis = v2{1,0};
        Entity->YAxis = v2{0,1};
        if (Entity->Updatable) {
            
            r32 dt = Input->dtForFrame;
            
            r32 ShadowAlpha = 1.0f - 0.5f * Entity->P.z;
            if (ShadowAlpha < 0) {
                ShadowAlpha = 0.0f;
            }
            
            move_spec MoveSpec = DefaultMoveSpec();
            v3 ddP = {};
            
            v3 CameraRelativeGroundP = GetEntityGroundPoint(Entity) - CameraP;
            r32 FadeTopEnd = 0.75f * WorldMode->TypicalFloorHeight;
            r32 FadeTopStart = 0.5f * WorldMode->TypicalFloorHeight;
            
            r32 FadeBottomStart = -2.0f * WorldMode->TypicalFloorHeight;
            r32 FadeBottomEnd = -2.25f * WorldMode->TypicalFloorHeight;
            
            RenderGroup->GlobalAlpha = 1.0f;
            
            
            if (CameraRelativeGroundP.z > FadeTopStart) {
                RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeTopStart, CameraRelativeGroundP.z, FadeTopEnd);
            }
            else if (CameraRelativeGroundP.z < FadeBottomStart) {
                RenderGroup->GlobalAlpha = 1.0f - Clamp01MapToRange(FadeBottomStart, CameraRelativeGroundP.z, FadeBottomEnd);
            }
            
            //hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[Entity->FacingDirection];
            switch (Entity->Type) {
                case EntityType_HeroHead: {
                    for (u32 ControlIndex = 0; ControlIndex < ArrayCount(GameState->ControlledHeroes); ++ControlIndex) {
                        controlled_hero* ConHero = GameState->ControlledHeroes + ControlIndex;
                        if (Entity->ID.Value == ConHero->ID.Value) {
                            if (ConHero->dZ != 0.0f) {
                                Entity->dP.z = ConHero->dZ;
                            }
                            MoveSpec.UnitMaxAccelVector = true;
                            MoveSpec.Speed = 30.0f;
                            MoveSpec.Drag = 8.0f;
                            ddP = V3(ConHero->ddP, 0);
                            
                            if (ConHero->dSword.x == 0.0f && ConHero->dSword.y == 0.0f) {
                                // remain whichever face direction it was
                            }
                            else {
                                Entity->FacingDirection = Atan2(ConHero->dSword.y, ConHero->dSword.x);
                            }
                            
                            v3 ClosestP = Entity->P;
                            if (GetClosestTraversable(SimRegion, Entity->P, &ClosestP)) {
                                ConHero->RecenterTimer = ClampAboveZero(ConHero->RecenterTimer - dt);
                                b32 TimeIsUp = ConHero->RecenterTimer == 0.0f? true: false;
                                b32 NoPush = (Length(ddP) < 0.1f);
                                r32 Cp = NoPush? 300.0f: 25.0f;
                                v3 ddP2 = {};
                                for (u32 E = 0; E < 3; ++E) {
#if 0
                                    if (Square(ddP.E[E]) < 0.1f)
#else
                                    if (NoPush || (TimeIsUp && (Square(ddP.E[E]) < 0.1f)))
#endif
                                    {
                                        ddP2.E[E] =
                                            Cp * (ClosestP.E[E] - Entity->P.E[E]) - 30.0f * Entity->dP.E[E];
                                    }
                                }
                                Entity->dP += dt *ddP2;
                            }
                            if (ConHero->Exited) {
                                ConHero->Exited = false;
                                DeleteEntity(SimRegion, ConHero->ID);
                                ConHero->ID.Value = 0;
                            }
                        }
                    }
                    
                } break;
                case EntityType_HeroBody: {
                    DEBUG_VALUE(GetEntityGroundPoint(Entity));
                    entity *Head = Entity->Head.Ptr;
                    if (Head) {
                        v3 ClosestP = Entity->P;
                        b32 Found = GetClosestTraversable(SimRegion, Head->P, &ClosestP);
                        v3 BodyDelta = ClosestP - Entity->P;
                        r32 BodyDistance = LengthSq(BodyDelta);
                        Entity->FacingDirection = Head->FacingDirection;
                        r32 ddtBob = 0.0f;
                        v3 HeadDelta = Head->P - Entity->P;
                        r32 HeadDistance = Length(HeadDelta);
                        r32 MaxHeadDistance = 0.4f;
                        Entity->dP = v3{0, 0, 0};
                        r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                        Entity->FloorDisplace = 0.25f * HeadDelta.xy;
                        switch (Entity->MovementMode) {
                            case MovementMode_Planted: {
                                if (Found && (BodyDistance > Square(0.01f))) {
                                    Entity->tMovement = 0;
                                    Entity->MovementFrom = Entity->P;
                                    Entity->MovementTo = ClosestP;
                                    Entity->MovementMode = MovementMode_Hopping;
                                    
                                }
                                ddtBob = -20.0f * tHeadDistance;
                                
                            } break;
                            case MovementMode_Hopping: {
                                r32 tTotal = Entity->tMovement;
                                r32 tJump = 0.1f;
                                r32 tLand = 0.9f;
                                
                                if (tTotal < tJump) {
                                    ddtBob = 45.0f;
                                }
                                if (tTotal < tLand) {
                                    r32 t = Clamp01MapToRange(tJump, tTotal, tLand);
                                    v3 a = v3{0, -2.0f, 0};
                                    v3 b = Entity->MovementTo - Entity->MovementFrom - a;
                                    Entity->P = a * t * t + b * t + Entity->MovementFrom;
                                }
                                
                                if (Entity->tMovement >= 1.0f) {
                                    Entity->P = Entity->MovementTo;
                                    Entity->MovementMode = MovementMode_Planted;
                                    Entity->dtBob = -2.0f;
                                }
                                Entity->tMovement += 6.0f * dt;
                                if (Entity->tMovement > 1.0f) {
                                    Entity->tMovement = 1.0f;
                                }
                                
                            } break;
                        }
                        r32 Cv = 10.0f;
                        ddtBob += -100.0f * Entity->tBob + Cv * -Entity->dtBob;
                        Entity->tBob = ddtBob * dt * dt + Entity->dtBob*dt;
                        Entity->dtBob += ddtBob * dt;
                        
                        //Entity->XAxis = Prep;
                        Entity->YAxis = v2{0, 1} + 0.5f * HeadDelta.xy;
                        //-0.1f * Sin(t * Tau32);
                        
                    }
                } break;
                
                case EntityType_Familiar: {
                    entity* ClosestHero = 0;
                    r32 ClosestHeroSq = Square(10.0f);
                    entity* TestEntity = SimRegion->Entities;
                    
                    if(Global_Sim_FamiliarFollowsHero)
                    {
                        for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
                            if (TestEntity->Type == EntityType_HeroBody) {
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
            if (IsSet(Entity, EntityFlag_Moveable)) {
                MoveEntity(WorldMode, SimRegion, Entity, Input->dtForFrame, &MoveSpec, ddP);
            }			
            
            object_transform EntityTransform = DefaultUprightTransform();
            EntityTransform.OffsetP = GetEntityGroundPoint(Entity) - CameraP;
            
            asset_vector MatchVector = {};
            MatchVector.E[Tag_FaceDirection] = (r32)Entity->FacingDirection;
            asset_vector WeightVector = {};
            WeightVector.E[Tag_FaceDirection] = 1.0f;
            r32 HeroSizeC = 3.0f;
            switch (Entity->Type) {
                case EntityType_HeroBody: {
                    
                    v4 Color = {1,1,1,1};
                    v2 XAxis = Entity->XAxis;
                    v2 YAxis = Entity->YAxis;
                    v3 Offset = V3(Entity->FloorDisplace, 0.0f);
                    PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), HeroSizeC * 1.2f, {0,0,0}, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, EntityTransform, GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector), HeroSizeC * 1.2f, Offset + v3{0, 0, -0.0002f}, Color, 1.0f, XAxis, YAxis);
                    PushBitmap(RenderGroup, EntityTransform, GetBestMatchBitmapFrom(TranState->Assets, Asset_Cape, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, Entity->tBob - 0.1f, -0.0001f }, Color, 1.0f, XAxis, YAxis);
                } break;
                case EntityType_HeroHead: {
                    PushBitmap(RenderGroup, EntityTransform, GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector), HeroSizeC * 1.2f, v3{ 0, -0.4f, 0 });
                    DrawHitPoints(Entity, RenderGroup, EntityTransform);
                    
                } break;
                case EntityType_Wall: {
                    
                    PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Tree), 2.5f, v3{ 0, 0, 0 });
                } break;
                
                case EntityType_Familiar: {
                    PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 0.5f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, EntityTransform, GetBestMatchBitmapFrom(TranState->Assets, Asset_Head, &MatchVector, &WeightVector), 1.0f, v3{ 0, 0, 0 });
                }
                case EntityType_Monster: {
                    
                    PushBitmap(RenderGroup, EntityTransform, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
                    PushBitmap(RenderGroup, EntityTransform, GetBestMatchBitmapFrom(TranState->Assets, Asset_Torso, &MatchVector, &WeightVector), 1.2f, v3{ 0, 0, 0 });
                    DrawHitPoints(Entity, RenderGroup, EntityTransform);
                } break;
                
                case EntityType_Stairwell: {
                    u32 OldIndex = RenderGroup->CurrentClipRectIndex;
                    RenderGroup->CurrentClipRectIndex = PushClipRect(RenderGroup, EntityTransform, v3{ 0, 0, 0 }, v2{10.0f, 10.0f});
                    
                    PushRect(RenderGroup, EntityTransform, v3{ 0, 0, 0 }, Entity->WalkableDim, v4{ 1.0f, 0.5f, 0, 1.0f });
                    PushRect(RenderGroup, EntityTransform, v3{ 0, 0, Entity->WalkableHeight }, Entity->WalkableDim, v4{ 1.0f, 1.0f, 0, 1.0f });
                    RenderGroup->CurrentClipRectIndex = OldIndex;
                    
                    //PushBitmap(&PieceGroup, &GameState->Stairwell, v2{ 0, 0 }, 0, v2{ 37, 37 });
                } break;
                case EntityType_Floor: {
                    
                    for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                        entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                        PushRectOutline(RenderGroup, EntityTransform, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, v4{ 0, 0.5f, 1.0f, 1.0f });
                    }
                    for (u32 Index = 0; Index < Entity->Collision->TraversableCount; Index++) {
                        entity_traversable_point* Traversable = Entity->Collision->Traversables + Index;
                        
                        PushRect(RenderGroup, EntityTransform, Traversable->P, v2{0.1f, 0.1f}, v4{ 0, 0.5f, 1.0f, 1.0f });
                    }
                } break;
                default: {
                    InvalidCodePath
                } break;
                
            }
            if (DEBUG_UI_ENABLED) {
                debug_id EntityDebugID = DEBUG_POINTER_ID((void *)Entity->ID.Value);
                for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                    entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                    v3 WorldMousePoint = Unproject(RenderGroup, EntityTransform, MouseP);
#if 0
                    PushRect(RenderGroup, V3(WorldMousePoint, 0.0f), v2{1.0f, 1.0f}, v4{1.0f, 0.0f, 1.0f, 1.0f});
#endif
                    if ((WorldMousePoint.x > -0.5f * Volume->Dim.x && WorldMousePoint.x < 0.5f * Volume->Dim.x) &&
                        (WorldMousePoint.y > -0.5f * Volume->Dim.y && WorldMousePoint.y < 0.5f * Volume->Dim.y)){
                        
                        DEBUG_HIT(EntityDebugID, WorldMousePoint.z);
                    }
                    v4 EntityOutlineColor;
                    if (DEBUG_HIGHLIGHTED(EntityDebugID, &EntityOutlineColor)) {
                        PushRectOutline(RenderGroup, EntityTransform, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, EntityOutlineColor, 0.05f);
                    }
                    
                }
                if (DEBUG_REQUESTED(EntityDebugID)) {
                    DEBUG_DATA_BLOCK("Simulation/Entity");
                    DEBUG_VALUE(Entity->P);
                    DEBUG_VALUE(Entity->dP);
                    DEBUG_VALUE(Entity->FacingDirection);
                    DEBUG_VALUE(Entity->WalkableDim);
                    DEBUG_VALUE(Entity->WalkableHeight);
                    DEBUG_VALUE(Entity->ID.Value);
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
    
    EndSim(SimRegion, WorldMode);
    //EndTemporaryMemory(SimMemory);
    
    if (!HeroExist) {
        //PlayIntroCutScene(GameState, TranState);
        PlayTitleScreen(GameState, TranState);
    }
    
    return(Result);
}


#if 0
if(Global_Particle_Demo)
{
    for (u32 ParticleSpawnIndex = 0; ParticleSpawnIndex < 2; ++ParticleSpawnIndex) {
        particle *Particle = WorldMode->Particle + WorldMode->NextParticle++;
        if (WorldMode->NextParticle >= ArrayCount(WorldMode->Particle)) {
            WorldMode->NextParticle = 0;
        }
        
        Particle->P = v3{RandomBetween(&WorldMode->EffectEntropy, -0.1f, 0.1f), 0, 0};
        Particle->dP = v3{ 
            RandomBetween(&WorldMode->EffectEntropy, -0.1f, 0.1f), 
            7.0f * RandomBetween(&WorldMode->EffectEntropy, 0.7f, 1.0f), 
            0.0f};
        Particle->ddP = v3{0, -9.8f, 0};
        Particle->dColor = v4{0, 0, 0, -0.5f};
        Particle->Color = v4{
            RandomBetween(&WorldMode->EffectEntropy, 0.7f, 1.0f),
            RandomBetween(&WorldMode->EffectEntropy, 0.7f, 1.0f),
            RandomBetween(&WorldMode->EffectEntropy, 0.7f, 1.0f),
            1.0f
        };
        
    }
    
    ZeroStruct(WorldMode->ParticleCels);
    
    r32 GridScale = 0.5f;
    v3 GridOrigin = {-0.5f * GridScale * PARTICEL_CEL_DIM, 0, 0};
    r32 InvGridScale = 1.0f / GridScale;
    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(WorldMode->Particle); ++ParticleIndex) {
        particle *Particle = WorldMode->Particle + ParticleIndex;
        
        v3 P = InvGridScale * (Particle->P - GridOrigin);
        s32 X = TruncateReal32ToInt32(P.x);
        s32 Y = TruncateReal32ToInt32(P.y);
        
        if (X < 0) {X = 0;}
        if (X > PARTICEL_CEL_DIM - 1) { X = PARTICEL_CEL_DIM - 1;}
        if (Y < 0) {Y = 0;}
        if (Y > PARTICEL_CEL_DIM - 1) { Y = PARTICEL_CEL_DIM - 1;}
        particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
        r32 Density = Particle->Color.a;
        Cel->Density += Density;
        Cel->VelocityTimesDensity += Particle->dP * Density;
    }
    if(Global_Particle_Grid)
    {
        for(u32 Y = 0; Y < PARTICEL_CEL_DIM; ++Y) {
            for (u32 X = 0; X < PARTICEL_CEL_DIM; ++X) {
                particle_cel *Cel = &WorldMode->ParticleCels[Y][X];
                r32 Alpha = Clamp01(0.1f * Cel->Density);
                PushRect(RenderGroup, EntityTransform, GridScale * v3{(r32)X, (r32)Y, 0} + GridOrigin, GridScale * v2{1.0f, 1.0f}, v4{Alpha, Alpha, Alpha, 1.0f});
            }
        }
    }
    
    for (u32 ParticleIndex = 0; ParticleIndex < ArrayCount(WorldMode->Particle); ++ParticleIndex) {
        particle *Particle = WorldMode->Particle + ParticleIndex;
        
        v3 P = InvGridScale * (Particle->P - GridOrigin);
        
        s32 X = TruncateReal32ToInt32(P.x);
        s32 Y = TruncateReal32ToInt32(P.y);
        
        if (X < 1) {X = 1;}
        if (X > PARTICEL_CEL_DIM - 2) { X = PARTICEL_CEL_DIM - 2;}
        if (Y < 1) {Y = 1;}
        if (Y > PARTICEL_CEL_DIM - 2) { Y = PARTICEL_CEL_DIM - 2;}
        
        particle_cel *CelCenter = &WorldMode->ParticleCels[Y][X];
        particle_cel *CelLeft = &WorldMode->ParticleCels[Y][X - 1];
        particle_cel *CelRight = &WorldMode->ParticleCels[Y][X + 1];
        particle_cel *CelDown = &WorldMode->ParticleCels[Y - 1][X];
        particle_cel *CelUp = &WorldMode->ParticleCels[Y + 1][X];
        
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
        PushBitmap(RenderGroup, EntityTransform, GetRandomBitmapFrom(TranState->Assets, Asset_Head, &WorldMode->EffectEntropy), 1.0f, Particle->P, Color);
        //PushBitmap(RenderGroup, GetFirstBitmapFrom(TranState->Assets, Asset_Shadow), 1.2f, v3{ 0, 0, 0 }, v4{ 1, 1, 1, ShadowAlpha });
        
    }
}
#endif