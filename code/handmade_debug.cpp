#include <stdio.h>

#include "handmade_debug.h"

internal void FreeFrame(debug_state *DebugState, debug_frame *Frame);

inline debug_id
GetDebugIDFromLink(debug_tree* Tree, debug_variable_link *Link) {
    debug_id Result = {};
    Result.Value[0] = Tree;
    Result.Value[1] = Link;
    return(Result);
}

inline debug_id
GetDebugIDFromGUID(debug_tree* Tree, char* GUID) {
    debug_id Result = {};
    Result.Value[0] = Tree;
    Result.Value[1] = GUID;
    return(Result);
}

inline b32
DebugIDsAreEqual(debug_id A, debug_id B) {
    b32 Result = (A.Value[0] == B.Value[0] && A.Value[1] == B.Value[1]) ? true: false;
    return(Result);
}

inline debug_state*
DEBUGGetState(game_memory* Memory) {
    debug_state *DebugState = 0;
    if (Memory) {
        DebugState = (debug_state*)Memory->DebugStorage;
        if(!DebugState->IsInitialized) {
            DebugState = 0;
        }
    }
    return(DebugState);
}
inline debug_state*
DEBUGGetState(void) {
    debug_state *DebugState = DEBUGGetState(DebugGlobalMemory);
    return(DebugState);
}

internal debug_tree*
DEBUGAddTree(debug_state *DebugState, debug_variable_group *Group, v2 TopLeftP) {
    debug_tree* Tree = (debug_tree*)PushStruct(&DebugState->DebugArena, debug_tree);
    Tree->UIP = TopLeftP;
    Tree->Group = Group;
    
    DLIST_INSERT(&(DebugState->TreeSentinal), Tree);
    
    return(Tree);
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

internal rectangle2
DEBUGTextOp(debug_text_op Op, debug_state* DebugState, v2 P, char* String, v4 Color = v4{1.0f, 1.0f, 1.0f, 1.0f}) {
    rectangle2 Result = InvertedInfinityRectangle2();
    if (DebugState && DebugState->Font) {
        
        render_group *RenderGroup = DebugState->RenderGroup;
        
        loaded_font* Font = DebugState->Font;
        hha_font* FontInfo = DebugState->FontInfo;
        
        u32 PrevCodePoint = 0;
        r32 TextY = P.y;
        r32 AtX = P.x;
        r32 CharScale = DebugState->FontScale;
        
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
                AdvancedX = CharScale * GetHorizontalAdvanceForPair(DebugState->FontInfo, DebugState->Font, PrevCodePoint, CodePoint);
            }
            AtX += AdvancedX;
            
            if (CodePoint != ' ') {
                // proportional mode
                // AdvancedX = CharScale * (r32)(Info->Dim[0] + 2);
                bitmap_id BitmapID = GetBitmapForGlyph(RenderGroup->Assets, DebugState->FontInfo, DebugState->Font, CodePoint);
                hha_bitmap* BitmapInfo = GetBitmapInfo(RenderGroup->Assets, BitmapID);
                
                r32 BitmapScale = CharScale * (r32)BitmapInfo->Dim[1];
                v3 BitmapOffset = {AtX, TextY, 0};
                
                if (Op == DEBUGTextOp_DrawText) {
                    PushBitmap(RenderGroup, BitmapID, BitmapScale, BitmapOffset, Color);
                } else {
                    Assert(Op == DEBUGTextOp_SizeText);
                    loaded_bitmap* Bitmap = GetBitmap(RenderGroup->Assets, BitmapID, RenderGroup->GenerationID);
                    if (Bitmap) {
                        used_bitmap_dim Dim = GetBitmapDim(RenderGroup, Bitmap, BitmapScale, BitmapOffset, 1.0f);
                        rectangle2 GlyphDim = RectMinDim(Dim.P.xy, Dim.Size);
                        Result = Union(Result, GlyphDim);
                    }
                }
            }
            // advance here
            
            PrevCodePoint = CodePoint;
        }
        //PushRect(RenderGroup, v3{LeftEdge, AtY, 0}, v2{1000.0f, 1.0f}, v4{1.0f, 0.0f, 0.0f, 1});
        //PushRect(RenderGroup, v3{LeftEdge, AtY + Info->Ascent, 0}, v2{1000.0f, 1.0f}, v4{0.0f, 1.0f, 0.0f, 1});
        //PushRect(RenderGroup, v3{LeftEdge, AtY - Info->Descent, 0}, v2{1000.0f, 1.0f}, v4{0.0f, 0.0f, 1.0f, 1});
    }
    return(Result);
}

internal void
DEBUGTextOutAt(v2 P, char* String, v4 Color = {1, 1, 1, 1}) {
    debug_state* DebugState = DEBUGGetState();
    if (DebugState) {
        DEBUGTextOp(DEBUGTextOp_DrawText, DebugState, P, String, Color);
    }
}

internal rectangle2
DEBUGGetTextSize(debug_state *DebugState, char* String) {
    rectangle2 Result = DEBUGTextOp(DEBUGTextOp_SizeText, DebugState, v2{ 0, 0 }, String);
    return(Result);
}

internal void
DEBUGTextLine(char *String) {
    debug_state* DebugState = DEBUGGetState();
    if (DebugState) {
        DEBUGTextOutAt(v2{DebugState->LeftEdge, DebugState->AtY - DebugState->FontScale * GetStartingBaselineY(DebugState->FontInfo)}, String);
        DebugState->AtY -= GetLineAdvancedFor(DebugState->FontInfo) * DebugState->FontScale;
    }
}

enum debug_var_to_text_flag {
    DebugVarToText_AddDebugUI = 0x1,
    DebugVarToText_AddName = 0x2,
    DebugVarToText_FloatSuffix = 0x4,
    DebugVarToText_LineFeedEnd = 0x8,
    DebugVarToText_NullTerminator = 0x10,
    DebugVarToText_Colon = 0x20,
    DebugVarToText_PrettyBools = 0x40,
};


internal memory_index
DEBUGEventToText(char* Buffer, char *End, debug_event* Event, u32 Flags) {
    
    char *At = Buffer;
    char *Name = Event->BlockName;
    
    if (Flags & DebugVarToText_AddDebugUI) {
        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                          "#define DEBUGUI_");
    } 
    
    if (Flags & DebugVarToText_AddName) {
        At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), "%s%s ", Name, 
                          ((Flags & DebugVarToText_Colon)? ":": ""));
    }
    switch (Event->Type) {
        case DebugType_b32:{
            if (Flags & DebugVarToText_PrettyBools) {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                                  "%s", Event->Value_b32? "True": "False");
            } else {
                At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                                  "%d", Event->Value_b32);
            }
            
        } break;
        case DebugType_s32: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "%i", Event->Value_s32);
        } break;
        case DebugType_u32: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "%u", Event->Value_u32);
        } break;
        case DebugType_r32: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "%f", Event->Value_r32);
            if (Flags & DebugVarToText_FloatSuffix) {
                *At++ = 'f';
            }
        } break;
        case DebugType_v2: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "V2(%f, %f)", Event->Value_v2.x, Event->Value_v2.y);
        } break;
        case DebugType_v3: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "V3(%f, %f, %f)", Event->Value_v3.x, Event->Value_v3.y, Event->Value_v3.z);
        } break;
        case DebugType_v4: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "V4(%ff, %ff, %ff, %ff)", Event->Value_v4.x, Event->Value_v4.y, Event->Value_v4.z, Event->Value_v4.w);
        } break;
        
        case DebugType_rectangle2: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "Min(v2):%f, %f / Max(v2):%f, %f",
                              Event->Value_rectangle2.Min.x, Event->Value_rectangle2.Min.y,
                              Event->Value_rectangle2.Max.x, Event->Value_rectangle2.Max.y);
        } break;
        
        case DebugType_rectangle3: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "Min(v3):%f, %f, %f / Max(v3):%f, %f, %f",
                              Event->Value_rectangle3.Min.x, Event->Value_rectangle3.Min.y, Event->Value_rectangle3.Min.z,
                              Event->Value_rectangle3.Max.x, Event->Value_rectangle3.Max.y, Event->Value_rectangle3.Max.z);
        } break;
        
        case DebugType_CounterThreadList:
        case DebugType_bitmap_id: {
        } break;
        case DebugType_OpenDataBlock: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "%s", Event->BlockName);
        } break;
        default: {
            At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                              "UNHANDLED: %s", Event->BlockName);
        } break;
    }
    if (Flags & DebugVarToText_LineFeedEnd) {
        *At++ = '\n'; 
    }
    if (Flags & DebugVarToText_NullTerminator) {
        *At++ = 0;
    }
    return(At - Buffer);
}

