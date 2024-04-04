/*
 *                               Alizarin Tetris
 * The piece definition structures.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */
#ifndef __PIECE_H
#define __PIECE_H

/* this structure describes a piece layout in the abstract: it has four "x
 * by y" bitmaps, one for each rotation. Within the bitmap, 0 means
 * "nothing there" and 1...Z means "tile N is there". All of the bitmaps
 * have the same upper-left corner (conceptually), and to find what is at
 * location (i,j) of rotation r, you would use:
 */
#include <SDL/SDL_video.h>
#define BITMAP(p,r,x,y) ((p).bitmap[(r)])[((p).dim)*(y)+(x)]
typedef struct piece_struct {
    int dim;		/* width/height in color-tile "units" */
    int num_color;	/* number of color-tile "units" here */
    unsigned char *bitmap[4];
} piece;

/* a piece_style contains a number of different pieces (as declared above)
 * and a name. 
 */
typedef struct piece_style_struct {
    char *name;		/* the name of the style */
    int num_piece;	/* number of pieces */
    piece * shape;	/* the piece shape */
} piece_style;

/* "piece_styles" contains all of the styles we have been able to load for
 * this game.
 */
typedef struct piece_styles_struct {
    int num_style;
    int choice;
    piece_style **style;
} piece_styles;

/* is this piece a "special" piece that will have extra powers when it
 * lands? */
typedef enum {
    No_Special			= -1,
    Special_Bomb		= 0,
    Special_Repaint		= 1,
    Special_Pushdown		= 2,
    Special_Colorkill		= 3,
} special_type;

/* a piece that will be presented to the player, instantiated from a piece
 * style */
typedef struct play_piece_struct {
    piece *		base;
    unsigned char 	colormap[12];
    special_type 	special;
} play_piece;

/* a color style holds a number of surfaces that are used to draw in the
 * tiles that make up pieces */
typedef struct color_style_struct {
    char *name;			/* the name of the style */
    int num_color;		/* number of colors defined */
    SDL_Surface **color;	/* surfaces for the colors */
    /* note that the colors go from 1 to "num_color" inclusive! */
    int w;			/* width of each color block */
    int h;			/* height of each color block */
} color_style;

color_style special_style;

#define HORIZ_LIGHT 	0
#define VERT_LIGHT 	1
#define HORIZ_DARK 	2
#define VERT_DARK	3
SDL_Surface *edge[4];	/* hikari to kage */

/* this structure holds all of the color styles we have been able to load
 * for this game */
typedef struct color_styles_struct {
    int num_style;
    int choice;
    color_style **style;
} color_styles;

/* random number. ZEROTO(5)=0,1,2,3,4 */
#define ZEROTO(x)       (FastRandom(x))

#include ".protos/piece.pro"

#endif
	
/*
 * $Log: piece.h,v $
 * Revision 1.10  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.9  2000/11/06 01:25:54  weimer
 * add in the other special piece
 *
 * Revision 1.8  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.7  2000/11/03 03:41:35  weimer
 * made the light and dark "edges" of pieces global, rather than part of a
 * specific color style. also fixed a bug where we were updating too much
 * when drawing falling pieces (bad min() code on my part)
 *
 * Revision 1.6  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.5  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.4  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.3  2000/09/03 18:26:11  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.2  2000/08/13 19:27:20  weimer
 * added file changelogs
 *
 */
