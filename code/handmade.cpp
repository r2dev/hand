#include "handmade.h"
#include "handmade_tile.cpp"
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
DrawBitmap(game_offscreen_buffer* Buffer, loaded_bitmap* Bitmap, real32 RealX, real32 RealY,
	int32 AlignX = 0, int32 AlignY = 0) {

	RealX -= AlignX;
	RealY -= AlignY;

	int32 MinX = RoundReal32ToInt32(RealX);
	int32 MinY = RoundReal32ToInt32(RealY);
	int32 MaxX = RoundReal32ToInt32(RealX + (real32)Bitmap->Width);
	int32 MaxY = RoundReal32ToInt32(RealY + (real32)Bitmap->Height);

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

		bit_scan_result RedShift = FindLeastSignificantSetBit(RedMask);
		bit_scan_result GreenShift = FindLeastSignificantSetBit(GreenMask);
		bit_scan_result BlueShift = FindLeastSignificantSetBit(BlueMask);
		bit_scan_result AlphaShift = FindLeastSignificantSetBit(AlphaMask);

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
				*SourceDest++ = ((C >> AlphaShift.Index) & 0xFF) << 24 |
					((C >> RedShift.Index) & 0xFF) << 16 |
					((C >> GreenShift.Index) & 0xFF) << 8 |
					((C >> BlueShift.Index) & 0xFF) << 0;
			}
		}
	}

	return(Result);

}

internal void
InitializateArena(memory_arena* Arena, memory_index Size, uint8* Storage) {
	Arena->Size = Size;
	Arena->Base = Storage;
	Arena->Used = 0;
}

inline entity*
GetEntity(game_state* GameState, uint32 Index) {
	entity* Entity = 0;
	if ((Index > 0) && (Index < ArrayCount(GameState->Entities))) {
		Entity = &GameState->Entities[Index];
	}
	return(Entity);
}

internal void
InitializePlayer(game_state * GameState, uint32 EntityIndex) {
	entity* Entity = GetEntity(GameState, EntityIndex);
	Entity->Exists = true;
	Entity->P.AbsTileX = 7;
	Entity->P.AbsTileY = 3;
	Entity->P.Offset_.X = 0;
	Entity->P.Offset_.Y = 0;
	Entity->Height = 1.4f;
	Entity->Width = 0.75f * Entity->Height;

	if (!GetEntity(GameState, GameState->CameraFollowingEntityIndex)) {
		GameState->CameraFollowingEntityIndex = EntityIndex;
	}
}


inline uint32
AddEntity(game_state* GameState) {
	uint32 EntityIndex = GameState->EntityCount++;
	Assert(GameState->EntityCount < ArrayCount(GameState->Entities));
	entity *Entity = &GameState->Entities[EntityIndex];
	*Entity = {};
	return(EntityIndex);
}

internal void 
TestWall(real32 WallX, real32 RelX, real32 RelY,
	real32 PlayerDeltaX, real32 PlayerDeltaY, real32* tMin, real32 MinY, real32 MaxY) {
	real32 tEpsilon = 0.0001f;
	if (PlayerDeltaX != 0.0f) {
		real32 tResult = (WallX - RelX) / PlayerDeltaX;
		real32 Y = RelY + tResult * PlayerDeltaY;
		if ((tResult >= 0.0f) && (*tMin > tResult)) {
			if ((Y >= MinY) && (Y <= MaxY)) {
				*tMin = Maximum(0.0f, tResult - tEpsilon);
			}
		}
	}
}


