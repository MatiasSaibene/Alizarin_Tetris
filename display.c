/*
 *                               Alizarin Tetris
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <unistd.h>

#include "atris.h"
#include "display.h"
#include "grid.h"
#include "piece.h"

#include ".protos/xflame.pro"

struct layout_struct {
    /* the whole board layout */
    SDL_Rect grid_border[2];
    SDL_Rect grid[2];

    SDL_Rect score[2];

    SDL_Rect name[2];

    struct adjust_struct {
	SDL_Rect symbol[3];
    } adjust[2];

    SDL_Rect time;
    SDL_Rect time_border;

    SDL_Rect next_piece_border[2];
    SDL_Rect next_piece[2];

    SDL_Rect 	  pause;
} layout;

/* The background image, global so we can do updates */
SDL_Surface *adjust_symbol[3] = { NULL, NULL, NULL };
/* Rectangles that specify where my/their pieces can legally be */

int Score[2];

/***************************************************************************
 *      poll_and_flame()
 * Poll for events and run the flaming background.
 *********************************************************************PROTO*/
void
poll_and_flame(SDL_Event *ev)
{
    while (!(SDL_PollEvent(ev))) {
	atris_run_flame();
    }
    return;
}

/***************************************************************************
 *      clear_screen_to_flame()
 * Clears the screen so that only the flame layer is visible.
 *********************************************************************PROTO*/
void
clear_screen_to_flame(void)
{
    SDL_Rect all;

    all.x = all.y = 0;
    all.w = screen->w;
    all.h = screen->h;

    SDL_FillRect(widget_layer, &all, int_black);
    SDL_FillRect(screen, &all, int_black);
    SDL_BlitSafe(flame_layer, NULL, screen, NULL);
    SDL_UpdateSafe(screen, 1, &all);
}

/***************************************************************************
 *      setup_colors()
 * Sets up our favorite colors.
 *********************************************************************PROTO*/
void
setup_colors(SDL_Surface *screen)
{
    int i = 0;

    color_white.r = color_white.g = color_white.b = 0xFF;
    color_white.unused = 0;

    color_black.r = color_black.g = color_black.b = color_black.unused = 0;

    color_red.r = 0xFF; color_red.g = color_red.b = color_red.unused = 0;

    color_blue.b = 0xFF; color_blue.r = color_blue.g = color_blue.unused = 0;

    color_purple.b = 128; color_purple.r = 128; color_purple.g = 64;
	color_purple.unused = 0;

    int_black = SDL_MapRGB(screen->format, 0, 0, 0);

    /* find the darkest black that is different from "all black" */
    do {
	i++;
	int_solid_black = SDL_MapRGB(screen->format, i, i, i);
    } while (int_black == int_solid_black && i < 255);
    /* couldn't find one?! */
    if (i == 255) {
	Debug("*** Warning: transparency is compromised by RGB format.\n");
	int_solid_black = SDL_MapRGB(screen->format, 1, 1, 1);
    } else if (i != 1)
	Debug("*** Note: First non-black color found at magnitude %d.\n",i);

    int_white = SDL_MapRGB(screen->format, 255, 255, 255);
    int_grey  = SDL_MapRGB(screen->format, 128, 128, 128);
    int_blue  = SDL_MapRGB(screen->format, 0, 0, 255);
    int_med_blue = SDL_MapRGB(screen->format, 0, 0, 0);
    int_dark_blue  = SDL_MapRGB(screen->format, 0, 0, 128);
    int_purple  = SDL_MapRGB(screen->format, 128, 64, 128);
    int_dark_purple  = SDL_MapRGB(screen->format, 64, 32, 64);
}

/***************************************************************************
 *      draw_string()
 * Draws the given string at the given location using the default font.
 *********************************************************************PROTO*/
