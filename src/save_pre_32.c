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

static char rcsid[] = "$Id: save.c,v 1.187 2003/08/16 16:45:07 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
/*#include <malloc.h>*/
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"
#include "lookup.h"
#include "tables.h"
 
#if !defined(macintosh)
extern  int     _filbuf         args( (FILE *) );
#endif


int rename(const char *oldfname, const char *newfname);

/*
 * Array of containers read for proper re-nesting of objects.
 */
#define MAX_NEST  100
static  OBJ_DATA *  rgObjNest [MAX_NEST + 1];



/*
 * Local functions.
 */
 void 	convert_bits	args( ( CHAR_DATA *ch ) );
void  fwrite_char args( ( CHAR_DATA *ch,  FILE *fp ) );
void  fwrite_obj  args( ( CHAR_DATA *ch,  OBJ_DATA  *obj,
          FILE *fp, int iNest ) );
void  fwrite_pet  args( ( CHAR_DATA *pet, FILE *fp) );
void  fread_char  args( ( CHAR_DATA *ch,  FILE *fp ) );
void    fread_pet args( ( CHAR_DATA *ch,  FILE *fp ) );
void  fread_obj args( ( CHAR_DATA *ch,  FILE *fp ) );


/*
 * Piddly converter by Ben
 * Rusty made me do it
 */
#define COMM_COMPACT		(L) 
#define COMM_BRIEF		(M)
#define COMM_PROMPT		(N)
#define COMM_SHOW_AFFECTS	(Q)
#define COMM_DISP_VNUM		(T)
#define PLR_LONGEQ		(O)
#define PLR_COLOR		(bb)
#define COMM_COMBINE		(O)
void convert_bits( CHAR_DATA *ch )
{
   if ( IS_SET(ch->comm,COMM_COMPACT) )
   {
	REMOVE_BIT(ch->comm,COMM_COMPACT);
	SET_BIT(ch->display,DISP_COMPACT);
   }

   if ( IS_SET(ch->comm,COMM_BRIEF) )
   {
	REMOVE_BIT(ch->comm,COMM_BRIEF);
	SET_BIT(ch->display,DISP_BRIEF_DESCR);
   }

   if ( IS_SET(ch->comm,COMM_PROMPT) )
   {
	REMOVE_BIT(ch->comm,COMM_PROMPT);
	SET_BIT(ch->display,DISP_PROMPT);
   }

   if ( IS_SET(ch->comm,COMM_SHOW_AFFECTS) )
   {
       REMOVE_BIT(ch->comm,COMM_SHOW_AFFECTS);
       SET_BIT(ch->display,DISP_SHOW_AFFECTS);
   }

   if ( IS_SET(ch->comm,COMM_DISP_VNUM) )
   {
       REMOVE_BIT(ch->comm,COMM_DISP_VNUM);
       SET_BIT(ch->display,DISP_DISP_VNUM);
   }

   if ( IS_SET(ch->act,PLR_LONGEQ) )
   {
       REMOVE_BIT(ch->act,PLR_LONGEQ);
       SET_BIT(ch->display,DISP_BRIEF_EQLIST);
   }

   if ( IS_SET(ch->act,PLR_COLOR) )
   {
       REMOVE_BIT(ch->act,PLR_COLOR);
       SET_BIT(ch->display,DISP_COLOR);
   }

   ch->version = 9;
   return;
}
#undef COMM_COMPACT
#undef COMM_BRIEF
#undef COMM_PROMPT
#undef COMM_SHOW_AFFECTS
#undef COMM_DISP_VNUM
#undef PLR_LONGEQ
#undef PLR_COLOR
#undef COMM_COMBINE


/*
 * Save a character and inventory.
 * Would be cool to save NPC's too for quest purposes,
 *   some of the infrastructure is provided.
 */
void save_char_obj( CHAR_DATA *ch )
{
    char strsave[MAX_INPUT_LENGTH],buf[MAX_STRING_LENGTH];
    FILE *fp;

    if ( IS_NPC(ch) )
  return;

#ifdef OLC_VERSION
return;
#endif

 /* Don't save if the character is invalidated. */
 if ( !IS_VALID(ch)) {
     bug("save_char_obj: Trying to save an invalidated character.\n", 0);
     return;
 }

    sprintf(buf,"SCO Saving: %s.",ch->name);
    log_string(buf);

    if ( (ch->desc != NULL) && (ch->desc->original != NULL) )
    ch = ch->desc->original;

#if defined(unix)
    /* create god log */
    if (IS_IMMORTAL(ch) || ch->level >= LEVEL_IMMORTAL)
    {
//  fclose(fpReserve);
  sprintf(strsave, "%s%s",GOD_DIR, capitalize(ch->name));
  if ((fp = fopen(strsave,"w")) == NULL)
  {
      bug("Save_char_obj: fopen",0);
      perror(strsave);
      return;
  }

  fprintf(fp,"Lev %2d Trust %2d  %s%s\n",
      ch->level, get_trust(ch), ch->name, ch->pcdata->title);
  fclose( fp );
//  fpReserve = fopen( NULL_FILE, "r" );
    }
#endif

//    fclose( fpReserve );
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( ch->name ) );
    if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
    {
  bug( "Save_char_obj: fopen", 0 );
  perror( strsave );
    }
    else
    {
  fwrite_char( ch, fp );
  if ( ch->carrying != NULL )
      fwrite_obj( ch, ch->carrying, fp, 0 );
  /* save the pets */
  if (ch->pet != NULL && ch->pet->in_room == ch->in_room)
      fwrite_pet(ch->pet,fp);
  fprintf( fp, "#END\n" );
    }
    fclose( fp );
    rename(TEMP_FILE,strsave);
/*
    sprintf(buf,"mv %s %s",PLAYER_TEMP,strsave);
    system(buf);
*/
//    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Write the char.
 */
