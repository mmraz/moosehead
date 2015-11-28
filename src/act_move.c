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

static char rcsid[] = "$Id: act_move.c,v 1.190 2004/10/31 03:32:07 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include "merc.h"
#include "tables.h"
#include "gladiator.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look    );
DECLARE_DO_FUN(do_recall  );
DECLARE_DO_FUN(do_stand   );

CLAN_DATA *clan_lookup   args( ( const char *name ) );
bool recall args( (CHAR_DATA *ch, char *argument, bool fPray ) );
bool  has_boat args( ( CHAR_DATA *ch ) );
char	kludge_string[MAX_STRING_LENGTH];

char *  const dir_name  []    =
{
    "north", "east", "south", "west", "up", "down"
};

const sh_int  rev_dir   []    =
{
    2, 3, 0, 1, 5, 4
};

const sh_int  movement_loss [SECT_MAX]  =
{
    1, 2, 2, 3, 4, 6, 4, 1, 6, 10, 6
};



/*
 * Local functions.
 */
int find_door args( ( CHAR_DATA *ch, char *arg ) );
int find_exit args( ( CHAR_DATA *ch, char *arg ) );
bool  has_key   args( ( CHAR_DATA *ch, int key ) );

void do_fade( CHAR_DATA *ch, char *argument )
{
    if ( get_skill(ch,skill_lookup("fade")) < 25 ||
	!HAS_KIT(ch,"nethermancer") )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if ( ch->mana < 100 ) {
	send_to_char("You don't have enough mana.\n\r",ch);
	return;
    }

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char("Gladiator's can not fade.\n\r",ch);
       return;
    }

    WAIT_STATE(ch,PULSE_VIOLENCE);
    if ( number_percent() < get_skill(ch,skill_lookup("fade")) / 2 )
    {
        if ( IS_SET( ch->mhs, MHS_FADE ) )
	{
	    REMOVE_BIT( ch->mhs, MHS_FADE );
	    act("$n fades into existence.",ch,NULL,NULL,TO_ROOM,FALSE);
	    act("You fade into existence.",ch,NULL,NULL,TO_CHAR,FALSE);
	}
	else
	{
	    SET_BIT(ch->mhs,MHS_FADE );
	    act("$n shimmers and fades away.",ch,NULL,NULL,TO_ROOM,FALSE);
	    act("You shimmer and fade away.",ch,NULL,NULL,TO_CHAR,FALSE);
  	}
	return;
    }

    send_to_char("You failed.\n\r",ch);
    return;
}


/* Mount code */
void do_mount( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *mount;
	char arg[MAX_INPUT_LENGTH];

	one_argument(argument,arg);

	if( is_affected(ch,skill_lookup("wraithform")) )
	{
	send_to_char("Not while in wraithform.\r\n",ch);
	return;
	}

        

	if(arg[0] == '\0')
	{
	    send_to_char("Ride what?\n\r",ch);
	    return;
	}


	if( ( mount = get_char_room(ch,arg) ) == NULL )
	{
	    send_to_char("They're not here.\n\r",ch);
	    return;
    	}

	if (mount->master != ch && !IS_IMMORTAL(ch))
	{
	  send_to_char("That's not your horse, and we hang horse thieves.\n\r",ch);
	  return;
	}

	if( (IS_NPC(mount) && !IS_SET(mount->act,ACT_MOUNT) ) ||
	    (!IS_NPC(mount) && !IS_SET(mount->mhs,MHS_MOUNT) ) )
	{
	    send_to_char("You can't ride that.\n\r",ch);
	    return;
	}

	if( !IS_NPC(mount) && IS_SET(mount->mhs,MHS_NOMOUNT) )
	{
	    act("$N isn't accepting riders.",ch,NULL,mount,TO_CHAR,FALSE);
	    return;
   	}

	if(mount->passenger != NULL)
	{
	    act("$N already has a rider.",ch,NULL,mount,TO_CHAR,FALSE);
	    return;
	}

	if(mount->riding != NULL)
	{
	    act("$N is occupied.",ch,NULL,mount,TO_CHAR,FALSE);
	    return;
	}

	if(ch->size > mount->size)
	{
	    send_to_char("You are too big to ride that.\n\r",ch);
	    return;
   	}

	ch->riding = mount;
	mount->passenger = ch;
	
	act("$n mounts $N.",ch,NULL,mount,TO_NOTVICT,FALSE);
	act("You mount $N.",ch,NULL,mount,TO_CHAR,FALSE);
	act("$n mounts you.",ch,NULL,mount,TO_VICT,FALSE);
	
	return;
}

void do_dismount( CHAR_DATA *ch, char *argument )
{
	CHAR_DATA *mount;

	if ( ( mount = ch->riding ) == NULL )
	{
	    send_to_char("You aren't riding anything.\n\r",ch);
	    return;
	}

	ch->riding = NULL;
	mount->passenger = NULL;

	act("You dismount.",ch,NULL,mount,TO_CHAR,FALSE);
	act("$n dismounts you.",ch,NULL,mount,TO_VICT,FALSE);
	act("$n dismounts from $N.",ch,NULL,mount,TO_NOTVICT,FALSE);

	return;
}