#define	DRAW_CENTER	(1<<0)
#define	DRAW_ABOVE	(1<<1)
#define DRAW_LEFT	(1<<2)
#define DRAW_UPDATE	(1<<3)
#define DRAW_CLEAR	(1<<4)
#define DRAW_HUGE	(1<<5)
#define DRAW_LARGE	(1<<6)
#define DRAW_GRID_0	(1<<7)
int
draw_string(char *text, SDL_Color sc, int x, int y, int flags)
{
    SDL_Surface * text_surface;
    SDL_Rect r;

    if (flags & DRAW_GRID_0) {
	r.x = layout.grid[0].x + layout.grid[0].w / 2;
	r.y = layout.grid[0].y + layout.grid[0].h / 2;
    } else {
	r.x = x;
	r.y = y;
    }

    if (flags & DRAW_HUGE) {
	text_surface = TTF_RenderText_Blended(hfont, text, sc); Assert(text_surface);
    } else if (flags & DRAW_LARGE) {
	text_surface = TTF_RenderText_Blended(lfont, text, sc); Assert(text_surface);
    } else {
	text_surface = TTF_RenderText_Blended(font, text, sc); Assert(text_surface);
    }

    r.w = text_surface->w;
    r.h = text_surface->h;

    if (flags & DRAW_CENTER)
	r.x -= (r.w / 2);
    if (flags & DRAW_LEFT)
	r.x -= text_surface->w;
    if (flags & DRAW_ABOVE)
	r.y -= (r.h);
    if (flags & DRAW_CLEAR) {
	SDL_FillRect(widget_layer, &r, int_black);
	SDL_FillRect(screen, &r, int_black);
    }

    SDL_BlitSafe(text_surface, NULL, widget_layer, &r);
    SDL_BlitSafe(flame_layer, &r, screen, &r);
    SDL_BlitSafe(widget_layer, &r, screen, &r);
    if (flags & DRAW_UPDATE)
	SDL_UpdateSafe(screen, 1, &r);

    SDL_FreeSurface(text_surface);
    return r.h;
}

/***************************************************************************
 *      give_notice()
 * Draws a little "press any key to continue"-type notice between levels.
 * "s" is any message you would like to display.
 * Returns 1 if the user presses 'q', 0 if the users presses 'c'.
 *********************************************************************PROTO*/
int
give_notice(char *s, int quit_possible)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	/* pull out all leading 'Q's */
    }

    if (s && s[0]) 	/* throw that 'ol user text up there */
	draw_string(s, color_blue, 
		layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y
		+ 64, DRAW_CENTER | DRAW_UPDATE);

    if (quit_possible) 
	draw_string("Press 'Q' to Quit", color_red,
		layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y + 128, 
		DRAW_CENTER | DRAW_UPDATE);

    draw_string("Press 'G' to Go On", color_red,
	    layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y +
	    128 + 30, DRAW_CENTER | DRAW_UPDATE);
    while (1) {
	poll_and_flame(&event);
	if (event.type == SDL_KEYDOWN &&
		event.key.keysym.sym == 'q') 
	    return 1;
	if (event.type == SDL_KEYDOWN &&
		event.key.keysym.sym == 'g')
	    return 0;
    } 
}

/***************************************************************************
 *      load_adjust_symbol()
 * Loads the level adjustment images.
 ***************************************************************************/
