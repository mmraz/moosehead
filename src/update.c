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

static char rcsid[] = "$Id: update.c,v 1.313 2004/09/28 01:23:39 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "lookup.h"
#include "music.h"
#include "tables.h"
#include "gladiator.h"
#include "db.h"

/* command procedures needed */
DECLARE_DO_FUN(do_quit    );
DECLARE_DO_FUN( do_help	);
DECLARE_DO_FUN( do_stand);
DECLARE_DO_FUN( do_look);

/*
 * Local functions.
 */
int hit_gain  args( ( CHAR_DATA *ch ) );
int mana_gain args( ( CHAR_DATA *ch ) );
int move_gain args( ( CHAR_DATA *ch ) );
void  dot_update	args( ( void ) );
void  mobile_update args( ( void ) );
void  weather_update  args( ( void ) );
void  char_update args( ( void ) );
void  obj_update  args( ( void ) );
void  room_update args( ( void ) );
void  aggr_update args( ( void ) );
void  shapeshift_remove args ((CHAR_DATA *ch));   
bool  ch_in_wraithform = FALSE;
void sector_damage args ((CHAR_DATA *ch));
/* Externals */
void remove_highlander args((CHAR_DATA *ch, CHAR_DATA *victim));

/* used for saving */

int save_number = 0;

int rainbow = 0;
time_t rainbow_found = 0;
AREA_DATA *rainbow_area = NULL;

extern int bounty_timer;
extern int bounty_complete;
extern int bounty_vnum;
extern int bounty_item;
extern int bounty_room;
extern int bounty_type;
extern bool bounty_downgrade;
void select_bounty(int qualifier);

void check_shapeshifted( CHAR_DATA *ch )
{
   if(IS_SET(ch->mhs,MHS_SHAPESHIFTED) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
   {
      if ((ch->race != ch->save_race) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
      {
         if ( number_percent() <= 20 )
	 {
	    ch->mod_stat[STAT_CON] -= 1;
	    ch->save_con_mod -= 1;
	 }
      }
      else
      {
	 if ( number_percent() <= 40)
	 {
	    ch->mod_stat[STAT_CON] += 1;
	    ch->save_con_mod += 1;
	 }

	 if (ch->save_con_mod == 0)
	 {
	    REMOVE_BIT(ch->mhs,MHS_SHAPESHIFTED);
	    REMOVE_BIT(ch->mhs,MHS_SHAPEMORPHED);
	 }
      }

      /* If a Shapeshifter hits 3 Con Kill them */
      if (get_curr_stat(ch,STAT_CON) <= 3)
      {
	 ch->save_con_mod = 0;
	 if (IS_SET(ch->mhs,MHS_SHAPESHIFTED))
	    shapeshift_remove(ch);
	 REMOVE_BIT(ch->mhs,MHS_SHAPESHIFTED);
	 REMOVE_BIT(ch->mhs,MHS_SHAPEMORPHED);
	 if (!IS_IMMORTAL(ch))
   	    raw_kill(ch,ch);
      }
   }

   return;
}

void check_savant( CHAR_DATA *ch )
{
   if ( IS_NPC(ch) )
	return;

   if ( IS_SET(ch->mhs,MHS_SAVANT) )
   {
	int dam;
       switch( number_percent() * number_percent() )
       {
       case 1: 
	   act("The wind picks up and whips your cloak about violently.",
		ch,NULL,NULL,TO_CHAR,FALSE);
	   ch->pcdata->savant += 10;
	   break;
       case 2:
	    act("Your arms feel cold and stiff suddenly.",
		ch,NULL,NULL,TO_CHAR,FALSE);
	    ch->pcdata->savant += 3;
	    break;
       case 3:
	    act("You double over in pain as a high scream pierces your head.",
		ch,NULL,NULL,TO_CHAR,FALSE);
		ch->pcdata->savant += 2;
	    break;
       case 4:
	    act("You hear voices babbling on the edges of the wind.",
		ch,NULL,NULL,TO_CHAR,FALSE);
		ch->pcdata->savant += 4;
	    break;
       case 5:
	    act("Your vision blurs momentarily.",
		ch,NULL,NULL,TO_CHAR,FALSE);
		ch->pcdata->savant += 9;
	    break;
       case 6:
	    act("Your soul cries out in agony.",
		ch,NULL,NULL,TO_CHAR,FALSE);
            dam = number_range(ch->level,ch->level*2);
	    ch->hit = UMAX(1, ch->hit - dam );
	    ch->pcdata->savant += ( dam / 5 );
	    break;
       case 7:
       case 8:
       case 9:
       case 10:
	    ch->pcdata->perm_hit++;
	    ch->max_hit++;
	    send_to_char("You have been blessed.\n\r", ch);
	    break;
       case 11:
       case 12:
       case 13:
       case 14:
       case 15:
       case 16:
       case 17:
       case 18:
       case 19:
       case 20:
       case 21:
       case 22:
       case 23:
       case 24:
       case 25:
	    if ( number_percent() * number_percent() < ch->pcdata->savant ) 
	    {
	    REMOVE_BIT(ch->mhs,MHS_SAVANT);
	    ch->pcdata->savant = 0;
	    send_to_char("Walk no more with the storm.\n\r",ch);
	    break;
	    }
       default:
	    break;
       };
   }
   
   if ( !IS_IMMORTAL(ch) && 
	number_percent() * number_percent() < 10 &&
	number_percent() * number_percent() < 10 &&
	number_percent() < ch->level &&
	number_percent() < get_curr_stat(ch,STAT_INT)
	&& !IS_SET(ch->mhs,MHS_SAVANT) )
   {
	ch->hit /= 2;
	ch->hit++;
	send_to_char("A sudden shooting pain runs through your forearms.\n\r",ch);
	SET_BIT(ch->mhs,MHS_SAVANT);
	act("$n cringes in sudden pain.",ch,NULL,NULL,TO_ROOM,FALSE);
	return;
    }
    
    return;
}

void check_nethermancer( CHAR_DATA *ch )
{
    OBJ_DATA *weapon;

   if ( !HAS_KIT(ch,"nethermancer") || IS_SET(ch->comm,COMM_AFK) )
	return;

//    changed the line below to be > 15 instead of <12
   if ( number_percent() * number_percent() < 20 &&
	((weapon = get_eq_char(ch,WEAR_WIELD)) != NULL) &&
// changed the line below to 4:2 instead of 5:2
	number_percent() < ch->level / (weapon->enchanted?5:2) &&
	(!IS_SET(weapon->value[4],WEAPON_NETHER))  &&
        (!IS_SET(weapon->value[4],WEAPON_FAVORED)) )
   {
        wiznet("{YNether Weapon made by:  $N.{x",ch,NULL,WIZ_NOTES,WIZ_SECURE,get_trust(ch));
	SET_BIT(weapon->value[4],WEAPON_NETHER);
	act("$p glows with a black aura.",ch,weapon,NULL,TO_CHAR,FALSE);
	act("$p glows with a black aura.",ch,weapon,NULL,TO_ROOM,FALSE);
   }
}

/*Here goes the vampiric touch check -Boogums*/
void check_vampirictouch( CHAR_DATA *ch)
{
  OBJ_DATA *weapon;

 // if( !HAS_KIT(ch,"necromancer") || ch->position != POS_FIGHTING )
 //   return;
  if ( !HAS_KIT(ch,"necromancer") || IS_SET(ch->comm,COMM_AFK) )
  {
    return;
  }
  if( 
      //number_percent() * number_percent() < 25       &&
      number_percent() * number_percent() < 50       &&
      ((weapon = get_eq_char(ch,WEAR_WIELD)) != NULL) &&
      number_percent() < ch->level / (weapon->enchanted?5:2)  &&
      (!IS_SET(weapon->value[4],WEAPON_VAMPIRIC))  &&
      (!IS_SET(weapon->value[4],WEAPON_FAVORED)) 
    )
    {
      wiznet("Vampiric Weapon made by:  $N.",ch,NULL,WIZ_DEITYFAVOR,0,get_trust(ch));
      SET_BIT(weapon->value[4],WEAPON_VAMPIRIC);
      act("$p suddenly looks a bit more {Dwicked{x.",ch,weapon,NULL,TO_CHAR,FALSE);
      act("$p suddenly looks a bit more {Dwicked{x.",ch,weapon,NULL,TO_ROOM,FALSE);
    }
}  


void check_mutate( CHAR_DATA *ch )
{
   if ( IS_NPC(ch) || !IS_SET(ch->mhs,MHS_MUTANT) )
	return;

   if ( (--ch->pcdata->mutant_timer)/2 < number_percent() )
   {
	int i;
	int new_race = race_lookup("mutant");

	ch->pcdata->mutant_timer = 100 + dice(30,30); /* up to 1000 ticks */

	new_race = number_range( 1, MAX_PC_RACE - 1 );

	/* strip all magic */

	 ch->affected_by = race_table[new_race].aff;
	 ch->imm_flags   = race_table[new_race].imm;
	 ch->res_flags   = race_table[new_race].res;
	 ch->vuln_flags  = race_table[new_race].vuln;
	 ch->form        = race_table[new_race].form;
         ch->parts       = race_table[new_race].parts;
		       
		      
	 for (i = 0; i < 5; i++)
         {
              if (pc_race_table[new_race].skills[i] == NULL)
   	   	break;
    	 	group_add(ch,pc_race_table[new_race].skills[i],FALSE);
        }
		
	ch->size = pc_race_table[new_race].size;
	ch->race = new_race;
	send_to_char("Your body shimmers and shakes.\n\r",ch);
	act("$n's body shimmers and shakes.",ch,NULL,NULL,TO_ROOM,FALSE);
   }

   return;
}

/*
 * Advancement stuff.
 */
void advance_level( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    int add_hp;
    int add_mana;
    int add_move;
    float mana,fInt,fWis,fP1,fP2,fP3;
    int add_prac;
    int old_class;
    /* get real size of char, prevent inflating hp gain with stature */
    int real_size;

if( ch != NULL )
 {
    old_class = !IS_NPC(ch) ? ch->pcdata->old_class : 3;

    ch->pcdata->last_level = 
  ( ch->played + (int) (current_time - ch->logon) ) / 3600;

/*    add_hp  = 3*(con_app[get_curr_stat(ch,STAT_CON)].hitp + number_range(
        class_table[old_class].hp_min,
        class_table[old_class].hp_max ))/3; */
    add_hp  = 3*(con_app[get_curr_stat(ch,STAT_CON)].hitp + number_range(
        class_table[old_class].hp_min,
        class_table[old_class].hp_max ))/4;// Replacing END with CON to not screw up gains, but this is broken
    add_hp  = add_hp + ((con_app[get_curr_stat(ch,STAT_STR)].hitp + number_range(
        class_table[old_class].hp_min,
        class_table[old_class].hp_max ))/5);

    real_size = pc_race_table[ch->race].size;
   /*  add_hp += ( ch->size - 2 ); */
    add_hp += ( real_size - 2 );

    /* Here we go.  Trying new mana syustem  ***
    add_mana  = number_range(3,
	(2*(get_curr_stat(ch,STAT_INT) + get_curr_stat(ch,STAT_WIS)))/5);
    if ( add_mana <= 4 )
    {
    add_mana += number_range(3,(2*get_curr_stat(ch,STAT_INT)
                 + get_curr_stat(ch,STAT_WIS))/5);
    add_mana /= 2;
    }
    ***/

/* This code provided by Marc Labelle (Ghor) */
    fInt = get_curr_stat(ch,STAT_INT) * 1.0;
    fWis = get_curr_stat(ch,STAT_WIS) * 1.0;
    fP1 = number_percent() * 1.0 / 100;
    fP2 = number_percent() * 1.0 / 100;
    fP3 = number_percent() * 1.0 / 100;

    mana = (fInt * .2 * ( 1 + fP1 + fP2 ) );
    mana += ( fWis * .1 * ( 1 + fP3 ) );
  
    if ( ch->race == race_lookup("elf") || ch->race == race_lookup("half-elf"))
	mana *= 1.1;

    if ( ch->race == race_lookup("faerie") )
        mana *= 1.25;

    if ( HAS_KIT(ch,"bishop") )
        mana *= 1.1;

    /* if (class_table[ch->class].fMana == 0)
    */
    if (class_table[old_class].fMana == 0)
  	mana *= .5;

    /*if ( class_table[ch->class].fMana == 1 )
    */
    if ( class_table[old_class].fMana == 1 )
  	mana *= .75;

/* changing move gains
    add_move  = number_range( 1, (get_curr_stat(ch,STAT_CON)
          + get_curr_stat(ch,STAT_DEX))/6 );
	  */
    add_prac  = wis_app[get_curr_stat(ch,STAT_WIS)].practice;

     /* Kits */
    if (HAS_KIT(ch,"prophet"))
	add_prac += 1;

    add_mana = (int) mana;
    add_hp  = UMAX(  2, add_hp   );
    add_mana = UMAX( 2, add_mana );
    /*
    add_move  = UMAX(  6, add_move );
    */

    add_move = 3;
    if (get_curr_stat(ch,STAT_STR) > 24)
       add_move += 1;
    if (get_curr_stat(ch,STAT_STR) > 23)
       add_move += 1;
    if (get_curr_stat(ch,STAT_STR) > 21)
       add_move += 1;
    if (get_curr_stat(ch,STAT_STR) > 19)
       add_move += 1;
    if (get_curr_stat(ch,STAT_STR) > 17)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 24)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 23)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 22)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 21)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 20)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 19)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 18)
       add_move += 1;
    if (get_curr_stat(ch,STAT_DEX) > 17)
       add_move += 1;


    if ( ch->race == race_lookup("rockbiter") )
       add_move -= 1;

    if (IS_SET(ch->act,PLR_MUMMY) || ch->race == race_lookup("gargoyle") )
       add_move /= 2;

    if ( HAS_KIT(ch,"ranger") )
	add_move += 3;

    if ( is_affected(ch,gsn_spirit_of_boar) )
	add_hp++;

    if ( is_affected(ch,gsn_spirit_of_owl) )
	add_mana++;

    ch->max_hit   += add_hp;
    ch->max_mana  += add_mana;
    ch->max_move  += add_move;
    ch->pcdata->perm_hit  += add_hp;
    ch->pcdata->perm_mana += add_mana;
    ch->pcdata->perm_move += add_move;

    ch->practice  += add_prac;
    ch->train   += 1;

    /* Clear last attacked by on level */
    ch->pcdata->last_attacked_by = str_dup("no one");
    ch->pcdata->last_attacked_by_timer = 0;

  if ( ch->desc != NULL && ch->desc->connected == CON_PLAYING )
   {
     sprintf( buf, "You gain %d hp, %d mana, %d moves and %d/%d prac.\n\r",
	add_hp, add_mana, add_move,
	      add_prac, ch->practice );
    send_to_char( buf, ch );

    if ( ch->level == 9 )
	do_help(ch,"levelnine");
    else
    if ( ch->level == 10 )
	do_help(ch,"levelten");
    else
    if ( ch->level == 11 )
	do_help(ch,"leveleleven");
    else
    if ( ch->level == 20 )
	do_help(ch,"leveltwenty");

   }
  }
    return;
}   

