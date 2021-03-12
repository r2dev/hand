#define IGNORED_TIMED_FUNCTION() 
global_variable b32 Global_Renderer_ShowLightning_SampleSource = false;

inline void
Swap(sort_entry *A, sort_entry *B) {
    sort_entry Temp = *A;
    *A = *B;
    *B = Temp;
}

struct sort_sprite_bound {
    r32 YMin;
    r32 YMax;
    r32 ZMax;
    u32 Index;
};

inline void
Swap(sort_sprite_bound *A, sort_sprite_bound *B) {
    sort_sprite_bound Temp = *A;
    *A = *B;
    *B = Temp;
}

inline b32
IsInFrontOf(sort_sprite_bound *A, sort_sprite_bound *B) {
    b32 BothZSprites = ((A->YMin == A->YMax) && (B->YMin == B->YMax));
    b32 AIncludesB = ((B->YMin >= A->YMin) && (B->YMax <= A->YMax));
    b32 BIncludesA = ((A->YMin >= B->YMin) && (A->YMax <= B->YMax));
    b32 SortByZ = BothZSprites || AIncludesB || BIncludesA;
    b32 Result = SortByZ? (A->ZMax > B->ZMax): (A->YMin < B->YMin);
}

internal void
MergeSort(sort_sprite_bound *First, u32 Count, sort_sprite_bound *Temp) {
    if (Count == 2) {
        sort_sprite_bound *EntryA = First;
        sort_sprite_bound *EntryB = First + 1;
        if (IsInFrontOf(EntryB, EntryA)) {
            Swap(EntryA, EntryB);
        }
    } else if (Count > 2) {
        u32 Half0 = Count / 2;
        sort_sprite_bound *FirstHalf = First;
        sort_sprite_bound *SecondHalf = First + Half0;
        MergeSort(FirstHalf, Half0, Temp);
        MergeSort(SecondHalf, Count - Half0, Temp);
        sort_sprite_bound *FirstRead = First;
        sort_sprite_bound *SecondRead = First + Half0;
        sort_sprite_bound *End = First + Count;
        
        sort_sprite_bound *Out = Temp;
        while(FirstRead != SecondHalf && SecondRead != End) {
            
            if (IsInFrontOf(SecondRead, FirstRead)) {
                *Out++ = *FirstRead++;
            } else {
                *Out++ = *SecondRead++;
            }
        }
        if (FirstRead == SecondHalf) {
            while (SecondRead != End) {
                *Out++ = *SecondRead++;
            }
        }
        if (SecondRead == End) {
            while (FirstRead != SecondHalf) {
                *Out++ = *FirstRead++;
            }
        }
        
        for (u32 Index = 0; Index < Count; ++Index) {
            First[Index] = Temp[Index];
        }
    }
}

inline u32
SortKeyToU32(r32 SortKey) {
    u32 Result = *(u32 *)&SortKey;
    if (Result & 0x80000000) {
        Result = ~Result;
    } else {
        Result |= 0x80000000;
    }
    return(Result);
}

internal void
RadixSort(sort_entry *First, u32 Count, sort_entry *Temp) {
    sort_entry *Src = First;
    sort_entry *Dest = Temp;
    for (u32 ByteIndex = 0; ByteIndex < 32; ByteIndex += 8) {
        u32 SortKeyOffset[256] = {};
        for (u32 Index = 0; Index < Count; ++Index) {
            // sort_entry *Entry = First + Index;
            u32 RadixValue = SortKeyToU32(Src[Index].SortKey);
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            ++SortKeyOffset[RadixPiece];
        }
        u32 Total = 0;
        for (u32 SortKeyIndex = 0; SortKeyIndex < 256; SortKeyIndex++) {
            u32 KeyCount = SortKeyOffset[SortKeyIndex];
            SortKeyOffset[SortKeyIndex] = Total;
            Total += KeyCount;
        }
        for(u32 Index = 0; Index < Count; ++Index) {
            u32 RadixValue = SortKeyToU32(Src[Index].SortKey);
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            Dest[SortKeyOffset[RadixPiece]++] = Src[Index];
        }
        // TODO(NAME): why? the dest value is ready, we can discard the src value
        sort_entry *SwapTemp = Dest;
        Dest = Src;
        Src = SwapTemp;
    }
}

internal void
BubbleSort(sort_entry *SortEntries, u32 Count) {
    for (u32 Outer = 0; Outer < Count; ++Outer) {
        b32 EarlyOut = true;
        for (u32 Inner = 0; Inner < Count - 1; ++Inner) {
            
            sort_entry *EntryA = SortEntries + Inner;
            sort_entry *EntryB = SortEntries + Inner + 1;
            if (EntryA->SortKey > EntryB->SortKey) {
                EarlyOut = false;
                Swap(EntryA, EntryB);
            }
        }
        if (EarlyOut) {
            break;
        }
    }
}

internal void
SortEntries(game_render_commands *Commands, void *Temp) {
    sort_entry *SortEntries = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
    u32 Count = Commands->PushBufferElementSize;
    
    // BubbleSort(SortEntries, Count);
    //MergeSort(SortEntries, Count, (sort_entry *)Temp);
    RadixSort(SortEntries, Count, (sort_entry *)Temp);
#if 0
    // verifier
    for (u32 Index = 0; Index < Commands->PushBufferElementSize - 1; ++Index) {
        sort_entry *EntryA = SortEntries + Index;
        sort_entry *EntryB = SortEntries + Index + 1;
        Assert(EntryA->SortKey <= EntryB->SortKey);
    }
#endif
}

