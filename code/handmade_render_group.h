#pragma once


struct loaded_bitmap {
    void* Memory;
	v2 AlignPercentage;
	r32 WidthOverHeight;
    
	s32 Width;
	s32 Height;
	s32 Pitch;
    
    // opengl handle texture
    u32 TextureHandle;
};

struct environment_map {
	loaded_bitmap LOD[4];
	r32 Pz;
};

enum render_group_entry_type {
	RenderGroupEntryType_render_entry_clear,
	RenderGroupEntryType_render_entry_rectangle,
	RenderGroupEntryType_render_entry_bitmap,
	RenderGroupEntryType_render_entry_coordinate_system
};

struct tile_sort_entry {
    u32 PushBufferOffset;
    r32 SortKey;
#if HANDMADE_INTERNAL
    render_group_entry_type Type;
#endif
};

struct render_group_entry_header {
	render_group_entry_type Type;
};

struct render_basis {
	v3 P;
	r32 Scale;
};

struct render_entity_basis {
	render_basis* Basis;
	v3 Offset;
};

struct object_transform {
    b32 Upright;
    r32 Scale;
	v3 OffsetP;
};

struct camera_transform {
    b32 Orthographic;
    
	r32 DistanceAboveTarget;
	r32 FocalLength;
	r32 MetersToPixels;
    v2 ScreenCenter;
};

inline object_transform
DefaultUprightTransform() {
    object_transform Result = {};
	Result.Scale = 1.0f;
    Result.Upright = true;
    return Result;
}

inline object_transform
DefaultFlatTransform() {
    object_transform Result = {};
	Result.Scale = 1.0f;
    return Result;
}

struct render_group {
    b32 IsHardware;
	
	r32 GlobalAlpha;
	v2 MonitorHalfDimInMeters;
	
	camera_transform CameraTransform;
    
	struct game_assets* Assets;
    
	u32 MissingResourceCount;
    u32 GenerationID;
    b32 RenderInBackground;
    
    game_render_commands *Commands;
};

struct render_entry_clear {
	v4 Color;
};

struct render_entry_rectangle {
	v2 P;
	v4 Color;
	v2 Dim;
    
};

struct render_entry_bitmap {
	v2 P;
	loaded_bitmap* Bitmap;
	v4 Color;
	v2 Size;
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

struct entity_basis_p_result {
	v2 P;
	r32 Scale;
	b32 Valid;
    r32 SortKey;
};

struct used_bitmap_dim {
    v2 Size;
	v2 Align;
	v3 P;
	entity_basis_p_result Basis;
};

void DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, r32 PixelsToMeters, rectangle2i ClipRect, b32 Even);