void fwrite_char( CHAR_DATA *ch, FILE *fp )
{
    int i;
    AFFECT_DATA *paf;
    int sn, gn, pos;
    MACRO_DATA *macro;

    fprintf( fp, "#%s\n", IS_NPC(ch) ? "MOB" : "PLAYER" );

    fprintf( fp, "Name %s~\n",  ch->name    );
    if ( !IS_NPC(ch) && ch->pcdata->surname != NULL )
	fprintf( fp, "SurN %s~\n", ch->pcdata->surname );
    fprintf( fp, "Id   %ld\n", ch->id     );
    fprintf( fp, "Email %s~\n", ch->pcdata->email );
    fprintf( fp, "LogO %ld\n",  current_time    );
    fprintf( fp, "Vers %d\n",   31   );
    if (ch->short_descr[0] != '\0')
        fprintf( fp, "ShD  %s~\n",  ch->short_descr );
    if( ch->long_descr[0] != '\0')
  fprintf( fp, "LnD  %s~\n",  ch->long_descr  );
    if (ch->description[0] != '\0')
      fprintf( fp, "Desc %s~\n",  ch->description );
    if (ch->prompt != NULL || !str_cmp(ch->prompt,"%h %m %v%c> "))
        fprintf( fp, "Prom %s~\n",      ch->prompt    );
    fprintf( fp, "Race %s~\n", pc_race_table[ch->race].name );
    if (ch->clan)
      fprintf( fp, "Clan %s~\n",clan_table[ch->clan].name);
    if (ch->pcdata->save_clan)
      fprintf( fp, "SaveClan %s~\n",clan_table[ch->pcdata->save_clan].name);
    fprintf( fp, "Outc %d\n",   ch->pcdata->outcT );
    fprintf( fp, "RufT %d\n",   ch->pcdata->ruffT );
    fprintf( fp, "MatookT %d\n",   ch->pcdata->matookT );
    if(ch->pcdata->abolish_timer != 0 )
    fprintf( fp, "Abol %d\n", ch->pcdata->abolish_timer );
    if(ch->pcdata->mutant_timer != 0 )
    fprintf( fp, "Mutant %d\n", ch->pcdata->mutant_timer );
    fprintf( fp, "SkPTimer %d\n", ch->pcdata->skill_point_timer);
    fprintf( fp, "SkPTracker %d\n", ch->pcdata->skill_point_tracker);
    if (!IS_NPC(ch))
    fprintf( fp, "LogoutTracker %d\n", ch->pcdata->logout_tracker);
    if(ch->pcdata->hostmask[0] != '\0')
      fprintf( fp, "Hstm %s~\n", ch->pcdata->hostmask );
    fprintf( fp, "Kill %d %d %d %d\n",
                ch->pcdata->killer_data[PC_LOWER_KILLS],
                ch->pcdata->killer_data[PC_EQUAL_KILLS],
                ch->pcdata->killer_data[PC_GREATER_KILLS],
		ch->pcdata->killer_data[PC_DEATHS]); 
    fprintf( fp, "Steal %ld %ld %ld\n",
		ch->pcdata->steal_data[PC_STOLEN_ITEMS],
		ch->pcdata->steal_data[PC_STOLEN_GOLD],
		ch->pcdata->steal_data[PC_SLICES]);
    fprintf( fp, "LstK %s~\n", ch->pcdata->last_kill );
    fprintf( fp, "LstKBy %s~\n", ch->pcdata->last_killed_by );
    fprintf( fp, "LstAtkedBy %s~\n", ch->pcdata->last_attacked_by );
    fprintf( fp, "LstAtkedByTimer %d\n", ch->pcdata->last_attacked_by_timer );
    fprintf( fp, "LstCombatDate %d\n", ch->pcdata->last_combat_date );
    fprintf( fp, "LstKillDate %d\n", ch->pcdata->last_kill_date );
    fprintf( fp, "LstDeathDate %d\n", ch->pcdata->last_death_date );
    fprintf( fp, "LoginsWOKill %d\n", ch->pcdata->logins_without_kill );
    fprintf( fp, "LoginsWODeath %d\n", ch->pcdata->logins_without_death);
    fprintf( fp, "LoginsWOCombat %d\n", ch->pcdata->logins_without_combat );
    fprintf( fp, "Bnty %ld\n", ch->pcdata->bounty );
    fprintf( fp, "DeityFavor %d %d\n", ch->pcdata->deity_favor, ch->pcdata->deity_favor_timer);
    fprintf( fp, "PrefStat %d\n", ch->pcdata->pref_stat);
    for(i = 0; i < QUEST_COUNT; i++)
	fprintf( fp, "QuestWins %d %d\n", i, ch->pcdata->quest_wins[i]);

 /*
    if(IS_SET(ch->mhs,MHS_SAVANT))
       fprintf(fp,"Savant %d\n",ch->pcdata->savant );
 */
    if(IS_SET(ch->mhs,MHS_HIGHLANDER))
       fprintf(fp,"High %d %d\n",ch->pcdata->highlander_data[ALL_KILLS],
          ch->pcdata->highlander_data[REAL_KILLS]);
    if(IS_SET(ch->mhs,MHS_SHAPESHIFTED))
       fprintf(fp,"Shift %d %d %d %d %d %d %d %d %d %d~\n",
	  ch->save_race,
          ch->save_con_mod,
	  ch->save_stat[STAT_STR],
	  ch->save_stat[STAT_INT],
	  ch->save_stat[STAT_WIS],
	  ch->save_stat[STAT_DEX],
	  ch->save_stat[STAT_CON],
          ch->save_stat[STAT_AGT],
          ch->save_stat[STAT_END],
          ch->save_stat[STAT_SOC]);
    fprintf( fp, "Deity %d %s~\n",
	       ch->pcdata->switched,deity_table[ch->pcdata->deity].name);
    if (ch->pcdata->deity_timer != 0 )
	fprintf(fp, "DeityT %d %s~\n", ch->pcdata->deity_timer, deity_table[ch->pcdata->new_deity].name);
    fprintf( fp, "Sac  %d\n", ch->pcdata->sac     );
    fprintf( fp, "Sex  %d\n", ch->sex     );
    fprintf( fp, "Vump  %d\n", ch->trumps );
    fprintf( fp, "Node %d\n", ch->pcdata->node );
    fprintf( fp, "Cap %d\n", ch->pcdata->capped );
    fprintf( fp, "Created %d\n", ch->pcdata->created_date );
    fprintf( fp, "Glad %d %d %d %d %d %d\n", 
           ch->pcdata->gladiator_data[GLADIATOR_VICTORIES],
           ch->pcdata->gladiator_data[GLADIATOR_KILLS],
           ch->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES],
           ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS],
           ch->pcdata->gladiator_data[GLADIATOR_PLAYS],
           ch->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS]);

    /* Save the Last Host logged in from to the Pfiles - Poquah */
    /* If the Desc is NULL don't save since its probably Link Dead
       Anyways, this way we'll save the socket from previous as well
       this covers Ploaded chars */
    if (ch->desc != NULL)
    {
       ch->pcdata->last_host = str_dup(ch->desc->host);
       fprintf( fp, "LastHost %s~\n", ch->pcdata->last_host);
    }

    fprintf( fp, "Cla  %d\n", ch->class   );
    if ( ch->kit )
    fprintf( fp, "Kit %s~\n", kit_table[ch->kit].name );
    fprintf( fp, "ClaO %d\n", ch->pcdata->old_class );
    fprintf( fp, "Levl %d\n", ch->level   );
    fprintf( fp, "DLevl %d\n", ch->pcdata->debit_level );
    if (ch->trust != 0)
  fprintf( fp, "Tru  %d\n", ch->trust );
    fprintf( fp, "Plyd %d\n",
  ch->played + (int) (current_time - ch->logon) );
  /*
    fprintf( fp, "Rdid %d\n", ch->redid);
    */
    fprintf( fp, "Not  %ld %ld %ld %ld %ld %ld %ld %ld %ld %ld\n",    
  ch->pcdata->last_note,ch->pcdata->last_idea,ch->pcdata->last_penalty,
  ch->pcdata->last_news,ch->pcdata->last_changes,ch->pcdata->last_ooc,  
  ch->pcdata->last_bug, ch->pcdata->last_cnote , ch->pcdata->last_immnote, 
  ch->pcdata->last_qnote);
    fprintf( fp, "Scro %d\n",   ch->lines   );
    fprintf( fp, "Rank %d\n",   ch->pcdata->rank);
    fprintf( fp, "Room %d\n",
        (  ch->in_room == get_room_index( ROOM_VNUM_LIMBO )
        && ch->was_in_room != NULL )
            ? ch->was_in_room->vnum
            : ch->in_room == NULL ? 3001 : ch->in_room->vnum );

    fprintf( fp, "HMV  %d %d %d %d %d %d\n",
  ch->hit, ch->max_hit, ch->mana, ch->max_mana, ch->move, ch->max_move );
    if (ch->gold > 0)
      fprintf( fp, "Gold %ld\n",  ch->gold    );
    else
      fprintf( fp, "Gold %d\n", 0     ); 
    if (ch->silver > 0)
  fprintf( fp, "Silv %ld\n",ch->silver    );
    else
  fprintf( fp, "Silv %d\n",0      );
    if (ch->in_bank > 0)
  fprintf( fp, "Bank %d\n", ch->in_bank );
    fprintf( fp, "Exp  %d\n", ch->exp     );
    if (ch->act != 0)
  fprintf( fp, "Act  %s\n",   print_flags(ch->act));
    if (ch->mhs != 0)
  fprintf( fp, "MHSF %s\n",	print_flags(ch->mhs));
    if (ch->pcdata->clan_flags != 0)
  fprintf(fp, "ClnF %s\n",      print_flags(ch->pcdata->clan_flags));
    if (ch->affected_by != 0)
  fprintf( fp, "AfBy %s\n",   print_flags(ch->affected_by));
    fprintf( fp, "Comm %s\n",       print_flags(ch->comm));
    fprintf( fp, "Disp %s\n",	print_flags(ch->display));
    fprintf( fp, "IcgB %s\n", print_flags(ch->icg_bits));
    fprintf( fp, "Icg %d\n", ch->icg);
    /*
    fprintf( fp, "IMC %s\n",        print_flags(ch->pcdata->imc_deaf));
    fprintf( fp, "IMCAllow %s\n",   print_flags(ch->pcdata->imc_allow));
    fprintf( fp, "IMCDeny %s\n",    print_flags(ch->pcdata->imc_deny));
    fprintf( fp, "ICEListen %s~\n", ch->pcdata->ice_listen);
     */
    if (ch->wiznet)
      fprintf( fp, "Wizn %s\n",   print_flags(ch->wiznet));
    if (ch->pnet)
      fprintf( fp, "Pnet %s\n",   print_flags(ch->pnet));
    if(ch->pcdata->new_opt_flags)
      fprintf( fp, "NewOpts %s\n",   print_flags(ch->pcdata->new_opt_flags));
    if (ch->invis_level)
  fprintf( fp, "Invi %d\n",   ch->invis_level );
    if (ch->incog_level)
  fprintf(fp,"Inco %d\n",ch->incog_level);
    fprintf( fp, "Pos  %d\n", 
  ch->position == POS_FIGHTING ? POS_STANDING : ch->position );
    if (ch->practice != 0)
      fprintf( fp, "Prac %d\n", ch->practice  );
    if (ch->train != 0)
  fprintf( fp, "Trai %d\n", ch->train );
    if(ch->pcdata->half_train != 0)
  fprintf( fp, "HTrai %d\n", ch->pcdata->half_train );
    if(ch->pcdata->retrain != 0)
  fprintf( fp, "ReTrai %d\n", ch->pcdata->retrain );
    if(ch->pcdata->half_retrain != 0)
  fprintf( fp, "HReTrai %d\n", ch->pcdata->half_retrain );
    if(ch->pcdata->trained_hit != 0 || ch->pcdata->trained_mana != 0 || ch->pcdata->trained_move != 0)
  fprintf(fp, "StatsTrained %d %d %d\n", ch->pcdata->trained_hit, ch->pcdata->trained_mana, ch->pcdata->trained_move);
    if (ch->skill_points != 0 )
	fprintf( fp, "SkiP %d\n", ch->skill_points );
    if (ch->saving_throw != 0)
  fprintf( fp, "Save  %d\n",  ch->saving_throw);
    fprintf( fp, "Alig  %d\n",  ch->alignment   );
    if (ch->hitroll != 0)
  fprintf( fp, "Hit   %d\n",  ch->hitroll );
    if (ch->damroll != 0)
  fprintf( fp, "Dam   %d\n",  ch->damroll );
    if (ch->pcdata->second_hitroll != 0)
  fprintf( fp, "SecHit   %d\n",  ch->pcdata->second_hitroll );
    if (ch->pcdata->second_damroll != 0)
  fprintf( fp, "SecDam   %d\n",  ch->pcdata->second_damroll );
    fprintf( fp, "ACs %d %d %d %d\n", 
  ch->armor[0],ch->armor[1],ch->armor[2],ch->armor[3]);
    if (ch->wimpy !=0 )
  fprintf( fp, "Wimp  %d\n",  ch->wimpy );
    fprintf( fp, "Attr %d %d %d %d %d %d %d %d\n",
  ch->perm_stat[STAT_STR],
  ch->perm_stat[STAT_INT],
  ch->perm_stat[STAT_WIS],
  ch->perm_stat[STAT_DEX],
  ch->perm_stat[STAT_CON],
  ch->perm_stat[STAT_AGT],
  ch->perm_stat[STAT_END],
  ch->perm_stat[STAT_SOC] );

    fprintf (fp, "AMod %d %d %d %d %d %d %d %d\n",
  ch->mod_stat[STAT_STR],
  ch->mod_stat[STAT_INT],
  ch->mod_stat[STAT_WIS],
  ch->mod_stat[STAT_DEX],
  ch->mod_stat[STAT_CON],
  ch->mod_stat[STAT_AGT],
  ch->mod_stat[STAT_END],
  ch->mod_stat[STAT_SOC] );
    

    if ( IS_NPC(ch) )
    {
  fprintf( fp, "Vnum %d\n", ch->pIndexData->vnum  );
    }
    else
    {
  fprintf( fp, "Pass %s~\n",  ch->pcdata->pwd   );
  if (ch->pcdata->bamfin[0] != '\0')
      fprintf( fp, "Bin  %s~\n",  ch->pcdata->bamfin);
  if (ch->pcdata->bamfout[0] != '\0')
    fprintf( fp, "Bout %s~\n",  ch->pcdata->bamfout);
  fprintf( fp, "Titl %s~\n",  ch->pcdata->title );
      fprintf( fp, "Pnts %d\n",     ch->pcdata->points      );
  if (ch->pcdata->who_name && ch->pcdata->who_name[0])
  fprintf( fp, "WhoName %s~\n", ch->pcdata->who_name );
  fprintf( fp, "TSex %d\n", ch->pcdata->true_sex  );
  fprintf( fp, "LLev %d\n", ch->pcdata->last_level  );
  fprintf( fp, "HMVP %d %d %d\n", ch->pcdata->perm_hit, 
               ch->pcdata->perm_mana,
               ch->pcdata->perm_move);
  fprintf( fp, "Cnd  %d %d %d %d\n",
      ch->pcdata->condition[0],
      ch->pcdata->condition[1],
      ch->pcdata->condition[2],
      ch->pcdata->condition[3] );
      
  if (ch->pcdata->edit.range) {
    VNUM_RANGE_DATA *range;
    
    fprintf (fp,"Edit %s\n",
      print_flags(ch->pcdata->edit.per_flags));
    range = ch->pcdata->edit.range;
    while (range) {
      fprintf (fp,"ERange %d %d\n",range->min,range->max);
      range = range->next;
    }    
  }

  /* write alias */
  
  for (pos = 0; pos < MAX_ALIAS; pos++)
  {
      if (ch->pcdata->alias[pos] == NULL
      ||  ch->pcdata->alias_sub[pos] == NULL)
    break;

      fprintf(fp,"Alias %s %s~\n",ch->pcdata->alias[pos],
        ch->pcdata->alias_sub[pos]);
  }
  
  /* write macros */
  macro = ch->pcdata->macro;
  while (macro) {
    fprintf (fp,"Macro %s %s~\n",macro->name,macro->text);
    macro = macro->next;
  }    

  if ( ch->species_enemy )
	fprintf(fp,"SpecEn %d\n", ch->species_enemy );

  if ( ch->pcdata->specialize )
	fprintf(fp, "Spz '%s'\n", skill_table[ch->pcdata->specialize].name);

  for ( sn = 0; sn < MAX_SKILL; sn++ )
  {
      if ( skill_table[sn].name != NULL && ch->pcdata->learned[sn] > 0 )
      {
    fprintf( fp, "Sk %d '%s'\n",
        ch->pcdata->learned[sn], skill_table[sn].name );
      }

      if ( skill_table[sn].name != NULL && ch->pcdata->old_learned[sn] > 0 )
      {
    fprintf( fp, "OldSk %d '%s'\n",
        ch->pcdata->old_learned[sn], skill_table[sn].name );
      }
  }

  for ( gn = 0; gn < MAX_GROUP; gn++ )
        {
            if ( group_table[gn].name != NULL && ch->pcdata->group_known[gn])
            {
                fprintf( fp, "Gr '%s'\n",group_table[gn].name);
            }
        }
    }

    for ( paf = ch->affected; paf != NULL; paf = paf->next )
    {
  if ((paf->type < 0) || (paf->type >= MAX_SKILL) )
      continue;
  
  fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
      skill_table[paf->type].name,
      paf->where,
      paf->level,
      paf->duration,
      paf->modifier,
      paf->location,
      paf->bitvector
      );
    }

    fprintf( fp, "End\n\n" );
    return;
}

