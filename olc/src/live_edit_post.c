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
#include "gc.h"
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"

DECLARE_DO_FUN(do_help   );
int liq_lookup (const char *name);
int ispunct(int val);
int isdigit(int val);
void do_text(CHAR_DATA *ch, char *argument);
void modify_room_marker(CLAN_DATA *clan, ROOM_INDEX_DATA *room, int dir, bool place);

extern   sh_int  rev_dir[];
extern CLAN_DATA *clan_first;

AREA_DATA clan_area;

int get_price(int index, bool hedit)
{
  if(index < 0)
    return -2;
  if(index >= PRICE_TOTAL)
    return -1;
  if(hedit)
    return price_table[index].point_cost;
  return price_table[index].egg_cost;
}

#define PRICE_STR(index, type) get_price(index, type), type ? "points" : "shards"
//#define PRICE_STR(index, type)  index < 0 ? -2, "" : index >= PRICE_TOTAL ? -1, "" : type ? price_table[index].point_cost, "points" : price_table[index].egg_cost, "shards"
#define GET_REGEN(start, bonus) (start + bonus <= PRICE_R_REGEN_MID ? 100 + (start + bonus) * 25 : 100 + PRICE_R_REGEN_MID * 25 + (start + bonus - 4) * 10)
#define COST_STR(amount, type) amount, type ? "points" : "shards"
#define GET_PRICE(index, type) (index < 0 ? -2 : index >= PRICE_TOTAL ? -1 : type ? price_table[index].point_cost : price_table[index].egg_cost)

#define START_OBJ(ch, hedit) (hedit ? (ch)->pcdata->clan_info->clan->planned : (ch)->pcdata->clan_info->pers_plan)

bool check_legal_hall(ROOM_INDEX_DATA *room)
{
  RESET_DATA *pReset;
  int iClass, iGuild, i;
  bool exitfound = FALSE;
  if(!room || !room->area)
    return FALSE;
  /* Check for resets */
  for(pReset = room->area->reset_first; pReset != NULL; pReset = pReset->next)
  {
    if(get_room_index(pReset->arg3) == room)
      return FALSE;
  }
  /* Check for restrictions */
  /* Can't be a clan room */
  if(room->heal_rate != 100 || room->mana_rate != 100)
    return FALSE;
  if(room->clan)
    return FALSE;
  /* No flags */
  if (IS_SET(room->room_flags, ROOM_IMP_ONLY))
    return FALSE;
  if (IS_SET(room->room_flags, ROOM_GODS_ONLY))
    return FALSE;
  if (IS_SET(room->room_flags, ROOM_HEROES_ONLY))
    return FALSE;
  if (IS_SET(room->room_flags,ROOM_NEWBIES_ONLY))
    return FALSE;
  if (IS_SET(room->room_flags,ROOM_NOCLAN))
    return FALSE;
  if (IS_SET(room->room_flags,ROOM_CLANONLY))
    return FALSE;
  if ( IS_SET(room->room_flags, ROOM_PRIVATE))
    return FALSE;
  if ( IS_SET(room->room_flags, ROOM_SOLITARY))
    return FALSE;
  /* No owner */
  if(room->owner != NULL && room->owner[0] != '\0')
    return FALSE;
  /* All exits from here must link back properly */
  for(i = 0; i < 6; i++)
  {
    if(room->exit[i])
    {
      exitfound = TRUE;
      if((!room->exit[i]->u1.to_room->exit[rev_dir[i]] ||
      room->exit[i]->u1.to_room->exit[rev_dir[i]]->u1.to_room != room))
        return FALSE;/* Clean exits only */
    }
  }
  if(!exitfound)
    return FALSE;
  /* Can't be a guild room */
  for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
  {
      for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)
      {
        if(room->vnum == class_table[iClass].guild[iGuild])
          return FALSE;
      }
  }
  /* All clear */
  return TRUE;
}

/* Allow clan leaders to review their own halls too */
void do_review(CHAR_DATA *ch, char *argument)
{
  char arg[256];
  if(IS_NPC(ch))
    return;
  argument = one_argument(argument, arg);
  if(IS_IMMORTAL(ch))
  {
//  review <clan|exitlink|all> <new|recent|flagged|all>
/* Add flagging capability to review */
/* Later updates may need to track last reviewed, if that occurs be sure to go
   through and verify the tracking is cleared whenever an item or clan is
   deleted */
    PLAN_DATA *to_rev;
    CLAN_DATA *rev_clan;
    for(rev_clan = clan_first; rev_clan != NULL; rev_clan = rev_clan->next)
    {
      for(to_rev = rev_clan->planned; to_rev != NULL; to_rev = to_rev->next) 
      {
        if(IS_SET(to_rev->type, PLAN_PLANNED) && !to_rev->reviewed)
        {/* Show the imm the details, flag it reviewed */
          char buf[256];
          sprintf(buf, "Label: %s\n\rName: %s\n\r", to_rev->label, to_rev->name);
          send_to_char(buf, ch);
          switch(to_rev->type & PLAN_MASK_TYPE)
          {
            case PLAN_ROOM: send_to_char("Desc:\n\r", ch);
                  send_to_char(to_rev->desc, ch);
                send_to_char("\n\r", ch);
                if(to_rev->exits)
                {/* Only shows on placed objects, but you can stat a placed room */
                  int i;
                  for(i = 0; i < 6; i++)
                  {
                    if(to_rev->exits[i].outside)
                    {
                      sprintf(buf, "Exits outside to %s [%d]\n\r", to_rev->exits[i].outside->name, to_rev->exits[i].outside->vnum);
                      send_to_char(buf, ch);
                    }
                  }
                }
                break;
            case PLAN_MOB:sprintf(buf, "Short: %s\n\rLong: %s\n\r", to_rev->short_d, to_rev->long_d);
                    send_to_char(buf, ch);
                    send_to_char("Desc:\n\r", ch);
                    send_to_char(to_rev->desc, ch);
                    send_to_char("\n\r", ch);
                    break;
            case PLAN_ITEM:sprintf(buf, "Short: %s\n\rLong: %s\n\r", to_rev->short_d, to_rev->long_d);
                     send_to_char(buf, ch);
                     break;
            case PLAN_EXIT:
                     break;
          }
          to_rev->reviewed = TRUE;
          save_hall(rev_clan->name, rev_clan->hall, TRUE);
          /* Include outside room details if it's an outside link */
          return;
        }
      }
    }
    send_to_char("There are no unreviewed clan objects at this time.\n\r", ch);
    return;
  }
/* Blocked for morts for now */
  send_to_char("Only immortals may use review at this time.\n\r", ch);
  return;
  if(ch->pcdata->clan_info && ch->pcdata->clan_info->rank == MAX_RANK)
  {
    return;
  }
  send_to_char("You are not the leader of a clan, you may not use review.\n\r", ch);
}

void verify_price_table(void)
{
  int i;
  for(i = 0; i <= PRICE_TOTAL; i++)
  {
    if(price_table[i].value != i)
    {/* We don't know whether it could cause an overrun or just bad pricing, but it's a problem either way */
      bug("Pricing table validation failure. CORRECT IMMEDIATELY, this could cause a crash.", 0);
      return;
    }
  }
}

void to_chars_in_room(char *msg, ROOM_INDEX_DATA *room)
{
  CHAR_DATA *in_room;
  if(!room)
    return;
  for(in_room = room->people; in_room != NULL; in_room = in_room->next_in_room)
  {
    send_to_char(msg, in_room);
  }
}

void edit_stop(CHAR_DATA *ch)
{/* Stops editing, in its own function so it can be easily used for if a player quits */
  if(ch->pcdata == NULL || !ch->pcdata->edit_flags)
    return;
  if(IS_SET(ch->pcdata->edit_flags, EDITMODE_DESC))
  {/* They're editing a description, wrap that up */
    REMOVE_BIT(ch->pcdata->edit_flags, EDITMODE_DESC);
    if(ch->pcdata->edit_obj)
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
  }
  if(ch->pcdata->edit_obj)
  {
    ch->pcdata->edit_obj->editing = FALSE;
    end_long_edit(ch, &ch->pcdata->edit_obj->desc);
    ch->pcdata->edit_obj = NULL;
  }
  if(ch->pcdata->edits_made)
  {  /* Save the area if modified */
    ch->pcdata->edits_made = FALSE;
    /* Don't need to save the values, anything updating money should have saved
     * when the change was made */
    save_clan(ch, FALSE, TRUE,
        IS_SET(ch->pcdata->edit_flags, EDITMODE_HALL) ? TRUE : FALSE);
  }
  ch->pcdata->edit_flags = 0;
  send_to_char("Editing stopped, the area has been saved.\n\r", ch);
}

int count_edit_obj(CHAR_DATA *ch, int flag, bool verbose, bool hedit)
{
  BUFFER *output = NULL;
  PLAN_DATA *obj = START_OBJ(ch, hedit);
  int count = 0;
  if(verbose)
    output = new_buf();
  for(; obj != NULL; obj = obj->next)
  {
    if(IS_SET(obj->type, flag) == flag)
    {/* Matches all requested filters */
      count++;
      if(verbose)
      {
        char buf[256];
        char word[10];
        switch(obj->type & PLAN_MASK_TYPE)
        {
          case PLAN_ROOM: strcpy(word, "Room"); break;
          case PLAN_ITEM: strcpy(word, "Item"); break;
          case PLAN_MOB: strcpy(word, "Mob"); break;
          case PLAN_EXIT: strcpy(word, "Exit"); break;
          default: strcpy(word, "Unknown"); break;
        }
        sprintf(buf, "%s: %s, %s, Cost: %d points.\n\r", word, obj->label,
            (obj->type & PLAN_PLACED) == PLAN_PLACED ? "placed" : ((obj->type & PLAN_PREVIEWED) == PLAN_PREVIEWED ? "previewed" : "planned"), obj->cost);
        add_buf(output, buf);
      }
    }
  }
  if(verbose)
  {
    page_to_char(buf_string(output),ch);
    free_buf(output);
  }
  return count;
}

PLAN_DATA *find_edit_obj(CHAR_DATA *ch, char *arg, bool hedit)
{
  PLAN_DATA *obj = START_OBJ(ch, hedit);
  for(; obj != NULL; obj = obj->next)
  {
    if(!str_cmp(obj->label, arg))
      return obj;
  }
  return NULL;
}

PLAN_DATA *find_edit_obj_by_index(PLAN_DATA *start, int type, int plan_index)
{
  PLAN_DATA *obj = start;
  for(; obj != NULL; obj = obj->next)
  {
    if(IS_SET(obj->type, type) == type && obj->plan_index == plan_index)
      return obj;
  }
  return NULL;
}

PLAN_DATA *find_char_room_obj(CHAR_DATA *ch, bool hedit)
{
  PLAN_DATA *obj = START_OBJ(ch, hedit);
  for(; obj != NULL; obj = obj->next)
  {
    if(IS_SET(obj->type, PLAN_ROOM) && IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED))
        && obj->to_place && obj->to_place == ch->in_room)
      return obj;
  }
  return NULL;
}

/* Remove punctuation and numbers */
void blast_punctuation(char *arg)
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
    if(ispunct(arg[j]))
    {
      i--;/* Hold i still */
      continue;
    }
    arg[i] = LOWER(arg[j]);
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

void lower_only(char *arg)
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
    arg[i] = LOWER(arg[j]);
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

int calc_cost_range(int start, int count, bool hedit)
{
  int i, cost;
  if(count <= 0)
    return 0;
  if(start + count >= PRICE_TOTAL)
  {
    bug("Cost range overrun.", 0);
    return -4;
  }
  cost = 0;
  for(i = start; i < start + count; i++)
  {
    cost += GET_PRICE(start + i, hedit);
  }
  return cost;
}

/* ch not used currently but available in case clan is needed in the future */
void set_obj_cost(CHAR_DATA *ch, PLAN_DATA *obj, bool hedit)
{
  if(IS_SET(obj->type, PLAN_PLACED))
  {
    bug("set_obj_cost called on created object.", 0);
    return;
  }
  switch(obj->type & PLAN_MASK_TYPE)
  {
    case PLAN_ROOM: obj->cost = GET_PRICE(PRICE_ROOM, hedit);
        if(IS_SET(obj->flags, PLAN_ROOM_REGEN))
        {
          obj->cost += calc_cost_range(PRICE_R_REGEN, obj->opt[0], hedit);
          obj->cost += calc_cost_range(PRICE_R_REGEN, obj->opt[1], hedit);
        }
        if(IS_SET(obj->flags, PLAN_ROOM_LAB))
          obj->cost += calc_cost_range(PRICE_LAB, obj->opt[0], hedit);
        if(IS_SET(obj->flags, PLAN_ROOM_ALTAR))
          obj->cost += calc_cost_range(PRICE_ALTAR, obj->opt[0], hedit);
        break;/* end type room */
    case PLAN_ITEM: obj->cost = GET_PRICE(PRICE_ITEM, hedit);
        if((obj->flags & PLAN_ITEM_FURNITURE) != 0)
        {
          obj->cost += GET_PRICE(PRICE_FURNITURE, hedit);
          obj->cost += calc_cost_range(PRICE_F_REGEN, obj->opt[0], hedit);
        }
        if(IS_SET(obj->flags, PLAN_ITEM_FOUNTAIN))
          obj->cost += GET_PRICE(PRICE_FOUNTAIN, hedit);
        if(IS_SET(obj->flags, PLAN_ITEM_PIT))
          obj->cost += GET_PRICE(PRICE_PIT, hedit);
        if(IS_SET(obj->flags, PLAN_ITEM_PORTAL))
          obj->cost += GET_PRICE(PRICE_PORTAL, hedit);
        if(IS_SET(obj->flags, PLAN_ITEM_DOODAD))
          obj->cost += GET_PRICE(PRICE_DOODAD, hedit);
        if(IS_SET(obj->flags, PLAN_ITEM_DRINK))
          obj->cost += GET_PRICE(PRICE_DRINK, hedit);
        break;/* end type item */
    case PLAN_MOB: obj->cost = GET_PRICE(PRICE_MOB, hedit);
             if(IS_SET(obj->flags, PLAN_MOB_HEALER))
             {
               obj->cost += GET_PRICE(PRICE_HEALER, hedit);
               obj->cost += calc_cost_range(PRICE_H_LEVEL, obj->opt[0], hedit);
             }
             if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
             {
               obj->cost += GET_PRICE(PRICE_MERCHANT, hedit);
               obj->cost += calc_cost_range(PRICE_M_DISCOUNT, obj->opt[0], hedit);
             }
             break;/* end type mob */
    case PLAN_EXIT: obj->cost = GET_PRICE(PRICE_EXIT, hedit);
        if(IS_SET(obj->flags, PLAN_EXIT_CLOSABLE))
        {/* No other price set if it can't be closed */
          obj->cost += GET_PRICE(PRICE_E_CLOSABLE, hedit);
          if(IS_SET(obj->flags, PLAN_EXIT_LOCKABLE))
          {/* No other costs for this if it can't be locked */
            obj->cost += GET_PRICE(PRICE_E_LOCKABLE, hedit);
            if(IS_SET(obj->flags, PLAN_EXIT_NOPICK))
              obj->cost += GET_PRICE(PRICE_E_NO_PICK, hedit);
          }
          if(IS_SET(obj->flags, PLAN_EXIT_HIDDEN))
            obj->cost += GET_PRICE(PRICE_E_HIDDEN, hedit);
        }
        break;/* end type exit */
  }
}

bool check_can_edit(CHAR_DATA *ch, int action, bool hedit)
{
  PLAN_DATA *room;
  if(IS_NPC(ch))
  {
    send_to_char("NPCs can not edit.\n\r", ch);
    return FALSE;
  }
  if(hedit)
  {
    if(!ch->pcdata->clan_info)
      return FALSE; /* You need a clan */
    if(IS_IMMORTAL(ch) && get_trust(ch) >= 57)
      return TRUE;
    if(ch->pcdata->clan_info->rank == 5)
      return TRUE; /* Leaders can do anything */
    if(IS_SET(ch->pcdata->edit_flags, action) == action)
      return TRUE;
    return FALSE;
  }
  send_to_char("Personal editing is not functional quite yet.\n\r", ch);
  return FALSE;
  if(!ch->in_room || ch->in_room->vnum >= 0)
    return FALSE;
  if(IS_IMMORTAL(ch) && get_trust(ch) >= 57)
    return TRUE;
  room = ch->pcdata->clan_info->pers_plan;
  for(; room != NULL; room = room->next)
  {
    if(IS_SET(room->type, (PLAN_ROOM | PLAN_PLACED)) == (PLAN_ROOM | PLAN_PLACED)
        && room->to_place == ch->in_room)
      break;/* In their personal rooms, they can edit */
  }
  return room != NULL;
}

/* if do_buy is set, actually pays.  returns FALSE if can't afford. */
bool pay_hall_cost(CHAR_DATA *ch, int amount, bool do_buy, bool hedit)
{
  if(IS_NPC(ch))
    return FALSE;
  int *target;
  if(hedit)
  {
    target = &ch->pcdata->clan_info->clan->tribute;
    amount *= 100;/* Amount is in points, convert to tribute */
  }
  else
  {
    target = &ch->pcdata->bank_gold;
    amount *= 1000;/* Amount is in shards, convert to gold */
  }
  if(IS_IMMORTAL(ch) || amount == 0)
    return TRUE;/* Imms buy for free */
  if(amount < 0)
  {
    if(do_buy)
      *target -= amount;
    return TRUE; /* Can always accept money */
  }
  if(*target < amount)
  {
    if(do_buy)
    {/* Send a failure message */
      char buf[256];
      if(hedit)
        sprintf(buf, "Cost is %d %s, you need %d %s more.\n\r",
          COST_STR((amount / 100), hedit),
          COST_STR(((amount - *target + 99) / 100), hedit));
      else
        sprintf(buf, "Cost is %d %s, you need %d %s more.\n\r",
          COST_STR((amount / 1000), hedit),
          COST_STR(((amount - *target + 999) / 1000), hedit));
      send_to_char(buf, ch);
    }
    return FALSE;
  }
  if(do_buy)
    *target -= amount;
  return TRUE;
}

void swap_rooms(CHAR_DATA *ch, PLAN_DATA *old_room, PLAN_DATA *new_room, bool hedit)
{
  int i;
  PLAN_DATA *start;
  /* Need to change over the room pointers */
  void *holder = new_room->to_place;
  new_room->to_place = old_room->to_place;
  old_room->to_place = holder;/* Don't lose the allocation */
  /* Need to move all the exits */
  for(i = 0; i < 6; i++)
  {
    new_room->exits[i].exit = old_room->exits[i].exit;
    new_room->exits[i].link = old_room->exits[i].link;
    if(new_room->exits[i].link)
      new_room->exits[i].link->exits[rev_dir[i]].link = new_room;
  }
  /* Need to move all PLAN_DATA spawns over to the new room */
  start = START_OBJ(ch, hedit);
  for(; start != NULL; start = start->next)
  {
    if(IS_SET(start->type, PLAN_ITEM | PLAN_MOB) && IS_SET(start->type, PLAN_PLACED))
    {
      if(start->loc == old_room->plan_index)
        start->loc = new_room->plan_index;
    }
  }
  /* Need to reset all of the room's strings */
  load_plan_obj(new_room, TRUE);
}

