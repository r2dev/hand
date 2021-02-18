enum finalize_asset_operation {
    FinalizeAsset_None,
    FinalizeAsset_Font,
    FinalizeAsset_Bitmap,
};

struct load_asset_work {
    task_with_memory* Task;
    asset* Asset;
    platform_file_handle *Handle;
    
    u64 Size;
    u64 Offset;
    void* Destination;
    u32 FinalState;
    finalize_asset_operation FinalizeOperation;
};

internal void
LoadAssetWorkDirectly(load_asset_work* Work) {
    TIMED_FUNCTION();
    Platform.ReadDataFromFile(Work->Handle, Work->Offset, Work->Size, Work->Destination);
    if (PlatformNoFileErrors(Work->Handle)) {
        switch(Work->FinalizeOperation) {
            case FinalizeAsset_None: {
            } break;
            case FinalizeAsset_Font: {
                loaded_font *Font = &Work->Asset->Header->Font;
                hha_font *Info = &Work->Asset->HHA.Font;
                for(u32 GlyphIndex = 1; GlyphIndex < Info->GlyphCount; ++GlyphIndex) {
                    hha_font_glyph *Glyph = Font->Glyphs + GlyphIndex;
                    Assert(Glyph->UnicodeCodePoint < Info->OnePastHighestCodePoint);
                    Assert((u32)(u16)GlyphIndex == GlyphIndex);
                    Font->UnicodeMap[Glyph->UnicodeCodePoint] = (u16)GlyphIndex;
                }
            } break;
            case FinalizeAsset_Bitmap: {
                loaded_bitmap *Bitmap = &Work->Asset->Header->Bitmap;
                Bitmap->TextureHandle = Platform.AllocateTexture(Bitmap->Width, Bitmap->Height, Bitmap->Memory);
            } break;
        }
    }
    CompletePreviousWritesBeforeFutureWrites;
    if (!PlatformNoFileErrors(Work->Handle)) {
        ZeroSize(Work->Size, Work->Destination);
    }
    Work->Asset->State = Work->FinalState;
}

internal PLATFORM_WORK_QUEUE_CALLBACK(LoadAssetWork) {
    load_asset_work* Work = (load_asset_work*)Data;
    LoadAssetWorkDirectly(Work);
    EndTaskWithMemory(Work->Task);
}

inline asset_file*
GetFile(game_assets* Assets, u32 FileIndex) {
    Assert(FileIndex < Assets->FileCount);
    asset_file*Result = Assets->Files + FileIndex;
    return(Result);
}

inline platform_file_handle*
GetFileHandleFor(game_assets* Assets, u32 FileIndex) {
    platform_file_handle *Result = &GetFile(Assets, FileIndex)->Handle;
    return(Result);
}

internal asset_memory_block*
InsertBlock(asset_memory_block* Prev, u64 Size, void* Memory) {
    
    Assert(Size > sizeof(asset_memory_block));
    asset_memory_block * MemoryBlock = (asset_memory_block*)Memory;
    
    MemoryBlock->Size = Size - sizeof(asset_memory_block);
    MemoryBlock->Flags = 0;
    
    MemoryBlock->Prev = Prev;
    MemoryBlock->Next = Prev->Next;
    
    MemoryBlock->Prev->Next = MemoryBlock;
    MemoryBlock->Next->Prev = MemoryBlock;
    
    return(MemoryBlock);
}


internal asset_memory_block *
FindBlockForSize(game_assets *Assets, memory_index Size) {
    asset_memory_block *Result = 0;
    for(asset_memory_block *Block = Assets->MemorySentinel.Next;
        Block != &Assets->MemorySentinel;
        Block = Block->Next) {
        if(!(Block->Flags & AssetMemory_Used)) {
            if (Block->Size >= Size) {
                Result = Block;
                break;
            }
        }
    }
    
    return(Result);
}

struct asset_memory_size {
    u32 Total;
    u32 Data;
    u32 Section;
};

internal void
MoveHeaderToFront(game_assets* Assets, asset* Asset) {
    asset_memory_header *Header = Asset->Header;
    RemoveAssetHeaderFromList(Header);
    InsertAssetHeaderAtFront(Assets, Header);
}

