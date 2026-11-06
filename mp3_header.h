#ifndef MP3_HEADER_H
#define MP3_HEADER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define VIEW 1
#define EDIT 2
#define FAILURE 0

#define TAG_HEADER_SIZE 10
#define FRAME_HEADER_SIZE 10

struct MP3
{
    char *mp3_filename;

    FILE *org_mp3_fptr;
    FILE *dup_mp3_fptr;

    char *tag_option;
    char *new_tag;
};

/* functions.c */
int validate_cla(int argc, char *argv[], struct MP3 *mp3);

/* mp3_view.c */
void view(struct MP3 *mp3);

/* mp3_edit.c */
void edit(struct MP3 *mp3);

#endif