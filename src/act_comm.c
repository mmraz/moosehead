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
#include "recycle.h"
#include "tables.h"
#include "interp.h"
#include "lookup.h"
#include "gladiator.h"

/* command procedures needed */
//DECLARE_DO_FUN(do_quit  );

/* In comm.c */
void count_clanners(void);

/* Locals */
void reclass		args( ( CHAR_DATA *ch, int class, bool fPenalty ) );

/* Externals */
void remove_highlander  args (( CHAR_DATA *ch, CHAR_DATA *victim));

// New code to transfer (Also anywhere with timestamp)
void send_timestamp(CHAR_DATA *ch, bool send_now, bool global)
{
  char timestamp[50];
  struct tm * timeinfo;
  time_t offset;
  char display[50];
  if(IS_NPC(ch))
    return;

  if(!IS_SET(ch->pcdata->timestamps, TIMESTAMP_SHOW))
    return;

  if((global && !IS_SET(ch->pcdata->timestamps, TIMESTAMP_GLOBAL)
    && IS_SET(ch->pcdata->timestamps, TIMESTAMP_TELLS)) ||
    (!global && !IS_SET(ch->pcdata->timestamps, TIMESTAMP_TELLS)))
      return;

  offset = current_time +
    (time_t)((ch->pcdata->timestamps & TIMESTAMP_MASK) * 3600);
  
  if(ch->pcdata->timestamp_color && strlen(ch->pcdata->timestamp_color) >= 3)
  {
    sprintf(display, "{%c[{%c%%H{%c:{%c%%M{%c]{x ",
      ch->pcdata->timestamp_color[0], ch->pcdata->timestamp_color[1],
      ch->pcdata->timestamp_color[2], ch->pcdata->timestamp_color[1],
      ch->pcdata->timestamp_color[0]);
  }
  else
    strcpy(display, "{y[{y%H{y:{y%M{y]{x ");
    

  timeinfo = localtime(&offset);
  strftime(timestamp, 49, display, timeinfo);
  if(send_now)
    send_to_char(timestamp, ch);
  else
    add_buf(ch->pcdata->buffer, timestamp);
}

bool check_color_code(char letter)
{
  switch(letter)
  {
    case 'r':
    case 'R':
    case 'b':
    case 'B':
    case 'g':
    case 'G':
    case 'y':
    case 'Y':
    case 'D':
    case 'W':
    case 'c':
    case 'C':
    case 'm':
    case 'M':
    case 'x': return TRUE;
  }
  return FALSE;
}

