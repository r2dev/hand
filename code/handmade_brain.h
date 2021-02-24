/* date = February 17th 2021 10:34 pm */

#ifndef HANDMADE_BRAIN_H
#define HANDMADE_BRAIN_H

struct entity;

//////////////////
struct brain_hero {
    entity *Head;
    entity *Body;
};

struct brain_monster {
    entity *Body;
};

struct brain_familiar {
    entity *Head;
};

struct brain_snake {
    entity *Segments[8];
};


///////////////////
enum brain_type {
    Type_brain_none,
    Type_brain_hero,
    Type_brain_snake,
    Type_brain_familiar,
    Type_brain_floaty_thing_for_now,
    Type_brain_monster,
    Type_brain_count,
};

struct brain_slot {
    u16 Type;
    u16 Index;
};

struct brain_id {
    u32 Value;
};


#define BrainSlotFor(type, Member) BrainSlotFor_(Type_##type, (&(((type *)0)->Member)) - (entity **)0)
#define IndexedBrainSlotFor(type, Member, Index) BrainSlotFor_(Type_##type, (u16)(Index) + (u16)((((type *)0)->Member) - (entity **)0))

inline brain_slot
BrainSlotFor_(brain_type Type, u16 PackValue) {
    brain_slot Result = {(u16)Type, PackValue};
    return(Result);
}

inline b32
IsType(brain_slot Slot, brain_type Type) {
    // TODO(not-set): why is checking index non zero here
    b32 Result = (Slot.Type == Type);
    return(Result);
}

struct brain {
    brain_id ID;
    brain_type Type;
    union {
        brain_hero Hero;
        brain_monster Monster;
        brain_familiar Familiar;
        brain_snake Snake;
        entity *Array;
    };
};

enum reserved_brain_id {
    ReservedBrainID_FirstControl = 1,
    ReservedBrainID_LastControl = (ReservedBrainID_FirstControl + MAX_CONTROLLER_COUNT - 1),
    ReservedBrainID_FirstFree,
};


#endif //HANDMADE_BRAIN_H