struct debug_variable_iterator {
    debug_variable_link* Link;
    debug_variable_link* Sentinal;
};

internal void
WriteZhaConfig(debug_state* DebugState) {
    
#if 0    
    char Temp[4096];
    char *At = Temp;
    char* End = Temp + sizeof(Temp);
    u32 Depth = 0;
    debug_variable_iterator Stack[DEBUG_MAX_VARIABLE_STACK_SIZE];
    Stack[Depth].Link = DebugState->RootGroup->VarGroup.Next;
    Stack[Depth].Sentinal = &DebugState->RootGroup->VarGroup;
    ++Depth;
    while (Depth > 0) {
        debug_variable_iterator *Iter = Stack + (Depth - 1);
        if (Iter->Link == Iter->Sentinal) {
            --Depth;
        } else {
            debug_variable* Var = Iter->Link->Var;
            Iter->Link = Iter->Link->Next;
            if (DEBUGShouldBeWritten(Var->Type))  {
                if (Var->Type == DebugVariableType_VarGroup) {
                    At += _snprintf_s(At, (size_t)(End - At), (size_t)(End - At), 
                                      "//");
                }
                At += DEBUGVariableToText(At, End, Var,
                                          DebugVarToText_AddDebugUI|DebugVarToText_AddName|
                                          DebugVarToText_LineFeedEnd|DebugVarToText_FloatSuffix);
            }
            if (Var->Type == DebugVariableType_VarGroup) {
                Iter = Stack + Depth;
                Iter->Link = Var->VarGroup.Next;
                Iter->Sentinal = &Var->VarGroup;
                Depth++;
            }
            
        }
    }
    Platform.DEBUGWriteEntireFile("../code/zha_config.h", (u32)(At - Temp), Temp);
    if (!DebugState->Compiling) {
        DebugState->Compiling = true;
        DebugState->Compiler = Platform.DEBUGExecuteSystemCommand("../code/", "c:\\windows\\system32\\cmd.exe", "/C build.bat");
    }
#endif
    
}

inline debug_interaction 
EventInteraction(debug_id ID, debug_interaction_type Type, debug_event *Event) {
    debug_interaction Result = {};
    Result.ID = ID;
    Result.Type = Type;
    Result.Event = Event;
    return(Result);
}

inline debug_interaction 
DebugIDInteraction(debug_interaction_type Type, debug_id ID) {
    debug_interaction Result = {};
    Result.ID = ID;
    Result.Type = Type;
    return(Result);
}

inline b32
InteractionAreEqual(debug_interaction A, debug_interaction B) {
    b32 Result = (DebugIDsAreEqual(A.ID, B.ID) && A.Type == B.Type && A.Generic == B.Generic);
    return(Result);
}

inline b32
InteractionIsHot(debug_state* DebugState, debug_interaction A) {
    b32 Result = InteractionAreEqual(A, DebugState->HotInteraction);
    return(Result);
}

internal void
DEBUG_HIT(debug_id ID, r32 ZValue){
    debug_state *DebugState = DEBUGGetState();
    if (DebugState) {
        DebugState->NextHotInteraction = DebugIDInteraction(DebugInteraction_Select, ID);
    }
};

internal void
ClearSelection(debug_state *DebugState) {
    DebugState->SelectIDCount = 0;
}



inline b32
IsSelected(debug_state *DebugState, debug_id ID) {
    b32 Result = false;
    for (u32 Index = 0; Index < DebugState->SelectIDCount; ++Index) {
        if (DebugIDsAreEqual(ID, DebugState->Selected[Index])) {
            Result = true;
            break;
        };
    }
    
    return(Result);
}

internal void
AddToSelection(debug_state *DebugState, debug_id ID) {
    if (DebugState->SelectIDCount < ArrayCount(DebugState->Selected) && !IsSelected(DebugState, ID)) {
        DebugState->Selected[DebugState->SelectIDCount++] = ID;
    }
}

internal b32
DEBUG_HIGHLIGHTED(debug_id ID, v4 *Color) {
    debug_state *DebugState = DEBUGGetState();
    b32 Result = false;
    if (DebugState) {
        if (IsSelected(DebugState, ID)) {
            *Color = V4(0, 1, 1, 1);
            Result = true;
        }
        if (DebugIDsAreEqual(DebugState->HotInteraction.ID, ID)) {
            *Color = V4(1, 1, 0, 1);
            Result = true;
        }
        
    }
    
    return(Result);
}

internal b32
DEBUG_REQUESTED(debug_id ID) {
    debug_state *DebugState = DEBUGGetState();
    b32 Result = false;
    if (DebugState) {
        Result = IsSelected(DebugState, ID) || DebugIDsAreEqual(DebugState->HotInteraction.ID, ID);
    }
    return(Result);
}


