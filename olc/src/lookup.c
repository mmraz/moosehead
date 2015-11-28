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
 
static char rcsid[] = "$Id: lookup.c,v 1.3 2000/02/12 14:33:04 mud Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "tables.h"
#include "lookup.h"

int flag_lookup (const char *name, const struct flag_type *flag_table)
{
    int flag;

    for (flag = 0; flag_table[flag].name != NULL; flag++)
    {
	if (LOWER(name[0]) == LOWER(flag_table[flag].name[0])
	&&  !str_prefix(name,flag_table[flag].name))
	    return flag_table[flag].bit;
    }

    return 0;
}

int nonclan_lookup(const char *name)
{
    int clan;

    for (clan = 0; clan < MAX_CLAN; clan++)
    {
	if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
	&&  !str_prefix(name,clan_table[clan].name))
	    return clan;
    }

    return 0;
}


CLAN_DATA *clan_lookup(const char *name)
{
  CLAN_DATA *temp = clan_first;
  while(temp)
  {
    if(!str_cmp(temp->name, name))
      return temp;
    temp = temp->next;
  }
  return NULL;
}

int gift_lookup(const char *name)
{
    int gift;

    for (gift = 0; gift < MAX_GIFTS; gift++)
    {
	if (LOWER(name[0]) == LOWER(gift_table[gift].name[0])
	&& !str_prefix(name,gift_table[gift].name))
	   return gift;
    }

    return MAX_GIFTS;
}

int deity_lookup(const char *name)
{
    int deity;

    for (deity = 0; deity < MAX_DEITY; deity++)
    {
	if (LOWER(name[0]) == LOWER(deity_table[deity].name[0])
	&& !str_prefix(name,deity_table[deity].name))
	   return deity;
    }

    return -1;
}

int position_lookup (const char *name)
{
   int pos;

   for (pos = 0; position_table[pos].name != NULL; pos++)
   {
	if (LOWER(name[0]) == LOWER(position_table[pos].name[0])
	&&  !str_prefix(name,position_table[pos].name))
	    return pos;
   }
   
   return -1;
}

int sex_lookup (const char *name)
{
   int sex;
   
   for (sex = 0; sex_table[sex].name != NULL; sex++)
   {
	if (LOWER(name[0]) == LOWER(sex_table[sex].name[0])
	&&  !str_prefix(name,sex_table[sex].name))
	    return sex;
   }

   return -1;
}

int immc_lookup (const char *name)
{
   int immc;
   
   for (immc = 0; immc < MAX_IMM_COMM; immc++)
   {
	if (LOWER(name[0]) == LOWER(imm_command_table[immc].name[0])
	&&  !str_prefix(name,imm_command_table[immc].name))
	    return immc;
   }

   return -1;
}

int size_lookup (const char *name)
{
   int size;
 
   for ( size = 0; size_table[size].name != NULL; size++)
   {
        if (LOWER(name[0]) == LOWER(size_table[size].name[0])
        &&  !str_prefix( name,size_table[size].name))
            return size;
   }
 
   return -1;
}
