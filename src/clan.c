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

static char rcsid[] = "$Id: live_edit.c,v 1.0 2011/08/19 18:33:00 mud Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <gc.h>
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"
#include "tables.h"

#define START_OBJ(ch, hedit) (hedit ? (ch)->pcdata->clan_info->clan->planned : (ch)->pcdata->clan_info->pers_plan)

CLAN_DATA *clan_first = NULL;

void add_clan_skill(CHAR_DATA *ch, int sn)
{
  if(ch->pcdata->old_learned[sn] > 0)
  {
    ch->pcdata->learned[sn] = UMIN(100, ch->pcdata->old_learned[sn]);
    ch->pcdata->old_learned[sn] = 0;
  }
  else
    ch->pcdata->learned[sn] = 1;
}

void remove_clan_skill(CHAR_DATA *ch, int sn)
{
  if(ch->pcdata->learned[sn])
    ch->pcdata->old_learned[sn] = ch->pcdata->learned[sn];
  ch->pcdata->learned[sn] = 0;
}

void remove_all_clan_skills(CHAR_DATA *ch)
{
  remove_clan_skill(ch, gsn_hemorrhage);
  remove_clan_skill(ch, gsn_convert);
  remove_clan_skill(ch, gsn_guardian);
  remove_clan_skill(ch, gsn_annointment);
  affect_strip(ch, gsn_annointment); /* No carrying it around */
  if(ch->pcdata->quest_count)
  {/* They may have a guardian, kill it */
    CHAR_DATA *check, *check_next;
    for(check = char_list; check; check = check_next)
    {
      check_next = check->next;
      if(IS_NPC(check) && check->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN &&
        check->qchar == ch)
      {
        release_clan_guardian(check);
      }
    }
  }
}

void check_link_count(CHAR_DATA *ch)
{
  int i, limit;
  for(i = 0; i < LINK_MAX; i++)
  {
    if(ch->pcdata->linked[i] == NULL)
      break;
  }
  i--;
  if(i >= (limit = get_link_limit(ch)))
  {/* Unlink items */
    send_to_char("You are over the allowed link limit, your newest linked items will be unlinked.\n\r", ch);
    for(; i > limit; i--)
    {
      clear_string(&ch->pcdata->linked[i]->link_name, NULL);
      ch->pcdata->linked[i] = NULL;
    }
  }
}

void set_clan_skills(CHAR_DATA *ch)
{
  CLAN_DATA *clan;
  if(!ch->pcdata)
    return;
  /* Capture if they left a peace or chaos clan with linking bonuses */
  check_link_count(ch);

  /* Start by yanking them all, they'll be restored as appropriate */
  remove_all_clan_skills(ch);

  if(!ch->pcdata->clan_info)
    return;

  clan = ch->pcdata->clan_info->clan;
  if(clan->default_clan || !clan->type)
    return;

  if(clan->tribute > 0 &&
    ((clan->type != CLAN_TYPE_CHAOS && clan->max_tribute >= 2000000)
    || (clan->type == CLAN_TYPE_CHAOS && clan->max_tribute >= 6000000 && clan->enemy)))
    {/* They get a basic skill */
      int value = clan->type == CLAN_TYPE_CHAOS ? clan->enemy : clan->type;
      switch(value)
      {
        case CLAN_TYPE_LAW: add_clan_skill(ch, gsn_guardian); break;
        case CLAN_TYPE_FAITH: add_clan_skill(ch, gsn_annointment); break;
        case CLAN_TYPE_MALICE: add_clan_skill(ch, gsn_hemorrhage); break;
        case CLAN_TYPE_GREED: add_clan_skill(ch, gsn_convert); break;
      }
    }
  /* Other bought clan skills here */
}

bool notify_clan_char(char *string, CLAN_CHAR *target, bool offline)
{
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  if(!string || string[0] == '\0' || !target)
    return FALSE;
  for(d = descriptor_list; d != NULL; d = d->next)
  {
    if((victim = d->character) == NULL || d->connected != CON_PLAYING)
      continue;
    if(!str_cmp(victim->name, target->name))
    {
      send_to_char(string, victim);
      return TRUE;
    }
  }
  if(offline)
  {/* Some messages get stored for when you log on */
    if(!target->messages)
      clear_string(&target->messages, string);
    else
    {
      bool overflow = FALSE;
      int len = strlen(target->messages);
      if(len > MAX_INPUT_LENGTH * 10)
        return FALSE;
      char *old_string = target->messages;
      len += strlen(string);
      if(len > MAX_INPUT_LENGTH * 10)
      {
        overflow = TRUE;
        len += strlen("You have reached the limit on offline messages.\n\r");
      }
#ifdef OLC_VERSION
    target->messages = alloc_mem(len + 1);
#else
    target->messages = GC_MALLOC(len);
#endif
      strcpy(target->messages, old_string);
      strcat(target->messages, string);
      if(overflow)
        strcat(target->messages, "You have reached the limit on offline messages.\n\r");
      free_string(old_string);
    }
  }
  return FALSE;
}

bool notify_clan_leaders(char *string, CLAN_DATA *clan, bool offline)
{
  CLAN_CHAR *target;
  bool leader_found = FALSE;
  if(!string || string[0] == '\0' || !clan)
    return FALSE;
  for(target = clan->members; target != NULL; target = target->next)
  {
    if(target->rank == 5)
    {
      notify_clan_char(string, target, offline);
      leader_found = TRUE;
    }
  }
  return leader_found;
}

void notify_clan(char *string, CLAN_DATA *clan)
{
  DESCRIPTOR_DATA *d;
  CHAR_DATA *victim;
  if(!string || string[0] == '\0' || !clan)
    return;
  for(d = descriptor_list; d != NULL; d = d->next)
  {
    if((victim = d->character) == NULL || d->connected != CON_PLAYING)
      continue;
    if(!victim->pcdata || !victim->pcdata->clan_info)
      continue;
    if(victim->pcdata->clan_info->clan == clan)
      send_to_char(string, victim);
  }
}

int calc_merit(CLAN_DATA *clan)
{
  int total = 0;
  MERIT_TRACKER *mtrack = clan->to_match;
  for(; mtrack; mtrack = mtrack->next)
    total += mtrack->amount;
  return total;
}

