/*
 *                               Alizarin Tetris
 * A generic way to display menus.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <assert.h>

#include "atris.h"
#include "display.h"
#include "menu.h"

/***************************************************************************
 *      menu()
 * Create a new menu and return it.
 *********************************************************************PROTO*/
ATMenu* menu(int nButtons, char** strings, int defaultchoice,
	     Uint32 face_color0, Uint32 text_color0,
	     Uint32 face_color1, Uint32 text_color1, int x, int y)
{
  int i, yp;
  ATMenu* am = (ATMenu*)malloc(sizeof(ATMenu)); assert(am);

  /* Create the buttons */
  am->buttons = (ATButton**)malloc(nButtons*sizeof(ATButton*)); 
  assert(am->buttons);
  am->clicked = (char*)malloc(nButtons*sizeof(char)); assert(am->clicked);
  am->nButtons = nButtons;
  am->x = x; am->y = y;
  am->defaultchoice = defaultchoice;

  yp = y;

  Assert(nButtons >= 0);

  for (i=0; i<nButtons; i++)
    {
      /* Line them up x-wise */
      am->buttons[i] = button(strings[i], face_color0, text_color0,
			      face_color1, text_color1, x, yp);
      yp += am->buttons[i]->area.h; /* add a separating space? */
      am->clicked[i] = FALSE;
    }

  am->w = am->buttons[0]->area.w;
  am->h = (am->buttons[nButtons-1]->area.y - am->buttons[0]->area.y)
    + am->buttons[nButtons-1]->area.h;
  /* Set the default one */
  if (defaultchoice >= 0) am->clicked[defaultchoice] = TRUE;

  return am;
}

/***************************************************************************
 *      show_menu()
 * Displays the menu.
 *********************************************************************PROTO*/
void show_menu(ATMenu* am)
{
  int i;
  for (i=0; i<am->nButtons; i++)
    show_button(am->buttons[i], am->clicked[i]);
}

/***************************************************************************
 *      check_menu()
 * Returns the index of the button that was clicked (-1 if none).
 *********************************************************************PROTO*/
int check_menu(ATMenu* am, int x, int y)
{
  int i, j;
  for (i=0; i<am->nButtons; i++)
    if (check_button(am->buttons[i], x, y)) 
      {
	/* invert its clicked state */
	if (am->clicked[i])
	  {
	    am->clicked[i] = FALSE;
	    /* reset the default */
	    am->clicked[am->defaultchoice] = TRUE;
	    show_button(am->buttons[am->defaultchoice],
			am->clicked[am->defaultchoice]);
	  }
	else
	  {
	    /* blank all others */
	    for (j=0; j<am->nButtons; j++)
	      if (i!=j && am->clicked[j])
		{
		  am->clicked[j] = FALSE;
		  show_button(am->buttons[j], am->clicked[j]);
		}
	    am->clicked[i] = TRUE;
	  }
	/* redraw it */
	show_button(am->buttons[i], am->clicked[i]);
	Debug("Button %d was clicked.\n", i);
	return i;
      }
  return -1;
}

/***************************************************************************
 *      delete_menu()
 * Deletes the menu and all of its buttons.
 *********************************************************************PROTO*/
void delete_menu(ATMenu* am)
{
  int i;
  for (i=0; i<am->nButtons; i++)
    Free(am->buttons[i]);
  Free(am);
}

#define BORDER_X	8
#define BORDER_Y	8
#define BUTTON_X_SPACE	4
#define RADIO_HEIGHT	12

/***************************************************************************
 *      setup_radio()
 * Prepared a WalkRadio button for display.
 *********************************************************************PROTO*/
