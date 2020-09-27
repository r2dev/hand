
#include "handmade_sim_region.h"
#include "handmade_entity.h"
internal sim_entity_hash*
GetHashFromStorageIndex(sim_region* SimRegion, uint32 StorageIndex) {
	Assert(StorageIndex);
	sim_entity_hash* Result = 0;
	uint32 HashValue = StorageIndex;
	for (uint32 Offset = 0; Offset < ArrayCount(SimRegion->Hash); ++Offset) {
		uint32 HashIndex = ((HashValue + Offset) & (ArrayCount(SimRegion->Hash) - 1));
		sim_entity_hash* Entry = SimRegion->Hash + HashIndex;
		if (Entry->Index == 0 || Entry->Index == StorageIndex) {
			Result = Entry;
			break;
		}
	}
	return(Result);
}

internal sim_entity*
AddEntity(game_state* GameState, sim_region* SimRegion, uint32 StorageIndex, low_entity* Source, v3* SimP);

inline sim_entity*
GetEntityByStorageIndex(sim_region* SimRegion, uint32 StorageIndex) {
	sim_entity_hash* Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
	sim_entity* Result = Entry->Ptr;
	return(Result);
}


inline void
StoreEntityReference(entity_reference* Ref) {
	if (Ref->Ptr != 0) {
		Ref->Index = Ref->Ptr->StorageIndex;
	}
}

inline v3
GetSimSpaceP(sim_region* SimRegion, low_entity* Stored) {
	v3 Result = InvalidP;
	if (!IsSet(&Stored->Sim, EntityFlag_Nonspatial)) {
		Result =
			Subtract(SimRegion->World, &Stored->P, &SimRegion->Origin);
	}
	return(Result);
}

inline void
LoadEntityReference(game_state* GameState, sim_region* SimRegion,
	entity_reference* Ref) {
	if (Ref->Index != 0) {
		sim_entity_hash* Entry = GetHashFromStorageIndex(SimRegion, Ref->Index);
		if (Entry->Ptr == 0) {
			Entry->Index = Ref->Index;
			low_entity* LowEntitiy = GetLowEntity(GameState, Ref->Index);
			v3 SimP = GetSimSpaceP(SimRegion, LowEntitiy);
			Entry->Ptr = AddEntity(GameState, SimRegion, Ref->Index, LowEntitiy, &SimP);
		}

		Ref->Ptr = Entry->Ptr;
	}

}

internal sim_entity*
AddEntityRaw(game_state* GameState, sim_region* SimRegion, uint32 StorageIndex, low_entity* Source) {
	Assert(StorageIndex);
	sim_entity* Entity = 0;
	sim_entity_hash* Entry = GetHashFromStorageIndex(SimRegion, StorageIndex);
	if (Entry->Ptr == 0) {
		if (SimRegion->EntityCount < SimRegion->MaxEntityCount) {
			Entity = SimRegion->Entities + SimRegion->EntityCount++;
			Entry->Index = StorageIndex;
			Entry->Ptr = Entity;
			if (Source) {
				// decompression
				*Entity = Source->Sim;
				LoadEntityReference(GameState, SimRegion, &Entity->Sword);
				Assert(!IsSet(&Source->Sim, EntityFlag_Simming));
				AddFlags(&Source->Sim, EntityFlag_Simming);
			}
			Entity->StorageIndex = StorageIndex;
			Entity->Updatable = false;


		}
		else {
			InvalidCodePath
		}
	}
	return(Entity);

}

inline bool32
EntityOverlapsRectangle(v3 P, v3 Dim, rectangle3 Rect) {
	rectangle3 Grown = AddRadiusTo(Rect, 0.5f * Dim);
	bool32 Result = IsInRectangle(Grown, P);
	return(Result);
}

internal sim_entity*
AddEntity(game_state* GameState, sim_region* SimRegion, uint32 StorageIndex, low_entity* Source, v3* SimP) {
	sim_entity* Dest = AddEntityRaw(GameState, SimRegion, StorageIndex, Source);
	if (Dest) {

		if (SimP) {
			Dest->P = *SimP;
			Dest->Updatable = EntityOverlapsRectangle(Dest->P, Dest->Dim, SimRegion->UpdatableBounds);
		}
		else {
			Dest->P = GetSimSpaceP(SimRegion, Source);
		}
	}
	return(Dest);
}

