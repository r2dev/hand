#include <stdio.h>

global_variable r32 AtY;
global_variable r32 LeftEdge;
global_variable r32 FontScale;
global_variable font_id FontID;
global_variable r32 GlobalWidth;
global_variable r32 GlobalHeight;

internal void
DEBUGReset(game_assets *Assets, u32 Width, u32 Height) {
    TIMED_FUNCTION();
    
    GlobalWidth = (r32)Width;
    GlobalHeight = (r32)Height;
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_FontType] = 1.0f;
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);
    
    FontScale = 1.0f;
    Orthographic(DEBUGRenderGroup, Width, Height, 1.0f);
    LeftEdge = -0.5f * (r32)Width;
    
    hha_font *Info = GetFontInfo(Assets, FontID);
    AtY = 0.5f * Height - FontScale * GetStartingBaselineY(Info);
    // test start
    //PushRect(DEBUGRenderGroup, v3{0, AtY, 0}, v2{1000.0f, 1.0f}, v4{1.0f, 0.5f, 1.0f, 0});
}

inline b32
IsHex(char Char) {
    b32 Result = (((Char >= '0') && (Char <= '9')) || ((Char >= 'A')&& (Char <= 'F')));
    return(Result);
}

inline u32
GetHex(char Char) {
    u32 Result = 0;
    if (Char >= '0' && Char <= '9') {
        Result = Char - '0';
    } else if (Char >= 'A' && Char <= 'F') {
        Result = 0xA + (Char - 'A');
    }
    return(Result);
}

struct debug_statistic {
    u32 Count;
    r64 Min;
    r64 Max;
    r64 Avg;
};

inline void
BeginDebugStatistic(debug_statistic* Stat) {
    Stat->Min = Real32Maximum;
    Stat->Max = -Real32Maximum;
    Stat->Avg = 0;
    Stat->Count = 0;
}

inline void
UpdateDebugStatistic(debug_statistic* Stat, r64 Value) {
    Stat->Count++;
    if (Value < Stat->Min) {
        Stat->Min = Value;
    }
    if (Value > Stat->Max) {
        Stat->Max = Value;
    }
    Stat->Avg += Value;
    
}

inline void
EndDebugStatistic(debug_statistic* Stat) {
    if (Stat->Count != 0) {
        Stat->Avg /= (r64)Stat->Count;
        
    } else {
        Stat->Min = 0;
        Stat->Max = 0;
    }
}


internal void
DEBUGTextLine(char *String) {
    if (DEBUGRenderGroup) {
        render_group* RenderGroup = DEBUGRenderGroup;
        
        loaded_font* Font = PushFont(RenderGroup, FontID);
        if (Font) {
            hha_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
            u32 PrevCodePoint = 0;
            
            r32 AtX = LeftEdge;
            r32 CharScale = FontScale;
            
            for(char* At = String;
                *At;
                ++At) {
                
                u32 CodePoint = *At;
                r32 AdvancedX = 0;
                
                if (At[0] == '\\' && IsHex(At[1]) && IsHex(At[2]) && IsHex(At[3]) && IsHex(At[4])) {
                    CodePoint = ((GetHex(At[1]) << 12) | (GetHex(At[2]) << 8) | (GetHex(At[3]) << 4) | (GetHex(At[4]) << 0));
                    At += 4;
                }
                if(PrevCodePoint) {
                    AdvancedX = CharScale * GetHorizontalAdvanceForPair(Info, Font, PrevCodePoint, CodePoint);
                }
                AtX += AdvancedX;
                
                if (CodePoint != ' ') {
                    // proportional mode
                    // AdvancedX = CharScale * (r32)(Info->Dim[0] + 2);
                    bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, Info, Font, CodePoint);
                    hha_bitmap* BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);
                    PushBitmap(RenderGroup, BitmapID, CharScale * (r32)BitmapInfo->Dim[1], v3{AtX, AtY, 0}, v4{1.0f, 1.0f, 1.0f, 1});
                }
                // advance here
                
                PrevCodePoint = CodePoint;
            }
            PushRect(RenderGroup, v3{LeftEdge, AtY, 0}, v2{1000.0f, 1.0f}, v4{1.0f, 0.0f, 0.0f, 1});
            PushRect(RenderGroup, v3{LeftEdge, AtY + Info->Ascent, 0}, v2{1000.0f, 1.0f}, v4{0.0f, 1.0f, 0.0f, 1});
            PushRect(RenderGroup, v3{LeftEdge, AtY - Info->Descent, 0}, v2{1000.0f, 1.0f}, v4{0.0f, 0.0f, 1.0f, 1});
            AtY -= GetLineAdvancedFor(Info) * CharScale;
        } else {
            
        }
    }
}


