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
 *  airound, comes around.                                                  *
 ***************************************************************************/

static char rcsid[] = "$Id: fight.c,v 1.945 2005/02/17 05:16:22 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "interp.h"
#include "gladiator.h"

#define MAX_DAMAGE_MESSAGE 44

extern char * const dir_name[];
int override = 0;

/* command procedures needed */
DECLARE_DO_FUN(do_tail_slap );
DECLARE_DO_FUN(do_backstab  );
DECLARE_DO_FUN(do_emote   );
DECLARE_DO_FUN(do_berserk );
DECLARE_DO_FUN(do_bash    );
DECLARE_DO_FUN(do_bite    );
DECLARE_DO_FUN(do_breathe    );
DECLARE_DO_FUN(do_trip    );
DECLARE_DO_FUN(do_dirt    );
DECLARE_DO_FUN(do_fear    );
DECLARE_DO_FUN(do_flee    );
DECLARE_DO_FUN(do_hex    );
DECLARE_DO_FUN(do_kick    );
DECLARE_DO_FUN(do_disarm  );
DECLARE_DO_FUN(do_get   );
DECLARE_DO_FUN(do_recall  );
DECLARE_DO_FUN(do_yell    );
DECLARE_DO_FUN(do_sacrifice );
DECLARE_DO_FUN(do_kcharge );
DECLARE_DO_FUN(do_grab    );

#ifdef DEITY_TRIAL_DEBUG_CODE
extern int deity_value_override;
#endif

extern int clanner_count;
extern int bounty_timer;
extern int bounty_complete;
extern int bounty_type;
extern bool bounty_downgrade;
void select_bounty(int qualifier);

CLAN_DATA* clan_lookup   args( ( const char *name ) );
/*
 * Local functions.
 */
void  kill      args( ( CHAR_DATA *ch, char *argument, bool canChange ) );
int   check_myrmidon    args( ( CHAR_DATA *ch, int sn ) );
void  flee              args( ( CHAR_DATA *ch, char *argument, bool fWimpy ) );
void  handle_critical( CHAR_DATA *ch, int *damage, int dam_type, int diceroll, int base_dam );
bool  is_safe_steal     args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
bool  is_clan_guard     args( ( CHAR_DATA *victim) );
void  do_cutpurse       args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void  check_assist  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool  check_scales args( ( CHAR_DATA *ch, CHAR_DATA *victim,bool fSecondary ) );
bool  check_dodge args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary ) );
bool  check_mistform args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool  check_kailindo args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void  check_killer  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
bool  check_parry args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary ) );
bool  check_shield_block args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary ) );
bool  check_nether args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary ) );
void  dam_message args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
          int dt, bool immune ) );
void    dam_message_new args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
                            int dt, bool immune ) );
void  death_cry args( ( CHAR_DATA *ch ) );
void  group_gain  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int xp_compute  args( ( CHAR_DATA *gch, CHAR_DATA *victim,
          int total_levels ) );
bool  is_safe   args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void  one_hit   args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void    mob_hit   args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
void  raw_kill  args( ( CHAR_DATA *victim, CHAR_DATA *ch ) );
void  set_fighting  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void  disarm    args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

/* Externals */
void  remove_highlander    args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );

/*
 * Utility function for rangers
 */
int is_enemy( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( !HAS_KIT(ch,"ranger") )
        return FALSE;

    if ( victim->race != ch->species_enemy )
        return FALSE;
    else
        return TRUE;
}

int terrain( CHAR_DATA *ch )
{
    if ( !HAS_KIT(ch,"ranger") )
       return -2;

    switch( ch->in_room->sector_type )
    {
    case SECT_INSIDE:
    case SECT_CITY:
        return 0;
    case SECT_FOREST:
    case SECT_HILLS:
    case SECT_MOUNTAIN:
    case SECT_FIELD:
        return 1;
    default:
        return -1;
    }
}

int calc_fast_rate(CHAR_DATA *ch, int room_rate, bool healing)
{
  int amount = 0;
  if(!ch->alert && !IS_AFFECTED(ch, AFF_POISON) && !IS_AFFECTED(ch, AFF_PLAGUE))
  {/* Alert and damage affects prevent fast healing entirely */
    int medium = 0;
    if((medium = room_has_medium(ch)) != 0 ||
      (ch->pcdata->condition[COND_HUNGER] > 0 && ch->pcdata->condition[COND_THIRST] > 0
      && !IS_AFFECTED(ch,AFF_HASTE) && !IS_AFFECTED(ch,AFF_SLOW)))
    {/* Medium overrides hunger/thirst/slow/haste */
      int booster;
      if(healing)
        booster = gsn_fast_healing;
      else
        booster = gsn_meditation;
      amount = 1;
      if(number_percent() < get_skill(ch,booster))
        amount += UMAX(1, get_skill(ch,booster) / 50);
      if(IS_SET(ch->act,PLR_MUMMY))
        amount += UMAX(1, ch->level / 25);
      if(medium)
        amount += UMAX(1, medium / 25);
      amount += UMAX(0, (room_rate - 200) / 50);
    }
  }
  return amount;
}

/*
 * Control the fights going on.
 * Called periodically by update_handler.
 */
void violence_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *victim;
    AFFECT_DATA *paf, *paf_next;

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
  ch_next = ch->next;

  if(ch->fighting == NULL)
  {
    if(ch->alert > 0)
    {
      ch->alert--;
      if(!ch->alert)
      {
        send_to_char("Your heightened alertness fades.\n\r", ch);
      }
    }
  }

  for ( paf = ch->flash_affected; paf != NULL; paf = paf_next )
  {
      paf_next  = paf->next;

        /* DOTs have their own handler */
      if ( paf->where == DAMAGE_OVER_TIME )
        continue;

      if ( paf->duration > 0 )
      {
         if ( paf->type == gsn_rage && ch->fighting != NULL )
         {
            check_improve(ch,paf->type,TRUE,5);
            paf->duration += 2;
         }
         /*Gladiators Do not lose spell duration/level during wait */
         if (IS_SET(ch->mhs,MHS_GLADIATOR) && !IS_NPC(ch))
         {
             if (gladiator_info.time_left < 1
                 && gladiator_info.bet_counter < 1)
             {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                   paf->level--;  /* spell strength fades with time */
             }
         }
         else
         {
            paf->duration--;
            if (number_range(0,4) == 0 && paf->level > 0)
               paf->level--;  /* spell strength fades with time */
         }
      }
      else if ( paf->duration < 0 )
    ;
      else
      {
    if ( paf_next == NULL
    ||   paf_next->type != paf->type
    ||   paf_next->duration > 0 )
    {
        if ( paf->type > 0 && skill_table[paf->type].msg_off )
        {
      send_to_char( skill_table[paf->type].msg_off, ch );
      send_to_char( "\n\r", ch );
        }
    }

    flash_affect_remove( ch, paf,APPLY_BOTH );

      }
  if (   !IS_AFFECTED(ch, AFF_CHARM)
      && IS_NPC(ch)
      && !IS_SET(ch->act,ACT_PET)
      && ch->master != NULL
      && ch->fighting == NULL &&
      ch->pIndexData->vnum != MOB_VNUM_CLAN_GUARDIAN)
     die_follower( ch );
  }


  if ( !IS_NPC(ch) )
  {
    if(ch->pcdata->clan_info && ch->pcdata->clan_info->delay_merit)
    {
      MERIT_TRACKER *mtrack = ch->pcdata->clan_info->delay_merit;
      while(mtrack && mtrack->expire <= 0)
      {/* Should be sorted in order, first expiring to last */
        char buf[256];
        sprintf(buf, "%d stolen merit has converted to your regular pool.\n\r", mtrack->amount);
        ch->pcdata->clan_info->merit += mtrack->amount;
        ch->pcdata->clan_info->delay_merit = mtrack->next;
        free_merit(mtrack);
        mtrack = ch->pcdata->clan_info->delay_merit;
      }
      for(; mtrack; mtrack = mtrack->next)
      {
        mtrack->expire--;
      }
    }
    if(ch->position == POS_SLEEPING && ch->in_room != NULL)
    {
      if(ch->pcdata->fast_h)
      {
        if(ch->alert)
        {/* Something combat related happened */
          if(ch->pcdata->fast_h >= 10 || ch->pcdata->fast_m >= 10)
            send_to_char("Your rapid regeneration has ended.\n\r", ch);
          ch->pcdata->fast_h = 0;
          ch->pcdata->fast_m = 0;
        }
        else
        {
          ch->pcdata->fast_h++;
          if(ch->pcdata->fast_h >= 10)
          {/* Full regen from 0 in 2 ticks once it begins, 20 pulses in a tick */
            if(ch->pcdata->fast_h == 10)
              send_to_char("You begin to rapidly recover your physical strength.\n\r", ch);
            if(ch->hit < ch->max_hit)
            {
              ch->hit += UMAX(1, ch->max_hit * ch->pcdata->fast_h / 1000);
              ch->hit = UMIN(ch->hit, ch->max_hit);
            }
            if(ch->move < ch->max_move)
            {
              ch->move += UMAX(1, ch->max_move * ch->pcdata->fast_h / 2000);
              ch->move = UMIN(ch->move, ch->max_move);
            }
          }
          if(ch->hit >= ch->max_hit && ch->move >= ch->max_move)
          {
            if(ch->pcdata->fast_m < 10 && ch->pcdata->fast_h >= 10)
              send_to_char("Your rapid regeneration has ended.\n\r", ch);
            ch->pcdata->fast_h = 0;
          }
        }
      }
      else if(ch->in_room->heal_rate >= 200 && (ch->hit < ch->max_hit || ch->move < ch->max_move) && !ch->pcdata->quit_time)
      {
        ch->pcdata->fast_h = calc_fast_rate(ch, ch->in_room->heal_rate, TRUE);
        if(ch->pcdata->fast_h >= 10)
          send_to_char("You begin to rapidly recover your physical strength.\n\r", ch);
      }
      if(ch->pcdata->fast_m)
      {
        if(ch->alert)
        {/* Something combat related happened */
          if(ch->pcdata->fast_h >= 10 || ch->pcdata->fast_m >= 10)
            send_to_char("Your rapid regeneration has ended.\n\r", ch);
          ch->pcdata->fast_h = 0;
          ch->pcdata->fast_m = 0;
        }
        else
        {
          ch->pcdata->fast_m++;
          if(ch->pcdata->fast_m >= 10)
          {/* Full regen from 0 in 2 ticks once it begins, 20 pulses in a tick */
          if(ch->pcdata->fast_m == 10)
            send_to_char("You begin to rapidly recover your mental strength.\n\r", ch);
            if(ch->mana < ch->max_mana)
            {
              ch->mana += UMAX(1, ch->max_mana * ch->pcdata->fast_m / 1000);
              ch->mana = UMIN(ch->mana, ch->max_mana);
            }
            if(ch->move < ch->max_move)
            {
              ch->move += UMAX(1, ch->max_move * ch->pcdata->fast_m / 2000);
              ch->move = UMIN(ch->move, ch->max_move);
            }
          }
          if(ch->mana >= ch->max_mana && ch->move >= ch->max_move)
          {
            if(ch->pcdata->fast_h < 10 && ch->pcdata->fast_m >= 10)
              send_to_char("Your rapid regeneration has ended.\n\r", ch);
            ch->pcdata->fast_m = 0;
          }
        }
      }
      else if(ch->in_room->mana_rate >= 200 && (ch->mana < ch->max_mana || ch->move < ch->max_move) && !ch->pcdata->quit_time)
      {
        ch->pcdata->fast_m = calc_fast_rate(ch, ch->in_room->mana_rate, FALSE);
        if(ch->pcdata->fast_m >= 10)
          send_to_char("You begin to rapidly recover your mental strength.\n\r", ch);
      }
//        !( ch->in_room->clan && IS_SET(ch->pcdata->clan_flags, CLAN_NO_REGEN))
    }
    else if(ch->pcdata->fast_h || ch->pcdata->fast_m)
    {
      /* The medium effect gives a tiny bit of resistance to not needing to sleep, right at the start */
      if(1) /* Not affected by the not yet implemented medium flash effect */
      {
        if(ch->pcdata->fast_h >= 10 || ch->pcdata->fast_m >= 10)
          send_to_char("Your rapid regeneration has ended.\n\r", ch);
        ch->pcdata->fast_h = 0;
        ch->pcdata->fast_m = 0;
      }
    }
    if(HAS_KIT(ch, "barbarian"))
    {
        AFFECT_DATA af;

        if ( ch->fighting == NULL )
                ch->pcdata->barbarian = UMAX(ch->pcdata->barbarian - 1, 0 );
        else
        if ( !is_affected( ch, gsn_barbarian_rage ) &&
                number_percent() < get_skill(ch,gsn_barbarian_rage) )
        { /** BARBARIAN RAGE!!! */
            if ( number_percent() < ++ch->pcdata->barbarian )
            {
            check_improve(ch,gsn_barbarian_rage,TRUE,5);
            af.type     = gsn_barbarian_rage;
            af.level    = ch->level;
            af.where    = TO_AFFECTS;
            af.duration = ch->pcdata->barbarian / 2 + 3;
            af.location = APPLY_STR;
            af.modifier = 3;
            af.bitvector = 0;
            affect_to_char( ch, &af );
            af.location = APPLY_DAMROLL;
            af.modifier = UMAX( 3, ch->pcdata->barbarian );
            affect_to_char( ch, &af );
            af.location = APPLY_AC;
            af.modifier = ch->pcdata->barbarian * 5;
            affect_to_char( ch, &af );
            act("$n screams with primal fury!!",ch,NULL,NULL,TO_ROOM,FALSE);
            act("You scream with primal fury!!", ch,NULL,NULL,TO_CHAR,FALSE);
            }
        }
        else
                check_improve(ch,gsn_barbarian_rage,FALSE,9);
    }
  }

  if ( ( victim = ch->fighting ) == NULL || ch->in_room == NULL )
      continue;

  if ( (IS_AWAKE(ch)) && (ch->in_room == victim->in_room) )
      multi_hit( ch, victim, TYPE_UNDEFINED );
  else
      stop_fighting( ch, FALSE );

  if ( ( victim = ch->fighting ) == NULL )
      continue;

  /*
   * Fun for the whole family!
   */
  check_assist(ch,victim);
    }

    return;
}

/* for auto assisting */
void check_assist(CHAR_DATA *ch,CHAR_DATA *victim)
{
    CHAR_DATA *rch, *rch_next;

    if( IS_NPC(ch) && ch->pIndexData->pShop != NULL )
        return;

    for (rch = ch->in_room->people; rch != NULL; rch = rch_next)
    {
  rch_next = rch->next_in_room;
  if (IS_AWAKE(rch) && rch->fighting == NULL && 
    (can_see(rch,victim,FALSE) || is_same_group(ch, rch)) )
  {// Group members can always assist
      /* quick check for ASSIST_PLAYER */
      if (!IS_NPC(ch) && IS_NPC(rch)
      && IS_SET(rch->off_flags,ASSIST_PLAYERS)
      &&  rch->level + 6 > victim->level)
      {
    do_emote(rch,"screams and attacks!");
    multi_hit(rch,victim,TYPE_UNDEFINED);
    continue;
      }

      /* PCs next */
      if (!IS_NPC(ch) || IS_AFFECTED(ch,AFF_CHARM))
      {
    if ( ( (!IS_NPC(rch) && IS_SET(rch->act,PLR_AUTOASSIST))
    ||     IS_AFFECTED(rch,AFF_CHARM))
    &&   is_same_group(ch,rch)
    &&   !is_safe(rch, victim) )
        multi_hit (rch,victim,TYPE_UNDEFINED);

    continue;
      }

      /* now check the NPC cases */

      if (IS_NPC(ch) && !IS_AFFECTED(ch,AFF_CHARM))

      {
    if ( (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALL))

    ||   (IS_NPC(rch) && rch->group && rch->group == ch->group)

    ||   (IS_NPC(rch) && rch->race == ch->race
       && IS_SET(rch->off_flags,ASSIST_RACE))

    ||   (IS_NPC(rch) && IS_SET(rch->off_flags,ASSIST_ALIGN)
       &&   ((IS_GOOD(rch)    && IS_GOOD(ch))
         ||  (IS_EVIL(rch)    && IS_EVIL(ch))
         ||  (IS_NEUTRAL(rch) && IS_NEUTRAL(ch))))

    ||   (rch->pIndexData == ch->pIndexData
       && IS_SET(rch->off_flags,ASSIST_VNUM)))

      {
        CHAR_DATA *vch;
        CHAR_DATA *target;
        int number;

        if (number_bits(1) == 0)
      continue;

        target = NULL;
        number = 0;
        for (vch = ch->in_room->people; vch; vch = vch->next)
        {
      if (can_see(rch,vch,FALSE)
      &&  is_same_group(vch,victim)
      &&  number_range(0,number) == 0)
      {
          target = vch;
          number++;
      }
        }

        if (target != NULL)
        {
      do_emote(rch,"screams and attacks!");
      multi_hit(rch,target,TYPE_UNDEFINED);
        }
    }
      }
  }
    }
}


/*
 * Do one group of attacks.
 */
void multi_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int     chance;
     int        wsn;
     int        counter;
     int  number_of_crusaders =0;

/* commented out to stop the useless log spam
 *
   sprintf(log_buf,"%s attacking %s",ch->name,victim->name);
   log_string(log_buf);
 */

    /* decrement the wait */
    if (ch->desc == NULL)
  ch->wait = UMAX(0,ch->wait - PULSE_VIOLENCE);

    if (ch->desc == NULL)
  ch->daze = UMAX(0,ch->daze - PULSE_VIOLENCE);

    /*no attacks for wraithform ppl */
    if (is_affected(ch, skill_lookup("wraithform")) )
    return;
/*no attacks for ppl just logged in*/
   if (is_clan(ch) && !IS_NPC(victim) && ch->pcdata->start_time > 0 )
      return;

    /* no attacks for stunnies -- just a check */
    if (ch->position <= POS_RESTING)
  return;

    if (IS_NPC(ch))
    {
  mob_hit(ch,victim,dt);
  return;
    }

    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
        send_to_char("(Reg) ",ch);
    if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 5 && IS_NPC(victim) && number_percent() > 66 && (dt != gsn_backstab  ||  dt != gsn_kcharge) && ch->fighting == victim)
        do_deity_msg("Your blow is held back by your trial from %s", ch);// Trial 5: Reduced attack rate
    else
        one_hit( ch, victim, dt );

    if (ch->fighting != victim)
  return;

    if (is_mounted(ch) && !is_mounted(victim))
    {
       if(number_percent() < UMIN(10, get_skill(ch,gsn_riding) / 5 ))
       {
          check_improve(ch,gsn_riding,TRUE,10);
          if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
             send_to_char("(Mnt) ",ch);
          one_hit(ch,victim,dt);
       }
       else
          check_improve(ch,gsn_riding,FALSE,10);
    }

    if (ch->fighting != victim)
  return;

    if ( ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL &&
         IS_WEAPON_STAT( wield, WEAPON_FAVORED ) &&
         number_percent() < ( ch->pcdata->sac / 8 )
         && (dt != gsn_backstab  ||  dt != gsn_kcharge) )
    {
        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Leg) ",ch);

        one_hit(ch,victim,dt);
    }

    if ( ch->fighting != victim )
    return;

    if (IS_AFFECTED(ch,AFF_HASTE) )
    {
        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Hst) ",ch);
        if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 5 && IS_NPC(victim) && number_percent() > 50 && (dt != gsn_backstab  ||  dt != gsn_kcharge))
            do_deity_msg("Your blow is held back by your trial from %s", ch);// Trial 5: Reduced attack rate
        else
            one_hit(ch,victim,dt);
    }

    if ( ch->fighting != victim )
  return;

    {
        AFFECT_DATA *paf;

    /* ONE nice backstab or kcharge, or extra regular attack */
    if ( (paf = affect_find(ch->affected,gsn_spirit_of_wolf) ) != NULL )
    {
        paf->duration--;

        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Wlf) ",ch);
        one_hit(ch,victim,dt);

        if ( dt == gsn_backstab || dt == gsn_kcharge )
            paf->duration = 0;

        if ( --paf->duration <  0 )
        {
            affect_remove(ch,paf,APPLY_BOTH);
            act("The spirit of wolf leaves you.",ch,NULL,NULL,TO_CHAR,FALSE);
        }
    }
    } /* end local declaration for 'paf' */

    if ( ch->fighting != victim || dt == gsn_backstab || dt == gsn_kcharge)
  return;

    number_of_crusaders = group_has_how_many_crusader( ch ) ;
    if (    number_of_crusaders
         && ( number_percent() < ( ch->pcdata->sac / 6) )
       )
    {
            for ( counter = 1; counter <= number_of_crusaders; counter++)
            {
                if   ( number_percent() < (100/counter) )
                {
                        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                                send_to_char("(Cru) ",ch);
                        one_hit(ch,victim,dt);
                }
            }
    }

  /*  if ( group_has_crusader( ch ) &&
     number_percent() < ( ch->pcdata->sac / 6 ) )
    {
        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Cru) ",ch);
        one_hit(ch,victim,dt);
    }
*/
/*Here comes the cavalier attack it goes off all the time but less than the crusader attack */
/* Added 19SEP01 Boogums*/

    if (!IS_NPC(ch) && group_has_cavalier( ch ) &&
     number_percent() < ( ch->pcdata->sac / 9 ) )
    {
        if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Ral) ",ch);
        one_hit(ch,victim,dt);
    }

    chance = get_skill(ch,gsn_second_attack)/2;

    if (get_skill(ch,gsn_second_attack) > 1)
    {
       if ( is_affected(ch,gsn_dae_tok) )
       {
          check_improve(ch,gsn_dae_tok,TRUE,5);
          chance += UMAX(0,get_skill(ch,gsn_dae_tok)-90);
       }

        if ( is_affected(ch,gsn_spirit_of_cat) )
            chance += ch->level/5;

       chance += check_myrmidon( ch, gsn_second_attack );

       if (IS_AFFECTED(ch,AFF_SLOW))
          chance /= 2;

       if ( number_percent( ) < chance )
       {
          if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
             send_to_char("(2nd) ",ch);
          if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 5 && IS_NPC(victim) && number_percent() > 50)
                do_deity_msg("Your blow is held back by your trial from %s", ch);// Trial 5: Reduced attack rate
          else
              one_hit( ch, victim, dt );
          check_improve(ch,gsn_second_attack,TRUE,5);
          if ( ch->fighting != victim )
             return;
       }
    }

    if ( IS_AFFECTED(ch,AFF_SLOW) )
        return;

    chance = get_skill(ch,gsn_third_attack)/4;

    if (get_skill(ch,gsn_third_attack) > 1)
    {
       chance += check_myrmidon( ch, gsn_third_attack );

       if (!IS_NPC(ch) &&
          ( ch->pcdata->old_class != class_lookup("warrior") &&
            ch->pcdata->old_class!=class_lookup("thief")))
                chance -= ( chance / 4 );

       if ( is_affected(ch,gsn_dae_tok) )
       {
          check_improve(ch,gsn_dae_tok,TRUE,8);
          chance += UMAX(0,get_skill(ch,gsn_dae_tok)-90);
       }

        if ( is_affected(ch,gsn_spirit_of_cat) )
            chance += ch->level/5;

    }

    if ( number_percent( ) < chance )
    {
  if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
      send_to_char("(3rd) ",ch);
  one_hit( ch, victim, dt );
  check_improve(ch,gsn_third_attack,TRUE,6);
  if ( ch->fighting != victim )
      return;
    }


    if ( number_percent() < get_skill(ch, gsn_fourth_attack) / 4 )
    {
   if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
        send_to_char("(4th) ",ch);
        one_hit( ch, victim, dt );
        check_improve(ch,gsn_fourth_attack,TRUE,8);
        if ( ch->fighting != victim )
                return;
     }

     if (get_eq_char( ch, WEAR_SECOND ) != NULL)
     {
        int chance = get_skill(ch,gsn_dual_wield);

        if ( ch->class == class_lookup("blademaster") )
                chance = 3 * chance / 5;
        else
                chance /= 4;

        if ( is_affected(ch,gsn_spirit_of_cat) )
            chance += ch->level/5;

        if ( number_percent() <  chance )
        {

           if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
              send_to_char("(Dua) ",ch);
           one_hit( ch, victim, TYPE_SECONDARY );
           check_improve(ch,gsn_dual_wield,TRUE,9);
           if ( ch->fighting != victim )
             return;

           /* Favored on the secondary?
           if ( ( wield = get_eq_char( ch, WEAR_SECOND ) ) != NULL &&
                  IS_WEAPON_STAT( wield, WEAPON_FAVORED ) &&
                  number_percent() < ( ch->pcdata->sac / 8 )
                  && dt != gsn_backstab )
          {
             if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Fav) ",ch);
             one_hit(ch,victim,dt);
          }
          */
        }
     }

     /* special attack for rogues */
    if (ch->class == class_lookup("rogue")
    && get_skill(ch,gsn_cutpurse) > 1)
    do_cutpurse(ch,victim);


    /* specialization */
    if ( IS_NPC(ch) )
        return;

    if ( ( wsn = get_weapon_sn(ch,FALSE) )  == ch->pcdata->specialize )
        if ( number_percent() < get_weapon_skill(ch,wsn) / 10 )
    {
        if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Spe) ",ch);
                one_hit( ch, victim, dt );
        if ( ch->fighting != victim )
                return;

    }

    if ( is_affected(ch,gsn_bladesong) &&
         (((get_skill(ch,gsn_bladesong) - 95) * 5) > number_percent()) )
     {
        if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Bla) ",ch);
        one_hit( ch, victim, dt );
        if ( ch->fighting != victim )
                return;
     }

    if ( is_affected(ch,gsn_rage) &&
           (((get_skill(ch,gsn_rage) - 95 ) * 5) > number_percent()) )
    {
        if (!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("(Bat) ",ch);
        one_hit( ch, victim, dt );
        if (ch->fighting != victim )
                return;
    }

    return;
}

/* procedure for all mobile attacks */
void mob_hit (CHAR_DATA *ch, CHAR_DATA *victim, int dt)
{
    int chance,number;
    CHAR_DATA *vch, *vch_next;

    one_hit(ch,victim,dt);

    if (ch->fighting != victim)
  return;

    /* Area attack -- BALLS nasty! */

    if (IS_SET(ch->off_flags,OFF_AREA_ATTACK) || is_shaded(ch))
    {
  for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
  {
      vch_next = vch->next;
      if ((vch != victim && vch->fighting == ch))
    one_hit(ch,vch,dt);
  }
    }

    if (IS_AFFECTED(ch,AFF_HASTE)
    ||  (IS_SET(ch->off_flags,OFF_FAST) && !IS_AFFECTED(ch,AFF_SLOW)))
  one_hit(ch,victim,dt);

    if (ch->fighting != victim || dt == gsn_backstab || dt == gsn_kcharge)
  return;

    chance = get_skill(ch,gsn_second_attack)/2;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
  chance /= 2;

    if (number_percent() < chance)
    {
  one_hit(ch,victim,dt);
  if (ch->fighting != victim)
      return;
    }

    chance = get_skill(ch,gsn_third_attack)/4;

    if (IS_AFFECTED(ch,AFF_SLOW) && !IS_SET(ch->off_flags,OFF_FAST))
  chance = 0;

    if (number_percent() < chance)
    {
  one_hit(ch,victim,dt);
  if (ch->fighting != victim)
      return;
    }

    /* oh boy!  Fun stuff! */

    if (ch->wait > 0)
  return;

    number = number_range(0,2);

    if (number == 1 && IS_SET(ch->act,ACT_MAGE))
    {
  /*  { mob_cast_mage(ch,victim); return; } */ ;
    }

    if (number == 2 && IS_SET(ch->act,ACT_CLERIC))
    {
  /* { mob_cast_cleric(ch,victim); return; } */ ;
    }

    /* now for the skills */

    number = number_range(0,8);

    switch(number)
    {
    case (0) :
  if (IS_SET(ch->off_flags,OFF_BASH))
      do_bash(ch,"");
  break;

    case (1) :
  if (IS_SET(ch->off_flags,OFF_BERSERK) && !IS_AFFECTED(ch,AFF_BERSERK))
      do_berserk(ch,"");
  break;


    case (2) :
  if (IS_SET(ch->off_flags,OFF_DISARM)
  || (get_weapon_sn(ch,FALSE) != gsn_hand_to_hand
  && (IS_SET(ch->act,ACT_WARRIOR)
    ||  IS_SET(ch->act,ACT_THIEF))))
      do_disarm(ch,"");
  break;

    case (3) :
  if (IS_SET(ch->off_flags,OFF_KICK))
      do_kick(ch,"");
  break;

    case (4) :
  if (IS_SET(ch->off_flags,OFF_KICK_DIRT))
      do_dirt(ch,"");
  break;

    case (5) :
  if (IS_SET(ch->off_flags,OFF_TAIL))
  {
         do_tail_slap(ch,"");
  }
  break;

    case (6) :
  if (IS_SET(ch->off_flags,OFF_TRIP))
      do_trip(ch,"");
  break;

    case (7) :
  if (IS_SET(ch->off_flags,OFF_CRUSH))
  {
      /* do_crush(ch,"") */ ;
  }
  break;
    case (8) :
  if (IS_SET(ch->off_flags,OFF_BACKSTAB))
  {
      do_backstab(ch,"");
  }
    }
}


/*
 * Hit one guy once.
 */
void one_hit( CHAR_DATA *ch, CHAR_DATA *victim, int dt )
{
    OBJ_DATA *wield;
    int victim_ac;
    int thac0;
    int thac0_00;
    int thac0_32;
    int dam, base_dam = 0;
    int diceroll;
    int diceroll_save;
    int sn,skill;
    int dam_type;
    bool result;
    bool fSecondary = FALSE;

    sn = -1;


    /* just in case */
    if (victim == ch || ch == NULL || victim == NULL)
  return;

    /*
     * Can't beat a dead char!
     * Guard against weird room-leavings.
     */
    if ( victim->position == POS_DEAD || ch->in_room != victim->in_room )
  return;

    /*
     * Figure out the type of damage message.
     */
    wield = get_eq_char( ch, WEAR_WIELD );

    /* Use the off weapon for dual wield.  This requires that they have
       a weapon in each hand.  Minor bug: can still improve at dual wield
       regardless of whether or not you're wearing a weapon */
    if ( dt == TYPE_SECONDARY )
    {
       if ( wield == NULL )
           return;

       if ( ( wield = get_eq_char( ch, WEAR_SECOND ) ) == NULL )
           return;

       fSecondary = TRUE;
    }

    if ( dt == TYPE_UNDEFINED || dt == TYPE_SECONDARY )
    {
  dt = TYPE_HIT;
  if ( wield != NULL && wield->item_type == ITEM_WEAPON )
      dt += wield->value[3];
  else {
        if( IS_AFFECTED(ch,AFF_MORPH) )
        dt += 5;
        else if( IS_AFFECTED(ch,AFF_WEAPONRY))
        dt += 13;
        else dt += ch->dam_type;
        }
    }

    if (dt < TYPE_HIT)
      if (wield != NULL)
          dam_type = attack_table[wield->value[3]].damage;
      else {
          if( IS_AFFECTED(ch,AFF_MORPH) )
          dam_type = DAM_SLASH;
          else if(IS_AFFECTED(ch,AFF_WEAPONRY))
          dam_type = DAM_MENTAL;
          else dam_type = attack_table[ch->dam_type].damage;
           }
    else
      dam_type = attack_table[dt - TYPE_HIT].damage;

    if(IS_SET(ch->mhs, MHS_GLADIATOR) && gladiator_info.exper == TRUE)
    {/* Blind gladiators you always vuln */
      dam_type = DAM_VULN;
      if(dt >= TYPE_HIT)
        dt = TYPE_HIT + MAX_DAMAGE_MESSAGE;
    }
    else if (dam_type == -1)
  dam_type = DAM_BASH;

    /* get the weapon skill */
    sn = get_weapon_sn(ch,fSecondary);
    skill = 20 + get_weapon_skill(ch,sn);

        /* Races that suck with regular weapons */
    if ( ch->race == race_lookup("faerie") )
        skill -= ( skill / 4 );

        /* Adjustment for druids -- removed
    if ( ch->class == class_lookup("druid") )
        skill -= ( skill / 20 );
        **/


  /*Adjustment for Elementalist, they aren't supposed to fight good*/
  if ( ch->class == class_lookup("elementalist"))
  {
    skill -= ( skill / number_range(15,25) );
  }

        /* Adjust for Posse Clan Skill -Boogums*/
/*      if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_KILLER) )
        {
          skill += (skill * .14);
        }
        if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_THUG) )
        {
          skill += (skill * .08);
        }
        if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_RUFFIAN) )
        {
          skill += (skill * .05);
        }
        if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_THIEF) )
        {
          skill += (skill * .11);
        }*/


        /* Ajdust for species enemy */
    if ( is_enemy( ch, victim ) )
        skill += number_range( ch->level / 4, ch->level / 2 );

        /* Adjust for terrain */
    switch( terrain(ch) )
    {
    case 0:
        skill -= ( skill / 10 ); break;
    case 1:
        skill += ( skill / 10 ); break;
    default:
        break;
    }

    if(wield != NULL && !IS_NPC(ch) && ch->pcdata->weapon_too_heavy)
      skill = 1; /* You can't use it */

    /*
     * Calculate to-hit-armor-class-0 versus armor.
     */
    if ( IS_NPC(ch) )
    {
  thac0_00 = 20;
  thac0_32 = -4;   /* as good as a thief */
  if (IS_SET(ch->act,ACT_WARRIOR))
      thac0_32 = -10;
  else if (IS_SET(ch->act,ACT_THIEF))
      thac0_32 = -4;
  else if (IS_SET(ch->act,ACT_CLERIC))
      thac0_32 = 2;
  else if (IS_SET(ch->act,ACT_MAGE))
      thac0_32 = 6;
    }
    else
    {
  thac0_00 = class_table[ch->class].thac0_00;
  thac0_32 = class_table[ch->class].thac0_32;
    }
    thac0  = interpolate( ch->level, thac0_00, thac0_32 );

    if (thac0 < 0)
        thac0 = thac0/2;

    if (thac0 < -5)
        thac0 = -5 + (thac0 + 5) / 2;

    if (fSecondary)
       thac0 -= GET_SECOND_HITROLL(ch) * skill/100;
    else
       thac0 -= GET_HITROLL(ch) * skill/100;

    thac0 += 5 * (100 - skill) / 100;

    if (dt == gsn_backstab)
  thac0 -= 10 * (100 - get_skill(ch,gsn_backstab));

    /*Added by Boogums 06SEP00 */
    if (dt == gsn_kcharge)
      {
        thac0 -= 10 * (100 - get_skill(ch,gsn_kcharge));
      }

    switch(dam_type)
    {
  case(DAM_PIERCE):victim_ac = GET_AC(victim,AC_PIERCE)/10; break;
  case(DAM_BASH):  victim_ac = GET_AC(victim,AC_BASH)/10;   break;
  case(DAM_SLASH): victim_ac = GET_AC(victim,AC_SLASH)/10;  break;
  default:   victim_ac = GET_AC(victim,AC_EXOTIC)/10; break;
    };

    /* Set up AC curves */
    /* For CASTER, the curve begins at -100 and hits again at -200
     * For MELEE, the curve begins at -160 and hits again at -320
     * For HYBRID, the curve begins at -130 and hits again at -260
     */

#define MELEE   0
#define HYBRID  1
#define CASTER  2

        switch ( class_table[victim->class].fMana )
        {
        case MELEE:
            if (victim_ac < -16)
                victim_ac = (victim_ac + 16) / 5 - 16;
            if (victim_ac < -32 )
                victim_ac = (victim_ac +32) / 2 - 32;
            break;
        case CASTER:
            if (victim_ac < -10 )
                victim_ac = (victim_ac + 10) / 5 - 10;
            if (victim_ac < -20 )
                victim_ac = (victim_ac + 20 ) /2 - 20;
            break;
        case HYBRID:
            if (victim_ac < -13 )
                victim_ac = (victim_ac + 13 ) /5 - 13;
            if (victim_ac < -26 )
                victim_ac = (victim_ac + 26 ) /2 - 26;
            break;
        }

#undef MELEE
#undef HYBRID
#undef CASTER

    if ( !can_see( ch, victim, FALSE ) )
  victim_ac -= 4;

    if ( victim->position < POS_FIGHTING)
  victim_ac += 4;

    if (victim->position < POS_RESTING)
  victim_ac += 6;

    if( IS_AFFECTED(victim,AFF_WEAPONRY))
  victim_ac -= 1;

  if (ch->class == 3)
  victim_ac += 2;

  if ( is_enemy(ch,victim) )
      victim_ac += 4;

  if ( is_mounted(ch) && !is_mounted(victim) )
  {
        victim_ac += ( victim_ac / 10 );
        if ( number_percent() < get_skill(ch,gsn_riding) )
        victim_ac += ( victim_ac / 10 );
  }

  if ( is_mounted(victim) && !is_mounted(ch) )
  {
        victim_ac -= ( victim_ac / 10 );
        if ( number_percent() < get_skill(victim,gsn_riding) )
        victim_ac -= ( victim_ac / 10 );
  }

  /* Terrain */
  switch( terrain(ch) )
  {
  case 0:       victim_ac -= 3; break;
  case 1:       victim_ac += 3; break;
  default:      break;
  }
  /* check diety and clan for Almigty */
  if ( !IS_NPC(victim)  && !IS_NPC(ch)
      && victim->clan == nonclan_lookup("zealot")
      && ch->pcdata->deity == deity_lookup("almighty")
      && !IS_SET(ch->mhs, MHS_GLADIATOR)
      && !IS_SET(victim->mhs, MHS_GLADIATOR))
         {
            ch->pcdata->sac = 0;
            if (!IS_SET(ch->mhs, MHS_BANISH))
               SET_BIT(ch->mhs, MHS_BANISH);
         }

    /*
     * The moment of excitement!
     */
    while ( ( diceroll = number_bits( 5 ) ) >= 20 ) ;

    if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_WERE)  &&
         is_affected(ch,gsn_morph) && ( ch->hit < ch->max_hit / 4 ) )
         diceroll = 19;

    if ( diceroll == 0
    || ( diceroll != 19 && diceroll < thac0 - victim_ac ) )
    {
  /* Miss. */
  damage( ch, victim, 0, dt, dam_type, TRUE, FALSE );
  if ( is_affected(ch,gsn_fumble) && wield != NULL)/*  &&
        !IS_OBJ_STAT(wield,ITEM_NOREMOVE) )*/
      if ( number_percent() > (get_curr_stat(ch,STAT_DEX)*5) && !is_flash_affected(victim, skill_lookup("disarm")) )
      {
        AFFECT_DATA af;
        af.where     = TO_AFFECTS;
        af.type      = gsn_disarm;
        af.level     = ch->level;
        af.location  = APPLY_HITROLL;
        af.modifier  = ch->level / -2;
        af.duration  = 2;
        flash_affect_to_char( ch, &af );
        af.location = APPLY_DAMROLL;
        flash_affect_to_char( ch, &af );
        act("Your hand feels clumsy and weak!",ch,NULL,NULL,TO_CHAR,FALSE);
        act("$N suddenly looks uncertain of $s grip on $s weapon!",ch,NULL,ch,TO_ROOM,FALSE);

         //act("You fumble and drop your weapon!",ch,NULL,NULL,TO_CHAR,FALSE);
         //act("$N fumbles and drops $s weapon!",ch,NULL,ch,TO_ROOM,FALSE);
         /*obj_from_char( wield );
         if ( IS_OBJ_STAT(wield,ITEM_NODROP) )
                obj_to_char( wield, ch );
         else
                obj_to_room( wield, ch->in_room );*/
      }
  tail_chain( );
  return;
    }

    diceroll_save = diceroll;

    /*
     * Hit.
     * Calc damage.
     */
    if ( IS_NPC(ch) && (!ch->pIndexData->new_format || wield == NULL))
  if (!ch->pIndexData->new_format)
  {
      dam = number_range( ch->level / 2, ch->level * 3 / 2 );
      if ( wield != NULL )
        dam += dam / 2;
  }
  else
      dam = dice(ch->damage[DICE_NUMBER],ch->damage[DICE_TYPE]);

    else
    {
  if (sn != -1)
      check_improve(ch,sn,TRUE,5);
  if ( wield != NULL )
  {
      if (wield->pIndexData->new_format)
        {
        dam = dice(wield->value[1],wield->value[2]);
        if( dam == (wield->value[1]*wield->value[2])
          && IS_WEAPON_STAT(wield,WEAPON_VORPAL)  && !IS_NPC(ch) )
          {
          if(number_percent() < get_skill(ch,gsn_vorpal) - 4 )
                {
                damage( ch, victim, victim->hit/2, dt, dam_type, TRUE ,FALSE);
                check_improve(ch,gsn_vorpal,TRUE,1);
                }
          else
                {
                if(!IS_NPC(ch))
                  { damage( ch, ch, ch->hit/2, dt, dam_type, TRUE ,FALSE); }
                check_improve(ch,gsn_vorpal,FALSE,2);
                }
          }
        dam = (dam * skill)/100;
        }
      else
        dam = number_range( wield->value[1] * skill/100,
        wield->value[2] * skill/100);


      /* use base damage for all adjustments */
      /* if you change anything from this point on, */
      /* be sure to use (dam += base_dam * modifier ) */

      base_dam = dam;


      if (get_eq_char(ch,WEAR_SHIELD) == NULL)  /* no shield = more */
    dam += base_dam / 10;


      /* sharpness! */
      if (IS_WEAPON_STAT(wield,WEAPON_SHARP))
      {
    int percent;

    if ((percent = number_percent()) <= (skill / 8))
        dam += 2 * base_dam + (base_dam * 2 * percent / 100);
      }
  }
  else{
      dam = number_range( 1 + 4 * skill/100, 2 * ch->level/3 * skill/100);
      base_dam = dam;

      if( get_skill(ch,gsn_morph) > number_percent()
          && IS_AFFECTED(ch,AFF_MORPH))
        dam += (base_dam * get_skill(ch,gsn_morph)) / 130;
      if( IS_AFFECTED(ch,AFF_WEAPONRY) )
        dam += base_dam/10;
      }
    }

    /* Rangers! */
    if ( is_enemy( ch, victim ) )
        dam += ( base_dam / 5 ); /* 20% hbonus */

    /*
     * Bonuses.
     */
    if ( get_skill(ch,gsn_enhanced_damage) > 0 )
    {
        diceroll = number_percent();
        if (diceroll <= get_skill(ch,gsn_enhanced_damage))
        {
            check_improve(ch,gsn_enhanced_damage,TRUE,6);
            dam += 2 * ( base_dam * diceroll/ 300);
        }
    }

    /*
     * Kurijitsu
     */
   if ( number_percent() < (get_skill(ch,gsn_kurijitsu)/6) )
   {
           check_improve(ch,gsn_kurijitsu,TRUE,6);
           diceroll = number_percent();
           do
                dam += 2 * ( base_dam * diceroll/300);
           while ( --diceroll > 95 );
   }

    if ( !IS_AWAKE(victim) )
        dam += base_dam;
     else if (victim->position < POS_FIGHTING)
        dam += base_dam / 2;

    if ( victim->fighting != ch
      && (!IS_SET(victim->mhs, MHS_GLADIATOR) || (gladiator_info.exper == FALSE && gladiator_info.WNR == FALSE)))
       dam += base_dam / 2;

    if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_WERE)  &&
         is_affected(ch,gsn_morph) && (ch->hit < ch->max_hit/4) )
        dam += base_dam * 9/10;

    if ( dt == gsn_backstab && wield != NULL)
    {
      if ( wield->value[0] != 2 )
      dam += base_dam * (2 + (ch->level / 12));
  else
      dam += base_dam * (2 + (ch->level / 10));

      if (ch->class == class_lookup("rogue"))
        dam += base_dam/10;

      if (ch->fighting != NULL)
        dam /= 2;

     }

     /* 6SEP00 - Adding Damage for kcharge - Boogums */
     if ( wield != NULL && !fSecondary
        && dt == gsn_kcharge && wield->value[0] != WEAPON_POLEARM)
     {
       dam += base_dam * (2 + (ch->level / 10));
       if (ch->fighting != NULL){dam /= 2;}
     }

     if ( sn == gsn_hand_to_hand &&
          ( ch->race == race_lookup("rockbiter") ||
            is_affected(ch,gsn_stonefist) ) )
          dam += ch->level / 3;

        /* so do ogres.  just not quite as much.  and giants */
    if ( sn == gsn_hand_to_hand &&
        ( (ch->race == race_lookup("ogre") || ch->race == race_lookup("giant") ) ) )// Giants and ogres are not getting pounding bonus during blind glads, sorry
        dam += ch->level / 4;


    if(fSecondary)
       dam += GET_SECOND_DAMROLL(ch) * UMIN(100,skill) /100;
    else
       dam += GET_DAMROLL(ch) * UMIN(100,skill) /100;

    if(wield != NULL && wield->damaged >= 100)
        dam -= dam / 2;//Half damage after the bonuses for a destroyed weapon

    if ( dam <= 0 )
      dam = 1;


  /* Size Adjustment - Larger races do more damage  - removed
        dam += (dam * (ch->size - 2)/10);
 */

    if ( ch->race == race_lookup("rockbiter"))
       dam += (base_dam * (ch->size -2)/10);

    /* CLass adjustment -
       Adjust damage bsed on class and oldclass

    if( !IS_NPC(ch))
      dam = dam * ( class_table[ch->class].dam_mod
                  * class_table[ch->pcdata->old_class].dam_mod) /10000 ; */

    if (!IS_NPC(ch))
    {
    switch (ch->pcdata->old_class)
        {
        case CLASS_MAGE:
                dam = dam * 85 /100;
                break;
        case CLASS_THIEF:
                dam = dam * 105 / 100;
                break;
        case CLASS_WARRIOR:
                dam = dam * 110 / 100;
                break;
        default:
        case CLASS_CLERIC:
                dam = dam * 95 / 100;
                break;
        }
    }

    if ( is_affected(ch,gsn_spirit_of_bear) )
        dam += base_dam / 10;

