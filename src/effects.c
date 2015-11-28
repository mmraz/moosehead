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
 
static char rcsid[] = "$Id: effects.c,v 1.40 2004/03/27 15:51:41 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"

void acid_effect(void *vo, int level, int dam, int target)
{
  bool permanent = TRUE;

    if (target == TARGET_ROOM) /* nail objects on the floor */
     {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    acid_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
    }

    if (target == TARGET_CHAR)  /* do the effect on a victim */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;
	
	/* let's toast some gear */
	if(!IS_SET(victim->mhs,MHS_GLADIATOR) && 
	   !IS_SET(victim->mhs,MHS_HIGHLANDER))
	{
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    acid_effect(obj,level,dam,TARGET_OBJ);
	}
	}
	return;
    }

    if (target == TARGET_OBJ) /* toast an object */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj,*n_obj;
	/*
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	 */
	int chance;
	char *msg;
  	if(obj->damaged >= 100)
	    return;// Can't destroy an already destroyed object

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	 if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
	    chance -= 5;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{// Chance modifiers based on type
	    default:
		return;
	    case ITEM_CONTAINER:
	    case ITEM_CORPSE_PC:
	    case ITEM_CORPSE_NPC:
	    case ITEM_ARMOR:
	    case ITEM_CLOTHING:
	 	break;
	    case ITEM_STAFF:
	    case ITEM_WAND:
		chance -= 10;
		break;
	    case ITEM_SCROLL:
	    case ITEM_SPELL_PAGE:
		chance += 10;
		break; 
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
	    return;

	obj->damaged = UMIN(obj->damaged + number_range(34, 55), 100);

	switch (obj->item_type)
	{
	    default:
		return;
	    case ITEM_CONTAINER: permanent = FALSE;// Containers are not destroyed
	    case ITEM_CORPSE_PC:
	    case ITEM_CORPSE_NPC:
	  if(obj->damaged >= 100)
			msg = "$p fumes and dissolves.";
		else
			msg = "$p fumes and sizzles.";// Not destroyed
		break;
	    case ITEM_ARMOR:
	    case ITEM_CLOTHING: permanent = FALSE;// Gear is not destroyed
		if(obj->damaged >= 100)
			msg = "$p is corroded into scrap!";
		else
			msg = "$p is pitted and etched.";
	 	break;
	    case ITEM_STAFF:
	    case ITEM_WAND:
	  if(obj->damaged >= 100)
			msg = "$p corrodes and breaks!";
		else
			msg = "$p corrodes slightly.";
		break;
	    case ITEM_SCROLL:
	    case ITEM_SPELL_PAGE:
		if(obj->damaged >= 100)
			msg = "$p is burned into waste!";
		else
			msg = "$p dissolves around its edges.";
		break; 
	}

	if (obj->carried_by != NULL)
	    act(msg,obj->carried_by,obj,NULL,TO_ALL,FALSE);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
	    act(msg,obj->in_room->people,obj,NULL,TO_ALL,FALSE);
	
	if(obj->damaged >= 100)
	{
		if(permanent)
		{
			/* get rid of the object */
			if (obj->contains)  /* dump contents */
			{
			    for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
			    {
				n_obj = t_obj->next_content;
				obj_from_obj(t_obj);
				if (obj->in_room != NULL)
				    obj_to_room(t_obj,obj->in_room);
				else if (obj->carried_by != NULL)
				    obj_to_room(t_obj,obj->carried_by->in_room);
				else
				{
				    extract_obj(t_obj);
				    continue;
				}
		
				acid_effect(t_obj,level/2,dam/2,TARGET_OBJ);
			    }
		 	}
		
			extract_obj(obj);
		}
		else
			remove_bonuses(obj->carried_by, obj);
	}
	return;
  }// End if object

}