void do_timestamps(CHAR_DATA *ch, char *argument)
{
  char buf[255], arg[255];
  int number, i;
  if(IS_NPC(ch))
    return;
  if(!argument[0])
  {
    send_to_char("Invalid timestamp option, see 'help timestamps' for commands.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(!str_prefix(arg, "show"))
  {
    send_to_char("Timestamps on channels enabled.\n\r", ch);
    ch->pcdata->timestamps |= TIMESTAMP_SHOW;
    return;
  }
  if(!str_prefix(arg, "hide"))
  {
    send_to_char("Timestamps on channels disabled.\n\r", ch);
    if(IS_SET(ch->pcdata->timestamps, TIMESTAMP_SHOW))
      ch->pcdata->timestamps ^= TIMESTAMP_SHOW;
    return;
  }
  if(!str_prefix(arg, "set"))
  {
    argument = one_argument(argument, arg);
    if(!is_number(arg) || (number = atoi(arg)) < 0 || number > 23)
    {
      send_to_char("Timestamp offset must be a number from 0 to 23.\n\r", ch);
      return;
    }
    sprintf(buf, "Timestamps on channels will now show with a %d hour offset.\n\r", number);
    send_to_char(buf, ch);
    ch->pcdata->timestamps &= TIMESTAMP_UPPERMASK;
    ch->pcdata->timestamps |= number;
    ch->pcdata->timestamps |= TIMESTAMP_SHOW;
    return;
  }
  if(!str_prefix(arg, "type"))
  {
    argument = one_argument(argument, arg);
    if(!str_prefix(arg, "all"))
    {
      ch->pcdata->timestamps |= TIMESTAMP_GLOBAL + TIMESTAMP_TELLS;
      send_to_char("Timestamps will show for all message types.\n\r", ch);
      ch->pcdata->timestamps |= TIMESTAMP_SHOW;
      return;
    }
    if(!str_prefix(arg, "global"))
    {
      if(IS_SET(ch->pcdata->timestamps, TIMESTAMP_TELLS))
        ch->pcdata->timestamps ^= TIMESTAMP_TELLS;
      ch->pcdata->timestamps |= TIMESTAMP_GLOBAL;
      send_to_char("Timestamps will show for only global messages.\n\r", ch);
      ch->pcdata->timestamps |= TIMESTAMP_SHOW;
      return;
    }
    if(!str_prefix(arg, "tells"))
    {
      if(IS_SET(ch->pcdata->timestamps, TIMESTAMP_GLOBAL))
        ch->pcdata->timestamps ^= TIMESTAMP_GLOBAL;
      ch->pcdata->timestamps |= TIMESTAMP_TELLS;
      send_to_char("Timestamps will show for only personal messages.\n\r", ch);
      ch->pcdata->timestamps |= TIMESTAMP_SHOW;
      return;
    }
    send_to_char("Valid type options are: all, global, or tells.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "color"))
  {
    if(strlen(argument) != 3)
    {
      if(!strcmp(argument, "none"))
      {
        send_to_char("Timestamp color reset.\n\r", ch);
        clear_string(&ch->pcdata->timestamp_color, NULL);
      }
      send_to_char("To set a timestamp color you must use three letters.\n\r", ch);
      return;
    }
    for(i = 0; i < 3; i++)
    {
      if(!check_color_code(argument[i]))
      {
        sprintf(buf, "%c is not a valid color code.\n\r", argument[i]);
        send_to_char(buf, ch);
        return;
      }
    }
    clear_string(&ch->pcdata->timestamp_color, argument);
    sprintf(buf, "Your timestamps are now colored {%c[{%c##{%c:{%c##{%c]{x\n\r",
      ch->pcdata->timestamp_color[0], ch->pcdata->timestamp_color[1],
      ch->pcdata->timestamp_color[2], ch->pcdata->timestamp_color[1],
      ch->pcdata->timestamp_color[0]);
    send_to_char(buf, ch);
    ch->pcdata->timestamps |= TIMESTAMP_SHOW;
    return;
  }
  send_to_char("Invalid timestamp option, see 'help timestamps' for commands.\n\r", ch);
}

/* Given two chars, send the inviso message */
void channel_vis_status( CHAR_DATA *ch, CHAR_DATA *victim )
{ 
    char buf[MAX_STRING_LENGTH];    
    if ( !IS_IMMORTAL(ch) || 
       ( !ch->invis_level && !ch->incog_level ) ||	
         !can_see(victim,ch,TRUE) ||	
         victim == ch )
    		return;    

    sprintf(buf,"({W%s{x@{W%d{x) ",
	ch->invis_level ? 
	  (IS_SET(victim->display,DISP_BRIEF_WHOLIST)?"W":"Wizi") : 
	  (IS_SET(victim->display,DISP_BRIEF_WHOLIST)?"I":"Incog"),
	ch->invis_level ? ch->invis_level : ch->incog_level);    

    send_to_char(buf,victim);    
    return;
}

/* RT code to delete yourself */

void do_delet( CHAR_DATA *ch, char *argument)
{
    send_to_char("You must type the full command to delete yourself.\n\r",ch);
    return;
}

void do_delete( CHAR_DATA *ch, char *argument)
{
   char strsave[MAX_INPUT_LENGTH];

   if (IS_NPC(ch))
  return;
  
   if (ch->pcdata->confirm_delete)
   {
  if (argument[0] != '\0')
  {
      send_to_char("{RDelete status removed.{x\n\r",ch);
      ch->pcdata->confirm_delete = FALSE;
      return;
  }
  else
  {
      sprintf(strsave, "{R$N at level %d turns $Mself into line noise.{x", ch->level);
      wiznet(strsave,ch,NULL,0,0,0);
      sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );
      if(ch->pcdata->clan_info)
      {
        remove_clan_member(ch->pcdata->clan_info);
        free_clan_char(ch->pcdata->clan_info);
        ch->pcdata->clan_info = NULL;
      }
      do_quit(ch,"");
      unlink(strsave);
      return;
  }
    }

    if (argument[0] != '\0')
    {
  send_to_char("Just type delete. No argument.\n\r",ch);
  return;
    }

    send_to_char("Type delete again to confirm this command.\n\r",ch);
    send_to_char("{RWARNING:{x this command is irreversible.\n\r",ch);
    send_to_char("Typing delete with an argument will undo delete status.\n\r",
  ch);
    ch->pcdata->confirm_delete = TRUE;
    wiznet("$N is contemplating deletion.",ch,NULL,0,0,get_trust(ch));
}

void do_reclas( CHAR_DATA *ch, char *argument)
{
    send_to_char("You must type the full command to reclass.\n\r",ch);
    return;
}
      
void do_reclass( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    int i, class;

send_to_char("Sorry, reclassing is currently disabled.\n\r", ch);
return;

    if ( class_table[ch->class].reclass )
    {
       send_to_char ("You are already a \"reclass\" class.  You cannot reclass again.\r\n", ch);
       return;
    }


    if ( !HAS_MHS(ch,MHS_OLD_RECLASS) 
	 && ch->level <= 50 && ch->class == class_lookup("elementalist") )
    {
	send_to_char("You must be at least level 51 to reclass.\n\r",ch);
	return;
    }

    if ( !HAS_MHS(ch,MHS_OLD_RECLASS) && ch->level <= 25 )
    {
	send_to_char("You must be at least level 26 to reclass.\n\r",ch);
	return;
    }

    one_argument(argument,arg);
    if( arg[0] == '\0' )
    {
	send_to_char("Syntax: reclass <class>\n\r",ch);
	send_to_char("        reclass check\n\r",ch);
        return;
    }

    if ( !str_cmp(arg,"check") )
    {
    /*
	int penalty;
	char buf[MAX_STRING_LENGTH];
	*/

	if ( class_table[ch->class].reclass )
	{
	    send_to_char("You already reclassed.\n\r",ch);
	    return;
	}

	send_to_char("Available reclasses: ",ch);
   	for ( i = 0 ; i< MAX_CLASS ; i++ )
	{
	    if ( !class_table[i].reclass )
		continue;

	   if( class_table[i].allowed[0] == ch->class ||
	       class_table[i].allowed[1] == ch->class )
	       send_to_char( class_table[i].name, ch );
	   else
	   if ( ch->class == class_lookup("elementalist") &&
		i != class_lookup("berzerker") )
	       send_to_char( class_table[i].name, ch );
	   else
	   continue;

	   send_to_char(" ", ch);
	}

/*
	sprintf(buf, "\n\rCreation Point Offset: %2d\n\r",  
                    UMAX(0, (int) (80-ch->pcdata->points) / 4 ) );
	send_to_char(buf,ch);
	penalty =  UMAX(0, (int) (51-ch->level) / 3 );
        penalty += UMAX(0, (int) (30-ch->level) );
	sprintf(buf, "Level Offset:          %2d\n\r", penalty);
	send_to_char(buf,ch);
	send_to_char("-------------         ----\n\r",ch);
        sprintf(buf, "Total Penalty:         %2d\n\r",
		penalty + UMAX(0, (int) (80-ch->pcdata->points) /4) );
	send_to_char(buf,ch);

	if ( !is_clan(ch) )
		send_to_char("You will -NOT- be able to join a clan.\n\r",ch);
	else
		send_to_char("You -WILL- be able to join a clan.\n\r",ch);
		*/
	return;
    }

    class = class_lookup(arg);
    
    if( class == -1 )
    {
	send_to_char("You want to be a WHAT?\n\r",ch);
	return;
    }

    if(IS_SET(ch->mhs,MHS_SHAPESHIFTED) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
    {
      send_to_char("You are not allowed to reclass while shapeshifted.\n\r",ch);
      return;
    }

    if ( HAS_MHS(ch,MHS_OLD_RECLASS) )
    {
	REMOVE_BIT(ch->mhs, MHS_OLD_RECLASS);
	reclass( ch, class, FALSE );
        return;
    }

    if( class_table[ch->class].reclass
	|| !class_table[class].reclass || IS_NPC(ch) )
    {
	send_to_char("You can not reclass to that.\n\r",ch);
	return;
    }

    if ( class_table[class].allowed[0] != ch->class &&
	 class_table[class].allowed[1] != ch->class &&
         ch->class != class_lookup("elementalist") )
    {
	send_to_char("You don't qualify for that reclass.\n\r",ch);
	return;
    }
    else
    if ( class == class_lookup("berzerker") && 
	 ch->class == class_lookup("elementalist") )
    {
	send_to_char("You don't qualify for that reclass.\n\r",ch);
	return;
    }

    if( ch->level < 26 || ch->level > 51 ) 
    {
	send_to_char("You must be between levels 26 and 51 to reclass.\n\r",ch);
	return;
    }

    reclass(ch, class, TRUE );
    return;
}

void reclass( CHAR_DATA *ch, int class, bool fPenalty  )
{
    int i, sn, gn;

    ch->pcdata->old_class = ch->class;
    ch->class = class;

  if(IS_SET(ch->act,PLR_WERE)) REMOVE_BIT(ch->act,PLR_WERE);
  if(IS_SET(ch->act,PLR_VAMP)) REMOVE_BIT(ch->act,PLR_VAMP);
  if(IS_SET(ch->act,PLR_MUMMY)) REMOVE_BIT(ch->act,PLR_MUMMY);
  /* initialize stats */
  for (i = 0; i < MAX_STATS; i++)
      ch->perm_stat[i] = pc_race_table[ch->race].stats[i];
  ch->affected_by = ch->affected_by|race_table[ch->race].aff;
  ch->imm_flags   = ch->imm_flags|race_table[ch->race].imm;
  ch->res_flags   = ch->res_flags|race_table[ch->race].res;
  ch->vuln_flags  = ch->vuln_flags|race_table[ch->race].vuln;
  ch->form        = race_table[ch->race].form;
  ch->parts       = race_table[ch->race].parts;

  ch->pcdata->retrain = 0;
  ch->pcdata->half_train = 0;
  ch->pcdata->half_retrain = 0;
  ch->pcdata->trained_hit = 0;
  ch->pcdata->trained_mana = 0;
  ch->pcdata->trained_move = 0;
  ch->pcdata->pref_stat = 0;
  ch->pcdata->deity_favor_timer = 0;

  /* Remove specialization if any */
  ch->pcdata->specialize = 0;
  ch->kit = 0;
  ch->species_enemy = 0;

  /* add skills */
  for (i = 0; i < 5; i++)
  {
      if (pc_race_table[ch->race].skills[i] == NULL)
    break;
      group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
  }

/*
  if ( fPenalty )
  {
  ch->pcdata->points = UMAX(0, (int) (80 - ch->pcdata->points) / 4 );
  ch->pcdata->points += UMAX(0, (int) (51  - ch->level) / 3 );
  ch->pcdata->points += UMAX(0, (int) (30 - ch->level) );
  ch->pcdata->points += pc_race_table[ch->race].points;
  }
  else
  */

  ch->pcdata->points = pc_race_table[ch->race].points;

  ch->size = pc_race_table[ch->race].size;

    while ( ch->flash_affected )
        flash_affect_remove( ch, ch->flash_affected,APPLY_BOTH );
    while ( ch->affected )
        affect_remove( ch, ch->affected,APPLY_BOTH );
    ch->affected_by = 0;

    ch->pcdata->perm_hit =20;
    ch->max_hit = 20;
    ch->wimpy = 5;
    ch->practice = 5;
    ch->train = 3;
    ch->pcdata->perm_mana = 100;
    ch->max_mana = 100;
    ch->pcdata->perm_move = 100;
    ch->max_move = 100;
    ch->level = 1;
/*
    ch->played = ch->pcdata->last_level * 1800;
    ch->redid = ch->played;
*/
    ch->played = 0;
    ch->exp = 0;
 /*
    if ( is_clan(ch) ) 
	 SET_BIT(ch->act,PLR_CANCLAN);
	 */
    ch->clan = 0;
    ch->kit = 0;
    ch->pcdata->rank = 0;
    ch->pcdata->sac = 0;
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
        if(skill_table[sn].name != NULL)
        ch->pcdata->learned[sn] = 0;
    }

    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
	gn_remove(ch,gn);
    }
    raw_kill(ch,ch);
    send_to_char("You will now be forced to quit. Upon reconnecting you "
		 "will begin the character generation process in your new "
		 "class.  If you loose link during this process, see an "
		 "IMM as soon as possible.\n\r",ch);
    SET_BIT(ch->act,PLR_RECLASS);
    /*
    ch->perm_stat[class_table[ch->class].attr_prime] += 3; 
    ch->perm_stat[class_table[ch->class].attr_second] += 2;
    */
    ch->trumps = 0;
    REMOVE_BIT(ch->act,PLR_KILLER);
    REMOVE_BIT(ch->act,PLR_THIEF);
    REMOVE_BIT(ch->wiznet,PLR_RUFFIAN);
    do_quit(ch,"");

  return;
}


void do_color( CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    bool acted = FALSE;

    argument = one_argument(argument,arg);
    one_argument(argument,arg2);
    if( arg[0] == '\0' )
    {
	send_to_char("You must specify ON or OFF.\n\r",ch);
	return;
    }

    if(!str_cmp(arg,"on"))
    {
	if(!IS_SET(ch->display,DISP_COLOR))
	    SET_BIT(ch->display,DISP_COLOR);
	send_to_char("Color {CENABLED{x.\n\r",ch);
	acted = TRUE;
    }

    if(!str_cmp(arg,"off"))
    {
	if(IS_SET(ch->display,DISP_COLOR))
	    REMOVE_BIT(ch->display,DISP_COLOR);
	send_to_char("Color {RDISABLED{x.\n\r",ch);
	acted = TRUE;
    }

    if(acted == FALSE)
	send_to_char("That's not a valid option for color.\n\r",ch);

    return;
}

/* RT code to display channel status */

void do_channels( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];

    /* lists all channels and their status */
    send_to_char("{W   channel     status{x\n\r",ch);
    send_to_char("---------------------\n\r",ch);
 
    send_to_char("gossip         ",ch);
    if (!IS_SET(ch->comm,COMM_NOGOSSIP))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    send_to_char("OOC            ",ch);
    if (!IS_SET(ch->comm,COMM_NOOOC))
	send_to_char("{CON{x\n\r",ch);
    else
	send_to_char("{ROFF{x\n\r",ch);

    send_to_char("clan gossip    ",ch);
    if (!IS_SET(ch->comm,COMM_NOAUCTION))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    send_to_char("music          ",ch);
    if (!IS_SET(ch->comm,COMM_NOMUSIC))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    send_to_char("Q/A            ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUESTION))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    send_to_char("Quest          ",ch);
    if (!IS_SET(ch->comm,COMM_NOQUOTE))
  send_to_char("{CON{x\n\r",ch);
    else
  send_to_char("{ROFF{x\n\r",ch);

    send_to_char("grats          ",ch);
    if (!IS_SET(ch->comm,COMM_NOGRATS))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    send_to_char("gladiator      ",ch);
    if (!IS_SET(ch->comm,COMM_NOGLADIATOR))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    if (IS_IMMORTAL(ch))
    {
      send_to_char("god channel    ",ch);
      if(!IS_SET(ch->comm,COMM_NOWIZ))
  send_to_char("{CON{x\n\r",ch);
      else
  send_to_char("{ROFF{x\n\r",ch);
    }

/*
    send_to_char("shouts         ",ch);
    if (!IS_SET(ch->comm,COMM_SHOUTSOFF))
      send_to_char("ON\n\r",ch);
    else
      send_to_char("OFF\n\r",ch);
*/

    send_to_char("deaf           ",ch);
    if (!IS_SET(ch->comm,COMM_DEAF))
  send_to_char("{ROFF{x\n\r",ch);
    else
  send_to_char("{CON{x\n\r",ch);

    send_to_char("quiet mode     ",ch);
    if (IS_SET(ch->comm,COMM_QUIET))
      send_to_char("{CON{x\n\r",ch);
    else
      send_to_char("{ROFF{x\n\r",ch);

    if (IS_SET(ch->comm,COMM_AFK))
  send_to_char("You are {CAFK{x.\n\r",ch);

    if (IS_SET(ch->comm,COMM_SNOOP_PROOF))
  send_to_char("You are immune to snooping.\n\r",ch);
   
    if (ch->ignoring != NULL)
    {
	sprintf(buf,"You are ignoring {W%s{x.\n\r",ch->ignoring->name);
	send_to_char(buf,ch);
    }
    else
    {
	send_to_char("You are not ignoring anyone.\n\r",ch);
    }

    if (ch->lines != PAGELEN)
    {
  if (ch->lines)
  {
      sprintf(buf,"You display %d lines of scroll.\n\r",ch->lines+2);
      send_to_char(buf,ch);
  }
  else
      send_to_char("Scroll buffering is off.\n\r",ch);
    }

    if (ch->prompt != NULL)
    {
  sprintf(buf,"Your current prompt is: %s\n\r",ch->prompt);
  send_to_char(buf,ch);
    }

    if(!IS_NPC(ch) && IS_SET(ch->pcdata->timestamps, TIMESTAMP_SHOW))
    {
      sprintf(buf, "Timestamps with a %d hour offset show", 
        (ch->pcdata->timestamps & TIMESTAMP_MASK));
      send_to_char(buf, ch);
      if(IS_SET(ch->pcdata->timestamps, TIMESTAMP_GLOBAL | TIMESTAMP_TELLS))
        send_to_char(" on all messages.\n\r", ch);
      else if(IS_SET(ch->pcdata->timestamps, TIMESTAMP_TELLS))
        send_to_char(" on personal messages.\n\r", ch);
      else
        send_to_char(" on global channels.\n\r", ch);
    }
    else
      send_to_char("Timestamps are not being shown.\n\r", ch);

    if (IS_SET(ch->comm,COMM_SILENCE))
      send_to_char("You are silenced to spam.\n\r",ch);
  
    if (IS_SET(ch->comm,COMM_NOTELL))
      send_to_char("You cannot use tell.\n\r",ch);
 
    if (IS_SET(ch->comm,COMM_NOCHANNELS))
     send_to_char("You cannot use channels.\n\r",ch);

    if (IS_SET(ch->comm,COMM_NOEMOTE))
      send_to_char("You cannot show emotions.\n\r",ch);

}

/* RT deaf blocks out all shouts */

void do_deaf( CHAR_DATA *ch, char *argument)
{
    
   if (IS_SET(ch->comm,COMM_DEAF))
   {
     send_to_char("You can now hear tells again.\n\r",ch);
     REMOVE_BIT(ch->comm,COMM_DEAF);
   }
   else 
   {
     send_to_char("From now on, you won't hear tells.\n\r",ch);
     SET_BIT(ch->comm,COMM_DEAF);
   }
}

/* RT quiet blocks out all communication */

void do_quiet ( CHAR_DATA *ch, char * argument)
{
    if (IS_SET(ch->comm,COMM_QUIET))
    {
      send_to_char("Quiet mode removed.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_QUIET);
    }
   else
   {
     send_to_char("From now on, you will only hear says and emotes.\n\r",ch);
     SET_BIT(ch->comm,COMM_QUIET);
   }
}

/* afk command */

void do_afk ( CHAR_DATA *ch, char * argument)
{
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->comm,COMM_AFK))
    {
      send_to_char("AFK mode removed. Type 'replay' to see tells.\n\r",ch);
	if(!IS_NPC(ch))
	{
	  if(ch->pcdata->afk_counter == 1)
	  {
      sprintf(buf,"You have %d message waiting.\n\r",ch->pcdata->afk_counter);
	  }else{
      sprintf(buf,"You have %d messages waiting.\n\r",ch->pcdata->afk_counter);
	  }
      send_to_char(buf,ch);
	}
      act("$n returns from {CAFK{x.",ch,NULL,NULL,TO_ROOM,TRUE);
      REMOVE_BIT(ch->comm,COMM_AFK);
    }
   else
   {
     send_to_char("You are now in AFK mode.\n\r",ch);
     act("$n goes {CAFK{x.",ch,NULL,NULL,TO_ROOM,TRUE);
     SET_BIT(ch->comm,COMM_AFK);
   }
}

void do_replay (CHAR_DATA *ch, char *argument)
{
    if (IS_NPC(ch))
    {
  send_to_char("You can't replay.\n\r",ch);
  return;
    }

    if ( ch->pcdata->buffer == NULL )
    {
	send_to_char("You have no tells buffered.\n\r",ch);
	return;
    }


    page_to_char(buf_string(ch->pcdata->buffer),ch);
    ch->pcdata->afk_counter = 0;
    clear_buf(ch->pcdata->buffer);
}

/* RT auction rewritten in ROM style */
void do_auction( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOAUCTION))
      {
  send_to_char("Clan Gossip channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOAUCTION);
      }
      else
      {
  send_to_char("Clan Gossip channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOAUCTION);
      }
    }
    else  /* auction message sent, turn auction on if it is off */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }

  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;

      }

  REMOVE_BIT(ch->comm,COMM_NOAUCTION);

      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Rclan gossip{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;

  victim = d->original ? d->original : d->character;

  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOAUCTION) &&
       !IS_SET(victim->comm,COMM_QUIET) 
	&& (victim->ignoring != ch) )

  {
      send_timestamp(victim, TRUE, TRUE);
      channel_vis_status(ch,victim);
      act_new("$n {Rclan gossips{x '$t'",
        ch,argument,d->character,TO_VICT,POS_DEAD,TRUE);
  }
      }
    }
}

void do_bitch( CHAR_DATA *ch, char *argument )
{
   char buf [MAX_STRING_LENGTH];
   DESCRIPTOR_DATA *d;

   if ( IS_SET(ch->affected_by, AFF_CHARM) )
      return;

   if (argument[0] == '\0')
   {
      if (IS_SET(ch->comm, COMM_NOBITCH))
      {
         send_to_char("Bitch channel is now ON.\n\r",ch);
         REMOVE_BIT(ch->comm,COMM_NOBITCH);
      }
      else
      {
         send_to_char("Bitch channel is now OFF.\n\r",ch);
         SET_BIT(ch->comm, COMM_NOBITCH);
      }
   }
   else
   {
      if (IS_SET(ch->comm,COMM_QUIET))
      {
         send_to_char("You can't bitch quietly.  Turn quiet mode off first.\n\r",ch);
         return;
      }
      if (IS_SET(ch->comm,COMM_NOCHANNELS))
      {
         send_to_char("You've done enough bitching.  The gods won't let you bitch more.\n\r",ch);
         return;
      }

      REMOVE_BIT(ch->comm, COMM_NOBITCH);

      send_timestamp(ch, TRUE, TRUE);
      sprintf(buf, "You {Mbitch{x '%s'\n\r", argument);
      send_to_char(buf,ch);
      for (d=descriptor_list; d != NULL; d = d->next )
      {
         CHAR_DATA *victim;
         victim = d->original ? d->original : d->character;
      
         if (d->connected == CON_PLAYING && d->character != ch
          && !IS_SET(victim->comm,COMM_NOBITCH) && !IS_SET(victim->comm,COMM_QUIET)
          && (victim->ignoring != ch) )
         {
            send_timestamp(victim, TRUE, TRUE);
            channel_vis_status(ch,victim);
            act_new("$n {Mbitches{x '$t'",ch,argument,d->character,TO_VICT,POS_DEAD,TRUE);
         }
      }
   }
}
 
   



