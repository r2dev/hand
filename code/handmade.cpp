#include "handmade.h"
#include "handmade_world.cpp"
#include <cstdlib>

internal void
GameOutputSound(game_state* GameState, game_sound_output_buffer* SoundBuffer, int ToneHz) {

	int16 ToneVolumn = 3000;
	int WavePeriod = SoundBuffer->SamplesPerSecond / ToneHz;

	int16* SampleOut = SoundBuffer->Samples;

	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
#if 0
		real32 SineValue = sinf(GameState->tSine);
		int16 SampleValue = (int16)(SineValue * ToneVolumn);
#else
		int16 SampleValue = 0;
#endif
		* SampleOut++ = SampleValue;
		*SampleOut++ = SampleValue;
#if 0
		GameState->tSine += 2.0f * Pi32 * 1.0f / (real32)WavePeriod;
#endif
	}
}

internal void
DrawRectangle(game_offscreen_buffer* Buffer,
	v2 vMin, v2 vMax,
	real32 R, real32 G, real32 B) {
	int32 MinX = RoundReal32ToInt32(vMin.X);
	int32 MinY = RoundReal32ToInt32(vMin.Y);
	int32 MaxX = RoundReal32ToInt32(vMax.X);
	int32 MaxY = RoundReal32ToInt32(vMax.Y);

	if (MinX < 0) {
		MinX = 0;
	}

	if (MinY < 0) {
		MinY = 0;
	}
	if (MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	if (MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}

	// BIT PATTERN AA RR GG BB
	uint32 Color = (uint32)(RoundReal32ToUInt32(R * 255) << 16 | RoundReal32ToUInt32(G * 255) << 8 |
		RoundReal32ToUInt32(B * 255) << 0);

	uint8* Row = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Pixel = (uint32*)Row;
		for (int X = MinX; X < MaxX; ++X) {
			*Pixel++ = Color;
		}
		Row += Buffer->Pitch;
	}
}

internal void
DrawBitmap(game_offscreen_buffer* Buffer, loaded_bitmap* Bitmap,
	real32 RealX, real32 RealY, real32 CAlpha = 1.0f) {

	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = MinX + Bitmap->Width;
	int32 MaxY = MinY + Bitmap->Height;

	int32 SourceOfferX = 0;
	if (MinX < 0) {
		SourceOfferX = -MinX;
		MinX = 0;
	}
	int32 SourceOfferY = 0;
	if (MinY < 0) {
		SourceOfferY = -MinY;
		MinY = 0;
	}
	if (MaxY > Buffer->Height) {
		MaxY = Buffer->Height;
	}
	if (MaxX > Buffer->Width) {
		MaxX = Buffer->Width;
	}

	uint32* SourceRow = Bitmap->Pixels + Bitmap->Width * (Bitmap->Height - 1);
	SourceRow += -SourceOfferY * Bitmap->Width + SourceOfferX;
	uint8* DestRow = ((uint8*)Buffer->Memory + MinX * Buffer->BytesPerPixel + MinY * Buffer->Pitch);

	for (int Y = MinY; Y < MaxY; ++Y) {
		uint32* Dest = (uint32*)DestRow;
		uint32* Source = SourceRow;
		for (int X = MinX; X < MaxX; ++X) {
			real32 A = ((real32)((*Source >> 24) & 0xFF)) / 255.0f;

			A *= CAlpha;
			real32 SR = (real32)((*Source >> 16) & 0xFF);
			real32 SG = (real32)((*Source >> 8) & 0xFF);
			real32 SB = (real32)((*Source >> 0) & 0xFF);

			real32 DR = (real32)((*Dest >> 16) & 0xFF);
			real32 DG = (real32)((*Dest >> 8) & 0xFF);
			real32 DB = (real32)((*Dest >> 0) & 0xFF);

			real32 R = (1.0f - A) * DR + A * SR;
			real32 G = (1.0f - A) * DG + A * SG;
			real32 B = (1.0f - A) * DB + A * SB;

			*Dest = (uint32)(R + 0.5f) << 16 | (uint32)(G + 0.5f) << 8 | (uint32)(B + 0.5f) << 0;
			++Dest;
			++Source;
		}
		DestRow += Buffer->Pitch;
		SourceRow -= Bitmap->Width;
	}
}