/* write a pet */
void fwrite_pet( CHAR_DATA *pet, FILE *fp)
{
    AFFECT_DATA *paf;
    
    fprintf(fp,"#PET\n");
    
    fprintf(fp,"Vnum %d\n",pet->pIndexData->vnum);
    
    fprintf(fp,"Name %s~\n", pet->name);
    fprintf(fp,"LogO %ld\n", current_time);
    if (pet->short_descr != pet->pIndexData->short_descr)
      fprintf(fp,"ShD  %s~\n", pet->short_descr);
    if (pet->long_descr != pet->pIndexData->long_descr)
      fprintf(fp,"LnD  %s~\n", pet->long_descr);
    if (pet->description != pet->pIndexData->description)
      fprintf(fp,"Desc %s~\n", pet->description);
    if (pet->race != pet->pIndexData->race)
      fprintf(fp,"Race %s~\n", race_table[pet->race].name);
    fprintf(fp,"Sex  %d\n", pet->sex);
    if (pet->level != pet->pIndexData->level)
      fprintf(fp,"Levl %d\n", pet->level);
    fprintf(fp, "HMV  %d %d %d %d %d %d\n",
      pet->hit, pet->max_hit, pet->mana, pet->max_mana, pet->move, pet->max_move);
    if (pet->gold > 0)
      fprintf(fp,"Gold %ld\n",pet->gold);
    if (pet->silver > 0)
  fprintf(fp,"Silv %ld\n",pet->silver);
    if (pet->exp > 0)
      fprintf(fp, "Exp  %d\n", pet->exp);
    if (pet->act != pet->pIndexData->act)
      fprintf(fp, "Act  %s\n", print_flags(pet->act));
    if (pet->affected_by != pet->pIndexData->affected_by)
      fprintf(fp, "AfBy %s\n", print_flags(pet->affected_by));
    if (pet->comm != 0)
      fprintf(fp, "Comm %s\n", print_flags(pet->comm));
    fprintf(fp,"Pos  %d\n", pet->position = POS_FIGHTING ? POS_STANDING : pet->position);
    if (pet->saving_throw != 0)
      fprintf(fp, "Save %d\n", pet->saving_throw);
    if (pet->alignment != pet->pIndexData->alignment)
      fprintf(fp, "Alig %d\n", pet->alignment);
    if (pet->hitroll != pet->pIndexData->hitroll)
      fprintf(fp, "Hit  %d\n", pet->hitroll);
    if (pet->damroll != pet->pIndexData->damage[DICE_BONUS])
      fprintf(fp, "Dam  %d\n", pet->damroll);
    fprintf(fp, "ACs  %d %d %d %d\n",
      pet->armor[0],pet->armor[1],pet->armor[2],pet->armor[3]);
    fprintf(fp, "Attr %d %d %d %d %d %d %d %d\n",
      pet->perm_stat[STAT_STR], pet->perm_stat[STAT_INT],
      pet->perm_stat[STAT_WIS], pet->perm_stat[STAT_DEX],
      pet->perm_stat[STAT_CON], pet->perm_stat[STAT_AGT],
      pet->perm_stat[STAT_END], pet->perm_stat[STAT_SOC]);
    fprintf(fp, "AMod %d %d %d %d %d %d %d %d\n",
      pet->mod_stat[STAT_STR], pet->mod_stat[STAT_INT],
      pet->mod_stat[STAT_WIS], pet->mod_stat[STAT_DEX],
      pet->mod_stat[STAT_CON], pet->mod_stat[STAT_AGT],
      pet->mod_stat[STAT_END], pet->mod_stat[STAT_SOC]);
    
    for ( paf = pet->affected; paf != NULL; paf = paf->next )
    {
      if (paf->type < 0 || paf->type >= MAX_SKILL)
          continue;
          
      fprintf(fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
          skill_table[paf->type].name,
          paf->where, paf->level, paf->duration, paf->modifier,paf->location,
          paf->bitvector);
    }
    
    fprintf(fp,"End\n");
    return;
}
    
/*
 * Write an object and its contents.
 */
void fwrite_obj( CHAR_DATA *ch, OBJ_DATA *obj, FILE *fp, int iNest )
{
    EXTRA_DESCR_DATA *ed;
    AFFECT_DATA *paf;
	int sent;

    /*
     * Slick recursion to write lists backwards,
     *   so loading them will load in forwards order.
     */
    if ( obj->next_content != NULL )
  fwrite_obj( ch, obj->next_content, fp, iNest );
  
  
   /*
    * Don't save under developed items
    */
    if (obj->pIndexData->area && obj->pIndexData->area->under_develop 
        && (ch == NULL || !IS_IMMORTAL (ch))) {
      if(ch != NULL)
      {
          char buf[MAX_STRING_LENGTH];

          sprintf (buf,"Item [%d] under construction saving %s.",
            obj->pIndexData->vnum, capitalize (ch->name));
          log_string ( buf );
      }
      return;
    }  

    /*
     * Castrate storage characters.
     * PFresh Characters within their login limit do not apply.
     */
    if(ch != NULL && !IS_NPC(ch) && ch->pcdata->logout_tracker == 0)
    {
       if ( ((ch->level < obj->level - 2 && obj->item_type != ITEM_CONTAINER)
	    || ( obj->item_type == ITEM_KEY && obj->wear_loc == -1 )
            || ( obj->item_type == ITEM_MAP && !obj->value[0]) ))
          return;

    }
    else if(ch == NULL)
    {/* Don't save keys on the ground/pits */
       if ( ( obj->item_type == ITEM_KEY )
            || ( obj->item_type == ITEM_MAP && !obj->value[0]) )
          return;
    }

    fprintf( fp, "#O\n" );
    fprintf( fp, "Vnum %d\n",   obj->pIndexData->vnum        );
    if (!obj->pIndexData->new_format)
  fprintf( fp, "Oldstyle\n");
    if (obj->enchanted)
  fprintf( fp,"Enchanted\n");
    fprintf( fp, "Nest %d\n", iNest          );
    fprintf( fp, "PrevOwner %s~\n", obj->prev_owner);

    /* these data are only used if they do not match the defaults */

    if ( obj->name != obj->pIndexData->name)
      fprintf( fp, "Name %s~\n",  obj->name        );
    if(obj->link_name && obj->carried_by && !str_cmp(obj->carried_by->name, obj->link_name))
      fprintf(fp, "Link %s\n", obj->link_name);
    if ( obj->short_descr != obj->pIndexData->short_descr)
        fprintf( fp, "ShD  %s~\n",  obj->short_descr       );
    if ( obj->description != obj->pIndexData->description)
        fprintf( fp, "Desc %s~\n",  obj->description       );
    if ( obj->extra_flags != obj->pIndexData->extra_flags)
        fprintf( fp, "ExtF %d\n", obj->extra_flags       );
    if ( obj->wear_flags != obj->pIndexData->wear_flags)
        fprintf( fp, "WeaF %d\n", obj->wear_flags        );
    if ( obj->item_type != obj->pIndexData->item_type)
        fprintf( fp, "Ityp %d\n", obj->item_type         );
    if ( obj->stolen_timer != 0 )
        fprintf( fp, "Stolen %d\n", obj->stolen_timer    );
    if ( obj->weight != obj->pIndexData->weight)
        fprintf( fp, "Wt   %d\n", obj->weight        );
    if ( obj->warps != 0 )
	fprintf( fp, "Wrp %d\n", obj->warps	);
    if ( obj->damaged != 0 )
		fprintf( fp, "Damaged %d\n", obj->damaged	);
    if ( obj->condition != obj->pIndexData->condition)
  fprintf( fp, "Cond %d\n", obj->condition         );

    /* variable data */

    fprintf( fp, "Wear %d\n",   obj->wear_loc                );
    if (obj->level != obj->pIndexData->level)
        fprintf( fp, "Lev  %d\n", obj->level         );
    if (obj->timer != 0)
        fprintf( fp, "Time %d\n", obj->timer       );
    if (obj->wear_timer != 0)
        fprintf( fp, "WearTime %d\n", obj->wear_timer       );
    fprintf( fp, "Cost %d\n", obj->cost        );
    if (obj->value[0] != obj->pIndexData->value[0]
    ||  obj->value[1] != obj->pIndexData->value[1]
    ||  obj->value[2] != obj->pIndexData->value[2]
    ||  obj->value[3] != obj->pIndexData->value[3]
    ||  obj->value[4] != obj->pIndexData->value[4]) 
      fprintf( fp, "Val  %d %d %d %d %d\n",
      obj->value[0], obj->value[1], obj->value[2], obj->value[3],
      obj->value[4]      );

    switch ( obj->item_type )
    {
    case ITEM_POTION:
    case ITEM_SCROLL:
  if ( obj->value[1] > 0 )
  {
      fprintf( fp, "Spell 1 '%s'\n", 
    skill_table[obj->value[1]].name );
  }

  if ( obj->value[2] > 0 )
  {
      fprintf( fp, "Spell 2 '%s'\n", 
    skill_table[obj->value[2]].name );
  }

  if ( obj->value[3] > 0 )
  {
      fprintf( fp, "Spell 3 '%s'\n", 
    skill_table[obj->value[3]].name );
  }

  break;

    case ITEM_PILL:
    case ITEM_STAFF:
    case ITEM_WAND:
  if ( obj->value[3] > 0 )
  {
      fprintf( fp, "Spell 3 '%s'\n", 
    skill_table[obj->value[3]].name );
  }

  break;
    }

	sent = 0;
    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
     if (sent > 20) break;
  if ( (paf->type < 0) || (paf->type >= MAX_SKILL) )
     { sent++;
      continue;
     }
        fprintf( fp, "Affc '%s' %3d %3d %3d %3d %3d %10ld\n",
            skill_table[paf->type].name,
            paf->where,
            paf->level,
            paf->duration,
            paf->modifier,
            paf->location,
            paf->bitvector
            );
    }

    for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
    {
  fprintf( fp, "ExDe %s~ %s~\n",
      ed->keyword, ed->description );
    }

    fprintf( fp, "End\n\n" );

    if ( obj->contains != NULL )
  fwrite_obj( ch, obj->contains, fp, iNest + 1 );

    return;
}



/*
 * Load a char and inventory into a new ch structure.
 */
