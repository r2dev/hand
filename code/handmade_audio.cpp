#include "handmade.h"

asset_sound_info* GetSoundInfo(game_assets* Assets, sound_id ID) {
	asset_sound_info* Info = Assets->SoundInfos + ID.Value;
	return(Info);
}

inline b32
IsValid(sound_id ID) {
	b32 Result = (ID.Value != 0);
	return(Result);
}

internal void
ChangeVolume(playing_sound* PlayingSound, v2 TargetVolumn, real32 ChangeInSeconds) {
	
	if (ChangeInSeconds <= 0.0f) {
		PlayingSound->CurrentVolume = PlayingSound->TargetVolume = TargetVolumn;
	}
	else {
		real32 OneOverChange = 1.0f / ChangeInSeconds;
		PlayingSound->TargetVolume = TargetVolumn;

		PlayingSound->dVolume = OneOverChange * (TargetVolumn - PlayingSound->CurrentVolume);
	}
}

#define OutputChannelCount 2

internal void
OutputPlayingSounds(audio_state* AudioState, game_sound_output_buffer* SoundBuffer, game_assets* Assets, memory_arena* TempArena) {
	temporary_memory MixMemory = BeginTemporaryMemory(TempArena);
	u32 SoundBufferSampleCountAlign4 = Align4(SoundBuffer->SampleCount);
	u32 SampleCount4 = (u32)(SoundBufferSampleCountAlign4 / 4.0f);
	__m128* RealChannel0 = PushArray(TempArena, SampleCount4, __m128, 16);
	__m128* RealChannel1 = PushArray(TempArena, SampleCount4, __m128, 16);
	// clear the channel to 0
	u32 SamplesPerSecond = SoundBuffer->SamplesPerSecond;
	real32 SecondsPerSamples = 1.0f / SamplesPerSecond;

	__m128 Zero = _mm_set1_ps(0);
	{
		__m128* Dest0 = RealChannel0;
		__m128* Dest1 = RealChannel1;
		for (u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex) {
			_mm_store_ps((float*)Dest0++, Zero);
			_mm_store_ps((float*)Dest1++, Zero);
		}
	}

	for (playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound; *PlayingSoundPtr;) {

		playing_sound* PlayingSound = *PlayingSoundPtr;
		bool32 SoundFinished = false;

		u32 TotalSampleToMix = SoundBuffer->SampleCount;
		real32* Dest0 = (real32 *)RealChannel0;
		real32* Dest1 = (real32 *)RealChannel1;
		v2 MasterVolume = AudioState->MasterVolume;

		while (TotalSampleToMix && !SoundFinished) {
			//todo
			loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
			if (LoadedSound) {
				asset_sound_info* Info = GetSoundInfo(Assets, PlayingSound->ID);
				PrefetchSound(Assets, Info->NextIDToPlay);

				v2 Volume = PlayingSound->CurrentVolume;
				v2 dVolume = PlayingSound->dVolume * SecondsPerSamples;
				
				Assert(PlayingSound->SamplesPlayed >= 0);

				r32 dSample = PlayingSound->dSample;
				uint32 SampleToMix = TotalSampleToMix;
				r32 RealSampleRemainingInSound = 
					((LoadedSound->SampleCount) - RoundReal32ToUInt32(PlayingSound->SamplesPlayed)) / dSample;
				u32 SampleRemainingInSound = RoundReal32ToUInt32(RealSampleRemainingInSound);

				if (SampleToMix > SampleRemainingInSound) {
					SampleToMix = SampleRemainingInSound;
				}

				b32 VolumeEnded[OutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEnded); ++ChannelIndex) {
					if (dVolume.E[ChannelIndex] != 0.0f) {
						real32 DeltaVolume = PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex];
						u32 VolumeSampleCount = (u32)((DeltaVolume / dVolume.E[ChannelIndex]) + 0.5f); 
						if (VolumeSampleCount < SampleToMix) {
							SampleToMix = VolumeSampleCount;
							VolumeEnded[ChannelIndex] = true;
						}
					}
				}
				
				r32 SamplePositionInLoadedSound = PlayingSound->SamplesPlayed;
				for (u32 SampleIndex = 0; SampleIndex < SampleToMix; ++SampleIndex) {
#if 0
					u32 SampleIndexInLoadedSound = RoundReal32ToUInt32(SamplePositionInLoadedSound);
					real32 SampleValue = LoadedSound->Samples[0][SampleIndexInLoadedSound];
#else
					u32 FloorValuePosition = FloorReal32ToUInt32(SamplePositionInLoadedSound);
					r32 Frac = SamplePositionInLoadedSound - (r32)FloorValuePosition;

					r32 SampleValue0 = (r32)LoadedSound->Samples[0][FloorValuePosition];
					r32 SampleValue1 = (r32)LoadedSound->Samples[0][FloorValuePosition + 1];
					r32 SampleValue = Lerp(SampleValue0, Frac, SampleValue1);
#endif
					*Dest0++ += MasterVolume.E[0] * SampleValue * Volume.E[0];
					*Dest1++ += MasterVolume.E[1] * SampleValue * Volume.E[1];

					Volume += dVolume;
					SamplePositionInLoadedSound += dSample;
				}
				
				// cliping the change to final value
				for (u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEnded); ++ChannelIndex) {
					if (VolumeEnded[ChannelIndex]) {
						Volume.E[ChannelIndex] = PlayingSound->TargetVolume.E[ChannelIndex];
						PlayingSound->dVolume.E[ChannelIndex] = 0.0f;
					}
				}
				
				PlayingSound->CurrentVolume = Volume;
				
				Assert(TotalSampleToMix >= SampleToMix);
				TotalSampleToMix -= SampleToMix;

				PlayingSound->SamplesPlayed = SamplePositionInLoadedSound;

				if ((uint32)PlayingSound->SamplesPlayed == LoadedSound->SampleCount) {
					if (IsValid(Info->NextIDToPlay)) {
						PlayingSound->ID = Info->NextIDToPlay;
						PlayingSound->SamplesPlayed = 0;
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
			// note(ren) not understand enough
			if (SoundFinished) {
				*PlayingSoundPtr = PlayingSound->Next;
				PlayingSound->Next = AudioState->FirstFreePlayingSound;
				AudioState->FirstFreePlayingSound = PlayingSound;
			}
			else {
				PlayingSoundPtr = &PlayingSound->Next;
			}
		}
	}

	__m128* Source0 = RealChannel0;
	__m128* Source1 = RealChannel1;
	__m128i* SampleOut = (__m128i*)SoundBuffer->Samples;
	for (u32 SampleIndex = 0; SampleIndex < SampleCount4; ++SampleIndex) {
		__m128 S0 = _mm_load_ps((float*)Source0++);
		__m128 S1 = _mm_load_ps((float*)Source1++);

		__m128i L = _mm_cvtps_epi32(S0);
		__m128i R = _mm_cvtps_epi32(S1);
		
		__m128i LR0 = _mm_unpacklo_epi32(L, R);
		__m128i LR1 = _mm_unpackhi_epi32(L, R);

		__m128i S01 = _mm_packs_epi32(LR0, LR1);

		*SampleOut++ = S01;
	}

	EndTemporaryMemory(MixMemory);
}

internal void
InitializeAudioState(audio_state* AudioState, memory_arena* Arena) {
	AudioState->FirstFreePlayingSound = 0;
	AudioState->FirstPlayingSound = 0;
	AudioState->PermArena = Arena;
	AudioState->MasterVolume = v2{ 1.0f, 1.0f };
}

internal playing_sound*
PlaySound(audio_state* AudioState, sound_id ID) {

	if (!AudioState->FirstFreePlayingSound) {
		AudioState->FirstFreePlayingSound = PushStruct(AudioState->PermArena, playing_sound);
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
ChangePitch(playing_sound* PlayingSound, real32 dSample) {
	PlayingSound->dSample = dSample;
}