#pragma pack(push, 1)
struct bitmap_header {
	uint16 FileType;
	uint32 FileSize;
	uint16 Reserved1;
	uint16 Reserved2;
	uint32 BitmapOffset;
	uint32 Size;
	int32 Width;
	int32 Height;
	uint16 Planes;
	uint16 BitsPerPixel;
	uint32 Compression;
	uint32 SizeOfBitmap;
	int32 HorzResolution;
	int32 VertResolution;
	uint32 ColorsUsed;
	uint32 ColorsImportant;
	uint32 RedMask;
	uint32 GreenMask;
	uint32 BlueMask;
};
#pragma pack(pop)

internal loaded_bitmap
DEBUGLoadBMP(thread_context* Thread, debug_platform_read_entire_file* ReadEntireFile, char* FileName) {
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = ReadEntireFile(Thread, FileName);
	if (ReadResult.ContentsSize != 0) {
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.Pixels = Pixels;
		uint32 RedMask = Header->RedMask;
		uint32 GreenMask = Header->GreenMask;
		uint32 BlueMask = Header->BlueMask;
		uint32 AlphaMask = ~(RedMask | GreenMask | BlueMask);

		bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
		Assert(RedScan.Found);
		Assert(GreenScan.Found);
		Assert(BlueScan.Found);
		Assert(AlphaScan.Found);

		int32 RedShift = 16 - (int32)RedScan.Index;
		int32 GreenShift = 8 - (int32)GreenScan.Index;
		int32 BlueShift = 0 - (int32)BlueScan.Index;
		int32 AlphaShift = 24 - (int32)AlphaScan.Index;

		uint32* SourceDest = Pixels;
		for (int32 Y = 0;
			Y < Header->Height;
			++Y)
		{
			for (int32 X = 0;
				X < Header->Width;
				++X)
			{
				uint32 C = *SourceDest;
				*SourceDest++ = (RotateLeft(C & RedMask, RedShift) | RotateLeft(C & GreenMask, GreenShift)
					| RotateLeft(C & BlueMask, BlueShift) | RotateLeft(C & AlphaMask, AlphaShift));
#if 0
				((C >> AlphaShift.Index) & 0xFF) << 24 |
					((C >> RedShift.Index) & 0xFF) << 16 |
					((C >> GreenShift.Index) & 0xFF) << 8 |
					((C >> BlueShift.Index) & 0xFF) << 0;
#endif
			}
		}
	}

	return(Result);

}

internal void
InitializeArena(memory_arena* Arena, memory_index Size, uint8* Storage) {
	Arena->Size = Size;
	Arena->Base = Storage;
	Arena->Used = 0;
}

inline v2
GetCameraSpaceP(game_state* GameState, low_entity* LowEntity) {

	world_difference Diff =
		Subtract(GameState->World, &LowEntity->P, &GameState->CameraP);
	v2 Result = Diff.dXY;
	return(Result);
}

inline high_entity*
MakeEntityHighFrequency(game_state* GameState, low_entity* LowEntity, uint32 LowIndex, v2 CameraSpaceP) {
	high_entity* HighEntity = 0;
	Assert(LowEntity->HighEntityIndex == 0);
	if (LowEntity->HighEntityIndex == 0) {
		if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities_)) {
			uint32 HighIndex = GameState->HighEntityCount++;
			HighEntity = GameState->HighEntities_ + HighIndex;
			HighEntity->P = CameraSpaceP;
			HighEntity->dP = v2{ 0, 0 };
			HighEntity->ChunkZ = LowEntity->P.ChunkZ;
			HighEntity->FacingDirection = 0;
			HighEntity->LowEntityIndex = LowIndex;
			LowEntity->HighEntityIndex = HighIndex;
		}
		else {
			InvalidCodePath;
		}
	}
	return HighEntity;
}

inline high_entity*
MakeEntityHighFrequency(game_state* GameState, uint32 LowIndex) {
	high_entity* HighEntity = 0;
	low_entity* LowEntity = GameState->LowEntities + LowIndex;
	if (LowEntity->HighEntityIndex) {
		HighEntity = GameState->HighEntities_ + LowEntity->HighEntityIndex;
	}
	else {
		v2 CameraSpaceP = GetCameraSpaceP(GameState, LowEntity);
		HighEntity = MakeEntityHighFrequency(GameState, LowEntity, LowIndex, CameraSpaceP);
	}
	return(HighEntity);
}

