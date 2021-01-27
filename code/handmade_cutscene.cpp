internal void
RenderLayerScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, layered_scene *Scene, r32 tNormal) {
    r32 FocalLength = 0.6f;
    if (RenderGroup) {
        Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, 0);
    }
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;
    
    MatchVector.E[Tag_ShotIndex] = (r32)Scene->ShotIndex;
    
    v3 CameraStart = Scene->CameraStart;
    v3 CameraEnd = Scene->CameraEnd;
    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
    if (Scene->LayerCount == 0) {
        Clear(RenderGroup, v4{0, 0, 0, 0});
    }
    for(u32 LayerIndex = 1; LayerIndex <= Scene->LayerCount; ++LayerIndex) {
        scene_layer *Layer = Scene->Layers + (LayerIndex - 1);
        b32 Active = true;
        if ((Layer->Flags & SceneLayerFlag_Transient)) {
            Active = (tNormal >= Layer->Params.x && tNormal < Layer->Params.y);
        }
        if (Active) {
            MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;
            bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Scene->AssetType, &MatchVector, &WeightVector);
            if (RenderGroup) {
                object_transform Transform = DefaultFlatTransform();
                v3 P = Layer->P;
                if (Layer->Flags & SceneLayerFlag_AtInfinity) {
                    P.z += CameraOffset.z;
                }
                
                if (Layer->Flags & SceneLayerFlag_CounterCameraX) {
                    Transform.OffsetP.x = P.x + CameraOffset.x;
                } else {
                    Transform.OffsetP.x = P.x - CameraOffset.x;
                }
                
                if (Layer->Flags & SceneLayerFlag_CounterCameraY) {
                    Transform.OffsetP.y = P.y + CameraOffset.y;
                } else {
                    Transform.OffsetP.y = P.y - CameraOffset.y;
                }
                
                Transform.OffsetP.z = P.z - CameraOffset.z;
                
                PushBitmap(RenderGroup, Transform, LayerImage, Layer->Height, v3{ 0, 0, 0 });
            } else {
                PrefetchBitmap(Assets, LayerImage);
            }
        }
    }
    
}

global_variable scene_layer IntroLayers1[] = {
    {{0.0f, 0.0f, -100.0f}, 150.0f, SceneLayerFlag_AtInfinity},
    {{0.0f, 0.0f, -60.0f}, 110.0f},
    {{0.0f, 0.0f, -40.0f}, 60.0f},
    {{0.0f, 0.0f, -38.0f}, 58.0f},
    {{0.0f, -2.0f, -30.0f}, 50.0f},
    
    {{30.0f, 0.0f, -20.0f}, 30.0f},
    {{0.0f, -2.0f, -19.0f}, 40.0f},
    {{2.0f, -1.0f, -5.0f}, 25.0f},
};

global_variable scene_layer IntroLayers2[] = {
    {{0.0f, 0.0f, -10.0f}, 15.0f},
    {{0.0f, 0.0f, -8.0f}, 13.0f},
    {{0.0f, 0.0f, -3.0f}, 4.5f},
};

global_variable scene_layer IntroLayers3[] = {
    {{0.0f, 0.0f, -50.0f}, 150.0f, SceneLayerFlag_CounterCameraY},
    {{0.0f, -10.0f, -30.0f}, 80.0f, SceneLayerFlag_CounterCameraY},
    {{0.0f, 0.0f, -4.0f}, 20.0f, SceneLayerFlag_CounterCameraY},
    {{0.0f, -4.0f, -3.0f}, 7.0f},
};

