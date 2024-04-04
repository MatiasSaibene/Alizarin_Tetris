/*
 *                               Alizarin Tetris
 * Functions relating to AI players. 
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */

#include "config.h"

#include "atris.h"
#include "display.h"
#include "identity.h"
#include "grid.h"
#include "piece.h"
#include "sound.h"
#include "ai.h"
#include "menu.h"

#include ".protos/event.pro"
#include ".protos/identity.pro"
#include ".protos/display.pro"

/*********** Wes's globals ***************/


typedef struct wessy_struct {
    int know_what_to_do;
    int desired_column;
    int desired_rot;
    int cc;
    int current_rot;
    int best_weight;
    Grid tg;
} Wessy_State;

typedef struct double_struct {
    int know_what_to_do;

    int desired_col;
    int desired_rot;
    int best_weight;

    int stage_alpha;
    int cur_alpha_col;
    int cur_alpha_rot;
    Grid ag;
    int cur_beta_col;
    int cur_beta_rot;
    Grid tg;
} Double_State;

#define WES_MIN_COL -4
static int weight_board(Grid *g);

/***************************************************************************
 * The code here determines what the board would look like after all the
 * color-sliding and tetris-clearing that would happen if you pasted the
 * given piece (pp) onto the given scratch grid (tg) at location (x,y) and
 * rotation (current_rot). You are welcome to copy it. 
 *
 * Returns -1 on failure or the number of lines cleared.
 ***************************************************************************/
int
drop_piece_on_grid(Grid *g, play_piece *pp, int col, int row, int rot)
{
    int should_we_loop = 0;
    int lines_cleared = 0;

    if (!valid_position(pp, col, row, rot, g))
	return -1;

    while (valid_position(pp, col, row, rot, g))
	row++;
    row--; /* back to last valid position */

    /* *-*-*-*
     */ 
    if (!valid_position(pp, col, row, rot, g))
	return -1;

    if (pp->special != No_Special) {
	handle_special(pp, row, col, rot, g, NULL);
	cleanup_grid(g);
    } else 
	paste_on_board(pp, col, row, rot, g);
    do { 
	int x,y,l;
	should_we_loop = 0;
	if ((l = check_tetris(g))) {
	    cleanup_grid(g);
	}
	lines_cleared += l;
	for (y=g->h-1;y>=0;y--) 
	    for (x=g->w-1;x>=0;x--) 
		FALL_SET(*g,x,y,UNKNOWN);
	run_gravity(g);

	if (determine_falling(g)) {
	    do { 
		fall_down(g);
		cleanup_grid(g);
		run_gravity(g);
	    } while (determine_falling(g));
	    should_we_loop = 1;
	}
    } while (should_we_loop);
    /* 
     * Simulation code ends here. 
     * *-*-*-*
     */
    return lines_cleared;
}

/***************************************************************************
 *      double_ai_reset()
 **************************************************************************/
static void *
double_ai_reset(void *state, Grid *g)
{	
    Double_State *retval;
    /* something has changed (e.g., a new piece) */

    if (state == NULL) {
	/* first time we've been called ... */
	Calloc(retval, Double_State *, sizeof(Double_State));
    } else
	retval = state;
    Assert(retval);
    retval->know_what_to_do=0;
    retval->desired_col = g->w / 2;
    retval->desired_rot = 0;
    retval->best_weight = 1<<30;
    retval->stage_alpha = 1;
    retval->cur_alpha_col = WES_MIN_COL;
    retval->cur_alpha_rot = 0;
    retval->cur_beta_col = WES_MIN_COL;
    retval->cur_beta_rot = 0;

    if (retval->tg.contents == NULL)
	retval->tg = generate_board(g->w, g->h, 0); 
    if (retval->ag.contents == NULL)
	retval->ag = generate_board(g->w, g->h, 0); 

    return retval;
}

/***************************************************************************
 *
 ***************************************************************************/