/* RT chat replaced with ROM gossip */
void do_gossip( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGOSSIP))
      {
  send_to_char("Gossip channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
      }
      else
      {
  send_to_char("Gossip channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOGOSSIP);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
 
  }

      REMOVE_BIT(ch->comm,COMM_NOGOSSIP);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Wgossip{x '%s'\n\r", argument );
      send_to_char( buf, ch );

      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( (d->connected == CON_PLAYING) &&
       (d->character != ch) &&
       (!IS_SET(victim->comm,COMM_NOGOSSIP)) &&
       (!IS_SET(victim->comm,COMM_QUIET))  &&
	(victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new( "$n {Wgossips{x '$t'", 
       ch,argument, d->character, TO_VICT,POS_SLEEPING,TRUE );
  }
      }
    }
}

void do_ooc( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ))
    return;


    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOOOC))
      {
  send_to_char("OOC channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOOOC);
      }
      else
      {
  send_to_char("OOC channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOOOC);
      }
    }
    else  /* gossip message sent, turn gossip on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }

  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;

  }

      REMOVE_BIT(ch->comm,COMM_NOOOC);

      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {COOC{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;

  victim = d->original ? d->original : d->character;

  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOOOC) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new( "$n {COOC's{x '$t'",
       ch,argument, d->character, TO_VICT,POS_SLEEPING,TRUE );
  }
      }
    }
}

void do_grats( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if ( is_affected(ch, gsn_cone_of_silence ))
    return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOGRATS))
      {
  send_to_char("Grats channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOGRATS);
      }
      else
      {
  send_to_char("Grats channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOGRATS);
      }
    }
    else  /* grats message sent, turn grats on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
 
  }
 
      REMOVE_BIT(ch->comm,COMM_NOGRATS);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Ygrats{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOGRATS) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new( "$n {Ygrats{x '$t'",
       ch,argument, d->character, TO_VICT,POS_SLEEPING,TRUE );
  }
      }
    }
}

void do_quest( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

 
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUOTE))
      {
  send_to_char("Quest channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOQUOTE);
      }
      else
      {
  send_to_char("Quest channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOQUOTE);
      }
    }
    else  /* quest message sent, turn quest on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
 
  }
 
      REMOVE_BIT(ch->comm,COMM_NOQUOTE);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You quest '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOQUOTE) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new( "$n quests '$t'",
       ch,argument, d->character, TO_VICT,POS_SLEEPING,TRUE );
  }
      }
    }
}

/* RT question channel */
void do_question( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

 
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUESTION))
      {
  send_to_char("Q/A channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOQUESTION);
      }
      else
      {
  send_to_char("Q/A channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOQUESTION);
      }
    }
    else  /* question sent, turn Q/A on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
 
  REMOVE_BIT(ch->comm,COMM_NOQUESTION);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Bquestion{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOQUESTION) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new("$n {Bquestions{x '$t'",
      ch,argument,d->character,TO_VICT,POS_SLEEPING,TRUE);
  }
      }
    }
}

/* RT answer channel - uses same line as questions */
void do_answer( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

 
    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOQUESTION))
      {
  send_to_char("Q/A channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOQUESTION);
      }
      else
      {
  send_to_char("Q/A channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOQUESTION);
      }
    }
    else  /* answer sent, turn Q/A on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
 
  REMOVE_BIT(ch->comm,COMM_NOQUESTION);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Banswer{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOQUESTION) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
    send_timestamp(victim, TRUE, TRUE);
    channel_vis_status(ch,victim);
    act_new("$n {Banswers{x '$t'",
      ch,argument,d->character,TO_VICT,POS_SLEEPING,TRUE);
  }
      }
    }
}

/* RT music channel */
void do_music( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
 
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    if (argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOMUSIC))
      {
  send_to_char("Music channel is now ON.\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOMUSIC);
      }
      else
      {
  send_to_char("Music channel is now OFF.\n\r",ch);
  SET_BIT(ch->comm,COMM_NOMUSIC);
      }
    }
    else  /* music sent, turn music on if it isn't already */
    {
  if (IS_SET(ch->comm,COMM_QUIET))
  {
    send_to_char("You must turn off quiet mode first.\n\r",ch);
    return;
  }
 
  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
    send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }
 
  REMOVE_BIT(ch->comm,COMM_NOMUSIC);
 
      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You MUSIC: '%s'\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "$n MUSIC: '%s'", argument );
      for ( d = descriptor_list; d != NULL; d = d->next )
      {
  CHAR_DATA *victim;
 
  victim = d->original ? d->original : d->character;
 
  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm,COMM_NOMUSIC) &&
       !IS_SET(victim->comm,COMM_QUIET) &&
        (victim->ignoring != ch) )
  {
      send_timestamp(victim, TRUE, TRUE);
	channel_vis_status(ch,victim);
      act_new("$n MUSIC: '$t'",
        ch,argument,d->character,TO_VICT,POS_SLEEPING,TRUE);
  }
      }
    }
}


/* MM clan channels */
void do_clantalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

  if(!IS_NPC(ch) && ch->pcdata->clan_info)
  {
    if(ch->pcdata->clan_info->clan->default_clan)
    {
      send_to_char("No one is listening...\n\r",ch);
      return;
    }
  }
  else
  {
    if ( !ch->clan )
    {
  send_to_char("You aren't in a clan.\n\r",ch);
  return;
    }

    if ( clan_table[ch->clan].independent  && clan_table[ch->clan].true_clan )
    {
  send_to_char("No one is listening...\n\r",ch);
  return;
    }

    if( !IS_NPC(ch) && IS_SET(ch->pcdata->clan_flags, CLAN_NO_CHANNEL ))
     {
     send_to_char("You have been forbidden to use the clan channel.\n\r", ch);
     return;
     }
  }
    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOCLAN))
      {
  send_to_char("Clan channel is now ON\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOCLAN);
      }
      else
      {
  send_to_char("Clan channel is now OFF\n\r",ch);
  SET_BIT(ch->comm,COMM_NOCLAN);
      }
      return;
    }

  if (IS_SET(ch->comm,COMM_NOCHANNELS))
  {
   send_to_char("The gods have revoked your channel priviliges.\n\r",ch);
    return;
  }

  REMOVE_BIT(ch->comm,COMM_NOCLAN);

      send_timestamp(ch, TRUE, TRUE);
      sprintf( buf, "You {Gclan{x '%s'\n\r", argument );
      send_to_char( buf, ch );
      sprintf( buf, "$n {Gclans{x '%s'", argument );
    for ( d = descriptor_list; d != NULL; d = d->next )
    { 
	CHAR_DATA *victim;

	victim = d->original ? d->original : d->character;

     if( d->connected != CON_PLAYING ) continue;

      if ( (d->connected == CON_PLAYING) &&
           (d->character != ch) &&
           (is_same_clan(ch,victim) || IS_SET(victim->mhs,MHS_LISTEN) ) &&
           (!IS_SET(d->character->comm,COMM_NOCLAN)) &&
           (!IS_SET(d->character->comm,COMM_QUIET)) &&
        (d->character->ignoring != ch) )
  {
      send_timestamp(victim, TRUE, TRUE);
      if ( !is_same_clan(ch,victim) )
      {
        if(ch->pcdata->clan_info)
	  sprintf(buf,"<%s> ",ch->pcdata->clan_info->clan->name);
        else
	  sprintf(buf,"[%s] ",clan_table[ch->clan].who_name);
	  send_to_char(buf,victim);
      } 

    channel_vis_status(ch,d->character);
    act_new("$n {Gclans{x '$t'",ch,argument,d->character,TO_VICT,POS_DEAD,TRUE);
  }
    }

    return;
}

void do_immtalk( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;

    if ( argument[0] == '\0' )
    {
      if (IS_SET(ch->comm,COMM_NOWIZ))
      {
  send_to_char("Immortal channel is now ON\n\r",ch);
  REMOVE_BIT(ch->comm,COMM_NOWIZ);
      }
      else
      {
  send_to_char("Immortal channel is now OFF\n\r",ch);
  SET_BIT(ch->comm,COMM_NOWIZ);
      } 
      return;
    }

    REMOVE_BIT(ch->comm,COMM_NOWIZ);

    send_timestamp(ch, TRUE, TRUE);
    sprintf( buf, "$n: %s", argument );
    act_new("$n: $t",ch,argument,NULL,TO_CHAR,POS_DEAD,TRUE);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
  if ( (d->connected == CON_PLAYING) && 
       (IS_IMMORTAL(d->character)) && 
       (!IS_SET(d->character->comm,COMM_NOWIZ)) )
  {
      if(d->character == ch)
        continue;
      send_timestamp(d->character, TRUE, TRUE);
	channel_vis_status(ch,d->character);
      act_new("$n: $t",ch,argument,d->character,TO_VICT,POS_DEAD,TRUE);
  }
    }

    return;
}


void do_whisper( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
  send_to_char( "Whisper who what?\n\r", ch );
  return;
    }

		if ( ( victim = get_char_room( ch, arg ) ) == NULL )
		{
		  send_to_char( "They aren't here.\n\r", ch );
		  return;
		}

    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    act( "$n whispers to you '$t'", ch, argument, victim, TO_VICT, FALSE );
    act( "$n whispers something to $N.", ch, argument, victim, TO_NOTVICT, FALSE );
    act( "You whisper to $N '$t'", ch, argument, victim, TO_CHAR, FALSE );
}

void do_sayto( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
  send_to_char( "Sayto who what?\n\r", ch );
  return;
    }

		if ( ( victim = get_char_room( ch, arg ) ) == NULL )
		{
		  send_to_char( "They aren't here.\n\r", ch );
		  return;
		}

    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    act( "$n says to you '$t'", ch, argument, victim, TO_VICT, FALSE );
    act( "$n says to $N '$t'", ch, argument, victim, TO_NOTVICT, FALSE );
    act( "You say to $N '$t'", ch, argument, victim, TO_CHAR, FALSE );
}

void do_say( CHAR_DATA *ch, char *argument )
{
    if ( argument[0] == '\0' )
    {
  send_to_char( "Say what?\n\r", ch );
  return;
    }

    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    act( "$n says '$T'", ch, NULL, argument, TO_ROOM, FALSE );
    act( "You say '$T'", ch, NULL, argument, TO_CHAR, FALSE );
    if(ch->in_room && ch->in_room->vnum == ROOM_VNUM_BOUNTY_PULL)
    {
      if(!str_cmp(argument, "so"))
      {
        send_to_char("A voice {gwhispers{x in your ear, 'So? So, what? Are you here to serve or not?'\n\r", ch);
      }
      else if(!str_suffix("here to serve", argument))
      {
        if(ch->level < 35)
          send_to_char("A voice {gwhispers{x, 'Your spirit is admirable, return when you are more powerful.'\n\r", ch);
        else
        {
          act("$n sinks into the shadow on the floor.", ch, NULL, NULL, TO_ROOM, FALSE);
          if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
          {
            act("$n sinks into the shadow on the floor.", ch->pet, NULL, NULL, TO_ROOM, FALSE);
            send_to_char("You sink into the shadow on the floor and find yourself somewhere else...\n\r\n\r", ch->pet);
            char_from_room(ch->pet);
            clear_mount(ch->pet);
            char_to_room(ch->pet,get_room_index(ROOM_VNUM_BOUNTY_ROOM));
            act("$n appears in the room from a flicker of shadow.", ch, NULL, NULL, TO_ROOM, FALSE);
            do_look(ch->pet, "auto");
          }
          send_to_char("You sink into the shadow on the floor and find yourself somewhere else...\n\r\n\r", ch);
          char_from_room(ch);
          clear_mount(ch);
          char_to_room(ch,get_room_index(ROOM_VNUM_BOUNTY_ROOM));
          act("$n appears in the room from a flicker of shadow.", ch, NULL, NULL, TO_ROOM, FALSE);
          do_look(ch, "auto");
        }
      }
    }
    return;
}


void do_silence( CHAR_DATA *ch, char *argument )
{

    if ( IS_SET( ch->comm, COMM_SILENCE ) )
    {
	send_to_char("Silence removed, brace for spam!\n\r",ch);
	REMOVE_BIT(ch->comm, COMM_SILENCE);
        return;
    }
    else
    {
	send_to_char("Ahhhhhhh, no more spam.\n\r",ch);
	SET_BIT(ch->comm,COMM_SILENCE);
	return;
    }
}

/* removed by Ben ... it's pointless ***
void do_shout( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    if (argument[0] == '\0' )
    {
  if (IS_SET(ch->comm,COMM_SHOUTSOFF))
  {
      send_to_char("You can hear shouts again.\n\r",ch);
      REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);
  }
  else
  {
      send_to_char("You will no longer hear shouts.\n\r",ch);
      SET_BIT(ch->comm,COMM_SHOUTSOFF);
  }
  return;
    }

    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
  send_to_char( "You can't shout.\n\r", ch );
  return;
    }
 
    REMOVE_BIT(ch->comm,COMM_SHOUTSOFF);

    WAIT_STATE( ch, 12 );

    act( "You shout '$T'", ch, NULL, argument, TO_CHAR, FALSE );
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
  CHAR_DATA *victim;

  victim = d->original ? d->original : d->character;

  if ( d->connected == CON_PLAYING &&
       d->character != ch &&
       !IS_SET(victim->comm, COMM_SHOUTSOFF) &&
       !IS_SET(victim->comm, COMM_QUIET) &&
        (d->character->ignoring != ch) ) 
  {
      act("$n shouts '$t'",ch,argument,d->character,TO_VICT,FALSE);
  }
    }

    return;
}
***/


