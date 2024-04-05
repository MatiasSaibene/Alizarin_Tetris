/*
 *                               Alizarin Tetris
 * The main file. Library initialization and utility functions.
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */

#include "config.h"	/* go autoconf! */
#include <unistd.h>
#include <sys/types.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
char *error_msg = NULL;
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

/* configure magic for dirent */
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#if HAVE_NETDB_H
#include <fcntl.h>
#endif

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#if HAVE_NETDB_H
#include <netdb.h>
#endif

#include <time.h>

#define ID_FILENAME	"Atris.Players"
#include <SDL/SDL.h>
#include <SDL/SDL_main.h>
#include <SDL/SDL_ttf.h>
#define ATRIS_LIBDIR "$(prefix)/games/atris"
#if HAVE_SYS_SOCKET_H
#	include <sys/socket.h>
#else
#	if HAVE_WINSOCK_H
#		include <winsock.h>
#	endif
#endif

/* header files */
#include "atris.h"
#include "ai.h"
#include "button.h"
#include "display.h"
#include "grid.h"
#include "piece.h"
#include "sound.h"
#include "identity.h"
#include "options.h"


/* function prototypes */
#include ".protos/ai.pro"
#include ".protos/display.pro"
#include ".protos/event.pro"
#include ".protos/gamemenu.pro"
#include ".protos/highscore.pro"
#include ".protos/identity.pro"
#include ".protos/network.pro"
#include ".protos/xflame.pro"

#define BUFSIZE 256 /* 255 letters for each name */

#define NUM_HIGH_SCORES 10 /* number of scores to save */

static color_style *event_cs[2];	/* pass these to event_loop */
static sound_style *event_ss[2];
static AI_Player *event_ai[2];
static char *event_name[2];
extern int Score[2];

/***************************************************************************
 *      Panic()
 * It's over. Don't even try to clean up.
 *********************************************************************PROTO*/
void
Panic(const char *func, const char *file, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);

  printf("\nPANIC in %s() of %s:\n\t",func,file);
#if HAVE_VPRINTF
  vprintf((const char *)fmt, ap);
#else
#warning "Since you lack vprintf(), you will not get informative PANIC messages." 
#endif
  printf("\n\nlibc error %3d| %s\n",errno,strerror(errno));
  printf(    "SDL error     | %s\n",SDL_GetError());
  SDL_CloseAudio();
  exit(1);
}

/***************************************************************************
 *      usage()
 * Display summary usage information.
 ***************************************************************************/
static void
usage(void)
{
    printf("\n\t\t\t\tatris -- Alizarin Tetris\n"
	   "Usage: atris [options] \n"
	   "\t-h --help\t\tThis message.\n"
	   "\t-b --bg     \t\tFlaming background (default).\n"
	   "\t-n --noflame\t\tNo flaming background.\n"
	   "\t-s --sound\t\tEnable sound effects (default).\n"
	   "\t-q --quiet\t\tNo sound effects.\n"
	   "\t-w --window\t\tWindowed display (default).\n"
	   "\t-f --fullscreen\t\tFull-screen display.\n"
	   "\t-d=X --depth=X\t\tSet color detph (bpp) to X.\n"
	   "\t-r=X --repeat=X\t\tSet the keyboard repeat delay to X.\n"
	   "\t\t\t\t(1 = Slow Repeat, 16 = Fast Repeat)\n"
	   );
    exit(1);
}

/***************************************************************************
 *      save_options()
 * Save current settings to a file.
 ***************************************************************************/
void
save_options(char *filespec)
{
    FILE *fout = fopen(filespec, "wt");
    if (!fout) return;
    fprintf(fout,
	    "# bpp = 15, 16, 24, 32 or 0 for auto-detect bits per pixel\n"
	    "bpp = %d\n"
	    "# sound_wanted = 0 or 1\n"
	    "sound_wanted = %d\n"
	    "# full_screen = 0 or 1\n"
	    "full_screen = %d\n"
	    "# flame = 0 or 1 (CPU-sucking background flame)\n"
	    "flame = %d\n"
	    "# key_repeat = 1 to 32 (1 = slow repeat, 16 = default)\n"
	    "key_repeat = %d\n"
	    "# power_pieces = 0 or 1\n"
	    "power_pieces = %d\n"
	    "# double_difficulty = 0 or 1\n"
	    "double_difficulty = %d\n"
	    "# long_settle = 0 or 1\n"
	    "long_settle = %d\n"
	    "# upward_rotation = 0 or 1\n"
	    "upward_rotation = %d\n"
	    "#\n"
	    "color_style = %d\n"
	    "sound_style = %d\n"
	    "piece_style = %d\n"
	    "game_style = %d\n"
	    ,
	    Options.bpp_wanted, Options.sound_wanted, 
	    Options.full_screen, Options.flame_wanted,
	    Options.key_repeat_delay, Options.special_wanted,
	    Options.faster_levels, Options.long_settle_delay,
	    Options.upward_rotation,
	    Options.named_color, Options.named_sound, Options.named_piece,
	    Options.named_game);
    fclose(fout);
    Debug("Preference file [%s] saved.\n",filespec);
    return;
}

/***************************************************************************
 *      load_options()
 * Load options from a user settings file.
 ***************************************************************************/
void
load_options(char * filespec)
{
    FILE *fin = fopen(filespec, "rt");

    Options.full_screen = FALSE;
    Options.sound_wanted = TRUE;
    Options.flame_wanted = TRUE;
    Options.bpp_wanted = 0;
    Options.key_repeat_delay = 16;
    Options.special_wanted = FALSE;
    Options.faster_levels = FALSE;
    Options.upward_rotation = TRUE;
    Options.long_settle_delay = TRUE;
    Options.named_color = -1;
    Options.named_sound = -1;
    Options.named_piece = -1;
    Options.named_game = -1;

    if (!fin) return;

    while (!feof(fin)) {
	char buf[1024];
	char cmd[1024];
	fgets(buf,sizeof(buf),fin);
	if (feof(fin)) break;
	if (buf[0] == '#' || buf[0] == '\n')
	    continue;

	sscanf(buf,"%s",cmd);
	if (!strcasecmp(cmd,"full_screen")) {
	    sscanf(buf,"%s = %d",cmd,&Options.full_screen);
	} else if (!strcasecmp(cmd,"sound_wanted")) {
	    sscanf(buf,"%s = %d",cmd,&Options.sound_wanted);
	} else if (!strcasecmp(cmd,"flame")) {
	    sscanf(buf,"%s = %d",cmd,&Options.flame_wanted);
	} else if (!strcasecmp(cmd,"bpp")) {
	    sscanf(buf,"%s = %d",cmd,&Options.bpp_wanted);
	} else if (!strcasecmp(cmd,"key_repeat")) {
	    sscanf(buf,"%s = %d",cmd,&Options.key_repeat_delay);
	    if (Options.key_repeat_delay < 1) Options.key_repeat_delay = 1;
	    if (Options.key_repeat_delay > 32) Options.key_repeat_delay = 32;
	} else if (!strcasecmp(cmd,"power_pieces")) {
	    sscanf(buf,"%s = %d",cmd,&Options.special_wanted);
	} else if (!strcasecmp(cmd,"double_difficulty")) {
	    sscanf(buf,"%s = %d",cmd,&Options.faster_levels);
	} else if (!strcasecmp(cmd,"long_settle")) {
	    sscanf(buf,"%s = %d",cmd,&Options.long_settle_delay);
	} else if (!strcasecmp(cmd,"upward_rotation")) {
	    sscanf(buf,"%s = %d",cmd,&Options.upward_rotation);
	} else if (!strcasecmp(cmd,"color_style")) {
	    sscanf(buf,"%s = %d",cmd,&Options.named_color);
	} else if (!strcasecmp(cmd,"sound_style")) {
	    sscanf(buf,"%s = %d",cmd,&Options.named_sound);
	} else if (!strcasecmp(cmd,"piece_style")) {
	    sscanf(buf,"%s = %d",cmd,&Options.named_piece);
	} else if (!strcasecmp(cmd,"game_style")) {
	    sscanf(buf,"%s = %d",cmd,&Options.named_game);
	} else {
	    Debug("Unable to parse configuration line\n%s",buf);
	}
    }
    fclose(fin);
    Debug("Preference file [%s] loaded.\n",filespec);
    return;
}

/***************************************************************************
 *      parse_options()
 * Check the command-line arguments.
 ***************************************************************************/
static void
parse_options(int argc, char *argv[])
{
    int i;

    for (i=1; i<argc; i++) {
	if (!strcmp(argv[i],"-h") || !strcmp(argv[i],"--help"))
	    usage();
	else if (!strcmp(argv[i],"-?") || !strcmp(argv[i],"-help"))
	    usage();
	else if (!strcmp(argv[i],"-b") || !strcmp(argv[i],"--bg"))
	    Options.flame_wanted = TRUE;
	else if (!strcmp(argv[i],"-n") || !strcmp(argv[i],"--noflame"))
	    Options.flame_wanted = FALSE;
	else if (!strcmp(argv[i],"-s") || !strcmp(argv[i],"--sound"))
	    Options.sound_wanted = TRUE;
	else if (!strcmp(argv[i],"-q") || !strcmp(argv[i],"--quiet"))
	    Options.sound_wanted = FALSE;
	else if (!strcmp(argv[i],"-w") || !strcmp(argv[i],"--window"))
	    Options.full_screen = FALSE;
	else if (!strcmp(argv[i],"-f") || !strcmp(argv[i],"--fullscreen"))
	    Options.full_screen = TRUE;
	else if (!strncmp(argv[i],"-d=", 3) || !strncmp(argv[i],"--depth=", 8)) {
	    sscanf(strchr(argv[i],'=')+1,"%d",&Options.bpp_wanted);
	} else if (!strncmp(argv[i],"-r=", 3) || !strncmp(argv[i],"--repeat=", 8)) {
	    sscanf(strchr(argv[i],'=')+1,"%d",&Options.key_repeat_delay);
	    if (Options.key_repeat_delay < 1) Options.key_repeat_delay = 1;
	    if (Options.key_repeat_delay > 32) Options.key_repeat_delay = 32;
	} else {
	    Debug("option not understood: [%s]\n",argv[i]);
	    usage();
	}
    }
    Debug("Command line arguments parsed.\n");
    return;
}

/***************************************************************************
 *      level_adjust()
 * What happens with all of those thumbs-up, thumbs-down adjustments?
 *
 * Returns the new value of "match". 
 ***************************************************************************/
static int 
level_adjust(int a[3], int b[3], int level[2], int match)
{
    int i;
    int up[2] = {0, 0}, down[2] = {0, 0};

    for (i=0;i<=match;i++) {
	if (a[i] == ADJUST_UP) up[0]++;
	if (a[i] == ADJUST_DOWN) down[0]++;
	if (b[i] == ADJUST_UP) up[1]++;
	if (b[i] == ADJUST_DOWN) down[1]++;
    }

    while (up[0] > 0 && up[1] > 0) {
	up[0]--; 
	up[1]--;
    }
    while (up[0] > 0 && down[0] > 0) {
	up[0]--;
	down[0]--;
    }
    while (up[1] > 0 && down[1] > 0) {
	up[1]--;
	down[1]--;
    }
    while (down[0] > 0 && down[1] > 0) {
	down[0]--;
	down[1]--;
    }
    if (up[0] == 3 || down[0] == 3 || up[1] == 3 || down[1] == 3) {
	if (up[0] == 3) 	level[0]++;
	if (down[0] == 3) 	level[0]--;
	if (up[1] == 3) 	level[1]++;
	if (down[1] == 3) 	level[1]--;
	a[0] = a[1] = a[2] = b[0] = b[1] = b[2] = -1;
	return 0;
    } 
    a[0] = a[1] = a[2] = b[0] = b[1] = b[2] = -1;
    match = max( max(up[0],up[1]), max(down[0], down[1]) );
    /* fill out the a[], b[] arrays */
    for (i=0;i<up[0];i++)
	a[i] = ADJUST_UP;
    for (i=up[0]; i<up[0] + down[0]; i++)
	a[i] = ADJUST_DOWN;
    for (i=up[0] + down[0]; i<match;i++)
	a[i] = ADJUST_SAME;

    for (i=0;i<up[1];i++)
	b[i] = ADJUST_UP;
    for (i=up[1]; i<up[1] + down[1]; i++)
	b[i] = ADJUST_DOWN;
    for (i=up[1] + down[1]; i<match;i++)
	b[i] = ADJUST_SAME;

    return match;
}

/***************************************************************************
 *      play_TWO_PLAYERS()
 * Play the TWO_PLAYERS-style game. You and someone else both have two
 * minutes per level, but the limit isn't deadly. Complex adjustment rules.
 ***************************************************************************/
static void
play_TWO_PLAYERS(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[2], person *p1, person *p2)
{
    int curtimeleft;
    int match;
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    int their_adj[3];			/* their three winnings so far */
    int done = 0;
    int level[2];
    time_t our_time;

    my_adj[0] = my_adj[1] = my_adj[2] = -1;
    their_adj[0] = their_adj[1] = their_adj[2] = -1;
    match = 0;

    level[0] = p1->level;
    level[1] = p2->level;

    /* start the games */
    while (!done) { 
	time(&our_time);
	/* make the boards */

	SeedRandom(our_time);
	g[0] = generate_board(10,20,level[0]);
	SeedRandom(our_time);
	g[1] = generate_board(10,20,level[1]);
	SeedRandom(our_time);

	event_name[0] = p1->name;
	event_name[1] = p2->name;

	/* draw the background */
	draw_background(screen, cs.style[0]->w, g, level, my_adj, their_adj,
		event_name);

	/* start the fun! */
	curtimeleft = 120;

	event_cs[0] = event_cs[1] = cs.style[cs.choice];
	event_ss[0] = event_ss[1] = ss.style[ss.choice];

	if (event_loop(screen, ps.style[ps.choice], 
		event_cs, event_ss, g,
		level, 0, &curtimeleft, 0, adjustment, NULL,
		our_time, HUMAN_PLAYER, HUMAN_PLAYER, NULL) >= 0) {
	    draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj,
		    event_name);
	    SDL_Delay(1000);
	}
	my_adj[match] = adjustment[0];
	their_adj[match] = adjustment[1];
	match = level_adjust(my_adj, their_adj, level, match);

	
	/* update results */
	p1->level = level[0];
	p2->level = level[1];

	/* show them what's what! */
	draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj, 
		event_name);
	draw_score(screen,0);
	draw_score(screen,1);
	done = give_notice(NULL,1);
    } /* end: while !done */
    return;
}

/***************************************************************************
 *      play_AI_VS_AI()
 * Play the AI_VS_AI-style game. Two AI's duke it out! Winnings and scores
 * are collected. 
 ***************************************************************************/
static void
play_AI_VS_AI(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[2], AI_Player *p1, AI_Player *p2)
{
    int curtimeleft;
    int match;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    int their_adj[3];			/* their three winnings so far */
    int done = 0;
    time_t our_time;
    int p1_results[3] = {0, 0, 0};
    int p2_results[3] = {0, 0, 0};

    my_adj[0] = -1; my_adj[1] = -1; my_adj[2] = -1;
    their_adj[0] = -1; their_adj[1] = -1; their_adj[2] = -1; 
    match = 0;

    /* start the games */
    while (!done) { 

	time(&our_time);
	/* make the boards */
	level[0] = level[1] = Options.faster_levels ? 1+ZEROTO(8) :
	    2+ZEROTO(16);
	

	SeedRandom(our_time);
	g[0] = generate_board(10,20,level[0]);
	SeedRandom(our_time);
	g[1] = generate_board(10,20,level[0]);
	SeedRandom(our_time);

	event_name[0] = p1->name;
	event_name[1] = p2->name;

	/* draw the background */
	draw_background(screen, cs.style[0]->w, g, level, 
		p1_results, p2_results, event_name); 
	/* start the fun! */
	curtimeleft = 120;

	event_cs[0] = cs.style[cs.choice];
	event_cs[1] = cs.style[ZEROTO(cs.num_style)];
	event_ss[0] = event_ss[1] = ss.style[ss.choice];
	event_ai[0] = p1; event_ai[1] = p2;

	if (event_loop(screen, ps.style[ps.choice], 
		event_cs, event_ss, g,
		level, 0, &curtimeleft, 0, adjustment, NULL,
		our_time, AI_PLAYER, AI_PLAYER, event_ai) < 0) {
	    return;
	}

	if (adjustment[0] != -1)
	    p1_results[ adjustment[0] ] ++;
	if (adjustment[1] != -1) 
	    p2_results[ adjustment[1] ] ++;

	/* show them what's what! */
	draw_background(screen, cs.style[0]->w, g, level,
		p1_results, p2_results, event_name);
	draw_score(screen,0);
	draw_score(screen,1);
    } /* end: while !done */
    return;
}

/***************************************************************************
 *      play_SINGLE_VS_AI()
 * Play the SINGLE_VS_AI-style game. You and someone else both have two
 * minutes per level, but the limit isn't deadly. Complex adjustment rules.
 ***************************************************************************/
static int
play_SINGLE_VS_AI(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[2], person *p, AI_Player *aip)
{
    int curtimeleft;
    int match;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    int their_adj[3];			/* their three winnings so far */
    int done = 0;
    time_t our_time;

    my_adj[0] = my_adj[1] = my_adj[2] = -1;
    their_adj[0] = their_adj[1] = their_adj[2] = -1;
    match = 0;

    /* start the games */
    level[0] = level[1] = p->level; /* AI matches skill */
    while (!done) { 
	time(&our_time);
	/* make the boards */
	level[1] = level[0];

	SeedRandom(our_time);
	g[0] = generate_board(10,20,level[0]);
	SeedRandom(our_time);
	g[1] = generate_board(10,20,level[0]);
	SeedRandom(our_time);

	event_name[0] = p->name;
	event_name[1] = aip->name;

	/* draw the background */
	draw_background(screen, cs.style[0]->w, g, level, my_adj, their_adj,
		event_name);

	/* start the fun! */
	curtimeleft = 120;

	event_cs[0] = cs.style[cs.choice];
	event_cs[1] = cs.style[ZEROTO(cs.num_style)];
	event_ss[0] = event_ss[1] = ss.style[ss.choice];
	event_ai[0] = NULL; event_ai[1] = aip;

	if (event_loop(screen, ps.style[ps.choice], 
		event_cs, event_ss, g,
		level, 0, &curtimeleft, 0, adjustment, NULL,
		our_time, HUMAN_PLAYER, AI_PLAYER, event_ai) >= 0) {
	    draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj,
		    event_name);
	    draw_score(screen,0);
	    draw_score(screen,1);
	    SDL_Delay(1000);
	}
	my_adj[match] = adjustment[0];
	their_adj[match] = adjustment[1];
	match = level_adjust(my_adj, their_adj, level, match);

	/* show them what's what! */
	draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj,
		event_name);
	draw_score(screen,0);
	draw_score(screen,1);
	done = give_notice(NULL, 1);
    } /* end: while !done */
    return level[0];
}


/***************************************************************************
 *      play_NETWORK()
 * Play the NETWORK-style game. You and someone else both have two minutes
 * per level, but the limit isn't deadly. Complex adjustment rules. :-)
 ***************************************************************************/
static int
play_NETWORK(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[2], person *p, char *hostname) 
{
    int curtimeleft;
    extern char *error_msg;
    int server;
    int match;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    int their_adj[3];			/* their three winnings so far */
    char message[1024];
    char *their_name;
    int done = 0;
    int sock;
    int their_cs_choice;
    int their_data;
    time_t our_time;

    server = (hostname == NULL);

#define SEND(msg,len) {if (send(sock,(const char *)msg,len,0) < (signed)len) goto error;}
#define RECV(msg,len) {if (recv(sock,(char *)msg,len,0) < (signed)len) goto error;}

#define SERVER_SYNC	0x12345678
#define CLIENT_SYNC	0x98765432

    level[0] = p->level;
    /* establish connection */
    if (server) {
	SDL_Event event;
	/* try this a lot until they press Q to give up */
	clear_screen_to_flame();

	draw_string("Waiting for the client ...",
		color_blue, screen->w/2, screen->h/2, DRAW_CENTER|DRAW_ABOVE
		|DRAW_UPDATE);
	draw_string("Press 'Q' to give up.",
		color_blue, screen->w/2, screen->h/2, DRAW_CENTER|DRAW_UPDATE);

	do {
	    sock = Server_AwaitConnection(7741);
	    if (sock == -1 && SDL_PollEvent(&event) && event.type == SDL_KEYDOWN)
		if (event.key.keysym.sym == SDLK_q)
		    goto done;
	} while (sock == -1);
    } else { /* client */
	int i;
	SDL_Event event;
	sock = -1;
	for (i=0; i<5 && sock == -1; i++) {
	    sock = Client_Connect(hostname,7741);
	    if (sock == -1 && SDL_PollEvent(&event) && event.type == SDL_KEYDOWN && 
		 event.key.keysym.sym == SDLK_q)
		goto done;
	    if (sock == -1) SDL_Delay(1000);
	}
	if (sock == -1)
	    goto error;
    }
    clear_screen_to_flame();

    /* consistency checks: same number of colors */
    SEND(&cs.style[cs.choice]->num_color, sizeof(int));
    RECV(&their_data, sizeof(int));
    if (their_data != cs.style[cs.choice]->num_color) {
	sprintf(message,"The # of colors in your styles are not equal. (%d/%d)",
		cs.style[cs.choice]->num_color, their_data);
	goto known_error;
    }

    SEND(&ps.style[ps.choice]->num_piece, sizeof(int));
    RECV(&their_data, sizeof(int));
    if (their_data != ps.style[ps.choice]->num_piece) {
	sprintf(message,"The # of shapes in your styles are not equal. (%d/%d)",
		ps.style[ps.choice]->num_piece, their_data);
	goto known_error;
    }

    SEND(&Options.special_wanted, sizeof(int));
    RECV(&their_data, sizeof(int));
    if (their_data != Options.special_wanted) {
	sprintf(message,"You must both agree on whether to use Power Pieces");
	goto known_error;
    }

    SEND(&Options.faster_levels, sizeof(int));
    RECV(&their_data, sizeof(int));
    if (their_data != Options.special_wanted) {
	sprintf(message,"You must both agree on Double Difficulty");
	goto known_error;
    }

    /* initial levels */
    SEND(&level[0],sizeof(level[0]));
    SEND(&cs.choice,sizeof(cs.choice));
    match = strlen(p->name);
    SEND(&match, sizeof(match));
    SEND(p->name,strlen(p->name));

    RECV(&level[1],sizeof(level[1]));
    Assert(sizeof(their_cs_choice) == sizeof(cs.choice));
    RECV(&their_cs_choice,sizeof(cs.choice));
    if (their_cs_choice < 0 || their_cs_choice >= cs.num_style) {
	their_cs_choice = cs.choice;
    }
    RECV(&match,sizeof(match));
    Calloc(their_name, char *, match);
    RECV(their_name, match);

    my_adj[0] = my_adj[1] = my_adj[2] = -1;
    their_adj[0] = their_adj[1] = their_adj[2] = -1;
    match = 0;

    /* start the games */
    while (!done) { 
	/* pass the seed */
	if (server) {
	    time(&our_time);
	    SEND(&our_time,sizeof(our_time));
	} else {
	    RECV(&our_time,sizeof(our_time));
	}
	/* make the boards */
	SeedRandom(our_time);
	g[0] = generate_board(10,20,level[0]);
	SeedRandom(our_time);
	g[1] = generate_board(10,20,level[1]);
	SeedRandom(our_time);

	event_name[0] = p->name;
	event_name[1] = their_name;

	/* draw the background */
	draw_background(screen, cs.style[0]->w, g, level, my_adj, their_adj,
		event_name);

	/* start the fun! */
	curtimeleft = 120;

	event_cs[0] = cs.style[cs.choice];
	event_cs[1] = cs.style[their_cs_choice];
	event_ss[0] = event_ss[1] = ss.style[ss.choice];

	event_loop(screen, ps.style[ps.choice], 
		event_cs, event_ss, g,
		level, sock, &curtimeleft, 0, adjustment, NULL,
		our_time, HUMAN_PLAYER, NETWORK_PLAYER, NULL);
	SEND(&Score[0],sizeof(Score[0]));
	RECV(&Score[1],sizeof(Score[1]));
	draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj,
		event_name);
	draw_score(screen,0);
	draw_score(screen,1);
	SDL_Delay(1000);
	my_adj[match] = adjustment[0];
	their_adj[match] = adjustment[1];
	match = level_adjust(my_adj, their_adj, level, match);

	/* verify that our hearts are in the right place */
	if (server) {
	    unsigned int msg = SERVER_SYNC;
	    SEND(&msg,sizeof(msg));
	    RECV(&msg,sizeof(msg));
	    if (msg != CLIENT_SYNC) {
		give_notice("Network Error: Syncronization Failed", 0);
		goto done;
	    }
	} else {
	    unsigned int msg;
	    RECV(&msg,sizeof(msg));
	    if (msg != SERVER_SYNC) {
		give_notice("Network Error: Syncronization Failed", 1);
		goto done;
	    }
	    msg = CLIENT_SYNC;
	    SEND(&msg,sizeof(msg));
	}
	/* OK, we believe we are both dancing on the beat ... */
	/* don't even talk to me about three-way handshakes ... */

	/* show them what's what! */
	draw_background(screen, cs.style[0]->w,g,level,my_adj,their_adj,
		event_name);
	draw_score(screen,0);
	draw_score(screen,1);
	if (server) {
	    done = give_notice(NULL, 1);
	    SEND(&done,sizeof(done));
	} else {
	    SDL_Event event;
	    draw_string("Waiting for the", color_blue, 0, 0,
		    DRAW_CENTER|DRAW_ABOVE |DRAW_UPDATE|DRAW_GRID_0);
	    draw_string("Server to go on.", color_blue, 0, 0, DRAW_CENTER
		    |DRAW_UPDATE|DRAW_GRID_0);
	    RECV(&done,sizeof(done));
	    while (SDL_PollEvent(&event))
		/* do nothing */ ;
	}
	clear_screen_to_flame();
    } /* end: while !done */
    close(sock);
    return level[0];

    /* error conditions! */
error: 
    if (error_msg) 
	sprintf(message,"Network Error: %s",error_msg);
    else 
	sprintf(message,"Network Error: %s",strerror(errno));
known_error: 
    clear_screen_to_flame();
    give_notice(message, 0);
done: 
    close(sock);
    return level[0];
}

/***************************************************************************
 *      play_MARATHON()
 * Play the MARATHON-style game. You have five minutes per level: try to
 * get many points. Returns the final level.
 ***************************************************************************/
static int
play_MARATHON(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[2], person *p) 
{
    int curtimeleft;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    char message[1024];
    int result;

    level[0] = p->level;		/* starting level */
    SeedRandom(0);
    Score[0] = 0;		/* global variable! */

    my_adj[0] = -1; my_adj[1] = -1; my_adj[2] = -1;
    while (1) {	
	/* until they give up by pressing 'q'! */
	/* 5 mintues per match */
	curtimeleft = 300;
	/* generate the board */
	g[0] = generate_board(10,20,level[0]);
	/* draw the background */

	event_name[0] = p->name;

	draw_background(screen, cs.style[0]->w, g, level, my_adj, NULL, 
		event_name);
	/* get the result, but don't update level yet */

	event_cs[0] = event_cs[1] = cs.style[cs.choice];
	event_ss[0] = event_ss[1] = ss.style[ss.choice];

	result = event_loop(screen, ps.style[ps.choice], 
		event_cs, event_ss, g,
		level, 0, &curtimeleft, 1, adjustment, NULL,
		time(NULL), HUMAN_PLAYER, NO_PLAYER, NULL);
	if (result < 0) { 	/* explicit quit */
	    return level[0];
	}
	/* reset board */
	my_adj[0] = adjustment[0];
	/* display those mighty accomplishments winnings */
	draw_background(screen, cs.style[0]->w ,g, level,my_adj,NULL,
		event_name);
	draw_score(screen,0);
	if (adjustment[0] != ADJUST_UP) {
	    give_notice("Game Over!", 0);
	    return level[0];
	}
	my_adj[0] = -1;
	level[0]++;
	sprintf(message,"Up to Level %d!",level[0]);
	if (give_notice(message, 1)) {
	    return level[0];
	}
    }
}

