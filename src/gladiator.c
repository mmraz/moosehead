
static char rcsid[] = "$Id: gladiator.c,v 1.110 2004/06/18 23:44:19 ndagger Exp $";

#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "merc.h"
#include "tables.h"
#include "gladiator.h"
#include "recycle.h"

/* the command procedures needed */
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_echo);
DECLARE_DO_FUN(do_startgladiator	);
DECLARE_DO_FUN(do_endgladiator	);
DECLARE_DO_FUN(do_removegladiator	);
DECLARE_DO_FUN(do_gladiator	);
DECLARE_DO_FUN(do_gbet          );
DECLARE_DO_FUN(do_gscore        );
DECLARE_DO_FUN(do_gtscore        );
DECLARE_DO_FUN(do_skipbet        );
DECLARE_DO_FUN(do_stand   );
DECLARE_DO_FUN(do_odds   );
DECLARE_DO_FUN(do_gstatus);

/*
 * External functions.
 */
int	nonclan_lookup	args( (const char *name) );
void    append_note     args( (NOTE_DATA *glad_qnote)); 

/*
 * Local functions.
 */
void    gladiator_bet_resolve args( (CHAR_DATA *winner, CHAR_DATA *bettor));
void    gladiator_start_countdown args((void));
void    begin_gladiator args((void));
void    end_gladiator args((void));
void    single_update args((void));
void    team_update args((void));

void do_startgladiator(CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
   char arg3[MAX_INPUT_LENGTH];
   char arg4[MAX_INPUT_LENGTH];
   char arg5[MAX_INPUT_LENGTH];
   char arg6[MAX_INPUT_LENGTH];
   int i1,i2,i3,i4,i6;
   DESCRIPTOR_DATA *d;

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);
   argument = one_argument(argument, arg3);
   argument = one_argument(argument, arg4);
   argument = one_argument(argument, arg5);
   argument = one_argument(argument, arg6);
   if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0' 
       || arg4[0] == '\0' || arg5[0] == '\0')
   {
      send_to_char("Syntax: startgladiator <type> <min_level> <max_level> <ticks till start> <blind/noblind> \n\r", ch);
      return;
   }

   if (gladiator_info.started == TRUE)
   {
      send_to_char("There is already a Gladiator Event going on!\n\r", ch);
      return;
   }

   if ( !is_number(arg1) || !is_number(arg2) 
	|| !is_number(arg3) || !is_number(arg4))
	{
	  send_to_char("All arguments to startgladiator are numeric.\n\r",ch);
	  return;
	}

   i1 = atoi(arg1);
   i2 = atoi(arg2);
   i3 = atoi(arg3);
   i4 = atoi(arg4);

   if ( i1 < 1 || i1 > 3 || i2 <= 0 || i2 > 51 
	|| i3 <= 0 || i3 > 60 || i4 < 3 
	|| arg5[0] == '\0' || (str_prefix(arg5,"blind") && str_prefix(arg5,"noblind") && str_prefix(arg5,"experimental") && str_prefix(arg5,"WNR")))
   {
      send_to_char("The type either has to be one of the following:\n\r",ch);
      send_to_char("1 - Singles Event\n\r",ch);
      send_to_char("2 - Teams Event\n\r",ch);
      send_to_char("3 - Assigned Teams Events\n\r",ch);
      send_to_char("Levels must be between 1 and 60.\n\r", ch);
      send_to_char("Minimum Start Time is 3 Ticks.\n\r",ch);
      send_to_char("blind to hide names or noblind to leave revealed.\n\r",ch);
      return;
    }

   if (i1 > 2)
   {
      send_to_char("Not finished yet, please wait.\n\r", ch);
      return;
   }
   
   if (i3 < i2)
   {
      send_to_char("Max level can not be less than the min level.\n\r", ch);
      return;
   }

   if (i1 == 2 || i1 == 3 )
   {
      if (arg6[0] == '\0')  
      {
         send_to_char("Syntax: startgladiator <type> <min_level> <max_level> <ticks till start> <blind/noblind> <ticks till end>\n\r", ch);
         return;
      }
     
      if (!is_number(arg6)) 
      {
         send_to_char("All arguments to startgladiator are numeric.\n\r",ch);
         return;
      }

      i6 = atoi(arg6);

      if (i6 < 5)
      {
	 send_to_char("Atleast give them some time to fight. Minimum 5 ticks.\n\r",ch);
	 return;
      }
      
      gladiator_info.gladiator_score = 0;
      gladiator_info.barbarian_score = 0;
      gladiator_info.team_counter = i6;
   }

   gladiator_info.started = TRUE;
   gladiator_info.type = i1;
   gladiator_info.min_level = i2;
   gladiator_info.max_level = i3;
   gladiator_info.time_left = i4;
   gladiator_info.playing = 0;
   gladiator_info.gladiator_score = 0;
   gladiator_info.barbarian_score = 0;
   gladiator_info.bet_counter = 5;
  gladiator_info.total_levels = 0;
  gladiator_info.total_plays = 0;
  gladiator_info.total_wins = 0;
  gladiator_info.num_of_glads = 0;

   log_string(arg5);
   if (!str_prefix(arg5,"blind"))
   {
      gladiator_info.blind = TRUE;
      gladiator_info.exper = FALSE;
      gladiator_info.WNR = FALSE;
      strcpy(arg5, "blind");
   }
   else if(!str_prefix(arg5, "experimental"))
   {
      gladiator_info.exper = TRUE;
      gladiator_info.blind = TRUE;
      gladiator_info.WNR = FALSE;
      strcpy(arg5, "{REXPERIMENTAL{x");
   }
   else if(!str_prefix(arg5, "wnr"))
   {
      gladiator_info.exper = FALSE;
      gladiator_info.blind = FALSE;
      gladiator_info.WNR = TRUE;
      strcpy(arg5, "{WWednesday Night Rumble!{x");
   }
   else
   {
      gladiator_info.blind = FALSE;
      gladiator_info.exper = FALSE;
      gladiator_info.WNR = FALSE;
      strcpy(arg5, "not blind");
   }

   sprintf(buf, "Gladiator started for levels %d to %d, %s %s combat.\n\rType 'GLADIATOR' to join. Read 'help gladiator' for more info.",
   gladiator_info.min_level, gladiator_info.max_level,
   (gladiator_info.type/2?"team":"single"),
   arg5);
   do_echo(ch,buf);
   log_string(buf);
   for (d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL && !IS_NPC(d->character))
      {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
            REMOVE_BIT(d->character->mhs, MHS_GLADIATOR);
	 if ( d->character->pcdata->glad_bet_amt != 0)
	   d->character->pcdata->glad_bet_amt = 0;
      }
   }
