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

static char rcsid[] = "$Id: gamble.c,v 1.2 1999/07/14 18:13:39 mud Exp $";
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

void do_blackjack(CHAR_DATA *ch,char *argument)
{
    char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];                         
    char buf[MAX_STRING_LENGTH];

	argument = one_argument( argument, arg1 );
	argument = one_argument( argument, arg2 );   

    if( arg1[0] == '\0' )
    {
      send_to_char("You must bet, hit, stand or hand.\n\r",ch);
      return;
    }

    switch( LOWER(arg1[0]) )
    {
      case 'b': place_bet(ch,UMIN(30000,atoi(arg2))); break;
      case 'h': give_card(ch); show_bj_hand(ch,TRUE); check_bj_hand(ch); break;
      case 's': show_bj_hand(ch,TRUE); show_bj_hand(ch,FALSE); break;
      case 'h':
      default: show_bj_hand(ch,TRUE);
    }

    return;
}

void place_bet(CHAR_DATA *ch, int amount)
{
  char buf[MAX_STRING_LENGTH];

  sprintf(buf,"If this was working you would have bet %d silver.\n\r",amount);
  send_to_char(buf,ch);
  return;
}

void give_card(CHAR_DATA *ch)
{
  CARD_DATA *newcard;
  char buf[MAX_STRING_LENGTH];
  bool nCard=TRUE;
  sh_int i;

  if(ch->hand[4] != NULL)
  {
    sprintf(buf,"attempt to give %s another cardwith a full hand",ch->name);
    bug(buf);
    return;
  }

  for( i=0, i<5, i++)
  {
    if(ch->hand[i] == NULL)
      break;
  }

  while(nCard)
  {
    newcard = gen_card();
    if(!has_card(ch,newcard)) 
      