static void
load_adjust_symbols()
{
    SDL_Surface *bitmap = SDL_LoadBMP("graphics/Level-Up.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Up.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[0] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[0]) 
	PANIC("Can not convert [graphics/Level-Up.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[0], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    bitmap = SDL_LoadBMP("graphics/Level-Medium.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Medium.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[1] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[1]) 
	PANIC("Can not convert [graphics/Level-Medium.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[1], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    bitmap = SDL_LoadBMP("graphics/Level-Down.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Down.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[2] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[2]) 
	PANIC("Can not convert [graphics/Level-Down.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[2], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    Assert(adjust_symbol[0]->h == adjust_symbol[1]->h);
    Assert(adjust_symbol[1]->h == adjust_symbol[2]->h);
}

/***************************************************************************
 *      draw_bordered_rect()
 * Draws a bordered rectangle. Sets "border" based on "orig". 
 *********************************************************************PROTO*/
void
draw_bordered_rect(SDL_Rect *orig, SDL_Rect *border, int thick)
{
    Assert(thick > 0 && thick < 8);

    border->x = orig->x - thick;
    border->y = orig->y - thick;
    border->w = orig->w + thick * 2;
    border->h = orig->h + thick * 2;

    SDL_FillRect(widget_layer, border, int_border_color);
    SDL_FillRect(widget_layer, orig, int_solid_black);

    /* unchanged: SDL_BlitSafe(flame_layer, NULL, screen, &r); */
    SDL_BlitSafe(widget_layer, border, screen, border);
    SDL_UpdateSafe(screen, 1, border);

    return;
}

/***************************************************************************
 *      draw_pre_bordered_rect()
 * Draws a bordered rectangle. 
 *********************************************************************PROTO*/
void
draw_pre_bordered_rect(SDL_Rect *border, int thick)
{
    SDL_Rect orig;
    Assert(thick > 0 && thick < 8);

    orig.x = border->x + thick;
    orig.y = border->y + thick;
    orig.w = border->w - thick * 2;
    orig.h = border->h - thick * 2;

    SDL_FillRect(widget_layer, border, int_border_color);
    SDL_FillRect(widget_layer, &orig, int_solid_black);

    /* unchanged: SDL_BlitSafe(flame_layer, NULL, screen, &r); */
    SDL_BlitSafe(widget_layer, border, screen, border);
    SDL_UpdateSafe(screen, 1, border);
    return;
}

/***************************************************************************
 *      setup_layers()
 * Set up the widget and flame layers.
 *********************************************************************PROTO*/
void
setup_layers(SDL_Surface * screen)
{
    SDL_Rect all;
    flame_layer = SDL_CreateRGBSurface
	(SDL_HWSURFACE|SDL_SRCCOLORKEY,
	 screen->w, screen->h, 8, 0, 0, 0, 0);
    widget_layer = SDL_CreateRGBSurface
	(SDL_HWSURFACE|SDL_SRCCOLORKEY,
	 screen->w, screen->h, screen->format->BitsPerPixel, 
	 screen->format->Rmask, screen->format->Gmask, 
	 screen->format->Bmask, screen->format->Amask);
    if (SDL_SetColorKey(widget_layer, SDL_SRCCOLORKEY, 
		SDL_MapRGB(widget_layer->format, 0, 0, 0)))
	PANIC("SDL_SetColorKey failed on the widget layer.");

    if (int_black != SDL_MapRGB(widget_layer->format, 0, 0, 0)) {
	Debug("*** Warning: screen and widget layer have different RGB format.\n");
    }
    if (int_black != SDL_MapRGB(flame_layer->format, 0, 0, 0)) {
	Debug("*** Warning: screen and flame layer have different RGB format.\n");
    }

    all.x = all.y = 0;
    all.w = screen->w;
    all.h = screen->h;

    SDL_FillRect(widget_layer, &all, int_black);
    SDL_FillRect(flame_layer, &all, int_solid_black);
}

/***************************************************************************
 *      draw_background()
 * Draws the Alizarin Tetris background. Not yet complete, but it's getting
 * better. :-)
 *********************************************************************PROTO*/
void
draw_background(SDL_Surface *screen, int blockWidth, Grid g[],
	int level[], int my_adj[], int their_adj[], char *name[])
{
    char buf[1024];
    int i;
#define IS_DOUBLE(x) ((x==NETWORK)||(x==SINGLE_VS_AI)||(x==TWO_PLAYERS)||(x==AI_VS_AI))

    if (IS_DOUBLE(gametype)) {
	Assert(g[0].w == g[1].w);
	Assert(g[0].h == g[1].h);
    }

    /*
     * clear away the old stuff
     */
    memset(&layout, 0, sizeof(layout));
    
    if (!adjust_symbol[0]) { /* only load these guys the first time */
	load_adjust_symbols();
    }
    /*
     * 	THE BOARD
     */
    if (IS_DOUBLE(gametype)) {
	layout.grid[0].x = ((screen->w / 2) - ((g[0].w*blockWidth))) - 5 * blockWidth - 2;
	layout.grid[0].y = ((screen->h - (g[0].h*blockWidth)) / 2);
	layout.grid[0].w = (g[0].w*blockWidth);
	layout.grid[0].h = (g[0].h*blockWidth);

	layout.grid[1].y = ((screen->h - (g[0].h*blockWidth)) / 2);
	layout.grid[1].w = (g[0].w*blockWidth);
	layout.grid[1].h = (g[0].h*blockWidth);
	layout.grid[1].x = ((screen->w / 2) - 4) + 5 * blockWidth + 6;

	/* Draw the opponent's board */
	draw_bordered_rect(&layout.grid[1], &layout.grid_border[1], 2);
	g[1].board = layout.grid[1];
    }  else {
	layout.grid[0].x = (screen->w - (g[0].w*blockWidth))/2 ;
	layout.grid[0].y = (screen->h - (g[0].h*blockWidth))/2 ;
	layout.grid[0].w = (g[0].w*blockWidth) ;
	layout.grid[0].h = (g[0].h*blockWidth) ;
	/* Don't need a Board[1] */
    }
    /* draw the leftmost board */
    draw_bordered_rect(&layout.grid[0], &layout.grid_border[0], 2);
    g[0].board = layout.grid[0];

    /*
     * 	SCORING, Names
     */
    for (i=0; i< 1+IS_DOUBLE(gametype); i++) {
	layout.name[i].x = layout.grid[i].x;
	layout.name[i].y = layout.grid[i].y + layout.grid[i].h + 2;
	layout.name[i].w = layout.grid[i].w;
	layout.name[i].h = screen->h - layout.name[i].y;

	if (gametype == DEMO) {
	    char buf[1024];
	    SDL_FillRect(widget_layer, &layout.name[i], int_black);
	    SDL_FillRect(screen, &layout.name[i], int_black);
	    sprintf(buf,"Demo (%s)",name[i]);
	    draw_string(buf, color_blue, layout.grid_border[i].x +
		    layout.grid_border[i].w/2, layout.grid_border[i].y +
		    layout.grid_border[i].h, DRAW_CENTER | DRAW_CLEAR | DRAW_UPDATE);
	} else {
	    draw_string(name[i], color_blue,
		    layout.grid_border[i].x + layout.grid_border[i].w/2,
		    layout.grid_border[i].y + layout.grid_border[i].h,
		    DRAW_CENTER);
	}
	/* Set up the coordinates for future score writing */
	layout.score[i].x = layout.grid_border[i].x;
	layout.score[i].w = layout.grid_border[i].w;
	layout.score[i].y = 0;
	layout.score[i].h = layout.grid_border[i].y;
	SDL_FillRect(widget_layer, &layout.score[i], int_black);
	SDL_FillRect(screen, &layout.score[i], int_black);

	sprintf(buf,"Level %d, Score:",level[i]);
	draw_string(buf, color_blue, layout.grid_border[i].x,
		layout.grid_border[i].y, DRAW_ABOVE | DRAW_CLEAR);

	layout.score[i].x = layout.grid_border[i].x + layout.grid_border[i].w;
	layout.score[i].y = layout.grid_border[i].y;
    }

    /*
     * 	TIME LEFT
     */

#define TIME_WIDTH 	(16*5)
#define TIME_HEIGHT 	28

    if (gametype == DEMO) {
	/* do nothing */
    } else if (IS_DOUBLE(gametype)) {
	draw_string("Time Left", color_blue, 
		screen->w/2, layout.score[0].y, DRAW_CENTER | DRAW_ABOVE);
	layout.time.x = (screen->w - TIME_WIDTH)/2;
	layout.time.y = layout.score[0].y;
	layout.time.w = TIME_WIDTH;
	layout.time.h = TIME_HEIGHT;
	draw_bordered_rect(&layout.time, &layout.time_border, 2);
    } else { /* single */
	int text_h = draw_string("Time Left", color_blue,
		screen->w/10, screen->h/5, 0);
	layout.time.x = screen->w / 10;
	layout.time.y = screen->h / 5 + text_h;
	layout.time.w = TIME_WIDTH;
	layout.time.h = TIME_HEIGHT;

	draw_bordered_rect(&layout.time, &layout.time_border, 2);
    }

    /*
     *	LEVEL ADJUSTMENT
     */
    if (gametype == DEMO) {

    } else if (gametype == AI_VS_AI) {
	for (i=0;i<3;i++) {
	    char buf[80];

	    layout.adjust[0].symbol[i].x = (screen->w - adjust_symbol[i]->w)/2;
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;

	    SDL_BlitSafe(adjust_symbol[i], NULL, widget_layer, 
		    &layout.adjust[0].symbol[i]);

	    /* draw the textual tallies */
	    sprintf(buf,"%d",my_adj[i]);
	    draw_string(buf, color_red,
		    layout.adjust[0].symbol[i].x - 10, 
		    layout.adjust[0].symbol[i].y, DRAW_LEFT | DRAW_CLEAR);
	    sprintf(buf,"%d",their_adj[i]);
	    draw_string(buf, color_red,
		    layout.adjust[0].symbol[i].x + layout.adjust[0].symbol[i].w + 10, 
		    layout.adjust[0].symbol[i].y, DRAW_CLEAR);
	}
    } else if (IS_DOUBLE(gametype)) {
	for (i=0;i<3;i++) {
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].x = (screen->w - 3*adjust_symbol[i]->w)/2;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[0].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[0].symbol[i], int_black);
	    if (my_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[my_adj[i]], NULL, widget_layer, 
			&layout.adjust[0].symbol[i]);
	    layout.adjust[1].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[1].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[1].symbol[i].x = (screen->w)/2 + adjust_symbol[i]->w/2;
	    layout.adjust[1].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[1].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[1].symbol[i], int_black);
	    if (their_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[their_adj[i]], NULL, widget_layer, 
			&layout.adjust[1].symbol[i]);
	}
    } else { /* single player */
	for (i=0;i<3;i++) {
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].x = 
		layout.grid_border[0].x + layout.grid_border[0].w +
		2 * adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y +layout.time_border.h+ i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[0].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[0].symbol[i], int_black);
	    if (my_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[my_adj[i]], NULL, widget_layer, 
			&layout.adjust[0].symbol[i]);
	}
    }

    /*
     * NEXT PIECE
     */
    if (gametype == DEMO) {
	/* do nothing */
    } else if (IS_DOUBLE(gametype)) {
	int text_h = draw_string("Next Piece", color_blue,
		screen->w / 2, 
		layout.adjust[0].symbol[2].y +
		layout.adjust[0].symbol[2].h, DRAW_CENTER);

	layout.next_piece[0].w = 5 * blockWidth;
	layout.next_piece[0].h = 5 * blockWidth;
	layout.next_piece[0].x = (screen->w / 2) - (5 * blockWidth);
	layout.next_piece[0].y = layout.adjust[0].symbol[2].y +
	    layout.adjust[0].symbol[2].h + text_h;
	draw_bordered_rect(&layout.next_piece[0], &layout.next_piece_border[0], 2);

	layout.next_piece[1].w = layout.next_piece[0].w;
	layout.next_piece[1].h = layout.next_piece[0].h;
	layout.next_piece[1].y = layout.next_piece[0].y;
	layout.next_piece[1].x = (screen->w / 2);
	draw_bordered_rect(&layout.next_piece[1], &layout.next_piece_border[1], 2);
    } else {
	int text_h = draw_string("Next Piece", color_blue,
		screen->w/10, 2*screen->h/5, 0);

	layout.next_piece[0].w = 5 * blockWidth;
	layout.next_piece[0].h = 5 * blockWidth;
	layout.next_piece[0].x = screen->w/10;
	layout.next_piece[0].y = (2*screen->h/5) + text_h;

	/* Draw the box for the next piece to fit in */
	draw_bordered_rect(&layout.next_piece[0], &layout.next_piece_border[0], 2);
    }

    /*
     *	PAUSE BOX
     */
    if (gametype == DEMO) {

    } else if (IS_DOUBLE(gametype)) {
	layout.pause.x = layout.next_piece_border[0].x;
	layout.pause.y = layout.next_piece_border[0].h + layout.next_piece_border[0].y + 2 + 
	    layout.next_piece_border[0].h / 3;
	layout.pause.w = layout.next_piece_border[0].w + layout.next_piece[1].w;
	layout.pause.h = layout.next_piece_border[0].h / 3;
    } else {
	layout.pause.x = layout.next_piece_border[0].x;
	layout.pause.y = layout.next_piece_border[0].h + layout.next_piece_border[0].y + 2 +
	    layout.next_piece_border[0].h / 3;
	layout.pause.w = layout.next_piece_border[0].w;
	layout.pause.h = layout.next_piece_border[0].h / 3;
    }

    /* Blit onto the screen surface */
    {
	SDL_Rect dest;
	dest.x = 0; dest.y = 0; dest.w = screen->w; dest.h = screen->h;

	SDL_BlitSafe(flame_layer, NULL, screen, NULL);
	SDL_BlitSafe(widget_layer, NULL, screen, NULL);
	SDL_UpdateSafe(screen, 1, &dest);
    }
    return;
}

