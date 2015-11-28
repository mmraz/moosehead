/****************************************************************************
 a  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
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

static char rcsid[] = "$Id: magic.c,v 1.484 2005/04/18 02:58:27 rusty Exp $";
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
#include "magic.h"
#include "recycle.h"
#include "tables.h"
#include "gladiator.h"

/* command procedures needed */
DECLARE_DO_FUN(do_look    );
DECLARE_DO_FUN(do_order    );

extern int bounty_item;
extern int bounty_type;
extern int bounty_timer;

/*
 * Local functions.
 */
bool cast_spell args((CHAR_DATA *ch, char *argument, bool fChant, bool fFocus));
void  say_spell args( ( CHAR_DATA *ch, int sn ) );
void write_spell( CHAR_DATA *ch, int sn );

/* imported functions */
bool    remove_obj      args( ( CHAR_DATA *ch, int iWear, bool fReplace ) );
void  wear_obj  args( ( CHAR_DATA *ch, OBJ_DATA *obj, bool fReplace ) );
void  set_fighting  args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
int	nonclan_lookup	args( (const char *name) );

bool check_annointment(CHAR_DATA *victim, CHAR_DATA *ch)
{
  if(!IS_NPC(victim) && IS_SET(victim->affected_by_ext, AFF_EXT_ANNOINTMENT) &&
    victim != ch && number_percent() > 50 && !is_same_clan(victim, ch))
    {
      AFFECT_DATA *paf;
      char buf[256];
      sprintf(buf, "%s rebukes your attacker.\n\r", deity_table[victim->pcdata->deity].pname);
      send_to_char(buf, victim);
      sprintf(buf, "%s rebukes you for harming %s.\n\r",
        deity_table[victim->pcdata->deity].pname, victim->name);
      send_to_char(buf, ch);
      for(paf = victim->affected; paf; paf = paf->next)
      {
        if(paf->where == TO_AFFECTS_EXT && paf->bitvector == AFF_EXT_ANNOINTMENT)
        {/* Stronger levels of annointment last longer */
          paf->modifier--;
          break;
        } 
      }
      if(!paf)
      {/* Bug, clean it up. */
        send_to_char("Your holy annointment fades.\n\r", victim);
        REMOVE_BIT(victim->affected_by_ext, AFF_EXT_ANNOINTMENT);
      }
      else if(paf->modifier <= 0)
      {/* Fired off as many times as it's allowed to */
        affect_strip(victim, gsn_annointment);
      }
      return TRUE;
    }
  return FALSE;
}

void apply_mala_damage(CHAR_DATA *ch, CHAR_DATA *victim, int amount)
{/* Apply damage of amount % of victim's max health */
  damage_add(ch, victim, (victim->max_hit * amount) / 100, 5);
}

/*
 * Process a damage over time spell
 */
void heal_dot( CHAR_DATA *victim, CHAR_DATA *ch, AFFECT_DATA *paf )
{
    int heal;

    heal = number_range( abs(paf->modifier), abs(paf->location) );
  
    if ( ch != NULL &&  ch->in_room != victim->in_room )
	heal /= 2;

    if ( victim->fighting != NULL )
	heal /= 2;

    victim->hit = UMIN(victim->hit+heal,victim->max_hit);

     paf->level--;
     if ( --paf->duration < 0 )
     {
        send_to_char(skill_table[paf->type].msg_off,victim);
        send_to_char("\n\r",victim);
        affect_remove(victim,paf,APPLY_BOTH);
     }

    return;
}

void dot( CHAR_DATA *ch, AFFECT_DATA *paf )
{
    int dam;
    CHAR_DATA *caster;
   
    if ( ch == NULL || paf == NULL )
	return;

    caster = NULL;

    if ( paf->location < 0 || paf->modifier < 0 )
    {
	heal_dot(ch,caster,paf);
	return;
    }

    dam = number_range(paf->modifier,paf->location);

    if ( paf->caster_id > 0 )
        caster = get_char_by_id( paf->caster_id );

    if ( caster == NULL )
    {
	caster = ch;
	dam /= 2;
    }
    else
    {
	/* if we know the caster, see if they're in the room with ch */

	/* Damage cut in half if caster isn't in same room */
	if( caster->in_room != ch->in_room )
	    dam /= 2;
    }

    /* The fields in AFFECT_DATA for DOT spells are different.
     * 
     * af.where = DAMAGE_OVER_TIME
     * af.duration = #DOT cycles ( 10 second cycles )
     * af.location = Minimum damage
     * af.modifier = Maximum damage
     * af.bitvector = damage type
     */


     damage(caster,ch,dam,TYPE_DOT,paf->bitvector,FALSE,FALSE);

     if ( ch == NULL )
	return;
 
     /** Handle wear-off **/ 

     paf->level--;
     if ( --paf->duration < 0 )
     {
	send_to_char(skill_table[paf->type].msg_off,ch);
	send_to_char("\n\r",ch);
	affect_remove(ch,paf,APPLY_BOTH);
     }
     return;
}

/*
 * Lookup a skill by name.
 */
int skill_lookup( const char *name )
{
    int sn;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
  if ( skill_table[sn].name == NULL )
      break;
  if ( LOWER(name[0]) == LOWER(skill_table[sn].name[0])
  &&   !str_prefix( name, skill_table[sn].name ) )
      return sn;
    }

    return -1;
}

int find_spell( CHAR_DATA *ch, const char *name )
{
    /* finds a spell the character can cast if possible */
    int sn, found = -1;

    if (IS_NPC(ch))
  return skill_lookup(name);

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
  if (skill_table[sn].name == NULL)
      break;
  if (LOWER(name[0]) == LOWER(skill_table[sn].name[0])
  &&  !str_prefix(name,skill_table[sn].name))
  {
      if ( found == -1)
    found = sn;
      if (ch->level >= skill_level(ch,sn)
      &&  ch->pcdata->learned[sn] > 0)
        return sn;
  }
    }
    return found;
}



/*
 * Lookup a skill by slot number.
 * Used for object loading.
 */
int slot_lookup( int slot )
{
    extern bool fBootDb;
    int sn;

    if ( slot <= 0 )
  return -1;

    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
  if ( slot == skill_table[sn].slot )
      return sn;
    }

    if ( fBootDb )
    {
  bug( "Slot_lookup: bad slot %d.", slot );
  abort( );
    }

    return -1;
}



/*
 * Utter mystical words for an sn.
 */
void say_spell( CHAR_DATA *ch, int sn )
{
    char buf  [MAX_STRING_LENGTH];
    char buf2 [MAX_STRING_LENGTH];
    CHAR_DATA *rch;
    char *pName;
    int iSyl;
    int length;

    struct syl_type
    {
  char *  old;
  char *  new;
    };

    static const struct syl_type syl_table[] =
    {
  { " ",    " "   },
  { "ar",   "abra"    },
  { "au",   "kada"    },
  { "bless",  "fido"    },
  { "blind",  "nose"    },
  { "bur",  "mosa"    },
  { "cu",   "judi"    },
  { "de",   "oculo"   },
  { "en",   "unso"    },
  { "light",  "dies"    },
  { "lo",   "hi"    },
  { "mor",  "zak"   },
  { "move", "sido"    },
  { "ness", "lacri"   },
  { "ning", "illa"    },
  { "per",  "duda"    },
  { "ra",   "gru"   },
  { "fresh",  "ima"   },
  { "re",   "candus"  },
  { "son",  "sabru"   },
  { "tect", "infra"   },
  { "tri",  "cula"    },
  { "ven",  "nofo"    },
  { "wall",  "denca"    },
  { "a", "a" }, { "b", "b" }, { "c", "q" }, { "d", "e" },
  { "e", "z" }, { "f", "y" }, { "g", "o" }, { "h", "p" },
  { "i", "u" }, { "j", "y" }, { "k", "t" }, { "l", "r" },
  { "m", "w" }, { "n", "i" }, { "o", "a" }, { "p", "s" },
  { "q", "d" }, { "r", "f" }, { "s", "g" }, { "t", "h" },
  { "u", "j" }, { "v", "z" }, { "w", "x" }, { "x", "n" },
  { "y", "l" }, { "z", "k" },
  { "", "" }
    };

    buf[0]  = '\0';
    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
  for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
  {
      if ( !str_prefix( syl_table[iSyl].old, pName ) )
      {
    strcat( buf, syl_table[iSyl].new );
    break;
      }
  }

  if ( length == 0 )
      length = 1;
    }

    sprintf( buf2, "$n utters the words, '%s'.", buf );
    sprintf( buf,  "$n utters the words, '%s'.", skill_table[sn].name );

    for ( rch = ch->in_room->people; rch; rch = rch->next_in_room )
    {
  if ( rch == ch )
		continue;

      if ( get_skill(rch,gsn_spellcraft) >= 65 &&
	   rch->position > POS_SLEEPING)
      {
	check_improve(rch,gsn_spellcraft,TRUE,10);
	act( buf, ch, NULL, rch, TO_VICT,FALSE);
      }
      else
      act( ch->class==rch->class ? buf : buf2, ch, NULL, rch, TO_VICT ,FALSE);
    }

    return;
}



/*
 * Compute a saving throw.
 * Negative apply's make saving throw better.
 */
bool saves_spell( int level, CHAR_DATA *victim, int dam_type )
{
    int save,saving_throw;
    OBJ_DATA *segment;

    if ( IS_NPC(victim) && victim->pIndexData->pShop != NULL )
    return TRUE;

    saving_throw = victim->saving_throw;

    if(dam_type == DAM_FIRE && ((segment = get_eq_char(victim, WEAR_HOLD)) != NULL && segment->pIndexData->vnum == VNUM_FIRE_SEGMENT))
    {
    return TRUE;
    }

    if(dam_type == DAM_DROWNING && ((segment = get_eq_char(victim, WEAR_HOLD)) != NULL && segment->pIndexData->vnum == VNUM_WATER_SEGMENT))
    {
    return TRUE;
    }

    if (saving_throw < -8 && !is_affected(victim,gsn_magic_resistance) )
    {
    saving_throw = ( saving_throw + 8 ) / 2 - 8;
    }

    if (saving_throw < -12 && !is_affected(victim,gsn_magic_resistance) )
    {
    saving_throw = ( saving_throw + 12 ) / 2 - 12;
    }

    if (saving_throw < -16 && !is_affected(victim,gsn_magic_resistance))
    {
    saving_throw = ( saving_throw + 16 ) / 2 - 16;
    }
    
    /* curve only once for people with res magic but curve is /3 */
    if (saving_throw < -24 && is_affected(victim,gsn_magic_resistance))
    {
    saving_throw = ( saving_throw + 24 ) / 3 - 24;
    }
    if( victim->race == race_lookup("elf") ) saving_throw -= 5;

    save = 80 + ( level / 6 ) +  saving_throw  ;

    save -= ( get_curr_stat(victim,STAT_INT)  +
	      get_curr_stat(victim,STAT_WIS) ) ;

    save -= ( ( victim->level - level ) * 5 ) ;

    if (IS_AFFECTED(victim,AFF_CURSE))
	 save += 5;


    if (victim->position < POS_FIGHTING)
	save += 5;

  
    if( number_percent() < get_skill(victim,gsn_weave_resistance) )
    {
        save -= ( get_skill(victim,gsn_weave_resistance) / 5 );
        check_improve(victim,gsn_weave_resistance,TRUE,1);
    }
    
    if (IS_AFFECTED(victim,AFF_BERSERK))
    {
	save -=( get_skill(victim, gsn_berserk) / 15) ;
    }

    switch(check_immune(victim,dam_type))
    {
  case IS_IMMUNE:   return TRUE;
  case IS_RESISTANT:  save -= 25;  break;
  case IS_VULNERABLE: save += 25;  break;
    }

    if ( get_skill(victim,gsn_spellcraft) >= 90 ) 
	save -= 10;

    if (!IS_NPC(victim) && class_table[victim->pcdata->old_class].fMana  == 1)
        save -= 10;
    if (!IS_NPC(victim) && class_table[victim->pcdata->old_class].fMana == 2 )
        save -= 14;

    if ( !IS_NPC(victim) && group_has_cavalier( victim ) &&
        number_percent() <  (victim->pcdata->sac / 8 )  )
    {
	save -=15;
    }


    save = URANGE( 5, save, 95 );
    return number_percent( ) > save;
}

/* RT save for dispels */

bool saves_dispel( int dis_level, int spell_level, int duration)
{
    int save;
    
    if (duration == -1)
      spell_level += 5;  
      /* very hard to dispel permanent effects */

    save = 50 + (spell_level - dis_level) * 5;
    save = URANGE( 5, save, 95 );
    return number_percent( ) < save;
}

/* co-routine for dispel magic and cancellation */

bool check_dispel( int dis_level, CHAR_DATA *victim, int sn)
{
    AFFECT_DATA *af;

    if (is_affected(victim, sn))
    {
        for ( af = victim->affected; af != NULL; af = af->next )
        {
            if ( af->type == sn )
            {
                if (!saves_dispel(dis_level,af->level,af->duration))
                {
                  affect_strip(victim,sn);
              	  if ( skill_table[sn].msg_off )
              	  {
		    send_to_char( skill_table[sn].msg_off, victim );
		    send_to_char( "\n\r", victim );
              	  }
        	  return TRUE;
    	    	}
    	    	else
    		  af->level--;
            }
        }
    }
    return FALSE;
}

bool dragon_breath( CHAR_DATA *ch, int sn )
{
   if ( ch->race != race_lookup("dragon") )
	return FALSE;

   if (
	 sn == skill_lookup("acid breath") ||
	 sn == skill_lookup("gas breath") ||
	 sn == skill_lookup("frost breath") ||
	 sn == skill_lookup("lightning breath") ||
	 sn == skill_lookup("fire breath") ) 
	return TRUE;

   return FALSE;
}

/* for finding mana costs -- temporary version */
int mana_cost (CHAR_DATA *ch, int min_mana, int level, int sn)
{
    int mana;

    if (ch->level + 2 == level)
  	return 1000;
    else
	mana = (100 / ( 2 + ch->level - level ) );

    if ( HAS_KIT(ch,"nethermancer") &&
	 ( sn == skill_lookup("gate") ||
	   sn == skill_lookup("summon") ||
	   sn == skill_lookup("portal") ||
	   sn == skill_lookup("nexus") ) )
	return ( min_mana / 2 );

    if ( HAS_KIT(ch, "enchanter") &&
       ( sn == skill_lookup("enchant weapon") ||
	 sn == skill_lookup("enchant armor") ) )
          return (min_mana * 8 /10 );


    if ( dragon_breath( ch, sn ) )
	return ( min_mana / 2 );
   
    return UMAX(min_mana,mana);
}


int compute_casting_level( CHAR_DATA *ch, int sn )
{
   int level;
   OBJ_DATA *segment = get_eq_char(ch, WEAR_HOLD) ;
   if ( IS_NPC(ch) )
	return ch->level;

   level = ch->level;

    if ( class_table[ch->pcdata->old_class].fMana == 2 )
    {
      level += 2;
    }

    if ( class_table[ch->pcdata->old_class].fMana == 1 )
    {
        level += 1;
    }
    switch( class_table[ch->class].fMana )
    {// Hybrids cast at full level for now
    case 0: level = ( 8 * level / 10 ); break;  /* 60% casting level */
    case 1:if(class_table[ch->pcdata->old_class].fMana != 2)
	   {
		 level = ( 9 * level / 10 ); break; /* 90% casting level */
	   }
    case 2: level = ch->level;break;
    }
    if ( ch->class == class_lookup("druid") )
    {
       level += ( get_curr_stat(ch,STAT_INT) == 25 );
       level += ( get_curr_stat(ch,STAT_WIS) == 25 );
       level += ( ch->level >= 25 );
       level += ( ch->level >= 50 );
       level += ( ch->mana / 400 );
    }

    if ( is_affected(ch,gsn_imbue) )
    {
	if ( level <  ch->level )
		level += number_range(2,3);
	else
		level++;
    }

    if ( ch->race == race_lookup("elf") )
	level++;

    if ( ch->race == race_lookup("faerie"))
	level += 1;

    if ( ch->class == class_lookup("elementalist") )
	level += ( ( ch->max_mana > 500 ) + ( ch->max_mana >= 1000 ) );

    if ( get_skill(ch,gsn_spellcraft) >= 100 )
	level++;

    if ( ch->race == race_lookup("smurf"))
       if (smurf_group_count(ch) > 1)
          level += smurf_group_count(ch);

    /* Casting Level Curve */
    if ( level > (ch->level + 2 ) )
       level = ( level - (ch->level +2 ) ) / 2 + (ch->level +2 );

    if ( level > (ch->level +5 ) )
	level = ( level - (ch->level +5 ) ) /2 + (ch->level +5 );

    if ( is_affected(ch,gsn_enervation) )
	level -= number_range( 2, 4 );

   /*below starts the check to see if they're a necromancer and
     are casting necromantic spells - Boogums */
    if ( ch->kit == kit_lookup("necromancer") &&
	 (  
	     ( sn == skill_lookup( "animate dead" ) )  ||
	     ( sn == skill_lookup( "draw life" ) )     ||
	     ( sn == skill_lookup( "enervation" ) )    ||
	     ( sn == skill_lookup( "summon dead" ) )   ||
	     ( sn == skill_lookup( "turn undead" ) )   ||
	     ( sn == skill_lookup( "wound transfer" ) )||
	     ( sn == skill_lookup( "cryogenesis" ) )   ||
	     ( sn == skill_lookup( "make bag" ) )   ||
	     ( sn == skill_lookup( "withstand death" ) ) 
	 ) 
       )
      {
        if ( ch->alignment >= 500)
          level -= 20;
	if ( 249 < ch->alignment && ch->alignment < 500 )
	  level -= 15;
	if ( -1 < ch->alignment  && ch->alignment < 250 )
	  level -= 10;
        if ( -251 < ch->alignment && ch->alignment < 0 )
	  level -= 5;
	if ( -751 < ch->alignment  && ch->alignment < -500 )
          level += 1;
	if ( -950 < ch->alignment && ch->alignment < -750)
          level += 2;
	if ( -1001 < ch->alignment  && ch->alignment < -950 )
          level += 3;
      } /*end the necromancy check */
     
    /*Starte check for holding various segments of the elements */
    if( segment != NULL && segment->pIndexData->vnum == VNUM_FIRE_SEGMENT &&
	(
		sn == skill_lookup("incinerate") ||
		sn == skill_lookup("fireball") ||
		sn == skill_lookup("flamesword") ||
		sn == skill_lookup("flame shield") ||
		sn == skill_lookup("burning hands") ||
		sn == skill_lookup("fireproof") ||
		sn == skill_lookup("heat metal") ||
		sn == skill_lookup("flameseek")||
		sn == skill_lookup("immolation")||
		sn == skill_lookup("flamestrike") 
	)) 
        {  
           level += 3;
     /* Ok, 50 percent chance of segment going byebye- ND */
           if (number_percent()<50)
           {
              send_to_char("The last of its energy expended, your fire segment flares and vanishes.\n\r",ch);
              extract_obj(segment);
           }  
         } 
   if(!IS_NPC(ch) && ch->pcdata->deity_trial_timer > 0 && ch->pcdata->deity_trial == 6)
	level /= 2;// Damage and casting levels are greatly reduced

   return level;

}     

/*
 * The kludgy global is for spells who want more stuff from command line.
 */
char *target_name;

void do_cast( CHAR_DATA *ch, char *argument )
{
   if ( is_affected(ch, gsn_cone_of_silence ) )
   {
    send_to_char("You are wrapped in a cone of silence.\n\r",ch);
    return;
    }

    if ( HAS_KIT(ch,"bishop") )
    {
	send_to_char("You must chant for your powers.\n\r",ch);
	return;
    }
    /*added for wraith form check 25NOV00 - Boogums*/
    if( is_affected(ch,skill_lookup("wraithform")) )
    {
      send_to_char("You cannot cast while affected by wraith form.\n\r",ch);
      return;
    } 
   
     cast_spell( ch, argument, FALSE, FALSE );
    return;
}

void do_chant( CHAR_DATA *ch, char *argument )
{
   if ( is_affected(ch, gsn_cone_of_silence ) )
   {
    send_to_char("You are wrapped in a cone of silence.\n\r",ch);
    return;
    }

    if ( number_percent() < get_skill(ch,gsn_holy_chant) )
    {
 	if ( cast_spell( ch, argument, TRUE, FALSE ) )
    		check_improve(ch,gsn_holy_chant,TRUE,4);
    }
    else
    {
	send_to_char("Your chant failed.\n\r",ch);
	check_improve(ch,gsn_holy_chant,FALSE,8);
	return;
    }
}

void do_focus( CHAR_DATA *ch, char *argument )
{

    if ( !HAS_KIT(ch,"wu jen") )
    {
      send_to_char("You are not Wu Jen.\n\r",ch);
      return;
    }

    if ( number_percent() < get_skill(ch,gsn_focus) )
    {
 	if ( cast_spell( ch, argument, FALSE, TRUE ) )
    		check_improve(ch,gsn_focus,TRUE,4);
    }
    else
    {
	send_to_char("You failed to focus.\n\r",ch);
	check_improve(ch,gsn_focus,FALSE,8);
	WAIT_STATE( ch, 24 );
	return;
    }
}

