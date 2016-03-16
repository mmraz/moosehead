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
 
static char rcsid[] = "$Id: convert.c,v 1.3 2002/03/09 19:14:28 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
 #include <gc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "merc.h"


SPEC_FUN spec_thief;
SPEC_FUN spec_guard;
SPEC_FUN spec_cast_mage;
SPEC_FUN spec_cast_cleric;

typedef struct link_type LINK;

struct link_type
{
    LINK *next; 
    int level;
};

struct convert_type
{
    int level;
    int hp[3];
    int mana[3];
    int to_hit;
    int ac;
    int dam[2];
    int dam_bonus;
};

struct convert_type convert[61] =
{
/*
    { level, { hp dice }, { mana dice }, + to hit, ac,{ dam dice } }
*/
    {	0, {  2, 4,   2 }, {  1,1, 99 }, 0,  10, { 1, 2 }, 0 },

    {	1, {  2, 6,  10 }, {  1,9,100 }, 0,   9, {  1, 4 },  0 },
    {   2, {  2, 7,  21 }, {  2,9,100 }, 0,   8, {  1, 5 },  0 },
    {   3, {  2, 6,  35 }, {  3,9,100 }, 0,   7, {  1, 6 },  0 },
    {	4, {  2, 7,  46 }, {  4,9,100 }, 0,   6, {  1, 5 },  1 },
    {   5, {  2, 6,  60 }, {  5,9,100 }, 0,   5, {  1, 6 },  1 },
 
    {   6, {  2, 7,  71 }, {  6,9,100 }, 0,   4, {  1, 7 },  1 },
    {   7, {  2, 6,  85 }, {  7,9,100 }, 0,   4, {  1, 8 },  1 },
    {   8, {  2, 7,  96 }, {  8,9,100 }, 0,   3, {  1, 7 },  2 },
    {   9, {  2, 6, 110 }, {  9,9,100 }, 0,   2, {  1, 8 },  2 },
    {  10, {  2, 7, 121 }, { 10,9,100 }, 0,   1, {  2, 4 },  2 },

    {  11, {  2, 8, 134 }, { 11,9,100 }, 0,   1, {  1,10 },  2 }, 
    {  12, {  2,10, 150 }, { 12,9,100 }, 0,   0, {  1,10 },  3 },
    {  13, {  2,10, 170 }, { 13,9,100 }, 0,  -1, {  2, 5 },  3 },
    {  14, {  2,10, 190 }, { 14,9,100 }, 0,  -1, {  1,12 },  3 },
    {  15, {  3, 9, 208 }, { 15,9,100 }, 0,  -2, {  2, 6 },  3 },  

    {  16, {  3, 9, 233 }, { 16,9,100 }, 0,  -2, {  2, 6 },  4 },
    {  17, {  3, 9, 258 }, { 17,9,100 }, 0,  -3, {  3, 4 },  4 },
    {  18, {  3, 9, 283 }, { 18,9,100 }, 0,  -3, {  2, 7 },  4 },
    {  19, {  3, 9, 308 }, { 19,9,100 }, 0,  -4, {  2, 7 },  5 },
    {  20, {  3, 9, 333 }, { 20,9,100 }, 0,  -4, {  2, 8 },  5 },  

    {  21, {  4,10, 360 }, { 21,9,100 }, 0,  -5, {  4, 4 },  5 },
    {  22, {  5,10, 400 }, { 22,9,100 }, 0,  -5, {  4, 4 },  6 },
    {  23, {  5,10, 450 }, { 23,9,100 }, 0,  -6, {  3, 6 },  6 },
    {  24, {  5,10, 500 }, { 24,9,100 }, 0,  -6, {  2,10 },  6 },
    {  25, {  5,10, 550 }, { 25,9,100 }, 0,  -7, {  2,10 },  7 },

    {  26, {  5,10, 600 }, { 26,9,100 }, 0,  -7, {  3, 7 },  7 },
    {  27, {  5,10, 650 }, { 27,9,100 }, 0,  -8, {  5, 4 },  7 },
    {  28, {  6,12, 703 }, { 28,9,100 }, 0,  -8, {  5, 4 },  8 },
    {  29, {  6,12, 778 }, { 29,9,100 }, 0,  -9, {  2,12 },  8 },
    {  30, {  6,12, 853 }, { 30,9,100 }, 0,  -9, {  4, 6 },  8 },
   
