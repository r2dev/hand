#pragma once
#pragma pack(push, 1)
#define RIFF_CODE(a, b, c, d) (((uint32)(a) << 0) | ((uint32)(b) << 8) | ((uint32)(c) << 16) | ((uint32)(d) << 24))

struct hha_header {
#define HHA_MAGIC_VALUE RIFF_CODE('h', 'h', 'a', 'f');
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
	u32 NextIDToPlay;
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