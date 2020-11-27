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
DEBUGLoadBMP(char* FileName, v2 AlignPercentage = v2{0.5f, 0.5f}) {
	loaded_bitmap Result = {};
	debug_read_file_result ReadResult = DEBUGPlatformReadEntireFile(FileName);
	if (ReadResult.ContentsSize != 0) {
		bitmap_header* Header = (bitmap_header*)ReadResult.Contents;
		uint32* Pixels = (uint32*)((uint8*)ReadResult.Contents + Header->BitmapOffset);
		Result.Height = Header->Height;
		Result.Width = Header->Width;
		Result.AlignPercentage = AlignPercentage;
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

struct load_bitmap_work {
	loaded_bitmap* Bitmap;
	bitmap_id ID;
	game_assets* Assets;
	task_with_memory* Task;
	asset_state FinalState;
};

PLATFORM_WORK_QUEUE_CALLBACK(DoLoadBitmapWork) {
	load_bitmap_work* Work = (load_bitmap_work*)Data;
	asset_bitmap_info* Info = Work->Assets->BitmapInfos + Work->ID.Value;
	*Work->Bitmap = DEBUGLoadBMP(Info->FileName, Info->AlignPercentage);
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
			
			Work->Bitmap = PushStruct(&Assets->Arena, loaded_bitmap);
			Work->FinalState = AssetState_loaded;

			PlatformAddEntry(Assets->TranState->LowPriorityQueue, DoLoadBitmapWork, Work);
		}
	}
}
internal bitmap_id
DEBUGAddBitmapInfo(game_assets* Assets, char* FileName, v2 AlignmentPercentage) {
	bitmap_id ID = { Assets->DEBUGUsedBitmapCount++ };
	asset_bitmap_info* Info = Assets->BitmapInfos + ID.Value;
	Info->AlignPercentage = AlignmentPercentage;
	Info->FileName = FileName;
	return(ID);
}

internal void
BeginAssetType(game_assets* Assets, asset_type_id ID) {
	Assert(Assets->DEBUGAssetType == 0);
	Assets->DEBUGAssetType = Assets->AssetTypes + ID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->DEBUGUsedAssetCount++;
	Assets->DEBUGAssetType->OnePassLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;

}

internal void
AddBitmapAsset(game_assets* Assets, char* FileName, v2 AlignmentPercentage = v2{0.5f, 0.5f}) {
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Assets->DEBUGAssetType->OnePassLastAssetIndex++;
	Asset->FirstTagIndex = 0;
	Asset->OnePassLastTagIndex = 0;
	Asset->SlotID = DEBUGAddBitmapInfo(Assets, FileName, AlignmentPercentage).Value;
}

internal void
EndAssetType(game_assets* Assets) {
	Assert(Assets->DEBUGAssetType);
	Assets->DEBUGUsedAssetCount = Assets->DEBUGAssetType->OnePassLastAssetIndex;
	Assets->DEBUGAssetType = 0;
}


internal game_assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TranState) {
	game_assets* Assets = PushStruct(Arena, game_assets);

	SubArena(&Assets->Arena, Arena, Size);
	Assets->TranState = TranState;

	Assets->BitmapsCount = 255 * Asset_Count;
	Assets->BitmapInfos = PushArray(Arena, Assets->BitmapsCount, asset_bitmap_info);
	Assets->Bitmaps = PushArray(Arena, Assets->BitmapsCount, asset_slot);

	Assets->TagCount = 0;
	Assets->Tags = 0;

	Assets->AssetCount = Assets->BitmapsCount;
	Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);

	Assets->DEBUGUsedBitmapCount = 1;
	Assets->DEBUGUsedAssetCount = 1;
	Assets->DEBUGAssetType = 0;

	BeginAssetType(Assets, Asset_Shadow);
	AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", V2(0.5f, 0.156682029f));
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Tree);
	AddBitmapAsset(Assets, "test2/tree00.bmp", V2(0.493827164f, 0.295652181f));
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Sword);
	AddBitmapAsset(Assets, "test2/rock02.bmp", V2(0.5f, 0.65625f));
	EndAssetType(Assets);


	BeginAssetType(Assets, Asset_Grass);
	AddBitmapAsset(Assets, "test2/grass00.bmp");
	AddBitmapAsset(Assets, "test2/grass01.bmp");
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Ground);
	AddBitmapAsset(Assets, "test2/ground00.bmp");
	AddBitmapAsset(Assets, "test2/ground01.bmp");
	AddBitmapAsset(Assets, "test2/ground02.bmp");
	AddBitmapAsset(Assets, "test2/ground03.bmp");
	EndAssetType(Assets);


	BeginAssetType(Assets, Asset_Tuft);
	AddBitmapAsset(Assets, "test2/tuft00.bmp");
	AddBitmapAsset(Assets, "test2/tuft01.bmp");
	EndAssetType(Assets);

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

bitmap_id RandomAssetFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {

	bitmap_id Result = {};
	asset_type* Type = Assets->AssetTypes + ID;
	if (Type->FirstAssetIndex != Type->OnePassLastAssetIndex) {
		uint32 Count = Type->OnePassLastAssetIndex - Type->FirstAssetIndex;
		asset* Asset = Assets->Assets + Type->FirstAssetIndex + RandomChoice(Series, Count);
		Result.Value = Asset->SlotID;
	}
	return(Result);
	



}