void do_donate(CHAR_DATA *ch, char *argument)
{
  CHAR_DATA *changer;
  char buf[256], arg[256];
  int amount = 0;
  if(IS_NPC(ch))
    return;
  if(!ch->pcdata->clan_info)
  {
    send_to_char("You must be in a new format clan to donate your merit.\n\r", ch);
    return;
  }
  if(ch->pcdata->clan_info->clan->default_clan == CLAN_OUTCAST)
  {
    send_to_char("Outcasts may not donate, you need to become a loner first.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(arg[0] != 0)
  {
    if(!str_cmp(arg, "sac"))
    {
      int total, amount;
      if(ch->pcdata->clan_info->clan->type != CLAN_TYPE_FAITH)
      {
        send_to_char("Only clans of faith may match merit with sac points.\n\r", ch);
        return;
      }
      if(!ch->pcdata->clan_info->clan->to_match)
      {
        send_to_char("Your clan has no merit available to match with sac points.\n\r", ch);
        return;
      }
      if(!ch->pcdata->sac)
      {
        send_to_char("You have no sac points to match merit with.\n\r", ch);
        return;
      }
      argument = one_argument(argument, arg);
      if(arg[0] == 0 || (!is_number(arg) && str_cmp(arg, "all")))
      {
        send_to_char("To donate sac points you need to specify 'all' or the #.\n\r", ch);
        return;
      }
      total = calc_merit(ch->pcdata->clan_info->clan);
      amount = atoi(arg);
      if(!str_cmp(arg, "all") || amount >= ch->pcdata->sac)
      {/* Donating it all */
        amount = ch->pcdata->sac;
      }
      else
      {
        amount = atoi(arg);
      }
      if(amount > total)
        amount = total;
      sprintf(buf, "You donate %d merit with %d sac points.\n\r", amount, amount);
      send_to_char(buf, ch);
      ch->pcdata->sac -= amount;
      ch->pcdata->clan_info->clan->tribute += amount;
      ch->pcdata->clan_info->donated += amount;
      total -= amount;
      while(amount > 0 && ch->pcdata->clan_info->clan->to_match)
      {
        amount -= ch->pcdata->clan_info->clan->to_match->amount;
        if(amount < 0)
        {/* Too much to match with one donation */
          ch->pcdata->clan_info->clan->to_match->amount = abs(amount);
          amount = 0;
        }
        else
        {
          MERIT_TRACKER *mtrack;
          mtrack = ch->pcdata->clan_info->clan->to_match;
          ch->pcdata->clan_info->clan->to_match = ch->pcdata->clan_info->clan->to_match->next;
          free_merit(mtrack);
        }
      }
      sprintf(buf, "You have %d merit still available to match.\n\r", total);
      send_to_char(buf, ch);
      do_save_clan(ch->pcdata->clan_info->clan);
      return;
    }
    else if(!str_cmp(arg, "gold"))
    {
      int total, amount;
      if(ch->pcdata->clan_info->clan->type != CLAN_TYPE_GREED)
      {
        send_to_char("Only clans of greed may match merit with gold.\n\r", ch);
        return;
      }
      if(!ch->pcdata->clan_info->clan->to_match)
      {
        send_to_char("Your clan has no merit available to match with gold.\n\r", ch);
        return;
      }
      if(!ch->gold)
      {
        send_to_char("You have no gold to match merit with.\n\r", ch);
        return;
      }
      argument = one_argument(argument, arg);
      if(arg[0] == 0 || (!is_number(arg) && str_cmp(arg, "all")))
      {
        send_to_char("To donate gold you need to specify 'all' or the #.\n\r", ch);
        return;
      }
      total = calc_merit(ch->pcdata->clan_info->clan);
      amount = atoi(arg);
      if(!str_cmp(arg, "all") || amount >= ch->pcdata->sac)
      {/* Donating it all */
        amount = ch->gold;
      }
      else
      {
        amount = atoi(arg);
      }
      amount *= 2; /* 2 sac points per gold */
      if(amount > total)
        amount = total;
      sprintf(buf, "You donate %d merit with %d gold.\n\r", amount, UMAX(1, amount / 2));
      send_to_char(buf, ch);
      ch->gold -= UMAX(1, amount / 2);
      ch->pcdata->clan_info->clan->tribute += amount;
      ch->pcdata->clan_info->donated += amount;
      total -= amount;
      while(amount > 0 && ch->pcdata->clan_info->clan->to_match)
      {
        amount -= ch->pcdata->clan_info->clan->to_match->amount;
        if(amount < 0)
        {/* Too much to match with one donation */
          ch->pcdata->clan_info->clan->to_match->amount = abs(amount);
          amount = 0;
        }
        else
        {
          MERIT_TRACKER *mtrack;
          mtrack = ch->pcdata->clan_info->clan->to_match;
          ch->pcdata->clan_info->clan->to_match = ch->pcdata->clan_info->clan->to_match->next;
          free_merit(mtrack);
        }
      }
      sprintf(buf, "You have %d merit still available to match.\n\r", total);
      send_to_char(buf, ch);
      do_save_clan(ch->pcdata->clan_info->clan);
      return;
    }
    else
    {
      send_to_char("To donate merit, just use donate.  Otherwise donate <gold|sac> <all|# amount>\n\r", ch);
      return;
    }
  }

  amount = UMAX(100, (ch->level - 1) * (ch->level - 1) * 4);
  if(ch->pcdata->clan_info->merit < amount)
  {
    sprintf(buf, "You need %d merit to turn in, you only have %d available.\n\r", amount, ch->pcdata->clan_info->merit);
    send_to_char(buf, ch);
    return;
  }
  if(ch->in_room->vnum >= 0)
  {
    for(changer = ch->in_room->people; changer; changer = changer->next_in_room)
    {
      if(IS_NPC(changer) && IS_SET(changer->act, ACT_IS_CHANGER))
        break;
    }
    if(!changer)
    {
      send_to_char("You must be in your hall or with a money changer to donate merit.\n\r", ch);
      return;
    }
  }
  amount = (ch->pcdata->clan_info->merit / amount) * amount;
  ch->pcdata->clan_info->merit -= amount;
  if(ch->pcdata->clan_info->clan->default_clan || ch->pcdata->clan_info->clan->initiation)
  {
    ch->pcdata->clan_info->banked_merit += amount;
    sprintf(buf, "You donate %d merit for use as future tribute (%d).\n\r", amount, amount / 100);
    send_to_char(buf, ch);
  }
  else
  {
    int extra = 0;
    ch->pcdata->clan_info->clan->tribute += amount;
    ch->pcdata->clan_info->donated += amount;
    sprintf(buf, "You donate %d merit as tribute (%d) for your clan.\n\r", amount, amount / 100);
    send_to_char(buf, ch);
    /* Bonus calculations */
    switch(ch->pcdata->clan_info->clan->type)
    {
      default:
      case CLAN_TYPE_LAW:
        if(ch->pcdata->clan_info->primary_merit)
        {
          if(ch->pcdata->clan_info->primary_merit <= amount)
          {/* The whole batch of primary is bonus */
            extra += (ch->pcdata->clan_info->primary_merit + 4) / 5; /* 20% */
            amount -= ch->pcdata->clan_info->primary_merit;
            ch->pcdata->clan_info->primary_merit = 0;
          }
          else
          {
            extra += (amount + 4) / 5;
            ch->pcdata->clan_info->primary_merit -= amount;
            amount = 0;
          }
        }
        if(ch->pcdata->clan_info->secondary_merit && amount)
        {
          ch->pcdata->clan_info->secondary_merit = 0;
          if(ch->pcdata->clan_info->secondary_merit <= amount)
          {/* The whole batch of secondary is bonus */
            extra += (ch->pcdata->clan_info->secondary_merit + 9) / 10; /* 10% */
            amount -= ch->pcdata->clan_info->secondary_merit;
            ch->pcdata->clan_info->secondary_merit = 0;
          }
          else
          {
            extra += (amount + 9) / 10;
            ch->pcdata->clan_info->secondary_merit -= amount;
            amount = 0;
          }
        }
        break;
      case CLAN_TYPE_FAITH:
      case CLAN_TYPE_GREED:
        {
          MERIT_TRACKER *mtrack = new_merit();
          if(!ch->pcdata->clan_info->clan->to_match)
            ch->pcdata->clan_info->clan->to_match = mtrack;
          else
          {
            mtrack->next = ch->pcdata->clan_info->clan->to_match;
            while(mtrack->next->next)
              mtrack->next = mtrack->next->next;
            mtrack->next->next = mtrack;
            mtrack->next = NULL;
          }
          mtrack->amount += (ch->pcdata->clan_info->primary_merit + 9) / 10;
          ch->pcdata->clan_info->primary_merit = 0;
          mtrack->expire = 720; /* 24 hours - by 2 minute intervals */
          if(ch->pcdata->clan_info->clan->type == CLAN_TYPE_FAITH)
            sprintf(buf, "%d additional merit is available to match with sac points.\n\r", mtrack->amount);
          else
            sprintf(buf, "%d additional merit is available to match with gold.\n\r", mtrack->amount);
          send_to_char(buf, ch);
        }
        break;
      case CLAN_TYPE_MALICE:
        if(ch->pcdata->clan_info->primary_merit)
        {
          if(ch->pcdata->clan_info->primary_merit <= amount)
          {/* The whole batch of primary is bonus */
            extra += (ch->pcdata->clan_info->primary_merit + 9) / 10; /* 10% */
            amount -= ch->pcdata->clan_info->primary_merit;
            ch->pcdata->clan_info->primary_merit = 0;
          }
          else
          {
            extra += (amount + 9) / 10;
            ch->pcdata->clan_info->primary_merit -= amount;
            amount = 0;
          }
        }
        break;
      case CLAN_TYPE_PEACE:
        if(ch->pcdata->clan_info->primary_merit)
        {
          if(ch->pcdata->clan_info->primary_merit <= amount)
          {/* The whole batch of primary is bonus */
            extra += (ch->pcdata->clan_info->primary_merit + 19) / 20; /* 5% */
            amount -= ch->pcdata->clan_info->primary_merit;
            ch->pcdata->clan_info->primary_merit = 0;
          }
          else
          {
            extra += (amount + 19) / 20;
            ch->pcdata->clan_info->primary_merit -= amount;
            amount = 0;
          }
        }
        break;
      case CLAN_TYPE_CHAOS: extra = (amount + 19) / 20; break;
    }
    if(extra)
    {
      sprintf(buf, "%d bonus merit (%d tribute) is awarded for your success against your foes!", extra, extra / 100);
      send_to_char(buf, ch);
      ch->pcdata->clan_info->clan->tribute += extra;
      ch->pcdata->clan_info->donated += extra;
    }
  }
  save_clan(ch, TRUE, FALSE, TRUE);
}

void do_rank(CHAR_DATA *ch, char *argument)
{
  if(IS_NPC(ch))
    return;
  char arg1[256], arg2[256];
  int rank;
  CLAN_CHAR *victim = NULL;
  if(IS_IMMORTAL(ch))
  {
    if(ch->level < 57)
    {
      send_to_char("You do not have the power to rank someone.\n\r", ch);
      return;
    }
  }
  else if(!ch->pcdata->clan_info || ch->pcdata->clan_info->rank < MAX_RANK)
  {
    send_to_char("You must be a leader of a current clan to rank someone.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if(!is_number(arg2))
        {
        send_to_char("Ranks need to be numbers.\n\r",ch);
        return;
        }
        rank = atoi(arg2);
        if(rank < 0 || rank > MAX_RANK)
        {
          sprintf(arg2, "Rank must be from 0 to %d.\n\r", MAX_RANK);
          send_to_char(arg2, ch);
          return;
        }
  if(IS_IMMORTAL(ch))
  {/* Check all clans for exact match name */
    CLAN_DATA *clan = clan_first;
    for(; clan != NULL; clan = clan->next)
    {
      for(victim = clan->members; victim != NULL; victim = victim->next)
      {
        if(!str_cmp(victim->name, arg1))
        {
          clan = NULL;/* End the outer loop */
          break;
        }
      }
      if(!clan)
        break;
    }
  }
  else
  {
    for(victim = ch->pcdata->clan_info->clan->members; victim != NULL; victim = victim->next)
    {
      if(!str_cmp(victim->name, arg1))
      {
        break;
      }
    }
  }
  if(!victim)
  {
    sprintf(arg2, "%s not found to set the rank of.  Name must be exact.\n\r", arg1);
    send_to_char(arg2, ch);
    return;
  }
  if(rank == victim->rank)
  {
    send_to_char("They are already that rank.\n\r", ch);
    return;
  }
  if(rank == 5 && victim->clan->leaders >= 3)
  {
    send_to_char("The clan already has as many leaders as are allowed.\n\r", ch);
    return;
  }
  if(victim->rank == 5 && !IS_IMMORTAL(ch) && ch != victim)
  {
    send_to_char("You may not demote another leader.\n\r", ch);
    return;
  }
  if(victim->rank == 5)
    victim->clan->leaders--;
  else if(rank == 5)
    victim->clan->leaders++;
  victim->rank = rank;
  if(victim != ch)
  {
    sprintf(arg2, "%s is now rank %d.\n\r", victim->name, rank);
    send_to_char(arg2, ch);
  }
  sprintf(arg2, "Your rank has been set to %d.\n\r", rank);
  notify_clan_char(arg2, victim, FALSE);
}

bool check_alliance(CLAN_DATA *first, CLAN_DATA *second)
{
  ALLIANCE_DATA *ally = first->allies;
  for(; ally != NULL; ally = ally->next)
  {
    if(ally->clan == second)
    {
      if(ally->pending)
        return FALSE;
      return TRUE;
    }
  }
  return FALSE;
}

void do_chelp(CHAR_DATA *ch, char *argument)
{/* Charters or rules */
  char buf[256], arg[256];
  char *to_page = NULL;
  BUFFER *output;
  if(IS_NPC(ch))
    return;
  argument = one_argument(argument, arg);
  if(arg[0] == 0)
  {
    send_to_char("Usage: chelp <clan name> <rules|charter> (Rules are for your clan only)\n\r", ch);
    return;
  }
  CLAN_DATA *clan = clan_lookup(arg);
  if(clan == NULL || clan->default_clan)
  {
    send_to_char("That is not a valid clan.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);

  if(!arg[0] || !str_prefix(arg, "charter"))
  {
    if(!clan->charter)
    {
      send_to_char("There is no charter available for that clan.\n\r", ch);
      return;
    }
    to_page = clan->charter;
  }
  else if(!str_prefix(arg, "rules"))
  {
    if(!IS_IMMORTAL(ch) && (!ch->pcdata->clan_info || ch->pcdata->clan_info->clan != clan))
    {
      send_to_char("You may only check the rules for your own clan.\n\r", ch);
      return;
    }
    if(!clan->rules)
    {
      send_to_char("There are no rules set for your clan yet.\n\r", ch);
      return;
    }
    to_page = clan->rules;
  }
  else
  {
    send_to_char("Usage: chelp <clan name> <rules|charter> (Rules are for your clan only)\n\r", ch);
    return;
  }
  if(!to_page)
    return; /* Error of some sort */
  output = new_buf();
  page_to_char(to_page,ch);
  free_buf(output);
}

void do_cinfo(CHAR_DATA *ch, char *argument)
{
  int count, leaders, recruiters, allies;
  char buf[256];
  CLAN_DATA *clan = NULL;
  CLAN_CHAR *cchar = NULL;
  ALLIANCE_DATA *ally = NULL;
  if(IS_NPC(ch))
    return;
  if(IS_IMMORTAL(ch))
  {/* Immortals are allowed to check clan infos */
    if(argument[0] != '\0')
    {
      clan = clan_lookup(argument);
      if(clan == NULL)
      {
        send_to_char("That is not a valid clan.\n\r", ch);
        return;
      }
    }
  }
  if(!clan && (!ch->pcdata->clan_info || ch->pcdata->clan_info->clan->default_clan))
  {
    send_to_char("You are not in a clan with a member list.\n\r", ch);
    return;
  }
  if(!IS_IMMORTAL(ch) && ch->pcdata->clan_info->rank < 4)
  {
    send_to_char("Sorry, only recruiters and leaders may check the clan's info at this time.\n\r", ch);
    return;
  }
  if(!clan)
    clan = ch->pcdata->clan_info->clan;
  cchar = clan->members;
  count = 0;
  leaders = 0;
  recruiters = 0;
  for(; cchar; cchar = cchar->next)
  {
    count++;
    if(cchar->rank == 5)
      leaders++;
    if(cchar->rank == 4)
      recruiters++;
  }
  allies = 0;
  for(ally = clan->allies; ally; ally = ally->next)
    allies++;
  sprintf(buf, "%s info:\n\rLeaders: %d\n\rRecruiters: %d\n\rTotal Members: %d\n\rNumber of Alliances: %d\n\r", clan->name, leaders, recruiters, count, allies);
  send_to_char(buf, ch);
  if(clan->initiation)
  {
    sprintf(buf, "Currently in initiation, %d merit remaining to complete.\n\r", clan->initiation);
    send_to_char(buf, ch);
  }
  sprintf(buf, "Available Tribute: %d\n\rTotal Tribute: %d\n\r", clan->tribute / 100, clan->max_tribute / 100);
  send_to_char(buf, ch);
  if(clan->inactive)
    send_to_char("{RThe clan is currently inactive.{x\n\r", ch);
  send_to_char("\n\r", ch);
}

void do_clist(CHAR_DATA *ch, char *argument)
{
  CLAN_DATA *clan;
  char buf[256];
  BUFFER *output;
  if(IS_NPC(ch))
    return;
  output = new_buf();
  for(clan = clan_first; clan != NULL; clan = clan->next)
  {
    if(!clan->default_clan && !clan->inactive)
    {
      sprintf( buf, "%s%s{x. Type: ", clan->color, clan->name);
      switch(clan->type)
      {
        default: strcat(buf, "Gang.\n\r"); break;
        case CLAN_TYPE_LAW: strcat(buf, "Law.\n\r"); break;
        case CLAN_TYPE_FAITH: strcat(buf, "Faith.\n\r"); break;
        case CLAN_TYPE_GREED: strcat(buf, "Greed.\n\r"); break;
        case CLAN_TYPE_MALICE: strcat(buf, "Malice.\n\r"); break;
        case CLAN_TYPE_PEACE: strcat(buf, "Peace.\n\r"); break;
        case CLAN_TYPE_CHAOS: strcat(buf, "Chaos.\n\r"); break;
      }
      add_buf(output,buf);
    }
  }
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

void do_cmembers(CHAR_DATA *ch, char *argument)
{
  char buf[256];
  CLAN_CHAR *cchar = NULL;
  BUFFER *output;
  if(IS_NPC(ch))
    return;
  output = new_buf();
  if(IS_IMMORTAL(ch))
  {/* Immortals are allowed to check clan lists */
    if(argument[0] != '\0')
    {
      CLAN_DATA *clan = clan_lookup(argument);
      if(clan == NULL)
      {
        send_to_char("That is not a valid clan.\n\r", ch);
        return;
      }
      if(clan->members)
        cchar = clan->members;
      else
      {
        send_to_char("There are no members in that clan.\n\r", ch);
        return;
      }
    }
  }
  if(!cchar && (!ch->pcdata->clan_info || ch->pcdata->clan_info->clan->default_clan))
  {
    send_to_char("You are not in a clan with a member list.\n\r", ch);
    return;
  }
  if(!cchar)
    cchar = ch->pcdata->clan_info->clan->members;
  for(; cchar != NULL; cchar = cchar->next)
  {
    if(IS_IMMORTAL(ch) || ch->pcdata->clan_info->rank == 5)
    {
      sprintf( buf, "%s. Rank: %d. Tribute: %d.\n\r", cchar->name, cchar->rank, cchar->donated / 100);
    }
    else
      sprintf( buf, "%s. Rank: %d.\n\r", cchar->name, cchar->rank);
    add_buf(output,buf);
  }
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

void do_callies(CHAR_DATA *ch, char *argument)
{
  char buf[256];
  ALLIANCE_DATA *ally = NULL;
  CLAN_DATA *clan;
  BUFFER *output;
  if(IS_NPC(ch))
    return;
  if(IS_IMMORTAL(ch))
  {/* Immortals are allowed to check clan lists */
    if(argument[0] != '\0')
    {
      clan = clan_lookup(argument);
      if(clan == NULL)
      {
        send_to_char("That is not a valid clan.\n\r", ch);
        return;
      }
      if(!clan->allies)
      {
        send_to_char("That clan has no allies currently.\n\r", ch);
        return;
      }
      ally = clan->allies;
    }
  }
  if(!ally && (!ch->pcdata->clan_info || !ch->pcdata->clan_info->clan->allies))
  {
    send_to_char("Your clan has no allies currently.\n\r", ch);
    return;
  }
  if(!ally)
    ally = ch->pcdata->clan_info->clan->allies;
  output = new_buf();
  for(; ally != NULL; ally = ally->next)
  {
    if(ally->pending && !IS_IMMORTAL(ch) && ch->pcdata->clan_info->rank != 5)
      continue;
    if(ally->duration < 90)
      sprintf(buf, "%s. Duration: %d minute%s.%s\n\r", ally->clan->name,
        ally->duration * 2 / 3, ally->duration == 2 ? "" : "s",
        ally->pending ? " (Pending)" : "");
    else if(ally->duration < 2160)
      sprintf(buf, "%s. Duration: %d hour%s.%s\n\r", ally->clan->name,
        ally->duration / 90, ally->duration < 180 ? "" : "s",
        ally->pending ? " (Pending)" : "");
    else
      sprintf(buf, "%s. Duration: %d day%s.%s\n\r", ally->clan->name,
        ally->duration / 2160, ally->duration < 5320 ? "" : "s",
        ally->pending ? " (Pending)" : "");
    add_buf(output,buf);
  }
  page_to_char(buf_string(output),ch);
  free_buf(output);
}

void do_ally(CHAR_DATA *ch, char *argument)
{
  char buf[256], arg[256];
  CLAN_DATA *target;
  ALLIANCE_DATA *ally, *prev_ally;
  int cost, other_cost;
  int duration, bribe = 0;
  if(IS_NPC(ch))
    return;

  if(IS_IMMORTAL(ch))
  {/* Special handling for immortals */
    if(ch->level < 57)
    {
      send_to_char("You may not form alliances for clans.\n\r", ch);
      return;
    }
    return;
  }
  if(!ch->pcdata->clan_info || ch->pcdata->clan_info->clan->default_clan || ch->pcdata->clan_info->rank != 5)
  {
    send_to_char("You may not form alliances for your clan.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(arg[0] == '\0')
  {
    send_to_char("Usage: ally <clan> <duration> <payment> <bribe>\n\rUsage: ally <clan> <display/accept/reject/withdraw>\n\r", ch);
    return;
  }
  if((target = clan_lookup(arg)) == NULL)
  {
    send_to_char("That is not a valid clan.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(!str_prefix(arg, "display") || !str_prefix(arg, "accept") || !str_prefix(arg, "reject"))
  {
    prev_ally = NULL;
    for(ally = ch->pcdata->clan_info->clan->allies; ally != NULL; ally = ally->next)
    {
      if(ally->clan == target)
      {
        if(ally->pending)
        {/* Can do something based on command */
          if(!str_cmp(arg, "reject"))
          {/* The simplest, kill the offer - refund the reserved tribute */
            send_to_char("You reject the alliance.\n\r", ch);
            sprintf(buf, "%s has {Rrejected{x your alliance offer, %d reserved tribute refunded.\n\r", ch->pcdata->clan_info->clan->name, ally->cost / 100);
            notify_clan_leaders(buf, ally->clan, FALSE);
            ally->clan->tribute += ally->cost;
            if(prev_ally)
              prev_ally->next = ally->next;
            else
              ch->pcdata->clan_info->clan->allies = ally->next;
            free_ally(ally);
            return;
          }
          if(!str_cmp(arg, "display"))
          {/* Show the details of the offer */
            if(ally->bribe)
              sprintf(buf, "Offer from %s for %d days. Cost: %d tribute. Bribe: %d tribute.\n\r", ally->clan->name, ally->offer_duration, ally->to_pay / 100, ally->bribe / 100);
            else
              sprintf(buf, "Offer from %s for %d days. Cost: %d tribute.\n\r", ally->clan->name, ally->offer_duration, ally->to_pay / 100);
            send_to_char(buf, ch);
            return;
          }
          /* Must be accept - pay for both clans, attach alliance to the offering clan */
          if(ally->to_pay && ally->to_pay > ch->pcdata->clan_info->clan->tribute)
          {
            sprintf(buf, "It costs %d tribute to accept that alliance, your clan only has %d.\n\r",
              ally->to_pay / 100, ch->pcdata->clan_info->clan->tribute / 100);
            send_to_char(buf, ch);
            return;
          }
          ch->pcdata->clan_info->clan->tribute -= ally->to_pay;
          ch->pcdata->clan_info->clan->tribute += ally->bribe;
          send_to_char("You accept the alliance.\n\r", ch);
          sprintf(buf, "%s has {Gaccepted{x your alliance offer, you will be allied for %d days.\n\r", ch->pcdata->clan_info->clan->name, ally->offer_duration);
          notify_clan_leaders(buf, ally->clan, FALSE);
          sprintf(buf, "Your clan is now allied with %s for %d days.\n\r", ally->clan->name, ally->offer_duration);
          notify_clan(buf, ch->pcdata->clan_info->clan);
          sprintf(buf, "Your clan is now allied with %s for %d days.\n\r", ch->pcdata->clan_info->clan->name, ally->offer_duration);
          notify_clan(buf, ally->clan);
          ally->duration = ally->offer_duration * 24 * 60 * 60 / 40;
          ally->pending = FALSE;
          /* Insert the new alliance back to the offering clan */
          prev_ally = new_ally();
          if(ally->clan->allies)
          {
            prev_ally->next = ally->clan->allies;
            while(prev_ally->next->next)
              prev_ally->next = prev_ally->next->next;
            prev_ally->next->next = prev_ally;
            prev_ally->next = NULL;
          }
          else
            ally->clan->allies = prev_ally;
          prev_ally->clan = ch->pcdata->clan_info->clan;
          prev_ally->duration = ally->duration;
        }
        else
          send_to_char("You already have an alliance with that clan.\n\r", ch);
        return;
      }
      prev_ally = ally;
    }
    send_to_char("That clan has not offered you an alliance.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "withdraw"))
  {/* Cancel an offered alliance */
    prev_ally = NULL;
    for(ally = target->allies; ally != NULL; ally = ally->next)
    {
      if(ally->clan == ch->pcdata->clan_info->clan)
        break;
      prev_ally = ally;
    }
    if(!ally)
    {
      send_to_char("You do not have a pending alliance with that clan.\n\r", ch);
      return;
    }
    sprintf(buf, "You withdraw your alliance offer, %d reserved tribute refunded.\n\r", ally->cost / 100);
    send_to_char(buf, ch);
    ch->pcdata->clan_info->clan->tribute += ally->cost;
    sprintf(buf, "%s has withdrawn their alliance offer.\n\r", ch->pcdata->clan_info->clan->name);
    notify_clan_leaders(buf, ally->clan, FALSE);
    if(!prev_ally)
      ally->clan->allies = ally->next;
    else
      prev_ally->next = ally->next;
    free_ally(ally);
    return;
  }
  if(!is_number(arg) || (duration = atoi(arg)) < 0 || duration > 7)
  {
    send_to_char("Duration must be a number from 1 to 7 (Days).\n\r", ch);
    return;
  }
  cost = duration * 1400 + 200; /* Tracking the alliance itself's cost */
  if(ch->pcdata->clan_info->clan->type == CLAN_TYPE_PEACE || target->type == CLAN_TYPE_PEACE)
    cost -= duration * 400;
  argument = one_argument(argument, arg);
  if(arg[0] == '\0')
  {
    send_to_char("You must specify who pays: 'full' (Your clan entirely) or 'split' (50/50).\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "full"))
  {
    other_cost = 0;
  }
  else if(!str_prefix(arg, "split"))
  {
    cost /= 2;
    other_cost = cost;
  }
  else
  {
    send_to_char("Invalid payment option.  Available: full, split.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(arg[0] != '\0')
  {
    if(!is_number(arg) || (bribe = atoi(arg)) <= 0)
    {
      send_to_char("The bribe you are offering must be a number greater than 0.\n\r", ch);
    }
    bribe *= 100;
    cost += bribe;
  }
  if(ch->pcdata->clan_info->clan->tribute < cost)
  {
    sprintf(buf, "It costs %d tribute to offer that alliance, your clan has %d.\n\r", cost / 100, ch->pcdata->clan_info->clan->tribute / 100);
    send_to_char(buf, ch);
    return;
  }
  sprintf(buf, "Your clan pays 5 tribute to offer %s an alliance for %d days.\n\r%d tribute is reserved until they accept or reject.\n\r", target->name, duration, (cost - 500) / 100);
  send_to_char(buf, ch);
  ch->pcdata->clan_info->clan->tribute -= cost;/* Reserved */
  if(bribe > 0)
    sprintf(buf, "%s offers your clan an alliance for %d days with %d bribe.\n\r", ch->pcdata->clan_info->clan->name, duration, bribe / 100);
  else
    sprintf(buf, "%s offers your clan an alliance for %d days.\n\r", ch->pcdata->clan_info->clan->name, duration);
  if(other_cost)
    sprintf(arg, "They want to split the cost 50/50, it will cost your clan %d tribute.\n\r",  other_cost / 100);
  else
    sprintf(arg, "They are paying full cost for the alliance themselves.\n\r");
  strcat(buf, arg);
  notify_clan_leaders(buf, target, FALSE);
  ally = new_ally();
  ally->cost = cost - 500; /* 500 non-refundable */
  ally->bribe = bribe;
  ally->to_pay = other_cost;
  ally->pending = TRUE;
  ally->offer_duration = duration;
  ally->duration = 24 * 60 * 60 / 40; /* Ticks last 40 seconds */
  ally->clan = ch->pcdata->clan_info->clan;
  if(target->allies)
  {
    ally->next = target->allies;
    while(ally->next->next)
      ally->next = ally->next->next;
    ally->next->next = ally;
    ally->next = NULL;
  }
  else
    target->allies = ally;
}

void calculate_award_tribute(CLAN_DATA *clan)
{
  CLAN_CHAR *cchar, *highest;
  bool found = TRUE;
  int divisor = 3;
  clan->tribute -= clan->awarded_tribute;
  clan->max_tribute -= clan->awarded_tribute;
  clan->awarded_tribute = 0;
  while(1)
  {
    found = FALSE;
    highest = NULL;
    for(cchar = clan->members; cchar; cchar = cchar->next)
    {
      if(cchar->award_merit <= 0)
        continue;
      if(!highest || cchar->award_merit > highest->award_merit)
        highest = cchar;
    }
    if(highest)
    {
      if(!clan->awarded_tribute)
        clan->awarded_tribute = BASE_BONUS_TRIBUTE; /* Earned for having anyone with bonus tribute */
      /* 3/3, 3/4, 3/5, 3/6 -- less harsh than 1, 1/2, 1/3, 1/4 */
      clan->awarded_tribute += highest->award_merit * 3 / divisor;
      highest->award_merit *= -1; /* Mark it as used */
      divisor++; /* Each extra player counts for less */
    }
    else
      break;/* Done */
  }
  /* 200% max */
  clan->awarded_tribute = UMIN(BASE_BONUS_TRIBUTE * 2, clan->awarded_tribute);
  clan->tribute += clan->awarded_tribute;
  clan->max_tribute += clan->awarded_tribute;
  for(cchar = clan->members; cchar; cchar = cchar->next)
    cchar->award_merit = abs(cchar->award_merit);
  do_save_clan(clan);
}

void remove_clan_member(CLAN_CHAR *cchar)
{
  CLAN_DATA *outcast;
  CLAN_CHAR *prev = NULL, *old_c = NULL;
  for(outcast = clan_first; outcast != NULL; outcast = outcast->next)
  {
    if(outcast->default_clan == CLAN_OUTCAST)
      break;
  }
  if(!outcast)
  {/* There must be an outcast clan. */
    bug("No outcast clan created for remove.", 0);
    outcast = new_clan();
    outcast->default_clan = CLAN_OUTCAST;
    outcast->name = str_dup("Outcast");
    outcast->next = clan_first;
    clan_first = outcast;
  }
  for(old_c = cchar->clan->members; old_c != NULL; old_c = old_c->next)
  {
    if(old_c == cchar)
      break;
    prev = old_c;
  }
  if(!old_c)
  {
    bug("CLAN_CHAR not found to remove from clan.", 0);
  }
  else
  {
    bool deleted_clan = FALSE;
    if(!prev)
    {
      cchar->clan->members = cchar->clan->members->next;
      if(cchar->clan->members == NULL && !cchar->clan->default_clan)
      {/* Last member is gone, mark to disband the clan */
        char save_old[256], save_new[256];
        CLAN_DATA *clan, *prev_clan;
        deleted_clan = TRUE;
        for(old_c = outcast->members; old_c != NULL; old_c = old_c->next)
        {
          if(old_c->old_clan == cchar->clan)
          {/* Can't rebel, clan is gone */
            old_c->old_clan = NULL;
            old_c->old_donated = 0;
            old_c->rebel_timer = 0;
          }
        }
        /* Back it up to this member's name, fairly protected backup this way */
        sprintf(save_old, "%s%s.clan", CLAN_DIR, capitalize(cchar->clan->name));
        sprintf(save_new, "%s%s_%s.clanbak", CLAN_BAK_DIR, capitalize(cchar->clan->name), cchar->name);
        rename(save_old, save_new);
        sprintf(save_old, "%s%s.hall", CLAN_DIR, capitalize(cchar->clan->name));
        sprintf(save_new, "%s%s_%s.hallbak", CLAN_BAK_DIR, capitalize(cchar->clan->name), cchar->name);
        rename(save_old, save_new);
        sprintf(save_old, "%s%s.msgs", CLAN_DIR, capitalize(cchar->clan->name));
        sprintf(save_new, "%s%s_%s.msgsbak", CLAN_BAK_DIR, capitalize(cchar->clan->name), cchar->name);
        rename(save_old, save_new);

        /* Save the clan list */
        if(clan_first == cchar->clan)
          clan_first = cchar->clan->next;
        else
        {
          for(clan = clan_first; clan != NULL; clan = clan->next)
          {
            if(clan->next == cchar->clan)
            {
              clan->next = clan->next->next;
              break;
            }
          }
        }
        save_clan_list();
        free_clan(cchar->clan);
        cchar->clan = NULL;
      }
    }
    else
      prev->next = prev->next->next;

    if(!deleted_clan)
    {/* Don't save rebel information if the clan is gone */
      if(!cchar->clan->default_clan)
      {
        cchar->old_donated = cchar->donated;
        cchar->rebel_timer = 15120;/* One week of ticks */
        cchar->old_clan = cchar->clan;
        calculate_award_tribute(cchar->old_clan);
      }
      do_save_clan(cchar->clan);/* Update their removal */
    }

  }

  cchar->clan = outcast;/* Gone from their clan */
  cchar->donated = 0;
  cchar->primary_merit = 0;
  cchar->secondary_merit = 0;
/* Note they're not "officially" part of the outcast clan */
  if(cchar->personal) /* Set rooms need to be removed */
  {
    /* If anyone is in them, move them out */
    /* Remove/refund for all items */
  }
}

int calculate_bonus_merit(CHAR_DATA *ch, bool new_join)
{
  int clan, hours, rank, bonus_merit;
char buf[256];
int hold1, hold2, hold3, hold4, hold5;
  int hourmod = 0, hourtotal = 0;
  if(new_join)
  {
    clan = ch->clan;
    hours = (int)(ch->played + (current_time - ch->logon) ) / 3600;
    rank = ch->pcdata->rank;
  }
  else
  {/* Recalculating */
    clan = ch->pcdata->old_c_clan;
    hours = ch->pcdata->old_c_hours;
    rank = ch->pcdata->old_c_rank;
  }
  if(hours > 1000)
    hours = 1000;
  else if(hours < 0)
    hours = 0;
  if(rank < 0 || rank > 5)
    rank = 0;
sprintf(buf, "Starting stats: %d clan %d hours %d rank", clan, hours, rank);
log_string(buf);

  /* Assign them reward merit based on current clan, rank, and hours */
  if(clan == nonclan_lookup("demise"))
    bonus_merit = 50;
  if(clan == nonclan_lookup("zealot"))
    bonus_merit = 40;
  else if(clan && clan != nonclan_lookup("loner") && clan != nonclan_lookup("outcast"))
    bonus_merit = 35;
  else
    bonus_merit = 30;
hold1 = bonus_merit;
  bonus_merit = BASE_BONUS_TRIBUTE * bonus_merit / 100; /* Less loss to integer division */
hold2 = bonus_merit;
  /* 100 = 25%, 300 = 50%, 600 = 75%, 1000 = 100% */
  while(hours > 0)
  {
    hourmod += 100;
    hours -= hourmod;
    if(hours >= 0)
      hourtotal++;
  }
  hourmod = UMAX(100, hourmod);
hold3 = hourtotal;
  hourtotal *= 25; /* 25% - 100% */
  hourtotal += (abs(hours) * 25) / (hourmod * 100);/* 0-25% based on amount left */
hold4 = hourtotal;
  bonus_merit = bonus_merit * hourtotal / 100;
  /* Leaders get 100%, everyone else is 3% less per less rank */
hold5 = bonus_merit;
  bonus_merit = bonus_merit * (ch->pcdata->rank * 3 + 85) / 100;
sprintf(buf, "%d start %d mult %d htotal %d hresult %d hbonus %d final",
  hold1, hold2, hold3, hold4, hold5, bonus_merit);
log_string(buf);
  return bonus_merit;
}

void add_clan_member(CLAN_DATA *clan, CHAR_DATA *ch, int rank)
{
  char buf[256];
  CLAN_CHAR *new_c = NULL, *prev = NULL;
  if(IS_NPC(ch))
  {
    send_to_char("NPCs may not join a clan.\n\r", ch);
    return;
  }

  if(clan->default_clan)
    rank = 0; /* No clan rank in the default clans */
  else if(rank < 0 || rank > 5)
    rank = 0;/* Error rank */

  if(ch->pcdata->clan_info)
  {
    remove_clan_member(ch->pcdata->clan_info);
    new_c = ch->pcdata->clan_info;
  }

  if(new_c == NULL)
  {
    new_c = new_clan_char();
    new_c->player = ch;
    ch->pcdata->clan_info = new_c;
    /* Assign them reward merit based on current clan, rank, and hours */
    if(!IS_IMMORTAL(ch))
    {
      ch->pcdata->clan_info->award_merit = calculate_bonus_merit(ch, TRUE);
      if(ch->pcdata->clan_info->award_merit < 100)
        send_to_char("You are awarded starter tribute credit for your past experience.\n\r", ch);
      else
      {
        sprintf(buf, "You are awarded %d starter tribute for your past experience.\n\r", ch->pcdata->clan_info->award_merit / 100);
        send_to_char(buf, ch);
      }
    }
    ch->pcdata->old_c_clan = ch->clan;
    ch->pcdata->old_c_rank = ch->pcdata->rank;
    ch->pcdata->old_c_hours = UMIN(1000, (int)(ch->played + (current_time - ch->logon) ) / 3600);
  }
  new_c->next = NULL;

  if(!clan->members)
  {
    clan->members = new_c;
  }
  else
  {
    CLAN_CHAR *cchar;
    for(prev = clan->members; prev->next != NULL; prev = prev->next);
      prev->next = new_c;
  }
  if(!clan->default_clan)
    calculate_award_tribute(clan);

  clear_string(&new_c->name, ch->name);
  new_c->clan = clan;
  new_c->rank = rank;
  new_c->join_date = current_time;
  new_c->last_login = current_time;
  new_c->donated = 0;
  new_c->flags = 0; /* Fresh start */

  new_c->old_clan = NULL;/* These are cleared upon joining a new clan */
  new_c->old_donated = 0;
  new_c->rebel_timer = 0;

  ch->clan = 0;/* Clear their old clan */
  ch->pcdata->rank = 0;/* Clear their old rank */
  do_save_clan(clan);

  if(clan->type == CLAN_TYPE_FAITH)
  {
    sprintf(buf,"To follow your clan's declared faith, you are now a follower of %s.\n\r",deity_table[clan->enemy].pname);
    send_to_char(buf,ch);
    ch->pcdata->deity = clan->enemy;
    ch->pcdata->deity_timer = 0;
  }
  set_clan_skills(ch);
  save_char_obj(ch);
}


bool is_clan_friendly(CHAR_DATA *first, CHAR_DATA *second)
{
  if(IS_NPC(first))
  {
    if(first->pIndexData->vnum < 0)
      return TRUE;
    return FALSE;
  }
  if(IS_NPC(second))
  {
    if(second->pIndexData->vnum < 0)
      return TRUE;
    return FALSE;
  }
/* Test only code */
//if(first->pcdata->clan_info)
//{/* They're not interested in fighting */
//  if(!is_name("fight", first->pcdata->title))
//    return TRUE;
//}
//if(second->pcdata->clan_info)
//{/* They're not interested in fighting */
//  if(!is_name("fight", second->pcdata->title))
//    return TRUE;
//}
  if(first->pcdata->clan_info && !first->pcdata->clan_info->clan->default_clan)
  {
    if(second->pcdata->clan_info && !second->pcdata->clan_info->clan->default_clan)
    {
      if(first->pcdata->clan_info->clan == second->pcdata->clan_info->clan)
        return TRUE;
      return check_alliance(first->pcdata->clan_info->clan, second->pcdata->clan_info->clan);
    }
  }
  /* No chance of them being in the same clan/alliance, just check if they're clanners */
  if(is_clan(first) && is_clan(second))
    return FALSE;
  return TRUE;
}


CLAN_CHAR *find_char_clan(char *name)
{
  CLAN_CHAR *cchar;
  CLAN_DATA *clan = clan_first;
  for(; clan != NULL; clan = clan->next)
  {
    for(cchar = clan->members; cchar != NULL; cchar = cchar->next)
    {
      if(!str_cmp(cchar->name, name))
        return cchar;
    }
  }
  return NULL;
}

void chars_only(char *arg)
{
  int i, j;
  for(i = 0, j = 0; arg[j] != '\0'; i++, j++)
  {
    if(arg[j] == '{')
    {
      if(arg[j + 1] != '\0')
        j++;/* Eat the color code completely */
      i--;/* Hold i still */
      continue;
    }
    if(ispunct(arg[j]) || isdigit(arg[j]))
    {
      i--;/* Hold i still */
      continue;
    }
    arg[i] = arg[j];
  }
  arg[i] = arg[j];
  /* Remove trailing spaces too */
  if(i > 0)
  {
    i--; /* Move it off the terminator */
    while(i >= 0 && arg[i] == ' ')
      i--;
    arg[i + 1] = '\0';
  }
}

bool check_offline_outcast(char *name)
{
  CLAN_DATA *clan;
  CLAN_CHAR *cchar;
  if(name == NULL || name[0] == '\0')
    return FALSE;
  for(clan = clan_first; clan != NULL; clan = clan->next)
  {
    if(clan->default_clan == CLAN_OUTCAST)
      break;
  }
  if(clan)
  {
    for(cchar = clan->members; cchar != NULL; cchar = cchar->next)
    {
      if(!str_cmp(cchar->name, name))
        return TRUE;
    }
  }
  return FALSE;
}

/* Returns if this is a bonus qualifying kill or not */
bool clan_kill_type(CHAR_DATA *killer, CHAR_DATA *victim)
{
  if(IS_NPC(killer) || !killer->pcdata->clan_info || killer->pcdata->clan_info->clan->default_clan)
    return FALSE;
  if(IS_SET(victim->act,PLR_DWEEB))
    return TRUE;
  switch(killer->pcdata->clan_info->clan->type)
  {
    default: return FALSE; /* Most types have no primary kill type or type may not be set */
    case CLAN_TYPE_LAW:
      switch(killer->pcdata->clan_info->clan->enemy)
      {
        default: return FALSE;
        case CLAN_ENEMY_SMALL: return (victim->size == SIZE_TINY || victim->size == SIZE_SMALL || victim->size == SIZE_MEDIUM);
        case CLAN_ENEMY_MEDIUM: return (victim->size == SIZE_MEDIUM || victim->size == SIZE_LARGE);
        case CLAN_ENEMY_LARGE: return (victim->size == SIZE_LARGE || victim->size == SIZE_HUGE || victim->size == SIZE_GIANT);
        case CLAN_ENEMY_FIGHTER:
          return(victim->class == class_lookup("warrior") || victim->class == class_lookup("blademaster") ||
            victim->class == class_lookup("berzerker") || victim->class == class_lookup("thief") ||
            victim->class == class_lookup("monk") || victim->class == class_lookup("rogue") ||
            victim->class == class_lookup("samurai") || victim->class == class_lookup("paladin"));
        case CLAN_ENEMY_CASTER:
          return(victim->class == class_lookup("mage") || victim->class == class_lookup("wizard") ||
            victim->class == class_lookup("cleric") || victim->class == class_lookup("crusader") ||
            victim->class == class_lookup("druid") || victim->class == class_lookup("assassin") ||
            victim->class == class_lookup("elementalist"));
        case CLAN_ENEMY_FLAGGED: return (IS_SET(victim->wiznet, PLR_RUFFIAN) ||
          IS_SET(victim->act, (PLR_THUG | PLR_KILLER | PLR_THIEF)) != 0);
        case CLAN_ENEMY_UNFLAGGED: return (!IS_SET(victim->wiznet, PLR_RUFFIAN) &&
          !IS_SET(victim->act, (PLR_THUG | PLR_KILLER | PLR_THIEF)));
        case CLAN_ENEMY_REMORTED: return IS_SET(victim->act, (PLR_VAMP | PLR_WERE | PLR_MUMMY)) != 0;
        case CLAN_ENEMY_UNREMORTED: return !IS_SET(victim->act, (PLR_VAMP | PLR_WERE | PLR_MUMMY));
        case CLAN_ENEMY_LAW: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_LAW);
        case CLAN_ENEMY_FAITH: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_FAITH);
        case CLAN_ENEMY_GREED: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_GREED);
        case CLAN_ENEMY_MALICE: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_MALICE);
        case CLAN_ENEMY_PEACE: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_PEACE);
        case CLAN_ENEMY_CHAOS: return (victim->pcdata && victim->pcdata->clan_info &&
          victim->pcdata->clan_info->clan->type == CLAN_TYPE_CHAOS);
        case CLAN_ENEMY_GANG: return (victim->pcdata && victim->pcdata->clan_info &&
          !victim->pcdata->clan_info->clan->default_clan &&
          !victim->pcdata->clan_info->clan->type);
      }
      break;
    case CLAN_TYPE_FAITH:
    case CLAN_TYPE_GREED:
    case CLAN_TYPE_MALICE: return !IS_NPC(victim);
    case CLAN_TYPE_PEACE: return IS_NPC(victim);
  }
}

/* Create a clan */
void do_establish(CHAR_DATA *ch, char *argument)
{
  CLAN_DATA *clan, *temp;
  char buf[256];
  char arg[256];
  int i, type;
  if(IS_NPC(ch))
  {
    send_to_char("NPCs may not establish a clan.\n\r", ch);
    return;
  }
  if(ch->level < 5)
  {
    send_to_char("You must be at least level 5 to establish a clan.\n\r", ch);
    return;
  }
  if(!IS_IMMORTAL(ch) && (!ch->in_room || ch->in_room->vnum != 3001))
  {
//    send_to_char("You must be in the Priest of All Gods' room to establish a clan or its type.\n\r", ch);
//    return;
  }
  if(ch->pcdata->clan_info && !ch->pcdata->clan_info->clan->default_clan
    && !ch->pcdata->clan_info->clan->inactive)
  {
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
      send_to_char("Usage: establish type <type> <options>\n\r", ch);//{RWarning{x: If you already have a type set, this will require initiation.\n\r", ch);
    else if(!str_cmp(arg, "type"))
    {
      char type[10];
      int new_type = 0, new_enemy = 0;
      if(!IS_IMMORTAL(ch) && ch->pcdata->clan_info->rank != 5)
      {
        send_to_char("Only a clan leader may establish the clan's type.\n\r", ch);
        return;
      }
      if(!IS_IMMORTAL(ch))/* && ch->pcdata->clan_info->clan->type != 0 && ch->pcdata->clan_info->clan->tribute < 100000)*/
      {
        //send_to_char("It costs 1000 points to change a clan type, your clan does not have enough.\n\r", ch);
        //return;
        send_to_char("Eventually it will cost to change your clan type.\n\r", ch);
      }
      argument = one_argument(argument, arg);
      type[0] = '\0';
      if(!str_cmp(arg, "law"))
      {
        strcpy(type, "law");
        argument = one_argument(argument, arg);
        if(!str_cmp(arg, "small"))
          new_enemy = CLAN_ENEMY_SMALL;
//        else if(!str_cmp(arg, "medium"))
//          new_enemy = CLAN_ENEMY_MEDIUM;
        else if(!str_cmp(arg, "large"))
          new_enemy = CLAN_ENEMY_LARGE;
        else if(!str_cmp(arg, "fighter"))
          new_enemy = CLAN_ENEMY_FIGHTER;
        else if(!str_cmp(arg, "caster"))
          new_enemy = CLAN_ENEMY_CASTER;
        else if(!str_cmp(arg, "flagged"))
          new_enemy = CLAN_ENEMY_FLAGGED;
        else if(!str_cmp(arg, "unflagged"))
          new_enemy = CLAN_ENEMY_UNFLAGGED;
        else if(!str_cmp(arg, "remorted"))
          new_enemy = CLAN_ENEMY_REMORTED;
        else if(!str_cmp(arg, "unremorted"))
          new_enemy = CLAN_ENEMY_UNREMORTED;
        else if(!str_cmp(arg, "law"))
          new_enemy = CLAN_ENEMY_LAW;
        else if(!str_cmp(arg, "faith"))
          new_enemy = CLAN_ENEMY_FAITH;
        else if(!str_cmp(arg, "greed"))
          new_enemy = CLAN_ENEMY_GREED;
        else if(!str_cmp(arg, "malice"))
          new_enemy = CLAN_ENEMY_MALICE;
        else if(!str_cmp(arg, "peace"))
          new_enemy = CLAN_ENEMY_PEACE;
        else if(!str_cmp(arg, "chaos"))
          new_enemy = CLAN_ENEMY_CHAOS;
        else if(!str_cmp(arg, "gang"))
          new_enemy = CLAN_ENEMY_GANG;
        else
        {
          send_to_char("Invalid enemy type.  Please see 'help clan law' for details.\n\r", ch);
          return;
        }
        new_type = CLAN_TYPE_LAW;
      }
      else if(!str_cmp(arg, "faith"))
      {
        strcpy(type, "faith");
        argument = one_argument(argument, arg);
        if ( (new_enemy = deity_lookup(arg)) == -1)
        {
          send_to_char("Invalid deity.  Please see 'help deity' for options.\n\r", ch);
          return;
        }
        new_type = CLAN_TYPE_FAITH;
      }
      else if(!str_cmp(arg, "greed"))
        new_type = CLAN_TYPE_GREED;
      else if(!str_cmp(arg, "malice"))
        new_type = CLAN_TYPE_MALICE;
      else if(!str_cmp(arg, "peace"))
        new_type = CLAN_TYPE_PEACE;
      else if(!str_cmp(arg, "chaos"))
        new_type = CLAN_TYPE_CHAOS;
      else
      {/* '\0' or wrong argument */
        send_to_char("Clan types: Law, Faith, Greed, Malice, Peace, Chaos.\n\r", ch);
        return;
      }
      if(ch->pcdata->clan_info->clan->type != 0)
      {/* Initiation, failure will deactivate type */
        CLAN_CHAR *cchar;
        if(ch->pcdata->clan_info->clan->type == new_type && ch->pcdata->clan_info->clan->enemy == new_enemy)
        {
          send_to_char("Your clan is already that type.\n\r", ch);
          return;
        }
        for(cchar = ch->pcdata->clan_info->clan->members; cchar; cchar = cchar->next)
          cchar->primary_merit = 0;/* With a type change, primary merit earned is wiped out */
        clan = ch->pcdata->clan_info->clan;
        while(clan->to_match)
        {/* No carrying over match merit */
          MERIT_TRACKER *temp = clan->to_match->next;
          free_merit(clan->to_match);
          clan->to_match = temp;
        }

        if(new_enemy)
          sprintf(buf, "Your clan type has been changed to %s with %s %s.\n\r",
            type, new_type == CLAN_TYPE_LAW ? "enemy type" : "deity", arg);
        else
          sprintf(buf, "Your clan type has been changed to %s.\n\r", arg);
        send_to_char(buf, ch);
        if(!IS_IMMORTAL(ch))
        {
          //send_to_char("1000 points have been deducted from your clan's tribute.\n\r", ch);
          //ch->pcdata->clan_info->clan->tribute -= 100000;
          //ch->pcdata->clan_info->clan->initiation = 100000; /* 1000 * 100 (It tracks in merit, not tribute) */
          //ch->pcdata->clan_info->clan->init_date = current_time;
          //send_to_char("You must complete initiation to secure your type.\n\rYou have 3 days to earn 100000 merit.\n\r", ch);
        }
      }
      else
      {
        if(new_enemy)
          sprintf(buf, "Your clan type has been set to %s with %s %s.\n\r",
            type, new_type == CLAN_TYPE_LAW ? "enemy type" : "deity", arg);
        else
          sprintf(buf, "Your clan type has been set to %s.\n\r", arg);
        send_to_char(buf, ch);
      }
      ch->pcdata->clan_info->clan->type = new_type;
      ch->pcdata->clan_info->clan->enemy = new_enemy;
      if(!ch->pcdata->clan_info->clan->initiation)
      {
        DESCRIPTOR_DATA *d;
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
          CHAR_DATA *victm;/* victm can == ch */

          victm = d->original ? d->original : d->character;

          if ( d->connected == CON_PLAYING && victm && victm->pcdata &&
            victm->pcdata->clan_info && victm->pcdata->clan_info->clan ==
            ch->pcdata->clan_info->clan)
          {
            if(new_type == CLAN_TYPE_FAITH && victm->pcdata->deity != new_enemy)
            {
              victm->pcdata->deity = new_enemy;
              victm->pcdata->deity_timer = 0;
              sprintf(buf,"To follow your clan's declared faith, you are now a follower of %s.\n\r",deity_table[new_enemy].pname);
              send_to_char(buf,victm);
            }
            set_clan_skills(victm);
            save_char_obj(victm);
          }
        }
      }
      save_clan(ch, TRUE, FALSE, TRUE);
    }
    else if(!str_cmp(arg, "color"))
    {
      char color;
      if(!IS_IMMORTAL(ch) && ch->pcdata->clan_info->rank != 5)
      {
        send_to_char("Only a clan leader may establish the clan's color.\n\r", ch);
        return;
      }
      argument = one_argument_cs(argument, arg);
      color = arg[0];
      switch(color)
      {
        case 'r': case 'b': case 'g': case 'y': case 'c': case 'm':
        case 'R': case 'B': case 'G': case 'Y': case 'C': case 'M':
        case 'x': case 'W': case 'D': break; /* x resets to base */
        default: send_to_char("Please input just the letter for the color code.  See 'help color' for a list.\n\r", ch); return;
      }
      if((!ch->pcdata->clan_info->clan->color[0] && color == 'x') ||
        ch->pcdata->clan_info->clan->color[1] == color)
        {
          send_to_char("That won't change anything.\n\r", ch);
          return;
        }
      argument = one_argument(argument, arg);
      if(!arg[0] || str_cmp(arg, "set"))
      {
        sprintf(buf, "Your clan will be {%c%s{x. Use 'establish color %c set' to finalize.\n\r",
          color, ch->pcdata->clan_info->clan->name,
          color);
        send_to_char(buf, ch);
        if(ch->pcdata->clan_info->clan->color[0])
        {
          send_to_char("It is currently free to switch colors but that will change later.\n\r", ch);
        }
      }
      else
      {/* Set it */
        ch->pcdata->clan_info->clan->color[0] = '{';
        ch->pcdata->clan_info->clan->color[1] = color;
        ch->pcdata->clan_info->clan->color[2] = '\0';
        sprintf(buf, "Your clan is now %s%s{x.\n\r",
          ch->pcdata->clan_info->clan->color, ch->pcdata->clan_info->clan->name);
        send_to_char(buf, ch);
        save_clan(ch, TRUE, FALSE, TRUE);
      }
    }
    else
    {
       send_to_char("Use rebel if you want to split from your current clan.\n\r", ch);
      send_to_char("Rebel is not currently functional but will be available later.\n\r", ch);
    }
    return;
  }
  argument = one_argument(argument, arg);
  if(!IS_IMMORTAL(ch) && ch->level > 20 && clan_table[ch->clan].true_clan == FALSE && !ch->pcdata->clan_info)
  {
    send_to_char("You are too high level to join the clan system, you may not establish a clan.\n\r", ch);
    return;
  }
  chars_only(arg);
  if(arg[0] == '\0')
  {
    send_to_char("Usage: establish <clan name>.  Please note the name is case sensitive.\n\r", ch);
    return;
  }
  if(strlen(arg) < 3)
  {
    send_to_char("Name too short, name must be at least 3 characters.\n\r", ch);
    return;
  }
  if(strlen(arg) > 8)
  {
    send_to_char("Name too long, maximum is 8 characters.\n\r", ch);
    return;
  }
  if(!check_legal_hall(ch->in_room))
  {
    send_to_char("You must be in a room that you could place your hall in to establish.\n\r", ch);
    return;
  }
  clan = NULL;
  /* Even immortals may not use an existing clan name */
  for(temp = clan_first; temp != NULL; temp = temp->next)
  {
    if(!str_cmp(arg, temp->name) && !temp->inactive)
    {
      send_to_char("A clan with that name already exists, please select another name.\n\r", ch);
      return;
    }
    clan = temp; /* Tracking the end */
  }
 // if(!IS_IMMORTAL(ch))/* Not yet. */
  {/* Immortals may override the blocked names */
    for(i = 0; i < MAX_CLAN; i++)
    {
      if(!str_cmp(arg, clan_table[i].name))
      {
        send_to_char("That is a reserved name, please select another.\n\r", ch);
        return;
      }
    }
    if(!str_cmp(arg, "gladiator") || !str_cmp(arg, "barbarian"))
    {
      send_to_char("That is a reserved name, please select another.\n\r", ch);
      return;
    }
  }
  if(!argument[0] || str_prefix(argument, "create"))
  {
    for(i = 0; i < strlen(arg); i++)
    {
      if(arg[i] >= 'A' && arg[i] <= 'Z')
        break;
    }
    if(i == strlen(arg))
      arg[0] = UPPER(arg[0]);
    sprintf(buf, "Your new clan's name will be %s.\n\rUse 'establish %s create' to create it.\n\r", arg, arg);
    send_to_char(buf, ch);
    return;
  }
  if(!ch->pcdata->clan_info || ch->pcdata->clan_info->clan->default_clan)
  {
    if(!clan)
    {
      bug("No default clans loaded.", 0);
      clan = new_clan();
      clan->next = clan_first;
      clan_first = clan;
      clan->vnum_min = 1;
      clan->vnum_max = 100;
    }
    else
    {
      int open_vnum = 1;
      /* Not trying to fill the gaps right now */
      for(temp = clan_first; temp != NULL; temp = temp->next)
      {
        if(temp->default_clan)
          continue;
        if(temp->vnum_min >= open_vnum)
        {
          open_vnum = temp->vnum_min + 100;
          continue;
        }
      }
      if(!temp)
      {
        clan->next = new_clan();
        clan = clan->next;
      }
      else
      {
        clan = new_clan();
        clan->next = temp->next;
        temp->next = clan;
      }
      clan->vnum_min = open_vnum;
      clan->vnum_max = open_vnum + 99;
    }
    for(i = 0; i < strlen(arg); i++)
    {
      if(arg[i] >= 'A' && arg[i] <= 'Z')
        break;
    }
    if(i == strlen(arg))
      arg[0] = UPPER(arg[0]);
    clan->name = str_dup(arg);
    save_clan_list();
    clan->creation = current_time;
    add_clan_member(clan, ch, 5);
    if(!IS_IMMORTAL(ch))
    {
      if(ch->pcdata->clan_info->award_merit)
      {
        sprintf(buf, "%s created.  Skipping initiation due to bonus merit.\n\r", clan->name);
        send_to_char(buf, ch);
      }
      else
      {
        clan->initiation = 100000;
        if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->default_clan == CLAN_OUTCAST)
        {
          clan->initiation += (50000 - ch->pcdata->clan_info->merit) * 2;
          ch->pcdata->clan_info->merit = 0;
        }
        clan->init_date = current_time;
        sprintf(buf, "%s created.  You must complete initiation to establish your clan.\n\rYou and your clanmates have 3 days to earn %d merit.\n\r", clan->name, clan->initiation);
        send_to_char(buf, ch);
      }
    }
  }
  else
  {
    clear_string(&ch->pcdata->clan_info->clan->name, arg);
    ch->pcdata->clan_info->clan->inactive = FALSE; /* Reactivate */
    if(clan->type > 0)
      clan->type = clan->type * -1; /* Type is cleared on reactivation */
    clan->initiation = 1000000; /* 10,000 * 100 (It tracks in merit, not points) */
    clan->init_date = current_time;
    sprintf(buf, "%s reactivated, type has been cleared.\n\rYou must complete initiation to secure your clan.\n\rYou have 3 days to earn 10000 merit.\n\r", clan->name);
    send_to_char(buf, ch);
  }
  save_clan(ch, TRUE, FALSE, TRUE);
}

void save_clan_list(void)
{
  CLAN_DATA *clan;
  char csave[256];
  FILE *fp;
  sprintf( csave, "%s%s", CLAN_DIR, "clan_list" );
  if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
  {
    bug( "Save_clan_list: fopen", 0 );
    perror( csave );
    return;
  }
  for(clan = clan_first; clan != NULL; clan = clan->next)
  {
    fprintf(fp, "%s %d\n", clan->name, clan->default_clan);
  }
  fprintf(fp, "#End 0");
  fclose(fp);
  rename(TEMP_FILE, csave);
}

void update_clan_version(CLAN_DATA *clan)
{
  bool clan_update = FALSE;
  bool hall_update = FALSE;
  if(clan->version >= CLAN_FILE_VERSION)
    return; /* No updates */
  if(clan->version < 1)
  {/* Recalculate all earned tribute based on current members */
  /* Only works because nobody has outcast yet since it's early in the system */
    CLAN_CHAR *cchar;
    PLAN_DATA *obj;
    if(clan->default_clan)
      return;
    clan_update = TRUE;
    clan->tribute = 0;
    clan->max_tribute = 0;
    clan->awarded_tribute = 0;
    for(cchar = clan->members; cchar; cchar = cchar->next)
      clan->tribute += cchar->donated;
    clan->max_tribute = clan->tribute;
    calculate_award_tribute(clan);
    /* Restored, now subtract their placed items */
    if(clan->planned)
      hall_update = TRUE; /* If it has no planned items, don't save the hall */
    for(obj = clan->planned; obj; obj = obj->next)
    {
      /* Calculate item cost fresh, this has been corrected too */
      if(IS_SET(obj->type, PLAN_ROOM) && IS_SET(obj->flags, PLAN_ROOM_LAB))
      {
        if(obj->opt[0] > PRICE_LAB_COUNT)
          obj->opt[0] = PRICE_LAB_COUNT;
      }
      set_obj_cost(NULL, obj, TRUE, TRUE);
      if(IS_SET(obj->type, (PLAN_PREVIEWED | PLAN_PLACED)))
        clan->tribute -= obj->cost * 100;/* Cost is in tribute */
    }
  }
  if(clan->version < 2)
  {/* Fix index collisions */
    PLAN_DATA *obj;
    CLAN_DATA *check = clan_first;
    for(; check != clan; check = check->next)
    {/* Check earlier clans for a match */
      if(check->vnum_min == clan->vnum_min)
      {/* Collision, reset this clan's vnum to as high as needed */
        bug("Clan vnum collision found, correcting.", 0);
        int max = 1;
        for(check = clan_first; check; check = check->next)
        {
          if(check->vnum_min >= max)
            max = check->vnum_min + 100;
        }
        clan->vnum_min = max;
        clan->vnum_max = clan->vnum_min + 99;
        for(obj = clan->planned; obj; obj = obj->next)
        {
          obj->plan_index = obj->plan_index % 100 + clan->vnum_min - 1;
          if(obj->loc > 0)
            obj->loc = obj->loc % 100 + clan->vnum_min - 1;
        }
        break;
      }
    }
    hall_update = TRUE;
    clan_update = TRUE;
  }
  /* Be sure to update CLAN_FILE_VERSION if this is updated */
  /* Save so it doesn't have to run this update every time until someone triggers a save */
  if(hall_update)
    save_hall(clan->name, clan->planned, TRUE);
  if(clan_update)
    do_save_clan(clan);
}

void load_clans(void)
{
  CLAN_DATA *clan;
  char cload[256];
  char input[MAX_STRING_LENGTH];
  int default_clan;
  FILE *fp;
  bool outcast = FALSE, loner = FALSE;
  sprintf( cload, "%s%s", CLAN_DIR, "clan_list" );
  if((fp = fopen(cload, "r")) == NULL)
  {
    bug("No clan list found to load.", 0);
  }
  else
  {
    fscanf(fp, "%s %d", input, &default_clan);
    while(!feof(fp) && str_cmp(input, "#End"))
    {
      load_clan(input, default_clan);
      fscanf(fp, "%s %d", input, &default_clan);
    }
    if(str_cmp(input, "#End"))
      bug("Unexpected end of file for loading clans.", 0);
    fclose(fp);
  }
  for(clan = clan_first; clan != NULL; clan = clan->next)
  {
    if(!loner && !str_cmp(clan->name, "Loner"))
      loner = TRUE;
    else if(!outcast && !str_cmp(clan->name, "Outcast"))
      outcast = TRUE;
    update_clan_version(clan);/* Even on the default clans, in case it impacts their members */
  }
  if(!loner)
  {
    clan = new_clan();
    clan->default_clan = CLAN_LONER;
    clan->name = str_dup("Loner");
    clan->next = clan_first;
    clan_first = clan;
  }
  if(!outcast)
  {
    clan = new_clan();
    clan->default_clan = CLAN_OUTCAST;
    clan->name = str_dup("Outcast");
    clan->next = clan_first;
    clan_first = clan;
  }
}

void fread_clan_messages(FILE *fp, CLAN_DATA *clan)
{
  CLAN_CHAR *member;
  char input[256];
  while(!feof(fp))
  {
    fscanf(fp, "%s", input);
    if(!str_cmp(input, "#End"))
      return; /* Done */
    if(!str_cmp(input, "Msg"))
    {
      fscanf(fp, "%s", input);
      for(member = clan->members; member != NULL; member = member->next)
      {
        if(!member->messages && !str_cmp(member->name, input))
        {
          read_to_tilde(fp, &member->messages);
          break;
        }
      }
    }
    else
      fread_to_eol(fp);
  }
  bug("Unexpected end to clan message file found.", 0);
}

void load_clan(char *clan_name, int def_clan)
{
  CLAN_DATA *clan;
  char hload[256];
  char input[MAX_STRING_LENGTH];
  int in_int;
  FILE *fp;
  CLAN_CHAR *cchar = NULL;
  bool first_end = FALSE;
  bool done = FALSE;
  sprintf( hload, "%s%s.clan", CLAN_DIR, capitalize(clan_name));
  if((fp = fopen(hload, "r")) == NULL)
  {
    sprintf(hload, "Failed to open clan file for %s", clan_name);
    bug(hload, 0);
    return;
  }
  clan = new_clan();
  clan->default_clan = def_clan;
  if(!clan_first)
    clan_first = clan;
  else
  {
    if(def_clan)
    {/* Defaults go at the start */
      clan->next = clan_first;
      clan_first = clan;
    }
    else
    {
      clan->next = clan_first;
      while(clan->next->next)
        clan->next = clan->next->next;
      clan->next->next = clan;
      clan->next = NULL;
    }
  }
  clan->name = str_dup(clan_name);
  while(!feof(fp) && !done)
  {
    bool found = FALSE;
    fscanf(fp, "%s", input);
    switch(input[0])
    {
      case '#': if(!str_cmp(input, "#End"))
                {/* There should be two ends */
                  if(!first_end)
                    first_end = TRUE;
                  else
                    done = TRUE;
                  found = TRUE;
                }
                break;
      case 'A': if(!str_cmp(input, "Ally"))
      {
        ALLIANCE_DATA *ally;
        CLAN_DATA *target;
        found = TRUE;
        for(target = clan_first; target != NULL; target = target->next)
        {
          if(!str_cmp(target->name, input))
            break;
        }
        if(!target)
        {
          bug("Clan not found to ally with.", 0);
          break;
        }
        ally = new_ally();
        if(clan->allies)
        {
          ally->next = clan->allies;
          while(ally->next->next)
            ally->next = ally->next->next;
          ally->next->next = ally;
          ally->next = NULL;
        }
        else
          clan->allies = ally;
        ally->clan = target;
        fscanf(fp, "%d %d %d %d %d %d", &ally->cost, &ally->bribe,
          &ally->to_pay, &in_int, &ally->offer_duration, &ally->duration);
        if(in_int)
          ally->pending = TRUE;
      }
      else if(!str_cmp(input, "Assists")) {
          fscanf(fp, "%d", &clan->assists); found = TRUE; }
      else if(!str_cmp(input, "AwardMerit"))
      {
        if(!cchar)
          bug("No clan char for setting award merit.", 0);
        else
          fscanf(fp, "%d", &cchar->award_merit);
        found = TRUE;
      }
      else if(!str_cmp(input, "ATribute")) {
        fscanf(fp, "%d", &clan->awarded_tribute); found = TRUE; }
      break;
      case 'C': if(!str_cmp(input, "Charter")) {
          read_to_tilde(fp, &clan->charter); found = TRUE; }
          if(!str_cmp(input, "CreateDate")) {
            fscanf(fp, "%ld", &clan->creation); found = TRUE; }
          if(!str_cmp(input, "Color")) {
            fscanf(fp, "%s", input);
            clan->color[0] = input[0]; clan->color[1] = input[1];
            clan->color[2] = '\0';
            found = TRUE; }
        break;
      case 'D': if(!str_cmp(input, "Donated"))
        {
          if(!cchar)
            bug("No clan char for setting donated.", 0);
          else
            fscanf(fp, "%d", &cchar->donated);
          found = TRUE;
        }
        break;
      case 'E': if(!str_cmp(input, "Enemy")) {
          fscanf(fp, "%d", &clan->enemy); found = TRUE; }
      break;
      case 'F': if(!str_cmp(input, "Flags"))
        {
          if(!cchar)
            bug("No clan char for setting flags.", 0);
          else
            fscanf(fp, "%ld", &cchar->flags);
          found = TRUE;
        }
        break;
      case 'H': if(!str_cmp(input, "Hall")) {
          fscanf(fp, "%d", &clan->hall_index);
          found = TRUE;
        }
        break;
      case 'I': if(!str_cmp(input, "Ini")) {
          fscanf(fp, "%d", &clan->initiation); found = TRUE; }
          if(!str_cmp(input, "IniDate")) {
            fscanf(fp, "%ld", &clan->init_date); found = TRUE; }
        break;
      case 'J': if(!str_cmp(input, "Join"))
        {
          if(!cchar)
            bug("No clan char for setting join.", 0);
          else
            fscanf(fp, "%ld", &cchar->join_date);
          found = TRUE;
        }
        break;
      case 'K': if(!str_cmp(input, "Kills")) {
          fscanf(fp, "%d", &clan->kills); found = TRUE; }
        break;
      case 'L': if(!str_cmp(input, "Login"))
        {
          if(!cchar)
            bug("No clan char for setting login.", 0);
          else
            fscanf(fp, "%ld", &cchar->last_login);
          found = TRUE;
        }
        break;
      case 'R': if(!str_cmp(input, "Rules")) {
          read_to_tilde(fp, &clan->rules); found = TRUE; }
        else if(!str_cmp(input, "Rank"))
        {
          if(!cchar)
            bug("No clan char for setting rank.", 0);
          else
          {
            fscanf(fp, "%d", &cchar->rank);
            if(clan->default_clan)
              cchar->rank = 0;/* No rank for loners or outcasts */
            else if(cchar->rank == 5)
              clan->leaders++;
          }
          found = TRUE;
        }
        break;
      case 'T': if(!str_cmp(input, "Type")) {
          fscanf(fp, "%d", &clan->type); found = TRUE; }
        else if(!str_cmp(input, "Tribute")) {
          fscanf(fp, "%d", &clan->tribute); found = TRUE; }
        break;
      case 'M': if(!str_cmp(input, "Member")) {
          cchar = new_clan_char();
          if(!clan->members)
            clan->members = cchar;
          else
          {
            cchar->next = clan->members;
            while(cchar->next->next)
              cchar->next = cchar->next->next;
            cchar->next->next = cchar;
            cchar->next = NULL;
          }
          cchar->clan = clan;
          fscanf(fp, "%s", input);
          cchar->name = str_dup(input);
          found = TRUE;
        }
        else if(!str_cmp(input, "Merit"))
        {
          if(!cchar)
            bug("No clan char for setting merit.", 0);
          else
            fscanf(fp, "%d", &cchar->merit);
          found = TRUE;
        }
        else if(!str_cmp(input, "MeritMatch"))
        {
          MERIT_TRACKER *cur, *last = NULL;
          int amount = -1, expire = -1;
          fscanf(fp, "%d %d", &amount, &expire);
          while(!feof(fp) && amount >= 0 && expire >= 0 &&
            (amount != 0 || expire != 0))
          {
            cur = new_merit();
            cur->amount = amount;
            cur->expire = expire;
            cur->next = NULL;
            if(!last)
            {
              clan->to_match = cur;
              last = cur;
            }
            else
            {
              last->next = cur;
              last = last->next;
            }
            fscanf(fp, "%d %d", &amount, &expire);
          }
          if(feof(fp))
            bug("Bad end to merit match", 0);
          found = TRUE;
        }
        else if(!str_cmp(input, "MeritLost"))
        {
          if(!cchar)
            bug("No clan char for setting merit lost.", 0);
          else
            fscanf(fp, "%d", &cchar->lost_merit);
          found = TRUE;
        }
        else if(!str_cmp(input, "MeritTrack"))
        {
          if(!cchar)
            bug("No clan char for setting delay merit.", 0);
          else
          {
            MERIT_TRACKER *cur, *last = NULL;
            int amount = -1, expire = -1;
            fscanf(fp, "%d %d", &amount, &expire);
            while(!feof(fp) && (amount != 0 || expire != 0))
            {
              cur = new_merit();
              cur->amount = amount;
              cur->expire = expire;
              if(!last)
              {
                cchar->delay_merit = cur;
                last = cur;
              }
              else
              {
                last->next = cur;
                last = last->next;
              }
              fscanf(fp, "%d %d", &amount, &expire);
            }
            if(feof(fp))
              bug("Bad end to merit delay", 0);
          }
          found = TRUE;
        }
        else if(!str_cmp(input, "MeritPrimary"))
        {
          if(!cchar)
            bug("No clan char for setting merit primary.", 0);
          else
            fscanf(fp, "%d", &cchar->primary_merit);
          found = TRUE;
        }
        else if(!str_cmp(input, "MeritBank"))
        {
          if(!cchar)
            bug("No clan char for setting merit bank.", 0);
          else
            fscanf(fp, "%d", &cchar->banked_merit);
          found = TRUE;
        }
        else if(!str_cmp(input, "MinVnum")) {
          fscanf(fp, "%d", &clan->vnum_min); found = TRUE; }
        else if(!str_cmp(input, "MaxVnum")) {
          fscanf(fp, "%d", &clan->vnum_max); found = TRUE; }
        else if(!str_cmp(input, "MTribute")) {
          fscanf(fp, "%d", &clan->max_tribute); found = TRUE; }
        break;
      case 'V':
        if(!str_cmp(input, "Version")) {
          fscanf(fp, "%d", &clan->version); found = TRUE; }
        break;
    }
    if(!found)
      fread_to_eol(fp);
  }
  if(!done)
    bug("Bad first end to clan file found.", 0);
  fclose(fp);
  if(!def_clan)
  {
    sprintf( hload, "%s%s.hall", CLAN_DIR, capitalize(clan_name));
    if((fp = fopen(hload, "r")) != NULL)
    {
      PLAN_DATA *obj;
      fread_clan_hall(fp, &clan->planned, clan);
      for(obj = clan->planned; obj != NULL; obj = obj->next)
      {
        if(IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
        {
          respawn_plan_obj(obj, clan->planned, TRUE);
          if(IS_SET(obj->type, PLAN_ITEM) && IS_SET(obj->flags, PLAN_ITEM_PORTAL))
          {
            PLAN_DATA *room = find_edit_obj_by_index(clan->planned, PLAN_ROOM, obj->loc);
            if(room && room->to_place)
              update_room_sign(clan, (ROOM_INDEX_DATA*)room->to_place);
          }
        }
      }
      fclose(fp);
    }
  }
  sprintf( hload, "%s%s.msgs", CLAN_DIR, capitalize(clan_name));
  if((fp = fopen(hload, "r")) != NULL)
  {/* Any stored messages for the players in the clan */
    fread_clan_messages(fp, clan);
    fclose(fp);
  }
}

/* Do the actual saving of the clan */
void do_save_clan(CLAN_DATA *clan)
{
  char csave[256];
  FILE *fp;
  CLAN_CHAR *list;

  /* Lowest priority first - if there's a crash in the second half, this doesn't matter much */
  fp = NULL;
  for(list = clan->members; list != NULL; list = list->next)
  {
    if(list->messages)
    {
      if(!fp)
      {
        fp = fopen(TEMP_FILE2, "w");
        if(fp == NULL)
          break;
      }
      fprintf(fp, "Msg %s %s~\n", list->name, list->messages);
    }
  }
  if(fp)
  {
    fprintf(fp, "#End\n");
    fclose(fp);
    sprintf( csave, "%s%s.msgs", CLAN_DIR, capitalize(clan->name));
    rename(TEMP_FILE2, csave);
    fp = NULL;
  }

  sprintf( csave, "%s%s.clan", CLAN_DIR, capitalize(clan->name));
  if ( ( fp = fopen( TEMP_FILE2, "w" ) ) == NULL )
  {
    bug( "Save_clan clan: fopen", 0 );
    perror( csave );
    return;
  }

  /* Store the clan details */
  fprintf(fp, "Type %d\n", clan->type);
  fprintf(fp, "Enemy %d\n", clan->enemy);
  fprintf(fp, "Version %d\n", CLAN_FILE_VERSION);
  fprintf(fp, "MinVnum %d\n", clan->vnum_min);
  fprintf(fp, "MaxVnum %d\n", clan->vnum_max);
  fprintf(fp, "Tribute %d\n", clan->tribute);
  fprintf(fp, "MTribute %d\n", clan->max_tribute);
  fprintf(fp, "ATribute %d\n", clan->awarded_tribute);
  fprintf(fp, "Kills %d\n", clan->kills);
  fprintf(fp, "Assists %d\n", clan->assists);
  if(clan->hall)
    fprintf(fp, "Hall %d\n", clan->hall->plan_index);
  if(clan->color[0] && clan->color[1])
  {
    clan->color[2] = 0;
    fprintf(fp, "Color %s\n", clan->color);
  }
  if(clan->initiation)
  {
    fprintf(fp, "Ini %d\n", clan->initiation);
    fprintf(fp, "IniDate %ld\n", clan->init_date);
  }
  fprintf(fp, "CreateDate %ld\n", clan->creation);
  if(clan->to_match)
  {
    fprintf(fp, "MeritMatch");
    MERIT_TRACKER *mtrack = clan->to_match;
    for(; mtrack != NULL; mtrack = mtrack->next)
    {
      fprintf(fp, " %d %d", mtrack->amount, mtrack->expire);
    }
    fprintf(fp, " 0 0\n");
  }
  /* Write the clan members */
  for(list = clan->members; list != NULL; list = list->next)
  {
    fprintf(fp, "Member %s\n", list->name);
    fprintf(fp, "Rank %d\n", list->rank);
    fprintf(fp, "Join %ld\n", list->join_date);
    fprintf(fp, "Login %ld\n", list->last_login);
    fprintf(fp, "Donated %d\n", list->donated);
    fprintf(fp, "Merit %d\n", list->merit);
    fprintf(fp, "MeritPrimary %d\n", list->primary_merit);
    fprintf(fp, "MeritBank %d\n", list->banked_merit);
    fprintf(fp, "AwardMerit %d\n", list->award_merit);
    fprintf(fp, "Flags %ld\n", list->flags);
    if(list->delay_merit)
    {
      fprintf(fp, "MeritTrack");
      MERIT_TRACKER *mtrack = list->delay_merit;
      for(; mtrack != NULL; mtrack = mtrack->next)
      {
        fprintf(fp, " %d %d", mtrack->amount, mtrack->expire);
      }
      fprintf(fp, " 0 0\n");
    }
    if(list->lost_merit)
      fprintf(fp, "MeritLost %d\n", list->lost_merit);
  }
  fprintf( fp, "#End\n" );
  /* Write the charter and rules */
  if(clan->charter)
    fprintf(fp, "Charter %s~\n", clan->charter);
  if(clan->rules)
    fprintf(fp, "Rules %s~\n", clan->rules);
  if(clan->allies)
  {
    ALLIANCE_DATA *ally;
    for(ally = clan->allies; ally != NULL; ally = ally->next)
    {
      fprintf(fp, "Ally %s %d %d %d %d %d %d\n", ally->clan->name, ally->cost,
        ally->bribe, ally->to_pay, ally->pending ? 1 : 0, ally->offer_duration, ally->duration);
    }
  }
  fprintf(fp, "#End\n");
  fclose( fp );
  rename(TEMP_FILE2, csave);
}

bool save_hall(char *clan_name, PLAN_DATA *plans, bool save_immediately)
{
  char hsave[255];
  PLAN_DATA *obj;
  FILE *fp = NULL;
  sprintf( hsave, "%s%s%s", CLAN_DIR, capitalize(clan_name), ".hall" );
  if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
  {
    bug( "Save_clan hall: fopen", 0 );
    perror( hsave );
    return FALSE;
  }
  /* Cycle through all the plans */
  for(obj = plans; obj != NULL; obj = obj->next)
  {
    fwrite_plan_obj(fp, obj);
  }
  /* Write the plan exits */
  for(obj = plans; obj != NULL; obj = obj->next)
  {
    if(!IS_SET(obj->type, (PLAN_ROOM | PLAN_PLACED)) == (PLAN_ROOM | PLAN_PLACED)
      || obj->to_place == NULL)
      continue;
    fwrite_room_exits(fp, obj);
  }
  fprintf( fp, "#End\n" );
  fclose( fp );
  if(save_immediately)
    rename(TEMP_FILE, hsave);
  return TRUE;
}

void save_clan(CHAR_DATA *ch, bool save_c, bool save_h, bool hedit)
{/* If hedit is false, save_clan means save_char */
  /* Save both into temp files, then after the save is done replace both temp files */
  /* This minimizes the chance of a desync from a bad crash timing between a clan
   * payment and the associated hall upgrade - only needed if both saves are TRUE */
  /* set backup_needed to TRUE if it's a clan modified, or in the player for their hall */
  /* Remove the has updated flag on the ch if hall is saved, that flag is only used for that */
  if(!ch || IS_NPC(ch))
    return;
  if(hedit)
  {
    //FILE *fp = NULL;
    char hsave[256];
    if(save_h)
    {
      sprintf( hsave, "%s%s%s", CLAN_DIR, capitalize(ch->pcdata->clan_info->clan->name), ".hall" );
      ch->pcdata->edits_made = FALSE;
      if(!save_hall(ch->pcdata->clan_info->clan->name, START_OBJ(ch, hedit), FALSE))
        return;
    }
/* clan save is similar, but saves clan into TEMP_FILE2 */
    if(save_c)
      do_save_clan(ch->pcdata->clan_info->clan);
    if(save_h)
      rename(TEMP_FILE, hsave);
  }
  else
  {
  }
}