static void
double_ai_think(void *data, Grid *g, play_piece *pp, play_piece *np, 
	int col, int row, int rot)
{
    Double_State *ds = (Double_State *)data;
    Uint32 incoming_time = SDL_GetTicks();

    Assert(ds);

    for (;SDL_GetTicks() == incoming_time;) {
	if (ds->know_what_to_do) 
	    return;

	if (ds->stage_alpha) {
	    memcpy(ds->ag.contents, g->contents, (g->w * g->h * sizeof(ds->ag.contents[0])));
	    memcpy(ds->ag.fall, g->fall, (g->w * g->h * sizeof(ds->ag.fall[0])));

	    if ( drop_piece_on_grid(&ds->ag, pp, ds->cur_alpha_col, row,
		    ds->cur_alpha_rot) != -1) {
		int weight;
		/* success */
		ds->cur_beta_col = WES_MIN_COL;
		ds->cur_beta_rot = 0;

		ds->stage_alpha = 0;

		weight = weight_board(&ds->ag);
		if (weight <= 0) {
		    ds->best_weight = weight;
		    ds->desired_col = ds->cur_alpha_col;
		    ds->desired_rot = ds->cur_alpha_rot;
		    ds->know_what_to_do = 1;
		}
	    } else {
		/* advance alpha */
		if (++ds->cur_alpha_col == g->w) {
		    ds->cur_alpha_col = WES_MIN_COL;
		    if (++ds->cur_alpha_rot == 4) {
			ds->cur_alpha_rot = 0;
			ds->know_what_to_do = 1;
		    }
		}
	    }
	} else {
	    /* stage beta */
	    int weight;
	    
	    memcpy(ds->tg.contents, ds->ag.contents, (ds->ag.w * ds->ag.h * sizeof(ds->ag.contents[0])));
	    memcpy(ds->tg.fall, ds->ag.fall, (ds->ag.w * ds->ag.h * sizeof(ds->ag.fall[0])));

	    if (drop_piece_on_grid(&ds->tg, np, ds->cur_beta_col, row,
		    ds->cur_beta_rot) != -1) {
		/* success */
		weight = 1+weight_board(&ds->tg);
		if (weight < ds->best_weight) {
		    ds->best_weight = weight;
		    ds->desired_col = ds->cur_alpha_col;
		    ds->desired_rot = ds->cur_alpha_rot;
		}
	    }

	    if (++ds->cur_beta_col == g->w) {
		ds->cur_beta_col = WES_MIN_COL;
		if (++ds->cur_beta_rot == 2) {
		    ds->cur_beta_rot = 0;

		    ds->stage_alpha = 1;
		    if (++ds->cur_alpha_col == g->w) {
			ds->cur_alpha_col = WES_MIN_COL;
			if (++ds->cur_alpha_rot == 4) {
			    ds->cur_alpha_rot = 0;
			    ds->know_what_to_do = 1;
			}
		    }
		}
	    }
	} /* endof: stage beta */
    } /* for: iterations */
}

/***************************************************************************
 *      double_ai_move()
 ***************************************************************************/
static Command
double_ai_move(void *state, Grid *g, play_piece *pp, play_piece *np, 
	int col, int row, int rot)
{
    /* naively, there are ((g->w - pp->base->dim) * 4) choices */
    /* determine how to get there ... */
    Double_State *ws = (Double_State *) state;

    if (rot != ws->desired_rot)
	return MOVE_ROTATE;
    else if (col > ws->desired_col) 
	return MOVE_LEFT;
    else if (col < ws->desired_col) 
	return MOVE_RIGHT;
    else if (ws->know_what_to_do) {
	return MOVE_DOWN;
    } else 
	return MOVE_NONE;
}

/***************************************************************************
 *      weight_board()
 * Determines the value the AI places on the given board configuration.
 * This is a Wes-specific function that is used to evaluate the result of
 * a possible AI choice. In the end, the choice with the best weight is
 * selected.
 ***************************************************************************/
static int
weight_board(Grid *g)
{
    int x,y;
    int w = 0;
    int holes = 0;
    int same_color = 0;
    int garbage = 0;

    /* favor the vast extremes ... */
    int badness[10] =  { 7, 9, 9, 9, 9,  9, 9, 9, 9, 7};

    /* 
     * Simple Heuristic: highly placed blocks are bad, as are "holes":
     * blank areas with blocks above them.
     */
    for (x=0; x<g->w; x++) {
	int possible_holes = 0;
	for (y=g->h-1; y>=0; y--) {
	    int what;
	    if ((what = GRID_CONTENT(*g,x,y))) {
		w += 2 * (g->h - y) * badness[x] / 3;
		if (possible_holes) {
		    if (what != 1) 
			holes += 3 * ((g->h - y)) * g->w * possible_holes;
		    possible_holes = 0;
		}

		if (x > 1 && what)
		    if (GRID_CONTENT(*g,x-1,y) == what)
			same_color++;
		if (what == 1)
		    garbage++;
	    } else {
		possible_holes++;
	    }
	}
    }
    w += holes * 2;
    w += same_color * 4;
    if (garbage == 0) w = 0;	/* you'll win! */

    return w;
}

