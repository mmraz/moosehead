/*  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
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


static char rcsid[] = "$Id: act_obj.c,v 1.414 2005/02/08 20:33:53 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <gc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"
#include "gladiator.h"
#include "recycle.h"

/* command procedures needed */
DECLARE_DO_FUN(do_split   );
DECLARE_DO_FUN(do_yell    );
DECLARE_DO_FUN(do_say   );
DECLARE_DO_FUN(do_help   );
DECLARE_SPELL_FUN(  spell_null    );
/* Imported */
CLAN_DATA* clan_lookup  args( ( const char *name ) );

void do_get( CHAR_DATA *ch, char *argument );
/*
 * Local functions.
 */
#define CD CHAR_DATA
#define OD OBJ_DATA
bool  remove_obj  args( (CHAR_DATA *ch, int iWear, bool fReplace ) );
void  wear_obj  args( (CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );
CD *  find_keeper args( (CHAR_DATA *ch ) );
int get_cost  args( (CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy ) );
void  obj_to_keeper args( (OBJ_DATA *obj, CHAR_DATA *ch ) );
OD *  get_obj_keeper  args( (CHAR_DATA *ch,CHAR_DATA *keeper,char *argument));

#undef OD
#undef  CD

#define COST_ACCURACY   100
#define COST_SPEED      300
#define COST_MAGIC_RESISTANCE   250
#define COST_VISION     50
#define COST_RESTORATION        25
#define COST_MOLOTOV    25

bool check_affect_match(OBJ_DATA *obj, AFFECT_DATA *check)
{
  AFFECT_DATA *paf;
  for(paf = obj->pIndexData->affected; paf != NULL; paf = paf->next)
  {
    if(paf->location == check->location)
    {
      if(paf->modifier == check->modifier)
        return TRUE;
      return FALSE;
    }
  }
  return FALSE;
}

int set_rarity(OBJ_DATA *obj)
{
  AFFECT_DATA *paf;

  switch(obj->pIndexData->vnum)
  {
    case OBJ_VNUM_FEATHER:
    case OBJ_VNUM_FRACTAL: return RARITY_RARE; 
    default: break;
  }

  if(IS_SET(obj->extra_flags, ITEM_IMM_LOAD))
    return obj->rarity = RARITY_IMPOSSIBLE;

  obj->rarity = RARITY_STOCK;

  if(obj->enchanted)
  {
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
      if(paf->duration >= 0)
        continue; /* Temporary effects are okay and don't need checking */
      if(!check_affect_match(obj, paf))
      {/* No match, let's see what it is */
        if(obj->item_type == ITEM_WEAPON)
        {
          if(paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)
          {
            if(paf->modifier <= 6)
              obj->rarity = UMAX(obj->rarity, RARITY_COMMON);
            else if(paf->modifier <= 12)
              obj->rarity = UMAX(obj->rarity, RARITY_UNCOMMON);
            else
              obj->rarity = UMAX(obj->rarity, RARITY_RARE);
            continue;
          }
          if(paf->where == TO_WEAPON && paf->bitvector == WEAPON_HOLY)
            continue;
          if(paf->where == TO_WEAPON && paf->bitvector == WEAPON_SHARP)
            continue;
          /* Sharp, vorpal, holy, vampiric, legendary and nether are all allowed */
        }
        else if(obj->item_type == ITEM_ARMOR)
        {
          if(paf->location == APPLY_AC)
          {
            if(paf->modifier >= -4)
              obj->rarity = UMAX(obj->rarity, RARITY_COMMON);
            else if(paf->modifier >= -9)
              obj->rarity = UMAX(obj->rarity, RARITY_UNCOMMON);
            else
              obj->rarity = UMAX(obj->rarity, RARITY_RARE);
            continue;
          }
        }
        /* It's enchanted when it's not supposed to be OR it has mods it shouldn't */
        return obj->rarity = RARITY_IMPOSSIBLE;
      }
    }
  }
  /* Check some of the values */
  switch(obj->item_type)
  {
    case ITEM_WEAPON:
      if(obj->value[1] != obj->pIndexData->value[1] || obj->value[2] != obj->pIndexData->value[2])
        return obj->rarity = RARITY_IMPOSSIBLE; /* That's a problem */
      break;
    case ITEM_ARMOR:
      if(obj->value[0] != obj->pIndexData->value[0] || obj->value[1] != obj->pIndexData->value[1]
        || obj->value[2] != obj->pIndexData->value[2])
        return obj->rarity = RARITY_IMPOSSIBLE;
      break;
  }
  /* Specific type handling and specific vnum items */
  if(obj->item_type == ITEM_SPELL_PAGE)
    obj->rarity = UMAX(obj->rarity, RARITY_UNCOMMON);
  return obj->rarity;
}


int check_repair_obj(OBJ_DATA *obj, CHAR_DATA *ch, CHAR_DATA *weapon, CHAR_DATA *armor, bool verbose)
{
  int cost = 0;
  CHAR_DATA *victim = NULL;
  if(obj->item_type == ITEM_WEAPON)
    victim = weapon;
  else if(obj->item_type == ITEM_ARMOR || obj->item_type == ITEM_CLOTHING
    || obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_JEWELRY
    || obj->item_type == ITEM_LIGHT ||
    (obj->wear_flags && obj->wear_flags != ITEM_HOLD))
    victim = armor;
  else
  {
    if(verbose)
      send_to_char("That item is too fragile to repair.\n\r", ch);
    return cost;
  }

  if(victim == NULL)
  {
    if(verbose)
      send_to_char("There is nobody here who can repair that item.\n\r", ch);
    return cost;
  }

  if(obj->damaged <= 0)
  {
    if(verbose)
      send_to_char("That item is not damaged.\n\r", ch);
    return cost;
  }

  if(obj->damaged < 60)
  {
    cost = obj->level * obj->level;
    if(cost > 11)// Greatly reduced price for doing it early
      cost /= 11;
  }
  else if(obj->damaged < 100)
    cost = obj->level * obj->level;
  else if(obj->damaged < 1000)
    cost = obj->level * obj->level * 3;
  else
    cost = obj->level * obj->level * 3 * 10;
  cost = UMAX(cost, obj->damaged / 2);// Cover those dirt cheap items
  return cost;
}

void do_repair(CHAR_DATA *ch, char *argument)
{
  char arg[256];
  OBJ_DATA *obj;
  CHAR_DATA *victim;
  char buf[256];
  int cost = 0;
  CHAR_DATA *weapon = NULL, *armor = NULL;

  if(argument[0] == '\0')
  {
    send_to_char("Usage: repair <item> or repair <item> check\n\r", ch);
    return;
  }

  for ( victim = ch->in_room->people;
    victim != NULL;
    victim = victim->next_in_room)
  {
      if (IS_NPC(victim))
      {
    if(armor == NULL && IS_SET(victim->act, ACT_IS_ARMOURER))
    {
        armor = victim;
        if(weapon != NULL)
      break;// Done finding NPCs
    }
    if(weapon == NULL && IS_SET(victim->act, ACT_IS_WEAPONSMITH))
    {
        weapon = victim;
        if(armor != NULL)
      break;// Done finding NPCs
    }
      }
  }

  argument = one_argument(argument,arg);

  if(!str_cmp(arg, "all"))
  {
    bool gear_found = FALSE;
    bool check_cost = FALSE;
    argument = one_argument(argument,arg);
    if(arg[0] == 'c')
      check_cost = TRUE;
    else if(arg[0] != '\0')
    {
      send_to_char("Usage: repair <item> or repair <item> check\n\r", ch);
      return;
    }
      for ( obj = ch->carrying; obj != NULL; obj = obj->next_content )
      {
      if ( can_see_obj( ch, obj ) )
      {
      if(check_cost)
      {
          cost += check_repair_obj(obj, ch, weapon, armor, FALSE);
      }
      else
          if((cost = check_repair_obj(obj, ch, weapon, armor, FALSE)) != 0)
          {
  if(cost > ch->gold * 100 + ch->silver)
  {// Leaving weird indent
    sprintf(buf, "It costs %d silver to repair %s, you don't have enough.\n\r", cost, obj->short_descr);
    send_to_char(buf, ch);
    return;
  }
  deduct_cost(ch,cost);
  sprintf(buf, "%s has been repaired for %d silver.\n\r", obj->short_descr, cost);
  send_to_char(buf, ch);
  if(obj->wear_loc != -1 && obj->damaged >= 100)
  {/* Add bonuses won't add a damaged item */
    obj->damaged = 0;
    add_bonuses(ch, obj);
  }
  else
    obj->damaged = 0;
  gear_found = TRUE;
          }
      }
      }// End for obj loop
      if(!gear_found && (!check_cost || cost == 0))
    send_to_char("You have no gear that is in need of repair.\n\r", ch);
      else if(check_cost)
      {
     sprintf(buf, "It costs %d silver to repair all of your gear.\n\r", cost);
    send_to_char(buf, ch);
      }
    return;
  }

  if((obj = get_obj_here( ch, arg )) == NULL || obj->carried_by != ch)
  {
    send_to_char("You don't have that object available to repair.\n\r", ch);
    return;
  }

  argument = one_argument(argument,arg);

  if((cost = check_repair_obj(obj, ch, weapon, armor, TRUE)) == 0)
    return;

  if(arg[0] == 'c')
  {
    sprintf(buf, "It costs %d silver to repair %s.\n\r", cost, obj->short_descr);
    send_to_char(buf, ch);
    return;
  }
  else if(arg[0] != '\0')
  {
    send_to_char("Usage: repair <item> or repair <item> check\n\r", ch);
    return;
  }

  if(cost > ch->gold * 100 + ch->silver)
  {
    sprintf(buf, "It costs %d silver to repair %s, you don't have enough.\n\r", cost, obj->short_descr);
    send_to_char(buf, ch);
    return;
  }
  deduct_cost(ch,cost);
  sprintf(buf, "%s has been repaired for %d silver.\n\r", obj->short_descr, cost);
  send_to_char(buf, ch);
  if(obj->wear_loc != -1 && obj->damaged >= 100)
  {/* Add bonuses won't add a damaged item */
    obj->damaged = 0;
    add_bonuses(ch, obj);
  }
  else
    obj->damaged = 0;

}