void move_char( CHAR_DATA *ch, int door, bool follow )
{
    CHAR_DATA *mount;
    CHAR_DATA *fch;
    CHAR_DATA *fch_next;
    ROOM_INDEX_DATA *in_room;
    ROOM_INDEX_DATA *to_room;
    EXIT_DATA *pexit;
    AFFECT_DATA *paf, *paf_next;
    OBJ_DATA *obj;
    bool found = FALSE;
    int swim = 0;
    int cost = 0;
    bool fHighlander = FALSE;
    bool slipsafe = TRUE;
    int move_time = 1;

    if ( is_affected(ch,gsn_trap) )
    {
	send_to_char("You are held fast by a snare trap.\n\r",ch);
	return;
    }
//COREY PUT THE WEATHER CHECK HERE
//#ifdef COREYTEST
    if ( !IS_IMMORTAL(ch) && !IS_NPC(ch) && IS_OUTSIDE(ch) )
	slipsafe = FALSE; 

    if ( ch->race != race_lookup("elf") 
    ||   ch->kit  != kit_lookup("ranger")
    ||   !IS_AFFECTED(ch, AFF_FLYING) 
       )
	slipsafe = TRUE;

    if ( slipsafe == FALSE && number_range(1,100) < 10 
       &&  weather_info.sky == SKY_RAINING )
    {
	if ( number_range(1,30) > get_curr_stat(ch,STAT_DEX) )
        {
	send_to_char("You slip and fall.  Damn Weather!",ch);
	return;
	}
    }

    if ( slipsafe == FALSE && number_range(1,100) < 15 
       &&  weather_info.sky == SKY_LIGHTNING )
    {
        if ( number_range(1,35) > get_curr_stat(ch,STAT_DEX) )
        {
        send_to_char("You slip and fall.  Damn Weather!",ch);
        return;
        }
    }


//#endif

    if ( is_affected(ch,skill_lookup("hold person")))
    {
	send_to_char("Your muscles are frozen!\n\r",ch);
	return;
    }
/*
    if ( is_affected(ch,gsn_earthbind) )
    {
	send_to_char("Your feet are bound to the earth.\n\r",ch);
	return;
    }
    */

  if ( is_affected(ch,gsn_hamstring) && number_percent() >
        ((get_curr_stat(ch,STAT_CON)*2)+(get_curr_stat(ch,STAT_STR*2))))
  {
	act("You try to move but your legs give out!",ch,NULL,NULL,TO_CHAR,FALSE);
	act("$n tries to move but $s legs give out!",ch,NULL,NULL,TO_ROOM,FALSE);
	return;
  }

  if(is_room_affected(ch->in_room, gsn_wall_ice))
	{
	AFFECT_DATA *paf;

        for(paf=ch->in_room->affected; paf != NULL; paf = paf->next)
	   if (paf->type == gsn_wall_ice)
              break;
        if(paf->location == door)
           {
           send_to_char("A wall of ice blocks that exit.\n\r", ch);
           return;
           }
        }

    mount = ch->riding;

    if ( door < 0 || door > 5 )
    {
  bug( "Do_move: bad door %d.", door );
  return;
    }

    in_room = ch->in_room;
   if ( ( pexit = in_room->exit[door] ) == NULL
      && !IS_NPC(ch) && IS_IMMORTAL(ch) &&
      IS_SET (ch->pcdata->edit.per_flags,EDIT_AUTO_CREATE) ) 
   {      
        if (create_room (ch,ch->in_room,door,TRUE))
          do_look( ch, "auto" );
        return;
   }
    

    in_room = ch->in_room;
    if ( ( pexit   = in_room->exit[door] ) == NULL
    ||   ( to_room = pexit->u1.to_room   ) == NULL 
    ||   ( !can_see_room(ch,to_room) ) )
    {    
  send_to_char( "Alas, you cannot go that way.\n\r", ch );
  return;
    }

   if (IS_SET(ch->mhs,MHS_HIGHLANDER))  
   {
      if (to_room->clan || is_room_owner(ch,to_room)
      || room_is_private(ch,to_room))
      {
         send_to_char("You are a Highlander, go out and fight, There can be only one!\n\r",ch);
         return;
      }
   }

   if (to_room->clan && ch->clan != to_room->clan && !IS_IMMORTAL(ch) )
   {
   /*
    if ( ch->move < 5 )
    {
        send_to_char( "You are too exhausted.\n\r", ch );
        return;
    }
       if ( is_clan(ch) && number_percent() < get_skill(ch, gsn_infiltrate)
       && (ch->pcdata && ch->pcdata->quit_time == 0) && !IS_IMMORTAL (ch))
       {
           if ( ch->move < 10 )
           {
              send_to_char( "You are too exhausted.\n\r", ch );
              return;
           }
	   cost += 5;
	   check_improve(ch,gsn_infiltrate,TRUE,2);
	   send_to_char("You infiltrate the clan hall.\n\r",ch);
       }
       else
       {
	   check_improve(ch,gsn_infiltrate,FALSE,4);
	   send_to_char("Alas, you cannot go that way.\n\r",ch);
	   return;
       }
     */ /* can't infiltrate clan halls */

	   send_to_char("Alas, you cannot go that way.\n\r",ch);
	   return;
   }
   if(to_room->vnum < 0)
   {
     CHAR_DATA *attempt = NULL;
     if(IS_NPC(ch))
     {
       if(ch->master)
       {
         if(ch->master->pet == ch || ch->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
           attempt = ch->master;
       }
       if(!attempt || IS_NPC(attempt))
       {
         send_to_char("Alas, you cannot go that way.\n\r", ch);
         return;
       }
     }
     else
       attempt = ch;
     if(!IS_IMMORTAL(attempt) && (!attempt || !attempt->pcdata->clan_info ||
       attempt->pcdata->clan_info->clan->vnum_min > abs(to_room->vnum) ||
       attempt->pcdata->clan_info->clan->vnum_max < abs(to_room->vnum)))
       {
         send_to_char("Alas, you cannot go that way.\n\r", ch);
         return;
       }
   }
    /* Banishment gift from almighty */
     if ( ch->clan && IS_SET(ch->mhs, MHS_BANISH) &&
	 to_room->clan == ch->clan )
	 {
	 send_to_char("The force of the Almighty prevents you from entering your clan hall \n\r", ch);
	 return;
	 }

     /* Banished from clan hall by use of sanctions */
     if (!IS_NPC(ch) && ch->clan && IS_SET(ch->pcdata->clan_flags, CLAN_NO_HALL) &&
	to_room->clan == ch->clan)
	{
	send_to_char("You have been forbidden entrance to the clan hall.\n\r", ch);
	return;
	}


    if (IS_SET(pexit->exit_info, EX_CLOSED)
    &&  (!IS_AFFECTED(ch, AFF_PASS_DOOR) || IS_SET(pexit->exit_info,EX_NOPASS))
    &&   !IS_TRUSTED(ch,ANGEL) )
    {
  if ( IS_SET(pexit->exit_info, EX_SECRET) )
  	send_to_char( "Alas, you cannot go that way.\n\r", ch);
  else
  	act( "The $d is closed.", ch, NULL, pexit->keyword, TO_CHAR ,FALSE);
  return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM)
    &&   ch->master != NULL
    &&   in_room == ch->master->in_room )
    {
  send_to_char( "What?  And leave your beloved master?\n\r", ch );
  return;
    }


    if ( !is_room_owner(ch,to_room) && room_is_private(ch, to_room ) )
    {
        if ( ch->move < 10 )
        {
           send_to_char( "You are too exhausted.\n\r", ch );
           return;
        }
        if ( number_percent() < get_skill(ch,gsn_infiltrate)
	&& (ch->pcdata && ch->pcdata->quit_time == 0) && !IS_IMMORTAL (ch))
	{
	    cost += 5;
	    check_improve(ch,gsn_infiltrate,TRUE,2);
	    send_to_char("You infiltrate the room.\n\r",ch);
	}
        else
        {
  check_improve(ch,gsn_infiltrate,FALSE,4);
  send_to_char( "That room is private right now.\n\r", ch );
  return;
        }
    }

    if ( !IS_NPC(ch) )
    {
  int iClass, iGuild;
  int move, ok=FALSE, fRoom=FALSE;

  for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
  {
      for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)  
      {
	if ( (iClass == ch->class 
		|| (!IS_NPC(ch) && iClass == ch->pcdata->old_class))
	&& to_room->vnum == class_table[iClass].guild[iGuild] )
	{
	  ok = TRUE;
	  break;
	}
	if ( iClass != ch->class
	&& to_room->vnum == class_table[iClass].guild[iGuild] )
	{
	fRoom = TRUE;
        }
      }
  }

    /*    if ( number_percent() < get_skill(ch,gsn_infiltrate)
           &&  ( ok == FALSE  && fRoom == TRUE )
	   )
	{
	    cost += 5;
	    check_improve(ch,gsn_infiltrate,TRUE,2);
	    send_to_char("You infiltrate the guild. \n\r",ch);
	}
        else
        {
  check_improve(ch,gsn_infiltrate,FALSE,4);
   send_to_char( "You aren't allowed in there.\n\r", ch );
  return;
        } */


           if  ( ok == FALSE  && fRoom == TRUE )
	   {
           if ( ch->move < 10 )
           {
              send_to_char( "You are too exhausted.\n\r", ch );
              return;
           }
             if ( number_percent() < get_skill(ch,gsn_infiltrate) )
		{
		    cost += 5;
		    check_improve(ch,gsn_infiltrate,TRUE,2);
		    send_to_char("You infiltrate the guild. \n\r",ch);
		}
		else
		{
		  check_improve(ch,gsn_infiltrate,FALSE,4);
		   send_to_char( "You aren't allowed in there.\n\r", ch );
		  return;
		} 
            }

  if ( in_room->sector_type == SECT_AIR
  ||   to_room->sector_type == SECT_AIR )
  {
      if ( !IS_AFFECTED(ch, AFF_FLYING) && !IS_IMMORTAL(ch))
      {
    send_to_char( "You can't fly.\n\r", ch );
    return;
      }
  }

  if (( in_room->sector_type == SECT_WATER_NOSWIM
  ||    to_room->sector_type == SECT_WATER_NOSWIM )
    &&    !IS_AFFECTED(ch,AFF_FLYING))
  {
      found = has_boat(ch);

      if ( !found )
      {
    send_to_char( "You need a boat to go there.\n\r", ch );
    return;
      }
  }

  if ( get_carry_weight(ch) > can_carry_w(ch) )
	{
	found = FALSE;
	for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
	 {
	  if ( obj->item_type != ITEM_FOOD && can_drop_obj(ch,obj) 
		&& obj->wear_loc == WEAR_NONE )
		{
		act("{GYou drop $p to move unhindered.{x",ch,obj,NULL,TO_CHAR,FALSE);
		act("$n drops $p.",ch,obj,NULL,TO_ROOM,FALSE);
		obj_from_char(obj);
		obj_to_room(obj,ch->in_room);
		found = TRUE;
		}
	   if ( found ) break;
	  }
	}

  move = movement_loss[UMIN(SECT_MAX-1, in_room->sector_type)]
       + movement_loss[UMIN(SECT_MAX-1, to_room->sector_type)]
       ;

        move /= 2;  /* i.e. the average */

  /* conditional effects */
  if (IS_AFFECTED(ch,AFF_FLYING) || IS_AFFECTED(ch,AFF_HASTE))
      move /= 2;

  if (IS_AFFECTED(ch,AFF_SLOW))
      move *= 2;
     
    if ( is_affected(ch,gsn_earthbind) )
    {
	move +=  (ch->level /2)   ;
    }

  move += cost;

  if ( HAS_KIT(ch,"ranger") )
      move = UMIN(move,1);

  if ( !is_mounted(ch) && ch->move < move )
  {
      send_to_char( "You are too exhausted.\n\r", ch );
      return;
  }

  if ( is_mounted(ch) && ch->riding->move < move )
  {
      send_to_char("Your mount is exhausted.\n\r",ch);
      send_to_char("Your rider wishes you to move, but you are too tired.\n\r",ch->riding);
      return;
  }

  if (  in_room->sector_type == SECT_WATER_SWIM
     || to_room->sector_type == SECT_WATER_SWIM)
    {
	found = has_boat(ch);

        if (IS_SET(ch->mhs,MHS_HIGHLANDER))
	{
	   found = TRUE;
	}

	if( !found && !is_affected(ch,gsn_water_breathing) )
	{
	
	  swim = get_skill(ch,gsn_swim);

	 /*check swim skill % and do damage as necessary*/
         switch(check_immune(ch,DAM_DROWNING))
         {
	   case(IS_IMMUNE): swim = 102; break;
	   case(IS_RESISTANT): swim += swim/5; break;
	   case(IS_VULNERABLE): swim /= 2; break;
         }

	  if(number_percent() > swim-2)
	    {
	     send_to_char("You suck down some water while trying to swim.\n\r",
			  ch);
	     damage(ch,ch,(110 - swim)/6 + ch->level/3,gsn_swim,
			DAM_DROWNING,FALSE,TRUE);
	     check_improve(ch,gsn_swim,FALSE,8);
	    }
	  else
	    check_improve(ch,gsn_swim,TRUE,7);
	 }
     }
  if(!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15)
    move_time++;
  if(IS_AFFECTED(ch, AFF_SLOW))
    move_time *= 2;
  if(IS_SET(ch->affected_by_ext, AFF_EXT_BLOODY))
  {
    AFFECT_DATA *paf;
    move_time *= 2;
    for(paf = ch->affected; paf; paf = paf->next)
    {
      if(paf->where == TO_AFFECTS_EXT && paf->bitvector == AFF_EXT_BLOODY)
      {
        break;
      } 
    }
    if(paf && paf->modifier > 0 && paf->modifier <= 3)
    {
      if(ch->hit >= ch->max_hit || paf->level <= 0)
      {
        affect_strip(ch, gsn_hemorrhage);
        if ( skill_table[gsn_hemorrhage].msg_off )
        {
          send_to_char( skill_table[gsn_hemorrhage].msg_off, ch );
          send_to_char( "\n\r", ch );
        }
      }
      else
      {
        OBJ_DATA *blood_trail;
        OBJ_INDEX_DATA *blood_base;
        if ((blood_base = get_obj_index(OBJ_VNUM_BLOOD)) != NULL )
        {
          char buf[256];
          char *dirstr;
          send_to_char("Your blood leaves a trail behind you as you move.\n\r", ch);
          blood_trail = create_object(blood_base, 1, FALSE );
          clear_string(&blood_trail->name, "blood");
          clear_string(&blood_trail->short_descr, "a trail of blood");
          switch(door)
          {
            case DIR_NORTH: dirstr = "north"; break;
            case DIR_SOUTH: dirstr = "south"; break;
            case DIR_EAST: dirstr = "east"; break;
            case DIR_WEST: dirstr = "west"; break;
            case DIR_UP: dirstr = "up"; break;
            case DIR_DOWN: dirstr = "down"; break;
            default: dirstr = "nowhere"; break;
          }
          sprintf(buf, "A trail of blood leads %s.", dirstr);
          clear_string(&blood_trail->description, buf);
          if(paf->modifier > 1)
            blood_trail->timer = 2;
          else
            blood_trail->timer = 1;
          obj_to_room(blood_trail, ch->in_room);
        }
        move_time += paf->modifier;/* Prevent bad modifier values */
        paf->level -= 2;/* Limited duration */
      }
    }
  }
  WAIT_STATE(ch, move_time);
//  WAIT_STATE( ch, IS_AFFECTED(ch,AFF_SLOW) ? 2 : 1 );
  if ( mount )
      mount->move -= apply_chi(mount,move);
  else
      ch->move -= apply_chi(ch,move);


  if ( IS_AFFECTED(ch,AFF_POISON) )
  { 
      send_to_char("The {gpoison{x courses through your blood.\n\r",ch);
      damage(ch,ch,UMAX(1,move * 3),gsn_poison,DAM_POISON,FALSE,TRUE);
  }

  if ( IS_AFFECTED(ch,AFF_PLAGUE) )
  {
      send_to_char("The {Gplague{x courses through your blood.\n\r",ch);
      ch->mana--; ch->move--;
  }

    }
    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    &&   ch->invis_level < LEVEL_HERO)
    {
      if(IS_SET(ch->in_room->room_affects, RAFF_SHADED) &&
      !IS_SET(ch->affected_by_ext, AFF_EXT_SHADED))
      {
        act("$n leaves the room.",ch,NULL,NULL,TO_ROOM,FALSE);
      }
      else
      {
        if( swim == 0 )
        {
  	 if ( is_mounted(ch) )
  	 {
    act("$n leaves $t riding $N.",ch,dir_name[door],mount,TO_NOTVICT,FALSE);
  	 }
  	 else
  	 if(IS_SET(ch->mhs,MHS_SHAPEMORPHED) || (IS_SET(ch->mhs,MHS_GLADIATOR)
  	       && gladiator_info.blind == TRUE))
              act( "$l leaves $T.", ch, NULL, dir_name[door], TO_ROOM ,FALSE);
  	 else
              act( "$n leaves $T.", ch, NULL, dir_name[door], TO_ROOM ,FALSE);
        }
           else
  	 if(IS_SET(ch->mhs,MHS_SHAPEMORPHED) || (IS_SET(ch->mhs,MHS_GLADIATOR)
  	       && gladiator_info.blind == TRUE))
              act( "$l swims $T.", ch, NULL, dir_name[door], TO_ROOM ,FALSE);
  	 else
              act( "$n swims $T.", ch, NULL, dir_name[door], TO_ROOM ,FALSE);
      }
    }

    if( IS_AFFECTED(ch, AFF_SNEAK) )
    {
      for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
      {
	fch_next = fch->next_in_room;
  while(fch_next != NULL && fch_next == mount)
    fch_next = fch_next->next_in_room;// Do not ever select the mount

	if(IS_IMMORTAL(fch) && IS_SET(fch->act,PLR_HOLYLIGHT)
	   && can_see(fch,ch,FALSE))
	   act("$n sneaks $t.",ch, dir_name[door], fch, TO_VICT,FALSE);
      }
    }

    if (IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
	  fch_next = fch->next_in_room;
	  if (IS_SET(fch->mhs,MHS_HIGHLANDER) && fch != ch)
	  {
	  fHighlander = TRUE;
	  send_to_char("The tingle in your neck stops and the presence of the other Highlander is gone.\n\r",fch);
	  }
       }
    }

    /*Ogre's Smell Remorts Leaving The Room */
    if (!IS_NPC(ch) && (IS_SET(ch->act,PLR_VAMP) ||
	                IS_SET(ch->act,PLR_MUMMY) ||
	                IS_SET(ch->act,PLR_WERE) ) )  
    {
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
	  fch_next = fch->next_in_room;
	  if (fch->race == race_lookup("ogre") && fch != ch ) 
	  {
	     if (number_percent() < (fch->level + get_curr_stat(fch,STAT_CON))) 
	     {
	        if(IS_SET(ch->act,PLR_VAMP))
	           send_to_char("The strange odor of blood is gone.\n\r",fch);
	        if(IS_SET(ch->act,PLR_MUMMY))
	           send_to_char("The scent of decay is gone.\n\r",fch);
	        if(IS_SET(ch->act,PLR_WERE))
	           send_to_char("The smell of filthy fur is gone.\n\r",fch);
	     }
	  }
       }
    }


    if ( is_mounted(ch) )
    {
    char_from_room( mount );
    char_to_room( mount, to_room );
    mount->last_move = door + 1;
    do_look( mount, "auto" );
    }

    if (is_room_affected(in_room, gsn_wall_fire))
    {
	for ( paf = in_room->affected ; paf != NULL ; paf = paf_next )
   	{
          paf_next = paf->next;
	  if (paf->type == gsn_wall_fire) break;
        }
	
	if ( paf->level < ch->level + 8 )
	{
	  damage(ch,ch,dice(12,paf->level),0, DAM_FIRE, FALSE, FALSE);
	  act("$n is scorched by a wall of fire!", ch, NULL, NULL,TO_ROOM, FALSE);
	  act("You are scorched running through a wall of fire!", ch, NULL, NULL, TO_CHAR, FALSE);
	} 
    }
    char_from_room( ch );
    char_to_room( ch, to_room );
    ch->last_move = door + 1;

    if ( !IS_AFFECTED(ch, AFF_SNEAK)
    &&   ch->invis_level < LEVEL_HERO)
    {
       if ( is_mounted(ch) )
  act( "$n arrives riding $N.",ch,NULL,mount,TO_NOTVICT,FALSE);
	else
           if(IS_SET(ch->mhs,MHS_SHAPEMORPHED)
	      || (IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE))
              act( "$l has arrived.", ch, NULL, NULL, TO_ROOM ,FALSE);
	   else
              act( "$n has arrived.", ch, NULL, NULL, TO_ROOM ,FALSE);
    }

    if( IS_AFFECTED(ch, AFF_SNEAK) )
    {
      for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
      {
	fch_next = fch->next_in_room;

	if(IS_IMMORTAL(fch) && IS_SET(fch->act,PLR_HOLYLIGHT)
	   && can_see(fch,ch,FALSE))
	   act("$n sneaks into the room.",ch, NULL, fch, TO_VICT,FALSE);
      }
    }

   if ( is_affected(ch,gsn_fumble) )
      if ( number_percent() > (get_curr_stat(ch,STAT_DEX)*5) ) 
      {
	 act("$n trips and falls over!",ch,NULL,NULL,TO_ROOM,FALSE);
	 act("You trip and fall over.",ch,NULL,NULL,TO_CHAR,FALSE);
	 DAZE_STATE(ch,PULSE_PER_SECOND);
	 ch->position = POS_RESTING;
	 ch->move  = (UMAX(0,ch->move  - 5));
      }

   for ( fch = ch->in_room->people; fch != NULL; fch = fch_next )
    {
      fch_next = fch->next_in_room;

      if( is_affected(fch,skill_lookup("orb of awakening")) && !IS_AWAKE(fch) 
	  && !IS_AFFECTED(fch,AFF_SLEEP) && !IS_SET(fch->act,PLR_NOWAKE) )
       {
     act_new("$n sets off your orb.", ch,NULL,fch,TO_VICT,POS_SLEEPING,FALSE);
     do_stand(fch,"");
     blow_orb(fch,skill_lookup("orb of awakening"));
	}

      /*Ogre's Smell Remorts Entering The Room */
      if (fch->race == race_lookup("ogre") && fch != ch ) 
      {
	 if (number_percent() < (fch->level + get_curr_stat(fch,STAT_CON))) 
	 {
            if(IS_SET(ch->act,PLR_VAMP))
               send_to_char("The strange odor of blood makes you feel uncomfortable.\n\r",fch);
            if(IS_SET(ch->act,PLR_MUMMY))
               send_to_char("The scent of decay makes your head dizzy.\n\r",fch);
            if(IS_SET(ch->act,PLR_WERE))
               send_to_char("The smell of filthy fur fills up the air.\n\r",fch);
	 }
      }
     }

    do_look( ch, "auto" );

    if (in_room == to_room) /* no circular follows */
  return;

    if (IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       if (fHighlander)
       {
         send_to_char("The tingle in your neck stops and the presence of the other Highlander is gone.\n\r",ch);
         fHighlander = FALSE;
       }
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
          fch_next = fch->next_in_room;
	  if (IS_SET(fch->mhs,MHS_HIGHLANDER) && ch != fch)
	  {
	     send_to_char("Your neck tingles as you feel the presence of another Highlander.\n\r",fch);
             fHighlander = TRUE;
	  }
       }
       if (fHighlander)
       {
          send_to_char("Your neck tingles as you feel the presence of another Highlander.\n\r",ch);
       }
    }

    for ( fch = in_room->people; fch != NULL; fch = fch_next )
    {
  fch_next = fch->next_in_room;
  if(fch->riding && fch->riding == fch_next)
    fch_next = fch_next->next_in_room;/* Don't lock on to the mount next */

  if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM) 
  &&   fch->position < POS_STANDING)
      do_stand(fch,"");

  if ( fch->master == ch && fch->position == POS_STANDING 
  &&   can_see_room(fch,to_room)
  &&   is_room_clan(fch,to_room))
  {

      if (IS_SET(ch->in_room->room_flags,ROOM_LAW)
      &&  (IS_NPC(fch) && IS_SET(fch->act,ACT_AGGRESSIVE)))
      {
    act("You can't bring $N into the city.",
        ch,NULL,fch,TO_CHAR,FALSE);
    act("You aren't allowed in the city.",
        fch,NULL,NULL,TO_CHAR,FALSE);
    continue;
      }

    if ( IS_NPC(fch) && IS_SET(ch->in_room->room_flags,ROOM_NO_MOB) 
	&& IS_SET(ch->in_room->room_flags,ROOM_SAFE) )
        continue;
      act( "You follow $N.", fch, NULL, ch, TO_CHAR ,FALSE);
      move_char( fch, door, TRUE );
  }
    }

    return;
}

