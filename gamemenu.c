/*
 *                               Alizarin Tetris
 * The game menu file. 
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <unistd.h>
#include <sys/types.h>

/* header files */
#include "atris.h"
#include "ai.h"
#include "display.h"
#include "grid.h"
#include "piece.h"
#include "button.h"
#include "sound.h"
#include "menu.h"
#include "options.h"

/* function prototypes */
#include ".protos/ai.pro"
#include ".protos/event.pro"
#include ".protos/piece.pro"
#include ".protos/button.pro"
#include ".protos/menu.pro"
#include ".protos/display.pro"

typedef enum {
    ColorStyleMenu = 0,
    SoundStyleMenu = 1,
    PieceStyleMenu = 2,
    GameMenu	   = 3,
    OptionsMenu	   = 4,
    MainMenu       = 5,
} MainMenuChoice;

typedef enum {
    Opt_ToggleFullScreen,
    Opt_ToggleFlame,
    Opt_ToggleSpecial,
    Opt_FasterLevels,
    Opt_LongSettleDelay,
    Opt_UpwardRotation,
    Opt_KeyRepeat,
} OptionMenuChoice;

#define MAX_MENU_CHOICE	6

static int start_playing = 0;
static sound_styles *_ss;
static color_styles *_cs;
static piece_styles *_ps;
static WalkRadioGroup *wrg = NULL;

static GT _local_gametype;

/***************************************************************************/
/* Update the given choice on the main menu */
static void updateMenu(int whichSubMenu, int choice)
{
  wrg->wr[MainMenu].label[whichSubMenu] = wrg->wr[whichSubMenu].label[choice];
  clear_radio(&wrg->wr[MainMenu]);
  setup_radio(&wrg->wr[MainMenu]);
  wrg->wr[MainMenu].x = (screen->w/3 - wrg->wr[MainMenu].area.w)/2;
  wrg->wr[MainMenu].y = (screen->h - wrg->wr[MainMenu].area.h)/2;
}

char key_repeat_label[] = "Key Repeat: 16 " ;

static void OptionsMenu_setup()
{
    wrg->wr[OptionsMenu].label[Opt_ToggleFullScreen] = "Toggle Full Screen";
    if (Options.flame_wanted)
	wrg->wr[OptionsMenu].label[Opt_ToggleFlame] = "Background Flame: On";
    else
	wrg->wr[OptionsMenu].label[Opt_ToggleFlame] = "Background Flame: Off";
    if (Options.special_wanted)
	wrg->wr[OptionsMenu].label[Opt_ToggleSpecial] = "Power Pieces: On";
    else
	wrg->wr[OptionsMenu].label[Opt_ToggleSpecial] = "Power Pieces: Off";
    if (Options.faster_levels)
	wrg->wr[OptionsMenu].label[Opt_FasterLevels] = "Double Difficulty: On";
    else
	wrg->wr[OptionsMenu].label[Opt_FasterLevels] = "Double Difficulty: Off";
    if (Options.long_settle_delay)
	wrg->wr[OptionsMenu].label[Opt_LongSettleDelay] = "Long Settle Delay: On";
    else
	wrg->wr[OptionsMenu].label[Opt_LongSettleDelay] = "Long Settle Delay: Off";
    if (Options.upward_rotation)
	wrg->wr[OptionsMenu].label[Opt_UpwardRotation] = "Upward Rotation: On";
    else
	wrg->wr[OptionsMenu].label[Opt_UpwardRotation] = "Upward Rotation: Off";

    sprintf(key_repeat_label,"Key Repeat: %.2d", Options.key_repeat_delay);
    wrg->wr[OptionsMenu].label[Opt_KeyRepeat] = key_repeat_label;
}

static int OptionsMenu_action(WalkRadio *wr)
{
    switch (wr->defaultchoice) {
	case Opt_ToggleFullScreen:
	    SDL_WM_ToggleFullScreen(screen);
	    break;
	case Opt_ToggleFlame: Options.flame_wanted = ! Options.flame_wanted; break;
	case Opt_ToggleSpecial: Options.special_wanted = ! Options.special_wanted; break;
	case Opt_FasterLevels: Options.faster_levels = ! Options.faster_levels; break;
	case Opt_LongSettleDelay: Options.long_settle_delay = ! Options.long_settle_delay; break;
	case Opt_UpwardRotation: Options.upward_rotation = ! Options.upward_rotation; break;
	case Opt_KeyRepeat: Options.key_repeat_delay = pick_key_repeat(screen); break;
	default: 
	    break;
    }
    OptionsMenu_setup();
    clear_radio(&wrg->wr[OptionsMenu]);
    setup_radio(&wrg->wr[OptionsMenu]);
    return 0;
}