#ifdef CODETEST
   glad_qnote = new_note();
   glad_qnote->next = NULL;
   glad_qnote->sender = str_dup("Emperor Flavius Vespasian");
   glad_qnote->date = str_dup("");
   glad_qnote->to_list = str_dup("all"); 
   glad_qnote->subject = str_dup("Echoes of Eternity");
   glad_qnote->text = str_dup("");
   glad_qnote->type = NOTE_QUEST;
  
   gladbuffer = new_buf();
   add_buf(gladbuffer,glad_qnote->text);
   add_buf(gladbuffer,"On this day the crowds at the arena were entertained by:\n\r");
   free_string(glad_qnote->text);
   glad_qnote->text = str_dup(buf_string(gladbuffer));
   free_buf(gladbuffer);
#endif
}

void remove_gladiator(CHAR_DATA *victim)
{
    char buf[MAX_STRING_LENGTH];

    if(!IS_SET(victim->mhs, MHS_GLADIATOR))
    {
      return;
    }

    REMOVE_BIT(victim->mhs, MHS_GLADIATOR);
    char_from_room(victim);
    if(victim->pcdata && victim->pcdata->clan_info && victim->pcdata->clan_info->clan->hall)
      char_to_room(victim, (ROOM_INDEX_DATA*)victim->pcdata->clan_info->clan->hall->to_place);
    else
      char_to_room(victim,get_room_index(clan_table[victim->clan].hall));
    victim->clan = victim->pcdata->save_clan;
    victim->pcdata->save_clan = 0;
    victim->hit  = victim->max_hit;
    victim->mana = victim->max_mana;
    victim->move = victim->max_move;
    victim->pcdata->quit_time = 0; 
    victim->position = POS_SLEEPING;
    update_pos(victim);
    do_look(victim, "auto");
    /* Check if the gladiator has started and if so if the 
      removal of this player leaves only 1 person */
    if((gladiator_info.playing == 2) && gladiator_info.time_left == 0)
    {
       DESCRIPTOR_DATA *d;

       for(d = descriptor_list; d != NULL; d = d->next)
       {
	  if (d->character != NULL)
	  {
          if (IS_SET(d->character->mhs, MHS_GLADIATOR))
          {
             sprintf(buf, "%s is victorious in the arena!", d->character->name);
             gladiator_talk_ooc(buf); 

#ifdef CODETEST
             gladbuffer = new_buf();
             add_buf(gladbuffer,glad_qnote->text);
             add_buf(gladbuffer,buf);
             free_string(glad_qnote->text);
             glad_qnote->text = str_dup(buf_string(gladbuffer));
             free_buf(gladbuffer);
#endif

             gladiator_winner(d->character);
          }
	  }
       }
    }
    gladiator_info.playing--;
    gladiator_info.num_of_glads--;
    return;
}

void do_removegladiator( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Syntax: removegladiator <char name>\n\r",ch);
        return;
    }

    if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
    {
        send_to_char( "There is no gladiator with that name to remove.\n\r", ch );
        return;
    }

    if (IS_NPC(victim))
       return;

    if (!IS_SET(victim->mhs,MHS_GLADIATOR))
    {
       send_to_char( "There is no gladiator with that name to remove.\n\r",ch);
       return;
    }

    sprintf(buf, "The Immortals have removed %s from the Event, oh the shame!",victim->name);
    gladiator_talk_ooc(buf);

#ifdef CODETEST
    gladbuffer = new_buf();
    add_buf(gladbuffer,glad_qnote->text);
    add_buf(gladbuffer,buf);
    free_string(glad_qnote->text);
    glad_qnote->text = str_dup(buf_string(gladbuffer));
    free_buf(gladbuffer);
#endif

    send_to_char("You were removed from the Arena by an immortal.\n\r",victim);
    remove_gladiator(victim);

    return;
}


void do_endgladiator( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    DESCRIPTOR_DATA *d;
    char arg1[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg1 );

    if ( arg1[0] == '\0' )
    {
        send_to_char( "Syntax: endgladiator <victor's name or none>\n\r",ch);
        return;
    }

    if (strcmp (arg1,"none")) 
    {
      DESCRIPTOR_DATA *d;
      victim = NULL;
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
        victim = d->original ? d->original : d->character;
        
        if ( (d->connected == CON_PLAYING) && !str_cmp(victim->name, arg1) )
          break;
        victim = NULL;
      }
      if(victim == NULL || !IS_SET(victim->mhs, MHS_GLADIATOR))
      {
        send_to_char("They aren't playing. Use a full name for the desired player.\n\r", ch);
        return;
      }

       /*if ( ( victim = get_char_world( ch, arg1 ) ) == NULL )
       {// UNSAFE - can return a mob
           send_to_char( "They aren't playing.\n\r", ch );
           return;
       }*/

       sprintf(buf, "The Immortals have declared %s to be victorious!",victim->name);
       gladiator_talk_ooc(buf);

#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif

       gladiator_winner(victim);
    }
    else
    {
       sprintf(buf, "The Immortals have declared combat over with no one the victor!");
       gladiator_talk_ooc(buf);

#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif

       gladiator_info.started = FALSE;
       gladiator_info.playing = 0;
       gladiator_info.time_left = 0;
       gladiator_info.min_level = 0;
       gladiator_info.max_level = 0;
       gladiator_info.type = 0;
       gladiator_info.team_counter = 0;
       gladiator_info.blind = FALSE;
       gladiator_info.gladiator_score = 0;
       gladiator_info.barbarian_score = 0;
       gladiator_info.bet_counter = 0;
       gladiator_info.bet_total = 0;

       for(d = descriptor_list; d != NULL; d = d->next)
       {
	  if(d->character != NULL)
	  {
          if (!IS_NPC(d->character))
          {
	     d->character->gold += d->character->pcdata->glad_bet_amt;
             d->character->pcdata->glad_bet_on = d->character;

             if (IS_SET(d->character->mhs, MHS_GLADIATOR))
             {
		d->character->pcdata->glad_tot_bet = 0;
                char_from_room(d->character);
                char_to_room(d->character,get_room_index(clan_table[d->character->clan].hall));
                REMOVE_BIT(d->character->mhs, MHS_GLADIATOR);
	        d->character->clan = d->character->pcdata->save_clan;
		d->character->pcdata->save_clan = 0;
                do_look(d->character, "auto");
		d->character->position = POS_SLEEPING;
		/* Tell the Glad its over incase they have channel off */
	//	send_to_char(buf,d->character);// Not needed currently
             }
          }
	  }
       }
    }
    return;
}