/*
 * Setting traps 
 *
 * Snares, Alarms, and Damage traps
 */
void do_trap( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    int chance, trap;
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument ( argument, arg );

    if ( ( chance = get_skill(ch,gsn_trap) ) == 0 
	|| ch->level < skill_level(ch,gsn_trap) )
    {
	send_to_char("You're trapped!\n\r",ch);
	return;
    }

    if ( ( obj = get_eq_char(ch,WEAR_HOLD) )   == NULL ||
	   obj->item_type != ITEM_TRAP )
    {
	send_to_char("You need something to make the trap with.\n\r",ch);
	return; 
    }

    if ( !IS_NPC(ch) && ch->pcdata->trap_timer > 0 )
    {
	send_to_char("You don't have time.\n\r",ch);
	return;
    }

    if ( ch->move - ( ch->level / 5 ) < 0 )
    {
	send_to_char("You are too exhausted.\n\r",ch);
	return;
    }

    /* a mob or a non clanner may not create traps */
    if ( IS_NPC(ch) || !is_clan(ch) )
      {
	send_to_char("You can't lay traps as a non-clanner.\n\r",ch);
	return;

      }

    if ( !str_cmp( arg, "snare" ) )
       trap = TRAP_SNARE;
    else
    if ( !str_cmp( arg, "alarm" ) )
       trap = TRAP_ALARM;
    else
    if ( !str_cmp( arg, "claw" ) )
       trap = TRAP_CLAW;
    else
    {
       send_to_char("Valid traps are: snare alarm claw\n\r",ch);
       return;
    }

    if ( check_trap( ch->in_room, trap ) )
    {
	send_to_char("That trap has already been set here.\n\r",ch);
	return;
    }

    af.where = TO_AFFECTS;
    af.type = gsn_trap;
    af.level = ch->level;
    af.location = trap; /* What kind of trap is it? */
    af.duration = number_range(ch->level/10, ch->level);
    af.modifier = obj->level;  /* level of trap */
    af.bitvector = 0;

    affect_to_room( ch->in_room, &af );
    ch->pcdata->trap_timer = number_percent( ) % 3;

    WAIT_STATE(ch,60);

    check_improve(ch,gsn_trap,TRUE,5);
    ch->move -= apply_chi( ch,ch->level /5);
    extract_obj( obj );
    send_to_char("Trap set.\n\r",ch);
    return;
}

void do_north( CHAR_DATA *ch, char *argument )
{
/*   if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
   {
   switch (number_range(0,9))
    {
    case 0:
    move_char ( ch, DIR_WEST, FALSE);
    break;
    case 1:
    move_char (ch, DIR_EAST, FALSE);
    break;
    case 2:
    move_char (ch, DIR_SOUTH, FALSE);
    break;
    case 3:
    move_char (ch, DIR_UP, FALSE);
    break;
    case 4: 
    move_char (ch, DIR_DOWN, FALSE);
    break;
    default:
    move_char( ch, DIR_NORTH, FALSE );
    }
  }
   else*/
    move_char( ch, DIR_NORTH, FALSE );
    
   return;
}



void do_east( CHAR_DATA *ch, char *argument )
{
    
/*  if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
   {
      switch (number_range(0,9))
      {
      case 0:
      move_char ( ch, DIR_WEST, FALSE);
      break;
      case 1:
      move_char (ch, DIR_NORTH, FALSE);
      break;
      case 2:
      move_char (ch, DIR_SOUTH, FALSE);
      break;
      case 3:
      move_char (ch, DIR_UP, FALSE);
      break;
      case 4:
      move_char (ch, DIR_DOWN, FALSE);
      break;
      default:
      move_char( ch, DIR_EAST, FALSE );
      }
   }
  else*/
    move_char( ch, DIR_EAST, FALSE );
    return;
}



void do_south( CHAR_DATA *ch, char *argument )
{
/*     if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
	{
	 switch (number_range(0,9))
	 {
	  case 0:
          move_char ( ch, DIR_WEST, FALSE);
	  break;
	  case 1:
	  move_char (ch, DIR_EAST, FALSE);
	  break;
	  case 2:
	  move_char (ch, DIR_NORTH, FALSE);
	  break;
	  case 3:
	  move_char (ch, DIR_UP, FALSE);
	  break;
	  case 4:
	  move_char (ch, DIR_DOWN, FALSE);
	  break;
	  default:
	  move_char( ch, DIR_SOUTH, FALSE );
	 }
        }
     else*/

        move_char( ch, DIR_SOUTH, FALSE );
        return;
}



void do_west( CHAR_DATA *ch, char *argument )
{
/*    if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
    {
     switch (number_range(0,9))
      {
	case 0:
        move_char ( ch, DIR_SOUTH, FALSE);
        break;
        case 1:
	move_char (ch, DIR_EAST, FALSE);
        break;
        case 2:
        move_char (ch, DIR_NORTH, FALSE);
	break;
        case 3:
        move_char (ch, DIR_UP, FALSE);
        break;
	case 4:
        move_char (ch, DIR_DOWN, FALSE);
        break;
        default:
	move_char( ch, DIR_WEST, FALSE );
       }
       }
    else*/
      move_char( ch, DIR_WEST, FALSE );
    
    return;
}



void do_up( CHAR_DATA *ch, char *argument )
{
/*    if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
       {
	 switch (number_range(0,9))
	 {
	  case 0:
	  move_char ( ch, DIR_WEST, FALSE);
	  break;
	  case 1:
	  move_char (ch, DIR_EAST, FALSE);
	  break;
	  case 2:
	  move_char (ch, DIR_SOUTH, FALSE);
	  break;
	  case 3:
	  move_char (ch, DIR_NORTH, FALSE);
	  break;
	  case 4:
	  move_char (ch, DIR_DOWN, FALSE);
	  break;
	  default:
	  move_char( ch, DIR_UP, FALSE );
	  }
	  }
   else*/
    move_char( ch, DIR_UP, FALSE );
    
    return;
}



void do_down( CHAR_DATA *ch, char *argument )
{
 /*   if (is_affected(ch, gsn_confusion) || (!IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 15) )
     {
      switch (number_range(0,9))
       {
	  case 0:
          move_char ( ch, DIR_WEST, FALSE);
	  break;
          case 1:
          move_char (ch, DIR_EAST, FALSE);
          break;
          case 2:
          move_char (ch, DIR_SOUTH, FALSE);
	  break;
          case 3:
	  move_char (ch, DIR_UP, FALSE);
          break;
	  case 4:
          move_char (ch, DIR_NORTH, FALSE);
	  break;
          default:
          move_char( ch, DIR_DOWN, FALSE );
       }
       }
     else*/
       move_char( ch, DIR_DOWN, FALSE );
    
    return;
}


int find_exit( CHAR_DATA *ch, char *arg )
{
    int door;

    if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = 5;
    else door = -1; 

    return door;
}