    {  31, {  6,12, 928 }, { 31,9,100 }, 0, -10, {  4, 6 },  9 },
    {  32, { 10,10,1000 }, { 32,9,100 }, 0, -10, {  6, 4 },  9 },
    {  33, { 10,10,1100 }, { 33,9,100 }, 0, -11, {  6, 4 }, 10 }, 
    {  34, { 10,10,1200 }, { 34,9,100 }, 0, -11, {  4, 7 }, 10 },
    {  35, { 10,10,1300 }, { 35,9,100 }, 0, -11, {  4, 7 }, 11 },

    {  36, { 10,10,1400 }, { 36,9,100 }, 0, -12, {  3,10 }, 11 },
    {  37, { 10,10,1500 }, { 37,9,100 }, 0, -12, {  3,10 }, 12 },
    {  38, { 10,10,1600 }, { 38,9,100 }, 0, -13, {  5, 6 }, 12 },
    {  39, { 15,10,1700 }, { 39,9,100 }, 0, -13, {  5, 6 }, 13 },
    {  40, { 15,10,1850 }, { 40,9,100 }, 0, -13, {  4, 8 }, 13 },

    {  41, { 25,10,2000 }, { 41,9,100 }, 0, -14, {  4, 8 }, 14 },
    {  42, { 25,10,2250 }, { 42,9,100 }, 0, -14, {  3,12 }, 14 },
    {  43, { 25,10,2500 }, { 43,9,100 }, 0, -15, {  3,12 }, 15 },
    {  44, { 25,10,2750 }, { 44,9,100 }, 0, -15, {  8, 4 }, 15 },
    {  45, { 25,10,3000 }, { 45,9,100 }, 0, -15, {  8, 4 }, 16 },
   
    {  46, { 25,10,3250 }, { 46,9,100 }, 0, -16, {  6, 6 }, 16 },
    {  47, { 25,10,3500 }, { 47,9,100 }, 0, -17, {  6, 6 }, 17 },
    {  48, { 25,10,3750 }, { 48,9,100 }, 0, -18, {  6, 6 }, 18 },
    {  49, { 50,10,4000 }, { 49,9,100 }, 0, -19, {  4,10 }, 18 },
    {  50, { 50,10,4500 }, { 50,9,100 }, 0, -20, {  5, 8 }, 19 },
   
    {  51, { 50,10,5000 }, { 51,9,100 }, 0, -21, {  5, 8 }, 20 },
    {  52, { 50,10,5500 }, { 52,9,100 }, 0, -22, {  6, 7 }, 20 },
    {  53, { 50,10,6000 }, { 53,9,100 }, 0, -23, {  6, 7 }, 21 },
    {  54, { 50,10,6500 }, { 54,9,100 }, 0, -24, {  7, 6 }, 22 },
    {  55, { 50,10,7000 }, { 55,9,100 }, 0, -25, { 10, 4 }, 23 },
   
    {  56, { 50,10,7500 }, { 56,9,100 }, 0, -26, { 10, 4 }, 24 },
    {  57, { 50,10,8000 }, { 57,9,100 }, 0, -27, {  6, 8 }, 24 },
    {  58, { 50,10,8500 }, { 58,9,100 }, 0, -28, {  5,10 }, 25 },
    {  59, { 50,10,9000 }, { 59,9,100 }, 0, -29, {  8, 6 }, 26 },
    {  60, { 50,10,9500 }, { 60,9,100 }, 0, -30, {  8, 6 }, 28 }
};

