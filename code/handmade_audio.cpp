internal void
ChangeVolume(playing_sound* PlayingSound, v2 TargetVolumn, r32 ChangeInSeconds) {
    TIMED_FUNCTION();
    if (PlayingSound) {
        if (ChangeInSeconds <= 0.0f) {
            PlayingSound->CurrentVolume = PlayingSound->TargetVolume = TargetVolumn;
        }
        else {
            r32 OneOverChange = 1.0f / ChangeInSeconds;
            PlayingSound->TargetVolume = TargetVolumn;
            
            PlayingSound->dVolume = OneOverChange * (TargetVolumn - PlayingSound->CurrentVolume);
        }
    }
}

#define OutputChannelCount 2

internal void
OutputPlayingSounds(audio_state* AudioState, game_sound_output_buffer* SoundBuffer, game_assets* Assets, memory_arena* TempArena) {
    TIMED_FUNCTION();
	temporary_memory MixMemory = BeginTemporaryMemory(TempArena);
    u32 GenerationID = BeginGeneration(Assets);
    
	u32 ChunkCount = (u32)(SoundBuffer->SampleCount / 4.0f);
	__m128* RealChannel0 = PushArray(TempArena, ChunkCount, __m128, AlignNoClear(16));
	__m128* RealChannel1 = PushArray(TempArena, ChunkCount, __m128, AlignNoClear(16));
	// clear the channel to 0
	u32 SamplesPerSecond = SoundBuffer->SamplesPerSecond;
	r32 SecondsPerSamples = 1.0f / SamplesPerSecond;
    
	__m128 Zero = _mm_set1_ps(0);
	__m128 One = _mm_set1_ps(1.0f);
	{
		__m128* Dest0 = RealChannel0;
		__m128* Dest1 = RealChannel1;
		for (u32 SampleIndex = 0; SampleIndex < ChunkCount; ++SampleIndex) {
			_mm_store_ps((float*)Dest0++, Zero);
			_mm_store_ps((float*)Dest1++, Zero);
		}
	}
    
	for (playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound; *PlayingSoundPtr;) {
        
		playing_sound* PlayingSound = *PlayingSoundPtr;
		b32 SoundFinished = false;
        
		u32 TotalChunkToMix = ChunkCount;
		__m128* Dest0 = (__m128*)RealChannel0;
		__m128* Dest1 = (__m128*)RealChannel1;
		v2 MasterVolume = AudioState->MasterVolume;
        
		while (TotalChunkToMix && !SoundFinished) {
			//todo
			loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID, GenerationID);
			if (LoadedSound) {
				sound_id NextSoundID = GetNextSoundInChain(Assets, PlayingSound->ID);
				PrefetchSound(Assets, NextSoundID);
                
				v2 Volume = PlayingSound->CurrentVolume;
				v2 dVolume = PlayingSound->dVolume * SecondsPerSamples;
				v2 dVolumeChunk = dVolume * 4.0f;
                
				// channel 0
                
				__m128 MasterVolume0 = _mm_set1_ps(AudioState->MasterVolume.E[0]);
				__m128 Volume0 = _mm_setr_ps(
                                             Volume.E[0] + 0 * dVolume.E[0],
                                             Volume.E[0] + 1.0f * dVolume.E[0],
                                             Volume.E[0] + 2.0f * dVolume.E[0],
                                             Volume.E[0] + 3.0f * dVolume.E[0]
                                             );
				__m128 dVolume0 = _mm_set1_ps(dVolume.E[0]);
				__m128 dVolumeChunk0 = _mm_set1_ps(dVolumeChunk.E[0]);
                
				// Channel 1
				__m128 MasterVolume1 = _mm_set1_ps(AudioState->MasterVolume.E[1]);
				__m128 Volume1 = _mm_setr_ps(
                                             Volume.E[1] + 0 * dVolume.E[1],
                                             Volume.E[1] + 1.0f * dVolume.E[1],
                                             Volume.E[1] + 2.0f * dVolume.E[1],
                                             Volume.E[1] + 3.0f * dVolume.E[1]
                                             );
				__m128 dVolume1 = _mm_set1_ps(dVolume.E[1]);
				__m128 dVolumeChunk1 = _mm_set1_ps(dVolumeChunk.E[1]);
                
                
				Assert(PlayingSound->SamplesPlayed >= 0);
                
				r32 dSample = PlayingSound->dSample;
				r32 dSampleChunk = 4.0f * dSample;
                
				u32 ChunkToMix = TotalChunkToMix;
                
				r32 RealChunkRemainingInSound =
					((LoadedSound->SampleCount) - RoundReal32ToUInt32(PlayingSound->SamplesPlayed)) / dSampleChunk;
				u32 ChunkRemainingInSound = RoundReal32ToUInt32(RealChunkRemainingInSound);
				b32 InputSampleEnded = false;
				if (ChunkToMix > ChunkRemainingInSound) {
					ChunkToMix = ChunkRemainingInSound;
					InputSampleEnded = true;
				}
                
				u32 VolumeEndedAt[OutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEndedAt); ++ChannelIndex) {
					if (dVolumeChunk.E[ChannelIndex] != 0.0f) {
						r32 DeltaVolume = PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex];
						u32 VolumeChunkCount = (u32)((DeltaVolume / dVolumeChunk.E[ChannelIndex]) + 0.5f);
						if (VolumeChunkCount < ChunkToMix) {
							ChunkToMix = VolumeChunkCount;
							VolumeEndedAt[ChannelIndex] = VolumeChunkCount;
						}
					}
				}
                
				r32 BeginSamplePosition = PlayingSound->SamplesPlayed;
				r32 EndSamplePosition = PlayingSound->SamplesPlayed + ChunkToMix * dSampleChunk;
                
				r32 LoopIndexC = (EndSamplePosition - BeginSamplePosition) / (r32)ChunkToMix;
                
                
				for (u32 SampleIndex = 0; SampleIndex < ChunkToMix; ++SampleIndex) {
					r32 SamplePosition = BeginSamplePosition + LoopIndexC * SampleIndex;
#if 0				
					__m128 SampleValue = _mm_setr_ps(LoadedSound->Samples[0][RoundReal32ToUInt32(SamplePosition + dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToUInt32(SamplePosition + 1.0f * dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToUInt32(SamplePosition + 2.0f * dSample)],
                                                     LoadedSound->Samples[0][RoundReal32ToUInt32(SamplePosition + 3.0f * dSample)]);
                    
                    
#else
					__m128 SamplePos = _mm_setr_ps(SamplePosition + 0.0f * dSample, SamplePosition + 1.0f * dSample, SamplePosition + 2.0f * dSample, SamplePosition + 3.0f * dSample);
					__m128i FloorSamplePos = _mm_cvttps_epi32(SamplePos);
					__m128 Frac = _mm_sub_ps(SamplePos, _mm_cvtepi32_ps(FloorSamplePos));
					
                    
					__m128 SampleValue0 = _mm_setr_ps((r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[0]],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[1]],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[2]],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[3]]);
					__m128 SampleValue1 = _mm_setr_ps((r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[0] + 1],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[1] + 1],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[2] + 1],
                                                      (r32)LoadedSound->Samples[0][((s32*)&FloorSamplePos)[3] + 1]);
                    
					__m128 SampleValue = _mm_add_ps(_mm_mul_ps(_mm_sub_ps(One, Frac), SampleValue0), _mm_mul_ps(Frac, SampleValue1));
