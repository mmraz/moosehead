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
 *  around, comes around.  Right?                                          *
 ***************************************************************************/

static char rcsid[] = "$Id: deity.c,v 1.59 2003/09/28 01:58:37 ndagger Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "recycle.h"
#include "lookup.h"

DECLARE_DO_FUN(do_recall  );
DECLARE_DO_FUN(do_rescue  );
DECLARE_DO_FUN(do_look  );

#ifdef DEITY_TRIAL_DEBUG_CODE
int deity_value_override = 0;
int deity_msg_override = 0;
#endif

bool recall	args( ( CHAR_DATA *ch, char *argument, bool fPray ) );

void do_pledg( CHAR_DATA *ch, char *argument )
{
   send_to_char("You must type the full command to change your pledge.\n\r",ch);
   return;
}

void do_pledge( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int deity;

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
	if(ch->pcdata->deity_timer > 0 ) 
	{
          sprintf(buf, "You will change deities in %d ticks.", ch->pcdata->deity_timer);
          send_to_char(buf, ch);
        }
        else
          send_to_char( "Syntax: pledge <deity>\n\r",ch);
        
        return;
    }
/* ADDED by Nightdagger on 04/13/03 */

   if ( deity_lookup(arg1) == deity_lookup("almighty") )
   {
      send_to_char("The Almighty chooses you...you do not choose the Almighty.\n\r",ch);
      return;
   }


    /*
    if ( ch->pcdata->deity > 0 && !IS_IMMORTAL(ch) )
    {
	sprintf(buf,"You already worship %s.\n\r",
		deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }
     */

    if ( (deity = deity_lookup(arg1)) == -1)
    {
	send_to_char("You must pick an existing deity.  See 'help deity' for"
			" choices.\n\r",ch);
	return;
    }

    if ( !is_clan(ch) && deity_table[deity].clan)
    {
	sprintf(buf,"%s can only be worshiped by those in the clan system.\n\r",
		deity_table[deity].pname);
	send_to_char(buf,ch);
	return;
    }

    if ( ((ch->played + (int) (current_time - ch->logon)) 
	  - ch->pcdata->switched) 
		/ 3600 < 1 && !IS_IMMORTAL(ch))
    {
        sprintf(buf,"%s doubts the tenacity of your pledge.\n\r",
                deity_table[deity].pname);
        return;
    }

 /*  REMOVED by Nightdagger on 04/13/03

   if(ch->pcdata->deity != 0)
       ch->pcdata->switched = ch->played;
    ch->pcdata->new_deity = deity;
    ch->pcdata->deity_timer = 90;
    ch->pcdata->sac = 0;
    sprintf(buf,"You will be  a follower of %s in %d ticks..\n\r",deity_table[deity].pname, ch->pcdata->deity_timer);
    send_to_char(buf,ch);
    return;
*/

   sprintf(buf,"You are now a follower of %s.\n\r",deity_table[deity].pname);
   send_to_char(buf,ch);
   if(deity != ch->pcdata->deity && ch->pcdata->deity_favor_timer > 0)
   {
	ch->pcdata->deity_favor_timer = 0;
	do_deity_msg("%s's gaze turns away from you.", ch);	
	if(ch->pcdata->deity_trial_timer > 0)
	{
		ch->pcdata->deity_trial_timer = 0;
		do_deity_msg("You have failed to complete the trial from %s", ch);	
	}
   }

   ch->pcdata->deity = deity;
   if ( ch->clan == clan_lookup("zealot") && ch->pcdata->deity != deity_lookup("almighty") )
   {
     send_to_char("Unbeliever!  Taste the Wrath of the Almighty!\n\r",ch);
     ch->clan = clan_lookup("outcast");
     ch->pcdata->learned[skill_lookup("annointment")] = 0;
     ch->pcdata->rank = 0;
     ch->pcdata->outcT = 2000; 
     ch->pcdata->node = 0;
     if ( IS_SET(ch->pcdata->clan_flags, CLAN_ALLOW_SANC) )
         REMOVE_BIT(ch->pcdata->clan_flags,  CLAN_ALLOW_SANC);
     char_from_room(ch);
     char_to_room(ch,get_room_index(ROOM_VNUM_MATOOK));
     clear_mount(ch);
     do_look(ch, "auto");
   }
   return;



}

bool has_gift( CHAR_DATA *ch, int gift)
{
  int gn;
  bool found = FALSE;

  if( gift == MAX_GIFTS)
	return found;

  for(gn = 0; gn < MAX_GIFT; gn++)
  {
    if(deity_table[ch->pcdata->deity].gifts[gn] == NULL)
	return found;
    if(!str_cmp(gift_table[gift].name,deity_table[ch->pcdata->deity].gifts[gn]))
	found = TRUE;
  }

  return found;
}

bool is_aligned( CHAR_DATA *ch )
{
  bool matches = FALSE;

  switch(deity_table[ch->pcdata->deity].align)
  {
   case ALIGN_NONE: matches = TRUE; break;
   case ALIGN_GOOD:
	if(IS_GOOD(ch)) matches = TRUE; break;
   case ALIGN_NEUTRAL:
	if(IS_NEUTRAL(ch)) matches = TRUE; break;
   case ALIGN_EVIL:
	if(IS_EVIL(ch)) matches = TRUE; break;
   default: return matches;
   }

   return matches;
}

void do_pray( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d; 
    int gift;
    int giftcost;

    if(IS_NPC(ch))
      return;

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        sprintf(buf, "%s appreciates your prayer for nothing.\n\r",
		deity_table[ch->pcdata->deity].pname);
        send_to_char( buf ,ch);
        return;
    }

/*  REMOVED by Nightdagger on 5/8/03
  if(ch->pcdata->deity_timer > 0 )
	{
	sprintf(buf, "%s is mightily upset you've started worshipping %s", deity_table[ch->pcdata->deity].pname, 
			deity_table[ch->pcdata->new_deity].pname);
        send_to_char(buf, ch);
        return;
        }
*/
#ifdef DEITY_TRIAL_DEBUG_CODE
		if(is_number(arg) && IS_IMMORTAL(ch) && ch->level == 60)
		{// Minimize the damage if this ever goes live accidentally
			int val = atoi(arg);
			if(val < 0)
			{
				deity_msg_override = abs(val);// 0 is no override
				send_to_char("Deity message override set.\n\r", ch);			
			}
			else if(val > 0)
			{
				deity_value_override = val;
				if(deity_value_override > 100)
				{
					deity_value_override = 100;
					send_to_char("Max deity value override is 100.\n\r", ch);
				}
				send_to_char("Deity value override set.\n\r", ch);			
			}
			else
			{
				deity_msg_override = 0;
				deity_value_override = 0;
				send_to_char("Deity value and message overrides cleared.\n\r", ch);
			}
			return;
		}
