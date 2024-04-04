/*
 *                               Alizarin Tetris
 * The main match event-loop. Code here should relate to the state machine
 * that keeps track of what to display, accepts as input, send out as output,
 * etc., as time progresses. 
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <unistd.h>
#include <sys/types.h>

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#else
#if HAVE_WINSOCK_H
#include <winsock.h>
#endif
#endif

#include "atris.h"
#include "grid.h"
#include "piece.h"
#include "sound.h"
#include "ai.h"
#include "options.h"

#include ".protos/ai.pro"
#include ".protos/display.pro"
#include ".protos/xflame.pro"

extern int Score[];

struct state_struct {
    int 	ai;
    int 	falling;
    int 	fall_speed; 
    int 	accept_input;
    int 	tetris_handling;
    int 	limbo;
    int 	other_in_limbo;
    int 	limbo_sent;
    int 	draw;
    Uint32	collide_time;	/* time when your piece merges with the rest */
    Uint32 	next_draw;
    Uint32 	draw_timeout;
    Uint32 	tv_next_fall;
    int 	fall_event_interval;
    Uint32 	tv_next_tetris;
    int 	tetris_event_interval;
    Uint32 	tv_next_ai_think;
    Uint32 	tv_next_ai_move;
    int		ai_interval;
    int 	ready_for_fast;
    int 	ready_for_rotate;
    int		seed;
    play_piece	cp, np;		
    void *	ai_state;
    /* these two are used by tetris_event() */
    int		check_result;
    int		num_lines_cleared;
} State[2];

/* one position structure per player */
struct pos_struct {
    int x;
    int y;
    int rot;
    int old_x;
    int old_y;
    int old_rot;
    Command move;
} pos[2];

Grid distract_grid[2];

/***************************************************************************
 *      paste_on_board()
 * Places the given piece on the board. Uses row-column (== grid)
 * coordinates. 
 *********************************************************************PROTO*/
void
paste_on_board(play_piece *pp, int col, int row, int rot, Grid *g)
{
    int i,j,c;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col; /* was + (screen_x / cs->w); */
		int t_y = j + row; /* was + (screen_y / cs->h); */
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    Debug("Serious consistency failure: dropping pieces.\n");
		    continue;
		}
		GRID_SET(*g,t_x,t_y,pp->colormap[c]);
		FALL_SET(*g,t_x,t_y,NOT_FALLING);
		if (t_x > 0) GRID_CHANGED(*g,t_x-1,t_y) = 1;
		if (t_y > 0) GRID_CHANGED(*g,t_x,t_y-1) = 1;
		if (t_x < g->w-1) GRID_CHANGED(*g,t_x+1,t_y) = 1;
		if (t_y < g->h-1) GRID_CHANGED(*g,t_x,t_y+1) = 1;
	}
    return;
}

/***************************************************************************
 *      screen_to_exact_grid_coords()
 * Converts screen coordinates to exact grid coordinates. Will abort the
 * program if you pass in unaligned coordinates!
 ***************************************************************************/
static void
screen_to_exact_grid_coords(Grid *g, int blockWidth, 
	int screen_x, int screen_y, int *row, int *col)
{
    screen_x -= g->board.x;
    screen_y -= g->board.y;

    Assert(screen_x % blockWidth == 0);
    Assert(screen_y % blockWidth == 0);

    *row = screen_y / blockWidth;
    *col = screen_x / blockWidth;

    return;
}

/***************************************************************************
 *      screen_to_grid_coords()
 * Converts screen coordinates to grid coordinates. Rounds "down". 
 ***************************************************************************/
void
screen_to_grid_coords(Grid *g, int blockWidth, 
	int screen_x, int screen_y, int *row, int *col)
{
    screen_x -= g->board.x;
    screen_y -= g->board.y;
    if (screen_x < 0) screen_x -= 19;	/* round up to negative #s */
    if (screen_y < 0) screen_y -= 19;	/* round up to negative #s */

    *row = screen_y / blockWidth;
    *col = screen_x / blockWidth;

    return;
}

/***************************************************************************
 *      valid_position()
 * Determines if the given position is valid. Uses row-column (== grid)
 * coordinates. Returns 0 if the piece would fall out of bounds or if
 * some solid part of the piece would fall over something already on the
 * the grid. Returns 1 otherwise (it is then safe to call
 * paste_on_board()). 
 *********************************************************************PROTO*/
int
valid_position(play_piece *pp, int col, int row, int rot, Grid *g)
{
    int i,j;

    /* 
     * We don't want this check because you can have col=-2 or whatnot if
     * your piece doesn't start on its leftmost border.
     *
     * if (col < 0 || col >= g->w || row < 0 || row >= g->h)
     *	return 0;
     */

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if (BITMAP(*pp->base,rot,i,j)) {
		int t_x = i + col; /* was + (screen_x / cs->w); */
		int t_y = j + row; /* was + (screen_y / cs->h); */
		if (t_x < 0 || t_y < 0 || t_x >= g->w || t_y >= g->h)
		    return 0;
		if (GRID_CONTENT(*g,t_x,t_y)) return 0;
	}
    return 1;
}

/***************************************************************************
 *      valid_screen_position()
 * Determines if the given position is valid. Uses screen coordinates.
 * Handles pieces that are not perfectly row-aligned. Pieces must still
 * be perfectly column aligned. 
 *
 * Returns 0 if the piece would fall out of bounds or if some solid part of
 * the piece would fall over something already on the the grid. Returns 1
 * otherwise (it is then safe to call paste_on_board()). 
 *********************************************************************PROTO*/
int
valid_screen_position(play_piece *pp, int blockWidth, Grid *g,
	int rot, int screen_x, int screen_y)
{
    int row, row2, col;

    screen_to_grid_coords(g, blockWidth, screen_x, screen_y, &row, &col);

    if (!valid_position(pp, col, row, rot, g))
	return 0;

    screen_to_grid_coords(g, blockWidth, screen_x, screen_y+19, &row2, &col);

    if (row == row2) return 1;	/* no need to recheck, you were aligned */
    else return valid_position(pp, col, row2, rot, g);
}

/***************************************************************************
 *      tetris_event()
 * Do some work associated with a collision. Needs the color style to draw
 * the grid.
 *************************************************************************/