internal void
DrawProfile(debug_state *DebugState, rectangle2 ProfileRect, v2 MouseP) {
    PushRect(DebugState->RenderGroup, ProfileRect, 0, V4(0, 0, 0, 0.25f));
    
    r32 BarSpacing = 4.0f;
    r32 LaneHeight = 0.0f;
    u32 LaneCount = DebugState->FrameBarLaneCount;
    r32 FrameBarScale = Real32Maximum;
    
    for (debug_frame *Frame = DebugState->OldestFrame; Frame; Frame = Frame->Next) {
        
        if (Frame->FrameBarScale < FrameBarScale) {
            FrameBarScale = Frame->FrameBarScale;
        }
    }
    
    u32 MaxFrame = DebugState->FrameCount;
    if (MaxFrame > 10) {
        MaxFrame = 10;
    }
    if (MaxFrame > 0 && LaneCount > 0) {
        LaneHeight = (GetDim(ProfileRect).y / ((r32)MaxFrame) - BarSpacing) / (r32)LaneCount;
    }
    
    
    r32 BarHeight = LaneHeight * LaneCount;
    r32 BarAdvance = BarHeight + BarSpacing;
    
    r32 ChartHeight = BarAdvance * MaxFrame;
    r32 ChartWidth = GetDim(ProfileRect).x;
    
    r32 ChartLeft = ProfileRect.Min.x;
    r32 ChartTop = ProfileRect.Max.y;
    
    r32 Scale = ChartWidth * FrameBarScale;
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
    u32 FrameIndex = 0;
    for (debug_frame *Frame = DebugState->OldestFrame; Frame; Frame = Frame->Next, ++FrameIndex) {
        
        r32 StackX = ChartLeft;
        r32 StackY = ChartTop - (r32)FrameIndex * BarAdvance;
        
        for(u32 RegionIndex = 0; RegionIndex < Frame->RegionCount; ++RegionIndex) {
            debug_frame_region* Region = Frame->Regions + RegionIndex;
            
            v3 Color = Colors[Region->ColorIndex % ArrayCount(Colors)];
            r32 ThisMinX = StackX + Scale * Region->MinT;
            r32 ThisMaxX = StackX + Scale * Region->MaxT;
            rectangle2 RegionRect = RectMinMax(v2{ThisMinX, StackY - (Region->LaneIndex + 1) * LaneHeight}, 
                                               v2{ThisMaxX, StackY - Region->LaneIndex * LaneHeight});
            
            PushRect(DebugState->RenderGroup, RegionRect, 0.0f, V4(Color, 1));
            if (IsInRectangle(RegionRect, MouseP)) {
                debug_event* Event = Region->Event;
                char TextBuffer[256];
                _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                            "%s: %10I64ucy %s(%d)",
                            Event->BlockName,
                            Region->CycleCount,
                            Event->FileName,
                            Event->LineNumber
                            );
                DEBUGTextOutAt(MouseP + v2{0, 10.0f}, TextBuffer);
                //HotRecord = Record;
            }
        }
    }
#endif
    //PushRect(DEBUGRenderGroup, v3{ChartLeft, ChartMinY + ChartHeight, 0}, v2{ChartWidth, 1}, v4{1, 1, 1, 1});
}

struct layout {
    v2 At;
    u32 Depth;
    r32 LineAdvance;
    r32 SpacingY;
    v2 MouseP;
    debug_state *DebugState;
};

struct layout_element {
    layout* Layout;
    v2 *Dim;
    v2 *Size;
    debug_interaction Interaction;
    rectangle2 Bounds;
};

inline layout_element
BeginElementRectangle(layout *Layout, v2 *Dim) {
    layout_element Element = {};
    Element.Layout = Layout;
    Element.Dim = Dim;
    return(Element);
}

inline void
SetElementSizable(layout_element* Element) {
    Element->Size = Element->Dim;
}

inline void
EndElement(layout_element *Element) {
    layout* Layout = Element->Layout;
    debug_state* DebugState = Layout->DebugState;
    
    r32 SizeHandlePixels = 4.0f;
    v2 Frame = {0, 0};
    if (Element->Size) {
        Frame.x = SizeHandlePixels;
        Frame.y = SizeHandlePixels;
    }
    v2 TotalDim = *Element->Dim + 2.0f * Frame;
    
    v2 TotalMinCorner = {Layout->At.x + Layout->Depth * Layout->LineAdvance * 2.0f, Layout->At.y - TotalDim.y};
    v2 TotalMaxCorner = TotalMinCorner + TotalDim;
    rectangle2 TotalBounds = RectMinMax(TotalMinCorner, TotalMaxCorner);
    
    v2 MinCorner = TotalMinCorner + Frame;
    // v2 MaxCorner = TotalMaxCorner - Frame;
    v2 MaxCorner = TotalMinCorner + *Element->Dim;
    Element->Bounds = RectMinMax(MinCorner, MaxCorner);
    
    PushRect(DebugState->RenderGroup, RectMinMax(v2{TotalMinCorner.x, MinCorner.y}, v2{MinCorner.x, MaxCorner.y}), 0.0f, v4{0, 0, 0, 1});
    PushRect(DebugState->RenderGroup, RectMinMax(v2{MaxCorner.x, MinCorner.y}, v2{TotalMaxCorner.x, MaxCorner.y}), 0.0f, v4{0, 0, 0, 1});
    
    PushRect(DebugState->RenderGroup, RectMinMax(v2{MinCorner.x, MaxCorner.y}, v2{MaxCorner.x, TotalMaxCorner.y}), 0.0f, v4{0, 0, 0, 1});
    PushRect(DebugState->RenderGroup, RectMinMax(v2{MinCorner.x, TotalMinCorner.y}, v2{MaxCorner.x, MinCorner.y}), 0.0f, v4{0, 0, 0, 1});
    
    if (Element->Interaction.Type && IsInRectangle(Element->Bounds, Layout->MouseP)) {
        DebugState->NextHotInteraction = Element->Interaction;
    }
    
    if (Element->Size) {
        v2 SizeP = {GetMaxCorner(Element->Bounds).x, GetMinCorner(Element->Bounds).y};
        rectangle2 SizeRect= RectMinMax(v2{MaxCorner.x, TotalMinCorner.y}, v2{TotalMaxCorner.x, MinCorner.y});
        
        debug_interaction SizeInteraction = {};
        SizeInteraction.Type = DebugInteraction_Resize;
        SizeInteraction.P = Element->Size;
        PushRect(DebugState->RenderGroup, SizeRect, 0.0f, 
                 (InteractionIsHot(DebugState, SizeInteraction))? v4{1, 1, 0, 1}: v4{1, 1, 1, 1});
        if (IsInRectangle(SizeRect, Layout->MouseP)) {
            DebugState->NextHotInteraction = SizeInteraction;
        }
    }
    
    
    
    r32 SpacingY = Layout->SpacingY;
    if (0) {
        SpacingY = 0;
    }
    Layout->At.y = GetMinCorner(TotalBounds).y - SpacingY;
}

inline void
SetElementDefaultInteraction(layout_element *Element, debug_interaction Interaction) {
    Element->Interaction = Interaction;
}

internal debug_view*
GetDebugViewFor(debug_state *DebugState, debug_id ID) {
    debug_view *Result = 0;
    u32 HashIndex = (((u32)ID.Value[0] >> 2) + ((u32)ID.Value[1] >> 2)) % ArrayCount(DebugState->ViewHash);
    debug_view **HashSlot = DebugState->ViewHash + HashIndex;
    for (debug_view *Search = *HashSlot; Search; Search = Search->NextInHash) {
        if (DebugIDsAreEqual(Search->ID, ID)) {
            Result = Search;
            break;
        }
    }
    if (!Result) {
        Result = (debug_view *)PushStruct(&DebugState->DebugArena, debug_view);
        Result->ID = ID;
        Result->Type = DebugViewType_Unknown;
        Result->NextInHash = *HashSlot;
        *HashSlot = Result;
        
    }
    return(Result);
}