/* Handle critical strikes */
    handle_critical(  ch, &dam,  dam_type, diceroll_save, base_dam );

    if(is_shaded(ch))
    {// Shades do a flat 70% damage while shadowed
      dam = dam * 7 / 10;
    }  

    result = damage( ch, victim, dam, dt, dam_type, TRUE ,FALSE);
    if(ch->fighting == victim && IS_NPC(victim) && result > 0 &&
      IS_SET(ch->off_flags, OFF_BANE_TOUCH))
    {
      int dam_cur = victim->hit / 5;//20% current
      int dam_max = victim->max_hit / 20;//5% max

      if(dam_cur > dam_max)
      {
        act("$n staggers as the strike {rrends{x $m!", victim, NULL, ch, TO_ROOM, FALSE);
        act("$n is staggered by your powerful strike!", victim, NULL, ch, TO_VICT, FALSE);
        act("You are staggered by $N's powerful strike!", victim, NULL, ch, TO_CHAR, FALSE);
        DAZE_STATE(victim, PULSE_VIOLENCE);
      }
      else
      {
        dam_cur = dam_max;
        act("$n is knocked down as the strike {rrends{x $m!", victim, NULL, ch, TO_ROOM, FALSE);
        act("$n is knocked down by your powerful strike!", victim, NULL, ch, TO_VICT, FALSE);
        act("You are knocked down by $N's powerful strike!", victim, NULL, ch, TO_CHAR, FALSE);
        DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
      }
#ifndef ANDARONDEV
      dam_cur *= 4;// To deal with the damage curve currently on live
#endif
      damage(ch, victim, dam_cur, 0, dam_type, FALSE, FALSE);
    }

    /* but do we have a funky weapon? */
    if (result && wield != NULL)
    {
  int dam=0;

  if ( ch->fighting == victim && is_affected(ch,gsn_midnight_cloak) && ( victim->level - ch->level ) >= -5 )
         /* no more than 5 levels above vict */
   {
      if ( time_info.hour >= 20 || time_info.hour <= 5 )
        ch->hit++;
      if ( time_info.hour == 0 )
        ch->hit += 4;
   }

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_POISON))
  {
      int level;
      AFFECT_DATA *poison, af;

      if ((poison = affect_find(wield->affected,gsn_poison)) == NULL)
    level = wield->level;
      else
    level = poison->level;

      if (!saves_spell(level / 2,victim,DAM_POISON))
      {
    send_to_char("You feel poison coursing through your veins.",
        victim);
    act("$n is poisoned by the venom on $p.",
        victim,wield,NULL,TO_ROOM,FALSE);

        af.where     = TO_AFFECTS;
        af.type      = gsn_poison;
        af.level     = level * 3/4;
        af.duration  = level / 2;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = AFF_POISON;
        affect_join( victim, &af );
      }

      /* weaken the poison if it's temporary */
      if (poison != NULL)
      {
        poison->level = UMAX(0,poison->level - 2);
        poison->duration = UMAX(0,poison->duration - 1);

        if (poison->level == 0 || poison->duration == 0) {
          act("The poison on $p has worn off.",ch,wield,NULL,TO_CHAR,FALSE);
          affect_remove_obj ( wield, poison );
        }
      }
  }

      if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_NETHER))
  {
      dam = number_range( 1, wield->level /5 + 1);
      /* can only use nether if the char has the kit
         no damage done to char if not a nethermancer */
      if ( HAS_KIT(ch,"nethermancer") )
      {
      act("$p bleeds the soul of $n.",victim,wield,NULL,TO_ROOM,FALSE);
      act("$p bleeds your soul.",victim,wield,NULL,TO_CHAR,FALSE);
      damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE,TRUE);
      ch->hit += dam /3; ch->mana += dam/3; ch->move += dam /3;
      gain_exp(ch, ( number_percent() < ( ch->level / 10 ) ) ? 1 : 0 );
      }
  }
      if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_VAMPIRIC))
  {
      /*Make Necromancer's hit harder - Boogums*/
      if (ch->kit== kit_lookup("necromancer") )
      {
        dam = number_range(1, wield->level / 3 + 1);
      }
      else
      {
        dam = number_range(1, wield->level / 5 + 1);
      }
      act("$p draws life from $n.",victim,wield,NULL,TO_ROOM,FALSE);
      act("You feel $p drawing your life away.",
    victim,wield,NULL,TO_CHAR,FALSE);
      damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE,TRUE);
      /*Gladiator's alignment doesnt change */
      if (!IS_SET(ch->mhs,MHS_GLADIATOR) &&
      !is_affected(ch, skill_lookup("indulgence"))
      /*added line below for necromancer kit, their alignment doesn't change*/
      &&
      !(ch->kit== kit_lookup("necromancer")) )
      {
         ch->alignment = UMAX(-1000,ch->alignment - 1);
      }
      ch->hit += dam/2;
  }

/*
  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SCION)
        && IS_SET(victim->mhs,MHS_SAVANT) )
  {
         OOO Sppooky no damage message
        ch->hit++; ch->mana++;
        victim->hit--; victim->mana--;
  }
*/

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FLAMING))
  {
      dam = number_range(1,wield->level / 4 + 1 );
      /* If there's a flame shield, halve the damage */
      if ( is_affected(victim,skill_lookup("flame shield")))
        dam /= 2;
      act("$n is burned by $p.",victim,wield,NULL,TO_ROOM,FALSE);
      act("$p sears your flesh.",victim,wield,NULL,TO_CHAR,FALSE);
      fire_effect( (void *) victim,wield->level/2,dam,TARGET_CHAR);
      damage(ch,victim,dam,0,DAM_FIRE,FALSE,FALSE);
  }

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_FROST))
  {
      dam = number_range(1,wield->level / 6 + 2);
      /* If there's a frost shield, halve the damage */
      if ( is_affected(victim,skill_lookup("frost shield")))
        dam /= 2;
      act("$p freezes $n.",victim,wield,NULL,TO_ROOM,FALSE);
      act("The cold touch of $p surrounds you with ice.",
    victim,wield,NULL,TO_CHAR,FALSE);
      cold_effect(victim,wield->level/2,dam,TARGET_CHAR);
      damage(ch,victim,dam,0,DAM_COLD,FALSE,FALSE);
  }

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_SHOCKING))
  {
      dam = number_range(1,wield->level/5 + 2);
      /* If there's an electric shield, halve the damage */
      if ( is_affected(victim,skill_lookup("electric shield")))
        dam /= 2;
      act("$n is struck by lightning from $p.",victim,wield,NULL,TO_ROOM,FALSE);
      act("You are shocked by $p.",victim,wield,NULL,TO_CHAR,FALSE);
      shock_effect(victim,wield->level/2,dam,TARGET_CHAR);
      damage(ch,victim,dam,0,DAM_LIGHTNING,FALSE,FALSE);
  }

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_HOLY))
  {
      dam = number_range(1,wield->level/5 +2);
      if ( ch->class != class_lookup("paladin") )
      {
         act("$n is consumed in holy wrath by $p.",ch,wield,NULL,TO_ROOM,FALSE);
         act("You are consumed in holy wrath by $p.",ch,wield,NULL,TO_CHAR,FALSE);
         damage(ch,ch,dam,0,DAM_HOLY,FALSE,FALSE);
      }
      else
      {
         if ( ( IS_GOOD(ch) && !IS_GOOD(victim) ) ||
              ( IS_EVIL(ch) && !IS_EVIL(victim) ) ||
              ( IS_NEUTRAL(ch) && !IS_NEUTRAL(victim) ) )
         {
           ch->mana += ( dam / 2 );
           ch->hit += ( dam / 2 );
           if ( ch->pcdata->old_class == class_lookup("warrior") )
           {
             ch->mana += ( dam / 3 );
             ch->hit += ( dam / 3 );
           }
         }
   act("$n is consumed in holy wrath by $p.",victim,wield,NULL,TO_ROOM,FALSE);
   act("You are consumed in holy wrath by $p.",victim,wield,NULL,TO_CHAR,FALSE);
         damage(ch,victim,dam,0,DAM_HOLY,FALSE,FALSE);
         if ( ( IS_GOOD(ch) && IS_EVIL(victim) ) ||
              ( IS_EVIL(ch) && IS_GOOD(victim) ) )
              holy_effect(victim,wield->level,ch->alignment,ch);
      }
  }

  if (ch->fighting == victim && IS_WEAPON_STAT(wield,WEAPON_STUN))
  {
    diceroll = number_percent();
    if (victim->race == race_lookup("kender"))
    diceroll = (3*diceroll)/4;
    /* faeries are easier to stun once you land the damn blow - removed
    if ( victim->race == race_lookup("faerie"))
    diceroll = (4*diceroll)/3;
        */
    if ( diceroll >= 95 + victim->level - wield->level)
     {
    dam = number_range(1,4);
    act("$p knocks down $n.",victim,wield,NULL,TO_ROOM,FALSE);
    act("You feel dazed as $p knocks you over.",
    victim,wield,NULL,TO_CHAR,FALSE);
    damage(ch,victim,dam,0,DAM_NEGATIVE,FALSE,TRUE);
    DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
    victim->position = POS_RESTING;
      }
  }


    }
    tail_chain( );
    return;
}

/*
 * Inflict damage from a hit.
 */
bool damage(CHAR_DATA *ch,CHAR_DATA *victim,int dam,int dt,int dam_type, bool show, bool iOld)
{
    OBJ_DATA *corpse;
    bool immune,surprised=FALSE;
    char cdbuf[MAX_STRING_LENGTH];
    char wdbuf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    CHAR_DATA *gch,*gch_next, *kch;
    char toast[15] = "toasted";
    bool fSecondary = FALSE;
    bool kill_by_plain_mob = TRUE;
    bool fDOT = FALSE;
    int base_dam;
    AFFECT_DATA *paf;
    bool primary_undead = FALSE;
    bool secondary_undead = FALSE;
    bool self_undead = FALSE;
    int extra = 0;
    bool is_dead = FALSE;

    ROOM_INDEX_DATA *died_in_room = ch->in_room;
    //OBJ_DATA *weapon;
//moved the necromancer check into here
    if(dam_type == DAM_LIGHT && IS_SET(ch->in_room->room_affects, RAFF_SHADED))
    {
      if(show)
      {
        send_to_char("The light from your attack is swallowed by the shadows.\n\r", ch);
        act("The light from $n's attack is swallowed by the shadows.", ch, NULL, NULL, TO_ROOM, FALSE);
      }
      return FALSE;
    }

#ifdef COREY_TEST
  if( HAS_KIT(ch,"necromancer")  && ch->alignment < 0 )
  {
  if(
      number_percent() * number_percent() < 1       &&
      ((weapon = get_eq_char(ch,WEAR_WIELD)) != NULL) &&
      number_percent() / 2 < ch->level / (weapon->enchanted?4:2)  &&
      (!IS_SET(weapon->value[4],WEAPON_VAMPIRIC))  &&
      (!IS_SET(weapon->value[4],WEAPON_FAVORED))
    )
    {
      SET_BIT(weapon->value[4],WEAPON_VAMPIRIC);
      act("{YWICKED!{x  $p suddenly looks a bit more {Devil{x.",ch,weapon,NULL,TO_CHAR,FALSE);
      act("{YWICKED!{x  $p suddenly looks a bit more {Devil{x.",ch,weapon,NULL,TO_ROOM,FALSE);
    }
  }
#endif

     /* See if this is a damage over time source */
    if ( dt == TYPE_DOT )
        fDOT = TRUE;

    if ( victim->position == POS_DEAD )
       return FALSE;

    if (dt == TYPE_SECONDARY)
    {
       OBJ_DATA *wield;
       wield = get_eq_char( ch, WEAR_SECOND );

       fSecondary = TRUE;

       dt = TYPE_HIT;
       if ( wield != NULL && wield->item_type == ITEM_WEAPON )
          dt += wield->value[3];
       else
       {
          if( IS_AFFECTED(ch,AFF_MORPH) )
             dt += 5;
          else if( IS_AFFECTED(ch,AFF_WEAPONRY))
             dt += 13;
          else dt += ch->dam_type;
       }
    }

    /*
     * Stop up any residual loopholes.
     */
    if ( dam > 1200 && dt >= TYPE_HIT)
    {
       sprintf(log_buf,"Damage (%d) from %s: more than 1200 points!",dam,ch->name);
       bug( log_buf, 0 );
       dam = 1200;
       if (!IS_IMMORTAL(ch))
       {
          OBJ_DATA *obj;
          if (fSecondary)
             obj = get_eq_char( ch, WEAR_SECOND );
          else
             obj = get_eq_char( ch, WEAR_WIELD );

          if ( obj != NULL && !IS_WEAPON_STAT(obj,WEAPON_VORPAL) )
          {
             send_to_char("You really shouldn't cheat.\n\r",ch);
             /*
                extract_obj(obj);  */
          }
       }
    }


    /* damage reduction */
    if ( dam > 40)
       dam = (dam - 40)/2 + 40;
    if ( dam > 80)
       dam = (dam - 80)/2 + 80;


    base_dam = dam;

    /* Damage bonus for shogun */
    if ( !IS_NPC(ch) && HAS_KIT(ch,"shogun") )
    {
        int total_groupies=0;

        /* Sword bonus */
        if ( check_hai_ruki( ch ) )
        {
            dam += (base_dam/10);
            check_improve(ch,skill_lookup("hai-ruki"),TRUE,10); /* only check on success */
        }

        /* player counts as 1 */
        total_groupies = count_groupies_in_room( ch );

        if ( total_groupies > 1 )
            dam += (((100+(5*total_groupies))*base_dam/100) - base_dam);
    }
    else
    if ( shogun_in_group(ch) )
    {
        int total_groupies;

        total_groupies = count_groupies_in_room( ch );

        if( total_groupies > 1 )
            dam += (((100+(2*total_groupies))*base_dam/100) - base_dam);
    }

/*    if( ch->clan == clan_lookup("demise") &&
        is_affected( victim, skill_lookup("honor guard") )
      )
    {
         dam -= dam/4;
    }*/

    if( dam_type == DAM_FIRE &&
        is_affected(victim,skill_lookup("flame shield")))
       dam /= 2;

    if( dam_type == DAM_COLD &&
        is_affected(victim,skill_lookup("frost shield")))
       dam /= 2;

    if( dam_type == DAM_LIGHTNING &&
        is_affected(victim,skill_lookup("electric shield")))
       dam /= 2;

    /* Improved damage for ogres and giants with bash weapons.
       Does (100% + 1%/2levels) extra damage AFTER the curve  */
    if ( dam_type == DAM_BASH &&
        ( ch->race == race_lookup("ogre") ||
          ch->race == race_lookup("giant") ) )
        dam = ( (ch->level / 2) + 100 ) * (UMAX(base_dam,dam)) / 100;

    /* barbarian endurance -> little randomness */
    if ( number_percent() < ( get_skill(victim,gsn_endurance) / 2 ) )
    {
        check_improve(victim,gsn_endurance,TRUE,1);
        dam = number_range(7,9) * dam / 10;
    }
    else
        check_improve(victim,gsn_endurance,FALSE,4);


    if ( victim != ch )
    {
  /*
   * Certain attacks are forbidden.
   * Most other attacks are returned.
   */
       if ( is_safe( ch, victim ) )
          return FALSE;
       check_killer( ch, victim );

       if ( victim->position > POS_STUNNED )
       {
          if ( ch->fighting == NULL
             && !is_affected(victim,skill_lookup("orb of surprise")) &&
                !fDOT )
             set_fighting( ch, victim );
       }

       if ( victim->position > POS_STUNNED && !fDOT )
       {
          if ( victim->fighting == NULL )
          {
             if(is_affected(victim,skill_lookup("orb of surprise")) )
             {
                send_to_char("Someone tried to sneak up on you, ",victim);
                send_to_char("{GRUN!{x\n\r", victim );

                blow_orb(victim,skill_lookup("orb of surprise"));
                surprised = TRUE;
                stop_fighting(victim,FALSE);
             }
             else  if ( !fDOT )
             {
                if (victim->timer <= 4 || iOld)
                   set_fighting( victim, ch );
             }
             /* Rude awakening, knocked off whatever you were on */
             victim->on = NULL;
          }
          if (victim->timer <= 4 && !surprised )
             victim->position = POS_FIGHTING;
       }

       if ( ch->fighting != NULL
            && is_affected(ch,skill_lookup("orb of surprise")) )
           blow_orb(ch,skill_lookup("orb of surprise"));

  /*
   * More charm stuff.
   */
       if ( victim->master == ch )
          stop_follower( victim );
    } /* Victim is not Ch */

    /*
     * Traps?  No steenkin traps!
     * Earthbind DOES remain in effect, tho
     */
    if ( is_affected(victim,gsn_trap) )
       affect_strip( victim, gsn_trap );

    /*
     * Inviso attacks ... not.
     */
    if ( IS_AFFECTED(ch, AFF_INVISIBLE) )
    {
       affect_strip( ch, gsn_invis );
       affect_strip( ch, gsn_mass_invis );
       REMOVE_BIT( ch->affected_by, AFF_INVISIBLE );
       act( "$n fades into existence.", ch, NULL, NULL, TO_ROOM ,FALSE);
    }

    if ( IS_SET(ch->mhs, MHS_FADE ) )
    {
        REMOVE_BIT(ch->mhs,MHS_FADE);
        act("$n shimmers into existance.",ch,NULL,NULL,TO_ROOM ,FALSE);
    }
    /*
     * sneaking away ... not.
     */
    if ( IS_AFFECTED(ch, AFF_SNEAK) &&
        ( ch->class != class_lookup("thief")
         && ch->class != class_lookup("assassin")
         && ch->class != class_lookup("rogue")
        )
       )
    {
       affect_strip( ch, gsn_sneak  );
       REMOVE_BIT( ch->affected_by, AFF_SNEAK );
     act( "$n no longer moves silently about.", ch, NULL, NULL, TO_ROOM ,FALSE);
    }

    /*
     * No hiding.
     */
    REMOVE_BIT( ch->affected_by, AFF_HIDE );

    /*
     * Remove Shapemorph
     */
    if ( IS_SET(ch->mhs,MHS_SHAPEMORPHED))
    {
       REMOVE_BIT(ch->mhs,MHS_SHAPEMORPHED);
       send_to_char("You return to your regular appearance. Yikes!\n\r",ch);
     act( "$n returns to $s regular appearance.", ch, NULL, ch, TO_ROOM ,FALSE);
    }

    /* Reset the attack timer so they dont get disqualified */
    if(!IS_NPC(ch) && IS_SET(ch->mhs,MHS_GLADIATOR) && victim != ch )
       ch->pcdata->gladiator_attack_timer = 5;

    /*
     * Damage modifiers.
     */

//#ifdef COREYTEST

   if ( !IS_NPC(victim)
        && IS_SET(ch->act,PLR_VAMP)
        && IS_SET(victim->act,PLR_VAMP)
      )
      {
        self_undead = TRUE;
      }
   if ( !IS_NPC(victim)
       && IS_SET(ch->act,PLR_WERE)
       && IS_SET(victim->act,PLR_WERE)
      )
      {
        self_undead = TRUE;
      }
   if ( !IS_NPC(victim)
       && IS_SET(ch->act,PLR_MUMMY)
       && IS_SET(victim->act,PLR_MUMMY)
      )
      {
        self_undead = TRUE;
      }

   if ( HAS_KIT(ch,"vampyre hunter")
       && !IS_NPC(victim)
       && IS_SET(victim->act,PLR_VAMP)
       )
       {
        primary_undead = TRUE;
       }
    else if ( !IS_NPC(victim) && (IS_SET(victim->act, PLR_WERE) || IS_SET(victim->act, PLR_MUMMY)) )
       {
        secondary_undead = TRUE;
       }
   if ( HAS_KIT(ch,"lycanthrope hunter")
       && !IS_NPC(victim)
       && IS_SET(victim->act,PLR_WERE)
       )
        {
        primary_undead = TRUE;
       }
    else if ( !IS_NPC(victim) && (IS_SET(victim->act, PLR_VAMP) || IS_SET(victim->act, PLR_MUMMY)) )
       {
        secondary_undead = TRUE;
       }

   if ( HAS_KIT(ch,"archeologist")
       && !IS_NPC(victim)
       && IS_SET(victim->act,PLR_MUMMY)
       )
       {
        primary_undead = TRUE;
       }
    else if ( !IS_NPC(victim) && (IS_SET(victim->act, PLR_WERE) || IS_SET(victim->act, PLR_VAMP)) )
       {
        secondary_undead = TRUE;
       }

   if ( primary_undead == TRUE  && self_undead == FALSE)
        {
          dam += (( ( 125 + (ch->level/2) ) * base_dam / 100) - base_dam );
        }
/* Always happening now, unless they get the kit boost */
//   if (secondary_undead == TRUE && self_undead == FALSE)
        else if(!IS_NPC(victim))
        {
          dam += (( ( 75 + (ch->level/4) ) * base_dam / 100) - base_dam );
        }

//#endif
#ifdef OLDCODEDUDE
    if ( HAS_KIT(ch,"buffy") &&
        ( IS_SET(victim->act, PLR_VAMP) ||
        IS_SET(victim->act, PLR_WERE) ||
        IS_SET(victim->act, PLR_MUMMY)) &&
        !IS_NPC(victim)
       )
    {
//   sprintf(log_buf,"bef-buf:%s hit/dam:%d/%d dam: %d to %s",ch->name,ch->hitroll,ch->damroll,dam,victim->name);
//   log_string(log_buf);
        dam += (( ( 125 + (ch->level/2) ) * base_dam / 100) - base_dam );
//   sprintf(log_buf,"aft-buf:%s hit/dam:%d/%d dam: %d to %s",ch->name,ch->hitroll,ch->damroll,dam,victim->name);
//   log_string(log_buf);
    }
#endif


    if ( HAS_KIT(ch,"wyrmslayer") &&
        IS_SET(victim->form,FORM_DRAGON) )
        dam += (( ( 100 + (ch->level/2) ) * base_dam / 100) - base_dam );

#ifdef CODETEST
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
      {
        sprintf(log_buf,"Before remort vuln %d, ",dam);
        send_to_char(log_buf,ch);
      }
#endif

 if(!iOld && dam_type == DAM_VULN)
 {/* Material vuln checks */
   int dam_mod = 0;
   if (IS_SET(victim->act,PLR_MUMMY) || IS_SET(victim->act,PLR_VAMP) || IS_SET(victim->act,PLR_WERE))
   {/* Split these checks because remort vulns may be higher later */
     dam_mod = 1;
   }
   else if(victim->race == race_lookup("elf") || victim->race == race_lookup("half elf"))
   {
     dam_mod = 1;
   }
    switch(check_immune(victim,dam_type))
    {
  case(IS_IMMUNE):
      immune = TRUE;
      dam_mod = 0;
      dam = 0;
      break;
  case(IS_RESISTANT):
      if(dam_mod)
        dam_mod = 2;// Resistant + material vuln
      else
        dam_mod = 3;// Resistant
      break;
  case(IS_VULNERABLE):
      dam_mod = 4;// Vulnerable
      break;
    }
    switch(dam_mod)
    {/* No default, if it's not one of these it shouldn't change */
      case 1: dam += base_dam / 5; break;
      case 2: dam = (dam * 2) / 3 + base_dam / 5; break;
      case 3: dam -= dam / 3; break;
      case 4: dam += dam / 3; break;
    }
 }
 else
 {
    if (IS_SET(victim->act,PLR_MUMMY) && !iOld )
    {
        OBJ_DATA *obj;
          if (fSecondary)
             obj = get_eq_char( ch, WEAR_SECOND );
          else
             obj = get_eq_char( ch, WEAR_WIELD );
        if ( obj != NULL && is_name("fire",obj->material) )
           dam += ((base_dam)/5);
        else if (obj != NULL && !IS_WEAPON_STAT(obj,WEAPON_FLAMING))
           dam = (3*dam)/4;
    }

    /* Ok, here goes teh POSSE stuff - Boogums*/
    /* This code removed by NIGHTDAGGER

    if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_KILLER) )
    {
      dam += (base_dam * .14);
    }
    if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_THUG) )
    {
      dam += (base_dam * .08);
    }
    if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_RUFFIAN) )
    {
       dam += (base_dam * .05);
    }
    if ( ch->clan == clan_lookup("posse") && IS_SET(victim->act, PLR_THIEF) )
    {
      dam += (base_dam * .11);
    }
    End code nerf */

    if ( IS_SET(victim->act,PLR_VAMP) && !iOld )
    {
        OBJ_DATA *obj;
          if (fSecondary)
             obj = get_eq_char( ch, WEAR_SECOND );
          else
             obj = get_eq_char( ch, WEAR_WIELD );
        if ( obj != NULL && is_name("wood",obj->material) )
        dam += base_dam/5;
    }

    if ( IS_SET(victim->act,PLR_WERE) && !iOld )
    {
        OBJ_DATA *obj;
          if (fSecondary)
             obj = get_eq_char( ch, WEAR_SECOND );
          else
             obj = get_eq_char( ch, WEAR_WIELD );
        if ( obj != NULL && is_name("silver",obj->material) )
        dam += base_dam/5;
    }

#ifdef CODETEST
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
      {
        sprintf(log_buf,"after remort vuln %d\n\r",dam);
        send_to_char(log_buf,ch);
      }
#endif
    if ( victim->race == race_lookup("elf") || victim->race == race_lookup("half elf"))
    {
       OBJ_DATA *obj;
       if (fSecondary)
         obj = get_eq_char(ch, WEAR_SECOND);
       else
         obj = get_eq_char(ch, WEAR_WIELD);

       if (obj != NULL && ( is_name("iron", obj->material) || is_name("steel", obj->material)))
           dam += base_dam/5;
     }

    /* Smurfs resist two handed weapons*/
    if (victim->race == race_lookup("smurf"))
    {
       OBJ_DATA *obj;
       if (fSecondary)
         obj = get_eq_char(ch, WEAR_SECOND);
       else
         obj = get_eq_char(ch, WEAR_WIELD);

       if(obj != NULL && IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS))
          dam -= (dam / 3);
    }
  }// End gladiator blind else
    immune = FALSE;

