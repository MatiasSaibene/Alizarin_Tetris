/*
 *                               Alizarin Tetris
 * A generic way to display buttons.
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */
#ifndef __BUTTON_H
#define __BUTTON_H

#include <SDL/SDL_video.h>
typedef struct _ATButton
{
  SDL_Surface* bitmap[2];
  SDL_Rect area;
  Uint32 face_color[2]; 
  Uint32 text_color[2];
} ATButton;

#include ".protos/button.pro"

#endif

/*
 * $Log: button.h,v $
 * Revision 1.4  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.3  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.2  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.1  2000/08/26 02:46:08  wkiri
 * Adding button.h
 *
 */