internal void
LinearizeClipRects(game_render_commands *Commands, void *ClipMemory) {
    render_entry_cliprect *Out = (render_entry_cliprect *)ClipMemory;
    for (render_entry_cliprect *Rect = Commands->FirstRect; Rect; Rect = Rect->Next) {
        *Out++ = *Rect;
    }
    Commands->ClipRects = (render_entry_cliprect *)ClipMemory;
}


internal void
DrawRectangle(loaded_bitmap* Buffer, v2 vMin, v2 vMax, v4 Color, rectangle2i ClipRect, b32 Even) {
    IGNORED_TIMED_FUNCTION();
    r32 R = Color.r;
    r32 G = Color.g;
    r32 B = Color.b;
    r32 A = Color.a;
    
    rectangle2i FillRect;
    FillRect.MinX = RoundReal32ToInt32(vMin.x);
    FillRect.MinY = RoundReal32ToInt32(vMin.y);
    FillRect.MaxX = RoundReal32ToInt32(vMax.x);
    FillRect.MaxY = RoundReal32ToInt32(vMax.y);
    
    FillRect = Intersect(FillRect, ClipRect);
    
    if (!Even == (FillRect.MinY & 1)) {
        FillRect.MinY += 1;
    }
    
    // BIT PATTERN AA RR GG BB
    u32 Color32 = (u32)
        (RoundReal32ToUInt32(A * 255) << 24 |
         RoundReal32ToUInt32(R * 255) << 16 |
         RoundReal32ToUInt32(G * 255) << 8 |
         RoundReal32ToUInt32(B * 255) << 0);
    
    u8* Row = ((u8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);
    
    for (int Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
        u32* Pixel = (u32*)Row;
        for (int X = FillRect.MinX; X < FillRect.MaxX; ++X) {
            *Pixel++ = Color32;
        }
        Row += 2 * Buffer->Pitch;
    }
}


internal void
RenderCommandsToBitmap(game_render_commands* RenderCommands, loaded_bitmap* OutputTarget, rectangle2i ClipRect, b32 Even) {
    IGNORED_TIMED_FUNCTION();
    v2 ScreenDim = { (r32)OutputTarget->Width, (r32)OutputTarget->Height };
    for (u32 BaseAddress = 0; BaseAddress < RenderCommands->PushBufferSize;) {
        render_group_entry_header* Header = (render_group_entry_header*)(RenderCommands->PushBufferBase + BaseAddress);
        BaseAddress += sizeof(*Header);
        void* Data = Header + 1;
        
        //todo null
        r32 NullPixelsToMeters = 1.0f;
        
        switch (Header->Type) {
            case RenderGroupEntryType_render_entry_clear: {
                render_entry_clear* Entry = (render_entry_clear*)Data;
                
                DrawRectangle(OutputTarget, v2{ 0, 0 }, v2{ (r32)OutputTarget->Width, (r32)OutputTarget->Height }, Entry->PremulColor, ClipRect, Even);
                BaseAddress += sizeof(*Entry);
            } break;
            
            case RenderGroupEntryType_render_entry_rectangle: {
                render_entry_rectangle* Entry = (render_entry_rectangle*)Data;
                
                DrawRectangle(OutputTarget, Entry->P, Entry->P + Entry->Dim, Entry->PremulColor, ClipRect, Even);
                
                BaseAddress += sizeof(*Entry);
            } break;
            case RenderGroupEntryType_render_entry_bitmap: {
                render_entry_bitmap* Entry = (render_entry_bitmap*)Data;
                v2 XAxis = {1, 0};
                v2 YAxis = {0, 1};
                DrawRectangle2(OutputTarget, Entry->P, Entry->XAxis, Entry->YAxis, Entry->PremulColor, Entry->Bitmap, NullPixelsToMeters, ClipRect, Even);
                
                BaseAddress += sizeof(*Entry);
                
            } break;
            case RenderGroupEntryType_render_entry_coordinate_system: {
                render_entry_coordinate_system* Entry = (render_entry_coordinate_system*)Data;
#if 0
                v2 P = Entry->Origin;
                
                DrawRectangle1(OutputTarget, P, Entry->AxisX, Entry->AxisY, Entry->Color, Entry->Texture,
                               Entry->NormalMap, Entry->Top, Entry->Middle, Entry->Bottom, PixelsToMeters);
                v2 Dim = { 2, 2 };
                v4 Color = { 1.0f, 1.0f, 0.0f, 1.0f };
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                
                P = Entry->Origin + Entry->AxisX;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                
                P = Entry->Origin + Entry->AxisY;
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
                P = Entry->Origin + Entry->AxisY + Entry->AxisX;
                
                DrawRectangle(OutputTarget, P - Dim, P + Dim, Color);
#endif
                BaseAddress += sizeof(*Entry);
            } break;
            InvalidDefaultCase;
        }
    }
}

struct tile_render_work {
    game_render_commands* RenderCommands;
    loaded_bitmap* OutputTarget;
    rectangle2i ClipRect;
};

internal PLATFORM_WORK_QUEUE_CALLBACK(DoTileRenderWork) {
    tile_render_work* Work = (tile_render_work*)Data;
    
    RenderCommandsToBitmap(Work->RenderCommands, Work->OutputTarget, Work->ClipRect, false);
    RenderCommandsToBitmap(Work->RenderCommands, Work->OutputTarget, Work->ClipRect, true);
}

internal void 
SoftwareRenderCommands(platform_work_queue* Queue, game_render_commands *RenderCommands, loaded_bitmap* OutputTarget) {
    IGNORED_TIMED_FUNCTION();
    u32 const TileCountX = 4;
    u32 const TileCountY = 4;
    
    
    Assert(((uintptr)OutputTarget->Memory & 15) == 0);
    u32 TileWidth = OutputTarget->Width / TileCountX;
    u32 TileHeight = OutputTarget->Height / TileCountY;
    TileWidth = ((TileWidth + 3) / 4) * 4;
#if 1
    tile_render_work Works[TileCountX * TileCountY];
    for (u32 Y = 0; Y < TileCountY; Y++) {
        for (u32 X = 0; X < TileCountX; X++) {
            
            
            rectangle2i ClipRect;
            ClipRect.MinX = X * TileWidth;
            ClipRect.MinY = Y * TileHeight;
            
            ClipRect.MaxY = (Y + 1) * TileHeight;
            ClipRect.MaxX = (X + 1) * TileWidth;
            
            
            if (X + 1 == TileCountX) {
                ClipRect.MaxX = OutputTarget->Width;
            }
            if (Y + 1 == TileCountY) {
                ClipRect.MaxY = OutputTarget->Height;
            }
            
            tile_render_work *Work = Works + (Y * 4 + X);
            Work->ClipRect = ClipRect;
            Work->OutputTarget = OutputTarget;
            Work->RenderCommands = RenderCommands;
            
            Platform.AddEntry(Queue, DoTileRenderWork, Work);
        }
    }
    Platform.CompleteAllWork(Queue);
#else
    rectangle2i Cilp = { 0, 0, OutputTarget->Width, OutputTarget->Height };
    RenderGroupToOutput(RenderGroup, OutputTarget, Cilp, false);
#endif
}

internal void
DrawBitmap(loaded_bitmap* Buffer, loaded_bitmap* Bitmap,
           r32 RealX, r32 RealY, r32 CAlpha = 1.0f) {
    IGNORED_TIMED_FUNCTION();
    s32 MinX = RoundReal32ToInt32(RealX);
    s32 MinY = RoundReal32ToInt32(RealY);
    s32 MaxX = MinX + Bitmap->Width;
    s32 MaxY = MinY + Bitmap->Height;
    
    s32 SourceOfferX = 0;
    if (MinX < 0) {
        SourceOfferX = -MinX;
        MinX = 0;
    }
    s32 SourceOfferY = 0;
    if (MinY < 0) {
        SourceOfferY = -MinY;
        MinY = 0;
    }
    if (MaxY > Buffer->Height) {
        MaxY = Buffer->Height;
    }
    if (MaxX > Buffer->Width) {
        MaxX = Buffer->Width;
    }
    
    u8* SourceRow = (u8*)Bitmap->Memory + SourceOfferY * Bitmap->Pitch + BITMAP_BYTE_PER_PIXEL * SourceOfferX;
    u8* DestRow = ((u8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);
    
    for (int Y = MinY; Y < MaxY; ++Y) {
        u32* Dest = (u32*)DestRow;
        u32* Source = (u32*)SourceRow;
        for (int X = MinX; X < MaxX; ++X) {
#if 0
            r32 SA = ((r32)((*Source >> 24) & 0xFF));
            r32 RSA = (SA / 255.0f) * CAlpha;
            r32 SR = CAlpha * (r32)((*Source >> 16) & 0xFF);
            r32 SG = CAlpha * (r32)((*Source >> 8) & 0xFF);
            r32 SB = CAlpha * (r32)((*Source >> 0) & 0xFF);
            
            r32 DA = (r32)((*Dest >> 24) & 0xFF);
            r32 DR = (r32)((*Dest >> 16) & 0xFF);
            r32 DG = (r32)((*Dest >> 8) & 0xFF);
            r32 DB = (r32)((*Dest >> 0) & 0xFF);
            r32 RDA = (DA / 255.0f);
            
            r32 InvRSA = (1.0f - RSA);
            
            r32 A = 255.0f * (RSA + RDA - RSA * RDA);
            r32 R = InvRSA * DR + SR;
            r32 G = InvRSA * DG + SG;
            r32 B = InvRSA * DB + SB;
            
            *Dest = (u32)(A + 0.5f) << 24 | (u32)(R + 0.5f) << 16 | (u32)(G + 0.5f) << 8 | (u32)(B + 0.5f) << 0;
#else
            v4 Texel = {
                (r32)((*Source >> 16) & 0xFF),
                (r32)((*Source >> 8) & 0xFF),
                (r32)((*Source >> 0) & 0xFF),
                (r32)((*Source >> 24) & 0xFF)
            };
            
            Texel = SRGBToLinear1(Texel);
            Texel *= CAlpha;
            v4 D = {
                (r32)((*Dest >> 16) & 0xFF),
                (r32)((*Dest >> 8) & 0xFF),
                (r32)((*Dest >> 0) & 0xFF),
                (r32)((*Dest >> 24) & 0xFF)
            };
            D = SRGBToLinear1(D);
            v4 Result = (1.0f - Texel.a) * D + Texel;
            Result = Linear1ToSRGB(Result);
            *Dest = (u32)(Result.a + 0.5f) << 24 |
                (u32)(Result.r + 0.5f) << 16 |
                (u32)(Result.g + 0.5f) << 8 |
                (u32)(Result.b + 0.5f) << 0;
#endif
            ++Dest;
            ++Source;
        }
        DestRow += Buffer->Pitch;
        SourceRow += Bitmap->Pitch;
    }
}



inline v4
Unpack4x8(u32 Packed) {
    v4 Result = v4{
        (r32)((Packed >> 16) & 0xFF),
        (r32)((Packed >> 8) & 0xFF),
        (r32)((Packed >> 0) & 0xFF),
        (r32)((Packed >> 24) & 0xFF)
    };
    return(Result);
}

struct bilinear_sample {
    u32 A, B, C, D;
};

inline bilinear_sample
BilinearSample(loaded_bitmap* Texture, s32 X, s32 Y) {
    bilinear_sample Result = {};
    u8* Color8 = (u8*)Texture->Memory + Texture->Pitch * Y + X * sizeof(u32);
    Result.A = *(u32*)Color8;
    Result.B = *(u32*)(Color8 + sizeof(u32));
    
    Result.C = *(u32*)(Color8 + Texture->Pitch);
    Result.D = *(u32*)(Color8 + Texture->Pitch + sizeof(u32));
    return(Result);
}

inline v4
SRGBBilinearBlend(bilinear_sample TexelSample, r32 fx, r32 fy) {
    v4 Result = {};
    v4 TexelA = Unpack4x8(TexelSample.A);
    v4 TexelB = Unpack4x8(TexelSample.B);
    v4 TexelC = Unpack4x8(TexelSample.C);
    v4 TexelD = Unpack4x8(TexelSample.D);
    TexelA = SRGBToLinear1(TexelA);
    TexelB = SRGBToLinear1(TexelB);
    TexelC = SRGBToLinear1(TexelC);
    TexelD = SRGBToLinear1(TexelD);
    Result = Lerp(Lerp(TexelA, fx, TexelB), fy, Lerp(TexelC, fx, TexelD));
    return(Result);
}


inline v4
UnscaleAndBiasNormal(v4 Normal)
{
    v4 Result;
    
    r32 Inv255 = 1.0f / 255.0f;
    
    Result.x = -1.0f + 2.0f * (Inv255 * Normal.x);
    Result.y = -1.0f + 2.0f * (Inv255 * Normal.y);
    Result.z = -1.0f + 2.0f * (Inv255 * Normal.z);
    
    Result.w = Inv255 * Normal.w;
    
    return(Result);
}

internal v3
SampleEnvironmentMap(v2 ScreenUV, v3 SampleDirection, r32 Roughness, environment_map* Map, r32 DistanceFromMapInZ) {
    
    u32 LODIndex = (u32)(Roughness * (r32)(ArrayCount(Map->LOD) - 1) + 0.5f);
    loaded_bitmap* LOD = Map->LOD + LODIndex;
    
    r32 UVsPerMeter = 0.1f;
    r32 C = (UVsPerMeter * DistanceFromMapInZ) / SampleDirection.y;
    v2 Offset = C * v2{ SampleDirection.x, SampleDirection.z };
    
    v2 UV = ScreenUV + Offset;
    UV.x = Clamp01(UV.x);
    UV.y = Clamp01(UV.y);
    
    r32 tX = UV.x * (r32)(LOD->Width - 2);
    r32 tY = UV.y * (r32)(LOD->Height - 2);
    
    s32 X = (s32)tX;
    s32 Y = (s32)tY;
    
    r32 fX = tX - (r32)X;
    r32 fY = tY - (r32)Y;
    
    Assert(X >= 0 && X < LOD->Width);
    Assert(Y >= 0 && Y < LOD->Height);
    if(Global_Renderer_ShowLightning_SampleSource)
    {
        u8 *TexelPtr = ((u8*)LOD->Memory + Y*LOD->Pitch + X *sizeof(u32));
        *(u32 *)TexelPtr = 0xFFFFFFFF;
    }
    bilinear_sample Sample = BilinearSample(LOD, X, Y);
    v3 Result = SRGBBilinearBlend(Sample, fX, fY).xyz;
    
    return Result;
}

internal void
DrawRectangle1(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, loaded_bitmap* NormalMap,
               environment_map* Top, environment_map* Middle, environment_map* Bottom, r32 PixelsToMeters) {
    // IGNORED_TIMED_FUNCTION();
#if 0
    u32 Color32 = (u32)(RoundReal32ToUInt32(Color.r * 255) << 16 |
                        RoundReal32ToUInt32(Color.g * 255) << 8 |
                        RoundReal32ToUInt32(Color.b * 255) << 0);
#endif
    
    r32 AxisXLength = Length(AxisX);
    r32 AxisYLength = Length(AxisY);
    
    v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
    v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;
    
    r32 NzScale = 0.5f * (AxisXLength + AxisYLength);
    
    r32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
    r32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
    u32 Color32 = (RoundReal32ToUInt32(Color.a * 255.0f) << 24 |
                   RoundReal32ToUInt32(Color.r * 255.0f) << 16 |
                   RoundReal32ToUInt32(Color.g * 255.0f) << 8 |
                   RoundReal32ToUInt32(Color.b * 255.0f) << 0);
    s32 WidthMax = Buffer->Width - 1;
    s32 HeightMax = Buffer->Height - 1;
    
    r32 invWidthMax = (r32)1.0f / WidthMax;
    r32 invHeightMax = (r32)1.0f / HeightMax;
    s32 MinX = WidthMax;
    s32 MaxX = 0;
    s32 MinY = HeightMax;
    s32 MaxY = 0;
    
    v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };
    
    r32 OriginZ = 0.0f;
    r32 OriginY = (Origin + 0.5f * AxisX + 0.5f * AxisY).y;
    r32 FixedCastY = invHeightMax * OriginY;
    
    for (s32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
        v2 Cur = P[PIndex];
        s32 FloorX = FloorReal32ToInt32(Cur.x);
        s32 FloorY = FloorReal32ToInt32(Cur.y);
        s32 CeilX = CeilReal32ToInt32(Cur.x);
        s32 CeilY = CeilReal32ToInt32(Cur.y);
        
        if (Cur.x > MaxX) { MaxX = CeilX; }
        if (Cur.y > MaxY) { MaxY = CeilY; }
        
        if (FloorX < MinX) { MinX = FloorX; }
        if (FloorY < MinY) { MinY = FloorY; }
    }
    if (MinX < 0) {
        MinX = 0;
    }
    
    if (MinY < 0) {
        MinY = 0;
    }
    if (MaxY > HeightMax) {
        MaxY = HeightMax;
    }
    if (MaxX > WidthMax) {
        MaxX = WidthMax;
    }
    
    u8* Row = ((u8*)Buffer->Memory + MinX * BITMAP_BYTE_PER_PIXEL + MinY * Buffer->Pitch);
    TIMED_BLOCK("PixelFill", (MaxX - MinX + 1) * (MaxY - MinY + 1));
    for (int Y = MinY; Y < MaxY; ++Y) {
        u32* Pixel = (u32*)Row;
        for (int X = MinX; X < MaxX; ++X) {
            v2 tempP = V2i(X, Y);
            v2 D = tempP - Origin;
            
            r32 Edge0 = Inner(D, -Perp(AxisX));
            r32 Edge1 = Inner(D - AxisX, -Perp(AxisY));
            r32 Edge2 = Inner(D - AxisY - AxisX, Perp(AxisX));
            r32 Edge3 = Inner(D - AxisY, Perp(AxisY));
            
            if (Edge0 < 0 && Edge1 < 0 && Edge2 < 0 && Edge3 < 0) {
                
                v2 ScreenSpaceUV = { invWidthMax * (r32)X, FixedCastY };
                r32 ZDiff = PixelsToMeters * ((r32)Y - OriginY);
                
                r32 U = InvAxisXLengthSq * Inner(D, AxisX);
                r32 V = InvAxisYLengthSq * Inner(D, AxisY);
                
                r32 tX = U * (r32)(Texture->Width - 2);
                r32 tY = V * (r32)(Texture->Height - 2);
                
                s32 intX = (s32)tX;
                s32 intY = (s32)tY;
                
                
                r32 fX = tX - (r32)intX;
                r32 fY = tY - (r32)intY;
                
                bilinear_sample TexelSample = BilinearSample(Texture, intX, intY);
                v4 Texel = SRGBBilinearBlend(TexelSample, fX, fY);
                
                if (NormalMap) {
                    bilinear_sample NormalSample = BilinearSample(NormalMap, intX, intY);
                    v4 NormalA = Unpack4x8(NormalSample.A);
                    v4 NormalB = Unpack4x8(NormalSample.B);
                    v4 NormalC = Unpack4x8(NormalSample.C);
                    v4 NormalD = Unpack4x8(NormalSample.D);
                    
                    v4 Normal = Lerp(Lerp(NormalA, fX, NormalB),
                                     fY,
                                     Lerp(NormalC, fX, NormalD));
                    Normal = UnscaleAndBiasNormal(Normal);
                    Normal.xy = Normal.x * NxAxis + Normal.y * NyAxis;
                    Normal.z *= NzScale;
                    Normal.xyz = Normalize(Normal.xyz);
                    
                    v3 EyeDirection = v3{ 0, 0, 1.0f };
                    r32 projectionLength = 2.0f * Inner(EyeDirection, Normal.xyz);
                    v3 BounceDirection = -EyeDirection + projectionLength * Normal.xyz;
                    BounceDirection.z = -BounceDirection.z;
                    
                    environment_map* FarMap = 0;
                    r32 Pz = OriginZ + ZDiff;
                    r32 tEnvMap = BounceDirection.y;
                    r32 tFarMap = 0.0f;
                    if (tEnvMap < -0.5f) {
                        FarMap = Bottom;
                        tFarMap = -1 - 2.0f * tEnvMap;
                    }
                    else if (tEnvMap > 0.5f) {
                        FarMap = Top;
                        tFarMap = 2 * (tEnvMap - 0.5f);
                    }
                    tFarMap *= tFarMap;
                    tFarMap *= tFarMap;
                    
                    v3 LightColor = { 0, 0, 0 };
                    if (FarMap) {
                        r32 DistanceFromMapInZ = FarMap->Pz - Pz;
                        v3 FarMapColor = SampleEnvironmentMap(ScreenSpaceUV, BounceDirection, Normal.w, FarMap, DistanceFromMapInZ);
                        LightColor = Lerp(LightColor, tFarMap, FarMapColor);
                    }
                    
                    Texel.rgb += Texel.a * LightColor;
                }
                
                Texel = Hadamard(Texel, Color);
                Texel.r = Clamp01(Texel.r);
                Texel.g = Clamp01(Texel.g);
                Texel.b = Clamp01(Texel.b);
                v4 Dest = Unpack4x8(*Pixel);
                Dest = SRGBToLinear1(Dest);
                v4 Blended = (1.0f - Texel.a) * Dest + Texel;
                
                Blended = Linear1ToSRGB(Blended);
                
                *Pixel = (u32)(Blended.a + 0.5f) << 24 |
                    (u32)(Blended.r + 0.5f) << 16 |
                    (u32)(Blended.g + 0.5f) << 8 |
                    (u32)(Blended.b + 0.5f) << 0;
                
            }
            *Pixel++;
            
        }
        Row += Buffer->Pitch;
    }
    
}


void
DrawRectangle2(loaded_bitmap* Buffer, v2 Origin, v2 AxisX, v2 AxisY, v4 Color, loaded_bitmap* Texture, r32 PixelsToMeters, rectangle2i ClipRect, b32 Even) {
    TIMED_FUNCTION();
    Assert(Texture->Memory);
    
    
    r32 AxisXLength = Length(AxisX);
    r32 AxisYLength = Length(AxisY);
    
    v2 NxAxis = (AxisYLength / AxisXLength) * AxisX;
    v2 NyAxis = (AxisXLength / AxisYLength) * AxisY;
    
    r32 NzScale = 0.5f * (AxisXLength + AxisYLength);
    
    r32 InvAxisXLengthSq = 1.0f / LengthSq(AxisX);
    r32 InvAxisYLengthSq = 1.0f / LengthSq(AxisY);
    
    rectangle2i FillRect = InvertedInfinityRectangle2i();
    
    v2 P[4] = { Origin, Origin + AxisX, Origin + AxisX + AxisY, Origin + AxisY };
    
    for (s32 PIndex = 0; PIndex < ArrayCount(P); ++PIndex) {
        v2 TestP = P[PIndex];
        s32 FloorX = FloorReal32ToInt32(TestP.x);
        s32 CeilX = CeilReal32ToInt32(TestP.x) + 1;
        
        s32 FloorY = FloorReal32ToInt32(TestP.y);
        s32 CeilY = CeilReal32ToInt32(TestP.y) + 1;
        
        if (FillRect.MinX > FloorX) { FillRect.MinX = FloorX; }
        if (FillRect.MinY > FloorY) { FillRect.MinY = FloorY; }
        
        if (FillRect.MaxX < CeilX) { FillRect.MaxX = CeilX; }
        if (FillRect.MaxY < CeilY) { FillRect.MaxY = CeilY; }
    }
    
    FillRect = Intersect(ClipRect, FillRect);
    
    if (!Even == (FillRect.MinY & 1)) {
        FillRect.MinY += 1;
    }
    
    if (HasArea(FillRect)) {
        __m128i StartClipMask = _mm_set1_epi8(-1);
        __m128i StartClipMasks[] = {
            StartClipMask,
            _mm_slli_si128(StartClipMask, 1 * 4),
            _mm_slli_si128(StartClipMask, 2 * 4),
            _mm_slli_si128(StartClipMask, 3 * 4)
        };
        __m128i EndClipMask = _mm_set1_epi8(-1);
        __m128i EndClipMasks[] = {
            EndClipMask,
            _mm_srli_si128(EndClipMask, 3 * 4),
            _mm_srli_si128(EndClipMask, 2 * 4),
            _mm_srli_si128(EndClipMask, 1 * 4)
        };
        
        if (FillRect.MinX & 3) {
            StartClipMask = StartClipMasks[FillRect.MinX & 3];
            FillRect.MinX -= FillRect.MinX & 3;
        }
        
        if (FillRect.MaxX & 3) {
            EndClipMask = EndClipMasks[FillRect.MaxX & 3];
            FillRect.MaxX += (4 - FillRect.MaxX & 3);
        }
        
        void* TextureMemory = Texture->Memory;
        
        r32 Inv255C = 1.0f / 255.0f;
        r32 One255C = 255.0f;
        __m128 Inv255C_x4 = _mm_set_ps1(Inv255C);
        __m128 One255C_x4 = _mm_set_ps1(One255C);
        __m128 Four_x4 = _mm_set_ps1(4.0f);
        __m128 StepFour_x4 = _mm_set_ps(3.0f, 2.0f, 1.0f, 0.0f);
        __m128 Min_x4 = _mm_set_ps1((r32)FillRect.MinX);
        __m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
        __m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
        __m128 One = _mm_set_ps1(1.0f);
        __m128 Zero = _mm_set_ps1(0.0f);
        __m128 Half = _mm_set_ps1(0.5f);
        __m128 ColorR = _mm_set_ps1(Color.r);
        __m128 ColorG = _mm_set_ps1(Color.g);
        __m128 ColorB = _mm_set_ps1(Color.b);
        __m128 ColorA = _mm_set_ps1(Color.a);
        __m128 OriginX_x4 = _mm_set_ps1(Origin.x);
        __m128 OriginY_x4 = _mm_set_ps1(Origin.y);
        
        __m128 XAxisx = _mm_set_ps1(AxisX.x);
        __m128 XAxisy = _mm_set_ps1(AxisX.y);
        __m128 YAxisx = _mm_set_ps1(AxisY.x);
        __m128 YAxisy = _mm_set_ps1(AxisY.y);
        
        __m128 WidthM2 = _mm_set1_ps((r32)(Texture->Width - 2));
        __m128 HeightM2 = _mm_set1_ps((r32)(Texture->Height - 2));
        
        __m128 InvAxisXLengthSq_x4 = _mm_set_ps1(InvAxisXLengthSq);
        __m128 InvAxisYLengthSq_x4 = _mm_set_ps1(InvAxisYLengthSq);
        
        __m128 NAxisXx = _mm_mul_ps(XAxisx, InvAxisXLengthSq_x4);
        __m128 NAxisXy = _mm_mul_ps(XAxisy, InvAxisXLengthSq_x4);
        __m128 NAxisYx = _mm_mul_ps(YAxisx, InvAxisYLengthSq_x4);
        __m128 NAxisYy = _mm_mul_ps(YAxisy, InvAxisYLengthSq_x4);
        
        
        __m128 MaxColorValue = _mm_set1_ps(255.0f * 255.0f);
        __m128i TexturePitch_x4 = _mm_set1_epi32(Texture->Pitch);
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        s32 RowAdvance = 2 * Buffer->Pitch;
        
        u8* Row = ((u8*)Buffer->Memory + FillRect.MinX * BITMAP_BYTE_PER_PIXEL + FillRect.MinY * Buffer->Pitch);
        
        TIMED_BLOCK("PixelFill", GetClampedRectArea(FillRect) / 2);
        for (s32 Y = FillRect.MinY; Y < FillRect.MaxY; Y += 2) {
            __m128 Y_x4 = _mm_set_ps1((r32)Y);
            __m128 Dy = _mm_sub_ps(Y_x4, OriginY_x4);
            
            __m128 PynX = _mm_mul_ps(Dy, NAxisXy);
            __m128 PynY = _mm_mul_ps(Dy, NAxisYy);
            
            __m128 tempPx = _mm_add_ps(Min_x4, StepFour_x4);
            tempPx = _mm_sub_ps(tempPx, OriginX_x4);
            
            __m128i ClipMask = StartClipMask;
            
            u32* Pixel = (u32*)Row;
            
            for (s32 X = FillRect.MinX; X < FillRect.MaxX; X += 4) {
                // load
                __m128i OriginalDest = _mm_load_si128((__m128i*)Pixel);
                __m128 U = _mm_add_ps(_mm_mul_ps(tempPx, NAxisXx), PynX);
                __m128 V = _mm_add_ps(_mm_mul_ps(tempPx, NAxisYx), PynY);
                
                // WriteMask ClipMask
                __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero), _mm_cmple_ps(U, One)), _mm_and_ps(_mm_cmpge_ps(V, Zero), _mm_cmple_ps(V, One))));
                WriteMask = _mm_and_si128(WriteMask, ClipMask);
                
                // clamp uv 
                U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                V = _mm_min_ps(_mm_max_ps(V, Zero), One);
                
                __m128 tX = _mm_add_ps(_mm_mul_ps(U, WidthM2), Half);
                __m128 tY = _mm_add_ps(_mm_mul_ps(V, HeightM2), Half);
                
                __m128i intX = _mm_cvttps_epi32(tX);
                __m128i intY = _mm_cvttps_epi32(tY);
                
                __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(intX));
                __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(intY));
