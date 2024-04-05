/*
 *                               Alizarin Tetris
 * A generic way to display menus.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */
#pragma once
#ifndef __MENU_H
#define __MENU_H

#include "button.h"
#include ".protos/button.pro"

typedef struct _ATMenu
{
  int nButtons;
  ATButton** buttons;
  char* clicked; /* one per button */
  int x, y; /* upper left position */
  int w, h;
  int defaultchoice;
} ATMenu;

typedef struct _WalkRadio
{
    int n;		/* number of choices */
    char **label;	/* array of choice labels */
    int x,y;		/* upper-left corner */
    int defaultchoice;	/* the default choice */
    int (* action)(struct _WalkRadio *);
			/* what to do when they make a selection:
			 * a non-zero return value means we are done
			 * with this menu */
    int *var_to_set;
    void *data;
    int inactive;	/* if this is is set, we don't check this 
			 * this particular radio menu for input */
    /* --- set by setup_radio --- */
    Uint32 face_color[2];
    Uint32 text_color[2];
    Uint32 border_color[2];
    int w,h;
    SDL_Rect area;
    SDL_Surface **bitmap0;
    SDL_Surface **bitmap1;
} WalkRadio;

typedef struct _WalkRadioGroup {
    WalkRadio *wr;
    int n;		/* number of walk-radios */
    int cur;		/* currently selected walk radio button group */
} WalkRadioGroup;

#include ".protos/menu.pro"

#endif

/*
 * $Log: menu.h,v $
 * Revision 1.6  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.5  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.4  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.3  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.2  2000/09/04 22:49:51  wkiri
 * gamemenu now uses the new nifty menus.  Also, added delete_menu.
 *
 * Revision 1.1  2000/09/04 19:54:26  wkiri
 * Added menus (menu.[ch]).
 *
 */
