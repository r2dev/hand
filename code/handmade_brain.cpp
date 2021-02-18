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
            
            
            if (Controller->IsAnalog) {
                ConHero->ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
            }
            else {
                r32 RecenterTimer = 0.2f;
                if (WasPressed(Controller->MoveUp)) {
                    ConHero->ddP.x = 0;
                    ConHero->ddP.y = 1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveDown)) {
                    ConHero->ddP.x = 0;
                    ConHero->ddP.y = -1.0f;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveLeft)) {
                    ConHero->ddP.x = -1.0f;
                    ConHero->ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (WasPressed(Controller->MoveRight)) {
                    ConHero->ddP.x = 1.0f;
                    ConHero->ddP.y = 0;
                    ConHero->RecenterTimer = RecenterTimer;
                }
                if (!IsDown(Controller->MoveUp) && !IsDown(Controller->MoveDown)) {
                    ConHero->ddP.y = 0;
                    if (IsDown(Controller->MoveRight)) {
                        ConHero->ddP.x = 1;
                    }
                    if (IsDown(Controller->MoveLeft)) {
                        ConHero->ddP.x = -1;
                    }
                }
                
                if (!IsDown(Controller->MoveRight) && !IsDown(Controller->MoveLeft)) {
                    ConHero->ddP.x = 0;
                    if (IsDown(Controller->MoveUp)) {
                        ConHero->ddP.y = 1;
                    }
                    if (IsDown(Controller->MoveDown)) {
                        ConHero->ddP.y = -1;
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
                b32 NoPush = (Length(ConHero->ddP) < 0.1f);
                r32 Cp = NoPush? 300.0f: 25.0f;
                v3 ddP2 = V3(ConHero->ddP, 0);
                for (u32 E = 0; E < 3; ++E) {
#if 0
                    if (Square(ddP.E[E]) < 0.1f)
#else
                    if (NoPush || (TimeIsUp && (Square(ConHero->ddP.E[E]) < 0.1f)))
#endif
                    {
                        ddP2.E[E] =
                            Cp * (ClosestP.E[E] - Head->P.E[E]) - 30.0f * Head->dP.E[E];
                    }
                }
                Head->MoveSpec.UnitMaxAccelVector = true;
                Head->MoveSpec.Speed = 30.0f;
                Head->MoveSpec.Drag = 8.0f;
                Head->ddP = ddP2;
            }
            
            
            if (Body) {
                Body->FacingDirection = Head->FacingDirection;
                Body->dP = v3{0, 0, 0};
                if (Body->MovementMode == MovementMode_Planted) {
                    Body->P = GetSimEntityTraversable(Body->Occupying).P;
                    if (Head) {
                        r32 HeadDistance = Length(Head->P - Body->P);
                        r32 MaxHeadDistance = 0.5f;
                        r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                        Body->ddtBob = -20.0f * tHeadDistance;
                    }
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
        case Brain_Familiar: {
            
#if 0            
            entity* ClosestHero = 0;
            r32 ClosestHeroSq = Square(10.0f);
            entity* TestEntity = SimRegion->Entities;
            
            if(Global_Sim_FamiliarFollowsHero)
            {
                for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
                    if (TestEntity->Type == EntityType_HeroBody) {
                        r32 TestDSq = LengthSq(TestEntity->P - Entity->P);
                        
                        if (ClosestHeroSq > TestDSq) {
                            ClosestHero = TestEntity;
                            ClosestHeroSq = TestDSq;
                        }
                    }
                }
            }
            
            if (ClosestHero && (ClosestHeroSq > Square(3.0f))) {
                r32 Acceleration = 0.5f;
                r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
                Entity->ddP = OneOverLength * (ClosestHero->P - Entity->P);
            }
            MoveSpec.UnitMaxAccelVector = true;
            MoveSpec.Speed = 50.0f;
            MoveSpec.Drag = 0.2f;
#endif
        } break;
        case Brain_FloatyThingForNow: {
            //Entity->P.z += 0.05f * Cos(Entity->tBob);
            //Entity->tBob += dt;
        } break;
        case Brain_Monstar: {
        } break;
        InvalidDefaultCase;
    }
}