/***************************************************************************
 *      play_SINGLE()
 * Play the SINGLE-style game. You have five total minutes to win three
 * matches. Returns the final level.
 ***************************************************************************/
static int
play_SINGLE(color_styles cs, piece_styles ps, sound_styles ss,
    Grid g[], person *p)
{
    int curtimeleft;
    int match;
    int level[2];			/* level array: me + dummy slot */
    int adjustment[2] = {-1, -1};	/* result of playing a match */
    int my_adj[3];			/* my three winnings so far */
    char message[1024];
    int result;

    level[0] = p->level;	/* starting level */
    SeedRandom(0);
    Score[0] = 0;		/* global variable! */

    while (1) {	
	/* until they give up by pressing 'q'! */
	/* 5 total minutes for three matches */
	curtimeleft = 300;
	my_adj[0] = -1; my_adj[1] = -1; my_adj[2] = -1;

	for (match=0; match<3 && curtimeleft > 0; match++) {
	    /* generate the board */
	    g[0] = generate_board(10,20,level[0]);

	    event_name[0] = p->name;

	    /* draw the background */
	    draw_background(screen, cs.style[0]->w, g, level, my_adj, NULL,
		    event_name);
	    /* get the result, but don't update level yet */

	    event_cs[0] = event_cs[1] = cs.style[cs.choice];
	    event_ss[0] = event_ss[1] = ss.style[ss.choice];

	    result = event_loop(screen, ps.style[ps.choice], 
		    event_cs, event_ss, g,
		    level, 0, &curtimeleft, 1, adjustment, NULL,
		    time(NULL), HUMAN_PLAYER, NO_PLAYER, NULL);
	    if (result < 0) { 	/* explicit quit */
		return level[0];
	    }
	    /* reset board */
	    my_adj[match] = adjustment[0];
	    /* display those mighty accomplishments winnings */
	    draw_background(screen, cs.style[0]->w,g, level,my_adj,NULL,
		    event_name);
	    draw_score(screen,0);
	    message[0] = 0;
	    if (match == 2) {
		if (my_adj[0] == ADJUST_UP && my_adj[1] == ADJUST_UP &&
			my_adj[2] == ADJUST_UP)  /* go up! */ {
		    level[0]++;
		    sprintf(message,"Up to level %d!",level[0]);
		}
		else if (my_adj[0] == ADJUST_DOWN && my_adj[1] == ADJUST_DOWN &&
			my_adj[2] == ADJUST_DOWN) { /* devolve! */
		    level[0]--;
		    sprintf(message,"Down to level %d!",level[0]);
		}
	    }
	    if (give_notice(message, 1) == 1) { /* explicit quit */
		return level[0];
	    }
	}
    }
}

/*
 *                               Alizarin Tetris
 * Functions relating to AI players. 
 *
 * Copyright 2000, Westley Weimer & Kiri Wagstaff
 */




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



struct layout_struct {
    /* the whole board layout */
    SDL_Rect grid_border[2];
    SDL_Rect grid[2];

    SDL_Rect score[2];

    SDL_Rect name[2];

    struct adjust_struct {
	SDL_Rect symbol[3];
    } adjust[2];

    SDL_Rect time;
    SDL_Rect time_border;

    SDL_Rect next_piece_border[2];
    SDL_Rect next_piece[2];

    SDL_Rect 	  pause;
} layout;

/* The background image, global so we can do updates */
SDL_Surface *adjust_symbol[3] = { NULL, NULL, NULL };
/* Rectangles that specify where my/their pieces can legally be */

int Score[2];

/***************************************************************************
 *      poll_and_flame()
 * Poll for events and run the flaming background.
 *********************************************************************PROTO*/
void
poll_and_flame(SDL_Event *ev)
{
    while (!(SDL_PollEvent(ev))) {
	atris_run_flame();
    }
    return;
}

/***************************************************************************
 *      clear_screen_to_flame()
 * Clears the screen so that only the flame layer is visible.
 *********************************************************************PROTO*/
void
clear_screen_to_flame(void)
{
    SDL_Rect all;

    all.x = all.y = 0;
    all.w = screen->w;
    all.h = screen->h;

    SDL_FillRect(widget_layer, &all, int_black);
    SDL_FillRect(screen, &all, int_black);
    SDL_BlitSafe(flame_layer, NULL, screen, NULL);
    SDL_UpdateSafe(screen, 1, &all);
}

/***************************************************************************
 *      setup_colors()
 * Sets up our favorite colors.
 *********************************************************************PROTO*/
void
setup_colors(SDL_Surface *screen)
{
    int i = 0;

    color_white.r = color_white.g = color_white.b = 0xFF;
    color_white.unused = 0;

    color_black.r = color_black.g = color_black.b = color_black.unused = 0;

    color_red.r = 0xFF; color_red.g = color_red.b = color_red.unused = 0;

    color_blue.b = 0xFF; color_blue.r = color_blue.g = color_blue.unused = 0;

    color_purple.b = 128; color_purple.r = 128; color_purple.g = 64;
	color_purple.unused = 0;

    int_black = SDL_MapRGB(screen->format, 0, 0, 0);

    /* find the darkest black that is different from "all black" */
    do {
	i++;
	int_solid_black = SDL_MapRGB(screen->format, i, i, i);
    } while (int_black == int_solid_black && i < 255);
    /* couldn't find one?! */
    if (i == 255) {
	Debug("*** Warning: transparency is compromised by RGB format.\n");
	int_solid_black = SDL_MapRGB(screen->format, 1, 1, 1);
    } else if (i != 1)
	Debug("*** Note: First non-black color found at magnitude %d.\n",i);

    int_white = SDL_MapRGB(screen->format, 255, 255, 255);
    int_grey  = SDL_MapRGB(screen->format, 128, 128, 128);
    int_blue  = SDL_MapRGB(screen->format, 0, 0, 255);
    int_med_blue = SDL_MapRGB(screen->format, 0, 0, 0);
    int_dark_blue  = SDL_MapRGB(screen->format, 0, 0, 128);
    int_purple  = SDL_MapRGB(screen->format, 128, 64, 128);
    int_dark_purple  = SDL_MapRGB(screen->format, 64, 32, 64);
}

/***************************************************************************
 *      draw_string()
 * Draws the given string at the given location using the default font.
 *********************************************************************PROTO*/
#define	DRAW_CENTER	(1<<0)
#define	DRAW_ABOVE	(1<<1)
#define DRAW_LEFT	(1<<2)
#define DRAW_UPDATE	(1<<3)
#define DRAW_CLEAR	(1<<4)
#define DRAW_HUGE	(1<<5)
#define DRAW_LARGE	(1<<6)
#define DRAW_GRID_0	(1<<7)
int
draw_string(char *text, SDL_Color sc, int x, int y, int flags)
{
    SDL_Surface * text_surface;
    SDL_Rect r;

    if (flags & DRAW_GRID_0) {
	r.x = layout.grid[0].x + layout.grid[0].w / 2;
	r.y = layout.grid[0].y + layout.grid[0].h / 2;
    } else {
	r.x = x;
	r.y = y;
    }

    if (flags & DRAW_HUGE) {
	text_surface = TTF_RenderText_Blended(hfont, text, sc); Assert(text_surface);
    } else if (flags & DRAW_LARGE) {
	text_surface = TTF_RenderText_Blended(lfont, text, sc); Assert(text_surface);
    } else {
	text_surface = TTF_RenderText_Blended(font, text, sc); Assert(text_surface);
    }

    r.w = text_surface->w;
    r.h = text_surface->h;

    if (flags & DRAW_CENTER)
	r.x -= (r.w / 2);
    if (flags & DRAW_LEFT)
	r.x -= text_surface->w;
    if (flags & DRAW_ABOVE)
	r.y -= (r.h);
    if (flags & DRAW_CLEAR) {
	SDL_FillRect(widget_layer, &r, int_black);
	SDL_FillRect(screen, &r, int_black);
    }

    SDL_BlitSafe(text_surface, NULL, widget_layer, &r);
    SDL_BlitSafe(flame_layer, &r, screen, &r);
    SDL_BlitSafe(widget_layer, &r, screen, &r);
    if (flags & DRAW_UPDATE)
	SDL_UpdateSafe(screen, 1, &r);

    SDL_FreeSurface(text_surface);
    return r.h;
}

/***************************************************************************
 *      give_notice()
 * Draws a little "press any key to continue"-type notice between levels.
 * "s" is any message you would like to display.
 * Returns 1 if the user presses 'q', 0 if the users presses 'c'.
 *********************************************************************PROTO*/
int
give_notice(char *s, int quit_possible)
{
    SDL_Event event;

    while (SDL_PollEvent(&event)) {
	/* pull out all leading 'Q's */
    }

    if (s && s[0]) 	/* throw that 'ol user text up there */
	draw_string(s, color_blue, 
		layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y
		+ 64, DRAW_CENTER | DRAW_UPDATE);

    if (quit_possible) 
	draw_string("Press 'Q' to Quit", color_red,
		layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y + 128, 
		DRAW_CENTER | DRAW_UPDATE);

    draw_string("Press 'G' to Go On", color_red,
	    layout.grid[0].x + (layout.grid[0].w / 2), layout.grid[0].y +
	    128 + 30, DRAW_CENTER | DRAW_UPDATE);
    while (1) {
	poll_and_flame(&event);
	if (event.type == SDL_KEYDOWN &&
		event.key.keysym.sym == 'q') 
	    return 1;
	if (event.type == SDL_KEYDOWN &&
		event.key.keysym.sym == 'g')
	    return 0;
    } 
}

/***************************************************************************
 *      load_adjust_symbol()
 * Loads the level adjustment images.
 ***************************************************************************/
static void
load_adjust_symbols()
{
    SDL_Surface *bitmap = SDL_LoadBMP("graphics/Level-Up.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Up.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[0] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[0]) 
	PANIC("Can not convert [graphics/Level-Up.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[0], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    bitmap = SDL_LoadBMP("graphics/Level-Medium.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Medium.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[1] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[1]) 
	PANIC("Can not convert [graphics/Level-Medium.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[1], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    bitmap = SDL_LoadBMP("graphics/Level-Down.bmp");
    if (!bitmap) PANIC("Could not load [graphics/Level-Down.bmp]");
    if (bitmap->format->palette != NULL ) {
	SDL_SetColors(screen, bitmap->format->palette->colors, 0,
		bitmap->format->palette->ncolors);
    }
    adjust_symbol[2] = SDL_DisplayFormat(bitmap);
    if (!adjust_symbol[2]) 
	PANIC("Can not convert [graphics/Level-Down.bmp] to hardware format");
    SDL_SetColorKey(adjust_symbol[2], SDL_SRCCOLORKEY,
	    SDL_MapRGB(screen->format, 0, 0, 0));
    SDL_FreeSurface(bitmap);

    Assert(adjust_symbol[0]->h == adjust_symbol[1]->h);
    Assert(adjust_symbol[1]->h == adjust_symbol[2]->h);
}

/***************************************************************************
 *      draw_bordered_rect()
 * Draws a bordered rectangle. Sets "border" based on "orig". 
 *********************************************************************PROTO*/
void
draw_bordered_rect(SDL_Rect *orig, SDL_Rect *border, int thick)
{
    Assert(thick > 0 && thick < 8);

    border->x = orig->x - thick;
    border->y = orig->y - thick;
    border->w = orig->w + thick * 2;
    border->h = orig->h + thick * 2;

    SDL_FillRect(widget_layer, border, int_border_color);
    SDL_FillRect(widget_layer, orig, int_solid_black);

    /* unchanged: SDL_BlitSafe(flame_layer, NULL, screen, &r); */
    SDL_BlitSafe(widget_layer, border, screen, border);
    SDL_UpdateSafe(screen, 1, border);

    return;
}

/***************************************************************************
 *      draw_pre_bordered_rect()
 * Draws a bordered rectangle. 
 *********************************************************************PROTO*/
void
draw_pre_bordered_rect(SDL_Rect *border, int thick)
{
    SDL_Rect orig;
    Assert(thick > 0 && thick < 8);

    orig.x = border->x + thick;
    orig.y = border->y + thick;
    orig.w = border->w - thick * 2;
    orig.h = border->h - thick * 2;

    SDL_FillRect(widget_layer, border, int_border_color);
    SDL_FillRect(widget_layer, &orig, int_solid_black);

    /* unchanged: SDL_BlitSafe(flame_layer, NULL, screen, &r); */
    SDL_BlitSafe(widget_layer, border, screen, border);
    SDL_UpdateSafe(screen, 1, border);
    return;
}

/***************************************************************************
 *      setup_layers()
 * Set up the widget and flame layers.
 *********************************************************************PROTO*/
void
setup_layers(SDL_Surface * screen)
{
    SDL_Rect all;
    flame_layer = SDL_CreateRGBSurface
	(SDL_HWSURFACE|SDL_SRCCOLORKEY,
	 screen->w, screen->h, 8, 0, 0, 0, 0);
    widget_layer = SDL_CreateRGBSurface
	(SDL_HWSURFACE|SDL_SRCCOLORKEY,
	 screen->w, screen->h, screen->format->BitsPerPixel, 
	 screen->format->Rmask, screen->format->Gmask, 
	 screen->format->Bmask, screen->format->Amask);
    if (SDL_SetColorKey(widget_layer, SDL_SRCCOLORKEY, 
		SDL_MapRGB(widget_layer->format, 0, 0, 0)))
	PANIC("SDL_SetColorKey failed on the widget layer.");

    if (int_black != SDL_MapRGB(widget_layer->format, 0, 0, 0)) {
	Debug("*** Warning: screen and widget layer have different RGB format.\n");
    }
    if (int_black != SDL_MapRGB(flame_layer->format, 0, 0, 0)) {
	Debug("*** Warning: screen and flame layer have different RGB format.\n");
    }

    all.x = all.y = 0;
    all.w = screen->w;
    all.h = screen->h;

    SDL_FillRect(widget_layer, &all, int_black);
    SDL_FillRect(flame_layer, &all, int_solid_black);
}

/***************************************************************************
 *      draw_background()
 * Draws the Alizarin Tetris background. Not yet complete, but it's getting
 * better. :-)
 *********************************************************************PROTO*/
void
draw_background(SDL_Surface *screen, int blockWidth, Grid g[],
	int level[], int my_adj[], int their_adj[], char *name[])
{
    char buf[1024];
    int i;
#define IS_DOUBLE(x) ((x==NETWORK)||(x==SINGLE_VS_AI)||(x==TWO_PLAYERS)||(x==AI_VS_AI))

    if (IS_DOUBLE(gametype)) {
	Assert(g[0].w == g[1].w);
	Assert(g[0].h == g[1].h);
    }

    /*
     * clear away the old stuff
     */
    memset(&layout, 0, sizeof(layout));
    
    if (!adjust_symbol[0]) { /* only load these guys the first time */
	load_adjust_symbols();
    }
    /*
     * 	THE BOARD
     */
    if (IS_DOUBLE(gametype)) {
	layout.grid[0].x = ((screen->w / 2) - ((g[0].w*blockWidth))) - 5 * blockWidth - 2;
	layout.grid[0].y = ((screen->h - (g[0].h*blockWidth)) / 2);
	layout.grid[0].w = (g[0].w*blockWidth);
	layout.grid[0].h = (g[0].h*blockWidth);

	layout.grid[1].y = ((screen->h - (g[0].h*blockWidth)) / 2);
	layout.grid[1].w = (g[0].w*blockWidth);
	layout.grid[1].h = (g[0].h*blockWidth);
	layout.grid[1].x = ((screen->w / 2) - 4) + 5 * blockWidth + 6;

	/* Draw the opponent's board */
	draw_bordered_rect(&layout.grid[1], &layout.grid_border[1], 2);
	g[1].board = layout.grid[1];
    }  else {
	layout.grid[0].x = (screen->w - (g[0].w*blockWidth))/2 ;
	layout.grid[0].y = (screen->h - (g[0].h*blockWidth))/2 ;
	layout.grid[0].w = (g[0].w*blockWidth) ;
	layout.grid[0].h = (g[0].h*blockWidth) ;
	/* Don't need a Board[1] */
    }
    /* draw the leftmost board */
    draw_bordered_rect(&layout.grid[0], &layout.grid_border[0], 2);
    g[0].board = layout.grid[0];

    /*
     * 	SCORING, Names
     */
    for (i=0; i< 1+IS_DOUBLE(gametype); i++) {
	layout.name[i].x = layout.grid[i].x;
	layout.name[i].y = layout.grid[i].y + layout.grid[i].h + 2;
	layout.name[i].w = layout.grid[i].w;
	layout.name[i].h = screen->h - layout.name[i].y;

	if (gametype == DEMO) {
	    char buf[1024];
	    SDL_FillRect(widget_layer, &layout.name[i], int_black);
	    SDL_FillRect(screen, &layout.name[i], int_black);
	    sprintf(buf,"Demo (%s)",name[i]);
	    draw_string(buf, color_blue, layout.grid_border[i].x +
		    layout.grid_border[i].w/2, layout.grid_border[i].y +
		    layout.grid_border[i].h, DRAW_CENTER | DRAW_CLEAR | DRAW_UPDATE);
	} else {
	    draw_string(name[i], color_blue,
		    layout.grid_border[i].x + layout.grid_border[i].w/2,
		    layout.grid_border[i].y + layout.grid_border[i].h,
		    DRAW_CENTER);
	}
	/* Set up the coordinates for future score writing */
	layout.score[i].x = layout.grid_border[i].x;
	layout.score[i].w = layout.grid_border[i].w;
	layout.score[i].y = 0;
	layout.score[i].h = layout.grid_border[i].y;
	SDL_FillRect(widget_layer, &layout.score[i], int_black);
	SDL_FillRect(screen, &layout.score[i], int_black);

	sprintf(buf,"Level %d, Score:",level[i]);
	draw_string(buf, color_blue, layout.grid_border[i].x,
		layout.grid_border[i].y, DRAW_ABOVE | DRAW_CLEAR);

	layout.score[i].x = layout.grid_border[i].x + layout.grid_border[i].w;
	layout.score[i].y = layout.grid_border[i].y;
    }

    /*
     * 	TIME LEFT
     */

#define TIME_WIDTH 	(16*5)
#define TIME_HEIGHT 	28

    if (gametype == DEMO) {
	/* do nothing */
    } else if (IS_DOUBLE(gametype)) {
	draw_string("Time Left", color_blue, 
		screen->w/2, layout.score[0].y, DRAW_CENTER | DRAW_ABOVE);
	layout.time.x = (screen->w - TIME_WIDTH)/2;
	layout.time.y = layout.score[0].y;
	layout.time.w = TIME_WIDTH;
	layout.time.h = TIME_HEIGHT;
	draw_bordered_rect(&layout.time, &layout.time_border, 2);
    } else { /* single */
	int text_h = draw_string("Time Left", color_blue,
		screen->w/10, screen->h/5, 0);
	layout.time.x = screen->w / 10;
	layout.time.y = screen->h / 5 + text_h;
	layout.time.w = TIME_WIDTH;
	layout.time.h = TIME_HEIGHT;

	draw_bordered_rect(&layout.time, &layout.time_border, 2);
    }

    /*
     *	LEVEL ADJUSTMENT
     */
    if (gametype == DEMO) {

    } else if (gametype == AI_VS_AI) {
	for (i=0;i<3;i++) {
	    char buf[80];

	    layout.adjust[0].symbol[i].x = (screen->w - adjust_symbol[i]->w)/2;
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;

	    SDL_BlitSafe(adjust_symbol[i], NULL, widget_layer, 
		    &layout.adjust[0].symbol[i]);

	    /* draw the textual tallies */
	    sprintf(buf,"%d",my_adj[i]);
	    draw_string(buf, color_red,
		    layout.adjust[0].symbol[i].x - 10, 
		    layout.adjust[0].symbol[i].y, DRAW_LEFT | DRAW_CLEAR);
	    sprintf(buf,"%d",their_adj[i]);
	    draw_string(buf, color_red,
		    layout.adjust[0].symbol[i].x + layout.adjust[0].symbol[i].w + 10, 
		    layout.adjust[0].symbol[i].y, DRAW_CLEAR);
	}
    } else if (IS_DOUBLE(gametype)) {
	for (i=0;i<3;i++) {
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].x = (screen->w - 3*adjust_symbol[i]->w)/2;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[0].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[0].symbol[i], int_black);
	    if (my_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[my_adj[i]], NULL, widget_layer, 
			&layout.adjust[0].symbol[i]);
	    layout.adjust[1].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[1].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[1].symbol[i].x = (screen->w)/2 + adjust_symbol[i]->w/2;
	    layout.adjust[1].symbol[i].y = 
		layout.time_border.y+layout.time_border.h + i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[1].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[1].symbol[i], int_black);
	    if (their_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[their_adj[i]], NULL, widget_layer, 
			&layout.adjust[1].symbol[i]);
	}
    } else { /* single player */
	for (i=0;i<3;i++) {
	    layout.adjust[0].symbol[i].w = adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].h = adjust_symbol[i]->h;
	    layout.adjust[0].symbol[i].x = 
		layout.grid_border[0].x + layout.grid_border[0].w +
		2 * adjust_symbol[i]->w;
	    layout.adjust[0].symbol[i].y = 
		layout.time_border.y +layout.time_border.h+ i * adjust_symbol[i]->h;
	    SDL_FillRect(widget_layer, &layout.adjust[0].symbol[i], int_black);
	    SDL_FillRect(screen, &layout.adjust[0].symbol[i], int_black);
	    if (my_adj[i] != -1) 
		SDL_BlitSafe(adjust_symbol[my_adj[i]], NULL, widget_layer, 
			&layout.adjust[0].symbol[i]);
	}
    }

    /*
     * NEXT PIECE
     */
    if (gametype == DEMO) {
	/* do nothing */
    } else if (IS_DOUBLE(gametype)) {
	int text_h = draw_string("Next Piece", color_blue,
		screen->w / 2, 
		layout.adjust[0].symbol[2].y +
		layout.adjust[0].symbol[2].h, DRAW_CENTER);

	layout.next_piece[0].w = 5 * blockWidth;
	layout.next_piece[0].h = 5 * blockWidth;
	layout.next_piece[0].x = (screen->w / 2) - (5 * blockWidth);
	layout.next_piece[0].y = layout.adjust[0].symbol[2].y +
	    layout.adjust[0].symbol[2].h + text_h;
	draw_bordered_rect(&layout.next_piece[0], &layout.next_piece_border[0], 2);

	layout.next_piece[1].w = layout.next_piece[0].w;
	layout.next_piece[1].h = layout.next_piece[0].h;
	layout.next_piece[1].y = layout.next_piece[0].y;
	layout.next_piece[1].x = (screen->w / 2);
	draw_bordered_rect(&layout.next_piece[1], &layout.next_piece_border[1], 2);
    } else {
	int text_h = draw_string("Next Piece", color_blue,
		screen->w/10, 2*screen->h/5, 0);

	layout.next_piece[0].w = 5 * blockWidth;
	layout.next_piece[0].h = 5 * blockWidth;
	layout.next_piece[0].x = screen->w/10;
	layout.next_piece[0].y = (2*screen->h/5) + text_h;

	/* Draw the box for the next piece to fit in */
	draw_bordered_rect(&layout.next_piece[0], &layout.next_piece_border[0], 2);
    }

    /*
     *	PAUSE BOX
     */
    if (gametype == DEMO) {

    } else if (IS_DOUBLE(gametype)) {
	layout.pause.x = layout.next_piece_border[0].x;
	layout.pause.y = layout.next_piece_border[0].h + layout.next_piece_border[0].y + 2 + 
	    layout.next_piece_border[0].h / 3;
	layout.pause.w = layout.next_piece_border[0].w + layout.next_piece[1].w;
	layout.pause.h = layout.next_piece_border[0].h / 3;
    } else {
	layout.pause.x = layout.next_piece_border[0].x;
	layout.pause.y = layout.next_piece_border[0].h + layout.next_piece_border[0].y + 2 +
	    layout.next_piece_border[0].h / 3;
	layout.pause.w = layout.next_piece_border[0].w;
	layout.pause.h = layout.next_piece_border[0].h / 3;
    }

    /* Blit onto the screen surface */
    {
	SDL_Rect dest;
	dest.x = 0; dest.y = 0; dest.w = screen->w; dest.h = screen->h;

	SDL_BlitSafe(flame_layer, NULL, screen, NULL);
	SDL_BlitSafe(widget_layer, NULL, screen, NULL);
	SDL_UpdateSafe(screen, 1, &dest);
    }
    return;
}

/***************************************************************************
 *      draw_pause()
 * Draw or clear the pause indicator.
 *********************************************************************PROTO*/
void
draw_pause(int on)
{
    int i;
    if (on) {
	draw_pre_bordered_rect(&layout.pause, 2);
	draw_string("* Paused *", color_blue, 
		layout.pause.x + layout.pause.w / 2, layout.pause.y,
		DRAW_CENTER | DRAW_UPDATE);
	for (i=0; i<2; i++) {
	    /* save this stuff so that the flame doesn't go over it .. */
	    if (layout.grid[i].w) {
		SDL_BlitSafe(screen, &layout.grid[i], widget_layer,
			&layout.grid[i]);
		SDL_BlitSafe(screen, &layout.next_piece[i], widget_layer,
			&layout.next_piece[i]);
	    }
	}
    } else {
	SDL_FillRect(widget_layer, &layout.pause, int_black);
	SDL_FillRect(screen, &layout.pause, int_black);
	SDL_BlitSafe(flame_layer, &layout.pause, screen, &layout.pause);
	/* no need to paste over it with the widget layer: we know it to
	 * be all transparent */
	SDL_UpdateSafe(screen, 1, &layout.pause);

	for (i=0; i<2; i++) 
	    if (layout.grid[i].w) {
		SDL_BlitSafe(widget_layer, &layout.grid[i], screen,
			&layout.grid[i]);
		SDL_FillRect(widget_layer, &layout.grid[i], int_solid_black);

		SDL_BlitSafe(widget_layer, &layout.next_piece[i], screen,
			&layout.next_piece[i]);
		SDL_FillRect(widget_layer, &layout.next_piece[i], int_solid_black);
	    }
    }
}

/***************************************************************************
 *      draw_clock()
 * Draws a five-digit (-x:yy) clock in the center of the screen.
 *********************************************************************PROTO*/