/***************************************************************************
 *      draw_pause()
 * Draw or clear the pause indicator.
 *********************************************************************PROTO*/
void
draw_pause(int on)
{
    int i;
    if (on) {
	draw_pre_bordered_rect(&layout.pause, 2);
	draw_string("* Paused *", color_blue, 
		layout.pause.x + layout.pause.w / 2, layout.pause.y,
		DRAW_CENTER | DRAW_UPDATE);
	for (i=0; i<2; i++) {
	    /* save this stuff so that the flame doesn't go over it .. */
	    if (layout.grid[i].w) {
		SDL_BlitSafe(screen, &layout.grid[i], widget_layer,
			&layout.grid[i]);
		SDL_BlitSafe(screen, &layout.next_piece[i], widget_layer,
			&layout.next_piece[i]);
	    }
	}
    } else {
	SDL_FillRect(widget_layer, &layout.pause, int_black);
	SDL_FillRect(screen, &layout.pause, int_black);
	SDL_BlitSafe(flame_layer, &layout.pause, screen, &layout.pause);
	/* no need to paste over it with the widget layer: we know it to
	 * be all transparent */
	SDL_UpdateSafe(screen, 1, &layout.pause);

	for (i=0; i<2; i++) 
	    if (layout.grid[i].w) {
		SDL_BlitSafe(widget_layer, &layout.grid[i], screen,
			&layout.grid[i]);
		SDL_FillRect(widget_layer, &layout.grid[i], int_solid_black);

		SDL_BlitSafe(widget_layer, &layout.next_piece[i], screen,
			&layout.next_piece[i]);
		SDL_FillRect(widget_layer, &layout.next_piece[i], int_solid_black);
	    }
    }
}