/***************************************************************************
 *      wes_ai_think()
 * Ruminates for the Wessy AI.
 *
 * This function is called every so (about every fall_event_interval) by
 * event_loop(). The AI is expected to think for < 1 "tick" (as in,
 * SDL_GetTicks()). 
 *
 * Input:
 * 	Grid *g		Your side of the board. The currently piece (the
 * 			you are trying to place) is not "stamped" on the
 * 			board yet. You may not modify this board, but you
 * 			may make copies of it in order to reason about it.
 * 	play_piece *pp	This describes the piece you are currently trying
 * 			to place. You can use it in conjunction with
 * 			functions like "valid_position()" to see where
 * 			possible placements are.
 * 	play_piece *np	This is the next piece you will be given.
 * 	int col,row	The current grid coordinates of your (falling)
 * 			piece.
 *	int rot		The current rotation (0-3) of your (falling) piece.
 *
 * Output:
 *      Nothing		Later, "ai_move()" will be called. Return your
 *      		value (== your action) there. 
 ***************************************************************************/
static void
wes_ai_think(void *data, Grid *g, play_piece *pp, play_piece *np, 
	int col, int row, int rot)
{
    int weight;
    Wessy_State *ws = (Wessy_State *)data;
    Uint32 incoming_time = SDL_GetTicks();

    Assert(ws);

    for (;SDL_GetTicks() == incoming_time;) {

	if (ws->know_what_to_do) 
	    return;

	memcpy(ws->tg.contents, g->contents, (g->w * g->h * sizeof(ws->tg.contents[0])));
	memcpy(ws->tg.fall, g->fall, (g->w * g->h * sizeof(ws->tg.fall[0])));
	/* what would happen if we dropped ourselves on cc, current_rot now? */
	if (drop_piece_on_grid(&ws->tg, pp, ws->cc, row, ws->current_rot) != -1) {
	    weight = weight_board(&ws->tg);

	    if (weight < ws->best_weight) {
		ws->best_weight = weight;
		ws->desired_column = ws->cc;
		ws->desired_rot = ws->current_rot;
		if (weight == 0)
		    ws->know_what_to_do = 1;
	    } 
	}
	if (++(ws->cc) == g->w)  {
	    ws->cc = 0;
	    if (++(ws->current_rot) == 4) 
		ws->know_what_to_do = 1;
	}
    }
}

/***************************************************************************
 ***************************************************************************/
static void
beginner_ai_think(void *data, Grid *g, play_piece *pp, play_piece *np, 
	int col, int row, int rot)
{
    int weight;
    Wessy_State *ws = (Wessy_State *)data;

    Assert(ws);

    if (ws->know_what_to_do || (SDL_GetTicks() & 3)) 
	return;

    memcpy(ws->tg.contents, g->contents, (g->w * g->h * sizeof(ws->tg.contents[0])));
    memcpy(ws->tg.fall, g->fall, (g->w * g->h * sizeof(ws->tg.fall[0])));
    /* what would happen if we dropped ourselves on cc, current_rot now? */

    if (drop_piece_on_grid(&ws->tg, pp, ws->cc, row, ws->current_rot) != -1) {

	weight = weight_board(&ws->tg);

	if (weight < ws->best_weight) {
	    ws->best_weight = weight;
	    ws->desired_column = ws->cc;
	    ws->desired_rot = ws->current_rot;
	    if (weight == 0)
		ws->know_what_to_do = 1;
	} 
    }
    if (++(ws->cc) == g->w)  {
	ws->cc = 0;
	if (++(ws->current_rot) == 4) 
	    ws->know_what_to_do = 1;
    }
}


