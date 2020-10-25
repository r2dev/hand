
#include "handmade_world.h"
#include "handmade.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILES_PER_CHUNK 8

inline world_position
NullPosition() {
	world_position Result = {};
	Result.ChunkX = TILE_CHUNK_UNINITIALIZED;
	return(Result);
}

inline bool32
IsValid(world_position P) {
	bool32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
	return(Result);
}

inline bool32
IsCanonical(real32 ChunkDim, real32 TileRel) {
	real32 Epsilon = 0.0001f;
	bool32 Result = ((TileRel >= -(0.5f * ChunkDim + Epsilon))
		&& (TileRel <= (0.5f * ChunkDim + Epsilon)));
	return(Result);
}

inline bool32
IsCanonical(world* World, v3 Offset) {
	bool32 Result = ((IsCanonical(World->ChunkDimInMeters.X, Offset.X)) 
		&& (IsCanonical(World->ChunkDimInMeters.Y, Offset.Y))
		&& (IsCanonical(World->ChunkDimInMeters.Z, Offset.Z))
		);
	return(Result);
}

inline void
RecanonicalizeCoord(real32 ChunkDimInMeters, int32* TileV, real32* TileRelV) {
	int32 Offset = RoundReal32ToInt32(*TileRelV / ChunkDimInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * ChunkDimInMeters;
	Assert(IsCanonical(ChunkDimInMeters, *TileRelV));

}

inline world_position
MapIntoChunkSpace(world* World, world_position BasePos, v3 Offset) {
	world_position Result = BasePos;
	Result.Offset_ += Offset;
	RecanonicalizeCoord(World->ChunkDimInMeters.X, &Result.ChunkX, &Result.Offset_.X);
	RecanonicalizeCoord(World->ChunkDimInMeters.Y, &Result.ChunkY, &Result.Offset_.Y);
	RecanonicalizeCoord(World->ChunkDimInMeters.Z, &Result.ChunkZ, &Result.Offset_.Z);
	return(Result);
}

inline world_position 
CenteredChunkPoint(uint32 ChunkX, uint32 ChunkY, uint32 ChunkZ) {
	world_position Result = {};
	Result.ChunkX = ChunkX;
	Result.ChunkY = ChunkY;
	Result.ChunkZ = ChunkZ;
	return(Result);
}

inline world_position
CenteredChunkPoint(world_chunk * Chunk) {
	return CenteredChunkPoint(Chunk->ChunkX, Chunk->ChunkY, Chunk->ChunkZ);
}

inline world_chunk*
GetWorldChunk(world* World, int32 ChunkX, int32 ChunkY, int32 ChunkZ, memory_arena* Arena = 0) {

	Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);

	Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);

	uint32 HashValue = 18 * ChunkX + 13 * ChunkY + 3 * ChunkZ;
	uint32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
	Assert(HashSlot < ArrayCount(World->ChunkHash));
	world_chunk* Chunk = World->ChunkHash + HashSlot;
	do {
		if (Chunk->ChunkX == ChunkX && Chunk->ChunkY == ChunkY && Chunk->ChunkZ == ChunkZ) {
			break;
		}

		if (Arena && Chunk->ChunkX != TILE_CHUNK_UNINITIALIZED && !(Chunk->NextInHash)) {
			Chunk->NextInHash = PushStruct(Arena, world_chunk);
			Chunk = Chunk->NextInHash;
			Chunk->ChunkX = TILE_CHUNK_UNINITIALIZED;
		}
		if (Arena && Chunk->ChunkX == TILE_CHUNK_UNINITIALIZED) {

			Chunk->ChunkX = ChunkX;
			Chunk->ChunkY = ChunkY;
			Chunk->ChunkZ = ChunkZ;

			Chunk->NextInHash = 0;
			break;
		}
		Chunk = Chunk->NextInHash;
	} while (Chunk);

	return(Chunk);
}

/*inline bool32
AreOnSameTile(world_position* A, world_position* B) {
	bool32 Result = (A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ);
	return Result;
}*/

