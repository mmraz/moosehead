/**************************************************************************r
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

static char rcsid[] = "$Id: handler.c,v 1.235 2004/03/27 15:51:41 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "gladiator.h"

/* command procedures needed */
DECLARE_DO_FUN(do_return  );
DECLARE_DO_FUN(do_look  );



/*
 * Local functions.
 */
void  affect_modify args( ( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, int AppType ) );
void  destruct_trade args( ( TRADE_DATA *trade, bool ifree ) );
void  add_prev_owner args( ( OBJ_DATA *obj, CHAR_DATA *ch) );
/*
 * External functions.
 */
int	nonclan_lookup	args( (const char *name) );


void prompt_pulse_command(CHAR_DATA *ch)
{
  char buf[256];
  if(IS_NPC(ch))
    return;
  switch(ch->pcdata->pulse_type)
  {
    case PULSE_RECALL:
      if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->hall)
        sprintf(buf, "<Recalling to hall in %d combat rounds>\n\r", (ch->pcdata->pulse_timer + 11) / 12);
      else
        sprintf(buf, "<Recalling in %d combat rounds>\n\r", (ch->pcdata->pulse_timer + 11) / 12);
      send_to_char(buf, ch);
      break;
    default: send_to_char("<Unknown timer>\n\r", ch); break; 
  } 
}

bool tick_pulse_command(CHAR_DATA *ch)
{
  if(IS_NPC(ch) || ch->pcdata->pulse_timer <= 0)
    return FALSE;
  switch(ch->pcdata->pulse_type)
  {
    case PULSE_RECALL:
      ch->pcdata->pulse_timer--;
      if(ch->pcdata->pulse_timer && !(ch->pcdata->pulse_timer % 12))
      {
        if(number_percent() <= get_skill(ch, gsn_recall) + number_percent() / 4)
          send_to_char("Your vision of your destination grows clearer.\n\r", ch);
        else
        {
          send_to_char("You fail to focus on your destination.\n\r", ch);
          ch->pcdata->pulse_timer += 12;
        }
      }
      break;
    default: ch->pcdata->pulse_timer--; break;
  }
  if(!ch->pcdata->pulse_timer)
  {
    end_pulse_command(ch, TRUE, FALSE);/* Success */
    return TRUE;
  }
  return FALSE;
}

void end_pulse_command(CHAR_DATA *ch, bool success, bool violent)
{
  int i;
  if(IS_NPC(ch))
    return;
  switch(ch->pcdata->pulse_type)
  {
    case PULSE_RECALL: if(success)
      {
        ROOM_INDEX_DATA *loc;
        check_improve(ch, gsn_recall, TRUE, 2);
        ch->move /= 2;
        if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->hall)
        {
          for(i = 0; i < 6; i++)
          {
            if(ch->pcdata->clan_info->clan->hall->exits[i].outside)
            {
              act("$n disappears.", ch, NULL, NULL, TO_ROOM, FALSE);
              send_to_char("You return to your hall!  You take a moment to orient yourself.\n\r", ch);
              char_from_room(ch);
              clear_mount(ch);
              char_to_room(ch, ch->pcdata->clan_info->clan->hall->exits[i].outside);
              act("$n appears in the room, $e looks briefly disoriented.", ch, NULL, NULL, TO_ROOM, FALSE);
              do_look(ch, "auto");
              WAIT_STATE(ch, PULSE_VIOLENCE);/* Slight delay on arrival */
              return;
            }
          }
        }
        loc = get_room_index(ROOM_VNUM_TEMPLE);
        if(loc == NULL)
        {
          send_to_char("You are completely lost.\n\r", ch);
          return;
        }
        act("$n disappears.",ch,NULL,NULL,TO_ROOM,FALSE);
        char_from_room(ch);
        clear_mount(ch);
        char_to_room(ch, loc);
        act("$n appears in the room.",ch,NULL,NULL,TO_ROOM,FALSE);
        do_look(ch, "auto");
        WAIT_STATE(ch, PULSE_VIOLENCE);/* Slight delay on arrival */
        return;
      }
      else
      {
        ch->pcdata->pulse_timer = 0;
        if(violent)
        {
          DAZE_STATE(ch,PULSE_VIOLENCE);
          act("You feel dazed as your focus is suddenly interrupted!",ch,NULL,NULL,TO_CHAR,FALSE);
          act("$n has been interrupted, $s eyes briefly lose focus.",ch,NULL,NULL,TO_ROOM,FALSE);
        }
        else
        {
          act("You stop focusing on recalling.",ch,NULL,NULL,TO_CHAR,FALSE);
          act("$n stops focusing so intently.",ch,NULL,NULL,TO_ROOM,FALSE);
        }
      }
      break;
    case PULSE_STEALMERIT: if(success)
      {
        if(!ch->pcdata->pulse_target)
        {
          send_to_char("They aren't here.", ch);
          return;
        }
        steal(ch, "merit", ch->pcdata->pulse_target);
      }
      else
        send_to_char("Your attempt to steal merit has been interrupted!", ch);
      return;
    default: return;
  }
}

bool damage_pulse_command(CHAR_DATA *ch)
{
  if(IS_NPC(ch))
    return TRUE;
  switch(ch->pcdata->pulse_type)
  {
    case PULSE_RECALL: return TRUE;
      break;
    default: return TRUE;
  }
}


/* Yes, this adds differently - it is simpler than an affect */
void damage_add(CHAR_DATA *ch, CHAR_DATA *victim, int amount, int duration)
{
  DAMAGE_DATA *dam_new;
  if(ch == victim)
    return;// No damage credit on yourself
  if(victim->damaged)
  {/* Check for an add rather than new allocation */
    dam_new = damage_find(victim, ch ? (ch->master ? ch->master->name : ch->name) : "");
    if(dam_new != NULL)
    {// Update the appropriate stats
      dam_new->duration = UMAX(dam_new->duration, duration);
      dam_new->damage += amount;
      return;
    }
  }
  dam_new = new_damage();
  dam_new->damage = amount;
  dam_new->duration = duration;
  dam_new->type = 0;
  if(ch)
    dam_new->source = str_dup(ch->master ? ch->master->name : ch->name);
  else
    dam_new->source = str_dup("~");
  VALIDATE(dam_new);
  dam_new->next = victim->damaged;
  victim->damaged = dam_new;
}

void damage_remove(CHAR_DATA *ch, DAMAGE_DATA *dam)
{
    if ( ch->damaged == NULL )
    {
   bug( "damage_remove: no damages.",0);
   return;
    }
  if(dam == ch->damaged)
  {
    ch->damaged = ch->damaged->next;
  }
  else
  {
    DAMAGE_DATA *prev = ch->damaged;
    while(prev->next && prev->next != dam)
      prev = prev->next;
    if(prev->next == NULL)
    {
      bug("damage_remove couldn't find damage.", 0);
      return;
    }
    prev->next = dam->next;
  }
  if(dam->source != NULL)
  {
    free_string(dam->source);
    dam->source = NULL;
  }
  free_damage(dam);
}

DAMAGE_DATA *damage_find(CHAR_DATA *victim, char *source)
{
  DAMAGE_DATA *dam;
  char *comp = source;
  if(comp == NULL)
    comp = "";
  if(victim->damaged == NULL)
    return NULL;
  dam = victim->damaged;
  for(dam = victim->damaged; dam != NULL; dam = dam->next)
  {
    if(dam->source == NULL)
    {
      if(comp[0] == '\0')
        return dam;
      continue;
    }
    if(!str_cmp(dam->source, source))
      return dam;
  }
  return NULL;
}

void damage_decrement(CHAR_DATA *ch)
{
  DAMAGE_DATA *prev = NULL;
  DAMAGE_DATA *cur = ch->damaged;
  while(cur != NULL)
  {
    cur->duration--;
    if(cur->duration == 0)
    {
      if(prev == NULL)
        ch->damaged = cur->next;
      else
        prev->next = cur->next;
      if(cur->source != NULL)
      {
        free_string(cur->source);
        cur->source = NULL;
      }
      free_damage(cur);
      if(prev == NULL)
        cur = ch->damaged;
      else
        cur = prev->next;
      continue;
    }
    prev = cur;
    cur = cur->next;
  }
}


void clear_mount( CHAR_DATA *ch )
{
    if ( ch->riding != NULL )
    {
	ch->riding->passenger = NULL;
	ch->riding = NULL;
    }

    if ( ch->passenger != NULL )
    {
	ch->passenger->riding = NULL;
	ch->passenger = NULL;
    }

    return;
}


bool is_mounted( CHAR_DATA *ch )
{
    return ( ch->riding == NULL ? FALSE : TRUE );
}

int get_sac_points( CHAR_DATA *ch, int points )
{
   	int max = 300;

	/* last minute */
	if (IS_NPC(ch))
		return 0;

	points /= 2;

	if ( ch->class == class_lookup("paladin") )
		max *= 2;

	if ( HAS_KIT(ch,"bishop") )
	{
		max += 100;
		points = 5 * points / 3;
	}

	if ( ch->pcdata->sac + points < max )
		return points;
	else
		return ( max - ch->pcdata->sac );
}

void wait_state(CHAR_DATA *ch, int npulse)
{
  if(IS_SET(ch->res_flags,RES_DELAY))
    {
     npulse /= 2;
     REMOVE_BIT(ch->res_flags,RES_DELAY);
    }

  if(is_affected(ch, skill_lookup("speed")))
    {
      if ( !is_affected(ch,gsn_arcantic_alacrity) )
      {
        npulse /= 2;
      }
    }

  ch->wait = (IS_IMMORTAL(ch) ?  0 : UMAX(ch->wait, npulse));
  if(IS_SET(ch->act, PLR_DWEEB))
  {
	// double for DWEEBs
	ch->wait *= 2;
  }
  return;
}

bool has_boat(CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    bool found = FALSE;

    if ( IS_IMMORTAL(ch) )
	found = TRUE;
    else
    {
     for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
       {
     if ( obj->item_type == ITEM_BOAT )
     {
         found = TRUE;
         break;
     }
       }
    }

    return found;
}

/* friend stuff -- for NPC's mostly */
bool is_friend(CHAR_DATA *ch,CHAR_DATA *victim)
{
    if (is_same_group(ch,victim))
  return TRUE;

    
    if (!IS_NPC(ch))
  return FALSE;

    if (!IS_NPC(victim))
    {
  if (IS_SET(ch->off_flags,ASSIST_PLAYERS))
      return TRUE;
  else
      return FALSE;
    }

    if (IS_AFFECTED(ch,AFF_CHARM))
  return FALSE;

    if (IS_SET(ch->off_flags,ASSIST_ALL))
  return TRUE;

    if (ch->group && ch->group == victim->group)
  return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_VNUM) 
    &&  ch->pIndexData == victim->pIndexData)
  return TRUE;

    if (IS_SET(ch->off_flags,ASSIST_RACE) && ch->race == victim->race)
  return TRUE;
     
    if (IS_SET(ch->off_flags,ASSIST_ALIGN)
    &&  !IS_SET(ch->act,ACT_NOALIGN) && !IS_SET(victim->act,ACT_NOALIGN)
    &&  ((IS_GOOD(ch) && IS_GOOD(victim))
    ||   (IS_EVIL(ch) && IS_EVIL(victim))
    ||   (IS_NEUTRAL(ch) && IS_NEUTRAL(victim))))
  return TRUE;

    return FALSE;
}

/* returns number of people on an object */
int count_users(OBJ_DATA *obj)
{
    CHAR_DATA *fch;
    int count = 0;

    if (obj->in_room == NULL)
  return 0;

    for (fch = obj->in_room->people; fch != NULL; fch = fch->next_in_room)
  if (fch->on == obj)
      count++;

    return count;
}
     
/* returns material number */
int material_lookup (const char *name)
{
    return 0;
}

/* returns race number */
int race_lookup (const char *name)
{
   int race;

   for ( race = 0; race_table[race].name != NULL; race++)
   {
  if (LOWER(name[0]) == LOWER(race_table[race].name[0])
  &&  !str_prefix( name,race_table[race].name))
      return race;
   }

   return 0;
} 

int liq_lookup (const char *name)
{
    int liq;

    for ( liq = 0; liq_table[liq].liq_name != NULL; liq++)
    {
  if (LOWER(name[0]) == LOWER(liq_table[liq].liq_name[0])
  && !str_prefix(name,liq_table[liq].liq_name))
      return liq;
    }

    return -1;
}

int weapon_lookup (const char *name)
{
    int type;

    for (type = 0; weapon_table[type].name != NULL; type++)
    {
  if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
  &&  !str_prefix(name,weapon_table[type].name))
      return type;
    }
 
    return -1;
}

int weapon_type (const char *name)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(weapon_table[type].name[0])
        &&  !str_prefix(name,weapon_table[type].name))
            return weapon_table[type].type;
    }
 
    return WEAPON_EXOTIC;
}


int item_lookup(const char *name)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
    {
        if (LOWER(name[0]) == LOWER(item_table[type].name[0])
        &&  !str_prefix(name,item_table[type].name))
            return item_table[type].type;
    }
 
    return -1;
}

char *item_name(int item_type)
{
    int type;

    for (type = 0; item_table[type].name != NULL; type++)
  if (item_type == item_table[type].type)
      return item_table[type].name;
    return "none";
}

char *weapon_name( int weapon_type)
{
    int type;
 
    for (type = 0; weapon_table[type].name != NULL; type++)
        if (weapon_type == weapon_table[type].type)
            return weapon_table[type].name;
    return "exotic";
}

int attack_lookup  (const char *name)
{
    int att;

    for ( att = 0; attack_table[att].name != NULL; att++)
    {
  if (LOWER(name[0]) == LOWER(attack_table[att].name[0])
  &&  !str_prefix(name,attack_table[att].name))
      return att;
    }

    return 0;
}

/* returns a flag for wiznet */
long wiznet_lookup (const char *name)
{
    int flag;

    for (flag = 0; wiznet_table[flag].name != NULL; flag++)
    {
  if (LOWER(name[0]) == LOWER(wiznet_table[flag].name[0])
  && !str_prefix(name,wiznet_table[flag].name))
      return flag;
    }

    return -1;
}

/* returns a flag for pnet */
long pnet_lookup (const char *name)
{
    int flag;

    for (flag = 0; pnet_table[flag].name != NULL; flag++)
    {
  if (LOWER(name[0]) == LOWER(pnet_table[flag].name[0])
  && !str_prefix(name,pnet_table[flag].name))
      return flag;
    }

    return -1;
}

int kit_lookup(const char *name)
{
    int kit;

    for ( kit = 0 ; kit_table[kit].name != NULL ; kit++ )
    {
	if (LOWER(name[0]) == LOWER(kit_table[kit].name[0])
	&&	!str_prefix( name,kit_table[kit].name))
		return kit;
    }

    return -1;
}

/* returns class number */
int class_lookup (const char *name)
{
   int class;
 
   for ( class = 0; class < MAX_CLASS; class++)
   {
        if (LOWER(name[0]) == LOWER(class_table[class].name[0])
        &&  !str_prefix( name,class_table[class].name))
            return class;
   }
 
   return -1;
}

int get_dam_bit(int dam)
{
  int bit;
    switch (dam)
    {
  case(DAM_BASH):   bit = IMM_BASH;   break;
  case(DAM_PIERCE): bit = IMM_PIERCE; break;
  case(DAM_SLASH):  bit = IMM_SLASH;  break;
  case(DAM_FIRE):   bit = IMM_FIRE;   break;
  case(DAM_COLD):   bit = IMM_COLD;   break;
  case(DAM_LIGHTNING):  bit = IMM_LIGHTNING;  break;
  case(DAM_ACID):   bit = IMM_ACID;   break;
  case(DAM_POISON): bit = IMM_POISON; break;
  case(DAM_NEGATIVE): bit = IMM_NEGATIVE; break;
  case(DAM_HOLY):   bit = IMM_HOLY;   break;
  case(DAM_ENERGY): bit = IMM_ENERGY; break;
  case(DAM_MENTAL): bit = IMM_MENTAL; break;
  case(DAM_DISEASE):  bit = IMM_DISEASE;  break;
  case(DAM_DROWNING): bit = IMM_DROWNING; break;
  case(DAM_LIGHT):  bit = IMM_LIGHT;  break;
  case(DAM_CHARM):  bit = IMM_CHARM;  break;
  case(DAM_SOUND):  bit = IMM_SOUND;  break;
  default:    return -1;
    }
  return bit;
}

/* for immunity, vulnerabiltiy, and resistant
   the 'globals' (magic and weapons) may be overriden
   three other cases -- wood, silver, and iron -- are checked in fight.c */