internal void
DEBUGDrawEvent(layout *Layout, debug_stored_event* StoredEvent, debug_id ID) {
    debug_state *DebugState = Layout->DebugState;
    render_group* RenderGroup = DebugState->RenderGroup;
    if (StoredEvent) {
        debug_event *Event = &StoredEvent->Event;
        debug_interaction AutoInteraction = EventInteraction(ID, DebugInteraction_AutoModifyVariable, Event);
        
        b32 IsHot = InteractionAreEqual(DebugState->HotInteraction, AutoInteraction);
        v4 ItemColor = (IsHot)? v4{1, 1, 0, 1}: v4{1, 1, 1, 1};
        
        debug_view *View = GetDebugViewFor(DebugState, ID);
        
        switch(Event->Type) {
            
            case DebugType_bitmap_id: {
                loaded_bitmap* Bitmap = GetBitmap(RenderGroup->Assets, Event->Value_bitmap_id, RenderGroup->GenerationID);
                r32 BitmapScale = View->InlineBlock.Dim.y;
                if (Bitmap) {
                    used_bitmap_dim Dim = GetBitmapDim(RenderGroup, Bitmap, BitmapScale, V3(0, 0, 0), 1.0f);
                    View->InlineBlock.Dim.x = Dim.Size.x;
                }
                debug_interaction TearInteraction = EventInteraction(ID, DebugInteraction_TearValue, Event);
                
                layout_element Element = BeginElementRectangle(Layout, &View->InlineBlock.Dim);
                SetElementSizable(&Element);
                SetElementDefaultInteraction(&Element, TearInteraction);
                EndElement(&Element);
                PushRect(DebugState->RenderGroup, Element.Bounds, 0.0f, v4{0, 0, 0, 1.0f});
                PushBitmap(DebugState->RenderGroup, Event->Value_bitmap_id, BitmapScale, V3(GetMinCorner(Element.Bounds), 0.0f), v4{1, 1, 1, 1}, 0.0f);
            } break;
            default: {
                char Text[256];
                DEBUGEventToText(Text, Text + sizeof(Text), Event, DebugVarToText_AddName|DebugVarToText_FloatSuffix
                                 |DebugVarToText_Colon|DebugVarToText_NullTerminator|DebugVarToText_PrettyBools);
                
                r32 LeftX = Layout->At.x + Layout->Depth * Layout->LineAdvance * 2.0f;
                r32 TopY = Layout->At.y;
                rectangle2 TextBound = DEBUGGetTextSize(DebugState, Text);
                
                v2 Dim = {GetDim(TextBound).x, Layout->LineAdvance};
                layout_element Element = BeginElementRectangle(Layout, &Dim);
                SetElementDefaultInteraction(&Element, AutoInteraction);
                EndElement(&Element);
                
                DEBUGTextOutAt(V2(GetMinCorner(Element.Bounds).x, GetMaxCorner(Element.Bounds).y - DebugState->FontScale * GetStartingBaselineY(DebugState->FontInfo)), Text, ItemColor);
            } break;
        }
    }
}

internal void
DEBUGDrawElement(layout *Layout, debug_tree *Tree, debug_element *Element, debug_id ID) {
    debug_stored_event *OldestEvent = Element->OldestEvent;
    debug_state *DebugState = Layout->DebugState;
    if (OldestEvent) {
        debug_view *View = GetDebugViewFor(DebugState, ID);
        switch(OldestEvent->Event.Type) {
            case DebugType_CounterThreadList: {
                layout_element LayoutElement = BeginElementRectangle(Layout, &View->InlineBlock.Dim);
                SetElementSizable(&LayoutElement);
                EndElement(&LayoutElement);
                DrawProfile(DebugState, LayoutElement.Bounds, Layout->MouseP);
            } break;
            
            case DebugType_OpenDataBlock: {
                debug_stored_event *LastOpenEvent = Element->OldestEvent;
                for (debug_stored_event *Event = Element->OldestEvent; Event; Event = Event->Next) {
                    if (Event->Event.Type == DebugType_OpenDataBlock) {
                        LastOpenEvent = Event;
                    }
                }
                
                for (debug_stored_event *Event = LastOpenEvent; Event; Event = Event->Next) {
                    
                    debug_id NewID = GetDebugIDFromGUID(Tree, Event->Event.GUID);
                    DEBUGDrawEvent(Layout, Event, NewID);
                }
            } break;
            default: {
                debug_stored_event *Event = Element->LatestEvent;
                DEBUGDrawEvent(Layout, Event, ID);
            } break;
        }
    }
}

internal void
DEBUGDrawMainMenu(debug_state* DebugState, render_group* RenderGroup, v2 MouseP) {
    for (debug_tree* Tree = DebugState->TreeSentinal.Next;
         Tree != &DebugState->TreeSentinal; 
         Tree = Tree->Next) {
        layout Layout = {};
        
        Layout.At = Tree->UIP;
        Layout.LineAdvance = GetLineAdvancedFor(DebugState->FontInfo) * DebugState->FontScale;
        Layout.SpacingY = 4.0f;
        Layout.MouseP = MouseP;
        Layout.Depth = 0;
        Layout.DebugState = DebugState;
        
        char Text[256];
        
        u32 Depth = 0;
        
        debug_variable_group *Group = Tree->Group;
        
        if (Group) {
            debug_variable_iterator Stack[DEBUG_MAX_VARIABLE_STACK_SIZE];
            Stack[Depth].Link = Group->Sentinal.Next;
            Stack[Depth].Sentinal = &Group->Sentinal;
            ++Depth;
            while (Depth > 0) {
                debug_variable_iterator *Iter = Stack + (Depth - 1);
                if (Iter->Link == Iter->Sentinal) {
                    --Depth;
                } else {
                    Layout.Depth = Depth;
                    debug_variable_link *Link = Iter->Link;
                    
                    Iter->Link = Iter->Link->Next;
                    if (Link->Children) {
                        debug_id ID = GetDebugIDFromLink(Tree, Link);
                        debug_view *View = GetDebugViewFor(DebugState, ID);
                        debug_interaction AutoInteraction = DebugIDInteraction(DebugInteraction_ToggleExpansion, ID);
                        
                        Text[0] = 0;
                        Assert((Link->Children->NameLength + 1) < ArrayCount(Text));
                        Copy(Link->Children->NameLength, Link->Children->Name, Text);
                        Text[Link->Children->NameLength] = 0;
                        
                        r32 LeftX = Layout.At.x + Layout.Depth * Layout.LineAdvance * 2.0f;
                        r32 TopY = Layout.At.y;
                        rectangle2 TextBound = DEBUGGetTextSize(DebugState, Text);
                        
                        v2 Dim = {GetDim(TextBound).x, Layout.LineAdvance};
                        layout_element Element = BeginElementRectangle(&Layout, &Dim);
                        SetElementDefaultInteraction(&Element, AutoInteraction);
                        EndElement(&Element);
                        b32 IsHot = InteractionAreEqual(DebugState->HotInteraction, AutoInteraction);
                        v4 ItemColor = (IsHot)? v4{1, 1, 0, 1}: v4{1, 1, 1, 1};
                        DEBUGTextOutAt(V2(GetMinCorner(Element.Bounds).x, GetMaxCorner(Element.Bounds).y - DebugState->FontScale * GetStartingBaselineY(DebugState->FontInfo)), Text, ItemColor);
                        
                        
                        if (View->Collapsible.ExpandedAlways) {
                            Iter = Stack + Depth;
                            Iter->Link = Link->Children->Sentinal.Next;
                            Iter->Sentinal = &Link->Children->Sentinal;
                            Depth++;
                        }
                        
                    } else {
                        debug_id ID = GetDebugIDFromLink(Tree, Link);
                        DEBUGDrawElement(&Layout, Tree, Link->Element, ID);
                    }
                }
            }
        }
        DebugState->AtY = Layout.At.y;
        {
            debug_interaction MoveInteraction = {};
            MoveInteraction.Type = DebugInteraction_Move;
            MoveInteraction.P = &Tree->UIP;
            
            rectangle2 MoveRect = RectMinDim(Tree->UIP - v2{5.0f, 5.0f}, v2{5.0f, 5.0f});
            PushRect(DebugState->RenderGroup, MoveRect, 0.0f, 
                     InteractionIsHot(DebugState, MoveInteraction)? v4{1, 1, 0, 1}: v4{1, 1, 1, 1});
            
            if (IsInRectangle(MoveRect, MouseP)) {
                DebugState->NextHotInteraction = MoveInteraction;
            }
            
        }
    }
}

