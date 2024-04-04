/*
 *                               Alizarin Tetris
 * For the management of high scores.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#define BUFSIZE 256 /* 255 letters for each name */

#define NUM_HIGH_SCORES 10 /* number of scores to save */

#include "config.h"
#include <assert.h>
#include <ctype.h>

#include "atris.h"
#include "button.h"
#include "display.h"
#include "identity.h"
#include "grid.h"
#include "highscore.h"

#include ".protos/display.pro"
#include ".protos/identity.pro"

static char  loaded = FALSE;
static int   high_scores[NUM_HIGH_SCORES];
static char* high_names[NUM_HIGH_SCORES];
static char* high_dates[NUM_HIGH_SCORES];

static SDL_Rect hs, hs_border;	/* where are the high scores? */
static int score_height;	/* how high is each score? */

#define FIRST_SCORE_Y	(hs.y + 60)

/***************************************************************************
 *      prep_hs_bg()
 * Prepare the high score background. 
 ***************************************************************************/
static void 
prep_hs_bg()
{
    hs.x = screen->w/20; hs.y = screen->h/20;
    hs.w = 9*screen->w/10; hs.h = 18*screen->h/20;

    draw_bordered_rect(&hs, &hs_border, 2);
}

/***************************************************************************
 *      save_high_scores()
 * Save the high scores out to the disk. 
 ***************************************************************************/
static void 
save_high_scores()
{
    FILE *fout;
    int i;

    if (!loaded) 
	return;

    fout = fopen("Atris.Scores","wt");
    if (!fout) {
	Debug("Unable to write High Score file [Atris.Scores]: %s\n", strerror(errno));
	return;
    }

    fprintf(fout,"# Alizarin Tetris High Score File\n");
    for (i=0; i<NUM_HIGH_SCORES; i++)
	fprintf(fout,"%04d|%s|%s\n",high_scores[i], high_dates[i], high_names[i]);
    fclose(fout);
}


/***************************************************************************
 *      load_high_scores()
 * Load the high scores from disk (and allocate space).
 ***************************************************************************/
static void 
load_high_scores()
{
    FILE* fin;
    int i = 0;
    char buf[2048];

    if (!loaded) {
	/* make up a dummy high score template first */

	for (i=0; i<NUM_HIGH_SCORES; i++) {
	    high_names[i] = strdup("No one yet..."); Assert(high_names[i]);
	    high_dates[i] = strdup("Never"); Assert(high_dates[i]);
	    high_scores[i] = 0;
	}
	loaded = TRUE;
    }

    fin = fopen("Atris.Scores", "r");
    if (fin) {

	for (i=0; !feof(fin) && i < NUM_HIGH_SCORES; i++) {
	    char *p, *q;
	    /* read in a line of text, but skip comments and blanks */
	    do {
		fgets(buf, sizeof(buf), fin);
	    } while (!feof(fin) && (buf[0] == '\n' || buf[0] == '#'));
	    /* are we done with this file? */
	    if (feof(fin)) break;
	    /* strip the newline */
	    if (strchr(buf,'\n'))
		*(strchr(buf,'\n')) = 0;
	    /* format: "score|date|name" */

	    sscanf(buf,"%d",&high_scores[i]);
	    p = strchr(buf,'|');
	    if (!p) break;
	    p++;
	    q = strchr(p, '|');
	    if (!q) break;
	    Free(high_dates[i]); Free(high_names[i]);
	    *q = 0;
	    high_dates[i] = strdup(p); Assert(high_dates[i]);
	    q++;
	    high_names[i] = strdup(q); Assert(high_names[i]);
	}
	fclose(fin);
    }
}

/***************************************************************************
 *      show_high_scores()
 * Display the current high score list.
 ***************************************************************************/
static void 
show_high_scores()
{
  char buf[256];
  int i, base;
  int delta;
    
  if (!loaded) load_high_scores();
  prep_hs_bg();

  draw_string("Alizarin Tetris High Scores", color_purple, screen->w/2,
	  hs.y, DRAW_LARGE | DRAW_UPDATE | DRAW_CENTER );

  base = FIRST_SCORE_Y;

  for (i=0; i<NUM_HIGH_SCORES; i++)
    {
      sprintf(buf, "%-2d)", i+1);
      delta = draw_string(buf, color_blue, 3*screen->w/40, base, DRAW_UPDATE );

      score_height = delta + 3;

      sprintf(buf, "%-20s", high_names[i]);
      draw_string(buf, color_red, 3*screen->w/20, base, DRAW_UPDATE );

      sprintf(buf, "%.4d", high_scores[i]);
      draw_string(buf, color_blue, 11*screen->w/20, base, DRAW_UPDATE );

      sprintf(buf, "%s", high_dates[i]);
      draw_string(buf, color_red, 7*screen->w/10, base, DRAW_UPDATE );

      base += score_height;
    }
}

/***************************************************************************
 *      is_high_score()
 * Checks whether score qualifies as a high score.
 * Returns 1 if so, 0 if not.
 ***************************************************************************/
static int 
is_high_score(int score)
{
  if (!loaded) load_high_scores();
  return (score >= high_scores[NUM_HIGH_SCORES-1]);
}

/***************************************************************************
 *      update_high_scores()
 * Modifies the high score list, adding in the current score and
 * querying for a name.
 ***************************************************************************/