void do_tell( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;

    if ( IS_SET ( ch->comm, COMM_DEAF )) 
    {
  send_to_char ( "You must turn deaf off first.\n\r", ch );
  return;
    }
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( IS_SET(ch->comm, COMM_NOTELL))
    {
  send_to_char( "Your message didn't get through.\n\r", ch );
  return;
    }

    if ( IS_SET(ch->comm, COMM_QUIET) )
    {
  send_to_char( "You must turn off quiet mode first.\n\r", ch);
  return;
    }

    if (IS_SET(ch->comm,COMM_DEAF))
    {
  send_to_char("You must turn off deaf mode first.\n\r",ch);
  return;
    }

    if (IS_AFFECTED(ch,AFF_SLEEP))
    {
  send_to_char("You're too tired.\n\r",ch);
  return;
    }

    argument = one_argument( argument, arg );

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
  send_to_char( "Tell whom what?\n\r", ch );
  return;
    }

    /*
     * Can tell to PC's anywhere, but NPC's only in same room.
     * -- Furey
     */
    if ( ( victim = get_char_online( ch, arg ) ) == NULL
    || ( IS_NPC(victim) && victim->in_room != ch->in_room) )
    {
      if(victim == NULL)
      {
        send_to_char( "They aren't here.\n\r", ch );
        return;
      }
      if(victim && IS_NPC(victim))
      {/* Try to find a player instead */
        DESCRIPTOR_DATA *d;
        /* buf is just temporary */
        int number = number_argument(arg, buf );
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
            if ( d->connected == CON_PLAYING && d->character && !str_prefix(d->character->name, buf))
            {
              number--;
              if(number <= 0)
                break;
            }
        }
        if(d)
          victim = d->character;
        else
        {
          send_to_char( "They aren't here.\n\r", ch );
          return;
        }
      }
    }

    send_timestamp(ch, TRUE, FALSE);
    if ( victim->desc == NULL && !IS_NPC(victim))
    {
  act("$N seems to have misplaced $S link...try again later.",
      ch,NULL,victim,TO_CHAR,TRUE);
  send_timestamp(victim, FALSE, FALSE);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, BOLD);
  sprintf(buf,"%s tells you ",PERS(ch,victim,TRUE));
  buf[0] = UPPER(buf[0]);
  add_buf(victim->pcdata->buffer,buf);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, NORMAL);
  sprintf(buf,"'%s'\n\r",argument);
  add_buf(victim->pcdata->buffer,buf);
  return;
    }