/***************************************************************************
 *      draw_clock()
 * Draws a five-digit (-x:yy) clock in the center of the screen.
 *********************************************************************PROTO*/
void
draw_clock(int seconds)
{
    static int old_seconds = -111;	/* last time we drew */
    static SDL_Surface * digit[12];
    char buf[16];
    static int w = -1, h= -1; /* max digit width/height */
    int i, c;

    if (seconds == old_seconds || gametype == DEMO) return;

    if (old_seconds == -111) {
	/* setup code */

	for (i=0;i<10;i++) {
	    sprintf(buf,"%d",i);
	    digit[i] = TTF_RenderText_Blended(font,buf,color_blue);
	}
	sprintf(buf,":"); digit[10] = TTF_RenderText_Blended(font,buf,color_red);
	sprintf(buf,"-"); digit[11] = TTF_RenderText_Blended(font,buf,color_red);

	for (i=0;i<12;i++) {
	    Assert(digit[i]);
	    /* the colorkey and display format are already done */
	    /* find the largest dimensions */
	    if (digit[i]->w > w) w = digit[i]->w;
	    if (digit[i]->h > h) h = digit[i]->h;
	}
    }

    old_seconds = seconds;

    sprintf(buf,"%d:%02d",seconds / 60, seconds % 60);

    c = layout.time.x;
    layout.time.w = w * 5;
    layout.time.h = h;

    SDL_FillRect(widget_layer, &layout.time, int_solid_black); 

    if (strlen(buf) > 5)
	sprintf(buf,"----");

    if (strlen(buf) < 5)
	layout.time.x += ((5 - strlen(buf)) * w) / 2;


    for (i=0;buf[i];i++) {
	SDL_Surface * to_blit;

	if (buf[i] >= '0' && buf[i] <= '9')
	    to_blit = digit[buf[i] - '0'];
	else if (buf[i] == ':')
	    to_blit = digit[10];
	else if (buf[i] == '-')
	    to_blit = digit[11];
	else PANIC("unknown character in clock string [%s]",buf);

	/* center the letter horizontally */
	if (w > to_blit->w) layout.time.x += (w - to_blit->w) / 2;
	layout.time.w = to_blit->w;
	layout.time.h = to_blit->h;
	/*
	Debug("[%d+%d, %d+%d]\n",
		clockPos.x,clockPos.w,clockPos.y,clockPos.h);
		*/
	SDL_BlitSafe(to_blit, NULL, widget_layer, &layout.time);
	if (w > to_blit->w) layout.time.x -= (w - to_blit->w) / 2;
	layout.time.x += w;
    }

    layout.time.x = c;
    /*    clockPos.x = (screen->w - (w * 5)) / 2;*/
    layout.time.w = w * 5;
    layout.time.h = h;
    SDL_BlitSafe(flame_layer, &layout.time, screen, &layout.time);
    SDL_BlitSafe(widget_layer, &layout.time, screen, &layout.time);
    SDL_UpdateSafe(screen, 1, &layout.time);

    return;
}

