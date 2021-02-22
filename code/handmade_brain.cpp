internal brain_id
AddBrain(game_mode_world *WorldMode) {
    brain_id ID = {++WorldMode->LastUsedEntityStorageIndex};
    return(ID);
}

internal void
ExecuteBrain(game_state *GameState, game_mode_world *WorldMode, game_input *Input, sim_region *SimRegion, brain *Brain, r32 dt) {
    switch(Brain->Type) {
        case Type_brain_hero: {
            entity *Head = Brain->Hero.Head;
            entity *Body = Brain->Hero.Body;
            
            u32 ControllerIndex = Brain->ID.Value - ReservedBrainID_FirstControl;
            game_controller_input *Controller = GetController(Input, ControllerIndex);
            controlled_hero *ConHero = GameState->ControlledHeroes + ControllerIndex;
            
            v2 dSword = {};
            r32 dZ = 0.0f;
            b32 Exited = false;
            b32 DebugSpawn = false;
            
            if (Controller->IsAnalog) {
                ConHero->ddP = v2{ Controller->StickAverageX, Controller->StickAverageY };
            }
            else {
                r32 RecenterTimer = 0.5f;
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
                if (!IsDown(Controller->MoveUp) &&
                    !IsDown(Controller->MoveDown)) {
                    ConHero->ddP.y = 0;
                    if (IsDown(Controller->MoveRight)) {
                        ConHero->ddP.x = 1;
                    }
                    if (IsDown(Controller->MoveLeft)) {
                        ConHero->ddP.x = -1;
                    }
                }
                
                if (!IsDown(Controller->MoveRight) &&
                    !IsDown(Controller->MoveLeft)) {
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
            DEBUG_VALUE(ConHero->RecenterTimer);
            DEBUG_VALUE(ConHero->ddP);
            dSword = {};
            if (Controller->ActionUp.EndedDown) {
                dSword = v2{ 0.0f, 1.0f };
            }
            if (Controller->ActionDown.EndedDown) {
                dSword = v2{ 0.0f, -1.0f };
            }
            if (Controller->ActionLeft.EndedDown) {
                dSword = v2{ -1.0f, 0.0f };
            }
            if (Controller->ActionRight.EndedDown) {
                dSword = v2{ 1.0f, 0.0f };
            }
            if (WasPressed(Controller->Back)) {
                Exited = true;
            }
            
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
                }
                
                v3 ClosestP = GetSimSpaceTraversable(Traversable).P;
                
                v3 ddP = V3(ConHero->ddP, 0);
                r32 ddpLength = LengthSq(ddP);
                if (ddpLength > 1.0f) {
                    ddP *= (1.0f / SquareRoot(ddpLength));
                }
                r32 MovementSpeed = 30.0f;
                r32 Drag = 8.0f;
                ddP *= MovementSpeed;
                b32 TimeIsUp = (ConHero->RecenterTimer == 0.0f);
                b32 NoPush = (LengthSq(ConHero->ddP) < 0.1f);
                r32 Cp = NoPush? 300.0f: 25.0f;
                for (u32 E = 0; E < 3; ++E) {
                    if (NoPush || (TimeIsUp && (Square(ddP.E[E]) < 0.1f))) {
                        // pull head to closest point
                        ddP.E[E] = Cp * (ClosestP.E[E] - Head->P.E[E]) - 30.0f * Head->dP.E[E];
                    } else {
                        // air drag
                        ddP.E[E] += -Drag * Head->dP.E[E];
                    }
                }
                ConHero->RecenterTimer = ClampAboveZero(ConHero->RecenterTimer - dt);
                Head->ddP = ddP;
            }
            
            
            if (Body) {
                
                Body->dP = v3{0, 0, 0};
                if (Body->MovementMode == MovementMode_Planted) {
                    Body->P = GetSimSpaceTraversable(Body->Occupying).P;
                    if (Head) {
                        r32 HeadDistance = Length(Head->P - Body->P);
                        r32 MaxHeadDistance = 0.5f;
                        r32 tHeadDistance = Clamp01MapToRange(0.0f, HeadDistance, MaxHeadDistance);
                        Body->ddtBob = -20.0f * tHeadDistance;
                    }
                }
                
                v3 HeadDelta = {};
                if (Head) {
                    Body->FacingDirection = Head->FacingDirection;
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
        case Type_brain_snake: {
            brain_snake *Snake = &Brain->Snake;
            entity *Head = Snake->Segments[0];
            if (Head) {
                traversable_reference Traversable;
                v3 Delta = {RandomBilateral(&WorldMode->GameEntropy), RandomBilateral(&WorldMode->GameEntropy), 0.0f};
                if (GetClosestTraversable(SimRegion, Head->P + Delta, &Traversable)) {
                    if (Head->MovementMode == MovementMode_Planted) {
                        if (!IsEqual(Traversable, Head->Occupying)) {
                            Head->CameFrom = Head->Occupying;
                            traversable_reference LastOccupying = Head->Occupying;
                            if (TransactionalOccupy(Head, &Head->Occupying, Traversable)) {
                                Head->tMovement = 0;
                                Head->MovementMode = MovementMode_Hopping;
                                for (u32 SegmentIndex = 1; SegmentIndex < ArrayCount(Snake->Segments); ++SegmentIndex) {
                                    entity *Segment = Snake->Segments[SegmentIndex];
                                    if (Segment) {
                                        traversable_reference Next = Segment->Occupying;
                                        TransactionalOccupy(Segment, &Segment->Occupying, LastOccupying);
                                        LastOccupying = Next;
                                        Segment->tMovement = 0;
                                        Segment->MovementMode = MovementMode_Hopping;
                                        Segment->CameFrom = Segment->Occupying;
                                    }
                                }
                            }
                        }
                    }
                }
                
                
                
            }
        } break;
        case Type_brain_familiar: {
            brain_familiar *Familiar = &Brain->Familiar;
            entity *Head = Familiar->Head;
            
            entity* ClosestHero = 0;
            r32 ClosestHeroSq = Square(10.0f);
            entity* TestEntity = SimRegion->Entities;
            
            if(Global_Sim_FamiliarFollowsHero)
            {
                for (u32 TestEntityIndex = 0; TestEntityIndex < SimRegion->EntityCount; ++TestEntityIndex, ++TestEntity) {
                    if (IsType(TestEntity->BrainSlot, Type_brain_hero)) {
                        r32 TestDSq = LengthSq(TestEntity->P - Head->P);
                        
                        if (ClosestHeroSq > TestDSq) {
                            ClosestHero = TestEntity;
                            ClosestHeroSq = TestDSq;
                        }
                    }
                }
            }
            
            if (ClosestHero && (ClosestHeroSq > Square(3.0f))) {
                r32 Acceleration = 1.0f;
                r32 OneOverLength = Acceleration / SquareRoot(ClosestHeroSq);
                Head->ddP = OneOverLength * (ClosestHero->P - Head->P);
            }
        } break;
        case Type_brain_floaty_thing_for_now: {
            //Entity->P.z += 0.05f * Cos(Entity->tBob);
            //Entity->tBob += dt;
        } break;
        case Type_brain_monster: {
            brain_monster *Monster = &Brain->Monster;
            entity *Body = Monster->Body;
            if (Body) {
                traversable_reference Traversable;
                v3 Delta = {RandomBilateral(&WorldMode->GameEntropy), RandomBilateral(&WorldMode->GameEntropy), 0.0f};
                if (GetClosestTraversable(SimRegion, Body->P + Delta, &Traversable)) {
                    if (Body->MovementMode == MovementMode_Planted) {
                        if (!IsEqual(Traversable, Body->Occupying)) {
                            Body->CameFrom = Body->Occupying;
                            if (TransactionalOccupy(Body, &Body->Occupying, Traversable)) {
                                Body->tMovement = 0;
                                Body->MovementMode = MovementMode_Hopping;
                            }
                        }
                    }
                }
            }
        } break;
        InvalidDefaultCase;
    }
}