/*
    if ( !(IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL) && !IS_AWAKE(victim) )
    {
  act( "$E can't hear you.", ch, 0, victim, TO_CHAR, TRUE );
  return;
    }
 */ 
    if ((IS_SET(victim->comm,COMM_QUIET) || 
	IS_SET(victim->comm,COMM_DEAF) || (victim->ignoring == ch) )
    && !IS_IMMORTAL(ch))
    {
  act( "$E is not receiving tells.", ch, 0, victim, TO_CHAR, TRUE );
  return;
    }

    if (IS_SET(victim->comm,COMM_AFK))
    {
  if (IS_NPC(victim))
  {
      act("$E is AFK, and not receiving tells.",ch,NULL,victim,TO_CHAR,TRUE);
      return;
  }

  send_timestamp(victim, FALSE, FALSE);
  act( "You tell [{CAFK{x] $N '$t'", ch, argument, victim, TO_CHAR, TRUE );
  /*act("$E is AFK, but your tell will go through when $E returns.",
      ch,NULL,victim,TO_CHAR,TRUE);*/
  victim->pcdata->afk_counter = victim->pcdata->afk_counter + 1;
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, BOLD);
  sprintf(buf,"%s tells you ",PERS(ch,victim,TRUE));                 
  buf[0] = UPPER(buf[0]);
  add_buf(victim->pcdata->buffer,buf);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, NORMAL);
  sprintf(buf,"'%s'\n\r",argument);
  add_buf(victim->pcdata->buffer,buf);
  return;
    }
    if(victim != ch)
      send_timestamp(victim, TRUE, FALSE);
    if(IS_NPC(victim))
      act( "You tell (mob)$N '$t'", ch, argument, victim, TO_CHAR, TRUE );
    else
      act( "You tell $N '$t'", ch, argument, victim, TO_CHAR, TRUE );
    if (IS_SET(victim->comm,COMM_TELL_BEEP))     
    {
      if(IS_SET(victim->display,DISP_COLOR))
        act_new(BOLD"\x07$n tells you"NORMAL" '$t'",
		ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      else
      act_new("\x07$n tells you '$t'",ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
    }
    else
    {
      if(IS_SET(victim->display,DISP_COLOR))
        act_new(BOLD"$n tells you"NORMAL" '$t'",
		ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      else
      act_new("$n tells you '$t'",ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
    }
    /*if(!IS_NPC(victim) && IS_SET(victim->pcdata->new_opt_flags, OPT_REPLYLOCK))
    {
      if(victim->reply == NULL)
      {// Ignore otherwise 
        sprintf(buf, "Reply locked to %s.\n\r", ch->name);
        send_to_char(buf, victim);
        victim->reply = ch;
      }
    }
    else*/
    if(IS_NPC(victim) || !victim->pcdata->rlock_time)
      victim->reply       = ch;

    return;
}

void do_noreply( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d;

  for ( d = descriptor_list; d != NULL; d = d->next )
      {
	if ( d->connected == CON_PLAYING
	   &&  (d->character->reply == ch)
	   &&  (d->character->level <= ch->level) )
	d->character->reply = NULL;
       }
   return;
}

void do_rlock( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char buf[256];
  if(IS_NPC(ch))
    return;
  one_argument(argument, buf);

  if(buf[0] != '\0')
  {
    if ( ( victim = get_char_online( ch, buf ) ) == NULL
      || ( IS_NPC(victim) && victim->in_room != ch->in_room ) )
    {
      send_to_char("They aren't here to lock your replies to.\n\r", ch);
      return;
    }
    ch->reply = victim;
    sprintf(buf, "Your replies are now locked to %s.\n\r", victim->name);
    send_to_char(buf, ch);
    SET_BIT(ch->pcdata->new_opt_flags, OPT_REPLYLOCK);
    ch->pcdata->rlock_time = 15;
    return;
  }
  TOGGLE_BIT(ch->pcdata->new_opt_flags, OPT_REPLYLOCK);
  if(!ch->pcdata->rlock_time)//IS_SET(ch->pcdata->new_opt_flags, OPT_REPLYLOCK))
  {
    if(ch->reply)
    {
      sprintf(buf, "Your reply is locked to %s.\n\r", ch->reply->name);
      send_to_char(buf, ch);
      ch->pcdata->rlock_time = 15;
    }
    else
      send_to_char("You have nobody to lock your reply to.\n\r", ch);
//      send_to_char("Your reply will lock to the next tell you receive.\n\r", ch);
  }
  else
  {
    send_to_char("Your reply is now unlocked.\n\r", ch);
    ch->pcdata->rlock_time = 0;
  }
}

void do_reply( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if(argument[0] == '\0')
    {
      if(ch->reply)
      {
        sprintf(buf, "Your reply is currently going to %s.\n\r", ch->reply->name);
        send_to_char(buf, ch);
      }
      else
        send_to_char("You have nobody on reply currently.\n\r", ch);
      if(!IS_NPC(ch) && ch->pcdata->rlock_time > 0)// IS_SET(ch->pcdata->new_opt_flags, OPT_REPLYLOCK))
        send_to_char("Your replies are currently locked.\n\r", ch);
      return;
    }

    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( IS_SET(ch->comm, COMM_NOTELL) )
    {
  send_to_char( "Your message didn't get through.\n\r", ch );
  return;
    }

    if ( ( victim = ch->reply ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if(!IS_NPC(ch) && ch->pcdata->rlock_time)
      ch->pcdata->rlock_time = 15;

    send_timestamp(ch, TRUE, FALSE);

    if ( victim->desc == NULL && !IS_NPC(victim))
    {
  act("$N seems to have misplaced $S link...try again later.",
      ch,NULL,victim,TO_CHAR,TRUE);
  send_timestamp(victim, FALSE, FALSE);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, BOLD);
  sprintf(buf,"%s tells you ",PERS(ch,victim,TRUE));
  buf[0] = UPPER(buf[0]);
  add_buf(victim->pcdata->buffer,buf);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, NORMAL);
  sprintf(buf,"'%s'\n\r",argument);
  add_buf(victim->pcdata->buffer,buf);
  return;
    }

/**
    if ( !IS_IMMORTAL(ch) && !IS_AWAKE(victim) )
    {
  act( "$E can't hear you.", ch, 0, victim, TO_CHAR, TRUE );
  return;
    }
 **/
    if ((IS_SET(victim->comm,COMM_QUIET) || 
	IS_SET(victim->comm,COMM_DEAF) || (victim->ignoring == ch))
    &&  !IS_IMMORTAL(ch) && !IS_IMMORTAL(victim))
    {
  act_new( "$E is not receiving tells.", ch, 0, victim, TO_CHAR,POS_DEAD,TRUE);
  return;
    }

/**
    if (!IS_IMMORTAL(victim) && !IS_AWAKE(ch))
 **/
    if ( IS_AFFECTED( ch, AFF_SLEEP ) )
  {
  send_to_char( "In your dreams, or what?\n\r", ch );
  return;
    }

    if (IS_SET(victim->comm,COMM_AFK))
    {
  if (IS_NPC(victim))
  {
      act_new("$E is AFK, and not receiving tells.",
    ch,NULL,victim,TO_CHAR,POS_DEAD,TRUE);
      return;
  }
 
  send_timestamp(victim, FALSE, FALSE);
  act_new("You tell [{CAFK{x] $N '$t'",ch,argument,victim,TO_CHAR,POS_DEAD,TRUE);
/*  act_new("$E is AFK, but your tell will go through when $E returns.",
      ch,NULL,victim,TO_CHAR,POS_DEAD,TRUE);*/
  victim->pcdata->afk_counter = victim->pcdata->afk_counter + 1;
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, BOLD);
  sprintf(buf,"%s tells you ",PERS(ch,victim,TRUE));
  buf[0] = UPPER(buf[0]);
  add_buf(victim->pcdata->buffer,buf);
  if(IS_SET(victim->display,DISP_COLOR))
    add_buf(victim->pcdata->buffer, NORMAL);
  sprintf(buf,"'%s'\n\r",argument);
  add_buf(victim->pcdata->buffer,buf);
  return;
    }

    if(victim != ch)
      send_timestamp(victim, TRUE, FALSE);

    act_new("You tell $N '$t'",ch,argument,victim,TO_CHAR,POS_DEAD,TRUE);
    if (IS_SET(victim->comm,COMM_TELL_BEEP))     
      {
      if(IS_SET(victim->display,DISP_COLOR))
        act_new(BOLD"\x07$n tells you"NORMAL" '$t'",
		ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      else
      act_new("\x07$n tells you '$t'",ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      }
    else
      {
      if(IS_SET(victim->display,DISP_COLOR))
        act_new(BOLD"$n tells you"NORMAL" '$t'",
		ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      else
        act_new("$n tells you '$t'",ch,argument,victim,TO_VICT,POS_DEAD,TRUE);
      }
   /* if(!IS_NPC(victim) && IS_SET(victim->pcdata->new_opt_flags, OPT_REPLYLOCK))
    {
      if(victim->reply == NULL)
      {// Ignore otherwise
        sprintf(buf, "Reply locked to %s.\n\r", ch->name);
        send_to_char(buf, victim);
        victim->reply = ch;
      }
    }
    else*/
    if(IS_NPC(victim) || !victim->pcdata->rlock_time)
      victim->reply       = ch;

    return;
}



void do_yell( CHAR_DATA *ch, char *argument )
{

    DESCRIPTOR_DATA *d;
    
    if(ch->in_room->vnum < 0)
    {
      send_to_char("Your hall swallows the sound of your yell.\n\r", ch);
      return;
    }

    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    if(IS_SET(ch->in_room->room_flags,ROOM_ISOLATED))
    {// New isolated code
      send_to_char("The room swallows your yell, nothing can be heard.\n\r", ch);
      return;
    }


    if ( IS_SET(ch->comm, COMM_NOSHOUT) )
    {
  send_to_char( "You can't yell.\n\r", ch );
  return;
    }
 
    if ( argument[0] == '\0' )
    {
  send_to_char( "Yell what?\n\r", ch );
  return;
    }


    act("You yell '$t'",ch,argument,NULL,TO_CHAR,FALSE);
    for ( d = descriptor_list; d != NULL; d = d->next )
    {
  if ( d->connected == CON_PLAYING
  &&   d->character != ch
  &&   d->character->in_room != NULL
  &&   d->character->in_room->area == ch->in_room->area 
  &&   !IS_SET(d->character->comm,COMM_QUIET) 
  &&   (d->character->ignoring != ch) 
  && !IS_SET(d->character->in_room->room_flags,ROOM_ISOLATED) )// New isolated code
  {
      act("$n yells '$t'",ch,argument,d->character,TO_VICT,FALSE);
  }
    }

    return;
}


void do_emote( CHAR_DATA *ch, char *argument )
{
     if ( is_affected(ch, gsn_cone_of_silence ) )
    return;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
  send_to_char( "You can't show your emotions.\n\r", ch );
  return;
    }
 
    if ( argument[0] == '\0' )
    {
  send_to_char( "Emote what?\n\r", ch );
  return;
    }
 
    act( "$n $T", ch, NULL, argument, TO_ROOM, FALSE );
    act( "$n $T", ch, NULL, argument, TO_CHAR, FALSE );
    return;
}


void do_pmote( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    char *letter,*name;
    char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
    int matches = 0;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
  send_to_char( "You can't show your emotions.\n\r", ch );
  return;
    }
 
    if ( argument[0] == '\0' )
    {
  send_to_char( "Emote what?\n\r", ch );
  return;
    }
 
    act( "$n $t", ch, argument, NULL, TO_CHAR, FALSE );

    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
  if (vch->desc == NULL || vch == ch)
      continue;

  if ((letter = strstr(argument,vch->name)) == NULL)
  {
      act("$N $t",vch,argument,ch,TO_CHAR, FALSE);
      continue;
  }

  strcpy(temp,argument);
  temp[strlen(argument) - strlen(letter)] = '\0';
  last[0] = '\0';
  name = vch->name;
  
  for (; *letter != '\0'; letter++)
  { 
      if (*letter == '\'' && matches == strlen(vch->name))
      {
    strcat(temp,"r");
    continue;
      }

      if (*letter == 's' && matches == strlen(vch->name))
      {
    matches = 0;
    continue;
      }
      
      if (matches == strlen(vch->name))
      {
    matches = 0;
      }

      if (*letter == *name)
      {
    matches++;
    name++;
    if (matches == strlen(vch->name))
    {
        strcat(temp,"you");
        last[0] = '\0';
        name = vch->name;
        continue;
    }
    strncat(last,letter,1);
    continue;
      }

      matches = 0;
      strcat(temp,last);
      strncat(temp,letter,1);
      last[0] = '\0';
      name = vch->name;
  }

  act("$N $t",vch,temp,ch,TO_CHAR, FALSE);
    }
  
    return;
}


/*
 * All the posing stuff.
 */
struct  pose_table_type
{
    char *      message[2*MAX_CLASS];
};

const   struct  pose_table_type pose_table      []      =
{
    {
  {
      "You sizzle with energy.",
      "$n sizzles with energy.",
      "You feel very holy.",
      "$n looks very holy.",
      "You perform a small card trick.",
      "$n performs a small card trick.",
      "You show your bulging muscles.",
      "$n shows $s bulging muscles."
  }
    },

    {
  {
      "You turn into a butterfly, then return to your normal shape.",
      "$n turns into a butterfly, then returns to $s normal shape.",
      "You nonchalantly turn wine into water.",
      "$n nonchalantly turns wine into water.",
      "You wiggle your ears alternately.",
      "$n wiggles $s ears alternately.",
      "You crack nuts between your fingers.",
      "$n cracks nuts between $s fingers."
  }
    },

    {
  {
      "Blue sparks fly from your fingers.",
      "Blue sparks fly from $n's fingers.",
      "A halo appears over your head.",
      "A halo appears over $n's head.",
      "You nimbly tie yourself into a knot.",
      "$n nimbly ties $mself into a knot.",
      "You grizzle your teeth and look mean.",
      "$n grizzles $s teeth and looks mean."
  }
    },

    {
  {
      "Little red lights dance in your eyes.",
      "Little red lights dance in $n's eyes.",
      "You recite words of wisdom.",
      "$n recites words of wisdom.",
      "You juggle with daggers, apples, and eyeballs.",
      "$n juggles with daggers, apples, and eyeballs.",
      "You hit your head, and your eyes roll.",
      "$n hits $s head, and $s eyes roll."
  }
    },

    {
  {
      "A slimy green monster appears before you and bows.",
      "A slimy green monster appears before $n and bows.",
      "Deep in prayer, you levitate.",
      "Deep in prayer, $n levitates.",
      "You steal the underwear off every person in the room.",
      "Your underwear is gone!  $n stole it!",
      "Crunch, crunch -- you munch a bottle.",
      "Crunch, crunch -- $n munches a bottle."
  }
    },

    {
  {
      "You turn everybody into a little pink elephant.",
      "You are turned into a little pink elephant by $n.",
      "An angel consults you.",
      "An angel consults $n.",
      "The dice roll ... and you win again.",
      "The dice roll ... and $n wins again.",
      "... 98, 99, 100 ... you do pushups.",
      "... 98, 99, 100 ... $n does pushups."
  }
    },

    {
  {
      "A small ball of light dances on your fingertips.",
      "A small ball of light dances on $n's fingertips.",
      "Your body glows with an unearthly light.",
      "$n's body glows with an unearthly light.",
      "You count the money in everyone's pockets.",
      "Check your money, $n is counting it.",
      "Arnold Schwarzenegger admires your physique.",
      "Arnold Schwarzenegger admires $n's physique."
  }
    },

    {
  {
      "Smoke and fumes leak from your nostrils.",
      "Smoke and fumes leak from $n's nostrils.",
      "A spot light hits you.",
      "A spot light hits $n.",
      "You balance a pocket knife on your tongue.",
      "$n balances a pocket knife on your tongue.",
      "Watch your feet, you are juggling granite boulders.",
      "Watch your feet, $n is juggling granite boulders."
  }
    },

    {
  {
      "The light flickers as you rap in magical languages.",
      "The light flickers as $n raps in magical languages.",
      "Everyone levitates as you pray.",
      "You levitate as $n prays.",
      "You produce a coin from everyone's ear.",
      "$n produces a coin from your ear.",
      "Oomph!  You squeeze water out of a granite boulder.",
      "Oomph!  $n squeezes water out of a granite boulder."
  }
    },

    {
  {
      "Your head disappears.",
      "$n's head disappears.",
      "A cool breeze refreshes you.",
      "A cool breeze refreshes $n.",
      "You step behind your shadow.",
      "$n steps behind $s shadow.",
      "You pick your teeth with a spear.",
      "$n picks $s teeth with a spear."
  }
    },

    {
  {
      "A fire elemental singes your hair.",
      "A fire elemental singes $n's hair.",
      "The sun pierces through the clouds to illuminate you.",
      "The sun pierces through the clouds to illuminate $n.",
      "Your eyes dance with greed.",
      "$n's eyes dance with greed.",
      "Everyone is swept off their foot by your hug.",
      "You are swept off your feet by $n's hug."
  }
    },

    {
  {
      "The sky changes color to match your eyes.",
      "The sky changes color to match $n's eyes.",
      "The ocean parts before you.",
      "The ocean parts before $n.",
      "You deftly steal everyone's weapon.",
      "$n deftly steals your weapon.",
      "Your karate chop splits a tree.",
      "$n's karate chop splits a tree."
  }
    },

    {
  {
      "The stones dance to your command.",
      "The stones dance to $n's command.",
      "A thunder cloud kneels to you.",
      "A thunder cloud kneels to $n.",
      "The Grey Mouser buys you a beer.",
      "The Grey Mouser buys $n a beer.",
      "A strap of your armor breaks over your mighty thews.",
      "A strap of $n's armor breaks over $s mighty thews."
  }
    },

    {
  {
      "The heavens and grass change colour as you smile.",
      "The heavens and grass change colour as $n smiles.",
      "The Burning Man speaks to you.",
      "The Burning Man speaks to $n.",
      "Everyone's pocket explodes with your fireworks.",
      "Your pocket explodes with $n's fireworks.",
      "A boulder cracks at your frown.",
      "A boulder cracks at $n's frown."
  }
    },

    {
  {
      "Everyone's clothes are transparent, and you are laughing.",
      "Your clothes are transparent, and $n is laughing.",
      "An eye in a pyramid winks at you.",
      "An eye in a pyramid winks at $n.",
      "Everyone discovers your dagger a centimeter from their eye.",
      "You discover $n's dagger a centimeter from your eye.",
      "Mercenaries arrive to do your bidding.",
      "Mercenaries arrive to do $n's bidding."
  }
    },

    {
  {
      "A black hole swallows you.",
      "A black hole swallows $n.",
      "Valentine Michael Smith offers you a glass of water.",
      "Valentine Michael Smith offers $n a glass of water.",
      "Where did you go?",
      "Where did $n go?",
      "Four matched Percherons bring in your chariot.",
      "Four matched Percherons bring in $n's chariot."
  }
    },

    {
  {
      "The world shimmers in time with your whistling.",
      "The world shimmers in time with $n's whistling.",
      "The great god Mojo gives you a staff.",
      "The great god Mojo gives $n a staff.",
      "Click.",
      "Click.",
      "Atlas asks you to relieve him.",
      "Atlas asks $n to relieve him."
  }
    }
};



void do_pose( CHAR_DATA *ch, char *argument )
{
    int level;
    int pose;
    int class;

    if ( IS_NPC(ch) )
  return;

    level = UMIN( ch->level, sizeof(pose_table) / sizeof(pose_table[0]) - 1 );
    pose  = number_range(0, level);
    if( ch->class <= 3 )
      class = ch->class;
    else
      {
	switch(ch->class)
	{
	case 4: class = number_range(1,2);
		break;
	case 5: class = number_range(2,3);
		if( class == 2) class--;
		break;
	case 6: class = number_range(2,3);
		break;
	case 7: class = number_range(0,1);
		break;
	case 8: class = number_range(1,2);
		if(class == 1) class--;
		break;
	case 9: class = number_range(0,1);
		if(class == 1) class += 2;
		break;
	case 10: class = number_range(0,1);
		break;
	case 11:
	case 12:
	case 13:
	case 14: class = ch->class - 11;
		break;
	default: class = 0;
	}
      }

    act( pose_table[pose].message[2*class+0], ch, NULL, NULL, TO_CHAR, FALSE );
    act( pose_table[pose].message[2*class+1], ch, NULL, NULL, TO_ROOM, FALSE );

    return;
}


/*
void do_bug( CHAR_DATA *ch, char *argument )
{
    append_file( ch, BUG_FILE, argument );
    send_to_char( "Bug logged.\n\r", ch );
    return;
}
*/

void do_typo( CHAR_DATA *ch, char *argument )
{
    append_file( ch, TYPO_FILE, argument );
    send_to_char( "Typo logged.\n\r", ch );
    return;
}



void do_rent( CHAR_DATA *ch, char *argument )
{
    send_to_char( "There is no rent here.  Just save and quit.\n\r", ch );
    return;
}



void do_qui( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to QUIT, you have to spell it out.\n\r", ch );
    return;
}

void do_quit_command ( CHAR_DATA *ch, char *argument )
{
  AFFECT_DATA *paf,*paf_next;
  bool can_quit=TRUE;

    if(IS_SET(ch->mhs,MHS_SHAPESHIFTED) )
    {
      send_to_char( "Unshape Shift First Please.\r\n", ch);
      return;
    }

    if ( is_affected(ch, gsn_cone_of_silence ) )
    {
      send_to_char("No way.  You're under the affect of Cone Of Silence",ch);
      return;
    }

    if( is_affected(ch, gsn_garotte) )
    {
      send_to_char("{CNo Way{x! You're {Ygarotted{x!!!",ch);
      return;
    }

    if ( ch->position == POS_FIGHTING )
    {
  send_to_char( "No way! You are fighting.\n\r", ch );
  return;
    }

/* Ron Down - Check if Character has note in progress */
    if ( ch->pnote != NULL )
    {
       send_to_char( "You have a note in progress, please post or clear before quiting.\n\r", ch );
       return;
    }
/* Ron Up */

/*
    if (IS_SET(ch->mhs,MHS_SHAPESHIFTED) && (ch->race != ch->save_race))
    {
       send_to_char(" You must return from a shapeshift before quiting.\n\r",ch);
       return;
    }
    */

    if ( ch->position  < POS_STUNNED  )
    {
  send_to_char( "You're not DEAD yet.\n\r", ch );
  return;
    }
    
	for ( paf = ch->affected ; paf != NULL ; paf = paf_next )
        {
            paf_next = paf->next;

            if ( paf->where == DAMAGE_OVER_TIME )
		can_quit=FALSE;
	}

    if (ch->pcdata && ch->pcdata->quit_time > 0 && !IS_IMMORTAL (ch) 
	&& ((!ch->pcdata->clan_info && ch->in_room->clan != ch->clan) || ch->in_room->vnum >= 0 || !can_quit ) ) {
      send_to_char ("Things are getting interesting.. wait a few ticks.\n\r",ch);
      return;
    }
    if(ch->pcdata && ch->pcdata->corpse_timer)
    {
      char buf[256];
      sprintf(buf, "Please wait %d ticks for your corpse to return first.\n\r", ch->pcdata->corpse_timer);
      send_to_char(buf, ch);
      return;
    }
    do_quit (ch, "");
}

void do_quit( CHAR_DATA *ch, char *argument )
{
    DESCRIPTOR_DATA *d, *d_next, *d_glad;
    int loss;
    long id;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char host[MAX_STRING_LENGTH];
    buf[0] = '\0'; 

    one_argument( argument, arg );
    if ( IS_NPC(ch) )
  return;

  if(ch->pcdata->deity_trial_timer > 0)
  {
	sprintf(buf, "You abandon the trial given to you by %s\n\r", deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	ch->pcdata->deity_trial_timer = 0;
	log_deity_favor(ch, NULL, DEITY_TRIAL_FAIL_QUIT);
  }
  edit_stop(ch);
  end_long_edit(ch, NULL);/* Handles its own safety checks */
/*
    if (IS_SET(ch->mhs,MHS_SHAPESHIFTED) && (ch->race != ch->save_race))
       shapeshift_remove(ch);
*/
    /*if ( ch->in_room->vnum >= 0 &&
     (ch->in_room->vnum < 3001 || ch->in_room->vnum >3383)
     && (ch->in_room->vnum < 9500 || ch->in_room->vnum >9799)
        && ch->in_room->vnum != 2 && !IS_IMMORTAL(ch) 
  && !IS_SET(ch->in_room->room_flags,ROOM_SAFE)
  && !IS_SET(ch->in_room->room_flags,ROOM_PRIVATE)
    )
       {
        loss = 2 * ch->level;
        gain_exp(ch, 0 - loss);
        send_to_char( 
  "You lost experience for quitting outside of town.\n\r", ch );
  if(ch->level >= 40 && number_percent() >50 
	&& (!IS_AFFECTED(ch,AFF_CURSE) || !IS_SET(ch->mhs,MHS_CURSE)) &&
	!ch->in_room->area->no_transport ) 
    {
	sprintf(buf,"%s sends you to visit Hassan.\n\r",
		deity_table[ch->pcdata->deity].pname);
      send_to_char(buf,ch);
       char_from_room (ch);
       clear_mount(ch);
       char_to_room (ch,get_room_index( ROOM_VNUM_TEMPLE ));          }
  }*/
    send_to_char( 
	"Alas, all good things must come to an end.\n\r",ch);
    act( "$n has left the game.", ch, NULL, NULL, TO_ROOM, FALSE );
    sprintf( log_buf, "%s has quit.", ch->name );
    log_string( log_buf );
    if (ch->desc == NULL)
    strcpy(host,"linkdead");
  else
  strcpy(host,ch->desc->host);
  wiznet("$N rejoins the real world.",ch,NULL,WIZ_LOGINS,WIZ_SITES,get_trust(ch));
  if( (!IS_IMMORTAL(ch) || (ch->incog_level == 0 && ch->invis_level == 0))
	&& !str_cmp(arg, "") )
  {
    //pnet("$N leaves Boinga.",ch,NULL,PNET_LOGINS,NULL,get_trust(ch));
    pnet("$N leaves Boinga.",ch,NULL,PNET_LOGINS,NULL,IS_IMMORTAL(ch) ? get_trust(ch) : 1);
  }
         sprintf( buf, "%s@%s has quit.", ch->name, host );
        wiznet(buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));

/* Send Departure of a Highlander to all Highlanders(with certain kills) */
if (IS_SET(ch->mhs,MHS_HIGHLANDER))
{
   sprintf(buf, "The presence of a Highlander has left Boinga.\n\r");

   for ( d = descriptor_list; d != NULL; d = d->next )
   {
      CHAR_DATA *victm;

      victm = d->original ? d->original : d->character;

      if ( d->connected == CON_PLAYING &&
           d->character != ch &&
           IS_SET(victm->mhs,MHS_HIGHLANDER) &&
	   (victm->pcdata->highlander_data[ALL_KILLS] >= 6))
        send_to_char(buf, victm);
   }
   remove_highlander(ch,ch); 
}
  count_clanners();
	if (IS_SET(ch->mhs,MHS_GLADIATOR))
	{
           sprintf(buf, "%s got scared and quit the arena!", ch->name);
           gladiator_talk(buf); 
	   gladiator_left_arena(ch,TRUE);
	
           
	   for ( d_glad = descriptor_list; d_glad != NULL; d_glad = d_glad->next)
	   {
		if(d_glad->character != NULL && !IS_NPC(d_glad->character) &&  d_glad->character->pcdata->glad_bet_on == ch)
		{
	          sprintf(buf, "The bookie refunds your bet of %d on %s.", 
			  d_glad->character->pcdata->glad_bet_amt, ch->name);
		  send_to_char(buf, d_glad->character);
		  d_glad->character->pcdata->glad_bet_on = d_glad->character;
		  d_glad->character->pcdata->glad_bet_amt = 0;
		}
           }
	   REMOVE_BIT(ch->mhs,MHS_GLADIATOR);

	}

    /*
     * After extract_char the ch is no longer valid!
     */
    /* decrement logins allowed from pfresh */
    if ( !IS_NPC(ch))
    {
       if (ch->pcdata->logout_tracker > 0)
          ch->pcdata->logout_tracker -= 1;
       if (!IS_IMMORTAL(ch) && is_clan(ch))
       {
          if(ch->pcdata->combats_since_last_login == 0)
            ch->pcdata->logins_without_combat++;
          if(ch->pcdata->died_today == FALSE)
            ch->pcdata->logins_without_death++;
          if(ch->pcdata->killed_today == FALSE)
            ch->pcdata->logins_without_kill++;
       }
    }
    save_char_obj( ch );
    id = ch->id;
    d = ch->desc;
    extract_char( ch, TRUE );
    if ( d != NULL )
        close_socket( d );

    for (d = descriptor_list; d != NULL; d = d_next)
    {
        CHAR_DATA *tch;

        d_next = d->next;
        tch = d->original ? d->original : d->character;
        if (tch && tch->id == id )
        {
            extract_char(tch,TRUE);
            close_socket(d);
        }
    }

    return;
}

void do_save( CHAR_DATA *ch, char *argument )
{
    if ( IS_NPC(ch) )
  return;
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    save_char_obj( ch );
    send_to_char("Saving. Remember that MHS has automatic saving now\n\r", ch);
  //  if (!IS_IMMORTAL(ch))
  //    WAIT_STATE(ch,3 * PULSE_VIOLENCE);
    return;
}



void do_follow( CHAR_DATA *ch, char *argument )
{
/* RT changed to allow unlimited following and follow the NOFOLLOW rules */
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  send_to_char( "Follow whom?\n\r", ch );
  return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL )
    {
  act( "But you'd rather follow $N!", ch, NULL, ch->master, TO_CHAR, FALSE );
  return;
    }

    if ( victim == ch )
    {
  if ( ch->master == NULL )
  {
      send_to_char( "You already follow yourself.\n\r", ch );
      return;
  }
  stop_follower(ch);
  return;
    }

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOFOLLOW) && !IS_IMMORTAL(ch))
    {
  act("$N doesn't seem to want any followers.\n\r",
       ch,NULL,victim, TO_CHAR, FALSE);
  return;
    }

    REMOVE_BIT(ch->act,PLR_NOFOLLOW);
    
    if ( ch->master != NULL )
  stop_follower( ch );

    add_follower( ch, victim );
    return;
}