internal sim_region*
BeginSim(memory_arena* SimArena, game_state* GameState, world* World, world_position Origin, rectangle3 Bounds, real32 dt) {
	sim_region* SimRegion = PushStruct(SimArena, sim_region);
	ZeroStruct(SimRegion->Hash);
	SimRegion->MaxEntityRadius = 5.0f;
	SimRegion->MaxEntityVelocity = 30.0f;
	real32 UpdateSafetyMargin = SimRegion->MaxEntityRadius + dt * SimRegion->MaxEntityVelocity;
	real32 UpdateSafetyMarginZ = 1.0f;
	SimRegion->World = World;
	SimRegion->Origin = Origin;
	SimRegion->UpdatableBounds = AddRadiusTo(Bounds, v3{ SimRegion->MaxEntityRadius, SimRegion->MaxEntityRadius, SimRegion->MaxEntityRadius });
	SimRegion->Bounds = AddRadiusTo(SimRegion->UpdatableBounds, v3{ UpdateSafetyMargin, UpdateSafetyMargin, UpdateSafetyMarginZ });

	SimRegion->MaxEntityCount = 4096;
	SimRegion->EntityCount = 0;
	SimRegion->Entities = PushArray(SimArena, SimRegion->MaxEntityCount, sim_entity);

	world_position MinChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMinCorner(SimRegion->Bounds));
	world_position MaxChunkP = MapIntoChunkSpace(World, SimRegion->Origin, GetMaxCorner(SimRegion->Bounds));
	for (int32 ChunkZ = MinChunkP.ChunkZ; ChunkZ <= MaxChunkP.ChunkZ; ++ChunkZ) {
		for (int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
			for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
				world_chunk* Chunk = GetWorldChunk(World, ChunkX, ChunkY, ChunkZ);
				if (Chunk) {
					for (world_entity_block* Block = &Chunk->FirstBlock; Block; Block = Block->Next) {
						for (uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
							uint32 LowEntityIndex = Block->LowEntityIndex[EntityIndex];
							low_entity* Low = GameState->LowEntities + LowEntityIndex;
							if (!IsSet(&Low->Sim, EntityFlag_Nonspatial)) {
								v3 SimSpaceP = GetSimSpaceP(SimRegion, Low);
								if (EntityOverlapsRectangle(SimSpaceP, Low->Sim.Dim, SimRegion->Bounds)) {
									AddEntity(GameState, SimRegion, LowEntityIndex, Low, &SimSpaceP);
								}
							}
						}
					}
				}
			}
		}
	}
	return(SimRegion);
}

internal void
EndSim(sim_region* Region, game_state* GameState) {
	sim_entity* Entity = Region->Entities;
	for (uint32 EntityIndex = 0; EntityIndex < Region->EntityCount; ++EntityIndex, ++Entity) {
		low_entity* Stored = GameState->LowEntities + Entity->StorageIndex;

		Assert(IsSet(&Stored->Sim, EntityFlag_Simming));

		Stored->Sim = *Entity;
		Assert(!IsSet(&Stored->Sim, EntityFlag_Simming));
		StoreEntityReference(&Stored->Sim.Sword);

		world_position NewP = IsSet(Entity, EntityFlag_Nonspatial) ? NullPosition() :
			MapIntoChunkSpace(GameState->World, Region->Origin, Entity->P);
		ChangeEntityLocation(&GameState->WorldArena, GameState->World,
			Entity->StorageIndex, Stored, NewP);
		if (Entity->StorageIndex == GameState->CameraFollowingEntityIndex) {
			world_position NewCameraP = GameState->CameraP;
			NewCameraP.ChunkZ = Stored->P.ChunkZ;
#if 0
			if (CameraFollowingEntity.High->P.X > (9.0f * World->TileSideInMeters)) {
				NewCameraP.AbsTileX += 17;
			}
			if (CameraFollowingEntity.High->P.X < -(9.0f * World->TileSideInMeters)) {
				NewCameraP.AbsTileX -= 17;
			}
			if (CameraFollowingEntity.High->P.Y > (5.0f * World->TileSideInMeters)) {
				NewCameraP.AbsTileY += 9;
			}
			if (CameraFollowingEntity.High->P.Y < -(5.0f * World->TileSideInMeters)) {
				NewCameraP.AbsTileY -= 9;
			}
#else
			NewCameraP = Stored->P;
#endif
			GameState->CameraP = NewCameraP;
		}
	}
}





