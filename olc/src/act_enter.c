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

static char rcsid[] = "$Id: act_enter.c,v 1.21 2003/05/26 16:29:01 boogums Exp $";
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

/* command procedures needed */
DECLARE_DO_FUN(do_look		);
DECLARE_DO_FUN(do_stand		);

/* random room generation procedure */
ROOM_INDEX_DATA  *get_random_room(CHAR_DATA *ch)
{
    ROOM_INDEX_DATA *room;

    for ( ; ; )
    {
        room = get_room_index( number_range( 0, 65535 ) );
        if ( room != NULL )
        if ( can_see_room(ch,room)
	&&   is_room_clan(ch,room)
	&&   !room_is_private(ch,room)
        &&   !IS_SET(room->room_flags, ROOM_PRIVATE)
        &&   !IS_SET(room->room_flags, ROOM_SOLITARY) 
	&&   !IS_SET(room->room_flags, ROOM_SAFE) 
        &&   !IS_SET(room->room_flags,ROOM_LAW)
        &&   !room->area->under_develop
	&&   !room->area->no_transport )
            break;
    }

    return room;
}

ROOM_INDEX_DATA *get_random_room_obj(void)
{
	ROOM_INDEX_DATA *room;
        
	for ( ; ; )
  	{
	  room = get_room_index( number_range(0, 65535 ));
          if(room != NULL)
	  if(   !IS_SET(room->room_flags, ROOM_PRIVATE)
	     && !IS_SET(room->room_flags, ROOM_SOLITARY)
	     && !IS_SET(room->room_flags, ROOM_LAW)
	     && !IS_SET(room->room_flags, ROOM_SAFE)
	     && !room->area->under_develop
	     && !room->area->no_transport )
		break;
        }
        
        return room;
}