#endif
                    
					__m128 D0 = _mm_load_ps((float*)&Dest0[0]);
					__m128 D1 = _mm_load_ps((float*)&Dest1[0]);
                    
                    
					D0 = _mm_add_ps(D0, _mm_mul_ps(_mm_mul_ps(MasterVolume0, _mm_add_ps(dVolume0, Volume0)), SampleValue));
					D1 = _mm_add_ps(D1, _mm_mul_ps(_mm_mul_ps(MasterVolume1, _mm_add_ps(dVolume1, Volume1)), SampleValue));
                    
					_mm_store_ps((float*)&Dest0[0], D0);
					_mm_store_ps((float*)&Dest1[0], D1);
                    
					Dest0++;
					Dest1++;
					Volume0 = _mm_add_ps(Volume0, dVolumeChunk0);
					Volume1 = _mm_add_ps(Volume1, dVolumeChunk1);
				}
                
				PlayingSound->CurrentVolume.E[0] = ((r32*)&Volume0)[0];
				PlayingSound->CurrentVolume.E[1] = ((r32*)&Volume1)[1];
                
				// cliping the change to final value
				for (u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEndedAt); ++ChannelIndex) {
					if (VolumeEndedAt[ChannelIndex] == ChunkToMix) {
						Volume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dVolume.E[ChannelIndex] = 0.0f;
					}
				}
                
				PlayingSound->SamplesPlayed = EndSamplePosition;
                
				Assert(TotalChunkToMix >= ChunkToMix);
				TotalChunkToMix -= ChunkToMix;
				if (ChunkRemainingInSound == ChunkToMix) {
					if (IsValid(NextSoundID)) {
						PlayingSound->ID = NextSoundID;
						PlayingSound->SamplesPlayed -= (r32)LoadedSound->SampleCount;
						if (PlayingSound->SamplesPlayed < 0) {
							PlayingSound->SamplesPlayed = 0.0f;
						}
					}
					else {
						SoundFinished = true;
					}
                    
				}
			}
			else {
				LoadSound(Assets, PlayingSound->ID);
				break;
			}
		}
        if (SoundFinished) {
            *PlayingSoundPtr = PlayingSound->Next;
            PlayingSound->Next = AudioState->FirstFreePlayingSound;
            AudioState->FirstFreePlayingSound = PlayingSound;
        }
        else {
            PlayingSoundPtr = &PlayingSound->Next;
        }
	}
	__m128* Source0 = RealChannel0;
	__m128* Source1 = RealChannel1;
	__m128i* SampleOut = (__m128i*)SoundBuffer->Samples;
	for (u32 SampleIndex = 0; SampleIndex < ChunkCount; ++SampleIndex) {
		__m128 S0 = _mm_load_ps((float*)Source0++);
		__m128 S1 = _mm_load_ps((float*)Source1++);
        
		__m128i L = _mm_cvtps_epi32(S0);
		__m128i R = _mm_cvtps_epi32(S1);
        
		__m128i LR0 = _mm_unpacklo_epi32(L, R);
		__m128i LR1 = _mm_unpackhi_epi32(L, R);
        
		__m128i S01 = _mm_packs_epi32(LR0, LR1);
        
		*SampleOut++ = S01;
	}
    EndGeneration(Assets, GenerationID);
	EndTemporaryMemory(MixMemory);
}