void
setup_radio(WalkRadio *wr)
{
    int i;
    int our_height;

    wr->face_color[0] = int_button_face0;
    wr->face_color[1] = int_button_face1;
    wr->text_color[0] = int_button_text0;
    wr->text_color[1] = int_button_text1;
    wr->border_color[0] = int_button_border0;
    wr->border_color[1] = int_button_border1;

    wr->bitmap0 = malloc(sizeof(SDL_Surface *) * wr->n); Assert(wr->bitmap0);
    wr->bitmap1 = malloc(sizeof(SDL_Surface *) * wr->n); Assert(wr->bitmap1);
    wr->w = 0;
    wr->h = 0;

    for (i=0; i<wr->n; i++) {
	SDL_Color sc;

	SDL_GetRGB(wr->text_color[0], screen->format, &sc.r, &sc.g, &sc.b);
	wr->bitmap0[i] = TTF_RenderText_Blended(sfont, wr->label[i], sc);
	Assert(wr->bitmap0[i]);

	SDL_GetRGB(wr->text_color[1], screen->format, &sc.r, &sc.g, &sc.b);
	wr->bitmap1[i] = TTF_RenderText_Blended(sfont, wr->label[i], sc);
	Assert(wr->bitmap1[i]);

	if (wr->bitmap0[i]->w + BUTTON_X_SPACE > wr->w) 
	    wr->w = wr->bitmap0[i]->w + BUTTON_X_SPACE;
	if (wr->bitmap0[i]->h > wr->h) wr->h = wr->bitmap0[i]->h;
    }
    wr->w += 5 ;
    wr->h += 10 ;
    wr->area.w = wr->w + BORDER_X;

    our_height = RADIO_HEIGHT;
    if (wr->n < our_height) our_height = wr->n;

    wr->area.h = wr->h * our_height + BORDER_Y;
    return;
}

/***************************************************************************
 *      check_radio()
 * Determines if we have clicked in a radio button.
 *********************************************************************PROTO*/
int
check_radio(WalkRadio *wr, int x, int y)
{
    if (wr->inactive) return 0;

    if (    x  >= wr->area.x &&
	    x  <= wr->area.x + wr->area.w &&
	    y  >= wr->area.y &&
	    y  <= wr->area.y + wr->area.h   ) {
	int start, stop;
	int c;

	if (wr->n < RADIO_HEIGHT) {	/* draw them all */
	    start = 0;
	    stop = wr->n;
	} else {		/* at least four */
	    start = wr->defaultchoice - (RADIO_HEIGHT/2);
	    if (start < 0) start = 0;
	    stop = start + RADIO_HEIGHT;
	    if (stop > wr->n) {	/* we're at the bottom */
		start -= (stop - wr->n);
		stop = wr->n;
	    }
	}
	c = (y - wr->area.y) / wr->h;
	c += start;
	if (c < 0) c = 0;
	if (c >= wr->n) c = wr->n - 1;
	wr->defaultchoice = c;
	return 1;
    }
    return 0;
}

/***************************************************************************
 *      clear_radio()
 * Clears away a WalkRadio button. Works even if I am inactive. 
 *********************************************************************PROTO*/
void
clear_radio(WalkRadio *wr)
{
    SDL_FillRect(widget_layer, &wr->area, int_black);
    SDL_FillRect(screen, &wr->area, int_black);
    SDL_BlitSurface(flame_layer, &wr->area, screen, &wr->area);
    SDL_BlitSurface(widget_layer, &wr->area, screen, &wr->area);
    SDL_UpdateSafe(screen, 1, &wr->area);
    return; 
}

/***************************************************************************
 *      draw_radio()
 * Draws a WalkRadio button.
 *
 * Only draws me if I am not inactive. 
 *********************************************************************PROTO*/
void
draw_radio(WalkRadio *wr, int state)
{
    int start, stop, i;
    SDL_Rect draw;
    
    if (wr->inactive) return;

    wr->area.x = wr->x;
    wr->area.y = wr->y;

    SDL_FillRect(widget_layer, &wr->area, wr->border_color[state]);

    if (wr->n < RADIO_HEIGHT) {	/* draw them all */
	start = 0;
	stop = wr->n;
    } else {		/* at least four */
	start = wr->defaultchoice - (RADIO_HEIGHT/2);
	if (start < 0) start = 0;
	stop = start + RADIO_HEIGHT;
	if (stop > wr->n) {	/* we're at the bottom */
	    start -= (stop - wr->n);
	    stop = wr->n;
	}
    }

    draw.x = wr->x + BORDER_X/2;
    draw.y = wr->y + BORDER_Y/2;
    draw.w = wr->w; 
    draw.h = wr->h;
    
    for (i=start; i<stop; i++) {
	int on = (wr->defaultchoice == i);
	SDL_Rect text;

	SDL_FillRect(widget_layer, &draw, wr->text_color[on]);
	draw.x += 2; draw.y += 2; draw.w -= 4; draw.h -= 4;
	if (state || on) 
	    SDL_FillRect(widget_layer, &draw, wr->face_color[on]);
	else 
	    SDL_FillRect(widget_layer, &draw, wr->border_color[on]);
	draw.x += 3; draw.y += 3; draw.w -= 6; draw.h -= 6;

	text.x = draw.x; text.y = draw.y;
	text.w = wr->bitmap0[i]->w;
	text.h = wr->bitmap0[i]->h;
	text.x += (draw.w - text.w) / 2;

	if (on) {
	    SDL_BlitSafe(wr->bitmap1[i], NULL, widget_layer, &text);
	} else {
	    SDL_BlitSafe(wr->bitmap0[i], NULL, widget_layer, &text);
	}
	draw.x -= 5; draw.y -= 5; draw.w += 10; draw.h += 10;

	draw.y += draw.h; /* move down! */
    }

    SDL_BlitSurface(flame_layer, &wr->area, screen, &wr->area);
    SDL_BlitSurface(widget_layer, &wr->area, screen, &wr->area);
    SDL_UpdateSafe(screen, 1, &wr->area);
}