void
draw_clock(int seconds)
{
    static int old_seconds = -111;	/* last time we drew */
    static SDL_Surface * digit[12];
    char buf[16];
    static int w = -1, h= -1; /* max digit width/height */
    int i, c;

    if (seconds == old_seconds || gametype == DEMO) return;

    if (old_seconds == -111) {
	/* setup code */

	for (i=0;i<10;i++) {
	    sprintf(buf,"%d",i);
	    digit[i] = TTF_RenderText_Blended(font,buf,color_blue);
	}
	sprintf(buf,":"); digit[10] = TTF_RenderText_Blended(font,buf,color_red);
	sprintf(buf,"-"); digit[11] = TTF_RenderText_Blended(font,buf,color_red);

	for (i=0;i<12;i++) {
	    Assert(digit[i]);
	    /* the colorkey and display format are already done */
	    /* find the largest dimensions */
	    if (digit[i]->w > w) w = digit[i]->w;
	    if (digit[i]->h > h) h = digit[i]->h;
	}
    }

    old_seconds = seconds;

    sprintf(buf,"%d:%02d",seconds / 60, seconds % 60);

    c = layout.time.x;
    layout.time.w = w * 5;
    layout.time.h = h;

    SDL_FillRect(widget_layer, &layout.time, int_solid_black); 

    if (strlen(buf) > 5)
	sprintf(buf,"----");

    if (strlen(buf) < 5)
	layout.time.x += ((5 - strlen(buf)) * w) / 2;


    for (i=0;buf[i];i++) {
	SDL_Surface * to_blit;

	if (buf[i] >= '0' && buf[i] <= '9')
	    to_blit = digit[buf[i] - '0'];
	else if (buf[i] == ':')
	    to_blit = digit[10];
	else if (buf[i] == '-')
	    to_blit = digit[11];
	else PANIC("unknown character in clock string [%s]",buf);

	/* center the letter horizontally */
	if (w > to_blit->w) layout.time.x += (w - to_blit->w) / 2;
	layout.time.w = to_blit->w;
	layout.time.h = to_blit->h;
	/*
	Debug("[%d+%d, %d+%d]\n",
		clockPos.x,clockPos.w,clockPos.y,clockPos.h);
		*/
	SDL_BlitSafe(to_blit, NULL, widget_layer, &layout.time);
	if (w > to_blit->w) layout.time.x -= (w - to_blit->w) / 2;
	layout.time.x += w;
    }

    layout.time.x = c;
    /*    clockPos.x = (screen->w - (w * 5)) / 2;*/
    layout.time.w = w * 5;
    layout.time.h = h;
    SDL_BlitSafe(flame_layer, &layout.time, screen, &layout.time);
    SDL_BlitSafe(widget_layer, &layout.time, screen, &layout.time);
    SDL_UpdateSafe(screen, 1, &layout.time);

    return;
}

/***************************************************************************
 *      draw_score()
 *********************************************************************PROTO*/
void
draw_score(SDL_Surface *screen, int i)
{
    char buf[256];

    sprintf(buf, "%d", Score[i]);
    draw_string(buf, color_red, 
	    layout.score[i].x, layout.score[i].y, DRAW_LEFT | DRAW_CLEAR |
	    DRAW_ABOVE | DRAW_UPDATE);
}

/***************************************************************************
 *      draw_next_piece()
 * Draws the next piece on the screen.
 *
 * Needs the color style because draw_play_piece() needs it to paste the
 * bitmaps. 
 *********************************************************************PROTO*/
void
draw_next_piece(SDL_Surface *screen, piece_style *ps, color_style *cs,
	play_piece *cp, play_piece *np, int P)
{
    if (gametype != DEMO) {
	/* fake-o centering */
	int cp_right = 5 - cp->base->dim;
	int cp_down = 5 - cp->base->dim;
	int np_right = 5 - np->base->dim;
	int np_down = 5 - np->base->dim;

	draw_play_piece(screen, cs,
		cp, 
		layout.next_piece[P].x + cp_right * cs->w / 2,
		layout.next_piece[P].y + cp_down * cs->w / 2, 0,
		np, 
		layout.next_piece[P].x + np_right * cs->w / 2,
		layout.next_piece[P].y + np_down * cs->w / 2, 0);
    }
    return;
}

/*
 * $Log: display.c,v $
 * Revision 1.25  2000/11/01 04:39:41  weimer
 * clear the little scoring spot correctly, updates for making a no-sound
 * distribution
 *
 * Revision 1.24  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.23  2000/10/30 16:25:25  weimer
 * display the network player score during network games. Also give the
 * non-server a little message when waiting for the server to go on.
 *
 * Revision 1.22  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.21  2000/10/29 21:28:58  weimer
 * fixed a few failures to clear the screen if we didn't have a flaming
 * backdrop
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
 * Revision 1.17  2000/10/28 13:39:14  weimer
 * added a pausing feature ...
 *
 * Revision 1.16  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.15  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.14  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.13  2000/10/14 02:52:44  weimer
 * fixed a memory corruption problem in display (a use after a free)
 *
 * Revision 1.12  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.11  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.10  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.9  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.8  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.7  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.6  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.5  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.4  2000/09/03 21:06:31  wkiri
 * Now handles three different game types (and does different things).
 * Major changes in display.c to handle this.
 *
 * Revision 1.3  2000/09/03 18:40:23  wkiri
 * Cleaned up atris.c some more (high_score_check()).
 * Added variable for game type (defaults to MARATHON).
 *
 */


extern int Score[];

struct state_struct {
    int 	ai;
    int 	falling;
    int 	fall_speed; 
    int 	accept_input;
    int 	tetris_handling;
    int 	limbo;
    int 	other_in_limbo;
    int 	limbo_sent;
    int 	draw;
    Uint32	collide_time;	/* time when your piece merges with the rest */
    Uint32 	next_draw;
    Uint32 	draw_timeout;
    Uint32 	tv_next_fall;
    int 	fall_event_interval;
    Uint32 	tv_next_tetris;
    int 	tetris_event_interval;
    Uint32 	tv_next_ai_think;
    Uint32 	tv_next_ai_move;
    int		ai_interval;
    int 	ready_for_fast;
    int 	ready_for_rotate;
    int		seed;
    play_piece	cp, np;		
    void *	ai_state;
    /* these two are used by tetris_event() */
    int		check_result;
    int		num_lines_cleared;
} State[2];

/* one position structure per player */
struct pos_struct {
    int x;
    int y;
    int rot;
    int old_x;
    int old_y;
    int old_rot;
    Command move;
} pos[2];

Grid distract_grid[2];

/***************************************************************************
 *      paste_on_board()
 * Places the given piece on the board. Uses row-column (== grid)
 * coordinates. 
 *********************************************************************PROTO*/
void
paste_on_board(play_piece *pp, int col, int row, int rot, Grid *g)
{
    int i,j,c;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col; /* was + (screen_x / cs->w); */
		int t_y = j + row; /* was + (screen_y / cs->h); */
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    Debug("Serious consistency failure: dropping pieces.\n");
		    continue;
		}
		GRID_SET(*g,t_x,t_y,pp->colormap[c]);
		FALL_SET(*g,t_x,t_y,NOT_FALLING);
		if (t_x > 0) GRID_CHANGED(*g,t_x-1,t_y) = 1;
		if (t_y > 0) GRID_CHANGED(*g,t_x,t_y-1) = 1;
		if (t_x < g->w-1) GRID_CHANGED(*g,t_x+1,t_y) = 1;
		if (t_y < g->h-1) GRID_CHANGED(*g,t_x,t_y+1) = 1;
	}
    return;
}

/***************************************************************************
 *      screen_to_exact_grid_coords()
 * Converts screen coordinates to exact grid coordinates. Will abort the
 * program if you pass in unaligned coordinates!
 ***************************************************************************/
static void
screen_to_exact_grid_coords(Grid *g, int blockWidth, 
	int screen_x, int screen_y, int *row, int *col)
{
    screen_x -= g->board.x;
    screen_y -= g->board.y;

    Assert(screen_x % blockWidth == 0);
    Assert(screen_y % blockWidth == 0);

    *row = screen_y / blockWidth;
    *col = screen_x / blockWidth;

    return;
}

/***************************************************************************
 *      screen_to_grid_coords()
 * Converts screen coordinates to grid coordinates. Rounds "down". 
 ***************************************************************************/
void
screen_to_grid_coords(Grid *g, int blockWidth, 
	int screen_x, int screen_y, int *row, int *col)
{
    screen_x -= g->board.x;
    screen_y -= g->board.y;
    if (screen_x < 0) screen_x -= 19;	/* round up to negative #s */
    if (screen_y < 0) screen_y -= 19;	/* round up to negative #s */

    *row = screen_y / blockWidth;
    *col = screen_x / blockWidth;

    return;
}

/***************************************************************************
 *      valid_position()
 * Determines if the given position is valid. Uses row-column (== grid)
 * coordinates. Returns 0 if the piece would fall out of bounds or if
 * some solid part of the piece would fall over something already on the
 * the grid. Returns 1 otherwise (it is then safe to call
 * paste_on_board()). 
 *********************************************************************PROTO*/
int
valid_position(play_piece *pp, int col, int row, int rot, Grid *g)
{
    int i,j;

    /* 
     * We don't want this check because you can have col=-2 or whatnot if
     * your piece doesn't start on its leftmost border.
     *
     * if (col < 0 || col >= g->w || row < 0 || row >= g->h)
     *	return 0;
     */

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if (BITMAP(*pp->base,rot,i,j)) {
		int t_x = i + col; /* was + (screen_x / cs->w); */
		int t_y = j + row; /* was + (screen_y / cs->h); */
		if (t_x < 0 || t_y < 0 || t_x >= g->w || t_y >= g->h)
		    return 0;
		if (GRID_CONTENT(*g,t_x,t_y)) return 0;
	}
    return 1;
}

/***************************************************************************
 *      valid_screen_position()
 * Determines if the given position is valid. Uses screen coordinates.
 * Handles pieces that are not perfectly row-aligned. Pieces must still
 * be perfectly column aligned. 
 *
 * Returns 0 if the piece would fall out of bounds or if some solid part of
 * the piece would fall over something already on the the grid. Returns 1
 * otherwise (it is then safe to call paste_on_board()). 
 *********************************************************************PROTO*/
int
valid_screen_position(play_piece *pp, int blockWidth, Grid *g,
	int rot, int screen_x, int screen_y)
{
    int row, row2, col;

    screen_to_grid_coords(g, blockWidth, screen_x, screen_y, &row, &col);

    if (!valid_position(pp, col, row, rot, g))
	return 0;

    screen_to_grid_coords(g, blockWidth, screen_x, screen_y+19, &row2, &col);

    if (row == row2) return 1;	/* no need to recheck, you were aligned */
    else return valid_position(pp, col, row2, rot, g);
}

/***************************************************************************
 *      tetris_event()
 * Do some work associated with a collision. Needs the color style to draw
 * the grid.
 *************************************************************************/
int tetris_event(int *delay, int count, SDL_Surface * screen, 
	piece_style *ps, color_style *cs, sound_style *ss, Grid g[], int
	level, int fall_event_interval, int sock, int draw,
	int *blank, int *garbage, int P)
{
    if (count == 1) { /* determine if anything happened, sounds */
	int i;

	State[P].check_result = check_tetris(g);
	State[P].num_lines_cleared += State[P].check_result;

	if (State[P].check_result >= 3)
	    play_sound(ss,SOUND_CLEAR4,256);
	else for (i=0;i<State[P].check_result;i++)
	    play_sound(ss,SOUND_CLEAR1,256+6144*i);

	*delay = 1;
	return 2;

    } else if (count == 2) {	/* run gravity */
	int x,y;

	if (sock) { 
	    char msg = 'c'; /* WRW: send update */
	    send(sock,&msg,1,0);
	    send(sock,g[0].contents,sizeof(*g[0].contents)
		    * g[0].h * g[0].w,0); 
		    
	}
	draw_grid(screen,cs,&g[0],draw);

	/*
	 * recalculate falling, run gravity
	 */
	memcpy(g->temp,g->fall,g->w*g->h*sizeof(*(g->temp)));
	for (y=g->h-1;y>=0;y--) 
	    for (x=g->w-1;x>=0;x--) 
		FALL_SET(*g,x,y,UNKNOWN);
	run_gravity(&g[0]);
	memset(g->changed,0,g->h*g->w*sizeof(*(g->changed)));
	for (y=g->h-1;y>=0;y--) 
	    for (x=g->w-1;x>=0;x--) 
		if (TEMP_CONTENT(*g,x,y)!=FALL_CONTENT(*g,x,y)){
		    GRID_CHANGED(*g,x,y) = 1;
		    if (x > 0) GRID_CHANGED(*g,x-1,y) = 1;
		    if (y > 0) GRID_CHANGED(*g,x,y-1) = 1;
		    if (x < g->w-1) GRID_CHANGED(*g,x+1,y) = 1;
		    if (y < g->h-1) GRID_CHANGED(*g,x,y+1) = 1;
		}
	/* 
	 * check: did FALL_CONTENT change? 
	 */
	draw_grid(screen,cs,&g[0],draw);

	if (determine_falling(&g[0])) {
	    *delay = 1;
	    return 3;
	} else {
	    Score[P] += State[P].num_lines_cleared *
		State[P].num_lines_cleared * level;
	    if (sock) {
		char msg = 's'; /* WRW: send update */
		send(sock,&msg,1,0);
		send(sock,(char *)&Score[P],sizeof(Score[P]),0);
		if (State[P].num_lines_cleared >= 5) {
		    char msg = 'g'; /* WRW: send garbage! */
		    send(sock,&msg,1,0);
		    State[P].num_lines_cleared -= 4; /* might possibly also blank! */
		}
		if (State[P].num_lines_cleared >= 3) {
		    int i;
		    for (i=3;i<=State[P].num_lines_cleared;i++) {
			char msg = 'b'; /* WRW: send blanking! */
			send(sock,&msg,1,0);
		    }
		}
	    } else {
		if (State[P].num_lines_cleared >= 5) {
		    *garbage = 1;
		    State[P].num_lines_cleared -= 4;
		}
		if (State[P].num_lines_cleared >= 3) {
		    *blank = (State[P].num_lines_cleared - 2);
		}
	    }
	    draw_score(screen,P);
	    State[P].num_lines_cleared = 0;
	    return 0;
	}
    } else if (count >= 3 && count <= 22) {
	if (draw) 
	    draw_falling(screen, cs->w , &g[0], count - 2);

	/*
	*delay = max(fall_event_interval / 5,4);
	*/
	*delay = 4;
	return count + 1;
    } else if (count == 23) {
	fall_down(g);
	draw_grid(screen,cs,g,draw);
	if (run_gravity(g))
	    play_sound(ss,SOUND_THUD,0);
	/*
	*delay = max(fall_event_interval / 5,4);
	*/
	*delay = 4; 
	if (determine_falling(&g[0])) 
	    return 3;
	if (check_tetris(g))
	    return 1;
	else /* cannot be 0: we must redraw without falling */
	    return 2;
    }
    return 0;	/* not done yet */
}

/***************************************************************************
 *      do_blank()
 * Blank the visual screen of the given (local) player. 
 ***************************************************************************/
static void
do_blank(SDL_Surface *screen, sound_style *ss[2], Grid g[], int P)
{
    play_sound(ss[P],SOUND_GARBAGE1,1);
    if (State[P].draw) {
	State[P].next_draw = SDL_GetTicks() + 1000;
	State[P].draw_timeout = 1000;
	SDL_FillRect(screen, &g[P].board, 
		SDL_MapRGB(screen->format,32,32,32));
	SDL_UpdateSafe(screen, 1, &g[P].board);
    }  else {
	State[P].next_draw += 1000;
	State[P].draw_timeout += 1000;
    }
    { int i,j;
	for (j=0;j<g[0].h;j++)
	    for (i=0;i<g[0].w;i++)
		GRID_CHANGED(distract_grid[P],i,j) = 0;
    }
    State[P].draw = 0;
}

/***************************************************************************
 *      bomb_fun()
 * Function for the bomb special piece.
 ***************************************************************************/
static void
bomb_fun(int x, int y, Grid *g) 
{
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_SET(*g,x,y,REMOVE_ME);
}

static int most_common = 1;

/***************************************************************************
 *      colorkill_fun()
 * Function for the repainting special piece.
 ***************************************************************************/
static void
colorkill_recurse(int x, int y, Grid *g, int target_color)
{
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    if (GRID_CONTENT(*g,x,y) != target_color) return;
    GRID_SET(*g,x,y,REMOVE_ME);
    colorkill_recurse(x - 1, y, g, target_color);
    colorkill_recurse(x + 1, y, g, target_color);
    colorkill_recurse(x, y - 1, g, target_color);
    colorkill_recurse(x, y + 1, g, target_color);
}

static void
colorkill_fun(int x, int y, Grid *g)
{
    int c;
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    c = GRID_CONTENT(*g,x,y);
    if (c <= 1 || c == REMOVE_ME) return;
    GRID_SET(*g,x,y,REMOVE_ME);
    colorkill_recurse(x - 1, y, g, c);
    colorkill_recurse(x + 1, y, g, c);
    colorkill_recurse(x, y - 1, g, c);
    colorkill_recurse(x, y + 1, g, c);
}

/***************************************************************************
 *      repaint_fun()
 * Function for the repainting special piece.
 ***************************************************************************/
static void
repaint_fun(int x, int y, Grid *g) 
{
    int c;
    if ((x<0 || y<0 || x>=g->w || y>=g->h)) 
	return;
    GRID_CHANGED(*g,x,y) = 1;
    c = GRID_CONTENT(*g,x,y);
    if (c <= 1 || c == most_common) return;
    GRID_SET(*g,x,y,most_common);
    repaint_fun(x - 1, y, g);
    repaint_fun(x + 1, y, g);
    repaint_fun(x, y - 1, g);
    repaint_fun(x, y + 1, g);
}

/***************************************************************************
 * 	push_down()
 * Teleport all of the pieces just below this special piece as far down as
 * they can go in their column. 
 ***************************************************************************/
static void
push_down(play_piece *pp, int col, int row, int rot, Grid *g,
	void (*fun)(int, int, Grid *))
{
    int i,j,c;
    int place_y, look_y;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col;
		int t_y = j + row;
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		GRID_SET(*g,t_x,t_y,REMOVE_ME);
		look_y = t_y + 1;
		if (look_y >= g->h) continue;
		if (!GRID_CONTENT(*g,t_x,look_y)) continue;
		/* OK, try to move look_y down as far as possible */
		for (place_y = g->h-1; 
			place_y > look_y &&
			GRID_CONTENT(*g,t_x,place_y) != 0;
			place_y --)
		    ;
		if (place_y == look_y) continue;
		/* otherwise, valid swap! */
		if (place_y < 0 || place_y >= g->h) 
		    continue;
		if (look_y < 0 || look_y >= g->h) 
		    continue;
		GRID_SET(*g,t_x,place_y, GRID_CONTENT(*g,t_x,look_y));
		GRID_SET(*g,t_x,look_y, REMOVE_ME);
	    }
}

/***************************************************************************
 *      find_on_board()
 * Finds where on the board a piece would go: used by special piece
 * handling routines. 
 ***************************************************************************/
static void
find_on_board(play_piece *pp, int col, int row, int rot, Grid *g,
	void (*fun)(int, int, Grid *))
{
    int i,j,c;

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col;
		int t_y = j + row; 
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		GRID_SET(*g,t_x,t_y,REMOVE_ME);
	    }

    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) 
	    if ((c=BITMAP(*pp->base,rot,i,j))) {
		int t_x = i + col; 
		int t_y = j + row;
		if ((t_x<0 || t_y<0 || t_x>=g->w || t_y>=g->h)) {
		    continue;
		}
		fun(t_x - 1, t_y, g);
		fun(t_x + 1, t_y, g);
		fun(t_x, t_y - 1, g);
		fun(t_x, t_y + 1, g);
	}
    return;
}

/***************************************************************************
 *      most_common_color()
 * Finds the most common (non-zero, non-garbage) color on the board. If
 * none, returns the garbage color. 
 ***************************************************************************/
static void
most_common_color(Grid *g)
{
    int count[256];
    int x,y,c;
    int max, max_count;

    memset(count,0, sizeof(count));
    for (x=0;x<g->w;x++)
	for (y=0;y<g->h;y++) {
	    c = GRID_CONTENT(*g,x,y);
	    if (c > 1) 
		count[c]++;
	}
    max = 1;
    max_count = 0;
    for (x=2;x<256;x++) 
	if (count[x] > max_count) {
	    max = x;
	    max_count = count[x];
	}
    most_common = max;
    return;
}

/***************************************************************************
 *      handle_special()
 * Change the state of the grid based on the magical special piece ...
 * Look in pos[P] for the current position.
 *********************************************************************PROTO*/
void
handle_special(play_piece *pp, int row, int col, int rot, Grid *g,
	sound_style *ss)
{
    switch (pp->special) {
	case No_Special: break;
	case Special_Bomb: 
			 find_on_board(pp, col, row, rot, g, bomb_fun);
			 if (ss) 
			     play_sound(ss,SOUND_CLEAR1,256);
			 break;
	case Special_Repaint: 
			 most_common_color(g);
			 find_on_board(pp, col, row, rot, g, repaint_fun);
			 if (ss) 
			     play_sound(ss,SOUND_GARBAGE1,256);
			 break;
	case Special_Pushdown: 
			 push_down(pp, col, row, rot, g, repaint_fun);
			 if (ss) 
			     play_sound(ss,SOUND_THUD,256*2);
			 break;
	case Special_Colorkill: 
			 find_on_board(pp, col, row, rot, g, colorkill_fun);
			 if (ss) 
			     play_sound(ss,SOUND_CLEAR1,256);
			 break;

    }
}

/***************************************************************************
 *      do_pause()
 * Change the pause status of the local player.
 ***************************************************************************/
static void
do_pause(int paused, Uint32 tv_now, int *pause_begin_time, int *tv_start)
{
    int i;
    draw_pause(paused);
    if (!paused) {
	/* fixup times */
	*tv_start += (tv_now - *pause_begin_time);
	for (i=0; i<2; i++) {
	    State[i].collide_time += (tv_now - *pause_begin_time);
	    State[i].next_draw += (tv_now - *pause_begin_time);
	    State[i].tv_next_fall += (tv_now - *pause_begin_time);
	    State[i].tv_next_tetris += (tv_now - *pause_begin_time);
	    State[i].tv_next_ai_think += (tv_now - *pause_begin_time);
	    State[i].tv_next_ai_move += (tv_now - *pause_begin_time);
	}
    } else {
	*pause_begin_time = tv_now;
    }
}

/***************************************************************************
 *      place_this_piece()
 * Given that the player's current piece structure is already chosen, try
 * to place it near the top of the board. this may involve shifting it up 
 * a bit or rotating it or something. 
 *
 * Returns 0 on success. 
 ***************************************************************************/
static int
place_this_piece(int P, int blockWidth, Grid g[])
{
    int Y, R;
    /* we'll try Y adjustments from -2 to 0 and rotations from 0 to 3 */

    pos[P].x = pos[P].old_x = g[P].board.x + g[P].board.w / 2;
    for (Y = 0; Y >= -2 ; Y --) {
	for (R = 0; R <= 3; R++) {
	    pos[P].y = pos[P].old_y = g[P].board.y + (blockWidth * Y);
	    pos[P].old_rot = pos[P].rot = R;
	    if (valid_screen_position(&State[P].cp, blockWidth,
			&g[P], pos[P].rot, pos[P].x, pos[P].y)) {
		return 0;
	    }
	}
    }
    return 1;	/* no valid position! */
}

/***************************************************************************
 *      event_loop()
 * The main event-processing dispatch loop. 
 *
 * Returns 0 on a successful game completion, -1 on a [single-user] quit.
 *********************************************************************PROTO*/
