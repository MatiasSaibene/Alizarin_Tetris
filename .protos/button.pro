
ATButton* button(char* text, Uint32 face_color0, Uint32 text_color0,
	Uint32 face_color1, Uint32 text_color1,
	int x, int y);
void show_button(ATButton* ab, int state);
char check_button(ATButton* ab, int x, int y);