void do_skipbet ( CHAR_DATA *ch , char *argument )
{
   char buf[MAX_STRING_LENGTH];

   if(gladiator_info.time_left > 0)
   {
      send_to_char("You must wait till betting begins before you skip it.\n\r",ch);
      return;
   }

   sprintf(buf, "The Immortals have decided to skip the betting!");
   gladiator_talk_ooc(buf);
   gladiator_info.bet_counter = 0;
   begin_gladiator(); 
   return;
}

void gladiator_winner( CHAR_DATA *ch )
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_STRING_LENGTH];

   if (gladiator_info.type == 1 )
      {
      ch->pcdata->gladiator_data[GLADIATOR_VICTORIES] += 1;
      sprintf(buf, "Flavius Vespasian grants you a prize of %d gold!!!\n\r", 
      gladiator_info.bet_total /10 );
      send_to_char(buf, ch);
      ch->gold += gladiator_info.bet_total /10;
      }

   for(d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL && !IS_NPC(d->character))
      {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
         {
            char_from_room(d->character);
            if(d->character->pcdata && d->character->pcdata->clan_info && d->character->pcdata->clan_info->clan->hall)
              char_to_room(d->character, (ROOM_INDEX_DATA*)d->character->pcdata->clan_info->clan->hall->to_place);
            else
              char_to_room(d->character,get_room_index(clan_table[d->character->clan].hall));
            REMOVE_BIT(d->character->mhs, MHS_GLADIATOR);
	    d->character->clan = d->character->pcdata->save_clan;
            d->character->pcdata->save_clan = 0;
            do_look(d->character, "auto");
            d->character->hit  = d->character->max_hit;
            d->character->mana = d->character->max_mana;
            d->character->move = d->character->max_move;
    	    d->character->position = POS_SLEEPING;
            d->character->pcdata->quit_time = 0; 
            d->character->pcdata->sac = (d->character->class == class_lookup("paladin")) ? 600:300;
            if(d->character->class == class_lookup("crusader"))
               d->character->pcdata->sac = 400;
            if(HAS_KIT(d->character,"bishop"))
               d->character->pcdata->sac += 100;

            if ((gladiator_info.type == 2 || gladiator_info.type == 3) 
                 && ch->pcdata->gladiator_team == d->character->pcdata->gladiator_team)
	       d->character->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES] += 1;
	 }
         if ( d->character->pcdata->glad_bet_amt != 0 )
	 {
	     if(gladiator_info.type == 1 )
	        gladiator_bet_resolve( ch, d->character);
	     else
	     {
	     /*Rage fill in the Team Bet resolve */
	     }
	   }
      }
   }

#ifdef CODETEST
   strtime       = ctime( &current_time );
   strtime[strlen(strtime)-1]  = '\0';  
   glad_qnote->date = str_dup(strtime);
   glad_qnote->date_stamp = current_time;

   append_note(glad_qnote);
   glad_qnote = NULL;
#endif

   gladiator_info.started = FALSE;
   gladiator_info.playing = 0;
   gladiator_info.time_left = 0;
   gladiator_info.min_level = 0;
   gladiator_info.max_level = 0;
   gladiator_info.type = 0;
   gladiator_info.team_counter = 0;
   gladiator_info.blind = FALSE;
   gladiator_info.gladiator_score = 0;
   gladiator_info.barbarian_score = 0;
   gladiator_info.bet_counter = 0;
   gladiator_info.bet_total = 0;
   gladiator_info.total_levels = 0;
   gladiator_info.total_plays = 0;
   gladiator_info.total_wins = 0;

   return;
}

