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
	real32* RealChannel0 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
	real32* RealChannel1 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
	// clear the channel to 0
	u32 SamplesPerSecond = SoundBuffer->SamplesPerSecond;
	real32 SecondsPerSamples = 1.0f / SamplesPerSecond;
	{
		real32* Dest0 = RealChannel0;
		real32* Dest1 = RealChannel1;
		for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
			*Dest0++ = 0;
			*Dest1++ = 0;
		}
	}

	for (playing_sound** PlayingSoundPtr = &AudioState->FirstPlayingSound; *PlayingSoundPtr;) {

		playing_sound* PlayingSound = *PlayingSoundPtr;
		bool32 SoundFinished = false;

		u32 TotalSampleToMix = SoundBuffer->SampleCount;
		real32* Dest0 = RealChannel0;
		real32* Dest1 = RealChannel1;
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

				uint32 SampleToMix = TotalSampleToMix;
				uint32 SampleRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
				if (SampleToMix > SampleRemainingInSound) {
					SampleToMix = SampleRemainingInSound;
				}

				b32 VolumeEnded[OutputChannelCount] = {};
				for (u32 ChannelIndex = 0; ChannelIndex < ArrayCount(VolumeEnded); ++ChannelIndex) {
					// if there is change
					if (dVolume.E[ChannelIndex] != 0.0f) {
						real32 DeltaVolume = PlayingSound->TargetVolume.E[ChannelIndex] - Volume.E[ChannelIndex];
						u32 VolumeSampleCount = (u32)(DeltaVolume / dVolume.E[ChannelIndex] + 0.5f); 
						if (VolumeSampleCount < SampleToMix) {
							SampleToMix = VolumeSampleCount;
							VolumeEnded[ChannelIndex] = true;
						}
					}
				}

				for (uint32 SampleIndex = 0; SampleIndex < SampleToMix; ++SampleIndex) {
					uint32 SampleIndexInLoadedSound = SampleIndex + PlayingSound->SamplesPlayed;
					real32 SampleValue = LoadedSound->Samples[0][SampleIndexInLoadedSound];
					*Dest0++ += MasterVolume.E[0] * SampleValue * Volume.E[0];
					*Dest1++ += MasterVolume.E[1] * SampleValue * Volume.E[1];
					Volume += dVolume;
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

				PlayingSound->SamplesPlayed += SampleToMix;

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

	real32* Source0 = RealChannel0;
	real32* Source1 = RealChannel1;
	int16* SampleOut = SoundBuffer->Samples;
	for (int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
		*SampleOut++ = (int16)(*Source0++ + 0.5f);
		*SampleOut++ = (int16)(*Source1++ + 0.5f);
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
	AudioState->FirstPlayingSound = PlayingSound;
	return(PlayingSound);
}