void add_follower( CHAR_DATA *ch, CHAR_DATA *master )
{
    CHAR_DATA *gch;

    if ( ch->master != NULL )
    {
  bug( "Add_follower: non-null master.", 0 );
  return;
    }

    /* There is late-minute checking in fight.c, as well, but this is
     * here to catch most of these right away so the spell is working
     * properly
     */
    if ( is_affected(ch,gsn_wound_transfer) )
    {

    if ( ( is_clan(ch) && !is_clan(master) ) ||
	 (!is_clan(ch) &&  is_clan(master) ) )
    {
	send_to_char("The boundries of magic forbid it.\n\r",ch);
	return;
    }

    if ( is_affected(master,gsn_wound_transfer) )
    {
       send_to_char("The boundries of magic forbid it.\n\r",ch);
       return;
    }

    /* Also make sure that njo one in master's GROUP has it */
    for ( gch = char_list ; gch != NULL ; gch = gch->next )
    {
       if ( is_same_group(gch,master) &&
	    is_affected(gch,gsn_wound_transfer) )
       {
       send_to_char("The boundries of magic forbid it.\n\r",ch);
       return;
       }

    }

    }



    ch->master        = master;
   // ch->leader        = NULL;
    remove_from_group(ch);

    if ( can_see( master, ch, FALSE ) )
  act( "$n now follows you.", ch, NULL, master, TO_VICT, FALSE );

    act( "You now follow $N.",  ch, NULL, master, TO_CHAR, FALSE );

    return;
}



void stop_follower( CHAR_DATA *ch )
{
    if ( ch->master == NULL )
    {
  bug( "Stop_follower: null master.", 0 );
  return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) )
    {
  REMOVE_BIT( ch->affected_by, AFF_CHARM );
  affect_strip( ch, gsn_charm_person );
    }

    if ( can_see( ch->master, ch, FALSE ) && ch->in_room != NULL)
    {
  act( "$n stops following you.",     ch, NULL, ch->master, TO_VICT, FALSE);
  act( "You stop following $N.",      ch, NULL, ch->master, TO_CHAR, FALSE);
    }
    if (ch->master->pet == ch)
  ch->master->pet = NULL;

    ch->master = NULL;
    //ch->leader = NULL;
    remove_from_group(ch);
    return;
}

/* nukes charmed monsters and pets */
void nuke_pets( CHAR_DATA *ch )
{    
    CHAR_DATA *pet;

    if ((pet = ch->pet) != NULL)
    {
  stop_follower(pet);
  if (pet->in_room != NULL)
      act("$N slowly fades away.",ch,NULL,pet,TO_NOTVICT,FALSE);
  extract_char(pet,TRUE);
    }
    ch->pet = NULL;

    return;
}

void die_ignore( CHAR_DATA *ch )
{
    CHAR_DATA *fch;

    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
  if ( fch->ignoring == ch )
	fch->ignoring = NULL;
    }

    return;
}


void die_follower( CHAR_DATA *ch )
{
    CHAR_DATA *fch;

    if ( ch->master != NULL )
    {
  if (ch->master->pet == ch)
      ch->master->pet = NULL;
  stop_follower( ch );
    }

    //ch->leader = NULL;
    remove_from_group(ch);

    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
  if ( fch->master == ch )
      stop_follower( fch );
  if ( fch->leader == ch )
      fch->leader = fch;
    }

    return;
}



void do_order( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *och;
    CHAR_DATA *och_next;
    bool found;
    bool fAll;
    int cmd;

    argument = one_argument( argument, arg );
    one_argument(argument,arg2);

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
       send_to_char("No ordering while in wraithform.\n\r",ch);
	return;
    }

   
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
	if ( arg2[0] == cmd_table[cmd].name[0]
	    &&   !str_prefix( arg2, cmd_table[cmd].name ))
	{
	  if (cmd_table[cmd].order == 0 )
	  {
	  send_to_char("That will not be done.\n\r",ch);
	  return;
	  }
	  else
	  break;
	}
	
    }

    if ( arg[0] == '\0' || argument[0] == '\0' )
    {
  send_to_char( "Order whom to do what?\n\r", ch );
  return;
    }

    if ( IS_AFFECTED( ch, AFF_CHARM ) )
    {
  send_to_char( "You feel like taking, not giving, orders.\n\r", ch );
  return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
  fAll   = TRUE;
  victim = NULL;
    }
    else
    {
  fAll   = FALSE;
  if ( ( victim = get_char_room( ch, arg ) ) == NULL )
  {
      send_to_char( "They aren't here.\n\r", ch );
      return;
  }

  if ( victim == ch )
  {
      send_to_char( "Aye aye, right away!\n\r", ch );
      return;
  }

  if (!IS_AFFECTED(victim, AFF_CHARM) || victim->master != ch 
  ||  (IS_IMMORTAL(victim) && victim->trust >= ch->trust))
  {
      send_to_char( "Do it yourself!\n\r", ch );
      return;
  }
    }


    found = FALSE;
    for ( och = ch->in_room->people; och != NULL; och = och_next )
    {
  och_next = och->next_in_room;

  if ( IS_AFFECTED(och, AFF_CHARM)
  &&   och->master == ch
  && ( fAll || och == victim ) )
  {
    if(IS_NPC(och) || cmd_table[cmd].order != 2)
    {
      found = TRUE;
      sprintf( buf, "$n orders you to '%s'.", argument );
      act( buf, ch, NULL, och, TO_VICT,FALSE );
      strcpy(kludge_string,"order");
      interpret( och, argument );
      strcpy(kludge_string,"");
    }
    else
      send_to_char("You can't order another player to do that.\n\r", ch);
  }
    }

    if ( found )
    {
  WAIT_STATE(ch,PULSE_VIOLENCE);
  send_to_char( "Ok.\n\r", ch );
    }
    else
  send_to_char( "You have no followers here.\n\r", ch );
    return;
}


/* Does exactly what it says, can't remove a leader */
void remove_from_group(CHAR_DATA *ch)
{
	if(ch->leader == NULL)
		return; // No leader to break off of
	if(ch->leader->follower == ch)
		ch->leader->follower = ch->next_groupmate;
	else
	{
		CHAR_DATA *prev = ch->leader->follower;
		while(prev != NULL && prev->next_groupmate != ch)
			prev = prev->next_groupmate;
		if(prev != NULL)
		{// Found the follower in this group chain
			prev->next_groupmate = ch->next_groupmate;
		}
		else
			bug("Failed to find follower in leader's group list.", 0);
	}
	ch->next_groupmate = NULL;
	ch->leader = NULL;
}

void add_to_group(CHAR_DATA *victim, CHAR_DATA *leader)
{
	CHAR_DATA *fol;
	victim->leader = leader;
	
	fol = leader->follower;
        if(fol != NULL)
        {
	  while(fol->next_groupmate && fol != victim)
		fol = fol->next_groupmate;/* Leaves fol at the end of the list, useful later */
        }
	/* DO NOT MOVE fol NOW UNLESS YOU FIX THE NPC HANDLING FURTHER IN THE FUNCTION */
	if(fol == victim)
	{/* Not a huge problem, but they're already in the list and shouldn't be */
		bug("Multiple follow attempted.", 0);
	}
	else
	{
		/* Basic sorting here, let's get this readable instead of the mess it is now */
		if(!IS_NPC(victim) && fol)
		{/* Players go before any NPCs 
			* Could sometimes skip looping by checking fol's NPC status but the code would get messier */
	  	  if(IS_NPC(leader->follower))
                    fol = NULL;// Needs to go in at the start of the list
                  else
                  {
		    fol = leader->follower;// We know they have a follower this time
		    while(fol->next_groupmate && !IS_NPC(fol->next_groupmate))
		      fol = fol->next_groupmate;
                  }
		}
		/* NPC HANDLING, NPCs go at the end of the list - where fol currently is. No loop needed. */
                if(fol == NULL)
                {
                   victim->next_groupmate = leader->follower;
		   leader->follower = victim;
                }
                else
                {
		   victim->next_groupmate = fol->next_groupmate;
		   fol->next_groupmate = victim;
                }
	}
}

