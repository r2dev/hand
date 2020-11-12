#pragma once

struct loaded_bitmap {
	v2 Align;
	int32 Width;
	int32 Height;
	int32 Pitch;
	void* Memory;
};

struct environment_map {
	loaded_bitmap LOD[4];
	real32 Pz;
};

enum render_group_entry_type {
	RenderGroupEntryType_render_entry_clear,
	RenderGroupEntryType_render_entry_rectangle,
	RenderGroupEntryType_render_entry_bitmap,
	RenderGroupEntryType_render_entry_coordinate_system
};

struct render_group_entry_header {
	render_group_entry_type Type;
};

struct render_basis {
	v3 P;
};

struct render_entity_basis {
	render_basis* Basis;
	v3 Offset;
};

struct render_group {
	real32 MetersToPixels;
	render_basis* DefaultBasis;

	uint32 MaxPushBufferSize;
	uint32 PushBufferSize;
	uint8* PushBufferBase;
};

struct render_entry_clear {
	v4 Color;
};

struct render_entry_rectangle {
	render_entity_basis EntityBasis;
	v4 Color;
	v2 Dim;

};

struct render_entry_bitmap {
	render_entity_basis EntityBasis;
	loaded_bitmap* Bitmap;
	v4 Color;
};

struct render_entry_coordinate_system {
	v2 Origin;
	v2 AxisX;
	v2 AxisY;
	v4 Color;

	loaded_bitmap* Texture;
	loaded_bitmap* NormalMap;

	environment_map* Top;
	environment_map* Middle;
	environment_map* Bottom;
};