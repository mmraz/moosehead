/* db_new.c */
static char rcsid[] = "$Id: db2.c,v 1.10 2001/06/25 20:40:04 mud Exp $";
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#ifdef GAME_VERSION
 #include "gc.h"
#endif
#include <ctype.h>
#include <time.h>
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif

#include "merc.h"
#include "db.h"
#include "lookup.h"


/* values for db2.c */
struct                         social_type social_table    [MAX_SOCIALS];
int                            social_count    = 0;
int                            social_count_targeted = 0;
extern AREA_DATA *             area_last;

extern void rename_area (char *strArea);

/* snarf a socials file */
void load_socials( FILE *fp)
{
    for ( ; ; ) 
    {
      struct social_type social;
      char *temp;
        /* clear social */
  social.char_no_arg = NULL;
  social.others_no_arg = NULL;
  social.char_found = NULL;
  social.others_found = NULL;
  social.vict_found = NULL; 
  social.char_not_found = NULL;
  social.char_auto = NULL;
  social.others_auto = NULL;

      temp = fread_word(fp);
      if (!strcmp(temp,"#0"))
      return;  /* done */
#if defined(social_debug) 
  else 
      fprintf(stderr,"%s\n\r",temp);
#endif

      strcpy(social.name,temp);
      fread_to_eol(fp);

  temp = fread_string_eol(fp);
  if (!strcmp(temp,"$"))
       social.char_no_arg = NULL;
  else if (!strcmp(temp,"#"))
  {
       social_table[social_count] = social;
       social_count++;
       continue; 
  }
        else
      social.char_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_no_arg = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.others_no_arg = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_found = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.char_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_found = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.others_found = temp; 

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.vict_found = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
	{
	     social.vict_found = temp;
	     social_count_targeted++;
	}

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_not_found = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.char_not_found = temp;

        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.char_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
       social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.char_auto = temp;
         
        temp = fread_string_eol(fp);
        if (!strcmp(temp,"$"))
             social.others_auto = NULL;
        else if (!strcmp(temp,"#"))
        {
             social_table[social_count] = social;
             social_count++;
             continue;
        }
        else
      social.others_auto = temp; 
  
  social_table[social_count] = social;
      social_count++;
   }
   return;
}
    





/*
 * Snarf a mob section.  new style
 */
