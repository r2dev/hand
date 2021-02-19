inline entity_traversable_point *
GetTraversable(entity *Entity, u32 Index) {
    entity_traversable_point* Result = 0;
    if (Entity) {
        Assert(Index < Entity->TraversableCount);
        Result = Entity->Traversables + Index;
    }
    return(Result);
}

inline entity_traversable_point *
GetTraversable(traversable_reference Reference) {
    entity_traversable_point *Result = GetTraversable(Reference.Entity.Ptr, Reference.Index);
    return(Result);
}

inline entity_traversable_point
GetSimEntityTraversable(entity *Entity, u32 Index) {
    entity_traversable_point Result = {};
    Result.P = Entity->P;
    Assert(Index < Entity->TraversableCount);
    entity_traversable_point *Point = GetTraversable(Entity, Index);
    if (Point) {
        Result.P += Point->P;
        Result.Occupier = Point->Occupier;
    }
    
    return(Result);
}

inline entity_traversable_point
GetSimEntityTraversable(traversable_reference Ref) {
    entity_traversable_point Result = GetSimEntityTraversable(Ref.Entity.Ptr, Ref.Index);
    return(Result);
}

internal entity_hash *
GetHashFromID(sim_region* SimRegion, entity_id ID) {
	Assert(ID.Value);
	entity_hash* Result = 0;
	u32 HashValue = ID.Value;
	for (u32 Offset = 0; Offset < ArrayCount(SimRegion->EntityHash); ++Offset) {
		u32 HashIndex = ((HashValue + Offset) & (ArrayCount(SimRegion->EntityHash) - 1));
		entity_hash* Entry = SimRegion->EntityHash + HashIndex;
		if (Entry->ID.Value == 0 || Entry->ID.Value == ID.Value) {
			Result = Entry;
			break;
		}
	}
	return(Result);
}

internal brain_hash *
GetHashFromID(sim_region* SimRegion, brain_id ID) {
	Assert(ID.Value);
    brain_hash* Result = 0;
	u32 HashValue = ID.Value;
	for (u32 Offset = 0; Offset < ArrayCount(SimRegion->BrainHash); ++Offset) {
		u32 HashIndex = ((HashValue + Offset) & (ArrayCount(SimRegion->BrainHash) - 1));
		brain_hash* Entry = SimRegion->BrainHash + HashIndex;
		if (Entry->ID.Value == 0 || Entry->ID.Value == ID.Value) {
			Result = Entry;
			break;
		}
	}
	return(Result);
}

inline entity *
GetEntityByID(sim_region* SimRegion, entity_id ID) {
	entity_hash* Entry = GetHashFromID(SimRegion, ID);
	entity* Result = Entry? Entry->Ptr: 0;
	return(Result);
}

inline void
LoadEntityReference(sim_region* SimRegion, entity_reference* Ref) {
    // TODO(not-set): 
	if (Ref->ID.Value != 0) {
		Ref->Ptr = GetEntityByID(SimRegion, Ref->ID);
	}
}

inline void
LoadTraversableReference(sim_region* SimRegion, traversable_reference* Ref) {
    LoadEntityReference(SimRegion, &Ref->Entity);
}

internal b32
EntitiesOverlap(entity* Entity, entity* TestEntity, v3 Epsilon = v3{ 0, 0, 0 }) {
    TIMED_FUNCTION();
	b32 Result = false;
	for (u32 VolumeIndex = 0; !Result && VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
		entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
		for (u32 TestVolumeIndex = 0; !Result && TestVolumeIndex < TestEntity->Collision->VolumeCount; TestVolumeIndex++) {
			entity_collision_volume* TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
            
			rectangle3 EntityRect = RectCenterDim(Entity->P + Volume->OffsetP, Volume->Dim + Epsilon);
			rectangle3 TestEntityRect = RectCenterDim(TestEntity->P + TestVolume->OffsetP, TestVolume->Dim);
			Result = RectanglesIntersect(EntityRect, TestEntityRect);
		}
	}
	return(Result);
}

inline b32
EntityOverlapsRectangle(v3 P, entity_collision_volume Volume, rectangle3 Rect) {
	rectangle3 Grown = AddRadiusTo(Rect, 0.5f * Volume.Dim);
	b32 Result = IsInRectangle(Grown, P + Volume.OffsetP);
	return(Result);
}

internal brain *
AddOrGetBrain(sim_region *SimRegion, brain_id ID, brain_type Type) {
    brain *Result = 0;
    brain_hash *BrainHash = GetHashFromID(SimRegion, ID);
    Result = BrainHash->Ptr;
    if (!Result) {
        Assert(SimRegion->BrainCount < SimRegion->MaxBrainCount );
        Result = SimRegion->Brains + SimRegion->BrainCount++;
        Result->ID = ID;
        Result->Type = Type;
        BrainHash->Ptr = Result;
    }
    return(Result);
}

