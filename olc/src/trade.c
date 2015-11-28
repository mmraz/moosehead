static char rcsid[] = "$Id: trade.c,v 1.15 2003/09/20 18:34:20 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>

#include "merc.h"
#include "recycle.h"

#define     PLR_NOTRADE         0
#define     TRADE_REQUEST       1
#define     TRADE_DECLINE       2
#define     TRADE_ADD           3
#define     TRADE_REMOVE        4
#define     TRADE_APPROVE       5
#define     TRADE_CANCEL        6
#define     TRADE_SHOW          7
#define     TRADE_CLEAR         8
#define     TRADE_ACCEPT        9
#define     TRADE_IDENTIFY      10

/* Import */
DECLARE_DO_FUN( do_help );

/*OBJ_DATA *get_obj_trade( TRADE_DATA *trade, int trader, char *argument )
{
    OBJ_DATA *obj;

    for ( obj = trade->items[trader] ;
          obj != NULL ;
          obj = obj->next_in_trade )
        if ( is_name( argument, obj->name ) )
            return obj;
    
    return NULL;
}*/
/* This is much more complicated now to handle x.item, particularily for trade identify */
OBJ_DATA *get_obj_trade( TRADE_DATA *trade, int trader, char *argument )
{
    OBJ_DATA *obj;
    int number, count;
    char arg[256];
    int switch_trader = 0;

    number = number_argument( argument, arg );
    count  = 0;

    if(trader > 1) /* Both traders - used for trade identify */
    {// 2 is trader 0 first, else trader 1 first
    /* switch_trader - 1 is used later to reset trader*/
      if(trader == 2)
      {
        switch_trader = 2;
        trader = 0;
      }
      else
      {
        switch_trader = 1;
        trader = 1;
      }
    }
    
    while(1)
    {
      for ( obj = trade->items[trader] ;
            obj != NULL ;
            obj = obj->next_in_trade )
      {
          if ( is_name( argument, obj->name ) )
          {
              count++;
              if(count == number)
                return obj;
          }
      }
      if(switch_trader)
      {// Rerun with the other trader's items
        trader = switch_trader - 1;
        switch_trader = 0;
        continue;
      }
      break;
    }
    return NULL;
}

bool obj_to_trade( OBJ_DATA *obj, TRADE_DATA *trade, int trader )
{
	/* Commented out again
    if ( IS_SET(obj->extra_flags,ITEM_NODROP) )
    	return FALSE;
	 */

    /* no trading Nethermancer Robes */
    if ( obj->pIndexData->vnum == OBJ_VNUM_ROBES )
    	return FALSE;

    if(obj->link_name)
      return FALSE;

    obj->next_in_trade = trade->items[trader];
    trade->items[trader] = obj;
    return TRUE;
}

void obj_from_trade( OBJ_DATA *obj, TRADE_DATA *trade, int trader )
{
    OBJ_DATA *tObj;

        if ( trade->items[trader] == obj )
        {
            trade->items[trader] = obj->next_in_trade;
            obj->next_in_trade = NULL;
            return;
        }

        for ( tObj = trade->items[trader] ;
              tObj != NULL;
              tObj = tObj->next_in_trade )
            if ( tObj->next_in_trade == obj )
                break;

        if ( tObj == NULL )
        {
            bug("obj_from_trade: NULL tObj",0);
            return;
        }

        tObj->next_in_trade = obj->next_in_trade;
        obj->next_in_trade = NULL;
        return;
}

void destruct_trade( TRADE_DATA *trade, bool iFree )
{
    int i;
    OBJ_DATA *obj, *obj_next;

        for ( i = 0 ; i < 2 ; i++ )
        {
            trade->trader[i]->gold += trade->gold[i];
            trade->trader[i]->silver += trade->silver[i];
            trade->gold[i] = 0;
            trade->silver[i] = 0;
            trade->approved[i] = 0;

            for ( obj = trade->items[i] ;
                  obj != NULL ;
                  obj = obj_next )
            {
               obj_next = obj->next_in_trade;

               obj->next_in_trade = NULL;
               obj_to_char( obj, trade->trader[i] );
            }
            trade->items[i] = NULL;
        }

	if ( iFree )
	{
	trade->trader[0]->trade = NULL;
	trade->trader[1]->trade = NULL;
	free_trade( trade );
	}
}


