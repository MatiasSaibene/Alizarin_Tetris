/*
 *                               Alizarin Tetris
 * The piece definition loading file.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"	/* go autoconf! */
#include <ctype.h>
#include <string.h>
#include <unistd.h>

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

#include "atris.h"
#include "display.h"
#include "piece.h"
#include "options.h"

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