#if 1	
                __m128i FetchX_4x = _mm_slli_epi32(intX, 2);
                __m128i FetchY_4x = _mm_or_si128(_mm_mullo_epi16(intY, TexturePitch_x4),
                                                 _mm_slli_epi32(_mm_mulhi_epi16(intY, TexturePitch_x4), 16));
                
                __m128i Fetch_4x = _mm_add_epi32(FetchX_4x, FetchY_4x);
                
                s32 Fetch0 = ((s32*)&(Fetch_4x))[0];
                s32 Fetch1 = ((s32*)&(Fetch_4x))[1];
                s32 Fetch2 = ((s32*)&(Fetch_4x))[2];
                s32 Fetch3 = ((s32*)&(Fetch_4x))[3];
                
                u8* TexelPtr0 = ((u8*)TextureMemory) + Fetch0;
                u8* TexelPtr1 = ((u8*)TextureMemory) + Fetch1;
                u8* TexelPtr2 = ((u8*)TextureMemory) + Fetch2;
                u8* TexelPtr3 = ((u8*)TextureMemory) + Fetch3;
                
                
                __m128i SampleA = _mm_setr_epi32(*(u32*)TexelPtr0, *(u32*)TexelPtr1, *(u32*)TexelPtr2, *(u32*)TexelPtr3);
                __m128i SampleB = _mm_setr_epi32(*(u32*)(TexelPtr0 + sizeof(u32)),
                                                 *(u32*)(TexelPtr1 + sizeof(u32)),
                                                 *(u32*)(TexelPtr2 + sizeof(u32)),
                                                 *(u32*)(TexelPtr3 + sizeof(u32)));
                __m128i SampleC = _mm_setr_epi32(*(u32*)(TexelPtr0 + Texture->Pitch),
                                                 *(u32*)(TexelPtr1 + Texture->Pitch),
                                                 *(u32*)(TexelPtr2 + Texture->Pitch),
                                                 *(u32*)(TexelPtr3 + Texture->Pitch));
                __m128i SampleD = _mm_setr_epi32(*(u32*)(TexelPtr0 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr1 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr2 + Texture->Pitch + sizeof(u32)),
                                                 *(u32*)(TexelPtr3 + Texture->Pitch + sizeof(u32)));
                
                __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
                __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
                TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
                __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
                TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);
                
                __m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
                __m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF00FF);
                TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
                __m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
                TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);
                
                __m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
                __m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF00FF);
                TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
                __m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
                TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);
                
                __m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
                __m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF00FF);
                TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
                __m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
                TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);