void do_level( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    if ( IS_NPC(ch) ) return;

    if ( ch->exp < exp_per_level(ch,ch->pcdata->points) * (ch->level+1) &&
         ch->pcdata->debit_level <= 0 ) {
	send_to_char("You have no debit levels available.\n\r",ch);
	return;
    }

    if ( ch->level >= 51 ) {
	send_to_char("You cannot level past 51.\n\r",ch);
	return;
    }

    if (IS_SET(ch->mhs, MHS_GLADIATOR))
    {
       send_to_char("You cannot level inside the Gladiator arena.\n\r", ch);
       return;
    }


    if (IS_SET(ch->mhs,MHS_SHAPESHIFTED) || IS_SET(ch->mhs,MHS_SHAPEMORPHED))
    {
  sprintf( buf, "You are not allowed to train while shapeshifted.\n\r");
  send_to_char( buf,ch);
  return;
    }

    if (ch->fighting != NULL)
    {
       send_to_char("You can not level while fighting.\n\r",ch);
       return;
    }

    if(ch->level == 20 && !is_clan(ch))
    {// Using confirm_outcast to not interfere with confirm_loner
      if(!ch->pcdata->confirm_outcast)
      {
        send_to_char("{YDid you mean to level without clanning?{x\n\rEnter '{Wlevel{x' again to continue or '{Wloner{x' first.\n\r", ch);
        ch->pcdata->confirm_outcast = TRUE;
        return;
      }
      ch->pcdata->confirm_outcast = FALSE;
    }
    
	if( ch->pcdata->debit_level > 0 ) 
    {
    	ch->pcdata->debit_level--;
	ch->exp += exp_per_level(ch,ch->pcdata->points);
	sprintf(buf,"%s is using a debit level (%d remaining)",
		ch->name, ch->pcdata->debit_level );
	wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);

	/* Give Pfresh Chars 10 skill points up to 500 max
	   ie. not a remort */
        if (!IS_SET(ch->act,PLR_VAMP) &&
	    !IS_SET(ch->act,PLR_WERE) &&
	    !IS_SET(ch->act,PLR_MUMMY))
	   ch->skill_points += 10;
    }
/*
    if ( ch->exp < exp_per_level(ch,ch->pcdata->points) * (ch->level+1) ) {
    	ch->pcdata->debit_level--;
	sprintf(buf,"%s is using a debit level (%d remaining)",
		ch->name, ch->pcdata->debit_level );
	wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);
    }
*/
    ch->level++;
    advance_level(ch);
    save_char_obj(ch);
		 
    sprintf(buf,"%s has attained level %d",ch->name,ch->level);
    log_string(buf);
    sprintf(buf,"$N has attained level %d!",ch->level);
    wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);
    pnet(buf,ch,NULL,PNET_LEVELS,0,0);
    return;
}

void gain_exp( CHAR_DATA *ch, long gain )
{
    char buf[MAX_STRING_LENGTH];
    int count = 1;
    int cur_exp;

    if ( IS_NPC(ch) || ch->level >= LEVEL_HERO )
  return;

    /* Do not gain or lose any exp if you have debit levels */
    if (ch->pcdata->debit_level > 0)
       gain = 0;


    cur_exp = ch->exp;
    ch->exp = UMAX( exp_per_level(ch,ch->pcdata->points), ch->exp + gain );
  if(cur_exp != ch->exp)
  {
    while ( ch->level < LEVEL_HERO && ch->exp >= 
  exp_per_level(ch,ch->pcdata->points) * (ch->level+count ) )
    {
   sprintf(buf,"You qualify for level %d!!\n\r", ch->level + count );
   send_to_char(buf,ch);
   if (ch->level == 20)
	do_help(ch, "twentywarning");
   /*
       sprintf(buf,"%s qualifies for level %d",ch->name,ch->level+count);
        log_string(buf);
        sprintf(buf,"$N qualifies for level %d!",ch->level+count);
        wiznet(buf,ch,NULL,WIZ_LEVELS,0,0);
	*/
  /*advance_level( ch );*/
  save_char_obj(ch);
   count++;
    }
  }

    return;
}



/*
 * Regeneration stuff.
 */
