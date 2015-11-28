/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku Mud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

static char rcsid[] = "$Id: special.c,v 1.125 2005/08/25 19:47:16 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "magic.h"
#include "lookup.h"

void	check_equip	args( (CHAR_DATA *ch) );
bool	check_wear	args( (CHAR_DATA *ch, OBJ_DATA *obj) );
void	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
int     count_fight_size args( (CHAR_DATA *ch) );

extern int social_count_targeted;
extern int social_count;

int bounty_vnum = -1;
int bounty_type = -1;
int bounty_desc = -1;
int bounty_available[MAX_BOUNTY_LEVEL];
int bounty_timer = 0;// Time since it was announced
int bounty_item = -1;
int bounty_room = -1;
int bounty_complete = 0;
bool bounty_downgrade = FALSE;
 
/* command procedures needed */
DECLARE_DO_FUN(do_order	);
DECLARE_DO_FUN( do_wear	);
DECLARE_DO_FUN(do_rescue);
DECLARE_DO_FUN(do_yell		);
DECLARE_DO_FUN(do_bash		);
DECLARE_DO_FUN(do_open		);
DECLARE_DO_FUN(do_close		);
DECLARE_DO_FUN(do_say	);
DECLARE_DO_FUN(do_backstab);
DECLARE_DO_FUN(do_flee);
DECLARE_DO_FUN(do_murder);
DECLARE_DO_FUN(do_wake);
DECLARE_DO_FUN(do_stand);
void say_spell( CHAR_DATA *ch, int sn );

/*
 * The following special functions are available for mobiles.
 */
DECLARE_SPEC_FUN(	spec_breath_any		);
DECLARE_SPEC_FUN(	spec_breath_acid	);
DECLARE_SPEC_FUN(	spec_breath_fire	);
DECLARE_SPEC_FUN(	spec_breath_frost	);
DECLARE_SPEC_FUN(	spec_breath_gas		);
DECLARE_SPEC_FUN(	spec_breath_lightning	);
DECLARE_SPEC_FUN(	spec_cast_adept		);
DECLARE_SPEC_FUN(	spec_cast_cleric	);
DECLARE_SPEC_FUN(	spec_cast_judge		);
DECLARE_SPEC_FUN(	spec_cast_mage		);
DECLARE_SPEC_FUN(	spec_cast_undead	);
DECLARE_SPEC_FUN(	spec_executioner	);
DECLARE_SPEC_FUN(	spec_fido		);
DECLARE_SPEC_FUN(	spec_guard_l		);
DECLARE_SPEC_FUN(	spec_guard_d		);
DECLARE_SPEC_FUN(	spec_janitor		);
DECLARE_SPEC_FUN(	spec_mayor		);
DECLARE_SPEC_FUN(	spec_poison		);
DECLARE_SPEC_FUN(	spec_thief		);
DECLARE_SPEC_FUN(	spec_puff		);
DECLARE_SPEC_FUN(	spec_phoenix		);
DECLARE_SPEC_FUN(	spec_nasty		);
DECLARE_SPEC_FUN(	spec_troll_member	);
DECLARE_SPEC_FUN(	spec_ogre_member	);
DECLARE_SPEC_FUN(	spec_patrolman		);
DECLARE_SPEC_FUN(	spec_rabbit		);
DECLARE_SPEC_FUN(	spec_cast_dispel	);
DECLARE_SPEC_FUN(	spec_monk		);
DECLARE_SPEC_FUN(	spec_altirin_undead	);
DECLARE_SPEC_FUN(	spec_vhan		);
DECLARE_SPEC_FUN(	spec_drachlan_melee	);
DECLARE_SPEC_FUN(	spec_drachlan_medic	);
DECLARE_SPEC_FUN(	spec_average		);
DECLARE_SPEC_FUN(	spec_salesman		);
DECLARE_SPEC_FUN(	spec_elemental_fire	);
DECLARE_SPEC_FUN(	spec_elemental_king	);
DECLARE_SPEC_FUN(	spec_elemental_water	);
DECLARE_SPEC_FUN(	spec_elemental_water_king	);
DECLARE_SPEC_FUN(       spec_honor_guard        );
DECLARE_SPEC_FUN(       spec_demise_guard       );
DECLARE_SPEC_FUN(       spec_posse_guard        );
DECLARE_SPEC_FUN(       spec_zealot_guard       );
DECLARE_SPEC_FUN(       spec_warlock_guard      );
DECLARE_SPEC_FUN(       spec_jabber             );
DECLARE_SPEC_FUN(       spec_rainbow       );
DECLARE_SPEC_FUN(       spec_poison_eater       );
DECLARE_SPEC_FUN(       spec_insane_mime        );
DECLARE_SPEC_FUN(       spec_clan_guardian        );
DECLARE_SPEC_FUN( spec_shade_bounty );

extern int rainbow;
extern time_t rainbow_found;
extern AREA_DATA *rainbow_area;
extern   sh_int  rev_dir[];
extern char *const dir_name[];

/* the function table */
const   struct  spec_type    spec_table[] =
{
    {	"spec_breath_any",		spec_breath_any, 2		},
    {	"spec_breath_acid",		spec_breath_acid, 2	},
    {	"spec_breath_fire",		spec_breath_fire, 2	},
    {	"spec_breath_frost",		spec_breath_frost, 2	},
    {	"spec_breath_gas",		spec_breath_gas, 2		},
    {	"spec_breath_lightning",	spec_breath_lightning, 2	},	
    {	"spec_cast_adept",		spec_cast_adept, 0		},
    {	"spec_cast_cleric",		spec_cast_cleric, 2	},
    {	"spec_cast_judge",		spec_cast_judge, 2		},
    {	"spec_cast_mage",		spec_cast_mage, 2		},
    {	"spec_cast_undead",		spec_cast_undead, 2	},
    {	"spec_executioner",		spec_executioner, 1	},
    {	"spec_fido",			spec_fido, 1		},
    {	"spec_guard_l",			spec_guard_l, 1		},
    {	"spec_guard_d",			spec_guard_d, 1		},
    {   "spec_honor_guard",             spec_honor_guard, 0        },
    {   "spec_demise_guard",            spec_demise_guard, 0       },
    {   "spec_posse_guard",             spec_posse_guard, 0        },
    {   "spec_zealot_guard",            spec_zealot_guard, 0       },
    {   "spec_warlock_guard",           spec_warlock_guard, 0      },
    {	"spec_janitor",			spec_janitor, 1		},
    {	"spec_mayor",			spec_mayor, 1		},
    {	"spec_poison",			spec_poison, 2		},
    {	"spec_thief",			spec_thief, 2		},
    {	"spec_puff",			spec_puff, 0		},
    {   "spec_phoenix",			spec_phoenix, 0		},
    {	"spec_nasty",			spec_nasty, 2		},
    {	"spec_troll_member",		spec_troll_member, 1	},
    {	"spec_ogre_member",		spec_ogre_member, 1	},
    {	"spec_patrolman",		spec_patrolman, 1		},
    {	"spec_rabbit",			spec_rabbit, 0		},
    {	"spec_cast_dispel",		spec_cast_dispel, 2	},
    {	"spec_monk",			spec_monk, 3		},
    {   "spec_altirin_undead",		spec_altirin_undead, 0	},
    {	"spec_vhan",			spec_vhan, 0		},
    {   "spec_drachlan_melee",		spec_drachlan_melee, 0	},
    {   "spec_drachlan_medic",		spec_drachlan_medic, 0	},
    {   "spec_average",			spec_average, 1		},
    {   "spec_salesman",		spec_salesman, 0		},
    {   "spec_elemental_king",		spec_elemental_king, 0	},
    {   "spec_elemental_fire",		spec_elemental_fire, 3	},
    {   "spec_elemental_water",		spec_elemental_water, 3	},
    {   "spec_elemental_water_king",		spec_elemental_water_king, 0	},
    {   "spec_jabber",                  spec_jabber, 1             },
    {	"spec_rainbow",			spec_rainbow, 0		},
    {		"spec_poison_eater", spec_poison_eater, 0 },
    {		"spec_insane_mime",		spec_insane_mime, 0 },
    {		"spec_clan_guardian",		spec_clan_guardian, 0 },
    {   "spec_shade_bounty", spec_shade_bounty, 0 },
    {	NULL,				NULL			}
};

/*
 * Given a name, return the appropriate spec fun.
 */
SPEC_FUN *spec_lookup( const char *name )
{
   int i;
 
   for ( i = 0; spec_table[i].name != NULL; i++)
   {
        if (LOWER(name[0]) == LOWER(spec_table[i].name[0])
        &&  !str_prefix( name,spec_table[i].name))
            return spec_table[i].function;
   }
 
    return 0;
}

char *spec_name( SPEC_FUN *function)
{
    int i;

    for (i = 0; spec_table[i].function != NULL; i++)
    {
	if (function == spec_table[i].function)
	    return spec_table[i].name;
    }

    return NULL;
}