void do_brew( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *potion;
    int chance, num;
    int vnum, cost;
    /* This array contains values of appropriate dam types from attack_table */
    int attack[11] = { 6,12,18,19,20,35,36,38,39,40,41};
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char colour[11][20] = { "red", "black", "gold", "silver", "blue",
                          "green", "yellow", "pink", "bronze", "magenta",
                          "transparent"};

    if ( ( chance = get_skill(ch,gsn_alchemy) ) < 5 ||
        ( !IS_IMMORTAL(ch) && !HAS_KIT(ch,"alchemist") ) )
    {
        send_to_char("H2O, NaCl, augh this is too hard!\n\r",ch);
        return;
    }

    chance /= 2;

    switch ( ch->in_room->sector_type )
    {
    case SECT_MAGELAB_SIMPLE: chance += 10; break;
    case SECT_MAGELAB_INTERMEDIATE: chance += 20; break;
    case SECT_MAGELAB_ADVANCED: chance += 40; break;
    case SECT_MAGELAB_SUPERIOR: chance += 60; break;
    default:
        send_to_char("You must be in a magelab.\n\r",ch);
        return;
    }

    one_argument ( argument, arg );


    if ( !str_cmp( arg, "accuracy" ) )
    {
        cost = COST_ACCURACY;
        vnum = OBJ_VNUM_POTION_ACCURACY;
    }
    else
    if ( !str_cmp( arg, "speed") )
    {
        cost = COST_SPEED;
        vnum = OBJ_VNUM_POTION_SPEED;
    }
    else
    if ( !str_cmp( arg, "resist") )
    {
        cost = COST_MAGIC_RESISTANCE;
        vnum = OBJ_VNUM_POTION_MAGIC_RESISTANCE;
    }
    else
    if ( !str_cmp( arg, "vision") )
    {
        cost = COST_VISION;
        vnum = OBJ_VNUM_POTION_VISION;
    }
    else
    if ( !str_cmp( arg, "mana") )
    {
        cost = COST_RESTORATION;
        vnum = OBJ_VNUM_POTION_RESTORATION;
    }
    else
    if ( !str_cmp( arg, "molotov") )
    {
        cost = COST_MOLOTOV;
        vnum = OBJ_VNUM_POTION_MOLOTOV;
    }
    else
    {
        send_to_char("That isn't a brewable potion.\n\r",ch);
        return;
    }

    if ( ch->gold < cost )
    {
        send_to_char("You don't have enough gold on hand.\n\r",ch);
        return;
    }

    ch->gold -= cost;
    WAIT_STATE( ch, PULSE_PER_SECOND * 5 );

    if ( number_percent() >= chance )
    {
        send_to_char("You failed.\n\r",ch);
        act("$n mixes ingredients together.......and nothing happens. ",ch,NULL,NULL,TO_ROOM,FALSE);
        check_improve(ch,gsn_alchemy,FALSE,4);
        return;
    }
    check_improve(ch,gsn_alchemy,TRUE,4);

    potion = create_object( get_obj_index(vnum), 0, FALSE );
    obj_to_char( potion, ch );

    switch( vnum )
    {
    case OBJ_VNUM_POTION_MOLOTOV:
       num = number_range(0,10);
       sprintf(buf, potion->short_descr, colour[num]);
       free_string(potion->short_descr);
       potion->short_descr = str_dup(buf);
       sprintf(buf, potion->description, colour[num]);
       free_string(potion->description);
       potion->description = str_dup(buf);
       sprintf(buf, potion->name, colour[num]);
       free_string(potion->name);
       potion->name = str_dup(buf);
       /* v4 is used to determine the damage type in do_greande() */
        potion->value[4] = attack[num];
        potion->value[1] = ch->level;

        switch ( ch->in_room->sector_type )
        {
            case SECT_MAGELAB_SIMPLE: potion->value[0] = 6; break;
            case SECT_MAGELAB_INTERMEDIATE: potion->value[0] = 9; break;
            case SECT_MAGELAB_ADVANCED: potion->value[0] = 12; break;
            case SECT_MAGELAB_SUPERIOR: potion->value[0] = 15; break;
        }
        break;
   default:
        potion->value[0] = ch->level;
        break;

    }

    act("$n has created $p!",ch,potion,NULL,TO_ROOM,FALSE);
    act("You create $p!",ch,potion,NULL,TO_CHAR,FALSE);
    return;
}
//#ifdef COREY_TEST
void do_endow (CHAR_DATA *ch, char *argument )
{
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char staffName[MAX_INPUT_LENGTH];
  OBJ_DATA *staff;
  int sn;
  int charges;
  int staffLevel;
  int spellLevel;
  int percent;
  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);

  if( get_skill(ch,gsn_endow) == 0 )
  {
    send_to_char ("Yes, yes, yes, we all know you are well endowed.  Now quit being silly.\n\r",ch);
    return;
  }
  if ( arg1[0] == '\0' || arg2[0] == '\0' )
  {
    send_to_char("Syntax for endow is:  endow spellname staffname \n\r",ch);
    return;
  }
  switch ( ch->in_room->sector_type )
  {
    case SECT_ALTAR_BASIC:
      break;
    case SECT_ALTAR_BLESSED:
      break;
    case SECT_ALTAR_ANNOINTED:
      break;
    case SECT_ALTAR_HOLY_GROUND:
      break;
    default:
      send_to_char("You must be on consecrated ground to endow a staff.\n\r",ch);
      return;
  }
   if ( ( sn = find_spell( ch,arg1 ) ) < 0
    || ( !IS_NPC(ch) && (ch->level < skill_level(ch,sn)
    ||       ch->pcdata->learned[sn] == 0)))
  {
      send_to_char( "You don't know any spells of that name.\n\r", ch );
      return;
  }
  if (skill_table[sn].spell_fun == spell_null)
  {
    send_to_char( "That's not a spell!\n\r", ch );
    return;
  }
  if ( skill_table[sn].bitvector & SS_STAFF )
  {
      //do nothing
  }
  else
  {
      send_to_char("You may not put that spell on a staff.\n\r",ch);
      return;
  }
  if ( ( staff = get_obj_carry( ch, arg2 ) ) == NULL )
  {
      send_to_char( "You do not have that staff.\n\r", ch );
      return;
  }
  if (  staff->value[0] != 0 )
  {
      send_to_char( "That staff already has been endowed.\n\r", ch );
      return;
  }
  if ( ch->pcdata->sac < number_range(175,225))
  {
    send_to_char("You have not given enough sacrifice to your god.\n\r",ch);
    return;
  }
  percent = ( (get_skill(ch,gsn_endow)*8) / 10 ) ;
  switch (staff->pIndexData->vnum )
  {
    case OBJ_VNUM_POOR_STAFF:
      percent += 0;
      charges = 1;
      break;
    case  OBJ_VNUM_MEDIUM_STAFF:
      percent += 5;
      charges = number_range(1,3);
      break;
    case OBJ_VNUM_HIGH_STAFF:
      percent += 10;
      charges = number_range(2,4);
      break;
    default:
      send_to_char("That is not a valid staff.\n\r",ch);
      return;
      break;
  }
  if ( percent >= number_percent() )
  {
    spellLevel= compute_casting_level( ch, sn );
    if (spellLevel > 54)
    {
       spellLevel = 54 ;
    }
    staffLevel = skill_level(ch,sn);
    check_improve(ch,gsn_endow,TRUE,2);
    sprintf (staffName, "a staff of ");
    strcat (staffName, skill_table[sn].name);
    free_string( staff -> name );
    staff->name = str_dup(staffName);
    free_string( staff->short_descr );
    staff->short_descr = str_dup(staffName);
    free_string( staff->description );
    sprintf (staffName, "A staff of ");
    strcat (staffName, skill_table[sn].name);
    strcat (staffName, " lies here.\n\r");
    staff->description = str_dup(staffName);
    staff->cost = 0;
    staff->level = staffLevel;
    staff->value[0] = spellLevel;
    staff->value[1] = 0;
    staff->value[2] = charges;
    staff->value[3] = sn;
    staff->value[4] = -1;
    SET_BIT(staff->extra_flags,ITEM_NOIDENTIFY);
    SET_BIT(staff->extra_flags,ITEM_GLOW);
    send_to_char("{YCRACK{x!  You endow the staff with your god's mojo!\n\r",ch);
    act("$n looks rather pios as the staff glows with a holy aura.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
  }
  else
  {
  //they fail
    check_improve(ch,gsn_endow,FALSE,4);
    send_to_char("{YCRACK{x!  You snap the staff in two!\n\r",ch);
    act("$n curses the gods as the staff breaks in two.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
    extract_obj(staff);
    return;
  }


}
void do_infuse (CHAR_DATA *ch, char *argument )
{ //start do_wmake
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char wand_name[MAX_INPUT_LENGTH];
    OBJ_DATA *wand;
        int sn;
        int charges;
        int spell_level1;
        int spell_level;
        int spell_level2;
        int percent;
    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    if( get_skill(ch,gsn_infuse) == 0 )
   { send_to_char ("You do not know how to infuse things.  Do not be a dumb-bunny.\n\r",ch);
        return;
        }
    if ( ( arg1[0] == '\0' ) || ( arg2[0] == '\0' ) )
    {
  send_to_char( "Syntax for infuse is: infuse spellname wandname \n\r", ch );
  return;
    }

   /*Begin the sanity checks*/
    switch ( ch->in_room->sector_type )
    {
    case SECT_MAGELAB_SIMPLE:  break;
    case SECT_MAGELAB_INTERMEDIATE:  break;
    case SECT_MAGELAB_ADVANCED:  break;
    case SECT_MAGELAB_SUPERIOR:  break;
    default:
        send_to_char("You must be in a magelab to infuse.\n\r",ch);
        return;
    }
    if ( ( sn = find_spell( ch,arg1 ) ) < 0
    || ( !IS_NPC(ch) && (ch->level < skill_level(ch,sn)
    ||       ch->pcdata->learned[sn] == 0)))
    {
        send_to_char( "You don't know any spells of that name.\n\r", ch );
        return;
    }
    if (skill_table[sn].spell_fun == spell_null)
    {
        send_to_char( "That's not a spell!\n\r", ch );
        return;
    }

    if ( skill_table[sn].bitvector & SS_WAND )
    {
        //do nothing
    }
    else
    {
        send_to_char("You may not put that spell on a wand.\n\r",ch);
        return;
    }

    if ( ( wand = get_obj_carry( ch, arg2 ) ) == NULL )
    {
        send_to_char( "You do not have that wand.\n\r", ch );
        return;
    }
    if (  wand->value[0] != 0 )
    {
        send_to_char( "That wand already has been infused.\n\r", ch );
        return;
    }

        switch ( wand->pIndexData->vnum )
        {
                case OBJ_VNUM_WAND_PINE:
                        charges = number_range(1,3);
                        percent -= 25;
                        break;
                case OBJ_VNUM_WAND_APPLE:
                        charges = number_range(2,4);
                        percent -= 15 ;
                        break;
                case OBJ_VNUM_WAND_OAK:
                        charges = number_range(3,5);
                        percent -= 5;
                        break;
                case OBJ_VNUM_WAND_WILLOW:
                        charges = number_range(4,6);
                        percent += 5;
                        break;
                case OBJ_VNUM_WAND_YEW:
                        charges = number_range(5,7);
                        percent += 10;
                        break;
                default:
                        send_to_char("That's not a valid wand.\n\r",ch);
                        return;
                        break;
        }

        percent = ( (get_skill(ch,gsn_infuse)*8) / 10 ) ;

        if ( percent >= number_percent() )
        {
                spell_level = compute_casting_level( ch, sn );
                if (spell_level > 54)
                {
                  spell_level = 54 ;
                }

          if ( sn == skill_lookup("turn undead") )
          {
            spell_level2 -= 5;
          }
          if ( number_percent() < 5 && number_percent() >= 1)
          {
            spell_level1 = skill_level(ch,sn);
            spell_level2 = spell_level;
            check_improve(ch,gsn_infuse,TRUE,2);

            sprintf (wand_name, "a wand of ");
            strcat (wand_name, "rasefseers");
            free_string( wand -> name );
            wand->name = str_dup(wand_name);
            free_string( wand->short_descr );
            wand->short_descr = str_dup(wand_name);
            free_string( wand->description );
            sprintf (wand_name, "A wand of ");
            strcat (wand_name, "rasefseers");
            strcat (wand_name, " lies here.\n\r");
            wand->description = str_dup(wand_name);
            wand->level = spell_level1;
            wand->cost = 0;
            wand->value[1] = charges;
            wand->value[0] = spell_level2;
            wand->value[2] = charges;
            wand->value[3] = skill_lookup("restrain");;
            wand->value[4] = -1;
            SET_BIT(wand->extra_flags,ITEM_NOIDENTIFY);
            SET_BIT(wand->extra_flags,ITEM_HUM);
            send_to_char("{YCRACK{x!  You infuse the wand with a special kind of mojo!\n\r",ch);
            act("$n grins evily as the piece of wood glows with a magical aura.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
          }
          else
          {
            spell_level1 = skill_level(ch,sn);
            spell_level2 = spell_level;
            check_improve(ch,gsn_infuse,TRUE,2);
            sprintf (wand_name, "a wand of ");
            strcat (wand_name, skill_table[sn].name);
            free_string( wand -> name );
            wand->name = str_dup(wand_name);
            free_string( wand->short_descr );
            wand->short_descr = str_dup(wand_name);
            free_string( wand->description );
            sprintf (wand_name, "A wand of ");
            strcat (wand_name, skill_table[sn].name);
            strcat (wand_name, " lies here.\n\r");
            wand->description = str_dup(wand_name);
            wand->level = spell_level1;
            wand->cost = 0;
             wand->value[1] = 0;
            wand->value[0] = spell_level2;
            wand->value[2] = charges;
            wand->value[3] = sn;
             wand->value[4] = -1;
            SET_BIT(wand->extra_flags,ITEM_NOIDENTIFY);
            SET_BIT(wand->extra_flags,ITEM_GLOW);
            send_to_char("{YCRACK{x!  You infuse the wand with your mojo!\n\r",ch);
            act("$n grins evily as the piece of wood glows with a magical aura.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
          }
        }
        else
        {
                //they fail
                check_improve(ch,gsn_infuse,FALSE,4);
                send_to_char("{YCRACK{x!  You snap the wand in two!\n\r",ch);
                act("$n curses as the wand breaks in two.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
                extract_obj(wand);
                return;
        }
} //end do_wmake
//#endif
void do_scribe( CHAR_DATA *ch, char *argument )
{ /*bFALSegin do_scribe */

    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char arg3[MAX_INPUT_LENGTH];

    char scroll_name[MAX_INPUT_LENGTH];
    OBJ_DATA *scroll;
    OBJ_DATA *ink;

    /*OBJ_DATA *obj;*/
        int sn;
        int new_sn_percent;
        int way_cool_spell;

        int spell_level1;
        int spell_level;
        int spell_level2;

        int percent;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );
    argument = one_argument( argument, arg3 );

    if ( ( arg1[0] == '\0' ) || ( arg3[0] == '\0' ) || ( arg2[0] == '\0' ) )
    {
  send_to_char( "Syntax for scribe is: scribe spellname scrollname inkname\n\r", ch );
  return;
    }


    /*Begin the sanity checks*/
    switch ( ch->in_room->sector_type )
    {
    case SECT_MAGELAB_SIMPLE:  break;
    case SECT_MAGELAB_INTERMEDIATE:  break;
    case SECT_MAGELAB_ADVANCED:  break;
    case SECT_MAGELAB_SUPERIOR:  break;
    default:
        send_to_char("You must be in a magelab.\n\r",ch);
        return;
    }


    if ( ( sn = find_spell( ch,arg1 ) ) < 0
    || ( !IS_NPC(ch) && (ch->level < skill_level(ch,sn)
    ||       ch->pcdata->learned[sn] == 0)))
    {
        send_to_char( "You don't know any spells of that name.\n\r", ch );
        return;
    }

    if (skill_table[sn].spell_fun == spell_null)
    {
        send_to_char( "That's not a spell!\n\r", ch );
        return;
    }

    if ( skill_table[sn].bitvector & SS_SCRIBE )
    {
        //do nothing
    }
    else
    {
        send_to_char("You may not put that spell on a scroll.\n\r",ch);
        return;
    }

    if ( ( scroll = get_obj_carry( ch, arg2 ) ) == NULL )
    {
        send_to_char( "You do not have that scroll.\n\r", ch );
        return;
    }

    if ( ( ink = get_obj_carry( ch, arg3 ) ) == NULL )
    {
        send_to_char( "You do not have that ink.\n\r",ch);
        return;
    }


    /*Now that they have a scroll, check to see if it's the right kind for the level */

    spell_level = compute_casting_level( ch, sn );
    if (spell_level >= 55)
    {
        spell_level = 54;
    }
    spell_level1 = skill_level(ch,sn);

    if ( spell_level1 >= 50
    && scroll->pIndexData->vnum != OBJ_VNUM_SCRIBE_PARCHMENT)
    {
        send_to_char("You need to use a parchment scroll to scribe this spell.\n\r",ch);
        return;
    }

    if ( (spell_level1 >= 39 && spell_level1 <= 49) && (scroll->pIndexData->vnum != OBJ_VNUM_SCRIBE_VELLUM) )
    {
        send_to_char("You need to use a vellum scroll to scribe this spell.\n\r",ch);
        return;
    }

    if ( (spell_level1 >= 25 && spell_level1 <=38)
    && ( scroll->pIndexData->vnum != OBJ_VNUM_SCRIBE_RICEPAPER) )
    {
        send_to_char("You need to put that spell on a rice paper scroll.\n\r",ch);
        return;
    }

    if ( (spell_level1 >= 11 && spell_level1 <=24 )
   && ( scroll->pIndexData->vnum != OBJ_VNUM_SCRIBE_KOZO) )

    {
        send_to_char("You need to scribe that spell on to a kozo scroll. \n\r",ch);
        return;
    }
    if ( (spell_level1 >= 1 && spell_level1 <=10 )
    && (scroll->pIndexData->vnum != OBJ_VNUM_SCRIBE_PAPYRUS ) )
    {
        send_to_char("You need to scribe that spell on to a papyrus scroll.\n\r",ch);
        return;
    }


/* Ok put the checks here to see if they succed or not*/

        percent = ( (get_skill(ch,gsn_scribe)*8) / 10 ) ;

        switch ( ink->pIndexData->vnum )
        {
                case OBJ_VNUM_SCRIBE_DBLOOD:
                        percent += 15;
                        break;

                case OBJ_VNUM_SCRIBE_INDIGO:
                        percent += 10;
                        break;
                case OBJ_VNUM_SCRIBE_BISTRE:
                        percent += 5;
                        break;

                case OBJ_VNUM_SCRIBE_SEPIA:
                        percent -= 10;
                        break;
                default:
                        percent += 0;
                        return;
        }

        percent = URANGE(5,percent,100);

        /*send_to_char("Checking to see if the scribe is going to work or not.\n\r",ch);*/

        new_sn_percent = number_percent( );


        if ( sn == skill_lookup("dispel magic") )
        {
                spell_level2 -= 15;
        }

        if ( (number_percent( ) < percent) )
        {
                /*spell_level2=(spell_level + spell_level1)/2;i*/
                spell_level2=spell_level;
                check_improve(ch,gsn_scribe,TRUE,2);
                sprintf (scroll_name, "a scroll of ");
                strcat (scroll_name, skill_table[sn].name);
                free_string( scroll -> name );
                scroll->name = str_dup(scroll_name);
                free_string( scroll->short_descr );
                scroll->short_descr = str_dup(scroll_name);
                free_string( scroll->description );
                sprintf (scroll_name, "A scroll of ");
                strcat (scroll_name, skill_table[sn].name);
                strcat (scroll_name, " lies here.\n\r");
                scroll->description = str_dup(scroll_name);
                scroll->level = spell_level1;
                scroll->cost = 0;
                scroll->value[1] =  sn;
                scroll->value[0] = spell_level2;
                scroll->value[2] = -1;
                scroll->value[3] = -1;
                scroll->value[4] = -1;
                SET_BIT(scroll->extra_flags,ITEM_NOIDENTIFY);
                /*scroll->extra_flags = ITEM_NOIDENTIFY;*/

                way_cool_spell = number_percent( );

                if (way_cool_spell <= 5)
                {
                        send_to_char("Whoa, what is this spell you have scribed?\n\r",ch);
                        SET_BIT(scroll->extra_flags,ITEM_HUM);

                        switch ( number_range(0,5)  )
                        {
                        case 0:
                                scroll->value[3] = skill_lookup("pox");
                                scroll->value[2] = sn;
                                scroll->value[1] = skill_lookup("haste");
                                SET_BIT(scroll->extra_flags,ITEM_MAGIC);
                                break;
                        case 1:
                                scroll->value[3] = skill_lookup("learned knowledge");
                                scroll->value[1] = sn;
                                scroll->value[2] = skill_lookup("sanctuary");
                                SET_BIT(scroll->extra_flags,ITEM_INVIS);
                                break;
                        case 2:
                                scroll->value[3] = skill_lookup("wrath of the pen");
                                scroll->value[1] = sn;
                                scroll->value[2] = skill_lookup("confusion");
                                SET_BIT(scroll->extra_flags,ITEM_EVIL);
                                SET_BIT(scroll->extra_flags,ITEM_GLOW);
                                break;
                        case 3:
                                scroll->value[3] = skill_lookup("magical rest");
                                scroll->value[2] = sn;
                                scroll->value[1] = skill_lookup("calm");
                                SET_BIT(scroll->extra_flags,ITEM_MELT_DROP);
                                break;
                        case 4:
                                scroll->value[1] = skill_lookup("haste");
                                scroll->value[2] = skill_lookup("shield");;
                                scroll->value[3] = skill_lookup("giant strength");
                                SET_BIT(scroll->extra_flags,ITEM_BLESS);
                                SET_BIT(scroll->extra_flags,ITEM_MAGIC);
                                break;
                        case 5:
                                scroll->value[1] = skill_lookup("shield");
                                scroll->value[2] = skill_lookup("magical rest");
                                scroll->value[3] = skill_lookup("learned knowledge");
                                SET_BIT(scroll->extra_flags,ITEM_NODROP);
                                SET_BIT(scroll->extra_flags,ITEM_INVIS);
                                SET_BIT(scroll->extra_flags,ITEM_GLOW);
                                break;

                        default:
                                break;
                        }
                }



                act("$n begins scribbling furiously on a piece of paper.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
                send_to_char("You have created a new scroll.\n\r",ch);
                extract_obj(ink);

                return;
        }
        else
        {
                /*they fail*/
                check_improve(ch,gsn_scribe,FALSE,4);
                send_to_char("Whoops!  This isn't the scroll you were trying to write.\n\r",ch);
                act("$n spills his ink all over the page.  What a mess.\n\r",ch,NULL,NULL,TO_ROOM,FALSE);
                extract_obj(scroll);
                extract_obj(ink);
                return;
        }



} /*end do_scribe */

void do_deposit( CHAR_DATA *ch, char *argument )
{
    int egg_deposit, brick_deposit, amount, egg_has = 0, brick_has = 0;
    OBJ_DATA *money_lose,  *money_next;
    CHAR_DATA *banker;
    char    buf[MAX_STRING_LENGTH];
    char    buf2[MAX_STRING_LENGTH];
    char    arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if(IS_NPC(ch))
    {
      send_to_char("Sorry, NPCs don't have accounts.\n\r", ch);
      return;
    }
    argument = one_argument(argument,arg);

    for (banker=ch->in_room->people;banker!=NULL;banker=banker->next_in_room)
    {
      if (IS_NPC(banker) && IS_SET(banker->act,ACT_IS_CHANGER))
        break;
    }

    if (banker == NULL)
    {
      send_to_char("You don't see any trustworthy bankers around.\n\r",ch);
      return;
    }

    argument = one_argument(argument,arg2);

    if(arg2[0])
    {/* Both types if nothing is specified */
      if(is_number(arg2))
      {
        amount = atoi(arg2);
        if(amount <= 0)
        {
          send_to_char("You can't deposit what you don't have.\n\r", ch);
          return;
        }
      }
      else if(!str_cmp(arg2, "all"))
      {
        amount = 0;
      }
      else
      {
        send_to_char("Syntax: deposit <all|eggs|bricks> <(Optional)all|# to deposit>\n\r", ch);
        return;
      }
    }
    else
      amount = 1;

    if(!str_cmp(arg, "all"))
    {
      if(!arg2[0])
        amount = 0;
      egg_deposit = amount;/* Deposit both */
      brick_deposit = amount;
    }
    else if(!str_prefix(arg, "eggs"))
    {
      egg_deposit = amount;
      brick_deposit = -1;/* No bricks */
    }
    else if(!str_prefix(arg, "bricks"))
    {
      egg_deposit = -1;/* No eggs */
      brick_deposit = amount;
    }
    else
    {
      send_to_char("Syntax: deposit <all|eggs|bricks> <(Optional)all|# to deposit>\n\r", ch);
      return;
    }

    /* count how many eggs and bricks there are in inventory */
    for ( money_next = ch->carrying; money_next!=NULL; money_next = money_next->next_content )
    {
      if(can_see_obj( ch, money_next ) && money_next->wear_loc == WEAR_NONE)
      {
        if (money_next->pIndexData->vnum == OBJ_VNUM_EGG)
          egg_has++;
        else if(money_next->pIndexData->vnum == OBJ_VNUM_BRICK)
          brick_has++;
      }
    }

    if((egg_deposit < 0 || !egg_has) && (brick_deposit < 0 || !brick_has))
    {
      send_to_char("Eggs or bricks must be in your inventory to deposit.\n\r", ch);
      return;
    }

    if(!egg_deposit || egg_has < egg_deposit)
      egg_deposit = egg_has; /* All of them */
    if(!brick_deposit || brick_has < brick_deposit)
      brick_deposit = brick_has;

    if(egg_deposit > 0 && brick_deposit > 0)
    {
      sprintf(buf,"You deposit %d egg%s and %d platinum brick%s.",
        egg_deposit,egg_deposit == 1 ? "" : "s", brick_deposit,brick_deposit == 1 ? "" : "s");
      sprintf(buf2,"$n deposits %d egg%s and %d platinum brick%s.",
        egg_deposit,egg_deposit == 1 ? "" : "s", brick_deposit,brick_deposit == 1 ? "" : "s");
    }
    else if(egg_deposit > 0)
    {
      sprintf(buf,"You deposit %d egg%s.",egg_deposit,egg_deposit == 1 ? "" : "s");
      sprintf(buf2,"$n deposits %d egg%s.",egg_deposit,egg_deposit == 1 ? "" : "s");
    }
    else
    {
      sprintf(buf,"You deposit %d platinum brick%s.",brick_deposit,brick_deposit == 1 ? "" : "s");
      sprintf(buf2,"$n deposits %d platinum brick%s.",brick_deposit,brick_deposit == 1 ? "" : "s");
    }
    act(buf,ch,NULL,NULL,TO_CHAR,FALSE);
    act(buf2,ch,NULL,NULL,TO_ROOM,FALSE);

    for (money_lose = ch->carrying;
         money_lose != NULL;
         money_lose = money_next )
        {
        money_next = money_lose->next_content;

        if(can_see_obj( ch, money_lose ) && money_lose->wear_loc == WEAR_NONE)
        {
          if (egg_deposit > 0 && money_lose->pIndexData->vnum == OBJ_VNUM_EGG)
          {
              obj_from_char(money_lose);
              extract_obj(money_lose);
              ch->pcdata->bank_eggs++;
              egg_deposit--;
          }
          if (brick_deposit > 0 && money_lose->pIndexData->vnum == OBJ_VNUM_BRICK)
          {
              obj_from_char(money_lose);
              extract_obj(money_lose);
              ch->pcdata->bank_bricks++;
              brick_deposit--;
          }
        }
        }

    sprintf(buf, "%s deposited %d eggs %d bricks for a new balance of %d eggs %d bricks.",
      ch->name, egg_deposit, brick_deposit, ch->pcdata->bank_eggs, ch->pcdata->bank_bricks);
    log_string(buf);

    sprintf(buf,"Your balance is %d egg%s and %d platinum brick%s.\n\r",
      ch->pcdata->bank_eggs,ch->pcdata->bank_eggs == 1 ? "" : "s",
      ch->pcdata->bank_bricks,ch->pcdata->bank_bricks == 1 ? "" : "s");
    send_to_char(buf,ch);

    save_char_obj(ch);
}

void do_withdraw( CHAR_DATA *ch, char *argument )
{
    int egg_withdraw, brick_withdraw, amount;
    OBJ_DATA *money;
    CHAR_DATA *banker;
    char    buf[MAX_STRING_LENGTH];
    char    buf2[MAX_STRING_LENGTH];
    char    arg[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if(IS_NPC(ch))
    {
      send_to_char("Sorry, NPCs don't have accounts.\n\r", ch);
      return;
    }

    argument = one_argument(argument,arg);

    for (banker=ch->in_room->people;banker!=NULL;banker=banker->next_in_room)
    {
      if (IS_NPC(banker) && IS_SET(banker->act,ACT_IS_CHANGER))
        break;
    }

    if (banker == NULL)
    {
      send_to_char("You don't see any trustworthy bankers around.\n\r",ch);
      return;
    }

    if ( !ch->pcdata->bank_eggs && !ch->pcdata->bank_bricks)
    {
      send_to_char("You don't have any assets saved.\n\r",ch);
      return;
    }

    argument = one_argument(argument,arg2);

    if(arg2[0])
    {/* Both types if nothing is specified */
      if(is_number(arg2))
      {
        amount = atoi(arg2);
        if(amount <= 0)
        {
          send_to_char("You can't withdraw what doesn't exist.\n\r", ch);
          return;
        }
      }
      else if(!str_cmp(arg2, "all"))
      {
        amount = 0;
      }
      else
      {
        send_to_char("Syntax: withdraw <all|eggs|bricks> <(Optional)all|# to withdraw>\n\r", ch);
        return;
      }
    }
    else
      amount = 1;

    if(!str_cmp(arg, "all"))
    {
      if(!arg2[0])/* Override the normal 1 */
        amount = 0;
      egg_withdraw = amount;/* Withdraw both */
      brick_withdraw = amount;
    }
    else if(!str_prefix(arg, "eggs"))
    {
      egg_withdraw = amount;
      brick_withdraw = -1;/* No bricks */
    }
    else if(!str_prefix(arg, "bricks"))
    {
      egg_withdraw = -1;/* No eggs */
      brick_withdraw = amount;
    }
    else
    {
      send_to_char("Syntax: withdraw <all|eggs|bricks> <(Optional)all|# to withdraw>\n\r", ch);
      return;
    }

    if(!egg_withdraw)
      egg_withdraw = ch->pcdata->bank_eggs; /* All of them */
    if(!brick_withdraw)
      brick_withdraw = ch->pcdata->bank_bricks;

    if(ch->pcdata->bank_eggs < egg_withdraw || ch->pcdata->bank_bricks < brick_withdraw)
    {
      send_to_char("You don't have that much to withdraw.\n\r", ch);
      sprintf(buf,"Your balance is %d egg%s and %d platinum brick%s.\n\r",
        ch->pcdata->bank_eggs,ch->pcdata->bank_eggs == 1 ? "" : "s",
        ch->pcdata->bank_bricks,ch->pcdata->bank_bricks == 1 ? "" : "s");
      send_to_char(buf,ch);
      return;
    }

    if(egg_withdraw > 0 && brick_withdraw > 0)
    {
      sprintf(buf,"You withdraw %d egg%s and %d platinum brick%s.",
        egg_withdraw,egg_withdraw == 1 ? "" : "s", brick_withdraw,brick_withdraw == 1 ? "" : "s");
      sprintf(buf2,"$n withdraws %d egg%s and %d platinum brick%s.",
        egg_withdraw,egg_withdraw == 1 ? "" : "s", brick_withdraw,brick_withdraw == 1 ? "" : "s");
    }
    else if(egg_withdraw > 0)
    {
      sprintf(buf,"You withdraw %d egg%s.",egg_withdraw,egg_withdraw == 1 ? "" : "s");
      sprintf(buf2,"$n withdraws %d egg%s.",egg_withdraw,egg_withdraw == 1 ? "" : "s");
    }
    else
    {
      sprintf(buf,"You withdraw %d platinum brick%s.",brick_withdraw,brick_withdraw == 1 ? "" : "s");
      sprintf(buf2,"$n withdraws %d platinum brick%s.",brick_withdraw,brick_withdraw == 1 ? "" : "s");
    }
    act(buf,ch,NULL,NULL,TO_CHAR,FALSE);
    act(buf2,ch,NULL,NULL,TO_ROOM,FALSE);

    sprintf(buf, "%s withdrew %d eggs %d bricks for a new balance of %d eggs %d bricks.",
      ch->name, egg_withdraw, brick_withdraw, ch->pcdata->bank_eggs, ch->pcdata->bank_bricks);
    log_string(buf);

    for(; egg_withdraw > 0; egg_withdraw--)
    {
      money = create_object( get_obj_index(OBJ_VNUM_EGG), 0, FALSE );
      obj_to_char(money,ch);
      ch->pcdata->bank_eggs--;
    }

    for(; brick_withdraw > 0; brick_withdraw--)
    {
      money = create_object( get_obj_index(OBJ_VNUM_BRICK), 0, FALSE );
      obj_to_char(money,ch);
      ch->pcdata->bank_bricks--;
    }

    sprintf(buf,"Your balance is %d egg%s and %d platinum brick%s.\n\r",
      ch->pcdata->bank_eggs,ch->pcdata->bank_eggs == 1 ? "" : "s",
      ch->pcdata->bank_bricks,ch->pcdata->bank_bricks == 1 ? "" : "s");
    send_to_char(buf,ch);

    save_char_obj(ch);
}

/* RT part of the corpse looting code */

bool can_loot(CHAR_DATA *ch, OBJ_DATA *obj, bool loot_check)
{
  CHAR_DATA *owner, *wch;

  if (IS_IMMORTAL(ch))
    return TRUE;

  owner = NULL;
  if (obj->owner) {
    for ( wch = char_list; wch != NULL ; wch = wch->next )
        if (!str_cmp(wch->name,obj->owner))
            owner = wch;
  }

  if (owner == NULL)
    return TRUE;

  if (!str_cmp(ch->name,owner->name))
    return TRUE;

  if (!IS_NPC(owner) && !IS_NPC(ch))
  {
      if (IS_SET(owner->act,PLR_CANLOOT))
         return TRUE;

     /* Seperate Clan and Nonclan corpses at this point */
     if (is_clan(owner))
     {
        if ( (is_same_group(ch,owner) && !is_affected(owner,AFF_CHARM) ) || is_same_clan(ch,owner))
           return TRUE;
     }
     else
     {
        if (is_same_group(ch,owner) )
           if(IS_SET(owner->act,PLR_CANLOOT))
              return TRUE;
     }

     if (IS_SET(obj->extra_flags,ITEM_CLAN_CORPSE)
        && is_clan(ch) && loot_check)
        return TRUE;
  }

  return FALSE;
}


void get_obj( CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *container )
{
    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];
    if ( !CAN_WEAR(obj, ITEM_TAKE) )
    {
  send_to_char( "You can't take that.\n\r", ch );
  return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
  act( "$d: you can't carry that many items.",
      ch, NULL, obj->name, TO_CHAR ,FALSE);
  return;
    }


    if ( get_carry_weight(ch) + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
  act( "$d: you can't carry that much weight.",
      ch, NULL, obj->name, TO_CHAR ,FALSE);
  return;
    }

    if (!can_loot(ch,obj,TRUE))
    {
  act("Corpse looting is not permitted.",ch,NULL,NULL,TO_CHAR ,FALSE);
  return;
    }

    if (obj->in_room != NULL)
    {
  for (gch = obj->in_room->people; gch != NULL; gch = gch->next_in_room)
      if (gch->on == obj)
      {
    act("$N appears to be using $p.",
        ch,obj,gch,TO_CHAR,FALSE);
    return;
      }
    }

    if ( container != NULL )
    {
      if (container->pIndexData->vnum == OBJ_VNUM_PIT
  &&  get_trust(ch) < obj->level)
  {
      send_to_char("You are not powerful enough to use it.\n\r",ch);
      return;
  }

  if (!CAN_WEAR(container,ITEM_TAKE) && !IS_OBJ_STAT(obj,ITEM_HAD_TIMER) &&
    (container->pIndexData->vnum == OBJ_VNUM_PIT || container->pIndexData->vnum == OBJ_VNUM_MATOOK_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_DEMISE_PIT || container->pIndexData->vnum == OBJ_VNUM_HONOR_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_POSSE_PIT || container->pIndexData->vnum == OBJ_VNUM_ZEALOT_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_WARLOCK_PIT || container->pIndexData->vnum == OBJ_VNUM_FLUZEL_PIT ||
    (container->pIndexData->vnum < 0 && container->pIndexData->item_type == ITEM_CONTAINER)))
  {
    if(obj->timer)
      obj->timer = 0;
    else
    {/* Check if any other item in this pit has a timer and yank it */
     /* obj is known to be earlier than the timer since it doesn't have one */
      OBJ_DATA *obj_check = obj;
      for(; obj_check; obj_check = obj_check->next_content)
      {
        if(obj_check->timer && !IS_OBJ_STAT(obj, ITEM_HAD_TIMER))
        {/* One item removed, one timer removed */
          obj_check->timer = 0;
          break;
        }
      }
    }
  }
  act( "You get $p from $P.", ch, obj, container, TO_CHAR ,FALSE);
  act( "$n gets $p from $P.", ch, obj, container, TO_ROOM ,FALSE);

  REMOVE_BIT(obj->extra_flags,ITEM_HAD_TIMER);
  obj_from_obj( obj );
    }
    else
    {
  act( "You get $p.", ch, obj, container, TO_CHAR ,FALSE);
  act( "$n gets $p.", ch, obj, container, TO_ROOM ,FALSE);
  obj_from_room( obj );
    }

    if ( obj->item_type == ITEM_MONEY)
    {
     if ( (get_carry_weight(ch) + obj->value[0]/10 + obj->value[1]*2/5)
          > can_carry_w( ch ))
     {
       act( "$N can't carry that much weight.", ch, NULL, ch, TO_CHAR ,FALSE);
       obj_to_room( obj, ch->in_room );
       return;
     }
  ch->silver += obj->value[0];
  ch->gold += obj->value[1];
        if (IS_SET(ch->act,PLR_AUTOSPLIT))
        { /* AUTOSPLIT code */
        members = 0;
        for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
        {
            if (!IS_AFFECTED(gch,AFF_CHARM) && is_same_group( gch, ch ) )
              members++;
        }

    if ( members > 1 && (obj->value[0] > 1 || obj->value[1]))
    {
      sprintf(buffer,"%d %d",obj->value[0],obj->value[1]);
      do_split(ch,buffer);
    }
        }

  extract_obj( obj );
    }
    else
    {
  obj_to_char( obj, ch );
    }

    return;
}

void do_specializ( CHAR_DATA *ch, char *argument )
{
    send_to_char("This command must be entered in full.\n\r",ch);
    return;
}

void do_specialize( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *mob;
    int weapon;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char("Syntax: specialize <weapon group>\n\r",ch);
        send_to_char("        specialize help\n\r",ch);
        return;
    }

    if ( IS_NPC(ch) )
        return;

    if ( ch->pcdata->old_class != class_lookup("warrior") )
    {
        send_to_char("Only warriors may specialize.\n\r",ch);
        return;
    }

    if ( !str_cmp( arg, "help" ) )
    {
        int i;

        do_help(ch,"specialize");

        send_to_char("Valid weapon groups are:\n\r",ch);
        send_to_char("(Note that these may not all be available "
                     " depeding on your class)\n\r",ch);
        for ( i = skill_lookup("axe" ) ;
              i <= skill_lookup("whip") ;
              i++ )
        {
            sprintf(buf," * %-12s %d\n\r",
                skill_table[i].name,
                (10 * skill_table[i].rating[ch->class]));
            send_to_char(buf,ch);
        }

        return;
    }

    if ( IS_NPC(ch) )
        return;

    for ( mob = ch->in_room->people; mob != NULL; mob = mob->next_in_room )
    {
       if ( IS_NPC(mob) && IS_SET(mob->act, ACT_TRAIN) )
       break;
    }

    if ( mob == NULL )
    {
       send_to_char( "You can't do that here.\n\r", ch );
       return;
    }

    /* Get GSN for weapon skill */
    weapon = skill_lookup(arg);

    if ( weapon            < skill_lookup("axe") ||
         weapon            > skill_lookup("whip") )
    {
        send_to_char("That's not a valid weapon group.\n\r",ch);
        return;
    }

/*
        ch->train < skill_table[weapon].rating[ch->class] )
*/
    if ( !HAS_KIT(ch,"myrmidon") &&
        ch->practice < skill_table[weapon].rating[ch->class] * 10 )
    {
        send_to_char("You don't have enough practices to specialize.\n\r",ch);
        return;
    }

    if ( ch->pcdata->learned[weapon] == 0 )
    {
        send_to_char("You don't know that weapon group anyway!\n\r",ch);
        return;
    }

    /* Special handling */
    if ( class_table[ch->class].reclass )
    {
        int fCan = TRUE;

       if ( ch->class == class_lookup("samurai") )
       {
          if ( weapon != skill_lookup("sword") &&
               weapon != skill_lookup("polearm") &&
               weapon != skill_lookup("dagger") &&
               (ch->race != race_lookup("dwarf") || weapon != skill_lookup("axe")))
                fCan = FALSE;
        }
        else
        if ( ch->class == class_lookup("paladin") )
        {
          if ( weapon != skill_lookup("sword") &&
               weapon != skill_lookup("flail") &&
               weapon != skill_lookup("mace" ) &&
               (ch->race != race_lookup("dwarf") || weapon != skill_lookup("axe")))
                fCan = FALSE;
        }
        else
        if ( ch->class == class_lookup("berzerker") )
        {
          if ( weapon != skill_lookup("sword") &&
               weapon != skill_lookup("mace") &&
               weapon != skill_lookup("axe") )
                fCan = FALSE;
        }

        if ( !fCan )
        {
            send_to_char("You cannot specialize in that.\n\r",ch);
            return;
         }
   }
   if ( !HAS_KIT(ch,"myrmidon") )
      ch->practice -= skill_table[weapon].rating[ch->class] * 10;
    /*
   ch->train -= skill_table[weapon].rating[ch->class];
    */
   ch->pcdata->specialize = weapon;
   send_to_char("You are now specialized.\n\r",ch);
   return;
}

void return_clan_gear(CHAR_DATA *ch, OBJ_DATA *corpse)
{
  OBJ_DATA *item, *next_item;
  for(item = ch->carrying; item; item = item->next_content)
  {
    if(item->wear_loc == WEAR_HIDDEN)
    {
      item->wear_loc = WEAR_NONE;
      if(item->damaged == 1000)
        act("$p is {Rshattered{x and will cost extra to repair.", ch, item, NULL, TO_CHAR, FALSE);
      else if(item->damaged == 100)
      {
        if(!ch->pcdata || !ch->pcdata->clan_info ||
          ch->pcdata->clan_info->clan->type != CLAN_TYPE_PEACE ||
          ch->pcdata->clan_info->clan->tribute < 0 ||
          ch->pcdata->clan_info->clan->max_tribute < 20000)
          act("$p is {Rbroken{x and will shatter if you die again.", ch, item, NULL, TO_CHAR, FALSE);
        else
          act("$p is {Rbroken{x.", ch, item, NULL, TO_CHAR, FALSE);
      }
    }
  }

  send_to_char("{WYour gear has returned to you.{x\n\r", ch);

  for(item = corpse->contains; item; item = next_item)
  {
    next_item = item->next_content;
    obj_from_obj(item);
    obj_to_char(item, ch);
  }

  ch->pcdata->corpse_timer = 0;/* Zero so they can quit if they want */

  corpse->value[4] = 0; /* Emptied */
  corpse->loot_next = NULL;

  save_char_obj(ch);
}

int get_link_limit(CHAR_DATA *ch)
{
  if(IS_NPC(ch))
    return 0;
  if(IS_IMMORTAL(ch))
    return LINK_MAX;
  if(!is_clan(ch) || IS_SET(ch->act, PLR_DWEEB))
    return 0;
  int amount = LINK_NORMAL;
  if(ch->pcdata->clan_info)
  {/* Only can get bonuses from new clan system stuff */
    if(ch->pcdata->clan_info->clan->type == CLAN_TYPE_PEACE)
      amount++;
  }
  return UMIN(amount, LINK_MAX);
}

void do_link(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  if(IS_NPC(ch))
    return;

  if(IS_SET(ch->mhs, MHS_GLADIATOR))
  {
    send_to_char("Gladiators can't link items.\n\r", ch);
    return;
  }

  int curlink;
  if(argument[0] == '\0')
  {
    char buf[256];
    if(ch->pcdata->linked[0] == NULL)
    {
      send_to_char("You have no items linked currently.\n\r", ch);
    }
    else
    {
      send_to_char("You have the following items linked:\n\r", ch);
      for(curlink = 0; curlink < LINK_MAX; curlink++)
      {
        if(ch->pcdata->linked[curlink] == NULL)
          break;
        sprintf(buf, "$p");/* Eventually put condition/quality here */
        act(buf, ch, ch->pcdata->linked[curlink], NULL, TO_CHAR, FALSE);
      }
    }
    sprintf(buf, "You are allowed to link up to %d items.\n\r", get_link_limit(ch));
    send_to_char(buf, ch);
    return;
  }
  if(((obj = get_eq_char(ch, WEAR_HOLD)) == NULL) || obj->pIndexData->vnum != OBJ_VNUM_SPIRIT_LINK)
  {
    send_to_char("You must hold a spirit focus in order to link items.\n\r", ch);
    return;
  }

  if(ch->pcdata->quit_time > 0 && !IS_IMMORTAL (ch)
        && ch->in_room && ch->in_room->clan != ch->clan && ch->in_room->vnum >= 0)
  {
    send_to_char("Things are getting interesting, wait a few ticks or get to your hall.\n\r", ch);
    return;
  }
  for(curlink = 0; curlink < LINK_MAX; curlink++)
  {
    if(ch->pcdata->linked[curlink] == NULL)
      break;
  }
  if(curlink >= get_link_limit(ch))
  {
    if(!curlink)
      send_to_char("You are not allowed to link items.\n\r", ch);
    else
      send_to_char("You can't link any more items, unlink some first.\n\r", ch);
    return;
  }
  /* Find the object, make sure it's a linkable type (Use repair code + floats) and not already linked */
  if((obj = get_obj_here( ch, argument )) == NULL || obj->carried_by != ch)
  {
    send_to_char("You must be carrying or wearing the object you wish to link.\n\r", ch);
    return;
  }

  if(!str_cmp(obj->link_name, ch->name))
  {
    send_to_char("That object is already linked to you.\n\r", ch);
    return;
  }

  if(obj->stolen_timer)
  {
    char buf[256];
    sprintf(buf, "That object has changed hands too recently, please wait %d more ticks.\n\r", obj->stolen_timer);
    send_to_char(buf, ch);
    return;
  }

  if(obj->item_type != ITEM_WEAPON && obj->item_type != ITEM_CONTAINER &&
    obj->item_type != ITEM_ARMOR && obj->item_type != ITEM_CLOTHING
     && obj->item_type != ITEM_DRINK_CON &&
    obj->item_type != ITEM_JEWELRY && obj->item_type != ITEM_LIGHT &&
    obj->rarity == RARITY_GEM)
    {
      if(!obj->wear_flags || obj->wear_flags == ITEM_TAKE ||
        obj->wear_flags == (ITEM_TAKE | ITEM_HOLD))
      {
        send_to_char("You may not link that type of object.\n\r", ch);
        return;
      }
    }

  if(obj->rarity == RARITY_IMPOSSIBLE)
  {
//    send_to_char("That item is too powerful for you to link.\n\r", ch);
//    return;
    send_to_char("That item is so powerful it will shatter on every death.\n\r", ch);
  }
  /* Link it */
  clear_string(&obj->link_name, ch->name);
  ch->pcdata->linked[curlink] = obj;
  act("$p is now linked to you.", ch, obj, NULL, TO_CHAR, FALSE);

  /* Can be rot death or inventory flagged but it will naturally rot on death -- warn them */
  if(IS_SET(obj->extra_flags,(ITEM_ROT_DEATH | ITEM_INVENTORY)))
    send_to_char("That object will still be destroyed if you die.\n\r", ch);
  if(obj->item_type == ITEM_KEY)
    send_to_char("That object will still be destroyed if you log out without it equipped.\n\r", ch);
  if(obj->item_type == ITEM_CONTAINER)
    send_to_char("That object will empty its contents into the corpse if you die.\n\r", ch);
}

void unlink_item(CHAR_DATA *ch, OBJ_DATA *obj)
{
  int i;
  int mod = 0;
  mod = 0;
  for(i = 0; i < LINK_MAX; i++)
  {
    if(mod)
      ch->pcdata->linked[i - mod] = ch->pcdata->linked[i];
    else if(ch->pcdata->linked[i] == obj)
      mod = 1;
  }
  if(mod)
    ch->pcdata->linked[i - 1] = NULL;/* Clear the last */
  else
    bug("Failed to locate item to unlink.", 0);
  clear_string(&obj->link_name, NULL);
}

// WORKING HERE
void do_unlink(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *obj;
  char arg[256];
  int dam_amount = 0;
  bool override = FALSE;
  if(IS_NPC(ch))
    return;
  if(IS_SET(ch->mhs, MHS_GLADIATOR))
  {
    send_to_char("Gladiators can't unlink items.\n\r", ch);
    return;
  }

  if(ch->pcdata->linked[0] == NULL)
  {
    send_to_char("You have no items linked to unlink.\n\r", ch);
    return;
  }
  if(((obj = get_eq_char(ch, WEAR_HOLD)) == NULL) || obj->pIndexData->vnum != OBJ_VNUM_SPIRIT_LINK)
  {
    send_to_char("You must hold a spirit focus in order to unlink items.\n\r", ch);
    return;
  }

  argument = one_argument(argument, arg);

  /* Find the object, make sure it's linked, unlink and damage it */
  if((obj = get_obj_here( ch, arg )) == NULL || obj->carried_by != ch)
  {
    send_to_char("You must be carrying or wearing the object you wish to unlink.\n\r", ch);
    return;
  }
  if(!obj->link_name || str_cmp(obj->link_name, ch->name))
  {
    act("$p is not linked to you.\n\r", ch, obj, NULL, TO_CHAR, FALSE);
//    send_to_char("That item is not linked to you.\n\r", ch);
    return;
  }
  if(argument[0] != '\0' && !str_prefix(argument, "override"))
    override = TRUE;
  switch(obj->rarity)
  {
    default: dam_amount = 3; break;/* Stock or unknown */
    case RARITY_COMMON: dam_amount = 3; break;
    case RARITY_UNCOMMON: dam_amount = 10; break;
    case RARITY_RARE: dam_amount = 20; break;
    case RARITY_IMPOSSIBLE: dam_amount = 50; break;
  }
  if(!override && obj->damaged <= 100)
  {
    if(obj->damaged == 100)
    {
      send_to_char("That object will be shattered unless you repair it before unlinking.\n\rUse 'unlink <name> override' to unlink anyway.\n\r", ch);
      return;
    }
    else if(obj->damaged + dam_amount >= 100)
    {
      send_to_char("That object will be broken unless you repair it before unlinking.\n\rUse 'unlink <name> override' to unlink anyway.\n\r", ch);
      return;
    }
  }
  if(obj->damaged == 100)
  {
    obj->damaged = 1000;
    act("$p has been unlinked but shattered from the strain.", ch, obj, NULL, TO_CHAR, FALSE);
  }
  else
  {
    if(obj->damaged <= 100)
      obj->damaged = UMIN(100, obj->damaged + dam_amount);
    if(obj->damaged == 100)
      act("$p has been unlinked but broke from the strain.", ch, obj, NULL, TO_CHAR, FALSE);
    else if(obj->damaged < 100)
      act("$p has been unlinked but was damaged by the strain.", ch, obj, NULL, TO_CHAR, FALSE);
    else /* Already shattered, doesn't take further damage */
      act("$p has been unlinked.", ch, obj, NULL, TO_CHAR, FALSE);
  }
  unlink_item(ch, obj);
}

void do_linksafe(CHAR_DATA *ch, char *argument)
{
  if(IS_NPC(ch))
    return;
  TOGGLE_BIT(ch->pcdata->new_opt_flags, OPT_NOSAFELINK);
  if(!IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    send_to_char("Your linked items are now protected from permanent destruction.\n\r", ch);
  else
    send_to_char("You may now risk permanent destruction of your linked items.\n\r", ch);
}

void do_offer(CHAR_DATA *ch, char *argument)
{
  OBJ_DATA *container;

  if(IS_NPC(ch))
    return;
  
  if(!ch->pcdata->clan_info)
  {
    send_to_char("You must be in a new clan to offer a corpse.\n\r", ch);
    return;
  }
  
  if( is_affected(ch,skill_lookup("wraithform")) )
  {
  send_to_char("The gods ignore your offer because you're in wraithform.\n\r",ch);
  return;
  }

  if (argument[0] == '\0')
  {
      send_to_char( "Offer what?\n\r", ch );
      return;
  }

  if ( ( container = get_obj_here( ch, argument ) ) == NULL )
  {
    act( "You see no $T here.", ch, NULL, argument, TO_CHAR ,FALSE);
    return;
  }

  switch ( container->item_type )
  {
    default:
    send_to_char( "You can only offer a clanner's corpse.\n\r", ch );
    return;

    case ITEM_CONTAINER:
    case ITEM_CORPSE_NPC:
    {
      send_to_char( "You can only offer a clanner's corpse.\n\r", ch );
      return;
    }

    case ITEM_CORPSE_PC:
    {
      if (!can_loot(ch,container,TRUE))
      {
        send_to_char( "You're not allowed to offer that.\n\r", ch );
        return;
      }
    }

    if(IS_SET(container->extra_flags,ITEM_CLAN_CORPSE) && is_clan(ch))
    {
      char buf[256];
      int amount;
      CHAR_DATA *owner = NULL;
      DESCRIPTOR_DATA *d;
      OBJ_DATA *looting, *prev;
      DAMAGE_DATA *loot_prev = NULL;
      DAMAGE_DATA *loot = container->loot_track;
      if(!loot)
      {
        send_to_char("That corpse has no items left in it, there's no point in offering it.\n\r", ch);
        return;
      }
      if(container->owner)
      {/* Check for a friendly return attempt */
        if(!str_cmp(container->owner, ch->name))
        {
          send_to_char("You can't offer your own corpse.\n\r", ch);
          return;
        }
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
          owner = d->original ? d->original : d->character;

          if (d->connected == CON_PLAYING && owner && owner->pcdata &&
               owner != ch && !str_cmp(owner->name, container->owner))
            break;
        }
        if(is_clan_friendly(owner, ch))
        {
          act("$N is an ally, you can't offer $S corpse.", ch, NULL, owner, TO_CHAR, FALSE);
          return;
        }
      }

      for(loot = container->loot_track; loot; loot = loot->next)
      {/* ~ means it was a mob kill - illegal name, can't be accidentally created */
        if(!str_cmp(loot->source, ch->name))
          break;
        loot_prev = loot;
      }
      if(!loot)
      {
        send_to_char("You must have items available in a corpse to be able to offer it.\n\r", ch);
        return;
      }
      if(IS_SET(loot->type, RARITY_LOOTED))
      {
        send_to_char("You can't offer a corpse after looting!\n\r", ch);
        return;
      }
      /* Offer the corpse */
      amount = (owner->level * owner->level) / 20;
      amount = (amount * number_range(95, 105)) / 100; /* A little variance */
      /* Reduce for frequent dying - one hour to full bonus */
      if(current_time - owner->pcdata->last_death_date < 3600)
        amount = UMAX(1, amount * (current_time - owner->pcdata->last_death_date) / 3600);
/* NOTE: Bonus merit is not being included at this time, but the framework is left in
         to make it easier to add later if desired */
      if(!loot->type && ch->pcdata->clan_info->clan->default_clan != CLAN_OUTCAST)
      {/* Straight into the bank, they aren't an outcast and didn't search */
        int bonus_merit = 0;
        /* CALCULATE BONUS MERIT HERE */
        if(!ch->pcdata->clan_info->clan->default_clan)
        {
          ch->pcdata->clan_info->donated += amount + bonus_merit;
          ch->pcdata->clan_info->clan->tribute += amount + bonus_merit;
          sprintf(buf, "%d merit has been donated for your offering.\n\r", amount);
        }
        else
        {
          ch->pcdata->clan_info->banked_merit += amount + bonus_merit;
          sprintf(buf, "%d merit has been stored for your offering.\n\r", amount);
        }
        send_to_char(buf, ch);
        /* TELL THEM ABOUT THEIR BONUS MERIT HERE */
      }
      else
      {/* Into their merit pool */
        int bonus_merit = 0;
        /* CALCULATE BONUS MERIT HERE */
        ch->pcdata->clan_info->merit += amount + bonus_merit;
        sprintf(buf, "You receive %d merit for your offering.\n\r", amount);
        send_to_char(buf, ch);
        /* TELL THEM ABOUT THEIR BONUS MERIT HERE */
      }
      /* Can't loot this in the future */
      if(loot_prev)
        loot_prev->next = loot->next;
      else
      {
        container->loot_track = loot->next;
        if(!container->loot_track)
          return_clan_gear(owner, container);
      }
      free_damage(loot);
    }
    break;
  }
}

void do_search(CHAR_DATA *ch, char *argument)
{
  int index;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  OBJ_DATA *container;
  /* Default search */
  int search_type = RARITY_UNCOMMON | RARITY_RARE | RARITY_GEM | RARITY_IMPOSSIBLE;

  if( is_affected(ch,skill_lookup("wraithform")) )
  {
  send_to_char("You can't rummage around the corpse while in wraithform.\n\r",ch);
  return;
  }

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  /* Get type. */
  if (arg1[0] == '\0')
  {
      send_to_char( "Search what?\n\r", ch );
      return;
  }

  if(arg2[0] != '\0')
  {
    int basic_type, lore_type;
    /* Lore improves the accuracy of your filter */
    if(!str_prefix(arg2, "all"))
      basic_type = lore_type = RARITY_ALL;
    else if(!str_prefix(arg2, "stock"))
    {
        lore_type = RARITY_STOCK;
        basic_type = RARITY_STOCK | RARITY_COMMON;
    }
    else if(!str_prefix(arg2, "common"))
    {
        lore_type = RARITY_COMMON;
        basic_type = RARITY_STOCK | RARITY_COMMON;
    }
    else if(!str_prefix(arg2, "uncommon"))
    {
        lore_type = RARITY_UNCOMMON;
        basic_type = RARITY_UNCOMMON | RARITY_RARE | RARITY_IMPOSSIBLE;
    }
    else if(!str_prefix(arg2, "rare"))
    {
        lore_type = RARITY_RARE | RARITY_IMPOSSIBLE;
        basic_type = RARITY_UNCOMMON | RARITY_RARE | RARITY_IMPOSSIBLE;
    }
    else if(!str_prefix(arg2, "gem"))
        lore_type = basic_type = RARITY_GEM;
    else
    {
      send_to_char("Unknown type. Optional types are all, stock, common, uncommon, rare or gem.\n\r", ch);
      return;
    }
    if(basic_type != lore_type && get_skill(ch,gsn_lore) > 1 && number_percent() < get_skill(ch,gsn_lore))
    {
      search_type = lore_type;
      send_to_char("Your knowledge of lore provides insight into the value of the items.\n\r", ch);
    }
    else
      search_type = basic_type;
  }
  /* Left at default search otherwise */

  if ( ( container = get_obj_here( ch, arg1 ) ) == NULL )
  {
    act( "You see no $T here.", ch, NULL, arg1, TO_CHAR ,FALSE);
    return;
  }

  switch ( container->item_type )
  {
    default:
    send_to_char( "That's not a container.\n\r", ch );
    return;

    case ITEM_CONTAINER:
    case ITEM_CORPSE_NPC:
    {
      send_to_char( "You can only search a clanner's corpse.\n\r", ch );
      return;
    }

    case ITEM_CORPSE_PC:
    {
      if (!can_loot(ch,container,TRUE))
      {
        send_to_char( "You're not allowed to search that.\n\r", ch );
        return;
      }
    }

    if(IS_SET(container->extra_flags,ITEM_CLAN_CORPSE) && is_clan(ch) && !IS_IMMORTAL(ch))
    {
      CHAR_DATA *owner = NULL;
      DESCRIPTOR_DATA *d;
      OBJ_DATA *looting, *prev;
      DAMAGE_DATA *loot_prev;
      DAMAGE_DATA *loot = container->loot_track;
      if(!loot)
      {
        send_to_char("That corpse has no items left in it, there's no point in searching.\n\r", ch);
        return;
      }
      if(container->owner)
      {/* Check for a friendly return attempt */
        if(!str_cmp(container->owner, ch->name))
        {
          send_to_char("You don't need to search your own corpse, use 'look'.\n\r", ch);
          return;
        }
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
          owner = d->original ? d->original : d->character;

          if (d->connected == CON_PLAYING && owner && owner->pcdata &&
               owner != ch && !str_cmp(owner->name, container->owner))
            break;
        }
        if(owner && is_clan_friendly(owner, ch))
        {
          act("$N is an ally, use loot instead to help return $S gear faster.", ch, NULL, owner, TO_CHAR, FALSE);
          return;
        }
      }

      for(loot = container->loot_track; loot; loot = loot->next)
      {/* ~ means it was a mob kill - illegal name, can't be accidentally created */
        if(!str_cmp(loot->source, ch->name) || !str_cmp(loot->source, "~"))
          break;
        loot_prev = loot;
      }
      if(!loot)
      {
        send_to_char("There are no items available for you in that corpse.\n\r", ch);
        return;
      }
      else
      {
        char buf[MAX_STRING_LENGTH];
         BUFFER *output;
         char **prgpstrShow;
         int *prgnShow;
         char *pstrShow;
         OBJ_DATA *obj;
         int nShow;
         int iShow;
         int count;
         bool fCombine;

          /* Allow looting, but don't lose the looted tag */
         loot->type = (loot->type & RARITY_LOOTED) | search_type;
         /*
          * Alloc space for output lines.
          */
         output = new_buf();

         count = 0;
        for(looting = container->loot_next; looting; looting = looting->loot_next)
        {
          if((IS_SET(looting->rarity, search_type) || looting->item_type == ITEM_MONEY) && can_see_obj(ch, looting))
            count++;
        }
    #ifdef OLC_VERSION
         prgpstrShow = alloc_mem( count * sizeof(char *) );
         prgnShow    = alloc_mem( count * sizeof(int)    );
    #else
         prgpstrShow = GC_MALLOC( count * sizeof(char *) );
         prgnShow    = GC_MALLOC( count * sizeof(int)    );
    #endif
         nShow       = 0;

         /*
          * Format the list of objects.
          */
        for(looting = container->loot_next; looting; looting = looting->loot_next)
        {
          if((IS_SET(looting->rarity, search_type) || looting->item_type == ITEM_MONEY) && can_see_obj(ch, looting))
          {
           pstrShow = format_obj_to_char( looting, ch, TRUE );

           fCombine = FALSE;

           if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE))
           {
         /*
          * Look for duplicates, case sensitive.
          * Matches tend to be near end so run loop backwords.
          */
         for ( iShow = nShow - 1; iShow >= 0; iShow-- )
         {
             if ( !strcmp( prgpstrShow[iShow], pstrShow ) )
             {
           prgnShow[iShow]++;
           fCombine = TRUE;
           break;
             }
         }
           }

           /*
            * Couldn't combine, or didn't want to.
            */
           if ( !fCombine )
           {
         prgpstrShow [nShow] = str_dup( pstrShow );
         prgnShow    [nShow] = 1;
         nShow++;
           }
       }
         }

         /*
          * Output the formatted list.
          */
        for ( iShow = 0; iShow < nShow; iShow++ )
        {
          if (prgpstrShow[iShow][0] == '\0')
          {
              free_string(prgpstrShow[iShow]);
              continue;
          }

          if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE ))
          {
            if ( prgnShow[iShow] != 1 )
            {
              sprintf( buf, "(%2d) ", prgnShow[iShow] );
              add_buf(output,buf);
            }
            else
            {
              add_buf(output,"     ");
            }
          }
          add_buf(output,prgpstrShow[iShow]);
          add_buf(output,"\n\r");
          free_string( prgpstrShow[iShow] );
        }

        if ( nShow == 0 )
        {
          if ( IS_NPC(ch) || IS_SET(ch->display, DISP_COMBINE))
            send_to_char( "     ", ch );
          send_to_char( "Nothing.\n\r", ch );
        }
        /* 32000 is the actual limit, this provides a little buffer for prompt, etc. */
        if(strlen(buf_string(output)) >= 30000)/* Only page it if it would disconnect them otherwise */
          page_to_char(buf_string(output),ch);
        else
          send_to_char(buf_string(output), ch);

        /*
         * Clean up.
         */
        free_buf(output);
        free_mem( prgpstrShow, count * sizeof(char *) );
        free_mem( prgnShow,    count * sizeof(int)    );
        return;
      }
    }
    break;
  }
}

