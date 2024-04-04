/*
 *                               Alizarin Tetris
 * The user identity file.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <ctype.h>

#include "atris.h"
#include "display.h"
#include "grid.h"
#include "identity.h"
#include "menu.h"

#include ".protos/xflame.pro"
#include ".protos/display.pro"

#define ID_FILENAME	"Atris.Players"

/***************************************************************************
 *      input_string()
 * Read input from the user ... on the widget layer. 
 *********************************************************************PROTO*/
char *
input_string(SDL_Surface *screen, int x, int y, int opaque)
{
    int pos;
    char c;
    char retval[1024];
    SDL_Surface *text, *ctext;
    SDL_Color tc, cc;
    SDL_Rect rect;
    SDL_Event event;
    Uint32 text_color = int_input_color0;
    Uint32 cursor_color = int_input_color1;
    Uint32 our_black = opaque ? int_solid_black : int_black;

    memset(retval, 0, sizeof(retval));
    retval[0] = ' ';
    pos = 1;

    SDL_GetRGB(text_color, screen->format, &tc.r, &tc.g, &tc.b);
    SDL_GetRGB(cursor_color, screen->format, &cc.r, &cc.g, &cc.b);

    ctext = TTF_RenderText_Blended(font, "_", cc); Assert(ctext);

    while (1) {
	int changed = 0;	/* did they change the text string? */
	int blink = 1;		/* toggle the blinking the cursor */
	Uint32  flip_when = SDL_GetTicks();
	/* display the current string */

	text = TTF_RenderText_Blended(font, retval, tc); Assert(text);

	rect.x = x;
	rect.y = y;
	rect.w = text->w;
	rect.h = text->h;

	SDL_BlitSurface(text, NULL, widget_layer, &rect);
	/* OK to ignore the intervening flame layer */
	SDL_BlitSurface(text, NULL, screen, &rect);
	SDL_UpdateSafe(screen, 1, &rect);

	rect.x += rect.w;
	rect.w = ctext->w;
	rect.h = ctext->h;

	changed = 0;
	while (!changed) { 
	    if (SDL_GetTicks() > flip_when) {
		if (blink) {
		    SDL_BlitSurface(ctext, NULL, screen, &rect);
		    SDL_BlitSurface(ctext, NULL, widget_layer, &rect);
		} else {
		    SDL_FillRect(widget_layer, &rect, our_black);
		    SDL_FillRect(screen, &rect, our_black);
		    SDL_BlitSurface(flame_layer, &rect, screen, &rect);
		    SDL_BlitSurface(widget_layer, &rect, screen, &rect);
		}
		SDL_UpdateSafe(screen, 1, &rect);
		flip_when = SDL_GetTicks() + 400;
		blink = !blink;
	    }
	    if (SDL_PollEvent(&event)) {
		if (event.type == SDL_KEYDOWN) {
		    changed = 1;
		    switch (event.key.keysym.sym) {
			case SDLK_RETURN:
			    return strdup(retval + 1);

			case SDLK_BACKSPACE:
			    if (pos > 1) pos--;
			    retval[pos] = 0;

			    rect.x = x;
			    rect.w = text->w + ctext->w;
			    SDL_FillRect(widget_layer, &rect, our_black);
			    SDL_FillRect(screen, &rect, our_black);
			    SDL_BlitSurface(flame_layer, &rect, screen, &rect);
			    SDL_BlitSurface(widget_layer, &rect, screen, &rect);
			    SDL_UpdateSafe(screen, 1, &rect);
			    break;

			default:
			    c = event.key.keysym.unicode;
			    if (c == 0) break;

			    SDL_FillRect(widget_layer, &rect, our_black);
			    SDL_FillRect(screen, &rect, our_black);
			    SDL_BlitSurface(flame_layer, &rect, screen, &rect);
			    SDL_BlitSurface(widget_layer, &rect, screen, &rect);
			    SDL_UpdateSafe(screen, 1, &rect);

			    if (isalpha(c) || isdigit(c) || isspace(c) || ispunct(c))
				retval[pos++] = c;
			    break;
		    }
		}
	    } else atris_run_flame();
	}
	SDL_FreeSurface(text);
    }
}

/***************************************************************************
 *      load_identity_file()
 * Parse an identity file.
 *********************************************************************PROTO*/