int find_door( CHAR_DATA *ch, char *arg )
{
    EXIT_DATA *pexit;
    int door, fDir = TRUE;

   if ( !str_cmp( arg, "n" ) || !str_cmp( arg, "north" ) ) door = 0;
    else if ( !str_cmp( arg, "e" ) || !str_cmp( arg, "east"  ) ) door = 1;
    else if ( !str_cmp( arg, "s" ) || !str_cmp( arg, "south" ) ) door = 2;
    else if ( !str_cmp( arg, "w" ) || !str_cmp( arg, "west"  ) ) door = 3;
    else if ( !str_cmp( arg, "u" ) || !str_cmp( arg, "up"    ) ) door = 4;
    else if ( !str_cmp( arg, "d" ) || !str_cmp( arg, "down"  ) ) door = 5;
    else
    {
   /* Type in at least 2 letters of the door's name, you can find secret doors */
  if ( strlen(arg) >= 2 ) 
	fDir = FALSE; 
  for ( door = 0; door <= 5; door++ )
  {
      if ( ( pexit = ch->in_room->exit[door] ) != NULL
      &&   IS_SET(pexit->exit_info, EX_ISDOOR)
      &&   pexit->keyword != NULL
      &&   is_name( arg, pexit->keyword ) )
    return door;
  }
  act( "I see no $T here.", ch, NULL, arg, TO_CHAR ,FALSE);
  return -1;
    }

    if ( ( pexit = ch->in_room->exit[door] ) == NULL )
    {
  act( "I see no $T here.", ch, NULL, arg, TO_CHAR ,FALSE);
  return -1;
    }

    if ( !IS_SET(pexit->exit_info, EX_ISDOOR) )
    {
  send_to_char( "You can't do that.\n\r", ch );
  return -1;
    }

    if ( IS_SET(pexit->exit_info,EX_SECRET) && fDir )
    {
  act("I see no $T here.",ch,NULL,arg,TO_CHAR,FALSE);
	return -1;
    }
    return door;
}



void do_open( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Open what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
  /* open portal */
  if (obj->item_type == ITEM_PORTAL)
  {
      if (!IS_SET(obj->value[1], EX_ISDOOR))
      {
    send_to_char("You can't do that.\n\r",ch);
    return;
      }

      if (!IS_SET(obj->value[1], EX_CLOSED))
      {
    send_to_char("It's already open.\n\r",ch);
    return;
      }

      if (IS_SET(obj->value[1], EX_LOCKED))
      {
    send_to_char("It's locked.\n\r",ch);
    return;
      }

      REMOVE_BIT(obj->value[1], EX_CLOSED);
      act("You open $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n opens $p.",ch,obj,NULL,TO_ROOM,FALSE);
      return;
  }

  if(obj->item_type == ITEM_CAPSULE)
  {
    char logbuf[255], buf[255];
    int variance = 0, xp_reward = 0, coin_gold = 0, coin_silver = 0, sp = 0;
    int reward_count = 0;
    OBJ_DATA *awarded = NULL;
    bool award_msg = FALSE;
    if(IS_NPC(ch))
    {
      send_to_char("Mobs can't open capsules!", ch);
      return;
    }
    act("You split open $p and claim its contents!",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n splits open $p and claims its contents.",ch,obj,NULL,TO_ROOM,FALSE);
    if(obj->value[4] && obj->value[4] > 0 && obj->value[4] <= 100)
    {
      variance = obj->value[4] * 100;// * 100 gives better precision
    }
    if(obj->value[0] > 0 && obj->value[0] <= 1000)
    {
      xp_reward = obj->value[0];
      reward_count--;
      if(variance)
        xp_reward = xp_reward * number_range(10000 - variance, 10000 + variance) / 10000;
      if(xp_reward)
      {
        gain_exp(ch, xp_reward);
        if(ch->pcdata->clan_info)
        {
          int tribute = 0;
          if(ch->level > 40)
          	tribute = ch->level * ch->level / 20 * (ch->level - 1) / 40 * xp_reward / 133;
          else
          	tribute = ch->level * ch->level / 20 * xp_reward / 133;
          tribute = UMAX(1, tribute);
          tribute = (tribute * number_range(95, 105)) / 100; /* A little variance */
          sprintf(buf, "You receive {W%d{x merit!\n\r", tribute);
          send_to_char(buf, ch);
          if(ch->pcdata->clan_info->clan->initiation > 0)
          {
            ch->pcdata->clan_info->clan->initiation -= tribute;
            if(ch->pcdata->clan_info->clan->initiation <= 0)
            {
              ch->pcdata->clan_info->clan->initiation = 0;
              /* Done initiation, announce it to everyone on from the clan */
              notify_clan("Your clan has completed its initiation successfully!\n\r", ch->pcdata->clan_info->clan);
              save_clan(ch, TRUE, FALSE, TRUE);
            }
          }
          else
          {
            ch->pcdata->clan_info->merit += tribute;
          }
        }
      }
    }
    if(obj->value[1] > 0 && obj->value[1] <= 110000)
    {
      coin_silver = obj->value[1];// This should be in silver
      if(variance)
        coin_silver = coin_silver * number_range(10000 - variance, 10000 + variance) / 10000;
      if(coin_silver)
      {// Gold reward -- convert it into gold and silver
        coin_gold = coin_silver / 100;
        coin_silver = coin_silver - coin_gold * 100;
        ch->gold += coin_gold;
        ch->silver += coin_silver;
        if(coin_silver)
          reward_count--;
        if(coin_gold)
          reward_count--;
      }
    }
    if(obj->value[2] > 0 && obj->value[2] <= 5)// Capping at 5 for now
    {
      sp = obj->value[2];
      ch->skill_points += sp;
      reward_count--;
    }
    // Wiznet the whole thing as a quest reward
    // buf is used to build the wiznet msg here - don't use it for other things
    sprintf(buf, "%s popped %s(%d)", ch->name,
      obj->short_descr, obj->pIndexData->vnum);
    if(xp_reward)
    {
      award_msg = TRUE;
      if(reward_count < 0)
      {
        reward_count *= -1;
        sprintf(logbuf, "You receive %d xp", xp_reward);
      }
      reward_count--;
      send_to_char(logbuf, ch);
      sprintf(logbuf, ", %d xp", xp_reward);
      strcat(buf, logbuf);
    }
    if(coin_gold)
    {
      award_msg = TRUE;
      if(reward_count < 0)
      {
        reward_count *= -1;
        sprintf(logbuf, "You receive %d gold", coin_gold);
      }
      else
      {
        if(reward_count <= 1)
          sprintf(logbuf, " and %d gold", coin_gold);
        else
          sprintf(logbuf, ", %d gold", coin_gold);
      }
      reward_count--;
      send_to_char(logbuf, ch);
      sprintf(logbuf, ", %d gold", coin_gold);
      strcat(buf, logbuf);
    }
    if(coin_silver)
    {
      award_msg = TRUE;
      if(reward_count < 0)
      {
        reward_count *= -1;
        sprintf(logbuf, "You receive %d silver", coin_silver);
      }
      else
      {
        if(reward_count <= 1)
          sprintf(logbuf, " and %d silver", coin_silver);
        else
          sprintf(logbuf, ", %d silver", coin_silver);
      }
      reward_count--;
      send_to_char(logbuf, ch);
      if(coin_gold)
        sprintf(logbuf, "/%d silver", coin_silver);
      else
        sprintf(logbuf, ", %d silver", coin_silver);
      strcat(buf, logbuf);
    }
    if(sp)
    {
      award_msg = TRUE;
      if(reward_count < 0)
      {
        reward_count *= -1;
        sprintf(logbuf, "You receive %d skill point%s", sp, sp==1?"":"s");
      }
      else
      {
        if(reward_count <= 1)
          sprintf(logbuf, " and %d skill point%s", sp, sp==1?"":"s");
        else
          sprintf(logbuf, ", %d skill point%s", sp, sp==1?"":"s");
      }
      reward_count--;
      send_to_char(logbuf, ch);
      sprintf(logbuf, ", %d skill", sp);
      strcat(buf, logbuf);
    }
    if(award_msg)
      send_to_char("!\n\r", ch);
    if(obj->value[3] > 0)
    {// Item to receive
      OBJ_INDEX_DATA *award_base;
      award_base = get_obj_index(obj->value[3]);
      if(award_base)
      {
        if(award_base->item_type != ITEM_CAPSULE)
        {
        	awarded = create_object(award_base,0,FALSE);
          obj_to_char(awarded,ch);
          act("You pull $p out!", ch, awarded, NULL, TO_CHAR, FALSE);
          act("$n pulls $p out!", ch, awarded, NULL, TO_ROOM, FALSE);
        }
      }
      else
      {
        sprintf(logbuf, "Error: Bad award vnum %d in capsule %d.",
          obj->value[3], obj->pIndexData->vnum);
        log_string(logbuf);
      }
    }
    if(awarded)
    {
      sprintf(logbuf, ", %s(%d) received", awarded->short_descr,
        awarded->pIndexData->vnum);
      strcat(buf, logbuf);
    }
    else if(!award_msg)
    {
      send_to_char("How disappointing. You find nothing inside.\n\r", ch);
      strcat(buf, " but found nothing inside it.");
    }
    wiznet(buf, NULL, NULL, WIZ_DEITYFAVOR, 0, 0);
    extract_obj(obj);
    return;
  }

  /* 'open object' */
  if ( obj->item_type != ITEM_CONTAINER)
      { send_to_char( "That's not a container.\n\r", ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSED) )
      { send_to_char( "It's already open.\n\r",      ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
      { send_to_char( "You can't do that.\n\r",      ch ); return; }
  if ( IS_SET(obj->value[1], CONT_LOCKED) )
      { send_to_char( "It's locked.\n\r",            ch ); return; }

  REMOVE_BIT(obj->value[1], CONT_CLOSED);
  act("You open $p.",ch,obj,NULL,TO_CHAR,FALSE);
  act( "$n opens $p.", ch, obj, NULL, TO_ROOM ,FALSE);
  return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
  /* 'open door' */
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  EXIT_DATA *pexit_rev;

  pexit = ch->in_room->exit[door];
  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
      { send_to_char( "It's already open.\n\r",      ch ); return; }
  if (  IS_SET(pexit->exit_info, EX_LOCKED) )
      { send_to_char( "It's locked.\n\r",            ch ); return; }

  REMOVE_BIT(pexit->exit_info, EX_CLOSED);
  act( "$n opens the $d.", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);
  send_to_char( "Ok.\n\r", ch );

  /* open the other side */
  if ( ( to_room   = pexit->u1.to_room            ) != NULL
  &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
  &&   pexit_rev->u1.to_room == ch->in_room )
  {
      CHAR_DATA *rch;

      REMOVE_BIT( pexit_rev->exit_info, EX_CLOSED );
      for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
    act( "The $d opens.", rch, NULL, pexit_rev->keyword, TO_CHAR ,FALSE);
  }
    }

    return;
}



void do_close( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }


    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  send_to_char( "Close what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
  /* portal stuff */
  if (obj->item_type == ITEM_PORTAL)
  {

      if (!IS_SET(obj->value[1],EX_ISDOOR)
      ||   IS_SET(obj->value[1],EX_NOCLOSE))
      {
    send_to_char("You can't do that.\n\r",ch);
    return;
      }

      if (IS_SET(obj->value[1],EX_CLOSED))
      {
    send_to_char("It's already closed.\n\r",ch);
    return;
      }

      SET_BIT(obj->value[1],EX_CLOSED);
      act("You close $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n closes $p.",ch,obj,NULL,TO_ROOM,FALSE);
      return;
  }

  /* 'close object' */
  if ( obj->item_type != ITEM_CONTAINER )
      { send_to_char( "That's not a container.\n\r", ch ); return; }
  if ( IS_SET(obj->value[1], CONT_CLOSED) )
      { send_to_char( "It's already closed.\n\r",    ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSEABLE) )
      { send_to_char( "You can't do that.\n\r",      ch ); return; }

  SET_BIT(obj->value[1], CONT_CLOSED);
  act("You close $p.",ch,obj,NULL,TO_CHAR,FALSE);
  act( "$n closes $p.", ch, obj, NULL, TO_ROOM ,FALSE);
  return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
  /* 'close door' */
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  EXIT_DATA *pexit_rev;

  pexit = ch->in_room->exit[door];
  if ( IS_SET(pexit->exit_info, EX_CLOSED) )
      { send_to_char( "It's already closed.\n\r",    ch ); return; }

  SET_BIT(pexit->exit_info, EX_CLOSED);
  act( "$n closes the $d.", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);
  send_to_char( "Ok.\n\r", ch );

  /* close the other side */
  if ( ( to_room   = pexit->u1.to_room            ) != NULL
  &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
  &&   pexit_rev->u1.to_room == ch->in_room )
  {
      CHAR_DATA *rch;

      SET_BIT( pexit_rev->exit_info, EX_CLOSED );
      for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
    act( "The $d closes.", rch, NULL, pexit_rev->keyword, TO_CHAR ,FALSE);
  }
    }

    return;
}



bool has_key( CHAR_DATA *ch, int key )
{
    OBJ_DATA *obj;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if ( obj->pIndexData->vnum == key )
      return TRUE;
    }

    return FALSE;
}