void do_loot( CHAR_DATA *ch, char *argument )
{
  int index;
  char arg1[MAX_INPUT_LENGTH];
  char arg2[MAX_INPUT_LENGTH];
  char orig[MAX_INPUT_LENGTH];
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  OBJ_DATA *container;
  bool found;

  if( is_affected(ch,skill_lookup("wraithform")) )
  {
  send_to_char("Not while in wraithform.\n\r",ch);
  return;
  }

  strcpy(orig, argument);

  argument = one_argument( argument, arg1 );
  argument = one_argument( argument, arg2 );

  if (!str_cmp(arg2,"from"))
     argument = one_argument(argument,arg2);

  index = number_argument(arg1,arg1);

  if(index <= 0)
    index = 1;

  /* Get type. */
  if ( arg1[0] == '\0' || arg2[0] == '\0' )
  {
      send_to_char( "Loot what?\n\r", ch );
      return;
  }

  if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
  {
    act( "You see no $T here.", ch, NULL, arg2, TO_CHAR ,FALSE);
    return;
  }

  switch ( container->item_type )
  {
    default:
    send_to_char( "That's not a container.\n\r", ch );
    return;

    case ITEM_CONTAINER:
    case ITEM_CORPSE_NPC:
    {
      send_to_char( "Use 'get'.\n\r", ch );
      return;
    }

    case ITEM_CORPSE_PC:
    {
      if (!can_loot(ch,container,TRUE))
      {
        send_to_char( "You can't do that.\n\r", ch );
        return;
      }
    }

    if(IS_SET(container->extra_flags,ITEM_CLAN_CORPSE) && is_clan(ch) && !IS_IMMORTAL(ch))
    {
      CHAR_DATA *owner = NULL;
      DESCRIPTOR_DATA *d;
      OBJ_DATA *looting, *prev;
      DAMAGE_DATA *loot_prev = NULL;
      DAMAGE_DATA *loot = container->loot_track;
      if(!loot)
      {
        send_to_char("That corpse has no items left in it.\n\r", ch);
        return;
      }
      if(container->owner)
      {/* Check for a friendly return attempt */
        if(!str_cmp(container->owner, ch->name))
        {
          do_get(ch, orig);/* Do get handles this because it doesn't do the unusual search */
          return;
        }
        for ( d = descriptor_list; d != NULL; d = d->next )
        {
          owner = d->original ? d->original : d->character;

          if (d->connected == CON_PLAYING && owner->pcdata &&
               owner != ch && !str_cmp(owner->name, container->owner))
            break;
        }
        if(is_clan_friendly(owner, ch))
        {
          if(!container->value[4])
          {/* An ally can send the corpse back to you */
            container->value[4] = -2;
          }
          else
          {
            if(container->value[4] > 0)
              container->value[4]--;
            else if(container->value[4] < 0)/* Less than 0 doesn't auto-decrement */
              container->value[4]++;
          }
          WAIT_STATE(ch, PULSE_VIOLENCE * 2);
          if(!container->value[4])
          {
            act("You have returned the gear to $N.", ch, NULL, owner, TO_CHAR, FALSE);
            return_clan_gear(owner, container);
            return;
          }
          if(container->value[4] > 0)/* Auto decrementing */
            act("You accelerate the gear return to $N.", ch, NULL, owner, TO_CHAR, FALSE);
          else /* Manual only */
            act("You try to return the gear to $N, but it's not ready yet.", ch, NULL, owner, TO_CHAR, FALSE);
          return;
        }
      }

      for(loot = container->loot_track; loot; loot = loot->next)
      {/* ~ means it was a mob kill - illegal name, can't be accidentally created */
        if(!str_cmp(loot->source, ch->name) || !str_cmp(loot->source, "~"))
          break;
        loot_prev = loot;
      }
      if(!loot)
      {
        send_to_char("There are no items available for you in that corpse.\n\r", ch);
        return;
      }
      if(!str_cmp(arg1, "all"))
      {
        send_to_char("Don't be so greedy!\n\r", ch);
        return;
      }
      if(str_cmp( arg1, "coins" ) && str_cmp( arg1, "coin" ))/* &&
        str_cmp( arg1, "gold"  ) && str_cmp( arg1, "silver"))*/
      {/* Not just taking coins, there must be a search type */
        int found = 0;
        if(!loot->type)
        {
          send_to_char("You must search the victim's corpse before you may loot anything besides '{Ycoin{x'.\n\r", ch);
          return;
        }
        prev = NULL;
        for(looting = container->loot_next; looting; looting = looting->loot_next)
        {
        /* Check if rarity matches, then check if name matches, then check if count is equal */
          if(IS_SET(looting->rarity, loot->type) && can_see_obj(ch, looting) && is_name(arg1, looting->name))
          {
            found++;
            if(found == index)
            {
              OBJ_DATA *prev_in = looting->in_obj;
              get_obj(ch, looting, container);
              if(looting->carried_by != ch || !prev_in)
                return; /* Fail */
              loot->type |= RARITY_LOOTED;
              if(!prev)
                container->loot_next = looting->loot_next;
              else
                prev->loot_next = looting->loot_next;/* Drop it from the loot list */

              while(looting->contains)
              {
                OBJ_DATA *moving = looting->contains;
                obj_from_obj(looting->contains);
                obj_to_obj(moving, prev_in);
              }

              looting->stolen_timer = UMAX(looting->stolen_timer, 45);

              /* Decrement the available items to take if it's not a gem */
              if(looting->rarity != RARITY_GEM)
              {
                if(loot->duration > 0)
                {
                  loot->duration--;/* From one of the unique ones for this character */
                  if(!loot->duration && !loot->damage)
                  {/* Drop an empty one */
                    if(!loot_prev)
                      container->loot_track = loot->next;
                    else
                      loot_prev->next = loot->next;
                    free_damage(loot);
                  }
                }
                else
                {/* Decrement everyone's damage */
                  DAMAGE_DATA *loot_next;
                  loot_prev = NULL;
                  for(loot = container->loot_track; loot; loot = loot_next)
                  {
                    loot_next = loot->next;
                    loot->damage = UMAX(loot->damage - 1, 0);
                    /* Drop empty ones */
                    if(!loot->damage && !loot->duration)
                    {
                      if(!loot_prev)
                        container->loot_track = loot->next;
                      else
                        loot_prev->next = loot->next;
                      free_damage(loot);
                      continue;
                    }
                    loot_prev = loot;
                  }
                }
                if((!container->loot_track || !container->contains) && owner)
                {
                  send_to_char("The corpse has no items left for you to loot.\n\r", ch);
                  return_clan_gear(owner, container);
                }
              }
              break;
            }
          }
          prev = looting;
        }
        return;
      }
      else
      {/* Just taking coins */
        obj = get_obj_list( ch, arg1, container->contains );
        if(obj == NULL || obj->item_type != ITEM_MONEY)
          send_to_char("There are no coins in the corpse for you to take.\n\r", ch);
        else
        {/* Remove the coins from the loot list */
          prev = NULL;
          for(looting = container->loot_next; looting; looting = looting->loot_next)
          {
            if(looting == obj)
            {
              if(!prev)
                container->loot_next = looting->loot_next;
              else
                prev->loot_next = looting->loot_next;
              looting->loot_next = NULL;
              break;
            }
            prev = looting;
          }
          get_obj( ch, obj, container );
          if(!container->contains && owner)
          {
            send_to_char("The corpse has no more items left for you to loot.\n\r", ch);
            return_clan_gear(owner, container);
          }
        }
        return;
      }

      return;
    }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
    /* 'get obj container' */
    obj = get_obj_list( ch, arg1, container->contains );
    if ( obj == NULL )
    {
  act( "I see nothing like that in the $T.",
      ch, NULL, arg2, TO_CHAR ,FALSE);
  return;
    }
/* Loot counter DISABLED by Nightdagger on 04/07/03

    if (  obj->item_type != ITEM_GEM && obj->item_type != ITEM_JEWELRY
       && obj->item_type != ITEM_PILL && obj->item_type != ITEM_COMPONENT
       && obj->item_type != ITEM_KEY && obj->item_type != ITEM_TREASURE
       && obj->item_type != ITEM_SCROLL && obj->item_type != ITEM_WAND
       && obj->item_type != ITEM_STAFF && obj->item_type != ITEM_POTION
       && obj->item_type != ITEM_TRAP && obj->item_type != ITEM_GRENADE
       && obj->item_type != ITEM_MONEY && obj->item_type != ITEM_TRASH
       && obj->item_type != ITEM_CONTAINER && !can_loot(ch,container,FALSE) )
    {
      if(container->value[4] > 0)
      {
        container->value[4]--;
      }
      else
      {
        send_to_char("This one has been picked clean already.\n\r",ch);
        return;
      }
    }

*/
    /* found something to grab, lag'em a bit */
    WAIT_STATE( ch, 12);
    get_obj( ch, obj, container );
    /* Half an hour if not the owner */
}
else
{
    /* 'get all container' or 'get all.obj container' */
    found = FALSE;
    for ( obj = container->contains; obj != NULL; obj = obj_next )
    {
  obj_next = obj->next_content;
  if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
  &&   can_see_obj( ch, obj ) )
  {
      found = TRUE;
      if( (container->pIndexData->vnum == OBJ_VNUM_PIT
              &&  !IS_IMMORTAL(ch))
         || (!can_loot(ch,container,FALSE)
              && !IS_IMMORTAL(ch)) )
      {
        send_to_char("Don't be so greedy!\n\r",ch);
        return;
      }
      get_obj( ch, obj, container );
      //obj->stolen_timer += 10 * number_fuzzy(5);
  }
    }

    if ( !found )
    {
  if ( arg1[3] == '\0' )
      act( "You see nothing in the $T.",
    ch, NULL, arg2, TO_CHAR ,FALSE);
  else
      act( "You see nothing like that in the $T.",
    ch, NULL, arg2, TO_CHAR ,FALSE);
    }
}
  }

  return;
}

void do_get( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *victim;
    CHAR_DATA *victim_next;
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;
    bool corpsefound = FALSE;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if (!str_cmp(arg2,"from"))
  argument = one_argument(argument,arg2);

    /* Get type. */
    if ( arg1[0] == '\0' )
    {
  send_to_char( "Get what?\n\r", ch );
  return;
    }

    if ( arg2[0] == '\0' )
    {
  if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
  {
      /* 'get obj' */
      obj = get_obj_list( ch, arg1, ch->in_room->contents );
      if ( obj == NULL )
      {
    act( "I see no $T here.", ch, NULL, arg1, TO_CHAR ,FALSE);
    return;
      } /* help hide items with no long description */
        else if ( (strlen(obj->description) < 1 || strlen(arg1) < 2) &&
                !IS_SET(obj->wear_flags,ITEM_TAKE)  )
        {
        act("I see no $T here.", ch, NULL, arg1, TO_CHAR, FALSE);
        return;
        }

      /* Only Clanmates and Groupmates or Yourself can get your corpse */
      if (obj->item_type == ITEM_CORPSE_PC && obj->owner != NULL
          && !IS_IMMORTAL(ch))
      {
         found = FALSE;
         for ( victim = char_list; victim != NULL; victim = victim_next )
         {
            victim_next  = victim->next;
            if (!str_cmp(obj->owner,victim->name))
            {
               found = TRUE;
               break;
            }
         }

         if(found && victim != ch && obj->contains != NULL)
         {
            if (!is_same_group( victim, ch ))
            {
               if(!is_clan_friendly(victim, ch)
                  || (ch->clan == victim->clan && (ch->level > victim->level+8
                  || ch->level +8 < victim->level)))
               {
                  send_to_char("You are not allowed to get the corpse.\n\r",ch);
                  return;
               }
            }
         }
      }

      get_obj( ch, obj, NULL );
  }
  else
  {
      /* 'get all' or 'get all.obj' */
      found = FALSE;
      for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
      {
         obj_next = obj->next_content;
         if ( strlen(obj->description) < 1 && !IS_SET(obj->wear_flags,ITEM_TAKE) ) /* hide items with no long descr */
                continue;
         if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
               &&   can_see_obj( ch, obj ) )
         {
            /* Only immortals can get your corpse */
            corpsefound = FALSE;
            if (obj->item_type == ITEM_CORPSE_PC && !IS_IMMORTAL(ch))
            {
               /*found = FALSE;
               for ( victim = char_list; victim != NULL; victim = victim_next )
               {
                  victim_next  = victim->next;
                  if (is_name(obj->owner,victim->name))
                  {
                     found = TRUE;
                     break;
                  }
               }

              if(found && victim != ch)
              {
                 if (!is_same_group( victim, ch ))
                 {
                    if(ch->clan != victim->clan
                       || ch->clan == nonclan_lookup("loner")
                       || (ch->clan == victim->clan
                           && (ch->level > victim->level+8
                       || ch->level +8 < victim->level)))
                    {
                       corpsefound = TRUE;
                    }
                 }
              }*/
              if(obj->contains)
                corpsefound = TRUE;
           }

           found = TRUE;
           if (corpsefound)
              send_to_char("You are not allowed to get the corpse.\n\r",ch);
           else
              get_obj( ch, obj, NULL );
        }
     }

      if ( !found )
      {
    if ( arg1[3] == '\0' )
        send_to_char( "I see nothing here.\n\r", ch );
    else
        act( "I see no $T here.", ch, NULL, &arg1[4], TO_CHAR ,FALSE);
      }
  }
    }
    else
    {
  /* 'get ... container' */
  if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
  {
      send_to_char( "You can't do that.\n\r", ch );
      return;
  }

  if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
  {
      act( "I see no $T here.", ch, NULL, arg2, TO_CHAR ,FALSE);
      return;
  }

  switch ( container->item_type )
  {
  default:
      send_to_char( "That's not a container.\n\r", ch );
      return;

  case ITEM_CONTAINER:
  case ITEM_CORPSE_NPC:
  case ITEM_FORGE:
      break;

  case ITEM_CORPSE_PC:
      {
        if (container->owner != NULL)
        {
          if(str_cmp(container->owner,ch->name))
          {
            send_to_char( "Use 'loot'.\n\r", ch );
            return;
          }
          if(ch->pcdata && ch->pcdata->linked[0] && container->value[4])
          {
            bool found = FALSE;
            int i;
            for(i = 0; i < LINK_MAX; i++)
            {
              if(!ch->pcdata->linked[i])
                break;
              if(ch->pcdata->linked[i]->wear_loc == WEAR_HIDDEN)
              {
                found = TRUE;
                ch->pcdata->linked[i]->wear_loc = WEAR_NONE;
                if(ch->pcdata->linked[i]->damaged == 1000)
                  act("$p is {Rshattered{x and will cost extra to repair.", ch, ch->pcdata->linked[i], NULL, TO_CHAR, FALSE);
                else if(ch->pcdata->linked[i]->damaged == 100)
                {
                  if(!ch->pcdata || !ch->pcdata->clan_info ||
                    ch->pcdata->clan_info->clan->type != CLAN_TYPE_PEACE ||
                    ch->pcdata->clan_info->clan->tribute < 0 ||
                    ch->pcdata->clan_info->clan->max_tribute < 20000)
                    act("$p is {Rbroken{x and will shatter if you die again.", ch, ch->pcdata->linked[i], NULL, TO_CHAR, FALSE);
                  else
                    act("$p is {Rbroken{x.", ch, ch->pcdata->linked[i], NULL, TO_CHAR, FALSE);
                }
              }
            }
            if(found)
            {
              send_to_char("{WYour {Ylinked{W gear has returned to you.{x\n\r", ch);
              ch->pcdata->corpse_timer = 0;/* You can quit whenever now */
            }
          }
          container->value[4] = -3; /* Interacted with - automatic timer is disabled (Same as a mob kill now) */
        }
        break;
      }
  }


  if ( (IS_SET(container->value[1], CONT_CLOSED)) && container->item_type == ITEM_CONTAINER )
  {
      act( "The $d is closed.", ch, NULL, container->name, TO_CHAR ,FALSE);
      return;
  }

  if(container->item_type == ITEM_CONTAINER && container->damaged >= 100)
  {
        act("The $d is too damaged to get items out of.", ch, NULL, container->name, TO_CHAR, FALSE);
        return;
  }

  if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
  {
      /* 'get obj container' */
      obj = get_obj_list( ch, arg1, container->contains );
      if ( obj == NULL )
      {
    act( "I see nothing like that in the $T.",
        ch, NULL, arg2, TO_CHAR ,FALSE);
    return;
      }
      get_obj( ch, obj, container );
  }
  else
  {
      /* 'get all container' or 'get all.obj container' */
      found = FALSE;
      for ( obj = container->contains; obj != NULL; obj = obj_next )
      {
    obj_next = obj->next_content;
    if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
    &&   can_see_obj( ch, obj ) )
    {
        found = TRUE;
        if (container->pIndexData->vnum == OBJ_VNUM_PIT
        &&  !IS_IMMORTAL(ch))
        {
          send_to_char("Don't be so greedy!\n\r",ch);
          return;
        }
        get_obj( ch, obj, container );
    }
      }

      if ( !found )
      {
    if ( arg1[3] == '\0' )
        act( "I see nothing in the $T.",
      ch, NULL, arg2, TO_CHAR ,FALSE);
    else
        act( "I see nothing like that in the $T.",
      ch, NULL, arg2, TO_CHAR ,FALSE);
      }
  }
    if(container->item_type == ITEM_CORPSE_PC && container->contains && container->loot_track)
    {/* Must be this owner */
      container->loot_next = NULL;
      sort_clanner_items(ch, container->contains, &container->loot_next, FALSE);
      return;
    }
    }
}