internal void
DEBUGBeginInteract(debug_state *DebugState, game_input *Input, v2 MouseP, b32 AltUI) {
    if (DebugState->HotInteraction.Type) {
        if (DebugState->HotInteraction.Type == DebugInteraction_AutoModifyVariable) {
            switch(DebugState->HotInteraction.Event->Type) {
                
                case DebugType_b32: {
                    DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;
                
                case DebugType_r32: {
                    DebugState->HotInteraction.Type = DebugInteraction_DragValue;
                } break;
                case DebugType_OpenDataBlock: {
                    DebugState->HotInteraction.Type = DebugInteraction_ToggleValue;
                } break;
            }
            
            if (AltUI) {
                DebugState->HotInteraction.Type = DebugInteraction_TearValue;
            } else {
            }
        }
        
        switch(DebugState->HotInteraction.Type) {
            case DebugInteraction_TearValue: {
#if 0                
                debug_variable* Group = DEBUGAddRootGroup(DebugState, "Tear");
                DEBUGPushVariableToGroup(DebugState, Group, DebugState->HotInteraction.Var);
                debug_tree *Tree = DEBUGAddTree(DebugState, Group, MouseP);
                Tree->UIP = MouseP;
                DebugState->HotInteraction.Type = DebugInteraction_Move;
                DebugState->HotInteraction.P = &Tree->UIP;
#endif
            } break;
            case DebugInteraction_Select: { 
                if (!Input->ShiftDown) {
                    ClearSelection(DebugState);
                }
                AddToSelection(DebugState, DebugState->HotInteraction.ID);
            } break;
        }
        
        DebugState->Interaction = DebugState->HotInteraction;
    } else {
        
        DebugState->Interaction.Type = DebugInteraction_NOP;
    }
}

internal void
DEBUGEndInteract(debug_state *DebugState, game_input *Input, v2 MouseP) {
    switch(DebugState->Interaction.Type) {
        case DebugInteraction_ToggleExpansion: {
            debug_view *View = GetDebugViewFor(DebugState, DebugState->Interaction.ID);
            View->Collapsible.ExpandedAlways = !View->Collapsible.ExpandedAlways;
        } break;
        case DebugInteraction_ToggleValue: {
            debug_event *Event = DebugState->Interaction.Event;
            Assert(Event);
            switch (Event->Type) {
                case DebugType_b32: {
                    Event->Value_b32 = !Event->Value_b32;
                } break;
            }
        } break;
        default: {
        } break;
    }
    
    WriteZhaConfig(DebugState);
    DebugState->Interaction.Type = DebugInteraction_None;
    DebugState->Interaction.Generic = 0;
}


internal void
DEBUGInteract(debug_state* DebugState, game_input *Input, v2 MouseP) {
    v2 dMouseP = MouseP - DebugState->LastMouseP;
    
    if (DebugState->Interaction.Type) {
        debug_event* Event = DebugState->Interaction.Event;
        // mouse handle
        debug_tree* Tree = DebugState->Interaction.Tree;
        v2* P = DebugState->Interaction.P;
        switch(DebugState->Interaction.Type) {
            case DebugInteraction_DragValue: {
                switch(Event->Type) {
                    case DebugType_r32: {
                        Event->Value_r32 += 0.1f * dMouseP.x;
                    } break;
                }
            } break;
            case DebugInteraction_Resize: {
                *P += v2{dMouseP.x, -dMouseP.y};
                P->x = Maximum(P->x, 10.0f);
                P->y = Maximum(P->y, 10.0f);
                
            } break;
            
            case DebugInteraction_Move: {
                *P += v2{dMouseP.x, dMouseP.y};
            } break;
            
            case DebugInteraction_ToggleValue: {
            } break;
            
        }
        // click handle
        b32 AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
        for (u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
             TransitionIndex > 1;
             --TransitionIndex) {
            DEBUGEndInteract(DebugState, Input, MouseP);
            DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
        }
        if (!(Input->MouseButtons[PlatformMouseButton_Left].EndedDown)) {
            DEBUGEndInteract(DebugState, Input, MouseP);
        }
    } else {
        DebugState->HotInteraction = DebugState->NextHotInteraction;
        b32 AltUI = Input->MouseButtons[PlatformMouseButton_Right].EndedDown;
        for (u32 TransitionIndex = Input->MouseButtons[PlatformMouseButton_Left].HalfTransitionCount;
             TransitionIndex > 1;
             --TransitionIndex) {
            DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
            DEBUGEndInteract(DebugState, Input, MouseP);
        }
        if (Input->MouseButtons[PlatformMouseButton_Left].EndedDown) {
            DEBUGBeginInteract(DebugState, Input, MouseP, AltUI);
        }
    }
    DebugState->LastMouseP = MouseP;
}

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
        FREELIST_ALLOC(DebugState->FirstFreeThread, Result, PushStruct(&DebugState->DebugArena, debug_thread));
        
        Result->Next = DebugState->FirstThread;
        DebugState->FirstThread = Result;
        
        Result->ID = ThreadID;
        Result->FirstOpenCodeBlock = 0;
        Result->FirstOpenDataBlock = 0;
        Result->LaneIndex = DebugState->FrameBarLaneCount++;
    }
    return (Result);
}

#if 0
internal debug_frame_region *
AddRegion(debug_state *DebugState, debug_frame* CurrentFrame) {
    Assert(CurrentFrame->RegionCount < MAX_REGIONS_PER_FRAME);
    debug_frame_region* Result = CurrentFrame->Regions + CurrentFrame->RegionCount++;
    return(Result);
}
#endif

inline open_debug_block *
AllocateOpenDebugBlock(debug_state *DebugState, debug_element *Element, u32 FrameIndex, debug_event *Event, open_debug_block **FirstOpenBlock) {
    open_debug_block *DebugBlock;
    FREELIST_ALLOC(DebugState->FirstFreeBlock, DebugBlock, PushStruct(&DebugState->DebugArena, open_debug_block));
    
    DebugBlock->StartingFrameIndex = FrameIndex;
    DebugBlock->Event = Event;
    DebugBlock->NextFree = 0;
    DebugBlock->Element = Element;
    
    DebugBlock->Parent = *FirstOpenBlock;
    *FirstOpenBlock = DebugBlock;
    
    return(DebugBlock);
}

inline void
DeallocateOpenDebugBlock(debug_state *DebugState, open_debug_block** Block) {
    open_debug_block *FreeBlock = *Block;
    *Block = FreeBlock->Parent;
    
    FreeBlock->NextFree = DebugState->FirstFreeBlock;
    DebugState->FirstFreeBlock = FreeBlock;
}