void set_room_exits(PLAN_DATA *obj)
{
  int i;
  ROOM_INDEX_DATA *pRoomIndex;
  ROOM_INDEX_DATA *target;
  if(!IS_SET(obj->type, PLAN_ROOM))
  {
    bug("Non-room type passed to set_room_exits", 0);
    return;
  }
  if(!IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
  {
    bug("Planned room called for set_room_exits", 0);
    return;
  }
  if(obj == NULL || obj->exits == NULL || obj->to_place == NULL)
  {
    bug("Null pass to set_room_exits", 0);
    return;
  }
  pRoomIndex = (ROOM_INDEX_DATA*)obj->to_place;
  for(i = 0; i < 6; i++)
  {
    if(obj->exits[i].link && obj->exits[i].link->to_place)
      target = obj->exits[i].link->to_place;
    else if(obj->exits[i].outside)
      target = obj->exits[i].outside;
    else
      target = NULL;
    if(target != NULL)
    {/* Check if it exists. If not, add it. */
      if(((ROOM_INDEX_DATA*)obj->to_place)->exit[i] == NULL)
      {
#ifdef OLC_VERSION
        pRoomIndex->exit[i] = alloc_perm( sizeof(*pRoomIndex->exit[i]) );
#else
        pRoomIndex->exit[i] = GC_MALLOC( sizeof(*pRoomIndex->exit[i]) );
#endif
/* Exits currently disabled */
/*        if(obj->exits[i].exit && obj->exits[i].exit->to_place)
          *pRoomIndex->exit[i] = *((EXIT_DATA*)(obj->exits[i].exit->to_place));*/
        switch(i)
        {
          case DIR_NORTH: to_chars_in_room("An exit appears leading north.\n\r", pRoomIndex); break;
          case DIR_SOUTH: to_chars_in_room("An exit appears leading south.\n\r", pRoomIndex); break;
          case DIR_EAST: to_chars_in_room("An exit appears leading east.\n\r", pRoomIndex); break;
          case DIR_WEST: to_chars_in_room("An exit appears leading west.\n\r", pRoomIndex); break;
          case DIR_UP: to_chars_in_room("An exit appears leading up.\n\r", pRoomIndex); break;
          case DIR_DOWN: to_chars_in_room("An exit appears leading down.\n\r", pRoomIndex); break;
        }

        pRoomIndex->exit[i]->u1.to_room = target;
        /* Check the other side */
        if(target->exit[rev_dir[i]] == NULL)
        {
          if(target->vnum >= 0)
            modify_room_marker(obj->clan, target, rev_dir[i], TRUE);

#ifdef OLC_VERSION
          target->exit[rev_dir[i]] = alloc_perm( sizeof(*target->exit[rev_dir[i]]) );
#else
          target->exit[rev_dir[i]] = GC_MALLOC( sizeof(*target->exit[rev_dir[i]]) );
#endif
/* Exits currently disabled */
/*          if(obj->exits[i].exit && obj->exits[i].exit->to_place)
            *target->exit[rev_dir[i]] = *((EXIT_DATA*)(obj->exits[i].exit->to_place));*/
          target->exit[rev_dir[i]]->u1.to_room = obj->to_place;
          switch(rev_dir[i])
          {
            case DIR_NORTH: to_chars_in_room("An exit appears leading north.\n\r", target); break;
            case DIR_SOUTH: to_chars_in_room("An exit appears leading south\n\r.", target); break;
            case DIR_EAST: to_chars_in_room("An exit appears leading east.\n\r", target); break;
            case DIR_WEST: to_chars_in_room("An exit appears leading west.\n\r", target); break;
            case DIR_UP: to_chars_in_room("An exit appears leading up.\n\r", target); break;
            case DIR_DOWN: to_chars_in_room("An exit appears leading down.\n\r", target); break;
          }
        }
      }
    }
    else
    {/* Check if it exists. If so, remove it.  Remove from both sides if needed */
      if(pRoomIndex->exit[i])
      {

        if(pRoomIndex->exit[i]->u1.to_room->exit[rev_dir[i]] &&
            pRoomIndex->exit[i]->u1.to_room->exit[rev_dir[i]]->u1.to_room == pRoomIndex)
        {
          if(pRoomIndex->exit[i]->u1.to_room->vnum >= 0)
            modify_room_marker(obj->clan, pRoomIndex->exit[i]->u1.to_room, -1, FALSE);
          ROOM_INDEX_DATA *target = pRoomIndex->exit[i]->u1.to_room;
          GC_FREE(target->exit[rev_dir[i]]);
          target->exit[rev_dir[i]] = NULL;
          switch(rev_dir[i])
          {
            case DIR_NORTH: to_chars_in_room("The exit leading north vanishes.\n\r", target); break;
            case DIR_SOUTH: to_chars_in_room("The exit leading south vanishes.\n\r", target); break;
            case DIR_EAST: to_chars_in_room("The exit leading east vanishes.\n\r", target); break;
            case DIR_WEST: to_chars_in_room("The exit leading west vanishes.\n\r", target); break;
            case DIR_UP: to_chars_in_room("The exit leading up vanishes.\n\r", target); break;
            case DIR_DOWN: to_chars_in_room("The exit leading down vanishes.\n\r", target); break;
          }
        }
        GC_FREE(((ROOM_INDEX_DATA*)obj->to_place)->exit[i]);
        ((ROOM_INDEX_DATA*)obj->to_place)->exit[i] = NULL;
        switch(i)
        {
          case DIR_NORTH: to_chars_in_room("The exit leading north vanishes.\n\r", pRoomIndex); break;
          case DIR_SOUTH: to_chars_in_room("The exit leading south vanishes.\n\r", pRoomIndex); break;
          case DIR_EAST: to_chars_in_room("The exit leading east vanishes.\n\r", pRoomIndex); break;
          case DIR_WEST: to_chars_in_room("The exit leading west vanishes.\n\r", pRoomIndex); break;
          case DIR_UP: to_chars_in_room("The exit leading up vanishes.\n\r", pRoomIndex); break;
          case DIR_DOWN: to_chars_in_room("The exit leading down vanishes.\n\r", pRoomIndex); break;
        }
      }
    }
  }
}

void load_room_obj(PLAN_DATA *obj, bool strings)
{
  int i;
  ROOM_INDEX_DATA *pRoomIndex;
  if(obj->to_place == NULL)
  {
#ifdef OLC_VERSION
    pRoomIndex      = alloc_perm( sizeof(*pRoomIndex) );
#else
    pRoomIndex      = GC_MALLOC( sizeof(*pRoomIndex) );
#endif
    obj->to_place = pRoomIndex;
  }
  else
    pRoomIndex = (ROOM_INDEX_DATA*)obj->to_place;
  if(pRoomIndex->owner == NULL)
    pRoomIndex->owner   = str_dup("");
//  pRoomIndex->people    = NULL;
//  pRoomIndex->contents    = NULL;
  pRoomIndex->extra_descr   = NULL;
  pRoomIndex->area    = &clan_area;
  pRoomIndex->vnum    = obj->plan_index * -1;
  if(strings)
  {
    char extra[256];
    if(IS_SET(obj->type, PLAN_PREVIEWED))
      sprintf(extra, "{D(Preview){x %s", obj->name ? obj->name : "default");
    else
      sprintf(extra, "%s", obj->name ? obj->name : "default");
    clear_string(&pRoomIndex->name, extra);
    clear_string(&pRoomIndex->label, obj->label);
    clear_string(&pRoomIndex->description, obj->desc ? obj->desc : "Default room desc.");
  }
  pRoomIndex->room_flags    = 0;
  if(IS_SET(obj->type, PLAN_PLACED))
  {
    pRoomIndex->heal_rate = 100;
    pRoomIndex->mana_rate = 100;
    pRoomIndex->obs_target = 0;
    if(IS_SET(obj->flags, PLAN_ROOM_LAB))
    {
      switch(obj->opt[0])
      {
        case 1: pRoomIndex->sector_type = SECT_MAGELAB_SIMPLE; break;
        case 2: pRoomIndex->sector_type = SECT_MAGELAB_INTERMEDIATE; break;
        case 3: pRoomIndex->sector_type = SECT_MAGELAB_ADVANCED; break;
        case 4: pRoomIndex->sector_type = SECT_MAGELAB_SUPERIOR; break;
        case 5: pRoomIndex->sector_type = SECT_MAGELAB_INCREDIBLE; break;
      }
    }
    else if(IS_SET(obj->flags, PLAN_ROOM_ALTAR))
    {
      pRoomIndex->heal_rate = 0;
      pRoomIndex->mana_rate = 0;
      switch(obj->opt[0])
      {
        case 1: pRoomIndex->sector_type = SECT_ALTAR_BASIC; break;
        case 2: pRoomIndex->sector_type = SECT_ALTAR_BLESSED; break;
        case 3: pRoomIndex->sector_type = SECT_ALTAR_ANNOINTED; break;
        case 4: pRoomIndex->sector_type = SECT_ALTAR_HOLY_GROUND; break;
        case 5: pRoomIndex->sector_type = SECT_ALTAR_INCREDIBLE; break;
      }
    }
    else
    {
      if(!IS_SET(obj->flags, PLAN_ROOM_OUTDOORS))
        SET_BIT(pRoomIndex->room_flags, ROOM_INDOORS);
      pRoomIndex->sector_type = SECT_INSIDE;
      if(IS_SET(obj->flags, PLAN_ROOM_REGEN))
      {
        pRoomIndex->heal_rate = GET_REGEN(obj->opt[0], 0);
        if(pRoomIndex->heal_rate < 0)
        {
          bug("Loaded heal rate too low.", 0);
          pRoomIndex->heal_rate = 0;
        }
        else if(pRoomIndex->heal_rate > 300)
        {
          bug("Loaded heal rate too high.", 0);
          pRoomIndex->heal_rate = 300;
        }
        pRoomIndex->mana_rate = GET_REGEN(obj->opt[1], 0);
        if(pRoomIndex->mana_rate < 0)
        {
          bug("Loaded mana rate too low.", 0);
          pRoomIndex->mana_rate = 0;
        }
        else if(pRoomIndex->mana_rate > 300)
        {
          bug("Loaded mana rate too high.", 0);
          pRoomIndex->mana_rate = 300;
        }
      }
    }
  }
  else
  {
    pRoomIndex->mana_rate = 0;
    pRoomIndex->heal_rate = 0;
  }
   if(IS_SET(obj->flags, PLAN_ROOM_DARK))
    SET_BIT(pRoomIndex->room_flags, ROOM_DARK);
  pRoomIndex->light = 0;
  //for (i = 0; i <= 5; i++ )
  //  pRoomIndex->exit[i] = NULL;
  /* end room */
}

void load_mob_obj(PLAN_DATA *obj, bool strings)
{
  int i;
  /* Create its pointer first */
  MOB_INDEX_DATA *pMobIndex;
  SHOP_DATA *pShop;
  if(obj->to_place == NULL)
  {
#ifdef OLC_VERSION
    pMobIndex                       = alloc_perm( sizeof(*pMobIndex) );
#else /*game version*/
    pMobIndex                       = GC_MALLOC( sizeof(*pMobIndex) );
#endif
    obj->to_place = pMobIndex;
  }
  else
    pMobIndex = (MOB_INDEX_DATA*)obj->to_place;
  pMobIndex->vnum                 = obj->plan_index * -1;
  pMobIndex->area                 = 0;
  pMobIndex->new_format   = TRUE;
  if(strings)
  {
    char extra[256];
    if(IS_SET(obj->type, PLAN_PREVIEWED))
      sprintf(extra, "{D(Preview){x %s\n\r", obj->long_d ? obj->long_d : "default is here");
    else
      sprintf(extra, "%s\n\r", obj->long_d ? obj->long_d : "default is here");
    clear_string(&pMobIndex->player_name, obj->name ? obj->name : "default");
    clear_string(&pMobIndex->label, obj->label);
    clear_string(&pMobIndex->short_descr, obj->short_d ? obj->short_d : "default");
    clear_string(&pMobIndex->long_descr, extra);
    clear_string(&pMobIndex->description, obj->desc ? obj->desc : "Default mob desc");
    pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
    pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);
  }
  pMobIndex->race     =             obj->opt[1];
  if(pMobIndex->race <= 0)
    pMobIndex->race = 2; /* Human */

  pMobIndex->act = race_table[pMobIndex->race].act | ACT_IS_NPC | ACT_SENTINEL | ACT_NOTRANS | ACT_NOPURGE;
  if(IS_SET(obj->type, PLAN_PLACED))
  {
    if(IS_SET(obj->flags, PLAN_MOB_HEALER))
      SET_BIT(pMobIndex->act, ACT_IS_HEALER);
    if(IS_SET(obj->flags, PLAN_MOB_MERCHANT) && pMobIndex->pShop == NULL)
    {
  #ifdef OLC_VERSION
      pShop     = alloc_perm( sizeof(*pShop) );
  #else
      pShop     = GC_MALLOC( sizeof(*pShop) );
  #endif
      pShop->keeper   = obj->plan_index * -1;
      for ( i = 0; i < MAX_TRADE; i++ )
        pShop->buy_type[i] = 0;
      pShop->profit_buy = 50;
      pShop->profit_sell  = 100;
      pShop->open_hour  = 0;
      pShop->close_hour = 24;
      pMobIndex->pShop  = pShop;
      /* Does not tie in normally to the shop list */
      pShop->next = NULL;
    }
    else
    {
      if(pMobIndex->pShop)
        GC_FREE(pMobIndex->pShop);
      pMobIndex->pShop                = NULL;
    }
  }

  pMobIndex->affected_by = race_table[pMobIndex->race].aff;

  if(IS_SET(obj->flags, PLAN_MOB_GOOD))
    pMobIndex->alignment = 1000;
  else if(IS_SET(obj->flags, PLAN_MOB_EVIL))
    pMobIndex->alignment = -1000;
  else
    pMobIndex->alignment = 0;
  pMobIndex->group                = 0;

  if(IS_SET(obj->flags, PLAN_MOB_HEALER))
    pMobIndex->level                = 40 + obj->opt[0];
  else
    pMobIndex->level = 1;
  pMobIndex->hitroll              = 1;

  /* read hit dice */
  pMobIndex->hit[DICE_NUMBER]     = 1;
  pMobIndex->hit[DICE_TYPE]     = 1;
  pMobIndex->hit[DICE_BONUS]      = 1;

  /* read mana dice */
  pMobIndex->mana[DICE_NUMBER]  = 1;
  pMobIndex->mana[DICE_TYPE]  = 1;
  pMobIndex->mana[DICE_BONUS] = 1;

  /* read damage dice */
  pMobIndex->damage[DICE_NUMBER]  = 1;
  pMobIndex->damage[DICE_TYPE]  = 1;
  pMobIndex->damage[DICE_BONUS] = 1;
  pMobIndex->dam_type   = attack_lookup("slash");

  /* read armor class */
  pMobIndex->ac[AC_PIERCE]  = 0;
  pMobIndex->ac[AC_BASH]    = 0;
  pMobIndex->ac[AC_SLASH]   = 0;
  pMobIndex->ac[AC_EXOTIC]  = 0;

  /* read flags and add in data from the race table */
  pMobIndex->off_flags    = race_table[pMobIndex->race].off;
  pMobIndex->imm_flags    = race_table[pMobIndex->race].imm;
  pMobIndex->res_flags    = race_table[pMobIndex->race].res;
  pMobIndex->vuln_flags   = race_table[pMobIndex->race].vuln;

  /* vital statistics */
  pMobIndex->start_pos    = position_lookup("stand");
  pMobIndex->default_pos    = position_lookup("stand");
  if(IS_SET(obj->flags, PLAN_MOB_FEMALE))
    pMobIndex->sex = SEX_FEMALE;
  else if(IS_SET(obj->flags, PLAN_MOB_NEUTER))
    pMobIndex->sex = SEX_NEUTRAL;
  else
    pMobIndex->sex = SEX_MALE;

  pMobIndex->wealth   = 0;

  pMobIndex->form     = race_table[pMobIndex->race].form;
  pMobIndex->parts    = race_table[pMobIndex->race].parts;
  /* size */
  pMobIndex->size     = size_lookup("medium");
  pMobIndex->material   = str_dup("flesh");
}

void modify_room_marker(CLAN_DATA *clan, ROOM_INDEX_DATA *room, int dir, bool place)
{/* Remove any previous marker in this room, place a new one if place is TRUE */
  PLAN_DATA *obj;
  int sign_vnum;
  if(clan == NULL || room == NULL)
    return; /* Personal editing */
  sign_vnum = clan->vnum_max * -1;
  if(room->contents != NULL)
  {/* There may be a sign, or need to be one */
    OBJ_DATA *next_on_ground;
    OBJ_DATA *on_ground = room->contents;
    /* Find and remove a sign if it exists, assumption is something has changed */
    while(on_ground != NULL)
    {
      next_on_ground = on_ground->next_content;
      if(on_ground->pIndexData->vnum == sign_vnum)
        extract_obj(on_ground);
      on_ground = next_on_ground;
    }
  }
  if(place && dir >= 0 && dir < 6)
  {
    OBJ_INDEX_DATA *sign_base;
    OBJ_DATA *object;
    char buf[256];
    char *dirstr = NULL;
#ifdef OLC_VERSION
    sign_base                       = alloc_perm( sizeof(*sign_base) );
#else
    sign_base                       = GC_MALLOC( sizeof(*sign_base) );
#endif
    sign_base->vnum = sign_vnum;
    sign_base->item_type = ITEM_TRASH;
    sign_base->area = 0;
    sign_base->new_format = TRUE;
    sign_base->reset_num = 0;
    sign_base->name = str_dup("sign");
    sign_base->label = str_dup("sign");
    switch(dir)
    {
      case DIR_NORTH: dirstr = "north"; break;
      case DIR_SOUTH: dirstr = "south"; break;
      case DIR_EAST: dirstr = "east"; break;
      case DIR_WEST: dirstr = "west"; break;
      case DIR_UP: dirstr = "up"; break;
      case DIR_DOWN: dirstr = "down"; break;
    }
    sprintf(buf, "A clan gate stone glows softly by a door leading %s.", dirstr);
    sign_base->description = str_dup(buf);
    object = create_object(sign_base, 1, FALSE );
    obj_to_room(object, room);
  }
}

/* Portals */
void update_room_sign(CLAN_DATA *clan, ROOM_INDEX_DATA *room)
{
  return;
  PLAN_DATA *obj;
  int sign_vnum;
  if(clan == NULL || room == NULL)
    return;
  sign_vnum = clan->vnum_max * -1;
  if(room->contents != NULL)
  {/* There may be a sign, or need to be one */
    OBJ_DATA *next_on_ground;
    OBJ_DATA *on_ground = room->contents;
    /* Find and remove a sign if it exists, assumption is something has changed */
    while(on_ground != NULL)
    {
      next_on_ground = on_ground->next_content;
      if(on_ground->pIndexData->vnum == sign_vnum)
        extract_obj(on_ground);
      on_ground = next_on_ground;
    }
  }

  for(obj = clan->planned; obj != NULL; obj = obj->next)
  {
    if(IS_SET(obj->type, (PLAN_PREVIEWED | PLAN_PLACED)) &&
      IS_SET(obj->type, PLAN_ITEM) && IS_SET(obj->flags, PLAN_ITEM_PORTAL) && 
      obj->loc == room->vnum * -1 && !IS_SET(obj->flags, PLAN_ITEM_HIDDEN))
        break;
  }

  if(obj)
  {/* Generate a new sign */
    OBJ_INDEX_DATA *sign_base;
    OBJ_DATA *object;
#ifdef OLC_VERSION
    sign_base                       = alloc_perm( sizeof(*sign_base) );
#else
    sign_base                       = GC_MALLOC( sizeof(*sign_base) );
#endif
    sign_base->vnum = sign_vnum;
    sign_base->item_type = ITEM_TRASH;
    sign_base->area = 0;
    sign_base->new_format = TRUE;
    sign_base->reset_num = 0;
    sign_base->name = str_dup("sign");
    sign_base->label = str_dup("sign");
    sign_base->description = str_dup("A sign with the portal destinations available is here");
    object = create_object(sign_base, 1, FALSE );
    obj_to_room(object, room);
  }
}

void load_item_obj(PLAN_DATA *obj, bool strings)
{
  OBJ_INDEX_DATA *pObjIndex;
  if(obj->to_place == NULL)
  {
#ifdef OLC_VERSION
    pObjIndex                       = alloc_perm( sizeof(*pObjIndex) );
#else
    pObjIndex                       = GC_MALLOC( sizeof(*pObjIndex) );
#endif
    obj->to_place = pObjIndex;
  }
  else
    pObjIndex = (OBJ_INDEX_DATA*)obj->to_place;

  pObjIndex->vnum                 = obj->plan_index * -1;
  pObjIndex->area                 = 0;
  pObjIndex->new_format           = TRUE;
  pObjIndex->reset_num    = 0;
  if(strings)
  {
    char extra[256];
    if(IS_SET(obj->type, PLAN_PREVIEWED))
      sprintf(extra, "{D(Preview){x %s", obj->long_d ? obj->long_d : "default item is here");
    else
      sprintf(extra, "%s", obj->long_d ? obj->long_d : "default item is here");
    clear_string(&pObjIndex->name, obj->name ? obj->name : "default");
    clear_string(&pObjIndex->label, obj->label);
    clear_string(&pObjIndex->short_descr, obj->short_d ? obj->short_d : "Default item description");
    clear_string(&pObjIndex->description, extra);
  }
  clear_string(&pObjIndex->material, "something");

  pObjIndex->extra_flags = 0;
  pObjIndex->wear_flags           = 0;

  if(IS_SET(obj->type, PLAN_PLACED))
  {
    switch(obj->flags & PLAN_MASK_OBJ)
    {
      case PLAN_ITEM_PORTAL: pObjIndex->item_type = ITEM_PORTAL; break;
      case PLAN_ITEM_DRINK: pObjIndex->item_type = ITEM_DRINK_CON;
              pObjIndex->extra_flags = ITEM_INVENTORY;
              break;
      case PLAN_ITEM_FURNITURE: pObjIndex->item_type = ITEM_FURNITURE; break;
      case PLAN_ITEM_DOODAD: pObjIndex->item_type = ITEM_TRASH; break;
      case PLAN_ITEM_FOUNTAIN: pObjIndex->item_type = ITEM_FOUNTAIN; break;
      case PLAN_ITEM_PIT: pObjIndex->item_type = ITEM_CONTAINER; break;
      default: pObjIndex->item_type = ITEM_TRASH; break;
    }
    switch(pObjIndex->item_type)
    {
      case ITEM_PORTAL:
        pObjIndex->value[0]   = 0;//Number
        pObjIndex->value[1]   = 0;//Flag
        pObjIndex->value[2]   = 0;//Number
        pObjIndex->value[3]   = obj->opt[0];//Number
        pObjIndex->value[4]   = 0;//Number
        break;
      case ITEM_CONTAINER:
        pObjIndex->value[0]   = 10000;//Number
        pObjIndex->value[1]   = 0;//Flag
        pObjIndex->value[2]   = 0;//Number
        pObjIndex->value[3]   = 1000;//Number
        pObjIndex->value[4]   = 100;//Number
        break;
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
        pObjIndex->value[0]         = 0;//Number
        pObjIndex->value[1]         = 0;//Number
        pObjIndex->value[2]         = obj->opt[0];
        if(pObjIndex->value[2] < 0)
          pObjIndex->value[2] = 0;
        else if(pObjIndex->value[2] >= MAX_LIQUIDS)
          pObjIndex->value[2] = 0;
        pObjIndex->value[3]         = 0;//Number
        pObjIndex->value[4]         = 0;//Number
        break;
      case ITEM_FURNITURE:
        pObjIndex->value[0]             = 5;
        pObjIndex->value[1]             = 0;//Flag
        pObjIndex->value[2]             = SLEEP_ON | SIT_ON | REST_ON;//Flag
        pObjIndex->value[3]             = 100 + URANGE(0, obj->opt[0] * 5, 20);//Flag
        pObjIndex->value[4]       = 100 + URANGE(0, obj->opt[0] * 5, 20);//Flag
        break;
      default:
        pObjIndex->value[0]             = 0;//Flag
        pObjIndex->value[1]             = 0;//Flag
        pObjIndex->value[2]             = 0;//Flag
        pObjIndex->value[3]             = 0;//Flag
        pObjIndex->value[4]       = 0;//Flag
        break;
    }
  }
  else// No using previewed items
    pObjIndex->item_type = ITEM_TRASH;
  pObjIndex->level = 1;
  pObjIndex->weight = 1;
  pObjIndex->cost = 200;
}