void do_put( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *container;
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"in") || !str_cmp(arg2,"on"))
  argument = one_argument(argument,arg2);

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
  send_to_char( "Put what in what?\n\r", ch );
  return;
    }

    if ( !str_cmp( arg2, "all" ) || !str_prefix( "all.", arg2 ) )
    {
  send_to_char( "You can't do that.\n\r", ch );
  return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
  act( "I see no $T here.", ch, NULL, arg2, TO_CHAR ,FALSE);
  return;
    }

    if ( container->item_type != ITEM_CONTAINER && container->item_type != ITEM_FORGE)
    {
  send_to_char( "That's not a container.\n\r", ch );
  return;
    }

    if ( IS_SET(container->value[1], CONT_CLOSED) && container->item_type == ITEM_CONTAINER )
    {
  act( "The $d is closed.", ch, NULL, container->name, TO_CHAR ,FALSE);
  return;
    }

  if(container->item_type == ITEM_CONTAINER && container->damaged >= 100)
  {
        act("The $d is too damaged to put items into.", ch, NULL, container->name, TO_CHAR, FALSE);
        return;
  }

    if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
    {
  /* 'put obj container' */
  if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
  {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
  }

  if ( obj == container )
  {
      send_to_char( "You can't fold it into itself.\n\r", ch );
      return;
  }

  if ( !can_drop_obj( ch, obj ) )
  {
      send_to_char( "You can't let go of it.\n\r", ch );
      return;
  }

      if (WEIGHT_MULT(obj) != 100)
      {
           send_to_char("You have a feeling that would be a bad idea.\n\r",ch);
            return;
        }

  if ((get_obj_weight( obj ) + get_true_weight( container )
       > (container->value[0] * 10)
  ||  get_obj_weight(obj) > (container->value[3] * 10)) && container->item_type == ITEM_CONTAINER)
  {
      send_to_char( "It won't fit.\n\r", ch );
      return;
  }

  if (!CAN_WEAR(container,ITEM_TAKE) &&
    (container->pIndexData->vnum == OBJ_VNUM_PIT || container->pIndexData->vnum == OBJ_VNUM_MATOOK_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_DEMISE_PIT || container->pIndexData->vnum == OBJ_VNUM_HONOR_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_POSSE_PIT || container->pIndexData->vnum == OBJ_VNUM_ZEALOT_PIT ||
    container->pIndexData->vnum == OBJ_VNUM_WARLOCK_PIT || container->pIndexData->vnum == OBJ_VNUM_FLUZEL_PIT ||
    (container->pIndexData->vnum < 0 && container->pIndexData->item_type == ITEM_CONTAINER)))
  {
    int obj_count = 0;
    OBJ_DATA *obj_check = container->contains;
    OBJ_DATA *last_obj = NULL;
    for(;obj_check;obj_check = obj_check->next_content)
    {
      if(!obj_check->timer)
      {
        obj_count++;
        last_obj = obj_check;
      }
    }
    if(obj_count > 50 && last_obj)
      last_obj->timer = 2200;/* Bit over 24 hours */
      if (obj->timer)
    SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
      //else if(container->pIndexData->vnum == OBJ_VNUM_PIT)
      //    obj->timer = 200;
//      else
//          obj->timer = 6500;//Around 72 hours
   }

  obj_from_char( obj );
  obj_to_obj( obj, container );

  if (IS_SET(container->value[1],CONT_PUT_ON) && container->item_type == ITEM_CONTAINER)
  {
      act("$n puts $p on $P.",ch,obj,container, TO_ROOM,FALSE);
      act("You put $p on $P.",ch,obj,container, TO_CHAR,FALSE);
  }
  else
  {
      act( "$n puts $p in $P.", ch, obj, container, TO_ROOM ,FALSE);
      act( "You put $p in $P.", ch, obj, container, TO_CHAR ,FALSE);
  }
    }
    else
    {
      /* Use for pits only */
      int num_added = 0;
  /* 'put all container' or 'put all.obj container' */
  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
      obj_next = obj->next_content;

      if ( (( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
      &&   can_see_obj( ch, obj )
      &&   WEIGHT_MULT(obj) == 100
      &&   obj->wear_loc == WEAR_NONE
      &&   obj != container
      &&   can_drop_obj( ch, obj )
      &&   get_obj_weight( obj ) + get_true_weight( container )
     <= (container->value[0] * 10)
      &&   get_obj_weight(obj) <= (container->value[3] * 10)) && container->item_type == ITEM_CONTAINER)
      {
          if (!CAN_WEAR(container,ITEM_TAKE) &&
            (container->pIndexData->vnum == OBJ_VNUM_PIT || container->pIndexData->vnum == OBJ_VNUM_MATOOK_PIT ||
            container->pIndexData->vnum == OBJ_VNUM_DEMISE_PIT || container->pIndexData->vnum == OBJ_VNUM_HONOR_PIT ||
            container->pIndexData->vnum == OBJ_VNUM_POSSE_PIT || container->pIndexData->vnum == OBJ_VNUM_ZEALOT_PIT ||
            container->pIndexData->vnum == OBJ_VNUM_WARLOCK_PIT || container->pIndexData->vnum == OBJ_VNUM_FLUZEL_PIT ||
            (container->pIndexData->vnum < 0 && container->pIndexData->item_type == ITEM_CONTAINER)))
        {
       if (obj->timer)
      SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
       else
         num_added++;
         //   else if(container->pIndexData->vnum == OBJ_VNUM_PIT)
         //     obj->timer = 200;
         //   else
         //     obj->timer = 6500;// Slightly over 72 hours
         }
    obj_from_char( obj );
    obj_to_obj( obj, container );

          if (IS_SET(container->value[1],CONT_PUT_ON))
          {
                  act("$n puts $p on $P.",ch,obj,container, TO_ROOM,FALSE);
                  act("You put $p on $P.",ch,obj,container, TO_CHAR,FALSE);
          }
    else
    {
        act( "$n puts $p in $P.", ch, obj, container, TO_ROOM ,FALSE);
        act( "You put $p in $P.", ch, obj, container, TO_CHAR ,FALSE);
    }
      }
  }
  if(num_added > 0)
  {/* It's a pit and multiple items were added to it */
      int obj_count = 0;
      OBJ_DATA *obj_check = container->contains;
      for(; obj_check; obj_check = obj_check->next_content)
      {
        if(!obj_check->timer)
          obj_count++;
      }
      if(obj_count > 50)
      {
        obj_count = 50;
        for(obj_check = container->contains; obj_check; obj_check = obj_check->next_content)
        {
          if(!obj_check->timer)
          {
            if(obj_count <= 0)
              obj_check->timer = 2200;
            else
              obj_count--;
          }
        }
      }
  }
    }

    return;
}



void do_drop( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    bool found;

    argument = one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Drop what?\n\r", ch );
  return;
    }

    if ( is_number( arg ) )
    {
  /* 'drop NNNN coins' */
  int amount, gold = 0, silver = 0;

  amount   = atoi(arg);
  argument = one_argument( argument, arg );
  if ( amount <= 0
  || ( str_cmp( arg, "coins" ) && str_cmp( arg, "coin" ) &&
       str_cmp( arg, "gold"  ) && str_cmp( arg, "silver") ) )
  {
      send_to_char( "Sorry, you can't do that.\n\r", ch );
      return;
  }

  if ( !str_cmp( arg, "coins") || !str_cmp(arg,"coin")
  ||   !str_cmp( arg, "silver"))
  {
      if (ch->silver < amount)
      {
    send_to_char("You don't have that much silver.\n\r",ch);
    return;
      }

      ch->silver -= amount;
      silver = amount;
  }

  else
  {
      if (ch->gold < amount)
      {
    send_to_char("You don't have that much gold.\n\r",ch);
    return;
      }

      ch->gold -= amount;
        gold = amount;
  }

  for ( obj = ch->in_room->contents; obj != NULL; obj = obj_next )
  {
      obj_next = obj->next_content;

      switch ( obj->pIndexData->vnum )
      {
      case OBJ_VNUM_SILVER_ONE:
    silver += 1;
    extract_obj(obj);
    break;

      case OBJ_VNUM_GOLD_ONE:
    gold += 1;
    extract_obj( obj );
    break;

      case OBJ_VNUM_SILVER_SOME:
    silver += obj->value[0];
    extract_obj(obj);
    break;

      case OBJ_VNUM_GOLD_SOME:
    gold += obj->value[1];
    extract_obj( obj );
    break;

      case OBJ_VNUM_COINS:
    silver += obj->value[0];
    gold += obj->value[1];
    extract_obj(obj);
    break;
      }
  }

  obj_to_room( create_money( gold, silver ), ch->in_room );
  act( "$n drops some coins.", ch, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char( "OK.\n\r", ch );
  return;
    }

    if ( str_cmp( arg, "all" ) && str_prefix( "all.", arg ) )
    {
  /* 'drop obj' */
  if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
  {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
  }

  if ( !can_drop_obj( ch, obj ) )
  {
      send_to_char( "You can't let go of it.\n\r", ch );
      return;
  }

  obj_from_char( obj );
  obj_to_room( obj, ch->in_room );
  act( "$n drops $p.", ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You drop $p.", ch, obj, NULL, TO_CHAR ,FALSE);
  if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
  {
      act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM,FALSE);
      act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR,FALSE);
      extract_obj(obj);
  }
    }
    else
    {
  /* 'drop all' or 'drop all.obj' */
  found = FALSE;
  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
      obj_next = obj->next_content;

      if ( ( arg[3] == '\0' || is_name( &arg[4], obj->name ) )
      &&   can_see_obj( ch, obj )
      &&   obj->wear_loc == WEAR_NONE
      &&   can_drop_obj( ch, obj ) )
      {
    found = TRUE;
    obj_from_char( obj );
    obj_to_room( obj, ch->in_room );
    act( "$n drops $p.", ch, obj, NULL, TO_ROOM ,FALSE);
    act( "You drop $p.", ch, obj, NULL, TO_CHAR ,FALSE);
          if (IS_OBJ_STAT(obj,ITEM_MELT_DROP))
          {
                  act("$p dissolves into smoke.",ch,obj,NULL,TO_ROOM,FALSE);
                  act("$p dissolves into smoke.",ch,obj,NULL,TO_CHAR,FALSE);
                  extract_obj(obj);
          }
      }
  }

  if ( !found )
  {
      if ( arg[3] == '\0' )
    act( "You are not carrying anything.",
        ch, NULL, arg, TO_CHAR ,FALSE);
      else
    act( "You are not carrying any $T.",
        ch, NULL, &arg[4], TO_CHAR ,FALSE);
  }
    }

    return;
}



void do_give( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA  *obj;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
  send_to_char( "Give what to whom?\n\r", ch );
  return;
    }

    if ( is_number( arg1 ) )
    {
  /* 'give NNNN coins victim' */
  int amount;
  bool silver;

  amount   = atoi(arg1);
  if ( amount <= 0
  || ( str_cmp( arg2, "coins" ) && str_cmp( arg2, "coin" ) &&
       str_cmp( arg2, "gold"  ) && str_cmp( arg2, "silver")) )
  {
      send_to_char( "Sorry, you can't do that.\n\r", ch );
      return;
  }

  silver = str_cmp(arg2,"gold");

  argument = one_argument( argument, arg2 );
  if ( arg2[0] == '\0' )
  {
      send_to_char( "Give what to whom?\n\r", ch );
      return;
  }

  if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
  {
      send_to_char( "They aren't here.\n\r", ch );
      return;
  }

  if( is_affected(victim,skill_lookup("wraithform")) )
  {
    send_to_char( "They are made of mist, how can they hold it.\n\r", ch);
    return;
  }

  if ( victim == ch )
  {
        send_to_char( "Why bother giving something to yourself?\n\r", ch);
        return;
  }

  if ( (!silver && ch->gold < amount) || (silver && ch->silver < amount) )
  {
      send_to_char( "You haven't got that much.\n\r", ch );
      return;
  }

  if (silver)
  {
   if (!IS_NPC(victim) &&
       get_carry_weight(victim) + amount/10 > can_carry_w( victim ) )
    {
      act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR ,FALSE);
      return;
    }
      ch->silver    -= amount;
      victim->silver  += amount;
  }
  else
  {
   if (!IS_NPC(victim) &&
       get_carry_weight(victim) + amount*2/5 > can_carry_w( victim ) )
    {
     act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR ,FALSE);
      return;
    }
      ch->gold    -= amount;
      victim->gold  += amount;
  }

  sprintf(buf,"$n gives you %d %s.",amount, silver ? "silver" : "gold");
  act( buf, ch, NULL, victim, TO_VICT    ,FALSE);
  act( "$n gives $N some coins.",  ch, NULL, victim, TO_NOTVICT ,FALSE);
  sprintf(buf,"You give $N %d %s.",amount, silver ? "silver" : "gold");
  act( buf, ch, NULL, victim, TO_CHAR    ,FALSE);

  if (IS_NPC(victim) && IS_SET(victim->act,ACT_IS_CHANGER))
  {
      int change;

      change = (silver ? 95 * amount / 100 / 100
           : 95 * amount);


      if (!silver && change > victim->silver)
        victim->silver += change;

      if (silver && change > victim->gold)
        victim->gold += change;

      if (change < 1 && can_see(victim,ch,FALSE))
      {
    act(
  "$n tells you 'I'm sorry, you did not give me enough to change.'"
        ,victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
    sprintf(buf,"%d %s %s",
      amount, silver ? "silver" : "gold",ch->name);
    do_give(victim,buf);
      }
      else if (can_see(victim,ch,FALSE))
      {
    sprintf(buf,"%d %s %s",
      change, silver ? "gold" : "silver",ch->name);
    do_give(victim,buf);
    if (silver)
    {
        sprintf(buf,"%d silver %s",
      (95 * amount / 100 - change * 100),ch->name);
        do_give(victim,buf);
    }
    act("$n tells you 'Thank you, come again.'",
        victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
      }
  }
  return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
 }
    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
    {
  send_to_char( "You do not have that item.\n\r", ch );
  return;
    }

    if ( obj->wear_loc != WEAR_NONE )
    {
  send_to_char( "You must remove it first.\n\r", ch );
  return;
    }

/*    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }*/
if( is_affected(victim,skill_lookup("wraithform")) )
  {
    send_to_char( "They are made of mist, how can they hold it.\n\r", ch);
    return;
  }


  if ( victim == ch )
  {
        send_to_char( "Why bother giving something to yourself?\n\r", ch);
        return;
  }

    if (IS_NPC(victim))
    {
      if(victim->pIndexData->pShop != NULL
        && victim->spec_fun == 0  && !IS_IMMORTAL(ch))
     {
  act("$N tells you 'Sorry, you'll have to sell that.'",
      ch,NULL,victim,TO_CHAR,FALSE);
//  ch->reply = victim;
  return;
     }
     else if(IS_SET(victim->act,ACT_IS_CHANGER))
     {
       act("$N is only interested in your coins.", ch, NULL, victim, TO_CHAR, FALSE);
       return;
     }
    }

    if ( !can_drop_obj( ch, obj ) && !IS_IMMORTAL(victim) && !IS_IMMORTAL(ch))
    {
  send_to_char( "You can't let go of it.\n\r", ch );
  return;
    }

    if ( victim->carry_number + get_obj_number( obj ) > can_carry_n( victim ) )
    {
  act( "$N has $S hands full.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if (get_carry_weight(victim) + get_obj_weight(obj) > can_carry_w( victim ) )
    {
  act( "$N can't carry that much weight.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if ( !IS_IMMORTAL(ch) && !can_see_obj( victim, obj ) )
    {
  act( "$N can't see it.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_WEAPONSMITH) && (obj->item_type == ITEM_WEAPON) )
    {
      int condition,original,i, cost;
      condition =0;
      original = 0;
      cost = 0;
      i =0;
      for ( i=1; i<3 ; i++ )
      {
         condition += obj->value[i];
         original += obj->pIndexData->value[i];
      }
      if ( ( cost = (original - condition)* 2000) == 0)
      {
    act("$n tells you 'I'm sorry, I would only damage this weapon.'"
        ,victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
        return;
      }
      if ( ( ch->silver + 100 * ch->gold) < cost )
      {
    act("$n tells you 'I'm sorry, you can't afford my services.'"
        ,victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
        return;
      }
      deduct_cost(ch,cost);
      for ( i=1; i<3 ; i++ )
      {
        obj->value[i] = obj->pIndexData->value[i];
      }
    act("$n tells you 'Thank you, come again.'",
        victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
      return;
    }

    if (IS_NPC(victim) && IS_SET(victim->act, ACT_IS_ARMOURER) && (obj->item_type == ITEM_ARMOR) )
    {
      int condition,original,i, cost;
      condition =0;
      original = 0;
      cost = 0;
      i =0;
      for ( i=0; i<4 ; i++ )
      {
         condition += obj->value[i];
         original += obj->pIndexData->value[i];
      }
      if ( ( cost = (original - condition)* 200) == 0)
      {
    act("$n tells you 'I'm sorry, I would only damage this piece of armor.'"
        ,victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
        return;
      }
      if ( ( ch->silver + 100 * ch->gold) < cost )
      {
    act("$n tells you 'I'm sorry, you can't afford my services.'"
        ,victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
        return;
      }
      deduct_cost(ch,cost);
      for ( i=0; i<4 ; i++ )
      {
        obj->value[i] = obj->pIndexData->value[i];
      }
    act("$n tells you 'Thank you, come again.'",
        victim,NULL,ch,TO_VICT,FALSE);
//    ch->reply = victim;
      return;

    }
    if(IS_NPC(victim) && victim->spec_fun != NULL)
    {// Need a way to not get the item at all
      if(victim->pIndexData->vnum == MOB_VNUM_BOUNTY_ADMIN)
      {
        bounty_admin_claim(ch, victim, obj);
        return;
      }
    }
    obj_from_char( obj );
    obj_to_char( obj, victim );
    act( "$n gives $p to $N.", ch, obj, victim, TO_NOTVICT ,FALSE);
    act( "$n gives you $p.",   ch, obj, victim, TO_VICT    ,FALSE);
    act( "You give $p to $N.", ch, obj, victim, TO_CHAR    ,FALSE);

    if(IS_NPC(victim) && victim->spec_fun != NULL)
    {// There may be many quests later that involve giving NPCs items, handle it here
    // For now, they have to have a spec_fun
        if(victim->pIndexData->vnum == MOB_VNUM_POISON_EATER)// Neutral quest update, unknown result
                                quest_handler(victim, ch, obj, TASK_POISON_EATER, QSTEP_DRAW);
    }

    return;
}


void do_lick( CHAR_DATA *ch, char *argument )
{
    OBJ_DATA *obj;
    AFFECT_DATA af;

    if ( ch->race != race_lookup("goblin") || ch->level < 15 )
    {
        send_to_char("You'll just cut your tongue.\n\r",ch);
        return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( argument == '\0' )
    {
  send_to_char("What do you wish to poison?\n\r",ch);
  return;
    }

        if ( ( obj = get_obj_list(ch,argument,ch->carrying) ) == NULL )
        {
     send_to_char("You are not carrying that.\n\r",ch);
     return;
        }

    if ( obj->item_type != ITEM_WEAPON )
    {
        send_to_char("That isn't a weapon.\n\r",ch);
        return;
    }

    /* bladed weapons only */
    /* since some exotscs could have blades, we'll just use damage type */
    if ( attack_table[obj->value[3]].damage <= 0 ||
         attack_table[obj->value[3]].damage == DAM_BASH )
    {
        send_to_char("Bladed weapons only.\n\r",ch);
        return;
    }

    if ( IS_WEAPON_STAT(obj,WEAPON_POISON) )
    {
        send_to_char("This weapon is already poisoned.\n\r",ch);
        return;
    }

       af.where     = TO_WEAPON;
       af.type          = gsn_poison;
       af.level         = ch->level + ( ch->level / 10 );
       af.duration      = ( ch->level / 2 );
       af.location      = 0;
       af.modifier      = 0;
       af.bitvector     = WEAPON_POISON;

       affect_to_obj(obj,&af);

        act("$n licks $p.",ch,obj,NULL,TO_ROOM,FALSE);
        act("You lick $p.",ch,obj,NULL,TO_CHAR,FALSE);

        return;
}

/* for poisoning weapons and food/drink */
void do_envenom(CHAR_DATA *ch, char *argument)
{
    OBJ_DATA *obj;
    AFFECT_DATA af;
    int skill;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    /* find out what */
    if (argument[0] == '\0')
    {
  send_to_char("Envenom what item?\n\r",ch);
  return;
    }

    obj =  get_obj_list(ch,argument,ch->carrying);

    if (obj== NULL)
    {
  send_to_char("You don't have that item.\n\r",ch);
  return;
    }

    if ((skill = get_skill(ch,gsn_envenom)) < 1)
    {
  send_to_char("Are you crazy? You'd poison yourself!\n\r",ch);
  return;
    }

    if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
    {
  if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
  {
      act("You fail to poison $p.",ch,obj,NULL,TO_CHAR,FALSE);
      return;
  }

  if (number_percent() < skill)  /* success! */
  {
      act("$n treats $p with deadly poison.",ch,obj,NULL,TO_ROOM,FALSE);
      act("You treat $p with deadly poison.",ch,obj,NULL,TO_CHAR,FALSE);
      if (!obj->value[3])
      {
    obj->value[3] = 1;
    check_improve(ch,gsn_envenom,TRUE,4);
      }
      WAIT_STATE(ch,skill_table[gsn_envenom].beats);
      return;
  }

  act("You fail to poison $p.",ch,obj,NULL,TO_CHAR,FALSE);
  if (!obj->value[3])
      check_improve(ch,gsn_envenom,FALSE,4);
  WAIT_STATE(ch,skill_table[gsn_envenom].beats);
  return;
     }

    if (obj->item_type == ITEM_WEAPON)
    {
        if (IS_WEAPON_STAT(obj,WEAPON_FLAMING)
        ||  IS_WEAPON_STAT(obj,WEAPON_FROST)
        ||  IS_WEAPON_STAT(obj,WEAPON_VAMPIRIC)
        ||  IS_WEAPON_STAT(obj,WEAPON_SHARP)
        ||  IS_WEAPON_STAT(obj,WEAPON_VORPAL)
        ||  IS_WEAPON_STAT(obj,WEAPON_SHOCKING)
        ||  IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
        {
            act("You can't seem to envenom $p.",ch,obj,NULL,TO_CHAR,FALSE);
            return;
        }

        if (obj->value[3] < 0
        ||  attack_table[obj->value[3]].damage == DAM_BASH)
        {
           send_to_char("You can only envenom edged weapons.\n\r",ch);
           return;
        }

        if (IS_WEAPON_STAT(obj,WEAPON_POISON))
        {
            act("$p is already envenomed.",ch,obj,NULL,TO_CHAR,FALSE);
            return;
        }

        if (number_percent() < skill)
        {

            af.where     = TO_WEAPON;
            af.type      = gsn_poison;
            af.level     = 5 + ch->level/5;
            af.duration  = 5 + ch->level/3;
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_POISON;
            affect_to_obj(obj,&af);

            act("$n coats $p with deadly venom.",ch,obj,NULL,TO_ROOM,FALSE);
            act("You coat $p with venom.",ch,obj,NULL,TO_CHAR,FALSE);
            check_improve(ch,gsn_envenom,TRUE,3);
            WAIT_STATE(ch,skill_table[gsn_envenom].beats);
            return;
        }
        else
        {
           act("You fail to envenom $p.",ch,obj,NULL,TO_CHAR,FALSE);
           check_improve(ch,gsn_envenom,FALSE,3);
           WAIT_STATE(ch,skill_table[gsn_envenom].beats);
           return;
        }
    }

    act("You can't poison $p.",ch,obj,NULL,TO_CHAR,FALSE);
    return;
}

void do_fill( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj2 = NULL;
    OBJ_DATA *fountain;
    bool found;
    int count;

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg1[0] == '\0' )
    {
  send_to_char( "Fill what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
    {
  send_to_char( "You do not have that item.\n\r", ch );
  return;
    }

    if( arg2[0] != '\0' )
    {
       if ( ( obj2 = get_obj_here( ch, arg2 ) ) == NULL )
       {
          send_to_char( "You can't find it.\n\r", ch );
          return;
       }
    }

    found = FALSE;
    count = 0;
    for ( fountain = ch->in_room->contents; fountain != NULL;
           fountain = fountain->next_content )
    {
       if (arg2[0] == '\0' )
       {
          if ( fountain->item_type == ITEM_FOUNTAIN )
          {
/*           if ( ++count == number )
             {
             */
                 found = TRUE;
                 break;
          /*   }         */
          }
       }
       else
       {
          if ( fountain->item_type == ITEM_FOUNTAIN )
          {
             if ( fountain == obj2 )
             {
                found = TRUE;
                break;
             }
          }
       }
    }

    if ( !found )
    {
  send_to_char( "There is no fountain here!\n\r", ch );
  return;
    }

      if ( obj->item_type != ITEM_DRINK_CON  || obj->pIndexData->vnum == OBJ_VNUM_GRAIL)
    {
  send_to_char( "You can't fill that.\n\r", ch );
  return;
    }

    if ( obj->value[1] != 0 && obj->value[2] != fountain->value[2] )
    {
  send_to_char( "There is already another liquid in it.\n\r", ch );
  return;
    }

    if ( obj->value[1] >= obj->value[0] )
    {
  send_to_char( "Your container is full.\n\r", ch );
  return;
    }

    sprintf(buf,"You fill $p with %s from $P.",
  liq_table[fountain->value[2]].liq_name);
    act( buf, ch, obj,fountain, TO_CHAR ,FALSE);
    sprintf(buf,"$n fills $p with %s from $P.",
  liq_table[fountain->value[2]].liq_name);
    act(buf,ch,obj,fountain,TO_ROOM,FALSE);
    obj->value[2] = fountain->value[2];
    obj->value[1] = obj->value[0];
    return;
}

void do_pour (CHAR_DATA *ch, char *argument)
{
    char arg[MAX_STRING_LENGTH],buf[MAX_STRING_LENGTH];
    OBJ_DATA *out, *in;
    CHAR_DATA *vch = NULL;
    int amount;

    argument = one_argument(argument,arg);

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if (arg[0] == '\0' || argument[0] == '\0')
    {
  send_to_char("Pour what into what?\n\r",ch);
  return;
    }


    if ((out = get_obj_carry(ch,arg)) == NULL)
    {
  send_to_char("You don't have that item.\n\r",ch);
  return;
    }

    if (out->item_type != ITEM_DRINK_CON)
    {
  send_to_char("That's not a drink container.\n\r",ch);
  return;
    }

    if (!str_cmp(argument,"out"))
    {
  if (out->value[1] == 0)
  {
      send_to_char("It's already empty.\n\r",ch);
      return;
  }

  out->value[1] = 0;
  out->value[3] = 0;
  sprintf(buf,"You invert $p, spilling %s all over the ground.",
    liq_table[out->value[2]].liq_name);
  act(buf,ch,out,NULL,TO_CHAR,FALSE);

  sprintf(buf,"$n inverts $p, spilling %s all over the ground.",
    liq_table[out->value[2]].liq_name);
  act(buf,ch,out,NULL,TO_ROOM,FALSE);
  return;
    }

    if ((in = get_obj_here(ch,argument)) == NULL)
    {
  vch = get_char_room(ch,argument);

  if (vch == NULL)
  {
      send_to_char("Pour into what?\n\r",ch);
      return;
  }

  in = get_eq_char(vch,WEAR_HOLD);

  if (in == NULL)
  {
      send_to_char("They aren't holding anything.",ch);
      return;
  }
    }

    if (in->item_type != ITEM_DRINK_CON)
    {
  send_to_char("You can only pour into other drink containers.\n\r",ch);
  return;
    }
    if (in->pIndexData->vnum == OBJ_VNUM_GRAIL)
    {
      send_to_char("Only the Lord Almighty may refill the grail.  Quit trying to cheat.\n\r",ch);
      return;
    }


    if (in == out)
    {
  send_to_char("You cannot change the laws of physics!\n\r",ch);
  return;
    }

    if (in->value[1] != 0 && in->value[2] != out->value[2])
    {
  send_to_char("They don't hold the same liquid.\n\r",ch);
  return;
    }

    if (out->value[1] == 0)
    {
  act("There's nothing in $p to pour.",ch,out,NULL,TO_CHAR,FALSE);
  return;
    }

    if (in->value[1] >= in->value[0])
    {
  act("$p is already filled to the top.",ch,in,NULL,TO_CHAR,FALSE);
  return;
    }

    amount = UMIN(out->value[1],in->value[0] - in->value[1]);

    in->value[1] += amount;
    out->value[1] -= amount;
    in->value[2] = out->value[2];

    if (vch == NULL)
    {
      sprintf(buf,"You pour %s from $p into $P.",
      liq_table[out->value[2]].liq_name);
      act(buf,ch,out,in,TO_CHAR,FALSE);
      sprintf(buf,"$n pours %s from $p into $P.",
      liq_table[out->value[2]].liq_name);
      act(buf,ch,out,in,TO_ROOM,FALSE);
    }
    else
    {
        sprintf(buf,"You pour some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_CHAR,FALSE);
  if(!IS_NPC(vch))
    {
    sprintf(buf,"$n pours you some %s.",
        liq_table[out->value[2]].liq_name);
    act(buf,ch,NULL,vch,TO_VICT,FALSE);
          }
  sprintf(buf,"$n pours some %s for $N.",
            liq_table[out->value[2]].liq_name);
        act(buf,ch,NULL,vch,TO_NOTVICT,FALSE);

    }
}

void do_drink( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;
    int liquid;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
  {
      if ( obj->item_type == ITEM_FOUNTAIN )
    break;
  }

  if ( obj == NULL )
  {
      send_to_char( "Drink what?\n\r", ch );
      return;
  }
    }
    else
    {
  if ( ( obj = get_obj_here( ch, arg ) ) == NULL )
  {
      send_to_char( "You can't find it.\n\r", ch );
      return;
  }
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK] > 10 )
    {
  send_to_char( "You fail to reach your mouth.  *Hic*\n\r", ch );
  return;
    }

    switch ( obj->item_type )
    {
    default:
  send_to_char( "You can't drink from that.\n\r", ch );
  return;

    case ITEM_FOUNTAIN:
        if ( ( liquid = obj->value[2] )  < 0 )
        {
            bug( "Do_drink: bad liquid number %d.", liquid );
            liquid = obj->value[2] = 0;
        }
  amount = liq_table[liquid].liq_affect[4] * 3;
  break;

    case ITEM_DRINK_CON:
  if ( obj->value[1] <= 0 )
  {
      send_to_char( "It is already empty.\n\r", ch );
      return;
  }

  if ( ( liquid = obj->value[2] )  < 0 )
  {
      bug( "Do_drink: bad liquid number %d.", liquid );
      liquid = obj->value[2] = 0;
  }

        amount = liq_table[liquid].liq_affect[4];
        amount = UMIN(amount, obj->value[1]);
  break;
     }
    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)
    &&  ch->pcdata->condition[COND_FULL] > 45)
    {
  send_to_char("You're too full to drink more.\n\r",ch);
  return;
    }

    if ( IS_SET(ch->act,PLR_VAMP)
         && !IS_NPC(ch)
         && liquid != liq_lookup("blood") )
    {
        send_to_char("But you desire blood...\n\r",ch);
        return;
    }

    if (IS_SET(ch->act,PLR_WERE)
        && !IS_NPC(ch))
    {
       send_to_char("The smell of rotting flesh is more inviting... \n\r",ch);
       return;
    }

    if (IS_SET(ch->act,PLR_MUMMY)
        && !IS_NPC(ch))
    {
       send_to_char("Fear is all the sustenance you need... \n\r",ch);
       return;
    }

    act( "$n drinks $T from $p.",
  ch, obj, liq_table[liquid].liq_name, TO_ROOM ,FALSE);
    act( "You drink $T from $p.",
  ch, obj, liq_table[liquid].liq_name, TO_CHAR ,FALSE);

    if ( !IS_SET(ch->act,PLR_VAMP) )
    {
    gain_condition( ch, COND_DRUNK,
  amount * liq_table[liquid].liq_affect[COND_DRUNK] / 36 );
    gain_condition( ch, COND_FULL,
  amount * liq_table[liquid].liq_affect[COND_FULL] / 4 );
    gain_condition( ch, COND_THIRST,
  amount * liq_table[liquid].liq_affect[COND_THIRST] / 10 );
    gain_condition(ch, COND_HUNGER,
  amount * liq_table[liquid].liq_affect[COND_HUNGER] / 2 );
    }
    else
    {
    gain_condition( ch, COND_FULL, 10 );
    gain_condition( ch, COND_HUNGER, 10 );
    gain_condition( ch, COND_THIRST, 10 );
    }

    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_DRUNK]  > 10 )
  send_to_char( "You feel drunk.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL]   > 40 )
  send_to_char( "You are full.\n\r", ch );
    if ( !IS_NPC(ch) && ch->pcdata->condition[COND_THIRST] > 40 )
  send_to_char( "Your thirst is quenched.\n\r", ch );

    if (obj->value[0] > 0)
        obj->value[1] -= amount;

    if (obj->pIndexData->vnum == OBJ_VNUM_GRAIL)
        {
        ch->hit = UMIN(ch->max_hit, ch->hit+75);
        send_to_char("Your wounds are healed by the power of the grail!\n\r",ch);
        }

    switch(check_immune(ch,DAM_POISON))
     {
        case IS_IMMUNE: if(obj->value[0]>0) obj->value[1] -= amount; return;
        case IS_RESISTANT:  amount -= 3;  break;
        case IS_VULNERABLE: amount += 2;  break;
     }

    if ( obj->value[3] != 0)
    {

  /* The shit was poisoned ! */
  AFFECT_DATA af;

  act( "$n chokes and gags.", ch, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char( "You choke and gag.\n\r", ch );
  af.where     = TO_AFFECTS;
  af.type      = gsn_poison;
  af.level   = number_fuzzy(amount);
  af.duration  = 3 * amount;
  af.location  = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = AFF_POISON;
  affect_join( ch, &af );
    }

    return;
}



void do_eat( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int amount;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }

    if ( arg[0] == '\0' )
    {
  send_to_char( "Eat what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
  send_to_char( "You do not have that item.\n\r", ch );
  return;
    }

    if ( !IS_IMMORTAL(ch) )
    {
  if ( obj->item_type != ITEM_FOOD && obj->item_type != ITEM_PILL )
  {
      send_to_char( "That's not edible.\n\r", ch );
      return;
  }

  if ( !IS_NPC(ch) && ch->pcdata->condition[COND_FULL] > 40 )
  {
      send_to_char( "You are too full to eat more.\n\r", ch );
      return;
  }
  if ( obj->item_type == ITEM_PILL)
  {
    if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not eat pills in the arena.\n\r", ch);
       return;
    }
    if(ch->position <= POS_RESTING)
    {
      send_to_char( "Nah....You feel too relaxed...\n\r", ch);
      return;
    }
  }
    }

    amount = obj->value[0];

    if ((obj->item_type == ITEM_FOOD)
        && IS_SET(ch->act,PLR_VAMP)
        && !IS_NPC(ch))
    {
       send_to_char("But you want blood... \n\r",ch);
       return;
    }

    /* Corpse vnums are less then 100, Were's eat those */
    if ((obj->item_type == ITEM_FOOD)
        && obj->pIndexData->vnum > 100
        && IS_SET(ch->act,PLR_WERE)
        && !IS_NPC(ch))
    {
       send_to_char("The smell of rotting flesh is more inviting... \n\r",ch);
       return;
    }

    if ((obj->item_type == ITEM_FOOD)
        && IS_SET(ch->act,PLR_MUMMY)
        && !IS_NPC(ch))
    {
       send_to_char("Fear is all the sustenance you need... \n\r",ch);
       return;
    }

    act( "$n eats $p.",  ch, obj, NULL, TO_ROOM ,FALSE);
    act( "You eat $p.", ch, obj, NULL, TO_CHAR ,FALSE);

    switch ( obj->item_type )
    {

    case ITEM_FOOD:

  if  ( !IS_NPC(ch) )
  {
      int condition;

      if (IS_SET(ch->act,PLR_WERE))
      {
         gain_condition(ch,COND_FULL,   15 - ( ch->level / 5 ) );
         gain_condition(ch,COND_HUNGER, 15 - ( ch->level / 5 ) );
         gain_condition(ch,COND_THIRST, 15 - ( ch->level / 5 ) );

         if ( ch->pcdata->condition[COND_FULL] > 46 )
         {
            send_to_char("Your cravings for flesh has ended.\n\r",ch);
            return;
         }
      }
      else
      {
         condition = ch->pcdata->condition[COND_HUNGER];
         gain_condition( ch, COND_FULL, obj->value[0] );
         gain_condition( ch, COND_HUNGER, obj->value[1]);
         if ( condition == 0 && ch->pcdata->condition[COND_HUNGER] > 0 )
            send_to_char( "You are no longer hungry.\n\r", ch );
         else if ( ch->pcdata->condition[COND_FULL] > 40 )
            send_to_char( "You are full.\n\r", ch );
      }
  }

    switch(check_immune(ch,DAM_POISON))
     {
        case IS_IMMUNE: extract_obj(obj); return;
        case IS_RESISTANT:  amount -= 3;  break;
        case IS_VULNERABLE: amount += 2;  break;
     }

  if ( obj->value[3] != 0 )
  {

      /* The shit was poisoned! */
      AFFECT_DATA af;

      act( "$n chokes and gags.", ch, 0, 0, TO_ROOM ,FALSE);
      send_to_char( "You choke and gag.\n\r", ch );

      af.where   = TO_AFFECTS;
      af.type      = gsn_poison;
      af.level   = number_fuzzy(amount);
      af.duration  = 2 * amount;
      af.location  = APPLY_NONE;
      af.modifier  = 0;
      af.bitvector = AFF_POISON;
      affect_join( ch, &af );
  }
  break;

    case ITEM_PILL:
  obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
  obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
  obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );
  break;
    }

    extract_obj( obj );
    return;
}

/*
 * Remove All Worn Objects.
 */
void remove_all_objs( CHAR_DATA *ch, bool verbose )
{
    OBJ_DATA *obj;
    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


   if ( ( obj = get_eq_char( ch, WEAR_LIGHT)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_FINGER_L)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_FINGER_R)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_NECK_1)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_NECK_2)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_BODY)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_HEAD)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_LEGS)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_FEET)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_HANDS)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_ARMS)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_ABOUT)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_WAIST)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_WRIST_L)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_WRIST_R)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_SHIELD)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char(ch,WEAR_SECOND)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_WIELD)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char( ch, WEAR_HOLD)) != NULL)
     unequip_char( ch, obj );
   if ( ( obj = get_eq_char(ch,WEAR_FLOAT)) != NULL)
     unequip_char( ch, obj );

   if(verbose)
   {
     act( "$n removes all of $s equipment.", ch, NULL, NULL, TO_ROOM ,FALSE);
     act( "You remove all of your equipment.", ch, NULL, NULL, TO_CHAR ,FALSE);
   }
}