void cold_effect(void *vo, int level, int dam, int target)
{
    bool permanent = TRUE;

    if (target == TARGET_ROOM) /* nail objects on the floor */
    {
        ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
        OBJ_DATA *obj, *obj_next;
 
        for (obj = room->contents; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            cold_effect(obj,level,dam,TARGET_OBJ);
        }
        return;
    }

    if (target == TARGET_CHAR) /* whack a character */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next,*segment;
	
        if((segment = get_eq_char(victim, WEAR_HOLD)) != NULL)
	    if (segment->pIndexData->vnum == VNUM_WATER_SEGMENT)
		return;
	
        /* chill touch effect */
	if (!saves_spell(level/4 + dam / 20, victim, DAM_COLD))
	{
	    AFFECT_DATA af;

            act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM,FALSE);
	    act("A chill sinks deep into your bones."
		,victim,NULL,NULL,TO_CHAR,FALSE);
            af.where     = TO_AFFECTS;
            af.type      = skill_lookup("chill touch");
            af.level     = level;
            af.duration  = 6;
            af.location  = APPLY_STR;
            af.modifier  = -1;
            af.bitvector = 0;
            affect_join( victim, &af );
	}

	/* hunger! (warmth sucked out */
	if (!IS_NPC(victim))
	    gain_condition(victim,COND_HUNGER,dam/20);

	/* let's toast some gear */
	if(!IS_SET(victim->mhs,MHS_GLADIATOR) && 
	   !IS_SET(victim->mhs,MHS_HIGHLANDER))
	{
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
 	    if ( is_affected(victim,skill_lookup("frost shield") ) )
		cold_effect(obj,level/2,dam,TARGET_OBJ);
	    else
	        cold_effect(obj,level,dam,TARGET_OBJ);
	}
	}
	return;
   }

   if (target == TARGET_OBJ) /* toast an object */
   {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char *msg;

	if(obj->damaged >= 100)
	    return;// Can't destroy an already destroyed object

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
	    chance -= 5;

 	chance -= obj->level * 2;

	switch(obj->item_type)
	{
	    default:
		return;
	    case ITEM_POTION:
		chance += 25;
		break;
	    case ITEM_DRINK_CON:
		chance += 5;
		break;
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
	    return;

	obj->damaged = UMIN(obj->damaged + number_range(34, 55), 100);

	switch(obj->item_type)
	{
	    default:
		return;
	    case ITEM_POTION:
		if(obj->damaged >= 100)
			msg = "$p freezes and shatters!";
		else
			msg = "$p is surrounded by ice.";
		break;
	    case ITEM_DRINK_CON: permanent = FALSE;
		if(obj->damaged >= 100)
			msg = "$p freezes solid!";
		else
			msg = "$p is surrounded by ice.";
		break;
	}

	if (obj->carried_by != NULL)
	    act(msg,obj->carried_by,obj,NULL,TO_ALL,FALSE);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
	    act(msg,obj->in_room->people,obj,NULL,TO_ALL,FALSE);

	if(obj->damaged >= 100)
	{
		if(permanent)
			extract_obj(obj);
		else
			remove_bonuses(obj->carried_by, obj);
	}
	return;
    }

}
  
void trap_effect( CHAR_DATA *ch,     AFFECT_DATA *paf )
{
    CHAR_DATA *victim, *vch_next;
    int dam;
    int save;

    if( is_affected(ch,skill_lookup("wraithform")) )
	return;

    /*  Can't be trapped if non clanned, a mob or if you have the 
	trap skill above 1%
     */
    if ( IS_NPC(ch) || !is_clan(ch) || ( get_skill(ch, gsn_trap) > 1 ) )
    {
	return;
    }

    if (IS_SET(ch->mhs,MHS_HIGHLANDER))
      return;

    save = ( paf->level - get_curr_stat(ch,STAT_DEX)/4 );        
    if ( paf->level > ch->level + 8 )
    /* Circumvent pkill?  NOT! */
	return;
    /* let's take it the other way too*/
    if ( paf->level < ch->level - 8 )
    {
      return;
    }

 
    switch( paf->location )
    {
    case TRAP_CLAW: /* damage */

     /* dam = number_range(ch->level,UMAX(ch->level,get_skill(ch,gsn_trap)));*/
     dam = dice( paf->level /5, paf->modifier  );
     raffect_remove(ch->in_room,paf);
     act("You tripped a claw trap!",ch,NULL,NULL,TO_CHAR,FALSE);
     act("$n trips a claw trap!",ch,NULL,NULL,TO_ROOM,FALSE);
     /* want these to work just a little more */
     if ( number_percent() > ( save + 10 ) )
     {
	 act("You dodged the trap.",ch,NULL,NULL,TO_CHAR,FALSE);
	 act("$n dodges the trap.",ch,NULL,NULL,TO_ROOM,FALSE);
	 return;
     }
     damage(ch,ch,dam,0,DAM_BASH,FALSE,FALSE);  
     break;
    case TRAP_SNARE: /* hold alter trap for char, and swap it */
     /* want these to work a little less often , -10 on save */
     if ( number_percent() > ( save - 10 ) )
     {
	 act("You dodged a snare trap!",ch,NULL,NULL,TO_CHAR,FALSE);
	 act("$n dodges a snare trap.",ch,NULL,NULL,TO_ROOM,FALSE);
	 raffect_remove(ch->in_room,paf);
	 return;
     }
     paf->where = TO_AFFECTS;
     paf->duration =  1+UMAX(0, paf->level - 51 );
     paf->location = 0;
     affect_to_char(ch,paf);
     raffect_remove(ch->in_room,paf);
     ch->position = POS_RESTING;
     ch->pcdata->quit_time = 2;
     act("You are caught in a snare trap!",ch,NULL,NULL,TO_CHAR,FALSE);
     act("$n is caught in a snare trap.",ch,NULL,NULL,TO_ROOM,FALSE);
     break;
    case TRAP_ALARM: /* just send an alarm out */
     raffect_remove(ch->in_room,paf);
     /* want these to wrok often, so add 30% to save number */
     if ( number_percent() > ( save + 30 ) )
     {
	act("You dodged an alarm trap!",ch,NULL,NULL,TO_CHAR,FALSE);
	act("$n dodges a alarm trap.",ch,NULL,NULL,TO_ROOM,FALSE);
     }
     else
     {
       act("You tripped an alarm trap!",ch,NULL,NULL,TO_CHAR,FALSE);
       act("$n trips an alarm trap!",ch,NULL,NULL,TO_ROOM,FALSE);
       for ( victim = char_list ; victim != NULL ; victim = vch_next )
       {
	 vch_next = victim->next;
	 if ( IS_NPC(victim) )
            continue;
         if ( victim->in_room == NULL )
	    continue;
         if (victim == ch)
	    continue;
	 if ( victim->in_room->area == ch->in_room->area && IS_AWAKE(victim) )
    send_to_char("You hear a noisy clatter in the distance.\n\r",victim);
       }
     }

     break;
    default:
     bug("Trap_effect: invalid trap",0);
     break;
   }
    return;
}