void load_exit_obj(PLAN_DATA *obj, bool strings)
{
  EXIT_DATA *pexit;
  if(obj->to_place == NULL)
  {
#ifdef OLC_VERSION
    pexit     = alloc_perm( sizeof(*pexit) );
#else
    pexit     = GC_MALLOC( sizeof(*pexit) );
#endif
    obj->to_place = pexit;
  }
  else
    pexit = (EXIT_DATA*)obj->to_place;
  if(strings)
  {
    clear_string(&pexit->keyword, obj->name);
  }
  if(IS_SET(obj->flags, PLAN_EXIT_CLOSABLE))
  {
    pexit->exit_info = EX_ISDOOR;
    //if(IS_SET(obj->flags, PLAN_EXIT_CLOSED))
    if(IS_SET(obj->flags, PLAN_EXIT_LOCKABLE))
    {
      pexit->key    = obj->opt[0];
      //if(IS_SET(obj->flags, PLAN_EXIT_LOCKED))
      if(IS_SET(obj->flags, PLAN_EXIT_NOPICK))
        pexit->exit_info |= EX_PICKPROOF;
    }
    if(IS_SET(obj->flags, PLAN_EXIT_NOPASS))
      pexit->exit_info |= EX_NOPASS;
    if(IS_SET(obj->flags, PLAN_ITEM_HIDDEN))
      pexit->exit_info |= EX_SECRET;
  }
  else
    pexit->exit_info  = 0;
}

void load_plan_obj(PLAN_DATA *obj, bool strings)
{
  switch(obj->type & PLAN_MASK_TYPE)
  {
    case PLAN_ROOM: load_room_obj(obj, strings);
      break;
    case PLAN_MOB: load_mob_obj(obj, strings);
      break;
    case PLAN_ITEM: load_item_obj(obj, strings);
      break;
    case PLAN_EXIT: load_exit_obj(obj, strings);
      break;
  }
}

/* Doesn't need personal room checking capability */
int calc_hall_type(CLAN_DATA *clan, CHAR_DATA *ch)
{
  if(clan->hall_tribute < 30000)
    return 0;
  else if(clan->hall_tribute < 90000)
    return HALL_TYPE_BASIC;
  return HALL_TYPE_ADVANCED;
/*  PLAN_DATA *obj;
  int room_count = 0;
  int max_hit = 0;
  int max_mana = 0;
  int max_lab = 0;
  int max_altar = 0;
  bool pit = FALSE;
  int healer = 0;
  int portals = 0;
  for(obj = clan->planned; obj != NULL; obj = obj->next)
  {
    if(IS_SET(obj->type, PLAN_PLACED))
    {
      switch(obj->type & PLAN_MASK_TYPE)
      {
        case PLAN_ROOM: room_count++;
          if(IS_SET(obj->flags, PLAN_ROOM_LAB))
          {
            if(obj->opt[0] > max_lab)
              max_lab = obj->opt[0];
          }
          else if(IS_SET(obj->flags, PLAN_ROOM_ALTAR))
          {
            if(obj->opt[0] > max_altar)
              max_altar = obj->opt[0];
          }
          else
          {
            if(obj->opt[0] > max_hit)
              max_hit = obj->opt[0];
            if(obj->opt[1] > max_mana)
              max_mana = obj->opt[1];
          }
          break;
        case PLAN_ITEM:
          if(IS_SET(obj->flags, PLAN_ITEM_PIT))
            pit = TRUE;
          else if(IS_SET(obj->flags, PLAN_ITEM_PORTAL))
            portals++;
          break;
        case PLAN_MOB:
          if(IS_SET(obj->flags, PLAN_MOB_HEALER))
          {
            if(obj->opt[0] + 1 > healer)
              healer = obj->opt[0] + 1;
          }
          break;
      }
    }
  }
  if(room_count > 10)
  {
    if(max_hit >= 14 && max_mana >= 14 && max_lab >= 4 && max_altar >= 4)
    {
      if(healer >= 5 && pit && portals >= 11)
      {
        if(!IS_SET(clan->hall_type, HALL_TYPE_ADVANCED))
        {
          if(ch != NULL)
            send_to_char("Your hall type is now advanced.\n\r", ch);
          REMOVE_BIT(clan->hall_type, HALL_TYPE_BASIC);
          SET_BIT(clan->hall_type, HALL_TYPE_ADVANCED);
        }
        return HALL_TYPE_ADVANCED;
      }
    }
  }
  if(room_count > 5)
  {
    if(max_hit >= 4 && max_mana >= 4 && max_lab >= 3 && max_altar >= 3)
    {
      if(healer && pit && portals >= 4)
      {
        if(!IS_SET(clan->hall_type, HALL_TYPE_BASIC))
        {
          if(ch != NULL)
            send_to_char("Your hall type is now basic.\n\r", ch);
          REMOVE_BIT(clan->hall_type, HALL_TYPE_ADVANCED);
          SET_BIT(clan->hall_type, HALL_TYPE_BASIC);
        }
        return HALL_TYPE_BASIC;
      }
    }
  }
  if(IS_SET(clan->hall_type, (HALL_TYPE_BASIC | HALL_TYPE_ADVANCED)))
  {
    if(ch != NULL)
      send_to_char("Your hall type is now beginner.\n\r", ch);
    REMOVE_BIT(clan->hall_type, (HALL_TYPE_BASIC | HALL_TYPE_ADVANCED));
  }
  return HALL_TYPE_NONE;*/
}

void respawn_plan_obj(PLAN_DATA *obj, PLAN_DATA *start)
{
  PLAN_DATA *dest;
  if(IS_SET(obj->type, (PLAN_ROOM | PLAN_EXIT)))
    return;
  if(!IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)) || obj->to_place == NULL)
    return;
 // if(IS_SET(obj->type, PLAN_ITEM) && !IS_SET(obj->flags, PLAN_ITEM_DRINK))
  {
    dest = find_edit_obj_by_index(start, PLAN_ROOM, obj->loc);
    if(dest == NULL || dest->to_place == NULL)
      return;
  }
  /*else
  {
    dest = find_edit_obj_by_index(start, PLAN_MOB, obj->loc);
    if(dest == NULL || dest->to_place == NULL)
      return;
  }*/
  if(IS_SET(obj->type, PLAN_MOB))
  {
    /* Despawn all copies */
    CHAR_DATA *victim, *vic_next;
    for ( victim = char_list; victim != NULL; victim = vic_next )
    {
      vic_next  = victim->next;
      if(IS_NPC(victim) && victim->pIndexData->vnum == obj->plan_index * -1)
        extract_char(victim, TRUE);
    }
    victim = create_mobile((MOB_INDEX_DATA*)obj->to_place);
    if(victim == NULL)
    {
      bug("Couldn't find mob to create", 0);
      return;
    }
    char_to_room(victim, (ROOM_INDEX_DATA*)dest->to_place);
    if(IS_SET(obj->type, PLAN_PREVIEWED))
      act( "$n forms from thin air.", victim, NULL, NULL, TO_ROOM ,FALSE);
    else
      act( "$n looks much more solid.", victim, NULL, NULL, TO_ROOM ,FALSE);
  }
  else
  {
    char buf[256];
    OBJ_DATA *object, *obj_next;
    for ( object = object_list; object != NULL; object = obj_next )
    {
      obj_next  = object->next;
      if(object->pIndexData->vnum == obj->plan_index * -1)
        extract_obj(object);
    }
    object = create_object((OBJ_INDEX_DATA*)obj->to_place, 1, FALSE );
//    if (IS_SET(obj->flags, PLAN_OBJ_DRINK))
//      obj_to_char(object, ch );/* NOT COMPLETE */
//    else
      obj_to_room(object, (ROOM_INDEX_DATA*)dest->to_place);
      if(IS_SET(obj->type, PLAN_PREVIEWED))
        sprintf(buf, "%s forms from thin air.\n\r", object->short_descr);
      else
        sprintf(buf, "%s looks much more solid.\n\r", object->short_descr);
      to_chars_in_room(buf, (ROOM_INDEX_DATA*)dest->to_place);
  }
}

void preview_hall_obj(CHAR_DATA *ch, PLAN_DATA *obj, bool hedit)
{
  REMOVE_BIT(obj->type, PLAN_PLANNED);
  SET_BIT(obj->type, PLAN_PREVIEWED);
  obj->reviewed = FALSE;
  /* Remember to call respawn on everything, this only loads it into an actual object */
  load_plan_obj(obj, TRUE);
  /* Spawns it if an instance doesn't already exist */
  respawn_plan_obj(obj, START_OBJ(ch, hedit));
}

void place_hall_obj(CHAR_DATA *ch, PLAN_DATA *obj, bool hedit)
{
  REMOVE_BIT(obj->type, PLAN_PREVIEWED);
  SET_BIT(obj->type, PLAN_PLACED);
  clear_string(&obj->previewer, NULL);
  obj->reviewed = FALSE;
  load_plan_obj(obj, TRUE);
  /* Spawns it if an instance doesn't already exist */
  respawn_plan_obj(obj, START_OBJ(ch, hedit));
}

/* Per item refund amount */
int get_refund_amount(CHAR_DATA *ch, int amount, bool hedit)
{/* Can only undo previewed items currently, 100% refund */
  return amount;
/*  if(hedit)
    return (amount * 7) / 10;
  else
    return (amount * 8) / 10;*/
}

int remove_hall_obj(CHAR_DATA *ch, PLAN_DATA *obj, bool hedit)
{/* Remove an object that is placed. */
  /* Remember: Imms get no refund. */
  /* Remember: Should be a message for the objects vanishing */
  int i, num_exits;
  PLAN_DATA *search = START_OBJ(ch, hedit);
  CHAR_DATA *in_room, *next_in_room;
  OBJ_DATA *on_ground, *next_on_ground;
  if(IS_SET(obj->type, (PLAN_PREVIEWED | PLAN_PLACED)))
  {/* Shouldn't be called on placed but if it is it should remove them anyway */
    int extra = 0;/* Some items have extra refund than just their own */
    if(!obj->to_place)
    {
      REMOVE_BIT(obj->type, PLAN_PREVIEWED);
      REMOVE_BIT(obj->type, PLAN_PLACED);
      SET_BIT(obj->type, PLAN_PLANNED);
      return 0;
    }
    switch(obj->type & PLAN_MASK_TYPE)
    {
      case PLAN_ROOM:
        /* First, remove any placed items based on their plans */
        while(search)
        {
          if(IS_SET(search->type, (PLAN_PLACED | PLAN_PREVIEWED)))
          {
            if((IS_SET(search->type, PLAN_MOB)
              || (IS_SET(search->type, PLAN_ITEM) && !IS_SET(search->flags, PLAN_ITEM_DRINK))) &&
              search->loc == obj->plan_index)
            {
              extra += remove_hall_obj(ch, search, hedit);
            }
          }
          search = search->next;
        }
        /* Now check for any mobs and items that are still in the room */
        if((in_room = ((ROOM_INDEX_DATA*)obj->to_place)->people) != NULL)
        {
          while(in_room != NULL)
          {
            next_in_room = in_room->next_in_room;
            if(in_room->pIndexData->vnum < 0)
              extract_char(in_room, TRUE);
            else
            {
              char_from_room(in_room);
              char_to_room(in_room, ch->in_room);
              if(!IS_NPC(in_room))
                do_look(in_room, "auto");
            }
            in_room = next_in_room;
          }
        }
        if((on_ground = ((ROOM_INDEX_DATA*)obj->to_place)->contents) != NULL)
        {
          while(on_ground != NULL)
          {
            next_on_ground = on_ground->next_content;
            if(on_ground->pIndexData->vnum < 0)
              extract_obj(on_ground);
            else
            {
              obj_from_room(on_ground);
              obj_to_room(on_ground, ch->in_room);
            }
            on_ground = next_on_ground;
          }
        }
        /* Now close up the exits */
        num_exits = 0;
        for(i = 0; i < 6; i++)
        {/* Break the exits */
          if(obj->exits[i].link)
          {
            if(obj->exits[i].link->exits[rev_dir[i]].link == obj)
              obj->exits[i].link->exits[rev_dir[i]].link = NULL;
            obj->exits[i].link = NULL;
            num_exits++;
          }
          if(obj->exits[i].outside)
            obj->exits[i].outside = NULL;
          if(obj->exits[i].exit)
          {
            extra += remove_hall_obj(ch, obj->exits[i].exit, hedit);
            obj->exits[i].exit = NULL;
          }
        }
        if(num_exits > 1)
          extra += get_refund_amount(ch, GET_PRICE(PRICE_EXIT, hedit), hedit) * num_exits - 1;
        set_room_exits(obj);
        clear_string(&((ROOM_INDEX_DATA*)obj->to_place)->name, NULL);
        clear_string(&((ROOM_INDEX_DATA*)obj->to_place)->description, NULL);
        clear_string(&((ROOM_INDEX_DATA*)obj->to_place)->owner, NULL);
        clear_string(&((ROOM_INDEX_DATA*)obj->to_place)->label, NULL);
        break;
      case PLAN_ITEM:/* Search all rooms for instances of this item */
        /* If it's a drink item, check only on merchants */
        /* Pits should dump contents on the ground in the room that ch is in */
        if(!IS_SET(obj->flags, PLAN_ITEM_DRINK))
        {/* Drinks are handled differently - they only delete their plan */
          for(; search != NULL; search = search->next)
          {
            if(IS_SET(search->type, PLAN_ROOM) && search->to_place)
            {
              if((on_ground = ((ROOM_INDEX_DATA*)search->to_place)->contents) != NULL)
              {
                while(on_ground != NULL)
                {
                  next_on_ground = on_ground->next_content;
                  if(on_ground->pIndexData == obj->to_place)
                  {
                    act( "$p dissolves into thin air.", ch, on_ground, NULL, TO_ROOM ,FALSE);
                    act( "$p dissolves into thin air.", ch, on_ground, NULL, TO_CHAR ,FALSE);
                    extract_obj(on_ground);
                  }
                  on_ground = next_on_ground;
                }
              }
            }
          }
        }
        clear_string(&((OBJ_INDEX_DATA*)obj->to_place)->name, NULL);
        clear_string(&((OBJ_INDEX_DATA*)obj->to_place)->short_descr, NULL);
        clear_string(&((OBJ_INDEX_DATA*)obj->to_place)->description, NULL);
        clear_string(&((OBJ_INDEX_DATA*)obj->to_place)->label, NULL);
        clear_string(&((OBJ_INDEX_DATA*)obj->to_place)->material, NULL);
        break;
      case PLAN_MOB:/* Search all rooms for instances of this mob */
        for(; search != NULL; search = search->next)
        {
          if(IS_SET(search->type, PLAN_ROOM) && search->to_place)
          {
            if((in_room = ((ROOM_INDEX_DATA*)search->to_place)->people) != NULL)
            {
              while(in_room != NULL)
              {
                next_in_room = in_room->next;
                if(in_room->pIndexData == obj->to_place)
                {
                  act("$n dissolves into the air.", in_room, NULL, NULL, TO_ROOM, FALSE);
                  extract_char(in_room, TRUE);
                }
                in_room = next_in_room;
              }
            }
          }
        }
        if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
        {/* Remove any drinks */
          for(search = START_OBJ(ch, hedit); search != NULL; search = search->next)
          {
            if(IS_SET(search->type, (PLAN_ITEM | PLAN_PLACED)) == (PLAN_ITEM | PLAN_PLACED) &&
              IS_SET(search->flags, PLAN_ITEM_DRINK) && search->loc == obj->plan_index)
                extra += remove_hall_obj(ch, search, hedit);
          }
        }
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->player_name, NULL);
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->short_descr, NULL);
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->long_descr, NULL);
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->description, NULL);
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->label, NULL);
        clear_string(&((MOB_INDEX_DATA*)obj->to_place)->material, NULL);
        break;
      case PLAN_EXIT:/* Search all rooms for this exit */
        for(; search != NULL; search = search->next)
        {
          if(IS_SET(search->type, PLAN_ROOM) && search->to_place)
          {/* Check its exits */
            for(i = 0; i < 6; i++)
            {
              if(search->exits[i].exit == obj)
              {/* Remove */
                search->exits[i].exit = NULL;
                ((ROOM_INDEX_DATA*)search->to_place)->exit[i]->exit_info = 0;
              }
            }
          }
        }
        break;
    }
    //GC_FREE(obj->to_place);
    //obj->to_place = NULL;
    REMOVE_BIT(obj->type, PLAN_PREVIEWED);
    REMOVE_BIT(obj->type, PLAN_PLACED);
    SET_BIT(obj->type, PLAN_PLANNED);
    if(IS_IMMORTAL(ch))
      return 0;
    return extra + get_refund_amount(ch, obj->cost, hedit);
  }
  bug("Remove on non-placed object.", 0);
  return 0;/* Returns the amount refunded */
}

void read_to_tilde(FILE *fp, char **string)
{
  char input[MAX_STRING_LENGTH];
  int i = 0;
  bool first = FALSE;
  while(!feof(fp))
  {
    input[i] = fgetc(fp);
    if(input[i] == '~')
    {
      input[i] = '\0';
      break;
    }
    if(!first)
    {
      if(input[i] != ' ')
        first = TRUE;
      else
        i--;
    }
    i++;
  }
  if(feof(fp) || input[0] == '\0')
    clear_string(string, NULL);
  else
    clear_string(string, input);
}