inline void
MakeEntityLowFrequency(game_state* GameState, uint32 LowIndex) {
	low_entity* LowEntity = &GameState->LowEntities[LowIndex];
	uint32 HighIndex = LowEntity->HighEntityIndex;
	if (HighIndex) {
		uint32 LastHighIndex = GameState->HighEntityCount - 1;
		if (HighIndex != LastHighIndex) {
			high_entity* LastHighEntity = GameState->HighEntities_ + LastHighIndex;
			high_entity* DelEntity = GameState->HighEntities_ + HighIndex;
			*DelEntity = *LastHighEntity;
			low_entity* LastHighEntityLowEntity = GameState->LowEntities + LastHighEntity->LowEntityIndex;
			LastHighEntityLowEntity->HighEntityIndex = HighIndex;
		}
		--GameState->HighEntityCount;
		LowEntity->HighEntityIndex = 0;
	}
}

inline void
OffsetAndCheckFrequencyByArea(game_state* GameState, v2 Offset, rectangle2 CameraBounds) {
	for (uint32 EntityIndex = 1; EntityIndex < GameState->HighEntityCount;) {
		high_entity* High = GameState->HighEntities_ + EntityIndex;
		High->P += Offset;
		if (IsInRectangle(CameraBounds, High->P)) {
			++EntityIndex;
		}
		else {
			Assert(GameState->LowEntities[High->LowEntityIndex].HighEntityIndex == EntityIndex);
			MakeEntityLowFrequency(GameState, High->LowEntityIndex);
		}
	}
}
inline low_entity*
GetLowEntity(game_state* GameState, uint32 Index) {
	low_entity* Result = 0;

	if ((Index > 0) && (Index < GameState->LowEntityCount)) {
		Result = GameState->LowEntities + Index;
	}
	return(Result);
}

inline entity
EntityFromHighIndex(game_state* GameState, uint32 HighEntityIndex) {
	entity Result = {};
	if (HighEntityIndex) {
		Assert(HighEntityIndex < ArrayCount(GameState->HighEntities_));
		Assert(HighEntityIndex < GameState->HighEntityCount);
		Result.High = GameState->HighEntities_ + HighEntityIndex;
		Result.LowIndex = Result.High->LowEntityIndex;
		Result.Low = GameState->LowEntities + Result.LowIndex;
	}
	return(Result);
}

struct add_low_entity_result {
	uint32 LowIndex;
	low_entity* Low;
};

inline add_low_entity_result
AddLowEntity(game_state* GameState, entity_type Type, world_position* P) {
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	add_low_entity_result Result = {};
	uint32 EntityIndex = GameState->LowEntityCount++;
	low_entity* LowEntity = GameState->LowEntities + EntityIndex;
	*LowEntity = {};
	LowEntity->Type = Type;
	if (P) {
		LowEntity->P = *P;
		ChangeEntityLocation(&GameState->WorldArena, GameState->World, EntityIndex, 0, P);
	}
	Result.Low = LowEntity;
	Result.LowIndex = EntityIndex;
	return(Result);
}


inline entity
ForceEntityIntoHigh(game_state* GameState, uint32 LowIndex) {
	entity Result = {};
	if ((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
		Result.LowIndex = LowIndex;
		Result.Low = GameState->LowEntities + LowIndex;
		Result.High = MakeEntityHighFrequency(GameState, LowIndex);
	}
	return(Result);
}

inline void
PushPiece(entity_visible_piece_group* Group, loaded_bitmap* Bitmap,
	v2 Offset, real32 OffsetZ, v2 Align, v2 Dim, v4 Color, real32 EntityZC) {

	Assert(Group->PieceCount < ArrayCount(Group->Pieces));
	entity_visible_piece* Piece = Group->Pieces + Group->PieceCount++;
	Piece->Bitmap = Bitmap;
	Piece->Offset = Group->GameState->MetersToPixels * v2{ Offset.X, -Offset.Y } - Align;
	Piece->OffsetZ = Group->GameState->MetersToPixels * OffsetZ;

	Piece->Dim = Dim;

	Piece->R = Color.R;
	Piece->G = Color.G;
	Piece->B = Color.B;
	Piece->A = Color.A;

	Piece->EntityZC = EntityZC;
}

inline void
PushBitmap(entity_visible_piece_group* Group, loaded_bitmap* Bitmap,
	v2 Offset, real32 OffsetZ, v2 Align, real32 Alpha = 1.0f, real32 EntityZC = 1.0f) {
	PushPiece(Group, Bitmap, Offset, OffsetZ, Align,
		v2{ 0, 0 }, v4{ 1.0f, 1.0f, 1.0f, Alpha }, EntityZC);
}

inline void
PushRect(entity_visible_piece_group* Group, v2 Offset, real32 OffsetZ,
	v2 Dim, v4 Color, real32 EntityZC = 1.0f) {
	PushPiece(Group, 0, Offset, OffsetZ, v2{ 0, 0 }, Dim, Color, EntityZC);
}

internal add_low_entity_result
AddPlayer(game_state* GameState) {
	world_position P = GameState->CameraP;
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Hero, &P);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = true;
	Entity.Low->HitPointMax = 3;
	Entity.Low->HitPoint[2].FilledAmount = HIT_POINT_SUB_COUNT;
	Entity.Low->HitPoint[0] = Entity.Low->HitPoint[1] = Entity.Low->HitPoint[2];
	MakeEntityHighFrequency(GameState, Entity.LowIndex);
	//ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);

	if (GameState->CameraFollowingEntityIndex == 0) {
		GameState->CameraFollowingEntityIndex = Entity.LowIndex;
	}
	return(Entity);
}

