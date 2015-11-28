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
 
static char rcsid[] = "$Id: alias.c,v 1.5 2002/06/20 15:55:00 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"

/* does aliasing and other fun stuff */
void substitute_alias(DESCRIPTOR_DATA *d, char *argument)
{
    CHAR_DATA *ch;
    char buf[MAX_STRING_LENGTH], name[MAX_INPUT_LENGTH];
    char *point;
    int alias;

    ch = d->original ? d->original : d->character;

    if (IS_NPC(ch) || ch->pcdata->alias[0] == NULL
    ||  !str_prefix(argument,"alias") || !str_prefix(argument,"una")) 
    {
	interpret(d->character,argument);
	return;
    }

    strcpy(buf,argument);

    for (alias = 0; alias < MAX_ALIAS; alias++)  /* go through the aliases */
    {
	if (ch->pcdata->alias[alias] == NULL)
	    break;

	if (!str_prefix(ch->pcdata->alias[alias],argument))
	{
	    point = one_argument(argument,name);
	    if (!strcmp(ch->pcdata->alias[alias],name))
	    {
	    	buf[0] = '\0';
	    	strcat(buf,ch->pcdata->alias_sub[alias]);
		if (point[0]) {
		    strcat(buf," ");
		    strcat(buf,point);
		}

	    if (strlen(buf) > MAX_INPUT_LENGTH -1)
	    {
	      send_to_char("Alias substitution too long. Truncated.\r\n",ch);
	      buf[MAX_INPUT_LENGTH -1] = '\0';
	    }
	    break;
	    }
	}
    }
    interpret(d->character,buf);
}

void do_alia(CHAR_DATA *ch, char *argument)
{
    send_to_char("I'm sorry, alias must be entered in full.\n\r",ch);
    return;
}

void do_alias(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    int pos;

    if (ch->desc == NULL)
	rch = ch;
    else
	rch = ch->desc->original ? ch->desc->original : ch;

    if (IS_NPC(rch))
	return;

    if ( IS_SET(rch->affected_by, AFF_CHARM) )
	return;

    argument = one_argument(argument,arg);
    

    if (arg[0] == '\0')
    {

	if (rch->pcdata->alias[0] == NULL)
	{
	    send_to_char("You have no aliases defined.\n\r",ch);
	    return;
 	}
	send_to_char("Your current aliases are:\n\r",ch);

	for (pos = 0; pos < MAX_ALIAS; pos++)
	{
	    if (rch->pcdata->alias[pos] == NULL
	    ||  rch->pcdata->alias_sub[pos] == NULL)
		break;

	    sprintf(buf,"    %s:  %s\n\r",rch->pcdata->alias[pos],
		    rch->pcdata->alias_sub[pos]);
	    send_to_char(buf,ch);
	}
	return;
    }

    if (!str_prefix("una",arg) || !str_cmp("alias",arg))
    {
	send_to_char("Sorry, that word is reserved.\n\r",ch);
	return;
    }

   if (strchr(arg,' ')||strchr(arg,'"')||strchr(arg,'\'')) {      
   	send_to_char("The word to be aliased should not contain a space, "
   	    "a tick or a double-quote.\n\r",ch);
   	return;
      }   

    if (argument[0] == '\0')
    {
	for (pos = 0; pos < MAX_ALIAS; pos++)
	{
	    if (rch->pcdata->alias[pos] == NULL
	    ||  rch->pcdata->alias_sub[pos] == NULL)
		break;

	    if (!str_cmp(arg,rch->pcdata->alias[pos]))
	    {
		sprintf(buf,"%s aliases to '%s'.\n\r",rch->pcdata->alias[pos],
			rch->pcdata->alias_sub[pos]);
		send_to_char(buf,ch);
	   	return;
	    }
	}

	send_to_char("That alias is not defined.\n\r",ch);
	return;
    }
	

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
	if (rch->pcdata->alias[pos] == NULL)
	    break;

	if (!str_cmp(arg,rch->pcdata->alias[pos])) /* redefine an alias */
	{
	    free_string(rch->pcdata->alias_sub[pos]);
	    rch->pcdata->alias_sub[pos] = str_dup(argument);
	    sprintf(buf,"%s is now realiased to '%s'.\n\r",arg,argument);
	    send_to_char(buf,ch);
	    return;
	}
     }

     if (pos >= MAX_ALIAS)
     {
	send_to_char("Sorry, you have reached the alias limit.\n\r",ch);
	return;
     }
  
     /* make a new alias */
     rch->pcdata->alias[pos] 		= str_dup(arg);
     rch->pcdata->alias_sub[pos]	= str_dup(argument);
     sprintf(buf,"%s is now aliased to '%s'.\n\r",arg,argument);
     send_to_char(buf,ch);
}


void do_unalias(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *rch;
    char arg[MAX_INPUT_LENGTH];
    int pos;
    bool found = FALSE;
 
    if (ch->desc == NULL)
        rch = ch;
    else
        rch = ch->desc->original ? ch->desc->original : ch;
 
    if (IS_NPC(rch))
        return;
 
    argument = one_argument(argument,arg);

    if (arg[0] == '\0')
    {
	send_to_char("Unalias what?\n\r",ch);
	return;
    }

    for (pos = 0; pos < MAX_ALIAS; pos++)
    {
	if (rch->pcdata->alias[pos] == NULL)
	    break;

	if (found)
	{
	    rch->pcdata->alias[pos-1]		= rch->pcdata->alias[pos];
	    rch->pcdata->alias_sub[pos-1]	= rch->pcdata->alias_sub[pos];
	    rch->pcdata->alias[pos]		= NULL;
	    rch->pcdata->alias_sub[pos]		= NULL;
	    continue;
	}

	if(!strcmp(arg,rch->pcdata->alias[pos]))
	{
	    send_to_char("Alias removed.\n\r",ch);
	    free_string(rch->pcdata->alias[pos]);
	    free_string(rch->pcdata->alias_sub[pos]);
	    rch->pcdata->alias[pos] = NULL;
	    rch->pcdata->alias_sub[pos] = NULL;
	    found = TRUE;
	}
    }

    if (!found)
	send_to_char("No alias of that name to remove.\n\r",ch);
}

     