#define		NO_PLAYER	0
#define		HUMAN_PLAYER	1
#define		AI_PLAYER	2
#define		NETWORK_PLAYER	3
int
event_loop(SDL_Surface *screen, piece_style *ps, color_style *cs[2], 
	sound_style *ss[2], Grid g[], int level[2], int sock,
	int *seconds_remaining, int time_is_hard_limit,
	int adjust[], int (*handle)(const SDL_Event *), 
	int seed, int p1, int p2, AI_Player *AI[2])
{
    SDL_Event event;
    Uint32 tv_now, tv_start; 
    int NUM_PLAYER = 0;
    int NUM_KEYBOARD = 0;
    int last_seconds = -1;
    int minimum_fall_event_interval = 100;
    int paused = 0;
    Uint32 pause_begin_time = 0;

    /* measured in milliseconds, 25 frames per second:
     * 1 frame = 40 milliseconds
     */
    int blockWidth = cs[0]->w;
    int i,j,P, Q;

    SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY/Options.key_repeat_delay,
	    SDL_DEFAULT_REPEAT_INTERVAL/2);

    if (gametype != DEMO)
	stop_all_playing(); 

    memset(pos, 0, sizeof(pos[0]) * 2);
    memset(State, 0, sizeof(State[0]) * 2);

    switch (p1) {
	case NO_PLAYER: Assert(!handle); break;
	case HUMAN_PLAYER: Assert(!handle); NUM_PLAYER++; NUM_KEYBOARD++; break;
	case AI_PLAYER: State[0].ai = 1; NUM_PLAYER++; break;
	case NETWORK_PLAYER: PANIC("Cannot have player 1 over the network!");
    }
    switch (p2) {
	case NO_PLAYER: break;
	case HUMAN_PLAYER: Assert(!handle); NUM_PLAYER++; NUM_KEYBOARD++; break;
	case AI_PLAYER: State[1].ai = 1; NUM_PLAYER++; break;
	case NETWORK_PLAYER: Assert(sock); break;
    }
    Assert(NUM_PLAYER >= 1 && NUM_PLAYER <= 2);

    tv_start = tv_now = SDL_GetTicks();
    tv_start += *seconds_remaining * 1000; 

    for (P=0; P<NUM_PLAYER; P++) {
	State[P].falling = 1;
	State[P].fall_speed = 1;
	State[P].tetris_handling = 0;
	State[P].accept_input = 1;
	State[P].limbo = 0;
	State[P].draw = 1;
	State[P].other_in_limbo = 0;
	State[P].next_draw = 0;
	State[P].limbo_sent = 0;
	State[P].cp = generate_piece(ps, cs[P], seed);
	State[P].np = generate_piece(ps, cs[P], seed+1);
	State[P].seed = seed+2;
	State[P].ready_for_fast = 1;
	State[P].ready_for_rotate = 1;

	draw_next_piece(screen, ps, cs[P], &State[P].cp, &State[P].np, P);

	adjust[P] = -1;

	if (SPEED_LEVEL(level[P]) <= 7)
	    State[P].fall_event_interval = 45 - SPEED_LEVEL(level[P]) * 5;
	else 
	    State[P].fall_event_interval = 16 - SPEED_LEVEL(level[P]);
	if (State[P].fall_event_interval < 1)
	    State[P].fall_event_interval = 1;

	if (State[P].fall_event_interval < minimum_fall_event_interval)
	    minimum_fall_event_interval = State[P].fall_event_interval;

	State[P].tv_next_fall = tv_now + State[P].fall_event_interval;

	if (place_this_piece(P, blockWidth, g)) {
	    /* failed to place piece initially ... */
	    pos[P].x = pos[P].old_x = g[P].board.x + g[P].board.w / 2;
	    pos[P].y = pos[P].old_y = g[P].board.y ;
	    pos[P].rot = pos[P].old_rot = 0;
	}

	if (State[P].ai) {
	    State[P].tv_next_ai_think = tv_now;
	    State[P].tv_next_ai_move = tv_now;
	    if (gametype == DEMO || gametype == AI_VS_AI ||
		    AI[P]->delay_factor == 0) {
		State[P].ai_interval = State[P].fall_event_interval;
		if (State[P].ai_interval > 15)
		    State[P].ai_interval = 15;
	    } else {
		if (AI[P]->delay_factor < 1)
		    AI[P]->delay_factor = 1;
		if (AI[P]->delay_factor > 100)
		    AI[P]->delay_factor = 100;
		State[P].ai_interval = AI[P]->delay_factor;
	    }
	    State[P].ai_state = AI[P]->reset(State[P].ai_state, &g[P]);
	}
    }


    /* generate the fake-out grid: shown when the opponent does something
     * good! */
    distract_grid[0] = generate_board(g[0].w,g[0].h,g[0].h-2);
    distract_grid[0].board = g[0].board;

    SeedRandom(seed);
    for (i=0;i<g[0].w;i++)
	for (j=0;j<g[0].h;j++) {
	    GRID_SET(distract_grid[0],i,j,ZEROTO(cs[0]->num_color));
	}
    if (NUM_PLAYER == 2) {
	distract_grid[1] = generate_board(g[1].w,g[1].h,g[1].h-2);
	distract_grid[1].board = g[1].board;
	for (i=0;i<g[1].w;i++)
	    for (j=0;j<g[1].h;j++) {
		GRID_SET(distract_grid[1],i,j,ZEROTO(cs[1]->num_color));
	    }
    }

    if (sock) { 
	char msg = 'c'; /* WRW: send update */
	send(sock,&msg,1,0);
	send(sock,g[0].contents,sizeof(*g[0].contents)
		* g[0].h * g[0].w,0); 
		
    }

    draw_clock(0);

    draw_grid(screen,cs[0],&g[0],1);
    draw_score(screen, 0);
    if (NUM_PLAYER == 2) {
	draw_grid(screen,cs[1],&g[1],1);
	draw_score(screen,1);
    }
    if (sock)
	draw_score(screen, 1);

    /* 
     * Major State-Machine Event Loop
     */

    P = 0;

    while (1) { 

	if (NUM_PLAYER == 2)
	    P = !P;

	tv_now = SDL_GetTicks();

	/* update the on-screen clock */
	if (tv_start >= tv_now)
	    * seconds_remaining = (tv_start - tv_now) / 1000;
	else
	    * seconds_remaining = - ((tv_now - tv_start) / 1000);

	if (*seconds_remaining != last_seconds && !paused) {
	    last_seconds = *seconds_remaining;
	    draw_clock(*seconds_remaining);
	    if (last_seconds <= 30 && last_seconds >= 0) {
		play_sound_unless_already_playing(ss[0],SOUND_CLOCK,0);
		if (NUM_PLAYER == 2) 
		    play_sound_unless_already_playing(ss[1],SOUND_CLOCK,0);
	    }
	}

	/* check for time-out */
	if (*seconds_remaining < 0 && time_is_hard_limit && !paused) { 
	    play_sound(ss[0],SOUND_LEVELDOWN,0);
	    adjust[0] = ADJUST_DOWN;
	    if (NUM_PLAYER == 2) {
		play_sound(ss[1],SOUND_LEVELDOWN,0);
		adjust[1] = ADJUST_DOWN;
	    }
	    stop_playing_sound(ss[0],SOUND_CLOCK);
	    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
	    return 0;
	} 

	/*
	 * 	Visual Events
	 */
	if (!State[P].draw && tv_now > State[P].next_draw && !paused) {
	    int i,j;
	    State[P].draw = 1;
	    for (i=0;i<g[P].w;i++)
		for (j=0;j<g[P].h;j++) {
		    GRID_CHANGED(g[P],i,j) = 1;
		    if (GRID_CONTENT(g[P],i,j) == 0)
			GRID_SET(g[P],i,j,REMOVE_ME);
		}
	    draw_grid(screen,cs[P],&g[P],1);
	} else if (!State[P].draw) {
	    int delta = State[P].next_draw - tv_now;
	    int amt = g[P].h - ((g[P].h * delta) / State[P].draw_timeout);
	    int i,j;
	    j = amt - 1;
	    if (j < 0) j = 0;
	    for (i=0;i<g[P].w;i++)
		GRID_CHANGED(distract_grid[P],i,j) = 1;
	    draw_grid(screen,cs[P],&distract_grid[P],1);
	}

	/* 
	 *	Falling Events
	 */

	if (State[P].falling && tv_now >= State[P].tv_next_fall && !paused) {
	    int try;
	    int we_fell = 0;

#if DEBUG
	    if (tv_now > State[P].tv_next_fall) 
		Debug("Fall: %d %d\n", tv_now, tv_now - State[P].tv_next_fall);
#endif

	    /* ok, we had a falling event */
	    do {
		State[P].tv_next_fall += State[P].fall_event_interval;
	    } while (State[P].tv_next_fall <= tv_now); 

	    for (try = State[P].fall_speed; try > 0; try--)
		if (valid_screen_position(&State[P].cp,blockWidth,&g[P],pos[P].rot,pos[P].x,pos[P].y+try)) {
		    pos[P].y += try;
		    State[P].fall_speed = try;
		    try = 0;
		    we_fell = 1;
		}
	    if (!we_fell) {
		if (!State[P].collide_time) {
		    State[P].collide_time = tv_now + 
			(Options.long_settle_delay ? 400 : 200);
		    continue; /* don't fall */
		}
		if (tv_now < State[P].collide_time)
		    continue; /* don't fall */
		/* do fall! */
	    } else continue;
	    /* collided! */

	    State[P].collide_time = 0;

	    play_sound(ss[P],SOUND_THUD,0);

	    /* this would only come into play if you were halfway
	     * between levels and suddenly someone added garbage */
	    while (!valid_screen_position(&State[P].cp,blockWidth,&g[P],pos[P].rot,pos[P].x,pos[P].y) && pos[P].y > 0) 
		pos[P].y--;

	    if (State[P].draw) 
		draw_play_piece(screen, cs[P], &State[P].cp, pos[P].old_x, pos[P].old_y, pos[P].old_rot,
			&State[P].cp, pos[P].x, pos[P].y, pos[P].rot);

	    if (State[P].cp.special != No_Special) {
		/* handle special powers! */
		int row, col;
		screen_to_exact_grid_coords(&g[P], blockWidth, 
			pos[P].x, pos[P].y, &row, &col);
		handle_special(&State[P].cp, row, col, pos[P].rot, &g[P], ss[P]);
	    } else { 
		/* paste the piece on the board */
		int row, col;
		screen_to_exact_grid_coords(&g[P], blockWidth, 
			pos[P].x, pos[P].y, &row, &col);
		paste_on_board(&State[P].cp, col, row, pos[P].rot, &g[P]);
	    }

	    if (sock) { 
		char msg = 'c'; /* WRW: send update */
		send(sock,&msg,1,0);
		send(sock,g[P].contents,sizeof(*g[P].contents)
			* g[P].h * g[P].w,0); 
	    }
	    draw_grid(screen,cs[P],&g[P],State[P].draw);

	    /* state change */
	    State[P].falling = 0;
	    State[P].fall_speed = 0;
	    State[P].tetris_handling = 1;
	    State[P].accept_input = 0;
	    State[P].tv_next_tetris = tv_now;
	}
	/* 
	 *	Tetris Clear Events
	 */
	if (State[P].tetris_handling != 0 && tv_now >= State[P].tv_next_tetris && !paused) {
	    int blank = 0, garbage = 0;

#if DEBUG 
	    if (tv_now >= State[P].tv_next_tetris) 
		Debug("Tetr: %d %d (%d)\n", tv_now, tv_now -
			State[P].tv_next_tetris, State[P].tetris_handling);
#endif

	    State[P].tetris_handling = tetris_event(
		    &State[P].tetris_event_interval, 
		    State[P].tetris_handling, screen, ps, cs[P], ss[P], &g[P], 
		    level[P], minimum_fall_event_interval, sock, State[P].draw,
		    &blank, &garbage, P);

	    if (NUM_PLAYER == 2) {
		if (blank)
		    do_blank(screen, ss, g, !P);
		if (garbage) {
		    add_garbage(&g[!P]);
		    play_sound(ss[!P],SOUND_GARBAGE1,1);
		    draw_grid(screen,cs[!P],&g[!P],State[!P].draw);
		}
	    }

	    tv_now = SDL_GetTicks();
	    do {  /* just in case we're *way* behind */
		State[P].tv_next_tetris += State[P].tetris_event_interval;
	    } while (State[P].tv_next_tetris < tv_now); 

	    if (State[P].tetris_handling == 0) { /* state change */
		/* Time for your next piece ...
		 * Yujia points out that we should try a little harder to
		 * fit your piece on the board. 
		 */
		State[P].cp = State[P].np;
		State[P].np = generate_piece(ps, cs[P], State[P].seed++);
		draw_next_piece(screen, ps, cs[P], 
			&State[P].cp, &State[P].np, P);

		if (place_this_piece(P, blockWidth, g)) {
		    /* failed to place piece */
you_lose: 
		    play_sound(ss[P],SOUND_LEVELDOWN,0);
		    adjust[P] = ADJUST_DOWN;
		    if (NUM_PLAYER == 2 && !sock)
			adjust[!P] = ADJUST_SAME;
		    if (sock == 0) {
			stop_playing_sound(ss[0],SOUND_CLOCK);
			if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
			return 0;
		    }
		    /*
		    Debug("Entering Limbo: adjust down.\n");
		    */
		    State[P].falling = 0;
		    State[P].fall_speed = 0;
		    State[P].tetris_handling = 0;
		    State[P].accept_input = 0;
		    State[P].limbo = 1;
		} else {
		    int x,y,count = 0;
		    if (State[P].ai) 
			State[P].ai_state =
			    AI[P]->reset(State[P].ai_state, &g[P]);
		    for (y=0;y<g->h;y++)
			for (x=0;x<g->w;x++)
			    if (GRID_CONTENT(g[P],x,y) == 1)
				count++;
		    if (count == 0) {
you_win: 
			play_sound(ss[P],SOUND_LEVELUP,256);
			if (*seconds_remaining <= 0) { 
			    adjust[P] = ADJUST_SAME;
			    if (NUM_PLAYER == 2 && !sock)
				adjust[!P] = ADJUST_DOWN;
			} else {
			    adjust[P] = ADJUST_UP;
			    if (NUM_PLAYER == 2 && !sock)
				adjust[!P] = ADJUST_SAME;
			}
			if (sock == 0) {
			    stop_playing_sound(ss[0],SOUND_CLOCK);
			    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
			    return 0;
			}
			/*
			Debug("Entering Limbo: you win, adjust ?/?.\n");
			*/
			State[P].falling = 0;
			State[P].fall_speed = 0;
			State[P].tetris_handling = 0;
			State[P].accept_input = 0;
			State[P].limbo = 1;
		    } else {
			/* keep playing */
			State[P].falling = 1;
			State[P].fall_speed = 1;
			State[P].tetris_handling = 0;
			State[P].accept_input = 1;
			State[P].tv_next_fall = SDL_GetTicks() + 
			    State[P].fall_event_interval;
		    }
		}
	    } 
	} /* end: handle tetris events */

	/* 
	 *	AI Events
	 */
	if (State[P].ai && tv_now >= State[P].tv_next_ai_think && !paused) {
	    int row, col;
#ifdef AI_THINK_TIME
	    Uint32 tv_before = SDL_GetTicks();
#endif

	    screen_to_grid_coords(&g[P], blockWidth, pos[P].x, pos[P].y, &row, &col);

	    /* simulate blanked screens */
	    if (State[P].draw) 
		AI[P]->think(State[P].ai_state, &g[P], &State[P].cp, &State[P].np, col, row, pos[P].rot);

#ifdef AI_THINK_TIME
	    tv_now = SDL_GetTicks();
	    if (tv_now > tv_before + 1)
		Debug("AI[%s] took too long in think() [%d ticks].\n",
			AI[P]->name, tv_now - tv_before);
#endif

	    do {  /* just in case we're *way* behind */
		State[P].tv_next_ai_think += 
		    State[P].ai_interval;
	    } while (State[P].tv_next_ai_think < tv_now); 
	}
	if (State[P].ai && State[P].accept_input &&
		tv_now >= State[P].tv_next_ai_move && !paused) {
	    int row, col;
#ifdef AI_THINK_TIME
	    Uint32 tv_before = SDL_GetTicks();
#endif

	    screen_to_grid_coords(&g[P], blockWidth, pos[P].x, pos[P].y, &row, &col);
	    pos[P].move =
		AI[P]->move(State[P].ai_state, &g[P], &State[P].cp, &State[P].np, 
			col, row, pos[P].rot);
#ifdef AI_THINK_TIME
	    tv_now = SDL_GetTicks();
	    if (tv_now > tv_before + 1)
		Debug("AI[%s] took too long in move() [%d ticks].\n",
			AI[P]->name, tv_now - tv_before);
#endif

	    do {  /* just in case we're *way* behind */
		State[P].tv_next_ai_move += 
		    State[P].ai_interval * 5;
	    } while (State[P].tv_next_ai_move < tv_now); 
	}

	/* 
	 * 	User Interface Events 
	 */

	if (SDL_PollEvent(&event)) {

	    /* special menu handling! */
	    if (handle) {
		if (handle(&event)) {
		    return -1;
		}
	    } else switch (event.type) {
		case SDL_KEYUP:
		    /* "down" will not affect you again until you release
		     * the down key and press it again */
		    if (event.key.keysym.sym == SDLK_DOWN) {
			State[1].ready_for_fast = 1;
			if (NUM_KEYBOARD == 1)
			    State[0].ready_for_fast = 1;
		    }
		    else if (event.key.keysym.sym == SDLK_UP) {
			State[1].ready_for_rotate = 1;
			if (NUM_KEYBOARD == 1)
			    State[0].ready_for_rotate = 1;
		    }
		    else if (event.key.keysym.sym == SDLK_w)
			State[0].ready_for_rotate = 1;
		    else if (event.key.keysym.sym == SDLK_s)
			State[0].ready_for_fast = 1;
		    else if (event.key.keysym.sym == SDLK_1) {
			P = 0;
			goto you_win;
		    }
		    else if (event.key.keysym.sym == SDLK_2) {
			P = 0;
			goto you_lose;
		    }
		    else if (event.key.keysym.sym == SDLK_3) {
			P = 1;
			goto you_win;
		    }
		    else if (event.key.keysym.sym == SDLK_4) {
			P = 1;
			goto you_lose;
		    } else if (event.key.keysym.sym == SDLK_p && gametype != DEMO) {
			/* Pause it! */
			tv_now = SDL_GetTicks();
			paused = !paused;
			if (sock) { 
			    char msg = 'p'; /* WRW: send pause update */
			    send(sock,&msg,1,0);
			}
			do_pause(paused, tv_now, &pause_begin_time, &tv_start);
		    }

		    break;

		case SDL_KEYDOWN:
		    {
			int ks = event.key.keysym.sym;
			Q = -1;

			/* keys for P=0 */
			if (ks == SDLK_UP || ks == SDLK_DOWN ||
				ks == SDLK_RIGHT || ks == SDLK_LEFT) {
			    Q = 1;
			} else if (ks == SDLK_w || ks == SDLK_s ||
				ks == SDLK_a || ks == SDLK_d) {
			    Q = 0;
			} else if (ks == SDLK_q) {
			    if (sock == 0) {
				adjust[0] = -1;
				if (NUM_PLAYER == 2)
				    adjust[1] = -1;
				stop_playing_sound(ss[0],SOUND_CLOCK);
				if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
				return -1;
			    } else {
				/*
				Debug("Entering Limbo: adjust down.\n");
				*/
				State[0].falling = 0;
				State[0].fall_speed = 0;
				State[0].tetris_handling = 0;
				State[0].accept_input = 0;
				State[0].limbo = 1;
				adjust[0] = ADJUST_DOWN;
			    }
			} else if ((ks == SDLK_RETURN) && 
                            ((event.key.keysym.mod & KMOD_LCTRL) ||
                             (event.key.keysym.mod & KMOD_RCTRL))) {
                          SDL_WM_ToggleFullScreen(screen);
                          break; 
                        } else break;
			if (NUM_KEYBOARD == 1) Q = 0;
			else if (NUM_KEYBOARD < 1) break;
			/* humans cannot modify AI moves! */

			Assert(Q == 0 || Q == 1);

			if (event.key.keysym.sym != SDLK_DOWN &&
				event.key.keysym.sym != SDLK_s)
			    State[Q].fall_speed = 1;
			if (!State[Q].accept_input) {
			    break;
			}
		    /* only if we are accepting input */
			switch (event.key.keysym.sym) {
			    case SDLK_UP: case SDLK_w: 
				if (State[Q].ready_for_rotate)
				    pos[Q].move = MOVE_ROTATE;
				break;
			    case SDLK_DOWN: case SDLK_s: 
				if (State[Q].ready_for_fast)
				    pos[Q].move = MOVE_DOWN;
				break;
			    case SDLK_LEFT: case SDLK_a: 
				pos[Q].move = MOVE_LEFT; break;
			    case SDLK_RIGHT: case SDLK_d: 
				pos[Q].move = MOVE_RIGHT; break;
			    default: 
				PANIC("unknown keypress");
			}
		    }
		    break;
		case SDL_QUIT:
		    Debug("Window-manager exit request.\n");
		    adjust[0] = -1;
		    if (NUM_PLAYER == 2)
			adjust[1] = -1;
		    stop_playing_sound(ss[0],SOUND_CLOCK);
		    if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		    return -1;
		case SDL_SYSWMEVENT:
		    break;
	    } /* end: switch (event.type) */
	} 

	/* 
	 *	Handle Movement 
	 */
	for (Q=0;Q<NUM_PLAYER;Q++) {
	    switch (pos[Q].move) {
		case MOVE_ROTATE: 
		    State[Q].ready_for_rotate = 0;
		    if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,pos[Q].x,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x-blockWidth,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].x -= blockWidth;
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x+blockWidth,pos[Q].y)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].x += blockWidth;
			State[Q].collide_time = 0;
		    } else if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],(pos[Q].rot+1)%4,
				pos[Q].x,pos[Q].y+blockWidth)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].y += blockWidth;
			State[Q].collide_time = 0;
		    } else if (Options.upward_rotation &&
			    valid_screen_position(&State[Q].cp,blockWidth,
				&g[Q],(pos[Q].rot+1)%4, pos[Q].x,
				pos[Q].y-blockWidth)) {
			pos[Q].rot = (pos[Q].rot+1)%4; 
			pos[Q].y -= blockWidth;
			State[Q].collide_time = 0;
		    }
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_LEFT:
		    for (i=0;i<10;i++) 
			if (valid_screen_position(&State[Q].cp,blockWidth,
				    &g[Q],pos[Q].rot,
				    pos[Q].x-blockWidth,
				    pos[Q].y+i)) {
			    pos[Q].x -= blockWidth;
			    pos[Q].y += i;
			    State[Q].collide_time = 0;
			    break;
			}
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_RIGHT:
		    for (i=0;i<10;i++)
			if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],pos[Q].rot,
				    pos[Q].x+blockWidth,pos[Q].y+i)){
			    pos[Q].x += blockWidth;
			    pos[Q].y += i;
			    State[Q].collide_time = 0;
			    break;
			}
		    pos[Q].move = MOVE_NONE;
		    break;
		case MOVE_DOWN:
		    if (valid_screen_position(&State[Q].cp,blockWidth,&g[Q],pos[Q].rot,
				pos[Q].x, pos[Q].y+blockWidth))
			pos[Q].y+=blockWidth;
		    State[Q].fall_speed = 20;
		    State[Q].ready_for_fast = 0;
		    pos[Q].move = MOVE_NONE;
		    break;
		default: 
		    break;
	    }
	} /* endof: for each player, check move */


	if (State[P].falling && !paused) { 
	    if (State[P].draw) 
		if (pos[P].old_x != pos[P].x || pos[P].old_y != pos[P].y || pos[P].old_rot != pos[P].rot) {
		    draw_play_piece(screen, cs[P], &State[P].cp, pos[P].old_x, pos[P].old_y, pos[P].old_rot,
			    &State[P].cp, pos[P].x, pos[P].y, pos[P].rot);
		    pos[P].old_x = pos[P].x; pos[P].old_y = pos[P].y; pos[P].old_rot = pos[P].rot;
		}
	}

	/* network connection */
	if (sock) {
	    fd_set read_fds;
	    struct timeval timeout = { 0, 0 };
	    int retval;

	    Assert(P == 0);

	    do { 
		FD_ZERO(&read_fds);
		FD_SET(sock,&read_fds);
#if HAVE_SELECT || HAVE_WINSOCK_H
		retval = select(sock+1, &read_fds, NULL, NULL, &timeout);
#else
#warning	"Since you do not have select(), networking play will fail."
		retval = 0;
#endif
		if (retval > 0) {
		    char msg;
		    if (recv(sock,&msg,1,0) != 1) {
			Debug("WARNING: Other player has left?\n");
			close(sock);
			sock = 0;
			retval = 0;
		    } else {
			switch (msg) {
			    case 'b': 
				do_blank(screen, ss, g, P);
			    break;
			    case 'p':
				paused = !paused;
				tv_now = SDL_GetTicks();
				do_pause(paused, tv_now, &pause_begin_time, &tv_start);
			    break; 

			    case ADJUST_DOWN: /* other play in limbo */
			    case ADJUST_SAME:
			    case ADJUST_UP:
				State[P].other_in_limbo = 1;
				adjust[!P] = msg;
				break;

			    case 'g': 
				add_garbage(&g[P]);
				play_sound(ss[P],SOUND_GARBAGE1,1);
				draw_grid(screen,cs[P],&g[P],State[P].draw);
				      break;
			    case 's':
				  recv(sock,(char *)&Score[1], sizeof(Score[1]),0);
				  draw_score(screen, 1);
				  break;
			    case 'c':  { int i,j;
				      memcpy(g[!P].temp,g[!P].contents,
					      sizeof(*g[0].temp) *
					      g[!P].w * g[!P].h);
				      recv(sock,g[!P].contents,
					      sizeof(*g[!P].contents) *
					      g[!P].w * g[!P].h,0);
				      for (i=0;i<g[!P].w;i++)
					  for (j=0;j<g[!P].h;j++)
					      if (GRID_CONTENT(g[!P],i,j) != TEMP_CONTENT(g[1],i,j)){
						  GRID_CHANGED(g[!P],i,j)=1;
						  if (i > 0) GRID_CHANGED(g[!P],i-1,j) = 1;
						  if (j > 0) GRID_CHANGED(g[!P],i,j-1) = 1;
						  if (i < g[!P].w-1) GRID_CHANGED(g[!P],i+1,j) = 1;
						  if (i < g[!P].h-1) GRID_CHANGED(g[!P],i,j+1) = 1;
						  if (GRID_CONTENT(g[!P],i,j) == 0) GRID_SET(g[!P],i,j,REMOVE_ME);
						  }
				      draw_grid(screen,cs[!P],&g[!P],1);
				       }
				      break;
			    default: break;
			}
		    }
		}
	    } while (retval > 0 && 
		    !(State[P].limbo && State[P].other_in_limbo));

	    /* limbo handling */
	    if (State[P].limbo && State[P].other_in_limbo) {
		Assert(adjust[0] != -1 && adjust[1] != -1);
		stop_playing_sound(ss[0],SOUND_CLOCK);
		if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		return 0;
	    } else if (State[P].limbo && !State[P].other_in_limbo &&
		    !State[P].limbo_sent) {
		char msg = adjust[P]; /* WRW: send update */
		State[P].limbo_sent = 1;
		send(sock,&msg,1,0);
	    } else if (!State[P].limbo && State[P].other_in_limbo) {
		char msg; /* WRW: send update */
		/* hmm, other guy is done ... */
		if (adjust[!P] == ADJUST_UP || adjust[!P] == ADJUST_SAME) {
		    if (*seconds_remaining > 0) 
			adjust[P] = ADJUST_SAME;
		    else
			adjust[P] = ADJUST_DOWN;
		} else if (adjust[!P] == ADJUST_DOWN) {
		    adjust[P] = ADJUST_SAME;
		}
		/*
		Debug("Entering Limbo: adjust same/down.\n");
		*/
		State[P].falling = 0;
		State[P].fall_speed = 0;
		State[P].tetris_handling = 0;
		State[P].accept_input = 0;
		State[P].limbo = 1;
		State[P].limbo_sent = 1;
		msg = adjust[P];
		send(sock,&msg,1,0);
		stop_playing_sound(ss[0],SOUND_CLOCK);
		if (NUM_PLAYER == 2) stop_playing_sound(ss[1],SOUND_CLOCK);
		return 0;
	    }
	}
	if (paused) {
	    atris_run_flame();
	}

	tv_now = SDL_GetTicks();
	{
	    Uint32 least = State[0].tv_next_fall;

	    if (State[0].tetris_handling && State[0].tv_next_tetris < least)
		least = State[0].tv_next_tetris;
	    if (State[0].ai && State[0].tv_next_ai_think < least)
		least = State[0].tv_next_ai_think;
	    if (State[0].ai && State[0].tv_next_ai_move < least)
		least = State[0].tv_next_ai_move;
	    if (NUM_PLAYER == 2) {
		if (State[1].tv_next_fall < least)
		    least = State[1].tv_next_tetris;
		if (State[1].tetris_handling && State[1].tv_next_tetris < least)
		    least = State[1].tv_next_tetris;
		if (State[1].ai && State[1].tv_next_ai_think < least)
		    least = State[1].tv_next_ai_think;
		if (State[1].ai && State[1].tv_next_ai_move < least)
		    least = State[1].tv_next_ai_move;
	    }

	    if (least >= tv_now + 4 && !SDL_PollEvent(NULL)) {
		/* hey, we could sleep for two ... */
		if (State[0].ai || State[1].ai) {
		    SDL_Delay(1);
		    if (State[0].ai && State[0].draw) {
			int row, col;
			screen_to_grid_coords(&g[0], blockWidth, pos[0].x, pos[0].y, &row, &col);
			AI[0]->think(State[0].ai_state, &g[0], &State[0].cp, &State[0].np, col, row, pos[0].rot);
		    } else SDL_Delay(1);
		    if (State[1].ai && State[1].draw) {
			int row, col;
			screen_to_grid_coords(&g[1], blockWidth, pos[1].x, pos[1].y, &row, &col);
			AI[1]->think(State[1].ai_state, &g[1], &State[1].cp, &State[1].np, col, row, pos[1].rot);
		    } else SDL_Delay(1);
		} else SDL_Delay(2);
	    } else if (least > tv_now && !SDL_PollEvent(NULL))
		SDL_Delay(least - tv_now);
	}
    } 
}

/*
 * $Log: event.c,v $
 * Revision 1.61  2001/01/05 21:12:32  weimer
 * advance to atris 1.0.5, add support for ".atrisrc" and changing the
 * keyboard repeat rate
 *
 * Revision 1.60  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.59  2000/11/06 04:39:56  weimer
 * networking consistency check for power pieces
 *
 * Revision 1.58  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.57  2000/11/06 01:25:54  weimer
 * add in the other special piece
 *
 * Revision 1.56  2000/11/06 00:51:55  weimer
 * fixed the paint thing so that it actually paints
 *
 * Revision 1.55  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.54  2000/11/03 04:25:58  weimer
 * add some optimizations to run_gravity to make it just a bit faster (down
 * to 0.01 ms/call from 0.02), sleep a bit more in event-loop: generally
 * trying to make us more CPU friendly ...
 *
 * Revision 1.53  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.52  2000/11/01 17:55:33  weimer
 * remove ambiguous references to "you win" in debugging output
 *
 * Revision 1.51  2000/11/01 04:39:41  weimer
 * clear the little scoring spot correctly, updates for making a no-sound
 * distribution
 *
 * Revision 1.50  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.49  2000/10/30 16:57:24  weimer
 * minor doc fixups
 *
 * Revision 1.48  2000/10/30 16:25:25  weimer
 * display the network player score during network games. Also give the
 * non-server a little message when waiting for the server to go on.
 *
 * Revision 1.47  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.46  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.45  2000/10/29 19:04:32  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.44  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.43  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.42  2000/10/28 13:39:14  weimer
 * added a pausing feature ...
 *
 * Revision 1.41  2000/10/27 00:07:36  weimer
 * some sound fixes ...
 *
 * Revision 1.40  2000/10/26 22:23:07  weimer
 * double the effective number of levels
 *
 * Revision 1.39  2000/10/22 22:05:51  weimer
 * Added a few new sounds ...
 *
 * Revision 1.38  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.37  2000/10/20 01:32:09  weimer
 * Minor play issue problems -- time is now truly a hard limit!
 *
 * Revision 1.36  2000/10/19 22:30:55  weimer
 * make pieces fall a bit faster
 *
 * Revision 1.35  2000/10/19 22:06:51  weimer
 * minor changes ...
 *
 * Revision 1.34  2000/10/19 00:20:27  weimer
 * sound directory changes, added a ticking clock ...
 *
 * Revision 1.33  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.32  2000/10/18 03:29:56  weimer
 * fixed problem with "machine-gun" sounds caused by "run_gravity()" returning
 * 1 too often
 *
 * Revision 1.31  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.30  2000/10/14 16:17:41  weimer
 * level adjustment changes, added some new AIs, etc.
 *
 * Revision 1.29  2000/10/14 01:42:53  weimer
 * better scoring of thumbs-up, thumbs-down
 *
 * Revision 1.28  2000/10/14 01:24:34  weimer
 * fixed error with not advancing levels when fighting AI
 *
 * Revision 1.27  2000/10/14 00:58:32  weimer
 * fixed an unsigned arithmetic problem that was sleeping too often!
 *
 * Revision 1.26  2000/10/14 00:56:41  weimer
 * whoops, minor boolean typo!
 *
 * Revision 1.25  2000/10/14 00:42:27  weimer
 * fixed minor error where you couldn't move all the way to the left ...
 *
 * Revision 1.24  2000/10/13 22:34:26  weimer
 * minor wessy AI changes
 *
 * Revision 1.23  2000/10/13 18:23:28  weimer
 * fixed a race condition in tetris_event()
 *
 * Revision 1.22  2000/10/13 17:55:36  weimer
 * Added another color "Style" ...
 *
 * Revision 1.21  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.20  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.19  2000/10/13 03:02:05  wkiri
 * Added Aliz (Kiri AI) to ai.c.
 * Note event.c currently calls Aliz, not Wessy AI.
 *
 * Revision 1.18  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.17  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.16  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.15  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.14  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.13  2000/09/05 22:10:46  weimer
 * fixed initial garbage generation strangeness
 *
 * Revision 1.12  2000/09/05 21:47:41  weimer
 * major timing revamping: it actually seems to work now!
 *
 * Revision 1.11  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.10  2000/09/04 15:20:21  weimer
 * faster blitting algorithms for drawing the grid and drawing falling pieces
 * (when you eliminate a line)
 *
 * Revision 1.9  2000/09/04 14:18:09  weimer
 * flushed out the pattern color so that it has the same number of colors as
 * the default
 *
 * Revision 1.8  2000/09/03 20:58:22  weimer
 * yada
 *
 * Revision 1.7  2000/09/03 20:57:02  weimer
 * changes the time variable to be passed by reference in the event loop
 *
 * Revision 1.6  2000/09/03 20:34:42  weimer
 * only draw their score in multiplayer game
 *
 * Revision 1.5  2000/09/03 20:27:38  weimer
 * the event loop actually works!
 *
 * Revision 1.4  2000/09/03 19:59:56  weimer
 * wessy event-loop changes
 *
 * Revision 1.3  2000/09/03 18:44:36  wkiri
 * Cleaned up atris.c (see high_score_check()).
 * Added game type (defaults to MARATHON).
 *
 */


