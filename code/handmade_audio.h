#pragma once
struct playing_sound {
	v2 CurrentVolume;
	v2 TargetVolume;
	v2 dVolume;
	r32 SamplesPlayed;
	sound_id ID;
	playing_sound* Next;
    
	r32 dSample;
};

struct audio_state {
	memory_arena* PermArena;
	playing_sound* FirstPlayingSound;
	playing_sound* FirstFreePlayingSound;
	v2 MasterVolume;
};