/***************************************************************************
 *      draw_score()
 *********************************************************************PROTO*/
void
draw_score(SDL_Surface *screen, int i)
{
    char buf[256];

    sprintf(buf, "%d", Score[i]);
    draw_string(buf, color_red, 
	    layout.score[i].x, layout.score[i].y, DRAW_LEFT | DRAW_CLEAR |
	    DRAW_ABOVE | DRAW_UPDATE);
}

/***************************************************************************
 *      draw_next_piece()
 * Draws the next piece on the screen.
 *
 * Needs the color style because draw_play_piece() needs it to paste the
 * bitmaps. 
 *********************************************************************PROTO*/
void
draw_next_piece(SDL_Surface *screen, piece_style *ps, color_style *cs,
	play_piece *cp, play_piece *np, int P)
{
    if (gametype != DEMO) {
	/* fake-o centering */
	int cp_right = 5 - cp->base->dim;
	int cp_down = 5 - cp->base->dim;
	int np_right = 5 - np->base->dim;
	int np_down = 5 - np->base->dim;

	draw_play_piece(screen, cs,
		cp, 
		layout.next_piece[P].x + cp_right * cs->w / 2,
		layout.next_piece[P].y + cp_down * cs->w / 2, 0,
		np, 
		layout.next_piece[P].x + np_right * cs->w / 2,
		layout.next_piece[P].y + np_down * cs->w / 2, 0);
    }
    return;
}