void do_gladiator( CHAR_DATA *ch, char *argument)
{
   char arg1[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   ROOM_INDEX_DATA *location;

   if (IS_NPC(ch))
      return;

   argument = one_argument( argument, arg1 );

   if (gladiator_info.started != TRUE) 
   {
      send_to_char("There is no Gladiator Combat going!\n\r", ch);
      return;
   }
   if (is_clan(ch) && ch->pcdata->start_time > 0 )
   {
      ch->pcdata->start_time = 0;
   }
   if (ch->pcdata && ch->pcdata->quit_time > 0 && !IS_IMMORTAL (ch) && ch->in_room->clan != ch->clan) 
   {
      sprintf(buf, "Not while your timer exists... wait %d ticks.\n\r", ch->pcdata->quit_time);
      send_to_char (buf,ch);
      return;
   }

   if (ch->level < gladiator_info.min_level || ch->level > gladiator_info.max_level)
   {
      send_to_char("Sorry, you have not the right experience for this Combat.\n\r", ch);
      return;
   }
   if ( ch->pcdata->glad_bet_amt != 0 && ch->pcdata->glad_bet_on != ch )
      {
      send_to_char("You already bet on someone else.  Either clear\n\r", ch);
      send_to_char("your bet, or don't participate.\n\r", ch);
      return;
      }

   if (IS_SET(ch->mhs, MHS_GLADIATOR))
   {
      send_to_char("You are already a Gladiator, no turning back now.\n\r", ch);
      return;
   }

   if (gladiator_info.started == TRUE && gladiator_info.time_left < 1)
   {
      send_to_char("Hey goober its started already.\n\r", ch);
      return;
   }


      location = get_room_index(ROOM_VNUM_SINGLE_GLADIATOR);  
      act("$n goes to spill some blood in Gladiator Combat!", ch, NULL, NULL, TO_ROOM,FALSE);
      ch->mana = ch->max_mana;// Not hp so you can't jump here to heal, then quit 
      while(ch->damaged)
  	damage_remove(ch, ch->damaged);

      char_from_room(ch);
      char_to_room(ch, location);
      SET_BIT(ch->mhs, MHS_GLADIATOR);
      gladiator_info.total_levels += ch->level;
      gladiator_info.total_plays += ch->pcdata->gladiator_data[GLADIATOR_PLAYS];
      gladiator_info.total_wins += ch->pcdata->gladiator_data[GLADIATOR_VICTORIES];
      ch->pcdata->save_clan = ch->clan; 
      ch->pcdata->gladiator_team = 0;
      if (!is_clan(ch))
	 ch->clan = nonclan_lookup("temp");
      sprintf(buf, "%s (Level %d %s) joins the Gladiators!", ch->name, ch->level, class_table[ch->class].name);
      gladiator_talk_ooc(buf);
      if (gladiator_info.blind)
      {
        free_string( ch->long_descr );                                        
        /* NEVER go past this length */
        ch->long_descr = str_dup( "An extra long Gladiator string is here, taking up space." );
        set_glad_name(ch);
        act("$l arrives to prove his worth!", ch, NULL, NULL, TO_ROOM,FALSE);
      }
      else
      {
        act("$n arrives to prove his worth!", ch, NULL, NULL, TO_ROOM,FALSE);
      }
      gladiator_info.playing++;
      gladiator_info.num_of_glads++;
      die_follower(ch);
      if(gladiator_info.exper == TRUE || gladiator_info.WNR == TRUE)
      {
        while ( ch->flash_affected )
          flash_affect_remove( ch, ch->flash_affected,APPLY_BOTH );
        while ( ch->affected )
          affect_remove( ch, ch->affected,APPLY_BOTH );
      }
      else
        affect_strip(ch,gsn_sneak);
      do_look(ch, "auto");
      return;
}

void do_gbet ( CHAR_DATA *ch , char *argument )

{
  int amount;
  CHAR_DATA *victim;
  char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);
 
  if ( arg1[0] == '\0' )
    {
    send_to_char("Syntax: gbet <amount> <character>\n\r", ch);
    send_to_char("        gbet clear\n\r",ch);
    return;
    }

  if ( gladiator_info.type != 1 )
  {
    send_to_char("You may only bet on single events.  Team betting is not yet available.\n\r", ch);
    return;
  }


  if (!str_cmp(arg1, "clear"))
   {
     if (gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
     {
       send_to_char("Umm....it's a little late, the event has already begun\n\r", ch);
       return;
     }

     if (ch->pcdata->glad_bet_on)
     {
       send_to_char("Bet cleared.\n\r", ch);
       ch->gold += ch->pcdata->glad_bet_amt;
       ch->pcdata->glad_bet_on->pcdata->glad_tot_bet -= ch->pcdata->glad_bet_amt;
       ch->pcdata->glad_bet_on = ch;
       gladiator_info.bet_total -= ch->pcdata->glad_bet_amt;
       ch->pcdata->glad_bet_amt = 0;
       return;
     }
     else
     {
       send_to_char("You have no bet to clear!!\n\r", ch);
       return;
     }
    }

    if(!str_cmp(arg1, "show") && ch->pcdata->glad_bet_on != 0)
    {
    sprintf(buf,"You have placed a bet of %d gold on %s.\n\r", ch->pcdata->glad_bet_amt, ch->pcdata->glad_bet_on->name);
    send_to_char(buf, ch);
    return;

    }


  if (ch->pcdata->glad_bet_amt != 0)
  {
  send_to_char("You must clear your current bet before betting again.\n\r", ch);
  return;
   }
   
   if (gladiator_info.started != TRUE) 
   {
      send_to_char("There is no Gladiator Combat going!\n\r", ch);
      return;
   }
  
  if ( (victim = get_char_world(ch, arg2)) == NULL)
     {
     send_to_char("They aren't here.\n\r", ch);
     return;
     }

  if ( !IS_SET(victim->mhs, MHS_GLADIATOR) )
     {
     send_to_char("You can't bet on them, they aren't participating.\n\r", ch);
     return;
     }

  if (gladiator_info.started == TRUE && victim != ch 
      && IS_SET(ch->mhs, MHS_GLADIATOR))
      {
      send_to_char("If you want to bet, you have to bet on yourself. \n\r", ch);
      return;
      }
 
  if (gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
   {
      send_to_char("You may not bet after the combat has started\n\r", ch);
      return;
   }
  
  amount = atoi(arg1);
  
  if ( amount < 0 )
    {
    send_to_char("An Immortal has created [1] a beautiful jeweled egg\n\rAn Immortal gives you a beautiful jeweled egg.\n\r", ch);
    return;
    }

  if ( ch->gold < amount )
    {
    send_to_char("You can't bet that much, you don't have it in gold.\n\r",ch);
    return;
    }

  if (amount > 500)
    {
    send_to_char("Maximum Betting amount is 500 gold.\n\r",ch);
    return;
    }

  ch->pcdata->glad_bet_amt = amount;
  ch->gold -= amount;
  ch->pcdata->glad_bet_on = victim;
  gladiator_info.bet_total += amount;
  victim->pcdata->glad_tot_bet += amount;

  sprintf(buf, "You have placed a bet of %d gold on %s.\n\r", amount, victim->name);
  send_to_char(buf,ch);
  sprintf(buf, "%s bet %d on %s", ch->name, amount, victim->name);
  log_string(buf);
  return;
}

void gladiator_bet_resolve( CHAR_DATA *winner, CHAR_DATA *bettor )
{
 int odds, this_odds, payout= 0;
 char buf[MAX_STRING_LENGTH];
 
 int avg_level, gladadj, winlossavg, thiswinloss;
 

 avg_level = (gladiator_info.total_levels  * 100/
	     gladiator_info.num_of_glads ) ;

 if ( bettor->pcdata->glad_bet_on == winner  && bettor->pcdata->glad_bet_amt > 0 )
   {
    odds =( gladiator_info.num_of_glads *avg_level  /
           (winner->level));
	   

    if( winner->pcdata->gladiator_data[GLADIATOR_VICTORIES] -1  != 0  &&
	winner->pcdata->gladiator_data[GLADIATOR_PLAYS] >= 1 )
      {
       winlossavg = (gladiator_info.total_wins  * 100 /
		    gladiator_info.total_plays);
       

       thiswinloss = ((winner->pcdata->gladiator_data[GLADIATOR_VICTORIES] -1 )* 100 /
		     winner->pcdata->gladiator_data[GLADIATOR_PLAYS] );

       gladadj = (thiswinloss  * 100 / winlossavg) ;
       
       this_odds = ((odds/200) + ((odds/2)/gladadj));
	      
       this_odds = URANGE(1, this_odds, 99);

       }
    else
      {
       this_odds = URANGE(1, odds/ 100, 99);
      }
    
   payout = bettor->pcdata->glad_bet_amt * this_odds ;

   sprintf(buf,"Your bet on %s came through, at %d odds.  You won %d gold!!!!\n\r", winner->name,this_odds, payout);
   bettor->gold += payout;
   }
 else
   {
   sprintf(buf,"Your bet on %s was a waste of money.  God they suck!  Better luck next time. \n\r", bettor->pcdata->glad_bet_on->name);
   }

 send_to_char(buf,bettor);
   sprintf(buf, "Paid out %d to %s, on bet of %d, winner: %s", payout, bettor->name, bettor->pcdata->glad_bet_amt, winner->name);
   log_string(buf);
 return;
}

/* Do not go over the length of: "An extra long Gladiator string is here, taking up space." */
void set_glad_name(CHAR_DATA *ch)
{// Mirrors condition values
    if(gladiator_info.started != TRUE || gladiator_info.blind != TRUE ||
      !IS_SET(ch->mhs, MHS_GLADIATOR) || IS_NPC(ch))
      return;
    char team[15];
    int percent = ch->hit * 100 / (ch->max_hit > 0 ? ch->max_hit : 1);
    if (gladiator_info.type == 2 && ch->pcdata->gladiator_team == 2)
      strcpy(team, "Barbarian");
    else
      strcpy(team, "Gladiator");
    if(gladiator_info.exper == TRUE)
    {
      if (percent >= 100)
        sprintf(ch->long_descr, "A healthy %s", team);
      else if (percent >= 75)
        sprintf(ch->long_descr, "A bruised %s", team);
      else if (percent >= 30)
        sprintf(ch->long_descr, "A wounded %s", team);
      else
        sprintf(ch->long_descr, "A dying %s", team);
    }
    else
      sprintf(ch->long_descr, "A %s", team);
}

void gladiator_rename_all(void)
{
  DESCRIPTOR_DATA *d;
  if(gladiator_info.started != TRUE || gladiator_info.blind != TRUE)
    return;/* Nothing to do if it's not running or isn't blind */
  for ( d = descriptor_list; d; d = d->next )
  {
    if ( d->character != NULL && IS_SET(d->character->mhs, MHS_GLADIATOR) && !IS_NPC(d->character))
    {
      set_glad_name(d->character);
    }
  }
}

void gladiator_update(void)
{
   if (gladiator_info.started == TRUE)
   {
      if (gladiator_info.bet_counter > 0)
         gladiator_start_countdown();
      else
      {
	 if(gladiator_info.type == 1 )
	    single_update();
         else
	    team_update();
      }
   }
   return;
}

void gladiator_start_countdown(void)
{
   sh_int time;
   DESCRIPTOR_DATA *d;
   char buf[MAX_INPUT_LENGTH];

   time = gladiator_info.time_left - 1;
   if(gladiator_info.time_left > 0)
      gladiator_info.time_left--;
   if (time > 0)
   {
      sprintf(buf, "%d tick%s left to join the Gladiator Combat. Read 'help gladiator' for more info.", time, time == 1 ? "" : "s");
      for ( d = descriptor_list; d; d = d->next )
      {
         if ( d->connected == CON_PLAYING )
         {
            send_to_char( buf, d->character );
            send_to_char( "\n\r",   d->character );
         }
      }

      sprintf(buf, "%d %s %s signed up to fight in the Arena so far.", gladiator_info.playing, gladiator_info.playing == 1 ? "Gladiator" : "Gladiators", gladiator_info.playing == 1 ? "is" : "are");
      gladiator_talk_ooc(buf);

      if(gladiator_info.blind)
      {
      if (gladiator_info.type == 1)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Singles Blind Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      if (gladiator_info.type == 2)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Random Teams Blind Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      if (gladiator_info.type == 3)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Assigned Teams Blind Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      }
      else
      {
      if (gladiator_info.type == 1)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Singles Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      if (gladiator_info.type == 2)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Random Teams Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      if (gladiator_info.type == 3)
      {
         sprintf(buf, "Type of Event: Levels %d - %d, Assigned Teams Combat.", gladiator_info.min_level, gladiator_info.max_level);
      }
      }
      gladiator_talk_ooc(buf);
   }
   else
   {  
      if (gladiator_info.playing < 2)
      {
         sprintf(buf, "Not enough people for the Event.  Gladiator Combat ended.");
         gladiator_talk_ooc(buf);
#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif
         end_gladiator();
         return;
      }
      else
      {
	 gladiator_info.bet_counter--;
	 if (gladiator_info.bet_counter == 0)
            begin_gladiator(); 
	 else
	 {
	    sprintf(buf,"%d Ticks of Betting until the event starts.",gladiator_info.bet_counter);
            gladiator_talk_ooc(buf);
	 }
      }
   }
   return;
}

void begin_gladiator (void)
{
   sh_int team_select;
   DESCRIPTOR_DATA *d;
   ROOM_INDEX_DATA *random;
   char buf[MAX_INPUT_LENGTH];

   sprintf(buf, "The battle begins! %d Gladiators are fighting!", gladiator_info.playing);
   gladiator_talk_ooc(buf);

   team_select = 1;
   for(d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL)
      {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
         {
            d->character->hit  = d->character->max_hit;
            d->character->mana = d->character->max_mana;
            d->character->move = d->character->max_move;
            d->character->pcdata->gladiator_attack_timer = 5;
            update_pos(d->character);
	    if (gladiator_info.type == 1)
               d->character->pcdata->gladiator_data[GLADIATOR_PLAYS]++;
	    else
               d->character->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS]++;

            /* Default random to single player range */
            random = get_room_index(number_range(10800,10819));

            if (gladiator_info.type == 2)
            {
               d->character->pcdata->gladiator_team = team_select;
               if(team_select == 1)
                  team_select = 2; 
               else
                  team_select = 1;

               if(d->character->pcdata->gladiator_team == 1)
                  random = get_room_index(ROOM_VNUM_TEAM_GLADIATOR);
                  if (gladiator_info.blind == TRUE)
                  {
                     free_string( d->character->long_descr );                
	             /* NEVER go past this length */
	             d->character->long_descr = str_dup( "An extra long Gladiator string is here, taking up space." );
	             set_glad_name(d->character);
                  }
               else
	       {
                  random = get_room_index(ROOM_VNUM_TEAM_BARBARIAN);
                  if (gladiator_info.blind == TRUE)
                  {
                     free_string( d->character->long_descr );           
	             /* NEVER go past this length */
	             d->character->long_descr = str_dup( "An extra long Barbarian string is here, taking up space." );
	             set_glad_name(d->character);
                  }
	       }
            }

            char_from_room(d->character);
            char_to_room(d->character, random);
	    if (!IS_AWAKE(d->character))
	       do_stand(d->character,"");
	    else
               do_look(d->character, "auto");
         }
      }
   }

   return;
}