int hit_gain( CHAR_DATA *ch )
{
    int gain;
    int number;

    int has_medium = 0;

    if (ch->in_room == NULL)
  return 0;

    if ( IS_NPC(ch) )
    {
  gain =  5 + ch->level;
  if (IS_AFFECTED(ch,AFF_REGENERATION))
      gain *= 2;

  switch(ch->position)
  {
      default :     gain /= 2;      break;
      case POS_SLEEPING:  gain = 3 * gain/2;    break;
      case POS_RESTING:           break;
      case POS_FIGHTING:  gain /= 3;      break;
  }

  
    }
    else
    {
  has_medium = room_has_medium(ch);
  gain = UMAX(3,get_curr_stat(ch,STAT_CON) - 3 + ch->level/2); 
  gain += class_table[ch->class].hp_max - 10;
  number = number_percent();
  if (number < get_skill(ch,gsn_fast_healing))
  {
      gain += number * gain / 100;
      if (ch->hit < ch->max_hit)
    check_improve(ch,gsn_fast_healing,TRUE,8);
  }

       if(IS_SET(ch->act,PLR_MUMMY) && !IS_NPC(ch))
       {
          number = number_percent();
          gain += number * gain / 100;
       }

  switch ( ch->position )
  {
      default:      gain /= 4;      break;
      case POS_SLEEPING:          break;
      case POS_RESTING:   gain /= 2;      break;
      case POS_FIGHTING:  gain /= 6;      break;
  }

  if (!has_medium && ch->pcdata->condition[COND_HUNGER]   == 0 )
      gain /= 2;

  if (!has_medium && ch->pcdata->condition[COND_THIRST] == 0 )
      gain /= 2;

  if ( ch->pcdata->condition[COND_DRUNK] > 10 )
      gain = (gain + 1)*3 / 2;
    }

    if(!IS_NPC(ch) && !( ch->in_room->clan && IS_SET(ch->pcdata->clan_flags, CLAN_NO_REGEN)) )
    {
    gain = gain * ch->in_room->heal_rate / 100;
    }

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
  gain = gain * ch->on->value[3] / 100;

    if (!has_medium && IS_AFFECTED(ch, AFF_POISON) )
  gain /= 4;

    if (!has_medium && IS_AFFECTED(ch, AFF_PLAGUE))
  gain /= 8;

    if(!has_medium && !IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_NPC(ch))
    {
       if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
          gain /=2 ;
    }
    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       if(IS_SET(ch->in_room->room_flags,ROOM_HOLY_GROUND))
          gain = 0;
       else
	  gain *= 3;
    }

    if ( ch->race == race_lookup("rockbiter") )
	gain = (4*gain)/5;


    if ( ch->race == race_lookup("gargoyle") )
    {
	if ( out_of_element(ch) )
		gain = gain *2/3;
	else
		gain = gain * 3/2;
    }

    if (ch->race == race_lookup("smurf"))
       gain *= 2;

    if ( has_medium )
        gain = ( gain * ( 100 + has_medium ) / 100 );

    if(is_clan(ch) && ch->level <= 20)/* Lower level clanners regen better */
      gain = (gain * 100) / (50 + (ch->level - 5) * 2);/* 200% - 125% */

    return UMIN(gain, ch->max_hit - ch->hit);
}



int mana_gain( CHAR_DATA *ch )
{
    int gain;
    int number;

    int has_medium = 0;

    if (ch->in_room == NULL)
  return 0;

    if ( IS_NPC(ch) )
    {
  gain = 5 + ch->level;
  switch (ch->position)
  {
      default:    gain /= 2;    break;
      case POS_SLEEPING:  gain = 3 * gain/2;  break;
        case POS_RESTING:       break;
      case POS_FIGHTING:  gain /= 3;    break;
      }
    }
    else
    {
  has_medium = room_has_medium(ch);
  gain = (get_curr_stat(ch,STAT_WIS) 
        + get_curr_stat(ch,STAT_INT) + ch->level) / 2;
  number = number_percent();
  if (number < get_skill(ch,gsn_meditation))
  {
      gain += number * gain / 100;
      if (ch->mana < ch->max_mana)
          check_improve(ch,gsn_meditation,TRUE,8);
  }


       if(IS_SET(ch->act,PLR_MUMMY) && !IS_NPC(ch))
       {
          number = number_percent();
          gain += number * gain / 100;
       }

  if (class_table[ch->class].fMana == 0)
      gain /= 2;

  switch ( ch->position )
  {
      default:    gain /= 4;      break;
      case POS_SLEEPING:          break;
      case POS_RESTING: gain /= 2;      break;
      case POS_FIGHTING:  gain /= 6;      break;
  }

  if (!has_medium && ch->pcdata->condition[COND_HUNGER]   == 0 )
      gain /= 2;

  if (!has_medium && ch->pcdata->condition[COND_THIRST] == 0 )
      gain /= 2;

  if ( ch->pcdata->condition[COND_DRUNK] > 10 )  
      gain = (gain + 1)*3 / 2;

    }

    if(!IS_NPC(ch) && !( ch->in_room->clan && IS_SET(ch->pcdata->clan_flags, CLAN_NO_REGEN)) )
    {
    gain = gain * ch->in_room->mana_rate / 100;
    }

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
  gain = gain * ch->on->value[4] / 100;

    if (!has_medium && IS_AFFECTED( ch, AFF_POISON ) )
  gain /= 4;

    if (!has_medium && IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if(!has_medium && !IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_NPC(ch))
    {
       if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
          gain /=2 ;
    }

    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       if(IS_SET(ch->in_room->room_flags,ROOM_HOLY_GROUND))
          gain = 0;
       else
	  gain *= 3;
    }

    if ( ch->race == race_lookup("rockbiter") )
	gain = (4*gain)/5;

    if ( ch->race == race_lookup("gargoyle") )
    {
	if ( out_of_element(ch) )
	    gain = gain *2/3;
	else
	    gain  = gain * 3/2;
    }

    if (ch->race == race_lookup("smurf"))
       gain *= 2;
  
    if( is_affected(ch,gsn_clarity) )
	gain = ( 100 + (ch->level/2) ) * gain / 100;

    if ( has_medium )
	gain = ( gain * ( 100 + has_medium ) / 100 );

    if(is_clan(ch) && ch->level <= 20)/* Lower level clanners regen better */
      gain = (gain * 100) / (50 + (ch->level - 5) * 2);/* 200% - 125% */

    return UMIN(gain, ch->max_mana - ch->mana);
}



int move_gain( CHAR_DATA *ch )
{
    int gain;
    int has_medium = 0;

    if (ch->in_room == NULL)
  return 0;

    if ( IS_NPC(ch) )
    {
  gain = ch->level;
    }
    else
    {
  has_medium = room_has_medium(ch);
  gain = UMAX( 15, ch->level );

  switch ( ch->position )
  {
  case POS_SLEEPING: gain += get_curr_stat(ch,STAT_DEX);    break;
  case POS_RESTING:  gain += get_curr_stat(ch,STAT_DEX) / 2;  break;
  }

  if (!has_medium && ch->pcdata->condition[COND_HUNGER]   == 0 )
      gain /= 2;

  if (!has_medium && ch->pcdata->condition[COND_THIRST] == 0 )
      gain /= 2;
    }

    gain = gain * ch->in_room->heal_rate/100;

    if (ch->on != NULL && ch->on->item_type == ITEM_FURNITURE)
  gain = gain * ch->on->value[3] / 100;

    if (!has_medium && IS_AFFECTED(ch, AFF_POISON) )
  gain /= 4;

    if (!has_medium && IS_AFFECTED(ch, AFF_PLAGUE))
        gain /= 8;

    if(!has_medium && !IS_SET(ch->mhs,MHS_HIGHLANDER) && !IS_NPC(ch))
    {
       if (IS_AFFECTED(ch,AFF_HASTE) || IS_AFFECTED(ch,AFF_SLOW))
          gain /=2 ;
    }

    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       if(IS_SET(ch->in_room->room_flags,ROOM_HOLY_GROUND))
          gain = 0;
       else
	  gain *= 3;
    }

    if ( ch->race == race_lookup("rockbiter") )
	gain = (4*gain)/5;

    if ( out_of_element(ch) && ch->race == race_lookup("gargoyle") )
	gain /= 2;

    if (ch->race == race_lookup("smurf"))
       gain *= 2;

    if ( has_medium )
        gain = ( gain * ( 100 + has_medium ) / 100 );

    if(is_clan(ch) && ch->level <= 20)/* Lower level clanners regen better */
      gain = (gain * 100) / (50 + (ch->level - 5) * 2);/* 200% - 125% */

    return UMIN(gain, ch->max_move - ch->move);
}



void gain_condition( CHAR_DATA *ch, int iCond, int value )
{
    int condition;

    if ( value == 0 || IS_NPC(ch) || (ch->level >= LEVEL_IMMORTAL && value < 0))
  return;

    if ( ch->race == race_lookup("rockbiter") )
	value /= 2;

    condition       = ch->pcdata->condition[iCond];
    if (condition == -1)
        return;
    ch->pcdata->condition[iCond]  = URANGE( 0, condition + value, 48 );

   if ( IS_SET(ch->comm, COMM_SILENCE) )
	return;

    if ( ch->pcdata->condition[iCond] == 0 )
    {

  switch ( iCond )
  {
  case COND_HUNGER:
      if ( IS_SET( ch->act,PLR_VAMP) )
	  send_to_char("Your throat aches for blood.\n\r",ch);
      else
          send_to_char( "You are hungry.\n\r",  ch );
      break;

  case COND_THIRST:
      if ( IS_SET( ch->act, PLR_VAMP) )
	  send_to_char("Your throat aches for blood.\n\r",ch);
      else
          send_to_char( "You are thirsty.\n\r", ch );
      break;

  case COND_DRUNK:
      if ( condition != 0 )
    send_to_char( "You are sober.\n\r", ch );
      break;
  }
    }

    return;
}



/*
 * Mob autonomous action.
 * This function takes 25% to 35% of ALL Merc cpu time.
 * -- Furey
 */