inline void
AddAssetHeaderToList(game_assets* Assets, u32 AssetIndex, asset_memory_size Size) {
    asset_memory_header *Header = Assets->Assets[AssetIndex].Header;
    Assert(AssetIndex != 0);
    Header->AssetIndex = AssetIndex;
    Header->TotalSize = Size.Total;
    InsertAssetHeaderAtFront(Assets, Header);
}


inline b32
MergeIfPossible(game_assets* Assets, asset_memory_block *First, asset_memory_block *Second) {
    b32 Result = false;
    if (First != &Assets->MemorySentinel &&
        Second != &Assets->MemorySentinel) {
        if (!(First->Flags & AssetMemory_Used) &&
            !(Second->Flags & AssetMemory_Used)) {
            u8* ExpecdSecond = (u8*)First + sizeof(asset_memory_block) + First->Size;
            if ((u8*)Second == ExpecdSecond) {
                Second->Next->Prev = Second->Prev;
                Second->Prev->Next = Second->Next;
                First->Size += sizeof(asset_memory_block) + Second->Size;
                Result = true;
            }
        }
    }
    
    return(Result);
}

internal u32
GenerationHasCompleted(game_assets* Assets, u32 CheckID) {
    b32 Result = true;
    for (u32 Index = 0; Index < Assets->InFlightGenerationCount; Index++) {
        if (Assets->InFlightGenerations[Index] == CheckID) {
            Result = false;
            break;
        }
    }
    return(Result);
}

internal asset_memory_header*
AcquireAssetMemory(game_assets *Assets, u32 Size, u32 AssetIndex, asset_header_type AssetHeaderType) {
    TIMED_FUNCTION();
    asset_memory_header* Result = 0;
    BeginAssetLock(Assets);
    asset_memory_block *Block = FindBlockForSize(Assets, Size);
    for(;;) {
        if (Block && Size <= Block->Size) {
            Assert((Block->Size) >= Size);
            
            Block->Flags |= AssetMemory_Used;
            Result = (asset_memory_header*)(Block + 1);
            memory_index RemainingSize = Block->Size - Size;
            memory_index BlockSplitThrehold = 4096;
            
            if (RemainingSize > BlockSplitThrehold) {
                Block->Size -= RemainingSize;
                InsertBlock(Block, RemainingSize, (u8*)Result + Block->Size);
            } else {
                
            }
            break;
        } else {
            for (asset_memory_header *Header = Assets->LoadedAssetSentinel.Prev;
                 Header != &Assets->LoadedAssetSentinel;
                 Header = Header->Prev
                 ) {
                asset *Asset = Assets->Assets + Header->AssetIndex;
                if (Asset->State >= AssetState_Loaded &&
                    (GenerationHasCompleted(Assets, Asset->Header->GenerationID))
                    ) {
                    Assert(Asset->State == AssetState_Loaded);
                    
                    RemoveAssetHeaderFromList(Header);
                    if (Asset->Header->AssetHeaderType == AssetType_Bitmap) {
                        Platform.DeallocateTexture(Asset->Header->Bitmap.TextureHandle);
                    }
                    
                    Block = (asset_memory_block* )Asset->Header - 1;
                    Block->Flags &= ~AssetMemory_Used;
                    
                    if(MergeIfPossible(Assets, Block->Prev, Block)) {
                        Block = Block->Prev;
                    }
                    MergeIfPossible(Assets, Block, Block->Next);
                    
                    Asset->State = AssetState_Unloaded;
                    Asset->Header = 0;
                    
                    break;
                } 
            }
        }
    }
    if (Result) {
        Result->AssetIndex = AssetIndex;
        Result->TotalSize = Size;
        Result->AssetHeaderType = AssetHeaderType;
        InsertAssetHeaderAtFront(Assets, Result);
    }
    EndAssetLock(Assets);
    return(Result);
}