#if 1
internal void
MovePlayer(game_state *GameState, entity* Entity, real32 dt, v2 ddP) {
	
	tile_map *TileMap = GameState->World->TileMap;
	real32 ddpLength = LengthSq(ddP);
	if (ddpLength > 1.0f) {
		ddP *= (1.0f / SquareRoot(ddpLength));
	}
	real32 PlayerSpeed = 50.0f;
	ddP *= PlayerSpeed;

	ddP += -8.0f * Entity->dP;
	tile_map_position OldPlayerP = Entity->P;
	v2 PlayerDelta = 0.5f * Square(dt) * ddP +
		dt * Entity->dP;
	Entity->dP = dt * ddP + Entity->dP;
	tile_map_position NewPlayerP = Offset(TileMap, OldPlayerP, PlayerDelta);

#if 0
	tile_map_position PlayerLeft = NewPlayerP;
	PlayerLeft.Offset_.X -= 0.5f * Entity->Width;
	PlayerLeft = RecanonicalizePosition(TileMap, PlayerLeft);
	tile_map_position PlayerRight = NewPlayerP;
	PlayerRight.Offset_.X += 0.5f * Entity->Width;
	PlayerRight = RecanonicalizePosition(TileMap, PlayerRight);

	bool32 Collided = false;
	tile_map_position ColP = {};
	if (!IsTileMapPointEmpty(TileMap, NewPlayerP)) {
		ColP = NewPlayerP;
		Collided = true;
	}
	if (!IsTileMapPointEmpty(TileMap, PlayerLeft)) {
		ColP = PlayerLeft;
		Collided = true;
	}
	if (!IsTileMapPointEmpty(TileMap, PlayerRight)) {
		ColP = PlayerRight;
		Collided = true;
	}
	if (Collided) {
		v2 r = { 0, 0 };
		if (ColP.AbsTileX < Entity->P.AbsTileX) {
			r = v2{ 1, 0 };
		}
		if (ColP.AbsTileX > Entity->P.AbsTileX) {
			r = v2{ -1, 0 };
		}
		if (ColP.AbsTileY < Entity->P.AbsTileY) {
			r = v2{ 0, 1 };
		}
		if (ColP.AbsTileY > Entity->P.AbsTileY) {
			r = v2{ 0, -1 };
		}

		Entity->dP = Entity->dP - 1 * Inner(Entity->dP, r) * r;
	}
	else {
		Entity->P = NewPlayerP;
	}
#else

#if 0
	uint32 MinTileX = Minimum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX);
	uint32 MinTileY = Minimum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY);
	uint32 OnePastMaxTileY = Maximum(OldPlayerP.AbsTileX, NewPlayerP.AbsTileX) + 1;
	uint32 OnePastMaxTileX = Maximum(OldPlayerP.AbsTileY, NewPlayerP.AbsTileY) + 1;
#else
	uint32 StartTileX = OldPlayerP.AbsTileX;
	uint32 StartTileY = OldPlayerP.AbsTileY;
	uint32 EndTileX = NewPlayerP.AbsTileX;
	uint32 EndTileY = NewPlayerP.AbsTileY;

	int32 DeltaX = SignOf(EndTileX - StartTileX);
	int32 DeltaY = SignOf(EndTileY - StartTileY);
#endif

	uint32 AbsTileZ = Entity->P.AbsTileZ;
	real32 tMin = 1.0f;
	uint32 AbsTileY = StartTileY;
	for (;;) {
		uint32 AbsTileX = StartTileX;
		for (;;) {
			tile_map_position TestTileP = CenteredTilePoint(AbsTileX, AbsTileY, AbsTileZ);
			uint32 TileValue = GetTileValue(TileMap, TestTileP);
			if (!isTileValueEmpty(TileValue)) {
				v2 MinCorner = -0.5f * v2{ (real32)TileMap->TileSideInMeters, (real32)TileMap->TileSideInMeters };
				v2 MaxCorner = 0.5f * v2{ (real32)TileMap->TileSideInMeters, (real32)TileMap->TileSideInMeters };

				tile_map_difference RelOldPlayerP = Subtract(TileMap, &OldPlayerP, &TestTileP);
				v2 Rel = RelOldPlayerP.dXY;
				
				//tResult = (MinCorner.X - RelOldPlayerP.dXY.X) / PlayerDelta.X;
				TestWall(MinCorner.X, Rel.X, Rel.Y, PlayerDelta.X, 
					PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y);
				TestWall(MaxCorner.X, Rel.X, Rel.Y, PlayerDelta.X,
					PlayerDelta.Y, &tMin, MinCorner.Y, MaxCorner.Y);
				TestWall(MinCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
					PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X);
				TestWall(MaxCorner.Y, Rel.Y, Rel.X, PlayerDelta.Y,
					PlayerDelta.X, &tMin, MinCorner.X, MaxCorner.X);
			}
			if (AbsTileX == EndTileX) {
				break;
			}
			else {
				AbsTileX += DeltaX;
			}
		}
		if (AbsTileY == EndTileY) {
			break;
		}
		else {
			AbsTileY += DeltaY;
		}
	}
	Entity->P = Offset(TileMap, OldPlayerP, tMin * PlayerDelta);