/***************************************************************************
 *      wes_ai_reset()
 * This is a special function instructing you to that the something has
 * changed (for example, you dropped your piece and you will have a new one
 * soon) and that you should clear any state you have lying around. 
 **************************************************************************/
static void *
wes_ai_reset(void *state, Grid *g)
{	
    Wessy_State *retval;
    /* something has changed (e.g., a new piece) */

    if (state == NULL) {
	/* first time we've been called ... */
	Calloc(retval, Wessy_State *, sizeof(Wessy_State));
    } else
	retval = state;
    Assert(retval);

    retval->know_what_to_do = 0;
    retval->cc = WES_MIN_COL; 
    retval->current_rot = 0;
    retval->best_weight = 1<<30;
    retval->desired_column = g->w / 2;
    retval->desired_rot = 0;

    if (retval->tg.contents == NULL)
	retval->tg = generate_board(g->w, g->h, 0); 

    return retval;
}

/***************************************************************************
 *      wes_ai_move()
 * Determines the AI's next move. All of the inputs are as for ai_think().
 * This function should be virtually instantaneous (do long calculations in
 * ai_think()). Possible return values are:
 * 	MOVE_NONE	Do nothing, fall normally.
 * 	MOVE_LEFT	[Try to] Move one column to the left.
 * 	MOVE_RIGHT	[Try to] Move one column to the right.
 * 	MOVE_ROTATE	[Try to] Rotate the piece.
 * 	MOVE_DOWN	Fall down really quickly.
 *
 * This is a special form instructing you to that the something has changed
 * (for example, you dropped your piece and you will have a new one soon)
 * and that you should clear any state you have lying around. 
 **************************************************************************/
static Command
wes_ai_move(void *state, Grid *g, play_piece *pp, play_piece *np, 
	int col, int row, int rot)
{
    /* naively, there are ((g->w - pp->base->dim) * 4) choices */
    /* determine how to get there ... */
    Wessy_State *ws = (Wessy_State *) state;

    if (rot != ws->desired_rot)
	return MOVE_ROTATE;
    else if (col > ws->desired_column) 
	return MOVE_LEFT;
    else if (col < ws->desired_column) 
	return MOVE_RIGHT;
    else if (ws->know_what_to_do) {
	return MOVE_DOWN;
    } else 
	return MOVE_NONE;
}

/*****************************************************************/
/*************** Impermeable AI barrier **************************/
/*****************************************************************/

/*********** Kiri's globals ***************/
/*#define DEBUG */

typedef struct Aliz_State_struct {
  double bestEval;
  int foundBest;
  int goalColumn, goalRotation;
  int checkColumn, checkRotation;
  int goalSides;
  int checkSides; /* 0, 1, 2 = middle, left, right */
  Grid kg;
} Aliz_State;


/*******************************************************************
 *   evalBoard()
 * Evaluates the 'value' of a given board configuration.
 * Return values range from 0 (in theory) to g->h + <something>.
 * The lowest value is the best.
 * Check separately for garbage?
 *******************************************************************/
static double evalBoard(Grid* g, int nLines, int row)
{
  /* Return the max height plus the number of holes under blocks */
  /* Should encourage smaller heights */
  int x, y, z;
  int maxHeight = 0, minHeight = g->h;
  int nHoles = 0, nGarbage = 0, nCanyons = 0;
  double avgHeight = 0;
  int nColumns = g->w;

  /* Find the minimum, maximum, and average height */
  for (x=0; x<g->w; x++) {
    int height = g->h;
    char gc;
    for (y=0; y<g->h; y++) {
      gc = GRID_CONTENT(*g, x, y);
      if (gc == 0) height--;
      else {
	/* garbage */
	if (gc == 1) nGarbage++;
	/* Penalize for holes under blocks */
	for (z=y+1; z<g->h; z++) {
	  char gc2 = GRID_CONTENT(*g, x, z);
	  /* count the holes under here */
	  if (gc2 == 0) {
	    nHoles += 2; /* these count for double! */
	  }
	  else if (gc2 == 1) nGarbage++;
	}
	break;
      }
    }
    avgHeight += height;
    if (height > maxHeight) maxHeight = height;
    if (height < minHeight) minHeight = height;
  }
  avgHeight /= nColumns;

  nHoles *= g->h;
  
  /* Find the number of holes lower than the maxHeight */
  for (x=0; x<g->w; x++) {
    for (y=g->h-maxHeight; y<g->h; y++) {
      /* count the canyons under here */
      if (GRID_CONTENT(*g, x, y) == 0&&
	  ((x == 0 || GRID_CONTENT(*g, x-1, y)) &&
	   (x == g->w-1 || GRID_CONTENT(*g, x+1, y))))
	nCanyons += y;
      }
  }

#ifdef DEBUG
  printf("*** %d holes ", nHoles);
#endif
  
  return (double)(maxHeight*(g->h) + avgHeight + (maxHeight - minHeight) +
		  nHoles + nCanyons + nGarbage + (g->h - row)
		  - nLines*nLines);
}

