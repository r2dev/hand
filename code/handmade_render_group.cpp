#define IGNORED_TIMED_FUNCTION() TIMED_FUNCTION()

inline entity_basis_p_result
GetRenderEntityBasisP(camera_transform CameraTransform, object_transform ObjectTransform, v3 OriginP) {
	entity_basis_p_result Result = {};
    
	v3 P = OriginP + ObjectTransform.OffsetP;
    //r32 Pw = OriginP.w + ObjectTransform.OffsetP.w;
    
	if (CameraTransform.Orthographic == false) {
        r32 DistanceAboveTarget = CameraTransform.DistanceAboveTarget;
        
        if(Global_Renderer_UseDebugCamera)
        {
            DistanceAboveTarget += Global_Renderer_Camera_DebugCameraDistance;
        }
        
		r32 DistancePz = DistanceAboveTarget - P.z;
		v3 RawXY = V3(P.xy, 1.0f);
		r32 NearClipPlane = 0.2f;
        
		if (DistancePz > NearClipPlane) {
			v3 ProjectedXY = 1.0f / DistancePz * CameraTransform.FocalLength * RawXY;
			Result.Scale = CameraTransform.MetersToPixels * ProjectedXY.z;
			v2 EntityGround = CameraTransform.ScreenCenter + CameraTransform.MetersToPixels * ProjectedXY.xy;
			Result.P = EntityGround;
			Result.Valid = true;
		}
	}
	else {
		Result.Scale = CameraTransform.MetersToPixels;
		Result.Valid = true;
		Result.P = CameraTransform.ScreenCenter + CameraTransform.MetersToPixels * P.xy;
	}
    r32 Pw = 0;
    r32 PerspectiveZ = Result.Scale;
    r32 DisplacementZ = Result.Scale * Pw;
    r32 PerspectiveSortTerm = 4096.0f *PerspectiveZ;
    r32 YSortTerm = -1024.0f * P.y;
    r32 ZSortTerm = DisplacementZ;
    
    // TODO(NAME): 
    Result.SortKey = PerspectiveSortTerm + YSortTerm + ZSortTerm + ObjectTransform.SortBias;
    
	return(Result);
}

#define PushRenderElement(Group, type, SortKey)(type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type, SortKey)


inline void*
PushRenderElement_(render_group* Group, u32 Size, render_group_entry_type Type, r32 SortKey) {
    game_render_commands *Commands = Group->Commands;
    
	void* Result = 0;
	Size += sizeof(render_group_entry_header);
	if (Commands->PushBufferSize + Size < Commands->SortEntryAt - sizeof(sort_entry)) {
        
        render_group_entry_header* Header = (render_group_entry_header*)(Commands->PushBufferBase + Commands->PushBufferSize);
		Header->Type = (u16)Type;
        Header->ClipRectIndex = SafeTruncateToU16(Group->CurrentClipRectIndex);
		Result = Header + 1;
        
        Commands->SortEntryAt -= sizeof(sort_entry);
        sort_entry *Entry = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
        Entry->SortKey = SortKey;
        
        Entry->PushBufferOffset = Commands->PushBufferSize;
        
		Commands->PushBufferSize += Size;
        ++Commands->PushBufferElementSize;
	}
	else {
		InvalidCodePath;
	}
	return(Result);
}

inline used_bitmap_dim
GetBitmapDim(render_group* Group, object_transform ObjectTransform, loaded_bitmap* Bitmap, r32 Height, v3 Offset, r32 CAlign, v2 XAxis = v2{1, 0}, v2 YAxis = v2{0, 1}) {
    used_bitmap_dim Result;
    Result.Size = v2{ Height * Bitmap->WidthOverHeight, Height };
	Result.Align = CAlign * Hadamard(Result.Size, Bitmap->AlignPercentage);
    Result.P.z = Offset.z;
	Result.P.xy = Offset.xy - Result.Align.x * XAxis - Result.Align.y * YAxis;
	Result.Basis = GetRenderEntityBasisP(Group->CameraTransform, ObjectTransform, Result.P);
    
    return(Result);
}

inline v4
StoreColor(render_group *Group, v4 Src) {
    v4 Dest;
    v4 t = Group->tGlobalColor;
    v4 C = Group->GlobalColor;
    
    Dest.a = Lerp(Src.a, t.a, C.a);
    
    Dest.r = Dest.a * Lerp(Src.r, t.r, C.r);
    Dest.g = Dest.a * Lerp(Src.g, t.g, C.g);
    Dest.b = Dest.a * Lerp(Src.b, t.b, C.b);
    
    
    return(Dest);
}

