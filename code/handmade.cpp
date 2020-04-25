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
		*SampleOut++ = SampleValue;
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
DrawBitmap(game_offscreen_buffer* Buffer, loaded_bitmap* Bitmap, real32 RealX, real32 RealY,
	int32 AlignX = 0, int32 AlignY = 0, real32 CAlpha = 1.0f) {

	RealX -= AlignX;
	RealY -= AlignY;

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

inline high_entity*
MakeEntityHighFrequency(game_state* GameState, uint32 LowIndex) {
	high_entity* HighEntity = 0;
	low_entity* LowEntity = GameState->LowEntities + LowIndex;

	if (LowEntity->HighEntityIndex) {
		HighEntity = GameState->HighEntities_ + LowEntity->HighEntityIndex;
	}
	else {
		if (GameState->HighEntityCount < ArrayCount(GameState->HighEntities_)) {
			uint32 HighIndex = GameState->HighEntityCount++;
			HighEntity = GameState->HighEntities_ + HighIndex;

			world_difference Diff =
				Subtract(GameState->World, &LowEntity->P, &GameState->CameraP);
			HighEntity->P = Diff.dXY;
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
			MakeEntityLowFrequency(GameState, High->LowEntityIndex);
		}
	}
}

inline uint32
AddLowEntity(game_state* GameState, entity_type Type) {
	Assert(GameState->LowEntityCount < ArrayCount(GameState->LowEntities));
	uint32 EntityIndex = GameState->LowEntityCount++;

	GameState->LowEntities[EntityIndex] = {};
	GameState->LowEntities[EntityIndex].Type = Type;

	return(EntityIndex);
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
GetHighEntity(game_state* GameState, uint32 LowIndex) {
	entity Result = {};
	if ((LowIndex > 0) && (LowIndex < GameState->LowEntityCount)) {
		Result.LowIndex = LowIndex;
		Result.Low = GameState->LowEntities + LowIndex;
		Result.High = MakeEntityHighFrequency(GameState, LowIndex);
	}
	return(Result);
}



internal uint32
AddPlayer(game_state* GameState) {
	uint32 EntityIndex = AddLowEntity(GameState, EntityType_Hero);
	low_entity* Entity = GetLowEntity(GameState, EntityIndex);

	Entity->P = GameState->CameraP;
	Entity->P.Offset_.X = 0;
	Entity->P.Offset_.Y = 0;
	Entity->Height = 0.5f;
	Entity->Width = 1.0f;
	Entity->Collides = true;
	MakeEntityHighFrequency(GameState, EntityIndex);
	//ChangeEntityResidence(GameState, EntityIndex, EntityResidence_High);

	if (GameState->CameraFollowingEntityIndex == 0) {
		GameState->CameraFollowingEntityIndex = EntityIndex;
	}
	return(EntityIndex);
}

internal uint32
AddWall(game_state* GameState, int32 AbsTileX, int32 AbsTileY, int32 AbsTileZ) {
	uint32 EntityIndex = AddLowEntity(GameState, EntityType_Wall);
	low_entity* Entity = GetLowEntity(GameState, EntityIndex);
	Entity->P = ChunkPositionFromTilePosition(GameState->World, AbsTileX, AbsTileY, AbsTileZ);
	Entity->Height = GameState->World->TileSideInMeters;;
	Entity->Width = Entity->Height;
	Entity->Collides = true;
	return(EntityIndex);
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
MovePlayer(game_state* GameState, entity Entity, real32 dt, v2 ddP) {

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
	Entity.Low->P = MapIntoTileSpace(GameState->World, GameState->CameraP, Entity.High->P);
}

#endif

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
#if 0
	int32 MinTileX = NewCameraP.AbsTileX - TileSpanX / 2;
	int32 MaxTileX = NewCameraP.AbsTileX + TileSpanX / 2;
	int32 MinTileY = NewCameraP.AbsTileY - TileSpanY / 2;
	int32 MaxTileY = NewCameraP.AbsTileY + TileSpanY / 2;
	for (uint32 EntityIndex = 1;
		EntityIndex < GameState->LowEntityCount;
		++EntityIndex)
	{
		low_entity* Low = GameState->LowEntities + EntityIndex;
		if (Low->HighEntityIndex == 0)
		{
			if ((Low->P.AbsTileZ == NewCameraP.AbsTileZ) &&
				(Low->P.AbsTileX >= MinTileX) &&
				(Low->P.AbsTileX <= MaxTileX) &&
				(Low->P.AbsTileY >= MinTileY) &&
				(Low->P.AbsTileY <= MaxTileY))
			{
				MakeEntityHighFrequency(GameState, EntityIndex);
			}
		}
	}
#endif
}


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;



	if (!Memory->IsInitialized) {
		AddLowEntity(GameState, EntityType_Null);
		GameState->HighEntityCount = 1;
		GameState->Background =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");
		GameState->Shadow =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_shadow.bmp");
		hero_bitmaps* Bitmap = GameState->HeroBitmaps;
		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_right_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_back_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_left_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;
		++Bitmap;

		Bitmap->Head = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_head.bmp");
		Bitmap->Cape = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_cape.bmp");
		Bitmap->Torso = DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_hero_front_torso.bmp");
		Bitmap->AlignX = 72;
		Bitmap->AlignY = 182;

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

		for (uint32 ScreenIndex = 0; ScreenIndex < 2000; ++ScreenIndex) {
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
		NewCameraP = ChunkPositionFromTilePosition(GameState->World, ScreenBaseX * TilesPerWidth + 17 / 2,
			ScreenBaseY * TilesPerHeight + 9 / 2,
			ScreenBaseZ);
		SetCamera(GameState, NewCameraP);

		Memory->IsInitialized = true;
	}

	world* World = GameState->World;

	int32 TileSideInPixels = 60;
	real32 MetersToPixels = (real32)TileSideInPixels / (real32)World->TileSideInMeters;
	real32 LowerLeftX = -(real32)(TileSideInPixels / 2);
	real32 LowerLeftY = (real32)Buffer->Height;



	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		uint32 LowIndex = GameState->PlayerIndexForController[ControllerIndex];
		if (LowIndex == 0) {
			if (Controller->Start.EndedDown) {
				uint32 EntityIndex = AddPlayer(GameState);

				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
		else {
			entity ControllingEntity = GetHighEntity(GameState, LowIndex);
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
			MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddPlayer);
		}
	}

	entity CameraFollowingEntity = GetHighEntity(GameState, GameState->CameraFollowingEntityIndex);
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
	DrawBitmap(Buffer, &GameState->Background, 0, 0);


	//DrawRectangle(Buffer, 0, 0, (real32)Buffer->Width, (real32)Buffer->Height, 0, 1, 1);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

	for (uint32 HighEntityIndex = 1; HighEntityIndex < GameState->HighEntityCount; ++HighEntityIndex) {
		
			high_entity* HighEntity = GameState->HighEntities_ + HighEntityIndex;
			low_entity* LowEntity = GameState->LowEntities + HighEntity->LowEntityIndex;

			real32 dt = Input->dtForFrame;
			real32 ddZ = -9.8f;
			HighEntity->Z += (0.5f * ddZ * Square(dt) + HighEntity->dZ * dt);
			HighEntity->dZ = HighEntity->dZ + ddZ * dt;
			if (HighEntity->Z < 0) {
				HighEntity->Z = 0;
			}
			real32 CAlpha = 1.0f - 0.5f * HighEntity->Z;
			if (CAlpha < 0) {
				CAlpha = 0.0f;
			}

			//tile_map_difference Diff = Subtract(TileMap, &Entity->P, &GameState->CameraP);
			real32 PlayerR = 1.0f;
			real32 PlayerG = 1.0f;
			real32 PlayerB = 0;

			real32 PlayerGroundX = ScreenCenterX + MetersToPixels * HighEntity->P.X;
			real32 PlayerGroundY = ScreenCenterY - MetersToPixels * HighEntity->P.Y;

			real32 Z = -HighEntity->Z * MetersToPixels;

			v2 PlayerLeftTop = {
				PlayerGroundX - (MetersToPixels * LowEntity->Width * 0.5f),
				PlayerGroundY - MetersToPixels * LowEntity->Height * 0.5f
			};
			v2 PlayerWidthHeight = {
				LowEntity->Width, LowEntity->Height
			};
			if (LowEntity->Type == EntityType_Hero) {
				hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[HighEntity->FacingDirection];
				DrawBitmap(Buffer, &GameState->Shadow, PlayerGroundX, PlayerGroundY, HeroBitmap->AlignX, HeroBitmap->AlignY, CAlpha);
				DrawBitmap(Buffer, &HeroBitmap->Torso, PlayerGroundX, PlayerGroundY + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
				DrawBitmap(Buffer, &HeroBitmap->Cape, PlayerGroundX, PlayerGroundY + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
				DrawBitmap(Buffer, &HeroBitmap->Head, PlayerGroundX, PlayerGroundY + Z, HeroBitmap->AlignX, HeroBitmap->AlignY);
				
			}
			else {
				DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels * PlayerWidthHeight,
					PlayerR, PlayerG, PlayerB);
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