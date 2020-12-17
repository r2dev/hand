#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "handmade_platform.h"
#include "handmade_file_formats.h"
#include "handmade_intrinsics.h"
#include "handmade_math.h"

enum asset_type {
    AssetType_Bitmap,
    AssetType_Sound,
    AssetType_Font,
    AssetType_FontGlyph,
};

struct asset_source_sound {
    char *FileName;
    u32 FirstSampleIndex;
};

struct loaded_font;
struct asset_source_font {
    loaded_font *Font;
};

struct asset_source_bitmap {
    char *FileName;
};

struct asset_source_font_glyph {
    loaded_font *Font;
    u32 Codepoint;
};

struct asset_source {
    asset_type Type;
    union {
        asset_source_sound Sound;
        asset_source_font_glyph Glyph;
        asset_source_font Font;
        asset_source_bitmap Bitmap;
    };
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