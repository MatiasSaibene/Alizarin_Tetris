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


/* INCLUDES! */
#include "config.h"
#include "atris.h"
#include "options.h"
#include <stdlib.h>
#include <stdio.h>

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