bool load_char_obj( DESCRIPTOR_DATA *d, char *name )
{
    char strsave[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    FILE *fp;
    bool found;
    int stat;
    int sn;
    int i;

    ch = new_char();
    ch->pcdata = new_pcdata();

    d->character      = ch;
    ch->desc        = d;
    ch->name        = str_dup( name );
    ch->id        = get_pc_id();
    ch->race        = race_lookup("human");
    ch->ignoring  = NULL;
    ch->pcdata->rank	= 0;
    ch->join    = 0;
    ch->trumps  = 0;
    ch->pcdata->deity   = 0;
    ch->pcdata->outcT   = 0;
    ch->pcdata->ruffT   = 0;
    ch->pcdata->matookT = 0;
    ch->pcdata->skill_point_timer = 0;
    ch->pcdata->skill_point_tracker = 0;
    ch->pcdata->gladiator_attack_timer = 0;
    ch->pcdata->gladiator_team = 0;
    ch->pcdata->last_attacked_by_timer = 0;
    ch->pcdata->last_death_timer = 0; /* we dont save this timer, it resets on logout */
    ch->pcdata->last_death_date = current_time; 
    ch->pcdata->last_kill_date = current_time; 
    ch->pcdata->last_combat_date = current_time; 
    ch->pcdata->logins_without_kill = 0; 
    ch->pcdata->logins_without_death = 0; 
    ch->pcdata->logins_without_combat = 0; 
    ch->pcdata->afk_counter = 0;
    ch->pcdata->died_today = FALSE;
    ch->pcdata->killed_today = FALSE;
    ch->pcdata->deity_favor = 0;
    ch->pcdata->deity_favor_timer = 0;
    ch->pcdata->deity_trial = 0;
    ch->pcdata->deity_trial_timer = 0;
    ch->pcdata->half_train = 0;
    ch->pcdata->retrain = 0;
    ch->pcdata->half_retrain = 0;
    ch->pcdata->trained_hit = 0;
    ch->pcdata->trained_mana = 0;
    ch->pcdata->trained_move = 0;
    ch->pcdata->bypass_blind = FALSE;
    ch->pcdata->new_opt_flags = 0;
    ch->pcdata->edit_type = 0;
    ch->pcdata->edit_limit = 0;
    ch->pcdata->edit_len = 0;
    ch->pcdata->edit_count = 0;
    ch->pcdata->edit_str = NULL;
    ch->pcdata->rlock_time = 0;
    ch->pcdata->pulse_target = NULL;
    ch->pcdata->corpse_timer = 0;

    ch->pcdata->rev_obj = NULL;
    ch->pcdata->rev_type = 0;
    ch->pcdata->rev_clan = NULL;
    ch->pcdata->edit_flags = 0;
    ch->pcdata->edit_obj = NULL;
    ch->pcdata->clan_info = NULL;
    ch->pcdata->edits_made = FALSE;
    ch->pcdata->bank_gold = 0;
    ch->pcdata->pulse_timer = 0;
    ch->pcdata->pulse_type = 0;

    for(i = 0; i < QUEST_COUNT; i++)
	ch->pcdata->quest_wins[i] = 0;
    ch->pcdata->quest_count = 0;// Active quests

    /* Set up for Pfrsh Old Skills, new chars just have 0's
       whereas old chars will populate this with the pfresh 
       in comm.c and then they will be saved off and loaded
       each time */
    for ( sn = 0; sn < MAX_SKILL; sn++ )
    {
       ch->pcdata->old_learned[sn] = 0;
    }

    ch->pcdata->logout_tracker = 0;
    ch->pcdata->hostmask[0] = '\0';
    /*
    ch->redid   = 0;
    */
    ch->pcdata->killer_data[0] = 0;
    ch->pcdata->killer_data[1] = 0;
    ch->pcdata->killer_data[2] = 0;
    ch->pcdata->killer_data[3] = 0;
    ch->pcdata->steal_data[0] = 0;
    ch->pcdata->steal_data[1] = 0;
    ch->pcdata->steal_data[2] = 0;
    ch->pcdata->highlander_data[0] = 0;
    ch->pcdata->highlander_data[1] = 0;
    ch->pcdata->gladiator_data[0] = 0;
    ch->pcdata->gladiator_data[1] = 0;
    ch->pcdata->gladiator_data[2] = 0;
    ch->pcdata->gladiator_data[3] = 0;
    ch->pcdata->gladiator_data[4] = 0;
    ch->pcdata->gladiator_data[5] = 0;
    ch->pcdata->save_clan = 0;
    ch->pcdata->switched   = 0;
    ch->pcdata->sac     = 0;
    ch->icg     = 0;
    ch->icg_bits = 0;
    ch->act       = PLR_NOSUMMON | PLR_AUTOASSIST | PLR_AUTOEXIT |
      PLR_AUTOLOOT | PLR_AUTOSAC | PLR_AUTOGOLD   |PLR_AUTOSPLIT;
    ch->comm	= 0;
    ch->display		= DISP_COMBINE | DISP_PROMPT | DISP_SURNAME;
    ch->prompt        = str_dup("<%hhp %mm %vmv %Xxp> ");
    ch->pcdata->confirm_delete    = FALSE;
    ch->pcdata->confirm_loner     = FALSE;
    ch->pcdata->pwd     = str_dup( "" );
    ch->pcdata->bamfin      = str_dup( "" );
    ch->pcdata->bamfout     = str_dup( "" );
    ch->pcdata->last_kill = str_dup( "no one" );
    ch->pcdata->last_killed_by = str_dup( "no one" );
    ch->pcdata->title     = str_dup( "" );
    ch->pcdata->who_name  = str_dup( "" );
    for (stat =0; stat < MAX_STATS; stat++)
  ch->perm_stat[stat]   = 13;
    ch->pcdata->condition[COND_THIRST]  = 48; 
    ch->pcdata->condition[COND_FULL]  = 48;
    ch->pcdata->condition[COND_HUNGER]  = 48;
    ch->pcdata->second_hitroll = 0;
    ch->pcdata->second_damroll = 0;

    found = FALSE;
//    fclose( fpReserve );
    
    sprintf( strsave, "%s%s", PLAYER_DIR, capitalize( name ) );
    if ( ( fp = fopen( strsave, "r" ) ) != NULL )
    {
  int iNest;

  for ( iNest = 0; iNest < MAX_NEST; iNest++ )
      rgObjNest[iNest] = NULL;

  found = TRUE;
  for ( ; ; )
  {
      char letter;
      char *word;

      letter = fread_letter( fp );
      if ( letter == '*' )
      {
    fread_to_eol( fp );
    continue;
      }

      if ( letter != '#' )
      {
    bug( "Load_char_obj: # not found.", 0 );
    break;
      }

      word = fread_word( fp );
      if      ( !str_cmp( word, "PLAYER" ) ) fread_char ( ch, fp ); 
      else if ( !str_cmp( word, "OBJECT" ) ) fread_obj  ( ch, fp );
      else if ( !str_cmp( word, "O"      ) ) fread_obj  ( ch, fp );
      else if ( !str_cmp( word, "PET"    ) ) fread_pet  ( ch, fp );
      else if ( !str_cmp( word, "END"    ) ) break;
      else
      {
    bug( "Load_char_obj: bad section.", 0 );
    break;
      }
  }
  fclose( fp );
    }
//    fpReserve = fopen( NULL_FILE, "r" );


    /* initialize race */
    if (found)
    {
  int i;

  if (ch->race == 0)
      ch->race = race_lookup("human");

  ch->size = pc_race_table[ch->race].size;
  ch->dam_type = 17; /*punch */

  for (i = 0; i < 5; i++)
  {
      if (IS_SET(ch->mhs,MHS_SHAPESHIFTED) && ch->race != ch->save_race)
      {
         if (pc_race_table[ch->save_race].skills[i] == NULL)
            break;
         group_add(ch,pc_race_table[ch->save_race].skills[i],FALSE);
      }
      else
      {
         if (pc_race_table[ch->race].skills[i] == NULL)
	    break;
         group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
      }
  }
  ch->affected_by = ch->affected_by|race_table[ch->race].aff;
  ch->imm_flags = ch->imm_flags | race_table[ch->race].imm;
  ch->res_flags = ch->res_flags | race_table[ch->race].res;
  ch->vuln_flags  = ch->vuln_flags | race_table[ch->race].vuln;
  ch->form  = race_table[ch->race].form;
  ch->parts = race_table[ch->race].parts;
    }

   if ( HAS_KIT(ch,"nethermancer") )
      ch->imm_flags = ch->imm_flags | IMM_NEGATIVE;

    /* Convert bits */
    if (found && ch->version < 9 )
    	convert_bits(ch);

    if (found && ch->version < 11 )
	if ( class_table[ch->class].reclass )
		SET_BIT(ch->act,PLR_CANCLAN);

    if ( found && ch->version < 12 &&
	class_table[ch->class].reclass )
    {  /* Fix old class.  Set to warrior if possible, thief if not */
	if ( ch->class == class_lookup("samurai") ||
	     ch->class == class_lookup("berzerker") ||
	     ch->class == class_lookup("paladin") )
		ch->pcdata->old_class = class_lookup("warrior");
	else
	if ( ch->class == class_lookup("monk") ||
	     ch->class == class_lookup("assassin") )
		ch->pcdata->old_class = class_lookup("thief");
	else
		ch->pcdata->old_class = class_lookup("mage");
    }

    if ( found && !class_table[ch->class].reclass )
	ch->pcdata->old_class = ch->class;

    if ( found && ch->version < 13 && class_table[ch->class].reclass )
	SET_BIT(ch->mhs, MHS_OLD_RECLASS);


    /* RT initialize skills */

    if (found && ch->version < 2)  /* need to add the new skills */
    {
  group_add(ch,"rom basics",FALSE);
  group_add(ch,class_table[ch->class].base_group,FALSE);
  group_add(ch,class_table[ch->class].default_group,TRUE);
  ch->pcdata->learned[gsn_recall] = 50;
    }
 
/********** fix levels 
    if (found && ch->version < 3 && (ch->level > 35 || ch->trust > 35))
    {
  switch (ch->level)
  {
      case(40) : ch->level = 60;  break;
      case(39) : ch->level = 58;  break;
      case(38) : ch->level = 56;  break;
      case(37) : ch->level = 53;  break;
  }

        switch (ch->trust)
        {
            case(40) : ch->trust = 60;  break;
            case(39) : ch->trust = 58;  break;
            case(38) : ch->trust = 56;  break;
            case(37) : ch->trust = 53;  break;
            case(36) : ch->trust = 51;  break;
        }
    }

    if (found && ch->version < 4)
    {
  ch->gold   /= 100;
    }

**************/

  if(ch->version < 32)
  {
  }

    return found;
}



/*
 * Read in a char.
 */

#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )          \
        if ( !str_cmp( word, literal ) )  \
        {         \
            field  = value;     \
            fMatch = TRUE;      \
            break;        \
        }

void fread_char( CHAR_DATA *ch, FILE *fp )
{
    char buf[MAX_STRING_LENGTH];
    char *word;
    bool fMatch;  
    int lastlogoff = current_time;
    int percent, count = 0;

    sprintf(buf,"Loading %s.",ch->name);
    log_string(buf);

    for ( ; ; )
    {
  word   = feof( fp ) ? "End" : fread_word( fp );
  fMatch = FALSE;

  switch ( UPPER(word[0]) )
  {
  case '*':
      fMatch = TRUE;
      fread_to_eol( fp );
      break;

  case 'A':
      KEY( "Abol",	ch->pcdata->abolish_timer,	fread_number( fp ) );
      KEY( "Act",   ch->act,    fread_flag( fp ) );
      KEY( "AffectedBy",  ch->affected_by,  fread_flag( fp ) );
      KEY( "AfBy",  ch->affected_by,  fread_flag( fp ) );
      KEY( "Alignment", ch->alignment,    fread_number( fp ) );
      KEY( "Alig",  ch->alignment,    fread_number( fp ) );
      
      if (!str_cmp( word, "Alia"))
      {
    if (count >= MAX_ALIAS)
    {
        fread_to_eol(fp);
        fMatch = TRUE;
        break;
    }

    ch->pcdata->alias[count]  = str_dup(fread_word(fp));
    ch->pcdata->alias_sub[count]  = str_dup(fread_word(fp));
    count++;
    fMatch = TRUE;
    break;
      }

            if (!str_cmp( word, "Alias"))
            {
                if (count >= MAX_ALIAS)
                {
                    fread_to_eol(fp);
                    fMatch = TRUE;
                    break;
                }
 
                ch->pcdata->alias[count]        = str_dup(fread_word(fp));
                ch->pcdata->alias_sub[count]    = fread_string(fp);
                count++;
                fMatch = TRUE;
                break;
            }
      

      if (!str_cmp( word, "AC") || !str_cmp(word,"Armor"))
      {
    fread_to_eol(fp);
    fMatch = TRUE;
    break;
      }

      if (!str_cmp(word,"ACs"))
      {
    int i;

    for (i = 0; i < 4; i++)
        ch->armor[i] = fread_number(fp);
    fMatch = TRUE;
    break;
      }

      if (!str_cmp(word, "AffD"))
      {
    AFFECT_DATA *paf;
    int sn;

    paf = new_affect();

    sn = skill_lookup(fread_word(fp));
    if (sn < 0)
        bug("Fread_char: unknown skill.",0);
    else
        paf->type = sn;

    paf->level  = fread_number( fp );
    paf->duration = fread_number( fp );
    paf->modifier = fread_number( fp );
    paf->location = fread_number( fp );
    paf->bitvector  = fread_number( fp );
    paf->next = ch->affected;
    ch->affected  = paf;
    fMatch = TRUE;
    break;
      }

            if (!str_cmp(word, "Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.",0);
                else
                    paf->type = sn;
 
                paf->where  = fread_number(fp);
                paf->level      = fread_number( fp );
                paf->duration   = fread_number( fp );
                paf->modifier   = fread_number( fp );
                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );
                paf->next       = ch->affected;
                ch->affected    = paf;
                fMatch = TRUE;
                break;
            }

      if ( !str_cmp( word, "AttrMod"  ) || !str_cmp(word,"AMod"))
      {
    int stat;
    if(ch->version < 31)
	{
    for (stat = 0; stat < MAX_STATS - 3; stat ++)
       ch->mod_stat[stat] = fread_number(fp);
	ch->train += 10;
    fMatch = TRUE;
    break;
	}
    else
	{
    for (stat = 0; stat < MAX_STATS; stat ++)
       ch->mod_stat[stat] = fread_number(fp);
    fMatch = TRUE;
    break;
    	}
      }

      if ( !str_cmp( word, "AttrPerm" ) || !str_cmp(word,"Attr"))
      {
    int stat;

    if(ch->version < 31)
	{
    for (stat = 0; stat < MAX_STATS - 3; stat ++)
        ch->perm_stat[stat] = fread_number(fp);
    fMatch = TRUE;
    break;
    	}
    else
	{
    for (stat = 0; stat < MAX_STATS; stat++)
        ch->perm_stat[stat] = fread_number(fp);
    fMatch = TRUE;
    break;
    	}
      }
      break;

  case 'B':
      KEY( "Bnty",	ch->pcdata->bounty, fread_number( fp ) );
      KEY( "Bamfin",  ch->pcdata->bamfin, fread_string( fp ) );
      KEY( "Bamfout", ch->pcdata->bamfout,  fread_string( fp ) );
      KEY( "Bank",	ch->in_bank,	fread_number( fp ) );
      KEY( "Bin",   ch->pcdata->bamfin, fread_string( fp ) );
      KEY( "Bout",  ch->pcdata->bamfout,  fread_string( fp ) );
      break;

  case 'C':
      KEY( "Cap", ch->pcdata->capped, fread_number( fp ) );
      KEY( "Class", ch->class,    fread_number( fp ) );
      KEY( "Cla",   ch->class,    fread_number( fp ) );
      KEY( "ClaO",  ch->pcdata->old_class, fread_number( fp ) );
      KEY( "ClnF",  ch->pcdata->clan_flags, fread_flag ( fp) ); 
      if ( !str_cmp( word, "Clan" ) )
      {
        char *tmp = fread_string(fp);
        ch->clan = nonclan_lookup(tmp);
        free_string(tmp);

	 if (  ch->clan  == 0 )
	    ch->clan = nonclan_lookup("loner");

/*	 if (ch->clan == clan_lookup("avarice"))
            ch->pcdata->learned[skill_lookup("cure vision")] = 0;
         
	 if (ch->clan == clan_lookup("demise") 
         &&  ch->pcdata->learned[skill_lookup("confusion")] > 0 )
	 {
	   ch->pcdata->learned[skill_lookup("confusion")] = 0;
         }
         if (ch->clan == clan_lookup("demise")
         && ch->pcdata->learned[skill_lookup("aura of cthon")] > 0)
         {
           ch->pcdata->learned[skill_lookup("aura of cthon")] = 0;
         }

	 if (ch->clan == clan_lookup("zealot"))
	     ch->pcdata->learned[skill_lookup("annointment")] = 0;

         if (ch->clan == clan_lookup("honor"))
             ch->pcdata->learned[skill_lookup("honor guard")] = 0;

         if (ch->clan == clan_lookup("posse")
	 && ch->pcdata->learned[skill_lookup("cuffs of justice")] > 0)
             ch->pcdata->learned[skill_lookup("cuffs of justice")] = 0;
*/
	 fMatch = TRUE;
	 break;
      }

      if ( !str_cmp( word, "Condition" ) || !str_cmp(word,"Cond"))
      {
    ch->pcdata->condition[0] = fread_number( fp );
    ch->pcdata->condition[1] = fread_number( fp );
    ch->pcdata->condition[2] = fread_number( fp );
    fMatch = TRUE;
    break;
      }
            if (!str_cmp(word,"Cnd"))
            {
                ch->pcdata->condition[0] = fread_number( fp );
                ch->pcdata->condition[1] = fread_number( fp );
                ch->pcdata->condition[2] = fread_number( fp );
    ch->pcdata->condition[3] = fread_number( fp );
                fMatch = TRUE;
                break;
            }
      KEY("Comm",   ch->comm,   fread_flag( fp ) ); 
      KEY("Created",   ch->pcdata->created_date,   fread_number( fp ) ); 
          
      break;

  case 'D':
      KEY( "DLevl", ch->pcdata->debit_level, fread_number( fp ) );
      KEY( "Damroll", ch->damroll,    fread_number( fp ) );
      KEY( "Dam",   ch->damroll,    fread_number( fp ) );
      KEY( "Description", ch->description,  fread_string( fp ) );
      KEY( "Desc",  ch->description,  fread_string( fp ) );
      KEY( "Disp", ch->display, fread_flag( fp ) );
      if (!str_cmp(word,"Deity"))
      {
      if(ch->version < 7)
	{
	 ch->pcdata->deity		= deity_lookup(fread_string( fp ));
         ch->pcdata->new_deity = ch->pcdata->deity;
	}
	else
	{
	 ch->pcdata->switched     = fread_number(fp);
	 ch->pcdata->deity  = deity_lookup(fread_string( fp ));
         ch->pcdata->new_deity = ch->pcdata->deity;
	}
	fMatch = TRUE;
	break;
      }
      if(!str_cmp(word, "DeityT"))
      {
	ch->pcdata->deity_timer 	= fread_number(fp);
	ch->pcdata->new_deity		= deity_lookup(fread_string(fp));
        fMatch = TRUE;
      }
      if (!str_cmp(word, "DeityFavor"))
      {
   	ch->pcdata->deity_favor = fread_number(fp);
   	ch->pcdata->deity_favor_timer = fread_number(fp);
	fMatch = TRUE;
      }
      break;

  case 'E':
      if (!strcmp (word,"Edit")) {
        ch->pcdata->edit.per_flags = fread_flag (fp);
        REMOVE_BIT (ch->pcdata->edit.per_flags,EDIT_AUTO_CREATE);
        fMatch = TRUE;
      }      
      if (!str_cmp (word,"ERange")) {
        VNUM_RANGE_DATA *range;
        
        range = new_range ();
        range->min = fread_number (fp);
        range->max = fread_number (fp);
        range->next = ch->pcdata->edit.range;
        ch->pcdata->edit.range = range;        
        fMatch = TRUE;                
      }
      
  
      if ( !str_cmp( word, "End" ) )
      {
        /* adjust hp mana move up  -- here for speed's sake */
        percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);

    percent = UMIN(percent,100);
 
        if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
        &&  !IS_AFFECTED(ch,AFF_PLAGUE))
        {
              ch->hit += (ch->max_hit - ch->hit) * percent / 100;
              ch->mana    += (ch->max_mana - ch->mana) * percent / 100;
              ch->move    += (ch->max_move - ch->move)* percent / 100;
        }
    return;
      }
      KEY( "Email", ch->pcdata->email, fread_string( fp ) );
      KEY( "Exp",   ch->exp,    fread_number( fp ) );
      break;

  case 'G':
      if ( !str_cmp( word, "Glad" ))
      {
         if (ch->version > 28)
	 {
	    ch->pcdata->gladiator_data[GLADIATOR_VICTORIES] = fread_number( fp );
	    ch->pcdata->gladiator_data[GLADIATOR_KILLS] = fread_number( fp );
	    ch->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES] = fread_number( fp );
	    ch->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS] = fread_number( fp );
	    ch->pcdata->gladiator_data[GLADIATOR_PLAYS] = fread_number( fp );
	    ch->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS] = fread_number( fp );
	 }
         fMatch = TRUE;
	 break;
      }
      KEY( "Gold",  ch->gold,   fread_number( fp ) );
            if ( !str_cmp( word, "Group" )  || !str_cmp(word,"Gr"))
            {
                int gn;
                char *temp;
 
                temp = fread_word( fp ) ;
                gn = group_lookup(temp);
                /* gn    = group_lookup( fread_word( fp ) ); */
                if ( gn < 0 )
                {
                    fprintf(stderr,"%s",temp);
                    bug( "Fread_char: unknown group. ", 0 );
                }
                else
        gn_add(ch,gn);
                fMatch = TRUE;
            }

      break;

  case 'H':
      KEY( "Hitroll", ch->hitroll,    fread_number( fp ) );
      KEY( "Hit",   ch->hitroll,    fread_number( fp ) );
      KEY( "HTrai", ch->pcdata->half_train,    fread_number(fp));
      KEY( "HReTrai", ch->pcdata->half_retrain,  fread_number(fp));

      if( !str_cmp( word, "Hstm") )
      {
	strcpy(ch->pcdata->hostmask,fread_string(fp));
	fMatch = TRUE;
	break;
      }

      if ( !str_cmp( word, "High" ))
      {
	 ch->pcdata->highlander_data[ALL_KILLS] = fread_number( fp );
	 ch->pcdata->highlander_data[REAL_KILLS] = fread_number( fp );
         fMatch = TRUE;
	 break;
      }

      if ( !str_cmp( word, "HpManaMove" ) || !str_cmp(word,"HMV"))
      {
    ch->hit   = fread_number( fp );
    ch->max_hit = fread_number( fp );
    ch->mana  = fread_number( fp );
    ch->max_mana  = fread_number( fp );
    ch->move  = fread_number( fp );
    ch->max_move  = fread_number( fp );
    /*
        if(ch->version < 31)
	{
ch->hit = pc_race_table[ch->race].starting_hmv[0];
ch->max_hit = pc_race_table[ch->race].starting_hmv[0];
ch->mana = pc_race_table[ch->race].starting_hmv[1];
ch->max_mana = pc_race_table[ch->race].starting_hmv[1];
ch->move = pc_race_table[ch->race].starting_hmv[2];
ch->max_move = pc_race_table[ch->race].starting_hmv[2];
	}
      */

    fMatch = TRUE;
    break;
      }

        if ( !str_cmp( word, "HpManaMovePerm" ) || !str_cmp(word,"HMVP"))
        {
            ch->pcdata->perm_hit  = fread_number( fp );
            ch->pcdata->perm_mana   = fread_number( fp );
            ch->pcdata->perm_move   = fread_number( fp );
	    /*
        if(ch->version < 31)
	{
ch->pcdata->perm_hit = pc_race_table[ch->race].starting_hmv[0];
ch->pcdata->perm_mana = pc_race_table[ch->race].starting_hmv[1];
ch->pcdata->perm_move = pc_race_table[ch->race].starting_hmv[2];
	}
	*/
                fMatch = TRUE;
                break;
            }
      
      break;

  case 'I':
      KEY( "Id",    ch->id,     fread_number( fp ) );
      KEY( "Icg", ch->icg, fread_number( fp ) );
      KEY( "IcgB", ch->icg_bits, fread_flag( fp ) );
      KEY( "InvisLevel",  ch->invis_level,  fread_number( fp ) );
      KEY( "Inco",  ch->incog_level,  fread_number( fp ) );
      KEY( "Invi",  ch->invis_level,  fread_number( fp ) );
      if (!str_prefix( word, "IMC") || !str_prefix( word, "ICEL") ) 
	 {
	 fread_flag( fp ); /* advance the file pointer */
	 break;
	 }
      /* No structures exist anymore to assign these values to.
      KEY( "IMC",         ch->pcdata->imc_deaf,     fread_flag( fp ) );
      KEY( "IMCAllow",    ch->pcdata->imc_allow,    fread_flag( fp ) );
      KEY( "IMCDeny",     ch->pcdata->imc_deny,     fread_flag( fp ) );
      KEY( "ICEListen",   ch->pcdata->ice_listen, fread_string( fp ) );
       */
      break;

  case 'K':
      KEY( "Kit", ch->kit, kit_lookup(fread_string( fp ) ) );

      if (!str_cmp( word, "Kill"))
	{
          if (ch->version > 28)
          { 
	     ch->pcdata->killer_data[PC_LOWER_KILLS] = fread_number( fp );
	     ch->pcdata->killer_data[PC_EQUAL_KILLS] = fread_number( fp );
	     ch->pcdata->killer_data[PC_GREATER_KILLS] = fread_number( fp );
	     ch->pcdata->killer_data[PC_DEATHS] = fread_number( fp );
	     fMatch = TRUE;
          }
	}
      break;

  case 'L':
      KEY( "LastLevel", ch->pcdata->last_level, fread_number( fp ) );
      KEY( "LastHost", ch->pcdata->last_host, fread_string( fp ) );
      KEY( "LLev",  ch->pcdata->last_level, fread_number( fp ) );