void convert_mob(MOB_INDEX_DATA *mob)
{
    int class; /* 1 thief 2 mage 3 cleric 4 warrior */
    int level;

    if (mob->new_format)
	return;

    class = mob->spec_fun == spec_thief ? 1 : 
	    mob->spec_fun == spec_cast_mage ? 2 :
	    mob->spec_fun == spec_cast_cleric ? 3 : 
	    mob->spec_fun == spec_guard ? 4 : 0;

    level = mob->level;


    switch (class)
    {
    case 0:
	mob->hit[0]	= convert[level].hp[0];
	mob->hit[1]	= convert[level].hp[1];
	mob->hit[2]	= convert[level].hp[2];

	mob->mana[0]	= convert[level].mana[0] / 2;
	mob->mana[1]	= convert[level].mana[1];
	mob->mana[2]	= convert[level].mana[2];

	mob->hitroll	= convert[level].to_hit;
	mob->ac[0]	= convert[level].ac * 10;
	mob->ac[1]	= convert[level].ac * 10;
	mob->ac[2]	= convert[level].ac * 10;
	mob->ac[3]	= ((convert[level].ac - 10) / 4 + 10) * 10;
	
	mob->damage[0]	= convert[level].dam[0];
	mob->damage[1]	= convert[level].dam[1];
	mob->damage[2]	= convert[level].dam_bonus;
	break;
    case 1: /* thief */
        mob->hit[0]     = convert[UMAX(0,level - 1)].hp[0];
        mob->hit[1]     = convert[UMAX(0,level -1 )].hp[1];
        mob->hit[2]     = convert[UMAX(0,level - 1)].hp[2];
 
        mob->mana[0]    = convert[level].mana[0] / 2;
        mob->mana[1]    = convert[level].mana[1];
        mob->mana[2]    = convert[level].mana[2];
 
        mob->hitroll    = convert[level].to_hit;
        mob->ac[0]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[1]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[2]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[3]      = ((convert[UMAX(0,level - 1)].ac - 10) / 3 + 10) * 10;
 
        mob->damage[0]  = convert[UMAX(0,level - 1)].dam[0];
        mob->damage[1]  = convert[UMAX(0,level - 1)].dam[1];
        mob->damage[2]  = convert[UMAX(0,level - 1)].dam_bonus;

	SET_BIT(mob->act,ACT_THIEF);
	mob->off_flags = mob->off_flags|OFF_KICK_DIRT|OFF_DISARM|OFF_BACKSTAB;
	mob->affected_by = mob->affected_by|AFF_DETECT_INVIS|AFF_DETECT_HIDDEN;
        break;

    case 2: /* mage */
        mob->hit[0]     = convert[UMAX(0,level - 1)].hp[0];
        mob->hit[1]     = convert[UMAX(0,level - 1)].hp[1];
        mob->hit[2]     = convert[UMAX(0,level - 1)].hp[2];
 
        mob->mana[0]    = convert[level].mana[0];
        mob->mana[1]    = convert[level].mana[1];
        mob->mana[2]    = convert[level].mana[2];
 
        mob->hitroll    = convert[UMAX(0,level - 2)].to_hit;
        mob->ac[0]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[1]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[2]      = convert[UMAX(0,level - 1)].ac * 10;
        mob->ac[3]      = ((convert[UMAX(0,level - 1)].ac - 10) / 2 + 10) * 10;
 
        mob->damage[0]  = convert[UMAX(0,level - 3)].dam[0];
        mob->damage[1]  = convert[UMAX(0,level - 3)].dam[1];
        mob->damage[2]  = convert[UMAX(0,level - 3)].dam_bonus;

	SET_BIT(mob->act,ACT_MAGE);
	REMOVE_BIT(mob->off_flags,OFF_TRIP);
	REMOVE_BIT(mob->off_flags,OFF_DISARM);
        break;

    case 3: /* cleric */
        mob->hit[0]     = convert[level].hp[0];
        mob->hit[1]     = convert[level].hp[1];
        mob->hit[2]     = convert[level].hp[2];
 
        mob->mana[0]    = convert[level].mana[0];
        mob->mana[1]    = convert[level].mana[1];
        mob->mana[2]    = convert[level].mana[2];
 
        mob->hitroll    = convert[UMAX(0,level - 1)].to_hit;
        mob->ac[0]      = convert[level].ac * 10;
        mob->ac[1]      = convert[level].ac * 10;
        mob->ac[2]      = convert[level].ac * 10;
        mob->ac[3]      = ((convert[level].ac - 10) / 3 + 10) * 10;
 
        mob->damage[0]  = convert[UMAX(0,level - 2)].dam[0];
        mob->damage[1]  = convert[UMAX(0,level - 2)].dam[1];
        mob->damage[2]  = convert[level].dam_bonus;

	SET_BIT(mob->act,ACT_CLERIC);
	REMOVE_BIT(mob->off_flags,OFF_DODGE);
	REMOVE_BIT(mob->off_flags,OFF_TRIP);
	mob->off_flags = mob->off_flags|OFF_KICK|OFF_PARRY;
        break;

    case 4: /* guard */
        mob->hit[0]     = convert[UMIN(60,level + 1)].hp[0];
        mob->hit[1]     = convert[UMIN(60,level + 1)].hp[1];
        mob->hit[2]     = convert[UMIN(60,level + 1)].hp[2];
 
        mob->mana[0]    = convert[level].mana[0] / 2;
        mob->mana[1]    = convert[level].mana[1];
        mob->mana[2]    = convert[level].mana[2];
 
        mob->hitroll    = convert[UMIN(60,level + 1)].to_hit;
        mob->ac[0]      = convert[level].ac * 10;
        mob->ac[1]      = convert[level].ac * 10;
        mob->ac[2]      = convert[level].ac * 10;
        mob->ac[3]      = ((convert[level].ac - 10) / 4 + 10) * 10;
 
        mob->damage[0]  = convert[level].dam[0];
        mob->damage[1]  = convert[level].dam[1];
        mob->damage[2]  = convert[level].dam_bonus;

	SET_BIT(mob->act,ACT_WARRIOR);
	REMOVE_BIT(mob->off_flags,OFF_DODGE);
	REMOVE_BIT(mob->off_flags,OFF_TRIP);
	mob->off_flags = mob->off_flags|OFF_BASH|OFF_BERSERK|OFF_PARRY|
				        OFF_KICK|OFF_RESCUE;
        break;
    }
}