void mobile_update( void )
{
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    EXIT_DATA *pexit;
    int door;

    /* Examine all mobs. */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
  ch_next = ch->next;

  if ( !IS_NPC(ch) || ch->in_room == NULL 
	|| (IS_AFFECTED(ch,AFF_CHARM) && !IS_SET(ch->mhs, MHS_ELEMENTAL)) )
      continue;

  if (ch->in_room->area && ch->in_room->area->empty && !IS_SET(ch->act,ACT_UPDATE_ALWAYS))
      continue;

  /* Examine call for special procedure */
  if ( ch->spec_fun != 0 )
  {
      if ( (*ch->spec_fun) ( ch ) )
    continue;
  }

  if (ch->pIndexData->pShop != NULL) /* give him some gold */
      if ((ch->gold * 100 + ch->silver) < ch->pIndexData->wealth)
      {
    ch->gold += ch->pIndexData->wealth * number_range(1,20)/5000000;
    ch->silver += ch->pIndexData->wealth * number_range(1,20)/50000;
      }
   

  /* That's all for sleeping / busy monster, and empty zones */
  if ( ch->position != POS_STANDING )
      continue;

  /* Scavenge */
  if ( IS_SET(ch->act, ACT_SCAVENGER)
  &&   ch->in_room->contents != NULL
  &&   number_bits( 6 ) == 0 && ch->in_room->area && !ch->in_room->area->freeze)
  {
      OBJ_DATA *obj;
      OBJ_DATA *obj_best;
      int max;

      max         = 1;
      obj_best    = 0;
      for ( obj = ch->in_room->contents; obj; obj = obj->next_content )
      {
    if ( CAN_WEAR(obj, ITEM_TAKE) && can_loot(ch, obj, TRUE)
         && obj->cost > max  && obj->cost > 0 && count_users(obj) == 0 )
    {
        obj_best    = obj;
        max         = obj->cost;
    }
      }

      if ( obj_best )
      {
    obj_from_room( obj_best );
    obj_to_char( obj_best, ch );
    act( "$n gets $p.", ch, obj_best, NULL, TO_ROOM ,FALSE);
      }
  }

  /* Wander */
  if ( !ch->in_room->area->freeze
  && !IS_SET(ch->act, ACT_SENTINEL) 
  && number_bits(3) == 0
  && ( door = number_bits( 5 ) ) <= 5
  && ( pexit = ch->in_room->exit[door] ) != NULL
  &&   pexit->u1.to_room != NULL
  &&   !IS_SET(pexit->exit_info, EX_CLOSED)
  &&   !IS_SET(pexit->u1.to_room->room_flags, ROOM_NO_MOB)
  && ( !IS_SET(ch->act, ACT_STAY_AREA)
  ||   pexit->u1.to_room->area == ch->in_room->area ) 
  && ( !IS_SET(ch->act, ACT_OUTDOORS)
  ||   !IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)) 
  && ( !IS_SET(ch->act, ACT_INDOORS)
  ||   IS_SET(pexit->u1.to_room->room_flags,ROOM_INDOORS)))
  {
      move_char( ch, door, FALSE );
  }                     
    }

    return;
}


void load_rbow_char(ROOM_INDEX_DATA *pRoom, int vnum)
{
  AFFECT_DATA af;
  CHAR_DATA *rbowchar = create_mobile(get_mob_index(vnum));
  char_to_room(rbowchar, pRoom);
  af.where     = TO_AFFECTS;
  af.type      = skill_lookup("wraithform");
  af.level     = rbowchar->level;
  af.duration  = -1;
  af.location  = 0;
  af.modifier  = 0;
  af.bitvector = 0;
  affect_to_char( rbowchar, &af );
  act("$n touches down right in front of you!", rbowchar, NULL, NULL, TO_ROOM, FALSE);
  /* Activate the AI just in case there's a player in the room already */ 
  spec_rainbow(rbowchar);
}

bool spawn_rainbow(void)
{
  AREA_DATA *pArea;
  ROOM_INDEX_DATA *pRoom;
  int target, sec_target, count = 0;
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
  {
    if(!pArea->under_develop)
      count++;
  }
  target = number_range(1, count);
  for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
  {
    if(!pArea->under_develop)
    {
      target--;
      if(!target)
        break;
    }
  }
  if(!pArea)
  {
    bug("No area found to spawn the rainbow in.", 0);
    rainbow = 0;
    return FALSE;
  }

  count = 0;
  for(target = pArea->min_vnum_room; target < pArea->max_vnum_room; target++)
  {
    pRoom = get_room_index(target);
    if(pRoom && !IS_SET(pRoom->room_flags, ROOM_INDOORS))
      count++;
  }
  if(count < 2)/* Sometimes it can't spawn */
  {
    rainbow = 0;
    return FALSE;
  }

  /* There's a couple outdoor rooms to spawn the rainbow in */
  target = number_range(1, count);
  while((sec_target = number_range(1, count)) == target);// Fun!
  for(count = pArea->min_vnum_room; count < pArea->max_vnum_room; count++)
  {
    pRoom = get_room_index(count);
    if(pRoom && !IS_SET(pRoom->room_flags, ROOM_INDOORS))
    {
      if(target)
      {
        target--;
        if(!target)
          load_rbow_char(pRoom, MOB_VNUM_RAINBOW);
      }
      if(sec_target)
      {
        sec_target--;
        if(!sec_target)
          load_rbow_char(pRoom, MOB_VNUM_RAINBOW);
      }
      if(!target && !sec_target)
        break;
    }
  }
  rainbow_area = pArea;
  return TRUE;
}

/*
 * Update the weather.
 */
void weather_update( void )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *d;
    int diff;

  if(rainbow)
  {
    if(rainbow > 0)
      rainbow--;
    else
      rainbow++;
  }
    buf[0] = '\0';

    switch ( ++time_info.hour )
    {
    case  5:
  weather_info.sunlight = SUN_LIGHT;
  strcat( buf, "The day has begun.\n\r" );
  break;

    case  6:
  weather_info.sunlight = SUN_RISE;
  strcat( buf, "The sun rises in the east.\n\r" );
  break;

    case 19:
  weather_info.sunlight = SUN_SET;
  strcat( buf, "The sun slowly disappears in the west.\n\r" );
  break;

    case 20:
  weather_info.sunlight = SUN_DARK;
  strcat( buf, "The night has begun.\n\r" );
  break;

    case 24:
  time_info.hour = 0;
  time_info.day++;
  break;
    }

    if ( time_info.day   >= 35 )
    {
  time_info.day = 0;
  time_info.month++;
    }

    if ( time_info.month >= 17 )
    {
  time_info.month = 0;
  time_info.year++;
    }

    /*
     * Weather change.
     */
    if ( time_info.month >= 9 && time_info.month <= 16 )
  diff = weather_info.mmhg >  985 ? -2 : 2;
    else
  diff = weather_info.mmhg > 1015 ? -2 : 2;

    weather_info.change   += diff * dice(1, 4) + dice(2, 6) - dice(2, 6);
    weather_info.change    = UMAX(weather_info.change, -12);
    weather_info.change    = UMIN(weather_info.change,  12);

    weather_info.mmhg += weather_info.change;
    weather_info.mmhg  = UMAX(weather_info.mmhg,  960);
    weather_info.mmhg  = UMIN(weather_info.mmhg, 1040);

    switch ( weather_info.sky )
    {
    default: 
  bug( "Weather_update: bad sky %d.", weather_info.sky );
  weather_info.sky = SKY_CLOUDLESS;
  break;

    case SKY_CLOUDLESS:
  if ( weather_info.mmhg <  990
  || ( weather_info.mmhg < 1010 && number_bits( 2 ) == 0 ) )
  {
      strcat( buf, "The sky is getting cloudy.\n\r" );
      weather_info.sky = SKY_CLOUDY;
  }
  break;

    case SKY_CLOUDY:
  if ( weather_info.mmhg <  970
  || ( weather_info.mmhg <  990 && number_bits( 2 ) == 0 ) )
  {
      strcat( buf, "It starts to rain.\n\r" );
      weather_info.sky = SKY_RAINING;
  }

  if ( weather_info.mmhg > 1030 && number_bits( 2 ) == 0 )
  {
      strcat( buf, "The clouds disappear.\n\r" );
      weather_info.sky = SKY_CLOUDLESS;
  }
  break;

    case SKY_RAINING:
  if ( weather_info.mmhg <  970 && number_bits( 2 ) == 0 )
  {
      strcat( buf, "Lightning flashes in the sky.\n\r" );
      weather_info.sky = SKY_LIGHTNING;
  }

  if ( weather_info.mmhg > 1030
  || ( weather_info.mmhg > 1010 && number_bits( 2 ) == 0 ) )
  {
      strcat( buf, "The rain stopped.\n\r" );
      weather_info.sky = SKY_CLOUDY;
      if(time_info.hour >= 6 && time_info.hour < 19 && !rainbow)
      {/* Rainbows only start in the day */
        if(current_time - rainbow_found > 36000 || (current_time - rainbow_found > 300 && number_percent() < 30))
        {/* Always happens at 10 hours+, can happen anytime after 5 minutes */
          rainbow = 15; /* 15 ticks of rainbow appearing */
          if(spawn_rainbow())
          {
            //log_quest_detail("A rainbow has formed.", TASK_RAINBOW);
          }
        }
      }
  }
  break;

    case SKY_LIGHTNING:
  if ( weather_info.mmhg > 1010
  || ( weather_info.mmhg >  990 && number_bits( 2 ) == 0 ) )
  {
      strcat( buf, "The lightning has stopped.\n\r" );
      weather_info.sky = SKY_RAINING;
      break;
  }
  break;
    }

    if ( buf[0] != '\0' )
    {
  for ( d = descriptor_list; d != NULL; d = d->next )
  {
      if ( d->connected == CON_PLAYING
      &&   IS_OUTSIDE(d->character)
      &&   IS_AWAKE(d->character)
      &&  !IS_SET(d->character->comm, COMM_SILENCE) )
    send_to_char( buf, d->character );
  }
    }

    return;
}



