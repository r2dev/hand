/* date = February 17th 2021 10:34 pm */

#ifndef HANDMADE_BRAIN_H
#define HANDMADE_BRAIN_H

struct entity;

enum brain_type {
    Brain_Hero,
    Brain_Snake,
    Brain_Familiar,
    Brain_FloatyThingForNow,
    Brain_Monstar,
    Brain_Count,
};

struct brain_slot {
    u32 Index;
};

struct brain_id {
    u32 Value;
};

struct brain_hero_parts {
    entity *Head;
    entity *Body;
};

#define BrainSlotFor(Type, Member) BrainSlotFor_((&(((Type *)0)->Member)) - (entity **)0)

inline brain_slot
BrainSlotFor_(u32 PackValue) {
    brain_slot Result = {PackValue};
    return(Result);
}

struct brain {
    brain_type Type;
    brain_id ID;
    union {
        brain_hero_parts Hero;
        entity *Array[16];
    };
};

enum reserved_brain_id {
    ReservedBrainID_FirstControl = 1,
    ReservedBrainID_LastControl = (ReservedBrainID_FirstControl + MAX_CONTROLLER_COUNT - 1),
    ReservedBrainID_FirstFree,
};


#endif //HANDMADE_BRAIN_H