static Uint32 randomSeed;

void SeedRandom(Uint32 Seed)
{
	if ( ! Seed ) {
		srand(time(NULL));
		Seed = (((rand()%0xFFFF)<<16)|(rand()%0xFFFF));
	}
	randomSeed = Seed;
}

Uint32 GetRandSeed(void)
{
	return(randomSeed);
}

/* This magic is wholly the result of Andrew Welch, not me. :-) */
Uint16 FastRandom(Uint16 range)
{
	Uint16 result;
	register Uint32 calc;
	register Uint32 regD0;
	register Uint32 regD1;
	register Uint32 regD2;

	calc = randomSeed;
	regD0 = 0x41A7;
	regD2 = regD0;
	
	regD0 *= calc & 0x0000FFFF;
	regD1 = regD0;
	
	regD1 = regD1 >> 16;
	
	regD2 *= calc >> 16;
	regD2 += regD1;
	regD1 = regD2;
	regD1 += regD1;
	
	regD1 = regD1 >> 16;
	
	regD0 &= 0x0000FFFF;
	regD0 -= 0x7FFFFFFF;
	
	regD2 &= 0x00007FFF;
	regD2 = (regD2 << 16) + (regD2 >> 16);
	
	regD2 += regD1;
	regD0 += regD2;
	
	/* An unsigned value < 0 is always 0 */
	/*************************************
	 Not compiled:
	if (regD0 < 0)
		regD0 += 0x7FFFFFFF;
	 *************************************/
	
	randomSeed = regD0;
	if ((regD0 & 0x0000FFFF) == 0x8000)
		regD0 &= 0xFFFF0000;

/* -- Now that we have our pseudo random number, pin it to the range we want */

	regD1 = range;
	regD1 *= (regD0 & 0x0000FFFF);
	regD1 = (regD1 >> 16);
	
	result = regD1;
	
	return result;
}

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

/***************************************************************************
 *      cleanup_grid()
 * Removes all of the REMOVE_ME's in a grid. Normally one uses draw_grid
 * for this purpose, but you might have a personal testing grid or
 * something that you're not displaying.
 *********************************************************************PROTO*/
void
cleanup_grid(Grid *g)
{
    int x,y;
    for (y=g->h-1;y>=0;y--) 
	for (x=g->w-1;x>=0;x--) 
	    if (GRID_CONTENT(*g,x,y) == REMOVE_ME)
		GRID_SET(*g,x,y,0);
}

/***************************************************************************
 *      generate_board()
 * Creates a new board at the given level.
 *********************************************************************PROTO*/
Grid
generate_board(int w, int h, int level)
{
    int i,j,r;

    Grid retval;

    retval.w = w;
    retval.h = h;

    Calloc(retval.contents,unsigned char *,(w*h*sizeof(*retval.contents)));
    Calloc(retval.fall,unsigned char *,(w*h*sizeof(*retval.fall)));
    Calloc(retval.changed,unsigned char *,(w*h*sizeof(*retval.changed)));
    Calloc(retval.temp,unsigned char *,(w*h*sizeof(*retval.temp)));

    if (level) {
	int start_garbage;
	level = GARBAGE_LEVEL(level);
	start_garbage = (h-level) - 2;

	if (start_garbage < 2) start_garbage = 2;
	else if (start_garbage >= h) start_garbage = h - 1;

	for (j=start_garbage;j<h;j++)
	    for (r=0;r<w/2;r++) {
		do {
		    i = ZEROTO(w);
		} while (GRID_CONTENT(retval,i,j) == 1);
		GRID_SET(retval,i,j,1);
	    }
    }
    return retval;
}

/***************************************************************************
 *      add_garbage()
 * Adds garbage to the given board. Pushes all of the lines up, adds the
 * garbage to the bottom.
 *********************************************************************PROTO*/
void
add_garbage(Grid *g)
{
    int i,j;
    for (j=0;j<g->h-1;j++)
	for (i=0;i<g->w;i++) {
	    GRID_SET(*g,i,j, GRID_CONTENT(*g,i,j+1));
	    FALL_SET(*g,i,j, NOT_FALLING);
	    if (GRID_CONTENT(*g,i,j) == 0)
		GRID_SET(*g,i,j,REMOVE_ME);
	    GRID_CHANGED(*g,i,j) = 1;
	}

    j = g->h - 1;
    for (i=0; i<g->w; i++) {
	    if (ZEROTO(100) < 50) {
		GRID_SET(*g,i,j,1);
		if (GRID_CONTENT(*g,i,j-1) &&
			GRID_CONTENT(*g,i,j-1) != REMOVE_ME)
		    GRID_SET(*g,i,j-1,1);
	    } else {
		GRID_SET(*g,i,j,REMOVE_ME);
	    }
	    FALL_SET(*g,i,j, NOT_FALLING);
	    GRID_CHANGED(*g,i,j) = 1;
    }

    for (j=0;j<g->h;j++)
	for (i=0;i<g->w;i++) {
	    Assert(GRID_CHANGED(*g,i,j));
	    Assert(FALL_CONTENT(*g,i,j) == NOT_FALLING);
	}

    return;
}

/***************************************************************************
 *      draw_grid()
 * Draws the main grid board. This involves drawing all of the pieces (and
 * garbage) currently pasted on to it. This is the function that actually
 * clears out grid pieces marked with "REMOVE_ME". The main bit of work
 * here is calculating the shadows. 
 *
 * Uses the color style to actually draw the right picture on the screen.
 *********************************************************************PROTO*/
void
draw_grid(SDL_Surface *screen, color_style *cs, Grid *g, int draw)
{
    SDL_Rect r,s;
    int i,j;
    int mini=20, minj=20, maxi=-1, maxj=-1;
    for (j=g->h-1;j>=0;j--) {
	for (i=g->w-1;i>=0;i--) {
	    int c = GRID_CONTENT(*g,i,j);
	    if (draw && c && c != REMOVE_ME && GRID_CHANGED(*g,i,j)) {
		if (i < mini) mini = i; if (j < minj) minj = j;
		if (i > maxi) maxi = i; if (j > maxj) maxj = j;
		

		s.x = g->board.x + (i * cs->w);
		s.y = g->board.y + (j * cs->h);
		s.w = cs->w;
		s.h = cs->h;

		SDL_BlitSafe(cs->color[c], NULL, screen, &s);

		{
		    int fall = FALL_CONTENT(*g,i,j);

		    int that_precolor = (j == 0) ? 0 : 
			GRID_CONTENT(*g,i,j-1);
		    int that_fall = (j == 0) ? -1 :
			FALL_CONTENT(*g,i,j-1);
		    /* light up */
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = g->board.y + j * cs->h;
			r.h = edge[HORIZ_LIGHT]->h;
			r.w = edge[HORIZ_LIGHT]->w;
			SDL_BlitSafe(edge[HORIZ_LIGHT],NULL,
				    screen, &r);
		    }

		    /* light left */
		    that_precolor = (i == 0) ? 0 :
			GRID_CONTENT(*g,i-1,j);
		    that_fall = (i == 0) ? -1 :
			FALL_CONTENT(*g,i-1,j);
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = g->board.y + j * cs->h;
			r.h = edge[VERT_LIGHT]->h;
			r.w = edge[VERT_LIGHT]->w;
			SDL_BlitSafe(edge[VERT_LIGHT],NULL,
				    screen,&r);
		    }
		    
		    /* shadow down */
		    that_precolor = (j == g->h-1) ? 0 :
			GRID_CONTENT(*g,i,j+1);
		    that_fall = (j == g->h-1) ? -1 :
			FALL_CONTENT(*g,i,j+1);
		    if (that_precolor != c || that_fall != fall) {
			r.x = g->board.x + i * cs->w;
			r.y = (g->board.y + (j+1) * cs->h) 
			    - edge[HORIZ_DARK]->h;
			r.h = edge[HORIZ_DARK]->h;
			r.w = edge[HORIZ_DARK]->w;
			SDL_BlitSafe(edge[HORIZ_DARK],NULL,
				    screen,&r);
		    }

		    /* shadow right */
		    that_precolor = (i == g->w-1) ? 0 :
			GRID_CONTENT(*g,i+1,j);
		    that_fall = (i == g->w-1) ? -1 :
			FALL_CONTENT(*g,i+1,j);
		    if (that_precolor != c || that_fall != fall) {
			r.x = (g->board.x + (i+1) * cs->w) 
			    - edge[VERT_DARK]->w;
			r.y = (g->board.y + (j) * cs->h);
			r.h = edge[VERT_DARK]->h;
			r.w = edge[VERT_DARK]->w;
			SDL_BlitSafe(edge[VERT_DARK],NULL,
				    screen,&r);
		    }
		} /* endof: hikari to kage */
		/* SDL_UpdateSafe(screen, 1, &s); */
		GRID_CHANGED(*g,i,j) = 0;
	    } else if (c == REMOVE_ME) {
		if (i < mini) mini = i; if (j < minj) minj = j;
		if (i > maxi) maxi = i; if (j > maxj) maxj = j;
		if (draw) { 
		    s.x = g->board.x + (i * cs->w);
		    s.y = g->board.y + (j * cs->h);
		    s.w = cs->w;
		    s.h = cs->h;
		    SDL_FillRect(screen, &s, int_solid_black);
		    GRID_SET(*g,i,j,0);
		    GRID_CHANGED(*g,i,j) = 0;
		} else {
		    GRID_SET(*g,i,j,0);
		}
	    }
	}
    }
    s.x = g->board.x + mini * cs->w;
    s.y = g->board.y + minj * cs->h;
    s.w = (maxi - mini + 1) * cs->w;
    s.h = (maxj - minj + 1) * cs->h;
    if (draw && maxi >  -1) SDL_UpdateSafe(screen,1,&s);
    return;
}

/***************************************************************************
 *      draw_falling()
 * Draws the falling pieces on the main grid. Offset should range from 1 to
 * the size of the color tiles -- the falling pieces are drawn that far
 * down out of their "real" places. This gives a smooth animation effect.
 *********************************************************************PROTO*/
void
draw_falling(SDL_Surface *screen, int blockWidth, Grid *g, int offset)
{
    SDL_Rect s;
    SDL_Rect r;  
    int i,j;
    int mini=100000, minj=100000, maxi=-1, maxj=-1;

    int cj;	/* cluster right, cluster bottom */

    memset(g->temp, 0, sizeof(*g->temp)*g->w*g->h);

    for (j=0;j<g->h;j++) {
	for (i=0;i<g->w;i++) {
	    int c = GRID_CONTENT(*g,i,j);
	    if (c && FALL_CONTENT(*g,i,j) == FALLING &&
		    TEMP_CONTENT(*g,i,j) == 0) {

		for (cj = j; GRID_CONTENT(*g,i,cj) &&
			FALL_CONTENT(*g,i,cj) &&
			TEMP_CONTENT(*g,i,cj) == 0 &&
			cj < g->h; cj++)
		    TEMP_CONTENT(*g,i,cj) = 1;

		/* source == up */
		s.x = g->board.x + (i * blockWidth);
		s.y = g->board.y + (j * blockWidth) + offset - 1;
		s.w = blockWidth;
		s.h = blockWidth * (cj - j + 1); 

		/* dest == down */
		r.x = g->board.x + (i * blockWidth);
		r.y = g->board.y + (j * blockWidth) + offset;
		r.w = blockWidth;
		r.h = blockWidth * (cj - j + 1); 

		/* just blit the screen down a notch */
		SDL_BlitSafe(screen, &s, screen, &r);

		if (s.x < mini) mini = s.x; 
		if (s.y < minj) minj = s.y;
		if (s.x+s.w > maxi) maxi = s.x+s.w; 
		if (s.y+s.h > maxj) maxj = s.y+s.h;

		/* clear! */
		if (j == 0 || FALL_CONTENT(*g,i,j-1) == NOT_FALLING ||
			GRID_CONTENT(*g,i,j-1) == 0) {
		    s.h = 1;
		    SDL_BlitSafe(widget_layer, &s, screen, &s);
		    /*  SDL_UpdateSafe(screen, 1, &s); */
		}
	    }
	}
    }

    s.x = mini;
    s.y = minj;
    s.w = maxi - mini + 1;
    s.h = maxj - minj + 1;
    if (maxi >  -1) SDL_UpdateSafe(screen,1,&s);
    return;
}

/***************************************************************************
 *      fall_down()
 * Move all of the fallen pieces down a notch.
 *********************************************************************PROTO*/
void
fall_down(Grid *g)
{
    int x,y;
    for (y=g->h-1;y>=1;y--) {
	for (x=0;x<g->w;x++) {
	    if (FALL_CONTENT(*g,x,y-1) == FALLING &&
		     GRID_CONTENT(*g,x,y-1)) {
		Assert(GRID_CONTENT(*g,x,y) == 0 || 
		       GRID_CONTENT(*g,x,y) == REMOVE_ME);
		GRID_SET(*g,x,y, GRID_CONTENT(*g,x,y-1));
		GRID_SET(*g,x,y-1,REMOVE_ME);
		FALL_SET(*g,x,y, FALLING);
		/* FALL_SET(*g,x,y-1,UNKNOWN); */
	    }
	}
    }
    return;
}

/***************************************************************************
 *      determine_falling()
 * Determines if anything can fall.
 *********************************************************************PROTO*/
int
determine_falling(Grid *g)
{
    int x,y;
#ifdef DEBUG
    {
	int retval=0;
    for (y=0;y<g->h;y++) {
	for (x=0;x<g->w;x++) {
	    if (GRID_CONTENT(*g,x,y) && FALL_CONTENT(*g,x,y) == FALLING) {
		printf("X");
		retval= 1;
	    } else printf(".");
	}
	printf("\n");
    }
    printf("--\n");
    return retval;
    }
#else
    for (y=0;y<g->h;y++) 
	for (x=0;x<g->w;x++) 
	    if (GRID_CONTENT(*g,x,y) && FALL_CONTENT(*g,x,y) == FALLING) 
		return 1;
    return 0;
#endif
}

/***************************************************************************
 *      run_gravity()
 * Applies gravity: pieces with no support change to "FALLING". After this,
 * "determine_falling" can be used to determine if anything should be
 * falling.
 *
 * Returns 1 if any pieces that were FALLING settled down.
 *
 * This is the Mark II iterative version. Heavy-sigh!
 *********************************************************************PROTO*/