/*
 * Update all chars, including mobs.
*/
void char_update( void )
{   
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *ch_quit;    
    char buf[MAX_STRING_LENGTH];
    ch_quit = NULL;

    /* update save counter */
    save_number++;

    if (save_number > 14)
  save_number = 0;

    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;

  ch_next = ch->next;

	check_mutate( ch );
	/*
	check_savant( ch );
	*/
        if ( IS_SET(ch->mhs,MHS_SAVANT) )
	    REMOVE_BIT(ch->mhs,MHS_SAVANT);
	check_shapeshifted( ch );
	check_nethermancer( ch );
	check_vampirictouch( ch );

	if ( !IS_NPC(ch) && ch->class == class_lookup("paladin") )
	    {
		if (ch->pcdata->abolish_timer-- < 0 )
			ch->pcdata->abolish_timer = 0;
	    }
       
  if ( !IS_NPC(ch) )
            {
                if(ch->pcdata->rlock_time > 0)
                {
                  ch->pcdata->rlock_time--;
                  if(!ch->pcdata->rlock_time)
                    send_to_char("Your reply lock has timed out.\n\r", ch);
                }
                damage_decrement(ch);
                if (ch->clan == nonclan_lookup("newbie") &&
                    get_age(ch) > 20)
                {
                    send_to_char("Your newbie flag has been removed.",ch);
                    ch->clan = 0;
                }
/*                if (ch->clan != nonclan_lookup("zealot") && ch->pcdata->deity == deity_lookup("almighty") )
                {
                    send_to_char("The Almighty has lost faith in you, and you turn to Mojo.\n\r",ch);
                    ch->pcdata->deity = deity_lookup("mojo");
                }*/

                if (ch->pcdata->trap_timer-- < 0 )
                        ch->pcdata->trap_timer = 0;
               
/* REMOVED by Nightdagger on 04/13/03
                if (ch->pcdata->deity_timer-- < 0 && ch->pcdata->deity != ch->pcdata->new_deity )
		        {
		        sprintf(buf, "You now worship %s.", deity_table[ch->pcdata->new_deity].pname);
		        ch->pcdata->deity = ch->pcdata->new_deity;
		        }
*/
        if(ch->pcdata->corpse_timer > 0)
          ch->pcdata->corpse_timer--;
	if(ch->pcdata->deity_favor_timer > 0)
		{
			ch->pcdata->deity_favor_timer--;
			if(!ch->pcdata->deity_favor_timer)
			{
				if(ch->pcdata->deity_trial_timer > 0)
					ch->pcdata->deity_trial_timer = 1;// Ensure failure
				sprintf(buf, "%s's gaze turns away from you.\n\r", deity_table[ch->pcdata->deity].pname);
				send_to_char(buf, ch);
			}
			if(ch->pcdata->deity_trial_timer > 0)
			{
				ch->pcdata->deity_trial_timer--;
				if(!ch->pcdata->deity_trial_timer)
				{
					sprintf(buf, "You have failed to complete the trial from %s in time.\n\r", deity_table[ch->pcdata->deity].pname);
					send_to_char(buf, ch);
					log_deity_favor(ch, NULL, DEITY_TRIAL_FAIL_TIMER);
				}
				else if(ch->pcdata->deity_trial_timer == 2)
				{
					send_to_char("{RHurry!{x You are running out of time on your trial!\n\r", ch);
				}
			}
		}

            }
 
	if ( IS_SET(ch->mhs, MHS_BANISH))
	  {
	  REMOVE_BIT(ch->mhs, MHS_BANISH);
	  send_to_char("The force blocking the entrance to your clan hall is gone.\n\r", ch);
	  }

        /* skill point tracker set not gaining skill points */
	if (!IS_NPC(ch) )
	{
           if (ch->pcdata->skill_point_tracker > 0)
	   {
	    /* If they have skill points, they should have a timer */
	      ch->pcdata->skill_point_timer =
	      	UMAX(0,ch->pcdata->skill_point_timer -1);


	      if (ch->pcdata->skill_point_timer ==  0) 
	      {
	         ch->pcdata->skill_point_tracker = 
		   UMAX(0,ch->pcdata->skill_point_tracker - 1);
		 if (ch->pcdata->skill_point_tracker > 0)
			ch->pcdata->skill_point_timer = 2;
	      }
		 
	   }

	   if(ch->pcdata->last_attacked_by_timer == 0)
   	      ch->pcdata->last_attacked_by = str_dup("no one");
	   if (ch->pcdata->last_attacked_by_timer > 0)
	      ch->pcdata->last_attacked_by_timer--;
	   if (ch->pcdata->last_death_timer > 0)
	      ch->pcdata->last_death_timer--;
	}

        if ( ch->timer > 30 )
            ch_quit = ch;
            
  if (ch->position == POS_SLEEPING && !IS_NPC(ch)) {
     if ( IS_SET(ch->display, DISP_PROMPT)    && (ch->desc))
        send_to_char ("",ch);
  }             

  if ( ch->position >= POS_STUNNED )
  {
    
            /* check to see if we need to go home */
      if (IS_NPC(ch) && ch->zone != NULL 
	  && ch->zone != ch->in_room->area
          && ch->desc == NULL &&  ch->fighting == NULL 
          && !IS_AFFECTED(ch,AFF_CHARM) 
	  && ( number_percent() < 5 
		|| (!str_prefix("spec_guard",spec_name(ch->spec_fun))
		    && number_percent() < 33) ) 
	  && !ch->in_room->area->freeze
	  && ch->in_room != get_room_index(ROOM_VNUM_LIMBO)
          && ch->passenger == NULL
	  && ch->pIndexData->pShop == NULL) 
	   {
              act("$n wanders on home.",ch,NULL,NULL,TO_ROOM,FALSE);
              extract_char(ch,TRUE);
              continue;
            }


	 /* check to see if we get moved from air/water currents,
	    move players only who are in a standing position  */

	 if ( !IS_NPC(ch)
	     && ( ch->position >= POS_STANDING ) 
	     && ( ch->in_room != NULL )
	     && (    ch->in_room->sector_type == SECT_AIR
		 || ch->in_room->sector_type == SECT_WATER_NOSWIM 
		 || ch->in_room->sector_type == SECT_WATER_SWIM          
	        )
	     && ( number_percent() > get_skill(ch, gsn_swim ) )
	    )
         {

		 EXIT_DATA *pexit;
		 int original_type, new_type;
		 int door = number_door();
		 pexit = ch->in_room->exit[door];
		 if (    ( pexit  == 0 )
			    ||   pexit->u1.to_room == NULL
			   ||   pexit->u1.to_room->clan
			  ||   IS_SET(pexit->exit_info, EX_CLOSED)
		   )
		 {
		   /* do nothign, room does not exists or can't move in
		    that direction */
		 }
		 else
		 {
			 new_type = pexit->u1.to_room->sector_type;
			 original_type = ch->in_room->sector_type;
			 if( /* can't from air to water */
			     (       original_type == SECT_AIR 
				&&  (    new_type == SECT_WATER_NOSWIM
				       || new_type == SECT_WATER_SWIM
				     )
			     )
			   || /* or from water to air */
			     (       new_type == SECT_AIR 
				&&  (    original_type == SECT_WATER_NOSWIM
				     || original_type == SECT_WATER_SWIM
				    )
			     )
			   )
			   {
			   /* do nothing to player, wrong sector types */
			   }
			 else
			 {
			   /* move the player to the next room */
			    move_char(ch, door, FALSE);
			 }
		
                  } /* end else, for no room */
             } /* end on the NPC check */

   if( !out_of_element(ch))
     {
	/* Gargoyles */
	if ( ch->race == race_lookup("gargoyle") )
	{
	    REMOVE_BIT(ch->vuln_flags,VULN_WEAPON);
	    SET_BIT(ch->res_flags,RES_WEAPON);
	    SET_BIT(ch->imm_flags,IMM_CHARM);
	}

      if ( ch->hit  < ch->max_hit)
    ch->hit  += hit_gain(ch);
      else
    ch->hit = ch->max_hit;
     }
    else if(ch->desc != NULL 
	    && ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
     {
   	/* Handle gargoyles first */
	if ( ch->race == race_lookup("gargoyle") )
	{
	 SET_BIT(ch->vuln_flags,VULN_WEAPON);
	 REMOVE_BIT(ch->res_flags,RES_WEAPON);
	 REMOVE_BIT(ch->imm_flags,IMM_CHARM);
	}

	/* ok, now remorts AND gargoyles */
	if ( IS_SET(ch->act,PLR_WERE) || IS_SET(ch->act,PLR_VAMP) || IS_SET(ch->act,PLR_MUMMY) )
	{

	if ( ch->hit  <= ch->max_hit)
	 {
	  if(IS_SET(ch->act,PLR_MUMMY))
	     send_to_char("Your rags are {Rb{yu{rr{Yn{ri{yn{rg{x up!\n\r",ch);
	  else
	     send_to_char("Your {rb{Rl{ro{Ro{rd{x torments you.\n\r",ch);
          if ( IS_AFFECTED(ch, AFF_SLEEP) )
          {
            REMOVE_BIT(ch->affected_by, AFF_SLEEP);
          }

	  damage( ch, ch, ch->level/3, gsn_plague,DAM_DISEASE,FALSE,TRUE);
	 }
	else
	 {
	  if(IS_SET(ch->act,PLR_MUMMY))
	     send_to_char("Your rags are {Rb{yu{rr{Yn{ri{yn{rg{x up!\n\r",ch);
	  else
	     send_to_char("Your {rb{Rl{ro{Ro{rd{x torments you.\n\r",ch);
	  ch->hit = ch->max_hit;
	 }

	} /* Brace matches IS_SET's above */
     }

      if ( ch->mana < ch->max_mana )
    ch->mana += mana_gain(ch);
      else
    ch->mana = ch->max_mana;

      if ( ch->move < ch->max_move )
    ch->move += move_gain(ch);
      else
    ch->move = ch->max_move;
  }

  else if ( ch->position == POS_INCAP && number_range(0,1) == 0)
  {
      damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE,FALSE);
  }
  else if ( ch->position == POS_MORTAL )
  {
      damage( ch, ch, 1, TYPE_UNDEFINED, DAM_NONE,FALSE,FALSE);
  }     

  if ( ch->position == POS_STUNNED )
      update_pos( ch );
      
  if ( ch->position == POS_SLEEPING )
    send_to_char ("\n\r",ch);

  if (!IS_NPC(ch) && ch->pcdata && ch->pcdata->outcT > 0)
      --ch->pcdata->outcT;

  if (!IS_NPC(ch) && ch->pcdata && ch->pcdata->ruffT > 0
	&& ch->pcdata->quit_time == 0 )
      --ch->pcdata->ruffT;

  if (!IS_NPC(ch) && ch->clan == nonclan_lookup("matook"))
  {
     if (ch->pcdata->matookT < 12001)
        ch->pcdata->matookT += 1;
     if (ch->pcdata->matookT == 12000)
	send_to_char("You have gained the wisdom to guild into Matook. Use it wisely.\n\r",ch);
  }


  if (!IS_NPC(ch) && ch->pcdata && ch->pcdata->ruffT == 0
	&& IS_SET(ch->wiznet,PLR_RUFFIAN) && ch->fighting == NULL
	&& ch->pcdata->quit_time == 0 )
  {
    REMOVE_BIT(ch->wiznet,PLR_RUFFIAN);
    send_to_char("Your ruffian reputation has been forgotten.\n\r",ch);
  }

  if ( !IS_NPC(ch) && ch->level < LEVEL_IMMORTAL )
  {
      OBJ_DATA *obj;

      if ( ( obj = get_eq_char( ch, WEAR_LIGHT ) ) != NULL
      &&   obj->item_type == ITEM_LIGHT
      &&   obj->value[2] > 0 )
      {
    if ( --obj->value[2] == 0 && ch->in_room != NULL )
    {
        --ch->in_room->light;
        act( "$p goes out.", ch, obj, NULL, TO_ROOM ,FALSE);
        act( "$p flickers and goes out.", ch, obj, NULL, TO_CHAR ,FALSE);
        extract_obj( obj );
    }
    else if ( obj->value[2] <= 5 && ch->in_room != NULL)
        act("$p flickers.",ch,obj,NULL,TO_CHAR,FALSE);
      }

      if (IS_IMMORTAL(ch))
    ch->timer = 0;

      if (ch->pcdata && ch->pcdata->start_time > 0)
      {
	  ch->pcdata->start_time -= 1;
          if(ch->pcdata->start_time == 0)
            send_to_char("{WYou may now attack other players.{x\n\r", ch);
      }
    
      if (ch->pcdata && ch->pcdata->quit_time > 0)
          --ch->pcdata->quit_time;

      if ( ++ch->timer >= 12 )
      {
    if ( ch->was_in_room == NULL && ch->in_room != NULL )
    {
        ch->was_in_room = ch->in_room;
        if ( ch->fighting != NULL )
      stop_fighting( ch, TRUE );
        act( "$n disappears into the void.",
      ch, NULL, NULL, TO_ROOM ,FALSE);
        send_to_char( "You disappear into the void.\n\r", ch );
	if (IS_SET(ch->mhs,MHS_HIGHLANDER))
	{
           remove_highlander(ch,ch);
	}
	if (IS_SET(ch->mhs,MHS_GLADIATOR))
	{
	   REMOVE_BIT(ch->mhs,MHS_GLADIATOR);
           sprintf(buf, "%s has left the arena for the void.", ch->name);
           gladiator_talk(buf); 
	   send_to_char("You drifted into the void and were removed from the Arena.\n\r",ch);
	   gladiator_left_arena(ch,FALSE);
	}

        if (ch->level > 1)
            save_char_obj( ch );
        char_from_room( ch );
	if ( is_mounted(ch) && IS_NPC(ch->riding) )
	{
	    char_from_room( ch->riding );
	    char_to_room(ch->riding,get_room_index( ROOM_VNUM_LIMBO ));
	}
	else
	if ( is_mounted(ch) && !IS_NPC(ch->riding) )
		clear_mount( ch );
        if ( ch->passenger != NULL )
	    clear_mount( ch ); 
	char_to_room( ch, get_room_index( ROOM_VNUM_LIMBO ) );
    }
      }

	gain_condition( ch, COND_DRUNK, ch->size > SIZE_LARGE ? -2 :  -1 );
	if ( !IS_SET(ch->act,PLR_VAMP) ||
	 ( IS_SET(ch->act,PLR_VAMP) && 
	   ( time_info.hour < 5 || time_info.hour > 20 ) ) )
	{
	gain_condition( ch, COND_FULL, ch->size > SIZE_MEDIUM ? -4 : -2 );
	gain_condition( ch, COND_THIRST, -1 );
	gain_condition( ch, COND_HUNGER, ch->size > SIZE_MEDIUM ? -2 : -1);
	}
  }

  for ( paf = ch->affected; paf != NULL; paf = paf_next )
  {
      paf_next  = paf->next;

	/* DOTs have their own handler */
      if ( paf->where == DAMAGE_OVER_TIME )
	continue;

      if( is_affected(ch, skill_lookup("wraithform")) )
      {
	ch_in_wraithform = TRUE;
      }

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

    /*Kill em if they're still in wraithform and haven't un wraithed*/
    if ( ch_in_wraithform == TRUE 
         && is_affected(ch, skill_lookup("wraithform")) 
         && paf->type == skill_lookup("wraithform") 
       ) 
    {
      if (!IS_IMMORTAL(ch) )
      {
        send_to_char("OH NO! You've stayed in wraithform to long!  {YACK!{x\r\n",ch);
        raw_kill(ch,ch);
      }
    }
    affect_remove( ch, paf,APPLY_BOTH );

      }
  if (   !IS_AFFECTED(ch, AFF_CHARM) 
      && IS_NPC(ch) 
      && !IS_SET(ch->act,ACT_PET) 
      && ch->master != NULL 
      && ch->fighting == NULL )
     die_follower( ch );
  }

  /*
   * Careful with the damages here,
   *   MUST NOT refer to ch after damage taken,
   *   as it may be lethal damage (on NPC).
   */

	if (is_affected(ch, gsn_asphyxiate) && ch != NULL )
	{
	    int dam;

	    if ( dice(3,10) > get_curr_stat(ch,STAT_CON) )
 	    {
		act("$n gasps in pain as $e struggles to breathe.",
			ch,NULL,NULL,TO_ROOM,FALSE);
		act("You gasp in pain as you struggle to breathe.",
			ch,NULL,NULL,TO_CHAR,FALSE);
		dam = dice(10,8);
	    }
	    else
	    {
		act("$n breathes laboriously, gasping.",
		    ch,NULL,NULL,TO_ROOM,FALSE);
		act("You breathe laboriously, gasping.",
		    ch,NULL,NULL,TO_CHAR,FALSE);
		dam = dice(5,8); 
	    }

	    damage(ch,ch,dam,gsn_asphyxiate,DAM_OTHER,FALSE,TRUE);
	}

	if (is_affected(ch, gsn_irradiate) && ch != NULL )
	{
	    int dam = 10;
	    AFFECT_DATA *paf;
	    int duration = 0, i;

	    if (ch->in_room == NULL)
		return;

 	    act("$n is wracked with painful nervous spasms.",
		ch,NULL,NULL,TO_ROOM,FALSE);
	    act("Your body is wracked by painful nervous spasms.",
		ch,NULL,NULL,TO_CHAR,FALSE);

	    for ( paf = ch->affected ;    paf != NULL ; paf = paf->next )
	       if ( paf->type == gsn_irradiate )
	       {
	   	   duration = paf->bitvector+1;
		   break;
	       }

	    for ( i = 0 ; i < duration ; i++ ) 
	       dam *= 2; /* dmage increases each round you have it */

	    damage(ch,ch,dam,gsn_irradiate,DAM_ENERGY,FALSE,TRUE);
	}


        if (is_affected(ch, gsn_plague) && ch != NULL)
        {
            //AFFECT_DATA *af;
	    ///* to go with comment out of contagion affect
            AFFECT_DATA *af, plague;
            CHAR_DATA *vch;
	    //*/
            int dam;

      if (ch->in_room == NULL)
    continue;
            
      act("$n writhes in agony as plague sores erupt from $s skin.",
    ch,NULL,NULL,TO_ROOM,FALSE);
      send_to_char("You writhe in agony from the plague.\n\r",ch);
            for ( af = ch->affected; af != NULL; af = af->next )
            {
              if (af->type == gsn_plague)
                    break;
            }
        
            if (af == NULL)
            {
              REMOVE_BIT(ch->affected_by,AFF_PLAGUE);
              continue;
            }

            if (af->level == 1)
              continue;

//  /* Removed contagion effect ****
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
              && vch->level > 10 
              && !IS_IMMORTAL(vch)
              && !IS_AFFECTED(vch,AFF_PLAGUE) && number_bits(4) == 0)
              {
                  send_to_char("You feel hot and feverish.\n\r",vch);
                  act("$n shivers and looks very ill.",vch,NULL,NULL,TO_ROOM,FALSE);
                  affect_join(vch,&plague);
              }
            }

 //****/
      dam = UMIN(ch->level,af->level/5+1);
      ch->mana -= dam;
      ch->move -= apply_chi(ch,dam);
      damage( ch, ch, dam, gsn_plague,DAM_DISEASE,FALSE,TRUE);
        }
  else if ( IS_AFFECTED(ch, AFF_POISON) && ch != NULL
       &&   !IS_AFFECTED(ch,AFF_SLOW))

  {
      AFFECT_DATA *poison;

      poison = affect_find(ch->affected,gsn_poison);

      if (poison != NULL)
      {
          act( "$n shivers and suffers.", ch, NULL, NULL, TO_ROOM ,FALSE);
          send_to_char( "You shiver and suffer.\n\r", ch );
          damage(ch,ch,poison->level/10 + 1,gsn_poison,DAM_POISON,FALSE,TRUE);
//COREY DO THE REM AFF SLEEP HERE
          if ( IS_AFFECTED(ch, AFF_SLEEP) )
          {
            affect_strip( ch, gsn_sleep );
            REMOVE_BIT(ch->affected_by, AFF_SLEEP);
          }
      }
  }

  /* Reset MOB s daze state if they aren't fighting */
  if(IS_NPC(ch) && ch->fighting == NULL)
	ch->daze = 0;

  /* Kris */
  if ((ch->life_timer > 0) && IS_NPC(ch)) {
    if (ch->life_timer == 1) { /* sorry, guy.. your dead */
      OBJ_DATA *obj,*obj_next;
      switch (ch->pIndexData->vnum) {
  case MOB_VNUM_SKEL_WAR:
     act("$n reenters the realm of the dead.",ch,NULL,NULL,TO_ROOM,FALSE);
     break;

/* Added 28-AUG-00 By Boogums for warhorses to go poof */
  case MOB_VNUM_WARHORSE:
     nuke_pets(ch);
     act("$n returns to the OK Corral.\r\n",ch,NULL,NULL,TO_ROOM,FALSE);
     break;

  case MOB_VNUM_CORPSE:
  default:
    act("$n decays into dust.",ch,NULL,NULL,TO_ROOM,FALSE);
      }
      stop_fighting( ch, TRUE );
      for ( obj = ch->carrying; obj != NULL; obj = obj_next )
      {
  obj_next = obj->next_content;
  obj_from_char( obj );
  REMOVE_BIT(obj->extra_flags,ITEM_VIS_DEATH);
  REMOVE_BIT(obj->extra_flags,ITEM_ROT_DEATH);
  if ( IS_SET( obj->extra_flags, ITEM_INVENTORY ) )
      extract_obj( obj );
  else
    obj_to_room( obj, ch->in_room );
      }

  ch->pIndexData->killed++;
  kill_table[URANGE(0, ch->level, MAX_LEVEL-1)].killed++;
  extract_char( ch, TRUE );
    } else {
      ch->life_timer--;        /* you still got some time left */
    }
  }         
    
    }
    /*
     * Autosave and autoquit.
     * Check that these chars still exist.
     */
    for ( ch = char_list; ch != NULL; ch = ch_next )
    {
      if (!IS_VALID(ch)) {
          bug("update_char: Trying to work with an invalidated character.\n",0);
          break;
      }

        ch_next = ch->next;

  if (ch->desc != NULL && ch->desc->descriptor % 15 == save_number)
      save_char_obj(ch);

        if ( ch == ch_quit )
            do_quit( ch, "" );
    }

    return;
}