/*******************************************************************
 *   cogitate()
 * Kiri's AI 'thinking' function.  Again, called once 'every so'
 * by event_loop().  
 *******************************************************************/
static void 
alizCogitate(void *state, Grid* g, play_piece* pp, play_piece* np, 
	int col, int row, int rot)
{
  Aliz_State *as = (Aliz_State *)state;
  double eval, evalLeft = -1, evalRight = -1;
  int nLines;

  Assert(as);
    
  if (as->foundBest) return;
  
  memcpy(as->kg.contents, g->contents, (g->w * g->h * sizeof(as->kg.contents[0])));
  memcpy(as->kg.fall, g->fall, (g->w * g->h * sizeof(*as->kg.fall)));

  if (as->bestEval == -1) {
    /* It's our first think! */
    as->checkColumn = col; /* check this column first */
    as->checkRotation = 0; /* check no rotation (it's easy!) */
    as->checkSides = 0; /* middle */
  }
  else {
    /* Try the next thing */
    as->checkRotation = (as->checkRotation+1)%4;
    if (as->checkRotation == 0) {
      /* Try a new column */
      /* Go all the way left first */
      if (as->checkColumn == col || as->checkColumn < col) {
	as->checkColumn--;
	if (!valid_position(pp, as->checkColumn, row,
			    as->checkRotation, &as->kg)) {
	  as->checkColumn = col + 1;
	  if (!valid_position(pp, as->checkColumn, row,
			      as->checkRotation, &as->kg)) {
	    /* skip this one */
	    return;
	  }
	}
      }
      else {
	/* Now go all the way to the right */
	as->checkColumn++;
	if (!valid_position(pp, as->checkColumn, row,
			    as->checkRotation, &as->kg)) {
	  /* skip this one */
	  return;
	}
      }
    } else if (!valid_position(pp, as->checkColumn, row,
			       as->checkRotation, &as->kg)) {
      /* as->checkRotation != 0 */
      if (as->checkColumn >= g->w && as->checkRotation == 3) {
	/* final one */
	as->foundBest = TRUE;
#ifdef DEBUG
	printf("Aliz: Found best! (last checked %d, %d)\n",
	       as->checkColumn, as->checkRotation);
#endif
	return;
      }
      /* Trying a specific rotation; skip this one */
      else return;
    }
  }
#ifdef DEBUG
  printf("Aliz: trying (col %d, rot %d) ", as->checkColumn, as->checkRotation);
#endif

  /* Check for impending doom */
  if (GRID_CONTENT(*g, col, row+1)) {
#ifdef DEBUG
    printf("Aliz: panic! about to crash, ");
#endif
   
    /* something right below us, so move the direction with more space */
    if (col > g->w - col) {
      /* go left, leave rotation as is */
      as->goalColumn = col/2;
      /* wessy commentary: you might want to set goalRotation to the
       * current rotation so that your next move (the panic flight) will
       * be a motion, not a rotation) ...
       */
      as->goalRotation = as->checkRotation;
#ifdef DEBUG
      printf("moving left.\n");
#endif
    }
    else {
      /* go right, leave rotation as is */
      as->goalColumn = col + (g->w - col)/2;
      as->goalRotation = as->checkRotation;
#ifdef DEBUG
      printf("moving right.\n");
#endif
    }
#ifdef DEBUG
    printf(" NEW goal: col %d, rot %d\n", as->goalColumn, as->goalRotation);
#endif
    return;
  }

  /************** Test the current choice ****************/
  nLines = drop_piece_on_grid(&as->kg, pp, as->checkColumn, row, 
			      as->checkRotation);
  if (nLines != -1) {	/* invalid place to drop something */
    eval = evalBoard(&as->kg, nLines, row);
#ifdef DEBUG
    printf(": eval = %.3f", eval);
#endif
    if (as->bestEval == -1 || eval < as->bestEval) {
      as->bestEval = eval;
      as->goalColumn = as->checkColumn;
      as->goalRotation = as->checkRotation;
#ifdef DEBUG
      printf(" **");
#endif
      
      /* See if we should try to slide left */
      if (as->checkColumn > 0 && !GRID_CONTENT(*g, as->checkColumn-1, row)) {
	int y;
	for (y=row; y>=0; y--)
	  /* worth trying if there's something above it */
	  if (GRID_CONTENT(*g, as->checkColumn-1, y)) break;
	if (y >= 0 && valid_position(pp, as->checkColumn-1, row,
				     as->checkRotation, &as->kg)) {
	  printf("Aliz: Trying to slip left.\n");
	  /* get a fresh copy */
	  memcpy(as->kg.contents, g->contents, (g->w * g->h * sizeof(as->kg.contents[0])));
	  memcpy(as->kg.fall, g->fall, (g->w * g->h * sizeof(*as->kg.fall)));
	  paste_on_board(pp, as->checkColumn-1, row,
			 as->checkRotation, &as->kg);
	  /* nLines is the same */
	  evalLeft = evalBoard(&as->kg, nLines, row);
	}
      }

      /* See if we should try to slide left */
      if (as->checkColumn < g->w-1 && !GRID_CONTENT(*g, as->checkColumn+1, row)) {
	int y;
	for (y=row; y>=0; y--)
	  /* worth trying if there's something above it */
	  if (GRID_CONTENT(*g, as->checkColumn+1, y)) break;
	if (y >= 0 && valid_position(pp, as->checkColumn+1, row,
				     as->checkRotation, &as->kg)) {
	  printf("Aliz: Trying to slip right.\n");
	  /* get a fresh copy */
	  memcpy(as->kg.contents, g->contents, (g->w * g->h * sizeof(as->kg.contents[0])));
	  memcpy(as->kg.fall, g->fall, (g->w * g->h * sizeof(*as->kg.fall)));
	  paste_on_board(pp, as->checkColumn+1, row,
			 as->checkRotation, &as->kg);
	  /* nLines is the same */
	  evalRight = evalBoard(&as->kg, nLines, row);
	}
      }

      if (evalLeft >= 0 && evalLeft < as->bestEval && evalLeft <= evalRight) {
	as->goalColumn = as->checkColumn;
	as->goalSides = -1;
	printf(": eval = %.3f left **", eval);
      } else if (evalRight >= 0 && evalRight < as->bestEval && evalRight < evalLeft) {
	as->goalColumn = as->checkColumn;
	as->goalSides = 1;
	printf(": eval = %.3f right **", eval);
      }
      else {
#ifdef DEBUG
	printf(": eval = %.3f", eval);
#endif
	if (as->bestEval == -1 || eval < as->bestEval) {
	  as->bestEval = eval;
	  as->goalColumn = as->checkColumn;
	  as->goalRotation = as->checkRotation;
	  as->goalSides = as->checkSides;
	}
      }
    }
#ifdef DEBUG
    printf("\n");
#endif
  }
    
}

