/* date = January 10th 2021 10:19 am */

#ifndef HANDMADE_META_H
#define HANDMADE_META_H

enum meta_type {
    MetaType_uint32,
    MetaType_uint8,
    MetaType_move_spec,
    MetaType_u32,
    MetaType_bool32,
    MetaType_b32,
    MetaType_s32,
    MetaType_r32,
    MetaType_entity_type,
    MetaType_v3,
    MetaType_real32,
    MetaType_sim_entity_collision_volume_group,
    MetaType_sim_entity_collision_volume,
    MetaType_int32,
    MetaType_hit_point,
    MetaType_entity_reference,
    MetaType_v2,
    MetaType_world,
    MetaType_world_position,
    MetaType_rectangle3,
    MetaType_sim_entity,
    MetaType_sim_entity_hash,
    MetaType_rectangle2,
    MetaType_sim_region,
    
};

enum member_definition_flag {
    MetaMemberFlag_IsPointer = 0x1,
};
struct member_definition {
    u32 Flags;
    meta_type Type;
    char* Name;
    u32 Offset;
};



#endif //HANDMADE_META_H
