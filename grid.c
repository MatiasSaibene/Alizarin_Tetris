/*
 *                               Alizarin Tetris
 * Functions relating to the game board.
 * 
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */

#include "atris.h"
#include "display.h"
#include "grid.h"
#include "options.h"
#include "piece.h"

/***************************************************************************
 *      cleanup_grid()
 * Removes all of the REMOVE_ME's in a grid. Normally one uses draw_grid
 * for this purpose, but you might have a personal testing grid or
 * something that you're not displaying.
 *********************************************************************PROTO*/
void
cleanup_grid(Grid *g)
{
    int x,y;
    for (y=g->h-1;y>=0;y--) 
	for (x=g->w-1;x>=0;x--) 
	    if (GRID_CONTENT(*g,x,y) == REMOVE_ME)
		GRID_SET(*g,x,y,0);
}

/***************************************************************************
 *      generate_board()
 * Creates a new board at the given level.
 *********************************************************************PROTO*/
Grid
generate_board(int w, int h, int level)
{
    int i,j,r;

    Grid retval;

    retval.w = w;
    retval.h = h;

    Calloc(retval.contents,unsigned char *,(w*h*sizeof(*retval.contents)));
    Calloc(retval.fall,unsigned char *,(w*h*sizeof(*retval.fall)));
    Calloc(retval.changed,unsigned char *,(w*h*sizeof(*retval.changed)));
    Calloc(retval.temp,unsigned char *,(w*h*sizeof(*retval.temp)));

    if (level) {
	int start_garbage;
	level = GARBAGE_LEVEL(level);
	start_garbage = (h-level) - 2;

	if (start_garbage < 2) start_garbage = 2;
	else if (start_garbage >= h) start_garbage = h - 1;

	for (j=start_garbage;j<h;j++)
	    for (r=0;r<w/2;r++) {
		do {
		    i = ZEROTO(w);
		} while (GRID_CONTENT(retval,i,j) == 1);
		GRID_SET(retval,i,j,1);
	    }
    }
    return retval;
}

/***************************************************************************
 *      add_garbage()
 * Adds garbage to the given board. Pushes all of the lines up, adds the
 * garbage to the bottom.
 *********************************************************************PROTO*/
void
add_garbage(Grid *g)
{
    int i,j;
    for (j=0;j<g->h-1;j++)
	for (i=0;i<g->w;i++) {
	    GRID_SET(*g,i,j, GRID_CONTENT(*g,i,j+1));
	    FALL_SET(*g,i,j, NOT_FALLING);
	    if (GRID_CONTENT(*g,i,j) == 0)
		GRID_SET(*g,i,j,REMOVE_ME);
	    GRID_CHANGED(*g,i,j) = 1;
	}

    j = g->h - 1;
    for (i=0; i<g->w; i++) {
	    if (ZEROTO(100) < 50) {
		GRID_SET(*g,i,j,1);
		if (GRID_CONTENT(*g,i,j-1) &&
			GRID_CONTENT(*g,i,j-1) != REMOVE_ME)
		    GRID_SET(*g,i,j-1,1);
	    } else {
		GRID_SET(*g,i,j,REMOVE_ME);
	    }
	    FALL_SET(*g,i,j, NOT_FALLING);
	    GRID_CHANGED(*g,i,j) = 1;
    }

    for (j=0;j<g->h;j++)
	for (i=0;i<g->w;i++) {
	    Assert(GRID_CHANGED(*g,i,j));
	    Assert(FALL_CONTENT(*g,i,j) == NOT_FALLING);
	}

    return;
}

/***************************************************************************
 *      draw_grid()
 * Draws the main grid board. This involves drawing all of the pieces (and
 * garbage) currently pasted on to it. This is the function that actually
 * clears out grid pieces marked with "REMOVE_ME". The main bit of work
 * here is calculating the shadows. 
 *
 * Uses the color style to actually draw the right picture on the screen.
 *********************************************************************PROTO*/