void do_trade( CHAR_DATA *ch, char *argument )
{
    TRADE_DATA *trade;
    CHAR_DATA *victim;
    OBJ_DATA *obj, *obj_next;
    int trader;
    char cmd[MAX_INPUT_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    int amount = -1;
    int state;

    argument = one_argument( argument, cmd );
    argument = one_argument( argument, arg );

    if ( argument[0] != '\0' && is_number( argument ) )
        amount = atoi( argument );

/*
    if ( cmd[0] == '\0' || arg[0] == '\0' )
    {
        do_help(ch,"trade");
        return;
    }
*/
    /*Added checks below on 21MAY01 - Boogums*/
    /*Yanking the wraithform check until i can get home and look at it*/
    if( is_affected(ch, skill_lookup("wraithform")) )
    {
        send_to_char("You may not trade while in wraith form.\n\r",ch);
        return;
    }
    /*
    victim = get_char_room(ch,arg); 
    if ( victim == ch )
    {
  send_to_char( "That's pointless.\n\r", ch );
  return;
    }

    if( is_affected(victim, skill_lookup("wraithform")) )
    {
        send_to_char("You may not trade with them right now.\n\r",ch);
        return;
    }
    */
    if(cmd[0] == '\0')
    {
        do_help(ch,"trade");
        return;
    }
    if ( !str_prefix( cmd, "request" ) )
        state = TRADE_REQUEST;
    else
    if ( !str_prefix( cmd, "accept" ) )
        state = TRADE_ACCEPT;
    else
    if ( !str_prefix( cmd, "approve" ) )
        state = TRADE_APPROVE;
    else
    if ( !str_prefix( cmd, "decline" ) )
        state= TRADE_DECLINE;
    else
    if ( !str_prefix( cmd, "add" ) )
        state = TRADE_ADD;
    else
    if ( !str_prefix( cmd, "remove" ) )
        state = TRADE_REMOVE;
    else
    if ( !str_prefix( cmd, "cancel" ) )
        state = TRADE_CANCEL;
    else
    if ( !str_prefix( cmd, "clear" ) )
        state = TRADE_CLEAR;
    else
    if ( !str_prefix( cmd, "view" ) || !str_prefix( cmd, "list" ) ||
         !str_prefix( cmd, "show" ) )
         state = TRADE_SHOW;
    else
    if ( !str_prefix( cmd, "identify" ) )
         state = TRADE_IDENTIFY; 
    else
    {
        do_help(ch,"trade");
        return;
    }

    /* now for the guts */
    switch( state )
    {
    default:
        bug("do_trade: bad command",0);
        break;

    case TRADE_REQUEST:
	if(arg[0] == '\0')
	{
	    send_to_char("Trade with who?\n\r", ch);
	    return;
	}

        if ( ( victim = get_char_room(ch,arg) ) == NULL )
        {
            send_to_char("That person isn't here.\n\r",ch);
            return;
        }
        if(victim == ch)
        {
            send_to_char("You cannot trade with yourself.\n\r", ch);
            return;
        }
        if( is_affected(victim, skill_lookup("wraithform")) )
	{
	    send_to_char("You cannot trade with them right now.\n\r",ch);
	    return;
	}

        if ( ch->trade != NULL )
        {
            send_to_char("You are already trading with someone else.\n\r",ch);
            return;
        }

        if ( IS_SET(victim->act, PLR_NOTRADE) )
        {
            act("$N doesn't wish to trade with anybody.",ch,NULL,victim,TO_CHAR,FALSE);
            return;
        }

        if ( victim->trade != NULL )
        {
            act("$N is already trading with somebody.",ch,NULL,victim,TO_CHAR,FALSE);
            return;
        }

        if ( IS_NPC(victim) )
        {
            send_to_char("NPC's don't trade with anybody.\n\r",ch);
            return;
        }

        if ( ch->pcdata->req_trade != NULL )
        {
            act("You haven't answered $N yet.",
                ch,NULL,ch->pcdata->req_trade,TO_CHAR,FALSE);
            return;
        }

        act("You request a trade with $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n requests a trade with you.",ch,NULL,victim,TO_VICT,FALSE);
        
        victim->pcdata->req_trade = ch;
        break;

    case TRADE_ACCEPT:
        if ( ( victim = ch->pcdata->req_trade ) == NULL )    
        {
            send_to_char("No one is trading with you.\n\r",ch);
            return;
        }

        act("You accept the invitation to trade with $N.",
                        ch,NULL,victim,TO_CHAR,FALSE);
        act("$n accepts your invitation to trade.",
                        ch,NULL,victim,TO_VICT,FALSE);
        

        ch->pcdata->req_trade = NULL;

        trade = new_trade();

        trade->trader[0] = ch;
        trade->trader[1] = victim;
        ch->trade = trade;
        victim->trade = trade;
        break;

    case TRADE_DECLINE:
        if ( ( victim = ch->pcdata->req_trade ) == NULL )
        {
            send_to_char("No one has offered to trade with you.\n\r",ch);
            return;
        }

        act("You decline the offer from $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n declines your offer.",ch,NULL,victim,TO_VICT,FALSE);
        ch->pcdata->req_trade = NULL;
        break;

    case TRADE_CANCEL:
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You're not trading with anybody.\n\r",ch);
            return;
        }

        victim = ( trade->trader[0] == ch ? trade->trader[1] :
                                            trade->trader[0] );
       
        destruct_trade( trade, TRUE );

        act("You cancel the trade.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n has cancelled the trade.",ch,NULL,victim,TO_VICT,FALSE);
        break;        

    case TRADE_CLEAR:
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You're not trading with anybody.\n\r",ch);
            return;
        }

	victim = ( trade->trader[0] == ch ? trade->trader[1] :
					    trade->trader[0] );
        
	destruct_trade( trade, FALSE );
        act("You clear the trade." ,ch,NULL,victim,TO_CHAR,FALSE);
        act("$n has cleared the trade.",ch,NULL,victim,TO_VICT,FALSE);
        break;        

    case TRADE_SHOW:
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You're not trading with anybody.\n\r",ch);
            return;
        }

        /* show trade, find trader */
        trader = ( trade->trader[0] == ch ? 0 : 1 );

        send_to_char("You are offering:\n\r",ch);
        if ( trade->gold[trader] || trade->silver[trader] )
        {
            sprintf(buf,"  * %d gold and %d silver\n\r",
                    trade->gold[trader], trade->silver[trader] );
            send_to_char(buf,ch);
        }
        for ( obj = trade->items[trader] ; 
              obj != NULL ;
              obj = obj->next_in_trade )
            act("   * $p",ch,obj,NULL,TO_CHAR,FALSE);
        send_to_char("\n\rin exchange for:\n\r",ch);
        if ( trade->gold[!trader] || trade->silver[!trader] )
        {
            sprintf(buf,"   * %d gold and %d silver\n\r",
                trade->gold[!trader], trade->silver[!trader] );
            send_to_char(buf,ch);
        }
        for ( obj = trade->items[!trader] ;
              obj != NULL;
              obj = obj->next_in_trade )
           act("   * $p",ch,obj,NULL,TO_CHAR,FALSE);

        break;
        
    case TRADE_APPROVE:
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You are not trading with anybody.\n\r",ch);
            return;
        }

        /* show trade, find trader */
        trader = ( trade->trader[0] == ch ? 0 : 1 );
        victim = ( trade->trader[!trader] );

        trade->approved[trader] = TRUE;

        act("You approve the trade.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n approves the trade.",ch,NULL,victim,TO_VICT,FALSE);

        if ( trade->approved[!trader] )
        {
            act("The trade is final.",ch,NULL,victim,TO_CHAR,FALSE);
            act("The trade is final.",ch,NULL,victim,TO_VICT,FALSE);

            /* Both people approve */
            ch->gold += trade->gold[!trader];
            ch->silver += trade->silver[!trader];
            victim->gold += trade->gold[trader];
            victim->silver += trade->silver[trader];
            /* Transfer objects */
            for ( obj = trade->items[trader] ;
                  obj != NULL;
                  obj = obj_next )
            {
                obj_next = obj->next_in_trade;

                obj->next_in_trade = NULL;            
                obj_to_char( obj, trade->trader[!trader] );
            }
            
            for ( obj = trade->items[!trader] ;
                  obj != NULL;
                  obj = obj_next )
            {
                obj_next = obj->next_in_trade;

                obj->next_in_trade = NULL;
                obj_to_char( obj, trade->trader[trader] );
            }

	    trade->trader[0]->trade = NULL;
	    trade->trader[1]->trade = NULL;

            free_trade( trade );
            /* DONE! */
         }

        break;

    case TRADE_ADD: /* This is where this begins to suck */
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You are not trading with anybody.\n\r",ch);
            return;
        }
        
        trader = ( trade->trader[0] == ch ? 0 : 1 );
        victim =   trade->trader[!trader];

        /* Always reset approvals */
        trade->approved[0] = FALSE;
        trade->approved[1] = FALSE;

        if ( !str_cmp( arg, "gold" ) )
        {
            if ( amount < 0 )
            {
                send_to_char("You must enter an amount.\n\r",ch);
                return;
            }

            if ( ch->gold < amount )
            {
                send_to_char("You don't have that much.\n\r",ch);
                return;
            }

            trade->gold[trader] += amount;
	    ch->gold -= amount;
            sprintf(buf,"You add %d gold to the trade.",amount);
            act(buf,ch,NULL,victim,TO_CHAR,FALSE);
            sprintf(buf,"$n adds %d gold to the trade.",amount);
            act(buf,ch,NULL,victim,TO_VICT,FALSE);
            return;
        }
        else
        if ( !str_cmp( arg, "silver" ) )
        {
            if ( amount < 0 )
            {
                send_to_char("You must enter an amount.\n\r",ch);
                return;
            }

            if ( ch->silver < amount )
            {
                send_to_char("You don't have that much.\n\r",ch);
                return;
            }

            trade->silver[trader] += amount;
	    ch->silver -= amount;
            sprintf(buf,"You add %d silver to the trade.",amount);
            act(buf,ch,NULL,victim,TO_CHAR,FALSE);
            sprintf(buf,"$n adds %d silver to the trade.",amount);
            act(buf,ch,NULL,victim,TO_VICT,FALSE);
            return;
        }
        else
        if ( ( obj = get_obj_carry(ch,arg) ) == NULL )
        {
            send_to_char("You're not carrying that.\n\r",ch);
            return;
        }