static void 
update_high_scores(int score)
{
  unsigned int i, j;
  char buf[256];
#if HAVE_STRFTIME
  const struct tm* tm;
  time_t t;
#endif
  
  if (!is_high_score(score)) return;
  if (!loaded) load_high_scores();

  for (i=0; i<NUM_HIGH_SCORES; i++)
    {
      if (score >= high_scores[i])
	{
	  prep_hs_bg();
	  /* move everything down */

	  Free(high_names[NUM_HIGH_SCORES-1]);
	  Free(high_dates[NUM_HIGH_SCORES-1]);

	  for (j=NUM_HIGH_SCORES-1; j>i; j--)
	    {
	      high_scores[j] = high_scores[j-1];
	      high_names[j] = high_names[j-1];
	      high_dates[j] = high_dates[j-1];
	    }
	  /* Get the date */
#if HAVE_STRFTIME
	  t = time(NULL); tm = localtime(&t);
	  strftime(buf, sizeof(buf), "%b %d %H:%M", tm);
	  high_dates[i] = strdup(buf);
#else
#warning  "Since you do not have strftime(), you will not have accurate times in the high score list."
	  high_dates[i] = "Unknown Time";
#endif

	  high_scores[i] = score;
	  /* Display the high scores */
	  high_names[i] = " ";
	  show_high_scores();
	  /* get the new name */

	  draw_string("Enter your name!", color_purple, screen->w / 2, hs.y
		  + hs.h - 10, DRAW_CENTER | DRAW_ABOVE | DRAW_UPDATE);

	  high_names[i] = input_string(screen, 3*screen->w/20,
		  FIRST_SCORE_Y + score_height * i, 1);
	  break;
	}
    }
  save_high_scores();
}

/***************************************************************************
 *      high_score_check()
 * Checks for a high score; if so, updates the list. Displays list.
 *********************************************************************PROTO*/
void high_score_check(int level, int new_score)
{
  SDL_Event event;

  clear_screen_to_flame();

  if (level < 0) {
      return;
  }

  /* Clear the queue */
  while (SDL_PollEvent(&event)) { 
      poll_and_flame(&event);
  }
  
  /* Check for a new high score.  If so, update the list. */
  if (is_high_score(new_score)) {
      update_high_scores(new_score);
      show_high_scores();
  } else { 
      char buf[BUFSIZE];

      show_high_scores();
      sprintf(buf,"Your score: %d", new_score);

      draw_string(buf, color_purple, screen->w / 2, hs.y
	      + hs.h - 10, DRAW_CENTER | DRAW_ABOVE | DRAW_UPDATE);

    }
  /* wait for any key */
  do {
      poll_and_flame(&event);
  } while (event.type != SDL_KEYDOWN);
}

/*
 * $Log: highscore.c,v $
 * Revision 1.31  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.30  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.29  2000/10/29 19:04:33  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.28  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.27  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.26  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.25  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.24  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.23  2000/09/03 19:41:30  wkiri
 * Now allows you to choose the game type (though it doesn't do anything yet).
 *
 * Revision 1.22  2000/09/03 18:44:36  wkiri
 * Cleaned up atris.c (see high_score_check()).
 * Added game type (defaults to MARATHON).
 *
 * Revision 1.21  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.20  2000/09/03 17:56:36  wkiri
 * Reorganization of files (display.[ch], event.c are new).
 *
 * Revision 1.19  2000/08/26 03:11:34  wkiri
 * Buttons now use borders.
 *
 * Revision 1.18  2000/08/26 02:45:28  wkiri
 * Beginnings of button class; also modified atris to query for 'new
 * game' vs. 'quit'.  (New Game doesn't work yet...)
 *
 * Revision 1.17  2000/08/26 00:54:00  wkiri
 * Now high-scoring user enters name at the proper place.
 * Also, score is shown even if user doesn't make the high score list.
 *
 * Revision 1.16  2000/08/20 23:45:58  wkiri
 * High scores reports the hour:minute rather than the year.
 *
 * Revision 1.15  2000/08/20 23:39:00  wkiri
 * Different color for the scores in the score display; positions adjusted.
 *
 * Revision 1.14  2000/08/20 19:00:29  weimer
 * text output changes (moved from _Solid to _Blended)
 *
 * Revision 1.13  2000/08/20 17:16:25  wkiri
 * Can quit using 'q' and this will skip the high score list.
 * Adjusted the high score display positions to fit in the window.
 *
 * Revision 1.12  2000/08/20 16:49:52  wkiri
 * Now supports dates as well.
 *
 * Revision 1.11  2000/08/20 16:20:46  wkiri
 * Now displays the new score correctly.
 *
 * Revision 1.10  2000/08/20 16:16:04  wkiri
 * Better display of high scores.
 *
 * Revision 1.9  2000/08/15 01:58:11  wkiri
 * Not allowed to backspace before the beginning of the array! :)
 * Handles this gracefully (i.e. ignores any such keypresses)
 * and clears the display correctly even when fewer letters are present
 * than were present previously (when BS is pressed).
 *
 * Revision 1.8  2000/08/15 01:30:33  wkiri
 * Handles shift keys (i.e. empty messages) properly.
 *
 * Revision 1.7  2000/08/15 01:22:59  wkiri
 * High scores can handle punctuation, backspace, and spaces in names.
 * Also, display is nicer (got rid of the weird boxes).
 *
 * Revision 1.6  2000/08/15 00:52:33  wkiri
 * Ooops! Now properly checks for new high scores.
 *
 * Revision 1.5  2000/08/15 00:50:49  wkiri
 * Now displays your high score name as you type it in.
 * No support for backspace, though.
 *
 * Revision 1.4  2000/08/15 00:37:01  wkiri
 * Wes found nasty memory bug!  Wes is my hero!
 * Also, changes to high scores to get input from the window.
 *
 * Revision 1.3  2000/08/15 00:22:15  wkiri
 * Now handles non-regular-ascii input to high scores.
 *
 * Revision 1.2  2000/08/14 23:37:06  wkiri
 * Removed debugging output from high score module.
 *
 * Revision 1.1  2000/08/14 01:07:17  wkiri
 * High score files.
 *
 */