void do_quicken (CHAR_DATA *ch, char *argument)
{

   char arg1[MAX_INPUT_LENGTH];
   char arg2[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   CHAR_DATA *victim;
   void *vo;
   int sn;
   int level;

   if(!IS_SET(ch->mhs,MHS_HIGHLANDER))
   {
      send_to_char("Only Highlanders can Quicken.\n\r",ch);
      return;
   }

   if(ch->fighting != NULL)
   {
      send_to_char("You can not invoke the Quickening during combat.\n\r",ch);
      return;
   }

   argument = one_argument(argument, arg1);
   argument = one_argument(argument, arg2);

   if ( arg1[0] == '\0' )
   {
      send_to_char( "What do wish to invoke your Quickening into?\n\r", ch );
      return;
   }
   if((sn = skill_lookup(arg1)) == -1)
   {
      send_to_char("No such skill or spell exists.\n\r",ch);
      return;
   }

   if (!strcmp(arg2,"self") || arg2[0] == '\0')
      victim = ch;
   else
   {
      if ( ( victim = get_char_world( ch, arg2 ) ) == NULL )
      {
         send_to_char( "They aren't here.\n\r", ch );
         return;
      }
   }

   sprintf(buf,"%s is quickening %s at %s",ch->name,skill_table[sn].name,victim->name);
   log_string(buf);

   if (sn != skill_lookup("haste") &&
       sn != skill_lookup("giant strength") &&
       sn != skill_lookup("farsee"))
   {
      send_to_char("That is not a valid Quickening power.\n\r",ch);
      return;
   }

   if (victim != ch && sn != skill_lookup("farsee"))  
   {
      send_to_char("The Quickening powers are for your use only.\n\r",ch);
      return;
   }

   level = 60;

   vo = (void *) ch;
   if (sn == skill_lookup("farsee"))
      vo = (void *) victim;

   (*skill_table[sn].spell_fun) (sn, level, ch, vo, TARGET_CHAR );
   return;
}


bool cast_spell( CHAR_DATA *ch, char *argument, bool fChant, bool fFocus )
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    EXIT_DATA *exit;
    void *vo;
    int mana;
    int spell_skill;
    int sn;
    int target;
    int level;
    bool iWyrm = FALSE;
    bool fConcentrate = FALSE;
    char buf  [MAX_STRING_LENGTH];

    /*
     * Switched NPC's can cast spells, but others can't.
     */
    if ( IS_NPC(ch) && ch->desc == NULL)
  return FALSE;

    target_name = one_argument( argument, arg1 );
    one_argument( target_name, arg2 );

    if ( arg1[0] == '\0' )
    {
  send_to_char( "Cast which what where?\n\r", ch );
  return FALSE;
    }

    if ( ( sn = find_spell( ch,arg1 ) ) < 0
    || ( !IS_NPC(ch) && (ch->level < skill_level(ch,sn)
    ||       ch->pcdata->learned[sn] == 0)))
    {
  send_to_char( "You don't know any spells of that name.\n\r", ch );
  return FALSE;
    }
    if (skill_table[sn].spell_fun == spell_null)
    {
      send_to_char( "That's not a spell!\n\r", ch );
      return FALSE;
    }
    if ( sn == skill_lookup("scion storm") )
    {
      send_to_char("You cannot cast that spell.\n\r",ch);
      return FALSE;
    }
    if ( ch->position < skill_table[sn].minimum_position && !fChant )
    {
  send_to_char( "You can't concentrate enough.\n\r", ch );
  return FALSE;
    }

    if( !IS_NPC(ch) && IS_SET(ch->affected_by, AFF_HIDE))
        REMOVE_BIT( ch->affected_by, AFF_HIDE );

    mana = mana_cost(ch,skill_table[sn].min_mana,
		     skill_level(ch,sn),sn);

/**
    if (ch->level + 2 == skill_table[sn].skill_level[ch->class])
  mana = 50;
    else
      mana = UMAX(
      skill_table[sn].min_mana,
      100 / ( 2 + ch->level - skill_table[sn].skill_level[ch->class] ) );
**/

    if ( fChant )
    {
	int reduction;

	reduction = 100;
	reduction -= ( get_skill(ch,gsn_holy_chant) / 5 );
	mana = reduction * mana / 100;
    }

    /*
     * Locate targets.
     */
    victim  = NULL;
    obj   = NULL;
    vo    = NULL;
    exit = NULL;
    target  = TARGET_NONE;
      
    switch ( skill_table[sn].target )
    {
    default:
  bug( "Do_cast: bad target for sn %d.", sn );
  return FALSE;

    case TAR_IGNORE:
  break;
    case TAR_CHAR_OFFENSIVE:
  if ( arg2[0] == '\0' )
  {
      if ( ( victim = ch->fighting ) == NULL )
      {
    send_to_char( "Cast the spell on whom?\n\r", ch );
    return FALSE;
      }
  }
  else
  {
      if ( ( victim = get_char_room( ch, target_name ) ) == NULL )
      {
    send_to_char( "They aren't here.\n\r", ch );
    return FALSE;
      }
  }

        if ( ch == victim && IS_SET(ch->mhs,MHS_GLADIATOR))
        {
            send_to_char( "Suicide is not in the Gladiator Code. Kill someone besides yourself!\n\r", ch );
            return FALSE;
        }
  if (ch == victim && IS_SET(ch->in_room->room_flags,ROOM_NOCOMBAT))
  {
     send_to_char("No Combat in this room.\n\r",ch);
     return FALSE;
  }

  if(victim->fighting != ch && victim->fighting != NULL && 
     IS_SET(ch->mhs,MHS_HIGHLANDER) 
     && IS_SET(victim->mhs,MHS_HIGHLANDER))
  {
     send_to_char("Honorable Combat is one on one.\n\r",ch);
     return FALSE;
  }
  
  /*Wraithform check-Boogums*/
  if (is_affected(victim,skill_lookup("wraithform")))
  {
    send_to_char("They are made of mist.\r\n",ch);
    return FALSE;
  }


  if( victim->fighting != NULL &&
  !is_same_group(ch,victim->fighting)  && ch != victim 
  && IS_NPC(victim) )
    {
        send_to_char("Kill stealing is not permitted.\n\r",ch);
        return FALSE;
    }

  if ( !IS_NPC(ch) )
  {

            if (is_safe(ch,victim) && victim != ch)
      {
    send_to_char("Not on that target.\n\r",ch);
    return FALSE; 
      }
  check_killer(ch,victim);
  }

        if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
  {
      send_to_char( "You can't do that on your own follower.\n\r",
    ch );
      return FALSE;
  }
  /* Gladiator Spectator Channel */
  if (IS_SET(ch->mhs,MHS_GLADIATOR))
  {
     if (number_percent() < 50)
     {
        sprintf(buf,"%s casts %s at %s.\n\r",ch->name,skill_table[sn].name,victim->name);
        gladiator_talk(buf);
     }
  }

  vo = (void *) victim;
  target = TARGET_CHAR;
  if(HAS_KIT(ch,"wyrmslayer") && victim->race == race_lookup("dragon"))
	iWyrm = TRUE;
  break;

    case TAR_CHAR_DEFENSIVE:
  if ( arg2[0] == '\0' )
  {
      victim = ch;
  }
  else
  {
      /*another wraithform check -Boogums
      if ( is_affected(victim,skill_lookup("wraithform")) )
      {
        send_to_char("Your spell has no affect on them.\r\n",ch);
	return FALSE;
      }
*/
      if ( ( victim = get_char_room( ch, target_name ) ) == NULL )
      {
    send_to_char( "They aren't here.\n\r", ch );
    return FALSE;
      }
    if(is_clan(ch) && is_clan(victim) && ch->pcdata->start_time > 0)
    {
      if(victim->pcdata->quit_time)
      {
        send_to_char("You can't pk yet and they have fought another player too recently.\n\r", ch);
        return FALSE;
      }
    }
  }

  vo = (void *) victim;
  target = TARGET_CHAR;
  break;

    case TAR_CHAR_SELF:
  if ( arg2[0] != '\0' && !is_name( target_name, ch->name ) )
  {
      send_to_char( "You cannot cast this spell on another.\n\r", ch );
      return FALSE;
  }

  vo = (void *) ch;
  target = TARGET_CHAR;
  break;

    case TAR_OBJ_INV:
  if ( arg2[0] == '\0' )
  {
      send_to_char( "What should the spell be cast upon?\n\r", ch );
      return FALSE;
  }

  if ( ( obj = get_obj_carry( ch, arg2 ) ) == NULL )
  {
      send_to_char( "You are not carrying that.\n\r", ch );
      return FALSE;
  }

  vo = (void *) obj;
  target = TARGET_OBJ;
  break;

    case TAR_OBJ_CHAR_OFF:
  if (arg2[0] == '\0')
  {
      if ((victim = ch->fighting) == NULL)
      {
    send_to_char("Cast the spell on whom or what?\n\r",ch);
    return FALSE;
      }
  
      target = TARGET_CHAR;
  }
  else if ((victim = get_char_room(ch,target_name)) != NULL)
  {
      target = TARGET_CHAR;
  }

  if (target == TARGET_CHAR) /* check the sanity of the attack */
  {
      if(is_safe_spell(ch,victim,FALSE, sn) && victim != ch)
      {
    send_to_char("Not on that target.\n\r",ch);
    return FALSE;
      }

            if ( IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim )
            {
                send_to_char( "You can't do that on your own follower.\n\r",
                    ch );
                return FALSE;
            }

      if (!IS_NPC(ch))
    check_killer(ch,victim);

      vo = (void *) victim;
  }
  else if ((obj = get_obj_here(ch,target_name)) != NULL)
  {
      vo = (void *) obj;
      target = TARGET_OBJ;
  }
  else
  {
      send_to_char("You don't see that here.\n\r",ch);
      return FALSE;
  }
  break; 

	case TAR_OBJ_CHAR_DEF:
        if (arg2[0] == '\0')
        {
            vo = (void *) ch;
            target = TARGET_CHAR;                                                 
        }
        else if ((victim = get_char_room(ch,target_name)) != NULL)
        {
            vo = (void *) victim;
            target = TARGET_CHAR;
  }
  else if ((obj = get_obj_carry(ch,target_name)) != NULL)
  {
      vo = (void *) obj;
      target = TARGET_OBJ;
  }
  else
  {
      send_to_char("You don't see that here.\n\r",ch);
      return FALSE;
  }
  break;
    }
   
   /* Dragon breath */
    if ( dragon_breath( ch, sn ) )
    {
       int moves;

       moves = mana_cost(ch,skill_table[sn].min_mana,
			    skill_level(ch,sn),sn) / 2;

       if ( moves > ch->move )
       {
	    send_to_char("You are too exhausted.\n\r",ch);
	    return FALSE;
	}

	ch->move -= moves;
    }
    
    if ( !IS_NPC(ch) && ch->mana < mana )
    {

       if ( number_percent() < get_skill(ch,gsn_nethermancy) )
       {
	   if ( ch->hit < ( mana * 2 ) )
	   {
		send_to_char("Your nethermancy has failed you.\n\r",ch);
			return FALSE;
	   }
	   check_improve(ch,gsn_nethermancy,TRUE,5);
	   ch->hit -= ( mana * 2 );
	   ch->mana += mana;
	   send_to_char("Your soul screams as you transfer your life force.\n\r",ch);
	}
	else
       if ( number_percent() < get_skill(ch,gsn_communion) )
       {
 	   if ( ch->pcdata->sac < mana * 2)
	   {
	      send_to_char("Your communion fails.\n\r",ch); 
	      return FALSE;
	   }

	   check_improve(ch,gsn_communion,TRUE,3);
	   ch->pcdata->sac -= mana * 2;
	   ch->mana += mana;
	   send_to_char("Your deity grants your communion.\n\r",ch);
       }
       else
       {
       check_improve(ch,gsn_nethermancy,FALSE,12);
       check_improve(ch,gsn_communion,FALSE,6);
       send_to_char("You don't have enough mana.\n\r",ch);
       return FALSE;
       }

    }
      
    if ( str_cmp( skill_table[sn].name, "ventriloquate" ) && 
	 str_cmp( skill_table[sn].name, "mirror image" ) )
  say_spell( ch, sn );
   
   /* Immortals do not lag with casting spells */
if (!IS_IMMORTAL(ch))
{
    int 	wait_beats;

    wait_beats = skill_table[sn].beats;

    if ( ch->race == race_lookup("faerie") && IS_AFFECTED(ch,AFF_SLOW) )
	wait_beats *= 2;

    if ( skill_table[sn].target != TAR_CHAR_OFFENSIVE && skill_table[sn].beats >= 24 
	&& ( number_percent() < get_skill(ch,gsn_spellcraft) ) )
    {
	check_improve(ch,gsn_spellcraft,TRUE,10);
	wait_beats /= 2;
    }
    else
	check_improve(ch,gsn_spellcraft,FALSE,15);

    if ( is_affected(ch,gsn_arcantic_alacrity) )
	wait_beats /= 2;

    if ( is_affected(ch,gsn_arcantic_lethargy) )
	wait_beats *= 2;

//COREY WAIT_STATE IS HERE
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:
                wait_beats *= 1;
                break;
        case 1:
                wait_beats *= .75;
                break;
        case 2:
                wait_beats *= .90;
                break;
        default:
                wait_beats *= 1;
                break;
        }
    }

    WAIT_STATE(ch,wait_beats);
}

    spell_skill = get_skill(ch,sn);
    if ( is_affected(ch,skill_lookup("shield bash")) )
	spell_skill = spell_skill * 3 / 4;

    if ( is_mounted(ch) )
	spell_skill -= ( spell_skill / 10 );

    if ( number_percent( ) > spell_skill ) 
    {

       /*  This code killed by Nightdagger
       if (ch->clan == clan_lookup("warlock") && !IS_SET(ch->pcdata->clan_flags, CLAN_NO_SKILL_1))
       {
	  if (number_percent( ) < 50)
	     fConcentrate = TRUE;
	  else
	     send_to_char ("You concentrate a little harder and succeed.\n\r",ch);
       }
       else
	  fConcentrate = TRUE;
       End code nerf */

          fConcentrate = TRUE;
    }

    if (fConcentrate)
    {
       sprintf(buf, "You lost your concentration trying to cast %s.\n\r", skill_table[sn].name);
       send_to_char(buf, ch);
       //send_to_char( "You lost your concentration.\n\r", ch );
       check_improve(ch,sn,FALSE,1);
       if ( number_percent() < get_skill(ch,gsn_spellcraft) )
       {
          ch->mana -= ( mana / 4 );
          if ( spell_skill > 1 )
             check_improve(ch,gsn_spellcraft,TRUE,6);
       }
       else
       {
          if ( spell_skill > 1 )
             check_improve(ch,gsn_spellcraft,FALSE,8);
          ch->mana -= mana / 2;
       }
    }
    else
    {
       ch->mana -= mana;

       if( target == TARGET_CHAR && victim != NULL && victim != ch 
          && is_affected(victim, skill_lookup("orb of turning")) )
       {
          send_to_char("Your spell encounters an orb of turning.\n\r",ch);
          blow_orb(victim,skill_lookup("orb of turning"));
          return TRUE;
       }

       level = compute_casting_level( ch, sn );
       if( iWyrm ) level += 2;
       if( fFocus )
       {
          int dam;
          dam = ((get_skill(ch, gsn_focus)+5) /20) - (4 - ch->level/10);
          level += dam;

          /* Curve it */
         /* if ( level > 53 )
             level = ( level - 53 ) / 2 + 53;

          if ( level > 56 )
             level = ( level - 56 ) / 2 + 56;*/
	  if(level > 53)// Capped for now
	  {
		dam /= 2;
		level = 53;
	  }

          dam += mana * dam;
          dam = UMAX(dam,0);
          damage(ch,ch,dam,0,DAM_MENTAL, FALSE,FALSE);
       }
     /* Dwarf magical resistance */
     if ( target == TARGET_CHAR 
          && victim != NULL 
          && (!IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("mage")) )
          && victim != ch 
          && victim->race == race_lookup("dwarf") )
     { 
         int dwarf_magres;

         dwarf_magres = victim->level / 5;

         if ( number_percent() < dwarf_magres )
         {
             send_to_char("You failed.\n\r",ch);
             return TRUE;
         }
     }

     if ( target == TARGET_CHAR && victim != NULL &&
	  victim != ch && victim->race == race_lookup("dragon") )
     { /* Dragon magical resistance */
	 int magres;

	 magres = victim->level / 4;

	 if ( HAS_KIT(ch,"wyrmslayer") )
	     magres /= 2;

        /* Lets slash Dragon's Res in half and see what happens */
         // magres * 3 /4;

	 if ( number_percent() < magres )
	 {
	     send_to_char("You failed.\n\r",ch);
	     return TRUE;
	 }
     }

    if( target == TARGET_CHAR && victim != NULL && victim != ch &&
	number_percent() < get_skill(victim,gsn_weave_resistance)/5 )
    {
	send_to_char("You failed.\n\r",ch);
	return TRUE;
    }

     (*skill_table[sn].spell_fun) (sn, level, ch, vo, target );
     check_improve(ch,sn,TRUE,1);
    }

    if ((skill_table[sn].target == TAR_CHAR_OFFENSIVE
    ||   (skill_table[sn].target == TAR_OBJ_CHAR_OFF && target == TARGET_CHAR))
    &&   victim != ch
    &&   victim->master != ch)
    {
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;

  for ( vch = ch->in_room->people; vch; vch = vch_next )
  {
      vch_next = vch->next_in_room;
      if ( victim == vch && victim->fighting == NULL )
      { check_killer(victim,ch);
    multi_hit( victim, ch, TYPE_UNDEFINED );
    break;
      }
  }
    }

    return TRUE;
}



/*
 * Cast spells at targets using a magical object.
 */
void obj_cast_spell( int sn, int level, CHAR_DATA *ch, CHAR_DATA *victim, OBJ_DATA *obj )
{
    void *vo;
    int target = TARGET_NONE;

    if ( sn <= 0 )
  return;

    if ( sn >= MAX_SKILL || skill_table[sn].spell_fun == 0 )
    {
  bug( "Obj_cast_spell: bad sn %d.", sn );
  return;
    }

    switch ( skill_table[sn].target )
    {
    default:
  bug( "Obj_cast_spell: bad target for sn %d.", sn );
  return;

    case TAR_IGNORE:
  vo = NULL;
  break;

    case TAR_CHAR_OFFENSIVE:
  if ( victim == NULL )
      victim = ch->fighting;
  if ( victim == NULL )
    victim = ch;
  if (is_safe(ch,victim) && ch != victim)
  {
      send_to_char("Something isn't right...\n\r",ch);
      return;
  }

        check_killer(ch,victim);
  vo = (void *) victim;
  target = TARGET_CHAR;
  break;

    case TAR_CHAR_DEFENSIVE:
    case TAR_CHAR_SELF:
  if ( victim == NULL )
      victim = ch;
  vo = (void *) victim;
  target = TARGET_CHAR;
  break;

    case TAR_OBJ_INV:
  if ( obj == NULL )
  {
      send_to_char( "You can't do that.\n\r", ch );
      return;
  }
  vo = (void *) obj;
  target = TARGET_OBJ;
  break;

    case TAR_OBJ_CHAR_OFF:
        if ( victim == NULL && obj == NULL)
	{
      if (ch->fighting != NULL)
    victim = ch->fighting;
      else
      {
    send_to_char("You can't do that.\n\r",ch);
    return;
      }
	}

      if (victim != NULL)
      {
    if (is_safe_spell(ch,victim,FALSE, sn) && ch != victim)
    {
        send_to_char("Somehting isn't right...\n\r",ch);
        return;
    }

    vo = (void *) victim;
    target = TARGET_CHAR;
      }
      else
      {
        vo = (void *) obj;
        target = TARGET_OBJ;
      }
        break;


    case TAR_OBJ_CHAR_DEF:
  if (victim == NULL && obj == NULL)
  {
      vo = (void *) ch;
      target = TARGET_CHAR;
  }
  else if (victim != NULL)
  {
      vo = (void *) victim;
      target = TARGET_CHAR;
  }
  else
  {
      vo = (void *) obj;
      target = TARGET_OBJ;
  }
  
  break;
    }

    target_name = "";

    if( target == TARGET_CHAR  && victim != NULL
	&& is_affected(victim, skill_lookup("orb of turning")) )
    {
        send_to_char("Your spell encounters an orb of turning.\n\r",ch);
        blow_orb(victim,skill_lookup("orb of turning"));
        return;
    }

    (*skill_table[sn].spell_fun) ( sn, level, ch, vo,target);

    

    if ( skill_table[sn].target == TAR_CHAR_OFFENSIVE
    &&   victim != ch
    &&   victim->master != ch )
    {
  CHAR_DATA *vch;
  CHAR_DATA *vch_next;

  for ( vch = ch->in_room->people; vch; vch = vch_next )
  {
      vch_next = vch->next_in_room;
      if ( victim == vch && victim->fighting == NULL )
      {
    multi_hit( victim, ch, TYPE_UNDEFINED );
    break;
      }
  }
    }

    return;
}



/*
 * Spell functions.
 */
void spell_acid_blast( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( level, 12 );
    if (ch->class == class_lookup("mage"))
    {
      dam = dice( level, 14);
    }
    if (ch->class == class_lookup("elementalist"))
    {
      dam = dice( level, 10);
    }

    if ( saves_spell( level, victim, DAM_ACID ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_ACID,TRUE,TRUE);
    return;
}


bool reup_affect(CHAR_DATA *ch, int sn, int duration, int level)
{
  AFFECT_DATA *paf;
  AFFECT_DATA *paf_next;
  bool up=FALSE;

 for ( paf = ch->affected; paf != NULL; paf = paf_next )
  {
      paf_next  = paf->next;

      if ( paf->type == sn && paf->duration >= 0)
      {
	    paf->duration = (paf->duration + duration)/2;
	    paf->level = (paf->level + level)/2;
	    up = TRUE;
      }
   }
   return up;
}

void spell_armor( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char( "You feel someone protecting you.\n\r", victim );
	if (victim != ch)
	{
	  act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR,FALSE);
	}
	reup_affect(victim,sn,24+level/4,level);
	return;
    }
    af.where   = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 24 + level / 4;
    af.modifier  = -20;
    af.location  = APPLY_AC;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel someone protecting you.\n\r", victim );
    if ( ch != victim )
  act("$N is protected by your magic.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}



void spell_bless( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
  obj = (OBJ_DATA *) vo;
  if (IS_OBJ_STAT(obj,ITEM_BLESS))
  {
      act("$p is already blessed.",ch,obj,NULL,TO_CHAR,FALSE);
      return;
  }

  if (IS_OBJ_STAT(obj,ITEM_EVIL))
  {
      AFFECT_DATA *paf;

      paf = affect_find(obj->affected,gsn_curse);
      if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
      {
    if (paf != NULL)
        affect_remove_obj(obj,paf);
    act("$p glows a pale blue.",ch,obj,NULL,TO_ALL,FALSE);
    REMOVE_BIT(obj->extra_flags,ITEM_EVIL);
    return;
      }
      else
      {
    act("The evil of $p is too powerful for you to overcome.",
        ch,obj,NULL,TO_CHAR,FALSE);
    return;
      }
  }
  
  af.where  = TO_OBJECT;
  af.type   = sn;
  af.level  = level;
  af.duration = 6 + level;
  af.location = APPLY_SAVES;
  af.modifier = -1;
  af.bitvector  = ITEM_BLESS;
  affect_to_obj(obj,&af);

  act("$p glows with a holy aura.",ch,obj,NULL,TO_ALL,FALSE);
  return;
    }

    /* character target */
    victim = (CHAR_DATA *) vo;


    if ( is_affected( victim, sn ) )
    {
    send_to_char( "You feel righteous.\n\r", victim );
    if ( ch != victim )
  act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR,FALSE);
	reup_affect(victim,sn,6+level,level);
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 6+level;
    af.location  = APPLY_HITROLL;
    af.modifier  = level / 8;
    af.bitvector = 0;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = 0 - level / 8;
    affect_to_char( victim, &af );
    send_to_char( "You feel righteous.\n\r", victim );
    if ( ch != victim )
  act("You grant $N the favor of your god.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}



void spell_blindness( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    char buf[MAX_STRING_LENGTH];

    level = UMIN(ch->level,level);

    if ( saves_spell(level,victim,DAM_OTHER))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if (IS_AFFECTED(victim,AFF_BLIND))
    {
	if(reup_affect(victim,sn,level/3,level))
	{
    send_to_char( "You are {Gblinded{x!\n\r", victim );
    if ( ch != victim )
    act("$n appears to be {Gblinded{x.",victim,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }


    /* Gladiator Spectator Channel */ 
    if (IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       sprintf(buf,"%s throws a flaming blast from his hands and singes %s's eyes.",ch->name,victim->name);
       gladiator_talk(buf);
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.location  = APPLY_HITROLL;
    af.modifier  = -4;
    af.duration  = 1+level/4;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = 1+level/8;break;
        case 1:af.duration = 1+level/6;break;
        case 2:af.duration = 1+level/4;break;
        default:af.duration = 1+level/4;break;
        }
    }
    if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
       af.duration  = 1;
    af.bitvector = AFF_BLIND;
    affect_to_char( victim, &af );
    send_to_char( "You are {Gblinded{x!\n\r", victim );
    act("$n appears to be {Gblinded{x.",victim,NULL,NULL,TO_ROOM,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_blindness(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



void spell_burning_hands(int sn,int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
   0,
   0,  0,  0,  0, 14, 17, 20, 23, 26, 29,
  29, 29, 30, 30, 31, 31, 32, 32, 33, 33,
  34, 34, 35, 35, 36, 36, 37, 37, 38, 38,
  39, 39, 40, 40, 41, 41, 42, 42, 43, 43,
  44, 44, 45, 45, 46, 46, 47, 47, 48, 48
    };
    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if ( saves_spell( level, victim,DAM_FIRE) )
    {
      dam /= 2;
    }
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    damage( ch, victim, dam, sn, DAM_FIRE,TRUE,TRUE);
    return;
}



void spell_call_lightning( int sn, int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    char buf[MAX_STRING_LENGTH];
    int dam;

    if ( !IS_OUTSIDE(ch) )
    {
  send_to_char( "You must be out of doors.\n\r", ch );
  return;
    }

    if ( weather_info.sky < SKY_RAINING )
    {
  send_to_char( "You need bad weather.\n\r", ch );
  return;
    }

    dam = dice(level,10 );

    sprintf(buf,"%s's lightning strikes your foes!\n\r",
		deity_table[ch->pcdata->deity].pname);
    send_to_char(buf,ch);
    act( "$n calls $s deity's lightning to strike $s foes!",
  ch, NULL, NULL, TO_ROOM ,FALSE);

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
  vch_next  = vch->next;
  if ( vch->in_room == NULL )
      continue;
  if ( vch->in_room == ch->in_room )
  {
      if ( vch != ch && !is_safe_spell(ch,vch,TRUE, sn) )
	/* None of this PC's only hitting NPC's and vice versa
	   && ( IS_NPC(ch) ? !IS_NPC(vch) : IS_NPC(vch) ) )
	 */
    damage( ch, vch, saves_spell( level,vch,DAM_LIGHTNING) 
    ? dam / 2 : dam, sn,DAM_LIGHTNING,TRUE,TRUE);
      continue;
  }

  if ( vch->in_room->area == ch->in_room->area
  &&   IS_OUTSIDE(vch)
  &&   IS_AWAKE(vch) )
      send_to_char( "Lightning flashes in the sky.\n\r", vch );
    }

    return;
}

/* RT calm spell stops all fighting in the room */

void spell_calm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    int mlevel = 0;
    int count = 0;
    int high_level = 0;    
    int chance;
    AFFECT_DATA af;


    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
	ch->in_room == get_room_index(ROOM_VNUM_SINGLE_GLADIATOR))
    {
	send_to_char("No casting Calm in Prep Room!\n\r",ch);
	return;
    }

    /* get sum of all mobile levels in the room */
    for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
  if (vch->position == POS_FIGHTING)
  {
      count++;
      if (IS_NPC(vch))
        mlevel += vch->level;
      else
        mlevel += vch->level/2;
      high_level = UMAX(high_level,vch->level);
  }
    }

    /* compute chance of stopping combat */
    chance = 4 * level - high_level + 2 * count;

    /*necromancers are creepy, make them have a less chance of beguiling -Boogums*/
    if (ch->kit== kit_lookup("necromancer") )
    {
      chance=chance*2/3;
    }

    if (IS_IMMORTAL(ch)) /* always works */
      mlevel = 0;

    if (number_range(0, chance) >= mlevel)  /* hard to stop large fights */
    {
  for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room)
    {
      if (IS_NPC(vch) && (IS_SET(vch->imm_flags,IMM_MAGIC) ||
        IS_SET(vch->act,ACT_UNDEAD)))
        return;

      if (IS_AFFECTED(vch,AFF_CALM) || IS_AFFECTED(vch,AFF_BERSERK)
      ||  is_affected(vch,skill_lookup("frenzy")))
        return;
      
      if(!is_clan(ch) && is_clan(vch) && !IS_NPC(vch) && !IS_NPC(ch) )
      {
 	/* a non-clanner cannot affect a clanner */      
	/* do nothing */
      }
      else if(is_clan(ch) && !is_clan(vch) && !IS_NPC(vch) && !IS_NPC(ch) )
      {
 	/* a clanner cannot affect a non-clanner */      
	/* do nothing*/
      }
      else
      {

      send_to_char("A wave of calm passes over you.\n\r",vch);

      if (vch->fighting || vch->position == POS_FIGHTING)
        stop_fighting(vch,FALSE);


      af.where = TO_AFFECTS;
      af.type = sn;
        af.level = level;
      af.duration = level/4;
      af.location = APPLY_HITROLL;
      if (!IS_NPC(vch))
        af.modifier = -5;
      else
        af.modifier = -2;
      af.bitvector = AFF_CALM;
      affect_to_char(vch,&af);

      af.location = APPLY_DAMROLL;
      affect_to_char(vch,&af);
      }

  }
    }
}

