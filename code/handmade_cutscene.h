/* date = January 21st 2021 9:55 pm */

#ifndef HANDMADE_CUTSCENE_H
#define HANDMADE_CUTSCENE_H

enum scene_layer_flag {
    SceneLayerFlag_AtInfinity = 0xf,
    SceneLayerFlag_CounterCameraX = 0xf0,
    SceneLayerFlag_CounterCameraY = 0xf00,
    SceneLayerFlag_Transient = 0xf000,
};
struct scene_layer {
    v3 P;
    r32 Height;
    u32 Flags;
    v2 Params;
};

struct layered_scene {
    asset_type_id AssetType;
    u32 LayerCount;
    u32 ShotIndex;
    scene_layer *Layers;
    
    r32 Duration;
    
    v3 CameraStart;
    v3 CameraEnd;
};

struct game_mode_cutscene {
    r32 t;
    u32 SceneCount;
    layered_scene *Scenes;
    
};

struct game_mode_titlescreen {
    r32 t;
};
struct game_state;
internal void PlayIntroCutScene(game_state *GameState);
internal void PlayTitleScreen(game_state *GameState);


#endif //HANDMADE_CUTSCENE_H
