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

static char rcsid[] = "$Id: mag2.c,v 1.438 2005/02/01 03:54:37 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "merc.h"
#include "magic.h"
#include "tables.h"
#include "gladiator.h"
#include "lookup.h"

DECLARE_DO_FUN(action_wraithform);
DECLARE_DO_FUN(action_zealot_convert);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_scan);
DECLARE_DO_FUN(do_morph);
DECLARE_DO_FUN(do_shapemorph);
DECLARE_DO_FUN(do_shapeshift);

extern char *target_name;

int find_door args( ( CHAR_DATA *ch, char *arg ) );
bool  has_key   args( ( CHAR_DATA *ch, int key ) );
extern sh_int rev_dir [];
void  remove_all_objs  args( (CHAR_DATA *ch, bool verbose) );
void  shapeshift_remove args ((CHAR_DATA *ch));
void  wear_obj  args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );


void spell_restore_mana(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
   CHAR_DATA *victim = (CHAR_DATA *) vo;
   int restore = 0;

   restore = dice( level , 2 );
   victim->mana = UMIN( victim->mana + restore, victim->max_mana );
   act("You feel energized.",victim,NULL,NULL,TO_CHAR,FALSE);
   act("$n appears energized.",victim,NULL,NULL,TO_ROOM,FALSE);
   return;
}

void spell_speed(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(ch, skill_lookup("arcantic alacrity") ) )
    {
        send_to_char("Don't be a fucknut.  Bug abuse is bad, mmmkay?\n\r",ch);
        return;
    }

    if ( IS_AFFECTED( victim, AFF_HASTE ) )
    {
	if ( victim == ch )
		send_to_char("You are already moving as fast as you can.\n\r",ch);
	else
		act("$N is already hasted.",ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }
    if ( is_affected( victim, sn ) )
    {
	send_to_char("Your motions begin to speed up.\n\r",victim);
	if ( victim != ch )
	  act("$N's motions begin to speed up.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,level/17,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level/17;
    af.modifier 	= 0;
    af.location		= 0;
    af.bitvector	= 0; 
    affect_to_char( victim, &af );

    send_to_char("Your motions begin to speed up.\n\r",victim);
    if ( victim != ch )
	act("$N's motions begin to speed up.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}
void spell_cone_of_silence(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{

    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *stone;
    AFFECT_DATA af;
    int skill;
    int currentLevel;

    if ((skill = get_skill(ch,gsn_cone_of_silence)) < 2 ) 
    {
      send_to_char("You try to silence the voices in your head.\r\n",ch);
      return;
    }
    stone = get_eq_char(ch,WEAR_HOLD);
    if (stone == NULL || stone->pIndexData->vnum != VNUM_SPELLCOMP_SILENCE  )
    {
        send_to_char("You lack the proper component to cast this spell.\n\r",ch);
        return;
    }
    if ( is_affected(victim, skill_lookup("cone of silence") ) )
    {
        send_to_char("How much more silence do you want to have?\n\r",ch);
        return;
    }
    if ( victim == ch )
    {
      send_to_char("If you want to be quiet, don't say anything.\r\n",ch);
      return;
    }
    currentLevel = ch->level;
    if ( number_range(1,10) == 4 )
    {
      send_to_char("A ball gag flares {Wbrightly{x and {YVANISHES{x!!!",ch);
      extract_obj(stone);
    }

    if ( number_percent() > get_skill(ch,gsn_cone_of_silence) - number_range(10,30) )
    {
        act("$n asks $N for some quiet and fails!", ch, NULL, victim, TO_ROOM, FALSE);
        act("$n chants and mumbles something about quiet.  You feel a brief tingling senstaion.",ch,NULL,victim,TO_VICT,FALSE);
        act("You failed to wrap $N in a Cone of Silence.", ch, NULL, victim, TO_CHAR, FALSE);
       //failed
       check_improve(ch,gsn_cone_of_silence,FALSE,8);
       if ( number_percent() < number_range(1,10) )
       {
         send_to_char("{YBACKLASH!!! Your cone of silence hits YOU!!!{x\r\n",ch);
         act("$n is enveloped in a cone of silence.",ch,NULL,victim,TO_ROOM,FALSE);
	 act("$n is enveloped in a cone of silence.",ch,NULL,victim,TO_CHAR,FALSE);
         af.where            = TO_AFFECTS;
         af.type             = sn;
         af.level            = level;
         af.duration         = number_range(2,4);
         af.modifier         = 0;
         af.location         = 0;
         af.bitvector        = 0;
         affect_to_char( ch, &af );
         send_to_char("The silence is deafening to your ears.\n\r",ch);
       } 
       return;
    }
    check_improve(ch,gsn_cone_of_silence,TRUE,6);

    if ( !saves_spell(level,victim,DAM_MENTAL) )
    {
      
      af.where            = TO_AFFECTS;
      af.type             = sn;
      af.level            = level;
      af.duration         = number_range(1,5);
      af.modifier         = 0;
      af.location         = 0;
      af.bitvector        = 0;
      affect_to_char( victim, &af );
      send_to_char("The silence is deafening to your ears.\n\r",victim);
      if ( victim != ch )
      {
        act("$N looks confused and shrouded by silence.",ch,NULL,victim,TO_ROOM,FALSE);
	act("$N looks confused and shrouded by silence.",ch,NULL,victim,TO_CHAR,FALSE);
      }
    }
    return;
}
void spell_creeping_doom(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{

    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && !IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The spiders swarm all over you!\n\r", ch);
    }

    if (victim != ch)
    {
        act("$n unleashes a stream of biting, venomous spiders at $N!", ch,             NULL, victim, TO_ROOM, FALSE);
        act("$n unleashes a stream of biting, venomous spiders at you!",ch,             NULL, victim, TO_VICT, FALSE);
        send_to_char("You unleash a stream of venemous spiders at your foe!\n\r", ch);
    }

    dam = dice(level, 10);

    /* Damage gets better as caster alignment gets worse. */

    if (ch->alignment <= -500)
        dam += dam * .05;
    if (ch->alignment <= -750)
        dam += dam * .08;
    if (ch->alignment <= -950)
        dam += dam * .1;

    if (saves_spell(level, victim, DAM_POISON))
        dam /= 2;

    /* Now poison the poor spiderbitten fool */
    spell_poison(gsn_poison, 3*level/4, ch, (void *) victim,                        TARGET_CHAR);

    damage( ch, victim, dam, sn, DAM_POISON, TRUE, TRUE);

} 

void spell_magic_resistance(int sn, int level,CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("You are protected from magic.\n\r",victim);
	if ( victim != ch )
	  act("$N is protected from magic.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,24+level/4,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level/10;
    af.modifier		= 0; 
    af.location		= 0;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("You are protected from magic.\n\r",victim);
    if ( victim != ch )
	act("$N is protected from magic.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}

void spell_vision(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("You are bestowed with the gift of vision.\n\r",victim);
	if ( victim != ch )
	  act("$N is betstowed with the gift of vision.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,25,level);
	return;
    }

    af.where			= TO_AFFECTS;
    af.type		 	= sn;
    af.level			= level;
    af.duration			= 25;
    af.modifier			= 0;
    af.location			= 0;
    af.bitvector		= AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    af.bitvector		= AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    af.bitvector		= AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );

    send_to_char("You are bestowed with the gift of vision.\n\r",victim);
    if ( victim != ch )
	act("$N is betstowed with the gift of vision.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_accuracy(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("Your accuracy is vastly improved.\n\r",victim);
	if ( victim != ch )
	  act("$N is able to hit more often.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,1+level/10,level);
	return;
    }

    af.where	= TO_AFFECTS;
    af.type	= sn;
    af.level	= level;
    af.duration	= level/10 +1 ;
    af.modifier	= victim->hitroll / 2;
    af.location = APPLY_HITROLL;
    af.bitvector = 0;

    affect_to_char( victim, &af );
    send_to_char("Your accuracy is vastly improved.\n\r",victim);
    if ( victim != ch )
	act("$N is able to hit more often.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

/* Attempt at call mount spell for knight kit started 23-AUG-00 */
void spell_call_mount(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{ /* start curley brace for spell_call_mount */
   CHAR_DATA *victim;
   int t,count;


/* Not going to allow highlanders to have mounts 
   if (IS_SET(ch->mhs,MHS_HIGHLANDER))
     {
     send_to_char("Honorable combat is one on one. Highlanders can not ride warhorses.\n\r",ch);
     return;
     }
   if(IS_SET(ch->mhs,MHS_GLADIATOR))
     {
     send_to_char("Gladiators can not ride warhorses.\n\r",ch);
     return;
     }
     */

/* This next part looks like it checks to see if they already have a warhorse */
   count =0;
   for ( victim = char_list; victim != NULL; victim = victim->next )
   {
      if(!IS_NPC(victim))
        continue;
      if ( (is_same_group( victim, ch ) && victim->pIndexData->vnum == MOB_VNUM_WARHORSE /*IS_SET(victim->mhs,MHS_WARHORSE)*/)
      || (is_same_group(victim->master,ch) && victim->pIndexData->vnum == MOB_VNUM_WARHORSE /*IS_SET(victim->mhs,MHS_WARHORSE)*/))
         {
            count++;
         }
   }
      if ( count >= 1 && !IS_IMMORTAL(ch))
         {
         send_to_char("Just how many horses can you ride?  One's plenty.\n\r",ch);
	 return;
         }
      victim = create_mobile(get_mob_index(MOB_VNUM_WARHORSE));
      act("The neighing of a horse gets your attention.",ch,NULL,victim,TO_ROOM,FALSE);
      act("You call forth a magestic warhorse.",ch,NULL, victim,TO_CHAR,FALSE);

      char_to_room (victim,ch->in_room);
      victim->size = UMAX(pc_race_table[ch->race].size + 1,SIZE_GIANT);
      victim->level = level * 3/4;

      for (t = 0; t < 3; t++)
      victim->armor[t] = interpolate(victim->level,50,-50);
      victim->armor[3] = interpolate(victim->level,0,-50);
      /* hmmm, this is weird
      victim->max_hit = level * 2 + number_range(level * level / 9, level * level/5 );
      victim->max_hit = UMAX( victim->max_hit, ch->max_hit * 2 / 4);
      */
      victim->hit = level * 13;
      victim->max_hit = level * 13;

      victim->max_mana = 100;
      victim->mana = 100;
      /*06SEP00 - Taken out by me to try somthing different
      victim->max_move = level * 5 + number_range(level * level / 9, level * level/5 );
      victim->max_move = UMAX( victim->max_move,ch-max_move * 3 / 4);
      */
      victim->move = level * 9;
      victim->max_move = level * 9; 

      victim->damage[DICE_NUMBER] = victim->level/5+1;
      victim->damage[DICE_TYPE]   = 5;
      victim->hitroll = victim->level * 3 / 5;
      victim->damroll = victim->level * 3 / 5;
      SET_BIT (victim->mhs,MHS_WARHORSE); /* just to make sure */
      add_follower( victim, ch );
      //victim->leader = ch;
      add_to_group(victim, ch);
/*make that monkey follow the player */ 
      ch->pet = victim; 

      SET_BIT(victim->form,FORM_INSTANT_DECAY);
      SET_BIT(victim->affected_by,AFF_CHARM); /* quick-charm */
      /* Timer modified 28AUG00 by Boogums */
      victim->life_timer = ch->level*2;

      return;
} /* there goes the end curley brace for spell_call_mount */

void spell_summon_elemental(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
   CHAR_DATA *victim;
   AFFECT_DATA af;
   OBJ_DATA *segment;
   int t,count, number;

   number = number_percent(); 

   if ( is_affected(ch,skill_lookup("wound transfer")) )
	return;

   if (IS_SET(ch->mhs,MHS_HIGHLANDER))
   {
      send_to_char("Honorable combat is one on one. Highlanders can not summon elementals.\n\r",ch);
      return;
   }

   if(IS_SET(ch->mhs,MHS_GLADIATOR))
   {
      send_to_char("Gladiators can not summon elementals.\n\r",ch); 
      return;
   }


   count =0;
   for ( victim = char_list; victim != NULL; victim = victim->next )
      if ( (is_same_group( victim, ch ) && IS_SET(victim->mhs,MHS_ELEMENTAL))
   || (is_same_group(victim->master,ch) && IS_SET(victim->mhs,MHS_ELEMENTAL)))
      count++;

   if ( count >= 1 && !IS_IMMORTAL(ch))
   {
   send_to_char("You are already controlling as many elementals as you can handle\n\r",ch);
   return;
   }
      
   if((segment = get_eq_char(ch, WEAR_HOLD)) != NULL)
   switch(segment->pIndexData->vnum)
   {
   case VNUM_FIRE_SEGMENT:
	level += 5;
	number = 76;
	break;
   case VNUM_WATER_SEGMENT:
	level += 5;
	number = 51;
	break;
   default: 
     number = number_percent();
     break;
   }

   if (number < 3)
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_SPIRIT));
act("A shining hole opens to the Elemental Plane of Spirit.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Spirit.",
	ch,NULL, victim,TO_CHAR,FALSE);
      if ( !(IS_SET(ch->res_flags, RES_MAGIC)))
      {
         af.where     = TO_RESIST;
         af.type      = sn;
         af.level     = level;
         af.duration  = level / 5;
	 af.location  = APPLY_NONE;
         af.modifier  = 0;
         af.bitvector = RES_MAGIC;
         affect_to_char( ch, &af );
      }
   }
   else if (number < 5)
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_ENERGY));
act("A shining hole opens to the Elemental Plane of Energy.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Energy.",
	ch,NULL, victim,TO_CHAR,FALSE);

      if ( !(IS_SET(ch->res_flags, RES_WEAPON)))
      {
         af.where     = TO_RESIST;
         af.type      = sn;
         af.level     = level;
         af.duration  = level / 5;
         af.location  = APPLY_NONE;
	 af.modifier  = 0;
         af.bitvector = RES_WEAPON;
	 affect_to_char( ch, &af );
      }
   } 
   else if (number < 25)
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_AIR));
act("A shining hole opens to the Elemental Plane of Air.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Air.",
	ch,NULL, victim,TO_CHAR,FALSE);
   }
   else if (number < 50)
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_EARTH));
act("A shining hole opens to the Elemental Plane of Earth.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Earth.",
	ch,NULL, victim,TO_CHAR,FALSE);
   }
   else if (number < 75)
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_WATER));
act("A shining hole opens to the Elemental Plane of Water.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Water.",
	ch,NULL, victim,TO_CHAR,FALSE);
   }
   else
   {
      victim = create_mobile(get_mob_index(MOB_VNUM_ELEM_FIRE));
act("A shining hole opens to the Elemental Plane of Fire.",
	ch,NULL,victim,TO_ROOM,FALSE);
act("You summon an elemental from the Elemental Plane of Fire.",
	ch,NULL, victim,TO_CHAR,FALSE);
  } 

   char_to_room (victim,ch->in_room);
   victim->level = level * 9/10;
   for (t = 0; t < 3; t++)
      victim->armor[t] = interpolate(victim->level,50,-150);
   victim->armor[3] = interpolate(victim->level,0,-100);
  victim->max_hit = level * 9 + number_range(level * level / 9, level * level );
   victim->max_hit = UMAX( victim->max_hit, ch->max_hit * 3 /4);
   victim->hit = victim->max_hit;
   victim->max_mana = 100;
   victim->mana = 100;
   victim->damage[DICE_NUMBER] = victim->level/5+1;
   victim->damage[DICE_TYPE]   = 6;
   victim->hitroll = victim->level * 4 / 5;
   victim->damroll = victim->level * 4 / 5;
   SET_BIT (victim->mhs,MHS_ELEMENTAL); /* just to make sure */

   add_follower( victim, ch );
   //victim->leader = ch;
   add_to_group(victim, ch);
   SET_BIT(victim->form,FORM_INSTANT_DECAY);
   SET_BIT(victim->affected_by,AFF_CHARM); /* quick-charm */
   victim->life_timer = ch->level/8+18;

}

/*
void spell_scion_storm(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_ADTA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( dice(4,8) < get_curr_stat(victim,STAT_INT) )
	return;

    af.where		= 0;
    af.type		= sn;
    af.level		= level;
    af.duration		= 1; 
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

act("You stagger beneath the mental assault.",ch,NULL,victim,TO_VICT,FALSE);
act("$N staggers beneath your psionic assault!",ch,NULL,victim,TO_CHAR,FALSE);
act("$N staggers beneath the psionic assault.",ch,NULL,victim,TO_NOTVICT,FALSE);
   return;
   }
*/
   
