#pragma once
struct playing_sound {
	real32 Volume[2];
	int32 SamplesPlayed;
	sound_id ID;
	playing_sound* Next;
};

struct audio_state {
	memory_arena* PermArena;
	playing_sound* FirstPlayingSound;
	playing_sound* FirstFreePlayingSound;
};