void do_lock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Lock what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
  /* portal stuff */
  if (obj->item_type == ITEM_PORTAL)
  {
      if (!IS_SET(obj->value[1],EX_ISDOOR)
      ||  IS_SET(obj->value[1],EX_NOCLOSE))
      {
    send_to_char("You can't do that.\n\r",ch);
    return;
      }
      if (!IS_SET(obj->value[1],EX_CLOSED))
      {
    send_to_char("It's not closed.\n\r",ch);
    return;
      }

      if (obj->value[4] < 0 || IS_SET(obj->value[1],EX_NOLOCK))
      {
    send_to_char("It can't be locked.\n\r",ch);
    return;
      }

      if (!has_key(ch,obj->value[4]))
      {
    send_to_char("You lack the key.\n\r",ch);
    return;
      }

      if (IS_SET(obj->value[1],EX_LOCKED))
      {
    send_to_char("It's already locked.\n\r",ch);
    return;
      }

      SET_BIT(obj->value[1],EX_LOCKED);
      act("You lock $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n locks $p.",ch,obj,NULL,TO_ROOM,FALSE);
      return;
  }

  /* 'lock object' */
  if ( obj->item_type != ITEM_CONTAINER )
      { send_to_char( "That's not a container.\n\r", ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSED) )
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( obj->value[2] < 0 )
      { send_to_char( "It can't be locked.\n\r",     ch ); return; }
  if ( !has_key( ch, obj->value[2] ) )
      { send_to_char( "You lack the key.\n\r",       ch ); return; }
  if ( IS_SET(obj->value[1], CONT_LOCKED) )
      { send_to_char( "It's already locked.\n\r",    ch ); return; }

  SET_BIT(obj->value[1], CONT_LOCKED);
  act("You lock $p.",ch,obj,NULL,TO_CHAR,FALSE);
  act( "$n locks $p.", ch, obj, NULL, TO_ROOM ,FALSE);
  return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
  /* 'lock door' */
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  EXIT_DATA *pexit_rev;

  pexit = ch->in_room->exit[door];
  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( pexit->key < 0 )
      { send_to_char( "It can't be locked.\n\r",     ch ); return; }
  if ( !has_key( ch, pexit->key) )
      { send_to_char( "You lack the key.\n\r",       ch ); return; }
  if ( IS_SET(pexit->exit_info, EX_LOCKED) )
      { send_to_char( "It's already locked.\n\r",    ch ); return; }

  SET_BIT(pexit->exit_info, EX_LOCKED);
  send_to_char( "*Click*\n\r", ch );
  act( "$n locks the $d.", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);

  /* lock the other side */
  if ( ( to_room   = pexit->u1.to_room            ) != NULL
  &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != 0
  &&   pexit_rev->u1.to_room == ch->in_room )
  {
      SET_BIT( pexit_rev->exit_info, EX_LOCKED );
  }
    }

    return;
}