/*  Guards for clans have their specs here. for easy removal if necessary */
bool spec_honor_guard( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *clanmate;
    DESCRIPTOR_DATA *d;

    if ( !IS_AWAKE(ch) )
	do_stand(ch,""); 

    if ( !IS_AWAKE(ch) || ch->fighting != NULL)
        return FALSE;

    for (victim = ch->in_room->people;  victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;
        if (ch == victim) continue;
        if (IS_NPC(victim)) continue;
        if ( !is_clan(victim) ) return FALSE;
/*  The next line is the meat and potatoes.  Change the clan as appropriate */
        if ( 
             ( IS_SET(victim->mhs, MHS_BANISH) 
             || victim->clan != nonclan_lookup("honor")
             )
              && !IS_IMMORTAL(victim) 
           )
            break;

    }

    if (ch->position < POS_STANDING)
        do_stand(ch,"");

    if (victim != NULL && is_clan(victim))
    {
        sprintf( buf, "What are you doing here, %s?!  Halt, knave!",
            victim->name );
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_yell(ch, buf);
        multi_hit( ch, victim, TYPE_UNDEFINED);

/*  Here's where the guard "clans" for help.  Wheee! */


        for (d = descriptor_list; d != NULL; d = d->next )
        {
            clanmate = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING) continue;

            if ( (d->connected == CON_PLAYING) && (!IS_SET(d->character->comm, COMM_NOCLAN)) &&
                (!IS_SET(d->character->comm, COMM_QUIET)) )
            {
                if ( IS_IMMORTAL(clanmate) && IS_SET(clanmate->mhs, MHS_LISTEN) )
                  send_to_char("[{CHonor{x] A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);

                if ( clanmate->clan == nonclan_lookup("honor") && !IS_IMMORTAL(clanmate) ) 
                  send_to_char("A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);   
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}


bool spec_demise_guard( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *clanmate;
    DESCRIPTOR_DATA *d;

    if ( !IS_AWAKE(ch) )
        do_stand(ch,"");

    if ( !IS_AWAKE(ch) || ch->fighting != NULL)
        return FALSE;

    for (victim = ch->in_room->people;  victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;
        if (ch == victim) continue;
        if (IS_NPC(victim)) continue;
        if ( !is_clan(victim) ) return FALSE;

/*  The next line is the meat and potatoes.  Change the clan as appropriate */
        //if ( victim->clan != clan_lookup("demise") && !IS_IMMORTAL(victim) )
        if (
             ( IS_SET(victim->mhs, MHS_BANISH)
             || victim->clan != nonclan_lookup("demise")
             )
              && !IS_IMMORTAL(victim)
           )
            break;
    }

    if (ch->position < POS_STANDING)
        do_stand(ch,"");

    if (victim != NULL  )
    {
        sprintf( buf, "What are you doing here, %s?!  Halt, knave!",
            victim->name );
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_yell(ch, buf);
        multi_hit( ch, victim, TYPE_UNDEFINED);

/*  Here's where the guard "clans" for help.  Wheee! */


        for (d = descriptor_list; d != NULL; d = d->next )
        {
            clanmate = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING) continue;

            if ( (d->connected == CON_PLAYING) && (!IS_SET(d->character->comm, COMM_NOCLAN)) &&
                (!IS_SET(d->character->comm, COMM_QUIET)) )
            {
                if ( IS_IMMORTAL(clanmate) && IS_SET(clanmate->mhs, MHS_LISTEN) )
                  send_to_char("[{RDemise{x] A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);

                if ( clanmate->clan == nonclan_lookup("demise") && !IS_IMMORTAL(clanmate) ) 
                  send_to_char("A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);   
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}


bool spec_posse_guard( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *clanmate;
    DESCRIPTOR_DATA *d;

    if ( !IS_AWAKE(ch) )
        do_stand(ch,"");

    if ( !IS_AWAKE(ch) || ch->fighting != NULL)
        return FALSE;

    for (victim = ch->in_room->people;  victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;
        if (ch == victim) continue;
        if (IS_NPC(victim)) continue;
        if ( !is_clan(victim) ) return FALSE;
/*  The next line is the meat and potatoes.  Change the clan as appropriate */
    //    if ( victim->clan != clan_lookup("posse") && !IS_IMMORTAL(victim) )
      //      break;
        if (
             ( IS_SET(victim->mhs, MHS_BANISH)
             || victim->clan != nonclan_lookup("posse")
             )
              && !IS_IMMORTAL(victim)
           )
            break;


    }

    if (ch->position < POS_STANDING)
        do_stand(ch,"");

    if (victim != NULL && is_clan(victim))
    {
        sprintf( buf, "What are you doing here, %s?!  Halt, knave!",
            victim->name );
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_yell(ch, buf);
        multi_hit( ch, victim, TYPE_UNDEFINED);

/*  Here's where the guard "clans" for help.  Wheee! */


        for (d = descriptor_list; d != NULL; d = d->next )
        {
            clanmate = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING) continue;

            if ( (d->connected == CON_PLAYING) && (!IS_SET(d->character->comm, COMM_NOCLAN)) &&
                (!IS_SET(d->character->comm, COMM_QUIET)) )
            {
                if ( IS_IMMORTAL(clanmate) && IS_SET(clanmate->mhs, MHS_LISTEN) )
                  send_to_char("[{MPosse{x] A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);

                if ( clanmate->clan == nonclan_lookup("posse") && !IS_IMMORTAL(clanmate) ) 
                  send_to_char("A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);   
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}


bool spec_zealot_guard( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *clanmate;
    DESCRIPTOR_DATA *d;

    if ( !IS_AWAKE(ch) )
        do_stand(ch,"");

    if ( !IS_AWAKE(ch) || ch->fighting != NULL)
        return FALSE;
    for (victim = ch->in_room->people;  victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;
        if (ch == victim) continue;
        if (IS_NPC(victim)) continue;
        if ( !is_clan(victim) ) return FALSE;
/*  The next line is the meat and potatoes.  Change the clan as appropriate */
        if (
             ( IS_SET(victim->mhs, MHS_BANISH)
             || victim->clan != nonclan_lookup("zealot")
             )
              && !IS_IMMORTAL(victim)
           )
            break;

    }



    if (ch->position < POS_STANDING)
        do_stand(ch,"");

    if (victim != NULL  && is_clan(victim))
    {
        sprintf( buf, "What are you doing here, %s?!  Halt, knave!",
            victim->name );
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_yell(ch, buf);
        multi_hit( ch, victim, TYPE_UNDEFINED);

/*  Here's where the guard "clans" for help.  Wheee! */


        for (d = descriptor_list; d != NULL; d = d->next )
        {
            clanmate = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING) continue;

            if ( (d->connected == CON_PLAYING) && (!IS_SET(d->character->comm, COMM_NOCLAN)) &&
                (!IS_SET(d->character->comm, COMM_QUIET)) )
            {
                if ( IS_IMMORTAL(clanmate) && IS_SET(clanmate->mhs, MHS_LISTEN) )
                  send_to_char("[{YZealot{x] A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);

                if ( clanmate->clan == nonclan_lookup("zealot") && !IS_IMMORTAL(clanmate) ) 
                  send_to_char("A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);   
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}


bool spec_warlock_guard( CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *clanmate;
    DESCRIPTOR_DATA *d;

    if ( !IS_AWAKE(ch) )
        do_stand(ch,"");

    if ( !IS_AWAKE(ch) || ch->fighting != NULL)
        return FALSE;

    for (victim = ch->in_room->people;  victim != NULL; victim = v_next)
    {
        v_next = victim->next_in_room;
        if (ch == victim) continue;
        if (IS_NPC(victim)) continue;
        if ( !is_clan(victim) ) return FALSE;
/*  The next line is the meat and potatoes.  Change the clan as appropriate */
        //if ( victim->clan != clan_lookup("warlock") && !IS_IMMORTAL(victim) )
        //    break;
           if (
             ( IS_SET(victim->mhs, MHS_BANISH)
             || victim->clan != nonclan_lookup("warlock")
             )
              && !IS_IMMORTAL(victim)
           )
            break;

    }

    if (ch->position < POS_STANDING)
        do_stand(ch,"");

    if (victim != NULL  && is_clan(victim))
    {
        sprintf( buf, "What are you doing here, %s?!  Halt, knave!",
            victim->name );
        REMOVE_BIT(ch->comm, COMM_NOSHOUT);
        do_yell(ch, buf);
        multi_hit( ch, victim, TYPE_UNDEFINED);

/*  Here's where the guard "clans" for help.  Wheee! */


        for (d = descriptor_list; d != NULL; d = d->next )
        {
            clanmate = d->original ? d->original : d->character;

            if (d->connected != CON_PLAYING) continue;

            if ( (d->connected == CON_PLAYING) && (!IS_SET(d->character->comm, COMM_NOCLAN)) &&
                (!IS_SET(d->character->comm, COMM_QUIET)) )
            {
                if ( IS_IMMORTAL(clanmate) && IS_SET(clanmate->mhs, MHS_LISTEN) )
                 send_to_char("[{gWarlock{x] A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);

                if ( clanmate->clan == nonclan_lookup("warlock") && !IS_IMMORTAL(clanmate) ) 
                  send_to_char("A guard {Gclans{x 'Help!  We are under attack!'\n\r"
                    ,clanmate);   
            }
        }
        return TRUE;
    }
    else
        return FALSE;
}




bool spec_rabbit( CHAR_DATA *ch)
{
	CHAR_DATA *vch;
	char buf[MAX_STRING_LENGTH];
	char social[MAX_STRING_LENGTH];
	int cmd;
    
    if (!IS_AWAKE(ch) || ch->in_room == NULL ||  IS_AFFECTED(ch,AFF_CHARM) 
        || ch->fighting != NULL)
        return FALSE;

    /* Find someone asleep to annoy */
    for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
    {
	if (ch == vch )
	continue;

	if ( vch != NULL && IS_AWAKE(vch) )
	continue;
    
	/* wake 'em, slap 'em and talk to 'em */

	do_wake( ch, vch->name );
	send_to_char( "\n\r", vch);

	for ( cmd = 0; social_table[cmd].name[0] != '\0'; cmd++ )
	{
	 strcpy(social,"slap");
	 if ( social[0]  == social_table[cmd].name[0]
	     && !str_prefix( social, social_table[cmd].name ) )
	 break;
	}

	act( social_table[cmd].others_found, ch, NULL, vch, TO_NOTVICT ,FALSE);
	act( social_table[cmd].char_found, ch, NULL, vch, TO_CHAR ,FALSE);
	act( social_table[cmd].vict_found, ch, NULL, vch, TO_VICT ,FALSE);

	send_to_char( "\n\r", vch);

	sprintf( buf, "Hey funny bunny!");
	do_say ( ch, buf);
    }
	return TRUE;
}

bool spec_troll_member( CHAR_DATA *ch)
{
    CHAR_DATA *vch, *victim = NULL;
    int count = 0;
    char *message;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL 
    ||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
	return FALSE;
 
    /* find an ogre to beat up */
    for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
    {
	if (!IS_NPC(vch) || ch == vch)
	    continue;

	if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
	    return FALSE;

	if (vch->pIndexData->group == GROUP_VNUM_OGRES
	&&  ch->level > vch->level - 2 && !is_safe(ch,vch))
	{
	    if (number_range(0,count) == 0)
		victim = vch;

	    count++;
	}
    }

    if (victim == NULL)
	return FALSE;

    /* say something, then raise hell */
    switch (number_range(0,6))
    {
	default:  message = NULL; 	break;
	case 0:	message = "$n yells 'I've been looking for you, punk!'";
		break;
	case 1: message = "With a scream of rage, $n attacks $N.";
		break;
	case 2: message = 
		"$n says 'What's slimy Ogre trash like you doing around here?'";
		break;
	case 3: message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
		break;
	case 4: message = "$n says 'There's no cops to save you this time!'";
		break;	
	case 5: message = "$n says 'Time to join your brother, spud.'";
		break;
	case 6: message = "$n says 'Let's rock.'";
		break;
    }

    if (message != NULL)
    	act(message,ch,NULL,victim,TO_ALL,FALSE);
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return TRUE;
}

bool spec_ogre_member( CHAR_DATA *ch)
{
    CHAR_DATA *vch, *victim = NULL;
    int count = 0;
    char *message;
 
    if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL
    ||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
        return FALSE;

    /* find an troll to beat up */
    for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
    {
        if (!IS_NPC(vch) || ch == vch)
            continue;
 
        if (vch->pIndexData->vnum == MOB_VNUM_PATROLMAN)
            return FALSE;
 
        if (vch->pIndexData->group == GROUP_VNUM_TROLLS
        &&  ch->level > vch->level - 2 && !is_safe(ch,vch))
        {
            if (number_range(0,count) == 0)
                victim = vch;
 
            count++;
        }
    }
 
    if (victim == NULL)
        return FALSE;
 
    /* say something, then raise hell */
    switch (number_range(0,6))
    {
	default: message = NULL;	break;
        case 0: message = "$n yells 'I've been looking for you, punk!'";
                break;
        case 1: message = "With a scream of rage, $n attacks $N.'";
                break;
        case 2: message =
                "$n says 'What's Troll filth like you doing around here?'";
                break;
        case 3: message = "$n cracks his knuckles and says 'Do ya feel lucky?'";
                break;
        case 4: message = "$n says 'There's no cops to save you this time!'";
                break;
        case 5: message = "$n says 'Time to join your brother, spud.'";
                break;
        case 6: message = "$n says 'Let's rock.'";
                break;
    }
 
    if (message != NULL)
    	act(message,ch,NULL,victim,TO_ALL,FALSE);
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return TRUE;
}

bool spec_patrolman(CHAR_DATA *ch)
{
    CHAR_DATA *vch,*victim = NULL;
    OBJ_DATA *obj;
    char *message;
    int count = 0;

    if (!IS_AWAKE(ch) || IS_AFFECTED(ch,AFF_CALM) || ch->in_room == NULL
    ||  IS_AFFECTED(ch,AFF_CHARM) || ch->fighting != NULL)
        return FALSE;

    /* look for a fight in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
	if (vch == ch)
	    continue;

	if (vch->fighting != NULL)  /* break it up! */
	{
	    if (number_range(0,count) == 0)
	        victim = (vch->level > vch->fighting->level) 
		    ? vch : vch->fighting;
	    count++;
	}
    }

    if (victim == NULL || (IS_NPC(victim) && victim->spec_fun == ch->spec_fun))
	return FALSE;

    if (((obj = get_eq_char(ch,WEAR_NECK_1)) != NULL 
    &&   obj->pIndexData->vnum == OBJ_VNUM_WHISTLE)
    ||  ((obj = get_eq_char(ch,WEAR_NECK_2)) != NULL
    &&   obj->pIndexData->vnum == OBJ_VNUM_WHISTLE))
    {
	act("You blow down hard on $p.",ch,obj,NULL,TO_CHAR,FALSE);
	act("$n blows on $p, ***WHEEEEEEEEEEEET***",ch,obj,NULL,TO_ROOM,FALSE);

    	for ( vch = char_list; vch != NULL; vch = vch->next )
    	{
            if ( vch->in_room == NULL )
            	continue;

            if (vch->in_room != ch->in_room 
	    &&  vch->in_room->area == ch->in_room->area)
            	send_to_char( "You hear a shrill whistling sound.\n\r", vch );
    	}
    }

    switch (number_range(0,6))
    {
	default:	message = NULL;		break;
	case 0:	message = "$n yells 'All roit! All roit! break it up!'";
		break;
	case 1: message = 
		"$n says 'Society's to blame, but what's a bloke to do?'";
		break;
	case 2: message = 
		"$n mumbles 'bloody kids will be the death of us all.'";
		break;
	case 3: message = "$n shouts 'Stop that! Stop that!' and attacks.";
		break;
	case 4: message = "$n pulls out his billy and goes to work.";
		break;
	case 5: message = 
		"$n sighs in resignation and proceeds to break up the fight.";
		break;
	case 6: message = "$n says 'Settle down, you hooligans!'";
		break;
    }

    if (message != NULL)
	act(message,ch,NULL,NULL,TO_ALL,FALSE);

    multi_hit(ch,victim,TYPE_UNDEFINED);

    return TRUE;
}
	

bool spec_nasty( CHAR_DATA *ch )
{
    CHAR_DATA *victim, *v_next;
    long gold;
 
    if (!IS_AWAKE(ch)) {
       return FALSE;
    }
 
    if (ch->position != POS_FIGHTING) {
       for ( victim = ch->in_room->people; victim != NULL; victim = v_next)
       {
          v_next = victim->next_in_room;
          if (!IS_NPC(victim)
             && (victim->level > ch->level)
             && (victim->level < ch->level + 10))
          {
	     do_backstab(ch,victim->name);
             if (ch->position != POS_FIGHTING)
                 do_murder(ch,victim->name);
             /* should steal some coins right away? :) */
             return TRUE;
          }
       }
       return FALSE;    /*  No one to attack */
    }
 
    /* okay, we must be fighting.... steal some coins and flee */
    if ( (victim = ch->fighting) == NULL)
        return FALSE;   /* let's be paranoid.... */
 
    switch ( number_bits(2) )
    {
        case 0:  act( "$n rips apart your coin purse, spilling your gold!",
                     ch, NULL, victim, TO_VICT,FALSE);
                 act( "You slash apart $N's coin purse and gather his gold.",
                     ch, NULL, victim, TO_CHAR,FALSE);
                 act( "$N's coin purse is ripped apart!",
                     ch, NULL, victim, TO_NOTVICT,FALSE);
                 gold = victim->gold / 10;  /* steal 10% of his gold */
                 victim->gold -= gold;
                 ch->gold     += gold;
                 return TRUE;
 
        case 1:  do_flee( ch, "");
                 return TRUE;
 
        default: return FALSE;
    }
}

/*
 * Core procedure for dragons.
 */
bool dragon( CHAR_DATA *ch, char *spell_name )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int sn;

    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim->fighting == ch && number_bits( 3 ) == 0 )
	    break;
    }

    if ( victim == NULL )
	return FALSE;

    if ( ( sn = skill_lookup( spell_name ) ) < 0 )
	return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}



/*
 * Special procedures for mobiles.
 */
bool spec_breath_any( CHAR_DATA *ch )
{
    if ( ch->position != POS_FIGHTING )
	return FALSE;

    switch ( number_bits( 3 ) )
    {
    case 0: return spec_breath_fire		( ch );
    case 1:
    case 2: return spec_breath_lightning	( ch );
    case 3: return spec_breath_gas		( ch );
    case 4: return spec_breath_acid		( ch );
    case 5:
    case 6:
    case 7: return spec_breath_frost		( ch );
    }

    return FALSE;
}



bool spec_breath_acid( CHAR_DATA *ch )
{
    return dragon( ch, "acid breath" );
}



bool spec_breath_fire( CHAR_DATA *ch )
{
    return dragon( ch, "fire breath" );
}



bool spec_breath_frost( CHAR_DATA *ch )
{
    return dragon( ch, "frost breath" );
}



bool spec_breath_gas( CHAR_DATA *ch )
{
    int sn;

    if ( ch->position != POS_FIGHTING )
	return FALSE;

    if ( ( sn = skill_lookup( "gas breath" ) ) < 0 )
	return FALSE;
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, NULL,TARGET_CHAR);
    return TRUE;
}



bool spec_breath_lightning( CHAR_DATA *ch )
{
    return dragon( ch, "lightning breath" );
}



bool spec_cast_adept( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim != ch && can_see( ch, victim, FALSE ) 
	     && number_bits( 1 ) == 0 
	     && !IS_NPC(victim) && victim->level < 11)
	    break;
    }

    if ( victim == NULL )
	return FALSE;

    switch ( number_bits( 4 ) )
    {
    case 0:
	act( "$n utters the word 'abrazak'.", ch, NULL, NULL, TO_ROOM ,FALSE);
	spell_armor( skill_lookup( "armor" ), ch->level,ch,victim,TARGET_CHAR);
	return TRUE;

    case 1:
	act( "$n utters the word 'fido'.", ch, NULL, NULL, TO_ROOM ,FALSE);
	spell_bless( skill_lookup( "bless" ), ch->level,ch,victim,TARGET_CHAR);
	return TRUE;

    case 2:
	act("$n utters the words 'judicandus noselacri'.",ch,NULL,NULL,TO_ROOM,FALSE);
	spell_cure_blindness( skill_lookup( "cure blindness" ),
	    ch->level, ch, victim,TARGET_CHAR);
	return TRUE;

    case 3:
	act("$n utters the words 'judicandus dies'.", ch,NULL, NULL, TO_ROOM ,FALSE);
	spell_cure_light( skill_lookup( "cure light" ),
	    ch->level, ch, victim,TARGET_CHAR);
	return TRUE;

    case 4:
	act( "$n utters the words 'judicandus sausabru'.",ch,NULL,NULL,TO_ROOM,FALSE);
	spell_cure_poison( skill_lookup( "cure poison" ),
	    ch->level, ch, victim,TARGET_CHAR);
	return TRUE;

    case 5:
	act("$n utters the word 'candusima'.", ch, NULL, NULL, TO_ROOM ,FALSE);
	spell_refresh( skill_lookup("refresh"),ch->level,ch,victim,TARGET_CHAR);
	return TRUE;

    case 6:
	act("$n utters the words 'judicandus eugzagz'.",ch,NULL,NULL,TO_ROOM,FALSE);
	spell_cure_disease(skill_lookup("cure disease"),
	    ch->level,ch,victim,TARGET_CHAR);
	return TRUE;

    case 7:
	act("$n utters the words 'yummy tummy'.",ch,NULL,NULL,TO_ROOM,FALSE);
	spell_feast(skill_lookup("feast"),
		ch->level,ch,victim,TARGET_CHAR);
	return TRUE;
    }

    return FALSE;
}



bool spec_cast_cleric( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim->fighting == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( victim == NULL )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "blindness";      break;
	case  1: min_level =  3; spell = "cause serious";  break;
	case  2: min_level =  7; spell = "earthquake";     break;
	case  3: min_level =  9; spell = "cause critical"; break;
	case  4: min_level = 10; spell = "dispel evil";    break;
	case  5: min_level = 12; spell = "curse";          break;
	case  6: min_level = 12; spell = "change sex";     break;
	case  7: min_level = 13; spell = "flamestrike";    break;
	case  8: min_level = 14; spell = "turn undead";    break;
	case  9:
	case 10: min_level = 15; spell = "harm";           break;
	case 11: min_level = 15; spell = "plague";	   break;
	default: min_level = 16; spell = "dispel magic";   break;
	}

	if ( ch->level >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    if (is_affected(victim, skill_lookup("orb of turning"))) 
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return FALSE;
    }
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim,TARGET_CHAR);

    return TRUE;
    
}

bool spec_cast_judge( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;
 
    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
        return FALSE;
 
    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }
 
    if ( victim == NULL )
        return FALSE;
 
    spell = "high explosive";
    if ( ( sn = skill_lookup( spell ) ) < 0 )
        return FALSE;
    if (is_affected(victim, skill_lookup("orb of turning"))) 
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return FALSE;
    }
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim,TARGET_CHAR);
    return TRUE;
}

bool spec_cast_dispel( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int sn;

    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
        return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
        v_next = victim->next_in_room;
        if ( victim->fighting == ch && number_bits( 2 ) == 0 )
            break;
    }

    if ( victim == NULL )
        return FALSE;

    if ( ( sn = skill_lookup( "dispel magic" ) ) < 0 )
        return FALSE;
    if (is_affected(victim, skill_lookup("orb of turning"))) 
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return FALSE;
    }
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim,TARGET_CHAR);
    return TRUE;
}

/* Special for night mobs in Altirin */
bool spec_altirin_undead( CHAR_DATA *ch )
{
    CHAR_DATA *rch;

    if ( ch->fighting != NULL )
    	return FALSE;

    
    /* always make sure they stay wizi unless somebody is in the room */

   /* They are only active at night, so only run this loop if its night time */
   if ( time_info.hour < 6 || time_info.hour > 19 )
    for ( rch = ch->in_room->people ; rch != NULL ; rch = rch->next_in_room )
    {
	if ( IS_NPC(rch) || rch == ch || IS_IMMORTAL(rch) )
		continue;

	if ( ch->invis_level )
	{
	   ch->invis_level = 0;
	   act("The air before you shimmers as $n fades into existance.",
		ch, NULL, NULL, TO_ROOM,FALSE);
	   return TRUE;
	}
	else
	{ /* undead already went vis, which means somebody is here attack! */
	    act("$n shifts and phases briefly.",ch,NULL,NULL,TO_ROOM,FALSE);
	    multi_hit( ch, rch, 0 );
	    return TRUE;
	}
    }

    /* Out here?  Means we didn't see anybody.  Go invis */
    if ( ch->invis_level )
	return FALSE;

 act("The air shimmers, and then $n fades away.",ch, NULL, NULL, TO_ROOM,FALSE);
    ch->invis_level = 53;
    return TRUE;
}

/* Follow a player around and try to sell them stuff */
bool spec_salesman( CHAR_DATA *ch )
{
  CHAR_DATA *vch;
 
  /* see if we're already following someone and they're in the room */
  if ( ch->master != NULL && (vch = get_char_room(ch,ch->master->name))!=NULL )
  {
    OBJ_DATA *obj,*sobj;

    /* anything to sell? */
    if( (sobj = ch->carrying) == NULL)
    {
      stop_follower(ch);
      return FALSE;
    }

    /* Can't have them sleeping through our sales pitch. */
    if(!IS_AWAKE(vch))
    {
      do_wake( ch, vch->name );
      do_say(ch,"Now's no time to sleep!");
      send_to_char( "\n\r", vch);
    }

    /* anything better to sell? */
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
      if( obj->cost > sobj->cost )
	sobj = obj;
    }

    switch( dice(1,30) )
    {
      case 3: act("$N inquires, 'Would you like to buy $p?'",vch,sobj,ch,
		TO_CHAR ,FALSE);
      act("$N inquires of $n, 'Would you like to buy $p?'",vch,sobj,ch,
	  TO_ROOM ,FALSE);
	      break;
      case 9: do_say(ch,ch->pIndexData->spec_words[1]); break;
      case 21: act("$n's face wells up with tears.\n\rShaking $s head from side to side and staring at $p in disbelief...",ch,sobj,NULL,TO_ROOM,FALSE);
      do_say(ch,"It's such a great deal, why won't they buy it?"); break;
      case 14: do_say(ch,ch->pIndexData->spec_words[2]); break;
      case 15: do_say(ch,"Come on, I got a wife and kids to feed."); break;
      case 26: act("$N points to $p.\n\r$N says, 'This is the finest $p in Boinga.  I'll give you a fabulous deal on it.'",vch,sobj,ch,TO_CHAR,FALSE);
      act("$n points to $p.",ch,sobj,vch,TO_NOTVICT,FALSE);
	      break;
      default: break;
    }
    
    return TRUE;
  }
    
  /* find a new victim */
  if( ch->carrying != NULL )
  {
    for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
    {
        if (!IS_NPC(vch))
        {
	  stop_follower(ch);
  	  add_follower(ch, vch);
	  do_say(ch,ch->pIndexData->spec_words[0]);
	  return TRUE;
        }
     } 
  }

  return FALSE;
}

bool spec_average( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if (!IS_AWAKE(ch))
	return FALSE;
    check_equip( ch );

    switch(dice(1,50))
    {
      case 2: do_say(ch,ch->pIndexData->spec_words[2]); return TRUE;
      case 14: do_say(ch,ch->pIndexData->spec_words[0]); return TRUE;
      case 36: do_say(ch,ch->pIndexData->spec_words[1]); return TRUE;
      default: break;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
      /* lets see if someone who just attacked us is in the room */
      if( ch->lAb != NULL  && ch->hit > ch->max_hit/4 )
      {
	if((victim=get_char_room(ch,ch->lAb->name)) != NULL )
	{
	  do_say(ch,"Hey!  I know you!");
	  one_hit(ch,victim,TYPE_UNDEFINED );
	  return TRUE;
	}
      }
    }
    else
    {
      if(!IS_NPC(victim))
      {
	ch->lAb = victim;
	return TRUE;
      }
      else
      {
	if(victim->master != NULL && !IS_NPC(victim->master))
	{
	  ch->lAb = victim->master;
	  return TRUE;
	}
      }
    }

    return FALSE;
}

bool spec_jabber( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if (!IS_AWAKE(ch))
        return FALSE;

    switch(dice(1,20))
    {
      case 2: if(ch->pIndexData->spec_words[2])
        do_say(ch,ch->pIndexData->spec_words[2]); return TRUE;
      case 10:  if(ch->pIndexData->spec_words[0])
        do_say(ch,ch->pIndexData->spec_words[0]); return TRUE;
      case 18:  if(ch->pIndexData->spec_words[1])
        do_say(ch,ch->pIndexData->spec_words[1]); return TRUE;
      default: break;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
      /* lets see if someone who just attacked us is in the room */
      if( ch->lAb != NULL  && ch->hit > ch->max_hit/4 )
      {
        if((victim=get_char_room(ch,ch->lAb->name)) != NULL )
        {
          do_say(ch,"Big meanie, go pick on someone your own size!");
          one_hit(ch,victim,TYPE_UNDEFINED );
          return TRUE;
        }
      }
    }
    else
    {
      if(!IS_NPC(victim))
      {
        ch->lAb = victim;
        return TRUE;
      }
      else
      {
        if(victim->master != NULL && !IS_NPC(victim->master))
        {
          ch->lAb = victim->master;
          return TRUE;
        }
      }
    }

    return FALSE;
}


bool spec_monk( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    char *spell="";
    int sn;
    int target = 0;
  
    check_equip( ch );
 
    /* It just got harder! Monk has supa regen */
    /* ch->hit = UMIN(ch->hit+200,ch->max_hit);
     * Lets try about 1/4 of that.  No one has been able to kill him
     * when he regens 800HP a round.
     */
    ch->hit = UMIN(ch->hit+50,ch->max_hit);

    if ( ( victim = ch->fighting ) == NULL )
	return FALSE;

    /* don't bother re-sanc'ing or re-haste'ing if our HP are low */
    if ( ch->hit < ( ch->max_hit / 4 ) )
    {
        spell = "heal";
        target = 1; /* True, mob is casting on itself */
    }
    else
    /* Always restore sanc first.  Ditto haste */
    if ( !IS_AFFECTED(ch,AFF_SANCTUARY) )
    {
	spell = "sanctuary";
	target = 1;
    }
    else
    if ( !IS_AFFECTED(ch,AFF_HASTE) )
    {
	spell = "haste";
	target = 1;
    }

    if ( target )
      {
        sn = skill_lookup(spell);
        (*skill_table[sn].spell_fun) ( sn, ch->level, ch, ch, TARGET_CHAR);
        return TRUE;
      }

    /* Ok we've taken care of ourselves, lets pound them */
    if ( is_affected(victim,gsn_sanctuary) )
        spell = "dispel magic";
    else
    if ( victim->hit > victim->max_hit / 2 )
    {
	do_backstab(ch,"");
	return TRUE;
    }
    else /* Just for our remorts */
    if ( IS_SET(victim->act,PLR_VAMP) && !IS_NPC(victim) &&
	 victim->pcdata->condition[COND_FULL] < 46 )
	spell = "feast";
    else
    if ( victim->mana > ( victim->max_mana / 8 ) &&
	!IS_NPC(victim) && class_table[victim->class].fMana )
	spell = "energy drain";
    else
    if ( IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_SKEL_WAR )
      {
	spell = "turn undead";
        do_say(ch,ch->pIndexData->spec_words[0]);
      }
    else
    {  /* Maladictions! */
	switch ( dice(2,6) )
	{
	case 2 :  spell = "turn undead";	break;
	case 3 :  spell = "slow";		break;
	case 4 :  spell = "poison";		break;
	case 5 :  spell = "blindness";		break;
	case 6 :  spell = "weaken";		break;
	case 7 :  spell = "curse";      	break;
	case 8 :  spell = "plague";     	break;
	case 9 :  spell = "irradiate";		break;
	case 10 :  spell = "chain lightning";	break;
	case 11:  spell = "enervation";		break;
	case 12:  spell = "irradiate";		break;
 	default :	return FALSE;
	};
    }

    if ( ( sn = skill_lookup(spell) ) < 0 )
	sn = skill_lookup("ice storm");

    if ( !target && is_affected(victim,sn) )
	sn = skill_lookup("acid blast");

    if ( sn < 0 ) /* last minute sanity check */
    {
	do_bash(ch,"");
	return TRUE;
    }

    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim, TARGET_CHAR);
    return TRUE;
}