/*
      KEY( "Level", ch->level,    fread_number( fp ) );
      KEY( "Lev",   ch->level,    fread_number( fp ) );
      KEY( "Levl",  ch->level,    fread_number( fp ) );
      */

      if (!str_cmp( word, "Levl"))
      {
	 ch->level = fread_number(fp);
         if(ch->level < 52 && ch->version < 20)
	    SET_BIT(ch->mhs,MHS_PREFRESHED);
         fMatch = TRUE;
      }

      KEY( "LogO",  lastlogoff,   fread_number( fp ) );
      KEY( "LogoutTracker", ch->pcdata->logout_tracker, fread_number( fp ) );
      KEY( "LongDescr", ch->long_descr,   fread_string( fp ) );
      KEY( "LstAtkedBy", ch->pcdata->last_attacked_by,   fread_string( fp ) );
      KEY( "LstAtkedByTimer", ch->pcdata->last_attacked_by_timer,   fread_number( fp ) );
      KEY( "LstK", ch->pcdata->last_kill,   fread_string( fp ) );
      KEY( "LstKBy", ch->pcdata->last_killed_by,   fread_string( fp ) );
      KEY( "LstCombatDate", ch->pcdata->last_combat_date, fread_number( fp ) );
      KEY( "LstDeathDate", ch->pcdata->last_death_date, fread_number( fp ) );
      KEY( "LstKillDate", ch->pcdata->last_kill_date, fread_number( fp ) );
      KEY( "LoginsWOKill", ch->pcdata->logins_without_kill,fread_number( fp ) );
      KEY( "LoginsWODeath", ch->pcdata->logins_without_death,fread_number( fp ) );
      KEY( "LoginsWOCombat", ch->pcdata->logins_without_combat,fread_number( fp ) );
      KEY( "LnD",   ch->long_descr,   fread_string( fp ) );
      break;
  case 'M':
      KEY( "Mutant", ch->pcdata->mutant_timer, fread_number( fp ) );
      KEY( "MatookT",	ch->pcdata->matookT, fread_number(fp) );
      /*
      KEY( "MHSF", ch->mhs, fread_flag( fp ) );
      */
      if (!str_cmp( word, "MHSF"))
      {
	 ch->mhs = fread_flag(fp);
	 if(ch->level < 52 && ch->version < 20)
	    SET_BIT(ch->mhs,MHS_PREFRESHED);
         if(ch->version < 28)
            REMOVE_BIT(ch->mhs,MHS_POSSE_ENEMY);
	 fMatch = TRUE;
      }

      if (!str_cmp( word, "Macro"))
      {
      
        MACRO_DATA *macro;
        
        macro = new_macro ();
        macro->name = str_dup (fread_word (fp));
        macro->text = fread_string (fp);
        macro->mark = FALSE;
        macro->next = ch->pcdata->macro;
        ch->pcdata->macro = macro;
        fMatch = TRUE;
        break;
      }         

  case 'N':
      KEY( "Name",  ch->name,   fread_string( fp ) );
      KEY("NewOpts",   ch->pcdata->new_opt_flags,   fread_flag( fp ) );
      KEY( "Node",  ch->pcdata->node,	fread_number( fp ) );
      KEY( "Note",  ch->pcdata->last_note,  fread_number( fp ) );
      if (!str_cmp(word,"Not"))
      {
    ch->pcdata->last_note     = fread_number(fp);
    ch->pcdata->last_idea     = fread_number(fp);
    ch->pcdata->last_penalty    = fread_number(fp);
    ch->pcdata->last_news     = fread_number(fp);
    ch->pcdata->last_changes    = fread_number(fp);
    if(ch->version > 7)
    ch->pcdata->last_ooc    = fread_number(fp);
    else ch->pcdata->last_ooc = 0;
    if(ch->version > 13)
    ch->pcdata->last_bug	= fread_number(fp);
    else ch->pcdata->last_bug = 0;
    if(ch->version > 14)
    ch->pcdata->last_cnote	= fread_number(fp);
    else ch->pcdata->last_cnote = 0;
    if(ch->version > 16)
    ch->pcdata->last_immnote	= fread_number(fp);
    else ch->pcdata->last_immnote = 0;
    if(ch->version > 29)
    ch->pcdata->last_qnote	= fread_number(fp);
    else ch->pcdata->last_qnote = 0;

    fMatch = TRUE;
    break;
      }
      break;

  case 'O':
      if ( !str_cmp(word,"OldSk"))
      {
    int sn;
    int value;
    char *temp;

    value = fread_number( fp );
    temp = fread_word( fp ) ;
    sn = skill_lookup(temp);
    if ( sn < 0 )
    {
        fprintf(stderr,"%s",temp);
        bug( "Fread_char: unknown old skill. ", 0 );
    }
    else
    {
        ch->pcdata->old_learned[sn] = value;
    }
    fMatch = TRUE;
      }

      KEY( "Outc",	ch->pcdata->outcT, fread_number(fp) );
      break;

  case 'P':
      KEY( "Password",  ch->pcdata->pwd,  fread_string (fp) );
      KEY( "Pass",  ch->pcdata->pwd,  fread_string( fp ) );
      KEY( "Played",  ch->played,   fread_number( fp ) );
      KEY( "Plyd",  ch->played,   fread_number( fp ) );
      KEY( "Points",  ch->pcdata->points, fread_number( fp ) );
      KEY( "Pnts",  ch->pcdata->points, fread_number( fp ) );
      KEY( "Pnet",  ch->pnet, fread_flag( fp ) );
      KEY( "Position",  ch->position,   fread_number( fp ) );
      KEY( "Pos",   ch->position,   fread_number( fp ) );
      KEY( "Practice",  ch->practice,   fread_number( fp ) );
      KEY( "Prac",  ch->practice,   fread_number( fp ) );
      KEY( "Prompt",      ch->prompt,             fread_string( fp ) );
      KEY( "Prom",  ch->prompt,   fread_string( fp ) );
      if(!str_cmp(word, "PrefStat")){
	ch->pcdata->pref_stat = fread_number(fp);
	fMatch = TRUE;
      }
      break;

	case 'Q':
	if ( !str_cmp( word, "QuestWins" ) )
	{
		int val = fread_number(fp);
		if(val >= 0 && val < QUEST_COUNT)
			ch->pcdata->quest_wins[val] = fread_number(fp);
		else
		{
			sprintf(buf, "Bad quest number saved: %d", val); 
			bug(buf, 0);
		}
	        fMatch = TRUE;
		break;				
	}
	break;		

  case 'R':
     if ( !str_cmp( word, "Race" ) ) {
          char *tmp = fread_string(fp);
          ch->race = race_lookup(tmp);
          free_string(tmp);
          fMatch = TRUE;
          break;
     }
	/*
      KEY( "Rdid",  ch->redid,   fread_number( fp ) );
      */
      KEY( "Rank",	ch->pcdata->rank, fread_number( fp) );
      KEY( "ReTrai", ch->pcdata->retrain,    fread_number(fp));
      if ( !str_cmp( word, "Room" ) )
      {
    ch->in_room = get_room_index( fread_number( fp ) );
    if ( ch->in_room == NULL )
        ch->in_room = get_room_index( ROOM_VNUM_LIMBO );
    fMatch = TRUE;
    break;
      }
      KEY( "RufT",      ch->pcdata->ruffT, fread_number(fp) );

      break;

  case 'S':
      KEY( "Sac",   ch->pcdata->sac,    fread_number( fp ) );
      KEY( "Savant", ch->pcdata->savant, fread_number( fp ) );
      KEY( "SavingThrow", ch->saving_throw, fread_number( fp ) );
      KEY( "Save",  ch->saving_throw, fread_number( fp ) );
      if ( !str_cmp( word, "SaveClan" ) )
      {
	 ch->pcdata->save_clan = nonclan_lookup(fread_string(fp));  
	 fMatch = TRUE;
	 break;
      }

      KEY( "Scro",  ch->lines,    fread_number( fp ) );
      KEY( "SecDam",ch->pcdata->second_damroll,    fread_number( fp ) );
      KEY( "SecHit",ch->pcdata->second_hitroll,    fread_number( fp ) );
      KEY( "Sex",   ch->sex,    fread_number( fp ) );

      if ( !str_cmp( word, "Shift" ))
      {
         ch->save_race = fread_number( fp );
         ch->save_con_mod = fread_number( fp );
	 ch->save_stat[STAT_STR] = fread_number( fp );
	 ch->save_stat[STAT_INT] = fread_number( fp );
	 ch->save_stat[STAT_WIS] = fread_number( fp );
	 ch->save_stat[STAT_DEX] = fread_number( fp );
	 ch->save_stat[STAT_CON] = fread_number( fp );
         ch->save_stat[STAT_AGT] = fread_number( fp );
         ch->save_stat[STAT_END] = fread_number( fp );
         ch->save_stat[STAT_SOC] = fread_number( fp );
	 ch->mod_stat[STAT_CON] -= ch->save_con_mod;

         fMatch = TRUE;
         break;
      }
      if ( !str_cmp( word, "StatsTrained" ) )
      {
         ch->pcdata->trained_hit = fread_number( fp );
         ch->pcdata->trained_mana = fread_number( fp );
         ch->pcdata->trained_move = fread_number( fp );
         fMatch = TRUE;
         break;
      }

      KEY( "ShortDescr",  ch->short_descr,  fread_string( fp ) );
      KEY( "ShD",   ch->short_descr,  fread_string( fp ) );
      KEY( "Silv",        ch->silver,             fread_number( fp ) );
      KEY( "SkPTimer", ch->pcdata->skill_point_timer, fread_number( fp ) );
      KEY( "SkPTracker", ch->pcdata->skill_point_tracker, fread_number( fp ) );
      KEY( "Spz", ch->pcdata->specialize, skill_lookup(fread_word( fp ) ) );
      KEY( "SpecEn", ch->species_enemy, fread_number( fp ) );
      KEY( "SurN", ch->pcdata->surname, fread_string( fp ) );
      if (!str_cmp( word, "Steal"))
	{
          if (ch->version > 28)
          {
	     ch->pcdata->steal_data[PC_STOLEN_ITEMS] = fread_number( fp );
	     ch->pcdata->steal_data[PC_STOLEN_GOLD] = fread_number( fp );
	     ch->pcdata->steal_data[PC_SLICES] = fread_number( fp );
	     fMatch = TRUE;
          }
	}

      if ( !str_cmp( word, "SkiP" ))
      {
	 if (ch->version < 19)
	    ch->skill_points = 0;
	 else
	    ch->skill_points = fread_number( fp );
	 fMatch = TRUE;
      }

      if ( !str_cmp( word, "Skill" ) || !str_cmp(word,"Sk"))
      {
    int sn;
    int value;
    char *temp;

    value = fread_number( fp );
    temp = fread_word( fp ) ;
    sn = skill_lookup(temp);
    /* sn    = skill_lookup( fread_word( fp ) ); */
    if ( sn < 0 )
    {
        fprintf(stderr,"%s",temp);
        bug( "Fread_char: unknown skill. ", 0 );
    }
    else
    {
        ch->pcdata->learned[sn] = value;
	ch->pcdata->last_learned[sn] = value;
    }
    fMatch = TRUE;
      }

      break;

  case 'T':
            KEY( "TrueSex",     ch->pcdata->true_sex,   fread_number( fp ) );
      KEY( "TSex",  ch->pcdata->true_sex,   fread_number( fp ) );
      KEY( "Trai",  ch->train,   fread_number( fp ) );
      KEY( "Trust", ch->trust,    fread_number( fp ) );
      KEY( "Tru",   ch->trust,    fread_number( fp ) );

      if ( !str_cmp( word, "Title" )  || !str_cmp( word, "Titl"))
      {
    ch->pcdata->title = fread_string( fp );
        if (ch->pcdata->title[0] != '.' && ch->pcdata->title[0] != ',' 
    &&  ch->pcdata->title[0] != '!' && ch->pcdata->title[0] != '?')
    {
        sprintf( buf, " %s", ch->pcdata->title );
        free_string( ch->pcdata->title );
        ch->pcdata->title = str_dup( buf );
    }
    fMatch = TRUE;
    break;
      }

      break;

  case 'V':
      KEY( "Version",     ch->version,    fread_number ( fp ) );
      KEY( "Vump",     ch->trumps,    fread_number ( fp ) );
      KEY( "Vers",  ch->version,    fread_number ( fp ) );
      if ( !str_cmp( word, "Vnum" ) )
      {
    ch->pIndexData = get_mob_index( fread_number( fp ) );
    fMatch = TRUE;
    break;
      }
      break;

  case 'W':
      KEY( "Wimpy", ch->wimpy,    fread_number( fp ) );
      KEY( "Wimp", ch->wimpy,    fread_number( fp ) );
      KEY( "Wizn",  ch->wiznet,   fread_flag( fp ) );
      KEY( "WhoName", ch->pcdata->who_name, fread_string( fp ) );
      break;
  }

  if ( !fMatch )
  {
      sprintf(log_buf,"Fread_char: no match for %s.", word );
      bug( log_buf, 0 );
      fread_to_eol( fp );
  }
    }
  if(ch->version < 31)
  {
    ch->train += 10;
    if(ch->wimpy > 50) ch->wimpy = 50;
  }
}