internal add_low_entity_result
AddWall(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Wall, &P);
	Entity.Low->Height = GameState->World->TileSideInMeters;;
	Entity.Low->Width = Entity.Low->Height;
	Entity.Low->Collides = true;
	return(Entity);
}

internal add_low_entity_result
AddMonster(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Monster, &P);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = true;
	return(Entity);
}

internal add_low_entity_result
AddFamiliar(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	world_position P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	add_low_entity_result Entity = AddLowEntity(GameState, EntityType_Familiar, &P);
	Entity.Low->Height = 0.5f;
	Entity.Low->Width = 1.0f;
	Entity.Low->Collides = false;
	return(Entity);
}

internal bool32
TestWall(real32 WallX, real32 RelX, real32 RelY,
	real32 PlayerDeltaX, real32 PlayerDeltaY, real32* tMin, real32 MinY, real32 MaxY) {
	real32 tEpsilon = 0.001f;
	bool32 Hit = false;
	if (PlayerDeltaX != 0.0f) {
		real32 tResult = (WallX - RelX) / PlayerDeltaX;
		real32 Y = RelY + tResult * PlayerDeltaY;
		if ((tResult >= 0.0f) && (*tMin > tResult)) {
			if ((Y >= MinY) && (Y <= MaxY)) {
				Hit = true;
				*tMin = Maximum(0.0f, tResult - tEpsilon);
			}
		}
	}
	return(Hit);
}