internal void
LoadBitmap(game_assets* Assets, bitmap_id ID, b32 Immediate) {
    TIMED_FUNCTION();
    asset *Asset = Assets->Assets + ID.Value;
    if (ID.Value) {
        if (AtomicCompareExchangeUInt32((u32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded) {
            task_with_memory* Task = 0;
            if (!Immediate) {
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }
            if (Immediate || Task) {
                
                hha_bitmap* Info = &Asset->HHA.Bitmap;
                
                asset_memory_size Size = {};
                // todo remove asset_meory_size
                Size.Section = Info->Dim[0] * BITMAP_BYTE_PER_PIXEL;
                Size.Data = Size.Section * Info->Dim[1];
                Size.Total = Size.Data + sizeof(asset_memory_header);
                
                Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Bitmap);
                loaded_bitmap* Bitmap = &Asset->Header->Bitmap;
                
                Bitmap->Width = SafeTruncateToU16(Info->Dim[0]);
                Bitmap->Height = SafeTruncateToU16(Info->Dim[1]);
                
                Bitmap->WidthOverHeight = SafeRatio1((r32)Bitmap->Width, (r32)Bitmap->Height);
                Bitmap->AlignPercentage = v2{Info->AlignPercentage[0], Info->AlignPercentage[1]};
                Bitmap->TextureHandle = 0;
                
                Bitmap->Pitch = SafeTruncateToU16(Size.Section);
                Bitmap->Memory = (Asset->Header + 1);
                
                load_asset_work Work;
                Work.Asset = Asset;
                
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Size = Size.Data;
                Work.Offset = Asset->HHA.DataOffset;
                Work.Task = Task;
                Work.FinalizeOperation = FinalizeAsset_Bitmap;
                
                Work.Destination = Bitmap->Memory;
                Work.FinalState = (AssetState_Loaded);
                if (Task) {
                    load_asset_work* TaskWork = PushStruct(&Task->Arena, load_asset_work, NoClear());
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                } else {
                    LoadAssetWorkDirectly(&Work);
                }
                
                //Copy(Work->Size, Assets->HHAContent + Asset->DataOffset, Work->Destination);
            }
            else {
                Asset->State = AssetState_Unloaded;
            }
        } else if (Immediate) {
            asset_state volatile* State = (asset_state volatile*)&Asset->State;
            while(Asset->State == AssetState_Queued) {}
        }
    }
}


internal void
LoadFont(game_assets* Assets, font_id ID, b32 Immediate) {
    
    asset *Asset = Assets->Assets + ID.Value;
    if (ID.Value) {
        if (AtomicCompareExchangeUInt32((u32 *)&Asset->State, AssetState_Queued, AssetState_Unloaded) == AssetState_Unloaded) {
            task_with_memory* Task = 0;
            if (!Immediate) {
                Task = BeginTaskWithMemory(Assets->TranState, false);
            }
            if (Immediate || Task) {
                
                hha_font* Info = &Asset->HHA.Font;
                
                u32 GlyphSize = Info->GlyphCount * sizeof(hha_font_glyph);
                u32 HorizontalAdvanceSize = Info->GlyphCount * Info->GlyphCount * sizeof(r32);
                u32 SizeData = HorizontalAdvanceSize + GlyphSize;
                
                u32 UnicodeMapSize = sizeof(u16) * Info->OnePastHighestCodePoint;
                u32 SizeTotal = SizeData + sizeof(asset_memory_header) + UnicodeMapSize;
                
                Asset->Header = AcquireAssetMemory(Assets, SizeTotal, ID.Value, AssetType_Font);
                
                loaded_font* Font = &Asset->Header->Font;
                Font->BitmapIDOffset = GetFile(Assets, Asset->FileIndex)->FontBitmapIDOffset;
                Font->Glyphs = (hha_font_glyph*)(Asset->Header + 1);
                Font->HorizontalAdvance = (r32*)((u8*)Font->Glyphs + GlyphSize);
                Font->UnicodeMap = (u16*)((u8*)Font->HorizontalAdvance + HorizontalAdvanceSize);
                ZeroSize(HorizontalAdvanceSize, Font->UnicodeMap);
                
                load_asset_work Work;
                Work.Asset = Asset;
                Work.Handle = GetFileHandleFor(Assets, Asset->FileIndex);
                Work.Size = SizeData;
                Work.Offset = Asset->HHA.DataOffset;
                Work.Task = Task;
                Work.FinalizeOperation = FinalizeAsset_Font;
                
                Work.Destination = Font->Glyphs;
                Work.FinalState = (AssetState_Loaded);
                if (Task) {
                    load_asset_work* TaskWork = PushStruct(&Task->Arena, load_asset_work, NoClear());
                    *TaskWork = Work;
                    Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, TaskWork);
                } else {
                    LoadAssetWorkDirectly(&Work);
                }
                
                //Copy(Work->Size, Assets->HHAContent + Asset->DataOffset, Work->Destination);
            }
            else {
                Asset->State = AssetState_Unloaded;
            }
        } else if (Immediate) {
            asset_state volatile* State = (asset_state volatile*)&Asset->State;
            while(Asset->State == AssetState_Queued) {}
        }
    }
}

