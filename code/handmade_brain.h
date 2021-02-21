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

///////////////////
enum brain_type {
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

inline brain_slot
BrainSlotFor_(brain_type Type, u16 PackValue) {
    brain_slot Result = {(u16)Type, PackValue};
    return(Result);
}

struct brain {
    brain_id ID;
    brain_type Type;
    union {
        brain_hero Hero;
        brain_monster Monster;
        brain_familiar Familiar;
        entity *Array[16];
    };
};

enum reserved_brain_id {
    ReservedBrainID_FirstControl = 1,
    ReservedBrainID_LastControl = (ReservedBrainID_FirstControl + MAX_CONTROLLER_COUNT - 1),
    ReservedBrainID_FirstFree,
};


#endif //HANDMADE_BRAIN_H
