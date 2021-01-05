/* date = January 1st 2021 8:47 pm */

#ifndef HANDMADE_DEBUG_VARIABLES_H
#define HANDMADE_DEBUG_VARIABLES_H

struct debug_variable_definition_context {
    debug_state* State;
    memory_arena* Arena;
    debug_variable_reference* Group;
};

internal debug_variable*
DEBUGPushUnreferencedVariable(debug_state *DebugState, char* Name, debug_variable_type Type) {
    debug_variable* Result = PushStruct(&DebugState->DebugArena, debug_variable);
    Result->Name = (char*)PushCopy(&DebugState->DebugArena, StringLength(Name) + 1, Name);
    Result->Type = Type;
    
    return(Result);
}

internal debug_variable_reference*
DEBUGAddVariableReference(debug_state* State, debug_variable_reference* GroupRef, debug_variable* Var) {
    debug_variable_reference* Ref = PushStruct(&State->DebugArena, debug_variable_reference);
    Ref->Var = Var;
    Ref->Next = 0;
    Ref->Parent = GroupRef;
    debug_variable *Group = 0;
    if (Ref->Parent) {
        Group = Ref->Parent->Var;
    }
    if (Group) {
        if (Group->Group.FirstChild) {
            Group->Group.LastChild->Next = Ref;
            Group->Group.LastChild = Ref;
        } else {
            Group->Group.FirstChild = Group->Group.LastChild = Ref;
        }
    }
    
    return(Ref);
}
internal debug_variable_reference*
DEBUGAddVariableReference(debug_variable_definition_context *Context, debug_variable* Var) {
    debug_variable_reference* Ref = DEBUGAddVariableReference(Context->State, Context->Group, Var);
    return(Ref);
}

internal debug_variable_reference*
DEBUGPushVariable(debug_variable_definition_context *Context, char* Name, debug_variable_type Type) {
    debug_variable* Var = DEBUGPushUnreferencedVariable(Context->State, Name, Type);
    debug_variable_reference* Ref = DEBUGAddVariableReference(Context, Var);
    
    return(Ref);
}

internal debug_variable*
DEBUGAddRootGroupInternal(debug_state* State, char* GroupName) {
    debug_variable* Group = DEBUGPushUnreferencedVariable(State, GroupName, DebugVariableType_Group);
    
    Group->Group.FirstChild = Group->Group.LastChild = 0;
    Group->Group.Expanded = false;
    return(Group);
}
internal debug_variable_reference*
DEBUGAddRootGroup(debug_state* State, char* GroupName) {
    debug_variable* RootGroup = DEBUGAddRootGroupInternal(State, GroupName);
    debug_variable_reference* Ref = DEBUGAddVariableReference(State, 0, RootGroup);
    return(Ref);
}

internal debug_variable_reference*
DEBUGBeginVariableGroup(debug_variable_definition_context *Context, char* GroupName) {
    debug_variable* DebugGroup = DEBUGAddRootGroupInternal(Context->State, GroupName);
    debug_variable_reference *DebugGroupRef = DEBUGAddVariableReference(Context, DebugGroup);
    Context->Group = DebugGroupRef;
    return(DebugGroupRef);
}

internal void
DEBUGEndVariableGroup(debug_variable_definition_context *Context) {
    Assert(Context->Group);
    Context->Group = Context->Group->Parent;
}

internal debug_variable_reference*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, b32 Value) {
    debug_variable_reference* Ref = DEBUGPushVariable(Context, Name, DebugVariableType_Bool32);
    Ref->Var->Bool32 = Value;
    return(Ref);
}

internal debug_variable_reference*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, u32 Value) {
    debug_variable_reference *Ref = DEBUGPushVariable(Context, Name, DebugVariableType_UInt32);
    Ref->Var->UInt32 = Value;
    return(Ref);
}

internal debug_variable_reference*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, r32 Value) {
    debug_variable_reference *Ref = DEBUGPushVariable(Context, Name, DebugVariableType_Real32);
    Ref->Var->Real32 = Value;
    return(Ref);
}

internal debug_variable_reference*
DebugAddVariable(debug_variable_definition_context *Context, char* Name, v4 Value) {
    debug_variable_reference *Ref = DEBUGPushVariable(Context, Name, DebugVariableType_V4);
    Ref->Var->Vector4 = Value;
    return(Ref);
}

internal void
DEBUGCreateVariables(debug_variable_definition_context* Context) {
    debug_variable_reference *UsedDebugCamRef = 0;
    
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
            UsedDebugCamRef = DEBUG_VARIABLE_LISITING(UseDebugCamera);
            DEBUG_VARIABLE_LISITING(RoomBaseCamera);
            DEBUG_VARIABLE_LISITING(DebugCameraDistance);
        }
        DEBUGEndVariableGroup(Context);
    }
    DEBUGEndVariableGroup(Context);
    
    DEBUG_VARIABLE_LISITING(FamiliarFollowsHero);
    DEBUG_VARIABLE_LISITING(ShowSpaceOutline);
    DEBUG_VARIABLE_LISITING(FauxV4);
    
    DEBUGAddVariableReference(Context, UsedDebugCamRef->Var);
    
#undef DEBUG_VARIABLE_LISITING
}

#endif //HANDMADE_DEBUG_VARIABLES_H