/* Healer code written for Merc 2.0 muds by Alander 
   direct questions or comments to rtaylor@cie-2.uoregon.edu
   any use of this code must include this header */

static char rcsid[] = "$Id: healer.c,v 1.9 2001/09/05 16:29:16 boogums Exp $";
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
#include "magic.h"
#include "gladiator.h"

void do_heal(CHAR_DATA *ch, char *argument)
{
    CHAR_DATA *mob;
    char arg[MAX_INPUT_LENGTH];
    int cost,sn;
    SPELL_FUN *spell;
    char *words;
    bool glad_healing = FALSE;

    /* check for healer */
    for ( mob = ch->in_room->people; mob; mob = mob->next_in_room )
    {
        if ( IS_NPC(mob) && IS_SET(mob->act, ACT_IS_HEALER) )
            break;
    }
 
    if ( mob == NULL )
    {
        send_to_char( "You can't do that here.\n\r", ch );
        return;
    }

    if( is_affected(ch,skill_lookup("wraithform")) )
	{
	send_to_char("You are to scary looking right now. \r\n", ch);
	return;
	}


    if( !IS_NPC(ch) && ch->clan && ch->in_room->clan 
	&& IS_SET(ch->pcdata->clan_flags, CLAN_NO_HEALER) )
	{
	act("$N says 'I have been forbidden to provide you with healing.'", ch, NULL, mob, TO_CHAR, FALSE);
	return;
	}

    if ( ch->in_room->clan && ch->in_room->clan != ch->clan && !IS_IMMORTAL(ch))
       {
       act("$N says 'I do not service those not of my clan, begone foul rogue!'",ch,NULL,mob,TO_CHAR,FALSE);
       return;
       }
    one_argument(argument,arg);
    if(IS_SET(ch->mhs, MHS_GLADIATOR) && gladiator_info.exper == TRUE)
	glad_healing = TRUE;

    if (arg[0] == '\0')
    {
        /* display price list */
	act("$N says 'I offer the following spells:'",ch,NULL,mob,TO_CHAR,FALSE);
	send_to_char("  light: cure light wounds      10 gold\n\r",ch);
	send_to_char("  serious: cure serious wounds  15 gold\n\r",ch);
	send_to_char("  critic: cure critical wounds  25 gold\n\r",ch);
	send_to_char("  heal: healing spell	      50 gold\n\r",ch);
	send_to_char("  blind: cure blindness         20 gold\n\r",ch);
	send_to_char("  disease: cure disease         15 gold\n\r",ch);
	send_to_char("  poison:  cure poison	      25 gold\n\r",ch); 
	send_to_char("  uncurse: remove curse	      50 gold\n\r",ch);
	send_to_char("  refresh: restore movement      5 gold\n\r",ch);
	send_to_char("  mana:  restore mana	      10 gold\n\r",ch);
	if(glad_healing)
	{
		send_to_char("  favor:    bless                FREE\n\r",ch);
		send_to_char("  armor:    armor                FREE\n\r",ch);
		send_to_char("  glow:     sanctuary            FREE\n\r",ch);
		send_to_char("  haste:    haste                FREE\n\r",ch);
		send_to_char("  giant:    giant strength       FREE\n\r",ch);
		send_to_char("  shield:   shield               FREE\n\r",ch);
		send_to_char("  fury:     frenzy               FREE\n\r",ch);
		send_to_char("  cancel:   cancellation         FREE\n\r",ch);
	}
	else
	{
		send_to_char("  favor:    bless                5 gold\n\r",ch);
		send_to_char("  armor:    armor                8 gold\n\r",ch);
		send_to_char("  glow:   sanctuary	      80 gold\n\r",ch);
	}
        if(IS_SET(ch->mhs, MHS_GLADIATOR) && gladiator_info.WNR == TRUE)
        {
		send_to_char("  gladiate: spell up             FREE\n\r",ch);
        }
	send_to_char(" Type heal <type> to be healed.\n\r",ch);
	return;
    }

    if (!str_prefix(arg,"light"))
    {
	    spell = spell_cure_light;
	    sn    = skill_lookup("cure light");
	    words = "judicandus dies";
	    cost  = 1000;
    }

    else if (!str_prefix(arg,"serious"))
    {
	    spell = spell_cure_serious;
	    sn    = skill_lookup("cure serious");
	    words = "judicandus gzfuajg";
	    cost  = 1600;
    }

    else if (!str_prefix(arg,"critical"))
    {
	    spell = spell_cure_critical;
	    sn    = skill_lookup("cure critical");
	    words = "judicandus qfuhuqar";
	    cost  = 2500;
    }

    else if (!str_prefix(arg,"heal"))
    {
	    spell = spell_heal;
	    sn = skill_lookup("heal");
	    words = "pzar";
	    cost  = 5000;
    }

    else if (!str_prefix(arg,"blindness"))
    {
	    spell = spell_cure_blindness;
	    sn    = skill_lookup("cure blindness");
	    words = "judicandus noselacri";		
	    cost  = 2000;
    }

    else if (!str_prefix(arg,"disease"))
    {
	    spell = spell_cure_disease;
	    sn    = skill_lookup("cure disease");
	    words = "judicandus eugzagz";
	    cost = 1500;
    }

    else if (!str_prefix(arg,"poison"))
    {
	    spell = spell_cure_poison;
	    sn    = skill_lookup("cure poison");
	    words = "judicandus sausabru";
	    cost  = 2500;
    }

    else if (!str_prefix(arg,"uncurse") || !str_prefix(arg,"curse"))
    {
	    spell = spell_remove_curse; 
	    sn    = skill_lookup("remove curse");
	    words = "candussido judifgz";
	    cost  = 5000;
    }

    else if (!str_prefix(arg,"mana") || !str_prefix(arg,"energize"))
    {
	    spell = NULL;
	    sn = -1;
	    words = "energizer";
	    cost = 1000;
    }


    else if (!str_prefix(arg,"refresh") || !str_prefix(arg,"moves"))
    {
	    spell =  spell_refresh;
	    sn    = skill_lookup("refresh");
	    words = "candusima"; 
	    cost  = 500;
    }

    else if (!str_prefix(arg,"favor") || !str_prefix(arg,"bless"))
    {
	    spell = spell_bless;
	    sn    = skill_lookup("bless");
	    words = "fido";
	    if(glad_healing == TRUE)
		    cost = 0;
	    else
		    cost  = 500;
    }

    else if (!str_prefix(arg,"armor"))
    {
	    spell = spell_armor;
	    sn    = skill_lookup("armor");
	    words = "knahk";
	    if(glad_healing == TRUE)
		    cost = 0;
	    else
		    cost  = 700;
    }

    else if (!str_prefix(arg,"glow") || !str_prefix(arg,"sanctuary"))
    {
	    spell = spell_sanctuary;
	    sn    = skill_lookup("sanctuary");
	    words = "havana cigar";
	    if(glad_healing == TRUE)
		    cost = 0;
	    else
		    cost  = 8000;
    }
    else if(glad_healing == TRUE)
    {
	    if (!str_prefix(arg,"haste"))
	    {
		    spell = spell_haste;
		    sn    = skill_lookup("haste");
		    words = "quick";
		    cost = 0;
	    }
	    else if (!str_prefix(arg,"giant") || !str_prefix(arg,"strength"))
	    {
		    spell = spell_giant_strength;
		    sn    = skill_lookup("giant strength");
		    words = "powerful";
		    cost = 0;
	    }
	    else if (!str_prefix(arg,"shield"))
      {
        spell = spell_shield;
        sn    = skill_lookup("shield");
        words = "gpuzre";
        cost = 0;
      }
      else if (!str_prefix(arg,"fury") || !str_prefix(arg,"frenzy"))
      {
        spell = spell_frenzy;
        sn    = skill_lookup("frenzy");
        words = "rage";
        cost = 0;
      }
      else if (!str_prefix(arg,"cancellation"))
      {
        spell = spell_cancellation;
        sn    = skill_lookup("cancellation");
        words = "buff-b-gon";
        cost = 0;
      }
      else
      {
    act("$N says 'Type 'heal' for a list of spells.'",
        ch,NULL,mob,TO_CHAR,FALSE);
    return;
      }
    }
    else if(IS_SET(ch->mhs, MHS_GLADIATOR) && gladiator_info.WNR == TRUE && 
      (!str_prefix(arg,"gladiate") || !str_prefix(arg,"gladiator")))
    {
      act("$n utters the words 'good luck'.",mob,NULL,words,TO_ROOM,FALSE);
      spell = spell_bless;
      sn    = skill_lookup("bless");
      if(sn != -1) spell(sn,mob->level,mob,ch,TARGET_CHAR);
      spell = spell_armor;
      sn    = skill_lookup("armor");
      if(sn != -1) spell(sn,mob->level,mob,ch,TARGET_CHAR);
      spell = spell_shield;
      sn    = skill_lookup("shield");
      if(sn != -1) spell(sn,mob->level,mob,ch,TARGET_CHAR);
      spell = spell_haste;
      sn    = skill_lookup("haste");
      if(sn != -1) spell(sn,mob->level,mob,ch,TARGET_CHAR);
      spell = spell_sanctuary;
      sn    = skill_lookup("sanctuary");
      if(sn != -1) spell(sn,mob->level,mob,ch,TARGET_CHAR);
      return;
    }
    else 
    {
	act("$N says 'Type 'heal' for a list of spells.'",
	    ch,NULL,mob,TO_CHAR,FALSE);
	return;
    }


    if (cost > (ch->gold * 100 + ch->silver))
    {
	act("$N says 'You do not have enough gold for my services.'",
	    ch,NULL,mob,TO_CHAR,FALSE);
	return;
    }

    WAIT_STATE(ch,PULSE_VIOLENCE);

    if(cost > 0)
    {
      deduct_cost(ch,cost);
      mob->gold += cost/100;
      mob->silver += cost % 100;
    }
    act("$n utters the words '$T'.",mob,NULL,words,TO_ROOM,FALSE);
     
    if (spell == NULL)  /* restore mana trap...kinda hackish */
    {
	ch->mana += dice(2,8) + mob->level / 3;
	ch->mana = UMIN(ch->mana,ch->max_mana);
	send_to_char("A warm glow passes through you.\n\r",ch);
	return;
     }

     if (sn == -1)
	return;
    
     spell(sn,mob->level,mob,ch,TARGET_CHAR);
}
