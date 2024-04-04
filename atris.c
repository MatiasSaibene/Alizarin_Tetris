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

#include <SDL/SDL.h>
#include <SDL/SDL_main.h>
#include <SDL/SDL_ttf.h>

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