/*******************************************************************
 *   alizReset()
 * Clear all of Kiri's globals.
 *********************************************************************/
static void *
alizReset(void *state, Grid *g)
{
    Aliz_State *as;

    if (state == NULL) {
	/* first time we have been called (this game) */
	Calloc(as, Aliz_State *,sizeof(Aliz_State));
    } else
	as = (Aliz_State *)state;
    Assert(as);

#ifdef DEBUG
    printf("Aliz: Clearing state.\n");
#endif
    as->bestEval = -1;
    as->foundBest = FALSE;
    if (as->kg.contents == NULL) as->kg = generate_board(g->w, g->h, 0);
    return as;
}

/*******************************************************************
 *   alizMove()
 * Kiri's AI 'move' function.  Possible retvals:
 * 	MOVE_NONE	Do nothing, fall normally.
 * 	MOVE_LEFT	[Try to] Move one column to the left.
 * 	MOVE_RIGHT	[Try to] Move one column to the right.
 * 	MOVE_ROTATE	[Try to] Rotate the piece.
 * 	MOVE_DOWN	Fall down really quickly.
 *********************************************************************/
static Command 
alizMove(void *state, Grid* g, play_piece* pp, play_piece* np, int col, int row, int rot)
{
    Aliz_State *as = (Aliz_State *)state;
    Assert(as);
  if (rot == as->goalRotation) {
    if (col == as->goalColumn) {
      if (as->foundBest) {
	if (GRID_CONTENT(*g, col, row+1)) {
	  if (as->goalSides == -1) return MOVE_LEFT;
	  else if (as->goalSides == 1) return MOVE_RIGHT;
	}
	return MOVE_DOWN;
      }
      else return MOVE_NONE;
    } else if (col < as->goalColumn) return MOVE_RIGHT;
    else return MOVE_LEFT;
  } else return MOVE_ROTATE;

  
}

