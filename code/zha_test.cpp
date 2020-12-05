#include "zha_test.h"

FILE* Out = 0;

internal void
BeginAssetType(game_assets* Assets, asset_type_id ID) {
	Assert(Assets->DEBUGAssetType == 0);

	Assets->DEBUGAssetType = Assets->AssetTypes + ID;
	Assets->DEBUGAssetType->TypeID = ID;
	Assets->DEBUGAssetType->FirstAssetIndex = Assets->AssetCount++;
	Assets->DEBUGAssetType->OnePassLastAssetIndex = Assets->DEBUGAssetType->FirstAssetIndex;
}

internal bitmap_id
AddBitmapAsset(game_assets* Assets, char* FileName, r32 AlignmentPercentageX = 0.5f, r32 AlignmentPercentageY = 0.5f) {
	bitmap_id Result = { Assets->DEBUGAssetType->OnePassLastAssetIndex++ };
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount++;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	asset_bitmap_info* Info = &Assets->Assets[Result.Value].Bitmap;
	Info->AlignPercentageX = AlignmentPercentageX;
	Info->AlignPercentageY = AlignmentPercentageY;
	Info->FileName = FileName;
	Assets->DEBUGAsset = Asset;
	return(Result);
}

internal sound_id
AddSoundAsset(game_assets* Assets, char* FileName, u32 FirstSampleIndex = 0, u32 LoadSampleCount = 0) {
	sound_id Result = { Assets->DEBUGAssetType->OnePassLastAssetIndex++ };
	Assert(Assets->DEBUGAssetType);
	asset* Asset = Assets->Assets + Result.Value;
	Asset->FirstTagIndex = Assets->TagCount++;
	Asset->OnePassLastTagIndex = Asset->FirstTagIndex;
	asset_sound_info* Info = &Assets->Assets[Result.Value].Sound;
	Info->FileName = FileName;
	Info->FirstSampleIndex = FirstSampleIndex;
	Info->SampleCount = LoadSampleCount;
	Info->NextIDToPlay.Value = 0;
	Assets->DEBUGAsset = Asset;
	return(Result);
}


internal void
EndAssetType(game_assets* Assets) {
	Assert(Assets->DEBUGAssetType);
	Assets->DEBUGAssetType = 0;
}

internal void
AddTag(game_assets* Assets, asset_tag_id TagID, real32 Value) {
	Assert(Assets->DEBUGAsset);
	hha_tag* Tag = Assets->Tags + Assets->TagCount++;

	Tag->ID = TagID;
	Tag->Value = Value;
}


int main(int ArgCount, char **Args) {
	game_assets Assets_;
	game_assets* Assets = &Assets_;

	Assets->AssetCount = 1;
	Assets->TagCount = 1;
	
	Assets->DEBUGAssetType = 0;
	Assets->DEBUGAsset = 0;
	BeginAssetType(Assets, Asset_Shadow);
	AddBitmapAsset(Assets, "test/test_hero_shadow.bmp", 0.5f, 0.156682029f);
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Tree);
	AddBitmapAsset(Assets, "test2/tree00.bmp", 0.493827164f, 0.295652181f);
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Sword);
	AddBitmapAsset(Assets, "test2/rock02.bmp", 0.5f, 0.65625f);
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

	real32 AngleRight = 0.0f * Tau32;
	real32 AngleBack = 0.25f * Tau32;
	real32 AngleLeft = 0.5f * Tau32;
	real32 AngleFront = 0.75f * Tau32;



	BeginAssetType(Assets, Asset_Head);
	AddBitmapAsset(Assets, "test/test_hero_right_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_head.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Cape);
	AddBitmapAsset(Assets, "test/test_hero_right_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_cape.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Torso);
	AddBitmapAsset(Assets, "test/test_hero_right_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleRight);
	AddBitmapAsset(Assets, "test/test_hero_back_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleBack);
	AddBitmapAsset(Assets, "test/test_hero_left_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleLeft);
	AddBitmapAsset(Assets, "test/test_hero_front_torso.bmp", 0.5f, 0.156682029f);
	AddTag(Assets, Tag_FaceDirection, AngleFront);
	EndAssetType(Assets);


	BeginAssetType(Assets, Asset_Music);
	u32 TotalSampleCount = 7468095;
	u32 MusicChunkSize = 10 * 48000;
	sound_id LastMusic = {};

	for (u32 FirstSampleIndex = 0; FirstSampleIndex < TotalSampleCount; FirstSampleIndex += MusicChunkSize) {
		u32 SampleCount = TotalSampleCount - FirstSampleIndex;
		if (SampleCount > MusicChunkSize) {
			SampleCount = MusicChunkSize;
		}
		sound_id thisMusic = AddSoundAsset(Assets, "test3/music_test.wav", FirstSampleIndex, SampleCount);
		if (LastMusic.Value) {
			Assets->Assets[LastMusic.Value].Sound.NextIDToPlay = thisMusic;
		}
		LastMusic = thisMusic;
	}

	EndAssetType(Assets);

	BeginAssetType(Assets, Asset_Bloop);
	AddSoundAsset(Assets, "test3/bloop_00.wav");
	AddSoundAsset(Assets, "test3/bloop_01.wav");
	AddSoundAsset(Assets, "test3/bloop_02.wav");
	AddSoundAsset(Assets, "test3/bloop_03.wav");
	AddSoundAsset(Assets, "test3/bloop_04.wav");
	EndAssetType(Assets);



	Out = fopen("test.hha", "wb");
	if (Out) {

		hha_header Header = {};
		Header.MagicValue = HHA_MAGIC_VALUE;
		Header.Version = HHA_VERSION;

		Header.AssetTypeCount = Asset_Count;
		Header.AssetCount = 1;

		Header.TagCount = 0;

		u32 TagArraySize = Header.TagCount * sizeof(hha_tag);
		u32 AssetArraySize = Header.AssetCount * sizeof(hha_asset);
		u32 AssetTypeArraySize = Header.AssetTypeCount * sizeof(hha_asset_type);

		Header.Tags = sizeof(Header);
		Header.AssetTypes = Header.Tags + TagArraySize;
		Header.Assets = Header.AssetTypes + AssetTypeArraySize;

		
		fwrite(&Header, sizeof(Header), 1, Out);
		fwrite(Assets->Tags, TagArraySize, 1, Out);
		fwrite(Assets->AssetTypes, AssetTypeArraySize, 1, Out);
		//fwrite(Header.Assets, AssetArraySize, 1, Out);


		fclose(Out);
	}
	else {
		printf("Err: Could not open file\n");
	}
	printf("hello world");



	
	return(0);
} 