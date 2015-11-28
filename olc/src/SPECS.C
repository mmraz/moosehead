/* most of this you know, but I'll coment it anyways, for completness */

/* extra include for special.c, gota have it or spec_evil_adept wonl't fly */
#include "trap.h"

/* extra declares for special.c, one for each function */
DECLARE_SPEC_FUN(       spec_tower_shop         );
DECLARE_SPEC_FUN(       spec_evil_adept         );  
DECLARE_SPEC_FUN(       spec_follow_sludge      );
DECLARE_SPEC_FUN(       spec_follow_slugbelch   );
DECLARE_SPEC_FUN(       spec_follow_saxmundham  );
DECLARE_SPEC_FUN(       spec_telep_master       );
   

/* strea bits for the aray, i think this might be handled difrentlt on Moose
   but this is how it's done in rom 2.4 that i did it in */
    {   "spec_tower_shop",              spec_tower_shop         },
    {   "spec_evil_adept",              spec_evil_adept         },
    {   "spec_follow_saxmundham",       spec_follow_saxmundham  },
    {   "spec_follow_slugbelch",        spec_follow_slugbelch   },
    {   "spec_follow_sludge",           spec_follow_sludge      },
    {   "spec_telep_master",            spec_telep_master       },

/* special function for mob in ramona.are, mob vnum is 30504 */
bool spec_follow_saxmundham( CHAR_DATA *ch )
{
    do_follow( ch, "sludge" );
    do_follow( ch, "slugbelch" );
    return TRUE;
}

/* special function for mob in ramona.are, mob vnum is 30505 */
bool spec_follow_sludge( CHAR_DATA *ch )
{
    do_follow( ch, "slugbelch" );
    do_follow( ch, "saxmundham" );
    return TRUE;
}

/* special function for mob in ramona.are, mob vnum is 30506 */
bool spec_follow_slugbelch( CHAR_DATA *ch )
{
    do_follow( ch, "saxmundham" );
    do_follow( ch, "sludge" );
    return TRUE;
}