void holy_effect( CHAR_DATA *victim, int level, int align, CHAR_DATA *ch ) 
{
    AFFECT_DATA af;

    if ( !IS_AFFECTED(victim,AFF_BLIND) &&
	 !saves_spell(level/5,victim,DAM_OTHER) )
    {
        if ( align >= 350 )
	{
     act("$n is blinded by the light.",victim,NULL,NULL,TO_ROOM,FALSE);
     act("You are blinded by the light.",victim,NULL,NULL,TO_CHAR,FALSE);
	}
	else
	{
    act("$n is surrounded by unholy darkness.",victim,NULL,NULL,TO_ROOM,FALSE);
    act("You are surrouned by unholy darkness.",victim,NULL,NULL,TO_CHAR,FALSE);
	}

	af.where 		= TO_AFFECTS;
	af.type 		= skill_lookup("blindness");
	af.level 		= level / 2;
	af.duration 		= level / 10;
	af.location		= APPLY_HITROLL; 
	af.modifier		= -4;
	af.bitvector		= AFF_BLIND;
        if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
           af.duration  = 1;

	affect_to_char(victim,&af); 
    }

    return;
}



void fire_effect(void *vo, int level, int dam, int target)
{
    bool permanent = TRUE;

    if (target == TARGET_ROOM)  /* nail objects on the floor */
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    fire_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
    }
 
    if (target == TARGET_CHAR)   /* do the effect on a victim */
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next, *segment;

        if((segment = get_eq_char(victim, WEAR_HOLD)) != NULL)
	    if (segment->pIndexData->vnum == VNUM_FIRE_SEGMENT)
		return;

 	if (!is_affected(victim,skill_lookup("flame shield") ) )
        { 
           /* chance of blindness */
           if (!IS_AFFECTED(victim,AFF_BLIND)
               &&  !saves_spell(level / 4 + dam / 20, victim,DAM_FIRE))
           {
              AFFECT_DATA af;
              act("$n is blinded by smoke!",victim,NULL,NULL,TO_ROOM,FALSE);
              act("Your eyes tear up from smoke...you can't see a thing!",
                   victim,NULL,NULL,TO_CHAR,FALSE);
	 
              af.where        = TO_AFFECTS;
              af.type         = skill_lookup("fire breath");
              af.level        = level;
              af.duration     = 0; /*number_range(0,level/10);*/
              af.location     = APPLY_HITROLL;
              af.modifier     = -4;
              af.bitvector    = AFF_BLIND;
 
              affect_to_char(victim,&af);
           }

           /* getting thirsty */
           if (!IS_NPC(victim))
              gain_condition(victim,COND_THIRST,dam/20);
	}

	/* let's toast some gear! */
	if(!IS_SET(victim->mhs,MHS_GLADIATOR) && 
	   !IS_SET(victim->mhs,MHS_HIGHLANDER))
	{
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;

 	    if ( is_affected(victim,skill_lookup("flame shield") ) )
		fire_effect(obj,level/2,dam,TARGET_OBJ);
	    else
	        fire_effect(obj,level,dam,TARGET_OBJ);
        }
	}
	return;
    }

    if (target == TARGET_OBJ)  /* toast an object */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	OBJ_DATA *t_obj,*n_obj;
	int chance;
	char *msg;

	if(obj->damaged >= 100)
	    return;// Can't destroy an already destroyed object
    	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
        ||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
            return;
 
        chance = level / 4 + dam / 10;
 
        if (chance > 25)
            chance = (chance - 25) / 2 + 25;
        if (chance > 50)
            chance = (chance - 50) / 2 + 50;

        if (IS_OBJ_STAT(obj,ITEM_BLESS))
            chance -= 5;
        chance -= obj->level * 2;

        switch ( obj->item_type )
        {
        default:             
	    return;
        case ITEM_CONTAINER:
            break;
        case ITEM_POTION:
            chance += 25;
            break;
        case ITEM_SCROLL:
        case ITEM_SPELL_PAGE:
            chance += 50;
            break;
        case ITEM_STAFF:
            chance += 10;
            break;
        case ITEM_WAND:
        case ITEM_FOOD:
        case ITEM_PILL:
            break;
        }

        chance = URANGE(5,chance,95);

        if (number_percent() > chance)
            return;

				obj->damaged = UMIN(obj->damaged + number_range(34, 55), 100);
 
        switch ( obj->item_type )
        {
        default:             
	    return;
        case ITEM_CONTAINER: permanent = FALSE;
        	if(obj->damaged >= 100)
            msg = "$p ignites and burns!";
        	else
            msg = "$p is singed by the fire.";
            break;
        case ITEM_POTION:
        	if(obj->damaged >= 100)
            msg = "$p bubbles and boils!";
          else
          	msg = "$p bubbles a little.";
            break;
        case ITEM_SCROLL:
        case ITEM_SPELL_PAGE:
        	if(obj->damaged >= 100)
            msg = "$p crackles and burns!";
          else
          	msg = "$p blackens around the edges.";
            break;
        case ITEM_STAFF:
        	if(obj->damaged >= 100)
            msg = "$p smokes and chars!";
          else
          	msg = "$p smokes a bit.";
            break;
        case ITEM_WAND:
        	if(obj->damaged >= 100)
            msg = "$p sparks and blows in half!";
          else
          	msg = "$p sparks and smokes.";
            break;
        case ITEM_FOOD:
        	if(obj->damaged >= 100)
            msg = "$p blackens and crisps!";
          else
          	msg = "$p is slightly crisped.";
            break;
        case ITEM_PILL:
        	if(obj->damaged >= 100)
            msg = "$p melts and drips!";
          else
          	msg = "$p melts around the edges.";
            break;
        }

	if (obj->carried_by != NULL)
		act( msg, obj->carried_by, obj, NULL, TO_ALL ,FALSE);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
		act(msg,obj->in_room->people,obj,NULL,TO_ALL,FALSE);
		
		if(obj->damaged >= 100)
		{
			if(permanent)
			{
        if (obj->contains)
        {
            /* dump the contents */
 
            for (t_obj = obj->contains; t_obj != NULL; t_obj = n_obj)
            {
                n_obj = t_obj->next_content;
                obj_from_obj(t_obj);
		if (obj->in_room != NULL)
      obj_to_room(t_obj,obj->in_room);
		else if (obj->carried_by != NULL)
		    obj_to_room(t_obj,obj->carried_by->in_room);
		else
		{
		    extract_obj(t_obj);
		    continue;
		}
		fire_effect(t_obj,level/2,dam/2,TARGET_OBJ);
            }
        }

        extract_obj( obj );
      }
      else
      	remove_bonuses(obj->carried_by, obj);
    }

	return;
    }// End if object

}