internal bool32
CanCollide(game_state* GameState, sim_entity* A, sim_entity* B) {
	bool32 Result = false;
	if (A != B) {
		if (A->StorageIndex > B->StorageIndex) {
			sim_entity* Temp = A;
			A = B;
			B = Temp;
		}
		if (!IsSet(A, EntityFlag_Nonspatial) && (!IsSet(B, EntityFlag_Nonspatial))) {
			Result = true;
		}
		if (A->Type == EntityType_Stairwell || B->Type == EntityType_Stairwell) {
			Result = false;
		}
		uint32 HashBucket = A->StorageIndex & (ArrayCount(GameState->CollisionRuleHash) - 1);
		for (pairwise_collision_rule* Rule = GameState->CollisionRuleHash[HashBucket]; Rule; Rule = Rule->NextHash) {
			if (Rule->StorageIndexA == A->StorageIndex && Rule->StorageIndexB == B->StorageIndex) {
				Result = Rule->CanCollide;
				break;
			}
		}
	}


	return(Result);
}

internal bool32
HandleCollision(game_state* GameState, sim_entity* A, sim_entity* B) {
	bool32 StopsOnCollision = false;
	if (A->Type == EntityType_Sword) {
		AddCollisionRule(GameState, A->StorageIndex, B->StorageIndex, false);
		StopsOnCollision = false;
	}
	else {
		StopsOnCollision = true;
	}
	if (A->Type > B->Type) {
		sim_entity* Temp = A;
		A = B;
		B = Temp;
	}
	if (A->Type == EntityType_Monster && B->Type == EntityType_Sword) {
		if (A->HitPointMax > 0) {
			--A->HitPointMax;
		}
	}

	return(StopsOnCollision);
}

internal bool32
CanOverlap(game_state* GameState, sim_entity* Mover, sim_entity* Region) {
	bool32 result = false;
	if (Mover != Region) {
		if (Region->Type == EntityType_Stairwell) {
			result = true;
		}
	}
	return(result);
}

internal void
HandleOverlap(game_state* GameState, sim_entity* Mover, sim_entity* Region, real32 dt, real32 *Ground) {
	if (Region->Type == EntityType_Stairwell) {
		rectangle3 RegionRect = RectCenterDim(Region->P, Region->Dim);
		v3 Bary = Clamp01(GetBarycentric(RegionRect, Mover->P));

		*Ground = Lerp(RegionRect.Min.Z, Bary.Y, RegionRect.Max.Z);
	}
}

