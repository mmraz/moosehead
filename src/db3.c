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
 
static char rcsid[] = "$Id: db3.c,v 1.11 2001/12/04 17:54:52 rage Exp $";
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#ifdef GAME_VERSION
#include "gc.h"
#endif
#include <time.h>
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif
 
#include "merc.h"
#include "tables.h"
#include "db.h"

int fputc(int,FILE *);

extern char *  const   dir_name        [];

void fdump(FILE *file, char *string)
{
    const char *p;

    for (p = string; *p != '\0' ; p++)
    {
  if (*p != '\r')
      fputc(*p,file);
    }
}

char *print_flags(long flag)
{
    int count, pos = 0;
    static char buf[52];


    for (count = 0; count < 32;  count++)
    {
  if (IS_SET(flag,1<<count))
  {
      if (count < 26)
    buf[pos] = 'A' + count;
      else
    buf[pos] = 'a' + (count - 26);
      pos++;
  }
    }

    if (pos == 0)
    {
  buf[pos] = '0';
  pos++;
    }
 
    buf[pos] = '\0';

    return buf;
}


/* funky outputting stuff! */
void save_area ( CHAR_DATA *ch, AREA_DATA *pArea )
{       
  ROOM_INDEX_DATA *room;
  OBJ_INDEX_DATA *obj;
  MOB_INDEX_DATA *mob;
  SHOP_DATA *shop;
  RESET_DATA *reset;
  EXIT_DATA *exit;
  EXTRA_DESCR_DATA *ed;
  AFFECT_DATA *af;
  FILE *fp;
  char *pstr,*pstr2;
  char buf[MAX_STRING_LENGTH],
       buf2[MAX_STRING_LENGTH],
       sbuf[MAX_STRING_LENGTH];
  int vnum,door,flags;

  sprintf(buf,"%s",pArea->file_name);
//  fclose(fpReserve);
  
  fp = fopen(buf,"r");
  if (fp != NULL) {    /* if the file exists, rename it */
    fclose (fp);
    strcpy (buf2,buf);
    pstr = buf2;
    while (pstr) {
      pstr2 = ++pstr;
      pstr = strchr (pstr,'.');
    }
    *pstr2 = NULL;
    strcat (buf2,"bak");
    fp = fopen (buf2,"r");
    if (fp != NULL) {
      fclose (fp);
      unlink (buf2);
    }
    rename (buf,buf2);
    sprintf (sbuf,"Renaming old '%s' as '%s'.\n\r",buf,buf2);
    send_to_char (sbuf,ch);
  }
  
  fp = fopen(buf,"w");

  if (fp == NULL)
  {
      send_to_char("Failed to open area file.\n\r",ch);
//      fpReserve = fopen( NULL_FILE, "r" );
      return;
  }

  fprintf(fp,"#AREA\n");
  fprintf(fp,"%s~\n",pArea->file_name);
  fprintf(fp,"%s~\n",pArea->name);
  fprintf(fp,"%s~\n",pArea->credits);
  fprintf(fp,"%d %d\n",get_area_min_vnum(pArea),get_area_max_vnum(pArea));

  fprintf(fp,"\n");

  /* moose */
  fprintf(fp,"#MOBILES\n");
  
  for (vnum = pArea->min_vnum_mob; vnum <= pArea->max_vnum_mob; vnum++)
  {
      if ((mob = get_mob_index(vnum)) == NULL)
    continue;


      fprintf(fp,"#%d\n",mob->vnum);
      if (mob->new_format)
        fprintf(fp,"%s~\n",mob->player_name);
      else
    fprintf(fp,"oldstyle %s~\n",mob->player_name);
      fprintf(fp,"%s~\n",mob->short_descr);
      sprintf(buf,"%s~\n",mob->long_descr);
      fdump(fp,buf);
      sprintf(buf,"%s~\n",mob->description);
      fdump(fp,buf);
      fprintf(fp,"%s~\n",race_table[mob->race].name);
      fprintf(fp,"%s~\n",mob->spec_words[0]);
      fprintf(fp,"%s~\n",mob->spec_words[1]);
      fprintf(fp,"%s~\n",mob->spec_words[2]);
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].act,mob->act))),
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].aff,
                  mob->affected_by)));
      fprintf(fp,"%d %d\n",mob->alignment,mob->group);
      fprintf(fp,"%d %d %dd%d+%d %dd%d+%d %dd%d+%d %s\n",
    mob->level, mob->hitroll,
    mob->hit[DICE_NUMBER],mob->hit[DICE_TYPE],mob->hit[DICE_BONUS],
    mob->mana[DICE_NUMBER],mob->mana[DICE_TYPE],
    mob->mana[DICE_BONUS],
    mob->damage[DICE_NUMBER],mob->damage[DICE_TYPE],
    mob->damage[DICE_BONUS], attack_table[mob->dam_type].name);
      fprintf(fp,"%d %d %d %d\n",
    mob->ac[AC_PIERCE]/10,mob->ac[AC_BASH]/10,
    mob->ac[AC_SLASH]/10,mob->ac[AC_EXOTIC]/10);
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].off,
              mob->off_flags)));
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].imm,
              mob->imm_flags)));
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].res,
              mob->res_flags)));
      fprintf(fp,"%s\n",
    print_flags(GET_UNSET(race_table[mob->race].vuln,
              mob->vuln_flags)));
      fprintf(fp,"%s %s %s %ld\n",
    position_table[mob->start_pos].short_name,
    position_table[mob->default_pos].short_name,
    sex_table[mob->sex].name,mob->wealth);
      /* Gold check */
