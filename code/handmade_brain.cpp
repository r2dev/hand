internal brain_id
AddBrain(game_mode_world *WorldMode) {
    brain_id ID = {++WorldMode->LastUsedEntityStorageIndex};
    return(ID);
}

internal void
ExecuteBrain(game_state *GameState, game_input *Input, sim_region *SimRegion, brain *Brain, r32 dt) {
    switch(Brain->Type) {
        case Brain_Hero: {
            entity *Head = Brain->Hero.Head;
            entity *Body = Brain->Hero.Body;
            u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstControl;
            game_controller_input* Controller = GetController(Input, ControllerIndex);
            controlled_hero* ConHero = GameState->ControlledHeroes + ControllerIndex;
            // ConHero_.ddP.x = 1.0f;
            
            
            
            v2 dSword = {};
            r32 dZ = 0.0f;
            b32 Exited = false;
            b32 DebugSpawn = false;
            
            v2 ddP = {};
            if (Controller->IsAnalog) {
                ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
            }
            else {
                r32 RecenterTimer = 0.2f;
                if (WasPressed(Controller->MoveUp)) {
                    ddP.x = 0;
                    ddP.y = 1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveDown)) {
                    ddP.x = 0;
                    ddP.y = -1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveLeft)) {
                    ddP.x = -1.0f;
                    ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveRight)) {
                    ddP.x = 1.0f;
                    ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (!IsDown(Controller->MoveUp) && !IsDown(Controller->MoveDown)) {
                    ddP.y = 0;
                    if (IsDown(Controller->MoveRight)) {
                        ddP.x = 1;
                    }
                    if (IsDown(Controller->MoveLeft)) {
                        ddP.x = -1;
                    }
                }
                
                if (!IsDown(Controller->MoveRight) && !IsDown(Controller->MoveLeft)) {
                    ddP.x = 0;
                    if (IsDown(Controller->MoveUp)) {
                        ddP.y = 1;
                    }
                    if (IsDown(Controller->MoveDown)) {
                        ddP.y = -1;
                    }
                }
                
                if (WasPressed(Controller->Start)) {
                    DebugSpawn = true;
                }
            }
#if 0
            if (Controller->Start.EndedDown) {
                ConHero->dZ = 3.0f;
            }
#endif
            dSword = {};
            if (Controller->ActionUp.EndedDown) {
                dSword = v2{ 0.0f, 1.0f };
                ChangeVolume(GameState->Music, v2{ 1, 1 }, 2.0f);
            }
            if (Controller->ActionDown.EndedDown) {
                dSword = v2{ 0.0f, -1.0f };
                ChangeVolume(GameState->Music, v2{ 0, 0 }, 2.0f);
            }
            if (Controller->ActionLeft.EndedDown) {
                ChangeVolume(GameState->Music, v2{ 1, 0 }, 2.0f);
                dSword = v2{ -1.0f, 0.0f };
            }
            if (Controller->ActionRight.EndedDown) {
                ChangeVolume(GameState->Music, v2{ 0, 1 }, 2.0f);
                dSword = v2{ 1.0f, 0.0f };
            }
            if (WasPressed(Controller->Back)) {
                Exited = true;
            }
            
#if 0                
            if (ConHero->DebugSpawn && Head) {
                traversable_reference Traversable;
                if (GetClosestTraversable(SimRegion, Head->P, &Traversable, TraversableSearch_Unoccupied)) {
                    AddPlayer(WorldMode, SimRegion, Traversable);
                } else {
                    
                }
                TestHero->DebugSpawn = false;
            }
#endif
            ConHero->RecenterTimer = ClampAboveZero(ConHero->RecenterTimer - dt);
            
            if (Head) {
                if (dSword.x == 0.0f && dSword.y == 0.0f) {
                    // remain whichever face direction it was
                }
                else {
                    Head->FacingDirection = Atan2(dSword.y, dSword.x);
                }
            }
            
            traversable_reference Traversable;
            if (Head && GetClosestTraversable(SimRegion, Head->P, &Traversable)) {
                if (Body) {
                    if (Body->MovementMode == MovementMode_Planted) {
                        if (!IsEqual(Traversable, Body->Occupying)) {
                            Body->CameFrom = Body->Occupying;
                            if (TransactionalOccupy(Body, &Body->Occupying, Traversable)) {
                                Body->tMovement = 0;
                                Body->MovementMode = MovementMode_Hopping;
                            }
                        }
                    }
                } else {
                }
                
                v3 ClosestP = GetSimEntityTraversable(Traversable).P;
                b32 TimeIsUp = ConHero->RecenterTimer == 0.0f? true: false;
                b32 NoPush = (Length(ddP) < 0.1f);
                r32 Cp = NoPush? 300.0f: 25.0f;
                v3 ddP2 = {};
                for (u32 E = 0; E < 3; ++E) {
#if 0
                    if (Square(ddP.E[E]) < 0.1f)
#else
                    if (NoPush || (TimeIsUp && (Square(ddP.E[E]) < 0.1f)))
#endif
                    {
                        ddP2.E[E] =
                            Cp * (ClosestP.E[E] - Head->P.E[E]) - 30.0f * Head->dP.E[E];
                    }
                }
                Head->dP += dt *ddP2;
            }
            
            
            if (Body) {
                Body->FacingDirection = Head->FacingDirection;
                
                DEBUG_VALUE(GetEntityGroundPoint(Body));
                
                
                Body->dP = v3{0, 0, 0};
                if (Body->MovementMode == MovementMode_Planted) {
                    Body->P = GetSimEntityTraversable(Body->Occupying).P;
                }
                
                
                v3 HeadDelta = {};
                if (Head) {
                    HeadDelta = Head->P - Body->P;
                }
                Body->FloorDisplace = 0.25f * HeadDelta.xy;
                Body->YAxis = v2{0, 1} + 0.5f * HeadDelta.xy;
                
            }
            
            if (Exited) {
                DeleteEntity(SimRegion, Body);
                DeleteEntity(SimRegion, Head);
                ConHero->BrainID.Value = 0;
            }
            
        } break;
        case Brain_Snake: {
            
        } break;
        InvalidDefaultCase;
    }
}