/***************************************************************************
 *      create_single_wrg()
 * Creates a walk-radio-group with a single walk-radio button with N
 * choices. 
 *********************************************************************PROTO*/
WalkRadioGroup *
create_single_wrg(int n)
{
    WalkRadioGroup *wrg;

    Calloc(wrg, WalkRadioGroup *, 1 * sizeof(*wrg));
    wrg->n = 1;
    Calloc(wrg->wr, WalkRadio *, wrg->n * sizeof(*wrg->wr));
    wrg->wr[0].n = n;
    Calloc(wrg->wr[0].label, char **, wrg->wr[0].n * sizeof(*wrg->wr[0].label));
    return wrg;
}

/***************************************************************************
 *      handle_radio_event()
 * Tries to handle the given event with respect to the given set of
 * WalkRadio's. Returns non-zero if an action said that we are done.
 *********************************************************************PROTO*/
int
handle_radio_event(WalkRadioGroup *wrg, const SDL_Event *ev)
{
    int i, ret;
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
	for (i=0; i<wrg->n; i++) {
	    if (check_radio(&wrg->wr[i], ev->button.x, ev->button.y)) {
		if (i != wrg->cur) {
		    draw_radio(&wrg->wr[wrg->cur], 0);
		}
		draw_radio(&wrg->wr[i], 1);
		if (wrg->wr[i].action) {
		    ret = wrg->wr[i].action(&wrg->wr[i]);
		    return ret;
		} else
		    return wrg->wr[i].defaultchoice;
	    }
	}
    } else if (ev->type == SDL_KEYDOWN) {
	switch (ev->key.keysym.sym) {
	    case SDLK_RETURN:
		if (wrg->wr[wrg->cur].action) {
		    ret = wrg->wr[wrg->cur].action(&wrg->wr[wrg->cur]);
		    return ret;
		} else
		    return wrg->wr[wrg->cur].defaultchoice;
		return 1;

	    case SDLK_UP:
		wrg->wr[wrg->cur].defaultchoice --;
		if (wrg->wr[wrg->cur].defaultchoice < 0)
		    wrg->wr[wrg->cur].defaultchoice = wrg->wr[wrg->cur].n - 1;
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;
		break;

	    case SDLK_DOWN:
		wrg->wr[wrg->cur].defaultchoice ++;
		if (wrg->wr[wrg->cur].defaultchoice >= wrg->wr[wrg->cur].n)
		    wrg->wr[wrg->cur].defaultchoice = 0;
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    case SDLK_LEFT:
		draw_radio(&wrg->wr[wrg->cur], 0);
		do { 
		    wrg->cur--;
		    if (wrg->cur < 0) 
			wrg->cur = wrg->n - 1;
		} while (wrg->wr[wrg->cur].inactive);
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    case SDLK_RIGHT: 
		draw_radio(&wrg->wr[wrg->cur], 0);
		do { 
		    wrg->cur++;
		    if (wrg->cur >= wrg->n) 
			wrg->cur = 0;
		} while (wrg->wr[wrg->cur].inactive);
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    default:
		return -1;
	}
    }
    return -1;
}

/*
 * $Log: menu.c,v $
 * Revision 1.17  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.16  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.15  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.14  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.13  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.12  2000/10/21 01:22:44  weimer
 * prevent segfault from out-of-range values
 *
 * Revision 1.11  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.10  2000/10/19 22:06:51  weimer
 * minor changes ...
 *
 * Revision 1.9  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.8  2000/10/13 02:26:54  weimer
 * rudimentary identity functions, including adding new players
 *
 * Revision 1.7  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.6  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.5  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
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
 * Revision 1.2  2000/09/04 22:49:51  wkiri
 * gamemenu now uses the new nifty menus.  Also, added delete_menu.
 *
 * Revision 1.1  2000/09/04 19:54:26  wkiri
 * Added menus (menu.[ch]).
 */
