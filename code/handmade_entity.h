#pragma once

#define InvalidP v3{100000.0f, 100000.0f, 100000.0f}
inline b32
IsSet(sim_entity* Entity, u32 Flag) {
	b32 Result = Entity->Flags & Flag;
	return(Result);
}

inline void
AddFlags(sim_entity* Entity, u32 Flags) {
	Entity->Flags |= Flags;
}

inline void
ClearFlags(sim_entity* Entity, u32 Flags) {
	Entity->Flags &= ~Flags;
}

inline void
MakeEntityNonSpatial(sim_entity* Entity) {
	AddFlags(Entity, EntityFlag_Nonspatial);
	Entity->P = InvalidP;
}

inline void
MakeEntitySpatial(sim_entity* Entity, v3 P, v3 dP) {
	ClearFlags(Entity, EntityFlag_Nonspatial);
	Entity->P = P;
	Entity->dP = dP;
}

internal v3
GetEntityGroundPoint(sim_entity* Entity, v3 ForEntityP) {
	return(ForEntityP);
}

internal v3
GetEntityGroundPoint(sim_entity* Entity) {
	v3 Result = Entity->P;
	return(Result);
}

inline r32
GetStairGround(sim_entity* Entity, v3 AtGroundPoint) {
	Assert(Entity->Type == EntityType_Stairwell);
	rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
	v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
	r32 Result = Entity->P.z + Bary.y * Entity->WalkableHeight;
	return(Result);
}