void end_gladiator(void)
{
   DESCRIPTOR_DATA *d;

   gladiator_info.started = FALSE;
   gladiator_info.time_left = 0;
   gladiator_info.min_level = 0;
   gladiator_info.max_level = 0;
   gladiator_info.type = 0;
   gladiator_info.playing = 0;
   gladiator_info.team_counter = 0;
   gladiator_info.blind = FALSE;
   gladiator_info.bet_counter = 0;
   gladiator_info.total_levels = 0;
   gladiator_info.total_wins = 0;
   gladiator_info.total_plays = 0;

   for(d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL)
      {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
         {
            char_from_room(d->character);
            char_to_room(d->character, get_room_index(clan_table[d->character->clan].hall));
            d->character->clan = d->character->pcdata->save_clan;
            d->character->pcdata->save_clan = 0;
            REMOVE_BIT(d->character->mhs,MHS_GLADIATOR);
            do_look(d->character, "auto");
            d->character->position = POS_SLEEPING;
         }
      }
   }
   return;
}

void single_update(void)
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_INPUT_LENGTH];

   sprintf(buf, "The battle rages on with %d Gladiators still remaining.", gladiator_info.playing);
   gladiator_talk_ooc(buf);

   for(d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL)
 {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
         {
            d->character->pcdata->gladiator_attack_timer--;
            if(d->character->pcdata->gladiator_attack_timer == 0)
            {
	       if (is_affected(d->character,gsn_blindness) ||
		   is_affected(d->character,gsn_dust_storm) ||
                   IS_AFFECTED(d->character, AFF_SLEEP) || 
                    d->character->pcdata->quit_time != 0)
                  d->character->pcdata->gladiator_attack_timer = 5;
               else
               {
                  sprintf(buf, "%s slips into a pit full of tigers, next time %s will be more active.", d->character->name,d->character->name);
                  gladiator_talk_ooc(buf);
#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif
		  send_to_char("You were removed from the Arena for being inactive.\n\r",d->character);
                  remove_gladiator(d->character);
               }
            }
         }
      }
   } /* for loop through players */ 
   return;
}

