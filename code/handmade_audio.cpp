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