int check_immune(CHAR_DATA *ch, int dam_type)
{
    int immune, def;
    int bit;

    immune = -1;
    def = IS_NORMAL;

    if(dam_type == DAM_NEUTRAL)
      return def;

    if (dam_type == DAM_NONE)
  return immune;

    if(dam_type == DAM_VULN)
    {/* Special case! Return the highest vuln */
    /* Currently does NOT handle global weapon immunity, sorry gargoyles.  On the flipside, doesn't do global weapon vulns */
    /* Intended for attacks only, used for blind gladiators.  Use elsewhere at your own risk */
      int i;
      immune = IS_IMMUNE;/* Worst case */
      /* This would be so much easier if the vuln flags were set up in some sensible order */
      for(i = DAM_VULN - 1; i > DAM_NONE; i--)
      {
        bit = get_dam_bit(i);
        if(bit == -1)
          continue;// Bad damage type
        switch(immune)
        {
          case IS_IMMUNE: if (IS_SET(ch->res_flags,bit))
                            immune = IS_RESISTANT;
                          else if(IS_SET(ch->vuln_flags,bit))
                            return IS_VULNERABLE;/* Highest level reached. */
                          else
                            immune = IS_NORMAL;
                          break;
          case IS_RESISTANT:if(IS_SET(ch->vuln_flags,bit))
                            return IS_VULNERABLE;/* Highest level reached. */
                          else
                            immune = IS_NORMAL;
                          break;
          case IS_NORMAL: if(IS_SET(ch->vuln_flags,bit))
                            return IS_VULNERABLE;/* Highest level reached. */
                          break;
        }
      }
      return immune;
    }

    if (dam_type <= 3)
    {
  if (IS_SET(ch->imm_flags,IMM_WEAPON))
      def = IS_IMMUNE;
  else if (IS_SET(ch->res_flags,RES_WEAPON))
      def = IS_RESISTANT;
  else if (IS_SET(ch->vuln_flags,VULN_WEAPON))
      def = IS_VULNERABLE;
    }
    else /* magical attack */
    { 
  if (IS_SET(ch->imm_flags,IMM_MAGIC))
      def = IS_IMMUNE;
  else if (IS_SET(ch->res_flags,RES_MAGIC))
      def = IS_RESISTANT;
  else if (IS_SET(ch->vuln_flags,VULN_MAGIC))
      def = IS_VULNERABLE;
    }

    /* set bits to check -- VULN etc. must ALL be the same or this will fail */
    if((bit = get_dam_bit(dam_type)) == -1)
      return def;

    if (IS_SET(ch->imm_flags,bit))
  immune = IS_IMMUNE;
    else if (IS_SET(ch->res_flags,bit) && immune != IS_IMMUNE)
  immune = IS_RESISTANT;
    else if (IS_SET(ch->vuln_flags,bit))
    {
  if (immune == IS_IMMUNE)
      immune = IS_RESISTANT;
  else if (immune == IS_RESISTANT)
      immune = IS_NORMAL;
  else
      immune = IS_VULNERABLE;
    } 

    if (immune == -1)
  return def;
    else
        return immune;
}

bool is_clan(CHAR_DATA *ch)
{
  if(IS_NPC(ch))
    return FALSE;
  if(ch->pcdata->clan_info)
    return TRUE;
  return ( clan_table[ch->clan].true_clan );
}

bool is_same_clan(CHAR_DATA *ch, CHAR_DATA *victim)
{
  if(IS_NPC(ch) || IS_NPC(victim))
    return FALSE;
  if(ch->pcdata->clan_info)
  {
    if(ch->pcdata->clan_info->clan->default_clan)
      return FALSE;
    if(victim->pcdata->clan_info)
      return ch->pcdata->clan_info->clan == victim->pcdata->clan_info->clan;
    return FALSE;
  }
  else if(victim->pcdata->clan_info)
    return FALSE;

    if (clan_table[ch->clan].independent && clan_table[ch->clan].true_clan)
  return FALSE;
    else 
  return (ch->clan == victim->clan);
}

/* checks mob format */
bool is_old_mob(CHAR_DATA *ch)
{
    if (ch->pIndexData == NULL)
  return FALSE;
    else if (ch->pIndexData->new_format)
  return FALSE;
    return TRUE;
}
 
/* for returning skill information */
int get_skill(CHAR_DATA *ch, int sn)
{
    int skill;

    if (sn == -1) /* shorthand for level based skills */
    {
  skill = ch->level * 5 / 2;
    }

    else if (sn < -1 || sn > MAX_SKILL)
    {
  bug("Bad sn %d in get_skill.",sn);
  skill = 0;
    }

    else if (!IS_NPC(ch))
    {
  if (ch->level < skill_level(ch,sn) )
      skill = 0;
  else
  {
      skill = ch->pcdata->learned[sn];
	if(ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 8)
	  skill /= 2;// Trial 8: Barely remember how to use your skills
  }
    }

    else /* mobiles */
    {


        if (skill_table[sn].spell_fun != spell_null)
      skill = 40 + 2 * ch->level;

  else if (sn == gsn_sneak || sn == gsn_hide)
      skill = ch->level * 2 + 20;

        else if ((sn == gsn_dodge && IS_SET(ch->off_flags,OFF_DODGE))
  ||       (sn == gsn_parry && IS_SET(ch->off_flags,OFF_PARRY)))
      skill = ch->level * 2;

  else if (sn == gsn_shield_block)
      skill = 10 + 2 * ch->level;

  else if (sn == gsn_second_attack 
  && (IS_SET(ch->act,ACT_WARRIOR) || IS_SET(ch->act,ACT_THIEF)))
      skill = 10 + 3 * ch->level;

  else if (sn == gsn_third_attack && IS_SET(ch->act,ACT_WARRIOR))
      skill = 4 * ch->level - 40;

  else if (sn == gsn_hand_to_hand)
      skill = 40 + 2 * ch->level;

  else if (sn == gsn_trip && IS_SET(ch->off_flags,OFF_TRIP))
      skill = 10 + 3 * ch->level;

  else if (sn == gsn_bash && IS_SET(ch->off_flags,OFF_BASH))
      skill = 10 + 3 * ch->level;

  else if (sn == gsn_disarm 
       &&  (IS_SET(ch->off_flags,OFF_DISARM) 
       ||   IS_SET(ch->act,ACT_WARRIOR)
       ||   IS_SET(ch->act,ACT_THIEF)))
      skill = 20 + 3 * ch->level;

  else if (sn == gsn_berserk && IS_SET(ch->off_flags,OFF_BERSERK))
      skill = 3 * ch->level;

  else if (sn == gsn_kick)
      skill = 10 + 3 * ch->level;

  else if (sn == gsn_backstab && IS_SET(ch->act,ACT_THIEF))
      skill = 20 + 2 * ch->level;

    else if (sn == gsn_rescue)
      skill = 40 + ch->level; 

  else if (sn == gsn_recall)
      skill = 40 + ch->level;

  else if (sn == gsn_sword
  ||  sn == gsn_dagger
  ||  sn == gsn_spear
  ||  sn == gsn_mace
  ||  sn == gsn_axe
  ||  sn == gsn_flail
  ||  sn == gsn_whip
  ||  sn == gsn_polearm)
      skill = 40 + 5 * ch->level / 2;

  else 
     skill = 0;
    }

    if (ch->daze > 0)
    {
  if (skill_table[sn].spell_fun != spell_null)
      if (ch->clan == nonclan_lookup("warlock"))
	 skill = (skill/2) + (skill/10); 
      else
         skill /= 2;
  else
      skill = 2 * skill / 3;
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
  skill = 9 * skill / 10;

    if ( is_affected(ch,gsn_forget) )
	skill -= (number_range(5,UMAX(15,skill/2)));

    return URANGE(0,skill,100);
}

/* for returning weapon information */
int get_weapon_sn(CHAR_DATA *ch, bool fSecondary)
{
    OBJ_DATA *wield;
    int sn;

    if (fSecondary)
       wield = get_eq_char( ch, WEAR_SECOND );
    else
       wield = get_eq_char( ch, WEAR_WIELD );

    if (wield == NULL || wield->item_type != ITEM_WEAPON)
        sn = gsn_hand_to_hand;
    else switch (wield->value[0])
    {
        default :               sn = -1;                break;
        case(WEAPON_SWORD):     sn = gsn_sword;         break;
        case(WEAPON_DAGGER):    sn = gsn_dagger;        break;
        case(WEAPON_SPEAR):     sn = gsn_spear;         break;
        case(WEAPON_MACE):      sn = gsn_mace;          break;
        case(WEAPON_AXE):       sn = gsn_axe;           break;
        case(WEAPON_FLAIL):     sn = gsn_flail;         break;
        case(WEAPON_WHIP):      sn = gsn_whip;          break;
        case(WEAPON_POLEARM):   sn = gsn_polearm;       break;
        case(WEAPON_GAROTTE):   sn = gsn_whip;	        break;
   }
   return sn;
}

int get_weapon_skill(CHAR_DATA *ch, int sn)
{
     int skill;

     /* -1 is exotic */
    if (IS_NPC(ch))
    {
  if (sn == -1)
      skill = 2 * ch->level;
  else if (sn == gsn_hand_to_hand)
      skill = 40 + 2 * ch->level;
  else 
      skill = 40 + 5 * ch->level / 2;
    }
    
    else
    {
  if (sn == -1)
      skill = 3 * ch->level;
  else
  {
      skill = ch->pcdata->learned[sn];
      if ( sn == ch->pcdata->specialize )
		skill += skill / 4;
  }
    }

    if ( IS_NPC(ch) )
	return URANGE(0,skill,100);
    else
    return ( sn == -1 ? URANGE(0,skill,100) : URANGE(0,skill,125) );

} 


/* used to de-screw characters */
void reset_char(CHAR_DATA *ch)
{
     int loc,mod,stat;
     OBJ_DATA *obj;
     AFFECT_DATA *af;
     int i;

     if (IS_NPC(ch))
  return;

    if (ch->pcdata->perm_hit == 0 
    ||  ch->pcdata->perm_mana == 0
    ||  ch->pcdata->perm_move == 0)
//    ||  ch->pcdata->last_level == 0)
    {
    /* do a FULL reset */
  for (loc = 0; loc < MAX_WEAR; loc++)
  {
      obj = get_eq_char(ch,loc);
      if (obj == NULL)
    continue;
      if (!obj->enchanted)
      for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
      {
    mod = af->modifier;
    switch(af->location)
    {
        case APPLY_SEX: ch->sex   -= mod;
          if (ch->sex < 0 || ch->sex >2)
              ch->sex = IS_NPC(ch) ?
            0 :
            ch->pcdata->true_sex;
                  break;
        case APPLY_MANA:  ch->max_mana  -= mod;   break;
        case APPLY_HIT: ch->max_hit -= mod;   break;
        case APPLY_MOVE:  ch->max_move  -= mod;   break;
    }
      }

            for ( af = obj->affected; af != NULL; af = af->next )
            {
                mod = af->modifier;
                switch(af->location)
                {
                    case APPLY_SEX:     ch->sex         -= mod;         break;
                    case APPLY_MANA:    ch->max_mana    -= mod;         break;
                    case APPLY_HIT:     ch->max_hit     -= mod;         break;
                    case APPLY_MOVE:    ch->max_move    -= mod;         break;
                }
            }
  }
  /* now reset the permanent stats */
  ch->pcdata->perm_hit  = ch->max_hit;
  ch->pcdata->perm_mana   = ch->max_mana;
  ch->pcdata->perm_move = ch->max_move;
  ch->pcdata->last_level  = ch->played/3600;
  if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
  {
    if (ch->sex > 0 && ch->sex < 3)
            ch->pcdata->true_sex  = ch->sex;
    else
        ch->pcdata->true_sex  = 0;
  }

    }

    /* now restore the character to his/her true condition */
    for (stat = 0; stat < MAX_STATS; stat++)
  ch->mod_stat[stat] = 0;

    if (ch->pcdata->true_sex < 0 || ch->pcdata->true_sex > 2)
  ch->pcdata->true_sex = 0; 
    ch->sex   = ch->pcdata->true_sex;
    ch->max_hit   = ch->pcdata->perm_hit;
    ch->max_mana  = ch->pcdata->perm_mana;
    ch->max_move  = ch->pcdata->perm_move;
   
    for (i = 0; i < 4; i++)
      ch->armor[i]  = 100;

    ch->hitroll   = 0;
    ch->damroll   = 0;
    ch->saving_throw  = 0;
    ch->pcdata->second_hitroll   = 0;
    ch->pcdata->second_damroll   = 0;

    /* now start adding back the effects */
    for (loc = 0; loc < MAX_WEAR; loc++)
    {
        obj = get_eq_char(ch,loc);
        if (obj == NULL)
            continue;
        for (i = 0; i < 4; i++)
           ch->armor[i] -= apply_ac( obj, loc, i );

        if (!obj->enchanted)
	{
           for ( af = obj->pIndexData->affected; af != NULL; af = af->next )
           {
              mod = af->modifier;
              switch(af->location)
              {
	 	 case APPLY_REFLEX_SAVE: ch->saves[SAVE_REFLEX] += mod; break;
		 case APPLY_FORTITUDE_SAVE: ch->saves[SAVE_FORTITUDE] += mod; break;
		 case APPLY_WILLPOWER_SAVE: ch->saves[SAVE_WILLPOWER] += mod; break;
                 case APPLY_STR:   ch->mod_stat[STAT_STR]  += mod; break;
                 case APPLY_DEX:   ch->mod_stat[STAT_DEX]  += mod; break;
                 case APPLY_INT:   ch->mod_stat[STAT_INT]  += mod; break;
                 case APPLY_WIS:   ch->mod_stat[STAT_WIS]  += mod; break;
                 case APPLY_CON:   ch->mod_stat[STAT_CON]  += mod; break;
                 case APPLY_AGT:   ch->mod_stat[STAT_AGT]  += mod; break;
                 case APPLY_END:   ch->mod_stat[STAT_END]  += mod; break;
                 case APPLY_SOC:   ch->mod_stat[STAT_SOC]  += mod; break;

                 case APPLY_SEX:   ch->sex     += mod; break;
                 case APPLY_MANA:  ch->max_mana    += mod; break;
                 case APPLY_HIT:   ch->max_hit   += mod; break;
                 case APPLY_MOVE:  ch->max_move    += mod; break;

                 case APPLY_AC:    
                    for (i = 0; i < 4; i ++)
                       ch->armor[i] += mod; 
                    break;

                 case APPLY_HITROLL: 
	            if (loc == WEAR_SECOND)
                       ch->pcdata->second_hitroll   += mod; 
	            else if (loc == WEAR_WIELD) 
                       ch->hitroll   += mod; 
	            else
	            {
                       ch->pcdata->second_hitroll   += mod; 
                       ch->hitroll   += mod; 
	            }
	            break;
		    
                 case APPLY_DAMROLL: 
                    if (loc == WEAR_SECOND)
	               ch->pcdata->second_damroll   += mod;
	            else if (loc == WEAR_WIELD)  
	               ch->damroll   += mod; 
                    else
	            {
	               ch->pcdata->second_damroll   += mod;
	               ch->damroll   += mod; 
	            }
                    break;
  
                 case APPLY_SAVES:   ch->saving_throw += mod; break;
                 case APPLY_SAVING_ROD:    ch->saving_throw += mod; break;
                 case APPLY_SAVING_PETRI:  ch->saving_throw += mod; break;
                 case APPLY_SAVING_BREATH:   ch->saving_throw += mod; break;
                 case APPLY_SAVING_SPELL:  ch->saving_throw += mod; break;
                 case APPLY_SIZE:  ch->size += mod; break;
              }
           }
	}
 
        for ( af = obj->affected; af != NULL; af = af->next )
        {
           mod = af->modifier;
           switch(af->location)
           {
              case APPLY_REFLEX_SAVE: ch->saves[SAVE_REFLEX] += mod; break;
              case APPLY_FORTITUDE_SAVE: ch->saves[SAVE_FORTITUDE] += mod; break;
              case APPLY_WILLPOWER_SAVE: ch->saves[SAVE_WILLPOWER] += mod; break;

              case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
              case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
              case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
              case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
              case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
              case APPLY_AGT:	      ch->mod_stat[STAT_AGT]  += mod; break;
              case APPLY_END:         ch->mod_stat[STAT_END]  += mod; break;
              case APPLY_SOC:         ch->mod_stat[STAT_SOC]  += mod; break;
 
              case APPLY_SEX:         ch->sex                 += mod; break;
              case APPLY_MANA:        ch->max_mana            += mod; break;
              case APPLY_HIT:         ch->max_hit             += mod; break;
              case APPLY_MOVE:        ch->max_move            += mod; break;
 
              case APPLY_AC:
                 for (i = 0; i < 4; i ++)
                    ch->armor[i] += mod;
                 break;
              case APPLY_HITROLL:
                 if (loc == WEAR_SECOND)
                    ch->pcdata->second_hitroll   += mod; 
                 else if (loc == WEAR_WIELD) 
                    ch->hitroll   += mod; 
                 else
                 {
                    ch->pcdata->second_hitroll   += mod; 
                    ch->hitroll   += mod; 
                 }
                 break;
              case APPLY_DAMROLL:     
                 if (loc == WEAR_SECOND)
                    ch->pcdata->second_damroll   += mod; 
                 else if (loc == WEAR_WIELD) 
                    ch->damroll   += mod; 
                 else
                 {
                    ch->pcdata->second_damroll   += mod; 
                    ch->damroll   += mod; 
                 }
                 break;
 
              case APPLY_SAVES:         ch->saving_throw += mod; break;
              case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
              case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
              case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
              case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
           } 
        }
     }
  
    /* now add back spell effects */
    for (af = ch->affected; af != NULL; af = af->next)
    {
        mod = af->modifier;
        switch(af->location)
        {
                 case APPLY_REFLEX_SAVE: ch->saves[SAVE_REFLEX] += mod; break;
                 case APPLY_FORTITUDE_SAVE: ch->saves[SAVE_FORTITUDE] += mod; break;
                 case APPLY_WILLPOWER_SAVE: ch->saves[SAVE_WILLPOWER] += mod; break;

                case APPLY_STR:         ch->mod_stat[STAT_STR]  += mod; break;
                case APPLY_DEX:         ch->mod_stat[STAT_DEX]  += mod; break;
                case APPLY_INT:         ch->mod_stat[STAT_INT]  += mod; break;
                case APPLY_WIS:         ch->mod_stat[STAT_WIS]  += mod; break;
                case APPLY_CON:         ch->mod_stat[STAT_CON]  += mod; break;
                case APPLY_AGT:         ch->mod_stat[STAT_AGT]  += mod; break;
                case APPLY_END:         ch->mod_stat[STAT_END]  += mod; break;
                case APPLY_SOC:         ch->mod_stat[STAT_SOC]  += mod; break;
 
                case APPLY_SEX:         ch->sex                 += mod; break;
                case APPLY_MANA:        ch->max_mana            += mod; break;
                case APPLY_HIT:         ch->max_hit             += mod; break;
                case APPLY_MOVE:        ch->max_move            += mod; break;
 
                case APPLY_AC:
                    for (i = 0; i < 4; i ++)
                        ch->armor[i] += mod;
                    break;
                case APPLY_HITROLL:     
		    ch->hitroll             += mod; 
		    ch->pcdata->second_hitroll             += mod; 
		    break;
                case APPLY_DAMROLL:     
		    ch->damroll             += mod; 
		    ch->pcdata->second_damroll             += mod; 
		    break;
 
                case APPLY_SAVES:         ch->saving_throw += mod; break;
                case APPLY_SAVING_ROD:          ch->saving_throw += mod; break;
                case APPLY_SAVING_PETRI:        ch->saving_throw += mod; break;
                case APPLY_SAVING_BREATH:       ch->saving_throw += mod; break;
                case APPLY_SAVING_SPELL:        ch->saving_throw += mod; break;
		case APPLY_SIZE:	ch->size += mod; break;
        } 
    }

    /* make sure sex is RIGHT!!!! */
    if (ch->sex < 0 || ch->sex > 2)
  ch->sex = ch->pcdata->true_sex;
}

/*
 * Retrieve a character's trusted level for permission checking.
 */
int get_trust( CHAR_DATA *ch )
{  
   
    if ( (ch->desc != NULL) && (ch->desc->original != NULL) )
         
    return ch->desc->original->level;
   
    if ( (ch->trust != 0) && (IS_SET(ch->comm,COMM_TRUE_TRUST)) )
  return ch->trust;

    if ( (IS_NPC(ch)) && (ch->level >= LEVEL_HERO) )
  return LEVEL_HERO - 1;
    else 
  return ch->level;
}


/*
 * Retrieve a character's age.
 */
int get_age( CHAR_DATA *ch )
{
    return 17 + ( ch->played + (int) (current_time - ch->logon) ) / 72000;
}

/* command for retrieving stats */
int get_curr_stat( CHAR_DATA *ch, int stat )
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
  max = 25;

    else
    {
  max = pc_race_table[ch->race].max_stats[stat] + 4;

  if ( (ch->class <= class_lookup("warrior")) 
       || ( ch->class >= class_lookup("elementalist")))
  {
     if (class_table[ch->class].attr_prime == stat)
        max += 2;
  }
  else
  {
     if (class_table[class_table[ch->class].allowed[0]].attr_prime == stat
	 || class_table[class_table[ch->class].allowed[1]].attr_prime == stat) 
	max += 1;
  }

  if ( ch->race == race_lookup("human") || ch->race == race_lookup("goblin") 
	|| ch->race == race_lookup("half-elf") )
      max += 1;

  if ( ch->race == race_lookup("rockbiter") && stat == STAT_WIS )
      max += ch->level / 5;

/*
  if ( stat == STAT_DEX &&
	(  ch->race == race_lookup("rockbiter")
	|| ch->race == race_lookup("ogre")
	|| ch->race == race_lookup("giant") ))
      max += 4;
*/

  max = UMIN(max,25);
    }
  
    return URANGE(3,ch->perm_stat[stat] + ch->mod_stat[stat], max);
}

