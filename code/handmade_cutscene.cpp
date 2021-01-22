#include "handmade_cutscene.h"
internal void
RenderLayerScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, layered_scene *Scene, r32 tNormal) {
    r32 FocalLength = 0.6f;
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, 0);
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;
    
    MatchVector.E[Tag_ShotIndex] = (r32)Scene->ShotIndex;
    
    v3 CameraStart = Scene->CameraStart;
    v3 CameraEnd = Scene->CameraEnd;
    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
    
    for(u32 LayerIndex = 1; LayerIndex <= Scene->LayerCount; ++LayerIndex) {
        scene_layer *Layer = Scene->Layers + (LayerIndex - 1);
        b32 Active = true;
        if ((Layer->Flags & SceneLayerFlag_Transient)) {
            Active = (tNormal >= Layer->Params.x && tNormal < Layer->Params.y);
        }
        
        
        if (Active) {
            v3 P = Layer->P;
            if (Layer->Flags & SceneLayerFlag_AtInfinity) {
                P.z += CameraOffset.z;
            }
            
            if (Layer->Flags & SceneLayerFlag_CounterCameraX) {
                RenderGroup->Transform.OffsetP.x = P.x + CameraOffset.x;
            } else {
                RenderGroup->Transform.OffsetP.x = P.x - CameraOffset.x;
            }
            
            if (Layer->Flags & SceneLayerFlag_CounterCameraY) {
                RenderGroup->Transform.OffsetP.y = P.y + CameraOffset.y;
            } else {
                RenderGroup->Transform.OffsetP.y = P.y - CameraOffset.y;
            }
            
            RenderGroup->Transform.OffsetP.z = P.z - CameraOffset.z;
            MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;
            bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Scene->AssetType, &MatchVector, &WeightVector);
            PushBitmap(RenderGroup, LayerImage, Layer->Height, v3{ 0, 0, 0 });
        }
    }
    
}

internal void
RenderCutScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, r32 tCutScene) {
    r32 tStart = 0.0f;
    r32 tEnd = 20.0f;
    r32 tNormal =  Clamp01MapToRange(tStart, tCutScene, tEnd);
    if (0)
    {
        layered_scene Scene = {};
        Scene.LayerCount = 8;
        Scene.ShotIndex = 1;
        scene_layer Layers[] = {
            {{0.0f, 0.0f, -100.0f}, 150.0f, SceneLayerFlag_AtInfinity},
            {{0.0f, 0.0f, -60.0f}, 110.0f},
            {{0.0f, 0.0f, -40.0f}, 60.0f},
            {{0.0f, 0.0f, -38.0f}, 58.0f},
            {{0.0f, -2.0f, -30.0f}, 50.0f},
            
            {{30.0f, 0.0f, -20.0f}, 30.0f},
            {{0.0f, -2.0f, -19.0f}, 40.0f},
            {{2.0f, -1.0f, -5.0f}, 25.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 10.0f};
        Scene.CameraEnd = {-4.0f, -5.0f, 5.0f};
        Scene.AssetType = Asset_OpeningCutScene;
        
        RenderLayerScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
    }
    
    if (0)
    {
        layered_scene Scene = {};
        Scene.LayerCount = 3;
        Scene.ShotIndex = 2;
        scene_layer Layers[] = {
            {{0.0f, 0.0f, -10.0f}, 15.0f},
            {{0.0f, 0.0f, -8.0f}, 13.0f},
            {{0.0f, 0.0f, -3.0f}, 4.5f},
            
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.5f, 0.5f, -1.5f};
        Scene.AssetType = Asset_OpeningCutScene;
        
        RenderLayerScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
    }
    
    if (0){
        layered_scene Scene = {};
        Scene.LayerCount = 4;
        Scene.ShotIndex = 3;
        scene_layer Layers[] = {
            {{0.0f, 0.0f, -50.0f}, 150.0f, SceneLayerFlag_CounterCameraY},
            {{0.0f, -10.0f, -30.0f}, 80.0f, SceneLayerFlag_CounterCameraY},
            {{0.0f, 0.0f, -4.0f}, 20.0f, SceneLayerFlag_CounterCameraY},
            {{0.0f, -4.0f, -3.0f}, 7.0f},
            
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.0f, 6.0f, 0.0f};
        Scene.AssetType = Asset_OpeningCutScene;
        
        RenderLayerScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
    }
    
    {
        layered_scene Scene = {};
        Scene.LayerCount = 5;
        Scene.ShotIndex = 4;
        r32 ShotChangeTime = 0.5f;
        scene_layer Layers[] = {
            {{0.0f, 0.0f, -3.0f}, 5.0f},
            {{-0.8f, -0.4f, -3.0f}, 3.0f, SceneLayerFlag_Transient, {0.0f, ShotChangeTime}},
            {{-0.8f, -0.4f, -3.0f}, 3.0f, SceneLayerFlag_Transient, {ShotChangeTime, 1.0f}},
            {{3.0f, -1.4f, -3.0f}, 1.5f},
            {{0.0f, 0.2f, -1.0f}, 1.0f},
        };
        Scene.Layers = Layers;
        Scene.CameraStart = {0.0f, 0.0f, 0.0f};
        Scene.CameraEnd = {0.0f, 0.0f, -0.5f};
        Scene.AssetType = Asset_OpeningCutScene;
        
        RenderLayerScene(Assets, RenderGroup, DrawBuffer, &Scene, tNormal);
    }
    
    
    
    
}