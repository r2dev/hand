/* date = January 1st 2021 8:47 pm */

#ifndef HANDMADE_DEBUG_VARIABLES_H
#define HANDMADE_DEBUG_VARIABLES_H

struct debug_variable_definition_context {
    debug_state* State;
    memory_arena* Arena;
    debug_variable* Group;
};

internal debug_variable*
DEBUGPushVariable(debug_variable_definition_context *Context, char* Name, debug_variable_type Type) {
    debug_variable* Result = PushStruct(Context->Arena, debug_variable);
    Result->Name = (char*)PushCopy(Context->Arena, StringLength(Name) + 1, Name);
    Result->Type = Type;
    Result->Next = 0;
    
    debug_variable *Group = Context->Group;
    Result->Parent = Group;
    
    if (Group) {
        if (Group->Group.FirstChild) {
            Group->Group.LastChild->Next = Result;
            Group->Group.LastChild = Result;
        } else {
            Group->Group.FirstChild = Group->Group.LastChild = Result;
        }
    }
    return(Result);
}

internal void 
DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char* GroupName) {
    debug_variable* DebugGroup = DEBUGPushVariable(Context, GroupName, DebugVariableType_Group);
    DebugGroup->Group.FirstChild = DebugGroup->Group.LastChild = 0;
    DebugGroup->Group.Expanded = false;
    
    Context->Group = DebugGroup;
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context *Context) {
    Assert(Context->Group);
    Context->Group = Context->Group->Parent;
}

internal void
DebugAddVariable(debug_variable_definition_context *Context, char* Name, b32 Value) {
    debug_variable* Var = DEBUGPushVariable(Context, Name, DebugVariableType_Bool32);
    Var->Bool32 = Value;
}

internal void
DebugAddVariable(debug_variable_definition_context *Context, char* Name, u32 Value) {
    debug_variable* Var = DEBUGPushVariable(Context, Name, DebugVariableType_UInt32);
    Var->UInt32 = Value;
}

internal void
DebugAddVariable(debug_variable_definition_context *Context, char* Name, r32 Value) {
    debug_variable* Var = DEBUGPushVariable(Context, Name, DebugVariableType_Real32);
    Var->Real32 = Value;
}

internal void
DebugAddVariable(debug_variable_definition_context *Context, char* Name, v4 Value) {
    debug_variable* Var = DEBUGPushVariable(Context, Name, DebugVariableType_V4);
    Var->Vector4 = Value;
}

internal void
DEBUGCreateVariables(debug_state* DebugState) {
    debug_variable_definition_context Context = {};
    Context.State = DebugState;
    Context.Arena = &DebugState->DebugArena;
    DEBUGBeginVariableGroup(&Context, "Root");
    
#define DEBUG_VARIABLE_LISITING(Name) DebugAddVariable(&Context, #Name, DEBUGUI_##Name) // DebugVariableType_Boolean, #name, name
    DEBUGBeginVariableGroup(&Context, "GroundChunk");
    DEBUG_VARIABLE_LISITING(ShowGroundChunkOutlines);
    DEBUG_VARIABLE_LISITING(ReGenGroundChunkOnReload);
    DEBUGEndVariableGroup(&Context);
    
    
    
    DEBUGBeginVariableGroup(&Context, "Particle");
    DEBUG_VARIABLE_LISITING(ParticleDemo);
    DEBUG_VARIABLE_LISITING(ParticleGrid);
    DEBUGEndVariableGroup(&Context);
    
    DEBUGBeginVariableGroup(&Context, "Renderer");
    {
        DEBUG_VARIABLE_LISITING(WeirdDrawBufferSize);
        DEBUG_VARIABLE_LISITING(ShowLightningSampleSource);
        DEBUGBeginVariableGroup(&Context, "Camera");
        {
            DEBUG_VARIABLE_LISITING(UseDebugCamera);
            DEBUG_VARIABLE_LISITING(RoomBaseCamera);
            DEBUG_VARIABLE_LISITING(DebugCameraDistance);
        }
        DEBUGEndVariableGroup(&Context);
    }
    DEBUGEndVariableGroup(&Context);
    
    DEBUG_VARIABLE_LISITING(FamiliarFollowsHero);
    DEBUG_VARIABLE_LISITING(ShowSpaceOutline);
    DEBUG_VARIABLE_LISITING(FauxV4);
#undef DEBUG_VARIABLE_LISITING
    DebugState->RootGroup = Context.Group;
}

#endif //HANDMADE_DEBUG_VARIABLES_H
