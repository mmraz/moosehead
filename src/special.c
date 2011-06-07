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
#include "magic.h"
#include "lookup.h"

void	check_equip	args( (CHAR_DATA *ch) );
bool	check_wear	args( (CHAR_DATA *ch, OBJ_DATA *obj) );
void	one_hit		args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
int     count_fight_size args( (CHAR_DATA *ch) );
 
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

/* the function table */
const   struct  spec_type    spec_table[] =
{
    {	"spec_breath_any",		spec_breath_any		},
    {	"spec_breath_acid",		spec_breath_acid	},
    {	"spec_breath_fire",		spec_breath_fire	},
    {	"spec_breath_frost",		spec_breath_frost	},
    {	"spec_breath_gas",		spec_breath_gas		},
    {	"spec_breath_lightning",	spec_breath_lightning	},	
    {	"spec_cast_adept",		spec_cast_adept		},
    {	"spec_cast_cleric",		spec_cast_cleric	},
    {	"spec_cast_judge",		spec_cast_judge		},
    {	"spec_cast_mage",		spec_cast_mage		},
    {	"spec_cast_undead",		spec_cast_undead	},
    {	"spec_executioner",		spec_executioner	},
    {	"spec_fido",			spec_fido		},
    {	"spec_guard_l",			spec_guard_l		},
    {	"spec_guard_d",			spec_guard_d		},
    {   "spec_honor_guard",             spec_honor_guard        },
    {   "spec_demise_guard",            spec_demise_guard       },
    {   "spec_posse_guard",             spec_posse_guard        },
    {   "spec_zealot_guard",            spec_zealot_guard       },
    {   "spec_warlock_guard",           spec_warlock_guard      },
    {	"spec_janitor",			spec_janitor		},
    {	"spec_mayor",			spec_mayor		},
    {	"spec_poison",			spec_poison		},
    {	"spec_thief",			spec_thief		},
    {	"spec_puff",			spec_puff		},
    {   "spec_phoenix",			spec_phoenix		},
    {	"spec_nasty",			spec_nasty		},
    {	"spec_troll_member",		spec_troll_member	},
    {	"spec_ogre_member",		spec_ogre_member	},
    {	"spec_patrolman",		spec_patrolman		},
    {	"spec_rabbit",			spec_rabbit		},
    {	"spec_cast_dispel",		spec_cast_dispel	},
    {	"spec_monk",			spec_monk		},
    {   "spec_altirin_undead",		spec_altirin_undead	},
    {	"spec_vhan",			spec_vhan		},
    {   "spec_drachlan_melee",		spec_drachlan_melee	},
    {   "spec_drachlan_medic",		spec_drachlan_medic	},
    {   "spec_average",			spec_average		},
    {   "spec_salesman",		spec_salesman		},
    {   "spec_elemental_king",		spec_elemental_king	},
    {   "spec_elemental_fire",		spec_elemental_fire	},
    {   "spec_elemental_water",		spec_elemental_water	},
    {   "spec_elemental_water_king",		spec_elemental_water_king	},
    {   "spec_jabber",                  spec_jabber             },
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
             || victim->clan != clan_lookup("honor")
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

                if ( clanmate->clan == clan_lookup("honor") && !IS_IMMORTAL(clanmate) ) 
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
             || victim->clan != clan_lookup("demise")
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

                if ( clanmate->clan == clan_lookup("demise") && !IS_IMMORTAL(clanmate) ) 
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
             || victim->clan != clan_lookup("posse")
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

                if ( clanmate->clan == clan_lookup("posse") && !IS_IMMORTAL(clanmate) ) 
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
             || victim->clan != clan_lookup("zealot")
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

                if ( clanmate->clan == clan_lookup("zealot") && !IS_IMMORTAL(clanmate) ) 
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
             || victim->clan != clan_lookup("warlock")
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

                if ( clanmate->clan == clan_lookup("warlock") && !IS_IMMORTAL(clanmate) ) 
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
      case 2: do_say(ch,ch->pIndexData->spec_words[2]); return TRUE;
      case 10: do_say(ch,ch->pIndexData->spec_words[0]); return TRUE;
      case 18: do_say(ch,ch->pIndexData->spec_words[1]); return TRUE;
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

    clanwinner = clan_lookup("valor");

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
        act( " ...but joy! Yet another novel phenomenon seizes your attention.",
		 ch, NULL, NULL, TO_CHAR,FALSE);
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
    	   victim->leader = vhan;
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
 