/*MM entering Clan halls*/
void do_enter( CHAR_DATA *ch, char *argument)
{    
    ROOM_INDEX_DATA *location; 

    if ( ch->fighting != NULL ) return;

    /* nifty portal stuff */
    if (argument[0] != '\0' && str_prefix(argument,"clan"))
    {
        ROOM_INDEX_DATA *old_room;
	OBJ_DATA *portal;
	CHAR_DATA *fch, *fch_next;

        old_room = ch->in_room;

	portal = get_obj_list( ch, argument,  ch->in_room->contents );
	
	if (portal == NULL)
	{
	    send_to_char("You don't see that here.\n\r",ch);
	    return;
	}
        
	if ( !IS_NPC(ch) && ch->in_room->clan && IS_SET(ch->pcdata->clan_flags, CLAN_NO_PORTALS))
	{
	send_to_char("You have been forbidden to use the clan portals.\n\r", ch);
	return;
	}

	if (portal->item_type != ITEM_PORTAL 
        ||  (IS_SET(portal->value[1],EX_CLOSED) && !IS_TRUSTED(ch,ANGEL)))
	{
	    send_to_char("You can't seem to find a way in.\n\r",ch);
	    return;
	}

	if (!IS_TRUSTED(ch,ANGEL) && !IS_SET(portal->value[2],GATE_NOCURSE)
	//&&  (IS_AFFECTED(ch,AFF_CURSE)  || IS_SET(ch->act,PLR_DWEEB || is_affected(ch,gsn_morph) )
        && (  ( IS_AFFECTED(ch,AFF_CURSE)  || IS_SET(ch->act,PLR_DWEEB) )
	||   IS_SET(old_room->room_flags,ROOM_NO_RECALL)))
	{
	    send_to_char("Something prevents you from leaving...\n\r",ch);
	    return;
	}

	if (IS_SET(portal->value[2],GATE_RANDOM) || portal->value[3] == -1)
	{
	    location = get_random_room(ch);
	    portal->value[3] = location->vnum; /* for record keeping :) */
	}
	else if (IS_SET(portal->value[2],GATE_BUGGY) && (number_percent() < 5))
	    location = get_random_room(ch);
	else
	    location = get_room_index(portal->value[3]);

	if (location == NULL
	||  location == old_room
	||  !can_see_room(ch,location) 
	||  !is_room_clan(ch,location)
	||  (room_is_private(ch,location) && !IS_TRUSTED(ch,IMPLEMENTOR)))
	{
	   act("$p doesn't seem to go anywhere.",ch,portal,NULL,TO_CHAR,FALSE);
	   return;
	}

        if (IS_NPC(ch) && IS_SET(ch->act,ACT_AGGRESSIVE)
        &&  IS_SET(location->room_flags,ROOM_LAW))
        {
            send_to_char("Something prevents you from leaving...\n\r",ch);
            return;
        }

	if ( ch->invis_level < 51 )
	    act("$n steps into $p.",ch,portal,NULL,TO_ROOM,FALSE);
	
        if (IS_SET(ch->mhs,MHS_HIGHLANDER))
	{
           for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
           {
              fch_next = fch->next_in_room;
	      if (IS_SET(fch->mhs,MHS_HIGHLANDER) && fch != ch)
	      {
	         send_to_char("The tingle in your neck stops and the presence of the other Highlander is gone.\n\r",fch);
	      }
	   }
	}

	if (IS_SET(portal->value[2],GATE_NORMAL_EXIT))
	    act("You enter $p.",ch,portal,NULL,TO_CHAR,FALSE);
	else
	    act("You walk through $p and find yourself somewhere else...",
	        ch,portal,NULL,TO_CHAR,FALSE); 

	char_from_room(ch);
	char_to_room(ch, location);
	if ( is_mounted(ch) )
	{
	    char_from_room( ch->riding );
	    char_to_room( ch->riding, location );
	}
	else
	if ( ch->passenger != NULL )
	{
	    char_from_room( ch->passenger );
	    char_to_room( ch->passenger, location );
 	}

	if (IS_SET(portal->value[2],GATE_GOWITH)) /* take the gate along */
	{
	    obj_from_room(portal);
	    obj_to_room(portal,location);
	}


        if (IS_SET(ch->mhs,MHS_HIGHLANDER))
	{
           for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
           {
              fch_next = fch->next_in_room;
	      if (IS_SET(fch->mhs,MHS_HIGHLANDER) && fch != ch)
	      {
	         send_to_char("Your neck tingles as you feel the presence of another Highlander.\n\r",fch);
	      }
	   }
	}

	if (ch->invis_level < 51 )
	{
	if (IS_SET(portal->value[2],GATE_NORMAL_EXIT))
	    act("$n has arrived.",ch,portal,NULL,TO_ROOM,FALSE);
	else
	    act("$n has arrived through $p.",ch,portal,NULL,TO_ROOM,FALSE);
	}

	do_look(ch,"auto");

	/* charges */
	if (portal->value[0] > 0)
	{
	    portal->value[0]--;
	    if (portal->value[0] == 0)
		portal->value[0] = -1;
	}

 	if (portal != NULL && portal->value[0] == -1)
	{
	    act("$p fades out of existence.",ch,portal,NULL,TO_CHAR,FALSE);
	    if (ch->in_room == old_room)
		act("$p fades out of existence.",ch,portal,NULL,TO_ROOM,FALSE);
	    else if (old_room->people != NULL)
	    {
		act("$p fades out of existence.", 
		    old_room->people,portal,NULL,TO_CHAR,FALSE);
		act("$p fades out of existence.",
		    old_room->people,portal,NULL,TO_ROOM,FALSE);
	    }
	    extract_obj(portal);
            portal = NULL;
	}

	/* protect against circular follows */
	if (old_room == location)
	    return;

    	for ( fch = old_room->people; fch != NULL; fch = fch_next )
    	{
            fch_next = fch->next_in_room;

            if (portal == NULL || portal->value[0] == -1) 
	    /* no following through dead portals */
                continue;
 
            if ( fch->master == ch && IS_AFFECTED(fch,AFF_CHARM)
            &&   fch->position < POS_STANDING)
            	do_stand(fch,"");

            if ( fch->master == ch && fch->position == POS_STANDING)
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
 
            	act( "You follow $N.", fch, NULL, ch, TO_CHAR,FALSE );
		do_enter(fch,argument);
            }
    	}
	return;
    }

    send_to_char("Nope, can't do it.\n\r",ch);
    return;
}
