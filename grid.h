/*
 *                               Alizarin Tetris
 * The game board definition structures. 
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */
#pragma once
#ifndef __GRID_H
#define __GRID_H
#include "piece.h"

/* the garbage height goes up every two levels, and the speed goes up 
 * every two levels, but the increases are staggered */
#define GARBAGE_LEVEL(level)	(Options.faster_levels ? level : ((level)/2) )
#define SPEED_LEVEL(level)	(Options.faster_levels ? level : ((level+1)/2) )

typedef struct { /* the playing area */
    int w;	/* width of the grid (e.g., 10) */
    int h;	/* height of the grid (e.g., 20) */
    unsigned char *contents;	/* what is at that square? */
    unsigned char *fall;	/* what is falling? */
    unsigned char *changed;	/* has this square changed since last draw? */
    unsigned char *temp;	/* scratch space for temporary values */
    SDL_Rect board;	/* ours, the opponents */
} Grid;
/* accessor macro */
#define GRID_CONTENT(g,x,y) ((g).contents[(x) + ((y)*((g).w))])
#define GRID_CHANGED(g,x,y) ((g).changed[(x) + ((y)*((g).w))])
#define GRID_SET(g,x,y,n)   (((g).changed [(x)+((y)*((g).w))]|=\
	    (g).contents[(x)+((y)*((g).w))] != (n)),\
	    (g).contents[(x)+((y)*((g).w))]=(n))
#define FALL_CONTENT(g,x,y) ((g).fall[(x) + ((y)*((g).w))])
#define FALL_SET(g,x,y,n)   (((g).changed[(x)+((y)*((g).w))] |=\
	    ((g).fall[(x)+((y)*((g).w))] != (n))),\
	    (g).fall[(x)+((y)*((g).w))]=(n))
#define TEMP_CONTENT(g,x,y) ((g).temp[(x) + ((y)*((g).w))])

#define FALLING 	0
#define NOT_FALLING	1
#define UNKNOWN		254

#define REMOVE_ME	255

#include ".protos/grid.pro"

#endif

/*
 * $Log: grid.h,v $
 * Revision 1.7  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.6  2000/10/26 22:23:07  weimer
 * double the effective number of levels
 *
 * Revision 1.5  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.4  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.3  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.2  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.1  2000/09/03 17:56:36  wkiri
 * Reorganization of files (display.[ch], event.c are new).
 *
 * Revision 1.7  2000/08/26 02:28:55  weimer
 * now you can see the other player!
 *
 * Revision 1.6  2000/08/26 01:36:25  weimer
 * fixed a long-running update bug: the grid changed field was being reset to
 * 0 when falling states or content were set to the same thing, even if they
 * had actually previously changed ...
 *
 * Revision 1.5  2000/08/24 02:32:18  weimer
 * gameplay changes (key repeats) and speed modifications (falling pieces,
 * redraws, etc.)
 *
 * Revision 1.4  2000/08/22 15:13:53  weimer
 * added a garbage-generation routine
 *
 * Revision 1.3  2000/08/15 00:59:41  weimer
 * a few more sound things
 *
 * Revision 1.2  2000/08/13 19:27:20  weimer
 * added file changelogs
 *
 */