int merge_levels (int current, int new)
{
    int level,old;

    level = UMAX(current,new);
    old   = UMIN(current,new);

    if (old <= 1)
	return level;

    if (level - old <= 5)
	level += 2;

    else if (level - old <= 10)
	level += 1;

    return level;
}

LINK *add_level (LINK *list,int level)
{
    LINK *new,*link,*last = NULL;

    for (link = list; link != NULL; link = link->next)
    {
        if (level > link->level)
    	    break;
	last = link;
    }

    new 		= GC_MALLOC(sizeof(*link));
    new->level 		= level;

    if (last == NULL) /* head of list */
    {
	new->next	= list;
	list		= new;
    }
    else
    {
	new->next	= last->next;
	last->next	= new;
    }

    return list;
}

void free_list (LINK *list)
{
    LINK *link_next;

    for ( ; list != NULL; list = link_next)
    {
	link_next = list->next;
	free_mem(list,sizeof(*list));
     }
}


int compute_level (LINK *list)
{
    LINK *link, *temp = NULL;
    int level = 0;

    /* find the highest element */
    for (link = list; link != NULL; link = link->next)
    {
	if (temp == NULL || link->level > temp->level)
	    temp = link;
	level = UMAX(level,link->level);
    }

    for (link = list; link != NULL; link = link->next)
	if (temp != link)
	    level = merge_levels(level,link->level);

    return level;
}



	


