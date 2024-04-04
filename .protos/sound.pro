
void
play_sound_unless_already_playing(sound_style *ss, int which, int delay);
void
stop_playing_sound(sound_style *ss, int which);
void
play_sound(sound_style *ss, int which, int delay);
void
stop_all_playing(void);
void
play_all_sounds(sound_style *ss);
sound_styles
load_sound_styles(int sound_wanted);