/* placed before damage skins so attacks will interrupt it */
    if ( !IS_NPC(victim) )
    {
      if(victim->pcdata->wraith_timer > 0)
      {
        victim->pcdata->wraith_timer = 0;
        act("Your attempt to go to wraithform is interrupted.",victim,NULL,NULL,TO_CHAR,FALSE);
        act("$n failed to go to wraithform.",victim,NULL,NULL,TO_ROOM,FALSE);
      }
      if(victim->pcdata->pulse_timer > 0)
      {
        if(damage_pulse_command(victim))
        {
          act("You have been interrupted!",victim,NULL,NULL,TO_CHAR,FALSE);
          act("$n has been interrupted!",victim,NULL,NULL,TO_ROOM,FALSE);
          end_pulse_command(victim, FALSE, TRUE);/* Failure */
        }
      }
    }

        if(!IS_NPC(victim) && victim->pcdata->deity_trial_timer > 0 && victim->pcdata->deity_trial == 9 && IS_NPC(ch))
        {// None of the avoidance works against trial 9
                if((paf = affect_find(victim->affected,gsn_shield_of_faith) ) != NULL || (paf = affect_find(victim->affected,gsn_steel_skin) ) != NULL ||
                        (paf = affect_find(victim->affected,gsn_diamond_skin) ) != NULL || (paf = affect_find(victim->affected,gsn_adamantite_skin) ) != NULL)
                        act("$n strikes through your protective magics because of your trial!\n\r",ch,NULL,victim,TO_VICT,FALSE);
                if ( (paf = affect_find(victim->affected,gsn_acclimate) ) != NULL )
                {
                affect_remove(victim,paf,APPLY_BOTH);
                act("You are no longer attuned to your environment.",victim,NULL,NULL,TO_CHAR,FALSE);
                act("$n is no longer attuned to $s environment.",victim,NULL,NULL,TO_ROOM,FALSE);
                }
        }
        else
        {
    if ( (paf = affect_find(victim->affected,gsn_shield_of_faith) ) != NULL )
     {
        paf->duration--;
        dam = 0;
        act("Your shield of faith protects you!",victim,NULL,NULL,TO_CHAR,FALSE);
        act("$n's shield of faith protects $m!",victim,NULL,NULL,TO_ROOM,FALSE);
        if ( --paf->duration <  0 )
        {
           affect_remove(victim,paf,APPLY_BOTH);
        act("Your shield of faith is gone.",victim,NULL,NULL,TO_CHAR,FALSE);
        act("$n's shield of faith is gone.",victim,NULL,NULL,TO_ROOM,FALSE);
        }
        return TRUE;
     }

     if ( (paf = affect_find(victim->affected,gsn_steel_skin) ) != NULL )
     {
        paf->duration--;
         dam = 0;
         act("$n's attack deflects harmlessly off your steel skin.",ch,NULL,victim,TO_VICT,FALSE);
         act("Your attack deflects harmlessly off $N's steel skin.",ch,NULL,victim,TO_CHAR,FALSE);
         act("$n's attack deflects harmlessly off $N's steel skin.",ch,NULL,victim,TO_NOTVICT,FALSE);
         if ( --paf->duration <  0 )
         {
              affect_remove(victim,paf,APPLY_BOTH);
                act("Your skin becomes flesh again.",victim,NULL,NULL,TO_CHAR,FALSE);
                act("$n's skin becomes flesh again.",victim,NULL,NULL,TO_ROOM,FALSE);
        }
        return TRUE;
    }

    if ( (paf = affect_find(victim->affected,gsn_diamond_skin) ) != NULL )
    {
        paf->duration--;
        dam = 0;
        act("$n's attack deflects harmlessly off your diamond skin.",ch,NULL,victim,TO_VICT,FALSE);
        act("Your attack deflects harmlessly off $N's diamond skin.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n's attack deflects harmlessly off $N's diamond skin.",ch,NULL,victim,TO_NOTVICT,FALSE);
        if ( --paf->duration <  0 )
        {
            affect_remove(victim,paf,APPLY_BOTH);
            act("Your skin becomes flesh again.",victim,NULL,NULL,TO_CHAR,FALSE);
            act("$n's skin becomes flesh again.",victim,NULL,NULL,TO_ROOM,FALSE);
        }
        return TRUE;
    }

    if ( (paf = affect_find(victim->affected,gsn_adamantite_skin) ) != NULL )
    {
        paf->duration--;
        dam = 0;
        act("$n's attack deflects harmlessly off your adamantite skin.",ch,NULL,victim,TO_VICT,FALSE);
        act("Your attack deflects harmlessly off $N's adamantite skin.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n's attack deflects harmlessly off $N's adamantite skin.",ch,NULL,victim,TO_NOTVICT,FALSE);
        if ( --paf->duration <  0 )
        {
            affect_remove(victim,paf,APPLY_BOTH);
            act("Your skin becomes flesh again.",victim,NULL,NULL,TO_CHAR,FALSE);
            act("$n's skin becomes flesh again.",victim,NULL,NULL,TO_ROOM,FALSE);
        }
        return TRUE;
    }


    if ( (paf = affect_find(victim->affected,gsn_acclimate) ) != NULL )
    {
        affect_remove(victim,paf,APPLY_BOTH);
        act("You are no longer attuned to your environment.",victim,NULL,NULL,TO_CHAR,FALSE);
        act("$n is no longer attuned to $s environment.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    /*
     * Check for parry, and dodge.
     */
    if ( dt >= TYPE_HIT && ch != victim)
    {
        if ( check_parry( ch, victim,fSecondary ) )
        {
           /* Gladiator Spectator Channel */
           if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
           {
              sprintf(cdbuf,"%s parries an attack from %s.",victim->name,ch->name);
              gladiator_talk(cdbuf);
           }
           return FALSE;
        }
        if ( check_mistform(ch,victim))
        {
           /* Gladiator Spectator Channel */
           if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
           {
              sprintf(cdbuf,"What the hell was that?! %s's weapon just went right through %s.",ch->name,victim->name);
              gladiator_talk(cdbuf);
           }
           return FALSE;
        }
        if ( check_dodge( ch, victim,fSecondary ) )
        {
           /* Gladiator Spectator Channel */
           if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
           {
              sprintf(cdbuf,"%s dodges an attack from %s.",victim->name,ch->name);
              gladiator_talk(cdbuf);
           }
           return FALSE;
        }
        if ( check_kailindo( ch, victim ) )
        {
          if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
          {
            sprintf(cdbuf,
                "%s evades an attack from %s and quickly strikes back.",
                victim->name,ch->name);
            gladiator_talk(cdbuf);
          }
          return FALSE;
        }
        if ( check_shield_block(ch,victim,fSecondary))
        {
           /* Gladiator Spectator Channel */
           if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
           {
              sprintf(cdbuf,"%s blocks an attack from %s with a shield.",victim->name,ch->name);
              gladiator_talk(cdbuf);
           }
           return FALSE;
        }
        if ( check_scales(ch,victim,fSecondary))
        {
           /* Gladiator Spectator Channel */
           if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 10)
           {
              sprintf(cdbuf,"%s wasn't strong enough to get through %s's scales.",ch->name,victim->name);
              gladiator_talk(cdbuf);
           }
            return FALSE;
        }
        if ( check_nether( ch, victim, fSecondary) )
                   return FALSE;
      if(is_shaded(victim) && !IS_SET(ch->affected_by_ext, AFF_EXT_SHADED))
      {
        if(number_percent() < (is_affected(ch,gsn_vision) ? 10 : 50))
        {// 50% miss chance unless they have a vision potion on, then just 10%
          {
            if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
              act( "$n's attack misses you, striking only your shadow.",  ch, NULL, victim, TO_VICT    ,FALSE);
            if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
              act( "You swing but only strike empty shadow.", ch, NULL, victim, TO_CHAR    ,FALSE);
            tail_chain();
            return FALSE;
          }
        }
      }
    }
        }// End trial 9 - no dodging

#ifdef CODETEST
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
      {
        sprintf(log_buf,"Before race vuln %d, ",dam);
        send_to_char(log_buf,ch);
      }
#endif

    if ( dam > 1 && !IS_NPC(victim)
         &&   victim->pcdata->condition[COND_DRUNK]  > 10 )
       dam = 9 * dam / 10;

    if ( dam > 1 && IS_AFFECTED(victim, AFF_SANCTUARY) )
       dam /= 2;

    if ( dam > 1 && (( (IS_AFFECTED(victim, AFF_PROTECT_EVIL) && IS_EVIL(ch))
    ||         (IS_AFFECTED(victim, AFF_PROTECT_GOOD) && IS_GOOD(ch))
    ||  (is_affected(victim,gsn_protect_neutral) && IS_NEUTRAL(ch)) ) || (dam_type == DAM_VULN && (IS_AFFECTED(victim, AFF_PROTECT_EVIL) || IS_AFFECTED(victim, AFF_PROTECT_GOOD) || is_affected(victim, gsn_protect_neutral)))))// Vuln is resisted if they have anything on
       dam -= dam / 4;

  if(!IS_SET(victim->mhs, MHS_GLADIATOR) || gladiator_info.exper == FALSE)
  {
    switch(check_immune(victim,dam_type))
    {
  case(IS_IMMUNE):
      immune = TRUE;
      dam = 0;
      break;
  case(IS_RESISTANT):
      dam -= (dam / 3);
      break;
  case(IS_VULNERABLE):
      dam += dam/3;
      break;
    }
  }

#ifdef CODETEST
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
      {
        sprintf(log_buf,"after race vuln %d\n\r",dam);
        send_to_char(log_buf,ch);
      }
#endif

    /* Wound transfer spell */
    if ( is_affected(victim,gsn_wound_transfer)
       && ch != victim)
    {
       int count = 1;

       for ( gch = victim->in_room->people ;
             gch != NULL ;
             gch = gch_next )
       {
           gch_next = gch->next_in_room;

           if ( gch == ch || gch == victim )
                continue;

           if ( is_same_group(gch,victim) )
              ++count;
       }

       dam /= UMIN(count,iOld?3:4);

       for ( gch = victim->in_room->people ;
             gch != NULL ;
             gch = gch_next )
       {
           gch_next = gch->next_in_room;

           if ( gch == ch || gch== victim )
               continue;

           if ( is_same_group(gch,victim) )
           {
             /* Extra check to avoid infinate recursion */
              if ( is_affected(gch,gsn_wound_transfer) )
              {
                 bug("Wound transfer: gch is_affected()",0);
                 continue;
              }
              else
                 damage(ch,gch,dam,dt,dam_type,TRUE,iOld);
           }
       }
    } /* wound transfer spell */

    /* Can't do negative damage */
    dam = UMAX(0,dam);

        if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && IS_NPC(victim))
        {
                if(ch->pcdata->deity_trial == 6)
                        dam /= 2; // Trial 6: Greatly reduced damage and casting level
                else if(ch->pcdata->deity_trial == 4)
                        dam = dam * 2 / 3;// Trial 4: Your enemies resist your blows
        }
        else if(!IS_NPC(victim) && victim->pcdata->deity_trial_timer > 0 && IS_NPC(ch))
                dam = dam * 133 / 100; // Trial 7: You are vulnerable to your enemy's blows

    if (show)
    {
      if(iOld)
        dam_message( ch, victim, dam, dt, immune );
      else
        dam_message_new( ch, victim, dam, dt, immune );
    }

    if (dam == 0)
       return FALSE;

    /* Handle damage shields */
    if ( is_affected(victim,gsn_shield_of_thorns) && dt >= TYPE_HIT )
    {
        extra = ch->hit;
        ch->hit = UMAX(1,ch->hit-4);
  if(!IS_NPC(ch) && !IS_NPC(victim) && (extra = extra - ch->hit) > 0)
    damage_add(victim, ch, extra, 5);// Reversed, the victim is hurting the attacker
        act("$n is injured by a shield of thorns protecting $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
        act("You are injured by a shield of thorns protecting $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n is injured by a shield of thorns protecting you.",ch,NULL,victim,TO_VICT,FALSE);
    }

    if ( is_affected(victim,gsn_shield_of_brambles) && dt >= TYPE_HIT )
    {
        extra = ch->hit;
        ch->hit = UMAX(1,ch->hit-8);
  if(!IS_NPC(ch) && !IS_NPC(victim) && (extra = extra - ch->hit) > 0)
    damage_add(victim, ch, extra, 5);// Reversed, the victim is hurting the attacker
        act("$n is injured by a shield of brambles protecting $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
        act("You are injured by a shield of brambles protecting $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n is injured by a shield of brambles protecting you.",ch,NULL,victim,TO_VICT,FALSE);
    }

    if ( is_affected(victim,gsn_shield_of_spikes) && dt >= TYPE_HIT )
    {
        extra = ch->hit;
        ch->hit = UMAX(1,ch->hit-12);
  if(!IS_NPC(ch) && !IS_NPC(victim) && (extra = extra - ch->hit) > 0)
    damage_add(victim, ch, extra, 5);// Reversed, the victim is hurting the attacker
        act("$n is injured by a shield of spikes protecting $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
        act("You are injured by a shield of spikes protecting $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n is injured by a shield of spikes protecting you.",ch,NULL,victim,TO_VICT,FALSE);
    }

    if ( is_affected(victim,gsn_shield_of_blades) && dt >= TYPE_HIT )
    {
        extra = ch->hit;
        ch->hit = UMAX(1,ch->hit-16);
  if(!IS_NPC(ch) && !IS_NPC(victim) && (extra = extra - ch->hit) > 0)
    damage_add(victim, ch, extra, 5);// Reversed, the victim is hurting the attacker
        act("$n is injured by a shield of blades protecting $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
        act("You are injured by a shield of blades protecting $N.",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n is injured by a shield of blades protecting you.",ch,NULL,victim,TO_VICT,FALSE);
    }

    /*
     * Hurt the victim.
     * Inform the victim of his new state.
     */

    if ( victim->hit - dam < 1 &&
          HAS_KIT(victim,"nethermancer") &&
         number_percent() <= get_skill(victim,gsn_nethermancy) )
    {
       if( victim->mana < ( dam * 2 ) )
       {
        check_improve(victim,gsn_nethermancy,FALSE,10);
        send_to_char("Your nethermancy has failed you.\n\r",victim);
        victim->hit -= dam;
       }
       else
       {
        check_improve(victim,gsn_nethermancy,TRUE,10);
        victim->hit = 1;
        victim->mana -= ( dam * 2 );
        }
    }
    else
     /* Communion */
    if ( victim->hit - dam < 1 &&
         !IS_NPC(victim) &&
         number_percent() <= get_skill(victim,gsn_communion))
    {
       if( victim->pcdata->sac < dam*2 )
       {
           check_improve(victim,gsn_communion,FALSE,6);
           send_to_char("Your communion has failed.\n\r",victim);
           victim->hit -= dam;
       }
       else
       {
       check_improve(victim,gsn_communion,TRUE,3);
       victim->hit = 1; /* ALWAYS set to 1 if communion works */
       victim->pcdata->sac -= dam*2;
       }
    }
    else
       victim->hit -= dam;

    if(IS_SET(victim->affected_by_ext, AFF_EXT_SHADED) && dam_type == DAM_LIGHT)
    {// Light damage
      AFFECT_DATA *light = affect_find(victim->affected,gsn_light_blast);
      if(light)
      {
        if(light->duration < 1)
        {
          act("$n looks a little more solid.", victim, NULL, NULL, TO_ROOM, FALSE);
          send_to_char("The light re-solidifies your shadow.\n\r", victim);
          light->duration = UMAX(1, light->duration);
        }
      }
      else
      {
        AFFECT_DATA af;
        af.where  = TO_AFFECTS;
        af.type   = gsn_light_blast;
        af.level  = ch->level;
        af.duration = 1;
        af.location = APPLY_NONE;
        af.modifier = 0;
        af.bitvector  = 0;
        af.caster_id = ch->id;
        affect_to_char(victim, &af);
        act("$n looks much more solid.", victim, NULL, NULL, TO_ROOM, FALSE);
        send_to_char("The light solidifies your shadow.\n\r", victim);
      }
    }

/*
    check_improve(victim,gsn_communion,FALSE,6);
 */

    if ( !IS_NPC(victim)
       && (victim->level >= LEVEL_IMMORTAL )
       &&   victim->hit < 1 )
       victim->hit = 1;

  if(!IS_NPC(victim) && !IS_NPC(ch) && dam > 0)
    damage_add(ch, victim, dam, 5);

    update_pos( victim );

    switch( victim->position )
    {
    case POS_MORTAL:
  act( "$n is mortally wounded, and will die soon, if not aided.",
      victim, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char(
      "You are mortally wounded, and will die soon, if not aided.\n\r",
      victim );
  send_to_char("You may type 'die' to end it if you don't think you will be saved.\n\r", victim);
  if (!is_clan(victim))
  {
     sprintf(cdbuf,
        "%s has been mortally wounded by %s at %s.\n\r",
        victim->name,ch->short_descr,victim->in_room->name);
           pnet(cdbuf,NULL,NULL,PNET_MATOOK,0,0);

     stop_fighting(victim,TRUE);

     for ( d = descriptor_list; d != NULL; d = d->next )
     {
        CHAR_DATA *victm;

        victm = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
             d->character != victim &&
             !IS_SET(victm->comm,COMM_QUIET) && !IS_NPC(victm) &&
             !str_cmp(deity_table[victm->pcdata->deity].pname,"matook"))
           {
           send_to_char(cdbuf, victm);
           if(!IS_IMMORTAL(victm) && victm->pcdata->sac >= 300 && !is_affected(ch,skill_lookup("holy silence")))
               send_to_char("Matook is willing to {Wintervene{x at your request.\n\r",victm);
           }
     }
  }
  break;

    case POS_INCAP:
  act( "$n is incapacitated and will slowly die, if not aided.",
      victim, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char(
      "You are incapacitated and will slowly die, if not aided.\n\r",
      victim );
  send_to_char("You may type 'die' to end it if you don't think you will be saved.\n\r", victim);
  if (!is_clan(victim))
  {
     sprintf(cdbuf,
        "%s has been incapacitated by %s at %s.\n\r",
        victim->name,ch->short_descr,victim->in_room->name);
           pnet(cdbuf,NULL,NULL,PNET_MATOOK,0,0);

     stop_fighting(victim,TRUE);

     for ( d = descriptor_list; d != NULL; d = d->next )
     {
        CHAR_DATA *victm;

        victm = d->original ? d->original : d->character;

        if ( d->connected == CON_PLAYING &&
             d->character != victim &&
             !IS_SET(victm->comm,COMM_QUIET) && !IS_NPC(victm) &&
             !str_cmp(deity_table[victm->pcdata->deity].pname,"matook"))
           {
           send_to_char(cdbuf, victm);
           if(!IS_IMMORTAL(victm) && victm->pcdata->sac >= 300 && !is_affected(ch,skill_lookup("holy silence")))
               send_to_char("Matook is willing to {Wintervene{x at your request.\n\r",victm);
           }
     }
  }
  break;

    case POS_STUNNED:
  act( "$n is stunned, but will probably recover.",
      victim, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char("You are stunned, but will probably recover.\n\r",
      victim );
  break;
    case POS_DEAD:
  act( "$n is {RDEAD{x!!", victim, 0, 0, TO_ROOM ,FALSE);
  send_to_char( "You have been {RKILLED{x!!\n\r\n\r", victim );
  break;

    default:
  if ( dam > victim->max_hit / 4 )
       send_to_char( "That really did {YHURT{x!\n\r", victim );
  if ( victim->hit < victim->max_hit / 4 &&
        !IS_SET(victim->comm,COMM_SILENCE) )
     {
       send_to_char( "You sure are {RBLEEDING{x!\n\r", victim );
     }
  break;
    }

    /*
     * Sleep spells and extremely wounded folks.
     */
    if ( !IS_AWAKE(victim) )
       stop_fighting( victim, FALSE );

    /*
     * Payoff for killing things.
     */
    if ( victim->position == POS_DEAD )
    {
       is_dead = TRUE;
       /* Poquah's new death code DEATH POQUAH */
       /* Ch and Victim are both gladiators in Gladiator Combat */
       if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
           IS_SET(victim->mhs,MHS_GLADIATOR) &&
           !IS_NPC(ch) &&
           !IS_NPC(victim) &&
           !IS_SET(victim->affected_by,AFF_WITHSTAND_DEATH) &&
           gladiator_info.started == TRUE)
       {
          gladiator_kill(victim,ch);
          return TRUE;
       }

       /* No Die Room */
       if (IS_SET (ch->in_room->room_flags,ROOM_NODIE))
       {
          if ( !is_affected(victim,skill_lookup("withstand death")) )
          {
             died_in_room = ch->in_room;
             sprintf( log_buf, "%s got toasted by %s at %s [room %d]",
               (IS_NPC(victim) ? victim->short_descr : victim->name),
               (IS_NPC(ch) ? ch->short_descr : ch->name),
               died_in_room->name, died_in_room->vnum);
             log_string (log_buf);
             if (IS_NPC(victim))
               wiznet(log_buf, NULL, NULL, WIZ_MOBDEATHS, 0, 0);
             else
             {
               wiznet(log_buf, NULL, NULL, WIZ_DEATHS, 0, 0);
               if (!is_clan(victim) )
               {
                 pnet("$N died.", victim, NULL, PNET_DEATHS, 0, 0);
               }
             }
             raw_kill(victim,ch);
          }
          else
             raw_kill(victim,ch);

          return TRUE;
       }

       if ( !IS_NPC(victim) )
       {
          if (IS_NPC(ch) && ch->master != NULL)
          {
             kch = ch->master;
             kill_by_plain_mob = FALSE;
          }
          else
          {
             if (!IS_NPC(ch))
             {
                kill_by_plain_mob = FALSE;
                kch = ch;
             }
             else
             {
                kill_by_plain_mob = TRUE;
                kch = ch;
             }
          }
          if (!IS_SET(victim->affected_by,AFF_WITHSTAND_DEATH))
          {
            /* if (kch->clan == clan_lookup("smurf")
                || victim->clan == clan_lookup("smurf"))
             {
                raw_kill(victim,kch);
                return TRUE;
             }*/

             group_gain( ch, victim );
             /*
              * Dying penalty: No Withstand
              * 2/3 way back to previous level.
              */

                 if(victim->pcdata->last_death_timer == 0)
                 {
                     if ( victim->exp > exp_per_level(victim,victim->pcdata->points)
                 * victim->level )
                    {
                         if(victim->pcdata->quit_time == 0)// Full xp loss, not a pk
                               gain_exp(victim,(exp_per_level(victim,victim->pcdata->points)*victim->level - victim->exp)*2/3);
                         else// 10% instead of 66% if it's pk
                               gain_exp(victim,(exp_per_level(victim,victim->pcdata->points)*victim->level - victim->exp) / 10);
                     }
                 }

             /* Is_clan will eliminate nonclanners giving out toast messages*/
             if (!kill_by_plain_mob && is_clan(victim))
             { /* Killed by char or Charmie */
                if(victim != kch)
                { /* victim is not char */

                   if(victim->pcdata->last_death_timer == 0)
                   {
/* Victim has not been killed recently */
                      if ( HAS_KIT(kch,"nethermancer") &&
                        HAS_KIT(victim,"nethermancer") &&
                        number_percent() < get_curr_stat(kch,STAT_INT) &&
                        number_percent() < kch->hit / 80 )
                      { /* give nether flag to weapon, if they have one */
                         OBJ_DATA *weapon;

                         if ( ( weapon = get_eq_char( kch, WEAR_WIELD ) ) != NULL )
                         {
                            SET_BIT(weapon->value[4],WEAPON_NETHER);
          act("$p {Yflashes{x with a {Dblack{x aura.",kch,weapon,NULL,TO_CHAR,FALSE);
          act("$p {Yflashes{x with a {Dblack{x aura.",kch,weapon,NULL,TO_ROOM,FALSE);
                         }
                      }

                      if (str_cmp(kch->pcdata->last_kill,victim->name) &&
                         str_cmp(victim->pcdata->last_killed_by,kch->name))
                      {

                         if(kch->level + 4 < victim->level)
                            kch->pcdata->killer_data[PC_GREATER_KILLS]++;
                         else if(kch->level > victim->level +4)
                            kch->pcdata->killer_data[PC_LOWER_KILLS]++;
                         else
                            kch->pcdata->killer_data[PC_EQUAL_KILLS]++;
                         if(kch->pcdata->clan_info && !kch->pcdata->clan_info->clan->default_clan)
                         {
                            kch->pcdata->clan_info->clan->kills++;
                            do_save_clan(kch->pcdata->clan_info->clan);
                         }


                     for ( gch = char_list ; gch != NULL; gch = gch->next )
                         {
                            if(!IS_NPC(gch) && kch != gch && is_same_group(kch,gch))
                            {
                         if(gch->level + 4 < victim->level)
                            gch->pcdata->killer_data[PC_GREATER_KILLS]++;
                         else if(gch->level > victim->level +4)
                            gch->pcdata->killer_data[PC_LOWER_KILLS]++;
                         else
                            gch->pcdata->killer_data[PC_EQUAL_KILLS]++;
                         if(gch->pcdata->clan_info && !gch->pcdata->clan_info->clan->default_clan &&
                           gch->pcdata->clan_info->clan != kch->pcdata->clan_info->clan)
                         {
                           gch->pcdata->clan_info->clan->assists++;
                           do_save_clan(gch->pcdata->clan_info->clan);
                         }

                            }
                         }

                         victim->pcdata->killer_data[PC_DEATHS] += 1 ;
                         /* update clan statistics */
                         /*if(kch->clan == clan_lookup("honor"))
                         {
                            honor_kills += 1;
                            if (victim->clan == clan_lookup("demise"))
                               honor_demise_kills += 1;
                         }
                         if(kch->clan == clan_lookup("posse"))
                            posse_kills += 1;
                         if(kch->clan == clan_lookup("warlock"))
                            warlock_kills += 1;
                         if(kch->clan == clan_lookup("demise"))
                            demise_kills += 1;
                         if(kch->clan == clan_lookup("zealot"))
                            zealot_kills += 1;
                         if(kch->clan == clan_lookup("avarice"))
                            avarice_kills += 1;*/

                         kch->pcdata->last_kill_date = current_time;
                         kch->pcdata->logins_without_kill = 0;
                         victim->pcdata->last_death_date = current_time;
                         victim->pcdata->logins_without_death = 0;
                      }

                      if(victim->trumps == 0 && kch->level - victim->level <= 8
                         && !IS_SET(victim->wiznet,PLR_RUFFIAN)
                         && !IS_SET(victim->act,PLR_DWEEB)
                         && !IS_SET(victim->act,PLR_THIEF)
                         && str_cmp(kch->pcdata->last_kill,victim->name) )
                      {
                         /*if (kch->clan == clan_lookup("Posse") &&
                            IS_SET(victim->mhs,MHS_POSSE_ENEMY))
                         {
                            sprintf( cdbuf, "%s NO TRUMP POSSE ENEMY killing %s at %d",kch->name, victim->name,kch->in_room->vnum );
                         }
                         else*/
                         {
                            kch->trumps += 1 ;
                            sprintf( cdbuf, "%s trump++ killing %s at %d",kch->name,
                            victim->name,kch->in_room->vnum );
                         }
                      }
                      log_string( cdbuf );

                      kch->pcdata->last_kill = str_dup( victim->name );
                      victim->pcdata->last_killed_by = str_dup( kch->name );
                      victim->trumps = UMAX(0,victim->trumps -1);
                      if(IS_SET(victim->wiznet,PLR_RUFFIAN))
                      {
                         REMOVE_BIT(victim->wiznet,PLR_RUFFIAN);
                         //if(kch->clan == clan_lookup("posse"))
                         //   posse_ruffian_kills += 1;
                      }
                      if(kch->trumps >= 3 && !IS_SET(kch->act,PLR_KILLER))
                      {
                         SET_BIT(kch->act,PLR_KILLER);
                         send_to_char("*** You are now a KILLER! ***\n\r",kch);
                         sprintf(wdbuf,"$N got a (KILLER) by murdering %s",victim->name);
                         wiznet(wdbuf,kch,NULL,WIZ_FLAGS,0,0);
                      }

                      if (IS_SET(victim->act,PLR_THIEF))
                      {
                         REMOVE_BIT(victim->act,PLR_THIEF);
                         //if(kch->clan == clan_lookup("posse"))
                         //   posse_thief_kills += 1;
                      }
                      else
                      {
                         if(victim->trumps >=3 && IS_SET(victim->act,PLR_KILLER))
                         {
                            victim->trumps = 2;
                            REMOVE_BIT(victim->act,PLR_KILLER);
                            //if(kch->clan == clan_lookup("posse"))
                            //   posse_killer_kills += 1;
                         }
                         else
                         {
                            REMOVE_BIT(victim->act,PLR_KILLER);
                            //if(kch->clan == clan_lookup("posse"))
                            //   posse_thug_kills += 1;
                         }
                      }

                      if (!str_cmp(victim->name,kch->pcdata->last_attacked_by))
                         kch->pcdata->last_attacked_by_timer = 0;

                     /* if (kch->clan == clan_lookup("warlock") &&
                         IS_SET(victim->mhs,MHS_WARLOCK_ENEMY))
                         REMOVE_BIT(victim->mhs,MHS_WARLOCK_ENEMY);
                      if (kch->clan == clan_lookup("zealot") &&
                          IS_SET(victim->mhs,MHS_ZEALOT_ENEMY))
                         REMOVE_BIT(victim->mhs,MHS_ZEALOT_ENEMY);
                      if (kch->clan == clan_lookup("posse") &&
                          IS_SET(victim->mhs,MHS_POSSE_ENEMY))
                         REMOVE_BIT(victim->mhs,MHS_POSSE_ENEMY);
                      if (kch->clan == clan_lookup("honor") &&
                          IS_SET(victim->mhs,MHS_HONOR_ENEMY))
                         REMOVE_BIT(victim->mhs,MHS_HONOR_ENEMY);*/

                      if (victim->pcdata->bounty > 0 )
                      {
                         kch->gold += victim->pcdata->bounty ;
                         act("You collect the bounty on $N.",kch,NULL,victim,TO_CHAR,FALSE);
                         sprintf(log_buf,"%s collects a %ld gold bounty on %s.",
                            kch->name, victim->pcdata->bounty, victim->name );
                         wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
                         pnet(log_buf,NULL,NULL,PNET_BOUNTY,0,0);
                         log_string(log_buf);
                         victim->pcdata->bounty = 0 ;
                      }
                   }
/* Victim has not died recently */
                } /* Victim not Ch */

                /* Set victim's death_timer so spam kills dont affect above */
                victim->pcdata->last_death_timer = 5;
                if(!kch->max_hit)
                   strcpy(toast, "edged out");
                else
                {
                if (((kch->hit*100)/kch->max_hit) < 10)
                   strcpy(toast,"edged out");
                if (((kch->hit*100)/kch->max_hit) >= 10)
                   strcpy(toast,"toasted");
                if (((kch->hit*100)/kch->max_hit) >= 25)
                   strcpy(toast,"trashed");
                if (((kch->hit*100)/kch->max_hit) >= 50)
                   strcpy(toast,"flattened");
                if (((kch->hit*100)/kch->max_hit) >= 75)
                   strcpy(toast,"crushed");
                if (((kch->hit*100)/kch->max_hit) >= 100)
                   strcpy(toast,"slaughtered");
                }

                /* Charmed mob did killing */
                if (kch != ch)
                   sprintf( cdbuf, "%s{W%s{x got %s by {W%s{x controlled by {W%s{x\n\r",
                  victim->desc == NULL ? "({YLinkdead{x) " : "",
                  victim->name,toast,ch->name,kch->name);
                else
                   sprintf( cdbuf, "%s{W%s{x got %s by {W%s{x\n\r",
                  victim->desc == NULL ? "({YLinkdead{x) " : "",
                  victim->name,toast,kch->name);

                for ( d = descriptor_list; d != NULL; d = d->next )
                {
                   CHAR_DATA *victm;

                   victm = d->original ? d->original : d->character;
                   if ( d->connected == CON_PLAYING && is_clan(victm) )
                      send_to_char(cdbuf, victm);
                }
             } /* Killed by Charmed mob or player */

             raw_kill(victim,ch);

             /* Save the victims pfile to avoid people crashing us
                to get back their EQ or duplicate items
*/
             //save_char_obj(victim);
          } /* No withstand */
          else
          { /*Had withstand */
                if(victim->pcdata->quit_time == 0)//Not a pk kill - no xp loss on withstand in pk
                {
                     if ( victim->exp > exp_per_level(victim,victim->pcdata->points)
                 * victim->level )
                      gain_exp(victim,(exp_per_level(victim,victim->pcdata->points)*victim->level - victim->exp)/4);
                }
                raw_kill( victim, ch );
                return TRUE;
          } /* Had withstand */
       } /* Victim was not a NPC */
       else
       { /* Victim was a NPC */
          if(is_bounty_target(victim, TRUE))
          {
            CHAR_DATA *killer = IS_NPC(ch) ? ch->master : ch;
            if(killer && !IS_NPC(killer))
            {
              char buf[255];
              OBJ_DATA *ear;
              int tribute = 0, complete_level = bounty_complete;
              act("$n claims an ear from $N.", killer, NULL, victim, TO_ROOM, FALSE);
              send_to_char("A voice {gwhispers{x to you, 'Excellent work, bring this to me to be {Yrewarded{x.'\n\r", killer);
              act("You get an ear from $N.", killer, NULL, victim, TO_CHAR, FALSE);
              killer->pcdata->quest_wins[QUEST_BOUNTY_KILL]++;
            	ear = create_object(get_obj_index(OBJ_VNUM_BOUNTY_EAR),0,FALSE);
            	sprintf(buf, ear->name, victim->short_descr);
            	clear_string(&ear->name, buf);
            	sprintf(buf, ear->short_descr, victim->short_descr);
            	clear_string(&ear->short_descr, buf);
            	sprintf(buf, ear->description, victim->short_descr);
            	clear_string(&ear->description, buf);
            	obj_to_char(ear,killer);
              ear->value[3] = 0;
            	if(bounty_downgrade)
            	{
                bounty_complete++;
                if(victim->pIndexData->bounty_level == MAX_BOUNTY_LEVEL)
                {
                  ear->value[3] |= BOUNTY_AWARD_STONE;// Give a shade stone for this one
                  bounty_complete = 0;// Restart after assigning a stone
                  ear->value[3] |= BOUNTY_AWARD_HARD;
                }
                if(bounty_type > BOUNTY_ITEM_NAME)
                  ear->value[0] = bounty_type - BOUNTY_ITEM_NAME;
                else
                  ear->value[0] = UMAX(1, bounty_type - 1);
                ear->value[3] |= BOUNTY_AWARD_DOWNGRADED;
              }
              else
              {
                bounty_complete += number_range(1, 2);
                ear->value[0] = bounty_type;
                if(victim->pIndexData->bounty_level == MAX_BOUNTY_LEVEL
                || ((victim->pIndexData->bounty_level + bounty_type) >=
                   (MAX_BOUNTY_LEVEL + BOUNTY_ITEM_NAME)))
                {
                  if(bounty_complete > 30 ||
                    victim->pIndexData->bounty_level == MAX_BOUNTY_LEVEL)
                  {
                    ear->value[3] |= BOUNTY_AWARD_STONE;
                    bounty_complete = 0;
                  }
                  ear->value[3] |= BOUNTY_AWARD_HARD;
                }
              }
              ear->value[1] = victim->pIndexData->vnum;
              ear->value[2] = victim->pIndexData->bounty_level;
              if(!bounty_downgrade && bounty_timer >= 15)
                ear->value[3] |= BOUNTY_AWARD_UPGRADED;
              if(killer->pcdata->clan_info)
              {/* Clan tribute is handled elsewhere, as it is not tied to xp earned */
                int target_level = UMAX(killer->level, victim->level);
                int bonus = 200;// Base is double, increases with difficulty
                tribute = (target_level * target_level) / 20;
                bonus += ear->value[2] * 10;// Another 50% for a rank 5
                if(IS_SET(ear->value[3], BOUNTY_AWARD_HARD))
                  bonus += 40;
                if(IS_SET(ear->value[3], BOUNTY_AWARD_UPGRADED))
                  bonus += 40;
                // Up to 72% more for a level 6
                bonus += ear->value[0] * ear->value[0] * 2;
                //Total max: 200 + 50 + 40 + 40 + 72 = 402
                tribute = UMAX(1, tribute);
                tribute = tribute * bonus / 100;
                tribute = (tribute * number_range(95, 105)) / 100; /* A little variance */
                sprintf(buf, "'We tricked the gods for you, you also have earned {W%d{x merit.'\n\r", tribute);
                send_to_char(buf, killer);
                if(killer->pcdata->clan_info->clan->initiation > 0)
                {
                  killer->pcdata->clan_info->clan->initiation -= tribute;
                  if(killer->pcdata->clan_info->clan->initiation <= 0)
                  {
                    killer->pcdata->clan_info->clan->initiation = 0;
                    /* Done initiation, announce it to everyone on from the clan */
                    notify_clan("Your clan has completed its initiation successfully!\n\r", killer->pcdata->clan_info->clan);
                    save_clan(killer, TRUE, FALSE, TRUE);
                  }
                }
                else
                {
                  killer->pcdata->clan_info->merit += tribute;
                }
              }
            	sprintf(buf, "%s killed %s(%d) for a bounty (%d merit) (%d %d %d%s)", killer->name,
                victim->short_descr, victim->pIndexData->vnum, tribute,
                bounty_type, bounty_timer, complete_level,
                bounty_downgrade ? " down" :
                IS_SET(ear->value[3], BOUNTY_AWARD_UPGRADED) ? " up" : "");
            	log_quest_detail(buf, QUEST_BOUNTY_KILL);
              // bounty_type and mob->bounty_level are the two key ones
              select_bounty(bounty_complete);
              bounty_timer = -2;
              if(number_percent() < bounty_complete * 3)
                bounty_timer--;
              if(number_percent() < bounty_complete * 2)
                bounty_timer--;
              if(number_percent() < bounty_complete)
                bounty_timer--;
              pnet("A shadow bounty has been claimed.",ch,NULL,PNET_SHADEBOUNTY,0,0);
            }
          }
          group_gain( ch, victim );

          died_in_room = ch->in_room;

          raw_kill( victim,ch );

          if ( !IS_NPC(ch) )
          { /* Ch was not a NPC */
             OBJ_DATA *coins;

             corpse = get_obj_list( ch, "corpse", ch->in_room->contents );

             if ( IS_SET(ch->act, PLR_AUTOLOOT) &&
                corpse && corpse->contains) /* exists and not empty */
                do_get( ch, "all corpse" );

             if (IS_SET(ch->act,PLR_AUTOGOLD) &&
                  corpse && corpse->contains  && /* exists and not empty */
                  !IS_SET(ch->act,PLR_AUTOLOOT))
                if ((coins = get_obj_list(ch,"gcash",corpse->contains))
                  != NULL)
                   do_get(ch, "all.gcash corpse");

             if ( IS_SET(ch->act, PLR_AUTOSAC) )
             {
                if ( IS_SET(ch->act,PLR_AUTOLOOT) && corpse && corpse->contains)
                   return TRUE;  /* leave if corpse has treasure */
                else
                   do_sacrifice( ch, "corpse" );
             }
          } /* Ch was not a NPC */
       } /* Victim was a NPC */

       /* Now do things that need to be done with both */
       sprintf( log_buf, "%s got toasted by %s at %s [room %d]",
          (IS_NPC(victim) ? victim->short_descr : victim->name),
          (IS_NPC(ch) ? ch->short_descr : ch->name),
          died_in_room->name, died_in_room->vnum);


       log_string( log_buf );
       if (IS_NPC(victim))
          wiznet(log_buf,NULL,NULL,WIZ_MOBDEATHS,0,0);
       else
       {
          wiznet(log_buf,NULL,NULL,WIZ_DEATHS,0,0);
          if( !is_clan(victim) )
          {
            pnet("$N died.",victim,NULL,PNET_DEATHS,0,0);
          }
        }

        sprintf(cdbuf, "before return: %s, ch: %s", victim->name, ch->name);
        log_string(cdbuf);

       return TRUE;
    } /* If victim position = DEAD */

    if ( victim == ch || is_dead)
       return TRUE;

    /*
     * Take care of link dead people.
     */
    if ( !IS_NPC(victim) && victim->desc == NULL && victim->level <= 10 )
    {
       if ( number_range( 0, victim->wait ) == 0 )
       {
          if (!IS_SET(victim->act,PLR_NOAUTORECALL))
            recall(victim, "", FALSE);
//             do_recall( victim, "" );
          return TRUE;
       }
    }

    /*
     * Wimp out?
     */
    if ( IS_NPC(victim) && dam > 0 && victim->wait < PULSE_VIOLENCE / 2)
    {
       if ( ( IS_SET(victim->act, ACT_WIMPY) && number_bits( 2 ) == 0
          &&   victim->hit < victim->max_hit / 5)
          ||   ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL
          &&     victim->master->in_room != victim->in_room ) )
          do_flee( victim, "" );
    }
/*02OCT02 - added the first victim->wimpy >0 below*/

    if ( !IS_NPC(victim)
       &&   !victim->pcdata->quit_time
       &&   victim->wimpy > 0
       &&   victim->hit > 0
       &&   (victim->hit*100)/UMAX(1, victim->max_hit) <= victim->wimpy
       &&   victim->wait < PULSE_VIOLENCE / 2
       &&  (!IS_SET(victim->mhs, MHS_GLADIATOR) || (gladiator_info.exper == FALSE && gladiator_info.WNR == FALSE)))
       flee( victim, "", TRUE );

    tail_chain( );
    return TRUE;
} /* bool damage */


void dam_message_new( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,bool
immune ) {
    char buf1[256], buf2[256], buf3[256];
    const char *vp1;
    const char *vp2;
    const char *vp3;
    const char *attack;

    if (ch == NULL || victim == NULL)
  return;

         if ( dam ==   0 ) { vp1 = "clumsy";    vp2 = "{ymisses{x";
                             vp3 = " harmlessly."                       ;}
    else if ( dam <=   4 ) { vp1 = "clumsy";    vp2 = "{cgives{x";
                             vp3 = " a bruise."           ;}
    else if ( dam <=   8 ) { vp1 = "wobbly";    vp2 = "{chits{x";
                             vp3 = " causing scrapes."     ;}
    else if ( dam <=  12 ) { vp1 = "lucky";    vp2 = "{chits{x";
                             vp3 = " making scratches."                 ;}
    else if ( dam <=   16 ) { vp1 = "amateur";  vp2 = "{chits{x";
                             vp3 = " causing light wounds."             ;}
    else if ( dam <=   20 ) { vp1 = "amateur";    vp2 = "{cstrikes{x";
                             vp3 = ", the wound bleeds."      ;}
    else if ( dam <=   26 ) { vp1 = "competent";    vp2 = "{cstrikes{x";
                             vp3 = ", hitting an organ."      ;}
    else if ( dam <=  32 ) { vp1 = "competent";    vp2 = "{ccauses{x";
                             vp3 = " to gasp in pain."        ;}
    else if ( dam <=  38 ) { vp1 = "skillful";    vp2 = "{ccauses{x";
                             vp3 = " harm!"           ;}
    else if ( dam <=  44 ) { vp1 = "skillful";
                  vp2 = "has a {cdevastating{x effect on"; vp3 = "."        ;}
    else if (dam <=   50 ) { vp1 = "cunning";   vp2 = "{ctears{x into";
                             vp3 = ", shredding flesh."           ;}
    else if ( dam <=  60 ) { vp1 = "strong";    vp2 = "{ccauses{x";
                             vp3 = " to spurt {Rblood{x!"         ;}
    else if ( dam <=  70 ) { vp1 = "{ccalculated{x";
    vp2 = "leaves large gashes on"; vp3 = "!"            ;}
    else if ( dam <=  80 ) { vp1 = "{ccalculated{x";        vp2 = "{ctears{x";
                             vp3 = " leaving a {RGAPING{x hole!"            ;}
    else if ( dam <=  87 ) { vp1 = "well aimed";    vp2 = "{CDISEMBOWELS{x";
                             vp3 = ". Guts spill out!!"         ;}
    else if ( dam <= 94 ) { vp1 = "calm";    vp2 = "{CDISMEMBERS{x";
                             vp3 = "! {RBlood{x splatters!" ;}
    else if ( dam <= 105 ) { vp1 = "wicked";    vp2 = "{CANNIHILATES{x";
                             vp3 = "!!"           ;}
    else if ( dam <= 117 ) { vp1 = "wicked";    vp2 = "{COBLITERATES{x";
                             vp3 = " completely!! "     ;}
    else if ( dam <= 125 ) { vp1 = "barbaric";    vp2 = "{CMASSACRES{x";
                             vp3 = ". Blood flies!"       ;}
    else if ( dam <= 130 ) { vp1 = "controlled";    vp2 = "{CERADICATES{x";
                             vp3 = " to bits!!"         ;}
    else                   { vp1 = "masterful";
                vp2 = "does {RUNSPEAKABLE{x things to"; vp3 = "!"         ;}


    if ( dt == TYPE_HIT )
    {
  if (ch  == victim)
  {
      sprintf( buf1, "$n fumbles and hits $melf!");
      sprintf( buf2, "You fumble and hit yourself!");
  }
  else
  {
             sprintf( buf1, "$n's %s strike %s $N%s",vp1,vp2,vp3);
             sprintf( buf2, "Your %s strike %s $N%s",vp1,vp2,vp3);
             sprintf( buf3, "$n's %s strike %s you%s",vp1,vp2,vp3);

  }
    }
    else
    {
  if ( dt >= 0 && dt < MAX_SKILL )
      attack  = skill_table[dt].noun_damage;
  else if ( dt >= TYPE_HIT
  && dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE)
      attack  = attack_table[dt - TYPE_HIT].noun;

  else
  {
      bug( "Dam_message: bad dt %d.", dt );
      dt  = TYPE_HIT;
      attack  = attack_table[0].name;
  }

  if (immune)
  {
      if (ch == victim)
      {
    sprintf(buf1,"$n is unaffected by $s own %s.",attack);
    sprintf(buf2,"Luckily, you are immune to that.");
      }
      else
      {
        sprintf(buf1,"$N is unaffected by $n's %s!",attack);
        sprintf(buf2,"$N is unaffected by your %s!",attack);
        sprintf(buf3,"$n's %s is powerless against you.",attack);
      }
  }
  else
  {
      if (ch == victim)
      {
                sprintf( buf1, "$n's %s %s %s $m%s", vp1, attack, vp2, vp3 );
                sprintf( buf2, "Your %s %s %s you%s", vp1, attack, vp2, vp3 );

      }
      else
      {
                sprintf( buf1, "$n's %s %s %s $N%s", vp1, attack, vp2, vp3 );
                sprintf( buf2, "Your %s %s %s $N%s", vp1, attack, vp2, vp3 );
                sprintf( buf3, "$n's %s %s %s you%s",vp1, attack, vp2, vp3 );

      }
  }
    }

    if (ch == victim)
    {
  act(buf1,ch,NULL,NULL,TO_ROOM,FALSE);
  if(!(IS_SET(ch->display,DISP_BRIEF_COMBAT) && dam == 0))
  act(buf2,ch,NULL,NULL,TO_CHAR,FALSE);
    }
    else
    {
      act( buf1, ch, NULL, victim, TO_NOTVICT ,FALSE);
  if(!(IS_SET(ch->display,DISP_BRIEF_COMBAT) && dam == 0))
      act( buf2, ch, NULL, victim, TO_CHAR ,FALSE);
  if(!(IS_SET(victim->display,DISP_BRIEF_COMBAT) && dam == 0))
      act( buf3, ch, NULL, victim, TO_VICT ,FALSE);
    }
    return;
}

bool is_clan_guard(CHAR_DATA *victim)
{
  if ( IS_NPC(victim) && victim->spec_fun != 0 )
  {
    if (
         victim->spec_fun == spec_lookup("spec_honor_guard")
         || victim->spec_fun == spec_lookup("spec_demise_guard")
         || victim->spec_fun == spec_lookup("spec_posse_guard")
         || victim->spec_fun == spec_lookup("spec_zealot_guard")
         || victim->spec_fun == spec_lookup("spec_warlock_guard")
       )
       return TRUE;
  }
  return FALSE;
}

bool is_safe(CHAR_DATA *ch, CHAR_DATA *victim)
{
  return is_safe_spell(ch, victim, FALSE, 0);
}

bool is_safe_steal(CHAR_DATA *ch, CHAR_DATA *victim)
{
  return is_safe_spell(ch, victim, FALSE, 0);
}

bool is_safe_spell(CHAR_DATA *ch, CHAR_DATA *victim, bool area , int sn )
{
    if (victim == ch)
    {
      if(area)
        return TRUE;
      return FALSE;
    }

    if (victim->in_room == NULL || ch->in_room == NULL)
       return TRUE;

    if (victim->fighting == ch || victim == ch)
       return FALSE;

    if ((!IS_NPC(ch)) && (ch->level > LEVEL_IMMORTAL))
       return FALSE;

    if (is_clan(ch) && !IS_NPC(victim) && ch->pcdata->start_time > 0 && ch != victim)
    {
      send_to_char("Easy there sparky.  You just got here.  Read some notes and such.\n\r",ch);
      return TRUE;
    }

   if (IS_SET(victim->in_room->room_flags,ROOM_NOCOMBAT))
   {
      send_to_char("No Combat in this room.\n\r",ch);
      return TRUE;
   }

   if( is_affected(victim,skill_lookup("wraithform")) )
   {
   send_to_char("They are made of spooky, wraith-like mist.\r\n",ch);
   return TRUE;
   }

   if( is_affected(ch,skill_lookup("wraithform")) )
   {
   return TRUE;
   }

    if (victim->passenger != NULL && is_safe_spell(ch,victim->passenger,area, sn) )
       return TRUE;

    /*if (IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_SET(ch->mhs,MHS_HIGHLANDER)
        && !IS_NPC(victim))
    {
       send_to_char("They are a Highlander, You are not.\n\r",ch);
       return TRUE;
    }

    if (IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_SET(victim->mhs,MHS_HIGHLANDER)
        && !IS_NPC(ch) && !IS_NPC(victim))
    {
       send_to_char("You are a Highlander, They are not.\n\r",ch);
       return TRUE;
    }

    if (IS_SET(ch->mhs,MHS_HIGHLANDER) &&
        IS_SET(victim->in_room->room_flags,ROOM_HOLY_GROUND))
   {
      send_to_char("This is Holy Ground Highlander!\n\r",ch);
      return TRUE;
   }*/

    if (IS_IMMORTAL(ch) && ch->level > LEVEL_IMMORTAL && !area)
       return FALSE;

    /* Special handling for Altirin mobs -Ben */
    if ( IS_NPC(victim) && victim->invis_level )
       return TRUE;

    if ( is_clan(victim) && victim->in_room->clan &&
         victim->in_room->clan != victim->clan )
       return FALSE;

    /* You cannot be attacked if you're in your own hall */
    if ( is_clan(ch) && ch->in_room->clan &&
         ch->in_room->clan != ch->clan )
       return TRUE;
    /* attacking a player's creature by another player */
/*    if ( IS_NPC(victim)
         && victim->master != NULL
         && victim->master->fighting != ch
         && (ch->level > (victim->master->level +8))
         && is_clan(ch) && is_clan(victim->master))
       return TRUE;*/

    if( area && is_same_group(ch,victim) )
       return TRUE;

    /* killing mobiles */
    if (IS_NPC(victim))
    {
       if (IS_AFFECTED(victim,AFF_CHARM) && (area || 
         (ch != victim->master && victim->master && is_safe(ch, victim->master))))
          return TRUE;

       /* safe room? */
       if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
       {
          send_to_char("Not in this room.\n\r",ch);
          return TRUE;
       }

       if (victim->pIndexData->pShop != NULL)
       {
          send_to_char("The shopkeeper wouldn't like that.\n\r",ch);
          return TRUE;
       }

       /* no killing healers, trainers, etc */
       if (IS_SET(victim->act,ACT_TRAIN)
           ||  IS_SET(victim->act,ACT_PRACTICE)
           ||  IS_SET(victim->act,ACT_IS_HEALER)
           ||  IS_SET(victim->act,ACT_IS_CHANGER))
       {
          send_to_char("I don't think Mojo would approve.\n\r",ch);
          return TRUE;
       }
       if (!IS_NPC(ch) && victim->master != NULL)
       {
         if(!IS_NPC(victim->master) && victim->master != ch)
           return is_safe(ch, victim->master);
         return FALSE;
       }
       else if(IS_NPC(ch))
       {
         if (area && !is_same_group(victim,ch->fighting))
           return TRUE;
       }
    }
    /* killing players */
    else
    {
       if (area && IS_IMMORTAL(victim) && victim->level > LEVEL_IMMORTAL)
          return TRUE;
       /* NPC doing the killing */
       if (IS_NPC(ch))
       {
          /* safe room check */
          if (IS_SET(victim->in_room->room_flags,ROOM_SAFE))
          {
             send_to_char("Not in this room.\n\r",ch);
             return TRUE;
          }

          /* charmed mobs and pets cannot attack players while owned */
          if (IS_AFFECTED(ch,AFF_CHARM) && ch->master != NULL
              &&  ch->master->fighting != victim)
          {
             send_to_char("Players are your friends!\n\r",ch);
             stop_fighting(ch,FALSE);
             return TRUE;
          }
          if(area && ch->fighting && ch->fighting != victim &&
            !is_same_group(ch->fighting, victim))
            return TRUE; /* NPCs shouldn't blast hapless players with aoe */
       }
       /* player doing the killing */
       else
       {
          if (IS_SET(victim->act,PLR_DWEEB))
             return FALSE;

          if(IS_SET(ch->mhs,MHS_GLADIATOR) && (gladiator_info.type == 2 || gladiator_info.type == 3) && ch->pcdata->gladiator_team == victim->pcdata->gladiator_team)
             return TRUE;

          if(IS_SET(ch->mhs,MHS_GLADIATOR) && IS_SET(victim->mhs,MHS_GLADIATOR))
             return FALSE;
          if(IS_SET(ch->mhs,MHS_HIGHLANDER) && IS_SET(victim->mhs,MHS_HIGHLANDER))
             return FALSE;

          if(is_clan_friendly(ch, victim))
          {
            send_to_char("They are not a legal target for you.\n\r", ch);
            return TRUE;
          }

          if (IS_SET(ch->act,PLR_NOOUTOFRANGE) )
          {

             if ((IS_SET(ch->act,PLR_KILLER) || IS_SET(ch->act,PLR_THIEF)) &&
                 ch->level+12 < victim->level)
             {
               send_to_char("If you want to pick on someone that big turn "
                         "off your NoOutOfRange toggle.\n\r",ch);
               return TRUE;
             }
             else
             {
                if (victim->level > ch->level + (ch->trumps > 0 ? 10 : 8))
                {
                   send_to_char("If you want to pick on someone that big turn "
                         "off your NoOutOfRange toggle.\n\r",ch);
                   return TRUE;
                }
             }
          }

/*        if (IS_SET(victim->act,PLR_KILLER) || IS_SET(victim->act,PLR_THIEF))    */
/*        This change made by Ndagger, 1/23/03                                    */
       if (IS_SET(victim->act,PLR_KILLER))
       {
          if (victim->level + 12 < ch->level &&
              str_cmp(ch->pcdata->last_attacked_by,victim->name))
          {
             if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("Pick on someone your own size.\n\r",ch);
             return TRUE;
          }
       }
       else if  ( IS_SET(victim->act,PLR_THIEF) )
       {
          if (victim->level + 12 < ch->level &&
              str_cmp(ch->pcdata->last_attacked_by,victim->name))
          {
             if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("Pick on someone your own size.\n\r",ch);
             return TRUE;
          }
       }
       else
       {
          /* Victim is a Thug or Ruffian */
          if (ch->level > victim->level + (victim->trumps > 0 ? 10 : 8) &&
              str_cmp(ch->pcdata->last_attacked_by,victim->name))
          {
             if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
                send_to_char("Pick on someone your own size.\n\r",ch);
                return TRUE;
          }
       }
    /* If someone not fighting the victim casts a spell at the victim then
      add the caster to the victims last attacked by, but only if the
      character is normally outside the victim's level range */
          if (((IS_SET(ch->act,PLR_THIEF) || IS_SET(ch->act,PLR_KILLER))
                    && ch->level+12 < victim->level )
               || (ch->trumps == 0 && ch->level+8 < victim->level)
               || (ch->trumps > 0 && ch->level+10 < victim->level ) )
               {
                victim->pcdata->last_attacked_by = str_dup(ch->name);
                victim->pcdata->last_attacked_by_timer = 15000;
               }

       }
    }
    return FALSE;
}

/*
 * See if an attack justifies a KILLER flag.
 */
void check_killer( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH],log_buf[MAX_STRING_LENGTH];
    /*
     * Follow charm thread to responsible character.
     * Attacking someone's charmed char is hostile!
     */
    if(ch != victim)
    {
      ch->alert = get_curr_stat(ch, STAT_INT) / 5 + get_curr_stat(ch, STAT_WIS) / 5;
      victim->alert = get_curr_stat(victim, STAT_INT) / 5 + get_curr_stat(victim, STAT_WIS) / 5;
    }
    else
      return;

    if ( victim->passenger != NULL )
    {
        check_killer( ch, victim->passenger );
        return;
    }

    if ((!IS_NPC(ch) && !IS_NPC(victim)) && (ch != victim) && ch->pcdata) {
      ch->pcdata->quit_time = 4;
      if(ch->pcdata->deity_trial_timer > 0)
      {
        ch->pcdata->deity_trial_timer = 0;
        sprintf(buf, "%s releases you from the trial so you may fight for your life!\n\r", deity_table[ch->pcdata->deity].pname);
        send_to_char(buf, ch);
        log_deity_favor(ch, victim, DEITY_TRIAL_FAIL_PK);
      }
      if (victim->pcdata)
        {
        victim->pcdata->quit_time = 4;
              if(victim->pcdata->deity_trial_timer > 0)
              {
                victim->pcdata->deity_trial_timer = 0;
                sprintf(buf, "%s releases you from the trial so you may fight for your life!\n\r", deity_table[victim->pcdata->deity].pname);
                send_to_char(buf, victim);
                log_deity_favor(victim, ch, DEITY_TRIAL_FAIL_PK);
              }
        }
    }

    if ( !IS_NPC(victim) && !IS_NPC(ch)
         && is_affected(victim,gsn_sacred_guardian) && is_clan( victim )
         && ch != victim
         && ch->position != POS_FIGHTING && victim->position != POS_FIGHTING )
    {
        DESCRIPTOR_DATA *d;

        for ( d = descriptor_list ; d!= NULL ; d= d->next )
        {
           if ( d->connected != CON_PLAYING ||
                !is_same_clan(d->character,victim) )
               continue;

           act("Your clanmate $N is being attacked!",
                d->character,NULL,victim,TO_CHAR,TRUE);
        }
    }

    /*
     * No flags for defending your clan hall from an infiltrator.
     */
     /*if(!IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_SET(victim->mhs,MHS_HIGHLANDER))
     {
     if ( !IS_NPC(ch) && !IS_NPC(victim)
     && ch->in_room->clan == ch->clan
     && is_clan(victim) && is_clan(ch)
     && victim->in_room->clan != victim->clan
     && !IS_SET(victim->act,PLR_THIEF)
     && ch->in_room == victim->in_room )
     {
       send_to_char("You got caught infiltrating, THIEF!\n\r",victim);
       SET_BIT(victim->act,PLR_THIEF);
       sprintf( log_buf, "%s got a THIEF caught by %s infiltrating %d",
                victim->name,ch->name,victim->in_room->vnum );
        log_string( log_buf );
          sprintf(buf,"$N got a THIEF caught infiltrating by %s",ch->name);
          wiznet(buf,victim,NULL,WIZ_TRANSGRESSION,0,0);
     }
     }*/

    if (!IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       if(!IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_SET(victim->mhs,MHS_HIGHLANDER))
       {

/*    Change by Nightdagger on July 10th, 2003, lets see if this fixes it
      Old statement preseved here.
Sorry Ken, had to put it back in, life with out transgres is to much :)
we'll find another solution to it -corey */


          if ((!IS_NPC (ch) && !IS_NPC (victim)) && (ch != victim))
          /*taken out till we can find another solution
                if ((!IS_NPC(ch) && !IS_NPC(victim)) && (ch != victim))
          */
           {
             if(ch->position != POS_FIGHTING && victim->position != POS_FIGHTING)
             {
               sprintf(buf,"$N is attempting to kill %s",victim->name);
               wiznet(buf,ch,NULL,WIZ_TRANSGRESSION,0,0);
               ch->pcdata->last_combat_date = current_time;
               ch->pcdata->logins_without_combat = 0;
               ch->pcdata->combats_since_last_login++;
             }
              if(!IS_SET(ch->wiznet,PLR_RUFFIAN) && victim->trumps == 0
                 && !IS_SET(victim->wiznet,PLR_RUFFIAN)
                 && !IS_SET(victim->act,PLR_DWEEB)
                 && !IS_SET(victim->act,PLR_THIEF) )
              {

                //COREY TAKE THIS OUT AND SEE IF IT FIXED FLAGS
        //      if( !(ch->clan == clan_lookup("posse")
                //      && IS_SET(victim->mhs,MHS_POSSE_ENEMY)) )
                //{
                 SET_BIT(ch->wiznet,PLR_RUFFIAN);
                 ch->pcdata->ruffT = 500;
                 sprintf( log_buf, "%s got a RUFFIAN attacking %s at %d",ch->name,
                    victim->name,ch->in_room->vnum );
                 log_string( log_buf );
                 sprintf(buf,"$N got a RUFFIAN by attacking %s",victim->name);
                 wiznet(buf,ch,NULL,WIZ_TRANSGRESSION,0,0);
                //}
              }
           }
        }
    }

    if ( IS_AFFECTED(victim, AFF_CHARM) && !IS_NPC(victim))
      stop_follower(victim);


    while ( IS_AFFECTED(victim, AFF_CHARM) && victim->master != NULL )
  victim = victim->master;

    /*
     * NPC's are fair game.
     * So are killers and thieves.
     */
    if ( IS_NPC(victim)
    ||   IS_SET(victim->act, PLR_KILLER)
    ||   IS_SET(victim->act, PLR_THIEF)
    ||   IS_SET(victim->act, PLR_DWEEB) )
  return;

    /*
     * Charm-o-rama.
     */
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
    {
  if ( ch->master == NULL )
  {
      char buf[MAX_STRING_LENGTH];

      sprintf( buf, "Check_killer: %s bad AFF_CHARM",
    IS_NPC(ch) ? ch->short_descr : ch->name );
      bug( buf, 0 );
      affect_strip( ch, gsn_charm_person );
      REMOVE_BIT( ch->affected_by, AFF_CHARM );
      return;
  }
/*
  send_to_char( "*** You are now a KILLER!! ***\n\r", ch->master );
    SET_BIT(ch->master->act, PLR_KILLER);
  stop_follower( ch );
*/
  return;
    }

    /*
     * NPC's are cool of course (as long as not charmed).
     * Hitting yourself is cool too (bleeding).
     * So is being immortal (Alander's idea).
     * And current killers stay as they are.
     */
    if ( IS_NPC(ch)
    ||   ch == victim
    ||   ch->level >= LEVEL_IMMORTAL
    ||   !is_clan(ch)
    ||   IS_SET(ch->act, PLR_KILLER)
    ||   ( IS_SET(victim->affected_by, AFF_CHARM) && !IS_NPC(victim) )
       )
  return;

   if(ch->trumps >= 3)
        {
         send_to_char( "*** You are now a KILLER!! ***\n\r", ch );
         SET_BIT(ch->act, PLR_KILLER);
         sprintf(buf,"$N got a (KILLER) attempting to murder %s",victim->name);
         wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
         save_char_obj( ch );
         return;
        }
}



/*
 * Check for parry.
 */
bool check_parry( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary )
{
    int chance;
    int hit ;
    OBJ_DATA *victim_wield;
    OBJ_DATA *wield;

    victim_wield= get_eq_char(victim,WEAR_WIELD);

    if(victim_wield != NULL && (victim_wield->damaged >= 100 ||
      (!IS_NPC(victim) && victim->pcdata->weapon_too_heavy)))
        return FALSE;// Can't parry with a brokenor too heavy  weapon

    if (fSecondary && !IS_NPC(ch))
    {
       wield = get_eq_char(ch,WEAR_SECOND) ;
       hit = ch->pcdata->second_hitroll;
    }
    else
    {
       wield = get_eq_char(ch,WEAR_WIELD) ;
       hit = ch->hitroll;
    }

    if ( !IS_AWAKE(victim) )
  return FALSE;


    /* Shogun kit - awesome on defensive.
        Can flag-out parry anything 10% of the time
        */
    if (check_hai_ruki(victim) && number_percent() < 10 )
    {
    if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
    act( "You parry $n's attack.",  ch, NULL, victim, TO_VICT    ,FALSE);
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
    act( "$N parries your attack.", ch, NULL, victim, TO_CHAR    ,FALSE);
    check_improve(victim,gsn_parry,TRUE,6);
    return TRUE;
    }

  if (!IS_NPC(victim) && victim_wield != NULL && victim->size < SIZE_LARGE
  &&  IS_WEAPON_STAT(victim_wield,WEAPON_TWO_HANDS)
  &&  get_eq_char(victim,WEAR_SHIELD) != NULL && !is_flash_affected(victim, skill_lookup("disarm")))
  {
    AFFECT_DATA af;
    af.where     = TO_AFFECTS;
    af.type      = gsn_disarm;
    af.level     = ch->level;
    af.location  = APPLY_HITROLL;
    af.modifier  = ch->level / -2;
    af.duration  = 2;
    flash_affect_to_char( ch, &af );
    af.location = APPLY_DAMROLL;
    flash_affect_to_char( ch, &af );

      send_to_char("You fumble your oversized weapon and hurt your wrist holding on to it.\n\r",victim);
      //obj_from_char( victim_wield);
      //obj_to_char( victim_wield, victim );
      return FALSE;
  }

    chance = get_skill(victim,gsn_parry) / 2;
    if ( victim->class == class_lookup("druid") )
          chance -= UMAX(1, chance / 5 );

    if ( ch->class == class_lookup("blademaster") )
        chance -= ( chance / 5 );

    chance += myrm_pen(ch, victim);

    /* Ranger bonus for terrain if victim */
    switch( terrain(victim) )
    {
    case 0:     chance -= ( chance / 5 );       break;
    case 1:     chance += ( chance / 5 );       break;
    default:    break;
    }

    if ( victim_wield == NULL )
    {
  if (IS_NPC(victim) || IS_AFFECTED(victim,AFF_MORPH))
      chance /= 2;
  else
      return FALSE;
    }

    if (!can_see(ch,victim,FALSE))
  chance /= 2;

    if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_WERE)  &&
             is_affected(ch,gsn_morph) && ( ch->hit < ch->max_hit / 4 ) )
  chance /= 3;

    if ( is_enemy(ch,victim) )
  chance -= 5;

    if ( is_affected(ch,gsn_rage))
        chance -= (get_skill(ch, gsn_rage) /10);


   /* CON affects parry now, similar to size code for dodge
      CON is your "stamina" in a fight. Favours large races
      compare to small races with low con AND forces people
      to train their con and/or wear con eq , a lower con
      for the victim as compared to the attacker will
      reduce the victims parry chance , a higher victim con
      improves the victims parry % */

  chance +=( (get_curr_stat(victim,STAT_CON) - get_curr_stat(ch,STAT_CON))*2 );

    /*If you know your attacker's weapon, you parry better */
    /* Also, if you know your vicitm's weapon, you hit more often */
    if (fSecondary)
       chance += get_weapon_skill(victim,get_weapon_sn(ch,TRUE))/10;
    else
       chance += get_weapon_skill(victim,get_weapon_sn(ch,FALSE))/10;

  /* Your primary weapon can't be too heavy -- we're not checking secondary at this point */
    if(fSecondary || wield == NULL || IS_NPC(ch) || !ch->pcdata->weapon_too_heavy)
      chance -= get_weapon_skill(ch,get_weapon_sn(victim,FALSE))/10;


   /* Hitrolls */
   if (hit > 20)
     hit = (hit-20)/2 +20;
   if (hit > 40)
     hit = (hit - 40)/2 +40;

    chance -= hit;
    /*
    chance += get_weapon_skill(victim,get_weapon_sn(victim,FALSE))/4;
    */

    /* If the weapon is the secondary then harder to parry */
    if (fSecondary)
       chance -= 25;

    if ( number_percent( ) >= chance + victim->level - ch->level )
  return FALSE;

    if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
    act( "You parry $n's attack.",  ch, NULL, victim, TO_VICT    ,FALSE);
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
    act( "$N parries your attack.", ch, NULL, victim, TO_CHAR    ,FALSE);
    check_improve(victim,gsn_parry,TRUE,6);
    return TRUE;
}

/*
 * Check for shield block.
 */
bool check_shield_block( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary )
{
    OBJ_DATA *weapon, *shield;
    int chance;

    if ( !IS_AWAKE(victim) )
        return FALSE;


    chance = get_skill(victim,gsn_shield_block) / 5 + 3;
    if ( victim->class == class_lookup("druid") )
          chance -= UMAX(1, chance / 5 );

    chance += myrm_pen(ch,victim);

    /* ranger bonus for terrain if victinm */
    switch( terrain(ch) )
    {
    case 0:     chance -= ( chance / 5 );       break;
    case 1:     chance += ( chance / 5 );       break;
    default:    break;
    }

    if ( ( shield = get_eq_char( victim, WEAR_SHIELD ) ) == NULL || shield->damaged >= 100)
        return FALSE;

    if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_WERE)  &&
             is_affected(ch,gsn_morph) && ( ch->hit < ch->max_hit / 4 ) )
        chance /= 3;

    if ( ch->class == class_lookup("blademaster") )
        chance -= ( chance / 5 );

    if (fSecondary)
       weapon = get_eq_char(ch,WEAR_SECOND) ;
    else
       weapon = get_eq_char(ch,WEAR_WIELD) ;

    if (  weapon != NULL &&
           weapon->value[0] == WEAPON_WHIP )
        chance /= 2;

    if ( weapon && weapon->value[0] == WEAPON_FLAIL )
        return FALSE;

    if ( is_enemy(ch,victim))
        chance -= 5;

    /* If the weapon is the secondary then harder to block */
    if (fSecondary)
       chance -= 25;

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
    act( "You block $n's attack with your shield."
        ,  ch, NULL, victim, TO_VICT, FALSE);
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
    act( "$N blocks your attack with a shield."
        , ch, NULL, victim, TO_CHAR, FALSE);
    check_improve(victim,gsn_shield_block,TRUE,6);

   if ( weapon && weapon->value[0] == WEAPON_AXE
        && !IS_SET(victim->mhs, MHS_GLADIATOR))
   {
    int split = 25;

     /* adjust for material type
     split = 100 - material_table[shield->material].toughness;
     */

     /* adjust for magic */
     if (  IS_OBJ_STAT(shield,ITEM_MAGIC) )
      split /= 3;

       if (  IS_OBJ_STAT(shield,ITEM_GLOW) )
       split = 2 * split / 3;

               split -= ( get_skill(victim,gsn_shield_block) / 10 );
                split = URANGE(5, split, 95);

               if ( number_percent()<split)
               {/*splittheshield*/
                shield->damaged = UMIN(shield->damaged + number_range(34, 50), 100);
                if(shield->damaged >= 100)
                {
                        act("$N's shield splits in two!",ch,NULL,victim,TO_CHAR,FALSE);
                        act("Your shield splits in two!",ch,NULL,victim,TO_VICT,FALSE);
                        remove_bonuses(shield->carried_by, shield);
                }
                else
                {
                        act("$N's shield is chopped through, but still usable.",ch,NULL,victim,TO_CHAR,FALSE);
                        act("Your shield is chopped through, but still usable.",ch,NULL,victim,TO_VICT,FALSE);
                }

/*              act("$N's shield splits in two!",ch,NULL,victim,TO_CHAR,FALSE);
                act("Your shield splits in two!",ch,NULL,victim,TO_VICT,FALSE);
                   obj_from_char(shield);
                          extract_obj(shield);*/
                }
    }

    return TRUE;
}