internal void
InitializeAudioState(audio_state* AudioState) {
	AudioState->FirstFreePlayingSound = 0;
	AudioState->FirstPlayingSound = 0;
	
	AudioState->MasterVolume = v2{ 0.3f, 0.3f };
}

internal playing_sound*
PlaySound(audio_state* AudioState, sound_id ID) {
    TIMED_FUNCTION();
	if (!AudioState->FirstFreePlayingSound) {
		AudioState->FirstFreePlayingSound = PushStruct(&AudioState->AudioArena, playing_sound);
		AudioState->FirstFreePlayingSound->Next = 0;
	}
	playing_sound* PlayingSound = AudioState->FirstFreePlayingSound;
	AudioState->FirstFreePlayingSound = PlayingSound->Next;
	PlayingSound->ID = ID;
	PlayingSound->SamplesPlayed = 0;
	PlayingSound->CurrentVolume = PlayingSound->TargetVolume = v2{ 1.0f, 1.0f };
	PlayingSound->dVolume = v2{ 0, 0 };
	PlayingSound->Next = AudioState->FirstPlayingSound;
	PlayingSound->dSample = 1.0f;
	AudioState->FirstPlayingSound = PlayingSound;
	return(PlayingSound);
}

internal void
ChangePitch(playing_sound* PlayingSound, r32 dSample) {
    if (PlayingSound) {
        PlayingSound->dSample = dSample;
    }
}