/* command for returning max training score */
int get_max_train( CHAR_DATA *ch, int stat )
{
    int max;

    if (IS_NPC(ch) || ch->level > LEVEL_IMMORTAL)
  return 25;

    max = pc_race_table[ch->race].max_stats[stat];
  if ( (ch->class <= class_lookup("warrior")) 
       || ( ch->class >= class_lookup("elementalist")))
  {
     if (class_table[ch->class].attr_prime == stat)
        max += 2;
  }
  else
  {
     if (class_table[class_table[ch->class].allowed[0]].attr_prime == stat
	 || class_table[class_table[ch->class].allowed[1]].attr_prime == stat) 
	max += 1;
  }
    if (class_table[ch->class].attr_prime == stat)
  if ( ch->race == race_lookup("human") || ch->race == race_lookup("goblin") 
	|| ch->race == race_lookup("half-elf") )
      max += 1;

    if ( stat == STAT_WIS && ch->race == race_lookup("rockbiter") )
       max += ch->level / 5;

    return UMIN(max,25);
}
   
  
/*
 * Retrieve a character's carry capacity.
 */
int can_carry_n( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
  return 10000;

    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
  return 0;
  
   if ( IS_NPC(ch) )
    return ( 10 * (MAX_WEAR +  2 * get_curr_stat(ch,STAT_DEX) + ch->level) );
   if ( ch->pcdata->logout_tracker != 0 )
    return ( 10 * (MAX_WEAR +  2 * 25 + 60));
   else
    return ( 10 * (MAX_WEAR +  2 * get_curr_stat(ch,STAT_DEX) + ch->level) );
}



/*
 * Retrieve a character's carry capacity.
 */
int can_carry_w( CHAR_DATA *ch )
{
    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
  return 10000000;

    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_PET) )
  return 0;
  if ( IS_NPC(ch) )
    return str_app[get_curr_stat(ch,STAT_STR)].carry * 10 + ch->level * 25;
  if (ch->pcdata->logout_tracker != 0 || IS_SET(ch->mhs,MHS_GLADIATOR))
    return str_app[25].carry * 10 + (60*25);
  else
    return str_app[get_curr_stat(ch,STAT_STR)].carry * 10 + ch->level * 25;
}



/*
 * See if a string is one of the names of an object.
 */
/*
bool is_name( const char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH];

    for ( ; ; )
    {
  namelist = one_argument( namelist, name );
  if ( name[0] == '\0' )
      return FALSE;
  if ( !str_cmp( str, name ))
      return TRUE;
    }
}
*/

bool is_name ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;


    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
  str = one_argument(str,part);

  if (part[0] == '\0' )
      return TRUE;

  /* check to see if this is part of namelist */
  list = namelist;
  for ( ; ; )  /* start parsing namelist */
  {
      list = one_argument(list,name);
      if (name[0] == '\0')  /* this name was not found */
    return FALSE;

      if (!str_prefix(string,name))
    return TRUE; /* full pattern match */

      if (!str_prefix(part,name))
    break;
  }
    }
}

bool is_exact_name ( char *str, char *namelist )
{
    char name[MAX_INPUT_LENGTH], part[MAX_INPUT_LENGTH];
    char *list, *string;


    string = str;
    /* we need ALL parts of string to match part of namelist */
    for ( ; ; )  /* start parsing string */
    {
  str = one_argument(str,part);

  if (part[0] == '\0' )
      return TRUE;

  /* check to see if this is part of namelist */
  list = namelist;
  for ( ; ; )  /* start parsing namelist */
  {
      list = one_argument(list,name);
      if (name[0] == '\0')  /* this name was not found */
    return FALSE;

      if (!strcmp(string,name))
    return TRUE; /* full pattern match */

      if (!strcmp(part,name))
    break;
  }
    }
}
/* enchanted stuff for eq */
void affect_enchant(OBJ_DATA *obj)
{
    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
        AFFECT_DATA *paf, *af_new;
        obj->enchanted = TRUE;

        for (paf = obj->pIndexData->affected;
             paf != NULL; paf = paf->next)
        {
      af_new = new_affect();

            af_new->next = obj->affected;
            obj->affected = af_new;
 
      af_new->where = paf->where;
            af_new->type        = UMAX(0,paf->type);
            af_new->level       = paf->level;
            af_new->duration    = paf->duration;
            af_new->location    = paf->location;
            af_new->modifier    = paf->modifier;
            af_new->bitvector   = paf->bitvector;
        }
    }
}
           

/*
 * Apply or remove an affect to a character.
 */
void affect_modify( CHAR_DATA *ch, AFFECT_DATA *paf, bool fAdd, int AppType)
{
    OBJ_DATA *wield;
    int mod,i;
 
    /* DOTs just handle damage. */
    if ( paf->where == DAMAGE_OVER_TIME )
	return;

    mod = paf->modifier;

    if ( fAdd )
    {
  switch (paf->where)
  {
  case TO_AFFECTS_EXT:
      SET_BIT(ch->affected_by_ext, paf->bitvector);
      break;
  case TO_AFFECTS:
      SET_BIT(ch->affected_by, paf->bitvector);
      break;
  case TO_IMMUNE:
      SET_BIT(ch->imm_flags,paf->bitvector);
      break;
  case TO_RESIST:
      SET_BIT(ch->res_flags,paf->bitvector);
      break;
  case TO_VULN:
      SET_BIT(ch->vuln_flags,paf->bitvector);
      break;
  }
    }
    else
    {
        switch (paf->where)
        {
        case TO_AFFECTS_EXT:
            REMOVE_BIT(ch->affected_by_ext, paf->bitvector);
            break;
        case TO_AFFECTS:
            REMOVE_BIT(ch->affected_by, paf->bitvector);
            break;
        case TO_IMMUNE:
            REMOVE_BIT(ch->imm_flags,paf->bitvector);
            break;
        case TO_RESIST:
            REMOVE_BIT(ch->res_flags,paf->bitvector);
            break;
        case TO_VULN:
            REMOVE_BIT(ch->vuln_flags,paf->bitvector);
            break;
        }
  mod = 0 - mod;
    }

    switch ( paf->location )
    {
    default:
  bug( "Affect_modify: unknown location %d.", paf->location );
  return;

    case APPLY_NONE:            break;
    case APPLY_REFLEX_SAVE: ch->saves[SAVE_REFLEX] += mod; break;
    case APPLY_FORTITUDE_SAVE: ch->saves[SAVE_FORTITUDE] += mod; break;
    case APPLY_WILLPOWER_SAVE: ch->saves[SAVE_WILLPOWER] += mod; break;

    case APPLY_STR:           ch->mod_stat[STAT_STR]  += mod; break;
    case APPLY_DEX:           ch->mod_stat[STAT_DEX]  += mod; break;
    case APPLY_INT:           ch->mod_stat[STAT_INT]  += mod; break;
    case APPLY_WIS:           ch->mod_stat[STAT_WIS]  += mod; break;
    case APPLY_CON:           ch->mod_stat[STAT_CON]  += mod; break;
    case APPLY_AGT:           ch->mod_stat[STAT_AGT]  += mod; break;
    case APPLY_END:           ch->mod_stat[STAT_END]  += mod; break;
    case APPLY_SOC:           ch->mod_stat[STAT_SOC]  += mod; break;
    case APPLY_SEX:           ch->sex     += mod; break;
    case APPLY_CLASS:           break;
    case APPLY_LEVEL:           break;
    case APPLY_AGE:           break;
    case APPLY_HEIGHT:            break;
    case APPLY_WEIGHT:            break;
    case APPLY_MANA:          ch->max_mana    += mod; break;
    case APPLY_HIT:           ch->max_hit   += mod; break;
    case APPLY_MOVE:          ch->max_move    += mod; break;
    case APPLY_GOLD:            break;
    case APPLY_EXP:           break;
    case APPLY_AC:
        for (i = 0; i < 4; i ++)
            ch->armor[i] += mod;
        break;
    case APPLY_HITROLL:       
	if (AppType == APPLY_PRIMARY || AppType == APPLY_BOTH)
	   ch->hitroll   += mod; 
	if(!IS_NPC(ch) && (AppType == APPLY_SECONDARY || AppType == APPLY_BOTH))
	   ch->pcdata->second_hitroll   += mod; 
	break;
    case APPLY_DAMROLL:       
	if (AppType == APPLY_PRIMARY || AppType == APPLY_BOTH)
	   ch->damroll   += mod; 
	if(!IS_NPC(ch) && (AppType == APPLY_SECONDARY || AppType == APPLY_BOTH))
	   ch->pcdata->second_damroll   += mod; 
	break;
    case APPLY_SAVES:   ch->saving_throw    += mod; break;
    case APPLY_SAVING_ROD:    ch->saving_throw    += mod; break;
    case APPLY_SAVING_PETRI:  ch->saving_throw    += mod; break;
    case APPLY_SAVING_BREATH: ch->saving_throw    += mod; break;
    case APPLY_SAVING_SPELL:  ch->saving_throw    += mod; break;
    case APPLY_SIZE:  ch->size    += mod; break;
    case APPLY_SPELL_AFFECT:            break;
    }

    /*
     * Check for weapon wielding.
     * Guard against recursion (for weapons with affects).
     */
/*    if ( !IS_NPC(ch) && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL
    &&   get_obj_weight(wield) > (str_app[get_curr_stat(ch,STAT_STR)].wield*10))
    {
  static int depth;

  if ( depth == 0 && ch->in_room != NULL && ch->in_room->vnum != ROOM_VNUM_LIMBO)
  {
      depth++;
      act( "You drop $p.", ch, wield, NULL, TO_CHAR ,FALSE);
      act( "$n drops $p.", ch, wield, NULL, TO_ROOM ,FALSE);
      obj_from_char( wield );
      obj_to_room( wield, ch->in_room );
      depth--;
  }
    }*/
    if ( !IS_NPC(ch) && ( wield = get_eq_char( ch, WEAR_WIELD ) ) != NULL)
    {
      if(get_obj_weight(wield) > (str_app[get_curr_stat(ch,STAT_STR)].wield*10))
      {
        static int depth;

        if (!ch->pcdata->weapon_too_heavy && depth == 0 && ch->in_room != NULL && ch->in_room->vnum != ROOM_VNUM_LIMBO)
        {
            depth++;
            ch->pcdata->weapon_too_heavy = TRUE;
            act("$p is too heavy for you to wield effectively.", ch, wield, NULL, TO_CHAR, FALSE);
            /*act( "You drop $p.", ch, wield, NULL, TO_CHAR ,FALSE);
            act( "$n drops $p.", ch, wield, NULL, TO_ROOM ,FALSE);
            obj_from_char( wield );
            obj_to_room( wield, ch->in_room );*/
            depth--;
        }
      }
      else if(ch->pcdata->weapon_too_heavy)
      {
        ch->pcdata->weapon_too_heavy = FALSE;
        act("You have the strength to wield $p again.", ch, wield, NULL, TO_CHAR, FALSE);
      }
    }


    return;
}


/* find an effect in an affect list */
AFFECT_DATA  *affect_find(AFFECT_DATA *paf, int sn)
{
    AFFECT_DATA *paf_find;
    
    for ( paf_find = paf; paf_find != NULL; paf_find = paf_find->next )
    {
        if ( paf_find->type == sn )
  return paf_find;
    }

    return NULL;
}

/* fix object affects when removing one */
void affect_check(CHAR_DATA *ch,int where,int vector)
{
    AFFECT_DATA *paf;
    OBJ_DATA *obj;

    if (where == TO_OBJECT || where == TO_WEAPON || vector == 0 || where == DAMAGE_OVER_TIME )
  return;

    for (paf = ch->affected; paf != NULL; paf = paf->next)
  if (paf->where == where && paf->bitvector == vector)
  {
      switch (where)
      {
          case TO_AFFECTS_EXT:
        SET_BIT(ch->affected_by_ext,vector);
        break;
          case TO_AFFECTS:
        SET_BIT(ch->affected_by,vector);
        break;
          case TO_IMMUNE:
        SET_BIT(ch->imm_flags,vector);   
        break;
          case TO_RESIST:
        SET_BIT(ch->res_flags,vector);
        break;
          case TO_VULN:
        SET_BIT(ch->vuln_flags,vector);
        break;
      }
      return;
  }

    for (obj = ch->carrying; obj != NULL; obj = obj->next_content)
    {
  if (obj->wear_loc == -1)
      continue;

            for (paf = obj->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS_EXT:
                        SET_BIT(ch->affected_by_ext,vector);
                        break;
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags,vector);
                  
                }
                return;
            }

        if (obj->enchanted)
      continue;

        for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
            if (paf->where == where && paf->bitvector == vector)
            {
                switch (where)
                {
                    case TO_AFFECTS_EXT:
                        SET_BIT(ch->affected_by_ext,vector);
                        break;
                    case TO_AFFECTS:
                        SET_BIT(ch->affected_by,vector);
                        break;
                    case TO_IMMUNE:
                        SET_BIT(ch->imm_flags,vector);
                        break;
                    case TO_RESIST:
                        SET_BIT(ch->res_flags,vector);
                        break;
                    case TO_VULN:
                        SET_BIT(ch->vuln_flags,vector);
                        break;
                }
                return;
            }
    }
}

/*
 * Link an affect to a room
 */
void affect_to_room( ROOM_INDEX_DATA *room, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new = *paf;   
    VALIDATE(paf_new);
    paf_new->next = room->affected;
    room->affected = paf_new;
    room->room_affects |= paf->bitvector;

    /* This may not be needed, depending on how this is handled **
    raffect_modify( room, paf_new, TRUE );  **/
    return;
}

/*
 * Ensure that only trap of a given kind is in the room
 */