struct test_wall {
	r32 X;
	r32 RelX;
	r32 RelY;
	r32 DeltaX;
	r32 DeltaY;
	r32 MinY;
	r32 MaxY;
	v3 Normal;
};

internal void
ConnectEntityPointers(sim_region *SimRegion) {
    // TODO(not-set): 
    for(u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
        entity *Entity = SimRegion->Entities + EntityIndex;
        //LoadEntityReference(SimRegion, &Entity->Head);
        LoadTraversableReference(SimRegion, &Entity->Occupying);
        if (Entity->Occupying.Entity.Ptr) {
            Entity->Occupying.Entity.Ptr->Traversables[Entity->Occupying.Index].Occupier = Entity;
        }
        LoadTraversableReference(SimRegion, &Entity->CameFrom);
    }
}

internal sim_region*
BeginSim(memory_arena* SimArena, game_mode_world* WorldMode, world* World, world_position Origin, rectangle3 Bounds, r32 dt) {
    TIMED_FUNCTION();
	sim_region* SimRegion = PushStruct(SimArena, sim_region);
	SimRegion->MaxEntityRadius = 5.0f;
	SimRegion->MaxEntityVelocity = 30.0f;
	r32 UpdateSafetyMargin = SimRegion->MaxEntityRadius + dt * SimRegion->MaxEntityVelocity;
	r32 UpdateSafetyMarginZ = 1.0f;
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	SimRegion->UpdatableBounds = AddRadiusTo(Bounds, v3{ SimRegion->MaxEntityRadius, SimRegion->MaxEntityRadius, SimRegion->MaxEntityRadius });
	SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, v3{ UpdateSafetyMargin, UpdateSafetyMargin, UpdateSafetyMarginZ });
    
	SimRegion->MaxEntityCount = 4096;
	SimRegion->EntityCount = 0;
	SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, entity);
    
    SimRegion->MaxBrainCount = 256;
	SimRegion->BrainCount = 0;
    SimRegion->Brains = PushArray(SimArena, SimRegion->MaxBrainCount, brain);
    
	world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
	for (s32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ) {
		for (s32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
			for (s32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
				world_chunk* Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);
                world_position ChunkP = {ChunkX, ChunkY, ChunkZ};
                // chunk delta relative to sim chunk
                v3 ChunkDelta = Subtract(SimRegion->World, &ChunkP, &SimRegion->Origin);
				if (Chunk) {
                    world_entity_block* Block = Chunk->FirstBlock;
                    while (Block) {
                        for (u32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
							entity *Source = (entity *)Block->EntityData + EntityIndex;
                            
                            entity_id ID = Source->ID;
                            entity_hash* Entry = GetHashFromID(SimRegion, ID);
                            Assert(Entry->Ptr == 0);
                            entity *Dest = 0;
                            if (SimRegion->EntityCount < SimRegion->MaxEntityCount) {
                                Dest = SimRegion->Entities + SimRegion->EntityCount++;
                                Entry->ID = ID;
                                Entry->Ptr = Dest;
                                if (Source) {
                                    // decompression
                                    *Dest = *Source;
                                }
                                Dest->ID = ID;
                                Dest->P += ChunkDelta;
                                
                                Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
                                if (Dest->BrainID.Value != 0) {
                                    brain *Brain = AddOrGetBrain(SimRegion, Dest->BrainID, Dest->BrainType);
                                    Assert(Dest->BrainSlot.Index < ArrayCount(Brain->Array));
                                    Brain->Array[Dest->BrainSlot.Index] = Dest;
                                }
                            }
                            else {
                                InvalidCodePath
                            }
						}
                        world_entity_block* NextBlock = Block->Next;
                        AddBlockToFreeList(World, Block);
                        Block = NextBlock;
                    }
                    
                    AddChunkToFreeList(World, Chunk);
				}
			}
		}
	}
    ConnectEntityPointers(SimRegion);
	return(SimRegion);
}

inline void
DeleteEntity(sim_region *SimRegion, entity *Entity) {
    if (Entity) {
        Entity->Flags |= EntityFlag_Deleted;
    }
}