inline void
PushBitmap(render_group* Group, object_transform ObjectTransform, loaded_bitmap* Bitmap, r32 Height, v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }, r32 CAlign = 1.0f, v2 XAxis = v2{1, 0}, v2 YAxis = v2{0, 1}) {
	used_bitmap_dim BitmapDim = GetBitmapDim(Group, ObjectTransform, Bitmap, Height, Offset, CAlign, XAxis, YAxis);
	if (BitmapDim.Basis.Valid) {
		render_entry_bitmap* Entry = PushRenderElement(Group, render_entry_bitmap, BitmapDim.Basis.SortKey);
		if (Entry) {
			Entry->P = BitmapDim.Basis.P;
			Entry->Bitmap = Bitmap;
			Entry->PremulColor = StoreColor(Group, Color);
			v2 Size = BitmapDim.Basis.Scale * BitmapDim.Size;
            Entry->XAxis = Size.x * XAxis;
            Entry->YAxis = Size.y * YAxis;
		}
	}
}


inline void
PushBitmap(render_group* Group, object_transform ObjectTransform, bitmap_id ID, r32 Height, v3 Offset, v4 Color = v4{ 1.0f, 1.0, 1.0f, 1.0f }, r32 CAlign = 1.0f, v2 XAxis = v2{1, 0}, v2 YAxis = v2{0, 1}) {
	loaded_bitmap* Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
    
    if (Group->RenderInBackground && !Bitmap) {
        LoadBitmap(Group->Assets, ID, true);
        Bitmap = GetBitmap(Group->Assets, ID, Group->GenerationID);
        
    }
	if (Bitmap) {
		PushBitmap(Group, ObjectTransform, Bitmap, Height, Offset, Color, CAlign, XAxis, YAxis);
	}
	else {
        Assert(!Group->RenderInBackground);
		LoadBitmap(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
	}
}

inline loaded_font*
PushFont(render_group* Group, font_id ID) {
	loaded_font* Font = GetFont(Group->Assets, ID, Group->GenerationID);
	if (Font) {
		//PushBitmap(Group, Font, Height, Offset, Color);
	}
	else {
        Assert(!Group->RenderInBackground);
		LoadFont(Group->Assets, ID, false);
        ++Group->MissingResourceCount;
	}
    return(Font);
}

inline void
PushRect(render_group* Group, object_transform ObjectTransform, v3 Offset, v2 Dim, v4 Color) {
	v3 P = Offset - V3(0.5f * Dim, 0);
	entity_basis_p_result Basis = GetRenderEntityBasisP(Group->CameraTransform, ObjectTransform, P);
	if (Basis.Valid) {
		render_entry_rectangle* Entry = PushRenderElement(Group, render_entry_rectangle, Basis.SortKey);
		if (Entry) {
			Entry->P = Basis.P;
			Entry->PremulColor = StoreColor(Group, Color);
			Entry->Dim = Basis.Scale * Dim;
		}
	}
}

inline void
PushRect(render_group* Group, object_transform ObjectTransform, rectangle2 Rect, r32 Z, v4 Color) {
	PushRect(Group, ObjectTransform, V3(GetCenter(Rect), Z), GetDim(Rect), Color);
}

inline void
PushRectOutline(render_group* Group, object_transform ObjectTransform, v3 Offset, v2 Dim, v4 Color, r32 Thickness = 0.1f) {
	// top - bottom
	PushRect(Group, ObjectTransform, Offset - v3{ 0, 0.5f * Dim.y, 0 }, v2{ Dim.x, Thickness }, Color);
	PushRect(Group, ObjectTransform, Offset + v3{ 0, 0.5f * Dim.y, 0 }, v2{ Dim.x, Thickness }, Color);
    
	PushRect(Group, ObjectTransform, Offset - v3{ 0.5f * Dim.x, 0, 0 }, v2{ Thickness, Dim.y }, Color);
	PushRect(Group, ObjectTransform, Offset + v3{ 0.5f * Dim.x, 0, 0 }, v2{ Thickness, Dim.y }, Color);
}

inline void
Clear(render_group* Group, v4 Color) {
    
	render_entry_clear* Piece = PushRenderElement(Group, render_entry_clear, Real32Minimum);
	if (Piece) {
		Piece->PremulColor = StoreColor(Group, Color);
	}
}

render_entry_coordinate_system*
CoordinateSystem(render_group* Group, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
                 environment_map* Top, environment_map* Middle, environment_map* Bottom) {
	render_entry_coordinate_system* Entry = PushRenderElement(Group, render_entry_coordinate_system, Real32Minimum);
	if (Entry) {
		Entry->AxisX = AxisX;
		Entry->AxisY = AxisY;
		Entry->Color = Color;
		Entry->Origin = Origin;
		Entry->Texture = Texture;
		Entry->NormalMap = NormalMap;
		Entry->Top = Top;
		Entry->Middle = Middle;
		Entry->Bottom = Bottom;
	}
	return(Entry);
}

inline u32
PushClipRect(render_group* Group, u32 X, u32 Y, u32 W, u32 H, clip_rect_fx FX = {}) {
    u32 Result = 0;
	u32 Size = sizeof(render_entry_cliprect);
    game_render_commands *Commands = Group->Commands;
	if (Commands->PushBufferSize + Size < Commands->SortEntryAt - sizeof(sort_entry)) {
        render_entry_cliprect *Entry = (render_entry_cliprect *)(Commands->PushBufferBase + Commands->PushBufferSize);
        Commands->PushBufferSize += Size;
        
        Result = Group->Commands->ClipRectCount++;
        if (Group->Commands->LastRect) {
            Group->Commands->LastRect = Group->Commands->LastRect->Next = Entry;
        } else {
            Group->Commands->FirstRect = Group->Commands->LastRect = Entry;
        }
        Entry->Next = 0;
        Entry->Rect.MinX = X;
        Entry->Rect.MinY = (u16)Y;
        Entry->Rect.MaxX = (u16)X + W;
        Entry->Rect.MaxY = (u16)Y + H;
    }
    return(Result);
}

inline u32
PushClipRect(render_group* Group, object_transform ObjectTransform, v3 Offset, v2 Dim) {
    u32 Result = 0;
    v3 P = Offset - V3(0.5f * Dim, 0);
	entity_basis_p_result Basis = GetRenderEntityBasisP(Group->CameraTransform, ObjectTransform, P);
	if (Basis.Valid) {
        v2 BasisP = Basis.P;
        v2 BasisDim = Basis.Scale * Dim;
        Result = PushClipRect(Group, RoundReal32ToUInt32(BasisP.x), RoundReal32ToUInt32(BasisP.y), RoundReal32ToUInt32(BasisDim.x), RoundReal32ToUInt32(BasisDim.y));
	}
    return(Result);
}

inline void
Perspective(render_group* RenderGroup, u32 PixelWidth, u32 PixelHeight, r32 FocalLength, r32 DistanceAboveTarget) {
	r32 WidthOfMonitor = 0.635f;
	r32 MetersToPixels = (r32)PixelWidth * WidthOfMonitor;
	r32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
	RenderGroup->CameraTransform.FocalLength = FocalLength;
	RenderGroup->CameraTransform.DistanceAboveTarget = DistanceAboveTarget;
	RenderGroup->CameraTransform.MetersToPixels = MetersToPixels;
	RenderGroup->MonitorHalfDimInMeters = v2{
		0.5f * PixelsToMeters * PixelWidth,
		0.5f * PixelsToMeters * PixelHeight
	};
	RenderGroup->CameraTransform.ScreenCenter = v2{ 0.5f * PixelWidth, 0.5f * PixelHeight };
	RenderGroup->CameraTransform.Orthographic = false;
    
    RenderGroup->CurrentClipRectIndex = PushClipRect(RenderGroup, 0, 0, PixelWidth, PixelHeight);
}


inline void
Orthographic(render_group* RenderGroup, u32 PixelWidth, u32 PixelHeight, r32 MetersToPixels) {
	r32 PixelsToMeters = SafeRatio1(1.0f, MetersToPixels);
	RenderGroup->CameraTransform.FocalLength = 1.0f;
	RenderGroup->CameraTransform.DistanceAboveTarget = 1.0f;
	RenderGroup->CameraTransform.MetersToPixels = MetersToPixels;
	RenderGroup->MonitorHalfDimInMeters = v2{
		0.5f * PixelsToMeters * PixelWidth,
		0.5f * PixelsToMeters * PixelHeight
	};
	RenderGroup->CameraTransform.ScreenCenter = v2{ 0.5f * PixelWidth, 0.5f * PixelHeight };
	RenderGroup->CameraTransform.Orthographic = true;
    
    RenderGroup->CurrentClipRectIndex = PushClipRect(RenderGroup, 0, 0, PixelWidth, PixelHeight);
}

inline b32
AllResoucePresent(render_group* Group) {
	b32 Result = (Group->MissingResourceCount == 0);
	return(Result);
}
inline v3
Unproject(render_group* Group, object_transform ObjectTransform, v2 PixelsXY) {
    v2 UnprojectedXY = {};
    camera_transform CameraTransform = Group->CameraTransform;
    
    if (CameraTransform.Orthographic) {
        UnprojectedXY = (1.0f / CameraTransform.MetersToPixels) * (PixelsXY - CameraTransform.ScreenCenter);
    } else {
        v2 dP = (PixelsXY - CameraTransform.ScreenCenter) * (1.0f / CameraTransform.MetersToPixels);
        UnprojectedXY = ((CameraTransform.DistanceAboveTarget - ObjectTransform.OffsetP.z) / CameraTransform.FocalLength) * dP;
    }
    v3 Result = V3(UnprojectedXY, ObjectTransform.OffsetP.z);
    Result -= ObjectTransform.OffsetP;
    return(Result);
}

inline render_group
BeginRenderGroup(game_assets* Assets, game_render_commands *Commands, u32 GenerationID, b32 RenderInBackground) {
	
    render_group Result = {};
    Result.GenerationID = GenerationID;
    
    Result.CurrentClipRectIndex = 0;
    
    Result.Commands = Commands;
    
    Result.Assets = Assets;
	
	Result.MissingResourceCount = 0;
    Result.RenderInBackground = RenderInBackground;
    
    return(Result);
}
inline void
EndRenderGroup(render_group *RenderGroup) {
    
}