void
draw_grid(SDL_Surface *screen, color_style *cs, Grid *g, int draw)
{
    SDL_Rect r,s;
    int i,j;
    int mini=20, minj=20, maxi=-1, maxj=-1;
    for (j=g->h-1;j>=0;j--) {
	for (i=g->w-1;i>=0;i--) {
	    int c = GRID_CONTENT(*g,i,j);
	    if (draw && c && c != REMOVE_ME && GRID_CHANGED(*g,i,j)) {
		if (i < mini) mini = i; if (j < minj) minj = j;
		if (i > maxi) maxi = i; if (j > maxj) maxj = j;
		

		s.x = g->board.x + (i * cs->w);
		s.y = g->board.y + (j * cs->h);
		s.w = cs->w;
		s.h = cs->h;

		SDL_BlitSafe(cs->color[c], NULL, screen, &s);

		{
		    int fall = FALL_CONTENT(*g,i,j);

		    int that_precolor = (j == 0) ? 0 : 
			GRID_CONTENT(*g,i,j-1);
		    int that_fall = (j == 0) ? -1 :
			FALL_CONTENT(*g,i,j-1);
		    /* light up */
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = g->board.y + j * cs->h;
			r.h = edge[HORIZ_LIGHT]->h;
			r.w = edge[HORIZ_LIGHT]->w;
			SDL_BlitSafe(edge[HORIZ_LIGHT],NULL,
				    screen, &r);
		    }

		    /* light left */
		    that_precolor = (i == 0) ? 0 :
			GRID_CONTENT(*g,i-1,j);
		    that_fall = (i == 0) ? -1 :
			FALL_CONTENT(*g,i-1,j);
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = g->board.y + j * cs->h;
			r.h = edge[VERT_LIGHT]->h;
			r.w = edge[VERT_LIGHT]->w;
			SDL_BlitSafe(edge[VERT_LIGHT],NULL,
				    screen,&r);
		    }
		    
		    /* shadow down */
		    that_precolor = (j == g->h-1) ? 0 :
			GRID_CONTENT(*g,i,j+1);
		    that_fall = (j == g->h-1) ? -1 :
			FALL_CONTENT(*g,i,j+1);
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = (g->board.y + (j+1) * cs->h) 
			    - edge[HORIZ_DARK]->h;
			r.h = edge[HORIZ_DARK]->h;
			r.w = edge[HORIZ_DARK]->w;
			SDL_BlitSafe(edge[HORIZ_DARK],NULL,
				    screen,&r);
		    }

		    /* shadow right */
		    that_precolor = (i == g->w-1) ? 0 :
			GRID_CONTENT(*g,i+1,j);
		    that_fall = (i == g->w-1) ? -1 :
			FALL_CONTENT(*g,i+1,j);
		    if (that_precolor != c || that_fall != fall) {
			r.x = (g->board.x + (i+1) * cs->w) 
			    - edge[VERT_DARK]->w;
			r.y = (g->board.y + (j) * cs->h);
			r.h = edge[VERT_DARK]->h;
			r.w = edge[VERT_DARK]->w;
			SDL_BlitSafe(edge[VERT_DARK],NULL,
				    screen,&r);
		    }
		} /* endof: hikari to kage */
		/* SDL_UpdateSafe(screen, 1, &s); */
		GRID_CHANGED(*g,i,j) = 0;
	    } else if (c == REMOVE_ME) {
		if (i < mini) mini = i; if (j < minj) minj = j;
		if (i > maxi) maxi = i; if (j > maxj) maxj = j;
		if (draw) { 
		    s.x = g->board.x + (i * cs->w);
		    s.y = g->board.y + (j * cs->h);
		    s.w = cs->w;
		    s.h = cs->h;
		    SDL_FillRect(screen, &s, int_solid_black);
		    GRID_SET(*g,i,j,0);
		    GRID_CHANGED(*g,i,j) = 0;
		} else {
		    GRID_SET(*g,i,j,0);
		}
	    }
	}
    }
    s.x = g->board.x + mini * cs->w;
    s.y = g->board.y + minj * cs->h;
    s.w = (maxi - mini + 1) * cs->w;
    s.h = (maxj - minj + 1) * cs->h;
    if (draw && maxi >  -1) SDL_UpdateSafe(screen,1,&s);
    return;
}

/***************************************************************************
 *      draw_falling()
 * Draws the falling pieces on the main grid. Offset should range from 1 to
 * the size of the color tiles -- the falling pieces are drawn that far
 * down out of their "real" places. This gives a smooth animation effect.
 *********************************************************************PROTO*/