int
run_gravity(Grid *g)
{
    int falling_pieces_settled;
    int x,y,c,f, X,Y, YY;
    int S; 
    int lowest_y = 0;
    int garbage_on_row[21] ;	/* FIXME! */
    /*   6.68      5.20     0.38    16769     0.02     0.02  run_gravity */

    char UP_QUEUE[2000], UP_POS=0;

#define UP_X(P) UP_QUEUE[(P)*3]
#define UP_Y(P) UP_QUEUE[(P)*3 + 1]
#define UP_S(P) UP_QUEUE[(P)*3 + 2]

    falling_pieces_settled = 0;
    for (y=0; y<g->h; y++) {
	garbage_on_row[y] = 0;
	for (x=0;x<g->w;x++) {
	    int c = GRID_CONTENT(*g,x,y);
	    if (!lowest_y && c)
		lowest_y = y;
	    if (c == 1)
		garbage_on_row[y] = 1;
	    if (FALL_CONTENT(*g,x,y) == NOT_FALLING)
		FALL_SET(*g,x,y,UNKNOWN);
	}
    }
    garbage_on_row[20] = 1;

    for (y=g->h-1;y>=lowest_y;y--)
	for (x=0;x<g->w;x++) {
	    c = GRID_CONTENT(*g,x,y);
	    if (!c) continue;
	    else if (c != 1 && y < g->h-1) continue;
	    else if (c == 1 && !garbage_on_row[y+1]) {
		/* if we do not set this, garbage from above us will appear
		 * to be supported and it will reach down and support us!
		 * scary! */
		garbage_on_row[y] = 0;
		continue;
	    }
	    f = FALL_CONTENT(*g,x,y);
	    if (f == NOT_FALLING) continue;
	    else if (f == FALLING) {
		falling_pieces_settled = 1;
	    }

	    /* now, run up as far as we can ... */
	    if (y >= 1) {
		Y = y;
		while ((c = GRID_CONTENT(*g,x,Y)) && 
			FALL_CONTENT(*g,x,Y) != NOT_FALLING) {
		    /* mark stable */
		    f = FALL_CONTENT(*g,x,Y);
		    if (f == FALLING) {
			falling_pieces_settled = 1;
		    }
		    FALL_SET(*g,x,Y,NOT_FALLING);
		    /* promise to get to our same-color buddies later */
		    if (x >= 1 && GRID_CONTENT(*g,x-1,Y) == c &&
			    FALL_CONTENT(*g,x-1,Y) != NOT_FALLING) {
			UP_X(UP_POS) = x-1 ; UP_Y(UP_POS) = Y; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    if (x < g->w-1 && GRID_CONTENT(*g,x+1,Y) == c &&
			    FALL_CONTENT(*g,x+1,Y) != NOT_FALLING) {
			UP_X(UP_POS) = x+1 ; UP_Y(UP_POS) = Y; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    if (Y < g->h-1 && GRID_CONTENT(*g,x,Y+1) == c &&
			    FALL_CONTENT(*g,x,Y+1) != NOT_FALLING) {
			UP_X(UP_POS) = x ; UP_Y(UP_POS) = Y+1; 
			UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
		    }
		    Y--;
		}
	    } else {
		/* Debug("2 (%2d,%2d)\n",x,y); */
		FALL_SET(*g,x,y,NOT_FALLING);
	    }
	    
	    /* now burn down the queue */
	    while (UP_POS > 0) {
		UP_POS--;
		X = UP_X(UP_POS);
		Y = UP_Y(UP_POS);
		S = UP_S(UP_POS);
		c = GRID_CONTENT(*g,X,Y);
		Assert(c);
		f = FALL_CONTENT(*g,X,Y);
		if (f == NOT_FALLING) continue;
		else if (f == FALLING && !S) continue;

		if (Y >= 1) {
		    YY = Y;
		    while ((c = GRID_CONTENT(*g,X,YY)) && 
			    FALL_CONTENT(*g,X,YY) != NOT_FALLING) {
			/* mark stable */
			f = FALL_CONTENT(*g,X,YY);
			if (f == FALLING) falling_pieces_settled = 1;
			FALL_SET(*g,X,YY,NOT_FALLING);
			/* promise to get to our same-color buddies later */
			if (X >= 1 && GRID_CONTENT(*g,X-1,YY) == c &&
				FALL_CONTENT(*g,X-1,YY) != NOT_FALLING) {
			    UP_X(UP_POS) = X-1 ; UP_Y(UP_POS) = YY; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			if (X < g->w-1 && GRID_CONTENT(*g,X+1,YY) == c &&
				FALL_CONTENT(*g,X+1,YY) != NOT_FALLING) {
			    UP_X(UP_POS) = X+1 ; UP_Y(UP_POS) = YY; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			if (YY < g->h-1 && GRID_CONTENT(*g,X,YY+1) == c &&
				FALL_CONTENT(*g,X,YY+1) != NOT_FALLING) {
			    UP_X(UP_POS) = X ; UP_Y(UP_POS) = YY+1; 
			    UP_S(UP_POS) = (f == FALLING || c == 1); UP_POS++;
			}
			YY--;
		    }
		} else {
		    FALL_SET(*g,X,Y,NOT_FALLING);
		}
	    }
	}
	    
    for (y=g->h-1;y>=0;y--) 
	for (x=0;x<g->w;x++) 
	    if (FALL_CONTENT(*g,x,y) == UNKNOWN)
		FALL_CONTENT(*g,x,y) = FALLING;

    return falling_pieces_settled;
}


/***************************************************************************
 *      check_tetris()
 * Checks to see if any rows have been completely filled. Returns the
 * number of such rows. Such rows are cleared.
 *********************************************************************PROTO*/
int
check_tetris(Grid *g)
{
    int tetris_count = 0;
    int x,y;
    for (y=g->h-1;y>=0;y--)  {
	int c = 0;
	for (x=0;x<g->w;x++) 
	    if (GRID_CONTENT(*g,x,y))
		c++;
	if (c == g->w) {
	    tetris_count++;
	    for (x=0;x<g->w;x++)
		GRID_SET(*g,x,y,REMOVE_ME);
	}
    }
    return tetris_count;
}

/*
 * $Log: grid.c,v $
 * Revision 1.24  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.23  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.22  2000/11/03 04:25:58  weimer
 * add some optimizations to run_gravity to make it just a bit faster (down
 * to 0.01 ms/call from 0.02), sleep a bit more in event-loop: generally
 * trying to make us more CPU friendly ...
 *
 * Revision 1.21  2000/11/03 03:41:35  weimer
 * made the light and dark "edges" of pieces global, rather than part of a
 * specific color style. also fixed a bug where we were updating too much
 * when drawing falling pieces (bad min() code on my part)
 *
 * Revision 1.20  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.19  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.18  2000/10/26 22:23:07  weimer
 * double the effective number of levels
 *
 * Revision 1.17  2000/10/21 01:14:42  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.16  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.15  2000/10/18 03:29:56  weimer
 * fixed problem with "machine-gun" sounds caused by "run_gravity()" returning
 * 1 too often
 *
 * Revision 1.14  2000/10/14 02:52:44  weimer
 * fixed a memory corruption problem in display (a use after a free)
 *
 * Revision 1.13  2000/10/14 00:31:53  weimer
 * non-recursive run-gravity ...
 *
 * Revision 1.12  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.11  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.10  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.9  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.8  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.7  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.6  2000/09/05 22:10:46  weimer
 * fixed initial garbage generation strangeness
 *
 * Revision 1.5  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.4  2000/09/04 21:02:18  weimer
 * Added some addition piece layout code, garbage now falls unless the row
 * below it features some non-falling garbage
 *
 * Revision 1.3  2000/09/04 15:20:21  weimer
 * faster blitting algorithms for drawing the grid and drawing falling pieces
 * (when you eliminate a line)
 *
 * Revision 1.2  2000/09/03 18:26:10  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.1  2000/09/03 17:56:36  wkiri
 * Reorganization of files (display.[ch], event.c are new).
 *
 * Revision 1.9  2000/08/26 02:28:55  weimer
 * now you can see the other player!
 *
 * Revision 1.8  2000/08/26 01:36:25  weimer
 * fixed a long-running update bug: the grid changed field was being reset to
 * 0 when falling states or content were set to the same thing, even if they
 * had actually previously changed ...
 *
 * Revision 1.7  2000/08/26 01:03:11  weimer
 * remove keyboard repeat when you enter high scores
 *
 * Revision 1.6  2000/08/24 02:32:18  weimer
 * gameplay changes (key repeats) and speed modifications (falling pieces,
 * redraws, etc.)
 *
 * Revision 1.5  2000/08/22 15:13:53  weimer
 * added a garbage-generation routine
 *
 * Revision 1.4  2000/08/21 00:04:56  weimer
 * minor text placement changes, falling speed, etc.
 *
 * Revision 1.3  2000/08/15 00:59:41  weimer
 * a few more sound things
 *
 * Revision 1.2  2000/08/13 19:27:20  weimer
 * added file changelogs
 *
 */

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

/***************************************************************************
 *      menu()
 * Create a new menu and return it.
 *********************************************************************PROTO*/
ATMenu* menu(int nButtons, char** strings, int defaultchoice,
	     Uint32 face_color0, Uint32 text_color0,
	     Uint32 face_color1, Uint32 text_color1, int x, int y)
{
  int i, yp;
  ATMenu* am = (ATMenu*)malloc(sizeof(ATMenu)); assert(am);

  /* Create the buttons */
  am->buttons = (ATButton**)malloc(nButtons*sizeof(ATButton*)); 
  assert(am->buttons);
  am->clicked = (char*)malloc(nButtons*sizeof(char)); assert(am->clicked);
  am->nButtons = nButtons;
  am->x = x; am->y = y;
  am->defaultchoice = defaultchoice;

  yp = y;

  Assert(nButtons >= 0);

  for (i=0; i<nButtons; i++)
    {
      /* Line them up x-wise */
      am->buttons[i] = button(strings[i], face_color0, text_color0,
			      face_color1, text_color1, x, yp);
      yp += am->buttons[i]->area.h; /* add a separating space? */
      am->clicked[i] = FALSE;
    }

  am->w = am->buttons[0]->area.w;
  am->h = (am->buttons[nButtons-1]->area.y - am->buttons[0]->area.y)
    + am->buttons[nButtons-1]->area.h;
  /* Set the default one */
  if (defaultchoice >= 0) am->clicked[defaultchoice] = TRUE;

  return am;
}

/***************************************************************************
 *      show_menu()
 * Displays the menu.
 *********************************************************************PROTO*/
void show_menu(ATMenu* am)
{
  int i;
  for (i=0; i<am->nButtons; i++)
    show_button(am->buttons[i], am->clicked[i]);
}

/***************************************************************************
 *      check_menu()
 * Returns the index of the button that was clicked (-1 if none).
 *********************************************************************PROTO*/
int check_menu(ATMenu* am, int x, int y)
{
  int i, j;
  for (i=0; i<am->nButtons; i++)
    if (check_button(am->buttons[i], x, y)) 
      {
	/* invert its clicked state */
	if (am->clicked[i])
	  {
	    am->clicked[i] = FALSE;
	    /* reset the default */
	    am->clicked[am->defaultchoice] = TRUE;
	    show_button(am->buttons[am->defaultchoice],
			am->clicked[am->defaultchoice]);
	  }
	else
	  {
	    /* blank all others */
	    for (j=0; j<am->nButtons; j++)
	      if (i!=j && am->clicked[j])
		{
		  am->clicked[j] = FALSE;
		  show_button(am->buttons[j], am->clicked[j]);
		}
	    am->clicked[i] = TRUE;
	  }
	/* redraw it */
	show_button(am->buttons[i], am->clicked[i]);
	Debug("Button %d was clicked.\n", i);
	return i;
      }
  return -1;
}

/***************************************************************************
 *      delete_menu()
 * Deletes the menu and all of its buttons.
 *********************************************************************PROTO*/
void delete_menu(ATMenu* am)
{
  int i;
  for (i=0; i<am->nButtons; i++)
    Free(am->buttons[i]);
  Free(am);
}

#define BORDER_X	8
#define BORDER_Y	8
#define BUTTON_X_SPACE	4
#define RADIO_HEIGHT	12

/***************************************************************************
 *      setup_radio()
 * Prepared a WalkRadio button for display.
 *********************************************************************PROTO*/
void
setup_radio(WalkRadio *wr)
{
    int i;
    int our_height;

    wr->face_color[0] = int_button_face0;
    wr->face_color[1] = int_button_face1;
    wr->text_color[0] = int_button_text0;
    wr->text_color[1] = int_button_text1;
    wr->border_color[0] = int_button_border0;
    wr->border_color[1] = int_button_border1;

    wr->bitmap0 = malloc(sizeof(SDL_Surface *) * wr->n); Assert(wr->bitmap0);
    wr->bitmap1 = malloc(sizeof(SDL_Surface *) * wr->n); Assert(wr->bitmap1);
    wr->w = 0;
    wr->h = 0;

    for (i=0; i<wr->n; i++) {
	SDL_Color sc;

	SDL_GetRGB(wr->text_color[0], screen->format, &sc.r, &sc.g, &sc.b);
	wr->bitmap0[i] = TTF_RenderText_Blended(sfont, wr->label[i], sc);
	Assert(wr->bitmap0[i]);

	SDL_GetRGB(wr->text_color[1], screen->format, &sc.r, &sc.g, &sc.b);
	wr->bitmap1[i] = TTF_RenderText_Blended(sfont, wr->label[i], sc);
	Assert(wr->bitmap1[i]);

	if (wr->bitmap0[i]->w + BUTTON_X_SPACE > wr->w) 
	    wr->w = wr->bitmap0[i]->w + BUTTON_X_SPACE;
	if (wr->bitmap0[i]->h > wr->h) wr->h = wr->bitmap0[i]->h;
    }
    wr->w += 5 ;
    wr->h += 10 ;
    wr->area.w = wr->w + BORDER_X;

    our_height = RADIO_HEIGHT;
    if (wr->n < our_height) our_height = wr->n;

    wr->area.h = wr->h * our_height + BORDER_Y;
    return;
}

/***************************************************************************
 *      check_radio()
 * Determines if we have clicked in a radio button.
 *********************************************************************PROTO*/
int
check_radio(WalkRadio *wr, int x, int y)
{
    if (wr->inactive) return 0;

    if (    x  >= wr->area.x &&
	    x  <= wr->area.x + wr->area.w &&
	    y  >= wr->area.y &&
	    y  <= wr->area.y + wr->area.h   ) {
	int start, stop;
	int c;

	if (wr->n < RADIO_HEIGHT) {	/* draw them all */
	    start = 0;
	    stop = wr->n;
	} else {		/* at least four */
	    start = wr->defaultchoice - (RADIO_HEIGHT/2);
	    if (start < 0) start = 0;
	    stop = start + RADIO_HEIGHT;
	    if (stop > wr->n) {	/* we're at the bottom */
		start -= (stop - wr->n);
		stop = wr->n;
	    }
	}
	c = (y - wr->area.y) / wr->h;
	c += start;
	if (c < 0) c = 0;
	if (c >= wr->n) c = wr->n - 1;
	wr->defaultchoice = c;
	return 1;
    }
    return 0;
}

/***************************************************************************
 *      clear_radio()
 * Clears away a WalkRadio button. Works even if I am inactive. 
 *********************************************************************PROTO*/
void
clear_radio(WalkRadio *wr)
{
    SDL_FillRect(widget_layer, &wr->area, int_black);
    SDL_FillRect(screen, &wr->area, int_black);
    SDL_BlitSurface(flame_layer, &wr->area, screen, &wr->area);
    SDL_BlitSurface(widget_layer, &wr->area, screen, &wr->area);
    SDL_UpdateSafe(screen, 1, &wr->area);
    return; 
}

/***************************************************************************
 *      draw_radio()
 * Draws a WalkRadio button.
 *
 * Only draws me if I am not inactive. 
 *********************************************************************PROTO*/
void
draw_radio(WalkRadio *wr, int state)
{
    int start, stop, i;
    SDL_Rect draw;
    
    if (wr->inactive) return;

    wr->area.x = wr->x;
    wr->area.y = wr->y;

    SDL_FillRect(widget_layer, &wr->area, wr->border_color[state]);

    if (wr->n < RADIO_HEIGHT) {	/* draw them all */
	start = 0;
	stop = wr->n;
    } else {		/* at least four */
	start = wr->defaultchoice - (RADIO_HEIGHT/2);
	if (start < 0) start = 0;
	stop = start + RADIO_HEIGHT;
	if (stop > wr->n) {	/* we're at the bottom */
	    start -= (stop - wr->n);
	    stop = wr->n;
	}
    }

    draw.x = wr->x + BORDER_X/2;
    draw.y = wr->y + BORDER_Y/2;
    draw.w = wr->w; 
    draw.h = wr->h;
    
    for (i=start; i<stop; i++) {
	int on = (wr->defaultchoice == i);
	SDL_Rect text;

	SDL_FillRect(widget_layer, &draw, wr->text_color[on]);
	draw.x += 2; draw.y += 2; draw.w -= 4; draw.h -= 4;
	if (state || on) 
	    SDL_FillRect(widget_layer, &draw, wr->face_color[on]);
	else 
	    SDL_FillRect(widget_layer, &draw, wr->border_color[on]);
	draw.x += 3; draw.y += 3; draw.w -= 6; draw.h -= 6;

	text.x = draw.x; text.y = draw.y;
	text.w = wr->bitmap0[i]->w;
	text.h = wr->bitmap0[i]->h;
	text.x += (draw.w - text.w) / 2;

	if (on) {
	    SDL_BlitSafe(wr->bitmap1[i], NULL, widget_layer, &text);
	} else {
	    SDL_BlitSafe(wr->bitmap0[i], NULL, widget_layer, &text);
	}
	draw.x -= 5; draw.y -= 5; draw.w += 10; draw.h += 10;

	draw.y += draw.h; /* move down! */
    }

    SDL_BlitSurface(flame_layer, &wr->area, screen, &wr->area);
    SDL_BlitSurface(widget_layer, &wr->area, screen, &wr->area);
    SDL_UpdateSafe(screen, 1, &wr->area);
}

/***************************************************************************
 *      create_single_wrg()
 * Creates a walk-radio-group with a single walk-radio button with N
 * choices. 
 *********************************************************************PROTO*/
WalkRadioGroup *
create_single_wrg(int n)
{
    WalkRadioGroup *wrg;

    Calloc(wrg, WalkRadioGroup *, 1 * sizeof(*wrg));
    wrg->n = 1;
    Calloc(wrg->wr, WalkRadio *, wrg->n * sizeof(*wrg->wr));
    wrg->wr[0].n = n;
    Calloc(wrg->wr[0].label, char **, wrg->wr[0].n * sizeof(*wrg->wr[0].label));
    return wrg;
}

/***************************************************************************
 *      handle_radio_event()
 * Tries to handle the given event with respect to the given set of
 * WalkRadio's. Returns non-zero if an action said that we are done.
 *********************************************************************PROTO*/
int
handle_radio_event(WalkRadioGroup *wrg, const SDL_Event *ev)
{
    int i, ret;
    if (ev->type == SDL_MOUSEBUTTONDOWN) {
	for (i=0; i<wrg->n; i++) {
	    if (check_radio(&wrg->wr[i], ev->button.x, ev->button.y)) {
		if (i != wrg->cur) {
		    draw_radio(&wrg->wr[wrg->cur], 0);
		}
		draw_radio(&wrg->wr[i], 1);
		if (wrg->wr[i].action) {
		    ret = wrg->wr[i].action(&wrg->wr[i]);
		    return ret;
		} else
		    return wrg->wr[i].defaultchoice;
	    }
	}
    } else if (ev->type == SDL_KEYDOWN) {
	switch (ev->key.keysym.sym) {
	    case SDLK_RETURN:
		if (wrg->wr[wrg->cur].action) {
		    ret = wrg->wr[wrg->cur].action(&wrg->wr[wrg->cur]);
		    return ret;
		} else
		    return wrg->wr[wrg->cur].defaultchoice;
		return 1;

	    case SDLK_UP:
		wrg->wr[wrg->cur].defaultchoice --;
		if (wrg->wr[wrg->cur].defaultchoice < 0)
		    wrg->wr[wrg->cur].defaultchoice = wrg->wr[wrg->cur].n - 1;
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;
		break;

	    case SDLK_DOWN:
		wrg->wr[wrg->cur].defaultchoice ++;
		if (wrg->wr[wrg->cur].defaultchoice >= wrg->wr[wrg->cur].n)
		    wrg->wr[wrg->cur].defaultchoice = 0;
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    case SDLK_LEFT:
		draw_radio(&wrg->wr[wrg->cur], 0);
		do { 
		    wrg->cur--;
		    if (wrg->cur < 0) 
			wrg->cur = wrg->n - 1;
		} while (wrg->wr[wrg->cur].inactive);
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    case SDLK_RIGHT: 
		draw_radio(&wrg->wr[wrg->cur], 0);
		do { 
		    wrg->cur++;
		    if (wrg->cur >= wrg->n) 
			wrg->cur = 0;
		} while (wrg->wr[wrg->cur].inactive);
		draw_radio(&wrg->wr[wrg->cur], 1);
		return -1;

	    default:
		return -1;
	}
    }
    return -1;
}

/*
 * $Log: menu.c,v $
 * Revision 1.17  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.16  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.15  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.14  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.13  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.12  2000/10/21 01:22:44  weimer
 * prevent segfault from out-of-range values
 *
 * Revision 1.11  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.10  2000/10/19 22:06:51  weimer
 * minor changes ...
 *
 * Revision 1.9  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.8  2000/10/13 02:26:54  weimer
 * rudimentary identity functions, including adding new players
 *
 * Revision 1.7  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.6  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.5  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.4  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.3  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.2  2000/09/04 22:49:51  wkiri
 * gamemenu now uses the new nifty menus.  Also, added delete_menu.
 *
 * Revision 1.1  2000/09/04 19:54:26  wkiri
 * Added menus (menu.[ch]).
 */

/***************************************************************************
 *	Server_AwaitConnection() 
 * Sets up the server side listening socket and awaits connections from the
 * outside world. Returns a socket.
 *********************************************************************PROTO*/
int
Server_AwaitConnection(int port)
{
  static struct sockaddr_in addr;
  static struct hostent *host;
  static int sockListen=0;  
  static int addrLen;
  int val1;
  struct linger val2;
  int sock;

  if (sockListen == 0) {
    memset(&addr,0,sizeof(addr));
    addr.sin_family=AF_INET;
    addr.sin_addr.s_addr=htonl(INADDR_ANY);
    addr.sin_port=htons(port);
    sockListen=socket(AF_INET,SOCK_STREAM,0);

    error_msg = 0;

    if (sockListen < 0) {
	Debug("unable to create socket: %s\n",error_msg = strerror(errno));
	return -1;
    } 

    val1=1;

#ifdef HAVE_SYS_SOCKET_H
    if (fcntl(sockListen, F_SETFL, O_NONBLOCK)) {
	Debug("unable to make socket non-blocking: %s\n", error_msg = strerror(errno));
	return -1;
    }
#endif

    if (setsockopt(sockListen,SOL_SOCKET,SO_REUSEADDR,
		   (void *)&val1, sizeof(val1))) {
      Debug("WARNING: setsockopt(...,SO_REUSEADDR) failed. You may have to wait a\n"
	      "\tfew minutes before you can try to be the server again.\n");
    }
    if (bind(sockListen, (struct sockaddr *)&addr, sizeof(addr))) {
	Debug("unable to bind socket (for listening): %s\n", error_msg = strerror(errno));
	return -1;
    }
    if (listen(sockListen,16)) {
	Debug("unable to listen on socket: %s\n", error_msg = strerror(errno));
	return -1;
    } 

    addrLen=sizeof(addr);

    Debug("accepting connections on port %d, socketfd %d\n",port,sockListen); 
  }
#if HAVE_SELECT || HAVE_WINSOCK_H
  /* it is possible to select() on a socket for the purpose of doing an
   * accept by putting it in the read group */
  {
      fd_set read_fds;
      struct timeval timeout = {0, 0};
      int retval;

      FD_ZERO(&read_fds);
      FD_SET(sockListen, &read_fds);

      retval = select(sockListen+1, &read_fds, NULL, NULL, &timeout);
      
      if (retval <= 0) {
	  /* no one is there */
	  return -1;
      }
  }
#endif
  sock=accept(sockListen, (struct sockaddr *)&addr,&addrLen);
  if (sock < 0) {
      if (
#ifdef HAVE_SYS_SOCKET_H
	      errno == EWOULDBLOCK || 
#endif
	      errno == EAGAIN)
	  return -1;
      else {
	  Debug("unable to accept on listening socket: %s\n", error_msg = strerror(errno));
	  return -1;
      }
  }

  val2.l_onoff=0; val2.l_linger=0;
  if (setsockopt(sock,SOL_SOCKET,SO_LINGER,(void *)&val2,sizeof(val2)))
      Debug("WARNING: setsockopt(...,SO_LINGER) failed. You may have to wait a few\n"
	      "\tminutes before you can try to be the server again.\n");

  host=gethostbyaddr((void *)&addr.sin_addr,sizeof(struct in_addr),AF_INET);
  if (!host) 
      Debug("connection from some unresolvable IP address, socketfd %d.\n", sock);
  else
      Debug("connection from %s, socketfd %d.\n",host->h_name,sock); 
  return sock;
}

/***************************************************************************
 *	Client_Connect() 
 * Set up client side networking and connect to the server. Returns a socket
 * pointing to the server.
 *
 * On error, -1 is returned.
 *********************************************************************PROTO*/
int
Client_Connect(char *hoststr, int lport)
{
    struct sockaddr_in addr;
    struct hostent *host;
    int mySock;
    short port = lport;
    int retval; 

    error_msg = NULL;

    host=gethostbyname(hoststr);
    if (!host) {
	Debug("unable to resolve [%s]: unknown hostname\n",hoststr);
	error_msg = "unknown hostname";
	return -1;
    }

    Debug("connecting to %s:%d ...\n",host->h_name,port);   

    memset(&addr, 0, sizeof(addr)); 
    addr.sin_family = AF_INET; /*host->h_addrtype;*/
    memcpy(&addr.sin_addr, host->h_addr, host->h_length);
    addr.sin_port=htons(port);
    mySock = socket(AF_INET, SOCK_STREAM, 0);
    if (mySock < 0) {
	Debug("unable to create socket: %s\n",error_msg = strerror(errno));
	return -1;
    }

    retval = connect(mySock, (struct sockaddr *) &addr, sizeof(addr));

    if (retval < 0) {
	Debug("unable to connect: %s\n",error_msg = strerror(errno));
	close(mySock);
	return -1;
    }

    Debug("connected to %s:%d, socketfd %d.\n",host->h_name,port,mySock);
    return mySock;
}

/***************************************************************************
 *	Network_Init() 
 * Call before you try to use any networking functions. Returns 0 on
 * success.
 *********************************************************************PROTO*/
int
Network_Init(void)
{
#if HAVE_WINSOCK_H
    WORD version_wanted = MAKEWORD(1,1);
    WSADATA wsaData;

    if ( WSAStartup(version_wanted, &wsaData) != 0 ) {
	Debug("WARNING: Couldn't initialize Winsock 1.1\n");
	return 1;
    }
    Debug("Winsock 1.1 networking available.\n");
    return 0;
#endif
    /* always works on Unix ... */
    return 0;
}

/***************************************************************************
 *	Network_Quit() 
 * Call when you are done networking. 
 *********************************************************************PROTO*/
void
Network_Quit(void)
{
#if HAVE_WINSOCK_H
    /* Clean up windows networking */
    if ( WSACleanup() == SOCKET_ERROR ) {
	if ( WSAGetLastError() == WSAEINPROGRESS ) {
	    WSACancelBlockingCall();
	    WSACleanup();
	}
    }
#endif
    return;
}


/***************************************************************************
 *      load_piece_style()
 * Load a piece style from the given file.
 ***************************************************************************/
static piece_style *
load_piece_style(const char *filename)
{
    piece_style *retval;
    char buf[2048];
    FILE *fin = fopen(filename,"rt");
    int i;

    if (!fin) {
	Debug("fopen(%s)\n",filename);
	return NULL;
    }
    Calloc(retval,piece_style *,sizeof(piece_style));

    fgets(buf,sizeof(buf),fin);
    if (feof(fin)) {
	Debug("unexpected EOF after name in [%s]\n",filename);
	free(retval);
	return NULL;
    }

    if (strchr(buf,'\n'))
	*(strchr(buf,'\n')) = 0;
    retval->name = strdup(buf);

    if (fscanf(fin,"%d",&retval->num_piece) != 1) {
	Debug("malformed piece count in [%s]\n",filename);
	free(retval->name);
	free(retval);
	return NULL;
    }

    Calloc(retval->shape,piece *,(retval->num_piece)*sizeof(piece));

    for (i=0;i<retval->num_piece;i++) {
	int x,y,rot,counter;
	do {
	    fgets(buf,sizeof(buf),fin);
	    if (feof(fin))
		PANIC("unexpected EOF in [%s], before piece %d",filename,i+1);
	} while (buf[0] == '\n');
	retval->shape[i].dim = strlen(buf) - 1;
	if (retval->shape[i].dim <= 0) 
	    PANIC("piece %d malformed height/width in [%s]", i,filename);
#ifdef DEBUG
	Debug("... loading piece %d (%d by %d)\n",i,
		retval->shape[i].dim, retval->shape[i].dim);
#endif
	for (rot=0;rot<4;rot++) {
	    Malloc(retval->shape[i].bitmap[rot], unsigned char *,
		    retval->shape[i].dim * retval->shape[i].dim);
	}
	counter = 1;
	for (y=0;y<retval->shape[i].dim;y++) {
	    for (x=0;x<retval->shape[i].dim;x++) {
		BITMAP(retval->shape[i],0,x,y) = 
		    (buf[x] != '.') ? counter++ : 0;
	    }
	    if (y != retval->shape[i].dim - 1) do {
		fgets(buf,sizeof(buf),fin);
		if (feof(fin))
		    PANIC("unexpected EOF in [%s]",filename);
	    } while (buf[0] == '\n');
	}
	retval->shape[i].num_color = counter-1;

	for (rot=1;rot<4;rot++) {
	    for (y=0;y<retval->shape[i].dim;y++) 
		for (x=0;x<retval->shape[i].dim;x++) {
		    BITMAP(retval->shape[i],rot,x,y) =
			BITMAP(retval->shape[i],rot-1,
				(retval->shape[i].dim - 1) - y,
				x);
		}
	} /* end: for rot = 0..4 */

#ifdef DEBUG
	for (y=0;y<retval->shape[i].dim;y++) {
	    for (rot=0;rot<4;rot++) {
		for (x=0;x<retval->shape[i].dim;x++) {
		    printf("%d", BITMAP(retval->shape[i],rot,x,y));
		}
		printf("\t");
	    }
	    printf("\n");
	}
#endif
    }


    Debug("Piece Style [%s] loaded (%d pieces).\n",retval->name,
	    retval->num_piece);
    return retval;
}

/***************************************************************************
 *	piece_Select()
 * Returns 1 if the file pointed to ends with ".Piece" 
 * Used by scandir() to grab all the *.Piece files from a directory. 
 ***************************************************************************/
static int
piece_Select(const struct dirent *d)
{
    if (strstr(d->d_name,".Piece") && 
	    (signed)strlen(d->d_name) == 
	    (strstr(d->d_name,".Piece") - d->d_name + 6))
	return 1;
    else 
	return 0; 
}

/***************************************************************************
 *      load_piece_styles()
 * Load all ".Style" files in the directory.
 *********************************************************************PROTO*/
piece_styles
load_piece_styles(void)
{
    piece_styles retval;
    int i = 0;
    DIR *my_dir;
    char filespec[2048];

    memset(&retval, 0, sizeof(retval));

    my_dir = opendir("styles");
    if (my_dir) {
	while (1) { 
	    struct dirent *this_file = readdir(my_dir);
	    if (!this_file) break;
	    if (piece_Select(this_file))
		i++;
	} 
	closedir(my_dir);
    } else {
	PANIC("Cannot read directory [styles/]");
    }
    my_dir = opendir("styles");
    if (my_dir) {
	if (i > 0) { 
	    int j;
	    Calloc(retval.style,piece_style **,sizeof(*(retval.style))*i);
	    retval.num_style = i;
	    j = 0;
	    while (j<i) {
		struct dirent *this_file = readdir(my_dir);
		if (!piece_Select(this_file)) continue;
		sprintf(filespec,"styles/%s",this_file->d_name);
		retval.style[j] = load_piece_style(filespec);
		if (strstr(retval.style[j]->name,"Default"))
		    retval.choice = j;
		j++;
	    }
	    closedir(my_dir);
	    return retval;
	} else {
	    PANIC("No piece styles [styles/*.Piece] found.\n");
	}
    } else { 
	PANIC("Cannot read directory [styles/]");
    }
    return retval;
}

/***************************************************************************
 *      load_color_style()
 * Load a color style from the given file.
 ***************************************************************************/
static color_style *
load_color_style(SDL_Surface * screen, const char *filename)
{
    color_style *retval;
    char buf[2048];
    FILE *fin = fopen(filename,"rt");
    int i;

    if (!fin) {
	Debug("fopen(%s)\n",filename);
	return NULL;
    }
    Calloc(retval,color_style *,sizeof(*retval));

    fgets(buf,sizeof(buf),fin);
    if (feof(fin)) {
	Debug("unexpected EOF after name in [%s]\n",filename);
	free(retval);
	return NULL;
    }
    if (strchr(buf,'\n'))
	*(strchr(buf,'\n')) = 0;
    retval->name = strdup(buf);

    if (fscanf(fin,"%d\n",&retval->num_color) != 1) {
	Debug("malformed color count in [%s]\n",filename);
	free(retval->name);
	free(retval);
	return NULL;
    }

    Malloc(retval->color, SDL_Surface **,
	    (retval->num_color+1)*sizeof(retval->color[0]));

    for (i=1;i<=retval->num_color;i++) {
	SDL_Surface *imagebmp;

	do {
	    buf[0] = 0;
	    fgets(buf,sizeof(buf),fin);
	} while (!feof(fin) && (buf[0] == '\n' || buf[0] == '#'));

	if (feof(fin)) PANIC("unexpected EOF in color style [%s]",retval->name);
	if (strchr(buf,'\n'))
	    *(strchr(buf,'\n')) = 0;

	imagebmp = SDL_LoadBMP(buf);
	if (!imagebmp) 
	    PANIC("cannot load [%s] in color style [%s]",buf,retval->name);
	/* set the video colormap */
	if ( imagebmp->format->palette != NULL ) {
	    SDL_SetColors(screen,
		    imagebmp->format->palette->colors, 0,
		    imagebmp->format->palette->ncolors);
	}
	/* Convert the image to the video format (maps colors) */
	retval->color[i] = SDL_DisplayFormat(imagebmp);
	SDL_FreeSurface(imagebmp);
	if ( !retval->color[i] ) 
	    PANIC("could not convert [%s] in color style [%s]", 
		buf, retval->name);
	if (i == 1) {
	    retval->h = retval->color[i]->h;
	    retval->w = retval->color[i]->w;
	} else {
	    if (retval->h != retval->color[i]->h ||
		    retval->w != retval->color[i]->w)
		PANIC("[%s] has the wrong size in color style [%s]",
			buf, retval->name);
	}

    }
    retval->color[0] = retval->color[1];

    Debug("Color Style [%s] loaded (%d colors).\n",retval->name,
	    retval->num_color);

    return retval;
}

/***************************************************************************
 *	color_Select()
 * Returns 1 if the file pointed to ends with ".Color" 
 * Used by scandir() to grab all the *.Color files from a directory. 
 ***************************************************************************/
static int
color_Select(const struct dirent *d)
{
    if (strstr(d->d_name,".Color") && 
	    (signed)strlen(d->d_name) == 
	    (strstr(d->d_name,".Color") - d->d_name + 6))
	return 1;
    else 
	return 0; 
}

/***************************************************************************
 *	load_specials()
 * Loads the pictures for the special pieces.
 ***************************************************************************/
static void
load_special(void)
{
    int i;
#define NUM_SPECIAL 6
    char *filename[NUM_SPECIAL] = {
	"graphics/Special-Bomb.bmp",		/* special_bomb */
	"graphics/Special-Drip.bmp",		/* special_repaint */
	"graphics/Special-DownArrow.bmp",	/* special_pushdown */
	"graphics/Special-Skull.bmp",		/* special_colorkill */
	"graphics/Special-X.bmp",
	"graphics/Special-YinYang.bmp" };

    special_style.name = "Special Pieces";
    special_style.num_color = NUM_SPECIAL;
    Malloc(special_style.color, SDL_Surface **, NUM_SPECIAL * sizeof(SDL_Surface *));
    special_style.w = 20;
    special_style.h = 20;

    for (i=0; i<NUM_SPECIAL; i++) {
	SDL_Surface *imagebmp;
	/* grab the lighting */
	imagebmp = SDL_LoadBMP(filename[i]);
	if (!imagebmp) 
	    PANIC("cannot load [%s], a required special piece",filename[i]);
	if ( imagebmp->format->palette != NULL ) {
	    SDL_SetColors(screen,
		    imagebmp->format->palette->colors, 0,
		    imagebmp->format->palette->ncolors);
	}
	/* Convert the image to the video format (maps colors) */
	special_style.color[i] = SDL_DisplayFormat(imagebmp);
	SDL_FreeSurface(imagebmp);
	if ( !special_style.color[i] ) 
	    PANIC("could not convert [%s], a required special piece",filename[i]);
    }
    return;
}

/***************************************************************************
 *	load_edges()
 * Loads the pictures for the edges.
 ***************************************************************************/
static void
load_edges(void)
{
    int i;
    char *filename[4] = {
	"graphics/Horiz-Light.bmp",
	"graphics/Vert-Light.bmp",
	"graphics/Horiz-Dark.bmp",
	"graphics/Vert-Dark.bmp" };

    for (i=0;i<4;i++) {
	SDL_Surface *imagebmp;
	/* grab the lighting */
	imagebmp = SDL_LoadBMP(filename[i]);
	if (!imagebmp) 
	    PANIC("cannot load [%s], a required edge",filename[i]);
	/* set the video colormap */
	if ( imagebmp->format->palette != NULL ) {
	    SDL_SetColors(screen,
		    imagebmp->format->palette->colors, 0,
		    imagebmp->format->palette->ncolors);
	}
	/* Convert the image to the video format (maps colors) */
	edge[i] = SDL_DisplayFormat(imagebmp);
	SDL_FreeSurface(imagebmp);
	if ( !edge[i] ) 
	    PANIC("could not convert [%s], a required edge",filename[i]);

	SDL_SetAlpha(edge[i],SDL_SRCALPHA|SDL_RLEACCEL, 48 /*128+64*/);
    }
    return; 
}


/***************************************************************************
 *      load_color_styles()
 * Loads all available color styles.
 *********************************************************************PROTO*/
color_styles 
load_color_styles(SDL_Surface * screen)
{
    color_styles retval;
    int i = 0;
    DIR *my_dir;
    char filespec[2048];

    load_edges();
    load_special();

    memset(&retval, 0, sizeof(retval));

    my_dir = opendir("styles");
    if (my_dir) {
	while (1) { 
	    struct dirent *this_file = readdir(my_dir);
	    if (!this_file) break;
	    if (color_Select(this_file))
		i++;
	} 
	closedir(my_dir);
    } else {
	PANIC("Cannot read directory [styles/]");
    }
    my_dir = opendir("styles");
    if (my_dir) {
	if (i > 0) { 
	    int j;
	    Calloc(retval.style,color_style **,sizeof(*(retval.style))*i);
	    retval.num_style = i;
	    j = 0;
	    while (j<i) {
		struct dirent *this_file = readdir(my_dir);
		if (!color_Select(this_file)) continue;
		sprintf(filespec,"styles/%s",this_file->d_name);
		retval.style[j] = load_color_style(screen, filespec);
		if (strstr(retval.style[j]->name,"Default"))
		    retval.choice = j;
		j++;
	    }
	    closedir(my_dir);
	    return retval;
	} else {
	    PANIC("No piece styles [styles/*.Color] found.\n");
	}
    } else { 
	PANIC("Cannot read directory [styles/]");
    }
    return retval;
}

/***************************************************************************
 *      generate_piece()
 * Chooses the next piece in sequence. This involves assigning colors to
 * all of the tiles that make up the shape of the piece.
 *
 * Uses the color style because it needs to know the number of colors.
 *********************************************************************PROTO*/
play_piece
generate_piece(piece_style *ps, color_style *cs, unsigned int seq)
{
    unsigned int p,q,r,c;
    play_piece retval;

    SeedRandom(seq);

    p = ZEROTO(ps->num_piece);
    q = 2 + ZEROTO(cs->num_color - 1);
    r = 2 + ZEROTO(cs->num_color - 1);
    retval.base = &(ps->shape[p]);

    retval.special = No_Special;
    if (Options.special_wanted && ZEROTO(10000) < 2000) {
	switch (ZEROTO(4)) {
	    case 0: retval.special = Special_Bomb; /* bomb */
		    break;
	    case 1: retval.special = Special_Repaint; /* repaint */
		    break; 
	    case 2: retval.special = Special_Pushdown; /* repaint */
		    break;
	    case 3: retval.special = Special_Colorkill; /* trace and kill color */
		    break;
	}
    }
    if (retval.special != No_Special) {
	for (c=1;c<=(unsigned)ps->shape[p].num_color;c++) {
	    retval.colormap[c] = (unsigned char) retval.special;
	}
    } else for (c=1;c<=(unsigned)ps->shape[p].num_color;c++) {
	if (ZEROTO(100) < 25) 
	    retval.colormap[c] = q; 
	else
	    retval.colormap[c] = r; 
	Assert(retval.colormap[c] > 1);
    }
    return retval;
}

#define PRECOLOR_AT(pp,rot,i,j) BITMAP(*pp->base,rot,i,j)

/***************************************************************************
 *      draw_play_piece()
 * Draws a play piece on the screen.
 *
 * Needs the color style because it actually has to paste the color
 * bitmaps.
 *********************************************************************PROTO*/
void
draw_play_piece(SDL_Surface *screen, color_style *cs, 
	play_piece *o_pp, int o_x, int o_y, int o_rot,	/* old */
	play_piece *pp,int x, int y, int rot)		/* new */
{
    SDL_Rect dstrect;
    int i,j;
    int w,h;

    if (pp->special != No_Special)
	cs = &special_style;

    w = cs->w;
    h = cs->h;

    for (j=0;j<o_pp->base->dim;j++)
	for (i=0;i<o_pp->base->dim;i++) {
	    int what;
	    /* clear old */
	    if ((what = PRECOLOR_AT(o_pp,o_rot,i,j))) {
		dstrect.x = o_x + i * w;
		dstrect.y = o_y + j * h;
		dstrect.w = w;
		dstrect.h = h;
		SDL_BlitSafe(widget_layer,&dstrect,screen,&dstrect);
	    }
	}
    for (j=0;j<pp->base->dim;j++)
	for (i=0;i<pp->base->dim;i++) {
	    int this_precolor;
	    /* draw new */
	    if ((this_precolor = PRECOLOR_AT(pp,rot,i,j))) {
		int this_color = pp->colormap[this_precolor];
		dstrect.x = x + i * w;
		dstrect.y = y + j * h;
		dstrect.w = w;
		dstrect.h = h;
		SDL_BlitSafe(cs->color[this_color], NULL,screen,&dstrect) ;
		if (pp->special == No_Special)
		{
		    int that_precolor = (j == 0) ? 0 : 
			PRECOLOR_AT(pp,rot,i,j-1);

		    /* light up */
		    if (that_precolor == 0 ||
			    pp->colormap[that_precolor] != this_color) {
			dstrect.x = x + i * w;
			dstrect.y = y + j * h;
			dstrect.h = edge[HORIZ_LIGHT]->h;
			dstrect.w = edge[HORIZ_LIGHT]->w;
			SDL_BlitSafe(edge[HORIZ_LIGHT],NULL,
				    screen,&dstrect) ;
		    }

		    /* light left */
		    that_precolor = (i == 0) ? 0 :
			PRECOLOR_AT(pp,rot,i-1,j);
		    if (that_precolor == 0 ||
			    pp->colormap[that_precolor] != this_color) {
			dstrect.x = x + i * w;
			dstrect.y = y + j * h;
			dstrect.h = edge[VERT_LIGHT]->h;
			dstrect.w = edge[VERT_LIGHT]->w;
			SDL_BlitSafe(edge[VERT_LIGHT],NULL,
				    screen,&dstrect) ;
		    }
		    
		    /* shadow down */
		    that_precolor = (j == pp->base->dim-1) ? 0 :
			PRECOLOR_AT(pp,rot,i,j+1);
		    if (that_precolor == 0 ||
			    pp->colormap[that_precolor] != this_color) {
			dstrect.x = x + i * w;
			dstrect.y = (y + (j+1) * h) - edge[HORIZ_DARK]->h;
			dstrect.h = edge[HORIZ_DARK]->h;
			dstrect.w = edge[HORIZ_DARK]->w;
			SDL_BlitSafe(edge[HORIZ_DARK],NULL,
				    screen,&dstrect);
		    }

		    /* shadow right */
		    that_precolor = (i == pp->base->dim-1) ? 0 :
			PRECOLOR_AT(pp,rot,i+1,j);
		    if (that_precolor == 0 ||
			    pp->colormap[that_precolor] != this_color) {
			dstrect.x = (x + (i+1) * w) - edge[VERT_DARK]->w;
			dstrect.y = (y + (j) * h);
			dstrect.h = edge[VERT_DARK]->h;
			dstrect.w = edge[VERT_DARK]->w;
			SDL_BlitSafe(edge[VERT_DARK],NULL, screen,&dstrect);
		    }
		}
	    }
	}
    /* now update the entire relevant area */
    dstrect.x = min(o_x, x);
    dstrect.x = max( dstrect.x, 0 );

    dstrect.w = max(o_x + (o_pp->base->dim * cs->w), 
	    x + (pp->base->dim * cs->w)) - dstrect.x;
    dstrect.w = min( dstrect.w , screen->w - dstrect.x );

    dstrect.y = min(o_y, y);
    dstrect.y = max( dstrect.y, 0 );
    dstrect.h = max(o_y + (o_pp->base->dim * cs->h),
	    y + (pp->base->dim * cs->h)) - dstrect.y;
    dstrect.h = min( dstrect.h , screen->h - dstrect.y );

    SDL_UpdateSafe(screen,1,&dstrect);

    return;
}


/*
 * $Log: piece.c,v $
 * Revision 1.29  2000/11/06 04:16:10  weimer
 * "power pieces" plus images
 *
 * Revision 1.28  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.27  2000/11/06 01:25:54  weimer
 * add in the other special piece
 *
 * Revision 1.26  2000/11/06 00:24:01  weimer
 * add WalkRadioGroup modifications (inactive field for Kiri) and some support
 * for special pieces
 *
 * Revision 1.25  2000/11/03 04:25:58  weimer
 * add some optimizations to run_gravity to make it just a bit faster (down
 * to 0.01 ms/call from 0.02), sleep a bit more in event-loop: generally
 * trying to make us more CPU friendly ...
 *
 * Revision 1.24  2000/11/03 03:41:35  weimer
 * made the light and dark "edges" of pieces global, rather than part of a
 * specific color style. also fixed a bug where we were updating too much
 * when drawing falling pieces (bad min() code on my part)
 *
 * Revision 1.23  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.22  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.21  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.20  2000/10/29 00:06:27  weimer
 * networking fixes, change "styles/" to "styles" so that it works on Windows
 *
 * Revision 1.19  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.18  2000/10/19 22:06:51  weimer
 * minor changes ...
 *
 * Revision 1.17  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.16  2000/10/13 18:23:28  weimer
 * fixed a race condition in tetris_event()
 *
 * Revision 1.15  2000/10/13 17:55:36  weimer
 * Added another color "Style" ...
 *
 * Revision 1.14  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.13  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.12  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.11  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.10  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.9  2000/09/04 21:02:18  weimer
 * Added some addition piece layout code, garbage now falls unless the row
 * below it features some non-falling garbage
 *
 * Revision 1.8  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.7  2000/09/04 14:08:30  weimer
 * Added another color scheme and color/piece scheme scanning code.
 *
 * Revision 1.6  2000/09/03 21:06:31  wkiri
 * Now handles three different game types (and does different things).
 * Major changes in display.c to handle this.
 *
 * Revision 1.5  2000/09/03 18:26:11  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.4  2000/08/20 16:14:10  weimer
 * changed the piece generation so that pieces and colors cluster together
 *
 * Revision 1.3  2000/08/15 00:48:28  weimer
 * Massive sound rewrite, fixed a few memory problems. We now have
 * sound_styles that work like piece_styles and color_styles.
 *
 * Revision 1.2  2000/08/13 19:27:20  weimer
 * added file changelogs
 *
 */


samples_to_be_played current;	/* what should we play now? */

char *sound_name[NUM_SOUND] = { /* english names */
    "thud", "clear1", "clear4", "levelup", "leveldown" , "garbage1", "clock"
};

/***************************************************************************
 *      fill_audio()
 ***************************************************************************/
static void 
fill_audio(void *udata, Uint8 *stream, int len)
{
    int i;

    for (i=0; i<MAX_MIXED_SAMPLES; i++) /* for each possible sample to be mixed */
	if (current.sample[i].in_use) {
	    if (current.sample[i].delay >= len) /* keep pausing! */
		current.sample[i].delay -= len;
	    else if (current.sample[i].delay > 0 && 
		     current.sample[i].delay < len) {
		/* pause a bit! */
		int diff = len - current.sample[i].delay;
		if (diff + current.sample[i].pos >= current.sample[i].len) {
		    diff = current.sample[i].len - current.sample[i].pos;
		    current.sample[i].in_use = 0;
		}
		current.sample[i].delay = 0;
		SDL_MixAudio(stream+current.sample[i].delay, 
			current.sample[i].audio_data, diff, SDL_MIX_MAXVOLUME);
		current.sample[i].audio_data += diff;
		current.sample[i].pos += diff;
	    } else {
		/* we can play it now */
		int to_play = len;
		Assert(current.sample[i].delay == 0);
		if (to_play + current.sample[i].pos >= current.sample[i].len) {
		    to_play = current.sample[i].len - current.sample[i].pos;
		    current.sample[i].in_use = 0;
		}
		SDL_MixAudio(stream, current.sample[i].audio_data, 
			to_play, SDL_MIX_MAXVOLUME);
		current.sample[i].audio_data += to_play;
		current.sample[i].pos += to_play;
	    }
	} /* end: if in_use */
    return;
}

/***************************************************************************
 *      play_sound_unless_already_playing()
 * Schedules a sound to be played (unless it is already playing). "delay"
 * is in bytes (sigh!) at 11025 Hz, unsigned 8-bit samples, mono.
 *********************************************************************PROTO*/
void
play_sound_unless_already_playing(sound_style *ss, int which, int delay)
{
    int i;

    if (ss->WAV[which].audio_len == 0) {
	if (strcmp(ss->name,"No Sound"))
		Debug("No [%s] sound in Sound Style [%s]\n", 
		    sound_name[which], ss->name);
	return;
    }
    /* are we already playing? */
    for (i=0; i< MAX_MIXED_SAMPLES; i++) 
	if (current.sample[i].in_use != 0 &&
		!strcmp(current.sample[i].filename, ss->WAV[which].filename)) 
	    /* alreadying playing */
	    return; 

    /* find an empty slot */
    for (i=0; i< MAX_MIXED_SAMPLES; i++) {
	if (current.sample[i].in_use == 0) {
	    /* found it! */
	    SDL_LockAudio();
	    current.sample[i].in_use = 1;
	    current.sample[i].delay = delay;
	    current.sample[i].len = ss->WAV[which].audio_len;
	    current.sample[i].pos = 0;
	    current.sample[i].audio_data = ss->WAV[which].audio_buf;
	    current.sample[i].filename = ss->WAV[which].filename;
	    SDL_UnlockAudio();
	    /*
	    Debug("Scheduled sound %d with delay %d\n",which,delay);
	    */
	    return;
	}
    }
    Debug("No room in the mixer for sound %d\n",which);
    return;
}

/***************************************************************************
 *      stop_playing_sound()
 * Stops all occurences of the given sound from playing.
 *********************************************************************PROTO*/
void
stop_playing_sound(sound_style *ss, int which)
{
    int i;
    /* are we already playing? */
    for (i=0; i< MAX_MIXED_SAMPLES; i++) 
	if (current.sample[i].in_use != 0 &&
		!strcmp(current.sample[i].filename, ss->WAV[which].filename)) {
	    /* alreadying playing that sound */
	    current.sample[i].in_use = 0; /* turn it off */
	}
    return;

}

/***************************************************************************
 *      play_sound()
 * Schedules a sound to be played. "delay" is in bytes (sigh!) at 11025 Hz,
 * unsigned 8-bit samples, mono.
 *********************************************************************PROTO*/
void
play_sound(sound_style *ss, int which, int delay)
{
    int i;

    if (ss->WAV[which].audio_len == 0) {
	if (strcmp(ss->name,"No Sound"))
		Debug("No [%s] sound in Sound Style [%s]\n", 
		    sound_name[which], ss->name);
	return;
    }

    /* find an empty slot */
    for (i=0; i< MAX_MIXED_SAMPLES; i++) {
	if (current.sample[i].in_use == 0) {
	    /* found it! */
	    SDL_LockAudio();
	    current.sample[i].in_use = 1;
	    current.sample[i].delay = delay;
	    current.sample[i].len = ss->WAV[which].audio_len;
	    current.sample[i].pos = 0;
	    current.sample[i].audio_data = ss->WAV[which].audio_buf;
	    current.sample[i].filename = ss->WAV[which].filename;
	    SDL_UnlockAudio();
	    /*
	    Debug("Scheduled sound %d with delay %d\n",which,delay);
	    */
	    return;
	}
    }
    Debug("No room in the mixer for sound %d\n",which);
    return;
}

/***************************************************************************
 *      stop_all_playing()
 * Schedule all of the sounds associated with a given style to be played.
 *********************************************************************PROTO*/
void
stop_all_playing(void)
{
    int i;
    for (i=0; i<MAX_MIXED_SAMPLES; i++) /* for each possible sample to be mixed */
	current.sample[i].in_use = 0;
    return;
}

/***************************************************************************
 *      play_all_sounds()
 * Schedule all of the sounds associated with a given style to be played.
 *********************************************************************PROTO*/
void
play_all_sounds(sound_style *ss)
{
    int i;
    int delay = 0;

    for (i=0; i<NUM_SOUND; i++) {
	if (i == SOUND_CLOCK) continue;
	play_sound(ss, i, delay);
	delay += ss->WAV[i].audio_len + 6144;
    }
}

/***************************************************************************
 *      load_sound_style()
 * Parse a sound config file.
 ***************************************************************************/
static sound_style *
load_sound_style(const char *filename)
{
    sound_style *retval;
    char buf[2048];

    FILE *fin = fopen(filename,"rt");
    int ok;
    int count = 0;

    if (!fin) {
	Debug("fopen(%s)\n",filename);
	return NULL;
    }
    Calloc(retval,sound_style *,sizeof(sound_style));

    /* find the name */
    fgets(buf,sizeof(buf),fin);
    if (strchr(buf,'\n'))
	*(strchr(buf,'\n')) = 0;
    retval->name = strdup(buf);

    while (!(feof(fin))) {
	int i;
	do {
	    fgets(buf,sizeof(buf),fin);
	} while (!feof(fin) &&
		(buf[0] == '\n' || buf[0] == '#'));
	if (feof(fin)) break;
	if (strchr(buf,'\n'))
	    *(strchr(buf,'\n')) = 0;

	ok = 0;
		 
	for (i=0; i<NUM_SOUND; i++)
	    if (!strncasecmp(buf,sound_name[i],strlen(sound_name[i]))) {
		char *p = strchr(buf,' ');
		if (!p) {
		    Debug("malformed input line [%s] in [%s]\n",
			    buf,filename);
		    return NULL;
		}
		p++;
		if (!(SDL_LoadWAV(p,&(retval->WAV[i].spec), 
				&(retval->WAV[i].audio_buf),
				&(retval->WAV[i].audio_len)))) {
		    PANIC("Couldn't open %s [%s] in [%s]: %s",
			    sound_name[i], p, filename, SDL_GetError());
		}
		retval->WAV[i].filename = strdup(filename);
		count++;
		ok = 1;
	    }
	if (!ok) {
	    Debug("unknown sound name [%s] in [%s]\nvalid names:",
		    buf, filename);
	    for (i=0;i<NUM_SOUND;i++)
		printf(" %s",sound_name[i]);
	    printf("\n");
	    return NULL;
	}
    }

    Debug("Sound Style [%s] loaded (%d/%d sounds).\n",retval->name,
	    count, NUM_SOUND);

    return retval;
}

/***************************************************************************
 *	sound_Select()
 * Returns 1 if the file pointed to ends with ".Sound" 
 * Used by scandir() to grab all the *.Sound files from a directory. 
 ***************************************************************************/
static int
sound_Select(const struct dirent *d)
{
    if (strstr(d->d_name,".Sound") && 
	    (signed)strlen(d->d_name) == 
	    (strstr(d->d_name,".Sound") - d->d_name + 6))
	return 1;
    else 
	return 0; 
}

/***************************************************************************
 *      load_sound_styles()
 * Load all sound styles and start the sound driver. If sound is not
 * wanted, return a dummy style.
 *********************************************************************PROTO*/
sound_styles
load_sound_styles(int sound_wanted)
{
    sound_styles retval;
    SDL_AudioSpec wanted;
    DIR *my_dir;
    int i = 0;

    memset(&retval,0,sizeof(retval));

    /* do we even want sound? */
    if (!sound_wanted) goto nosound;

    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
	Debug("Couldn't initialize audio subsystem: %s\n",SDL_GetError());
	goto nosound;
    }

    memset(&current,0,sizeof(current));	/* clear memory */

    wanted.freq = 11025;
    wanted.format = AUDIO_U8;
    wanted.channels = 1;
    wanted.samples = 512; /* good low-latency value for callback */
    wanted.callback = fill_audio;
    wanted.userdata = NULL;
    /* Open the audio device, forcing the desired format */
    if (SDL_OpenAudio(&wanted, NULL) < 0) {
	Debug("Couldn't open audio: %s\n",SDL_GetError());
	goto nosound;
    }

    my_dir = opendir("sounds");
    if (my_dir) {
	while (1) { 
	    struct dirent *this_file = readdir(my_dir);
	    if (!this_file) break;
	    if (sound_Select(this_file))
		i++;
	} 
	closedir(my_dir);
    } else {
	Debug("Cannot read directory [sounds/]: atris-sounds not found!\n");
	goto nosound;
    }
    my_dir = opendir("sounds");
    if (my_dir) {
	if (i >= 0) { 
	    int j;
	    Calloc(retval.style,sound_style **,sizeof(*(retval.style))*i+1);
	    retval.num_style = i+1;
	    j = 0;
	    while (j<i) {
		char filespec[1024];
		struct dirent *this_file = readdir(my_dir);
		if (!sound_Select(this_file)) continue;
		sprintf(filespec,"sounds/%s",this_file->d_name);
		retval.style[j] = load_sound_style(filespec);
		if (strstr(retval.style[j]->name,"Default"))
		    retval.choice = j;
		j++;
	    }
	    closedir(my_dir);
	    Calloc(retval.style[i],sound_style *,sizeof(sound_style));
	    retval.style[i]->name = "No Sound";
	    SDL_PauseAudio(0);	/* start playing sound! */
	    return retval;
	} else {
	    PANIC("No sound styles [sounds/*.Sound] found.\n");
	}
    } else { 
	PANIC("Cannot read directory [sounds/]");
    }
    Debug("No sound styles [sounds/*.Sound] found.\n");

    /* for whatever reason, you don't get sound */
nosound: 
    retval.num_style = 1;
    Malloc(retval.style,sound_style **,sizeof(*(retval.style))*1);
    Calloc(retval.style[0],sound_style *,sizeof(sound_style));
    retval.style[0]->name = "No Sound";
    return retval;
}

/*
 * $Log: sound.c,v $
 * Revision 1.25  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.24  2000/11/01 16:28:53  weimer
 * sounds removed ...
 *
 * Revision 1.23  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.22  2000/10/29 00:06:27  weimer
 * networking fixes, change "styles/" to "styles" so that it works on Windows
 *
 * Revision 1.21  2000/10/27 00:07:36  weimer
 * some sound fixes ...
 *
 * Revision 1.20  2000/10/22 22:05:51  weimer
 * Added a few new sounds ...
 *
 * Revision 1.19  2000/10/21 01:14:43  weimer
 * massic autoconf/automake restructure ...
 *
 * Revision 1.18  2000/10/20 01:32:09  weimer
 * Minor play issue problems -- time is now truly a hard limit!
 *
 * Revision 1.17  2000/10/19 00:20:27  weimer
 * sound directory changes, added a ticking clock ...
 *
 * Revision 1.16  2000/10/12 01:38:07  weimer
 * added initial support for persistent player identities
 *
 * Revision 1.15  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.14  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.13  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.12  2000/09/04 14:08:30  weimer
 * Added another color scheme and color/piece scheme scanning code.
 *
 * Revision 1.11  2000/09/03 21:08:23  weimer
 * Added alternate sounds, we now actually load *.Sound ...
 *
 * Revision 1.10  2000/09/03 20:57:38  weimer
 * quick fix for sound.c
 *
 * Revision 1.9  2000/09/03 20:57:02  weimer
 * changes the time variable to be passed by reference in the event loop
 *
 * Revision 1.8  2000/09/03 18:26:11  weimer
 * major header file and automatic prototype generation changes, restructuring
 *
 * Revision 1.7  2000/08/26 02:35:23  weimer
 * add sounds when you are given garbage
 *
 * Revision 1.6  2000/08/26 00:56:47  weimer
 * client-server mods, plus you can now disable sound
 *
 * Revision 1.5  2000/08/20 16:14:10  weimer
 * changed the piece generation so that pieces and colors cluster together
 *
 * Revision 1.4  2000/08/15 00:59:41  weimer
 * a few more sound things
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


/*****************************************************************************/
/*                               XFlame v1.0                                 */
/*****************************************************************************/
/* By:                                                                       */
/*     The Rasterman (Carsten Haitzler)                                      */
/*      Copyright (C) 1996                                                   */
/*****************************************************************************/
/* Ported to SDL by:                                                         */
/*     Sam Lantinga                                                          */
/*                                                                           */
/* This is a dirty port, just to get it working on SDL.                      */
/* Improvements left to the reader:                                          */
/* 	Use depth-specific code to optimize HiColor/TrueColor display        */
/* 	Fix the delta code -- it's broken -- shame on Rasterman ;-)          */
/*                                                                           */
/*****************************************************************************/
/* Modified for use in Atris by: 
 * 	Wes Weimer
 *****************************************************************************/
/*****************************************************************************/
/* This code is Freeware. You may copy it, modify it or do with it as you    */
/* please, but you may not claim copyright on any code wholly or partly      */
/* based on this code. I accept no responisbility for any consequences of    */
/* using this code, be they proper or otherwise.                             */
/*****************************************************************************/
/* Okay, now all the legal mumbo-jumbo is out of the way, I will just say    */
/* this: enjoy this program, do with it as you please and watch out for more */
/* code releases from The Rasterman running under X... the only way to code. */
/*****************************************************************************/

/* DEFINES */
#define GEN 500
#define MAX 300
#define WID 80
#define HIH 60
#define HSPREAD 26
#define VSPREAD 78
#define VFALLOFF 14
#define VARIANCE 5
#define VARTREND 2
#define RESIDUAL 68

#define NONE 0x00
#define CMAP 0x02
#define DELT 0x08		/* Delta code is broken -- Rasterman? */
#define BLOK 0x20
#define LACE 0x40
/*This structure contains all of the "Global" variables for my program so */
/*that I just pass a pointer to my functions, not a million parameters */
struct globaldata
{
  Uint32 flags;
  SDL_Surface *screen;
  int nrects;
  SDL_Rect *rects;
};

int powerof(unsigned int n)
{
  /* This returns the power of a number (eg powerof(8)==3, powerof(256)==8, */
  /* powerof(1367)==11, powerof(2568)==12) */
  int p;
  if (n<=0x00000001) p=0;
  else if (n<=0x00000002) p=1;
  else if (n<=0x00000004) p=2;
  else if (n<=0x00000008) p=3;
  else if (n<=0x00000010) p=4;
  else if (n<=0x00000020) p=5;
  else if (n<=0x00000040) p=6;
  else if (n<=0x00000080) p=7;
  else if (n<=0x00000100) p=8;
  else if (n<=0x00000200) p=9;
  else if (n<=0x00000400) p=10;
  else if (n<=0x00000800) p=11;
  else if (n<=0x00001000) p=12;
  else if (n<=0x00002000) p=13;
  else if (n<=0x00004000) p=14;
  else if (n<=0x00008000) p=15;
  else if (n<=0x00010000) p=16;
  else if (n<=0x00020000) p=17;
  else if (n<=0x00040000) p=18;
  else if (n<=0x00080000) p=19;
  else if (n<=0x00100000) p=20;
  else if (n<=0x00200000) p=21;
  else if (n<=0x00400000) p=22;
  else if (n<=0x00800000) p=23;
  else if (n<=0x01000000) p=24;
  else if (n<=0x02000000) p=25;
  else if (n<=0x04000000) p=26;
  else if (n<=0x08000000) p=27;
  else if (n<=0x10000000) p=28;
  else if (n<=0x20000000) p=29;
  else if (n<=0x40000000) p=30;
  else if (n<=0x80000000) p=31;
  else p = 32;
  return p;
}

static void
SetFlamePalette(struct globaldata *gb, int f,int *ctab)
{
  /*This function sets the flame palette */
  int r,g,b,i;
  SDL_Color cmap[MAX];
  
  /* This step is only needed on palettized screens */
  r = g = b = 0;
  for (i=0; (r != 255) || (g != 255) || (b != 255); i++)
    {
      r=i*3;
      g=(i-80)*3;
      b=(i-160)*3;
      if (r<0) r=0;
      if (r>255) r=255;
      if (g<0) g=0;
      if (g>255) g=255;
      if (b<0) b=0;
      if (b>255) b=255;
      cmap[i].r = r;
      cmap[i].g = g;
      cmap[i].b = b;
    }
  SDL_SetColors(gb->screen, cmap, 0, i);

  /* This step is for all depths */
  for (i=0;i<MAX;i++)
    {
      r=i*3;
      g=(i-80)*3;
      b=(i-160)*3;
      if (r<0) r=0;
      if (r>255) r=255;
      if (g<0) g=0;
      if (g>255) g=255;
      if (b<0) b=0;
      if (b>255) b=255;
      ctab[i]=SDL_MapRGB(gb->screen->format, (Uint8)r, (Uint8)g, (Uint8)b);
    }
}

static void
XFSetRandomFlameBase(int *f, int w, int ws, int h)
{
  /*This function sets the base of the flame to random values */
  int x,y,*ptr;
  
  /* initialize a random number seed from the time, so we get random */
  /* numbers each time */
  SeedRandom(0);
  y=h-1;
  for (x=0;x<w;x++)
    {
      ptr=f+(y<<ws)+x;
      *ptr = FastRandom(MAX);
    }
}

static void
XFModifyFlameBase(int *f, int w, int ws, int h)
{
  /*This function modifies the base of the flame with random values */
  int x,y,*ptr,val;
  
  y=h-1;
  for (x=0;x<w;x++)
    {
      ptr=f+(y<<ws)+x;
      *ptr+=((FastRandom(VARIANCE))-VARTREND);
      val=*ptr;
      if (val>MAX) *ptr=0;
      if (val<0) *ptr=0;
    }
}

static void
XFProcessFlame(int *f, int w, int ws, int h, int *ff)
{
  /*This function processes entire flame array */
  int x,y,*ptr,*p,tmp,val;
  
  for (y=(h-1);y>=2;y--)
    {
      for (x=1;x<(w-1);x++)
	{
	  ptr=f+(y<<ws)+x;
	  val=(int)*ptr;
	  if (val>MAX) *ptr=(int)MAX;
	  val=(int)*ptr;
	  if (val>0)
	    {
	      tmp=(val*VSPREAD)>>8;
	      p=ptr-(2<<ws);				
	      *p=*p+(tmp>>1);
	      p=ptr-(1<<ws);				
	      *p=*p+tmp;
	      tmp=(val*HSPREAD)>>8;
	      p=ptr-(1<<ws)-1;
	      *p=*p+tmp;
	      p=ptr-(1<<ws)+1;
	      *p=*p+tmp;
	      p=ptr-1;
	      *p=*p+(tmp>>1);
	      p=ptr+1;
	      *p=*p+(tmp>>1);
	      p=ff+(y<<ws)+x;
	      *p=val;
	      if (y<(h-1)) *ptr=(val*RESIDUAL)>>8;
	    }
	}
    }
}

static void
XFDrawFlameBLOK(struct globaldata *g,int *f, int w, int ws, int h, int *ctab)
{
  /*This function copies & displays the flame image in one large block */
  int x,y,*ptr,xx,yy,cl,cl1,cl2,cl3,cl4;
  unsigned char *cptr,*im,*p;
  
  /* get pointer to the image data */
  if ( SDL_LockSurface(g->screen) < 0 )
    return;

  /* copy the calculated flame array to the image buffer */
  im=(unsigned char *)g->screen->pixels;
  for (y=0;y<(h-1);y++)
    {
      for (x=0;x<(w-1);x++)
	{
	  xx=x<<1;
	  yy=y<<1;
	  ptr=f+(y<<ws)+x;
	  cl1=cl=(int)*ptr;
	  ptr=f+(y<<ws)+x+1;
	  cl2=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x+1;
	  cl3=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x;
	  cl4=(int)*ptr;
	  cptr=im+yy*g->screen->pitch+xx;
	  *cptr=(unsigned char)ctab[cl%MAX];
	  p=cptr+1;
	  *p=(unsigned char)ctab[((cl1+cl2)>>1)%MAX];
	  p=cptr+1+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl3)>>1)%MAX];
	  p=cptr+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl4)>>1)%MAX];
	}
    }
  SDL_UnlockSurface(g->screen);

  /* copy the image to the screen in one large chunk */
  SDL_Flip(g->screen);
}