identity *
load_identity_file()
{
    identity *retval;
    char buf[2048];

    FILE *fin = fopen(ID_FILENAME, "rt");
    int count = 0;
    int i;

    if (!fin) {
	Debug("fopen(%s)\n", ID_FILENAME);
	Debug("Cannot open Identity File.\n");

	Calloc(retval,identity *,sizeof(identity));
	return retval;
    }
    Calloc(retval,identity *,sizeof(identity));

    while (!(feof(fin))) {
	do {
	    fgets(buf,sizeof(buf),fin);
	} while (!feof(fin) &&
		(buf[0] == '\n' || buf[0] == '#'));
	if (feof(fin)) break;
	if (strchr(buf,'\n'))
	    *(strchr(buf,'\n')) = 0;
	count++;
    }
    rewind(fin);

    retval->n = count;

    if (!count) {
	/* do nothing */
    } else {
	Calloc(retval->p,person *,sizeof(person)*count);
	i = 0;
	while (!(feof(fin))) {
	    char *p;
	    do {
		fgets(buf,sizeof(buf),fin);
	    } while (!feof(fin) &&
		    (buf[0] == '\n' || buf[0] == '#'));
	    if (feof(fin)) break;
	    if (strchr(buf,'\n'))
		*(strchr(buf,'\n')) = 0;

	    sscanf(buf,"%d",&retval->p[i].level);
	    p = strchr(buf,' ');
	    if (!p) {
		retval->p[i].name = "-garbled-";
	    } else {
		retval->p[i].name = strdup( p + 1 );
	    }
	    i++;
	}
    }
    fclose(fin);

    Debug("Identity File [%s] loaded (%d players).\n",ID_FILENAME, count);

    return retval;
}

/***************************************************************************
 *      save_identity_file()
 * Saves the information to an identity file.
 *********************************************************************PROTO*/
void 
save_identity_file(identity *id, char *new_name, int new_level)
{
    FILE *fin = fopen(ID_FILENAME,"wt");
    int i;
    if (!fin) {
	Debug("fopen(%s): cannot write Identity File.\n",ID_FILENAME);
	return;
    }
    fprintf(fin,"# Alizarin Tetris Identity File\n"
	        "#\n"
		"# Format:\n"
		"#[level] [name] (no space before level, one space after)\n"
		);
    for (i=0; i<id->n ;i++) {
	fprintf(fin,"%d %s\n",id->p[i].level, id->p[i].name);
    }
    if (new_name)  {
	fprintf(fin,"%d %s\n",new_level, new_name);
    }

#ifdef DEBUG
    Debug("Identity File [%s] saved (%d players).\n",ID_FILENAME, id->n +
	    (new_name != NULL));
#endif
    fclose(fin);
    return;
}

/***************************************************************************
 *      network_choice_action()
 * What to do when they click on the network choice button ...
 ***************************************************************************/
static int
network_choice_action(WalkRadio *wr) 
{
    if (wr->defaultchoice == 0) {
	/* I am client */
	clear_screen_to_flame();
	draw_string("Who is the Server?", color_network_menu, screen->w/2,
		screen->h/2, DRAW_LEFT | DRAW_UPDATE);
	wr->data = input_string(screen, screen->w/2, screen->h/2, 0);
    } else if (wr->data) {
	Free(wr->data);
    } else
	wr->data = NULL;
    return 1;
}

/***************************************************************************
 *      network_choice()
 * Do you want to be the client or the server? Returns the hostname.
 *********************************************************************PROTO*/
char *
network_choice(SDL_Surface *screen)
{
    static WalkRadioGroup *wrg = NULL;
    SDL_Event event;

    if (!wrg) {
	wrg = create_single_wrg( 2 ) ; 
	wrg->wr[0].label[0] = "I am the Client. I will specify a server.";
	wrg->wr[0].label[1] = "I will be the Server.";

	setup_radio(&wrg->wr[0]);
	wrg->wr[0].x = (screen->w - wrg->wr[0].area.w) / 2;
	wrg->wr[0].y = (screen->h - wrg->wr[0].area.h) / 2;
	wrg->wr[0].action = network_choice_action;
    }

    clear_screen_to_flame();

    draw_string("Who is the Server?", color_network_menu, 
	    screen->w/2, wrg->wr[0].y, DRAW_CENTER | DRAW_UPDATE | DRAW_ABOVE);

    draw_radio(&wrg->wr[0], 1);

    while (1) {
	int retval;
	poll_and_flame(&event);
	retval = handle_radio_event(wrg, &event);
	if (retval != -1)
	    return wrg->wr[0].data;
    }
}

/***************************************************************************
 *	new_player()
 * Add another player ... Displays a little dialog so that the new player
 * can enter a new name and a new level. 
 **************************************************************************/