bool check_trap( ROOM_INDEX_DATA *room, int trap )
{
    AFFECT_DATA *paf;

    for ( paf = room->affected ; paf != NULL ; paf = paf->next )
	if ( paf->type == gsn_trap && paf->location == trap )
		return TRUE;

    return FALSE;
}

/*
 * Give a flash affect to a char.
 */
void flash_affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new    = *paf;
    VALIDATE(paf_new);
    paf_new->next = ch->flash_affected;
    ch->flash_affected  = paf_new;

    affect_modify( ch, paf_new, TRUE, APPLY_BOTH);
    return;
}

/*
 * Give an affect to a char.
 */
void affect_to_char( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new    = *paf;
    VALIDATE(paf_new);
    paf_new->next = ch->affected;
    ch->affected  = paf_new;

    affect_modify( ch, paf_new, TRUE, APPLY_BOTH);
    return;
}

/* give an affect to an object */
void affect_to_obj(OBJ_DATA *obj, AFFECT_DATA *paf)
{
    AFFECT_DATA *paf_new;

    paf_new = new_affect();

    *paf_new    = *paf;
    VALIDATE(paf_new);
    paf_new->next = obj->affected;
    obj->affected = paf_new;

    /* apply any affect vectors to the object's extra_flags */
    if (paf->bitvector)
        switch (paf->where)
        {
        case TO_OBJECT:
          SET_BIT(obj->extra_flags,paf->bitvector);
      break;
        case TO_WEAPON:
      if (obj->item_type == ITEM_WEAPON)
          SET_BIT(obj->value[4],paf->bitvector);
      break;
        }
    

    return;
}

void raffect_remove( ROOM_INDEX_DATA *room, AFFECT_DATA *paf )
{
    AFFECT_DATA *prev;
    if ( room->affected == NULL )
    {
   bug( "RAffect_remove: no affect.",0);
   return;
    }

    if ( paf == room->affected )
    {
    room->affected = paf->next;
    }
    else
    {

    for ( prev = room->affected ; prev != NULL ; prev = prev->next )
    {
	if ( prev->next == paf )
	{
	prev->next = paf->next;
	break;
	}
    }

    if ( prev == NULL )
    {
    bug("RAffect_remove: cannot find affect.", 0);
    return;
    }
    }
    if(paf->bitvector && IS_SET(room->room_affects, paf->bitvector))
    {// Check that it's not applied by a second affect
      for ( prev = room->affected ; prev != NULL ; prev = prev->next )
      {
        if(prev != paf && prev->bitvector == paf->bitvector)
          break;
      }
      if(!prev)
        REMOVE_BIT(room->room_affects, paf->bitvector);
    }

    free_affect(paf);

    return;
}

/*
 * Remove a flash affect from a char.
 */
void flash_affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf, int AppType)
{
    int where;
    int vector;

    if ( ch->flash_affected == NULL )
    {
  bug( "Affect_remove: no affect.", 0 );
  return;
    }

    affect_modify( ch, paf, FALSE, AppType);
    where = paf->where;
    vector = paf->bitvector;

    if ( paf == ch->flash_affected )
    {
  ch->flash_affected  = paf->next;
    }
    else
    {
  AFFECT_DATA *prev;

  for ( prev = ch->flash_affected; prev != NULL; prev = prev->next )
  {
      if ( prev->next == paf )
      {
    prev->next = paf->next;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Affect_remove: cannot find paf.", 0 );
      return;
  }
    }

    if ( !IS_NPC(ch)  &&  paf->type == gsn_barbarian_rage )
	ch->pcdata->barbarian = -10;

    free_affect(paf);

    affect_check(ch,where,vector);
    return;
}

/*
 * Remove an affect from a char.
 */
void affect_remove( CHAR_DATA *ch, AFFECT_DATA *paf, int AppType)
{
    int where;
    int vector;

    if ( ch->affected == NULL )
    {
  bug( "Affect_remove: no affect.", 0 );
  return;
    }

    affect_modify( ch, paf, FALSE, AppType);
    where = paf->where;
    vector = paf->bitvector;

    if ( paf == ch->affected )
    {
  ch->affected  = paf->next;
    }
    else
    {
  AFFECT_DATA *prev;

  for ( prev = ch->affected; prev != NULL; prev = prev->next )
  {
      if ( prev->next == paf )
      {
    prev->next = paf->next;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Affect_remove: cannot find paf.", 0 );
      return;
  }
    }

    if ( !IS_NPC(ch)  &&  paf->type == gsn_barbarian_rage )
	ch->pcdata->barbarian = -10;

    free_affect(paf);

    affect_check(ch,where,vector);
    return;
}

void affect_remove_obj( OBJ_DATA *obj, AFFECT_DATA *paf)
{
    int where, vector;
    if ( obj->affected == NULL )
    {
        bug( "Affect_remove_object: no affect.", 0 );
        return;
    }

    if (obj->carried_by != NULL && obj->wear_loc != -1)
  affect_modify( obj->carried_by, paf, FALSE, APPLY_BOTH );

    where = paf->where;
    vector = paf->bitvector;

    /* remove flags from the object if needed */
    if (paf->bitvector)
  switch( paf->where)
        {
        case TO_OBJECT:
            REMOVE_BIT(obj->extra_flags,paf->bitvector);
            break;
        case TO_WEAPON:
            if (obj->item_type == ITEM_WEAPON)
                REMOVE_BIT(obj->value[4],paf->bitvector);
            break;
        }

    if ( paf == obj->affected )
    {
        obj->affected    = paf->next;
    }
    else
    {
        AFFECT_DATA *prev;

        for ( prev = obj->affected; prev != NULL; prev = prev->next )
        {
            if ( prev->next == paf )
            {
                prev->next = paf->next;
                break;
            }
        }

        if ( prev == NULL )
        {
            bug( "Affect_remove_object: cannot find paf.", 0 );
            return;
        }
    }

    free_affect(paf);

    if (obj->carried_by != NULL && obj->wear_loc != -1)
  affect_check(obj->carried_by,where,vector);
    return;
}



/*
 * Strip all affects of a given sn.
 */
void affect_strip( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    for ( paf = ch->affected; paf != NULL; paf = paf_next )
    {
  paf_next = paf->next;
  if ( paf->type == sn )
      affect_remove( ch, paf,APPLY_BOTH);
    }

    return;
}


/*
 * Return true if a char is affected by a spell.
 */
bool is_flash_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;

    for ( paf = ch->flash_affected; paf != NULL; paf = paf->next )
    {
  if ( paf->type == sn )
      return TRUE;
    }

    return FALSE;
}


/*
 * Return true if a char is affected by a spell.
 */
bool is_affected( CHAR_DATA *ch, int sn )
{
    AFFECT_DATA *paf;

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
  if ( paf->type == sn )
      return TRUE;
    }

    return FALSE;
}

bool is_room_affected ( ROOM_INDEX_DATA *room, int sn )
{
	AFFECT_DATA *paf;
	
	for ( paf = room->affected; paf!= NULL; paf = paf->next )
	{
		if (paf->type == sn )
			return TRUE;
	}
	return FALSE;
} 

/*
 * Add or enhance an affect.
 */
void affect_join( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    AFFECT_DATA *paf_old;
    bool found;

    found = FALSE;
    for ( paf_old = ch->affected; paf_old != NULL; paf_old = paf_old->next )
    {
  if ( paf_old->type == paf->type )
  {
      paf->level = (paf->level += paf_old->level) / 2;
      paf->duration += paf_old->duration;
      paf->modifier += paf_old->modifier;
      affect_remove( ch, paf_old, APPLY_BOTH);
      break;
  }
    }

    affect_to_char( ch, paf );
    return;
}



/*
 * Move a char out of a room.
 */
void char_from_room( CHAR_DATA *ch )
{
    OBJ_DATA *obj;

    ch->last_move = 0;

    if ( ch->in_room == NULL )
    {
  bug( "Char_from_room: NULL.", 0 );
  return;
    }

    if ( is_room_affected(ch->in_room, gsn_wall_fire))
           damage( ch, ch, dice(10,10), gsn_wall_fire ,DAM_FIRE,TRUE,TRUE);	

    if ( !IS_NPC(ch) )
  --ch->in_room->area->nplayer;

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
    &&   obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0
    &&   ch->in_room->light > 0 )
  --ch->in_room->light;

    if ( ch == ch->in_room->people )
    {
  ch->in_room->people = ch->next_in_room;
    }
    else
    {
  CHAR_DATA *prev;

  for ( prev = ch->in_room->people; prev; prev = prev->next_in_room )
  {
      if ( prev->next_in_room == ch )
      {
    prev->next_in_room = ch->next_in_room;
    break;
      }
  }

  if ( prev == NULL )
      bug( "Char_from_room: ch not found.", 0 );
    }

    /* If this person is involved with a trade, it needs to be completely
     * cancelled */
    if ( ch->trade != NULL )
	destruct_trade( ch->trade, TRUE );

    ch->in_room      = NULL;
    ch->next_in_room = NULL;
    ch->on       = NULL;  /* sanity check! */
    return;
}



/*
 * Move a char into a room.
 */
void char_to_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    OBJ_DATA *obj;
    AFFECT_DATA *paf, *paf_next;

    if ( pRoomIndex == NULL )
    {
  ROOM_INDEX_DATA *room;

  bug( "Char_to_room: NULL.", 0 );
  
  if ((room = get_room_index(ROOM_VNUM_TEMPLE)) != NULL)
      char_to_room(ch,room);
  
  return;
    }

    ch->in_room   = pRoomIndex;
    ch->next_in_room  = pRoomIndex->people;
    pRoomIndex->people  = ch;

    if ( !IS_NPC(ch))
    {
  if (ch->in_room->area->empty)
  {
      ch->in_room->area->empty = FALSE;
      ch->in_room->area->age = 0;
  }
  ++ch->in_room->area->nplayer;
    }

    if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
    &&   obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0 )
  ++ch->in_room->light;
  
    /*
     * No hiding.
     */
    if( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE)) 
	REMOVE_BIT( ch->affected_by, AFF_HIDE );

/**
    if (IS_AFFECTED(ch,AFF_PLAGUE))
    {
        AFFECT_DATA *af, plague;
        CHAR_DATA *vch;
        
        for ( af = ch->affected; af != NULL; af = af->next )
        {
            if (af->type == gsn_plague)
                break;
        }
        
        if (af == NULL)
        {
            REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
            return;
        }
        
        if (af->level == 1)
            return;
        
  plague.where    = TO_AFFECTS;
        plague.type     = gsn_plague;
        plague.level    = af->level - 1; 
        plague.duration   = number_range(1,2 * plague.level);
        plague.location   = APPLY_STR;
        plague.modifier   = -5;
        plague.bitvector  = AFF_PLAGUE;
        
        for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
        {
            if (!saves_spell(plague.level - 2,vch,DAM_DISEASE) 
      &&  !IS_IMMORTAL(vch) &&
              !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(6) == 0)
            {
              send_to_char("You feel hot and feverish.\n\r",vch);
              act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM,FALSE);
              affect_join(vch,&plague);
            }
        }
    }
***/

    /*
     * handle room affects 
     */
    for ( paf = pRoomIndex->affected ; paf != NULL ; paf = paf_next )
    {
	paf_next = paf->next;

	/* Affect exception handling: traps */
	if ( paf->type == gsn_trap )
	{
	  if ( is_clan(ch)  )
	      trap_effect( ch, paf );

	  continue;
        }

/***
	if ( ( IS_SET( paf->bitvector, AFF_CLANNER ) && !is_clan(ch) ) ||
	     ( !IS_SET( paf->bitvector, AFF_CLANNER ) && is_clan(ch) ) )
	     continue;

	affect_to_char( ch, paf );
 ****/
    }
	     
    return;
}



/*
 * Give an obj to a char.
 */
void obj_to_char( OBJ_DATA *obj, CHAR_DATA *ch )
{
    obj->next_content  = ch->carrying;
    ch->carrying   = obj;
    obj->carried_by  = ch;
    obj->in_room   = NULL;
    obj->in_obj    = NULL;
    ch->carry_number  += get_obj_number( obj );
    ch->carry_weight  += get_obj_weight( obj );
}


// Clear all original flags on objects when removed from a room
void clear_obj_original(OBJ_DATA *obj)
{
  OBJ_DATA *in;
  if(!obj->original)
    return;
  obj->original = FALSE;
  for(in = obj->contains; in; in = in->next_content)
    clear_obj_original(in);
}

/*
 * Take an obj from its character.
 */
void obj_from_char( OBJ_DATA *obj )
{
    CHAR_DATA *ch;

    if ( ( ch = obj->carried_by ) == NULL )
    {
  bug( "Obj_from_char: null ch.", 0 );
  return;
    }

    clear_obj_original(obj);

    if ( obj->wear_loc != WEAR_NONE )
  unequip_char( ch, obj );

    if ( ch->carrying == obj )
    {
  ch->carrying = obj->next_content;
    }
    else
    {
  OBJ_DATA *prev;

  for ( prev = ch->carrying; prev != NULL; prev = prev->next_content )
  {
      if ( prev->next_content == obj )
      {
    prev->next_content = obj->next_content;
    break;
      }
  }

  if ( prev == NULL )
      bug( "Obj_from_char: obj not in list.", 0 );
    }
 
  if (!IS_NPC(obj->carried_by)  && !IS_IMMORTAL(obj->carried_by) )
      obj->prev_owner  = str_dup(obj->carried_by->name);

    obj->carried_by  = NULL;
    obj->next_content  = NULL;
    ch->carry_number  -= get_obj_number( obj );
    ch->carry_weight  -= get_obj_weight( obj );
    if (IS_SET(obj->extra_flags, ITEM_NODROP)
      && IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
      REMOVE_BIT(obj->extra_flags2, ITEM2_TEMP_UNCURSED);
    return;
}



/*
 * Find the ac value of an obj, including position effect.
 */
int apply_ac( OBJ_DATA *obj, int iWear, int type )
{
  CHAR_DATA *ch = obj->carried_by;
  bool sam = ((ch->class == class_lookup("samurai")) ? TRUE : FALSE);
  bool dwarf = ((ch->race == race_lookup("dwarf")) ? TRUE : FALSE);

  if(ch->class == class_lookup("monk"))
    {
	if(obj->item_type == ITEM_CLOTHING)
	return (obj->level / 3);

      if ( obj->value[4] != 0 || ch->level <= 10 )
	{
        switch ( iWear )
        {
      case WEAR_HEAD: return 2 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8)+1);
      case WEAR_ARMS: return (400/UMAX(10,get_obj_weight(obj)))+(ch->level/8)+1;
      case WEAR_BODY: return 3 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8)+1);
      case WEAR_WAIST:  return (400/UMAX(10,get_obj_weight(obj)))+(ch->level/8)+1;
      case WEAR_LEGS: return 2 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8)+1);
      default:	return 1;
	}
      }
      else
      {
        switch ( iWear )
        {
      case WEAR_HEAD: return 2 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8));
      case WEAR_ARMS: return (400/UMAX(10,get_obj_weight(obj)))+(ch->level/8);
      case WEAR_BODY: return 3 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8));
      case WEAR_WAIST:  return (400/UMAX(10,get_obj_weight(obj)))+(ch->level/8);
      case WEAR_LEGS: return 2 * ((400/UMAX(10,get_obj_weight(obj)))+(ch->level/8));
      default: return 1;
	}
      }
     }
	
    if ( obj->item_type != ITEM_ARMOR )
  return 0;

  if ( obj->value[4] != 0 || ch->level <= 10 )
  {
    switch ( iWear )
    {
    case WEAR_BODY: return (dwarf?4:3) * obj->value[type];
    case WEAR_HEAD: return (dwarf?3:2) * obj->value[type];
    case WEAR_LEGS: return (dwarf?3:2) * obj->value[type];
    case WEAR_FEET: return (sam?2:1) * obj->value[type];
    case WEAR_HANDS:  return (sam?2:1) * obj->value[type];
    case WEAR_ARMS: return (sam?2:1) * obj->value[type];
    case WEAR_SHIELD: return    (dwarf?2:1) * obj->value[type];
    case WEAR_FINGER_L: return     0;
    case WEAR_FINGER_R: return     0;
    case WEAR_NECK_1: return     obj->value[type];
    case WEAR_NECK_2: return     obj->value[type];
    case WEAR_ABOUT:  return (dwarf?3:2) * obj->value[type];
    case WEAR_WAIST:  return (sam?2:1) * obj->value[type];
    case WEAR_WRIST_L:  return    (dwarf?2:1)* obj->value[type];
    case WEAR_WRIST_R:  return     (dwarf?2:1)*obj->value[type];
    case WEAR_HOLD: return     obj->value[type];
    }
  }
  else
  {
    switch ( iWear )
    {
    case WEAR_BODY: return (dwarf?4:3) * (obj->value[type] -1);
    case WEAR_HEAD: return (dwarf?3:2) * (obj->value[type] -1);
    case WEAR_LEGS: return (dwarf?3:2) * (obj->value[type] -1);
    case WEAR_FEET: return (sam?2:1) * (obj->value[type] -1);
    case WEAR_HANDS: return (sam?2:1) * (obj->value[type] -1);
    case WEAR_ARMS: return (sam?2:1) * (obj->value[type] -1);
    case WEAR_SHIELD: return     (dwarf?2:1)*(obj->value[type] -1);
    case WEAR_FINGER_L: return     0;
    case WEAR_FINGER_R: return     0;               
    case WEAR_NECK_1: return     (obj->value[type] -1);
    case WEAR_NECK_2: return     (obj->value[type] -1);
    case WEAR_ABOUT:  return (dwarf?3:2) * (obj->value[type] -1);
    case WEAR_WAIST:  return (sam?2:1) * (obj->value[type] -1);
    case WEAR_WRIST_L:  return     (dwarf?2:1)*(obj->value[type] -1);
    case WEAR_WRIST_R:  return     (dwarf?2:1)*(obj->value[type] -1);
    case WEAR_HOLD: return     (obj->value[type] -1);
    }
  }
    return 0;
}