/*
 * Update all room affects
 * This one's a bitch
 */
void room_update( void )
{
    AFFECT_DATA *raf, *raf_next;
    ROOM_INDEX_DATA *pRoom;
    int i;

    for ( i = 1 ; i < 40000 ; i++ )
    {
       if ( ( pRoom = get_room_index(i) ) == NULL )
	   continue;

       if ( pRoom->affected == NULL )
	   continue;

       for ( raf = pRoom->affected ; raf != NULL ; raf = raf_next )
       {
	  raf_next = raf->next;
	  if ( raf->duration > 0 )
	  {
	      raf->duration--;
	  }
	  else
    	  if ( raf->duration < 0 )
		;
	  else
	  {   
	     if ( raf_next == NULL ||
		  raf_next->type != raf->type ||
		  raf_next->duration > 0 )
	     {
		 if ( raf->type > 0 && skill_table[raf->type].msg_off )
		 {
		    send_to_room(skill_table[raf->type].msg_off,pRoom);
		 }
	     }

	     raffect_remove(pRoom,raf);
       	  }
    }
    }
    return;
} 

/*
 * Update all objs.
 * This function is performance sensitive.
 */
void obj_update( void )
{   
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf, *paf_next;
    ROOM_INDEX_DATA *to_room; 

    for ( obj = object_list; obj != NULL; obj = obj_next )
    {
  CHAR_DATA *rch;
  char *message;

  obj_next = obj->next;

  if (obj->item_type == ITEM_PORTAL 
      && obj->value[2] > 0 
      && obj->carried_by == NULL 
      && IS_SET(obj->value[2], GATE_RANDOM_ROOM))
  {
    obj->value[4] -= 1;
    if (obj->value[4] < 0 )
    {
    obj->value[4] = number_range(1,10);
    to_room = get_random_room_obj();
    obj_from_room(obj);
    obj_to_room(obj, to_room);
    }
  }
  if(obj->pIndexData->vnum == OBJ_VNUM_CORPSE_PC && obj->owner && obj->value[4] > 0)
  {
    obj->value[4]--;
    if(!obj->value[4])
    {
      CHAR_DATA *owner = check_is_online(obj->owner);
      if(owner)
        return_clan_gear(owner, obj);
    }
  }
  /* Decriment the stolen timer */
  if(obj->stolen_timer)
  { obj->stolen_timer--;
  if(obj->stolen_timer < 0)
   obj->stolen_timer = 0; }

  /* go through affects and decrement */
        for ( paf = obj->affected; paf != NULL; paf = paf_next )
        {
            paf_next    = paf->next;
            if ( paf->duration > 0 )
            {
                paf->duration--;
                if (number_range(0,4) == 0 && paf->level > 0)
                  paf->level--;  /* spell strength fades with time */
            }
            else if ( paf->duration < 0 )
                ;
            else
            {
                if ( paf_next == NULL
                ||   paf_next->type != paf->type
                ||   paf_next->duration > 0 )
                {
                    if ( paf->type > 0 && skill_table[paf->type].msg_obj )
                    {
      if (obj->carried_by != NULL)
      {
          rch = obj->carried_by;
          act(skill_table[paf->type].msg_obj,
        rch,obj,NULL,TO_CHAR,FALSE);
      }
      if (obj->in_room != NULL 
      && obj->in_room->people != NULL)
      {
          rch = obj->in_room->people;
          act(skill_table[paf->type].msg_obj,
        rch,obj,NULL,TO_ALL,FALSE);
      }
                    }
                }

                affect_remove_obj( obj, paf );
            }
        }

  /* If the Object has a Wear_timer and the Obj is being worn */
  /* If the carried_by is NULL it could be in a container, but its not being worn */
  /*
  if (obj->carried_by != NULL)
  {
     if (IS_SET(obj->extra_flags,ITEM_WEAR_TIMER)
         && obj == get_eq_char( obj->carried_by, obj->wear_loc ) )
     {
        if (obj->wear_timer <= 0 || --obj->wear_timer > 0)
           continue;
     }
  }
  else
  {
  */
     if ( obj->timer <= 0 || --obj->timer > 0 )
     {
       /*  Added by Nightdagger, 07/16/03   */
       if ( (obj->carried_by != NULL) && (obj->timer > 0 && obj->timer < 5) && (obj->pIndexData->vnum == OBJ_VNUM_DISC) )
          send_to_char("Your floating disc shimmers and begins to fade\n\r",obj->carried_by);

       /* if the object is in a room, is takeable, and is in air or water */
        if (     obj->in_room != NULL 
	     && IS_SET( obj->wear_flags , ITEM_TAKE ) 
	     && ( obj->item_type != ITEM_FURNITURE ) 
	     && (    obj->in_room->sector_type == SECT_AIR
		 || obj->in_room->sector_type == SECT_WATER_NOSWIM 
		 || obj->in_room->sector_type == SECT_WATER_SWIM          
	        )
	   )
	{ 
/* pick a direction, similar to tsumani code, can't move
   if no door in that direction, no room in that direction,
   room is a clan hall, door is closed,  go from air to
   water or water to air , I split it up in different
   if statements to simplify it for the reader */ 

         EXIT_DATA *pexit;
         int original_type, new_type;
	 int door = number_door();
	 if (    ( pexit = obj->in_room->exit[door] ) == 0
		    ||   pexit->u1.to_room == NULL
		   ||   pexit->u1.to_room->clan
		  ||   IS_SET(pexit->exit_info, EX_CLOSED)
    	   )
	  continue;


         new_type = pexit->u1.to_room->sector_type;
	 original_type = obj->in_room->sector_type;
         if( /* can't from air to water */
             (       original_type == SECT_AIR 
		&&  (    new_type == SECT_WATER_NOSWIM
		       || new_type == SECT_WATER_SWIM
		     )
	     )
	   || /* or from water to air */
	     (       new_type == SECT_AIR 
	        &&  (    original_type == SECT_WATER_NOSWIM
	       	     || original_type == SECT_WATER_SWIM
	            )
 	     )
	   )
	   continue;

/* send a message to people in the original room with the object */  
          if (( rch = obj->in_room->people ) != NULL )
	  {
	     if ( original_type == SECT_AIR )
	     {
	       act( "$p is blown away.", rch, obj, NULL, TO_ROOM ,FALSE);
	       act( "$p is blown away.", rch, obj, NULL, TO_CHAR ,FALSE);
	     }
	     else
	     {
	       act( "$p floats away.", rch, obj, NULL, TO_ROOM ,FALSE);
	       act( "$p floats away.", rch, obj, NULL, TO_CHAR ,FALSE);
             }
	  }
	   /* move the object to the next room */
         obj_from_room(obj);
	 obj_to_room(obj,pexit->u1.to_room);

/* send a message to people in the new room with the object */  
          if (( rch = obj->in_room->people ) != NULL )
	  {
	     if ( original_type == SECT_AIR )
	     {
	       act( "$p is blown in.", rch, obj, NULL, TO_ROOM ,FALSE);
	       act( "$p is blown in.", rch, obj, NULL, TO_CHAR ,FALSE);
	     }
	     else
	     {
	       act( "$p floats in.", rch, obj, NULL, TO_ROOM ,FALSE);
	       act( "$p floats in.", rch, obj, NULL, TO_CHAR ,FALSE);
             }
          }
        
      }
     continue;
     }
  /*}
  */

  switch ( obj->item_type )
  {
  default:              message = "$p crumbles into dust.";  break;
  case ITEM_FOUNTAIN:   message = "$p dries up.";         break;
  case ITEM_CORPSE_NPC: message = "$p decays into dust."; break;
  case ITEM_CORPSE_PC:  message = "$p decays into dust."; break;
  case ITEM_FOOD:       message = "$p decomposes."; break;
  case ITEM_POTION:     message = "$p has evaporated from disuse."; 
                break;
  case ITEM_PORTAL:     message = "$p fades out of existence."; break;
  case ITEM_CONTAINER: 
      if (CAN_WEAR(obj,ITEM_WEAR_FLOAT))
    if (obj->contains)
        message = 
    "$p flickers and vanishes, spilling its contents on the floor.";
    else
        message = "$p flickers and vanishes.";
      else
    message = "$p crumbles into dust.";
      break;
  }

  if ( obj->carried_by != NULL )
  {
      if (IS_NPC(obj->carried_by) 
      &&  obj->carried_by->pIndexData->pShop != NULL)
    obj->carried_by->silver += obj->cost/5;
      else
      {
        act( message, obj->carried_by, obj, NULL, TO_CHAR ,FALSE);
    if ( obj->wear_loc == WEAR_FLOAT)
        act(message,obj->carried_by,obj,NULL,TO_ROOM,FALSE);
      }
  }
  else if ( obj->in_room != NULL
  &&      ( rch = obj->in_room->people ) != NULL )
  {
      if (! (obj->in_obj && obj->in_obj->pIndexData->vnum == OBJ_VNUM_PIT
             && !CAN_WEAR(obj->in_obj,ITEM_TAKE)))
      {
        act( message, rch, obj, NULL, TO_ROOM ,FALSE);
        act( message, rch, obj, NULL, TO_CHAR ,FALSE);
      }
  }

        if ((obj->item_type == ITEM_CORPSE_PC || obj->wear_loc == WEAR_FLOAT)
  &&  obj->contains)
  {   /* save the contents */
          OBJ_DATA *t_obj, *next_obj;

      for (t_obj = obj->contains; t_obj != NULL; t_obj = next_obj)
      {
    next_obj = t_obj->next_content;
    obj_from_obj(t_obj);

    if (obj->in_obj) /* in another object */
        obj_to_obj(t_obj,obj->in_obj);

    else if (obj->carried_by)  /* carried */
        if (obj->wear_loc == WEAR_FLOAT)
      if (obj->carried_by->in_room == NULL)
          extract_obj(t_obj);
      else
          obj_to_room(t_obj,obj->carried_by->in_room);
        else
          obj_to_char(t_obj,obj->carried_by);

    else if (obj->in_room == NULL)  /* destroy it */
        extract_obj(t_obj);

    else /* to a room */
        obj_to_room(t_obj,obj->in_room);
      }
  }

  extract_obj( obj );
    }

    return;
}



