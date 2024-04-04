/*
 *                               Alizarin Tetris
 * A generic way to display buttons.
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */

#define BUFSIZE 256

#include "config.h"
#include <assert.h>

#include "atris.h"
#include "display.h"
#include "button.h"

/***************************************************************************
 *      button()
 * Create a new button and return it.
 *********************************************************************PROTO*/
ATButton* button(char* text, Uint32 face_color0, Uint32 text_color0,
	Uint32 face_color1, Uint32 text_color1,
	int x, int y)
{
  SDL_Color sc;
  ATButton* ab = (ATButton*)malloc(sizeof(ATButton)); Assert(ab);

  /* Copy the colors */
  ab->face_color[0] = face_color0;  
  ab->text_color[0] = text_color0;
  ab->face_color[1] = face_color1;  
  ab->text_color[1] = text_color1;
  /* Copy the text */
  SDL_GetRGB(text_color0, screen->format, &sc.r, &sc.g, &sc.b);
  ab->bitmap[0] = TTF_RenderText_Blended(font, text, sc);
  Assert(ab->bitmap[0]);
  SDL_GetRGB(text_color1, screen->format, &sc.r, &sc.g, &sc.b);
  ab->bitmap[1]  = TTF_RenderText_Blended(font, text, sc);
  Assert(ab->bitmap[1]);
  /* Make the button bg */
  ab->area.x = x; ab->area.y = y;
  ab->area.w = ab->bitmap[0]->w + 10;
  ab->area.h = ab->bitmap[0]->h + 10;
  return ab;
}

/***************************************************************************
 *      show_button()
 * Displays the button, taking into account whether it's clicked or not.
 *********************************************************************PROTO*/
void show_button(ATButton* ab, int state)
{
    SDL_FillRect(screen, &ab->area, ab->text_color[state]);
    SDL_UpdateSafe(screen, 1, &ab->area);
    ab->area.x += 2; ab->area.y += 2;
    ab->area.w -= 4; ab->area.h -= 4;
    SDL_FillRect(screen, &ab->area, ab->face_color[state]);
    SDL_UpdateSafe(screen, 1, &ab->area);
    ab->area.x += 3; ab->area.y += 3;
    ab->area.w -= 6; ab->area.h -= 6;
    SDL_BlitSurface(ab->bitmap[state], NULL, screen, &ab->area);

    ab->area.x -= 5;  ab->area.y -= 5;
    ab->area.w += 10; ab->area.h += 10;
    SDL_UpdateSafe(screen, 1, &ab->area);
}

/***************************************************************************
 *      check_button()
 * Returns 1 if (x,y) is inside the button, else 0.
 * Also sets the is_clicked field in ab.
 *********************************************************************PROTO*/
char check_button(ATButton* ab, int x, int y)
{
  if ((ab->area.x <= x && x <= ab->area.x + ab->area.w) &&
	  (ab->area.y <= y && y <= ab->area.y + ab->area.h)) {
      return 1; 
  } else return 0;
}

/*
 * $Log: button.c,v $
 * Revision 1.9  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.8  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.7  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.6  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.5  2000/09/04 22:49:51  wkiri
 * gamemenu now uses the new nifty menus.  Also, added delete_menu.
 *
 * Revision 1.4  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.3  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.2  2000/08/26 03:11:34  wkiri
 * Buttons now use borders.
 *
 * Revision 1.1  2000/08/26 02:45:28  wkiri
 * Beginnings of button class; also modified atris to query for 'new
 * game' vs. 'quit'.  (New Game doesn't work yet...)
 *
 */