/*
 * Find a piece of eq on a character.
 */
OBJ_DATA *get_eq_char( CHAR_DATA *ch, int iWear )
{
    OBJ_DATA *obj;

    if (ch == NULL)
  return NULL;

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if (ch == NULL)
  {
    return NULL;
  }
  if ( obj->wear_loc == iWear )
      return obj;
    }

    return NULL;
}



/*
 * Equip a char with an obj.
 */
void equip_char( CHAR_DATA *ch, OBJ_DATA *obj, int iWear )
{
    AFFECT_DATA *paf;
    ROOM_INDEX_DATA *pRoomIndex;
    int i;
    int AppType;
    char buf[MAX_STRING_LENGTH];

    AppType = APPLY_BOTH; 

    if (iWear == WEAR_SECOND)
       AppType = APPLY_SECONDARY;

    if (iWear == WEAR_WIELD)
       AppType = APPLY_PRIMARY;

    if ( get_eq_char( ch, iWear ) != NULL )
    {
  bug( "Equip_char: already equipped (%d).", iWear );
  return;
    }

    if ( ( IS_OBJ_STAT(obj, ITEM_ANTI_EVIL)    && IS_EVIL(ch)    )
    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_GOOD)    && IS_GOOD(ch)    )
    ||   ( IS_OBJ_STAT(obj, ITEM_ANTI_NEUTRAL) && IS_NEUTRAL(ch) ) )
    {
  /*
   * Thanks to Morgenes for the bug fix here!
   */
  act( "You are zapped by $p and quickly remove it.", ch, obj, NULL, TO_CHAR ,FALSE);
  act( "$n is zapped by $p and quickly remove it.",  ch, obj, NULL, TO_ROOM ,FALSE);
  obj_from_char( obj );
  obj_to_char(obj, ch);
//  obj_to_room( obj, ch->in_room );
  return;
    }

    obj->wear_loc  = iWear;

	if(obj->damaged >= 100)
		return;// Don't add bonuses from damaged gear

    for (i = 0; i < 4; i++)
      ch->armor[i]        -= apply_ac( obj, iWear,i );

    if (!obj->enchanted)
  for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      if ( paf->location != APPLY_SPELL_AFFECT )
          affect_modify( ch, paf, TRUE, AppType );
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
  if ( paf->location == APPLY_SPELL_AFFECT )
          affect_to_char ( ch, paf );
  else
      affect_modify( ch, paf, TRUE, AppType );

    if ( obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0
    &&   ch->in_room != NULL )
  ++ch->in_room->light;

    if( IS_OBJ_STAT(obj, ITEM_TELEPORT) && (obj->item_type != ITEM_ARMOR)
	&& (obj->value[0] > 0 || obj->value[1] == -1) )
      {
        pRoomIndex = get_room_index(obj->value[4]);

       if ( IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
	||   IS_AFFECTED(ch, AFF_CURSE)
        ||   is_affected(ch,gsn_morph)
	||   IS_SET(ch->act, PLR_DWEEB) )
	{
	 sprintf(buf,"%s has forsaken you.\n\r",deity_table[ch->pcdata->deity].pname);
	 send_to_char(buf,ch);
	 return;
	}

        if( pRoomIndex != NULL && !room_is_private(ch,pRoomIndex) )
         {
          if(ch->fighting != NULL)
	   {
	    if(number_percent() <= obj->level*2)
	     {
	      stop_fighting(ch,FALSE);
	      send_to_char("You lose 25 exp for leaving combat.\n\r",ch);
              gain_exp(ch,-25);
	     }
	    else
	     { return; }
	   }
          act( "$n flickers and phases.", ch, NULL, NULL, TO_ROOM ,FALSE);
          char_from_room(ch);
	  clear_mount(ch);
          char_to_room(ch,pRoomIndex);
          send_to_char("You flicker and phase.\n\r",ch);
          act("$n phases in.",ch,NULL,NULL,TO_ROOM,FALSE);
          do_look(ch,"auto");
	  obj->value[0]--;
         }
	}

    return;
}



/*
 * Unequip a char with an obj.
 */
void unequip_char( CHAR_DATA *ch, OBJ_DATA *obj )
{
    AFFECT_DATA *paf = NULL;
    AFFECT_DATA *lpaf = NULL;
    AFFECT_DATA *lpaf_next = NULL;
    int i;
    int AppType;

    AppType = APPLY_BOTH;

    if (obj->wear_loc == WEAR_SECOND)
       AppType = APPLY_SECONDARY;

    if (obj->wear_loc == WEAR_WIELD)
       AppType = APPLY_PRIMARY;

    if ( obj->wear_loc == WEAR_NONE )
    {
  bug( "Unequip_char: already unequipped.", 0 );
  return;
    }

  if(IS_SET(obj->extra_flags, ITEM_NOREMOVE)
    && IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
    REMOVE_BIT(obj->extra_flags2, ITEM2_TEMP_UNCURSED);

	if(obj->damaged >= 100)
	{
		obj->wear_loc = -1;
		return;// Don't remove bonuses from damaged gear (Already taken off)
	}

    for (i = 0; i < 4; i++)
      ch->armor[i]  += apply_ac( obj, obj->wear_loc,i );
    obj->wear_loc  = -1;


    if (!obj->enchanted)
    {
  for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
  {
      if ( paf->location == APPLY_SPELL_AFFECT )
      {
          for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
          {
        lpaf_next = lpaf->next;
        if ((lpaf->type == paf->type) &&
            (lpaf->level == paf->level) &&
            (lpaf->location == APPLY_SPELL_AFFECT))
        {
            affect_remove( ch, lpaf, AppType);
      lpaf_next = NULL;
        }
          }
      }
      else
      {
          affect_modify( ch, paf, FALSE, AppType );
    affect_check(ch,paf->where,paf->bitvector);
      }
  }
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
  if ( paf->location == APPLY_SPELL_AFFECT )
  {
      bug ( "Norm-Apply: %d", 0 );
      for ( lpaf = ch->affected; lpaf != NULL; lpaf = lpaf_next )
      {
    lpaf_next = lpaf->next;
    if ((lpaf->type == paf->type) &&
        (lpaf->level == paf->level) &&
        (lpaf->location == APPLY_SPELL_AFFECT))
    {
        bug ( "location = %d", lpaf->location );
        bug ( "type = %d", lpaf->type );
        affect_remove( ch, lpaf, AppType);
        lpaf_next = NULL;
    }
      }
  }
  else
  {
      affect_modify( ch, paf, FALSE, AppType );
      affect_check(ch,paf->where,paf->bitvector); 
  }

    if ( obj->item_type == ITEM_LIGHT
    &&   obj->value[2] != 0
    &&   ch->in_room != NULL
    &&   ch->in_room->light > 0 )
  --ch->in_room->light;

    /* Primary has been disarmed move Secondary to Primary */
    if (AppType == APPLY_PRIMARY)
    {
       if ((obj = get_eq_char(ch,WEAR_SECOND)) != NULL)
       {
 	  obj_from_char( obj );
 	  obj_to_char( obj, ch);
	  equip_char( ch, obj, WEAR_WIELD );
       }
    }

    /* readd racial affects to restore incase lost on item remove */
    ch->affected_by = ch->affected_by|race_table[ch->race].aff;

    return;
}



/*
 * Count occurrences of an obj in a list.
 */
int count_obj_list( OBJ_INDEX_DATA *pObjIndex, OBJ_DATA *list )
{
    OBJ_DATA *obj;
    int nMatch;

    nMatch = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
  if ( obj->pIndexData == pObjIndex )
      nMatch++;
    }

    return nMatch;
}



/*
 * Move an obj out of a room.
 */
void obj_from_room( OBJ_DATA *obj )
{
    ROOM_INDEX_DATA *in_room;
    CHAR_DATA *ch;

    if ( ( in_room = obj->in_room ) == NULL )
    {
  bug( "obj_from_room: NULL.", 0 );
  return;
    }

    clear_obj_original(obj);

    for (ch = in_room->people; ch != NULL; ch = ch->next_in_room)
  if (ch->on == obj)
      ch->on = NULL;

    if ( obj == in_room->contents )
    {
  in_room->contents = obj->next_content;
    }
    else
    {
  OBJ_DATA *prev;

  for ( prev = in_room->contents; prev; prev = prev->next_content )
  {
      if ( prev->next_content == obj )
      {
    prev->next_content = obj->next_content;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Obj_from_room: obj not found.", 0 );
      return;
  }
    }

    obj->in_room      = NULL;
    obj->next_content = NULL;
    return;
}



/*
 * Move an obj into a room.
 */
void obj_to_room( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex )
{
    obj->next_content   = pRoomIndex->contents;
    pRoomIndex->contents  = obj;
    obj->in_room    = pRoomIndex;
    obj->carried_by   = NULL;
    obj->in_obj     = NULL;
    return;
}



/*
 * Move an object into an object.
 */
void obj_to_obj( OBJ_DATA *obj, OBJ_DATA *obj_to )
{
    obj->next_content   = obj_to->contains;
    obj_to->contains    = obj;
    obj->in_obj     = obj_to;
    obj->in_room    = NULL;
    obj->carried_by   = NULL;
    if (obj_to->pIndexData->vnum == OBJ_VNUM_PIT)
        obj->cost = 0; 

    for ( ; obj_to != NULL; obj_to = obj_to->in_obj )
    {
  if ( obj_to->carried_by != NULL )
  {
      obj_to->carried_by->carry_number += get_obj_number( obj );
      obj_to->carried_by->carry_weight += get_obj_weight( obj )
    * WEIGHT_MULT(obj_to) / 100;
  }
    }

    return;
}



/*
 * Move an object out of an object.
 */
void obj_from_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_from;

    if ( ( obj_from = obj->in_obj ) == NULL )
    {
  bug( "Obj_from_obj: null obj_from.", 0 );
  return;
    }

    clear_obj_original(obj);

    if ( obj == obj_from->contains )
    {
  obj_from->contains = obj->next_content;
    }
    else
    {
  OBJ_DATA *prev;

  for ( prev = obj_from->contains; prev; prev = prev->next_content )
  {
      if ( prev->next_content == obj )
      {
    prev->next_content = obj->next_content;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Obj_from_obj: obj not found.", 0 );
      return;
  }
    }

    obj->next_content = NULL;
    obj->in_obj       = NULL;

    for ( ; obj_from != NULL; obj_from = obj_from->in_obj )
    {
  if ( obj_from->carried_by != NULL )
  {
      obj_from->carried_by->carry_number -= get_obj_number( obj );
      obj_from->carried_by->carry_weight -= get_obj_weight( obj ) 
    * WEIGHT_MULT(obj_from) / 100;
  }
    }

    return;
}



/*
 * Extract an obj from the world.
 */
void extract_obj( OBJ_DATA *obj )
{
    OBJ_DATA *obj_content;
    OBJ_DATA *obj_next;

    if(obj->link_name)
    {
      CHAR_DATA *owner = check_is_online(obj->link_name);
      if(owner)
        unlink_item(owner, obj);
      else
        clear_string(&obj->link_name, NULL);
    }

    if ( obj->in_room != NULL )
  obj_from_room( obj );
    else if ( obj->carried_by != NULL )
      obj_from_char( obj );
    else if ( obj->in_obj != NULL )
  obj_from_obj( obj );

    for ( obj_content = obj->contains; obj_content; obj_content = obj_next )
    {
  obj_next = obj_content->next_content;
  extract_obj( obj_content );
    }

    if ( object_list == obj )
    {
  object_list = obj->next;
    }
    else
    {
  OBJ_DATA *prev;

  for ( prev = object_list; prev != NULL; prev = prev->next )
  {
      if ( prev->next == obj )
      {
    prev->next = obj->next;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Extract_obj: obj %d not found.", obj->pIndexData->vnum );
      return;
  }
    }

    --obj->pIndexData->count;
    free_obj(obj);
    return;
}



/*
 * Extract a char from the world.
 */
void extract_char( CHAR_DATA *ch, bool fPull )
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *wch;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if ( ch->in_room == NULL )
    {
  bug( "Extract_char: NULL.", 0 );
  return;
    }
   
    if(IS_NPC(ch))
    {
      if(IS_SET(ch->affected_by_ext, AFF_EXT_SHADED))
      {
        remove_shaded_room(ch);// Will only remove its shadow
        if(ch->pIndexData->vnum == MOB_VNUM_BOUNTY_ADMIN)
        {
          char buf[255];
          sprintf(buf, "The bounty administrator has been slain in room %d",
            ch->in_room->vnum);
          log_quest_detail(buf, QUEST_UNKNOWN);
        }
      }
      if(ch->pIndexData->vnum == MOB_VNUM_INSANE_MIME ||
        ch->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
      {/* Consider adding a quest mob variable if we get a bunch of these */
        quest_handler(ch, NULL, NULL, QUEST_UNKNOWN, QSTEP_NPC_DEAD);
      }
    }
 
    nuke_pets(ch);
    ch->pet = NULL; /* just in case */

  while(ch->damaged)
    damage_remove(ch, ch->damaged);

    if ( fPull )

  die_follower( ch );
  die_ignore(ch);
    
    stop_fighting( ch, TRUE );

  if(IS_NPC(ch))// || (!IS_NPC(ch) && ch->pcdata->sac <= 0))
  {
    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
  obj_next = obj->next_content;
  extract_obj( obj );
    }
  }
    
    char_from_room( ch );
    clear_mount(ch);

    /* MM send them to Clan Hall on death */
    if ( !fPull )
    {
        if (IS_SET(ch->act, PLR_DWEEB))
           char_to_room(ch,get_room_index(ROOM_VNUM_ALTAR));
        else
        {
           if(!ch->pcdata || !ch->pcdata->clan_info)
             char_to_room(ch,get_room_index(clan_table[ch->clan].hall));
           else if(!ch->pcdata->clan_info->clan->hall || !ch->pcdata->clan_info->clan->hall->to_place)
             char_to_room(ch,get_room_index(ROOM_VNUM_MATOOK));
           else
             char_to_room(ch, (ROOM_INDEX_DATA*)ch->pcdata->clan_info->clan->hall->to_place);
        }
  return;
    }

    if(ch->pcdata && ch->pcdata->quest_count)
    {
      quest_handler(NULL, ch, NULL, QUEST_UNKNOWN, QSTEP_QUIT);
    }
   for ( d = descriptor_list; d != NULL; d = d->next )
   {
      CHAR_DATA *victm;

      victm = d->original ? d->original : d->character;

      if ( d->connected == CON_PLAYING && victm && victm->pcdata &&
           victm->pcdata->pulse_target == ch)
        victm->pcdata->pulse_target = NULL;
   }

    if ( IS_NPC(ch) )
  --ch->pIndexData->count;

    if ( ch->desc != NULL && ch->desc->original != NULL )
    {
  do_return( ch, "" );
  ch->desc = NULL;
    }

    for ( wch = char_list; wch != NULL; wch = wch->next )
    {
  if ( wch->reply == ch )
      wch->reply = NULL;
    }

    if ( ch == char_list )
    {
       char_list = ch->next;
    }
    else
    {
  CHAR_DATA *prev;

  for ( prev = char_list; prev != NULL; prev = prev->next )
  {
      if ( prev->next == ch )
      {
    prev->next = ch->next;
    break;
      }
  }

  if ( prev == NULL )
  {
      bug( "Extract_char: char not found.", 0 );
      return;
  }
    }

    if ( ch->desc != NULL )
  ch->desc->character = NULL;
    free_char( ch );
    return;
}



/*
 * Find a char in the room.
 */
CHAR_DATA *get_char_room( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *rch,*wch;
    int number;
    int count;

    if (atoi (argument))
      number = number_argument( argument, arg );
    else {
      one_argument (argument,arg);
      number = 0;
    }
    count  = 0;
    if ( !str_cmp( arg, "self" ) )
        return ch;

    wch = NULL;
    for ( rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room )
    {
        if ( !can_see( ch, rch, FALSE ))
	   continue;

	if(IS_SET(rch->mhs,MHS_SHAPEMORPHED))
	{
	  if(is_name( arg, rch->long_descr ) || is_name( arg, rch->name ) )
	  {
	    if (number)
	    {
	      if ( ++count == number )
	        return rch;
	    }
	    else
	    {
	      return rch;
	    }
	  }
	  else
	  {
	    continue;
	  }
	}

	if(IS_SET(rch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE
	       && rch != ch && !IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT))
	{//Moved the HOLYLIGHT check out to fix imm problems seeing blind glads
//	  if(!IS_NPC(ch) && !IS_SET(ch->act, PLR_HOLYLIGHT))
	  {
	    if(!is_name( arg, rch->long_descr ) )
	      continue;
	  }
        }
	else
	{
	  if(!is_name( arg, rch->name ) )
            continue;
	}

        if (number)
	{
           if ( ++count == number ) 
              return rch;
        }
	else
	{
           if (IS_NPC (rch)) return rch;
           if (!wch) wch = rch;
        }
    }

    return wch;
}

/*
 * Lookup a character by ID
 */
CHAR_DATA *get_char_by_id( long id )
{
    CHAR_DATA *ch;

    for ( ch = char_list ; ch != NULL ; ch = ch->next )
	if ( ch->id == id )
		return ch;

    return NULL;
}

/*
 * Find a char that's online, respecting wizi/incog status
 * Does not care about room except that they need to be in one
 */
CHAR_DATA *get_char_online( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;


    number = number_argument( argument, arg );
    count  = 0;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
    {
  if ( wch->in_room == NULL || !can_see( ch, wch, TRUE ) 
  ||   !is_name( arg, wch->name ) )
      continue;
  if ( ++count == number )
      return wch;
    }

    return NULL;
}

/*
 * Find a char in the world.
 */
CHAR_DATA *get_char_world( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *wch;
    int number;
    int count;

    if ( ( wch = get_char_room( ch, argument ) ) != NULL )
  return wch;

    number = number_argument( argument, arg );
    count  = 0;
    for ( wch = char_list; wch != NULL ; wch = wch->next )
    {
  if ( wch->in_room == NULL || !can_see( ch, wch, FALSE ) 
  ||   !is_name( arg, wch->name ) )
      continue;
  if ( ++count == number )
      return wch;
    }

    return NULL;
}



/*
 * Find some object with a given index data.
 * Used by area-reset 'P' command.
 */
OBJ_DATA *get_obj_type( OBJ_INDEX_DATA *pObjIndex )
{
    OBJ_DATA *obj;

    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
  if ( obj->pIndexData == pObjIndex )
      return obj;
    }

    return NULL;
}


/*
 * Find an obj in a list.
 */
OBJ_DATA *get_obj_list( CHAR_DATA *ch, char *argument, OBJ_DATA *list )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = list; obj != NULL; obj = obj->next_content )
    {
  if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    return NULL;
}