/*
	if ( !can_drop_obj(ch,obj) )
	{
	    send_to_char("You can't let go of it.\n\r",ch);
	    return;
	}
 */

        if ( !obj_to_trade( obj, trade, trader ) )
	    act("You can't trade $p.",ch,obj,victim,TO_CHAR,FALSE);
	else
	{
            obj_from_char( obj );
            act("You add $p to the trade.",ch,obj,victim,TO_CHAR,FALSE);
            act("$n adds $p to the trade.",ch,obj,victim,TO_VICT,FALSE);
	}
        return;

   case TRADE_REMOVE:
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You are not trading with anybody.\n\r",ch);
            return;
        }

        trader = ( trade->trader[0] == ch ? 0 : 1 );
        victim =   trade->trader[!trader]; 

        /* Always reset approvals */
        trade->approved[0] = FALSE;
        trade->approved[1] = FALSE;

        if ( !str_cmp( arg, "gold" ) )
        {
            if ( amount < 0 )
            {
                send_to_char("You must enter an amount.\n\r",ch);
                return;
            }

            if ( amount > trade->gold[trader] )
            {
                send_to_char("You aren't offering that much.\n\r",ch);
                return;
            }

            trade->gold[trader] -= amount;
            ch->gold += amount;
            sprintf(buf,"You remove %d gold from the trade.",amount);
            act(buf,ch,NULL,victim,TO_CHAR,FALSE);
            sprintf(buf,"$n removes %d gold from the trade.",amount);
            act(buf,ch,NULL,victim,TO_VICT,FALSE);
            return;
        }
        else
        if ( !str_cmp( arg, "silver" ) )
        {
            if ( amount < 0 )
            {
                send_to_char("You must enter an amount.\n\r",ch);
                return;
            }

            if ( amount > trade->silver[trader] )
            {
                send_to_char("You aren't offering that much.\n\r",ch);
                return;
            }

            trade->silver[trader] -= amount;
	    ch->silver += amount;
            sprintf(buf,"You remove %d silver from the trade.",amount);
            act(buf,ch,NULL,victim,TO_CHAR,FALSE);
            sprintf(buf,"$n removes %d silver from the trade.",amount);
            act(buf,ch,NULL,victim,TO_VICT,FALSE);
            return;
        }
        else
        if ( ( obj = get_obj_trade(trade,trader,arg) ) == NULL )
        {
            send_to_char("You aren't offering that.\n\r",ch);
            return;
        }

        /* ok, the obj is what we want to remove */
        obj_from_trade( obj, trade, trader );
        obj_to_char( obj, ch );

        act("You remove $p from the trade.",ch,obj,victim,TO_CHAR,FALSE);
        act("$n removes $p from the trade.",ch,obj,victim,TO_VICT,FALSE);
        break;
      case TRADE_IDENTIFY: 
        if ( ( trade = ch->trade ) == NULL )
        {
            send_to_char("You are not trading with anybody.\n\r",ch);
            return;
        }
        if(ch->gold * 100 + ch->silver < 1000)
        {
            send_to_char("It takes 10 gold to identify a trade item, you don't have enough.\n\r",ch);
            return;
        }
        trader = ( trade->trader[0] == ch ? 0 : 1 );
        /* Identify your items first, same as trade show does it */
        if ( ( obj = get_obj_trade(trade,trader + 2,arg) ) == NULL )
        {
            send_to_char("That item is not in the current trade.\n\r",ch);
            return;
        }
        deduct_cost(ch, 1000);
        send_to_char("You offer 10 gold to the gods to provide you insight on your trade.\n\r",ch);
        spell_identify(gsn_lore, ch->level, ch, obj, TARGET_OBJ);
        break;

    };

    return;
}