void poison_effect(void *vo,int level, int dam, int target)
{
    if (target == TARGET_ROOM)  /* nail objects on the floor */
    {
        ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
        OBJ_DATA *obj, *obj_next;
 
        for (obj = room->contents; obj != NULL; obj = obj_next)
        {
            obj_next = obj->next_content;
            poison_effect(obj,level,dam,TARGET_OBJ);
        }
        return;
    }
 
    if (target == TARGET_CHAR)   /* do the effect on a victim */
    {
        CHAR_DATA *victim = (CHAR_DATA *) vo;
        OBJ_DATA *obj, *obj_next;

	/* chance of poisoning */
        if (!saves_spell(level / 4 + dam / 20,victim,DAM_POISON))
        {
	    AFFECT_DATA af;

            send_to_char("You feel poison coursing through your veins.\n\r",
                victim);
            act("$n looks very ill.",victim,NULL,NULL,TO_ROOM,FALSE);

            af.where     = TO_AFFECTS;
            af.type      = gsn_poison;
            af.level     = level;
            af.duration  = level / 2;
            af.location  = APPLY_STR;
            af.modifier  = -1;
            af.bitvector = AFF_POISON;
            affect_join( victim, &af );
        }

	/* equipment */
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    poison_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
    }

    if (target == TARGET_OBJ)  /* do some poisoning */
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
  	||  IS_OBJ_STAT(obj,ITEM_BLESS)
	||  number_range(0,4) == 0)
	    return;

	chance = level / 4 + dam / 10;
	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) / 2 + 50;

	chance -= obj->level * 2;

	switch (obj->item_type)
	{
	    default:
		return;
	    case ITEM_FOOD:
		break;
	    case ITEM_DRINK_CON:
		if (obj->value[0] == obj->value[1])
		    return;
		break;
	}

	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
	    return;

	obj->value[3] = 1;
	return;
    }
}


