
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

inline b32
IsValid(world_position P) {
	b32 Result = (P.ChunkX != TILE_CHUNK_UNINITIALIZED);
	return(Result);
}

inline b32
IsCanonical(r32 ChunkDim, r32 TileRel) {
	r32 Epsilon = 0.0001f;
	b32 Result = ((TileRel >= -(0.5f * ChunkDim + Epsilon))
                  && (TileRel <= (0.5f * ChunkDim + Epsilon)));
	return(Result);
}

inline b32
IsCanonical(world* World, v3 Offset) {
	b32 Result = ((IsCanonical(World->ChunkDimInMeters.x, Offset.x)) 
                  && (IsCanonical(World->ChunkDimInMeters.y, Offset.y))
                  && (IsCanonical(World->ChunkDimInMeters.z, Offset.z))
                  );
	return(Result);
}

inline void
RecanonicalizeCoord(r32 ChunkDimInMeters, s32* TileV, r32* TileRelV) {
	s32 Offset = RoundReal32ToInt32(*TileRelV / ChunkDimInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * ChunkDimInMeters;
	Assert(IsCanonical(ChunkDimInMeters, *TileRelV));
    
}

inline world_position
MapIntoChunkSpace(world* World, world_position BasePos, v3 Offset) {
	world_position Result = BasePos;
	Result.Offset_ += Offset;
	RecanonicalizeCoord(World->ChunkDimInMeters.x, &Result.ChunkX, &Result.Offset_.x);
	RecanonicalizeCoord(World->ChunkDimInMeters.y, &Result.ChunkY, &Result.Offset_.y);
	RecanonicalizeCoord(World->ChunkDimInMeters.z, &Result.ChunkZ, &Result.Offset_.z);
	return(Result);
}

inline world_position 
CenteredChunkPoint(u32 ChunkX, u32 ChunkY, u32 ChunkZ) {
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

inline void
ClearWorldEntityBlock(world_entity_block *Block) {
    Block->EntityCount = 0;
    Block->Next = 0;
    Block->EntityDataSize = 0;
}

inline world_chunk **
GetWorldChunkInternal(world* World, s32 ChunkX, s32 ChunkY, s32 ChunkZ) {
    TIMED_FUNCTION();
	Assert(ChunkX > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY > -TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ > -TILE_CHUNK_SAFE_MARGIN);
    
	Assert(ChunkX < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkY < TILE_CHUNK_SAFE_MARGIN);
	Assert(ChunkZ < TILE_CHUNK_SAFE_MARGIN);
    
	u32 HashValue = 18 * ChunkX + 13 * ChunkY + 3 * ChunkZ;
	u32 HashSlot = HashValue & (ArrayCount(World->ChunkHash) - 1);
	Assert(HashSlot < ArrayCount(World->ChunkHash));
	world_chunk **Chunk = &World->ChunkHash[HashSlot];
    for (; *Chunk; Chunk = &((*Chunk)->NextInHash)) {
        if ((*Chunk)->ChunkX == ChunkX && (*Chunk)->ChunkY == ChunkY && (*Chunk)->ChunkZ == ChunkZ) {
			break;
		}
    }
	return(Chunk);
}

inline world_chunk*
GetWorldChunk(world* World, s32 ChunkX, s32 ChunkY, s32 ChunkZ, memory_arena* Arena = 0) {
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    
    if (!Result && Arena) {
        
        Result = PushStruct(Arena, world_chunk, NoClear());
        
        Result->FirstBlock = 0;
        Result->ChunkX = ChunkX;
        Result->ChunkY = ChunkY;
        Result->ChunkZ = ChunkZ;
        
        Result->NextInHash = *ChunkPtr;
        *ChunkPtr = Result;
    }
    return(Result);
}

internal world_chunk *
RemoveWorldChunk(world* World, s32 ChunkX, s32 ChunkY, s32 ChunkZ) {
    world_chunk **ChunkPtr = GetWorldChunkInternal(World, ChunkX, ChunkY, ChunkZ);
    world_chunk *Result = *ChunkPtr;
    if (Result) {
        *ChunkPtr = Result->NextInHash;
    }
    return(Result);
}

/*inline b32
AreOnSameTile(world_position* A, world_position* B) {
	b32 Result = (A->AbsTileX == B->AbsTileX) && (A->AbsTileY == B->AbsTileY) && (A->AbsTileZ == B->AbsTileZ);
	return Result;
}*/

inline b32
AreInSameChunk(world* World, world_position* A, world_position* B) {
	Assert(IsCanonical(World, A->Offset_));
	Assert(IsCanonical(World, B->Offset_));
	b32 Result = (A->ChunkX == B->ChunkX
                  && A->ChunkY == B->ChunkY && A->ChunkZ == B->ChunkZ);
	return(Result);
}

internal v3
Subtract(world* World, world_position* A, world_position* B) {
	v3 dTile = {
		(r32)A->ChunkX - (r32)B->ChunkX,
		(r32)A->ChunkY - (r32)B->ChunkY,
		(r32)A->ChunkZ - (r32)B->ChunkZ
	};
	
	v3 Result = Hadamard(World->ChunkDimInMeters, dTile) + (A->Offset_ - B->Offset_);
	return(Result);
}

internal world *
CreateWorld(v3 ChunkDimInMeter, memory_arena* ParentArena) {
    world *World = PushStruct(ParentArena, world);
    World->ChunkDimInMeters = ChunkDimInMeter;
	World->FirstFree = 0;
    SubArena(&World->Arena, ParentArena, GetArenaSizeRemaining(ParentArena), NoClear());
    return(World);
}

inline b32
HasRoomFor(world_entity_block *Block, u32 Size) {
    b32 Result = Block->EntityDataSize + Size <= sizeof(Block->EntityData);
    return(Result);
}

inline void
PackEntityReference(entity_reference* Ref) {
	if (Ref->Ptr != 0) {
        Ref->ID = Ref->Ptr->ID;
	}
}

inline void
PackTraverableReference(traversable_reference* Ref) {
    PackEntityReference(&Ref->Entity);
}

internal void
PackEntityIntoChunk(world *World, entity *Source, world_chunk* Chunk) {
    u32 PackSize = sizeof(*Source);
    if (!Chunk->FirstBlock || !HasRoomFor(Chunk->FirstBlock, PackSize)) {
        if (!World->FirstFreeBlock) {
            World->FirstFreeBlock = PushStruct(&World->Arena, world_entity_block);
            World->FirstFreeBlock->Next = 0;
        }
        Chunk->FirstBlock = World->FirstFreeBlock;
        World->FirstFreeBlock = Chunk->FirstBlock->Next;
        ClearWorldEntityBlock(Chunk->FirstBlock);
    }
    world_entity_block *Block = Chunk->FirstBlock;
    Assert(HasRoomFor(Block, PackSize));
    void *Dest = Block->EntityData + Block->EntityDataSize;
    ++Block->EntityCount;
    Block->EntityDataSize += PackSize;
    entity *DestE = (entity *)Dest;
    *DestE = *Source;
    
    PackTraverableReference(&DestE->Occupying);
    PackTraverableReference(&DestE->CameFrom);
    PackEntityReference(&DestE->Head);
}

internal void
PackEntityIntoWorld(world *World, entity *Source, world_position ChunkP) {
    world_chunk* Chunk = GetWorldChunk(World, ChunkP.ChunkX, ChunkP.ChunkY, ChunkP.ChunkZ, &World->Arena);
    Assert(Chunk);
    PackEntityIntoChunk(World, Source, Chunk);
}

inline void
AddBlockToFreeList(world *World, world_entity_block *Block) {
    Block->Next = World->FirstFreeBlock;
    World->FirstFreeBlock = Block;
}

inline void
AddChunkToFreeList(world *World, world_chunk *Chunk) {
    Chunk->NextInHash = World->FirstFreeChunk;
    World->FirstFreeChunk = Chunk;
}

