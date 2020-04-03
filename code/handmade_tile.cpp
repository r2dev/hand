#include "handmade_tile.h"

inline void
RecanonicalizeCoord(tile_map* TileMap, uint32* TileV, real32* TileRelV) {
	int32 Offset = RoundReal32ToInt32(*TileRelV / TileMap->TileSideInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * TileMap->TileSideInMeters;

	Assert(*TileRelV >= -0.5f * TileMap->TileSideInMeters);
	Assert(*TileRelV <= 0.5f * TileMap->TileSideInMeters)
}

inline tile_map_position
RecanonicalizePosition(tile_map* TileMap, tile_map_position Pos) {
	tile_map_position Result = Pos;
	RecanonicalizeCoord(TileMap, &Result.AbsTileX, &Result.TileRelX);
	RecanonicalizeCoord(TileMap, &Result.AbsTileY, &Result.TileRelY);
	return(Result);
}

inline tile_chunk*
GetTileChunk(tile_map* TileMap, uint32 TileChunkX, uint32 TileChunkY) {
	tile_chunk* TileChunk = 0;
	if (TileChunkX >= 0 && TileChunkX < TileMap->TileChunkCountX
		&& TileChunkY >= 0 && TileChunkY < TileMap->TileChunkCountY) {
		TileChunk = &TileMap->TileChunks[TileChunkY * TileMap->TileChunkCountX + TileChunkX];
	}
	return(TileChunk);
}

inline uint32
GetTileValueUnchecked(tile_map* TileMap, tile_chunk* TileChunk, uint32 TileX, uint32 TileY) {
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);

	uint32 TileChunkValue = TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX];
	return(TileChunkValue);
}

inline void
SetTileValueUnchecked(tile_map* TileMap, tile_chunk* TileChunk, uint32 TileX, uint32 TileY, uint32 TileValue) {
	Assert(TileChunk);
	Assert(TileX < TileMap->ChunkDim);
	Assert(TileY < TileMap->ChunkDim);
	TileChunk->Tiles[TileY * TileMap->ChunkDim + TileX] = TileValue;
}

internal uint32
GetTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY) {
	uint32 TileChunkValue = 0;
	if (TileChunk && TileChunk->Tiles) {
		TileChunkValue = GetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY);
	}
	return(TileChunkValue);
}

internal void
SetTileValue(tile_map* TileMap, tile_chunk* TileChunk, uint32 TestTileX, uint32 TestTileY,
	uint32 TileValue) {
	if (TileChunk && TileChunk->Tiles) {
		SetTileValueUnchecked(TileMap, TileChunk, TestTileX, TestTileY, TileValue);
	}
}



inline tile_chunk_position
GetChunkPositionFor(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY) {
	tile_chunk_position Result;
	Result.TileChunkX = AbsTileX >> TileMap->ChunkShift;
	Result.TileChunkY = AbsTileY >> TileMap->ChunkShift;
	Result.RelTileX = AbsTileX & TileMap->ChunkMask;
	Result.RelTileY = AbsTileY & TileMap->ChunkMask;
	return(Result);
}

internal uint32
GetTileValue(tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY) {
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	uint32 TileChunkValue = GetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY);
	return(TileChunkValue);
}


internal bool32
IsTileMapPointEmpty(tile_map* TileMap, tile_map_position CanPos) {
	uint32 TileValue = GetTileValue(TileMap, CanPos.AbsTileX, CanPos.AbsTileY);
	bool32 Empty = (TileValue == 1);
	return(Empty);
}

internal void
SetTileValue(memory_arena* Arena, tile_map* TileMap, uint32 AbsTileX, uint32 AbsTileY, uint32 TileValue) {
	tile_chunk_position ChunkPos = GetChunkPositionFor(TileMap, AbsTileX, AbsTileY);
	tile_chunk* TileChunk = GetTileChunk(TileMap, ChunkPos.TileChunkX, ChunkPos.TileChunkY);
	Assert(TileChunk);
	if (!TileChunk->Tiles) {
		uint32 TileCount = TileMap->ChunkDim * TileMap->ChunkDim;
		TileChunk->Tiles = PushArray(Arena, TileCount, uint32);
		for (uint32 TileIndex = 0; TileIndex < TileCount; ++TileIndex) {
			TileChunk->Tiles[TileIndex] = 1;
		}
	}

	SetTileValue(TileMap, TileChunk, ChunkPos.RelTileX, ChunkPos.RelTileY, TileValue);
}