void fread_clan_hall(FILE *fp, PLAN_DATA **head, CLAN_DATA *clan)
{
  PLAN_DATA *obj;
  char input[MAX_INPUT_LENGTH];
  while(!feof(fp))
  {
    fscanf(fp, "%s", input);
    if(!strcmp(input, "Label"))
    {
      int type = 0;
      obj = new_plan();
      obj->clan = clan;
      if(!fread_plan_obj(fp, obj))
        return;/* Bad load */
      if((obj->type & PLAN_MASK_TYPE) == 0)
        return;/* Bad object type */
      type = obj->type & PLAN_MASK_TYPE;
      /* Sort the object based on its type */
      if(*head == NULL || type < ((*head)->type & PLAN_MASK_TYPE) ||
          obj->plan_index < (*head)->plan_index)
      {
        obj->next = (*head);
        *head = obj;
      }
      else
      {
        PLAN_DATA *prev = (*head);
        while(prev->next && (type > (prev->type & PLAN_MASK_TYPE) ||
              (type == (prev->type & PLAN_MASK_TYPE) && prev->next->plan_index < prev->plan_index)))
          prev = prev->next;
        obj->next = prev->next;
        prev->next = obj;
      }
    }
    else if(!strcmp(input, "Exit"))
    {/* Done with main loading, read all the exits */
      for(obj = *head; obj != NULL; obj = obj->next)
      {
        if(IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
        {
          load_plan_obj(obj, TRUE);
          if(clan->hall_index && IS_SET(obj->type, PLAN_ROOM) && obj->plan_index == clan->hall_index)
            clan->hall = obj;
        }
      }
      while(!feof(fp))
      {
        if(!strcmp(input, "Exit"))
          fread_plan_exit(fp, *head, clan);
        else if(!strcmp(input, "#End"))
          break;
        else
          fread_to_eol(fp);
        fscanf(fp, "%s", input);
      }
      /* Finalize */
      for(obj = *head; obj != NULL; obj = obj->next)
      {
        if(IS_SET(obj->type, PLAN_ROOM) && IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
          set_room_exits(obj);
      }
      break;
    }
    else
      fread_to_eol(fp);
  }
}

bool fread_plan_obj(FILE *fp, PLAN_DATA *obj)
{
  bool found = FALSE;
  char input[MAX_INPUT_LENGTH];
  if(fp == NULL || obj == NULL)
    return FALSE;
  /* Should be called only after "Label" has been found */
  fscanf(fp, "%s", input);
  obj->label = str_dup(input);
  while(!feof(fp))
  {
    fscanf(fp, "%s", input);
    switch(input[0])
    {
      case 'C':
        if(!strcmp(input, "Cost"))
        {
          fscanf(fp, "%d", &obj->cost);
          found = TRUE;
        }
      case 'D':
        if(!strcmp(input, "Desc"))
        {
          read_to_tilde(fp, &obj->desc);
          found = TRUE;
        }
        break;
      case '#':
        if(!strcmp(input, "#End"))
        {
          return TRUE; /* Done */
        }
        break;
      case 'F':
        if(!strcmp(input, "Flags"))
        {
          fscanf(fp, "%ld", &obj->flags);
          found = TRUE;
        }
        else if(!strcmp(input, "Flagged"))
        {
          obj->flagged = TRUE;
          found = TRUE;
        }
        break;
      case 'I':
        if(!strcmp(input, "Index"))
        {
          fscanf(fp, "%d", &obj->plan_index);
          found = TRUE;
        }
        break;
      case 'L':
        if(!strcmp(input, "Long"))
        {
          read_to_tilde(fp, &obj->long_d);
          found = TRUE;
        }
        else if(!strcmp(input, "Loc"))
        {
          fscanf(fp, "%d", &obj->loc);
          found = TRUE;
        }
        break;
      case 'N':
        if(!strcmp(input, "Name"))
        {
          read_to_tilde(fp, &obj->name);
          found = TRUE;
        }
        break;
      case 'S':
        if(!strcmp(input, "Short"))
        {
          read_to_tilde(fp, &obj->short_d);
          found = TRUE;
        }
        break;
      case 'O':
        if(!strcmp(input, "Opt"))
        {
          fscanf(fp, "%d %d", &obj->opt[0], &obj->opt[1]);
          found = TRUE;
        }
        break;
      case 'R':
        if(!strcmp(input, "Review"))
        {
          obj->reviewed = TRUE;
          found = TRUE;
        }
        break;
      case 'T':
        if(!strcmp(input, "Type"))
        {
          fscanf(fp, "%ld", &obj->type);
          found = TRUE;
          if(IS_SET(obj->type, PLAN_ROOM))
            obj->exits = new_p_exit();
        }
        break;
    }
    if(!found)
      fread_to_eol(fp);
  }
  bug("No end to planned object found.", 0);
  return FALSE; /* Bad ending */
}

/* Read and attach the exits, all linked rooms should already be built */
bool fread_plan_exit(FILE *fp, PLAN_DATA *first, CLAN_DATA *clan)
{
  int dir, index, link_index, exit_index, outside_index;
  PLAN_DATA *room = NULL;
  fscanf(fp, "%d %d %d %d %d", &dir, &index, &link_index, &exit_index, &outside_index);
  room = find_edit_obj_by_index(first, PLAN_ROOM, index);
  if(room == NULL || dir < 0 || dir > 6)
    return FALSE;
  if(exit_index >= 0)
    room->exits[dir].exit = find_edit_obj_by_index(first, PLAN_EXIT, exit_index);
  if(link_index >= 0)
    room->exits[dir].link = find_edit_obj_by_index(first, PLAN_ROOM, link_index);
  if(outside_index >= 0)
  {
    room->exits[dir].outside = get_room_index(outside_index);
    if(clan && !clan->hall)
    {
      clan->hall = room;
      clan->hall_index = room->plan_index;
    }
  }
  return TRUE;
}

void fwrite_plan_obj(FILE *fp, PLAN_DATA *obj)
{
  if(fp == NULL || obj == NULL)
    return; /* Can't do anything */
  if(obj->label != NULL && obj->label[0] != '\0')
    fprintf(fp, "Label %s\n", obj->label);
  else
    return; /* No saving an object with no label */
  fprintf(fp, "Type %ld\n", obj->type);
  if(obj->name != NULL && obj->name[0] != '\0')
    fprintf(fp, "Name %s~\n", obj->name);
  if(obj->short_d != NULL && obj->short_d[0] != '\0')
    fprintf(fp, "Short %s~\n", obj->short_d);
  if(obj->long_d != NULL && obj->long_d[0] != '\0')
    fprintf(fp, "Long %s~\n", obj->long_d);
  if(obj->desc != NULL && obj->desc[0] != '\0')
    fprintf(fp, "Desc %s~\n", obj->desc);
  fprintf(fp, "Index %d\n", obj->plan_index);
  fprintf(fp, "Cost %d\n", obj->cost);
  fprintf(fp, "Flags %ld\n", obj->flags);
  fprintf(fp, "Opt %d %d\n", obj->opt[0], obj->opt[1]);
  fprintf(fp, "Loc %d\n", obj->loc);
  if(obj->reviewed)
    fprintf(fp, "Review\n");
  if(obj->flagged)
    fprintf(fp, "Flagged\n");
  fprintf(fp, "#End\n");
}

/* Written after all the other objects */
void fwrite_room_exits(FILE *fp, PLAN_DATA *obj)
{
  int i;
  if(fp == NULL || obj == NULL || obj->exits == NULL)
    return;
  for(i = 0; i < 6; i++)
  {
    if(obj->exits[i].link || obj->exits[i].outside)
    {
      fprintf(fp, "Exit %d %d %d %d %d\n", i, obj->plan_index,
          obj->exits[i].link ? obj->exits[i].link->plan_index : -1,
          obj->exits[i].exit ? obj->exits[i].exit->plan_index : -1,
          obj->exits[i].outside ? obj->exits[i].outside->vnum : -1);
    }
  }
}

int get_arg_dir(char *arg)
{
  if(arg == NULL || arg[0] == '\0')
    return -1;
  if(!str_prefix(arg, "north"))
    return DIR_NORTH;
  else if(!str_prefix(arg, "south"))
    return DIR_SOUTH;
  else if(!str_prefix(arg, "east"))
    return DIR_EAST;
  else if(!str_prefix(arg, "west"))
    return DIR_WEST;
  else if(!str_prefix(arg, "up"))
    return DIR_UP;
  else if(!str_prefix(arg, "down"))
    return DIR_DOWN;
  return -1;
}

/* Clear and set a string - saves checks in function for NULL str */
void clear_string(char **str, char *new_str)
{
/*if(new_str != NULL)
{
if(*str != NULL)
free_string(*str);
  *str = str_dup(new_str);
}
else
  *str = str_dup("");
return;*/
  if(*str != NULL)
  {
    free_string(*str);
    *str = NULL;
  }
  if(new_str != NULL)
    *str = str_dup(new_str);
}

/* Never enter an already marked room, never enter the room being removed */
void search_linked_rooms(PLAN_DATA *start)
{
  int i;
  if(!start->exits)
    return;
  SET_BIT(start->flags, PLAN_ROOM_MARKED);
  for(i = 0; i < 6; i++)
  {
    if(start->exits[i].link)
    {/* Must have a first set - this room being set MARKED ensures it won't recurse into itself */
      if(!IS_SET(start->exits[i].link->flags, PLAN_ROOM_MARKED))
        search_linked_rooms(start->exits[i].link);
    }
  }
}

/* Fill in gaps in the index */
int new_obj_index(CHAR_DATA *ch, int type, bool hedit)
{
  bool use_high = FALSE;
  int open_val = -1;
  int low = ch->pcdata->clan_info->clan->vnum_min, high = ch->pcdata->clan_info->clan->vnum_min - 1;
  PLAN_DATA *obj = START_OBJ(ch, hedit);
  for(; obj != NULL; obj = obj->next)
  {
    if((obj->type & PLAN_MASK_TYPE) == type)
    {
      if(open_val < 0)
      {/* Only check this until we find a gap */
        if(low < obj->plan_index)
          open_val = low;/* We found a gap */
        low++;
      }
      else if(obj->plan_index == open_val)
      {
        use_high = TRUE;/* Bad */
        bug("Out of order index found.\n\r", 0);
      }
      if(obj->plan_index > high)
        high = obj->plan_index;/* Find the top */
    }
  }
  if(use_high || open_val < 0)
  {
    if(hedit)
    {
      if(high > ch->pcdata->clan_info->clan->vnum_max)
      {
        high = ch->pcdata->clan_info->clan->vnum_min;
        bug("high vnum too high for a clan value", 0);
      }
      else
        high++;
    }
    else
      high++;
    return high;
  }
  if(hedit)
  {
    if(open_val > ch->pcdata->clan_info->clan->vnum_max)
    {
      bug("gap vnum too high for a clan value", 0);
      open_val = ch->pcdata->clan_info->clan->vnum_min;
    }
  }
  return open_val;
}

void display_obj(CHAR_DATA *ch, PLAN_DATA *obj)
{
  char buf[256];
  if(ch == NULL || ch->pcdata == NULL || obj == NULL)
    return; /* Can't do anything */
  /* fp takes precedent */
  sprintf(buf, "Label: %s\n\rName: %s\n\r", obj->label, obj->name);
  send_to_char(buf, ch);
  switch(obj->type & PLAN_MASK_TYPE)
  {
    case PLAN_ROOM: send_to_char("Desc:\n\r", ch);
        if(!ch->pcdata->edit_obj || ch->pcdata->edit_obj != obj)
          send_to_char(obj->desc, ch);
        else
          send_to_char(ch->pcdata->edit_str, ch);
        send_to_char("\n\r", ch);
        if(!IS_SET(obj->flags, (PLAN_ROOM_LAB | PLAN_ROOM_ALTAR)))
          sprintf(buf, "Hit regen: %d%%. Mana regen: %d%%.\n\r",
              GET_REGEN(obj->opt[0], 0), GET_REGEN(obj->opt[1], 0));
        else if(IS_SET(obj->flags, PLAN_ROOM_LAB))
          sprintf(buf, "Mage lab level: %d.\n\r", obj->opt[0]);
        else
          sprintf(buf, "Altar level: %d.\n\r", obj->opt[0]);
        send_to_char(buf, ch);
        if(IS_SET(obj->flags, PLAN_ROOM_OUTDOORS))
          send_to_char("This room is outdoors.\n\r", ch);
        else
          send_to_char("This room is indoors.\n\r", ch);
        if(IS_SET(obj->flags, PLAN_ROOM_DARK))
          send_to_char("This room is dark.\n\r", ch);
        else
          send_to_char("This room has light.\n\r", ch);
        if(obj->exits)
        {/* Only shows on placed objects, but you can stat a placed room */
          int i;
          char exit[10];
          for(i = 0; i < 6; i++)
          {
            if(obj->exits[i].link)
            {
              switch(i)
              {
                case DIR_NORTH: strcpy(exit, "north"); break;
                case DIR_SOUTH: strcpy(exit, "south"); break;
                case DIR_EAST: strcpy(exit, "east"); break;
                case DIR_WEST: strcpy(exit, "west"); break;
                case DIR_UP: strcpy(exit, "up"); break;
                case DIR_DOWN: strcpy(exit, "down"); break;
                default: strcpy(exit, "nowhere"); break;
              }
              sprintf(buf, "Exit %s to %s with exit %s\n", exit,
                  obj->exits[i].link->label,
                  obj->exits[i].exit ? obj->exits[i].exit->label : "default");
              send_to_char(buf, ch);
            }
          }
        }
        break;
    case PLAN_MOB:sprintf(buf, "Short: %s\n\rLong: %s\n\r", obj->short_d, obj->long_d);
            send_to_char(buf, ch);
            send_to_char("Desc:\n\r", ch);
            if(!ch->pcdata->edit_obj || ch->pcdata->edit_obj != obj)
              send_to_char(obj->desc, ch);
            else
              send_to_char(ch->pcdata->edit_str, ch);
            send_to_char("\n\r", ch);
            if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
            {
              sprintf(buf, "Type: Merchant.\n\r, Discount: %d%%.\n\r", obj->opt[0] * 10);
              send_to_char(buf, ch);
            }
            else if(IS_SET(obj->flags, PLAN_MOB_HEALER))
            {
              sprintf(buf, "Type: Healer.\n\r, Level: %d.\n\r", obj->opt[0] + 40);
              send_to_char(buf, ch);
            }
            else
              send_to_char("Type: None.\n\r", ch);
            sprintf(buf, "Race: %s.\n\r", race_table[obj->opt[1]].name);
            send_to_char(buf, ch);
            if(IS_SET(obj->flags, PLAN_MOB_GOOD))
              send_to_char("Alignment: Good.\n\r", ch);
            else if(IS_SET(obj->flags, PLAN_MOB_EVIL))
              send_to_char("Alignment: Evil.\n\r", ch);
            else
              send_to_char("Alignment: Neutral.\n\r", ch);
            if(IS_SET(obj->flags, PLAN_MOB_FEMALE))
              send_to_char("Gender: Female.\n\r", ch);
            else if(IS_SET(obj->flags, PLAN_MOB_NEUTER))
              send_to_char("Gender: Neuter.\n\r", ch);
            else
              send_to_char("Gender: Male.\n\r", ch);
            break;
    case PLAN_ITEM:sprintf(buf, "Short: %s\n\rLong: %s\n\r", obj->short_d, obj->long_d);
             send_to_char(buf, ch);
             if(IS_SET(obj->flags, PLAN_ITEM_PIT))
             {
               send_to_char("Type: Pit.\n\r", ch);
             }
             else if(IS_SET(obj->flags, PLAN_ITEM_DRINK))
             {
               sprintf(buf, "Type: Drink.\n\rLiquid: %s.\n\r", liq_table[obj->opt[0]].liq_name);
               send_to_char(buf, ch);
             }
             else if(IS_SET(obj->flags, PLAN_ITEM_FOUNTAIN))
             {
               sprintf(buf, "Type: Fountain.\n\rLiquid: %s.\n\r", liq_table[obj->opt[0]].liq_name);
               send_to_char(buf, ch);
             }
             else if(IS_SET(obj->flags, PLAN_ITEM_FURNITURE))
             {
               sprintf(buf, "Type: Furniture.\n\rRegen: %d%%.\n\r", obj->opt[0] * 5);
               send_to_char(buf, ch);
             }
             else if(IS_SET(obj->flags, PLAN_ITEM_DOODAD))
             {
               send_to_char("Type: Doodad.\n\r", ch);
             }
             else if(IS_SET(obj->flags, PLAN_ITEM_PORTAL))
             {
               send_to_char("Type: Portal.\n\r", ch);
               send_to_char("Target: Setup in progress.\n\r", ch);
             }
             else
               send_to_char("Type: None.\n\r", ch);
             //if(IS_SET(obj->flags, PLAN_ITEM_HIDDEN))
             //  send_to_char("This object is hidden.\n\r", ch);
             break;
    case PLAN_EXIT:
             if(IS_SET(obj->flags, PLAN_EXIT_CLOSABLE))
             {
               if(IS_SET(obj->flags, PLAN_EXIT_CLOSED))
                 send_to_char("The exit has a door that starts closed.\n\r", ch);
               else
                 send_to_char("The exit has a door that starts open.\n\r", ch);
               if(IS_SET(obj->flags, PLAN_EXIT_LOCKABLE))
               {
                 if(IS_SET(obj->flags, PLAN_EXIT_LOCKED))
                   send_to_char("The exit is lockable and starts locked.\n\r", ch);
                 else
                   send_to_char("The exit is lockable and starts unlocked.\n\r", ch);
                 if(IS_SET(obj->flags, PLAN_EXIT_NOPICK))
                   send_to_char("The lock can not be picked.\n\r", ch);
               }
               if(IS_SET(obj->flags, PLAN_EXIT_NOPASS))
                 send_to_char("The door can't be passed through.\n\r", ch);
               if(IS_SET(obj->flags, PLAN_ITEM_HIDDEN))
                 send_to_char("This object is hidden.\n\r", ch);
             }
             break;
  }
}

bool check_upgrade(CHAR_DATA *ch, PLAN_DATA *obj, char prefix)
{
  char buf[256];
  if(ch->pcdata->clan_info->clan->leaders > 1)
  {
    if(obj->previewer == NULL)
    {
      sprintf(buf, "%c%s", prefix, ch->name);
      clear_string(&obj->previewer, buf);
      send_to_char("Upgrade prepared, awaiting another leader to verify.\n\r", ch);
      return FALSE;
    }
    if(obj->previewer[0] != prefix)
    {
      sprintf(buf, "%c%s", prefix, ch->name);
      clear_string(&obj->previewer, buf);
      send_to_char("Upgrade request type changed, awaiting another leader to verify.\n\r", ch);
      return FALSE;
    }
    if(!str_cmp(obj->previewer + 1, ch->name))
    {
      send_to_char("You can't verify your own upgrade.\n\r", ch);
      return FALSE;
    }
  }
  return TRUE;
}

/* This function is awkward, pedit and hedit share enough to be worth having in
 * one function, but have enough separate checks to clutter it up. */
/* Big scary function. */
void player_edit(CHAR_DATA *ch, char *argument, bool hedit)
{
  int i;
  char buf[300], arg[256];
  PLAN_DATA *obj = NULL;
  if(IS_IMMORTAL(ch))
  {
    sprintf(buf, "Imm hall edit (%s clan): %s", argument, ch->pcdata->clan_info ? ch->pcdata->clan_info->clan->name : "None");
    log_string(buf);
  }
  /* Using nested ifs because switches get too cluttered for nesting */
  argument = one_argument(argument, arg);
  if(arg[0] == '\0')
  {
    if(ch->pcdata->edit_flags)
    {/* Prevent accidental quits */
      if(hedit)
      {
        send_to_char("Available commands: ({Y*{x means they have a cost or refund)\n\r", ch);
        send_to_char("list display create clone edit delete {Y*place *upgrade *replace *(un)link{x stop\n\r", ch);
        if(ch->pcdata->edit_obj)
        {
          switch(ch->pcdata->edit_obj->type & PLAN_MASK_TYPE)
          {
            case PLAN_ROOM: send_to_char("Room Edit: label name desc hregen mregen magelab altar outdoor dark\n\r", ch);
              break;
            case PLAN_ITEM: send_to_char("Item Edit: label name short desc type target regen liquid hidden\n\r", ch);
              break;
            case PLAN_MOB: send_to_char("Mob Edit: label name short long desc race align gender discount level\n\r", ch);
              break;
            case PLAN_EXIT: send_to_char("Exits are not functional currently, sorry.\n\r", ch);
              break;
          }
        }
      }
      else
        send_to_char("Use pedit stop if you are done.\n\r", ch);
      return;
    }
    if(!check_can_edit(ch, CLAN_CAN_EDIT, hedit))
    {
      send_to_char("You do not have permissions to edit.\n\r", ch);
      return;
    }
    if(hedit)
    {
      SET_BIT(ch->pcdata->edit_flags, EDITMODE_HALL);
      send_to_char("Hall edit mode activated.\n\r", ch);
    }
    else
    {
      SET_BIT(ch->pcdata->edit_flags, EDITMODE_PERSONAL);
      send_to_char("Personal edit mode activated.\n\r", ch);
    }
    return;
  }
  else if(!IS_SET(ch->pcdata->edit_flags, (EDITMODE_PERSONAL | EDITMODE_HALL)))
  {
    send_to_char("You have not begun editing yet.\n\r", ch);
    return;
  }
  if(ch->pcdata->edit_obj)
  {/* They're editing an object of some type, check the edit specific commands */
    obj = ch->pcdata->edit_obj;
    if(obj == NULL)
    {
      send_to_char("Bad edit obj, please edit again.\n\r", ch);
      bug("Bad edit obj for editing.", 0);
      return;
    }
    if(!str_prefix(arg, "desc"))
    {
      if(IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("Exits have no desc.\n\r", ch);
        return;
      }

      if(ch->pcdata->edit_str == NULL)
      {
        send_to_char("You must have a desc to edit to use desc.\n\r", ch);
        return;
      }

      argument = one_argument(argument, arg);

      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Syntax: hedit desc <command> <text>  See help text edit for commands.\n\r", ch);
        else
          send_to_char("Syntax: pedit desc <command> <text>  See help text edit for commands.\n\r", ch);
        return;
      }

      if(!str_prefix(arg, "clear"))
      {
        ch->pcdata->edit_str[0] = '\0';
        ch->pcdata->edit_count = 0;
        ch->pcdata->edit_len = 0;
        send_to_char("Desc cleared.\n\r", ch);
        return;
      }

      if(!str_prefix(arg, "show"))
      {
        send_to_char("Current desc:\n\r", ch);
        send_to_char(ch->pcdata->edit_str, ch);
        send_to_char("\n\r", ch);
        return;
      }

      if(!str_prefix(arg, ">"))
      {
        if(argument[0] == '\0')
        {
          send_to_char("You must provide text to append.\n\r", ch);
          return;
        }
        do_long_edit(ch, argument, LONG_EDIT_DESC, LONG_EDIT_EDIT);
        return;
      }

      if ( !str_cmp( arg, "+" ) )
      {
        do_long_edit(ch,argument, LONG_EDIT_DESC, LONG_EDIT_NEWLINE);
        return;
      }

      if ( !str_cmp( arg, "++" ) )
      {
        do_long_edit(ch,argument, LONG_EDIT_DESC, LONG_EDIT_PARAGRAPH);
        return;
      }

      if (!str_cmp(arg,"-"))
      {
        do_long_edit(ch,argument, LONG_EDIT_DESC, LONG_EDIT_DELETE);
        return;
      }
      send_to_char("Invalid command for desc.  Please see 'help text edit' for commands.\n\r", ch);
      return;
    }/* End desc */
    if(!str_prefix(arg, "label"))
    {
      argument = one_argument(argument, arg);
      blast_punctuation(arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit label <label>\n\r", ch);
        else
          send_to_char("Usage: pedit label <label>\n\r", ch);
        return;
      }

      if(strlen(arg) > 20)
      {
        send_to_char("Label too long, maximum is 20 characters.\n\r", ch);
        return;
      }

      if(!str_cmp(arg, "exit") || !str_cmp(arg, "default"))
      {
        send_to_char("That is a reserved label, please choose another.\n\r", ch);
        return;
      }

      if(find_edit_obj(ch, arg, hedit) != NULL)
      {
        sprintf(buf, "%s already in use.\n\r", arg);
        send_to_char(buf, ch);
        return;
      }
      clear_string(&obj->label, arg);
      sprintf(buf, "Label set to %s.\n\r", arg);
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end label */
    if(!str_prefix(arg, "name"))
    {/* Rooms can be anything, everything else is lowercase letters only. */
      if(!IS_SET(obj->type, PLAN_ROOM))
      {
        lower_only(argument);
      }
      if(argument[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit name <name>\n\r", ch);
        else
          send_to_char("Usage: pedit name <name>\n\r", ch);
        return;
      }
      if(strlen(argument) > 50)
      {
        send_to_char("Name too long, maximum is 50 characters.\n\r", ch);
        return;
      }
      clear_string(&obj->name, argument);
      sprintf(buf, "%s name set to %s\n\r", obj->label, argument);
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }
    if(!str_prefix(arg, "hregen"))
    {
      int value, max;
      if(!IS_SET(obj->type, PLAN_ROOM))
      {
        send_to_char("You can only edit hregen for a room.\n\r", ch);
        return;
      }
      if(IS_SET(obj->flags, (PLAN_ROOM_LAB | PLAN_ROOM_ALTAR)))
      {
        send_to_char("You can't add hit regen to a magelab or altar room.\n\r", ch);
        return;
      }
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit hregen <number>\n\r", ch);
        else
          send_to_char("Usage: pedit hregen <number>\n\r", ch);
        return;
      }
      /* Check if valid range */
      if(hedit)
        max = PRICE_R_REGEN_END;
      else
        max = PRICE_R_REGEN_MID;
      if(!is_number(arg) || (value = atoi(arg)) < 0 || value > max)
      {
        sprintf(buf, "hregen value must be from 0 to %d.\n\r", max);
        send_to_char(buf, ch);
        return;
      }
      obj->opt[0] = value;
      if(obj->opt[0] > 0 || obj->opt[1] > 0)
        SET_BIT(obj->flags, PLAN_ROOM_REGEN);
      else
        REMOVE_BIT(obj->flags, PLAN_ROOM_REGEN);
      set_obj_cost(ch, obj, hedit);
      sprintf(buf, "Hit regen value set to %d (%d%% regen), cost set to %d %s",
          obj->opt[0], GET_REGEN(obj->opt[0], 0),
          COST_STR(obj->cost, hedit));
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end hregen */
    if(!str_prefix(arg, "mregen"))
    {
      int value, max;
      if(!IS_SET(obj->type, PLAN_ROOM))
      {
        send_to_char("You can only edit mregen for a room.\n\r", ch);
        return;
      }
      if(IS_SET(obj->flags, (PLAN_ROOM_LAB | PLAN_ROOM_ALTAR)))
      {
        send_to_char("You can't add hit regen to a magelab or altar room.\n\r", ch);
        return;
      }
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit mregen <number>\n\r", ch);
        else
          send_to_char("Usage: pedit mregen <number>\n\r", ch);
        return;
      }
      /* Check if valid range */
      if(hedit)
        max = PRICE_R_REGEN_END;
      else
        max = PRICE_R_REGEN_MID;
      if(!is_number(arg) || (value = atoi(arg)) < 0 || value > max)
      {
        sprintf(buf, "mregen value must be from 0 to %d.\n\r", max);
        send_to_char(buf, ch);
        return;
      }
      obj->opt[1] = value;
      if(obj->opt[0] > 0 || obj->opt[1] > 0)
        SET_BIT(obj->flags, PLAN_ROOM_REGEN);
      else
        REMOVE_BIT(obj->flags, PLAN_ROOM_REGEN);
      set_obj_cost(ch, obj, hedit);
      sprintf(buf, "Mana regen value set to %d (%d%% regen), cost set to %d %s",
          obj->opt[1], GET_REGEN(obj->opt[1], 0),
          COST_STR(obj->cost, hedit));
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end mregen */
    if(hedit)
    {/* Lab and altar is hall edit only */
      if(!str_prefix(arg, "magelab"))
      {
        int value;
        if(!IS_SET(obj->type, PLAN_ROOM))
        {
          send_to_char("You can only set a magelab for a room.\n\r", ch);
          return;
        }
        if(IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_ALTAR)))
        {
          send_to_char("You can't add a magelab to a room with regen or an altar.\n\r", ch);
          return;
        }
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          send_to_char("Usage: hedit magelab <number>\n\r", ch);
          return;
        }
        if(!is_number(arg) || (value = atoi(arg)) < 0 || value > PRICE_LAB_COUNT)
        {
          sprintf(buf, "Magelab value must be from 0 to %d.\n\r", PRICE_LAB_COUNT);
          send_to_char(buf, ch);
          return;
        }
        obj->opt[0] = value;
        if(obj->opt[0] > 0)
          SET_BIT(obj->flags, PLAN_ROOM_LAB);
        else
          REMOVE_BIT(obj->flags, PLAN_ROOM_LAB);
        set_obj_cost(ch, obj, hedit);
        sprintf(buf, "Magelab value set to %d, cost set to %d %s",
            obj->opt[0], COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end magelab */
      if(!str_prefix(arg, "altar"))
      {
        int value;
        if(!IS_SET(obj->type, PLAN_ROOM))
        {
          send_to_char("You can only set an altar for a room.\n\r", ch);
          return;
        }
        if(IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_LAB)))
        {
          send_to_char("You can't add an altar to a room with regen or a magelab.\n\r", ch);
          return;
        }
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          send_to_char("Usage: hedit altar <number>\n\r", ch);
          return;
        }
        if(!is_number(arg) || (value = atoi(arg)) < 0 || value > PRICE_ALTAR_COUNT)
        {
          sprintf(buf, "Altar value must be from 0 to %d.\n\r", PRICE_ALTAR_COUNT);
          send_to_char(buf, ch);
          return;
        }
        obj->opt[0] = value;
        if(obj->opt[0] > 0)
          SET_BIT(obj->flags, PLAN_ROOM_ALTAR);
        else
          REMOVE_BIT(obj->flags, PLAN_ROOM_ALTAR);
        set_obj_cost(ch, obj, hedit);
        sprintf(buf, "Altar value set to %d, cost set to %d %s.\n\r",
            obj->opt[0], COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end altar */
    }/* end if hedit for rooms */
    if(!str_prefix(arg, "outdoors") || !str_prefix(arg, "indoors"))
    {
      if(!IS_SET(obj->type, PLAN_ROOM))
      {
        send_to_char("You can only toggle outdoors for a room.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_ROOM_OUTDOORS);
      if(IS_SET(obj->flags, PLAN_ROOM_OUTDOORS))
      {
        sprintf(buf, "%s is now outdoors.\n\r", obj->label);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s is now indoors.\n\r", obj->label);
        send_to_char(buf, ch);
      }
      if(IS_SET(obj->flags, (PLAN_ROOM_ALTAR | PLAN_ROOM_LAB)))
      {
        send_to_char("Magelab or altar override outdoor setting, they will be indoors.\n\r", ch);
        return;
      }
      ch->pcdata->edits_made = TRUE;
      return;
    } /* end outdoors */
    if(!str_prefix(arg, "dark") || !str_prefix(arg, "light"))
    {
      if(!IS_SET(obj->type, PLAN_ROOM))
      {
        send_to_char("You can only toggle dark for a room.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_ROOM_DARK);
      if(IS_SET(obj->flags, PLAN_ROOM_DARK))
      {
        sprintf(buf, "%s is now dark.\n\r", obj->label);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s now has light.\n\r", obj->label);
        send_to_char(buf, ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    } /* end dark */
    if(!str_prefix(arg, "short"))
    {
      if(!IS_SET(obj->type, (PLAN_MOB | PLAN_ITEM)))
      {
        send_to_char("You can only set the short for a mob or item.\n\r", ch);
        return;
      }
      if(argument[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit short <short>\n\r", ch);
        else
          send_to_char("Usage: pedit short <short>\n\r", ch);
        return;
      }
      if(strlen(argument) > 30)
      {
        send_to_char("Short too long, maximum is 30 characters.\n\r", ch);
        return;
      }
      clear_string(&obj->short_d, argument);
      sprintf(buf, "%s short set to %s\n\r", obj->label, argument);
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end short */
    if(!str_prefix(arg, "long"))
    {
      if(!IS_SET(obj->type, PLAN_MOB | PLAN_ITEM))
      {
        send_to_char("You can only set the long for a mob or an item.\n\r", ch);
        return;
      }
      if(argument[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit long <long>\n\r", ch);
        else
          send_to_char("Usage: pedit long <long>\n\r", ch);
        return;
      }
      if(strlen(argument) > 80)
      {
        send_to_char("Long too long, maximum is 80 characters.\n\r", ch);
        return;
      }
      clear_string(&obj->long_d, argument);
      sprintf(buf, "%s long set to %s\n\r", obj->label, argument);
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end long */
    if(!str_prefix(arg, "type"))
    {
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit type <type>\n\r", ch);
        else
          send_to_char("Usage: pedit type <type>\n\r", ch);
        return;
      }
      if((hedit && !IS_SET(obj->type, (PLAN_MOB | PLAN_ITEM)))
          || (!hedit && !IS_SET(obj->type, PLAN_ITEM)))
      {
        if(hedit)
          send_to_char("You can only set the type for an item.\n\r", ch);
        else
          send_to_char("You can only set the type for an item.\n\r", ch);
        return;
      }
      if(IS_SET(obj->type, PLAN_ITEM))
      {
        if(IS_SET(obj->flags, PLAN_MASK_OBJ))
        {
          send_to_char("You may only set a type on a new object.\n\rUse delete if you don't want this object around any more.\n\r", ch);
          return;
        }
        if(!str_prefix(arg, "pit"))
          obj->flags = PLAN_ITEM_PIT;
        //else if(!str_prefix(arg, "drink"))
        //  obj->flags = PLAN_ITEM_DRINK;
        else if(!str_prefix(arg, "fountain"))
          obj->flags = PLAN_ITEM_FOUNTAIN;
        else if(!str_prefix(arg, "furniture"))
          obj->flags = PLAN_ITEM_FURNITURE;
        else if(!str_prefix(arg, "doodad"))
          obj->flags = PLAN_ITEM_DOODAD;
        else if(hedit && !str_prefix(arg, "portal"))
          obj->flags = PLAN_ITEM_PORTAL;
        else
        {
          if(hedit)
            send_to_char("Valid types: pit, fountain, furniture, doodad, portal.\n\r", ch);
          else
            send_to_char("Valid types: pit, fountain, furniture, doodad.\n\r", ch);
          return;
        }
        set_obj_cost(ch, obj, hedit);
        switch(obj->flags)
        {
          case PLAN_ITEM_PIT: sprintf(buf, "The item is now a pit, cost is %d %s.\n\r",
                      COST_STR(obj->cost, hedit)); break;
          case PLAN_ITEM_DRINK: sprintf(buf, "The item is now a drink, cost is %d %s.\n\r",
                        COST_STR(obj->cost, hedit)); break;
          case PLAN_ITEM_FOUNTAIN: sprintf(buf, "The item is now a fountain, cost is %d %s.\n\r",
                     COST_STR(obj->cost, hedit)); break;
          case PLAN_ITEM_FURNITURE: sprintf(buf, "The item is now furniture, cost is %d %s.\n\r",
                      COST_STR(obj->cost, hedit)); break;
          case PLAN_ITEM_DOODAD: sprintf(buf, "The item is now a doodad, cost is %d %s.\n\r",
                         COST_STR(obj->cost, hedit)); break;
          case PLAN_ITEM_PORTAL: sprintf(buf, "The item is now a portal, cost is %d %s.\n\r",
                         COST_STR(obj->cost, hedit)); break;
        }
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }
      else if(hedit)
      {
        send_to_char("Mob type is not currently available.\n\r", ch);
        return;
        if(!str_prefix(arg, "merchant"))
        {
          if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
          {
            send_to_char("That mob is already a merchant.\n\r", ch);
            return;
          }
          obj->flags = PLAN_MOB_MERCHANT;/* Remove anything previously set */
          obj->opt[0] = 0; /* Clear values */
          obj->opt[1] = 0;
          set_obj_cost(ch, obj, hedit);
          sprintf(buf, "%s set to type merchant, cost is now %d %s.\n\r",
              obj->label, COST_STR(obj->cost, hedit));
          send_to_char(buf, ch);
          ch->pcdata->edits_made = TRUE;
          return;
        }
        if(!str_prefix(arg, "healer"))
        {
          if(IS_SET(obj->flags, PLAN_MOB_HEALER))
          {
            send_to_char("That mob is already a healer.\n\r", ch);
            return;
          }
          obj->flags = PLAN_MOB_HEALER;/* Remove anything previously set */
          obj->opt[0] = 0; /* Clear values */
          obj->opt[1] = 0;
          set_obj_cost(ch, obj, hedit);
          sprintf(buf, "%s set to type healer, cost is now %d %s.\n\r",
              obj->label, COST_STR(obj->cost, hedit));
          send_to_char(buf, ch);
          ch->pcdata->edits_made = TRUE;
          return;
        }
        send_to_char("Valid types: merchant, healer.\n\r", ch);
        return;
      }/* End if hedit for merchant or healer */
      return;
    }/* end type */
    if(hedit)
    {
      if(!str_prefix(arg, "discount"))
      {
        int value;
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          if(hedit)
            send_to_char("Usage: hedit discount <value>\n\r", ch);
          else/* May as well leave this in case it ever goes into pedit */
            send_to_char("Usage: pedit discount <value>\n\r", ch);
          return;
        }
        if(!IS_SET(obj->type, PLAN_MOB) || !IS_SET(obj->flags, PLAN_MOB_MERCHANT))
        {
          send_to_char("You may only set a discount on a merchant mob.\n\r", ch);
          return;
        }
        if(!is_number(arg) || (value = atoi(arg)) < 0 || value > PRICE_M_DISCOUNT_COUNT)
        {
          sprintf(buf, "Discount value must be from 0 to %d.\n\r", PRICE_M_DISCOUNT_COUNT);
          send_to_char(buf, ch);
          return;
        }
        obj->opt[0] = value;
        set_obj_cost(ch, obj, hedit);
        sprintf(buf, "Discount value set to %d (%d%%), cost set to %d %s.\n\r",
            obj->opt[0], obj->opt[0] * 10, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end discount */
      if(!str_prefix(arg, "level"))
      {
        int value;
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          if(hedit)
            send_to_char("Usage: hedit level <value>\n\r", ch);
          else/* May as well leave this in case it ever goes into pedit */
            send_to_char("Usage: pedit level <value>\n\r", ch);
          return;
        }
        if(!IS_SET(obj->type, PLAN_MOB) || !IS_SET(obj->flags, PLAN_MOB_HEALER))
        {
          send_to_char("You may only set a level on a healer mob.\n\r", ch);
          return;
        }
        if(!is_number(arg) || (value = atoi(arg)) < 0 || value > PRICE_H_LEVEL_COUNT)
        {
          sprintf(buf, "Level must be from 0 to %d.\n\r", PRICE_H_LEVEL_COUNT);
          send_to_char(buf, ch);
          return;
        }
        obj->opt[0] = value;
        set_obj_cost(ch, obj, hedit);
        sprintf(buf, "Level set to %d (Level %d), cost set to %d %s.\n\r",
            obj->opt[0], obj->opt[0] + 40, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end level */
      if(!str_prefix(arg, "race"))
      {
        int race;
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          if(hedit)
            send_to_char("Usage: hedit race <race>\n\r", ch);
          else/* May as well leave this in case it ever goes into pedit */
            send_to_char("Usage: pedit race <race>\n\r", ch);
          return;
        }
        race = race_lookup(arg);
        if(race == 0)
        {
          send_to_char("That is not a race, please select another.\n\r", ch);
          return;
        }
        if (!race_table[race].pc_race || race == race_lookup("mutant") || race == race_lookup("smurf"))
        {
          send_to_char("That race is not player selectable, please choose another.\n\r", ch);
          return;
        }
        obj->opt[1] = race;
        sprintf(buf, "Race set to %s.\n\r",
            race_table[race].name);
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end race */
      if(!str_prefix(arg, "alignment"))
      {
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          if(hedit)
            send_to_char("Usage: hedit align <alignment>\n\r", ch);
          else/* May as well leave this in case it ever goes into pedit */
            send_to_char("Usage: pedit align <alignment>\n\r", ch);
          return;
        }
        if(!str_prefix(arg, "good"))
        {
          REMOVE_BIT(obj->flags, PLAN_MOB_EVIL);
          SET_BIT(obj->flags, PLAN_MOB_GOOD);
          sprintf(buf, "%s is now good aligned.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else if(!str_prefix(arg, "evil"))
        {
          REMOVE_BIT(obj->flags, PLAN_MOB_GOOD);
          SET_BIT(obj->flags, PLAN_MOB_EVIL);
          sprintf(buf, "%s is now evil aligned.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else if(!str_prefix(arg, "neutral"))
        {
          REMOVE_BIT(obj->flags, PLAN_MOB_EVIL);
          REMOVE_BIT(obj->flags, PLAN_MOB_GOOD);
          sprintf(buf, "%s is now neutral aligned.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else
        {
          send_to_char("Valid alignments are good, neutral, or evil.\n\r", ch);
          return;
        }
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end align */
      if(!str_prefix(arg, "gender") || !str_prefix(arg, "sex"))
      {
        argument = one_argument(argument, arg);
        if(arg[0] == '\0')
        {
          if(hedit)
            send_to_char("Usage: hedit gender <gender>\n\r", ch);
          else/* May as well leave this in case it ever goes into pedit */
            send_to_char("Usage: pedit gender <gender>\n\r", ch);
          return;
        }
        if(!str_prefix(arg, "male"))
        {
          REMOVE_BIT(obj->flags, PLAN_MOB_FEMALE);
          REMOVE_BIT(obj->flags, PLAN_MOB_NEUTER);
          sprintf(buf, "%s is now male.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else if(!str_prefix(arg, "female"))
        {
          SET_BIT(obj->flags, PLAN_MOB_FEMALE);
          REMOVE_BIT(obj->flags, PLAN_MOB_NEUTER);
          sprintf(buf, "%s is now female.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else if(!str_prefix(arg, "none") || !str_prefix(arg, "neuter"))
        {
          REMOVE_BIT(obj->flags, PLAN_MOB_FEMALE);
          SET_BIT(obj->flags, PLAN_MOB_NEUTER);
          sprintf(buf, "%s now has no gender.\n\r", obj->label);
          send_to_char(buf, ch);
        }
        else
        {
          send_to_char("Valid genders are male, female, or none.\n\r", ch);
          return;
        }
        ch->pcdata->edits_made = TRUE;
        return;
      }/* end gender */
      if(!str_prefix(arg, "target"))
      {/* Portal destination */
        int new_target;
        char *name;
        if(argument[0] == '\0')
        {
          send_to_char("Portals: Hoan Dor, Drow City, Sands of Sorrow, Ancient Thalos.\n\r", ch);
          send_to_char("         Elemental Canyon, Pyramids, Emerald Forest, Atlantis.\n\r", ch);
          send_to_char("         Camelot, City of Thieves, New Thalos.\n\r", ch);
          return;
        }
        if(!str_prefix(argument, "hoan dor"))
        {
          new_target = VNUM_PORTAL_HOAN;
          name = "Haon Dor";
        }
        else if(!str_prefix(argument, "drow city"))
        {
          new_target = VNUM_PORTAL_DROW;
          name = "Drow City";
        }
        else if(!str_prefix(argument, "sands of sorrow"))
        {
          new_target = VNUM_PORTAL_SANDS;
          name = "Sands of Sorrow";
        }
        else if(!str_prefix(argument, "ancient thalos"))
        {
          new_target = VNUM_PORTAL_THALOS;
          name = "Ancient Thalos";
        }
        else if(!str_prefix(argument, "elemental canyon"))
        {
          new_target = VNUM_PORTAL_CANYON;
          name = "Elemental Canyon";
        }
        else if(!str_prefix(argument, "pyramids"))
        {
          new_target = VNUM_PORTAL_PYRAMIDS;
          name = "Pyramids";
        }
        else if(!str_prefix(argument, "emerald forest"))
        {
          new_target = VNUM_PORTAL_EMERALD;
          name = "Emerald Forest";
        }
        else if(!str_prefix(argument, "atlantis"))
        {
          new_target = VNUM_PORTAL_ATLANTIS;
          name = "Atlantis";
        }
        else if(!str_prefix(argument, "camelot"))
        {
          new_target = VNUM_PORTAL_CAMELOT;
          name = "Camelot";
        }
        else if(!str_prefix(argument, "city of thieves"))
        {
          new_target = VNUM_PORTAL_THIEVES;
          name = "City of Thieves";
        }
        else if(!str_prefix(argument, "new thalos"))
        {
          new_target = VNUM_PORTAL_NEW_THALOS;
          name = "New Thalos";
        }
        else
        {
          send_to_char("That is not a valid portal destination.\n\r", ch);
          return;
        }
        if(obj->opt[0] == new_target)
        {
          sprintf(buf, "%s is already the target for %s.\n\r", name, obj->label);
          send_to_char(buf, ch);
          return;
        }
        obj->opt[0] = new_target;
        sprintf(buf, "%s is now the target for %s.\n\r", name, obj->label);
        send_to_char(buf, ch);
        ch->pcdata->edits_made = TRUE;
        return;
      }
    }/* End if hedit for item/mob hall only settings */
    if(!str_prefix(arg, "regen"))
    {
      int value;
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit regen <value>\n\r", ch);
        else
          send_to_char("Usage: pedit regen <value>\n\r", ch);
        return;
      }
      if(!IS_SET(obj->type, PLAN_ITEM) || !IS_SET(obj->flags, PLAN_ITEM_FURNITURE))
      {
        send_to_char("You may only set regen on furniture.\n\r", ch);
        return;
      }
      if(!is_number(arg) || (value = atoi(arg)) < 0 || value > PRICE_F_REGEN_COUNT)
      {
        sprintf(buf, "Regen value must be from 0 to %d.\n\r", PRICE_F_REGEN_COUNT);
        send_to_char(buf, ch);
        return;
      }
      obj->opt[0] = value;
      set_obj_cost(ch, obj, hedit);
      sprintf(buf, "Regen value set to %d (%d%%), cost set to %d %s.\n\r",
          obj->opt[0], obj->opt[0] * 5, COST_STR(obj->cost, hedit));
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end regen */
    if(!str_prefix(arg, "liquid"))
    {
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(hedit)
          send_to_char("Usage: hedit liquid <type>\n\r", ch);
        else
          send_to_char("Usage: pedit liquid <type>\n\r", ch);
        do_help(ch, "liquid");
        return;
      }
      if(!IS_SET(obj->type, PLAN_ITEM) || !IS_SET(obj->flags, (PLAN_ITEM_DRINK | PLAN_ITEM_FOUNTAIN)))
      {
        if(hedit)
          send_to_char("You may only set the liquid for fountains or drinks.\n\r", ch);
        else
          send_to_char("You may only set the liquid for fountains.\n\r", ch);
        return;
      }
      obj->opt[0] = liq_lookup(arg);
      if(obj->opt[0] < 0)
        obj->opt[0] = 0;
      sprintf(buf, "Liquid set to %s.\n\r", liq_table[obj->opt[0]].liq_name);
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end liquid */
/*    if(!str_prefix(arg, "hidden"))
    {
      if(!IS_SET(obj->type, (PLAN_ITEM | PLAN_EXIT)) || obj->flags == 0 || IS_SET(obj->flags, PLAN_ITEM_DRINK))
      {
        send_to_char("You can only toggle hidden for non-drink items.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_ITEM_HIDDEN);
      set_obj_cost(ch, obj, hedit);
      if(IS_SET(obj->flags, PLAN_ITEM_HIDDEN))
      {
        sprintf(buf, "%s will now be hidden, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
      }
      else
      {
        sprintf(buf, "%s will now be visible, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
      }
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }*//* end hidden */
    if(!str_prefix(arg, "door"))
    {
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only toggle a door for an exit.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_EXIT_CLOSABLE);
      set_obj_cost(ch, obj, hedit);
      if(IS_SET(obj->flags, PLAN_EXIT_CLOSABLE))
      {/* Remember, this is opposite of how it started */
        sprintf(buf, "%s now has a door, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s no longer has a door, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
        send_to_char("Its settings will be preserved for if you set a door on it again.\n\r", ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end door */
    if(!str_prefix(arg, "closed") || !str_prefix(arg, "open"))
    {
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only toggle a door for starting closed.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_EXIT_CLOSED);/* Costs don't change */
      if(IS_SET(obj->flags, PLAN_EXIT_CLOSED))
      {/* Remember, this is opposite of how it started */
        sprintf(buf, "%s will now start closed if it has a door.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s will now start open.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end closed */
    if(!str_prefix(arg, "key"))
    {
      OBJ_DATA *key;
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only set a key on a door.\n\r", ch);
        return;
      }
      argument = one_argument(argument, arg);
      if(arg[0] == '\0')
      {
        if(IS_SET(obj->flags, PLAN_EXIT_LOCKABLE))
        {
          REMOVE_BIT(obj->flags, PLAN_EXIT_LOCKABLE);
          set_obj_cost(ch, obj, hedit);
          sprintf(buf, "Key cleared, %s is no longer lockable. Cost is %d %s.\n\r",
              obj->label, COST_STR(obj->cost, hedit));
          ch->pcdata->edits_made = TRUE;
          obj->opt[0] = 0;
        }
        else
          send_to_char("That exit already has no key.\n\r", ch);
        return;
      }
      /* Find the key item */
      if((key = get_obj_carry(ch, arg)) == NULL)
      {
        if((key = get_obj_wear(ch, arg)) == NULL)
        {
          send_to_char("The key must be an object you are carrying or wearing.\n\r", ch);
          return;
        }
      }
      obj->opt[0] = key->pIndexData->vnum;
      SET_BIT(obj->flags, PLAN_EXIT_LOCKABLE);
      set_obj_cost(ch, obj, hedit);
      sprintf(buf, "%s is now lockable with key %s, cost is %d %s.\n\r",
          obj->label, key->short_descr, COST_STR(obj->cost, hedit));
      send_to_char(buf, ch);
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end key */
    if(!str_prefix(arg, "locked"))
    {
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only toggle a door for starting locked.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_EXIT_LOCKED);/* Costs don't change */
      if(IS_SET(obj->flags, PLAN_EXIT_LOCKED))
      {/* Remember, this is opposite of how it started */
        sprintf(buf, "%s will now start locked if it can be locked.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s will now start unlocked.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end locked */
    if(!str_prefix(arg, "pickproof"))
    {
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only toggle a door for being pickproof.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_EXIT_NOPICK);
      set_obj_cost(ch, obj, hedit);
      if(IS_SET(obj->flags, PLAN_EXIT_NOPICK))
      {/* Remember, this is opposite of how it started */
        sprintf(buf, "%s will now be pickproof if it is locked, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s is no longer pickproof, its cost is %d %s.\n\r",
            obj->label, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end pickproof */
    if(!str_prefix(arg, "nopass"))
    {
      if(!IS_SET(obj->type, PLAN_EXIT))
      {
        send_to_char("You can only toggle a door for being passed through.\n\r", ch);
        return;
      }
      TOGGLE_BIT(obj->flags, PLAN_EXIT_NOPASS);/* Costs don't change */
      if(IS_SET(obj->flags, PLAN_EXIT_NOPASS))
      {/* Remember, this is opposite of how it started */
        sprintf(buf, "%s can't be passed through while it's closed.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      else
      {
        sprintf(buf, "%s can now be passed through while it's closed.\n\r",
            obj->label);
        send_to_char(buf, ch);
      }
      ch->pcdata->edits_made = TRUE;
      return;
    }/* end locked */
  }/* End editing */
  /* They're doing general editing, check the top level commands */
  if(!str_prefix(arg, "edit"))
  {
    if(!check_can_edit(ch, CLAN_CAN_EDIT, hedit))
    {
      send_to_char("You don't have permission to edit.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == 0)
    {
      if(hedit)
        send_to_char("Usage: hedit edit <label>\n\r", ch);
      else
        send_to_char("Usage: pedit edit <label>\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      sprintf(buf, "%s not found to edit.\n\r", arg);
      send_to_char(buf, ch);
      return;
    }
    if(IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
    {
      send_to_char("You may not edit a placed or previewed object.\n\r", ch);
      return;
    }
    if(obj->editing == TRUE)
    {
      send_to_char("Someone else is editing that object already.\n\r", ch);
      return;
    }
    sprintf(buf, "You are now editing object %s.\n\r", arg);
    send_to_char(buf, ch);
    if(ch->pcdata->edit_obj)
    {
      ch->pcdata->edit_obj->editing = FALSE; /* Release the previous one */
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
    }
    ch->pcdata->edit_obj = obj;
    if(!IS_SET(obj->type, PLAN_EXIT | PLAN_ITEM))
      start_long_edit(ch, MAX_CUSTOM_DESC, LONG_EDIT_DESC, ch->pcdata->edit_obj->desc);
    obj->editing = TRUE;
    return;/* Editing does not mark edits_made */
  }/* End edit */
  if(!str_prefix(arg, "list"))
  {
    int listed = 0;
    long type = 0;
    argument = one_argument(argument, arg);
    if(!str_cmp(arg, "room"))
      type = PLAN_ROOM;
    else if(!str_cmp(arg, "item"))
      type = PLAN_ITEM;
    else if(!str_cmp(arg, "exit"))
    {
      send_to_char("Sorry, exits may not be listed yet.\n\r", ch);
      return;
      type = PLAN_EXIT;
    }
    else if(!str_cmp(arg, "mob") && hedit)
      type = PLAN_MOB;
    else if(arg[0] != '\0' && str_cmp(arg, "all"))
    {/* All leaves it at 0 */
      if(hedit)
      {
        send_to_char("Usage: hedit list <type> <(Optional)planned|previewed|placed|all>\n\r", ch);
        send_to_char("type may be: room, mob, item, exit, all\n\r", ch);
      }
      else
      {
        send_to_char("Usage: pedit list <type> <(Optional)planned|previewed|placed|all>\n\r", ch);
        send_to_char("type may be: room, item, exit, all\n\r", ch);
      }
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0' || !str_prefix(arg, "planned"))
      type |= PLAN_PLANNED;
    else if(!str_prefix(arg, "placed"))
      type |= PLAN_PLACED;
    else if(!str_prefix(arg, "previewed"))
      type |= PLAN_PREVIEWED;
    else if(str_prefix(arg, "all"))
    {/* All doesn't modify the flag */
      send_to_char("Valid filters are: planned, placed, all. If empty, planned will be used.\n\r", ch);
      return;
    }
    listed = count_edit_obj(ch, type, TRUE, hedit);
    if(listed == 0)
    {
      send_to_char("No objects of the selected type were found.\n\r", ch);
    }
    return;/* Listing does not mark edits_made */
  }/* End list */
  if(!str_prefix(arg, "display") || !str_prefix(arg, "show"))
  {
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(ch->pcdata->edit_obj)
        display_obj(ch, ch->pcdata->edit_obj);
      else
        send_to_char("Display which object?\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      send_to_char("That is not a valid object.\n\r", ch);
      return;
    }
    display_obj(ch, obj);
    return;
  }
  if(!str_prefix(arg, "create"))
  {
    PLAN_DATA **head;
    char prefix[6];
    PLAN_DATA *start;
    int len = 0, high = 1;
    int type = 0, count = -1;
    if(!check_can_edit(ch, CLAN_CAN_CREATE, hedit))
    {
      send_to_char("You do not have permission to create.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(!str_cmp(arg, "room"))
      type = PLAN_ROOM;
    else if(!str_cmp(arg, "item"))
      type = PLAN_ITEM;
    else if(!str_cmp(arg, "exit"))
    {
      send_to_char("Sorry, exits may not be created yet.\n\r", ch);
      return;
      type = PLAN_EXIT;
    }
    else if(!str_cmp(arg, "mob") && hedit)
      type = PLAN_MOB;
    else
    {
      if(hedit)
      {
        send_to_char("Usage: hedit create <type> <label>\n\r", ch);
        send_to_char("type may be: room, mob, item, exit\n\r", ch);
      }
      else
      {
        send_to_char("Usage: pedit create <type> <label>\n\r", ch);
        send_to_char("type may be: room, item, exit\n\r", ch);
      }
      return;
    }
    argument = one_argument(argument, arg);
    blast_punctuation(arg);
    if(arg[0] != '\0' && (!str_cmp(arg, "exit") || !str_cmp(arg, "type") ||
      !str_cmp(arg, "exitlink") || get_arg_dir(arg) != -1))
    {
      send_to_char("Invalid label to create an object.\n\r", ch);
      return;
    }
    count = count_edit_obj(ch, type, FALSE, hedit);
    switch(type)/* type doesn't need masking here */
    {
      case PLAN_ROOM: if(count >= 50) count = -1;
        strcpy(prefix, "room_"); len = 5;
        break;
      case PLAN_MOB: if(count >= 10) count = -1;
        strcpy(prefix, "mob_"); len = 4;
        break;
      case PLAN_ITEM: if(count >= 99) count = -1;
        strcpy(prefix, "item_"); len = 5;
        break;
      case PLAN_EXIT: if(count >= 100) count = -1;
        strcpy(prefix, "exit_"); len = 5;
        break;
      default: count = -1; break;
    }
    if(count < 0)
    {
      send_to_char("You have the maximum for that type of object already.\n\r", ch);
      return;
    }
    if(arg[0] == '\0')
    {/* Autonumber */
      high = 1;
      for(start = START_OBJ(ch, hedit); start != NULL; start = start->next)
      {
        for(i = 0; i < len; i++)
        {
          if(start->label[i] != prefix[i])
            break;
        }
        if(i == len)
        {/* Match */
          if(is_number(start->label + len))
          {
            i = atoi(start->label + len);
            if(i >= high)
              high = i + 1;
            if(high < 0)
              high = 0; /* Bad.  It can get caught later if one already is set to 0. */
          }
        }
      }
      sprintf(buf, "%s%d", prefix, high);
    }
    else
      strcpy(buf, arg);
    if(find_edit_obj(ch, buf, hedit) != NULL)
    {
      send_to_char("An object with that label already exists.\n\r", ch);
      return;
    }
    obj = new_plan();
    obj->type = type | PLAN_PLANNED;
    obj->clan = ch->pcdata->clan_info->clan;
    if(type == PLAN_ROOM)
      obj->exits = new_p_exit();
    else if(type == PLAN_MOB)
      obj->flags = PLAN_MOB_HEALER;
    /* Need to not line up with an existing index, can't just use count */
    obj->plan_index = new_obj_index(ch, type, hedit);
    set_obj_cost(ch, obj, hedit);
    if(hedit)
      head = &(ch->pcdata->clan_info->clan->planned);
    else
      head = &(ch->pcdata->clan_info->pers_plan);
    if(*head == NULL || type < ((*head)->type & PLAN_MASK_TYPE) ||
      obj->plan_index < (*head)->plan_index)
    {
      obj->next = (*head);
      *head = obj;
    }
    else
    {
      PLAN_DATA *prev = (*head);
      while(prev->next && (type > (prev->type & PLAN_MASK_TYPE) ||
        (type == (prev->type & PLAN_MASK_TYPE) && prev->next->plan_index < prev->plan_index)))
        prev = prev->next;
      obj->next = prev->next;
      prev->next = obj;
    }
    if(ch->pcdata->edit_obj)
    {
      ch->pcdata->edit_obj->editing = FALSE; /* Release the previous one */
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
    }
    ch->pcdata->edit_obj = obj;
    if(!IS_SET(obj->type, PLAN_EXIT | PLAN_ITEM))
      start_long_edit(ch, MAX_CUSTOM_DESC, LONG_EDIT_DESC, ch->pcdata->edit_obj->desc);
    obj->editing = TRUE;
    if(arg[0] == '\0')
    {
      sprintf(buf, "%s%d", prefix, high);
      obj->label = str_dup(buf);
    }
    else
      obj->label = str_dup(arg);
    sprintf(buf, "Object %s created, entering edit mode.\n\r", obj->label);
    send_to_char(buf, ch);
    ch->pcdata->edits_made = TRUE;
    return;
  }/* End create */
  if(!str_prefix(arg, "clone"))
  {
    if(!check_can_edit(ch, CLAN_CAN_CREATE, hedit))
    {
      send_to_char("You do not have permissions to clone.\n\r", ch);
      return;
    }
    PLAN_DATA *orig;
    argument = one_argument(argument, arg);
    int count = -1;
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit clone <original> <label>\n\r", ch);
      else
        send_to_char("Usage: pedit clone <original> <label>\n\r", ch);
      return;
    }
    if((orig = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      send_to_char("No object of that label exists to clone.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    blast_punctuation(arg);
    if(arg[0] == '\0')
    {
      send_to_char("Invalid label to create an object.\n\r", ch);
      return;
    }
    count = count_edit_obj(ch, orig->type & PLAN_MASK_TYPE, FALSE, hedit);
    switch(orig->type & PLAN_MASK_TYPE)
    {
      case PLAN_ROOM: if(count >= 50) count = -1; break;
      case PLAN_MOB: if(count >= 10) count = -1; break;
      case PLAN_ITEM: if(count >= 100) count = -1; break;
      case PLAN_EXIT: if(count >= 100) count = -1; break;
    }
    if(count < 0)
    {
      send_to_char("You have the maximum for that type of object already.\n\r", ch);
      return;
    }
    if(find_edit_obj(ch, arg, hedit) != NULL)
    {
      send_to_char("An object with that label already exists.\n\r", ch);
      return;
    }
    obj = new_plan();
    obj->type = (orig->type & PLAN_MASK_TYPE) | PLAN_PLANNED;
    if(IS_SET(obj->type, PLAN_ROOM))
      obj->exits = new_p_exit();
    obj->plan_index = new_obj_index(ch, (obj->type & PLAN_MASK_TYPE), hedit);

    /* Copy all details */
    obj->cost = orig->cost;
    obj->flagged = orig->flagged;
    if(orig->name) obj->name = str_dup(orig->name);
    if(orig->short_d) obj->short_d = str_dup(orig->short_d);
    if(orig->long_d) obj->long_d = str_dup(orig->long_d);
    if(orig->desc) obj->desc = str_dup(orig->desc);
    obj->opt[0] = orig->opt[0];
    obj->opt[1] = orig->opt[1];
    obj->flags = orig->flags;

    if(ch->pcdata->edit_obj)
    {
      ch->pcdata->edit_obj->editing = FALSE; /* Release the previous one */
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
    }
    ch->pcdata->edit_obj = obj;
    if(!IS_SET(obj->type, PLAN_EXIT | PLAN_ITEM))
      start_long_edit(ch, MAX_CUSTOM_DESC, LONG_EDIT_DESC, ch->pcdata->edit_obj->desc);
    obj->editing = TRUE;
    obj->label = str_dup(arg);
    sprintf(buf, "Object %s cloned from %s, entering edit mode.\n\r", obj->label, orig->label);
    send_to_char(buf, ch);
    /* Link it in */
    orig = START_OBJ(ch, hedit);
    while(orig->next && ((obj->type & PLAN_MASK_TYPE) > (orig->type & PLAN_MASK_TYPE) ||
      ((obj->type & PLAN_MASK_TYPE) == (orig->type & PLAN_MASK_TYPE) && orig->next->plan_index < orig->plan_index)))
      orig = orig->next;
    obj->next = orig->next;
    orig->next = obj;
    ch->pcdata->edits_made = TRUE;
    return;
  }/* End clone */
  if(!str_prefix(arg, "upgrade"))
  {/* Find a placed, upgradable object */
    PLAN_DATA *ch_room;
    if(!check_can_edit(ch, CLAN_CAN_PLACE, hedit))
    {
      send_to_char("You do not have permissions to upgrade.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit upgrade <label> <attribute>\n\r", ch);
      else
        send_to_char("Usage: pedit upgrade <label> <attribute>\n\r", ch);
      send_to_char("Leave attribute blank to check current upgradable stats.\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      sprintf(buf, "%s not found to upgrade.\n\r", arg);
      send_to_char(buf, ch);
      return;
    }
    if(!IS_SET(obj->type, PLAN_PLACED) || !obj->to_place)
    {
      send_to_char("You may only upgrade placed objects.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_EXIT))
    {
      send_to_char("You may not upgrade exits.\n\r", ch);
      return;
    }
    ch_room = find_char_room_obj(ch, hedit);
    if((IS_SET(obj->type, PLAN_ROOM) && obj != ch_room) ||
    (!IS_SET(obj->type, PLAN_ROOM) && obj->loc != ch_room->plan_index))
    {
      send_to_char("You must be in the same room as the object you wish to upgrade spawns in.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {/* Checking upgradable stats on this object */
      if(IS_SET(obj->type, PLAN_ROOM))
      {
        if(!IS_SET(obj->flags, (PLAN_ROOM_LAB | PLAN_ROOM_ALTAR)))
        {/* opt 0 is hit regen, 1 is mana regen */
          int max;
          if(hedit)
            max = PRICE_R_REGEN_END;
          else
            max = PRICE_R_REGEN_MID;
          if(obj->opt[0] >= max)
          {
            if(obj->opt[1] >= max)
              send_to_char("Regen: Hit and mana maxed.\n\r", ch);
            else
            {
              sprintf(buf, "Regen: Hit regen maxed, mana regen to %d%% for %d %s.\n\r",
                GET_REGEN(obj->opt[1], 1), PRICE_STR((PRICE_R_REGEN + obj->opt[1]), hedit));
              send_to_char(buf, ch);
            }
          }
          else if(obj->opt[1] >= max)
          {
            sprintf(buf, "Regen: Hit regen to %d%% for %d %s, mana regen maxed.\n\r",
              GET_REGEN(obj->opt[0], 1), PRICE_STR((PRICE_R_REGEN + obj->opt[0]), hedit));
            send_to_char(buf, ch);
          }
          else
          {
            sprintf(buf, "Regen: Upgrade hit regen to %d%% for %d %s, mana regen to %d%% for %d %s.\n\r",
              GET_REGEN(obj->opt[0], 1), PRICE_STR((PRICE_R_REGEN + obj->opt[0]), hedit),
              GET_REGEN(obj->opt[1], 1), PRICE_STR((PRICE_R_REGEN + obj->opt[1]), hedit));
            send_to_char(buf, ch);
          }
        }
        else
          send_to_char("Regen: Can't upgrade.\n\r", ch);
        if(hedit)
        {
          if(!IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_ALTAR)))
          {
            if(obj->opt[0] >= PRICE_LAB_COUNT)
              send_to_char("Magelab: maxed.\n\r", ch);
            else
            {
              sprintf(buf, "Magelab: Upgrade to %d for %d %s.\n\r",
                obj->opt[0] + 1, PRICE_STR((PRICE_LAB + obj->opt[0]), hedit));
              send_to_char(buf, ch);
            }
          }
          else
            send_to_char("Magelab: Can't upgrade.\n\r", ch);
          if(!IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_LAB)))
          {
            if(obj->opt[0] >= PRICE_ALTAR_COUNT)
              send_to_char("Altar: maxed.\n\r", ch);
            else
            {
              sprintf(buf, "Altar: Upgrade to %d for %d %s.\n\r",
                obj->opt[0] + 1, PRICE_STR((PRICE_ALTAR + obj->opt[0]), hedit));
              send_to_char(buf, ch);
            }
          }
          else
            send_to_char("Altar: Can't upgrade.\n\r", ch);
        }
      }
      else if(IS_SET(obj->type, PLAN_MOB))
      {/* Can't get in here if it's not hedit */
        bool can_up = FALSE;
        if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
        {
          can_up = TRUE;
          if(obj->opt[0] >= PRICE_M_DISCOUNT_COUNT)
          {
            sprintf(buf, "Discount: maxed.\n\rItems: %d %s per item.\n\r",
              PRICE_STR(PRICE_M_ITEM, hedit));
            send_to_char(buf, ch);
          }
          else
          {
            sprintf(buf, "Discount: %d%% for %d %s.\n\rItems: %d %s per item.\n\r",
              (obj->opt[0] + 1) * 10, PRICE_STR((PRICE_M_DISCOUNT + obj->opt[0]), hedit),
              PRICE_STR(PRICE_M_ITEM, hedit));
          }
        }
        if(IS_SET(obj->flags, PLAN_MOB_HEALER))
        {
          can_up = TRUE;
          if(obj->opt[0] >= PRICE_H_LEVEL_COUNT)
            send_to_char("Level: Maxed.\n\r", ch);
          else
          {
            sprintf(buf, "Level %d for %d %s.\n\r",
              obj->opt[0] + 1, PRICE_STR((PRICE_H_LEVEL + obj->opt[0]), hedit));
            send_to_char(buf, ch);
          }
        }
        if(!can_up)
          send_to_char("You can't upgrade that mob.\n\r", ch);
      }
      else if(IS_SET(obj->type, PLAN_ITEM))
      {
        if(IS_SET(obj->flags, PLAN_ITEM_FURNITURE))
        {
          if(obj->opt[0] >= PRICE_F_REGEN_COUNT)
            send_to_char("Regen: Maxed.\n\r", ch);
          else
          {
            sprintf(buf, "Regen: Upgrade hit and mana regen to %d%% for %d %s.\n\r",
              (obj->opt[0] + 1) * 5, PRICE_STR((PRICE_F_REGEN + obj->opt[0]), hedit));
            send_to_char(buf, ch);
          }
        }
        else
          send_to_char("Only furniture may be upgraded.\n\r", ch);
      }
      else
      {
        bug("Bad object type found for upgrade.", 0);
        send_to_char("Object type not known.\n\r", ch);
      }
      return;/* Checking available upgrades does not set edits_made */
    }/* End arg[0] == '\0'. */
    if(IS_SET(obj->type, PLAN_ROOM))
    {
      if(!str_prefix(arg, "hregen") || !str_prefix(arg, "mregen"))
      {
        if(!IS_SET(obj->flags, (PLAN_ROOM_LAB | PLAN_ROOM_ALTAR)))
        {/* opt 0 is hit regen, 1 is mana regen */
          int max;
          if(hedit)
            max = PRICE_R_REGEN_END;
          else
            max = PRICE_R_REGEN_MID;
          if(!str_prefix(arg, "hregen"))
          {
            if(hedit && !IS_SET(obj->flags, PLAN_ROOM_REGEN))
            {/* Requires verification */
              if(!check_upgrade(ch, obj, 'h'))
                return;
            }
            if(obj->opt[0] >= max)
            {
              send_to_char("That room's hit regen is already maxed.\n\r", ch);
              return;
            }
            if(!pay_hall_cost(ch, GET_PRICE(PRICE_R_REGEN + obj->opt[0], hedit), TRUE, hedit))
              return;/* It handles error messages if told to actually buy */
            obj->opt[0]++;
            sprintf(buf, "Room %s hit regen enhanced to %d%% for %d %s.\n\r",
              obj->label, GET_REGEN(obj->opt[0], 0),
              PRICE_STR((PRICE_R_REGEN + obj->opt[0] - 1), hedit));
            send_to_char(buf, ch);
            SET_BIT(obj->flags, PLAN_ROOM_REGEN);
            load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
            clear_string(&obj->previewer, NULL);
            save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
            return;/* Does not set edits_made because it already saved */

          }
          else
          {
            if(hedit && !IS_SET(obj->flags, PLAN_ROOM_REGEN))
            {/* Requires verification */
              if(!check_upgrade(ch, obj, 'm'))
                return;
            }
            if(obj->opt[1] >= max)
            {
              send_to_char("That room's mana regen is already maxed.\n\r", ch);
              return;
            }
            if(!pay_hall_cost(ch, GET_PRICE(PRICE_R_REGEN + obj->opt[1], hedit), TRUE, hedit))
              return;/* It handles error messages if told to actually buy */
            obj->opt[1]++;
            sprintf(buf, "Room %s mana regen enhanced to %d%% for %d %s.\n\r",
              obj->label, GET_REGEN(obj->opt[1], 0),
              PRICE_STR((PRICE_R_REGEN + obj->opt[1] - 1), hedit));
            send_to_char(buf, ch);
            SET_BIT(obj->flags, PLAN_ROOM_REGEN);
            load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
            clear_string(&obj->previewer, NULL);
            save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
            return;/* Does not set edits_made because it already saved */
          }
        }
        else
          send_to_char("You can't upgrade that room's regen.\n\r", ch);
      }
      else if(hedit)
      {
        if(!str_prefix(arg, "magelab"))
        {
          if(!IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_ALTAR)))
          {
            if(hedit && !IS_SET(obj->flags, PLAN_ROOM_LAB))
            {/* Requires verification */
              if(!check_upgrade(ch, obj, 'l'))
                return;
            }
            if(obj->opt[0] >= PRICE_LAB_COUNT)
            {
              send_to_char("That room's magelab is already maxed.\n\r", ch);
              return;
            }
            if(!pay_hall_cost(ch, GET_PRICE(PRICE_LAB + obj->opt[0], hedit), TRUE, hedit))
              return;/* It handles error messages if told to actually buy */
            obj->opt[0]++;
            sprintf(buf, "Room %s magelab: Upgraded to %d for %d %s.\n\r",
              obj->label, obj->opt[0], PRICE_STR((PRICE_LAB + obj->opt[0] - 1), hedit));
            send_to_char(buf, ch);
            obj->flags |= PLAN_ROOM_LAB;
            load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
            clear_string(&obj->previewer, NULL);
            save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
            return;/* Does not set edits_made because it already saved */
          }
          else
            send_to_char("You can't upgrade that room's magelab.\n\r", ch);
        }
        else if(!str_prefix(arg, "altar"))
        {
          if(!IS_SET(obj->flags, (PLAN_ROOM_REGEN | PLAN_ROOM_LAB)))
          {
            if(hedit && !IS_SET(obj->flags, PLAN_ROOM_ALTAR))
            {/* Requires verification */
              if(!check_upgrade(ch, obj, 'a'))
                return;
            }
            if(obj->opt[0] >= PRICE_ALTAR_COUNT)
            {
              send_to_char("That room's altar is already maxed.\n\r", ch);
              return;
            }
            if(!pay_hall_cost(ch, GET_PRICE((PRICE_ALTAR + obj->opt[0]), hedit), TRUE, hedit))
              return;/* It handles error messages if told to actually buy */
            obj->opt[0]++;
            sprintf(buf, "Room %s altar: Upgraded to %d for %d %s.\n\r",
              obj->label, obj->opt[0], PRICE_STR((PRICE_ALTAR + obj->opt[0] - 1), hedit));
            send_to_char(buf, ch);
            obj->flags |= PLAN_ROOM_ALTAR;
            load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
            clear_string(&obj->previewer, NULL);
            save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
            return;/* Does not set edits_made because it already saved */
          }
          else
            send_to_char("You can't upgrade that room's altar\n\r", ch);
        }
        else
          send_to_char("Valid upgrades: hregen, mregen, magelab, altar.\n\r", ch);
      }
      else
        send_to_char("Valid upgrades: hregen, mregen.\n\r", ch);
    }
    else if((obj->type & PLAN_MOB) == PLAN_MOB)
    {/* Can't get in here if it's not hedit */
      if(!str_prefix(arg, "discount"))
      {
        if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
        {
          if(hedit && obj->opt[0] == 0)
          {/* Requires verification */
            if(!check_upgrade(ch, obj, 'd'))
              return;
          }
          if(obj->opt[0] >= PRICE_M_DISCOUNT_COUNT)
          {
            send_to_char("That mob's discount is already maxed.\n\r", ch);
            return;
          }
          if(!pay_hall_cost(ch, GET_PRICE(PRICE_M_DISCOUNT + obj->opt[0], hedit), TRUE, hedit))
            return;/* It handles error messages if told to actually buy */
          obj->opt[0]++;
          sprintf(buf, "Mob %s discount increased to %d%% for %d %s.\n\r",
            obj->label, obj->opt[0] * 10, PRICE_STR(PRICE_M_DISCOUNT + obj->opt[0] - 1, hedit));
          send_to_char(buf, ch);
          load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
        clear_string(&obj->previewer, NULL);
          save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
          return; /* Does not set edits_made because it already saved */
        }
        else
          send_to_char("You can't upgrade that mob's discount.\n\r", ch);
        return;
      }/* end discount */
      if(!str_prefix(arg, "item"))
      {/* Drink item on a merchant */
      /* Incomplete, has no verification */
        if(IS_SET(obj->flags, PLAN_MOB_MERCHANT))
        {
          PLAN_DATA *drink;
          argument = one_argument(argument, arg);
          /* Who knew making a drink could be so complicated */
          if(arg[0] == '\0' || (drink = find_edit_obj(ch, arg, hedit)) == NULL)
          {
            sprintf(buf, "Drink item %s not found to add.\n\r", arg);
            send_to_char(buf, ch);
            return;
          }
          if(IS_SET(drink->type, PLAN_ITEM))
          {
            send_to_char("That is not an item object.\n\r", ch);
            return;
          }
          if(IS_SET(drink->flags, PLAN_ITEM_DRINK))
          {
            send_to_char("Only drink items can be added to merchants.\n\r", ch);
            return;
          }
          if(IS_SET(drink->type, PLAN_PLACED))
          {
            send_to_char("That drink is already available on a merchant.\n\r", ch);
            return;
          }
          if(!pay_hall_cost(ch, GET_PRICE(PRICE_M_ITEM, hedit), TRUE, hedit))
            return;
          /* Finally, we can make it. */
          drink->loc = obj->plan_index;
          REMOVE_BIT(drink->type, PLAN_PLANNED);
          SET_BIT(drink->type, PLAN_PLACED);
          sprintf(buf, "%s is now sold by %s, added for %d %s.\n\r",
            drink->label, obj->label, PRICE_STR(PRICE_M_ITEM, hedit));
          send_to_char(buf, ch);
          load_plan_obj(obj, TRUE);
          save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
          return; /* Does not set edits_made because it already saved */
        }
        else
          send_to_char("You can't add items for that mob to sell.\n\r", ch);
        return;
      }/* end item */
      if(!str_prefix(arg, "level"))
      {
        if(IS_SET(obj->flags, PLAN_MOB_HEALER))
        {
          if(hedit && obj->opt[0] == 0)
          {/* Requires verification */
            if(!check_upgrade(ch, obj, 'l'))
              return;
          }
          if(obj->opt[0] >= PRICE_H_LEVEL_COUNT)
          {
            send_to_char("That mob's level is already maxed.\n\r", ch);
            return;
          }
          if(!pay_hall_cost(ch, GET_PRICE(PRICE_H_LEVEL + obj->opt[0], hedit), TRUE, hedit))
            return;/* It handles error messages if told to actually buy */
          obj->opt[0]++;
          sprintf(buf, "%s's level increased to %d for %d %s.\n\r",
            obj->label, obj->opt[0] + 40, PRICE_STR(PRICE_H_LEVEL + obj->opt[0] - 1, hedit));
          send_to_char(buf, ch);
          load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
          clear_string(&obj->previewer, NULL);
          save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
          return; /* Does not set edits_made because it already saved */
        }
        else
          send_to_char("You can't upgrade that mob's level.\n\r", ch);
        return;
      }/* end level */
      send_to_char("Valid upgrades: discount, item, level.\n\r", ch);
      return;
    }/* end upgrade mob */
    else if(IS_SET(obj->type, PLAN_ITEM))
    {
      if(!str_prefix(arg, "regen"))
      {
        if(IS_SET(obj->flags, PLAN_ITEM_FURNITURE))
        {
          if(hedit && obj->opt[0] == 0)
          {/* Requires verification */
            if(!check_upgrade(ch, obj, 'r'))
              return;
          }
          if(obj->opt[0] >= PRICE_F_REGEN_COUNT)
            send_to_char("That item's regen is already maxed.\n\r", ch);
          else
          {
            if(!pay_hall_cost(ch, GET_PRICE(PRICE_F_REGEN + obj->opt[0], hedit), TRUE, hedit))
              return;/* It handles error messages if told to actually buy */
            obj->opt[0]++;
            sprintf(buf, "Item %s hit and mana regen enhanced to %d%% for %d %s.\n\r\n\r",
              obj->label, obj->opt[0] * 5, PRICE_STR(PRICE_F_REGEN + obj->opt[0] - 1, hedit));
            send_to_char(buf, ch);
            load_plan_obj(obj, FALSE);/* Just reset the values, no strings changed */
            clear_string(&obj->previewer, NULL);
            save_clan(ch, TRUE, TRUE, hedit);/* Save the clan and the hall in one go */
            return; /* Does not set edits_made because it already saved */
          }
        }
        else
          send_to_char("Only furniture may be upgraded.\n\r", ch);
      }
      else
        send_to_char("Valid upgrades: regen.\n\r", ch);
    }/* End upgrade item */
    else
    {
      bug("Bad object type found for upgrade.", 0);
      send_to_char("Object type not known.\n\r", ch);
    }
    return;
  }/* End upgrade. */
  if(!str_prefix(arg, "delete"))
  {
    PLAN_DATA *prev = NULL;
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit delete <label>\n\r", ch);
      else
        send_to_char("Usage: pedit delete <label>\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      sprintf(buf, "%s not found to delete.\n\r", arg);
      send_to_char(buf, ch);
      return;
    }
    if(ch->pcdata->edit_obj != obj)
    {
      send_to_char("To delete, object must be the one you are currently editing.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_PLACED))
    {/* How did this happen? */
      send_to_char("You may not delete a placed object, remove it first.\n\r", ch);
      bug("Player editing a placed object, attempted delete.", 0);
      return;
    }
    /* Delete is go */
    /* Unlink and release the memory - planned objects never point at each other */
    if(hedit)
    {
      if(obj == ch->pcdata->clan_info->clan->planned)
        ch->pcdata->clan_info->clan->planned = ch->pcdata->clan_info->clan->planned->next;
      else
        prev = ch->pcdata->clan_info->clan->planned;
    }
    else
    {
      if(obj == ch->pcdata->clan_info->pers_plan)
        ch->pcdata->clan_info->pers_plan = ch->pcdata->clan_info->pers_plan->next;
      else
        prev = ch->pcdata->clan_info->pers_plan;
    }
    if(prev)
    {/* Not been removed yet, was only removed if it was the head object */
      while(prev->next && prev->next != obj)
        prev = prev->next;
      if(prev->next)
      {
        prev->next = prev->next->next;
      }
      else
      {
        bug("Object not found to delete.", 0);
      }
    }
    sprintf(buf, "%s deleted.\n\r", obj->label);
    send_to_char(buf, ch);
    ch->pcdata->edit_obj = NULL;
    free_plan(obj);/* Release it */
    ch->pcdata->edits_made = TRUE;
    return;
  }/* End delete */
  if(!str_prefix(arg, "stop"))
  {
    edit_stop(ch);
    return;
  }/* End stop */
  if(!str_prefix(arg, "place"))
  {/* Find the object, make sure it's marked preview, place it */
  /* Comment out "replace" for now, also should be able to copy its code here */
    PLAN_DATA *ch_room;
    if(!check_can_edit(ch, CLAN_CAN_PLACE, hedit))
    {
      send_to_char("You do not have permissions to place.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit place <label>\n\r", ch);
      else
        send_to_char("Usage: pedit place <label>\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      sprintf(buf, "%s not found to place.\n\r", arg);
      send_to_char(buf, ch);
      return;
    }
    if(!IS_SET(obj->type, PLAN_PREVIEWED) || !obj->to_place)
    {
      send_to_char("You may only place previewed objects.\n\r", ch);
      return;
    }
    if(hedit && !str_cmp(obj->previewer, ch->name) && ch->pcdata->clan_info->clan->leaders > 1)
    {
      send_to_char("A different leader must place an item you have previewed.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_EXIT))
    {
      send_to_char("You may not place exits currently.\n\r", ch);
      return;
    }
    ch_room = find_char_room_obj(ch, hedit);
    if((IS_SET(obj->type, PLAN_ROOM) && obj != ch_room) ||
    (!IS_SET(obj->type, PLAN_ROOM) && obj->loc != ch_room->plan_index))
    {
      send_to_char("You must be in the same room as the object starts in.\n\r", ch);
      return;
    }
    if(!IS_SET(obj->type, PLAN_ROOM))
    {
      if(!IS_SET(ch_room->type, PLAN_PLACED))
      {
        send_to_char("You may not place mobs or items in previewed rooms, place the room first.\n\r", ch);
        return;
      }
    }
    else
    {
      for(i = 0; i < 6; i++)
      {
        if(obj->exits[i].outside || (obj->exits[i].link && IS_SET(obj->exits[i].link->type, PLAN_PLACED)))
          break;
      }
      if(i == 6)
      {
        send_to_char("A placed room must be adjacent to place this one.\n\r", ch);
        return;
      }
    }
    place_hall_obj(ch, obj, hedit);
    sprintf(buf, "%s has been made a permanent part of your hall.\n\r", obj->label);
    send_to_char(buf, ch);
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }
  if(!str_prefix(arg, "check"))
  {
    if(!check_legal_hall(ch->in_room))
      send_to_char("You may not place your hall in this room.\n\r", ch);
    else
      send_to_char("You may place your hall in this room.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "preview"))
  {
    int dir;
    PLAN_DATA *room;
    if(!check_can_edit(ch, CLAN_CAN_REMOVE, hedit))
    {/* remove is used for both preview and remove -- non-final decisions */
      send_to_char("You do not have permissions to preview.\n\r", ch);
      return;
    }
    if(!ch->pcdata->clan_info->clan->type)
    {
      send_to_char("Your clan must have a type to place a hall.\n\r", ch);
      return;
    }
    obj = NULL;
    dir = -1;
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(ch->pcdata->edit_obj)
      {
        if(IS_SET(ch->pcdata->edit_obj->type, PLAN_ROOM))
        {
          send_to_char("Usage: hedit preview <direction> to preview the room you are editing.\n\r", ch);
          return;
        }
        obj = ch->pcdata->edit_obj;
      }
      else
      {
        send_to_char("You must include the object to preview if you are not editing one.\n\r", ch);
        if(hedit)
          send_to_char("Usage: hedit preview <label> <direction if room>\n\r", ch);
        else
          send_to_char("Usage: pedit preview <label> <direction if room>\n\r", ch);
        return;
      }
      /* SPECIAL CASE - Let it fall out of here */
    }
    if(!obj && (obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      if(ch->pcdata->edit_obj && IS_SET(ch->pcdata->edit_obj->type, PLAN_ROOM) &&
        (dir = get_arg_dir(arg)) >= 0)/* Short circuit - Note that dir is being set here */
        obj = ch->pcdata->edit_obj;
      else
      {
        sprintf(buf, "%s not found to preview.\n\r", arg);
        send_to_char(buf, ch);
        return;
      }
    }
    if(IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)))
    {
      send_to_char("You may only preview planned objects.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_EXIT))
    {
      if(hedit)
        send_to_char("Use 'hedit replace' or 'hedit link' to place an exit.\n\r", ch);
      else
        send_to_char("Use 'pedit replace' or 'pedit link' to place an exit.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_ITEM) && IS_SET(obj->flags, PLAN_ITEM_DRINK))
    {
      send_to_char("Use 'hedit upgrade' to place a drink item.\n\r", ch);
      return;
    }
    if(obj->editing && ch->pcdata->edit_obj != obj)
    {
      send_to_char("Someone else is editing that object, you may not preview it.\n\r", ch);
      return;
    }
    room = find_char_room_obj(ch, hedit);
    if(IS_SET(obj->type, PLAN_ROOM))
    {
      PLAN_DATA *start;
      if(obj->name == NULL || (obj->desc == NULL && ch->pcdata->edit_len == 0))
      {
        send_to_char("Name and desc must be set to preview a room.\n\r", ch);
        return;
      }
      if(dir < 0)
      {/* If dir is already set, they shortcut this one */
        argument = one_argument(argument, arg);
        dir = get_arg_dir(arg);
      }
      if(dir < 0)
      {
        send_to_char("Invalid direction. Valid: north, south, east, west, up, down.\n\r", ch);
        return;
      }
      if(room)
      {
        if(room->exits[dir].link || room->exits[dir].outside)
        {
          send_to_char("There is already an exit in that direction.\n\r", ch);
          return;
        }
      }
      else
      {/* Verify no exit, then verify no other outside link */
        if(!check_legal_hall(ch->in_room))
        {
          if(IS_IMMORTAL(ch))
            send_to_char("This is not a location a mort could set.\n\r", ch);
          else
          {
            send_to_char("This room is not legal to attach a hall to.\n\r", ch);
            return;
          }
        }

        if(ch->in_room->exit[dir] != NULL)
        {
          send_to_char("There is already an exit in that direction.\n\r", ch);
          return;
        }
        start = START_OBJ(ch, hedit);
        /* Find the entry room */
        for(; start != NULL; start = start->next)
        {
          if(IS_SET(start->type, PLAN_ROOM) && IS_SET(start->type, (PLAN_PLACED | PLAN_PLANNED)) && start != obj)
          {
            for(i = 0; i < 6; i++)
            {
              if(start->exits[i].outside != NULL)
                break;
            }
          }
        }
        if(start)
        {
          send_to_char("You can't make a second outside link to your hall.\n\r", ch);
          return;
        }
      }
      if(!pay_hall_cost(ch, obj->cost, TRUE, hedit))
        return;
      if(room)
      {
        room->exits[dir].link = obj;
        obj->exits[rev_dir[dir]].link = room;
      }
      else
      {
        obj->exits[rev_dir[dir]].outside = ch->in_room;
        if(hedit && !ch->pcdata->clan_info->clan->hall)
        {
          ch->pcdata->clan_info->clan->hall = obj;
          ch->pcdata->clan_info->clan->hall_index = obj->plan_index;
        }
      }
      if(ch->pcdata->edit_obj == obj)
      {
        end_long_edit(ch, &ch->pcdata->edit_obj->desc);
        ch->pcdata->edit_obj->editing = FALSE;
        ch->pcdata->edit_obj = NULL;
      }
      preview_hall_obj(ch, obj, hedit);
      set_room_exits(obj);
      sprintf(buf, "%s has been previewed linked to this room for %d %s.\n\r",
        obj->label, COST_STR(obj->cost, hedit));
      send_to_char(buf, ch);
      clear_string(&obj->previewer, ch->name);
      save_clan(ch, TRUE, TRUE, hedit);
      return;
    }
    if(room == NULL)
    {
      send_to_char("You may not preview an object in this room.\n\r", ch);
      return;
    }
    if(obj->name == NULL || (obj->long_d == NULL) || obj->short_d == NULL ||
      (IS_SET(obj->type, PLAN_MOB) && obj->desc == NULL && ch->pcdata->edit_len == 0))
    {
      if(IS_SET(obj->type, PLAN_MOB))
        send_to_char("Name, desc, short and long must be set to preview a mob.\n\r", ch);
      else
        send_to_char("Name, short and long must be set to preview an item.\n\r", ch);
      return;
    }
    if((IS_SET(obj->type, PLAN_MOB) && !IS_SET(obj->flags, (PLAN_MOB_MERCHANT | PLAN_MOB_HEALER))) ||
      (IS_SET(obj->type, PLAN_ITEM) && !IS_SET(obj->flags, PLAN_MASK_OBJ)))
    {
      send_to_char("You must have a type set to preview that object.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_ITEM))
    {/* Only one pit allowed per room, check for that. */
      if(IS_SET(obj->flags, PLAN_ITEM_PIT))
      {
        PLAN_DATA *search = START_OBJ(ch, hedit);
        for(; search != NULL; search = search->next)
        {
          if(IS_SET(search->type, PLAN_ITEM) && IS_SET(search->type, (PLAN_PLACED | PLAN_PREVIEWED)) &&
            IS_SET(search->flags, PLAN_ITEM_PIT) && search->loc == room->plan_index)
          {
            send_to_char("You may only have one pit in a room.\n\r", ch);
            return;
          }
        }
      }
    }
    else
    {/* Mob, make sure there are none already placed in this room */
      PLAN_DATA *search = START_OBJ(ch, hedit);
      for(; search != NULL; search = search->next)
      {
        if(IS_SET(search->type, PLAN_MOB) && IS_SET(search->type, (PLAN_PLACED | PLAN_PREVIEWED)) &&
          search->loc == room->plan_index)
        {
          send_to_char("You may only have one mob in a room.\n\r", ch);
          return;
        }
      }
    }
    if(!pay_hall_cost(ch, obj->cost, TRUE, hedit))
      return;
    if(ch->pcdata->edit_obj == obj)
    {
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
      ch->pcdata->edit_obj->editing = FALSE;
      ch->pcdata->edit_obj = NULL;
    }
    obj->loc = room->plan_index;
    preview_hall_obj(ch, obj, hedit);
    sprintf(buf, "%s has been previewed in this room for %d %s.\n\r",
      obj->label, COST_STR(obj->cost, hedit));
    send_to_char(buf, ch);
    if(IS_SET(obj->type, PLAN_ITEM) && IS_SET(obj->flags, PLAN_ITEM_PORTAL))
      update_room_sign(ch->pcdata->clan_info->clan, room->to_place);
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }/* end place */
  if(!str_prefix(arg, "remove"))
  {
    int amount;
    PLAN_DATA *room;
    if(!check_can_edit(ch, CLAN_CAN_REMOVE, hedit))
    {
      send_to_char("You do not have permissions to remove.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit remove <label> <(room only) direction>\n\r", ch);
      else
        send_to_char("Usage: pedit remove <label> <(room only) direction>\n\r", ch);
      return;
    }
    if(!str_cmp(arg, "exit"))
    {
      send_to_char("Sorry, exits are not implemented yet.\n\r", ch);
      return;
      if(hedit)
        send_to_char("Use 'hedit replace' or 'hedit unlink' to remove an exit.\n\r", ch);
      else
        send_to_char("Use 'pedit replace' or 'pedit unlink' to remove an exit.\n\r", ch);
      return;
    }
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL)
    {
      sprintf(buf, "%s not found to remove.\n\r", arg);
      send_to_char(buf, ch);
      return;
    }
    if(!IS_SET(obj->type, PLAN_PREVIEWED) || !obj->to_place)
    {
      send_to_char("You may only remove previewed objects.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, PLAN_EXIT))
    {/* Duplicate from above, can't really be helped */
      if(hedit)
        send_to_char("Use 'hedit replace' or 'hedit unlink' to remove an exit.\n\r", ch);
      else
        send_to_char("Use 'hedit replace' or 'hedit unlink' to remove an exit.\n\r", ch);
      return;
    }
    room = find_char_room_obj(ch, hedit);
    if(IS_SET(obj->type, PLAN_ROOM))
    {
      PLAN_DATA *start;
      int dir = -1;
      argument = one_argument(argument, arg);
      dir = get_arg_dir(arg);
      if(dir < 0)
      {
        send_to_char("Invalid direction. Valid: north, south, east, west, up, down.\n\r", ch);
        return;
      }

      if(room == obj)
      {
        send_to_char("You can't remove the room you're in.\n\r", ch);
        return;
      }

      if(!room || room->exits[dir].link == NULL || room->exits[dir].link != obj)
      {
        send_to_char("The room you are removing must be linked to your current room.\n\r", ch);
        return;
      }

      if(room->exits[dir].link->exits[rev_dir[dir]].link != room)
      {
        bug("Unmatched exit in player hall.", 0);
        send_to_char("Unmatched exits may not be removed.\n\r", ch);
        return;
      }

      /* Ensure nobody is in the target room */
/* Not needed - they'll all move out */
/*      if(obj->to_place && ((ROOM_INDEX_DATA*)obj->to_place)->people)
      {
        send_to_char("There is a player or mob in that room, you may not remove it.\n\r", ch);
        return;
      }*/
      start = START_OBJ(ch, hedit);
      /* Find the entry room */
      for(; start != NULL; start = start->next)
      {
        if(IS_SET(start->type, (PLAN_ROOM | PLAN_PLACED)) == (PLAN_ROOM | PLAN_PLACED) && start != obj)
        {
          for(i = 0; i < 6; i++)
          {
            if(start->exits[i].outside != NULL)
              break;/* It's a non-target room with an outside bind, we can start here */
          }
          if(i != 6)
            break;/* It found an outside exit */
        }
      }
      if(!start)
      {
        send_to_char("You can't remove the only outside link for your hall.\n\r", ch);
        return;
      }
      SET_BIT(obj->flags, PLAN_ROOM_MARKED);/* Prevents it being entered by the search */
      search_linked_rooms(start);
      /* After complete, scan through and verify all placed rooms are marked */
      start = START_OBJ(ch, hedit);
      for(; start != NULL; start = start->next)
      {
        if(IS_SET(start->type, (PLAN_ROOM | PLAN_PLACED)) == (PLAN_ROOM | PLAN_PLACED))
        {
          if(IS_SET(start->flags, PLAN_ROOM_MARKED))
            REMOVE_BIT(start->flags, PLAN_ROOM_MARKED);
          else
            obj = NULL;/* Mark failure */
        }
      }
      if(obj == NULL)
      {
        send_to_char("Removing that room would make some rooms unreachable.\n\r", ch);
        return;
      }
      /* All checks complete, remove this room */
      amount = remove_hall_obj(ch, obj, hedit);
      sprintf(buf, "%s removed, %d %s refunded.\n\r",
        obj->label, COST_STR(amount, hedit));
      pay_hall_cost(ch, amount * -1, TRUE, hedit);
      send_to_char(buf, ch);
      save_clan(ch, TRUE, TRUE, hedit);
      return;
    }
    if(!room || obj->loc != room->plan_index)
    {
      send_to_char("You must be in the same room as the object you wish to remove.\n\r", ch);
      return;
    }
    amount = remove_hall_obj(ch, obj, hedit);
    if(IS_SET(obj->type, PLAN_ITEM) && IS_SET(obj->flags, PLAN_ITEM_PORTAL))
      update_room_sign(ch->pcdata->clan_info->clan, ch->in_room);
    sprintf(buf, "%s removed from the room, %d %s refunded.\n\r",
      obj->label, COST_STR(amount, hedit));
    pay_hall_cost(ch, amount * -1, TRUE, hedit);
    send_to_char(buf, ch);
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }/* end remove */
  if(!str_prefix(arg, "link"))
  {
    PLAN_DATA *room;
    int dir;
    if(!check_can_edit(ch, CLAN_CAN_PLACE, hedit))
    {
      send_to_char("You do not have permissions to link.\n\r", ch);
      return;
    }
    room = find_char_room_obj(ch, hedit);
    if(!room)
    {
      send_to_char("You can not link from this room.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if((obj = find_edit_obj(ch, arg, hedit)) == NULL || !IS_SET(obj->type, PLAN_ROOM) ||
      !IS_SET(obj->type, PLAN_PLACED))
    {
      send_to_char("You can only link to placed rooms.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    dir = get_arg_dir(arg);
    if(dir < 0)
    {
      send_to_char("Invalid direction. Valid: north, south, east, west, up, down.\n\r", ch);
      return;
    }
    if(obj->exits[dir].link != NULL || obj->exits[dir].outside != NULL)
    {
      send_to_char("There is already an exit in that direction from your current room.\n\r", ch);
      if(hedit)
        send_to_char("Use 'hedit replace' if you want to customize an exit.\n\r", ch);
      else
        send_to_char("Use 'pedit replace' if you want to customize an exit.\n\r", ch);
      return;
    }
    if(room->exits[rev_dir[dir]].link != NULL || obj->exits[dir].outside != NULL)
    {
      send_to_char("There is already an exit in that direction from your target room.\n\r", ch);
      return;
    }
    if(!pay_hall_cost(ch, GET_PRICE(PRICE_LINK, hedit), TRUE, hedit))
      return;
    obj->exits[rev_dir[dir]].link = room;
    room->exits[dir].link = obj;
    set_room_exits(obj);
    sprintf(buf, "%s has been linked to %s for %d %s.\n\r",
      obj->label, room->label, PRICE_STR(PRICE_LINK, hedit));
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }/* end link */
  if(!str_prefix(arg, "unlink"))
  {
    int amount, dir;
    PLAN_DATA *room;
    PLAN_DATA *start;
    if(!check_can_edit(ch, CLAN_CAN_REMOVE, hedit))
    {
      send_to_char("You do not have permissions to unlink.\n\r", ch);
      return;
    }
    room = find_char_room_obj(ch, hedit);
    if(room == NULL)
    {
      send_to_char("You can not unlink exits from this room.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);

    dir = get_arg_dir(arg);
    if(dir < 0)
    {
      send_to_char("Invalid direction. Valid: north, south, east, west, up, down.\n\r", ch);
      return;
    }

    if(room->exits[dir].link == NULL)
    {
      send_to_char("There is no exit there for you to unlink.\n\r", ch);
      return;
    }

    if(room->exits[dir].link->exits[rev_dir[dir]].link != room)
    {
      bug("Unmatched exit in player hall.", 0);
      send_to_char("Unmatched exits may not be unlinked.\n\r", ch);
      return;
    }

    for(i = 0; i < 6; i++)
    {
      if(i == dir)
        continue;
      if(room->exits[i].link != NULL)
        break;/* It has at least one other hall-facing link */
    }
    if(i == 6)
    {
      send_to_char("The room you are in only has one internal link, you may not remove it.\n\r", ch);
      return;
    }

    obj = room->exits[i].link;

    for(i = 0; i < 6; i++)
    {
      if(i == rev_dir[dir])
        continue;
      if(obj->exits[i].link != NULL)
        break;/* It has at least one other hall-facing link */
    }
    if(i == 6)
    {
      send_to_char("You may not remove that exit, it is the connected room's only internal link.\n\r", ch);
      return;
    }

    /* Will be re-bound if this fails */
    room->exits[dir].link = NULL;
    obj->exits[rev_dir[dir]].link = NULL;

    start = START_OBJ(ch, hedit);
    /* Find the entry room */
    for(; start != NULL; start = start->next)
    {
      if(IS_SET(start->type, (PLAN_ROOM | PLAN_PLACED)))
      {/* Any room will work this time */
        break;
      }
    }
    if(!start)
    {
      bug("No placed rooms located for unlink.", 0);
      send_to_char("No placed rooms located.\n\r", ch);
      return;
    }
    search_linked_rooms(start);
    /* After complete, scan through and verify all placed rooms are marked */
    start = START_OBJ(ch, hedit);
    for(; start != NULL; start = start->next)
    {
      if(IS_SET(start->type, (PLAN_ROOM | PLAN_PLACED)))
      {
        if(IS_SET(start->flags, PLAN_ROOM_MARKED))
          REMOVE_BIT(start->flags, PLAN_ROOM_MARKED);
        else
          obj->exits[dir].link = room;/* Mark failure */
      }
    }
    if(obj->exits[dir].link)
    {
      /* Restore the links */
      room->exits[rev_dir[dir]].link = obj;
      send_to_char("Removing that link would make some rooms unreachable.\n\r", ch);
      return;
    }

    if(obj->exits[dir].exit)
    {/* Exit to remove, too */
      amount = remove_hall_obj(ch, obj->exits[dir].exit, hedit);
      sprintf(buf, "Custom exit %s removed, refunded %d %s.\n\r",
        obj->exits[dir].exit->label, COST_STR(amount, hedit));
      send_to_char(buf, ch);
    }
    set_room_exits(obj);
    amount = get_refund_amount(ch, GET_PRICE(PRICE_LINK, hedit), hedit);
    sprintf(buf, "Link removed, refunded %d %s.\n\r", COST_STR(amount, hedit));
    send_to_char(buf, ch);
    pay_hall_cost(ch, amount * -1, TRUE, hedit);
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }/* end link */
  /*if(!str_prefix(arg, "replace"))
  {
    int amount = 0;
    PLAN_DATA *room;
    if(hedit && (!check_can_edit(ch, CLAN_CAN_PLACE, hedit) ||
      !check_can_edit(ch, CLAN_CAN_REMOVE, hedit)))
    {
      send_to_char("You do not have permissions to replace.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(arg[0] == '\0')
    {
      if(hedit)
        send_to_char("Usage: hedit replace <label> <(exit only) direction>\n\r", ch);
      else
        send_to_char("Usage: pedit replace <label> <(exit only) direction>\n\r", ch);
      return;
    }
    room = find_char_room_obj(ch, hedit);
    if(room == NULL)
    {
      send_to_char("You can't replace this room or its exits.\n\r", ch);
      return;
    }
    obj = NULL;
    if(!str_cmp(arg, "exit") || ((obj = find_edit_obj(ch, arg, hedit)) != NULL &&
      IS_SET(obj->type, PLAN_EXIT | PLAN_PLANNED) == (PLAN_EXIT | PLAN_PLANNED)))
    {// Check if there is a special exit bound in the provided direction
      send_to_char("Sorry, exits are not implemented yet.\n\r", ch);
      return;
      int dir;
      argument = one_argument(argument, arg);
      dir = get_arg_dir(arg);
      if(dir < 0)
      {
        send_to_char("Invalid direction. Valid: north, south, east, west, up, down.\n\r", ch);
        return;
      }
      if(!room->exits[dir].link)
      {
        send_to_char("There is no exit in that direction.\n\r", ch);
        return;
      }
      if(room->exits[dir].link->exits[rev_dir[dir]].link != room ||
        room->exits[dir].link->exits[rev_dir[dir]].exit != room->exits[dir].exit)
        {
          bug("Bad exit link for replace", 0);
          send_to_char("Unmatched exit or link, you can't replace it.\n\r", ch);
          return;
        }
      if(obj == NULL)
      {// Revert to default
        if(room->exits[dir].exit == NULL)
        {
          send_to_char("That exit is already a default exit.\n\r", ch);
          return;
        }
        // remove_hall_obj will handle removing the linked exit and the costs
        amount = remove_hall_obj(ch, room->exits[dir].exit, hedit);
        sprintf(buf, "Custom exit %s removed, refunded %d %s.\n\r",
          room->exits[dir].exit->label, COST_STR(amount, hedit));
        pay_hall_cost(ch, amount * -1, TRUE, hedit);
      }
      else
      {
        if(room->exits[dir].exit)
          amount = get_refund_amount(ch, room->exits[dir].exit->cost, hedit);
        if(!pay_hall_cost(ch, obj->cost - amount, TRUE, hedit))
          return;
        if(room->exits[dir].exit)
        {
          sprintf(buf, "Custom exit %s removed, refunded %d %s.\n\r",
            room->exits[dir].exit->label, COST_STR(amount, hedit));
          REMOVE_BIT(room->exits[dir].exit->type, PLAN_PLACED);
          SET_BIT(room->exits[dir].exit->type, PLAN_PLANNED);
          // Cost was already paid with discount applied
        }
        room->exits[dir].exit = obj;
        room->exits[dir].link->exits[rev_dir[dir]].exit = obj;
        if(ch->pcdata->edit_obj == obj)
        {
          end_long_edit(ch, &ch->pcdata->edit_obj->desc);
          ch->pcdata->edit_obj->editing = FALSE;
          ch->pcdata->edit_obj = NULL;
        }
        REMOVE_BIT(obj->type, PLAN_PLANNED);
        SET_BIT(obj->type, PLAN_PLACED);
        load_plan_obj(obj, TRUE);
        set_room_exits(room);
        sprintf(buf, "Custom exit %s placed for %d %s.\n\r",
          obj->label, COST_STR(obj->cost, hedit));
        send_to_char(buf, ch);
      }
      save_clan(ch, TRUE, TRUE, hedit);
      return;
    }
    if(obj == NULL)
    {
      send_to_char("Object not found to replace with.\n\r", ch);
      return;
    }
    if(IS_SET(obj->type, (PLAN_MOB | PLAN_ITEM | PLAN_PLACED)))
    {
      send_to_char("You may only replace using a planned room.\n\r", ch);
      return;
    }
    // A placed exit would already be caught, this must be a placed room
    // Verify exits
    for(i = 0; i < 6; i++)
    {
      if(obj->exits[i].link != NULL)
      {
        if(obj->exits[i].link->exits[rev_dir[i]].link != obj)
        {
          send_to_char("Exit mismatch, can't replace this room.\n\r", ch);
          bug("NULL Exit mismatch in room replace.", 0);
          return;
        }
      }
    }
    // Verify they can afford it and charge them
    amount = get_refund_amount(ch, room->cost, hedit);
    if(!pay_hall_cost(ch, obj->cost - amount, TRUE, hedit))
      return;
    // They've paid
    if(ch->pcdata->edit_obj == obj)
    {
      end_long_edit(ch, &ch->pcdata->edit_obj->desc);
      ch->pcdata->edit_obj->editing = FALSE;
      ch->pcdata->edit_obj = NULL;
    }
    swap_rooms(ch, room, obj, hedit);
    sprintf(buf, "Room %s placed for %d %s.\n\r",
      obj->label, COST_STR(obj->cost - amount, hedit));
    save_clan(ch, TRUE, TRUE, hedit);
    return;
  }*//* end replace */
  /* End general commands */

  /* If they're not in edit mode, check if any of these are edit specific
   * commands, to give a more meaningful error message */
  if(!ch->pcdata->edit_obj)
  {/* This duplicates checks, but better than the five extra lines per check
    * if this were inserted with each edit check */
    bool bFound = TRUE;
    if(!str_prefix(arg, "label"))
      send_to_char("label is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "name"))
      send_to_char("name is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "desc"))
      send_to_char("desc is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "hregen"))
      send_to_char("hregen is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "mregen"))
      send_to_char("mregen is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "magelab"))
      send_to_char("magelab is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "altar"))
      send_to_char("altar is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "outdoor"))
      send_to_char("outdoor is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "indoor"))
      send_to_char("indoor is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "dark"))
      send_to_char("dark is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "light"))
      send_to_char("light is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "short"))
      send_to_char("short is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "long"))
      send_to_char("long is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "type"))
      send_to_char("type is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "discount"))
      send_to_char("discount is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "level"))
      send_to_char("level is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "race"))
      send_to_char("race is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "alignment"))
      send_to_char("align is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "gender"))
      send_to_char("gender is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "sex"))
      send_to_char("sex is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "target"))
      send_to_char("target is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "regen"))
      send_to_char("regen is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "liquid"))
      send_to_char("liquid is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "hidden"))
      send_to_char("hidden is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "door"))
      send_to_char("door is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "closed"))
      send_to_char("closed is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "key"))
      send_to_char("key is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "locked"))
      send_to_char("locked is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "nopass"))
      send_to_char("nopass is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else if(!str_prefix(arg, "pickproof"))
      send_to_char("pickproof is an edit mode command, you must be editing an object to use it.\n\r", ch);
    else
      bFound = FALSE;
    if(bFound)
      return;/* Gave them their error message */
  }
  /* Unknown command */
  if(hedit)
    send_to_char("Unknown hedit command.  Please see help hedit for details.\n\r", ch);
  else
    send_to_char("Unknown pedit command.  Please see help pedit for details.\n\r", ch);
}

void do_movehall(CHAR_DATA *ch, char *argument )
{
  CLAN_DATA *clan;
  int i, dir;
  char arg[256], buf[256];
  if(!IS_IMMORTAL(ch) || ch->level < 57)
  {
    send_to_char("You don't have the power to move halls.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(arg[0] == 0)
  {
    send_to_char("Usage: movehall <clan> <dir> or movehall check\n\r", ch);
    return;
  }
  if(!check_legal_hall(ch->in_room))
  {
    send_to_char("This room is not legal to attach a hall to.\n\r", ch);
    return;
  }
  if(!str_cmp(arg, "check"))
  {
    send_to_char("This is a legal room for a clan hall.\n\r", ch);
    return;
  }
  clan = clan_lookup(arg);
  if(clan == NULL)
  {
    send_to_char("That is not a valid clan.\n\r", ch);
    return;
  }
  if(!clan->hall)
  {
    send_to_char("That clan has no hall to move.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  dir = get_arg_dir(arg);
  if(dir < 0)
  {
    send_to_char("Usage: movehall <clan> <dir> or movehall check\n\r", ch);
    return;
  }
  if(ch->in_room->exit[dir] != NULL)
  {
    send_to_char("There is already an exit in that direction.\n\r", ch);
    return;
  }
  if(clan->hall->exits[rev_dir[dir]].link)
  {
    send_to_char("The hall has an internal link in that direction, it can't connect that way.\n\r", ch);
    return;
  }
  /* Good to go.  Break the old link */
  for(i = 0; i < 6; i++)
  {
    if(clan->hall->exits[i].outside)
    {
      modify_room_marker(clan, clan->hall->exits[i].outside, -1, FALSE);
      clan->hall->exits[i].outside = NULL;
    }
  }
  set_room_exits(clan->hall);
  clan->hall->exits[rev_dir[dir]].outside = ch->in_room;
  set_room_exits(clan->hall);
  modify_room_marker(clan, clan->hall->exits[rev_dir[dir]].outside, dir, TRUE);
  send_to_char("Done.\n\r", ch);
  save_hall(clan->name, clan->hall, TRUE);
}

/* NOT COMPLETE YET */
void do_hedit(CHAR_DATA *ch, char *argument)
{
  //if(ch->in_room == NULL)
  //  return;
  if(IS_NPC(ch))
  {
    send_to_char("NPCs can not edit halls.\n\r", ch);
    return;
  }

  if(ch->pnote)
  {
    send_to_char("You may not edit your hall while writing a note.\n\r", ch);
    return;
  }

  if(!ch->pcdata->clan_info || ch->pcdata->clan_info->clan->default_clan == TRUE)
  {
    send_to_char("You must be in a clan to use hedit.\n\r", ch);
    return;
  }

  if(ch->alert)
  {
    send_to_char("Wait until you're a bit calmer before editing your hall.\n\r", ch);
    return;
  }

  /* Do they have permissions? */
  if(!check_can_edit(ch, CLAN_CAN_BUILD, TRUE))
  {
    send_to_char("You are not allowed to edit your clan's hall.\n\r", ch);
    return;
  }

  /* They're not editing something else, right? */
  if(IS_SET(ch->pcdata->edit_flags, EDITMODE_PERSONAL))
  {
    send_to_char("End your personal room editing first.\n\r", ch);
    return;
  }
  if(ch->in_room->vnum < 0 && (abs(ch->in_room->vnum) < ch->pcdata->clan_info->clan->vnum_min || abs(ch->in_room->vnum) > ch->pcdata->clan_info->clan->vnum_max))
  {
    send_to_char("You may not edit in someone else's hall.\n\r", ch);
    return;
  }

  player_edit(ch, argument, TRUE);
}

/* NOT COMPLETE YET */
void do_pedit(CHAR_DATA *ch, char *argument)
{
  send_to_char("Sorry, personal editing is not implemented yet.\n\r", ch);
  return;
  if(ch->in_room == NULL)
    return;
  if(IS_NPC(ch))
  {
    send_to_char("NPCs may not edit personal rooms.\n\r", ch);
    return;
  }

  if(ch->pnote)
  {
    send_to_char("You may not edit personal rooms while writing a note.\n\r", ch);
    return;
  }

  /* Location check here */
  if(!check_can_edit(ch, 0, FALSE))
  {
    send_to_char("You are not in your personal rooms, you may not use pedit here.\n\r", ch);
    return;
  }

  /* They're not editing something else, right? */
  if(IS_SET(ch->pcdata->edit_flags, EDITMODE_HALL))
  {
    send_to_char("End your hall editing first.\n\r", ch);
    return;
  }

  player_edit(ch, argument, FALSE);
}

