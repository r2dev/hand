internal void
DrawHitPoints(entity* Entity, render_group* PieceGroup, object_transform ObjectTransform) {
	if (Entity->HitPointMax >= 1) {
		v2 HealthDim = { 0.2f, 0.2f };
		r32 SpacingX = 1.5f * HealthDim.x;
		v2 HitP = { -0.5f * (Entity->HitPointMax - 1) * SpacingX, -0.2f };
		v2 dHitP = { SpacingX, 0.0f };
		for (u32 HealthIndex = 0; HealthIndex < Entity->HitPointMax; ++HealthIndex) {
			hit_point* HitPoint = Entity->HitPoint + HealthIndex;
			v4 Color = { 1.0f, 0.0f, 0.0f, 1.0f };
			if (HitPoint->FilledAmount == 0) {
				Color = { 0.2f, 0.2f, 0.2f, 1.0f };
			}
			PushRect(PieceGroup, ObjectTransform, V3(HitP, 0), HealthDim, Color);
			HitP += dHitP;
		}
	}
}

inline s32
ConvertToLayerRelative(game_mode_world *WorldMode, r32 *Z) {
    s32 RelativeIndex = 0;
    RecanonicalizeCoord(WorldMode->TypicalFloorHeight, &RelativeIndex, Z);
    return(RelativeIndex);
}