bool spec_cast_mage( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim->fighting == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( victim == NULL )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "blindness";      break;
	case  1: min_level =  3; spell = "chill touch";    break;
	case  2: min_level =  7; spell = "weaken";         break;
	case  3: min_level =  8; spell = "teleport";       break;
	case  4: min_level = 11; spell = "colour spray";   break;
	case  5: min_level = 12; spell = "change sex";     break;
	case  6: min_level = 13; spell = "energy drain";   break;
	case  7:
	case  8:
	case  9: min_level = 15; spell = "fireball";       break;
	case 10: min_level = 20; spell = "plague";	   break;
	default: min_level = 20; spell = "acid blast";     break;
	}

	if ( ch->level >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    if (is_affected(victim, skill_lookup("orb of turning"))) 
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return FALSE;
    }
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim,TARGET_CHAR);
    return TRUE;
}



bool spec_cast_undead( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *spell;
    int sn;

    if ( ch->position != POS_FIGHTING 
	|| (ch->daze > 0 && number_percent() > (ch->level*3)/2))
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim->fighting == ch && number_bits( 2 ) == 0 )
	    break;
    }

    if ( victim == NULL )
	return FALSE;

    for ( ;; )
    {
	int min_level;

	switch ( number_bits( 4 ) )
	{
	case  0: min_level =  0; spell = "curse";          break;
	case  1: min_level =  3; spell = "weaken";         break;
	case  2: min_level =  6; spell = "chill touch";    break;
	case  3: min_level =  9; spell = "blindness";      break;
	case  4: min_level = 12; spell = "poison";         break;
	case  5: min_level = 15; spell = "energy drain";   break;
	case  6: min_level = 18; spell = "harm";           break;
	case  7: min_level = 21; spell = "teleport";       break;
	case  8: min_level = 20; spell = "plague";	   break;
	default: min_level = 18; spell = "harm";           break;
	}

	if ( ch->level >= min_level )
	    break;
    }

    if ( ( sn = skill_lookup( spell ) ) < 0 )
	return FALSE;
    if (is_affected(victim, skill_lookup("orb of turning"))) 
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return FALSE;
    }
    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, victim,TARGET_CHAR);
    return TRUE;
}

/* returns true if this person is part of the
clan which won the war    */
bool clanwar_winner( CHAR_DATA *ch)
{
    
    int clanwinner; /* clan war winner */

    /* every clan war set this name to the winning clan
       set it to an unused clan name if not running clan wars */

    clanwinner = nonclan_lookup("valor");

    if (ch->clan == clanwinner)
    {
      return TRUE;
    }

    return FALSE;

}


bool spec_executioner( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    char *crime;

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	return FALSE;

    crime = "";
    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) && !clanwar_winner(victim) )
	    { crime = "KILLER"; break; }

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) && !clanwar_winner(victim) )
	    { crime = "THIEF"; break; }
   
	if ( !IS_NPC(victim) && IS_SET(victim->wiznet, PLR_RUFFIAN) && number_percent() < 10 )
	    { do_say( ch, "SETTLE DOWN!" ); return TRUE; }
    }

    if ( victim == NULL )
	return FALSE;

    if (ch->position < POS_STANDING)
       do_stand(ch,"");

    sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!  MORE BLOOOOD!!!",
	victim->name, crime );
    REMOVE_BIT(ch->comm,COMM_NOSHOUT);
    do_yell( ch, buf );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return TRUE;
}

bool spec_phoenix( CHAR_DATA *ch )
{
    if ( ch->fighting != NULL ) 
	return TRUE;

    if ( number_percent() < 25 )
    {
	int sn;

	/* Teleport */
	if ( ( sn = skill_lookup( "teleport" ) ) < 0 )
                return FALSE;
    
        (*skill_table[sn].spell_fun) ( sn, 50, ch, ch, TARGET_CHAR );
    }
    else
    if ( number_percent() * number_percent() <= 5 )
    {
	/* Despawn */
	act("$n fades away to the outer planes.",ch,NULL,NULL,TO_ROOM,FALSE);
        extract_char(ch,TRUE);
    }

	return TRUE;
}

/* A procedure for Puff the Fractal Dragon--> it gives her an attitude.
	Note that though this procedure may make Puff look busy, she in
	fact does nothing quite more often than she did in Merc 1.0;
	due to null victim traps, my own do-nothing options, and various ways
	to return without doing much, Puff is... well, not as BAD of a gadfly
	as she may look, I assure you.  But I think she's fun this way ;)

	(btw--- should you ever want to test out your socials, just tweak
	the percentage table ('silliness') to make her do lots of socials,
	and then go to a quiet room and load up about thirty Puffs... ;) 
			
		written by Seth of Rivers of Mud         */

/* important constant :) */

#define MAXPUFF 15
bool spec_puff( CHAR_DATA *ch )
{
   char buf[MAX_STRING_LENGTH];
   int rnd_social, sn, silliness;
   int count = 0;
   CHAR_DATA *victim = NULL;
   CHAR_DATA *vch;
   CHAR_DATA *v_next;
   extern int social_count;
 
   if ( ch->position == POS_FIGHTING)
   {
   for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
      v_next = victim->next_in_room;
      if ( victim->fighting == ch && number_bits( 2 ) == 0 )
           break;
    }
 
    if ( victim == NULL )
                return FALSE;
 
    if ( ( sn = skill_lookup( "teleport" ) ) < 0 )
                return FALSE;
    (*skill_table[sn].spell_fun) ( sn, 50, ch, victim, TARGET_CHAR );
        return TRUE;
 
    }
 
   if (number_percent() <= 50)     /* Easy way to change Puff's hyperness. */
      return TRUE;
 
   rnd_social = (number_range (0, ( social_count - 1)) );
 
   /* Choose some manner of silliness to perpetrate.  */
 
   silliness = number_range(1, 100);
 
   if ( silliness <= 17)
      {
      act( social_table[rnd_social].others_auto, ch, NULL, ch, TO_ROOM ,FALSE);
      act( social_table[rnd_social].char_auto,   ch, NULL, ch, TO_CHAR ,FALSE);
      }
   else if ( silliness <= 23)
      {
      sprintf( buf, "Tongue-tied and twisted, just an earthbound misfit, ...");
      do_say ( ch, buf);
      }
      else if ( silliness <= 30)
      {
       sprintf( buf, "The colors, the colors!");
      do_say ( ch, buf);
      }
      else if ( silliness <= 40)
      {
      sprintf( buf, "Did you know that I'm written in C?");
      do_say ( ch, buf);
      }
      else if ( silliness <= 55)
      {
   act( social_table[rnd_social].others_no_arg, ch, NULL, NULL, TO_ROOM ,FALSE);
   act( social_table[rnd_social].char_no_arg, ch, NULL, NULL, TO_CHAR ,FALSE);
      }
      else if ( silliness <= 88)
      {
 
      /*
        *  Grab a random person in the room with Puff.
       */
 
      for ( vch = ch->in_room->people; vch != NULL; vch = v_next )
         {
         v_next = vch->next_in_room;
 
         if ( number_range( 0, count ) == 0 )
         victim = vch;
         count++;
         }
 
         if ( victim == NULL
          ||   victim == ch)
         return FALSE;
 
         act( social_table[rnd_social].others_found, ch,
                  NULL, victim, TO_NOTVICT ,FALSE);
         act( social_table[rnd_social].char_found, ch,
                  NULL, victim, TO_CHAR    ,FALSE);
         act( social_table[rnd_social].vict_found, ch,
                  NULL, victim, TO_VICT    ,FALSE);
         }
 
        else if ( silliness <= 93)
            {
            act( "For a moment, $n flickers and phases.", ch,
                        NULL, NULL, TO_ROOM ,FALSE);
            act( "For a moment, you flicker and phase.", ch,
                        NULL, NULL, TO_CHAR ,FALSE);
            }
        else if ( silliness <= 94)
            {
            if (ch->pIndexData->count < 2)
            return FALSE;
 
                act( "The Operator kills $n's job!", ch,
                NULL, NULL, TO_ROOM,FALSE);
                act( "  $n howls in anguish, and unravels!", ch,
                NULL, NULL, TO_ROOM,FALSE);
                act( "The Operator kills your job!  You unravel!", ch,
                NULL, NULL, TO_CHAR,FALSE);
                extract_char( ch, TRUE);
                }
        else if ( silliness <= 95)
        {
        if (ch->pIndexData->count >= MAXPUFF)
                return FALSE;
 
        act( "$n pushes $s stack frame, and clones $mself!", ch,
                NULL, NULL, TO_ROOM,FALSE);
        act( "You clone yourself!", ch, NULL, NULL, TO_CHAR,FALSE);
        char_to_room( create_mobile(ch->pIndexData), ch->in_room );
        }
 
/* The Fractal Dragon sometimes teleports herself around, to check out
 * new and stranger things. If you're playing Puff and
 * you want to talk with someone, just rest or sit!
 */
 
        else
              {
              if (ch->position < POS_STANDING)
              {
         act( "For a moment, $n seems lucid...", ch,
                NULL, NULL, TO_ROOM ,FALSE);
        act( "   ...but then $e returns to $s contemplations once again.", ch,
                NULL, NULL, TO_ROOM ,FALSE);
        act( "For a moment, the world's mathematical beauty is lost to you!",
                ch, NULL, NULL, TO_CHAR ,FALSE);
        act( "   ...but joy! yet another novel phenomenon seizes your attention.", ch, NULL, NULL, TO_CHAR,FALSE);
	return TRUE;
              }
 
              if ( ( sn = skill_lookup( "teleport" ) ) < 0 )
                return FALSE;
              (*skill_table[sn].spell_fun) (sn,ch->level,ch,ch,TARGET_CHAR);
 
        }
 
        return TRUE;
}
			

bool spec_fido( CHAR_DATA *ch )
{
    OBJ_DATA *corpse;
    OBJ_DATA *c_next;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( corpse = ch->in_room->contents; corpse != NULL; corpse = c_next )
    {
	c_next = corpse->next_content;
	if ( corpse->item_type != ITEM_CORPSE_NPC )
	    continue;

	act( "$n savagely devours a corpse.", ch, NULL, NULL, TO_ROOM ,FALSE);
	for ( obj = corpse->contains; obj; obj = obj_next )
	{
	    obj_next = obj->next_content;
	    obj_from_obj( obj );
	    obj_to_room( obj, ch->in_room );
	}
	extract_obj( corpse );
	return TRUE;
    }

    return FALSE;
}



bool spec_guard_d( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *ech;
    char *crime;
    int max_evil;
    int connive_check;
    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	return FALSE;

    max_evil = 300;
    ech      = NULL;
    crime    = "";

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;

        if(IS_IMMORTAL(victim))
          continue;

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) && !clanwar_winner(victim) && !IS_SET(victim->mhs,MHS_HIGHLANDER) )
	    { crime = "KILLER"; break; }

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) && !clanwar_winner(victim)  && !IS_SET(victim->mhs,MHS_HIGHLANDER))
	    { crime = "THIEF"; break; }

	if ( victim->fighting != NULL
	&&   victim->fighting != ch
	&&   victim->alignment < max_evil )
	{
	    max_evil = victim->alignment;
	    ech      = victim;
	}
    }


    if ( victim != NULL )
    {
    if (victim->kit== kit_lookup("fence"))
    {
        connive_check = number_percent();
        if(!IS_NPC(victim) && connive_check < get_curr_stat(victim,STAT_SOC)*2)
        {
                sprintf(buf, "%s you are tooo cool.",victim->name);
                do_say(ch,buf);
                return FALSE;
        }
    }

        if (ch->position < POS_STANDING)
           do_stand(ch,"");

	sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
	    victim->name, crime );
 	REMOVE_BIT(ch->comm,COMM_NOSHOUT);
	do_yell( ch, buf );
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
    }

    if ( ech != NULL )
    {
        if (ch->position < POS_STANDING)
           do_stand(ch,"");

	act( "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	multi_hit( ch, ech, TYPE_UNDEFINED );
	return TRUE;
    }

    return FALSE;
}


bool spec_guard_l( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    CHAR_DATA *ech;
    char *crime;
    int max_evil;

    if ( !IS_AWAKE(ch) || ch->fighting != NULL )
	return FALSE;

    max_evil = 300;
    ech      = NULL;
    crime    = "";

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;

        if(IS_IMMORTAL(victim))
          continue;

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_KILLER) && !clanwar_winner(victim) && !IS_SET(victim->mhs,MHS_HIGHLANDER))
	    { crime = "KILLER"; break; }

	if ( !IS_NPC(victim) && IS_SET(victim->act, PLR_THIEF) && !clanwar_winner(victim) && !IS_SET(victim->mhs,MHS_HIGHLANDER))
	    { crime = "THIEF"; break; }

	if ( victim->fighting != NULL
	&&   victim->fighting != ch
	&&   victim->alignment < max_evil )
	{
	    max_evil = victim->alignment;
	    ech      = victim;
	}
    }

    if ( victim != NULL )
    {
        if (ch->position < POS_STANDING)
           do_stand(ch,"");

	sprintf( buf, "%s is a %s!  PROTECT THE INNOCENT!!  BANZAI!!",
	    victim->name, crime );
 	REMOVE_BIT(ch->comm,COMM_NOSHOUT);
	do_yell( ch, buf );
	multi_hit( ch, victim, TYPE_UNDEFINED );
	return TRUE;
    }

    if ( ech != NULL )
    {
        if (ch->position < POS_STANDING)
           do_stand(ch,"");

	act( "$n screams 'PROTECT THE INNOCENT!!  BANZAI!!",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	multi_hit( ch, ech, TYPE_UNDEFINED );
	return TRUE;
    }

    return FALSE;
}

bool spec_janitor( CHAR_DATA *ch )
{
    OBJ_DATA *trash;
    OBJ_DATA *trash_next;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( trash = ch->in_room->contents; trash != NULL; trash = trash_next )
    {
	trash_next = trash->next_content;
	if ( !IS_SET(trash->wear_flags,ITEM_TAKE) || !can_loot(ch,trash,TRUE))
	    continue;
	if ( trash->item_type == ITEM_DRINK_CON
	||   trash->item_type == ITEM_TRASH
	||   trash->cost < 10 )
	{
	    act( "$n picks up some trash.", ch, NULL, NULL, TO_ROOM ,FALSE);
	    obj_from_room( trash );
	    obj_to_char( trash, ch );
	    return TRUE;
	}
    }

    return FALSE;
}