/*
 * Remove an object.
 */
bool remove_obj( CHAR_DATA *ch, int iWear, bool fReplace )
{
    OBJ_DATA *obj;

    if ( ( obj = get_eq_char( ch, iWear ) ) == NULL )
  return TRUE;

    if ( !fReplace )
  return FALSE;

    if ( IS_SET(obj->extra_flags, ITEM_NOREMOVE) &&
      (!IS_IMMORTAL(ch) || IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED) ))
    {
  act( "You can't remove $p.", ch, obj, NULL, TO_CHAR ,FALSE);
  return FALSE;
    }

    unequip_char( ch, obj );
    act( "$n stops using $p.", ch, obj, NULL, TO_ROOM ,FALSE);
    act( "You stop using $p.", ch, obj, NULL, TO_CHAR ,FALSE);
    return TRUE;
}



/*
 * Is the secondary weapon wear-able?
 */
bool can_wear_secondary( CHAR_DATA *ch, OBJ_DATA *prim, OBJ_DATA *sec )
{
   if ( sec->weight == prim->weight )
   {
       send_to_char("That combination isn't possible.\n\r",ch);
      // send_to_char("It's too heavy.\n\r",ch);
       return FALSE;
   }

   if (HAS_KIT(ch,"ranger"))
      return TRUE;

   if (prim->value[0] == WEAPON_SWORD && sec->value[0] == WEAPON_DAGGER)
      return TRUE;

   if (prim->value[0] == WEAPON_EXOTIC || sec->value[0] == WEAPON_EXOTIC)
      return TRUE;

   if ( prim->value[0] != sec->value[0])
   {
       send_to_char("That combination isn't possible.\n\r",ch);
       return FALSE;
   }

   return TRUE;
}

void add_bonuses( CHAR_DATA *ch, OBJ_DATA *obj)
{
  if(obj->damaged >= 100)
  {
        bug("Damaged object having bonuses added.", 0);
        return;
  }
  if(obj->wear_loc == -1)
  {
        bug("Non-worn gear trying to add bonuses.", 0);
        return;
  }
    int i;
    int AppType = APPLY_BOTH;
    if(obj->wear_loc == WEAR_WIELD)
        AppType = APPLY_PRIMARY;
    else if(obj->wear_loc == WEAR_SECOND)
        AppType = APPLY_SECONDARY;
    AFFECT_DATA *paf = NULL;
    for (i = 0; i < 4; i++)
      ch->armor[i]        -= apply_ac( obj, obj->wear_loc,i );

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

}

/* Remove bonuses, used for damaged gear */
void remove_bonuses( CHAR_DATA *ch, OBJ_DATA *obj)
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
    {/* Not a bug for this one, means it's a destroyed object on the ground */
  return;
    }

    if(obj->damaged < 100)
    {
        bug("remove_bonuses: Object not destroyed.", 0);
        return;
    }

    for (i = 0; i < 4; i++)
      ch->armor[i]  += apply_ac( obj, obj->wear_loc,i );

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

    /* readd racial affects to restore incase lost on item remove */
    ch->affected_by = ch->affected_by|race_table[ch->race].aff;

    return;
}

/*
 * Wear one object.
 * Optional replacement of existing objects.
 * Big repetitive code, ick.
 */