/*
 * Check for scaled creatures
 */
bool check_scales( CHAR_DATA *ch, CHAR_DATA *victim , bool fSecondary)
{
    int chance;
    OBJ_DATA *weapon;

    if ( !IS_SET(race_table[victim->race].parts,PART_SCALES) )
        return FALSE;

    chance = victim->level / 2 + 5;
    chance += victim->size * 5;
    chance -= get_curr_stat( ch, STAT_STR ) * 2;

    if ( HAS_KIT(ch,"wyrmslayer") )
        return FALSE;

    if (fSecondary)
       weapon = get_eq_char(ch,WEAR_SECOND) ;
    else
       weapon = get_eq_char(ch,WEAR_WIELD) ;

    if (  weapon != NULL &&
           IS_WEAPON_STAT( weapon, WEAPON_SHARP ) )
           chance /= 3;

    chance = URANGE( 5, chance, 95 );

    if ( number_percent() > chance )
        return FALSE;

    if ( !IS_SET(ch->display,DISP_BRIEF_COMBAT) )
    act("Your attack is deflected harmlessly off $N's scales.",
        ch,NULL,victim,TO_CHAR,FALSE);
    if ( !IS_SET(victim->display,DISP_BRIEF_COMBAT) )
    act("$n's attack is deflected off your scales.",
        ch,NULL,victim,TO_VICT,FALSE);

    return TRUE;
}



bool check_mistform( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;
    AFFECT_DATA *paf;


    if ( !is_affected(victim, skill_lookup("mistform")) )
        return FALSE;

    chance = 50 + ( ( victim->level - ch->level ) * 5 );

    for ( paf = victim->affected ; paf != NULL ; paf = paf->next )
        if ( paf->type == skill_lookup("mistform") )
                break;

    if ( paf == NULL )
    {
        bug("check_mistform: no affect",0);
        return FALSE;
    }

    paf->duration = UMAX(0,paf->duration - 1);
    if (paf->duration == 0)
    {
      send_to_char(skill_table[paf->type].msg_off,victim);
      send_to_char("\n\r",victim);
        act("$n's misty form becomes more corporeal.",victim,NULL,NULL,TO_ROOM,FALSE);
      affect_remove(victim, paf, APPLY_BOTH );
        return FALSE;  /* paf is no longer valid */
    }

    chance += ( paf->duration / 2 );

    if ( number_percent() < chance )
    {
        act("Your attack passes through $N ineffectually.",
                ch,NULL,victim,TO_CHAR,FALSE);
        act("$n's attack passes ineffectually through you.",
                ch,NULL,victim,TO_VICT,FALSE);
        return TRUE;
    }

    return FALSE;
}


/*
 * Check for dodge.
 */
bool check_dodge( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary )
{
    int chance;
    int hit ;

    if (fSecondary && !IS_NPC(ch))
       hit = ch->pcdata->second_hitroll;
    else
       hit = ch->hitroll;

    if ( !IS_AWAKE(victim) )
        return FALSE;

    /* Shogun kit - awesome on defensive.
        Can flag-out dodge anything 10%o f the time
        */
    if (check_hai_ruki(victim) && number_percent() < 10 )
    {
    if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
    act( "You dodge $n's attack.", ch, NULL, victim, TO_VICT    ,FALSE);
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
    act( "$N dodges your attack.", ch, NULL, victim, TO_CHAR    ,FALSE);
    check_improve(victim,gsn_dodge,TRUE,6);
    return TRUE;
    }

    chance = get_skill(victim,gsn_dodge) / 2;

    if ( victim->class == class_lookup("druid") )
      chance -= UMAX( 1, (chance / 5) );

    chance += myrm_pen( ch, victim );

    /* rangers */
    switch( terrain(ch) )
    {
    case 0:             chance -= ( chance / 5 );       break;
    case 1:             chance += ( chance / 5 );       break;
    default:            break;
    }

    if ( victim->race == race_lookup("faerie") )
        chance = ( 4 * chance ) / 3;

    if ( ch->class == class_lookup("blademaster") )
        chance -= ( chance / 5 );

    if( IS_AFFECTED(victim,AFF_MORPH) )
        chance += victim->level/5;

/* adjust for dex/hitroll */
  chance += (2 * get_curr_stat(victim,STAT_DEX) / 3);

   if (hit > 20)
        hit = (hit-20)/2 +20;
   if (hit > 40)
        hit = (hit - 40)/2 +40;
  chance -= hit;

/* Adjust for size */
/* Smaller creatures can dodge bigger ones more easily */
    chance += ( ( ch->size - victim->size ) * 2 );

    if (is_affected(victim,gsn_blur))
  chance += 15;

    if ( number_percent() < get_skill(victim,gsn_tumbling))
    {
      if(victim->race == race_lookup("faerie"))
        chance += 5;/* Faeries don't benefit quite as much */
      else
        chance += 10;
    }

    if (!can_see(victim,ch,FALSE))
  chance /= 2;

    if ( !IS_NPC(ch) && IS_SET(ch->act,PLR_WERE)  &&
             is_affected(ch,gsn_morph) && ( ch->hit < ch->max_hit / 4 ) )
   chance /= 3;

    if ( is_affected(victim,gsn_fumble) )
        chance -= 15;

    if (is_enemy(ch,victim))
     chance -= 5;

    if ( is_affected(ch,gsn_bladesong) )
        chance -= (get_skill(ch, gsn_bladesong) /10);

    /* If the weapon is the secondary then harder to dodge */
    if (fSecondary)
       chance -= 25;

    if ( number_percent( ) >= chance + victim->level - ch->level )
    {
        if ( is_affected(victim,gsn_fumble) && number_percent() >
            ( ( get_curr_stat(victim,STAT_DEX) * 2 ) +
              ( get_skill( victim, gsn_dodge ) / 2 ) ) )
        {
            act("$N trips and falls while trying to dodge!",
                ch,NULL,victim,TO_CHAR,FALSE);
            act("You trip and fall while trying to dodge!",
                ch,NULL,victim,TO_VICT,FALSE);
            DAZE_STATE(victim,PULSE_VIOLENCE / 2);
            victim->position = POS_RESTING;
            victim->move = (UMAX(0,victim->move - 5));
        }
      return FALSE;
    }

    if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
    act( "You dodge $n's attack.", ch, NULL, victim, TO_VICT    ,FALSE);
    if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
    act( "$N dodges your attack.", ch, NULL, victim, TO_CHAR    ,FALSE);
    check_improve(victim,gsn_dodge,TRUE,6);
    return TRUE;
}

bool check_kailindo( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int chance;

    if ( !IS_AWAKE(victim) || IS_NPC(victim) )
  return FALSE;

    chance = get_skill(victim,gsn_kailindo) / 3;

    if(chance <= 2) return FALSE;

    if( IS_AFFECTED(victim,AFF_MORPH) )
        chance += victim->level/5;

    if (!can_see(victim,ch,FALSE))
      chance /= 2;
    if( !IS_NPC(ch)
    && ch->pcdata->old_class == class_lookup("thief")
    && ch->class == class_lookup("monk") )
    {
      chance += number_range(5,15);
    }

    if ( number_percent( ) >= chance + victim->level - ch->level )
        return FALSE;

    act( "You evade $n's attack and strike back.", ch, NULL, victim, TO_VICT    ,FALSE);
    act( "$N evades your attack and strikes back.", ch, NULL, victim, TO_CHAR    ,FALSE);
    one_hit( victim, ch, TYPE_UNDEFINED );
    check_improve(victim,gsn_kailindo,TRUE,6);
    return TRUE;
}


/*
 * Set position of a victim.
 */
void update_pos( CHAR_DATA *victim )
{
    if ( victim->hit > 0 )
    {
      if ( victim->position <= POS_STUNNED )
      victim->position = POS_STANDING;
  return;
    }

    if ( IS_NPC(victim) && victim->hit < 1 )
    {
  victim->position = POS_DEAD;
  return;
    }

    if ( victim->hit <= -11 )
    {
  victim->position = POS_DEAD;
  return;
    }

         if ( victim->hit <= -6 ) victim->position = POS_MORTAL;
    else if ( victim->hit <= -3 ) victim->position = POS_INCAP;
    else                          victim->position = POS_STUNNED;

    return;
}



/*
 * Start fights.
 */
void set_fighting( CHAR_DATA *ch, CHAR_DATA *victim )
{
    if ( ch->fighting != NULL )
    {
  bug( "Set_fighting: already fighting", 0 );
  return;
    }

    if ( IS_AFFECTED(ch, AFF_SLEEP) )
    {
  affect_strip( ch, gsn_sleep );
  affect_strip( ch, gsn_garotte );
    }

    if ( is_affected(ch, skill_lookup("garotte")))
       affect_strip (ch, skill_lookup("garotte"));

    if ( is_affected(ch, gsn_trap) )
    {
      affect_strip( ch, gsn_trap );
    }
    if ( is_affected(ch, skill_lookup("hold person") ) )
   affect_strip( ch, skill_lookup("hold person") );

    ch->fighting = victim;
    ch->position = POS_FIGHTING;

    return;
}



/*
 * Stop fights.
 *
 * Fix loophole where people can walk out mid-night
 *
void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;

    for ( fch = char_list; fch != NULL; fch = fch->next )
    {
  if ( (fch == ch) || ( fBoth && fch->fighting == ch ) )
  {
      fch->fighting = NULL;
      fch->position = IS_NPC(fch) ? ch->default_pos : POS_STANDING;
      update_pos( fch );
  }
    }

    return;
}
 */

void stop_fighting( CHAR_DATA *ch, bool fBoth )
{
    CHAR_DATA *fch;
    CHAR_DATA *och;

    ch->fighting = NULL;
    ch->position = IS_NPC(ch) ? ch->default_pos : POS_STANDING;
    update_pos( ch );

    if(fBoth)
    {
        for ( fch = char_list; fch != NULL; fch = fch->next )
        {
            if(fch->fighting == ch )
            {
                for(och = char_list; och!=NULL; och = och->next )
                {
                    if(och->fighting==fch&&och!=ch)
                        break;

                }

                if(!och)
                {
                    fch->fighting = NULL;
                    fch->position = IS_NPC(fch) ? fch->default_pos : POS_STANDING;
                    update_pos( fch );
                }
                else//NEW SECTION, only one you need to copy over
                {
                    fch->fighting = och;
                }

            }
        }
    }
    return;
}



/* loot_list is ONLY updated by "loot" from a clanner corpse, DO NOT LINK anywhere else */
void sort_clanner_items(CHAR_DATA *ch, OBJ_DATA *container, OBJ_DATA **loot_start, bool do_linked)
{
  OBJ_DATA *obj = container->contains;
  OBJ_DATA *obj_next;
  for(; obj; obj = obj_next)
  {
    obj_next = obj->next_content;
    if(obj->contains)
      sort_clanner_items(ch, obj, loot_start, do_linked);
    if(do_linked)
    {
      if(obj->link_name)
      {
        if(!str_cmp(obj->link_name, ch->name))
        {/* Vanish! */
            if(obj->damaged >= 100)
            {/* Peace type protects from shattered */
              if(!ch->pcdata || !ch->pcdata->clan_info || 
                ch->pcdata->clan_info->clan->type != CLAN_TYPE_PEACE ||
                ch->pcdata->clan_info->clan->tribute < 0 ||
                ch->pcdata->clan_info->clan->max_tribute < 20000)
              obj->damaged = 1000; /* Shattered */
            }
            else
            {
              int dam_amount;
              switch(obj->rarity)
              {
                default: dam_amount = 10; break;/* Stock or unknown */
                case RARITY_COMMON: dam_amount = 10; break;
                case RARITY_UNCOMMON: dam_amount = 35; break;
                case RARITY_RARE: dam_amount = 50; break;
                case RARITY_IMPOSSIBLE: dam_amount = 100; break;
              }
              obj->damaged = UMIN(100, obj->damaged + dam_amount);
            }
          obj_from_obj(obj);
          obj_to_char(obj, ch);
          obj->wear_loc = WEAR_HIDDEN;
          if(obj->contains)
          {
            OBJ_DATA *removing;
            while(obj->contains)
            {
              removing = obj->contains;
              obj_from_obj(removing);
              /* Start of the list, where we already checked -- that's okay, linked items can't be in containers */
              obj_to_obj(removing, container);
            }
          }
          continue;
        }
        else
        {/* What? */
          bug("Linked object found on wrong player.", 0);
        }
      }
    }
    obj->loot_next = *loot_start;
    *loot_start = obj;
  }
}

CHAR_DATA *check_is_online(char *name)
{
  DESCRIPTOR_DATA *d;
  for ( d = descriptor_list; d != NULL; d = d->next )
  {
    CHAR_DATA *owner = d->original ? d->original : d->character;
    if (d->connected == CON_PLAYING && owner && owner->pcdata &&
      !str_cmp(owner->name, name))
      return owner;
  }
  return NULL;
}