internal void
EndSim(sim_region* SimRegion, game_mode_world* WorldMode) {
    TIMED_FUNCTION();
	entity* Entity = SimRegion->Entities;
    world *World = WorldMode->World;
	for (u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex, ++Entity) {
        if (!(Entity->Flags & EntityFlag_Deleted)) {
            world_position EntityChunkP = MapIntoChunkSpace(WorldMode->World, SimRegion->Origin, Entity->P);
            world_position ChunkP = EntityChunkP;
            ChunkP.Offset_ = {0, 0, 0};
            v3 ChunkDelta = -Subtract(SimRegion->World, &ChunkP, &SimRegion->Origin);
            if (Entity->ID.Value == WorldMode->CameraFollowingEntityID.Value) {
                world_position NewCameraP = WorldMode->CameraP;
                
                v3 RoomDelta = {24.0f, 12.5f, WorldMode->TypicalFloorHeight};
                v3 hRoomDelta = 0.5f * RoomDelta;
                r32 ApronSize = 0.7f;
                r32 BounceHeight = 0.1f;
                v3 hRoomApron = hRoomDelta - v3{ApronSize, ApronSize, ApronSize};
                if(Global_Sim_RoomBaseCamera)
                {
                    v3 AppliedCameraDelta = {};
                    
                    for (u32 E = 0; E < 3; ++E) {
                        if (Entity->P.E[E] > hRoomDelta.E[E]) {
                            AppliedCameraDelta = {};
                            AppliedCameraDelta.E[E] = RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedCameraDelta);
                        }
                        if (Entity->P.E[E] < -hRoomDelta.E[E]) {
                            AppliedCameraDelta = {};
                            AppliedCameraDelta.E[E] = -RoomDelta.E[E];
                            NewCameraP = MapIntoChunkSpace(World, NewCameraP, AppliedCameraDelta);
                        }
                    }
                    
                    v3 EntityP = Entity->P - AppliedCameraDelta;
                    // parabolic arc for camera
                    r32 a = -1;
                    r32 b = 2;
                    
                    if (EntityP.y > hRoomApron.y) {
                        r32 t = Clamp01MapToRange(hRoomApron.y, EntityP.y, hRoomDelta.y);
                        WorldMode->CameraOffset = {0, t * hRoomDelta.y, a * t * t + b * t} ;
                    }
                    if (EntityP.y < -hRoomApron.y) {
                        r32 t = Clamp01MapToRange(-hRoomApron.y, EntityP.y, -hRoomDelta.y);
                        WorldMode->CameraOffset = {0, -t * hRoomDelta.y, a * t * t + b * t};
                    }
                    
                    if (EntityP.x > hRoomApron.x) {
                        r32 t = Clamp01MapToRange(hRoomApron.x, EntityP.x, hRoomDelta.x);
                        WorldMode->CameraOffset = {t * hRoomDelta.x, 0, a * t * t + b * t} ;
                    }
                    if (EntityP.x < -hRoomApron.x) {
                        r32 t = Clamp01MapToRange(-hRoomApron.x, EntityP.x, -hRoomDelta.x);
                        WorldMode->CameraOffset = {-t * hRoomDelta.x, 0, a * t * t + b * t};
                    }
                    
                    
                    if (EntityP.z > hRoomApron.z) {
                        r32 t = Clamp01MapToRange(hRoomApron.z, EntityP.z, hRoomDelta.z);
                        WorldMode->CameraOffset = {0, 0, t * hRoomDelta.z} ;
                    }
                    if (EntityP.z < -hRoomApron.z) {
                        r32 t = Clamp01MapToRange(-hRoomApron.z, EntityP.z, -hRoomDelta.z);
                        WorldMode->CameraOffset = {0, 0, -t * hRoomDelta.z};
                    }
                } else {
                    //r32 CamZOffset = NewCameraP.Offset_.z;
                    NewCameraP = EntityChunkP;
                    //NewCameraP.Offset_.z = CamZOffset;
                }
                WorldMode->CameraP = NewCameraP;
            }
            
            Entity->P += ChunkDelta;
            PackEntityIntoWorld(World, SimRegion, Entity, EntityChunkP);
        }
	}
}

internal b32
CanCollide(game_mode_world* WorldMode, entity* A, entity* B) {
	b32 Result = false;
	if (A != B) {
		if (A->ID.Value > B->ID.Value) {
			entity* Temp = A;
			A = B;
			B = Temp;
        }
        if (IsSet(A, EntityFlag_Collides) && (IsSet(B, EntityFlag_Collides))) {
            Result = true;
            
            u32 HashBucket = A->ID.Value & (ArrayCount(WorldMode->CollisionRuleHash) - 1);
            for (pairwise_collision_rule* Rule = WorldMode->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
                if (Rule->EntityIDA == A->ID.Value && Rule->EntityIDB == B->ID.Value) {
                    Result = Rule->CanCollide;
                    break;
                }
            }
        }
    }
    
	return(Result);
}

