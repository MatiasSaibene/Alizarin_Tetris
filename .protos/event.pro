
void
paste_on_board(play_piece *pp, int col, int row, int rot, Grid *g);
int
valid_position(play_piece *pp, int col, int row, int rot, Grid *g);
int
valid_screen_position(play_piece *pp, int blockWidth, Grid *g,
	int rot, int screen_x, int screen_y);
void
handle_special(play_piece *pp, int row, int col, int rot, Grid *g,
	sound_style *ss);
#define		NO_PLAYER	0
#define		HUMAN_PLAYER	1
#define		AI_PLAYER	2
#define		NETWORK_PLAYER	3
int
event_loop(SDL_Surface *screen, piece_style *ps, color_style *cs[2], 
	sound_style *ss[2], Grid g[], int level[2], int sock,
	int *seconds_remaining, int time_is_hard_limit,
	int adjust[], int (*handle)(const SDL_Event *), 
	int seed, int p1, int p2, AI_Player *AI[2]);