int tetris_event(int *delay, int count, SDL_Surface * screen, 
	piece_style *ps, color_style *cs, sound_style *ss, Grid g[], int
	level, int fall_event_interval, int sock, int draw,
	int *blank, int *garbage, int P)
{
    if (count == 1) { /* determine if anything happened, sounds */
	int i;

	State[P].check_result = check_tetris(g);
	State[P].num_lines_cleared += State[P].check_result;

	if (State[P].check_result >= 3)
	    play_sound(ss,SOUND_CLEAR4,256);
	else for (i=0;i<State[P].check_result;i++)
	    play_sound(ss,SOUND_CLEAR1,256+6144*i);

	*delay = 1;
	return 2;

    } else if (count == 2) {	/* run gravity */
	int x,y;

	if (sock) { 
	    char msg = 'c'; /* WRW: send update */
	    send(sock,&msg,1,0);
	    send(sock,g[0].contents,sizeof(*g[0].contents)
		    * g[0].h * g[0].w,0); 
		    
	}
	draw_grid(screen,cs,&g[0],draw);

	/*
	 * recalculate falling, run gravity
	 */
	memcpy(g->temp,g->fall,g->w*g->h*sizeof(*(g->temp)));
	for (y=g->h-1;y>=0;y--) 
	    for (x=g->w-1;x>=0;x--) 
		FALL_SET(*g,x,y,UNKNOWN);
	run_gravity(&g[0]);
	memset(g->changed,0,g->h*g->w*sizeof(*(g->changed)));
	for (y=g->h-1;y>=0;y--) 
	    for (x=g->w-1;x>=0;x--) 
		if (TEMP_CONTENT(*g,x,y)!=FALL_CONTENT(*g,x,y)){
		    GRID_CHANGED(*g,x,y) = 1;
		    if (x > 0) GRID_CHANGED(*g,x-1,y) = 1;
		    if (y > 0) GRID_CHANGED(*g,x,y-1) = 1;
		    if (x < g->w-1) GRID_CHANGED(*g,x+1,y) = 1;
		    if (y < g->h-1) GRID_CHANGED(*g,x,y+1) = 1;
		}
	/* 
	 * check: did FALL_CONTENT change? 
	 */
	draw_grid(screen,cs,&g[0],draw);

	if (determine_falling(&g[0])) {
	    *delay = 1;
	    return 3;
	} else {
	    Score[P] += State[P].num_lines_cleared *
		State[P].num_lines_cleared * level;
	    if (sock) {
		char msg = 's'; /* WRW: send update */
		send(sock,&msg,1,0);
		send(sock,(char *)&Score[P],sizeof(Score[P]),0);
		if (State[P].num_lines_cleared >= 5) {
		    char msg = 'g'; /* WRW: send garbage! */
		    send(sock,&msg,1,0);
		    State[P].num_lines_cleared -= 4; /* might possibly also blank! */
		}
		if (State[P].num_lines_cleared >= 3) {
		    int i;
		    for (i=3;i<=State[P].num_lines_cleared;i++) {
			char msg = 'b'; /* WRW: send blanking! */
			send(sock,&msg,1,0);
		    }
		}
	    } else {
		if (State[P].num_lines_cleared >= 5) {
		    *garbage = 1;
		    State[P].num_lines_cleared -= 4;
		}
		if (State[P].num_lines_cleared >= 3) {
		    *blank = (State[P].num_lines_cleared - 2);
		}
	    }
	    draw_score(screen,P);
	    State[P].num_lines_cleared = 0;
	    return 0;
	}
    } else if (count >= 3 && count <= 22) {
	if (draw) 
	    draw_falling(screen, cs->w , &g[0], count - 2);

	/*
	*delay = max(fall_event_interval / 5,4);
	*/
	*delay = 4;
	return count + 1;
    } else if (count == 23) {
	fall_down(g);
	draw_grid(screen,cs,g,draw);
	if (run_gravity(g))
	    play_sound(ss,SOUND_THUD,0);
	/*
	*delay = max(fall_event_interval / 5,4);
	*/
	*delay = 4; 
	if (determine_falling(&g[0])) 
	    return 3;
	if (check_tetris(g))
	    return 1;
	else /* cannot be 0: we must redraw without falling */
	    return 2;
    }
    return 0;	/* not done yet */
}

/***************************************************************************
 *      do_blank()
 * Blank the visual screen of the given (local) player. 
 ***************************************************************************/
static void
do_blank(SDL_Surface *screen, sound_style *ss[2], Grid g[], int P)
{
    play_sound(ss[P],SOUND_GARBAGE1,1);
    if (State[P].draw) {
	State[P].next_draw = SDL_GetTicks() + 1000;
	State[P].draw_timeout = 1000;
	SDL_FillRect(screen, &g[P].board, 
		SDL_MapRGB(screen->format,32,32,32));
	SDL_UpdateSafe(screen, 1, &g[P].board);
    }  else {
	State[P].next_draw += 1000;
	State[P].draw_timeout += 1000;
    }
    { int i,j;
	for (j=0;j<g[0].h;j++)
	    for (i=0;i<g[0].w;i++)
		GRID_CHANGED(distract_grid[P],i,j) = 0;
    }
    State[P].draw = 0;
}

/***************************************************************************
 *      bomb_fun()
 * Function for the bomb special piece.
 ***************************************************************************/
static void
bomb_fun(int x, int y, Grid *g) 
{
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_SET(*g,x,y,REMOVE_ME);
}

static int most_common = 1;

/***************************************************************************
 *      colorkill_fun()
 * Function for the repainting special piece.
 ***************************************************************************/
static void
colorkill_recurse(int x, int y, Grid *g, int target_color)
{
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    if (GRID_CONTENT(*g,x,y) != target_color) return;
    GRID_SET(*g,x,y,REMOVE_ME);
    colorkill_recurse(x - 1, y, g, target_color);
    colorkill_recurse(x + 1, y, g, target_color);
    colorkill_recurse(x, y - 1, g, target_color);
    colorkill_recurse(x, y + 1, g, target_color);
}

static void
colorkill_fun(int x, int y, Grid *g)
{
    int c;
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    c = GRID_CONTENT(*g,x,y);
    if (c <= 1 || c == REMOVE_ME) return;
    GRID_SET(*g,x,y,REMOVE_ME);
    colorkill_recurse(x - 1, y, g, c);
    colorkill_recurse(x + 1, y, g, c);
    colorkill_recurse(x, y - 1, g, c);
    colorkill_recurse(x, y + 1, g, c);
}

/***************************************************************************
 *      repaint_fun()
 * Function for the repainting special piece.
 ***************************************************************************/
static void
repaint_fun(int x, int y, Grid *g) 
{
    int c;
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    c = GRID_CONTENT(*g,x,y);
    if (c <= 1 || c == most_common) return;
    GRID_SET(*g,x,y,most_common);
    repaint_fun(x - 1, y, g);
    repaint_fun(x + 1, y, g);
    repaint_fun(x, y - 1, g);
    repaint_fun(x, y + 1, g);
}

/***************************************************************************
 * 	push_down()
 * Teleport all of the pieces just below this special piece as far down as
 * they can go in their column. 
 ***************************************************************************/