internal b32
HandleCollision(game_mode_world* WorldMode, entity* A, entity* B) {
	b32 StopsOnCollision = false;
	
	if (A->Type > B->Type) {
		entity* Temp = A;
		A = B;
		B = Temp;
	}
	
	return(StopsOnCollision);
}

internal b32
CanOverlap(game_mode_world* WorldMode, entity* Mover, entity* Region) {
	b32 Result = false;
	if (Mover != Region) {
		if (Region->Type == EntityType_Stairwell) {
			Result = true;
		}
	}
	return(Result);
}

internal b32
SpeculativeCollide(entity* Mover, entity* Region, v3 TestP) {
    TIMED_FUNCTION();
	b32 Result = true;
	if (Region->Type == EntityType_Stairwell) {
		v3 MoverGroundPoint = GetEntityGroundPoint(Mover, TestP);
		r32 Ground = GetStairGround(Region, MoverGroundPoint);
		r32 StepHeight = 0.1f;
		Result = (AbsoluteValue(GetEntityGroundPoint(Mover).z - Ground) > StepHeight);
	}
    
    return(Result);
}
internal b32
TransactionalOccupy(entity *Entity, traversable_reference *Ref, traversable_reference Target) {
    b32 Result = false;
    entity_traversable_point *DesiredPoint = GetTraversable(Target);
    if (!DesiredPoint->Occupier) {
        entity_traversable_point *OriginalPoint = GetTraversable(*Ref);
        if (OriginalPoint) {
            OriginalPoint->Occupier = 0;
        }
        *Ref = Target;
        DesiredPoint->Occupier = Entity;
        Result = true;
    }
    return(Result);
}