inline b32
EventsMatch(debug_event *A, debug_event* B) {
    b32 Result = (A->ThreadID == B->ThreadID);
    return(Result);
}

internal debug_variable_group*
CreateVariableGroup(debug_state *DebugState, char* Name, u32 NameLength) {
    debug_variable_group *Group = PushStruct(&DebugState->DebugArena, debug_variable_group);
    DLIST_INIT(&Group->Sentinal);
    Group->NameLength = NameLength;
    Group->Name = Name;
    
    return(Group);
}

internal debug_variable_link *
AddElementToGroup(debug_state* DebugState, debug_variable_group *Parent, debug_element* Addend) {
    debug_variable_link *Link = PushStruct(&DebugState->DebugArena, debug_variable_link);
    Link->Element = Addend;
    Link->Children = 0;
    DLIST_INSERT(&Parent->Sentinal, Link);
    
    return(Link);
}

internal debug_variable_link *
AddGroupToGroup(debug_state* DebugState, debug_variable_group *Group, debug_variable_group* Addend) {
    debug_variable_link *Link = PushStruct(&DebugState->DebugArena, debug_variable_link);
    Link->Element = 0;
    Link->Children = Addend;
    DLIST_INSERT(&Group->Sentinal, Link);
    return(Link);
}


internal debug_variable_group*
GetOrCreateGroupWithName(debug_state *DebugState, debug_variable_group* Parent, char *Name, u32 NameLength) {
    debug_variable_group* Result = 0;
    for (debug_variable_link *Link = Parent->Sentinal.Next; Link != &Parent->Sentinal; Link = Link->Next) {
        if (Link->Children && StringsAreEqual(Link->Children->NameLength, Link->Children->Name,
                                              NameLength, Name)) {
            Result = Link->Children;
        }
    }
    if (!Result) {
        Result = CreateVariableGroup(DebugState, Name, NameLength);
        AddGroupToGroup(DebugState, Parent, Result);
    }
    
    return(Result);
}

internal debug_variable_group*
GetGroupForHierarchicalName(debug_state *DebugState, debug_variable_group* Parent, char *Name) {
    debug_variable_group *Result = Parent;
    
    char *FirstUnderScore = 0;
    for (char *Scan = Name; *Scan; ++Scan) {
        if (*Scan == '_') {
            FirstUnderScore = Scan;
        }
    }
    
    if (FirstUnderScore) {
        debug_variable_group *SubGroup = GetOrCreateGroupWithName(DebugState, Parent, Name, (u32)(FirstUnderScore - Name));
        Result = GetGroupForHierarchicalName(DebugState, SubGroup, FirstUnderScore + 1);
    }
    return(Result);
}

internal void
FreeVariableGroup(debug_state *DebugState, debug_variable_group *Group) {
    Assert(!"not implement");
    // FREELIST_DEALLOC(Group, DebugState->First)
}

internal void
FreeFrame(debug_state *DebugState, debug_frame *Frame) {
    for (u32 ElementHashIndex = 0; ElementHashIndex < ArrayCount(DebugState->ElementHash); ++ElementHashIndex) {
        for (debug_element *Element = DebugState->ElementHash[ElementHashIndex]; Element; Element = Element->NextInHash) {
            while(Element->OldestEvent && Element->OldestEvent->FrameIndex <= Frame->FrameIndex) {
                debug_stored_event *Event = Element->OldestEvent;
                Element->OldestEvent = Event->Next;
                if (Element->LatestEvent == Event) {
                    Assert(Event->Next == 0);
                    Element->LatestEvent = 0;
                }
                FREELIST_DEALLOC(DebugState->FirstFreeStoredEvent, Event);
            }
        }
    }
    FREELIST_DEALLOC(DebugState->FirstFreeFrame, Frame);
}

internal void
FreeOldestFrame(debug_state *DebugState) {
    
    if (DebugState->OldestFrame) {
        debug_frame *Frame = DebugState->OldestFrame;
        DebugState->OldestFrame = Frame->Next;
        if (DebugState->LatestFrame == Frame) {
            Assert(Frame->Next == 0);
            DebugState->LatestFrame = 0;
        }
        FreeFrame(DebugState, Frame);
    }
    
}

internal debug_frame *
NewFrame(debug_state *DebugState, u64 BeginClock) {
    debug_frame *Result = 0;
    
    while (!Result) {
        Result = DebugState->FirstFreeFrame;
        if (Result) {
            DebugState->FirstFreeFrame = Result->NextFree;
        } else {
            if (ArenaHasRoomFor(&DebugState->PerFrameArena, sizeof(debug_frame))) {
                Result = PushStruct(&DebugState->PerFrameArena, debug_frame);
            } else {
                Assert(DebugState->OldestFrame);
                FreeOldestFrame(DebugState);
            }
        }
    }
    ZeroStruct(*Result);
    Result->FrameBarScale = 0;
    Result->FrameIndex = DebugState->TotalFrameCount++;
    Result->BeginClock = BeginClock;
    return(Result);
}

internal debug_stored_event *
StoreEvent(debug_state *DebugState, debug_element *Element, debug_event *Event) {
    debug_stored_event *Result = 0;
    while (!Result) {
        Result = DebugState->FirstFreeStoredEvent;
        if (Result) {
            DebugState->FirstFreeStoredEvent = Result->NextFree;
        } else {
            if (ArenaHasRoomFor(&DebugState->PerFrameArena, sizeof(debug_stored_event))) {
                Result = PushStruct(&DebugState->PerFrameArena, debug_stored_event);
            } else {
                Assert(DebugState->OldestFrame);
                FreeOldestFrame(DebugState);
            }
        }
    }
    
    Result->Next = 0;
    Result->FrameIndex = DebugState->CollationFrame->FrameIndex;
    Result->Event = *Event;
    if (Element->LatestEvent) {
        Element->LatestEvent->Next = Result;
        Element->LatestEvent = Result;
    } else {
        Element->OldestEvent = Element->LatestEvent = Result;
    }
    return(Result);
}

internal debug_element *
GetElementFromEvent(debug_state *DebugState, debug_event *Event) {
    
    Assert(Event->GUID);
    
    u32 HashValue = ((u32)(memory_index)Event->GUID >> 2);
    u32 Index = HashValue % ArrayCount(DebugState->ElementHash);
    debug_element * Result = 0;
    
    for(debug_element *Chain = DebugState->ElementHash[Index]; Chain; Chain = Chain->NextInHash) {
        if (Chain->GUID == Event->GUID) {
            Result = Chain;
            break;
        }
    }
    
    if (!Result) {
        Result = PushStruct(&DebugState->DebugArena, debug_element);
        
        Result->NextInHash = DebugState->ElementHash[Index];
        Result->GUID = Event->GUID;
        DebugState->ElementHash[Index] = Result;
        
        Result->OldestEvent = Result->LatestEvent = 0;
        debug_variable_group *ParentGroup = GetGroupForHierarchicalName(DebugState, DebugState->RootGroup, Event->BlockName);
        AddElementToGroup(DebugState, ParentGroup, Result);
        
    }
    return(Result);
}