static void
push_down(play_piece *pp, int col, int row, int rot, Grid *g,
	void (*fun)(int, int, Grid *))
{
    int i,j,c;
    int place_y, look_y;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col;
		int t_y = j + row;
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		GRID_SET(*g,t_x,t_y,REMOVE_ME);
		look_y = t_y + 1;
		if (look_y >= g->h) continue;
		if (!GRID_CONTENT(*g,t_x,look_y)) continue;
		/* OK, try to move look_y down as far as possible */
		for (place_y = g->h-1; 
			place_y > look_y &&
			GRID_CONTENT(*g,t_x,place_y) != 0;
			place_y --)
		    ;
		if (place_y == look_y) continue;
		/* otherwise, valid swap! */
		if (place_y < 0 || place_y >= g->h) 
		    continue;
		if (look_y < 0 || look_y >= g->h) 
		    continue;
		GRID_SET(*g,t_x,place_y, GRID_CONTENT(*g,t_x,look_y));
		GRID_SET(*g,t_x,look_y, REMOVE_ME);
	    }
}

/***************************************************************************
 *      find_on_board()
 * Finds where on the board a piece would go: used by special piece
 * handling routines. 
 ***************************************************************************/
static void
find_on_board(play_piece *pp, int col, int row, int rot, Grid *g,
	void (*fun)(int, int, Grid *))
{
    int i,j,c;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col;
		int t_y = j + row; 
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		GRID_SET(*g,t_x,t_y,REMOVE_ME);
	    }

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col; 
		int t_y = j + row;
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		fun(t_x - 1, t_y, g);
		fun(t_x + 1, t_y, g);
		fun(t_x, t_y - 1, g);
		fun(t_x, t_y + 1, g);
	}
    return;
}

/***************************************************************************
 *      most_common_color()
 * Finds the most common (non-zero, non-garbage) color on the board. If
 * none, returns the garbage color. 
 ***************************************************************************/
static void
most_common_color(Grid *g)
{
    int count[256];
    int x,y,c;
    int max, max_count;

    memset(count,0, sizeof(count));
    for (x=0;x<g->w;x++)
	for (y=0;y<g->h;y++) {
	    c = GRID_CONTENT(*g,x,y);
	    if (c > 1) 
		count[c]++;
	}
    max = 1;
    max_count = 0;
    for (x=2;x<256;x++) 
	if (count[x] > max_count) {
	    max = x;
	    max_count = count[x];
	}
    most_common = max;
    return;
}

/***************************************************************************
 *      handle_special()
 * Change the state of the grid based on the magical special piece ...
 * Look in pos[P] for the current position.
 *********************************************************************PROTO*/
void
handle_special(play_piece *pp, int row, int col, int rot, Grid *g,
	sound_style *ss)
{
    switch (pp->special) {
	case No_Special: break;
	case Special_Bomb: 
			 find_on_board(pp, col, row, rot, g, bomb_fun);
			 if (ss) 
			     play_sound(ss,SOUND_CLEAR1,256);
			 break;
	case Special_Repaint: 
			 most_common_color(g);
			 find_on_board(pp, col, row, rot, g, repaint_fun);
			 if (ss) 
			     play_sound(ss,SOUND_GARBAGE1,256);
			 break;
	case Special_Pushdown: 
			 push_down(pp, col, row, rot, g, repaint_fun);
			 if (ss) 
			     play_sound(ss,SOUND_THUD,256*2);
			 break;
	case Special_Colorkill: 
			 find_on_board(pp, col, row, rot, g, colorkill_fun);
			 if (ss) 
			     play_sound(ss,SOUND_CLEAR1,256);
			 break;

    }
}

/***************************************************************************
 *      do_pause()
 * Change the pause status of the local player.
 ***************************************************************************/
static void
do_pause(int paused, Uint32 tv_now, int *pause_begin_time, int *tv_start)
{
    int i;
    draw_pause(paused);
    if (!paused) {
	/* fixup times */
	*tv_start += (tv_now - *pause_begin_time);
	for (i=0; i<2; i++) {
	    State[i].collide_time += (tv_now - *pause_begin_time);
	    State[i].next_draw += (tv_now - *pause_begin_time);
	    State[i].tv_next_fall += (tv_now - *pause_begin_time);
	    State[i].tv_next_tetris += (tv_now - *pause_begin_time);
	    State[i].tv_next_ai_think += (tv_now - *pause_begin_time);
	    State[i].tv_next_ai_move += (tv_now - *pause_begin_time);
	}
    } else {
	*pause_begin_time = tv_now;
    }
}

/***************************************************************************
 *      place_this_piece()
 * Given that the player's current piece structure is already chosen, try
 * to place it near the top of the board. this may involve shifting it up 
 * a bit or rotating it or something. 
 *
 * Returns 0 on success. 
 ***************************************************************************/
static int
place_this_piece(int P, int blockWidth, Grid g[])
{
    int Y, R;
    /* we'll try Y adjustments from -2 to 0 and rotations from 0 to 3 */

    pos[P].x = pos[P].old_x = g[P].board.x + g[P].board.w / 2;
    for (Y = 0; Y >= -2 ; Y --) {
	for (R = 0; R <= 3; R++) {
	    pos[P].y = pos[P].old_y = g[P].board.y + (blockWidth * Y);
	    pos[P].old_rot = pos[P].rot = R;
	    if (valid_screen_position(&State[P].cp, blockWidth,
			&g[P], pos[P].rot, pos[P].x, pos[P].y)) {
		return 0;
	    }
	}
    }
    return 1;	/* no valid position! */
}

/***************************************************************************
 *      event_loop()
 * The main event-processing dispatch loop. 
 *
 * Returns 0 on a successful game completion, -1 on a [single-user] quit.
 *********************************************************************PROTO*/