/* Based on damages */
void assign_clanner_looting(CHAR_DATA *ch, CHAR_DATA *killer, OBJ_DATA *corpse)
{
  char buf[256];
  int total_merit;
  MERIT_TRACKER *dmerit;
  DAMAGE_DATA *track, *top = NULL;
  bool killer_set = FALSE;
  bool merit_steal = FALSE;
  int total = 0;

  if(IS_NPC(ch))
    return;

  if(ch->pcdata->clan_info)
  {
    total_merit = ch->pcdata->clan_info->merit;
    for(dmerit = ch->pcdata->clan_info->delay_merit; dmerit; dmerit = dmerit->next)
      total_merit += dmerit->amount;
  }
  else
    total_merit = 0;

  if((!IS_NPC(killer) || (killer->master && !IS_NPC(killer->master)))
    && killer != ch)
  {
    CHAR_DATA *target = IS_NPC(killer) ? killer->master : killer;
    if(target->pcdata->clan_info)
    {/* Merit bonuses only if in a new clan */
      int bonus_merit = (ch->level * ch->level) / 20;
      bonus_merit = (bonus_merit * number_range(95, 105)) / 100; /* A little variance */
      /* Reduce for frequent dying - one hour to full bonus */
      if(current_time - ch->pcdata->last_death_date < 3600)
        bonus_merit = UMAX(1, bonus_merit * (current_time - ch->pcdata->last_death_date) / 3600);
      sprintf(buf, "You receive %d merit for your kill of %s!\n\r", bonus_merit, ch->name);
      send_to_char(buf, target);
      target->pcdata->clan_info->merit += bonus_merit;
      /* All clan types that get bonuses from a player kill gain primary merit from this */
      if(clan_kill_type(target, ch))
        target->pcdata->clan_info->primary_merit += bonus_merit;
    }
    ch->pcdata->corpse_timer = 3;
    corpse->value[4] = 3;
  }
  else
    corpse->value[4] = -3;

  if(ch->damaged)
  {
    DAMAGE_DATA *prev = NULL;
    top = NULL;
    track = ch->damaged;
    while(track)
    {
      if((track->temp_target = check_is_online(track->source)) == NULL
        || track->temp_target == ch)// Just in case self damage remains
      {/* Remove NPCs and logged off players from this list */
        if(prev)
        {
          prev->next = track->next;
          free_damage(track);
          track = prev->next;/* prev was already handled */
        }
        else
        {
          ch->damaged = track->next;
          free_damage(track);
          track = ch->damaged;
        }
        continue;
      }
      if(!top || top->damage < track->damage)
        top = track;
      if(track->temp_target->pcdata->clan_info)
        total += track->damage;
      prev = track;
      track = track->next;
    }
  }
  for(track = ch->damaged; track; track = track->next)
  {
    if(total_merit && track->temp_target->pcdata->clan_info)
    {
      int bonus = UMAX(1, total_merit * track->damage / total);
      sprintf(buf, "You take {Y%d merit{x from %s.\n\r", bonus, ch->name);
      send_to_char(buf, track->temp_target);
      track->temp_target->pcdata->clan_info->merit += bonus;
      if(clan_kill_type(track->temp_target, ch))
      {
        if(track->temp_target->pcdata->clan_info->clan->type == CLAN_TYPE_LAW)
          track->temp_target->pcdata->clan_info->secondary_merit += bonus;
        else
          track->temp_target->pcdata->clan_info->primary_merit += bonus;
      }
      merit_steal = TRUE;
    }
    track->damage = 3;/* Shared lootables */
    track->duration = 0; /* Unique lootables - damages don't expire normally on corpses */
    if(track == top)
      track->duration++;
    if(!killer_set && track->temp_target == killer)
    {
      killer_set = TRUE;
      track->duration++;
    }
    track->temp_target = NULL; /* Does not persist outside this function */
  }
  if(merit_steal)
  {
    ch->pcdata->clan_info->merit = 0;
    while(ch->pcdata->clan_info->delay_merit)
    {
      dmerit = ch->pcdata->clan_info->delay_merit->next;
      free_merit(ch->pcdata->clan_info->delay_merit);
      ch->pcdata->clan_info->delay_merit = dmerit;
    }
    send_to_char("Your unbanked merit has been taken by your killers.\n\r", ch);
  }
  /* Transfer over the now-loot-counting trackers */
  if(IS_NPC(killer) && (!killer->master || IS_NPC(killer->master)))
  {
    damage_add(NULL, ch, 4, 0);/* Mob kill, killer bonus goes in here */
  }
  else if(!killer_set)
  {/* Something is wrong, but we'll handle it by assigning one for the killer */
    bug("Killer damage not found for looting.", 0);
    if(ch->damaged)
      damage_add(killer, ch, 3, 1);/* Only 1 reserved, top damage already assigned */
    else
      damage_add(killer, ch, 3, 2);/* Killer gets all 5 */
  }
  corpse->loot_track = ch->damaged;
  ch->damaged = NULL;
}

/*
 * Make a corpse out of a character.
 */
void make_corpse( CHAR_DATA *ch, CHAR_DATA *killer )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *corpse = NULL;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    char *name;
    bool clan_kill = FALSE;
//below taken out when the 'take the gear from the corpse' code removed
    //int pc_item_in_count = 0;

  if (!IS_SET (ch->form,FORM_INSTANT_DECAY)) {

    if ( IS_NPC(ch) )
    {
  name            = ch->short_descr;
  corpse          = create_object(get_obj_index(OBJ_VNUM_CORPSE_NPC), 0, FALSE);
  corpse->value[3] = ch->pIndexData->vnum;
  if (ch->life_timer) {
    corpse->timer = ch->life_timer/2+1;
  } else {
    corpse->timer   = number_range( 3, 6 );  /* original */
  }

  if ( ch->gold > 0 )
  {
      obj_to_obj( create_money( ch->gold, ch->silver ), corpse );
      ch->gold = 0;
      ch->silver = 0;
  }
  corpse->cost = 0;
    }
    else
    {
  name    = ch->name;
  corpse    = create_object(get_obj_index(OBJ_VNUM_CORPSE_PC), 0, FALSE);
  corpse->timer = number_range( 25, 40 );
  corpse->value[4] = MAX_LOOT_ITEMS;
  REMOVE_BIT(ch->act,PLR_CANLOOT);
  if (!is_clan(ch))
      corpse->owner = str_dup(ch->name);
  else
  {
      corpse->owner = str_dup(ch->name);
      SET_BIT(corpse->extra_flags,ITEM_CLAN_CORPSE);
      if (ch->gold > 1 || ch->silver > 1)
      {
    obj_to_obj(create_money(ch->gold / 2, ch->silver/2), corpse);
    ch->gold -= ch->gold/2;
    ch->silver -= ch->silver/2;
      }
      clan_kill = TRUE;
  }

    /*
   if(IS_SET(ch->act,PLR_DWEEB))
     SET_BIT(corpse->extra_flags,ITEM_DARK);
     */

  corpse->cost = 0;
    }

    corpse->level = ch->level;

    sprintf( buf, corpse->short_descr, name );
    corpse->short_descr = str_dup( buf );

    sprintf( buf, corpse->description, name );
    corpse->description = str_dup( buf );

  }

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
  bool floating = FALSE;

  obj_next = obj->next_content;

  /* drop corpse from inside other corpse to avoid fast full loot */
  // Can't pick up clanner corpses any more
/*  if (IS_SET(obj->extra_flags,ITEM_CLAN_CORPSE))
  {
    obj_from_char(obj);
    obj_to_room(obj,ch->in_room);
    continue;
  }*/
  if (obj->wear_loc == WEAR_FLOAT)
      floating = TRUE;
  obj_from_char( obj );
  /*if (obj->item_type == ITEM_POTION)
      obj->timer = number_range(500,1000);
  if (obj->item_type == ITEM_SCROLL)
      obj->timer = number_range(1000,2500);*/
  if (IS_SET(obj->extra_flags,ITEM_ROT_DEATH) && !floating)
  {
      obj->timer = number_range(5,10);
      REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
  }
  REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);

  if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
      extract_obj( obj );
  else if (floating)
  {
      if (IS_OBJ_STAT(obj,ITEM_ROT_DEATH)) /* get rid of it! */
      {
    if (obj->contains != NULL)
    {
        OBJ_DATA *in, *in_next;

        act("$p evaporates,scattering its contents.",
      ch,obj,NULL,TO_ROOM,FALSE);
        for (in = obj->contains; in != NULL; in = in_next)
        {
      in_next = in->next_content;
      obj_from_obj(in);
      obj_to_room(in,ch->in_room);
        }
     }
     else
        act("$p evaporates.",
      ch,obj,NULL,TO_ROOM,FALSE);
     extract_obj(obj);
      }
      else
      {
        if(obj->link_name && !str_cmp(obj->link_name, ch->name))
        {
          obj->wear_loc = WEAR_HIDDEN;
          obj_to_char(obj, ch);
        }
        else
        {/* What? */
          act("$p falls to the floor.",ch,obj,NULL,TO_ROOM,FALSE);
          obj_to_room(obj,ch->in_room);
        }
      }
  }
  else
   if (!IS_SET (ch->form,FORM_INSTANT_DECAY))
      obj_to_obj( obj, corpse );
   else
      obj_to_room (obj, ch->in_room );
  }

   if (!IS_SET (ch->form,FORM_INSTANT_DECAY))
    obj_to_room( corpse, ch->in_room );
   else
    act("The corpse crumbles into dust.",ch,NULL,NULL,TO_ROOM,FALSE);
  if(clan_kill)
  {
    remove_all_objs(ch, FALSE);
    sort_clanner_items(ch, corpse, &corpse->loot_next, TRUE);
    assign_clanner_looting(ch, killer, corpse);
  }
}

void highlander_die( CHAR_DATA *ch, CHAR_DATA *victim )
{
  char buf[MAX_STRING_LENGTH];

   sprintf(buf,"%s kills %s in Highlander, %s before %d/%d/%d/%d/%d/%d with all=%d and real=%d",
           ch->name,victim->name,ch->name,ch->pcdata->perm_hit,
           ch->pcdata->perm_mana,ch->pcdata->perm_move,
           ch->max_hit,ch->max_mana,ch->max_move,
           ch->pcdata->highlander_data[ALL_KILLS],
           ch->pcdata->highlander_data[REAL_KILLS]);
   log_string(buf);
   /* Add Victims Kills to ALL_KILLS */
   ch->pcdata->highlander_data[ALL_KILLS] += victim->pcdata->highlander_data[ALL_KILLS];

   /* Add Victim to ALL_KILLS and REAL_KILLS */
   ch->pcdata->highlander_data[ALL_KILLS] += 1;
   ch->pcdata->highlander_data[REAL_KILLS] += 1;

   /* Add Stats for Victims ALL_KILLS +1 for Victim */
   ch->pcdata->perm_hit += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;
   ch->pcdata->perm_mana += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;
   ch->pcdata->perm_move += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;
   ch->max_hit += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;
   ch->max_mana += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;
   ch->max_move += (victim->pcdata->highlander_data[ALL_KILLS] +1) * 100;

   /* Zap the Winner with the Quickening */
   ch->hit /= 2;
   ch->mana /= 2;

   remove_highlander(ch,victim);
   sprintf(buf,"%s kills %s in Highlander, %s after %d/%d/%d/%d/%d/%d with all=%d and real=%d",
           ch->name,victim->name,ch->name,ch->pcdata->perm_hit,
           ch->pcdata->perm_mana,ch->pcdata->perm_move,
           ch->max_hit,ch->max_mana,ch->max_move,
           ch->pcdata->highlander_data[ALL_KILLS],
           ch->pcdata->highlander_data[REAL_KILLS]);
   log_string(buf);

   return;
}

/*
 * Improved Death_cry contributed by Diavolo.
 */
void death_cry( CHAR_DATA *ch )
{
    ROOM_INDEX_DATA *was_in_room;
    char *msg;
    int door;
    int vnum;

    vnum = 0;
    msg = "You hear $n's death cry.";

    switch ( number_bits(4))
    {
    case  0: msg  = "$n hits the ground ... DEAD.";     break;
    case  1:
    if (ch->material == 0)
    {
      msg  = "$n splatters blood on your armor.";
      break;
    }
    case  2:
  if (IS_SET(ch->parts,PART_GUTS))
  {
      msg = "$n spills $s guts all over the floor.";
      vnum = OBJ_VNUM_GUTS;
  }
  break;
    case  3:
  if (IS_SET(ch->parts,PART_HEAD))
  {
      msg  = "$n's severed head plops on the ground.";
      vnum = OBJ_VNUM_SEVERED_HEAD;
  }
  break;
    case  4:
  if (IS_SET(ch->parts,PART_HEART))
  {
      msg  = "$n's heart is torn from $s chest.";
      vnum = OBJ_VNUM_TORN_HEART;
  }
  break;
    case  5:
  if (IS_SET(ch->parts,PART_ARMS))
  {
      msg  = "$n's arm is sliced from $s dead body.";
      vnum = OBJ_VNUM_SLICED_ARM;
  }
  break;
    case  6:
  if (IS_SET(ch->parts,PART_LEGS))
  {
      msg  = "$n's leg is sliced from $s dead body.";
      vnum = OBJ_VNUM_SLICED_LEG;
  }
  break;
    case 7:
  if (IS_SET(ch->parts,PART_BRAINS))
  {
      msg = "$n's head is shattered, and $s brains splash all over you.";
      vnum = OBJ_VNUM_BRAINS;
  }
    }

    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
      msg  = "$n's severed head plops on the ground.";
      vnum = OBJ_VNUM_SEVERED_HEAD;
    }

    act( msg, ch, NULL, NULL, TO_ROOM ,FALSE);

    if ( vnum != 0 )
    {
  char buf[MAX_STRING_LENGTH];
  OBJ_DATA *obj;
  char *name;

  name    = IS_NPC(ch) ? ch->short_descr : ch->name;
  obj   = create_object( get_obj_index( vnum ), 0, FALSE );
  obj->timer  = number_range( 4, 7 );

  sprintf( buf, obj->short_descr, name );
  /*free_string( obj->short_descr );*/
  obj->short_descr = str_dup( buf );

  sprintf( buf, obj->description, name );
  /*free_string( obj->description );*/
  obj->description = str_dup( buf );

  if (obj->item_type == ITEM_FOOD)
  {
      if (IS_SET(ch->form,FORM_POISON))
    obj->value[3] = 1;
      else if (!IS_SET(ch->form,FORM_EDIBLE))
    obj->item_type = ITEM_TRASH;
  }

   /* The vnum i s aved on the item */
  obj->value[2] = (IS_NPC(ch) ? ch->pIndexData->vnum : 0 );

  obj_to_room( obj, ch->in_room );
    }

    if ( IS_NPC(ch) )
  msg = "You hear something's death cry.";
    else
  msg = "You hear someone's death cry.";

    was_in_room = ch->in_room;
    for ( door = 0; door <= 5; door++ )
    {
  EXIT_DATA *pexit;

  if ( ( pexit = was_in_room->exit[door] ) != NULL
  &&   pexit->u1.to_room != NULL
  &&   pexit->u1.to_room != was_in_room )
  {
      ch->in_room = pexit->u1.to_room;
      act( msg, ch, NULL, NULL, TO_ROOM ,FALSE);
  }
    }

    ch->in_room = was_in_room;

    return;
}



void raw_kill( CHAR_DATA *victim, CHAR_DATA *ch )
{
    CHAR_DATA *gch,*gch_next;
    char buf[MAX_STRING_LENGTH];
    int i;

    stop_fighting( victim, TRUE );
    death_cry( victim );

    if ( is_affected(victim,gsn_wound_transfer) )
    for ( gch = char_list ;
          gch != NULL ;
          gch = gch_next )
   {
        gch_next = gch->next;

        if ( gch->leader == victim )
        {
  sprintf(buf, "Your soul shatters with your link to %s.",victim->name);
  send_to_char(buf,gch);
  /* one more to avoid recursive crap */
  if ( is_affected( gch, gsn_wound_transfer ) ) /*strip it */
      affect_strip( gch, gsn_wound_transfer );
  raw_kill(gch,ch);
        }
    }

    if ( IS_NPC(victim) )
    {
  make_corpse (victim, ch);
  victim->pIndexData->killed++;
  kill_table[URANGE(0, victim->level, MAX_LEVEL-1)].killed++;
  extract_char( victim, TRUE );
  return;
    }

  if (!IS_SET(victim->affected_by,AFF_WITHSTAND_DEATH))
  {
        if(victim->pcdata->deity_trial_timer > 0)
        {
        victim->pcdata->deity_trial_timer = 0;
        sprintf(buf, "You have failed your trial from %s.\n\r", deity_table[victim->pcdata->deity].pname);
        send_to_char(buf, victim);
        log_deity_favor(victim, ch, DEITY_TRIAL_FAIL_DEATH);
        }

     if (IS_SET (victim->in_room->room_flags,ROOM_NODIE))
//         || (ch->clan == clan_lookup("smurf")
//             || victim->clan == clan_lookup("smurf")))
     {
        act("$n disintegrates into dust.",victim,NULL,NULL,TO_ROOM,FALSE);
        char_from_room (victim);
        clear_mount( victim );
        if(IS_SET(victim->mhs,MHS_GLADIATOR) &&
           (gladiator_info.type == 2 || gladiator_info.type == 3))
        {
           if(victim->pcdata->gladiator_team == 1)
              char_to_room(victim,get_room_index(ROOM_VNUM_TEAM_GLADIATOR));
           else
              char_to_room(victim,get_room_index(ROOM_VNUM_TEAM_BARBARIAN));
        }
        else
        {
           if (IS_SET(victim->act, PLR_DWEEB))
              char_to_room(victim,get_room_index(ROOM_VNUM_ALTAR));
           else
           {
              if(victim->pcdata && victim->pcdata->clan_info && victim->pcdata->clan_info->clan->hall)
                char_to_room(victim, (ROOM_INDEX_DATA*)victim->pcdata->clan_info->clan->hall->to_place);
              else
                char_to_room(victim,get_room_index(clan_table[victim->clan].hall));
           }
        }
     }
     else
     {
        if (IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
        {
           highlander_die( ch,victim );
           char_from_room(victim);
           clear_mount( victim );
           if (!is_clan(victim))
              char_to_room(victim,get_room_index(clan_table[0].hall));
           else
              char_to_room(victim,get_room_index(clan_table[victim->clan].hall));
           send_to_char( "You are blasted with the {GQuickening{x!\n\r", ch);
           act("$n is blasted by the {GQuickening{x.",ch,NULL,NULL,TO_ROOM,FALSE);
        }
        else
        {
           make_corpse( victim, ch );
           extract_char( victim, FALSE );
        }
     }
     while ( victim->flash_affected )
        flash_affect_remove( victim, victim->flash_affected,APPLY_BOTH );
     while ( victim->affected )
        affect_remove( victim, victim->affected,APPLY_BOTH );

    victim->alert = 0;
    if(victim->pcdata && victim->pcdata->clan_info)
      victim->pcdata->clan_info->lost_merit = 0;

    while(victim->damaged)
      damage_remove(victim, victim->damaged);

     /* victim->affected_by = victim->affected_by|race_table[victim->race].aff;*/
     victim->affected_by = race_table[victim->race].aff;

     if (IS_SET(ch->mhs, MHS_BANISH))
        REMOVE_BIT(ch->mhs, MHS_BANISH);
     if(!IS_SET(ch->mhs, MHS_GLADIATOR))
     {// Gladiators don't actually die, don't set their ac
        for (i = 0; i < 4; i++)
           victim->armor[i]= 100;
     }

     victim->position    = POS_RESTING;
     victim->hit         = UMAX( 1, victim->hit  );
     victim->mana        = UMIN( 20, victim->mana );
     victim->move        = UMAX( 1, victim->move );

     /*
     REMOVE_BIT(victim->act,PLR_KILLER);
     REMOVE_BIT(victim->act,PLR_THIEF);
     REMOVE_BIT(victim->act,PLR_BOUGHT_PET);
     save_char_obj( victim ); */

     return;
  }
  else
  {

     victim->position    = POS_STANDING;
     while ( victim->flash_affected )
        flash_affect_remove( victim, victim->flash_affected,APPLY_BOTH );
     while ( victim->affected )
        affect_remove( victim, victim->affected,APPLY_BOTH );

    victim->alert = 0;
    if(victim->pcdata && victim->pcdata->clan_info)
      victim->pcdata->clan_info->lost_merit = 0;

     victim->affected_by = victim->affected_by|race_table[victim->race].aff;
     victim->hit         = victim->max_hit/8;
//COREY WITHSTAND FIX HERE
     stop_fighting(victim, TRUE);
     act("$n twitches a bit then stands up.",victim,NULL,NULL,TO_ROOM,FALSE);
     send_to_char ("A chilling wave passes over as you withstand death.\n\r",victim);
     WAIT_STATE(victim, 4);
    /* Gladiator Spectator Channel */
    if (IS_SET(victim->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s withstood a mighty blow from %s! He's still alive!",victim->name,ch->name);
       gladiator_talk(buf);
    }
        if(victim->pcdata->deity_trial_timer > 0)
        {
        sprintf(buf, "%s allows you to continue in your trial.\n\r", deity_table[victim->pcdata->deity].pname);
        send_to_char(buf, victim);
        }
  }
}



void group_gain( CHAR_DATA *ch, CHAR_DATA *victim )
{
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *gch;
    CHAR_DATA *lch;
    int xp;
    int members;
    int group_levels;
    int first_charmy = 0;

    /*
     * Monsters don't get kill xp's or alignment changes.
     * P-killing doesn't help either.
     * Dying of mortal wounds or poison doesn't give xp to anyone!
     */
    if ( victim == ch ||
        (IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_MIRROR_IMAGE )
       )
  return;

    members = 0;
    group_levels = 0;
    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  if ( is_same_group( gch, ch ) )
       {
      members++;
      if (!IS_NPC(gch))
         {
          group_levels += gch->level-(gch->level/5);
          if(class_table[gch->class].fMana != 2)
          {
            if(first_charmy > 0)
            {
              group_levels += first_charmy;
            }
            first_charmy = -1;// Non-caster player ally was found
          }
         }
      else
         {
          if(gch->master && gch->master->pet == gch)
                continue;// Pets no longer steal xp
          if(!first_charmy && gch->master &&
            class_table[gch->master->class].fMana == 2
            && gch->level + 2 <= gch->master->level)
            {// Necro skellies can get above level sometimes
              first_charmy = gch->level;
              continue;
            }
            if(first_charmy > 0)
            {// Check if this one is higher level, if so store the higher one
              if(first_charmy < gch->level)
              {
                group_levels += first_charmy;
                first_charmy = gch->level;
                continue;
              }
            }
            group_levels += gch->level;
          /*if (gch->level >= 6)
           {
            group_levels += (gch->level *3)/2;
           }
          else
           {
            group_levels += 6;
           }*/
         }
       }
    }

    if ( members == 0 )
    {
  bug( "Group_gain: members.", members );
  members = 1;
  group_levels = ch->level ;
    }

    lch = (ch->leader != NULL) ? ch->leader : ch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;

  if ( !is_same_group( gch, ch ) || IS_NPC(gch))
      continue;

  xp = xp_compute( gch, victim, group_levels );

  for ( obj = gch->carrying; obj != NULL; obj = obj_next )
  {
      obj_next = obj->next_content;
      if ( obj->wear_loc == WEAR_NONE )
    continue;

      if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(gch)    )
      ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(gch)    )
      ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(gch) ) )
      {
    act( "You are zapped by $p.", gch, obj, NULL, TO_CHAR ,FALSE);
    act( "$n is zapped by $p.",   gch, obj, NULL, TO_ROOM ,FALSE);
    obj_from_char( obj );
    obj_to_char(obj, gch);
//    obj_to_room( obj, gch->in_room );
      }
  }
    }

    return;
}



/*
 * Compute xp for a kill.
 * Also adjust alignment of killer.
 * Edit this function to change xp computations.
 */