bool spec_mayor( CHAR_DATA *ch )
{
    static const char open_path[] =
	"W3a3003b33000c111d0d111Oe333333Oe22c222112212111a1S.";

    static const char close_path[] =
	"W3a3003b33000c111d0d111CE333333CE22c222112212111a1S.";

    static const char *path;
    static int pos;
    static bool move;

    if ( !move )
    {
	if ( time_info.hour ==  6 )
	{
	    path = open_path;
	    move = TRUE;
	    pos  = 0;
	}

	if ( time_info.hour == 20 )
	{
	    path = close_path;
	    move = TRUE;
	    pos  = 0;
	}
    }

    if ( ch->fighting != NULL )
	return spec_cast_mage( ch );
    if ( !move || ch->position < POS_SLEEPING )
	return FALSE;

    switch ( path[pos] )
    {
    case '0':
    case '1':
    case '2':
    case '3':
	move_char( ch, path[pos] - '0', FALSE );
	break;

    case 'W':
	ch->position = POS_STANDING;
	act( "$n awakens and groans loudly.", ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'S':
	ch->position = POS_SLEEPING;
	act( "$n lies down and falls asleep.", ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'a':
	act( "$n says 'Hello Honey!'", ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'b':
	act( "$n says 'What a view!  I must do something about that dump!'",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'c':
	act( "$n says 'Vandals!  Youngsters have no respect for anything!'",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'd':
	act( "$n says 'Good day, citizens! Re-Elect me for Mayor!'", ch, NULL, NULL, TO_ROOM ,FALSE);
	break;

    case 'e':
/*	act( "$n says 'I hereby declare the city of Midgaard open!'",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	    */
	act("$n says 'I'm glad I left the city gates open, one less thing to do!'",ch,NULL,NULL,TO_ROOM,FALSE);
	break;

    case 'E':
/*	act( "$n says 'I hereby declare the city of Midgaard closed!'",
	    ch, NULL, NULL, TO_ROOM ,FALSE);
	    */
	act("$n says 'I hereby declare the city ... bah I'm too tired to close the gates!'",ch,NULL,NULL,TO_ROOM,FALSE);
	break;

    case 'O':
/*	do_unlock( ch, "gate" ); */
/*	do_open( ch, "gate" ); */
	break;

    case 'C':
/*	do_close( ch, "gate" ); */
/*	do_lock( ch, "gate" ); */
	break;

    case '.' :
	move = FALSE;
	break;
    }

    pos++;
    return FALSE;
}



bool spec_poison( CHAR_DATA *ch )
{
    CHAR_DATA *victim;

    if ( ch->position != POS_FIGHTING
    || ( victim = ch->fighting ) == NULL
    ||   number_percent( ) > 2 * ch->level )
	return FALSE;

    act( "You bite $N!",  ch, NULL, victim, TO_CHAR    ,FALSE);
    act( "$n bites $N!",  ch, NULL, victim, TO_NOTVICT ,FALSE);
    act( "$n bites you!", ch, NULL, victim, TO_VICT    ,FALSE);
    spell_poison( gsn_poison, ch->level, ch, victim,TARGET_CHAR);
    return TRUE;
}



bool spec_thief( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    long gold,silver;

    if ( ch->position != POS_STANDING )
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;

	if ( IS_NPC(victim)
	||   victim->level >= LEVEL_IMMORTAL
	||   number_bits( 5 ) != 0 
	||   !can_see(ch,victim,FALSE))
	    continue;

	if ( IS_AWAKE(victim) && number_range( 0, ch->level ) == 0 )
	{
	    act( "You discover $n's hands in your wallet!",
		ch, NULL, victim, TO_VICT ,FALSE);
	    act( "$N discovers $n's hands in $S wallet!",
		ch, NULL, victim, TO_NOTVICT ,FALSE);
	    return TRUE;
	}
	else
	{
	    gold = victim->gold * UMIN(number_range(1,20),ch->level / 2) / 100;
	    gold = UMIN(gold, ch->level * ch->level * 10 );
	    ch->gold     += gold;
	    victim->gold -= gold;
	    silver = victim->silver * UMIN(number_range(1,20),ch->level/2)/100;
	    silver = UMIN(silver,ch->level*ch->level * 25);
	    ch->silver	+= silver;
	    victim->silver -= silver;
	    return TRUE;
	}
    }

    return FALSE;
}

/* Vhan is a bad-ass acid serpent in the Drachlan Canyon.
 * He guards some whack lewtz and is considered to be immortal.
 * He regens silly amounts of hit points, can kill with 1 hit
 * against NPCs, and being undead himself, can take control of
 * skeletal warriors.
 *
 *  Vhan has two special attacks.  One is a fatal coil attack,
 * and the other is his lethal venom.
 *
 * He doesn't dispel or re-sanc like the monk, but his massive regen
 * capacity makes him formidable anyway.
 */
bool spec_vhan( CHAR_DATA *vhan )
{
    CHAR_DATA *victim;

    /* First, add 250 hp */
    vhan->hit = UMIN(vhan->hit + 250, vhan->max_hit );

    victim = vhan->fighting;
    /* Next, see who we're fighting.  If it's an NPC, kill them */
    if ( victim != NULL && IS_NPC(victim) &&
	 victim->pIndexData->vnum != MOB_VNUM_SKEL_WAR )
    {
	act("$n wraps $s coils around $N and constricts!",vhan,NULL,victim,TO_NOTVICT,FALSE);
	act("$n wraps $s coils around you and constricts!",vhan,NULL,victim,TO_VICT,FALSE);
	act("You wrap your coils around $N and constrict!",vhan,NULL,victim,TO_CHAR,FALSE);
	victim->hit = UMIN(victim->hit,1);
	act("You begin to die.",vhan,NULL,victim,TO_VICT,FALSE);
	return TRUE;
    }
    else if ( victim != NULL )
    {
    /* Ok.  Now!  If we DIDN'T do that, inject them with venom */
	AFFECT_DATA af;
  	int resist_chance, sn;

 	af.where = DAMAGE_OVER_TIME;
	af.type  = sn = skill_lookup("venom of vhan");
	af.level = vhan->level;
 	af.location =	vhan->level * 2;
	af.modifier =	vhan->level * 5;
	af.duration = 18;
	af.bitvector = DAM_POISON;

	resist_chance = victim->level + (get_curr_stat(victim,STAT_CON)*2);
	if ( IS_SET(victim->res_flags,RES_POISON) )
	    resist_chance = 3 * resist_chance / 2;

	if ( IS_SET(victim->imm_flags,IMM_POISON) )
	    resist_chance *= 2;

 	if ( number_percent( ) * 2 > resist_chance && !is_affected(victim,sn) ) /* GOT EM! */
	{
	    affect_to_char(victim,&af);
		act("$n sinks $s teeth into you, injecting deadly venom.",vhan,NULL,victim,TO_VICT,FALSE);
		act("You sink your teeth into $N, injecting deadly venom.",vhan,NULL,victim,TO_CHAR,FALSE);
		act("$n sinks $s teeth into $N, injecting deadly venom.",vhan,NULL,victim,TO_NOTVICT,FALSE);
	    return TRUE;
 	}
    }

    /* Now check for skellies and steal them! */
    for ( victim = vhan->in_room->people ; victim != NULL ; victim = victim->next_in_room )
 	if ( IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_SKEL_WAR &&
		!is_same_group(victim,vhan) )
  	{
	    switch( number_range(1,10) )
	    {
		case 1:
	    do_say(vhan,"Kash'tak vhan.  Your restless soul belongs to me now."); break;
		case 2:
	    do_say(vhan,"The dead walk with Vhan."); break;
		case 3:
	    do_say(vhan,"Turn from the heathen and bow before the Lord of the Dead."); break;
		case 4:
	    do_say(vhan,"Turn your lifeless eyes upon the visage of the Master."); break;
		case 5:
	    do_say(vhan,"From listless sleep you rise and only to follow me."); break;
		case 6:
	    do_say(vhan,"Vhanta neis.  Arise and step into the shadow of Vhan."); break;
		case 7:
	    do_say(vhan,"Turn upon your captors, and fight for your true Master."); break;
		case 8:
	    do_say(vhan,"Slay now those you captured you."); break;
	    	case 9:
	    do_say(vhan,"Your will belongs to me."); break;
		case 10:
	    do_say(vhan,"Dae'tayana.  Vhas'ka braen kalidus."); break;
	    }

	   act("$n shudders and then bows before $N.",victim,NULL,vhan,TO_NOTVICT,FALSE);
	   act("You now follow $N.",victim,NULL,vhan,TO_CHAR,FALSE);
	   act("$n now follows you.",victim,NULL,vhan,TO_VICT,FALSE);

      	   stop_follower( victim );
    	   add_follower( victim, vhan );
    	   //victim->leader = vhan;
           add_to_group(victim, vhan);
	   SET_BIT(victim->affected_by,AFF_CHARM);
	
	   if( victim->fighting != NULL )
	       stop_fighting( victim, TRUE );

	   if ( vhan->fighting != NULL )
	       do_rescue(victim,"vhan");

	   return TRUE;
	}

    /* Out here?  Nothing to do at all? */
    if ( vhan->fighting != NULL )
    	do_order(vhan,"all rescue vhan");
    return FALSE;
}

bool spec_drachlan_melee( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    EXIT_DATA *pExit;
    int i;

    check_equip(ch );
    /* Drachlan regen */
    ch->hit = UMIN(ch->hit+100,ch->max_hit);

    victim = ch->fighting;

    /* First, check for an NPC and drop 'em */
    if ( victim != NULL && IS_NPC(victim) )
    {
	AFFECT_DATA af;

	af.where	= TO_AFFECTS;
	af.type		= skill_lookup("sleep");
   	af.modifier	= 0;
	af.location	= 0;
	af.level	= 0;
	af.duration	= -1;
	af.bitvector	= AFF_SLEEP;
	
	affect_to_char(victim,&af);
	stop_fighting(victim, TRUE);
	victim->position = POS_SLEEPING;
	do_say(ch,"Like your ancestors, you will have eternal sleep.");
	return TRUE;
    }
    else
    if ( victim != NULL ) /* PC */
    {
	int sn;
	int chance;
	AFFECT_DATA af;

	chance = (ch->level * 2) - (get_curr_stat(victim,STAT_DEX)*3);
	sn = skill_lookup("eye gouge");

	if ( !is_affected(victim,sn) && number_percent( ) < chance )
	{
	    af.where 	= TO_AFFECTS;
	    af.level	= ch->level;
	    af.type	= sn;
	    af.duration = -1;
	    af.modifier = -4;
	    af.location = APPLY_HITROLL;
	    af.bitvector = AFF_BLIND;

	    act("$n rakes $s claws across your face and gouges your eyes!",
		ch,NULL,victim,TO_VICT,FALSE);
	    act("You rake your claws across $N's face and gouge $S eyes!",
		ch,NULL,victim,TO_CHAR,FALSE);
	    act("$n takes $s claws across $N's face and gouges $S eyes!",
		ch,NULL,victim,TO_NOTVICT,FALSE);

	    affect_to_char(victim,&af);
	    return TRUE;
	}
	act("$n snarls and claws at your face!",ch,NULL,victim,TO_VICT,FALSE);
	act("You snarl and claw at $N's face!",ch,NULL,victim,TO_CHAR,FALSE);
	act("$n snarls and claws at $N's face!",ch,NULL,victim,TO_NOTVICT,FALSE);
	return FALSE;
    }

    /* Ok, we're not fighting anybody.  Check adjacent rooms for other drachlans
     * who are fighting, and move to help.
     */
    for ( i = 0 ; i < 6 ; i++ )
    {
	pExit = ch->in_room->exit[i];

	if ( pExit == NULL )
	    continue;
	else
	{
	    CHAR_DATA *vch;

	    for ( vch = pExit->u1.to_room->people ; vch != NULL ; vch = vch->next_in_room )
		if ( IS_NPC(vch) && vch->fighting != NULL &&
		     (vch->spec_fun == spec_drachlan_melee || 
		      vch->spec_fun == spec_drachlan_medic ) )
		{ /* If a drachlan NPC is fighting in this room, move in! */
		    move_char( ch, i, FALSE );
		    return TRUE;
		}
	}
    }

    /* check adjucent rooms for PC's to kill */
    for ( i = 0 ; i < 6 ; i++ )
    {
        pExit = ch->in_room->exit[i];

        if ( pExit == NULL )
            continue;
        else
        {
            CHAR_DATA *vch;

            for ( vch = pExit->u1.to_room->people ; vch != NULL ; vch = vch->next_in_room )
                if ( !IS_NPC(vch) && !is_affected(vch,skill_lookup("wraithform")) && can_see(ch,vch,FALSE) )
                { /* A player?  KILL! */
                    move_char( ch, i, FALSE );
                    return TRUE;
                }
        }
    }
    return FALSE ;
}

bool spec_drachlan_medic( CHAR_DATA *ch )
{
    CHAR_DATA *vch;
    int i;
    EXIT_DATA *pExit;


    check_equip(ch);
    /* Drachlan regen */
    ch->hit = UMIN(ch->hit+50,ch->max_hit);

    /* Medics will DOT their attackers and flee */
    if ( ch->fighting != NULL )
    {
	CHAR_DATA *victim = ch->fighting;
	int sn_cold;
	int sn_hot;
	int sn_elec;
 	int use_sn = -1;
	
	sn_cold = skill_lookup("frostbite");
	sn_hot = skill_lookup("blistering skin");
	sn_elec = skill_lookup("electrocution");

	if ( victim->race == race_lookup("dragon") && !is_affected(victim,sn_cold) )
		use_sn = sn_cold;
	else
	if ( victim->race == race_lookup("yinn") && !is_affected(victim,sn_hot) )
		use_sn = sn_hot;
	else
	if ( !is_affected(victim,sn_elec) )
	        use_sn = sn_elec;

	if ( use_sn < 0 ) /* Didn't meet any of these conditions.  Flee! */
 	{
	    do_flee(ch,"");
	    return TRUE;
	}

	if ( use_sn < 0 )
	    return FALSE;

	say_spell(ch,use_sn);
	(*skill_table[use_sn].spell_fun) ( use_sn, ch->level, ch, victim, TARGET_CHAR);
	return TRUE;
    }

    /* First, if any melee drachlans are here and fighting, heal them */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
	if ( IS_NPC(vch) && vch->fighting != NULL && vch->spec_fun == spec_drachlan_melee 
	      && vch->hit < (75*vch->max_hit/100) )
	{
	    int sn = skill_lookup("heal");

	    say_spell(ch,sn);
	    (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
	    return TRUE;
	}		
    }

    /* Nobody?  Check for the same thing, only not fighting, heal to 100% */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
	if ( IS_NPC(vch) && 
	(vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic) &&
	 vch->hit < vch->max_hit )
	{
	    act("$n's hands glow with a blue aura; $N appears to feel better.",
		ch,NULL,vch,TO_NOTVICT,FALSE);
	    act("Your hands glow with a blue aura; $N appears to feel better.",
		ch,NULL,vch,TO_CHAR,FALSE);
	    act("$n's hands glow with a blue aura; you feel better.",
		ch,NULL,vch,TO_VICT,FALSE);
	    
	    vch->hit = UMIN(vch->hit+100,vch->max_hit);
	    return TRUE;
	}		
    }

    /* Look in adjacent rooms, see if friends need help! */
    for ( i = 0 ; i < 6 ; i++ )
    {
        pExit = ch->in_room->exit[i];

        if ( pExit == NULL )
            continue;
        else
        {
            CHAR_DATA *vch;

            for ( vch = pExit->u1.to_room->people ; vch != NULL ; vch = vch->next_in_room )
                if ( IS_NPC(vch) && vch->fighting != NULL &&
                     (vch->spec_fun == spec_drachlan_melee ||
                      vch->spec_fun == spec_drachlan_medic ) )
                { /* If a drachlan NPC is fighting in this room, move in! */
                    move_char( ch, i, FALSE );
                    return TRUE;
                }
        }
    }

    /* Nobody?  Cure poison/disease/blind */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
        if ( IS_NPC(vch) &&
        (vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic)
         && is_affected(vch,gsn_blindness) )
        {
            int sn;

            sn = skill_lookup("cure blindness");

	    say_spell(ch,sn);
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
            return TRUE;
        }
    }

    /* Nobody?  Cure poison/disease/blind */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
	if ( IS_NPC(vch) && 
	(vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic) 
	 && is_affected(vch,gsn_poison) )
	{
	    int sn;

	    sn = skill_lookup("cure poison"); 
	   
	    say_spell(ch,sn); 
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);	    
	    return TRUE;
	}		
    }

    /* Nobody?  Cure poison/disease/blind */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
	if ( IS_NPC(vch) && 
	(vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic) 
	 && is_affected(vch,gsn_plague) )
	{
	    int sn;

	    sn = skill_lookup("cure disease"); 
	   
	    say_spell(ch,sn); 
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);	    
	    return TRUE;
	}		
    }

    /* Nobody? Start spelling up! */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
	if ( IS_NPC(vch) && 
	(vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic) 
	 && !IS_AFFECTED(vch,AFF_SANCTUARY) )
	{
	    say_spell(ch,gsn_sanctuary);
            (*skill_table[gsn_sanctuary].spell_fun) ( gsn_sanctuary, ch->level, ch, vch, TARGET_CHAR);	    
	    return TRUE;
	}		
    }


     for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
        if ( IS_NPC(vch) &&
        (vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic)
         && !IS_AFFECTED(vch,AFF_HASTE) && !IS_SET(vch->off_flags,OFF_FAST) )
        {
	    int sn = skill_lookup("haste");

            say_spell(ch,sn);
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
            return TRUE;
        }
    }

  for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
        if ( IS_NPC(vch) &&
        (vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic)
         && !is_affected(vch,skill_lookup("armor")) )
        {
	    int sn = skill_lookup("armor");

            say_spell(ch,sn);
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
            return TRUE;
        }
    }

     for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
        if ( IS_NPC(vch) &&
        (vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic)
         && !is_affected(vch,skill_lookup("shield")) )
        {
	     int sn = skill_lookup("shield");

            say_spell(ch,sn);
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
            return TRUE;
        }
    }

    /* VISION! */
     for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next )
    {
        if ( IS_NPC(vch) &&
        (vch->spec_fun == spec_drachlan_melee || vch->spec_fun == spec_drachlan_medic)
         && !is_affected(vch,skill_lookup("vision")) )
        {
             int sn = skill_lookup("vision");

            say_spell(ch,sn);
            (*skill_table[sn].spell_fun) ( sn, ch->level, ch, vch, TARGET_CHAR);
            return TRUE;

	}
    }

    /* STILL nothing?  Relax */
    return FALSE;
}

void check_equip( CHAR_DATA *ch )
{
    OBJ_DATA *obj;
    CHAR_DATA *vch;
    bool foundPC = FALSE;

    /* don't bother swapping EQ with no PCs in the room */
    for (vch = ch->in_room->people;  vch != NULL;  vch = vch->next_in_room)
    {   
        if (!IS_NPC(vch))
	   {
	    foundPC = TRUE;
            break;
	   }
    }

    /* Alright, a player is here.  Let's bother checking if we should swap. */
    if (foundPC)
    {
      for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
	if ( obj->wear_loc == WEAR_NONE &&  check_wear( ch, obj ) )
	    do_wear(ch,obj->name);
    }
}

bool check_wear( CHAR_DATA *ch, OBJ_DATA *obj1 )
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj2;
    int pafC1=0,pafC2=0,sum1=0,sum2=0;

   for (obj2 = ch->carrying; obj2 != NULL; obj2 = obj2->next_content)
   {
       if (obj2->wear_loc != WEAR_NONE
       &&  obj1->item_type == obj2->item_type
       &&  (obj1->wear_flags & obj2->wear_flags & ~ITEM_TAKE) != 0 )
     break;
   }

    /* not wearing anything in that slot and most anything
	is better than nothing at all.
     */
   if (obj2 == NULL)
   {
    return TRUE;
   }
   else
   {
	/* sum up the values that matter for comparison */
       switch ( obj1->item_type )
     {
     default:
       break;

     case ITEM_ARMOR:
	/* give the item points for AC values, half for magic */
       sum1 = obj1->value[0] +obj1->value[1] +obj1->value[2] +obj1->value[3]/2;
       sum2 = obj2->value[0] +obj2->value[1] +obj2->value[2] +obj1->value[3]/2;

	/* give some value to enchantments */
        for ( paf = obj1->affected; paf != NULL; paf = paf->next)
  	{
	  if ( paf->location == APPLY_AC)
	  sum1 -= paf->modifier/2;
	}

        for ( paf = obj2->affected; paf != NULL; paf = paf->next)
  	{
	  if ( paf->location == APPLY_AC)
	  sum2 -= paf->modifier/2;
	}
       break;

     case ITEM_WEAPON:
         if (obj1->pIndexData->new_format)
       sum1 = (1 + obj1->value[2]) * obj1->value[1];
         else
       sum1 = obj1->value[1] + obj1->value[2];

         if (obj2->pIndexData->new_format)
       sum2 = (1 + obj2->value[2]) * obj2->value[1];
         else
       sum2 = obj2->value[1] + obj2->value[2];

	/* increment the PAF counter if it has a weapon flag */
	 if(IS_WEAPON_STAT(obj1,WEAPON_FLAMING)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_FROST)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_VAMPIRIC)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_SHARP)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_VORPAL)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_TWO_HANDS)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_SHOCKING)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_POISON)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_STUN)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_HOLY)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_FAVORED)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_NETHER)) pafC1++;
	 if(IS_WEAPON_STAT(obj1,WEAPON_SCION)) pafC1++;
	 if(obj1->enchanted) pafC1++;

	 if(IS_WEAPON_STAT(obj2,WEAPON_FLAMING)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_FROST)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_VAMPIRIC)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_SHARP)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_VORPAL)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_TWO_HANDS)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_SHOCKING)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_POISON)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_STUN)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_HOLY)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_FAVORED)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_NETHER)) pafC2++;
	 if(IS_WEAPON_STAT(obj2,WEAPON_SCION)) pafC2++;
	 if(obj2->enchanted) pafC2++;

         break;
     }

	/* increment the PAF counter if it has a worth PAF */
     for ( paf = obj1->pIndexData->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_NONE && paf->modifier >= 0 )
            pafC1++;

     for ( paf = obj2->pIndexData->affected; paf != NULL; paf = paf->next )
        if ( paf->location == APPLY_NONE && paf->modifier >= 0 )
            pafC2++;

	/* totally equal objects, why bother switching? */
     if ( sum1 == sum2 && pafC1 == pafC2 && obj1->level == obj2->level )
	return FALSE;

	/* switch if attribs are equal but one has more PAF's
	   or if attribs are better and we're not losing PAF's
	   or if attribs are worse but we gain PAF's and don't lose levels
	 */
     if ( (sum1 == sum2 && pafC1 > pafC2)
	||(sum1 > sum2 && pafC1 >= pafC2)
	|| (sum1 <= sum2 && pafC1 > pafC2 && obj1->level > obj2->level) )
	return TRUE;
    }
  return FALSE;
}

