internal void
RenderCutScene(game_assets *Assets, render_group *RenderGroup, loaded_bitmap *DrawBuffer, r32 tCutScene) {
    r32 tStart = 0.0f;
    r32 tEnd = 5.0f;
    r32 tNormal =  Clamp01MapToRange(tStart, tCutScene, tEnd);
    
    r32 FocalLength = 0.6f;
	r32 DistanceAboveTarget = 10.0f - tNormal * 3.0f;
    Perspective(RenderGroup, DrawBuffer->Width, DrawBuffer->Height, FocalLength, DistanceAboveTarget);
    
    asset_vector MatchVector = {};
    asset_vector WeightVector = {};
    WeightVector.E[Tag_ShotIndex] = 10.0f;
    WeightVector.E[Tag_LayerIndex] = 1.0f;
    
    u32 ShotIndex = 1;
    MatchVector.E[Tag_ShotIndex] = (r32)ShotIndex;
    
    v3 CameraStart = {0.0f, 0.0f, 0.0f};
    v3 CameraEnd = {-8.0f, -5.0f, -10.0f};
    v3 CameraOffset = Lerp(CameraStart, tNormal, CameraEnd);
    
    v4 LayerPlacements[] = {
        {0.0f, 0.0f, -200.0f, 300.0f},
        {0.0f, 0.0f, -170.0f, 300.0f},
        {0.0f, 0.0f, -100.0f, 40.0f},
        {0.0f, 10.0f, -70.0f, 80.0f},
        {0.0f, 0.0f, -50.0f, 70.0f},
        {30.0f, 0.0f, -30.0f, 50.0f},
        {0.0f, -2.0f, -20.0f, 40.0f},
        {2.0f, -1.0f, -5.0f, 25.0f},
        
    };
    for(u32 LayerIndex = 1; LayerIndex <= 8; ++LayerIndex) {
        v4 LayerPlacement = LayerPlacements[LayerIndex - 1];
        RenderGroup->Transform.OffsetP = LayerPlacement.xyz - CameraOffset;
        MatchVector.E[Tag_LayerIndex] = (r32)LayerIndex;
        bitmap_id LayerImage = GetBestMatchBitmapFrom(Assets, Asset_OpeningCutScene, &MatchVector, &WeightVector);
        PushBitmap(RenderGroup, LayerImage, LayerPlacement.w, v3{ 0, 0, 0 });
    }
}