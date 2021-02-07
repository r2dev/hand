inline entity_traversable_point
GetSimEntityTraversable(entity *Entity, u32 Index) {
    Assert(Index < Entity->Collision->TraversableCount);
    entity_traversable_point Result = Entity->Collision->Traversables[Index];
    Result.P += Entity->P;
    return(Result);
}

internal entity_hash *
GetHashFromID(sim_region* SimRegion, entity_id ID) {
	Assert(ID.Value);
	entity_hash* Result = 0;
	u32 HashValue = ID.Value;
	for (u32 Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset) {
		u32 HashIndex = ((HashValue + Offset) & (ArrayCount(SimRegion->Hash) - 1));
		entity_hash* Entry = SimRegion->Hash + HashIndex;
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
	entity* Result = Entry->Ptr;
	return(Result);
}


inline void
StoreEntityReference(entity_reference* Ref) {
	if (Ref->Ptr != 0) {
        Ref->ID = Ref->Ptr->ID;
	}
}

inline v3
GetSimSpaceP(sim_region* SimRegion, entity *Stored) {
	v3 Result = InvalidP;
    Result = Subtract(SimRegion->World, &Stored->ChunkP, &SimRegion->Origin);
	return(Result);
}

inline void
LoadEntityReference(game_mode_world* GameWorld, sim_region* SimRegion, entity_reference* Ref) {
	if (Ref->ID.Value != 0) {
		entity_hash* Entry = GetHashFromID(SimRegion, Ref->ID);
		Ref->Ptr = Entry ? Entry->Ptr: 0;
	}
}

internal entity *
AddEntityRaw(game_mode_world* GameWorld, sim_region* SimRegion, entity_id ID, entity *Source) {
	Assert(ID.Value);
	entity* Entity = 0;
	entity_hash* Entry = GetHashFromID(SimRegion, ID);
	if (Entry->Ptr == 0) {
		if (SimRegion->EntityCount < SimRegion->MaxEntityCount) {
			Entity = SimRegion->Entities + SimRegion->EntityCount++;
			Entry->ID = ID;
			Entry->Ptr = Entity;
			if (Source) {
				// decompression
				*Entity = *Source;
                LoadEntityReference(GameWorld, SimRegion, &Entity->Head);
			}
			Entity->Updatable = false;
		}
		else {
			InvalidCodePath
		}
	}
	return(Entity);
    
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

internal entity*
AddEntity(game_mode_world* GameWorld, sim_region* SimRegion, entity* Source, v3* SimP) {
    entity_id ID = Source->ID;
	entity* Dest = AddEntityRaw(GameWorld, SimRegion, ID, Source);
	if (Dest) {
		if (SimP) {
			Dest->P = *SimP;
			Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Collision->TotalVolume, SimRegion->UpdatableBounds);
		}
		else {
			Dest->P = GetSimSpaceP(SimRegion, Source);
		}
	}
	return(Dest);
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


internal sim_region*
BeginSim(memory_arena* SimArena, game_mode_world* GameWorld, world* World, world_position Origin, rectangle3 Bounds, r32 dt) {
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
    
	world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
	for (s32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ) {
		for (s32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
			for (s32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
				world_chunk* Chunk = RemoveWorldChunk(World, ChunkX, ChunkY, ChunkZ);
				if (Chunk) {
                    world_entity_block* Block = Chunk->FirstBlock;
                    while (Block) {
                        for (u32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
							entity *Entity = (entity *)Block->EntityData + EntityIndex;
                            v3 SimSpaceP = GetSimSpaceP(SimRegion, Entity);
                            if (EntityOverlapsRectangle(SimSpaceP, Entity->Collision->TotalVolume, SimRegion->Bounds)) {
                                AddEntity(GameWorld, SimRegion, Entity, &SimSpaceP);
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
	return(SimRegion);
}

internal void
EndSim(sim_region* Region, game_mode_world* GameWorld) {
    TIMED_FUNCTION();
	entity* Entity = Region->Entities;
    world *World = GameWorld->World;
	for (u32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity) {
        StoreEntityReference(&Entity->Head);
        world_position ChunkP = MapIntoChunkSpace(GameWorld->World, Region->Origin, Entity->P);
		if (Entity->ID.Value == GameWorld->CameraFollowingEntityID.Value) {
			world_position NewCameraP = GameWorld->CameraP;
			NewCameraP.ChunkZ = Entity->ChunkP.ChunkZ;
            if(Global_Sim_RoomBaseCamera)
            {
                if (Entity->P.x > (9.0f)) {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, v3{18.0f, 0, 0});
                }
                if (Entity->P.x < -(9.0f)) {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, v3{-18.0f, 0, 0});
                }
                if (Entity->P.y > (5.0f)) {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, v3{0, 10.0f, 0});
                }
                if (Entity->P.y < -(5.0f)) {
                    NewCameraP = MapIntoChunkSpace(World, NewCameraP, v3{0, -10.0f, 0});
                }
            } else {
                //r32 CamZOffset = NewCameraP.Offset_.z;
                NewCameraP = Entity->ChunkP;
                //NewCameraP.Offset_.z = CamZOffset;
            }
			GameWorld->CameraP = NewCameraP;
		}
        PackEntityIntoWorld(World, Entity, ChunkP);
	}
}

internal b32
CanCollide(game_mode_world* GameWorld, entity* A, entity* B) {
	b32 Result = false;
	if (A != B) {
		if (A->ID.Value > B->ID.Value) {
			entity* Temp = A;
			A = B;
			B = Temp;
        }
        if (IsSet(A, EntityFlag_Collides) && (IsSet(B, EntityFlag_Collides))) {
            Result = true;
            
            u32 HashBucket = A->ID.Value & (ArrayCount(GameWorld->CollisionRuleHash) - 1);
            for (pairwise_collision_rule* Rule = GameWorld->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
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
HandleCollision(game_mode_world* GameWorld, entity* A, entity* B) {
	b32 StopsOnCollision = false;
	
	if (A->Type > B->Type) {
		entity* Temp = A;
		A = B;
		B = Temp;
	}
	
	return(StopsOnCollision);
}

internal b32
CanOverlap(game_mode_world* GameWorld, entity* Mover, entity* Region) {
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


internal void
MoveEntity(game_mode_world *GameWorld, sim_region* SimRegion, entity* Entity, r32 dt, move_spec* MoveSpec,
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
                if (CanCollide(GameWorld, Entity, TestEntity)) {
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
				b32 StopsOnCollision = HandleCollision(GameWorld, Entity, HitEntity);
                
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


