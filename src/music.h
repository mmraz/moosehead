/* $Id: music.h,v 1.4 1999/07/14 18:40:11 mud Exp $" */
#define MAX_SONGS	50
#define MAX_LINES	100 /* this boils down to about 1k per song */
#define MAX_GLOBAL	10 /* max songs the global jukebox can hold */

struct song_data
{
    char *group;
    char *name;
    char *lyrics[MAX_LINES];
    int lines;
};

extern struct song_data song_table[MAX_SONGS];

void song_update args( (void) );
void load_songs	args( (void) );