int xp_compute( CHAR_DATA *gch, CHAR_DATA *victim, int total_levels )
{
    int xp,base_exp;
    int align,level_range;
    int change;
    AFFECT_DATA af;
    bool double_bonus = FALSE;
    int tribute = 0;
    char buf[256];

    if(total_levels < gch->level)
        total_levels = gch->level;// Don't give a bonus just for being ungrouped

  if ( IS_NPC(victim) && victim->spec_fun != 0 )
  {
    if ( is_clan_guard(victim) == TRUE )
    {
       return 0;
    }
  }

    level_range = victim->level - gch->level;
    /* compute the base exp */
    switch (level_range)
    {
  default :   base_exp =   0;   break;
  case -8 : base_exp =   2;   break;
  case -7 : base_exp =   7;   break;
  case -6 : base_exp =   13;   break;
  case -5 : base_exp =   20;   break;
  case -4 : base_exp =  26;   break;
  case -3 : base_exp =  40;   break;
  case -2 : base_exp =  60;   break;
  case -1 : base_exp =  80;   break;
  case  0 : base_exp =  100;   break;
  case  1 : base_exp =  140;   break;
  case  2 : base_exp =  180;   break;
  case  3 : base_exp = 220;   break;
  case  4 : base_exp = 280;   break;
  case  5 : base_exp = 320;   break;
    }

    if (level_range > 5)
  base_exp = 320 + 30 * (level_range - 5);

    if (!IS_NPC(victim) && IS_SET(victim->act,PLR_DWEEB))
  base_exp = 0;

    /* do alignment computations */

    align = victim->alignment - gch->alignment;

    /*Gladiator's alignment doesnt change */
    if (IS_SET(victim->act,ACT_NOALIGN) || IS_SET(gch->mhs,MHS_GLADIATOR)
    || is_affected(gch, skill_lookup("indulgence")) )
    {
  /* no change */
    }

    else if (align > 500) /* monster is more good than slayer */
    {
  change = (align - 500) * base_exp / 500 * gch->level/total_levels;
  change = UMAX(1,change);
        gch->alignment = UMAX(-1000,gch->alignment - change);
    }

    else if (align < -500) /* monster is more evil than slayer */
    {
  change =  ( -1 * align - 500) * base_exp/500 * gch->level/total_levels;
  change = UMAX(1,change);
  gch->alignment = UMIN(1000,gch->alignment + change);
    }

    else /* improve this someday */
    {
  change =  gch->alignment * base_exp/500 * gch->level/total_levels;
  gch->alignment -= change;
    }

    /* calculate exp multiplier */
    if (IS_SET(victim->act,ACT_NOALIGN))
  xp = base_exp;

    else if (gch->alignment > 500)  /* for goodie two shoes */
    {
  if (victim->alignment < -750)
      xp = (base_exp *4)/3;

    else if (victim->alignment < -500)
      xp = (base_exp * 5)/4;

    else if (victim->alignment > 750)
      xp = base_exp / 3;

    else if (victim->alignment > 500)
      xp = base_exp / 2;

    else if (victim->alignment > 250)
      xp = (base_exp * 3)/4;


  else
      xp = base_exp;
    }

    else if (gch->alignment < -500) /* for baddies */
    {
  if (victim->alignment > 750)
      xp = (base_exp * 5)/4;

    else if (victim->alignment > 500)
      xp = (base_exp * 11)/10;

    else if (victim->alignment < -750)
      xp = base_exp/2;

  else if (victim->alignment < -500)
      xp = (base_exp * 3)/4;

  else if (victim->alignment < -250)
      xp = (base_exp * 9)/10;

  else
      xp = base_exp;
    }

    else if (gch->alignment > 200)  /* a little good */
    {

  if (victim->alignment < -500)
      xp = (base_exp * 6)/5;

  else if (victim->alignment > 750)
      xp = base_exp/2;

  else if (victim->alignment > 0)
      xp = (base_exp * 3)/4;

  else
      xp = base_exp;
    }

    else if (gch->alignment < -200) /* a little bad */
    {
  if (victim->alignment > 500)
      xp = (base_exp * 6)/5;

  else if (victim->alignment < -750)
      xp = base_exp/2;

  else if (victim->alignment < 0)
      xp = (base_exp * 3)/4;

  else
      xp = base_exp;
    }

    else /* neutral */
    {

  if (victim->alignment > 500 || victim->alignment < -500)
      xp = (base_exp * 4)/3;

  else if (victim->alignment < 200 && victim->alignment > -200)
      xp = base_exp/2;

  else
      xp = base_exp;
    }

    /* more exp at the low levels */
    if (gch->level < 11)
      xp = 15 * xp / (gch->level + 4);

    /* less at high */

    if (gch->level > 40 )
  xp =  40 * xp / (gch->level -1);


    /* reduce for playing time -
       Removed */

    {
  /**** compute quarter-hours per level
  time_per_level = 4 *
       (gch->played + (int) (current_time - gch->logon))/3600
       / gch->level;

  time_per_level = URANGE(2,time_per_level,12);
  if (gch->level < 15)
      time_per_level = UMAX(time_per_level,(15 - gch->level));
  xp = xp * time_per_level / 12;
 *****/
  }

    /* randomize the rewards */
    xp = number_range (xp * 4/5, xp * 6/5);

    /* adjust for grouping */
    if ( total_levels - 4 > gch->level)
    xp = xp * gch->level/( UMAX(1,total_levels -4) );

    /* Adjust for WIS */
    xp = ( ( 100 + get_curr_stat(gch,STAT_WIS) ) * xp ) / 100;

//     if ( gch->clan == clan_lookup("outcast") && (gch->pcdata->outcT > 0) )
    if(gch->pcdata->clan_info && !str_cmp(gch->pcdata->clan_info->clan->name, "outcast"))
     xp /= 2;

    xp = 100 * xp / 100; /* Gradually step this down until it's about 75 */

//making clanners who kill other clanners get a boost
    if( is_clan(gch) && (!IS_NPC(victim) && is_clan(victim)) )
    {
      xp = 1.25 * xp;
    }
    else if(is_clan(gch) && gch->level <= 20)/* Slight bonus xp for clanning early */
      xp = (xp * 100) / (80 + (gch->level - 5));


/* no exp for killing the same person twice */
    if (!IS_NPC(victim))
    {
       if (!str_cmp(gch->pcdata->last_kill,victim->name))
          xp = 0;
    }
    else if(victim->pIndexData->vnum == MOB_VNUM_SKEL_WAR ||
      victim->pIndexData->vnum == MOB_VNUM_CORPSE ||
      victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    xp = 0;


    /* double EXP day for the 25th of each month
    --- Dont forget to change 'override' to TRUE in act_wiz.c
*/
    if (xp > 0)
    {// Deity triggers and double xp
     /* Tribute triggers if you earn xp */
      if(gch->pcdata->clan_info && IS_NPC(victim))
      {/* Clan tribute is handled elsewhere, as it is not tied to xp earned */
        tribute = (victim->level * victim->level) / 20;
        tribute = (tribute * number_range(95, 105)) / 100; /* A little variance */

//tribute *= 1000; /* DEBUG RATE */

        if(total_levels > gch->level)
          tribute = UMAX(1, (tribute * gch->level) / total_levels);
        else
          tribute = UMAX(1, tribute);
        if(gch->pcdata->clan_info->clan->initiation > 0)
        {
          gch->pcdata->clan_info->clan->initiation -= tribute;
          if(gch->pcdata->clan_info->clan->initiation <= 0)
          {
            gch->pcdata->clan_info->clan->initiation = 0;
            /* Done initiation, announce it to everyone on from the clan */
            notify_clan("Your clan has completed its initiation successfully!\n\r", gch->pcdata->clan_info->clan);
            save_clan(gch, TRUE, FALSE, TRUE);
          }
        }
        else
        {
          switch(gch->pcdata->clan_info->clan->type)
          {/* Handle NPC based merit bonuses */
            case CLAN_TYPE_PEACE: gch->pcdata->clan_info->primary_merit += tribute; break;
            case CLAN_TYPE_LAW: if(clanner_count <= 5)/* 5% */
              gch->pcdata->clan_info->secondary_merit += (tribute + 1) / 2; break;
            case CLAN_TYPE_MALICE: if(clanner_count <= 5)/* 5% */
              gch->pcdata->clan_info->primary_merit += (tribute + 1) / 2; break;
            case CLAN_TYPE_FAITH:
            case CLAN_TYPE_GREED: gch->pcdata->clan_info->primary_merit += (tribute + 1) / 2; break;
          }
          gch->pcdata->clan_info->merit += tribute;
        }
      }
     if(override && gch->pcdata->deity_trial_timer <= 0)
     {
        double_bonus = TRUE;
       switch( number_percent() )
       {
       case 1:
           do_spreward(gch,"all 1");
           break;
       case 2:
            act("You feel a rejuvenating rush.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            gch->pcdata->skill_point_timer = 0;
            gch->pcdata->skill_point_tracker = 0;
            break;
       case 3:
            act("You have been rewarded by the gods instead.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            gch->skill_points += 5;
            gch->pcdata->skill_point_tracker += 5;
            break;
       case 4:
            act("Boogums gives you a great big hulking Bear Hug!",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 3;
            break;
       case 5:
            act("This space for rent, pester Slodhian alot for details.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            gch->skill_points += 10;
            xp *= 2;
            break;
       case 6:
            act("A shard has fallen out of the sky and bonks you on the head.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            obj_to_char(create_object(get_obj_index(OBJ_VNUM_SHARD),0,FALSE),gch);
            break;
       case 7:
            act("Kuno, the Blue Wombat, has given you the power to heal all.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            do_restore(gch,"all");
            break;
       case 9:
            act("Alagaster asks 'HOW can you have any pudding if you don't eat your meat???",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 2;
            break;
       case 10:
            act("KD says Quit Killing the Diploma Beast...Damn it.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 2;
            break;
       case 17:
            act("Matook smiles down upon you!",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 3;
            break;
       case 18:
            act("Rusty hits you with his Clue Sitck.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 2;
            break;
       case 19:
            act("KallaLilly plays her fiddle to a crowd of turtles...",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 2;
            break;
       case 21:
            act("*Squish* goes the Communion bug and the crowd cheers!",
                gch,NULL,NULL,TO_CHAR,FALSE);
            xp *= 3;
            gch->gold += 100;
            break;
       case 23:
            xp *= 2;
            if ( (gch->pcdata->debit_level > 0) ||
                (gch->exp > exp_per_level(gch,gch->pcdata->points) * (gch->level + 1)))
            break;// Qualified before this, don't reward xp

            if( gch->exp + xp > (exp_per_level(gch,gch->pcdata->points) * (gch->level+1)) && gch->level < 50 )
            {
               act("Nightdagger thinks you deserve an extra level!",
                   gch,NULL,NULL,TO_CHAR,FALSE);
               gch->exp += exp_per_level(gch,gch->pcdata->points);
            }
            break;
       case 24:
            xp *= 4;
            act("You are a beautiful person.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            break;
       case 25:
            xp *= 2;
            act("Buy scrolls from Enchantrem.",
                gch,NULL,NULL,TO_CHAR,FALSE);
            break;
       case 26:
            xp *= 2;
            act("Ravenclaw says, 'I wasn't cheating!'.",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            break;
       case 30:
            xp *= 3;
            act("Fjornir is awesome.",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            break;
       case 31:
            xp *= 5;
            act("You heard we're going to yank all the clans right?",
                gch,NULL,NULL,TO_CHAR,FALSE);
            break;
       case 32:
            xp *= 2;
            act("I suppose you want me to say something witty... just take the double exp.",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            break;
       case 33:
            xp *= 3;
            act("BriarRose really likes color restrings..",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            break;
       case 35:
            xp *= 3;
            act("*sigh* I've run out of witty things to say.",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            break;
       case 36:
            xp *= 2;
            act("Have a drink on me.",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            obj_to_char(create_object(get_obj_index(4822),0,FALSE),gch);
            break;
       case 37:
            xp *= 2;
            affect_strip(gch,gsn_sanctuary);
            REMOVE_BIT(gch->affected_by,AFF_SANCTUARY);
            af.where     = TO_AFFECTS;
            af.type      = gsn_sanctuary;
            af.level     = 60;
            af.duration  = 20;
            af.location  = APPLY_NONE;
            af.modifier  = 0;
            af.bitvector = AFF_SANCTUARY;
            affect_to_char( gch, &af );
            act("Nightdagger slams the door and says 'Another one.'",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            act( "$n is surrounded by a white aura.", gch, NULL, NULL, TO_ROOM ,FALSE);
            send_to_char( "You are surrounded by a white aura.\n\r", gch );
            break;
       case 38:
            xp *= 2;
            affect_strip(gch,skill_lookup("haste"));
            REMOVE_BIT(gch->affected_by, AFF_HASTE);
            af.where     = TO_AFFECTS;
            af.type      = skill_lookup("haste");
            af.level     = 1;
            af.duration  = 30;
            af.location  = APPLY_DEX;
            af.modifier  = 5;
            af.bitvector = AFF_HASTE;
            affect_to_char( gch, &af );
            act("BriarRose asks, 'Do you have any french fried faerie wings?'",
                 gch, NULL, NULL, TO_CHAR, FALSE);
            send_to_char( "You feel yourself moving more quickly.\n\r", gch );
            act("$n is moving more quickly.",gch,NULL,NULL,TO_ROOM,FALSE);
            break;
       default:
            double_bonus = FALSE;
            xp *= 2;
            break;
       }
     }// End double XP loop
      if(IS_NPC(victim))
      {// Don't activate this at all if target is a player
              if(gch->pcdata->deity_favor_timer > 0)
              {
                            if ( !is_aligned(gch))
                            {
                                do_deity_msg("%s does not favor your alignment.", gch);
                            }
                            else if ( !is_clan(gch) && deity_table[gch->pcdata->deity].clan)
                            {
                                do_deity_msg("%s does not favor those who are not in a clan.", gch);
                            }
                            else if(gch->pcdata->deity_trial_timer > 0)
                {// See if this kill satisfies the trial requirement - deity_trial_kill is in deity.c
                        if(override)
                                xp *= 2;// The override was skipped so this adds double xp in if it's running
                        xp += deity_trial_kill(gch, victim, override ? xp / 2 : xp);
                }
                            else
                            {
                        int range = 21;
                        int check = number_percent();
#ifdef DEITY_TRIAL_DEBUG_CODE
                                                if(deity_value_override > 0 && deity_value_override <= 100)
                                                        check = deity_value_override - 1;// Just drop it straight in instead of the %
#endif
                        switch(gch->pcdata->deity_favor)
                        {
                                case 0: range = 21; break;
                                case 1: range = 27; break;
                                case 2: range = 33; break;
                        }
                        if(check < range && !override)
                        {// Non-message takes the bottom 20-30% chances, don't occur at all if double xp is on
                                /* XP amount is based on same value as chance to occur */
                                int bonus = number_range(range, range * 2);
#ifdef DEITY_TRIAL_DEBUG_CODE
                                                        int old_xp = xp;
#endif
                                xp += (xp * bonus) / 100;
                                do_deity_msg("%s enhances your experience.", gch);
#ifdef DEITY_TRIAL_DEBUG_CODE
                                                        sprintf(buf, "Old xp: %d New xp: %d Boost: %f%%\n\r", old_xp, xp, (xp / (float)old_xp) * 100.0f);
                                                        send_to_char(buf, gch);
#endif
                        }
                        else if(double_bonus == FALSE && check >= 100 - range)
                        {// Messages take the top 20-30% chances, don't overlap with a double xp message
                                if(check >= 100 - range / 3)
                                {// Top third is group bonus xp
                                        if(gch->level != total_levels)
                                        {// There's a group in the room.  Nothing happens if there is none.
#ifdef DEITY_TRIAL_DEBUG_CODE
                                        sprintf(buf, "%d your level, %d group level.\n\r", gch->level, total_levels);
send_to_char(buf, gch);
#endif
                                        /* Undo the group xp reduction previously done */
                                                xp = xp * total_levels / gch->level;
                                                range = number_range(1, 3);
                                                switch(range)
                                                {
                                                        case 1: do_deity_msg("%s approves of you working with allies.", gch); break;
                                                        case 2: do_deity_msg("You please %s with your cooperation with allies.", gch); break;
                                                        case 3: do_deity_msg("%s is in favor of your fighting alongside allies.", gch); break;
                                                }
                                        }
                                }
                                else
                                      {// deity_favor_message is in deity.c
                                        xp += deity_favor_message(gch, victim, override ? xp / 2 : xp);
                                      }
                                    }
                                  }
                                }
                                else
                                {
          OBJ_DATA *focus = get_eq_char(gch, WEAR_FLOAT);
          if(focus != NULL && (focus->pIndexData->vnum == OBJ_VNUM_LESSERFAVOR ||
                focus->pIndexData->vnum == OBJ_VNUM_NORMALFAVOR
                 || focus->pIndexData->vnum == OBJ_VNUM_GREATERFAVOR
                || focus->pIndexData->vnum == OBJ_VNUM_TINYFAVOR))
                {// It is one of the deity focuses, destroy it and set appropriately
                                            if ( !is_aligned(gch))
                                            {
                                                do_deity_msg("%s does not favor those of your alignment.", gch);
                                            }
                                            else if ( !is_clan(gch) && deity_table[gch->pcdata->deity].clan)
                                            {
                                                do_deity_msg("%s does not favor those who are not clanners.",  gch);
                                            }
                                            else
                                            {
                                sprintf(buf, "$p flares {Ybrilliantly{x and vanishes!");
                        act(buf,gch,focus,NULL,TO_CHAR,FALSE);
                        act("$n's $p flares {Ybrilliantly{x and vanishes!",gch,focus,NULL,TO_ROOM,FALSE);
                        do_deity_msg("You feel %s's gaze turn to you.", gch);
                        gch->pcdata->deity_favor = 0;// Set just in case
                        gch->pcdata->deity_favor_timer = 90;
                        switch(focus->pIndexData->vnum)
                        {
                                case OBJ_VNUM_LESSERFAVOR: gch->pcdata->deity_favor = 0; break;
                                case OBJ_VNUM_NORMALFAVOR: gch->pcdata->deity_favor = 1; break;
                                case OBJ_VNUM_GREATERFAVOR: gch->pcdata->deity_favor = 2; break;
                                case OBJ_VNUM_TINYFAVOR: gch->pcdata->deity_favor = 3;
                                  gch->pcdata->deity_favor_timer /= 2;
                                  break;// Tiny favor doesn't last as long
                        }
                        log_deity_favor(gch, NULL, DEITY_FAVOR_ACTIVATE);
                                              extract_obj(focus);
                                            }
                                        }
                                }
                        }
    }

    if ( gch->race == race_lookup("gargoyle") && weather_info.sunlight >= SUN_RISE
        && weather_info.sunlight <= SUN_SET )
    {   /* up to a 25% bonus */
        xp = (100+(gch->level/2)) * xp / 100;
    }

    if ( is_affected(gch,gsn_spirit_of_owl) )
        xp = xp * (100+get_curr_stat(gch,STAT_WIS)) / 100;

    /* check if they've got debit levels */
    if ( (gch->pcdata->debit_level > 0) ||
        (gch->exp > exp_per_level(gch,gch->pcdata->points) * (gch->level + 1)))
    {
      int orig_xp = xp;
      if(gch->pcdata->debit_level <= 0)
      {
        xp = UMAX(0, xp - (gch->exp - exp_per_level(gch,gch->pcdata->points) * (gch->level + 1)));
      }
      else
        xp = 0;
  /* Tribute is added in earlier when it's calculated */
    if(tribute > 0)
    {
      if(gch->pcdata->clan_info->clan->initiation)
        sprintf( buf, "You receive {G%d{x (%d) experience above level and {W%d{xmerit (%d initiation).\n\r", xp, orig_xp, tribute, gch->pcdata->clan_info->clan->initiation );
      else
        sprintf( buf, "You receive {G%d{x (%d) experience points above level and {W%d{x merit.\n\r", xp, orig_xp, tribute );
    }
    else
      sprintf( buf, "You receive {G%d{x (%d) experience points above level.\n\r", xp, orig_xp );
  }
  else
  {
    if(tribute > 0)
    {
      if(gch->pcdata->clan_info->clan->initiation)
        sprintf( buf, "You receive {G%d{x experience points and {W%d{x merit (%d initiation remains)\n\r", xp, tribute, gch->pcdata->clan_info->clan->initiation );
      else
        sprintf( buf, "You receive {G%d{x experience points and {W%d{x merit.\n\r", xp, tribute );
    }
    else
      sprintf( buf, "You receive {G%d{x experience points.\n\r", xp );
  }
  send_to_char( buf, gch );
  gain_exp( gch, xp );

    return xp;// Taking the xp reduction out because I removed the bonus for ungrouped xp

}


void dam_message( CHAR_DATA *ch, CHAR_DATA *victim,int dam,int dt,bool immune )
{
    char buf1[256], buf2[256], buf3[256];
    const char *vs;
    const char *vp;
    const char *attack;
    char punct;

    if (ch == NULL || victim == NULL)
  return;

   if ( dam ==   0 ) { vs = "{ymiss{x"; vp = "{ymisses{x";    }
    else if ( dam <=   4 ) { vs = "{cscratch{x";  vp = "{cscratches{x"; }
    else if ( dam <=   8 ) { vs = "{cgraze{x";  vp = "{cgrazes{x";    }
    else if ( dam <=  12 ) { vs = "{chit{x";  vp = "{chits{x";    }
    else if ( dam <=  16 ) { vs = "{cinjure{x"; vp = "{cinjures{x";   }
    else if ( dam <=  20 ) { vs = "{cwound{x";  vp = "{cwounds{x";    }
    else if ( dam <=  24 ) { vs = "{cmaul{x";       vp = "{cmauls{x";   }
    else if ( dam <=  28 ) { vs = "{cdecimate{x"; vp = "{cdecimates{x"; }
    else if ( dam <=  32 ) { vs = "{cdevastate{x";  vp = "{cdevastates{x";  }
    else if ( dam <=  36 ) { vs = "{cmaim{x"; vp = "{cmaims{x";   }
    else if ( dam <=  40 ) { vs = "{cMUTILATE{x"; vp = "{cMUTILATES{x"; }
    else if ( dam <=  44 ) { vs = "{cDISEMBOWEL{x"; vp = "{cDISEMBOWELS{x"; }
    else if ( dam <=  48 ) { vs = "{cDISMEMBER{x";  vp = "{cDISMEMBERS{x";  }
    else if ( dam <=  52 ) { vs = "{cMASSACRE{x"; vp = "{cMASSACRES{x"; }
    else if ( dam <=  56 ) { vs = "{cMANGLE{x"; vp = "{cMANGLES{x";   }
    else if ( dam <=  60 ) { vs = "{R*** {cDEMOLISH {R***{x";
           vp = "{R*** {cDEMOLISHES {R***{x";     }
    else if ( dam <=  75 ) { vs = "{R*** {cDEVASTATE {R***{x";
           vp = "{R*** {cDEVASTATES {R***{x";     }
    else if ( dam <= 100)  { vs = "{R=== {COBLITERATE {R==={x";
           vp = "{R=== {COBLITERATES {R==={x";    }
    else if ( dam <= 125)  { vs = "{R>>> {CANNIHILATE {R<<<{x";
           vp = "{R>>> {CANNIHILATES {R<<<{x";    }
    else if ( dam <= 150)  { vs = "{R<<< {CERADICATE {R>>>{x";
           vp = "{R<<< {CERADICATES {R>>>{x";     }
    else                   { vs = "do {RUNSPEAKABLE{x things to";
           vp = "does {RUNSPEAKABLE{x things to";   }

    punct   = (dam <= 24) ? '.' : '!';

    if ( dt == TYPE_HIT )
    {
  if (ch  == victim)
  {
      sprintf( buf1, "$n %s $melf%c",vp,punct);
      sprintf( buf2, "You %s yourself%c",vs,punct);
  }
  else
  {
      sprintf( buf1, "$n %s $N%c",  vp, punct );
      sprintf( buf2, "You %s $N%c", vs, punct );
      sprintf( buf3, "$n %s you%c", vp, punct );
  }
    }
    else
    {
  if ( dt >= 0 && dt < MAX_SKILL )
      attack  = skill_table[dt].noun_damage;
  else if ( dt >= TYPE_HIT
  && dt <= TYPE_HIT + MAX_DAMAGE_MESSAGE)
      attack  = attack_table[dt - TYPE_HIT].name;
  else
  {
      bug( "Dam_message: bad dt %d.", dt );
      dt  = TYPE_HIT;
      attack  = attack_table[0].name;
  }

  if (immune)
  {
      if (ch == victim)
      {
    sprintf(buf1,"$n is unaffected by $s own %s.",attack);
    sprintf(buf2,"Luckily, you are immune to that.");
      }
      else
      {
        sprintf(buf1,"$N is unaffected by $n's %s!",attack);
        sprintf(buf2,"$N is unaffected by your %s!",attack);
        sprintf(buf3,"$n's %s is powerless against you.",attack);
      }
  }
  else
  {
      if (ch == victim)
      {
    sprintf( buf1, "$n's %s %s $m%c",attack,vp,punct);
    sprintf( buf2, "Your %s %s you%c",attack,vp,punct);
      }
      else
      {
        sprintf( buf1, "$n's %s %s $N%c",  attack, vp, punct );
        sprintf( buf2, "Your %s %s $N%c",  attack, vp, punct );
        sprintf( buf3, "$n's %s %s you%c", attack, vp, punct );
      }
  }
    }

    if (ch == victim)
    {
  act(buf1,ch,NULL,NULL,TO_ROOM,FALSE);
  act(buf2,ch,NULL,NULL,TO_CHAR,FALSE);
    }
    else
    {
      act( buf1, ch, NULL, victim, TO_NOTVICT ,FALSE);
      act( buf2, ch, NULL, victim, TO_CHAR ,FALSE);
      act( buf3, ch, NULL, victim, TO_VICT ,FALSE);
    }

    return;
}



/*
 * Disarm a creature.
 * Caller must check for successful attack.
 */
void disarm( CHAR_DATA *ch, CHAR_DATA *victim )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
  return;

  if(!IS_NPC(victim))
  {
    AFFECT_DATA af;
    af.where     = TO_AFFECTS;
    af.type      = gsn_disarm;
    af.level     = ch->level;
    af.location  = APPLY_HITROLL;
    af.modifier  = ch->level / -2;
    af.duration  = 2;
    af.bitvector = 0;
  
    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    {
      act("$n {GDISARMS{x you, but your weapon won't budge!\n\rYou feel a shattering pain as your wrist sprains.",
  	ch,NULL,victim,TO_VICT ,FALSE);
      act("You disarm $N but $S weapon won't budge!  $N cringes in pain.",
  	ch,NULL,victim,TO_CHAR,FALSE);
      act("$n disarms $N but $S weapon won't budge!  $N cringes in pain.",ch,NULL,victim,TO_NOTVICT,FALSE);
      af.duration = 3;
      flash_affect_to_char( victim, &af );
      af.location = APPLY_DAMROLL;
      flash_affect_to_char( victim, &af );
      return;
    }
    if(number_percent() < ( get_skill(ch,gsn_disarm) / 20 ) && ch->class == class_lookup("blademaster"))
    {
      act("$n {GDISARMS{x you, adding a savage twist as $e breaks your grip!\n\rYou feel a shattering pain as your wrist sprains, but still recover your weapon.",
  	ch,NULL,victim,TO_VICT ,FALSE);
      act("You disarm $N with a vicious flourish!\n\r$N cringes in pain, but scoops $S weapon back up.",
  	ch,NULL,victim,TO_CHAR,FALSE);
      act("$n disarms $N with a vicious flourish!\n\r$N cringes in pain, but scoops $S weapon back up.",ch,NULL,victim,TO_NOTVICT,FALSE);
      af.modifier *= 2; /* Hammer their hit and damroll */
      af.duration = 3;
      flash_affect_to_char( victim, &af );
      af.location = APPLY_DAMROLL;
      flash_affect_to_char( victim, &af );
      return;
    }
    act("$n {GDISARMS{x you, leaving your wrist stinging!\n\rYou quickly recover your weapon.",
      ch,NULL,victim,TO_VICT ,FALSE);
    act("You disarm $N but $E quickly recovers $S weapon.",
      ch,NULL,victim,TO_CHAR,FALSE);
    act("$n disarms $N but $E quickly recovers $S weapon.", ch,NULL,victim,TO_NOTVICT,FALSE);
  
    flash_affect_to_char( victim, &af );
    af.location = APPLY_DAMROLL;
    flash_affect_to_char( victim, &af );
    return;
  }

    if ( IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    {
  act("$S weapon won't budge!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n tries to disarm you, but your weapon won't budge!",
      ch,NULL,victim,TO_VICT,FALSE);
  act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT,FALSE);
  return;
    }

     /* Blademasters */
    if ( number_percent() < ( get_skill(ch,gsn_disarm) / 10 )
         && !IS_OBJ_STAT(obj,ITEM_NODROP) &&
         ch->class == class_lookup("blademaster")
         && (!IS_NPC(ch) && !IS_SET(ch->mhs,MHS_GLADIATOR)))
    {
       act("$n {GDISARMS{x you, and sends your weapon into $s own hands!",
        ch,NULL,victim,TO_VICT ,FALSE);
       act("You disarm $N and send $S weapon into your own hands!",
        ch,NULL,victim,TO_CHAR,FALSE);
       act("$n disarms $N and catches the weapon!",ch,NULL,victim,TO_NOTVICT,FALSE);
       obj_from_char( obj );
       obj_to_char( obj, ch );

       /* Primary has been disarmed move Secondary to Primary */
       /* moved to handler.c
       if ((obj = get_eq_char(victim,WEAR_SECOND)) != NULL)
       {
          obj_from_char( obj );
          obj_to_char( obj, victim);
          equip_char( victim, obj, WEAR_WIELD );
       }
       */
    }
    else
    {
    act( "$n {GDISARMS{x you and sends your weapon flying!",
   ch, NULL, victim, TO_VICT    ,FALSE);
    act( "You disarm $N!",  ch, NULL, victim, TO_CHAR    ,FALSE);
    act( "$n disarms $N!",  ch, NULL, victim, TO_NOTVICT ,FALSE);

    obj_from_char( obj );
    if ( IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_INVENTORY)
         || (!IS_NPC(victim) && IS_SET(victim->mhs,MHS_GLADIATOR)))
  obj_to_char( obj, victim );
    else
    {
  obj_to_room( obj, victim->in_room );
  obj->stolen_timer += 10 * number_fuzzy(5);
  if (IS_NPC(victim) && victim->wait == 0 && can_see_obj(victim,obj))
      get_obj(victim,obj,NULL);
    }

       /* Primary has been disarmed move Secondary to Primary */
       /* moved to handler.c
       if ((obj = get_eq_char(victim,WEAR_SECOND)) != NULL)
       {
          obj_from_char( obj );
          obj_to_char( obj, victim);
          equip_char( victim, obj, WEAR_WIELD );
       }
       */
    }

    return;
}

void do_dae_tok( CHAR_DATA *ch, char *argument )
{
    int skill;
    AFFECT_DATA af;

    if( ch->race != race_lookup("yinn") )
        return;

    if ( is_affected(ch,gsn_dae_tok) )
    {
        send_to_char("You already did that.\n\r",ch);
        return;
    }

    if ( ch->move < 10 )
    {
        send_to_char("You must rest.\n\r",ch);
        return;
     }

     ch->move -= apply_chi(ch,10 );

     WAIT_STATE(ch,skill_table[gsn_dae_tok].beats);

    if ( number_percent() > ( skill = get_skill(ch,gsn_dae_tok) ) )
    {
        check_improve(ch,gsn_dae_tok,FALSE,2);
        send_to_char("You failed.\n\r",ch);
        return;
    }

    check_improve(ch,gsn_dae_tok,TRUE,4);
    af.where            = TO_AFFECTS;
    af.type             = gsn_dae_tok;
    af.level            = ch->level;
    af.duration         = skill / 8;
    af.modifier         = skill / 20;
    af.location         = APPLY_HITROLL;
    af.bitvector        = 0;
    affect_to_char(ch,&af);

     af.location                = APPLY_DAMROLL;
     affect_to_char(ch,&af);

     send_to_char("You put yourself into a state of combat readiness.\n\r",ch);
     act("$n quietly utters a yinnish mantra.",ch,NULL,NULL,TO_ROOM,FALSE);
     return;
}

void do_berserk( CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    int chance, hp_percent;

    if ((chance = get_skill(ch,gsn_berserk)) == 0
    ||  (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BERSERK))
    ||  (!IS_NPC(ch)
    &&   ch->level < skill_level(ch,gsn_berserk)))
    {
  send_to_char("You turn red in the face, but nothing happens.\n\r",ch);
  return;
    }

    //if (IS_AFFECTED(ch,AFF_BERSERK) || is_affected(ch,gsn_berserk)
    if (is_affected(ch,gsn_berserk)
    || ( is_affected(ch,skill_lookup("frenzy")) &&
         ch->race != race_lookup("dwarf") ) )
    {
  send_to_char("You get a little madder.\n\r",ch);
  return;
    }

    if (IS_AFFECTED(ch,AFF_CALM))
    {
  send_to_char("You're feeling too mellow to berserk.\n\r",ch);
  return;
    }

    if (ch->mana < 50)
    {
  send_to_char("You can't get up enough energy.\n\r",ch);
  return;
    }

    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s goes berserk! The crowd goes wild!",ch->name);
       gladiator_talk(buf);
    }

    /* modifiers */

    /* fighting */
    if (ch->position == POS_FIGHTING)
  chance += 10;

    /* damage -- below 50% of hp helps, above hurts */
    hp_percent = 100 * ch->hit/UMAX(1, ch->max_hit);
    chance += 25 - hp_percent/2;

    if (number_percent() < chance)
    {
  AFFECT_DATA af;

  WAIT_STATE(ch,PULSE_VIOLENCE);
  ch->mana -= 50;
  ch->move *= 5;
  ch->move /= UMAX(6,apply_chi(ch,10));

  /* heal a little damage */
  ch->hit += ch->level * 2;
  ch->hit = UMIN(ch->hit,ch->max_hit);

  send_to_char("Your pulse races as you are consumed by rage!\n\r",ch);
  act("$n gets a wild look in $s eyes.",ch,NULL,NULL,TO_ROOM,FALSE);
  check_improve(ch,gsn_berserk,TRUE,2);

  af.where  = TO_AFFECTS;
  af.type   = gsn_berserk;
  af.level  = ch->level;
  af.duration = number_fuzzy(ch->level / 8);
  af.modifier = UMAX(1, ch->level/5);

  if ( !IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("warrior") &&
        ch->class != class_lookup("berzerker")))
        af.modifier = UMAX(1,ch->level/7);

  if ( !IS_NPC(ch) && ( ch->pcdata->old_class == class_lookup("warrior") &&
        ch->class != class_lookup("berzerker")))
        af.modifier = UMAX(1,ch->level/5);

  if ( !IS_NPC(ch) && ( ch->class == class_lookup("berzerker")))
        af.modifier = UMAX(1,ch->level/4 + 1);


  af.bitvector  = AFF_BERSERK;

  af.location = APPLY_HITROLL;
  affect_to_char(ch,&af);

  af.location = APPLY_DAMROLL;
  affect_to_char(ch,&af);

  af.modifier = UMAX(10,10 * (ch->level/5));
  af.location = APPLY_AC;
  affect_to_char(ch,&af);
    }

    else
    {
  WAIT_STATE(ch,2 * PULSE_VIOLENCE);
  ch->mana -= 25;
  ch->move *= 5;
  ch->move /= UMAX(6,apply_chi(ch,8));

  send_to_char("Your pulse speeds up, but nothing happens.\n\r",ch);
  check_improve(ch,gsn_berserk,FALSE,2);
    }
}

void do_grenade_affect(CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *grenade, int dam, bool single_target)
{
  AFFECT_DATA af;
  CHAR_DATA *vch, *vch_next;
  switch ( attack_table[grenade->value[4]].damage )
  {
    case DAM_COLD:
       cold_effect(victim, grenade->value[1],dam,single_target);
       break;
    case DAM_FIRE:
       fire_effect(victim, grenade->value[1],dam,single_target);
       break;
    case DAM_ACID:
       acid_effect(victim,grenade->value[1],dam,single_target);
       break;
    case DAM_LIGHTNING:
       shock_effect(victim,grenade->value[1],dam,single_target);
       break;
    case DAM_LIGHT:
      af.where		= TO_AFFECTS;
      af.type		= gsn_sunburst;
      af.level		= grenade->level;
      af.duration		= (dam%2); /* duration will be 0 or 1 */
      af.modifier		= -6;
      af.location		= APPLY_HITROLL;
      af.bitvector	= AFF_BLIND;
      af.caster_id = ch->id;
      if(single_target)
      {
        damage(ch,victim,number_range(1,2),grenade->value[4]+TYPE_HIT,
          attack_table[grenade->value[4]].damage,TRUE,FALSE);
        if(IS_SET(victim->mhs,MHS_GLADIATOR))
          af.duration = 1;
        else
          af.duration++;
        if(!IS_AFFECTED(victim,AFF_BLIND) && !saves_spell(ch->level, victim, DAM_LIGHT))
        {
          affect_to_char(victim, &af);
          apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);
        }
      }
      else
      {
        for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
        {
          vch_next = vch->next_in_room;
          if (is_safe_spell(ch,vch,TRUE, gsn_sunburst)
          ||  (IS_NPC(vch) && IS_NPC(ch)
          &&   (ch->fighting != vch || vch->fighting != ch)))
            continue;
          damage(ch,vch,number_range(1,2),grenade->value[4]+TYPE_HIT,
            attack_table[grenade->value[4]].damage,FALSE,FALSE);
          if(!IS_AFFECTED(vch,AFF_BLIND) && !saves_spell(ch->level / 2, vch, DAM_LIGHT))
          {// Boom
            affect_to_char(vch,&af);
            act("You are blinded by the burst of light!",vch,NULL,NULL,TO_CHAR,FALSE);
            act("$n is blinded by the burst of light!",vch,NULL,NULL,TO_ROOM,FALSE);
            apply_mala_damage(ch, vch, MALA_NORMAL_DAMAGE);
          }
        }
      }
       break;
  }
}

void do_grenade( CHAR_DATA *ch, char *argument )
{
    int dam = 1, chance;
    char arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *grenade;
    int iMiss;
    argument = one_argument( argument, arg );
    one_argument( argument, arg2 );

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
      send_to_char("Gladiators can't chuck grenades.\n\r", ch);
      return;
    }

    if ( arg[0] == '\0' || (!ch->fighting && argument[0] == '\0') )
    {
        send_to_char("Syntax: grenade <item> <victim/'ground'>\n\r",ch);
        return;
    }

    if ( ( grenade = get_obj_carry( ch, arg ) ) == NULL )
    {
        send_to_char("You aren't carrying that.\n\r",ch);
        return;
    }

    if ( grenade->item_type != ITEM_GRENADE )
    {
       send_to_char("That isn't a grenade weapon.\n\r",ch);
       return;
    }
    
    if(grenade->level > ch->level + 2)
    {
      send_to_char("That grenade is too powerful for you to safely throw.\n\r", ch);
      return;
    }

    if(arg2[0])
    {
      if ( ( victim = get_char_room(ch,arg2) ) == NULL  && str_cmp(arg2,"ground"))
      {
          send_to_char("Nobody here by that name.\n\r",ch);
          return;
      }
    }
    else
      victim = ch->fighting;

    /* special affects, based on damage type, hit either the character
       if the grenade hits the target, or the room, if grenade misses */
    if(victim)
    {// Ground otherwise
      if ( is_safe(ch,victim) )
      {
          send_to_char("Go play with someone else, they aren't worth your time.\n\r",ch);
          return;
      }
  
      if ( IS_NPC(victim) && victim->fighting != NULL &&
            !is_same_group(ch,victim->fighting))
      {
          send_to_char("Kill stealing is not permitted.\n\r",ch);
          return;
      }
  
      if ( IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
      {
          act("But $N is your friend!",ch,NULL,victim,TO_CHAR,FALSE);
          return;
      }
  
  
      check_killer( ch, victim );

      chance = get_curr_stat(victim, STAT_DEX);
      chance += ( victim->level - ch->level ) * 5;
      if(HAS_KIT(ch,"alchemist"))
        chance /= 2; // Alchemists are better at nailing these
      chance += ( !can_see(victim,ch,FALSE) ) ? 15 : 0;
      chance /= ( IS_AWAKE(victim) ) ? 1 : 2;
      chance += is_shaded(victim) ? 30 : 0;// Very hard to hit a shaded target
      dam = dice( grenade->value[0], grenade->value[1] );

      chance = UMIN(95, chance);// Always a 5% chance of hitting
      /* Miss */
      if ( number_percent() < chance )
      {
          act("$n lobs $p at $N, but misses.",ch,grenade,victim,TO_NOTVICT,FALSE);
          act("You lob $p at $N but miss.",ch,grenade,victim,TO_CHAR,FALSE);
          act("$n lobs $p at you, but misses.",ch,grenade,victim,TO_VICT,FALSE);
          /* Add handling for spaslh here */
          iMiss = TARGET_ROOM;
      }
      else
      {
        act("You hit $N with $p!",ch,grenade,victim,TO_CHAR,FALSE);
        act("$n hits you with $p!",ch,grenade,victim,TO_VICT,FALSE);
        act("$n hits $N with $p!",ch,grenade,victim,TO_NOTVICT,FALSE);
        /* add handling for special effects here */
        iMiss = TARGET_CHAR;
        if(attack_table[grenade->value[4]].damage == DAM_LIGHT)
        {
           if(IS_SET(victim->affected_by_ext, AFF_EXT_SHADED) &&
            !is_affected(victim, gsn_light_blast))
           {
             act("$n shudders in pain and solidifies as the {Wlight{x explodes inside $m", victim, NULL, NULL, TO_ROOM, FALSE);
             send_to_char("You shudder in pain as the {Wlight{x explodes inside your body!\n\r", victim);
             dam += number_range(dam * 3, dam * 4); // OUCH!
             AFFECT_DATA af;
             af.where  = TO_AFFECTS;
             af.type   = gsn_light_blast;
             af.level  = ch->level;
             af.duration = 30;
             af.location = APPLY_NONE;
             af.modifier = 0;
             af.bitvector  = 0;
             af.caster_id = ch->id;
             affect_to_char(victim, &af);
             remove_shaded_room(victim);// Can't maintain the shade
           }
        }
        // Problem: If a second shade has the room shaded this does no damage
        damage(ch,victim,dam,grenade->value[4]+TYPE_HIT,attack_table[grenade->value[4]].damage,TRUE,FALSE);
        do_grenade_affect(ch, victim, grenade, dam, TRUE);
      }
    }
    else
    {
      act("You smash $p into the ground!",ch,grenade,victim,TO_CHAR,FALSE);
      act("$n smashes $p into the ground!",ch,grenade,NULL,TO_ROOM,FALSE);
      iMiss = TARGET_ROOM;
    }
    if(iMiss == TARGET_ROOM)
    {
      if(attack_table[grenade->value[4]].damage == DAM_LIGHT)
      {
        if(IS_SET(ch->in_room->room_affects, RAFF_SHADED))
        {
          AFFECT_DATA *raf, *raf_next;
          for ( raf = ch->in_room->affected ; raf != NULL ; raf = raf_next )
          {
            raf_next = raf->next;
            if(raf->type == gsn_shaded_room)
              raffect_remove(ch->in_room, raf);
          }
          act("The darkness in the room shatters in the blast of {WLIGHT{x!", ch, NULL, NULL, TO_ALL, FALSE);
        }
      }
      do_grenade_affect(ch, victim, grenade, dam, FALSE);
    }
    WAIT_STATE( ch, PULSE_VIOLENCE  );
    obj_from_char( grenade );
    extract_obj( grenade );
    return;
}



void do_throw( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_throw)) == 0
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_level(ch,gsn_throw)))
    {
  send_to_char("Throw? What? Huh?\n\r",ch);
  return;
    }
    if (is_mounted(ch) )
    {
    send_to_char("So, you're going to get off your horse and throw them?\r\n",ch);
    return;
    }

    if (arg[0] == '\0')
    {
  victim = ch->fighting;
  if (victim == NULL)
  {
      send_to_char("But you aren't fighting anyone!\n\r",ch);
      return;
  }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }


    if (victim->position < POS_FIGHTING)
    {
  act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (victim == ch)
    {
  send_to_char("You try to flip over to everyone's delight.\n\r",ch);
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if (is_safe(ch,victim))
  return;

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( IS_NPC(victim) &&
  victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("But $N is your friend!",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE(ch,skill_table[gsn_throw].beats/2);
        return;
    }

    /* modifiers */

    /* size  and weight */
    chance -= victim->carry_weight / 200;

    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 30;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 10;

    /* level */
    chance += 2*(ch->level - victim->level);

    /* race */
    /*
    if (victim->race == race_lookup("kender")) chance = (3*chance) / 4;
    */

    /* Dodge lowers the chance of landing throw, up to -33% */
    if (!IS_NPC(victim))
       chance -= (get_skill(victim,gsn_dodge)/3 );

    /* Always a slim chance it will land */
    if (chance < 0 )
       chance = 1;

    if( victim->race == race_lookup("kender")
        && number_percent() <= victim->level)
    {
        act("You roll out of the way of $n's throw.",ch,NULL,victim,TO_VICT,FALSE);
        act("$N rolls to safety, you fall flat on your face.",ch,NULL,victim,TO_CHAR,FALSE);
        if (IS_AFFECTED(ch,AFF_SLOW))
                WAIT_STATE(ch,skill_table[gsn_throw].beats+12);
        else
                WAIT_STATE(ch,skill_table[gsn_throw].beats);
        return;
    }

    /* now the attack */
    if (number_percent() <= chance )
    {

  act("$n flips you to the ground with an amazing throw!",
    ch,NULL,victim,TO_VICT,FALSE);
  act("You flip $N with a throw and send $M to the ground!",
        ch,NULL,victim,TO_CHAR,FALSE);
  act("$n throws $N to the ground.", ch,NULL,victim,TO_NOTVICT,FALSE);
  check_improve(ch,gsn_throw,TRUE,1);

    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s throws %s to the ground! I bet that hurt.",ch->name,victim->name);
       gladiator_talk(buf);
    }
  DAZE_STATE(victim, 3 * PULSE_VIOLENCE);
  WAIT_STATE(ch,skill_table[gsn_throw].beats);
  victim->position = POS_RESTING;
  damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_throw,
      DAM_BASH,FALSE,FALSE);

    }
    else
    {
  damage(ch,victim,0,gsn_throw,DAM_BASH,FALSE,FALSE);
  act("You fall flat on your face!",
      ch,NULL,victim,TO_CHAR,FALSE);
  act("$n falls flat on $s face.",
      ch,NULL,victim,TO_NOTVICT,FALSE);
  act("You evade $n's throw, causing $m to fall flat on $s face.",
      ch,NULL,victim,TO_VICT,FALSE);
  check_improve(ch,gsn_throw,FALSE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s tried to throw %s to the ground, %s eats dirt instead.",ch->name,victim->name,ch->name);
       gladiator_talk(buf);
    }
  ch->position = POS_RESTING;
  WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2);
    }

  return;
}


void do_tail_slap(   CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    int chance;

    one_argument(argument,arg);

    if (!IS_SET(race_table[ch->race].parts,PART_TAIL) )
    {
        send_to_char("You wiggle your butt.\n\r", ch);
        return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("You're not fighting anybody.\n\r",ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They're not here.\n\r",ch);
        return;
    }


    if (victim == ch)
    {
        send_to_char("You chase your tail.\n\r",ch);
        return;
    }

    if (is_safe(ch,victim))
        return;

if( is_affected(victim,skill_lookup("wraithform")) )
{
send_to_char("Your tail passes right through their misty form.\r\n",ch);
return;
}

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( IS_NPC(victim) &&
        victim->fighting != NULL &&
        !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
      act("$N is your beloved master.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }

    check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
      {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE(ch, 2*PULSE_VIOLENCE);
        return;
      }


    /* Compute chance of landing the tail slap.
     * Tail slap is based on your size, level, and hours.
     * Defense is just a dodge, basically.
     */

    chance = ch->level + ( ch->size * 5 );
    chance += ( IS_NPC(ch) ? 0 : ( 2 * ch->level / 3 ) );
    chance += ( ch->hit / 100 );
    chance -= get_curr_stat(victim,STAT_DEX)*2;
    chance -= ( victim->size * 3 );

    if (victim->race == race_lookup("kender")) chance = (3*chance) / 4;

    chance = URANGE( 10, chance, 90 );

    if ( number_percent() < chance )
    {
        act("$n sends you to the ground with a tail slap!",
                ch,NULL,victim,TO_VICT,FALSE);
        act("You knock $N to the ground with your tail!",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n knocks $N to the ground with $s tail!",
                ch,NULL,victim,TO_NOTVICT,FALSE);

    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s trips up %s's feet , score one for the dragons.",ch->name,victim->name);
       gladiator_talk(buf);
    }
        DAZE_STATE(victim, 2 * PULSE_VIOLENCE);
        WAIT_STATE(ch,2 * PULSE_VIOLENCE);
        damage(ch,victim,dice(ch->level /10+1,8),gsn_bash,DAM_BASH,FALSE,FALSE);
        /*
        victim->position = POS_RESTING;
        */
    }

    else
    {
        WAIT_STATE(ch,2 * PULSE_VIOLENCE);
          act("You attempt to whip $N with your tail, but {ymiss{x.",ch,NULL,victim,TO_CHAR,FALSE);
          act("$n attempts to whip you with $s tail, but {ymisses{x.",ch,NULL,victim,TO_VICT,FALSE);
          act("$n attempts to whip $N with $s tail, but {ymisses{x.",ch,NULL,victim,TO_NOTVICT,FALSE);
          damage(ch,victim,0,gsn_bash,DAM_BASH,FALSE,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"Some dragons just can't use their tails well, %s failed to trip up %s.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    }

    return;
}

void do_bash( CHAR_DATA *ch, char *argument )
{
    bool fGiant = FALSE;
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_bash)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_BASH))
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_level(ch,gsn_bash)))
    {
  send_to_char("Bashing? What's that?\n\r",ch);
  return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }

    if (arg[0] == '\0')
    {
  victim = ch->fighting;
  if (victim == NULL)
  {
      send_to_char("But you aren't fighting anyone!\n\r",ch);
      return;
  }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }


    if (victim->position < POS_FIGHTING)
    {
  act("You'll have to let $M get back up first.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (victim == ch)
    {
  send_to_char("You try to bash your brains out, but fail.\n\r",ch);
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if (is_safe(ch,victim))
  return;
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("You bash your way through their misty form.\r\n",ch);
    return;
    }



    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( IS_NPC(victim) &&
  victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("But $N is your friend!",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

  check_killer(ch,victim);

    fGiant = ( ch->race == race_lookup("giant") || ch->race == race_lookup("ogre") );

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        /* Giants and ogres get a +10% chance to dispel an orb of touch with bash */
        check_dispel( !fGiant ? ch->level : (ch->level + (ch->level/10)) , victim, skill_lookup("orb of touch"));
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_bash].beats);
        else
            WAIT_STATE(ch,skill_table[gsn_bash].beats/2);
        return;
    }

    /* modifiers */
    /* size  and weight */
    chance += ch->carry_weight / 250;
    chance -= victim->carry_weight / 200;

    if (!IS_NPC(ch) &&   ch->pcdata->old_class != class_lookup("warrior") )
        chance -= 25;

    if (ch->size < victim->size)
  chance += (ch->size - victim->size) * 15;
    else
  chance += (ch->size - victim->size) * 10;


    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= (get_curr_stat(victim,STAT_DEX) * 4)/3;
    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
        chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
        chance -= 30;

    /* level */
    chance += (ch->level - victim->level);

    /* magic */
    if ( is_affected(victim,gsn_fumble) )
    chance += 15;

    /* race */
    if (victim->race == race_lookup("kender")) chance = (3*chance) / 4;

    /* mountes */
    if ( is_mounted( victim ) && !is_mounted(ch) &&
        victim->riding->size >= ch->size )
        chance /= 2;

    if (!IS_NPC(victim)
       && chance < get_skill(victim,gsn_dodge) && !is_mounted(victim) )
    { /*
        act("$n tries to bash you, but you dodge it.",ch,NULL,victim,TO_VICT,FALSE);
        act("$N dodges your bash, you fall flat on your face.",ch,NULL,victim,TO_CHAR,FALSE);
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_bash].beats+12);
        else
            WAIT_STATE(ch,skill_table[gsn_bash].beats);
        return;*/
  chance -= 3 * (get_skill(victim,gsn_dodge) - chance);
    }

    if ( !IS_NPC(victim) &&  !is_mounted(victim) &&
          ((get_skill(victim,gsn_tumbling) > 0  &&
           (number_percent() + chance / 2) < get_skill(victim,gsn_tumbling))
        || (victim->race == race_lookup("kender")
            && number_percent() <= victim->level)))
    {
        act("You roll out of the way of $n's bash.",ch,NULL,victim,TO_VICT,FALSE);
        act("$N rolls to safety, you fall flat on your face.",ch,NULL,victim,TO_CHAR,FALSE);
        check_improve(victim,gsn_tumbling,TRUE,3);
        if (IS_AFFECTED(ch,AFF_SLOW))
                WAIT_STATE(ch,skill_table[gsn_bash].beats+12);
        else
                WAIT_STATE(ch,skill_table[gsn_bash].beats);
        return;
    }

    /* 20% bonus based on level */
    if ( fGiant )
        chance += 20;

    /* now the attack */
    if (number_percent() < chance )
    {
        act("$n sends you sprawling with a powerful bash!", ch,NULL,victim,TO_VICT,FALSE);
        act("You slam into $N, and send $M flying!",ch,NULL,victim,TO_CHAR,FALSE);
        act("$n sends $N sprawling with a powerful bash.", ch,NULL,victim,TO_NOTVICT,FALSE);
        check_improve(ch,gsn_bash,TRUE,1);
        victim->position = POS_RESTING;
        if ( IS_AFFECTED(ch,AFF_SLOW))
                WAIT_STATE(ch,skill_table[gsn_bash].beats+12);
        else
                WAIT_STATE(ch,skill_table[gsn_bash].beats);

        /* Gladiator Spectator Channel */
        if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
        {
           sprintf(buf,"Somebody get the number of that wagon! %s just got slammed by %s.",victim->name,ch->name);
           gladiator_talk(buf);
        }

        if ( is_mounted(victim) ) /* knocked off horse */
        {
            if ( number_percent() < get_skill(victim,gsn_riding/3) )
            {
                act("You manage to stay on your mount.",victim,NULL,NULL,TO_CHAR,FALSE);
                act("$n manages to stay mounted.",victim,NULL,NULL,TO_ROOM,FALSE);
                DAZE_STATE(victim, PULSE_VIOLENCE);
                damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE,TRUE);
                return;
            }
            else
            {
                act("$n falls from $s mount!",victim,NULL,NULL,TO_ROOM,FALSE);
                act("You fall from your mount!",victim,NULL,NULL,TO_CHAR,FALSE);
                victim->riding->passenger = NULL;
                victim->riding = NULL;
                DAZE_STATE(victim, PULSE_VIOLENCE*4);
                damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE,TRUE);
                return;
            }
        }

            if ( number_percent() + chance < get_skill(victim,gsn_tumbling) && get_skill(victim,gsn_tumbling) > 0 )
            {
                act("$n rolls to $s feet!",victim,NULL,NULL,TO_ROOM,FALSE);
                act("You roll to your feet.",victim,NULL,NULL,TO_CHAR,FALSE);
                victim->position = POS_STANDING;
                check_improve(ch,gsn_tumbling,TRUE,3);
                DAZE_STATE(victim, PULSE_VIOLENCE);
                damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE,TRUE);
                return;
            }

        /* Not mounted.  Now figure out delay - 2 or 3 rounds */
        /* Ogres/giants have enhanced chance to lay 'em out flat */
        /* Otherwise, compare STR versus some random chance. */
        chance = get_curr_stat(ch,STAT_STR) + ( fGiant ? 10 : 0 );
        if ( number_percent() < chance )
                DAZE_STATE( victim, PULSE_VIOLENCE*3 );
        else
                DAZE_STATE( victim, PULSE_VIOLENCE*5/2 );
        damage(ch,victim,number_range(2,2 + 2 * ch->size + chance/20),gsn_bash, DAM_BASH,FALSE,TRUE);

   }
    else
    {
  damage(ch,victim,0,gsn_bash,DAM_BASH,FALSE,FALSE);
  act("You fall flat on your face!",
      ch,NULL,victim,TO_CHAR,FALSE);
  act("$n falls flat on $s face.",
      ch,NULL,victim,TO_NOTVICT,FALSE);
  act("You evade $n's bash, causing $m to fall flat on $s face.",
      ch,NULL,victim,TO_VICT,FALSE);
  check_improve(ch,gsn_bash,FALSE,1);
  ch->position = POS_RESTING;
  if ( IS_AFFECTED(ch,AFF_SLOW) )
      WAIT_STATE(ch,skill_table[gsn_bash].beats * 2 );
  else
      WAIT_STATE(ch,skill_table[gsn_bash].beats * 3/2);
    }

  return;
}

void do_dirt( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_dirt)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK_DIRT))
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_level(ch,gsn_dirt)))
    {
  send_to_char("You get your feet dirty.\n\r",ch);
  return;
    }

    if (arg[0] == '\0')
    {
  victim = ch->fighting;
  if (victim == NULL)
  {
      send_to_char("But you aren't in combat!\n\r",ch);
      return;
  }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
  act("$E's already been blinded.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (victim == ch)
    {
  send_to_char("Very funny.\n\r",ch);
  return;
    }

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("Your well kicked clods of dirt pass right through them.\r\n",ch);
    return;
    }



    if (is_safe(ch,victim))
  return;
    if (IS_NPC(victim) &&
   victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("But $N is such a good friend!",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    /* modifiers */

    /* dexterity */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_DEX);

    /* speed  */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
  chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
  chance -= 25;
    if (!IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("warrior") &&
                         ch->pcdata->old_class != class_lookup("thief") ) )
  chance -= 25;

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* sloppy hack to prevent false zeroes */
    if (chance % 5 == 0)
  chance += 1;

    /* terrain */

    switch(ch->in_room->sector_type)
    {
  case(SECT_INSIDE):    chance -= 20; break;
  case(SECT_CITY):    chance -= 10; break;
  case(SECT_FIELD):   chance +=  5; break;
  case(SECT_FOREST):        break;
  case(SECT_HILLS):       break;
  case(SECT_MOUNTAIN):    chance -= 10; break;
  case(SECT_WATER_SWIM):    chance  =  0; break;
  case(SECT_WATER_NOSWIM):  chance  =  0; break;
  case(SECT_AIR):     chance  =  0;   break;
  case(SECT_DESERT):    chance += 10;   break;
    }

    if (chance == 0)
    {
  send_to_char("There isn't any dirt to kick.\n\r",ch);
  return;
    }

  if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }

  ch->move -= apply_chi(ch,(ch->level/15));
  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_dirt].beats);
        else
            WAIT_STATE(ch,skill_table[gsn_dirt].beats/2);
        return;
    }

    /* now the attack */
    if (number_percent() < chance)
    {
  AFFECT_DATA af;
  act("$n is blinded by the dirt in $s eyes!",victim,NULL,NULL,TO_ROOM,FALSE);
  act("$n kickes dirt in your eyes!",ch,NULL,victim,TO_VICT,FALSE);
        damage(ch,victim,number_range(2,5),gsn_dirt,DAM_NONE,FALSE,FALSE);
  send_to_char("You can't see a thing!\n\r",victim);
  check_improve(ch,gsn_dirt,TRUE,2);
  WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent())
    {
       sprintf(buf,"%s throws sand in the eyes of %s. Very unsportsmanlike, but a good sport doesnt live long in the Arena",ch->name,victim->name);
       gladiator_talk(buf);
    }

  af.where  = TO_AFFECTS;
  af.type   = gsn_dirt;
  af.level  = ch->level;
  af.duration = 0;
  af.location = APPLY_HITROLL;
  af.modifier = -4;
  af.bitvector  = AFF_BLIND;

  affect_to_char(victim,&af);
    }
    else
    {
  damage(ch,victim,0,gsn_dirt,DAM_NONE,TRUE,FALSE);
  check_improve(ch,gsn_dirt,FALSE,2);
  WAIT_STATE(ch,skill_table[gsn_dirt].beats);
    }
}