bool spec_elemental_king (CHAR_DATA *king)
{
	CHAR_DATA *victim,*summ;
        int summon, cost = 1000;
	int use_sn = -1, sn1, sn2, sn3, sn4, sn5;
        int percent;
        OBJ_DATA *segment;
	
	if ( (victim = king->fighting) == NULL )
		return FALSE;

	switch (king->pIndexData->vnum)
	{
		case MOB_VNUM_KING_FIRE:
			summon = MOB_VNUM_FIRE_1;
			sn1    = skill_lookup("flameseek");
			sn2    = skill_lookup("blistering skin");
			sn3    = skill_lookup("incinerate");
			sn4    = skill_lookup("fire breath");
			sn5    = skill_lookup("wall of fire");
             		break;
		default:
		  return FALSE;
        }	

	if ( king->hit <= king->max_hit * 3/4 )
	{
        
        percent = number_percent();

        if (percent <= 40 )
	{
	   if (king->mana > cost  )
	   {
		king->mana -= cost;
		summ = create_mobile(get_mob_index(summon));
		char_to_room(summ, king->in_room);
		send_to_room("A shining globe appears and help arrives!\n\r",king->in_room);
		do_say(king,"Show the mortals the true power of the elements.");
		do_rescue(summ, "king");
                multi_hit( king, victim, TYPE_UNDEFINED );
		return TRUE;
 	   }	
        }
	else if ( percent <= 50 && !is_affected(victim, sn1))
		use_sn = sn1;
 	else if ( percent <= 60 && !is_affected(victim, sn2) )
		use_sn = sn2;
	else if ( percent <= 70 && !is_room_affected(king->in_room, sn5) )
		use_sn = sn5;
	else if ( percent <= 80 ) 
		use_sn = sn4;
	else if ( percent <= 90)
		use_sn = sn3;
        else
		return FALSE;
	
	}

	if ( use_sn > 0 )
	{
          say_spell(king,use_sn);
          (*skill_table[use_sn].spell_fun) ( use_sn, king->level, king, victim, TARGET_CHAR);
 	  return TRUE;
	}
        
        if ( king->hit < king->max_hit /4 )
        {
          do_say(king, "Follow me mortal, to the realm of the Spirit King.  If you dare.");
	  segment = create_object(get_obj_index(VNUM_FIRE_SEGMENT), 0, FALSE);
	  send_to_room("A small piece of gold falls from the King's grasp as he suddenly disappears!\n\r", king->in_room);
	  obj_to_room(segment, king->in_room);
          stop_fighting(king, FALSE);
          char_from_room(king);
          char_to_room(king, get_room_index(22500));
          return TRUE;
        }
	return FALSE;

}

bool spec_elemental_fire(CHAR_DATA *ch)
{
	int summon;
	int cost = 1000;
        CHAR_DATA *vch, *summ, *victim;

	/* first, if we're not fighting anyone, check to see if there's a king here
	   If there is, attach whoever he's fighting, if anyone. */
	if ( (victim = ch->fighting) == NULL )
	{
		for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next_in_room)
			if ( IS_NPC(vch) && (vch->pIndexData->vnum == MOB_VNUM_KING_FIRE ||
                                             vch->spec_fun == spec_elemental_fire ||
					     vch->spec_fun == spec_breath_fire ) )
				if ( (victim = vch->fighting) != NULL && (
					victim->spec_fun != spec_elemental_fire &&
					victim->spec_fun != spec_breath_fire ))
					{
					do_say(ch, ch->pIndexData->spec_words[2]);
					multi_hit(ch, victim, TYPE_UNDEFINED);
					return TRUE;
					}
				else
					return FALSE;
	}
	else
	{

	/* If we made it here, we're either fighting someone ourselves, or fighting whoever
	   the king is fighting, so we're safe to proceed */
	switch(ch->pIndexData->vnum)
	{
	case MOB_VNUM_FIRE_1:
		summon = MOB_VNUM_FIRE_2;
		break;
	case MOB_VNUM_FIRE_2:
		summon = MOB_VNUM_FIRE_3;
		break;
	case MOB_VNUM_FIRE_3:
		summon = MOB_VNUM_FIRE_SALAMANDER;
		break;
	default:
		summon = MOB_VNUM_MAGMAN;
		break;
	}

	
	if ( (ch->hit <= ch->max_hit * 3/4) && number_percent() < 30 )
	{
		if (ch->mana > cost )
		{
			ch->mana -= cost;
			summ = create_mobile(get_mob_index(summon));
			char_to_room(summ, ch->in_room);
			send_to_room("A shining globe appears and help arrives!\n\r",ch->in_room);
			do_say(ch,ch->pIndexData->spec_words[0]);
			/* Always rescue the king to prevent animated corpses
			   from getting rescued and having summoned attack king
			 */
			//do_rescue(summ, ch->pIndexData->player_name);
			do_rescue(summ, "king");
                	multi_hit( ch, victim, TYPE_UNDEFINED );
			return TRUE;
		}

	}
	else if ( number_percent() < 5 )
	{
	do_say(ch, ch->pIndexData->spec_words[1]);
	return FALSE;
	}
	}
	return FALSE;

}

bool spec_elemental_water(CHAR_DATA *ch)
{

   int summon, use_sn = -1;
   CHAR_DATA *summ, *victim, *vch;
   int cost = 1000;

   
	if ((victim = ch->fighting) == NULL)
		return FALSE;

        if (count_fight_size(ch) < 1)
		return FALSE;
	else if ( count_fight_size(ch) > 1)
		use_sn = skill_lookup("tsunami");
	else
	   {
	   /* if we're here, we should be fighting just one person or mob */
           switch(ch->pIndexData->vnum)
	   {
		case MOB_VNUM_WATER_1:
		  summon = MOB_VNUM_WATER_2;
  		  break;
		case MOB_VNUM_WATER_2:
		  summon = MOB_VNUM_WATER_3;
		  break;
                case MOB_VNUM_WATER_3:
		  summon = MOB_VNUM_WATER_WIERD;
		  break;
		case MOB_VNUM_WATER_WIERD:
		  summon = MOB_VNUM_ICE_DRAKE;
		  break;
	   }
	    if ( (ch->hit <= ch->max_hit * 3/4) && number_percent() < 30 )
            {
                if (ch->mana > cost )
                {
                        ch->mana -= cost;
                        summ = create_mobile(get_mob_index(summon));
                        char_to_room(summ, ch->in_room);
                        send_to_room("A shining globe appears and help arrives!\n\r",ch->in_room);
                        do_say(ch,ch->pIndexData->spec_words[0]);
			/* Always rescue the king to prevent animated corpses
			   from getting rescued and having summoned attack king
			 */
                        //do_rescue(summ, ch->pIndexData->player_name);
                        do_rescue(summ, "king");
                        multi_hit( ch, victim, TYPE_UNDEFINED );
                        return TRUE;
                }

            }
	    else if ( number_percent() < 5 )
        	{
        	do_say(ch, ch->pIndexData->spec_words[1]);
	        return FALSE;
	        }

 
           }
		
	if ( use_sn > 0 )
	{
          say_spell(ch,use_sn);
          (*skill_table[use_sn].spell_fun) ( use_sn, ch->level, ch, victim, TARGET_CHAR);
 	  return TRUE;
	}
	   return FALSE;
}

bool spec_elemental_water_king(CHAR_DATA *king)
{
	
   CHAR_DATA *victim,*summ;
   int summon, cost = 1000;
   int sn1, sn2, sn3, sn4, sn5,sn6;
   int percent,count;
   OBJ_DATA *segment;

   
        if ( king->hit < king->max_hit /4 )
        {
          do_say(king, "Follow me mortal, to the realm of the Spirit King.  If you dare.");
          segment = create_object(get_obj_index(VNUM_WATER_SEGMENT), 0, FALSE);
          send_to_room("A small piece of gold falls from the King's grasp as he suddenly disappears!\n\r", king->in_room);
          obj_to_room(segment, king->in_room);
          char_from_room(king);
          char_to_room(king, get_room_index(22500));
          return TRUE;
        }

   count = count_fight_size(king);

   if(count < 1 || (victim = king->fighting) == NULL)
	return FALSE;
   else if (count > 1)
       {
	sn6 = skill_lookup("tsunami");
        say_spell(king, sn6);
        (*skill_table[sn6].spell_fun) ( sn6, king->level, king, victim, TARGET_CHAR);
        return TRUE;
       }
   else
      {

      summon = MOB_VNUM_WATER_1;
      sn1    = skill_lookup("hydrophilia");
      sn2    = skill_lookup("frostbite");
      sn3    = skill_lookup("ice storm");
      sn4    = skill_lookup("frost breath");
      sn5    = skill_lookup("wall of ice");

      if (!is_room_affected(king->in_room, gsn_wall_ice))
      {
        say_spell(king, sn5);
        (*skill_table[sn5].spell_fun) ( sn5, king->level, king, victim, TARGET_CHAR);
        return TRUE;
      }
      
      if(!is_affected(victim, gsn_hydrophilia))
      {
        say_spell(king, sn1);
        (*skill_table[sn1].spell_fun) ( sn1, king->level, king, victim, TARGET_CHAR);
        return TRUE;
      }

      if ( king->hit <= king->max_hit * 3/4 && king->hit > king->max_hit /4)
      {

      percent = number_percent();

      if (percent <= 40 )
      {
           if (king->mana > cost  )
           {
                king->mana -= cost;
                summ = create_mobile(get_mob_index(summon));
                char_to_room(summ, king->in_room);
                send_to_room("A shining globe appears and help arrives!\n\r",king->in_room);
                do_say(king,"Show the mortals the true power of the elements.");
                do_rescue(summ, "king");
                multi_hit( king, victim, TYPE_UNDEFINED );
                return TRUE;
           }
        }
        else if ( percent <= 60 && !is_affected(victim, sn2) )
         {
          say_spell(king, sn4);
          (*skill_table[sn4].spell_fun) ( sn4, king->level, king, victim, TARGET_CHAR);
          return TRUE;
         }
        else if ( percent <= 80 )
         {
          say_spell(king, sn3);
          (*skill_table[sn3].spell_fun) ( sn3, king->level, king, victim, TARGET_CHAR);
          return TRUE;
         }
        else if ( percent <= 90)
         {
          say_spell(king, sn2);
          (*skill_table[sn2].spell_fun) ( sn2, king->level, king, victim, TARGET_CHAR);
          return TRUE;
         }
        else
                return FALSE;
        }

        return FALSE;
    }
}