internal void
CollateDebugRecords(debug_state *DebugState, u32 EventCount, debug_event *EventArray) {
    for (u32 EventIndex = 0; EventIndex < EventCount; ++EventIndex) {
        
        debug_event *Event = EventArray + EventIndex;
        
        debug_element *Element = GetElementFromEvent(DebugState, Event);
        
        if (!DebugState->CollationFrame) {
            DebugState->CollationFrame = NewFrame(DebugState, Event->Clock);
        }
        
        if (Event->Type == DebugType_VariableMarker) {
            StoreEvent(DebugState, Element, Event);
        } else if (Event->Type == DebugType_FrameMarker) {
            Assert(DebugState->CollationFrame);
            
            
            DebugState->CollationFrame->EndClock = Event->Clock;
            DebugState->CollationFrame->WallSecondsElapsed = Event->Value_r32;
            
            
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
            
            if (DebugState->Paused) {
                FreeFrame(DebugState, DebugState->CollationFrame);
            } else {
                if (DebugState->LatestFrame) {
                    DebugState->LatestFrame->Next = DebugState->CollationFrame;
                    DebugState->LatestFrame = DebugState->CollationFrame;
                } else {
                    DebugState->LatestFrame = DebugState->OldestFrame = DebugState->CollationFrame;
                }
                ++DebugState->FrameCount;
            }
            
            DebugState->CollationFrame = NewFrame(DebugState, Event->Clock);
            
        } 
        else {
            Assert(DebugState->CollationFrame);
            
            u32 FrameIndex = DebugState->FrameCount - 1;
            debug_thread *Thread = GetDebugThread(DebugState, Event->ThreadID);
            u64 RelativeClock = Event->Clock - DebugState->CollationFrame->BeginClock;
            
            switch(Event->Type) {
                case DebugType_BeginBlock: {
                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(DebugState, Element, FrameIndex, Event, &Thread->FirstOpenCodeBlock);
                } break;
                case DebugType_EndBlock: {
                    if (Thread->FirstOpenCodeBlock) {
                        open_debug_block* MatchingBlock = Thread->FirstOpenCodeBlock;
                        debug_event* OpeningEvent = MatchingBlock->Event;
                        if (EventsMatch(OpeningEvent, Event)) {
                            if (MatchingBlock->StartingFrameIndex == FrameIndex) {
                                char *MatchName = MatchingBlock->Parent? MatchingBlock->Parent->Event->BlockName: 0;
                                if (MatchName == DebugState->RecordToScope) {
#if 0                                    
                                    r32 MinT = (r32)(OpeningEvent->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 MaxT = (r32)(Event->Clock - DebugState->CollationFrame->BeginClock);
                                    r32 ThresholdT = 0.01f;
                                    if (MaxT - MinT > ThresholdT) {
                                        debug_frame_region *Region = AddRegion(DebugState, DebugState->CollationFrame);
                                        Region->Event = OpeningEvent;
                                        Region->CycleCount = Event->Clock - OpeningEvent->Clock;
                                        Region->LaneIndex = (u16)Thread->LaneIndex;
                                        Region->MinT = MinT;
                                        Region->MaxT = MaxT;
                                        Region->ColorIndex = (u16)OpeningEvent->BlockName;
                                    }
#endif
                                }
                            } else {
                            }
                            DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenCodeBlock);
                        } else {
                        }
                    }
                } break;
                case DebugType_OpenDataBlock: {
                    open_debug_block *DebugBlock = AllocateOpenDebugBlock(DebugState, Element, FrameIndex, Event, &Thread->FirstOpenDataBlock);
                    StoreEvent(DebugState, Element, Event);
                } break;
                case DebugType_CloseDataBlock: {
                    if (Thread->FirstOpenDataBlock) {
                        StoreEvent(DebugState, Thread->FirstOpenDataBlock->Element, Event);
                        open_debug_block* MatchingBlock = Thread->FirstOpenDataBlock;
                        debug_event* OpeningEvent = MatchingBlock->Event;
                        if (EventsMatch(OpeningEvent, Event)) {
                            DeallocateOpenDebugBlock(DebugState, &Thread->FirstOpenDataBlock);
                        }
                    }
                } break;
                default: {
                    debug_element * StoredElement = Element;
                    if (Thread->FirstOpenDataBlock) {
                        StoredElement = Thread->FirstOpenDataBlock->Element;
                    }
                    StoreEvent(DebugState, StoredElement, Event);
                } break;
            }
        }
    }
}

internal void
DEBUGDumpStruct(u32 MemberCount, member_definition *MemberDefs, void *StructPtr, u32 LineIndent = 0) {
    for (u32 MemberIndex = 0; MemberIndex < MemberCount; ++MemberIndex) {
        member_definition *Member = MemberDefs + MemberIndex;
        char TextBufferBase[256];
        char *TextBuffer = TextBufferBase;
        for (u32 Indent = 0; Indent < LineIndent; ++Indent) {
            *TextBuffer++ = ' ';
            *TextBuffer++ = ' ';
            *TextBuffer++ = ' ';
            *TextBuffer++ = ' ';
        }
        TextBuffer[0] = 0;
        size_t TextBufferLeft = TextBufferBase + sizeof(TextBufferBase) - TextBuffer;
        void* MemberPtr = (((u8 *)StructPtr)+ Member->Offset);
        if (Member->Flags & MetaMemberFlag_IsPointer) {
            MemberPtr = *(void **)MemberPtr;
        }
        if (MemberPtr) {
            
            switch(Member->Type) {
                case MetaType_u32: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, 
                                "%s: %u",
                                Member->Name, *(u32 *)MemberPtr);
                    
                } break;
                case MetaType_b32: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft,
                                "%s: %u",
                                Member->Name, *(b32 *)MemberPtr);
                    
                } break;
                case MetaType_s32: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, 
                                "%s: %d",
                                Member->Name, *(s32 *)MemberPtr);
                    
                } break;
                case MetaType_r32: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft,
                                "%s: %f",
                                Member->Name, *(r32 *)MemberPtr);
                    
                } break;
                case MetaType_v2: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft, 
                                "%s: {%f, %f}",
                                Member->Name, ((v2 *)MemberPtr)->x, ((v2 *)MemberPtr)->y);
                } break;
                case MetaType_v3: {
                    _snprintf_s(TextBuffer, TextBufferLeft, TextBufferLeft,
                                "%s: {%f, %f, %f}",
                                Member->Name, ((v3 *)MemberPtr)->x, ((v3 *)MemberPtr)->y, ((v3 *)MemberPtr)->z);
                } break;
                
                META_HANDLE_TYPE_DUMP(MemberPtr, LineIndent + 1);
                default: {
                } break;
            }
        }
        if (TextBuffer[0]) {
            DEBUGTextLine(TextBufferBase);
        }
        
    }
}

