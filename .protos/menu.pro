ATMenu* menu(int nButtons, char** strings, int defaultchoice,
	     Uint32 face_color0, Uint32 text_color0,
	     Uint32 face_color1, Uint32 text_color1, int x, int y);
void show_menu(ATMenu* am);
int check_menu(ATMenu* am, int x, int y);
void delete_menu(ATMenu* am);
void
setup_radio(WalkRadio *wr);
int
check_radio(WalkRadio *wr, int x, int y);
void
clear_radio(WalkRadio *wr);
void
draw_radio(WalkRadio *wr, int state);
WalkRadioGroup *
create_single_wrg(int n);
int
handle_radio_event(WalkRadioGroup *wrg, const SDL_Event *ev);
