/*
 *                               Alizarin Tetris
 * Run-time and startup-time options. 
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */
#ifndef __OPTIONS_H
#define __OPTIONS_H

struct option_struct {
    /* these are startup-time options */
    int bpp_wanted;
    int sound_wanted;	/* you can select no-sound later */

    /* these are run-time options: you can change them in the game */
    int full_screen;
    int flame_wanted;
    int special_wanted; /* "int" because network uses it */
    int faster_levels;
    int long_settle_delay;
    int upward_rotation;
    int key_repeat_delay;
    /* what did ".atrisrc" say about these? */
    int named_color;
    int named_sound;
    int named_piece;
    int named_game;
} Options;

#endif