internal void
MoveEntity(game_state* GameState, sim_region* SimRegion, sim_entity* Entity, real32 dt, move_spec* MoveSpec,
	v3 ddP) {
	Assert(!IsSet(Entity, EntityFlag_Nonspatial));
	world* World = SimRegion->World;

	if (MoveSpec->UnitMaxAccelVector) {
		real32 ddpLength = LengthSq(ddP);
		if (ddpLength > 1.0f) {
			ddP *= (1.0f / SquareRoot(ddpLength));
		}
	}

	ddP *= MoveSpec->Speed;
	ddP += v3{ 0, 0, -9.8f };
	ddP += -MoveSpec->Drag * Entity->dP;
	v3 OldPlayerP = Entity->P;
	v3 PlayerDelta = 0.5f * Square(dt) * ddP +
		dt * Entity->dP;
	Entity->dP = dt * ddP + Entity->dP;
	Assert(LengthSq(Entity->dP) <= Square(SimRegion->MaxEntityVelocity));
	real32 DistanceRemaining = Entity->DistanceLimit;
	if (DistanceRemaining == 0.0f) {
		DistanceRemaining = 10000.0f;
	}

	for (uint32 Iteration = 0; (Iteration < 4); ++Iteration) {
		real32 tMin = 1.0f;
		real32 PlayerDeltaLength = Length(PlayerDelta);
		if (PlayerDeltaLength > 0.0f) {
			if (PlayerDeltaLength > DistanceRemaining) {
				tMin = (DistanceRemaining / PlayerDeltaLength);
			}

			v3 WallNormal = {};
			sim_entity* Hit = 0;

			v3 DesiredPosition = Entity->P + PlayerDelta;

			if (!IsSet(Entity, EntityFlag_Nonspatial)) {
				for (uint32 TestHighEntityIndex = 0;
					TestHighEntityIndex < SimRegion->EntityCount;
					++TestHighEntityIndex) {
					sim_entity* TestEntity = SimRegion->Entities + TestHighEntityIndex;
					if (CanCollide(GameState, Entity, TestEntity) && (TestEntity->P.Z == Entity->P.Z)) {
						v3 MinkowskiDiameter = v3{ TestEntity->Dim.X + Entity->Dim.X, TestEntity->Dim.Y + Entity->Dim.Y, TestEntity->Dim.Z + Entity->Dim.Z };

						v3 MinCorner = -0.5f * MinkowskiDiameter;
						v3 MaxCorner = 0.5f * MinkowskiDiameter;

						v3 Rel = Entity->P - TestEntity->P;

						//tResult = (MinCorner.X - RelOldPlayerP.dXY.X) / PlayerDelta.X;
						if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X,
							PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y)) {
							WallNormal = v3{ -1, 0, 0 };
							Hit = TestEntity;
						}
						if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X,
							PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y)) {
							WallNormal = v3{ 1, 0, 0 };
							Hit = TestEntity;
						}
						if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
							PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X)) {
							WallNormal = v3{ 0, 1, 0 };
							Hit = TestEntity;
						}
						if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
							PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X)) {
							WallNormal = v3{ 0, -1, 0 };
							Hit = TestEntity;
						}

					}
				}
			}
			Entity->P += tMin * PlayerDelta;
			DistanceRemaining -= tMin * PlayerDeltaLength;
			if (Hit) {
				PlayerDelta = DesiredPosition - Entity->P;
				
				// bool32 WasOverlapping = (OverlapIndex != OverlappingCount);
				bool32 StopsOnCollision = HandleCollision(GameState, Entity, Hit);

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
	
	real32 Ground = 0.0f;
	{
		rectangle3 EntityRect = RectCenterDim(Entity->P, Entity->Dim);
		for (uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex) {
			sim_entity* TestEntity = SimRegion->Entities + TestEntityIndex;
			if (CanOverlap(GameState, Entity, TestEntity)) {
				rectangle3 TestEntityRect = RectCenterDim(TestEntity->P, TestEntity->Dim);
				if (RectangleIntersect(EntityRect, TestEntityRect)) {
					HandleOverlap(GameState, Entity, TestEntity, dt, &Ground);
				}
			}
		}
	}


	if (Entity->P.Z < Ground) {
		Entity->P.Z = Ground;
		Entity->dP.Z = 0;
	}
	if (Entity->DistanceLimit != 0.0f) {
		Entity->DistanceLimit = DistanceRemaining;
	}

	if (Entity->dP.X == 0.0f && Entity->dP.Y == 0.0f) {

	}
	else if (AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y)) {
		if (Entity->dP.X > 0) {
			Entity->FacingDirection = 0;
		}
		else {
			Entity->FacingDirection = 2;
		}
	}
	else if (AbsoluteValue(Entity->dP.X) < AbsoluteValue(Entity->dP.Y)) {
		if (Entity->dP.Y > 0) {
			Entity->FacingDirection = 1;
		}
		else {
			Entity->FacingDirection = 3;
		}
	}
}