internal void
LoadSound(game_assets* Assets, sound_id ID) {
    TIMED_FUNCTION();
    asset *Asset = Assets->Assets + ID.Value;
    if (ID.Value &&
        _InterlockedCompareExchange((long volatile*)&Asset->State, AssetState_Unloaded, AssetState_Queued) == AssetState_Unloaded) {
        task_with_memory* Task = BeginTaskWithMemory(Assets->TranState, false);
        if (Task) {
            
            hha_sound* Info = &Asset->HHA.Sound;
            asset_memory_size Size = {};
            Size.Section = Info->SampleCount * sizeof(s16);
            Size.Data = Size.Section * Info->ChannelCount;
            Size.Total = Size.Data + sizeof(asset_memory_header);
            
            Asset->Header = AcquireAssetMemory(Assets, Size.Total, ID.Value, AssetType_Sound);
            
            loaded_sound* Sound = &Asset->Header->Sound;
            Sound->SampleCount = Info->SampleCount;
            Sound->ChannelCount = Info->ChannelCount;
            // 
            u32 ChannelSize = Size.Section;
            
            void* Memory = Asset->Header + 1;
            s16 *SoundAt = (s16*)Memory;
            
            for (u32 ChannelIndex = 0; ChannelIndex < Sound->ChannelCount; ++ChannelIndex) {
                Sound->Samples[ChannelIndex] = SoundAt;
                SoundAt += ChannelSize;
            }
            
            load_asset_work* Work = PushStruct(&Task->Arena, load_asset_work);
            
            Work->Asset = Asset;
            Work->Handle = GetFileHandleFor(Assets, Asset->FileIndex);
            Work->Offset = Asset->HHA.DataOffset;
            Work->Size = Size.Data;
            Work->Task = Task;
            Work->FinalizeOperation = FinalizeAsset_None;
            Work->Destination = Asset->Header + 1;
            
            //Work->Asset->Sound = Sound;
            //Copy(Work->Size, Assets->HHAContent + Asset->DataOffset, Work->Destination);
            Work->FinalState = AssetState_Loaded;
            
            AddAssetHeaderToList(Assets, ID.Value, Size);
            
            Platform.AddEntry(Assets->TranState->LowPriorityQueue, LoadAssetWork, Work);
        }
        else {
            Asset->State = AssetState_Unloaded;
        }
    }
}

inline void
PrefetchSound(game_assets* Assets, sound_id ID) { LoadSound(Assets, ID); }

inline void
PrefetchBitmap(game_assets* Assets, bitmap_id ID) { LoadBitmap(Assets, ID, false); }

hha_sound* GetSoundInfo(game_assets* Assets, sound_id ID) {
    Assert(ID.Value <= Assets->AssetCount);
    hha_sound* Result = &Assets->Assets[ID.Value].HHA.Sound;
    return(Result);
}

hha_bitmap* GetBitmapInfo(game_assets* Assets, bitmap_id ID) {
    Assert(ID.Value <= Assets->AssetCount);
    hha_bitmap* Result = &Assets->Assets[ID.Value].HHA.Bitmap;
    return(Result);
}