/*
      if (mob->wealth > mob->level * 100)
    fprintf(stderr,"Mobile %d has %ld wealth at level %d.\n",         mob->vnum,mob->wealth,mob->level);
*/
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].form,mob->form)));
      fprintf(fp,"%s ",
    print_flags(GET_UNSET(race_table[mob->race].parts,
              mob->parts)));

      if (strchr(mob->material,' ') == NULL) /* no spaces */
    fprintf(fp,"%s %s\n",size_table[mob->size].name,mob->material);
      else
        fprintf(fp,"%s '%s'\n",
        size_table[mob->size].name,mob->material);

      /* print unset flags */
      flags = GET_UNSET(mob->act,race_table[mob->race].act);
      if (flags != 0)
    fprintf(fp,"F act %s\n",print_flags(flags));
      
      flags = GET_UNSET(mob->affected_by,race_table[mob->race].aff);
            if (flags != 0)
                fprintf(fp,"F aff %s\n",print_flags(flags));

      flags = GET_UNSET(mob->off_flags,race_table[mob->race].off);
            if (flags != 0)
                fprintf(fp,"F off %s\n",print_flags(flags));

      flags = GET_UNSET(mob->imm_flags,race_table[mob->race].imm);
            if (flags != 0)
                fprintf(fp,"F imm %s\n",print_flags(flags));

      flags = GET_UNSET(mob->res_flags,race_table[mob->race].res);
            if (flags != 0)
                fprintf(fp,"F res %s\n",print_flags(flags));

      flags = GET_UNSET(mob->vuln_flags,race_table[mob->race].vuln);
            if (flags != 0)
                fprintf(fp,"F vul %s\n",print_flags(flags));

      flags = GET_UNSET(mob->form,race_table[mob->race].form);
      if (flags != 0)
    fprintf(fp,"F for %s\n",print_flags(flags));

      flags = GET_UNSET(mob->parts,race_table[mob->race].parts);
            if (flags != 0)
                fprintf(fp,"F par %s\n",print_flags(flags));
  }
  fprintf(fp,"#0\n\n");

        /* objects */
        fprintf(fp,"#OBJECTS\n");

  for (vnum = pArea->min_vnum_obj; vnum <= pArea->max_vnum_obj; vnum++)
  {
      if ((obj = get_obj_index(vnum)) == NULL)
    continue;

      fprintf(fp,"#%d\n",obj->vnum);
      fprintf(fp,"%s~\n",obj->name);
      fprintf(fp,"%s~\n",obj->short_descr);
      fprintf(fp,"%s~\n",obj->description);
      if (obj->new_format)
        fprintf(fp,"%s~\n",obj->material);
      else
    fprintf(fp,"oldstyle~\n");
      fprintf(fp,"%s %s ",
    item_name(obj->item_type),
    print_flags(obj->extra_flags));
      fprintf(fp,"%s\n",print_flags(obj->wear_flags));
      switch (obj->item_type)
      {
      case ITEM_WEAPON:
    fprintf(fp,"%s %d %d %s %s\n",
        weapon_name(obj->value[0]),obj->value[1],obj->value[2],
        obj->value[3] > 0 ? attack_table[obj->value[3]].name 
               : "none", 
        print_flags(obj->value[4]));
    break;
            case ITEM_ARMOR:
                fprintf(fp,"%d %d %d %d %d\n",
                    obj->value[0], obj->value[1], obj->value[2],
                    obj->value[3], obj->value[4]);
                break;
      case ITEM_PORTAL:
    fprintf(fp,"%d %s ",
        obj->value[0],print_flags(obj->value[1]));
    fprintf(fp,"%s %d %d\n",
        print_flags(obj->value[2]),
        obj->value[3],obj->value[4]);
    break;
      case ITEM_FURNITURE:
    fprintf(fp,"%d %d %s %d %d\n",
        obj->value[0],obj->value[1],print_flags(obj->value[2]),
        obj->value[3],obj->value[4]);
    break;
      case ITEM_FOOD:
    fprintf(fp,"%d %d %d %d %d\n",
        obj->value[0], obj->value[1],obj->value[2],
        obj->value[3], obj->value[4]);
    break;
      case ITEM_CONTAINER:
    fprintf(fp,"%d %s %d %d %d\n",
        obj->value[0],print_flags(obj->value[1]),obj->value[2],
        obj->value[3], obj->value[4]);
    break;
      case ITEM_DRINK_CON:
      case ITEM_FOUNTAIN:
    fprintf(fp,"%d %d '%s' %d %d\n",
        obj->value[0],obj->value[1],
        obj->value[2] < 0 ? "water" : 
      liq_table[obj->value[2]].liq_name,
        obj->value[3],obj->value[4]);
    break;
      case ITEM_WAND:
      case ITEM_STAFF:
    fprintf(fp,"%d %d %d '%s' %d\n",
        obj->value[0],obj->value[1],obj->value[2],
        obj->value[3] > 0 ? skill_table[obj->value[3]].name : "",
        obj->value[4]);
    break;
      case ITEM_SPELL_PAGE:
    fprintf(fp,"%d %d '%s' %d %d\n",
	obj->value[0], obj->value[1],
	obj->value[2] > 0 ? skill_table[obj->value[2]].name : "",
	obj->value[3], obj->value[4] );
     break;
      case ITEM_POTION:
      case ITEM_SCROLL:
      case ITEM_PILL:
    fprintf(fp,"%d '%s' '%s' '%s' '%s'\n",
        obj->value[0],
        obj->value[1] > 0 ? skill_table[obj->value[1]].name : "",
                    obj->value[2] > 0 ? skill_table[obj->value[2]].name : "",
                    obj->value[3] > 0 ? skill_table[obj->value[3]].name : "",
                    obj->value[4] > 0 ? skill_table[obj->value[4]].name : "");
    break;
      default:  
        fprintf(fp,"%d %d %d %d %d\n",
        obj->value[0], obj->value[1], obj->value[2],
        obj->value[3], obj->value[4]);
    break;
      }
      fprintf(fp,"%d %d %d %s\n",
    obj->level, obj->weight, obj->cost,
    obj->condition >= 100 ? "P" :
    obj->condition >=  90 ? "G" :
    obj->condition >=  75 ? "A" :
    obj->condition >=  50 ? "W" :
    obj->condition >=  20 ? "D" :
    obj->condition >=  10 ? "B" : "R");

       for (af = obj->affected; af != NULL; af = af->next)
       {
    if (af->bitvector == 0)
        fprintf(fp,"A\n%d %d\n",af->location,af->modifier);
    else
        fprintf(fp,"F\n%s %d %d %s\n",
      af->where == TO_AFFECTS ? "A" :
      af->where == TO_IMMUNE  ? "I" :
      af->where == TO_RESIST  ? "R" :
      af->where == TO_VULN  ? "V" : "X",
      af->location,af->modifier,
      print_flags(af->bitvector));
       }

       for (ed = obj->extra_descr; ed != NULL; ed = ed->next)
       {
    fprintf(fp,"E\n%s~\n",ed->keyword);
    sprintf(buf,"%s~\n",ed->description);
    fdump(fp,buf);
       }
  }

  fprintf(fp,"#0\n\n");

  /* rooms */
  fprintf(fp,"#ROOMS\n");

  for (vnum = pArea->min_vnum_room; vnum <= pArea->max_vnum_room; vnum++)
  {
      if ((room = get_room_index(vnum)) == NULL)
    continue;

      fprintf(fp,"#%d\n",room->vnum);
      fprintf(fp,"%s~\n",room->name);
      sprintf(buf,"%s~\n",room->description);
      fdump(fp,buf);
      fprintf(fp,"0 %s %d\n",print_flags(room->room_flags),
        room->sector_type);
      
      if (room->heal_rate != 100 && room->mana_rate != 100)
    fprintf(fp,"H %d M %d\n",room->heal_rate,room->mana_rate);
      else if (room->heal_rate != 100)
    fprintf(fp,"H %d\n",room->heal_rate);
      else if (room->mana_rate != 100)
    fprintf(fp,"M %d\n",room->mana_rate);
      
      if(room->obs_target != 0)
        fprintf(fp, "B %d\n", room->obs_target);

      if (room->clan)
    fprintf(fp,"C %s~\n",clan_table[room->clan].name);

      if (room->owner != NULL && room->owner[0] != '\0')
    fprintf(fp,"O %s~\n",room->owner);

      for (door = 0; door < 6; door++)
      {
    if ((exit = room->exit[door]) != NULL)
    {
        int to_room;

        if (exit->u1.to_room == NULL)
      to_room = -1;
        else
      to_room = exit->u1.to_room->vnum;

	/* Trying something new.  Just save this entire vector */

/*
        if (IS_SET(exit->exit_info,EX_ISDOOR))
        {
      door_flag = 1;
      if (IS_SET(exit->exit_info,EX_PICKPROOF))
          door_flag++;
      if (IS_SET(exit->exit_info,EX_NOPASS))
          door_flag += 2;
        }
 */
        fprintf(fp,"D%d\n",door);
        sprintf(buf,"Exit ~\n");
        fdump(fp,buf);
        fprintf(fp,"%s~\n",exit->keyword);
        fprintf(fp,"%ld %d %d\n",(exit->exit_info|EX_NEW_FORMAT),exit->key,to_room);
    }
       }

       for (ed = room->extra_descr; ed != NULL; ed = ed->next)
       {
    fprintf(fp,"E\n");
    fprintf(fp,"%s~\n",ed->keyword);
    sprintf(buf,"%s~\n",ed->description);
    fdump(fp,buf);
       }
       fprintf(fp,"S\n");
  }
  fprintf(fp,"#0\n\n");

  /* resets */
  fprintf(fp,"#RESETS\n");
  
  for (reset = pArea->reset_first; reset != NULL; reset = reset->next)
  {
      switch(reset->command)
      {
      case 'M':
    mob = get_mob_index(reset->arg1);
    fprintf(fp,"M 0 %4d %3d %4d %2d\t* %s\n",
        reset->arg1,reset->arg2,reset->arg3,reset->arg4,
        mob->short_descr);
    break;
      case 'O':
                obj = get_obj_index(reset->arg1);
                fprintf(fp,"O 0 %4d %3d %4d\t* %s\n",
                    reset->arg1,reset->arg2,reset->arg3,obj->short_descr);                             
                break;
      case 'P':
                obj = get_obj_index(reset->arg1);
                fprintf(fp,"P 1 %4d %3d %4d %2d\t*   %s\n",
                    reset->arg1,reset->arg2,reset->arg3,reset->arg4,
        obj->short_descr);                             
                break;
      case 'E':
    obj = get_obj_index(reset->arg1);
    fprintf(fp,"E 1 %4d %3d %4d\t*   %s\n",
        reset->arg1,reset->arg2,reset->arg3,obj->short_descr);
    break;
      case 'G':
                obj = get_obj_index(reset->arg1);
                fprintf(fp,"G 1 %4d %3d     \t*   %s\n",
                    reset->arg1,reset->arg2,obj->short_descr);                             
                break;
      case 'D':
    room = get_room_index(reset->arg1);
	sprintf(log_buf,"Reset: %d %d %d",reset->arg1,reset->arg2,reset->arg3);
	log_string(log_buf);
    fprintf(fp,"D 0 %4d %3d %4d\t* %s %s\n",
        reset->arg1,reset->arg2,(reset->arg3|EX_NEW_FORMAT),
        room->name,dir_name[reset->arg2]);
    break;
      case 'R':
    room = get_room_index(reset->arg1);
    fprintf(fp,"R 0 %4d %3d     \t* %s\n",
        reset->arg1,reset->arg2,room->name);
    break;
  }
  }
  fprintf(fp,"S\n\n");

  /* shops */
  fprintf(fp,"#SHOPS\n");

  for (vnum = pArea->min_vnum_mob; vnum <= pArea->max_vnum_mob; vnum++)
  {
      if ((mob = get_mob_index(vnum)) == NULL
      ||  (shop = mob->pShop) == NULL)
    continue;

      fprintf(fp,"%4d %2d %2d %2d %2d %2d \t%4d %3d \t%2d %2d \t* %s\n",
    shop->keeper,shop->buy_type[0],shop->buy_type[1],
    shop->buy_type[2],shop->buy_type[3],shop->buy_type[4],
    shop->profit_buy,shop->profit_sell,
    shop->open_hour,shop->close_hour,
    mob->short_descr);
  }

  fprintf(fp,"0\n\n");

  /* specials */
  fprintf(fp,"#SPECIALS\n");

  for (vnum = pArea->min_vnum_mob; vnum <= pArea->max_vnum_mob; vnum++)
  {
      if ((mob = get_mob_index(vnum)) == NULL)
    continue;

      if (mob->spec_fun)
    fprintf(fp,"M %d %-20s\t* %s\n",
        mob->vnum,spec_name(mob->spec_fun),mob->short_descr);
  }
  
  fprintf(fp,"S\n\n");

  fprintf(fp,"#$\n");
  fclose(fp);
//  fpReserve = fopen(NULL_FILE, "r");      
}

void update_area_list ( CHAR_DATA *ch, char *strArea )
{
  FILE *fpList;
  AREA_NAME_DATA *area_name;
  
#ifdef OLC_VERSION
  area_name = alloc_perm (sizeof (AREA_NAME_DATA));
#else
  area_name = GC_MALLOC (sizeof (AREA_NAME_DATA));
#endif
  area_name->name = str_dup (strArea);
  if (!area_name_first) {
    area_name_first = area_name_last = area_name;
  } else {
    area_name_last->next = area_name;
    area_name->next = NULL;
    area_name_last  = area_name;
  }      

//  fclose (fpReserve);
  if ( ( fpList = fopen( AREA_LIST, "w" ) ) == NULL )
  {
      send_to_char ("Could not write to '",ch);
      send_to_char (AREA_LIST,ch);
      send_to_char ("'.\n\r",ch);
      return;
  }  
  area_name = area_name_first;
  while (area_name) {
    fprintf (fpList,"%s\n",area_name->name);
    area_name = area_name->next;  
  }    
  fprintf (fpList,"$\n");
  fclose (fpList);
//  fpReserve = fopen(NULL_FILE, "r");
}