/*************************************************************************
 *   AI_Players_Setup()
 * This function creates a structure describing all of the available AI
 * players.
 *******************************************************************PROTO*/
AI_Players *
AI_Players_Setup(void)
{
    int i;
    AI_Players *retval;

    Calloc(retval, AI_Players *, sizeof(AI_Players));

    retval->n = 4;	/* change this to add another */
    Calloc(retval->player, AI_Player *, sizeof(AI_Player) * retval->n);
    i = 0;

    retval->player[i].name 	= "Simple Robot";
    retval->player[i].msg	= "An introductory opponent.";
    retval->player[i].move 	= wes_ai_move;
    retval->player[i].think 	= beginner_ai_think;
    retval->player[i].reset	= wes_ai_reset;
    i++;

    retval->player[i].name 	= "Lightning";
    retval->player[i].msg	= "Warp-speed heuristics.";
    retval->player[i].move 	= wes_ai_move;
    retval->player[i].think 	= wes_ai_think;
    retval->player[i].reset	= wes_ai_reset;
    i++;

    retval->player[i].name	= "Aliz";
    retval->player[i].msg	= "Kiri's codified wit and wisdom.";
    retval->player[i].move 	= alizMove;
    retval->player[i].think 	= alizCogitate;
    retval->player[i].reset	= alizReset;
    i++;

    retval->player[i].name	= "Double-Think";
    retval->player[i].msg	= "Brilliant but slow.";
    retval->player[i].move 	= double_ai_move;
    retval->player[i].think 	= double_ai_think;
    retval->player[i].reset	= double_ai_reset;
    i++;
    
    Debug("AI Players Initialized (%d AIs).\n",retval->n);

    return retval;
}

/***************************************************************************
 *      pick_key_repeat()
 * Ask the player to select a keyboard repeat rate. 
 *********************************************************************PROTO*/
int
pick_key_repeat(SDL_Surface * screen) 
{
    char *factor;
    int retval;

    clear_screen_to_flame();
    draw_string("(1 = Slow Repeat, 16 = Default, 32 = Fastest)",
	    color_purple, screen->w/2,
	    screen->h/2, DRAW_UPDATE | DRAW_CENTER | DRAW_ABOVE);
    draw_string("Keyboard repeat delay factor:", color_purple,
	    screen->w/2, screen->h/2, DRAW_UPDATE | DRAW_LEFT);
    factor = input_string(screen, screen->w/2, screen->h/2, 0);
    retval = 0;
    sscanf(factor,"%d",&retval);
    free(factor);
    if (retval < 1) retval = 1;
    if (retval > 32) retval = 32;
    clear_screen_to_flame();
    return retval;
}

/***************************************************************************
 *      pick_ai_factor()
 * Asks the player to choose an AI delay factor.
 *********************************************************************PROTO*/