void load_mobiles( FILE *fp )
{
    MOB_INDEX_DATA *pMobIndex;
 
    for ( ; ; )
    {
        sh_int vnum;
        char letter;
        int iHash;
 
        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_mobiles: # not found.", 0 );
            if ( fp != stdin ) fclose( fp );
            fp = NULL;
            if (area_last)
              rename_area (area_last->file_name);                                        
            exit( 1 );
        }
 
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
 
        fBootDb = FALSE;
        if ( get_mob_index( vnum ) != NULL )
        {
            bug( "Load_mobiles: vnum %d duplicated.", vnum );
            if ( fp != stdin ) fclose( fp );
            fp = NULL;
            if (area_last)
              rename_area (area_last->file_name);                          
            exit( 1 );
        }
        fBootDb = TRUE;
        
        if (area_last->min_vnum_mob == 0) {
          area_last->min_vnum_mob = vnum;
          area_last->max_vnum_mob = vnum;  
        } else {  
          if (vnum > area_last->max_vnum_mob)
            area_last->max_vnum_mob = vnum;
          if (vnum < area_last->min_vnum_mob)
            area_last->min_vnum_mob = vnum;
        }          
 
#ifdef OLC_VERSION
        pMobIndex                       = alloc_perm( sizeof(*pMobIndex) );
#else /*game version*/
        pMobIndex                       = GC_MALLOC( sizeof(*pMobIndex) );
#endif
        pMobIndex->vnum                 = vnum;
	pMobIndex->area			= area_last;
  pMobIndex->new_format   = TRUE;
  newmobs++;
        pMobIndex->player_name          = fread_string( fp );
        pMobIndex->short_descr          = fread_string( fp );
        pMobIndex->long_descr           = fread_string( fp );
        pMobIndex->description          = fread_string( fp );
  pMobIndex->race     = race_lookup(fread_string( fp ));
 
        pMobIndex->long_descr[0]        = UPPER(pMobIndex->long_descr[0]);
        pMobIndex->description[0]       = UPPER(pMobIndex->description[0]);

	pMobIndex->spec_words[0]	= fread_string( fp ); 
	pMobIndex->spec_words[1]	= fread_string( fp ); 
	pMobIndex->spec_words[2]	= fread_string( fp ); 

        pMobIndex->act                  = fread_flag( fp ) | ACT_IS_NPC
          | race_table[pMobIndex->race].act;

	  /* Speial handling, no trans areas */
	if ( IS_SET(pMobIndex->act,ACT_NOTRANS) )
		pMobIndex->area->no_transport = TRUE; 

        pMobIndex->affected_by          = fread_flag( fp )
          | race_table[pMobIndex->race].aff;
        pMobIndex->pShop                = NULL;
        pMobIndex->alignment            = fread_number( fp );
        pMobIndex->group                = fread_number( fp );

        pMobIndex->level                = fread_number( fp );
        pMobIndex->hitroll              = fread_number( fp );  

  /* read hit dice */
        pMobIndex->hit[DICE_NUMBER]     = fread_number( fp );  
        /* 'd'          */                fread_letter( fp ); 
        pMobIndex->hit[DICE_TYPE]     = fread_number( fp );
        /* '+'          */                fread_letter( fp );   
        pMobIndex->hit[DICE_BONUS]      = fread_number( fp ); 

  /* read mana dice */
  pMobIndex->mana[DICE_NUMBER]  = fread_number( fp );
            fread_letter( fp );
  pMobIndex->mana[DICE_TYPE]  = fread_number( fp );
            fread_letter( fp );
  pMobIndex->mana[DICE_BONUS] = fread_number( fp );

  /* read damage dice */
  pMobIndex->damage[DICE_NUMBER]  = fread_number( fp );
            fread_letter( fp );
  pMobIndex->damage[DICE_TYPE]  = fread_number( fp );
            fread_letter( fp );
  pMobIndex->damage[DICE_BONUS] = fread_number( fp );
  pMobIndex->dam_type   = attack_lookup(fread_word(fp));

  /* read armor class */
  pMobIndex->ac[AC_PIERCE]  = fread_number( fp ) * 10;
  pMobIndex->ac[AC_BASH]    = fread_number( fp ) * 10;
  pMobIndex->ac[AC_SLASH]   = fread_number( fp ) * 10;
  pMobIndex->ac[AC_EXOTIC]  = fread_number( fp ) * 10;

  /* read flags and add in data from the race table */
  pMobIndex->off_flags    = fread_flag( fp ) 
          | race_table[pMobIndex->race].off;
  pMobIndex->imm_flags    = fread_flag( fp )
          | race_table[pMobIndex->race].imm;
  pMobIndex->res_flags    = fread_flag( fp )
          | race_table[pMobIndex->race].res;
  pMobIndex->vuln_flags   = fread_flag( fp )
          | race_table[pMobIndex->race].vuln;

  /* vital statistics */
  pMobIndex->start_pos    = position_lookup(fread_word(fp));
  pMobIndex->default_pos    = position_lookup(fread_word(fp));
  pMobIndex->sex      = sex_lookup(fread_word(fp));

  pMobIndex->wealth   = fread_number( fp );

  pMobIndex->form     = fread_flag( fp )
          | race_table[pMobIndex->race].form;
  pMobIndex->parts    = fread_flag( fp )
          | race_table[pMobIndex->race].parts;
  /* size */
  pMobIndex->size     = size_lookup(fread_word(fp));
  pMobIndex->material   = str_dup(fread_word( fp ));
 
  for ( ; ; )
        {
            letter = fread_letter( fp );

            if (letter == 'F')
            {
    char *word;
    long vector;

                word                    = fread_word(fp);
    vector      = fread_flag(fp);

    if (!str_prefix(word,"act"))
        REMOVE_BIT(pMobIndex->act,vector);
                else if (!str_prefix(word,"aff"))
        REMOVE_BIT(pMobIndex->affected_by,vector);
    else if (!str_prefix(word,"off"))
        REMOVE_BIT(pMobIndex->affected_by,vector);
    else if (!str_prefix(word,"imm"))
        REMOVE_BIT(pMobIndex->imm_flags,vector);
    else if (!str_prefix(word,"res"))
        REMOVE_BIT(pMobIndex->res_flags,vector);
    else if (!str_prefix(word,"vul"))
        REMOVE_BIT(pMobIndex->vuln_flags,vector);
    else if (!str_prefix(word,"for"))
        REMOVE_BIT(pMobIndex->form,vector);
    else if (!str_prefix(word,"par"))
        REMOVE_BIT(pMobIndex->parts,vector);
    else
    {
        bug("Flag remove: flag not found.",0);
        if ( fp != stdin ) fclose( fp );
        fp = NULL;
        if (area_last)
          rename_area (area_last->file_name);                      
        exit(1);
    }
       }
       else
       {
    ungetc(letter,fp);
    break;
       }
  }

        iHash                   = vnum % MAX_KEY_HASH;
        pMobIndex->next         = mob_index_hash[iHash];
        mob_index_hash[iHash]   = pMobIndex;
        top_mob_index++;
        kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }
 
    return;
}