internal void
UpdateAndRenderEntities(sim_region *SimRegion, game_mode_world *WorldMode, render_group *RenderGroup, v3 CameraP, loaded_bitmap *DrawBuffer, v4 BackgroundColor, r32 dt, transient_state *TranState, v2 MouseP) {
    
    r32 FadeTopEnd = 0.75f * WorldMode->TypicalFloorHeight;
    r32 FadeTopStart = 0.5f * WorldMode->TypicalFloorHeight;
    
    r32 FadeBottomStart = -1.0f * WorldMode->TypicalFloorHeight;
    r32 FadeBottomEnd = -3.25f * WorldMode->TypicalFloorHeight;
    
#define MinimumLevelIndex -4
#define MaximumLevelIndex  1
    
    u32 ClipRectIndex[(MaximumLevelIndex - MinimumLevelIndex + 1)];
    for (u32 LevelIndex = 0; LevelIndex < ArrayCount(ClipRectIndex); ++LevelIndex) {
        s32 RelativeLayerIndex = LevelIndex + MinimumLevelIndex;
        r32 CameraRelativeGroundZ = SimRegion->Origin.Offset_.z + WorldMode->TypicalFloorHeight * (r32)RelativeLayerIndex;
        
        
        clip_rect_fx FX = {};
        
        if (CameraRelativeGroundZ > FadeTopStart) {
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[0];
            r32 t = Clamp01MapToRange(FadeTopStart, CameraRelativeGroundZ, FadeTopEnd);
            
            FX.tColor = v4{0, 0, 0, t};
        } else if (CameraRelativeGroundZ < FadeBottomStart) {
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[1];
            r32 t = Clamp01MapToRange(FadeBottomStart, CameraRelativeGroundZ, FadeBottomEnd);
            
            FX.tColor = v4{t, t, t, 0};
            FX.Color = BackgroundColor;
        } else {
            RenderGroup->CurrentClipRectIndex = ClipRectIndex[2];
            
        }
        
        ClipRectIndex[LevelIndex] = PushClipRect(RenderGroup, 0, 0, DrawBuffer->Width, DrawBuffer->Height, FX);
    }
    transient_clip_rect Rect(RenderGroup);
    for (u32 EntityIndex = 0; EntityIndex < SimRegion->EntityCount; ++EntityIndex) {
        entity* Entity = SimRegion->Entities + EntityIndex;
        if (Entity->Updatable) {
            //hero_bitmaps* HeroBitmap = &GameState->HeroBitmaps[Entity->FacingDirection];
            
            switch (Entity->MovementMode) {
                case MovementMode_Planted: {
                    
                } break;
                case MovementMode_Hopping: {
                    r32 tTotal = Entity->tMovement;
                    r32 tJump = 0.1f;
                    r32 tLand = 0.9f;
                    v3 MovementFrom = GetSimSpaceTraversable(Entity->CameFrom).P;
                    v3 MovementTo = GetSimSpaceTraversable(Entity->Occupying).P;
                    
                    if (tTotal < tJump) {
                        Entity->ddtBob = 45.0f;
                    }
                    
                    if (tTotal < tLand) {
                        r32 t = Clamp01MapToRange(tJump, tTotal, tLand);
                        v3 a = v3{0, -2.0f, 0};
                        v3 b = MovementTo - MovementFrom - a;
                        Entity->P = a * t * t + b * t + MovementFrom;
                    }
                    
                    if (Entity->tMovement >= 1.0f) {
                        Entity->P = MovementTo;
                        Entity->CameFrom = Entity->Occupying;
                        Entity->MovementMode = MovementMode_Planted;
                        Entity->dtBob = -2.0f;
                    }
                    Entity->tMovement += 6.0f * dt;
                    if (Entity->tMovement > 1.0f) {
                        Entity->tMovement = 1.0f;
                    }
                    
                } break;
                case MovementMode_AttackSwipe: {
                    if (Entity->tMovement < 1.0f) {
                        Entity->AngleCurrent = Lerp(Entity->AngleStart, Entity->tMovement, Entity->AngleTarget);
                        
                        Entity->AngleCurrentDistance = Lerp(Entity->AngleBaseDistance, Triangle01(Entity->tMovement), Entity->AngleSwipeDistance);
                    } else {
                        Entity->MovementMode = MovementMode_AngleOffset;
                        Entity->AngleCurrent = Entity->AngleTarget;
                        Entity->AngleCurrentDistance = Entity->AngleBaseDistance;
                    }
                    Entity->tMovement += 1.0f * dt;
                    if (Entity->tMovement > 1.0f) {
                        Entity->tMovement = 1.0f;
                    }
                    
                }
                case MovementMode_AngleOffset: {
                    
                    v2 Arm = Entity->AngleCurrentDistance * Arm2( Entity->AngleCurrent + Entity->FacingDirection);
                    Entity->P = Entity->AngleBase + V3(Arm.x, Arm.y, 0.0f);
                } break;
            }
            r32 Cv = 10.0f;
            Entity->ddtBob += -100.0f * Entity->tBob + Cv * -Entity->dtBob;
            Entity->tBob = Entity->ddtBob * dt * dt + Entity->dtBob*dt;
            Entity->dtBob += Entity->ddtBob * dt;
            
            if (LengthSq(Entity->dP) > 0 ||  LengthSq(Entity->ddP) > 0) {
                MoveEntity(WorldMode, SimRegion, Entity, dt, Entity->ddP);
            }
            
            // rendering
            object_transform EntityTransform = DefaultUprightTransform();
            EntityTransform.OffsetP = GetEntityGroundPoint(Entity) - CameraP;
            s32 RelativeLayer = ConvertToLayerRelative(WorldMode, &EntityTransform.OffsetP.z);
            if (RelativeLayer >= MinimumLevelIndex && RelativeLayer <= MaximumLevelIndex) {
                RenderGroup->CurrentClipRectIndex = ClipRectIndex[RelativeLayer - MinimumLevelIndex];
                asset_vector MatchVector = {};
                MatchVector.E[Tag_FaceDirection] = (r32)Entity->FacingDirection;
                asset_vector WeightVector = {};
                WeightVector.E[Tag_FaceDirection] = 1.0f;
                
                DrawHitPoints(Entity, RenderGroup, EntityTransform);
                
                for (u32 PieceIndex = 0; PieceIndex < Entity->PieceCount; ++PieceIndex) {
                    entity_visible_piece *Piece = Entity->Pieces + PieceIndex;
                    bitmap_id BitmapID = GetBestMatchBitmapFrom(TranState->Assets, Piece->AssetTypeID, &MatchVector, &WeightVector);
                    
                    v2 XAxis = {1.0f , 0};
                    v2 YAxis = {0, 1.0f};
                    if (Piece->Flags & PieceMove_AxesDeform) {
                        XAxis = Entity->XAxis;
                        YAxis = Entity->YAxis;
                    }
                    v3 Offset = {};
                    if (Piece->Flags & PieceMove_BobOffset) {
                        Offset = V3(Entity->FloorDisplace, 0.0f);
                        Offset.y += Entity->tBob;
                    }
                    PushBitmap(RenderGroup, EntityTransform, BitmapID, Piece->Height, Piece->Offset + Offset, Piece->Color, 1.0f, XAxis, YAxis);
                }
                EntityTransform.Upright = false;
                for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                    entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                    PushRectOutline(RenderGroup, EntityTransform, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, v4{ 0, 0.5f, 1.0f, 1.0f });
                }
                for (u32 Index = 0; Index < Entity->TraversableCount; Index++) {
                    entity_traversable_point* Traversable = Entity->Traversables + Index;
                    
                    PushRect(RenderGroup, EntityTransform, Traversable->P, v2{1.2f, 1.2f}, 
                             Traversable->Occupier? v4{ 1.0f, 0.5f, 0.0f, 1.0f }: v4{ 0.05f, 0.25f, 0.25f, 1.0f });
                    PushRectOutline(RenderGroup, EntityTransform, Traversable->P, v2{1.2f, 1.2f}, v4{0, 0, 0, 1});
                }
                if (DEBUG_UI_ENABLED) {
                    debug_id EntityDebugID = DEBUG_POINTER_ID((void *)Entity->ID.Value);
                    for (u32 VolumeIndex = 0; VolumeIndex < Entity->Collision->VolumeCount; VolumeIndex++) {
                        entity_collision_volume* Volume = Entity->Collision->Volumes + VolumeIndex;
                        v3 WorldMousePoint = Unproject(RenderGroup, EntityTransform, MouseP);
#if 0
                        PushRect(RenderGroup, V3(WorldMousePoint, 0.0f), v2{1.0f, 1.0f}, v4{1.0f, 0.0f, 1.0f, 1.0f});
#endif
                        if ((WorldMousePoint.x > -0.5f * Volume->Dim.x && WorldMousePoint.x < 0.5f * Volume->Dim.x) &&
                            (WorldMousePoint.y > -0.5f * Volume->Dim.y && WorldMousePoint.y < 0.5f * Volume->Dim.y)){
                            
                            DEBUG_HIT(EntityDebugID, WorldMousePoint.z);
                        }
                        v4 EntityOutlineColor;
                        if (DEBUG_HIGHLIGHTED(EntityDebugID, &EntityOutlineColor)) {
                            PushRectOutline(RenderGroup, EntityTransform, V3(Volume->OffsetP.xy, 0.0f), Volume->Dim.xy, EntityOutlineColor, 0.05f);
                        }
                        
                    }
                    
                    if (DEBUG_REQUESTED(EntityDebugID)) {
                        DEBUG_DATA_BLOCK("Simulation/Entity");
                        DEBUG_VALUE(Entity->P);
                        DEBUG_VALUE(Entity->dP);
                        DEBUG_VALUE(Entity->FacingDirection);
                        DEBUG_VALUE(Entity->WalkableDim);
                        DEBUG_VALUE(Entity->WalkableHeight);
                        DEBUG_VALUE(Entity->ID.Value);
                    }
                }
            }
        }
    }
}