void do_trip( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_trip)) == 0
    ||   (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_TRIP))
    ||   (!IS_NPC(ch)
    && ch->level < skill_level(ch,gsn_trip)))
    {
  send_to_char("Tripping?  What's that?\n\r",ch);
  return;
    }

    if (arg[0] == '\0')
    {
  victim = ch->fighting;
  if (victim == NULL)
  {
      send_to_char("But you aren't fighting anyone!\n\r",ch);
      return;
  }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }

    if ( victim == ch )
    {
        send_to_char("Doh!  You're such a clutz!\n\r",ch);
        return;
        }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("They are made of mist.\r\n",ch);
    return;
    }


    if (is_safe(ch,victim))
  return;
    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if (IS_NPC(victim) &&
   victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
  send_to_char("Kill stealing is not permitted.\n\r",ch);
  return;
    }

    if (victim->position < POS_FIGHTING)
    {
  act("$N is already down.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (victim == ch)
    {
  send_to_char("You fall flat on your face!\n\r",ch);
  WAIT_STATE(ch,2 * skill_table[gsn_trip].beats);
  act("$n trips over $s own feet!",ch,NULL,NULL,TO_ROOM,FALSE);
  return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("$N is your beloved master.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE(ch,skill_table[gsn_trip].beats/2);
        return;
    }

    /* modifiers */

    /* size */
    if (ch->size < victim->size)
        chance += (ch->size - victim->size) * 10;  /* bigger = harder to trip */

    if (!IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("warrior") &&
                          ch->pcdata->old_class!=class_lookup("thief")))
                chance -= 25;

    /* dex */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_DEX) * 3 / 2;

    /* speed */
    if (IS_SET(ch->off_flags,OFF_FAST) || IS_AFFECTED(ch,AFF_HASTE))
  chance += 10;
    if (IS_SET(victim->off_flags,OFF_FAST) || IS_AFFECTED(victim,AFF_HASTE))
  chance -= 20;

    /* level */
    chance += (ch->level - victim->level) * 2;



    if (IS_AFFECTED(victim,AFF_FLYING))
    chance /= 2;

    /* now the attack */
    if (number_percent() < chance)
    {
  act("$n trips you and you go down!",ch,NULL,victim,TO_VICT,FALSE);
  act("You trip $N and $N goes down!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n trips $N, sending $M to the ground.",ch,NULL,victim,TO_NOTVICT,FALSE);
  check_improve(ch,gsn_trip,TRUE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s can't stay standing. Especially not with %s tripping.",victim->name,ch->name);
       gladiator_talk(buf);
    }

  DAZE_STATE(victim,2 * PULSE_VIOLENCE);
        WAIT_STATE(ch,skill_table[gsn_trip].beats);
  victim->position = POS_RESTING;
  damage(ch,victim,number_range(2, 2 +  2 * victim->size),gsn_trip,
      DAM_BASH,TRUE,FALSE);
    }
    else
    {
  damage(ch,victim,0,gsn_trip,DAM_BASH,TRUE,FALSE);
  WAIT_STATE(ch,skill_table[gsn_trip].beats*2/3);
  check_improve(ch,gsn_trip,FALSE,1);
    }

  return;
}

void do_attack( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    if ( !HAS_KIT(ch,"brawler") ) {
        send_to_char("Just kill them!\n\r",ch);
        return;
    }

    one_argument( argument, arg );
    if ( (victim = get_char_room( ch, arg ) ) != NULL )
    {
       if (victim == ch->fighting)
       {
          send_to_char("Don't try to cheat you are being watched.\n\r",ch);
          return;
        }
    }


    kill( ch, argument, TRUE );
    return;
}

void do_kill( CHAR_DATA *ch, char *argument )
{
    kill( ch, argument, FALSE );
    return;
}

void kill( CHAR_DATA *ch, char *argument, bool canChange )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    one_argument( argument, arg );

    if ( canChange && ch->fighting == NULL )
    {
    send_to_char("You aren't in combat.\n\r",ch);
    return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("You are made of mist, you can not attack.\n\r",ch);
    return;
    }

    if ( arg[0] == '\0' )
    {
  send_to_char( "Kill whom?\n\r", ch );
  return;
    }

  if(!IS_NPC(ch))
    ch->pcdata->bypass_blind = TRUE;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
      if(!IS_NPC(ch))
        ch->pcdata->bypass_blind = FALSE;
      if(IS_AFFECTED(ch, AFF_BLIND))
      {
        if(HAS_KIT(ch,"brawler"))
          WAIT_STATE( ch, PULSE_VIOLENCE / 3);
        else
          WAIT_STATE( ch, PULSE_VIOLENCE );
        send_to_char("You flail around blindly but can't find them.\n\r", ch);
        return;
      }
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }
  if(!IS_NPC(ch))
    ch->pcdata->bypass_blind = FALSE;


/*
    if ( !IS_NPC(victim) )
    {
        if ( !IS_SET(victim->act, PLR_KILLER)
        &&   !IS_SET(victim->act, PLR_THIEF) )
        {
            send_to_char( "You must MURDER a player.\n\r", ch );
            return;
        }
    }
*/
    if ( victim == ch )
    {
  send_to_char( "You hit yourself.  Ouch!\n\r", ch );
  multi_hit( ch, ch, TYPE_UNDEFINED );
  return;
    }
    if ( is_safe( ch, victim ) )
  return;
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
      send_to_char("They are made of mist.  Your weapon goes right through them.\r\n",ch);
      return;
    }

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
  act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if ( ch->position == POS_FIGHTING )
    {
        if ( !canChange ) {
  send_to_char( "You do the best you can!\n\r", ch );
  return;
        }
        else
        {
            ch->fighting = victim;
            act("$n focuses %s attack on $N",ch,NULL,victim,TO_NOTVICT,FALSE);
            act("You focus your attack on $N",ch,NULL,victim,TO_CHAR,FALSE);
            act("$n focuses %s attack on you!",ch,NULL,victim,TO_VICT,FALSE);
        }
    }

    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s trades blows with %s.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_murde( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to MURDER, spell it out.\n\r", ch );
    return;
}



void do_murder( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  send_to_char( "Murder whom?\n\r", ch );
  return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) || (IS_NPC(ch) && IS_SET(ch->act,ACT_PET)))
  return;

    if(!IS_NPC(ch))
      ch->pcdata->bypass_blind = TRUE;

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
     if(!IS_NPC(ch))
      ch->pcdata->bypass_blind = FALSE;
      if(IS_AFFECTED(ch, AFF_BLIND))
      {
        if(HAS_KIT(ch,"brawler"))
          WAIT_STATE( ch, PULSE_VIOLENCE / 3);
        else
          WAIT_STATE( ch, PULSE_VIOLENCE );
        send_to_char("You flail around blindly but can't find them.\n\r", ch);
        return;
      }
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }
    if(!IS_NPC(ch))
      ch->pcdata->bypass_blind = FALSE;

    if ( victim == ch )
    {
  send_to_char( "Suicide is a mortal sin.\n\r", ch );
  return;
    }

    if ( is_safe( ch, victim ) )
  return;

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if (IS_NPC(victim) &&
   victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
    {
  act( "$N is your beloved master.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if ( ch->position == POS_FIGHTING )
    {
  send_to_char( "You do the best you can!\n\r", ch );
  return;
    }

    WAIT_STATE( ch, 1 * PULSE_VIOLENCE );
    if (IS_NPC(ch))
  sprintf(buf, "Help! I am being attacked by %s!",ch->short_descr);
    else
    {
      if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE)
         sprintf( buf, "Help!  I am being attacked by %s!", ch->long_descr );
      else
         sprintf( buf, "Help!  I am being attacked by %s!", ch->name );
    }
    do_yell( victim, buf );
    check_killer( ch, victim );
    multi_hit( ch, victim, TYPE_UNDEFINED );
    return;
}



void do_backstab( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    char buf[MAX_STRING_LENGTH];
    int wait_mod = 0;
    int percent;

    one_argument( argument, arg );

    if ( IS_AFFECTED(ch,AFF_SLOW) )
        wait_mod = 12;

    if ( !IS_NPC(ch) && ch->pcdata->old_class != class_lookup("thief") )
        wait_mod += 12;

    if ( !IS_NPC(ch)
    &&   ch->level < skill_level(ch,gsn_backstab) )
    {
  send_to_char("You better leave assasinations to thieves.\n\r", ch );
  return;
    }

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("Backstab whom?\n\r",ch);
            return;
        }
    }

    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }


    if ( victim == ch )
    {
  send_to_char( "How can you sneak up on yourself?\n\r", ch );
  return;
    }

    if ( is_safe( ch, victim ) )
      return;

      if( is_affected(victim,skill_lookup("wraithform")) )
      {
      send_to_char("Your brilliant backstab passes through thier misty body.\r\n",ch);
      return;
      }


    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if (IS_NPC(victim) &&
   victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if ( !IS_AFFECTED(ch,AFF_MORPH)
        && ( obj = get_eq_char( ch, WEAR_WIELD ) ) == NULL)
    {
  send_to_char( "You need to wield a weapon to backstab.\n\r", ch );
  return;
    }

    if ( ch->move < (ch->level/10) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/10));

    if ( victim->fighting != NULL && victim->hit < victim->max_hit / 2 )
    {
  send_to_char( "You can't hack a bigger hole into them.\n\r", ch );
  return;
    }

    if ( victim->hit < victim->max_hit / 3)
    {
  act( "$N is hurt and suspicious ... you can't sneak up.",
      ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    check_killer( ch, victim );

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE( ch, skill_table[gsn_backstab].beats/2+wait_mod);
        return;
    }
    percent  = get_skill(ch,gsn_backstab);
    if(!IS_NPC(victim) && victim->class == class_lookup("rogue"))
        percent -= 25;

    if(!IS_NPC(ch) && ch->class == class_lookup("rogue"))
        percent += 25;

    if(!IS_NPC(ch) && (ch->class == class_lookup("rogue")))
        percent = URANGE(5,percent,100);
    else
        percent = URANGE(5,percent,95);


    WAIT_STATE( ch, skill_table[gsn_backstab].beats + wait_mod );
    if ( (number_percent( ) < percent )
    || ( get_skill(ch,gsn_backstab) >= 2 && !IS_AWAKE(victim) ) )
    {
  check_improve(ch,gsn_backstab,TRUE,1);
  multi_hit( ch, victim, gsn_backstab );
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s sticks a knife in %s's back, there goes that friendship.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    }
    else
    {
  check_improve(ch,gsn_backstab,FALSE,1);
  damage( ch, victim, 0, gsn_backstab,DAM_NONE,TRUE,FALSE);
    }

    return;
}


void do_flee( CHAR_DATA *ch, char *argument )
{
   flee( ch, argument, FALSE );
   return;
}

void flee( CHAR_DATA *ch, char *argument, bool fWimpy )
{
    ROOM_INDEX_DATA *was_in;
    ROOM_INDEX_DATA *now_in;
    CHAR_DATA *victim;
    int attempt;
    char buf[MAX_STRING_LENGTH];

    if ( fWimpy && ch->daze )
        return;

    if ( ( victim = ch->fighting ) == NULL )
    {
        if ( ch->position == POS_FIGHTING )
            ch->position = POS_STANDING;
  send_to_char( "You aren't fighting anyone.\n\r", ch );
  return;
    }
    if ( is_affected(victim,skill_lookup("aura of cthon")) && number_percent() < 30 )
    {
    send_to_char( "THE HORROR!! THE HORROR of {RCthon{x freezes the blood in your veins!\n\r ", ch );
    send_to_char( "You feel the aura of Cthon around you.\r\n",victim );
    return;
    }

    if ( is_affected(ch,skill_lookup("restrain")) && number_percent() < 50 )
    {
    send_to_char( "{YPANIC!!!{x  You have been {BRESTRAINED{x!!!\n\r ", ch );
    return;
    }

    if( !fWimpy )
    {
      WAIT_STATE( ch, PULSE_PER_SECOND / 2 );
    }
    ch->move -= apply_chi( ch, 2 );

    was_in = ch->in_room;

    for ( attempt = 0; attempt < 6; attempt++ )
    {
  EXIT_DATA *pexit;
  int door;

  door = number_door( );
  if ( ( pexit = was_in->exit[door] ) == 0
  ||   pexit->u1.to_room == NULL
  ||   pexit->u1.to_room->clan
  ||   IS_SET(pexit->exit_info, EX_CLOSED)
  ||   number_range(0,ch->daze) != 0
  || ( IS_NPC(ch)
  &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
      continue;

  move_char( ch, door, FALSE );
  if ( ( now_in = ch->in_room ) == was_in )
      continue;

  ch->in_room = was_in;
  act( "$n has fled!", ch, NULL, NULL, TO_ROOM ,FALSE);
  ch->in_room = now_in;

  ch->move -= apply_chi( ch, 10 );

  if ( number_percent() < get_skill(ch,gsn_tumbling) ||
        HAS_KIT(ch,"ninja") )
  {
      check_improve(ch,gsn_tumbling,TRUE,3);
      act( "You flee $T!", ch, NULL, dir_name[door], TO_CHAR,FALSE);
  }
  else
        send_to_char("You flee from combat!\n\r", ch );

  if ( !IS_NPC(ch) )
  {// No xp loss any more
/*    if( (ch->class == 2 || HAS_KIT(ch,"acrobat") ||
        HAS_KIT(ch,"ninja") )
        && (number_percent() < 3*(ch->level/2) ) )
    send_to_char( "You snuck away safely.\n\r", ch);
  else
      {
      send_to_char( "You lost 10 exp.\n\r", ch);
      gain_exp( ch, -10 );
      }*/
  if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 0)
  {//Trial 0: Can't flee
        sprintf(buf, "You have {Rfailed{x the trial given to you by %s.\n\r", deity_table[ch->pcdata->deity].pname);
        send_to_char(buf,ch);
        ch->pcdata->deity_trial_timer = 0;
        log_deity_favor(ch, victim, DEITY_TRIAL_FAIL_REQS);
  }
  }

  stop_fighting( ch, TRUE );
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"%s couldn't handle the pressure and runs off.",ch->name);
       gladiator_talk(buf);
    }
  return;
    }

    send_to_char( "PANIC! You couldn't escape!\n\r", ch );
    return;
}



void do_rescue( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    CHAR_DATA *fch;
    char buf[MAX_STRING_LENGTH];

    argument = one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
  send_to_char( "Rescue whom?\n\r", ch );
  return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if ( victim == ch || victim->fighting == ch )
    {
  send_to_char( "What about fleeing instead?\n\r", ch );
  return;
    }

    if ( !IS_NPC(ch) && IS_NPC(victim) && !is_same_group(ch,victim))
    {
  send_to_char( "Doesn't need your help!\n\r", ch );
  return;
    }

    if ( ch->fighting == victim )
    {
  send_to_char( "Too late.\n\r", ch );
  return;
    }

    if ( ( fch = victim->fighting ) == NULL )
    {
  send_to_char( "That person is not fighting right now.\n\r", ch );
  return;
    }

    if ( IS_NPC(fch) && (!is_same_group(ch,victim) && !is_same_clan(ch,victim)))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_SET(victim->mhs,MHS_NORESCUE))
    {
       send_to_char("They don't want to be rescued.\n\r",ch);
       return;
    }

    if ( is_safe(ch,fch))
    {
        send_to_char("They don't need your help!\n\r",ch);
        return;
    }

    check_killer(ch,fch);

    WAIT_STATE( ch, skill_table[gsn_rescue].beats );
    one_argument(argument,arg2);
    if(!strcmp(arg2, "xvx2"))
    {
      sprintf(buf, "%s attempted to cheat rescue.", ch->name);
      log_string(buf);
      if(ch->master)
      {
        sprintf(buf, "%s's ally attempted to cheat rescue.", ch->master->name);
        log_string(buf);
      }
    }
    if ( number_percent( ) > get_skill(ch,gsn_rescue) && strcmp(arg2, "~"))
    {
  send_to_char( "You fail the rescue.\n\r", ch );
  check_improve(ch,gsn_rescue,FALSE,1);
  return;
    }

    act( "You rescue $N!",  ch, NULL, victim, TO_CHAR    ,FALSE);
    act( "$n rescues you!", ch, NULL, victim, TO_VICT    ,FALSE);
    act( "$n rescues $N!",  ch, NULL, victim, TO_NOTVICT ,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"It appears we have a hero in the Arena today! %s decides to rescue %s.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    check_improve(ch,gsn_rescue,TRUE,1);

    stop_fighting( fch, FALSE );
    stop_fighting( victim, FALSE );
    stop_fighting( ch, FALSE );

    check_killer( ch, fch );
    set_fighting( ch, fch );
    set_fighting( fch, ch );
    return;
}



void do_kick( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char buf[MAX_STRING_LENGTH];

    if ( !IS_NPC(ch)
    &&   ch->level < skill_level(ch,gsn_kick))
    {
  send_to_char(
      "You better leave the martial arts to fighters.\n\r", ch );
  return;
    }

    if (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_KICK))
  return;

    if ( ( victim = ch->fighting ) == NULL )
    {
  send_to_char( "You aren't fighting anyone.\n\r", ch );
  return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Cool, you kicked a cloud.\r\n",ch);
    return;
    }


    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE( ch, skill_table[gsn_kick].beats);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_kick].beats );
    if ( get_skill(ch,gsn_kick) > number_percent())
    {
 damage(ch,victim,number_range(2,(3*ch->level)/2),gsn_kick,DAM_BASH,TRUE,FALSE);
  check_improve(ch,gsn_kick,TRUE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"Amazing! A beautiful roundhouse from %s nails %s.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    }
    else
    {
  damage( ch, victim, 0, gsn_kick,DAM_BASH,TRUE,FALSE);
  check_improve(ch,gsn_kick,FALSE,1);
    }
    return;
}

void do_insanity( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;

    if ( !IS_NPC(ch)
    &&   ch->level < skill_level(ch,gsn_insanity) )
    {
  send_to_char(
      "You better just take your lithium for now.\n\r", ch );
  return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
  send_to_char( "You aren't fighting anyone.\n\r", ch );
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/5));

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE( ch, skill_table[gsn_insanity].beats);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_insanity].beats );
    if ( get_skill(ch,gsn_insanity) > number_percent())
    {
  act("$n goes into a fit of rage directed at you!",
    ch,NULL,victim,TO_VICT,FALSE);
  act("You charge madly at $N, and flay $M!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n charges $N looking crazed.",
    ch,NULL,victim,TO_NOTVICT,FALSE);
  damage(ch,victim,number_range(10,3*ch->level),gsn_insanity,DAM_BASH,TRUE,FALSE);
  DAZE_STATE(victim, 2* PULSE_VIOLENCE/3);
  check_improve(ch,gsn_insanity,TRUE,1);
    }
    else
    {
  act("You swing wildly and hurt yourself!", ch,NULL,victim,TO_CHAR,FALSE);
  act("$n flails widly hurting $mself.", ch,NULL,victim,TO_NOTVICT,FALSE);
  if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
 act("You narrowly escape $n's fit of insanity.", ch,NULL,victim,TO_VICT,FALSE);
  damage(ch,ch,number_range(10,3*ch->level),gsn_insanity,DAM_BASH,TRUE,FALSE);
  DAZE_STATE(ch, 2* PULSE_VIOLENCE/3);
  check_improve(ch,gsn_insanity,FALSE,2);
    }
  return;
}


void do_dbite( CHAR_DATA *ch, char *argument )
{
        char  arg[MAX_INPUT_LENGTH];
        CHAR_DATA *victim;
        int bite_damage;
        int chance;

        one_argument(argument, arg);

      if ( !IS_SET(race_table[ch->race].parts,PART_FANGS) )
      {
        send_to_char("Your chompers a tad lacking for that.\n\r",ch);
        return;
      }

        if ( arg[0] == '\0' )
        {
        if ( ( victim = ch->fighting ) == NULL )
        {
        send_to_char( "Yeah, we already know you bite.\n\r", ch );
        return;
        }

        if( is_affected(ch,skill_lookup("wraithform")) )
        {
        send_to_char("Not while in wraithform.\r\n",ch);
        return;
        }

        }
        else
        if (( victim = get_char_room(ch, arg) ) == NULL )
        {
                send_to_char("That individual isn't here.\n\r", ch);
                return;
        }



        if (victim->fighting != NULL &&
           IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
        {
           send_to_char("Honorable combat is one on one.\n\r",ch);
           return;
        }

        if ( IS_NPC(victim) &&
                victim->fighting != NULL &&
                !is_same_group(ch, victim->fighting))
        {
                send_to_char("Kill stealing is not permitted.\n\r", ch);
                return;
        }

        if (victim == ch)
        {
                send_to_char("You reach around and gnaw on your tail.\n\r",ch);
                return;
        }

        if( is_affected(victim,skill_lookup("wraithform")) )
        {
        send_to_char("Biting at a fog bank? \r\n",ch);
        return;
        }


        if ( is_safe(ch, victim))
        {
                send_to_char("Pick on somebody your own size.\n\r", ch);
                return;
        }
      if( is_affected(victim,skill_lookup("orb of touch")) )
      {
           send_to_char("You bounce off an orb of touch.\n\r",ch);
           check_dispel( ch->level , victim, skill_lookup("orb of touch"));
           WAIT_STATE(ch,12);
           return;
      }

        check_killer(ch,victim);
        WAIT_STATE( ch, 24 );
        bite_damage= dice(ch->size,ch->level);

          chance = 3 * ch->level / 2;
          chance += ch->size * 4;
          chance -= 2 * get_curr_stat(victim,STAT_DEX) /3;
          chance -= ( check_dodge( ch, victim,FALSE ) ? 20 : 0 );

        if ( victim->fighting == NULL )
        {
                bite_damage *= 2;
                chance += 25;
        }

        chance = URANGE( 5, chance, 95 );

        if ( number_percent( ) < chance || !IS_AWAKE(victim) )
              damage(ch,victim,bite_damage,gsn_bite,DAM_PIERCE,TRUE,FALSE);
        else
                damage( ch, victim, 0, gsn_bite, DAM_PIERCE,TRUE,FALSE);

        return;
}

void do_bite( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    int sn;
    char buf[MAX_STRING_LENGTH];

    if(IS_NPC(ch))
       return;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Mistfangs, kinky but not very effictive.\r\n",ch);
    return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
  send_to_char( "You aren't fighting anyone.\n\r", ch );
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }

    if ( ch->pcdata->condition[COND_FULL] > 46 )
    {
        send_to_char("You no longer crave the flesh.\n\r",ch);
        return;
    }

    ch->move -= apply_chi(ch,(ch->level/15));

   check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE( ch, skill_table[gsn_bite].beats/2);
        return;
    }
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("You bite air, cold, fridgid, undead air.\r\n",ch);
    return;
    }



    WAIT_STATE( ch, skill_table[gsn_bite].beats );
    if ( get_skill(ch,gsn_bite) > number_percent())
    {
  damage(ch,victim,number_range( ch->level/4, ch->level ), gsn_bite,DAM_PIERCE,TRUE,FALSE);
    if (saves_spell(ch->level-2,victim,DAM_DISEASE) ||
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
     if (ch == victim)
        send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
     else
        act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
    }
    else
    {
        sn = skill_lookup("plague");
        af.where     = TO_AFFECTS;
        af.type     = sn;
        af.level    = ch->level;
        af.duration  = (ch->level/10) + 1;
        af.location  = APPLY_STR;
        af.modifier  = -1;
        af.bitvector = AFF_PLAGUE;
        affect_join(victim,&af);

        send_to_char("Your stomache turns and you feel ill.\n\r",victim);
        send_to_char("Your bite spreads the infestation.\n\r",ch);

        /* Gladiator Spectator Channel */
        if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
        {
           sprintf(buf,"%s sinks a pair of plague-infested rotting teeth into %s.",ch->name,victim->name);
           gladiator_talk(buf);
        }
    }
    check_improve(ch,gsn_bite,TRUE,1);
   }
   else
   {
        damage( ch, victim, 0, gsn_bite,DAM_PIERCE,TRUE,FALSE);
        check_improve(ch,gsn_bite,FALSE,1);
   }
    return;
}

void do_bleed( CHAR_DATA *ch, char *argument )
{
        CHAR_DATA *victim;
        int dam;
        char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

        one_argument( argument, arg );

    if (IS_NPC(ch))
       return;


    if ( arg[0] == '\0' )
    {
        victim = ch->fighting;
        if ( ( victim = ch->fighting ) == NULL )
        {
            send_to_char("You aren't fighting anybody.\n\r",ch);
            return;
        }
    }
    else if ( ( victim = get_char_room(ch,arg) ) == NULL )
    {
        send_to_char("Who?  They aren't here.\n\r",ch);
        return;
    }

    if (IS_IMMORTAL(victim))
    {
        send_to_char("Don't be a dumbass.\n\r", ch);
        return;
    }


    if ((is_clan(ch) && !is_clan(victim) && !IS_NPC(victim))
         || (!is_clan(ch) && !IS_NPC(victim)))
    {
        send_to_char("Don't cheat...you will get caught.\n\r",ch);
        return;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_UNDEAD))
    {
        send_to_char("You cannot bleed the already dead.\n\r", ch);
        return;
    }


    if ( victim == ch )
    {
        send_to_char("You taste like chicken.\n\r",ch);
        return;
    }

    if (is_safe(ch,victim))
       return;

 if ( IS_NPC(victim) &&
   victim->fighting != NULL &&
    !is_same_group(ch,victim->fighting))
         {
           send_to_char("Kill stealing is not permitted.\n\r",ch);
         return;
      }


    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }

    if ( ch->pcdata->condition[COND_FULL] > 46 )
    {
        send_to_char("Your lust for blood is already satiated.\n\r",ch);
        return;
    }

    ch->move -= apply_chi(ch,(ch->level/15));

    if ( get_skill(ch,gsn_bleed) < 1 )
        return;

    check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        WAIT_STATE( ch, skill_table[gsn_bleed].beats/2);
        return;
    }

    WAIT_STATE( ch, skill_table[gsn_bleed].beats );
    if ( get_skill(ch,gsn_bleed) > number_percent())
    {
        dam = number_range(ch->level/2, 5*ch->level/3);
        switch(check_immune(victim,DAM_PIERCE))
        {
         case(IS_IMMUNE):
          dam = 0;
          break;
         case(IS_RESISTANT):
          dam -= dam/4;
          break;
         case(IS_VULNERABLE):
          dam += dam/4;
          break;
        }

        damage( ch, victim, dam, gsn_bleed, DAM_PIERCE, TRUE,FALSE);
        ch->mana += 4 * dam  / 5;
        send_to_char("You feel your lifeblood seeping from your neck.\n\r",victim);
        send_to_char("Your heart beats strong with life.\n\r",ch);
        check_improve(ch,gsn_bleed,TRUE,1);

    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 75)
    {
       sprintf(buf,"EWWW! %s drinks blood from %s. The crowd loves it!",ch->name,victim->name);
       gladiator_talk(buf);
    }

/** The higher your level, the more bleeding you need to get full
    Seems backwards, but it allows for better bleed at 51 **/

        gain_condition(ch,COND_FULL,   15 - ( ch->level / 5 ) );
        gain_condition(ch,COND_HUNGER, 15 - ( ch->level / 5 ) );
        gain_condition(ch,COND_THIRST, 15 - ( ch->level / 5 ) );

        if ( ch->pcdata->condition[COND_FULL] > 46 )
        {
          send_to_char("Your lust for blood has been satiated.\n\r",ch);
          return;
        }

    }
    else
    {
        send_to_char("Your teeth gnash together, {ymissing{x their target.\n\r",ch);
        damage(ch,victim,0,gsn_bleed,DAM_PIERCE,TRUE,FALSE);
        check_improve(ch,gsn_bite,FALSE,1);
    }
  return;
}

void do_hex(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

   if(IS_NPC(ch))
     return;

   if(!IS_SET(ch->act,PLR_MUMMY))
   {
     send_to_char("You ain't no stinking Mummy!\n\r",ch);
     return;
   }

   if( is_affected(ch,skill_lookup("wraithform")) )
   {
     send_to_char("Not while in wraithform.\r\n",ch);
     return;
   }

   if ( ( victim = ch->fighting ) == NULL )
    {
       send_to_char( "You aren't fighting anyone.\n\r", ch );
       return;
    }
    one_argument( argument, arg );

    if (arg[0] != '\0')
    {
       if ( ( victim = get_char_room(ch,arg) ) == NULL )
       {
        send_to_char("Who?  They aren't here.\n\r",ch);
        return;
       }
    }
    if ( IS_NPC(victim) && victim->spec_fun != 0 )
    {
      if (
         victim->spec_fun == spec_lookup("spec_honor_guard")
         || victim->spec_fun == spec_lookup("spec_demise_guard")
         || victim->spec_fun == spec_lookup("spec_posse_guard")
         || victim->spec_fun == spec_lookup("spec_zealot_guard")
         || victim->spec_fun == spec_lookup("spec_warlock_guard")
         )
         {
           send_to_char("Clan guards of Boigna are immune to your hex.  Ha-ha.\r\n",ch);
           return;
         }
    }


    if (is_safe(ch,victim))
       return;
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("Your hex does not affect them, they are pretty scary looking themselves.\r\n",ch);
    return;
    }


 if ( IS_NPC(victim) &&
   victim->fighting != NULL &&
    !is_same_group(ch,victim->fighting))
         {
           send_to_char("Kill stealing is not permitted.\n\r",ch);
         return;
         }

    WAIT_STATE( ch, skill_table[gsn_hex].beats );

    if ( get_skill(ch,gsn_hex) > number_percent())
    {
       if ( saves_spell(ch->level,victim,DAM_OTHER))
       {
          send_to_char("Your hex fails.\n\r",ch);
          send_to_char("You ward off a hex.\n\r",victim);
        act( "$n makes a sign with $s fingers and wards off the hex.",
                victim, NULL, victim, TO_ROOM ,FALSE);
          return;
       }
       else
       {
          af.where     = TO_AFFECTS;
          af.type      = gsn_curse;
          af.level     = ch->level;
          af.duration  = ch->level / 3;
          af.location  = APPLY_HITROLL;
          af.modifier  = -3;
          af.bitvector = AFF_CURSE;
          affect_to_char( victim, &af );

          af.location  = APPLY_SAVING_SPELL;
          af.modifier  = 3;
          affect_to_char( victim, &af );

          send_to_char( "You have been hexed!\n\r", victim );
          act( "$n has been hexed.", victim, NULL, victim, TO_ROOM ,FALSE);
          damage(ch,victim,0,gsn_hex,DAM_OTHER,FALSE,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"Have you ever seen a hex in action? %s is teaching %s what its all about.",ch->name,victim->name);
       gladiator_talk(buf);
    }
       }
       check_improve(ch,gsn_hex,TRUE,1);
    }
    else
    {
       send_to_char("You attempt to cast a hex but fail.\n\r",ch);
       check_improve(ch,gsn_hex,FALSE,1);
    }

    return;
}

void do_fear( CHAR_DATA *ch, char *argument )
{
   CHAR_DATA *victim,*nextvictim;
   ROOM_INDEX_DATA *was_in;
   ROOM_INDEX_DATA *now_in;
   EXIT_DATA *pexit;
   int attempt, door;
    char buf[MAX_STRING_LENGTH];

   if (IS_NPC(ch))
      return;

   if(!IS_SET(ch->act,PLR_MUMMY))
   {
     send_to_char("You ain't no stinking Mummy!\n\r",ch);
     return;
   }

   if ( ch->move < 20 )
   {
       send_to_char("You're tired of making scary faces.\n\r",ch);
       return;
   }

    if ( ch->pcdata->condition[COND_FULL] > 46 )
    {
        send_to_char("Your desire for fear has been completed.\n\r",ch);
        return;
    }

   ch->move -= apply_chi(ch,20);

   WAIT_STATE( ch, skill_table[gsn_fear].beats );

   if ( number_percent() < get_skill(ch,gsn_fear) )
   {
      act( "$n spreads fear through the room!", ch, NULL, NULL, TO_ROOM ,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"%s is scaring everyone away!",ch->name);
       gladiator_talk(buf);
    }
      for ( victim= ch->in_room->people; victim != NULL; victim = nextvictim )
      {
         nextvictim = victim->next_in_room;
         if(victim != ch  &&
              ( (is_clan(victim) && is_clan(ch) ) || IS_NPC(victim) ) )
         {
            if (IS_NPC(victim) && (IS_SET(victim->act,ACT_TRAIN)
                ||  victim->pIndexData->pShop != NULL
                ||  IS_SET(victim->act,ACT_PRACTICE)
                ||  IS_SET(victim->act,ACT_IS_HEALER)
                ||  IS_SET(victim->act,ACT_NOPURGE)
                ||  IS_SET(victim->act, ACT_AGGRESSIVE)
                ||  IS_SET(victim->act,ACT_IS_CHANGER)))
               continue;

            if(is_safe(ch,victim))
                continue;

            if ( saves_spell(ch->level,victim,DAM_MENTAL))
               continue;
            if( is_affected(ch,skill_lookup("wraithform")) )
               continue;

            if (is_same_group(ch,victim))
               continue;

            if ( IS_AFFECTED(victim, AFF_SLEEP))
               continue;

            for ( attempt = 0; attempt < 6; attempt++ )
            {
               door = number_door();
               was_in = victim->in_room;
               if ( ( pexit = was_in->exit[door] ) == 0
                 ||   pexit->u1.to_room == NULL
                 ||   IS_SET(pexit->exit_info, EX_CLOSED)
                 ||   number_range(0,victim->daze) != 0
                 ||   (IS_NPC(victim)
                 &&   IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
                 continue;

               move_char( victim, door, FALSE );
               if ( ( now_in = victim->in_room ) == was_in )
                  continue;

               victim->in_room = was_in;
               act( "$n runs in fear!", victim, NULL, NULL, TO_ROOM ,FALSE);
               victim->in_room = now_in;
               send_to_char("You run in fear.\n\r",victim);
               check_killer(ch,victim);
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"%s hops out of the shadows and scares the hell out of %s, who runs off like a little girl.",ch->name,victim->name);
       gladiator_talk(buf);
    }

               gain_condition(ch,COND_FULL,   15 - ( ch->level / 5 ) );
               gain_condition(ch,COND_HUNGER, 15 - ( ch->level / 5 ) );
               gain_condition(ch,COND_THIRST, 15 - ( ch->level / 5 ) );

               if ( ch->pcdata->condition[COND_FULL] > 46 )
               {
                  send_to_char("Your fear has empowered you completely.\n\r",ch);
                  return;
               }
            }
         }
      }
      check_improve(ch,gsn_fear,TRUE,1);
   }
   else
   {
      send_to_char("You attempt to instill fear but fail.\n\r",ch);
      check_improve(ch,gsn_fear,FALSE,1);
   }
   return;
}

