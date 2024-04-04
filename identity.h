/*
 *                               Alizarin Tetris
 * The identity interface header file.
 *
 * Copyright 2000, Kiri Wagstaff & Westley Weimer
 */
#ifndef __IDENTITY_H
#define __IDENTITY_H

typedef struct person_struct {
    char *name;		/* who is it? */
    int level;		/* at what level? */
} person; 

typedef struct identity_struct {
    int n;
    person *p;
} identity;

#endif