void shock_effect(void *vo,int level, int dam, int target)
{
    bool permanent = TRUE;

    if (target == TARGET_ROOM)
    {
	ROOM_INDEX_DATA *room = (ROOM_INDEX_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	for (obj = room->contents; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
	    shock_effect(obj,level,dam,TARGET_OBJ);
	}
	return;
    }

    if (target == TARGET_CHAR)
    {
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	OBJ_DATA *obj, *obj_next;

	/* daze and confused? */
	if (!saves_spell(level/4 + dam/20,victim,DAM_LIGHTNING))
	{
	    send_to_char("Your muscles stop responding.\n\r",victim);
	    DAZE_STATE(victim,UMAX(12,level/4 + dam/20));
	}

	/* toast some gear */
	if(!IS_SET(victim->mhs,MHS_GLADIATOR) && 
	   !IS_SET(victim->mhs,MHS_HIGHLANDER))
	{
	for (obj = victim->carrying; obj != NULL; obj = obj_next)
	{
	    obj_next = obj->next_content;
 	    if ( is_affected(victim,skill_lookup("electric shield") ) )
		shock_effect(obj,level/2,dam,TARGET_OBJ);
	    else
	        shock_effect(obj,level,dam,TARGET_OBJ);
	}
	}
	return;
    }

    if (target == TARGET_OBJ)
    {
	OBJ_DATA *obj = (OBJ_DATA *) vo;
	int chance;
	char *msg;
	if(obj->damaged >= 100)
	    return;// Can't destroy an already destroyed object

	if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF)
	||  IS_OBJ_STAT(obj,ITEM_NOPURGE)
	||  number_range(0,4) == 0)
	    return;

	chance = level / 4 + dam / 10;

	if (chance > 25)
	    chance = (chance - 25) / 2 + 25;
	if (chance > 50)
	    chance = (chance - 50) /2 + 50;

	if (IS_OBJ_STAT(obj,ITEM_BLESS))
	    chance -= 5;

 	chance -= obj->level * 2;

	switch(obj->item_type)
	{
	    default:
		return;
	   case ITEM_WAND:
	   case ITEM_STAFF:
		chance += 10;
		break;
	   case ITEM_JEWELRY:
		chance -= 10;
		break;
	}
	
	chance = URANGE(5,chance,95);

	if (number_percent() > chance)
	    return;

	obj->damaged = UMIN(obj->damaged + number_range(34, 55), 100);

	switch(obj->item_type)
	{
	    default:
		return;
	   case ITEM_WAND:
	   case ITEM_STAFF:
	  if(obj->damaged >= 100)
			msg = "$p overloads and explodes!";
		else
			msg = "Electricity crackles over $p.";
		break;
	   case ITEM_JEWELRY:permanent = FALSE;
	  if(obj->damaged >= 100)
			msg = "$p is fused into a worthless lump.";
		else
			msg = "$p melts slightly.";
		break;
	}

	if (obj->carried_by != NULL)
	    act(msg,obj->carried_by,obj,NULL,TO_ALL,FALSE);
	else if (obj->in_room != NULL && obj->in_room->people != NULL)
	    act(msg,obj->in_room->people,obj,NULL,TO_ALL,FALSE);

	if(obj->damaged >= 100)
	{
		if(permanent)
			extract_obj(obj);
		else
			remove_bonuses(obj->carried_by, obj);
	}

	return;
    }

}