static int ColorStyleMenu_action(WalkRadio *wr)
{
    _cs->choice = wr->defaultchoice;
    /* Update this choice on the main menu */
    updateMenu((int)ColorStyleMenu, wr->defaultchoice);
    return 1;
}
static int SoundStyleMenu_action(WalkRadio *wr) 
{
    _ss->choice = wr->defaultchoice;
    play_all_sounds(_ss->style[_ss->choice]);
    /* Update this choice on the main menu */
    updateMenu((int)SoundStyleMenu, wr->defaultchoice);
    return 1;	
}
static int PieceStyleMenu_action(WalkRadio *wr)
{
    _ps->choice = wr->defaultchoice;
    /* Update this choice on the main menu */
    updateMenu((int)PieceStyleMenu, wr->defaultchoice);
    return 1;
}
static int GameMenu_action(WalkRadio *wr)
{
    _local_gametype = wr->defaultchoice;
    /*    start_playing = 1;*/
    /* Update this choice on the main menu */
    updateMenu((int)GameMenu, wr->defaultchoice);
    return 1;
}

static int MainMenu_action(WalkRadio *wr)
{
  int i;
  /* If they selected a submenu, activate it */
  switch (wr->defaultchoice) {
      case ColorStyleMenu:
      case SoundStyleMenu:
      case PieceStyleMenu:
      case GameMenu:
      case OptionsMenu: 
	  /* Make sure everything is inactive (not including the main menu) */ 
	  for (i=0; i<MAX_MENU_CHOICE-1; i++)
	      if (wrg->wr[i].inactive == FALSE) {
		  wrg->wr[i].inactive = TRUE;
		  clear_radio(&wrg->wr[i]);
	      }
	  /* now activate the proper submenu */
	  wrg->wr[wr->defaultchoice].inactive = FALSE;
	  wrg->cur = wr->defaultchoice;
	  break;
      case 5: /* they chose go! */
	  start_playing = 1;
	  gametype = _local_gametype;
	  break;
      case 6: /* they chose quit */
	  start_playing = 1;
	  gametype = QUIT;
	  break;
      default:
	  Debug("Invalid menu choice %d.\n", wr->defaultchoice);
  }
  return 1;
}

/***************************************************************************
 *      menu_handler()
 * Play the MENU-style game. 
 ***************************************************************************/
static int
menu_handler(const SDL_Event *event) 
{
    int retval;
    if (event->type == SDL_KEYDOWN) {
	switch (event->key.keysym.sym) {
	    case SDLK_q: 
		start_playing = 1;
		_local_gametype = gametype = QUIT;
		return 1;

	    case SDLK_RETURN:
                if ((event->key.keysym.mod & KMOD_LCTRL) ||
                    (event->key.keysym.mod & KMOD_RCTRL)) {
                  SDL_WM_ToggleFullScreen(screen);
                  return 1;
                }

	    default:
		break;
	}
    }
    retval = handle_radio_event(wrg, event);

    if (retval != -1)
	return 1;
    return 0;
}

/***************************************************************************
 *      play_MENU()
 * Play the MENU-style game. 
 ***************************************************************************/
static int
play_MENU(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[], int my_level, AI_Player *aip)
{
    int curtimeleft;
    int match;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    int result;
    color_style *event_cs[2];	/* pass these to event_loop */
    sound_style *event_ss[2];
    AI_Player *event_ai[2];
    extern int Score[];

    level[0] = my_level;	/* starting level */
    Score[0] = Score[1] = 0;
    SeedRandom(0);

    while (1) {	
	/* until they give up by pressing 'q'! */
	/* 5 total minutes for three matches */
	curtimeleft = 300;
	my_adj[0] = 0; my_adj[1] = 1; my_adj[2] = 2;

	for (match=0; match<3 && curtimeleft > 0; match++) {
	    /* generate the board */
	    /* draw the background */
	    g[0] = generate_board(10,20,level[0]);
	    draw_background(screen, cs.style[0]->w, g, level, my_adj, NULL,
		    &(aip->name));
		{
		    int i;
		    for (i=0; i<MAX_MENU_CHOICE; i++) 
			draw_radio(&wrg->wr[i], i == wrg->cur);
		}
	    /* get the result, but don't update level yet */

	    event_cs[0] = event_cs[1] = cs.style[cs.choice];
	    event_ss[0] = event_ss[1] = ss.style[ss.choice];
	    event_ai[0] = aip;
	    event_ai[1] = NULL;

	    result = event_loop(screen, ps.style[ps.choice], 
		    event_cs, event_ss, g,
		    level, 0, &curtimeleft, 1, adjustment,
		    menu_handler, time(NULL), AI_PLAYER, NO_PLAYER, event_ai);
	    if (result < 0) { 	/* explicit quit */
		return -1;
	    }
	    /* reset board */
	    my_adj[match] = adjustment[0];
	}
    }
}