OBJ_DATA *find_obj_carry( CHAR_DATA *thief, CHAR_DATA *victim, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    
    for ( obj = victim->carrying; obj != NULL; obj = obj->next_content )
    {
  	if (        obj->wear_loc == WEAR_NONE
              &&   (can_see_obj( thief, obj ) )
              &&   is_name( arg, obj->name ) )
	      {
      		if ( ++count == number )
	  		return obj;
	      }
    }

    return NULL;
}

OBJ_DATA *find_obj_wear( CHAR_DATA *thief, CHAR_DATA *victim, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;

 for ( obj = victim->carrying; obj != NULL; obj = obj->next_content )
  {
  if (        obj->wear_loc != WEAR_NONE
    &&   (can_see_obj( thief, obj ) )
    &&   is_name( arg, obj->name ) )
      {
      if ( ++count == number )
      return obj;
      }
    }

  return NULL;
}


/*
 * Find an obj in player's inventory.
 */
OBJ_DATA *get_obj_carry( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if ( obj->wear_loc == WEAR_NONE
  &&   (can_see_obj( ch, obj ) ) 
  &&   is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    return NULL;
}



/*
 * Find an obj in player's equipment.
 */
OBJ_DATA *get_obj_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if ( obj->wear_loc > WEAR_NONE
  &&   can_see_obj( ch, obj )
  &&   is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    return NULL;
}



/*
 * Find an obj in the room or in inventory.
 */
OBJ_DATA *get_obj_here( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    int number;
    int count;
    char arg[256];

    number = number_argument( argument, arg );
    count  = 0;

  if(ch->in_room)
  {
    for ( obj = ch->in_room->contents; obj != NULL; obj = obj->next_content )
    {
  if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }
  }

  for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if ( obj->wear_loc == WEAR_NONE
  &&   (can_see_obj( ch, obj ) ) 
  &&   is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
    {
  if ( obj->wear_loc != WEAR_NONE
  &&   can_see_obj( ch, obj )
  &&   is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    return NULL;
}


/*
 * Find an obj in the world.
 */
OBJ_DATA *get_obj_world( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    if ( ( obj = get_obj_here( ch, argument ) ) != NULL )
  return obj;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
  if ( can_see_obj( ch, obj ) && is_name( arg, obj->name ) )
  {
      if ( ++count == number )
    return obj;
  }
    }

    return NULL;
}

/* deduct cost from a character */

void deduct_cost(CHAR_DATA *ch, int cost)
{
    int silver = 0, gold = 0;

    silver = UMIN(ch->silver,cost); 

    if (silver < cost)
    {
  gold = ((cost - silver + 99) / 100);
  silver = cost - 100 * gold;
    }

    ch->gold -= gold;
    ch->silver -= silver;

    if (ch->gold < 0)
    {
  bug("deduct costs: gold %d < 0",ch->gold);
  ch->gold = 0;
    }
    if (ch->silver < 0)
    {
  bug("deduct costs: silver %d < 0",ch->silver);
  ch->silver = 0;
    }
}   
/*
 * Create a 'money' obj.
 */
OBJ_DATA *create_money( int gold, int silver )
{
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    if ( gold < 0 || silver < 0 || (gold == 0 && silver == 0) )
    {
  bug( "Create_money: zero or negative money.",UMIN(gold,silver));
  gold = UMAX(1,gold);
  silver = UMAX(1,silver);
    }

    if (gold == 0 && silver == 1)
    {
  obj = create_object( get_obj_index( OBJ_VNUM_SILVER_ONE ), 0, FALSE );
    }
    else if (gold == 1 && silver == 0)
    {
  obj = create_object( get_obj_index( OBJ_VNUM_GOLD_ONE), 0, FALSE );
    }
    else if (silver == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_GOLD_SOME ), 0, FALSE );
        sprintf( buf, obj->short_descr, gold );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[1]           = gold;
        obj->cost               = gold;
  obj->weight   = gold/5;
    }
    else if (gold == 0)
    {
        obj = create_object( get_obj_index( OBJ_VNUM_SILVER_SOME ), 0, FALSE );
        sprintf( buf, obj->short_descr, silver );
        free_string( obj->short_descr );
        obj->short_descr        = str_dup( buf );
        obj->value[0]           = silver;
        obj->cost               = silver;
  obj->weight   = silver/20;
    }
 
    else
    {
  obj = create_object( get_obj_index( OBJ_VNUM_COINS ), 0, FALSE );
  sprintf( buf, obj->short_descr, silver, gold );
  free_string( obj->short_descr );
  obj->short_descr  = str_dup( buf );
  obj->value[0]   = silver;
  obj->value[1]   = gold;
  obj->cost   = 100 * gold + silver;
  obj->weight   = gold / 5 + silver / 20;
    }

    return obj;
}



/*
 * Return # of objects which an object counts as.
 * Thanks to Tony Chamberlain for the correct recursive code here.
 */
int get_obj_number( OBJ_DATA *obj )
{
    int number;

    if (obj->item_type == ITEM_CONTAINER || obj->item_type == ITEM_MONEY)
        number = 0;
    else
        number = 10;

    if ( obj->item_type == ITEM_GEM )  
	number = 5;

    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
    {

        number +=   get_obj_number( obj );
 }
    return number;
}


/*
 * Return weight of an object, including weight of contents.
 */
int get_obj_weight( OBJ_DATA *obj )
{
    int weight;
    OBJ_DATA *tobj;

    weight = obj->weight;
    for ( tobj = obj->contains; tobj != NULL; tobj = tobj->next_content )
  weight += get_obj_weight( tobj ) * WEIGHT_MULT(obj) / 100;

    return weight;
}

int get_true_weight(OBJ_DATA *obj)
{
    int weight;
 
    weight = obj->weight;
    for ( obj = obj->contains; obj != NULL; obj = obj->next_content )
        weight += get_obj_weight( obj );
 
    return weight;
}

/*
 * True if room is dark.
 */
bool room_is_dark( ROOM_INDEX_DATA *pRoomIndex )
{
    if ( pRoomIndex->light > 0 )
  return FALSE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_DARK) )
  return TRUE;

    if ( pRoomIndex->sector_type == SECT_INSIDE
    ||   pRoomIndex->sector_type == SECT_CITY )
  return FALSE;

    if ( weather_info.sunlight == SUN_SET
    ||   weather_info.sunlight == SUN_DARK )
  return TRUE;

    return FALSE;
}


bool is_room_owner(CHAR_DATA *ch, ROOM_INDEX_DATA *room)
{
    if (room->owner == NULL || room->owner[0] == '\0')
  return FALSE;
    if (IS_IMMORTAL(ch) && !is_name(ch->name,room->owner) )
	{
	 return is_name("immortal",room->owner);
	}
    return is_name(ch->name,room->owner);
}

/*
 * True if room is private.
 */
bool room_is_private( CHAR_DATA *ch,ROOM_INDEX_DATA *pRoomIndex )
{
    CHAR_DATA *rch;
    int count;


    if (pRoomIndex->owner != NULL && pRoomIndex->owner[0] != '\0')
  return TRUE;

    count = 0;
    for ( rch = pRoomIndex->people; rch != NULL; rch = rch->next_in_room )
  count++;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_PRIVATE)  
    && (count >= 2 || IS_SET(ch->mhs,MHS_HIGHLANDER)))
  return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_SOLITARY) 
    && (count >= 1 || IS_SET(ch->mhs,MHS_HIGHLANDER)))
  return TRUE;
    
    if ( IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) )
  return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_NOCLAN) 
    && (is_clan(ch) || IS_SET(ch->mhs,MHS_HIGHLANDER))
    && !IS_IMMORTAL(ch))
  return TRUE;

    if ( IS_SET(pRoomIndex->room_flags, ROOM_CLANONLY)
    && (!is_clan(ch) )
    && !IS_IMMORTAL(ch))
  return TRUE;

    return FALSE;
}

/* true if the char can go into the room w/ clan considerations */
bool is_room_clan( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if ( IS_IMMORTAL(ch) )
	return TRUE;

    if(pRoomIndex->vnum < 0)
    {
      CHAR_DATA *owner = NULL;
      if(!ch->pcdata)
      {
        if(ch->master && !IS_NPC(ch->master) &&
          (ch->master->pet == ch || ch->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN))
          owner = ch->master;
        else
          return FALSE;
      }
      else
        owner = ch;
      if(!owner->pcdata->clan_info || 
        owner->pcdata->clan_info->clan->vnum_min > abs(pRoomIndex->vnum) ||
        owner->pcdata->clan_info->clan->vnum_max < abs(pRoomIndex->vnum))
          return FALSE;
      return TRUE; 
    }

    if ( !pRoomIndex->clan )
	return TRUE;

    if ( pRoomIndex->clan == ch->clan )
	return TRUE;

    return FALSE;
}

/* visibility on a room -- for entering and exits */
bool can_see_room( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex )
{
    if (IS_SET(pRoomIndex->room_flags, ROOM_IMP_ONLY) 
    &&  get_trust(ch) < MAX_LEVEL)
  return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_GODS_ONLY)
    &&  !IS_IMMORTAL(ch))
  return FALSE;

    if (IS_SET(pRoomIndex->room_flags, ROOM_HEROES_ONLY)
    &&  !IS_IMMORTAL(ch))
  return FALSE;

    if (IS_SET(pRoomIndex->room_flags,ROOM_NEWBIES_ONLY)
    &&  ch->level > 5 && !IS_IMMORTAL(ch))
  return FALSE;

    if (IS_SET(pRoomIndex->room_flags,ROOM_NOCLAN)
    &&  (is_clan(ch) || IS_SET(ch->mhs,MHS_HIGHLANDER)) 
    && !IS_IMMORTAL(ch))
  return FALSE;
    if (IS_SET(pRoomIndex->room_flags,ROOM_CLANONLY)
    &&  (!is_clan(ch) )
    && !IS_IMMORTAL(ch))
  return FALSE;

    return TRUE;
}



/*
 * True if char can see victim.
 */
bool can_see( CHAR_DATA *ch, CHAR_DATA *victim, bool ooc )
{
    int chance,nchance;

/* RT changed so that WIZ_INVIS has levels */
    if ( ch == victim )
       return TRUE;

    if ( ooc && victim->level < 52 && (!IS_NPC(victim) || is_same_group(ch, victim)))
	return TRUE;

    if ( get_trust(ch) < victim->invis_level)
       return FALSE;

    if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
       return FALSE;

    if ( (!IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT)) 
          ||(IS_NPC(ch) && IS_IMMORTAL(ch)))
       return TRUE;

    if(victim->in_room && ch->in_room)
    {
      if((IS_SET(victim->in_room->room_affects, RAFF_SHADED) ||
        IS_SET(ch->in_room->room_affects, RAFF_SHADED)) &&
        !IS_SET(ch->affected_by_ext, AFF_EXT_SHADED))// && !IS_SET(ch->act, PLR_HOLYLIGHT))
        return FALSE;
    }

    if ( IS_AFFECTED(ch, AFF_BLIND) || (room_is_dark( ch->in_room ) && (!IS_AFFECTED(ch, AFF_INFRARED) )))
    {
      if(!IS_NPC(ch) && !IS_NPC(victim) && ch->pcdata->bypass_blind)
      {/* Special exception - you can kill a player while blind */
        if(victim->fighting == NULL && // If they're fighting you can "see" them
         (IS_AFFECTED(victim, AFF_HIDE) || IS_SET(victim->mhs,MHS_FADE)
         || IS_AFFECTED(victim, AFF_SNEAK) || IS_AFFECTED(victim, AFF_INVISIBLE)))
           return FALSE;
        return TRUE;
      }
       return FALSE;
    }

    if ( IS_NPC(ch) && IS_SET(ch->act, ACT_IS_CHANGER))
	return TRUE;
   
    if ( !IS_SET(ch->mhs, MHS_GLADIATOR) && IS_SET(victim->mhs, MHS_GLADIATOR))
        return TRUE;

    if(!IS_NPC(ch) && ch->clan == nonclan_lookup("smurf"))
    {
        if (victim->level <= ch->level)
           return TRUE;

        if (IS_SET(ch->act,PLR_KILLER) || IS_SET(ch->act,PLR_THIEF))
        {
           if(ch->level+12 >= victim->level)
              return TRUE;
        }
        else
        {
           /* Victim is a Thug or Ruffian */
           if(ch->level + (ch->trumps > 0 ? 10 : 8) >= victim->level)
              return TRUE;
        }
    }

    if ( IS_SET(victim->mhs,MHS_FADE) && !IS_NPC(victim))
    {
       nchance = get_skill(victim,gsn_fade);
       if (is_affected(ch,gsn_vision))
          nchance /= 5;

       if(number_percent() <= nchance)
          return FALSE;
       
    }

//    if ( room_is_dark( ch->in_room ) && (!IS_AFFECTED(ch, AFF_INFRARED) ) )
//       return FALSE;

    if ( IS_AFFECTED(victim, AFF_INVISIBLE)
         && !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
       return FALSE;

    /* sneaking */ 
    if (IS_AFFECTED(victim, AFF_SNEAK) && victim->fighting == NULL) 
    {
       if(!IS_AFFECTED(ch,AFF_DETECT_HIDDEN))
	  return FALSE;
	  
       chance = get_skill(victim,gsn_sneak) * 2 / 3;
       chance += get_curr_stat(victim,STAT_DEX);
       chance -= 50;//get_curr_stat(ch,STAT_INT) * 2; /* Maxed until rework */
       chance -= (ch->level - victim->level) * 3/2;
       chance += get_skill(victim, gsn_ninjitsu)/5;


       if (IS_NPC(ch))
	  chance *= 3/2;

          if( number_percent() < chance ) 
             return FALSE;
       } 
    
    if (IS_AFFECTED(victim, AFF_HIDE) && victim->fighting == NULL) 
    {
       if(!IS_AFFECTED(ch,AFF_DETECT_HIDDEN))
	  return FALSE;

       chance = get_skill(victim,gsn_hide);
       chance -= 25;//get_curr_stat(ch,STAT_INT);/* Maxed until rework */
       chance += get_curr_stat(victim,STAT_DEX);
       chance -= (ch->level - victim->level) * 2;

       if ( victim->class == class_lookup("assassin") )
          chance += get_curr_stat(victim,STAT_DEX);

          if( number_percent() <= chance ||
	      number_percent() < get_skill(victim, gsn_ninjitsu))
             return FALSE;
       }

    return TRUE;
}



/*
 * True if char can see obj.
 */
