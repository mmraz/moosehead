
/* $Id: gladiator.h,v 1.4 2000/07/24 15:50:09 mud Exp $"; */

/*
 * Structure types.
 */
typedef struct  gladiator_info_data     GLADIATOR_INFO_DATA;

struct  gladiator_info_data
{
    bool	started;
    int         time_left;
    int         min_level;
    int         max_level;
    int         type;
    int		playing;
    int		team_counter;
    int		gladiator_score;
    int		barbarian_score;
    int		bet_counter;
    int		bet_total;
    int  	total_levels;
    int 	total_wins;
    int 	total_plays;
    int		num_of_glads;
    bool	blind;
    bool        exper;
    bool        WNR;
};

/*
 * Global variables.
 */
extern          GLADIATOR_INFO_DATA     gladiator_info;


void set_glad_name args((CHAR_DATA *ch));
void gladiator_rename_all args((void));
void  gladiator_update args( ( void ) );
void  remove_gladiator args ((CHAR_DATA *ch));   
void  gladiator_left_arena args ((CHAR_DATA *ch, bool DidQuit));   
void  gladiator_talk	args( ( char *txt) );
void  gladiator_talk_ooc  args( ( char *txt) );
void  gladiator_winner  args( (CHAR_DATA *ch) );
void  gladiator_kill args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
