/* Code specifically for the new skill system */

static char rcsid[] = "$Id: skills.c,v 1.81 2003/12/07 20:56:13 boogums Exp $";
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
#include "merc.h"
#include "magic.h"

/* command procedures needed */
DECLARE_DO_FUN(do_groups	);
DECLARE_DO_FUN(do_help		);
DECLARE_DO_FUN(do_say		);


/* used to get new skills */
void do_gain(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char arg1[MAX_INPUT_LENGTH];
    char *argN;
    CHAR_DATA *trainer;
    int gn = 0, sn = 0;

    if (IS_NPC(ch))
	return;

    /* find a trainer */
    for ( trainer = ch->in_room->people; 
	  trainer != NULL; 
	  trainer = trainer->next_in_room)
	if (IS_NPC(trainer) && IS_SET(trainer->act,ACT_GAIN))
	    break;

    if (trainer == NULL || !can_see(ch,trainer,FALSE))
    {
	send_to_char("You can't do that here.\n\r",ch);
	return;
    }

    argN = str_dup(argument);
    argN = one_argument(argN,arg);
    argN = one_argument(argN,arg1);

    if (arg[0] == '\0')
    {
	do_say(trainer,"Pardon me?");
	return;
    }

    if (!str_prefix(arg,"list"))
    {
	int col;
	
	col = 0;

	sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
	             "group","cost","group","cost","group","cost");
	send_to_char(buf,ch);

	for (gn = 0; gn < MAX_GROUP; gn++)
	{
	    if (group_table[gn].name == NULL)
		break;

	    if (!ch->pcdata->group_known[gn]
	    &&  group_table[gn].rating[ch->class] > 0)
	    {
	       sprintf(buf,"%-18s %-5d ",
	         group_table[gn].name,(group_table[gn].rating[ch->class]) * 10);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
	}
	if (col % 3 != 0)
	    send_to_char("\n\r",ch);
	
	send_to_char("\n\r",ch);		

	col = 0;

        sprintf(buf, "%-18s %-5s %-18s %-5s %-18s %-5s\n\r",
                     "skill","cost","skill","cost","skill","cost");
        send_to_char(buf,ch);
 
        for (sn = 0; sn < MAX_SKILL; sn++)
        {
            if (skill_table[sn].name == NULL)
                break;
 
            if (!ch->pcdata->learned[sn]
            &&  skill_table[sn].rating[ch->class] > 0
	    &&  skill_table[sn].spell_fun == spell_null)
            {
                sprintf(buf,"%-18s %-5d ",
                    skill_table[sn].name,(skill_table[sn].rating[ch->class]) * 10);
                send_to_char(buf,ch);
                if (++col % 3 == 0)
                    send_to_char("\n\r",ch);
            }
        }
        if (col % 3 != 0)
            send_to_char("\n\r",ch);

        if(ch->pcdata->retrain > 0 || ch->pcdata->half_retrain > 0)
        {// They have at least 1 train of some type to regain
          send_to_char("\n\r",ch);
          sprintf(buf,"%-18s %-5s\n\r", "extra", "cost"); 
          send_to_char(buf,ch);
          sprintf(buf,"%-18s %-5d ",
                    "Retrain", 20);
          send_to_char(buf,ch);
          send_to_char("\n\r",ch);
        }


        sprintf( buf, "\n\rYou have %d practice sessions left.\n\r",
           ch->practice );
        send_to_char(buf, ch);
	return;
    }

    if (!str_prefix(arg,"apply"))
    {
	if (ch->skill_points < 10) 
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR,FALSE); return;
	}
        if (!strcmp(arg1,"all"))
        {
	   send_to_char("You apply all of your skill towards practicing.\n\r",ch);
	   ch->practice += ch->skill_points/10;
	   ch->skill_points -= (ch->skill_points/10) * 10;
        }
	else
        {
	   send_to_char("You apply some of your skill towards practicing.\n\r",ch);
	   ch->skill_points -= 10;
	   ch->practice += 1;
        }
	return;
    }

    if(!str_prefix(arg, "retrain"))
    {/* Convert pracs to trains if they have any stored. Check half_trains first */
      if(ch->pcdata->half_retrain <= 0 && ch->pcdata->retrain <= 0)
      {
        send_to_char("You don't have any retrains available.\n\r", ch);
        return;
      }
      if(ch->practice < 20)
      {
        send_to_char("You need 20 practices to retrain.\n\r", ch);
        return;
      } 
      if(ch->pcdata->half_retrain > 0)
      {// half trains were gained pre-remort, so half retrains come back first
        ch->pcdata->half_retrain--;
        ch->pcdata->half_train++;
	ch->practice -= 20;
        act("$N helps you retrain.  You gain one half train.",
          ch,NULL,trainer,TO_CHAR,FALSE);
      }
      else
      {// Trains, the earlier check will have blocked it if neither is > 0
        ch->pcdata->retrain--;
        ch->train++;
	ch->practice -= 20;
        act("$N helps you retrain.  You gain one train.",
          ch,NULL,trainer,TO_CHAR,FALSE);
      }
      return;
    }