static void
XFUpdate(struct globaldata *g)
{
	if ( (g->screen->flags & SDL_DOUBLEBUF) == SDL_DOUBLEBUF ) {
		SDL_Flip(g->screen);
	} else {
	    int i;
	    extern SDL_Surface * screen, * widget_layer, *flame_layer;
	    for (i=0; i<g->nrects; i++) {
		SDL_BlitSurface(flame_layer, &g->rects[i],
			screen, &g->rects[i]);
		SDL_BlitSurface(widget_layer, &g->rects[i],
			screen, &g->rects[i]);
	    }
	    SDL_UpdateRects(screen, g->nrects, g->rects);
	}
}

static void
XFDrawFlameLACE(struct globaldata *g,int *f, int w, int ws, int h, int *ctab)
{
  /*This function copies & displays the flame image in interlaced fashion */
  /*that it, it first processes and copies the even lines to the screen, */
  /* then is processes and copies the odd lines of the image to the screen */
  int x,y,*ptr,xx,yy,cl,cl1,cl2,cl3,cl4;
  unsigned char *cptr,*im,*p;
  
  /* get pointer to the image data */
  if ( SDL_LockSurface(g->screen) < 0 )
    return;

  /* copy the calculated flame array to the image buffer */
  im=(unsigned char *)g->screen->pixels;
  for (y=0;y<(h-1);y++)
    {
      for (x=0;x<(w-1);x++)
	{
	  xx=x<<1;
	  yy=y<<1;
	  ptr=f+(y<<ws)+x;
	  cl1=cl=(int)*ptr;
	  ptr=f+(y<<ws)+x+1;
	  cl2=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x+1;
	  cl3=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x;
	  cl4=(int)*ptr;
	  cptr=im+yy*g->screen->pitch+xx;
	  *cptr=(unsigned char)ctab[cl%MAX];
	  p=cptr+1;
	  *p=(unsigned char)ctab[((cl1+cl2)>>1)%MAX];
	  p=cptr+1+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl3)>>1)%MAX];
	  p=cptr+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl4)>>1)%MAX];
	}
    }
  SDL_UnlockSurface(g->screen);

  /* copy the even lines to the screen */
  w <<= 1;
  h <<= 1;
  g->nrects = 0;
  for (y=0;y<(h-1);y+=4)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
  /* copy the odd lines to the screen */
  g->nrects = 0;
  for (y=2;y<(h-1);y+=4)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
  /* copy the even lines to the screen */
  g->nrects = 0;
  for (y=1;y<(h-1);y+=4)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
  /* copy the odd lines to the screen */
  g->nrects = 0;
  for (y=3;y<(h-1);y+=4)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
}

