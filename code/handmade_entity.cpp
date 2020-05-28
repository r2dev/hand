#include "handmade.h"

inline move_spec
DefaultMoveSpec() {
	move_spec Result = {};
	Result.UnitMaxAccelVector = false;
	Result.Speed = 1.0f;
	Result.Drag = 0.0f;
	return(Result);
}


internal void
UpdateFamiliar(sim_region* SimRegion, sim_entity * Entity, real32 dt) {
	sim_entity * ClosestHero = 0;
	real32 ClosestHeroSq = Square(10.0f);
	sim_entity* TestEntity = SimRegion->Entities;
	for (uint32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex) {
		if (TestEntity->Type == EntityType_Hero) {
			real32 TestDSq = LengthSq(TestEntity->P - Entity->P);
			TestDSq *= 0.75f;
			if (ClosestHeroSq > TestDSq) {
				ClosestHero = TestEntity;
				ClosestHeroSq = TestDSq;
			}
		}
	}
	v2 ddp = {};
	if (ClosestHero && (ClosestHeroSq > Square(3.0f))) {
		real32 Acceleration = 0.5f;
		real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
		ddp = OneOverLength * (ClosestHero->P - Entity->P);
	}
	move_spec MoveSpec = DefaultMoveSpec();
	MoveSpec.UnitMaxAccelVector = true;
	MoveSpec.Speed = 50.0f;
	MoveSpec.Drag = 8.0f;
	MoveEntity(SimRegion, Entity, dt, &MoveSpec, ddp);
}

internal void
UpdateSword(sim_region* SimRegion, sim_entity *Entity, real32 dt) {
	if (IsSet(Entity, EntityFlag_Nonsptial)) {

	}
	else {
		move_spec MoveSpec = DefaultMoveSpec();
		MoveSpec.UnitMaxAccelVector = false;
		MoveSpec.Speed = 0.0f;
		MoveSpec.Drag = 0.0f;

		v2 OldP = Entity->P;
		MoveEntity(SimRegion, Entity, dt, &MoveSpec, v2{ 0, 0 });
		real32 DistanceTraveled = Length(Entity->P - OldP);
		Entity->DistanceRemaining -= DistanceTraveled;
		if (Entity->DistanceRemaining < 0.0f) {
			MakeEntityNonSpatial(Entity);
			
		}
	}
}