bool can_see_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    CHAR_DATA *victim;
    int cancelSN =0;
    int cure_blindSN = 0;
    int i= 0;
    bool  foundSN = FALSE; 

    if(IS_SET(ch->in_room->room_affects, RAFF_SHADED) &&
      !IS_SET(ch->affected_by_ext, AFF_EXT_SHADED) &&
      !IS_SET(ch->act, PLR_HOLYLIGHT))
    {
      if(is_name("shadow",obj->material))
        return TRUE;// You can see shadow things here! WHAT A TWIST!
      return FALSE;// Not even vision will let you see them directly
    }

    if(obj->carried_by != NULL)
    {
       victim = obj->carried_by;
       if (!IS_NPC(victim))
       {
          if ( get_trust(ch) < victim->invis_level)
             return FALSE;

          if (get_trust(ch) < victim->incog_level && ch->in_room != victim->in_room)
             return FALSE;
       }
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_HOLYLIGHT))
  return TRUE;

    if ( IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
  return FALSE;


    if ( IS_OBJ_STAT(obj,ITEM_MAGIC) && IS_OBJ_STAT(obj,ITEM_HUM)
	&& IS_OBJ_STAT(obj,ITEM_GLOW))
  return TRUE;


    if ( IS_AFFECTED(ch,AFF_BLIND) && 
 ( (obj->item_type != ITEM_POTION) && (obj->item_type != ITEM_PILL) )
       )
       {
       /* blind and item is not a pill or potion, can't see it */
             return FALSE;
       }
    
    if ( IS_AFFECTED(ch,AFF_BLIND) && 
 ( (obj->item_type == ITEM_POTION) || (obj->item_type == ITEM_PILL) )
       )
       {
           /* blind and item is pill or potion, check if cure 
	   blind or cancel on it */
           cancelSN = skill_lookup("cure blindness");
	   cure_blindSN = skill_lookup("cancellation");
           for ( i = 1; i<=3 ; i++)
	   {
	     if(obj->value[i] == cancelSN || obj->value[i] == cure_blindSN )
	       foundSN = TRUE;
	   }
	      return foundSN;
       }

    if ( ( IS_OBJ_STAT(obj,ITEM_MAGIC) && IS_OBJ_STAT(obj,ITEM_HUM) )
	|| IS_OBJ_STAT(obj,ITEM_GLOW))
  return TRUE;


    if(!IS_NPC(ch) && ch->clan != clan_lookup("smurf"))
    {
       if ( IS_SET(obj->extra_flags, ITEM_INVIS)
            && !IS_AFFECTED(ch, AFF_DETECT_INVIS) )
          return FALSE;

       if ( obj->item_type == ITEM_LIGHT && obj->value[2] != 0 )
          return TRUE;

       if ( room_is_dark( ch->in_room ) && !IS_AFFECTED(ch, AFF_INFRARED) )
          return FALSE;
    }

    return TRUE;
}


/*
 * True if char can wear obj.
 */
bool can_wear_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
    if ( IS_NPC(ch) || ch->level >= LEVEL_IMMORTAL )
	return TRUE;

    if ( obj->item_type != ITEM_ARMOR )
	return TRUE;

    if ( obj->value[4] == 0 )
	return TRUE;

    if ( (obj->value[4] -1) == ch->size )
	return TRUE;

    if ( ch->level < 11 )
	return TRUE;

    return FALSE;
}

/*
 * True if char can drop obj.
 */