/*
 * $Log: display.c,v $
 * Revision 1.25  2000/11/01 04:39:41  weimer
 * clear the little scoring spot correctly, updates for making a no-sound
 * distribution
 *
 * Revision 1.24  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.23  2000/10/30 16:25:25  weimer
 * display the network player score during network games. Also give the
 * non-server a little message when waiting for the server to go on.
 *
 * Revision 1.22  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.21  2000/10/29 21:28:58  weimer
 * fixed a few failures to clear the screen if we didn't have a flaming
 * backdrop
 *
 * Revision 1.20  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.19  2000/10/29 19:04:32  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.18  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.17  2000/10/28 13:39:14  weimer
 * added a pausing feature ...
 *
 * Revision 1.16  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.15  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.14  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.13  2000/10/14 02:52:44  weimer
 * fixed a memory corruption problem in display (a use after a free)
 *
 * Revision 1.12  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.11  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.10  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.9  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.8  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.7  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.6  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.5  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.4  2000/09/03 21:06:31  wkiri
 * Now handles three different game types (and does different things).
 * Major changes in display.c to handle this.
 *
 * Revision 1.3  2000/09/03 18:40:23  wkiri
 * Cleaned up atris.c some more (high_score_check()).
 * Added variable for game type (defaults to MARATHON).
 *
 */

