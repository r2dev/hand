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
OutputPlayingSounds(audio_state* AudioState, game_sound_output_buffer* SoundBuffer, game_assets* Assets, memory_arena* TempArena) {
	temporary_memory MixMemory = BeginTemporaryMemory(TempArena);
	real32* RealChannel0 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
	real32* RealChannel1 = PushArray(TempArena, SoundBuffer->SampleCount, real32);
	// clear the channel to 0

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

		while (TotalSampleToMix && !SoundFinished) {
			//todo
			loaded_sound* LoadedSound = GetSound(Assets, PlayingSound->ID);
			if (LoadedSound) {
				asset_sound_info* Info = GetSoundInfo(Assets, PlayingSound->ID);
				PrefetchSound(Assets, Info->NextIDToPlay);

				real32 Volumn0 = PlayingSound->Volume[0];
				real32 Volumn1 = PlayingSound->Volume[1];

				Assert(PlayingSound->SamplesPlayed >= 0);

				uint32 SampleToMix = TotalSampleToMix;
				uint32 SampleRemainingInSound = LoadedSound->SampleCount - PlayingSound->SamplesPlayed;
				if (SampleToMix > SampleRemainingInSound) {
					SampleToMix = SampleRemainingInSound;
				}
				for (uint32 SampleIndex = 0; SampleIndex < SampleToMix; ++SampleIndex) {
					uint32 SampleIndexInLoadedSound = SampleIndex + PlayingSound->SamplesPlayed;
					real32 SampleValue = LoadedSound->Samples[0][SampleIndexInLoadedSound];
					*Dest0++ += SampleValue * Volumn0;
					*Dest1++ += SampleValue * Volumn1;
				}
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
				else {
					Assert(TotalSampleToMix == 0);
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
}