static void
XFDrawFlame(struct globaldata *g,int *f, int w, int ws, int h, int *ctab)
{
  /*This function copies & displays the flame image in interlaced fashion */
  /*that it, it first processes and copies the even lines to the screen, */
  /* then is processes and copies the odd lines of the image to the screen */
  int x,y,*ptr,xx,yy,cl,cl1,cl2,cl3,cl4;
  unsigned char *cptr,*im,*p;
  
  /* get pointer to the image data */
  if ( SDL_LockSurface(g->screen) < 0 )
    return;

  /* copy the calculated flame array to the image buffer */
  im=(unsigned char *)g->screen->pixels;
  for (y=0;y<(h-1);y++)
    {
      for (x=0;x<(w-1);x++)
	{
	  xx=x<<1;
	  yy=y<<1;
	  ptr=f+(y<<ws)+x;
	  cl1=cl=(int)*ptr;
	  ptr=f+(y<<ws)+x+1;
	  cl2=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x+1;
	  cl3=(int)*ptr;
	  ptr=f+((y+1)<<ws)+x;
	  cl4=(int)*ptr;
	  cptr=im+yy*g->screen->pitch+xx;
	  *cptr=(unsigned char)ctab[cl%MAX];
	  p=cptr+1;
	  *p=(unsigned char)ctab[((cl1+cl2)>>1)%MAX];
	  p=cptr+1+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl3)>>1)%MAX];
	  p=cptr+g->screen->pitch;
	  *p=(unsigned char)ctab[((cl1+cl4)>>1)%MAX];
	}
    }
  SDL_UnlockSurface(g->screen);

  /* copy the even lines to the screen */
  w <<= 1;
  h <<= 1;
  g->nrects = 0;
  for (y=0;y<(h-1);y+=2)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
  /* copy the odd lines to the screen */
  g->nrects = 0;
  for (y=1;y<(h-1);y+=2)
    {
      g->rects[g->nrects].x = 0;
      g->rects[g->nrects].y = y;
      g->rects[g->nrects].w = w;
      g->rects[g->nrects].h = 1;
      ++g->nrects;
    }
  XFUpdate(g);
}


static int *flame,flamesize,ws,flamewidth,flameheight,*flame2;
static struct globaldata *g;
static int w, h, f, *ctab;

/***************************************************************************
 *********************************************************************PROTO*/
void 
atris_run_flame(void)
{
    if (!Options.flame_wanted) return;

    /* modify the bas of the flame */
    XFModifyFlameBase(flame,w>>1,ws,h>>1);
    /* process the flame array, propagating the flames up the array */
    XFProcessFlame(flame,w>>1,ws,h>>1,flame2);
    /* if the user selected BLOCK display method, then display the flame */
    /* all in one go, no fancy upating techniques involved */
    if (f&BLOK)
    {
	XFDrawFlameBLOK(g,flame2,w>>1,ws,h>>1,ctab);
    }
    else if (f&LACE)
	/* the default of displaying the flames INTERLACED */
    {
	XFDrawFlameLACE(g,flame2,w>>1,ws,h>>1,ctab);
    }
    else
    {
	XFDrawFlame(g,flame2,w>>1,ws,h>>1,ctab);
    }
}

static int Xflame(struct globaldata *_g,int _w, int _h, int _f, int *_ctab)
{

  g = _g; w = _w; h = _h; f = _f; ctab = _ctab;

  /*This function is the hub of XFlame.. it initialises the flame array, */
  /*processes the array, genereates the flames and displays them */
  
  /* workout the size needed for the flame array */
  flamewidth=w>>1;
  flameheight=h>>1;
  ws=powerof(flamewidth);
  flamesize=(1<<ws)*flameheight*sizeof(int);
  /* allocate the memory for the flame array */
  flame=(int *)malloc(flamesize);
  /* if we didn't get the memory, return 0 */
  if (!flame) return 0;
  memset(flame, 0, flamesize);
  /* allocate the memory for the second flame array */
  flame2=(int *)malloc(flamesize);
  /* if we didn't get the memory, return 0 */
  if (!flame2) return 0;
  memset(flame2, 0, flamesize);
  if (f&BLOK)
    {
      g->rects = NULL;
    }
  else if (f&LACE)
    {
      /* allocate the memory for update rectangles */
      g->rects=(SDL_Rect *)malloc((h>>2)*sizeof(SDL_Rect));
      /* if we couldn't get the memory, return 0 */
      if (!g->rects) return 0;
    }
  else
    {
      /* allocate the memory for update rectangles */
      g->rects=(SDL_Rect *)malloc((h>>1)*sizeof(SDL_Rect));
      /* if we couldn't get the memory, return 0 */
      if (!g->rects) return 0;
    }
  /* set the base of the flame to something random */
  XFSetRandomFlameBase(flame,w>>1,ws,h>>1);
  /* now loop, generating and displaying flames */
#if 0
  for (done=0; !done; )
    {
    }
    return(done);
#endif
    return 0;
}

static struct globaldata glob;
static int flags;
static int colortab[MAX];

/***************************************************************************
 *********************************************************************PROTO*/
void
atris_xflame_setup(void)
{
    extern SDL_Surface *flame_layer;

    memset(&glob, 0, sizeof(glob));
    /*
    glob.flags |= SDL_HWSURFACE;
    */
    glob.flags |= SDL_HWPALETTE;
    glob.screen = flame_layer; 

    SetFlamePalette(&glob,flags,colortab);
    Xflame(&glob,glob.screen->w,glob.screen->h,flags,colortab);
}




/***************************************************************************
 *      main()
 * Start the program, check the arguments, etc.
 ***************************************************************************/
int
main(int argc, char *argv[])
{
    color_styles cs;
    piece_styles ps;
    sound_styles ss;
    identity *id;
    AI_Players *ai;
    Grid g[2];
    int renderstyle = TTF_STYLE_NORMAL;
    int flags;
    Uint32 time_now;
    SDL_Event event;

    umask(000);
    Debug("\tWelcome to Alizarin Tetris (version %s - %s)\n", VERSION, "Delay" );
    Debug("\t~~~~~~~~~~~~~~~~~~~~~~~~~~ (%s)\n", __DATE__);

#ifdef HAVE_WINSOCK_H
    load_options("atris.rc");
#else
    {
	char filespec[2048];
	sprintf(filespec,"%s/.atrisrc", getenv("HOME"));
	load_options(filespec);
    }
#endif
    parse_options(argc, argv);

    if (SDL_Init(SDL_INIT_VIDEO)) 
	PANIC("SDL_Init failed!");

    /* Clean up on exit */
    atexit(SDL_Quit);
    /* Initialize the TTF library */
    if ( TTF_Init() < 0 ) PANIC("TTF_Init failed!"); atexit(TTF_Quit);
    Debug("SDL initialized.\n");

    /* Initialize the display in a 640x480 native-depth mode */
    flags = SDL_HWSURFACE | SDL_HWPALETTE | SDL_SRCCOLORKEY | SDL_ANYFORMAT; 
    if (Options.full_screen) flags |= SDL_FULLSCREEN;
    screen = SDL_SetVideoMode(640, 480, Options.bpp_wanted, flags);
    if ( screen == NULL ) PANIC("Could not set 640x480 video mode");
    Debug("Video Mode: %d x %d @ %d bpp\n",
	    screen->w, screen->h, screen->format->BitsPerPixel);

    if (screen->format->BitsPerPixel <= 8) {
	PANIC("You need >256 colors to play atris");
    }

    /* Set the window title */
    SDL_WM_SetCaption("Alizarin Tetris", (char*)NULL);

    Network_Init();

    setup_colors(screen);
    setup_layers(screen);
    
    if (chdir(ATRIS_LIBDIR)) {
	Debug("WARNING: cannot change directory to [%s]\n", ATRIS_LIBDIR);
	Debug("WARNING: playing in current directory instead\n");
    } else 
	Debug("Changing directory to [%s]\n",ATRIS_LIBDIR);

    /* Set up the font */
    sfont = TTF_OpenFont("graphics/NewMediumNormal.ttf",18);
     font = TTF_OpenFont("graphics/NewMediumNormal.ttf",24);
    lfont = TTF_OpenFont("graphics/NewMediumNormal.ttf",36);
    hfont = TTF_OpenFont("graphics/NewMediumNormal.ttf",96);
    if ( font == NULL ) PANIC("Couldn't open [graphics/NewMediumNormal.ttf].", ATRIS_LIBDIR); 
    TTF_SetFontStyle(font, renderstyle);
    TTF_SetFontStyle(sfont, renderstyle);
    /* Initialize scores */
    Score[0] = Score[1] = 0;
    /* enable unicode so we can easily see which key they pressed */
    SDL_EnableUNICODE(1);

    ps = load_piece_styles();
    cs = load_color_styles(screen);
    ss = load_sound_styles(Options.sound_wanted);

    if (Options.named_color >= 0 && Options.named_color <
	    cs.num_style) cs.choice = Options.named_color;
    if (Options.named_piece >= 0 && Options.named_piece <
	    ps.num_style) ps.choice = Options.named_piece;
    if (Options.named_sound >= 0 && Options.named_sound <
	    ss.num_style) ss.choice = Options.named_sound;
    if (Options.named_game >= 0 && Options.named_game <= 5) 
	gametype = (GT) Options.named_game;
    else gametype = SINGLE;

    ai = AI_Players_Setup();
    id = load_identity_file();

    atris_xflame_setup();

    /* our happy splash screen */
    { 
	while (SDL_PollEvent(&event))
	    ;
	draw_string(PACKAGE " - " VERSION, color_white, screen->w, 0,
		DRAW_LEFT | DRAW_UPDATE);
	draw_string("Welcome To", color_purple, screen->w/2,
		screen->h/2, DRAW_CENTER | DRAW_UPDATE | DRAW_ABOVE | DRAW_LARGE);
	draw_string("Alizarin Tetris", color_blue, screen->w/2,
		screen->h/2, DRAW_CENTER | DRAW_UPDATE | DRAW_HUGE);
    }

    time_now = SDL_GetTicks();
    do { 
	poll_and_flame(&event);
    } while (SDL_GetTicks() < time_now + 600);

    clear_screen_to_flame();	/* lose the "welcome to" words */

    while (1) {
	int p1, p2;
	int retval;

	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY*10,
		SDL_DEFAULT_REPEAT_INTERVAL);
	choose_gametype(&ps,&cs,&ss,ai);
	if (gametype == QUIT) break;
	else Options.named_game = (int)gametype;
	/* play our game! */

	Score[0] = Score[1] = 0;
	SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY*10,
		SDL_DEFAULT_REPEAT_INTERVAL);

	/* what sort of game would you like? */
	switch (gametype) {
	    case SINGLE:
		p1 = who_are_you(screen, &id, -1, 1);
		clear_screen_to_flame();
		if (p1 < 0) break;
		id->p[p1].level = play_SINGLE(cs,ps,ss,g,&id->p[p1]);
		clear_screen_to_flame();
		break;
	    case MARATHON:
		p1 = who_are_you(screen, &id, -1, 1);
		clear_screen_to_flame();
		if (p1 < 0) break;
		retval = play_MARATHON(cs,ps,ss,g,&id->p[p1]);
		clear_screen_to_flame();
		/* it's high score time */
		SDL_EnableKeyRepeat(SDL_DEFAULT_REPEAT_DELAY*10,
			SDL_DEFAULT_REPEAT_INTERVAL);
		high_score_check(retval, Score[0]);
		clear_screen_to_flame();
		break;
	    case SINGLE_VS_AI:
		p1 = who_are_you(screen, &id, -1, 1);
		clear_screen_to_flame();
		if (p1 < 0) break;
		p2 = pick_an_ai(screen, "As Your Opponent", ai);
		if (p2 < 0) break;
		ai->player[p2].delay_factor = pick_ai_factor(screen);
		clear_screen_to_flame();
		id->p[p1].level = play_SINGLE_VS_AI(cs,ps,ss,g,
			&id->p[p1], &ai->player[p2]);
		clear_screen_to_flame();
		break;
	    case AI_VS_AI:
		p1 = pick_an_ai(screen, "As Player 1", ai);
		clear_screen_to_flame();
		if (p1 < 0) break;
		p2 = pick_an_ai(screen, "As Player 2", ai);
		clear_screen_to_flame();
		if (p2 < 0) break;
		play_AI_VS_AI(cs,ps,ss,g,
			&ai->player[p1], &ai->player[p2]);
		clear_screen_to_flame();
		break;
	    case TWO_PLAYERS:
		p1 = who_are_you(screen, &id, -1, 1);
		clear_screen_to_flame();
		if (p1 < 0) break;
		p2 = who_are_you(screen, &id, p1, 2);
		clear_screen_to_flame();
		if (p2 < 0) break;
		play_TWO_PLAYERS(cs,ps,ss,g, &id->p[p1], &id->p[p2]);
		clear_screen_to_flame();
		break;
	    case NETWORK: 
		p1 = who_are_you(screen, &id, -1, 1);
		clear_screen_to_flame();
		if (p1 < 0) break;

		id->p[p1].level = 
		    play_NETWORK(cs,ps,ss,g,&id->p[p1],network_choice(screen));
		clear_screen_to_flame();
		break;
	    default:
		break;
	}

	/*
	 * save those updates!
	 */
	save_identity_file(id, NULL, 0);
    } /* end: while(choose_gametype() != -1) */
    SDL_CloseAudio();
    TTF_CloseFont(sfont);
    TTF_CloseFont( font);
    TTF_CloseFont(lfont);
    TTF_CloseFont(hfont);
    Network_Quit();

    Options.named_color = cs.choice;
    Options.named_piece = ps.choice;
    Options.named_sound = ss.choice;

#ifdef HAVE_WINSOCK_H
    save_options("atris.rc");
#else
    {
	char filespec[2048];
	sprintf(filespec,"%s/.atrisrc", getenv("HOME"));
	save_options(filespec);
    }
#endif
    return 0;
}

/*
 * $Log: atris.c,v $
 * Revision 1.100  2001/01/06 00:30:55  weimer
 * yada
 *
 * Revision 1.99  2001/01/06 00:25:23  weimer
 * changes so that we remember things ...
 *
 * Revision 1.98  2001/01/05 21:38:01  weimer
 * more key changes
 *
 * Revision 1.97  2001/01/05 21:36:28  weimer
 * little change ...
 *
 * Revision 1.96  2001/01/05 21:12:31  weimer
 * advance to atris 1.0.5, add support for ".atrisrc" and changing the
 * keyboard repeat rate
 *
 * Revision 1.95  2000/11/10 18:16:48  weimer
 * changes for Atris 1.0.4 - three new special options
 *
 * Revision 1.94  2000/11/06 04:41:47  weimer
 * fixed up Nirgal to Olympus
 *
 * Revision 1.93  2000/11/06 04:39:56  weimer
 * networking consistency check for power pieces
 *
 * Revision 1.92  2000/11/06 04:06:44  weimer
 * option menu
 *
 * Revision 1.91  2000/11/06 01:22:40  wkiri
 * Updated menu system.
 *
 * Revision 1.90  2000/11/03 03:41:35  weimer
 * made the light and dark "edges" of pieces global, rather than part of a
 * specific color style. also fixed a bug where we were updating too much
 * when drawing falling pieces (bad min() code on my part)
 *
 * Revision 1.89  2000/11/02 03:06:20  weimer
 * better interface for walk-radio menus: we are now ready for Kiri to change
 * them to add run-time options ...
 *
 * Revision 1.88  2000/11/01 03:53:06  weimer
 * modifications for version 1.0.1: you can pick your starting level, you can
 * pick the AI difficulty factor, the game is better about placing new pieces
 * when there is garbage, when things fall out of your control they now fall
 * at a uniform rate ...
 *
 * Revision 1.87  2000/10/30 16:25:25  weimer
 * display the network player score during network games. Also give the
 * non-server a little message when waiting for the server to go on.
 *
 * Revision 1.86  2000/10/30 03:49:09  weimer
 * minor changes ...
 *
 * Revision 1.85  2000/10/29 22:55:01  weimer
 * networking consistency checks (you must have the same number of doodads):
 * special hotkey 'f' in main menu toggles full screen mode
 * added depth specification on the command line
 * automatically search for the darkest non-black color ...
 *
 * Revision 1.84  2000/10/29 21:28:58  weimer
 * fixed a few failures to clear the screen if we didn't have a flaming
 * backdrop
 *
 * Revision 1.83  2000/10/29 21:23:28  weimer
 * One last round of header-file changes to reflect my newest and greatest
 * knowledge of autoconf/automake. Now if you fail to have some bizarro
 * function, we try to go on anyway unless it is vastly needed.
 *
 * Revision 1.82  2000/10/29 19:04:32  weimer
 * minor highscore handling changes: new filename, use the draw_string() and
 * draw_bordered_rect() and input_string() interfaces, handle the widget layer
 * and the flame layer, etc. Also fix a minor bug where you would be prevented
 * from settling if you pressed a key even if it didn't really move you. :-)
 *
 * Revision 1.81  2000/10/29 17:23:13  weimer
 * incorporate "xflame" flaming background for added spiffiness ...
 *
 * Revision 1.80  2000/10/29 00:17:39  weimer
 * added support for a system independent random number generator
 *
 * Revision 1.79  2000/10/29 00:06:27  weimer
 * networking fixes, change "styles/" to "styles" so that it works on Windows
 *
 * Revision 1.78  2000/10/28 23:39:24  weimer
 * added initialization support for Winsock 1.1 networking, ala SDL_net
 *
 * Revision 1.77  2000/10/28 16:40:17  weimer
 * Further changes: we can now build .tar.gz files and RPMs!
 *
 * Revision 1.76  2000/10/28 13:39:14  weimer
 * added a pausing feature ...
 *
 * Revision 1.75  2000/10/27 19:39:49  weimer
 * doubled the level for random AI deathmatches ...
 *
 * Revision 1.74  2000/10/21 01:14:42  weimer
 * massive autoconf/automake restructure ...
 *
 * Revision 1.73  2000/10/20 21:17:37  weimer
 * re-added the lemmings sound style ...
 *
 * Revision 1.72  2000/10/20 01:32:09  weimer
 * Minor play issue problems -- time is now truly a hard limit!
 *
 * Revision 1.71  2000/10/19 01:04:46  weimer
 * consistency error
 *
 * Revision 1.70  2000/10/19 00:20:27  weimer
 * sound directory changes, added a ticking clock ...
 *
 * Revision 1.69  2000/10/18 23:57:49  weimer
 * general fixup, color changes, display changes.
 * Notable: "Safe" Blits and Updates now perform "clipping". No more X errors,
 * we hope!
 *
 * Revision 1.68  2000/10/18 02:12:37  weimer
 * network fix
 *
 * Revision 1.67  2000/10/18 02:07:16  weimer
 * network play improvements
 *
 * Revision 1.66  2000/10/18 02:04:02  weimer
 * playability changes ...
 *
 * Revision 1.65  2000/10/14 16:17:41  weimer
 * level adjustment changes, added some new AIs, etc.
 *
 * Revision 1.64  2000/10/14 02:52:44  weimer
 * fixed a memory corruption problem in display (a use after a free)
 *
 * Revision 1.63  2000/10/14 01:42:53  weimer
 * better scoring of thumbs-up, thumbs-down
 *
 * Revision 1.62  2000/10/14 01:24:34  weimer
 * fixed error with not advancing levels when fighting AI
 *
 * Revision 1.61  2000/10/13 22:34:26  weimer
 * minor wessy AI changes
 *
 * Revision 1.60  2000/10/13 18:23:28  weimer
 * fixed a race condition in tetris_event()
 *
 * Revision 1.59  2000/10/13 16:37:39  weimer
 * Changed the AI so that it now passes state around via void pointers, rather
 * than using global variables. This allows the same AI to play itself. Also
 * changed the "AI vs. AI" display so that you can keep track of total wins
 * and losses.
 *
 * Revision 1.58  2000/10/13 15:41:53  weimer
 * revamped AI support, now you can pick your AI and have AI duels (such fun!)
 * the mighty Aliz AI still crashes a bit, though ... :-)
 *
 * Revision 1.57  2000/10/13 02:26:54  weimer
 * rudimentary identity functions, including adding new players
 *
 * Revision 1.56  2000/10/12 22:21:25  weimer
 * display changes, more multi-local-play threading (e.g., myScore ->
 * Score[0]), that sort of thing ...
 *
 * Revision 1.55  2000/10/12 19:17:08  weimer
 * Further support for AI players and multiple game types.
 *
 * Revision 1.54  2000/10/12 01:38:07  weimer
 * added initial support for persistent player identities
 *
 * Revision 1.53  2000/10/12 00:49:08  weimer
 * added "AI" support and walking radio menus for initial game configuration
 *
 * Revision 1.52  2000/09/09 19:14:34  weimer
 * forgot to draw the background in MULTI games ..
 *
 * Revision 1.51  2000/09/09 17:05:35  wkiri
 * Hideous log changes (Wes: how dare you include a comment character!)
 *
 * Revision 1.50  2000/09/09 16:58:27  weimer
 * Sweeping Change of Ultimate Mastery. Main loop restructuring to clean up
 * main(), isolate the behavior of the three game types. Move graphic files
 * into graphics/-, style files into styles/-, remove some unused files,
 * add game flow support (breaks between games, remembering your level within
 * this game), multiplayer support for the event loop, some global variable
 * cleanup. All that and a bag of chips!
 *
 * Revision 1.49  2000/09/05 20:22:12  weimer
 * native video mode selection, timing investigation
 *
 * Revision 1.48  2000/09/04 19:48:02  weimer
 * interim menu for choosing among game styles, button changes (two states)
 *
 * Revision 1.47  2000/09/03 21:17:38  wkiri
 * non-MULTI player games don't need to check for passing random seed around.
 *
 * Revision 1.46  2000/09/03 21:06:31  wkiri
 * Now handles three different game types (and does different things).
 * Major changes in display.c to handle this.
 */
