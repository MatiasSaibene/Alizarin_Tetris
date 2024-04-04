/*
 *                               Alizarin Tetris
 * Interfaces relating to AI players. 
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */
#ifndef __AI_H
#define __AI_H

#include "grid.h"
#include "piece.h"

/* These are the possible commands an AI (or a player) can issue with
 * respect to the currently falling piece.
 */
typedef enum {
    MOVE_NONE		= 0,
    MOVE_LEFT		= 1,
    MOVE_RIGHT		= 2,
    MOVE_ROTATE		= 3,
    MOVE_DOWN		= 4,
} Command;

/* An AI player has a name and must implement these three functions. */
typedef struct AI_Player_struct {
    char *name;	
    char *msg;
    Command (*move)(void *state, Grid *, play_piece *, play_piece *, 
		    int , int , int );
    void (*think)  (void *state, Grid *, play_piece *, play_piece *,
		    int , int , int );
    void * (*reset)  (void *state, Grid *);
    int delay_factor;	
} AI_Player;

typedef struct AI_Players_struct {
    int n;
    AI_Player	*player;
} AI_Players; 

#endif