void team_update(void)
{
   sh_int team_select;
   DESCRIPTOR_DATA *d;
   char buf[MAX_INPUT_LENGTH];

   sprintf(buf, "The current score is: Gladiators %d - Barbarians %d.\n\r",gladiator_info.gladiator_score,gladiator_info.barbarian_score);  
   gladiator_talk_ooc(buf);

   gladiator_info.team_counter--;

   if(gladiator_info.team_counter == 0)
   {
      if(gladiator_info.gladiator_score == gladiator_info.barbarian_score)
      {
	 gladiator_info.team_counter = 2;
         sprintf(buf," The Teams are Tied, they continue to fight!\n\r");
         gladiator_talk_ooc(buf);
	 return;
      }

      if(gladiator_info.gladiator_score > gladiator_info.barbarian_score)
      {
         sprintf(buf, "The Gladiators are victorious!\n\r");  
         team_select = 1;
      }
      else
      {
         sprintf(buf, "The Barbarians are victorious!\n\r");  
         team_select = 2;
      }
      gladiator_talk_ooc(buf);

      for(d = descriptor_list; d != NULL; d = d->next)
      {
         if (d->character != NULL)
         {
            if (IS_SET(d->character->mhs, MHS_GLADIATOR))
            {
	       send_to_char(buf,d->character);
               if(d->character->pcdata->gladiator_team == team_select)
               {
                  gladiator_winner(d->character);
		  break; 
               }
	    }
	 }
      }
   }
   return;
}

void do_gtscore ( CHAR_DATA *ch , char *argument )
{
   char buf[MAX_INPUT_LENGTH];

   if (gladiator_info.type == 1)
   {
      send_to_char("There is no Team Event on.\n\r",ch);
      return;
   }

   sprintf(buf, "The current score is: Gladiators %d - Barbarians %d. With %d ticks remaining\n\r",gladiator_info.gladiator_score,gladiator_info.barbarian_score,gladiator_info.team_counter);  
   send_to_char(buf,ch);
}

void do_gscore( CHAR_DATA *ch, char *argument )
{     
     char buf[MAX_STRING_LENGTH];
 
     if(!IS_NPC(ch))  
     { 
        sprintf(buf,"Single Events Played: %d  Victories: %d  Kills: %d \n\r "
        "Team Events Played: %d  Victories: %d  Kills: %d\n\r",
               ch->pcdata->gladiator_data[GLADIATOR_PLAYS],
               ch->pcdata->gladiator_data[GLADIATOR_VICTORIES],
               ch->pcdata->gladiator_data[GLADIATOR_KILLS],
               ch->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS],
               ch->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES],
               ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS]);
        send_to_char(buf,ch);
     }
     return;
}