internal void
DEBUGOverlay(game_memory* Memory, game_input* Input) {
    debug_state *DebugState = (debug_state* )Memory->DebugStorage;
    
    if (DebugState && DEBUGRenderGroup) {
        render_group* RenderGroup = DEBUGRenderGroup;
        // mouse Position
        v2 MouseP = V2(Input->MouseX, Input->MouseY);
        
        if (WasPress(&Input->MouseBottons[PlatformMouseButton_Right])) {
            DebugState->Paused = !DebugState->Paused;
        }
        
        loaded_font* Font = PushFont(RenderGroup, FontID);
        if (Font) {
            hha_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
            
            
#if 0            
            for (u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex) {
                debug_counter_state* Counter = DebugState->CounterStates + CounterIndex;
                
                debug_statistic HitCount, CycleCount, CycleOverHit;
                BeginDebugStatistic(&HitCount);
                BeginDebugStatistic(&CycleCount);
                BeginDebugStatistic(&CycleOverHit);
                
                
                for (u32 SnapIndex = 0; SnapIndex < DEBUG_SNAPSHOT_COUNT; ++SnapIndex) {
                    UpdateDebugStatistic(&HitCount, Counter->Snapshots[SnapIndex].HitCount);
                    UpdateDebugStatistic(&CycleCount, (u32)Counter->Snapshots[SnapIndex].CycleCount);
                    
                    r64 HOC = 0.0f;
                    if (Counter->Snapshots[SnapIndex].HitCount) {
                        HOC = (r64)Counter->Snapshots[SnapIndex].CycleCount / (r64)Counter->Snapshots[SnapIndex].HitCount;
                    }
                    UpdateDebugStatistic(&CycleOverHit, HOC);
                }
                
                EndDebugStatistic(&HitCount);
                EndDebugStatistic(&CycleCount);
                EndDebugStatistic(&CycleOverHit);
                
                if (Counter->BlockName) {
                    if (CycleCount.Max > 0.0f) {
                        r32 ChartLeft = 100.0f;
                        r32 ChartMinY = AtY;
                        r32 BarWidth = 4.0f;
                        r32 ChartHeight = Info->Ascent * FontScale;
                        r32 Scale = 1.0f / (r32)CycleCount.Max;
                        for (u32 SnapIndex = 0; SnapIndex < DEBUG_SNAPSHOT_COUNT; ++SnapIndex) {
                            r32 ThisProportion = Scale * (r32)Counter->Snapshots[SnapIndex].CycleCount;
                            r32 ThisHeight = ChartHeight * ThisProportion;
                            PushRect(DEBUGRenderGroup, v3{ChartLeft + (r32)SnapIndex * BarWidth + 0.5f * BarWidth, ChartMinY + 0.5f * ThisHeight, 0}, v2{BarWidth, ThisHeight}, v4{ThisProportion, 1, 0.0f, 1});
                        }
                    }
                    
                    char TextBuffer[256];
                    _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                                "%32s(%4d): %10ucy %8uh %10ucy/h",
                                Counter->BlockName,
                                Counter->LineNumber,
                                (u32)CycleCount.Avg,
                                (u32)HitCount.Avg,
                                (u32)CycleOverHit.Avg);
                    DEBUGTextLine(TextBuffer);
                }
                
            }
#endif
            if (DebugState->FrameCount) {
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                            "last frame time %.02fms, MonseX - %f, MouseY - %f",
                            DebugState->Frames[DebugState->FrameCount - 1].WallSecondsElapsed * 1000.0f,
                            MouseP.x, MouseP.y);
                DEBUGTextLine(TextBuffer);
            }
#if 1
            r32 LaneHeight = 20.0f;
            u32 LaneCount = DebugState->FrameBarLaneCount;
            r32 BarHeight = LaneHeight * LaneCount;
            r32 BarSpacing = BarHeight + 4.0f;;
            
            r32 ChartHeight = BarSpacing * DebugState->FrameCount;
            r32 ChartWidth = 300.0f;
            r32 ChartLeft = LeftEdge + 10.0f;
            r32 ChartTop = 0.5f * GlobalHeight - 10.0f;
            
            r32 Scale = ChartWidth * DebugState->FrameBarScale;
            v3 Colors[] = {
                {1, 0, 0},
                {0, 1, 0},
                {0, 0, 1},
                {1, 1, 0},
                {0, 1, 1},
                {1, 0, 1},
                {1, 0.5f, 0},
                {0, 1, 0.5f},
                {1, 0, 0.5f},
                {0.5f, 1, 0},
                {0, 0.5f, 1},
                {0.5f, 0, 1},
            };
            
            for (u32 FrameIndex = 0; FrameIndex < DebugState->FrameCount; ++FrameIndex) {
                debug_frame * Frame = DebugState->Frames + FrameIndex;
                
                r32 StackX = ChartLeft;
                r32 StackY = ChartTop - (r32)FrameIndex * BarSpacing;
                
                for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex) {
                    debug_frame_region* Region = Frame->Regions + RegionIndex;
                    
                    v3 Color = Colors[RegionIndex % ArrayCount(Colors)];
                    r32 ThisMinX = StackX + Scale * Region->MinT;
                    r32 ThisMaxX = StackX + Scale * Region->MaxT;
                    rectangle2 RegionRect = RectMinMax(v2{ThisMinX, StackY - (Region->LaneIndex + 1) * LaneHeight}, 
                                                       v2{ThisMaxX, StackY - Region->LaneIndex * LaneHeight});
                    
                    PushRect(DEBUGRenderGroup, RegionRect, 0.0f, V4(Color, 1));
                    if (IsInRectangle(RegionRect, MouseP)) {
                        debug_record* Record = Region->Record;
                        char TextBuffer[256];
                        _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                                    "%32s: %10I64ucy %s(%d)",
                                    Record->BlockName,
                                    Region->CycleCount,
                                    Record->FileName,
                                    Record->LineNumber
                                    );
                        DEBUGTextLine(TextBuffer);
                    }
                }
            }
            
            //PushRect(DEBUGRenderGroup, v3{ChartLeft, ChartMinY + ChartHeight, 0}, v2{ChartWidth, 1}, v4{1, 1, 1, 1});