static
void new_player(SDL_Surface * screen, identity **id)
{
    char *new_name, *new_level;
    int level;
    clear_screen_to_flame();

    draw_string("Enter your name:", color_who_are_you, screen->w / 2,
	    screen->h / 2, DRAW_CLEAR | DRAW_LEFT);

    new_name = input_string(screen, screen->w/2, screen->h/2, 0);

    if (strlen(new_name)) {
	clear_screen_to_flame();
	draw_string("Welcome ", color_purple, screen->w/2,
		screen->h/2, DRAW_CLEAR | DRAW_LEFT | DRAW_LARGE | DRAW_ABOVE);
	draw_string(new_name, color_purple, screen->w/2,
		screen->h/2, DRAW_CLEAR | DRAW_LARGE | DRAW_ABOVE);
	draw_string("Starting level (2-10):", color_who_are_you, screen->w
		/ 2, screen->h / 2, DRAW_CLEAR | DRAW_LEFT);
	new_level = input_string(screen, screen->w/2, screen->h/2, 0);
	level = 0;
	sscanf(new_level,"%d",&level);
	if (level < 2) level = 2;
	if (level > 10) level = 10;

	save_identity_file(*id, new_name, level);

	(*id) = load_identity_file();

	free(new_level);
    }
    free(new_name);
}


/***************************************************************************
 *      who_are_you()
 * Asks the player to choose an identity ...
 * Returns -1 on "cancel". 
 *********************************************************************PROTO*/
int 
who_are_you(SDL_Surface *screen, identity **id, int taken, int p)
{
    WalkRadioGroup *wrg = NULL;
    int i;
    int retval;
    SDL_Event event;
    char buf[1024];

restart: 	/* sigh! */
    wrg = create_single_wrg((*id)->n + 2);
    for (i=0; i<(*id)->n; i++) {
	if (i == taken)
	    sprintf(buf,"%s (already taken!)",(*id)->p[i].name);
	else
	    sprintf(buf,"%s (Level %d)",(*id)->p[i].name, (*id)->p[i].level);
	wrg->wr[0].label[i] = strdup(buf);
    }
    wrg->wr[0].label[(*id)->n] = "-- New Player --";
    wrg->wr[0].label[(*id)->n+1] = "-- Cancel --";

    setup_radio(&wrg->wr[0]);
    wrg->wr[0].x = (screen->w - wrg->wr[0].area.w) / 2;
    wrg->wr[0].y = (screen->h - wrg->wr[0].area.h) / 2;
    wrg->wr[0].action = NULL /* return default choice */;

    clear_screen_to_flame();

    if (p == 1)  {
	draw_string("Left = A  Rotate = W  Right = D  Drop = S", color_keys_explain,
		screen->w / 2, 0, DRAW_CENTER | DRAW_UPDATE);
	draw_string("Player 1: Who Are You?", color_who_are_you,
		screen->w / 2, wrg->wr[0].y - 30, DRAW_CENTER | DRAW_UPDATE);
    } else {
	draw_string("Use the Arrow keys. Rotate = Up  Drop = Down", color_keys_explain,
		screen->w / 2, 0, DRAW_CENTER | DRAW_UPDATE);
	draw_string("Player 2: Who Are You?", color_who_are_you,
		screen->w / 2, wrg->wr[0].y - 30, DRAW_CENTER | DRAW_UPDATE);
    }

    draw_radio(&wrg->wr[0], 1);

    while (1) {
	poll_and_flame(&event);

	retval = handle_radio_event(wrg, &event);
	if (retval == -1 || retval == taken)
	    continue;
	if (retval == (*id)->n) {
	    new_player(screen, id);
	    goto restart;
	}
	if (retval == (*id)->n+1)
	    return -1;
	return retval;
    }
    return 0;
}

/*
 * $Log: identity.c,v $
 * Revision 1.16  2000/11/06 04:44:15  weimer
 * fixed that who-is-the-server thing
 *
 * Revision 1.15  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.14  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.13  2000/10/29 21:28:58  weimer
 * fixed a few failures to clear the screen if we didn't have a flaming
 * backdrop
 *
 * Revision 1.12  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.11  2000/10/29 19:04:33  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.10  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.9  2000/10/28 22:33:18  weimer
 * add a blinking cursor to the input string widget
 *
 * Revision 1.8  2000/10/28 16:40:17  weimer
 * Further changes: we can now build .tar.gz files and RPMs!
 *
 * Revision 1.7  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.6  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.5  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.4  2000/10/14 01:56:35  weimer
 * remove dates from identity files
 *
 * Revision 1.3  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.2  2000/10/13 02:26:54  weimer
 * rudimentary identity functions, including adding new players
 *
 * Revision 1.1  2000/10/12 01:38:07  weimer
 * added initial support for persistent player identities
 *
 */