hha_font* GetFontInfo(game_assets* Assets, font_id ID) {
    Assert(ID.Value <= Assets->AssetCount);
    hha_font* Result = &Assets->Assets[ID.Value].HHA.Font;
    return(Result);
}

inline sound_id
GetNextSoundInChain(game_assets* Assets, sound_id ID) {
    sound_id Result = {};
    hha_sound* Info = GetSoundInfo(Assets, ID);
    switch(Info->Chain) {
        case HHASoundChain_none: {
        } break;
        case HHASoundChain_loop: {
            Result = ID;
        } break;
        case HHASoundChain_Advance: {
            Result.Value = ID.Value + 1;
        } break;
        
        default: {
            InvalidCodePath;
        }
        
    }
    return(Result);
}

internal u32
BestMatchAsset(game_assets* Assets, asset_type_id TypeID, asset_vector* MatchVector, asset_vector* WeightVector) {
    TIMED_FUNCTION();
    u32 Result = 0;
    asset_type* Type = Assets->AssetTypes + TypeID;
    r32 BestDiff = Real32Maximum;
    for (u32 AssetIndex = Type->FirstAssetIndex; AssetIndex < Type->OnePassLastAssetIndex; ++AssetIndex) {
        asset* Asset = Assets->Assets + AssetIndex;
        r32 TotalWeight = 0.0f;
        for (u32 TagIndex = Asset->HHA.FirstTagIndex; TagIndex < Asset->HHA.OnePassLastTagIndex; ++TagIndex) {
            hha_tag* Tag = Assets->Tags + TagIndex;
            r32 A = MatchVector->E[Tag->ID];
            r32 B = Tag->Value;
            r32 D0 = AbsoluteValue(A - B);
            r32 D1 = AbsoluteValue(A - Assets->TagRange[Tag->ID] * SignOf(A) - B);
            TotalWeight += Minimum(D0, D1) * WeightVector->E[Tag->ID];
        }
        
        if (BestDiff > TotalWeight) {
            BestDiff = TotalWeight;
            Result = AssetIndex;
        }
    }
    return(Result);
}

internal bitmap_id
GetBestMatchBitmapFrom(game_assets* Assets, asset_type_id ID, asset_vector* MatchVector, asset_vector* WeightVector) {
    bitmap_id Result = { BestMatchAsset(Assets, ID, MatchVector, WeightVector) };
    return(Result);
}

internal sound_id
GetBestMatchSoundFrom(game_assets* Assets, asset_type_id ID, asset_vector* MatchVector, asset_vector* WeightVector) {
    sound_id Result = { BestMatchAsset(Assets, ID, MatchVector, WeightVector) };
    return(Result);
}

internal font_id
GetBestMatchFontFrom(game_assets* Assets, asset_type_id ID, asset_vector *Match, asset_vector *Weight) {
    font_id FontID = { BestMatchAsset(Assets, ID, Match, Weight) };
    return(FontID);
}