void wear_obj( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace )
{
    char buf[MAX_STRING_LENGTH];

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }

    if ( !IS_NPC(ch) && HAS_KIT(ch,"nethermancer")
         && obj->item_type == ITEM_ARMOR )
    {
act("$n tries to wear $p and shudders in pain.",ch,obj,NULL,TO_ROOM,FALSE);
act("Your pact with the Netherworld forbids it.",ch,obj,NULL,TO_CHAR,FALSE);
        return;
    }

    if ( obj->pIndexData->vnum == 4436)
    {
       send_to_char("That promise ring is no longer allowed in the game.\n\r",ch);
       return;
    }


    if ( ch->level < obj->level )
    {
  sprintf( buf, "You must be level %d to use this object.\n\r",
      obj->level );
  send_to_char( buf, ch );
  act( "$n tries to use $p, but is too inexperienced.",
      ch, obj, NULL, TO_ROOM ,FALSE);
  return;
    }


    if ( !can_wear_obj(ch,obj) )
    {
act("$n tries to wear $p and it doesn't fit.", ch, obj, NULL, TO_ROOM,FALSE);
act("You try to wear $p and it doesn't fit.", ch, obj, NULL, TO_CHAR,FALSE);
        return;
    }

    if ( obj->item_type == ITEM_LIGHT )
    {
  if ( !remove_obj( ch, WEAR_LIGHT, fReplace ) )
      return;
  act( "$n lights $p and holds it.", ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You light $p and hold it.",  ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_LIGHT );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FINGER ) )
    {
  if ( get_eq_char( ch, WEAR_FINGER_L ) != NULL
  &&   get_eq_char( ch, WEAR_FINGER_R ) != NULL
  &&   !remove_obj( ch, WEAR_FINGER_L, fReplace )
  &&   !remove_obj( ch, WEAR_FINGER_R, fReplace ) )
      return;

  if ( get_eq_char( ch, WEAR_FINGER_L ) == NULL )
  {
      act( "$n wears $p on $s left finger.",    ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p on your left finger.",  ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_FINGER_L );
      return;
  }

  if ( get_eq_char( ch, WEAR_FINGER_R ) == NULL )
  {
      act( "$n wears $p on $s right finger.",   ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p on your right finger.", ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_FINGER_R );
      return;
  }

  bug( "Wear_obj: no free finger.", 0 );
  send_to_char( "You already wear two rings.\n\r", ch );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_NECK ) )
    {
  if ( get_eq_char( ch, WEAR_NECK_1 ) != NULL
  &&   get_eq_char( ch, WEAR_NECK_2 ) != NULL
  &&   !remove_obj( ch, WEAR_NECK_1, fReplace )
  &&   !remove_obj( ch, WEAR_NECK_2, fReplace ) )
      return;

  if ( get_eq_char( ch, WEAR_NECK_1 ) == NULL )
  {
      act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_NECK_1 );
      return;
  }

  if ( get_eq_char( ch, WEAR_NECK_2 ) == NULL )
  {
      act( "$n wears $p around $s neck.",   ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p around your neck.", ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_NECK_2 );
      return;
  }

  bug( "Wear_obj: no free neck.", 0 );
  send_to_char( "You already wear two neck items.\n\r", ch );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_BODY ) )
    {
  if ( !remove_obj( ch, WEAR_BODY, fReplace ) )
      return;
  act( "$n wears $p on $s torso.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your torso.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_BODY );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HEAD ) )
    {
  if ( !remove_obj( ch, WEAR_HEAD, fReplace ) )
      return;
  act( "$n wears $p on $s head.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your head.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_HEAD );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_LEGS ) )
    {
  if ( !remove_obj( ch, WEAR_LEGS, fReplace ) )
      return;
  act( "$n wears $p on $s legs.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your legs.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_LEGS );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_FEET ) )
    {
  if ( !remove_obj( ch, WEAR_FEET, fReplace ) )
      return;
  act( "$n wears $p on $s feet.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your feet.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_FEET );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_HANDS ) )
    {
  if ( !remove_obj( ch, WEAR_HANDS, fReplace ) )
      return;
  act( "$n wears $p on $s hands.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your hands.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_HANDS );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ARMS ) )
    {
  if ( !remove_obj( ch, WEAR_ARMS, fReplace ) )
      return;
  act( "$n wears $p on $s arms.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p on your arms.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_ARMS );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_ABOUT ) )
    {
  if ( !remove_obj( ch, WEAR_ABOUT, fReplace ) )
      return;
  act( "$n wears $p about $s torso.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p about your torso.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_ABOUT );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WAIST ) )
    {
  if ( !remove_obj( ch, WEAR_WAIST, fReplace ) )
      return;
  act( "$n wears $p about $s waist.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p about your waist.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_WAIST );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_WRIST ) )
    {
  if ( get_eq_char( ch, WEAR_WRIST_L ) != NULL
  &&   get_eq_char( ch, WEAR_WRIST_R ) != NULL
  &&   !remove_obj( ch, WEAR_WRIST_L, fReplace )
  &&   !remove_obj( ch, WEAR_WRIST_R, fReplace ) )
      return;

  if ( get_eq_char( ch, WEAR_WRIST_L ) == NULL )
  {
      act( "$n wears $p around $s left wrist.",
    ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p around your left wrist.",
    ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_WRIST_L );
      return;
  }

  if ( get_eq_char( ch, WEAR_WRIST_R ) == NULL )
  {
      act( "$n wears $p around $s right wrist.",
    ch, obj, NULL, TO_ROOM ,FALSE);
      act( "You wear $p around your right wrist.",
    ch, obj, NULL, TO_CHAR ,FALSE);
      equip_char( ch, obj, WEAR_WRIST_R );
      return;
  }

  bug( "Wear_obj: no free wrist.", 0 );
  send_to_char( "You already wear two wrist items.\n\r", ch );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WEAR_SHIELD ) )
    {
  OBJ_DATA *weapon;

  if ( !remove_obj( ch, WEAR_SHIELD, fReplace ) )
      return;

  weapon = get_eq_char(ch,WEAR_WIELD);
  if (weapon != NULL && ch->size < SIZE_LARGE
  &&  IS_WEAPON_STAT(weapon,WEAPON_TWO_HANDS))
  {
      send_to_char("Your hands are tied up with your weapon!\n\r",ch);
      return;
  }

  if ( is_affected(ch,gsn_bladesong))
  {
     send_to_char("You can not wear a shield while performing the bladesong.\n\r",ch);
     return;
  }

  act( "$n wears $p as a shield.", ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You wear $p as a shield.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_SHIELD );
  return;
    }

    if ( CAN_WEAR( obj, ITEM_WIELD ) )
    {
 /*
  int sn,skill;
 */
  if ( !IS_NPC(ch) && get_skill(ch,gsn_dual_wield) <= 0 )
  if ( !remove_obj( ch, WEAR_WIELD, fReplace ) )
      return;

  if ( !IS_NPC(ch)
  && get_obj_weight(obj) > (str_app[get_curr_stat(ch,STAT_STR)].wield
    * 10))
  {
      send_to_char( "It is too heavy for you to wield.\n\r", ch );
      return;
  }

  if ( is_affected(ch,gsn_bladesong) &&
       obj->value[0] != WEAPON_SWORD &&
       obj->value[0] != WEAPON_DAGGER)
  {
     send_to_char("You can only wear a sword or dagger while performing the bladesong.\n\r",ch);
     return;
  }

  if ( is_affected(ch,gsn_rage) && obj->value[0] != WEAPON_AXE )
  {
     send_to_char("You must wield an axe while battleraging.\n\r", ch);
     return;
  }

  if (!IS_NPC(ch) && ch->size < SIZE_LARGE
  &&  IS_WEAPON_STAT(obj,WEAPON_TWO_HANDS)
  &&  get_eq_char(ch,WEAR_SHIELD) != NULL)
  {
      send_to_char("You need two hands free for that weapon.\n\r",ch);
      return;
  }

  /* This gets tricky.  Check if they have dual wield.  If so, see if
     they're already equipped with a weapon.  If so, try to equip thisa
     as a secondary weapon, if THAT fails, replace the primary
     */

  if ( get_skill(ch,gsn_dual_wield) && get_eq_char(ch,WEAR_WIELD) != NULL )
  {
    OBJ_DATA *primary = get_eq_char(ch, WEAR_WIELD);
    OBJ_DATA *secondary = get_eq_char(ch, WEAR_SECOND);
    if(!IS_IMMORTAL(ch))
    {
      if(IS_SET(primary->extra_flags, ITEM_NOREMOVE))
      {
        /* Primary can't be removed, so end state has to still be wearing it */
        if(secondary != NULL && IS_SET(secondary->extra_flags, ITEM_NOREMOVE))
        {
          send_to_char("You can't remove either of your weapons!\n\r", ch);
          return;
        }

        if(obj->weight > primary->weight)
        {
          if(!can_wear_secondary(ch, obj, primary))
            return;
          if(!remove_obj(ch,WEAR_SECOND,fReplace))
            return;
          act( "$n transfer $p to $s off hand.", ch, primary, NULL, TO_ROOM ,FALSE);
          act( "You transfer $p to your off hand.", ch, primary, NULL, TO_CHAR ,FALSE);
          unequip_char(ch, primary);
          equip_char(ch, primary, WEAR_SECOND);

          act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
          act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
          equip_char( ch, obj, WEAR_WIELD );
        }
        else
        {
          if(!can_wear_secondary(ch, primary, obj))
            return;
          if(!remove_obj(ch,WEAR_SECOND,fReplace))
            return;
          act( "$n wields $p in $s off hand.", ch, obj, NULL, TO_ROOM ,FALSE);
          act( "You wield $p in your off hand.", ch, obj, NULL, TO_CHAR ,FALSE);
          equip_char( ch, obj, WEAR_SECOND );
        }

        return;
      }
      else if(secondary != NULL && IS_SET(secondary->extra_flags, ITEM_NOREMOVE))
      {
        /* Secondary can't be removed, check compatibility */
        if(obj->weight > secondary->weight)
        {
          if(!can_wear_secondary(ch, obj, secondary))
            return;
           if(!remove_obj(ch,WEAR_WIELD,fReplace))
            return;
          unequip_char(ch, secondary);
          equip_char(ch, secondary, WEAR_SECOND);
          act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
          act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
          equip_char( ch, obj, WEAR_WIELD );
        }
        else
        {
          if(!can_wear_secondary(ch, secondary, obj))
            return;
           if(!remove_obj(ch,WEAR_WIELD,fReplace))
            return;
          act( "$n transfers $p to $s main hand.", ch, secondary, NULL, TO_ROOM ,FALSE);
          act( "You transfer $p to your main hand.", ch, secondary, NULL, TO_CHAR ,FALSE);

          act("$n wields $p in $s off hand.", ch, obj, NULL, TO_ROOM,FALSE);
          act("You wield $p in your off hand.", ch, obj, NULL, TO_CHAR,FALSE);
          equip_char( ch, obj, WEAR_SECOND );
        }
        return;
      }
    }
    /* Neither weapon is no_remove or it's an immortal equipping it
 *        Wear it any way possible if no secondary is equipped, otherwise
 *               three outcomes: If it's heavier than secondary, remove primary and wear it.
 *                      If it's lighter or equal to secondary, wear it in the offhand slot.
 *                             Anything more complicated would probably be less intuitive to have succeed */
    if(secondary == NULL)
    {
      if(obj->weight > primary->weight)
      {
        if(!can_wear_secondary(ch, obj, primary))
        {
         /* Can't wear it as a dual?  Just replace it.*/
          if (!remove_obj(ch,WEAR_WIELD,fReplace))
            return;

          act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
          act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
          equip_char( ch, obj, WEAR_WIELD );
          return;
        }
        act( "$n transfers $p to $s off hand.", ch, primary, NULL, TO_ROOM ,FALSE);
        act( "You transfer $p to your off hand.", ch, primary, NULL, TO_CHAR ,FALSE);
        unequip_char(ch, primary);
        equip_char(ch, primary, WEAR_SECOND);

        act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
        act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
        equip_char( ch, obj, WEAR_WIELD );
      }
      else
      {
        if(!can_wear_secondary(ch, primary, obj))
        {
         /* Can't wear it as a dual?  Just replace it.*/
          if (!remove_obj(ch,WEAR_WIELD,fReplace))
            return;

          act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
          act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
          equip_char( ch, obj, WEAR_WIELD );
          return;
        }
        act( "$n wields $p in $s off hand.", ch, obj, NULL, TO_ROOM ,FALSE);
        act( "You wield $p in your off hand.", ch, obj, NULL, TO_CHAR ,FALSE);
        equip_char( ch, obj, WEAR_SECOND );
      }
      return;
    }
    if(obj->weight > secondary->weight)
    {
      if(!can_wear_secondary(ch, obj, secondary))
        return;
       if(!remove_obj(ch,WEAR_WIELD,fReplace))
        return;

      unequip_char(ch, secondary);
      equip_char(ch, secondary, WEAR_SECOND);

      act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
      act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
      equip_char( ch, obj, WEAR_WIELD );
      return;
    }
    if(!can_wear_secondary(ch, primary, obj))
      return;
    if(!remove_obj(ch,WEAR_SECOND,fReplace))
      return;
    act("$n wields $p in $s off hand.", ch, obj, NULL, TO_ROOM,FALSE);
    act("You wield $p in your off hand.", ch, obj, NULL, TO_CHAR,FALSE);
    equip_char( ch, obj, WEAR_SECOND );
    return;
  }
  else /*  Wield as primary */
  {
     if (!remove_obj(ch,WEAR_WIELD,fReplace))
         return;

     act("$n wields $p.", ch, obj, NULL, TO_ROOM,FALSE);
     act("You wield $p.", ch, obj, NULL, TO_CHAR,FALSE);
     equip_char( ch, obj, WEAR_WIELD );
     return;
  }
/*
        sn = get_weapon_sn(ch);

  if (sn == gsn_hand_to_hand)
     return;

        skill = get_weapon_skill(ch,sn);

        if (skill >= 100)
            act("$p feels like a part of you!",ch,obj,NULL,TO_CHAR,FALSE);
        else if (skill > 85)
            act("You feel quite confident with $p.",ch,obj,NULL,TO_CHAR,FALSE);
        else if (skill > 70)
            act("You are skilled with $p.",ch,obj,NULL,TO_CHAR,FALSE);
        else if (skill > 50)
            act("Your skill with $p is adequate.",ch,obj,NULL,TO_CHAR,FALSE);
        else if (skill > 25)
       act("$p feels a little clumsy in your hands.",ch,obj,NULL,TO_CHAR,FALSE);
        else if (skill > 1)
            act("You fumble and almost drop $p.",ch,obj,NULL,TO_CHAR,FALSE);
        else
            act("You don't even know which end is up on $p.",
                ch,obj,NULL,TO_CHAR,FALSE);

  return;
  */
    }

    if ( CAN_WEAR( obj, ITEM_HOLD ) )
    {
  if ( !remove_obj( ch, WEAR_HOLD, fReplace ) )
      return;
  act( "$n holds $p in $s hand.",   ch, obj, NULL, TO_ROOM ,FALSE);
  act( "You hold $p in your hand.", ch, obj, NULL, TO_CHAR ,FALSE);
  equip_char( ch, obj, WEAR_HOLD );
  return;
    }

    if ( CAN_WEAR(obj,ITEM_WEAR_FLOAT) )
    {
  if (!remove_obj(ch,WEAR_FLOAT, fReplace) )
      return;
  act("$n releases $p to float next to $m.",ch,obj,NULL,TO_ROOM,FALSE);
  act("You release $p and it floats next to you.",ch,obj,NULL,TO_CHAR,FALSE);
  equip_char(ch,obj,WEAR_FLOAT);
  return;
    }

    if ( fReplace )
  send_to_char( "You can't wear, wield, or hold that.\n\r", ch );

    return;
}



void do_wear( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Wear, wield, or hold what?\n\r", ch );
  return;
    }

    if ( !str_cmp( arg, "all" ) )
    {
  OBJ_DATA *obj_next;

  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
      obj_next = obj->next_content;
      if ( obj->wear_loc == WEAR_NONE && can_see_obj( ch, obj ) )
    wear_obj( ch, obj, FALSE );
  }
  return;
    }
    else
    {
  if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
  {
      send_to_char( "You do not have that item.\n\r", ch );
      return;
  }

  wear_obj( ch, obj, TRUE );
    }

    return;
}



void do_remove( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Remove what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_wear( ch, arg ) ) == NULL )
    {
  send_to_char( "You do not have that item.\n\r", ch );
  return;
    }

    remove_obj( ch, obj->wear_loc, TRUE );

/* Primary has been removed, move Secondary to Primary */
    if (get_eq_char(ch,WEAR_SECOND) != NULL &&
        get_eq_char(ch,WEAR_WIELD) == NULL)
    {
       obj = get_eq_char(ch,WEAR_SECOND);
       obj_from_char( obj );
       obj_to_char( obj, ch);
       equip_char( ch, obj, WEAR_WIELD );
    }

    return;
}



void do_sacrifice( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int silver;

    /* variables for AUTOSPLIT */
    CHAR_DATA *gch;
    int members;
    char buffer[100];


    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }



    if ( IS_NPC(ch))
    {
    send_to_char("You are unable to sacrifice anything, silly MOB.\n\r", ch);
    return;
    }

    if ( arg[0] == '\0' || !str_cmp( arg, ch->name ) )
    {
  act( "$n offers $mself to $s deity, who graciously declines.",
      ch, NULL, NULL, TO_ROOM ,FALSE);
  sprintf(buf,"%s appreciates your offer and may accept it later.\n\r",
          deity_table[ch->pcdata->deity].pname);
  send_to_char(buf,ch);
  return;
    }

    obj = get_obj_list( ch, arg, ch->in_room->contents );
    if ( obj == NULL )
    {
  send_to_char( "You can't find it.\n\r", ch );
  return;
    }

    if ( obj->item_type == ITEM_CORPSE_PC )
    {
  if (obj->contains)
        {
     sprintf(buf,"%s wouldn't like that.\n\r",deity_table[ch->pcdata->deity].pname);
     send_to_char(buf,ch);
     return;
        }
    }


    if ( !CAN_WEAR(obj, ITEM_TAKE) || CAN_WEAR(obj, ITEM_NO_SAC)
        || obj->item_type == ITEM_FURNITURE)
    {
  act( "$p is not an acceptable sacrifice.", ch, obj, 0, TO_CHAR ,FALSE);
  return;
    }

    silver = UMAX(1,obj->level * 3);

    if (obj->item_type != ITEM_CORPSE_NPC && obj->item_type != ITEM_CORPSE_PC)
      silver = UMIN(silver,obj->cost);

    if (obj->item_type == ITEM_CORPSE_NPC || obj->item_type == ITEM_CORPSE_PC)
    {
        sprintf(buf,"%s finds your offering pleasing and rewards you.\n\r",
                deity_table[ch->pcdata->deity].pname);
        send_to_char(buf,ch);
        ch->pcdata->sac += get_sac_points(ch, silver );
    }
    else
    {
       if(IS_SET(ch->mhs,MHS_GLADIATOR))
       {
          send_to_char("Sorry you are not permitted to sacrific things.\n\r",ch);
          return;
       }

       if ( obj->item_type == ITEM_CONTAINER )
       {
          if (!IS_SET(ch->mhs,MHS_FULL_SAC) && obj->contains)
          {
             sprintf(buf,"You have full sacing off, read 'help fullsac'.\n\r");
             send_to_char(buf,ch);
             return;
          }
       }

    if (silver == 1)
        {
        sprintf(buf,"%s gives you one silver coin for your sacrifice.\n\r",
                deity_table[ch->pcdata->deity].pname);
        send_to_char(buf,ch);
        }
    else
    {
  sprintf(buf,"%s gives you %d silver coins for your sacrifice.\n\r",
        deity_table[ch->pcdata->deity].pname,silver);
  send_to_char(buf,ch);
    }

    ch->silver += silver;

    if (IS_SET(ch->act,PLR_AUTOSPLIT) )
    { /* AUTOSPLIT code */
      members = 0;
  for (gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
      {
          if ( is_same_group( gch, ch ) )
            members++;
      }

  if ( members > 1 && silver > 1)
  {
      sprintf(buffer,"%d",silver);
      do_split(ch,buffer);
  }
    }
    }

    act( "$n sacrifices $p to $s deity.", ch, obj, NULL, TO_ROOM ,FALSE);
    wiznet("$N sends up $p as a burnt offering.",
     ch,obj,WIZ_SACCING,0,0);
    extract_obj( obj );
    return;
}

void do_mix( CHAR_DATA *ch, char *argument )
{
    return;
}

void do_create( CHAR_DATA *ch, char *argument )
{
    return;
}

void do_quaff( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    if ( arg[0] == '\0' )
    {
  send_to_char( "Quaff what?\n\r", ch );
  return;
    }

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
  send_to_char( "You do not have that potion.\n\r", ch );
  return;
    }

    if ( obj->item_type != ITEM_POTION )
    {
  send_to_char( "You can quaff only potions.\n\r", ch );
  return;
    }

    if (ch->level < obj->level)
    {
  send_to_char("This liquid is too powerful for you to drink.\n\r",ch);
  return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not quaff potions in the arena.\n\r", ch);
       return;
    }

    act( "$n quaffs $p.", ch, obj, NULL, TO_ROOM ,FALSE);
    act( "You quaff $p.", ch, obj, NULL ,TO_CHAR ,FALSE);

    obj_cast_spell( obj->value[1], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[2], obj->value[0], ch, ch, NULL );
    obj_cast_spell( obj->value[3], obj->value[0], ch, ch, NULL );

    extract_obj( obj );
    return;
}



void do_recite( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *scroll;
    OBJ_DATA *obj;
    int smod;


    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not recite scrolls in the arena.\n\r", ch);
       return;
    }

    if ( ( scroll = get_obj_carry( ch, arg1 ) ) == NULL )
    {
  send_to_char( "You do not have that scroll.\n\r", ch );
  return;
    }

    if ( scroll->item_type != ITEM_SCROLL )
    {
  send_to_char( "You can recite only scrolls.\n\r", ch );
  return;
    }

    if ( ch->level < scroll->level)
    {
  send_to_char(
    "This scroll is too complex for you to comprehend.\n\r",ch);
  return;
    }

    obj = NULL;
    if ( arg2[0] == '\0' )
    {
  victim = ch;
    }
    else
    {
  if ( ( victim = get_char_room ( ch, arg2 ) ) == NULL
  &&   ( obj    = get_obj_here  ( ch, arg2 ) ) == NULL )
  {
      send_to_char( "You can't find it.\n\r", ch );
      return;
  }
    }

    act( "$n recites $p.", ch, scroll, NULL, TO_ROOM ,FALSE);
    act( "You recite $p.", ch, scroll, NULL, TO_CHAR ,FALSE);

    if  (
           scroll->pIndexData->vnum == OBJ_VNUM_SCRIBE_PARCHMENT
        || scroll->pIndexData->vnum == OBJ_VNUM_SCRIBE_VELLUM
        || scroll->pIndexData->vnum == OBJ_VNUM_SCRIBE_RICEPAPER
        || scroll->pIndexData->vnum == OBJ_VNUM_SCRIBE_KOZO
        || scroll->pIndexData->vnum == OBJ_VNUM_SCRIBE_PAPYRUS
        )
        {
            smod = 5;
        }
    else
        {
            smod = 20;
        }
/*Origional line below commented out to make way for the scribe kit.  Scribed scrolls
are less effective than stock scrolls */

/*    if (number_percent() >= 20 + get_skill(ch,gsn_scrolls) * 4/5)*/
    if (number_percent() >= smod + get_skill(ch,gsn_scrolls) * 4/5)
    {
  send_to_char("You mispronounce a syllable.\n\r",ch);
  check_improve(ch,gsn_scrolls,FALSE,2);
    }

    else
    {
      obj_cast_spell( scroll->value[1], scroll->value[0], ch, victim, obj );
      obj_cast_spell( scroll->value[2], scroll->value[0], ch, victim, obj );
      obj_cast_spell( scroll->value[3], scroll->value[0], ch, victim, obj );
  check_improve(ch,gsn_scrolls,TRUE,2);
    }

    extract_obj( scroll );
    return;
}



void do_brandish( CHAR_DATA *ch, char *argument )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    OBJ_DATA *staff;
    int sn;

    int brandish_base;

    brandish_base = 20;
    if(ch->race == race_lookup("dwarf") )
    {
      brandish_base = 10;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not brandish staves in the arena.\n\r", ch);
       return;
    }

    if ( ( staff = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
  send_to_char( "You hold nothing in your hand.\n\r", ch );
  return;
    }

    if ( staff->item_type != ITEM_STAFF )
    {
  send_to_char( "You can brandish only with a staff.\n\r", ch );
  return;
    }

    if ( ( sn = staff->value[3] ) < 0
    ||   sn >= MAX_SKILL
    ||   skill_table[sn].spell_fun == 0 )
    {
  bug( "Do_brandish: bad sn %d.", sn );
  return;
    }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( staff->value[2] > 0 )
    {
  act( "$n brandishes $p.", ch, staff, NULL, TO_ROOM ,FALSE);
  act( "You brandish $p.",  ch, staff, NULL, TO_CHAR ,FALSE);
  if ( ch->level < staff->level
  ||   number_percent() >= brandish_base + get_skill(ch,gsn_staves) * 4/5)
  {
      act("You fail to invoke $p.",ch,staff,NULL,TO_CHAR,FALSE);
      act("...and nothing happens.",ch,NULL,NULL,TO_ROOM,FALSE);
      check_improve(ch,gsn_staves,FALSE,2);
  }

  else for ( vch = ch->in_room->people; vch; vch = vch_next )
  {
      vch_next  = vch->next_in_room;

      switch ( skill_table[sn].target )
      {
      default:
    bug( "Do_brandish: bad target for sn %d.", sn );
    return;

      case TAR_IGNORE:
    if ( vch != ch )
        continue;
    break;

      case TAR_CHAR_OFFENSIVE:
    if ( IS_NPC(ch) ? IS_NPC(vch) : !IS_NPC(vch) )
        continue;
    break;

      case TAR_CHAR_DEFENSIVE:
    if ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) )
        continue;
    break;

      case TAR_CHAR_SELF:
    if ( vch != ch )
        continue;
    break;
      }

      obj_cast_spell( staff->value[3], staff->value[0], ch, vch, NULL );
      check_improve(ch,gsn_staves,TRUE,2);
  }
    }

    if ( abs(--staff->value[2]) == 0 )
    {
  act( "$n's $p blazes bright and is gone.", ch, staff, NULL, TO_ROOM ,FALSE);
  act( "Your $p blazes bright and is gone.", ch, staff, NULL, TO_CHAR ,FALSE);
  extract_obj( staff );
    }

    return;
}



void do_zap( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *wand;
    OBJ_DATA *obj;
    int wand_base;

    wand_base = 20;
    if(ch->race == race_lookup("dwarf") )
    {
      wand_base = 10;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) &&
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You can not zap wands in the arena.\n\r", ch);
       return;
    }

    one_argument( argument, arg );
    if ( arg[0] == '\0' && ch->fighting == NULL )
    {
  send_to_char( "Zap whom or what?\n\r", ch );
  return;
    }

    if ( ( wand = get_eq_char( ch, WEAR_HOLD ) ) == NULL )
    {
  send_to_char( "You hold nothing in your hand.\n\r", ch );
  return;
    }

    if ( wand->item_type != ITEM_WAND )
    {
  send_to_char( "You can zap only with a wand.\n\r", ch );
  return;
    }
    if (wand->pIndexData->vnum == OBJ_VNUM_WAND_PINE ||
 wand->pIndexData->vnum == OBJ_VNUM_WAND_PINE   ||
 wand->pIndexData->vnum == OBJ_VNUM_WAND_APPLE  ||
 wand->pIndexData->vnum == OBJ_VNUM_WAND_OAK    ||
 wand->pIndexData->vnum == OBJ_VNUM_WAND_WILLOW ||
 wand->pIndexData->vnum == OBJ_VNUM_WAND_YEW )
     {
        wand_base /= 2;
     }

    obj = NULL;
    if ( arg[0] == '\0' )
    {
  if ( ch->fighting != NULL )
  {
      victim = ch->fighting;
  }
  else
  {
      send_to_char( "Zap whom or what?\n\r", ch );
      return;
  }
    }
    else
    {
  if ( ( victim = get_char_room ( ch, arg ) ) == NULL
  &&   ( obj    = get_obj_here  ( ch, arg ) ) == NULL )
  {
      send_to_char( "You can't find it.\n\r", ch );
      return;
  }
    }



   if ( victim != NULL)
   {
      if ( IS_NPC(victim) &&
           victim->fighting != NULL &&
          !is_same_group(ch,victim->fighting))
      {
         send_to_char("Kill stealing is not permitted.\n\r",ch);
         return;
      }
   }

    WAIT_STATE( ch, 2 * PULSE_VIOLENCE );

    if ( wand->value[2] > 0 )
    {
  if ( victim != NULL )
  {
      act( "$n zaps $N with $p.", ch, wand, victim, TO_ROOM ,FALSE);
      act( "You zap $N with $p.", ch, wand, victim, TO_CHAR ,FALSE);
  }
  else
  {
      act( "$n zaps $P with $p.", ch, wand, obj, TO_ROOM ,FALSE);
      act( "You zap $P with $p.", ch, wand, obj, TO_CHAR ,FALSE);
  }


  if (ch->level < wand->level
  ||  number_percent() >= wand_base + get_skill(ch,gsn_wands) * 4/5)
  {
      act( "Your efforts with $p produce only smoke and sparks.",
     ch,wand,NULL,TO_CHAR,FALSE);
      act( "$n's efforts with $p produce only smoke and sparks.",
     ch,wand,NULL,TO_ROOM,FALSE);
      check_improve(ch,gsn_wands,FALSE,2);
  }
  else
  {
      obj_cast_spell( wand->value[3], wand->value[0], ch, victim, obj );
      check_improve(ch,gsn_wands,TRUE,2);
  }
    }

    if ( --wand->value[2] <= 0 )
    {
  act( "$n's $p explodes into fragments.", ch, wand, NULL, TO_ROOM ,FALSE);
  act( "Your $p explodes into fragments.", ch, wand, NULL, TO_CHAR ,FALSE);
  extract_obj( wand );
    }

    return;
}

void do_snatch( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj, *t_obj, *n_obj;
    int percent;

    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );

    if  (!IS_NPC(ch) && get_skill(ch, gsn_snatch) < 2)
    {
      send_to_char( "That's a skill best left to professionals\n\r",ch );
      return;
    }
    if ( arg1[0] == '\0' && arg2[0] == '\0' )
    {
       send_to_char( " Syntax: snatch <float item> <victim>\n\r", ch);
       return;
    }
    if ( arg1[0] == '\0'  || arg2[0] == '\0' )
    {
      send_to_char( "Try to snatch what from who?\n\r", ch );
      return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }


//ok now that victim's actuall set lets do things to it
    if ( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
    {
       REMOVE_BIT(ch->affected_by, AFF_HIDE);
    }
    if ( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_SNEAK))
    {
       REMOVE_BIT(ch->affected_by, AFF_SNEAK);
    }

    if (is_clan(ch) && !IS_NPC(victim) && ch->pcdata->start_time > 0)
    {
      send_to_char("Easy there sparky.  You just got here.  Read some notes and such.\n\r",ch);
      return;
    }
    if (IS_SET(ch->mhs, MHS_GLADIATOR) || IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("You are not permitted to snatch.\n\r",ch);
       return;
    }



    if ( victim->fighting != NULL)// && !IS_AFFECTED(victim,AFF_BLIND) )
    {
      send_to_char("You cannot get close enough to them.\n\r",ch);
      return;
    }
    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("They are made of mist.\n\r",ch);
    return;
    }
    if ( ch->clan  && ch->in_room->clan )
    {
        send_to_char("Snatching items from your clan mates is illegal.\n\r", ch);
        return;
    }
    if ( victim == ch )
    {
      send_to_char( "That's {YPATHETIC{x.\n\r", ch );
      return;
    }
    if ( is_affected(ch,skill_lookup("hold person")))
    {
        send_to_char("Your muscles are frozen!\n\r",ch);
        return;
    }
    if( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
         REMOVE_BIT( ch->affected_by, AFF_HIDE );
    if ( ( obj = find_obj_wear(ch,victim, arg1) ) == NULL )
    {
      send_to_char("Your victim isn't wearing that!\n\r", ch);
      return;
    }
    if ( obj->wear_loc != WEAR_FLOAT )
    {
      send_to_char( "That's not a floating item.\n\r", ch );
      return;
    }
    if (is_safe_steal(ch,victim))
    {
       send_to_char("They are safe, leave them alone\n\r",ch);
       return;
    }
    if ( ch->move < (ch->level/15) )
    {
      send_to_char("You're too exhausted.\n\r",ch);
      return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));
    WAIT_STATE( ch, skill_table[gsn_snatch].beats) ;
    percent  = number_percent();
    /* Why the hell is this here?
     * Because those without the steal skill should NEVER succeed! -Rusty
     */
   sprintf(log_buf,"steal-bef:%s race %d cls %d lvl %d dex %d percent:%d victim %s lvl %d dex %d",ch->name,ch->race,ch->class,ch->level,
get_curr_stat(ch,STAT_DEX),percent,victim->name,victim->level,get_curr_stat(victim,STAT_DEX));
   log_string(log_buf);
    if (get_skill(ch,gsn_snatch) > 1)
    {
       percent  += ( IS_AWAKE(victim) ? 10 : -50 );
       percent += (victim->level - ch->level)*15;
       percent += (get_curr_stat(victim,STAT_DEX) - get_curr_stat(ch,STAT_DEX)) *2;
       if(IS_SET(victim->vuln_flags,VULN_DISTRACTION))
          percent -= ch->level/2;
       if(!IS_NPC(ch) && ch->pcdata->old_class != class_lookup("thief") )
          percent += 25;
       if(!IS_NPC(ch) && ch->class == class_lookup("rogue"))
          percent -= 25;
       if ( ch->race == race_lookup("kender") )
          percent -= 10;
    }
    else
    {
       /* garauntee failure */
       percent = 110;
    }

   sprintf(log_buf,"snatch-aft:%s skill %d percent:%d victim %s ",ch->name,(get_skill(ch,gsn_snatch)*9/10),percent,victim->name);
   log_string(log_buf);
    if ( ((ch->level -7 > victim->level)
          && !IS_NPC(victim) && !IS_NPC(ch) )
       || ( !IS_NPC(ch) && percent > get_skill(ch,gsn_snatch) * 9 / 10 )
       || ( !IS_NPC(victim) && !is_clan(ch)) )
    {
  /*
   * Failure.
   */
       send_to_char( "{RDOH!!!{x\n\r", ch );
       act( "{Y$n tried to snatch your floater!{x\n\r", ch, NULL, victim, TO_VICT    ,FALSE);
       act( "{Y$n tried to snatch a floater from $N{x.\n\r",  ch, NULL, victim, TO_NOTVICT ,FALSE);
       if (IS_AWAKE(victim))
       {
          switch(number_range(0,5))
          {
             case 0 :
                sprintf( buf, "%s is a lousy thief!", ch->name );
                break;
             case 1 :
                sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
                  ch->name,(ch->sex == 2) ? "her" : "his");
                break;
             case 2 :
                sprintf( buf,"%s tried to grope me!",ch->name );
                break;
             case 3 :
                sprintf(buf,"Keep your hands out of there, %s!",ch->name);
                break;
             case 4 :
                sprintf(buf,"%s touched me in my special place! Help!", ch->name);
                break;
             case 5 :
                sprintf(buf, "Don't touch me there %s!", ch->name);
                break;
          }
          do_yell( victim, buf );
       }
       if ( !IS_NPC(ch) )
       {
          if ( IS_NPC(victim) )
          {
             check_improve(ch,gsn_steal,FALSE,2);
             multi_hit( victim, ch, TYPE_UNDEFINED );
          }
          else
          {
             ch->pcdata->quit_time = 5;
             sprintf(buf,"$N tried to snatch from %s.",victim->name);
             wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
             if (((IS_SET(ch->act,PLR_THIEF) || IS_SET(ch->act,PLR_KILLER))
                   && ch->level+12 < victim->level )
                   || (ch->trumps == 0 && ch->level+8 < victim->level)
                   || (ch->trumps > 0 && ch->level+10 < victim->level ) )
                victim->pcdata->last_attacked_by = str_dup(ch->name);
               if ( !IS_SET(ch->act, PLR_THIEF)
               &&   !IS_SET(victim->act, PLR_THIEF)
                  )
             {
                  SET_BIT(ch->act, PLR_THIEF);
                  send_to_char( "***{Y You are now a THIEF!!{x ***\n\r", ch );
                  sprintf(buf,"%s got THIEF on %s in room %d",ch->name,
                  victim->name,ch->in_room->vnum);
                  log_string(buf);
                  save_char_obj( ch );
             }
          }
       }
       return;
    }

    if ( !can_drop_obj( victim, obj )
    ||   (obj->level - 2) > ch->level )
    {
  send_to_char( "You can't pry it away.\n\r", ch );
  return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
  send_to_char( "You have your hands full.\n\r", ch );
  return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
  send_to_char( "You can't carry that much weight.\n\r", ch );
  return;
    }

    obj_from_char( obj );
    obj_to_char( obj, ch );
    check_improve(ch,gsn_snatch,TRUE,2);
    obj->stolen_timer += 10 * number_fuzzy(5);
    if (!IS_NPC(victim))
    {
       sprintf(buf,"$N snatched %s from %s.",obj->short_descr,victim->name);
       wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
       ch->pcdata->steal_data[PC_STOLEN_ITEMS] += 1 ;
    }
    send_to_char( "Got it!\n\r", ch );
    return;
}

void do_slice( CHAR_DATA *ch, char *argument )
{
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj, *t_obj, *n_obj;
    int percent;
    int count = 0;
    int item_bonus = 0, item_chance = 33;

    if (IS_SET(ch->mhs, MHS_GLADIATOR) || IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("You are not permitted to slice.\n\r",ch);
       return;
    }
    if (ch->clan == nonclan_lookup("smurf"))
    {
       send_to_char("You are not permitted to slice.\n\r",ch);
       return;
    }
    argument = one_argument( argument, arg1 );
    one_argument( argument, arg2 );


    if ( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
       REMOVE_BIT(ch->affected_by, AFF_HIDE);

    if  (!IS_NPC(ch) && get_skill(ch, gsn_slice) < 2)
    {
      send_to_char( "That's a skill best left to professionals.\n\r",ch );
      return;
    }
    if ( arg1[0] == '\0' && arg2[0] == '\0' )
       {
       send_to_char( " Syntax: slice <container> <victim>\n\r", ch);
       return;
       }

    if ( arg1[0] == '\0'  || arg2[0] == '\0' )
    {
      send_to_char( "Try to cut open whose containers?\n\r", ch );
      return;
    }

    if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
    {
      send_to_char( "They aren't here.\n\r", ch );
      return;
    }

    if ( victim == ch )
    {
      send_to_char( "That's pointless.\n\r", ch );
      return;
    }

    if ( is_affected(ch,skill_lookup("hold person")))
    {
        send_to_char("Your muscles are frozen!\n\r",ch);
        return;
    }

    if((obj = get_eq_char(ch, WEAR_HOLD)) == NULL)
    {
      send_to_char("You must be holding a cutting tool to slice.\n\r", ch);
      return;
    }

    switch(obj->pIndexData->vnum)
    {
      case OBJ_VNUM_CUTTING_1: 
        if(ch->level > 30)
        {
          send_to_char("That tool is too weak for you to slice with.\n\r", ch);
          return;
        }
        item_bonus = 0; item_chance = 10; break;
      case OBJ_VNUM_CUTTING_2:
        if(ch->level > 40)
        {
          send_to_char("That tool is too weak for you to slice with.\n\r", ch);
          return;
        }
        item_bonus = 0; item_chance = 10; break;
      case OBJ_VNUM_CUTTING_3: item_bonus = 0; item_chance = 10; break;
      default: send_to_char("You must be holding a cutting tool to slice.\n\r", ch);  return;
    }

    if ( victim->fighting != NULL || victim->alert > 0)// && !IS_AFFECTED(victim,AFF_BLIND) )
    {
      send_to_char("You cannot get close enough to them.\n\r",ch);
      return;
    }


    if( is_affected(victim,skill_lookup("wraithform")) )
    {
    send_to_char("They are made of mist.\n\r",ch);
    return;
    }



     if( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
         REMOVE_BIT( ch->affected_by, AFF_HIDE );

    if ( ( obj = find_obj_wear(ch,victim, arg1) ) == NULL )
    {
      send_to_char("Your victim isn't wearing that!\n\r", ch);
      return;
    }

    if ( obj->item_type != ITEM_CONTAINER
        || obj->pIndexData->vnum == OBJ_VNUM_DISC )
    {
      send_to_char( "That's not a container.\n\r", ch );
      return;
    }
    if (is_clan(ch) && !IS_NPC(victim) && ch->pcdata->start_time > 0)
    {
      send_to_char("Easy there sparky.  You just got here.  Read some notes and such.\n\r",ch);
      return;
    }

    if (is_safe_steal(ch,victim))
    {
       send_to_char("They are safe, leave them alone\n\r",ch);
       return;
    }

    if ( ch->move < (ch->level/15) )
    {
      send_to_char("You're too exhausted.\n\r",ch);
      return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if (ch->in_room->clan && ch->in_room->clan != ch->clan)
    {
    WAIT_STATE( ch,( skill_table[gsn_slice].beats)*3/2 );
    }

    /*  This code removed by NIGHTDAGGER
    else if (!IS_NPC(ch) && ch->clan == clan_lookup("avarice"))
    {
    WAIT_STATE(ch, skill_table[gsn_slice].beats/2);
    }
    End code nerf */

    else
    {
    WAIT_STATE( ch, skill_table[gsn_slice].beats);
    }

    percent = get_skill(ch,gsn_slice);
    percent -= ( IS_AWAKE(victim) ? 10 : -50 );
    percent -= (victim->level - ch->level)*20;
    percent -= ( (get_curr_stat(victim, STAT_INT) + get_curr_stat(victim, STAT_WIS))/2
                        - get_curr_stat(ch, STAT_INT))*5;
    percent -= ( get_curr_stat(victim, STAT_DEX) - get_curr_stat(ch, STAT_DEX))*2;
    /*percent -= (obj->level - ch->level);*/
    percent += item_bonus;
    if (IS_OBJ_STAT(obj, ITEM_BURN_PROOF))
        percent -= 25;
    if ( ch->race == race_lookup("kender") )
        percent += 10;
    if(IS_SET(victim->vuln_flags, VULN_DISTRACTION))
        percent += ch->level/2;

    if ( ( (ch->level -7 > victim->level)
         && !IS_NPC(victim) && !IS_NPC(ch) )
    || ( !IS_NPC(ch) && percent < number_percent())
    || ( !IS_NPC(victim) && !is_clan(ch) ) )
    {
      send_to_char("Doh!\n\r",ch);
      check_improve(ch,gsn_slice,FALSE,2);
      if (!IS_SET(ch->act, PLR_THIEF) && !IS_SET(victim->act, PLR_THIEF)
         && !IS_NPC(victim))
         {
           SET_BIT(ch->act, PLR_THIEF);
           send_to_char("*****You are now a THIEF*****\n\r",ch);
           save_char_obj(ch);
         }
      damage(ch,victim,0,gsn_slice,DAM_OTHER,FALSE,TRUE);
    }
    else
    {
       damage(victim,victim,0,gsn_slice,DAM_OTHER,FALSE,TRUE);
       check_improve(ch,gsn_slice,TRUE,1);
       send_to_char("Nicely done!\n\r",ch);
       if (!IS_NPC(victim)) ch->pcdata->steal_data[PC_SLICES] += 1 ;
     /* This is the fun part.  If the attempt succeeds, dump the contents of the container
      * into the victim's inventory.
      */
      if (obj->contains)  /* dump contents */
       {
         int chance = 100;
         for (t_obj = obj->contains; t_obj != NULL; t_obj = t_obj->next_content)
         {
           count++;
           /*n_obj = t_obj->next_content;
           if (number_percent() < item_chance)//((ch->level)*3/2))
           {
           obj_from_obj(t_obj);
           obj_to_char(t_obj, victim);
           }*/
         }
         while(count)
         {
           count--;
           if(number_percent() < chance)
           {
             int to_drop = number_range(1, count);
             for (t_obj = obj->contains; t_obj != NULL; t_obj = t_obj->next_content)
             {
               to_drop--;
               if(!to_drop)
               {
                 obj_from_obj(t_obj);
                 break;
               }
             }
             chance -= item_chance;
           }
           else
             count = 0;
         }
      }
    }

    return;
}

void do_bump( CHAR_DATA *ch, char *argument )
{
  char arg1[256], arg2[256];
  bool stealing = FALSE;
  int chance;
  CHAR_DATA *victim;
  if (get_skill(ch,gsn_bump) < 1)
  {
    send_to_char("You don't know how to bump someone.\n\r", ch );
    return;
  }

  argument = one_argument(argument, arg1);
  argument = one_argument(argument, arg2);


  if(ch->fighting == NULL)
  {
    if(arg1[0] == '\0')
    {
      send_to_char("Bump who or what from who?\n\r", ch);
      return;
    }
    if(arg2[0] != '\0')
      victim = get_char_room(ch, arg2);
    else
      victim = get_char_room(ch, arg1);
    if(victim == NULL)
    {
      send_to_char("They aren't here.\n\r", ch);
      return;
    }
    stealing = TRUE;
  }
  else
  {
    if(arg2[0] != '\0')
    {
      victim = get_char_room(ch, arg2);
      if(victim != ch->fighting)
      {
        send_to_char("You are fighting someone else!\n\r", ch);
        return;
      }
      stealing = TRUE;
    }
    else if(arg1[0] != '\0')
    {
      victim = get_char_room(ch, arg1);
      if(victim != ch->fighting)
      {/* Assume they're trying to steal an item named victim name from their current target */
      /* Bad luck if they didn't mean to steal, should be more careful. */
        victim = ch->fighting;
        stealing = TRUE;
      }
    }
    else
      victim = ch->fighting;
  }

  if (victim == ch)
  {
    send_to_char("Strut your stuff!\n\r", ch );
    return;
  }

  if(is_safe(ch, victim))
    return;

  if ( ch->move < (ch->level/7) )
  {
      send_to_char("You're too exhausted to bump into them properly.\n\r",ch);
      return;
  }
  ch->move -= apply_chi(ch,(ch->level/7));
  WAIT_STATE(ch, skill_table[gsn_bump].beats);
  damage(ch,victim,0,gsn_bump,DAM_BASH,FALSE,FALSE);

  /* The bump part is sheer size and strength */

  chance = get_skill(ch, gsn_bump);

  if (ch->size < victim->size)
    chance += (ch->size - victim->size) * 15;
  else
    chance += (ch->size - victim->size) * 10;

  chance += ch->carry_weight / 400;
  chance -= victim->carry_weight / 500;

  /* stats */
  chance += get_curr_stat(ch,STAT_STR);
  chance -= get_curr_stat(victim,STAT_STR) / 2;

  /* level */
  chance += (ch->level - victim->level);

  /* magic */
  if ( is_affected(victim,gsn_fumble) )
    chance += 15;

  /* Very hard to bump into someone if they're mounted and you're not */
  if ( is_mounted( victim ) && !is_mounted(ch))
    chance /= 2;

  if(number_percent() < chance)
  {
    act("$n bumps into you, knocking you off balance.", ch,NULL,victim,TO_VICT,FALSE);
    act("You bump into $N, and knock $M off balance.",ch,NULL,victim,TO_CHAR,FALSE);
    act("$n bumps into $N and knocks $M off balance.", ch,NULL,victim,TO_NOTVICT,FALSE);
    check_improve(ch,gsn_bump,TRUE,1);
    DAZE_STATE(victim, PULSE_VIOLENCE);
  }
  else
  {
    act("$n bumps into you to knock you off balance, but you stagger $m instead.", ch,NULL,victim,TO_VICT,FALSE);
    act("You try to bump into $N but $e sends you staggering instead.",ch,NULL,victim,TO_CHAR,FALSE);
    act("$n bumps into $N but can't knock $M off balance.", ch,NULL,victim,TO_NOTVICT,FALSE);
    check_improve(ch,gsn_bump,FALSE,1);
    DAZE_STATE(ch, PULSE_VIOLENCE / 2);
  }

  if(stealing)
  {
    if ( ch->move < (ch->level/15) )
    {
      send_to_char("You're too exhausted to steal after bumping into them.\n\r",ch);
      return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));
    if(!str_cmp(arg1, "merit"))
    {
      send_to_char("You may not steal merit by bumping into someone.\n\r", ch);
      return;
    }
    steal(ch, arg1, victim);
  }
}

void steal(CHAR_DATA *ch, char *arg1, CHAR_DATA *victim)
{
  char buf[256];
  OBJ_DATA *obj;
  int percent;
  bool was_hidden = FALSE;
  bool merit_steal = FALSE;

  if (IS_SET(ch->mhs, MHS_GLADIATOR) || IS_SET(ch->mhs,MHS_HIGHLANDER))
  {
     send_to_char("You are not permitted steal.\n\r",ch);
     return;
  }

  if ( IS_NPC(victim)
  && victim->position == POS_FIGHTING && ch->fighting != victim)
  {
send_to_char("You'd better not -- you might get hit.\n\r",ch);
return;
  }

  if (get_skill(ch,gsn_steal) < 1)
  {
send_to_char( "Leave the stealing to the thieves.\n\r", ch );
return;
  }

  /*  This code killed by NIGHTDAGGER
  if(!IS_NPC(ch) && ch->clan == clan_lookup("avarice") && !IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1) && !is_affected(ch,  skill_lookup("speed")) && !IS_SET(ch->res_flags, RES_DELAY) )
  {
  WAIT_STATE(ch, skill_table[gsn_steal].beats/2);
  }
  else
  {
  WAIT_STATE( ch, skill_table[gsn_steal].beats );
  }
  End code nerf */

  WAIT_STATE( ch, skill_table[gsn_steal].beats) ;

  if( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
  {
    REMOVE_BIT( ch->affected_by, AFF_HIDE );
    was_hidden = TRUE;
  }

  if(victim->position == POS_FIGHTING && ch->fighting != victim)
  {/* The're moving around a lot and it's not a bump */
    if(number_percent() < get_curr_stat(victim, STAT_DEX) * 2)
    {
      act("While fighting, $N makes a sudden movement! You narrowly avoid being caught.", ch, NULL, victim, TO_CHAR, FALSE);
      return;
    }
  }
  percent  = number_percent();
  /* Why the hell is this here?
   * Because those without the steal skill should NEVER succeed! -Rusty
   */
 if((ch->level -7 > victim->level)
        && !IS_NPC(victim) && !IS_NPC(ch) )
  {
    send_to_char("They are too weak for you, pick on someone else.\n\r", ch);
    return;
  }
 sprintf(log_buf,"steal-bef:%s race %d cls %d lvl %d dex %d percent:%d victim %s lvl %d dex %d",ch->name,ch->race,ch->class,ch->level,get_curr_stat(ch,STAT_DEX),percent,victim->name,victim->level,get_curr_stat(victim,STAT_DEX));
 log_string(log_buf);
  if (get_skill(ch,gsn_steal) > 1)
  {
    if(IS_AWAKE(victim))
    {/* Check how many other people are in the room and seen by the victim */
      if(IS_NPC(victim))/* NPCs don't care how many people are around */
        percent += 20;
      else
      {
        int count = 0;
        CHAR_DATA *in_room = ch->in_room->people;
        for(; in_room; in_room = in_room->next_in_room)
        {
          if(in_room != ch && in_room != victim && !IS_NPC(in_room) && can_see(victim, in_room, FALSE))
            count++;
        }
        if(!count)/* Blind offsets -30, so nets -15 */
          percent += 15;
        else
          percent -= count * 5; /* The more people around, the more distracted the victim is */
      }
    }
    else
      percent -= 50;
    percent += (victim->level - ch->level)*15;
    percent += (get_curr_stat(victim,STAT_DEX) - get_curr_stat(ch,STAT_DEX)) *2;
    percent += (get_curr_stat(victim,STAT_INT) + get_curr_stat(victim,STAT_WIS)) / 2;
    if(IS_SET(victim->vuln_flags,VULN_DISTRACTION))
      percent -= ch->level/2;/* Negates alert AND provides bonus */
    else if(victim->alert)
      percent += 25;
    if(!IS_NPC(ch))
    {
      if(ch->pcdata->old_class != class_lookup("thief") )
        percent += 25;
      else if(ch->class == class_lookup("monk")  )
      {
          percent += 15;
      }
      else if( ch->class == class_lookup("berzerker")  )
      {
          percent += 20;
      }
      else if(ch->class == class_lookup("assassin"))
         percent -= 15;
      else if(ch->class == class_lookup("rogue"))
         percent -= 25;
    }
    if(ch->size == victim->size)
      percent -= 5; /* Very small bonus for being the same size */
    if ( ch->race == race_lookup("kender") )
      percent -= 10;
    if(IS_AFFECTED(ch,AFF_SNEAK))
      percent -= 10;
    if(was_hidden)
      percent -= 10;
    if(IS_AFFECTED(ch, AFF_INVISIBLE))
      percent -= 5; /* Small bonus */
    if(IS_AFFECTED(victim,AFF_BLIND))
      percent -= 30; /* Blinded victims are easier to steal from - must offset not seeing anyone else, too*/
    percent -= victim->daze;
    if(!str_cmp(arg1, "merit"))
    {
      merit_steal = TRUE;
      percent += 25; /* It's just flat out harder */
    }
  }
  else
  {
     /* guarantee failure */
     percent = 110;
  }

 sprintf(log_buf,"steal-aft:%s skill %d percent:%d victim %s ",ch->name,(get_skill(ch,gsn_steal)*9/10),percent,victim->name);
 log_string(log_buf);
  if (
      ( !IS_NPC(ch) && percent > get_skill(ch,gsn_steal) * 9 / 10 )
     || ( !IS_NPC(victim) && !is_clan(ch)) )
  {
/*
 * Failure.
 */
     send_to_char( "Oops.\n\r", ch );
     act( "$n tried to steal from you.\n\r", ch, NULL, victim, TO_VICT    ,FALSE);
     act( "$n tried to steal from $N.\n\r",  ch, NULL, victim, TO_NOTVICT ,FALSE);
     if (IS_AWAKE(victim))
     {
        switch(number_range(0,5))
        {
           case 0 :
              sprintf( buf, "%s is a lousy thief!", ch->name );
              break;
           case 1 :
              sprintf( buf, "%s couldn't rob %s way out of a paper bag!",
                ch->name,(ch->sex == 2) ? "her" : "his");
              break;
           case 2 :
              sprintf( buf,"%s tried to grope me!",ch->name );
              break;
           case 3 :
              sprintf(buf,"Keep your hands out of there, %s!",ch->name);
              break;
           case 4 :
              sprintf(buf,"%s touched me in my special place! Help!", ch->name);
              break;
           case 5 :
              sprintf(buf, "Don't touch me there %s!", ch->name);
              break;
        }
        do_yell( victim, buf );
     }

     if ( !IS_NPC(ch) )
     {
        if ( IS_NPC(victim) )
        {
           check_improve(ch,gsn_steal,FALSE,2);
           multi_hit( victim, ch, TYPE_UNDEFINED );
        }
        else
        {
           ch->pcdata->quit_time = 5;
           sprintf(buf,"$N tried to steal from %s.",victim->name);
           wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);

           if (((IS_SET(ch->act,PLR_THIEF) || IS_SET(ch->act,PLR_KILLER))
                 && ch->level+12 < victim->level )
                 || (ch->trumps == 0 && ch->level+8 < victim->level)
                 || (ch->trumps > 0 && ch->level+10 < victim->level ) )
              victim->pcdata->last_attacked_by = str_dup(ch->name);

           if (
                (!IS_SET(ch->act, PLR_THIEF) &&
                !IS_SET(victim->act, PLR_THIEF)
                )
              )
           {
                SET_BIT(ch->act, PLR_THIEF);
                send_to_char( "*** You are now a THIEF!! ***\n\r", ch );
                sprintf(buf,"%s got THIEF on %s in room %d",ch->name,
                victim->name,ch->in_room->vnum);
                log_string(buf);
                save_char_obj( ch );
           }
        }
     }
     return;
  }

  if(merit_steal)
  {
    int limit_percent, impress_percent;
    int amount, limit;
    int min_percent, max_percent, bonus_percent;
    MERIT_TRACKER *mtrack;
    int tally = 0;
    if(IS_NPC(victim) || IS_NPC(ch) || !victim->pcdata->clan_info || !ch->pcdata->clan_info)
      return;/* Shouldn't have gotten here */
    amount = UMAX(100, (ch->level - 1) * (ch->level - 1) * 4); /* Turn in amount */
    /* Variable in case of future changes to it (From balance or from a clan skill) */
    limit_percent = 1500;
    min_percent = 500;
    max_percent = 800;
    bonus_percent = 200;
    impress_percent = 900; /* The point where you get the impress message */
    limit = amount * limit_percent / 10000;/* Max of 15% */
    amount = amount * number_range(min_percent, max_percent) / 10000; /* 5-8% of turn in stolen per shot, big range for more variety */
    if(number_percent() < get_curr_stat(ch, STAT_INT) * 2)
    {/* At 25 int this drives the average up to 5-10% range */
      amount += limit * bonus_percent / 10000; /* 2% more */
    }
    /* Delay merit stolen first */
    while(victim->pcdata->clan_info->delay_merit)
    {/* Oldest should be first */
      mtrack = victim->pcdata->clan_info->delay_merit;
      if(mtrack->amount + tally > amount)
      {
        mtrack->amount -= (amount - tally);
        tally = amount;
        break;
      }
      tally += mtrack->amount;
      /* It's dead */
      victim->pcdata->clan_info->delay_merit = mtrack->next;
      free_merit(mtrack);
      if(tally >= amount) /* Should only be ==, but being safe */
        break;
    }
    if(tally < amount)
    {/* Check if they have any "live" merit to steal */
      if(victim->pcdata->clan_info->merit && victim->pcdata->clan_info->lost_merit < limit)
      {
        int cur_limit = limit - victim->pcdata->clan_info->lost_merit; /* Max possible */
        if(victim->pcdata->clan_info->merit < cur_limit)
          cur_limit = victim->pcdata->clan_info->merit; /* They don't have enough to hit their limit */
        if(cur_limit + tally > amount)
        {
          victim->pcdata->clan_info->merit -= (amount - tally);
          victim->pcdata->clan_info->lost_merit += (amount - tally);
          tally = amount;
        }
        else
        {
          tally += cur_limit;
          /* Do not zero merit, the limit may be 15% turn-in NOT total */
          victim->pcdata->clan_info->merit -= cur_limit;
          victim->pcdata->clan_info->lost_merit += cur_limit;
        }
      }
    }
    if(tally > limit * impress_percent / limit_percent)
      sprintf(buf, "Wow, you really impressed someone!  You stole %d merit from %s.\n\r", tally, victim->name);
    else if(tally)
      sprintf(buf, "You have stolen %d merit from %s!\n\r", tally, victim->name);
    else
      sprintf(buf, "You successfully tried to steal merit, but %s had none available.\n\r", victim->name);
    send_to_char(buf, ch);
    if(tally)
    {/* Stolen merit has an hour delay before it's secured */
      mtrack = new_merit();
      mtrack->next = ch->pcdata->clan_info->delay_merit;
      ch->pcdata->clan_info->delay_merit = mtrack;
      mtrack->amount = tally;
      mtrack->expire = 90;
      ch->pcdata->merit_stolen += tally;
    }
    return;
  }

  if ( !str_cmp( arg1, "coin"  )
    ||   !str_cmp( arg1, "coins" )
    ||   !str_cmp( arg1, "gold"  )
    ||   !str_cmp( arg1, "silver"))
    {
  int gold, silver;

  gold = victim->gold * number_range(1, ch->level) / MAX_LEVEL;
  silver = victim->silver * number_range(1,ch->level) / MAX_LEVEL;
  if ( gold <= 0 && silver <= 0 )
  {
      send_to_char( "You couldn't get any coins.\n\r", ch );
      return;
  }

  ch->gold      += gold;
  ch->silver    += silver;
  victim->silver  -= silver;
  victim->gold  -= gold;
  if (silver <= 0)
      sprintf( buf, "Bingo!  You got %d gold coins.\n\r", gold );
  else if (gold <= 0)
      sprintf( buf, "Bingo!  You got %d silver coins.\n\r",silver);
  else
      sprintf(buf, "Bingo!  You got %d silver and %d gold coins.\n\r",
        silver,gold);

  send_to_char( buf, ch );
  if (!IS_NPC(victim))
  {
     sprintf(buf,"$N stole %d gold and %d silver from %s.",
          gold,silver,victim->name);
     wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
     ch->pcdata->steal_data[PC_STOLEN_GOLD] += gold ;
     ch->pcdata->steal_data[PC_STOLEN_GOLD] += silver/100 ;
  }
  check_improve(ch,gsn_steal,TRUE,2);
  return;
    }

    if ( ( obj = find_obj_carry( ch, victim, arg1 ) ) == NULL )
    {
  send_to_char( "You can't find it.\n\r", ch );
  return;
    }

    if ( !can_drop_obj( victim, obj )
    ||   IS_SET(obj->extra_flags, ITEM_INVENTORY)
    ||   (obj->level - 2) > ch->level )
    {
  send_to_char( "You can't pry it away.\n\r", ch );
  return;
    }

    if ( ch->carry_number + get_obj_number( obj ) > can_carry_n( ch ) )
    {
  send_to_char( "You have your hands full.\n\r", ch );
  return;
    }

    if ( ch->carry_weight + get_obj_weight( obj ) > can_carry_w( ch ) )
    {
  send_to_char( "You can't carry that much weight.\n\r", ch );
  return;
    }

    obj_from_char( obj );
    obj_to_char( obj, ch );
    check_improve(ch,gsn_steal,TRUE,2);
    obj->stolen_timer += 10 * number_fuzzy(5);
    if (!IS_NPC(victim))
    {
       sprintf(buf,"$N stole %s from %s.",obj->short_descr,victim->name);
       wiznet(buf,ch,NULL,WIZ_FLAGS,0,0);
       ch->pcdata->steal_data[PC_STOLEN_ITEMS] += 1 ;
    }
    act("Got it! You stole $p from $N.", ch, obj, victim, TO_CHAR, FALSE);
//    send_to_char( "Got it!\n\r", ch );
}

void do_steal( CHAR_DATA *ch, char *argument )
{
    char buf  [MAX_STRING_LENGTH];
    char arg1 [MAX_INPUT_LENGTH];
    char arg2 [MAX_INPUT_LENGTH];
    CHAR_DATA *victim;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (ch->clan == nonclan_lookup("smurf"))
    {
       send_to_char("You are not permitted to steal.\n\r",ch);
       return;
    }

    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
  send_to_char( "Steal what from whom?\n\r", ch );
  return;
    }

    if(ch->fighting != NULL)
      victim = ch->fighting; /* Bump safety override in case of two people with same name start */
    else
    {
      if ( ( victim = get_char_room( ch, arg2 ) ) == NULL )
      {
    send_to_char( "They aren't here.\n\r", ch );
    return;
      }
    }

    if ( is_affected(ch,skill_lookup("hold person")))
    {
      send_to_char("Your muscles are frozen!\n\r",ch);
      return;
    }

    if (is_safe(ch,victim))
    {
      send_to_char("Not on that person.\n\r",ch);
      return;
    }

    if ( victim == ch )
    {
  send_to_char( "That's pointless.\n\r", ch );
  return;
    }

    if ( ch->move < (ch->level/15) )
    {
      send_to_char("You're too exhausted.\n\r",ch);
      return;
    }
    ch->move -= apply_chi(ch,(ch->level/15));

    if(!str_cmp( arg1, "merit"))
    {/* Special case, send them into the merit steal pulse check */
      if(IS_NPC(victim))
      {
        send_to_char("NPCs don't have merit for you to steal.\n\r", ch);
        return;
      }
      if(IS_NPC(ch))
      {
        send_to_char("NPCs may not steal merit.\n\r", ch);
        return;
      }
      if(!ch->pcdata->clan_info)
      {
        send_to_char("You can't steal merit yet, join a new clan first.\n\r", ch);
        return;
      }
      if(!victim->pcdata->clan_info)
      {
char buf[256];
sprintf(buf, "%s is your target, %s is you.\n\r", victim->name, ch->name);
send_to_char(buf, ch);
        send_to_char("They can't have merit for you to steal, they are in an old clan.\n\r", ch);
        return;
      }
      REMOVE_BIT( ch->affected_by, AFF_HIDE );
      ch->pcdata->pulse_timer = skill_table[gsn_steal].beats;
      ch->pcdata->pulse_type = PULSE_STEALMERIT;
      ch->pcdata->pulse_target = victim;
      sprintf(buf, "You begin to demonstrate to the gods why you deserve merit more than %s.\n\r", victim->name);
      send_to_char(buf, ch);
      return;
    }
    steal(ch, arg1, victim);
    return;
}



/*
 * Shopping commands.
 */
CHAR_DATA *find_keeper( CHAR_DATA *ch )
{
    /*char buf[MAX_STRING_LENGTH];*/
    CHAR_DATA *keeper;
    SHOP_DATA *pShop;

    pShop = NULL;
    for ( keeper = ch->in_room->people; keeper; keeper = keeper->next_in_room )
    {
  if ( IS_NPC(keeper) && (pShop = keeper->pIndexData->pShop) != NULL )
      break;
    }

    if ( pShop == NULL )
    {
  send_to_char( "You can't do that here.\n\r", ch );
  return NULL;
    }

    /*
     * Undesirables.
     *
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_KILLER) )
    {
  do_say( keeper, "Killers are not welcome!" );
  sprintf( buf, "%s the KILLER is over here!\n\r", ch->name );
  do_yell( keeper, buf );
  return NULL;
    }

    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_THIEF) )
    {
  do_say( keeper, "Thieves are not welcome!" );
  sprintf( buf, "%s the THIEF is over here!\n\r", ch->name );
  do_yell( keeper, buf );
  return NULL;
    }
  */
    /*
     * Shop hours.
     */
    if ( time_info.hour < pShop->open_hour )
    {
  do_say( keeper, "Sorry, I am closed. Come back later." );
  return NULL;
    }

    if ( time_info.hour > pShop->close_hour )
    {
  do_say( keeper, "Sorry, I am closed. Come back tomorrow." );
  return NULL;
    }

    /*
     * Invisible or hidden people.
     */
/*COREY PUT THAT STUFF FOR FENCE HERE*/
   if ( (!can_see( keeper, ch, FALSE ) &&  ch->kit != kit_lookup("fence")) )
    {
  do_say( keeper, "I don't trade with folks I can't see." );
  return NULL;
    }

    return keeper;
}

/* insert an object at the right spot for the keeper */
void obj_to_keeper( OBJ_DATA *obj, CHAR_DATA *ch )
{
    OBJ_DATA *t_obj, *t_obj_next;

    /* see if any duplicates are found */
    for (t_obj = ch->carrying; t_obj != NULL; t_obj = t_obj_next)
    {
  t_obj_next = t_obj->next_content;

  if (obj->pIndexData == t_obj->pIndexData
  &&  !str_cmp(obj->short_descr,t_obj->short_descr))
  {
      /* if this is an unlimited item, destroy the new one */
      if (IS_OBJ_STAT(t_obj,ITEM_INVENTORY))
      {
    extract_obj(obj);
    return;
      }
      obj->cost = t_obj->cost; /* keep it standard */
      break;
  }
    }

    if (t_obj == NULL)
    {
  obj->next_content = ch->carrying;
  ch->carrying = obj;
    }
    else
    {
  obj->next_content = t_obj->next_content;
  t_obj->next_content = obj;
    }

    obj->carried_by      = ch;
    obj->in_room         = NULL;
    obj->in_obj          = NULL;
    ch->carry_number    += get_obj_number( obj );
    ch->carry_weight    += get_obj_weight( obj );
}

/* get an object from a shopkeeper's list */
OBJ_DATA *get_obj_keeper( CHAR_DATA *ch, CHAR_DATA *keeper, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    int number;
    int count;

    number = number_argument( argument, arg );
    count  = 0;
    for ( obj = keeper->carrying; obj != NULL; obj = obj->next_content )
    {
        if (obj->wear_loc == WEAR_NONE
        &&  can_see_obj( keeper, obj )
  &&  can_see_obj(ch,obj)
        &&  is_name( arg, obj->name ) )
        {
            if ( ++count == number )
                return obj;

      /* skip other objects of the same name */
      while (obj->next_content != NULL
      && obj->pIndexData == obj->next_content->pIndexData
      && !str_cmp(obj->short_descr,obj->next_content->short_descr))
    obj = obj->next_content;
        }
    }

    return NULL;
}

int get_cost( CHAR_DATA *keeper, OBJ_DATA *obj, bool fBuy )
{
    SHOP_DATA *pShop;
    int cost;

    if ( obj == NULL || ( pShop = keeper->pIndexData->pShop ) == NULL )
       return 0;

    if ( fBuy )
    {
       cost = obj->cost * pShop->profit_buy  / 100;
    }
    else
    {
       OBJ_DATA *obj2;
       int itype;

       cost = 0;
       for ( itype = 0; itype < MAX_TRADE; itype++ )
       {
          if ( obj->item_type == pShop->buy_type[itype] )
          {
             cost = obj->cost * pShop->profit_sell / 100;
             break;
          }
       }

       if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
       {
          for ( obj2 = keeper->carrying; obj2; obj2 = obj2->next_content )
          {
             if ( obj->pIndexData == obj2->pIndexData
             &&   !str_cmp(obj->short_descr,obj2->short_descr) )
             {
                if (IS_OBJ_STAT(obj2,ITEM_INVENTORY))
                {
                   cost /= 2;
                }
                else
                {
                   cost = cost * 3 / 4;
                }
             }
          }
       }
    }

    if ( (obj->item_type == ITEM_STAFF || obj->item_type == ITEM_WAND))
    {

       if(obj->pIndexData->vnum == 8147 ||
          obj->pIndexData->vnum == 3141 ||
          obj->pIndexData->vnum == 3128
         )
       {
        cost = cost;
        }
        else
       {

                if (obj->value[1] == 0)
                {
                  cost /= 4;
                }
                else
                {
                  cost = cost * obj->value[2] / obj->value[1];
                }
       }
    }

    return cost;
}



void do_buy( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int roll;
    long cost;
    int weight_change;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("You are pretty freaky looking, and scare the shopkeeper.\n\r",ch);
    return;
    }


    if ( argument[0] == '\0' )
    {
  send_to_char( "Buy what?\n\r", ch );
  return;
    }

    if (!IS_NPC(ch) && ch->in_room->clan && ch->clan &&
       IS_SET(ch->pcdata->clan_flags, CLAN_NO_STORE) )
       {
       send_to_char("You have been forbidden to use the clan store.\n\r", ch);
       return;
       }


    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
  char arg[MAX_INPUT_LENGTH];
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *pet;
  ROOM_INDEX_DATA *pRoomIndexNext;
  ROOM_INDEX_DATA *in_room;

  if ( IS_NPC(ch) )
      return;

  argument = one_argument(argument,arg);

  /* hack to make new thalos pets work */
  if (ch->in_room->vnum == 9621)
      pRoomIndexNext = get_room_index(9706);
  else
      pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );
  if ( pRoomIndexNext == NULL )
  {
      bug( "Do_buy: bad pet shop at vnum %d.", ch->in_room->vnum );
      send_to_char( "Sorry, you can't buy that here.\n\r", ch );
      return;
  }

  in_room     = ch->in_room;
  ch->in_room = pRoomIndexNext;
  pet         = get_char_room( ch, arg );
  ch->in_room = in_room;

  if ( pet == NULL || !IS_SET(pet->act, ACT_PET) )
  {
      send_to_char( "Sorry, you can't buy that here.\n\r", ch );
      return;
  }

  if ( ch->pet != NULL )
  {
      send_to_char("You already own a pet.\n\r",ch);
      return;
  }

  cost = 10 * pet->level * pet->level;

  if ( (ch->silver + 100 * ch->gold) < cost )
  {
      sprintf(buf, "You can't afford it, you need %d more silver.\n\r", cost - ch->silver - 100 * ch->gold);
      send_to_char(buf, ch);
      return;
  }

  if ( ch->level < pet->level )
  {
      send_to_char(
    "You're not powerful enough to master this pet.\n\r", ch );
      return;
  }

  /* haggle */
  roll = number_range(30,100);
  if ( (roll - get_curr_stat(ch,STAT_SOC)) < get_skill(ch,gsn_haggle))
  {
      int orig = (int)cost;
      cost -= cost / 2 * roll / 100;
//      sprintf(buf,"You haggle the price down to %d coins.\n\r",cost);
      sprintf(buf,"You haggle the price down to %d coins from %d coins.\n\r",(int)cost, orig);
      send_to_char(buf,ch);
      check_improve(ch,gsn_haggle,TRUE,4);

  }
  if (IS_SET(ch->act,PLR_DWEEB)) cost *= 2;

  deduct_cost(ch,cost);
  pet     = create_mobile( pet->pIndexData );
  SET_BIT(pet->act, ACT_PET);
  SET_BIT(pet->affected_by, AFF_CHARM);
  pet->comm = COMM_NOTELL|COMM_NOSHOUT|COMM_NOCHANNELS;

/* Naming pets causes to many problems
 *
  argument = one_argument( argument, arg );
  if ( arg[0] != '\0' )
  {
      sprintf( buf, "%s %s", pet->name, arg );
      free_string( pet->name );
      pet->name = str_dup( buf );
  }
 */

  sprintf( buf, "%sA neck tag says 'I belong to %s'.\n\r",
      pet->description, ch->name );
  free_string( pet->description );
  pet->description = str_dup( buf );

  char_to_room( pet, ch->in_room );
  add_follower( pet, ch );
  //pet->leader = ch;
  add_to_group(pet, ch);
  ch->pet = pet;
  send_to_char( "Enjoy your pet.\n\r", ch );
  act( "$n bought $N as a pet.", ch, NULL, pet, TO_ROOM ,FALSE);
  return;
    }
    else
    {
  CHAR_DATA *keeper;
  OBJ_DATA *obj,*t_obj;
  char arg[MAX_INPUT_LENGTH];
  int number, count = 1;

  if ( ( keeper = find_keeper( ch ) ) == NULL )
      return;

  number = mult_argument(argument,arg);
  if ( number <= 0 )
  {
     if (!is_number(arg))
     {
     sprintf(buf,"Well, aren't you clever. We fixed this a long time ago.\n\r");
     /* Removed punishing them
     ch->gold = 0;
     ch->silver =0;
     */
     }
     else
     {
     sprintf(buf,"Wrong syntax, please try again or look at 'Help Buy'.\n\r");
     }
     send_to_char(buf,ch);
     return;
  }
  else
  if( number > 100 )
  {
      send_to_char("Rent a U-Haul first, to carry all that.\n\r",ch);
      return;
  }
  obj  = get_obj_keeper( ch,keeper, arg );
  cost = get_cost( keeper, obj, TRUE );
  if (IS_SET(ch->act,PLR_DWEEB)) cost *= 2;
  if ( cost <= 0 || !can_see_obj( ch, obj ) )
  {
      act( "$n tells you 'I don't sell that -- try 'list''.",
    keeper, NULL, ch, TO_VICT ,FALSE);
//      ch->reply = keeper;
      return;
  }

  if (ch->pcdata && ch->pcdata->clan_info && ch->pcdata->clan_info->clan->default_clan == CLAN_OUTCAST)
  {
     act("$n tells you 'I don't do business with Outcasts!'",
        keeper,NULL,ch, TO_VICT ,FALSE);
     return;
  }

  if (!IS_OBJ_STAT(obj,ITEM_INVENTORY))
  {
      for (t_obj = obj->next_content;
         count < number && t_obj != NULL;
         t_obj = t_obj->next_content)
      {
        if (t_obj->pIndexData == obj->pIndexData
        &&  !str_cmp(t_obj->short_descr,obj->short_descr))
        count++;
        else
        break;
      }

      if (count < number)
      {
        sprintf(buf, "$n tells you 'I only have %d in stock.'", count); 
        act(buf,
        keeper,NULL,ch,TO_VICT,FALSE);
//        ch->reply = keeper;
        return;
      }
  }

  if ( (ch->silver + ch->gold * 100) < cost * number )
  {
      if (number > 1)
        sprintf(buf, "$n tells you 'You're short %d silver to buy %d of those.'", cost * number - ch->silver - 100 * ch->gold, number);
      else
        sprintf(buf, "$n tells you 'You can't afford to buy $p, you need %d more silver.'", cost - ch->silver - 100 * ch->gold);
      act(buf,
        keeper, obj, ch, TO_VICT ,FALSE);
//      ch->reply = keeper;
      return;
  }

  if ( obj->level > ch->level + 2 )
  {
    sprintf(buf, "$n tells you 'You need %d more levels to buy $p.'",
      obj->level - 2 - ch->level); 
      act(buf,
    keeper, obj, ch, TO_VICT ,FALSE);
//      ch->reply = keeper;
      return;
  }

  if (ch->carry_number + number * get_obj_number(obj) > can_carry_n(ch))
  {
    if(number == 1 || ch->carry_number * get_obj_number(obj) > can_carry_n(ch))
      sprintf(buf, "You can't carry that many items, even one more is too many.\n\r");
    else
      sprintf(buf, "You can't carry that many items, you can only manage %d of those.\n\r",
      (can_carry_n(ch) - ch->carry_number) / get_obj_number(obj));
    send_to_char(buf, ch );
    return;
  }

  // Reduce by weight of coins they won't be carrying after
  // This ignores haggling so they could end up overweight there, but rarely
  if(ch->silver >= cost)
  {// Only silver changes
    weight_change = cost / 10;
  }
  else
  {
    int gold = ((cost - ch->silver + 99) / 100);
    int silver = cost - 100 * gold;
    weight_change = silver / 10 + gold * 2 / 5; 
  }

  if ( get_carry_weight(ch) + number * get_obj_weight(obj) - weight_change >
    can_carry_w(ch))
  {
    if(number == 1 || get_carry_weight(ch)+ get_obj_weight(obj) > can_carry_w(ch))
      sprintf(buf, "You can't carry that much weight, even one of those is too much.\n\r");
    else
      sprintf(buf, "You can't carry that much weight, you can only manage %d of those.\n\r",
      (can_carry_w(ch) - get_carry_weight(ch)) / get_obj_weight(obj));
    send_to_char( buf, ch );
    return;
  }

  /* haggle */
  roll = number_range(30,100);
  if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT)
  && ((roll - get_curr_stat(ch,STAT_SOC)) < get_skill(ch,gsn_haggle)) )
  {
      int orig = (int)cost;
      cost -= obj->cost / 2 * roll / 100;
//      act("You haggle with $N.",ch,NULL,keeper,TO_CHAR,FALSE);
      sprintf(buf,"You haggle with $N for a discount of %d silver.", orig - (int)cost);
      act(buf,ch,NULL,keeper,TO_CHAR,FALSE);
      check_improve(ch,gsn_haggle,TRUE,4);
  }

  if (number > 1)
  {
      sprintf(buf,"$n buys $p[%d].",number);
      act(buf,ch,obj,NULL,TO_ROOM,FALSE);
      sprintf(buf,"You buy $p[%d] for %d silver.",number,cost * number);
      act(buf,ch,obj,NULL,TO_CHAR,FALSE);
  }
  else
  {
      act( "$n buys $p.", ch, obj, NULL, TO_ROOM ,FALSE);
      sprintf(buf,"You buy $p for %d silver.",cost);
      act( buf, ch, obj, NULL, TO_CHAR ,FALSE);
  }
  deduct_cost(ch,cost * number);
  keeper->gold += cost * number/100;
  keeper->silver += cost * number - (cost * number/100) * 100;

  for (count = 0; count < number; count++)
  {
      if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
        t_obj = create_object( obj->pIndexData, obj->level, FALSE );
      else
      {
    t_obj = obj;
    obj = obj->next_content;
        obj_from_char( t_obj );
      }

      if (t_obj->timer > 0 && !IS_OBJ_STAT(t_obj,ITEM_HAD_TIMER))
        t_obj->timer = 0;
      REMOVE_BIT(t_obj->extra_flags,ITEM_HAD_TIMER);
      obj_to_char( t_obj, ch );
      if (cost < t_obj->cost)
        t_obj->cost = cost;
  }
    }
}


