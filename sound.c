/*
 *                               Alizarin Tetris
 * The sound interface definition file.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */

#include "config.h"
#include <ctype.h>
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
#include "sound.h"

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
