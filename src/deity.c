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
      give_gift(ch,gift);
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
	 af.type      = 0;
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
          af.type      = 0;
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
	   