void
draw_falling(SDL_Surface *screen, int blockWidth, Grid *g, int offset)
{
    SDL_Rect s;
    SDL_Rect r;  
    int i,j;
    int mini=100000, minj=100000, maxi=-1, maxj=-1;

    int cj;	/* cluster right, cluster bottom */

    memset(g->temp, 0, sizeof(*g->temp)*g->w*g->h);

    for (j=0;j<g->h;j++) {
	for (i=0;i<g->w;i++) {
	    int c = GRID_CONTENT(*g,i,j);
	    if (c && FALL_CONTENT(*g,i,j) == FALLING &&
		    TEMP_CONTENT(*g,i,j) == 0) {

		for (cj = j; GRID_CONTENT(*g,i,cj) &&
			FALL_CONTENT(*g,i,cj) &&
			TEMP_CONTENT(*g,i,cj) == 0 &&
			cj < g->h; cj++)
		    TEMP_CONTENT(*g,i,cj) = 1;

		/* source == up */
		s.x = g->board.x + (i * blockWidth);
		s.y = g->board.y + (j * blockWidth) + offset - 1;
		s.w = blockWidth;
		s.h = blockWidth * (cj - j + 1); 

		/* dest == down */
		r.x = g->board.x + (i * blockWidth);
		r.y = g->board.y + (j * blockWidth) + offset;
		r.w = blockWidth;
		r.h = blockWidth * (cj - j + 1); 

		/* just blit the screen down a notch */
		SDL_BlitSafe(screen, &s, screen, &r);

		if (s.x < mini) mini = s.x; 
		if (s.y < minj) minj = s.y;
		if (s.x+s.w > maxi) maxi = s.x+s.w; 
		if (s.y+s.h > maxj) maxj = s.y+s.h;

		/* clear! */
		if (j == 0 || FALL_CONTENT(*g,i,j-1) == NOT_FALLING ||
			GRID_CONTENT(*g,i,j-1) == 0) {
		    s.h = 1;
		    SDL_BlitSafe(widget_layer, &s, screen, &s);
		    /*  SDL_UpdateSafe(screen, 1, &s); */
		}
	    }
	}
    }

    s.x = mini;
    s.y = minj;
    s.w = maxi - mini + 1;
    s.h = maxj - minj + 1;
    if (maxi >  -1) SDL_UpdateSafe(screen,1,&s);
    return;
}

/***************************************************************************
 *      fall_down()
 * Move all of the fallen pieces down a notch.
 *********************************************************************PROTO*/
void
fall_down(Grid *g)
{
    int x,y;
    for (y=g->h-1;y>=1;y--) {
	for (x=0;x<g->w;x++) {
	    if (FALL_CONTENT(*g,x,y-1) == FALLING &&
		     GRID_CONTENT(*g,x,y-1)) {
		Assert(GRID_CONTENT(*g,x,y) == 0 || 
		       GRID_CONTENT(*g,x,y) == REMOVE_ME);
		GRID_SET(*g,x,y, GRID_CONTENT(*g,x,y-1));
		GRID_SET(*g,x,y-1,REMOVE_ME);
		FALL_SET(*g,x,y, FALLING);
		/* FALL_SET(*g,x,y-1,UNKNOWN); */
	    }
	}
    }
    return;
}

/***************************************************************************
 *      determine_falling()
 * Determines if anything can fall.
 *********************************************************************PROTO*/
int
determine_falling(Grid *g)
{
    int x,y;
#ifdef DEBUG
    {
	int retval=0;
    for (y=0;y<g->h;y++) {
	for (x=0;x<g->w;x++) {
	    if (GRID_CONTENT(*g,x,y) && FALL_CONTENT(*g,x,y) == FALLING) {
		printf("X");
		retval= 1;
	    } else printf(".");
	}
	printf("\n");
    }
    printf("--\n");
    return retval;
    }
#else
    for (y=0;y<g->h;y++) 
	for (x=0;x<g->w;x++) 
	    if (GRID_CONTENT(*g,x,y) && FALL_CONTENT(*g,x,y) == FALLING) 
		return 1;
    return 0;
#endif
}

/***************************************************************************
 *      run_gravity()
 * Applies gravity: pieces with no support change to "FALLING". After this,
 * "determine_falling" can be used to determine if anything should be
 * falling.
 *
 * Returns 1 if any pieces that were FALLING settled down.
 *
 * This is the Mark II iterative version. Heavy-sigh!
 *********************************************************************PROTO*/