/* load a pet from the forgotten reaches */
void fread_pet( CHAR_DATA *ch, FILE *fp )
{
    char *word;
    CHAR_DATA *pet;
    bool fMatch;
    int lastlogoff = current_time;
    int percent;

    /* first entry had BETTER be the vnum or we barf */
    word = feof(fp) ? "END" : fread_word(fp);
    if (!str_cmp(word,"Vnum"))
    {
      int vnum;
      
      vnum = fread_number(fp);
      if (get_mob_index(vnum) == NULL)
  {
          bug("Fread_pet: bad vnum %d.",vnum);
      pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
  }
      else
          pet = create_mobile(get_mob_index(vnum));
    }
    else
    {
        bug("Fread_pet: no vnum in file.",0);
        pet = create_mobile(get_mob_index(MOB_VNUM_FIDO));
    }
    
    for ( ; ; )
    {
      word  = feof(fp) ? "END" : fread_word(fp);
      fMatch = FALSE;
      
      switch (UPPER(word[0]))
      {
      case '*':
          fMatch = TRUE;
          fread_to_eol(fp);
          break;
        
      case 'A':
          KEY( "Act",   pet->act,   fread_flag(fp));
          KEY( "AfBy",  pet->affected_by, fread_flag(fp));
          KEY( "Alig",  pet->alignment,   fread_number(fp));
          
          if (!str_cmp(word,"ACs"))
          {
            int i;
            
            for (i = 0; i < 4; i++)
                pet->armor[i] = fread_number(fp);
            fMatch = TRUE;
            break;
          }
          
          if (!str_cmp(word,"AffD"))
          {
            AFFECT_DATA *paf;
            int sn;
            
            paf = new_affect();
            
            sn = skill_lookup(fread_word(fp));
            if (sn < 0)
                bug("Fread_char: unknown skill.",0);
            else
               paf->type = sn;
               
            paf->level  = fread_number(fp);
            paf->duration = fread_number(fp);
            paf->modifier = fread_number(fp);
            paf->location = fread_number(fp);
            paf->bitvector  = fread_number(fp);
            paf->next = pet->affected;
            pet->affected = paf;
            fMatch    = TRUE;
            break;
          }

            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_char: unknown skill.",0);
                else
                   paf->type = sn;
 
    paf->where  = fread_number(fp);
                paf->level      = fread_number(fp);
                paf->duration   = fread_number(fp);
                paf->modifier   = fread_number(fp);
                paf->location   = fread_number(fp);
                paf->bitvector  = fread_number(fp);
                paf->next       = pet->affected;
                pet->affected   = paf;
                fMatch          = TRUE;
                break;
            }
           
          if (!str_cmp(word,"AMod"))
          {
            int stat;
            
              if(ch->version < 31)
		{
               for (stat = 0; stat < MAX_STATS-3; stat++)
                   pet->mod_stat[stat] = fread_number(fp);
               fMatch = TRUE;
               break;
		}
	      else
		 {
            for (stat = 0; stat < MAX_STATS; stat++)
                pet->mod_stat[stat] = fread_number(fp);
            fMatch = TRUE;
            break;
		}
          }
           
          if (!str_cmp(word,"Attr"))
          {
               int stat;
              if(ch->version < 31)
		{
               for (stat = 0; stat < MAX_STATS-3; stat++)
                   pet->perm_stat[stat] = fread_number(fp);
               fMatch = TRUE;
               break;
		}
	      else
		 {
               for (stat = 0; stat < MAX_STATS; stat++)
                   pet->perm_stat[stat] = fread_number(fp);
               fMatch = TRUE;
               break;
		 }
          }
          break;
           
       case 'C':
           KEY( "Comm", pet->comm,    fread_flag(fp));
           break;
           
       case 'D':
           KEY( "Dam",  pet->damroll,   fread_number(fp));
           KEY( "Desc", pet->description, fread_string(fp));
           break;
           
       case 'E':
           if (!str_cmp(word,"End"))
       {
    //pet->leader = ch;
    add_to_group(pet, ch);
    pet->master = ch;
    ch->pet = pet;
        /* adjust hp mana move up  -- here for speed's sake */
        percent = (current_time - lastlogoff) * 25 / ( 2 * 60 * 60);
 
        if (percent > 0 && !IS_AFFECTED(ch,AFF_POISON)
        &&  !IS_AFFECTED(ch,AFF_PLAGUE))
        {
        percent = UMIN(percent,100);
            pet->hit  += (pet->max_hit - pet->hit) * percent / 100;
              pet->mana   += (pet->max_mana - pet->mana) * percent / 100;
              pet->move   += (pet->max_move - pet->move)* percent / 100;
        }
            return;
       }
           KEY( "Exp",  pet->exp,   fread_number(fp));
           break;
           
       case 'G':
           KEY( "Gold", pet->gold,    fread_number(fp));
           break;
           
       case 'H':
           KEY( "Hit",  pet->hitroll,   fread_number(fp));
           
           if (!str_cmp(word,"HMV"))
           {
            pet->hit  = fread_number(fp);
            pet->max_hit  = fread_number(fp);
            pet->mana = fread_number(fp);
            pet->max_mana = fread_number(fp);
            pet->move = fread_number(fp);
            pet->max_move = fread_number(fp);
            fMatch = TRUE;
            break;
           }
           break;
           
      case 'L':
           KEY( "Levl", pet->level,   fread_number(fp));
           KEY( "LnD",  pet->long_descr,  fread_string(fp));
       KEY( "LogO", lastlogoff,   fread_number(fp));
           break;
           
      case 'N':
           KEY( "Name", pet->name,    fread_string(fp));
           break;
           
      case 'P':
           KEY( "Pos",  pet->position,    fread_number(fp));
           break;
           
  case 'R':
     if ( !str_cmp( word, "Race" ) ) {
          char *tmp = fread_string(fp);
          ch->race = race_lookup(tmp);
          free_string(tmp);
          fMatch = TRUE;
          break;
     }
      
      case 'S' :
          KEY( "Save",  pet->saving_throw,  fread_number(fp));
          KEY( "Sex",   pet->sex,   fread_number(fp));
          KEY( "ShD",   pet->short_descr, fread_string(fp));
            KEY( "Silv",        pet->silver,            fread_number( fp ) );
          break;
          
      if ( !fMatch )
      {
          bug("Fread_pet: no match.",0);
          fread_to_eol(fp);
      }
      
      }
    }
}