/***************************************************************************
 *      choose_gametype()
 * User selects the desired game type, which is returned. Uses walking
 * radio menus.
 *********************************************************************PROTO*/
int choose_gametype(piece_styles *ps, color_styles *cs,
	sound_styles *ss, AI_Players *ai)
{ /* walk-radio menu */
    int i;

    _local_gametype = gametype;

    if (!wrg) {
	Calloc(wrg , WalkRadioGroup * , sizeof(*wrg));
	wrg->n = MAX_MENU_CHOICE;
	Calloc(wrg->wr, WalkRadio *, wrg->n * sizeof(*wrg->wr));
	wrg->cur = MainMenu;

	wrg->wr[PieceStyleMenu].n = ps->num_style;
	Malloc(wrg->wr[PieceStyleMenu].label, char**, sizeof(char*)*ps->num_style);
	for (i=0; i<ps->num_style; i++)
	    wrg->wr[PieceStyleMenu].label[i] = ps->style[i]->name;
	wrg->wr[PieceStyleMenu].defaultchoice = ps->choice;
	wrg->wr[PieceStyleMenu].action = PieceStyleMenu_action;

	wrg->wr[ColorStyleMenu].n = cs->num_style;
	Malloc(wrg->wr[ColorStyleMenu].label, char**, sizeof(char*)*cs->num_style);
	for (i=0; i<cs->num_style; i++)
	    wrg->wr[ColorStyleMenu].label[i] = cs->style[i]->name;
	wrg->wr[ColorStyleMenu].defaultchoice = cs->choice;
	wrg->wr[ColorStyleMenu].action = ColorStyleMenu_action;

	wrg->wr[SoundStyleMenu].n = ss->num_style;
	Malloc(wrg->wr[SoundStyleMenu].label, char**, sizeof(char*)*ss->num_style);
	for (i=0; i<ss->num_style; i++)
	    wrg->wr[SoundStyleMenu].label[i] = ss->style[i]->name;
	wrg->wr[SoundStyleMenu].defaultchoice = ss->choice;
	wrg->wr[SoundStyleMenu].action = SoundStyleMenu_action;

	_ss = ss;
	_cs = cs;
	_ps = ps;

	wrg->wr[GameMenu].n = 6;
	Malloc(wrg->wr[GameMenu].label, char**, sizeof(char*)*wrg->wr[GameMenu].n);
	wrg->wr[GameMenu].label[0] = "Solo Normal Game";
	wrg->wr[GameMenu].label[1] = "Solo Scoring Marathon";
	wrg->wr[GameMenu].label[2] = "Solo vs. Computer";
	wrg->wr[GameMenu].label[3] = "2 Players @ 1 Keyboard";
	wrg->wr[GameMenu].label[4] = "2 Players, Use Network";
	wrg->wr[GameMenu].label[5] = "Computer vs. Computer";
	wrg->wr[GameMenu].defaultchoice = 0;
	wrg->wr[GameMenu].action = GameMenu_action;

	wrg->wr[OptionsMenu].n = 7;
	Malloc(wrg->wr[OptionsMenu].label, char**, sizeof(char*)*wrg->wr[OptionsMenu].n);
	OptionsMenu_setup();
	wrg->wr[OptionsMenu].defaultchoice = 0;
	wrg->wr[OptionsMenu].action = OptionsMenu_action;

	/* Make the main menu */
	wrg->wr[MainMenu].n = 7;
	Malloc(wrg->wr[MainMenu].label, char**, sizeof(char*)*wrg->wr[MainMenu].n);
	/* Main menu displays the current value for each choice */
	wrg->wr[MainMenu].label[0] = wrg->wr[ColorStyleMenu].label[wrg->wr[ColorStyleMenu].defaultchoice];
	wrg->wr[MainMenu].label[1] = wrg->wr[SoundStyleMenu].label[wrg->wr[SoundStyleMenu].defaultchoice];
	wrg->wr[MainMenu].label[2] = wrg->wr[PieceStyleMenu].label[wrg->wr[PieceStyleMenu].defaultchoice];
	wrg->wr[MainMenu].label[3] = wrg->wr[GameMenu].label[(int)_local_gametype];
	wrg->wr[MainMenu].label[4] = "Special Options";
	wrg->wr[MainMenu].label[5] = "Play!";
	wrg->wr[MainMenu].label[6] = "Quit";
	wrg->wr[MainMenu].defaultchoice = 5;
	wrg->wr[MainMenu].action = MainMenu_action;

	for (i=0; i<MAX_MENU_CHOICE; i++) {
	    setup_radio(&wrg->wr[i]);
	}

	wrg->wr[ColorStyleMenu].x = 2*(screen->w/3) +
	    (screen->w/3 - wrg->wr[ColorStyleMenu].area.w)/2;
	wrg->wr[ColorStyleMenu].y =
	    (screen->h - wrg->wr[ColorStyleMenu].area.h)/2;
	wrg->wr[ColorStyleMenu].inactive = TRUE;

	wrg->wr[SoundStyleMenu].x = 2*(screen->w/3) +
	    (screen->w/3 - wrg->wr[SoundStyleMenu].area.w)/2;
	wrg->wr[SoundStyleMenu].y =
	    (screen->h - wrg->wr[SoundStyleMenu].area.h)/2;
	wrg->wr[SoundStyleMenu].inactive = TRUE;

	wrg->wr[OptionsMenu].x = 2*(screen->w/3) +
	    (screen->w/3 - wrg->wr[OptionsMenu].area.w)/2;
	wrg->wr[OptionsMenu].y =
	    (screen->h - wrg->wr[OptionsMenu].area.h)/2;
	wrg->wr[OptionsMenu].inactive = TRUE;

	wrg->wr[PieceStyleMenu].x = 2*(screen->w/3) +
	    (screen->w/3 - wrg->wr[PieceStyleMenu].area.w)/2;
	wrg->wr[PieceStyleMenu].y =
	    (screen->h - wrg->wr[PieceStyleMenu].area.h)/2;
	wrg->wr[PieceStyleMenu].inactive = TRUE;

	wrg->wr[GameMenu].x = 2*(screen->w/3) +
	    (screen->w/3 - wrg->wr[GameMenu].area.w)/2;
	wrg->wr[GameMenu].y =
	    (screen->h - wrg->wr[GameMenu].area.h)/2;
	wrg->wr[GameMenu].inactive = TRUE;

	wrg->wr[MainMenu].x = (screen->w/3 - wrg->wr[MainMenu].area.w)/2;
	wrg->wr[MainMenu].y = (screen->h - wrg->wr[MainMenu].area.h)/2;
	wrg->wr[MainMenu].inactive = FALSE;
    }

    clear_screen_to_flame();
    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY*10,
	    SDL_DEFAULT_REPEAT_INTERVAL*10);

    start_playing = 0;
    while (1) {
	Grid g[2];
	gametype = DEMO;
	play_MENU(*cs, *ps, *ss, g, Options.faster_levels ? 5 : 10, 
		&ai->player[ZEROTO(ai->n)]);
	if (start_playing) {
	    return 0;
	}
    }
}