/* special function for mob in midgaard.are, mob vnum is 3014 */
bool spec_evil_adept( CHAR_DATA *ch )
{
    CHAR_DATA *victim;
    CHAR_DATA *v_next;
    int dam;
    int num;
    OBJ_DATA *corpse;

    if ( !IS_AWAKE(ch) )
	return FALSE;

    for ( victim = ch->in_room->people; victim != NULL; victim = v_next )
    {
	v_next = victim->next_in_room;
	if ( victim != ch && can_see( ch, victim ) && number_bits( 1 ) == 0 && !IS_NPC(victim))
	    break;
    }

    if ( victim == NULL )
	return FALSE;

   /* kinda hackish, but i want to do damage without starting a fight
    * so I haked up the damage function a *littel* bit :)
    */

 num = dice( 1, 4);
 if( num == 1 )
 {
    /* get a damage number to inflict */
    dam = dice( victim->level, 2 );
    

    if ( victim->position == POS_DEAD )
	return FALSE;

    /*
     * Stop up any residual loopholes.
     */
    if ( dam > 1200 )
    {  
	bug( "Damage: %d: more than 1200 points!", dam );
	dam = 1200;
    }
       
    /*
     * Inviso attacks ... not.
     */

    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
	affect_strip( ch, gsn_invis );
	affect_strip( ch, gsn_mass_invis );
	REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
	act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM );
    }

    /*
     * Damage modifiers.
     */

    if ( dam > 1 && !IS_NPC(victim) 
    &&   victim->pcdata->condition[COND_DRUNK]  > 10 )
	dam = 9 * dam / 10;

    if ( dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY) )
	dam /= 2;

    if ( dam > 1 && ((IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch) )
    ||		     (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch) )))
	dam -= dam / 4;

    trap_damage_mesage( ch, victim, dam );

    if (dam == 0)
	return FALSE;

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */
    victim->hit -= dam;
    if ( !IS_NPC(victim)
    &&   victim->level >= LEVEL_IMMORTAL
    &&   victim->hit < 1 )
	victim->hit = 1;
    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
	act( "$n is mortally wounded, and will die soon, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char( 
	    "You are mortally wounded, and will die soon, if not aided.\n\r",
	    victim );
	break;

    case POS_INCAP:
	act( "$n is incapacitated and will slowly die, if not aided.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char(
	    "You are incapacitated and will slowly die, if not aided.\n\r",
	    victim );
	break;

    case POS_STUNNED:
	act( "$n is stunned, but will probably recover.",
	    victim, NULL, NULL, TO_ROOM );
	send_to_char("You are stunned, but will probably recover.\n\r",
	    victim );
	break;

    case POS_DEAD:
	act( "$n is DEAD!!", victim, 0, 0, TO_ROOM );
	send_to_char( "You have been KILLED!!\n\r\n\r", victim );
	break;

    default:
	if ( dam > victim->max_hit / 4 )
	    send_to_char( "That really did HURT!\n\r", victim );
	if ( victim->hit < victim->max_hit / 4 )
	    send_to_char( "You sure are BLEEDING!\n\r", victim );
	break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
	stop_fighting( victim, FALSE );

    /*
     * Paydown for dieing.
     */
    if ( victim->position == POS_DEAD )
    {

        if ( !IS_NPC(victim) )
	{
	    sprintf( log_buf, "%s killed by %s at %d",
		victim->name,
		(IS_NPC(ch) ? ch->short_descr : ch->name),
		ch->in_room->vnum );
	    log_string( log_buf );

	    /*
	     * Dying penalty:
	     * 2/3 way back to previous level.
	     */
	    if ( victim->exp > exp_per_level(victim,victim->pcdata->points) 
			       * victim->level )
       	gain_exp( victim, (2 * (exp_per_level(victim,victim->pcdata->points)
			         * victim->level - victim->exp)/3) + 50 );
	}
	
        sprintf( log_buf, "%s got toasted by %s at %s [room %d]",
            (IS_NPC(victim) ? victim->short_descr : victim->name),
            (IS_NPC(ch) ? ch->short_descr : ch->name),
            ch->in_room->name, ch->in_room->vnum);
 
        if (IS_NPC(victim))
            wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
        else
            wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);

	raw_kill( victim );
        /* dump the flags */
        if (ch != victim && !IS_NPC(ch) && !is_same_clan(ch,victim))
        {
            if (IS_SET(victim->act,PLR_KILLER))
                REMOVE_BIT(victim->act,PLR_KILLER);
            else
                REMOVE_BIT(victim->act,PLR_THIEF);
        }

        /* RT new auto commands */

	if (!IS_NPC(ch)
	&&  (corpse = get_obj_list(ch,"corpse",ch->in_room->contents)) != NULL
	&&  corpse->item_type == ITEM_CORPSE_NPC && can_see_obj(ch,corpse))
	{
	    OBJ_DATA *coins;

	    corpse = get_obj_list( ch, "corpse", ch->in_room->contents ); 

	    if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
		 corpse && corpse->contains) /* exists and not empty */
		do_get( ch, "all corpse" );

 	    if (IS_SET(ch->act,PLR_AUTOGOLD) &&
	        corpse && corpse->contains  && /* exists and not empty */
		!IS_SET(ch->act,PLR_AUTOLOOT))
		if ((coins = get_obj_list(ch,"gcash",corpse->contains))
		     != NULL)
	      	    do_get(ch, "all.gcash corpse");
            
	    if ( IS_SET(ch->act, PLR_AUTOSAC) )
       	      if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains)
		return TRUE;  /* leave if corpse has treasure */
	      else
		do_sacrifice( ch, "corpse" );
	}

	return TRUE;
    }

    if ( victim == ch )
	return TRUE;

    tail_chain( );
 } 
 return TRUE;
}

/* special fun for mob in midgaard.are, mob vnum is 3013 */
bool spec_tower_shop(CHAR_DATA *ch )
{
    CHAR_DATA *victum;
    CHAR_DATA *v_next;
    OBJ_DATA  *obj;

    if( !IS_AWAKE(ch) )
        return FALSE;
    
    for( victum = ch->in_room->people; victum != NULL; victum = v_next )
    {
        v_next = victum->next_in_room;

        if ( victum == ch ) continue;

        if( ( obj = get_obj_carry( victum, "bigboobs") )== NULL )
	{
	   send_to_char("Bad eq dude, NULL that is :P\n\r", ch);
	}
	else
        {
           obj_from_char( obj );
           if( victum->fighting != NULL )
		stop_fighting( victum, TRUE );
	   act( "$n jet's to the tower.", victum, NULL, NULL, TO_ROOM );
           char_from_room( victum );
	   char_to_room( victum, get_room_index( 3039 ) );
           act( "$n poof's in.", victum, NULL, NULL, TO_ROOM );
 	   send_to_char( "Out to the tower you go!\n\r", victum );
	   do_look( victum, "auto" );
           do_echo( ch, "$n has entered the Tower of Observation!\n\r");
        }
    }
    return TRUE;
} 

