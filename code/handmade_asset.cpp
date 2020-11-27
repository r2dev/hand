#include "handmade_asset.h"
#include "handmade.h"
inline v2
GetTopDownAlign(loaded_bitmap* Bitmap, v2 Align) {
	Align.y = (real32)(Bitmap->Height) - 1.0f - Align.y;
	Align.x = SafeRatio0(Align.x, (real32)Bitmap->Width);
	Align.y = SafeRatio0(Align.y, (real32)Bitmap->Height);
	return Align;
}

internal void
SetTopDownAlign(hero_bitmaps* Bitmaps, v2 Align) {
	Align = GetTopDownAlign(&Bitmaps->Head, Align);
	Bitmaps->Head.AlignPercentage = Align;
	Bitmaps->Torso.AlignPercentage = Align;
	Bitmaps->Cape.AlignPercentage = Align;
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
DEBUGLoadBMP(char* FileName, int32 AlignX, int32 AlignY) {
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.AlignPercentage = GetTopDownAlign(&Result, V2i(AlignX, AlignY));
		Result.Memory = Pixels;
		Result.WidthOverHeight = SafeRatio0((real32)Result.Width, (real32)Result.Height);
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

		int32 RedShiftDown = (int32)RedScan.Index;
		int32 GreenShiftDown = (int32)GreenScan.Index;
		int32 BlueShiftDown = (int32)BlueScan.Index;
		int32 AlphaShiftDown = (int32)AlphaScan.Index;

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
				v4 Texel = {
					(real32)((C & RedMask) >> RedShiftDown),
					(real32)((C & GreenMask) >> GreenShiftDown),
					(real32)((C & BlueMask) >> BlueShiftDown),
					(real32)((C & AlphaMask) >> AlphaShiftDown)
				};
				Texel = SRGBToLinear1(Texel);
				Texel.rgb *= Texel.a;
				Texel = Linear1ToSRGB(Texel);

				*SourceDest++ = ((uint32)(Texel.a + 0.5f) << 24) |
					((uint32)(Texel.r + 0.5f) << 16) |
					((uint32)(Texel.g + 0.5f) << 8) |
					((uint32)(Texel.b + 0.5f) << 0);
			}
		}
	}
	Result.Pitch = Result.Width * BITMAP_BYTE_PER_PIXEL;
#if 0
	Result.Pitch = -Result.Width * BITMAP_BYTE_PER_PIXEL;
	Result.Memory = (uint8*)Result.Memory - Result.Pitch * (Result.Height - 1);
#endif

	return(Result);

}

internal loaded_bitmap
DEBUGLoadBMP(char* FileName) {
	loaded_bitmap Result = DEBUGLoadBMP(FileName, 0, 0);
	Result.AlignPercentage = v2{ 0.5f, 0.5f };
	return(Result);
}

struct load_bitmap_work {
	char* Filename;
	loaded_bitmap* Bitmap;
	bitmap_id ID;
	game_assets* Assets;
	task_with_memory* Task;

	bool32 HasAlignment;

	int32 AlignX;
	int32 AlignY;

	asset_state FinalState;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork) {
	load_bitmap_work* Work = (load_bitmap_work*)Data;

	if (Work->HasAlignment) {
		*Work->Bitmap = DEBUGLoadBMP(Work->Filename, Work->AlignX, Work->AlignY);
	}
	else {
		*Work->Bitmap = DEBUGLoadBMP(Work->Filename);
	}
	_WriteBarrier();
	Work->Assets->Bitmaps[Work->ID.Value].Bitmap = Work->Bitmap;
	Work->Assets->Bitmaps[Work->ID.Value].State = Work->FinalState;
	EndTaskWithMemory(Work->Task);
}


void LoadBitmap(game_assets* Assets, bitmap_id ID) {
	if (ID.Value &&
		_InterlockedCompareExchange((long volatile*)&Assets->Bitmaps[ID.Value].State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
		task_with_memory* Task = BeginTaskWithMemory(Assets->TranState);
		if (Task) {
			load_bitmap_work* Work = PushStruct(&Task->Arena, load_bitmap_work);

			Work->Assets = Assets;
			Work->ID = ID;
			Work->Task = Task;
			Work->Filename = "";
			Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
			Work->HasAlignment = false;
			Work->FinalState = AssetState_loaded;
			switch (ID.Value) {
			case Asset_Background: {
				Work->Filename = "test/test_background.bmp";
			} break;
			case Asset_Shadow: {
				Work->Filename = "test/test_hero_shadow.bmp";
				Work->HasAlignment = true;
				Work->AlignX = 72;
				Work->AlignY = 182;
			} break;
			case Asset_Stairwell: {
				Work->Filename = "test2/rock03.bmp";
				Work->HasAlignment = true;
				Work->AlignX = 29;
				Work->AlignY = 10;
			} break;

			case Asset_Sword: {
				Work->Filename = "test2/rock02.bmp";
			} break;
			case Asset_Tree: {
				Work->Filename = "test2/tree00.bmp";
				Work->HasAlignment = true;
				Work->AlignX = 40;
				Work->AlignY = 80;
			} break;
			}

			PlatformAddEntry(Assets->TranState->LowPriorityQueue, DoLoadBitmapWork, Work);
		}
	}
}

internal game_assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TranState) {
	game_assets* Assets = PushStruct(Arena, game_assets);

	SubArena(&Assets->Arena, Arena, Size);
	Assets->TranState = TranState;

	Assets->BitmapsCount = Asset_Count;
	Assets->Bitmaps = PushArray(Arena, Assets->BitmapsCount, asset_slot);

	Assets->TagCount = 0;
	Assets->Tags = 0;

	Assets->AssetCount = Assets->BitmapsCount;
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

	for (uint32 AssetID = 0; AssetID < Asset_Count; ++AssetID) {
		asset_type* Type = Assets->AssetTypes + AssetID;
		Type->FirstAssetIndex = AssetID;
		Type->OnePassLastAssetIndex = AssetID + 1;
		asset* Asset = Assets->Assets + Type->FirstAssetIndex;
		Asset->FirstTagIndex = 0;
		Asset->OnePassLastTagIndex = 0;
		Asset->SlotID = Type->FirstAssetIndex;
	}

	return(Assets);

}


bitmap_id GetFirstBitmapID(game_assets *Assets, asset_type_id ID) {

	bitmap_id Result = {};
	asset_type* Type = Assets->AssetTypes + ID;
	if (Type->FirstAssetIndex < Type->OnePassLastAssetIndex) {
		asset* Asset = Assets->Assets + Type->FirstAssetIndex;
		Result.Value = Asset->SlotID;
	}
	return(Result);
}