void spell_cancellation( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
 
    level += 2;
	if( level < ch->level)
      level = UMIN(ch->level+2, level + ch->level/5);

    if (((!IS_NPC(ch) && IS_NPC(victim) && 
   !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) ) ||
        (IS_NPC(ch) && !IS_NPC(victim)) ||
        (!IS_NPC(victim) && IS_SET (victim->act,PLR_NOCANCEL) && (ch != victim)))
    && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_IS_HEALER))
    )
    {
  send_to_char("You failed, try dispel magic.\n\r",ch);
  return;
    }

    /* unlike dispel magic, the victim gets NO save */
 
    /* begin running through the spells */

    if (check_dispel(level,victim,gsn_spirit_of_boar) )
	found = TRUE;

    if (check_dispel(level,victim,gsn_spirit_of_bear) )
        found = TRUE;

    if (check_dispel(level,victim,gsn_spirit_of_cat ) )
        found = TRUE;

    if (check_dispel(level,victim,gsn_spirit_of_owl ) )
        found = TRUE;

    if (check_dispel(level,victim,gsn_spirit_of_wolf) )
        found = TRUE;

    if (check_dispel(level/2,victim,skill_lookup("spirit of phoenix")))
	found = TRUE; 
    
    if (check_dispel(level,victim,gsn_honor_guard) )
	found = TRUE;
 
    if (check_dispel(level,victim,gsn_aura_of_valor) )
        found = TRUE;
    if (check_dispel(level,victim,gsn_spell_restrain) )
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("pox")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("armor")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("aura of cthon")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("asphyxiate")))
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("bless")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("blindness")))
    {
        found = TRUE;
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("body of stone")))
    {
	found = TRUE;
	act("$n regains a fleshy texture.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("calm")))
    {
  found = TRUE;
  act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("change sex")))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("charm person")))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("cone of silence")))
    {
        found = TRUE;
        act("$n can hear again.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("chill touch")))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("orb of touch")))
    {
        found = TRUE;
        act("$n can be touched.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("curse")) 
	&& !IS_AFFECTED(victim,AFF_MORPH))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("detect good")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect invis")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect magic")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("tower of iron will")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("body weaponry")))
        found = TRUE;

    if (check_dispel(level-2,victim,skill_lookup("earthbind")))
	found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("faerie fog")))
    {
        act("$n's purple cloud fades.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("faerie fire")))
    {
        act("$n's outline fades.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("fly")))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("frenzy")))
    {
  act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM,FALSE);;
  found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("fumble")))
	found = TRUE;

    if (check_dispel(level-1,victim,skill_lookup("confusion")))
    {
        found = TRUE;
        act("$n doesn't look boggled.",victim,NULL,NULL,TO_ROOM,FALSE);
    }


    if (check_dispel(level,victim,skill_lookup("giant strength")))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("haste")))
    {
  act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM,FALSE);
  found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("infravision")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("invis")))
    {
        act("$n fades into existance.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("irradiate")))
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("mass invis")))
    {
        act("$n fades into existance.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("pass door")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("protection neutral")))
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("protection evil")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("protection good")))
        found = TRUE; 
 
    if (check_dispel(level,victim,skill_lookup("sanctuary")))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("shield")))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("sleep")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("slow")))
    {
        act("$n is no longer moving so slowly.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("stone skin")))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("weaken")))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
    
    if (check_dispel(level,victim,skill_lookup("withstand death")))
    {
        act("$n no longer looks more powerful than death.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }    
 
    if (found)
        send_to_char("Ok.\n\r",ch);
    else
        send_to_char("Spell failed.\n\r",ch);
}

void spell_cause_light( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

       dam = dice( level, 2);

    if ( saves_spell( level, victim, DAM_HARM ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_HARM,TRUE,TRUE);
    return;
}



void spell_cause_critical(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

       dam = dice( level, 6);

    if ( saves_spell( level, victim, DAM_HARM ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_HARM,TRUE,TRUE);
    return;
}



void spell_cause_serious(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

       dam = dice( level, 4);

    if ( saves_spell( level, victim, DAM_HARM ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_HARM,TRUE,TRUE);
    return;
}

void spell_chain_lightning(int sn,int level,CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *tmp_vict,*last_vict,*next_vict;
    bool found;
    int dam;

    /* first strike */

    act("A lightning bolt leaps from $n's hand and arcs to $N.",
        ch,NULL,victim,TO_ROOM,FALSE);
    act("A lightning bolt leaps from your hand and arcs to $N.",
  ch,NULL,victim,TO_CHAR,FALSE);
    act("A lightning bolt leaps from $n's hand and hits you!",
  ch,NULL,victim,TO_VICT,FALSE);  

    dam = dice(level,6);
    if (ch->class == class_lookup("mage"))
    {
      dam = dice( level, 9);
    }
    if (saves_spell(level,victim,DAM_LIGHTNING))
  dam /= 3;
    damage(ch,victim,dam,sn,DAM_LIGHTNING,TRUE,TRUE);
    last_vict = victim;
    level -= 4;   /* decrement damage */

    /* new targets */
    while (level > 0)
    {
  found = FALSE;
  for (tmp_vict = ch->in_room->people; 
       tmp_vict != NULL; 
       tmp_vict = next_vict) 
  {
    next_vict = tmp_vict->next_in_room;
    if (!is_safe_spell(ch,tmp_vict,TRUE, sn) && tmp_vict != last_vict)
    {
      found = TRUE;
      last_vict = tmp_vict;
      /** 
      act("The bolt arcs to $n!",tmp_vict,NULL,NULL,TO_ROOM,FALSE);
      act("The bolt hits you!",tmp_vict,NULL,NULL,TO_CHAR,FALSE);
       **/
      dam = dice(level,6);
      if (saves_spell(level,tmp_vict,DAM_LIGHTNING))
    dam /= 3;
      damage(ch,tmp_vict,dam,sn,DAM_LIGHTNING,TRUE,TRUE);
      level -= 4;  /* decrement damage */
    }
  }   /* end target searching loop */
  
  if (!found) /* no target found, hit the caster */
  {
    if (ch == NULL)
          return;

    if (last_vict == ch) /* no double hits */
    {
      act("The bolt seems to have fizzled out.",ch,NULL,NULL,TO_ROOM,FALSE);
      act("The bolt grounds out through your body.",
    ch,NULL,NULL,TO_CHAR,FALSE);
      return;
    }
  
    last_vict = ch;
    act("The bolt arcs to $n...whoops!",ch,NULL,NULL,TO_ROOM,FALSE);
    send_to_char("You are struck by your own lightning!\n\r",ch);
    dam = dice(level,6);
    if (saves_spell(level,ch,DAM_LIGHTNING))
      dam /= 3;
    damage(ch,ch,dam,sn,DAM_LIGHTNING,TRUE,TRUE);
    level -= 4;  /* decrement damage */
    if (ch == NULL) 
      return;
  }
    /* now go back and find more targets */
    }
}
    

void spell_change_sex( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ))
    {
  if (victim == ch)
    send_to_char("You've already been changed.\n\r",ch);
  else
    act("$N has already had $s(?) sex changed.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }
    if (saves_spell(level , victim,DAM_OTHER))
  return; 
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 2 * level;
    af.location  = APPLY_SEX;
    do
    {
  af.modifier  = number_range( 0, 2 ) - victim->sex;
    }
    while ( af.modifier == 0 );
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You feel different.\n\r", victim );
    act("$n doesn't look like $mself anymore...",victim,NULL,NULL,TO_ROOM,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_change_sex(sn,level+2,victim,ch,target);
    }   
    return;
}



void spell_charm_person( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *fvict;
    AFFECT_DATA af;
    int levtot = 0;

    if (is_safe(ch,victim)) return;

    if ( victim == ch )
    {
  send_to_char( "You like yourself even better!\n\r", ch );
  return;
    }

    if(is_affected(ch,skill_lookup("wound transfer")))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Highlanders can not charm.\n\r",ch);
       return;
    }
    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not charm person.\n\r",ch);
       return;
    }
    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians may not be mentally manipulated.\n\r", ch);
      return;
    }

    if ( !IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("mage")) )
	level = 9* level /10 ;
    /*necromancers are creepy, they don't beguile well -Boogums*/
    if (ch->kit== kit_lookup("necromancer") )
    {
      level = level*3/4;
    }
//make social stat affect it a bit
    if (!IS_NPC(ch) )
    {
	if(get_curr_stat(ch,STAT_SOC) < 10)
	{
	    level = level - number_range(1,4);
	}
	if(get_curr_stat(ch,STAT_SOC) >= 10 &&
           get_curr_stat(ch,STAT_SOC) <= 18 )
  	{
	    level = level + number_range(-2,2);
	}

	if(get_curr_stat(ch,STAT_SOC) >= 19 )
	{
	  level = level + number_range(1,4);
	}
    }
    if ( (IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE))
    ||   IS_AFFECTED(victim, AFF_CHARM)
    ||   IS_AFFECTED(ch, AFF_CHARM)
    ||   IS_SET(victim->act, PLR_DWEEB) 
    ||   level < victim->level
    ||   IS_SET(victim->imm_flags,IMM_CHARM)
    ||   saves_spell( level, victim,DAM_CHARM)
    ||   victim->position == POS_FIGHTING )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

  for ( fvict = char_list; fvict != NULL; fvict = fvict->next )
    {
    if ( is_same_group( fvict, ch ) && (IS_NPC(fvict) && fvict->master == ch) )
      levtot += fvict->level;
    }

  if ( IS_NPC(victim) &&
	(compute_casting_level(ch,sn)*5) < (levtot + victim->level) )
  {
    send_to_char ("You're controlling as many mental slaves as you can handle.",ch);
    return;
  }

/*    if (IS_SET(victim->in_room->room_flags,ROOM_LAW))
    {
  send_to_char(
      "The mayor does not allow charming in the city limits.\n\r",ch);
  return;
    }
*/  

    switch(check_immune(victim,DAM_MENTAL))
    {
  case IS_IMMUNE:   level = (IS_NPC(victim)?1:0);  break;
  case IS_RESISTANT:  level = (IS_NPC(victim)?level/2:1);  break;
  case IS_VULNERABLE: level = (IS_NPC(victim)?level+4:2);  break;
  default:	level = (IS_NPC(victim)?level:1); break;
    }

    if ( IS_NPC(victim) && victim->spec_fun != 0 )
       if (victim->spec_fun == spec_lookup("spec_executioner")
	   || victim->spec_fun == spec_lookup("spec_guard_l")
	   || victim->spec_fun == spec_lookup("spec_guard_d"))
	  victim->spec_fun = 0;

    if ( victim->master )
      stop_follower( victim );
    add_follower( victim, ch );
    //victim->leader = ch;
    add_to_group(victim, ch);
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = IS_NPC(victim)?number_fuzzy(level/4):level;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = IS_NPC(victim)?number_fuzzy(level/8):level/4;break;
        case 1:af.duration = IS_NPC(victim)?number_fuzzy(level/6):level/2;break;
        case 2:af.duration = IS_NPC(victim)?number_fuzzy(level/4):level;break;
        default:af.duration = IS_NPC(victim)?number_fuzzy(level/4):level;break;
        }
    }
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_CHARM;
    affect_to_char( victim, &af );
    act( "Isn't $n just so nice?", ch, NULL, victim, TO_VICT ,FALSE);
    if ( ch != victim )
  act("$N looks at you with adoring eyes.",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_charm_person(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



void spell_chill_touch( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
   0,
   0,  0,  6,  7,  8,  9, 12, 13, 13, 13,
  14, 14, 14, 15, 15, 15, 16, 16, 16, 17,
  17, 17, 18, 18, 18, 19, 19, 19, 20, 20,
  20, 21, 21, 21, 22, 22, 22, 23, 23, 23,
  24, 24, 24, 25, 25, 25, 26, 26, 26, 27
    };
    AFFECT_DATA af;
    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( !saves_spell( level, victim,DAM_COLD ) && !is_affected( victim, sn ))
    {
  act("$n turns blue and shivers.",victim,NULL,NULL,TO_ROOM,FALSE);
  af.where     = TO_AFFECTS;
  af.type      = sn;
        af.level     = level;
  af.duration  = 6;
  af.location  = APPLY_STR;
  af.modifier  = -1;
  af.bitvector = 0;
  affect_join( victim, &af );
    }
    else
    {
  dam /= 2;
    }

    damage( ch, victim, dam, sn, DAM_COLD,TRUE ,TRUE);
    return;
}



void spell_colour_spray( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
   0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  30, 35, 40, 45, 50, 55, 55, 55, 56, 57,
  58, 58, 59, 60, 61, 61, 62, 63, 64, 64,
  65, 66, 67, 67, 68, 69, 70, 70, 71, 72,
  73, 73, 74, 75, 76, 76, 77, 78, 79, 79
    };
    int dam;

    if(IS_SET(ch->in_room->room_affects, RAFF_SHADED))
    {
      send_to_char("The darkness swallows up your burst of light.\n\r", ch);
      return;
    }

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2,  dam_each[level] * 2 );
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim,DAM_LIGHT) )
  dam /= 2;
    else 
  spell_blindness(skill_lookup("blindness"),
      level/2,ch,(void *) victim,TARGET_CHAR);

    damage( ch, victim, dam, sn, DAM_LIGHT,TRUE ,TRUE);
    return;
}



void spell_continual_light(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *light;

    if (target_name[0] != '\0')  /* do a glow on some object */
    {
  light = get_obj_carry(ch,target_name);
  
  if (light == NULL)
  {
      send_to_char("You don't see that here.\n\r",ch);
      return;
  }

  if (IS_OBJ_STAT(light,ITEM_GLOW))
  {
      act("$p is already glowing.",ch,light,NULL,TO_CHAR,FALSE);
      return;
  }

  SET_BIT(light->extra_flags,ITEM_GLOW);
  act("$p glows with a white light.",ch,light,NULL,TO_ALL,FALSE);
  return;
    }

    light = create_object( get_obj_index( OBJ_VNUM_LIGHT_BALL ), 0, FALSE );
    obj_to_room( light, ch->in_room );
    act( "$n twiddles $s thumbs and $p appears.",   ch, light, NULL, TO_ROOM ,FALSE);
    act( "You twiddle your thumbs and $p appears.", ch, light, NULL, TO_CHAR ,FALSE);
    return;
}



void spell_control_weather(int sn,int level,CHAR_DATA *ch,void *vo,int target) 
{
    if ( !str_cmp( target_name, "better" ) )
  weather_info.change += dice( level / 3, 4 );
    else if ( !str_cmp( target_name, "worse" ) )
  weather_info.change -= dice( level / 3, 4 );
    else
  send_to_char ("Do you want it to get better or worse?\n\r", ch );

    send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_create_food( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *mushroom;

    mushroom = create_object( get_obj_index( OBJ_VNUM_MUSHROOM ), 0, FALSE );
    mushroom->value[0] = level / 2;
    mushroom->value[1] = level;
    obj_to_room( mushroom, ch->in_room );
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_ROOM ,FALSE);
    act( "$p suddenly appears.", ch, mushroom, NULL, TO_CHAR ,FALSE);
    return;
}

void spell_create_rose( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *rose;
    rose = create_object(get_obj_index(OBJ_VNUM_ROSE), 0, FALSE);
    act("$n has created a beautiful red rose.",ch,rose,NULL,TO_ROOM,FALSE);
    send_to_char("You create a beautiful red rose.\n\r",ch);
    obj_to_char(rose,ch);
    return;
}

void spell_create_fountain(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *fount;

    fount = create_object( get_obj_index( OBJ_VNUM_FOUNTAIN ), 0, FALSE );
    fount->timer = level;
    fount->value[2] = liq_lookup( target_name );

    if(fount->value[2] < 0)
      fount->value[2] = 0;

    obj_to_room( fount, ch->in_room );
    act( "$p materializes before you.",ch,fount,NULL,TO_CHAR,FALSE);
    act( "$p materializes before you.",ch,fount,NULL,TO_ROOM,FALSE);
    return;
}

void spell_create_spring(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    OBJ_DATA *spring;

    spring = create_object( get_obj_index( OBJ_VNUM_SPRING ), 0, FALSE );
    spring->timer = level;
    obj_to_room( spring, ch->in_room );
    act( "$p flows from the ground.", ch, spring, NULL, TO_ROOM ,FALSE);
    act( "$p flows from the ground.", ch, spring, NULL, TO_CHAR ,FALSE);
    return;
}



void spell_create_water( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int water;

    if ( obj->item_type != ITEM_DRINK_CON  
	|| obj->pIndexData->vnum == OBJ_VNUM_GRAIL)
    {
  send_to_char( "It is unable to hold water.\n\r", ch );
  return;
    }

    if ( obj->value[2] != LIQ_WATER && obj->value[1] != 0 )
    {
  send_to_char( "It contains some other liquid.\n\r", ch );
  return;
    }

    water = UMIN(
    level * (weather_info.sky >= SKY_RAINING ? 4 : 2),
    obj->value[0] - obj->value[1]
    );
  
    if ( water > 0 )
    {
  obj->value[2] = LIQ_WATER;
  obj->value[1] += water;
  if ( !is_name( "water", obj->name ) )
  {
      char buf[MAX_STRING_LENGTH];

      sprintf( buf, "%s water", obj->name );
      free_string( obj->name );
      obj->name = str_dup( buf );
  }
  act( "$p is filled.", ch, obj, NULL, TO_CHAR ,FALSE);
    }

    return;
}



void spell_cure_blindness(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if ( !is_affected( victim, gsn_blindness ) && !is_affected(victim,skill_lookup("eye gouge")) )
    {
        if (victim == ch)
          send_to_char("You aren't blind.\n\r",ch);
        else
          act("$N doesn't appear to be blinded.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }
 
   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric")) ) 
	level = level * 9 / 10 ;

    if ( is_affected(victim,gsn_blindness) && check_dispel(level,victim,gsn_blindness))
    {
        send_to_char( "Your vision returns!\n\r", victim );
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    else
    if ( check_dispel(level,victim,skill_lookup("eye gouge") ) )
    {
	send_to_char("Your vision returns!\n\r",victim);
	act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_critical( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(3, 8) + level - 6;
    if ( ch->race == race_lookup("volare") )
	heal = 3 * heal / 2;

    if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric") &&
		 ch->pcdata->old_class!=class_lookup("elementalist")))
	heal -= ( heal / 4 );

    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}

/* RT added to cure plague */
void spell_cure_disease( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int found = FALSE;

    if ( !is_affected( victim, gsn_plague ) &&
	 !is_affected( victim, gsn_irradiate ) )
    {
        if (victim == ch)
          send_to_char("You aren't ill.\n\r",ch);
        else
          act("$N doesn't appear to be diseased.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }
    
   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric") &&
	 ch->pcdata->old_class!=class_lookup("elementalist")))
	level = 9 * level / 10;

    if (check_dispel(level,victim,gsn_plague))
    {
  send_to_char("Your sores vanish.\n\r",victim);
  act("$n looks relieved as $s sores vanish.",victim,NULL,NULL,TO_ROOM,FALSE);
  found = TRUE;
    }

    if (check_dispel(level,victim,gsn_irradiate))
    {
  send_to_char("Your sickness leaves your body.\n\r",victim);
  act("$n appears to hvae regained $s health.",victim,NULL,NULL,TO_ROOM,FALSE);
  found = TRUE;
    }

   /* 
    if (check_dispel(level, victim, skill_lookup("confusion")))
    {
  act("$n appears to have regained $s mental faculties.\n\r", victim, NULL, NULL, TO_ROOM, FALSE);
  found = TRUE;
    }
    */
  
  if( !found )
  send_to_char("Spell failed.\n\r",ch);
}



void spell_cure_light( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(1, 8) + level / 3;
    if ( ch->race == race_lookup("volare") )
	heal = 3 * heal / 2;

   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric") &&
	 ch->pcdata->old_class!=class_lookup("elementalist")))
	heal -= ( heal / 4 );

    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_cure_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
 
    if ( !is_affected( victim, gsn_poison ) )
    {
        if (victim == ch)
          send_to_char("You aren't poisoned.\n\r",ch);
        else
          act("$N doesn't appear to be poisoned.",ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }
 
   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric") &&
	 ch->pcdata->old_class!=class_lookup("elementalist")))
	level = 9* level/ 10;
	
    if (check_dispel(level,victim,gsn_poison))
    {
        send_to_char("A warm feeling runs through your body.\n\r",victim);
        act("$n looks much better.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    else
        send_to_char("Spell failed.\n\r",ch);
}


void spell_cure_serious( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int heal;

    heal = dice(2, 8) + level /2 ;
    if ( ch->race == race_lookup("volare") )
	heal = 3 * heal / 2;

    if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("cleric") &&
		 ch->pcdata->old_class!=class_lookup("elementalist")))
	heal -= ( heal / 4 );

    victim->hit = UMIN( victim->hit + heal, victim->max_hit );
    update_pos( victim );
    send_to_char( "You feel better!\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* deal with the object case first */
    if (target == TARGET_OBJ)
    {
        obj = (OBJ_DATA *) vo;
        if (IS_OBJ_STAT(obj,ITEM_EVIL))
        {
            act("$p is already filled with evil.",ch,obj,NULL,TO_CHAR,FALSE);
            return;
        }

        if (IS_OBJ_STAT(obj,ITEM_BLESS))
        {
            AFFECT_DATA *paf;

            paf = affect_find(obj->affected,skill_lookup("bless"));
            if (!saves_dispel(level,paf != NULL ? paf->level : obj->level,0))
            {
                if (paf != NULL)
                    affect_remove_obj(obj,paf);
                act("$p glows with a red aura.",ch,obj,NULL,TO_ALL,FALSE);
                REMOVE_BIT(obj->extra_flags,ITEM_BLESS);
                return;
            }
            else
            {
                act("The holy aura of $p is too powerful for you to overcome.",
                    ch,obj,NULL,TO_CHAR,FALSE);
                return;
            }
        }

        af.where        = TO_OBJECT;
        af.type         = sn;
        af.level        = level;
        af.duration     = 2 * level;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = level/2;break;
        case 1:af.duration = level;break;
        case 2:af.duration = 2 * level;break;
        default:af.duration = 2 * level;break;
        }
    }
        af.location     = APPLY_SAVES;
        af.modifier     = +1;
        af.bitvector    = ITEM_EVIL;
        affect_to_obj(obj,&af);

        act("$p glows with a malevolent aura.",ch,obj,NULL,TO_ALL,FALSE);
        return;
    }

    /* character curses */
    victim = (CHAR_DATA *) vo;

    if ( saves_spell(level,victim,DAM_NEGATIVE))
    {
       send_to_char("You failed.\n\r",ch);
       return;
    }

    if (IS_AFFECTED(victim,AFF_CURSE) || is_affected(ch,gsn_morph))
    {
	if(reup_affect(victim,sn,level/3,level))
	{
    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
  act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    af.location  = APPLY_HITROLL;
    af.modifier  = -1 * (level / 8);
    af.bitvector = AFF_CURSE;
    affect_to_char( victim, &af );

    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = level / 8;
    affect_to_char( victim, &af );

    send_to_char( "You feel unclean.\n\r", victim );
    if ( ch != victim )
  act("$N looks very uncomfortable.",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_curse(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

/* RT replacement demonfire spell */

void spell_demonfire(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( !IS_NPC(ch) && !IS_EVIL(ch) )
    {
        victim = ch;
  send_to_char("The demons turn upon you!\n\r",ch);
    }
 
   if ( !is_affected(ch, skill_lookup("indulgence")) )
    ch->alignment = UMAX(-1000, ch->alignment - 50);

    if (victim != ch)
    {
  act("$n calls forth the demons of Hell upon $N!",
      ch,NULL,victim,TO_ROOM,FALSE);
        act("$n has assailed you with the demons of Hell!",
      ch,NULL,victim,TO_VICT,FALSE);
  send_to_char("You conjure forth the demons of hell!\n\r",ch);
    }
    dam = dice( level, 10 );
    if ( saves_spell( level, victim,DAM_NEGATIVE) )
        dam /= 2;
    spell_curse(gsn_curse, 3 * level / 4, ch, (void *) victim,TARGET_CHAR);
    damage( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE,TRUE);
}

void spell_detect_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_ALIGN) )
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = number_fuzzy(level);
    af.duration  = number_fuzzy(level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_ALIGN;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}


void spell_detect_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if ( IS_AFFECTED(victim, AFF_DETECT_ALIGN) )
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }
 
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = number_fuzzy(level);
    af.duration  = number_fuzzy(level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_ALIGN;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_hidden(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_HIDDEN) )
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "Your awareness improves.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = number_fuzzy(level);
    af.duration  = number_fuzzy(level);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_DETECT_HIDDEN;
    affect_to_char( victim, &af );
    send_to_char( "Your awareness improves.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_INVIS) )
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = number_fuzzy(level);
    af.duration  = number_fuzzy(level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_INVIS;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_DETECT_MAGIC) )
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = number_fuzzy(level);
    af.duration  = number_fuzzy(level);
    af.modifier  = 0;
    af.location  = APPLY_NONE;
    af.bitvector = AFF_DETECT_MAGIC;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes tingle.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_detect_poison( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;

    if ( obj->item_type == ITEM_DRINK_CON || obj->item_type == ITEM_FOOD )
    {
  if ( obj->value[3] != 0 )
      send_to_char( "You smell poisonous fumes.\n\r", ch );
  else
      send_to_char( "It looks delicious.\n\r", ch );
    }
    else
    {
  send_to_char( "It doesn't look poisoned.\n\r", ch );
    }

    return;
}



void spell_dispel_evil( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
  
    if ( !IS_NPC(ch) && IS_EVIL(ch) )
  victim = ch;
  
    if ( IS_GOOD(victim) )
    {
  act( "$N is protected by $s deity.", ch, NULL, victim, TO_ROOM ,FALSE);
  return;
    }

    if ( IS_NEUTRAL(victim) )
    {
  act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR ,FALSE);
  return;
    }

    if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
    else
      dam = UMAX(victim->hit, dice(level,4));
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim,DAM_HOLY) )
  dam /= 2;
    damage( ch, victim, dam, sn, DAM_HOLY ,TRUE,TRUE);
    return;
}


void spell_dispel_good( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
 
    if ( !IS_NPC(ch) && IS_GOOD(ch) )
        victim = ch;
 
    if ( IS_EVIL(victim) )
    {
        act( "$N is protected by $S evil.", ch, NULL, victim, TO_ROOM ,FALSE);
        return;
    }
 
    if ( IS_NEUTRAL(victim) )
    {
        act( "$N does not seem to be affected.", ch, NULL, victim, TO_CHAR ,FALSE);
        return;
    }
 
    if (victim->hit > (ch->level * 4))
      dam = dice( level, 4 );
    else
      dam = UMAX(victim->hit, dice(level,4));
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim,DAM_NEGATIVE) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE,TRUE);
    return;
}


/* modified for enhanced use */