int
run_gravity(Grid *g)
{
    int falling_pieces_settled;
    int x,y,c,f, X,Y, YY;
    int S; 
    int lowest_y = 0;
    int garbage_on_row[21] ;	/* FIXME! */
    /*   6.68      5.20     0.38    16769     0.02     0.02  run_gravity */

    char UP_QUEUE[2000], UP_POS=0;

#define UP_X(P) UP_QUEUE[(P)*3]
#define UP_Y(P) UP_QUEUE[(P)*3 + 1]
#define UP_S(P) UP_QUEUE[(P)*3 + 2]

    falling_pieces_settled = 0;
    for (y=0; y<g->h; y++) {
	garbage_on_row[y] = 0;
	for (x=0;x<g->w;x++) {
	    int c = GRID_CONTENT(*g,x,y);
	    if (!lowest_y && c)
		lowest_y = y;
	    if (c == 1)
		garbage_on_row[y] = 1;
	    if (FALL_CONTENT(*g,x,y) == NOT_FALLING)
		FALL_SET(*g,x,y,UNKNOWN);
	}
    }
    garbage_on_row[20] = 1;

    for (y=g->h-1;y>=lowest_y;y--)
	for (x=0;x<g->w;x++) {
	    c = GRID_CONTENT(*g,x,y);
	    if (!c) continue;
	    else if (c != 1 && y < g->h-1) continue;
	    else if (c == 1 && !garbage_on_row[y+1]) {
		/* if we do not set this, garbage from above us will appear
		 * to be supported and it will reach down and support us!
		 * scary! */
		garbage_on_row[y] = 0;
		continue;
	    }
	    f = FALL_CONTENT(*g,x,y);
	    if (f == NOT_FALLING) continue;
	    else if (f == FALLING) {
		falling_pieces_settled = 1;
	    }

	    /* now, run up as far as we can ... */
	    if (y >= 1) {
		Y = y;
		while ((c = GRID_CONTENT(*g,x,Y)) && 
			FALL_CONTENT(*g,x,Y) != NOT_FALLING) {
		    /* mark stable */
		    f = FALL_CONTENT(*g,x,Y);
		    if (f == FALLING) {
			falling_pieces_settled = 1;
		    }
		    FALL_SET(*g,x,Y,NOT_FALLING);
		    /* promise to get to our same-color buddies later */
		    if (x >= 1 && GRID_CONTENT(*g,x-1,Y) == c &&
			    FALL_CONTENT(*g,x-1,Y) != NOT_FALLING) {
			UP_X(UP_POS) = x-1 ; UP_Y(UP_POS) = Y; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    if (x < g->w-1 && GRID_CONTENT(*g,x+1,Y) == c &&
			    FALL_CONTENT(*g,x+1,Y) != NOT_FALLING) {
			UP_X(UP_POS) = x+1 ; UP_Y(UP_POS) = Y; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    if (Y < g->h-1 && GRID_CONTENT(*g,x,Y+1) == c &&
			    FALL_CONTENT(*g,x,Y+1) != NOT_FALLING) {
			UP_X(UP_POS) = x ; UP_Y(UP_POS) = Y+1; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    Y--;
		}
	    } else {
		/* Debug("2 (%2d,%2d)\n",x,y); */
		FALL_SET(*g,x,y,NOT_FALLING);
	    }
	    
	    /* now burn down the queue */
	    while (UP_POS > 0) {
		UP_POS--;
		X = UP_X(UP_POS);
		Y = UP_Y(UP_POS);
		S = UP_S(UP_POS);
		c = GRID_CONTENT(*g,X,Y);
		Assert(c);
		f = FALL_CONTENT(*g,X,Y);
		if (f == NOT_FALLING) continue;
		else if (f == FALLING && !S) continue;

		if (Y >= 1) {
		    YY = Y;
		    while ((c = GRID_CONTENT(*g,X,YY)) && 
			    FALL_CONTENT(*g,X,YY) != NOT_FALLING) {
			/* mark stable */
			f = FALL_CONTENT(*g,X,YY);
			if (f == FALLING) falling_pieces_settled = 1;
			FALL_SET(*g,X,YY,NOT_FALLING);
			/* promise to get to our same-color buddies later */
			if (X >= 1 && GRID_CONTENT(*g,X-1,YY) == c &&
				FALL_CONTENT(*g,X-1,YY) != NOT_FALLING) {
			    UP_X(UP_POS) = X-1 ; UP_Y(UP_POS) = YY; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			if (X < g->w-1 && GRID_CONTENT(*g,X+1,YY) == c &&
				FALL_CONTENT(*g,X+1,YY) != NOT_FALLING) {
			    UP_X(UP_POS) = X+1 ; UP_Y(UP_POS) = YY; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			if (YY < g->h-1 && GRID_CONTENT(*g,X,YY+1) == c &&
				FALL_CONTENT(*g,X,YY+1) != NOT_FALLING) {
			    UP_X(UP_POS) = X ; UP_Y(UP_POS) = YY+1; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			YY--;
		    }
		} else {
		    FALL_SET(*g,X,Y,NOT_FALLING);
		}
	    }
	}
	    
    for (y=g->h-1;y>=0;y--) 
	for (x=0;x<g->w;x++) 
	    if (FALL_CONTENT(*g,x,y) == UNKNOWN)
		FALL_CONTENT(*g,x,y) = FALLING;

    return falling_pieces_settled;
}


/***************************************************************************
 *      check_tetris()
 * Checks to see if any rows have been completely filled. Returns the
 * number of such rows. Such rows are cleared.
 *********************************************************************PROTO*/
int
check_tetris(Grid *g)
{
    int tetris_count = 0;
    int x,y;
    for (y=g->h-1;y>=0;y--)  {
	int c = 0;
	for (x=0;x<g->w;x++) 
	    if (GRID_CONTENT(*g,x,y))
		c++;
	if (c == g->w) {
	    tetris_count++;
	    for (x=0;x<g->w;x++)
		GRID_SET(*g,x,y,REMOVE_ME);
	}
    }
    return tetris_count;
}

/*
 * $Log: grid.c,v $
 * Revision 1.24  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.23  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.22  2000/11/03 04:25:58  weimer
 * add some optimizations to run_gravity to make it just a bit faster (down
 * to 0.01 ms/call from 0.02), sleep a bit more in event-loop: generally
 * trying to make us more CPU friendly ...
 *
 * Revision 1.21  2000/11/03 03:41:35  weimer
 * made the light and dark "edges" of pieces global, rather than part of a
 * specific color style. also fixed a bug where we were updating too much
 * when drawing falling pieces (bad min() code on my part)
 *
 * Revision 1.20  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.19  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.18  2000/10/26 22:23:07  weimer
 * double the effective number of levels
 *
 * Revision 1.17  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.16  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.15  2000/10/18 03:29:56  weimer
 * fixed problem with "machine-gun" sounds caused by "run_gravity()" returning
 * 1 too often
 *
 * Revision 1.14  2000/10/14 02:52:44  weimer
 * fixed a memory corruption problem in display (a use after a free)
 *
 * Revision 1.13  2000/10/14 00:31:53  weimer
 * non-recursive run-gravity ...
 *
 * Revision 1.12  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.11  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.10  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.9  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.8  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.7  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.6  2000/09/05 22:10:46  weimer
 * fixed initial garbage generation strangeness
 *
 * Revision 1.5  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.4  2000/09/04 21:02:18  weimer
 * Added some addition piece layout code, garbage now falls unless the row
 * below it features some non-falling garbage
 *
 * Revision 1.3  2000/09/04 15:20:21  weimer
 * faster blitting algorithms for drawing the grid and drawing falling pieces
 * (when you eliminate a line)
 *
 * Revision 1.2  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.1  2000/09/03 17:56:36  wkiri
 * Reorganization of files (display.[ch], event.c are new).
 *
 * Revision 1.9  2000/08/26 02:28:55  weimer
 * now you can see the other player!
 *
 * Revision 1.8  2000/08/26 01:36:25  weimer
 * fixed a long-running update bug: the grid changed field was being reset to
 * 0 when falling states or content were set to the same thing, even if they
 * had actually previously changed ...
 *
 * Revision 1.7  2000/08/26 01:03:11  weimer
 * remove keyboard repeat when you enter high scores
 *
 * Revision 1.6  2000/08/24 02:32:18  weimer
 * gameplay changes (key repeats) and speed modifications (falling pieces,
 * redraws, etc.)
 *
 * Revision 1.5  2000/08/22 15:13:53  weimer
 * added a garbage-generation routine
 *
 * Revision 1.4  2000/08/21 00:04:56  weimer
 * minor text placement changes, falling speed, etc.
 *
 * Revision 1.3  2000/08/15 00:59:41  weimer
 * a few more sound things
 *
 * Revision 1.2  2000/08/13 19:27:20  weimer
 * added file changelogs
 *
 */
