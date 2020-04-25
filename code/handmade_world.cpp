
#include "handmade_world.h"
#include "handmade.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX
#define TILE_PER_CHUNK 16

inline bool32
IsCanonical(world* World, real32 TileRel) {
	bool32 Result = ((TileRel >= -0.5f * World->ChunkSideInMeters)
		&& (TileRel <= 0.5f * World->ChunkSideInMeters));
	return(Result);
}

inline bool32
IsCanonical(world* World, v2 Offset) {
	bool32 Result = ((IsCanonical(World, Offset.X)) && (IsCanonical(World, Offset.Y)));
	return(Result);
}

inline void
RecanonicalizeCoord(world* World, int32* TileV, real32* TileRelV) {
	int32 Offset = RoundReal32ToInt32(*TileRelV / World->ChunkSideInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * World->ChunkSideInMeters;
	Assert(IsCanonical(World, *TileRelV));
	
}

inline world_position
MapIntoTileSpace(world* World, world_position BasePos, v2 Offset) {
	world_position Result = BasePos;
	Result.Offset_ += Offset;
	RecanonicalizeCoord(World, &Result.ChunkX, &Result.Offset_.X);
	RecanonicalizeCoord(World, &Result.ChunkY, &Result.Offset_.Y);
	return(Result);
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
AreInSameChunk(world *World, world_position* A, world_position* B) {
	Assert(IsCanonical(World, A->Offset_));
	Assert(IsCanonical(World, B->Offset_));
	bool32 Result = (A->ChunkX == B->ChunkX
		&& A->ChunkY == B->ChunkY && A->ChunkZ == B->ChunkZ);
	return false;
}


world_difference
Subtract(world* World, world_position* A, world_position* B) {
	world_difference Result = {};
	v2 dTileXY = {
		(real32)A->ChunkX - (real32)B->ChunkX,
		(real32)A->ChunkY - (real32)B->ChunkY
	};
	real32 dTileZ = (real32)A->ChunkZ - (real32)B->ChunkZ;
	Result.dXY = World->ChunkSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
	Result.dZ = dTileZ * World->ChunkSideInMeters;
	return(Result);
}

inline world_position
CenteredTilePoint(uint32 ChunkX, uint32 ChunkY, uint32 ChunkZ) {
	world_position Result = {};
	Result.ChunkX = ChunkX;
	Result.ChunkY = ChunkY;
	Result.ChunkZ = ChunkZ;
	return(Result);
}

void
InitializeWorld(world* World, real32 TileSideInMeter) {

	World->TileSideInMeters = TileSideInMeter;
	World->ChunkSideInMeters = (real32)TILE_PER_CHUNK * TileSideInMeter;
	World->FirstFree = 0;

	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex) {
		World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
		World->ChunkHash[ChunkIndex].FirstBlock.EntityCount = 0;
	}
}


inline void
ChangeEntityLocation(memory_arena* Arena, world* World, uint32 LowEntityIndex,
	world_position* OldP, world_position* NewP) {
	if (OldP && AreInSameChunk(World, OldP, NewP)) {

	}
	else {
		if (OldP) {
			world_chunk* Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
			Assert(Chunk);
			if (Chunk) {
				world_entity_block* FirstBlock = &Chunk->FirstBlock;
				for (world_entity_block* Block = FirstBlock; Block; Block = Block->Next) {
					for (uint32 Index = 0; Index < Block->EntityCount; Index++) {
						if (Block->LowEntityIndex[Index] == LowEntityIndex) {

							Block->LowEntityIndex[Index]
								= FirstBlock->LowEntityIndex[--FirstBlock->EntityCount];

							if (FirstBlock->EntityCount == 0) {
								if (FirstBlock->Next) {
									world_entity_block* NextBlock = FirstBlock->Next;
									FirstBlock = NextBlock;

									NextBlock->Next = World->FirstFree;
									World->FirstFree = NextBlock;
								}
							}
							Block = 0;     
							break;
						}
					}
				}
			}

		}
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

inline world_position
ChunkPositionFromTilePosition(world * World, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position Result = {};
	Result.ChunkX = AbsTileX / TILE_PER_CHUNK;
	Result.ChunkY = AbsTileY / TILE_PER_CHUNK;
	Result.ChunkY = AbsTileY / TILE_PER_CHUNK;
	Result.Offset_ = v2{
		(real32)(Result.ChunkX - (Result.ChunkX * TILE_PER_CHUNK)) * World->ChunkSideInMeters,
		(real32)(Result.ChunkY - (Result.ChunkY * TILE_PER_CHUNK)) * World->ChunkSideInMeters
	};
	return(Result);
}