internal game_assets*
AllocateGameAssets(memory_arena* Arena, memory_index Size, transient_state* TranState) {
    TIMED_FUNCTION();
    game_assets* Assets = PushStruct(Arena, game_assets);
    
    Assets->NextGenerationID = 0;
    Assets->InFlightGenerationCount = 0;
    
    Assets->MemorySentinel.Next = &Assets->MemorySentinel;
    Assets->MemorySentinel.Prev = &Assets->MemorySentinel;
    Assets->MemorySentinel.Flags = 0;
    Assets->MemorySentinel.Size = 0;
    
    InsertBlock(&Assets->MemorySentinel, Size, PushSize(Arena, Size, NoClear()));
    
    Assets->TranState = TranState;
    
    for (u32 Tag = 0; Tag < Tag_Count; ++Tag) {
        Assets->TagRange[Tag] = 100000.0f;
    }
    Assets->TagRange[Tag_FaceDirection] = Tau32;
    Assets->TagCount = 1;
    Assets->AssetCount = 1;
    
    Assets->LoadedAssetSentinel.Prev = &Assets->LoadedAssetSentinel;
    Assets->LoadedAssetSentinel.Next = &Assets->LoadedAssetSentinel;
    
    platform_file_group FileGroup = Platform.GetAllFileOfTypeBegin(PlatformFileType_AssetFile);
    Assets->FileCount = FileGroup.FileCount;
    Assets->Files = PushArray(Arena, Assets->FileCount, asset_file);
    for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
        asset_file* File = Assets->Files + FileIndex;
        
        File->FontBitmapIDOffset = 0;
        File->TagBase = Assets->TagCount;
        File->Handle = Platform.OpenNextFile(&FileGroup);
        // TODO(NAME): maybe remove 
        ZeroStruct(File->Header);
        
        Platform.ReadDataFromFile(&File->Handle, 0, sizeof(File->Header), &File->Header);
        u32 AssetTypeArraySize = File->Header.AssetTypeCount * sizeof(hha_asset_type);
        File->AssetTypeArray = (hha_asset_type*)PushSize(Arena, AssetTypeArraySize);
        Platform.ReadDataFromFile(&File->Handle, File->Header.AssetTypes, AssetTypeArraySize, File->AssetTypeArray);
        if (File->Header.MagicValue != HHA_MAGIC_VALUE) {
            Platform.FileError(&File->Handle, "invalid magic value");
        }
        if (File->Header.Version > HHA_VERSION) {
            Platform.FileError(&File->Handle, "for a late version");
        }
        if (PlatformNoFileErrors(&File->Handle)) {
            Assets->TagCount += File->Header.TagCount - 1;
            Assets->AssetCount += File->Header.AssetCount - 1;
        } else {
            InvalidCodePath;
        }
    }
    Platform.GetAllFileOfTypeEnd(&FileGroup);
    
    
    Assets->Assets = PushArray(Arena, Assets->AssetCount, asset);
    Assets->Tags = PushArray(Arena, Assets->TagCount, hha_tag);
    
    ZeroStruct(Assets->Tags[0]);
    
    for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
        asset_file* File = Assets->Files + FileIndex;
        if (PlatformNoFileErrors(&File->Handle)) {
            u32 TagArraySize = sizeof(hha_tag) * (File->Header.TagCount - 1);
            Platform.ReadDataFromFile(&File->Handle, File->Header.Tags + sizeof(hha_tag), TagArraySize, Assets->Tags + File->TagBase);
        }
    }
    
    u32 AssetCount = 0;
    ZeroStruct(*(Assets->Assets + AssetCount));
    
    ++AssetCount;
    
    
    for (u32 DestTypeId = 0; DestTypeId < Asset_Count; ++DestTypeId) {
        asset_type* DestType = Assets->AssetTypes + DestTypeId;
        DestType->FirstAssetIndex = AssetCount;
        for (u32 FileIndex = 0; FileIndex < Assets->FileCount; ++FileIndex) {
            asset_file *File = Assets->Files + FileIndex;
            if (PlatformNoFileErrors(&File->Handle)) {
                for (u32 SourceIndex = 0; SourceIndex < File->Header.AssetTypeCount; ++SourceIndex) {
                    hha_asset_type *SourceType = File->AssetTypeArray + SourceIndex;
                    if (SourceType->TypeID == DestTypeId) {
                        if (SourceType->TypeID == Asset_FontGlyph) {
                            File->FontBitmapIDOffset = AssetCount - SourceType->FirstAssetIndex;
                        }
                        Assert(SourceType->OnePassLastAssetIndex >= SourceType->FirstAssetIndex);
                        u32 AssetCountForType = SourceType->OnePassLastAssetIndex - SourceType->FirstAssetIndex;
                        
                        temporary_memory TempMem = BeginTemporaryMemory(&TranState->TranArena);
                        hha_asset *HHAAssetArray = PushArray(&TranState->TranArena, AssetCountForType, hha_asset);
                        
                        Platform.ReadDataFromFile(&File->Handle, 
                                                  File->Header.Assets + SourceType->FirstAssetIndex * sizeof(hha_asset),
                                                  AssetCountForType * sizeof(hha_asset),
                                                  HHAAssetArray);
                        for (u32 AssetIndex = 0; AssetIndex < AssetCountForType; ++AssetIndex) {
                            Assert(AssetCount < Assets->AssetCount);
                            hha_asset* Source = HHAAssetArray + AssetIndex;
                            asset* Asset = Assets->Assets + AssetCount++;
                            Asset->FileIndex = FileIndex;
                            Asset->HHA = *Source;
                            if (Asset->HHA.FirstTagIndex == 0) {
                                Asset->HHA.FirstTagIndex = Asset->HHA.OnePassLastTagIndex = 0;
                            } else {
                                Asset->HHA.FirstTagIndex += (File->TagBase - 1);
                                Asset->HHA.OnePassLastTagIndex += (File->TagBase - 1);
                            }
                        }
                        EndTemporaryMemory(TempMem);
                    }
                }
            }
        }
        DestType->OnePassLastAssetIndex = AssetCount;
    }
    Assert(AssetCount == Assets->AssetCount);
    
    return(Assets);
}