void do_list( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[6*MAX_STRING_LENGTH];

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("You are pretty scary looking, the shopkeeper is scared.\n\r",ch);
    return;
    }


    if (!IS_NPC(ch) && ch->in_room->clan && ch->clan &&
       IS_SET(ch->pcdata->clan_flags, CLAN_NO_STORE) )
       {
       send_to_char("You have been forbidden to use the clan store.\n\r", ch);
       return;
       }

    if ( IS_SET(ch->in_room->room_flags, ROOM_PET_SHOP) )
    {
  ROOM_INDEX_DATA *pRoomIndexNext;
  CHAR_DATA *pet;
  bool found;

        /* hack to make new thalos pets work */
        if (ch->in_room->vnum == 9621)
            pRoomIndexNext = get_room_index(9706);
        else
            pRoomIndexNext = get_room_index( ch->in_room->vnum + 1 );

  if ( pRoomIndexNext == NULL )
  {
      bug( "Do_list: bad pet shop at vnum %d.", ch->in_room->vnum );
      send_to_char( "You can't do that here.\n\r", ch );
      return;
  }

  found = FALSE;
  buf2[0] = '\0';
  for ( pet = pRoomIndexNext->people; pet; pet = pet->next_in_room )
  {
      if ( IS_SET(pet->act, ACT_PET) )
      {
    if ( !found )
    {
        found = TRUE;
        send_to_char( "Pets for sale:\n\r", ch );
    }
    sprintf( buf, "[%2d] %8d - %s\n\r",
        pet->level,
        10 * pet->level * pet->level,
        pet->short_descr );
    strcat( buf2, buf );
      }
  }
    page_to_char( buf2, ch);
  if ( !found )
      send_to_char( "Sorry, we're out of pets right now.\n\r", ch );
  return;
    }
    else
    {
  CHAR_DATA *keeper;
  OBJ_DATA *obj;
  int cost,count;
  bool found;
  char arg[MAX_INPUT_LENGTH];

  if ( ( keeper = find_keeper( ch ) ) == NULL )
      return;
        one_argument(argument,arg);

  found = FALSE;
  buf2[0] = '\0';
  for ( obj = keeper->carrying; obj; obj = obj->next_content )
  {
      if ( obj->wear_loc == WEAR_NONE
      &&   can_see_obj( ch, obj )
      &&   ( cost = get_cost( keeper, obj, TRUE ) ) > 0
      &&   ( arg[0] == '\0'
         ||  is_name(arg,obj->name) ))
      {
    if ( !found )
    {
        found = TRUE;
        send_to_char( "[Lv Price Qty] Item\n\r", ch );
    }

    if (IS_OBJ_STAT(obj,ITEM_INVENTORY))
        sprintf(buf,"[%2d %5d -- ] %s\n\r",
      obj->level,cost,obj->short_descr);
    else
    {
        count = 1;

        while (obj->next_content != NULL
        && obj->pIndexData == obj->next_content->pIndexData
        && !str_cmp(obj->short_descr,
              obj->next_content->short_descr))
        {
      obj = obj->next_content;
      count++;
        }
        sprintf(buf,"[%2d %5d %2d ] %s\n\r",
      obj->level,cost,count,obj->short_descr);
    }
     strcat(buf2, buf);
      }
  }
     page_to_char(buf2, ch);

  if ( !found )
      send_to_char( "You can't buy anything here.\n\r", ch );
  return;
    }
}