void gladiator_left_arena( CHAR_DATA *ch, bool DidQuit )
{
   char buf[MAX_STRING_LENGTH];

   REMOVE_BIT(ch->mhs,MHS_GLADIATOR);
   ch->clan = ch->pcdata->save_clan;
   ch->pcdata->save_clan = 0;

   if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->hall)
   {
     if(DidQuit)
     {
       char_from_room(ch);
       char_to_room(ch, ch->pcdata->clan_info->clan->hall->to_place);
     }
     else
       ch->was_in_room = ch->pcdata->clan_info->clan->hall->to_place;
   }
   else
   {
     if(DidQuit)
     {
        char_from_room(ch);
        char_to_room(ch, get_room_index(clan_table[ch->clan].hall));
     }
     else
        ch->was_in_room = get_room_index(clan_table[ch->clan].hall);
   }
   /* Check if the gladiator has started and if so if the 
      removal of this player leaves only 1 person */
   if(gladiator_info.playing == 2 && gladiator_info.time_left == 0
       && gladiator_info.type == 1)
   {
      DESCRIPTOR_DATA *d;

      for(d = descriptor_list; d != NULL; d = d->next)
      {
         if (d->character != NULL)
	 {
            if (IS_SET(d->character->mhs, MHS_GLADIATOR))
            {
               sprintf(buf, "%s is victorious in the arena!", d->character->name);
               gladiator_talk_ooc(buf); 
#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif
               gladiator_winner(d->character);
            }
         }
      }
   }
   gladiator_info.playing--;

}

void gladiator_kill( CHAR_DATA *victim, CHAR_DATA *ch )
{
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "%s lands the killing blow on %s!", ch->name,victim->name);
   gladiator_talk_ooc(buf); 

   if(gladiator_info.exper == TRUE)
   {// Restore for percent damages dealt to this player. Sanc/withstand on the killer.
    AFFECT_DATA af;
    DAMAGE_DATA *damages;
    int percent;
   	int total = 0;
   	DESCRIPTOR_DATA *d;
    AFFECT_DATA *afp = NULL;
    if(!IS_NPC(ch))
    {
      if(IS_SET(ch->affected_by,AFF_SANCTUARY))
      {// Find their sanctuary, boost its duration
        afp = ch->affected;
        while(afp != NULL && afp->type != gsn_sanctuary)
          afp = afp->next;
        if(afp != NULL)
        {
          if(afp->duration < 10)
          {
            send_to_char("Your sanctuary has been extended.\n\r", ch);
            afp->duration = 10;// Reset the timer
          }
        }
      }
      if(afp == NULL)
      {// Give them a long sanctuary at their level, couldn't find an existing one
  			act( "$n is surrounded by a white aura.", ch, NULL, NULL, TO_ROOM ,FALSE);
  			send_to_char( "You are surrounded by a white aura.\n\r", ch );
  			af.where     = TO_AFFECTS;
  			af.type      = gsn_sanctuary;
  			af.level     = ch->level;
  			af.duration  = 9;
  			af.location  = APPLY_NONE;
  			af.modifier  = 0;
  			af.bitvector = AFF_SANCTUARY;
  			affect_to_char( ch, &af );
      }
      afp = NULL;
      if(IS_SET(ch->affected_by,AFF_WITHSTAND_DEATH))
      {// Find their withstand, boost its duration
        afp = ch->affected;
        while(afp != NULL && afp->type != gsn_withstand_death)
          afp = afp->next;
        if(afp != NULL)
        {
          if(afp->duration < 12)
          {
            send_to_char("Your withstand death has been extended.\n\r", ch);
            afp->duration = 12;// Reset the timer
          }
        }
      }
      if(afp == NULL)
      {
        af.where     = TO_AFFECTS;
        af.type      = gsn_withstand_death;
        af.level     = ch->level;
        af.duration  = 12;
        af.location  = APPLY_NONE;
        af.modifier  = 0;
        af.bitvector = AFF_WITHSTAND_DEATH;
        affect_to_char( ch, &af );
        send_to_char( "You feel like you can withstand death itself.\n\r", ch );
        act( "$n looks more powerful than death.", ch, NULL, NULL, TO_ROOM ,FALSE);
      }
    }
     for(damages = victim->damaged; damages != NULL; damages = damages->next)
       total += damages->damage;
     if(total > 0)
     {/* Now find each person and give them a reward based on % damage dealt */
       for(damages = victim->damaged; damages != NULL; damages = damages->next)
       {
         if(damages->source != NULL)
         {/* Find the gladiator with this name */
           percent = damages->damage * 100 / total;
	  if(percent <= 0)
	    continue;// Too little to heal
          for(d = descriptor_list; d != NULL; d = d->next)
           {
            if (d->character != NULL)
             {
               if (IS_SET(d->character->mhs, MHS_GLADIATOR) && !str_cmp(d->character->name, damages->source))
               {
		if(percent > 30)
		   percent = 30;
                 d->character->hit = UMIN(d->character->hit + d->character->max_hit * percent / 100, d->character->max_hit);
                 d->character->mana = UMIN(d->character->mana + d->character->max_mana * percent / 100, d->character->max_mana);
                 d->character->move = UMIN(d->character->move + d->character->max_move * percent / 100, d->character->max_move);
                 sprintf(buf, "The gods {Wrestore{x your strength for your damage to %s!\n\r",victim->name);
                 send_to_char(buf, d->character);
               }
             }
           }
         }
       }
    }
   }