#define		NO_PLAYER	0
#define		HUMAN_PLAYER	1
#define		AI_PLAYER	2
#define		NETWORK_PLAYER	3
int
event_loop(SDL_Surface *screen, piece_style *ps, color_style *cs[2], 
	sound_style *ss[2], Grid g[], int level[2], int sock,
	int *seconds_remaining, int time_is_hard_limit,
	int adjust[], int (*handle)(const SDL_Event *), 
	int seed, int p1, int p2, AI_Player *AI[2])
{
    SDL_Event event;
    Uint32 tv_now, tv_start; 
    int NUM_PLAYER = 0;
    int NUM_KEYBOARD = 0;
    int last_seconds = -1;
    int minimum_fall_event_interval = 100;
    int paused = 0;
    Uint32 pause_begin_time = 0;

    /* measured in milliseconds, 25 frames per second:
     * 1 frame = 40 milliseconds
     */
    int blockWidth = cs[0]->w;
    int i,j,P, Q;

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY/Options.key_repeat_delay,
	    SDL_DEFAULT_REPEAT_INTERVAL/2);

    if (gametype != DEMO)
	stop_all_playing(); 

    memset(pos, 0, sizeof(pos[0]) * 2);
    memset(State, 0, sizeof(State[0]) * 2);

    switch (p1) {
	case NO_PLAYER: Assert(!handle); break;
	case HUMAN_PLAYER: Assert(!handle); NUM_PLAYER++; NUM_KEYBOARD++; break;
	case AI_PLAYER: State[0].ai = 1; NUM_PLAYER++; break;
	case NETWORK_PLAYER: PANIC("Cannot have player 1 over the network!");
    }
    switch (p2) {
	case NO_PLAYER: break;
	case HUMAN_PLAYER: Assert(!handle); NUM_PLAYER++; NUM_KEYBOARD++; break;
	case AI_PLAYER: State[1].ai = 1; NUM_PLAYER++; break;
	case NETWORK_PLAYER: Assert(sock); break;
    }
    Assert(NUM_PLAYER >= 1 && NUM_PLAYER <= 2);

    tv_start = tv_now = SDL_GetTicks();
    tv_start += *seconds_remaining * 1000; 

    for (P=0; P<NUM_PLAYER; P++) {
	State[P].falling = 1;
	State[P].fall_speed = 1;
	State[P].tetris_handling = 0;
	State[P].accept_input = 1;
	State[P].limbo = 0;
	State[P].draw = 1;
	State[P].other_in_limbo = 0;
	State[P].next_draw = 0;
	State[P].limbo_sent = 0;
	State[P].cp = generate_piece(ps, cs[P], seed);
	State[P].np = generate_piece(ps, cs[P], seed+1);
	State[P].seed = seed+2;
	State[P].ready_for_fast = 1;
	State[P].ready_for_rotate = 1;

	draw_next_piece(screen, ps, cs[P], &State[P].cp, &State[P].np, P);

	adjust[P] = -1;

	if (SPEED_LEVEL(level[P]) <= 7)
	    State[P].fall_event_interval = 45 - SPEED_LEVEL(level[P]) * 5;
	else 
	    State[P].fall_event_interval = 16 - SPEED_LEVEL(level[P]);
	if (State[P].fall_event_interval < 1)
	    State[P].fall_event_interval = 1;

	if (State[P].fall_event_interval < minimum_fall_event_interval)
	    minimum_fall_event_interval = State[P].fall_event_interval;

	State[P].tv_next_fall = tv_now + State[P].fall_event_interval;

	if (place_this_piece(P, blockWidth, g)) {
	    /* failed to place piece initially ... */
	    pos[P].x = pos[P].old_x = g[P].board.x + g[P].board.w / 2;
	    pos[P].y = pos[P].old_y = g[P].board.y ;
	    pos[P].rot = pos[P].old_rot = 0;
	}

	if (State[P].ai) {
	    State[P].tv_next_ai_think = tv_now;
	    State[P].tv_next_ai_move = tv_now;
	    if (gametype == DEMO || gametype == AI_VS_AI ||
		    AI[P]->delay_factor == 0) {
		State[P].ai_interval = State[P].fall_event_interval;
		if (State[P].ai_interval > 15)
		    State[P].ai_interval = 15;
	    } else {
		if (AI[P]->delay_factor < 1)
		    AI[P]->delay_factor = 1;
		if (AI[P]->delay_factor > 100)
		    AI[P]->delay_factor = 100;
		State[P].ai_interval = AI[P]->delay_factor;
	    }
	    State[P].ai_state = AI[P]->reset(State[P].ai_state, &g[P]);
	}
    }


    /* generate the fake-out grid: shown when the opponent does something
     * good! */
    distract_grid[0] = generate_board(g[0].w,g[0].h,g[0].h-2);
    distract_grid[0].board = g[0].board;

    SeedRandom(seed);
    for (i=0;i<g[0].w;i++)
	for (j=0;j<g[0].h;j++) {
	    GRID_SET(distract_grid[0],i,j,ZEROTO(cs[0]->num_color));
	}
    if (NUM_PLAYER == 2) {
	distract_grid[1] = generate_board(g[1].w,g[1].h,g[1].h-2);
	distract_grid[1].board = g[1].board;
	for (i=0;i<g[1].w;i++)
	    for (j=0;j<g[1].h;j++) {
		GRID_SET(distract_grid[1],i,j,ZEROTO(cs[1]->num_color));
	    }
    }

    if (sock) { 
	char msg = 'c'; /* WRW: send update */
	send(sock,&msg,1,0);
	send(sock,g[0].contents,sizeof(*g[0].contents)
		* g[0].h * g[0].w,0); 
		
    }

    draw_clock(0);

    draw_grid(screen,cs[0],&g[0],1);
    draw_score(screen, 0);
    if (NUM_PLAYER == 2) {
	draw_grid(screen,cs[1],&g[1],1);
	draw_score(screen,1);
    }
    if (sock)
	draw_score(screen, 1);

    /* 
     * Major State-Machine Event Loop
     */

    P = 0;

    while (1) { 

	if (NUM_PLAYER == 2)
	    P = !P;

	tv_now = SDL_GetTicks();

	/* update the on-screen clock */
	if (tv_start >= tv_now)
	    * seconds_remaining = (tv_start - tv_now) / 1000;
	else
	    * seconds_remaining = - ((tv_now - tv_start) / 1000);

	if (*seconds_remaining != last_seconds && !paused) {
	    last_seconds = *seconds_remaining;
	    draw_clock(*seconds_remaining);
	    if (last_seconds <= 30 && last_seconds >= 0) {
		play_sound_unless_already_playing(ss[0],SOUND_CLOCK,0);
		if (NUM_PLAYER == 2) 
		    play_sound_unless_already_playing(ss[1],SOUND_CLOCK,0);
	    }
	}

	/* check for time-out */
	if (*seconds_remaining < 0 && time_is_hard_limit && !paused) { 
	    play_sound(ss[0],SOUND_LEVELDOWN,0);
	    adjust[0] = ADJUST_DOWN;
	    if (NUM_PLAYER == 2) {
		play_sound(ss[1],SOUND_LEVELDOWN,0);
		adjust[1] = ADJUST_DOWN;
	    }
	    stop_playing_sound(ss[0],SOUND_CLOCK);
	    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
	    return 0;
	} 

	/*
	 * 	Visual Events
	 */
	if (!State[P].draw && tv_now > State[P].next_draw && !paused) {
	    int i,j;
	    State[P].draw = 1;
	    for (i=0;i<g[P].w;i++)
		for (j=0;j<g[P].h;j++) {
		    GRID_CHANGED(g[P],i,j) = 1;
		    if (GRID_CONTENT(g[P],i,j) == 0)
			GRID_SET(g[P],i,j,REMOVE_ME);
		}
	    draw_grid(screen,cs[P],&g[P],1);
	} else if (!State[P].draw) {
	    int delta = State[P].next_draw - tv_now;
	    int amt = g[P].h - ((g[P].h * delta) / State[P].draw_timeout);
	    int i,j;
	    j = amt - 1;
	    if (j < 0) j = 0;
	    for (i=0;i<g[P].w;i++)
		GRID_CHANGED(distract_grid[P],i,j) = 1;
	    draw_grid(screen,cs[P],&distract_grid[P],1);
	}

	/* 
	 *	Falling Events
	 */

	if (State[P].falling && tv_now >= State[P].tv_next_fall && !paused) {
	    int try;
	    int we_fell = 0;

#if DEBUG
	    if (tv_now > State[P].tv_next_fall) 
		Debug("Fall: %d %d\n", tv_now, tv_now - State[P].tv_next_fall);
#endif

	    /* ok, we had a falling event */
	    do {
		State[P].tv_next_fall += State[P].fall_event_interval;
	    } while (State[P].tv_next_fall <= tv_now); 

	    for (try = State[P].fall_speed; try > 0; try--)
		if (valid_screen_position(&State[P].cp,blockWidth,&g[P],pos[P].rot,pos[P].x,pos[P].y+try)) {
		    pos[P].y += try;
		    State[P].fall_speed = try;
		    try = 0;
		    we_fell = 1;
		}
	    if (!we_fell) {
		if (!State[P].collide_time) {
		    State[P].collide_time = tv_now + 
			(Options.long_settle_delay ? 400 : 200);
		    continue; /* don't fall */
		}
		if (tv_now < State[P].collide_time)
		    continue; /* don't fall */
		/* do fall! */
	    } else continue;
	    /* collided! */

	    State[P].collide_time = 0;

	    play_sound(ss[P],SOUND_THUD,0);

	    /* this would only come into play if you were halfway
	     * between levels and suddenly someone added garbage */
	    while (!valid_screen_position(&State[P].cp,blockWidth,&g[P],pos[P].rot,pos[P].x,pos[P].y) && pos[P].y > 0) 
		pos[P].y--;

	    if (State[P].draw) 
		draw_play_piece(screen, cs[P], &State[P].cp, pos[P].old_x, pos[P].old_y, pos[P].old_rot,
			&State[P].cp, pos[P].x, pos[P].y, pos[P].rot);

	    if (State[P].cp.special != No_Special) {
		/* handle special powers! */
		int row, col;
		screen_to_exact_grid_coords(&g[P], blockWidth, 
			pos[P].x, pos[P].y, &row, &col);
		handle_special(&State[P].cp, row, col, pos[P].rot, &g[P], ss[P]);
	    } else { 
		/* paste the piece on the board */
		int row, col;
		screen_to_exact_grid_coords(&g[P], blockWidth, 
			pos[P].x, pos[P].y, &row, &col);
		paste_on_board(&State[P].cp, col, row, pos[P].rot, &g[P]);
	    }

	    if (sock) { 
		char msg = 'c'; /* WRW: send update */
		send(sock,&msg,1,0);
		send(sock,g[P].contents,sizeof(*g[P].contents)
			* g[P].h * g[P].w,0); 
	    }
	    draw_grid(screen,cs[P],&g[P],State[P].draw);

	    /* state change */
	    State[P].falling = 0;
	    State[P].fall_speed = 0;
	    State[P].tetris_handling = 1;
	    State[P].accept_input = 0;
	    State[P].tv_next_tetris = tv_now;
	}
	/* 
	 *	Tetris Clear Events
	 */
	if (State[P].tetris_handling != 0 && tv_now >= State[P].tv_next_tetris && !paused) {
	    int blank = 0, garbage = 0;

#if DEBUG 
	    if (tv_now >= State[P].tv_next_tetris) 
		Debug("Tetr: %d %d (%d)\n", tv_now, tv_now -
			State[P].tv_next_tetris, State[P].tetris_handling);
#endif

	    State[P].tetris_handling = tetris_event(
		    &State[P].tetris_event_interval, 
		    State[P].tetris_handling, screen, ps, cs[P], ss[P], &g[P], 
		    level[P], minimum_fall_event_interval, sock, State[P].draw,
		    &blank, &garbage, P);

	    if (NUM_PLAYER == 2) {
		if (blank)
		    do_blank(screen, ss, g, !P);
		if (garbage) {
		    add_garbage(&g[!P]);
		    play_sound(ss[!P],SOUND_GARBAGE1,1);
		    draw_grid(screen,cs[!P],&g[!P],State[!P].draw);
		}
	    }

	    tv_now = SDL_GetTicks();
	    do {  /* just in case we're *way* behind */
		State[P].tv_next_tetris += State[P].tetris_event_interval;
	    } while (State[P].tv_next_tetris < tv_now); 

	    if (State[P].tetris_handling == 0) { /* state change */
		/* Time for your next piece ...
		 * Yujia points out that we should try a little harder to
		 * fit your piece on the board. 
		 */
		State[P].cp = State[P].np;
		State[P].np = generate_piece(ps, cs[P], State[P].seed++);
		draw_next_piece(screen, ps, cs[P], 
			&State[P].cp, &State[P].np, P);

		if (place_this_piece(P, blockWidth, g)) {
		    /* failed to place piece */
you_lose: 
		    play_sound(ss[P],SOUND_LEVELDOWN,0);
		    adjust[P] = ADJUST_DOWN;
		    if (NUM_PLAYER == 2 && !sock)
			adjust[!P] = ADJUST_SAME;
		    if (sock == 0) {
			stop_playing_sound(ss[0],SOUND_CLOCK);
			if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
			return 0;
		    }
		    /*
		    Debug("Entering Limbo: adjust down.\n");
		    */
		    State[P].falling = 0;
		    State[P].fall_speed = 0;
		    State[P].tetris_handling = 0;
		    State[P].accept_input = 0;
		    State[P].limbo = 1;
		} else {
		    int x,y,count = 0;
		    if (State[P].ai) 
			State[P].ai_state =
			    AI[P]->reset(State[P].ai_state, &g[P]);
		    for (y=0;y<g->h;y++)
			for (x=0;x<g->w;x++)
			    if (GRID_CONTENT(g[P],x,y) == 1)
				count++;
		    if (count == 0) {
you_win: 
			play_sound(ss[P],SOUND_LEVELUP,256);
			if (*seconds_remaining <= 0) { 
			    adjust[P] = ADJUST_SAME;
			    if (NUM_PLAYER == 2 && !sock)
				adjust[!P] = ADJUST_DOWN;
			} else {
			    adjust[P] = ADJUST_UP;
			    if (NUM_PLAYER == 2 && !sock)
				adjust[!P] = ADJUST_SAME;
			}
			if (sock == 0) {
			    stop_playing_sound(ss[0],SOUND_CLOCK);
			    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
			    return 0;
			}
			/*
			Debug("Entering Limbo: you win, adjust ?/?.\n");
			*/
			State[P].falling = 0;
			State[P].fall_speed = 0;
			State[P].tetris_handling = 0;
			State[P].accept_input = 0;
			State[P].limbo = 1;
		    } else {
			/* keep playing */
			State[P].falling = 1;
			State[P].fall_speed = 1;
			State[P].tetris_handling = 0;
			State[P].accept_input = 1;
			State[P].tv_next_fall = SDL_GetTicks() + 
			    State[P].fall_event_interval;
		    }
		}
	    } 
	} /* end: handle tetris events */

	/* 
	 *	AI Events
	 */
	if (State[P].ai && tv_now >= State[P].tv_next_ai_think && !paused) {
	    int row, col;
#ifdef AI_THINK_TIME
	    Uint32 tv_before = SDL_GetTicks();
#endif

	    screen_to_grid_coords(&g[P], blockWidth, pos[P].x, pos[P].y, &row, &col);

	    /* simulate blanked screens */
	    if (State[P].draw) 
		AI[P]->think(State[P].ai_state, &g[P], &State[P].cp, &State[P].np, col, row, pos[P].rot);

#ifdef AI_THINK_TIME
	    tv_now = SDL_GetTicks();
	    if (tv_now > tv_before + 1)
		Debug("AI[%s] took too long in think() [%d ticks].\n",
			AI[P]->name, tv_now - tv_before);
#endif

	    do {  /* just in case we're *way* behind */
		State[P].tv_next_ai_think += 
		    State[P].ai_interval;
	    } while (State[P].tv_next_ai_think < tv_now); 
	}
	if (State[P].ai && State[P].accept_input &&
		tv_now >= State[P].tv_next_ai_move && !paused) {
	    int row, col;
#ifdef AI_THINK_TIME
	    Uint32 tv_before = SDL_GetTicks();
#endif

	    screen_to_grid_coords(&g[P], blockWidth, pos[P].x, pos[P].y, &row, &col);
	    pos[P].move =
		AI[P]->move(State[P].ai_state, &g[P], &State[P].cp, &State[P].np, 
			col, row, pos[P].rot);
#ifdef AI_THINK_TIME
	    tv_now = SDL_GetTicks();
	    if (tv_now > tv_before + 1)
		Debug("AI[%s] took too long in move() [%d ticks].\n",
			AI[P]->name, tv_now - tv_before);
#endif

	    do {  /* just in case we're *way* behind */
		State[P].tv_next_ai_move += 
		    State[P].ai_interval * 5;
	    } while (State[P].tv_next_ai_move < tv_now); 
	}

	/* 
	 * 	User Interface Events 
	 */

	if (SDL_PollEvent(&event)) {

	    /* special menu handling! */
	    if (handle) {
		if (handle(&event)) {
		    return -1;
		}
	    } else switch (event.type) {
		case SDL_KEYUP:
		    /* "down" will not affect you again until you release
		     * the down key and press it again */
		    if (event.key.keysym.sym == SDLK_DOWN) {
			State[1].ready_for_fast = 1;
			if (NUM_KEYBOARD == 1)
			    State[0].ready_for_fast = 1;
		    }
		    else if (event.key.keysym.sym == SDLK_UP) {
			State[1].ready_for_rotate = 1;
			if (NUM_KEYBOARD == 1)
			    State[0].ready_for_rotate = 1;
		    }
		    else if (event.key.keysym.sym == SDLK_w)
			State[0].ready_for_rotate = 1;
		    else if (event.key.keysym.sym == SDLK_s)
			State[0].ready_for_fast = 1;
		    else if (event.key.keysym.sym == SDLK_1) {
			P = 0;
			goto you_win;
		    }
		    else if (event.key.keysym.sym == SDLK_2) {
			P = 0;
			goto you_lose;
		    }
		    else if (event.key.keysym.sym == SDLK_3) {
			P = 1;
			goto you_win;
		    }
		    else if (event.key.keysym.sym == SDLK_4) {
			P = 1;
			goto you_lose;
		    } else if (event.key.keysym.sym == SDLK_p && gametype != DEMO) {
			/* Pause it! */
			tv_now = SDL_GetTicks();
			paused = !paused;
			if (sock) { 
			    char msg = 'p'; /* WRW: send pause update */
			    send(sock,&msg,1,0);
			}
			do_pause(paused, tv_now, &pause_begin_time, &tv_start);
		    }

		    break;

		case SDL_KEYDOWN:
		    {
			int ks = event.key.keysym.sym;
			Q = -1;

			/* keys for P=0 */
			if (ks == SDLK_UP || ks == SDLK_DOWN ||
				ks == SDLK_RIGHT || ks == SDLK_LEFT) {
			    Q = 1;
			} else if (ks == SDLK_w || ks == SDLK_s ||
				ks == SDLK_a || ks == SDLK_d) {
			    Q = 0;
			} else if (ks == SDLK_q) {
			    if (sock == 0) {
				adjust[0] = -1;
				if (NUM_PLAYER == 2)
				    adjust[1] = -1;
				stop_playing_sound(ss[0],SOUND_CLOCK);
				if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
				return -1;
			    } else {
				/*
				Debug("Entering Limbo: adjust down.\n");
				*/
				State[0].falling = 0;
				State[0].fall_speed = 0;
				State[0].tetris_handling = 0;
				State[0].accept_input = 0;
				State[0].limbo = 1;
				adjust[0] = ADJUST_DOWN;
			    }
			} else if ((ks == SDLK_RETURN) && 
                            ((event.key.keysym.mod & KMOD_LCTRL) ||
                             (event.key.keysym.mod & KMOD_RCTRL))) {
                          SDL_WM_ToggleFullScreen(screen);
                          break; 
                        } else break;
			if (NUM_KEYBOARD == 1) Q = 0;
			else if (NUM_KEYBOARD < 1) break;
			/* humans cannot modify AI moves! */

			Assert(Q == 0 || Q == 1);

			if (event.key.keysym.sym != SDLK_DOWN &&
				event.key.keysym.sym != SDLK_s)
			    State[Q].fall_speed = 1;
			if (!State[Q].accept_input) {
			    break;
			}
		    /* only if we are accepting input */
			switch (event.key.keysym.sym) {
			    case SDLK_UP: case SDLK_w: 
				if (State[Q].ready_for_rotate)
				    pos[Q].move = MOVE_ROTATE;
				break;
			    case SDLK_DOWN: case SDLK_s: 
				if (State[Q].ready_for_fast)
				    pos[Q].move = MOVE_DOWN;
				break;
			    case SDLK_LEFT: case SDLK_a: 
				pos[Q].move = MOVE_LEFT; break;
			    case SDLK_RIGHT: case SDLK_d: 
				pos[Q].move = MOVE_RIGHT; break;
			    default: 
				PANIC("unknown keypress");
			}
		    }
		    break;
		case SDL_QUIT:
		    Debug("Window-manager exit request.\n");
		    adjust[0] = -1;
		    if (NUM_PLAYER == 2)
			adjust[1] = -1;
		    stop_playing_sound(ss[0],SOUND_CLOCK);
		    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		    return -1;
		case SDL_SYSWMEVENT:
		    break;
	    } /* end: switch (event.type) */
	} 

	/* 
	 *	Handle Movement 
	 */
	for (Q=0;Q<NUM_PLAYER;Q++) {
	    switch (pos[Q].move) {
		case MOVE_ROTATE: 
		    State[Q].ready_for_rotate = 0;
		    if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,pos[Q].x,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x-blockWidth,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].x -= blockWidth;
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x+blockWidth,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].x += blockWidth;
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x,pos[Q].y+blockWidth)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].y += blockWidth;
			State[Q].collide_time = 0;
		    } else if (Options.upward_rotation &&
			    valid_screen_position(&State[Q].cp,blockWidth,
				&g[Q],(pos[Q].rot+1)%4, pos[Q].x,
				pos[Q].y-blockWidth)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].y -= blockWidth;
			State[Q].collide_time = 0;
		    }
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_LEFT:
		    for (i=0;i<10;i++) 
			if (valid_screen_position(&State[Q].cp,blockWidth,
				    &g[Q],pos[Q].rot,
				    pos[Q].x-blockWidth,
				    pos[Q].y+i)) {
			    pos[Q].x -= blockWidth;
			    pos[Q].y += i;
			    State[Q].collide_time = 0;
			    break;
			}
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_RIGHT:
		    for (i=0;i<10;i++)
			if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],pos[Q].rot,
				    pos[Q].x+blockWidth,pos[Q].y+i)){
			    pos[Q].x += blockWidth;
			    pos[Q].y += i;
			    State[Q].collide_time = 0;
			    break;
			}
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_DOWN:
		    if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],pos[Q].rot,
				pos[Q].x, pos[Q].y+blockWidth))
			pos[Q].y+=blockWidth;
		    State[Q].fall_speed = 20;
		    State[Q].ready_for_fast = 0;
		    pos[Q].move = MOVE_NONE;
		    break;
		default: 
		    break;
	    }
	} /* endof: for each player, check move */


	if (State[P].falling && !paused) { 
	    if (State[P].draw) 
		if (pos[P].old_x != pos[P].x || pos[P].old_y != pos[P].y || pos[P].old_rot != pos[P].rot) {
		    draw_play_piece(screen, cs[P], &State[P].cp, pos[P].old_x, pos[P].old_y, pos[P].old_rot,
			    &State[P].cp, pos[P].x, pos[P].y, pos[P].rot);
		    pos[P].old_x = pos[P].x; pos[P].old_y = pos[P].y; pos[P].old_rot = pos[P].rot;
		}
	}

	/* network connection */
	if (sock) {
	    fd_set read_fds;
	    struct timeval timeout = { 0, 0 };
	    int retval;

	    Assert(P == 0);

	    do { 
		FD_ZERO(&read_fds);
		FD_SET(sock,&read_fds);
#if HAVE_SELECT || HAVE_WINSOCK_H
		retval = select(sock+1, &read_fds, NULL, NULL, &timeout);
#else
#warning	"Since you do not have select(), networking play will fail."
		retval = 0;
#endif
		if (retval > 0) {
		    char msg;
		    if (recv(sock,&msg,1,0) != 1) {
			Debug("WARNING: Other player has left?\n");
			close(sock);
			sock = 0;
			retval = 0;
		    } else {
			switch (msg) {
			    case 'b': 
				do_blank(screen, ss, g, P);
			    break;
			    case 'p':
				paused = !paused;
				tv_now = SDL_GetTicks();
				do_pause(paused, tv_now, &pause_begin_time, &tv_start);
			    break; 

			    case ADJUST_DOWN: /* other play in limbo */
			    case ADJUST_SAME:
			    case ADJUST_UP:
				State[P].other_in_limbo = 1;
				adjust[!P] = msg;
				break;

			    case 'g': 
				add_garbage(&g[P]);
				play_sound(ss[P],SOUND_GARBAGE1,1);
				draw_grid(screen,cs[P],&g[P],State[P].draw);
				      break;
			    case 's':
				  recv(sock,(char *)&Score[1], sizeof(Score[1]),0);
				  draw_score(screen, 1);
				  break;
			    case 'c':  { int i,j;
				      memcpy(g[!P].temp,g[!P].contents,
					      sizeof(*g[0].temp) *
					      g[!P].w * g[!P].h);
				      recv(sock,g[!P].contents,
					      sizeof(*g[!P].contents) *
					      g[!P].w * g[!P].h,0);
				      for (i=0;i<g[!P].w;i++)
					  for (j=0;j<g[!P].h;j++)
					      if (GRID_CONTENT(g[!P],i,j) != TEMP_CONTENT(g[1],i,j)){
						  GRID_CHANGED(g[!P],i,j)=1;
						  if (i > 0) GRID_CHANGED(g[!P],i-1,j) = 1;
						  if (j > 0) GRID_CHANGED(g[!P],i,j-1) = 1;
						  if (i < g[!P].w-1) GRID_CHANGED(g[!P],i+1,j) = 1;
						  if (i < g[!P].h-1) GRID_CHANGED(g[!P],i,j+1) = 1;
						  if (GRID_CONTENT(g[!P],i,j) == 0) GRID_SET(g[!P],i,j,REMOVE_ME);
						  }
				      draw_grid(screen,cs[!P],&g[!P],1);
				       }
				      break;
			    default: break;
			}
		    }
		}
	    } while (retval > 0 && 
		    !(State[P].limbo && State[P].other_in_limbo));

	    /* limbo handling */
	    if (State[P].limbo && State[P].other_in_limbo) {
		Assert(adjust[0] != -1 && adjust[1] != -1);
		stop_playing_sound(ss[0],SOUND_CLOCK);
		if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		return 0;
	    } else if (State[P].limbo && !State[P].other_in_limbo &&
		    !State[P].limbo_sent) {
		char msg = adjust[P]; /* WRW: send update */
		State[P].limbo_sent = 1;
		send(sock,&msg,1,0);
	    } else if (!State[P].limbo && State[P].other_in_limbo) {
		char msg; /* WRW: send update */
		/* hmm, other guy is done ... */
		if (adjust[!P] == ADJUST_UP || adjust[!P] == ADJUST_SAME) {
		    if (*seconds_remaining > 0) 
			adjust[P] = ADJUST_SAME;
		    else
			adjust[P] = ADJUST_DOWN;
		} else if (adjust[!P] == ADJUST_DOWN) {
		    adjust[P] = ADJUST_SAME;
		}
		/*
		Debug("Entering Limbo: adjust same/down.\n");
		*/
		State[P].falling = 0;
		State[P].fall_speed = 0;
		State[P].tetris_handling = 0;
		State[P].accept_input = 0;
		State[P].limbo = 1;
		State[P].limbo_sent = 1;
		msg = adjust[P];
		send(sock,&msg,1,0);
		stop_playing_sound(ss[0],SOUND_CLOCK);
		if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		return 0;
	    }
	}
	if (paused) {
	    atris_run_flame();
	}

	tv_now = SDL_GetTicks();
	{
	    Uint32 least = State[0].tv_next_fall;

	    if (State[0].tetris_handling && State[0].tv_next_tetris < least)
		least = State[0].tv_next_tetris;
	    if (State[0].ai && State[0].tv_next_ai_think < least)
		least = State[0].tv_next_ai_think;
	    if (State[0].ai && State[0].tv_next_ai_move < least)
		least = State[0].tv_next_ai_move;
	    if (NUM_PLAYER == 2) {
		if (State[1].tv_next_fall < least)
		    least = State[1].tv_next_tetris;
		if (State[1].tetris_handling && State[1].tv_next_tetris < least)
		    least = State[1].tv_next_tetris;
		if (State[1].ai && State[1].tv_next_ai_think < least)
		    least = State[1].tv_next_ai_think;
		if (State[1].ai && State[1].tv_next_ai_move < least)
		    least = State[1].tv_next_ai_move;
	    }

	    if (least >= tv_now + 4 && !SDL_PollEvent(NULL)) {
		/* hey, we could sleep for two ... */
		if (State[0].ai || State[1].ai) {
		    SDL_Delay(1);
		    if (State[0].ai && State[0].draw) {
			int row, col;
			screen_to_grid_coords(&g[0], blockWidth, pos[0].x, pos[0].y, &row, &col);
			AI[0]->think(State[0].ai_state, &g[0], &State[0].cp, &State[0].np, col, row, pos[0].rot);
		    } else SDL_Delay(1);
		    if (State[1].ai && State[1].draw) {
			int row, col;
			screen_to_grid_coords(&g[1], blockWidth, pos[1].x, pos[1].y, &row, &col);
			AI[1]->think(State[1].ai_state, &g[1], &State[1].cp, &State[1].np, col, row, pos[1].rot);
		    } else SDL_Delay(1);
		} else SDL_Delay(2);
	    } else if (least > tv_now && !SDL_PollEvent(NULL))
		SDL_Delay(least - tv_now);
	}
    } 
}

