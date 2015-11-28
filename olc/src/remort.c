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

static char rcsid[] = "$Id: remort.c,v 1.31 2004/03/29 23:57:16 ndagger Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"

DECLARE_DO_FUN(do_remort    );
DECLARE_DO_FUN(do_quit  );

bool out_of_element( CHAR_DATA *ch)
{ 
  bool DARK = FALSE, LIGHT = FALSE;
  if(IS_NPC(ch) ) return FALSE;

  if( time_info.hour > 6 && time_info.hour < 19 )
	LIGHT = TRUE;

  if( time_info.hour < 6 || time_info.hour > 19 )
	DARK = TRUE;

  if( LIGHT
    && IS_SET(ch->act,PLR_VAMP)
    && !IS_NPC(ch)
    && !IS_SET(ch->in_room->room_flags,ROOM_INDOORS) )
	return TRUE;

  if( IS_SET(ch->act,PLR_WERE) 
    && DARK
    && !IS_NPC(ch)
    && !IS_SET(ch->in_room->room_flags,ROOM_INDOORS)
    && IS_AFFECTED(ch,AFF_MORPH) )
	return TRUE;
  
  if( LIGHT
    && IS_SET(ch->act,PLR_MUMMY)
    && !IS_NPC(ch)
    && !IS_SET(ch->in_room->room_flags,ROOM_INDOORS) )
	return TRUE;

  if ( LIGHT 
    && !IS_NPC(ch)
    && ch->race == race_lookup("gargoyle")
    && !IS_SET(ch->in_room->room_flags, ROOM_INDOORS) )
	return TRUE;

   return FALSE;
}

void do_remort( CHAR_DATA *ch, char *argument)
{
	char arg[MAX_INPUT_LENGTH];
	int gn = 0, sn = 0, i=0;
	/* unused var?
	    char buf  [MAX_STRING_LENGTH];
	 */

  if(IS_NPC(ch))
  {
    send_to_char("Mobs can not remort.\n\r", ch);
    return;
  }

  if (ch->position != POS_STANDING)
  {
      send_to_char("I don't think so, you're too preoccupied to remort.\n\r",ch);
      return;
  }

  if( IS_SET(ch->act, PLR_WERE) 
      || IS_SET(ch->act, PLR_VAMP)
      || IS_SET(ch->act,PLR_MUMMY))
  {
	send_to_char("Nope, can't do it twice.\n\r",ch);
	return;
  }

  if(IS_SET(ch->mhs,MHS_SHAPESHIFTED) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
  {
     send_to_char("You are not allowed to remort while shapeshifted.\n\r",ch);
     return;
  }

  if( IS_IMMORTAL(ch) )
  {
	send_to_char("What is ya some kinda dumb bunny?\n\r",ch);
	return;
  }

  one_argument(argument,arg);
  if( arg[0] == '\0')
  {
	send_to_char("You must supply a remort class!\n\r",ch);
	return;
  }

  if( !str_cmp(arg,"garou") 
    || !str_cmp(arg,"nosferatu") 
    || !str_cmp(arg,"mummy"))
  {
    /* strip them down and prepare to be remorted */
    while ( ch->flash_affected )
	flash_affect_remove( ch, ch->flash_affected,APPLY_BOTH);
    while ( ch->affected )
	affect_remove( ch, ch->affected,APPLY_BOTH);
    ch->affected_by = 0;


/*    if ( ch->train >= 1 )
	{
	ch->pcdata->perm_hit += 10 * ch->train;
	ch->train = 0;
	}*/
  /* Halve trains instead of putting them into hp */
  ch->pcdata->half_train = ch->train;
  ch->train = 0;
  ch->pcdata->half_retrain = ch->pcdata->retrain;
  ch->pcdata->retrain = 0;
  ch->pcdata->trained_hit /= 2;
  ch->pcdata->trained_mana /= 2;
  ch->pcdata->trained_move /= 2;

		/* Remove buffy kit  and special groups*/
			if ( ch->kit== kit_lookup("buffy") 
                           || ch->kit== kit_lookup("archeologist") 
                           || ch->kit== kit_lookup("lycanthrope hunter")
			   || ch->kit== kit_lookup("vampyre hunter")
			   )
			{
			   for ( i = 0 ; i < 5 ; i++ )
				 if ( kit_table[ch->kit].skills[i] == NULL )
				   break;
				 else
				 group_remove(ch,kit_table[ch->kit].skills[i]);

				 ch->kit = 0;
													 }


    /* Remove the starting hp,mana,move then divide the remaining
       by 2 then add the starting hp,mana,move back on 
     */
    ch->pcdata->perm_hit  -= 20;
    ch->pcdata->perm_mana -= 100;
    ch->pcdata->perm_move -= 100;

    ch->pcdata->perm_hit  = ch->pcdata->perm_hit / 2;
    ch->pcdata->perm_mana = ch->pcdata->perm_mana / 2; 
    ch->pcdata->perm_move = ch->pcdata->perm_move / 2;

    ch->pcdata->perm_hit  += 20;
    ch->pcdata->perm_mana += 100;
    ch->pcdata->perm_move += 100;
    
    ch->level = 25;

    /* Hours code has no affect, no need to cut their hours 
    ch->redid = ch->played;
    ch->played = ch->pcdata->last_level * 1800;
    */

    ch->pcdata->switched = ch->played;
    ch->exp = (25 * (exp_per_level(ch,ch->pcdata->points)));
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {  
	if( (skill_table[sn].name != NULL) && (ch->pcdata->learned[sn] >= 75) )
	ch->pcdata->learned[sn] = 75;
    }  
  } else {
    send_to_char("You must pick a valid remort, mummy, garou or nosferatu.\n\r",ch);
    return;
  }



    if( !str_cmp(arg,"garou") )
    {
	gn = group_lookup("garou");
	gn_add(ch,gn);
	SET_BIT(ch->act,PLR_WERE);
	send_to_char("You are filled with the rage of the wolf.\n\r",ch);
	send_to_char("\n\rPlease log back in with your new soul.\n\r\n\r",ch);
	do_quit(ch,"");
	return;
    }

    if( !str_cmp(arg,"nosferatu") )
    {	
	gn = group_lookup("nosferatu");
	gn_add(ch,gn);
	SET_BIT(ch->act,PLR_VAMP);
	send_to_char("Your blood courses with an ancient lust.\n\r",ch);
	send_to_char("\n\rPlease log back in with your new soul.\n\r\n\r",ch);
	do_quit(ch,"");
	return;
    }

    if( !str_cmp(arg,"mummy") )
    {	
	gn = group_lookup("mummy");
	gn_add(ch,gn);
	SET_BIT(ch->act,PLR_MUMMY);
	send_to_char("You are wrapped in the rags of the mummy.\n\r",ch);
	send_to_char("\n\rPlease log back in with your new soul.\n\r\n\r",ch);
	do_quit(ch,"");
	return;
    }
  return;
}
