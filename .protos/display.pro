
void
poll_and_flame(SDL_Event *ev);
void
clear_screen_to_flame(void);
void
setup_colors(SDL_Surface *screen);
#define	DRAW_CENTER	(1<<0)
#define	DRAW_ABOVE	(1<<1)
#define DRAW_LEFT	(1<<2)
#define DRAW_UPDATE	(1<<3)
#define DRAW_CLEAR	(1<<4)
#define DRAW_HUGE	(1<<5)
#define DRAW_LARGE	(1<<6)
#define DRAW_GRID_0	(1<<7)
int
draw_string(char *text, SDL_Color sc, int x, int y, int flags);
int
give_notice(char *s, int quit_possible);
void
draw_bordered_rect(SDL_Rect *orig, SDL_Rect *border, int thick);
void
draw_pre_bordered_rect(SDL_Rect *border, int thick);
void
setup_layers(SDL_Surface * screen);
void
draw_background(SDL_Surface *screen, int blockWidth, Grid g[],
	int level[], int my_adj[], int their_adj[], char *name[]);
void
draw_pause(int on);
void
draw_clock(int seconds);
void
draw_score(SDL_Surface *screen, int i);
void
draw_next_piece(SDL_Surface *screen, piece_style *ps, color_style *cs,
	play_piece *cp, play_piece *np, int P);
