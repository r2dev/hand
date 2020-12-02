#pragma once
struct playing_sound {
	v2 CurrentVolume;
	v2 TargetVolume;
	v2 dVolume;
	real32 SamplesPlayed;
	sound_id ID;
	playing_sound* Next;

	real32 dSample;
};

struct audio_state {
	memory_arena* PermArena;
	playing_sound* FirstPlayingSound;
	playing_sound* FirstFreePlayingSound;
	v2 MasterVolume;
};
