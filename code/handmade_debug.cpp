#include <stdio.h>

global_variable r32 AtY;
global_variable r32 LeftEdge;
global_variable r32 FontScale;
global_variable font_id FontID;

internal void
DEBUGReset(game_assets *Assets, u32 Width, u32 Height) {
    TIMED_FUNCTION();
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
    PushRect(DEBUGRenderGroup, v3{0, AtY, 0}, v2{1000.0f, 1.0f}, v4{1.0f, 0.5f, 1.0f, 0});
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
DEBUGOverlay(game_memory* Memory) {
    debug_state *DebugState = (debug_state* )Memory->DebugStorage;
    
    if (DebugState && DEBUGRenderGroup) {
        render_group* RenderGroup = DEBUGRenderGroup;
        loaded_font* Font = PushFont(RenderGroup, FontID);
        if (Font) {
            hha_font *Info = GetFontInfo(RenderGroup->Assets, FontID);
            
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
            r32 ChartHeight = 300.0f;
            r32 BarSpacing = 10.0f;
            r32 ChartWidth = BarSpacing * DEBUG_SNAPSHOT_COUNT;
            r32 ChartLeft = LeftEdge + 10.0f;
            r32 ChartMinY = AtY - ChartHeight - 80.0f;
            r32 BarWidth = 8.0f;
            
            r32 Scale = 1.0f / 0.033333f;
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
            
#if 0            
            for (u32 SnapIndex = 0; SnapIndex < DEBUG_SNAPSHOT_COUNT; ++SnapIndex) {
                debug_frame_end_info * FrameEndInfo = DebugState->FrameEndInfos + SnapIndex;
                r32 PreTimestampSeconds = 0.0f;
                r32 StackY = ChartMinY;
                for(u32 TimestampIndex = 0; TimestampIndex < FrameEndInfo->TimestampCount; ++TimestampIndex) {
                    debug_frame_timestamp* Timestamp = FrameEndInfo->Timestamps + TimestampIndex;
                    r32 ThisSecondsElapsed = Timestamp->Seconds - PreTimestampSeconds;
                    PreTimestampSeconds = Timestamp->Seconds;
                    
                    v3 Color = Colors[TimestampIndex % ArrayCount(Colors)];
                    r32 ThisProportion = Scale * ThisSecondsElapsed;
                    r32 ThisHeight = ChartHeight * ThisProportion;
                    PushRect(DEBUGRenderGroup, v3{ChartLeft + (r32)SnapIndex * BarSpacing + 0.5f * BarWidth, StackY + 0.5f * ThisHeight, 0}, v2{BarWidth, ThisHeight}, V4(Color, 1));
                    
                    StackY += ThisHeight;
                    
                }
            }
#endif
            
            PushRect(DEBUGRenderGroup, v3{ChartLeft, ChartMinY + ChartHeight, 0}, v2{ChartWidth, 1}, v4{1, 1, 1, 1});
        }
    }
}

#define DebugRecords_Main_Count __COUNTER__
extern u32 DebugRecords_Optimized_Count;

global_variable debug_table GlobalDebugTable_;
debug_table* GlobalDebugTable = &GlobalDebugTable_;

internal void
CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *Events) {
    debug_counter_state *CounterArray[MAX_DEBUG_TRANSLATION_UNIT];
    debug_counter_state *CurrentCounter = DebugState->CounterStates;
    u32 TotalRecordCount = 0;
    for (u32 UnitIndex = 0; UnitIndex < MAX_DEBUG_TRANSLATION_UNIT; ++UnitIndex) {
        CounterArray[UnitIndex] = CurrentCounter;
        TotalRecordCount += GlobalDebugTable->RecordCounts[UnitIndex];
        CurrentCounter += GlobalDebugTable->RecordCounts[UnitIndex];
    }
    
    DebugState->CounterCount = TotalRecordCount;
    
    for (u32 CounterIndex = 0; CounterIndex < DebugState->CounterCount; ++CounterIndex) {
        debug_counter_state* Dest = DebugState->CounterStates + CounterIndex;
        
        Dest->Snapshots[DebugState->SnapIndex].HitCount = 0;
        Dest->Snapshots[DebugState->SnapIndex].CycleCount = 0;
	}
    
    for (u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex) {
        debug_event *Event = Events + EventIndex;
        
        debug_counter_state *Dest = CounterArray[Event->TranslationUnit] + Event->DebugRecordIndex;
        debug_record* Src = GlobalDebugTable->Records[Event->TranslationUnit] + Event->DebugRecordIndex;
        
        Dest->FileName = Src->FileName;
        Dest->LineNumber = Src->LineNumber;
        Dest->BlockName = Src->BlockName;
        
        if (Event->Type == DebugEvent_BeginBlock) {
            ++Dest->Snapshots[DebugState->SnapIndex].HitCount;
            Dest->Snapshots[DebugState->SnapIndex].CycleCount -= Event->Clock;
        } else if (Event->Type == DebugEvent_EndBlock) {
            Dest->Snapshots[DebugState->SnapIndex].CycleCount += Event->Clock;
        }
    }
    
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
    
    debug_state *DebugState = (debug_state* )Memory->DebugStorage;
    if (DebugState) {
        DebugState->CounterCount = 0;
        CollateDebugRecords(DebugState, EventCount, GlobalDebugTable->Events[EventArrayIndex]);
        ++DebugState->SnapIndex;
        if (DebugState->SnapIndex >= DEBUG_SNAPSHOT_COUNT) {
            DebugState->SnapIndex = 0;
        }
    }
    
    return(GlobalDebugTable);
}