/*
 * Aggress.
 *
 * for each mortal PC
 *     for each mob in room
 *         aggress on some random PC
 *
 * This function takes 25% to 35% of ALL Merc cpu time.
 * Unfortunately, checking on each PC move is too tricky,
 *   because we don't the mob to just attack the first PC
 *   who leads the party into the room.
 *
 * -- Furey
 */
void aggr_update( void )
{
    CHAR_DATA *wch;
    CHAR_DATA *wch_next;
    CHAR_DATA *ch;
    CHAR_DATA *ch_next;
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    CHAR_DATA *victim;

    for ( wch = char_list; wch != NULL; wch = wch_next )
    {
  wch_next = wch->next;
  if ( IS_NPC(wch) || !wch->pcdata
  ||   wch->level >= LEVEL_IMMORTAL
  ||   wch->in_room == NULL 
  ||   wch->in_room->area->empty)
      continue;

  for ( ch = wch->in_room->people; ch != NULL; ch = ch_next )
  {
      int count;

      ch_next = ch->next_in_room;
      if ((wch->pcdata->outcT > 0 && wch->clan == nonclan_lookup("outcast"))
      &&   IS_NPC(ch)
      && IS_AWAKE(ch)
      && !IS_AFFECTED(ch,AFF_CALM)
      && !IS_AFFECTED(ch,AFF_CHARM)
      && ch->fighting == NULL
      && can_see( ch, wch, FALSE ) 
      && !IS_IMMORTAL(wch) 
      && number_bits(1) != 0 
      && ch->pIndexData->pShop == NULL
      && !IS_SET(ch->act,ACT_TRAIN)
      && !IS_SET(ch->act,ACT_PRACTICE)
      && !IS_SET(ch->act,ACT_IS_HEALER)
      && !IS_SET(ch->act,ACT_IS_CHANGER)
      && ch->pIndexData->vnum != 3011
           )
      {
         multi_hit( ch, wch, TYPE_UNDEFINED ); 
         continue;
      } 

      if ( !IS_NPC(ch)
      ||   !IS_SET(ch->act, ACT_AGGRESSIVE)
      ||   IS_SET(ch->in_room->room_flags,ROOM_SAFE)
      ||   IS_AFFECTED(ch,AFF_CALM)
      ||   ch->fighting != NULL
      ||   IS_AFFECTED(ch, AFF_CHARM)
      ||   !IS_AWAKE(ch)
      ||   ( IS_SET(ch->act, ACT_WIMPY) && IS_AWAKE(wch) )
      ||   !can_see( ch, wch, FALSE ) 
      ||   number_bits(1) == 0)
    continue;

      /*
       * Ok we have a 'wch' player character and a 'ch' npc aggressor.
       * Now make the aggressor fight a RANDOM pc victim in the room,
       *   giving each 'vch' an equal chance of selection.
       */
      count = 0;
      victim  = NULL;
      for ( vch = wch->in_room->people; vch != NULL; vch = vch_next )
      {
    vch_next = vch->next_in_room;

    if ( !IS_NPC(vch)
    &&   vch->level < LEVEL_IMMORTAL
    &&  vch->position > POS_INCAP
    &&   ch->level >= vch->level - 5 
    &&   get_curr_stat(vch,STAT_SOC) + (vch->level - ch->level) / 3 <= number_range(16,23) 
    &&   ( !IS_SET(ch->act, ACT_WIMPY) || !IS_AWAKE(vch) )
    &&   can_see( ch, vch, FALSE ) )
    {
        if ( number_range( 0, count ) == 0 )
      victim = vch;
        count++;
    }
      }

      if ( victim == NULL )
    continue;

      if (ch->position < POS_STANDING)
	 do_stand(ch,"");

      multi_hit( ch, victim, TYPE_UNDEFINED );
  }
    }

    return;
}