void fread_obj( CHAR_DATA *ch, FILE *fp )
{
    OBJ_DATA *obj;
    char *word;
    int iNest, nEnch;
    bool fMatch;
    bool fNest;
    bool fVnum;
    bool first;
    bool new_format;  /* to prevent errors */
    bool make_new;    /* update object */
    
    bool prefresh_char = FALSE;
    if(ch != NULL)
	prefresh_char = IS_SET(ch->mhs, MHS_PREFRESHED);

    fVnum = FALSE;
    obj = NULL;
    first = TRUE;  /* used to counter fp offset */
    new_format = FALSE;
    make_new = FALSE;

    word   = feof( fp ) ? "End" : fread_word( fp );
    if (!str_cmp(word,"Vnum" ))
    {
        int vnum;
  first = FALSE;  /* fp will be in right place */
 
        vnum = fread_number( fp );
        if (  get_obj_index( vnum )  == NULL )
  {
            bug( "Fread_obj: bad vnum %d.", vnum );
  }
        else
  {
      obj = create_object(get_obj_index(vnum),-1,FALSE);
      new_format = TRUE;
  }
      
    }

    if (obj == NULL)  /* either not found or old style */
    {
      obj = new_obj();
      obj->name   = str_dup( "" );
      obj->short_descr  = str_dup( "" );
      obj->description  = str_dup( "" );
      obj->stolen_timer = 0;
    }

    fNest   = FALSE;
    fVnum   = TRUE;
    iNest   = 0;

    for ( ; ; )
    {
  if (first)
      first = FALSE;
  else
      word   = feof( fp ) ? "End" : fread_word( fp );
  fMatch = FALSE;

  switch ( UPPER(word[0]) )
  {
  case '*':
      fMatch = TRUE;
      fread_to_eol( fp );
      break;

  case 'A':

      if (!str_cmp(word,"AffD"))
      {

    AFFECT_DATA *paf;
    int sn;

    paf = new_affect();

    sn = skill_lookup(fread_word(fp));
    if (sn < 0)
        bug("Fread_obj: unknown skill.",0);
    else
        paf->type = sn;

    paf->level  = fread_number( fp );
    paf->duration = fread_number( fp );
    paf->modifier = fread_number( fp );
    paf->location = fread_number( fp );
    paf->bitvector  = fread_number( fp );
    paf->next = obj->affected;
    obj->affected = paf;
    fMatch    = TRUE;
    break;
      }
            if (!str_cmp(word,"Affc"))
            {
                AFFECT_DATA *paf;
                int sn;
 
                paf = new_affect();
 
                sn = skill_lookup(fread_word(fp));
                if (sn < 0)
                    bug("Fread_obj: unknown skill.",0);
                else
                    paf->type = sn;
                
/*		if(!IS_SET(ch->mhs, MHS_PREFRESHED) ||
		  sn != skill_lookup("enchant weapon")) 
                 {
*/

                paf->where  = fread_number( fp );
                paf->level      = fread_number( fp );
                paf->duration   = fread_number( fp );

		if(!prefresh_char ||
		  sn != skill_lookup("enchant weapon")) 
                {
                   paf->modifier   = fread_number( fp );
		}
		else
		{
                   nEnch         = fread_number ( fp );
		   nEnch = UMIN(nEnch/2, 8);
		   paf->modifier = nEnch;
		}

                paf->location   = fread_number( fp );
                paf->bitvector  = fread_number( fp );
                paf->next       = obj->affected;
                obj->affected   = paf;
                fMatch          = TRUE;
                break;
/*
                }
		else 
		{
		paf->where = fread_number (fp);
		paf->level = fread_number (fp);
		paf->duration = fread_number (fp);


                nEnch         = fread_number (fp);

		nEnch = UMIN(nEnch/2, 8);
		paf->modifier = nEnch;
		
		paf->location = fread_number (fp);
		paf->bitvector = fread_number (fp);
		paf->next      = paf;
		obj->affected = paf;
		fMatch = TRUE;
		break;
		}
*/
	    }
      break;

  case 'C':
  if (!prefresh_char)
  {
      KEY( "Cond",  obj->condition,   fread_number( fp ) );
      KEY( "Cost",  obj->cost,    fread_number( fp ) );
  }    
      break;
  
  case 'D':
      KEY( "Description", obj->description, fread_string( fp ) );
      KEY( "Desc",  obj->description, fread_string( fp ) );
	if ( !str_cmp( word, "Damaged" ))
	{
	  obj->damaged = fread_number( fp );
	  fMatch = TRUE;
	  break;
	}
      break;

  case 'E':

      if ( !str_cmp( word, "Enchanted"))
      {
    obj->enchanted = TRUE;
    fMatch  = TRUE;
    break;
      }
    
    if (!prefresh_char)
    {
      KEY( "ExtraFlags",  obj->extra_flags, fread_number( fp ) );
      KEY( "ExtF",  obj->extra_flags, fread_number( fp ) );
    }

      if ( !str_cmp( word, "ExtraDescr" ) || !str_cmp(word,"ExDe"))
      {
    EXTRA_DESCR_DATA *ed;

    ed = new_extra_descr();

    ed->keyword   = fread_string( fp );
    ed->description   = fread_string( fp );
    ed->next    = obj->extra_descr;
    obj->extra_descr  = ed;
    fMatch = TRUE;
      }

      if ( !str_cmp( word, "End" ) )
      {
    if ( !fNest || !fVnum || obj->pIndexData == NULL)
    {
        bug( "Fread_obj: incomplete object.", 0 );
        free_obj(obj);
        return;
    }
    else
    {
        if (!new_format)
        {
          obj->next = object_list;
          object_list = obj;
          obj->pIndexData->count++;
        }

        if (!obj->pIndexData->new_format 
        && obj->item_type == ITEM_ARMOR
        &&  obj->value[1] == 0)
        {
      obj->value[1] = obj->value[0];
      obj->value[2] = obj->value[0];
        }
        if (make_new && ch != NULL)
        {
      int wear;
      
      wear = obj->wear_loc;
      extract_obj(obj);

      obj = create_object(obj->pIndexData,0,FALSE);
      obj->wear_loc = wear;
        }
        if ( iNest == 0 || rgObjNest[iNest] == NULL )
      obj_to_char( obj, ch );
        else
      obj_to_obj( obj, rgObjNest[iNest-1] );
      set_rarity(obj);
        return;
    }
      }
      break;

  case 'I':
      KEY( "ItemType",  obj->item_type,   fread_number( fp ) );
      KEY( "Ityp",  obj->item_type,   fread_number( fp ) );
      break;

  case 'L':

   if (!prefresh_char)
   {
      KEY( "Level", obj->level,   fread_number( fp ) );
      KEY( "Lev",   obj->level,   fread_number( fp ) );
   } 
   if(!str_cmp(word, "Link"))
   {
     fMatch = TRUE;
     if(ch && ch->pcdata)
     {
       obj->link_name = str_dup(fread_word(fp));
       if(!str_cmp(obj->link_name, ch->name))
       {
         int i;
         for(i = 0; i < LINK_MAX; i++)
         {
           if(ch->pcdata->linked[i] == NULL)
             break;
         }
         if(i == LINK_MAX)
           clear_string(&obj->link_name, NULL);
         else
           ch->pcdata->linked[i] = obj;
       }
     }
   }
      break;

  case 'N':
      KEY( "Name",  obj->name,    fread_string( fp ) );

      if ( !str_cmp( word, "Nest" ) )
      {
    iNest = fread_number( fp );
    if ( iNest < 0 || iNest >= MAX_NEST )
    {
        bug( "Fread_obj: bad nest %d.", iNest );
    }
    else
    {
        rgObjNest[iNest] = obj;
        fNest = TRUE;
    }
    fMatch = TRUE;
      }
      break;

    case 'O':
      if ( !str_cmp( word,"Oldstyle" ) )
      {
    if (obj->pIndexData != NULL && obj->pIndexData->new_format)
        make_new = TRUE;
    fMatch = TRUE;
      }
      break;
    
    case 'P':
      KEY( "PrevOwner", obj->prev_owner, fread_string(fp) );
      break;

  case 'S':
      KEY( "ShortDescr",  obj->short_descr, fread_string( fp ) );
      KEY( "ShD",   obj->short_descr, fread_string( fp ) );
      KEY( "Stolen",  obj->stolen_timer,   fread_number( fp ) );

      if ( !str_cmp( word, "Spell" ) )
      {
    int iValue;
    int sn;

    iValue = fread_number( fp );
    sn     = skill_lookup( fread_word( fp ) );
    if ( iValue < 0 || iValue > 3 )
    {
        bug( "Fread_obj: bad iValue %d.", iValue );
    }
    else if ( sn < 0 )
    {
        bug( "Fread_obj: unknown skill.", 0 );
    }
    else
    {
        obj->value[iValue] = sn;
    }
    fMatch = TRUE;
    break;
      }

      break;

  case 'T':
      KEY( "Timer", obj->timer,   fread_number( fp ) );
      KEY( "Time",  obj->timer,   fread_number( fp ) );
      break;

  case 'V':
      if ( (!str_cmp( word, "Values" ) || !str_cmp(word,"Vals")))
      {
	if    (!prefresh_char)
      {
    obj->value[0] = fread_number( fp );
    obj->value[1] = fread_number( fp );
    obj->value[2] = fread_number( fp );
    obj->value[3] = fread_number( fp );
    if (obj->item_type == ITEM_WEAPON && obj->value[0] == 0)
       obj->value[0] = obj->pIndexData->value[0];
      }
    fMatch    = TRUE;
    break;
      }

      if ( !str_cmp( word, "Val") )  
      {
      if (!prefresh_char)
      {
    obj->value[0]   = fread_number( fp );
    obj->value[1] = fread_number( fp );
    obj->value[2]   = fread_number( fp );
    obj->value[3] = fread_number( fp );
    obj->value[4] = fread_number( fp );
      }
   fMatch = TRUE;
    break;
      }

      if ( !str_cmp( word, "Vnum" ) )
      {
    int vnum;

    vnum = fread_number( fp );
    if ( ( obj->pIndexData = get_obj_index( vnum ) ) == NULL )
        bug( "Fread_obj: bad vnum %d.", vnum );
    else
        fVnum = TRUE;
    fMatch = TRUE;
    break;
      }
      break;

  case 'W':
  if (!prefresh_char)
  {
      KEY( "Wrp", obj->warps, fread_number( fp ) );
      KEY( "WearFlags", obj->wear_flags,  fread_number( fp ) );
      KEY( "WeaF",  obj->wear_flags,  fread_number( fp ) );
      KEY( "WearLoc", obj->wear_loc,    fread_number( fp ) );
      KEY( "Wear",  obj->wear_loc,    fread_number( fp ) );
      KEY( "WearTime",  obj->wear_timer,    fread_number( fp ) );
      KEY( "Weight",  obj->weight,    fread_number( fp ) );
      KEY( "Wt",    obj->weight,    fread_number( fp ) );
      break;
  }
  }

  if ( !fMatch )
  {
	char buf[256];
	sprintf(buf, "Fread_obj: no match for %s.", word);
	bug(buf, 0);
//      bug( "Fread_obj: no match.", 0 );
      fread_to_eol( fp );
  }
    }
}