void spell_imbue(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("The gods imbue you with enhanced casting.\n\r",victim);
	reup_affect(victim,sn,24+level/4,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= 5 + (level / 5);
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("The gods imbue you with enhanced casting.\n\r",victim);
    return;
}

void spell_wrath(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if( IS_NPC(ch) )
	return;

    if ( number_percent() > ch->pcdata->sac / 3 )
    {
	send_to_char("You fail to invoke the anger of your deity.\n\r",ch);
	return;
    }

    ch->pcdata->sac = ( UMAX( 0, ch->pcdata->sac - dice(2,4 ) ) );
    
    dam = ( ch->pcdata->sac * level );
    dam /= ( 100 - (get_skill(ch,gsn_communion)/4) );

    if ( is_affected(victim,sn) )
	dam *= 2;

    if ( saves_spell(level,victim,DAM_OTHER) )
	dam /= 2;

    dam = number_range( 90,100 ) * dam / 100;

    damage(ch,victim,dam,sn,DAM_OTHER,TRUE,TRUE);

    if ( saves_spell(level,victim,DAM_OTHER) )
	return;

    af.where 		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.modifier 	= 0;
    af.duration		= 0;
    af.location		= 0;
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    return;
}

/* mostly a cut an paste of do_pick */
void spell_knock(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj;
    EXIT_DATA *pexit;
    char arg[MAX_INPUT_LENGTH];
    int door;

    one_argument( target_name,  arg );
    log_string( arg );

    if ( arg[0] == '\0' )
    {
    send_to_char( "What do you wish to knock open?\n\r", ch );
	return;
    }

    if ( ( obj = get_obj_here( ch, arg ) ) != NULL )
    {	     
	/* 'pick object' */
    if ( obj->item_type != ITEM_CONTAINER )         
        { send_to_char( "That isn't a container.\n\r", ch ); return; }
	if ( !IS_SET(obj->value[1], CONT_CLOSED) )
	    { send_to_char( "It's not closed.\n\r",        ch ); return; }
	if ( obj->value[2] < 0 )
	    { send_to_char( "It can't be unlocked.\n\r",   ch ); return; }
	if ( !IS_SET(obj->value[1], CONT_LOCKED) )
	    { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
	if ( IS_SET(obj->value[1], CONT_PICKPROOF) )
	    { send_to_char( "You failed.\n\r",             ch ); return; }

	REMOVE_BIT(obj->value[1], CONT_LOCKED);
        act("You knock open $p!",ch,obj,NULL,TO_CHAR,FALSE);
        act("$n knocks open $p.",ch,obj,NULL,TO_ROOM,FALSE);
	return;
    }

    if ( ( door = find_door( ch, arg ) ) >= 0
    	&& (pexit=ch->in_room->exit[door]) != 0)
    {
	/* 'pick door' */
	ROOM_INDEX_DATA *to_room;
	EXIT_DATA *pexit_rev;

    if ( !IS_SET(pexit->exit_info, EX_CLOSED) && !IS_IMMORTAL(ch))
    { send_to_char( "It's not closed.\n\r",        ch ); return; }
    if ( pexit->key < 0 && !IS_IMMORTAL(ch))
	    { send_to_char( "It can't be picked.\n\r",     ch ); return; }
	if ( !IS_SET(pexit->exit_info, EX_LOCKED) )
	    { send_to_char( "It's already unlocked.\n\r",  ch ); return; }
    if ( IS_SET(pexit->exit_info, EX_PICKPROOF) && !IS_IMMORTAL(ch))
	    { send_to_char( "You failed.\n\r",ch ); return; }

	REMOVE_BIT(pexit->exit_info, EX_LOCKED);
    send_to_char( "You knock the door open!\n\r", ch );
    act( "$n knocks the $d open!", ch, NULL, pexit->keyword, TO_ROOM ,FALSE);

	/* pick the other side */
	if ( ( to_room   = pexit->u1.to_room            ) != NULL
	&& ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
	&&   pexit_rev->u1.to_room == ch->in_room )
	{
	    REMOVE_BIT( pexit_rev->exit_info, EX_LOCKED );
	}
    }

    return;
}

void spell_enervation(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( saves_spell(level,victim,DAM_NEGATIVE) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if ( is_affected( victim, sn ) )
    {
	send_to_char("Your magical prowess wavers and falters.\n\r",victim);
	if ( victim != ch )
	  act("$N's magical ability falters.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,2+level/10,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level/10) + 2;
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector		= 0;
    affect_to_char(victim, &af );

    send_to_char("Your magical prowess wavers and falters.\n\r",victim);
    if ( victim != ch )
	act("$N's magical ability falters.",ch,NULL,victim,TO_CHAR,FALSE);

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_fumble(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || saves_spell(level-1,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level / 5);
    af.modifier		= -1 * (level / 10 );
    af.location		= APPLY_DEX;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("You feel awkward.\n\r",victim);

    if ( victim != ch )
    {
	act("$N stumbles clumsily.",ch,NULL,victim,TO_CHAR,FALSE);
    }

    if(check_annointment(victim, ch))
    {
        spell_fumble(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_feeblemind(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || saves_spell(level,victim,DAM_MENTAL) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level/12);
    af.modifier		= -1 * (level/8);
    af.location		= APPLY_INT;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("Your willpower and intellect flicker and fade.\n\r",victim);
    if ( victim != ch )
	act("$N's willpower fades away.",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_feeblemind(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_forget(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || saves_spell(level,victim,DAM_MENTAL) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level/12);
    af.modifier		= 0;
    af.location		= 0;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("Your skills feel unpracticed and unfamiliar.\n\r",victim);
    if ( victim != ch )
	act("$N appears to have forgotten something.",
		ch, NULL, victim, TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_forget(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_blur(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("Your form begins to blur and shift.\n\r",victim);
	if ( victim != ch )
	  act("$N begins to blur and shift before your eyes.",
	      ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,level/5,level);
	return;
    }



    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level / 5 );
    af.location		= APPLY_AC;
    af.modifier		= -20;
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("Your form begins to blur and shift.\n\r",victim);
    if ( victim != ch )
	act("$N begins to blur and shift before your eyes.",
		ch,NULL,victim,TO_CHAR,FALSE);
    return;
}

void spell_hold_person(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if( ch->fighting != NULL || victim->fighting != NULL)
      {
        send_to_char("Things need to be calmer.\n\r",ch);
        return;
      }

    if(IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
    {
       send_to_char("Highlanders are immune to that.\n\r",ch);
       return;
    }
    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians may not be held against their will.\n\r", ch);
      return;
    }
 
    level = UMIN(ch->level, level);

    if ( is_affected( victim, sn ) ||
	saves_spell(level,victim,DAM_OTHER) ) 
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.location		= APPLY_DEX;
    af.modifier		= ( 3 - get_curr_stat(victim,STAT_DEX) );
    af.duration 	= 1 + (UMAX(0,level-51));
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    victim->position = POS_RESTING;
    act("$N stops moving!",ch,NULL,victim,TO_NOTVICT,FALSE);
    send_to_char("Your muscles freeze and refuse to respond!\n\r",victim);
    if ( victim != ch )
	act("$N stops moving.",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_hold_person(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_endure_cold(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_SET(victim->vuln_flags,VULN_COLD)
       ||IS_SET(victim->imm_flags,IMM_COLD)
       ||IS_SET(victim->res_flags,RES_COLD))
    {
	if (ch != victim)
	   send_to_char("You can not cast this on them.\n\r",ch);
	else
	   send_to_char("You can not cast this on yourself.\n\r",ch);
	return;
    }

    af.where		= TO_RESIST;
    af.type		= sn;
    af.level		= level;
    af.location		= 0;
    af.modifier		= 0;
    af.duration		= (level / 5 ) + 2;
    af.bitvector	= RES_COLD;
    affect_to_char( victim, &af );

    send_to_char("Your body is able to endure cold.\n\r",victim);
    if ( victim != ch )
	act("$N is able to endure cold.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_sacred_guardian(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("You are being watched by the gods.\n\r",victim);
	if ( victim != ch )
	  act("$N is being watched over by the gods.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,level/3,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level / 3;
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector 	= 0;
    affect_to_char( victim, &af );

    send_to_char("You are being watched by the gods.\n\r",victim);
    if ( victim != ch )
      act("$N is being watched over by the gods.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_endure_heat(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_SET(victim->vuln_flags,VULN_FIRE)
       ||IS_SET(victim->imm_flags,IMM_FIRE)
       ||IS_SET(victim->res_flags,RES_FIRE))
    {
	if (ch != victim)
	   send_to_char("You can not cast this on them.\n\r",ch);
	else
	   send_to_char("You can not cast this on yourself.\n\r",ch);
	return;
    }

    af.where		= TO_RESIST;
    af.type		= sn;
    af.level		= level;
    af.modifier		= 0;
    af.location		= 0;
    af.duration		= (level / 5 ) + 2;
    af.bitvector	= RES_FIRE;
    affect_to_char( victim, &af );

    send_to_char("Your body is able to resist heat.\n\r",victim);
    if ( victim != ch )
	act("$N is able to resist heat.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_aid(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("Your vitality is blessed by the gods.\n\r",victim);
	if ( victim != ch )
	  act("$N's vitality is blessed by the gods.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,(level/5)+5,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (level / 5) + 5;
    af.location		= APPLY_HIT;
    af.modifier		= (victim->max_hit/(number_fuzzy(5)+number_fuzzy(5)));
    af.bitvector 	= 0;
    affect_to_char( victim, &af );

    send_to_char("Your vitality is blessed by the gods.\n\r",victim);
    if ( victim != ch )
      act("$N's vitality is blessed by the gods.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}  

void spell_bestow_holiness(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if ( obj->item_type != ITEM_WEAPON )
    {
	send_to_char("You can only bestow holiness on weapons.  Back to sunday school for you!.\n\r",ch);
	return;
    }
    if ( IS_WEAPON_STAT(obj,WEAPON_HOLY) )
    {
       send_to_char("Sorry, there are no Mega-Holy weapons.  It only takes one holy flag.\n\r",ch);
       return;
    }

    if(obj->link_name && ch->pcdata && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    {
      send_to_char("Turn off safelink if you want to risk destroying your linked weapon.\n\r", ch);
      return;
    }


    af.where		= TO_WEAPON;
    af.type		= sn;
    af.level		= level;
    af.location		= 0;
    if ( number_percent() == number_percent() )
    {
      if ( number_percent() >= number_percent() )
      {
        act("$p is consumed by the Gods and disappears!",ch,obj,NULL,TO_ROOM,FALSE);
        send_to_char("{RZOUNDS!!!{x Forsooth and {YYea Verily{x that was a little too much praying over the weapon.  It's toast now.\r\n", ch);
        extract_obj(obj);
        return;
      }
      else
      {
        send_to_char("{CPRAISE{x the maker!!!  This weapon can crush many foes!\r\n",ch);
        af.duration       = -1;
      }
    }
    else
    {
      af.duration		= obj->value[4] == 0 ? level / 2 : level / 4;
    }
    af.modifier 	= 0;
    af.bitvector	= WEAPON_HOLY;
    affect_to_obj( obj, &af );

    act("$p radiates with holiness.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$p radiates with holiness.",ch,obj,NULL,TO_ROOM,FALSE);
    return;
}

void spell_flamesword(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if (IS_WEAPON_STAT(obj,WEAPON_FLAMING))
    {
       send_to_char("This weapon can not get any hotter.\n\r",ch);
       return;
    }

    if ( obj->item_type != ITEM_WEAPON  )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where      	= TO_WEAPON; 
    af.type	  	= sn;
    af.level		= level;
    af.duration		= obj->value[4] == 0 ? level / 3 : level / 6;  
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= WEAPON_FLAMING;
    affect_to_obj( obj, &af );

    af.where		= TO_AFFECTS;
    af.bitvector 	= 0;
    af.duration		= obj->value[4] == 0 ? level / 6 : level / 12;  
    af.location		= APPLY_CON;
    af.modifier		= -2;
    affect_to_char( ch, &af );

    act("$p flares brightly with an aura of fire.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$p flares brightly with an aura of fire.",ch,obj,NULL,TO_ROOM,FALSE);
    return;
}    

void spell_frostsword(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if ( obj->item_type != ITEM_WEAPON  )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if (IS_WEAPON_STAT(obj,WEAPON_FROST))
    {
       send_to_char("This weapon can not get any colder.\n\r",ch);
       return;
    }


    af.where      	= TO_WEAPON; 
    af.type	  	= sn;
    af.level		= level;
    af.duration		= obj->value[4] == 0 ? level / 3 : level / 6;  
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= WEAPON_FROST;
    affect_to_obj( obj, &af );

    af.where		= TO_AFFECTS;
    af.bitvector 	= 0;
    af.duration		= obj->value[4] == 0 ? level / 6 : level / 12;  
    af.location		= APPLY_CON;
    af.modifier		= -2;
    affect_to_char( ch, &af );

    act("$p freezes brightly with an aura of ice.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$p freezes brightly with an aura of ice.",ch,obj,NULL,TO_ROOM,FALSE);
    return;
}    

void spell_electricsword(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;

    if ( obj->item_type != ITEM_WEAPON  )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if (IS_WEAPON_STAT(obj,WEAPON_SHOCKING))
    {
       send_to_char("This weapon can not hold more of a charge.\n\r",ch);
       return;
    }

    af.where      	= TO_WEAPON; 
    af.type	  	= sn;
    af.level		= level;
    af.duration		= obj->value[4] == 0 ? level / 3 : level / 6;  
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= WEAPON_SHOCKING;
    affect_to_obj( obj, &af );

    af.where		= TO_AFFECTS;
    af.bitvector 	= 0;
    af.duration		= obj->value[4] == 0 ? level / 6 : level / 12;  
    af.location		= APPLY_CON;
    af.modifier		= -2;
    affect_to_char( ch, &af );

act("$p sizzles brightly with an aura of lightning.",ch,obj,NULL,TO_CHAR,FALSE);
act("$p sizzles brightly with an aura of lightning.",ch,obj,NULL,TO_ROOM,FALSE);
    return;
}    

void spell_asphyxiate(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( is_affected( victim,sn ) )
    {
	send_to_char("That target is already suffocating.\n\r",ch);
	return;
    }

    if ( saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    /* NAIL 'em! */
    dam = level + ( (ch->level > 20) * 3 ) 
		+ ( (ch->level > 35) * 3 )
		+ ( (ch->level > 50) * 3 );
    dam = dice(dam,12);

    damage(ch,victim,dam,sn,DAM_OTHER,TRUE,TRUE);

    if ( IS_AFFECTED(victim,AFF_HASTE) )
       affect_strip(victim,skill_lookup("haste"));

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level / 6;
    af.modifier		= -4;
    af.location		= APPLY_DEX;
    af.bitvector	= AFF_SLOW;
    affect_to_char(victim,&af);

    act("$N gasps in pain as $S throat constricts!",
	ch,NULL,victim,TO_CHAR,FALSE);
    act("You gasp in pain as your throat constricts!",
	ch,NULL,victim,TO_VICT,FALSE);
    act("$N gasps in pain as $S throat constricts!",
	ch,NULL,victim,TO_NOTVICT,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_asphyxiate(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



void spell_create_node(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    if ( ch->in_room == NULL
      || IS_SET(ch->in_room->room_flags, ROOM_GODS_ONLY)    
      || IS_SET(ch->in_room->room_flags, ROOM_NODIE)    
      || IS_SET(ch->in_room->room_flags, ROOM_IMP_ONLY)
      || (ch->in_room->clan && ch->in_room->clan == ch->clan ))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if (IS_NPC(ch))
	return;

    ch->pcdata->node = ch->in_room->vnum;
    send_to_char("Node created.\n\r",ch);
    act("An aura of magical energy lingers.",ch,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_mistform(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn))
    {
	if ( ch == victim )
	  act("Your body becomes insubstantial.",ch,NULL,NULL,TO_CHAR,FALSE);
	act("$n becomes insubstantial.",victim,NULL,NULL,TO_ROOM,FALSE);
	reup_affect(victim,sn,(level/2)+10,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.location		= APPLY_AC;
    af.duration		= level / 2 + 10;
    af.modifier		= ( -2 ) * ( level / 2 );   
    af.bitvector	= 0;

    affect_to_char(ch,&af);

    if ( ch == victim )
    act("Your body becomes insubstantial.",ch,NULL,NULL,TO_CHAR,FALSE);
    act("$n becomes insubstantial.", ch,NULL,NULL,TO_ROOM,FALSE);

    return;
}

void spell_visit_node(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    ROOM_INDEX_DATA *pRoom;

    if (IS_NPC(ch))
	return;

    if ( ch->pcdata->node == 0 )
    {
	send_to_char("You have no node available to visit.\n\r",ch);
	return;
    }

    if ( ( pRoom = get_room_index( ch->pcdata->node ) ) == NULL )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if ( IS_SET(pRoom->room_flags, ROOM_NOCLAN) 
    && (IS_SET(ch->mhs,MHS_HIGHLANDER)))
    {
       send_to_char("Highlander's can not visit nodes there.\n\r",ch);
       return;
    }

    ch->pcdata->node = 0;

    act("$n vanishes from sight!",ch,NULL,NULL,TO_ROOM,FALSE);
    char_from_room(ch);
    clear_mount( ch );
    stop_fighting(ch, TRUE );
    char_to_room(ch,pRoom);
    do_look(ch,"");
    act("$n materializes before you.",ch,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_rust_weapon(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj;
    /*
    int material;
    */

    if(IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
    {
       send_to_char("Highlanders are immune to that.\n\r",ch);
       return;
    }

/*    if ( saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }*/


    if ( (   obj  = get_eq_char(victim,WEAR_WIELD) ) == NULL )
    {
	send_to_char("Your opponent is not wielding anything.\n\r",ch);
	return;
    }

    if (    obj->item_type != ITEM_WEAPON ) /* just in case */
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    /* vorpal check, can't rust a vorpal */
    if ( IS_WEAPON_STAT(obj, WEAPON_VORPAL ) || obj->damaged >= 100 )
    {
	send_to_char( "You failed.\n\r", ch);
	return;
    }

/*
    material = material_lookup(obj->material);

    if ( !IS_SET(material_table[material].flags,MAT_VULN_RUST) )
    {
	send_to_char("You failed3.\n\r",ch);
	return;
    }
*/

if (
str_cmp(obj->material,"adamantanium") && 
str_cmp(obj->material,"adamantite") &&
str_cmp(obj->material,"adamantium") &&
str_cmp(obj->material,"adamite") &&
str_cmp(obj->material,"alloy") &&
str_cmp(obj->material,"aluminum") && 
str_cmp(obj->material,"brass") &&
str_cmp(obj->material,"bronze") &&
str_cmp(obj->material,"coins") &&
str_cmp(obj->material,"copper") &&
str_cmp(obj->material,"fools-gold") &&
str_cmp(obj->material,"gold") &&
str_cmp(obj->material,"iron") &&
str_cmp(obj->material,"lead") &&
str_cmp(obj->material,"metal") &&
str_cmp(obj->material,"mineral") &&
str_cmp(obj->material,"mithril") &&
str_cmp(obj->material,"nickel") &&
str_cmp(obj->material,"obsidian") &&
str_cmp(obj->material,"peweter") &&
str_cmp(obj->material,"pewter") &&
str_cmp(obj->material,"platinum") &&
str_cmp(obj->material,"silver") &&
str_cmp(obj->material,"stainless") && 
str_cmp(obj->material,"steel") &&
str_cmp(obj->material,"tin") &&
str_cmp(obj->material,"tinfoil") &&
str_cmp(obj->material,"titanium") && 
str_cmp(obj->material,"wire"))
    {
	send_to_char("Their weapon is not made out of metal.\n\r",ch);
	return;
    }

    if ( obj->enchanted )
	level -= 4;

    /* This spell is really mean, give 'em two saving throws */
    /* Not so mean now, first one earlier is commented out */
    if ( saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    /* Ok, that's that.  nail the weapon. */
		/* Large variance but it should take it out after two - three casts land*/
	obj->damaged = UMIN(obj->damaged + number_range(level / 5, level * 2), 100);
	if(obj->damaged >= 100)
	{
	    act("$p rusts completely through!",ch,obj,victim,TO_CHAR,FALSE);
	    act("$p rusts completely through!",ch,obj,victim,TO_ROOM,FALSE);
	    remove_bonuses(obj->carried_by, obj);
	}
	else
	{
	    act("Spots of rust appear on $p.",ch,obj,victim,TO_CHAR,FALSE);
	    act("Spots of rust appear on $p.",ch,obj,victim,TO_ROOM,FALSE);
	}

/*    act("$p whithers and rusts away!",ch,obj,victim,TO_CHAR,FALSE);
    act("$p whithers and rusts away!",ch,obj,victim,TO_ROOM,FALSE);
    if ( obj->value[1] <= 0 )
       obj->value[2] = UMAX( 0, obj->value[2] - 1 );
    else
       obj->value[1] = UMAX( 0, obj->value[1] - 1 );*/

    return;
}

void spell_rust_armor(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj, *obj_next;
    /*
    int material;
    */
    int loclevel;
    int i;

    if(IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
    {
       send_to_char("Highlanders are immune to that.\n\r",ch);
       return;
    }

    if ( saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    for ( obj = victim->carrying ; obj != NULL ; obj = obj_next )
    { 
	obj_next = obj->next_content;

        if ( obj->wear_loc == -1 )
	   continue;

	if ( obj->item_type != ITEM_ARMOR || obj->damaged >= 100 )
	    continue;
/*
	material = material_lookup(obj->material);

	if (!IS_SET(material_table[material].flags,MAT_VULN_RUST) )
	    continue;
*/
if (
str_cmp(obj->material,"adamantanium") && 
str_cmp(obj->material,"adamantite") &&
str_cmp(obj->material,"adamantium") &&
str_cmp(obj->material,"adamite") &&
str_cmp(obj->material,"alloy") &&
str_cmp(obj->material,"aluminum") && 
str_cmp(obj->material,"brass") &&
str_cmp(obj->material,"bronze") &&
str_cmp(obj->material,"coins") &&
str_cmp(obj->material,"copper") &&
str_cmp(obj->material,"fools-gold") &&
str_cmp(obj->material,"gold") &&
str_cmp(obj->material,"iron") &&
str_cmp(obj->material,"lead") &&
str_cmp(obj->material,"metal") &&
str_cmp(obj->material,"mineral") &&
str_cmp(obj->material,"mithril") &&
str_cmp(obj->material,"nickel") &&
str_cmp(obj->material,"obsidian") &&
str_cmp(obj->material,"peweter") &&
str_cmp(obj->material,"pewter") &&
str_cmp(obj->material,"platinum") &&
str_cmp(obj->material,"silver") &&
str_cmp(obj->material,"stainless") && 
str_cmp(obj->material,"steel") &&
str_cmp(obj->material,"tin") &&
str_cmp(obj->material,"tinfoil") &&
str_cmp(obj->material,"titanium") && 
str_cmp(obj->material,"wire"))
    {
       continue;
    }


	loclevel = level - 4;// Start with a penalty, level no longer reduces

	if (obj->enchanted)
	   loclevel -= 4;

	if ( saves_spell(loclevel,victim,DAM_OTHER) )
		continue;

		/* Large variance but it should take it out after two - three casts land */
	obj->damaged = UMIN(obj->damaged + number_range(level / 5, level * 2), 100);
	if(obj->damaged >= 100)
	{
	    act("$p rusts completely through!",ch,obj,victim,TO_CHAR,FALSE);
	    act("$p rusts completely through!",ch,obj,victim,TO_ROOM,FALSE);
	    remove_bonuses(obj->carried_by, obj);
	}
	else
	{
	    act("Spots of rust appear on $p.",ch,obj,victim,TO_CHAR,FALSE);
	    act("Spots of rust appear on $p.",ch,obj,victim,TO_ROOM,FALSE);
	}


	/*act("$p whithers and rusts away!",ch,obj,victim,TO_CHAR,FALSE);
	act("$p whithers are rusts away!",ch,obj,victim,TO_ROOM,FALSE);

	for ( i = 0 ; i < 4 ; i++ )
	   obj->value[i] = 0;*/

//	level -= 2; /* reduce casting levelfor every successful land */

    }

    return;
}

void spell_irradiate(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
       if ( ch != victim )
	   act("$N is already affected.",ch,NULL,victim,TO_CHAR,FALSE);
       else
	   act("You're plenty sick as it is.",ch,NULL,victim,TO_CHAR,FALSE);
       return;
    }

    if ( saves_spell(level,victim,DAM_ENERGY) )
    {
       send_to_char("You failed.\n\r",ch);
       return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= 2 + (ch->level > 5 ) +
			      (ch->level > 20 ) +
			      (ch->level > 35 ) +
			      (ch->level > 50 );
    af.modifier		=    - (ch->level > 5)
			     - (ch->level > 20)
			     - (ch->level > 35)
			     - (ch->level > 50);
    af.location		= APPLY_STR;
    af.bitvector        = 0;
    affect_to_char(victim,&af);

    af.location		= APPLY_DEX;
    affect_to_char(victim,&af);

    af.location		= APPLY_CON;
    affect_to_char(victim,&af);


   act("$N appears to be stricken with sickness.",ch,NULL,victim,TO_CHAR,FALSE);
   act("You suddenly feel sick and nauseated.",ch,NULL,victim,TO_VICT,FALSE);
   act("$N appears to be sick and nauseated.",ch,NULL,victim,TO_NOTVICT,FALSE);

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_travel_fog(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *fog;

    if( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
    {
      send_to_char("Your master didn't tell you to do that.\n\r",ch);
      return;
    }

    if((fog = create_object(get_obj_index(OBJ_VNUM_FOG),0,FALSE)) == NULL )
    {
	send_to_char("The sphere of Air has failed you.\n\r",ch);
	return;
    }

    fog->value[0] = 1 +(ch->level>15)+(ch->level>35)+(ch->level>50);
    fog->timer = 10 + ( level / 10 );
    SET_BIT(fog->value[2],GATE_RANDOM);
    SET_BIT(fog->value[2],GATE_BUGGY);
    fog->value[3] = -1;  /*just to reinforce the randomness*/

    obj_to_room(fog,ch->in_room);

    act("$p shimmers into being.",ch,fog,NULL,TO_CHAR,FALSE);
    act("$p shimmers into being.",ch,fog,NULL,TO_ROOM,FALSE);
    return;
}

void spell_earthbind(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    level = UMIN(ch->level, level);

    if ( is_affected(victim,sn) )
    {
       if ( victim != ch )
	  act("$N is already bound to the earth.",ch,NULL,victim,TO_CHAR,FALSE);
       else
	   act("Your feet are already bound.",ch,NULL,victim,TO_CHAR,FALSE);
       return;
    }
/*
    if (  !IS_AFFECTED(victim, AFF_FLYING) )
    {
           
       if ( victim != ch )
	  act("$N is already on the ground.",ch,NULL,victim,TO_CHAR,FALSE);
       else
       act("Your feet are already on the ground.",ch,NULL,victim,TO_CHAR,FALSE);
       return;

    }
 */

    /* This one is a doosy.  Removes a haste AND a bit of dex */
    /* To make it more fair, it's hard to land, especially in town
       where most fights will happen */

    switch( ch->in_room->sector_type )
    {
    /*
    case SECT_INSIDE:    level /= 2;  break;   
    case SECT_CITY:	 level -= 5;  break;
    default:		 level -= 2;  break; 
    */
    case SECT_INSIDE:    level -= 5;  break;   /* Fat chance */
    case SECT_CITY:	 level -= 3;  break;
    default:		 level -= 1;  break; 
    }

    if ( saves_spell(level,victim,DAM_OTHER) )
    {
	act("You failed.",ch,NULL,NULL,TO_CHAR,FALSE);
	return;
    }

     /* remove a haste if it's there */
    if ( IS_AFFECTED(victim,AFF_HASTE) )
	affect_strip(victim,skill_lookup("haste"));
    /* remove half moves */
    ch->move /= 2;

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= ( ch->level / 17 ) ; /* > 35 ); */
    af.location		= APPLY_DEX;
    af.modifier		= -1 * (get_curr_stat(victim,STAT_DEX)/ 4);
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    act("Your feet sink into the ground, you can barely move!",
	ch,NULL,victim,TO_VICT,FALSE);
    act("$N's feet sink into the earth!",ch,NULL,victim,TO_CHAR,FALSE);
    act("$N's feet sink into the earth!",ch,NULL,victim,TO_NOTVICT,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_earthbind(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_stonefist(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
       if ( victim == ch )
	  act("You're already sporting some rocky hands.",
		ch,NULL,NULL,TO_CHAR,FALSE);
       else
	  act("$N is already sporting some rocky hands.",
		ch,NULL,victim,TO_CHAR,FALSE);
       return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.location		= APPLY_DAMROLL;
    af.duration		= level / 3;
    af.modifier		= ch->level / 5;
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    send_to_char("Your hands harden into stone.\n\r",victim);
    if ( victim != ch )
    act("$N's hands harden into stone.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_wall_of_wind(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;

    act("$n summons forth a powerful gale!",ch,NULL,NULL,TO_ROOM,FALSE);
    act("You summon forth a powerful gale!",ch,NULL,NULL,TO_CHAR,FALSE);

    /* Hit primary victim */
    /* Give them some credit for being big */
    level += ( ch->level / 10 );

    if ( saves_spell(level-victim->size,victim,DAM_BASH) )
	act("$N appears to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
    else
    {
	act("$N is blown to the ground!",ch,NULL,victim,TO_CHAR,FALSE);
	act("$N is blown to the ground!",ch,NULL,victim,TO_NOTVICT,FALSE);
	act("You are blown to the ground!",ch,NULL,victim,TO_VICT,FALSE);
	victim->position = POS_RESTING;
	DAZE_STATE(victim,24);
	damage(ch,victim,dice(2,4),sn,DAM_BASH,FALSE,TRUE);
    }

    /* Now hit friends and neighbours */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch_next )
    {
      /* vch_next = vch->next;*/
       vch_next = vch->next_in_room;

       if ( is_safe_spell(ch,vch,TRUE, sn) ||
	   !is_same_group(vch,victim) ||
	    is_same_group(ch,vch) ||
	    vch == victim) 
	    continue;

 	if ( saves_spell(level-vch->size,vch,DAM_BASH) )
	   act("$N appears to be unaffected.",ch,NULL,vch,TO_CHAR,FALSE);
	else
	{
	   act("$N is blown to the ground!",ch,NULL,vch,TO_CHAR,FALSE);
	   act("You are blown to the ground!",ch,NULL,vch,TO_VICT,FALSE);
	   act("$N is blown to the ground!",ch,NULL,vch,TO_NOTVICT,FALSE);
	   vch->position = POS_RESTING;
	   DAZE_STATE(vch,24);
	   damage(ch,vch,dice(2,4),sn,DAM_BASH,FALSE,TRUE);
 	}
    }
    return;
}

void spell_dust_storm(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    CHAR_DATA *vch, *vch_next;

    level = UMIN(ch->level, level);

    if ( IS_AFFECTED(victim,AFF_BLIND) )
    {
	send_to_char("That victim is already blinded.\n\r",ch);
	return;
   }

  act("$n summons forth a billowing storm of dust!",ch,NULL,NULL,TO_ROOM,FALSE);
  act("You summon forth a billowing storm of dust!",ch,NULL,NULL,TO_CHAR,FALSE);

     af.where		= TO_AFFECTS;
     af.type		= sn;
     af.level		= level;
     af.location	= APPLY_HITROLL;
     af.modifier	= -4;
     af.duration	= 1 + (ch->level > 35 ) + (ch->level > 50 );
     af.bitvector	= AFF_BLIND;

     if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
       af.duration  = 1;
 /* Check intended victim first */
    if ( saves_spell(level,victim,DAM_OTHER) )
	act("$N appears to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
    else
    {
	act("$N is blinded!",ch,NULL,victim,TO_CHAR,FALSE);
	act("You are blinded!",ch,NULL,victim,TO_VICT,FALSE);
	act("$N  is blinded!",ch,NULL,victim,TO_NOTVICT,FALSE);
	affect_to_char(victim,&af);
	damage(ch,victim,dice(2,4),sn,DAM_OTHER,FALSE,TRUE);
        apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);
    }

    /* Check victim's group members */
    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch_next )
    {
       vch_next = vch->next_in_room;

       if ( !is_same_group(victim,vch) ||
	     is_same_group(vch,ch) ||
	     is_safe_spell(ch,vch,TRUE, sn) ||
	     victim == vch )
	     continue;

       if ( saves_spell(level,vch,DAM_OTHER) || IS_AFFECTED(vch, AFF_BLIND) )
	   act("$N appears to be unaffacted.",ch,NULL,vch,TO_CHAR,FALSE);
       else
       {
	  act("$N is blinded!",ch,NULL,vch,TO_CHAR,FALSE);
	  act("You are blinded!",ch,NULL,vch,TO_VICT,FALSE);
	  act("$N is blinded!",ch,NULL,vch,TO_NOTVICT,FALSE);
	  affect_to_char(vch,&af);
	  damage(ch,vch,dice(2,4),sn,DAM_OTHER,FALSE,TRUE);
       }
    }

    if(check_annointment(victim, ch))
    {
        spell_dust_storm(sn,level+2,victim,ch,target);
    } 

    return;
}

void spell_water_breathing(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("Your lungs have the capacity to breathe water.\n\r",victim);
	if ( victim != ch )
	  act("$N can now breathe underwater.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,level/2,level);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.modifier		= 0;
    af.duration		= level / 2;
    af.location		= 0;
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    if ( level > 35 
      && !IS_SET(victim->res_flags,RES_DROWNING) )
    {
    af.where 		= TO_RESIST;
    af.bitvector	= RES_DROWNING;
    af.duration		= level / 10;
    affect_to_char(victim,&af);
    }

    send_to_char("Your lungs have the capacity to breathe water.\n\r",victim);

    if ( victim != ch )
    act("$N can now breathe underwater.",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}

void spell_body_of_stone(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( victim->race == race_lookup("rockbiter") )
    {
	if ( victim != ch )
	    act("$N is already affected.",ch,NULL,victim,TO_CHAR,FALSE);
	else
	    act("You are already made of stone.",ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    if( is_affected( victim, sn ) )
    {
	reup_affect(victim,sn,level,level);
	send_to_char("Your body hardens to stone!\n\r",victim);
	if ( victim != ch )
	  act("$N's body hardens to stone!",ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    af.where 		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= ch->level;
    af.location		= APPLY_CON;
    af.modifier		= 1 + ( level > 15 ) + (level > 30 ) + (level > 45 );
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    if ( ch->level > 35 ) 
    {
    af.where		= TO_RESIST;
    af.level		= level / 5;
    af.duration		= UMAX(0,level / 10 - 1);
    af.location		= 0;
    af.modifier		= 0;
    af.bitvector	= RES_ACID;  
    if ( !IS_SET(victim->res_flags,RES_ACID) )
        affect_to_char(victim,&af);
    if ( ch->level > 50 )
    af.bitvector 	= RES_WEAPON;
    if ( !IS_SET(victim->res_flags,RES_WEAPON ) )
	affect_to_char(victim,&af);
    }

    send_to_char("Your body hardens to stone!\n\r",victim);
    if ( victim != ch )
       act("$N's body hardens to stone!",ch,NULL,victim,TO_CHAR,FALSE);

    return;
}


void spell_ice_storm(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam;
   char buf[MAX_STRING_LENGTH];

    dam = dice(level, ch->level/3);   

    if ( saves_spell(level,victim,DAM_COLD)) 
        damage(ch,victim,dam/2,sn,DAM_COLD, TRUE ,TRUE);
    else
           damage(ch,victim,dam,sn,DAM_COLD, TRUE,TRUE);

    /* Gladiator Spectator Channel */ 
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s summons a powerful ice storm, freezing everyone in the room.",ch->name);
       gladiator_talk(buf);
    }

    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch_next )
    {
       vch_next = vch->next_in_room;
       
       if ( is_safe_spell(ch,vch,TRUE, sn) ||
           !is_same_group(vch,victim) ||
            is_same_group(ch,vch) ||
            vch == victim )
            continue;

        if ( saves_spell(level,vch,DAM_COLD) )
           damage(ch,vch,dam/2,sn,DAM_COLD, TRUE,TRUE);
        else
           damage(ch,vch,dam,sn,DAM_COLD, TRUE,TRUE);
    }

    return;
}

void spell_farsight( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    if (IS_AFFECTED(ch,AFF_BLIND))
    {
        send_to_char("Maybe it would help if you could see?\n\r",ch);
        return;
    }
 
    do_scan(ch,target_name);
}


void spell_portal( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;

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

        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   ch->in_room->vnum < 0
    ||   victim->in_room->vnum < 0
    ||   !can_see_room(ch,victim->in_room)
    ||   !is_room_clan(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   victim->level >= ch->level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) 
    ||  (is_clan(victim) && !is_same_clan(ch,victim))
    ||  victim->in_room->area->under_develop 
    ||  victim->in_room->area->no_transport 
    ||  ch->in_room->area->no_transport )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   

    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch) 
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
  send_to_char("You lack the proper component for this spell.\n\r",ch);
  return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
      act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR,FALSE);
      act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR,FALSE);
      extract_obj(stone);
    }

    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0,FALSE);
    portal->timer = 2 + level / 25; 
    portal->value[3] = victim->in_room->vnum;

    obj_to_room(portal,ch->in_room);

    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM,FALSE);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR,FALSE);
}

void spell_nexus( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *portal, *stone;
    ROOM_INDEX_DATA *to_room, *from_room;

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

    from_room = ch->in_room;
 
        if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   (to_room = victim->in_room) == NULL
    ||   !can_see_room(ch,to_room) || !can_see_room(ch,from_room)
    ||	 !is_room_clan(ch,to_room) || !is_room_clan(ch,from_room)
    ||   ch->in_room->vnum < 0
    ||   victim->in_room->vnum < 0
    ||   IS_SET(to_room->room_flags, ROOM_SAFE)
    ||   IS_SET(from_room->room_flags,ROOM_SAFE)
    ||   IS_SET(to_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(to_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(to_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(from_room->room_flags,ROOM_NO_RECALL)
    ||   victim->level >= ch->level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_HERO)  /* NOT trust */
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_NONE) ) 
    ||   (is_clan(victim) && !is_same_clan(ch,victim))
    ||  victim->in_room->area->under_develop 
    ||  victim->in_room->area->no_transport
    ||  ch->in_room->area->no_transport )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    }   
 
    stone = get_eq_char(ch,WEAR_HOLD);
    if (!IS_IMMORTAL(ch)
    &&  (stone == NULL || stone->item_type != ITEM_WARP_STONE))
    {
        send_to_char("You lack the proper component for this spell.\n\r",ch);
        return;
    }
 
    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
        act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR,FALSE);
        act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR,FALSE);
        extract_obj(stone);
    }

    /* portal one */ 
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0,FALSE);
    portal->timer = 1 + level / 10;
    portal->value[3] = to_room->vnum;
 
    obj_to_room(portal,from_room);
 
    act("$p rises up from the ground.",ch,portal,NULL,TO_ROOM,FALSE);
    act("$p rises up before you.",ch,portal,NULL,TO_CHAR,FALSE);

    /* no second portal if rooms are the same */
    if (to_room == from_room)
  return;

    /* portal two */
    portal = create_object(get_obj_index(OBJ_VNUM_PORTAL),0,FALSE);
    portal->timer = 1 + level/10;
    portal->value[3] = from_room->vnum;

    obj_to_room(portal,to_room);

    if (to_room->people != NULL)
    {
  act("$p rises up from the ground.",to_room->people,portal,NULL,TO_ROOM,FALSE);
  act("$p rises up from the ground.",to_room->people,portal,NULL,TO_CHAR,FALSE);
    }
}
     
void do_morph( CHAR_DATA *ch, char *argument )
{
    AFFECT_DATA af;
    int sn, dur;

    if(IS_NPC(ch))
      return;

    sn = skill_lookup("morph");

    if ( ch->mana < (ch->level/5) )
    {
	send_to_char("You can't seem to transform.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_MORPH))
    {
       send_to_char("You are already transformed.\n\r",ch);
       return;
    }

    ch->mana -= (ch->level/5);

    if ( number_percent( ) < get_skill(ch,gsn_morph))
    {
       send_to_char( "You transform into a beast state.\n\r", ch );
       dur = number_range(4,5);

       check_improve(ch,gsn_morph,TRUE,3);
       af.where     = TO_AFFECTS;
       af.type      = gsn_morph;
       af.level     = ch->level;
       af.duration  = dur;
       af.location  = APPLY_NONE;
       af.modifier  = 0;
       af.bitvector = AFF_MORPH;
       affect_to_char( ch, &af );

/*       if(!IS_SET(ch->mhs,MHS_CURSE))
       {
          af.where     = TO_AFFECTS;
          af.type      = sn;
          af.level     = 2*ch->level;
          af.duration  = dur;
          af.location  = APPLY_NONE;
          af.modifier  = 0;
          af.bitvector = MHS_CURSE;
          affect_to_char( ch, &af );
       }
*/
    }
    else
    {
       send_to_char("You fail to transform into a beast state.\n\r",ch);
       check_improve(ch,gsn_morph,FALSE,3);
    }
     
    return;
}    

void do_shapemorph( CHAR_DATA *ch, char *argument)
{

   char arg[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   /*
   DESCRIPTOR_DATA d;
   bool isChar;
    */
   FILE	*fp;
   char strsave[MAX_INPUT_LENGTH];
   sh_int i;
   char strstore[MAX_INPUT_LENGTH];
   return;



    sprintf(log_buf,"%s shapemorphing into %s",ch->name,argument);
    log_string(log_buf);

   if( IS_NPC(ch) || !HAS_KIT(ch,"shapeshifter"))
   {
     send_to_char("You got a problem with how you look?\n\r",ch);
     return;
   }

   if(IS_SET(ch->mhs,MHS_GLADIATOR))
   {
      send_to_char("Gladiator's can not shapemorph.\n\r",ch);
      return;
   }

   strcpy(strstore,argument);

   for( i=0 ; argument[i] != '\0' ; i++ )
   {
	if( ispunct(argument[i]) || argument[i] == '{' || isdigit(argument[i]) )
	{
	  send_to_char( "Oooh, you are so tricky aren't you?", ch);
	  return;
	}
   }

   one_argument(argument,arg);
   if( arg[0] == '\0')
   {
      send_to_char("You must supply a name to morph into.\n\r",ch);
      return;
   }

   if ( !str_prefix(arg,"return"))
   {
      if (IS_SET(ch->mhs,MHS_SHAPEMORPHED))
      {
	 REMOVE_BIT(ch->mhs,MHS_SHAPEMORPHED);
         send_to_char("You return to your regular appearance. How boring!\n\r",ch);
	 act( "$n returns to $s regular appearance.", ch,NULL,ch,TO_ROOM,FALSE);
         return;
      }
      else
      {
         send_to_char("You are not Morphed.\n\r",ch);
         return;
      } 
   }

   if(IS_SET(ch->mhs,MHS_SHAPEMORPHED))
   {
      send_to_char("You are already morphed.\n\r",ch);
      return;
   }


   /* Don't allow morphing into other players */
   /* Simply check if the pfile name is there to open
      and then close it!
    */
   sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( arg ) );
   if ( ( fp = fopen( strsave, "r" ) ) != NULL )
   {                                                                                             
      send_to_char( "They would not be very happy with you, would they?\n\r", ch );
      fclose( fp );
      return;                   
   }     

  /* Dont allow morphing into Mobs, this is only cause 2.mobname isnt working */
   if ( ( victim = get_char_world( ch, argument ) ) != NULL)
   {
      send_to_char("Becoming a Mob is so beneath you.\n\r",ch);
      return;
   }

   i = 0;
   while(arg[i] != '\0')
   {
      arg[i] = tolower(arg[i]);
      i++;
   }

   if (!strncmp(arg,"self",4))
   {
      send_to_char("Self is not valid as the start to a morphing name.\n\r",ch);
      return;
   }

/*
   i = 0;
   while (strstore[i] != '\0')
   {
      if(!isalpha(strstore[i]))
      {
	 send_to_char("a thru Z are the only valid characers for shapemorph.\n\r",ch);
	 return;
      }
      i++;
   }
   */


   free_string( ch->long_descr );                                        
   ch->long_descr = str_dup( argument ); 
   SET_BIT(ch->mhs,MHS_SHAPEMORPHED);

   sprintf(buf,"You morph into %s.\n\r",ch->long_descr );
   send_to_char(buf,ch);
   act( "$n morphs into $r.",ch,NULL,ch->long_descr,TO_ROOM ,FALSE);
   return;
}

/* ShapeShift in Work - Poquah */
void do_shapeshift( CHAR_DATA *ch, char *argument)
{
   char arg[MAX_INPUT_LENGTH];
   sh_int i = 0;
   sh_int temp_stat[MAX_STATS];
   sh_int race;
   char buf[MAX_STRING_LENGTH];
   sh_int failchance;
   sh_int sizediff;
   

   if( IS_NPC(ch) || !HAS_KIT(ch,"shapeshifter"))
   {
     send_to_char("You got a problem with the race you are currently?\n\r",ch);
     return;
   }

   one_argument(argument,arg);
   if( arg[0] == '\0')
   {
      send_to_char("You must supply a race to shapeshift into.\n\r",ch);
      return;
   }
   if ( !str_prefix(arg,"return"))
   {
      if (IS_SET(ch->mhs,MHS_SHAPESHIFTED))
      {
	 if (ch->save_race != ch->race)
	 {
	    REMOVE_BIT(ch->mhs,MHS_SHAPESHIFTED);
            shapeshift_remove(ch); 
            return;
	 }
	 else
	 {
	    send_to_char("You are recovering from a shift.\n\r",ch);
	    return;
	 }
      }
      else
      {
         send_to_char("You are not Shapeshifted.\n\r",ch);
         return;
      } 
   }

   race = race_lookup(arg);

   if (race == 0 || !race_table[race].pc_race)
   {
      send_to_char("That is not a valid race.\n\r",ch);
      return;
   }

   if (!str_cmp(race_table[race].name,"mutant"))
   {
      send_to_char("You can not shift into a mutant.\n\r",ch);
      return;
   }

   if (IS_SET(ch->mhs,MHS_SHAPESHIFTED))
   {
      send_to_char("You have not recovered from your last shift.\n\r",ch);
      return;
   }

   if (race == ch->race)
   {
      send_to_char("You are already that race.\n\r",ch);
      return;
   }

   /* Not allowed to attempt shift with no mana */
   if (ch->mana < 25)
   {
     send_to_char("You are too drained to attempt the shift.\n\r",ch);
     return;
   }

   sizediff = abs(ch->size - pc_race_table[race].size);
   /* At low level can not shift into any size different from original */
   if (ch->level <= 15)
   {
      if (sizediff != 0) 
      {
    send_to_char("You have not the skill to shift into that size.\n\r",ch);
    return;
      }
   }

   /* Place Current HP/MANA drain here to hit people even if they fail */
   /* Slash Current HP and Mana from Drain of Shifting */
   /* Dont drop below one, no need to kill them */
   if (ch->hit > 1)
      ch->hit = ch->hit /2;

   /* Let mana drop below 25 so they cant continue spamming to shift */
   if (ch->mana > 1)
      ch->mana = ch->mana /2;

   /* Sizes greater then original is % based on level */
   failchance = 100 - ((ch->level - (9 + (sizediff * 7))) * 14);

   if (failchance <= 0)
      failchance = 1;

   if (failchance > 95)
      failchance = 95;

   if ((ch->level == 51) && (sizediff == 5))
      failchance = 5;

   if (sizediff == 0)
      failchance = 1;
  
   if (number_percent() <= failchance)
   {
send_to_char("Your skin stretches and tears then returns to its normal form.\n\r",ch);
return;
   }

   /* Save off Real Race Information for Restore later */
   /* Save Race */
   ch->save_race = ch->race;

   /* Save Stats */
   for (i = 0; i < MAX_STATS; i++)
      ch->save_stat[i] = ch->perm_stat[i];

   /* Save HP,Mana 
   ch->save_max_hit = ch->max_hit;
   ch->save_max_mana = ch->max_mana;
*/

   /* Remove ALL Worn EQ */
   remove_all_objs(ch, TRUE);

   /* Begin Shapeshift */
   ch->race = race;

   /* initialize stats */
   /* Get New Race Stats Temp */
   for (i = 0; i < MAX_STATS; i++)
      temp_stat[i] = pc_race_table[race].stats[i];

   /* Compare New Race Stats to Existing Stats */
   /* If Existing Stat is below New Race Minimum */
   for ( i = 0 ; i < MAX_STATS ; i++ )
      if (ch->perm_stat[i] < temp_stat[i])
      {
         ch->perm_stat[i] = temp_stat[i];
      }

   /* If Existing Stat is above New Race Maximum */
   for ( i = 0 ; i < MAX_STATS ; i++ )
      if (ch->perm_stat[i]  >= get_max_train(ch,i)) 
      {
         ch->perm_stat[i] = get_max_train(ch,i); 
      }
      
   /* Add on New Race Aff,Imm,Res,Vulns,Form,Parts,Size */
   ch->affected_by = race_table[race].aff;
   ch->imm_flags   = race_table[race].imm;
   ch->res_flags   = race_table[race].res;
   ch->vuln_flags  = race_table[race].vuln;
   ch->form        = race_table[race].form;
   ch->parts       = race_table[race].parts;
   ch->size 	   = pc_race_table[race].size;

   /* Drop CON for being ShapeShifted -3 initial 
      and %5 chance every tick -1 */
   ch->mod_stat[STAT_CON] -= 3;
   ch->save_con_mod = -3;

   /* Display change to CH and Room */
   if (!str_prefix(race_table[race].name ,"elf"))
   {
      sprintf(buf,"You transform into an %s.\n\r",race_table[race].name );
      send_to_char(buf,ch);
 act( "$n transforms into an $r.",ch,NULL,race_table[race].name,TO_ROOM ,FALSE);
   }
   else
   {
      sprintf(buf,"You transform into a %s.\n\r",race_table[race].name );
      send_to_char(buf,ch);
 act( "$n transforms into a $r.",ch,NULL,race_table[race].name,TO_ROOM ,FALSE);
   }
   SET_BIT(ch->mhs,MHS_SHAPESHIFTED);
   return;
}

void shapeshift_remove( CHAR_DATA *ch)
{
   sh_int i = 0;
   sh_int race = 0;

   /* Return from Shapeshift */

   /* Remove All Worn EQ */
   remove_all_objs(ch, TRUE);

   /* Retrieve Real Race Information from Save */
   /* Return Race */
   race = ch->save_race;
   ch->race = race;

   /* Return HP,Mana 
   ch->max_hit = ch->save_max_hit;
   ch->max_mana = ch->save_max_mana;
*/

   /* Return Stats */
   for (i = 0; i < MAX_STATS; i++)
      ch->perm_stat[i] = ch->save_stat[i];

   /* Return Old Race Aff,Imm,Res,Vulns,Form,Parts,Size */
   ch->affected_by = race_table[race].aff;
   ch->imm_flags   = race_table[race].imm;
   ch->res_flags   = race_table[race].res;
   ch->vuln_flags  = race_table[race].vuln;
   ch->form        = race_table[race].form;
   ch->parts       = race_table[race].parts;
   ch->size        = pc_race_table[race].size;

   /* Slash Current HP and Mana from Drain of Returning */
   /* Dont drop below one, no need to kill them */
   if (ch->hit > 1)
      ch->hit = ch->hit /2;
   
   /* Dont drop Mana below 1, just to be kind */
   if (ch->mana > 1)
      ch->mana = ch->mana /2;

   /* Display return to CH and Room */
   act( "You return to your natural form.\n\r",ch,NULL,NULL,TO_CHAR,FALSE);
   act( "$n returns to $s natural form.", ch, NULL, ch, TO_ROOM ,FALSE);

   return;
}
/* Shapeshift currently in work - Poquah */

void spell_smoke_screen ( int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim;
  AFFECT_DATA af;

  if((victim = ch->fighting) == NULL)
    {
        send_to_char("But you aren't fighting anyone.\n\r",ch);
        return;
    }

  if ( saves_spell(level,victim,DAM_OTHER))
  return;

  send_to_char("You bring forth a cloud of smoke to hide your escape, ",ch);
  if(IS_SET(ch->display,DISP_COLOR))
   send_to_char( GREEN"RUN!"NORMAL"\n\r", ch );
  else
   send_to_char( "RUN!\n\r", ch );

  stop_fighting( victim, FALSE );
  stop_fighting( ch, FALSE );

    if (IS_AFFECTED(victim,AFF_BLIND))
        return;

  act("$n is blinded by the smoke in $s eyes!",victim,NULL,NULL,TO_ROOM,FALSE);
  act("$n's smoke cloud gets in your eyes!",ch,NULL,victim,TO_VICT,FALSE);
  send_to_char("You can't see a thing!\n\r",victim);

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level;
  af.duration = 0;
  af.location = APPLY_HITROLL;
  af.modifier = -2;
  af.bitvector  = AFF_BLIND;

  affect_to_char(victim,&af);
    
    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

  return;
} 

void blow_orb(CHAR_DATA *victim,int sn)
{
    affect_strip(victim,sn);
    if ( skill_table[sn].msg_off )
      {
       send_to_char( skill_table[sn].msg_off, victim );
       send_to_char( "\n\r", victim );
      }

     if ( sn == skill_lookup("orb of touch") )
     WAIT_STATE(victim,12);

    return;
}

void spell_orb_of_touch(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
  if (victim == ch)
    send_to_char("You are already surrounded by that orb.\n\r",ch);
  else
    act("$N is already surrounded by that orb.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not cast orbs in the arena.\n\r", ch);
       return;
    }

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level+ ch->level/8;    
  af.duration = level/2;
  af.location = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = 0;

  affect_to_char(victim,&af);
  send_to_char( "You are surrounded by an orb of touch.\n\r", victim );
    if ( ch != victim )
  act("$N is surrounded by an orb of touch.",ch,NULL,victim,TO_CHAR,FALSE);

  return;
}

void spell_orb_of_surprise(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
  if (victim == ch)
    send_to_char("You are already surrounded by that orb.\n\r",ch);
  else
    act("$N is already surrounded by that orb.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not cast orbs in the arena.\n\r", ch);
       return;
    }

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level;
  af.duration = level/2;
  af.location = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = 0;

  affect_to_char(victim,&af);
  send_to_char( "You are surrounded by an orb of surprise.\n\r", victim );
    if ( ch != victim )
  act("$N is surrounded by an orb of surprise.",ch,NULL,victim,TO_CHAR,FALSE);  

  return;
}

void spell_orb_of_awakening(int sn, int level,CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
  if (victim == ch)
    send_to_char("You are already surrounded by that orb.\n\r",ch);
  else
    act("$N is already surrounded by that orb.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not cast orbs in the arena.\n\r", ch);
       return;
    }

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level;
  af.duration = level/2;
  af.location = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = 0;

  affect_to_char(victim,&af);
  send_to_char( "You are surrounded by an orb of awakening.\n\r", victim );
    if ( ch != victim )
  act("$N is surrounded by an orb of awakening.",ch,NULL,victim,TO_CHAR,FALSE);  

  return;
}

void spell_orb_of_turning(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
  if (victim == ch)
    send_to_char("You are already surrounded by that orb.\n\r",ch);
  else
    act("$N is already surrounded by that orb.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not cast orbs in the arena.\n\r", ch);
       return;
    }

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level;
  af.duration = level/2;
  af.location = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = 0;

  affect_to_char(victim,&af);
  send_to_char( "You are surrounded by an orb of turning.\n\r", victim );
    if ( ch != victim )
  act("$N is surrounded by an orb of turning.",ch,NULL,victim,TO_CHAR,FALSE);  

  return;
}

void spell_see_soul(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{

  CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf[MAX_STRING_LENGTH];

   level = UMAX(ch->level,level);
  if( saves_spell( level, victim,DAM_HOLY) )
	return;

  send_to_char( "They are ", ch );
    if ( victim->alignment >  900 ) send_to_char( "angelic.\n\r", ch );
     else if ( victim->alignment >  700 ) send_to_char( "saintly.\n\r", ch );
     else if ( victim->alignment >  350 ) send_to_char( "good.\n\r",    ch );
     else if ( victim->alignment >  100 ) send_to_char( "kind.\n\r",    ch );
     else if ( victim->alignment > -100 ) send_to_char( "neutral.\n\r", ch );
     else if ( victim->alignment > -350 ) send_to_char( "mean.\n\r",    ch );
     else if ( victim->alignment > -700 ) send_to_char( "evil.\n\r",    ch );
     else if ( victim->alignment > -900 ) send_to_char( "demonic.\n\r", ch );
     else                             send_to_char( "satanic.\n\r", ch );

  if(!IS_NPC(victim))
  {
 sprintf(buf,"They are a follower of %s.\n\r",deity_table[victim->pcdata->deity].pname);
 send_to_char(buf,ch);
  }
  /*
check_killer(ch,victim);
*/

 return;
}

void spell_crusade(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
 CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf[MAX_STRING_LENGTH];

   level = UMAX(ch->level,level);
  if(IS_NPC(victim) || ch->pcdata->deity == victim->pcdata->deity 
	|| saves_spell( level, victim,DAM_HOLY) )
    {
	send_to_char("Nothing seems to happen.\n\r", ch);
	return;
    }

       if(ch->pcdata->sac< 10)
    {
	sprintf(buf,"You are powerless to act in %s's name.\n\r",
	deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,ch);
	return;
    }

	  sprintf(buf,"Using the gifts of %s you attempt to convert %s.\n\r",
		deity_table[ch->pcdata->deity].pname, victim->name);
	send_to_char(buf,ch);
	sprintf(buf,"%s protects you from %s's agent.\n\r",
		deity_table[victim->pcdata->deity].pname,deity_table[ch->pcdata->deity].pname);
	send_to_char(buf,victim);

/* Poquah change, cause it cause + sac points
  loss = ch->pcdata->sac / 2;
  loss = UMIN( victim->pcdata->sac - loss, loss );
  */
  /* loss = UMIN( loss, 10 ); */

/* Poquah change cause it cases + sac points
  ch->pcdata->sac -= loss;
  victim->pcdata->sac = UMAX(0,victim->pcdata->sac - loss);
  */

  ch->pcdata->sac /= 2;
  victim->pcdata->sac /= 2;

  switch(number_range(1,2))
  {
	default: break;
	case 1:  if ( !saves_spell( level, victim,DAM_HOLY) )
		victim->move /= 2; 
		break;
	case 2:   if ( !saves_spell( level, victim,DAM_HOLY) )
		{ victim->mana /= 3; victim->mana *= 2; }
		break;
  }
  check_killer(ch,victim);

  return;
}

void spell_holy_silence(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
 CHAR_DATA *vch,*vch_next;
  AFFECT_DATA af;
  char buf[MAX_STRING_LENGTH];

   level = UMAX(ch->level,level);
   for ( vch = char_list; vch != NULL; vch = vch_next )
    {
    vch_next  = vch->next;
  if ( vch->in_room == NULL )
      continue;
  if ( vch->in_room == ch->in_room 
      && !IS_NPC(vch)
      && !is_safe_spell(ch,vch,FALSE, sn) )
  {
    if ( is_affected( vch, sn ) || saves_spell( level, vch,DAM_HOLY) )
  continue;

  af.where  = TO_AFFECTS;
  af.type   = sn;
  af.level  = level;
  af.duration = level/10;
  af.location = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = 0;

affect_to_char(vch,&af);
  sprintf(buf, "You no longer feel the presence of %s.\n\r", 
	deity_table[vch->pcdata->deity].pname );
  send_to_char(buf,vch);
    if ( ch != vch )
  act("$N is lost to their deity.",ch,NULL,vch,TO_CHAR,FALSE);
  }
	}

return;
}


void spell_farsee(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
//1518 is the object's vnum
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  OBJ_DATA *obj;
  char  buf[MAX_STRING_LENGTH];
  if ( IS_NPC(ch) )
  {
    return;
  }

  if ( !is_clan(ch) )
  {
    send_to_char("You are not a clanner.  This is a clanner only spell.  You are a fool and will not be reimbursed.\r\n",ch);
    return;
  }

  obj = get_eq_char(ch,WEAR_HOLD);

  if ( obj == NULL || obj->pIndexData->vnum != 1518 )
  {
    send_to_char("You lack the component for this spell.\n\r",ch);
    return;
  }
  
  if ( ch->in_room->sector_type == SECT_INSIDE )
  {
    send_to_char("You cannot be inside to cast the farsee spell.\r\n",ch);
    return;
  }
/*
 if (!IS_NPC(ch) && !HAS_KIT(ch,"seer"))
 {
   send_to_char("You are no longer a Seer, the spell fails you.\n\r",ch);
   return;
 }
*/
      act("You draw upon the power of $p.",ch,obj,NULL,TO_CHAR,FALSE);
      act("It flares brightly and vanishes!",ch,obj,NULL,TO_CHAR,FALSE);
      extract_obj(obj);

 if ( ( victim = get_char_world( ch, target_name ) ) == NULL                 
   ||   victim == ch                                                           
   ||   victim->in_room == NULL                                                
   ||   !can_see_room(ch,victim->in_room)                                      
   ||   !is_room_clan(ch,victim->in_room)                                      
   ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)                      
   ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)                     
   ||   (is_clan(victim) && !is_clan(ch) && !is_same_group(ch,victim))
   ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */
   ||   saves_spell( level, victim, DAM_MENTAL)
   ||   (!is_room_owner(ch,victim->in_room) && room_is_private(ch,victim->in_room))
   ||  victim->in_room->area->under_develop                                    
   ||  victim->in_room->area->no_transport                                     
   ||  ch->in_room->area->no_transport )                                       
   {
       send_to_char( "You failed.\n\r", ch );                                  
       return;                                                                 
   }
    if ( saves_spell( level, victim, DAM_MENTAL ) )
    {
act("You feel as though you are being watched.",victim,NULL,NULL,TO_CHAR,FALSE);
    }

    sprintf(log_buf,"%s casting seer to find %s",ch->name,victim->name);
    log_string(log_buf);
 sprintf(buf,"%s is at %s\n\r",IS_NPC(victim)?victim->short_descr:victim->name
	,victim->in_room->name);
 send_to_char(buf,ch);

 return;
}

void spell_tsunami(int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    sprintf(log_buf,"%s casting tsunami in room %d %s",ch->name,ch->in_room->vnum,ch->in_room->name);
    log_string(log_buf);

    send_to_char( "You summon forth a wall of water!\n\r", ch );
    act( "$n summons forth a wall of water.", ch, NULL, NULL, TO_ROOM ,FALSE);

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
       vch_next  = vch->next;
       if ( vch->in_room == NULL )
          continue;
       if ( vch->in_room == ch->in_room )
       {
          if ( vch == ch || is_safe_spell(ch,vch,TRUE, sn))
          {
             if ( vch != ch )
                send_to_char("Some water splashes at your ankles.\n\r",vch);
             continue;
          }
          else
          {
             damage( ch,vch,2*level + dice(3, 8), sn, DAM_DROWNING,TRUE,TRUE);

	     /* CAREFUL, they may have died.  Don't want to play with
		moving a non-existant (NULL) vch as in a dead NPC
	      */
	      if ( (vch == NULL) || (vch->in_room == NULL) 
		   || (vch->in_room != ch->in_room) )
	      {
		continue;
	      }
              if ( IS_NPC(vch) && vch->spec_fun != 0 )
              {
                if (
                   vch->spec_fun == spec_lookup("spec_honor_guard")
                   || vch->spec_fun == spec_lookup("spec_demise_guard")
                   || vch->spec_fun == spec_lookup("spec_posse_guard")
                   || vch->spec_fun == spec_lookup("spec_zealot_guard")
                   || vch->spec_fun == spec_lookup("spec_warlock_guard")
                   )
                   {
                     continue;
                   }
               }


             if(!saves_spell(level,vch,DAM_DROWNING))
             {
                 EXIT_DATA *pexit;
                 int door;

                 /* you failed your saves, you are now dazed and you might be moved */
                 send_to_char("You are knocked about gurgling and sputtering.\n\r",vch);
                 DAZE_STATE(vch,24);
                 vch->position = POS_RESTING;
	
                 door = number_door();
                 if ( ( pexit = vch->in_room->exit[door] ) == 0
                        ||  pexit->u1.to_room == NULL
                        ||  pexit->u1.to_room->clan
                        ||  IS_SET(pexit->exit_info, EX_CLOSED)
                        || (IS_NPC(vch)
                            && IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
                    continue;
         
                 /* should stop combat only if you can move the person */
	         stop_fighting(vch,TRUE);
                 move_char(vch, door, FALSE );
              }
           }
           continue;
        }
        if ( vch->in_room->area == ch->in_room->area )
           send_to_char( "The rumbling of water echos in the distance.\n\r", vch );
     }
     return;
}

void spell_enhance(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{                                                                               
    CHAR_DATA *victim = (CHAR_DATA *) vo;                                       
    AFFECT_DATA af;                                                             
    char  buf[MAX_STRING_LENGTH];
                                                                                
    if ( is_affected(victim,sn) )
    {
	send_to_char("You improve your self.\n\r",ch);
	reup_affect(victim,sn,level/3,level);
        return;                                                                 
    }                                                                           
                                                                                
    switch(number_range(1,3))
    {
	case 1:	af.location = APPLY_CON;
	sprintf(buf,"constitution.\n\r"); break;
	case 2:	af.location = APPLY_STR; 
	sprintf(buf,"strength.\n\r"); break;
	case 3:	af.location = APPLY_DEX; 
	sprintf(buf,"dexterity.\n\r"); break;
	default: af.location = APPLY_NONE;
	sprintf(buf,"self.\n\r"); break;
    }

    af.where            = TO_AFFECTS;                                           
    af.type             = sn;                                                   
    af.level            = level;                                                
    af.duration         = ch->level/3;
    af.modifier         = 1 + (number_percent() < get_skill(ch,sn));
    af.bitvector        = 0;                                                    
    affect_to_char(victim,&af);

    damage(ch,ch,level,0,DAM_MENTAL, FALSE,FALSE);
    send_to_char("You improve your ",ch);
    send_to_char(buf,ch);

  return;
}

void spell_flame_shield(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( is_affected(victim,skill_lookup("flame shield")) ||
	 is_affected(victim,skill_lookup("frost shield")) ||
	 is_affected(victim,skill_lookup("electric shield")))
    {
	if ( victim == ch )
  send_to_char("You're already protected with an element shield.\n\r",ch);
	else
	act("$N is already protected by an element shield.",
		ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    /* If you are vuln fire you need to make a saving throw for this
     * to work and either way you take damage from it.
     */
    if ( IS_SET(ch->vuln_flags,VULN_FIRE) )
    {
	if ( !saves_spell(level,ch,DAM_FIRE) )
	{
		send_to_char("You failed.\n\r",ch);
		return;
	}

        ch->hit = UMAX( 1, ch->hit - dice(5,10) );
    }

    af.where    		= 0;
    af.type			= sn;
    af.level			= level;
    af.modifier			= 0;
    af.location			= 0;
    af.duration			= level / 5;
    af.bitvector		= 0;
    affect_to_char(victim,&af);

    if( victim == ch )
	send_to_char("A flame shield flickers around you.\n\r",ch);
    else
    {
	act("A flame shield flickers around $N.",ch,NULL,victim,TO_CHAR,FALSE);
	act("A flame shield flickers around you.",ch,NULL,victim,TO_VICT,FALSE);
    }

    act("A flame shield flickers around $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
    return;
} 

void spell_frost_shield(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( is_affected(victim,skill_lookup("flame shield")) ||
	 is_affected(victim,skill_lookup("frost shield")) ||
	 is_affected(victim,skill_lookup("electric shield")))
    {
	if ( victim == ch )
  send_to_char("You're already protected with an element shield.\n\r",ch);
	else
  act("$N is already protected by an element shield.",
	ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    /* If you are vuln cold you need to make a saving throw for this
     * to work and either way you take damage from it.
     */
    if ( IS_SET(ch->vuln_flags,VULN_COLD) )
    {
	if ( !saves_spell(level,ch,DAM_COLD) )
	{
		send_to_char("You failed.\n\r",ch);
		return;
	}

        ch->hit = UMAX( 1, ch->hit - dice(5,10) );
    }

    af.where    		= 0;
    af.type			= sn;
    af.level			= level;
    af.modifier			= 0;
    af.location			= 0;
    af.duration			= level / 5;
    af.bitvector		= 0;
    affect_to_char(victim,&af);

    send_to_char("A frost shield freezes around you.\n\r",ch);
    act("A frost shield freezes around $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
    return;
} 


void spell_electric_shield(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( is_affected(victim,skill_lookup("flame shield")) ||
	 is_affected(victim,skill_lookup("frost shield")) ||
	 is_affected(victim,skill_lookup("electric shield")))
    {
	if ( victim == ch )
  send_to_char("You're already protected with an element shield.\n\r",ch);
	else
  act("$N is already protected by an element shield.",
	ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    /* If you are vuln lightning you need to make a saving throw for this
     * to work and either way you take damage from it.
     */
    if ( IS_SET(ch->vuln_flags,VULN_LIGHTNING) )
    {
	if ( !saves_spell(level,ch,DAM_LIGHTNING) )
	{
		send_to_char("You failed.\n\r",ch);
		return;
	}

        ch->hit = UMAX( 1, ch->hit - dice(5,10) );
    }

    af.where    		= 0;
    af.type			= sn;
    af.level			= level;
    af.modifier			= 0;
    af.location			= 0;
    af.duration			= level / 5;
    af.bitvector		= 0;
    affect_to_char(victim,&af);

    send_to_char("An electric shield energizes around you.\n\r",ch);
    act("An electric shield energizes around $N.",
	ch,NULL,victim,TO_NOTVICT,FALSE);
    return;
} 

void spell_confusion(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf[MAX_STRING_LENGTH];

    level = UMAX(level, ch->level);
    
    if (IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
    {
      send_to_char("You have been sanctioned from using this skill.\n\r", ch);
      return;
    }

    if (is_affected(victim, skill_lookup("confusion")))
      {
      send_to_char("They are already confused.\n\r", ch);
      return;
      }

    if ( saves_spell(level,victim,DAM_MENTAL) )
       {
	 send_to_char("You failed.\n\r", ch);
	 return;
       }

       af.where                    = 0;
       af.type                     = sn;
       af.level                    = level;
       af.modifier                 = 0;
       af.location                 = 0;
       af.duration                 = level / 9;
       af.bitvector                = 0;
       affect_to_char(victim,&af);

       send_to_char( "The ground spins beneath your feet.\n\r", victim);
       act("$n appears to be confused.",victim,NULL,NULL,TO_ROOM,FALSE);
    /* Gladiator Spectator Channel */ 
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s confuses the hell out of %s, they are walking in circles!",ch->name,victim->name);
       gladiator_talk(buf);
    }
       return;
}

void spell_cuffs_of_justice(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf[MAX_STRING_LENGTH];
    send_to_char("Sorry, that isn't a spell anymore.\n\r",ch);
    return;
    level = UMAX(level, ch->level);
    if (IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
    {
      send_to_char("You have been sanctioned from using this skill.\n\r", ch);
      return;
    }
    if (is_affected(victim, skill_lookup("cuffs of justice" )))
      {
      send_to_char("They are already bound by justice.\n\r", ch);
      return;
      }
    if ( saves_spell(level,victim,DAM_MENTAL) )
       {
         send_to_char("You failed.\n\r", ch);
         return;
       }
       af.where                    = 0;
       af.type                     = sn;
       af.level                    = level;
       af.modifier                 = -1 * (get_curr_stat(victim,STAT_DEX)/ 4);
       af.location                 = APPLY_DEX;
       af.duration                 = level / 9;
       af.bitvector                = 0;
       affect_to_char(victim,&af);
       affect_to_char(ch,&af);
       send_to_char("You put the smackdown on them.\r\n",ch);
       act( "$n screams \"Respect my authority!\" and puts the smack down on $N.", ch, NULL, victim,TO_ROOM,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s makes  %s respect his authorita!",ch->name,victim->name);
       gladiator_talk(buf);
    }
       return;
}

void spell_restrain(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    AFFECT_DATA af;
    CHAR_DATA *victim = (CHAR_DATA *) vo;
   char buf[MAX_STRING_LENGTH];
    level = UMAX(level, ch->level);
    if (is_affected(victim, skill_lookup("restrain" )))
      {
      send_to_char("They are already restrained.\n\r", ch);
      return;
      }
    if ( saves_spell(level,victim,DAM_MENTAL) )
       {
         send_to_char("You failed.\n\r", ch);
         return;
       }
       af.where                    = 0;
       af.type                     = sn;
       af.level                    = level;
       af.modifier                 = -1 * (get_curr_stat(victim,STAT_DEX)/ 4);
       af.location                 = APPLY_DEX;
       af.duration                 = level / 9;
       af.bitvector                = 0;
       affect_to_char(victim,&af);
       affect_to_char(ch,&af);
       send_to_char("You put the smackdown on them.\r\n",ch);
       act( "$n screams \"Respect my authority!\" and puts the smack down on $N.", ch, NULL, victim,TO_ROOM,FALSE);
    /* Gladiator Spectator Channel */
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s makes  %s respect his authorita!",ch->name,victim->name);
       gladiator_talk(buf);
    }
       return;
}





void spell_nether_shield(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{

  OBJ_DATA *shield, *wShield;

  wShield = get_eq_char(ch, WEAR_SHIELD);

  if (wShield != NULL)
    {
    send_to_char("You must remove your current shield first.\n\r", ch);
    return;
    }

  if (!HAS_KIT(ch, "nethermancer"))
    {
    send_to_char("Only nethermancers may create a nether shield.\n\r", ch);
    return;
    }
  shield = create_object(get_obj_index(OBJ_VNUM_NETHER_SHIELD), 0, FALSE);

  shield->timer = ch->level  - number_range(0,ch->level /3);

  act("$n has created a shield from the nether plane.\n\r",
	ch, NULL, NULL, TO_ROOM,FALSE);
  send_to_char("You create a shield from the nether plane.\n\r", ch);
  obj_to_char(shield, ch);
  wear_obj(ch, shield, TRUE);

  return;

}

void spell_cure_vision(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    send_to_char("Sorry, this isn't a spell anymore\n\r",ch);
    return;

    if (victim->clan != ch->clan)
    {
       send_to_char("They are not in your clan.\n\r",ch);
       return;
    }

    if (IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
    {
      send_to_char("You have been sanctioned from using this skill.\n\r", ch);
      return;
    }

    if ( !is_affected( victim, gsn_blindness ) )
    {
        if (victim == ch)
          send_to_char("You aren't blind.\n\r",ch);
        else
          act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }
 /* Everyone casts at their actual level or casting level, which ever is higher */
    level = UMAX(ch->level, level);

    if (check_dispel(level,victim,gsn_blindness))
    {
        send_to_char( "Your vision returns!\n\r", victim );
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}


void spell_indulgence(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{

 AFFECT_DATA af;
 CHAR_DATA *victim = (CHAR_DATA * )vo;

 if ( is_affected(victim,sn)) 
 {
  send_to_char("You have been granted an indulgence to kill as you choose.\n\r", ch);
  reup_affect(victim,sn,level,level);
  return;
 }

 
       af.where                    = 0;
       af.type                     = sn;
       af.level                    = level;
       af.modifier                 = 0;
       af.location                 = 0;
       af.duration                 = level;
       af.bitvector                = 0;
       affect_to_char(victim,&af);

  send_to_char("You have been granted an indulgence to kill as you choose.\n\r", ch);

  return;
}
/*19SEP00 - Start of the charm_animal spell - Boogums */
void spell_charm_animal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  CHAR_DATA *victim_group = (CHAR_DATA *) vo;
  AFFECT_DATA af;
  int count=0;

  if ( is_safe(ch,victim) )
  {
    return;
  }
  if ( victim->form != NULL && !IS_SET(victim->form,FORM_ANIMAL) )
  {
    if ( victim == ch )
    {
      send_to_char("Grrrowl, you are QUITE the animal aren't you?\n\r",ch);
      return;
    }
    if (!IS_NPC(victim))
    {
    send_to_char("They might be weird but they aren't an animal.\n\r",ch);
    return;
    }

    send_to_char("You can only cast charm animal on animals.\n\r", ch);
    return;
  }


  for ( victim_group = char_list; victim_group != NULL; victim_group = victim_group->next )
  if ( (is_same_group( victim_group, ch ) && IS_SET(victim_group->form,FORM_ANIMAL))
    || (is_same_group(victim_group->master,ch) && IS_SET(victim_group->form,FORM_ANIMAL)))
  {
    count++;
  }
  if ( count >= level/5 && !IS_IMMORTAL(ch))
  {
    send_to_char("You lack the experience to control any more animals.\n\r",ch);
   return;
  }
  
  if ( IS_SET(victim->act, ACT_AGGRESSIVE) 
  ||   IS_AFFECTED(victim, AFF_CHARM)
  ||   IS_AFFECTED(ch, AFF_CHARM)
  ||   level < victim->level
  ||   victim->position == POS_FIGHTING) 
  {
    send_to_char("Sorry Charlie, not on that target.\n\r",ch);
    return;
  }

  switch(check_immune(victim,DAM_MENTAL))
  {
    case IS_IMMUNE:   level = (IS_NPC(victim)?1:0);  break;
    case IS_RESISTANT:  level = (IS_NPC(victim)?level/2:1);  break;
    case IS_VULNERABLE: level = (IS_NPC(victim)?level+4:2);  break;
    default:            level = (IS_NPC(victim)?level:1);break;
  }

  if ( victim->master )
  {
    stop_follower( victim );
  }
  add_follower( victim,ch );
  //victim->leader = ch;
  add_to_group(victim, ch);
  af.where       = TO_AFFECTS;
  af.type        = sn;
  af.level       = level;
  af.duration    = IS_NPC(victim)?number_fuzzy(level/2):level*1.5;
  af.location    = 0;
  af.modifier    = 0;
  af.bitvector   = AFF_CHARM;
  affect_to_char( victim, &af );

  if ( ch != victim )
  {
    act("You have quelled the beast within $N.",ch,NULL,victim,TO_CHAR,FALSE);
  }
  return;
}/*end brace for spell_charm_animal */

/* 07OCT00 - Start of spell_swarm by Boogums */
void spell_swarm( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{ /*Start Brace of spell_swarm */
  CHAR_DATA *victim = (CHAR_DATA *) vo;
  char buf  [MAX_STRING_LENGTH];
  int dam,hp_damage,dice_damage;
  int casters_hp;
  int damage_attack = 0;
  AFFECT_DATA af;

  casters_hp = UMAX( 10, ch->hit );
  hp_damage = number_range( casters_hp/9+1, casters_hp/5 );
  dice_damage = dice(level,15);
    
  dam = UMAX(hp_damage + dice_damage /10, dice_damage + hp_damage /10);

  /* Gladiator Spectator Channel */
  if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
      sprintf(buf,"%s calls forth nature's wrath on %s, and boy is she pissed.",
      ch->name,victim->name);
      gladiator_talk(buf);
    }

  switch (number_range(0,6) )
  { 
    case 1: /* Bees - Poison */
      act("BZZZZZZZZZZZZZZZ! It's Killer Bees!\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("BZZZZZZZZZZZZZZZ! Someone's stirred up a hornets nest.\r\n",ch,NULL,victim,TO_VICT,FALSE);
      send_to_char("Your Africanized wasps swarm about them.\r\n",ch);

      if (( saves_spell( level, victim,DAM_POISON) || is_affected(victim,sn) )
	 ||(IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
      {
	damage_attack = 1;
	act("WASPS!!! RUN!!!",victim,NULL,NULL,TO_ROOM,FALSE);
	send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
	return;
      }
      else 
      {
	af.where     = TO_AFFECTS;
	af.type      = sn;
	af.level     = level;
	af.duration  = level/4;
	af.location  = APPLY_STR;
	af.modifier  = -1;
	af.bitvector = AFF_POISON;
	affect_join( victim, &af );

	damage_attack=1;
	send_to_char( "DOH!  You're not feeling to well right now.\n\r", victim );
	act("$n looks very ill.",victim,NULL,NULL,TO_ROOM,FALSE);
      }
    break;
    case 0: /* Rats - Plague */
      act("Oh rats...\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("$n looks really freaky with those rats swarming out from behind them.\r\n",ch,NULL,victim,TO_VICT,FALSE);
      act("Pretty cool Willard, your rats attack!\r\n",ch,NULL,NULL,TO_CHAR,FALSE);
      if ( ( saves_spell(level,victim,DAM_DISEASE) || is_affected( victim, sn) ) ||
      (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
      {
        damage_attack = 1;
        if (ch == victim)
        {
	  send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
	}
        else
	{
          act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
	}
      }
      else
      {
        damage_attack = 1;
        af.where     = TO_AFFECTS;
        af.type     = sn;
        af.level    = level*3/4 ;
        af.duration  = level/4;
        af.location  = APPLY_STR;
        af.modifier  = -3;
        af.bitvector = AFF_PLAGUE;
        affect_join(victim,&af);
        send_to_char("You scream in agony as plague sores erupt from your skin.\n\r",victim);
        act("$n screams in agony as plague sores erupt from $s skin.",victim,NULL,NULL,TO_ROOM,FALSE);
      }
      break;

    case 2: /* Butterflies - calm */
      damage_attack = 0;
      act("$n opens a hole to the elemental plane of butterfly.\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("$n opens a hole to the elemental plane of butterfly.\r\n",ch,NULL,victim,TO_VICT,FALSE);
      act("You open a portal to the elemental plane of butterfly.\r\n",ch,NULL,NULL,TO_CHAR,FALSE);

      if ( ( saves_spell( level, victim,DAM_MENTAL) || is_affected( victim, sn) ) )
      {
        if (ch == victim)
        {
	  send_to_char("Awwww, those butterflies sure is pretty.\r\n",ch);
        }
        else
        {
	  send_to_char("Awwww, those butterflies sure are pretty.\r\n",victim);
        }
      }
      else
      {
	act("Grab your nets!!! $n is summoning butterflies!  Who wants to fight anymore?\r\n",ch,NULL,victim,TO_ROOM,FALSE);
        if (victim->fighting || victim->position == POS_FIGHTING)
        {
          stop_fighting(victim,FALSE);
          af.where = TO_AFFECTS;
          af.type = sn;
          af.level = level;
          af.duration = level/4;
          af.location = APPLY_HITROLL;
          if (!IS_NPC(victim))
          af.modifier = -5;
          else
          af.modifier = -2;
          af.bitvector = AFF_CALM;
          affect_to_char(victim,&af);
          af.location = APPLY_DAMROLL;
          affect_to_char(victim,&af);
        }
      }
      break;

    case 3: /* Spiders - hold person */
      damage_attack = 1;
      act("$n spews spiders from their fingertips.\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("$n spews spiders from their fingertips.",ch,NULL,victim,TO_VICT,FALSE);
      act("Spiders fly from your fingertips!\r\n",ch,NULL,NULL,TO_CHAR,FALSE);

      if ( ( is_affected( victim, sn) ) ||
      saves_spell(level,victim,DAM_OTHER) )
      {
      send_to_char("Come into my parlor...\r\n",victim);
      send_to_char("They dodge your spiders web.\r\n",ch);
      }
      else
      {
	damage_attack = 0;
        af.where            = TO_AFFECTS;
        af.type             = sn;
        af.level            = level;
        af.location         = APPLY_DEX;
        af.modifier         = ( 3 - get_curr_stat(victim,STAT_DEX) );
        af.duration         = 1 + (UMAX(0,level-51));
        af.bitvector        = 0;
        affect_to_char( victim, &af );
        victim->position = POS_RESTING;
        act("$N is wrapped up snug as a bug in a rug!",ch,NULL,victim,TO_NOTVICT,FALSE);
        send_to_char("The spiders wrap you snugly in their web.\n\r",victim);
        if ( victim != ch )
	{
          act("$N is wrapped in the spider's web..",ch,NULL,victim,TO_CHAR,FALSE);
	}
      }
      break;

    case 4: /* Ants - Stun affect */
      act("Small holes open in the ground and ants stream out.\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("*CRACK* The ground opens and ants pour out.",ch,NULL,victim,TO_VICT,FALSE);
      act("The whole room is filled with ants.\r\n",ch,NULL,NULL,TO_CHAR,FALSE);
      if ( saves_spell(level,victim,DAM_MENTAL) )
      {
        damage_attack = 1;
      }
      else
      {
        send_to_char("Ants!  Ants with stingers! Ouch!\r\n.",victim);
	act("$n is stunned by the ants.\r\n",victim,NULL,NULL,TO_ROOM,FALSE);
	damage_attack = 1;
        DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
        victim->position = POS_RESTING;
      }
      break;


    case 5: /* Locust - blindness */
      act("A swarm of locusts comes from the sky.\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("A swarm of locusts comes from the sky.\r\n",ch,NULL,victim,TO_VICT,FALSE);
      act("The whole room is filled with locusts.\r\n",ch,NULL,NULL,TO_CHAR,FALSE);

      if (is_affected(victim,sn) || saves_spell(level,victim,DAM_OTHER) )
      {
        damage_attack = 1;
	act("$n dodges the locusts.\r\n",victim,NULL,NULL,TO_ROOM,FALSE);
	send_to_char("You dodge the locusts.\r\n", victim );
      }
      else
      {
	damage_attack = 1;

        af.where     = TO_AFFECTS;
	af.type      = sn;
        af.level     = level;
	af.location  = APPLY_HITROLL;
        af.modifier  = -4;
        af.duration  = 1+level/4;
	af.bitvector = AFF_BLIND;
        if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
           af.duration  = 1;
	affect_to_char( victim, &af );
	send_to_char( "You are {Gblinded{x by the locusts!\n\r", victim );
	act("$n appears to be {Gblinded{x by the locusts.",victim,NULL,NULL,TO_ROOM,FALSE);
      }
    break;

    case 6: /* Fireflys - feeblemind */
      damage_attack = 0;
      act("$n fills the room with fireflies.\r\n",ch,NULL,victim,TO_NOTVICT,FALSE);
      act("$n fills the room with fireflies.",ch,NULL,victim,TO_VICT,FALSE);
      act("The whole room is filled with fireflies.\r\n",ch,NULL,NULL,TO_CHAR,FALSE);
      if ( is_affected(victim,sn ) || saves_spell(level,victim,DAM_MENTAL) )
      {
        send_to_char("The fireflies you called fly away.\n\r",ch);
	send_to_char("You were distracted momentarily by the firefiles.\n\r",victim);
      }
      else
      {
        af.where            = TO_AFFECTS;
        af.type             = sn;
        af.level            = level;
        af.duration         = (level/12);
        af.modifier         = -1 * (level/8);
        af.location         = APPLY_INT;
        af.bitvector        = 0;
        affect_to_char( victim, &af );
       
        send_to_char("Oh the designs!  They are so simple yet so beautiful!  Those fireflies sure are smart.\n\r",victim);
        if ( victim != ch )
        {
	  act("$N's willpower fades away.",ch,NULL,victim,TO_CHAR,FALSE);
        }
      }
      break;
    default:
    break;
    }/*end brace for case*/

/*hit em hard damage taken from firebreath spell */

  if(damage_attack == 1)
  {
    
        if (saves_spell(level,victim,DAM_HARM))
        {
          damage( ch, victim, dam/2, sn, DAM_HARM,TRUE ,TRUE);
        }
        else
        {
          damage( ch, victim, dam, sn, DAM_HARM,TRUE ,TRUE);
        }
  }
  return;

} /*End Brace of spell_swarm */

void spell_wraithform( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
  CHAR_DATA *victim = (CHAR_DATA *) vo;

  if ( is_affected( ch, sn ))
  {
    if (victim == ch)
    send_to_char("You are already in wraith form.",ch);
    return;
  }
     if(IS_SET(ch->mhs,MHS_GLADIATOR))
	  {
	       send_to_char("A ghost in the gladiators? Tune in next week for The Undead Wars.\n\r",ch);
		    return;
			 }

  /* Set timer */
  if ( !IS_NPC(victim) )
  {
     victim->pcdata->wraith_timer = 48;
     act("$n begins to fade into shadow...",victim,NULL,NULL,TO_ROOM,FALSE);
     send_to_char("You begin to fade into wraithform...\n\r",victim);
  }
 
  return;
}

void action_wraithform( CHAR_DATA *victim, char *arg )
{ 
    AFFECT_DATA af;

  if ( IS_NPC(victim) )
	return;

  af.where     = TO_AFFECTS;
  af.type      = skill_lookup("wraithform");
  af.level     = victim->level;
  af.duration  = victim->level *3/4;
  af.location  = 0;
  af.modifier  = 0;
  af.bitvector = 0;
  affect_to_char( victim, &af );
  act( "$n becomes very shadowy.", victim, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char( "You become very wraithlike.\n\r", victim );
  return;
}
void action_zealot_convert( CHAR_DATA *victim,char *argh)
{
  if ( IS_NPC(victim) )
  { 
    return;
  }
  victim->pcdata->deity = deity_lookup("almighty");
  victim->pcdata->rank = 0;
}


/* Support function for wraith form */
void spell_make_bag( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    static char *headers[] = { "corpse of the ", "corpse of The ",
			       "corpse of an ", "corpse of An ",
   			       "corpse of a ", "corpse of A ",
			       "the corpse of ",
			       "corpse of " }; // (This one must be last)
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    int i;

    if ( obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
    {
       send_to_char("That is not a corpse.\n\r",ch);
	return;
    }

    if (obj->contains != NULL)
    {
       send_to_char("You must empty out the corpse first.\n\r",ch);
       return;
    }

    for (i = 0; i < 7; i++)
    {
       int len = strlen(headers[i]);
       if ( memcmp(obj->short_descr, headers[i], len) == 0 )
       {
          sprintf( buf, "bag beta %s", obj->short_descr+len );
/*   	  free_string( obj->name );
*/
	  obj->name = str_dup(buf);

          sprintf( buf, "A bag of fine %s hide catches your eye.  ",
                   obj->short_descr+len );
/*	  free_string( obj->description );
*/
          obj->description = str_dup( buf );

          sprintf( buf, "a bag made from %s's hide", obj->short_descr+len );
/*          free_string( obj->short_descr );
*/
	  obj->short_descr = str_dup( buf );

          break;
       }
    }
    /* remove old weight of item */ 
    ch->carry_weight  -= get_obj_weight( obj );
    ch->carry_number -= get_obj_number( obj );

    obj->item_type = ITEM_CONTAINER;
    obj->wear_flags = ITEM_HOLD|ITEM_TAKE;
    obj->timer = 0;
    obj->weight = 5;
    obj->level = level/3;
    obj->cost = level * 50;
    obj->value[0] = level * 10;                 /* Weight capacity */
    obj->value[1] = 1;                          /* Closeable */
    obj->value[2] = -1;                         /* No key needed */
    obj->value[3] = 90;			/* maximum weight */
    obj->value[4] = 25;    			/* weight % multiplier */
    obj->pIndexData = get_obj_index( 1258 );    /* So it's not a corpse */

    act( "Your new $p looks pretty snazzy.", ch, obj, NULL, TO_CHAR,FALSE );
    act( "$n's new $p looks pretty snazzy.", ch, obj, NULL, TO_ROOM,FALSE );

    clear_string(&obj->owner, NULL);
    REMOVE_BIT(obj->extra_flags,ITEM_CLAN_CORPSE);

    /* add new weight of item */
    ch->carry_weight  += get_obj_weight( obj );
    ch->carry_number  += get_obj_number( obj );

    send_to_char( "Ok.\n\r", ch );
    return;
}

/*void spell_annointment(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{    
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    send_to_char("Sorry, this spell doesn't exist anymore.\n\r",ch);
    return;

    if (IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
    {
        send_to_char("You have been sanctioned from using this skill.\n\r",
          ch);
        return;
    }

    if ( is_affected(victim,sn) || ch->clan != clan_lookup("zealot") )
    {
        send_to_char("You failed.\n\r",ch);
        return;   
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = (level / 5);
    af.modifier         = 0;
    af.location         = APPLY_NONE;
    af.bitvector        = 0;  
    affect_to_char( victim, &af );

    send_to_char("You gain the annointment of The Almighty.\n\r",victim);
    return;
}*/

void spell_aura_of_valor(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    send_to_char("Sorry, this isn't a spell anymore.\n\r",ch);
    return;

    if ( IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1) )
    {
        send_to_char("You have been sanctioned from using this skill.\n\r",ch);
        return;
    }

    if ( ch->clan != clan_lookup("honor") )
    {
        send_to_char("You are not a member of Honor.\n\r",ch);
        return;
    } 

    if ( is_affected(victim,sn) )
    {
        send_to_char("You are already surrounded by an aura of valor.\n\r",victim);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = (victim->level / 2);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    send_to_char("You are surrounded by an aura of valor./n/r",victim);
    return;
}

void spell_spirit_phoenix(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *feather;
    AFFECT_DATA af;

    feather = get_eq_char(ch,WEAR_HOLD);
    if ( !IS_IMMORTAL(ch) && (feather == NULL || feather->pIndexData->vnum != OBJ_VNUM_FEATHER) )
    {
	send_to_char("You lack the proper material component.\n\r",ch);
	return;
    }

    if (feather != NULL &&  feather->pIndexData->vnum == OBJ_VNUM_FEATHER && number_percent( ) < 25 )
    {
        act("$p whithers away to ashes.",ch,feather,NULL,TO_CHAR,FALSE);
        extract_obj(feather);
	/* It's possible to save the ashes */
	if ( number_percent() < level / 5 )
	{
	    OBJ_DATA *ash;

	    ash = create_object( get_obj_index( OBJ_VNUM_ASHES ), 0, FALSE );
	    obj_to_char( ash, ch );
	}
    }

    if ( is_affected( victim, sn ) )
    {
	if ( victim == ch )
	    send_to_char("You are already gifted with the spirit.\n\r",ch);
	else
	    send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level / 3;
    af.modifier		= 0;
    af.location		= 0;
    af.bitvector	= (AFF_SANCTUARY | AFF_WITHSTAND_DEATH);
    affect_to_char( victim, &af );

    /* Now the cool part.  If this is cast in a superior magelab at midnight,
     * the feather becomes an amulet.
     * No. Seriously. ~Andaron
     */
/*    if ( ch->in_room->sector_type == SECT_MAGELAB_SUPERIOR && time_info.hour == 0 && ch == victim )
    {
	OBJ_DATA *amulet;
 
	amulet = create_object( get_obj_index( OBJ_VNUM_AMULET ), 0, FALSE );
	amulet->level = UMAX(level,ch->level);
	amulet->level = UMIN(amulet->level,51);
	act("$p materializes in your hands!",ch,amulet,NULL,TO_CHAR,FALSE);
	act("$p materializes in $n's hands!",ch,amulet,NULL,TO_ROOM,FALSE);
	obj_to_char( amulet, ch );
    }*/

    return;
}

void spell_flameseek(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{

	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;

    	if ( victim == ch )
  	{
	send_to_char("If you desire the flames that much, cast fireball on yourself!\n\r", ch);
	return;
	}

	if ( IS_SET(victim->vuln_flags, VULN_FIRE))
	{
	send_to_char("They already desire oneness with the flame.\n\r", ch);
	return;
	}

    	if ( saves_spell(IS_SET(ch->mhs, MHS_ELEMENTAL) ? level + 5 : level,victim,DAM_FIRE) )
       	{
	 send_to_char("You failed.\n\r", ch);
	 return;
        }

    	af.where		= TO_VULN;
    	af.type			= sn;
    	af.level		= level;
    	af.duration		= level /11; 
    	af.modifier		= 0;
    	af.location		= 0;
    	af.bitvector		= VULN_FIRE; 
    	affect_to_char( victim, &af );
	
	act("You suddenly feel drawn to the flames.",ch,NULL,victim,TO_VICT,FALSE);
	act("$N suddenly feels drawn to the flames.",ch,NULL,victim,TO_CHAR,FALSE);

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

	return;
}

void spell_wall_of_fire( int sn, int level, CHAR_DATA *ch, void *vo, int traget)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	AFFECT_DATA af;
	OBJ_DATA *wall;
	int dam;

	dam = dice(level, (IS_SET(ch->mhs, MHS_ELEMENTAL) ? 15 : 10));

	if ( victim == ch)
	{
	send_to_char("You may not cast this spell on yourself.\n\r", ch);
	return;
	}

    	if ( saves_spell(IS_SET(ch->mhs, MHS_ELEMENTAL) ? level + 5 : level,victim,DAM_FIRE) )
	{
	send_to_char("You failed.\n\r", ch);
	return;
	}

	wall = create_object(get_obj_index(OBJ_VNUM_WALL_FIRE),0,FALSE);
        obj_to_room(wall, ch->in_room);
 	damage( ch, victim, dam, sn,DAM_FIRE,TRUE,TRUE);
 
   	af.where = TO_AFFECTS;
    	af.type = gsn_wall_fire;
    	af.level = level;
    	af.location = 0;
    	af.duration = level * 2 / 3;
        wall->timer = level * 2 / 3;
    	af.modifier = 0; 
    	af.bitvector = 0;
	af.caster_id = ch->id;

    	affect_to_room( ch->in_room, &af );

	return;
}

/* Like an acid blast, but a little less damage, and some DOT */	
void spell_boiling_blood( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level+2,victim,DAM_DISEASE) || is_affected(victim,sn) )
    {
	send_to_char("Nothing seems to happen.\n\r",ch);
	return;
    }

    dam = dice(level,9); /* Acid blast is level d 12 */
    /* It worked */
    damage( ch, victim, dam, sn, DAM_DISEASE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
	return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12; 	/* 2 minutes of damage */
    af.location = 52;
    af.modifier = 52; /* 624/312/156 */
    af.level	= level;
    af.type	= sn;
    af.bitvector	= DAM_DISEASE;
    af.caster_id	= ch->id;

    affect_to_char( victim, &af );

    send_to_char("You scream in agony as your blood begins to boil!\n\r",victim);
    act("$n screams in unimaginable pain as $s skin blisters and $s blood boils.",
		victim,NULL,NULL,TO_ROOM,FALSE);

    return;
}

/* Level 25 DOT. Does damage comprable to fireball up front, plus
  more over 12 dot pulses */
void spell_blight( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level+2,victim,DAM_DISEASE) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        return;
    }

    
    dam = number_range( level * 2, level * 4 );
    /* It worked */
    damage( ch, victim, dam, sn, DAM_DISEASE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
	return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 2 minutes of damage */
    af.location = 40;
    af.modifier = 40; /* 480/240/120 */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_DISEASE;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    send_to_char("Your heart constricts with undescribable pain!\n\r",victim);
    act("$n doubles over in pain as $s heart constricts with the Blight!",
                victim,NULL,NULL,TO_ROOM,FALSE);

    return;
}

/* Level 18 DOT */
void spell_contagion( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level+2,victim,DAM_DISEASE) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        return;
    }

    dam = number_range( 3 * level / 2 , level * 4 );
    /* It worked */
    damage( ch, victim, dam, sn, DAM_DISEASE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
	return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 2 minutes of damage */
    af.location = 23;
    af.modifier = 23; /* 276, 138, 69 if caster not there */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_DISEASE;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    af.where	= TO_AFFECTS;
    af.duration	= 4;
    af.location	= APPLY_STR;
    af.modifier = -1 * (level / 10);
    af.bitvector	= 0;
    affect_to_char( victim, &af );

    send_to_char("You feel sick and weak.\n\r",victim);
    act("$n stumbles about with the contagion.",
                victim,NULL,NULL,TO_ROOM,FALSE);

    return;
}


void spell_scourge( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
     AFFECT_DATA af;
     int dam;

    if ( saves_spell(level+2,victim,DAM_DISEASE) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        return;
    }

    dam = number_range( 5 * level / 2 , level * 5 );
    /* It worked */
    damage( ch, victim, dam, sn, DAM_DISEASE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
	return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 3 minutes of damage */
    af.location = 64;
    af.modifier = 64;
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_DISEASE;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    send_to_char("You begin to sweat as you contract the Scourge.\n\r",victim);
    act("$n begins to sweat as $e contracts the Scourge.",
                victim,NULL,NULL,TO_ROOM,FALSE);
 
    return;
}
void spell_blade_barrier(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	AFFECT_DATA af;


	af.where = TO_AFFECTS;
	af.duration = level;
	af.location = 0;
	af.modifier = 0;
	af.level = level;
	af.type = sn;
	af.bitvector = DAM_SLASH;
	
	affect_to_char(ch, &af);

	act("$n surrounds $eself with a barrier of spinning blades summoned from the Earth itself!\n\r", ch, ch,  NULL, TO_ROOM, FALSE);
	send_to_char("You call forth a barrier of blades from the earth.\n\r", ch);

	return;
}

void spell_thunderclap(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    AFFECT_DATA 	af;
    int		dam;

    for ( vch = victim->in_room->people ; vch != NULL ; vch = vch_next )
    {
	vch_next = vch->next_in_room;

	if ( is_safe_spell( ch, vch, TRUE , sn)|| is_affected(vch,sn)
		|| is_same_group(ch, vch) )
		continue;

  	/* Hit 'em */
	dam = number_range( level, level * 2 );
	if ( victim == vch )
	    dam = 3 * dam / 2;

	/* Put the spell affect on first.  If they die, it'll just get removed */
	af.where		= DAMAGE_OVER_TIME;
	af.level		= level;
	af.duration		= 9; /* 90 seconds */
	af.type			= sn;
	af.location		= level / 2;
	af.modifier		= 3 * level / 2;
	af.bitvector		= DAM_SOUND;
	af.caster_id		= ch->id;

	affect_to_char( vch, &af );
	send_to_char("The sound of the thunderclap rings in your ears.\n\r",vch);
	damage( ch, vch, dam, sn, DAM_SOUND, TRUE,TRUE);
    }
    
    return;
}

void spell_venom_of_vhan(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level,victim,DAM_POISON) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
 	return;
    }

     dam = number_range( level * 3, level * 6 );
     damage( ch, victim, dam, sn, DAM_POISON,TRUE,TRUE );

     if ( ch->fighting != victim )
 	return;

     af.where = DAMAGE_OVER_TIME;
     af.duration = 18;   /* 3 minutes of damage */
     af.location = level * 2;    if ( IS_NPC(victim) ); af.location *= 2;
     af.modifier = level * 5;	 if ( IS_NPC(victim) ); af.modifier *= 2;
     af.level    = level;
     af.type     = sn;
     af.bitvector= DAM_POISON;
     af.caster_id= ch->id;

     affect_to_char( victim, &af );

    send_to_char("Your blood turns ice cold as the venom begins to circulate.\n\r",victim);
    act("$n breaks into a cold sweat as venom enters $s veins.",victim,NULL,NULL,TO_ROOM,FALSE);

    return;
} 

void spell_remove_disease(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *paf;

    /* Find the DOT */
    for ( paf = victim->affected ; paf != NULL ; paf = paf->next )
    	if ( paf->where == DAMAGE_OVER_TIME && paf->bitvector == DAM_DISEASE )
	    break;

    if ( paf == NULL )
    {
	if ( victim == ch )
	    send_to_char("You aren't diseased.\n\r",ch);
	else
	    act("$N isn't diseased.",ch,NULL,victim,TO_CHAR,FALSE);
	return;
    }

    /** Tke down the level **/
    paf->level -= level / 2;
    if ( paf->level < 1 )
    {
	act("$n sighs in relief as the disease fades.",victim,NULL,NULL,TO_ROOM,FALSE);
	act("You sigh in relief as the disease fades.",victim,NULL,NULL,TO_CHAR,FALSE);
	affect_remove( victim, paf , APPLY_BOTH);
    }
    else
    {
	act("$n regains some strength as the disease weakens.",victim,NULL,NULL,TO_ROOM,FALSE);
	act("You regain some strength as the disease weakens.",victim,NULL,NULL,TO_CHAR,FALSE);
    }

    return;
}

void spell_remove_poison(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA *paf;

    /* Find the DOT */
    for ( paf = victim->affected ; paf != NULL ; paf = paf->next )
        if ( paf->where == DAMAGE_OVER_TIME && paf->bitvector == DAM_POISON )
            break;

    if ( paf == NULL )
    {
        if ( victim == ch )
            send_to_char("You aren't poisoned.\n\r",ch);
        else
            act("$N isn't poisoned.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }

    /** Tke down the level **/
    paf->level -= level / 2;
    if ( paf->level < 1 )
    {
        act("$n sighs in relief as the poison fades.",victim,NULL,NULL,TO_ROOM,FALSE);
        act("You sigh in relief as the poison fades.",victim,NULL,NULL,TO_CHAR,FALSE);
        affect_remove( victim, paf , APPLY_BOTH );
    }
    else
    {
        act("$n regains some strength as the poison weakens.",victim,NULL,NULL,TO_ROOM,FALSE);
        act("You regain some strength as the poison weakens.",victim,NULL,NULL,TO_CHAR,FALSE);
    }

    return;
}

void spell_frostbite(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level,victim,DAM_COLD) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        return;
    }

    dam = dice(level,9); /* Acid blast is level d 12 */
    /* It worked */
    damage( ch, victim, dam, sn, DAM_COLD,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
        return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 2 minutes of damage */
    af.location = 3 * level / 2;
    af.modifier = 3 * level;      /* 75 to 150! */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_COLD;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    act("Your skin frosts over as a numbing coldness envelops you.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n's skin frosts over as a numbing coldness evenlops $m.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_blistering_skin(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level,victim,DAM_FIRE) || is_affected(victim,sn) )
    {
       send_to_char("Nothing seems to happen.\n\r",ch);
       return;
    }

    dam = dice(level,9); /* Acid blast is level d 12 */
    /* It worked */
    damage( ch, victim, dam, sn, DAM_FIRE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
        return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 2 minutes of damage */
    af.location = 3 * level / 2;
    af.modifier = 3*level;      /* 50 to 100! */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_FIRE;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    act("Your skin blisters and peels as a heat wave envelops you.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n's skin blisters and peels as a heat wave evenlops $m.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_electrocution(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level,victim,DAM_ENERGY) || is_affected(victim,sn) )
    {
       send_to_char("Nothing seems to happen.\n\r",ch);
       return;
    }

    dam = dice(level,9); /* Acid blast is level d 12 */
    /* It worked */
    damage( ch, victim, dam, sn, DAM_ENERGY,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
        return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 12;   /* 2 minutes of damage */
    af.location = 3 * level / 2;
    af.modifier = 3 * level;      /* 50 to 100! */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_ENERGY;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    act("Your skin shivers as a shock of electricity courses through your body.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n shivers as a chock of electricity courses through $s body.",victim,NULL,NULL,TO_VICT,FALSE);
    return;
}

void spell_betray(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	CHAR_DATA *gch;
	

 	if ( victim == ch )
	{
	send_to_char("You cannot cast this spell on yourself.\n\r", ch);
	return;
	}

	if (IS_NPC(victim))
	{
	send_to_char("Not on that target.\n\r", ch);
	return;
	}
 
	if (!ch->fighting )
	{
	send_to_char("You can only cause betrayal during the confusion of combat.\n\r", ch);
	return;
	}

 	for ( gch = char_list; 	gch != NULL ; gch = gch->next )
	{
	
	/* OK, we need to find all the undead and elementals that are in the same group
	   as the victim.  Make sure that they are NPC's too.  */

	if ( (is_same_group(victim, gch) || is_same_group(victim->master, gch))  &&
	     (IS_SET(gch->mhs, MHS_ELEMENTAL) || IS_SET(gch->act, ACT_UNDEAD))  &&
	     (IS_NPC(gch)) && !saves_spell(level, gch, DAM_CHARM) )
	{
		if(gch->fighting != NULL)
			stop_fighting(gch, TRUE);
	
		if(gch->master)
		   stop_follower(gch);

		add_follower(gch, ch);
		gch->master = ch;
	 	
      		SET_BIT(gch->affected_by,AFF_CHARM); /* quick-charm */
			
		if(ch->fighting != NULL)
		{
			gch->fighting = ch->fighting;
		}
		else
		{
			ch->fighting = victim;
			gch->fighting = victim;
		}

		act("$n betrays you!\n\r", gch, NULL, NULL, TO_VICT, FALSE);
		act("$n betrays $N!\n\r", gch, NULL, victim, TO_NOTVICT, FALSE);
	}
	}
	return;
}

void spell_shield_of_faith(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level / 5;
    af.modifier		= 0;
    af.location		= 0;
    af.bitvector	= 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a shield of faith.\n\r",victim);
    act("$n is surrounded by a shield of faith.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_diamond_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || is_affected(victim,gsn_steel_skin) || is_affected(victim,gsn_stone_skin) 
	|| is_affected(victim,gsn_adamantite_skin) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level / 3;
    af.modifier         = -1 * get_curr_stat(victim,STAT_CON) * 3;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("Your skin turns as hard as diamond!\n\r",victim);
    act("$n's skin turns as hard as diamond!",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_steel_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || is_affected(victim,gsn_diamond_skin) || is_affected(victim,gsn_stone_skin)  
	|| is_affected(victim,gsn_adamantite_skin) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level / 5;
    af.modifier         = -1 * get_curr_stat(victim,STAT_CON) * 2;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("Your skin hardens to steel!\n\r",victim);
    act("$n's skin hardens to steel!",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_crushing_grip(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;
	int dam;

 	dam = dice(level, 12);

	if (saves_spell(level, victim, DAM_BASH))
		dam /= 2;
 	
	act("$n summons a hand of solid air that tries to squeeze the life out of you!\n\r", ch, NULL, victim, TO_VICT, FALSE);
	act("$n summons	a hand of solid air that tries to squeeze the life from $N!\n\r", ch, NULL, victim, TO_NOTVICT, FALSE);
        damage(ch,victim,dam,sn,DAM_BASH,TRUE,TRUE);

	return;
}

void spell_sunburst(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if(IS_SET(ch->in_room->room_affects, RAFF_SHADED))
    {
      send_to_char("The darkness swallows up your burst of light.\n\r", ch);
      return;
    }
    
    dam = dice(level,10);

    if( saves_spell(level,victim,DAM_LIGHT) )
	dam /= 2;

    damage(ch,victim,dam,sn,DAM_LIGHT,TRUE,TRUE);

    /* make sure damage didn't kill 'em.  if not, give a save */
    if ( ch->fighting != victim ||
	 is_affected(victim,sn) ||
	saves_spell(level,victim,DAM_LIGHT) )
       return;


    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= (dam%3); /* duration will be 0,1, or 2 */
    af.modifier		= -6;
    af.location		= APPLY_HITROLL;
    af.bitvector	= AFF_BLIND;
    if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
       af.duration  = 1;

    affect_to_char(victim,&af);

    act("You are blinded by the light!",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n is blinded by the light!",victim,NULL,NULL,TO_ROOM,FALSE);
    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_sonic_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    bool saved;
    int chance;

    dam = dice(level,9);
    
    if ( saves_spell(level,victim,DAM_SOUND) )
	dam /= 2;

    damage(ch,victim,dam,sn,DAM_SOUND,TRUE,TRUE);

    if ( ch->fighting != victim )
	return;

    /* Base 50% chance */
    chance = 50;

    /* best case: still at 50%.  Worst, at 25%.  Average will be 25-30% */
    chance -= get_curr_stat(victim,STAT_CON);

    if ( (saved = saves_spell(level,victim,DAM_SOUND)) )
	chance /= 2;

    if ( number_percent( ) < chance )
    {
	act("You are stunned by the sonic shockwave!",victim,NULL,NULL,TO_CHAR,FALSE);
	act("$n is stunned by the sonic shockwave!",victim,NULL,NULL,TO_ROOM,FALSE);
	if ( saved )
	    DAZE_STATE(victim,PULSE_VIOLENCE);
	else
	    DAZE_STATE(victim,PULSE_VIOLENCE*2);
    }
    return;
}

void spell_regeneration( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
    act("You feel full of vigor and health.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n appears vigorous and healthy.",victim,NULL,NULL,TO_ROOM,FALSE);
    reup_affect(victim,sn,24,level);
	return;
    }

    af.where		= DAMAGE_OVER_TIME; /* engine, not really damage */
	af.level	= level;
	af.type		= sn;
	af.modifier	= -1 * (level/5);
	af.location	= -1 * (level/3);
	af.duration	= 24;
  	af.bitvector	= 0;
   
    affect_to_char(victim,&af);
    act("You feel full of vigor and health.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n appears vigorous and healthy.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_adamantite_skin(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || is_affected(victim,gsn_steel_skin) || is_affected(victim,gsn_stone_skin) ||
	 is_affected(victim,gsn_diamond_skin) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level / 2;
    af.modifier         = -1 * get_curr_stat(victim,STAT_CON) * 4;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("Your skin becomes adamantite!\n\r",victim);
    act("$n's skin becomes adamantite!",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_incinerate(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

       dam = dice( level, 12 );

    if ( saves_spell( level, victim, DAM_FIRE ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_FIRE,TRUE,TRUE);
    return;
}

void spell_acclimate(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = DAMAGE_OVER_TIME;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/5; /* At 100hp/pulse, this is all-star! */
    af.modifier         = -100;
    af.location         = -100;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are more attuned to your environment.\n\r",victim);
    act("$n is more attuned to $s environment.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_symbol_1(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_symbol_1) ||
	 is_affected(victim,gsn_symbol_2) ||
	 is_affected(victim,gsn_symbol_3) ||
	 is_affected(victim,gsn_symbol_4) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if ( ch->gold < 500 )
    {
	send_to_char("You lack the appropriate sacrifice for your deity.\n\r",ch);
	return;
    }

    ch->gold -= 500;

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level*3;
    af.duration         = level*3;
    af.modifier         = 50;
    af.location         = APPLY_HIT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("An ancient symbol shines before your eyes.\n\r",victim);
    act("A symbol glows before $n.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_symbol_2(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_symbol_1) ||
         is_affected(victim,gsn_symbol_2) ||
         is_affected(victim,gsn_symbol_3) ||
         is_affected(victim,gsn_symbol_4) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->gold < 1000 )
    {
        send_to_char("You lack the appropriate sacrifice for your deity.\n\r",ch);
        return;
    }

    ch->gold -= 1000;

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level*3;
    af.duration         = level*3;
    af.modifier         = 100;
    af.location         = APPLY_HIT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("An ancient symbol shines before your eyes.\n\r",victim);
    act("A symbol glows before $n.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_symbol_3(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_symbol_1) ||
         is_affected(victim,gsn_symbol_2) ||
         is_affected(victim,gsn_symbol_3) ||
         is_affected(victim,gsn_symbol_4) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->gold < 2000 )
    {
        send_to_char("You lack the appropriate sacrifice for your deity.\n\r",ch);
        return;
    }

    ch->gold -= 2000;

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level*3;
    af.duration         = level*3;
    af.modifier         = 200;
    af.location         = APPLY_HIT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("An ancient symbol shines before your eyes.\n\r",victim);
    act("A symbol glows before $n.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_symbol_4(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_symbol_1) ||
         is_affected(victim,gsn_symbol_2) ||
         is_affected(victim,gsn_symbol_3) ||
         is_affected(victim,gsn_symbol_4) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->gold < 4000 )
    {
        send_to_char("You lack the appropriate sacrifice for your deity.\n\r",ch);
        return;
    }

    ch->gold -= 4000;

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level*3;
    af.duration         = level*3;
    af.modifier         = 400;
    af.location         = APPLY_HIT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("An ancient symbol shines before your eyes.\n\r",victim);
    act("A symbol glows before $n.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_spirit_of_bear(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->skill_points < (level/10) )
    {
	send_to_char("You lack sufficient skill points.\n\r",ch);
	return;
    }

    ch->skill_points -= (level/10);

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/2;
    af.modifier         = (level/10);
    af.location         = APPLY_STR;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are imbued with the Spirit of Bear.\n\r",victim);
    act("$n is strong as a bear.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_spirit_of_boar(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->skill_points < (level/10) )
    {
        send_to_char("You lack sufficient skill points.\n\r",ch);
        return;
    }

    ch->skill_points -= (level/10);

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/2;
    af.modifier         = (level/10);
    af.location         = APPLY_CON;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are imbued with the Spirit of Boar.\n\r",victim);
    act("$n is hardy as a boar.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_spirit_of_cat(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->skill_points < (level/10) )
    {
        send_to_char("You lack sufficient skill points.\n\r",ch);
        return;
    }

    ch->skill_points -= (level/10);

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/2;
    af.modifier         = (level/10);
    af.location         = APPLY_DEX;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are imbued with the Spirit of Cat.\n\r",victim);
    act("$n is quick as a cat.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_spirit_of_owl(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if ( ch->skill_points < (level/10) )
    {
        send_to_char("You lack sufficient skill points.\n\r",ch);
        return;
    }

    ch->skill_points -= (level/10);

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/2;
    af.modifier         = (level/10);
    af.location         = APPLY_WIS;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are imbued with the Spirit of Owl.\n\r",victim);
    act("$n is wise as an owl.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_spirit_of_wolf(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    /* Costs up to 10 sp per casting */
    if ( ch->skill_points < (level/5) )
    {
        send_to_char("You lack sufficient skill points.\n\r",ch);
        return;
    }

    ch->skill_points -= (level/5);

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = (level/5);
    af.modifier         = 0;
    af.location         = 0;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are imbued with the Spirit of Wolf.\n\r",victim);
    act("$n is cunning as a wolf.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

/*Here are the spells for the scribe kit */
void spell_scribe_maguswrath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
    int dam1;
    int dam2;

    dam = dice(level / 10, 4);
    dam1= dice(level / 8, 4);
    dam2 = dice(level /8, 4);

    if ( saves_spell( level, victim,DAM_FIRE) )
    {
        dam /= 2;
    }
    if ( saves_spell( level, victim,DAM_COLD) )
    {
        dam1 /= 2;
    }
    if ( saves_spell( level, victim,DAM_MENTAL) )
    {
        dam2 /= 2;
    }
    send_to_char("OUCH!!! So THAT'S the wrath of a mage!!! \r \n",victim);
    damage( ch, victim, dam, sn, DAM_FIRE ,TRUE,TRUE);
    damage( ch, victim, dam1, sn, DAM_COLD,TRUE ,TRUE);
    damage( ch, victim, dam2, sn, DAM_MENTAL,TRUE ,TRUE);


    return;
}

void spell_scribe_knowledge( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	    CHAR_DATA *victim = (CHAR_DATA *) vo;

	if ( saves_spell(level,victim,DAM_MENTAL) )
	{
 	send_to_char("Nope, nothing.\r\n",victim);
	send_to_char("I guess they'll never learn.\r\n",ch);	
	return;
	}
	else
	{
	send_to_char("Wow, you should read more often!\r\b",victim);
        victim->skill_points += 15;
	return;
	}


}




void spell_scribe_rest( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
	CHAR_DATA *victim = (CHAR_DATA *) vo;

	if (saves_spell(level, victim, DAM_MENTAL) )
	{
		send_to_char("Nothing seemed to happen.\r\n",ch);
		send_to_char("Nothing seemed to happen.\r\n",victim);
		return;
	}
	else
	{
		send_to_char("WHOA! What a rush!!!\r\n",victim);
		send_to_char("They look much better now.\r\n",ch);
            	victim->hit  = victim->max_hit;
            	victim->mana = victim->max_mana;
            	victim->move = victim->max_move;
		return;
	}
}


void spell_scribe_pox( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int dam;

    if ( saves_spell(level,victim,DAM_DISEASE) || is_affected(victim,sn) )
    {
        send_to_char("Nothing seems to happen.\n\r",ch);
        return;
    }


    dam = number_range( level * 1, level * 2 );
    /* It worked */
    damage( ch, victim, dam, sn, DAM_DISEASE,TRUE,TRUE );

    /* At this point, ch should be fighting victim.
     * If not, we killed victim.  So quit!  */
    if ( ch->fighting != victim )
        return;

    af.where = DAMAGE_OVER_TIME;
    af.duration = 9;   /* 2 minutes of damage */
    af.location = 35;
    af.modifier = 35; /* 360/180/90 */
    af.level    = level;
    af.type     = sn;
    af.bitvector        = DAM_DISEASE;
    af.caster_id        = ch->id;

    affect_to_char( victim, &af );

    send_to_char("This ain't Chicken pox!!!!\n\r",victim);
    act("$n suddenly is covered with oozing sores!",
                victim,NULL,NULL,TO_ROOM,FALSE);

    return;
}















void spell_shield_of_thorns(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_shield_of_thorns) ||
         is_affected(victim,gsn_shield_of_brambles) ||
         is_affected(victim,gsn_shield_of_spikes) ||
         is_affected(victim,gsn_shield_of_blades) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/10;
    af.modifier         = 15;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a shield of thorns.\n\r",victim);
    act("$n is surrounded by a shield of thorns.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_shield_of_brambles(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_shield_of_thorns) ||
         is_affected(victim,gsn_shield_of_brambles) ||
         is_affected(victim,gsn_shield_of_spikes) ||
         is_affected(victim,gsn_shield_of_blades) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/10;
    af.modifier         = 30;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a shield of brambles.\n\r",victim);
    act("$n is surrounded by a shield of brambles.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_shield_of_spikes(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_shield_of_thorns) ||
         is_affected(victim,gsn_shield_of_brambles) ||
         is_affected(victim,gsn_shield_of_spikes) ||
         is_affected(victim,gsn_shield_of_blades) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/10;
    af.modifier         = 45;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a shield of spikes.\n\r",victim);
    act("$n is surrounded by a shield of spikes.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_shield_of_blades(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,gsn_shield_of_thorns) ||
	 is_affected(victim,gsn_shield_of_brambles) ||
	 is_affected(victim,gsn_shield_of_spikes) ||
	 is_affected(victim,gsn_shield_of_blades) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level/10;
    af.modifier         = 60;
    af.location         = APPLY_AC;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a shield of blades.\n\r",victim);
    act("$n is surrounded by a shield of blades.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_midnight_cloak(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = ( HAS_KIT(ch,"necromancer") ? 2 : 0 ) + ( time_info.hour == 0 ? 2 : 0 );
    af.modifier         = 10;
    af.location         = APPLY_HITROLL;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You are surrounded by a cloak of darkness.\n\r",victim);
    act("$n is surrounded by a cloak of darkness.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}


void spell_arcantic_alacrity( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || is_affected(victim,gsn_arcantic_lethargy) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = 1;
    af.modifier         = (level/10);
    af.location         = APPLY_INT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("Your magical abilities are enhanced.\n\r",victim);
    act("$n's magical abilities are enhanced.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}

void spell_arcantic_lethargy( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) || saves_spell(level,victim,DAM_OTHER) )
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = 1;
    af.modifier         = (level/-10);
    af.location         = APPLY_INT;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You feel your magical abilities slip away.\n\r",victim);
    act("$n appears shocked as $s magical powers fade.",victim,NULL,NULL,TO_ROOM,FALSE);
    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_immolate( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level,12);

    if( saves_spell(level,victim,DAM_FIRE) )
        dam /= 2;

    damage(ch,victim,dam,sn,DAM_FIRE,TRUE,TRUE);
    return;
}

void spell_lacerate( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(level,15);

    if ( saves_spell(level,victim,DAM_SLASH) )
	dam /= 2;

    damage(ch,victim,dam,sn,DAM_SLASH,TRUE,TRUE);
    return;
}

void spell_clarity( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("You feel relaxed as your mind clears.\n\r",victim);
	act("$n relaxes as $s mind clears.",victim,NULL,NULL,TO_ROOM,FALSE);
	reup_affect(victim,sn,level,level);
        return;
    }

    af.where            = TO_AFFECTS;
    af.type             = sn;
    af.level            = level;
    af.duration         = level;
    af.modifier         = 0;
    af.location         = 0;
    af.bitvector        = 0;

    affect_to_char(victim,&af);
    send_to_char("You feel relaxed as your mind clears.\n\r",victim);
    act("$n relaxes as $s mind clears.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}
void spell_dispel_wall( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
   OBJ_DATA *wall;
   AFFECT_DATA *paf;
   char *targ;

   if ( !(is_room_affected(ch->in_room, gsn_wall_fire) ) && !(is_room_affected(ch->in_room, gsn_wall_ice)) )
	{
	send_to_char("You can't dispel that.\n\r", ch);
	return;
	}

   for(paf = ch->in_room->affected; paf !=  NULL; paf = paf->next)
   	{
	if(paf->type == gsn_wall_fire) break;
	if(paf->type == gsn_wall_ice ) break;
	}

if (target_name == "") {
    targ = "wall";
  } else {
    targ = target_name;
  }
  wall = get_obj_list( ch, targ, ch->in_room->contents );

   raffect_remove(ch->in_room, paf);
   act("$p slowly disappears", ch, wall, NULL, TO_ROOM, FALSE);
   act("$p slowly disappears", ch, wall, NULL, TO_CHAR, FALSE);
   obj_from_room(wall);
   extract_obj(wall);
}

void spell_wall_of_ice( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
  EXIT_DATA *pexit;
  int door;
  OBJ_DATA *wall;
  AFFECT_DATA af;

  if (is_room_affected(ch->in_room, gsn_wall_ice))
  {
  send_to_char("There is already a wall of ice here.\n\r", ch);
  return;
  }


  for (door =0; door < 6; door++ ){
  if ( ( pexit = ch->in_room->exit[door] ) == 0 || pexit->u1.to_room == NULL || pexit->u1.to_room->clan)
        continue;
  else
    break;
  } 

  wall = create_object(get_obj_index(OBJ_VNUM_WALL_ICE),0,FALSE);
        obj_to_room(wall, ch->in_room);

        af.where = TO_AFFECTS;
        af.type = gsn_wall_ice;
        af.level = level;
        af.location = door;
        af.duration = level * 2 / 3;
 	wall->timer = level * 2 / 3;
        af.modifier = 0;
        af.bitvector = 0;
        af.caster_id = ch->id;

        affect_to_room( ch->in_room, &af );

        act("You summon $p!",ch,wall,NULL,TO_CHAR,FALSE);
        act("$n summons $p!",ch,wall,NULL,TO_ROOM,FALSE);
        return;
}

void spell_aura_cthon(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{

        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;

        send_to_char("Sorry, this isn't a spell anymore\n\r",ch);
        return;

        if ( victim != ch )
        {
        send_to_char("You may not cast this on another.\n\r", ch);
        return;
        }
        if( is_affected(ch, skill_lookup("aura of cthon")) )
        {
        send_to_char("How much creepier do you want to get?  You are already affected by the aura of {RCthon{x.\n\r", ch);
        return;
        }

        check_improve(ch,gsn_honor_guard,TRUE,8);
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = ch->level;
    af.duration = number_range(1,10) + number_range(1,ch->level/10);
    af.bitvector= 0;
    af.modifier = 0;
    af.location = APPLY_NONE;
    affect_to_char(ch, &af );
    return;


}

void spell_hydrophilia(int sn,int level,CHAR_DATA *ch, void *vo, int target)
{

        CHAR_DATA *victim = (CHAR_DATA *) vo;
        AFFECT_DATA af;

        if ( victim == ch )
        {
        send_to_char("If you desire the waves that much, go jump in the lake!\n\r", ch);
        return;
        }

        if ( IS_SET(victim->vuln_flags, VULN_DROWNING) || IS_SET(victim->vuln_flags, VULN_COLD))
        {
        send_to_char("They already desire oneness with the waves.\n\r", ch);
        return;
        }

        if ( saves_spell(IS_SET(ch->mhs, MHS_ELEMENTAL) ? level + 5 : level,victim,DAM_DROWNING) )
        {
         send_to_char("You failed.\n\r", ch);
         return;
        }

        af.where                = TO_VULN;
        af.type                 = sn;
        af.level                = level;
        af.duration             = level /11;
        af.modifier             = 0;
        af.location             = 0;
        af.bitvector            = VULN_DROWNING | VULN_COLD;
        affect_to_char( victim, &af );

        act("You suddenly feel drawn to the waves.",ch,NULL,victim,TO_VICT,FALSE);
        act("$N suddenly feels drawn to the waves.",ch,NULL,victim,TO_CHAR,FALSE);

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

        return;
}

void spell_stalk ( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    if( is_affected(ch, skill_lookup("stalk")) )
    {
        send_to_char("You are to spent from your last attempt to stalk, wait a minute.\n\r",ch);
        return;
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
    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not stalk.\n\r",ch);
       return;
    }
    if ( (victim = get_char_world( ch, target_name ))  == NULL )
    {
    send_to_char("They aren't there.\n\r",ch);
    return;
    }
    if( IS_IMMORTAL(victim) )
    {
      send_to_char("That would be really, really, really stupid...\r\n",ch);
      return;
    }

    if( IS_NPC(victim) )
    {
      send_to_char("Stalking a mob?  Get a life.\r\n",ch);
      return;
    }
    if ( !is_clan(ch) )
    {
      send_to_char("You really should have read the helpfile.  This spell can't be used by nonclanners.\r\n",ch);
      return;
    }
    if ( !is_clan(victim) )
    {
      send_to_char("You will not stalk non-clanners.\r\n",ch);
      return;
    }
    if ( !is_clan(ch) && is_clan(victim) )
    {
	send_to_char("That would be in violation of pkrules.\r\n",ch);
	return;
    }
    af.where    = TO_AFFECTS;
    af.type     = sn;
    af.level    = 0;
    af.duration = 0;
    af.bitvector= 0;
    af.modifier = 0;
    af.location = APPLY_NONE;
    affect_to_char(ch, &af );

  
    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   saves_spell(ch->level,victim,DAM_OTHER) 
    ||   !is_room_clan(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (victim->level >= ch->level + 12)
    ||   (victim->level <= ch->level - 12)
    ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */
    ||   (!is_room_owner(ch,victim->in_room) && room_is_private(ch,victim->in_room) )
    ||  victim->in_room->area->under_develop
    ||  victim->in_room->area->no_transport
    ||  ch->in_room->area->no_transport )
    {
        send_to_char( "You failed.\n\r", ch );
	if (victim != NULL )
        {
	send_to_char( "You feel a shiver run down your spine.\n\r",victim);
	}
        return;
    }

    /* act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM,FALSE); */
    send_to_char("You begin your stalking adventure.\n\r",ch);

    char_from_room(ch);
    clear_mount(ch);
    char_to_room(ch,victim->in_room);
 /*   act("$n has arrived through a gate.",ch,NULL,NULL,TO_ROOM,FALSE); */

 /*   do_look(ch,"auto");*/
    if( saves_spell(ch->level,victim,DAM_OTHER) )
    {
        send_to_char("Brrrr, that was weird.\r\n",victim); 
    }

}

void spell_hemorrhage( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    level = ch->level + ch->level / 14; /* Up to casting at 54 */

    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
      send_to_char("Gladiators can not use clan skills.\n\r", ch);
      return;
    }

    if ( saves_spell(level,victim,DAM_OTHER) || (victim->hit >= victim->max_hit) ||
      number_percent() < victim->hit * 100 / victim->max_hit)
    {
        send_to_char("You failed.\n\r",ch);
        return;
    }

    if (IS_SET(victim->affected_by_ext, AFF_EXT_BLOODY))
    {
      send_to_char("Blood is already pouring from their wounds.\n\r", ch);
      return;
    }

    af.where     = TO_AFFECTS_EXT;
    af.type      = sn;
    af.level     = level;
    af.location  = 0;
    af.modifier  = 1;/* Level of the clan skill */
    af.duration  = 1;
    af.bitvector = AFF_EXT_BLOODY;
    affect_to_char( victim, &af );
    send_to_char( "Blood pours from your wounds, spattering on the floor!\n\r", victim );
    act("Blood pours from $n's wounds and spatters on the floor.",victim,NULL,NULL,TO_ROOM,FALSE);

    if(check_annointment(victim, ch))
    {/* Success triggers the message on its own, just handle the spell itself here */
        spell_hemorrhage(sn,level+1,victim,ch,target);
    }

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_annointment(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

  level = ch->level;

  if(IS_SET(ch->mhs,MHS_GLADIATOR))
  {
    send_to_char("Gladiators can not use clan skills.\n\r",ch);
    return;
  }

    if (IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
    {
        send_to_char("You have been sanctioned from using this skill.\n\r",
          ch);
        return;
    }

  if(!ch->in_room || ch->in_room->vnum >= 0)
  {
    send_to_char("You must be in your hall to annoint yourself.\n\r", ch);
    return;
  }

    if ( is_affected(victim, sn) || !is_same_clan(victim, ch))
    {
      send_to_char("You failed.\n\r",ch);
      return;
    }

    af.where            = TO_AFFECTS_EXT;
    af.type             = sn;
    af.level            = level;
    af.duration         = (level / 5);
    af.modifier         = 1;/* Skill level strength */
    af.location         = APPLY_NONE;
    af.bitvector        = AFF_EXT_ANNOINTMENT;
    affect_to_char( ch, &af );
    do_deity_msg("You gain the annointment of %s.", victim);
    return;
}

void spell_convert(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
  char arg[256], *argument;

  if(IS_SET(ch->mhs,MHS_GLADIATOR))
  {
    send_to_char("Gladiators can not use clan skills.\n\r",ch);
    return;
  }

  if(!target_name[0])
  {
    send_to_char("What kind of alchemy do you want to perform?\n\r", ch);
    return;
  }
  argument = target_name;
  argument = one_argument(argument, arg);
  if(!str_prefix(arg, "silver"))
  {
    int amount;
    if(argument[0] && str_cmp(argument, "all"))
    {
      if(!is_number(argument))
      {
        send_to_char("Amount of silver to convert must be 'all' or a number.\n\r", ch);
        return;
      }
      amount = atoi(argument);
      if(amount < 100)
      {
        send_to_char("That's too little silver to convert to gold, it must be at least 100.\n\r", ch);
        return;
      }
      if(amount > ch->silver)
        amount = (ch->silver / 100) * 100;
      else
        amount = amount / 100 * 100; /* Remove any extra that would be lost */
    }
    else
      amount = (ch->silver / 100) * 100;
    if(!amount)
    {
      send_to_char("You don't have enough silver to produce any gold.\n\r", ch);
      return;
    }
    ch->silver -= amount;
    ch->gold += amount / 100;
    sprintf(arg, "You convert %d silver into %d gold.\n\r", amount, amount / 100);
    send_to_char(arg, ch);
    act("The silver held by $n shimmers into gold.", ch, NULL, NULL, TO_ROOM, FALSE);
    return;
  }
  send_to_char("Your power is only enough to convert silver to gold.\n\r", ch);
}

void release_clan_guardian(CHAR_DATA *guardian)
{
  if(!IS_NPC(guardian) || !guardian->qchar)
    return;
  act("$n flares and vanishes as $e is released from service!", guardian, NULL, NULL, TO_ROOM, FALSE);
  if(guardian->qchar)
  {
    if(!IS_NPC(guardian->qchar))
      guardian->qchar->pcdata->quest_count = UMAX(0, guardian->qchar->pcdata->quest_count - 1);
    send_to_char("Your guardian has been released.\n\r", guardian->qchar);
  }
  guardian->qchar = NULL;/* Break recursion */
  extract_char(guardian, TRUE);
}

void spell_guardian(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
  int cost;
  char buf[256];
  CHAR_DATA *guardian;
  AFFECT_DATA af;
  bool release = FALSE;
  int i, count;

  level = ch->level;
  
  if(IS_NPC(ch) || !ch->pcdata->clan_info)
  {
    send_to_char("You may not summon a guardian.\n\r", ch);
    return;
  }
  
  if(IS_SET(ch->mhs,MHS_GLADIATOR))
  {
    send_to_char("Gladiators can not use clan skills.\n\r",ch);
    return;
  }
  
  count = 0;

  for ( guardian = char_list; guardian != NULL; guardian = guardian->next )
    if (guardian->pIndexData && guardian->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN && guardian->qchar == ch)
      break;

  if(target_name[0])
  {
    if(!guardian)
    {
      send_to_char("You have no guardian to command.\n\r", ch);
      return;
    }
    if(!str_prefix(target_name, "release"))
    {
      release_clan_guardian(guardian);
      return;
    }
    else if(!str_prefix(target_name, "recall"))
    {
      if(!guardian->fighting && guardian->in_room->vnum != ROOM_VNUM_TEMPLE)
      {/* Can't move your guardian if it's engaged with someone */
        act("$n vanishes!", guardian, NULL, NULL, TO_ROOM, FALSE);
        char_from_room(guardian);
        char_to_room(guardian, get_room_index(ROOM_VNUM_TEMPLE));
        act("$n appears in the room.", guardian, NULL, NULL, TO_ROOM, FALSE);
      }
      send_to_char("Okay.\n\r", ch);
    }
    else if(!str_prefix(target_name, "home"))
    {
      if(!guardian->fighting || !ch->pcdata->clan_info->clan->hall)
      {/* Can't move your guardian if it's engaged with someone */
        for(i = 0; i < 6; i++)
        {
          if(ch->pcdata->clan_info->clan->hall->exits[i].outside)
          {
            act("$n vanishes!", guardian, NULL, NULL, TO_ROOM, FALSE);
            char_from_room(guardian);
            char_to_room(guardian, ch->pcdata->clan_info->clan->hall->exits[i].outside);
            act("$n appears in the room.", guardian, NULL, NULL, TO_ROOM, FALSE);
            break;
          }
        }
      }
      send_to_char("Okay.\n\r", ch);
    }
    else if(!str_prefix(target_name, "summon"))
    {
      if(!guardian->fighting && guardian->in_room != ch->in_room)
      {/* Can't move your guardian if it's engaged with someone */
        act("$n vanishes!", guardian, NULL, NULL, TO_ROOM, FALSE);
        char_from_room(guardian);
        char_to_room(guardian, guardian->qchar->in_room);
        act("$n appears in the room.", guardian, NULL, NULL, TO_ROOM, FALSE);
      }
      send_to_char("Okay.\n\r", ch);
    }
    else if(!str_prefix(target_name, "status"))
    {/* Higher level guardians can provide details on who they're fighting */
      sprintf(buf, "Your guardian has %d/%d hit points and is %s.\n\r", guardian->hit,
        guardian->max_hit, guardian->fighting ? "in combat" : "not in combat");
      send_to_char(buf, ch);
      if(IS_SET(guardian->qnum, GUARDIAN_ATTACK))
        send_to_char("Your guardian will attack any enemy it sees and ", ch);
      else if(IS_SET(guardian->qnum, GUARDIAN_ASSIST))
        send_to_char("Your guardian will only attack your enemy target and ", ch);
      else
        send_to_char("Your guardian will never initiate combat and ", ch);
      if(IS_SET(guardian->qnum, GUARDIAN_FOLLOW))
        send_to_char("is following you.\n\r", ch);
      else
        send_to_char("is holding position.\n\r", ch);
/*      if(IS_SET(guardian->qnum, GUARDIAN_SNEAK))
        send_to_char("When your guardian moves, it moves stealthily.\n\r", ch);
      else
        send_to_char("When your guardian moves, it can be easily seen.\n\r", ch);*/
    }
    else if(!str_prefix(target_name, "assist"))
    {/* Change its attack mode */
      REMOVE_BIT(guardian->qnum, (GUARDIAN_PEACE | GUARDIAN_ATTACK));
      SET_BIT(guardian->qnum, GUARDIAN_ASSIST);
      send_to_char("Your guardian will now only attack enemies you are attacking.\n\r", ch);
    }
    else if(!str_prefix(target_name, "attack"))
    {/* Change its attack mode */
      REMOVE_BIT(guardian->qnum, (GUARDIAN_SENTINEL | GUARDIAN_ASSIST));
      SET_BIT(guardian->qnum, GUARDIAN_ATTACK);
      send_to_char("Your guardian will now attack any enemy it sees.\n\r", ch);
    }
    else if(!str_prefix(target_name, "passive"))
    {/* Change its attack mode */
      REMOVE_BIT(guardian->qnum, (GUARDIAN_ASSIST | GUARDIAN_ATTACK));
      SET_BIT(guardian->qnum, GUARDIAN_PEACE);
      send_to_char("Your guardian will not attack anything.\n\r", ch);
    }
    else if(!str_prefix(target_name, "hold"))
    {/* Change its movement mode */
      REMOVE_BIT(guardian->qnum, GUARDIAN_FOLLOW);
      SET_BIT(guardian->qnum, GUARDIAN_SENTINEL);
      guardian->master = NULL;
      send_to_char("Your guardian will remain where it is.\n\r", ch);
    }
    else if(!str_prefix(target_name, "follow"))
    {/* Change its movement mode */
      REMOVE_BIT(guardian->qnum, GUARDIAN_SENTINEL);
      SET_BIT(guardian->qnum, GUARDIAN_FOLLOW);
      guardian->master = guardian->qchar;
      send_to_char("Your guardian will follow you.\n\r", ch);
    }
/*    else if(!str_prefix(target_name, "sneak"))// Eventually, with higher levels.  Not yet.
    {// Change its movement mode
      SET_BIT(guardian->qnum, GUARDIAN_SNEAK);
      send_to_char("Your guardian will move stealthily.\n\r", ch);
    }
    else if(!str_prefix(target_name, "visible"))
    {// Change its movement mode
      REMOVE_BIT(guardian->qnum, GUARDIAN_SNEAK);
      send_to_char("Your guardian will move visibly.\n\r", ch);
    }*/
    else
    {
      send_to_char("Unrecognized guardian command.  See 'help clan guardian' for options.\n\r", ch);
      return;
    }
    return;
  }
  if(ch->pcdata->start_time > 0)
  {
    send_to_char("You must be able to attack other players to create a clan guardian.\n\r", ch);
    return;
  }
  if(ch->in_room->vnum >= 0)
  {
    send_to_char("You must be in your hall to create a clan guardian.\n\r", ch);
    return;
  }
  if(guardian)
  {
    send_to_char("You already have a guardian following you.\n\r", ch);
    return;
  }
  cost = URANGE(10, ch->level * 2, 100);
  if(ch->hit <= cost * 2)
  {
    send_to_char("It would kill you to create a clan guardian.  Literally.\n\r", ch);
    return;
  }
  if(ch->mana < cost - 5)
  {/* Account for the 5 energy to cast guardian */
    send_to_char("You don't have enough mana to create a clan guardian.\n\r", ch);
    return;
  }
  if(ch->move < cost)
  {
    send_to_char("You're too tired to create a clan guardian.\n\r", ch);
    return;
  }
  WAIT_STATE(ch, 36);
  ch->pcdata->quest_count++;/* The guardian uses a quest AI */
  guardian = create_mobile(get_mob_index(MOB_VNUM_CLAN_GUARDIAN));
  /* String guardian to match the clan */
  sprintf(buf, "guardian %s", ch->pcdata->clan_info->clan->name);
  clear_string(&guardian->name, buf);
  sprintf(buf, "A Guardian of %s", ch->pcdata->clan_info->clan->name);
  clear_string(&guardian->short_descr, buf);
  sprintf(buf, "A Guardian of %s stands here, ready to defend the clan.\n\r", ch->pcdata->clan_info->clan->name);
  clear_string(&guardian->long_descr, buf);
  guardian->qchar = ch;
  char_to_room (guardian,ch->in_room);
  guardian->level = level;
  for (i = 0; i < 3; i++)
    guardian->armor[i] = interpolate(guardian->level,50,-150);
  guardian->armor[3] = interpolate(guardian->level,0,-100);
  /* Big bag of hit points */
  guardian->max_hit = level * 20 + number_range(level * level * 2 / 3, level * level );
  guardian->hit = guardian->max_hit;
  guardian->max_mana = 100;
  guardian->mana = 100;
  guardian->damage[DICE_NUMBER] = guardian->level/8+1;
  guardian->damage[DICE_TYPE]   = 6;
  guardian->hitroll = guardian->level / 2;
  guardian->damroll = guardian->level / 2;
  SET_BIT(guardian->form,FORM_INSTANT_DECAY);
  ch->hit -= cost * 2;
  ch->mana -= cost - 5;
  ch->move -= cost;
  guardian->qnum = GUARDIAN_ATTACK | GUARDIAN_FOLLOW;
  act("$n explodes into existance with a {Ybrilliant{x flash!", guardian, NULL, NULL, TO_ROOM, FALSE);
  guardian->master = ch;
  act("$N now follows you.", ch, NULL, guardian, TO_CHAR, FALSE);
  send_to_char("You feel drained.\n\r", ch);
}