void do_gprompt( CHAR_DATA *ch, char *argument )
{
	if(IS_NPC(ch))
		return;
  if (IS_SET(ch->pcdata->new_opt_flags,OPT_NOGPROMPT))
  {
    send_to_char("Group hp prompt will now show.\n\r",ch);
    REMOVE_BIT(ch->pcdata->new_opt_flags,OPT_NOGPROMPT);
  }
  else
  {
    send_to_char("Group hp prompt will no longer show.\n\r",ch);
    SET_BIT(ch->pcdata->new_opt_flags,OPT_NOGPROMPT);
  }
}

void display_group_member(CHAR_DATA *ch, CHAR_DATA *gch)
{
    char buf[256], buf2[256];
    sprintf( buf,
    "[%2d %s] %-16s ",
        gch->level,
        IS_NPC(gch) ? "Mob" : class_table[gch->class].who_name,
        capitalize( PERS(gch,ch,TRUE) )); 
	if ( is_affected(gch,gsn_rage) )
              sprintf(buf2, "H: {c????{x M: %d/%d V: %d/%d %6d xp\n\r",gch->mana, gch->max_mana, gch->move, gch->max_move, gch->exp);
//		strcat(buf,"{c???? {xM:");
	    else
              sprintf(buf2, "H: %d/%d M: %d/%d V: %d/%d %6d xp\n\r",gch->hit, gch->max_hit, gch->mana, gch->max_mana, gch->move, gch->max_move, gch->exp);
/*	if ( gch->hit >= UMAX((9*gch->max_hit)/10,1) )
		strcat(buf,"{W**** {xM:");
	    else
	    if ( gch->hit * 100 / UMAX(gch->max_hit,1) > 75 )
		strcat(buf,"{W***- {xM:");
	    else
	    if ( gch->hit * 100 / UMAX(gch->max_hit,1) > 50 )
		strcat(buf,"{Y**-- {xM:");
	    else
	    if ( gch->hit * 100 / UMAX(gch->max_hit,1) > 25 )
		strcat(buf,"{R*--- {xM:");
	    else
		strcat(buf,"{r---- {xM:");

	    if ( gch->mana >= UMAX((9*gch->max_mana)/10,1) )
		strcat(buf,"{W**** {xV:");
	    else
	    if ( gch->mana * 100 / UMAX(gch->max_mana,1) > 75 )
		strcat(buf,"{W***- {xV:");
	    else
	    if ( gch->mana * 100 / UMAX(gch->max_mana,1) > 50 )
		strcat(buf,"{Y**-- {xV:");
	    else
	    if ( gch->mana * 100 / UMAX(gch->max_mana,1) > 25 )
		strcat(buf,"{R*--- {xV:");
	    else
		strcat(buf,"{r---- {xV:");

	    if ( gch->move >= UMAX((9*gch->max_move)/10,1) )
		strcat(buf,"{W****{x");
	    else
	    if ( gch->move * 100 / UMAX(gch->max_move,1) > 75 )
		strcat(buf,"{W***-{x");
	    else
	    if ( gch->move * 100 / UMAX(gch->max_move,1) > 50 )
		strcat(buf,"{Y**--{x");
	    else
	    if ( gch->move * 100 / UMAX(gch->max_move,1) > 25 )
		strcat(buf,"{R*---{x");
	    else
		strcat(buf,"{r----{x");*/
//	    sprintf( buf2, "%6d xp\n\r", gch->exp    );
	    strcat(buf,buf2);
    send_to_char( buf, ch );
}

void do_group( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  CHAR_DATA *gch;
  CHAR_DATA *leader;

  leader = (ch->leader != NULL) ? ch->leader : ch;
  sprintf( buf, "%s's group:\n\r", PERS(leader, ch, TRUE) );
  send_to_char( buf, ch );

  display_group_member(ch, leader);
//  for ( gch = char_list; gch != NULL; gch = gch->next )
  for ( gch = leader->follower; gch != NULL; gch = gch->next_groupmate )
  {
//      if ( is_same_group( gch, ch ) )
     display_group_member(ch, gch);
  }
  return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if(victim == ch)
    {
      send_to_char("You're already part of your own group.\n\r", ch);
      return;
    }

    if(IS_SET(victim->act,PLR_DWEEB) || IS_SET(ch->act,PLR_DWEEB))
    {
      send_to_char("No grouping with DWEEBs.\n\r",ch);
      return;
    }

/*    if(ch->clan == clan_lookup("demise") && is_clan(victim) && !is_same_clan(ch,victim)  )
    {
      send_to_char("You have been {YWARNED{x about grouping with others outside your clan!\r\n",ch);
      send_to_char("You are now ranked a 0.  Pray an Imm or a leader does not see you.\r\n",ch);
      set_title(ch,"Has a simple Clan Rule and broke it.");
      SET_BIT(ch->comm,COMM_NOTITLE);
      ch->pcdata->rank = 0;
      return;
    }
    if(is_clan(ch) && victim->clan == clan_lookup("demise") && !is_same_clan(ch,victim) )
    {
      send_to_char("The hand of {RCthon{x stops you from grouping with them.\r\n",ch);
      return;
    }*/

    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians may not be grouped.\n\r", ch);
      return;
    }

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char("Sorry Gladiators do not group.\n\r",ch);
       return;
    }

    if ( ch->master != NULL || ( ch->leader != NULL && ch->leader != ch ) )
    {
  send_to_char( "But you are following someone else!\n\r", ch );
  return;
    }

    if ( victim->master != ch && ch != victim )
    {
  act( "$N isn't following you.", ch, NULL, victim, TO_CHAR,FALSE );
  return;
    }
    
    if (IS_AFFECTED(victim,AFF_CHARM))
    {
  send_to_char("You can't remove charmed mobs from your group.\n\r",ch);
  return;
    }
    
    if (IS_AFFECTED(ch,AFF_CHARM) && !IS_NPC(victim))
    {
 act("You like your master too much to leave $m!",ch,NULL,victim,TO_VICT,FALSE);
  return;
    }

    if (IS_SET(ch->mhs,MHS_HIGHLANDER) || IS_SET(victim->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Highlanders can not group. Honorable combat is one on one.\n\r",ch);
       return;
    }

    if (victim->level - ch->level > 8)
    {
  send_to_char("They are to high of a level for your group.\n\r",ch);
  return;
    }

    if (victim->level - ch->level < -8)
    {
  send_to_char("They are to low of a level for your group.\n\r",ch);
  return;
    }

    if ( is_same_group( victim, ch ) && ch != victim )
    {
  remove_from_group(victim);
//  victim->leader = NULL;
  act( "$n removes $N from $s group.",   ch, NULL, victim, TO_NOTVICT,TRUE);
  if (!IS_NPC(victim))
  act( "$n removes you from $s group.",  ch, NULL, victim, TO_VICT,TRUE);
  act( "You remove $N from your group.", ch, NULL, victim, TO_CHAR,TRUE);
  return;
    }

    if(ch->position < POS_RESTING)
	{
	  send_to_char("Too late for that...",ch);
	  return;
	}
//    victim->leader = ch;
    add_to_group(victim, ch);
    act( "$N joins $n's group.", ch, NULL, victim, TO_NOTVICT,FALSE);
    if(!IS_NPC(victim))
    act( "You join $n's group.", ch, NULL, victim, TO_VICT,FALSE);
    act( "$N joins your group.", ch, NULL, victim, TO_CHAR,FALSE);
    return;
}



/*
 * 'Split' originally by Gnort, God of Chaos.
 */
void do_split( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *gch;
    int members;
    int amount_gold = 0, amount_silver = 0;
    int share_gold, share_silver;
    int extra_gold, extra_silver;

    argument = one_argument( argument, arg1 );
         one_argument( argument, arg2 );

    if ( arg1[0] == '\0' )
    {
  send_to_char( "Split how much?\n\r", ch );
  return;
    }
    
    amount_silver = atoi( arg1 );

    if (arg2[0] != '\0')
  amount_gold = atoi(arg2);

    if ( amount_gold < 0 || amount_silver < 0)
    {
  send_to_char( "Your group wouldn't like that.\n\r", ch );
  return;
    }

    if ( amount_gold == 0 && amount_silver == 0 )
    {
  send_to_char( "You hand out zero coins, but no one notices.\n\r", ch );
  return;
    }

    if ( ch->gold <  amount_gold || ch->silver < amount_silver)
    {
  send_to_char( "You don't have that much to split.\n\r", ch );
  return;
    }
  
    members = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  if ( is_same_group( gch, ch ) && !IS_AFFECTED(gch,AFF_CHARM))
      members++;
    }

    if ( members < 2 )
    {
  send_to_char( "Just keep it all.\n\r", ch );
  return;
    }
      
    share_silver = amount_silver / members;
    extra_silver = amount_silver % members;

    share_gold   = amount_gold / members;
    extra_gold   = amount_gold % members;

    if ( share_gold == 0 && share_silver == 0 )
    {
  send_to_char( "Don't even bother, cheapskate.\n\r", ch );
  return;
    }

    ch->silver  -= amount_silver;
    ch->silver  += share_silver + extra_silver;
    ch->gold    -= amount_gold;
    ch->gold    += share_gold + extra_gold;

    if (share_silver > 0)
    {
  sprintf(buf,
      "You split %d silver coins. Your share is %d silver.\n\r",
      amount_silver,share_silver + extra_silver);
  send_to_char(buf,ch);
    }

    if (share_gold > 0)
    {
  sprintf(buf,
      "You split %d gold coins. Your share is %d gold.\n\r",
       amount_gold,share_gold + extra_gold);
  send_to_char(buf,ch);
    }

    if (share_gold == 0)
    {
  sprintf(buf,"$n splits %d silver coins. Your share is %d silver.",
    amount_silver,share_silver);
    }
    else if (share_silver == 0)
    {
  sprintf(buf,"$n splits %d gold coins. Your share is %d gold.",
    amount_gold,share_gold);
    }
    else
    {
  sprintf(buf,
"$n splits %d silver and %d gold coins, giving you %d silver and %d gold.\n\r",
   amount_silver,amount_gold,share_silver,share_gold);
    }

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  if ( gch != ch && is_same_group(gch,ch) && !IS_AFFECTED(gch,AFF_CHARM))
  {
   if ( (get_carry_weight(gch) + share_silver/10 + share_gold*2/5)
        > can_carry_w(gch) )
    {
      act( "$N can't carry that much weight.", ch, NULL, gch, TO_CHAR,FALSE);
      ch->gold += share_gold;
      ch->silver += share_silver;
    }
   else
    {
      act( buf, ch, NULL, gch, TO_VICT, FALSE );
      gch->gold += share_gold;
      gch->silver += share_silver;
    }
  }
    }

    return;
}



void do_gtell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH],bufc[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    if ( is_affected(ch, gsn_cone_of_silence ) )
    return;


    if ( argument[0] == '\0' )
    {
  send_to_char( "Tell your group what?\n\r", ch );
  return;
    }

    if ( IS_SET( ch->comm, COMM_NOTELL ) )
    {
  send_to_char( "Your message didn't get through!\n\r", ch );
  return;
    }

    /*
     * Note use of send_to_char, so gtell works on sleepers.
     */
    sprintf( buf, "%s tells the group '%s'\n\r", ch->name, argument );
    sprintf( bufc, "%s%s%s tells the group%s '%s'\n\r",BOLD,ch->name,BLUE,NORMAL, argument );
    for ( gch = char_list; gch != NULL; gch = gch->next )
    {
  if ( is_same_group( gch, ch ) )
	{
        send_timestamp(gch, TRUE, FALSE);
	if(IS_SET(gch->display,DISP_COLOR))
      send_to_char( bufc, gch );
	else
      send_to_char( buf, gch );
	}
    }

    return;
}



/*
 * It is very important that this be an equivalence relation:
 * (1) A ~ A
 * (2) if A ~ B then B ~ A
 * (3) if A ~ B  and B ~ C, then A ~ C
 */
bool is_same_group( CHAR_DATA *ach, CHAR_DATA *bch )
{
    if ( ach == NULL || bch == NULL)
  return FALSE;

    if (!IS_NPC(ach) && !IS_NPC(bch))
    {
       if (IS_SET(ach->mhs,MHS_GLADIATOR) && IS_SET(bch->mhs,MHS_GLADIATOR)
	   && (gladiator_info.type == 2 || gladiator_info.type == 3)
           && gladiator_info.bet_counter == 0)
       {
          if(ach->pcdata->gladiator_team == bch->pcdata->gladiator_team)
             return TRUE;
       }
    }

    if ( ach->leader != NULL ) ach = ach->leader;
    if ( bch->leader != NULL ) bch = bch->leader;
    return ach == bch;
}


bool has_group_mates( CHAR_DATA *ch )
{

//    DESCRIPTOR_DATA *d;
//    CHAR_DATA *vch;

    if ( IS_NPC(ch) || ch == NULL)
	return FALSE;
    if(ch->leader || ch->follower)
      return TRUE;
    return FALSE;

/*    for ( d = descriptor_list ; d != NULL ; d= d->next )
    {
       if ( ( vch = d->character ) == NULL )
		continue;

	if ( vch == ch )
		continue;*/
/* this is where i check if someone has the current char as the leader 	
	or if the vch is the leader for the current char */
/*	if ( ch == vch->leader || vch == ch->leader )
	   return TRUE;

    }

    return FALSE;*/


}

bool group_has_crusader( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;

    if ( IS_NPC(ch) )
	return FALSE;

    for ( d = descriptor_list ; d != NULL ; d= d->next )
    {
       if ( ( vch = d->character ) == NULL )
		continue;

	if ( is_same_group( ch, vch ) && 
		vch->class == class_lookup("crusader") &&
                vch->in_room == ch->in_room  &&
		has_group_mates(ch) 
		)
	  return TRUE;
    }

    return FALSE;
}