void save_pits(void)
{
	OBJ_DATA *obj;
	char strsave[MAX_INPUT_LENGTH];
	FILE *fp;
  sprintf( strsave, "%spits.save", PLAYER_DIR);//pits.save into the player directory
//	strcpy(strsave, "pits.save");// Leave it wherever, it wasn't saving in the player dir
  if ( ( fp = fopen( TEMP_FILE, "w" ) ) == NULL )
  {
	  bug( "Save_pits: fopen", 0 );
  	perror( strsave );
  	return;
  }
	for ( obj = object_list; obj != NULL; obj = obj->next )
  {// This is a horrible if statement, I apologize. Need a pit flag eventually.
	  if ((obj->pIndexData->vnum == OBJ_VNUM_PIT || obj->pIndexData->vnum == OBJ_VNUM_MATOOK_PIT || obj->pIndexData->vnum == OBJ_VNUM_DEMISE_PIT || obj->pIndexData->vnum == OBJ_VNUM_HONOR_PIT || obj->pIndexData->vnum == OBJ_VNUM_POSSE_PIT || obj->pIndexData->vnum == OBJ_VNUM_ZEALOT_PIT || obj->pIndexData->vnum == OBJ_VNUM_WARLOCK_PIT || obj->pIndexData->vnum == OBJ_VNUM_FLUZEL_PIT)
		&& !CAN_WEAR(obj,ITEM_TAKE))
	  {
    	if ( obj->contains != NULL && obj->carried_by == NULL && obj->in_room != NULL)
    	{
    		fprintf(fp, "Pit %d %d\n", obj->pIndexData->vnum, obj->in_room->vnum);
    		rgObjNest[0] = obj;// This item is the top, everything in it is nested
  			fwrite_obj(NULL, obj->contains, fp, 1);// Start with the first object it contains
  		}
	}
  }
  fprintf(fp, "#END\n");
  fclose( fp );
  rename(TEMP_FILE,strsave);// Done
}

/* Locate an object by its vnum and the room vnum it's in */
OBJ_DATA *find_ground_obj( int vnum, int room )
{
	OBJ_DATA *obj;
	for ( obj = object_list; obj != NULL; obj = obj->next )
	{
		if(obj->pIndexData->vnum == vnum && obj->in_room != NULL && obj->in_room->vnum == room && !obj->contains)// Don't fill ground containers
			return obj;// Found it
	}
	return NULL;// Failed to find it
}

		
bool locate_pit(FILE *fp, bool first)
{
	int vnum, room, iNest;
  for ( iNest = 0; iNest < MAX_NEST; iNest++ )
      rgObjNest[iNest] = NULL;
	char *word;
	if(first)
	    word = fread_word(fp);
	else
	    word = "Pit";
	while(!feof(fp))
	{
		if(!str_cmp(word, "Pit"))
		{
			vnum = fread_number( fp );
			room = fread_number( fp );
			if ( (rgObjNest[0] = find_ground_obj( vnum, room ))  == NULL )
			{// It will try again in the outer loop since this pit was invalid - contents are discarded
				char buf[256];
				sprintf(buf, "locate_pit: bad container vnum %d room %d.", vnum, room );
				bug(buf, 0);
				fread_to_eol(fp);
				if(feof(fp))
				{
					return FALSE;
				}
			}
			else
				return TRUE;// Located a pit and loaded it into the nest
		}			
		else if(!str_cmp(word, "#End"))
			return FALSE;
		else
			fread_to_eol(fp);
		word = fread_word(fp);
	}
	return FALSE;// No valid keyword found
}

void load_pits(void)
{
	bool first = TRUE;
	char strsave[MAX_INPUT_LENGTH];
	char *in;
	FILE *fp;
  sprintf( strsave, "%spits.save", PLAYER_DIR);//pits.save into the player directory
//  strcpy(strsave, "pits.save");// Wasn't working in the previous dir, lets try now
  if ( ( fp = fopen( strsave, "r" ) ) == NULL )
  {
	  bug( "Load_pits: fopen", 0 );
  	return;// May just not exist, don't make too big deal about it
  }
  while(!feof(fp))
  {
		if(locate_pit(fp, first))
		{
			for( ; ; )
			{
				in = fread_word(fp);// #O
				if(!str_cmp("#O", in) || ! str_cmp("#OBJECT", in))
					fread_obj(NULL, fp);
				else if(!str_cmp("#END", in))
				{// Done
					fclose(fp);
					return;
				}
				else
					break;// It's another pit
			}
		}
		else
			break;
		first = FALSE;
	}
  fclose( fp );
}