#endif
        }
    }
}

#define DebugRecords_Main_Count __COUNTER__
extern u32 DebugRecords_Optimized_Count;

global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;

inline u32
GetLaneFromtThreadIndex(debug_state *DebugState, u32 ThreadIndex) {
    u32 Result = 0;
    return(Result);
}

internal debug_thread *
GetDebugThread(debug_state *DebugState, u32 ThreadID) {
    debug_thread *Result = 0;
    for (debug_thread* Thread = DebugState->FirstThread; Thread; Thread = Thread->Next) {
        if (Thread->ID == ThreadID) {
            Result = Thread;
            break;
        }
    }
    if (!Result) {
        Result = PushStruct(&DebugState->CollateArena, debug_thread);
        Result->Next = DebugState->FirstThread;
        DebugState->FirstThread = Result;
        
        Result->ID = ThreadID;
        Result->FirstOpenBlock = 0;
        Result->LaneIndex = DebugState->FrameBarLaneCount++;
    }
    return (Result);
}

internal debug_frame_region *
AddRegion(debug_state *DebugState, debug_frame* CurrentFrame) {
    Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
    debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;
    return(Result);
}

internal void
CollateDebugRecords(debug_state *DebugState, u32 InvalidEventArrayIndex) {
    for (; ; ++DebugState->CollationArrayIndex) {
        
        if (DebugState->CollationArrayIndex == MAX_DEBUG_EVENT_ARRAY_COUNT) {
            DebugState->CollationArrayIndex = 0;
        }
        
        u32 EventArrayIndex = DebugState->CollationArrayIndex;
        if (EventArrayIndex == InvalidEventArrayIndex) {
            break;
        }
        for (u32 EventIndex = 0; EventIndex < GlobalDebugTable->EventCounts[EventArrayIndex]; ++EventIndex) {
            debug_event *Event = GlobalDebugTable->Events[EventArrayIndex] + EventIndex;
            debug_record* Src = GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex;
            if (Event->Type == DebugEvent_FrameMarker) {
                if (DebugState->CollationFrame) {
                    DebugState->CollationFrame->EndClock = Event->Clock;
                    DebugState->CollationFrame->WallSecondsElapsed = Event->SecondsElapsed;
                    ++DebugState->FrameCount;
                    r32 ClockRange = (r32)DebugState->CollationFrame->EndClock - (r32)DebugState->CollationFrame->BeginClock;
#if 0                 
                    if (ClockRange > 0.0f) {
                        // todo
                        r32 FrameBarScale = 1.0f / ClockRange;
                        if (DebugState->FrameBarScale > FrameBarScale) {
                            DebugState->FrameBarScale = FrameBarScale;
                        }
                    }
#endif
                }
                DebugState->CollationFrame = DebugState->Frames + DebugState->FrameCount;
                DebugState->CollationFrame->BeginClock = Event->Clock;
                DebugState->CollationFrame->EndClock = 0;
                DebugState->CollationFrame->RegionCount = 0;
                DebugState->CollationFrame->Regions = PushArray(&DebugState->CollateArena, MAX_REGIONS_PER_FRAME, debug_frame_region);
                DebugState->CollationFrame->WallSecondsElapsed = 0.0f;
            } 
            else if (DebugState->CollationFrame) {
                u32 FrameIndex = DebugState->FrameCount - 1;
                debug_thread *Thread = GetDebugThread(DebugState, Event->TC.ThreadID);
                u64 RelativeClock = Event->Clock - DebugState->CollationFrame->BeginClock;
                if (Event->Type == DebugEvent_BeginBlock) {
                    open_debug_block *DebugBlock = DebugState->FirstFreeBlock;
                    if (DebugBlock) {
                        DebugState->FirstFreeBlock = DebugBlock->NextFree;
                    } else {
                        DebugBlock = PushStruct(&DebugState->CollateArena, open_debug_block);
                    }
                    DebugBlock->StartingFrameIndex = FrameIndex;
                    DebugBlock->Event = Event;
                    DebugBlock->Parent = Thread->FirstOpenBlock;
                    Thread->FirstOpenBlock = DebugBlock;
                    DebugBlock->NextFree = 0;
                } else if(Event->Type == DebugEvent_EndBlock) {
                    if (Thread->FirstOpenBlock) {
                        open_debug_block* MatchingBlock = Thread->FirstOpenBlock;
                        debug_event* OpeningEvent = MatchingBlock->Event;
                        if (OpeningEvent->TC.ThreadID == Event->TC.ThreadID &&
                            OpeningEvent->DebugRecordIndex == Event->DebugRecordIndex &&
                            OpeningEvent->TranslationUnit == Event->TranslationUnit) {
                            if (MatchingBlock->StartingFrameIndex == FrameIndex) {
                                if (Thread->FirstOpenBlock->Parent == 0) {
                                    r32 MinT = (r32)(OpeningEvent->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 MaxT = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 ThresholdT = 0.01f;
                                    if (MaxT - MinT > ThresholdT) {
                                        debug_frame_region *Region = AddRegion(DebugState, DebugState->CollationFrame);
                                        Region->Record = Src;
                                        Region->CycleCount = Event->Clock - OpeningEvent->Clock;
                                        Region->LaneIndex = Thread->LaneIndex;
                                        Region->MinT = MinT;
                                        Region->MaxT = MaxT;
                                    }
                                }
                            } else {
                                
                            }
                            MatchingBlock->NextFree = DebugState->FirstFreeBlock;
                            DebugState->FirstFreeBlock = Thread->FirstOpenBlock;
                            Thread->FirstOpenBlock = MatchingBlock->Parent;
                        } else {
                            
                            
                        }
                    }
                } else {
                    Assert(!"Invalid Event Type")
                }
            }
        }
    }
}

internal void
RestartCollation(debug_state *DebugState, u32 InvalidEventArrayIndex) {
    EndTemporaryMemory(DebugState->CollateTemp);
    DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);
    DebugState->FirstThread = 0;
    DebugState->FirstFreeBlock = 0;
    
    DebugState->Frames = PushArray(&DebugState->CollateArena, MAX_DEBUG_EVENT_ARRAY_COUNT * 4, debug_frame);
    DebugState->FrameBarLaneCount = 0;
    DebugState->FrameBarScale = 1.0f / 60000000.0f;
    DebugState->FrameCount = 0;
    
    DebugState->CollationArrayIndex = InvalidEventArrayIndex + 1;
    DebugState->CollationFrame = 0;
}

