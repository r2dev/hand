#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"

struct bitmap_id {
	uint32 Value;
};

struct sound_id {
	uint32 Value;
};

struct asset_bitmap_info {
	char* FileName;
	r32 AlignPercentageX;
	r32 AlignPercentageY;
};

struct asset_sound_info {
	char* FileName;
	u32 FirstSampleIndex;
	u32 SampleCount;
	sound_id NextIDToPlay;
};

struct asset {
	uint32 FirstTagIndex;
	uint32 OnePassLastTagIndex;
	union {
		asset_bitmap_info Bitmap;
		asset_sound_info Sound;
	};
};

#define VERY_LARGE_NUMBER 4096

struct game_assets {


	u32 TagCount;
	hha_tag Tags[VERY_LARGE_NUMBER];


	u32 AssetCount;
	asset Assets[VERY_LARGE_NUMBER];


	u32 AssetTypeCount;
	hha_asset_type AssetTypes[Asset_Count];
	
	hha_asset_type* DEBUGAssetType;
	asset* DEBUGAsset;

};