bool can_drop_obj( CHAR_DATA *ch, OBJ_DATA *obj )
{
  if(obj->link_name)
    return FALSE;

    if ( !IS_SET(obj->extra_flags, ITEM_NODROP) ||
      IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
  return TRUE;

    if ( !IS_NPC(ch) && ch->level >= LEVEL_IMMORTAL )
  return TRUE;

    return FALSE;
}



/*
 * Return ascii name of an item type.
 */
char *item_type_name( OBJ_DATA *obj )
{
    return item_type_name_num (obj->item_type);
}

char *item_type_name_num (int num)
{
    switch ( num )
    {
    case ITEM_LIGHT:    return "light";
    case ITEM_SCROLL:   return "scroll";
    case ITEM_WAND:   return "wand";
    case ITEM_STAFF:    return "staff";
    case ITEM_WEAPON:   return "weapon";
    case ITEM_TREASURE:   return "treasure";
    case ITEM_ARMOR:    return "armor";
    case ITEM_CLOTHING:   return "clothing";
    case ITEM_POTION:   return "potion";
    case ITEM_FURNITURE:  return "furniture";
    case ITEM_TRASH:    return "trash";
    case ITEM_CONTAINER:  return "container";
    case ITEM_DRINK_CON:  return "drink container";
    case ITEM_KEY:    return "key";
    case ITEM_FOOD:   return "food";
    case ITEM_MONEY:    return "money";
    case ITEM_BOAT:   return "boat";
    case ITEM_CORPSE_NPC: return "npc corpse";
    case ITEM_CORPSE_PC:  return "pc corpse";
    case ITEM_FOUNTAIN:   return "fountain";
    case ITEM_PILL:   return "pill";
    case ITEM_MAP:    return "map";
    case ITEM_PORTAL:   return "portal";
    case ITEM_WARP_STONE: return "warp stone";
    case ITEM_GEM:    return "gem";
    case ITEM_JEWELRY:    return "jewelry";
    case ITEM_JUKEBOX:    return "juke box";
    case ITEM_TRAP:	return "trap part";
    case ITEM_GRENADE: return "grenade";
    case ITEM_SPELL_PAGE: return "spell page";
    case ITEM_PART: return "item part";
    case ITEM_FORGE: return "forge";
    case ITEM_CAPSULE: return "capsule";
    case 0: return "not used";
    }

    /* bug( "Item_type_name: unknown type %d.", num );  */
    return "unknown";
}




/*
 * Return ascii name of an affect location.
 */
char *affect_loc_name( int location )
{
    switch ( location )
    {
    case APPLY_NONE:    return "none";
    case APPLY_REFLEX_SAVE: 	return "reflex saves";
    case APPLY_FORTITUDE_SAVE:	return "fortitude saves";
    case APPLY_WILLPOWER_SAVE:  return "willpower saves";
    case APPLY_STR:   return "strength";
    case APPLY_DEX:   return "dexterity";
    case APPLY_INT:   return "intelligence";
    case APPLY_WIS:   return "wisdom";
    case APPLY_CON:   return "constitution";
    case APPLY_AGT:   return "agility";
    case APPLY_END:   return "endurance";
    case APPLY_SOC:   return "charisma";
    case APPLY_SEX:   return "sex";
    case APPLY_CLASS:   return "class";
    case APPLY_LEVEL:   return "level";
    case APPLY_AGE:   return "age";
    case APPLY_MANA:    return "mana";
    case APPLY_HIT:   return "hp";
    case APPLY_MOVE:    return "moves";
    case APPLY_GOLD:    return "gold";
    case APPLY_EXP:   return "experience";
    case APPLY_AC:    return "armor class";
    case APPLY_HITROLL:   return "hit roll";
    case APPLY_DAMROLL:   return "damage roll";
    case APPLY_SAVES:   return "saves";
    case APPLY_SAVING_ROD:  return "save vs rod";
    case APPLY_SAVING_PETRI:  return "save vs petrification";
    case APPLY_SAVING_BREATH: return "save vs breath";
    case APPLY_SAVING_SPELL:  return "save vs spell";
    case APPLY_SIZE:   return "size";
    case APPLY_SPELL_AFFECT:  return "none";
    }

    bug( "Affect_location_name: unknown location %d.", location );
    return "(unknown)";
}



/*
 * Return ascii name of an affect bit vector.
 */
char *affect_bit_name( int vector )
{
    static char buf[512];

    buf[0] = '\0';
    if ( vector & AFF_BLIND         ) strcat( buf, " blind"         );
    if ( vector & AFF_INVISIBLE     ) strcat( buf, " invisible"     );
    if ( vector & AFF_DETECT_ALIGN  ) strcat( buf, " detect_align"  );
    if ( vector & AFF_DETECT_INVIS  ) strcat( buf, " detect_invis"  );
    if ( vector & AFF_DETECT_MAGIC  ) strcat( buf, " detect_magic"  );
    if ( vector & AFF_DETECT_HIDDEN ) strcat( buf, " detect_hidden" );
    if ( vector & AFF_SANCTUARY     ) strcat( buf, " sanctuary"     );
    if ( vector & AFF_FAERIE_FIRE   ) strcat( buf, " faerie_fire"   );
    if ( vector & AFF_FAERIE_FOG    ) strcat( buf, " faerie_fog"    );
    if ( vector & AFF_INFRARED      ) strcat( buf, " infrared"      );
    if ( vector & AFF_CURSE         ) strcat( buf, " curse"         );
    if ( vector & AFF_POISON        ) strcat( buf, " poison"        );
    if ( vector & AFF_PROTECT_EVIL  ) strcat( buf, " prot_evil"     );
    if ( vector & AFF_PROTECT_GOOD  ) strcat( buf, " prot_good"     );
    if ( vector & AFF_SLEEP         ) strcat( buf, " sleep"         );
    if ( vector & AFF_SNEAK         ) strcat( buf, " sneak"         );
    if ( vector & AFF_HIDE          ) strcat( buf, " hide"          );
    if ( vector & AFF_CHARM         ) strcat( buf, " charm"         );
    if ( vector & AFF_FLYING        ) strcat( buf, " flying"        );
    if ( vector & AFF_PASS_DOOR     ) strcat( buf, " pass_door"     );
    if ( vector & AFF_BERSERK     ) strcat( buf, " berserk"     );
    if ( vector & AFF_CALM      ) strcat( buf, " calm"      );
    if ( vector & AFF_HASTE     ) strcat( buf, " haste"     );
    if ( vector & AFF_SLOW          ) strcat( buf, " slow"          );
    if ( vector & AFF_PLAGUE      ) strcat( buf, " plague"      );
    if ( vector & AFF_WEAPONRY   ) strcat( buf, " weaponry"   );
    if ( vector & AFF_WITHSTAND_DEATH ) strcat (buf, " withstand_death" );
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}



/*
 * Return ascii name of extra flags vector.
 */
char *extra_bit_name( int extra_flags )
{
    static char buf[512];

    buf[0] = '\0';
    if ( extra_flags & ITEM_GLOW         ) strcat( buf, " glow"         );
    if ( extra_flags & ITEM_HUM          ) strcat( buf, " hum"          );
    /*
    if ( extra_flags & ITEM_DARK         ) strcat( buf, " dark"         );
    */
    if ( extra_flags & ITEM_LOCK         ) strcat( buf, " lock"         );
    if ( extra_flags & ITEM_EVIL         ) strcat( buf, " evil"         );
    if ( extra_flags & ITEM_INVIS        ) strcat( buf, " invis"        );
    if ( extra_flags & ITEM_MAGIC        ) strcat( buf, " magic"        );
    if ( extra_flags & ITEM_NODROP       ) strcat( buf, " nodrop"       );
    if ( extra_flags & ITEM_BLESS        ) strcat( buf, " bless"        );
    if ( extra_flags & ITEM_ANTI_GOOD    ) strcat( buf, " anti-good"    );
    if ( extra_flags & ITEM_ANTI_EVIL    ) strcat( buf, " anti-evil"    );
    if ( extra_flags & ITEM_ANTI_NEUTRAL ) strcat( buf, " anti-neutral" );
    if ( extra_flags & ITEM_NOREMOVE     ) strcat( buf, " noremove"     );
    if ( extra_flags & ITEM_INVENTORY    ) strcat( buf, " inventory"    );
    if ( extra_flags & ITEM_NOPURGE  ) strcat( buf, " nopurge"  );
    if ( extra_flags & ITEM_VIS_DEATH  ) strcat( buf, " vis_death"  );
    if ( extra_flags & ITEM_ROT_DEATH  ) strcat( buf, " rot_death"  );
    if ( extra_flags & ITEM_NOLOCATE   ) strcat( buf, " no_locate"  );
    if ( extra_flags & ITEM_SELL_EXTRACT ) strcat( buf, " sell_extract" );
    if ( extra_flags & ITEM_WEAR_TIMER  ) strcat( buf, " wear_timer" );
    if ( extra_flags & ITEM_BURN_PROOF   ) strcat( buf, " burn_proof" );
    if ( extra_flags & ITEM_NOUNCURSE  ) strcat( buf, " no_uncurse" );
    if ( extra_flags & ITEM_WARPED  ) strcat( buf, " warped" );
    if ( extra_flags & ITEM_TELEPORT  ) strcat( buf, " teleport" );
    if ( extra_flags & ITEM_NOIDENTIFY  ) strcat( buf, " noidentify" );
    if ( extra_flags & ITEM_IMM_LOAD    ) strcat( buf, " immload"     );
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *extra2_bit_name( int extra_flags2 )
{
    static char buf[512];

    buf[0] = '\0';
    if ( extra_flags2 & ITEM2_TEMP_UNCURSED  ) strcat( buf, " temp-uncursed");
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *obj_spec_bit_name( int obj_spec_flags )
{
    static char buf[512];

    buf[0] = '\0';

    return ( buf[0] != '\0' ) ? buf+1 : "none";
} 


char *mhs_bit_name( int mhs_flags )
{
    static char buf[512];

    buf[0] = '\0';

   if (mhs_flags & MHS_OLD_RECLASS) 	strcat(buf, " old-reclass");
   if (mhs_flags & MHS_MUTANT  ) 	strcat(buf, " mutant");
   if (mhs_flags & MHS_HIGHLANDER  ) 	strcat(buf, " highlander");
   if (mhs_flags & MHS_SAVANT )		strcat(buf, " savant");
   if (mhs_flags & MHS_SHAPESHIFTED )	strcat(buf, " shapeshifted");
   if (mhs_flags & MHS_SHAPEMORPHED )	strcat(buf, " shapemorphed");
   if (mhs_flags & MHS_NORESCUE )	strcat(buf, " norescue");
   if (mhs_flags & MHS_PREFRESHED )	strcat(buf, " prefreshed");
   if (mhs_flags & MHS_GLADIATOR )	strcat(buf, " gladiator");
   if (mhs_flags & MHS_ELEMENTAL )      strcat(buf, " elemental");
   if (mhs_flags & MHS_BANISH    )      strcat(buf, " banished");
   if (mhs_flags & MHS_WARLOCK_ENEMY )	strcat(buf, " warlock-enemy");
   if (mhs_flags & MHS_ZEALOT_ENEMY )	strcat(buf, " zealot-enemy");
   if (mhs_flags & MHS_POSSE_ENEMY )	strcat(buf, " posse-enemy");
   if (mhs_flags & MHS_HONOR_ENEMY )	strcat(buf, " honor-enemy");
   if (mhs_flags & MHS_FULL_SAC )	strcat(buf, " full_sac");

     return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *exit_bit_name( int exit_flags )
{
    static char buf[512];

    buf[0] = '\0';

    if ( exit_flags & EX_ISDOOR )	strcat(buf, " is-door");
    if ( exit_flags & EX_CLOSED )	strcat(buf, " closed");
    if ( exit_flags & EX_LOCKED )	strcat(buf, " locked");
    if ( exit_flags & EX_PICKPROOF)	strcat(buf, " pickproof");
    if ( exit_flags & EX_NOPASS)	strcat(buf, " nopassdoor");
    if ( exit_flags & EX_EASY)		strcat(buf, " easy-pick");
    if ( exit_flags & EX_HARD)		strcat(buf, " hard-pick");
    if ( exit_flags & EX_INFURIATING)    strcat(buf, " infur-pick");
    if ( exit_flags & EX_NOCLOSE)	strcat(buf, " no-close");
    if ( exit_flags & EX_NOLOCK)	strcat(buf, " no-lock");
    if ( exit_flags & EX_CONCEALED)	strcat(buf, " concealed");
    if ( exit_flags & EX_SECRET)	strcat(buf, " secret");
    if ( exit_flags & EX_NEW_FORMAT)	strcat(buf, " NEW_FORMAT");

     return ( buf[0] != '\0' ) ? buf+1 : "none";
}

/* return ascii name of clan flags */
char *clan_bit_name ( int clan_flags )
{
   static char buf[512];

   buf[0] = '\0';

   if ( clan_flags & CLAN_NO_HALL ) strcat(buf, " no_hall");
   if ( clan_flags & CLAN_NO_HEALER) strcat(buf, " no_healer");
   if ( clan_flags & CLAN_NO_CHANNEL) strcat(buf, " no_clan_channel");
   if ( clan_flags & CLAN_NO_PORTALS) strcat(buf, " no_portal");
   if ( clan_flags & CLAN_NO_REGEN) strcat(buf, " no_regen");
   if ( clan_flags & CLAN_NO_STORE) strcat(buf, " no_store");
   if ( clan_flags & CLAN_NO_SKILL_1) strcat(buf, " no_skill");
   if ( clan_flags & CLAN_ALLOW_SANC ) strcat(buf, " allowed");
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

/* return ascii name of an act vector */
char *act_bit_name( int act_flags )
{
    static char buf[512];

    buf[0] = '\0';

    if (IS_SET(act_flags,ACT_IS_NPC))
    { 
  strcat(buf," npc");
      if (act_flags & ACT_SENTINEL  ) strcat(buf, " sentinel");
      if (act_flags & ACT_SCAVENGER ) strcat(buf, " scavenger");
  if (act_flags & ACT_AGGRESSIVE  ) strcat(buf, " aggressive");
  if (act_flags & ACT_STAY_AREA ) strcat(buf, " stay_area");
  if (act_flags & ACT_WIMPY ) strcat(buf, " wimpy");
  if (act_flags & ACT_PET   ) strcat(buf, " pet");
  if (act_flags & ACT_TRAIN ) strcat(buf, " train");
  if (act_flags & ACT_PRACTICE  ) strcat(buf, " practice");
  if (act_flags & ACT_UNDEAD  ) strcat(buf, " undead");
  if (act_flags & ACT_CLERIC  ) strcat(buf, " cleric");
  if (act_flags & ACT_MAGE  ) strcat(buf, " mage");
  if (act_flags & ACT_THIEF ) strcat(buf, " thief");
  if (act_flags & ACT_WARRIOR ) strcat(buf, " warrior");
  if (act_flags & ACT_NOALIGN ) strcat(buf, " no_align");
  if (act_flags & ACT_NOPURGE ) strcat(buf, " no_purge");
  if (act_flags & ACT_IS_HEALER ) strcat(buf, " healer");
  if (act_flags & ACT_IS_CHANGER  ) strcat(buf, " changer");
  if (act_flags & ACT_IS_ARMOURER  ) strcat(buf, " armourer");
  if (act_flags & ACT_IS_WEAPONSMITH  ) strcat(buf, " weaponsmith");
  if (act_flags & ACT_GAIN  ) strcat(buf, " skill_train");
  if (act_flags & ACT_UPDATE_ALWAYS) strcat(buf," update_always");
    }
    else
    {
  strcat(buf," player");
  if (act_flags & PLR_AUTOASSIST  ) strcat(buf, " autoassist");
  if (act_flags & PLR_RECLASS ) strcat(buf, " reclass");
  if (act_flags & PLR_CANCLAN ) strcat(buf, " canclan");
  if (act_flags & PLR_AUTOEXIT  ) strcat(buf, " autoexit");
  if (act_flags & PLR_AUTOLOOT  ) strcat(buf, " autoloot");
  if (act_flags & PLR_AUTOSAC ) strcat(buf, " autosac");
  if (act_flags & PLR_AUTOGOLD  ) strcat(buf, " autogold");
  if (act_flags & PLR_AUTOSPLIT ) strcat(buf, " autosplit");
  if (act_flags & PLR_HOLYLIGHT ) strcat(buf, " holy_light");
  if (act_flags & PLR_CANLOOT ) strcat(buf, " loot_corpse");
  if (act_flags & PLR_NOSUMMON  ) strcat(buf, " no_summon");
  if (act_flags & PLR_NOFOLLOW  ) strcat(buf, " no_follow");
  if (act_flags & PLR_FREEZE  ) strcat(buf, " frozen");
  if (act_flags & PLR_THIEF ) strcat(buf, " thief");
  if (act_flags & PLR_KILLER  ) strcat(buf, " killer");
  if (act_flags & PLR_THUG  ) strcat(buf, " thug");
    }
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *comm_bit_name(int comm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (comm_flags & COMM_QUIET   ) strcat(buf, " quiet");
    if (comm_flags & COMM_DEAF    ) strcat(buf, " deaf");
    if (comm_flags & COMM_NOWIZ   ) strcat(buf, " no_wiz");
    if (comm_flags & COMM_NOBITCH ) strcat(buf, " no_bitch");
    if (comm_flags & COMM_NOAUCTION ) strcat(buf, " no_auction");
    if (comm_flags & COMM_NOGOSSIP  ) strcat(buf, " no_gossip");
    if (comm_flags & COMM_NOOOC  ) strcat(buf, " no_ooc");
    if (comm_flags & COMM_NOQUESTION  ) strcat(buf, " no_question");
    if (comm_flags & COMM_NOMUSIC ) strcat(buf, " no_music");
    if (comm_flags & COMM_NOQUOTE ) strcat(buf, " no_quote");
    if (comm_flags & COMM_NOEMOTE ) strcat(buf, " no_emote");
    if (comm_flags & COMM_NOSHOUT ) strcat(buf, " no_shout");
    if (comm_flags & COMM_NOTELL  ) strcat(buf, " no_tell");
    if (comm_flags & COMM_NOCHANNELS  ) strcat(buf, " no_channels");
    if (comm_flags & COMM_BRIEF_MENUS) strcat (buf," brief_menus");    

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *imm_bit_name(int imm_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (imm_flags & IMM_SUMMON    ) strcat(buf, " summon");
    if (imm_flags & IMM_CHARM   ) strcat(buf, " charm");
    if (imm_flags & IMM_MAGIC   ) strcat(buf, " magic");
    if (imm_flags & IMM_WEAPON    ) strcat(buf, " weapon");
    if (imm_flags & IMM_BASH    ) strcat(buf, " blunt");
    if (imm_flags & IMM_PIERCE    ) strcat(buf, " piercing");
    if (imm_flags & IMM_SLASH   ) strcat(buf, " slashing");
    if (imm_flags & IMM_FIRE    ) strcat(buf, " fire");
    if (imm_flags & IMM_COLD    ) strcat(buf, " cold");
    if (imm_flags & IMM_LIGHTNING ) strcat(buf, " lightning");
    if (imm_flags & IMM_ACID    ) strcat(buf, " acid");
    if (imm_flags & IMM_POISON    ) strcat(buf, " poison");
    if (imm_flags & IMM_NEGATIVE  ) strcat(buf, " negative");
    if (imm_flags & IMM_HOLY    ) strcat(buf, " holy");
    if (imm_flags & IMM_ENERGY    ) strcat(buf, " energy");
    if (imm_flags & IMM_MENTAL    ) strcat(buf, " mental");
    if (imm_flags & IMM_DISEASE ) strcat(buf, " disease");
    if (imm_flags & IMM_DROWNING  ) strcat(buf, " drowning");
    if (imm_flags & IMM_LIGHT   ) strcat(buf, " light");
    if (imm_flags & IMM_IRON   ) strcat(buf, " iron");
    if (imm_flags & IMM_WOOD   ) strcat(buf, " wood");
    if (imm_flags & IMM_SILVER ) strcat(buf, " silver");
    if (imm_flags & VULN_DISTRACTION ) strcat(buf, " distraction");
    if (imm_flags & RES_DELAY ) strcat(buf, " delay");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *wear_bit_name(int wear_flags)
{
    static char buf[512];

    buf [0] = '\0';
    if (wear_flags & ITEM_TAKE    ) strcat(buf, " take");
    if (wear_flags & ITEM_WEAR_FINGER ) strcat(buf, " finger");
    if (wear_flags & ITEM_WEAR_NECK ) strcat(buf, " neck");
    if (wear_flags & ITEM_WEAR_BODY ) strcat(buf, " torso");
    if (wear_flags & ITEM_WEAR_HEAD ) strcat(buf, " head");
    if (wear_flags & ITEM_WEAR_LEGS ) strcat(buf, " legs");
    if (wear_flags & ITEM_WEAR_FEET ) strcat(buf, " feet");
    if (wear_flags & ITEM_WEAR_HANDS  ) strcat(buf, " hands");
    if (wear_flags & ITEM_WEAR_ARMS ) strcat(buf, " arms");
    if (wear_flags & ITEM_WEAR_SHIELD ) strcat(buf, " shield");
    if (wear_flags & ITEM_WEAR_ABOUT  ) strcat(buf, " body");
    if (wear_flags & ITEM_WEAR_WAIST  ) strcat(buf, " waist");
    if (wear_flags & ITEM_WEAR_WRIST  ) strcat(buf, " wrist");
    if (wear_flags & ITEM_WIELD   ) strcat(buf, " wield");
    if (wear_flags & ITEM_HOLD    ) strcat(buf, " hold");
    if (wear_flags & ITEM_WEAR_FLOAT  ) strcat(buf, " float");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *form_bit_name(int form_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (form_flags & FORM_POISON  ) strcat(buf, " poison");
    else if (form_flags & FORM_EDIBLE ) strcat(buf, " edible");
    if (form_flags & FORM_MAGICAL ) strcat(buf, " magical");
    if (form_flags & FORM_INSTANT_DECAY ) strcat(buf, " instant_rot");
    if (form_flags & FORM_OTHER   ) strcat(buf, " other");
    if (form_flags & FORM_ANIMAL  ) strcat(buf, " animal");
    if (form_flags & FORM_SENTIENT  ) strcat(buf, " sentient");
    if (form_flags & FORM_UNDEAD  ) strcat(buf, " undead");
    if (form_flags & FORM_CONSTRUCT ) strcat(buf, " construct");
    if (form_flags & FORM_MIST    ) strcat(buf, " mist");
    if (form_flags & FORM_INTANGIBLE  ) strcat(buf, " intangible");
    if (form_flags & FORM_BIPED   ) strcat(buf, " biped");
    if (form_flags & FORM_CENTAUR ) strcat(buf, " centaur");
    if (form_flags & FORM_INSECT  ) strcat(buf, " insect");
    if (form_flags & FORM_SPIDER  ) strcat(buf, " spider");
    if (form_flags & FORM_CRUSTACEAN  ) strcat(buf, " crustacean");
    if (form_flags & FORM_WORM    ) strcat(buf, " worm");
    if (form_flags & FORM_BLOB    ) strcat(buf, " blob");
    if (form_flags & FORM_MAMMAL  ) strcat(buf, " mammal");
    if (form_flags & FORM_BIRD    ) strcat(buf, " bird");
    if (form_flags & FORM_REPTILE ) strcat(buf, " reptile");
    if (form_flags & FORM_SNAKE   ) strcat(buf, " snake");
    if (form_flags & FORM_DRAGON  ) strcat(buf, " dragon");
    if (form_flags & FORM_AMPHIBIAN ) strcat(buf, " amphibian");
    if (form_flags & FORM_FISH    ) strcat(buf, " fish");
    if (form_flags & FORM_COLD_BLOOD  ) strcat(buf, " cold_blooded");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *part_bit_name(int part_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (part_flags & PART_HEAD    ) strcat(buf, " head");
    if (part_flags & PART_ARMS    ) strcat(buf, " arms");
    if (part_flags & PART_LEGS    ) strcat(buf, " legs");
    if (part_flags & PART_HEART   ) strcat(buf, " heart");
    if (part_flags & PART_BRAINS  ) strcat(buf, " brains");
    if (part_flags & PART_GUTS    ) strcat(buf, " guts");
    if (part_flags & PART_HANDS   ) strcat(buf, " hands");
    if (part_flags & PART_FEET    ) strcat(buf, " feet");
    if (part_flags & PART_FINGERS ) strcat(buf, " fingers");
    if (part_flags & PART_EAR   ) strcat(buf, " ears");
    if (part_flags & PART_EYE   ) strcat(buf, " eyes");
    if (part_flags & PART_LONG_TONGUE ) strcat(buf, " long_tongue");
    if (part_flags & PART_EYESTALKS ) strcat(buf, " eyestalks");
    if (part_flags & PART_TENTACLES ) strcat(buf, " tentacles");
    if (part_flags & PART_FINS    ) strcat(buf, " fins");
    if (part_flags & PART_WINGS   ) strcat(buf, " wings");
    if (part_flags & PART_TAIL    ) strcat(buf, " tail");
    if (part_flags & PART_CLAWS   ) strcat(buf, " claws");
    if (part_flags & PART_FANGS   ) strcat(buf, " fangs");
    if (part_flags & PART_HORNS   ) strcat(buf, " horns");
    if (part_flags & PART_SCALES  ) strcat(buf, " scales");

    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *weapon_bit_name(int weapon_flags)
{
    static char buf[512];

    buf[0] = '\0';
    if (weapon_flags & WEAPON_FLAMING ) strcat(buf, " flaming");
    if (weapon_flags & WEAPON_FROST ) strcat(buf, " frost");
    if (weapon_flags & WEAPON_VAMPIRIC  ) strcat(buf, " vampiric");
    if (weapon_flags & WEAPON_SHARP ) strcat(buf, " sharp");
    if (weapon_flags & WEAPON_VORPAL  ) strcat(buf, " vorpal");
    if (weapon_flags & WEAPON_TWO_HANDS ) strcat(buf, " two-handed");
    if (weapon_flags & WEAPON_SHOCKING  ) strcat(buf, " shocking");
    if (weapon_flags & WEAPON_POISON  ) strcat(buf, " poison");
    if (weapon_flags & WEAPON_STUN  ) strcat(buf, " stun");
    if (weapon_flags & WEAPON_HOLY ) strcat(buf, " holy");
    if (weapon_flags & WEAPON_FAVORED ) strcat(buf, " legendary");
    if (weapon_flags & WEAPON_NETHER ) strcat(buf, " nether" );
    if (weapon_flags & WEAPON_SCION ) strcat(buf, " scionic" );
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}

char *cont_bit_name( int cont_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (cont_flags & CONT_CLOSEABLE ) strcat(buf, " closable");
    if (cont_flags & CONT_PICKPROOF ) strcat(buf, " pickproof");
    if (cont_flags & CONT_CLOSED  ) strcat(buf, " closed");
    if (cont_flags & CONT_LOCKED  ) strcat(buf, " locked");

    return (buf[0] != '\0' ) ? buf+1 : "none";
}


char *off_bit_name(int off_flags)
{
    static char buf[512];

    buf[0] = '\0';

    if (off_flags & OFF_AREA_ATTACK ) strcat(buf, " area attack");
    if (off_flags & OFF_BACKSTAB  ) strcat(buf, " backstab");
    if (off_flags & OFF_BASH    ) strcat(buf, " bash");
    if (off_flags & OFF_BERSERK   ) strcat(buf, " berserk");
    if (off_flags & OFF_DISARM    ) strcat(buf, " disarm");
    if (off_flags & OFF_DODGE   ) strcat(buf, " dodge");
    if (off_flags & OFF_FADE    ) strcat(buf, " fade");
    if (off_flags & OFF_FAST    ) strcat(buf, " fast");
    if (off_flags & OFF_KICK    ) strcat(buf, " kick");
    if (off_flags & OFF_KICK_DIRT ) strcat(buf, " kick_dirt");
    if (off_flags & OFF_PARRY   ) strcat(buf, " parry");
    if (off_flags & OFF_RESCUE    ) strcat(buf, " rescue");
    if (off_flags & OFF_TAIL    ) strcat(buf, " tail");
    if (off_flags & OFF_TRIP    ) strcat(buf, " trip");
    if (off_flags & OFF_CRUSH   ) strcat(buf, " crush");
    if (off_flags & ASSIST_ALL    ) strcat(buf, " assist_all");
    if (off_flags & ASSIST_ALIGN  ) strcat(buf, " assist_align");
    if (off_flags & ASSIST_RACE   ) strcat(buf, " assist_race");
    if (off_flags & ASSIST_PLAYERS  ) strcat(buf, " assist_players");
    if (off_flags & ASSIST_GUARD  ) strcat(buf, " assist_guard");
    if (off_flags & ASSIST_VNUM   ) strcat(buf, " assist_vnum");
    if (off_flags & OFF_CHARGE   ) strcat(buf, " charge");
    if (off_flags & ASSIST_ELEMENT   ) strcat(buf, " assist_element");
    if (off_flags & OFF_BANE_TOUCH   ) strcat(buf, " bane_touch");
    return ( buf[0] != '\0' ) ? buf+1 : "none";
}


void add_prev_owner( OBJ_DATA *obj, CHAR_DATA *ch)
{

 char buf[MAX_STRING_LENGTH];
 char buf2[MAX_STRING_LENGTH];
 char buf3[MAX_STRING_LENGTH];
 int count = 0, i;
 int n = 0;
 bool sCopy = FALSE;

 /* grab the current list of previous owners */

   strcpy(buf, obj->prev_owner);
   log_string(buf);

 /* count the number of names in the list */

 for ( i = 0; i <=strlen(buf); i++)
 {
   if (isspace(buf[i]))
     count++;
 }
 
 /* if we don't have 5 names yet, just tack on the last name, and return */
 if ( count < 5)
 {
 strcat(buf, ch->name);
 strcat(buf, " ");
 obj->prev_owner = strdup(buf);
 sprintf(buf3, "count: %d  buf: %s", count, buf);
 log_string(buf3);
 return;
 }
 /* now comes the hard part, removed the first name, and tack on the new one */
 else
 {
 for (i=0; i<=strlen(buf) ; i++) 
 {
 if ( isspace(buf[i]) && !sCopy )
    sCopy = TRUE;
 
 if (sCopy)
 {
  buf2[n++] = buf[i];
 }
 }
 obj->prev_owner = strdup(buf2);
 sprintf(buf3, "count: %d  buf: %s", count, buf);
 log_string(buf3);
 }
 return;
}

bool check_hai_ruki( CHAR_DATA *ch )
{
    OBJ_DATA *sword;

    if ((sword=get_eq_char(ch,WEAR_WIELD))!=NULL &&
         sword->value[0] == WEAPON_SWORD &&
             number_percent() > get_skill(ch,skill_lookup("hai-ruki")))
	return TRUE;

    return FALSE;
}

int count_groupies_in_room( CHAR_DATA *ch )
{
    CHAR_DATA *v;
     int count=0;

    for ( v = ch->in_room->people ; v != NULL ; v = v->next_in_room )
	if ( is_same_group(ch,v) || v == ch )
		++count;

    return count;
}

bool shogun_in_group( CHAR_DATA *ch )
{
    return FALSE;

    /* CHAR_DATA *v;

    for ( v = ch->in_room->people ; v != NULL ; v = v->next_in_room )
        if ( is_same_group(ch,v) && HAS_KIT(v,"shogun") )
	return TRUE;

    return FALSE;
    */
}

int smurf_group_count (CHAR_DATA *ch )
{
    CHAR_DATA *v;
    int count = 0;

    for ( v = ch->in_room->people ; v != NULL ; v = v->next_in_room )
        if ( v->race == race_lookup("smurf") &&
	   ( is_same_group(ch,v) || v == ch ))
		++count;

    return count;
}

int room_has_medium( CHAR_DATA *ch )
{
    CHAR_DATA *vch;

    if ( ch == NULL || ch->in_room == NULL )
	return 0;

    for ( vch = ch->in_room->people ; vch != NULL ; vch = vch->next_in_room )
	if ( !IS_NPC(vch) && HAS_KIT(vch,"medium") )
	    return vch->level;

    return 0;
}

int count_fight_size(CHAR_DATA *ch)
{
  int count = 0;
  CHAR_DATA *vch;

  for (vch = ch->in_room->people ; vch != NULL; vch = vch->next_in_room )
  {

       if (vch->fighting == ch)
	 ++count;
  }
  
  return count;

}
  