/*
 * $Log: event.c,v $
 * Revision 1.61  2001/01/05 21:12:32  weimer
 * advance to atris 1.0.5, add support for ".atrisrc" and changing the
 * keyboard repeat rate
 *
 * Revision 1.60  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.59  2000/11/06 04:39:56  weimer
 * networking consistency check for power pieces
 *
 * Revision 1.58  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.57  2000/11/06 01:25:54  weimer
 * add in the other special piece
 *
 * Revision 1.56  2000/11/06 00:51:55  weimer
 * fixed the paint thing so that it actually paints
 *
 * Revision 1.55  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.54  2000/11/03 04:25:58  weimer
 * add some optimizations to run_gravity to make it just a bit faster (down
 * to 0.01 ms/call from 0.02), sleep a bit more in event-loop: generally
 * trying to make us more CPU friendly ...
 *
 * Revision 1.53  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.52  2000/11/01 17:55:33  weimer
 * remove ambiguous references to "you win" in debugging output
 *
 * Revision 1.51  2000/11/01 04:39:41  weimer
 * clear the little scoring spot correctly, updates for making a no-sound
 * distribution
 *
 * Revision 1.50  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.49  2000/10/30 16:57:24  weimer
 * minor doc fixups
 *
 * Revision 1.48  2000/10/30 16:25:25  weimer
 * display the network player score during network games. Also give the
 * non-server a little message when waiting for the server to go on.
 *
 * Revision 1.47  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.46  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.45  2000/10/29 19:04:32  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.44  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.43  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.42  2000/10/28 13:39:14  weimer
 * added a pausing feature ...
 *
 * Revision 1.41  2000/10/27 00:07:36  weimer
 * some sound fixes ...
 *
 * Revision 1.40  2000/10/26 22:23:07  weimer
 * double the effective number of levels
 *
 * Revision 1.39  2000/10/22 22:05:51  weimer
 * Added a few new sounds ...
 *
 * Revision 1.38  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.37  2000/10/20 01:32:09  weimer
 * Minor play issue problems -- time is now truly a hard limit!
 *
 * Revision 1.36  2000/10/19 22:30:55  weimer
 * make pieces fall a bit faster
 *
 * Revision 1.35  2000/10/19 22:06:51  weimer
 * minor changes ...
 *
 * Revision 1.34  2000/10/19 00:20:27  weimer
 * sound directory changes, added a ticking clock ...
 *
 * Revision 1.33  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.32  2000/10/18 03:29:56  weimer
 * fixed problem with "machine-gun" sounds caused by "run_gravity()" returning
 * 1 too often
 *
 * Revision 1.31  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.30  2000/10/14 16:17:41  weimer
 * level adjustment changes, added some new AIs, etc.
 *
 * Revision 1.29  2000/10/14 01:42:53  weimer
 * better scoring of thumbs-up, thumbs-down
 *
 * Revision 1.28  2000/10/14 01:24:34  weimer
 * fixed error with not advancing levels when fighting AI
 *
 * Revision 1.27  2000/10/14 00:58:32  weimer
 * fixed an unsigned arithmetic problem that was sleeping too often!
 *
 * Revision 1.26  2000/10/14 00:56:41  weimer
 * whoops, minor boolean typo!
 *
 * Revision 1.25  2000/10/14 00:42:27  weimer
 * fixed minor error where you couldn't move all the way to the left ...
 *
 * Revision 1.24  2000/10/13 22:34:26  weimer
 * minor wessy AI changes
 *
 * Revision 1.23  2000/10/13 18:23:28  weimer
 * fixed a race condition in tetris_event()
 *
 * Revision 1.22  2000/10/13 17:55:36  weimer
 * Added another color "Style" ...
 *
 * Revision 1.21  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.20  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.19  2000/10/13 03:02:05  wkiri
 * Added Aliz (Kiri AI) to ai.c.
 * Note event.c currently calls Aliz, not Wessy AI.
 *
 * Revision 1.18  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.17  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.16  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.15  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.14  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.13  2000/09/05 22:10:46  weimer
 * fixed initial garbage generation strangeness
 *
 * Revision 1.12  2000/09/05 21:47:41  weimer
 * major timing revamping: it actually seems to work now!
 *
 * Revision 1.11  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.10  2000/09/04 15:20:21  weimer
 * faster blitting algorithms for drawing the grid and drawing falling pieces
 * (when you eliminate a line)
 *
 * Revision 1.9  2000/09/04 14:18:09  weimer
 * flushed out the pattern color so that it has the same number of colors as
 * the default
 *
 * Revision 1.8  2000/09/03 20:58:22  weimer
 * yada
 *
 * Revision 1.7  2000/09/03 20:57:02  weimer
 * changes the time variable to be passed by reference in the event loop
 *
 * Revision 1.6  2000/09/03 20:34:42  weimer
 * only draw their score in multiplayer game
 *
 * Revision 1.5  2000/09/03 20:27:38  weimer
 * the event loop actually works!
 *
 * Revision 1.4  2000/09/03 19:59:56  weimer
 * wessy event-loop changes
 *
 * Revision 1.3  2000/09/03 18:44:36  wkiri
 * Cleaned up atris.c (see high_score_check()).
 * Added game type (defaults to MARATHON).
 *
 */