void log_quest_detail(char *buf, int quest)
{
  char *strtime;
  char wizbuf[256];
  FILE *fp = fopen("quest_log.txt", "a");
  if(quest == QUEST_UNKNOWN)
    sprintf(wizbuf, "QUEST UNKNOWN: %s", buf);
  else
    sprintf(wizbuf, "Quest %d: %s", quest, buf);
  if(quest == QUEST_BOUNTY_KILL || quest == QUEST_BOUNTY_CLAIM)
    wiznet(wizbuf, NULL, NULL, WIZ_SHADEBOUNTY, 0, 0);
  else
    wiznet(wizbuf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
  if(!fp)// Opened earlier
    return;
  strtime = ctime( &current_time );
  strtime[strlen(strtime)-1] = '\0';
  fprintf(fp, "%s :: %s\n", strtime, wizbuf);
  fclose(fp);
}

bool poison_eater_handler(CHAR_DATA *creature, CHAR_DATA *ch, OBJ_DATA *obj) 
{
	bool edible = FALSE;
	bool preserved = FALSE;
	bool food = TRUE;
	OBJ_DATA *sliver = NULL;

	if ( strlen(obj->description) < 1 && !IS_SET(obj->wear_flags,ITEM_TAKE) ) 
		food = FALSE;

	if(!can_see_obj( creature, obj ) || obj->item_type != ITEM_FOOD)
		food = FALSE;

	if(food && obj->pIndexData->vnum >= OBJ_VNUM_TORN_HEART &&
      obj->pIndexData->vnum <= OBJ_VNUM_BRAINS)
	{
		if(obj->timer == 0)
		{// Not edible
			preserved = TRUE;
		}
		else if(obj->value[3] != 0)
		{// Poisoned
			edible = TRUE;
		}
	}

	if(ch == NULL)
	{// Just being used for an edibility check
		return edible;
	}

	if(!edible)
	{// Give it back
		if(!food)
			act("A look of distaste crosses $n's face. That is not food!", creature, NULL, NULL, TO_ROOM, FALSE);
		else if(preserved)
			act("A look of distaste crosses $n's face. The food is not fresh!", creature, NULL, NULL, TO_ROOM, FALSE);
		else// Not poisoned is the last edible failure
			act("A look of distaste crosses $n's face. The food is too healthy!", creature, NULL, NULL, TO_ROOM, FALSE);

    obj_from_char( obj );
    obj_to_char( obj, ch );
    act( "$n gives $p to $N.", creature, obj, ch, TO_NOTVICT ,FALSE);
    act( "$n gives you $p.",   creature, obj, ch, TO_VICT    ,FALSE);
    act( "You give $p to $N.", creature, obj, ch, TO_CHAR    ,FALSE);

		return edible;
	}
	
	act("$n messily consumes $p and smiles happily at you.", creature, obj, ch, TO_VICT, FALSE);
	act("$n messily consumes $p and smiles happily at $N.", creature, obj, ch, TO_NOTVICT, FALSE);
	act("You messily consume $p and smile happily at $N.", creature, obj, ch, TO_CHAR, FALSE);
	sliver = create_object(get_obj_index(OBJ_VNUM_TINYFAVOR),0,FALSE);
        sliver->timer = 110;// Slightly longer than two last
	obj_to_char(sliver,ch);
	act("$n spits something from $s cheek into $s hand.", creature, NULL, NULL, TO_ROOM, FALSE);
	act("$n gives you a sliver from a crystal.", creature, NULL, ch, TO_VICT, FALSE);
	act("$n gives you a sliver from a crystal.", creature, NULL, ch, TO_NOTVICT, FALSE);
	act("You give $N a sliver from a crystal.", creature, NULL, ch, TO_CHAR, FALSE);

	if(!IS_NPC(ch) && (ch->pcdata->debit_level > 0 || ch->level > 5 || 
		ch->exp > (exp_per_level(ch,ch->pcdata->points) * (ch->level + 1))))
	{
		char buf[256];
		sprintf(buf, "%s fed the poison eater a %s for 0 xp.", ch->name, obj->name);
		log_quest_detail(buf, TASK_POISON_EATER);
	}	
	else
	{// Only reward a player that can get xp
		char buf[256];
		sprintf(buf, "%s fed the poison eater a %s for 200 xp.", ch->name, obj->name);
		log_quest_detail(buf, TASK_POISON_EATER);
		gain_exp(ch, 200);
		send_to_char("You receive 200 experience points!\n\r", ch);
	}
	extract_obj(obj);// Remove the eaten object

	return edible;
}

bool check_rainbow_move(CHAR_DATA *creature, int dir)
{
  if(!creature->in_room->exit[dir] ||
        !creature->in_room->exit[dir]->u1.to_room ||
        IS_SET(creature->in_room->exit[dir]->exit_info, EX_CLOSED) ||
        creature->in_room->exit[dir]->u1.to_room->area != rainbow_area ||
        creature->in_room->exit[dir]->u1.to_room == creature->in_room)
    return FALSE;
  return TRUE;
}

bool spec_rainbow(CHAR_DATA *creature)
{
  CHAR_DATA *nearby, *partner = NULL, *player = NULL;
  int need_move = -2;
  if(!rainbow)
  {
    act("$n fades away.", creature, NULL, NULL, TO_ROOM, FALSE);
    extract_char(creature, TRUE);
    return TRUE;
  }
  if(!creature->in_room)
    return FALSE;
  if(creature->position == POS_FIGHTING)
  {/* No fighting! */
    stop_fighting(creature, TRUE);
    creature->hit = creature->max_hit; /* Rainbows don't get hurt */
  }
  for(nearby = creature->in_room->people; nearby; nearby = nearby->next_in_room)
  {
    if(nearby == creature)
      continue;
    if(!IS_NPC(nearby))
    {/* FLEE! */
      if(!IS_IMMORTAL(nearby) && need_move == -2)
      {
        if(nearby->last_move)
          need_move = rev_dir[nearby->last_move - 1];
        else
          need_move = -1;
        player = nearby;
      }
    }
    else if(!partner && nearby->pIndexData->vnum == MOB_VNUM_RAINBOW)
      partner = nearby;
  }
  if(partner)
  {/* Found its match */
    if(player)
    {/* Wait until a player is present to merge */
      char buf[256];
      OBJ_DATA *pot;
      int gold = 1000;
      if(current_time - rainbow_found < 36000)/* 10 hour max */
        gold = UMAX(50, gold * (current_time - rainbow_found) / 39600);
      pot = create_money(gold, 0);
      if(gold > 700)
      {
        clear_string(&pot->description, "A large pot of gold shines here.");
        clear_string(&pot->name, "large pot gold");
      }
      else if(gold > 300)
      {
        clear_string(&pot->description, "A pot of gold shines here.");
        clear_string(&pot->name, "pot gold");
      }
      else if(gold > 100)
      {
        clear_string(&pot->description, "A small pot of gold shines here.");
        clear_string(&pot->name, "small pot gold");
      }
      else
      {
        clear_string(&pot->description, "A tiny pot of gold shines here.");
        clear_string(&pot->name, "tiny pot gold");
      }
      obj_to_room(pot, creature->in_room);
      switch(number_range(1, 5))
      {
        case 1: strcpy(buf, "The {rr{Ra{Yi{Gn{Cb{Bo{Mw{x doubles up on itself! Oh my goodness."); break;
        case 2: strcpy(buf, "The {rr{Ra{Yi{Gn{Cb{Bo{Mw{x doubles up on itself! What does it mean?"); break;
        case 3: strcpy(buf, "The {rr{Ra{Yi{Gn{Cb{Bo{Mw{x doubles up on itself! It's so bright and vivid."); break;
        case 4: strcpy(buf, "The {rr{Ra{Yi{Gn{Cb{Bo{Mw{x doubles up on itself! Too much, it's too much."); break;
        default: strcpy(buf, "The {rr{Ra{Yi{Gn{Cb{Bo{Mw{x doubles up on itself! It's so beautiful."); break;
      }
      act(buf, creature, NULL, NULL, TO_ROOM, FALSE);
      act("As the {rr{Ra{Yi{Gn{Cb{Bo{Mw{x fades, a pot of gold shimmers into view.", creature, NULL, NULL, TO_ROOM, FALSE);
      extract_char(creature, TRUE);
      extract_char(partner, TRUE);
      rainbow_found = current_time;
      rainbow = 0;
      sprintf(buf, "The rainbow was completed by %s.", player->name);
      wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
    }
    return TRUE;
  }
  if(player)
  {/* Need to flee someone */
    char buf[256];
    ROOM_INDEX_DATA *dest;
    int i, count = 0;
    if(rainbow > 0)
      rainbow = -45; /* Half an hour to chase rainbows in circles once you get in the room with one */
    for(i = 0; i < 6; i++)
    {
      if(i == need_move || !check_rainbow_move(creature, i))
        continue;
      count++;
    }
    if(!count)
    {
      if(!check_rainbow_move(creature, need_move))
        return TRUE; /* Can't move */
      i = need_move; /* Move out, only way */
    }
    else
    {
      count = number_range(1, count);
      for(i = 0; i < 6; i++)
      {
        if(i == need_move || !check_rainbow_move(creature, i))
          continue;
        count--;
        if(!count)
          break;
      }
    }
    dest = creature->in_room->exit[i]->u1.to_room;
    sprintf(buf, "$n shifts away %s.", dir_name[i]);
    act(buf, creature, NULL, NULL, TO_ROOM, FALSE);
    char_from_room(creature);
    char_to_room(creature, dest);
    act("$n bathes you with its light.", creature, NULL, NULL, TO_ROOM, FALSE);
    sprintf(buf, "The rainbow is being chased by %s.", player->name);
    wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
  }
  return TRUE;
}

/* Not a true quest, but I'd like to track it anyway */
bool spec_poison_eater(CHAR_DATA *creature)
{
	if(number_percent() < 70)
		return FALSE;
	if(creature->position == POS_FIGHTING)
	{// Watch out, its breath is NASTY!
//		if(number_percent() < 50)// Don't breath too often
		//	spec_breath_gas(creature);
		return TRUE;
	}
	if(number_percent() < 65)
	{
		OBJ_DATA *obj;
		OBJ_DATA *obj_next = NULL;
		switch(number_range(1, 3))
		{
			case 1: act("$n looks around for fresh, poisonous body parts to consume.", creature, NULL, NULL, TO_ROOM, FALSE);
				break;
			case 2: act("$n snuffles around the room, clearly looking for something.", creature, NULL, NULL, TO_ROOM, FALSE);
				break;
			case 3: act("$n drools a bit.  The globs sizzle when they hit the ground.", creature, NULL, NULL, TO_ROOM, FALSE);
				break;
		}
		/* Check if there's any guts or poison flagged bits that have not been cryo'ed */
		for ( obj = creature->in_room->contents; obj != NULL; obj = obj_next )
    {
      obj_next = obj->next_content;
      if(poison_eater_handler(creature, NULL, obj))
      {// Pick it up and consume it
		act("$n picks up and messily consumes $p.", creature, obj, NULL, TO_ROOM, FALSE);
		extract_obj(obj);
		break;// Only eat one per time, regardless of how many are in the room
      }
		}
	}
	else
	{// Move somewhere within mud school
		if(creature->position == POS_STANDING && creature->in_room != NULL)
		{
			int i;
	    EXIT_DATA *pExit;
	    int count = 0;
			for ( i = 0 ; i < 6 ; i++ )
			{
				pExit = creature->in_room->exit[i];

				if ( pExit == NULL || IS_SET(pExit->exit_info, EX_CLOSED))
				    continue;
				if(IS_SET(pExit->u1.to_room->room_flags, ROOM_NEWBIES_ONLY))
					count++;
			}
			if(count > 0)
			{// If not, it's stuck somewhere
				if(count > 1)
					count = number_range(1, count);
				for ( i = 0 ; i < 6 ; i++ )
				{
					pExit = creature->in_room->exit[i];
	
					if ( pExit == NULL || IS_SET(pExit->exit_info, EX_CLOSED))
					    continue;
					if(IS_SET(pExit->u1.to_room->room_flags, ROOM_NEWBIES_ONLY))
					{
						count--;
						if(count <= 0)
							break;
					}
				}
				if(i == 6 || count < 0)
					return FALSE;// Some problem with this
				move_char( creature, i, FALSE );
			}			
		}
	}
	return TRUE;
}

/* A function to use for any quest based kill (Rather than kill through damage) */
bool do_quest_kill(CHAR_DATA *victim, CHAR_DATA *killer, bool ignorewithstand, bool deathcry, int bodypart, int quest)
{
	int i;
	char *msg;
	char log_buf[256], buf[256];
	if(victim == NULL)
		return FALSE;// Were not killed
	if(victim->in_room == NULL)
		return FALSE;// Were not killed
	if(killer == NULL)
		killer = victim;

	sprintf(buf, "%s quest killed by %s.", victim->name, killer->name);
	log_quest_detail(buf, quest);

	act( "$n is {RDEAD{x!!", victim, 0, 0, TO_ROOM ,FALSE);
	send_to_char( "You have been {RKILLED{x!!\n\r\n\r", victim );

    stop_fighting( victim, TRUE );
  if(deathcry)
  {
  	ROOM_INDEX_DATA *was_in_room;
  	int door;
    if ( IS_NPC(victim) )
  msg = "You hear something's death cry.";
    else
  msg = "You hear someone's death cry.";

    was_in_room = victim->in_room;
    for ( door = 0; door <= 5; door++ )
    {
  EXIT_DATA *pexit;

  if ( ( pexit = was_in_room->exit[door] ) != NULL
  &&   pexit->u1.to_room != NULL
  &&   pexit->u1.to_room != was_in_room )
  {
      victim->in_room = pexit->u1.to_room;
      act( msg, victim, NULL, NULL, TO_ROOM ,FALSE);
  }
    }

    victim->in_room = was_in_room;
	}
		/* Wound transfer death code removed here, quest kills don't currently trigger it */

    if ( IS_NPC(victim) )
    {
     sprintf( log_buf, "%s got quest killed by %s at %s [room %d]",
      (IS_NPC(victim) ? victim->short_descr : victim->name),
      (IS_NPC(killer) ? killer->short_descr : killer->name),
      victim->in_room->name, victim->in_room->vnum);
          wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
   make_corpse (victim, killer);
  victim->pIndexData->killed++;
  kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
  extract_char( victim, TRUE );
  return TRUE;
    }

	if(!ignorewithstand && IS_SET(victim->affected_by,AFF_WITHSTAND_DEATH))
	{/* They withstood this death */
     victim->position    = POS_STANDING;
     while ( victim->flash_affected )
        flash_affect_remove( victim, victim->flash_affected,APPLY_BOTH );
     while ( victim->affected )
        affect_remove( victim, victim->affected,APPLY_BOTH );

    victim->alert = 0;
    if(victim->pcdata && victim->pcdata->clan_info)
      victim->pcdata->clan_info->lost_merit = 0;

     victim->affected_by = victim->affected_by|race_table[victim->race].aff;
     victim->hit         = victim->max_hit/8;
     stop_fighting(victim, TRUE);
     act("$n twitches a bit then stands up.",victim,NULL,NULL,TO_ROOM,FALSE);
     send_to_char ("A chilling wave passes over as you withstand death.\n\r",victim);
     WAIT_STATE(victim, 4);
  	if(victim->pcdata->deity_trial_timer > 0)
  	{
    	sprintf(buf, "%s allows you to continue in your trial.\n\r", deity_table[victim->pcdata->deity].pname);
    	send_to_char(buf, victim);
  	}

		return FALSE;
	}
	else
	{// Silent death, head falls off
		int vnum = 0;
		msg = "$n hits the ground ... DEAD.";
		if(bodypart > 0 && IS_SET(victim->parts,bodypart))
	  {
	  	switch(bodypart)
	  	{
  			case PART_GUTS: msg = "$n spills $s guts all over the floor.";
      		vnum = OBJ_VNUM_GUTS; break;
      	case PART_HEAD: msg  = "$n's severed head plops on the ground.";
					vnum = OBJ_VNUM_SEVERED_HEAD; break;
				case PART_HEART: msg  = "$n's heart is torn from $s chest.";
		      vnum = OBJ_VNUM_TORN_HEART; break;
				case PART_ARMS: msg  = "$n's arm is sliced from $s dead body.";
		      vnum = OBJ_VNUM_SLICED_ARM; break;
				case PART_LEGS: msg  = "$n's leg is sliced from $s dead body.";
     			vnum = OBJ_VNUM_SLICED_LEG; break;
     		case PART_BRAINS: msg = "$n's head is shattered, and $s brains splash all over you.";
  		    vnum = OBJ_VNUM_BRAINS; break;
  		}
			act( msg, victim, NULL, NULL, TO_ROOM ,FALSE);
			if ( vnum != 0 )
			{
			  OBJ_DATA *obj;
			  char *name;
			
			  name    = IS_NPC(victim) ? victim->short_descr : victim->name;
			  obj   = create_object( get_obj_index( vnum ), 0, FALSE );
			  obj->timer  = number_range( 4, 7 );
			
			  sprintf( buf, obj->short_descr, name );
			  /*free_string( obj->short_descr );*/
			  obj->short_descr = str_dup( buf );
			
			  sprintf( buf, obj->description, name );
			  /*free_string( obj->description );*/
			  obj->description = str_dup( buf );
			
			  if (obj->item_type == ITEM_FOOD)
			  {
			      if (IS_SET(victim->form,FORM_POISON))
			    obj->value[3] = 1;
			      else if (!IS_SET(victim->form,FORM_EDIBLE))
			    obj->item_type = ITEM_TRASH;
			  }
			
			   /* The vnum is saved on the item */
			  obj->value[2] = (IS_NPC(victim) ? victim->pIndexData->vnum : 0 );
			
			  obj_to_room( obj, victim->in_room );
			}
		}

     sprintf( log_buf, "%s got quest killed by %s at %s [room %d]",
      (IS_NPC(victim) ? victim->short_descr : victim->name),
      (IS_NPC(killer) ? killer->short_descr : killer->name),
      victim->in_room->name, victim->in_room->vnum);
      wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
		  if( !is_clan(victim) )
          pnet("$N died.",victim,NULL,PNET_DEATHS,0,0);
  	if(victim->pcdata->deity_trial_timer > 0)
  	{
    	victim->pcdata->deity_trial_timer = 0;
    	sprintf(buf, "You have failed your trial from %s.\n\r", deity_table[victim->pcdata->deity].pname);
    	send_to_char(buf, victim);
    	log_deity_favor(victim, killer, DEITY_TRIAL_FAIL_DEATH);
  	}
     if (IS_SET (victim->in_room->room_flags,ROOM_NODIE)
         || (victim->clan == nonclan_lookup("smurf") 
             || victim->clan == nonclan_lookup("smurf")))
     {
        act("$n disintegrates into dust.",victim,NULL,NULL,TO_ROOM,FALSE);
        char_from_room (victim);
        clear_mount( victim );
         if (IS_SET(victim->act, PLR_DWEEB))
            char_to_room(victim,get_room_index(ROOM_VNUM_ALTAR));
         else
            char_to_room(victim,get_room_index(clan_table[victim->clan].hall));
     } 
     else 
     {
        make_corpse( victim, killer);
        extract_char( victim, FALSE );
     }
     while ( victim->flash_affected )
        flash_affect_remove( victim, victim->flash_affected,APPLY_BOTH );
     while ( victim->affected )
        affect_remove( victim, victim->affected,APPLY_BOTH );

    victim->alert = 0;
    if(victim->pcdata && victim->pcdata->clan_info)
      victim->pcdata->clan_info->lost_merit = 0;

     victim->affected_by = race_table[victim->race].aff;

     for (i = 0; i < 4; i++)
        victim->armor[i]= 100;

     victim->position    = POS_RESTING;
     victim->hit         = number_range(victim->level / 2, victim->level * 3 / 4);
     victim->mana        = UMIN( 20, victim->mana );
     victim->move        = UMAX( 1, victim->move );
     
     do_look( victim, "auto" );

     return TRUE;
	}
	return FALSE;// Not sure how we got here, but they didn't die
}

/* Handle the responses the mime can make */
void insane_mime_handler(CHAR_DATA *mime, CHAR_DATA *target, int update)
{
	char buf[256];
	int i;
	ROOM_INDEX_DATA *pRoomIndex;
	switch(update)
	{
		case QSTEP_MOVE:
			act("$n shrugs and begins to run in place, fading as $e goes.", mime,NULL,NULL,TO_ROOM,FALSE);
	    stop_fighting(mime, TRUE);// Just in case
	    pRoomIndex = get_random_room(mime);
			char_from_room( mime );
			char_to_room( mime, pRoomIndex );
			mime->zone = NULL;// Clear the zone, no wandering home
			act( "$n fades into the room, running in place.", mime, NULL, NULL, TO_ROOM ,FALSE);
			if(mime->qchar != NULL)
			{
				mime->qchar->pcdata->quest_count = UMAX(0, mime->qchar->pcdata->quest_count - 1);// No longer interested in this player
				mime->qchar = NULL;// Reset mime values
			}
			mime->qnum = 0;
			mime->qnum2 = 0;
			break;
		case QSTEP_WIN:// Next step, or victory if enough wins
			// (Level + 4) / 5 socials in order to win
		{
			int wins_needed = mime->qchar->level / 5;
			if(mime->qnum < 0)
			{
				mime->qnum = 0;
				sprintf(buf, "%s has accepted the mime duel!", mime->qchar->name);
				wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
			}
			if(mime->qnum / 16 >= wins_needed)
			{// They have won enough - 1 per every 5 levels, 11 at 51
				mime->qchar->pcdata->quest_wins[QUEST_MIME]++;
				if(wins_needed < 10)
				{// XP reward + a little gold, higher chance of winning a "jackpot" bonus
					// Reward a little more gold and no xp if they already qualify for level
					int gold = 0;
					int xp = 0;
					if(number_percent() < 28 + mime->qchar->pcdata->quest_wins[QUEST_MIME] * 2)// Base 30%
					{// Clearing the counter often also means you can't build the counter much for the cone before you can earn the cone
						mime->qchar->pcdata->quest_wins[QUEST_MIME] = 0;// Clear the counter
						xp = 200;
						gold = UMAX(5, wins_needed * 5 + number_range(wins_needed, wins_needed * 4));
						act("$n makes a deep bow before you then hands you some coins.", mime, NULL, mime->qchar, TO_VICT, FALSE);
						act("$n makes a deep bow before $N then hands $M some coins.", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
					}
					else
					{
						xp = 100;
						gold = UMAX(3, wins_needed * 4 + number_range(0, wins_needed * 3));
						act("$n bows in recognition of your victory then hands you some coins.", mime, NULL, mime->qchar, TO_VICT, FALSE);
						act("$n bows in recognition of $N's victory then hands $M some coins.", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
					}
			    if ( (mime->qchar->pcdata->debit_level > 0) || 
						(mime->qchar->exp > exp_per_level(mime->qchar,mime->qchar->pcdata->points) * (mime->qchar->level + 1)))
					{// Already qualified, don't let someone store a bunch of levels from this
						xp = 0;
						gold += wins_needed * 3;// Averages less than a 51 still. 
					}
					if(xp > 0)
					{
						gain_exp(mime->qchar, xp);
						sprintf(buf, "You receive %d experience points!\n\r", xp);
						send_to_char(buf, mime->qchar);
					}
					send_to_char("You have gained some skill!\n\r", mime->qchar);
					mime->qchar->skill_points++;
					mime->qchar->gold += gold;
					sprintf(buf, "%s won insane mime for %d gold and %d xp", mime->qchar->name, gold, xp);
					log_quest_detail(buf, QUEST_MIME);
				}
				else
				{// A diamond, low but increasing chance of winning cone of silence
					if(number_percent() < (mime->qchar->pcdata->quest_wins[QUEST_MIME] - 1) / 4)
					{// First 4 you have no chance, then 1% next 10, 2% next 10, etc.
					// After 40 mime victories you have nearly a 50% chance to have earned this, should be doable
					// Win cone of silence!
						mime->qchar->pcdata->quest_wins[QUEST_MIME] = 0;// Clear this counter, you start fresh for earning it again
						act("$n grabs an invisible bag, then pulls a crumpled scroll out of it.", mime, NULL, NULL, TO_ROOM, FALSE);
						act("$n's shoulders slump in defeat as $e hands you a crumpled scroll.", mime, NULL, mime->qchar, TO_VICT, FALSE);
						act("$n's shoulders slump in defeat as $e hands $N a crumpled scroll.", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
//						act("$n's shoulders slump in utter defeat as $e hands you a jewelled shard.", mime, NULL, mime->qchar, TO_VICT, FALSE);
//						act("$n's shoulders slump in utter defeat as $e hands $N a jewelled shard.", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
						obj_to_char(create_object(get_obj_index(OBJ_VNUM_SCROLL_SILENCE),0,FALSE),mime->qchar);
						sprintf(buf, "%s won insane mime for shard.", mime->qchar->name);
						log_quest_detail(buf, QUEST_MIME);
					}
					else
					{// Just a diamond - not bad anyway
						act("$n bows in recognition of your victory and hands you a diamond.", mime, NULL, mime->qchar, TO_VICT, FALSE);
						act("$n bows before $N and hands $M a diamond.", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
						obj_to_char(create_object(get_obj_index(OBJ_VNUM_DIAMOND),0,FALSE),mime->qchar);
						if(mime->qchar->level < LEVEL_HERO)
						{
							gain_exp(mime->qchar, 100);
							send_to_char("You receive 100 experience points!\n\r", mime->qchar);
						}
						sprintf(buf, "%s won insane mime for a diamond.", mime->qchar->name);
						log_quest_detail(buf, QUEST_MIME);
					}
					send_to_char("You have gained some skill!\n\r", mime->qchar);
					if(mime->qchar->level < LEVEL_HERO)
						mime->qchar->skill_points++;
					else
						mime->qchar->skill_points+=2;
				}
				act("$n makes a wiping motion in the air, vanishing as $s hand passes!", mime,NULL,NULL,TO_ROOM,FALSE);
		    stop_fighting(mime, TRUE);// Just in case
		    pRoomIndex = get_random_room(mime);
				char_from_room( mime );
				char_to_room( mime, pRoomIndex );
				act( "$n appears in the room as $e wipes away invisible dirt.", mime, NULL, NULL, TO_ROOM ,FALSE);
				mime->qchar->pcdata->quest_count = UMAX(0, mime->qchar->pcdata->quest_count - 1);// No longer interested in this player
				mime->qchar = NULL;// Reset mime values
				mime->qnum = 0;
				mime->qnum2 = 0;
			}
			else
			{// Next social
				int count = number_range(1, social_count_targeted);
				for ( i = 0; i < social_count; i++ )
				{
					if ( social_table[i].vict_found != NULL)
					{
						count--;
						if(count <= 0)
							break;
					}
				}
				if(i == social_count || count < 0)
				{// Failed to find a social
					bug("spec_insane_mime: Failed to find social to use", 0);
					insane_mime_handler(mime, mime->qchar, QSTEP_MOVE);// Abort!
					return;
				}
         act( social_table[i].others_found, mime,
                  NULL, mime->qchar, TO_NOTVICT ,FALSE);
         act( social_table[i].char_found, mime,
                  NULL, mime->qchar, TO_CHAR    ,FALSE);
         act( social_table[i].vict_found, mime,
                  NULL, mime->qchar, TO_VICT    ,FALSE);

				mime->qnum = (mime->qnum & 496) + 16;// Strip out 1-15 (Supports up to 30 socials)  
				mime->qnum2 = i;// The social to respond to
			}
		}
      break;
    case QSTEP_LOSE:
			/* Hide the exact base number */
			i = number_range(mime->qchar->level + 1, mime->qchar->level * 3 / 2 + 2);
			act("$n grins in triumph and shoves $s hands towards you!", mime,NULL,mime->qchar,TO_VICT,FALSE);
			act("$n grins in triumph and shoves $s hands towards $N!", mime,NULL,mime->qchar,TO_NOTVICT,FALSE);
			if(mime->qchar->hit > i)
			{// Leave them at enough hp so remort burn probably won't kill them
				send_to_char("The world {Yshatters{x around you, shards {Rcutting{x into you as they fly past!\n\r", mime->qchar);
			  if( mime->qchar->hit - i > mime->qchar->max_hit / 4 )
  	     send_to_char("That really did {YHURT{x!\n\r", mime->qchar);
				if ( i < mime->qchar->max_hit / 4 && 
				!IS_SET(mime->qchar->comm,COMM_SILENCE) )
			   {
			     send_to_char("You sure are {RBLEEDING{x!\n\r", mime->qchar);
			   }
				mime->qchar->hit = i;
			}
			else// No damage dealt, different message
				send_to_char("The world {Yshatters{x around you!\n\r", mime->qchar);
			send_to_char("\n\r", mime->qchar);
			sprintf(buf, "%s failed insane mime at step %d on %s.", mime->qchar->name, mime->qnum / 16, social_table[mime->qnum2].name);
			log_quest_detail(buf, QUEST_MIME);
		    act( "$n {Wvanishes{x abruptly!", mime->qchar, NULL, NULL, TO_ROOM ,FALSE);
        pRoomIndex = get_random_room(mime->qchar);
		    char_from_room(mime->qchar);
		    char_to_room(mime->qchar, pRoomIndex );
		    clear_mount(mime->qchar);
		    act( "$n suddenly appears in the room!", mime->qchar, NULL, NULL, TO_ROOM ,FALSE);
		    do_look(mime->qchar, "auto" );
    	break;
    case QSTEP_QUIT:
    	if(mime->qchar == target)// Mime doesn't care if anyone else quits
				insane_mime_handler(mime, mime->qchar, QSTEP_MOVE);
    	break;
    case QSTEP_NPC_DEAD:
      if(mime->qchar)
      {// Clear the values if he's doing anything
        mime->qchar->pcdata->quest_count = UMAX(0, mime->qchar->pcdata->quest_count - 1);// No longer interested in this player
        mime->qchar = NULL;// Reset mime values
        mime->qnum = 0;
        mime->qnum2 = 0;
      }
      break;
  }
}

/* Separate function to save doubling up this list */
void quest_handler_function(CHAR_DATA *quest_npc, CHAR_DATA *ch, OBJ_DATA *obj, int quest, int update)
{
  switch(quest)
  {
    case TASK_POISON_EATER: poison_eater_handler(quest_npc, ch, obj);
      break;
    case TASK_CLAN_GUARDIAN: 
      if(update == QSTEP_QUIT)
        release_clan_guardian(quest_npc);
      break;/* Only thing guardians have here is owner quit or they died */
    case QUEST_MIME: insane_mime_handler(quest_npc, ch, update); 
      break;
  }	
} 

int get_quest_vnum(CHAR_DATA *quest_npc)
{
   switch(quest_npc->pIndexData->vnum)
   {// Match the quest to the vnum here
     case MOB_VNUM_INSANE_MIME: return QUEST_MIME;
     case MOB_VNUM_CLAN_GUARDIAN: return TASK_CLAN_GUARDIAN;
   }
   return QUEST_UNKNOWN;
}

/* Ensure that all the quest handler functions can be accessed through here */
/* Passes on only the variables that each function needs */
void quest_handler(CHAR_DATA *quest_npc, CHAR_DATA *ch, OBJ_DATA *obj, int quest, int update)
{
  CHAR_DATA *qnext = NULL;
  if(update == QSTEP_QUIT && quest_npc == NULL)
  {// All of them -- if NPC is NULL, find each.
    if(ch == NULL || IS_NPC(ch) || ch->pcdata->quest_count <= 0)
      return;// Shouldn't have been called
    for ( quest_npc = char_list; quest_npc != NULL; quest_npc = qnext )
    {
      qnext = quest_npc->next;
      if ( IS_NPC(quest_npc) && quest_npc->in_room != NULL)
      {
        if((quest = get_quest_vnum(quest_npc)) != QUEST_UNKNOWN)
          quest_handler_function(quest_npc, ch, obj, quest, update);
      }
    }
    return;
  }
  if(quest == QUEST_UNKNOWN)
  {
   if(quest_npc == NULL || !IS_NPC(quest_npc))
     return;
   quest = get_quest_vnum(quest_npc);
   if(quest == QUEST_UNKNOWN)
   {
     bug("Unable to locate quest npc", 0);
     return;// Couldn't find it
   }
  }
  quest_handler_function(quest_npc, ch, obj, quest, update);// Pass it straight through
}

/* Mime target is the player it is duelling with
* qnum is a timer, pre-duel it goes -1 per pulse, while duelling it goes by +16
* (Lower bits for timeout - because of +16, can use up to 15 timeout if needed)
* qnum2 is the expected answering social */
bool spec_insane_mime(CHAR_DATA *mime)
{
	char buf[256];
	CHAR_DATA *target, *t_next;
	ROOM_INDEX_DATA *pRoomIndex;
	int i;
	bool teleport = FALSE;
	if(mime->qchar && mime->qchar->in_room != mime->in_room)
	{// The target left, get bored and bug out
		teleport = TRUE;
		if(mime->qnum > 0)
		{
			sprintf(buf, "%s failed insane mime by leaving at step %d on %s.", mime->qchar->name, mime->qnum / 16, social_table[mime->qnum2].name);
			log_quest_detail(buf, QUEST_MIME);
		}
	}
	else if(mime->qchar && mime->qnum > 0 && mime->qchar->fighting)
	{
		teleport = TRUE;
		if(mime->qchar->fighting == mime)
		{// The mime-dueller attacked the mime.  Kill them.
			act("$n glares at $N and draws $s hand across $s throat!", mime, NULL, mime->qchar, TO_NOTVICT, FALSE);
			act("$n glares at you and draws $s hand across $s throat!", mime, NULL, mime->qchar, TO_VICT, FALSE);
			act("$n looks shocked as a {Rcrimson{x line traces across $s neck.", mime->qchar, NULL, NULL, TO_ROOM, FALSE);
			send_to_char("You feel a {Rblazing pain{x shoot through your neck!\n\r", mime->qchar);
			do_quest_kill(mime->qchar, mime, FALSE, FALSE, PART_HEAD, QUEST_MIME);
		}
		{// Someone interrupted the duel with combat
			act("$n looks insulted.\n\r$n quickly acts out a box shrinking around $mself and {Wvanishes{x!\n\r", mime,NULL,NULL,TO_ROOM,FALSE);
      stop_fighting(mime, TRUE);
      pRoomIndex = get_random_room(mime);
		  char_from_room( mime );
		  char_to_room( mime, pRoomIndex );
		  act( "A pair of hands fade in and push away invisible walls.\n\r$n appears in the room!", mime, NULL, NULL, TO_ROOM ,FALSE);
		  mime->qchar->pcdata->quest_count = UMAX(0, mime->qchar->pcdata->quest_count - 1);// No longer interested in this player
		  mime->qchar = NULL;// Reset mime values
		  mime->qnum = 0;
		  mime->qnum2 = 0;
		  return TRUE;
		}
	}
	if(mime->fighting != NULL)
	{// Check for anyone else fighting the mime
		if(mime->qchar == NULL)
			teleport = TRUE;// Don't let others break a duel
		mime->hit = mime->max_hit;
		act("$n looks annoyed and flicks $s fingers in a shooing motion.", mime,NULL,NULL,TO_ROOM,FALSE);
		for(target = mime->in_room->people;  target != NULL; target = t_next)
    {
        t_next = target->next_in_room;
        if(target == mime || target->fighting != mime)
        	continue;
        /* Someone is in combat with the mime, teleport them. There is no way to avoid being teleported. */
        stop_fighting(target, TRUE);
				send_to_char("You have been teleported!\n\r\n\r",target);
		    act( "$n {Wvanishes{x!", target, NULL, NULL, TO_ROOM ,FALSE);
        pRoomIndex = get_random_room(target);
		    char_from_room( target );
		    char_to_room( target, pRoomIndex );
		    clear_mount(target);
		    act( "$n slowly fades into existence.", target, NULL, NULL, TO_ROOM ,FALSE);
		    do_look( target, "auto" );
    }
	}
	if(!teleport)
	{
		if(mime->qchar == NULL)
		{
			if(number_range(1, 5) == 1)
			{// Check if there's a valid target in the room
				i = 0;// Count of players
				for(target = mime->in_room->people; target != NULL; target = target->next_in_room)
				{
					if(IS_NPC(target) || IS_IMMORTAL(target))
						continue;
					i++;
				}
				if(i > 1)
					i = number_range(1, i);
				if(i != 0)
				{// Select a target
					for(target = mime->in_room->people; target != NULL; target = target->next_in_room)
					{
						if(IS_NPC(target) || IS_IMMORTAL(target))
							continue;
						i--;
						if(!i)
							break;
					}
					if(target)
					{// We have a player target now
						for ( i = 0; i < social_count; i++ )
						{
						 if ( 'b' == social_table[i].name[0]
						     && !str_prefix( "bow", social_table[i].name ) )
						 break;
						}
						if(i == social_count)
						{// Failed to find a social
							bug("spec_insane_mime: Failed to find bow social", 0);
							return TRUE;
						}
         act( social_table[i].others_found, mime,
                  NULL, target, TO_NOTVICT ,FALSE);
         act( social_table[i].char_found, mime,
                  NULL, target, TO_CHAR    ,FALSE);
         act( social_table[i].vict_found, mime,
                  NULL, target, TO_VICT    ,FALSE);

						mime->qnum = -1;// Begin waiting - initial one is negative
						mime->qnum2 = i;// The social to respond to
						mime->qchar = target;
						mime->qchar->pcdata->quest_count++;
						sprintf(buf, "%s has been challenged to a mime duel.", mime->qchar->name);
						wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
					}
				}
			}
			else if(number_range(1, 40) == 1)
			{// Randomly bored.

				for(target = mime->in_room->people; target != NULL; target = target->next_in_room)
				{
					if(IS_NPC(target) || IS_IMMORTAL(target))
						continue;
					break;
				}
				if(target == NULL)// Teleport if nobody is in the room, else wait to challenge them then teleport
					teleport = TRUE;
			}
			else if(number_percent() < 50)
			{
				switch(number_range(1, 9))
				{
            case 1: act("$n is trapped in an invisible box! Panic flashes in $s eyes.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 2: act("$n wipes down some non-existant windows very carefully.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 3: act("$n opens an invisible door, steps through it, turns, and locks it.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 4: act("$n smiles with exaggerated pleasure.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 5: act("$n melodramatically collapses in despair.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 6: act("$n twitches a bit.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 7: act("$n sets up an invisible easel and paints nothing.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 8: act("$n strikes a pose and holds it a while.", mime, NULL, NULL, TO_ROOM, FALSE); break;
            case 9: act("$n peers around intently, looking very suspicious.", mime, NULL, NULL, TO_ROOM, FALSE); break;
				}
			}
		}
		else
		{
			if(mime->qnum < 0)
			{
				mime->qnum--;
				if(mime->qnum < -5)// He's out of here, target didn't react in time
					teleport = TRUE;
			}
			else
			{
				mime->qnum++;
				if((mime->qnum & 15) > 2)// Mask to 15 for convenience - Do NOT go above 15, it counts by 16's
				{// Lost the duel due to timeout
					insane_mime_handler(mime, mime->qchar, QSTEP_LOSE);
					teleport = TRUE;
				}
			}
		}
	}
	if(teleport)
	{
		insane_mime_handler(mime, mime->qchar, QSTEP_MOVE);
		return TRUE;
	}
	return FALSE;
}

bool check_guardian_target(CHAR_DATA *guardian, CHAR_DATA *target)
{
  if(guardian->qchar == target)
    return FALSE;
  if(clan_kill_type(guardian->qchar, target))
  {
    if(!is_safe(guardian->qchar, target))
    {
      return TRUE;
    }
  }
  return FALSE;  
}

void do_guardian_attack(CHAR_DATA *guardian)
{
  CHAR_DATA *target;
  for(target = guardian->in_room->people; target; target = target->next_in_room)
  {
    if(IS_NPC(target) || !check_guardian_target(guardian, target))
      continue;
    if(can_see(guardian->qchar, target, FALSE))
      break;
  }
  if(target)
  {
    act("$n screams and attacks!", guardian, NULL, NULL, TO_ROOM, FALSE);
    multi_hit(guardian, target, TYPE_UNDEFINED);
  }
}

/* Higher levels of guardian skills can do more things, such as calling when they're
   attacking or being attacked, spotting, and announcing who the enemy is */
/* Also can enable special skill usage while fighting in the room with the owner */
bool spec_clan_guardian(CHAR_DATA *guardian)
{
  if(!IS_NPC(guardian))
    return FALSE;
  if(!guardian->qchar)
  {
    act("$n flares and vanishes as $e is released from service!\n\r", guardian, NULL, NULL, TO_ROOM, FALSE);
    extract_char(guardian, TRUE);
    return TRUE;
  }
  if(!guardian->master && IS_SET(guardian->qnum, GUARDIAN_FOLLOW))
  {
    guardian->master = guardian->qchar;
    if(guardian->in_room == guardian->qchar->in_room)
      act("$N now follows you.", guardian->qchar, NULL, guardian, TO_CHAR, FALSE);
  }
  if(guardian->fighting)/* No special AI once it's fighting */
    return FALSE;/* It can use any standard actions it has */
  if(IS_SET(guardian->qnum, GUARDIAN_PEACE))
    return TRUE; /* Not doing anything */
  if(IS_SET(guardian->qnum, GUARDIAN_ASSIST))
  {/* No movement check, can do an attack check */
    if(guardian->in_room == guardian->qchar->in_room)
    {
      if(guardian->qchar->fighting &&
        check_guardian_target(guardian, guardian->qchar->fighting))
      {
        act("$n screams and attacks!", guardian, NULL, NULL, TO_ROOM, FALSE);
        multi_hit(guardian, guardian->qchar->fighting, TYPE_UNDEFINED);
      }
    }
    return TRUE;
  }
  if(IS_SET(guardian->qnum, GUARDIAN_ATTACK))
  {/* Look for a target.  This is based on if its owner can see the target. */
    do_guardian_attack(guardian);
  }
  return TRUE;
  /* Not set to sentinel, not in the same room, not fighting -- chase its owner */
  /*if(!IS_SET(guardian->qnum, GUARDIAN_SNEAK))
    act("$n seeks $s master.", guardian, NULL, NULL, TO_ROOM, FALSE);
  char_from_room(guardian);
  char_to_room(guardian, guardian->qchar->in_room);
  if(!IS_SET(guardian->qnum, GUARDIAN_SNEAK))
    act("$n has arrived.", guardian, NULL, NULL, TO_ROOM, FALSE);*/
}

void bounty_admin_claim(CHAR_DATA *ch, CHAR_DATA *shade, OBJ_DATA *obj)
{
  char buf[255];
  int value = 0, gold = 0, sp = 0;
  if(IS_SET(shade->affected_by_ext, AFF_EXT_SHADED))
  {
    send_to_char("You realize that would be a bad idea.\n\r", ch);
    return;
  }
  if(obj->pIndexData->vnum != OBJ_VNUM_BOUNTY_EAR)
  {
    send_to_char("It has no use for that.\n\r", ch);
    return;
  }
  if(IS_NPC(ch))
  {
    send_to_char("It is not interested in receiving that from you.\n\r", ch);
    return;
  }
  act( "$n gives $p to $N.", ch, obj, shade, TO_NOTVICT ,FALSE);
  act( "$n gives you $p.",   ch, obj, shade, TO_VICT    ,FALSE);
  act( "You give $p to $N.", ch, obj, shade, TO_CHAR    ,FALSE);
  send_to_char("The man {gwhispers{x to you, 'Good job.  Here is the reward you were promised.'\n\r", ch);
  value = obj->value[0] + obj->value[2] - 1;
  if(obj->value[0] > BOUNTY_ITEM_NAME)
  {
    sp = 2;// Guarantees 2 skill points
  }
  else if(obj->value[1] > 0)
  {
    MOB_INDEX_DATA *base = get_mob_index(obj->value[1]);
    if(base)
    {
      if(base->level >= 30)
        sp = 1;
      else if(base->level >= 20 && number_percent() < 50)
        sp = 1;// 50/50 for bonus reward at 20
    }
  } 
  while(value > 0)
  {
    if(sp == gold / 2)// Can go either way
      number_percent() < 50 ? sp++ : gold++;
    else if(sp < gold / 2)
      sp++;
    else
      gold++;
    value--;
  }
  if(!sp)
  {
    sp++;
    gold--;
  }
  else if(sp > 3)// 3 is max
  {
    gold += sp - 3;
    sp = 3;
  }
  if(IS_SET(obj->value[3], BOUNTY_AWARD_UPGRADED))
    sp++;
  if(IS_SET(obj->value[3], BOUNTY_AWARD_HARD))
    sp++;

  gold *= obj->value[0] > BOUNTY_ITEM_NAME ? 25 : 20;
  if(IS_SET(obj->value[3], BOUNTY_AWARD_STONE))
    gold += 100;// Flat bonus for completing an exceptional

  sprintf(buf, "You receive %d gold and %d skill point%s\n\r", gold, sp,
    sp == 1 ? "." : "s.");
  ch->gold += gold;
  ch->skill_points += sp;
  send_to_char(buf, ch);
  if(IS_SET(obj->value[3], BOUNTY_AWARD_STONE))
  {
    OBJ_DATA *stone = create_object(get_obj_index(OBJ_VNUM_SHADE_STONE),35,FALSE);
    send_to_char("'That's an exceptional bounty. Accept this as well, it is valued among us.'\n\r", ch);
    act("$n gives you $p.", shade, stone, ch, TO_VICT, FALSE);
    obj_to_char(stone, ch);
    act( "A dark shadow passes briefly over $n.", ch, NULL, NULL, TO_ROOM,FALSE);
    sprintf(buf, "%s completed big bounty! %d sp, %d gold. (%d %d %d %d)",
      ch->name, sp, gold, obj->value[0], obj->value[1], obj->value[2],
      obj->value[4]);// value[3] is already covered by big bounty
    log_quest_detail(buf, QUEST_BOUNTY_CLAIM);
  }
  else
  {
    act( "A shadow passes briefly over $n.", ch, NULL, NULL, TO_ROOM,FALSE);
    sprintf(buf, "%s completed bounty! %d sp, %d gold. (%d %d %d %d)",
      ch->name, sp, gold, obj->value[0], obj->value[1], obj->value[2],
      obj->value[4]);// value[3] is already covered by big bounty
    log_quest_detail(buf, QUEST_BOUNTY_CLAIM);
  }
  extract_obj(obj);
  if(!ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM])
    ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM] = 2;
  else
    ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM]++;
  if(ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM] == 2)
  {
    send_to_char("\n\rA voice {gwhispers{x, 'Your assistance has been noted and will be remembered.'\n\r", ch);
  }
  else if(ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM] == 3)
  {
    send_to_char("\n\rA voice {gwhispers{x, 'Good work, but you must perform a few more services\n\r", ch);
    send_to_char("  for us if you would convince us of your true sincerity.'\n\r", ch);
  }
  else if(ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM] == 6)
  {
    send_to_char("\n\rA voice {gwhispers{x, 'You have served us well, so we grant you this boon:\n\r", ch);
    send_to_char("  If you sit in the room we watch from here, you will hear our desired bounty.'\n\r", ch);
  }
  return;
}

bool is_bounty_target(CHAR_DATA *victim, bool kill)
{
  if(!IS_NPC(victim) || victim->pIndexData->vnum != bounty_vnum)
    return FALSE;
  if(!kill && bounty_timer <= 0)
    return FALSE;// Allow to gate to/summon a bounty if nobody knows it's active
  return TRUE;
}

// Since it uses %# to determine messages try to not line them up with clean
// multiples, or certain messages will always pair with others
void describe_mob_bounty(CHAR_DATA *ch, CHAR_DATA *teller, bool just_started)
{
  char buf[256];
  MOB_INDEX_DATA *pMobIndex = NULL;
  ROOM_INDEX_DATA *room = NULL;
  OBJ_INDEX_DATA *item = NULL;
  if(bounty_vnum < 0)
  {
    if(IS_IMMORTAL(ch))
    {
      send_to_char("Sorry there seems to be a problem with the bounty.\n\r", ch);
      return;
    }
    return;
  }
  if(bounty_room >= 0)
    room = get_room_index(bounty_room);
  else
    return;
  if(bounty_item >= 0)
    item = get_obj_index(bounty_item);
  else if(bounty_type == BOUNTY_ITEM_NAME || bounty_type == BOUNTY_ITEM_DESC)
    return;
  pMobIndex = get_mob_index( bounty_vnum );
  if(!pMobIndex)
  {
    bug("No mob index found for selected bounty.", 0);
    return;
  }
  // Describe using %'s of the message number for which block of text
  if(teller)
    send_to_char("The man {gwhispers{x in your ear, '", ch);
  else
    send_to_char("You hear a {gwhisper{x in your ear, '", ch);
  if(bounty_timer < 0)
  {
    switch(bounty_desc % 9)
    {
      case 0: send_to_char("Our scouts are out hunting still.\n\r", ch); break;
      case 1: send_to_char("I have sent our scouts to find a target.\n\r", ch); break;
      case 2: send_to_char("The scouts we sent out have not returned.\n\r", ch); break;
      case 3: send_to_char("There is no word yet from the scouts.\n\r", ch); break;
      case 4: send_to_char("Our scouting parties are searching hard.\n\r", ch); break;
      case 5: send_to_char("I haven't heard back from the scouts yet.\n\r", ch); break;
      case 6: send_to_char("The scouts are still out.\n\r", ch); break;
      case 7: send_to_char("The scouting is not yet complete.\n\r", ch); break;
      case 8: send_to_char("I have not heard from the scouts yet.\n\r", ch); break;
    }
    switch(bounty_timer)
    {
      case -1: send_to_char("  The reports should be coming in anytime now.'\n\r", ch);
        break;
      case -2: send_to_char("  I expect I will get a report fairly soon.'\n\r", ch);
        break;
      case -3: send_to_char("  It's not been long enough to expect an answer.'\n\r", ch);
        break;
      case -4: send_to_char("  There may be a bit of a wait still, the scouts have not been gone long.'\n\r", ch);
        break;
      default: send_to_char("  They just left so it will be a little while.'\n\r", ch);
        break;
    }
    return;
  }

  // Timer for downgrading difficulty begins
  if(!IS_IMMORTAL(ch) && bounty_timer <= 0)
  {
    bounty_timer = 1;
    sprintf(buf, "%s has started the current bounty's timer.", ch->name);
    log_quest_detail(buf, QUEST_BOUNTY_KILL);
  }

  if(!IS_NPC(ch))
  {
    if(!ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM])
    {
      ch->pcdata->quest_wins[QUEST_BOUNTY_CLAIM]++;
      send_to_char("Your help is invaluable to us. We have many\n\r", ch);
      send_to_char("scouts exploring the world, and need you to aid us in eliminating the targets\n\r", ch);
      send_to_char("they find.  Of course, we will reward you for your effort.'\n\r", ch);
      if(teller)
        send_to_char("He pauses briefly then continues, '", ch);
      else// Shouldn't really be possible (Have to have 3 turn-ins to hear here)
        send_to_char("It pauses briefly then continues, '", ch);
    }
  }
  switch(bounty_desc % 5)
  {
    case 0: send_to_char("There is a target available for you to hunt.\n\r", ch);
      break;
    case 1: send_to_char("Our scouts have identified a target for you.\n\r", ch);
      break;
    case 2: send_to_char("We have identified a suspicious creature.\n\r", ch);
      break;
    case 3: send_to_char("Your timing is good as we have need of you.\n\r", ch);
      break;
    case 4: send_to_char("A reward has been set on a creature's head.\n\r", ch);
      break;
  }
  //23, 45, 78 are the break-timers for an hour, only using 23/78 at this point
  //30 is the 2/3rds break-timer for half an hour
  if(bounty_timer >= 30 && bounty_type >= BOUNTY_ITEM_NAME &&
    (bounty_downgrade || !IS_IMMORTAL(ch)))
  {// Reward lowered, extra tip added
    bounty_downgrade = TRUE;
    switch(bounty_desc % 3)
    {
      case 0: send_to_char("Since nobody could find it, our scout risked his life for a second look.\n\r", ch);
        break;
      case 1: send_to_char("Our scout has taken the dangerous task of finding out more about the creature.\n\r", ch);
        break;
      case 2: send_to_char("We have received more detailed information on the creature.\n\r", ch);
        break;
    }
    if(bounty_type == BOUNTY_MOB_DESC)
      sprintf(buf, "The creature is called {W%s{x\n\r", pMobIndex->short_descr);
    else if(bounty_type == BOUNTY_ITEM_DESC)
      sprintf(buf, "The item worn by the creature was seen: {W%s{x\n\r", item->short_descr);
    else// if(bounty_type == BOUNTY_ROOM_DESC)
      sprintf(buf, "The creature was spotted in {W%s{x\n\r", room->name);
    send_to_char(buf, ch);
  }
  else
  {
    switch(bounty_type)
    {
      default:
        sprintf(buf, "The creature is called {W%s{x\n\r", pMobIndex->short_descr);
        break;
      case BOUNTY_ROOM_NAME:
        sprintf(buf, "The creature was spotted in {W%s{x\n\r", room->name);
        break;
      case BOUNTY_ITEM_NAME:
        sprintf(buf, "Only an item was spotted on the creature: {W%s{x\n\r", item->short_descr);
        break;
      case BOUNTY_MOB_DESC:
        send_to_char("We didn't get a close enough look to identify it, but we have a {Wdescription{x:\n\r", ch);
        send_to_char(pMobIndex->description, ch);
        strcpy(buf, "\n\r");
        break;
      case BOUNTY_ROOM_DESC:
        send_to_char("Our scout was too nervous to get close, but described the {Wroom{x it was in:\n\r", ch);
        send_to_char(room->description, ch);
        strcpy(buf, "\n\r");
        break;
      case BOUNTY_ITEM_DESC:
        send_to_char("The creature was too dangerous to approach but we can describe an {Witem{x it had:\n\r", ch);
        send_to_char(item->description, ch);
        strcpy(buf, "\n\r");
        break;
    }
    send_to_char(buf, ch);
    if(bounty_timer >= 15 && !bounty_downgrade)
    {// Reward raised
      switch(bounty_desc % 4)
      {
        case 0: send_to_char("I have authorized additional rewards due to the challenge this poses.\n\r", ch);
          break;
        case 1: send_to_char("Since nobody has yet claimed the bounty I have increased its reward.\n\r", ch);
          break;
        case 2: send_to_char("If you kill it quickly you will get an additional reward.\n\r", ch);
          break;
        case 3: send_to_char("This bounty seems difficult so I will pay a larger reward for it.\n\r", ch);
          break;
      }
    }
  }
  if(bounty_timer >= 40)
    send_to_char("{DHurry{x! You are almost out of time on this bounty!'\n\r", ch);
  else
  {
    if(just_started)
    {
      if(IS_IMMORTAL(ch))
        send_to_char("No mortal has been told of this bounty yet, do not spoil it for them.'\n\r", ch);
      else
        send_to_char("Nobody has been told of this bounty before you, it {Gbegins{x now.'\n\r", ch);
    }
    else
    {
      switch(bounty_timer / 5)
      {
        case 0: send_to_char("The hunt has only just begun, hurry and you may still be first.'\n\r", ch);
          break;
        case 1: send_to_char("Find it and end its life, then bring the proof to me.'\n\r", ch);
          break;
        case 2: send_to_char("Kill it for us then you may claim your reward.'\n\r", ch);
          break;
        case 3: send_to_char("Bring proof that you have eliminated it and I will reward you.'\n\r", ch);
          break;
        case 4: send_to_char("Plenty of time remains for you to finish this bounty.'\n\r", ch);
          break;
        case 5: send_to_char("Eliminate it swiftly and you will be rewarded.'\n\r", ch);
          break;
        case 6: send_to_char("You will be suitably rewarded but you must kill it soon.'\n\r", ch);
          break;
        default: send_to_char("Hurry, the chance to kill it for a reward will not last forever.'\n\r", ch);
          break;
      }
    }
  }
  if(ch->level >= 59)
  {
    sprintf(buf, "\n\r'{DPssst{x. The mob's {Wvnum{x is {W%d{x and %d ticks remain. Don't tell anyone.'\n\r", pMobIndex->vnum, 45 - bounty_timer + 1);
    send_to_char(buf, ch);
  }
}

bool is_shaded(CHAR_DATA *shade)
{
  if(!shade->in_room)
    return FALSE;
  if(is_affected(shade, gsn_light_blast))
    return FALSE;
  return (IS_SET(shade->affected_by_ext, AFF_EXT_SHADED) &&
    IS_SET(shade->in_room->room_affects, RAFF_SHADED));
}

void shade_room(CHAR_DATA *shade)
{
  AFFECT_DATA *raf;
  for ( raf = shade->in_room->affected ; raf != NULL ; raf = raf->next )
  {
    if(raf->type == gsn_shaded_room)
    {
      if(raf->caster_id == shade->id)
      {
        if(raf->duration < 1)
          raf->duration = 1;
        return;// Renew the shading
      }
      break;
    }
  }
  if(!raf)
  {// Not shaded yet, display the startup message  
    act("Total {DDARKNESS{x explodes from $n and all light vanishes!\n\r",
      shade, NULL, NULL, TO_ROOM, FALSE);
    send_to_char("You fill the room with {DDARKNESS{x!\n\r", shade);
  }
  //Always apply the affect so if the other one dies it stays shaded properly
  AFFECT_DATA af;
  af.where = TO_AFFECTS;
  af.type = gsn_shaded_room;
  af.level = shade->level;
  af.location = 0;
  af.duration = 1;
  af.bitvector = RAFF_SHADED;
  af.caster_id = shade->id;
  affect_to_room( shade->in_room, &af );
}

void remove_shaded_room(CHAR_DATA *shade)
{
  AFFECT_DATA *raf, *raf_sec;
  for ( raf = shade->in_room->affected ; raf != NULL ; raf = raf->next )
  {
    if(raf->type == gsn_shaded_room && raf->caster_id == shade->id)
    {
	    if ( skill_table[raf->type].msg_off )
 		  {
 		    for(raf_sec = shade->in_room->affected; raf_sec != NULL; raf_sec = raf_sec->next)
 		    {
 		      if(raf_sec->type == gsn_shaded_room && raf_sec != raf)
 		        break;// A second source of it, don't display the end message
 		    }
 		    if(!raf_sec)
   	     act(skill_table[raf->type].msg_off, shade, NULL, NULL, TO_ALL, FALSE);
   	  }
      raffect_remove(shade->in_room, raf);
      break;
    }
  }
}

/* New bracer: Level 50, 4/4/4/6 +20 mana +1 con
New bracer: Level 50, 6/6/6/4 +35 hp +1 int +1 wis
New neck: Level 50, 2 hit 2 dam -1 str -6 AC*/
// Vnums: 8808-8899 !!! (Using these), or 20k-20027
// Not available for hooking up yet, used by the shade bounty NPC if attacked
bool spec_shade_major(CHAR_DATA *shade)
{
  int mob_targets = 0;
  CHAR_DATA *vch, *vch_next;
  if(!IS_SET(shade->affected_by_ext, AFF_EXT_SHADED))
    return FALSE;// No action from this AI if not shaded
  if(!shade->alert)
  {// Drop out of combat status if light blast is done
    if(is_affected(shade, gsn_light_blast))
      return TRUE;
    shade->hit = shade->max_hit;
    remove_shaded_room(shade);
    return FALSE;
  }
  if(is_shaded(shade))
  {
    // Automatically regens in the shadow
    shade->hit = UMIN(shade->hit + number_range(shade->level, shade->level * 2), shade->max_hit);
    shade_room(shade);// Renew it
  }
  else if(!is_affected(shade, gsn_light_blast))
    shade_room(shade);
  for(vch = shade->in_room->people; vch; vch = vch_next)
  {
    vch_next = vch->next;
    if(vch->fighting == shade)
    {
      if(IS_NPC(vch))
      {
        if(IS_AFFECTED(vch,AFF_CHARM) && (!vch->master
          || vch->master->pet != vch))
          mob_targets++;
      }
    }
  }
  if(mob_targets)
  {
    for(vch = shade->in_room->people; vch; vch = vch_next)
    {
      vch_next = vch->next;
      if(vch->fighting == shade)
      {
        if(IS_NPC(vch))
        {
          if(IS_AFFECTED(vch,AFF_CHARM) && (!vch->master
            || vch->master->pet != vch))
          {
            mob_targets--;
            if(mob_targets == 0)
              break;
          }
        }
      }
    }
    if(vch)// Whisper to this target
    {
      act("Sibilant {Dwhispers{x fill the room, focusing on $n.", vch, NULL, NULL, TO_ROOM, FALSE);
      if(IS_NPC(vch))
      {
        if(IS_SET(vch->form, FORM_UNDEAD) )
        {// Skellies are overwhelmed by the whispering
          OBJ_DATA *obj, *next_obj;
          act("$n twitches violently and crumbles into dust.",vch,NULL,NULL,TO_ROOM,FALSE);
          for (obj = vch->carrying; obj != NULL; obj = next_obj) 
          {
            next_obj = obj->next_content;
            obj_from_char (obj);
            obj_to_room (obj,vch->in_room);
          }
          stop_fighting (vch,FALSE);
          vch->pIndexData->killed++;
          kill_table[URANGE(0, vch->level, MAX_LEVEL-1)].killed++;
          extract_char( vch, TRUE );
        }
        else if(!saves_spell(shade->level, vch, DAM_NEUTRAL))
        {
          if(IS_AFFECTED(vch,AFF_CHARM) && vch->master)
          {
            if(vch->master->pet != vch)
            {// Break its charm and turn it against its master
              CHAR_DATA *owner = vch->master;
              act("$n looks around with hate filled eyes, then turns against YOU!", vch, NULL, owner, TO_VICT, FALSE);
              act("$n looks around with hate filled eyes, then turns against $N!", vch, NULL, owner, TO_NOTVICT, FALSE);
              stop_follower( vch );
              vch->fighting = owner;
            }
          }
          else// The whisper convinces it to stop fighting
            stop_fighting(vch, TRUE);
        }
      }
    }
  }
  return FALSE;
}

bool spec_shade_bounty(CHAR_DATA *shade)
{
  char buf[255];
  bool just_started = bounty_timer <= 0;
  CHAR_DATA *target;
  if(!IS_NPC(shade))
    return FALSE;
  if(shade->qnum2 > 0)
    shade->qnum2--;
  if(!shade->fighting && shade->qnum2 <= 0)
  {// Check if someone is sitting in the room
    ROOM_INDEX_DATA *sit = get_room_index(ROOM_VNUM_BOUNTY_WATCH);
    if(sit && sit->people)
    {
      for(target = sit->people; target; target = target->next_in_room)
      {
    		if(IS_NPC(target))
    			continue;
    		if(target->position != POS_SITTING)
    		  continue;
    		if(target->pcdata->quest_wins[QUEST_BOUNTY_CLAIM] >= 6 || IS_IMMORTAL(target))
    		{
          describe_mob_bounty(target, NULL, just_started);
          shade->qnum2 = 5;
        }
      }
    }
  }
  if(shade->fighting && !IS_SET(shade->affected_by_ext, AFF_EXT_SHADED))
  {// Transform
    CHAR_DATA *attacker = shade->fighting;
    if(IS_NPC(attacker) && attacker->master)
      attacker = attacker->master;
    sprintf(buf, "The bounty administrator is being engaged by %s.", attacker->name);
    wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
    shade->qnum = 0;
    shade->act |= ACT_AGGRESSIVE;
    SET_BIT(shade->affected_by_ext, AFF_EXT_SHADED);
    act("$n hisses, 'You dare!' and erupts into a creature of shadow!", shade, NULL, NULL, TO_ROOM, FALSE);
    clear_string(&shade->name, "large shade");
    clear_string(&shade->short_descr, "a large shade");
    clear_string(&shade->long_descr, "A large shade obscures the room in darkness.\n\r");
    clear_string(&shade->description, "Try as hard as you can, you can't make out any details among the shadows.\n\r");
    spec_shade_major(shade);
    return TRUE;
  }
  if(spec_shade_major(shade))
    return TRUE;
  else if(shade->alert)
    return FALSE;// Free it to do other combat-ey things
  if(IS_SET(shade->affected_by_ext, AFF_EXT_SHADED))
  {// Undo the transformation as well as removing all affects on the guy
    AFFECT_DATA *paf, *paf_next;
    shade->qnum = shade->qnum2 = 0;
    shade->hit = shade->max_hit;
    REMOVE_BIT(shade->act, ACT_AGGRESSIVE);
    REMOVE_BIT(shade->affected_by_ext, AFF_EXT_SHADED);
    clear_string(&shade->name, shade->pIndexData->player_name);
    clear_string(&shade->short_descr, shade->pIndexData->short_descr);
    clear_string(&shade->long_descr, shade->pIndexData->long_descr);
    clear_string(&shade->description, shade->pIndexData->description);
    act("$n reforms from the shadows.", shade, NULL, NULL, TO_ROOM, FALSE);
    for(paf = shade->affected; paf; paf = paf_next)
    {
      paf_next = paf->next;
      affect_remove(shade, paf, APPLY_BOTH);
    }
    for(paf = shade->flash_affected; paf; paf = paf_next)
    {
      paf_next = paf->next;
      flash_affect_remove(shade, paf, APPLY_BOTH);
    }
  }
// Check if someone is sitting in the room, randomly tell them about the bounty
	for(target = shade->in_room->people; target != NULL; target = target->next_in_room)
	{
		if(IS_NPC(target))
			continue;
		if(target->position != POS_SITTING)
		  continue;
		shade->qnum++;
		if(shade->qnum - 5 > number_percent())
		{// Tell anyone sitting -- wait 5 rounds before it can happen
		  for(; target; target = target->next_in_room)
		  {
		    if(!IS_NPC(target) && target->position == POS_SITTING)
    		  describe_mob_bounty(target, shade, just_started);
    	}
		}
		break;
	}
	if(!target)// It completed its check and told everyone or nobody was there
  	shade->qnum = 0;
  return FALSE;
}
 
