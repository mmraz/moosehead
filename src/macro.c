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
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/

static char rcsid[] = "$Id: macro.c,v 1.2 1999/07/14 18:13:58 mud Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"

void clear_macro_marks ( CHAR_DATA *ch )
{
  MACRO_DATA *macro;
  
  macro = ch->pcdata->macro;  
  while (macro) {
    macro->mark = FALSE;
    macro = macro->next;
  }  
}

char *one_argument_spec ( char *argument, char *arg_first )
{
    char cEnd;

    while ( isspace(*argument) )
  argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
  cEnd = *argument++;

    while ( *argument != '\0' )
    {
  if ( *argument == cEnd )
  {
      argument++;
      break;
  }
  
  *arg_first = *argument;
  arg_first++;
  argument++;
    }
    *arg_first = '\0';

    while ( isspace(*argument) )
  argument++;

    return argument;
}

bool check_macro ( CHAR_DATA *ch, char *argument )
{
  MACRO_DATA *macro;
  char *buf,buf2[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH],*argp[100];
  int idx, len, cnt;
  
  if (IS_NPC (ch))
    return FALSE;
      
  argument = one_argument (argument, arg);  
  
  macro = ch->pcdata->macro;  
  while (macro) {
    if (!str_cmp (arg,macro->name)) {      
      if (macro->mark) {
        send_to_char ("Circular macro error.\n\r",ch);
        return TRUE;
      }       
      for (cnt = 0; cnt < 100; cnt++) argp[cnt] = NULL;
      idx = 0;
      while (arg[0] && (idx < 100)) {      /* load argument table */
        argp[idx++] = argument;
        argument = one_argument (argument,arg);
      }      
      ch->pcdata->macro_count++;
      macro->mark = TRUE;
      sprintf (buf2,"Macro '%s' invoked.\n\r",macro->name);
      send_to_char (buf2,ch);      
      buf = macro->text;
      idx = 0;      
      buf2[0] = 0;
      while (buf[idx]) {
        if (buf[idx] == '%') {
          cnt = buf[++idx] - '1';
          if ((cnt >= 0) && (cnt < 10)) {
            if (argp[cnt]) {
              one_argument_spec (argp[cnt],arg);
              strcat (buf2,arg);
            }
          }
        } else if ((buf[idx] == ';') || (buf[idx] == ':')) {
          len = strlen (buf2);          
          buf2[len]   = '\n';
          buf2[len+1] = NULL;                  
        } else if (buf[idx] == '$') {
          cnt = buf[++idx] - '1';
          if ((cnt >= 0) && (cnt < 10)) {
            strcat (buf2,argp[cnt]);
          }
        } else {          
          len = strlen (buf2);          
          buf2[len]   = buf[idx];
          buf2[len+1] = NULL;
        }
        idx++;
      }
      strcat (buf2,argument);
      len = strlen (buf2);
      buf2[len]   = '\n';       
      buf2[len+1] = 1;                /* signals end of macro */      
      buf2[len+2] = NULL;

      if(len + 4 + strlen(ch->desc->inbuf) > MAX_STRING_LENGTH)
      {// Protection from far too long input buffers
			  sprintf( log_buf, "%s input overflow!", ch->desc->host );
			  log_string( log_buf );
			  write_to_descriptor( ch->desc->descriptor,
			      "\n\r*** PUT A LID ON IT!!! ***\n\r", 0, ch->desc );
      	return FALSE;
      }

      strcat (buf2,ch->desc->inbuf);  /* make sure extra commands are after macro */
      strcpy (ch->desc->inbuf,buf2);  /* also needed for macros inside of macros */
     /* for (cnt = 0; cnt < 100; cnt++)
        if (argp[cnt])
          free_string (argp[cnt]);*/
      return TRUE;
    }
    macro = macro->next;    
  }  
  return FALSE;
}

void do_unmacro ( CHAR_DATA *ch, char *argument )
{
  MACRO_DATA *macro,*previous;
  char buf[MAX_STRING_LENGTH];
  char arg[MAX_STRING_LENGTH];
  
  one_argument (argument, arg);  
  if (!arg[0]) {
    send_to_char ("Syntax:  unmacro <macro name>\n\r",ch);
    return;
  }  
  macro = ch->pcdata->macro;
  previous = NULL;
  while (macro) {    
    if (!str_cmp (arg,macro->name)) {
      sprintf (buf,"Macro '%s' removed.\n\r",macro->name);      
      send_to_char (buf,ch);
      if (previous)
        previous->next = macro->next;
      else
        ch->pcdata->macro = macro->next;
      free_macro (macro);
      return;      
    }
    previous = macro;
    macro = macro->next;    
  }
  sprintf (buf,"Macro '%s' not found.\n\r",arg);      
  send_to_char (buf,ch);
}

void do_macro ( CHAR_DATA *ch, char *argument )
{
  char arg[MAX_STRING_LENGTH];
  char buf [MAX_STRING_LENGTH];
  MACRO_DATA *macro;
  int count = 0;
  
  if (IS_NPC(ch)) {
    send_to_char ("NPC's cannot set macro's.\n\r",ch);
    return;
  }
  
  if (!ch->pcdata) return;
  
  argument = one_argument (argument,arg);
  
  if (!arg[0]) {    
    macro = ch->pcdata->macro;
    if (macro == NULL) {
      send_to_char ("No macros defined.\n\r",ch);
      return;
    }
    send_to_char ("Macros defined as follows:\n\r",ch);
    while (macro) {
      sprintf (buf,"  %-8s - '%s'\n\r",macro->name,macro->text);
      send_to_char (buf,ch);
      macro = macro->next;
    }    
    return;
  }

  if (!str_cmp (arg,"macro")) {
    send_to_char ("You cannot define a macro named 'macro'\n\r",ch);
    return;
  }  
    
  macro = ch->pcdata->macro;
  while (macro) {
    count++;
    if (!str_cmp (arg,macro->name)) {
      sprintf (buf,"Macro '%s' already defined as '%s'.\n\r",macro->name,macro->text);
      send_to_char (buf,ch);
      return;      
    }
    macro = macro->next;
  }

  if (!argument[0]) {      
    send_to_char ("Macro not found.\n\r",ch);
    return;
  }
    
  if ((count > 15) && !IS_IMMORTAL (ch)) {
    send_to_char ("Maximum of 15 macros for mortals.\n\r",ch);
    return;
  }    
    
  if ((strlen (argument) > 80) && !IS_IMMORTAL(ch)) {
    send_to_char ("Macro truncated to 80 characters.\n\r",ch);
    argument[80] = NULL;    
  }
    
  macro = new_macro();    
  macro->next = ch->pcdata->macro;
  macro->name = str_dup (arg);
  macro->text = str_dup (argument);
  macro->mark = FALSE;
  ch->pcdata->macro = macro;  
  sprintf (buf,"Macro '%s' now defined as '%s'.\n\r",macro->name,macro->text);
  send_to_char (buf,ch);
  return;
}