/*
 * $Log: gamemenu.c,v $
 * Revision 1.30  2001/01/06 00:25:23  weimer
 * changes so that we remember things ...
 *
 * Revision 1.29  2001/01/05 21:38:01  weimer
 * more key changes
 *
 * Revision 1.28  2001/01/05 21:12:32  weimer
 * advance to atris 1.0.5, add support for ".atrisrc" and changing the
 * keyboard repeat rate
 *
 * Revision 1.27  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.26  2000/11/06 04:16:10  weimer
 * "power pieces" plus images
 *
 * Revision 1.25  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.24  2000/11/06 01:24:43  wkiri
 * Removed 'quit' from the game type menu.
 *
 * Revision 1.23  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.22  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.21  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.20  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.19  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.18  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.17  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.16  2000/10/28 21:52:56  weimer
 * you can now press 'Q' to quit from the main menu!
 *
 * Revision 1.15  2000/10/28 16:40:17  weimer
 * Further changes: we can now build .tar.gz files and RPMs!
 *
 * Revision 1.14  2000/10/22 22:05:51  weimer
 * Added a few new sounds ...
 *
 * Revision 1.13  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.12  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.11  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.10  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.9  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.8  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.7  2000/10/12 01:38:07  weimer
 * added initial support for persistent player identities
 *
 * Revision 1.6  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.5  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.4  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.3  2000/09/04 22:49:51  wkiri
 * gamemenu now uses the new nifty menus.  Also, added delete_menu.
 *
 * Revision 1.2  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.1  2000/09/03 19:41:30  wkiri
 * Now allows you to choose the game type (though it doesn't do anything yet).
 *
 */
