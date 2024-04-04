/*
 *                               Alizarin Tetris
 * The sound interface header file.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#ifndef __SOUND_H
#define __SOUND_H

#include <SDL/SDL_audio.h>
#include <SDL/SDL_error.h>

#define MAX_MIXED_SAMPLES	32

/* a sample to be played */
typedef struct playing_sample_struct {
    int		in_use;		/* only play this if it is in use */
    int		delay;		/* bytes to wait before starting */
    Uint32	len;
    Uint32	pos;
    Uint8 *	audio_data;
    char *	filename;
} playing_sample;

/* all samples to be played */
typedef struct samples_to_be_played_struct {
    playing_sample sample[MAX_MIXED_SAMPLES];	
} samples_to_be_played;

typedef struct WAV_sample_struct {
    SDL_AudioSpec	spec;
    Uint8 *		audio_buf;
    Uint32 		audio_len;
    char *		filename;
} WAV_sample;

#define SOUND_THUD	0	/* the piece you were moving settled */
#define SOUND_CLEAR1	1	/* you cleared a row */
#define SOUND_CLEAR4	2	/* you clear four at once */
#define SOUND_LEVELUP	3	/* you have moved to the next level */
#define SOUND_LEVELDOWN	4	/* you have moved to the previous level */
#define SOUND_GARBAGE1  5	/* opponent gives you garbage */
#define SOUND_CLOCK	6	/* running out of time! */
#define NUM_SOUND	7

typedef struct sound_style_struct {
    WAV_sample WAV[NUM_SOUND];
    char *name;	/* name of this sound style */
} sound_style;

typedef struct sound_styles_struct {
    int num_style;
    int choice;
    sound_style **style;
} sound_styles;

#include ".protos/sound.pro"

#endif

/*
 * $Log: sound.h,v $
 * Revision 1.10  2000/10/27 00:07:36  weimer
 * some sound fixes ...
 *
 * Revision 1.9  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.8  2000/10/19 00:20:27  weimer
 * sound directory changes, added a ticking clock ...
 *
 * Revision 1.7  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.6  2000/09/03 18:26:11  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.5  2000/08/26 02:35:23  weimer
 * add sounds when you are given garbage
 *
 * Revision 1.4  2000/08/26 00:56:47  weimer
 * client-server mods, plus you can now disable sound
 *
 * Revision 1.3  2000/08/15 00:48:28  weimer
 * Massive sound rewrite, fixed a few memory problems. We now have
 * sound_styles that work like piece_styles and color_styles.
 *
 * Revision 1.2  2000/08/13 21:53:08  weimer
 * preliminary sound support (mixing multiple samples, etc.)
 *
 * Revision 1.1  2000/08/13 21:24:03  weimer
 * preliminary sound
 *
 */