#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif
   raw_kill( victim,ch );
   if(gladiator_info.type == 1 )
   {
      REMOVE_BIT(victim->mhs, MHS_GLADIATOR);
      victim->clan = victim->pcdata->save_clan;
      victim->pcdata->save_clan = 0;
   }
   victim->hit  = victim->max_hit;
   victim->mana = victim->max_mana;
   victim->move = victim->max_move;
   victim->pcdata->quit_time = 0; 
   victim->position = POS_SLEEPING;
   victim->pcdata->sac = (victim->class == class_lookup("paladin")) ? 600:300;
   if(victim->class == class_lookup("crusader"))
      victim->pcdata->sac = 400;
   if(HAS_KIT(victim,"bishop"))
      victim->pcdata->sac += 100;
   update_pos(victim);
   if(gladiator_info.type == 1)
   {
      gladiator_info.playing--;
      if (ch != victim)
         ch->pcdata->gladiator_data[GLADIATOR_KILLS] += 1;

      if (gladiator_info.playing == 1)
      {
         DESCRIPTOR_DATA *d;
         for(d = descriptor_list; d != NULL; d = d->next)
         {
            if (d->character != NULL)
	    {
               if (IS_SET(d->character->mhs, MHS_GLADIATOR))
               {
                  sprintf(buf, "%s is victorious in the arena!", d->character->name);
                  gladiator_talk_ooc(buf); 
#ifdef CODETEST
       gladbuffer = new_buf();
       add_buf(gladbuffer,glad_qnote->text);
       add_buf(gladbuffer,buf);
       free_string(glad_qnote->text);
       glad_qnote->text = str_dup(buf_string(gladbuffer));
       free_buf(gladbuffer);
#endif

                  gladiator_winner(d->character);
               }
            }
         }
      }
   }

   if(gladiator_info.type == 2 || gladiator_info.type == 3)
   {
      ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS] += 1;
      if(ch->pcdata->gladiator_team == 1)
	 gladiator_info.gladiator_score++;
      else
	 gladiator_info.barbarian_score++;
   }
   return;
}

void do_odds ( CHAR_DATA *ch , char *argument )
{

  DESCRIPTOR_DATA *d;
  char buf[MAX_STRING_LENGTH];
  char buf1[60];
  char buf2[10];
  int this_odds = 0;
  int odds = 0;
  int thiswinloss, gladadj, winlossavg, avg_level; 

    if(gladiator_info.started != TRUE )
    {
    send_to_char("There is no event running currently.\n\r", ch);
    return;
    }
   
   if ( gladiator_info.num_of_glads < 1 )
     {
     send_to_char("There are no gladiators yet.\n\r", ch);
     return;
     }

    sprintf(buf, "%-12s\t%-10s\t%-10s\t%-10s\n\r", "NAME", "ODDS", "PLAYED", "WON"); 
    avg_level = (gladiator_info.total_levels  * 100 )/ gladiator_info.num_of_glads;   

    for(d = descriptor_list; d != NULL; d = d->next)
      {
         if (d->character != NULL)
	 {
            if (IS_SET(d->character->mhs, MHS_GLADIATOR))
            {
             odds = (gladiator_info.num_of_glads  * avg_level /
		   ( d->character->level));
             

	     if( d->character->pcdata->gladiator_data[GLADIATOR_VICTORIES] != 0  &&
		 d->character->pcdata->gladiator_data[GLADIATOR_PLAYS] >= 1)
             {
	       winlossavg = (gladiator_info.total_wins  * 100) /
			    gladiator_info.total_plays;


	       thiswinloss = (d->character->pcdata->gladiator_data[GLADIATOR_VICTORIES] * 100  /
			     d->character->pcdata->gladiator_data[GLADIATOR_PLAYS]);
	
	       gladadj = (thiswinloss   *100 / winlossavg);
               

	       this_odds = ((odds/200) + ((odds/2)/gladadj));
	       this_odds = URANGE(1, this_odds, 99);

	       sprintf(buf2, "%7d :1", this_odds);
	     }
             else
             {
	       odds=  URANGE(1, odds/100, 99);
	       sprintf(buf2, "%7d :1", odds);
	     }

             sprintf(buf1, "%-12s\t%-10s\t%-10d\t%-10d\n\r" ,d->character->name,
	     buf2, d->character->pcdata->gladiator_data[GLADIATOR_PLAYS],
	     d->character->pcdata->gladiator_data[GLADIATOR_VICTORIES]);

	     strcat(buf, buf1);
	    }
	 }
      }

      send_to_char(buf, ch);
      return;
}


void do_gstatus( CHAR_DATA *ch, char *argument)
{
   char buf[MAX_STRING_LENGTH];
   bool fFirst;
   DESCRIPTOR_DATA *d;
   sh_int percent;

   if(gladiator_info.started != TRUE )
   {
      send_to_char("There is no event running currently.\n\r", ch);
      return;
   }


   if(gladiator_info.time_left > 0)
     sprintf(buf, "%d ticks remain until the betting begins.\n\r", gladiator_info.time_left);
   else if(gladiator_info.bet_counter > 0)
     sprintf(buf, "%d ticks of betting remain.\n\r", gladiator_info.bet_counter);
   else if(gladiator_info.team_counter > 0)
     sprintf(buf, "%d ticks remain in this combat.\n\r", gladiator_info.team_counter);
   else
     sprintf(buf, "The battle rages on with %d Gladiators still remaining.\n\r", gladiator_info.playing);
   send_to_char(buf, ch);
   if (IS_SET(ch->mhs,MHS_GLADIATOR))
   {
//      send_to_char("Looking for every advantage you can get?.\n\r",ch);
      return;
   }

   fFirst = TRUE;
   for (d = descriptor_list; d != NULL; d = d->next)
   {
      if (d->character != NULL)
      {
         if (IS_SET(d->character->mhs, MHS_GLADIATOR))
	 {
	    if(fFirst)
	    {
               strcpy( buf, d->character->name);
	       fFirst = FALSE;
	    }
	    else
               strcat( buf, d->character->name);

            if ( d->character->max_hit > 0 )
               percent = ( 100 * d->character->hit ) / d->character->max_hit;
            else
               percent = -1;

            if (percent >= 100) 
               strcat( buf, " is in excellent condition.\n\r");
            else if (percent >= 90) 
               strcat( buf, " has a few scratches.\n\r");
            else if (percent >= 75) 
               strcat( buf," has some small wounds and bruises.\n\r");
            else if (percent >=  50) 
               strcat( buf, " has quite a few wounds.\n\r");
            else if (percent >= 30)
               strcat( buf, " has some big nasty wounds and scratches.\n\r");
            else if (percent >= 15)
               strcat ( buf, " looks pretty hurt.\n\r");
            else if (percent >= 0 )
               strcat (buf, " is in awful condition.\n\r");
            else
               strcat(buf, " is bleeding to death.\n\r");
 
            buf[0] = UPPER(buf[0]);
	 }
      }
   }
   send_to_char( buf, ch );
   return;
}