global_variable scene_layer IntroLayers4[] = {
    {{0.0f, 0.0f, -3.0f}, 5.0f},
    {{-0.8f, -0.4f, -3.0f}, 3.0f, SceneLayerFlag_Transient, {0.0f, 0.5}},
    {{-0.8f, -0.4f, -3.0f}, 3.0f, SceneLayerFlag_Transient, {0.5, 1.0f}},
    {{3.0f, -1.4f, -3.0f}, 1.5f},
    {{0.0f, 0.2f, -1.0f}, 1.0f},
};
#define CUT_SCENE_WARMUP_TIME 1
#define INTRO_SCENE(Index) Asset_OpeningCutScene, ArrayCount(IntroLayers##Index), Index, IntroLayers##Index
global_variable layered_scene IntroCutScenes[] = {
    {Asset_None, 0, 0, 0, CUT_SCENE_WARMUP_TIME},
    {INTRO_SCENE(1), 20.0f, {0.0f, 0.0f, 10.0f}, {-4.0f, -5.0f, 5.0f}},
    {INTRO_SCENE(2), 20.0f, {0.0f, 0.0f, 0.0f}, {0.5f, 0.5f, -1.5f}},
    {INTRO_SCENE(3), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 6.0f, 0.0f}},
    {INTRO_SCENE(4), 20.0f, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -0.5f}},
};

internal b32
RenderCutSceneAtTime(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, game_mode_cutscene *CutScene, r32 tCutScene) {
    b32 CutSceneRunning = false;
    r32 tBase = 0.0f;
    for (u32 ShotIndex = 0; ShotIndex < CutScene->SceneCount; ++ShotIndex) {
        layered_scene *Scene = CutScene->Scenes + ShotIndex;
        r32 tStart = tBase;
        r32 tEnd = tBase + Scene->Duration;
        if (tCutScene >= tStart && tCutScene < tEnd) {
            r32 tNormal = Clamp01MapToRange(tStart, CutScene->t, tEnd);
            RenderLayerScene(Assets, RenderGroup, DrawBuffer, Scene, tNormal);
            CutSceneRunning= true;
            break;
        }
        tBase = tEnd;
    }
    return(CutSceneRunning);
}

internal void
PlayIntroCutScene(game_state *GameState, transient_state *TranState) {
    SetGameMode(GameState, TranState, GameMode_CutScrene);
    game_mode_cutscene *Result = PushStruct(&GameState->ModeArena, game_mode_cutscene);
    Result->SceneCount = ArrayCount(IntroCutScenes);
    Result->Scenes = IntroCutScenes;
    Result->t = 0;
    GameState->CutScene = Result;
}

internal void
PlayTitleScreen(game_state *GameState, transient_state *TranState) {
    SetGameMode(GameState, TranState, GameMode_TitleScreen);
    game_mode_titlescreen *Result = PushStruct(&GameState->ModeArena, game_mode_titlescreen);
    Result->t = 0;
    GameState->TitleScreen = Result;
}

internal b32
UpdateAndRenderTitleScreen(game_state *GameState, transient_state *TranState, render_group *RenderGroup, loaded_bitmap *DrawBuffer, game_mode_titlescreen *TitleScreen, game_input *Input) {
    game_assets *Assets = TranState->Assets;
    b32 Rerun = CheckForMetaInput(GameState, TranState, Input);
    Clear(RenderGroup, v4{1,0,1,1});
    if (!Rerun) {
        TitleScreen->t += Input->dtForFrame;
        if (TitleScreen->t >= 10.0f) {
            PlayIntroCutScene(GameState, TranState);
        }
    }
    return(Rerun);
}

internal b32
UpdateAndRenderCutScene(game_state *GameState, transient_state *TranState, render_group *RenderGroup, loaded_bitmap *DrawBuffer, game_mode_cutscene *CutScene, game_input *Input) {
    game_assets *Assets = TranState->Assets;
    b32 Rerun = CheckForMetaInput(GameState, TranState, Input);
    if (!Rerun) {
        RenderCutSceneAtTime(Assets, 0, DrawBuffer, CutScene, CutScene->t + CUT_SCENE_WARMUP_TIME);
        b32 CutSceneRunning = RenderCutSceneAtTime(Assets, RenderGroup, DrawBuffer, CutScene, CutScene->t);
        if (CutSceneRunning) {
            // loop the cutscene
            //CutScene->t = 0;
            CutScene->t += Input->dtForFrame;
        } else {
            PlayTitleScreen(GameState, TranState);
        }
        
    }
    return(Rerun);
}