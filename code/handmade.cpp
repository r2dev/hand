#include "handmade.h"
#include "handmade_intrinsics.h"
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
	real32 RealMinX, real32 RealMinY, real32 RealMaxX, real32 RealMaxY,
	real32 R, real32 G, real32 B) {
	int32 MinX = RoundReal32ToInt32(RealMinX);
	int32 MinY = RoundReal32ToInt32(RealMinY);
	int32 MaxX = RoundReal32ToInt32(RealMaxX);
	int32 MaxY = RoundReal32ToInt32(RealMaxY);

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

inline uint32
GetTileValueUnchecked(world* World, tile_map* TileMap, int32 TileX, int32 TileY) {
	Assert(TileMap);
	Assert((TileX >= 0 && TileX < World->CountX&& TileY >= 0 && TileY < World->CountY))
		uint32 TileMapValue = TileMap->Tiles[TileY * World->CountX + TileX];
	return(TileMapValue);
}

inline tile_map*
GetTileMap(world* World, int32 TileMapX, int32 TileMapY) {
	tile_map* TileMap = 0;
	if (TileMapX >= 0 && TileMapX < World->TileMapCountX && TileMapY >= 0 && TileMapY < World->TileMapCountY) {
		TileMap = &World->TileMaps[TileMapY * World->TileMapCountX + TileMapX];
	}
	return(TileMap);
}

internal bool32
IsTileMapPointEmpty(world* World, tile_map* TileMap, int32 TestTileX, int32 TestTileY) {
	bool32 Empty = false;
	if (TileMap) {
		if ((TestTileX >= 0 && TestTileX < World->CountX
			&& TestTileY >= 0 && TestTileY < World->CountY)) {
			uint32 TileMapValue = GetTileValueUnchecked(World, TileMap, TestTileX, TestTileY);
			Empty = (TileMapValue == 0);
		}
	}
	return(Empty);
}

inline void
RecanonicalizeCoord(world* World, int32 TileCount, int32* TileMapV, int32* TileV, real32* TileRelV) {
	int32 Offset = FloorReal32ToInt32(*TileRelV / World->TileSideInMeters);
	*TileV += Offset;
	*TileRelV -= Offset * World->TileSideInMeters;

	Assert(*TileRelV >= 0);
	// Assert(*TileRelV < World->TileSideInMeters);

	if (*TileV < 0) {
		*TileV = TileCount + *TileV;
		--* TileMapV;
	}
	if (*TileV >= TileCount) {
		*TileV = *TileV - TileCount;
		++* TileMapV;
	}

}

inline canonical_position
RecanonicalizePosition(world* World, canonical_position Pos) {
	canonical_position Result = Pos;
	RecanonicalizeCoord(World, World->CountX, &Result.TileMapX, &Result.TileX, &Result.TileRelX);
	RecanonicalizeCoord(World, World->CountY, &Result.TileMapY, &Result.TileY, &Result.TileRelY);
	return(Result);
}


internal bool32
IsWorldPointEmpty(world* World, canonical_position TestPos) {
	bool32 Empty = false;
	canonical_position CanPos = RecanonicalizePosition(World, TestPos);
	tile_map* TileMap = GetTileMap(World, CanPos.TileMapX, CanPos.TileMapY);
	Empty = IsTileMapPointEmpty(World, TileMap, CanPos.TileX, CanPos.TileY);

	return(Empty);
}

extern "C" GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
	Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
	game_state* GameState = (game_state*)Memory->PermanentStorage;

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

	uint32 Tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,    1, 1, 1, 1, 1},
		{1, 1, 0, 0,   0, 0, 0, 0,   0, 0, 1, 1,    1, 0, 0, 0, 1},
		{1, 0, 1, 0,   0, 1, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 1, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 1,   0, 1, 0, 0,   0, 0, 0, 0,    0, 0, 1, 0, 0},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 1, 0, 0,    1, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 1, 0, 0,    0, 1, 0, 1, 1},
		{1, 1, 1, 1,   1, 1, 1, 1,   0, 1, 1, 1,    1, 1, 1, 1, 1}
	};

	uint32 Tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,   1, 1, 1, 1,   0, 1, 1, 1,    1, 1, 1, 1, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 0},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,    1, 1, 1, 1, 1}
	};
	uint32 Tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,    1, 1, 1, 1, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 1, 1, 1,   1, 1, 1, 1,   0, 1, 1, 1,    1, 1, 1, 1, 1}
	};
	uint32 Tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] = {
		{1, 1, 1, 1,   1, 1, 1, 1,   0, 1, 1, 1,    1, 1, 1, 1, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{0, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 0, 0, 0,   0, 0, 0, 0,   0, 0, 0, 0,    0, 0, 0, 0, 1},
		{1, 1, 1, 1,   1, 1, 1, 1,   1, 1, 1, 1,    1, 1, 1, 1, 1}
	};

	tile_map TileMaps[2][2];
	TileMaps[0][0].Tiles = (uint32*)Tiles00;
	TileMaps[0][1].Tiles = (uint32*)Tiles10;
	TileMaps[1][0].Tiles = (uint32*)Tiles01;
	TileMaps[1][1].Tiles = (uint32*)Tiles11;


	world World = {};
	World.TileMaps = (tile_map*)TileMaps;
	World.TileMapCountX = 2;
	World.TileMapCountY = 2;

	World.CountX = TILE_MAP_COUNT_X;
	World.CountY = TILE_MAP_COUNT_Y;

	World.TileSideInMeters = 1.4f;
	World.TileSideInPixels = 60;
	World.MetersToPixels = (real32)World.TileSideInPixels / (real32)World.TileSideInMeters;

	World.UpperLeftX = -(real32)(World.TileSideInPixels / 2);
	World.UpperLeftY = 0;

	real32 PlayerHeight = 1.4f;
	real32 PlayerWidth = 0.75f * PlayerHeight;

	if (!Memory->IsInitialized) {
		GameState->PlayerP.TileMapX = 0;
		GameState->PlayerP.TileMapY = 0;
		GameState->PlayerP.TileX = 7;
		GameState->PlayerP.TileY = 3;
		GameState->PlayerP.TileRelX = 5.0f;
		GameState->PlayerP.TileRelY = 5.0f;
		//GameState->PlayerX = 200;
		//GameState->PlayerY = 150;
		Memory->IsInitialized = true;
	}

	tile_map* TileMap = GetTileMap(&World, GameState->PlayerP.TileMapX, GameState->PlayerP.TileMapY);
	Assert(TileMap);

	for (int ControllerIndex = 0; ControllerIndex < ArrayCount(Input->Controllers); ControllerIndex++) {
		game_controller_input* Controller = GetController(Input, ControllerIndex);

		if (Controller->IsAnalog) {

		}
		else {
			real32 dPlayerX = 0.0f;
			real32 dPlayerY = 0.0f;
			if (Controller->MoveUp.EndedDown)
			{
				dPlayerY = -1.0f;
			}
			if (Controller->MoveDown.EndedDown)
			{
				dPlayerY = 1.0f;
			}
			if (Controller->MoveLeft.EndedDown)
			{
				dPlayerX = -1.0f;
			}
			if (Controller->MoveRight.EndedDown)
			{
				dPlayerX = 1.0f;
			}
			dPlayerX *= 4.0f;
			dPlayerY *= 4.0f;

			canonical_position NewPlayerP = GameState->PlayerP;
			NewPlayerP.TileRelX += dPlayerX * Input->dtForFrame;
			NewPlayerP.TileRelY += dPlayerY * Input->dtForFrame;
			NewPlayerP = RecanonicalizePosition(&World, NewPlayerP);

			canonical_position PlayerLeft = NewPlayerP;
			PlayerLeft.TileRelX -= 0.5f * PlayerWidth;
			PlayerLeft = RecanonicalizePosition(&World, PlayerLeft);
			canonical_position PlayerRight = NewPlayerP;
			PlayerRight.TileRelX += 0.5f * PlayerWidth;
			PlayerRight = RecanonicalizePosition(&World, PlayerRight);

			if (IsWorldPointEmpty(&World, NewPlayerP) &&
				IsWorldPointEmpty(&World, PlayerLeft) &&
				IsWorldPointEmpty(&World, PlayerRight)) {
				GameState->PlayerP = NewPlayerP;
			}
		}
	}

	DrawRectangle(Buffer, 0, 0, (real32)Buffer->Width, (real32)Buffer->Height, 0, 1, 1);


	for (int Row = 0; Row < 9; ++Row) {
		for (int Column = 0; Column < 17; ++Column) {
			uint32 TileID = GetTileValueUnchecked(&World, TileMap, Column, Row);
			real32 Gray = 0.5f;
			if (TileID == 1) {
				Gray = 1.0f;
			}
			if (Row == GameState->PlayerP.TileY && Column == GameState->PlayerP.TileX) {
				Gray = 0;
			}
			real32 MinX = World.UpperLeftX + ((real32)Column) * World.TileSideInPixels;
			real32 MinY = World.UpperLeftY + ((real32)Row) * World.TileSideInPixels;
			real32 MaxX = MinX + World.TileSideInPixels;
			real32 MaxY = MinY + World.TileSideInPixels;
			DrawRectangle(Buffer, MinX, MinY, MaxX, MaxY, Gray, Gray, Gray);
		}
	}
	real32 PlayerR = 1.0f;
	real32 PlayerG = 1.0f;
	real32 PlayerB = 0;

	real32 PlayerLeft = World.UpperLeftX + 
		GameState->PlayerP.TileX * World.TileSideInPixels +
		World.MetersToPixels * GameState->PlayerP.TileRelX - (World.MetersToPixels * PlayerWidth * 0.5f);
	real32 PlayerTop = World.UpperLeftY +
		GameState->PlayerP.TileY * World.TileSideInPixels +
		World.MetersToPixels * GameState->PlayerP.TileRelY - World.MetersToPixels * (PlayerHeight);

	DrawRectangle(Buffer, PlayerLeft, PlayerTop, 
		PlayerLeft + World.MetersToPixels * PlayerWidth, PlayerTop + World.MetersToPixels * PlayerHeight, 
		PlayerR, PlayerG, PlayerB);
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