internal void
DEBUGStart(debug_state* DebugState, game_assets *Assets, u32 Width, u32 Height) {
    TIMED_FUNCTION();
    if(!DebugState->IsInitialized) {
        
        DebugState->FirstThread = 0;
        DebugState->FirstFreeThread = 0;
        DebugState->FirstFreeBlock = 0;
        DebugState->FrameBarLaneCount = 0;
        
        DebugState->FrameCount = 0;
        DebugState->TotalFrameCount = 0;
        DebugState->OldestFrame = 0;
        DebugState->LatestFrame = 0;
        DebugState->FirstFreeFrame = 0;
        DebugState->CollationFrame = 0;
        
        memory_index TotalMemorySize = DebugGlobalMemory->DebugStorageSize - sizeof(debug_state);
        InitializeArena(&DebugState->DebugArena, TotalMemorySize, DebugState + 1);
        SubArena(&DebugState->PerFrameArena, &DebugState->DebugArena, TotalMemorySize / 2);
        
        DebugState->RootGroup = CreateVariableGroup(DebugState, "root", 4);
        
#if 0        
        debug_variable_definition_context Context = {};
        Context.State = DebugState;
        Context.Arena = &DebugState->DebugArena;
        Context.GroupStack[0] = 0;
        Context.GroupDepth = 0;
        
        DebugState->RootGroup = DEBUGBeginVariableGroup(&Context, "root");
        DEBUGBeginVariableGroup(&Context, "Begin Debug");
        {
            DEBUGCreateVariables(&Context);
            {
                DEBUGBeginVariableGroup(&Context, "View");
                {
                    DEBUGBeginVariableGroup(&Context, "by Thread");
                    debug_variable *ThreadList = 
                        DEBUGPushVariable(&Context, "", DebugVariableType_CounterThreadList);
                    // debug_view *View = GetDebugViewFor(DebugState, );
                    //View->InlineBlock.Dim = v2{1024.0f, 100.0f};
                }
                DEBUGEndVariableGroup(&Context);
                DEBUGBeginVariableGroup(&Context, "by Function");
                {
                    debug_variable *FunctionList = 
                        DEBUGPushVariable(&Context, "", DebugVariableType_CounterThreadList);
                    //debug_view *View = GetDebugViewFor(DebugState, FunctionList);
                    //View->InlineBlock.Dim = v2{1024.0f, 200.0f};
                }
                DEBUGEndVariableGroup(&Context);
                
                asset_vector MatchVector = {};
                MatchVector.E[Tag_FaceDirection] = 0.0f;
                asset_vector WeightVector = {};
                WeightVector.E[Tag_FaceDirection] = 1.0f;
                
                bitmap_id ID = GetBestMatchBitmapFrom(Assets, Asset_Head, &MatchVector, &WeightVector);
                DebugAddVariable(&Context, "", ID);
                
            }
            DEBUGEndVariableGroup(&Context);
        }
        DEBUGEndVariableGroup(&Context);
        DEBUGEndVariableGroup(&Context);
        Assert(Context.GroupDepth == 0);
#endif
        
        
        DebugState->HighPriorityQueue = DebugGlobalMemory->HighPriorityQueue;
        DebugState->RenderGroup = AllocateRenderGroup(Assets, &DebugState->DebugArena, Megabytes(16), false);
        
        DebugState->IsInitialized = true;
        DebugState->Paused = false;
        DebugState->RecordToScope = 0;
        
        DLIST_INIT(&DebugState->TreeSentinal);
        DEBUGAddTree(DebugState, DebugState->RootGroup, v2{-0.5f * (r32)Width, 0.5f * (r32)Height});
    }
    BeginRender(DebugState->RenderGroup);
    
    DebugState->GlobalWidth = (r32)Width;
    DebugState->GlobalHeight = (r32)Height;
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_FontType] = 1.0f;
    MatchVector.E[Tag_FontType] = (r32)FontType_Debug;
    
    Orthographic(DebugState->RenderGroup, Width, Height, 1.0f);
    
    DebugState->FontID = GetBestMatchFontFrom(Assets, Asset_Font, &MatchVector, &WeightVector);
    DebugState->FontScale = 1.0f;
    DebugState->Font = PushFont(DebugState->RenderGroup, DebugState->FontID);
    DebugState->FontInfo = GetFontInfo(DebugState->RenderGroup->Assets, DebugState->FontID);
    
    DebugState->LeftEdge = -0.5f * (r32)Width;
    DebugState->RightEdge = 0.5f * (r32)Width;
    DebugState->AtY = 0.5f * Height;
}

internal void
DEBUGEnd(debug_state* DebugState, game_input* Input, loaded_bitmap* DrawBuffer) {
    TIMED_FUNCTION();
    
    render_group* RenderGroup = DebugState->RenderGroup;
    
    debug_event* HotEvent = 0;
    // mouse Position
    v2 MouseP = Unproject(DebugState->RenderGroup, V2(Input->MouseX, Input->MouseY)).xy;
    
    DEBUGDrawMainMenu(DebugState, DebugState->RenderGroup, MouseP);
    DEBUGInteract(DebugState, Input, MouseP);
    
    if (DebugState->Compiling) {
        debug_executing_state State = Platform.DEBUGGetProcessState(DebugState->Compiler);
        if (State.IsRunning) {
            DEBUGTextLine("Compiling");
        } else {
            DebugState->Compiling = false;
        }
    }
    
    loaded_font* Font = DebugState->Font;
    hha_font *Info = DebugState->FontInfo;
    
    
    if (Font) {
        
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
        if (DebugState->LatestFrame) {
            char TextBuffer[256];
            _snprintf_s(TextBuffer, sizeof(TextBuffer), 
                        "last frame time %.02fms, MonseX - %f, MouseY - %f",
                        DebugState->LatestFrame->WallSecondsElapsed * 1000.0f,
                        MouseP.x, MouseP.y);
            DEBUGTextLine(TextBuffer);
            
            _snprintf_s(TextBuffer, sizeof(TextBuffer),
                        "per frame arena remaining %ukb",
                        (u32)GetArenaSizeRemaining(&DebugState->PerFrameArena, 1) / 1024);
            DEBUGTextLine(TextBuffer);
        }
    }
    if (WasPressed(&Input->MouseButtons[PlatformMouseButton_Left])) {
        if (HotEvent) {
            DebugState->RecordToScope = HotEvent->BlockName;
        } else {
            DebugState->RecordToScope = 0;
        }
    }
    
    TiledRenderGroupToOutput(DebugState->HighPriorityQueue, DebugState->RenderGroup, DrawBuffer);
    EndRender(DebugState->RenderGroup);
    
    ZeroStruct(DebugState->NextHotInteraction);
}

extern "C" DEBUG_FRAME_END(DEBUGGameFrameEnd) {
    
    GlobalDebugTable->CurrentEventArrayIndex = !GlobalDebugTable->CurrentEventArrayIndex;
    
    // flip value
    u64 ArrayIndex_EventIndex = AtomicExchangeU64(&GlobalDebugTable->EventArrayIndex_EventIndex, (u64)GlobalDebugTable->CurrentEventArrayIndex << 32);
    
    u32 EventArrayIndex = ArrayIndex_EventIndex >> 32;
    Assert(EventArrayIndex <= 1);
    u32 EventCount = ArrayIndex_EventIndex & 0xFFFFFFFF;
    
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    if (DebugState) {
        game_assets *Assets = DEBUGGetGameAsset(Memory);
        DEBUGStart(DebugState, Assets, Buffer->Width, Buffer->Height);
        CollateDebugRecords(DebugState, EventCount, GlobalDebugTable->Events[EventArrayIndex]);
        
        loaded_bitmap DrawBuffer = {};
        DrawBuffer.Height = SafeTruncateToUInt16(Buffer->Height);
        DrawBuffer.Width = SafeTruncateToUInt16(Buffer->Width);
        DrawBuffer.Pitch = SafeTruncateToUInt16(Buffer->Pitch);
        DrawBuffer.Memory = Buffer->Memory;
        DEBUGEnd(DebugState, Input, &DrawBuffer);
    }
    
    return(GlobalDebugTable);
}