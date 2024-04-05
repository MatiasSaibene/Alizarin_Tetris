/*
 *                               Alizarin Tetris
 * The main header file. Data structures, definitions and global variables
 * needed by all.
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */
#pragma once
#ifndef __ATRIS_H
#define __ATRIS_H

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <SDL/SDL.h>

/* configure magic for string.h */
#if STDC_HEADERS
# include <string.h>
#else
# ifndef HAVE_STRCHR
#  define strchr index
#  define strrchr rindex
# endif
char *strchr (), *strrchr ();
# ifndef HAVE_MEMCPY
#  define memcpy(d, s, n) bcopy ((s), (d), (n))
#  define memmove(d, s, n) bcopy ((s), (d), (n))
# endif
# ifndef HAVE_STRERROR
#  define strerror(x) "unknown error message (no strerror())"
# endif
# ifndef HAVE_STRDUP
#  error "We need strdup()!"
# endif
# ifndef HAVE_STRSTR
#  error "We need strstr()!"
# endif
#endif

/* configure magic for time.h */
#if TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

typedef enum { 
    SINGLE		=0, 
    MARATHON		=1, 
    SINGLE_VS_AI	=2,
    TWO_PLAYERS		=3,
    NETWORK		=4, 
    AI_VS_AI		=5,
    QUIT		=6,
    DEMO		=7
} GT;
GT gametype;

#ifndef min
#define min(a,b)	((a)<(b)?(a):(b))
#endif
#ifndef max
#define max(a,b)	((a)>(b)?(a):(b))
#endif

/* Debugging Output */
#define Debug(fmt, args...) printf("%-14.14s| ",__FUNCTION__), printf(fmt, ## args), fflush(stdout)

/* Panic! Exit gracelessly. */
void Panic(const char *func, const char *file, char *fmt, ...) __attribute__ ((noreturn));
#define PANIC(fmt, args...) Panic(__FUNCTION__,__FILE__,fmt, ##args)

#define Malloc(ptr,cast,size) {if(!(ptr=(cast)malloc(size)))PANIC("Out of Memory:\n\tcannot allocate %d bytes for "#ptr,size);}
#define Calloc(ptr,cast,size) {if(!(ptr=(cast)calloc(size,1)))PANIC("Out of Memory:\n\tcannot callocate %d bytes for "#ptr,size);}
#define Realloc(ptr,cast,size) {if(!(ptr=(cast)realloc(ptr,size)))PANIC("Out of Memory:\n\tcannot reallocate %d bytes for "#ptr,size);}
#define Free(ptr)       {free(ptr);ptr=NULL;}
#ifdef __STRING
#define Assert(cond)    {if(!(cond))PANIC("Failed assertion \"%s\" on line %d",__STRING(cond),__LINE__);}
#else
#define Assert(cond)    {if(!(cond))PANIC("Failed assertion \"%s\" on line %d",#cond,__LINE__);}
#endif

extern void SeedRandom(Uint32 Seed);
extern Uint16 FastRandom(Uint16 range);


/*
 * Quick safety routines. 
 */ 
#define SDL_BlitSafe(a,b,c,d) { SDL_Rect *my_b = b, *my_d = d; \
    if (my_b) { \
	if (my_b->x < 0) my_b->x = 0; \
	if (my_b->y < 0) my_b->y = 0; \
	if (my_b->x + my_b->w > 640) my_b->w = 640-my_b->x; \
	if (my_b->y + my_b->h > 480) my_b->h = 480-my_b->y; \
    } if (my_d) { \
	if (my_d->x < 0) my_d->x = 0; \
	if (my_d->y < 0) my_d->y = 0; \
	if (my_d->x + my_d->w > 640) my_d->w = 640-my_d->x; \
	if (my_d->y + my_d->h > 480) my_d->h = 480-my_d->y; \
    } \
    Assert(SDL_BlitSurface(a,my_b,c,my_d) == 0); }
#define SDL_UpdateSafe(a,b,c) { SDL_Rect *my_c = c; \
    Assert(my_c); \
    if (my_c) { \
	if (my_c->x < 0) my_c->x = 0; \
	if (my_c->y < 0) my_c->y = 0; \
	if (my_c->x + my_c->w > 640) my_c->w = 640-my_c->x; \
	if (my_c->y + my_c->h > 480) my_c->h = 480-my_c->y; \
    } \
    SDL_UpdateRects(a,b,c); }

#define ADJUST_UP	0
#define ADJUST_SAME	1
#define ADJUST_DOWN	2

#include ".protos/atris.pro"

#endif /* __ATRIS_H */

/*
 * $Log: atris.h,v $
 * Revision 1.19  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.18  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.17  2000/10/29 03:16:59  weimer
 * fflush on "Debug()"-style output
 *
 * Revision 1.16  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.15  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.14  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.13  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.12  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.11  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.10  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.9  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.8  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.7  2000/09/03 21:06:31  wkiri
 * Now handles three different game types (and does different things).
 * Major changes in display.c to handle this.
 *
 * Revision 1.6  2000/09/03 19:41:30  wkiri
 * Now allows you to choose the game type (though it doesn't do anything yet).
 *
 * Revision 1.5  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.4  2000/09/03 17:56:36  wkiri
 * Reorganization of files (display.[ch], event.c are new).
 *
 * Revision 1.3  2000/08/14 01:06:45  wkiri
 * Keeps track of high scores and adds to the list when appropriate.
 * The high scores are kept in highscores.txt.
 *
 * Revision 1.2  2000/08/13 21:24:03  weimer
 * preliminary sound
 *
 * Revision 1.1  2000/08/13 19:08:45  weimer
 * Initial CVS creation for Atris.
 *
 * Revision 1.4  2000/08/05 00:50:53  weimer
 * Weimer Changes
 *
 * Revision 1.3  2000/07/23 16:41:50  weimer
 * yada
 *
 * Revision 1.2  2000/07/23 13:48:55  weimer
 * added link to piece.h
 *
 * Revision 1.1  2000/07/23 13:29:30  wkiri
 * Initial revision
 *
 */
