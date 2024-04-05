/*
 *                               Alizarin Tetris
 * Display commonalities.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */
#pragma once
#ifndef __DISPLAY_H
#define __DISPLAY_H
#include <SDL/SDL_ttf.h>

SDL_Color color_white;
SDL_Color color_black;
SDL_Color color_red;
SDL_Color color_blue;
SDL_Color color_purple;

Uint32	int_black;
Uint32  int_white;
Uint32	int_grey;
Uint32	int_blue;
Uint32 	int_med_blue;
Uint32 	int_dark_blue;
Uint32	int_purple;
Uint32  int_dark_purple;
Uint32  int_solid_black;

SDL_Surface *screen, *widget_layer, *flame_layer;
TTF_Font *font, *sfont, *lfont, *hfont;	/* normal, small , large, huge font */

#define int_border_color	int_grey
#define int_button_face1	int_dark_blue
#define int_button_face0	int_solid_black
#define int_button_text1	int_white
#define int_button_text0	int_purple
#define int_button_border0	int_solid_black
#define int_button_border1	int_grey
#define int_highscore_color	int_blue
#define int_input_color0	int_blue
#define int_input_color1	int_purple

#define color_network_menu	color_blue
#define color_who_are_you	color_blue
#define color_ai_menu		color_blue
#define color_keys_explain	color_blue

#endif
