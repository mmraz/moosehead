#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include "count.h"

FILE *input,*output;
int item_count[MAX_ITEMS];
struct timeval now_time;
time_t current_time;

void open_files(void)
{
    input = fopen(PLAYER_LIST,"r");
    output = fopen(OUTPUT_FILE,"w");

    if (input == NULL)
    {
	fprintf(stderr,"Error: could not open player file listing.\n\r");
	exit(1);
    }

    if (output == NULL)
    {
	fprintf(stderr,"Error: could not open output file.\n\r");
	exit(1);
    }
}

void close_files(void)
{
    if (fclose(input))
	fprintf(stderr,"Error: close failed on player file listing.\n\r");

    if (fclose(output))
	fprintf(stderr,"Error: close failed on output file.\n\r");
}

void dump_output(void)
{
    int i;

    for (i = 0; i < MAX_ITEMS; i++)
	if (item_count[i])
	    fprintf(output,"%d\t%d\n",i,item_count[i]);
}

void read_player(char *file)
{
    FILE *player;
    char buf[MAX_STRING_LENGTH];
    char *word;
    char letter;

    sprintf(buf,"../player/%s",file);

    player = fopen(buf,"r");
    if (player == NULL)
    {
	fprintf(stderr,"Error: could not open file %s.\n\r",buf);
	return;
    }

    for ( ; ;)
    {
    	letter = getc(player);
    	if (letter != '#' && letter != 'L' && letter != 'V')
	    fread_to_eol(player);
    	else
   	{
	    ungetc(letter,player);
	    word = fread_word(player);

	    if (!strcmp(word,"Vnum"))
		item_count[fread_number(player)]++;
	    else if (!strcmp(word,"#END")
	    ||  (!strcmp(word,"Levl") && fread_number(player) > 51))
		break;
	    else
		fread_to_eol(player);
	}
    }

    if (fclose(player))
	fprintf("Error: close failed on output dir.\n\r");
}
	
void read_input(void)
{
    char *file;

    while ((file = fread_word(input)) != NULL)
	read_player(file);
}

void main (void)
{
    gettimeofday(&now_time,NULL);
    current_time = (time_t) now_time.tv_sec;
    open_files();
    read_input();
    dump_output();
    close_files();
}