#endif
	if (!AreOnSameTile(&OldPlayerP, &Entity->P)) {
		uint32 NewTileValue = GetTileValue(TileMap, Entity->P);
		if (NewTileValue == 3) {
			++Entity->P.AbsTileZ;
		}
		else if (NewTileValue == 4) {
			--Entity->P.AbsTileZ;
		}

	}

	if (AbsoluteValue(Entity->dP.X) > AbsoluteValue(Entity->dP.Y)) {
		if (Entity->dP.X > 0) {
			Entity->FacingDirection = 0;
		}
		else {
			Entity->FacingDirection = 2;
		}
	}
	else if (AbsoluteValue(Entity->dP.X) < AbsoluteValue(Entity->dP.Y)) {
		if (Entity->dP.Y > 0) {
			Entity->FacingDirection = 1;
		}
		else {
			Entity->FacingDirection = 3;
		}
	}
}
#endif


extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;

	

	if (!Memory->IsInitialized) {
		AddEntity(GameState);
		GameState->Background =
			DEBUGLoadBMP(Thread, Memory->DEBUGPlatformReadEntireFile, "test/test_background.bmp");

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

		InitializateArena(&GameState->WorldArena, Memory->PermanentStorageSize - sizeof(game_state),
			(uint8*)Memory->PermanentStorage + sizeof(game_state));
		GameState->World = PushSize(&GameState->WorldArena, world);
		world* World = GameState->World;
		World->TileMap = PushSize(&GameState->WorldArena, tile_map);
		tile_map* TileMap = World->TileMap;

		TileMap->ChunkShift = 4;
		TileMap->ChunkMask = (1 << TileMap->ChunkShift) - 1;
		TileMap->ChunkDim = (1 << TileMap->ChunkShift);

		TileMap->TileChunkCountX = 128;
		TileMap->TileChunkCountY = 128;
		TileMap->TileChunkCountZ = 2;

		TileMap->TileChunks = PushArray(&GameState->WorldArena,
			TileMap->TileChunkCountX * TileMap->TileChunkCountY * TileMap->TileChunkCountZ, tile_chunk);

		TileMap->TileSideInMeters = 1.4f;

		GameState->CameraP.AbsTileX = 17 / 2;
		GameState->CameraP.AbsTileY = 9 / 2;

		uint32 TilesPerWidth = 17;
		uint32 TilesPerHeight = 9;

		uint32 ScreenX = 0;
		uint32 ScreenY = 0;
		uint32 AbsTileZ = 0;

		bool32 DoorLeft = false;
		bool32 DoorRight = false;
		bool32 DoorTop = false;
		bool32 DoorBottom = false;
		bool32 DoorUp = false;
		bool32 DoorDown = false;

		for (uint32 ScreenIndex = 0; ScreenIndex < 100; ++ScreenIndex) {
			uint32 RandomChoice;
			if (DoorUp || DoorDown) {
				RandomChoice = rand() % 2;
			}
			else {
				RandomChoice = rand() % 3;
			}
			bool32 CreatedZDoor = false;
			if (RandomChoice == 2) {
				CreatedZDoor = true;
				if (AbsTileZ == 0) {
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
					SetTileValue(&GameState->WorldArena, World->TileMap, AbsTileX, AbsTileY, AbsTileZ, TileValue);
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
				if (AbsTileZ == 0) {
					AbsTileZ = 1;
				}
				else {
					AbsTileZ = 0;
				}
			}
			else if (RandomChoice == 1) {
				ScreenX += 1;
			}
			else {
				ScreenY += 1;
			}
		}
		Memory->IsInitialized = true;
	}

	world* World = GameState->World;
	tile_map* TileMap = World->TileMap;

	int32 TileSideInPixels = 60;
	real32 MetersToPixels = (real32)TileSideInPixels / (real32)TileMap->TileSideInMeters;
	real32 LowerLeftX = -(real32)(TileSideInPixels / 2);
	real32 LowerLeftY = (real32)Buffer->Height;

	

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);
		entity* ControllingEntity = GetEntity(GameState, GameState->PlayerIndexForController[ControllerIndex]);
		if (ControllingEntity) {
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
			MovePlayer(GameState, ControllingEntity, Input->dtForFrame, ddPlayer);
		}
		else {
			if (Controller->Start.EndedDown) {
				uint32 EntityIndex = AddEntity(GameState);
				InitializePlayer(GameState, EntityIndex);
				GameState->PlayerIndexForController[ControllerIndex] = EntityIndex;
			}
		}
		
	}
	entity* CameraFollowingEntity = GetEntity(GameState, GameState->CameraFollowingEntityIndex);
	if (CameraFollowingEntity) {
		GameState->CameraP.AbsTileZ = CameraFollowingEntity->P.AbsTileZ;

		tile_map_difference Diff = Subtract(TileMap, &CameraFollowingEntity->P, &GameState->CameraP);
		if (Diff.dXY.X > (9.0f * TileMap->TileSideInMeters)) {
			GameState->CameraP.AbsTileX += 17;
		}
		if (Diff.dXY.X < -(9.0f * TileMap->TileSideInMeters)) {
			GameState->CameraP.AbsTileX -= 17;
		}
		if (Diff.dXY.Y > (5.0f * TileMap->TileSideInMeters)) {
			GameState->CameraP.AbsTileY += 9;
		}
		if (Diff.dXY.Y < -(5.0f * TileMap->TileSideInMeters)) {
			GameState->CameraP.AbsTileY -= 9;
		}
	}
	DrawBitmap(Buffer, &GameState->Background, 0, 0);


	//DrawRectangle(Buffer, 0, 0, (real32)Buffer->Width, (real32)Buffer->Height, 0, 1, 1);

	real32 ScreenCenterX = 0.5f * (real32)Buffer->Width;
	real32 ScreenCenterY = 0.5f * (real32)Buffer->Height;

	for (int RelRow = -10; RelRow < 10; ++RelRow) {
		for (int RelColumn = -20; RelColumn < 20; ++RelColumn) {
			uint32 Column = GameState->CameraP.AbsTileX + RelColumn;
			uint32 Row = GameState->CameraP.AbsTileY + RelRow;
			uint32 TileID = GetTileValue(TileMap, Column, Row, GameState->CameraP.AbsTileZ);
			if (TileID > 1) {
				real32 Gray = 0.5f;
				if (TileID == 2) {
					Gray = 1.0f;
				}

				if (TileID > 2) {
					Gray = 0.25f;
				}

				if (Row == GameState->CameraP.AbsTileY && Column == GameState->CameraP.AbsTileX) {
					Gray = 0.0f;
				}

				v2 TileSide = { 0.5f * TileSideInPixels , 0.5f * TileSideInPixels };
				v2 Cen = {
				ScreenCenterX - MetersToPixels * GameState->CameraP.Offset_.X
					+ ((real32)RelColumn) * TileSideInPixels,
				ScreenCenterY + MetersToPixels * GameState->CameraP.Offset_.Y
					- ((real32)RelRow) * TileSideInPixels };

				v2 Min = Cen - TileSide;
				v2 Max = Cen + TileSide;
				DrawRectangle(Buffer, Min, Max, Gray, Gray, Gray);
			}
		}
	}

	entity* Entity = GameState->Entities;
	for (uint32 EntityIndex = 0; EntityIndex < GameState->EntityCount; ++EntityIndex, ++Entity) {
		
		if (Entity->Exists) {
			tile_map_difference Diff = Subtract(TileMap, &Entity->P, &GameState->CameraP);
			real32 PlayerR = 1.0f;
			real32 PlayerG = 1.0f;
			real32 PlayerB = 0;

			real32 PlayerGroundX = ScreenCenterX + MetersToPixels * Diff.dXY.X;
			real32 PlayerGroundY = ScreenCenterY - MetersToPixels * Diff.dXY.Y;
			v2 PlayerLeftTop = { PlayerGroundX - (MetersToPixels * Entity->Width* 0.5f),
				PlayerGroundY - MetersToPixels * (Entity->Height) };
			v2 PlayerWidthHeight = {
				Entity->Width, Entity->Height
			};

			DrawRectangle(Buffer, PlayerLeftTop, PlayerLeftTop + MetersToPixels * PlayerWidthHeight,
				PlayerR, PlayerG, PlayerB);

			hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[Entity->FacingDirection];
			DrawBitmap(Buffer, &HeroBitmap->Torso, PlayerGroundX, PlayerGroundY, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(Buffer, &HeroBitmap->Cape, PlayerGroundX, PlayerGroundY, HeroBitmap->AlignX, HeroBitmap->AlignY);
			DrawBitmap(Buffer, &HeroBitmap->Head, PlayerGroundX, PlayerGroundY, HeroBitmap->AlignX, HeroBitmap->AlignY);
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