void do_sell( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int roll;
    long cost;

    one_argument( argument, arg );

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }



    if ( arg[0] == '\0' )
    {
  send_to_char( "Sell what?\n\r", ch );
  return;
    }

    if (!IS_NPC(ch) && ch->in_room->clan && ch->clan &&
       IS_SET(ch->pcdata->clan_flags, CLAN_NO_STORE) )
       {
       send_to_char("You have been forbidden to use the clan store.\n\r", ch);
       return;
       }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
  return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
  act( "$n tells you 'You don't have that item'.",
      keeper, NULL, ch, TO_VICT ,FALSE);
//  ch->reply = keeper;
  return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
  send_to_char( "You can't let go of it.\n\r", ch );
  return;
    }

    if (!can_see_obj(keeper,obj))
    {
  act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT,FALSE);
  return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
  act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT ,FALSE);
  return;
    }
    /* Modify sell value based on damaged status - enhanced items sell for more */
    if(obj->damaged)
    {
        if(obj->damaged < 0)
                cost = cost * 11 / 10;// 10% bonus for perfect condition
        else
        {/* Ding them for the repair cost as if it were fully broken (Whether it is or isn't) */
                cost -= obj->level * obj->level * 3;
                if(cost <= 0)
                {
                        act("$p is too damaged for $n to purchase from you.", keeper, obj, ch, TO_VICT ,FALSE);
                        return;
                }
        }
    }

  if (IS_SET(ch->act,PLR_DWEEB)) cost /= 2;
    if ( cost > (keeper-> silver + 100 * keeper->gold) )
    {
  act("$n tells you 'I'm afraid I don't have enough wealth to buy $p.",
      keeper,obj,ch,TO_VICT,FALSE);
  return;
    }

    act( "$n sells $p.", ch, obj, NULL, TO_ROOM ,FALSE);
    /* connive */
    roll = number_percent();
    if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT) && roll < get_skill(ch,gsn_connive))
    {
        int orig = (int)cost;
        //send_to_char("You are sooooooo smooth.\n\r",ch);
        cost += obj->cost / 4 * roll / 100;
        cost = UMIN(cost,95 * get_cost(keeper,obj,TRUE) / 100);
        cost = UMIN(cost,(keeper->silver + 100 * keeper->gold));
        sprintf(buf,"You are sooooooo smooth that $N is giving you %d more silver.", (int)cost - orig);
        act(buf,ch,NULL,keeper,TO_CHAR,FALSE);
        check_improve(ch,gsn_connive,TRUE,4);
    }
    roll = number_range(30,100);
        /* haggle*/
    if (!IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT) && (roll - get_curr_stat(ch,STAT_SOC) < get_skill(ch,gsn_haggle)) )
    {
        int orig = (int)cost;
//        send_to_char("You haggle with the shopkeeper.\n\r",ch);
        cost += obj->cost / 2 * roll / 100;
        cost = UMIN(cost,95 * get_cost(keeper,obj,TRUE) / 100);
  cost = UMIN(cost,(keeper->silver + 100 * keeper->gold));
        sprintf(buf,"You haggle with $N for an increase of %d silver.", (int)cost - orig);
        act(buf,ch,NULL,keeper,TO_CHAR,FALSE);
        check_improve(ch,gsn_haggle,TRUE,4);
    }
    sprintf( buf, "You sell $p for %d silver and %d gold piece%s.",
  cost - (cost/100) * 100, cost/100, cost == 1 ? "" : "s" );
    act( buf, ch, obj, NULL, TO_CHAR ,FALSE);
    ch->gold     += cost/100;
    ch->silver   += cost - (cost/100) * 100;
    deduct_cost(keeper,cost);
    if ( keeper->gold < 0 )
  keeper->gold = 0;
    if ( keeper->silver< 0)
  keeper->silver = 0;

    if ( obj->item_type == ITEM_TRASH || IS_OBJ_STAT(obj,ITEM_SELL_EXTRACT))
    {
  extract_obj( obj );
    }
    else
    {
  obj_from_char( obj );
  if (obj->timer)
      SET_BIT(obj->extra_flags,ITEM_HAD_TIMER);
  else
      /* No timers on clan shopkeeper special MOBs */
      if(!keeper->in_room->clan || keeper->in_room->clan != ch->clan)
        obj->timer = number_range(50,100);
  obj_to_keeper( obj, keeper );
    }

    return;
}



void do_value( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *keeper;
    OBJ_DATA *obj;
    int cost;

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
    send_to_char("Not while in wraithform.\n\r",ch);
    return;
    }


    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
  send_to_char( "Value what?\n\r", ch );
  return;
    }

    if (!IS_NPC(ch) && ch->in_room->clan && ch->clan &&
       IS_SET(ch->pcdata->clan_flags, CLAN_NO_STORE) )
       {
       send_to_char("You have been forbidden to use the clan store.\n\r", ch);
       return;
       }

    if ( ( keeper = find_keeper( ch ) ) == NULL )
  return;

    if ( ( obj = get_obj_carry( ch, arg ) ) == NULL )
    {
  act( "$n tells you 'You don't have that item'.",
      keeper, NULL, ch, TO_VICT ,FALSE);
//  ch->reply = keeper;
  return;
    }

    if (!can_see_obj(keeper,obj))
    {
      act("$n doesn't see what you are offering.",keeper,NULL,ch,TO_VICT,FALSE);
      return;
    }

    if ( !can_drop_obj( ch, obj ) )
    {
  send_to_char( "You can't let go of it.\n\r", ch );
  return;
    }

    if ( ( cost = get_cost( keeper, obj, FALSE ) ) <= 0 )
    {
  act( "$n looks uninterested in $p.", keeper, obj, ch, TO_VICT ,FALSE);
  return;
    }

    if(obj->damaged)
    {
        if(obj->damaged < 0)
                cost = cost * 11 / 10;// 10% bonus for perfect condition
        else
        {/* Ding them for the repair cost as if it were fully broken (Whether it is or isn't) */
                cost -= obj->level * obj->level * 3;
                if(cost <= 0)
                {
                        act("$p is too damaged for $n to purchase from you.", keeper, obj, ch, TO_VICT ,FALSE);
                        return;
                }
        }
    }

    sprintf( buf,
  "$n tells you 'I'll give you %d silver and %d gold coins for $p'.",
  cost - (cost/100) * 100, cost/100 );
    act( buf, keeper, obj, ch, TO_VICT ,FALSE);
//    ch->reply = keeper;

    return;
}


void do_assemble ( CHAR_DATA *ch, char *argument)
{
        OBJ_DATA *piece;
        OBJ_DATA *whole;
        OBJ_DATA *obj;
        int count = 0, check_count = 0;
        int i;

        if ( argument[0] == '\0')
        {
        send_to_char("Assemble what?\n\r", ch);
        return;
        }

        if ( ( piece = get_obj_carry(ch, argument) )== NULL )
        {
        send_to_char("You're not carrying that.\n\r", ch);
        return;
        }

        if ( piece->item_type != ITEM_PART )
        {
        act( "$p is not part of a greater item.",ch, piece, NULL, TO_CHAR, FALSE);
        return;
        }

        for ( i = 0; i <= 4; i++)
        {
          if (piece->value[i] != 0) count++;
        }

        // now, take one away from count, since we don't need the vnum of the whole item
        if (count != 0) count -= 1;

        // if count = 0 at this point then the item is a part, with no whole, so lets return
        if ( count == 0) return;

        // now start checking to see if they have the items necessary
        for ( i = 1; i <= count; i++)
                for ( obj = ch->carrying; obj != NULL; obj = obj->next_content)
                        if ( obj->pIndexData->vnum == piece->value[i] && obj->item_type == ITEM_PART )
                                check_count++;

        //now we know if they have all the items if the count and check_count match
        if ( count == check_count )
        {
        whole = create_object(get_obj_index(piece->value[0]), 0, FALSE);
        send_to_char("A blinding light engulfs you!\n\r", ch);
        act ("You have created $p!",ch, whole, NULL, TO_CHAR, FALSE);
        act ("$N creates $p in a blaze of magical light!", ch, whole, NULL, TO_ROOM, FALSE);
        obj_to_char( whole, ch);

        // now remove all the parts
        for ( i = 1; i <= count; i++)
                for ( obj = ch->carrying; obj != NULL; obj = obj->next_content)
                        if ( obj->pIndexData->vnum == piece->value[i] )
                                {
                                obj_from_char(obj);
                                extract_obj(obj);
                                }
        }
        else
        {
        send_to_char ("You don't have all the necessary pieces.\n\r", ch);
        return;
        }
        return;
}

void do_get_voucher ( CHAR_DATA *ch, CHAR_DATA *argument )
{
  OBJ_DATA *obj;
  OBJ_DATA *obj_next;
  int brick_count=0;
  int taken_bricks=0;

  if ( !IS_IMMORTAL(ch) )
  {
    return;
  }
  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
    obj_next = obj->next_content;

    if ( obj->pIndexData->vnum == OBJ_VNUM_PLAT_BRICK )
    {
      brick_count++;
    }
  }
  if ( ( ch->pcdata->rank < 4 && ch->clan == nonclan_lookup("zealot") ) && !IS_IMMORTAL(ch) )
  {
    if ( brick_count >=5 )
    {
      send_to_char("The Almighty bestows a vial of his Holy Oil upon you.\n\r", ch );
      act( "$n is surrouned by a beautiful {Wwhite light.", ch, NULL, NULL, TO_ROOM ,FALSE);
      for ( obj = ch->carrying; obj != NULL; obj = obj_next )
      {
        obj_next = obj->next_content;

        if ( obj->pIndexData->vnum == OBJ_VNUM_PLAT_BRICK && taken_bricks < 5)
        {
          extract_obj(obj);
          taken_bricks++;
        }
      }
      obj_to_char(create_object(get_obj_index(OBJ_VNUM_HOLY_OIL),0,FALSE),ch);
    }
    else
    {
      send_to_char("You do not have enough beautiful platinum bricks.  You must have a sacrifice of at least five. \n\r", ch );
    }
  }

  return;
}

void do_forge ( CHAR_DATA *ch, char *argument )
{
        OBJ_DATA *forge, *obj, *complete, *t_obj;
        int count = 0;
        int  i, j;
        bool  match = FALSE;
        int part_list[MAX_IN_RECIPE];
        int chance;
        RECIPE_DATA *recipe = NULL;

        if ( argument[0] == '\0' )
        {
        send_to_char("Attempt to use what to craft a greater item?\n\r", ch);
        return;
        }

        if ( (forge = get_obj_here(ch, argument)) == NULL)
        {
        send_to_char("You don't see that here.\n\r", ch);
        return;
        }

        if ( forge->item_type != ITEM_FORGE )
        {
        send_to_char("That item is not capable of transmuting objects.\n\r", ch);
        return;
        }

        /* first thing we need to do is loop through all the possible recipes associated with
           this forge.  For each recipe, we need to check what items are required for
           this recipe.  If we don't have all the items, check the next recipe for this forge.
           if we do have them all, remove them from the game, and replace the contents with
           the completed item for this recipe */

        for ( i = 0 ; i <= 4; i++ )
        {
                count = 0;

                if ( forge->value[i] <= 0 )
                        continue;

                if ((recipe = get_recipe_data(forge->value[i])) == NULL)
                {
                act("$p appears to be broken.", ch, forge, NULL, TO_CHAR, FALSE);
                return;
                }

                for ( j = 0 ; j <= MAX_IN_RECIPE ; j++)
                {
                  if (recipe->vnum_parts[j] != 0)
                  {
                        part_list[j] = recipe->vnum_parts[j];
                        count++;
                  }
                  else
                        break;
                }
                for (obj = forge->contains ; obj != NULL ; obj = obj->next_content )
                   for ( j = 0 ; j < count ; j++)
                   {
                        if (obj->pIndexData->vnum == part_list[j] )
                        {
                                 part_list[j] = 0;
                                break; /* Added by G to fix bug */
                        }
                   }
                for ( j = 0 ; j < count; j++ )
                {
                if ( part_list[j] != 0 )
                        {
                        match = FALSE;
                        break;
                        }
                else
                        match = TRUE;
                }
                if (match) break;
        }

        /* At this point, we've gone through all possible recipes for this forge.  If match
           is true, we have all the items we need, so lets extract them from the forge.  If
           match is false, tell the player, and return */

        if (!match)
        {
        send_to_char("The forge does not have all the required ingredients.\n\r", ch);
        return;
        }
        else
        {
            bool bFail = FALSE;

           /* Check chance with skill */
           if ( recipe->skill_sn > 0 )
            {
                chance = get_skill(ch, recipe->skill_sn ) - recipe->difficulty;
                if ( number_percent() > chance )
                {
                    check_improve(ch,recipe->skill_sn,FALSE,15); /* failed */
                    bFail = TRUE;
                }
                else
                        check_improve(ch,recipe->skill_sn,TRUE,5);
            }

           for(obj = forge->contains ; obj != NULL ; obj = t_obj )
           {
                t_obj = obj->next_content;
                obj_from_obj(obj);
                extract_obj(obj);
           }

        if ( bFail )
        {
                send_to_char("You failed to create anything.\n\r",ch);
                return;
        }
        complete = ( create_object(get_obj_index(recipe->vnum_complete), 0, FALSE));
        if ( complete == NULL )
        {
            bug("ERROR in forge, completion object doesn't exit!  Vnum %d",recipe->vnum_complete);
            send_to_char("There is a game error with this recipe.\n\r",ch);
            return;
        }

        obj_to_obj(complete, forge);
        act("$p glows with a magical light!", ch, forge, NULL, TO_ROOM, FALSE);
        act("Your forging is successful.  $p glows with magical light!", ch, forge, NULL, TO_CHAR, FALSE);
        }
        return;
}

void do_ritual( CHAR_DATA *ch, char *argument )
{
    int chance;
    OBJ_DATA *scroll;
    int sector;

    if ( IS_NPC(ch) )
        return;

    if ( ch->pcdata->old_class != class_lookup("cleric") && ch->pcdata->old_class != class_lookup("elementalist"))
    {
        send_to_char("You are not devoted enough.  Only oldclass clerics may Ritual.\n\r",ch);
        return;
    }

    sector = ch->in_room->sector_type;

    switch( sector )
    {
    case SECT_ALTAR_BASIC:
    case SECT_ALTAR_BLESSED:
    case SECT_ALTAR_ANNOINTED:
    case SECT_ALTAR_HOLY_GROUND: break;
    default:
        send_to_char("You can only perform the Ritual at an Altar.\n\r",ch);
        return;
    }

    if ( ( scroll = get_eq_char(ch,WEAR_HOLD) ) == NULL ||
         scroll->item_type != ITEM_SPELL_PAGE )
    {
        send_to_char("You must be holding a spell page.\n\r",ch);
        return;
    }

    if ( scroll->value[3] < 1 )
    {
        send_to_char("This scroll has been worn out beyond readability.\n\r",ch);
        return;
    }

        /* 0 : scribe level, 1 : Diff, 2: slot, 3: uses */
    if ( ch->level < scroll->value[0] )
    {
        send_to_char("You aren't ready yet.\n\r",ch);
        return;
    }

    if ( skill_table[scroll->value[2]].skill_level[ch->class] > 51 )
    {
        send_to_char("Your class cannot learn this spell.\n\r",ch);
        return;
    }

    if ( ch->pcdata->learned[scroll->value[2]] >= 1 )
    {
        send_to_char("You already know that spell!\n\r",ch);
        return;
    }

    /* DOn't have spell, can learn spell, have scroll, is right level, is in altar room */
    chance = ch->level;
    chance += (get_curr_stat(ch,STAT_INT)+get_curr_stat(ch,STAT_WIS))*2;
    chance -= scroll->value[1];
    switch ( sector )
   {
        case SECT_ALTAR_BASIC: chance += 10; break;
        case SECT_ALTAR_BLESSED: chance += 15; break;
        case SECT_ALTAR_ANNOINTED: chance += 30; break;
        case SECT_ALTAR_HOLY_GROUND: chance += 60; break;
   }
   chance = URANGE(5,chance,95);

   if ( number_percent( ) < chance )
   {
        ch->pcdata->learned[scroll->value[2]] = 1;
        scroll->value[3]--;
        scroll->cost = 0;
        act("You have learned a new spell from $p!",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$n has learned a new spell from $p!",ch,scroll,NULL,TO_ROOM,FALSE);
        gain_exp(ch, scroll->value[0] * 100 );
   }
   else
   {
        scroll->value[3]--;
        scroll->cost = 0;
        act("You attempt to learn from $p, but fail.",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$n attempts to learn from $p, but fails.",ch,scroll,NULL,TO_ROOM,FALSE);
    }

    if ( scroll->value[3] < 1 )
    {
        act("$p wavers and then crumbles to ashes.",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$p wavers and then crumbles to ashes.",ch,scroll,NULL,TO_ROOM,FALSE);
        extract_obj(scroll);
    }
}

void do_copyspell( CHAR_DATA *ch, char *argument )
{
    int chance;
    OBJ_DATA *scroll;
    int sector;

    if ( IS_NPC(ch) )
        return;

    if ( ch->pcdata->old_class != class_lookup("mage") )
    {
        send_to_char("You are not learned enough.  Only oldclass mages may copy scrolls.\n\r",ch);
        return;
    }

    sector = ch->in_room->sector_type;

    switch( sector )
    {
    case SECT_MAGELAB_SIMPLE:
    case SECT_MAGELAB_INTERMEDIATE:
    case SECT_MAGELAB_ADVANCED:
    case SECT_MAGELAB_SUPERIOR: break;
    default:
        send_to_char("You must be in a Magelab to study spell pages.\n\r",ch);
        return;
    }

    if ( ( scroll = get_eq_char(ch,WEAR_HOLD) ) == NULL ||
         scroll->item_type != ITEM_SPELL_PAGE )
    {
        send_to_char("You must be holding a spell page.\n\r",ch);
        return;
    }

    if ( scroll->value[3] < 1 )
    {
        send_to_char("This spell page has been worn out beyond readability.\n\r",ch);
        return;
    }

        /* 0 : scribe level, 1 : Diff, 2: slot, 3: uses */
    if ( ch->level < scroll->value[0] )
    {
        send_to_char("You aren't ready yet.\n\r",ch);
        return;
    }

    if ( skill_table[scroll->value[2]].skill_level[ch->class] > 51 )
    {
        send_to_char("Your class cannot learn this spell.\n\r",ch);
        return;
    }

    if ( ch->pcdata->learned[scroll->value[2]] >= 1 )
    {
        send_to_char("You already know that spell!\n\r",ch);
        return;
    }

    /* DOn't have spell, can learn spell, have scroll, is right level, is in altar room */
    chance = ch->level;
    chance += (get_curr_stat(ch,STAT_INT)+get_curr_stat(ch,STAT_WIS))*2;
    chance -= scroll->value[1];
    switch ( sector )
   {
        case SECT_MAGELAB_SIMPLE: chance += 10; break;
        case SECT_MAGELAB_INTERMEDIATE: chance += 15; break;
        case SECT_MAGELAB_ADVANCED: chance += 30; break;
        case SECT_MAGELAB_SUPERIOR: chance += 60; break;
   }
   /* It's harder to read a damaged scroll - shouldn't be 100, that is destroyed - paranoid check */
   if(scroll->damaged > 0 && scroll->damaged <= 100)
           chance -= scroll->damaged / 4;// Don't make it too much harder

   chance = URANGE(5,chance,95);

   if ( number_percent( ) < chance )
   {
        ch->pcdata->learned[scroll->value[2]] = 1;
        scroll->value[3]--;
        scroll->cost = 0;
        act("You have learned a new spell from $p!",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$n has learned a new spell from $p!",ch,scroll,NULL,TO_ROOM,FALSE);
        gain_exp(ch,scroll->value[0] * 100 );
   }
   else
   {
        scroll->value[3]--;
        scroll->cost = 0;
        act("You attempt to learn from $p, but fail.",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$n attempts to learn from $p, but fails.",ch,scroll,NULL,TO_ROOM,FALSE);
    }

    if ( scroll->value[3] < 1 )
    {
        act("$p wavers and then crumbles to ashes.",ch,scroll,NULL,TO_CHAR,FALSE);
        act("$p wavers and then crumbles to ashes.",ch,scroll,NULL,TO_ROOM,FALSE);
        extract_obj(scroll);
    }
}

void do_newloot( CHAR_DATA *ch, char *argument )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    OBJ_DATA *container;
    bool found;

    argument = one_argument( argument, arg1 );
    argument = one_argument( argument, arg2 );

    if (!str_cmp(arg2,"from"))
       argument = one_argument(argument,arg2);

    /* no loot 1. corpse */
    number_argument(arg1,arg1);

    if( is_affected(ch,skill_lookup("wraithform")) )
    {
        send_to_char( "You cannot loot while in wraithform.\n\r", ch);
        return;
    }

    /* Get type. */
    if ( arg1[0] == '\0' || arg2[0] == '\0' )
    {
        send_to_char( "Loot what?\n\r", ch );
        return;
    }

    if ( ( container = get_obj_here( ch, arg2 ) ) == NULL )
    {
      act( "I see no $T here.", ch, NULL, arg2, TO_CHAR ,FALSE);
      return;
    }

    switch ( container->item_type )
    {
       default:
          send_to_char( "That's not a container.\n\r", ch );
          return;

       case ITEM_CONTAINER:
       case ITEM_CORPSE_NPC:
       {
          send_to_char( "Use 'get'.\n\r", ch );
          return;
       }

       case ITEM_CORPSE_PC:
       {
          if (!can_loot(ch,container,TRUE))
          {
             send_to_char( "You can't do that.\n\r", ch );
             return;
          }
       }

       if ( str_cmp( arg1, "all" ) && str_prefix( "all.", arg1 ) )
       {
          /* 'get obj container' */
          obj = get_obj_list( ch, arg1, container->contains );
          if ( obj == NULL )
          {
             act( "I see nothing like that in the $T.",
                 ch, NULL, arg2, TO_CHAR ,FALSE);
             return;
          }
/*          if (  obj->item_type != ITEM_GEM
             && obj->item_type != ITEM_JEWELRY
             && obj->item_type != ITEM_PILL
             && obj->item_type != ITEM_COMPONENT
             && obj->item_type != ITEM_KEY
             && obj->item_type != ITEM_TREASURE
             && obj->item_type != ITEM_SCROLL
             && obj->item_type != ITEM_WAND
             && obj->item_type != ITEM_STAFF
             && obj->item_type != ITEM_POTION
             && obj->item_type != ITEM_TRAP
             && obj->item_type != ITEM_GRENADE
             && obj->item_type != ITEM_MONEY
             && obj->item_type != ITEM_TRASH
             && obj->item_type != ITEM_CONTAINER
             && !can_loot(ch,container,FALSE) )
          {
             if(container->value[4] > 0)
             {
                container->value[4]--;
             }
             else
             {
                send_to_char("This one has been picked clean already.\n\r",ch);
                return;
             }
          }*/

          /* found something to grab, lag'em a bit */
          WAIT_STATE( ch, 12);
          get_obj( ch, obj, container );
       }
       else
       {
          /* 'get all container' or 'get all.obj container' */
          found = FALSE;
          for ( obj = container->contains; obj != NULL; obj = obj_next )
          {
             obj_next = obj->next_content;
             if ( ( arg1[3] == '\0' || is_name( &arg1[4], obj->name ) )
                   &&   can_see_obj( ch, obj ) )
             {
                found = TRUE;
                if( (container->pIndexData->vnum == OBJ_VNUM_PIT
                        &&  !IS_IMMORTAL(ch))
                     || (!can_loot(ch,container,FALSE)
                        && !IS_IMMORTAL(ch)) )
                {
                   send_to_char("Don't be so greedy!\n\r",ch);
                   return;
                }
                get_obj( ch, obj, container );
             }
          }
          if ( !found )
          {
             if ( arg1[3] == '\0' )
                act( "I see nothing in the $T.",
                   ch, NULL, arg2, TO_CHAR ,FALSE);
             else
                act( "I see nothing like that in the $T.",
                   ch, NULL, arg2, TO_CHAR ,FALSE);
          }
       }
    }

    return;
}


//#ifdef COREYCODE
void do_sharpen( CHAR_DATA *ch, char *argument )
{ //start of do sharpen
    AFFECT_DATA af;
    int skill,save_it;
    //OBJ_DATA obj;
    OBJ_DATA *obj,*stone;


    char arg1[MAX_INPUT_LENGTH];


    argument = one_argument( argument, arg1 );

    if ((skill = get_skill(ch,gsn_sharpen)) < 1)
    {
        send_to_char("No, this skill will not let you sharpen your wits, much less your weapon.\n\r",ch);
        return;
    }
    if ( ( arg1[0] == '\0' ) )
    {
  send_to_char( "Syntax for sharpen is: sharpen <weapon> \n\r", ch );
  return;
    }

if ( ( obj = get_obj_carry( ch, arg1 ) ) == NULL )
{
  send_to_char("You do not have that item.\n\r",ch);
  return;
}

    if ( obj->item_type != ITEM_WEAPON  )
    {
        send_to_char("That is not a weapon.\n\r",ch);
        return;
    }

    if(obj->link_name && ch->pcdata && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    {
      send_to_char("Turn off safelink if you want to risk destroying your linked weapon.\n\r", ch);
      return;
    }

    stone = get_eq_char(ch,WEAR_HOLD);
    if (stone == NULL || stone->item_type != ITEM_WARP_STONE)
    {
  send_to_char("You lack the proper type of stone to sharpen this weapon.\n\r",ch);
  return;
    }

  switch (obj->value[0])
  {
      case(WEAPON_EXOTIC) : send_to_char("You may not sharpen that.\n\r",ch); return;break;
      case(WEAPON_MACE)   : send_to_char("You may not sharpen that mace/club.\n\r",ch);return;  break;
      case(WEAPON_WHIP) : send_to_char("You may not sharpen that whip.\n\r",ch);return;   break;
      case(WEAPON_GAROTTE): send_to_char("You may not sharpen that garotte.\n\r",ch);return;  break;
      default: break;
  }


    if (IS_WEAPON_STAT(obj,WEAPON_SHARP))
    {
       send_to_char("This weapon can not be made any sharper..\n\r",ch);
       return;
    }

    if (stone != NULL && stone->item_type == ITEM_WARP_STONE)
    {
      act("You draw upon the power of $p.",ch,stone,NULL,TO_CHAR,FALSE);
      act("It flares brightly and vanishes!",ch,stone,NULL,TO_CHAR,FALSE);
      extract_obj(stone);
    }
    if ( get_skill(ch,gsn_sharpen)*9/10 > number_percent() )
    {
      send_to_char("You apply a fine edge to the weapon.\n\r",ch);
      check_improve(ch,gsn_sharpen,TRUE,2);

            af.where     = TO_WEAPON;
            af.type      = gsn_sharpen;
            af.level     = ch->level;
            if ( (number_percent() < get_skill(ch,gsn_sharpen)/10 + ch->level/10)
            && ch->level >= 40 )
            {
                send_to_char("{YWHOA!{x Now {RTHAT'S{x a fine edge!!!\n\r",ch);
                af.duration = -1;
            }
            else
            {
                af.duration  = 30 + ch->level/3;
            }
            af.location  = 0;
            af.modifier  = 0;
            af.bitvector = WEAPON_SHARP;
            affect_to_obj(obj,&af);

        save_it = ch->level;
        save_it = URANGE(5,save_it, 100);

    }
    else
    {
      send_to_char("You fail to sharpen the weapon.\n\r",ch);
    check_improve(ch,gsn_sharpen,FALSE,2);
      if ( number_percent() > ch->level + (get_skill(ch,gsn_sharpen) / 3) )
      {
        if ( number_percent() < number_range(1,ch->level/5) )
        {
          SET_BIT( obj->value[4], WEAPON_VORPAL );
          act("ACK!  What's happened to $p ??? ",ch,obj,NULL,TO_ROOM,FALSE);
          act("ACK!  What's happened to $p ??? ",ch,obj,NULL,TO_CHAR,FALSE);
        }
        else
        {
          act("Whoops! There's a horrible sound as $p shatters into many many pieces!",ch,obj,NULL,TO_ROOM,FALSE);
          act("Whoops! There's a horrible sound as $p shatters into many many pieces!",ch,obj,NULL,TO_CHAR,FALSE);
          extract_obj(obj);
        }
      }
    }
return;

}//end of do sharpen
//#endif