int
pick_ai_factor(SDL_Surface * screen) 
{
    char *factor;
    int retval;

    clear_screen_to_flame();
    draw_string("(1 = Impossible, 100 = Easy, 0 = Set Automatically)",
	    color_purple, screen->w/2,
	    screen->h/2, DRAW_UPDATE | DRAW_CENTER | DRAW_ABOVE);
    draw_string("Pick an AI delay factor:", color_purple,
	    screen->w/2, screen->h/2, DRAW_UPDATE | DRAW_LEFT);
    factor = input_string(screen, screen->w/2, screen->h/2, 0);
    retval = 0;
    sscanf(factor,"%d",&retval);
    free(factor);
    if (retval < 0) retval = 0;
    if (retval > 100) retval = 100;
    return retval;
}


/***************************************************************************
 *      pick_an_ai()
 * Asks the player to choose an AI.
 *
 * Returns -1 on "cancel". 
 *********************************************************************PROTO*/
int 
pick_an_ai(SDL_Surface *screen, char *msg, AI_Players *AI)
{
    WalkRadioGroup *wrg;
    int i, text_h;
    int retval;
    SDL_Event event;

    wrg = create_single_wrg( AI->n + 1 );
    for (i=0; i<AI->n ; i++) {
	char buf[1024];
	sprintf(buf,"\"%s\" : %s", AI->player[i].name, AI->player[i].msg);
	wrg->wr[0].label[i] = strdup(buf);
    }

    wrg->wr[0].label[AI->n] = "-- Cancel --";

    wrg->wr[0].defaultchoice = retval = 0;

    if (wrg->wr[0].defaultchoice > AI->n) 
	PANIC("not enough choices!");
    
    setup_radio(&wrg->wr[0]);

    wrg->wr[0].x = (screen->w - wrg->wr[0].area.w) / 2;
    wrg->wr[0].y = (screen->h - wrg->wr[0].area.h) / 2;

    clear_screen_to_flame();

    text_h = draw_string(msg, color_ai_menu, ( screen->w ) / 2,
	    wrg->wr[0].y - 30, DRAW_UPDATE | DRAW_CENTER);

    draw_string("Choose a Computer Player", color_ai_menu, (screen->w ) /
	    2, (wrg->wr[0].y - 30) - text_h, DRAW_UPDATE | DRAW_CENTER);

    draw_radio(&wrg->wr[0], 1);

    while (1) {
	int retval;
	poll_and_flame(&event);

	retval = handle_radio_event(wrg,&event);
	if (retval == -1)
	    continue;
	if (retval == AI->n)
	    return -1;
	return retval;
    }
}


/*
 * $Log: ai.c,v $
 * Revision 1.27  2001/01/05 21:12:31  weimer
 * advance to atris 1.0.5, add support for ".atrisrc" and changing the
 * keyboard repeat rate
 *
 * Revision 1.26  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.25  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.24  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.23  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.22  2000/11/01 04:39:41  weimer
 * clear the little scoring spot correctly, updates for making a no-sound
 * distribution
 *
 * Revision 1.21  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.20  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.19  2000/10/29 19:04:32  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.18  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.17  2000/10/28 21:30:34  wkiri
 * Kiri AI rules!
 *
 * Revision 1.16  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.15  2000/10/20 01:32:09  weimer
 * Minor play issue problems -- time is now truly a hard limit!
 *
 * Revision 1.14  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.13  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.12  2000/10/14 16:17:41  weimer
 * level adjustment changes, added some new AIs, etc.
 *
 * Revision 1.11  2000/10/14 03:36:22  weimer
 * wessy AI changes: resource management
 *
 * Revision 1.10  2000/10/14 02:56:37  wkiri
 * Kiri AI gets another leg up.
 *
 * Revision 1.9  2000/10/14 01:56:35  weimer
 * remove dates from identity files
 *
 * Revision 1.8  2000/10/14 01:56:10  wkiri
 * Kiri AI comes of age!
 *
 * Revision 1.7  2000/10/14 01:24:34  weimer
 * fixed error with not advancing levels when fighting AI
 *
 * Revision 1.6  2000/10/13 22:34:26  weimer
 * minor wessy AI changes
 *
 * Revision 1.5  2000/10/13 21:16:43  wkiri
 * Aliz AI evolves!
 *
 * Revision 1.4  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.3  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.2  2000/10/13 03:02:05  wkiri
 * Added Aliz (Kiri AI) to ai.c.
 * Note event.c currently calls Aliz, not Wessy AI.
 *
 * Revision 1.1  2000/10/12 19:19:43  weimer
 * Sigh!
 *
 */