internal u32
GetFirstAssetFrom(game_assets *Assets, asset_type_id ID) {
    u32 Result = 0;
    asset_type* Type = Assets->AssetTypes + ID;
    if (Type->FirstAssetIndex < Type->OnePassLastAssetIndex) {
        Result = Type->FirstAssetIndex;
    }
    return(Result);
}

internal bitmap_id
GetFirstBitmapFrom(game_assets* Asset, asset_type_id ID) {
    bitmap_id Result = { GetFirstAssetFrom(Asset, ID) };
    return(Result);
}

internal sound_id
GetFirstSoundFrom(game_assets* Asset, asset_type_id ID) {
    sound_id Result = { GetFirstAssetFrom(Asset, ID) };
    return(Result);
}

internal u32
RandomAssetFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {
    u32 Result = 0;
    asset_type* Type = Assets->AssetTypes + ID;
    if (Type->FirstAssetIndex != Type->OnePassLastAssetIndex) {
        u32 Count = Type->OnePassLastAssetIndex - Type->FirstAssetIndex;
        Result = Type->FirstAssetIndex + RandomChoice(Series, Count);
    }
    return(Result);
}

internal bitmap_id
GetRandomBitmapFrom(game_assets* Assets, asset_type_id ID, random_series* Series) {
    bitmap_id Result = {RandomAssetFrom(Assets, ID, Series)};
    return(Result);
}

inline u32
GetGlyphFromCodePoint(hha_font* FontInfo, loaded_font* Font, u32 CodePoint) {
    u32 Result = 0;
    if (CodePoint + 1 < FontInfo->OnePastHighestCodePoint) {
        Result = Font->UnicodeMap[CodePoint];
        Assert(Result < FontInfo->GlyphCount);
    }
    return(Result);
}

internal r32 
GetHorizontalAdvanceForPair(hha_font* FontInfo, loaded_font* Font, u32 DesiredPrevCodePoint, u32 DesiredCodePoint) {
    u32 Glyph0 = GetGlyphFromCodePoint(FontInfo, Font, DesiredPrevCodePoint);
    u32 Glyph1 = GetGlyphFromCodePoint(FontInfo, Font, DesiredCodePoint);
    
    r32 Result = Font->HorizontalAdvance[Glyph0 * FontInfo->GlyphCount + Glyph1];
    return(Result);
}

internal bitmap_id
GetBitmapForGlyph(game_assets *Assets, hha_font* FontInfo, loaded_font* Font, u32 DesiredCodePoint) {
    u32 Glyph = GetGlyphFromCodePoint(FontInfo, Font, DesiredCodePoint);
    bitmap_id Result = {};
    Result.Value = Font->Glyphs[Glyph].BitmapID.Value + Font->BitmapIDOffset;
    return(Result);
}

internal r32
GetLineAdvancedFor(hha_font* FontInfo) {
    r32 Result = FontInfo->Ascent + FontInfo->Descent + FontInfo->ExternalLeading;
    return(Result);
}

internal r32
GetStartingBaselineY(hha_font* FontInfo) {
    r32 Result = FontInfo->Ascent;
    return(Result);
}

