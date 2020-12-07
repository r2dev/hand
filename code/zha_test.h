#pragma once
#include <stdio.h>
#include <stdlib.h>
#include "handmade_platform.h"
#include "handmade_asset_type_id.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

enum asset_type {
    AssetType_Bitmap,
    AssetType_Sound,
};

struct asset_source {
    asset_type Type;
    char *FileName;
    u32 FirstSampleIndex;
};

#define VERY_LARGE_NUMBER 4096

struct game_assets {
    
    
	u32 TagCount;
	hha_tag Tags[VERY_LARGE_NUMBER];
    
    
	u32 AssetCount;
	asset_source AssetSources[VERY_LARGE_NUMBER];
    hha_asset Assets[VERY_LARGE_NUMBER];
    
    
	u32 AssetTypeCount;
	hha_asset_type AssetTypes[Asset_Count];
	
	hha_asset_type* DEBUGAssetType;
    u32 AssetIndex;
    
};