/* special fuin for mob in sriraji.are, mob vnum is 18003 */
bool spec_telep_master(CHAR_DATA *ch )
{
    CHAR_DATA *victum;
    CHAR_DATA *v_next;
    OBJ_DATA  *obj;

    if( !IS_AWAKE(ch) )
        return FALSE;
    
    for( victum = ch->in_room->people; victum != NULL; victum = v_next )
    {
        v_next = victum->next_in_room;
        if (victum == ch ) continue;

        if( ( obj = get_obj_carry( victum, "scrawnyscrotum") )== NULL )
	{
	   send_to_char("Bad eq dude, NULL that is :P\n\r", ch);
	}
	else
        {
           obj_from_char( obj );
           if( victum->fighting != NULL )
		stop_fighting( victum, TRUE );
	   spell_teleport( skill_lookup( "teleport" ), 60, ch,
                 victum, TARGET_CHAR);
	   
        }
    }
    return TRUE;
} 



/* new .h file, called trap.h, douse damage to a pc without starting a fight */
extern
void    trap_damage_mesage  args( ( CHAR_DATA *ch, CHAR_DATA *victim,int dam ) );



/* trap.c, code for trap fucntion */
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "trap.h"

void trap_damage_mesage( CHAR_DATA *ch, CHAR_DATA *victim,int dam )
{
    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    char punct;

    if (ch == NULL || victim == NULL)
	return;

	 if ( dam ==   0 ) { vs = "miss";	vp = "misses";		}
    else if ( dam <=   4 ) { vs = "scratch";	vp = "scratches";	}
    else if ( dam <=   8 ) { vs = "graze";	vp = "grazes";		}
    else if ( dam <=  12 ) { vs = "hit";	vp = "hits";		}
    else if ( dam <=  16 ) { vs = "injure";	vp = "injures";		}
    else if ( dam <=  20 ) { vs = "wound";	vp = "wounds";		}
    else if ( dam <=  24 ) { vs = "maul";       vp = "mauls";		}
    else if ( dam <=  28 ) { vs = "decimate";	vp = "decimates";	}
    else if ( dam <=  32 ) { vs = "devastate";	vp = "devastates";	}
    else if ( dam <=  36 ) { vs = "maim";	vp = "maims";		}
    else if ( dam <=  40 ) { vs = "MUTILATE";	vp = "MUTILATES";	}
    else if ( dam <=  44 ) { vs = "DISEMBOWEL";	vp = "DISEMBOWELS";	}
    else if ( dam <=  48 ) { vs = "DISMEMBER";	vp = "DISMEMBERS";	}
    else if ( dam <=  52 ) { vs = "MASSACRE";	vp = "MASSACRES";	}
    else if ( dam <=  56 ) { vs = "MANGLE";	vp = "MANGLES";		}
    else if ( dam <=  60 ) { vs = "*** DEMOLISH ***";
			     vp = "*** DEMOLISHES ***";			}
    else if ( dam <=  75 ) { vs = "*** DEVASTATE ***";
			     vp = "*** DEVASTATES ***";			}
    else if ( dam <= 100)  { vs = "=== OBLITERATE ===";
			     vp = "=== OBLITERATES ===";		}
    else if ( dam <= 125)  { vs = ">>> ANNIHILATE <<<";
			     vp = ">>> ANNIHILATES <<<";		}
    else if ( dam <= 150)  { vs = "<<< ERADICATE >>>";
			     vp = "<<< ERADICATES >>>";			}
    else                   { vs = "do UNSPEAKABLE things to";
			     vp = "does UNSPEAKABLE things to";		}

    punct   = (dam <= 24) ? '.' : '!';

    sprintf( buf1, "A gigantic psychic blast %s $N%c", vp, punct );
    sprintf( buf2, "Your psychic blast %s $N%c", vp, punct );
    sprintf( buf3, "A gigantic psychic blast %s you%c", vp, punct );


    act( buf1, ch, NULL, victim, TO_NOTVICT );
    act( buf2, ch, NULL, victim, TO_CHAR );
    act( buf3, ch, NULL, victim, TO_VICT );

    return;
}