/*
 * Snarf an obj section. new style
 */
void load_objects( FILE *fp )
{
    OBJ_INDEX_DATA *pObjIndex;
 
    for ( ; ; )
    {
        sh_int vnum;
        char letter;
        int iHash;
 
        letter                          = fread_letter( fp );
        if ( letter != '#' )
        {
            bug( "Load_objects: # not found.", 0 );
            if ( fp != stdin ) fclose( fp );
            fp = NULL;
            if (area_last)
              rename_area (area_last->file_name);                          
            exit( 1 );
        }
 
        vnum                            = fread_number( fp );
        if ( vnum == 0 )
            break;
 
        fBootDb = FALSE;
        if ( get_obj_index( vnum ) != NULL )
        {
            bug( "Load_objects: vnum %d duplicated.", vnum );
            if ( fp != stdin ) fclose( fp );
            fp = NULL;
            if (area_last)
              rename_area (area_last->file_name);                          
            exit( 1 );
        }
        fBootDb = TRUE;
        
        if (area_last->min_vnum_obj == 0) {
          area_last->min_vnum_obj = vnum;
          area_last->max_vnum_obj = vnum;  
        } else {  
          if (vnum > area_last->max_vnum_obj)
            area_last->max_vnum_obj = vnum;
          if (vnum < area_last->min_vnum_obj)
            area_last->min_vnum_obj = vnum;
        }            
 
#ifdef OLC_VERSION
        pObjIndex                       = alloc_perm( sizeof(*pObjIndex) );
#else
        pObjIndex                       = GC_MALLOC( sizeof(*pObjIndex) );
#endif
        pObjIndex->vnum                 = vnum;
        pObjIndex->area                 = area_last;
        pObjIndex->new_format           = TRUE;
  pObjIndex->reset_num    = 0;
  newobjs++;
        pObjIndex->name                 = fread_string( fp );
        pObjIndex->short_descr          = fread_string( fp );
        pObjIndex->description          = fread_string( fp );
        pObjIndex->material   = fread_string( fp );
 
        pObjIndex->item_type            = item_lookup(fread_word( fp ));
        pObjIndex->extra_flags          = fread_flag( fp );
        pObjIndex->wear_flags           = fread_flag( fp );
  switch(pObjIndex->item_type)
  {
  case ITEM_WEAPON:
      pObjIndex->value[0]   = weapon_type(fread_word(fp));
      pObjIndex->value[1]   = fread_number(fp);
      pObjIndex->value[2]   = fread_number(fp);
      pObjIndex->value[3]   = attack_lookup(fread_word(fp));
      pObjIndex->value[4]   = fread_flag(fp);
      break;
  case ITEM_CONTAINER:
      pObjIndex->value[0]   = fread_number(fp);
      pObjIndex->value[1]   = fread_flag(fp);
      pObjIndex->value[2]   = fread_number(fp);
      pObjIndex->value[3]   = fread_number(fp);
      pObjIndex->value[4]   = fread_number(fp);
      break;
        case ITEM_DRINK_CON:
  case ITEM_FOUNTAIN:
            pObjIndex->value[0]         = fread_number(fp);
            pObjIndex->value[1]         = fread_number(fp);
            pObjIndex->value[2]         = liq_lookup(fread_word(fp));
            pObjIndex->value[3]         = fread_number(fp);
            pObjIndex->value[4]         = fread_number(fp);
            break;
  case ITEM_WAND:
  case ITEM_STAFF:
      pObjIndex->value[0]   = fread_number(fp);
      pObjIndex->value[1]   = fread_number(fp);
      pObjIndex->value[2]   = fread_number(fp);
      pObjIndex->value[3]   = skill_lookup(fread_word(fp));
      pObjIndex->value[4]   = fread_number(fp);
      break;
  case ITEM_SPELL_PAGE:
      pObjIndex->value[0]	= fread_number(fp);
      pObjIndex->value[1]	= fread_number(fp);
      pObjIndex->value[2]	= skill_lookup( fread_word(fp) );
      pObjIndex->value[3]	= fread_number(fp);
      pObjIndex->value[4]	= fread_number(fp);
      break;
  case ITEM_PART:
      pObjIndex->value[0]	= fread_number(fp);
      pObjIndex->value[1]	= fread_number(fp);
      pObjIndex->value[2]	= fread_number(fp); 
      pObjIndex->value[3]	= fread_number(fp);
      pObjIndex->value[4]	= fread_number(fp);
      break;
  case ITEM_POTION:
  case ITEM_PILL:
  case ITEM_SCROLL:
      pObjIndex->value[0]   = fread_number(fp);
      pObjIndex->value[1]   = skill_lookup(fread_word(fp));
      pObjIndex->value[2]   = skill_lookup(fread_word(fp));
      pObjIndex->value[3]   = skill_lookup(fread_word(fp));
      pObjIndex->value[4]   = skill_lookup(fread_word(fp));
      break;
  default:
            pObjIndex->value[0]             = fread_flag( fp );
            pObjIndex->value[1]             = fread_flag( fp );
            pObjIndex->value[2]             = fread_flag( fp );
            pObjIndex->value[3]             = fread_flag( fp );
      pObjIndex->value[4]       = fread_flag( fp );
      break;
  }
  pObjIndex->level    = fread_number( fp );
        pObjIndex->weight               = fread_number( fp );
        pObjIndex->cost                 = fread_number( fp ); 

        /* condition */
        letter        = fread_letter( fp );
  switch (letter)
  {
      case ('P') :    pObjIndex->condition = 100; break;
      case ('G') :    pObjIndex->condition =  90; break;
      case ('A') :    pObjIndex->condition =  75; break;
      case ('W') :    pObjIndex->condition =  50; break;
      case ('D') :    pObjIndex->condition =  25; break;
      case ('B') :    pObjIndex->condition =  10; break;
      case ('R') :    pObjIndex->condition =   0; break;
      default:      pObjIndex->condition = 100; break;
  }
 
        for ( ; ; )
        {
            char letter;
 
            letter = fread_letter( fp );
 
            if ( letter == 'A' )
            {
                AFFECT_DATA *paf;

#ifdef OLC_VERSION
                paf                     = alloc_perm( sizeof(*paf) );
#else
                paf                     = GC_MALLOC( sizeof(*paf) );
#endif
		paf->where    = TO_OBJECT;
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number( fp );
                paf->modifier           = fread_number( fp );
                paf->bitvector          = 0;
                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }

      else if (letter == 'F')
            {
                AFFECT_DATA *paf;
 
#ifdef OLC_VERSION
                paf                     = alloc_perm( sizeof(*paf) );
#else           
                paf                     = GC_MALLOC( sizeof(*paf) );
#endif
    letter      = fread_letter(fp);
    switch (letter)
    {
    case 'A':
                    paf->where          = TO_AFFECTS;
        break;
    case 'I':
        paf->where    = TO_IMMUNE;
        break;
    case 'R':
        paf->where    = TO_RESIST;
        break;
    case 'V':
        paf->where    = TO_VULN;
        break;
    default:
                  bug( "Load_objects: Bad where on flag set.", 0 );
                  if ( fp != stdin ) fclose( fp );
                  fp = NULL;
                  if (area_last)
                    rename_area (area_last->file_name);                                
                 exit( 1 );
    }
                paf->type               = -1;
                paf->level              = pObjIndex->level;
                paf->duration           = -1;
                paf->location           = fread_number(fp);
                paf->modifier           = fread_number(fp);
                paf->bitvector          = fread_flag(fp);
                paf->next               = pObjIndex->affected;
                pObjIndex->affected     = paf;
                top_affect++;
            }
 
            else if ( letter == 'E' )
            {
                EXTRA_DESCR_DATA *ed;
#ifdef OLC_VERSION
                ed                     = alloc_perm( sizeof(*ed) );
#else           
                ed                      = GC_MALLOC( sizeof(*ed) );
#endif
                ed->keyword             = fread_string( fp );
                ed->description         = fread_string( fp );
                ed->next                = pObjIndex->extra_descr;
                pObjIndex->extra_descr  = ed;
                top_ed++;
            }
 
            else
            {
                ungetc( letter, fp );
                break;
            }
        }
 
        iHash                   = vnum % MAX_KEY_HASH;
        pObjIndex->next         = obj_index_hash[iHash];
        obj_index_hash[iHash]   = pObjIndex;
        top_obj_index++;
    }
 
    return;
}