void do_unlock( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int door;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }



    if ( arg[0] == '\0' )
    {
  send_to_char( "Unlock what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
  /* portal stuff */
  if (obj->item_type == ITEM_PORTAL)
  {
      if (IS_SET(obj->value[1],EX_ISDOOR))
      {
    send_to_char("You can't do that.\n\r",ch);
    return;
      }

      if (!IS_SET(obj->value[1],EX_CLOSED))
      {
    send_to_char("It's not closed.\n\r",ch);
    return;
      }

      if (obj->value[4] < 0)
      {
    send_to_char("It can't be unlocked.\n\r",ch);
    return;
      }

      if (!has_key(ch,obj->value[4]))
      {
    send_to_char("You lack the key.\n\r",ch);
    return;
      }

      if (!IS_SET(obj->value[1],EX_LOCKED))
      {
    send_to_char("It's already unlocked.\n\r",ch);
    return;
      }

      REMOVE_BIT(obj->value[1],EX_LOCKED);
      act("You unlock $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n unlocks $p.",ch,obj,NULL,TO_ROOM,FALSE);
      return;
  }

  /* 'unlock object' */
  if ( obj->item_type != ITEM_CONTAINER )
      { send_to_char( "That's not a container.\n\r", ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSED) )
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( obj->value[2] < 0 )
      { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
  if ( !has_key( ch, obj->value[2] ) )
      { send_to_char( "You lack the key.\n\r",       ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_LOCKED) )
      { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

  REMOVE_BIT(obj->value[1], CONT_LOCKED);
  act("You unlock $p.",ch,obj,NULL,TO_CHAR,FALSE);
  act( "$n unlocks $p.", ch, obj, NULL, TO_ROOM ,FALSE);
  return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
  /* 'unlock door' */
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  EXIT_DATA *pexit_rev;

  pexit = ch->in_room->exit[door];
  if ( !IS_SET(pexit->exit_info, EX_CLOSED) )
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( pexit->key < 0 )
      { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
  if ( !has_key( ch, pexit->key) )
      { send_to_char( "You lack the key.\n\r",       ch ); return; }
  if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
      { send_to_char( "It's already unlocked.\n\r",  ch ); return; }

  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
  send_to_char( "*Click*\n\r", ch );
  act( "$n unlocks the $d.", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);

  /* unlock the other side */
  if ( ( to_room   = pexit->u1.to_room            ) != NULL
  &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
  &&   pexit_rev->u1.to_room == ch->in_room )
  {
      REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
  }
    }

    return;
}



void do_pick( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    OBJ_DATA *obj;
    int door;

    if ( IS_NPC(ch) && ch->desc == NULL)
    return;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  send_to_char( "Pick what?\n\r", ch );
  return;
    }

    if ( ch->move < (ch->level/15) )
    {   
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    WAIT_STATE( ch, skill_table[gsn_pick_lock].beats );

    /* look for guards */
    for ( gch = ch->in_room->people; gch; gch = gch->next_in_room )
    {
  if ( IS_NPC(gch) && IS_AWAKE(gch) && ch->level + 5 < gch->level )
  {
      act( "$N is standing too close to the lock.",
    ch, NULL, gch, TO_CHAR ,FALSE);
      return;
  }
    }

    if ( !IS_NPC(ch) && number_percent( ) > get_skill(ch,gsn_pick_lock))
    {
  send_to_char( "You failed.\n\r", ch);
  check_improve(ch,gsn_pick_lock,FALSE,2);
  return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {
  /* portal stuff */
  if (obj->item_type == ITEM_PORTAL)
  {
      if (!IS_SET(obj->value[1],EX_ISDOOR))
      { 
    send_to_char("You can't do that.\n\r",ch);
    return;
      }

      if (!IS_SET(obj->value[1],EX_CLOSED))
      {
    send_to_char("It's not closed.\n\r",ch);
    return;
      }

      if (obj->value[4] < 0)
      {
    send_to_char("It can't be unlocked.\n\r",ch);
    return;
      }

      if (IS_SET(obj->value[1],EX_PICKPROOF))
      {
    send_to_char("You failed.\n\r",ch);
    return;
      }

      REMOVE_BIT(obj->value[1],EX_LOCKED);
      act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM,FALSE);
      check_improve(ch,gsn_pick_lock,TRUE,2);
      return;
  }

      


  
  /* 'pick object' */
  if ( obj->item_type != ITEM_CONTAINER )
      { send_to_char( "That's not a container.\n\r", ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_CLOSED) )
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( obj->value[2] < 0 )
      { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
  if ( !IS_SET(obj->value[1], CONT_LOCKED) )
      { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
  if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
      { send_to_char( "You failed.\n\r",             ch ); return; }

  REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You pick the lock on $p.",ch,obj,NULL,TO_CHAR,FALSE);
        act("$n picks the lock on $p.",ch,obj,NULL,TO_ROOM,FALSE);
  check_improve(ch,gsn_pick_lock,TRUE,2);
  return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
  /* 'pick door' */
  ROOM_INDEX_DATA *to_room;
  EXIT_DATA *pexit;
  EXIT_DATA *pexit_rev;

  pexit = ch->in_room->exit[door];
  if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
      { send_to_char( "It's not closed.\n\r",        ch ); return; }
  if ( pexit->key < 0 && !IS_IMMORTAL(ch))
      { send_to_char( "It can't be picked.\n\r",     ch ); return; }
  if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
      { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
  if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
      { send_to_char( "You failed.\n\r",             ch ); return; }

  REMOVE_BIT(pexit->exit_info, EX_LOCKED);
  send_to_char( "*Click*\n\r", ch );
  act( "$n picks the $d.", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);
  check_improve(ch,gsn_pick_lock,TRUE,2);

  /* pick the other side */
  if ( ( to_room   = pexit->u1.to_room            ) != NULL
  &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
  &&   pexit_rev->u1.to_room == ch->in_room )
  {
      REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
  }
    }

    return;
}




void do_stand( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;
    OBJ_DATA *obj_on = ch->on;

    if ( is_affected(ch,gsn_trap) )
    {
	send_to_char("You are held fast by a snare trap.\n\r",ch);
	return;
    }
  if ( is_affected(ch,skill_lookup("hold person")))
    {
        send_to_char("Your muscles are frozen!\n\r",ch);
        return;
    }

    if (argument[0] != '\0')
    {
  if (ch->position == POS_FIGHTING)
  {
      send_to_char("Maybe you should finish fighting first?\n\r",ch);
      return;
  }
  obj = get_obj_list(ch,argument,ch->in_room->contents);
  if (obj == NULL)
  {
      send_to_char("You don't see that here.\n\r",ch);
      return;
  }
  if (obj->item_type != ITEM_FURNITURE
  ||  (!IS_SET(obj->value[2],STAND_AT)
  &&   !IS_SET(obj->value[2],STAND_ON)
  &&   !IS_SET(obj->value[2],STAND_IN)))
  {
      send_to_char("You can't seem to find a place to stand.\n\r",ch);
      return;
  }
  if (ch->on != obj && count_users(obj) >= obj->value[0])
  {
      act_new("There's no room to stand on $p.",
    ch,obj,NULL,TO_ROOM,POS_DEAD,FALSE);
      return;
  }
    }
    
    switch ( ch->position )
    {
    case POS_SLEEPING:
  if ( IS_AFFECTED(ch, AFF_SLEEP) )
      { send_to_char( "You can't wake up!\n\r", ch ); return; }
  
  if (obj == NULL)
  {
      send_to_char( "You wake and stand up.\n\r", ch );
      act( "$n wakes and stands up.", ch, NULL, NULL, TO_ROOM ,FALSE);
      ch->on = NULL;
  }
  else if (IS_SET(obj->value[2],STAND_AT))
  {
     act_new("You wake and stand at $p.",ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
     act("$n wakes and stands at $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  else if (IS_SET(obj->value[2],STAND_ON))
  {
      act_new("You wake and stand on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
      act("$n wakes and stands on $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  else 
  {
      act_new("You wake and stand in $p.",ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
      act("$n wakes and stands in $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  ch->position = POS_STANDING;
  do_look(ch,"auto");
  break;

    case POS_RESTING: case POS_SITTING:
  if (obj == NULL)
  {
      send_to_char( "You stand up.\n\r", ch );
      act( "$n stands up.", ch, NULL, NULL, TO_ROOM ,FALSE);
      ch->on = NULL;
  }
  else if (IS_SET(obj->value[2],STAND_AT))
  {
      act("You stand at $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n stands at $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  else if (IS_SET(obj->value[2],STAND_ON))
  {
      act("You stand on $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n stands on $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  else
  {
      act("You stand in $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n stands on $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  ch->position = POS_STANDING;
  break;

    case POS_STANDING:
  send_to_char( "You are already standing.\n\r", ch );
  break;

    case POS_FIGHTING:
  send_to_char( "You are already fighting!\n\r", ch );
  break;
    }

    if (obj_on !=NULL && CAN_WEAR(obj_on,ITEM_TAKE) && !is_clan(ch))
       get_obj(ch,obj_on,NULL);

    return;
}



void do_rest( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;
    
    if ( IS_AFFECTED(ch, AFF_SLEEP) ) {
      send_to_char ("You can't wake up!\n\r",ch);
      return;
    }

    if ( is_mounted(ch) ) {
	send_to_char("Dismount first.\n\r",ch);
	return;
    }

    if (ch->position == POS_FIGHTING)
    {
  send_to_char("You are already fighting!\n\r",ch);
  return;
    }

      if (IS_AFFECTED(ch,AFF_CHARM) && str_cmp(kludge_string,"order"))
	  {
	    send_to_char("Did you master tell you to do that?\n\r",ch);
	    return;
	  }

    /* okay, now that we know we can rest, find an object to rest on */
    if (argument[0] != '\0')
    {
  obj = get_obj_list(ch,argument,ch->in_room->contents);
  if (obj == NULL)
  {
      send_to_char("You don't see that here.\n\r",ch);
      return;
  }
    }
    else obj = ch->on;

    if (obj != NULL)
    {
        if (!IS_SET(obj->item_type,ITEM_FURNITURE) 
      ||  (!IS_SET(obj->value[2],REST_ON)
      &&   !IS_SET(obj->value[2],REST_IN)
      &&   !IS_SET(obj->value[2],REST_AT)))
      {
      send_to_char("You can't rest on that.\n\r",ch);
      return;
      }

        if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
        {
      act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
      return;
      }
  
  ch->on = obj;
    }

    switch ( ch->position )
    {
    case POS_SLEEPING:
  if (obj == NULL)
  {
      send_to_char( "You wake up and start resting.\n\r", ch );
      act("$n wakes up and starts resting.",ch,NULL,NULL,TO_ROOM,FALSE);
  }
  else if (IS_SET(obj->value[2],REST_AT))
  {
      act_new("You wake up and rest at $p.",
        ch,obj,NULL,TO_CHAR,POS_SLEEPING,FALSE);
      act("$n wakes up and rests at $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
        else if (IS_SET(obj->value[2],REST_ON))
        {
            act_new("You wake up and rest on $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING,FALSE);
            act("$n wakes up and rests on $p.",ch,obj,NULL,TO_ROOM,FALSE);
        }
        else
        {
            act_new("You wake up and rest in $p.",
                    ch,obj,NULL,TO_CHAR,POS_SLEEPING,FALSE);
            act("$n wakes up and rests in $p.",ch,obj,NULL,TO_ROOM,FALSE);
        }
  ch->position = POS_RESTING;
  break;

    case POS_RESTING:
  send_to_char( "You are already resting.\n\r", ch );
  break;

    case POS_STANDING:
  if (obj == NULL)
  {
      send_to_char( "You rest.\n\r", ch );
      act( "$n sits down and rests.", ch, NULL, NULL, TO_ROOM ,FALSE);
  }
        else if (IS_SET(obj->value[2],REST_AT))
        {
      act("You sit down at $p and rest.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n sits down at $p and rests.",ch,obj,NULL,TO_ROOM,FALSE);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
      act("You sit on $p and rest.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n sits on $p and rests.",ch,obj,NULL,TO_ROOM,FALSE);
        }
        else
        {
      act("You rest in $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n rests in $p.",ch,obj,NULL,TO_ROOM,FALSE);
        }
  ch->position = POS_RESTING;
  break;

    case POS_SITTING:
  if (obj == NULL)
  {
      send_to_char("You rest.\n\r",ch);
      act("$n rests.",ch,NULL,NULL,TO_ROOM,FALSE);
  }
        else if (IS_SET(obj->value[2],REST_AT))
        {
      act("You rest at $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n rests at $p.",ch,obj,NULL,TO_ROOM,FALSE);
        }
        else if (IS_SET(obj->value[2],REST_ON))
        {
      act("You rest on $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n rests on $p.",ch,obj,NULL,TO_ROOM,FALSE);
        }
        else
        {
      act("You rest in $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("$n rests in $p.",ch,obj,NULL,TO_ROOM,FALSE);
  }
  ch->position = POS_RESTING;
  break;
    }


    return;
}


void do_sit (CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;
   
    if ( is_mounted(ch) ) {
	send_to_char("Dismount first.\n\r",ch);
	return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) ) {
      send_to_char ("You can't wake up!\n\r",ch);
      return;                          
    }

    if (ch->position == POS_FIGHTING)
    {
  send_to_char("Maybe you should finish this fight first?\n\r",ch);
  return;
    }

      if (IS_AFFECTED(ch,AFF_CHARM) && str_cmp(kludge_string,"order"))
	  {
	    send_to_char("Did you master tell you to do that?\n\r",ch);
	    return;
	  }

    /* okay, now that we know we can sit, find an object to sit on */
    if (argument[0] != '\0')
    {
  obj = get_obj_list(ch,argument,ch->in_room->contents);
  if (obj == NULL)
  {
      send_to_char("You don't see that here.\n\r",ch);
      return;
  }
    }
    else obj = ch->on;

    if (obj != NULL)                                                              
    {
  if (!IS_SET(obj->item_type,ITEM_FURNITURE)
  ||  (!IS_SET(obj->value[2],SIT_ON)
  &&   !IS_SET(obj->value[2],SIT_IN)
  &&   !IS_SET(obj->value[2],SIT_AT)))
  {
      send_to_char("You can't sit on that.\n\r",ch);
      return;
  }

  if (obj != NULL && ch->on != obj && count_users(obj) >= obj->value[0])
  {
      act_new("There's no more room on $p.",ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
      return;
  }

  ch->on = obj;
    }
    switch (ch->position)
    {
  case POS_SLEEPING:
            if (obj == NULL)
            {
              send_to_char( "You wake and sit up.\n\r", ch );
              act( "$n wakes and sits up.", ch, NULL, NULL, TO_ROOM ,FALSE);
            }
            else if (IS_SET(obj->value[2],SIT_AT))
            {
              act_new("You wake and sit at $p.",
		ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
              act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM,FALSE);
            }
            else if (IS_SET(obj->value[2],SIT_ON))
            {
              act_new("You wake and sit on $p.",
		ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
              act("$n wakes and sits at $p.",ch,obj,NULL,TO_ROOM,FALSE);
            }
            else
            {
              act_new("You wake and sit in $p.",
		ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
              act("$n wakes and sits in $p.",ch,obj,NULL,TO_ROOM,FALSE);
            }

      ch->position = POS_SITTING;
      break;
  case POS_RESTING:
      if (obj == NULL)
    send_to_char("You stop resting.\n\r",ch);
      else if (IS_SET(obj->value[2],SIT_AT))
      {
    act("You sit at $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n sits at $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }

      else if (IS_SET(obj->value[2],SIT_ON))
      {
    act("You sit on $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n sits on $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      ch->position = POS_SITTING;
      break;
  case POS_SITTING:
      send_to_char("You are already sitting down.\n\r",ch);
      break;
  case POS_STANDING:
      if (obj == NULL)
          {
    send_to_char("You sit down.\n\r",ch);
              act("$n sits down on the ground.",ch,NULL,NULL,TO_ROOM,FALSE);
      }
      else if (IS_SET(obj->value[2],SIT_AT))
      {
    act("You sit down at $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n sits down at $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      else if (IS_SET(obj->value[2],SIT_ON))
      {
    act("You sit on $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n sits on $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      else
      {
    act("You sit down in $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n sits down in $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
          ch->position = POS_SITTING;
          break;
    }
    return;
}


void do_sleep( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj = NULL;

    if ( is_mounted(ch) ) {
	send_to_char("Dismount first.\n\r",ch);
	return;
    }

    switch ( ch->position )
    {
    case POS_SLEEPING:
  send_to_char( "You are already sleeping.\n\r", ch );
  break;

    case POS_RESTING:
    case POS_SITTING:
    case POS_STANDING: 
      if (IS_AFFECTED(ch,AFF_CHARM) && str_cmp(kludge_string,"order"))
	  {
	    send_to_char("Did you master tell you to do that?\n\r",ch);
	    return;
	  }

  if (argument[0] == '\0' && ch->on == NULL)
  {
      send_to_char( "You go to sleep.\n\r", ch );
      act( "$n goes to sleep.", ch, NULL, NULL, TO_ROOM ,FALSE);
      ch->position = POS_SLEEPING;
  }
  else  /* find an object and sleep on it */
  {
      if (argument[0] == '\0')
    obj = ch->on;
      else
        obj = get_obj_list( ch, argument,  ch->in_room->contents );

      if (obj == NULL)
      {
    send_to_char("You don't see that here.\n\r",ch);
    return;
      }
      if (obj->item_type != ITEM_FURNITURE
      ||  (!IS_SET(obj->value[2],SLEEP_ON) 
      &&   !IS_SET(obj->value[2],SLEEP_IN)
      &&   !IS_SET(obj->value[2],SLEEP_AT)))
      {
    send_to_char("You can't sleep on that!\n\r",ch);
    return;
      }

      if (ch->on != obj && count_users(obj) >= obj->value[0])
      {
    act_new("There is no room on $p for you.",
        ch,obj,NULL,TO_CHAR,POS_DEAD,FALSE);
    return;
      }

      ch->on = obj;
      if (IS_SET(obj->value[2],SLEEP_AT))
      {
    act("You go to sleep at $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n goes to sleep at $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      else if (IS_SET(obj->value[2],SLEEP_ON))
      {
          act("You go to sleep on $p.",ch,obj,NULL,TO_CHAR,FALSE);
          act("$n goes to sleep on $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      else
      {
    act("You go to sleep in $p.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$n goes to sleep in $p.",ch,obj,NULL,TO_ROOM,FALSE);
      }
      ch->position = POS_SLEEPING;
  }
  break;

    case POS_FIGHTING:
  send_to_char( "You are already fighting!\n\r", ch );
  break;
    }

    return;
}



void do_wake( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
  { do_stand( ch, argument ); return; }

    if ( !IS_AWAKE(ch) )
  { send_to_char( "You are asleep yourself!\n\r",       ch ); return; }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
  { send_to_char( "They aren't here.\n\r",              ch ); return; }

    if ( IS_AWAKE(victim) )
  { act( "$N is already awake.", ch, NULL, victim, TO_CHAR ,FALSE); return; }

    if ( IS_AFFECTED(victim, AFF_SLEEP) 
	|| (IS_SET(victim->act,PLR_NOWAKE) && !IS_IMMORTAL(ch)))
  { act( "You can't wake $M!",   ch, NULL, victim, TO_CHAR ,FALSE);  return; }

    act_new( "$n wakes you.", ch, NULL, victim, TO_VICT,POS_SLEEPING,FALSE );
    do_stand(victim,"");
    return;
}



void do_sneak( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;

    if ( IS_AFFECTED(ch,AFF_FAERIE_FOG)){
	send_to_char("You're too easy to see!\n\r",ch);
		return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char("Everyone is too alert in the Arena.\n\r",ch);
       return;
    }

    if ( ch->move < (ch->level/15) )
    {   
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if (IS_AFFECTED(ch,AFF_SNEAK))
	{
	send_to_char( "You attempt to move silently.\n\r", ch );
  	return;
	}

    send_to_char( "You attempt to move silently.\n\r", ch );
    affect_strip( ch, gsn_sneak );

    if ( number_percent( ) < get_skill(ch,gsn_sneak))
    {
  check_improve(ch,gsn_sneak,TRUE,3);
  check_improve(ch,gsn_ninjitsu,TRUE,3);
  af.where     = TO_AFFECTS;
  af.type      = gsn_sneak;
  af.level     = ch->level; 
  af.duration  = ch->level;
  af.location  = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = AFF_SNEAK;
  affect_to_char( ch, &af );
    }
/*    else
  check_improve(ch,gsn_sneak,FALSE,3);
*/

    return;
}



void do_hide( CHAR_DATA *ch, char *argument )
{
    if ( IS_AFFECTED(ch,AFF_FAERIE_FOG)){
	send_to_char("You're too easy to see!\n\r",ch);
		return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char("Nothing to hide behind in the Arena.\n\r",ch);
       return;
    }

    if ( ch->move < (ch->level/10) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/10));

    send_to_char( "You attempt to hide.\n\r", ch );

    if ( IS_AFFECTED(ch, AFF_FAERIE_FOG) )
	return;

    if ( IS_AFFECTED(ch, AFF_HIDE) )
	return;

    if ( number_percent( ) < get_skill(ch,gsn_hide)/
    HAS_KIT(ch,"ninja") ? 2 : 3)
    {
  SET_BIT(ch->affected_by, AFF_HIDE);
  check_improve(ch,gsn_hide,TRUE,3);
  check_improve(ch,gsn_ninjitsu,TRUE,3);
    WAIT_STATE(ch,skill_table[gsn_hide].beats);
    }
    else
  {
  check_improve(ch,gsn_hide,FALSE,3);
  check_improve(ch,gsn_ninjitsu,FALSE,3);
  WAIT_STATE(ch,skill_table[gsn_hide].beats);
  }

    return;
}



/*
 * Contributed by Alander.
 */
void do_visible( CHAR_DATA *ch, char *argument )
{
    affect_strip ( ch, gsn_invis      );
    affect_strip ( ch, gsn_mass_invis     );
    affect_strip ( ch, gsn_sneak      );
    affect_strip ( ch, gsn_hide      );
    REMOVE_BIT   ( ch->affected_by, AFF_HIDE    );
    REMOVE_BIT   ( ch->affected_by, AFF_INVISIBLE );
    REMOVE_BIT   ( ch->affected_by, AFF_SNEAK   );
    send_to_char( "Ok.\n\r", ch );
    return;
}


void do_recall( CHAR_DATA *ch, char *argument )
{
  int timer = 5;
  if(IS_SET(ch->mhs, MHS_GLADIATOR))
  {
    send_to_char("Gladiators don't recall.\n\r", ch);
    return;
  }
  if(IS_NPC(ch) || ((ch->pcdata->clan_info == NULL || ch->pcdata->clan_info->clan->hall == NULL) && ch->level <= 10))
  {
    if(recall ( ch, argument, FALSE ) || ch->fighting != NULL)
      return;
    if(IS_NPC(ch))
      return;
/* Fall back to the new recall if the old one fails */
  }
  if ( is_affected(ch,gsn_trap) )
  {
    send_to_char("You are held fast by a snare trap.\n\r",ch);
    return;
  }
  if( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
  {
    send_to_char("Your master didn't tell you to do that.\n\r",ch);
    return;
  }
  if(IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL))
  {
    send_to_char("A powerful force blocks your ability to recall here.\n\r", ch);
    return;
    //timer += 10;
  }
  else if(ch->in_room->area->no_transport)
    timer += 10;
  if(IS_AFFECTED(ch, AFF_CURSE) || is_affected(ch, gsn_morph))
    timer += 5;
  if(ch->pcdata->quit_time > 0)
    timer += 10;
  if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->hall)
    send_to_char("You focus on recalling to your hall.\n\r", ch);
  else
    send_to_char("You focus on recalling.\n\r", ch);
  ch->pcdata->pulse_type = PULSE_RECALL;
  ch->pcdata->pulse_timer = timer * 12;
  return;
}

bool recall( CHAR_DATA *ch, char *argument, bool fPray )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    ROOM_INDEX_DATA *location;
    int loc_hold;

    if (IS_NPC(ch) && !IS_SET(ch->act,ACT_PET))
    {
  send_to_char("Only players can recall.\n\r",ch);
  return FALSE;
    }

    if (( ch->in_room->area->no_transport ) || ( ch->in_room->clan && ch->in_room->clan != ch->clan))

    {
	send_to_char("Your deity cannot hear your plea from here.\n\r",ch);
	return FALSE;
    }

    if( !IS_NPC(ch) 
	&& ch->level > 10 
	&& !fPray 
	&& !IS_IMMORTAL(ch))
    {
	send_to_char("Sorry, no free recalls anymore.\n\r",ch);
	return FALSE;
    }

    act( "$n prays for transportation!", ch, 0, 0, TO_ROOM ,FALSE);

  /*
    if( ch->level < 6 )
	loc_hold = ROOM_VNUM_TEMPLE;
    else
	loc_hold = (pc_race_table[ch->race].color ? ROOM_VNUM_TEMPLE 
			: ROOM_VNUM_NTFOUNTAIN);
   */
    loc_hold = ROOM_VNUM_TEMPLE;

    location = get_room_index( loc_hold );
    if ( location  == NULL )
    {
  send_to_char( "You are completely lost.\n\r", ch );
  return FALSE;
    }

    if ( ch->in_room == location )
  return TRUE;

    if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_AFFECTED(ch, AFF_CURSE) 
    ||   is_affected(ch,gsn_morph)
    ||   IS_SET(ch->act, PLR_DWEEB) )
    {
  if(!IS_NPC(ch))
  {
    sprintf(buf,"%s has forsaken you.\n\r",
	deity_table[ch->pcdata->deity].pname);
    send_to_char(buf,ch);
  }
  return FALSE;
    }

    if ( ( victim = ch->fighting ) != NULL )
    {
  int lose,skill;

  skill = get_skill(ch,gsn_recall);

  if ( number_percent() > 80 * skill / 100 )
  {
      check_improve(ch,gsn_recall,FALSE,6);
      WAIT_STATE( ch, 4 );
      sprintf( buf, "You failed!.\n\r");
      send_to_char( buf, ch );
      return FALSE;
  }

  lose = (ch->desc != NULL) ? 25 : 50;
  gain_exp( ch, 0 - lose );
  check_improve(ch,gsn_recall,TRUE,4);
  sprintf( buf, "You recall from combat!  You lose %d exps.\n\r", lose );
  send_to_char( buf, ch );
  stop_fighting( ch, TRUE );
  
    }

   // ch->move *= 50;
   // ch->move /= apply_chi(ch,100);
      ch->move -= apply_chi(ch, ch->move/2);
    act( "$n disappears.", ch, NULL, NULL, TO_ROOM ,FALSE);
    char_from_room( ch );
    char_to_room( ch, location );
    clear_mount( ch );
    act( "$n appears in the room.", ch, NULL, NULL, TO_ROOM ,FALSE);
    do_look( ch, "auto" );
    
    if (ch->pet != NULL)
  do_recall(ch->pet,"");

    return TRUE;
}



void do_train( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    bool htrain = FALSE;
    sh_int stat = - 1;
    char *pOutput = NULL;
    int cost;

    if ( IS_NPC(ch) )
  return;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }


    /*
     * Check for trainer.
     */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
  if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
      break;
    }

    if ( mob == NULL )
    {
  send_to_char( "You can't do that here.\n\r", ch );
  return;
    }

    if ( argument[0] == '\0' )
    {
  sprintf( buf, "You have %d training sessions.\n\r", ch->train );
  send_to_char( buf, ch );
  if(ch->pcdata->half_train > 0)
  {
    sprintf( buf, "You have %d half training sessions. These will be used first.\n\r", ch->pcdata->half_train );
    send_to_char( buf, ch );
  }

  argument = "foo";
    }

    if (IS_SET(ch->mhs,MHS_SHAPESHIFTED) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
    {
  sprintf( buf, "You are not allowed to train while shapeshifted.\n\r");
  send_to_char( buf,ch);
  return;
    }

    cost = 1;

    if ( !str_cmp( argument, "str" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_STR )
      cost    = 1;
  stat        = STAT_STR;
  pOutput     = "strength";
    }

    else if ( !str_cmp( argument, "int" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_INT )
      cost    = 1;
  stat      = STAT_INT;
  pOutput     = "intelligence";
    }

    else if ( !str_cmp( argument, "wis" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_WIS )
      cost    = 1;
  stat      = STAT_WIS;
  pOutput     = "wisdom";
    }

    else if ( !str_cmp( argument, "dex" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_DEX )
      cost    = 1;
  stat        = STAT_DEX;
  pOutput     = "dexterity";
    }

    else if ( !str_cmp( argument, "con" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_CON )
      cost    = 1;
  stat      = STAT_CON;
  pOutput     = "constitution";
    }
/*        else if ( !str_cmp( argument, "agt" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_AGT )
      cost    = 1;
  stat        = STAT_AGT;
  pOutput     = "agility";
    }

    else if ( !str_cmp( argument, "end" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_END )
      cost    = 1;
  stat        = STAT_END;
  pOutput     = "endurance";
    }*/

    else if ( !str_cmp( argument, "cha" ) )
    {
  if ( class_table[ch->class].attr_prime == STAT_SOC )
      cost    = 1;
  stat        = STAT_SOC;
  pOutput     = "charisma";
    }
    else if ( !str_cmp(argument, "practice" ) )
  cost = 1;
    else if ( !str_cmp(argument, "hp" ) )
  cost = 1;

    else if ( !str_cmp(argument, "mana" ) )
  cost = 1;

    else if ( !str_cmp(argument, "moves" ) )
  cost = 1;
    else
    {
  strcpy( buf, "You can train:" );
  if ( ch->perm_stat[STAT_STR] < get_max_train(ch,STAT_STR)) 
      strcat( buf, " str" );
  if ( ch->perm_stat[STAT_INT] < get_max_train(ch,STAT_INT))  
      strcat( buf, " int" );
  if ( ch->perm_stat[STAT_WIS] < get_max_train(ch,STAT_WIS)) 
      strcat( buf, " wis" );
  if ( ch->perm_stat[STAT_DEX] < get_max_train(ch,STAT_DEX))  
      strcat( buf, " dex" );
  if ( ch->perm_stat[STAT_CON] < get_max_train(ch,STAT_CON))  
      strcat( buf, " con" );
//  if ( ch->perm_stat[STAT_AGT] < get_max_train(ch,STAT_AGT)) 
//      strcat( buf, " agt" );
//  if ( ch->perm_stat[STAT_END] < get_max_train(ch,STAT_END)) 
//      strcat( buf, " end" );
  if ( ch->perm_stat[STAT_SOC] < get_max_train(ch,STAT_SOC)) 
      strcat( buf, " cha" );
  strcat( buf, " practice");
  strcat( buf, " hp mana moves");

  if ( buf[strlen(buf)-1] != ':' )
  {
      strcat( buf, ".\n\r" );
      send_to_char( buf, ch );
  }
  else
  {
      /*
       * This message dedicated to Jordan ... you big stud!
       */
      act( "You have nothing left to train, you $T!",
    ch, NULL,
    ch->sex == SEX_MALE   ? "big stud" :
    ch->sex == SEX_FEMALE ? "hot babe" :
          "wild thing",
    TO_CHAR ,TRUE);
  }

  return;
    }

    if(ch->pcdata->half_train && cost <= ch->pcdata->half_train)
	htrain = TRUE;// They'll use a half train
    else if(cost > ch->train)// Can't afford the other way
    {
        send_to_char( "You don't have enough training sessions.\n\r", ch );
        return;
    }

    if (!str_cmp("hp",argument))
    {
	int amount = 10;
	if(!htrain)
	{
	  ch->train -= cost;
          act( "Your durability increases!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's durability increases!",ch,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
	  ch->pcdata->half_train -= cost;
	  amount = 5;// half as much
          act( "Your durability increases slightly!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's durability increases slightly!",ch,NULL,NULL,TO_ROOM,FALSE);
	}
        ch->pcdata->perm_hit += amount;
        ch->max_hit += amount;
        ch->hit +=amount;
        ch->pcdata->trained_hit += amount;
        return;
    }
 
    if (!str_cmp("mana",argument))
    {
	int amount = 10;
	if(!htrain)
	{
	  ch->train -= cost;
          act( "Your power increases!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's power increases!",ch,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
	  ch->pcdata->half_train -= cost;
	  amount = 5;// half as much
          act( "Your power increases slightly!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's power increases slightly!",ch,NULL,NULL,TO_ROOM,FALSE);
	}

        ch->pcdata->perm_mana += amount;
        ch->max_mana += amount;
        ch->mana += amount;
        ch->pcdata->trained_mana += amount;
        return;
    }

    if (!str_cmp("moves",argument))
    {
	int amount = 10;
	if(!htrain)
	{
	  ch->train -= cost;
          act( "Your feet shuffle about!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's feet shuffle about!",ch,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
	  ch->pcdata->half_train -= cost;
	  amount = 5;// half as much
          act( "Your feet shuffle about!",ch,NULL,NULL,TO_CHAR,FALSE);
          act( "$n's feet shuffle about!",ch,NULL,NULL,TO_ROOM,FALSE);
	}

        ch->pcdata->perm_move += amount;
        ch->max_move += amount;
        ch->move += amount;
        ch->pcdata->trained_move += amount;
        return;
    }

    if (!str_cmp("practice",argument))
    {
	if(!htrain)
	{
		ch->train -= cost;
	        ch->pcdata->retrain += cost;
        	act( "You convert 1 train to 10 practices.",ch,NULL,NULL,TO_CHAR,FALSE);
	}
	else
	{
		ch->pcdata->half_train -= cost;
		ch->pcdata->half_retrain += cost;
        	act( "You convert 1 half train to 10 practices.",ch,NULL,NULL,TO_CHAR,FALSE);
	}
        ch->practice += 10;
        act( "$n converts trains to practices.",ch,NULL,NULL,TO_ROOM,FALSE);
	return;
    }

    if ( ch->perm_stat[stat]  >= get_max_train(ch,stat) )
    {
  act( "Your $T is already at maximum.", ch, NULL, pOutput, TO_CHAR ,FALSE);
  return;
    }

    if(!htrain)
	ch->train   -= cost;
    else
	ch->pcdata->half_train -= cost;
  
    ch->perm_stat[stat]   += 1;
    act( "Your $T increases!", ch, NULL, pOutput, TO_CHAR ,FALSE);
    act( "$n's $T increases!", ch, NULL, pOutput, TO_ROOM ,FALSE);
    return;
}

void fade( CHAR_DATA *ch, char *argument )
{
   int skill;

   if (IS_SET(ch->mhs,MHS_GLADIATOR))
      return;

   skill = get_skill(ch,gsn_fade);

   if ( number_percent() * number_percent() < 6 &&
	number_percent() > skill )
   {
	if ( IS_SET(ch->mhs, MHS_FADE ) )
	{
    act("$n shimmers into existance before you.",ch,NULL,NULL,TO_ROOM,FALSE);
    act("You shimmer into existance.",ch,NULL,NULL,TO_CHAR,FALSE);
	    REMOVE_BIT(ch->mhs, MHS_FADE );
	}
	else
	{
	    act("$n vanishes!",ch,NULL,NULL,TO_ROOM,FALSE);
	    act("You vanish abruptly.",ch,NULL,NULL,TO_CHAR,FALSE);
	    SET_BIT(ch->mhs, MHS_FADE );
	}
	check_improve( ch, gsn_fade, TRUE, 10 );
   }

    return;
}


/* Original Drag by  "Artur 'Revinor' Biesiadowski" <abies@pg.gda.pl> */
/* Modified by Poquah */
void do_drag( CHAR_DATA * ch, char * argument )
{
   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   OBJ_DATA  *obj;
   ROOM_INDEX_DATA *was_in;
   ROOM_INDEX_DATA *now_in;
   CHAR_DATA *vch;
   CHAR_DATA *victim; 
   bool found;
   int door;

   argument = one_argument (argument, arg1);
   one_argument (argument, arg2);

   if( is_affected(ch,skill_lookup("wraithform")) )
   {
   send_to_char("Not while in wraithform.\r\n",ch);
   return;
   }
   

   if (ch->fighting != NULL)
   {
      send_to_char("No way! You are still fighting.\n\r",ch);
      return;
   }

   /* Assume its Drag a Player */
   if ( ( victim = get_char_room( ch, arg1 ) ) != NULL )  
   {
      if(!IS_NPC(victim))
      {
         if (!is_clan(victim) || !is_clan(ch) ) 
         {
	    send_to_char("You can't drag them.\n\r",ch);
	    return;
	 }
      }

      if( is_affected(victim,skill_lookup("wraithform")) )
      {
      send_to_char("They're made of mist.\r\n",ch);
      return;
      }

    if ( is_clan_guard(victim) == TRUE )
    {
       send_to_char("You may not drag clan hall guards.\r\n",ch);
       return;
    }


      if ( victim->position > POS_SLEEPING )
      {
         act( "You try to drag $N out, but they are not co-operating.", 
	       ch,NULL,victim, TO_CHAR,FALSE);
         act( "$N tries to drag you out of the room.", 
	       victim,NULL,ch, TO_CHAR,FALSE);
         act( "$n tries to move $N, but they are not co-operating.",
	       ch,NULL,victim, TO_ROOM,FALSE);
         return;
      }
      if (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
      {
	 send_to_char("No way, you might wake them!\n\r",ch);
	 return;
      }

      if (!IS_NPC(victim))
      {
//	 if(ch->clan != victim->clan ||
//	    ch->clan == clan_lookup("loner"))
         if(!is_clan_friendly(ch, victim))
	 {
	    if((is_same_group(ch,victim) &&
                IS_AFFECTED(victim, AFF_CHARM)) ||
		!is_same_group(ch,victim))
  send_to_char("They don't like you enough to let you drag them.\n\r",ch);
	    return;
	 }
      }

	    
      if ( ch->move < (victim->carry_weight/5) )
      {
         send_to_char("You are too exhausted to drag them.\n\r",ch);
         return;
      }

      if ( victim->carry_weight >  (2 * can_carry_w (ch)) )
      {
         act( "You try, but $N is too heavy.", ch, NULL, victim,TO_CHAR,FALSE);
         act( "$n tries to move $N, but fails.", ch, NULL, victim,TO_ROOM,FALSE);
         return;
      }

      if ( arg2[0] == '\0' )
      {
         send_to_char ( "Where do you want to drag them ?\n\r", ch);
         return;
      }

      was_in = ch->in_room;

      if( was_in->clan && is_same_clan(ch,victim) )
      {
	send_to_char( "They can move around the hall by themselves.\n\r",ch);
	return;
      }

      if ( ( door = find_exit( ch, arg2 ) ) < 0 )
         return;

      if (door < 0)
      {
	 send_to_char("You can not go that direction.\n\r",ch);
	 return;
      }

      move_char( ch, door, FALSE);

      if (ch->in_room == was_in)
         return;                      /* For some reason player didn't move */

      /* Dont move the victim if is they do not belong to that clan
	or the room is not ownered to them or the room is private */
      if ((ch->in_room->clan && (victim->clan != ch->in_room->clan ||
			 (!IS_NPC(victim) && IS_SET(victim->pcdata->clan_flags, CLAN_NO_HALL)  ) ) )
         || (!is_room_owner(victim,ch->in_room) 
	     && room_is_private(victim,ch->in_room)))  
         return;

      char_from_room(victim);
      char_to_room(victim,ch->in_room);

      if(!IS_NPC(victim) && (victim->pcdata->fast_h || victim->pcdata->fast_m))
      {
        if(ch->pcdata->fast_h >= 10 || ch->pcdata->fast_m >= 10)
          send_to_char("Your rapid regeneration has ended.\n\r", ch);
        ch->pcdata->fast_h = 0;
        ch->pcdata->fast_m = 0;
      }

      ch->move -= apply_chi(ch,(victim->carry_weight/5));  

      act( "You dragged $N with you.", ch, NULL,victim,TO_CHAR,FALSE );
      act( "$n dragged $N into the room.", ch, NULL,victim, TO_ROOM,FALSE );

      if ( !(vch = was_in->people) )
         return;
      
      now_in = ch->in_room;
      ch->in_room = was_in;

      act( "$n dragged $N out of the room.", ch, NULL,victim, TO_ROOM,FALSE );
      ch->in_room = now_in;

   }
   else
   {
   /* Has to be drag an object then */
      obj = get_obj_list( ch, arg1, ch->in_room->contents );
      if ( !obj )
      {
         send_to_char ( "I do not see anything like that here.\n\r", ch);
         return;
      }
  
      /* ITEM_DRAGGABLE is flag which I added for items which I want to be dragged
         but not to be taken ( for example wagon in mine ).
         If you dislike this idea comment it out */

      if ( (!IS_SET( obj->wear_flags, ITEM_TAKE ) && 
           (!IS_SET( obj->wear_flags, ITEM_DRAGGABLE ) ) ) )
      {
 act( "You try to drag $p, but without success.", ch,obj, NULL, TO_CHAR,FALSE);
 act( "$n tries to move $p, but it doesn't budge.",ch,obj, NULL, TO_ROOM,FALSE);
         return;
      }

      /* Only Clanmates and Groupmates or Yourself can drag your corpse */
      if (obj->item_type == ITEM_CORPSE_PC && !IS_IMMORTAL(ch))
      {
	 found = FALSE;
         for ( victim = char_list; victim != NULL; victim = vch)
         {
            vch  = victim->next;
            if (is_name(obj->owner,victim->name))
            {
               found = TRUE;
               break;
            }
         }

         if(found && victim != ch) 
	 {
            if (!is_same_group( victim, ch ) && !is_clan_friendly(ch, victim))
	    {
	       //if(ch->clan != victim->clan || ch->clan == non_clan_lookup("loner"))
	       {
	      send_to_char("You are not allowed to drag the corpse.\n\r",ch);
	          return;
	       }
	    }
	 }
      }

      if ( ch->move < (obj->weight/10) )
      {
         send_to_char("You are too exhausted to drag it.\n\r",ch);
         return;
      }

      if (count_users(obj) > 0 )
      {
      send_to_char("Someone is using that right now!", ch);
      return;
      }

      if ( obj->weight >  (2 * can_carry_w (ch)) )
      {
         act( "You try, but $p is too heavy.", ch, obj, NULL, TO_CHAR,FALSE);
         act( "$n tries to move $p, but fails.", ch, obj, NULL, TO_CHAR,FALSE);
         return;
      }

      if ( arg2[0] == '\0' )
      {
         send_to_char ( "Where do you want to drag it ?\n\r", ch);
         return;
      }

      was_in = ch->in_room;

      if ( ( door = find_exit( ch, arg2 ) ) < 0 )
         return;

      if (door < 0 )
      {
	 send_to_char("You can not go in that direction.\n\r",ch); 
	 return;
      }

      move_char( ch, door, FALSE);

      if (ch->in_room == was_in )
         return;                      /* For some reason player didn't move */

      obj_from_room (obj);
      obj_to_room (obj, ch->in_room);

      ch->move -= apply_chi(ch,(obj->weight/10));  

      act( "You dragged $p with you.", ch, obj, NULL, TO_CHAR,FALSE );
      act( "$n dragged $p into the room.", ch, obj, NULL, TO_ROOM,FALSE );

      if ( !(vch = was_in->people) )
         return;
      
      now_in = ch->in_room;
      ch->in_room = was_in;

      act( "$n dragged $p out of the room.", ch, obj,NULL, TO_ROOM,FALSE );
      ch->in_room = now_in;
   }
   
   return;
}

void do_wraithform_return(CHAR_DATA *ch)
{ /*start brace for wraithform_return*/

  if( !is_affected(ch,skill_lookup("wraithform")) )
  {
    send_to_char( "But you aren't in wraithform.\r\n",ch);
    return;
  }
  WAIT_STATE(ch,48);
  act( "$n becomes less shadowy.\r\n", ch, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char( "You regain your material body.\n\r", ch );
  affect_strip(ch,skill_lookup("wraithform"));
  return;
}/*endbrace for wraithform_return*/

void do_abolish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    bool found = FALSE;
    AFFECT_DATA *paf, *paf_next;

    one_argument( argument, arg );

    if ( ch->class != class_lookup("paladin") )
    {
	send_to_char("You aren't a paladin.\n\r",ch);
	return;
    }

    if ( (victim=get_char_room(ch,arg)) == NULL )
    {
	send_to_char("You can't find that person.\n\r",ch);
	return;
    }

    if ( !IS_NPC(ch) && ch->pcdata->abolish_timer > 0 )
    {
	send_to_char("Your deity will not grant your request.\n\r",ch);
	return;
    }
    
    if ( IS_NPC(victim) )
    {
        send_to_char("Abolish a mob? Yeah right.\r\n",ch);
	return;
    }
    if ( !is_clan(ch) && (victim->pcdata && victim->pcdata->quit_time > 0) )
    {
        send_to_char("They've just come from a pfight, are you sure you want to violate the Big Rule?\r\n",ch);
        return;
    }

    ch->pcdata->abolish_timer = 60 - (ch->level/2);
   
    /* Remove any of these affects that are poison/plague related */ 
   for ( paf = victim->affected ; paf != NULL ; paf = paf_next )
   {
	paf_next = paf->next;
 
	if ( is_abolishable( paf ) )
	{
	    found = TRUE;
	    affect_remove( victim, paf, APPLY_BOTH );
            victim->hit = UMIN( victim->hit + (number_range(ch->level,ch->level*2)), victim->max_hit );
	}
   }

    if ( found )
    {
	 ch->pcdata->abolish_timer = 200 - (ch->level*3);
	 act("$n regains $s health as the sickness leaves $m.",victim,NULL,NULL,TO_ROOM,FALSE);
	 act("You regain your health as the sickness leaves you and a warm sensation flows through your body.",victim,NULL,NULL,TO_CHAR,FALSE);
    }
    else
	send_to_char("That person isn't sick.\n\r",ch);
	
    return;
}

bool is_abolishable( AFFECT_DATA *af )
{
     int sn = af->type;

     if ( 	sn == skill_lookup("poison")
 ||             sn == skill_lookup("dust storm")
 ||		sn == skill_lookup("plague")
 ||	af->where == DAMAGE_OVER_TIME )
	return TRUE;

	return FALSE;
}

void action_ambush( CHAR_DATA *ch, char *argument )
{
	/* STUB to be filled in */
}

void do_knock(CHAR_DATA *ch, char *argument)
{
  /* Constructs taken from do_open().  */
  int door;
  char arg[MAX_INPUT_LENGTH];

  one_argument(argument,arg);

  if (arg[0] == '\0')
    {
      send_to_char("Knock on what?\n\r",ch);
      return;
    }

  if ( ( door = find_door( ch, arg ) ) >= 0 )
    {
      ROOM_INDEX_DATA *to_room;
      EXIT_DATA *pexit;
      EXIT_DATA *pexit_rev;

      pexit = ch->in_room->exit[door];
      act( "$n knocks on the $d.", ch, NULL, pexit->keyword, TO_ROOM,FALSE);
      act( "You knock on the $d.", ch, NULL, pexit->keyword, TO_CHAR,FALSE);

      /* Notify the other side.  */
      if (   ( to_room   = pexit->u1.to_room            ) != NULL
          && ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
          && pexit_rev->u1.to_room == ch->in_room )
        {
          CHAR_DATA *rch;
          for ( rch = to_room->people; rch != NULL; rch = rch->next_in_room )
            act( "You hear someone knocking.", rch, NULL, pexit_rev->keyword, TO_CHAR,FALSE);
        }
    }

  return;
}
