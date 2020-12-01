#pragma once
struct playing_sound {
	v2 CurrentVolume;
	v2 TargetVolume;
	v2 dVolume;
	int32 SamplesPlayed;
	sound_id ID;
	playing_sound* Next;
};

struct audio_state {
	memory_arena* PermArena;
	playing_sound* FirstPlayingSound;
	playing_sound* FirstFreePlayingSound;
};
