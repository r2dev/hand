#pragma once

enum asset_type_id {
	Asset_none,
	Asset_Shadow,
	Asset_Tree,
	Asset_Sword,
    
	Asset_Grass,
	Asset_Ground,
	Asset_Tuft,
    
	Asset_Head,
	Asset_Cape,
	Asset_Torso,
    
    Asset_Font,
    
	Asset_Music,
    //Asset_Forget,
	Asset_Bloop,
    
	Asset_Count
};

enum asset_tag_id {
	Tag_Smoothness,
	Tag_Flagness,
	Tag_FaceDirection,
    
    Tag_UnicodeCodepoint,
    
    
	Tag_Count
};


#pragma pack(push, 1)

struct bitmap_id {
	uint32 Value;
};

struct sound_id {
	uint32 Value;
};

enum {
    HHASoundChain_none,
    HHASoundChain_loop,
    HHASoundChain_Advance
};
#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

struct hha_header {
#define HHA_MAGIC_VALUE RIFF_CODE('h', 'h', 'a', 'f')
	u32 MagicValue;
#define HHA_VERSION 0
	u32 Version;
	
	u32 AssetTypeCount;
	u32 AssetCount;
	u32 TagCount;
    
	u64 AssetTypes;
	u64 Assets;
	u64 Tags;
};

struct hha_bitmap {
	u32 Dim[2];
	r32 AlignPercentage[2];
};

struct hha_sound {
	u32 FirstSampleIndex;
    u32 ChannelCount;
	u32 SampleCount;
    u32 Chain;
};

struct hha_asset_type {
	u32 TypeID;
	u32 FirstAssetIndex;
	u32 OnePassLastAssetIndex;
};

struct hha_asset {
	u64 DataOffset;
	u32 FirstTagIndex;
	u32 OnePassLastTagIndex;
	union {
		hha_bitmap Bitmap;
		hha_sound Sound;
	};
};


struct hha_tag {
	u32 ID;
	r32 Value;
};

#pragma pack(pop)