internal void
MoveEntity(game_mode_world *WorldMode, sim_region* SimRegion, entity* Entity, r32 dt, move_spec* MoveSpec,
           v3 ddP) {
    TIMED_FUNCTION();
	world* World = SimRegion->World;
    
	if (MoveSpec->UnitMaxAccelVector) {
		r32 ddpLength = LengthSq(ddP);
		if (ddpLength > 1.0f) {
			ddP *= (1.0f / SquareRoot(ddpLength));
		}
	}
	ddP *= MoveSpec->Speed;
	v3 Drag = -MoveSpec->Drag * Entity->dP;
	Drag.z = 0;
	ddP += Drag;
    
	v3 OldPlayerP = Entity->P;
	v3 PlayerDelta = 0.5f * Square(dt) * ddP +
		dt * Entity->dP;
	Entity->dP = dt * ddP + Entity->dP;
	Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
	r32 DistanceRemaining = Entity->DistanceLimit;
	if (DistanceRemaining == 0.0f) {
		DistanceRemaining = 10000.0f;
	}
    
	for (u32 Iteration = 0; (Iteration < 4); ++Iteration) {
		r32 tMin = 1.0f;
		r32 tMax = 1.0f;
		r32 PlayerDeltaLength = Length(PlayerDelta);
		if (PlayerDeltaLength > 0.0f) {
			if (PlayerDeltaLength > DistanceRemaining) {
				tMin = (DistanceRemaining / PlayerDeltaLength);
			}
            
			v3 WallNormalMin = {};
			v3 WallNormalMax = {};
			entity* HitEntityMin = 0;
			entity* HitEntityMax = 0;
            
			v3 DesiredPosition = Entity->P + PlayerDelta;
            
            for (u32 TestHighEntityIndex = 0;
                 TestHighEntityIndex < SimRegion->EntityCount;
                 ++TestHighEntityIndex) {
                entity* TestEntity = SimRegion->Entities + TestHighEntityIndex;
                r32 OverlapEpsilon = 0.01f;
                if (CanCollide(WorldMode, Entity, TestEntity)) {
                    for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                        entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                        for (u32 TestVolumeIndex = 0; TestVolumeIndex < TestEntity->Collision->VolumeCount; TestVolumeIndex++) {
                            entity_collision_volume* TestVolume = TestEntity->Collision->Volumes + TestVolumeIndex;
                            
                            v3 MinkowskiDiameter = v3{ TestVolume->Dim.x + Volume->Dim.x, TestVolume->Dim.y + Volume->Dim.y, TestVolume->Dim.z + Volume->Dim.z };
                            
                            v3 MinCorner = -0.5f * MinkowskiDiameter;
                            v3 MaxCorner = 0.5f * MinkowskiDiameter;
                            
                            v3 Rel = (Entity->P + Volume->OffsetP) - (TestEntity->P + TestVolume->OffsetP);
                            if (Rel.z >= MinCorner.z && Rel.z < MaxCorner.z) {
                                test_wall Walls[] = {
                                    {MinCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, v3{-1, 0, 0}},
                                    {MaxCorner.x, Rel.x, Rel.y, PlayerDelta.x, PlayerDelta.y, MinCorner.y, MaxCorner.y, v3{1, 0, 0}},
                                    {MinCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, v3{0, 1, 0}},
                                    {MaxCorner.y, Rel.y, Rel.x, PlayerDelta.y, PlayerDelta.x, MinCorner.x, MaxCorner.x, v3{0, -1, 0}}
                                };
                                
                                {
                                    r32 tMinTest = tMin;
                                    b32 HitThis = false;
                                    v3 TestWallNormal = {};
                                    for (u32 WallIndex = 0; WallIndex < ArrayCount(Walls); ++WallIndex) {
                                        test_wall* Wall = Walls + WallIndex;
                                        r32 tEpsilon = 0.001f;
                                        
                                        if (Wall->DeltaX != 0.0f) {
                                            r32 tResult = (Wall->X - Wall->RelX) / Wall->DeltaX;
                                            r32 Y = Wall->RelY + tResult * Wall->DeltaY;
                                            if ((tResult >= 0.0f) && (tMinTest > tResult)) {
                                                if ((Y >= Wall->MinY) && (Y <= Wall->MaxY)) {
                                                    tMinTest = Maximum(0.0f, tResult - tEpsilon);
                                                    TestWallNormal = Wall->Normal;
                                                    HitThis = true;
                                                }
                                            }
                                        }
                                    }
                                    if (HitThis) {
                                        v3 TestP = Entity->P + tMinTest * PlayerDelta;
                                        if (SpeculativeCollide(Entity, TestEntity, TestP)) {
                                            HitEntityMin = TestEntity;
                                            tMin = tMinTest;
                                            WallNormalMin = TestWallNormal;
                                        }
                                    }
                                }
                                
                            }
                        }
                    }
                }
			}
			v3 WallNormal;
			entity* HitEntity;
			r32 tStop;
			if (tMin < tMax) {
				tStop = tMin;
				HitEntity = HitEntityMin;
				WallNormal = WallNormalMin;
			}
			else {
				tStop = tMax;
				HitEntity = HitEntityMax;
				WallNormal = WallNormalMax;
			}
            
            
			Entity->P += tStop * PlayerDelta;
			DistanceRemaining -= tStop * PlayerDeltaLength;
			if (HitEntity) {
				PlayerDelta = DesiredPosition - Entity->P;
                
				// b32 WasOverlapping = (OverlapIndex != OverlappingCount);
				b32 StopsOnCollision = HandleCollision(WorldMode, Entity, HitEntity);
                
				if (StopsOnCollision) {
					Entity->dP = Entity->dP - 1 * Inner(Entity->dP, WallNormal) * WallNormal;
                    
					PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;
				}
			}
			else {
				break;
			}
            
		}
		else {
			break;
		}
        
	}
    
	if (Entity->DistanceLimit != 0.0f) {
		Entity->DistanceLimit = DistanceRemaining;
	}
    
#if 0    
	if (Entity->dP.x == 0.0f && Entity->dP.y == 0.0f) {
		// remain whichever face direction it was
	}
	else {
		Entity->FacingDirection = Atan2(Entity->dP.y, Entity->dP.x);
	}
#endif
}

enum traversable_search_flag {
    TraversableSearch_Unoccupied = 0x1,
};

internal b32
GetClosestTraversable(sim_region *SimRegion, v3 FromP, traversable_reference *Result,
                      u32 Flags = 0) {
    b32 Found = false;
    r32 BestDistanceSq = Square(1000.0f);
    entity* TestEntity = SimRegion->Entities;
    for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
        for (u32 PIndex = 0; PIndex < TestEntity->TraversableCount; ++PIndex) {
            entity_traversable_point Point = GetSimEntityTraversable(TestEntity, PIndex);
            if (Point.Occupier == 0 || !(Flags & TraversableSearch_Unoccupied)) {
                v3 HeadToPoint = Point.P - FromP;
                // HeadToPoint.z 
                r32 TestDSq = LengthSq(HeadToPoint);
                if (BestDistanceSq > TestDSq) {
                    //*Result = P.P;
                    Result->Entity.Ptr = TestEntity;
                    Result->Index = PIndex;
                    BestDistanceSq = TestDSq;
                    Found = true;
                }
            }
        }
    }
    if (!Found) {
        Result->Entity.Ptr = 0;
        Result->Index = 0;
    }
    return(Found);
}