void convert_obj(OBJ_INDEX_DATA *obj)
{
    int minuses, level, val, dam, adds, lowest, cost;
    AFFECT_DATA *af;
    LINK *list = NULL, *list2 = NULL;

/*
    if (obj->new_format)
	return;
*/

    level = 0;
    cost  = 0;
    minuses = 0;

    switch(obj->item_type)
    {
    case ITEM_WEAPON:
	dam = obj->value[1] + (obj->value[1] * obj->value[2]);
	switch (dam)
	{
	default:	level =  0; 	break;
	case  7:	level =  1;	break;
	case  8:	level =  3;	break;
	case  9:	level =  4;	break;
	case 10:	level =  5;	break;
	case 11:	level =  6;	break;
	case 12:	level =  7;	break;
	case 13:	level =  8;	break;
	case 14:	level = 10;	break;
	case 15:	level = 11;	break;
	case 16:	level = 12;	break;
	case 17:	level = 14;	break;
	case 18:	level = 15;	break;
	case 19:	level = 16;	break;
	case 20:	level = 17;	break;
	case 21:	level = 18;	break;
	case 22:	level = 19;	break;
	case 23:	level = 20;	break;
	case 24:	level = 21;	break;
	case 25:	level = 23;	break;
	case 26:	level = 24;	break;
	case 27:	level = 25;	break;
	case 28:	level = 26;	break;
	case 29:	level = 27;	break;
	case 30:	level = 28;	break;
	case 31:	level = 29;	break;
	case 32:	level = 30;	break;
	case 33:	level = 31;	break;
	case 34:	level = 32;	break;
	case 35:	level = 34;	break;
	case 36:	level = 35;	break;
	case 37:	level = 36;	break;
	case 38:	level = 37;	break;
	case 39:	level = 38;	break;
	case 40:	level = 39;	break;
	case 41:	level = 40;	break;
	case 42:	level = 41;	break;
	case 43:	level = 43;	break;
	case 44:	level = 44;	break;
	case 45:	level = 45;	break;
	}

	if (dam > 45)
	    level = dam;

	if (obj->value[0] == WEAPON_POLEARM)
	    SET_BIT(obj->value[4],WEAPON_TWO_HANDS);

	if (IS_SET(obj->value[4],WEAPON_TWO_HANDS))
	    level -= 2;

	if (IS_SET(obj->value[4],WEAPON_VAMPIRIC))
	    level += 2;

	if (IS_SET(obj->value[4],WEAPON_SHARP))
	    level += 2;

	if (IS_SET(obj->value[4],WEAPON_SHOCKING))
	    level += 2;

	if (IS_SET(obj->value[4],WEAPON_FROST))
	    level += 2;

	if (IS_SET(obj->value[4],WEAPON_FLAMING))
	    level += 2;

	if ( dam == 0)
	    fprintf(stderr,"Error: weapon %s (vnum %d) has damage %dd%d.\n",
		obj->short_descr, obj->vnum, obj->value[1], obj->value[2]);

	cost = 40 * dam +  2 * dam * dam / 5;

 	switch (obj->value[0])
	{
	case WEAPON_EXOTIC:	cost = 3 * cost / 2;	break;
	case WEAPON_SWORD:				break; 
	case WEAPON_DAGGER:	cost = 3 * cost / 4;	break;
	case WEAPON_SPEAR:	cost =     cost / 5;	break;
	case WEAPON_MACE:	cost = 2 * cost / 3;	break;
	case WEAPON_FLAIL:	cost = 4 * cost / 5;	break;
	case WEAPON_WHIP:	cost =     cost / 2;	break;
	case WEAPON_POLEARM:	cost =     cost / 3;	break;
	}
	if (IS_SET(obj->value[4], WEAPON_TWO_HANDS))
	    cost = cost * 11/10;
	break;

    case ITEM_ARMOR:
	switch((obj->value[0] + obj->value[1] + obj->value[2] + 1) / 3)
	{
	default:	level =  0;	break;
	case  3:	level =  3;	break;
	case  4:	level =  5;	break;
	case  5:	level = 10;	break;
	case  6:	level = 15;	break;
	case  7:	level = 20;	break;
	case  8:	level = 25;	break;
	case  9:	level = 30;	break;
	case 10:	level = 35;	break;
	case 11:	level = 40;	break;
	case 12:	level = 45;	break;
	}
	
	if (obj->value[0] > 12 || obj->value[1] > 12
	||  obj->value[2] > 12 
	||  (obj->value[0] <= 0 && obj->value[1] <= 0 && obj->value[2] <= 0))
	    fprintf(stderr,"Error: armor %s (vnum %d) has ac %d.\n",
		obj->short_descr, obj->vnum, obj->value[0]);

	cost = 20 * (obj->value[0] + obj->value[1] + 
		     obj->value[2] + obj->value[3]);
	cost += (obj->value[0] * obj->value[0] +
		 obj->value[1] * obj->value[1] +
		 obj->value[2] * obj->value[2] +
		 obj->value[3] * obj->value[3]) * 4;
	if (CAN_WEAR(obj,ITEM_WEAR_SHIELD))
	   cost = 3 * cost / 2; 
	if (CAN_WEAR(obj,ITEM_WEAR_BODY))
	   cost *= 2;

	break;
    
    case ITEM_POTION:
    case ITEM_SCROLL:
    case ITEM_PILL:
	adds = 0;

	cost = 100 + 10 * obj->value[0];

	for (val = 1; val < 4; val++)
	{
	    lowest = 53;

	    if (obj->value[val] < 1)
		continue;

            if (obj->item_type != ITEM_SCROLL
	    &&  (skill_table[obj->value[val]].target == TAR_CHAR_OFFENSIVE
	    ||   skill_table[obj->value[val]].target == TAR_OBJ_CHAR_OFF))
                adds -= 2;

	    lowest = UMIN(skill_table[obj->value[val]].skill_level[0],
			  skill_table[obj->value[val]].skill_level[1]);

	    lowest = UMAX(0,((lowest * 3)/4) - 4);

	    if (obj->value[val] == skill_lookup("change sex"))
		lowest = 0;

	    cost += lowest * lowest * 2 + lowest * 10;

       	    list2 = add_level(list2,lowest);
	}

	if (obj->value[1] < 1 && obj->value[2] < 1 && obj->value[3] < 1 )
	    fprintf(stderr,"No spell on object %s (vnum %d)\n",
		obj->short_descr,obj->vnum);

	level = compute_level(list2);
  	free_list(list2);

	level += adds; 

	if (obj->item_type == ITEM_SCROLL)
	    cost *= 2;
	break;

    case ITEM_WAND:
    case ITEM_STAFF:

	if (obj->value[3] < 1)
	{
	    fprintf(stderr,"Error: wand %s (vnum %d) has no spell!\n",
		obj->short_descr,obj->vnum);
	    break;
	}

	level = UMIN(skill_table[obj->value[3]].skill_level[0],
		     skill_table[obj->value[3]].skill_level[1]);

	cost = obj->value[0] * 25 + (level * level * 4) * 
	       UMAX(1,obj->value[2]/3) + level * 25 + 50 * obj->value[2] / 3;
	if (obj->item_type == ITEM_STAFF)
	    cost = cost * 3/2;

	level = (level * 3) /4 + obj->value[2] / 5;

	if (obj->value[3] == skill_lookup("high explosive"))
	    level = 25 + obj->value[2] / 5;

	if (obj->value[3] == skill_lookup("general purpose"))
	    level = 15 + obj->value[2] / 5;

	if (obj->value[2] == 1)
	    level -= 2;
	break;
    }

    /* we have a level, now go through and add in affects */
    for (af = obj->affected; af != NULL; af = af->next)
    {
	switch(af->location)
	{
	case APPLY_STR:
	case APPLY_DEX:
	case APPLY_CON:
	case APPLY_WIS:
	case APPLY_INT:
	case APPLY_AGT:
	case APPLY_END:
	case APPLY_SOC:
	     if (af->modifier < 0)
	     {
		minuses +=  2 * af->modifier;
		cost -= af->modifier * af->modifier * 250;
	     }
	     else
	     {
		switch (af->modifier)
	  	{
		case 1: list = add_level(list,5);	break;
		case 2:	list = add_level(list,15);	break;
		case 3: list = add_level(list,30);	break;
		case 4: list = add_level(list,45);	break;
	
		default:
		    fprintf(stderr,
			"Error: stat effect %d on obj %s, vnum %d\n",
			af->modifier, obj->short_descr,obj->vnum);
		}
	        cost += (af->modifier * af->modifier * 500);
	    }
	    break;

	case APPLY_HIT:
	case APPLY_MANA:
	case APPLY_MOVE:
	    if (af->modifier < 0)
	    {
		minuses += af->modifier/5;
		cost -= UMAX(5 * af->modifier,af->modifier * af->modifier);
	    }

	    else
	    {
		int mod;

		mod = af->modifier;
		
		if (mod > 75)
		    fprintf(stderr,
			"Error: points effect %d on obj %s, vnum %d\n",
			af->modifier, obj->short_descr,obj->vnum);
		else if (mod > 50)
		    list = add_level(list,45);
		else if (mod > 40)
		    list = add_level(list,40);
		else if (mod > 30)
		    list = add_level(list,35);
		else if (mod > 25)
		    list = add_level(list,30);
		else if (mod > 20)
		    list = add_level(list,25);
		else if (mod > 15)
		    list = add_level(list,20);
		else if (mod > 10)
		    list = add_level(list,10);
		else
		    list = add_level(list,5);

		cost += UMAX(10 * af->modifier,af->modifier * af->modifier);
	    }
	    break;

	case APPLY_HITROLL:
	case APPLY_DAMROLL:
	    if (obj->item_type == ITEM_WEAPON)
	    {
		if (af->location == APPLY_HITROLL)
		    level += af->modifier / 2;
		else
		    level += af->modifier;
		if (af->modifier > 0)
		    cost += af->modifier * af->modifier * 50;
		else
		    cost -= af->modifier * af->modifier * 25;
	    }
	    else
	    {
		if (af->modifier < 0)
		    minuses += af->modifier;
		else switch (af->modifier)
		{
		case 1:	list = add_level(list,5);	break;
		case 2: list = add_level(list,20);	break;
		case 3: list = add_level(list,35);	break;
		default: fprintf(stderr,
		    "Error: hit/dam effect %d on obj %s, vnum %d\n",
		    af->modifier,obj->short_descr,obj->vnum);
		}
		if (af->modifier > 0)
		    cost += af->modifier * af->modifier * 200;
		else
		    cost -= af->modifier * af->modifier * 100;
	     }
	     break;

	case APPLY_SAVING_PARA:
	case APPLY_SAVING_BREATH:
	case APPLY_SAVING_ROD:
	case APPLY_SAVING_PETRI:
	case APPLY_SAVING_SPELL:
	     af->location = APPLY_SAVES;

	     if (af->modifier > 0)
		minuses -= af->modifier * 2;
	     else switch (af->modifier)
	     {
	     case -1:					break;
	     case -2:	list = add_level(list,20);	break;
	     case -3:	list = add_level(list,35); break;
	     case -4:	list = add_level(list,45); break;
	     default:	fprintf(stderr,
		"Error: save effect %d on obj %s, vnum %d\n",
		af->modifier,obj->short_descr,obj->vnum);
	     }
	     if (af->modifier < 0)
		cost += af->modifier * af->modifier * 150;
	     else
		cost -= af->modifier * af->modifier * 75;
	     break;

	case APPLY_AC:
	    if (af->modifier > 0)
		minuses -= af->modifier * 2;
	     else 
		list = add_level(list,-4 * af->modifier);
	    cost -= af->modifier * 200;
	    break;

	default:
	    if (af->bitvector == 0)
	    	fprintf(stderr,
		    "Affect %d (%d) on obj %s, vnum %d has no effect.\n",
		    af->location,af->modifier,obj->short_descr,obj->vnum);
	    break;
	}
    }

    if (level > 50)
    	fprintf(stderr,"Obj %s (vnum %d) has level %d > 50\n",
	    obj->short_descr,obj->vnum,level);


    list = add_level(list,level);
    level = compute_level(list);
    level += minuses;
    free_list(list);
    level = UMAX(0,level);

    cost = cost * ((100 + level) * (100 + level)) / 10000;
    cost = UMAX(0,cost);
 
    if (cost > 2000)
	cost = ((cost + 50)/100) * 100;
     else if (cost > 200)
	cost = ((cost +5)/10) * 10;

    /* do the corrections */
    if (!obj->new_format)
    {
	obj->level = level;
	obj->cost = cost;
    }
    else 
    {
	if (level != obj->level )
	    fprintf(stderr,"Level mismatch: obj %s (%d) level %d computed %d\n",
		obj->short_descr,obj->vnum,obj->level,level);
	if ( cost > 0 && (cost > (obj->cost * 11 / 10) 
	||                cost < (obj->cost *  9 / 10)))
	    fprintf(stderr,"Cost mismatch: obj %s (%d) cost %d computed %d\n",
	    	obj->short_descr,obj->vnum,obj->cost,cost);
/*
	if (cost > 0)
	    obj->cost  = cost;
*/
    }

    if (!CAN_WEAR(obj,ITEM_TAKE))
	obj->weight = 0;
}