#else
                __m128i SampleA;
                __m128i SampleB;
                __m128i SampleC;
                __m128i SampleD;
                for (int I = 0; I < 4; ++I) {
                    s32 FetchX = ((u32*)&(intX))[I];
                    s32 FetchY = ((u32*)&(intY))[I];
                    
                    u8* TexelPtr = ((u8*)Texture->Memory) + Texture->Pitch * FetchY + FetchX * sizeof(u32);
                    ((u32*)&(SampleA))[I] = *(u32*)TexelPtr;
                    ((u32*)&(SampleB))[I] = *(u32*)(TexelPtr + sizeof(u32));
                    ((u32*)&(SampleC))[I] = *(u32*)(TexelPtr + Texture->Pitch);
                    ((u32*)&(SampleD))[I] = *(u32*)(TexelPtr + Texture->Pitch + sizeof(u32));
                }
#endif
                __m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
                __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
                __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
                
                __m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
                __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
                __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));
                
                __m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
                __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
                __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));
                
                __m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
                __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
                __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));
                
                
                __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
                __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
                __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
                __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
                
                
#define mmSquare(a) _mm_mul_ps(a, a)
                
                
                __m128 ifX = _mm_sub_ps(One, fX);
                __m128 ifY = _mm_sub_ps(One, fY);
                
                __m128 l1 = _mm_mul_ps(ifX, ifY);
                __m128 l2 = _mm_mul_ps(fX, ifY);
                __m128 l3 = _mm_mul_ps(ifX, fY);
                __m128 l4 = _mm_mul_ps(fX, fY);
                
                
                __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAr, l1), _mm_mul_ps(TexelBr, l2)), _mm_mul_ps(TexelCr, l3)), _mm_mul_ps(TexelDr, l4));
                __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAg, l1), _mm_mul_ps(TexelBg, l2)), _mm_mul_ps(TexelCg, l3)), _mm_mul_ps(TexelDg, l4));
                __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAb, l1), _mm_mul_ps(TexelBb, l2)), _mm_mul_ps(TexelCb, l3)), _mm_mul_ps(TexelDb, l4));
                __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_add_ps(_mm_mul_ps(TexelAa, l1), _mm_mul_ps(TexelBa, l2)), _mm_mul_ps(TexelCa, l3)), _mm_mul_ps(TexelDa, l4));
                
                Texelr = _mm_mul_ps(Texelr, ColorR);
                Texelg = _mm_mul_ps(Texelg, ColorG);
                Texelb = _mm_mul_ps(Texelb, ColorB);
                Texela = _mm_mul_ps(Texela, ColorA);
                
                Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), MaxColorValue);
                Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), MaxColorValue);
                Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), MaxColorValue);
                
                
                
                Destr = mmSquare(Destr);
                Destg = mmSquare(Destg);
                Destb = mmSquare(Destb);
                
                __m128 InvTexelA = _mm_sub_ps(One, _mm_mul_ps(Inv255C_x4, Texela));
                __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                
                Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
                Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
                Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
                
                __m128i IntR = _mm_cvtps_epi32(Blendedr);
                __m128i IntG = _mm_cvtps_epi32(Blendedg);
                __m128i IntB = _mm_cvtps_epi32(Blendedb);
                __m128i IntA = _mm_cvtps_epi32(Blendeda);
                
                __m128i Sr = _mm_slli_epi32(IntR, 16);
                __m128i Sg = _mm_slli_epi32(IntG, 8);
                __m128i Sb = IntB;
                __m128i Sa = _mm_slli_epi32(IntR, 24);
                
                __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
#if 1
                Out = _mm_or_si128(_mm_and_si128(WriteMask, Out), _mm_andnot_si128(WriteMask, OriginalDest));
#else
                Out = _mm_and_si128(WriteMask, Out);
#endif
                _mm_store_si128((__m128i*)Pixel, Out);
                
                tempPx = _mm_add_ps(tempPx, Four_x4);
                Pixel += 4;
                if ((X + 8) < FillRect.MaxX) {
                    ClipMask = _mm_set1_epi8(-1);
                }
                else {
                    ClipMask = EndClipMask;
                }
            }
            Row += RowAdvance;
        }
    }
}