/* No more converting pracs to trains 
    if (!str_prefix(arg,"convert"))
    {
	if (ch->practice < 10)
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

	act("$N helps you apply your practice to training",
		ch,NULL,trainer,TO_CHAR,FALSE);
	ch->practice -= 10;
	ch->train +=1 ;
	return;
    }
    */

/*
    if (!str_prefix(arg,"points"))
    {
	if (ch->train < 2)
	{
	    act("$N tells you 'You are not yet ready.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

	if (ch->pcdata->points <= 40)
	{
	    act("$N tells you 'There would be no point in that.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

	act("$N trains you, and you feel more at ease with your skills.",
	    ch,NULL,trainer,TO_CHAR,FALSE);

	ch->train -= 2;
	ch->pcdata->points -= 1;
	ch->exp = exp_per_level(ch,ch->pcdata->points) * ch->level;
	return;
    }
    */

    /* else add a group/skill */

    gn = group_lookup(argument);
    if (gn > 0)
    {
	if (ch->pcdata->group_known[gn])
	{
	    act("$N tells you 'You already know that group!'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

	if (group_table[gn].rating[ch->class] <= 0)
	{
	    act("$N tells you 'That group is beyond your powers.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

/*
	if (ch->train < group_table[gn].rating[ch->class])
	if (ch->practice < (group_table[gn].rating[ch->class] * 10))
	*/
	if (ch->practice < (group_table[gn].rating[ch->class] * 10))
	{
	    act("$N tells you 'You are not yet ready for that group.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}

	/* add the group */
	gn_add(ch,gn);
	act("$N trains you in the art of $t",
	    ch,group_table[gn].name,trainer,TO_CHAR,FALSE);
	ch->practice -= (group_table[gn].rating[ch->class] * 10);
	/*
	ch->train -= group_table[gn].rating[ch->class];
	*/
	return;
    }

    sn = skill_lookup(argument);
    if (sn > -1)
    {
	if (skill_table[sn].spell_fun != spell_null)
	{
	    act("$N tells you 'You must learn the full group.'",
		ch,NULL,trainer,TO_CHAR,FALSE);
	    return;
	}
	    

        if (ch->pcdata->learned[sn])
        {
            act("$N tells you 'You already know that skill!'",
                ch,NULL,trainer,TO_CHAR,FALSE);
            return;
        }
 
        if (skill_table[sn].rating[ch->class] <= 0)
        {
            act("$N tells you 'That skill is beyond your powers.'",
                ch,NULL,trainer,TO_CHAR,FALSE);
            return;
        }
 
 /*
        if (ch->train < skill_table[sn].rating[ch->class])
	*/
        if (ch->practice < (skill_table[sn].rating[ch->class] * 10 ))
        {
            act("$N tells you 'You are not yet ready for that skill.'",
                ch,NULL,trainer,TO_CHAR,FALSE);
            return;
        }
 
        /* add the skill */
	ch->pcdata->learned[sn] = 1;
        act("$N trains you in the art of $t",
            ch,skill_table[sn].name,trainer,TO_CHAR,FALSE);
	    /*
        ch->train -= skill_table[sn].rating[ch->class];
	*/
        ch->practice -= (skill_table[sn].rating[ch->class] * 10);
        return;
    }

    act("$N tells you 'I do not understand...'",ch,NULL,trainer,TO_CHAR,FALSE);
}
    
int apply_chi(CHAR_DATA *ch, int num)
{
  if( number_percent() < get_skill(ch,gsn_chi) )
    {
     num -=  ((num / 2) * get_skill(ch,gsn_chi)) / 100 ;
     check_improve(ch,gsn_chi,TRUE,8);
    }
  else
     check_improve(ch,gsn_chi,FALSE,9);

  return num;
}

/* RT spells and skills show the players spells (or skills) */

void do_spells(CHAR_DATA *ch, char *argument)
{
//return;	
    char spell_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char spell_columns[LEVEL_HERO];
    int sn,lev,mana,lev0=0,levN=LEVEL_HERO;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    char buf2[4*MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
		    
	if (argument[0] != '\0')    
	{
	  argument = one_argument(argument,arg);
	  lev0 = atoi(arg);
	  lev0 = URANGE(0,lev0,LEVEL_HERO);
	}
		    
	if (argument[0] != '\0')    
	{
	  argument = one_argument(argument,arg);
	  levN = atoi(arg);
	  levN = URANGE(0,levN,LEVEL_HERO);
	}

    if (IS_NPC(ch))
      return;

    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++)
    {
	spell_columns[lev] = 0;
	spell_list[lev][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
      if (skill_table[sn].name == NULL)
        break;

      if (skill_level(ch,sn) < LEVEL_HERO &&
	  skill_table[sn].spell_fun != spell_null &&
          ch->pcdata->learned[sn] > 0)
      {
	found = TRUE;
	lev = skill_level(ch,sn);
	if (ch->level < lev)
	  sprintf(buf,"%-18s  n/a      ", skill_table[sn].name);
	else
	{
	     mana = mana_cost(ch,skill_table[sn].min_mana,
				  skill_level(ch,sn),sn);
	  
	  sprintf(buf,"%-18s  %3d mana  ",skill_table[sn].name,mana);
	}
	
	if (spell_list[lev][0] == '\0')
	  sprintf(spell_list[lev],"\n\rLevel %2d: %s",lev,buf);
        else /* append */
	{
	  if ( ++spell_columns[lev] % 2 == 0)
            strcat(spell_list[lev],"\n\r          ");
	  strcat(spell_list[lev],buf);
        }
      }
    }

    /* return results */
 
    if (!found)
    {
      send_to_char("You know no spells.\n\r",ch);
      return;
    }
    buf2[0] = '\0';
    for (lev = lev0; lev < UMIN(levN+1,LEVEL_HERO); lev++)
      if (spell_list[lev][0] != '\0')
       {
	strcat(buf2, spell_list[lev]);
      /*  strcat(buf2, "\n\r"); */
       }
      page_to_char(buf2, ch);
}

void do_skills(CHAR_DATA *ch, char *argument)
{
//return;
    char skill_list[LEVEL_HERO][MAX_STRING_LENGTH];
    char skill_columns[LEVEL_HERO];
    int sn,lev,lev0=0,levN=LEVEL_HERO;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    char buf2[4*MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
		    
	if (argument[0] != '\0')    
	{
	  argument = one_argument(argument,arg);
	  lev0 = atoi(arg);
	  lev0 = URANGE(0,lev0,LEVEL_HERO);
	}
		    
	if (argument[0] != '\0')    
	{
	  argument = one_argument(argument,arg);
	  levN = atoi(arg);
	  levN = URANGE(0,levN,LEVEL_HERO);
	}

 
    if (IS_NPC(ch))
      return;
 
    /* initilize data */
    for (lev = 0; lev < LEVEL_HERO; lev++)
    {
        skill_columns[lev] = 0;
        skill_list[lev][0] = '\0';
    }
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
      if (skill_table[sn].name == NULL )
        break;

 
      if ( skill_level( ch, sn ) < LEVEL_HERO &&
	  skill_table[sn].spell_fun == spell_null &&
	  ch->pcdata->learned[sn] > 0)
      {
        found = TRUE;
        lev = skill_level( ch, sn );
        if (ch->level < lev)
          sprintf(buf,"%-18s n/a      ", skill_table[sn].name);
        else
          sprintf(buf,"{%c%-18s{x %3d%%      ", 
		ch->pcdata->learned[sn] != ch->pcdata->last_learned[sn]
			? 'R' : 'x',
		skill_table[sn].name,
		ch->pcdata->learned[sn]);
	
	ch->pcdata->last_learned[sn] = ch->pcdata->learned[sn];

        if (skill_list[lev][0] == '\0')
          sprintf(skill_list[lev],"\n\rLevel %2d: %s",lev,buf);
        else /* append */
        {
          if ( ++skill_columns[lev] % 2 == 0)
            strcat(skill_list[lev],"\n\r          ");
          strcat(skill_list[lev],buf);
        }
      }
    }
     
    /* return results */
 
    if (!found)
    {
      send_to_char("You know no skills.\n\r",ch);
      return;
    }
    buf2[0] = '\0';
    for (lev = lev0; lev < UMIN(levN+1,LEVEL_HERO); lev++)
      if (skill_list[lev][0] != '\0')
       {
        strcat(buf2, skill_list[lev]);
       /* strcat(buf2,"\n\r"); */
       }

      if ( ch->pcdata->specialize )
      {
	 sprintf(buf,"\n\rSpecialization: %s\n\r",
		skill_table[ch->pcdata->specialize].name);
	 strcat(buf2,buf);
      }

      page_to_char(buf2, ch);
}


/* shows skills, groups and costs (only if not bought) */
void list_group_costs(CHAR_DATA *ch)
{
    char buf[100];
    int gn,sn,col,newLevel;

    if (IS_NPC(ch))
	return;

    col = 0;

    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","group","cp","group","cp","group","cp");
    send_to_char(buf,ch);

    for (gn = 0; gn < MAX_GROUP; gn++)
    {
	if (group_table[gn].name == NULL)
	    break;

        if (!ch->gen_data->group_chosen[gn] 
	&&  !ch->pcdata->group_known[gn]
	&&  group_table[gn].rating[ch->class] > 0)
	{
	    sprintf(buf,"%-18s %-5d ",group_table[gn].name,
				    group_table[gn].rating[ch->class]);
	    send_to_char(buf,ch);
	    if (++col % 3 == 0)
		send_to_char("\n\r",ch);
	}
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s\n\r","skill","cp","skill","cp","skill","cp");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (!ch->gen_data->skill_chosen[sn] 
	&&  ch->pcdata->learned[sn] == 0
	&&  skill_table[sn].spell_fun == spell_null
	&&  skill_table[sn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",skill_table[sn].name,
                                    skill_table[sn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);

    sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
    send_to_char(buf,ch);
    sprintf(buf,"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    if(IS_SET(ch->mhs,MHS_PREFRESHED))
    {
       newLevel = ch->exp / exp_per_level(ch,ch->pcdata->points);
       newLevel -= 1;
       if( ( ch->level < 11 ) && (newLevel > ch->level ) )
          newLevel = ch->level ;
       if(   IS_SET(ch->act,PLR_VAMP) ||
          IS_SET(ch->act,PLR_WERE) ||
          IS_SET(ch->act,PLR_MUMMY) 
           )
       {
          if ( newLevel > 76 )
             newLevel = 76;
       }
       else
       {
          if(newLevel > 50 )
             newLevel =50;
       }
       sprintf(buf,"Debit levels: %d\n\r",newLevel);
       send_to_char(buf,ch);
    }
    return;
}


void list_group_chosen(CHAR_DATA *ch)
{
    char buf[100];
    int gn,sn,col;
 
    if (IS_NPC(ch))
        return;
 
    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","group","cp","group","cp","group","cp\n\r");
    send_to_char(buf,ch);
 
    for (gn = 0; gn < MAX_GROUP; gn++)
    {
        if (group_table[gn].name == NULL)
            break;
 
        if (ch->gen_data->group_chosen[gn] 
	&&  group_table[gn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",group_table[gn].name,
                                    group_table[gn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    col = 0;
 
    sprintf(buf,"%-18s %-5s %-18s %-5s %-18s %-5s","skill","cp","skill","cp","skill","cp\n\r");
    send_to_char(buf,ch);
 
    for (sn = 0; sn < MAX_SKILL; sn++)
    {
        if (skill_table[sn].name == NULL)
            break;
 
        if (ch->gen_data->skill_chosen[sn] 
	&&  skill_table[sn].rating[ch->class] > 0)
        {
            sprintf(buf,"%-18s %-5d ",skill_table[sn].name,
                                    skill_table[sn].rating[ch->class]);
            send_to_char(buf,ch);
            if (++col % 3 == 0)
                send_to_char("\n\r",ch);
        }
    }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
    send_to_char("\n\r",ch);
 
    sprintf(buf,"Creation points: %d\n\r",ch->gen_data->points_chosen);
    send_to_char(buf,ch);
    sprintf(buf,"Experience per level: %d\n\r",
	    exp_per_level(ch,ch->gen_data->points_chosen));
    send_to_char(buf,ch);
    return;
}

int exp_per_level(CHAR_DATA *ch, int points)
{
    int expl,inc,noneg;
    int race;

    race = ( IS_SET(ch->mhs,MHS_SHAPESHIFTED) ? ch->save_race : ch->race );

    if (IS_NPC(ch))
        return 1000;

    expl = 1000;
    inc = 500;

    if (points < 40)
        return 1000 * pc_race_table[race].class_mult[ch->class]/100;

    /* processing */
    points -= 40;

    while (points > 9)
    {
        expl += inc;
        points -= 10;
        if (points > 9)
        {
            expl += inc;
            inc *= 2;
            points -= 10;
        }
    }

    expl += points * inc / 10;

    noneg = expl * pc_race_table[race].class_mult[ch->class]/100;
    if (noneg > 411000 || noneg < 0)
	return 411000;

    return expl * pc_race_table[race].class_mult[ch->class]/100;
}

/* this procedure handles the input parsing for the skill generator */
bool parse_gen_groups(CHAR_DATA *ch,char *argument)
{
    char arg[MAX_INPUT_LENGTH];
    char buf[100];
    int gn,sn,i;
 
    if (argument[0] == '\0')
	return FALSE;

    argument = one_argument(argument,arg);

    if (!str_prefix(arg,"help"))
    {
	if (argument[0] == '\0')
	{
	    do_help(ch,"group help");
	    return TRUE;
	}

        do_help(ch,argument);
	return TRUE;
    }
//CP cap IS WHERE WE NEED TO REMOVE THESE IF STATEMENTS TO REMOVE THE CAP
//COREY PAY ATTENTION
    if (!str_prefix(arg,"add"))
    {
	if (argument[0] == '\0')
	{
	    send_to_char("You must provide a skill name.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1)
	{
//101 was the old cap
	  if(ch->gen_data->points_chosen + group_table[gn].rating[ch->class] < 151)
	  {
	    if (ch->gen_data->group_chosen[gn]
	    ||  ch->pcdata->group_known[gn])
	    {
		send_to_char("You already know that group!\n\r",ch);
		return TRUE;
	    }

	    if (group_table[gn].rating[ch->class] < 1)
	    {
	  	send_to_char("That group is not available.\n\r",ch);
	 	return TRUE;
	    }

	    sprintf(buf,"%s group added\n\r",group_table[gn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->group_chosen[gn] = TRUE;
	    ch->gen_data->points_chosen += group_table[gn].rating[ch->class];
	    gn_add(ch,gn);
	    ch->pcdata->points += group_table[gn].rating[ch->class];
	    return TRUE;
	  }
	  else
	  {
	    sprintf(buf,"That group costs %d and you only have %dCP's left under the CP cap.\n\r",group_table[gn].rating[ch->class],(150 - ch->gen_data->points_chosen));
	    send_to_char(buf,ch);
	    return TRUE;
	  }
	}

	sn = skill_lookup(argument);
	if (sn != -1)
	{
	  if(ch->gen_data->points_chosen + skill_table[sn].rating[ch->class] < 151)
	  {
	    if (ch->gen_data->skill_chosen[sn]
	    ||  ch->pcdata->learned[sn] > 0)
	    {
		send_to_char("You already know that skill!\n\r",ch);
		return TRUE;
	    }

	    if (skill_table[sn].rating[ch->class] < 1
	    ||  skill_table[sn].spell_fun != spell_null)
	    {
		send_to_char("That skill is not available.\n\r",ch);
		return TRUE;
	    }
	    sprintf(buf, "%s skill added\n\r",skill_table[sn].name);
	    send_to_char(buf,ch);
	    ch->gen_data->skill_chosen[sn] = TRUE;
	    ch->gen_data->points_chosen += skill_table[sn].rating[ch->class];
	    ch->pcdata->learned[sn] = 1;
	    ch->pcdata->points += skill_table[sn].rating[ch->class];
	    return TRUE;
	  }
	  else
	  {
	    sprintf(buf,"That skill costs %d and you only have %dCP's left under the CP cap.\n\r",skill_table[sn].rating[ch->class],(150 - ch->gen_data->points_chosen));
	    send_to_char(buf,ch);
	    return TRUE;
	  }
	}

	send_to_char("No skills or groups by that name...\n\r",ch);
	return TRUE;
    }

    if (!strcmp(arg,"drop"))
    {
	if (argument[0] == '\0')
  	{
	    send_to_char("You must provide a skill to drop.\n\r",ch);
	    return TRUE;
	}

	gn = group_lookup(argument);
	if (gn != -1 && ch->gen_data->group_chosen[gn])
	{
	    send_to_char("Group dropped.\n\r",ch);
	    ch->gen_data->group_chosen[gn] = FALSE;
	    ch->gen_data->points_chosen -= group_table[gn].rating[ch->class];
	    gn_remove(ch,gn);
	    for (i = 0; i < MAX_GROUP; i++)
	    {
		if (ch->gen_data->group_chosen[i])
		    gn_add(ch,i);
	    }
	    ch->pcdata->points -= group_table[gn].rating[ch->class];
	    return TRUE;
	}

	sn = skill_lookup(argument);
	if (sn != -1 && ch->gen_data->skill_chosen[sn])
	{
	    send_to_char("Skill dropped.\n\r",ch);
	    ch->gen_data->skill_chosen[sn] = FALSE;
	    ch->gen_data->points_chosen -= skill_table[sn].rating[ch->class];
	    ch->pcdata->learned[sn] = 0;
	    ch->pcdata->points -= skill_table[sn].rating[ch->class];
	    return TRUE;
	}

	send_to_char("You haven't bought any such skill or group.\n\r",ch);
	return TRUE;
    }

    if (!str_prefix(arg,"premise"))
    {
	do_help(ch,"premise");
	return TRUE;
    }

    if (!str_prefix(arg,"list"))
    {
	list_group_costs(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"learned"))
    {
	list_group_chosen(ch);
	return TRUE;
    }

    if (!str_prefix(arg,"info"))
    {
	do_groups(ch,argument);
	return TRUE;
    }

    return FALSE;
}
	    
	


        

/* shows all groups, or the sub-members of a group */
void do_groups(CHAR_DATA *ch, char *argument)
{
    char buf[100];
    int gn,sn,col;

    if (IS_NPC(ch))
	return;

    col = 0;

    if (argument[0] == '\0')
    {   /* show all groups */
	
	for (gn = 0; gn < MAX_GROUP; gn++)
        {
	    if (group_table[gn].name == NULL)
		break;
	    if (ch->pcdata->group_known[gn])
	    {
		sprintf(buf,"%-20s ",group_table[gn].name);
		send_to_char(buf,ch);
		if (++col % 3 == 0)
		    send_to_char("\n\r",ch);
	    }
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
        sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
	send_to_char(buf,ch);
	return;
     }

     if (!str_cmp(argument,"all"))    /* show all groups */
     {
        for (gn = 0; gn < MAX_GROUP; gn++)
        {
            if (group_table[gn].name == NULL)
                break;
	    sprintf(buf,"%-20s ",group_table[gn].name);
            send_to_char(buf,ch);
	    if (++col % 3 == 0)
            	send_to_char("\n\r",ch);
        }
        if ( col % 3 != 0 )
            send_to_char( "\n\r", ch );
	return;
     }
	
     
     /* show the sub-members of a group */
     gn = group_lookup(argument);
     if (gn == -1)
     {
	send_to_char("No group of that name exist.\n\r",ch);
	send_to_char(
	    "Type 'groups all' or 'info all' for a full listing.\n\r",ch);
	return;
     }

     for (sn = 0; sn < MAX_IN_GROUP; sn++)
     {
	if (group_table[gn].spells[sn] == NULL)
	    break;
	sprintf(buf,"%-20s ",group_table[gn].spells[sn]);
	send_to_char(buf,ch);
	if (++col % 3 == 0)
	    send_to_char("\n\r",ch);
     }
    if ( col % 3 != 0 )
        send_to_char( "\n\r", ch );
}

/* returns skill level */
int skill_level( CHAR_DATA *ch, int sn )
{
    int iOld_class = 0;

    /* Exceptions first - gargoyles */
    if ( sn == gsn_stone_skin && ch->race == race_lookup("gargoyle") && !IS_NPC(ch) )
    {	
	/* the break is superfluous but it's good coding style */
	switch(class_table[ch->pcdata->old_class].fMana)
	{
	case 0:	return 15; break;
	case 1: return 10; break;
	case 2: return 5; break;
	default:	break;
	}
    }

    if ( skill_table[sn].skill_level[ch->class] > 51 )
	return 53;

    if ( skill_table[sn].skill_level[ch->class] < 1 )
	return ( skill_table[sn].skill_level[ch->class] );

     /* Leave as is for normal classes */
    if ( !class_table[ch->class].reclass )
	return ( skill_table[sn].skill_level[ch->class] );

    /* if oldclass gets it, give it at that level */
    if ( skill_table[sn].skill_level[ch->pcdata->old_class] <= 51 )
	return ( skill_table[sn].skill_level[ch->pcdata->old_class] );

    /* otherwise use the Prowler Method */
    /* get other old class */
    iOld_class = ( class_table[ch->class].allowed[0] == ch->pcdata->old_class )
		 ? class_table[ch->class].allowed[1] :
		   class_table[ch->class].allowed[0];

    if ( skill_table[sn].skill_level[iOld_class] <= 51 )
	return ( ( skill_table[sn].skill_level[iOld_class] +
		   skill_table[sn].skill_level[ch->class] ) / 2 );

     /* if it's still here it's a reclass specific skill */
      /* so, just return the level */
     return ( skill_table[sn].skill_level[ch->class] );
}

/* checks for skill improvement */
void check_improve( CHAR_DATA *ch, int sn, bool success, int multiplier )
{
    int chance, blahy,blah,skillpoints;
    char buf[100];

    if (IS_NPC(ch))
	return;

    if ( HAS_KIT(ch,"prophet") )
    {
	multiplier = UMAX(1, multiplier/2 );
	success = TRUE;
    }

/*
    if ( IS_SET(ch->mhs,MHS_SAVANT) )
	multiplier = 1;
*/

    blah = abs(skill_table[sn].rating[ch->class]);

    /* Only gain skill points if your skill % is at 100 */
    if (ch->pcdata->learned[sn] >= 80 && 
	ch->position == POS_FIGHTING &&
	ch->pcdata->skill_point_tracker < 5)
    {
       blahy = number_percent();
       /*
       if ( number_percent() * number_percent() < UMAX(2,10 - blah) )
       */
       if ( blahy <= (10 - blah)/2 )
       {
       /*
          skillpoints = UMAX(1,10-blah);
	  */
	  if (skill_table[sn].spell_fun != spell_null)  
	     skillpoints = 2;
	  else
	     skillpoints = 1;

	  sprintf(buf,"Your use of {G%s{x has gained you some {Wskill{x!\n\r",
                  skill_table[sn].name
                  );
	    send_to_char(buf,ch);
            ch->skill_points += skillpoints;
	    ch->pcdata->skill_point_tracker += skillpoints;

	    // Kick off the timer.
	    if(ch->pcdata->skill_point_timer <= 0)
		    ch->pcdata->skill_point_timer = 2;
       }
    }

    if (ch->level < skill_level(ch,sn)
    ||  skill_table[sn].rating[ch->class] == 0
    ||  ch->pcdata->learned[sn] <= 1
    ||  ch->pcdata->learned[sn] == 100)
	return;  /* skill is not known or at 1% */ 

/* Removed from here, its a skill point bug Poquah 
    blah = abs(skill_table[sn].rating[ch->class]);
    if ( number_percent() * number_percent() < UMAX(2,10 - blah) )
	ch->skill_points += UMAX(1,10-blah);
	*/

    /* check to see if the character has a chance to learn */
    chance = 10 * int_app[get_curr_stat(ch,STAT_INT)].learn;
    chance /= (		multiplier * blah *	4);
    chance += ch->level;

    if (number_range(1,1000) > chance)
	return;

    /* now that the character has a CHANCE to learn, see if they really have */	

    if (success)
    {
	chance = URANGE(5,100 - ch->pcdata->learned[sn], 95);
	if (number_percent() < chance)
	{
	    ch->pcdata->learned[sn] += number_range(2,3);
	    ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],100);
	    sprintf(buf,"You have become better at %s! ({G%d{x)\n\r",
		    skill_table[sn].name,
		    ch->pcdata->learned[sn]);
	    send_to_char(buf,ch);
	    gain_exp(ch,2 * blah);
	}
    }

    else
    {
	chance = URANGE(5,ch->pcdata->learned[sn]/2,30);
	if (number_percent() < chance)
	{
	    ch->pcdata->learned[sn]++;
	    /* Special chance to really learn a thing or two */
	    if ( number_percent() > 95 )
	    ch->pcdata->learned[sn] += number_range(3,5);
		
	    ch->pcdata->learned[sn] = UMIN(ch->pcdata->learned[sn],100);
	    sprintf(buf,
	"Learning from your mistakes, your %s skill improves to {G%d{x%%.\n\r",
		skill_table[sn].name,ch->pcdata->learned[sn]);
	    send_to_char(buf,ch);
	    gain_exp(ch,2 * blah);
	}
    }

}

/* returns a group index number given the name */
int group_lookup( const char *name )
{
    int gn;
 
    for ( gn = 0; gn < MAX_GROUP; gn++ )
    {
        if ( group_table[gn].name == NULL )
            break;
        if ( LOWER(name[0]) == LOWER(group_table[gn].name[0])
        &&   !str_prefix( name, group_table[gn].name ) )
            return gn;
    }
 
    return -1;
}

/* recursively adds a group given its number -- uses group_add */
void gn_add( CHAR_DATA *ch, int gn)
{
    int i;
    
    ch->pcdata->group_known[gn] = TRUE;
    for ( i = 0; i < MAX_IN_GROUP; i++)
    {
        if (group_table[gn].spells[i] == NULL)
            break;
        group_add(ch,group_table[gn].spells[i],FALSE);
    }
}

/* recusively removes a group given its number -- uses group_remove */
void gn_remove( CHAR_DATA *ch, int gn)
{
    int i;

    ch->pcdata->group_known[gn] = FALSE;

    for ( i = 0; i < MAX_IN_GROUP; i ++)
    {
	if (group_table[gn].spells[i] == NULL)
	    break;
	group_remove(ch,group_table[gn].spells[i]);
    }
}
	
/* use for processing a skill or group for addition  */
void group_add( CHAR_DATA *ch, const char *name, bool deduct)
{
    int sn,gn;

    if (IS_NPC(ch)) /* NPCs do not have skills */
	return;

    sn = skill_lookup(name);

    if (sn != -1)
    {
	if (ch->pcdata->learned[sn] == 0) /* i.e. not known */
	{
	    ch->pcdata->learned[sn] = 1;
	    if (deduct)
	   	ch->pcdata->points += skill_table[sn].rating[ch->class]; 
	}
	return;
    }
	
    /* now check groups */

    gn = group_lookup(name);

    if (gn != -1)
    {
	if (ch->pcdata->group_known[gn] == FALSE)  
	{
	    ch->pcdata->group_known[gn] = TRUE;
	    if (deduct)
		ch->pcdata->points += group_table[gn].rating[ch->class];
	}
	gn_add(ch,gn); /* make sure all skills in the group are known */
    }
}

/* used for processing a skill or group for deletion -- no points back! */

void group_remove(CHAR_DATA *ch, const char *name)
{
    int sn, gn;
    
     sn = skill_lookup(name);

    if (sn != -1)
    {
	ch->pcdata->learned[sn] = 0;
	return;
    }
 
    /* now check groups */
 
    gn = group_lookup(name);
 
    if (gn != -1 && ch->pcdata->group_known[gn] == TRUE)
    {
	ch->pcdata->group_known[gn] = FALSE;
	gn_remove(ch,gn);  /* be sure to call gn_add on all remaining groups */
    }
}