#if 1
internal void
MoveEntity(game_state* GameState, entity Entity, real32 dt, v2 ddP) {

	world* World = GameState->World;
	real32 ddpLength = LengthSq(ddP);
	if (ddpLength > 1.0f) {
		ddP *= (1.0f / SquareRoot(ddpLength));
	}
	real32 PlayerSpeed = 50.0f;
	ddP *= PlayerSpeed;

	ddP += -8.0f * Entity.High->dP;
	v2 OldPlayerP = Entity.High->P;
	v2 PlayerDelta = 0.5f * Square(dt) * ddP +
		dt * Entity.High->dP;
	Entity.High->dP = dt * ddP + Entity.High->dP;

	//v2 NewPlayerP = Offset(TileMap, OldPlayerP, PlayerDelta);

	/*
	uint32 MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
	uint32 MaxTileX = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MaxTileY = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);

	uint32 EntityTileWidth = CeilReal32ToInt32(Entity.Dormant->Width / TileMap->TileSideInMeters);
	uint32 EntityTileHeight = CeilReal32ToInt32(Entity.Dormant->Height / TileMap->TileSideInMeters);

	MinTileX -= EntityTileWidth;
	MinTileY -= EntityTileHeight;
	MaxTileX += EntityTileWidth;
	MaxTileY += EntityTileHeight;

	Assert((MaxTileX - MinTileX) < 32);
	Assert((MaxTileY - MinTileY) < 32);

	uint32 AbsTileZ = Entity.Dormant->P.AbsTileZ;
	*/

	for (uint32 Iteration = 0; (Iteration < 4); ++Iteration) {
		real32 tMin = 1.0f;
		v2 WallNormal = {};
		uint32 HitHighEntityIndex = 0;

		v2 DesiredPosition = Entity.High->P + PlayerDelta;
		for (uint32 TestHighEntityIndex = 1;
			TestHighEntityIndex < GameState->HighEntityCount;
			++TestHighEntityIndex) {
			if (TestHighEntityIndex != Entity.Low->HighEntityIndex) {
				entity TestEntity;
				TestEntity.High = GameState->HighEntities_ + TestHighEntityIndex;
				TestEntity.LowIndex = TestEntity.High->LowEntityIndex;
				TestEntity.Low = GameState->LowEntities + TestEntity.LowIndex;

				if (TestEntity.Low->Collides) {
					real32 DiameterW = TestEntity.Low->Width + Entity.Low->Width;
					real32 DiameterH = TestEntity.Low->Height + Entity.Low->Height;

					v2 MinCorner = -0.5f * v2{ DiameterW, DiameterH };
					v2 MaxCorner = 0.5f * v2{ DiameterW, DiameterH };

					v2 Rel = Entity.High->P - TestEntity.High->P;

					//tResult = (MinCorner.X - RelOldPlayerP.dXY.X) / PlayerDelta.X;
					if (TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X,
						PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y)) {
						WallNormal = v2{ -1, 0 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X,
						PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y)) {
						WallNormal = v2{ 1, 0 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
						PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X)) {
						WallNormal = v2{ 0, 1 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
					if (TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
						PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X)) {
						WallNormal = v2{ 0, -1 };
						HitHighEntityIndex = TestHighEntityIndex;
					}
				}
			}
		}
		Entity.High->P += tMin * PlayerDelta;
		if (HitHighEntityIndex) {
			Entity.High->dP = Entity.High->dP - 1 * Inner(Entity.High->dP, WallNormal) * WallNormal;
			PlayerDelta = DesiredPosition - Entity.High->P;
			PlayerDelta = PlayerDelta - 1 * Inner(PlayerDelta, WallNormal) * WallNormal;

			high_entity* HitHighEntity = GameState->HighEntities_ + HitHighEntityIndex;
			low_entity* HitLowEntity = GameState->LowEntities + HitHighEntity->LowEntityIndex;
			// Entity.High->AbsTileZ += HitLowEntity->P.AbsTileZ;
		}
		else {
			break;
		}

	}
	if (Entity.High->dP.X == 0.0f && Entity.High->dP.Y == 0.0f) {

	}
	else if (AbsoluteValue(Entity.High->dP.X) > AbsoluteValue(Entity.High->dP.Y)) {
		if (Entity.High->dP.X > 0) {
			Entity.High->FacingDirection = 0;
		}
		else {
			Entity.High->FacingDirection = 2;
		}
	}
	else if (AbsoluteValue(Entity.High->dP.X) < AbsoluteValue(Entity.High->dP.Y)) {
		if (Entity.High->dP.Y > 0) {
			Entity.High->FacingDirection = 1;
		}
		else {
			Entity.High->FacingDirection = 3;
		}
	}
	world_position NewP = MapIntoChunkSpace(GameState->World, GameState->CameraP, Entity.High->P);
	ChangeEntityLocation(&GameState->WorldArena, GameState->World, Entity.LowIndex, &Entity.Low->P, &NewP);
	Entity.Low->P = NewP;
}

#endif

internal void
UpdateFamiliar(game_state* GameState, entity Entity, real32 dt) {
	entity ClosestHero = {};
	real32 ClosestHeroSq = Square(10.0f);
	for (uint32 HighIndex = 1; HighIndex < GameState->HighEntityCount; ++HighIndex) {
		entity TestEntity = EntityFromHighIndex(GameState, HighIndex);
		if (TestEntity.Low->Type == EntityType_Hero) {
			real32 RealDSq = LengthSq(TestEntity.High->P - Entity.High->P);
			if (ClosestHeroSq > RealDSq) {
				ClosestHero = TestEntity;
				ClosestHeroSq = RealDSq;
			}
		}
	}
	v2 ddp = {};
	if (ClosestHero.High && (ClosestHeroSq > Square(3.0f))) {
		real32 Acceleration = 0.5f;
		real32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
		ddp = OneOverLength * (ClosestHero.High->P - Entity.High->P);
		MoveEntity(GameState, Entity, dt, ddp);
	}
}

