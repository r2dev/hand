
#include "handmade_world.h"
#include "handmade.h"

#define TILE_CHUNK_SAFE_MARGIN (INT32_MAX / 64)
#define TILE_CHUNK_UNINITIALIZED INT32_MAX

inline void
RecanonicalizeCoord(world* World, int32* TileV, real32* TileRelV) {
	int32 Offset = RoundReal32ToInt32(*TileRelV / World->TileSideInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * World->TileSideInMeters;

	Assert(*TileRelV >= -0.5f * World->TileSideInMeters);
	Assert(*TileRelV <= 0.5f * World->TileSideInMeters)
}

inline world_position
MapIntoTileSpace(world* World, world_position BasePos, v2 Offset) {
	world_position Result = BasePos;
	Result.Offset_ += Offset;
	RecanonicalizeCoord(World, &Result.AbsTileX, &Result.Offset_.X);
	RecanonicalizeCoord(World, &Result.AbsTileY, &Result.Offset_.Y);
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


#if 0
inline tile_chunk_position
GetChunkPositionFor(world* World, uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
	tile_chunk_position Result;
	Result.WorldChunkX = AbsTileX >> World->ChunkShift;
	Result.WorldChunkY = AbsTileY >> World->ChunkShift;
	Result.WorldChunkZ = AbsTileZ;
	Result.RelTileX = AbsTileX & World->ChunkMask;
	Result.RelTileY = AbsTileY & World->ChunkMask;
	return(Result);
}

#endif

inline bool32
AreOnSameTile(world_position* A, world_position* B) {
	bool32 Result = (A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ);
	return Result;
}


world_difference
Subtract(world* World, world_position* A, world_position* B) {
	world_difference Result = {};
	v2 dTileXY = {
		(real32)A->AbsTileX - (real32)B->AbsTileX,
		(real32)A->AbsTileY - (real32)B->AbsTileY
	};
	real32 dTileZ = (real32)A->AbsTileZ - (real32)B->AbsTileZ;
	Result.dXY = World->TileSideInMeters * dTileXY + (A->Offset_ - B->Offset_);
	Result.dZ = dTileZ * World->TileSideInMeters;
	return(Result);
}

inline world_position
CenteredTilePoint(uint32 AbsTileX, uint32 AbsTileY, uint32 AbsTileZ) {
	world_position Result = {};
	Result.AbsTileX = AbsTileX;
	Result.AbsTileY = AbsTileY;
	Result.AbsTileZ = AbsTileZ;
	return(Result);
}

void
InitializeWorld(world* World, real32 TileSideInMeter) {
	World->ChunkShift = 4;
	World->ChunkMask = (1 << World->ChunkShift) - 1;
	World->ChunkDim = (1 << World->ChunkShift);

	World->TileSideInMeters = TileSideInMeter;

	for (uint32 ChunkIndex = 0; ChunkIndex < ArrayCount(World->ChunkHash); ++ChunkIndex) {
		World->ChunkHash[ChunkIndex].ChunkX = TILE_CHUNK_UNINITIALIZED;
	}
}

inline bool32
AreInSameChunk(world_position a, world_position b) {
	bool32 Result = (a.ChunkX == b.ChunkX
		&& a.ChunkY == b.ChunkY && a.ChunkZ == b.ChunkZ);
	return false;
}

inline void
ChangeEntityLocation(memory_arena * Arena, world* World, uint32 LowEntityIndex,
	world_position* OldP, world_position* NewP) {
	if (OldP && AreInSameChunk(*OldP, *NewP)) {
		
	}
	else {
		if (OldP) {
			world_chunk* Chunk = GetWorldChunk(World, OldP->ChunkX, OldP->ChunkY, OldP->ChunkZ);
			Assert(Chunk);
			if (Chunk) {
				world_entity_block* FirstBlock = &Chunk->FirstBlock;
				for (world_entity_block* Block = FirstBlock Block; Block = Block->Next) {
					for (uint32 Index = 0; Index < Chunk->EntityCount; Index++) {
						if (Block->LowEntityIndex[Index] == LowEntityIndex) {
							if (FirstBlock == Block) {
								if (FirstBlock->EntityCount == 1) {
									if (FirstBlock->Next) {
										world_entity_block* NextBlock = FirstBlock->Next;
										FirstBlock = NextBlock;
										FreeBlock(NextBlock);
									}
								}
								else {
									// note --
									FirstBlock->LowEntityIndex[Index] = Block->LowEntityIndex[--Block->EntityCount];
								}
							}
							else {

							}
						}
					}
				}
			}

		}
		world_chunk* TargetChunk = GetWorldChunk(World, NewP->ChunkX, NewP->ChunkY, NewP->ChunkZ, Arena);
		world_entity_block* Block = &TargetChunk->FirstBlock;
		if (Block->EntityCount == ArrayCount(Block->LowEntityIndex)) {
			world_entity_block *OldBlock = PushStruct(Arena, world_entity_block);
			*OldBlock = *Block;
			Block->Next = OldBlock;
			Block->EntityCount = 0;
		}
		Assert(Block->EntityCount < ArrayCount(Block->LowEntityIndex));
		Block->LowEntityIndex[Block->EntityCount++] = LowEntityIndex;
	}


}