#endif

	if(!str_cmp(arg, "endgaze"))
	{// Must type this whole word out 
		if(ch->pcdata->deity_favor_timer <= 0)
		{
			send_to_char("No deity is gazing upon you at this time.\n\r", ch);
		}
		else
		{
			if(ch->pcdata->deity_trial_timer > 0)
			{
				do_deity_msg("You abandon the trial given to you by %s", ch);
				ch->pcdata->deity_trial_timer = 0;
				log_deity_favor(ch, NULL, DEITY_TRIAL_FAIL_ABANDON);
			}
			ch->pcdata->deity_favor_timer = 0;
			do_deity_msg("%s's gaze turns away from you.", ch);
		}
		return;
	}
	
	if(!str_prefix(arg, "abandon"))
	{
		if(ch->pcdata->deity_trial_timer > 0)
		{
			do_deity_msg("You abandon the trial given to you by %s", ch);
			ch->pcdata->deity_trial_timer = 0;
			log_deity_favor(ch, NULL, DEITY_TRIAL_FAIL_ABANDON);
		}
		else
		{
			send_to_char("You have no trial to abandon.\n\r", ch);
		}
		return;
	}

  if(is_affected(ch,skill_lookup("holy silence")) )
    {
	sprintf(buf, "%s does not hear you.\n\r", deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }

    if ( !is_clan(ch) && deity_table[ch->pcdata->deity].clan)
    {
	sprintf(buf,"%s can only be worshiped by those in the clan system.\n\r",
		deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }

  if(!str_prefix(arg, "intervene") || !str_prefix(arg, "intervention"))
  {
    CHAR_DATA *victim;
    // Find the target
      argument = one_argument( argument, arg );
      if(arg[0] == '\0')
      {
        send_to_char("Usage: pray intervene <character>\n\r", ch);
        return;
      }
      if(str_cmp(deity_table[ch->pcdata->deity].pname, "matook"))
      {
        sprintf(buf,
        "%s can not {Wintervene{x for you.\n\r",
        deity_table[ch->pcdata->deity].pname);
        send_to_char(buf,ch);
        return;
      }
      if(ch->pcdata->sac < 300)
      {
        sprintf(buf,
        "You haven't given enough homage to %s to be granted that gift.\n\r",
        deity_table[ch->pcdata->deity].pname);
        send_to_char(buf,ch);
        return;
      }
      victim = get_char_world( ch, arg );
      if(victim != NULL && !IS_NPC(victim) && !IS_IMMORTAL(victim) && !is_clan(victim) && (victim->position == POS_INCAP || victim->position == POS_MORTAL))
      {
        CHAR_DATA *in_room;
        bool calmed = FALSE;
	// There is a non-clan, non-immortal incapacitated player being targeted
        sprintf(buf, "You feel spiritually {bdrained{x as Matook {Wintervenes{x for %s.\n\r", victim->name);
        send_to_char(buf,ch);
        ch->pcdata->sac -= 300;
        victim->hit = ch->level;
        if(victim->hit > victim->max_hit)
          victim->hit = victim->max_hit;
        update_pos(victim);
	// Prevent a followup kill from aggressive enemies - 2 tick calm
        for (in_room = victim->in_room->people; in_room != NULL; in_room = in_room->next_in_room)
        {
          // Don't calm enemies that are fighting someone else
          if(IS_NPC(in_room) && IS_SET(in_room->act, ACT_AGGRESSIVE) && in_room->position != POS_FIGHTING) 
          {
            AFFECT_DATA af;
            af.where = TO_AFFECTS;
            af.type = TAR_IGNORE;
            af.level = 60;
            af.duration = 2;
            af.location = APPLY_NONE;
            af.modifier = 0;
            af.bitvector = AFF_CALM;
            affect_to_char(in_room,&af);

            calmed = TRUE;
          }
        }

        do_look(victim,"auto");
        send_to_char("\n\rYou feel a warm glow as Matook intervenes for you.\n\r", victim);
        if(calmed)
          send_to_char("Matook calms your foes so that you may return to safety.\n\r", victim);
        return;
      }
      // No valid victim found
      send_to_char("Nobody by that name needs intervention.",ch);
      return;
    }


    if( !str_prefix(arg,"immortal"))
    {
	if (IS_SET(ch->comm,COMM_NOCHANNELS))
	{
	 send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
	 return;
	}

    send_to_char("The immortals hear your prayer.\n\r",ch);

    if ( ch->pcdata->sac < 2 )
    {
	sprintf(buf,
	"You haven't given enough homage to %s to be granted that gift.\n\r",
	deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }
    else
    {
	ch->pcdata->sac -= 2;
    }

    for ( d = descriptor_list; d != NULL; d = d->next )                         
    {                                                                           
	if ( (d->connected == CON_PLAYING) &&
	     (IS_IMMORTAL(d->character)) &&
	     (!IS_SET(d->character->comm,COMM_NOWIZ)) &&
	     (!IS_SET(d->character->comm,COMM_QUIET))  &&
	     (d->character->ignoring != ch) )
	{
        channel_vis_status(ch,d->character);
	if(IS_SET(d->character->display,DISP_COLOR))
	    send_to_char(BOLD,d->character);
      act_new("$n prays '$t'",ch,argument,d->character,TO_VICT,POS_DEAD,FALSE);
	if(IS_SET(d->character->display,DISP_COLOR))
	    send_to_char(NORMAL,d->character);
	}
    }   	

    return;
    }

    if(ch->clan==clan_lookup("outcast") && ( ch->pcdata->outcT > 0))
    {
        send_to_char("Nothing happened.\n\r",ch);
        return;
    }

    gift = gift_lookup(arg);

    if ( !has_gift(ch,gift) )
    {
     sprintf(buf,
	     "%s can not grant you %s.\n\r",deity_table[ch->pcdata->deity].pname,
	     arg);
     send_to_char(buf,ch);
     return;
    }

    if ( !is_aligned(ch))
    {
	sprintf(buf,"%s frowns upon those of your alignment.\n\r",
		deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }

    if ( ((ch->played + (int) (current_time - ch->logon))
	   - ch->pcdata->switched) / 3600 < 1 && !IS_IMMORTAL(ch))
    {
	sprintf(buf,"%s is weary of your faith.\n\r",
		deity_table[ch->pcdata->deity].pname);
	return;
    }

    if ( ch->pcdata->sac < gift_table[gift].cost )
    {
	   sprintf(buf,
	"You haven't given enough homage to %s to be granted that gift.\n\r",
	   deity_table[ch->pcdata->deity].pname);
	   send_to_char(buf,ch);
	   return;
    }

/*
    if ( (!str_prefix(arg,"knowledge")) && (ch->clan==clan_lookup("zealot")) )
       ch->pcdata->sac -= 10;
    else */
       ch->pcdata->sac -= gift_table[gift].cost;


    if(ch->daze > 0 && number_percent() < 33)
      send_to_char("You lost your concentration.\n\r",ch);
    else
    {
      sprintf(buf,
        "%s grants your request for %s.\n\r",
        deity_table[ch->pcdata->deity].pname, gift_table[gift].name);
        send_to_char(buf, ch);
      give_gift(ch,gift);
    }
    return;
}

void give_gift(CHAR_DATA *ch,int gift)
{
  char name[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  CHAR_DATA *gch,*nextvictim;
  AFFECT_DATA *paf,*paf_next;
  AFFECT_DATA af;
  ROOM_INDEX_DATA *room,*was_in,*now_in;
  int gain,attempt,door;
  EXIT_DATA *pexit;
       DESCRIPTOR_DATA *d;
       char buf2[MAX_STRING_LENGTH];
       char buf[MAX_STRING_LENGTH];
       bool first = TRUE;

  strcpy(name,gift_table[gift].name);

  switch ( name[0] )
  {
  case 'r': /* 3 find out which one */
	if( !str_prefix(name,"recall"))
	{
	 recall( ch, "", TRUE );
	 break;
	}

	if( !str_prefix(name,"reanimation"))
	{
	 reanimation(ch);
	 break;
	}

	if( !str_prefix(name,"random"))
	{
	 gift = number_range(2,MAX_GIFTS - 1);
	 give_gift(ch,gift);
	 break;
	}
	break;

  case 'k': /*knowledge*/
       

       for ( d = descriptor_list ; d != NULL; d = d->next)
	  {
	  if (d->character != NULL )
	    {
	    if (d->character->clan
		&& d->character->clan != clan_lookup("matook") 
		&& d->character->clan != clan_lookup("newbie") 
		&& !IS_IMMORTAL(d->character) )
            {
	      if (first)
	      {
		if (d->character->pcdata->deity != deity_lookup("almighty"))
		sprintf(buf, RED"%s is not a follower of the Almighty."NORMAL"\n\r", 
			d->character->name); 
                else
		sprintf(buf, "%s is a follower of the Almighty.\n\r", 
			d->character->name); 
		first = FALSE;
	      }
	      else
	      {
		if (d->character->pcdata->deity != deity_lookup("almighty"))
		sprintf(buf2, RED"%s is not a follower of the Almighty."NORMAL"\n\r", 
			d->character->name); 
                else
		sprintf(buf2, "%s is a follower of the Almighty.\n\r", 
			d->character->name); 
	      strcat(buf, buf2);
              }
	    }
	  }
	}
        send_to_char(buf, ch);
	break;


  case 'p': /* patience */

	for ( paf = ch->affected; paf != NULL; paf = paf_next )
	{
	 paf_next  = paf->next;
	 if ( paf->duration > 0 )
	 {
	  paf->duration++;
	  if (number_range(0,4) == 0 && paf->level > 0)
		paf->level++;
	 }
        }
	break;

  case 'n': /* nurture */
	gain_condition(ch, COND_HUNGER, 20 );
	gain_condition(ch, COND_THIRST, 20 );
	break;

  case 'm': /* meld */
	room = get_room_index(ch->in_room->vnum);
	if((gain = room->sector_type) > SECT_CITY)
	gain *= 4;
	ch->mana = UMIN(ch->max_mana,ch->mana + gain);
	ch->hit = UMIN(ch->max_hit,ch->hit + room->sector_type);
	break;

  case 'o': /* opiate */
	if(ch->pcdata->condition[COND_DRUNK] <= 15)
        {
	ch->pcdata->condition[COND_DRUNK] =
	UMIN(48,ch->pcdata->condition[COND_DRUNK] + 15);
	DAZE_STATE(ch, 5 * PULSE_VIOLENCE);
         af.where     = TO_AFFECTS;
         af.type      = skill_lookup("opiate");
         af.level   = ch->level;
         af.duration  = 2;
         af.location  = APPLY_INT;
         af.modifier  = 1 + (ch->level < 26 ? 1 : 0);
         af.bitvector = 0;
         affect_to_char( ch, &af );
         }
	 else
	 send_to_char("You're already buzzed.\n\r",ch);
         break;

  case 'b': /* bravery */
	if ( !str_prefix(name, "bravery") )
	  for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	  {
	   if ( gch != ch && is_same_group( gch, ch ) && gch->fighting != NULL )
	    {
	     strcpy(arg,gch->name);
	     strcat(arg," xvx2");
	     do_rescue(ch,arg);
	    }
	   }

         if ( !str_prefix(name, "banishment"))
	   {
           for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	     {
	     if(!IS_NPC(gch))
	       if ( !IS_SET (gch->mhs, MHS_BANISH) && 
		  gch->pcdata->deity != deity_lookup("almighty") &&
		  number_percent() < ch->clan == clan_lookup("zealot") ?
		  40 : 5 )
                  {
		    SET_BIT(gch->mhs, MHS_BANISH);
		  }
             }
	   }
	break;

  case 's': /* 2 find out which one */
	if( !str_prefix(name,"speed") )
	{
	 if(!IS_SET(ch->res_flags,RES_DELAY) && !is_affected(ch,skill_lookup("arcantic alacrity")) )
	 {
	  SET_BIT(ch->res_flags,RES_DELAY);
	 }
	break;
	}

	if( !str_prefix(name,"stature") 
	  && ch->size == pc_race_table[ch->race].size)
	{
	 af.where     = TO_AFFECTS;
	 af.type      = -1;
	 af.level   = ch->level;
	 af.duration  = (ch->level / 4)+1;
	 af.location  = APPLY_SIZE;
	 af.modifier  = 1;
	 af.bitvector = 0;
	 affect_to_char( ch, &af );
	 break;
	}
	break;

  case 'd': /* distraction */
	for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
	{
	 if( gch != ch && (IS_NPC(gch) || is_clan(gch)) 
	     && !IS_SET(ch->vuln_flags,VULN_DISTRACTION) )
	 {
	  af.where     = TO_VULN;
          af.type      = -1;
          af.level   = ch->level;
          af.duration  = 1;
          af.location  = APPLY_NONE;
          af.modifier  = 0;
          af.bitvector = VULN_DISTRACTION;
          affect_to_char( gch, &af );
	 }
	}
	break;

  case 'f': /* fear */
      for ( gch= ch->in_room->people; gch != NULL; gch = nextvictim )
      {
	 nextvictim = gch->next_in_room;
         if(gch != ch  && 
	      ( (is_clan(gch) && is_clan(ch) ) || IS_NPC(gch) ) )
         {
            if (IS_NPC(gch) && (IS_SET(gch->act,ACT_TRAIN)
                ||  IS_SET(gch->act,ACT_PRACTICE)
                ||  IS_SET(gch->act,ACT_IS_HEALER)
                ||  IS_SET(gch->act,ACT_NOPURGE)
                ||  IS_SET(gch->act,ACT_IS_CHANGER)))  
	       continue;

            if ( saves_spell(ch->level,gch,DAM_MENTAL))
	       continue;

            if (is_same_group(ch,gch))
	       continue;

            for ( attempt = 0; attempt < 6; attempt++ )
            {
	       door = number_door();
               was_in = gch->in_room;
	       if ( ( pexit = was_in->exit[door] ) == 0
		 ||   pexit->u1.to_room == NULL
		 ||   IS_SET(pexit->exit_info, EX_CLOSED)
		 ||   number_range(0,gch->daze) != 0
		 ||   (IS_NPC(gch)
		 &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
                 continue;

               move_char( gch, door, FALSE );
               if ( ( now_in = gch->in_room ) == was_in )
                  continue;

               gch->in_room = was_in;
               act( "$n runs in fear!", gch, NULL, NULL, TO_ROOM ,FALSE);
               gch->in_room = now_in;
               send_to_char("You run in fear.\n\r",gch);
            }
         }
      }
      break;
    
  case 't': /* transport */
	if( !( (IS_AFFECTED(ch,AFF_CURSE) || (IS_AFFECTED(ch,AFF_CURSE))) && number_percent() < 50) 
	    && !IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL) 
	    && !(ch->fighting != NULL && number_percent() < 75) )
	{
 	 room = get_random_room(ch);
	 send_to_char("You have been transported!\n\r",ch);
	 act( "$n vanishes!", ch, NULL, NULL, TO_ROOM ,FALSE);
	 if (ch->fighting != NULL) stop_fighting(ch,FALSE);
	 char_from_room( ch );
	 char_to_room( ch, room );
	 clear_mount(ch);
	 act( "$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM ,FALSE);
	 do_look( ch, "auto" );
	}
	break;

  default: return;
  }
  return;
}

void reanimation(CHAR_DATA *ch)
{
  OBJ_DATA *obj,*cobj,*cobj_next;
  ROOM_INDEX_DATA *aroom= NULL;

  for ( obj = object_list; obj != NULL; obj = obj->next )
  {
    if(obj->pIndexData->vnum != OBJ_VNUM_CORPSE_PC)
	continue;

    
    if(!str_cmp(ch->name,obj->owner) && obj->timer > 0 )
	{
	 if( obj->carried_by != NULL )
	 {
	   aroom= obj->carried_by->in_room;
	 }
	 if( obj->in_room != NULL )
	 {
	   aroom= obj->in_room;
	 }
	 if( aroom != NULL && aroom->clan)
	 {
	   send_to_char("You can't reanimate into a clan hall.\n\r",ch);
	   return  ;
	 }

	 char_from_room(ch);
	 char_to_room(ch,aroom);
	 clear_mount(ch);
	 if(obj->carried_by != NULL)
	 {
	  obj_from_char( obj );
	  obj_to_room( obj, ch->in_room );
  act( "$p springs to life.", obj->carried_by, obj, NULL, TO_CHAR ,FALSE);
	  act( "$n drops $p.", obj->carried_by, obj, NULL, TO_ROOM ,FALSE);
	 }
	 for(cobj = obj->contains; cobj != NULL; cobj = cobj_next )
	 {
	  cobj_next = cobj->next_content;
	  obj_from_obj( cobj );
	  obj_to_char( cobj, ch );
	 }
	
	 send_to_char("You spring to life.\n\r",ch);
	 extract_obj( obj );
	 return;
	}
	
  }
  return;
}

void do_deity_msg(char *msg, CHAR_DATA *ch)
{// Inserts a deity name into buf and sends it, saves a long line of code being repeated frequently
/*Inserts new line for you*/
	char buf[255];
	sprintf(buf, msg, deity_table[ch->pcdata->deity].pname);
	strcat(buf, "\n\r");
	send_to_char(buf, ch);
}

void fill_trial_name(char *toFill, CHAR_DATA *ch)
{
	switch(ch->pcdata->deity_trial)
	{
		case 0: strcpy(toFill, "Trial 0: Can't flee."); break;
		case 1: strcpy(toFill, "Trial 1: Kill good."); break;
		case 2: strcpy(toFill, "Trial 2: Kill neutral."); break;
		case 3: strcpy(toFill, "Trial 3: Kill evil."); break;
		case 4: strcpy(toFill, "Trial 4: Weak attacks."); break;
		case 5: strcpy(toFill, "Trial 5: Slow attacks."); break;
		case 6: strcpy(toFill, "Trial 6: Weak damage and casting."); break;
		case 7: strcpy(toFill, "Trial 7: Vuln to enemy attack."); break;
		case 8: strcpy(toFill, "Trial 8: Reduced skills."); break;
		case 9: strcpy(toFill, "Trial 9: No dodging."); break;
	}
}

void log_deity_favor(CHAR_DATA *ch, CHAR_DATA *alt, int type)
{
  char *strtime;
	char buf[255], trial[50], names[100];
	FILE *fp = fopen("deity_favor_details.txt", "a");
	if(ch == NULL)
	{
	  if(fp)
	  {
		fprintf(fp, "BUG: NULL character logged for deity favor.\n");
		fclose(fp);
	  }
		return;
	}
	if(IS_NPC(ch))
	{
	  if(fp)
	  {
		fprintf(fp, "BUG: NPC %s logged for deity favor.\n", ch->name);
		fclose(fp);
	  }
		return;
	}
	fill_trial_name(trial, ch);
	if(alt != NULL)
		sprintf(names, "%s (%d Lvl, %d favor) vs %s (%d Lvl):", ch->name, ch->level, ch->pcdata->deity_favor, alt->name, alt->level);
	else
		sprintf(names, "%s (%d Lvl, %d favor):", ch->name, ch->level, ch->pcdata->deity_favor);
	switch(type)
	{
		case DEITY_FAVOR_ACTIVATE:
			switch(ch->pcdata->deity_favor)
			{
				case 0: sprintf(buf, "%s has activated lesser favor.", names); break;	
				case 1: sprintf(buf, "%s has activated normal favor.", names); break;	
				case 2: sprintf(buf, "%s has activated greater favor.", names); break;
				case 3: sprintf(buf, "%s has activated a sliver of favor.", names); break;
				default: sprintf(buf, "%s has activated unknown favor.", names); break;
			} 
			break;
		case DEITY_TRIAL_ACTIVATE: sprintf(buf, "%s %s Trial activated.", names, trial); break;
		case DEITY_TRIAL_SUCCESS: sprintf(buf, "%s %s Trial completed!", names, trial); break;
		case DEITY_TRIAL_FAIL_DEATH: sprintf(buf, "%s %s Trial failed due to death.", names, trial); break;
		case DEITY_TRIAL_FAIL_TIMER: sprintf(buf, "%s %s Trial failed due to timer.", names, trial); break;
		case DEITY_TRIAL_FAIL_PK: sprintf(buf, "%s %s Trial failed due to pvp.", names, trial); break;
		case DEITY_TRIAL_FAIL_QUIT: sprintf(buf, "%s %s Trial failed due to quitting.", names, trial); break;
		case DEITY_TRIAL_FAIL_ABANDON: sprintf(buf, "%s %s Trial failed due to abandoning.", names, trial); break;
	}
	wiznet(buf, NULL,NULL,WIZ_DEITYFAVOR,0,0);
  strtime = ctime( &current_time );
  strtime[strlen(strtime)-1] = '\0';
  if(fp)
  {
    fprintf(fp, "%s :: %s\n", strtime, buf);
    fclose(fp);
  }
}

bool deity_enchant_armor(CHAR_DATA *ch, int amount)
{// Moved to its own function to reduce clutter - this one gets messy
	OBJ_DATA *obj;
	AFFECT_DATA *paf;
	int armors = 0;
	bool ac_found = FALSE;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
		if (obj->wear_loc != -1 && obj->item_type == ITEM_ARMOR)
		{// Check if it has room to be enchanted
			if(obj->level + 1 > ch->level && obj->level < LEVEL_HERO - 1)
				continue;// Don't enchant it above their level
			armors++;// Armor found
			if (!obj->enchanted)
			{
			  for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
			  {
				if ( paf->location == APPLY_AC )
				{
					if(paf->modifier < 0)
						armors--;// Not valid, can't be -ac at all
					break;// Done checking this item
				}
			  }
			}
			else
			{
				for ( paf = obj->affected; paf != NULL; paf = paf->next )
				{
				  if ( paf->location == APPLY_AC )
				  {
					if(paf->modifier < 0)
						armors--;// Not valid, can't be -ac at all
					break;// Done checking this item
				  }
				}
			}
		}
	}
	if(armors == 0)
	{// Send back a failure message
		return FALSE;
	}
	/* The real fun: randomly select a valid armor piece, find it, then enchant it */
	armors = number_range(1, armors);
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
		if (obj->wear_loc != -1 && obj->item_type == ITEM_ARMOR)
		{// Check if it has room to be enchanted
			if(obj->level + 1 > ch->level && obj->level < LEVEL_HERO - 1)
				continue;// Don't enchant it above their level
			armors--;// Armor found
			if (!obj->enchanted)
			{
			  for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
			  {
				if ( paf->location == APPLY_AC )
				{
					if(paf->modifier < 0)
						armors++;// Not valid, can't be -ac at all
					break;// Done checking this item
				}
			  }
			}
			else
			{
				for ( paf = obj->affected; paf != NULL; paf = paf->next )
				{
				  if ( paf->location == APPLY_AC )
				  {
					if(paf->modifier < 0)
						armors++;// Not valid, can't be -ac at all
					break;// Done checking this item
				  }
				}
			}
		}
		if(!armors)
			break;
	}
	if(obj == NULL)
	{// Send back a failure message - This one shouldn't happen
		bug( "Expected armor missing", 0);
		return FALSE;
	}
	do_deity_msg("%s is pleased with your choice of armor.", ch);
	/* Time to do an enchant on this object, it was cleared earlier */
    if (!obj->enchanted)
    {
  AFFECT_DATA *af_new;
  obj->enchanted = TRUE;

  for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
  {
      af_new = new_affect();
  
      af_new->next = obj->affected;
      obj->affected = af_new;

      af_new->where = paf->where;
      af_new->type  = UMAX(0,paf->type);
      af_new->level = paf->level;
      af_new->duration  = paf->duration;
      af_new->location  = paf->location;
      af_new->modifier  = paf->modifier;
      af_new->bitvector = paf->bitvector;
  }
    }

    if (amount == 1)
    {
  act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags, ITEM_MAGIC);
    }
    
    else if ( amount == 2)
    {
  act("$p glows a {Ybrilliant{x gold!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows a {Ybrilliant{x gold!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
    }

    else
    {// This one's safety is here in case someone accidentally does 
  act("$p glows with a {YBLINDING gold{x aura!!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows with a {YBLINDING gold{x aura!!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
	amount = 4;
    }

    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO)
  obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (ac_found)
    {
  for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
      if ( paf->location == APPLY_AC)
      {
    ac_found = TRUE;
    paf->type = skill_lookup("enchant armor");
    paf->modifier -= amount;
    paf->level = UMAX(paf->level,ch->level);
      }
  }
    }
    if(!ac_found)
    {// Wasn't added yet
  paf = new_affect();

  paf->where  = TO_OBJECT;
  paf->type = skill_lookup("enchant armor");
  paf->level  = ch->level;
  paf->duration = -1;
  paf->location = APPLY_AC;
  paf->modifier =  amount * -1;
  paf->bitvector  = 0;
      paf->next = obj->affected;
      obj->affected = paf;
    }
 return TRUE;
}

bool deity_enchant_weapon(CHAR_DATA *ch, OBJ_DATA *obj, int amount)
{
    AFFECT_DATA *paf; 
    int hit_bonus, dam_bonus;
    bool hit_found = FALSE, dam_found = FALSE;

	if(amount <= 0)
		return FALSE;

    if (obj->item_type != ITEM_WEAPON)
    {
  return FALSE;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;

    /* find the bonuses */

    if (!obj->enchanted)
    {
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
            if ( paf->location == APPLY_HITROLL )
            {
		hit_bonus = paf->modifier;
		hit_found = TRUE;
      	    }

            if (paf->location == APPLY_DAMROLL )
      	    {
        	dam_bonus = paf->modifier;
    		dam_found = TRUE;
            }
      }
    }
    else  /* object is enchanted */
    {
      for ( paf = obj->affected; paf != NULL; paf = paf->next )
      {
	if ( paf->location == APPLY_HITROLL )
	{
		hit_bonus = paf->modifier;
		hit_found = TRUE;
	}

	if (paf->location == APPLY_DAMROLL )
	{
		dam_bonus = paf->modifier;
		dam_found = TRUE;
	}

      }
    }

	if(hit_bonus + amount > amount * 2 || dam_bonus + amount > amount * 2 || (obj->level + 1 > ch->level && obj->level < LEVEL_HERO - 1))
		return FALSE;// Don't enchant it above their level, or the hit or damage too much

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
  AFFECT_DATA *af_new;
  obj->enchanted = TRUE;

  for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
  {
      af_new = new_affect();
  
      af_new->next = obj->affected;
      obj->affected = af_new;

      af_new->where = paf->where;
      af_new->type  = UMAX(0,paf->type);
      af_new->level = paf->level;
      af_new->duration  = paf->duration;
      af_new->location  = paf->location;
      af_new->modifier  = paf->modifier;
      af_new->bitvector = paf->bitvector;
  }
    }

	do_deity_msg("%s is pleased with your choice of weapons.", ch);

    if (amount == 1)
    {
  act("$p glows blue.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows blue.",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags, ITEM_MAGIC);
    }
    else if(amount == 3)
    {
  act("$p glows with a {BBLINDING blue{x aura!!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows with a {BBLINDING blue{x aura!!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
     }
     else
     {
  act("$p glows a {Bbrilliant{x blue!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows a {Bbrilliant{x blue!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
  amount = 2;// Safety check
     }
    
    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO - 1)
  obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (dam_found)
    {
  for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
      if ( paf->location == APPLY_DAMROLL)
      {
    paf->type = skill_lookup("enchant weapon");
    paf->modifier += amount;
    paf->level = UMAX(paf->level,ch->level);
    if (paf->modifier > 4)
        SET_BIT(obj->extra_flags,ITEM_HUM);
      }
  }
    }
    else /* add a new affect */
    {
  paf = new_affect();

  paf->where  = TO_OBJECT;
  paf->type = skill_lookup("enchant weapon");
  paf->level  = ch->level;
  paf->duration = -1;
  paf->location = APPLY_DAMROLL;
  paf->modifier =  amount;
  paf->bitvector  = 0;
      paf->next = obj->affected;
      obj->affected = paf;
    }

    if (hit_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
            if ( paf->location == APPLY_HITROLL)
            {
    paf->type = skill_lookup("enchant weapon");
                paf->modifier += amount;
                paf->level = UMAX(paf->level,ch->level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags,ITEM_HUM);
            }
  }
    }
    else /* add a new affect */
    {
        paf = new_affect();
 
        paf->type       = skill_lookup("enchant weapon");
        paf->level      = ch->level;
        paf->duration   = -1;
        paf->location   = APPLY_HITROLL;
        paf->modifier   =  amount;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }
	return TRUE;
}

int do_favor_error(CHAR_DATA *ch, int rarity, int index, int xp, int favor_strength)
{// There was a problem, let's reward them with a nice message but log a bug
	char buf[250];
	do_deity_msg("%s adores you.", ch);
	xp = xp * (90 + 25 * favor_strength) / 100;
	sprintf(buf, "Deity_favor out of range or level not set. %s (%d Lvl, %d favor) Values: %d rarity, %d index.", ch->name, ch->level, ch->pcdata->deity_favor, rarity, index);
	bug(buf, 0);
	return xp;
}

/* Return the xp bonus for a deity favor message */
int do_favor_reward(CHAR_DATA *ch, CHAR_DATA *victim, int rarity, int index, int xp, int favor_strength) 
{
	AFFECT_DATA af;
	char buf[255];
	int xp_level = -1;// Not set, used to catch bad cases
	switch(rarity)
	{
		case 3:{
			act("$n {yglows{x with a dim holy aura.",ch,NULL,NULL,TO_ROOM,FALSE);
			switch(index)
			{
				case 0: do_deity_msg("%s favors you.", ch); xp_level = 3; break;
				case 1: do_deity_msg("You feel %s's presence with you.", ch); xp_level = 3; break;
				case 2: do_deity_msg("%s smiles at you.", ch); xp_level = 3; break;
				case 3: do_deity_msg("You are granted a boon by %s.", ch);
					xp_level = 2;
					af.type      = skill_lookup("bless");
					affect_strip(ch,af.type);
					af.where     = TO_AFFECTS;
					af.level   = ch->level;
					af.duration  = 60;
					af.location  = APPLY_HITROLL;
					af.modifier  = 8;
					af.bitvector = 0;
					affect_to_char( ch, &af );

					af.location  = APPLY_SAVING_SPELL;
					af.modifier  = 0 - 8;
					affect_to_char( ch, &af );
					send_to_char( "You feel righteous.\n\r", ch );
					break;// Bless
				case 4: do_deity_msg("%s strengthens you.", ch);
					xp_level = 2;
					af.type      = skill_lookup("giant strength");
					affect_strip(ch,af.type);
					af.where     = TO_AFFECTS;
					af.level   = ch->level;
					af.duration  = 54;
					af.location  = APPLY_STR;
					af.modifier  = 4;
					af.bitvector = 0;
					affect_to_char( ch, &af );
					send_to_char( "Your muscles surge with heightened power!\n\r", ch );
					act("$n's muscles surge with heightened power.",ch,NULL,NULL,TO_ROOM,FALSE);
					break;// Giant Strength
				case 5: do_deity_msg("%s places magical barriers around you.", ch);
					xp_level = 2;
					af.type      = skill_lookup("armor");
					affect_strip(ch,af.type);
					af.where   = TO_AFFECTS;
					af.level   = ch->level;
					af.duration  = 50;
					af.modifier  = -20;
					af.location  = APPLY_AC;
					af.bitvector = 0;
					affect_to_char( ch, &af );
					send_to_char( "You feel someone protecting you.\n\r", ch );

					af.type      = skill_lookup("shield");
					affect_strip(ch,af.type);
					af.where     = TO_AFFECTS;
					af.level     = ch->level;
					af.duration  = 50;
					af.location  = APPLY_AC;
					af.modifier  = -20;
					af.bitvector = 0;
					affect_to_char( ch, &af );
					act( "$n is surrounded by a force shield.", ch, NULL, NULL, TO_ROOM ,FALSE);
					send_to_char( "You are surrounded by a force shield.\n\r", ch );
					break;// Armor and Shield
			}
			} break;// END RARITY 3
		case 2:{
			act("$n {yglows{x with a holy aura.",ch,NULL,NULL,TO_ROOM,FALSE);
			switch(index)
			{
				case 0:
					if((IS_AFFECTED(ch,AFF_BERSERK) && ch->race != race_lookup("dwarf")) || is_affected(ch,skill_lookup("calm")) )
					{
						xp_level = 2;// Slightly more xp to make up for no bonus spell
						do_deity_msg("%s is pleased with your enthusiasm for combat.", ch);
					}
					else
					{
						xp_level = 1;
						do_deity_msg("%s fills you with zeal!", ch);
						affect_strip(ch,skill_lookup("frenzy"));
						af.where     = TO_AFFECTS;
						af.type    = skill_lookup("frenzy");
						af.level   = ch->level;
						af.duration  = 20;
						af.modifier  = 8;
						af.bitvector = 0;

						af.location  = APPLY_HITROLL;
						affect_to_char(ch,&af);

						af.location  = APPLY_DAMROLL;
						affect_to_char(ch,&af);

						af.modifier  = 40;
						af.location  = APPLY_AC;
						affect_to_char(ch,&af);
						send_to_char("You are filled with holy wrath!\n\r",ch);
						act("$n gets a wild look in $s eyes!",ch,NULL,NULL,TO_ROOM,FALSE);
					}
					break;// Frenzy
				case 1: do_deity_msg("You have pleased %s.", ch);
					xp_level = 1;
					ch->hit  = UMIN(ch->hit + ch->max_hit / 5, ch->max_hit);
					ch->mana  = UMIN(ch->mana + ch->max_mana / 5, ch->max_mana);
					ch->move  = UMIN(ch->move + ch->max_move / 5, ch->max_move);
					send_to_char("You feel better.\n\r", ch);
					break;// 20% restore
				case 2:
					if ( is_affected(ch, gsn_hamstring) || IS_AFFECTED(ch,AFF_SLOW))
					{
						xp_level = 2;// Slightly more xp to make up for no bonus spell
						do_deity_msg("%s is pleased with your enthusiasm for combat.", ch);
					}
					else
					{
						do_deity_msg("%s accelerates your actions!", ch);
						xp_level = 1;
						af.type      = skill_lookup("haste");
						affect_strip(ch,af.type);
						REMOVE_BIT(ch->affected_by, AFF_HASTE);
						af.where     = TO_AFFECTS;
						af.level     = ch->level;
						af.duration  = 20;
						af.location  = APPLY_DEX;
						af.modifier  = 4;
						af.bitvector = AFF_HASTE;
						affect_to_char( ch, &af );
						send_to_char( "You feel yourself moving more quickly.\n\r", ch );
						act("$n is moving more quickly.",ch,NULL,NULL,TO_ROOM,FALSE);
					}
					break;// Haste
				case 3: do_deity_msg("%s protects you from harm.", ch);
					xp_level = 1;
					affect_strip(ch,gsn_sanctuary);
					REMOVE_BIT(ch->affected_by,AFF_SANCTUARY);
					act( "$n is surrounded by a white aura.", ch, NULL, NULL, TO_ROOM ,FALSE);
					send_to_char( "You are surrounded by a white aura.\n\r", ch );
					af.where     = TO_AFFECTS;
    				af.type      = gsn_sanctuary;
    				af.level     = ch->level;
					af.duration  = 10;
					af.location  = APPLY_NONE;
					af.modifier  = 0;
					af.bitvector = AFF_SANCTUARY;
					affect_to_char( ch, &af );
					break;// Sanctuary
				case 4: do_deity_msg("%s blesses you with material gains.", ch);
					xp_level = 2;
					ch->gold += 10 + favor_strength * 20;
					break;// +10/30/50 gold
				case 5: if(deity_enchant_armor(ch, favor_strength + 1) == TRUE)
							xp_level = 1;// If armor is enchanted, only level 1 xp
						else
						{// No message is sent on failure, same as for weapon enchant function
							do_deity_msg("%s is pleased with the care you take of your armor.", ch);
							xp_level = 2;
						}
					break;// Enchant armor
				case 6: do_deity_msg("%s protects you with a sharp wall of steel.", ch);
					xp_level = 1;
					affect_strip(ch,gsn_shield_of_thorns);
					affect_strip(ch,gsn_shield_of_brambles);
					affect_strip(ch,gsn_shield_of_spikes);
					affect_strip(ch,gsn_shield_of_blades);

					af.where            = TO_AFFECTS;
					af.type             = gsn_shield_of_blades;
					af.level            = ch->level;
					af.duration         = 8;
					af.modifier         = 60;
					af.location         = APPLY_AC;
					af.bitvector        = 0;

					affect_to_char(ch,&af);
					send_to_char("You are surrounded by a shield of blades.\n\r",ch);
					act("$n is surrounded by a shield of blades.",ch,NULL,NULL,TO_ROOM,FALSE);
					break;// Shield of Blades
				case 7: do_deity_msg("%s clouds your foe's minds.", ch);
					xp_level = 1;
    			af.type      = skill_lookup("blur");
					affect_strip(ch,af.type);
					af.where		= TO_AFFECTS;
					af.level		= ch->level;
					af.duration		= 12;
					af.location		= APPLY_AC;
					af.modifier		= -20;
					af.bitvector	= 0;
					affect_to_char( ch, &af );

					send_to_char("Your form begins to blur and shift.\n\r",ch);
					break;// Blur
				case 8: do_deity_msg("%s desires that you succeed.", ch);
					xp_level = 1;
    			af.type      = skill_lookup("aid");
					affect_strip(ch,af.type);
					af.where		= TO_AFFECTS;
					af.level		= ch->level;
					af.duration		= 17;
					af.location		= APPLY_HIT;
					af.modifier		= (ch->max_hit/(number_fuzzy(5)+number_fuzzy(5)));
					af.bitvector 	= 0;
					affect_to_char( ch, &af );

					send_to_char("Your vitality is blessed by the gods.\n\r",ch);
					break;// Aid
				case 9: 
					if(is_affected(ch,skill_lookup("calm")) )
					{
						xp_level = 2;// Slightly more xp to make up for no bonus spell
						do_deity_msg("%s is pleased with your enthusiasm for combat.", ch);
					}
					else
					{
						do_deity_msg("%s fills you with holy fury!", ch);
						xp_level = 1;
						send_to_char("Your pulse races as you are consumed by rage!\n\r",ch);
						act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM,FALSE);

						affect_strip(ch,skill_lookup("frenzy"));
						affect_strip(ch,gsn_berserk);

						af.where  = TO_AFFECTS;
						af.type   = gsn_berserk;
						af.level  = ch->level;
						af.duration = 10;
						af.modifier = 10;

						af.bitvector  = AFF_BERSERK;

						af.location = APPLY_HITROLL;
						affect_to_char(ch,&af);

						af.location = APPLY_DAMROLL;
						affect_to_char(ch,&af);

						af.modifier = 80;
						af.location = APPLY_AC;
						affect_to_char(ch,&af);
					}
					break;// Berserk
			}
			} break;// END RARITY 2
		case 1:{
			act("$n {yglows{x with a holy aura.",ch,NULL,NULL,TO_ROOM,FALSE);
			switch(index)
			{
				case 0: do_deity_msg("%s is especially pleased with you.", ch);
					xp_level = 1;
					ch->hit  = UMIN(ch->hit + ch->max_hit / 2, ch->max_hit);
					ch->mana  = UMIN(ch->mana + ch->max_mana / 2, ch->max_mana);
					ch->move  = UMIN(ch->move + ch->max_move / 2, ch->max_move);
					send_to_char("You feel greatly restored.\n\r", ch);
					break;// 50% restore
				case 1: {
					OBJ_DATA *obj;
					xp_level = 1;// Success level, failure will boost it
				  if ( ( obj = get_eq_char( ch, WEAR_WIELD ) ) != NULL )
					{
						if(!deity_enchant_weapon(ch, obj, favor_strength + 1))
						{// It handles success in the function, so do failure - check offhand
							if( (obj = get_eq_char( ch, WEAR_SECOND ) ) != NULL)
							{
								if(!deity_enchant_weapon(ch, obj, favor_strength + 1))
								{
									do_deity_msg("%s is pleased by the care you take of your weaponry.", ch);
									xp_level = 2;// Bonus xp for no item to enchant
								}
							}
							else
							{
								do_deity_msg("%s is pleased by the care you take of your weaponry.", ch);
								xp_level = 2;// Bonus xp for no item to enchant
							}
						}
					}
					else if( (obj = get_eq_char( ch, WEAR_SECOND ) ) != NULL)
					{// This shouldn't be possible, but may as well catch it in case it happens
						if(deity_enchant_weapon(ch, obj, favor_strength + 1) == FALSE)
						{
							do_deity_msg("%s is pleased by the care you take of your weaponry.", ch);
							xp_level = 2;// Bonus xp for no item to enchant
						}
					}
					else
					{
						do_deity_msg("%s is impressed that you fight with your bare hands.", ch);
						xp_level = 2;// Same bonus as for other gear that can't be enchanted
					}
					}
					break;// Enchant weapon. Go ahead and check offhand if primary doesn't qualify
				case 2: do_deity_msg("%s has granted you some skill.", ch);
					xp_level = 2;
					ch->skill_points++;
					break;// +1 skill point
				case 3: 
    				af.type      = skill_lookup("acclimate");
					if ( is_affected(ch,af.type) )
					{// Be very surprised if it gets here since they just got experience
						do_deity_msg("%s is disappointed that you were not participating with your allies.", ch);
						xp_level = 1;// Don't acclimate while your friends are fighting
						break;
					}
					do_deity_msg("{YDon't move!{x %s has greatly accelerated your healing!", ch);
					xp_level = 2;
					af.where            = DAMAGE_OVER_TIME;
					af.level            = ch->level;
					af.duration         = 12; /* At 100hp/pulse, this is all-star! */
					af.modifier         = -100;
					af.location         = -100;
					af.bitvector        = 0;

					affect_to_char(ch,&af);
					send_to_char("You are more attuned to your environment.\n\r",ch);
					act("$n is more attuned to $s environment.",ch,NULL,NULL,TO_ROOM,FALSE);
					break;// Acclimate
				case 4: do_deity_msg("%s finds you a {Wdiamond{x in the rough", ch);
					xp_level = 2;
					obj_to_char(create_object(get_obj_index(OBJ_VNUM_DIAMOND),0,FALSE),ch);
					break;// Gives you a diamond
			}
			} break;// END RARITY 1
		case 0:{
			/* No spell effects here - if any are added in the future, insert the glow message there
				 and add a special case at the end to exempt that one.
				 Glow message is at the end because trial gets a different one than others 
				 trial xp_level is set at end based on trial timer being set*/
			switch(index)
			{
				case 0: do_deity_msg("%s renews your mind.", ch);
					xp_level = 1;
					ch->pcdata->skill_point_tracker = UMAX(0, ch->pcdata->skill_point_tracker - 2 + 2 * favor_strength);
					break;// Reset stat point pool 2/4/6 (6 is rarer but can happen, usually is 5)
				case 1: do_deity_msg("%s is thrilled with you!", ch);
					xp_level = 1;
					ch->hit  = ch->max_hit;
					ch->mana  = ch->max_mana;
					ch->move  = ch->max_move;
					send_to_char("You feel fully restored.\n\r", ch);
					break;// Restore
				case 2: do_deity_msg("%s improves your knowledge.", ch);
					xp_level = 1;
	        {// Swimming and recall aren't normally improved by this
	        	int sn, count = 0;
					for ( sn = 0; sn < MAX_SKILL; sn++ )
	        {
	           if ( skill_table[sn].name == NULL )
	              break;
	           if ( ch->level < skill_level(ch,sn)
	               || ch->pcdata->learned[sn] < 2 || ch->pcdata->learned[sn] > 99
								 || sn == gsn_recall || sn == gsn_swim)
	              continue;
	          count++;
	        }
	        count = number_range(1, count);// Select one of the available skills to improve
					for ( sn = 0; sn < MAX_SKILL; sn++ )
	        {
	           if ( skill_table[sn].name == NULL )
	              break;
	           if ( ch->level < skill_level(ch,sn)
	               || ch->pcdata->learned[sn] < 2 || ch->pcdata->learned[sn] > 99
								 || sn == gsn_recall || sn == gsn_swim)
	              continue;
						count--;
						if(!count)
							break;
	        }
	        if(sn >= 0)
	        {// found a skill to improve
	        	 sprintf(buf, "You have become better at %s!\n\r", skill_table[sn].name);
	        	 send_to_char(buf, ch);
						 ch->pcdata->learned[sn]++;// 1% better
	        }
	        else
	        {// Check swimming, then check recall - everything else is done
	        	if(ch->pcdata->learned[gsn_swim] < 100)
	        	{
	        	 sprintf(buf, "You have become better at %s!\n\r", skill_table[gsn_swim].name);
	        	 send_to_char(buf, ch);
						 ch->pcdata->learned[gsn_swim]++;// 1% better
						 xp_level = 2; // Better xp for having all other skills maxed
	        	}
	        	else if(ch->pcdata->learned[gsn_recall] < 100)
	        	{// Since characters above 10 can't use this, it's mostly a just because
	        	 sprintf(buf, "You have become better at %s!\n\r", skill_table[gsn_recall].name);
	        	 send_to_char(buf, ch);
						 ch->pcdata->learned[gsn_recall]++;// 1% better
						 xp_level = 2; // Better xp for having all other skills maxed
	        	}
	        	else
	        	{
	        		do_deity_msg("%s is {Wvery impressed{x with your mastery of your skills!", ch);
	        		xp_level = 3;// Top xp reward for this case
	        	}
	        }
	        }
					break;// 1% improvement to a >1%, <100% skill
				case 3: do_deity_msg("%s rewards you with prosperity.", ch);
					xp_level = 2;
					ch->gold += 150;
					break;// 150 gold
				case 4:{
					CHAR_DATA *clone = create_mobile(victim->pIndexData);
					clone_mobile(victim,clone); 
					clone->hit  = clone->max_hit;
					clone->mana = clone->max_mana;
					clone->move = clone->max_move;
					update_pos(clone);
					do_deity_msg("%s tests you with a trial of courage! {YDo not flee!{x", ch);
					char_to_room(clone,ch->in_room);
					act("$n appears with a {yBLINDING{x flash and screams and attacks!", clone, NULL, NULL, TO_ROOM, FALSE);
					multi_hit(clone,ch,TYPE_UNDEFINED);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 0;
					}
					break; // Trial - no flee, 1 skill point
				case 5: 
					ch->pcdata->deity_trial_timer = 10;
					if(IS_EVIL(victim))
					{
						ch->pcdata->deity_trial = 1;
						do_deity_msg("%s tests you with a trial of balance. {YKill a good aligned foe.{x", ch);
					}
					else if(IS_GOOD(victim))
					{
						ch->pcdata->deity_trial = 3;
						do_deity_msg("%s tests you with a trial of balance. {YKill an evil aligned foe.{x", ch);
					}
					else//Good, Neutral, Evil requirements -- opposite of current enemy
					{
						ch->pcdata->deity_trial = 2;
						do_deity_msg("%s tests you with a trial of balance. {YKill a neutral aligned foe.{x", ch);
					}
					break; // Trial - kill aligned mob, neutral: 1 skill point, good/evil: 2 skill points
				case 6: do_deity_msg("%s tests you with a trial of resistance. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 4;
					break; // Trial - enemy resists your attack damage. 1 skill point.
				case 7: do_deity_msg("%s tests you with a trial of sloth. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 5;
					break; // Trial - bonus attacks reduced, even haste. 2 skill points.
				case 8: do_deity_msg("%s tests you with a trial of weakness. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 6;
					break; // Trial - damage and casting level reduced 50%. 2 skill points.
				case 9: do_deity_msg("%s tests you with a trial of vulnerability. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 7;
					break; // Trial - you take 33% increased damage from enemy attacks. 2 skill points.
				case 10: do_deity_msg("%s tests you with a trial of forgetfulness. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 8;
					break; // Trial - all skills at 1/2 skill level. 3 skill points.
				case 11: do_deity_msg("%s tests you with a trial of pain. {RBe careful!{x", ch);
					ch->pcdata->deity_trial_timer = 10;
					ch->pcdata->deity_trial = 9;
					break; // Trial - no dodge/parry/shield block/kailindo. 3 skill points.
			}
			if(ch->pcdata->deity_trial_timer > 0)
			{
				xp_level = 0;// No bonus xp for trial assignment
				sprintf(buf, "You have {Y%d ticks{x to complete your trial. 'pray abandon' to end early.\n\r", ch->pcdata->deity_trial_timer);
				send_to_char(buf, ch);
				act("$n {Yglows{x with a {YBLINDING{x holy aura.",ch,NULL,NULL,TO_ROOM,FALSE);
				log_deity_favor(ch, NULL, DEITY_TRIAL_ACTIVATE);
			}
			else
			{// Rare
				act("$n {Yglows{x with a {Ybright{x holy aura.",ch,NULL,NULL,TO_ROOM,FALSE);
			}
			} break;// END RARITY 0 (Least common)
	}
#ifdef DEITY_TRIAL_DEBUG_CODE
{
	int old_xp = xp;
#endif
	switch(xp_level)
	{// Three xp levels, leave at 0 if it's set elsewhere. Higher is more.
		case 0: xp = 0; break;
		case 1: xp = xp * (40 + favor_strength * 15) / 100; break;
		case 2: xp = xp * (65 + favor_strength * 20) / 100; break;
		case 3: xp = xp * (90 + favor_strength * 25) / 100; break;
		default: return do_favor_error(ch, rarity, index, xp, favor_strength);
	}
#ifdef DEITY_TRIAL_DEBUG_CODE
	sprintf(buf, "Old xp: %d New xp: %d Boost: %f%% (Level %d)\n\r", old_xp, old_xp + xp, ((old_xp + xp) / (float)old_xp) * 100.0f, xp_level);
	send_to_char(buf, ch);
}// Close the braces opened in earlier ifdef
#endif
	return xp;
}

// Returns lower xp than started -- returned xp should be added in to original xp
int deity_favor_message(CHAR_DATA *ch, CHAR_DATA *victim, int xp)
{
	/* The meat of this, all the special messages
	   Broken down by rarity levels. At this time, there are four.
		 Each level adds its index + 1 into the pool: So rarity 0 adds 1, while rarity 3 adds 4
		 A pool with one rarity 0 and one rarity 3 would have 20% chance of 0 and 80% for 3
		 Not all levels need events, blanks can be used to add more levels while preserving ratios
		 If a lower level has a unique a higher level doesn't (And the higher level has uniques also)
		  it should be split in do_favor_reward by checking favor level for that index
	*/
	int rarity[FAVOR_RARITY], i, j, count, total[FAVOR_RARITY], strength;
	memset(rarity, 0, sizeof(int) * FAVOR_RARITY);
	/* Three favor levels, which modify which rewards are available */
	switch(ch->pcdata->deity_favor)
	{//Listed in common -> rare
		case 0:	rarity[3] = 6;
			rarity[2] = 6;
			rarity[1] = 3;
			rarity[0] = 2;
			strength = 0; break;
		case 1:	rarity[3] = 6;
			rarity[2] = 10;
			rarity[1] = 5;
			rarity[0] = 2;
			strength = 1; break;
		case 2:	rarity[3] = 6;
			rarity[2] = 10;
			rarity[1] = 5;
			rarity[0] = 12;
			strength = 2; break;
		case 3: rarity[3] = 6;// Sliver - doesn't do much
			rarity[2] = 4;
			strength = 0; break;
	}
	/* From here on, all loops go from high to low -- for the breakdown to work right
		 the most common (Highest pool) have to be checked first 
		 It should be set up to not need modification for most uses*/	

	for(i = FAVOR_RARITY - 1; i >= 0; i--)
	{
		rarity[i] *= (i + 1);
		total[i] = 0;
		for(j = FAVOR_RARITY - 1; j > i; j--)
  		total[i] += rarity[j];// Add any higher levels already gone past
	}
	count = number_range(0, total[0] + rarity[0] - 1);
	/* Formula converts it to an index in the proper range based on rarity */
#ifdef DEITY_TRIAL_DEBUG_CODE
	if(deity_msg_override > 0)
	{
		count = deity_msg_override - 1;
	}
#endif
	for(i = FAVOR_RARITY - 1; i >= 0; i--)
	{
		if(count - total[i] < rarity[i])
		{
#ifdef DEITY_TRIAL_DEBUG_CODE
			if(deity_msg_override > 0)
			{
				char buf[255];
				sprintf(buf, "%d override: %d rarity, %d index\n\r", deity_msg_override, i, (count - total[i]) / (i + 1));
				send_to_char(buf, ch);
			}
#endif
			xp = do_favor_reward(ch, victim, i, (count - total[i]) / (i + 1), xp, strength);
			break;
		}
	}
	if(i < 0)// Number out of range
		return do_favor_error(ch, -1, -1, xp, strength);
	return xp;
}
   
int deity_trial_kill(CHAR_DATA *ch, CHAR_DATA *victim, int xp)
{// If it calls this function, xp was awarded
	int skillVal = 0;
	char buf[255];
	switch(ch->pcdata->deity_trial)
	{
		case 0: // Trial - no flee, 1 skill point
			skillVal = 1;
			break;
		case 1: // Trial - kill good mob, 2 skill points
			if(IS_GOOD(victim))
				skillVal = 2;
			else
			{// Failure for killing the wrong alignment enemy
				sprintf(buf, "You have {Rfailed{x the trial given to you by %s.\n\r", deity_table[ch->pcdata->deity].pname);
				send_to_char(buf,ch);
				ch->pcdata->deity_trial_timer = 0;
				log_deity_favor(ch, victim, DEITY_TRIAL_FAIL_REQS);
			}
			break;
		case 2: // Trial - kill neutral mob, 1 skill point
			if(!IS_GOOD(victim) && !IS_EVIL(victim))
				skillVal = 1;
			else
			{// Failure for killing the wrong alignment enemy
				sprintf(buf, "You have {Rfailed{x the trial given to you by %s.\n\r", deity_table[ch->pcdata->deity].pname);
				send_to_char(buf,ch);
				ch->pcdata->deity_trial_timer = 0;
				log_deity_favor(ch, victim, DEITY_TRIAL_FAIL_REQS);
			}
			break;
		case 3: // Trial - kill evil mob, 2 skill points
			if(IS_EVIL(victim))
				skillVal = 2;
			else
			{// Failure for killing the wrong alignment enemy
				sprintf(buf, "You have {Rfailed{x the trial given to you by %s.\n\r", deity_table[ch->pcdata->deity].pname);
				send_to_char(buf,ch);
				ch->pcdata->deity_trial_timer = 0;
				log_deity_favor(ch, victim, DEITY_TRIAL_FAIL_REQS);
			}
			break;
		case 4: // Trial - enemy resists your attack damage. 1 skill point.
			skillVal = 1;
			break;
		case 5: // Trial - bonus attacks reduced, even haste. 3 skill point. - out of order, raised skill points based on testing
			skillVal = 3;
			break;
		case 6: // Trial - damage and casting level reduced 50%. 2 skill points.
			skillVal = 2;
			break;
		case 7: // Trial - you take 33% increased damage from enemy attacks. 2 skill points.
			skillVal = 2;
			break;
		case 8: // Trial - all skills at 1/2 skill level. 3 skill points.
			skillVal = 3;
			break;
		case 9: // Trial - no dodge/parry/shield block/kailindo/scales. 3 skill points.
			skillVal = 3;
			break;
		
	}
	if(skillVal)
	{// Right now, skill points are always part of trial rewards.
		sprintf(buf, "You have completed the trial given to you by %s!\n\r", deity_table[ch->pcdata->deity].pname);
		send_to_char(buf, ch);
		send_to_char("You are rewarded with some skill!\n\r", ch);
		act("$n is surrounded by a holy glow.", ch, NULL, NULL, TO_ROOM, FALSE);
		xp *= 2;// Triple xp, so return double the current amount
		ch->skill_points += skillVal;
		ch->pcdata->deity_trial_timer = 0;// End the trial
		log_deity_favor(ch, victim, DEITY_TRIAL_SUCCESS);
		return xp;
	}
	return 0;// For now, it's not success if no skillVal
}	   
