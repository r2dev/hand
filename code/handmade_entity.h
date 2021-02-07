#pragma once

#define InvalidP v3{100000.0f, 100000.0f, 100000.0f}
inline b32
IsSet(entity* Entity, u32 Flag) {
	b32 Result = Entity->Flags & Flag;
	return(Result);
}

inline void
AddFlags(entity* Entity, u32 Flags) {
	Entity->Flags |= Flags;
}

inline void
ClearFlags(entity* Entity, u32 Flags) {
	Entity->Flags &= ~Flags;
}

internal v3
GetEntityGroundPoint(entity* Entity, v3 ForEntityP) {
	return(ForEntityP);
}

internal v3
GetEntityGroundPoint(entity* Entity) {
	v3 Result = Entity->P;
	return(Result);
}

inline r32
GetStairGround(entity* Entity, v3 AtGroundPoint) {
	Assert(Entity->Type == EntityType_Stairwell);
	rectangle2 RegionRect = RectCenterDim(Entity->P.xy, Entity->WalkableDim);
	v2 Bary = Clamp01(GetBarycentric(RegionRect, AtGroundPoint.xy));
	r32 Result = Entity->P.z + Bary.y * Entity->WalkableHeight;
	return(Result);
}