extern "C" DEBUG_FRAME_END(DEBUGGameFrameEnd) {
    
    GlobalDebugTable->RecordCounts[0] = DebugRecords_Main_Count;
    GlobalDebugTable->RecordCounts[1] = DebugRecords_Optimized_Count;
    
    ++GlobalDebugTable->CurrentEventArrayIndex;
    if (GlobalDebugTable->CurrentEventArrayIndex >= ArrayCount(GlobalDebugTable->Events)) {
        GlobalDebugTable->CurrentEventArrayIndex = 0;
    }
    // flip value
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex, (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);
    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    GlobalDebugTable->EventCounts[EventArrayIndex] = EventCount;
    debug_state *DebugState = (debug_state* )Memory->DebugStorage;
    if (DebugState) {
        if (!DebugState->IsInitialized) {
            InitializeArena(&DebugState->CollateArena, Memory->DebugStorageSize - sizeof(debug_state), 
                            DebugState + 1);
            DebugState->CollateTemp = BeginTemporaryMemory(&DebugState->CollateArena);
            RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
            DebugState->IsInitialized = true;
        }
        
        if (!DebugState->Paused) {
            if (DebugState->FrameCount >= MAX_DEBUG_EVENT_ARRAY_COUNT * 4) {
                RestartCollation(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
            }
            
            CollateDebugRecords(DebugState, GlobalDebugTable->CurrentEventArrayIndex);
        }
    }
    
    return(GlobalDebugTable);
}