void spell_dispel_magic( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    bool found = FALSE;
    char buf[MAX_STRING_LENGTH];
    int  lower_level; 

/*02OCT02 trying to help out casters a bit - Corey*/
#ifdef COREYTEST
    if ( !IS_NPC(victim) )
    {
	switch( class_table[victim->pcdata->old_class].fMana )
	{
	case 0:
		level += 2;
		break;
	case 1:
		level += 1;
		break;
	case 2:
		level += 0;
		break;
	default:
		level += 0;
		break;
	} 
    }
#endif

    if (saves_spell(level, victim,DAM_OTHER))
    {
  send_to_char( "You feel a brief tingling sensation.\n\r",victim);
  send_to_char( "You failed.\n\r", ch);
  return;
    }

    /* begin running through the spells */ 

    if ((IS_SET(victim->mhs,MHS_GLADIATOR) || 
	IS_SET(victim->mhs,MHS_HIGHLANDER)) && !IS_NPC(victim)) 
       lower_level = 15;
    else
       lower_level = 66;

    if (check_dispel(level/2,victim,skill_lookup("spirit of phoenix")))
    {
	act("$n shivers as though a great spirit has left.",victim,NULL,NULL,TO_ROOM,FALSE);
	found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("wound transfer")))
    {
	int dam;

	if(number_percent() > lower_level ) level--;
	found = TRUE;
	act("Your body screams in pain as your soul is stripped of its link.",
		ch,NULL,victim,TO_VICT,FALSE);
	act("$N cries out in pain!",ch,NULL,victim,TO_CHAR,FALSE);
	act("$N cries out in pain!",ch,NULL,victim,TO_NOTVICT,FALSE);
	dam = number_range( level / 2, 3 * level / 2 );
	   damage( ch, victim, dam, sn, DAM_NEGATIVE, FALSE,TRUE);
    }

    if (check_dispel(level,victim,skill_lookup("armor")))
    {
	if(number_percent() > lower_level) level--;
	found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("asphyxiate")))
    {
	found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("bless")))
    {
	if(number_percent() > lower_level) level--;
	found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("blindness")))
    {
        found = TRUE;
        act("$n is no longer blinded.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("body of stone")))
    {
	if(number_percent() > lower_level) level--;
	found = TRUE;
	act("$n regains a fleshy texture.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("calm")))
    {
        found = TRUE;
        act("$n no longer looks so peaceful...",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("change sex")))
    {
        found = TRUE;
        act("$n looks more like $mself again.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("charm person")))
    {
        found = TRUE;
        act("$n regains $s free will.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
 
    if (check_dispel(level,victim,skill_lookup("chill touch")))
    {
        found = TRUE;
        act("$n looks warmer.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if (check_dispel(level,victim,skill_lookup("honor guard")))
    {
        found = TRUE;
	act("The shield of honor around $n vanishes.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    if (check_dispel(level,victim,skill_lookup("aura of cthon")))
    {
        found = TRUE;
        act("The aura of {RCthon{x around $n vanishes.",victim,NULL,NULL,TO_ROOM,FALSE);
    }
    if (check_dispel(level,victim,skill_lookup("confusion")))
    {
        found = TRUE;
        act("$n doesn't look boggled.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

 
    if (check_dispel(level,victim,skill_lookup("curse")))
        found = TRUE;
 
    if (check_dispel(level,victim,skill_lookup("detect evil")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("detect good")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("detect hidden")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("detect invis")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("detect magic")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("earthbind")))
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("faerie fog")))
    {
        act("$n's purple cloud fades.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("faerie fire")))
    {
        act("$n's outline fades.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("fly")))
    {
        act("$n falls to the ground!",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("frenzy")))
    {
        act("$n no longer looks so wild.",victim,NULL,NULL,TO_ROOM,FALSE);;
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

  if (check_dispel(level,victim,skill_lookup("fumble")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("giant strength")))
    {
        act("$n no longer looks so mighty.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("haste")))
    {
        act("$n is no longer moving so quickly.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("infravision")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("invis")))
    {
        act("$n fades into existance.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("irradiate")))
	found = TRUE;

    if (check_dispel(level,victim,skill_lookup("mass invis")))
    {
        act("$n fades into existance.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("pass door")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }


    if (check_dispel(level,victim,skill_lookup("protection evil")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("protection neutral")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (check_dispel(level,victim,skill_lookup("protection good")))
    {
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("sanctuary")))
    {
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }

    if (IS_NPC(victim) && IS_AFFECTED(victim,AFF_SANCTUARY) 
  && !saves_dispel(level, victim->level,-1)
  && !is_affected(victim,skill_lookup("sanctuary")))
    {
  REMOVE_BIT(victim->affected_by,AFF_SANCTUARY);
        act("The white aura around $n's body vanishes.",
            victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("shield")))
    {
        act("The shield protecting $n vanishes.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("sleep")))
        found = TRUE;

    if (check_dispel(level,victim,skill_lookup("slow")))
    {
        act("$n is no longer moving so slowly.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("stone skin")))
    {
        act("$n's skin regains its normal texture.",victim,NULL,NULL,TO_ROOM,FALSE);
	if(number_percent() > lower_level) level--;
        found = TRUE;
    }
 
    if (check_dispel(level,victim,skill_lookup("weaken")))
    {
        act("$n looks stronger.",victim,NULL,NULL,TO_ROOM,FALSE);
        found = TRUE;
    }
 
    if (found)
    {
        send_to_char("Ok.\n\r",ch);

    if(check_annointment(victim, ch))
    {
        spell_dispel_magic(sn,level+2,victim,ch,target);
    }   

     /* Gladiator Spectator Channel */ 
     if (IS_SET(ch->mhs,MHS_GLADIATOR))
     {
       sprintf(buf,"%s invokes a red aura around %s, removing his protections.",ch->name,victim->name);
       gladiator_talk(buf);
     }
    }
    else
    {
        send_to_char("Spell failed.\n\r",ch);
     /* Gladiator Spectator Channel */ 
     if (IS_SET(ch->mhs,MHS_GLADIATOR) && number_percent() < 50)
     {
       sprintf(buf,"%s fails to invoke a destructive aura on %s.",ch->name,victim->name);
       gladiator_talk(buf);
     }
    }
  return;
}

void spell_earthquake( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;

    send_to_char( "The earth trembles beneath your feet!\n\r", ch );
    act( "$n makes the earth tremble and shiver.", ch, NULL, NULL, TO_ROOM ,FALSE);

    for ( vch = char_list; vch != NULL; vch = vch_next )
    {
  vch_next  = vch->next;
  if ( vch->in_room == NULL )
      continue;
  if ( vch->in_room == ch->in_room )
  {
      if ( vch != ch && !is_safe_spell(ch,vch,TRUE, sn))
      {
    if (IS_AFFECTED(vch,AFF_FLYING))
        damage(ch,vch,0,sn,DAM_BASH,TRUE,TRUE);
    else
        damage( ch,vch,level + dice(2, 8), sn, DAM_BASH,TRUE,TRUE);
      }
      continue;
  }

  if(IS_SET(ch->in_room->room_flags, ROOM_ISOLATED))// New isolated code
    continue;

  if ( vch->in_room->area == ch->in_room->area &&
    !IS_SET(vch->in_room->room_flags,ROOM_ISOLATED))// New isolated code
      send_to_char( "The earth trembles and shivers.\n\r", vch );
    }

    return;
}

void spell_warp( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char size[MAX_INPUT_LENGTH];
    char blah[MAX_INPUT_LENGTH];
    int old_size,factor;

    int perc;

    if (obj->item_type != ITEM_ARMOR)
    {
	send_to_char("That isn't an armor.\n\r",ch);
	return;
    }

    if(obj->link_name && ch->pcdata && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    {
      send_to_char("Turn off safelink if you want to risk destroying your linked armor.\n\r", ch);
      return;
    }

    if (obj->wear_loc != -1)
    {
	send_to_char("The item must be carried to be warped.\n\r",ch);
	return;
    }

    /* item destroyed? */
    if (IS_SET(obj->extra_flags,ITEM_WARPED))
    {
	level /= 2;
	level -= ( ++obj->warps * 10 );
    }

    perc = number_percent();

    if (perc < (obj->level - level) && perc < 95)
    {/* 5% success even at worst case */
	act("$p wavers... and crumbles to dust!",ch,obj,NULL,TO_CHAR,FALSE);
	act("$p wavers... and crumbles to dust!",ch,obj,NULL,TO_ROOM,FALSE);
	extract_obj(obj);
	return;
    }

    target_name = one_argument( target_name, blah );
    target_name = one_argument( target_name, size );

    if ( (old_size = obj->value[4]) == 0 )
	old_size = 3;

    switch ( UPPER(size[0]) )
    {
        case 'O':
	  obj->value[4] = 0;
	  break;

	case 'T':
	  obj->value[4] = 1;
	  break;

	case 'S':
	  obj->value[4] = 2;
	  break;

	case 'M':
	  obj->value[4] = 3;
	  break;

	case 'L':
	  obj->value[4] = 4;
	  break;

	case 'H':
	  obj->value[4] = 5;
	  break;

	case 'G':
	  obj->value[4] = 6;
	  break;

	default:
	  send_to_char("That's not a valid size.\n\r",ch);
	  return;
    }

    /** We want to change the object's weight as well, so that
     ** making something smaller makes it lighter (for elves, etc).
     * Change weight by +- 10% per size difference 
     * Using a new variable for the sake of clarity 
     * Also remove object from char and re-assign it to fix weights */
    
    obj_from_char( obj );
    factor = 10 * (obj->value[4] - old_size);
    obj->weight =  UMAX(1,( ( 100 + factor ) * obj->weight ) / 100);
    obj_to_char( obj, ch );

    SET_BIT(obj->extra_flags,ITEM_WARPED);
    act("$n warps $p to a new size.",ch,obj,NULL,TO_ROOM,FALSE);
    act("You warp $p to a new size.",ch,obj,NULL,TO_CHAR,FALSE);

    return;
}

void spell_enchant_armor( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf; 
    int result, fail;
    int ac_bonus, added;
    bool ac_found = FALSE;

    if (obj->item_type != ITEM_ARMOR)
    {
  send_to_char("That isn't an armor.\n\r",ch);
  return;
    }

    if(obj->link_name && ch->pcdata && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    {
      send_to_char("Turn off safelink if you want to risk destroying your linked armor.\n\r", ch);
      return;
    }

    if (obj->wear_loc != -1)
    {
  send_to_char("The item must be carried to be enchanted.\n\r",ch);
  return;
    }

    /* this means they have no bonus */
    ac_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
            if ( paf->location == APPLY_AC )
            {
        ac_bonus = paf->modifier;
    ac_found = TRUE;
        fail += 5 * (ac_bonus * ac_bonus);
      }

      else  /* things get a little harder */
        fail += 20;
      }
 
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
  if ( paf->location == APPLY_AC )
    {
      ac_bonus = paf->modifier;
      ac_found = TRUE;
      fail += 5 * (ac_bonus * ac_bonus);
  }

  else /* things get a little harder */
      fail += 20;
    }

    /* apply other modifiers */
    fail -= level;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
  fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
  fail -= 5;

    if (HAS_KIT(ch,"enchanter"))
  fail -= 30;
    if (number_percent() < get_skill(ch,gsn_spellcraft) )
  fail -= ( get_skill(ch, gsn_spellcraft) / 7 );

  fail += ( 90 - get_skill(ch,sn) );

   /* Magelabs */
   if(!IS_NPC(ch)&&( ch->class == class_lookup("mage") || 
		     ch->class == class_lookup("wizard") ) )
   {
       int factor = 0;
       switch( ch->in_room->sector_type )
       {
       case SECT_MAGELAB_SIMPLE: factor = 20; break;
       case SECT_MAGELAB_INTERMEDIATE: factor = 40; break;
       case SECT_MAGELAB_ADVANCED:
       case SECT_MAGELAB_SUPERIOR:
       case SECT_MAGELAB_INCREDIBLE:
         factor = 70; break;
       default: break;
       }
       if ( HAS_KIT(ch,"enchanter") ) factor *= 2;
       fail -= factor;
   }

   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("mage")) ) 
	fail += 25;

    fail = URANGE(5,fail,HAS_KIT(ch,"enchanter") ? 75 : 85);

    result = number_percent();

    /* the moment of truth */
    if (result < (fail / 5))  /* item destroyed */
    {
  act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p flares blindingly... and evaporates!",ch,obj,NULL,TO_ROOM,FALSE);
  extract_obj(obj);
  return;
    }

    if (result < (fail / 3)) /* item disenchanted */
    {
  AFFECT_DATA *paf_next;

  act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM,FALSE);
  obj->cost = obj->cost/2;
  obj->enchanted = TRUE;

  /* remove all affects */
  for (paf = obj->affected; paf != NULL; paf = paf_next)
  {
      paf_next = paf->next; 
      affect_remove_obj( obj, paf );
  }
  obj->affected = NULL;

  /* clear all flags */
  obj->extra_flags = 0;
  return;
    }

    if ( result <= fail )  /* failed, no bad result */
    {
  send_to_char("Nothing seemed to happen.\n\r",ch);
  return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
  AFFECT_DATA *af_new;
  obj->enchanted = TRUE;

  for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
  {
      af_new = new_affect();
  
      af_new->next = obj->affected;
      obj->affected = af_new;

      af_new->where = paf->where;
      af_new->type  = UMAX(0,paf->type);
      af_new->level = paf->level;
      af_new->duration  = paf->duration;
      af_new->location  = paf->location;
      af_new->modifier  = paf->modifier;
      af_new->bitvector = paf->bitvector;
  }
    }

    if (result <= (90 - level/5))  /* success! */
    {
  act("$p shimmers with a gold aura.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p shimmers with a gold aura.",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags, ITEM_MAGIC);
  added = -1;
    }
    
    else if ( !HAS_KIT(ch,"enchanter") ||
		number_percent() > level / 4)
    {
  act("$p glows a {Ybrilliant{x gold!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows a {Ybrilliant{x gold!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
  added = -2;
    }

    else
    {
  act("$p glows with a {YBLINDING gold{x aura!!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows with a {YBLINDING gold{x aura!!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
  added = -4;
    }

    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO)
  obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (ac_found)
    {
  for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
      if ( paf->location == APPLY_AC)
      {
    paf->type = sn;
    paf->modifier += added;
    paf->level = UMAX(paf->level,level);
      }
  }
    }
    else /* add a new affect */
    {
  paf = new_affect();

  paf->where  = TO_OBJECT;
  paf->type = sn;
  paf->level  = level;
  paf->duration = -1;
  paf->location = APPLY_AC;
  paf->modifier =  added;
  paf->bitvector  = 0;
      paf->next = obj->affected;
      obj->affected = paf;
    }
  set_rarity(obj);
}




void spell_enchant_weapon(int sn,int level,CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA *paf; 
    int result, fail;
    int hit_bonus, dam_bonus, added=0;
    bool hit_found = FALSE, dam_found = FALSE;
    bool enchanter = HAS_KIT(ch, "enchanter");

    if (obj->item_type != ITEM_WEAPON)
    {
  send_to_char("That isn't a weapon.\n\r",ch);
  return;
    }

    if(obj->link_name && ch->pcdata && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOSAFELINK))
    {
      send_to_char("Turn off safelink if you want to risk destroying your linked weapon.\n\r", ch);
      return;
    }

    if (obj->wear_loc != -1)
    {
  send_to_char("The item must be carried to be enchanted.\n\r",ch);
  return;
    }

    /* this means they have no bonus */
    hit_bonus = 0;
    dam_bonus = 0;
    fail = 25;  /* base 25% chance of failure */

    /* find the bonuses */

    if (!obj->enchanted)
    {
      for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
      {
            if ( paf->location == APPLY_HITROLL )
            {
		hit_bonus = paf->modifier;
		hit_found = TRUE;
		fail += (hit_bonus * hit_bonus);
      	    }

            if (paf->location == APPLY_DAMROLL )
      	    {
        	dam_bonus = paf->modifier;
    		dam_found = TRUE;
        	fail += (dam_bonus * dam_bonus);
            }
      }
    }
    else  /* object is enchanted, a bit tougher */
    {
      for ( paf = obj->affected; paf != NULL; paf = paf->next )
      {
	if ( paf->location == APPLY_HITROLL )
	{
		hit_bonus = paf->modifier;
		hit_found = TRUE;
		fail += (hit_bonus * hit_bonus);
	}

	if (paf->location == APPLY_DAMROLL )
	{
		dam_bonus = paf->modifier;
		dam_found = TRUE;
		fail += (dam_bonus * dam_bonus);
	}

	fail += 25; /* because it's enchanted */
      }
    }

    /* apply other modifiers */
    fail -= 3 * level/2;

    if (IS_OBJ_STAT(obj,ITEM_BLESS))
  fail -= 15;
    if (IS_OBJ_STAT(obj,ITEM_GLOW))
  fail -= 5;

    if (IS_WEAPON_STAT(obj,WEAPON_FAVORED))
  fail -= 25;

    if (HAS_KIT(ch,"enchanter") )
  fail -= 30;

  /* reward good enchant skills */
  fail += ( 90 - get_skill(ch,sn) );

    if ( number_percent() < get_skill(ch,gsn_spellcraft) )
  fail -= ( get_skill(ch,gsn_spellcraft) / 10 );

   /* Magelabs */
  if(!IS_NPC(ch)&&( ch->class == class_lookup("mage") || 
                    ch->class == class_lookup("wizard") ) )
  {
	int factor = 0;
     switch( ch->in_room->sector_type )
    {
   case SECT_MAGELAB_SIMPLE: factor = 15; break;
   case SECT_MAGELAB_INTERMEDIATE: factor = 30; break;
   case SECT_MAGELAB_ADVANCED: 
   case SECT_MAGELAB_SUPERIOR:
   case SECT_MAGELAB_INCREDIBLE:
      if(enchanter)
	factor = 60;
      else
	factor = 30;// Non-enchanters can't use this to its best
      break;
   default: break;
    }
    if (enchanter) factor *= 2;
    fail -= factor;
  }

   if(!IS_NPC(ch)&&( ch->pcdata->old_class != class_lookup("mage")) ) 
		   fail += 20;


    fail = URANGE(5,fail,HAS_KIT(ch,"enchanter") ? 85 : 95);
   
    result = number_percent();

//    if( IS_IMMORTAL(ch))// Debug code
/*    {
	char buf[256];
        sprintf(buf, "Fail: %d result: %d hit: %d dam: %d Resist: %d\n\r", fail, result, hit_bonus, dam_bonus, IS_SET(obj->value[4],WEAPON_RESIST_ENCHANT));
    send_to_char(buf, ch);
    }// TEST TEST TEST
*/
    if( IS_IMMORTAL(ch)) fail = 0;

    /* the moment of truth */
    if (result < (fail / 5) 
	|| (hit_found && hit_bonus > 15)
	|| (dam_found && dam_bonus > 15) 
 	|| ( IS_SET(obj->value[4],WEAPON_RESIST_ENCHANT) &&
	     number_percent() > level && !IS_IMMORTAL(ch) ) )  /* item destroyed */
    {
  act("$p shivers violently and explodes!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p shivers violently and explodes!",ch,obj,NULL,TO_ROOM,FALSE);
  extract_obj(obj);
  return;
    }

    if (result < (fail / 2)) /* item disenchanted */
    {
  AFFECT_DATA *paf_next;

  act("$p glows brightly, then fades...oops.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows brightly, then fades.",ch,obj,NULL,TO_ROOM,FALSE);
  obj->enchanted = TRUE;
  obj->cost = obj->cost/2;


  /* remove all affects */
  for (paf = obj->affected; paf != NULL; paf = paf_next)
  {
      paf_next = paf->next; 
      affect_remove_obj( obj, paf );
  }
  obj->affected = NULL;

  /* clear all flags */
  obj->extra_flags = 0;
  return;
    }
    if ( result <= fail )  /* failed, no bad result */
    {
  send_to_char("Nothing seemed to happen.\n\r",ch);
  return;
    }

    /* okay, move all the old flags into new vectors if we have to */
    if (!obj->enchanted)
    {
  AFFECT_DATA *af_new;
  obj->enchanted = TRUE;

  for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) 
  {
      af_new = new_affect();
  
      af_new->next = obj->affected;
      obj->affected = af_new;

      af_new->where = paf->where;
      af_new->type  = UMAX(0,paf->type);
      af_new->level = paf->level;
      af_new->duration  = paf->duration;
      af_new->location  = paf->location;
      af_new->modifier  = paf->modifier;
      af_new->bitvector = paf->bitvector;
  }
    }
/*{
char buf[256];
sprintf(buf, "%d resist %d chance, %d random %d second chance\n\r", result,  (level/(enchanter?5:7)), fail, (enchanter?8:5));
send_to_char(buf, ch);// TEST TEST TEST
}*/
    if ((result > (level/(enchanter?5:7)) && number_percent() > (enchanter?4:3))
	|| IS_IMMORTAL(ch))  /* success! */
    {
  act("$p glows blue.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows blue.",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags, ITEM_MAGIC);
  added = 1;
    }
    else
    {
     if (number_percent() < level / 2 
	&& (enchanter && number_percent() < 40) )
     {
  act("$p glows with a {BBLINDING blue{x aura!!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows with a {BBLINDING blue{x aura!!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
  added = 3;
     }
     else
     {
  act("$p glows a {Bbrilliant{x blue!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows a {Bbrilliant{x blue!",ch,obj,NULL,TO_ROOM,FALSE);
  SET_BIT(obj->extra_flags,ITEM_MAGIC);
  SET_BIT(obj->extra_flags,ITEM_GLOW);
  added = 2;
     }
    }
    
    /* now add the enchantments */ 

    if (obj->level < LEVEL_HERO - 1)
  obj->level = UMIN(LEVEL_HERO - 1,obj->level + 1);

    if (dam_found)
    {
  for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
      if ( paf->location == APPLY_DAMROLL)
      {
    paf->type = sn;
    paf->modifier += added;
    paf->level = UMAX(paf->level,level);
    if (paf->modifier > 4)
        SET_BIT(obj->extra_flags,ITEM_HUM);
      }
  }
    }
    else /* add a new affect */
    {
  paf = new_affect();

  paf->where  = TO_OBJECT;
  paf->type = sn;
  paf->level  = level;
  paf->duration = -1;
  paf->location = APPLY_DAMROLL;
  paf->modifier =  added;
  paf->bitvector  = 0;
      paf->next = obj->affected;
      obj->affected = paf;
    }

    if (hit_found)
    {
        for ( paf = obj->affected; paf != NULL; paf = paf->next)
  {
            if ( paf->location == APPLY_HITROLL)
            {
    paf->type = sn;
                paf->modifier += added;
                paf->level = UMAX(paf->level,level);
                if (paf->modifier > 4)
                    SET_BIT(obj->extra_flags,ITEM_HUM);
            }
  }
    }
    else /* add a new affect */
    {
        paf = new_affect();
 
        paf->type       = sn;
        paf->level      = level;
        paf->duration   = -1;
        paf->location   = APPLY_HITROLL;
        paf->modifier   =  added;
        paf->bitvector  = 0;
        paf->next       = obj->affected;
        obj->affected   = paf;
    }

  set_rarity(obj);
}



/*
 * Drain XP, MANA, HP.
 * Caster gains HP.
 */
void spell_energy_drain( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    if ( (victim != ch && !is_affected(ch, skill_lookup("indulgence")))  || ( victim != ch && ch->kit != kit_lookup("necromancer")) )
	{
	if(IS_EVIL(ch))
         ch->alignment = UMAX(-1000, ch->alignment - 50);
	if(IS_GOOD(ch))
	 ch->alignment = UMIN(1000, ch->alignment +50);
	if(IS_NEUTRAL(ch))
	 ch->alignment = 0;
	}

    if ( saves_spell( level, victim,DAM_NEGATIVE) )
    {
  send_to_char("You feel a momentary chill.\n\r",victim);   
  return;
    }


    if ( victim->level <= 2 )
    {
  dam    = ch->hit + 1;
    }
    else
    {
  gain_exp( victim, 0 - number_range( level/2, 3 * level / 2 ) );
  if ( !saves_spell( level, victim, DAM_NEGATIVE) )
  	victim->mana  /= 2;
  else
	victim->mana = ( 3 * victim->mana ) / 4;

  if ( !saves_spell( level, victim, DAM_NEGATIVE) )
	victim->move  /= 2; 
  else
	victim->move = ( 3 * victim->move ) / 4;

  dam    = dice(1, level);
  ch->hit   += dam;
    }

    send_to_char("You feel your life slipping away!\n\r",victim);
    send_to_char("Wow....what a rush!\n\r",ch);
    damage( ch, victim, dam, sn, DAM_NEGATIVE ,TRUE,TRUE);

    if(check_annointment(victim, ch))
    {
        spell_energy_drain(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_LESSER_DAMAGE);

    return;
}



void spell_fireball( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
    0,
    0,   0,   0,   0,   0,    0,   0,   0,   0,   0,
    0,   0,   0,   0,  30,   35,  40,  45,  50,  55,
   60,  65,  70,  75,  80,   82,  84,  86,  88,  90,
   92,  94,  96,  98, 100,  102, 104, 106, 108, 110,
  112, 114, 116, 118, 120,  122, 124, 126, 128, 130
    };
    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim, DAM_FIRE) )
  dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE ,TRUE,TRUE);
    return;
}


void spell_fireproof(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    AFFECT_DATA af;
 
    if (IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
    {
        act("$p is already protected from burning.",ch,obj,NULL,TO_CHAR,FALSE);
        return;
    }
 
    af.where     = TO_OBJECT;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy(level / 3);
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = ITEM_BURN_PROOF;
 
    affect_to_obj(obj,&af);
 
    act("You protect $p from fire.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$p is surrounded by a protective aura.",ch,obj,NULL,TO_ROOM,FALSE);
}



void spell_flamestrike( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice(6 + level / 2, 8);
    if ( saves_spell( level, victim,DAM_FIRE) )
  dam /= 2;
    damage( ch, victim, dam, sn, DAM_FIRE ,TRUE,TRUE);
    return;
}



void spell_faerie_fire( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FAERIE_FIRE) || 
	 saves_spell(level+5,victim,DAM_FIRE) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }
    
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = 2 * level;
    af.bitvector = AFF_FAERIE_FIRE;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a pink outline.\n\r", victim );
    act( "$n is surrounded by a pink outline.", victim, NULL, NULL, TO_ROOM ,FALSE);

    if(check_annointment(victim, ch))
    {
	spell_faerie_fire(sn,level+2,victim,ch,target);
    }

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



void spell_faerie_fog( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *ich;
    AFFECT_DATA af;

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
	ch->in_room == get_room_index(ROOM_VNUM_SINGLE_GLADIATOR))
    {
	send_to_char("No casting Faerie Fog in Prep Room!\n\r",ch);
	return;
    }
    level +=2;

    act( "$n conjures a cloud of purple smoke.", ch, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You conjure a cloud of purple smoke.\n\r", ch );

   for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room )
   {
      if (ich->invis_level > 0)
         continue;

      if ( ich == ch || saves_spell( level, ich,DAM_OTHER) )
         continue;
  
      if (!is_safe_spell(ch,ich,TRUE, sn))
      {
         affect_strip ( ich, gsn_invis     );
         affect_strip ( ich, gsn_mass_invis    );
         affect_strip ( ich, gsn_sneak     );
         REMOVE_BIT   ( ich->affected_by, AFF_HIDE );
         REMOVE_BIT   ( ich->affected_by, AFF_INVISIBLE  );
         REMOVE_BIT   ( ich->affected_by, AFF_SNEAK  );
         REMOVE_BIT   ( ich->mhs, MHS_FADE ); 
         act( "$n is revealed!", ich, NULL, NULL, TO_ROOM ,FALSE);
         send_to_char( "You are revealed!\n\r", ich );

         if ( IS_AFFECTED(ich, AFF_FAERIE_FOG) )
  	    continue;

         af.where     = TO_AFFECTS;
         af.type      = sn;
         af.level   = level;
         af.duration  = level/12;
         af.location  = APPLY_NONE;
         af.modifier  = 0;
         af.bitvector = AFF_FAERIE_FOG;
         affect_to_char( ich, &af );
    apply_mala_damage(ch, ich, MALA_LESSER_DAMAGE);
      } 
    }

    return;
}

void spell_famine( int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

/*
    if ( ( victim != ch ) && IS_SET(ch->act,PLR_NOCANCEL) &&
	saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }
    */

    if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.bet_counter <= 0 && gladiator_info.exper != TRUE)
    {
       send_to_char(" Gladiators can not famine.\n\r",ch);
       return;
    }

/* Adding in so people just dont famine others without worry */
    if( is_safe(ch,victim) && !is_same_group(ch,victim) 
        && !is_same_clan(ch,victim) && !IS_NPC(ch) 
        && (!is_clan(ch) && IS_SET (victim->act,PLR_NOCANCEL)))
    {
	send_to_char("You are not permitted to cast on them.\n\r",ch);
	return;
    }
    else
       if( is_clan(ch) && is_clan(victim) && !is_same_group(ch,victim)
           && !is_same_clan(ch,victim) )
          check_killer(ch,victim);

    if(!is_same_group(ch,victim) && ch != victim &&
        saves_spell(level,victim,DAM_OTHER))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    gain_condition(victim,COND_HUNGER,-1 * level / 2);
    gain_condition(victim,COND_FULL,-1 * level / 2 );
    gain_condition(victim,COND_THIRST,-1 * level / 2 );

    send_to_char("You are absolutely famished!\n\r",victim);

    if ( victim != ch )
    act("$N appears to be famished!",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_famine(sn,level+2,victim,ch,target);
    }   

    return;
}

void spell_feast(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

/*
    if ( (victim != ch) && IS_SET(victim->act,PLR_NOCANCEL) &&
	 saves_spell(level,victim,DAM_OTHER) )
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }
    */

    if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.bet_counter <= 0 && gladiator_info.exper != TRUE)
    {
       send_to_char(" Gladiators can not feast.\n\r",ch);
       return;
    }

/* Adding in so people just dont feast others without worry */
    if( is_safe(ch,victim) && !is_same_group(ch,victim) 
        && !is_same_clan(ch,victim) && !IS_NPC(ch) 
        && (!is_clan(ch) && IS_SET (victim->act,PLR_NOCANCEL)))
    {
	send_to_char("You are not permitted to cast on them.\n\r",ch);
	return;
    }
    else
       if( is_clan(ch) && is_clan(victim) && !is_same_group(ch,victim)
           && !is_same_clan(ch,victim) )
          check_killer(ch,victim);

    if(!is_same_group(ch,victim) && ch != victim &&
        saves_spell(level,victim,DAM_OTHER))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    gain_condition(victim,COND_HUNGER, level / 2 );
    gain_condition(victim,COND_FULL, level / 2 );
    gain_condition(victim,COND_THIRST, level / 2 );

    if(!IS_NPC(victim) && victim->pcdata->condition[COND_FULL] > 40)
      send_to_char("You feel full.\n\r", victim);
    else
      send_to_char("You feel nourished.\n\r",victim);

    if ( victim != ch )
    act("$N appears to be nourished.",ch,NULL,victim,TO_CHAR,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_feast(sn,level+2,victim,ch,target);
    }   

    return;
}

void spell_floating_disc( int sn, int level,CHAR_DATA *ch,void *vo,int target )
{
    OBJ_DATA *disc, *floating;

    floating = get_eq_char(ch,WEAR_FLOAT);
    if (floating != NULL && IS_OBJ_STAT(floating,ITEM_NOREMOVE))
    {
  act("You can't remove $p.",ch,floating,NULL,TO_CHAR,FALSE);
  return;
    }

    disc = create_object(get_obj_index(OBJ_VNUM_DISC), 0, FALSE );
    disc->value[0]  = ch->level * 10; /* 10 pounds per level capacity */
    disc->value[3]  = ch->level * 5; /* 5 pounds per level max per item */
    disc->timer   = ch->level * 2 - number_range(0,level / 2); 

    act("$n has created a floating black disc.",ch,NULL,NULL,TO_ROOM,FALSE);
    send_to_char("You create a floating disc.\n\r",ch);
    obj_to_char(disc,ch);
    wear_obj(ch,disc,TRUE);
    return;
}


void spell_fly( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_FLYING) )
    {
    	if(reup_affect(victim,sn,level+3,level))
	{
    send_to_char( "Your feet rise off the ground.\n\r", victim );
    if ( ch != victim )
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM ,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level + 3;
    af.location  = 0;
    af.modifier  = 0;
    af.bitvector = AFF_FLYING;
    affect_to_char( victim, &af );
    send_to_char( "Your feet rise off the ground.\n\r", victim );
    act( "$n's feet rise off the ground.", victim, NULL, NULL, TO_ROOM ,FALSE);
    return;
}

/* RT clerical berserking spell */

void spell_frenzy(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (is_affected(victim,sn) || 
    ( IS_AFFECTED(victim,AFF_BERSERK) && 
      victim->race != race_lookup("dwarf") ) )
    {
  if (victim == ch)
    send_to_char("You are already in a frenzy.\n\r",ch);
  else
    act("$N is already in a frenzy.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (is_affected(victim,skill_lookup("calm")))
    {
  if (victim == ch)
    send_to_char("Why don't you just relax for a while?\n\r",ch);
  else
    act("$N doesn't look like $e wants to fight anymore.",
        ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (((IS_GOOD(ch) && !IS_GOOD(victim)) ||
     (IS_NEUTRAL(ch) && !IS_NEUTRAL(victim)) ||
     (IS_EVIL(ch) && !IS_EVIL(victim))) && 
     victim->level > 5 && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_IS_HEALER)))
    {
  act("Your god doesn't seem to like $N",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    af.where     = TO_AFFECTS;
    af.type    = sn;
    af.level   = level;
    af.duration  = level / 3;
    af.modifier  = level / 6;
    af.bitvector = 0;

    af.location  = APPLY_HITROLL;
    affect_to_char(victim,&af);

    af.location  = APPLY_DAMROLL;
    affect_to_char(victim,&af);

    af.modifier  = 10 * (level / 12);
    af.location  = APPLY_AC;
    affect_to_char(victim,&af);

    send_to_char("You are filled with holy wrath!\n\r",victim);
    act("$n gets a wild look in $s eyes!",victim,NULL,NULL,TO_ROOM,FALSE);
}

/* RT ROM-style gate */
    
void spell_gate( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    bool gate_pet;
    CHAR_DATA *fch=NULL;
    CHAR_DATA *fch_next;
    bool fHighlander = FALSE;

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
    ||   victim->in_room->vnum < 0
    ||   !can_see_room(ch,victim->in_room)
    ||	 !is_room_clan(ch,victim->in_room)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   IS_SET(ch->in_room->room_flags, ROOM_NO_RECALL)
    ||   (victim->level >= ch->level + 3 && !is_same_clan(ch,victim) && !is_same_group(ch,victim))
    ||   (is_clan(victim) && !is_same_clan(ch,victim) && !is_same_group(ch,victim))
    ||   (!is_clan(victim) && is_clan(ch) && IS_SET(victim->act,PLR_NOSUMMON) )
    ||   (!IS_NPC(victim) && victim->level > LEVEL_HERO)  /* NOT trust */ 
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||	 (IS_NPC(victim) && 
          (IS_SET(victim->affected_by,AFF_CHARM) && victim->leader != ch))
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_OTHER) ) 
    ||   (!is_room_owner(ch,victim->in_room) && room_is_private(ch,victim->in_room) ) 
    ||  victim->in_room->area->under_develop 
    ||  victim->in_room->area->no_transport 
    ||  ch->in_room->area->no_transport
    ||  is_bounty_target(victim, FALSE) )
    {
        send_to_char( "You failed.\n\r", ch );
        return;
    } 
    if (ch->pet != NULL && ch->in_room == ch->pet->in_room)
  gate_pet = TRUE;
    else
  gate_pet = FALSE;
    
    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not gate.\n\r",ch);
       return;
    }

    if(is_clan(ch) && is_clan(victim) && ch->pcdata->start_time > 0)
    {
      send_to_char("You just got here, you must wait before gating to them.\n\r", ch);
      return;
    }

    act("$n steps through a gate and vanishes.",ch,NULL,NULL,TO_ROOM,FALSE);
    send_to_char("You step through a gate and vanish.\n\r",ch);

    if (IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
      for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
      {
         fch_next = fch->next_in_room;
         if (IS_SET(fch->mhs,MHS_HIGHLANDER) && fch != ch)
         {
	    fHighlander = TRUE;
	    send_to_char("The tingle in your neck stops and the presence of the other Highlander is gone.\n\r",fch);
	 }
      }
    }

    /*Ogre's Smell Remorts Leaving The Room */
    if (!IS_NPC(ch) && (IS_SET(ch->act,PLR_VAMP) ||
	                IS_SET(ch->act,PLR_MUMMY) ||
	                IS_SET(ch->act,PLR_WERE) ) )  
    {
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
	  fch_next = fch->next_in_room;
	  if (fch->race == race_lookup("ogre") && fch != ch ) 
	  {
	     if (number_percent() < (fch->level + get_curr_stat(fch,STAT_CON))) 
	     {
	        if(IS_SET(ch->act,PLR_VAMP))
	           send_to_char("The strange odor of blood is gone.\n\r",fch);
	        if(IS_SET(ch->act,PLR_WERE))
	           send_to_char("The scent of decay is gone.\n\r",fch);
	        if(IS_SET(ch->act,PLR_MUMMY))
	           send_to_char("The smell of filthy fur is gone.\n\r",fch);
	     }
	  }
       }
    }

    char_from_room(ch);
    clear_mount(ch);
    char_to_room(ch,victim->in_room);
    act("$n has arrived through a gate.",ch,NULL,NULL,TO_ROOM,FALSE);
    do_look(ch,"auto");

    if (IS_SET(ch->mhs,MHS_HIGHLANDER) && fch != NULL)
    {
       if (fHighlander)
       {
	  send_to_char("The tingle in your neck stops and the presence of the other Highlander is gone.\n\r",fch);
	  fHighlander = FALSE;
       } 
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
          fch_next = fch->next_in_room;
	  if (IS_SET(fch->mhs,MHS_HIGHLANDER) && fch != ch)
	  {
             send_to_char("Your neck tingles as you feel the presence of another Highlander.\n\r",fch);
	     fHighlander = TRUE;
          }
       }
       if (fHighlander)
       {
	  send_to_char("Your neck tingles as you feel the presence of another Highlander.\n\r",fch);
       }
    }

    /*Ogre's Smell Remorts Entering The Room */
    if (!IS_NPC(ch) && (IS_SET(ch->act,PLR_VAMP) ||
	                IS_SET(ch->act,PLR_MUMMY) ||
	                IS_SET(ch->act,PLR_WERE) ) )  
    {
       for (fch = ch->in_room->people; fch != NULL; fch = fch_next)
       {
	  fch_next = fch->next_in_room;
          if (fch->race == race_lookup("ogre") && fch != ch) 
          {
	     if (number_percent() < (fch->level + get_curr_stat(fch,STAT_CON))) 
	     {
                if(IS_SET(ch->act,PLR_VAMP))
                   send_to_char("The strange odor of blood makes you feel uncomfortable.\n\r",fch);
                if(IS_SET(ch->act,PLR_WERE))
                   send_to_char("The scent of decay makes your head dizzy.\n\r",fch);
                if(IS_SET(ch->act,PLR_MUMMY))
                   send_to_char("The smell of filthy fur fills up the air.\n\r",fch);
	     }
          }
       }
    }

    if (gate_pet)
    {
  act("$n steps through a gate and vanishes.",ch->pet,NULL,NULL,TO_ROOM,FALSE);
  send_to_char("You step through a gate and vanish.\n\r",ch->pet);
  char_from_room(ch->pet);
  clear_mount(ch->pet);
  char_to_room(ch->pet,victim->in_room);
  act("$n has arrived through a gate.",ch->pet,NULL,NULL,TO_ROOM,FALSE);
  do_look(ch->pet,"auto");
    }
}



void spell_giant_strength(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	reup_affect(victim,sn,level,level);
	send_to_char( "Your muscles surge with heightened power!\n\r", victim );
	act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM,FALSE);
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = level;
    af.location  = APPLY_STR;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "Your muscles surge with heightened power!\n\r", victim );
    act("$n's muscles surge with heightened power.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}



void spell_harm( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

       dam = dice( level, 12);

    if ( saves_spell( level, victim, DAM_HARM ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_HARM,TRUE,TRUE);
    return;
}

/* RT haste spell */

void spell_haste( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim, gsn_hamstring) )
    {
	act("Your spell failed!",ch,NULL,NULL,TO_CHAR,FALSE);
	return;
    }
 
    if( IS_SET(victim->off_flags,OFF_FAST) || is_affected(victim, skill_lookup("speed")))
    {
  if (victim == ch)
    send_to_char("You can't move any faster!\n\r",ch);
  else
    act("$N is already moving as fast as $e can.",
        ch,NULL,victim,TO_CHAR,FALSE);
        return;
    }

    if ( IS_AFFECTED(victim,AFF_HASTE))
    {
	if(reup_affect(victim,sn,level/3,level))
	{
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM,FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }


    if (IS_AFFECTED(victim,AFF_SLOW))
    {
  if (!check_dispel(level,victim,skill_lookup("slow")))
  {
      if (victim != ch)
          send_to_char("Spell failed.\n\r",ch);
      send_to_char("You feel momentarily faster.\n\r",victim);
      return;
  }
        act("$n is moving less slowly.",victim,NULL,NULL,TO_ROOM,FALSE);
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    if (victim == ch)
      af.duration  = level/2;
    else
      af.duration  = level/4;
    af.location  = APPLY_DEX;
    af.modifier  = 1 + (level >= 18) + (level >= 25) + (level >= 32);
    af.bitvector = AFF_HASTE;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself moving more quickly.\n\r", victim );
    act("$n is moving more quickly.",victim,NULL,NULL,TO_ROOM,FALSE);
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}



void spell_heal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    int healed;
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    healed = 100;
    if (!IS_NPC(ch) && ch->race == race_lookup("volare") )
	healed = 3 * healed / 2;

    if ( !IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("cleric") &&
		       ch->pcdata->old_class != class_lookup("elementalist")))
	healed -= ( healed / 5 );

    victim->hit = UMIN( victim->hit + healed, victim->max_hit );
    update_pos( victim );
    send_to_char( "A warm feeling fills your body.\n\r", victim );
    if ( ch != victim )
  send_to_char( "Ok.\n\r", ch );
    return;
}

void spell_heat_metal( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    OBJ_DATA *obj_lose, *obj_next;
    int dam = 0;
    AFFECT_DATA *paf;
    bool fail = TRUE;

  if(IS_NPC(victim))
  {
   if (!saves_spell(level + 2,victim,DAM_FIRE) 
   &&  !IS_SET(victim->imm_flags,IMM_FIRE))
   {
        for ( obj_lose = victim->carrying;
        obj_lose != NULL; 
        obj_lose = obj_next)
        {
      obj_next = obj_lose->next_content;
            if ( number_range(1,2 * level) > obj_lose->level 
      &&   !saves_spell(level,victim,DAM_FIRE)
      &&   !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL)
      &&   !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF))
            {
                switch ( obj_lose->item_type )
                {
                case ITEM_ARMOR:
    if (obj_lose->wear_loc != -1) /* remove the item */
    {
        if (can_drop_obj(victim,obj_lose)
        &&  (obj_lose->weight / 10) < 
      number_range(1,2 * get_curr_stat(victim,STAT_DEX))
        &&  remove_obj( victim, obj_lose->wear_loc, TRUE ))
        {
            act("$n yelps and throws $p to the ground!",
          victim,obj_lose,NULL,TO_ROOM,FALSE);
            act("You remove and drop $p before it burns you.",
          victim,obj_lose,NULL,TO_CHAR,FALSE);
      dam += (number_range(1,obj_lose->level) / 3);
                        obj_from_char(obj_lose);
			if(!IS_NPC(victim) && IS_SET(victim->mhs,MHS_GLADIATOR))
                           obj_to_char( obj_lose, victim ); 
			else
                           obj_to_room(obj_lose, victim->in_room);
			   obj_lose->stolen_timer += 10 * number_fuzzy(5);
                        fail = FALSE;
                    }
        else /* stuck on the body! ouch! */
        {
      act("Your skin is seared by $p!",
          victim,obj_lose,NULL,TO_CHAR,FALSE);
      dam += (number_range(1,obj_lose->level));
      fail = FALSE;
        }

    }
    else /* drop it if we can */
    {
        if (can_drop_obj(victim,obj_lose))
        {
                        act("$n yelps and throws $p to the ground!",
                            victim,obj_lose,NULL,TO_ROOM,FALSE);
                        act("You and drop $p before it burns you.",
                            victim,obj_lose,NULL,TO_CHAR,FALSE);
                        dam += (number_range(1,obj_lose->level) / 6);
                        obj_from_char(obj_lose);
			if(!IS_NPC(victim) && IS_SET(victim->mhs,MHS_GLADIATOR))
                           obj_to_char( obj_lose, victim ); 
			else
                           obj_to_room(obj_lose, victim->in_room);
			   obj_lose->stolen_timer += 10 * number_fuzzy(5);
      fail = FALSE;
                    }
        else /* cannot drop */
        {
                        act("Your skin is seared by $p!",
                            victim,obj_lose,NULL,TO_CHAR,FALSE);
                        dam += (number_range(1,obj_lose->level) / 2);
      fail = FALSE;
                    }
    }
                break;
                case ITEM_WEAPON:
    if (obj_lose->wear_loc != -1) /* try to drop it */
    {
        if (IS_WEAPON_STAT(obj_lose,WEAPON_FLAMING))
      continue;

        if (can_drop_obj(victim,obj_lose) 
        &&  remove_obj(victim,obj_lose->wear_loc,TRUE))
        {
      act("$n is burned by $p, and throws it to the ground.",
          victim,obj_lose,NULL,TO_ROOM,FALSE);
      send_to_char(
          "You throw your red-hot weapon to the ground!\n\r",
          victim);
      dam += 1;
      obj_from_char(obj_lose);
      if(!IS_NPC(victim) && IS_SET(victim->mhs,MHS_GLADIATOR))
         obj_to_char( obj_lose, victim ); 
      else
         obj_to_room(obj_lose,victim->in_room);
	 obj_lose->stolen_timer += 10 * number_fuzzy(5);
      fail = FALSE;
        }
        else /* YOWCH! */
        {
      send_to_char("Your weapon sears your flesh!\n\r",
          victim);
      dam += number_range(1,obj_lose->level);
      fail = FALSE;
        }
    }
                else /* drop it if we can */
                {
                    if (can_drop_obj(victim,obj_lose))
                    {
                        act("$n throws a burning hot $p to the ground!",
                            victim,obj_lose,NULL,TO_ROOM,FALSE);
                        act("You and drop $p before it burns you.",
                            victim,obj_lose,NULL,TO_CHAR,FALSE);
                        dam += (number_range(1,obj_lose->level) / 6);
                        obj_from_char(obj_lose);
                        if(!IS_NPC(victim) && IS_SET(victim->mhs,MHS_GLADIATOR))
                           obj_to_char( obj_lose, victim ); 
                        else
                           obj_to_room(obj_lose, victim->in_room);
			   obj_lose->stolen_timer += 10 * number_fuzzy(5);
                        fail = FALSE;
                    }
                    else /* cannot drop */
                    {
                        act("Your skin is seared by $p!",
                            victim,obj_lose,NULL,TO_CHAR,FALSE);
                        dam += (number_range(1,obj_lose->level) / 2);
                        fail = FALSE;
                    }
                }
                break;
    }
      }
  }
    } 
    if (fail)
    {
        send_to_char("Your spell had no effect.\n\r", ch);
  send_to_char("You feel momentarily warmer.\n\r",victim);
    }
    else /* damage! */
    {
  if (saves_spell(level,victim,DAM_FIRE))
      dam = 2 * dam / 3;
  damage(ch,victim,dam,sn,DAM_FIRE,TRUE,TRUE);

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    }

    if(check_annointment(victim, ch))
    {
        spell_heat_metal(sn,level+2,victim,ch,target);
    }   
  }
  else
  {
  int dam = 0;
  int count = 0;

  if (IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
  {
     send_to_char("Highlanders are immune to that.\n\r",ch);
     return;
  }

 if (!saves_spell(level + 2,victim,DAM_FIRE) 
 &&  !IS_SET(victim->imm_flags,IMM_FIRE))
 {
    for ( obj_lose = victim->carrying;
    obj_lose != NULL; 
    obj_lose = obj_lose->next_content)
    {
      if ( !IS_OBJ_STAT(obj_lose,ITEM_NONMETAL) &&
         !IS_OBJ_STAT(obj_lose,ITEM_BURN_PROOF) &&
         number_range(1,2 * level) > obj_lose->level && 
         !saves_spell(level,victim,DAM_FIRE))
      {
        act("$n yelps as $e is seared by $p!",
          victim,obj_lose,NULL,TO_ROOM,FALSE);
        act("You are seared as $p glows red hot.",
          victim,obj_lose,NULL,TO_CHAR,FALSE);
          dam += (number_range(2,level) / 2);
          count++;
      }
    }
  }
  if (!count)
  {
    send_to_char("Your spell had no effect.\n\r", ch);
    send_to_char("You feel momentarily warmer.\n\r",victim);
  }
  else /* damage! */
  {
    if(ch->level >= 25)
      count *= 2; /* 2 hit per item */
    if (saves_spell(level,victim,DAM_FIRE))
      dam = 2 * dam / 3;
    damage(ch,victim,dam,sn,DAM_FIRE,TRUE,TRUE);

    for ( paf = victim->affected; paf != NULL; paf = paf->next )
    {
      if(paf->type == sn)
      {
  	    paf->duration = UMIN(2, paf->duration + 1);
  	    if(abs(paf->modifier) < count)
  	    {/* Update the hitroll penalty if this hits them harder */
  	      victim->max_hit -= (count - paf->modifier);
  	      paf->modifier = count * -1;
  	    }
  	    break;
      }
    }

    if(!paf)
    {/* Not already present */
      AFFECT_DATA af;
      af.where     = TO_AFFECTS;
      af.type      = sn;
      af.level     = level;
      af.location  = APPLY_HITROLL;
      af.modifier  = count * -1;
      af.duration  = 0;
      af.bitvector = 0;
      affect_to_char( victim, &af );
    }

    if(!IS_NPC(victim) && is_affected(victim, gsn_annointment)
      && victim != ch && number_percent() > 50)
    {
      send_to_char( "The Almighty rebukes your attacker.\n\r", victim ); 
      act( "The Almighty rebukes you for harming $N.",ch,NULL,victim,
        TO_CHAR,FALSE);
      spell_heat_metal(sn,level+2,victim,ch,target);
      affect_strip(victim, gsn_annointment);
      send_to_char("You feel your annointing fade.\n\r", victim);
    }
  }

  }
    return;
}

/* RT really nasty high-level attack spell */
void spell_holy_word(int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam;
    int bless_num, curse_num, frenzy_num;
   
    bless_num = skill_lookup("bless");
    curse_num = skill_lookup("curse"); 
    frenzy_num = skill_lookup("frenzy");

    act("$n utters a word of divine power!",ch,NULL,NULL,TO_ROOM,FALSE);
    send_to_char("You utter a word of divine power.\n\r",ch);
 
    for ( vch = ch->in_room->people; vch != NULL; vch = vch_next )
    {
        vch_next = vch->next_in_room;

  if ((IS_GOOD(ch) && IS_GOOD(vch)) ||
      (IS_EVIL(ch) && IS_EVIL(vch)) ||
      (IS_NEUTRAL(ch) && IS_NEUTRAL(vch)) )
  {
    send_to_char("You feel fully more powerful.\n\r",vch);
    spell_frenzy(frenzy_num,level,ch,(void *) vch,TARGET_CHAR); 
    spell_bless(bless_num,level,ch,(void *) vch,TARGET_CHAR);
  }

  else if ((IS_GOOD(ch) && IS_EVIL(vch)) ||
     (IS_EVIL(ch) && IS_GOOD(vch)) )
  {
    if (!is_safe_spell(ch,vch,TRUE, sn))
    {
            spell_curse(curse_num,level,ch,(void *) vch,TARGET_CHAR);
      send_to_char("You are struck down!\n\r",vch);
      dam = dice(level,6);
      damage(ch,vch,dam,sn,DAM_ENERGY,TRUE,TRUE);
    }
  }

        else if (IS_NEUTRAL(ch))
  {
    if (!is_safe_spell(ch,vch,TRUE, sn))
    {
            spell_curse(curse_num,level/2,ch,(void *) vch,TARGET_CHAR);
      send_to_char("You are struck down!\n\r",vch);
      dam = dice(level,4);
      damage(ch,vch,dam,sn,DAM_ENERGY,TRUE,TRUE);
      }
  }
    }  
    
    send_to_char("You feel drained.\n\r",ch);
    ch->move = 0;
    ch->hit /= 2;
}
 
void spell_identify( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    char buf[MAX_STRING_LENGTH];
    AFFECT_DATA *paf;
    //float num;
    bool stop_looping;
    bool already_did_enchant;
    already_did_enchant = FALSE;
    stop_looping = FALSE;
    sprintf( buf,
  "Object '%s' is type %s.\n\rWeight is %d.%d, value is %d, level is %d.\n\r",

  obj->name,
  item_type_name( obj ),
  obj->weight / 10, obj->weight % 10,
  obj->cost,
  obj->level
  );
    send_to_char( buf, ch );

    if( obj->pIndexData->new_format )
    {
     sprintf( buf, "Made of: %s", obj->material);
     send_to_char( buf, ch );
	if (obj->item_type == ITEM_ARMOR && obj->value[4] <= MAX_OBJ_SIZE)
	  sprintf( buf, ", Size: %s\n\r", obj_size_table[obj->value[4]].name);
	else
	  sprintf( buf, "\n\r");
     send_to_char( buf, ch );
    }

    
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
       sprintf( buf, "Extra Flags %s.\n\r", extra_bit_name(obj->extra_flags));
       send_to_char( buf, ch);
       if(IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
        send_to_char("The curse on this object is weakened currently.\n\r", ch);
    }
     

    switch ( obj->item_type )
    {
    case ITEM_SCROLL: 
    case ITEM_POTION:
    case ITEM_PILL:
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
  sprintf( buf, "Level %d spells of:", obj->value[0] );
      send_to_char ("Contains the following spells:\r\n",ch);


  if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      /*send_to_char( skill_table[obj->value[1]].name, ch );*/
      write_spell(ch,obj->value[1]);
      send_to_char( "'", ch );
  }

  if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      /*send_to_char( skill_table[obj->value[2]].name, ch );*/
      write_spell(ch,obj->value[2]);
      send_to_char( "'", ch );
  }

  if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      /*send_to_char( skill_table[obj->value[3]].name, ch );*/
      write_spell(ch,obj->value[3]);
      send_to_char( "'", ch );
  }

  send_to_char( ".\n\r", ch );
    }
	break;
	    /* Here's the origional section below
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
  sprintf( buf, "Level %d spells of:", obj->value[0] );
  send_to_char( buf, ch );

  if ( obj->value[1] >= 0 && obj->value[1] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      send_to_char( skill_table[obj->value[1]].name, ch );
      send_to_char( "'", ch );
  }

  if ( obj->value[2] >= 0 && obj->value[2] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      send_to_char( skill_table[obj->value[2]].name, ch );
      send_to_char( "'", ch );
  }

  if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      send_to_char( skill_table[obj->value[3]].name, ch );
      send_to_char( "'", ch );
  }

  send_to_char( ".\n\r", ch );
    }
  break;
*/

    case ITEM_WAND: 
    case ITEM_STAFF: 
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    { 
  sprintf( buf, "Has %d charges of ",
      obj->value[2]);
  send_to_char( buf, ch );
      
  if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
  {
      send_to_char( " '", ch );
      write_spell(ch,obj->value[3]);
     /* send_to_char( skill_table[obj->value[3]].name, ch );*/
      send_to_char( "'", ch );
  }

  send_to_char( ".\n\r", ch );
    }
  break;

    case ITEM_SPELL_PAGE:
  /*
	sprintf(buf,"It is a page containing the spell '%s'.\n\r",
	    skill_table[obj->value[2]].name );
	send_to_char(buf,ch);
	*/
 	if ( obj->value[1] <= 25 )
	    sprintf(buf,"This page is trivial to copy.");
	else
	if ( obj->value[1] <= 50 )
	    sprintf(buf,"This is of moderate complexity.");
	else
	if ( obj->value[1] <= 75 )
	    sprintf(buf,"This page is somewhat difficult to copy.");
	else
	if ( obj->value[1] <= 100 )
	    sprintf(buf,"This page is fairly challenging to copy.");
	else
	    sprintf(buf,"This page is very difficult to copy.");
	send_to_char(buf,ch); send_to_char("\n\r",ch);
	/*
	if ( obj->value[3] < 0 )
	    sprintf(buf,"This spell can be copied without limit.\n\r");
	else
	    sprintf(buf,"This spell can be copied %d time%s.\n\r",
		obj->value[3], obj->value[3] == 1 ? "" : "s" );
	send_to_char(buf,ch);
	*/
	break;
    case ITEM_DRINK_CON:
        sprintf(buf,"It holds %s-colored %s.\n\r",
            liq_table[obj->value[2]].liq_color,
            liq_table[obj->value[2]].liq_name);
        send_to_char(buf,ch);
        break;

    case ITEM_CONTAINER:
	//
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
  sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
      obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
  send_to_char(buf,ch);
  if (obj->value[4] != 100)
  {
      sprintf(buf,"Weight multiplier: %d%%\n\r",
      obj->value[4]);
      send_to_char(buf,ch);
  }
    }
    //
  break;
    
    case ITEM_WEAPON:
  send_to_char("Weapon type is ",ch);
  switch (obj->value[0])
  {
      case(WEAPON_EXOTIC) : send_to_char("exotic.\n\r",ch); break;
      case(WEAPON_SWORD)  : send_to_char("sword.\n\r",ch);  break;  
      case(WEAPON_DAGGER) : send_to_char("dagger.\n\r",ch); break;
      case(WEAPON_SPEAR)  : send_to_char("spear/staff.\n\r",ch);  break;
      case(WEAPON_MACE)   : send_to_char("mace/club.\n\r",ch);  break;
      case(WEAPON_AXE)  : send_to_char("axe.\n\r",ch);    break;
      case(WEAPON_FLAIL)  : send_to_char("flail.\n\r",ch);  break;
      case(WEAPON_WHIP) : send_to_char("whip.\n\r",ch);   break;
      case(WEAPON_POLEARM): send_to_char("polearm.\n\r",ch);  break;
      case(WEAPON_GAROTTE): send_to_char("garotte.\n\r",ch);  break;
      default   : send_to_char("unknown.\n\r",ch);  break;
  }
  //
  if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
  {
    if (obj->pIndexData->new_format)
    {
      sprintf(buf,"Damage is %dd%d (average %d).\n\r",
       obj->value[1],obj->value[2],
       ((1 + obj->value[2]) * obj->value[1] / 2));
    }
    else
    {
      sprintf( buf, "Damage is %d to %d (average %d).\n\r",
        obj->value[1], obj->value[2],
        ( (obj->value[1] + obj->value[2] ) / 2) );
    }
  send_to_char( buf, ch );
     //
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
        if (obj->value[4])  // weapon flags
        {
            sprintf(buf,"Weapons flags: %s\n\r",weapon_bit_name(obj->value[4]));
            send_to_char(buf,ch);
        }
    }
  } 
  break;

    case ITEM_ARMOR:
    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
      sprintf( buf, 
       "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic.\n\r", 
        (obj->value[0]), (obj->value[1]), (obj->value[2]), (obj->value[3]) );
      send_to_char( buf, ch );
    }
    
  break;
    }

    if( !IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
    {
    if (!obj->enchanted)
    for ( paf = obj->pIndexData->affected; paf != NULL; paf = paf->next )
    {
      if ( paf->location != APPLY_NONE && paf->modifier != 0 )
      {
        if ( stop_looping == TRUE)
        {
          break;
        }
//COREY TEST THIS BREAK
    //break;
		 
        sprintf( buf, "Affects %s by %d.\n\r",
         affect_loc_name( paf->location ), (paf->modifier) );
        send_to_char(buf,ch);
        if (paf->bitvector)
        {
          switch(paf->where)
          {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf->bitvector));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s object flag.\n",
                            extra_bit_name(paf->bitvector));
                        break;
                    case TO_IMMUNE:
                        sprintf(buf,"Adds immunity to %s.\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf(buf,"Adds resistance to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf(buf,"Adds vulnerability to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    default:
                        sprintf(buf,"Unknown bit %d: %ld\n\r",
                            paf->where,paf->bitvector);
                        break;
                }
          send_to_char( buf, ch );
      }
//
  }
    }
stop_looping = FALSE;

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
  if ( paf->location != APPLY_NONE && paf->modifier != 0 )
  {
  if (already_did_enchant == FALSE)
  {
    if(paf->location == APPLY_HITROLL || paf->location == APPLY_DAMROLL)
    {
     int Nench = number_fuzzy(paf->modifier);

      strcpy(buf,"Magically enchanted: ");
      if ( Nench >= 14 )
      { strcat(buf,"perfectly\n\r"); }
      else if( Nench >= 11 )
      { strcat(buf,"well\n\r"); }
      else if( Nench >= 8 )
      { strcat(buf,"good\n\r"); }
      else if( Nench >= 5 )
      { strcat(buf,"fairly\n\r"); }
      else if( Nench > 2 )
      { strcat(buf,"some\n\r"); }
      else
      { strcat(buf,"a bit\n\r"); }
      send_to_char(buf,ch);
      already_did_enchant = TRUE;
    } else {
      send_to_char("Magically enhanced.\n\r",ch);
      already_did_enchant = TRUE;
    }
  } //end of already did enchant
if ( stop_looping == TRUE)
{
break;
}
//  break;
//#ifdef COREY_TAKEOUT_SO_OTHER_AFFECTS_SHOW
      sprintf( buf, "Affects %s by %d",
        affect_loc_name( paf->location ), (paf->modifier) );
      send_to_char( buf, ch );
            if ( paf->duration > -1)
                sprintf(buf,", %d hours.\n\r",(paf->duration));
            else
                sprintf(buf,".\n\r");
      send_to_char(buf,ch);
            if (paf->bitvector)
            {
                switch(paf->where)
                {
                    case TO_AFFECTS:
                        sprintf(buf,"Adds %s affect.\n",
                            affect_bit_name(paf->bitvector));
                        break;
                    case TO_OBJECT:
                        sprintf(buf,"Adds %s object flag.\n",
                            extra_bit_name(paf->bitvector));
                        break;
        case TO_WEAPON:
      sprintf(buf,"Adds %s weapon flags.\n",
          weapon_bit_name(paf->bitvector));
      break;
                    case TO_IMMUNE:
                        sprintf(buf,"Adds immunity to %s.\n",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_RESIST:
                        sprintf(buf,"Adds resistance to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    case TO_VULN:
                        sprintf(buf,"Adds vulnerability to %s.\n\r",
                            imm_bit_name(paf->bitvector));
                        break;
                    default:
                        sprintf(buf,"Unknown bit %d: %ld\n\r",
                            paf->where,paf->bitvector);
                        break;
                }
                send_to_char(buf,ch);
            }
//#endif
  }
    }
    }

		if(obj->damaged < 0)
			send_to_char("Condition: {GPerfect!{x\n\r", ch);
		else if(!obj->damaged)
			send_to_char("Condition: {WUndamaged.{x\n\r", ch);
		else if(obj->damaged < 60)
			send_to_char("Condition: {YLightly damaged.{x\n\r", ch);
		else if(obj->damaged < 100)
			send_to_char("Condition: {rHeavily damaged.{x\n\r", ch);
		else
			send_to_char("Condition: {RBroken.{x\n\r", ch);

    if( IS_OBJ_STAT(obj,ITEM_NOIDENTIFY))
       send_to_char("Your scrying is unable to attain more information.\n\r",ch);
    return;
}



void spell_infravision( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_INFRARED) )
    {
    	if(reup_affect(victim,sn,level*2,level))
	{
    send_to_char( "Your eyes glow red.\n\r", victim );
    act( "$n's eyes glow red.\n\r", victim, NULL, NULL, TO_ROOM ,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}

  return;
    }

    act( "$n's eyes glow red.\n\r", victim, NULL, NULL, TO_ROOM ,FALSE);

    af.where   = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 2 * level;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INFRARED;
    affect_to_char( victim, &af );
    send_to_char( "Your eyes glow red.\n\r", victim );
    return;
}



void spell_invis( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;

    /* object invisibility */
    if (target == TARGET_OBJ)
    {
  obj = (OBJ_DATA *) vo;  

  if (IS_OBJ_STAT(obj,ITEM_INVIS))
  {
      act("$p is already invisible.",ch,obj,NULL,TO_CHAR,FALSE);
      return;
  }
  
  af.where  = TO_OBJECT;
  af.type   = sn;
  af.level  = level;
  af.duration = level + 12;
  af.location = APPLY_NONE;
  af.modifier = 0;
  af.bitvector  = ITEM_INVIS;
  affect_to_obj(obj,&af);

  act("$p fades out of sight.",ch,obj,NULL,TO_ALL,FALSE);
  return;
    }

    /* character invisibility */
    victim = (CHAR_DATA *) vo;

    if ( IS_AFFECTED(victim, AFF_INVISIBLE) )
    {
	if(reup_affect(victim,sn,level+12,level))
	{
    send_to_char( "You fade out of existence.\n\r", victim );
    if ( ch != victim )
    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM ,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    if ( IS_AFFECTED(victim,AFF_FAERIE_FOG) ) {
	send_to_char("You failed.\n\r",ch);
	return;
    }

    act( "$n fades out of existence.", victim, NULL, NULL, TO_ROOM ,FALSE);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level + 12;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_INVISIBLE;
    affect_to_char( victim, &af );
    send_to_char( "You fade out of existence.\n\r", victim );
    return;
}



void spell_know_alignment(int sn,int level,CHAR_DATA *ch,void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    char *msg;
    int ap;

    ap = victim->alignment;

         if ( ap >  700 ) msg = "$N has a pure and good aura.";
    else if ( ap >  350 ) msg = "$N is of excellent moral character.";
    else if ( ap >  100 ) msg = "$N is often kind and thoughtful.";
    else if ( ap > -100 ) msg = "$N doesn't have a firm moral commitment.";
    else if ( ap > -350 ) msg = "$N lies to $S friends.";
    else if ( ap > -700 ) msg = "$N is a black-hearted murderer.";
    else msg = "$N is the embodiment of pure evil!.";

    act( msg, ch, NULL, victim, TO_CHAR ,FALSE);
    return;
}



void spell_lightning_bolt(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
   0,
   0,  0,  0,  0,  0,  0,  0,  0, 25, 28,
  31, 34, 37, 40, 40, 41, 42, 42, 43, 44,
  44, 45, 46, 46, 47, 48, 48, 49, 50, 50,
  51, 52, 52, 53, 54, 54, 55, 56, 56, 57,
  58, 58, 59, 60, 60, 61, 62, 62, 63, 64
    };
    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim,DAM_LIGHTNING) )
  dam /= 2;

    if ( level == 1 )
    { 
	sprintf(log_buf,"%s casting lightning bolt",ch->name);
	log_string(log_buf);
    }

    damage( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE,TRUE);
    return;
}


bool is_bounty_obj(OBJ_DATA *obj)
{
  if(bounty_timer <= 0)
    return FALSE;// Nobody knows it's the object, it's fine to locate
  if(bounty_type == BOUNTY_ITEM_NAME || bounty_type == BOUNTY_ITEM_DESC)
  {
    if(obj->pIndexData->vnum == bounty_item)
      return TRUE;
  }
  return FALSE;
}


void spell_locate_object( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    char buf[MAX_INPUT_LENGTH];
    BUFFER *buffer;
    OBJ_DATA *obj;
    OBJ_DATA *in_obj;
    bool found;
    int number = 0, max_found;

    found = FALSE;
    number = 0;
    max_found = IS_IMMORTAL(ch) ? 200 : 2 * level;

    buffer = new_buf();
 
    for ( obj = object_list; obj != NULL; obj = obj->next )
    {
  if ( !can_see_obj( ch, obj ) || !is_name( target_name, obj->name ) 
      ||   IS_OBJ_STAT(obj,ITEM_NOLOCATE) || number_percent() > 2 * level
  ||   ch->level < obj->level || is_bounty_obj(obj))
      continue;

  for ( in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj )
      ;

  if ( in_obj->carried_by != NULL && can_see(ch,in_obj->carried_by,FALSE))
  {
      sprintf( buf, "one is carried by %s\n\r",
    PERS(in_obj->carried_by, ch, FALSE) );
  }
  else
  {
      if (IS_IMMORTAL(ch) && in_obj->in_room != NULL)
    sprintf( buf, "one is in %s [Room %d]\n\r",
        in_obj->in_room->name, in_obj->in_room->vnum);
      else
      {
	if(in_obj->carried_by != NULL && !can_see(ch,in_obj->carried_by,TRUE))
  		continue;// No locating objects on immortals you can't see 
        sprintf( buf, "one is in %s\n\r",
        in_obj->in_room == NULL
          ? "somewhere" : in_obj->in_room->name );
      }
  }

  buf[0] = UPPER(buf[0]);
  add_buf(buffer,buf);

  found = TRUE;
        number++;

      if (number >= max_found)
      break;
    }

    if ( !found )
  send_to_char( "Nothing like that in heaven or earth.\n\r", ch );
    else
  page_to_char(buf_string(buffer),ch);

    free_buf(buffer);

    return;
}



void spell_magic_missile( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const sh_int dam_each[] = 
    {
   0,
   3,  3,  4,  4,  5,  6,  6,  6,  6,  6,
   7,  7,  7,  7,  7,  8,  8,  8,  8,  8,
   9,  9,  9,  9,  9, 10, 10, 10, 10, 10,
  11, 11, 11, 11, 11, 12, 12, 12, 12, 12,
  13, 13, 13, 13, 13, 14, 14, 14, 14, 14
    };
    int dam;
    int num_missiles, i;

    num_missiles = dice(1,4) + ch->level / 12; 

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    
    for ( i = 0 ; i < num_missiles ; i++ )
    {
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    damage( ch, victim, dam, sn, DAM_ENERGY ,TRUE,TRUE);
    if ( victim == NULL || victim->position <= POS_STUNNED || ch->fighting == NULL )
	return;
    }

    return;
}

void spell_mass_healing(int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *gch;
    int heal_num, refresh_num;
    
    heal_num = skill_lookup("heal");
    refresh_num = skill_lookup("refresh"); 

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  if ((IS_NPC(ch) && IS_NPC(gch)) ||
      (!IS_NPC(ch) && !IS_NPC(gch)))
  {
      spell_heal(heal_num,level,ch,(void *) gch,TARGET_CHAR);
      spell_refresh(refresh_num,level,ch,(void *) gch,TARGET_CHAR);  
  }
    }
}
      

void spell_mass_invis( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    AFFECT_DATA af;
    CHAR_DATA *gch;

    for ( gch = ch->in_room->people; gch != NULL; gch = gch->next_in_room )
    {
  if ( !is_same_group( gch, ch ) || IS_AFFECTED(gch, AFF_INVISIBLE)
	|| IS_AFFECTED(gch,AFF_FAERIE_FOG) )
      continue;
  act( "$n slowly fades out of existence.", gch, NULL, NULL, TO_ROOM ,FALSE);
  send_to_char( "You slowly fade out of existence.\n\r", gch );

  af.where     = TO_AFFECTS;
  af.type      = sn;
      af.level     = level/2;
  af.duration  = 24;
  af.location  = APPLY_NONE;
  af.modifier  = 0;
  af.bitvector = AFF_INVISIBLE;
  affect_to_char( gch, &af );
    }
    send_to_char( "Ok.\n\r", ch );

    return;
}



void spell_null( int sn, int 
level, CHAR_DATA *ch, void *vo, int target ) {
    send_to_char( "That's not a spell!\n\r", ch );
    return;
}



void spell_pass_door( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PASS_DOOR) )
    {
    	if(reup_affect(victim,sn,level/4,level))
	{
    send_to_char( "You turn translucent.\n\r", victim );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM ,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = number_fuzzy( level / 4 );
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_PASS_DOOR;
    affect_to_char( victim, &af );
    act( "$n turns translucent.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You turn translucent.\n\r", victim );
    return;
}

/* RT plague spell, very nasty */

void spell_plague( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if (saves_spell(level,victim,DAM_DISEASE) || 
        (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
    {
  if (ch == victim)
    send_to_char("You feel momentarily ill, but it passes.\n\r",ch);
  else
    act("$N seems to be unaffected.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if (IS_AFFECTED(victim,AFF_PLAGUE))
    {
       if ( victim != ch )
          act("$N is already plagued.",ch,NULL,victim,TO_CHAR,FALSE);
       else
	  send_to_char("You're already plagued!",ch);
       return;
    }

    if(!is_affected( victim, sn ))
    {
    af.where     = TO_AFFECTS;
    af.type     = sn;
    af.level    = level * 3/4;
    af.duration  = level;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = level/7;break;
        case 1:af.duration = level/6;break;
        case 2:af.duration = level/5;break;
        default:af.duration = level;break;
        }
	if(af.duration < 5)
		af.duration = 5;
    }
    af.location  = APPLY_STR;
    af.modifier  = -3; 
    af.bitvector = AFF_PLAGUE;
    affect_join(victim,&af);
   
    send_to_char
      ("You scream in agony as plague sores erupt from your skin.\n\r",victim);
    act("$n screams in agony as plague sores erupt from $s skin.",
  victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if(check_annointment(victim, ch))
    {
        spell_plague(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_poison( int sn, int level, CHAR_DATA *ch, void *vo, int target )
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    AFFECT_DATA af;


    if (target == TARGET_OBJ)
    {
  obj = (OBJ_DATA *) vo;

  if (obj->item_type == ITEM_FOOD || obj->item_type == ITEM_DRINK_CON)
  {
      if (IS_OBJ_STAT(obj,ITEM_BLESS) || IS_OBJ_STAT(obj,ITEM_BURN_PROOF))
      {
    act("Your spell fails to corrupt $p.",ch,obj,NULL,TO_CHAR,FALSE);
    return;
      }
      obj->value[3] = 1;
      act("$p is infused with poisonous vapors.",ch,obj,NULL,TO_ALL,FALSE);
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

      if (IS_WEAPON_STAT(obj,WEAPON_POISON))
      {
    act("$p is already envenomed.",ch,obj,NULL,TO_CHAR,FALSE);
    return;
      }

      af.where   = TO_WEAPON;
      af.type  = sn;
      af.level   = level;
      af.duration  = level/2;
      af.location  = 0;
      af.modifier  = 0;
      af.bitvector = WEAPON_POISON;
      affect_to_obj(obj,&af);

      act("$p is coated with deadly venom.",ch,obj,NULL,TO_ALL,FALSE);
      return;
  }

  act("You can't poison $p.",ch,obj,NULL,TO_CHAR,FALSE);
  return;
    }

    victim = (CHAR_DATA *) vo;

    if ( saves_spell( level, victim,DAM_POISON) )
    {
  act("$n turns slightly green, but it passes.",victim,NULL,NULL,TO_ROOM,FALSE);
  send_to_char("You feel momentarily ill, but it passes.\n\r",victim);
  return;
    }

    if (IS_AFFECTED(victim,AFF_POISON))
    {
    	if(reup_affect(victim,sn,level,level))
	{
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    if(!is_affected( victim, sn ))
    {
    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = UMAX(6, level / 5);
    af.location  = APPLY_STR;
    af.modifier  = -1;
    af.bitvector = AFF_POISON;
    affect_join( victim, &af );
    send_to_char( "You feel very sick.\n\r", victim );
    act("$n looks very ill.",victim,NULL,NULL,TO_ROOM,FALSE);
    }

    if(check_annointment(victim, ch))
    {
        spell_poison(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



void spell_protection_neutral(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL) 	||
	 IS_AFFECTED(victim, AFF_PROTECT_GOOD)  ||
	 is_affected(victim, gsn_protect_neutral) )
    {
    	if(reup_affect(victim,sn,24,level))
	{
    send_to_char("You are ready to smite wishy washy neutral punks.\n\r",victim);
    if ( victim != ch )
    act("$N is protected from neutrality.",ch,NULL,victim,TO_CHAR,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= 24;
    af.location		= APPLY_SAVING_SPELL;
    af.modifier		= -1;
    af.bitvector	= 0;
    affect_to_char(victim,&af);
    send_to_char("You are ready to smite wishy washy neutral punks.\n\r",victim);
    if ( victim != ch )
    act("$N is protected from neutrality.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}


void spell_protection_evil(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if ( IS_AFFECTED(victim, AFF_PROTECT_EVIL) 
    ||   IS_AFFECTED(victim, AFF_PROTECT_GOOD)
    ||   is_affected(victim,gsn_protect_neutral))
    {
    	if(reup_affect(victim,sn,24,level))
	{
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_EVIL;
    affect_to_char( victim, &af );
    send_to_char( "You feel holy and pure.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from evil.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}
 
void spell_protection_good(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if ( IS_AFFECTED(victim, AFF_PROTECT_GOOD) 
    ||   IS_AFFECTED(victim, AFF_PROTECT_EVIL)
    ||   is_affected(victim, gsn_protect_neutral))
    {
    	if(reup_affect(victim,sn,24,level))
	{
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR,FALSE);
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
        return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 24;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -1;
    af.bitvector = AFF_PROTECT_GOOD;
    affect_to_char( victim, &af );
    send_to_char( "You feel aligned with darkness.\n\r", victim );
    if ( ch != victim )
        act("$N is protected from good.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}


void spell_ray_of_truth (int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam, align;
 
    if (IS_EVIL(ch) )
    {
        victim = ch;
        send_to_char("The energy explodes inside you!\n\r",ch);
    }
 
    if((victim != ch || !IS_EVIL(ch)) && IS_SET(ch->in_room->room_affects, RAFF_SHADED))
    {
      send_to_char("The darkness swallows up your burst of light.\n\r", ch);
      return;
    }

    if (victim != ch)
    {
        act("$n raises $s hand, and a blinding ray of light shoots forth!",
            ch,NULL,NULL,TO_ROOM,FALSE);
        send_to_char(
     "You raise your hand and a blinding ray of light shoots forth!\n\r",
     ch);
    }

    if(IS_SET(victim->affected_by_ext, AFF_EXT_SHADED))
    {// Light damage component of this spell always works on shadows
      AFFECT_DATA *light = affect_find(ch->affected,gsn_light_blast);
      if(light)
        light->duration = UMAX(1, light->duration);
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
      }
    }

    if (IS_GOOD(victim))
    {
  act("$n seems unharmed by the light.",victim,NULL,victim,TO_ROOM,FALSE);
  send_to_char("The light seems powerless to affect you.\n\r",victim);
  return;
    }

    dam = dice( level, 10 );
    if ( saves_spell( level, victim,DAM_HOLY) )
        dam /= 2;

    align = victim->alignment;
    align -= 350;

    if (align < -1000)
  align = -1000 + (align + 1000) / 3;

    dam = (dam * align * align) / 1000000;

    spell_blindness(gsn_blindness, 
		   3 * level / 4, ch, (void *) victim,TARGET_CHAR);
    damage( ch, victim, dam, sn, DAM_HOLY ,TRUE,TRUE);

   if( (!is_affected(ch, skill_lookup("indulgence"))) || (ch->kit != kit_lookup("necromancer")) )
    ch->alignment = UMIN(1000, ch->alignment + 50);
}


void spell_recharge( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    OBJ_DATA *obj = (OBJ_DATA *) vo;
    int chance, percent;

    if (obj->pIndexData->vnum == OBJ_VNUM_WAND_PINE ||
 obj->pIndexData->vnum == OBJ_VNUM_WAND_PINE   ||
 obj->pIndexData->vnum == OBJ_VNUM_WAND_APPLE  ||
 obj->pIndexData->vnum == OBJ_VNUM_WAND_OAK    ||
 obj->pIndexData->vnum == OBJ_VNUM_WAND_WILLOW ||
 obj->pIndexData->vnum == OBJ_VNUM_WAND_YEW )
     {
       send_to_char("You may not recharge this wand.\r\n",ch);
	return; 
     }

    if ( (obj->item_type != ITEM_WAND) & (obj->item_type != ITEM_STAFF) )
    {
  send_to_char("That item does not carry charges.\n\r",ch);
  return;
    }

    if (obj->value[0] >= 3 * level / 2)
    {
  send_to_char("Your skills are not great enough for that.\n\r",ch);
  return;
    }

    if (obj->value[1] == 0)
    {
  send_to_char("That item has already been recharged once.\n\r",ch);
  return;
    }

    chance = 40 + 2 * level;

    chance -= obj->value[0]; /* harder to do high-level spells */
    chance -= (obj->value[1] - obj->value[2]) *
        (obj->value[1] - obj->value[2]);

    chance = UMAX(level/2,chance);

    percent = number_percent();

    if (percent < chance / 2)
    {
  act("$p glows softly.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows softly.",ch,obj,NULL,TO_ROOM,FALSE);
  obj->value[2] = UMAX(obj->value[1],obj->value[2]);
  obj->value[1] = 0;
  return;
    }

    else if (percent <= chance)
    {
  int chargeback,chargemax;

  act("$p glows softly.",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows softly.",ch,obj,NULL,TO_ROOM,FALSE);

  chargemax = obj->value[1] - obj->value[2];
  
  if (chargemax > 0)
      chargeback = UMAX(1,chargemax * percent / 100);
  else
      chargeback = 0;

  obj->value[2] += chargeback;
  obj->value[1] = 0;
  return;
    } 

    else if (percent <= UMIN(95, 3 * chance / 2))
    {
  send_to_char("Nothing seems to happen.\n\r",ch);
  if (obj->value[1] > 1)
      obj->value[1]--;
  return;
    }

    else /* whoops! */
    {
  act("$p glows brightly and explodes!",ch,obj,NULL,TO_CHAR,FALSE);
  act("$p glows brightly and explodes!",ch,obj,NULL,TO_ROOM,FALSE);
  extract_obj(obj);
    }
}

void spell_refresh( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    victim->move = UMIN( victim->move + level, victim->max_move );
    if (victim->max_move == victim->move)
        send_to_char("You feel fully refreshed!\n\r",victim);
    else
        send_to_char( "You feel less tired.\n\r", victim );
    if ( ch != victim )
        send_to_char( "Ok.\n\r", ch );
    return;
}

void spell_remove_curse( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim;
    OBJ_DATA *obj;
    bool found = FALSE;

    /* do object cases first */
    if (target == TARGET_OBJ)
    {
  obj = (OBJ_DATA *) vo;

  if ((IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE))
    && !IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
  {
      if (!IS_OBJ_STAT(obj,ITEM_NOUNCURSE))
      {
        if(!saves_dispel(level + 2,obj->level,0))
        {
      REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
      REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
      act("$p glows blue.",ch,obj,NULL,TO_ALL,FALSE);
      return;
        }
      }
      else
      {
        int min = UMIN(obj->level - 5 + level / 10, level - 5);
        if(!IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED) &&
          !saves_dispel(min,obj->level,0))
        {
          SET_BIT(obj->extra_flags2, ITEM2_TEMP_UNCURSED);
          act("$p glows a faint blue.",ch,obj,NULL,TO_ALL,FALSE);
          return;
        }
      }

      act("The curse on $p is beyond your power.",ch,obj,NULL,TO_CHAR,FALSE);
      return;
  }
  return;
    }

    /* characters */
    victim = (CHAR_DATA *) vo;

/* Adding in so people just dont remove curse others without worry */
/*    if( (is_safe(ch,victim) && !is_same_group(ch,victim) 
        && !is_same_clan(ch,victim) && !IS_NPC(ch) )
        || (!is_clan(ch) && IS_SET(victim->act,PLR_NOCANCEL)))
    {
	send_to_char("You are not permitted to cast on them.\n\r",ch);
	return;
    }
    else
       if( is_clan(ch) && is_clan(victim) && !is_same_group(ch,victim)
           && !is_same_clan(ch,victim) )
          check_killer(ch,victim);

    if(!is_same_group(ch,victim) && ch != victim &&
        saves_spell(level,victim,DAM_OTHER))
    {
	send_to_char("You failed.\n\r",ch);
	return;
    }*/

  if(is_affected( victim, gsn_curse))
  {
    if (check_dispel(level,victim,gsn_curse))
    {
  send_to_char("You feel better.\n\r",victim);
  act("$n looks more relaxed.",victim,NULL,NULL,TO_ROOM,FALSE);
  return;
    }
    act("You failed to lift the curse from $N.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
  }

    if (((!IS_NPC(ch) && IS_NPC(victim) &&
   !(IS_AFFECTED(ch, AFF_CHARM) && ch->master == victim) ) ||
        (IS_NPC(ch) && !IS_NPC(victim)) ||
        (!IS_NPC(victim) && IS_SET (victim->act,PLR_NOCANCEL) && (ch != victim)))
    && (!IS_NPC(ch) || !IS_SET(ch->act, ACT_IS_HEALER))
    )
    {
  send_to_char("You failed, target must be cancellable to uncurse an item.\n\r",ch);
  return;
    }

   for (obj = victim->carrying; (obj != NULL && !found); obj = obj->next_content)
   {
        if ((IS_OBJ_STAT(obj,ITEM_NODROP) || IS_OBJ_STAT(obj,ITEM_NOREMOVE)))
        {   /* attempt to remove curse */
          if(IS_OBJ_STAT(obj,ITEM_NOUNCURSE) &&
            !IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED))
          {// Harder than uncursing it directly
            int min = UMIN(obj->level - 8 + level / 10, level - 8);
            if(!IS_SET(obj->extra_flags2, ITEM2_TEMP_UNCURSED) &&
              !saves_dispel(min,obj->level,0))
            {
              SET_BIT(obj->extra_flags2, ITEM2_TEMP_UNCURSED);
              act("$p glows a faint blue.",ch,obj,NULL,TO_ALL,FALSE);
              return;
            }
          }
          else
          {
            if (!saves_dispel(level,obj->level,0))
            {
                found = TRUE;
                REMOVE_BIT(obj->extra_flags,ITEM_NODROP);
                REMOVE_BIT(obj->extra_flags,ITEM_NOREMOVE);
                act("Your $p glows blue.",victim,obj,NULL,TO_CHAR,FALSE);
                act("$n's $p glows blue.",victim,obj,NULL,TO_ROOM,FALSE);
            }
         }
        }
    }
}

void spell_sanctuary( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_SANCTUARY) )
    {
    	if(reup_affect(victim,sn,level/6,level))
	{
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
	}
	else
	{
	  send_to_char("Nothing seems to happen.\n\r",ch);
	}
	return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 6;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SANCTUARY;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a white aura.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You are surrounded by a white aura.\n\r", victim );
    return;
}



void spell_shield( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
      reup_affect(victim,sn,8+level,level);
      return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = 8 + level;
    af.location  = APPLY_AC;
    af.modifier  = -20;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n is surrounded by a force shield.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "You are surrounded by a force shield.\n\r", victim );
    return;
}



void spell_shocking_grasp(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    static const int dam_each[] = 
    {
   0,
   0,  0,  0,  0,  0,  0, 20, 25, 29, 33,
  36, 39, 39, 39, 40, 40, 41, 41, 42, 42,
  43, 43, 44, 44, 45, 45, 46, 46, 47, 47,
  48, 48, 49, 49, 50, 50, 51, 51, 52, 52,
  53, 53, 54, 54, 55, 55, 56, 56, 57, 57
    };
    int dam;

    level = UMIN(level, sizeof(dam_each)/sizeof(dam_each[0]) - 1);
    level = UMAX(0, level);
    dam   = number_range( dam_each[level] / 2, dam_each[level] * 2 );
    if (ch->class == class_lookup("mage"))
    {
      dam *= 1.25;
    }
    if ( saves_spell( level, victim,DAM_LIGHTNING) )
  dam /= 2;
    damage( ch, victim, dam, sn, DAM_LIGHTNING ,TRUE,TRUE);
    return;
}

void spell_entrance( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;

    if (is_safe(ch,victim)) return;
    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not entrance.\n\r",ch);
       return;
    }
    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
    {
       send_to_char("Highlanders can not entrance.\n\r",ch);
       return;
    }
    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians may not be mentally manipulated.\n\r", ch);
      return;
    }

    if (ch->level < victim->level)
    {
	send_to_char("Your will is not strong enough.\n\r",ch);
	return;
    }

    if (saves_spell (ch->level,victim,DAM_MENTAL))
    {
       send_to_char("Your will has been defeated.\n\r",ch);
       return;
    }

  if ( IS_AFFECTED(victim, AFF_CHARM) 
       || (victim->master != ch && victim->master != NULL) )
  {  
      send_to_char("You can not will those under another's power.\n\r", ch );
      return;
  }

    SET_BIT(victim->affected_by,AFF_CHARM);
    add_follower( victim, ch );
    do_order(ch,target_name);
    stop_follower( victim );
    REMOVE_BIT(victim->affected_by,AFF_CHARM);
    victim->master = NULL;
    return;
}

void spell_sleep( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if (IS_SET(victim->mhs,MHS_HIGHLANDER) && !IS_NPC(victim))
    {
       send_to_char("Highlanders are immune to that.\n\r",ch);
       return;
    }
    if(IS_NPC(victim) && victim->pIndexData->vnum == MOB_VNUM_CLAN_GUARDIAN)
    {
      send_to_char("Guardians may not be mentally manipulated.\n\r", ch);
      return;
    }

    level = UMIN(level,ch->level);

    if ( !IS_NPC(ch) && ( ch->pcdata->old_class != class_lookup("mage")) ) 
	level = 9* level /10 ;
    if ( IS_AFFECTED(victim, AFF_SLEEP)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD))
    ||   saves_spell( level, victim,DAM_CHARM) )
  return;

    if ( !IS_NPC(victim) 
    && victim->race == race_lookup("elf")
    && saves_spell(level/4, victim, DAM_CHARM) )
    {
        return;
    }

    switch(check_immune(victim,DAM_MENTAL))
    {
  case IS_IMMUNE:   level = 0;  break;
  case IS_RESISTANT:  level /= 2;  break;
  case IS_VULNERABLE: level += 4;  break;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    if (IS_SET(victim->mhs,MHS_GLADIATOR) && !IS_NPC(victim))
       af.duration  = 3;
    else
       af.duration  = level / 5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_SLEEP;
    affect_join( victim, &af );

    if ( IS_AWAKE(victim) )
    {
  send_to_char( "You feel very sleepy ..... zzzzzz.\n\r", victim );
  act( "$n goes to sleep.", victim, NULL, NULL, TO_ROOM ,FALSE);
  victim->position = POS_SLEEPING;
    }

    if(check_annointment(victim, ch))
    {
        spell_sleep(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}

void spell_slow( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
 
    if ( is_affected( victim, sn ) || IS_AFFECTED(victim,AFF_SLOW))
    {
    	if(reup_affect(victim,sn,level/2,level))
	{
    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM,FALSE);
	}
	else
	{
        if (victim == ch)
          send_to_char("You can't move any slower!\n\r",ch);
        else
          act("$N can't get any slower than that.", ch,NULL,victim,TO_CHAR,FALSE);
	}
        return;
    }

    if ( victim->race != race_lookup("faerie") )
        level = UMIN( level, 51 );

    if (saves_spell(level,victim,DAM_OTHER) 
    ||  IS_SET(victim->imm_flags,IMM_MAGIC))
    {
  if (victim != ch)
            send_to_char("Nothing seemed to happen.\n\r",ch);
        send_to_char("You feel momentarily lethargic.\n\r",victim);
        return;
    }
 
    if (IS_AFFECTED(victim,AFF_HASTE))
    {
        if (!check_dispel(level+2,victim,skill_lookup("haste")))
        {
      if (victim != ch)
              send_to_char("Spell failed.\n\r",ch);
            send_to_char("You feel momentarily slower.\n\r",victim);
            return;
        }

        act("$n is moving less quickly.",victim,NULL,NULL,TO_ROOM,FALSE);
    	level -= ( level / 10 );

	/* Another saves check to see if they still get slowed
	   since they lost their haste
	 */
        if( saves_spell(level,victim,DAM_OTHER))
	  return;
 
    }


    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/2;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = level/6;break;
        case 1:af.duration = level/4;break;
        case 2:af.duration = level/2;break;
        default:af.duration =level/2;break;
        }
    }
    af.location  = APPLY_DEX;
    af.modifier  = -1 - (level >= 18) - (level >= 25) - (level >= 32);
    af.bitvector = AFF_SLOW;
    affect_to_char( victim, &af );
    send_to_char( "You feel yourself slowing d o w n...\n\r", victim );
    act("$n starts to move in slow motion.",victim,NULL,NULL,TO_ROOM,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_slow(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}




void spell_stone_skin( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    int modifier;

    if ( is_affected(victim,gsn_steel_skin) || is_affected(victim,gsn_diamond_skin) || is_affected(victim,gsn_adamantite_skin) )
   {
	send_to_char("You failed.\n\r",ch);
	return;
   }

    if ( ch->race == race_lookup("rockbiter") )
    {
  if (victim == ch)
    send_to_char("Your skin is already as hard as a rock.\n\r",ch); 
  else
    act("$N is already as hard as can be.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    if ( is_affected( victim, sn ) )
    {
	act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM ,FALSE);
	send_to_char( "Your skin turns to stone.\n\r", victim );
	reup_affect(victim,sn,level,level);
	return;
    }

    modifier = -40;

    if ( ch == victim && ch->race == race_lookup("gargoyle") )
	modifier = UMIN(-40,-3*level/2);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level;
    af.location  = APPLY_AC;
    af.modifier  = modifier;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    act( "$n's skin turns to stone.", victim, NULL, NULL, TO_ROOM ,FALSE);
    send_to_char( "Your skin turns to stone.\n\r", victim );
    return;
}



void spell_summon( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim;
    char log_buf[MAX_STRING_LENGTH];
    CHAR_DATA *ich;
    bool legal;

    
    if ( ( victim = get_char_world( ch, target_name ) ) == NULL
    ||   victim == ch
    ||   victim->in_room == NULL
    ||   victim->in_room->vnum < 0
    ||   ch->in_room->vnum < 0
    ||   IS_SET(ch->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SAFE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_PRIVATE)
    ||   IS_SET(victim->in_room->room_flags, ROOM_SOLITARY)
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    ||   (IS_NPC(victim) && IS_SET(victim->act,ACT_AGGRESSIVE))
    ||   victim->level >= level + 3
    ||   (!IS_NPC(victim) && victim->level >= LEVEL_IMMORTAL)
    ||   victim->fighting != NULL
    ||   (IS_NPC(victim) && IS_SET(victim->imm_flags,IMM_SUMMON))
    ||	 (IS_NPC(victim) && IS_SET(victim->affected_by,AFF_CHARM))
    ||   (IS_NPC(victim) && victim->pIndexData->pShop != NULL)
    ||   (!IS_NPC(victim) && IS_SET(victim->act,PLR_NOSUMMON) && 
  !IS_SET(victim->act, PLR_DWEEB) )  
    ||   (IS_NPC(victim) && saves_spell( level, victim,DAM_OTHER)) 
    ||  victim->in_room->area->under_develop 
    ||  victim->in_room->area->no_transport
    ||   (IS_SET(ch->in_room->room_flags,ROOM_NOCLAN) && 
	  (is_clan(victim) || IS_SET(ch->mhs,MHS_HIGHLANDER)))
    ||  ch->in_room->area->no_transport 
    ||  is_bounty_target(victim, FALSE))

    {
  send_to_char( "You failed.\n\r", ch );
  return;
    }

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not summon.\n\r",ch);
       return;
    }

/* NIGHTDAGGER added code to prevent summoning mobs in with slept etc. people*/
    if(is_clan(ch) && is_clan(victim) && ch->pcdata->start_time > 0)
    {
      send_to_char("You just got here, you must wait before summoning them.\n\r", ch);
      return;
    }

    legal = TRUE;
    for ( ich = ch->in_room->people; ich != NULL; ich = ich->next_in_room)
    {
        if ( is_clan(ich) && ( IS_AFFECTED(ich, AFF_SLEEP) || is_affected(ich, skill_lookup("hold person")) || is_affected(ich, skill_lookup("garotte")) || is_affected(ich,gsn_trap) ))
        {
            legal = FALSE;
            break;
        }
    }

    if ( (!legal) && (IS_NPC(victim)) && ( (victim->spec_fun == spec_lookup("spec_guard_l")) || (victim->spec_fun == spec_lookup("spec_guard_d")) ) ) 
    {
        send_to_char("Your spell failed, for some reason...\n\r",ch);
        return;
    }
    
    act( "$n disappears suddenly.", victim, NULL, NULL, TO_ROOM ,FALSE);
    char_from_room( victim );
    char_to_room( victim, ch->in_room );
    clear_mount(ch);
    act( "$n arrives suddenly.", victim, NULL, NULL, TO_ROOM ,FALSE);
    act( "$n has summoned you!", ch, NULL, victim,   TO_VICT ,FALSE);
    if (!IS_NPC(victim))
    {
    sprintf(log_buf, "%s has summoned %s to Room [%d]", ch->name, victim->name, ch->in_room->vnum);
    log_string(log_buf);
    }

    do_look( victim, "auto" );
    return;
}



void spell_teleport( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *pRoomIndex;

    if ( is_affected(victim,gsn_trap) )
    {
        send_to_char("You are held fast by a snare trap.\n\r",victim);
        return;                                                                 
    }

  if( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
  {
    send_to_char("Your master didn't tell you to do that.\n\r",ch);
    return;
  }

    if ( victim->in_room == NULL
    ||   IS_SET(victim->in_room->room_flags, ROOM_NO_RECALL)
    || ( victim != ch && IS_SET(victim->imm_flags,IMM_SUMMON))
    || ( victim == ch && victim->fighting != NULL )
    || ( victim->in_room->area->no_transport )
    || ( IS_AFFECTED(victim,AFF_CURSE) && number_percent() < 75 )
    || ( is_affected(ch,gsn_morph) && number_percent() < 75 )
    || ( is_safe_spell(ch,victim,FALSE, sn))
    || ( IS_NPC(victim) && IS_SET(victim->act, ACT_AGGRESSIVE) )
    || ( victim != ch
    && ( saves_spell( level + (IS_AFFECTED(victim, AFF_CHARM)?1:-5), victim,DAM_OTHER))))
    {
  send_to_char( "You failed.\n\r", ch );
  return;
    }

    pRoomIndex = get_random_room(victim);

    if (victim != ch)
  send_to_char("You have been teleported!\n\r",victim);

    act( "$n vanishes!", victim, NULL, NULL, TO_ROOM ,FALSE);
    if (victim->fighting != NULL) stop_fighting(victim,FALSE);
    char_from_room( victim );
    char_to_room( victim, pRoomIndex );
    clear_mount(victim);
    act( "$n slowly fades into existence.", victim, NULL, NULL, TO_ROOM ,FALSE);
    do_look( victim, "auto" );

    if(check_annointment(victim, ch))
    {
        spell_teleport(sn,level+2,victim,ch,target);
    }   
    return;
}



void spell_ventriloquate( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char speaker[MAX_INPUT_LENGTH];
    CHAR_DATA *vch;

    target_name = one_argument( target_name, speaker );

    sprintf( buf1, "%s says '%s'.\n\r",              speaker, target_name );
    sprintf( buf2, "Someone makes %s say '%s'.\n\r", speaker, target_name );
    buf1[0] = UPPER(buf1[0]);

    for ( vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room )
    {
  if ( !is_name( speaker, vch->name ) )
      send_to_char( saves_spell(level,vch,DAM_OTHER) ? buf2 : buf1, vch );
    }

    return;
}



void spell_weaken( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
	send_to_char( "You feel your strength slip away.\n\r", victim );
	act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM,FALSE);
	reup_affect(victim,sn,level/3,level);
	return;
    }

    if ( saves_spell(level,victim,DAM_OTHER))
    {
       send_to_char("You failed.\n\r",ch);
       return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level / 3;
    if ( !IS_NPC(ch) )
    {
        switch( class_table[ch->class].fMana )
        {
        case 0:af.duration = level/7;break;
        case 1:af.duration = level/5;break;
        case 2:af.duration = level/3;break;
        default:af.duration = level/3;break;
        }
    }
    af.location  = APPLY_STR;
    af.modifier  = -1 * (level / 10);
    af.bitvector = AFF_WEAKEN;
    affect_to_char( victim, &af );
    send_to_char( "You feel your strength slip away.\n\r", victim );
    act("$n looks tired and weak.",victim,NULL,NULL,TO_ROOM,FALSE);

    if(check_annointment(victim, ch))
    {
        spell_weaken(sn,level+2,victim,ch,target);
    }   

    apply_mala_damage(ch, victim, MALA_NORMAL_DAMAGE);

    return;
}



/* RT recall spell is back */

void spell_word_of_recall( int sn, int level, CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    ROOM_INDEX_DATA *location;
    
    if ( is_affected(victim,gsn_trap) )
    {                                                                           
        send_to_char("You are held fast by a snare trap.\n\r",victim);
        return;                                                                 
    }  

  if( IS_AFFECTED(ch, AFF_CHARM) && ch->master != NULL)
  {
    send_to_char("Your master didn't tell you to do that.\n\r",ch);
    return;
  }

    if (IS_NPC(victim))
      return;

	if( is_safe(ch,victim) && !is_same_group(ch,victim) 
	    && !is_same_clan(ch,victim) )
	  return;
	else
        if( is_clan(ch) && is_clan(victim) && !is_same_group(ch,victim)
	    && !is_same_clan(ch,victim) )
	  check_killer(ch,victim);

    if(!is_same_group(ch,victim) && ch != victim &&
		      saves_spell(level,victim,DAM_OTHER))
	 return;
    location = get_room_index( ROOM_VNUM_TEMPLE );
    if ( location  == NULL )
    {
  send_to_char("You are completely lost.\n\r",victim);
  return;
    } 

    if (IS_SET(victim->in_room->room_flags,ROOM_NO_RECALL) ||
  (IS_AFFECTED(victim,AFF_CURSE) && number_percent() < 85) ||
  (is_affected(ch,gsn_morph) && number_percent() < 85)
       )
    {
  send_to_char("Spell failed.\n\r",victim);
  return;
    }

    if ( victim->in_room->area->no_transport )
    {
  send_to_char("Spell failed.\n\r",victim);
  return;
    }

    if (victim->fighting != NULL)
    stop_fighting(victim,TRUE);
    
    ch->move /= 2;
    act("$n disappears.",victim,NULL,NULL,TO_ROOM,FALSE);
    char_from_room(victim);
    clear_mount(victim);
    char_to_room(victim,location);
    act("$n appears in the room.",victim,NULL,NULL,TO_ROOM,FALSE);
    do_look(victim,"auto");
}

/*
 * NPC spells.
 */
void spell_acid_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam,hp_dam,dice_dam,hpch;

    act("$n spits acid at $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
    act("$n spits a stream of corrosive acid at you.",ch,NULL,victim,TO_VICT,FALSE);
    act("You spit acid at $N.",ch,NULL,victim,TO_CHAR,FALSE);

    hpch = UMAX(12,ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    
    if (saves_spell(level,victim,DAM_ACID))
    {
  acid_effect(victim,level/2,dam/4,TARGET_CHAR);
  damage(ch,victim,dam/2,sn,DAM_ACID,TRUE,TRUE);
    }
    else
    {
  acid_effect(victim,level,dam,TARGET_CHAR);
  damage(ch,victim,dam,sn,DAM_ACID,TRUE,TRUE);
    }
}



void spell_fire_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam,hp_dam,dice_dam;
    int hpch;

    act("$n breathes forth a cone of fire.",ch,NULL,victim,TO_NOTVICT,FALSE);
    act("$n breathes a cone of hot fire over you!",ch,NULL,victim,TO_VICT,FALSE);
    act("You breath forth a cone of fire.",ch,NULL,NULL,TO_CHAR,FALSE);

    hpch = UMAX( 10, ch->hit );
    hp_dam  = number_range( hpch/9+1, hpch/5 );
    dice_dam = dice(level,20);

    dam = UMAX(hp_dam + dice_dam /10, dice_dam + hp_dam / 10);
    fire_effect(victim->in_room,level,dam/2,TARGET_ROOM);

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
  vch_next = vch->next_in_room;

  if (is_safe_spell(ch,vch,TRUE, sn) 
  ||  (IS_NPC(vch) && IS_NPC(ch) 
  &&   (ch->fighting != vch || vch->fighting != ch)))
      continue;

  if (vch == victim) /* full damage */
  {
      if (saves_spell(level,vch,DAM_FIRE))
      {
    fire_effect(vch,level/2,dam/4,TARGET_CHAR);
    damage(ch,vch,dam/2,sn,DAM_FIRE,TRUE,TRUE);
      }
      else
      {
    fire_effect(vch,level,dam,TARGET_CHAR);
    damage(ch,vch,dam,sn,DAM_FIRE,TRUE,TRUE);
      }
  }
  else /* partial damage */
  {
      if (saves_spell(level - 2,vch,DAM_FIRE))
      {
    fire_effect(vch,level/4,dam/8,TARGET_CHAR);
    damage(ch,vch,dam/4,sn,DAM_FIRE,TRUE,TRUE);
      }
      else
      {
    fire_effect(vch,level/2,dam/4,TARGET_CHAR);
    damage(ch,vch,dam/2,sn,DAM_FIRE,TRUE,TRUE);
      }
  }
    }
}

void spell_frost_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    CHAR_DATA *vch, *vch_next;
    int dam,hp_dam,dice_dam, hpch;

    act("$n breathes out a freezing cone of frost!",ch,NULL,victim,TO_NOTVICT,FALSE);
    act("$n breathes a freezing cone of frost over you!",
  ch,NULL,victim,TO_VICT,FALSE);
    act("You breath out a cone of frost.",ch,NULL,NULL,TO_CHAR,FALSE);

    hpch = UMAX(12,ch->hit);
    hp_dam = number_range(hpch/11 + 1, hpch/6);
    dice_dam = dice(level,16);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    cold_effect(victim->in_room,level,dam/2,TARGET_ROOM); 

    for (vch = victim->in_room->people; vch != NULL; vch = vch_next)
    {
  vch_next = vch->next_in_room;

  if (is_safe_spell(ch,vch,TRUE, sn)
  ||  (IS_NPC(vch) && IS_NPC(ch) 
  &&   (ch->fighting != vch || vch->fighting != ch)))
      continue;

  if (vch == victim) /* full damage */
  {
      if (saves_spell(level,vch,DAM_COLD))
      {
    cold_effect(vch,level/2,dam/4,TARGET_CHAR);
    damage(ch,vch,dam/2,sn,DAM_COLD,TRUE,TRUE);
      }
      else
      {
    cold_effect(vch,level,dam,TARGET_CHAR);
    damage(ch,vch,dam,sn,DAM_COLD,TRUE,TRUE);
      }
  }
  else
  {
      if (saves_spell(level - 2,vch,DAM_COLD))
      {
    cold_effect(vch,level/4,dam/8,TARGET_CHAR);
    damage(ch,vch,dam/4,sn,DAM_COLD,TRUE,TRUE);
      }
      else
      {
    cold_effect(vch,level/2,dam/4,TARGET_CHAR);
    damage(ch,vch,dam/2,sn,DAM_COLD,TRUE,TRUE);
      }
  }
    }
}

    
void spell_gas_breath( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *vch;
    CHAR_DATA *vch_next;
    int dam,hp_dam,dice_dam,hpch;

    act("$n breathes out a cloud of poisonous gas!",ch,NULL,NULL,TO_ROOM,FALSE);
    act("You breath out a cloud of poisonous gas.",ch,NULL,NULL,TO_CHAR,FALSE);

    hpch = UMAX(16,ch->hit);
    hp_dam = number_range(hpch/15+1,8);
    dice_dam = dice(level,12);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);
    poison_effect(ch->in_room,level,dam,TARGET_ROOM);

    for (vch = ch->in_room->people; vch != NULL; vch = vch_next)
    {
  vch_next = vch->next_in_room;

  if (is_safe_spell(ch,vch,TRUE, sn)
  ||  (IS_NPC(ch) && IS_NPC(vch) 
  &&   (ch->fighting == vch || vch->fighting == ch)))
      continue;

  if (saves_spell(level,vch,DAM_POISON))
  {
      poison_effect(vch,level/2,dam/4,TARGET_CHAR);
      damage(ch,vch,dam/2,sn,DAM_POISON,TRUE,TRUE);
  }
  else
  {
      poison_effect(vch,level,dam,TARGET_CHAR);
      damage(ch,vch,dam,sn,DAM_POISON,TRUE,TRUE);
  }
    }
}

void spell_lightning_breath(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam,hp_dam,dice_dam,hpch;

    act("$n breathes a bolt of lightning at $N.",ch,NULL,victim,TO_NOTVICT,FALSE);
    act("$n breathes a bolt of lightning at you!",ch,NULL,victim,TO_VICT,FALSE);
    act("You breathe a bolt of lightning at $N.",ch,NULL,victim,TO_CHAR,FALSE);

    hpch = UMAX(10,ch->hit);
    hp_dam = number_range(hpch/9+1,hpch/5);
    dice_dam = dice(level,20);

    dam = UMAX(hp_dam + dice_dam/10,dice_dam + hp_dam/10);

    if (saves_spell(level,victim,DAM_LIGHTNING))
    {
  shock_effect(victim,level/2,dam/4,TARGET_CHAR);
  damage(ch,victim,dam/2,sn,DAM_LIGHTNING,TRUE,TRUE);
    }
    else
    {
  shock_effect(victim,level,dam,TARGET_CHAR);
  damage(ch,victim,dam,sn,DAM_LIGHTNING,TRUE,TRUE);
    }
}

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
void spell_general_purpose(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
 
    dam = number_range( 25, 100 );
    if ( saves_spell( level, victim, DAM_PIERCE) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE ,TRUE,TRUE);
    return;
}

void spell_high_explosive(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;
 
    dam = number_range( 35, 125 );
    if ( saves_spell( level, victim, DAM_PIERCE) )
        dam /= 2;
    damage( ch, victim, dam, sn, DAM_PIERCE ,TRUE,TRUE);
    return;
}

void spell_animate_dead ( int sn, int level, CHAR_DATA *ch, void *vo, int targ) 
{
  CHAR_DATA *victim;
  OBJ_DATA *obj,*obj2,*next_obj;
  int t,perc,count;
  char *target;
  char str [MAX_INPUT_LENGTH];
  sh_int fail;

  count = 0;
  for ( victim = char_list; victim != NULL; victim = victim->next )
    if ( is_same_group( victim, ch ) && IS_SET(victim->act,ACT_UNDEAD))
      count++;

  if ((count > (ch->level/10)) && !IS_IMMORTAL(ch)) {
    send_to_char ("Your controlling as many undead as you can handle.",ch);
    return;
  }
  if (target_name == "") {
    target = "corpse";
  } else {
    target = target_name;
  }
  obj = get_obj_list( ch, target, ch->in_room->contents );
  if ( obj == NULL )
  {
     act( "I see no $T on the ground here.", ch, NULL, target, TO_CHAR ,FALSE);
     return;
  }
  if ((obj->pIndexData->vnum != OBJ_VNUM_CORPSE_NPC)) {
    send_to_char ("That object cannot be animated.\n\r",ch);
    return;
  }
  
  /* maybe throw in a check for no_loot? */
  if (obj->owner != NULL && !IS_SET(obj->extra_flags,ITEM_CLAN_CORPSE)) {
    send_to_char ("Leave that corpse alone!\n\r",ch);
    return;
  }

  fail = 0;
  if (obj->level > (ch->level*2)) {
    fail = 1;                        /* Nope.. corpse too high */
  } else {
    perc = 100;
    if (obj->level > (ch->level-3)) {
      perc = (ch->level*2 - obj->level)*10;
      if (number_percent() > perc)
        fail = 1;
    }
  }

  if (IS_IMMORTAL (ch)) fail = 0;

  if (fail) {
    send_to_char ("Spell failed.\n\r",ch);
    return;
  }

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not animate dead.\n\r",ch);
       return;
    }

  /* let's create a corpse mob */
  if (obj->value[3]) {          /* weaken him up a bit */
    victim = create_mobile ( get_mob_index(obj->value[3] ));
    victim->level = UMAX (1,victim->level*2/3);
    victim->max_hit = victim->max_hit*2/3+1;
    victim->hit = victim->max_hit;
    victim->max_mana = victim->max_mana*2/3+1;
    victim->mana = victim->max_mana;
    victim->damage[DICE_NUMBER] = victim->damage[DICE_NUMBER]*2/3+1;    
    victim->damroll = victim->damroll*2/3+1;
    SET_BIT (victim->res_flags,victim->imm_flags);
    victim->imm_flags = 0;
  } else {
    victim = create_mobile ( get_mob_index( MOB_VNUM_CORPSE ));
    victim->level = obj->level*2/3+1;
    for (t = 0; t < 3; t++)
      victim->armor[t] = interpolate(victim->level,100,-100);
    victim->armor[3] = interpolate(victim->level,100,0);
    victim->max_hit = victim->level * 8 + number_range(
          victim->level * victim->level/8,
          victim->level * victim->level);
    victim->hit = victim->max_hit;
    victim->max_mana = 100;
    victim->mana = 100;
    victim->damage[DICE_NUMBER] = victim->level/8+1;
    victim->damage[DICE_TYPE]   = 5;
    victim->hitroll = victim->level;
    victim->damroll = victim->level/2;
  }  
  SET_BIT (victim->form, FORM_UNDEAD);
  SET_BIT (victim->form, FORM_INSTANT_DECAY);
  if ( IS_SET(victim->act, ACT_AGGRESSIVE) )
	REMOVE_BIT(victim->act,ACT_AGGRESSIVE);
  if(victim->spec_fun) victim->spec_fun = NULL;
  char_to_room (victim,ch->in_room);
  free_string( victim->long_descr );
  free_string( victim->short_descr );  
  free_string( victim->name );
  strcpy (str,obj->short_descr); 
  strcat (str," is twitching here.\n\r");  
  victim->long_descr = str_dup (capitalize (str));
  victim->short_descr = str_dup ("the corpse");
  strcpy (str,victim->name);
  strcat (str," corpse");
  victim->name = str_dup (str);
  SET_BIT (victim->act,ACT_UNDEAD); /* just to make sure */

  for (obj2 = obj->contains; obj2 != NULL; obj2 = next_obj) {
    next_obj = obj2->next_content;
    obj_from_obj (obj2);
    obj_to_char (obj2,victim);
  } /* copy the objects over */

  act( "$N raises from the dead and bows before $n.",   ch, NULL, victim, TO_ROOM ,FALSE);
  act( "$N raises from the dead and bows before you.", ch, NULL, victim, TO_CHAR ,FALSE);

  add_follower( victim, ch );
  //victim->leader = ch;
  add_to_group(victim, ch);
  SET_BIT(victim->affected_by,AFF_CHARM); /* quick-charm */
  victim->life_timer = obj->timer*2;
  if (victim->life_timer == 0) {
    victim->life_timer = 10;
  }

  obj_from_room (obj); /* delete the corpse object */
  extract_obj (obj);

  if( !is_affected(ch, skill_lookup("indulgence")) )
    {
      ch->alignment = UMAX(-1000, ch->alignment - number_range (50,100));
    }
}

void spell_summon_dead (  int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  CHAR_DATA *victim;
  int t,count;

  if ( is_affected(ch,skill_lookup("wound transfer")) )
  {
	send_to_char("You failed.\n\r",ch);
	return;
  }

  if (IS_SET(ch->mhs,MHS_HIGHLANDER))
  {
     send_to_char("Honorable combat is one on one. Highlanders can not summon dead.\n\r",ch);
     return;
  }

    if(IS_SET(ch->mhs,MHS_GLADIATOR))
    {
       send_to_char(" Gladiators can not summon dead.\n\r",ch);
       return;
    }
  count = 0;
  for ( victim = char_list; victim != NULL; victim = victim->next )
    if ( (is_same_group( victim, ch ) && (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD)))
      || (is_same_group(victim->master, ch) && (IS_NPC(victim) && IS_SET(victim->act,ACT_UNDEAD))) )
      count++;
  if ( (ch->kit == kit_lookup("necromancer")) && (count > (ch->level/7)) && !IS_IMMORTAL(ch) )
  {
    send_to_char ("You're controlling as many undead as you can handle.",ch);
    return;
  }

  if ( ((count > (ch->level/10)) && (ch->kit != kit_lookup("necromancer")) && !IS_IMMORTAL(ch))  ) 
  {
    send_to_char ("You're controlling as many undead as you can handle.",ch);
    return;
  }

  /* let's create a skeletal warrior mob */
  victim = create_mobile ( get_mob_index( MOB_VNUM_SKEL_WAR ));
  char_to_room (victim,ch->in_room);
  if ( ch->kit == kit_lookup("necromancer") )
  {
    victim->level = level *4/5 + 3;
  }
  else
  {
    victim->level = level * 4/5+1;
  }
  //victim->level = level * 4/5+1;  
  for (t = 0; t < 3; t++)
    victim->armor[t] = interpolate(victim->level,100,-100);
  victim->armor[3] = interpolate(victim->level,100,0);
  victim->max_hit = level * 8 + number_range(
          level * level / 8, level * level );
  victim->max_hit = UMAX( victim->max_hit, ch->max_hit * 3 /4);
  victim->hit = victim->max_hit;
  victim->max_mana = 100;
  victim->mana = 100;
  victim->damage[DICE_NUMBER] = victim->level/4+1;
  victim->damage[DICE_TYPE]   = 5;
  if ( ch->kit == kit_lookup("necromancer") )
  {
    victim->hitroll = victim->level + number_range(1,4);
    victim->damroll = victim->level/2 + number_range(1,4);
  }
  else
  {
    victim->hitroll = victim->level;
    victim->damroll = victim->level/2;
  }
  //victim->hitroll = victim->level;
  //victim->damroll = victim->level/2;
  SET_BIT (victim->act,ACT_UNDEAD); /* just to make sure */

  act( "A dark hole opens to reveal a skeletal warrior.",ch, NULL, victim,TO_ROOM,FALSE);
  act( "A dark hole opens to reveal a skeletal warrior.",ch, NULL, victim,TO_CHAR,FALSE);

  add_follower( victim, ch );
  //victim->leader = ch;
  add_to_group(victim, ch);
  SET_BIT(victim->form,FORM_INSTANT_DECAY);
  SET_BIT(victim->affected_by,AFF_CHARM); /* quick-charm */
  if ( ch->kit == kit_lookup("necromancer") )
  {
    victim->life_timer = ch->level/3+10;
  }
  else
  {
    victim->life_timer = ch->level/5+10;
  }

  //victim->life_timer = ch->level/5+10;

  if( !is_affected(ch, skill_lookup("indulgence")) )
    {
      ch->alignment = UMAX(-1000, ch->alignment - number_range (50,70));
    }
}

void spell_cryogenesis ( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  OBJ_DATA *obj = vo;
  int vnum;

  vnum = obj->pIndexData->vnum;

  if ((vnum >= OBJ_VNUM_CORPSE_NPC) &&
      (vnum <= OBJ_VNUM_BRAINS)) {
    obj->timer = 0;
    act("$p looks more solid.",ch,obj,NULL,TO_CHAR,FALSE);
    act("$p looks more solid.",ch,obj,NULL,TO_ROOM,FALSE);
  } else {
    send_to_char ("That object cannot be preserved.\n\r",ch);
    return;
  }
}

void spell_draw_life ( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  OBJ_DATA *obj = vo,*obj2,*next_obj;
  int vnum,hp;

  vnum = obj->pIndexData->vnum;
  if ((vnum == OBJ_VNUM_CORPSE_NPC) ||
      (vnum == OBJ_VNUM_CORPSE_PC)) {
    if (obj->owner != NULL) {
      send_to_char ("Not on that corpse!\n\r",ch);
      return;
    }
    hp = number_range (obj->level*2,4*obj->level);
    if (hp > 120) hp = 120;
    if (hp < 20) hp = 20;
    
  if( (!is_affected(ch, skill_lookup("indulgence"))) || (ch->kit != kit_lookup("necromancer")) )
    ch->alignment = UMAX(-1000, ch->alignment - number_range (40,70));

    act("You draw the last remains of energy from the corpse.",ch,NULL,NULL,TO_CHAR,FALSE);
    act("As the corpse disintegrates, $n looks healthier.",ch,NULL,NULL,TO_ROOM,FALSE);

    ch->hit += hp;   /* hit can go over max_hit by level/2 */
    if (ch->hit > (ch->max_hit + (ch->level/2))) ch->hit = ch->max_hit + (ch->level/2);

    for (obj2 = obj->contains; obj2 != NULL; obj2 = next_obj) {
      next_obj = obj2->next_content;
      obj_from_obj (obj2);
      if ( obj->item_type == ITEM_MONEY) {
        ch->silver += obj->value[0];
        ch->gold += obj->value[1];
        extract_obj( obj );
      } else {      
        obj_to_char (obj2,ch);
      }
    }                    /* copy the objects to the carrier */
    obj_from_char (obj); /* delete the corpse object */
    extract_obj (obj);
  } else {
    send_to_char ("You cannot draw life from that object.\n\r",ch);
    return;
  }
}


void spell_turn_undead ( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
  CHAR_DATA *gch,*next_char;
  OBJ_DATA *obj,*next_obj;
  sh_int found = 0;
  sh_int skl = 0;
  sh_int passedLevel =0;

  for ( gch = ch->in_room->people; gch != NULL; gch = next_char ) 
  {
    next_char = gch->next_in_room;
    
    if (IS_SET((gch)->form, FORM_UNDEAD) && IS_NPC(gch)) 
    {
      found = TRUE;

	// Commented out as it seems very HARD to turn skellies this way
	// putting it back it, it seems really easy to turn skellies
      if (gch->leader != NULL)
      {
//	skl = get_skill(gch->leader,skill_lookup("summon dead"));
//	skl /= 15 ; 
        if (gch->leader->kit == kit_lookup("necromancer"))
	{
	  skl = get_skill(gch->leader,skill_lookup("summon dead"));
	  skl /= 5;
	}
      }
      //
      if (saves_spell (level-skl,gch,DAM_HOLY)) 
      {
  	act("$n seems unaffected.",gch,NULL,NULL,TO_ROOM,FALSE);
  	if (IS_AWAKE(gch) && gch->fighting == NULL) 
	{
    	act("$n twitches and attacks!",gch,NULL,NULL,TO_ROOM,FALSE);
        if (gch->leader == ch) 
	{
       	   REMOVE_BIT(gch->affected_by,AFF_CHARM);
           //gch->leader = NULL;
           remove_from_group(gch);
           gch->master = NULL;
    	}
  	multi_hit(gch,ch,TYPE_UNDEFINED);
  	}
      } 
      else 
      {
  act("$n twitches violently and crumbles into dust.",gch,NULL,NULL,TO_ROOM,FALSE);
  for (obj = gch->carrying; obj != NULL; obj = next_obj) {
    next_obj = obj->next_content;
    obj_from_char (obj);
    obj_to_room (obj,ch->in_room);
      }                    /* copy the objects to the room */

  stop_fighting (gch,FALSE);
  gch->pIndexData->killed++;
  kill_table[URANGE(0, gch->level, MAX_LEVEL-1)].killed++;
  extract_char( gch, TRUE );
      }
    }
  }
  if (!found) {
    send_to_char ("You don't see any undead here.\n\r",ch);
  }
}
/*
void spell_feign_death( int sn, int level, CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected(victim,sn) )
    {
	send_to_char("You are already falsifying your own destruction.\n\r",victim);
	return;
    }

    af.where		= TO_AFFECTS;
    af.type		= sn;
    af.level		= level;
    af.duration		= level/6;
    af.modifier		= 0;
    af.location		= 0;
    af.bitvector	= 0;
    affect_to_char(victim,&af);

    act("Your flesh rots away into the visage of the dead.",victim,NULL,NULL,TO_CHAR,FALSE);
    act("$n's flesh rots away to reveal the visage of the dead.",victim,NULL,NULL,TO_ROOM,FALSE);
    return;
}
*/

void spell_wound_transfer( int sn, int level, CHAR_DATA *ch, void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;
    CHAR_DATA *gch;

    if ( is_affected( victim, sn ) )
    {
	send_to_char("You are already prepared to transfer wounds.\n\r",ch);
	return;
    }

    for ( gch = char_list;
	  gch != NULL ;
	  gch = gch->next )
    {
	if ( is_same_group( ch, gch ) )
	{
 	    if ( is_affected(gch,gsn_wound_transfer) )
	    {
   send_to_char("Another group member is transferring.\n\r",ch);
   return;
	    }

	    if ( (is_clan(victim) && !is_clan(gch)) ||
		(!is_clan(victim) &&  is_clan(gch)) )
	    {
   send_to_char("This spell cannot cross clan boundries.\n\r",victim);
   return;
	    }
       }
   }
    af.where	= TO_AFFECTS;
    af.type	= sn;
    af.level	= level;
    af.duration = dice(1,3) + 3;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = 0;

    affect_to_char( victim, &af );
    send_to_char("Your soul is prepared to share your pain.\n\r",ch);

    return;
}

void spell_withstand_death ( int sn, int level, CHAR_DATA *ch, void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( IS_AFFECTED(victim, AFF_WITHSTAND_DEATH) )
    {
       send_to_char("You are already more powerful then death.\n\r",ch);
       return;
    }

    if (IS_SET(ch->mhs,MHS_GLADIATOR) && 
        gladiator_info.started == TRUE && gladiator_info.bet_counter < 1)
    {
       send_to_char("You may not become more powerful then death in the arena.\n\r", ch);
       return;
    }

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = level/5+5;
    af.location  = APPLY_NONE;
    af.modifier  = 0;
    af.bitvector = AFF_WITHSTAND_DEATH;
    affect_to_char( victim, &af );
    send_to_char( "You feel like you can withstand death itself.\n\r", victim );
    act( "$n looks more powerful than death.", victim, NULL, NULL, TO_ROOM ,FALSE);
    
  if( !is_affected(ch, skill_lookup("indulgence")) )
    ch->alignment = UMAX(-1000, ch->alignment - number_range (20,50));
    
    return;
}

void spell_lay_on_hands( int sn, int level, CHAR_DATA *ch, void *vo,int target )
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int HchLoses,HvictNeeds;

    if ( IS_AFFECTED(ch,AFF_CHARM) )
	return;

    if( ch->fighting != NULL || victim->fighting != NULL)
	{
	send_to_char("Things need to be calmer.\n\r",ch);
	return;
	}

    if(victim != ch )
    {

     HvictNeeds = victim->max_hit - victim->hit;
     HchLoses = UMIN( ch->hit +3, HvictNeeds );

     victim->hit = UMIN( victim->hit + HchLoses, victim->max_hit );
     update_pos( victim );
     send_to_char( "A warm feeling fills your body.\n\r", victim );

     ch->hit = UMAX( -3, ch->hit - HchLoses);
     update_pos(ch);
     send_to_char( "You force your life out through your hands.\n\r",ch);
    }
    return;
}

void spell_psionic_blast(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    int dam;

    dam = dice( (level*7)/10, 
          (get_curr_stat(ch,STAT_CON) + get_curr_stat(ch,STAT_INT)) / 4 );
    if ( saves_spell( level, victim, DAM_MENTAL ) )
  dam /= 2;
    damage( ch, victim, dam, sn,DAM_MENTAL,TRUE,TRUE);
    /* now we hit the caster */
    dam /= 8;
    damage(ch,ch,dam,sn,DAM_MENTAL,TRUE,TRUE);
    return;
}

void spell_body_weaponry(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    if ( is_affected( victim, sn ) )
    {
  if (victim == ch)
    send_to_char("Your body is already equipped.\n\r",ch);
  else
    act("$N's body is already equipped.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }
    af.where   = TO_AFFECTS;
    af.type      = sn;
    af.level   = level;
    af.duration  = 24;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = AFF_WEAPONRY;
    affect_to_char( victim, &af );
    send_to_char( "Your body feels protected and armed.\n\r", victim );
    if ( ch != victim )
  act("$N's body becomes equipped for combat.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}

void spell_tower_of_iron_will(int sn,int level,CHAR_DATA *ch,void *vo,int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

  if ( is_affected( victim, sn ) || IS_SET(victim->res_flags,RES_MENTAL))
    {
  if (victim == ch)
    send_to_char("Your will has already peaked.\n\r",ch);
  else
   act("$N is already strong willed.",ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }
  // Those vulns shouldn't be covered easily
  if ( IS_SET(victim->vuln_flags,VULN_MENTAL))
    {
  if (victim == ch)
    send_to_char("You thought you already took care of that.\n\r",ch);
  else
    act("$N is too easily swayed to make use of your help.",
	ch,NULL,victim,TO_CHAR,FALSE);
  return;
    }

    af.where   = TO_RESIST;
    af.type      = sn;
    af.level   = level;
    af.duration  = level / 2;
    af.location = APPLY_NONE;
    af.modifier = 0;
    af.bitvector = RES_MENTAL;
    affect_to_char( victim, &af );


    send_to_char( "Your will can not be broken.\n\r", victim );
    if ( ch != victim )
  act("$N's will can not be broken.",ch,NULL,victim,TO_CHAR,FALSE);
    return;
}

void spell_mirror_image ( int sn, int level, CHAR_DATA *ch, void *vo, int targ)
{
  CHAR_DATA *victim, *fch;
  char descr[MAX_STRING_LENGTH];
  int t;

  if( is_mounted(ch) && IS_NPC(ch->riding) && ch->riding->pIndexData->vnum == MOB_VNUM_WARHORSE)
  {
    send_to_char("Your magic is not strong enough to mirror your handy dandy warhorse.\n\r",ch);
    return;
  }

  if((fch = ch->fighting) == NULL)
    {
	send_to_char("But you aren't fighting anyone.\n\r",ch);
	return;
    }

    victim = create_mobile ( get_mob_index( MOB_VNUM_MIRROR_IMAGE ));
    victim->level = ch->level*2/3+1;
    for (t = 0; t < 3; t++)
      victim->armor[t] = interpolate(victim->level,100,-100);
    victim->armor[3] = interpolate(victim->level,100,0);
    victim->max_hit = level;
    victim->hit = victim->max_hit;
    victim->max_mana = 100;
    victim->mana = 100;
    victim->damage[DICE_NUMBER] = 1;
    victim->damage[DICE_TYPE]   = 5;
    victim->hitroll = victim->level;
    victim->damroll = 2;
  SET_BIT (victim->form, FORM_INSTANT_DECAY);
  if ( IS_SET(victim->act, ACT_AGGRESSIVE) )
        REMOVE_BIT(victim->act,ACT_AGGRESSIVE);
  SET_BIT (victim->imm_flags, IMM_CHARM);
  SET_BIT (victim->imm_flags, IMM_MENTAL);
  char_to_room (victim,ch->in_room);
/*  free_string( victim->long_descr );
  free_string( victim->short_descr );
  free_string( victim->name ); */
  sprintf(descr,"%s %s is here.\n\r",ch->name,ch->pcdata->title);
  victim->long_descr = str_dup (descr);
  victim->short_descr = str_dup (ch->name);
  victim->name = str_dup (ch->name);

  send_to_char("Your mirror image fools your opponent, ",ch);
  if(IS_SET(ch->display,DISP_COLOR))
   send_to_char( GREEN"RUN!"NORMAL"\n\r", ch );
  else
   send_to_char( "RUN!\n\r", ch );


  stop_fighting( fch, FALSE );
  stop_fighting( ch, FALSE );
  set_fighting(victim,fch);
  set_fighting(fch,victim);

  return;
}

void spell_honor_guard(int sn,int level,CHAR_DATA *ch,void *vo, int target)
{
    CHAR_DATA *victim = (CHAR_DATA *) vo;
    AFFECT_DATA af;

    send_to_char("Sorry, this isn't a spell anymore.\n\r",ch);
    return;

    if( is_affected(victim,skill_lookup("honor guard")) )
	{
	    send_to_char("Just how much more honorable do you want to be?\r\n",victim);
	    return;
	}

    if (ch != victim)
        {
         send_to_char("You may not cast this on another.\r\n",victim);
         return;
        }

        check_improve(ch,gsn_honor_guard,TRUE,8);

    af.where     = TO_AFFECTS;
    af.type      = sn;
    af.level     = level;
    af.duration  = victim->level;
    af.location  = APPLY_SAVING_SPELL;
    af.modifier  = -3;
    af.bitvector = 0;
    affect_to_char( victim, &af );
    send_to_char( "You are surrounded by a shield of honor.\n\r", victim );
    return;
}
void write_spell( CHAR_DATA *ch, int sn )
{
    static char buf  [MAX_STRING_LENGTH];
    static char buf2 [MAX_STRING_LENGTH];
    char *pName;
    int iSyl;
    int length;

    struct syl_type
    {
  char *  old;
  char *  new;
    };

    static const struct syl_type syl_table[] =
    {
  { " ",    " "   },
  { "ar",   "abra"    },
  { "au",   "kada"    },
  { "bless",  "fido"    },
  { "blind",  "nose"    },
  { "bur",  "mosa"    },
  { "cu",   "judi"    },
  { "de",   "oculo"   },
  { "en",   "unso"    },
  { "light",  "dies"    },
  { "lo",   "hi"    },
  { "mor",  "zak"   },
  { "move", "sido"    },
  { "ness", "lacri"   },
  { "ning", "illa"    },
  { "per",  "duda"    },
  { "ra",   "gru"   },
  { "fresh",  "ima"   },
  { "re",   "candus"  },
  { "son",  "sabru"   },
  { "tect", "infra"   },
  { "tri",  "cula"    },
  { "ven",  "nofo"    },
  { "wall",  "denca"    },
  { "a", "a" }, { "b", "b" }, { "c", "q" }, { "d", "e" },
  { "e", "z" }, { "f", "y" }, { "g", "o" }, { "h", "p" },
  { "i", "u" }, { "j", "y" }, { "k", "t" }, { "l", "r" },
  { "m", "w" }, { "n", "i" }, { "o", "a" }, { "p", "s" },
  { "q", "d" }, { "r", "f" }, { "s", "g" }, { "t", "h" },
  { "u", "j" }, { "v", "z" }, { "w", "x" }, { "x", "n" },
  { "y", "l" }, { "z", "k" },
  { "", "" }
    };

    buf[0]  = '\0';
    for ( pName = skill_table[sn].name; *pName != '\0'; pName += length )
    {
  for ( iSyl = 0; (length = strlen(syl_table[iSyl].old)) != 0; iSyl++ )
  {
      if ( !str_prefix( syl_table[iSyl].old, pName ) )
      {
    strcat( buf, syl_table[iSyl].new );
    break;
      }
  }

  if ( length == 0 )
      length = 1;
    }

    sprintf( buf2, "%s", buf );
    sprintf( buf,  "%s", skill_table[sn].name );


      if ( get_skill(ch,gsn_spellcraft) >= number_percent() )
      {
        check_improve(ch,gsn_spellcraft,TRUE,10);
	send_to_char(buf,ch);
	return;
      }
      else
	send_to_char(buf2,ch);
	return;
}