/*
 * Handle all kinds of updates.
 * Called once per pulse from game loop.
 * Random times to defeat tick-timing clients and players.
 */

void update_handler( void )
{
    static  int     pulse_area;
    static  int     pulse_mobile;
    static  int     pulse_violence;
    static  int     pulse_point;
    static  int     pulse_music;
    static  int		pulse_dot;
    char buf[MAX_STRING_LENGTH];

    if ( --pulse_area     <= 0 )
    {
      CLAN_DATA *clan = clan_first;
      static int auto_save = 0;

      for(; clan; clan = clan->next)
      {
        if(clan->to_match)
        {
          MERIT_TRACKER *mtrack;
          while(clan->to_match && clan->to_match->expire <= 0)
          {
            mtrack = clan->to_match;
            clan->to_match = clan->to_match->next;
            free_merit(mtrack);
          }
          for(mtrack = clan->to_match; mtrack; mtrack = mtrack->next)
          {/* By two minute intervals */
            mtrack->expire--;
          }
        }
      }

      auto_save++;
      if(auto_save == 30)
      {/* Every hour */
        /* Save all pits */
        save_pits();
        auto_save = 0;
      }
  pulse_area  = PULSE_AREA;
  /* number_range( PULSE_AREA / 2, 3 * PULSE_AREA / 2 ); */
  area_update ( );
    }

    if ( --pulse_music    <= 0 )
    {
  pulse_music = PULSE_MUSIC;
  /* song_update(); */
    }

    if ( --pulse_mobile   <= 0 )
    {
  pulse_mobile  = PULSE_MOBILE;
  mobile_update ( );
    }

     /* Do this before combat */
    if ( --pulse_dot <= 0 )
    {
  pulse_dot	= PULSE_DOT;
  dot_update( );
     }

    if ( --pulse_violence <= 0 )
    {
  pulse_violence  = PULSE_VIOLENCE;
  violence_update ( );
  gladiator_rename_all();
    }

    if ( --pulse_point    <= 0 )
    {
  if(override > 0)
    override--;
  pulse_point     = number_range( 3 * PULSE_TICK / 4, 5 * PULSE_TICK / 4 );
  sprintf(buf,"TICK at %s{G-->{x Next tick lasts %d seconds",
	(char *) ctime( &current_time ), (pulse_point/5)+2 );
  wiznet(buf,NULL,NULL,WIZ_TICKS,0,0);
  weather_update  ( );
  char_update ( );
  obj_update  ( );
  room_update ( );
  gladiator_update ( );
  if(bounty_timer)
  {
    if(bounty_timer > 45)// Out of luck for this one
    {
      int extra = 0;
      if(bounty_type==BOUNTY_ITEM_NAME || bounty_type == BOUNTY_ITEM_DESC)
        extra = bounty_item;
      if(extra)
        sprintf(buf, "Mob %d(In %d)(Item %d) bounty failed (%d %d%s).",
          bounty_vnum, bounty_room, extra, bounty_complete, bounty_type,
          bounty_downgrade ? " down" : "");
      else
        sprintf(buf, "Mob %d(In %d) bounty failed (%d %d%s).", bounty_vnum, 
          bounty_room, bounty_complete, bounty_type,
          bounty_downgrade ? " down" : "");
      bounty_complete = UMAX(0, bounty_complete - 1);
      log_quest_detail(buf, QUEST_BOUNTY_KILL);
      select_bounty(bounty_complete);
      bounty_timer = -3;
      pnet("A shadow bounty has failed. How disappointing.",NULL,NULL,PNET_SHADEBOUNTY,0,0);
    }
    bounty_timer++;
  }
    }

    aggr_update( );
    tail_chain( );
    return;
}

void dot_update( void )
{
    CHAR_DATA *ch, *ch_next;
    AFFECT_DATA *paf, *paf_next;

    for ( ch = char_list ; ch != NULL ; ch = ch_next )
    {
	ch_next = ch->next;
	
	sector_damage(ch);

      if ( ch->affected == NULL || (IS_SET(ch->mhs,MHS_GLADIATOR) && !IS_NPC(ch)
        && (gladiator_info.time_left > 0 || gladiator_info.bet_counter > 0)))
	    continue;

	for ( paf = ch->affected ; paf != NULL ; paf = paf_next )
  	{
 	    paf_next = paf->next;

	    if ( paf->where == DAMAGE_OVER_TIME )
		{
		  dot( ch, paf );
		  if (ch->pcdata && ch->pcdata->quit_time == 1)
		  ++ch->pcdata->quit_time;
		}
	}
    }
    return;
}

void sector_damage(CHAR_DATA *ch)
{
	bool char_safe = FALSE;
	if (is_affected(ch,skill_lookup("wraithform")) )
		char_safe = TRUE;
        if (is_affected(ch,skill_lookup("water breathing")) )
                char_safe = TRUE;

	if(ch->in_room != NULL)
	{
	switch(ch->in_room->sector_type)
 	{
	default:
	break;
	case SECT_FIRE_PLANE:
	  if (number_percent() < 15 && !IS_SET(ch->imm_flags,IMM_FIRE) && !IS_IMMORTAL(ch) &&
	      !is_affected(ch,skill_lookup("wraithform")) )
	  {
		act("You are scorched by a jet of flame from the ground!", ch, NULL, NULL, TO_CHAR, FALSE);
		act("$n is scorched by a jet of flame from the ground!", ch, NULL, NULL, TO_ROOM, FALSE);
		damage(ch,ch,ch->level/2,0,DAM_FIRE,FALSE,FALSE);
	  }
          break;
        case SECT_WATER_PLANE:
	  if ( number_percent() < 40 && !IS_SET(ch->imm_flags, IMM_DROWNING) && !IS_IMMORTAL(ch) && char_safe == FALSE ) 
/*       ( (!is_affected(ch, skill_lookup("water breathing"))) || (!is_affected(ch,skill_lookup("wraithform"))) )
*/
          {
		act("Your lungs scream with the need for air!", ch, NULL, NULL, TO_CHAR, FALSE);
		act("$n gasps for air and gulps down some water!", ch, NULL, NULL, TO_ROOM, FALSE);
		damage(ch,ch,ch->level*3/2,0,DAM_DROWNING,FALSE,FALSE);
	  }
	  break;
	}
	}
	return;
}	