void do_breathe( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    AFFECT_DATA af;
    int sn;
    char buf[MAX_STRING_LENGTH];

     if(IS_NPC(ch))
       return;

    if(!IS_SET(ch->act,PLR_MUMMY))
    {
      send_to_char( "Breathe deep, the gathering gloom...\n\r", ch );
      return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
       send_to_char( "You aren't fighting anyone.\n\r", ch );
       return;
    }

    if ( ch->move < (ch->level/15) )
    {
       send_to_char("You're too exhausted.\n\r",ch);
       return;
    }

    ch->move -= apply_chi(ch,(ch->level/15));
    check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of turning")) )
    {
       send_to_char("Your breath bounces off an orb of turning.\n\r",ch);
       check_dispel( ch->level , victim, skill_lookup("orb of turning"));
       WAIT_STATE( ch, skill_table[gsn_breathe].beats/2);
       return;
    }

    WAIT_STATE( ch, skill_table[gsn_breathe].beats );

    if ( get_skill(ch,gsn_breathe) > number_percent())
    {
       damage(ch,victim,number_range( ch->level/4, ch->level ),
              gsn_breathe,DAM_POISON,TRUE,FALSE);
       if (saves_spell(ch->level-2,victim,DAM_POISON) ||
          (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
       {
          if (ch == victim)
             send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
          else
             act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
       }
       else
       {
          sn = skill_lookup("poison");
          af.where     = TO_AFFECTS;
          af.type     = sn;
          af.level    = ch->level;
          af.duration  = (ch->level/10) + 1;
          af.location  = APPLY_STR;
          af.modifier  = -1;
          af.bitvector = AFF_POISON;
          affect_join(victim,&af);

          send_to_char("Your feel ill from the putrescent breath.\n\r",victim);
          send_to_char("Your rancid breath spreads the poison.\n\r",ch);
        /* Gladiator Spectator Channel */
        if (IS_SET(ch->mhs,MHS_GLADIATOR))
        {
           sprintf(buf,"The stench of %s's makes %s spew chunks all over.",ch->name,victim->name);
           gladiator_talk(buf);
        }
       }
       check_improve(ch,gsn_breathe,TRUE,1);
    }
    else
    {
       damage( ch, victim, 0, gsn_breathe,DAM_POISON,TRUE,FALSE);
       check_improve(ch,gsn_breathe,FALSE,1);
    }
    return;
}

void do_disarm( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    int chance,hth,ch_weapon,vict_weapon,ch_vict_weapon;
    char buf[MAX_STRING_LENGTH];

    hth = 0;

    if ((chance = get_skill(ch,gsn_disarm)) == 0)
    {
  send_to_char( "You don't know how to disarm opponents.\n\r", ch );
  return;
    }

    if ( get_eq_char( ch, WEAR_WIELD ) == NULL
    &&   ((hth = get_skill(ch,gsn_hand_to_hand)) == 0
    ||    (IS_NPC(ch) && !IS_SET(ch->off_flags,OFF_DISARM))))
    {
  send_to_char( "You must wield a weapon to disarm.\n\r", ch );
  return;
    }

    if ( ( victim = ch->fighting ) == NULL )
    {
  send_to_char( "You aren't fighting anyone.\n\r", ch );
  return;
    }

    if ( ( obj = get_eq_char( victim, WEAR_WIELD ) ) == NULL )
    {
  send_to_char( "Your opponent is not wielding a weapon.\n\r", ch );
  return;
    }

    if(is_flash_affected(victim, skill_lookup("disarm")))
    {
      send_to_char("Their grip is already weakened.\n\r", ch);
      return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    /* find weapon skills */
    ch_weapon = get_weapon_skill(ch,get_weapon_sn(ch,FALSE));
    vict_weapon = get_weapon_skill(victim,get_weapon_sn(victim,FALSE));
    ch_vict_weapon = get_weapon_skill(ch,get_weapon_sn(victim,FALSE));

    /* modifiers */

    /* skill */
    if ( get_eq_char(ch,WEAR_WIELD) == NULL)
  chance = chance * hth/150;
    else
  chance = chance * ch_weapon/100;

    chance += (ch_vict_weapon/2 - vict_weapon) / 2;

    if (!IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("warrior") &&
                         ch->pcdata->old_class!=class_lookup("thief")))
        chance -= 25;

    /* dex vs. strength */
    chance += get_curr_stat(ch,STAT_DEX);
    chance -= 2 * get_curr_stat(victim,STAT_STR);

    /* level */
    chance += (ch->level - victim->level) * 2;

    /* Spells */
    chance /= ( is_affected(victim,gsn_stonefist) ? 2 : 1 );

    /* Blademaster */
    if ( victim->class == class_lookup("blademaster") )
        chance /= 3;

     /* Battleragers and bladesingers */
    if ( is_affected(victim,gsn_rage) || is_affected(victim,gsn_bladesong) )
        chance /= 2;

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel( ch->level , victim, skill_lookup("orb of touch"));
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_disarm].beats);
        else
            WAIT_STATE(ch,skill_table[gsn_disarm].beats/2);
        return;
    }

    /* Shoguns are impossible to disarm */
    if ( check_hai_ruki(victim) )
        chance = UMIN(chance,15);

    /* and now the attack */
    if (number_percent() < chance)
    {
      WAIT_STATE( ch, skill_table[gsn_disarm].beats );
  disarm( ch, victim );
  check_improve(ch,gsn_disarm,TRUE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"Now that was impressive! %s just sent %s's weapon flying.",ch->name,victim->name);
       gladiator_talk(buf);
    }
    }
    else
    {
  WAIT_STATE(ch,skill_table[gsn_disarm].beats);
  act("You fail to disarm $N.",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n tries to disarm you, but fails.",ch,NULL,victim,TO_VICT,FALSE);
  act("$n tries to disarm $N, but fails.",ch,NULL,victim,TO_NOTVICT,FALSE);
  check_improve(ch,gsn_disarm,FALSE,1);
    }
    check_killer(ch,victim);
    return;
}



void do_sla( CHAR_DATA *ch, char *argument )
{
    send_to_char( "If you want to SLAY, spell it out.\n\r", ch );
    return;
}



void do_slay( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    char arg[MAX_INPUT_LENGTH];

    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
  send_to_char( "Slay whom?\n\r", ch );
  return;
    }

    if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if ( ch == victim )
    {
  send_to_char( "Suicide is a mortal sin.\n\r", ch );
  return;
    }

    if ( !IS_NPC(victim) && victim->level >= get_trust(ch) )
    {
  send_to_char( "You failed.\n\r", ch );
  return;
    }

    act( "You slay $M in cold blood!",  ch, NULL, victim, TO_CHAR    ,FALSE);
    act( "$n slays you in cold blood!", ch, NULL, victim, TO_VICT    ,FALSE);
    act( "$n slays $N in cold blood!",  ch, NULL, victim, TO_NOTVICT ,FALSE);
    raw_kill( victim,ch );
    return;
}

void do_cutpurse( CHAR_DATA *ch, CHAR_DATA *victim )
{
   int chance;
   int gold,silver;

   if(IS_SET(ch->mhs,MHS_GLADIATOR) || IS_SET(ch->mhs,MHS_HIGHLANDER))
      return;
//   if(ch->clan == clan_lookup("smurf"))
//     return;

   chance = get_skill(ch,gsn_cutpurse)/3;

   chance += (get_curr_stat(ch,STAT_DEX) - (get_curr_stat(victim,STAT_DEX)))*5;
   chance += (ch->level - victim->level)*2;

   if (number_percent() <= chance)
   {
act("You rip apart $N's purse and steal their gold!", ch, NULL, victim,TO_CHAR,FALSE);
act("$n rips open your purse and scoops up the coins!",ch,NULL,victim,TO_VICT,FALSE);
act("$n rips open $N's purse, and scoops up the coins!",ch,NULL,victim,TO_ROOM,FALSE);
   gold = victim->gold/25;
   victim->gold -= gold;
   ch->gold += gold;

   silver = victim->silver/20;
   victim->silver -= silver;
   ch->silver += silver;

   check_improve(ch,gsn_cutpurse,TRUE,3);

   return;
   }
   else
   {
   check_improve(ch,gsn_cutpurse,FALSE,6);
   return;
   }
}

void do_garotte( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    OBJ_DATA *wield;
    AFFECT_DATA af;
    bool checkfail = FALSE;
    int failroll;
    int failchance;
    int level;
    int chance;
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ( !IS_NPC(ch)
    &&   ch->level < skill_level(ch,gsn_garotte) )
    {
      send_to_char( "You better stick to choking your chicken.\n\r", ch );
      return;
    }


    if ( ch->fighting != NULL )
    {
      send_to_char( "You can't sneak up on anyone while you're fighting.\n\r", ch );
      return;
    }

    if( arg[0] == '\0' || (victim = get_char_room(ch,arg)) == NULL )
    {
      send_to_char("They're not here.\n\r",ch);
      return;
    }

    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians do not sleep.\n\r", ch);
      return;
    }

    if(IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
    {
     send_to_char("Highlanders are immune to that.\n\r",ch);
     return;
    }

    if ((!IS_NPC(ch) && !IS_NPC(victim)) && (ch != victim) && ch->pcdata)
    {
        ch->pcdata->quit_time = 4;
        if (victim->pcdata) victim->pcdata->quit_time = 4;
    }

    if (victim->fighting != NULL)
    {
       send_to_char("They are too active to get a hold on.\n\r",ch);
       return;
    }

    if (ch == victim)
    {
     send_to_char(" You attempt to garotte yourself and fail like the pathetic miserable loser you are.\n\r", ch);
     return;
     }

    if (is_safe(ch,victim))
       return;

    if ( IS_AFFECTED(victim, AFF_SLEEP)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)) )
    {
      send_to_char("Leave the sleeping and the undead in peace.\n\r",ch);
      return;
    }

    if ( ch->move < (ch->level/5) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/5));

    /* modifiers */

    chance = get_skill(ch,gsn_garotte);

    /* stats */
    chance += get_curr_stat(ch,STAT_STR);
    chance -= get_curr_stat(victim,STAT_DEX);
    chance -= get_curr_stat(victim,STAT_CON);

    /* level */
    chance += 2 * (ch->level - victim->level);

    if (!IS_NPC(victim))
    {
  chance -= get_skill(victim,gsn_dodge)/4;
    }

    switch(check_immune(victim,DAM_MENTAL))
    {
  case IS_IMMUNE:   chance = 0;  break;
  case IS_RESISTANT:  chance -= 50;  break;
  case IS_VULNERABLE: chance += 50;  break;
    }

/*  if( is_affected(ch,gsn_sneak) || is_affected(ch,gsn_hide) ) */
  if( IS_AFFECTED(ch,AFF_SNEAK) || IS_AFFECTED(ch,AFF_HIDE) )
   {
   if( ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL &&
       wield->value[0] == WEAPON_GAROTTE )
     {
      if ( number_percent() <= chance &&
          dice(wield->value[1],wield->value[2]) > 2 )
      {
         check_improve(ch,gsn_garotte,TRUE,1);
         level = wield->level;

    switch(check_immune(victim,DAM_MENTAL))
    {
  case IS_IMMUNE:   level = 0;  break;
  case IS_RESISTANT:  level /= 2;  break;
  case IS_VULNERABLE: level += 4;  break;
    }

    af.where     = TO_AFFECTS;
    af.type      = gsn_garotte;
    af.level     = level;
    af.duration  = UMIN(level,dice(wield->value[1],wield->value[2]));
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    if ( IS_AWAKE(victim) )
    {
  send_to_char( "You feel very lightheaded.....\n\r", victim );
  act( "$n passes out from sufocation.", victim, NULL, NULL, TO_ROOM ,FALSE);
  WAIT_STATE(ch, 24);
  victim->position = POS_SLEEPING;

        /* Gladiator Spectator Channel */
        if (IS_SET(ch->mhs,MHS_GLADIATOR))
        {
           sprintf(buf,"%s finds it hard to breath with %s's rope around their neck.",victim->name,ch->name);
           gladiator_talk(buf);
        }
    }

      }
      else
      {
        check_improve(ch,gsn_garotte,FALSE,2);
        checkfail = TRUE;
      }
     }
   else
     {
       send_to_char("You expect to choke someone with that?\n\r",ch);
     }
   }
  else
   {
     send_to_char("You've been spotted!\n\r",ch);
     checkfail = TRUE;
   }

   if (checkfail)
   {
       REMOVE_BIT(ch->affected_by, AFF_HIDE);
       WAIT_STATE(ch, 36);
       /*  get attacker's vital statistics, whee!  */
       failchance = ( get_skill(ch, gsn_sneak) /3) + ( get_skill(ch, gsn_hide) /2) + ( get_curr_stat(ch, STAT_DEX) /2);

       failroll = number_percent();

       if (failroll <= failchance)
       {
         send_to_char("Phew, you escaped notice.\n\r",ch);
         return;
       }

       if ( failroll > failchance  && failroll < 95 )
       {
         send_to_char("Whoops...\n\r",ch);
         send_to_char("Someone tried to sneak up on you..\n\r", victim);
       }

       if (failroll >= 95)
       {
         send_to_char("{DUhoh,{R NOW{D you've done it!!{x\n\r",ch);
         act("$n just tried to garotte you!",ch,NULL, victim, TO_VICT, FALSE);
         sprintf(buf, "{YHelp!{C  %s{x just tried to{R garotte{x me!!", ch->name);
         do_yell(victim, buf);
       }
   }

  return;
}

void do_rage ( CHAR_DATA *ch, char *argument )
{
        AFFECT_DATA af;
          OBJ_DATA *weapon;

        if (!HAS_KIT(ch,"battlerager"))
        {
                send_to_char("Huh?\n\r", ch);
                return;
        }

        if (IS_NPC(ch))
        {
                send_to_char("NPC's aren't the raging type.\n\r", ch);
                return;
        }

        if ( skill_level(ch,gsn_rage) > ch->level )
        {
                send_to_char("Soon...very soon.\n\r", ch);
                return;
        }

        if (is_affected(ch, gsn_rage))
        {
                send_to_char("More blood!  More death!  Charge!!\n\r", ch);
                return;
        }

        if (ch->fighting == NULL )
        {
                send_to_char("There's no one to rage upon.\n\r", ch);
                return;
        }

        if ( (weapon= get_eq_char(ch,WEAR_WIELD)) == NULL )
                return;

        switch(weapon->value[0])
        {
        case WEAPON_AXE: break;
        default:
            send_to_char("What?  Without an axe?  What kind of dwarf are you?\n\r", ch);
            return;
        }

        if ( number_percent() >= get_skill(ch, gsn_rage) )
        {
                check_improve(ch,gsn_rage,FALSE,3);
                send_to_char("The anger just isn't there.\n\r", ch);
                return;
        }
        else
        {

        send_to_char("Screaming a battle cry, you fly into a rage!\n\r", ch);

        af.where     = TO_AFFECTS;
        af.type      = gsn_rage;
        af.level     = ch->level;
        af.duration  = 1 + (ch->level / 10);
        af.modifier  = ch->level / 5;
        af.location  = APPLY_HITROLL;
        af.bitvector = 0;
        affect_to_char( ch,     &af );

        af.modifier += get_curr_stat(ch,STAT_CON) / 3;
        af.location  = APPLY_DAMROLL;
        affect_to_char( ch,     &af );

        af.modifier  = 1 * ch->level;
        af.location  = APPLY_AC;
        affect_to_char( ch,     &af );

        af.where            = TO_AFFECTS;
        af.bitvector        = 0;
        af.location         = APPLY_DEX;
        af.modifier         = -4;
        affect_to_char( ch, &af );

        af.location = APPLY_HIT;
        af.modifier = ((!IS_NPC(ch) ? ch->pcdata->perm_hit : ch->max_hit) / 4);
        affect_to_char( ch,  &af );

        ch->hit += (ch->max_hit /5);
        ch->hit = UMIN(ch->hit, ch->max_hit);

        af.location = APPLY_SAVES;
        af.modifier = -4;
        affect_to_char( ch,  &af );

        if (!IS_SET(ch->imm_flags, IMM_CHARM))
        {
                af.bitvector = IMM_CHARM;
                af.where = TO_IMMUNE;
                af.location = APPLY_NONE;
                affect_to_char( ch, &af );
        }
    }
    check_improve(ch,gsn_rage,TRUE,1);
    return;
}


void do_bladesong ( CHAR_DATA *ch, char *argument )
{
        int factor;
        AFFECT_DATA af;
        OBJ_DATA *weapon;
        char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

        if (!HAS_KIT(ch,"bladesinger") )
        {
                send_to_char("Huh?\n\r",ch);
                return;
        }

        one_argument(argument,arg);
        if ( !str_cmp(arg,"stop") && is_affected(ch,gsn_bladesong))
        {
           affect_strip(ch,gsn_bladesong);
           send_to_char("You stop performing the bladesong.\n\r",ch);
           act("$n ends his dancing blades!",ch,NULL,NULL,TO_ROOM,FALSE);
           return;
        }

        if (is_affected(ch, gsn_bladesong))
        {
                send_to_char("You are already using the Bladesong.\n\r", ch);
                return;
        }

        if ( IS_NPC(ch) )
                return;

        if ( ch->level < skill_level(ch,gsn_bladesong) )
        {
                send_to_char("You are not yet skilled enough.\n\r", ch);
                return;
        }

        if ( ch->fighting == NULL )
        {
                send_to_char("You aren't in combat.\n\r", ch);
                return;
        }

        if ( get_eq_char( ch, WEAR_SHIELD ) != NULL )
        {
                send_to_char(
                "The bladesong cannot be performed while wearing a shield.\n\r",
                ch);
                return;
        }

        if ( (weapon = get_eq_char( ch, WEAR_WIELD )) == NULL )
                return;

        switch (weapon->value[0])
        {
                case(WEAPON_SWORD):
                case(WEAPON_DAGGER):
                        break;
                default:
                        send_to_char(
            "The bladesong art is only possible with swords and daggers.\n\r",
                        ch);
                        return;
        }

        if ( number_percent() > get_skill( ch, gsn_bladesong ) )
        {
           send_to_char("You fail to make your blades dance.\n\r",ch);
                check_improve(ch,gsn_bladesong,FALSE,5);
                return;
        }

        factor = ch->level / 6;

        af.where = TO_AFFECTS;
        af.type = gsn_bladesong;
        af.level = ch->level;
        af.duration = factor;
        af.modifier = factor;
        af.location = APPLY_HITROLL;
        af.bitvector = 0;
        affect_to_char( ch, &af );

        af.location = APPLY_DAMROLL;
        affect_to_char( ch, &af );

        af.modifier = factor * -10;
        af.location = APPLY_AC;
        affect_to_char( ch, &af );

        check_improve(ch,gsn_bladesong,TRUE,5);

        send_to_char("You begin to perform the bladesong.\n\r",ch);
        act("$n begins to make his blades dance!",ch,NULL,NULL,TO_ROOM,FALSE);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) )
    {
       sprintf(buf,"An elven bladesong is one of the deadliest maneuvers around. %s is going to open up a can of whopass with this one.",ch->name);
       gladiator_talk(buf);
    }
}

int myrm_pen( CHAR_DATA *ch, CHAR_DATA *victim )
{
    int sd;

    if ( !HAS_KIT(ch,"myrmidon") ) return 0;

    sd = ch->size - victim->size;

    if ( sd < 0 ) sd *= -1;

    sd *= 8 - ( ch->level / 10 );

    return ( sd );
}

int check_myrmidon( CHAR_DATA *ch, int gsn )
{
    int skill;
    int wsn;

    if ( !HAS_KIT(ch,"myrmidon") )
        return 0;

    /* no bonus if current weapon is not the one specialized in */
    if ( ( wsn = get_weapon_sn(ch,FALSE) )  != ch->pcdata->specialize )
        return 0;


   skill = get_skill(ch, gsn );
   if ( gsn == gsn_second_attack )
        skill /=2 ;
   else if ( gsn == gsn_third_attack )
        skill /=4 ;
   else return 0;

  /* return difference always */
   return ( (100+ ch->level ) * skill / 100 - skill );
}


bool check_nether ( CHAR_DATA *ch, CHAR_DATA *victim, bool fSecondary )
{

  OBJ_DATA *shield, *weapon;
  int chance, hit;


   if ( !HAS_KIT( victim, "nethermancer") )
        {
        return FALSE;
        }

   if ( ( shield = get_eq_char( victim, WEAR_SHIELD ) ) == NULL )
         {
         return FALSE;
         }

  if ( shield->pIndexData->vnum != OBJ_VNUM_NETHER_SHIELD )
       {
       return FALSE;
       }

   if (fSecondary && !IS_NPC(ch))
       {
         weapon = get_eq_char(ch,WEAR_SECOND) ;
         hit = ch->pcdata->second_hitroll;
       }
   else
       {
        weapon = get_eq_char(ch,WEAR_WIELD) ;
        hit = ch->hitroll;
       }

  /* no avoiding nether weapons by slipping into the nether plane, they
     just follow you there and smack you */

   if ( weapon != NULL)
   {
   if ( HAS_KIT(ch, "nethermancer") && IS_WEAPON_STAT(weapon,WEAPON_NETHER) )
         {
         return FALSE;
         }
    }

   chance = get_skill( victim, gsn_nether_shield ) / 2;

   if ( ch->class == class_lookup("blademaster") )
           chance -= ( chance / 5 );

   chance += myrm_pen(ch, victim);


   /* Ranger bonus for terrain if victim */
      switch( terrain(victim) )
      {
       case 0:     chance -= ( chance / 5 );       break;
       case 1:     chance += ( chance / 5 );       break;
       default:    break;
      }


    /* you int and your victims int affect your chances of hitting
       the nether before he moves to the nether plane */

    chance +=( (get_curr_stat(victim,STAT_INT) - get_curr_stat(ch,STAT_INT))*2 );
    /* Hitrolls */
    if (hit > 20)
       hit = (hit-20)/2 +20;
    if (hit > 40)
       hit = (hit - 40)/2 +40;

   chance -= hit;

   /* If the weapon is the secondary then harder to parry */
   if (fSecondary)
      chance -= 25;

   if ( number_percent( ) >= chance + victim->level - ch->level )
      {
      return FALSE;
      }
   if(!IS_SET(victim->display,DISP_BRIEF_COMBAT))
   act( "You fade to the nether plane and avoid $n's blow.",  ch, NULL, victim, TO_VICT    ,FALSE);
   if(!IS_SET(ch->display,DISP_BRIEF_COMBAT))
   act( "$N fades to the nether plane and avoids your attack.", ch, NULL, victim, TO_CHAR    ,FALSE);
   victim->mana -= 20;
   return TRUE;


}


/*STart of DO_kcharge function - started 26AUG000 by Boogums */
void do_kcharge( CHAR_DATA *ch, char *argument )
{

  char arg[MAX_INPUT_LENGTH];
  CHAR_DATA *victim;
  /* CHAR_DATA *victim1; */
  OBJ_DATA *obj;
  int percent;
  EXIT_DATA *pexit;
  int door;

  one_argument( argument, arg );

  if (arg[0] == '\0')
  {
    victim = ch->fighting;
    if (victim == NULL)
      {
      send_to_char("Charge whom?\n\r",ch);
      return;
      }
  }

  else if ((victim = get_char_room(ch,arg)) == NULL)
      {
      send_to_char("They aren't here.\n\r",ch);
      return;
      }
      if( is_affected(victim,skill_lookup("wraithform")) )
      {
      send_to_char("Your skillful charge carries you through their misty form.\r\n",ch);
      return;
      }

  obj = get_eq_char( ch,WEAR_WIELD );
/* 15SEP00 - Variations on a theme:  added checks for polearm use by warrior
             and blademaster and likewse for flail by paladins and clerics.
             Rage suggested leaving the caviler kit for something else :)
             -Boogums
*/

  if ( (ch->class == class_lookup("warrior")
        || ch->class == class_lookup("blademaster"))
     && (obj == NULL || obj->value[0] != WEAPON_POLEARM) )
    {
    send_to_char( "You need to wield a polearm in order to charge.\r\n", ch );
    return;
    }

  if ( (ch->class == class_lookup("paladin")
        || ch->class == class_lookup("cleric"))
     && ( obj == NULL || obj->value[0] != WEAPON_FLAIL) )
     {
       send_to_char( "You need to wield a flail order to charge.\r\n", ch );
       return;
     }

  if( !is_mounted(ch) )
    {
    send_to_char("You better saddle up on your handy dandy warhorse if you want to charge.\n\r",ch );
    return;
    }
/*Ok here comes the start, all sanity checks a go here goes the kcharge */

  if( is_mounted(ch) && IS_NPC(ch->riding) && ch->riding->pIndexData->vnum == MOB_VNUM_WARHORSE )
    {

    if (is_safe( ch,victim ) ) return;
    if (IS_NPC(victim) && victim->fighting != NULL &&
        !is_same_group(ch,victim->fighting))
      {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
      }

    if (victim == ch)
      {
         send_to_char("Charging yourself? That'd be quite the sight.\n\r",ch);
         return;
      }

     if (is_safe(ch,victim)) return;

     if (victim->fighting != NULL)
       {
         if (ch->fighting != NULL)
         {
         send_to_char("You are unable to charge at them.\n\r",ch);
         return;
         }
       }


    check_killer( ch, victim );

        if( is_affected(victim,skill_lookup("orb of touch")) )
          {
            send_to_char("You bounce off an orb of touch.\n\r",ch);
            check_dispel( ch->level , victim, skill_lookup("orb of touch"));
            WAIT_STATE( ch, skill_table[gsn_kcharge].beats);
            return;
          }

    percent = get_skill(ch,gsn_kcharge);
    if(!IS_NPC(victim) && victim->kit == kit_lookup("knight"))
      percent -= 25;

    percent = URANGE(5,percent,100);

    if ( (number_percent( ) < percent )
    || ( get_skill(ch,gsn_kcharge) >=2 && !IS_AWAKE(victim) ) )
      {
      check_improve(ch,gsn_kcharge,TRUE,1);
      WAIT_STATE( ch, skill_table[gsn_kcharge].beats);
      multi_hit( ch, victim, gsn_kcharge );
      }
    else
      {
      switch (number_range(0,1))
      {
        case 0:
        check_improve(ch,gsn_kcharge,TRUE,1);
        damage( ch,victim,0,gsn_kcharge,DAM_NONE,TRUE,FALSE);
        door = number_door();
        if ( ( pexit = victim->in_room->exit[door] ) == 0
        ||  pexit->u1.to_room == NULL
        ||  pexit->u1.to_room->clan
        ||  IS_SET(pexit->exit_info, EX_CLOSED)
        || (IS_NPC(ch)
        && IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB) ) )
        /* should stop combat only if the room is clear to move in to */
        stop_fighting(ch,TRUE);
        stop_fighting(victim,TRUE);
        act("ROFL!!! $n charges right out of the room.",ch,NULL,victim,TO_ROOM,FALSE);
        act("You charge right out of the room.",ch,NULL, victim,TO_CHAR,FALSE);
        move_char(ch, door, FALSE );
        WAIT_STATE( ch, skill_table[gsn_kcharge].beats*2);
        WAIT_STATE( victim, skill_table[gsn_kcharge].beats/2);
        break;
      case 1:
        act("$n tries to perform a beautiful charge but fails.\r\n",ch,NULL,victim,TO_ROOM,FALSE);
        act("Your charge fails.\r\n",ch,NULL, victim,TO_CHAR,FALSE);
        WAIT_STATE( ch, skill_table[gsn_kcharge].beats);
        break;
      default:
        WAIT_STATE( ch, skill_table[gsn_kcharge].beats*2);
        break;

      } /*end switch */
      return;
    }
  }
  else
  {
    send_to_char("Your squire knocks coconuts together while you charge around the room.\n\r",ch );
    return;
  }
return;

} /* End curly brace for do_kcharge */


void  handle_critical( CHAR_DATA *ch, int *damage, int dam_type, int diceroll, int base_dam )
{
    int cap;
    bool fCleave = FALSE, fBludgeon = FALSE, fEnhance = FALSE;
    int bonus;

    cap = 20;
    /* not possible to roll a twenty, if they don't have
                a skill or are oldclass warrior, no crit*/

    if ( IS_NPC(ch) && IS_SET(ch->act,ACT_WARRIOR) )
        --cap;

    if ( !IS_NPC(ch) && ch->pcdata->old_class == class_lookup("warrior") )
        --cap;

    if ( number_percent() < get_skill(ch,gsn_enhanced_critical) )
    {
        fEnhance = TRUE;
        --cap;
    }

    if ( dam_type == DAM_SLASH && number_percent() < get_skill(ch,gsn_cleave) )
    {
        fCleave = TRUE;
        --cap;
    }

    if ( dam_type == DAM_BASH && number_percent() < get_skill(ch,gsn_bludgeon) )
    {
        fBludgeon = TRUE;
        --cap;
    }

    if ( diceroll < cap )
        return;

    bonus = ch->level;
    if ( !fEnhance )
    {
        if ( bonus > 15 )
            bonus = ( bonus - 20 ) / 2 + 20;
    }
    else
    {
        if ( bonus > 30 )
            bonus = ( bonus - 30 ) / 2 + 30;
    }

    switch( diceroll )
    {
    case 19: break;
    case 18: bonus /= 2;  break;
    case 17: bonus /= 3;  break;
    default: bonus = 0; /* shouldn't have this */
    }

    if ( fEnhance )
        check_improve(ch,gsn_enhanced_critical,fEnhance,10);

    if ( fCleave )
        check_improve(ch,gsn_cleave,fCleave,10);

    if ( fBludgeon )
        check_improve(ch,gsn_bludgeon,fBludgeon,10);

    (*damage) += (100+bonus) * (base_dam) / 100;

    return;
}

void do_hamstring( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];

    one_argument(argument,arg);

    if ( (chance = get_skill(ch,gsn_hamstring)) == 0
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_level(ch,gsn_hamstring)))
    {
  send_to_char("Isn't hamstring a kind of sandwich?\n\r",ch);
  return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char("Hamstring whom?\n\r",ch);
   return;
    }

    if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }


    if (victim == ch)
    {
  send_to_char("That'll ruin your cross-country career.\n\r",ch);
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if (is_safe(ch,victim))
  return;
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("You can't get past the wriathform.\r\n",ch);
    return;
    }


    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( IS_NPC(victim) &&
  victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("But $N is your friend!",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel(  ch->level , victim, skill_lookup("orb of touch"));
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_hamstring].beats);
        else
            WAIT_STATE(ch,skill_table[gsn_hamstring].beats/2);
        return;
    }

    chance += ch->level;
    chance -= victim->level;
    chance -= 3*(get_curr_stat(victim,STAT_STR)+get_curr_stat(victim,STAT_CON))/2;


    /* now the attack */
    if (number_percent() < chance )
    {
        AFFECT_DATA af;

  act("$n cuts through your hamstring!  You can barely move!",
    ch,NULL,victim,TO_VICT,FALSE);
  act("You slice $N's hamstring, $E can barely move!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n slices $N's hamstring, $E can barely move!",
    ch,NULL,victim,TO_NOTVICT,FALSE);
  check_improve(ch,gsn_hamstring,TRUE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"Zoinks!  %s takes %s's legs out with a vicious slice to the hamstring!",ch->name,victim->name);
       gladiator_talk(buf);
    }

        /* strip haste */
        if ( IS_AFFECTED(victim,AFF_HASTE) )
            affect_strip(victim,skill_lookup("haste") );

        af.where        = TO_AFFECTS;
        af.type         = gsn_hamstring;
        af.duration     = number_percent() % 2;
        af.location     = APPLY_DEX;
        af.modifier     = ch->level / -12;
        af.level        = ch->level;
        af.bitvector    = AFF_SLOW;

        affect_to_char(victim,&af);

     damage(ch,victim,ch->level,gsn_hamstring,DAM_SLASH,FALSE,FALSE);
        victim->move /= 2;
        check_improve(ch,gsn_hamstring,TRUE,1);
    }
    else
   {
  check_improve(ch,gsn_hamstring,FALSE,1);
  act("You attempt to hamstring $N but fail.",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n tries to hamstring you but fails.",ch,NULL,victim,TO_VICT,FALSE);
  act("$n tries to hamstring $N but fails.",ch,NULL,victim,TO_NOTVICT,FALSE);
  damage(ch,victim,ch->level,gsn_hamstring,DAM_SLASH,FALSE,FALSE);
    }

 /* Wait states */
  if ( IS_AFFECTED(ch,AFF_SLOW) )
        WAIT_STATE(ch,PULSE_VIOLENCE*3);
  else
        WAIT_STATE(ch,PULSE_VIOLENCE*2);

  return;
}

void do_shieldbash( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];
    int sn;
    OBJ_DATA *shield;

    one_argument(argument,arg);
    sn = skill_lookup("shield bash");

    if ( (chance = get_skill(ch,sn)) == 0
    ||   (!IS_NPC(ch)
    &&    ch->level < skill_level(ch,sn)))
    {
  send_to_char("You'd probably just knock yourself out.\n\r",ch);
  return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }

    if ( (shield = get_eq_char(ch,WEAR_SHIELD)) == NULL )
    {
        send_to_char("You're not wearing a shield.\n\r",ch);
        return;
    }

    if(shield->damaged >= 100)
    {
        send_to_char("Your shield is destroyed, you can't bash with it!\n\r",ch);
        return;
    }

    if (arg[0] == '\0')
    {
        victim = ch->fighting;
        if (victim == NULL)
        {
            send_to_char("But you aren't fighting anyone!\n\r",ch);
            return;
        }
    }
    else if ((victim = get_char_room(ch,arg)) == NULL)
    {
        send_to_char("They aren't here.\n\r",ch);
        return;
    }


    if (victim == ch)
    {
  send_to_char("Not one your brightest ideas.\n\r",ch);
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
        send_to_char("You're too exhausted.\n\r",ch);
        return;
    }
    ch->move -= apply_chi(ch,(ch->move/3));

    if (is_safe(ch,victim))
  return;

    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("You can't get past the wraithform.\r\n",ch);
    return;
    }

    if (victim->fighting != NULL &&
       IS_SET(victim->mhs,MHS_HIGHLANDER) && IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Honorable combat is one on one.\n\r",ch);
       return;
    }

    if ( IS_NPC(victim) &&
  victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting))
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return;
    }

    if (IS_AFFECTED(ch,AFF_CHARM) && ch->master == victim)
    {
  act("But $N is your friend!",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

  check_killer(ch,victim);

    if( is_affected(victim,skill_lookup("orb of touch")) )
    {
        send_to_char("You bounce off an orb of touch.\n\r",ch);
        check_dispel(  ch->level , victim, skill_lookup("orb of touch"));
        if (IS_AFFECTED(ch,AFF_SLOW))
            WAIT_STATE(ch,skill_table[gsn_hamstring].beats);
        else
            WAIT_STATE(ch,skill_table[gsn_hamstring].beats/2);
        return;
    }

    chance += ch->level;
    chance -= victim->level;
    chance -= get_curr_stat(victim,STAT_STR)+get_curr_stat(victim,STAT_CON);

    if ( ch->size > victim->size )
        chance -= ( ch->size - victim->size )*3;

    /* now the attack */
    if (number_percent() < chance )
    {
        AFFECT_DATA af;

  act("$n smacks you in the face with $s shield!",
    ch,NULL,victim,TO_VICT,FALSE);
  act("You smack $N in the face with your shield!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n smacks $N in the face with $s shield!",
    ch,NULL,victim,TO_NOTVICT,FALSE);
  check_improve(ch,sn,TRUE,1);
    /* Gladiator Spectator Channel */
    if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
    {
       sprintf(buf,"*Wince*  %s clocks %s in the face with a shield!",ch->name,victim->name);
       gladiator_talk(buf);
    }

        /* strip haste */
        DAZE_STATE(victim,PULSE_VIOLENCE);

        af.where        = TO_AFFECTS;
        af.type         = sn;
        af.duration     = 0;
        af.location     = 0;
        af.modifier     = 0;
        af.level        = ch->level;
        af.bitvector    = 0;

        affect_to_char(victim,&af);

        damage(ch,victim,ch->level,sn,DAM_SLASH,FALSE,FALSE);
        victim->move /= 2;
        check_improve(ch,sn,TRUE,1);
    }
    else
   {
  check_improve(ch,sn,FALSE,1);
  act("You miss!",ch,NULL,victim,TO_CHAR,FALSE);
  act("$n tries to shield bash you but fails.",ch,NULL,victim,TO_VICT,FALSE);
  act("$n tries to shield bash $N but fails.",ch,NULL,victim,TO_NOTVICT,FALSE);
  damage(ch,victim,0,sn,DAM_SLASH,FALSE,FALSE);
    }

 /* Wait states */
  if ( IS_AFFECTED(ch,AFF_SLOW) )
        WAIT_STATE(ch,PULSE_VIOLENCE*2);
  else
        WAIT_STATE(ch,PULSE_VIOLENCE);

  return;
}

void do_grab( CHAR_DATA *ch, char *argument )
{
send_to_char("Sorry, grab is not currently available. Please try again later.\n\r", ch);
return;
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int chance;
    char buf[MAX_STRING_LENGTH];
    int sn;

    one_argument(argument,arg);
    sn = skill_lookup("grab");

    if ((victim = get_char_room(ch,arg)) == NULL)
    {
  send_to_char("They aren't here.\n\r",ch);
  return;
    }

    if ( (chance = get_skill(ch,sn)) == 0
    ||   (!IS_NPC(ch) && ch->level < skill_level(ch,sn))
    ||   ch->race != race_lookup("giant")
    ||   ch->fighting != NULL
    ||   ch == victim )
    {
  check_social( ch, "hug", "self" );
  return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\r\n",ch);
    return;
    }

    if ( ch->move < 40 )
    {
        send_to_char("You can't move that much.\n\r",ch);
        return;
     }
     ch->move -= apply_chi(ch,40);
}

