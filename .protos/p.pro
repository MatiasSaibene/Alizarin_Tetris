
piece_styles
load_piece_styles(void);
color_styles 
load_color_styles(SDL_Surface * screen);
play_piece
generate_piece(piece_style *ps, color_style *cs, unsigned int seq);
void
draw_grid_play_piece(SDL_Surface *screen, color_style *cs, 
	play_piece *o_pp, int o_x, int o_y, int o_rot,	/* old */
	play_piece *pp,int x, int y, int rot)		/* new */;
void
draw_next_play_piece(SDL_Surface *screen, color_style *cs, 
	play_piece *o_pp, int o_x, int o_y, int o_rot,	/* old */
	play_piece *pp,int x, int y, int rot)		/* new */;
