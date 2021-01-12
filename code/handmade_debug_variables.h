/* date = January 1st 2021 8:47 pm */

#ifndef HANDMADE_DEBUG_VARIABLES_H
#define HANDMADE_DEBUG_VARIABLES_H
#define DEBUG_MAX_VARIABLE_STACK_SIZE 64
#if 0

struct debug_variable_definition_context {
    debug_state* State;
    memory_arena* Arena;
    u32 GroupDepth;
    debug_variable* GroupStack[DEBUG_MAX_VARIABLE_STACK_SIZE];
};

internal debug_variable*
DEBUGPushVariable(debug_state *DebugState, char* Name, debug_variable_type Type) {
    debug_variable* Result = PushStruct(&DebugState->DebugArena, debug_variable);
    Result->Name = (char*)PushCopy(&DebugState->DebugArena, StringLength(Name) + 1, Name);
    Result->Type = Type;
    return(Result);
}

internal void
DEBUGPushVariableToGroup(debug_state* DebugState, debug_variable *Group, debug_variable* Add) {
    debug_variable_link *Link = PushStruct(&DebugState->DebugArena, debug_variable_link);
    Link->Var = Add;
    DLIST_INSERT(&Group->VarGroup, Link);
}

internal void
DEBUGPushVariableToDefaultGroup(debug_variable_definition_context* Context, debug_variable* Var) {
    debug_variable* Parent = Context->GroupStack[Context->GroupDepth];
    if (Parent) {
        DEBUGPushVariableToGroup(Context->State, Parent, Var);
    }
}

internal debug_variable*
DEBUGPushVariable(debug_variable_definition_context *Context, char* Name, debug_variable_type Type) {
    debug_variable* Var = DEBUGPushVariable(Context->State, Name, Type);
    DEBUGPushVariableToDefaultGroup(Context, Var);
    return(Var);
}

internal debug_variable*
DEBUGAddRootGroup(debug_state* DebugState, char* GroupName) {
    debug_variable *Group = DEBUGPushVariable(DebugState, GroupName, DebugVariableType_VarGroup);
    DLIST_INIT(&Group->VarGroup);
    return(Group);
}

internal debug_variable*
DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char* GroupName) {
    debug_variable *Group = DEBUGAddRootGroup(Context->State, GroupName);
    DEBUGPushVariableToDefaultGroup(Context, Group);
    Assert(Context->GroupDepth < (ArrayCount(Context->GroupStack) - 1));
    Context->GroupStack[++Context->GroupDepth] = Group;
    return(Group);
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context *Context) {
    Assert(Context->GroupDepth > 0);
    --Context->GroupDepth;
}

internal debug_variable*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, b32 Value) {
    debug_variable* Var = DEBUGPushVariable(Context, Name, DebugVariableType_Event);
    Var->Bool32 = Value;
    return(Var);
}

internal debug_variable*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, u32 Value) {
    debug_variable *Var = DEBUGPushVariable(Context, Name, DebugVariableType_Event);
    Var->UInt32 = Value;
    return(Var);
}

internal debug_variable*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, r32 Value) {
    debug_variable *Var = DEBUGPushVariable(Context, Name, DebugVariableType_Event);
    Var->Real32 = Value;
    return(Var);
}

internal debug_variable*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, v4 Value) {
    debug_variable *Var = DEBUGPushVariable(Context, Name, DebugVariableType_Event);
    Var->Vector4 = Value;
    return(Var);
}

internal debug_variable*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, bitmap_id Value) {
    debug_variable *Var = DEBUGPushVariable(Context, Name, DebugVariableType_BitmapDiplay);
    Var->BitmapDisplay.ID = Value;
    return(Var);
}

internal void
DEBUGCreateVariables(debug_variable_definition_context* Context) {
#define DEBUG_VARIABLE_LISITING(Name) DebugAddVariable(Context, #Name, DEBUGUI_##Name) // DebugVariableType_Boolean, #name, name
    
    DEBUGBeginVariableGroup(Context, "GroundChunk");
    {
        DEBUG_VARIABLE_LISITING(ShowGroundChunkOutlines);
        DEBUG_VARIABLE_LISITING(ReGenGroundChunkOnReload);
    }
    
    DEBUGEndVariableGroup(Context);
    
    DEBUGBeginVariableGroup(Context, "Particle");
    {
        DEBUG_VARIABLE_LISITING(ParticleDemo);
        DEBUG_VARIABLE_LISITING(ParticleGrid);
    }
    DEBUGEndVariableGroup(Context);
    
    DEBUGBeginVariableGroup(Context, "Renderer");
    {
        DEBUG_VARIABLE_LISITING(WeirdDrawBufferSize);
        DEBUG_VARIABLE_LISITING(ShowLightningSampleSource);
        DEBUGBeginVariableGroup(Context, "Camera");
        {
            DEBUG_VARIABLE_LISITING(UseDebugCamera);
            DEBUG_VARIABLE_LISITING(RoomBaseCamera);
            DEBUG_VARIABLE_LISITING(DebugCameraDistance);
        }
        DEBUGEndVariableGroup(Context);
        
    }
    DEBUGEndVariableGroup(Context);
    
    DEBUG_VARIABLE_LISITING(FamiliarFollowsHero);
    DEBUG_VARIABLE_LISITING(ShowSpaceOutline);
    DEBUG_VARIABLE_LISITING(ShowEntityOutline);
    DEBUG_VARIABLE_LISITING(FauxV4);
    
    
#undef DEBUG_VARIABLE_LISITING
}
#endif

#endif //HANDMADE_DEBUG_VARIABLES_H