inline bool32
AreInSameChunk(world* World, world_position* A, world_position* B) {
	Assert(IsCanonical(World, A->Offset_));
	Assert(IsCanonical(World, B->Offset_));
	bool32 Result = (A->ChunkX == B->ChunkX
		&& A->ChunkY == B->ChunkY && A->ChunkZ == B->ChunkZ);
	return(Result);
}


v3
Subtract(world* World, world_position* A, world_position* B) {
	v3 dTile = {
		(real32)A->ChunkX - (real32)B->ChunkX,
		(real32)A->ChunkY - (real32)B->ChunkY,
		(real32)A->ChunkZ - (real32)B->ChunkZ
	};
	
	v3 Result = Hadamard(World->ChunkDimInMeters, dTile) + (A->Offset_ - B->Offset_);
	return(Result);
}


void
InitializeWorld(world* World, v3 ChunkDimInMeter) {

	//World->TileSideInMeters = TileSideInMeter;
	// = v3{ (real32)TILES_PER_CHUNK * TileSideInMeter, (real32)TILES_PER_CHUNK * TileSideInMeter, (real32)TileDepthInMeter };
	//World->TileDepthInMeters = (real32)TileDepthInMeter;

	World->ChunkDimInMeters = ChunkDimInMeter;
	World->FirstFree = 0;

	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex) {
		World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
		World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
	}
}


inline void
ChangeEntityLocationRaw(memory_arena* Arena, world* World, uint32 LowEntityIndex, 
	world_position* OldP, world_position* NewP) {
	Assert(!OldP || IsValid(*OldP));
	Assert(!NewP || IsValid(*NewP));
	if (OldP && NewP && AreInSameChunk(World, OldP, NewP)) {

	}
	else {
		if (OldP) {
			world_chunk* Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
			Assert(Chunk);
			if (Chunk) {
				bool32 NotFound = true;
				world_entity_block* FirstBlock = &Chunk->FirstBlock;
				for (world_entity_block* Block = FirstBlock; Block && NotFound; Block = Block->Next) {
					for (uint32 Index = 0; ((Index < Block->EntityCount) && NotFound); ++Index) {
						if (Block->LowEntityIndex[Index] == LowEntityIndex) {

							Block->LowEntityIndex[Index]
								= FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];

							if (FirstBlock->EntityCount == 0) {
								if (FirstBlock->Next) {
									world_entity_block* NextBlock = FirstBlock->Next;
									*FirstBlock = *NextBlock;

									NextBlock->Next = World->FirstFree;
									World->FirstFree = NextBlock;
								}
							}
							NotFound = 0;
						}
					}
				}
			}

		}
		if (NewP) {
			world_chunk* TargetChunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
			world_entity_block* Block = &TargetChunk->FirstBlock;
			if (Block->EntityCount == ArrayCount(Block->LowEntityIndex)) {
				world_entity_block* OldBlock = World->FirstFree;
				if (OldBlock) {
					World->FirstFree = OldBlock->Next;
				}
				else {
					OldBlock = PushStruct(Arena, world_entity_block);
				}
				*OldBlock = *Block;
				Block->Next = OldBlock;
				Block->EntityCount = 0;
			}

			Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
			Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
		}
	}


}

internal void
ChangeEntityLocation(memory_arena* Arena, world* World,
	uint32 LowEntityIndex, low_entity* LowEntity,
	world_position NewPInit) {
	world_position* OldP = 0;
	world_position* NewP = 0;
	if (!IsSet(&LowEntity->Sim, EntityFlag_Nonspatial) && IsValid(LowEntity->P)) {
		OldP = &LowEntity->P;
	}
	if (IsValid(NewPInit)) {
		NewP = &NewPInit;
	}

	ChangeEntityLocationRaw(Arena, World, LowEntityIndex, OldP, NewP);
	if (NewP) {
		LowEntity->P = *NewP;
		ClearFlags(&LowEntity->Sim, EntityFlag_Nonspatial);
	}
	else {
		LowEntity->P = NullPosition();
		AddFlags(&LowEntity->Sim, EntityFlag_Nonspatial);
	}
}