internal void
SetCamera(game_state* GameState, world_position NewCameraP)
{
	world* World = GameState->World;

	world_difference dCameraP = Subtract(World, &NewCameraP, &GameState->CameraP);
	GameState->CameraP = NewCameraP;

	// TODO(casey): I am totally picking these numbers randomly!
	uint32 TileSpanX = 17 * 3;
	uint32 TileSpanY = 9 * 3;
	rectangle2 CameraBounds = RectCenterDim(V2(0, 0),
		World->TileSideInMeters * V2((real32)TileSpanX,
			(real32)TileSpanY));
	v2 EntityOffsetForFrame = -dCameraP.dXY;
	OffsetAndCheckFrequencyByArea(GameState, EntityOffsetForFrame, CameraBounds);

	world_position MinChunkP = MapIntoChunkSpace(World, NewCameraP, GetMinCorner(CameraBounds));
	world_position MaxChunkP = MapIntoChunkSpace(World, NewCameraP, GetMaxCorner(CameraBounds));

	for (int32 ChunkY = MinChunkP.ChunkY; ChunkY <= MaxChunkP.ChunkY; ++ChunkY) {
		for (int32 ChunkX = MinChunkP.ChunkX; ChunkX <= MaxChunkP.ChunkX; ++ChunkX) {
			world_chunk* Chunk = GetWorldChunk(World, ChunkX, ChunkY, NewCameraP.ChunkZ);
			if (Chunk) {
				for (world_entity_block* Block = &Chunk->FirstBlock; Block; Block = Block->Next) {
					for (uint32 EntityIndex = 0; EntityIndex < Block->EntityCount; ++EntityIndex) {
						low_entity* Low = GameState->LowEntities + Block->LowEntityIndex[EntityIndex];
						if (Low->HighEntityIndex == 0) {
							v2 CameraSpaceP = GetCameraSpaceP(GameState, Low);
							if (IsInRectangle(CameraBounds, CameraSpaceP)) {
								MakeEntityHighFrequency(GameState, Low, Block->LowEntityIndex[EntityIndex], CameraSpaceP);
							}
						}
					}
				}

			}
		}
	}
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;



	if (!Memory->IsInitialized) {
		AddLowEntity(GameState, EntityType_Null, 0);
		GameState->HighEntityCount = 1;
		GameState->Background =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
		GameState->Shadow =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
		GameState->Tree =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test2/tree00.bmp");
		hero_bitmaps* Bitmap = GameState->HeroBitmaps;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->Align = v2{ 72, 182 };

		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->Align = v2{ 72, 182 };
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->Align = v2{ 72, 182 };
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->Align = v2{ 72, 182 };

		InitializeArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
			(uint8*)Memory->PermanentStorage + sizeof(game_state));
		GameState->World = PushStruct(&GameState->WorldArena, world);
		world* World = GameState->World;

		InitializeWorld(World, 1.4f);

		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;
		uint32 ScreenBaseX = 0;
		uint32 ScreenBaseY = 0;
		uint32 ScreenBaseZ = 0;
		uint32 ScreenX = ScreenBaseX;
		uint32 ScreenY = ScreenBaseY;
		uint32 AbsTileZ = ScreenBaseZ;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 30; ++ScreenIndex) {
			uint32 RandomChoice;
			//if (DoorUp || DoorDown) 
			{
				RandomChoice = rand() % 2;
			}
#if 0
	else {
		RandomChoice = rand() % 3;
	}
#endif
	bool32 CreatedZDoor = false;
	if (RandomChoice == 2) {
		CreatedZDoor = true;
		if (AbsTileZ == ScreenBaseZ) {
			DoorUp = true;
		}
		else {
			DoorDown = true;
		}
	}
	else if (RandomChoice == 1) {
		DoorRight = true;
	}
	else {
		DoorTop = true;
	}
	for (uint32 TileY = 0; TileY < TilesPerHeight; ++TileY) {
		for (uint32 TileX = 0; TileX < TilesPerWidth; ++TileX) {
			uint32 AbsTileX = ScreenX * TilesPerWidth + TileX;
			uint32 AbsTileY = ScreenY * TilesPerHeight + TileY;

			uint32 TileValue = 1;
			if (TileX == 0 && (!DoorLeft || TileY != (TilesPerHeight / 2))) {
				TileValue = 2;
			}

			if (TileX == TilesPerWidth - 1 && (!DoorRight || TileY != (TilesPerHeight / 2))) {
				TileValue = 2;
			}

			if (TileY == 0 && (!DoorBottom || TileX != (TilesPerWidth / 2))) {
				TileValue = 2;
			}

			if (TileY == TilesPerHeight - 1 && (!DoorTop || TileX != (TilesPerWidth / 2))) {
				TileValue = 2;
			}
			if (TileY == 6 && TileX == 10) {
				if (DoorUp) {
					TileValue = 3;
				}
				if (DoorDown) {
					TileValue = 4;
				}

			}

			if (TileValue == 2) {
				AddWall(GameState, AbsTileX, AbsTileY, AbsTileZ);
			}

		}
	}
	DoorLeft = DoorRight;
	DoorBottom = DoorTop;

	if (CreatedZDoor) {
		DoorDown = !DoorDown;
		DoorUp = !DoorUp;
	}

	else {
		DoorUp = false;
		DoorDown = false;
	}

	DoorRight = false;
	DoorTop = false;

	if (RandomChoice == 2) {
		if (AbsTileZ == ScreenBaseZ) {
			AbsTileZ = ScreenBaseZ + 1;
		}
		else {
			AbsTileZ = ScreenBaseZ;
		}
	}
	else if (RandomChoice == 1) {
		ScreenX += 1;
	}
	else {
		ScreenY += 1;
	}
		}

		world_position NewCameraP = {};
		uint32 CameraTileX = ScreenBaseX * TilesPerWidth + 17 / 2;
		uint32 CameraTileY = ScreenBaseY * TilesPerHeight + 9 / 2;
		uint32 CameraTileZ = ScreenBaseZ;
		NewCameraP =
			ChunkPositionFromTilePosition(GameState->World, CameraTileX, CameraTileY, CameraTileZ);
		AddMonster(GameState, CameraTileX + 2, CameraTileY + 2, CameraTileZ);
		AddFamiliar(GameState, CameraTileX - 2, CameraTileY - 2, CameraTileZ);
		SetCamera(GameState, NewCameraP);
		Memory->IsInitialized = true;
	}

	world* World = GameState->World;

	int32 TileSideInPixels = 60;
	GameState->MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;
	
	real32 MetersToPixels = GameState->MetersToPixels;
	

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
		if (LowIndex == 0) {
			if (Controller->Start.EndedDown) {
				uint32 EntityIndex = AddPlayer(GameState).LowIndex;

				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
		else {
			entity ControllingEntity = ForceEntityIntoHigh(GameState, LowIndex);
			v2 ddPlayer = {};
			if (Controller->IsAnalog) {
				ddPlayer = v2{ Controller->StickAverageX, Controller->StickAverageY };
			}
			else {

				if (Controller->MoveUp.EndedDown) {
					ddPlayer.Y = 1.0f;
				}
				if (Controller->MoveDown.EndedDown) {
					ddPlayer.Y = -1.0f;
				}
				if (Controller->MoveLeft.EndedDown) {
					ddPlayer.X = -1.0f;
				}
				if (Controller->MoveRight.EndedDown) {
					ddPlayer.X = 1.0f;
				}
			}
			if (Controller->ActionUp.EndedDown) {
				ControllingEntity.High->dZ = 3.0f;
			}
			MoveEntity(GameState, ControllingEntity, Input->dtForFrame, ddPlayer);
		}
	}

	entity CameraFollowingEntity = ForceEntityIntoHigh(GameState, GameState->CameraFollowingEntityIndex);
	if (CameraFollowingEntity.High) {
		world_position NewCameraP = GameState->CameraP;
		NewCameraP.ChunkZ = CameraFollowingEntity.Low->P.ChunkZ;
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
		NewCameraP = CameraFollowingEntity.Low->P;
#endif
		SetCamera(GameState, NewCameraP);
	}
	//

#if 1
	DrawRectangle(Buffer, v2{ 0, 0 },
		v2{ (real32)Buffer->Width ,  (real32)Buffer->Height }, 0, 1, 1);
#else
	DrawBitmap(Buffer, &GameState->Background, 0, 0);
#endif

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;
	entity_visible_piece_group PieceGroup;
	PieceGroup.GameState = GameState;
	for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex) {
		PieceGroup.PieceCount = 0;

		high_entity* HighEntity = GameState->HighEntities_ + HighEntityIndex;
		low_entity* LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

		entity Entity;
		Entity.High = HighEntity;
		Entity.Low = LowEntity;
		Entity.LowIndex = HighEntity->LowEntityIndex;

		real32 dt = Input->dtForFrame;

		real32 ShadowAlpha = 1.0f - 0.5f * HighEntity->Z;
		if (ShadowAlpha < 0) {
			ShadowAlpha = 0.0f;
		}


#if 0
		v2 PlayerLeftTop = {
			PlayerGroundX - (MetersToPixels * LowEntity->Width * 0.5f),
			PlayerGroundY - MetersToPixels * LowEntity->Height * 0.5f
		};
		v2 PlayerWidthHeight = {
			LowEntity->Width, LowEntity->Height
		};
		DrawRectangle(Buffer, PlayerLeftTop,
			PlayerLeftTop + MetersToPixels * PlayerWidthHeight, 1.0f, 1.0f, 0.0f);
#endif
		hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[HighEntity->FacingDirection];
		switch (LowEntity->Type) {
		case EntityType_Hero: {
			PushBitmap(&PieceGroup, &GameState->Shadow, v2{ 0, 0 }, 0, HeroBitmap->Align, ShadowAlpha, 0);
			PushBitmap(&PieceGroup, &HeroBitmap->Torso, v2{ 0, 0 }, 0, HeroBitmap->Align);
			PushBitmap(&PieceGroup, &HeroBitmap->Cape, v2{ 0, 0 }, 0, HeroBitmap->Align);
			PushBitmap(&PieceGroup, &HeroBitmap->Head, v2{ 0, 0 }, 0, HeroBitmap->Align);

			if (LowEntity->HitPointMax >= 1) {
				v2 HealthDim = { 0.2f, 0.2f };
				real32 SpacingX = 1.5f * HealthDim.X;
				v2 HitP = { -0.5f * (LowEntity->HitPointMax - 1) * SpacingX, -0.2f };
				v2 dHitP = { SpacingX, 0.0f };
				for (uint32 HealthIndex = 0; HealthIndex < LowEntity->HitPointMax; ++HealthIndex) {
					hit_point* HitPoint = LowEntity->HitPoint + HealthIndex;
					v4 Color = { 1.0f, 0.0f, 0.0f, 1.0f };
					if (HitPoint->FilledAmount == 0) {
						Color = { 0.2f, 0.2f, 0.2f, 1.0f };
					}
					PushRect(&PieceGroup, HitP, 0, HealthDim, Color, 0.0f);
					HitP += dHitP;
				}

			}
		} break;

		case EntityType_Wall: {
			PushBitmap(&PieceGroup, &GameState->Tree, v2{ 0, 0 }, 0, v2{ 40, 80 });
		} break;
		case EntityType_Familiar: {
			UpdateFamiliar(GameState, Entity, dt);
			PushBitmap(&PieceGroup, &GameState->Shadow, v2{ 0, 0 }, 0, HeroBitmap->Align, ShadowAlpha, 0);
			PushBitmap(&PieceGroup, &HeroBitmap->Head, v2{ 0, 0 }, 0, HeroBitmap->Align);
		}
		case EntityType_Monster: {

			PushBitmap(&PieceGroup, &GameState->Shadow, v2{ 0, 0 }, 0, HeroBitmap->Align, ShadowAlpha, 0);
			PushBitmap(&PieceGroup, &HeroBitmap->Torso, v2{ 0, 0 }, 0, HeroBitmap->Align);
		} break;
		default: {
			InvalidCodePath
		} break;
		}

		real32 ddZ = -9.8f;
		HighEntity->Z += (0.5f * ddZ * Square(dt) + HighEntity->dZ * dt);
		HighEntity->dZ = HighEntity->dZ + ddZ * dt;
		if (HighEntity->Z < 0) {
			HighEntity->Z = 0;
		}

		real32 EntityGroundX = ScreenCenterX + MetersToPixels * HighEntity->P.X;
		real32 EntityGroundY = ScreenCenterY - MetersToPixels * HighEntity->P.Y;

		real32 EntityZ = -HighEntity->Z * MetersToPixels;

		for (uint32 PieceIndex = 0; PieceIndex < PieceGroup.PieceCount; ++PieceIndex) {
			entity_visible_piece* Piece = PieceGroup.Pieces + PieceIndex;

			v2 Center = {
				EntityGroundX + Piece->Offset.X,
				EntityGroundY + Piece->Offset.Y + Piece->OffsetZ + EntityZ * Piece->EntityZC
			};

			if (Piece->Bitmap) {
				DrawBitmap(Buffer, Piece->Bitmap, Center.X, Center.Y, Piece->A);
			}
			else {
				v2 HalfDim = 0.5f * MetersToPixels * Piece->Dim;
				DrawRectangle(Buffer, Center - HalfDim, Center + HalfDim, Piece->R, Piece->G, Piece->B);
			}

		}

	}
}

extern "C" GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
	game_state* GameState = (game_state*)Memory->PermanentStorage;
	GameOutputSound(GameState, SoundBuffer, 400);
}

/*
internal void
RenderWeirdGradient(game_offscreen_buffer* Buffer, int XOffset, int YOffset) {
	uint8* Row = (uint8*)Buffer->Memory;
	for (int Y = 0; Y < Buffer->Height; Y++) {
		uint32* Pixel = (uint32*)Row;
		for (int X = 0; X < Buffer->Width; X++) {
			uint8 Blue = (uint8)(X + XOffset);
			uint8 Green = (uint8)(Y + YOffset);
			*Pixel++ = (Green << 16) | Blue;
		}
		Row += Buffer->Pitch;
	}
}


**/