bool group_has_cavalier( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;

    if ( IS_NPC(ch) )
        return FALSE;

    for ( d = descriptor_list ; d != NULL ; d= d->next )
    {
       if ( ( vch = d->character ) == NULL )
                continue;

        if ( is_same_group( ch, vch ) &&
                vch->kit == kit_lookup("cavalier") &&
		(is_mounted(vch) && IS_NPC(vch->riding) && vch->riding->pIndexData->vnum == MOB_VNUM_WARHORSE) &&
                vch->in_room == ch->in_room  &&
                has_group_mates(ch)
                )
          return TRUE;
    }

    return FALSE;
}


int group_has_how_many_crusader( CHAR_DATA *ch )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *vch;
    int number_of =0;

    if ( IS_NPC(ch) )
	return 0;

    for ( d = descriptor_list ; d != NULL ; d= d->next )
    {
       if ( ( vch = d->character ) == NULL )
		continue;

	if ( is_same_group( ch, vch ) && 
		vch->class == class_lookup("crusader") &&
                vch->in_room == ch->in_room /* &&
		has_group_mates(ch) */ //No longer needs group mates
		)
		number_of++;
    }

    return number_of;
}


void do_nogladiator(CHAR_DATA *ch, char *argument)
{
    if (IS_SET(ch->comm, COMM_NOGLADIATOR))
    {
       REMOVE_BIT(ch->comm, COMM_NOGLADIATOR);
       send_to_char("Gladiator channel ON.\n\r", ch);
    }
    else
    {
       SET_BIT(ch->comm, COMM_NOGLADIATOR);
       send_to_char("Gladiator channel OFF.\n\r", ch);
    }
}

void gladiator_talk_ooc(char *txt)
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "{W[Glad Update]{x %s\n\r", txt);
   for (d = descriptor_list; d != NULL; d = d->next)
   {
      CHAR_DATA *victim;

      victim = d->original ? d->original : d->character;

      if (d->connected == CON_PLAYING &&
          (!IS_SET(victim->comm,COMM_NOGLADIATOR) || (!IS_NPC(victim) && victim->pcdata->glad_bet_on) || IS_SET(victim->mhs, MHS_GLADIATOR)) &&
          !IS_SET(victim->comm,COMM_QUIET))
      {
         send_to_char(buf, victim);
      }
   }
   log_string(buf);
}

void gladiator_talk(char *txt)
{
   DESCRIPTOR_DATA *d;
   char buf[MAX_STRING_LENGTH];

   sprintf(buf, "{W[Gladiator]{x %s\n\r", txt);
   for (d = descriptor_list; d != NULL; d = d->next)
   {
      CHAR_DATA *victim;

      victim = d->original ? d->original : d->character;

      if (d->connected == CON_PLAYING &&
          !IS_SET(victim->comm,COMM_NOGLADIATOR) &&
          !IS_SET(victim->comm,COMM_QUIET)
          && (!IS_SET(victim->mhs, MHS_GLADIATOR) || gladiator_info.blind == FALSE))
      {
         send_to_char(buf, victim);
      }
   }
   log_string(buf);
}

void do_die( CHAR_DATA *ch, char *argument )
{
    if(ch->position != POS_INCAP && ch->position != POS_MORTAL) 
    {
       send_to_char("As much as you'd like to die and we'd like to kill you, sorry you can't do that.",ch);
       return;
    }
    send_to_char("You head for the bright light!\n\r",ch);
    if ( ch->exp > exp_per_level(ch,ch->pcdata->points) * ch->level ) 
       gain_exp(ch,(exp_per_level(ch,ch->pcdata->points)*ch->level - ch->exp)*2/3); 

    raw_kill(ch,ch);
    return;
}


void do_sanction ( CHAR_DATA *ch, char *argument )
{
  CHAR_DATA *victim;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];

send_to_char("Sorry, sanctioning is temporarily disabled.\n\rPlease notify the imms if you need help.\n\r", ch);
return;

  argument = one_argument(argument, arg1);
  one_argument(argument, arg2);

  if( ch->pcdata->rank < MAX_RANK  && !IS_SET(ch->pcdata->clan_flags, CLAN_ALLOW_SANC) )
  {
  send_to_char("You cannot enforce sanctions.\n\r", ch);
  return;
  }

  if ( arg1[0] == '\0' || arg2[0] == '\0' )
  {
  send_to_char("Syntax: sanction <char> <option>\n\r", ch);
  send_to_char("Options are: allow show hall portal regen healer channel skill \n\r", ch);
  return;
  }

  if ( (victim = get_char_online(ch, arg1)) == NULL )
  {
  send_to_char("They aren't here.\n\r", ch);
  return;
  }

  if (IS_NPC(victim))
  {
  send_to_char("Don't be a fucknut.\n\r", ch);
  return;
  }

  if ( ch == victim )
  {
  send_to_char("You may not sanction yourself.\n\r", ch);
  return;
  }

  if( ch->clan != victim->clan )
  {
  send_to_char("You may only enforce sanctions on those in your own clan.\n\r", ch);
  return;
  }

  if ( victim->pcdata->rank == MAX_RANK )
  {
  send_to_char("You may not enforce sanctions on a clan Leader.\n\r", ch);
  return;
  }

  switch(UPPER(arg2[0]))
  {
  case 'A':
    if (victim->pcdata->rank < MAX_RANK -1 )
     {
     send_to_char("You may only grant sanctioning priveledges to /4's.\n\r", ch);
     return;
     }

     if ( !IS_SET(victim->pcdata->clan_flags, CLAN_ALLOW_SANC))
       {
      send_to_char("You now have sanctioning priveledges.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_ALLOW_SANC);
	sprintf(buf,"$N enables %s's sanctioning ability." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
       }
     else
      {
      send_to_char("Your sanctioning priveledges have been revoked.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_ALLOW_SANC);
	sprintf(buf,"$N revokes %s's sanctioning ability." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
      }

      break;


  case 'C':
    if(!IS_SET(victim->pcdata->clan_flags, CLAN_NO_CHANNEL))
      {
      send_to_char("You have been forbidden to use the clan channel.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_NO_CHANNEL);
	sprintf(buf,"$N sanctions %s's with no clan channel." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
      }
      else
      {
      send_to_char("You are once more permitted to use the clan channel.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_CHANNEL);
	sprintf(buf,"$N removes %s's sanctions of no clan channel." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
      }
    break;

  case 'H':
    if( !str_prefix(arg2, "hall"))
    {
    if (!IS_SET(victim->pcdata->clan_flags, CLAN_NO_HALL))
    {
     send_to_char("You have been forbidden entrance to your clan hall.\n\r", victim);
     SET_BIT(victim->pcdata->clan_flags, CLAN_NO_HALL);
	sprintf(buf,"$N sanctions %s's with no clan hall." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
     if ( victim->in_room->clan && victim->in_room->clan == victim->clan)
     {
     send_to_char("You have been forcefully ejected from your clan hall.\n\r", victim);
     char_from_room(victim);
     char_to_room(victim,get_room_index( ROOM_VNUM_TEMPLE));
     do_look(victim, "auto");
     }

    }
    else
    {
     send_to_char("You are once more permitted to enter the clan hall.\n\r", victim);
     REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_HALL);
	sprintf(buf,"$N removes %s's sanctions of no clan hall." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    }

    if ( !str_prefix(arg2, "healer"))
    {
    if (!IS_SET(victim->pcdata->clan_flags, CLAN_NO_HEALER))
    {
      send_to_char("The clan healer will no longer provide services for you.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_NO_HEALER);
	sprintf(buf,"$N sanctions %s's with no clan healer." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    { 
      send_to_char("The clan healer will once more provide his services to you.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_HEALER);
	sprintf(buf,"$N removes %s's sanctions of no clan healer." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    }

    break;

  case 'P':
    if(!IS_SET(victim->pcdata->clan_flags, CLAN_NO_PORTALS ))
    {
      send_to_char("You have been forbidden to use the clan portals.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_NO_PORTALS);
	sprintf(buf,"$N sanctions %s's with no clan portals." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
      send_to_char("You are once more permitted to use the clan portals.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_PORTALS);
	sprintf(buf,"$N removes %s's sanctions of no clan portals." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    break;
    
  case 'R':
    if (!IS_SET(victim->pcdata->clan_flags, CLAN_NO_REGEN))
    {
      send_to_char("You will receive no benefit from the clan regen room.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_NO_REGEN);
	sprintf(buf,"$N sanctions %s's with no clan regen." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    else
    {
      send_to_char("You will once again receive the benefits of the clan regen room.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_REGEN);
	sprintf(buf,"$N removes %s's sanctions of no clan regen." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
    }
    break;

  case 'S':
    if (!str_prefix(arg2, "store"))
    {
     if (!IS_SET(victim->pcdata->clan_flags, CLAN_NO_STORE))
     {
      send_to_char("You may no longer purchase items at the clan store.\n\r", victim);
      SET_BIT(victim->pcdata->clan_flags, CLAN_NO_STORE);
	sprintf(buf,"$N sanctions %s's with no clan store." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
     }
     else
     {
      send_to_char("You may once more purchase items at the clan store.\n\r", victim);
      REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_STORE);
	sprintf(buf,"$N removes %s's sanctions of no clan store." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
     }
    }

    if (!str_prefix(arg2, "skill"))
    {
     if(!IS_SET(victim->pcdata->clan_flags, CLAN_NO_SKILL_1))
     {
       send_to_char("You have been forbidden to use your clan skills.\n\r", victim);
       SET_BIT(victim->pcdata->clan_flags, CLAN_NO_SKILL_1);
	sprintf(buf,"$N sanctions %s's with no clan skill." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
     }
     else
     {
       send_to_char("You are once more permitted to use your clan skills.\n\r", victim);
       REMOVE_BIT(victim->pcdata->clan_flags, CLAN_NO_SKILL_1);
	sprintf(buf,"$N removes %s's sanctions of no clan skill." ,victim->name);
	wiznet(buf,ch,NULL,WIZ_PENALTIES,WIZ_SECURE,0);
     }
    }
    if (!str_prefix(arg2, "show"))
    {
      sprintf(buf, "%s has the following sanctions:\n\r%s\n\r", victim->name, clan_bit_name(victim->pcdata->clan_flags));
      send_to_char(buf, ch);
    }
    break;

  default:
    send_to_char("That is not a valid sanction.\n\r", ch);
    return;
    break;

   }

   send_to_char("Ok.\n\r", ch);
   return;
}





void do_bounty( CHAR_DATA *ch, char *argument )
{ 
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    long amt;

    argument = one_argument( argument, arg );
    argument = one_argument( argument, arg2 );

    if ( arg[0] == '\0' )
    {
	send_to_char("Usage: bounty <character> <amount in gold>\n\r",ch);
	return;
    }

    if ( !str_cmp(arg,"list") || !str_cmp(arg,"show") )
    {
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];

 	sprintf(buf,"{W%-28s Amount{x\n\r\n\r","Character");
	send_to_char(buf,ch);

	for ( d = descriptor_list ; d != NULL ; d = d->next )
	{
	    if ( d->connected != CON_PLAYING || d->character == NULL )
		continue;

	    if ( d->character->pcdata == NULL )
		continue;

	    if ( d->character->pcdata->bounty <= 0 )
 		continue;

	    sprintf(buf,"%-28s %ld\n\r",d->character->name,d->character->pcdata->bounty);
	    send_to_char(buf,ch);
	}
	return;
    }
    victim = get_char_online(ch,arg);

    if (victim == NULL)
    {
        send_to_char("Nobody by that name to bounty.\n\r",ch);
        return;
    }

    if (ch == victim)
    {
     send_to_char("If you're that eager to die, go to Altirin unsanced.\n\r",ch);
        return;
    }

    if ( IS_IMMORTAL(victim) )
    {
        send_to_char("Don't be a dumb bunny.  Setting a bounty on an Imm, you have a death-wish.\r\n",ch);
	return;
    }
/*    if ( ch->clan == clan_lookup("hunter") )
    {
	send_to_char("You cannot set a bounty as a bounty hunter yourself!\n\r",ch);
	return;
    }*/
    if ( IS_NPC(victim) )
    {
       send_to_char("Setting a bounty on a MOB? Don't be a dumb bunny.\r\n",ch);
       return;
    }

    if ( !is_clan(victim) || !is_clan(ch) || 
			(IS_SET(victim->mhs, MHS_GLADIATOR) && victim->pcdata->save_clan == 0) || 
				(IS_SET(ch->mhs, MHS_GLADIATOR) && ch->pcdata->save_clan == 0))
    {
	send_to_char("Impossible.\n\r",ch);
	return;
    }

    if ( arg2 == NULL || *arg2 == '\0' || !is_number(arg2) )
    {
        send_to_char("Usage: bounty <character> <amount in gold>\n\r",ch); 	
 	return;
    }

    if ( (amt = atoi(arg2)) < 200 || amt > ch->gold )
    {	
	send_to_char("Bounties must be at least 200 gold and no more than what you have.\n\r",ch);
	return;
    }

    victim->pcdata->bounty += amt;
    ch->gold -= amt;

    act("You set a bounty on $N!",ch,NULL,victim,TO_CHAR,FALSE);
    if ( !IS_SET(victim->comm,COMM_SILENCE) )
        act("The bounty on your head has been raised.",ch,NULL,victim,TO_VICT,FALSE);
    sprintf(log_buf,"%s sets a bounty of %ld on %s.",ch->name,amt,victim->name);
    log_string(log_buf);
    wiznet(log_buf,NULL,NULL,WIZ_SECURE,0,0);
    sprintf(log_buf,"A bounty of %ld has been placed on %s.",amt,victim->name);
    pnet(log_buf,NULL,NULL,PNET_BOUNTY,0,0);
    return;
}
