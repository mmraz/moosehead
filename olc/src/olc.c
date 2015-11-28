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

static char rcsid[] = "$Id: olc.c,v 1.48 2003/09/26 04:07:13 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "merc.h"
#include "recycle.h"
#include "db.h"
#include "ctype.h"
#include "lookup.h"
#include "tables.h"
#include "gc.h"
#ifdef GAME_VERSION
#define str_dup_perm str_dup
#define alloc_mem GC_MALLOC
#define alloc_perm GC_MALLOC
#endif

/* What may appear to be memory leaks are not necessarily leaks, but safeguards.
   MOBs/Objects share the same strings as their indexes, so freeing that permanent
   string may have drastic results.. hence, the string is just left in memory.
   - kris   */
   
/* This bothers me on a fundamental level, but since memory is cheap and
   and plentiful and it would be a massive amount of work to clean it all up I
   am just going to leave it.
   - Andaron */

#define RANGE_ROOM             1
#define RANGE_MOB              2
#define RANGE_OBJ              4
#define RANGE_AREA             8

// New help code
extern int top_help;
void format_help_to_char(CHAR_DATA *ch, HELP_DATA *pHelp);
void edit_help_file (CHAR_DATA *ch, char *buf);

void do_bflag ( CHAR_DATA *ch, char *argument )
{
  VNUM_RANGE_DATA *range,*prev_range;
  char arg1[MAX_STRING_LENGTH];
  CHAR_DATA *victim;
  char buf[MAX_STRING_LENGTH];

  argument = one_argument (argument,arg1);
  if (arg1[0]) {
  if (!str_prefix (arg1,"stat")) {
    int t;

    argument = one_argument (argument,arg1);
    victim = get_char_world (ch,arg1);

    if (!victim) {
      send_to_char ("Syntax:  bflag stat <char_name>\n\r",ch);
      return;
    }

    if (!victim->pcdata) {
      send_to_char ("BFlag can only be used on PCs.\n\r",ch);
      return;
    }
    range = victim->pcdata->edit.range;
    if (range == NULL) {
      send_to_char ("No olc ranges set.\n\r",ch);
      return;
    }
    send_to_char ("OLC ranges:\n\r-----------------------\n\r",ch);
    t = 0;
    while (range) {
      sprintf (buf," %2d:  %d-%d\n\r",++t,range->min,range->max);
      send_to_char (buf,ch);
      range = range->next;
    }
    return;
  }
   if (!str_prefix (arg1,"add")) {
    int min_vnum,max_vnum;
    argument = one_argument (argument,arg1);
    victim = get_char_world (ch,arg1);

    if (!victim) {
  send_to_char ("Syntax: bflag add <char_name> <min_vnum> <max_vnum>\n\r",ch);
  return;
    }
    if (!victim->pcdata) {
  send_to_char ("BFlag can only be used on PCs.\n\r",ch);
  return;
    }
    argument = one_argument (argument,arg1);
    if (!is_number (arg1)) {
  send_to_char ("Syntax: bflag add <char_name> <min_vnum> <max_vnum>\n\r",ch);
  return;
    }
    min_vnum = atoi (arg1);
    argument = one_argument (argument,arg1);
    if (!is_number (arg1)) {
  send_to_char ("Syntax: bflag add <char_name> <min_vnum> <max_vnum>\n\r",ch);
  return;
    }
    max_vnum = atoi (arg1);
    if (min_vnum > max_vnum) {
      int temp;
      temp = min_vnum;
      min_vnum = max_vnum;
      max_vnum = temp;
    }
    range = new_range ();
    range->min = min_vnum;
    range->max = max_vnum;
    range->next = victim->pcdata->edit.range;
    victim->pcdata->edit.range = range;
    sprintf (buf,"Range %d-%d added to '%s'.\n\r",
      min_vnum,max_vnum,victim->name);
    send_to_char (buf,ch);
    return;
  }
  if (!str_prefix (arg1,"remove")) {
    int t,remove;

    argument = one_argument (argument,arg1);
    victim = get_char_world (ch,arg1);

    if (!victim) {
      send_to_char ("Syntax: bflag remove <char_name> <stat_number>\n\r",ch);
      return;
    }

    if (!victim->pcdata) {
      send_to_char ("BFlag can only be used on PCs.\n\r",ch);
      return;
    }
    range = victim->pcdata->edit.range;
    if (range == NULL) {
      send_to_char ("No olc ranges set.\n\r",ch);
      return;
    }
    argument = one_argument (argument,arg1);
    if (!is_number (arg1)) {
      send_to_char ("Syntax: bflag remove <char_name> <stat_number>\n\r",ch);
      return;
    }
    remove = atoi (arg1);
    t = 0;
    prev_range = NULL;
    while (range) {
      if (++t == remove) {
        if (prev_range) {
          prev_range->next = range->next;
        } else {
          victim->pcdata->edit.range = range->next;
        }
        sprintf (buf,"Range %d-%d removed from %s.\n\r",range->min,range->max,
          victim->name);
        send_to_char (buf,ch);
        free_range (range);
        return;
      }
      prev_range = range;
      range = range->next;
    }
    sprintf (buf,"Range not found for %s.\n\r",victim->name);
    send_to_char (buf,ch);
    return;
  }
  }

  send_to_char ("Syntax for bflag:\n\r",ch);
  send_to_char ("  bflag add <char_name> <min_vnum> <max_vnum>\n\r",ch);
  send_to_char ("  bflag stat <char_name>\n\r",ch);
  send_to_char ("  bflag remove <char_name> <stat_number>\n\r",ch);
}

#define ID_MENU_INIT           0

#define ID_EDIT_SETTINGS       1
#define ID_EDIT_AREA           2
#define ID_EDIT_ROOM           3
#define ID_EDIT_MOB            4
#define ID_EDIT_OBJECT         5
#define ID_EDIT_RESETS         6

#define ID_SETTINGS_DEF_ROOM   1
#define ID_SETTINGS_DEF_OBJ    2
#define ID_SETTINGS_DEF_MOB    3
#define ID_SETTINGS_DOOR       4
#define ID_SETTINGS_AUTO       5
#define ID_SETTINGS_BRIEF      6
#define ID_SETTINGS_HELP       7

#define ID_EDIT_AREA_SELECT    1
#define ID_EDIT_AREA_INFO      2
#define ID_EDIT_AREA_NEW       3
#define ID_EDIT_AREA_LOAD      4
#define ID_EDIT_AREA_SAVE      5
#define ID_EDIT_AREA_RENAME    6
#define ID_EDIT_AREA_RESET     7
#define ID_EDIT_AREA_PURGE     8
#define ID_EDIT_AREA_FREEZE    9
#define ID_EDIT_AREA_UNDER_DEV 10

#define ID_EDIT_VNUM           1
#define ID_EDIT_LIST           2
#define ID_EDIT_CREATE         3
#define ID_EDIT_CLONE          4
#define ID_EDIT_MODIFY         5
#define ID_EDIT_INFO           6
#define ID_EDIT_SHOP           7
#define ID_EDIT_INSTANCE       8

#define ID_EDIT_RESET_ROOM     1
#define ID_EDIT_RESET_MOB      2
#define ID_EDIT_RESET_OBJECT   3
#define ID_EDIT_RESET_DOOR     4
#define ID_EDIT_RESET_ALL      5

#define ID_ROOM_NAME           1
#define ID_ROOM_DESC           2
#define ID_ROOM_EXTRA          3
#define ID_ROOM_FLAGS          4
#define ID_ROOM_DOOR           5
#define ID_ROOM_TERRAIN        6
#define ID_ROOM_OWNER          7
#define ID_ROOM_CLAN           8
#define ID_ROOM_EXTENDED       9
#define ID_ROOM_REM_EXTEND     10
#define ID_ROOM_HEAL           11
#define ID_ROOM_MANA           12
#define ID_ROOM_OBS            13

#define ID_DOOR_SELECT         1
#define ID_DOOR_INFO           2
#define ID_DOOR_VNUM           3
#define ID_DOOR_KEYWORD        4
#define ID_DOOR_KEY            6
#define ID_DOOR_FLAGS          7
#define ID_DOOR_COPY           8
#define ID_DOOR_REMOVE         9

#define ID_MOB_NAME            1
#define ID_MOB_SHORT           2
#define ID_MOB_LONG            3
#define ID_MOB_DESC            4
#define ID_MOB_LEVEL           5
#define ID_MOB_HP              6
#define ID_MOB_MANA            7
#define ID_MOB_AC              8
#define ID_MOB_DAMTYPE         9
#define ID_MOB_DAMAGE         10
#define ID_MOB_HITROLL        11
#define ID_MOB_ACT            12
#define ID_MOB_OFF            13
#define ID_MOB_IMM            14
#define ID_MOB_RES            15
#define ID_MOB_VULN           16
#define ID_MOB_PARTS          17
#define ID_MOB_FORM           18
#define ID_MOB_RACE           19
#define ID_MOB_SEX            20
#define ID_MOB_POS            21
#define ID_MOB_WEALTH         22
#define ID_MOB_SIZE           23
#define ID_MOB_ALIGN          24
#define ID_MOB_AFF            25
#define ID_MOB_SPEC           26
#define ID_MOB_SPEC_WORDS     27

#define ID_SPEC_NONE         200

#define ID_SHOP_CLONE          1
#define ID_SHOP_BUY            2
#define ID_SHOP_PROFIT         3
#define ID_SHOP_HOURS          4
#define ID_SHOP_INFO           5
#define ID_SHOP_REMOVE         6

#define ID_OBJ_NAME            1
#define ID_OBJ_LEVEL           2
#define ID_OBJ_SHORT           3
#define ID_OBJ_DESCR           4
#define ID_OBJ_MATERIAL        5
#define ID_OBJ_TYPE            6
#define ID_OBJ_FLAGS           7
#define ID_OBJ_WEAR            8
#define ID_OBJ_COND            9
#define ID_OBJ_WEIGHT         10
#define ID_OBJ_COST           11
#define ID_OBJ_VALUE          12
#define ID_OBJ_EXTENDED       13
#define ID_OBJ_REM_EXTEND     14
#define ID_OBJ_AFF_ADD        15
#define ID_OBJ_AFF_REMOVE     16
#define ID_OBJ_WEAR_TIMER     17


#define ID_EDIT_GOTO_MAIN    100
#define ID_EDIT_EXIT         101
#define ID_EDIT_PREVIOUS     102
#define ID_EDIT_DONE         103
#define ID_EDIT_CANCEL       103

void edit_flags_init    ( CHAR_DATA *ch, int num );
void edit_flags         ( CHAR_DATA *ch, int num );
void edit_main          ( CHAR_DATA *ch, int num );
void edit_settings      ( CHAR_DATA *ch, int num );
void edit_area_init     ( CHAR_DATA *ch, int num );
void edit_area          ( CHAR_DATA *ch, int num );
void edit_room_init     ( CHAR_DATA *ch, int num );
void edit_room          ( CHAR_DATA *ch, int num );
void edit_room_modify   ( CHAR_DATA *ch, int num );
void edit_sector_init   ( CHAR_DATA *ch, int num );
void edit_sector        ( CHAR_DATA *ch, int num );
void edit_exit_init     ( CHAR_DATA *ch, int num );
void edit_room_exits    ( CHAR_DATA *ch, int num );
void edit_mob_init      ( CHAR_DATA *ch, int num );
void edit_mob           ( CHAR_DATA *ch, int num );
void edit_mob_modify    ( CHAR_DATA *ch, int num );
void edit_mob_size_init ( CHAR_DATA *ch, int num );
void edit_mob_size      ( CHAR_DATA *ch, int num );
void edit_mob_race_init ( CHAR_DATA *ch, int num );
void edit_mob_race      ( CHAR_DATA *ch, int num );
void edit_mob_spec_init ( CHAR_DATA *ch, int num );
void edit_mob_spec      ( CHAR_DATA *ch, int num );
void edit_mob_shop      ( CHAR_DATA *ch, int num );
void edit_mob_pos_init  ( CHAR_DATA *ch, int num );
void edit_mob_position  ( CHAR_DATA *ch, int num );
void edit_object_init   ( CHAR_DATA *ch, int num );
void edit_object        ( CHAR_DATA *ch, int num );
void edit_goto_main     ( CHAR_DATA *ch, int num );
void edit_exit          ( CHAR_DATA *ch, int num );
void edit_reset_init    ( CHAR_DATA *ch, int num );
void edit_reset_main    ( CHAR_DATA *ch, int num );
void edit_obj_init      ( CHAR_DATA *ch, int num );
void edit_obj_modify    ( CHAR_DATA *ch, int num );
void edit_obj_type      ( CHAR_DATA *ch, int num );
void edit_obj_add_aff   ( CHAR_DATA *ch, int num );

MENU_DATA edit_menu = {
  {"Edit Menu","",0,NULL},
  {"Personal [Settings]","settings",ID_EDIT_SETTINGS,edit_main},
  {"Edit [Area]","area",ID_EDIT_AREA,edit_main},
  {"Edit [Room]","room",ID_EDIT_ROOM,edit_main},
  {"Edit [Mob]","mob",ID_EDIT_MOB,edit_main},
  {"Edit [Object]","object",ID_EDIT_OBJECT,edit_main},
  {"Edit [Resets]","resets",ID_EDIT_RESETS,edit_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA settings_menu = {
  {"Personal Settings","",0,NULL},
  {"Toggle [Room]   - Default to current room","room",ID_SETTINGS_DEF_ROOM,edit_settings},
  {"Toggle [Mob]    - Default to first mob in room","mob",ID_SETTINGS_DEF_MOB,edit_settings},
  {"Toggle [Object] - Default to first object in inv.","object",ID_SETTINGS_DEF_OBJ,edit_settings},
  {"Toggle [Auto]   - Create rooms when walking","auto",ID_SETTINGS_AUTO,edit_settings},
  {"Toggle [Door]   - Double/Single door mode","door",ID_SETTINGS_DOOR,edit_settings},
  {"[Brief] Menus   - Toggle between brief and full menus","brief",ID_SETTINGS_BRIEF,edit_settings},
  {"[Help] on the Above Options","help",ID_SETTINGS_HELP,edit_settings},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA area_menu = {
  {"Area Menu","",20,edit_area_init},
  {"[Select] New Area","select",ID_EDIT_AREA_SELECT,edit_area},
  {"Area [Info]","info",ID_EDIT_AREA_INFO,edit_area},
  {"[Purge] Area]","purge",ID_EDIT_AREA_PURGE,edit_area},
  {"[Reset] Area","reset",ID_EDIT_AREA_RESET,edit_area},
  {"[New] Area","new",ID_EDIT_AREA_NEW,edit_area},
/*  {"[Load] Area","load",ID_EDIT_AREA_LOAD,edit_area}, */
  {"[Save] Area","save",ID_EDIT_AREA_SAVE,edit_area},
  {"[Rename] Area","rename",ID_EDIT_AREA_RENAME,edit_area},
  {"Toggle [Freeze] Area","freeze",ID_EDIT_AREA_FREEZE,edit_area},
  {"Toggle [Under] Construction","under",ID_EDIT_AREA_UNDER_DEV,edit_area},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA reset_menu = {
  {"Reset Menu","",0,edit_reset_init},
  {"Reload [Mob] Resets", "mob", ID_EDIT_RESET_MOB,edit_reset_main},
  {"Reload [Object] Resets","object",ID_EDIT_RESET_OBJECT,edit_reset_main},
  {"Reload [Door] Resets","door",ID_EDIT_RESET_DOOR,edit_reset_main},
  {"Reload [All] Resets","all",ID_EDIT_RESET_ALL,edit_reset_main},
  {"Add Current [Room] Resets","room",ID_EDIT_RESET_ROOM,edit_reset_main},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA room_menu = {
  {"Room Menu","",0,edit_room_init},
  {"[Select] Room Vnum","select",ID_EDIT_VNUM,edit_room},
  {"[List] Rooms in Area","list",ID_EDIT_LIST,edit_room},
  {"Room [Info]","info",ID_EDIT_INFO,edit_room},
  {"[Create] New Room","create",ID_EDIT_CREATE,edit_room},
  {"[Copy] Room","copy",ID_EDIT_CLONE,edit_room},
  {"[Modify] Room","modify",ID_EDIT_MODIFY,edit_room},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA room_modify_menu = {
  {" Modify Room Menu","",0,edit_room_init},
  {" Modify [Name]","name",ID_ROOM_NAME,edit_room_modify},
  {" Modify [Description]","description",ID_ROOM_DESC,edit_room_modify},
  {" Add [Extended] Descrip","extended",ID_ROOM_EXTENDED,edit_room_modify},
  {" [Remove] Extended Desrip","remove",ID_ROOM_REM_EXTEND,edit_room_modify},   
  {" Modify [Flags]","flags",ID_ROOM_FLAGS,edit_room_modify},
  {" Modify [Door]","door",ID_ROOM_DOOR,edit_room_modify},
  {" Modify [Sector]","sector",ID_ROOM_TERRAIN,edit_room_modify},
  {" Modify [Heal] Rate","heal",ID_ROOM_HEAL,edit_room_modify},
  {" Modify [Mana] Rate","mana",ID_ROOM_MANA,edit_room_modify},
  {"Modify [Clan]","clan",ID_ROOM_CLAN,edit_room_modify},
  {"Modify [Observation] Target","observation",ID_ROOM_OBS,edit_room_modify},
  {"Modify [Owner]","owner",ID_ROOM_OWNER,edit_room_modify},
  {"[Done] Modifying Room","done",ID_EDIT_PREVIOUS,edit_room_modify},
  {"[Main] Menu","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA exit_modify_menu = {
  {"Modify Door Menu","",0,edit_exit_init},
  {"Toggle Door [Mode] (Double/Single)","mode",ID_SETTINGS_DOOR,edit_settings},
  {"[Select] Door","select",ID_DOOR_SELECT,edit_room_exits},
  {"Door [Info]","info",ID_DOOR_INFO,edit_room_exits},
  {"Modify [Destination]","destination",ID_DOOR_VNUM,edit_room_exits},
  {"Modify [Name]","name",ID_DOOR_KEYWORD,edit_room_exits},
  {"Modify [Key]","key",ID_DOOR_KEY,edit_room_exits},
  {"Modify [Flags]","flags",ID_DOOR_FLAGS,edit_room_exits},
  {"[Copy] to Other Side","copy",ID_DOOR_COPY,edit_room_exits},
  {"[Remove] Exit","remove",ID_DOOR_REMOVE,edit_room_exits},
  {"[Done] Modifying Door","done",ID_EDIT_PREVIOUS,edit_room_exits},
  {"[Main] Menu","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA sector_menu = {
  {"Select Sector","",0,edit_sector_init},
  {"Set Sector to [Inside]","inside",SECT_INSIDE,edit_sector},
  {"Set Sector to [City]","city",SECT_CITY,edit_sector},
  {"Set Sector to [Field]","field",SECT_FIELD,edit_sector},
  {"Set Sector to [Forest]","forest",SECT_FOREST,edit_sector},
  {"Set Sector to [Hills]","hills",SECT_HILLS,edit_sector},
  {"Set Sector to [Mountain]","mountain",SECT_MOUNTAIN,edit_sector},
  {"Set Sector to [Water_swim]","water_swim",SECT_WATER_SWIM,edit_sector},
  {"Set Sector to [Water_noswim]","water_noswim",SECT_WATER_NOSWIM,edit_sector},
  {"Set Sector to [Air]","air",SECT_AIR,edit_sector},
  {"Set Sector to [Desert]","desert",SECT_DESERT,edit_sector},
  {"Set Sector to [Simple] Magelab","simple",SECT_MAGELAB_SIMPLE,edit_sector},
  {"Set Sector to [Intermediate] Magelab","intermediate",SECT_MAGELAB_INTERMEDIATE,edit_sector},
  {"Set Sector to [Advanced] Magelab","advanced",SECT_MAGELAB_ADVANCED,edit_sector},
  {"Set Sector to [Superior] Magelab","superior",SECT_MAGELAB_SUPERIOR,edit_sector},
  {"Set Sector to [Basic] Altar","basic",SECT_ALTAR_BASIC,edit_sector},
  {"Set Sector to [Blessed] Altar","blessed",SECT_ALTAR_BLESSED,edit_sector},
  {"Set Sector to [Annointed] Altar","annointed",SECT_ALTAR_ANNOINTED,edit_sector},
  {"Set Sector to [HolyGround] Altar","holyground",SECT_ALTAR_HOLY_GROUND,edit_sector},
  {"Set Sector to [Fire Plane]","fire plane",SECT_FIRE_PLANE,edit_sector},
  {"Set Sector to [Water Plane]","water plane",SECT_WATER_PLANE,edit_sector},
  {"Set Sector to [Observation] Room","observation",SECT_OBS_ROOM,edit_sector},
  {"[Done] Modifying Sector","done",ID_EDIT_PREVIOUS,edit_sector},
  {"[Main] Menu","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};


MENU_DATA mob_menu =  {
  {"Mob Menu","",0,edit_mob_init},
  {" [Select] Mob Vnum","select",ID_EDIT_VNUM,edit_mob},
  {" [List] Mobs in Area","list",ID_EDIT_LIST,edit_mob},
  {" [Create] New Mob","create",ID_EDIT_CREATE,edit_mob},
  {" [Copy] Mob","copy",ID_EDIT_CLONE,edit_mob},
  {" [Modify] Mob","modify",ID_EDIT_MODIFY,edit_mob},
  {" Mob [Info]","info",ID_EDIT_INFO,edit_mob},
  {" Update [Instances] of the Mob","instances",ID_EDIT_INSTANCE,edit_mob},
  {" Modify [Shop] Info","shop",ID_EDIT_SHOP,edit_mob},
  {" Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA mob_modify_menu = {
  {"Modify Mob Menu","",30,edit_mob_init},
  {" Modify [Name]","name",ID_MOB_NAME,edit_mob_modify},
  {" Modify [Level]","level",ID_MOB_LEVEL,edit_mob_modify},
  {" Modify [Short] Description","short",ID_MOB_SHORT,edit_mob_modify},
  {" Modify [Long] Description","long",ID_MOB_LONG,edit_mob_modify},
  {" Modify [Description]","description",ID_MOB_DESC,edit_mob_modify},
  {" Modify [Hp]","hp",ID_MOB_HP,edit_mob_modify},
  {" Modify [Mana]","mana",ID_MOB_MANA,edit_mob_modify},
  {" Modify [Ac]","ac",ID_MOB_AC,edit_mob_modify},
  {" Modify [Attack] Type","attack",ID_MOB_DAMTYPE,edit_mob_modify},
  {"Modify [Damage]","damage",ID_MOB_DAMAGE,edit_mob_modify},
  {"Modify [Hitroll]","hitroll",ID_MOB_HITROLL,edit_mob_modify},
  {"Modify [Act] Flags","act",ID_MOB_ACT,edit_mob_modify},
  {"Modify [Offensive] Flags","offensive",ID_MOB_OFF,edit_mob_modify},
  {"Modify [Immunities]","immunities",ID_MOB_IMM,edit_mob_modify},
  {"Modify [Resistances]","resistances",ID_MOB_RES,edit_mob_modify},
  {"Modify [Vulnerabilties]","vulnerabilities",ID_MOB_VULN,edit_mob_modify},
  {"Modify [Parts]","parts",ID_MOB_PARTS,edit_mob_modify},
  {"Modify [Form]","form",ID_MOB_FORM,edit_mob_modify},
  {"Modify [Race]","race",ID_MOB_RACE,edit_mob_modify},
  {"Modify [Sex]","sex",ID_MOB_SEX,edit_mob_modify},
  {"Modify [Position]","position",ID_MOB_POS,edit_mob_modify},
  {"Modify [Wealth]","wealth",ID_MOB_WEALTH,edit_mob_modify},
  {"Modify [Size]","size",ID_MOB_SIZE,edit_mob_modify},
  {"Modify [Alignment]","alignment",ID_MOB_ALIGN,edit_mob_modify},
  {"Modify [Affects]","affects",ID_MOB_AFF,edit_mob_modify},
  {"Modify [Special]","special",ID_MOB_SPEC,edit_mob_modify},
  {"Modify Spec [Words]","words",ID_MOB_SPEC_WORDS,edit_mob_modify},
  {"[Done] Modifying Mob","done",ID_EDIT_PREVIOUS,edit_mob_modify},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA mob_shop_menu  = {
  {"Mob Shop Menu","",0,NULL},
  {"[Copy] Shop","copy",ID_SHOP_CLONE,edit_mob_shop},
  {"Modify Shop [Buy] Types","buy",ID_SHOP_BUY,edit_mob_shop},
  {"Modify Shop [Profits]","profits",ID_SHOP_PROFIT,edit_mob_shop},
  {"Modify Shop [Hours]","hours",ID_SHOP_HOURS,edit_mob_shop},
  {"Shop [Info]","info",ID_SHOP_INFO,edit_mob_shop},
  {"[Remove] Shop","remove",ID_SHOP_REMOVE,edit_mob_shop},
  {"[Done] Modifing Shop","done",ID_EDIT_PREVIOUS,edit_mob_shop},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA mob_size_menu = {
  {"Modify Mob Size Menu","",0,edit_mob_size_init},
  {"Set Size to [Tiny]","tiny",SIZE_TINY,edit_mob_size},
  {"Set Size to [Small]","small",SIZE_SMALL,edit_mob_size},
  {"Set Size to [Medium]","medium",SIZE_MEDIUM,edit_mob_size},
  {"Set Size to [Large]","large",SIZE_LARGE,edit_mob_size},
  {"Set Size to [Huge]","huge",SIZE_HUGE,edit_mob_size},
  {"Set Size to [Giant]","giant",SIZE_GIANT,edit_mob_size},
  {"[Cancel] Operation","cancel",ID_EDIT_CANCEL,edit_mob_size},
  {NULL,"",0,NULL}
};

MENU_DATA mob_position_menu = {
  {"Modify Mob Position","",0,edit_mob_pos_init},
  {"Position [Sleeping]","sleeping",POS_SLEEPING,edit_mob_position},
  {"Position [Resting]","resting",POS_RESTING,edit_mob_position},
  {"Position [Sitting]","sitting",POS_SITTING,edit_mob_position},
  {"Position [Standing]","standing",POS_STANDING,edit_mob_position},
  {"[Cancel] Operation","cancel",ID_EDIT_CANCEL,edit_mob_position},
  {NULL,"",0,NULL}
};

MENU_DATA *mob_modify_att_menu;

MENU_DATA object_menu = {
  {"Object Menu","",0,edit_obj_init},
  {"[Select] Object Vnum","select",ID_EDIT_VNUM,edit_object},
  {"[List] Objects in Area","list",ID_EDIT_LIST,edit_object},
  {"[Create] New Object","create",ID_EDIT_CREATE,edit_object},
  {"[Copy] Object","copy",ID_EDIT_CLONE,edit_object},
  {"[Modify] Object","modify",ID_EDIT_MODIFY,edit_object},
  {"Object [Info]","info",ID_EDIT_INFO,edit_object},
  {"Update [Instances] of the Object","instances",ID_EDIT_INSTANCE,edit_object},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};


MENU_DATA obj_modify_menu = {
  {"Object Modify Menu","",30,NULL},
  {" Modify [Name]","name",ID_OBJ_NAME,edit_obj_modify},
  {" Modify [Level]","level",ID_OBJ_LEVEL,edit_obj_modify},
  {" Modify [Short] Description","short",ID_OBJ_SHORT,edit_obj_modify},
  {" Modify [Long] Description","long",ID_OBJ_DESCR,edit_obj_modify},
  {" Add [Extended] Descrip","extended",ID_OBJ_EXTENDED,edit_obj_modify},
  {" [Remove] Extended Desrip","remove",ID_OBJ_REM_EXTEND,edit_obj_modify},
  {" Modify [Material]","material",ID_OBJ_MATERIAL,edit_obj_modify},
  {" Modify Item [Type]","type",ID_OBJ_TYPE,edit_obj_modify},
  {" Modify Extra [Flags]","flags",ID_OBJ_FLAGS,edit_obj_modify},
  {"Modify [Wear] Flags","wear",ID_OBJ_WEAR,edit_obj_modify},
  {"Modify [Condition]","condition",ID_OBJ_COND,edit_obj_modify},
  {"Modify [Weight]","weight",ID_OBJ_WEIGHT,edit_obj_modify},
  {"Modify [Cost]","cost",ID_OBJ_COST,edit_obj_modify},
  {"Modify [Values]","values",ID_OBJ_VALUE,edit_obj_modify},
  {"[Add] Affect","add",ID_OBJ_AFF_ADD,edit_obj_modify},
  {"[Delete] Affect","delete",ID_OBJ_AFF_REMOVE,edit_obj_modify},
  {"Modify Wear [Timer]","timer",ID_OBJ_WEAR_TIMER,edit_obj_modify},
  {"[Done] Modifying Object","done",ID_EDIT_PREVIOUS,edit_obj_modify},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};

MENU_DATA obj_affect_menu = {
  {"Add Affect Menu","",30,NULL},
  {"Apply [Str]","str",APPLY_STR,edit_obj_add_aff},
  {"Apply [Dex]","dex",APPLY_DEX,edit_obj_add_aff},
  {"Apply [Int]","int",APPLY_INT,edit_obj_add_aff},
  {"Apply [Wis]","wis",APPLY_WIS,edit_obj_add_aff},
  {"Apply [Con]","con",APPLY_CON,edit_obj_add_aff},
//  {"Apply [Agt]","agt",APPLY_AGT,edit_obj_add_aff},
//  {"Apply [End]","end",APPLY_END,edit_obj_add_aff},
  {"Apply [Cha]","cha",APPLY_SOC,edit_obj_add_aff},
  {"Apply [Sex]","sex",APPLY_SEX,edit_obj_add_aff},
/*  {"Apply [Class]","class",APPLY_CLASS,edit_obj_add_aff},
  {"Apply [Level]","level",APPLY_LEVEL,edit_obj_add_aff},
  {"Apply [Age]","age",APPLY_AGE,edit_obj_add_aff},
  {"Apply [Height]","height",APPLY_HEIGHT,edit_obj_add_aff},
  {"Apply [Weight]","weight",APPLY_WEIGHT,edit_obj_add_aff}, */
  {"Apply [Mana]","mana",APPLY_MANA,edit_obj_add_aff},
  {"Apply [Hit] Points","hit",APPLY_HIT,edit_obj_add_aff},
  {"Apply [Move]","move",APPLY_MOVE,edit_obj_add_aff},
/*  {"Apply [Gold]","gold",APPLY_GOLD,edit_obj_add_aff},
  {"Apply [Exp]","exp",APPLY_EXP,edit_obj_add_aff}, */
  {"Apply [Ac]","ac",APPLY_AC,edit_obj_add_aff},
  {"Apply [Hitroll]","hitroll",APPLY_HITROLL,edit_obj_add_aff},
  {"Apply [Damroll]","damroll",APPLY_DAMROLL,edit_obj_add_aff},
  {"Apply [Saving] Spell","saving",APPLY_SAVES,edit_obj_add_aff},
  {"Apply [Spell] Affect","spell",APPLY_SPELL_AFFECT,edit_obj_add_aff},
  {"Apply [Reflex] Save","reflex",APPLY_REFLEX_SAVE,edit_obj_add_aff},
  {"Apply [Will] Save","will",APPLY_WILLPOWER_SAVE,edit_obj_add_aff},
  {"Apply [Fortitude] Save","fortitude",APPLY_FORTITUDE_SAVE,edit_obj_add_aff},
  {"[Done] Modifying Affects","done",ID_EDIT_PREVIOUS,edit_obj_add_aff},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};


/* name
   level
   short
   long
   cost
   weight
   type
   apply
   extra
   extended
   wear
   - food, liquid, armor, furniture, container, portal, spell, use
   valuesfs
*/

MENU_DATA obj_type_menu = {
  {"Object Type Menu","",30,NULL},
  {" Type [Light]","light",ITEM_LIGHT,edit_obj_type},
  {" Type [Scroll]","scroll",ITEM_SCROLL,edit_obj_type},
  {" Type [Wand]","wand",ITEM_WAND,edit_obj_type},
  {" Type [Staff]","staff",ITEM_STAFF,edit_obj_type},
  {" Type [Weapon]","weapon",ITEM_WEAPON,edit_obj_type},
/*  {"Type [Treasure]","treasure",ITEM_TREASURE,edit_obj_type}, */
  {" Type [Armor]","armor",ITEM_ARMOR,edit_obj_type},
  {" Type [Potion]","potion",ITEM_POTION,edit_obj_type},
  {" Type [Clothing]","clothing",ITEM_CLOTHING,edit_obj_type},
  {" Type [Furniture]","furniture",ITEM_FURNITURE,edit_obj_type},
  {"Type [Trash]","trash",ITEM_TRASH,edit_obj_type},
  {"Type [Container]","container",ITEM_CONTAINER,edit_obj_type},
  {"Type [Drink] Container","drink",ITEM_DRINK_CON,edit_obj_type},
  {"Type [Key]","key",ITEM_KEY,edit_obj_type},
  {"Type [Food]","food",ITEM_FOOD,edit_obj_type},
  {"Type [Money]","money",ITEM_MONEY,edit_obj_type},
  {"Type [Boat]","boat",ITEM_BOAT,edit_obj_type},
  {"Type [Corpse]","corpse",ITEM_CORPSE_NPC,edit_obj_type},
  {"Type [Fountain]","fountain",ITEM_FOUNTAIN,edit_obj_type},
  {"Type [Pill]","pill",ITEM_PILL,edit_obj_type},
  {"Type [Protect]","protect",ITEM_PROTECT,edit_obj_type},
  {"Type [Map]","map",ITEM_MAP,edit_obj_type},
  {"Type [Portal]","portal",ITEM_PORTAL,edit_obj_type},
  {"Type [Warp] Stone","warp",ITEM_WARP_STONE,edit_obj_type},
  {"Type [Room] Key","room",ITEM_ROOM_KEY,edit_obj_type},
  {"Type [Gem]","gem",ITEM_GEM,edit_obj_type},
  {"Type [Jewelry]","jewelry",ITEM_JEWELRY,edit_obj_type},
  {"Type [Jukebox]","jukebox",ITEM_JUKEBOX,edit_obj_type},
  {"Type [Trap]","trap",ITEM_TRAP,edit_obj_type},
  {"Type [Mixer]","mixer",ITEM_MIXER,edit_obj_type},
  {"Type [Grenade]","grenade",ITEM_GRENADE,edit_obj_type},
  {"Type [Page]","page",ITEM_SPELL_PAGE,edit_obj_type},
  {"Type [Part]", "part", ITEM_PART, edit_obj_type},
  {"Type [Forge]", "forge", ITEM_FORGE, edit_obj_type},
  {"Type [Herb]", "herb", ITEM_HERB, edit_obj_type},
  {"Type [Capsule]", "capsule", ITEM_CAPSULE, edit_obj_type},
  {"[Cancel] Setting Object Type","cancel",ID_EDIT_PREVIOUS,edit_obj_type},
  {"Goto [Main]","main",ID_EDIT_GOTO_MAIN,edit_goto_main},
  {"[Exit] OLC","exit",ID_EDIT_EXIT,edit_exit},
  {NULL,"",0,NULL}
};


struct val_type {
  int num;
  char *val[5];
};

struct val_type val_table[] = {
  {ITEM_LIGHT,{NULL,NULL,"Hours of light [0 is dead, -1 is infinite]",NULL,NULL}},
  {ITEM_SCROLL,{"Spell level","SN #1","SN #2","SN #3",NULL}},
  {ITEM_WAND,{"Spell level","Max Charges","Current Charges","SN",NULL}},
  {ITEM_STAFF,{"Spell level","Max Charges","Current Charges","SN",NULL}},
  {ITEM_WEAPON,{"Weapon type","Dice Sides","# of Dice","Damage Noun","Weapon flags"}},
  {ITEM_ARMOR,{"Pierce AC","Bash AC","Slash AC","Magic AC",NULL}},
  {ITEM_POTION,{"Spell level","SN #1","SN #2","SN #3",NULL}},
  {ITEM_FURNITURE,{"Max Users",NULL,"Position Flags","HP Gain [Percentage]","Mana Gain [Percentage]"}},
  {ITEM_CONTAINER,{"Weight Capacity","Flags","Key Vnum","Max Weight that Fits","Weight multiplier"}},
  {ITEM_DRINK_CON,{"Capacity","Current Quantity","Liquid Number","Poisoned [Non zero is poisoned]",NULL}},
  {ITEM_FOOD,{"Hours of food value","Saturation","Liquid Number","Poisoned [Non zero is poisoned]","Liquid Amount"}},
  {ITEM_MONEY,{"Silver Value","Gold Value",NULL,NULL,NULL}},
  {ITEM_PILL,{"Spell level","SN #1","SN #2","SN #3",NULL}},
  {ITEM_MIXER,{"Mixer level","Comp #1","Comp #2","Comp #3","Comp #4"}},
  {ITEM_PORTAL,{"Charges [0 for infinite]", "","Exit Flags","Location (vnum)",""}},
  {ITEM_SPELL_PAGE,{"Scribe Level","Difficulty","Spell Slot","Uses",""}},
  {ITEM_PART, {"Whole Object Vnum", "Part 1 Vnum","Part 2 Vnum", "Part3 Vnum", "Part 4 Vnum"}},
  {ITEM_FORGE,{"Recipe 1", "Recipe 2", "Recipe 3", "Recipe 4", "Recipe 5"}},
  {ITEM_CAPSULE,{"XP Reward", "Silver Reward", "SP Reward", "Item Reward", "Reward Variance %"}},
  {0,{NULL,NULL,NULL,NULL,NULL}}
};

static char *sector_table[] = {
  "inside",
  "city",
  "field",
  "forest",
  "hills",
  "mountain",
  "water_swim",
  "water_noswim",
  "unused",
  "air",
  "desert",
  "simple",
  "intermediate",
  "advanced",
  "superior",
  "basic",
  "blessed",
  "annointed",
  "holy_ground",
  "fire plane",
  "water plane",
  "observation"
};

static char *dir_table[] = {
  "north","east","south","west","up","down"
};

static char *room_exit_flags[] = {
  "door",
  "closed",
  "locked",
  "NA","NA",
  "pickproof",
  "nopass",
  "easy",
  "hard",
  "infuriating",
  "noclose",
  "nolock",
  "concealed",
  "secret",
  NULL
};

static char *mod_room_flags[] = {
  "dark",
  "NA",
  "no_mob",
  "indoors",
  "NA",
  "NA",
  "NA",
  "NA",
  "NA",
  "private",
  "safe",
  "solitary",
  "pet_shop",
  "no_recall",
  "IMP_only",
  "gods_only",
  "heroes_only",
  "newbies_only",
  "lawful",
  "no_where",
  "no_die",
  "no_clan",
  "no_combat",
  "holy_ground",
  "clan_only",
  "isolated",// New isolated code
  "no_hall",
  NULL
};

static char *act_table[] = {
  "npc",
  "sentinel",
  "scavenger",
  "NA","NA",
  "aggressive",
  "stay_area",
  "wimpy",
  "pet",
  "train",
  "practice",
  "NA","NA","NA",
  "undead",
  "weaponsmith",
  "cleric",
  "mage",
  "thief",
  "warrior",
  "no_align",
  "no_purge",
  "NA","armourer","NA",
  "mount",
  "healer",
  "gain",
  "update_always",
  "changer", "notrans",
  NULL
};

/* static char *att_table[] = {
  "slice"
  "stab"
  "slash"
  "whip"
  "claw"
  "blast",
  "pound",
  "crush",
  "grep",
  "bite",
  "pierce",
  "suction",
  "beating",
  "digestion",
  "charge",
  "slap",
  "punch",
  "wrath",
  "magic",
  "divine power",
  "cleave",
  "scratch",
  "peck",
  "peck",
  "chop",
  "sting",
  "smash",
  "shocking bite",
  "flaming bite",
  "freezing bite",
  "acidic bite",
  "chomp",
  NULL
}; */


/* static char *dam_table[] = {
  "none",
  "bash",
  "pierce",
  "slash",
  "fire",
  "cold",
  "lightning",
  "acid",
  "poison",
  "negative",
  "holy",
  "energy",
  "mental",
  "disease",
  "drowning",
  "light",
  "other",
  "harm",
  NULL
}; */


static char *off_table[] = {
  "area_attack",
  "backstab",
  "bash",
  "berserk",
  "disarm",
  "dodge",
  "fade",
  "fast",
  "kick",
  "kick_dirt",
  "parry",
  "rescue",
  "tail",
  "trip",
  "crush",
  "assist_all",
  "assist_align",
  "assist_race",
  "assist_players",
  "assist_guard",
  "assist_vnum",
  "off_charge",
  "assist_element",
  "bane_touch",
  NULL
};


static char *irv_table[] = {
  "summon",
  "charm",
  "magic",
  "weapon",
  "bash",
  "pierce",
  "slash",
  "fire",
  "cold",
  "lightning",
  "acid",
  "poison",
  "negative",
  "holy",
  "energy",
  "mental",
  "disease",
  "drowning",
  "light",
  NULL
};

static char *form_table[] = {
  "edible",
  "poison",
  "magical",
  "instant decay",
  "other",
  "NA",
  "animal",
  "sentient",
  "undead",
  "construct",
  "mist",
  "intangible",
  "biped",
  "centaur",
  "insect",
  "spider",
  "crustacean",
  "worm",
  "blob",
  "NA","NA",
  "mammal",
  "bird",
  "reptile",
  "snake",
  "dragon",
  "amphibian",
  "fish",
  "cold blood",
  NULL
};

static char *part_table[] = {
  "head",
  "arms",
  "legs",
  "heart",
  "brains",
  "guts",
  "hands",
  "feet",
  "fingers",
  "ear",
  "eye",
  "long_tongue",
  "eyestalks",
  "tentacles",
  "fins",
  "wings",
  "tail"
  "NA","NA","NA",
  "claws",
  "fangs",
  "horns",
  "scales",
  "tusks",
  NULL
};

static char *aff_table[] = {
  "blind",
  "invisible",
  "detect_align",
  "detect_invis",
  "detect_magic",
  "detect_hidden",
  "faerie_fog",
  "sanctuary",
  "faerie_fire",
  "infrared",
  "curse",
  "flaming",
  "poison",
  "protect",
  "paralysis",
  "sneak",
  "hide",
  "sleep",
  "charm",
  "flying",
  "pass_door",
  "haste",
  "calm",
  "plague",
  "weaken",
  "dark_vision",
  "berserk",
  "swim",
  "regeneration",
  "NA",
  "withstand_death",
  NULL
};

static char *obj_extra_flags[] = {
  "glow",
  "hum",
  "immload",
  "lock",
  "evil",
  "invis",
  "magic",
  "nodrop",
  "bless",
  "anti_good",
  "anti_evil",
  "anti_neutral",
  "noremove",
  "inventory",
  "nopurge",
  "rot_death",
  "vis_death",
  "nosac",
  "nonmetal",
  "nolocate",
  "melt_drop",
  "had_timer",
  "sell_extract",
  "wear_timer",
  "burn_proof",
  "nouncurse",
  "clan_corpse",
  "warped",
  "teleport",
  "noidentify",
  NULL
};

static char *obj_wear_flags[] = {
  "take",
  "finger",
  "neck",
  "torso",
  "head",
  "legs",
  "feet",
  "hands",
  "arms",
  "shield",
  "body",
  "waist",
  "wrist",
  "wield",
  "hold",
  "no_sac",
  "float",
  NULL
};

char *avg_table[] = {
  "hopelessly lost",
  "pathetic",
  "terrible",
  "poor",
  "less than average",
  "average",
  "better than average",
  "good",
  "great",
  "superb",
  "awesome",
  NULL
};

extern   sh_int  rev_dir[];
/* Recipe editing only accessible on OLC */
#ifdef OLC_VERSION
  extern RECIPE_DATA *recipe_first;
  extern RECIPE_DATA *recipe_last;
  extern int top_recipe;
  extern char recipe_file[];
#endif

#ifdef OLC_VERSION
/* Save all recipes, called whenever one is created, edited, or deleted */
void save_recipes(void)
{
  RECIPE_DATA *walker;
  int i;
  FILE *fp = fopen(TEMP_FILE, "w");
  if(!fp)
  {
    bug("Error opening recipe file to save.\n\r", 0);
    return;
  }
  fprintf(fp, "#RECIPES\n\n");
  walker = recipe_first;
  while(walker)
  {
    fprintf(fp, "#%d\n", walker->recipe_num);
    if(walker->skill_sn < 0 || walker->skill_sn >= MAX_SKILL)
      fprintf(fp, "noskill %d\n", walker->difficulty);
    else
      fprintf(fp, "'%s' %d\n", skill_table[walker->skill_sn].name, walker->difficulty);
    fprintf(fp, "%d\n%d\n", walker->vnum_container, walker->vnum_complete);
    for(i = 0; i < MAX_IN_RECIPE; i++)
    {
      if(!i)
        fprintf(fp, "%d", walker->vnum_parts[i]);
      else/* Keep the spacing clean */
        fprintf(fp, " %d", walker->vnum_parts[i]);
      if(!walker->vnum_parts[i])
        break;
    }
    if(walker->active)
      fprintf(fp, "\n1\n");
    else
      fprintf(fp, "\n0\n");
    if(walker->cmessage)
      fprintf(fp, "%s~\n", walker->cmessage);
    else
      fprintf(fp, "~\n");
    if(walker->rmessage)
      fprintf(fp, "%s~\n", walker->rmessage);
    else
      fprintf(fp, "~\n");
    fprintf(fp, "\n");
    walker = walker->next;
  }
  fprintf(fp, "$\n#$\n");
  /* Done */
  fclose(fp);
  /* Copy it over the original file now */
  rename(TEMP_FILE, recipe_file);
}

/* Recipe editing command */
void do_recipe(CHAR_DATA *ch, char *argument)
{
  char buf[255], arg[255];
  int i;
  argument = one_argument(argument,arg);

  if(ch->in_room->vnum < 0)
  {
    send_to_char("Please leave the clan hall to edit recipes.\n\r", ch);
    return;
  }

  if(IS_NPC(ch) || !ch->in_room->area)
  {
    send_to_char("Error with edit range, please report this issue to an Admin.\n\r", ch);
    return;
  }
  
  if(get_trust(ch) < CREATOR)
  {
    if(!ch->pcdata->edit.range)
    {
      send_to_char("You have no permissions to edit areas.\n\r", ch);
      return;
    }
    VNUM_RANGE_DATA *range = ch->pcdata->edit.range;
    while(range)
    {
      if(ch->in_room->vnum >= range->min && ch->in_room->vnum <= range->max)
        break;
      range = range->next;
    }
    if(!range)
    {
      send_to_char("You do not have permission to work with recipes in this zone.\n\r", ch);
      send_to_char("Recipe access is based on the result item's vnum.\n\r", ch);
      return;
    }
  }
/*    sh_int              min_vnum_obj;
    sh_int              max_vnum_obj;*/
/*  range = ch->pcdata->edit.range;
  while (range) {
    if ((vnum >= range->min) && (vnum <= range->max)) {
      return TRUE;
    }
    range = range->next;
  }*/
/* WORKING HERE */
// Allow an option "any" forge
// Allow list by current area vnum
// Permission to view based on result vnum
// Allow difficulty without a skill

  if (arg[0] == '\0')
  {
/*    if(get_trust(ch) >= CREATOR)
      send_to_char("Valid recipe commands are: List, Detail, Create, Edit, Delete and Help.\n\r", ch);
    else*/
    send_to_char("Commands: List, Detail, Create, Edit, Ingr, Activate, CMessage, RMessage, Help.\n\r", ch);
    return;
  }
  
  if(!str_prefix(arg, "help"))
  {
    /*if(get_trust(ch) >= CREATOR)
      send_to_char("Valid recipe commands are: List, Detail, Create, Edit, Delete and Help.\n\r", ch);
    else*/
      send_to_char("Commands: List, Detail, Create, Edit, Ingr, Activate, CMessage, RMessage, Help.\n\r", ch);
    send_to_char("Editing or creating a recipe is done in one step:\n\r", ch);
    /* Can't be quite MAX_IN_RECIPE because loading would break if the number is ever increased and a recipe has max */
    send_to_char("create <forge vnum> <result vnum> <skill> <difficulty>", ch);
    send_to_char("edit <recipe #> <forge vnum> <result vnum> <skill> <difficulty>", ch);
    sprintf(buf, "ingr <recipe #> <ingredient1 vnum> <ingredient2 vnum> ... <ingredient%d vnum>\n\r", MAX_IN_RECIPE - 1);
    send_to_char(buf, ch);
    send_to_char("To make a recipe that works in any forge set forge vnum to -1\n\r", ch);
    send_to_char("Use noskill if you don't want a skill required but want a difficulty.\n\r", ch);
    send_to_char("Difficulty is % failure, negative improves chances with a skill required.\n\r", ch);
    send_to_char("Only ingredient1 and ingredient2 are required, the rest are optional.\n\r\n\r", ch);
    send_to_char("List is used to show all recipes with results in your current area.\n\r", ch);
    send_to_char("Detail is used to show more details on a given recipe #\n\r", ch);
    send_to_char("CMessage is Char Message, shows to the user upon success.\n\r", ch);
    send_to_char("RMessage is Room Message, shows to the room upon success.\n\r", ch);
    send_to_char("For both messages, $p uses the forge name and $n uses the user name.\n\r\n\r", ch);
    send_to_char("A recipe must be activated in order for it to be used.\n\r", ch);
    send_to_char("Activate is a toggle, to deactivate a recipe use Activate again.\n\r", ch);
    /*if(get_trust(ch) >= CREATOR)
    {
      send_to_char("You may use 'recipe list all' to see all recipes.\n\r", ch);
      send_to_char("{RNote that delete is permanent and does not ask if you're sure, be careful.{x\n\r\n\r", ch);
    }*/
    return;
  }

  if(!str_prefix(arg, "list"))
  {
    char extra[255];
    RECIPE_DATA *recipe = recipe_first;
    while(recipe)
    {
      if(recipe->vnum_complete >= ch->in_room->area->min_vnum_obj &&
        recipe->vnum_complete <= ch->in_room->area->max_vnum_obj)
      {
        sprintf(buf, "Recipe #: %d Forge: %d, Result: %d", recipe->recipe_num, recipe->vnum_container, recipe->vnum_complete);
        if(recipe->skill_sn > 0)
        {
          if(recipe->skill_sn < MAX_SKILL)
            sprintf(extra, " Skill: %s, Diff: %d", skill_table[recipe->skill_sn].name, recipe->difficulty);
          else
            sprintf(extra, " Skill: N/A, Diff: %d", recipe->difficulty);
          strcat(buf, extra);
        }
        if(recipe->active)
          strcat(buf, " ACTIVE");
        else
          strcat(buf, " Inactive");
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
        sprintf(buf, "  Ingredients: %d", recipe->vnum_parts[0]);
        if(recipe->vnum_parts[0] != 0)
        {
          for(i = 1; i < MAX_IN_RECIPE; i++)
          {
            if(recipe->vnum_parts[i] == 0)
              break;
            sprintf(extra, " %d", recipe->vnum_parts[i]);
            strcat(buf, extra);
          }
        }
        strcat(buf, "\n\r");
        send_to_char(buf, ch);
      }
      recipe = recipe->next;
    }
  }

  if(!str_prefix(arg, "detail"))
  {
    OBJ_INDEX_DATA *ingredient;
    int value;
    RECIPE_DATA *recipe = recipe_first;
    argument = one_argument(argument, arg);
    if(arg[0] == 0 || !is_number(arg))
    {
      send_to_char("Usage: recipe detail <recipe num>\n\r", ch);
      return;
    }
    
    value = atoi(arg);
    
    while(recipe)
    {
      if(recipe->recipe_num == value)
      {
        if(get_trust(ch) < CREATOR &&
          (recipe->vnum_complete < ch->in_room->area->min_vnum_obj ||
          recipe->vnum_complete > ch->in_room->area->max_vnum_obj))
        {/* Can't view this recipe */
          send_to_char("No recipe with that vnum found in the current area.\n\r", ch);
          break;
        }
        sprintf(buf, "Vnum: %d (%s)\n\r", recipe->recipe_num, recipe->active ? "ACTIVE" : "Inactive");
        send_to_char(buf, ch);
        if(recipe->vnum_container == -1)
          send_to_char("Forge: Any\n\r", ch);
        else
        {
          ingredient = get_obj_index(recipe->vnum_container);
          if(ingredient)
            sprintf(buf, "Forge: %s (Vnum %d)\n\r", ingredient->name, recipe->vnum_container);
          else
            sprintf(buf, "Forge: N/A (Vnum %d)\n\r", recipe->vnum_container);
          send_to_char(buf, ch);
        }
        ingredient = get_obj_index(recipe->vnum_complete);
        if(ingredient)
          sprintf(buf, "Result: %s (Vnum %d)\n\r", ingredient->name, recipe->vnum_complete);
        else
          sprintf(buf, "Result: N/A (Vnum %d)\n\r", recipe->vnum_complete);
        send_to_char(buf, ch);
        if(recipe->skill_sn > 0)
        {
          if(recipe->skill_sn < MAX_SKILL)
            sprintf(buf, "Skill: %s\n\r", skill_table[recipe->skill_sn].name);
          else
            sprintf(buf, "Skill: N/A\n\r");
          send_to_char(buf, ch);
        }
        else
        {
          send_to_char("Skill: noskill\n\r", ch);
        }
        sprintf(buf, "Difficulty: %d\n\r", recipe->difficulty);
        send_to_char(buf, ch);
        send_to_char("Ingredients:\n\r", ch);
        for(i = 0; i < MAX_IN_RECIPE; i++)
        {
          if(recipe->vnum_parts[i] == 0)
            break;
          ingredient = get_obj_index(recipe->vnum_parts[i]);
          if(ingredient)
            sprintf(buf, "  %s (Vnum %d)\n\r", ingredient->name, recipe->vnum_parts[i]);
          else
            sprintf(buf, "  N/A (Vnum %d)\n\r", recipe->vnum_parts[i]);
          send_to_char(buf, ch);
        }
        if(recipe->cmessage)
        {
          send_to_char("CMessage: ", ch);
          send_to_char(recipe->cmessage, ch);
          send_to_char("\n\r", ch);
        }
        if(recipe->rmessage)
        {
          send_to_char("RMessage: ", ch);
          send_to_char(recipe->rmessage, ch);
          send_to_char("\n\r", ch);
        }
        send_to_char("\n\r", ch);
        break;
      }
      recipe = recipe->next;
    }
    return;
  }
  
  if(!str_prefix(arg, "create") || !str_prefix(arg, "edit"))
  {/* Together because they almost entirely overlap */ 
    int skill;
    int difficulty;
    int container;
    int result;
    OBJ_INDEX_DATA *forge;
    int recipe_num = 0;
    RECIPE_DATA *recipe = NULL;
    bool create = !str_prefix(arg, "create");
    if(!create)
    {
      argument = one_argument(argument, arg);
      if(arg[0] == 0)
      {
        send_to_char("Usage: recipe edit <recipe #> <forge vnum> <result vnum> <skill> <difficulty>", ch);
        return;
      }
      if(!is_number(arg))
      {
        send_to_char("Recipe # must be a number. Use recipe help for syntax.\n\r", ch);
        return;
      }
      recipe_num = atoi(arg);
      if((recipe = get_recipe_data(recipe_num)) == NULL)
      {
        send_to_char("That recipe # does not exist yet. Use recipe help for options.\n\r", ch);
        return;
      }
    }
    
    argument = one_argument(argument, arg);
    if(create && arg[0] == 0)
    {
      send_to_char("Usage: recipe create <forge vnum> <result vnum> <skill> <difficulty>", ch);
      return;
    }
    
    if(!is_number(arg))
    {
      send_to_char("The forge vnum must be a number.\n\r", ch);
      return;
    }
    container = atoi(arg);
    if(container != -1)
    {
      if((forge = get_obj_index(container)) == NULL)
      {
        sprintf(buf, "vnum %d for the forge does not exist.\n\r", container);
        send_to_char(buf, ch);
        return;
      }
      if(forge->item_type != ITEM_FORGE)
      {
        sprintf(buf, "Object %s (Vnum %d) is not a forge.\n\r", forge->name, container);
        send_to_char(buf, ch);
        return;
      }
    }
    
    argument = one_argument(argument, arg);
    if(!is_number(arg))
    {
      send_to_char("The result vnum must be a number.\n\r", ch);
      return;
    }
    result = atoi(arg);
    if(get_trust(ch) < CREATOR &&
      (result < ch->in_room->area->min_vnum_obj ||
      result > ch->in_room->area->max_vnum_obj))
    {
      send_to_char("The result vnum must be within this area.\n\r", ch);
      return;
    }
    if(get_obj_index(result) == NULL)
    {
      sprintf(buf, "vnum %d for the result does not exist.\n\r", result);
      send_to_char(buf, ch);
      return;
    }
    
    argument = one_argument(argument, arg);
    if(arg[0] == 0)
    {
      skill = -1;
      difficulty = 0;
    }
    else
    {
      if(!str_cmp(arg, "noskill"))
        skill = -1;
      else if((skill = skill_lookup(arg)) == -1)
      {
        send_to_char("Invalid skill requirement. Use noskill if you want no skill required.\n\r", ch);
        return;
      }
      
      argument = one_argument(argument, arg);
      if(!is_number(arg) || (difficulty = atoi(arg)) < -99 || difficulty > 99)
      {
        send_to_char("Difficulty must be a number from -99 to 99.  Use 0 for no difficulty.\n\r", ch);
        return;
      }
    }
    /* Ready to create or edit a recipe */
    if(create)
    {
      /* Assign recipe_num to one higher than the highest existing recipe_num */
      recipe = recipe_first;
      recipe_num = 1;
      while(recipe)
      {
        if(recipe->recipe_num >= recipe_num)
          recipe_num = recipe->recipe_num + 1;
        recipe = recipe->next;
      }
/* In case it's ever not OLC only */
#ifdef OLC_VERSION
      recipe = alloc_mem ( sizeof (*recipe));
#else
      recipe = GC_MALLOC( sizeof (*recipe) );
#endif
      if(recipe_first == NULL)
        recipe_first = recipe;
      if(recipe_last != NULL )
        recipe_last->next = recipe;
      recipe_last = recipe;
      recipe->next = NULL;
      top_recipe++;
    }
    /* If !create, recipe is already loaded from an earlier check */
    recipe->recipe_num = recipe_num;
    recipe->vnum_container = container;
    recipe->vnum_complete = result;
    recipe->skill_sn = skill;
    recipe->difficulty = difficulty;
    if(create)
    {
      recipe->active = FALSE;
      recipe->cmessage = NULL;
      recipe->rmessage = NULL;
      recipe->vnum_parts[0] = 0;
    }
    /* All set, save this */
    save_recipes();
    if(create)
      sprintf(buf, "Recipe %d created.\n\r", recipe->recipe_num);
    else
      sprintf(buf, "Recipe %d edited.\n\r", recipe->recipe_num);
    send_to_char(buf, ch);
    return;
  }
  
  if(!str_prefix(arg, "ingredients"))
  {
    int ingredients[MAX_IN_RECIPE];
    RECIPE_DATA *recipe = NULL;
    argument = one_argument(argument, arg);
    if(arg[0] == 0)
    {
      sprintf(buf, "Usage: recipe ingr <recipe #> <ingr1 vnum> <ingr2 vnum> ... <ingr%d vnum>\n\r", MAX_IN_RECIPE - 1);
      send_to_char(buf, ch);
      return;
    }
    if(!is_number(arg))
    {
      send_to_char("Recipe # must be a number. Use recipe help for syntax.\n\r", ch);
      return;
    }
    if((recipe = get_recipe_data(atoi(arg))) == NULL)
    {
      send_to_char("That recipe # does not exist yet. Use recipe help for options.\n\r", ch);
      return;
    }
    i = 0;
    while(i < MAX_IN_RECIPE)
    {
      argument = one_argument(argument, arg);
      if(arg[0] == 0)
        break;
      if(!is_number(arg))
      {
        send_to_char("Ingredient vnums must be numbers.\n\r", ch);
        return;
      }
      ingredients[i] = atoi(arg);
      if(get_obj_index(ingredients[i]) == NULL)
      {
        sprintf(buf, "vnum %d for an ingredient does not exist.\n\r", ingredients[i]);
        send_to_char(buf, ch);
        return;
      }
      i++;
    }
    if(i <= 1)
    {
      send_to_char("A recipe must have at least two ingredients.\n\r", ch);
      return;
    }
    if(i == MAX_IN_RECIPE)
    {
      send_to_char("Too many ingredients for one recipe.  You might use two recipes instead?\n\r", ch);
      return;
    }
    ingredients[i] = 0;/* Cap it off */
    for(i = 0; i < MAX_IN_RECIPE; i++)
    {
      recipe->vnum_parts[i] = ingredients[i];
      if(ingredients[i] == 0)
        break;
    }
    send_to_char("Ingredients updated.\n\r", ch);
    save_recipes();
    return;
  }
  if(!str_prefix(arg, "activate"))
  {
    RECIPE_DATA *recipe = NULL;
    argument = one_argument(argument, arg);
    if(arg[0] == 0)
    {
      send_to_char("Usage: recipe activate <recipe #>\n\r", ch);
      return;
    }
    if(!is_number(arg))
    {
      send_to_char("Recipe # must be a number. Use recipe help for syntax.\n\r", ch);
      return;
    }
    if((recipe = get_recipe_data(atoi(arg))) == NULL)
    {
      send_to_char("That recipe # does not exist yet. Use recipe help for options.\n\r", ch);
      return;
    }
    if(recipe->active)
    {
      send_to_char("Recipe deactivated.\n\r", ch);
    }
    else
    {
      if(recipe->vnum_parts[0] == 0 || recipe->vnum_parts[1] == 0)
      {
        send_to_char("That recipe does not have enough ingredients to be activated.\n\r", ch);
        return;
      }
      send_to_char("Recipe activated.\n\r", ch);
    }
    recipe->active = !recipe->active;
    save_recipes();
    return;
  }
  if(!str_prefix(arg, "cmessage") || !str_prefix(arg, "rmessage"))
  {
    bool room;
    RECIPE_DATA *recipe = NULL;
    if(arg[0] == 'c')
      room = FALSE;
    else
      room = TRUE;
    argument = one_argument(argument, arg);
    if(arg[0] == 0)
    {
      if(room)
        send_to_char("Usage: recipe rmessage <recipe #> What to show the room.\n\r", ch);
      else
        send_to_char("Usage: recipe cmessage <recipe #> What to show the user.\n\r", ch);
      send_to_char("Use the message 'clear' to erase the message.\n\r", ch);
      return;
    }
    if(!is_number(arg))
    {
      send_to_char("Recipe # must be a number. Use recipe help for syntax.\n\r", ch);
      return;
    }
    if((recipe = get_recipe_data(atoi(arg))) == NULL)
    {
      send_to_char("That recipe # does not exist yet. Use recipe help for options.\n\r", ch);
      return;
    }
    if(argument[0] == 0)
    {
      send_to_char("You must include a message.  Use 'clear' to erase.\n\r", ch);
      return;
    }
    if(!str_cmp(argument, "clear"))
    {
      if(room)
        clear_string(&recipe->rmessage, NULL);
      else
        clear_string(&recipe->cmessage, NULL);
    }
    else
    {
      if(room)
        clear_string(&recipe->rmessage, argument);
      else
        clear_string(&recipe->cmessage, argument);
    }
    if(room)
      send_to_char("The message the room will see has been updated.\n\r", ch);
    else
      send_to_char("The message the user will see has been updated.\n\r", ch);
    save_recipes();
    return;
  }
  /* Can't actually free the recipes due to alloc_perm, may as well let them leak like everything else in OLC */
  /*if(!str_prefix(arg, "delete"))
  {
    RECIPE_DATA *recipe;
    RECIPE_DATA *walker;
    if(get_trust(ch) < CREATOR)
    {
      send_to_char("You do not have the authority to delete recipes.\n\r", ch);
      return;
    }
    argument = one_argument(argument, arg);
    if(!is_number(arg))
    {
      send_to_char("Recipe vnum must be a number.\n\r", ch);
      return;
    }
    if((recipe = get_recipe_data(atoi(arg))) == NULL)
    {
      send_to_char("There is no recipe with that vnum to delete.\n\r", ch);
      return;
    }
    sprintf(buf, "detail %d", recipe->recipe_num);
    // Present them the details of the recipe they are deleting so they can recover if it is a mistake
    do_recipe(ch, buf);
    if(recipe_first == recipe)
    {
      recipe_first = recipe_first->next;
      //GC_FREE(recipe);
    }
    else
    {
      walker = recipe_first;
      while(walker->next && walker->next != recipe)
        walker = walker->next;
      if(!walker->next)
      {
        send_to_char("Error deleting recipe.  Please tell an admin.\n\r", ch);
        return;
      }
      if(walker->next == recipe_last)
        recipe_last = walker;
      else
        walker->next = recipe->next;
      //GC_FREE(recipe);
    }
    recipe = NULL;
    send_to_char("Recipe has been deleted.\n\r", ch);
    top_recipe--;
    save_recipes();
    return;
  }*/
  send_to_char("Commands: List, Detail, Create, Edit, Ingr, Activate, CMessage, RMessage, Help.\n\r", ch);
}

// New help code
bool keyword_check(CHAR_DATA *ch, char *argument, HELP_DATA *pHelp)
{
  char arg[255];
  int track;
  HELP_TRACKER *pTrack;
  bool found = FALSE;
  argument = one_argument(argument, arg);
  while(arg[0])
  {
    if(UPPER(arg[0]) >= 'A' && UPPER(arg[0]) <= 'Z')
      track = UPPER(arg[0]) - 'A' + 1;
    else
      track = 0;
    pTrack = help_tracks[track];
    for(; pTrack && pTrack != help_tracks[track + 1]; pTrack = pTrack->next)
    {
      if(pTrack->help != pHelp && !str_cmp(arg, pTrack->keyword))
      {
        found = TRUE;
        break;
      }
    }
    if(found)
    {
      send_to_char("Keyword '", ch);
      send_to_char(pTrack->keyword, ch);
      send_to_char("' is already in use.\n\r", ch);
      return FALSE;
    }
    argument = one_argument(argument, arg);
  }
  return TRUE;
}

void related_check(CHAR_DATA *ch, char *argument)
{
  char arg[255];
  int track;
  HELP_TRACKER *pTrack;
  bool found = FALSE;
  argument = one_argument(argument, arg);
  if(!str_cmp(arg, "none"))
    return;
  while(arg[0])
  {
    if(UPPER(arg[0]) >= 'A' && UPPER(arg[0]) <= 'Z')
      track = UPPER(arg[0]) - 'A' + 1;
    else
      track = 0;
    pTrack = help_tracks[track];
    for(; pTrack && pTrack != help_tracks[track + 1]; pTrack = pTrack->next)
    {
      if(!str_cmp(arg, pTrack->keyword))
      {
        found = TRUE;
        break;
      }
    }
    if(!found)
    {
      send_to_char("Warning: Related '", ch);
      send_to_char(arg, ch);
      send_to_char("' is not a keyword for any help file.\n\r", ch);
    }
    argument = one_argument(argument, arg);
  }
}

void unlink_help_trackers(HELP_DATA *pHelp)
{
  int i;
  HELP_TRACKER *pTrack_next, *pTrack;
  // Clear out the list heads first so we don't have to re-check those
  for(i = 0; i < 27; i++)
  {
    while(help_tracks[i] && help_tracks[i]->help == pHelp)
    {// in case multiple at the start of the category point here
      pTrack = help_tracks[i];
      help_tracks[i] = pTrack->next;
      if(pTrack->prev)
        pTrack->prev->next = pTrack->next;
      if(pTrack->next)
        pTrack->next->prev = pTrack->prev;
      clear_string(&pTrack->keyword, NULL);
      GC_FREE(pTrack);
    }
  }
  for(pTrack = help_track_first; pTrack; pTrack = pTrack_next)
  {
    pTrack_next = pTrack->next;
    if(pTrack->help == pHelp)
    {
      if(pTrack->prev)
        pTrack->prev->next = pTrack->next;
      if(pTrack->next)
        pTrack->next->prev = pTrack->prev;
      clear_string(&pTrack->keyword, NULL);
      GC_FREE(pTrack);
    }
  }
}

void do_edithelp(CHAR_DATA *ch, char *argument)
{
  char buf[255], arg[255];
  char *words;
  HELP_DATA *pHelp;
  HELP_TRACKER *pTrack;
  int track = 0;
  bool found = FALSE;
  if(IS_NPC(ch))
  {
    send_to_char("Switch back before editing a help file.\n\r", ch);
    return;
  }
  argument = one_argument(argument, arg);
  if(!arg[0])
  {
    send_to_char("Please see 'help ehelp' for commands available.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "new"))
  {
    if(ch->pcdata->ref_help)
    {
      if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
      {
        send_to_char("Your current help file has been edited. ehelp save or discard first.\n\r", ch);
        return;
      }
    }
    if(ch->pcdata->ref_help)
      ch->pcdata->ref_help->status &= HELP_STAT_BASICS;
    ch->pcdata->ref_help = &ch->pcdata->edit_help;
    ch->pcdata->edit_help.level = 0;
    ch->pcdata->edit_help.keyword = NULL;
    ch->pcdata->edit_help.text = NULL;
    ch->pcdata->edit_help.related = NULL;
    ch->pcdata->edit_help.status = HELP_STAT_DRAFT | HELP_STAT_MODIFIED;
    ch->pcdata->edit_help.modified = 0;
    ch->pcdata->edit_help.editor = NULL;
    send_to_char("Now editing a new helpfile.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "edit"))
  {
    if(ch->pcdata->ref_help)
    {
      if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
      {
        send_to_char("Your current help file has been edited. ehelp save or discard first.\n\r", ch);
        return;
      }
    }
    // check for editing imotd, wizlist, or greeting -- those are 59+ only
    if(UPPER(argument[0]) >= 'A' && UPPER(argument[0]) <= 'Z')
      track = UPPER(argument[0]) - 'A' + 1;
    else
      track = 0;
    pTrack = help_tracks[track];
    for(; pTrack && pTrack != help_tracks[track + 1]; pTrack = pTrack->next)
    {
      if(pTrack->help->level <= get_trust(ch) &&
        !str_prefix(argument, pTrack->keyword))
      {
        found = TRUE;
        break;
      }
    }
    if(!found)
    {
      sprintf(buf, "No help file with keyword '%s' found. Use 'new' to create a new one.\n\r", argument);
      send_to_char(buf, ch);
      return;
    }

    if(ch->level < 59 && (!str_cmp(pTrack->keyword, "greeting")
      || !str_cmp(pTrack->keyword, "imotd") || !str_cmp(pTrack->keyword, "wizlist")))
      {
        send_to_char("You do not have the power to edit that help file.\n\r", ch);
        return;
      }    

    if(IS_SET(pTrack->help->status, HELP_STAT_EDITING))
    {
      send_to_char("That help file is being edited by someone else.\n\r", ch);
      return;
    }

    if(ch->pcdata->ref_help)
    {
      // Strip any working flags from the status of the previous one
      ch->pcdata->ref_help->status &= HELP_STAT_BASICS;
    }
    ch->pcdata->ref_help = pTrack->help;
    ch->pcdata->edit_help.level = pTrack->help->level;
    clear_string(&ch->pcdata->edit_help.keyword, pTrack->help->keyword);
    clear_string(&ch->pcdata->edit_help.text, pTrack->help->text);
    clear_string(&ch->pcdata->edit_help.related, pTrack->help->related);
    ch->pcdata->edit_help.status = pTrack->help->status;
    ch->pcdata->edit_help.modified = pTrack->help->modified;
    clear_string(&ch->pcdata->edit_help.editor, pTrack->help->editor);
    pTrack->help->status |= HELP_STAT_EDITING;//Being edited
    sprintf(buf, "Now editing %s\n\r", pTrack->help->keyword);
    send_to_char(buf, ch);
    return;
  }
  if(!str_prefix(arg, "list"))
  {// List all helpfiles that match the given keyword
   argument = one_argument(argument, arg);
   if(!arg[0])
   {
    send_to_char("You need to enter at least a letter to list matching keywords.\n\r", ch);
    return;
   }
   send_to_char("All keywords beginning with '", ch);
   send_to_char(arg, ch);
   send_to_char("':\n\r", ch);
   for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
   {
      if ( pHelp->level > get_trust( ch ) )
         continue;

      if ( is_name(arg, pHelp->keyword ) )
      {
        sprintf(buf, "%d %s\n\r", pHelp->level, pHelp->keyword);
        send_to_char(buf, ch);
      }
    }
    return;
  }
  if(!str_prefix(arg, "editor"))
  {// List all helpfiles that match the given keyword
   argument = one_argument(argument, arg);
   if(!arg[0])
   {
    send_to_char("You need to enter an exact name to list helpfiles edited by.\n\r", ch);
    return;
   }
   for(track = 1; track < strlen(arg); track++)
    arg[track] = LOWER(arg[track]);
   send_to_char("All helpfiles edited by ", ch);
   send_to_char(arg, ch);
   send_to_char(":\n\r", ch);
   for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
   {
      if ( pHelp->level > get_trust( ch ) )
         continue;

      if ( !str_cmp(arg, pHelp->editor ) )
      {
        sprintf(buf, "%d %s\n\r", pHelp->level, pHelp->keyword);
        send_to_char(buf, ch);
      }
    }
    return;
  }
  if(!str_prefix(arg, "sort"))
  {// List all keywords of a given status (legacy, draft, review, final)
    argument = one_argument(argument, arg);
    if(!arg[0])
    {
     send_to_char("Sort by which status? (Legacy, draft, review, or final)\n\r", ch);
     return;
    }
    if(!str_prefix(arg, "legacy"))
    {
      track = HELP_STAT_LEGACY;
      send_to_char("All helpfiles with status Legacy:\n\r", ch);
    }
    else if(!str_prefix(arg, "draft"))
    {
      track = HELP_STAT_DRAFT;
      send_to_char("All helpfiles with status Draft:\n\r", ch);
    }
    else if(!str_prefix(arg, "review"))
    {
      track = HELP_STAT_REVIEW;
      send_to_char("All helpfiles with status Review:\n\r", ch);
    }
    else if(!str_prefix(arg, "final"))
    {
      track = HELP_STAT_FINAL;
      send_to_char("All helpfiles with status Final:\n\r", ch);
    }
    else
    {
     send_to_char("Sort by which status? (Legacy, draft, review, or final)\n\r", ch);
     return;
    }
    for ( pHelp = help_first; pHelp != NULL; pHelp = pHelp->next )
    {
      if ( pHelp->level > get_trust( ch ) )
         continue;

      if (IS_SET(pHelp->status, track))
      {
        sprintf(buf, "%d %s\n\r", pHelp->level, pHelp->keyword);
        send_to_char(buf, ch);
      }
    }
    return;
  }
  if(!ch->pcdata->ref_help)
  {
    send_to_char("Please see 'help ehelp' for commands available and when each may be used.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "show"))
  {// Show current helpfile
    format_help_to_char(ch, &ch->pcdata->edit_help);
    return;
  }
  if(!str_prefix(arg, "details"))
  {// Show current helpfile's stats (editor, last edited, etc.
    sprintf(buf, "Help file level is %d\n\r", ch->pcdata->edit_help.level);
    send_to_char(buf, ch);
    if(!ch->pcdata->edit_help.keyword)
      send_to_char("There are no keywords defined for this help.\n\r", ch);
    else
    {
      send_to_char("Keywords: {W", ch);
      send_to_char(ch->pcdata->edit_help.keyword, ch);
      send_to_char("{x\n\r", ch);
    }
    if(!ch->pcdata->edit_help.related)
      send_to_char("There are no related words defined for this help.\n\r", ch);
    else
    {
      send_to_char("Related: {C", ch);
      send_to_char(ch->pcdata->edit_help.related, ch);
      send_to_char("{x\n\r", ch);
    }
    switch(ch->pcdata->edit_help.status & HELP_STAT_BASICS)
    {
      case HELP_STAT_LEGACY: send_to_char("Current status is Legacy.\n\r", ch);
        break;
      case HELP_STAT_DRAFT: send_to_char("Current status is Draft.\n\r", ch);
        break;
      case HELP_STAT_REVIEW: send_to_char("Current status is Review.\n\r", ch);
        break;
      case HELP_STAT_FINAL: send_to_char("Current status is Final.\n\r", ch);
        break;
      default: send_to_char("Current status is unknown.\n\r", ch); break;
    }
    if(ch->pcdata->ref_help == &ch->pcdata->edit_help)
      send_to_char("This help file is new and has not been saved yet.\n\r", ch);
    else
    {
      /*if(!ch->pcdata->edit_help.modified)
        send_to_char("This help file has not been modified.\n\r", ch);
      else*/
      {
        send_to_char("Last modified on ", ch);
        send_to_char((char *) ctime( &current_time ), ch);
        send_to_char("by ", ch);
        send_to_char(ch->pcdata->edit_help.editor, ch);
        send_to_char("\n\r", ch);
      }
    }
    return;
  }
  if(!str_prefix(arg, "text"))
  {// Set the help file's text -- launch the editor
    do_line_editor (ch,ch->pcdata->edit_help.text,&edit_help_file);
    ch->pcdata->edit_help.status |= HELP_STAT_MODIFIED;
    return;
  }
  if(!str_prefix(arg, "level"))
  {// Set the help file's visible level
    argument = one_argument(argument, arg);
    if(!arg[0] || !is_number(arg) || ((track = atoi(arg)) < -1 || track > 60))
    {
      send_to_char("Level must be a number from -1 to 60.\n\r", ch);
      return;
    }
    if(track == ch->pcdata->edit_help.level)
    {
      send_to_char("The help file's level is already ", ch);
      send_to_char(arg, ch);
      send_to_char("\n\r", ch);
      return;
    }
    if(!str_cmp(ch->pcdata->ref_help->keyword, "greeting") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "imotd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "motd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "wizlist"))
      {
        send_to_char("You may not change the level for this help file.\n\r", ch);
        return;
      }
    send_to_char("The help file's level is now ", ch);
    send_to_char(arg, ch);
    send_to_char("\n\r", ch);
    ch->pcdata->edit_help.level = track;
    ch->pcdata->edit_help.status |= HELP_STAT_MODIFIED;
    return;
  }
  if(!str_prefix(arg, "keys") || !str_prefix(arg, "keywords"))
  {// Set the help file's keywords
    if(!argument[0])
    {
      send_to_char("You need to include a keyword to set for this file.\n\r", ch);
      return;
    }
    if(!str_cmp(argument, ch->pcdata->edit_help.keyword))
    {
      send_to_char("The help file already has that set of keywords.\n\r", ch);
      return;
    }
    if(!str_cmp(ch->pcdata->ref_help->keyword, "greeting") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "imotd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "motd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "wizlist"))
      {
        send_to_char("You may not set keywords for this help file.\n\r", ch);
        return;
      }
    if(!keyword_check(ch, argument, ch->pcdata->ref_help))
      return;
    for(track = 0; track < strlen(argument); track++)
      argument[track] = UPPER(argument[track]);
    clear_string(&ch->pcdata->edit_help.keyword, argument);
    send_to_char("Keywords are now: ", ch);
    send_to_char(ch->pcdata->edit_help.keyword, ch);
    send_to_char("\n\r", ch);
    ch->pcdata->edit_help.status |= HELP_STAT_MODIFIED;
    return;
  }
  if(!str_prefix(arg, "status"))
  {// Set the help file's status (draft, review, or final)
    argument = one_argument(argument, arg);
    if(!arg[0])
    {
      send_to_char("That is not a valid status. Options are: Draft, review, or final.\n\r", ch);
      return;
    }
    if(!str_prefix(arg, "draft"))
    {
      if(!str_cmp(ch->pcdata->ref_help->keyword, "greeting") ||
        !str_cmp(ch->pcdata->ref_help->keyword, "imotd") ||
        !str_cmp(ch->pcdata->ref_help->keyword, "motd") ||
        !str_cmp(ch->pcdata->ref_help->keyword, "wizlist"))
        {
          send_to_char("You may not set this help file's status to Draft.\n\r", ch);
          return;
        }
      track = HELP_STAT_DRAFT;
      words = "Draft";
    }
    else if(!str_prefix(arg, "review"))
    {
      track = HELP_STAT_REVIEW;
      words = "Review";
    }
    else if(!str_prefix(arg, "final"))
    {
      track = HELP_STAT_FINAL;
      words = "Final";
      if(ch->level < 58)
      {
        send_to_char("You are not high enough level to set a helpfile to final.\n\r", ch);
        return;
      }
    }
    else
    {
      send_to_char("That is not a valid status. Options are: Draft, review, or final.\n\r", ch);
      return;
    }
    if(IS_SET(ch->pcdata->edit_help.status, track))
    {
      send_to_char("The status is already ", ch);
      send_to_char(words, ch);
      send_to_char("\n\r", ch);
      return; 
    }
    ch->pcdata->edit_help.status &= HELP_STAT_EXTRAS;
    ch->pcdata->edit_help.status |= track;
    send_to_char("The status is now ", ch);
    send_to_char(words, ch);
    send_to_char("\n\r", ch);
    ch->pcdata->edit_help.status |= HELP_STAT_MODIFIED;
    return;
  }
  if(!str_prefix(arg, "related") || !str_prefix(arg, "also"))
  {// Set the help file's related terms
    if(!argument[0])
    {
      send_to_char("Use 'none' if you want to clear the related list.\n\r", ch);
      return;
    }
    if(!str_cmp(argument, ch->pcdata->edit_help.related))
    {
      send_to_char("The help file already has that set of related words.\n\r", ch);
      return; 
    }
    if(!str_cmp(ch->pcdata->ref_help->keyword, "greeting") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "imotd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "motd") ||
      !str_cmp(ch->pcdata->ref_help->keyword, "wizlist"))
      {
        send_to_char("You may not set related words for this help file.\n\r", ch);
        return;
      }
    for(track = 0; track < strlen(argument); track++)
    {
      argument[track] = UPPER(argument[track]);
    }
    clear_string(&ch->pcdata->edit_help.related, argument);
    send_to_char("Related now set to: ", ch);
    send_to_char(argument, ch);
    send_to_char("\n\r", ch);
    related_check(ch, argument);
    ch->pcdata->edit_help.status |= HELP_STAT_MODIFIED;
    return;
  }
  if(!str_prefix(arg, "save"))
  {// Save current changes, remain editing
    if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
    {
      bool key_change = FALSE;
      if(!ch->pcdata->edit_help.keyword || !strlen(ch->pcdata->edit_help.keyword))
      {
        send_to_char("You must set at least one keyword before saving.\n\r", ch);
        return;
      }
      if(!keyword_check(ch, ch->pcdata->edit_help.keyword, ch->pcdata->ref_help))
      {
        send_to_char("You must modify the keywords before saving.\n\r", ch);
        return;
      }
      if(ch->pcdata->edit_help.related)
        related_check(ch, ch->pcdata->edit_help.related);
      else
        clear_string(&ch->pcdata->edit_help.related, "none"); 
      
      if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_LEGACY))
      {// Legacy turns into Review on first save -- so it will go live
        ch->pcdata->edit_help.status &= HELP_STAT_EXTRAS;
        ch->pcdata->edit_help.status |= HELP_STAT_REVIEW;
      }
      
      if(ch->pcdata->ref_help != &ch->pcdata->edit_help)
      {
        if(str_cmp(ch->pcdata->ref_help->keyword, ch->pcdata->edit_help.keyword))
        {// Keywords changed, clear all of the trackers pointing at this
          unlink_help_trackers(ch->pcdata->ref_help);
          key_change = TRUE;
        }
        pHelp = ch->pcdata->ref_help;
        pHelp->level = ch->pcdata->edit_help.level;
        clear_string(&pHelp->keyword, ch->pcdata->edit_help.keyword);
        clear_string(&pHelp->text, ch->pcdata->edit_help.text);
        clear_string(&pHelp->related, ch->pcdata->edit_help.related);
        pHelp->status = (ch->pcdata->edit_help.status & HELP_STAT_BASICS) | HELP_STAT_EDITING;
        pHelp->modified = current_time;
        clear_string(&pHelp->editor, ch->name);
      }
      else
      {
        key_change = TRUE;
        pHelp   = GC_MALLOC( sizeof(*pHelp) );
        ch->pcdata->ref_help = pHelp;
        pHelp->level = ch->pcdata->edit_help.level;
        clear_string(&pHelp->keyword, ch->pcdata->edit_help.keyword);
        clear_string(&pHelp->text, ch->pcdata->edit_help.text);
        clear_string(&pHelp->related, ch->pcdata->edit_help.related);
        pHelp->status = (ch->pcdata->edit_help.status & HELP_STAT_BASICS) | HELP_STAT_EDITING;
        pHelp->modified = current_time;
        clear_string(&pHelp->editor, ch->name);
        if(help_last)
          help_last->next = pHelp;
        else
          help_first = pHelp;
        help_last = pHelp;
        pHelp->next = NULL;
        top_help++;
      }
      // Save to disk so it won't be lost if there's a crash or anything
      write_new_helps();
      if(key_change)
      {// Assign keywords to their place in the tracking list
        words = one_argument(pHelp->keyword, arg);
        
        while(arg[0] != '\0')
        {// Break it down into the various keywords
          HELP_TRACKER *pWalker;
          pTrack = GC_MALLOC( sizeof(*pTrack) );
          pTrack->help = pHelp;
          clear_string(&pTrack->keyword, arg);
          pWalker = help_track_first;
          if(!help_track_first)
          {
            help_track_first = pTrack;
            // Categorize this first one based on its letter
            if(pTrack->prev && UPPER(pTrack->keyword[0]) >= 'A' && 
              UPPER(pTrack->keyword[0]) <= 'Z')
              help_tracks[UPPER(pTrack->keyword[0]) - 'A' + 1] = pTrack;
            else
              help_tracks[0] = pTrack;
          }
          else
          {
            while(pWalker->next && 
              alphabet_before(pWalker->keyword, pTrack->keyword))
              pWalker = pWalker->next;
            if(!pWalker->prev)
            {// First of the list
              pTrack->next = help_track_first;
              help_track_first->prev = pTrack;
              help_track_first = pTrack;
              help_tracks[0] = pTrack;// If it's first, it's first here too
            }
            else if(!pWalker->next)
            {// End of the list
              pTrack->prev = pWalker;
              pWalker->next = pTrack;
            }
            else
            {// Middle of the list -- needs to go in before this one
              pTrack->prev = pWalker->prev;
              pTrack->next = pWalker;
              pWalker->prev->next = pTrack;
              pWalker->prev = pTrack;
            }
            if(pTrack->prev && UPPER(pTrack->keyword[0]) >= 'A' && 
              UPPER(pTrack->keyword[0]) <= 'Z')
            {// It's not the first, if it was it would be assigned.
            // Also means it's not in 0, which simplifies this a little
              if(UPPER(pTrack->keyword[0]) != UPPER(pTrack->prev->keyword[0]))
              {// It's first for this letter 
                help_tracks[UPPER(pTrack->keyword[0]) - 'A' + 1] = pTrack;
              }
            }
          }
          words = one_argument(words, arg);
        }
      }
      switch(ch->pcdata->ref_help->status & HELP_STAT_BASICS)
      {
        case HELP_STAT_LEGACY: words = "Legacy"; break;
        case HELP_STAT_DRAFT: words = "Draft"; break;
        case HELP_STAT_REVIEW: words = "Review"; break;
        case HELP_STAT_FINAL: words = "Final"; break;
        default: words = "Unknown"; break;
      }
      sprintf(arg, "Help file has been saved with status %s at level %d.\n\r", 
        words, ch->pcdata->ref_help->level);
      send_to_char(arg, ch);
      ch->pcdata->edit_help.status = pHelp->status & HELP_STAT_BASICS;
    }
    else
      send_to_char("You have made no changes to save.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "discard"))
  {// Discard changes, reset to base help (If not new)
    if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
    {
      if(ch->pcdata->ref_help != &ch->pcdata->edit_help)
      {
        pHelp = ch->pcdata->ref_help;
        ch->pcdata->edit_help.level = pHelp->level;
        clear_string(&ch->pcdata->edit_help.keyword, pHelp->keyword);
        clear_string(&ch->pcdata->edit_help.text, pHelp->text);
        clear_string(&ch->pcdata->edit_help.related, pHelp->related);
        ch->pcdata->edit_help.status = pHelp->status & HELP_STAT_BASICS;
        ch->pcdata->edit_help.modified = pHelp->modified;
        clear_string(&ch->pcdata->edit_help.editor, pHelp->editor);
        send_to_char("Discarding changes and reloading from the original help file.\n\r", ch);
      }
      else
      {
        send_to_char("Discarding new helpfile. Exiting edithelp.\n\r", ch);
        ch->pcdata->ref_help = NULL;
      }
    }
    else
      send_to_char("You have made no changes to discard.\n\r", ch);
    return;
  }
  if(!str_prefix(arg, "delete"))
  {// Make sure they've entered at least one of its full keywords
    if(ch->pcdata->ref_help == &ch->pcdata->edit_help)
    {
      send_to_char("You have not saved that help file, just discard it if you want it gone.\n\r", ch);
      return;
    }
    if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
    {
      send_to_char("You have edited this help file, discard the changes before deleting it.\n\r", ch);
      return;
    }
    if(ch->level < 58)
    {
      send_to_char("You do not have the power to delete a helpfile.\n\r", ch);
      return;
    }
    pHelp = ch->pcdata->ref_help;
    if(!is_name(argument, pHelp->keyword))
    {
      send_to_char("You must include one keyword for the help file to delete it.\n\r", ch);
      return;
    }
    if(is_name("greeting", pHelp->keyword) ||
      is_name("imotd", pHelp->keyword) || is_name("motd", pHelp->keyword) ||
      is_name("wizlist", pHelp->keyword))
    {
      send_to_char("You may not delete this help file.\n\r", ch);
      return;
    }
    unlink_help_trackers(pHelp);
    // Search through the help files and unlink this one
    if(ch->pcdata->ref_help == help_first)
    {
      help_first = help_first->next;
      top_help--;
    }
    else
    {
      for(pHelp = help_first; pHelp->next && pHelp->next != ch->pcdata->ref_help; pHelp = pHelp->next);
      if(!pHelp->next)// This would be a big surprise
      {
        bug("Unable to find help in list to remove.\n\r", 0);
      }
      else
      {
        pHelp->next = pHelp->next->next;
        top_help--;
      }
    }
    pHelp = ch->pcdata->ref_help;
    send_to_char("Deleting ", ch);
    send_to_char(pHelp->keyword, ch);
    send_to_char("\n\r", ch);
    clear_string(&pHelp->keyword, NULL);
    clear_string(&pHelp->text, NULL);
    clear_string(&pHelp->related, NULL);
    clear_string(&pHelp->editor, NULL);
    GC_FREE(pHelp);
    ch->pcdata->ref_help = NULL;
    write_new_helps();
    return;
  }
  if(!str_prefix(arg, "done") || !str_prefix(arg, "stop") || !str_prefix(arg, "quit") || !str_prefix(arg, "end"))
  {// Stop editing a help file -- clean up data
    if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
    {
      send_to_char("Your current help file has been edited. ehelp save or discard first.\n\r", ch);
      return;
    }
    // Remove the editing flag in case it's an existing helpfile
    ch->pcdata->ref_help->status &= HELP_STAT_BASICS;
    ch->pcdata->ref_help = NULL;
    send_to_char("Exiting edithelp.\n\r", ch);
    // Any other cleanup is done at extract_char or when new editing begins
    return;
  }
  // Error in using -- refer to help file
  send_to_char("Invalid ehelp option.  Please see 'help ehelp' for valid commands.\n\r", ch);
}

void edit_help_file (CHAR_DATA *ch, char *buf)
{
  clear_string(&ch->pcdata->edit_help.text, buf);
}
#endif

void olc_log_string (CHAR_DATA *ch,char *str)
{
    FILE *fp;
    char *strtime;
    char buf[MAX_STRING_LENGTH];

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';

    if ( str[0] == '\0' ) return;

//    fclose( fpReserve );
    if ( ( fp = fopen( OLC_LOG_FILE, "a" ) ) == NULL )
    {
  perror( OLC_LOG_FILE );
    }
    else
    {
  fprintf(fp, "%s::%s -%s\n",strtime,ch->name,str);
  sprintf(buf, "OLC %s - %s",ch->name,str);
  wiznet (buf,ch,NULL,WIZ_OLC,0,get_trust (ch));
  fclose( fp );
    }

//    fpReserve = fopen( NULL_FILE, "r" );
    return;


}

bool check_range (CHAR_DATA *ch,int range_type,int vnum)
{
  VNUM_RANGE_DATA *range;
  char *type = "unknown";

  /* if (get_trust(ch) >= CREATOR) {
    return TRUE;
  }  */
  if (vnum == -1) {
    switch (range_type) {
      case RANGE_ROOM:
        if (ch->pcdata->edit.room)
          vnum = ch->pcdata->edit.room->vnum;
        type = "room";
        break;
      case RANGE_MOB:
        if (ch->pcdata->edit.mob)
          vnum = ch->pcdata->edit.mob->vnum;
        type = "mob";
        break;
      case RANGE_OBJ:
        if (ch->pcdata->edit.obj)
          vnum = ch->pcdata->edit.obj->vnum;
        type = "object";
        break;
      case RANGE_AREA:
        if (ch->pcdata->edit.area)
          vnum = get_area_min_vnum (ch->pcdata->edit.area);
        type = "area";
        break;
    }
  }

  range = ch->pcdata->edit.range;
  while (range) {
    if ((vnum >= range->min) && (vnum <= range->max)) {
      return TRUE;
    }
    range = range->next;
  }

  send_to_char ("You do not have permission.\n\r>  ",ch);
  return FALSE;
}

void build_flag_menu ( char **flag_table, char *title, CHAR_DATA *ch )
{
  int idx,count;
  int t;
  MENU_DATA *flag_menu;

  count = 0;
  for (t = 0; t < 50; t++) {          /* count flag items */
    if (!flag_table[t]) break;
    if (strcmp (flag_table[t],"NA") != NULL)
      count++;
  }

  flag_menu = alloc_mem (sizeof(MENU_ITEM)*(count+3));
  if (title) {
    (*flag_menu)[0].text = str_dup (title);
  } else {
    (*flag_menu)[0].text = str_dup ("Set Flags");
  }
  (*flag_menu)[0].menu_fun = edit_flags_init;
  (*flag_menu)[0].context = "";
  if (count > 8)
    (*flag_menu)[0].id = 30;
  else
    (*flag_menu)[0].id = 0;
  idx = 1;
  for ( t = 0;; t++ )
  {
    char buf[MAX_STRING_LENGTH];

    if (!flag_table[t] || (idx > 45)) break;
    if (str_cmp(flag_table[t],"NA")) {
      sprintf (buf,"Toggle Flag [%s]",flag_table[t]);
      (*flag_menu)[idx].text = str_dup (buf);
      (*flag_menu)[idx].context = flag_table[t];
      (*flag_menu)[idx].id = 1 << t;
      (*flag_menu)[idx].menu_fun = edit_flags;
      idx++;
    }
  }
  (*flag_menu)[idx].text = str_dup ("[Done] Setting Flags");
  (*flag_menu)[idx].context = "done";
  (*flag_menu)[idx].id = ID_EDIT_DONE;
  (*flag_menu)[idx].menu_fun = edit_flags;
  idx++;
  (*flag_menu)[idx].text = NULL;
  (*flag_menu)[idx].menu_fun = NULL;

  ch->pcdata->edit.prev_menu = ch->pcdata->menu;
  ch->pcdata->menu = flag_menu;
  ch->pcdata->edit.flag_table = flag_table;

}

void destroy_flag_menu ( CHAR_DATA *ch )
{
  int t,count;
  MENU_DATA *flag_menu;

  flag_menu = ch->pcdata->menu;
  count = 0;
  for ( t = 0; t < 50; t++) {
    count++;
    if (!(*flag_menu)[t].text) break;
    free_string ( (*flag_menu)[t].text );
  }

  free_mem (flag_menu,sizeof (MENU_ITEM)*(count));
  ch->pcdata->menu = ch->pcdata->edit.prev_menu;
}

void build_spec_menu ( CHAR_DATA *ch )
{
  int count;
  int t;
  static MENU_DATA *spec_menu = NULL;

  if (!spec_menu) {
    count = 0;
    while (spec_table[count].name) {
      count++;
    }

    spec_menu = alloc_mem (sizeof(MENU_ITEM)*(count+4));
    (*spec_menu)[0].text = "Select Special";

    (*spec_menu)[0].menu_fun = edit_mob_spec_init;
    (*spec_menu)[0].context = "";
    (*spec_menu)[0].id = 30;

    for ( t = 0; t < count; t++ )
    {
      char buf[MAX_STRING_LENGTH];

      sprintf (buf,"%sSet [%s]",t < 9 ? " ":"",spec_table[t].name);
      (*spec_menu)[t+1].text = str_dup (buf);
      (*spec_menu)[t+1].context = spec_table[t].name;
      (*spec_menu)[t+1].id = t;
      (*spec_menu)[t+1].menu_fun = edit_mob_spec;
    }
    (*spec_menu)[count+1].text = "[Cancel] Selection";
    (*spec_menu)[count+1].context = "cancel";
    (*spec_menu)[count+1].id = ID_EDIT_DONE;
    (*spec_menu)[count+1].menu_fun = edit_mob_spec;
    (*spec_menu)[count+2].text = "[Remove] Special";
    (*spec_menu)[count+2].context = "remove";
    (*spec_menu)[count+2].id = ID_SPEC_NONE;
    (*spec_menu)[count+2].menu_fun = edit_mob_spec;
    (*spec_menu)[count+3].text = NULL;
    (*spec_menu)[count+3].menu_fun = NULL;
  }

  ch->pcdata->edit.prev_menu = ch->pcdata->menu;
  ch->pcdata->menu = spec_menu;
}

void build_race_menu ( CHAR_DATA *ch )
{
  int count;
  int t;
  static MENU_DATA *race_menu = NULL;

  if (!race_menu) {
    count = 0;
    while (race_table[count].name) {
      count++;
    }

    race_menu = alloc_mem (sizeof(MENU_ITEM)*(count+3));
    (*race_menu)[0].text = "Select a Race";

    (*race_menu)[0].menu_fun = edit_mob_race_init;
    (*race_menu)[0].context = "";
    (*race_menu)[0].id = 30;

    for ( t = 0; t < count; t++ )
    {
      char buf[MAX_STRING_LENGTH];

      sprintf (buf,"%sSet race to [%s]",t < 9 ? " ":"",race_table[t].name);
      (*race_menu)[t+1].text = str_dup (buf);
      (*race_menu)[t+1].context = race_table[t].name;
      (*race_menu)[t+1].id = t;
      (*race_menu)[t+1].menu_fun = edit_mob_race;
    }
    (*race_menu)[count+1].text = "[Cancel] Race Selection";
    (*race_menu)[count+1].context = "cancel";
    (*race_menu)[count+1].id = ID_EDIT_DONE;
    (*race_menu)[count+1].menu_fun = edit_mob_race;
    (*race_menu)[count+2].text = NULL;
    (*race_menu)[count+2].menu_fun = NULL;
  }

  ch->pcdata->edit.prev_menu = ch->pcdata->menu;
  ch->pcdata->menu = race_menu;
}

void build_average_menu ( char *title, CHAR_DATA *ch, MENU_FUN *call_back )
{
  int t;
  MENU_DATA *avg_menu;

  avg_menu = alloc_mem (sizeof(MENU_ITEM)*13);
  if (title) {
    (*avg_menu)[0].text = str_dup (title);
  } else {
    (*avg_menu)[0].text = str_dup ("Average Menu");
  }

  (*avg_menu)[0].menu_fun = call_back;
  (*avg_menu)[0].context = "";
  (*avg_menu)[0].id = 0;
  for ( t = 1; t <= 10; t++ )
  {
    char buf[MAX_STRING_LENGTH];

    sprintf (buf,"%s%s",(t < 10) ? " ":"",capitalize (avg_table[t]));
    (*avg_menu)[t].text = str_dup (buf);
    (*avg_menu)[t].context = avg_table[t];
    (*avg_menu)[t].id = t;
    (*avg_menu)[t].menu_fun = call_back;
  }
  (*avg_menu)[11].text = str_dup ("[Cancel]");
  (*avg_menu)[11].context = "cancel";
  (*avg_menu)[11].id = ID_EDIT_CANCEL;
  (*avg_menu)[11].menu_fun = call_back;
  (*avg_menu)[12].text = NULL;
  (*avg_menu)[12].menu_fun = NULL;

  ch->pcdata->edit.prev_menu = ch->pcdata->menu;
  ch->pcdata->menu = avg_menu;
}

void destroy_avg_menu (CHAR_DATA *ch)
{
  MENU_DATA *avg_menu;
  int t;

  avg_menu = ch->pcdata->menu;
  for ( t = 0; t <= 12; t++) {
    if (!(*avg_menu)[t].text) break;
    free_string ( (*avg_menu)[t].text );
  }

  free_mem (avg_menu,sizeof (MENU_ITEM)*13);
  ch->pcdata->menu = ch->pcdata->edit.prev_menu;
}

void build_attack_menu (CHAR_DATA *ch, MENU_FUN call_back)
{
  int t, count;
  MENU_DATA *att_menu;

  count = 0;
  for (t = 0;; t++) {
    if (!attack_table[t].name) break;
    count++;
  }
  count -= 2;
  att_menu = alloc_mem (sizeof(MENU_ITEM)*(count+3));

  (*att_menu)[0].text = str_dup ("Attack Type Menu");

  (*att_menu)[0].menu_fun = call_back;
  (*att_menu)[0].context = "";
  (*att_menu)[0].id = 20;
  for ( t = 1; t <= count; t++ )
  {
    char buf[MAX_STRING_LENGTH];

    sprintf (buf,"%s%s",(t < 10) ? " ":"",capitalize (attack_table[t].name));
    (*att_menu)[t].text = str_dup (buf);
    (*att_menu)[t].context = attack_table[t].name;
    (*att_menu)[t].id = t;
    (*att_menu)[t].menu_fun = call_back;
  }
  (*att_menu)[count+1].text = str_dup ("[Cancel]");
  (*att_menu)[count+1].context = "cancel";
  (*att_menu)[count+1].id = ID_EDIT_CANCEL;
  (*att_menu)[count+1].menu_fun = call_back;
  (*att_menu)[count+2].text = NULL;
  (*att_menu)[count+2].menu_fun = NULL;

  ch->pcdata->edit.prev_menu = ch->pcdata->menu;
  ch->pcdata->menu = att_menu;
}

void destroy_attack_menu (CHAR_DATA *ch)
{
  MENU_DATA *att_menu;
  int t,count;

  att_menu = ch->pcdata->menu;
  count = 0;
  for (t = 0;; t++) {
    if (!attack_table[t].name) break;
    count++;
  }
  count = 0;
  for ( t = 0; t <= (count+2); t++) {
    count++;
    if (!(*att_menu)[t].text) break;
    free_string ( (*att_menu)[t].text );
  }

  free_mem (att_menu,sizeof (MENU_ITEM)*count);
  ch->pcdata->menu = ch->pcdata->edit.prev_menu;
}

void show_flags ( int flags, char **flag_table, CHAR_DATA *ch)
{
  int t;
  int found = FALSE;
  char buf[MAX_STRING_LENGTH];

  for (t = 0;; t++) {
    if (!flag_table[t]) break;
    if (flags & (1 << t)) {
      if (found) send_to_char (", ",ch);
      if (!strcmp(flag_table[t],"NA")) {
        sprintf (buf,"Flag (%c)",(t <= 25) ? 'A'+t:'a'+(t-25));
        send_to_char (buf,ch);
      } else {
        send_to_char (capitalize (flag_table[t]),ch);
      }
      found = TRUE;
    }
  }
  if (!found) send_to_char ("None",ch);
}

void edit_flags_init ( CHAR_DATA *ch, int num )
{
  send_to_char (" - Flags:  ",ch);
  show_flags (*ch->pcdata->edit.mod_flags,ch->pcdata->edit.flag_table,ch);
  send_to_char ("\n\r\n\r",ch);
}

void edit_flags ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];
  int t;
  MENU_DATA *flag_menu;

  flag_menu = ch->pcdata->menu;

  if (num != ID_EDIT_DONE) {
    char *flag = "unknown";

    TOGGLE_BIT (*ch->pcdata->edit.mod_flags, num);
    for ( t = 1; t < 50; t++ ) {
      if ((*flag_menu)[t].text == NULL) {
        break;
      }
      if ((*flag_menu)[t].id == num) {
        flag = (*flag_menu)[t].context;
        break;
      }
    }
    sprintf (buf,"Flag [%s]:  %s\n\r>  ",flag,IS_SET(*ch->pcdata->edit.mod_flags,num) ?
                 "Set":"Unset");
    send_to_char (buf,ch);
  } else {
    destroy_flag_menu (ch);
    do_menu (ch, NULL);
  }
}

void edit_main        ( CHAR_DATA *ch, int num )
{
  switch (num) {
    case ID_EDIT_SETTINGS:
      ch->pcdata->menu = &settings_menu;
      break;
    case ID_EDIT_AREA:
      ch->pcdata->menu = &area_menu;
      break;
    case ID_EDIT_ROOM:
      ch->pcdata->menu = &room_menu;
      break;
    case ID_EDIT_MOB:
      ch->pcdata->menu = &mob_menu;
      break;
    case ID_EDIT_OBJECT:
      ch->pcdata->menu = &object_menu;
      break;
    case ID_EDIT_RESETS:
      ch->pcdata->menu = &reset_menu;
      break;
  }
  do_menu (ch,NULL);
}

void edit_settings ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_SETTINGS_DEF_ROOM:
      TOGGLE_BIT (ch->pcdata->edit.per_flags,EDIT_DEFAULT_ROOM);
      sprintf (buf,"Default to current room:  %s\n\r",IS_SET
              (ch->pcdata->edit.per_flags,EDIT_DEFAULT_ROOM) ? "Yes":"No");
      send_to_char (buf,ch);
      break;
    case ID_SETTINGS_DEF_OBJ:
      TOGGLE_BIT (ch->pcdata->edit.per_flags,EDIT_DEFAULT_OBJ);
      sprintf (buf,"Default to first object in inventory:  %s\n\r",IS_SET
              (ch->pcdata->edit.per_flags,EDIT_DEFAULT_OBJ) ? "Yes":"No");
      send_to_char (buf,ch);
      break;
    case ID_SETTINGS_DEF_MOB:
      TOGGLE_BIT (ch->pcdata->edit.per_flags,EDIT_DEFAULT_MOB);
      sprintf (buf,"Default to first mob in room:  %s\n\r",IS_SET
              (ch->pcdata->edit.per_flags,EDIT_DEFAULT_MOB) ? "Yes":"No");
      send_to_char (buf,ch);
      break;
    case ID_SETTINGS_AUTO:
      TOGGLE_BIT (ch->pcdata->edit.per_flags,EDIT_AUTO_CREATE);
      sprintf (buf,"Auto-create rooms when walking:  %s\n\r",IS_SET
              (ch->pcdata->edit.per_flags,EDIT_AUTO_CREATE) ? "Yes":"No");
      send_to_char (buf,ch);
      break;
    case ID_SETTINGS_DOOR:
      TOGGLE_BIT (ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR);
      send_to_char (IS_SET (ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR) ?
                    "Mode:  Double Sided Door\n\r":"Mode:  Single Sided Door\n\r",ch);
      break;
    case ID_SETTINGS_BRIEF:
      TOGGLE_BIT (ch->comm,COMM_BRIEF_MENUS);
      send_to_char (IS_SET (ch->comm,COMM_BRIEF_MENUS) ?
                    "Brief menus on.\n\r":"Brief menus off.\n\r",ch);
      break;
    case ID_SETTINGS_HELP:
      break;
  }
  send_to_char (">  ",ch);
}

void edit_area_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if ( ch->pcdata->edit.area )
    sprintf (buf," - Selected Area:  %s\n\r\n\r",ch->pcdata->edit.area->name);
  else
    sprintf (buf," - Selected Area:  None\n\r\n\r");
  send_to_char (buf,ch);
}

void edit_area_info ( CHAR_DATA *ch )
{
  char buf[MAX_STRING_LENGTH];
  AREA_DATA *area;

  area = ch->pcdata->edit.area;
  if (!area) {
    send_to_char ("Select an area first.\n\r",ch);
    return;
  }

  sprintf (buf,"Name:  %s\n\rFilename:  %s\n\r",area->name,area->file_name);
  send_to_char (buf,ch);
  sprintf (buf,"Under construction:  %s\n\r",area->under_develop ? "YES":"NO");
  send_to_char (buf,ch);
  sprintf (buf,"%-10s - Min Vnum: %5d     Max Vnum: %5d\n\r","Rooms",
           area->min_vnum_room,area->max_vnum_room);
  send_to_char (buf,ch);
  sprintf (buf,"%-10s - Min Vnum: %5d     Max Vnum: %5d\n\r","Mobs",
           area->min_vnum_mob,area->max_vnum_mob);
  send_to_char (buf,ch);
  sprintf (buf,"%-10s - Min Vnum: %5d     Max Vnum: %5d\n\r","Objects",
           area->min_vnum_obj,area->max_vnum_obj);
  send_to_char (buf,ch);
  return;
}


void edit_area_purge ( CHAR_DATA *ch )
{
  CHAR_DATA *victim;
  OBJ_DATA *obj;
  CHAR_DATA *vnext;
  OBJ_DATA  *obj_next;
  ROOM_INDEX_DATA *pRoom;
  sh_int vnum;

  if (!ch->pcdata->edit.area) {
    send_to_char ("Select an area first.\n\r>  ",ch);
    return;
  }

  for (vnum = ch->pcdata->edit.area->min_vnum_room;
       vnum <= ch->pcdata->edit.area->max_vnum_room;  vnum++) {

    pRoom = get_room_index (vnum);
    if (pRoom) {
      for ( victim = pRoom->people; victim != NULL; victim = vnext )
      {
        vnext = victim->next_in_room;
        if ( IS_NPC(victim) &&  victim != ch )
          extract_char( victim, TRUE );
      }

      for ( obj = pRoom->contents; obj != NULL; obj = obj_next )
      {
        obj_next = obj->next_content;
        extract_obj( obj );
      }
    }
  }

  send_to_char( "Area purged.\n\r",ch );
}

void edit_area_new_vnum (CHAR_DATA *ch, char *arg)
{
  int vnum;
  AREA_DATA *pArea;

  vnum = atoi (arg);
  if (vnum == 0) {
    send_to_char ("Invalid vnum.  Operation canceled.\n\r>  ",ch);
    /* Area is permanent.. no need to free. Let's just
       forget about it */
    ch->pcdata->edit.area = ch->pcdata->edit.room->area;
    ch->pcdata->interp_fun = do_menu;
  }
  pArea = ch->pcdata->edit.area;
  pArea->min_vnum_room =
  pArea->max_vnum_room =
  pArea->min_vnum_obj  =
  pArea->max_vnum_obj  =
  pArea->min_vnum_mob  =
  pArea->max_vnum_mob  = vnum;
  pArea->age      = 15;
  pArea->nplayer  = 0;
  pArea->empty    = FALSE;
  pArea->freeze   = TRUE;
  pArea->under_develop = TRUE;
  pArea->no_transport = FALSE;
  if ( area_first == NULL )
    area_first = pArea;
  if ( area_last  != NULL )
    area_last->next = pArea;
  area_last = pArea;
  pArea->next = NULL;
  top_area++;

  create_room (ch,NULL,0,TRUE);
  update_area_list (ch,pArea->file_name);
  save_area (ch,pArea);
  send_to_char ("Area created.\n\r>  ",ch);
  ch->pcdata->interp_fun = do_menu;
}

void edit_area_new_file (CHAR_DATA *ch, char *arg)
{
  char buf[MAX_STRING_LENGTH];
  char *ptr;
  FILE *fp;
  int idx;

  if (!arg[0]) {
    send_to_char ("Invalid filename.  Operation canceled.\n\r>  ",ch);
    /* no use de-allocing area, it's perm */
    ch->pcdata->edit.area = ch->pcdata->edit.room->area;
    ch->pcdata->interp_fun = do_menu;
    return;
  }
  idx = 0;
  while (arg[idx]) {
    arg[idx] = tolower (arg[idx]);
    idx++;
  }

  if ((ptr = strstr (arg,".are")) != NULL) {
    *ptr = NULL;
  }
  if (strlen (arg) > 8) {
    arg[8] = NULL;
  }
  strcat (arg,".are");
  sprintf (buf,"%s%s",NEW_DIR,arg);
  if ((fp = fopen (buf,"r")) != NULL) {
    fclose (fp);
    send_to_char ("Filename '%s' already exists.\n\r>  ",ch);
    /* no use de-allocing area, it's perm */
    ch->pcdata->edit.area = ch->pcdata->edit.room->area;
    ch->pcdata->interp_fun = do_menu;
    return;
  }

  ch->pcdata->edit.area->file_name = str_dup (buf);

  ch->pcdata->interp_fun = &edit_area_new_vnum;
  sprintf (buf,"Filename is '%s'\n\rEnter starting vnum:  ",
           ch->pcdata->edit.area->file_name);
  send_to_char (buf,ch);
}

void edit_area_new_name ( CHAR_DATA *ch, char *arg )
{
  char buf[MAX_STRING_LENGTH];
  AREA_DATA *pArea;

  pArea = new_area ();
  ch->pcdata->edit.area  = pArea;
  pArea->file_name       = NULL;
  sprintf (buf,"{ All }         %s",arg);
  pArea->credits         = str_dup (buf);
  pArea->name            = str_dup (arg);

  send_to_char ("Filename (8 characters):  ",ch);
  ch->pcdata->interp_fun = edit_area_new_file;
}

void edit_area_rename ( CHAR_DATA *ch, char *arg )
{
  AREA_DATA *area;
  char buf[MAX_STRING_LENGTH],
       buf2[MAX_STRING_LENGTH];

  if (strlen (arg) > 16) {
    area = ch->pcdata->edit.area;
    strcpy (buf2,area->name);
    ch->pcdata->interp_fun = do_menu;
    free_string (area->credits);
    area->credits = str_dup (arg);
    free_string (area->name);
    area->name    = str_dup (&arg[16]);
    send_to_char ("Area renamed.\n\r>  ",ch);

    sprintf (buf,"Renamed area %s to %s.",buf2,area->name);
    olc_log_string (ch,buf);
  } else {
    send_to_char ("String must be at lest 16 characters.\n\r>  ",ch);
  }
}

void edit_area_select ( CHAR_DATA *ch, char *arg )
{
  int vnum,min,max;
  AREA_DATA *area;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  if (is_number (arg)) {
    vnum = atoi (arg);
    area = area_first;
    while (area) {
      min = get_area_min_vnum (area);
      max = get_area_max_vnum (area);
      if ((vnum >= min) && (vnum <= max)) {
        ch->pcdata->edit.area = area;
        sprintf (buf,"Selected area '%s'.\n\r>  ",area->name);
        send_to_char (buf,ch);
        return;
      }
      area = area->next;
    }
  } else {
    send_to_char ("Operation cancelled.\n\r>  ",ch);
    return;
  }
  send_to_char ("Vnum not found within an area.\n\r>  ",ch);
}

void edit_area ( CHAR_DATA *ch, int num )
{
  extern void reset_area( AREA_DATA *pArea );
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_EDIT_AREA_SELECT:
      send_to_char ("Enter vnum within area:  ",ch);
      ch->pcdata->interp_fun = edit_area_select;
      return;
      break;
    case ID_EDIT_AREA_INFO:
      edit_area_info (ch);
      break;
    case ID_EDIT_AREA_NEW:
      if (get_trust (ch) < CREATOR) {
        send_to_char ("You do not have permision to create new areas.\n\r>  ",ch);
        return;
      }
      send_to_char ("Enter name of area:  ",ch);
      ch->pcdata->interp_fun = edit_area_new_name;
      return;
      break;
    case ID_EDIT_AREA_LOAD:
      break;
    case ID_EDIT_AREA_SAVE:
      if (!ch->pcdata->edit.area)
        send_to_char ("Select an area to save first.\n\r",ch);
      else {
        if (check_range (ch,RANGE_AREA,-1)) {
          save_area (ch,ch->pcdata->edit.area);
          send_to_char ("Area saved.\n\r",ch);
          sprintf (buf,"Saved area %s.",ch->pcdata->edit.area->name);
          olc_log_string (ch,buf);
        } else {
          return;
        }
      }
      break;
    case ID_EDIT_AREA_RENAME:
      if (check_range (ch,RANGE_AREA,-1)) {
        send_to_char ("             {-----} ------- -------------------\n\r",ch);
        send_to_char ("Enter name:  ",ch);
        ch->pcdata->interp_fun = edit_area_rename;
        return;
      }
      break;
    case ID_EDIT_AREA_RESET:
      if (check_range (ch,RANGE_AREA,-1)) {
        if (ch->pcdata->edit.room) {
          bool temp;

          temp = ch->pcdata->edit.area->freeze;
          ch->pcdata->edit.area->freeze = TRUE;
          reset_area (ch->pcdata->edit.area);
          ch->pcdata->edit.area->freeze = temp;
          send_to_char ("Area reset.\n\r",ch);
          sprintf (buf,"Reset area %s.",ch->pcdata->edit.area->name);
          olc_log_string (ch,buf);
        }
      } else {
        return;
      }
      break;
    case ID_EDIT_AREA_PURGE:
      if (check_range (ch,RANGE_AREA,-1)) {
        edit_area_purge (ch);
        sprintf (buf,"Purged area %s.",ch->pcdata->edit.area->name);
        olc_log_string (ch,buf);
      } else {
        return;
      }
      break;
    case ID_EDIT_AREA_FREEZE:
      if (!ch->pcdata->edit.area)
        send_to_char ("Select an area to freeze first.\n\r",ch);
      else {
        if (check_range (ch,RANGE_AREA,-1)) {
          ch->pcdata->edit.area->freeze = !ch->pcdata->edit.area->freeze;
          if (ch->pcdata->edit.area->freeze) {
            send_to_char ("Area frozen.\n\r",ch);
            sprintf (buf,"Froze area %s.",ch->pcdata->edit.area->name);
            olc_log_string (ch,buf);
          } else {
            send_to_char ("Area unfrozen.\n\r",ch);
            sprintf (buf,"Unfroze area %s.",ch->pcdata->edit.area->name);
            olc_log_string (ch,buf);
          }
        } else {
          return;
        }

      }
      break;
    case ID_EDIT_AREA_UNDER_DEV:
      if (!ch->pcdata->edit.area)
        send_to_char ("Select an area to toggle first.\n\r",ch);
      else {
        if (check_range (ch,RANGE_AREA,-1)) {
          ch->pcdata->edit.area->freeze = !ch->pcdata->edit.area->freeze;
          if (ch->pcdata->edit.area->freeze) {
            send_to_char ("Area marked under contruction.\n\r",ch);
            sprintf (buf,"Construction on for area %s.",ch->pcdata->edit.area->name);
            olc_log_string (ch,buf);
          } else {
            send_to_char ("Under construction removed.\n\r",ch);
            sprintf (buf,"Construction off for area %s.",ch->pcdata->edit.area->name);
            olc_log_string (ch,buf);
          }
        } else {
          return;
        }

      }
      break;
  }
  send_to_char (">  ",ch);
}

void edit_room_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if (ch->pcdata->edit.room)
    sprintf (buf," - Selected Room:  %s [%d]\n\r\n\r",
             ch->pcdata->edit.room->name, ch->pcdata->edit.room->vnum );
  else
    sprintf (buf," - Selected Room:  None\n\r\n\r");
  send_to_char (buf,ch);
}

void edit_room_vnum ( CHAR_DATA *ch, char *arg )
{
   ROOM_INDEX_DATA *room;
   int vnum;
   char buf[MAX_STRING_LENGTH];

   vnum = atoi (arg);
   room = get_room_index (vnum);
   if (!room) {
     sprintf (buf,"Invalid room vnum %d.\n\r>  ",vnum);
   } else {
     ch->pcdata->edit.room = room;
     sprintf (buf,"Selected Room:  %s [%d]\n\r>  ",room->name,vnum);
   }
   send_to_char (buf,ch);
   ch->pcdata->interp_fun = do_menu;
}

void edit_room_info ( CHAR_DATA *ch )
{
  ROOM_INDEX_DATA *room;
  char buf[MAX_STRING_LENGTH];
  int t;

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char ("No room selected.\n\r",ch);
    return;
  }
  sprintf (buf,"Room:  %s [%d]\n\r",room->name,room->vnum);
  send_to_char (buf,ch);
  sprintf (buf,"Area:  %s\n\r",room->area->name);
  send_to_char (buf,ch);
  send_to_char ("Description:\n\r",ch);
  send_to_char (room->description,ch);
  sprintf (buf,"\n\rLight: %s  Sector: %s  Heal Rate: %d  Mana Rate: %d\n\r",
           room->light ? "Yes":"No",room->sector_type < SECT_MAX  ?
           capitalize(sector_table[room->sector_type]):"Unknown",
           room->heal_rate,room->mana_rate);
  send_to_char (buf,ch);
  send_to_char ("Room flags:  ",ch);
  show_flags (room->room_flags,mod_room_flags,ch);
  send_to_char ("\n\r",ch);
  if (room->owner)
    sprintf (buf,"Owner:  %s\n\r",room->owner);
  send_to_char (buf,ch);
  for (t = 0; t < 6; t++ ) {
    EXIT_DATA *pexit;

    pexit = room->exit[t];
    if (pexit != NULL) {
      if (pexit->u1.to_room != NULL) {
        sprintf (buf,"%-5s - %s [%d]\n\r",capitalize(dir_table[t]),
                 pexit->u1.to_room->name,pexit->u1.to_room->vnum);
        send_to_char (buf,ch);
      }
    }
  }

  /* extra descrip */
  send_to_char (">  ",ch);
}

EXIT_DATA *create_exit ()
{
  EXIT_DATA *exit;

  exit             = new_exit ();
  exit->keyword    = str_dup ("door");
  exit->u1.to_room = NULL;
  exit->exit_info  = 0;
  exit->next       = NULL;
  exit->key        = 0;

  return exit;
}

bool create_room ( CHAR_DATA *ch, ROOM_INDEX_DATA *room, int dir, int move_char )
{
  int vnum,t;
  ROOM_INDEX_DATA *pRoomIndex;
  static ROOM_INDEX_DATA zero_room;
  int iHash,door;
  char buf[MAX_STRING_LENGTH];

  vnum = 0;
  for (t = ch->pcdata->edit.area->min_vnum_room;
       t <= (ch->pcdata->edit.area->max_vnum_room+1); t++) {
    if (!get_room_index (t)) {
      vnum = t;
      break;
    }
  }
  if (vnum == 0) {
    send_to_char ("Out of room vnums for this area.\n\r",ch);
    return FALSE;
  }

  if (!check_range (ch,RANGE_ROOM,vnum)) {
    send_to_char ("Room could not be created.\n\r",ch);
    return FALSE;
  }

  sprintf (buf,"Created room [%d].",vnum);
  olc_log_string (ch,buf);


  if (vnum >  ch->pcdata->edit.area->max_vnum_room)
    ch->pcdata->edit.area->max_vnum_room = vnum;
  pRoomIndex                      = alloc_perm(sizeof(*pRoomIndex));
  *pRoomIndex                     = zero_room;
  pRoomIndex->owner               = str_dup("");
  pRoomIndex->people              = NULL;
  pRoomIndex->contents            = NULL;
  pRoomIndex->extra_descr         = NULL;
  pRoomIndex->area                = ch->pcdata->edit.area;
  pRoomIndex->vnum                = vnum;
  pRoomIndex->name                = str_dup_perm ("Generic room name");
  pRoomIndex->description         = str_dup_perm ("Generic room description.\n\r");
  pRoomIndex->room_flags          = 0;
  pRoomIndex->sector_type         = ch->pcdata->edit.room->sector_type;
  pRoomIndex->light               = 0;
  pRoomIndex->clan                = 0;
  pRoomIndex->heal_rate           = 100;
  pRoomIndex->mana_rate           = 100;

  for ( door = 0; door < 6; door++ )
      pRoomIndex->exit[door] = NULL;

  iHash                   = vnum % MAX_KEY_HASH;
  pRoomIndex->next        = room_index_hash[iHash];
  room_index_hash[iHash]  = pRoomIndex;
  top_room++;

  if (room) {
      if (room->exit[dir] != NULL)
        free_exit (room->exit[dir]);
      room->exit[dir] = create_exit ();
      room->exit[dir]->u1.to_room = pRoomIndex;
      pRoomIndex->exit[rev_dir[dir]] = create_exit ();
      pRoomIndex->exit[rev_dir[dir]]->u1.to_room = room;
      sprintf (buf,"[%d] %s connected to [%d]\n\r[%d] %s connected to [%d]\n\r>  ",
               room->vnum,dir_table[dir], pRoomIndex->vnum,
               pRoomIndex->vnum,dir_table[rev_dir[dir]],room->vnum);
      send_to_char (buf,ch);
  }

  if (move_char) {
    char_from_room (ch);
    char_to_room (ch, pRoomIndex);
    ch->pcdata->edit.room = pRoomIndex;
  } else {
    do_menu (ch,NULL);
  }

  return TRUE;
}

void edit_room_create_dir (CHAR_DATA *ch, char *arg)
{
  int t;
  char buf[MAX_STRING_LENGTH];

  for ( t = 0; t < 6; t++ ) {
    if (!str_prefix (arg, dir_table[t])) {
      ch->pcdata->edit.exit = t;
      sprintf (buf,"Selected:  %s\n\r",dir_table[t]);
      send_to_char (buf,ch);
      ch->pcdata->interp_fun = do_menu;
      if (create_room (ch, ch->pcdata->edit.room, t, TRUE)) {
        sprintf (buf,"Room [%d] created.\n\r>  ",ch->pcdata->edit.room->vnum);
        send_to_char (buf,ch);
      } else {
        send_to_char ("\n\r>  ",ch);
      }
      return;
    }
  }

  if (!str_prefix (arg,"none")) {
    ch->pcdata->interp_fun = do_menu;
    send_to_char ("Selected:  None\n\r",ch);
    if (create_room (ch,NULL,0,TRUE)) {
      sprintf (buf,"Room [%d] created.\n\r>  ",ch->pcdata->edit.room->vnum);
      send_to_char (buf,ch);
    } else {
      send_to_char ("\n\r>  ",ch);
    }

    return;
  }

  send_to_char ("Invalid choice.\n\rWhich directiont [north,east,south,west,up,down,none]:  ",ch);
}

void edit_room_create (CHAR_DATA *ch)
{
  char buf[MAX_STRING_LENGTH];

  if (ch->pcdata->edit.room) {
    ch->pcdata->interp_fun = edit_room_create_dir;
    send_to_char ("Which direction [north,east,south,west,up,down,none]:  ",ch);
    return;
  } else {
    if (create_room ( ch, NULL, 0, TRUE )) {
      sprintf (buf,"Room [%d] created.\n\r>  ",ch->pcdata->edit.room->vnum);
      send_to_char (buf,ch);
    } else {
      send_to_char ("\n\r>  ",ch);
    }

    ch->pcdata->interp_fun = do_menu;
  }
}

void edit_room_clone ( CHAR_DATA *ch, char *arg )
{
  int vnum;
  ROOM_INDEX_DATA *src,*pRoomIndex;
  char buf[MAX_STRING_LENGTH];
  EXTRA_DESCR_DATA *ed_copy, *ed_last;

  vnum = atoi (arg);
  ch->pcdata->interp_fun = do_menu;
  src = get_room_index (vnum);
  if (!src) {
    sprintf (buf,"Room index %d does not exist.\n\r>  ",vnum);
    send_to_char (buf,ch);
    return;
  }

  pRoomIndex = ch->pcdata->edit.room;

  pRoomIndex->owner               = str_dup (src->owner);
  pRoomIndex->extra_descr = NULL;
  ed_last = NULL;
  for(ed_copy = src->extra_descr; ed_copy; ed_copy = ed_copy->next)
  {
    if(!pRoomIndex->extra_descr)
    {
      pRoomIndex->extra_descr = new_extra_descr();
      ed_last = pRoomIndex->extra_descr; 
    }
    else
    {
      ed_last->next = new_extra_descr();
      ed_last = ed_last->next;
    }
    ed_last->keyword = str_dup_perm(ed_copy->keyword);
    ed_last->description = str_dup_perm(ed_copy->description);
    ed_last->next = NULL;
  }
  pRoomIndex->name                = str_dup_perm (src->name);
  pRoomIndex->description         = str_dup_perm (src->description);
  pRoomIndex->room_flags          = src->room_flags;
  pRoomIndex->sector_type         = src->sector_type;
  pRoomIndex->light               = src->light;
  pRoomIndex->clan                = src->clan;
  pRoomIndex->heal_rate           = src->heal_rate;
  pRoomIndex->mana_rate           = src->mana_rate;
  send_to_char ("Ok.\n\r>  ",ch);

}

void edit_room_list (CHAR_DATA *ch)
{
  ROOM_INDEX_DATA *room;
  int idx;
  char buf[MAX_STRING_LENGTH],bigbuf[4*MAX_STRING_LENGTH];
  int col = FALSE;

  bigbuf[0] = NULL;
  for (idx = ch->pcdata->edit.area->min_vnum_room;
       idx <= ch->pcdata->edit.area->max_vnum_room; idx++) {
    if (strlen (bigbuf) > (4*MAX_STRING_LENGTH - 200)) {
      strcat (bigbuf,"\n\rWarning: Buffer Full\n\r");
      break;
    }
    room = get_room_index (idx);
    if (room) {
      sprintf (buf,"[%5d] %-30s",room->vnum,room->name);
      if (col) {
        col = FALSE;
        if (strlen (buf) > 38) {
          strcat (bigbuf,"\n\r");
        }
        strcat (bigbuf,buf);
      } else {
        col = TRUE;
        if (strlen (buf) > 38) {
          col = FALSE;
        }
        strcat (bigbuf,"\n\r");
        strcat (bigbuf,buf);
      }
    }
  }
  strcat (bigbuf,"\n\r>  ");
  page_to_char (bigbuf,ch);
}

void edit_room ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_EDIT_VNUM:
      send_to_char ("Enter new vnum:  ",ch);
      ch->pcdata->interp_fun = edit_room_vnum;
      break;
    case ID_EDIT_CREATE:
      if (check_range (ch,RANGE_ROOM,-1))
        edit_room_create (ch);
      break;
    case ID_EDIT_CLONE:
      if (check_range (ch,RANGE_ROOM,-1)) {
        send_to_char ("Enter vnum to copy from:  ",ch);
        ch->pcdata->interp_fun = edit_room_clone;
        sprintf (buf,"Copied to room [%d].",ch->pcdata->edit.room->vnum);
        olc_log_string (ch,buf);
      }
      break;
    case ID_EDIT_LIST:
      edit_room_list (ch);
      break;
    case ID_EDIT_MODIFY:
      if (check_range (ch,RANGE_ROOM,-1)) {
        sprintf (buf,"Modify room [%d].",ch->pcdata->edit.room->vnum);
        olc_log_string (ch,buf);
        ch->pcdata->menu = &room_modify_menu;
        do_menu (ch,NULL);
      }
      break;
    case ID_EDIT_INFO:
      edit_room_info (ch);
      break;
  }
}

void edit_room_name ( CHAR_DATA *ch, char *arg )
{
  ROOM_INDEX_DATA *room;
  char buf[MAX_STRING_LENGTH];

  room = ch->pcdata->edit.room;
  ch->pcdata->interp_fun = do_menu;
  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }
  if (!arg || !arg[0]) {
    send_to_char ("Invalid entry.\n\r>  ",ch);
    return;
  }

  free_string ( room->name );
  room->name = str_dup_perm (arg);
  sprintf (buf,"Room name set to '%s'\n\r>  ",room->name);
  send_to_char (buf,ch);
}

void edit_sector_init ( CHAR_DATA *ch, int num )
{
  ROOM_INDEX_DATA *room;
  char buf[MAX_STRING_LENGTH];

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char (" - Invalid room.\n\r>  ",ch);
    return;
  }

  sprintf (buf,"Room: %s [%d] - Sector: %s\n\r",room->name,room->vnum,sector_table[room->sector_type]);
  send_to_char (buf,ch);
}

void edit_sector ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->menu = &room_modify_menu;
  if (num == ID_EDIT_PREVIOUS) {
    do_menu (ch,NULL);
    return;
  }

  ch->pcdata->edit.room->sector_type = num;
  sprintf (buf,"Sector set to '%s'.\n\r",
    sector_table[ch->pcdata->edit.room->sector_type]);
  send_to_char (buf,ch);
  do_menu (ch,NULL);
}

void edit_room_flags ( CHAR_DATA *ch )
{
  ROOM_INDEX_DATA *room;

  ch->pcdata->interp_fun = do_menu;

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }

  ch->pcdata->edit.mod_flags = &room->room_flags;
  build_flag_menu ( mod_room_flags, "Set Room Flags", ch );
  do_menu (ch,NULL);
}


void edit_exit_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  sprintf (buf," - Selected Exit:  %s - Mode:  %s\n\r\n\r",
           capitalize (dir_table[ch->pcdata->edit.exit]),
           IS_SET (ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR) ?
           "Double sided door":"Single sided door");
  send_to_char (buf,ch);
}

void edit_room_door_select ( CHAR_DATA *ch, char *arg )
{
  int t;
  char buf[MAX_STRING_LENGTH];

  for ( t = 0; t < 6; t++ ) {
    if (!str_prefix (arg, dir_table[t])) {
      ch->pcdata->edit.exit = t;
      sprintf (buf,"Selected:  %s\n\r",dir_table[t]);
      send_to_char (buf,ch);
      ch->pcdata->interp_fun = do_menu;
      do_menu (ch, NULL);
      return;
    }
  }

  send_to_char ("Invalid choice.\n\rWhich exit [north,east,south,west,up,down]:  ",ch);
}

void edit_room_door_info ( CHAR_DATA *ch )
{
  char buf[MAX_STRING_LENGTH];
  EXIT_DATA *pexit;
  ROOM_INDEX_DATA *room;

  room = ch->pcdata->edit.room;

  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }

  pexit = room->exit[ch->pcdata->edit.exit];
  if (!pexit) {
    send_to_char ("Exit does not exist.\n\rUse destination to create one.\n\r>  ",ch);
    return;
  }

  sprintf (buf,"Direction:  %s\n\r",dir_table[ch->pcdata->edit.exit]);
  send_to_char (buf,ch);
  sprintf (buf,"Name:  %s\n\r",pexit->keyword);
  send_to_char (buf,ch);
  sprintf (buf,"To room:  %s [%d]\n\r",pexit->u1.to_room->name,pexit->u1.to_room->vnum);
  send_to_char (buf,ch);
  send_to_char ("Exit flags:  ",ch);
  show_flags (pexit->exit_info,room_exit_flags,ch);
  send_to_char ("\n\r>  ",ch);
}

EXIT_DATA *other_side ( CHAR_DATA *ch )
{
  int room_dir;
  EXIT_DATA *exit;

  room_dir = rev_dir[ch->pcdata->edit.exit];
  if ((exit = ch->pcdata->edit.room->exit[room_dir]) != NULL)
    return exit;
  else
    return NULL;
}

void edit_room_door_connect ( CHAR_DATA *ch, char *arg )
{
  ROOM_INDEX_DATA *room, *room2;
  int vnum;
  char buf[MAX_STRING_LENGTH];

  vnum = atoi (arg);
  room2 = get_room_index (vnum);
  ch->pcdata->interp_fun = do_menu;
  if (room2) {
    room = ch->pcdata->edit.room;
    if (room) {
      if (room->exit[ch->pcdata->edit.exit] != NULL)
        free_exit (room->exit[ch->pcdata->edit.exit]);
      room->exit[ch->pcdata->edit.exit] = create_exit ();
      if (IS_SET(ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR) &&
          !check_range (ch,RANGE_ROOM,room2->vnum)) {
        room->exit[ch->pcdata->edit.exit]->u1.to_room = room2;
        sprintf (buf,"[%d] %s connected to [%d]\n\r",room->vnum,
                 dir_table[ch->pcdata->edit.exit], room2->vnum);
        strcat (buf,"Could not connect other side - permission denied.\n\r>  ");
      } else {
        if (IS_SET(ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR)) {
          room->exit[ch->pcdata->edit.exit]->u1.to_room = room2;
          if (room2->exit[rev_dir[ch->pcdata->edit.exit]] != NULL)
            free_exit (room2->exit[rev_dir[ch->pcdata->edit.exit]]);
          room2->exit[rev_dir[ch->pcdata->edit.exit]] = create_exit ();
          room2->exit[rev_dir[ch->pcdata->edit.exit]]->u1.to_room = room;
          sprintf (buf,"[%d] %s connected to [%d]\n\r[%d] %s connected to [%d]\n\r>  ",
                   room->vnum,dir_table[ch->pcdata->edit.exit], room2->vnum,
                   room2->vnum,dir_table[rev_dir[ch->pcdata->edit.exit]],room->vnum);

        } else {
          room->exit[ch->pcdata->edit.exit]->u1.to_room = room2;
          sprintf (buf,"[%d] %s connected to [%d]\n\r>  ",room->vnum,
                   dir_table[ch->pcdata->edit.exit], room2->vnum);
        }
      }
    } else {
      send_to_char ( "Valid room not selected.\n\r>  ",ch);
      return;
    }
    send_to_char (buf,ch);
    return;
  }
  sprintf (buf,"Invalid vnum %d.\n\r>  ",vnum);                                 
  send_to_char (buf,ch);
}

void edit_room_door_keyword ( CHAR_DATA *ch, char *arg )
{
  ROOM_INDEX_DATA *room;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  if (!arg || !arg[0]) {
    send_to_char ("Invalid keyword.\n\r>  ",ch);
    return;
  }

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }

  if (room->exit[ch->pcdata->edit.exit] == NULL) {
    send_to_char ("Exit does not exist.\n\rUse destination to create one.\n\r>  ",ch);
    return;
  }

  free_string (room->exit[ch->pcdata->edit.exit]->keyword);
  room->exit[ch->pcdata->edit.exit]->keyword = str_dup_perm (arg);
  sprintf (buf,"Exit %s keyword set to %s.\n\r>  ",dir_table[ch->pcdata->edit.exit],arg);
  send_to_char (buf,ch);
}

void edit_room_door_key ( CHAR_DATA *ch, char *arg )
{
  int vnum;
  ROOM_INDEX_DATA *room;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }

  if (room->exit[ch->pcdata->edit.exit] == NULL) {
    send_to_char ("Exit does not exist.\n\rUse destination to create one.\n\r>  ",ch);
    return;
  }

  vnum = atoi (arg);
  room->exit[ch->pcdata->edit.exit]->key = vnum;
  sprintf (buf,"Exit %s key vnum set to %d.\n\r>  ",dir_table[ch->pcdata->edit.exit],vnum);
  send_to_char (buf,ch);
}

void edit_door_flags ( CHAR_DATA *ch )
{
  ROOM_INDEX_DATA *room;

  ch->pcdata->interp_fun = do_menu;

  room = ch->pcdata->edit.room;
  if (!room) {
    send_to_char ("Invalid room selected.\n\r>  ",ch);
    return;
  }

  if (room->exit[ch->pcdata->edit.exit] == NULL) {
    send_to_char ("Exit does not exist.\n\rUse destination to create one.\n\r>  ",ch);
    return;
  }

  ch->pcdata->edit.mod_flags = &room->exit[ch->pcdata->edit.exit]->exit_info;
  build_flag_menu ( room_exit_flags, "Set Exit Flags", ch );
  do_menu (ch,NULL);
}



/*    union
    {
  ROOM_INDEX_DATA *       to_room;
  sh_int                  vnum;
    } u1;
    sh_int              exit_info;
    sh_int              key;
    char *              keyword;
    char *              description; */

void copy_other_side (CHAR_DATA *ch)
{
  int room_dir;
  EXIT_DATA *exit,*other;

  room_dir = rev_dir[ch->pcdata->edit.exit];
  if ((exit = ch->pcdata->edit.room->exit[ch->pcdata->edit.exit]) != NULL) {
    if (exit->u1.to_room) {
      if ((other = exit->u1.to_room->exit[room_dir]) != NULL) {
        if (other->u1.to_room == ch->pcdata->edit.room) {
          other->exit_info = exit->exit_info;
          other->key       = exit->key;
          other->keyword   = str_dup (exit->keyword);
          send_to_char ("Ok.\n\r>  ",ch);
        } else {
          send_to_char ("Other side does not point to this room.\n\r>  ",ch);   
        }
      } else {
        send_to_char ("No door on the other side.\n\r>  ",ch);
      }
    } else {
      send_to_char ("This exit does not point to valid room.\n\r>  ",ch);
    }
  } else {
    send_to_char ("Create an exit first.\n\r>  ",ch);
  }
}

void edit_door_remove (CHAR_DATA *ch)
{
  int room_dir;
  EXIT_DATA *exit,*other;
  ROOM_INDEX_DATA *room;

  room_dir = rev_dir[ch->pcdata->edit.exit];
  if ((exit = ch->pcdata->edit.room->exit[ch->pcdata->edit.exit]) != NULL) {
    if (exit->u1.to_room) {
      if (((other = exit->u1.to_room->exit[room_dir]) != NULL) &&
            IS_SET(ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR)) {
         room = exit->u1.to_room;
         free_exit (other);
         room->exit[room_dir] = NULL;
      }
      ch->pcdata->edit.room->exit[ch->pcdata->edit.exit] = NULL;
      free_exit (exit);
      send_to_char ("Ok.\n\r>  ",ch);
    } else {
      send_to_char ("This exit does not point to valid room.\n\r>  ",ch);
    }
  } else {
    send_to_char ("There is no exit in that direction.\n\r>  ",ch);
  }
}

void edit_room_exits ( CHAR_DATA *ch, int num )
{
  switch (num) {
    case ID_DOOR_SELECT:
      send_to_char ("Which exit [north,east,south,west,up,down]:  ",ch);
      ch->pcdata->interp_fun = edit_room_door_select;
      break;
    case ID_DOOR_INFO:
      edit_room_door_info (ch);
      break;
    case ID_DOOR_VNUM:
      send_to_char (IS_SET (ch->pcdata->edit.per_flags,EDIT_DOUBLE_DOOR) ?
                    "Mode:  Double Sided Door\n\r":"Mode:  Single Sided Door\n\r",ch);
      send_to_char ("Connect exit to which room (vnum):  ",ch);
      ch->pcdata->interp_fun = edit_room_door_connect;
      break;
    case ID_DOOR_KEYWORD:
      send_to_char ("Enter keyword:  ",ch);
      ch->pcdata->interp_fun = edit_room_door_keyword;
      break;
    case ID_DOOR_KEY:
      send_to_char ("Enter key vnum:  ",ch);
      ch->pcdata->interp_fun = edit_room_door_key;
      break;
    case ID_DOOR_FLAGS:
      edit_door_flags (ch);
      break;
    case ID_DOOR_COPY:
      copy_other_side (ch);
      break;
    case ID_DOOR_REMOVE:
      edit_door_remove (ch);
      break;
    case ID_EDIT_PREVIOUS:
      ch->pcdata->menu = &room_modify_menu;
      do_menu (ch,NULL);
      break;
  }
}

void edit_room_desc ( CHAR_DATA *ch, char *buf )
{
  ch->pcdata->edit.room->description = str_dup_perm (buf);
  send_to_char (">  ",ch);
}

void edit_room_extend_desc ( CHAR_DATA *ch, char *arg )
{


  /* no need to free string */
  ch->pcdata->edit.room->extra_descr->description = str_dup (arg);
  send_to_char (">  ",ch);
}

void edit_room_clan ( CHAR_DATA *ch, char *arg )
{
  int num;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  num = ch->pcdata->edit.room->clan = clan_lookup(arg);
  if (num == 0) {
    send_to_char ("Clan not found.  Set to no clan.\n\r>  ",ch);
    return;
  }

  sprintf (buf,"Room set to clan '%s'.\n\r>  ",clan_table[num].name);
  send_to_char (buf,ch);

}

void edit_room_extend_add ( CHAR_DATA *ch, char *arg )
{
  EXTRA_DESCR_DATA *ed,*first_ed,*prev_ed;
  char buf[MAX_STRING_LENGTH];

  prev_ed = NULL;
  first_ed = ed = ch->pcdata->edit.room->extra_descr;
  while (ed) {
    if (!str_prefix (arg,ed->keyword)) {
      if (prev_ed) {
        prev_ed->next = ed->next;
        ed->next = first_ed;
        ch->pcdata->edit.room->extra_descr = ed;
        sprintf (buf,"Editing extended '%s'.\n\r",ed->keyword);
        send_to_char (buf,ch);
      }
      break;
    }
    prev_ed = ed;
    ed = ed->next;
  }

  if (ed == NULL) {
    ed = new_extra_descr();
    ed->keyword = str_dup (arg);
    ed->description = str_dup_perm ("Generic extended description.\n\r");
    ed->next = first_ed;
    ch->pcdata->edit.room->extra_descr = ed;
    send_to_char ("Creating new extended.\n\r",ch);
  }

  do_line_editor (ch,ch->pcdata->edit.room->extra_descr->description,
                  &edit_room_extend_desc);
}

void edit_room_extend_rem ( CHAR_DATA *ch, char *arg )
{
  EXTRA_DESCR_DATA *ed,*first_ed,*prev_ed;
  char buf[MAX_STRING_LENGTH];
  bool found = FALSE;

  prev_ed = NULL;
  ch->pcdata->interp_fun = do_menu;
  first_ed = ed = ch->pcdata->edit.room->extra_descr;
  while (ed) {
    if (!str_prefix (arg,ed->keyword)) {
      if (prev_ed) {
        prev_ed->next = ed->next;
      } else {
        ch->pcdata->edit.room->extra_descr = ed->next;
      }
      sprintf (buf,"Extended description '%s' removed.\n\r>  ",ed->keyword);    
      send_to_char (buf,ch);
      found = TRUE;
      free_extra_descr (ed);
      break;
    }
    prev_ed = ed;
    ed = ed->next;
  }

  if (!found) {
    send_to_char ("Extended description not found.\n\r>  ",ch);
  }
}

void edit_room_owner ( CHAR_DATA *ch, char *arg )
{
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  if (!strcmp (arg,"clear")) {
    ch->pcdata->edit.room->owner = NULL;
    send_to_char ("Owner cleared.\n\r>  ",ch);
    return;
  }

  sprintf (buf,"Owners set to:  %s\n\r>  ",arg);
  send_to_char (buf,ch);
  ch->pcdata->edit.room->owner = str_dup (arg);
}

void edit_room_heal ( CHAR_DATA *ch, char *arg )
{
  int num;

  ch->pcdata->interp_fun = do_menu;
  if (is_number (arg)) {
    num = atoi (arg);
    ch->pcdata->edit.room->heal_rate = num;
    send_to_char ("Ok.\n\r>  ",ch);
    return;
  }
  send_to_char ("Invalid entry.\n\r>  ",ch);
}

void edit_room_mana ( CHAR_DATA *ch, char *arg )
{
  int num;

  ch->pcdata->interp_fun = do_menu;
  if (is_number (arg)) {
    num = atoi (arg);
    ch->pcdata->edit.room->mana_rate = num;
    send_to_char ("Ok.\n\r>  ",ch);
    return;
  }
  send_to_char ("Invalid entry.\n\r>  ",ch);
}

void edit_room_obs ( CHAR_DATA *ch, char *arg )
{
  int num;

  ch->pcdata->interp_fun = do_menu;
  if (is_number (arg)) {
    num = atoi (arg);
    ch->pcdata->edit.room->obs_target = num;
    send_to_char ("Ok.\n\r>  ",ch);
    return;
  }
  send_to_char ("Invalid entry.\n\r>  ",ch);
}

void edit_room_modify ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_ROOM_NAME:
      sprintf (buf,"Current Name:  %s\n\rNew name (Nothing to cancel):  ",
               ch->pcdata->edit.room->name);
      send_to_char (buf,ch);
      ch->pcdata->interp_fun = edit_room_name;
      break;
    case ID_ROOM_DESC:
      if (!ch->pcdata->edit.room)
        send_to_char ("Select a room first.\n\r>  ",ch);
      else
        do_line_editor (ch,ch->pcdata->edit.room->description,&edit_room_desc); 
      break;
    case ID_ROOM_OWNER:
      send_to_char ("Enter owners of room or [clear]:  ",ch);
      ch->pcdata->interp_fun = edit_room_owner;
      break;
    case ID_ROOM_FLAGS:
      edit_room_flags ( ch );
      break;
    case ID_ROOM_OBS:
      send_to_char("Enter vnum of room in target area:  ", ch);
      ch->pcdata->interp_fun = edit_room_obs;
      break;
    case ID_ROOM_HEAL:
      send_to_char ("Enter heal rate:  ",ch);
      ch->pcdata->interp_fun = edit_room_heal;
      break;
    case ID_ROOM_MANA:
      send_to_char ("Enter mana rate:  ",ch);
      ch->pcdata->interp_fun = edit_room_mana;
      break;
    case ID_ROOM_DOOR:
      ch->pcdata->menu = &exit_modify_menu;
      send_to_char ("Which exit [north,east,south,west,up,down]:  ",ch);
      ch->pcdata->interp_fun = edit_room_door_select;
      break;
    case ID_ROOM_TERRAIN:
      ch->pcdata->menu = &sector_menu;
      do_menu (ch,NULL);
      break;
    case ID_ROOM_CLAN:
      send_to_char ("Enter clan:  ",ch);
      ch->pcdata->interp_fun = edit_room_clan;
      break;
    case ID_ROOM_EXTENDED:
      send_to_char ("Enter keyword:  ",ch);
      ch->pcdata->interp_fun = edit_room_extend_add;
      break;
    case ID_ROOM_REM_EXTEND:
      send_to_char ("Enter keyword to remove:  ",ch);
      ch->pcdata->interp_fun = edit_room_extend_rem;
      break;
    case ID_EDIT_PREVIOUS:
      ch->pcdata->menu = &room_menu;
      do_menu (ch,NULL);
      break;
  }
}

void edit_mob_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if (ch->pcdata->edit.mob)
    sprintf (buf," - Selected Mob:  %s [%d]\n\r\n\r",
             ch->pcdata->edit.mob->short_descr,ch->pcdata->edit.mob->vnum );
  else
    sprintf (buf," - Selected Mob:  None\n\r\n\r");
  send_to_char (buf,ch);
}


void edit_mob_vnum ( CHAR_DATA *ch, char *arg )
{
   MOB_INDEX_DATA *mob;
   int vnum;
   char buf[MAX_STRING_LENGTH];

   vnum = atoi (arg);
   mob = get_mob_index (vnum);
   if (!mob) {
     sprintf (buf,"Invalid mob vnum %d.\n\r>  ",vnum);
   } else {
     ch->pcdata->edit.mob = mob;
     sprintf (buf,"Selected Mob:  %s [%d]\n\r>  ",mob->short_descr,vnum);
   }
   send_to_char (buf,ch);
   ch->pcdata->interp_fun = do_menu;
}

void edit_mob_list (CHAR_DATA *ch)
{
  MOB_INDEX_DATA *mob;
  int idx;
  char buf[MAX_STRING_LENGTH],bigbuf[4*MAX_STRING_LENGTH];
  int col = FALSE;

  bigbuf[0] = NULL;
  for (idx = ch->pcdata->edit.area->min_vnum_mob;
       idx <= ch->pcdata->edit.area->max_vnum_mob; idx++) {
    if (strlen (bigbuf) > (4*MAX_STRING_LENGTH - 200)) {
      strcat (bigbuf,"\n\rWarning: Buffer Full\n\r");
      break;
    }
    mob = get_mob_index (idx);
    if (mob) {
      sprintf (buf,"[%5d] %-30s",mob->vnum,mob->short_descr);
      if (col) {
        col = FALSE;
        if (strlen (buf) > 38) {
          strcat (bigbuf,"\n\r");
        }
        strcat (bigbuf,buf);
      } else {
        col = TRUE;
        if (strlen (buf) > 38) {
          col = FALSE;
        }
        strcat (bigbuf,"\n\r");
        strcat (bigbuf,buf);
      }
    }
  }
  strcat (bigbuf,"\n\r>  ");
  page_to_char (bigbuf,ch);
}

void edit_mob_create (CHAR_DATA *ch)
{
  MOB_INDEX_DATA *mob;
  int vnum, iHash, t;
  extern KILL_DATA kill_table[MAX_LEVEL];
  char buf[MAX_STRING_LENGTH];

  vnum = 0;
  for (t = ch->pcdata->edit.area->min_vnum_mob;
       t <= (ch->pcdata->edit.area->max_vnum_mob+1); t++) {
    if (!get_mob_index (t)) {
      vnum = t;
      break;
    }
  }
  if (vnum == 0) {
    send_to_char ("Out of mob vnums for this area.\n\r",ch);
    return;
  }

  if (!check_range (ch,RANGE_MOB,vnum)) {
    send_to_char ("Mob could not be created.\n\r",ch);
    return;
  }

  sprintf (buf,"Created mob [%d].",vnum);
  olc_log_string (ch,buf);

  if (vnum >  ch->pcdata->edit.area->max_vnum_mob)
    ch->pcdata->edit.area->max_vnum_mob = vnum;

  mob = alloc_perm (sizeof (MOB_INDEX_DATA));
  mob->new_format  = TRUE;
  mob->vnum        = vnum;
  mob->spec_fun    = NULL;
  mob->pShop       = NULL;
  mob->count       = 0;
  mob->killed      = 0;
  mob->player_name = str_dup_perm ("generic mob");
  mob->short_descr = str_dup_perm ("Generic short description");
  mob->long_descr  = str_dup_perm ("Generic long description");
  mob->description = str_dup_perm ("Generic description.");
  mob->spec_words[0] = NULL;
  mob->spec_words[1] = NULL;
  mob->spec_words[2] = NULL;
  mob->act         = ACT_IS_NPC | ACT_WARRIOR;
  mob->affected_by = 0;
  mob->alignment   = 0;
  mob->level       = 1;
  mob->hitroll     = 1;
/*  mob->avg_hp      = 5;
  mob->avg_mana    = 5;
  mob->avg_damage  = 5;
  mob->avg_ac      = 5; */
  mob->dam_type    = 1;
  mob->race        = race_lookup("human");
  mob->off_flags   = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_VNUM;
  mob->imm_flags   = 0;
  mob->res_flags   = 0;
  mob->vuln_flags  = 0;
  mob->start_pos   = POS_STANDING;
  mob->default_pos = POS_STANDING;
  mob->sex         = 0;
  mob->gold        = 1;
  mob->form        = FORM_EDIBLE|FORM_SENTIENT|FORM_BIPED|FORM_MAMMAL;
  mob->parts       = PART_HEAD|PART_ARMS|PART_LEGS|PART_HEART|
                     PART_BRAINS|PART_GUTS;
  mob->size        = SIZE_MEDIUM;
  mob->material    = str_dup("0");

  iHash            = vnum % MAX_KEY_HASH;
  mob->next        = mob_index_hash[iHash];
  mob_index_hash[iHash] = mob;
  top_mob_index++;
  kill_table[URANGE(0, mob->level, MAX_LEVEL-1)].number++;
  ch->pcdata->edit.mob = mob;
  send_to_char ("Ok.\n\r",ch);
}

void edit_mob_name ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Name not changed.\n\r>  ",ch);
    return;
  }

  free_string (mob->player_name);
  mob->player_name = str_dup_perm (arg);
  sprintf (buf,"Name set to '%s'.\n\r>",mob->player_name);
  send_to_char (buf,ch);
}

void edit_mob_short ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Short description not changed.\n\r>  ",ch);
    return;
  }

  free_string (mob->short_descr);
  mob->short_descr = str_dup_perm (arg);
  sprintf (buf,"Short set to '%s'.\n\r> ",mob->short_descr);
  send_to_char (buf,ch);
}

void edit_mob_long ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Long description not changed.\n\r>  ",ch);
    return;
  }

  strcat (arg,"\n\r");
  free_string (mob->long_descr);
  mob->long_descr = str_dup_perm (arg);
  sprintf (buf,"Long:  %s> ",mob->long_descr);
  send_to_char (buf,ch);
}

void edit_mob_spec_words ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];
  char arg1[MAX_STRING_LENGTH];
  int v;

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Spec words not changed.\n\r>  ",ch);
    return;
  }
  arg = one_argument (arg,arg1);
  v = atoi(arg1);
  if( v < 0 || v > 2 )
  {
    send_to_char("Invalid spec word number.\n\r>  ",ch);
    return;
  }

  free_string(mob->spec_words[v]);
  mob->spec_words[v] = str_dup_perm(arg);
  sprintf(buf,"Spec Words[%d]:  %s> ",v,mob->spec_words[v]);
  send_to_char(buf,ch);
}

void edit_mob_level ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];
  int level;

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Level not changed.\n\r>  ",ch);
    return;
  }
  level = atoi (arg);
  if ((level < 0) || (level > 60)) {
    send_to_char ("Level must be from 0 to 60.\n\r>  ",ch);
    return;
  }

  mob->level = level;
  sprintf (buf,"Level set to %d.\n\r> ",level);
  send_to_char (buf,ch);
}

void edit_mob_align ( CHAR_DATA *ch, char *arg )
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];
  int align;

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Alignment not changed.\n\r>  ",ch);
    return;
  }
  align = atoi (arg);
  if ((align < -1000) || (align > 1000)) {
    send_to_char ("Alignment must be from -1000 to 1000.\n\r>  ",ch);
    return;
  }

  mob->alignment = align;
  sprintf (buf,"Alignment set to %d.\n\r> ",align);
  send_to_char (buf,ch);
}

void edit_mob_desc (CHAR_DATA *ch, char *buf)
{
  /* free_string (ch->pcdata->edit.mob->description); */
  ch->pcdata->edit.mob->description = str_dup_perm (buf);
  send_to_char (">  ",ch);
}

void edit_mob_ac_magic (CHAR_DATA *ch,char *arg)
{
  char buf[100];
  int num;

  num = atoi (arg);
  if (num > 500) num = 500;
  if (num < -500) num = -500;

  ch->pcdata->edit.mob->ac[AC_EXOTIC] = num;
  sprintf (buf,"Magic AC set to %d.\n\r>  ",num);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = do_menu;
}

void edit_mob_ac_slash (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  int num;

  num = atoi (arg);
  if (num > 500) num = 500;
  if (num < -500) num = -500;

  ch->pcdata->edit.mob->ac[AC_SLASH] = num;
  sprintf (buf,"Slash AC set to %d.\n\rEnter magic AC (-500 - 500):  ",num);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = edit_mob_ac_magic;
}

void edit_mob_ac_bash (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  int num;

  num = atoi (arg);
  if (num > 500) num = 500;
  if (num < -500) num = -500;

  ch->pcdata->edit.mob->ac[AC_BASH] = num;
  sprintf (buf,"Bash AC set to %d.\n\rEnter slash AC (-500 - 500):  ",num);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = edit_mob_ac_slash;
}

void edit_mob_ac_pierce (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  int num;

  num = atoi (arg);
  if (num > 500) num = 500;
  if (num < -500) num = -500;

  ch->pcdata->edit.mob->ac[AC_PIERCE] = num;
  sprintf (buf,"Pierce AC set to %d.\n\rEnter bash AC (-500 - 500):  ",num);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = edit_mob_ac_bash;
}

void edit_mob_hp (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  MOB_INDEX_DATA *mob;
  int hit0,hit1,hit2;

  ch->pcdata->interp_fun = do_menu;

  mob = ch->pcdata->edit.mob;
  if (sscanf (arg,"%dd%d+%d",&hit0,&hit1,&hit2) != 3) {
    send_to_char ("Invalid format.  Format is 'ndn+n'.\n\r>  ",ch);
    return;
  }
  mob->hit[DICE_NUMBER] = hit0;
  mob->hit[DICE_TYPE]   = hit1;
  mob->hit[DICE_BONUS]  = hit2;

  sprintf (buf,"Hit Dice set too %dd%d+%d.\n\r>  ",mob->hit[DICE_NUMBER],
    mob->hit[DICE_TYPE],mob->hit[DICE_BONUS]);
  send_to_char (buf,ch);
}

void edit_mob_mana (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  MOB_INDEX_DATA *mob;
  int hit0,hit1,hit2;

  ch->pcdata->interp_fun = do_menu;

  mob = ch->pcdata->edit.mob;
  if (sscanf (arg,"%dd%d+%d",&hit0,&hit1,&hit2) != 3) {
    send_to_char ("Invalid format.  Format is 'ndn+n'.\n\r>  ",ch);
    return;
  }
  mob->mana[DICE_NUMBER] = hit0;
  mob->mana[DICE_TYPE]   = hit1;
  mob->mana[DICE_BONUS]  = hit2;

  sprintf (buf,"Mana Dice set too %dd%d+%d.\n\r>  ",mob->mana[DICE_NUMBER],
    mob->mana[DICE_TYPE],mob->mana[DICE_BONUS]);
  send_to_char (buf,ch);
}

void edit_mob_dam (CHAR_DATA *ch,char *arg)
{
  char buf[MAX_STRING_LENGTH];
  MOB_INDEX_DATA *mob;
  int hit0,hit1,hit2;

  ch->pcdata->interp_fun = do_menu;

  mob = ch->pcdata->edit.mob;
  if (sscanf (arg,"%dd%d+%d",&hit0,&hit1,&hit2) != 3) {
    send_to_char ("Invalid format.  Format is 'ndn+n'.\n\r>  ",ch);
    return;
  }
  mob->damage[DICE_NUMBER] = hit0;
  mob->damage[DICE_TYPE]   = hit1;
  mob->damage[DICE_BONUS]  = hit2;

  sprintf (buf,"Damage Dice set too %dd%d+%d.\n\r>  ",mob->damage[DICE_NUMBER],
    mob->damage[DICE_TYPE],mob->damage[DICE_BONUS]);
  send_to_char (buf,ch);
}

void edit_mob_hit (CHAR_DATA *ch, char *arg)
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];
  int hit;

  mob = ch->pcdata->edit.mob;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Hitroll not changed.\n\r>  ",ch);
    return;
  }
  hit = atoi (arg);
  if ((hit < -20) || (hit > 100)) {
    send_to_char ("Hitroll must be from -20 to 100.\n\r>  ",ch);
    return;
  }

  mob->hitroll = hit;
  sprintf (buf,"Hitroll set to %d.\n\r> ",hit);
  send_to_char (buf,ch);
}

void edit_mob_attack (CHAR_DATA *ch, int num)
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];
  int count,t;

  count = 0;
  for (t = 0;; t++) {
    if (!attack_table[t].name) break;
    count++;
  }
  count -= 1;

  mob = ch->pcdata->edit.mob;
  if (num == 0) {
    sprintf (buf," - Selected: [%d] %s - Attack Type: %s\n\r",mob->vnum,
      mob->short_descr,attack_table[mob->dam_type].name);
    send_to_char (buf,ch);
    return;
  }

  if ((num > 0) && (num <= count)) {
    mob->dam_type = num;
    sprintf (buf,"Attack type now %s.\n\r>  ",
      capitalize(attack_table[mob->dam_type].name));
    send_to_char (buf,ch);
    destroy_attack_menu (ch);
    return;
  }
  send_to_char ("Operation canceled.\n\r>  ",ch);
  destroy_attack_menu (ch);
  do_menu (ch,NULL);
  return;
}

void edit_mob_sex ( CHAR_DATA *ch, char *arg )
{
  ch->pcdata->interp_fun = do_menu;
  if (!str_prefix (arg,"male")) {
    send_to_char ("Sex set to male.\n\r>  ",ch);
    ch->pcdata->edit.mob->sex = SEX_MALE;
    return;
  }
  if (!str_prefix (arg,"female")) {
    send_to_char ("Sex set to female.\n\r>  ",ch);
    ch->pcdata->edit.mob->sex = SEX_FEMALE;
    return;
  }
  if (!str_prefix (arg,"nosex")) {
    send_to_char ("Sex set to sexless.\n\r>  ",ch);
    ch->pcdata->edit.mob->sex = SEX_NEUTRAL;
    return;
  }

  send_to_char ("Invalid choice.  Sex not changed.\n\r>  ",ch);
}

void edit_mob_wealth ( CHAR_DATA *ch, char *arg )
{
  int wealth;
  char buf[MAX_STRING_LENGTH];

  wealth = atoi (arg);
  ch->pcdata->interp_fun = do_menu;
  sprintf (buf,"Mob wealth set to %d.\n\r>  ",wealth);
  ch->pcdata->edit.mob->wealth = wealth;
  send_to_char (buf,ch);
}

void edit_mob_size_init ( CHAR_DATA *ch, int num )
{
  sh_int size;
  char *size_table[6] = {"Tiny","Small","Medium","Large","Huge","Giant"};
  char buf[MAX_STRING_LENGTH];

  size = ch->pcdata->edit.mob->size;
  if (size < 0 || size > 5)
    size = ch->pcdata->edit.mob->size = 2;
  sprintf (buf," - Current mob size '%s'\n\r",size_table[size]);
  send_to_char (buf,ch);
}

void edit_mob_size ( CHAR_DATA *ch, int num )
{
  if (num == ID_EDIT_CANCEL) {
    send_to_char ("Operation Cancelled.\n\r",ch);
    ch->pcdata->menu = &mob_modify_menu;
    do_menu (ch,NULL);
    return;
  }
  ch->pcdata->edit.mob->size = num;
  send_to_char ("Ok.\n\r",ch);
  ch->pcdata->menu = &mob_modify_menu;
  do_menu (ch,NULL);
}

void edit_mob_race_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  sprintf (buf," - Current Race '%s'\n\r",race_table[ch->pcdata->edit.mob->race].name);
  send_to_char (buf,ch);
}

void edit_mob_race      ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if (num == ID_EDIT_CANCEL) {
    ch->pcdata->menu = ch->pcdata->edit.prev_menu;
    send_to_char ("Operation Cancelled.\n\r",ch);
    do_menu (ch,NULL);
    return;
  }

  ch->pcdata->edit.mob->race = num;
  sprintf (buf,"Race set to '%s'\n\r",race_table[ch->pcdata->edit.mob->race].name);
  send_to_char (buf,ch);
  ch->pcdata->menu = ch->pcdata->edit.prev_menu;
  do_menu (ch,NULL);
}

void edit_mob_spec_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];
  char *spec_str;

  spec_str = spec_name (ch->pcdata->edit.mob->spec_fun);
  if (!spec_str) spec_str = "None";

  sprintf (buf," - Current Special '%s'\n\r",spec_str);
  send_to_char (buf,ch);
}

void edit_mob_spec ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if (num == ID_EDIT_CANCEL) {
    ch->pcdata->menu = ch->pcdata->edit.prev_menu;
    send_to_char ("Operation Cancelled.\n\r",ch);
    do_menu (ch,NULL);
    return;
  }

  if (num == ID_SPEC_NONE) {
    ch->pcdata->edit.mob->spec_fun = NULL;
    sprintf (buf,"Special removed.\n\r");
  } else {
    ch->pcdata->edit.mob->spec_fun = spec_table[num].function;
    sprintf (buf,"Special set to '%s'\n\r",spec_table[num].name);
  }

  send_to_char (buf,ch);
  ch->pcdata->menu = ch->pcdata->edit.prev_menu;
  do_menu (ch,NULL);
}

void edit_mob_pos_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  sprintf (buf," - Current starting position:  %s.\n\r",
           position_table[ch->pcdata->edit.mob->start_pos].name);
  send_to_char (buf,ch);
}

void edit_mob_position ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->menu = &mob_modify_menu;
  switch (num) {
    case ID_EDIT_CANCEL:
      send_to_char ("Operation cancelled.\n\r>  ",ch);
      return;
      break;
    default:
      ch->pcdata->edit.mob->start_pos =
      ch->pcdata->edit.mob->default_pos = num;
      break;
  }
  sprintf (buf,"Position set to '%s'.\n\r>  ",position_table[num].name);
  send_to_char (buf,ch);
  do_menu (ch,NULL);
}


void edit_mob_modify ( CHAR_DATA *ch, int num )
{
  switch (num)  {
    case ID_MOB_NAME:
      send_to_char ("Enter Name (Keywords separated by spaces):  ",ch);
      ch->pcdata->interp_fun = edit_mob_name;
      break;
    case ID_MOB_SHORT:
      send_to_char ("Enter Short Description:  ",ch);
      ch->pcdata->interp_fun = edit_mob_short;
      break;
    case ID_MOB_LONG:
      send_to_char ("Enter Long Description:  ",ch);
      ch->pcdata->interp_fun = edit_mob_long;
      break;
    case ID_MOB_SPEC_WORDS:
      send_to_char("Enter # (0-2) and Spec Words seperated by spaces: ",ch);
      ch->pcdata->interp_fun = edit_mob_spec_words;
      break;
    case ID_MOB_DESC:
      do_line_editor (ch,ch->pcdata->edit.mob->description,&edit_mob_desc);
      break;
    case ID_MOB_LEVEL:
      send_to_char ("Enter Level:  ",ch);
      ch->pcdata->interp_fun = edit_mob_level;
      break;
    case ID_MOB_HP:
      send_to_char ("Enter Hit Dice (ndn+n):  ",ch);
      ch->pcdata->interp_fun = edit_mob_hp;
      break;
    case ID_MOB_MANA:
      send_to_char ("Enter Mana Dice (ndn+n):  ",ch);
      ch->pcdata->interp_fun = edit_mob_mana;
      break;
    case ID_MOB_AC:
      send_to_char ("Enter Pierce AC (-500 - 500):  ",ch);
      ch->pcdata->interp_fun = edit_mob_ac_pierce;
      break;
    case ID_MOB_DAMTYPE:
      build_attack_menu (ch,edit_mob_attack);
      do_menu (ch,NULL);
      break;
    case ID_MOB_DAMAGE:
      send_to_char ("Enter Damage Dice (ndn+n):  ",ch);
      ch->pcdata->interp_fun = edit_mob_dam;
      break;
    case ID_MOB_HITROLL:
      send_to_char ("Enter Hitroll [-20-100]:  ",ch);
      ch->pcdata->interp_fun = edit_mob_hit;
      break;
    case ID_MOB_ACT:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->act;
      build_flag_menu ( act_table, "Set Act Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_OFF:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->off_flags;
      build_flag_menu ( off_table, "Set Offensive Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_IMM:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->imm_flags;
      build_flag_menu ( irv_table, "Set Immune Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_RES:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->res_flags;
      build_flag_menu ( irv_table, "Set Resistant Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_VULN:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->vuln_flags;
      build_flag_menu ( irv_table, "Set Vulnerable Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_PARTS:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->parts;
      build_flag_menu ( part_table, "Set Part Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_FORM:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->form;
      build_flag_menu ( form_table, "Set Form Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_RACE:
      build_race_menu (ch);
      do_menu (ch,NULL);
      break;
    case ID_MOB_SEX:
      send_to_char ("Enter Sex [male,female,nosex]:  ",ch);
      ch->pcdata->interp_fun = edit_mob_sex;
      break;
    case ID_MOB_POS:
      ch->pcdata->menu = &mob_position_menu;
      do_menu (ch,NULL);
      break;
    case ID_MOB_WEALTH:
      send_to_char ("Enter mob wealth:  ",ch);
      ch->pcdata->interp_fun = edit_mob_wealth;
      break;
    case ID_MOB_SIZE:
      ch->pcdata->menu = &mob_size_menu;
      do_menu (ch,NULL);
      break;
    case ID_MOB_ALIGN:
      send_to_char ("Enter Align:  ",ch);
      ch->pcdata->interp_fun = edit_mob_align;
      break;
    case ID_MOB_AFF:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.mob->affected_by;
      build_flag_menu ( aff_table, "Set Affect Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_MOB_SPEC:
      build_spec_menu (ch);
      do_menu (ch,NULL);
      break;
    case ID_EDIT_PREVIOUS:
      ch->pcdata->menu = &mob_menu;
      do_menu (ch,NULL);
      break;
  }

}

void edit_mob_copy (CHAR_DATA *ch, char *arg)
{
  MOB_INDEX_DATA *mob,*victim;

  int vnum,t;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  mob = ch->pcdata->edit.mob;
  if (!mob) {
    send_to_char ("No mob selected to copy to.\n\r>  ",ch);
    return;
  }
  vnum = atoi (arg);
  victim = get_mob_index (vnum);
  if (!victim) {
    sprintf (buf,"Invalid mob vnum %d.\n\r>  ",vnum);
    send_to_char (buf,ch);
    return;
  }

  mob->spec_fun    = victim->spec_fun;
  if (victim->pShop) {
    SHOP_DATA* pShop;
    extern SHOP_DATA *shop_first,*shop_last;
    extern int top_shop;

    pShop               = alloc_perm( sizeof(*pShop) );
    *pShop              = *victim->pShop;
    pShop->keeper       = mob->vnum;
    mob->pShop          = pShop;
    if ( shop_first == NULL ) shop_first = pShop;
    if ( shop_last  != NULL ) shop_last->next = pShop;
    shop_last = pShop;
    pShop->next = NULL;
    top_shop++;

  }

  mob->count       = 0;
  mob->killed      = 0;
  free_string (mob->player_name);
  mob->player_name = str_dup_perm (victim->player_name);
  free_string (mob->short_descr);
  mob->short_descr = str_dup_perm (victim->short_descr);
  free_string (mob->long_descr);
  mob->long_descr  = str_dup_perm (victim->long_descr);
  free_string (mob->description);
  mob->description = str_dup_perm (victim->description);
  mob->act         = victim->act;
  mob->affected_by = victim->affected_by;
  mob->alignment   = victim->alignment;
  mob->level       = victim->level;
  mob->hitroll     = victim->hitroll;
  /* mob->avg_hp      = victim->avg_hp;
  mob->avg_mana    = victim->avg_mana;
  mob->avg_damage  = victim->avg_damage;
  mob->avg_ac      = victim->avg_ac; */
  mob->dam_type    = victim->dam_type;
  mob->race        = victim->race;
  mob->off_flags   = victim->off_flags;
  mob->imm_flags   = victim->imm_flags;
  mob->res_flags   = victim->res_flags;
  mob->vuln_flags  = victim->vuln_flags;
  mob->start_pos   = victim->start_pos;
  mob->default_pos = victim->default_pos;
  mob->sex         = victim->sex;
  mob->gold        = victim->gold;
  mob->form        = victim->form;
  mob->parts       = victim->parts;
  mob->size        = victim->size;
  mob->material    = victim->material;
  for ( t = 0; t < 3; t++ ) {
    mob->hit[t]    = victim->hit[t];
    mob->mana[t]   = victim->mana[t];
    mob->damage[t] = victim->damage[t];
  }
  for ( t = 0; t < 4; t++ )
    mob->ac[t]     = victim->ac[t];


  sprintf (buf,"Copied Mob %s [%d] to [%d]\n\r>  ",
           victim->short_descr,victim->vnum,mob->vnum);
  send_to_char (buf,ch);
}

void edit_mob_info (CHAR_DATA *ch)
{
  MOB_INDEX_DATA *mob;
  char buf[MAX_STRING_LENGTH];

  mob = ch->pcdata->edit.mob;
  if (!mob) {
    send_to_char ("No mob selected.\n\r",ch);
    return;
  }

  sprintf( buf, "Name: %s.\n\r",
    mob->player_name );
  send_to_char( buf, ch );

  sprintf( buf, "Vnum: %d  Race: %s  Sex: %s\n\r",
    mob->vnum,
    race_table[mob->race].name,
    mob->sex == SEX_MALE    ? "male"   :
    mob->sex == SEX_FEMALE  ? "female" : "neutral");
  send_to_char( buf, ch );

  sprintf(buf,"Count: %d - Killed: %d\n\r",
      mob->count,mob->killed);
  send_to_char(buf,ch);

  sprintf( buf,
    "Lv: %d  Align: %d  Wealth: %ld  ",
    mob->level,
    mob->alignment,
    mob->wealth );
  send_to_char( buf, ch );

  sprintf( buf, "Hp: %dd%d+%d  Mana: %dd%d+%d\n\r", mob->hit[0],mob->hit[1],
           mob->hit[2],mob->mana[0],mob->mana[1],mob->mana[2]);
  send_to_char( buf, ch );


  sprintf(buf,"Armor: %d %d %d %d   Damage:  %dd%d+%d   Dam_Message: %s\n\r",
    mob->ac[0],mob->ac[1],mob->ac[2],mob->ac[3],mob->damage[0],mob->damage[1],
    mob->damage[2],attack_table[mob->dam_type].name);
  send_to_char(buf,ch);

  send_to_char ("Act:  ",ch);
  show_flags (mob->act,act_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Offense:  ",ch);
  show_flags (mob->off_flags,off_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Immune:  ",ch);
  show_flags (mob->imm_flags,irv_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Resist:  ",ch);
  show_flags (mob->res_flags,irv_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Vulnerable:  ",ch);
  show_flags (mob->vuln_flags,irv_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Form:  ",ch);
  show_flags (mob->form,form_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Parts:  ",ch);
  show_flags (mob->parts,part_table,ch);
  send_to_char ("\n\r",ch);
  send_to_char ("Affected by:  ",ch);
  show_flags (mob->affected_by,aff_table,ch);
  send_to_char ("\n\r",ch);

  sprintf( buf, "Short description: %s\n\rLong  description: %s",
    mob->short_descr,
    mob->long_descr[0] != '\0' ? mob->long_descr : "(none)\n\r" );
  send_to_char( buf, ch );

  if ( mob->spec_fun != 0 )
  {
    send_to_char( "Mobile has special procedure.\n\r", ch );
    sprintf( buf, "Words[0]: %s\n\rWords[1]: %s\n\rWords[2]: %s\n\r",
                mob->spec_words[0],mob->spec_words[1],mob->spec_words[2]);
  }
}

void edit_mob_conf_shop ( CHAR_DATA *ch, char *arg )
{
  ch->pcdata->interp_fun = do_menu;
  if (arg[0] == 'y' || arg[0] == 'Y') {
    MOB_INDEX_DATA *mob;
    SHOP_DATA *pShop;
    extern SHOP_DATA *shop_first,*shop_last;
    extern int top_shop;
    int iTrade;

    mob                 = ch->pcdata->edit.mob;
    pShop               = alloc_perm( sizeof(*pShop) );
    pShop->keeper       = mob->vnum;
    for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
      pShop->buy_type[iTrade] = 0;
    pShop->profit_buy   = 50;
    pShop->profit_sell  = 50;
    pShop->open_hour    = 0;
    pShop->close_hour   = 24;
    mob->pShop          = pShop;

    if ( shop_first == NULL )
      shop_first = pShop;
    if ( shop_last  != NULL )
      shop_last->next = pShop;

    shop_last = pShop;
    pShop->next = NULL;
    top_shop++;
  } else {
    send_to_char ("Operation Cancelled.\n\r>  ",ch);
    return;
  }

  ch->pcdata->menu = &mob_shop_menu;
  do_menu (ch,NULL);
}

void edit_shop_clone ( CHAR_DATA *ch, char *arg )
{
  int vnum,iTrade;
  MOB_INDEX_DATA *mob;
  SHOP_DATA *pShop;
  char buf[MAX_STRING_LENGTH];

  pShop = ch->pcdata->edit.mob->pShop;
  ch->pcdata->interp_fun = do_menu;
  vnum = atoi (arg);
  mob = get_mob_index (vnum);
  if (mob == NULL) {
    sprintf (buf,"%d is an invalid vnum.\n\r>  ",vnum);
    send_to_char (buf,ch);
    return;
  }
  if (mob->pShop == NULL) {
    send_to_char ("Mob is not a shop keeper.\n\r>  ",ch);
    return;
  }
  for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
    pShop->buy_type[iTrade] = mob->pShop->buy_type[iTrade];
  pShop->profit_buy   = mob->pShop->profit_buy;
  pShop->profit_sell  = mob->pShop->profit_sell;
  pShop->open_hour    = mob->pShop->open_hour;
  pShop->close_hour   = mob->pShop->close_hour;
  send_to_char ("Ok.\n\r>  ",ch);
}

void edit_shop_buy_type ( CHAR_DATA *ch, char *arg )
{
  char buf[MAX_STRING_LENGTH];
  int num,iTrade;

  ch->pcdata->interp_fun = do_menu;
  if (strcmp (arg,"clear") == 0) {
    for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
      ch->pcdata->edit.mob->pShop->buy_type[iTrade] = 0;
      send_to_char ("Buy types cleared.\n\r>  ",ch);
      return;
  }
  num = item_lookup (arg);
  if (num == 0) {
    send_to_char ("Item type does not exist.\n\r>  ",ch);
    return;
  }
  for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ ) {
    if (ch->pcdata->edit.mob->pShop->buy_type[iTrade] == 0) {
      ch->pcdata->edit.mob->pShop->buy_type[iTrade] = num;
      sprintf (buf,"Shop will now buy type '%s'.\n\r>  ",item_name (num));
      send_to_char (buf,ch);
      return;
    }
  }
  sprintf (buf,"Only %d buy types allowed.\n\r>  ",MAX_TRADE);
  send_to_char (buf,ch);
}

void edit_shop_sell_profit ( CHAR_DATA *ch, char *arg )
{
  int profit;
  char buf[MAX_STRING_LENGTH];

  profit = atoi (arg);
  ch->pcdata->interp_fun = do_menu;
  ch->pcdata->edit.mob->pShop->profit_sell = profit;
  sprintf (buf,"Sell profit set to %d.\n\r>  ",profit);
  send_to_char (buf,ch);
}

void edit_shop_buy_profit ( CHAR_DATA *ch, char *arg )
{
  int profit;
  char buf[MAX_STRING_LENGTH];

  profit = atoi (arg);
  ch->pcdata->interp_fun = edit_shop_sell_profit;
  ch->pcdata->edit.mob->pShop->profit_buy = profit;
  sprintf (buf,"Buy profit set to %d.\n\rEnter selling profit:  ",profit);
  send_to_char (buf,ch);
}

void edit_shop_close ( CHAR_DATA *ch, char *arg )
{
  int hour;
  char buf[MAX_STRING_LENGTH];

  hour = atoi (arg);
  ch->pcdata->interp_fun = do_menu;
  ch->pcdata->edit.mob->pShop->close_hour = hour;
  sprintf (buf,"Closing hour set to %d.\n\r>  ",hour);
  send_to_char (buf,ch);
}

void edit_shop_open ( CHAR_DATA *ch, char *arg )
{
  int hour;
  char buf[MAX_STRING_LENGTH];

  hour = atoi (arg);
  ch->pcdata->interp_fun = edit_shop_close;
  ch->pcdata->edit.mob->pShop->open_hour = hour;
  sprintf (buf,"Opening hour set to %d.\n\rEnter closing hour:  ",hour);
  send_to_char (buf,ch);
}

void edit_shop_info ( CHAR_DATA *ch )
{
  SHOP_DATA *shop;
  char buf[MAX_STRING_LENGTH];
  int iTrade;

  shop = ch->pcdata->edit.mob->pShop;
  sprintf (buf,"Shop keeper vnum:  %d\n\r",shop->keeper);
  send_to_char (buf,ch);
  send_to_char ("Buy types:  ",ch);
  for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ ) {
    sprintf (buf,"[%s] ",item_type_name_num (shop->buy_type[iTrade]));
    send_to_char (buf,ch);
  }
  sprintf (buf,"\n\rProfit Buying: %d - Profit Selling: %d\n\r",
    shop->profit_buy,shop->profit_sell);
  send_to_char (buf,ch);
  sprintf (buf,"Opens at %d - Closes at %d\n\r>  ",
    shop->open_hour,shop->close_hour);
  send_to_char (buf,ch);
}

void edit_shop_remove ( CHAR_DATA *ch )
{
  ch->pcdata->edit.mob->pShop = NULL;
  send_to_char ("Shop removed.\n\r",ch);
  ch->pcdata->menu = &mob_menu;
  ch->pcdata->interp_fun = do_menu;
  do_menu (ch,NULL);
}

void edit_mob_shop ( CHAR_DATA *ch, int num )
{
  switch (num) {
    case ID_SHOP_CLONE:
      send_to_char ("Enter vnum of mob to copy shop from:  ",ch);
      ch->pcdata->interp_fun = edit_shop_clone;
      break;
    case ID_SHOP_BUY:
      send_to_char ("Enter item type [ex: weapon] to buy or 'clear':  ",ch);
      ch->pcdata->interp_fun = edit_shop_buy_type;
      break;
    case ID_SHOP_PROFIT:
      send_to_char ("Enter buying profit:  ",ch);
      ch->pcdata->interp_fun = edit_shop_buy_profit;
      break;
    case ID_SHOP_HOURS:
      send_to_char ("Enter opening hour:  ",ch);
      ch->pcdata->interp_fun = edit_shop_open;
      break;
    case ID_SHOP_REMOVE:
      edit_shop_remove (ch);
      break;
    case ID_SHOP_INFO:
      edit_shop_info (ch);
      break;
    case ID_EDIT_PREVIOUS:
      ch->pcdata->menu = &mob_menu;
      do_menu (ch,NULL);
      break;
  }
}

void edit_mob_update_instances ( CHAR_DATA *ch )
{
  CHAR_DATA *mob;

  mob = char_list;
  while (mob) {
    if (mob->pIndexData == ch->pcdata->edit.mob) {
      MobIndexToInstance (mob,mob->pIndexData);
    }
    mob = mob->next;
  }
}

void edit_mob ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_EDIT_VNUM:
      send_to_char ("Enter new vnum:  ",ch);
      ch->pcdata->interp_fun = edit_mob_vnum;
      break;
    case ID_EDIT_CREATE:
      if (check_range (ch,RANGE_AREA,-1)) {
        edit_mob_create (ch);
        send_to_char (">  ",ch);
      }
      break;
    case ID_EDIT_CLONE:
      if (!ch->pcdata->edit.mob) {
        send_to_char ("Select a mob to be copied to first.\n\r>  ",ch);
      } else
      if (check_range (ch,RANGE_MOB,-1)) {
        send_to_char ("Enter vnum to copy from:  ",ch);
        ch->pcdata->interp_fun = edit_mob_copy;
        sprintf (buf,"Copied to mob [%d].",ch->pcdata->edit.mob->vnum);
        olc_log_string (ch,buf);
      }
      break;
    case ID_EDIT_MODIFY:
      if (!ch->pcdata->edit.mob) {
        send_to_char ("Select a mob to be modified first.\n\r>  ",ch);
      } else {
        if (check_range (ch,RANGE_MOB,-1)) {
          sprintf (buf,"Modify mob [%d].",ch->pcdata->edit.mob->vnum);
          olc_log_string (ch,buf);
          ch->pcdata->menu = &mob_modify_menu;
          do_menu (ch,NULL);
        }
      }
      break;
    case ID_EDIT_LIST:
      edit_mob_list (ch);
      break;
    case ID_EDIT_INFO:
      edit_mob_info (ch);
      send_to_char (">  ",ch);
      break;
    case ID_EDIT_SHOP:
      if (!ch->pcdata->edit.mob) {
        send_to_char ("Select a mob first.\n\r>  ",ch);
        return;
      }
      if (check_range (ch,RANGE_MOB,-1)) {
        sprintf (buf,"Modify shop on mob [%d].",ch->pcdata->edit.mob->vnum);
        olc_log_string (ch,buf);
        if (!ch->pcdata->edit.mob->pShop) {
          send_to_char ("Mob is not a shopkeeper, make it one (Y/N)?  ",ch);
          ch->pcdata->interp_fun = edit_mob_conf_shop;
        } else {
          ch->pcdata->menu = &mob_shop_menu;
          do_menu (ch,NULL);
        }
      }
      break;
    case ID_EDIT_INSTANCE:
      if (!ch->pcdata->edit.mob) {
        send_to_char ("Select a mob to be updated first.\n\r>  ",ch);
        return;
      }
      if (check_range (ch,RANGE_MOB,-1)) {
        edit_mob_update_instances (ch);
        send_to_char ("Ok.\n\r>  ",ch);
      }
      break;
  }
}

void edit_obj_init ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  if (ch->pcdata->edit.obj)
    sprintf (buf," - Selected Object:  %s [%d]\n\r\n\r",
             ch->pcdata->edit.obj->short_descr,ch->pcdata->edit.obj->vnum );
  else
    sprintf (buf," - Selected Object:  None\n\r\n\r");
  send_to_char (buf,ch);
}


void edit_obj_vnum ( CHAR_DATA *ch, char *arg )
{
   OBJ_INDEX_DATA *obj;
   int vnum;
   char buf[MAX_STRING_LENGTH];

   vnum = atoi (arg);
   obj = get_obj_index (vnum);
   if (!obj) {
     sprintf (buf,"Invalid object vnum %d.\n\r>  ",vnum);
   } else {
     ch->pcdata->edit.obj = obj;
     sprintf (buf,"Selected Object:  %s [%d]\n\r>  ",obj->short_descr,vnum);
   }
   send_to_char (buf,ch);
   ch->pcdata->interp_fun = do_menu;
}

void edit_obj_info  ( CHAR_DATA *ch )
{
  OBJ_INDEX_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  AFFECT_DATA *paf;

  obj = ch->pcdata->edit.obj;
  if (!obj) {
    send_to_char ("No object selected.\n\r",ch);
    return;
  }

  sprintf( buf, "Name: %s.\n\r",
    obj->name );
  send_to_char( buf, ch );

    sprintf( buf, "Vnum: %d  Format: %s  Type: %s  Resets: %d\n\r",
  obj->vnum, obj->new_format ? "new" : "old",
  item_type_name_num (obj->item_type), obj->reset_num );
    send_to_char( buf, ch );

    sprintf( buf, "Short descrip: %s\n\rLong descrip: %s\n\r",
  obj->short_descr, obj->description );
    send_to_char( buf, ch );

    sprintf( buf, "Material: %s\n\r", obj->material );
    send_to_char( buf, ch );

    sprintf( buf, "Wear bits: %s  Extra bits: %s\n\r",
  wear_bit_name(obj->wear_flags), extra_bit_name( obj->extra_flags ) );
    send_to_char( buf, ch );

/*    sprintf( buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds)\n\r",
  1,           get_obj_number( obj ),
  obj->weight, get_obj_weight( obj ),get_true_weight(obj) );
    send_to_char( buf, ch ); */

    sprintf( buf, "Level: %d  Cost: %d  Condition: %d  Wear Timer: %d\n\r",
  obj->level, obj->cost, obj->condition, obj->wear_timer);
    send_to_char( buf, ch );


    sprintf( buf, "Values: %d %d %d %d %d\n\r",
  obj->value[0], obj->value[1], obj->value[2], obj->value[3],
  obj->value[4] );
    send_to_char( buf, ch );

    /* now give out vital statistics as per identify */

    switch ( obj->item_type )
    {
      case ITEM_SCROLL:
      case ITEM_POTION:
      case ITEM_PILL:
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
  break;

      case ITEM_WAND:
      case ITEM_STAFF:
      sprintf( buf, "Has %d(%d) charges of level %d",
        obj->value[1], obj->value[2], obj->value[0] );
      send_to_char( buf, ch );

      if ( obj->value[3] >= 0 && obj->value[3] < MAX_SKILL )
      {
        send_to_char( " '", ch );
        send_to_char( skill_table[obj->value[3]].name, ch );
        send_to_char( "'", ch );
      }

      send_to_char( ".\n\r", ch );
  break;

  case ITEM_DRINK_CON:
      sprintf(buf,"It holds %s-colored %s.\n\r",
    liq_table[obj->value[2]].liq_color,
    liq_table[obj->value[2]].liq_name);
      send_to_char(buf,ch);
      break;


      case ITEM_WEAPON:
      send_to_char("Weapon type is ",ch);
      switch (obj->value[0])
      {
        case(WEAPON_EXOTIC):
        send_to_char("exotic\n\r",ch);
        break;
        case(WEAPON_SWORD):
        send_to_char("sword\n\r",ch);
        break;
        case(WEAPON_DAGGER):
        send_to_char("dagger\n\r",ch);
        break;
        case(WEAPON_SPEAR):
        send_to_char("spear/staff\n\r",ch);
        break;
        case(WEAPON_MACE):
        send_to_char("mace/club\n\r",ch);
        break;
      case(WEAPON_AXE):
        send_to_char("axe\n\r",ch);
        break;
        case(WEAPON_FLAIL):
        send_to_char("flail\n\r",ch);
        break;
        case(WEAPON_WHIP):
        send_to_char("whip\n\r",ch);
        break;
        case(WEAPON_POLEARM):
        send_to_char("polearm\n\r",ch);
        break;
        case(WEAPON_GAROTTE):
        send_to_char("garotte\n\r",ch);
        break;
        default:
        send_to_char("unknown\n\r",ch);
        break;
      }
      if (obj->new_format)
        sprintf(buf,"Damage is %dd%d (average %d)\n\r",
        obj->value[1],obj->value[2],
        (1 + obj->value[2]) * obj->value[1] / 2);
      else
        sprintf( buf, "Damage is %d to %d (average %d)\n\r",
            obj->value[1], obj->value[2],
            ( obj->value[1] + obj->value[2] ) / 2 );
      send_to_char( buf, ch );

      sprintf(buf,"Damage noun is %s.\n\r",
    attack_table[obj->value[3]].noun);
      send_to_char(buf,ch);

      if (obj->value[4])  /* weapon flags */
      {
          sprintf(buf,"Weapons flags: %s\n\r",
        weapon_bit_name(obj->value[4]));
          send_to_char(buf,ch);
            }
  break;

      case ITEM_ARMOR:
      sprintf( buf,
      "Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\rSize is (%d) %s\n\r",
          obj->value[0], obj->value[1], obj->value[2], obj->value[3],
          obj->value[4], obj_size_table[obj->value[4]].name );
      send_to_char( buf, ch );
  break;

        case ITEM_CONTAINER:
            sprintf(buf,"Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
                obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
            send_to_char(buf,ch);
            if (obj->value[4] != 100)
            {
                sprintf(buf,"Weight multiplier: %d%%\n\r",
        obj->value[4]);
                send_to_char(buf,ch);
            }
        break;
    }


    if ( obj->extra_descr != NULL)
    {
  EXTRA_DESCR_DATA *ed;

  send_to_char( "Extra description keywords: '", ch );

  for ( ed = obj->extra_descr; ed != NULL; ed = ed->next )
  {
      send_to_char( ed->keyword, ch );
      if ( ed->next != NULL )
        send_to_char( " ", ch );
  }

  send_to_char( "'\n\r", ch );
    }

    for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
  sprintf( buf, "Affects %s by %d, level %d",
      affect_loc_name( paf->location ), paf->modifier,paf->level );
  send_to_char(buf,ch);
  if ( paf->duration > -1)
      sprintf(buf,", %d hours.\n\r",paf->duration);
  else
      sprintf(buf,".\n\r");
  send_to_char( buf, ch );
  if (paf->bitvector)
  {
      switch(paf->where)
      {
    case TO_AFFECTS:
        sprintf(buf,"Adds %s affect.\n",
      affect_bit_name(paf->bitvector));
        break;
                case TO_WEAPON:
                    sprintf(buf,"Adds %s weapon flags.\n",
                        weapon_bit_name(paf->bitvector));
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
      send_to_char(buf,ch);
  }
    }


    return;
}

/*    OBJ_INDEX_DATA *    next;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    bool                new_format;
    char *              name;
    char *              short_descr;
    char *              description;
    sh_int              vnum;
    sh_int              reset_num;
    char *              material;
    sh_int              item_type;
    int                 extra_flags;
    int                 wear_flags;
    sh_int              level;
    sh_int              condition;
    sh_int              count;
    sh_int              weight;
    sh_int              wear_timer;
    int                 cost;
    int                 value[5]; */



void edit_obj_list (CHAR_DATA *ch)
{
  OBJ_INDEX_DATA *obj;
  int idx;
  char buf[MAX_STRING_LENGTH],bigbuf[4*MAX_STRING_LENGTH];
  int col = FALSE;

  bigbuf[0] = NULL;
  for (idx = ch->pcdata->edit.area->min_vnum_obj;
       idx <= ch->pcdata->edit.area->max_vnum_obj; idx++) {
    if (strlen (bigbuf) > (4*MAX_STRING_LENGTH - 200)) {
      strcat (bigbuf,"\n\rWarning: Buffer Full\n\r");
      break;
    }
    obj = get_obj_index (idx);
    if (obj) {
      sprintf (buf,"[%5d] %-30s",obj->vnum,obj->short_descr);
      if (col) {
        col = FALSE;
        if (strlen (buf) > 38) {
          strcat (bigbuf,"\n\r");
        }
        strcat (bigbuf,buf);
      } else {
        col = TRUE;
        if (strlen (buf) > 38) {
          col = FALSE;
        }
        strcat (bigbuf,"\n\r");
        strcat (bigbuf,buf);
      }
    }
  }
  strcat (bigbuf,"\n\r>  ");
  page_to_char (bigbuf,ch);
}

void edit_obj_create (CHAR_DATA *ch)
{
  OBJ_INDEX_DATA *obj;
  int vnum, iHash, t;
  char buf[MAX_STRING_LENGTH];

  vnum = 0;
  for (t = ch->pcdata->edit.area->min_vnum_obj;
       t <= (ch->pcdata->edit.area->max_vnum_obj+1); t++) {
    if (!get_obj_index (t)) {
      vnum = t;
      break;
    }
  }
  if (vnum == 0) {
    send_to_char ("Out of object vnums for this area.\n\r",ch);
    return;
  }

  if (!check_range (ch,RANGE_OBJ,vnum)) {
    send_to_char ("Object could not be created.\n\r",ch);
    return;
  }

  sprintf (buf,"Created object [%d].",vnum);
  olc_log_string (ch,buf);

  if (vnum >  ch->pcdata->edit.area->max_vnum_obj)
    ch->pcdata->edit.area->max_vnum_obj = vnum;

  obj = alloc_perm (sizeof (OBJ_INDEX_DATA));
  obj->new_format  = TRUE;
  obj->vnum        = vnum;
  obj->area        = ch->pcdata->edit.area;
  obj->extra_descr = NULL;
  obj->affected    = NULL;
  obj->new_format  = TRUE;
  obj->name        = str_dup ("generic object");
  obj->short_descr = str_dup ("Generic Short Description");
  obj->description = str_dup ("Generic Description");
  obj->reset_num   = 0;
  obj->material    = str_dup ("");
  obj->item_type   = 0;
  obj->extra_flags = 0;
  obj->wear_flags  = ITEM_TAKE;
  obj->level       = 0;
  obj->condition   = 100;
  obj->count       = 0;
  obj->weight      = 100;
  obj->wear_timer  = 0;
  obj->cost        = 0;
  for (t = 0; t < 5; t++)
    obj->value[t] = 0;

  iHash            = vnum % MAX_KEY_HASH;
  obj->next        = obj_index_hash[iHash];
  obj_index_hash[iHash] = obj;
  top_obj_index++;
  ch->pcdata->edit.obj = obj;
  send_to_char ("Ok.\n\r",ch);
}

void edit_obj_copy (CHAR_DATA *ch, char *arg)
{
  OBJ_INDEX_DATA *obj,*victim;
  EXTRA_DESCR_DATA *ed_copy, *ed_last;
  AFFECT_DATA *af_copy, *af_last;
  int vnum,t;
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  obj= ch->pcdata->edit.obj;
  if (!obj) {
    send_to_char ("No object selected to copy to.\n\r>  ",ch);
    return;
  }
  vnum = atoi (arg);
  victim = get_obj_index (vnum);
  if (!victim) {
    sprintf (buf,"Invalid object vnum %d.\n\r>  ",vnum);
    send_to_char (buf,ch);
    return;
  }

  obj->extra_descr = NULL;
  ed_last = NULL;
  for(ed_copy = victim->extra_descr; ed_copy; ed_copy = ed_copy->next)
  {
    if(!obj->extra_descr)
    {
      obj->extra_descr = new_extra_descr();
      ed_last = obj->extra_descr; 
    }
    else
    {
      ed_last->next = new_extra_descr();
      ed_last = ed_last->next;
    }
    ed_last->keyword = str_dup(ed_copy->keyword);
    ed_last->description = str_dup(ed_copy->description);
    ed_last->next = NULL;
  }
  
  obj->affected = NULL;
  af_last = NULL;
  for(af_copy = victim->affected; af_copy; af_copy = af_copy->next)
  {
    if(!obj->affected)
    {
      obj->affected = new_affect();
      af_last = obj->affected; 
    }
    else
    {
      af_last->next = new_affect();
      af_last = af_last->next;
    }
    af_last->type = af_copy->type;
    af_last->where = af_copy->where;
    af_last->level = af_copy->level;
    af_last->duration = af_copy->duration;
    af_last->location = af_copy->location;
    af_last->modifier = af_copy->modifier;
    af_last->bitvector = af_copy->bitvector;
    af_last->next = NULL;
  }

  obj->new_format  = victim->new_format;
  obj->name = NULL;
  if(victim->name)
    obj->name        = str_dup(victim->name);
  obj->short_descr = NULL;
  if(victim->short_descr)
    obj->short_descr = str_dup(victim->short_descr);
  obj->description = NULL;
  if(obj->description)
    obj->description = str_dup(victim->description);
  obj->material = NULL;
  if(obj->material)
    obj->material    = str_dup(victim->material);
  obj->item_type   = victim->item_type;
  obj->extra_flags = victim->extra_flags;
  obj->wear_flags  = victim->wear_flags;
  obj->level       = victim->level;
  obj->condition   = victim->condition;
  obj->weight      = victim->weight;
  obj->wear_timer  = victim->wear_timer;
  obj->cost        = victim->cost;
  for (t = 0; t < 5; t++)
    obj->value[t] = victim->value[t];

  sprintf (buf,"Copied Object %s [%d] to [%d]\n\r>  ",
           victim->short_descr,victim->vnum,obj->vnum);
  send_to_char (buf,ch);
}

void edit_obj_name ( CHAR_DATA *ch, char *arg )
{
  OBJ_INDEX_DATA *obj;
  char buf[MAX_STRING_LENGTH];

  obj = ch->pcdata->edit.obj;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Name not changed.\n\r>  ",ch);
    return;
  }

  free_string (obj->name);
  obj->name = str_dup_perm (arg);
  sprintf (buf,"Name set to '%s'.\n\r>",obj->name);
  send_to_char (buf,ch);
}

void edit_obj_material ( CHAR_DATA *ch, char *arg )
{
   OBJ_INDEX_DATA *obj;
  char buf[MAX_STRING_LENGTH];

  obj = ch->pcdata->edit.obj;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Material not changed.\n\r>  ",ch);
    return;
  }

  obj->material = str_dup_perm (arg);
  sprintf (buf,"Material set to '%s'.\n\r> ",obj->material);
  send_to_char (buf,ch);
}

void edit_obj_short ( CHAR_DATA *ch, char *arg )
{
  OBJ_INDEX_DATA *obj;
  char buf[MAX_STRING_LENGTH];

  obj = ch->pcdata->edit.obj;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Short description not changed.\n\r>  ",ch);
    return;
  }

  obj->short_descr = str_dup_perm (arg);
  sprintf (buf,"Short set to '%s'.\n\r> ",obj->short_descr);
  send_to_char (buf,ch);
}

void edit_obj_long (CHAR_DATA *ch, char *arg)
{
  char buf[MAX_STRING_LENGTH];

  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Long description not changed.\n\r>  ",ch);
    return;
  }

  ch->pcdata->edit.obj->description = str_dup_perm (arg);
  sprintf (buf,"Long set to '%s'.\n\r>  ",arg);
  send_to_char (buf,ch);
}

void edit_obj_level ( CHAR_DATA *ch, char *arg )
{
  OBJ_INDEX_DATA *obj;
  char buf[MAX_STRING_LENGTH];
  int level;

  obj = ch->pcdata->edit.obj;
  ch->pcdata->interp_fun = do_menu;
  if (!arg[0]) {
    send_to_char ("Level not changed.\n\r>  ",ch);
    return;
  }
  level = atoi (arg);
  if ((level < 0) || (level > 60)) {
    send_to_char ("Level must be from 0 to 60.\n\r>  ",ch);
    return;
  }

  obj->level = level;
  sprintf (buf,"Level set to %d.\n\r> ",obj->level);
  send_to_char (buf,ch);
}

void edit_obj_type ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];
  int t, found;

  if (num != ID_EDIT_PREVIOUS) {
    ch->pcdata->edit.obj->item_type = num;
    t = 0;
    found = FALSE;
    while (item_table[t].type && !found) {
      if (item_table[t].type == num) {
        num = t;
        found = TRUE;
        break;
      }
      t++;
    }
    if (found)
      sprintf (buf,"Item type set to '%s'\n\r",item_table[num].name);
    else
      sprintf (buf,"Item type not defined.\n\r");
    send_to_char (buf,ch);
  }

  ch->pcdata->menu = &obj_modify_menu;
  do_menu (ch,NULL);
  return;
}

char *get_val_string (OBJ_INDEX_DATA *obj,int val_num)
{
  int num,t = 0;

  num = obj->item_type;
  while (val_table[t].num) {
    if (val_table[t].num == num) {
      if (val_table[t].val[val_num])
        return val_table[t].val[val_num];
      else
        return "Not used";
    }
    t++;
  }

  return "Not used";
}

void obj_value_4 ( CHAR_DATA *ch, char *arg )
{
  int num;

  num = atoi (arg);
  if (arg[0]) ch->pcdata->edit.obj->value[4] = num;
  send_to_char ("Ok.\n\r>  ",ch);
  ch->pcdata->interp_fun = &do_menu;
}
void obj_value_3 ( CHAR_DATA *ch, char *arg )
{
  int num;  char buf[MAX_STRING_LENGTH];
  char *str;

  num = atoi (arg);
  str = get_val_string (ch->pcdata->edit.obj,4);
  if (arg[0]) ch->pcdata->edit.obj->value[3] = num;
  sprintf (buf,"Value 4 (%s) [%d]:  ",str,ch->pcdata->edit.obj->value[4]);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = obj_value_4;
}
void obj_value_2 ( CHAR_DATA *ch, char *arg )
{
  int num;  char buf[MAX_STRING_LENGTH];
  char *str;

  num = atoi (arg);
  str = get_val_string (ch->pcdata->edit.obj,3);
  if (arg[0]) ch->pcdata->edit.obj->value[2] = num;
  sprintf (buf,"Value 3 (%s) [%d]:  ",str,ch->pcdata->edit.obj->value[3]);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = obj_value_3;
}
void obj_value_1 ( CHAR_DATA *ch, char *arg )
{
  int num;  char buf[MAX_STRING_LENGTH];
  char *str;

  num = atoi (arg);
  str = get_val_string (ch->pcdata->edit.obj,2);
  if (arg[0]) ch->pcdata->edit.obj->value[1] = num;
  sprintf (buf,"Value 2 (%s) [%d]:  ",str,ch->pcdata->edit.obj->value[2]);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = &obj_value_2;
}
void obj_value_0 ( CHAR_DATA *ch, char *arg )
{
  int num;  char buf[MAX_STRING_LENGTH];
  char *str;

  num = atoi (arg);
  str = get_val_string (ch->pcdata->edit.obj,1);
  if (arg[0]) ch->pcdata->edit.obj->value[0] = num;
  sprintf (buf,"Value 1 (%s) [%d]:  ",str,ch->pcdata->edit.obj->value[1]);
  send_to_char (buf,ch);
  ch->pcdata->interp_fun = &obj_value_1;
}

void edit_obj_cond ( CHAR_DATA *ch, char *arg )
{
  int num;
  char buf[MAX_STRING_LENGTH];

  num = atoi (arg);
  if (num > 0 && num <= 100) {
    ch->pcdata->edit.obj->condition = num;
    send_to_char ("Ok.\n\r>  ",ch);
  } else {
    sprintf (buf,"%d is not a valid condition.  Must be 1 to 100.\n\r>  ",num);
    send_to_char (buf,ch);
  }
  ch->pcdata->interp_fun = do_menu;
}

void edit_obj_weight ( CHAR_DATA *ch, char *arg )
{
  int num;
  char buf[MAX_STRING_LENGTH];

  num = atoi (arg);
  sprintf (buf,"Weight is %d.%d pounds.\n\r>  ",num/10,num % 10);
  send_to_char (buf,ch);
  ch->pcdata->edit.obj->weight = num;
  ch->pcdata->interp_fun = do_menu;
}

void edit_obj_wear_timer ( CHAR_DATA *ch, char *arg )
{
  int num;
  char buf[MAX_STRING_LENGTH];

  num = atoi (arg);
  sprintf (buf,"Wear Timer is %d.\n\r>  ",num);
  send_to_char (buf,ch);
  ch->pcdata->edit.obj->wear_timer = num;
  ch->pcdata->interp_fun = do_menu;
}

void edit_obj_cost ( CHAR_DATA *ch, char *arg )
{
  int num;
  char buf[MAX_STRING_LENGTH];

  num = atoi (arg);
  sprintf (buf,"Cost set to %d.\n\r>  ",num);
  send_to_char (buf,ch);
  ch->pcdata->edit.obj->cost = num;
  ch->pcdata->interp_fun = do_menu;
}

void edit_obj_extend_desc ( CHAR_DATA *ch, char *arg )
{


  /* no need to free string */
  ch->pcdata->edit.obj->extra_descr->description = str_dup (arg);
  send_to_char (">  ",ch);
}

void edit_obj_extend_add ( CHAR_DATA *ch, char *arg )
{
  EXTRA_DESCR_DATA *ed,*first_ed,*prev_ed;
  char buf[MAX_STRING_LENGTH];

  prev_ed = NULL;
  first_ed = ed = ch->pcdata->edit.obj->extra_descr;
  while (ed) {
    if (!str_prefix (arg,ed->keyword)) {
      if (prev_ed) {
        prev_ed->next = ed->next;
        ed->next = first_ed;
        ch->pcdata->edit.obj->extra_descr = ed;
        sprintf (buf,"Editing extended '%s'.\n\r",ed->keyword);
        send_to_char (buf,ch);
      }
      break;
    }
    prev_ed = ed;
    ed = ed->next;
  }

  if (ed == NULL) {
    ed = new_extra_descr();
    ed->keyword = str_dup (arg);
    ed->description = str_dup_perm ("Generic extended description.\n\r");
    ed->next = first_ed;
    ch->pcdata->edit.obj->extra_descr = ed;
    send_to_char ("Creating new extended.\n\r",ch);
  }

  do_line_editor (ch,ch->pcdata->edit.obj->extra_descr->description,
                  &edit_obj_extend_desc);
}

void edit_obj_extend_rem ( CHAR_DATA *ch, char *arg )
{
  EXTRA_DESCR_DATA *ed,*first_ed,*prev_ed;
  char buf[MAX_STRING_LENGTH];
  bool found = FALSE;

  prev_ed = NULL;
  ch->pcdata->interp_fun = do_menu;
  first_ed = ed = ch->pcdata->edit.obj->extra_descr;
  while (ed) {
    if (!str_prefix (arg,ed->keyword)) {
      if (prev_ed) {
        prev_ed->next = ed->next;
      } else {
        ch->pcdata->edit.obj->extra_descr = ed->next;
      }
      sprintf (buf,"Extended description '%s' removed.\n\r>  ",ed->keyword);    
      send_to_char (buf,ch);
      found = TRUE;
      free_extra_descr (ed);
      break;
    }
    prev_ed = ed;
    ed = ed->next;
  }

  if (!found) {
    send_to_char ("Extended description not found.\n\r>  ",ch);
  }
}

void edit_obj_add_mod ( CHAR_DATA *ch, char *arg )
{
  int num;

  num = atoi (arg);
  ch->pcdata->interp_fun = do_menu;
  if (ch->pcdata->edit.obj->affected)
    ch->pcdata->edit.obj->affected->modifier = num;
  else {
    send_to_char ("Error adding modifier.\n\r>  ",ch);
    return;
  }

  send_to_char ("Affect added.\n\r>  ",ch);
}

void edit_obj_add_aff ( CHAR_DATA *ch, int num )
{
  AFFECT_DATA *paf;

  if ( num == ID_EDIT_PREVIOUS ) {
    ch->pcdata->menu = &obj_modify_menu;
    do_menu (ch,NULL);
    return;
  }

  if ( num == APPLY_SPELL_AFFECT ) {
    paf = new_affect ();
    paf->type = -1;
    paf->where = TO_AFFECTS;
    paf->level = ch->pcdata->edit.obj->level;
    paf->duration = -1;
    paf->location = 0;
    paf->modifier = 0;
    paf->bitvector = 0;
    paf->next = ch->pcdata->edit.obj->affected;
    ch->pcdata->edit.obj->affected = paf;

    ch->pcdata->edit.mod_flags = &paf->bitvector;
    build_flag_menu ( aff_table, "Set Affect Flags", ch );
    do_menu (ch,NULL);
    return;
  }

  paf = new_affect ();
  paf->type = -1;
  paf->where = 0;
  paf->level = ch->pcdata->edit.obj->level;
  paf->duration = -1;
  paf->location = num;
  paf->modifier = 0;
  paf->bitvector = 0;
  paf->next = ch->pcdata->edit.obj->affected;
  ch->pcdata->edit.obj->affected = paf;

  send_to_char ("Modifies by:  ",ch);
  ch->pcdata->interp_fun = edit_obj_add_mod;
};

bool edit_obj_disp_aff ( CHAR_DATA *ch, OBJ_INDEX_DATA *obj )
{
  AFFECT_DATA *paf;
  char buf[MAX_STRING_LENGTH];
  int idx = 0;

  if (!obj->affected) return FALSE;

     for ( paf = obj->affected; paf != NULL; paf = paf->next )
    {
    idx++;
  sprintf( buf, "%d: Affects %s by %d, level %d",idx,
      affect_loc_name( paf->location ), paf->modifier,paf->level );
  send_to_char(buf,ch);
  if ( paf->duration > -1)
      sprintf(buf,", %d hours.\n\r",paf->duration);
  else
      sprintf(buf,".\n\r");
  send_to_char( buf, ch );
  if (paf->bitvector)
  {
      switch(paf->where)
      {
    case TO_AFFECTS:
        sprintf(buf,"  Adds %s affect.\n",
      affect_bit_name(paf->bitvector));
        break;
                case TO_WEAPON:
                    sprintf(buf,"  Adds %s weapon flags.\n",
                        weapon_bit_name(paf->bitvector));
        break;
    case TO_OBJECT:
        sprintf(buf,"  Adds %s object flag.\n",
      extra_bit_name(paf->bitvector));
        break;
    case TO_IMMUNE:
        sprintf(buf,"  Adds immunity to %s.\n",
      imm_bit_name(paf->bitvector));
        break;
    case TO_RESIST:
        sprintf(buf,"  Adds resistance to %s.\n\r",
      imm_bit_name(paf->bitvector));
        break;
    case TO_VULN:
        sprintf(buf,"  Adds vulnerability to %s.\n\r",
      imm_bit_name(paf->bitvector));
        break;
    default:
        sprintf(buf,"  Unknown bit %d: %ld\n\r",
      paf->where,paf->bitvector);
        break;
      }
      send_to_char(buf,ch);
  }
    }

  return TRUE;
}

void edit_obj_rem_aff ( CHAR_DATA *ch, char *arg )
{
  int idx,num;
  OBJ_INDEX_DATA *obj;
  AFFECT_DATA *paf,*prev_paf;

  num = atoi (arg);
  idx = 0;
  obj = ch->pcdata->edit.obj;
  paf = obj->affected;
  prev_paf = NULL;
  ch->pcdata->interp_fun = do_menu;
  while (paf) {
    if (num == ++idx) {
      if (prev_paf) {
        prev_paf->next = paf->next;
        free_affect (paf);
        send_to_char ("Affect removed.\n\r>  ",ch);
        return;
      } else {
        obj->affected = paf->next;
        free_affect (paf);
        send_to_char ("Affect removed.\n\r>  ",ch);
        return;
      }
    }
    prev_paf = paf;
    paf = paf->next;
  }

  send_to_char ("Affect not found.\n\r>  ",ch);
  return;
}

void edit_obj_modify  ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];
  char *str;

  switch (num) {
    case ID_OBJ_NAME:
      send_to_char ("Enter Name (Keywords separated by spaces):  ",ch);
      ch->pcdata->interp_fun = edit_obj_name;
      break;
    case ID_OBJ_LEVEL:
      send_to_char ("Enter Level:  ",ch);
      ch->pcdata->interp_fun = edit_obj_level;
      break;
    case ID_OBJ_SHORT:
      send_to_char ("Enter Short Description:  ",ch);
      ch->pcdata->interp_fun = edit_obj_short;
      break;
    case ID_OBJ_DESCR:
      send_to_char ("Enter Long Description:  ",ch);
      ch->pcdata->interp_fun = edit_obj_long;
      break;
    case ID_OBJ_MATERIAL:
      send_to_char ("Enter Material Word: ",ch);
      ch->pcdata->interp_fun = edit_obj_material;
      break;
    case ID_OBJ_TYPE:
      ch->pcdata->menu = &obj_type_menu;
      do_menu (ch,NULL);
      break;
    case ID_OBJ_FLAGS:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.obj->extra_flags;
      build_flag_menu ( obj_extra_flags, "Set Extra Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_OBJ_WEAR:
      ch->pcdata->edit.mod_flags = &ch->pcdata->edit.obj->wear_flags;
      build_flag_menu ( obj_wear_flags, "Set Wear Flags", ch );
      do_menu (ch,NULL);
      break;
    case ID_OBJ_COND:
      send_to_char ("Enter condition percentage [1-100]:  ",ch);
      ch->pcdata->interp_fun = edit_obj_cond;
      break;
    case ID_OBJ_WEIGHT:
      send_to_char ("Enter weight (10th pounds):  ",ch);
      ch->pcdata->interp_fun = edit_obj_weight;
      break;
    case ID_OBJ_WEAR_TIMER:
      send_to_char ("Enter wear timer (Don't forget the Extra Flag):  ",ch);
      ch->pcdata->interp_fun = edit_obj_wear_timer;
      break;
    case ID_OBJ_COST:
      send_to_char ("Enter cost:  ",ch);
      ch->pcdata->interp_fun = edit_obj_cost;
      break;
    case ID_OBJ_VALUE:
      str = get_val_string (ch->pcdata->edit.obj,0);
      sprintf (buf,"Value 0 (%s) [%d]:  ",str,ch->pcdata->edit.obj->value[0]);
      send_to_char (buf,ch);
      ch->pcdata->interp_fun = obj_value_0;
      break;
    case ID_OBJ_EXTENDED:
      send_to_char ("Enter keyword:  ",ch);
      ch->pcdata->interp_fun = edit_obj_extend_add;
      break;
    case ID_OBJ_REM_EXTEND:
      send_to_char ("Enter keyword to remove:  ",ch);
      ch->pcdata->interp_fun = edit_obj_extend_rem;
      break;
    case ID_OBJ_AFF_ADD:
      ch->pcdata->menu = &obj_affect_menu;
      do_menu (ch,NULL);
      break;
    case ID_OBJ_AFF_REMOVE:
      if (edit_obj_disp_aff (ch,ch->pcdata->edit.obj)) {
        send_to_char ("Choose affect to remove:  ",ch);
        ch->pcdata->interp_fun = edit_obj_rem_aff;
      } else
        send_to_char ("No affects to remove.\n\r>  ",ch);
      break;
    case ID_EDIT_PREVIOUS:
      ch->pcdata->menu = &object_menu;
      do_menu (ch,NULL);
      break;
  }
}

void edit_obj_update_instances ( CHAR_DATA *ch )
{
  OBJ_DATA *obj;

  obj = object_list;
  while (obj) {
    if (obj->pIndexData == ch->pcdata->edit.obj) {
      ObjIndexToInstance (obj,obj->pIndexData,obj->level,FALSE);
    }
    obj = obj->next;
  }
}

void edit_object ( CHAR_DATA *ch, int num )
{
  char buf[MAX_STRING_LENGTH];

  switch (num) {
    case ID_EDIT_VNUM:
      send_to_char ("Enter new vnum:  ",ch);
      ch->pcdata->interp_fun = edit_obj_vnum;
      break;
    case ID_EDIT_CREATE:
      if (check_range (ch,RANGE_AREA,-1)) {
        edit_obj_create (ch);
        send_to_char (">  ",ch);
      }
      break;
    case ID_EDIT_CLONE:
      if (!ch->pcdata->edit.obj) {
        send_to_char ("Select an object to be copied to first.\n\r>  ",ch);
      } else {
        if (check_range (ch,RANGE_OBJ,-1)) {
          send_to_char ("Enter vnum to copy from:  ",ch);
          ch->pcdata->interp_fun = edit_obj_copy;
          sprintf (buf,"Copied to object [%d].",ch->pcdata->edit.obj->vnum);
          olc_log_string (ch,buf);
        }
      }
      break;
    case ID_EDIT_MODIFY:
      if (!ch->pcdata->edit.obj) {
        send_to_char ("Select an object to be modified first.\n\r>  ",ch);
      } else {
        if (check_range (ch,RANGE_OBJ,-1)) {
          sprintf (buf,"Modify object [%d].",ch->pcdata->edit.obj->vnum);
          olc_log_string (ch,buf);
          ch->pcdata->menu = &obj_modify_menu;
          do_menu (ch,NULL);
        }
      }
      break;
    case ID_EDIT_LIST:
      edit_obj_list (ch);
      break;
    case ID_EDIT_INFO:
      edit_obj_info (ch);
      send_to_char (">  ",ch);
      break;
    case ID_EDIT_INSTANCE:
      if (!ch->pcdata->edit.obj) {
        send_to_char ("Select an object first.\n\r>  ",ch);
        return;
      }
      if (check_range (ch,RANGE_OBJ,-1)) {
        edit_obj_update_instances ( ch );
        send_to_char ("Ok.\n\r>  ",ch);
      }
      break;
  }
}


void edit_reset_init ( CHAR_DATA *ch, int num )
{
}

void remove_resets ( AREA_DATA *area, char c )
{
  RESET_DATA *reset,*prev_reset,*next_reset;

  reset = area->reset_first;
  prev_reset = NULL;
  while (reset) {
    next_reset = reset->next;
    if (reset->command == c) {
      if (reset == area->reset_last) {
        area->reset_last = prev_reset;
      }
      if (reset == area->reset_first) {
        area->reset_first = reset->next;
        free_reset (reset);
        top_reset--;
      } else {
        prev_reset->next = reset->next;
        free_reset (reset);
        top_reset--;
      }
    } else {
      prev_reset = reset;
    }
    reset = next_reset;
  }
}

void add_reset (AREA_DATA *area,char command,sh_int arg1,sh_int arg2,sh_int arg3,sh_int arg4)
{
  RESET_DATA *reset;

  reset          = new_reset ();
  reset->command = command;
  reset->arg1    = arg1;
  reset->arg2    = arg2;
  reset->arg3    = arg3;
  reset->arg4    = arg4;

  if ( area->reset_first == NULL )
      area->reset_first  = reset;
  if ( area->reset_last  != NULL )
      area->reset_last->next = reset;

  area->reset_last = reset;
  reset->next    = NULL;
  top_reset++;
}

void add_reset_obj_recurse ( AREA_DATA *area, OBJ_DATA *rec_obj)
{
  OBJ_DATA *obj;
  sh_int count;

  obj = rec_obj->contains;
  while (obj) {
    count = count_obj_list(obj->pIndexData,rec_obj->contains);
    add_reset (area,'P',obj->pIndexData->vnum,-1,rec_obj->pIndexData->vnum,count);
    add_reset_obj_recurse (area,obj);
    obj = obj->next_content;
  }
}

int add_mob_resets_room ( ROOM_INDEX_DATA *pRoom, AREA_DATA *area )
{
  CHAR_DATA *victim,*mob;
  int count,reset_count = 0;
  OBJ_DATA *obj;

  victim = pRoom->people;
  while (victim) {
    if ( IS_NPC(victim) ) {
      count = 0;
      mob = pRoom->people;
      while (mob) {
        if (mob->pIndexData == victim->pIndexData)  count++;
        mob = mob->next_in_room;
      }
      reset_count++;
      add_reset (area,'M',victim->pIndexData->vnum,
        victim->pIndexData->count,pRoom->vnum,count);
      obj = victim->carrying;
      while (obj) {
        if (obj->wear_loc == -1) {
          add_reset (area,'G',obj->pIndexData->vnum,-1,-1,-1);
          add_reset_obj_recurse (area,obj);
        } else {
          add_reset (area,'E',obj->pIndexData->vnum,-1,obj->wear_loc,-1);
          add_reset_obj_recurse (area,obj);
        }
        obj = obj->next_content;
      }
    }
    victim = victim->next_in_room;
  }
  return reset_count;
}


int add_mob_resets ( AREA_DATA *area )
{
  ROOM_INDEX_DATA *pRoom;
  sh_int vnum, count;

  count = 0;
  for (vnum = area->min_vnum_room;
       vnum <= area->max_vnum_room;  vnum++) {
    pRoom = get_room_index (vnum);
    if (pRoom) {
      count += add_mob_resets_room (pRoom,area);
    }
  }
  return count;
}

int add_object_resets_room ( ROOM_INDEX_DATA *pRoom, AREA_DATA *area )
{
  int reset_count = 0;
  OBJ_DATA *obj;
  obj = pRoom->contents;
  while (obj) {
    reset_count++;
    add_reset (area,'O',obj->pIndexData->vnum,100,pRoom->vnum,100);
    add_reset_obj_recurse (area,obj);
    obj = obj->next_content;
  }
  return reset_count;
}

int add_object_resets ( AREA_DATA *area )
{
  ROOM_INDEX_DATA *pRoom;
  sh_int vnum,count = 0;

  for (vnum = area->min_vnum_room;
       vnum <= area->max_vnum_room;  vnum++) {
    pRoom = get_room_index (vnum);
    if (pRoom) {
      count += add_object_resets_room (pRoom,area);
    }
  }
  return count;
}

int add_door_resets_room ( ROOM_INDEX_DATA *pRoom, AREA_DATA *area )
{
  EXIT_DATA *exit;
  sh_int dir;
  int reset_count = 0;

    for ( dir =0; dir<=5 ;dir++)
    {
        exit = pRoom->exit[dir];
        if ( exit && exit->u1.to_room )
        {
            if ( exit->exit_info != 0 )
            {
                ++reset_count;
                add_reset( area, 'D', pRoom->vnum,dir, exit->exit_info, -1 );
                sprintf(log_buf,"door reset: %d %d %ld",pRoom->vnum,dir,exit->exit_info);
                log_string(log_buf);
            }
        }
    }
    return reset_count;
}

/***
  for (dir = 0; dir <= 5; dir++) {
    exit = pRoom->exit[dir];
    if (exit && exit->u1.to_room) {
      if (IS_SET(exit->exit_info,EX_LOCKED) || (IS_SET(exit->exit_info,EX_CLOSED))) {
        if ( exit->exit_info != 0 ) {
        reset_count++;
        if (IS_SET(exit->exit_info,EX_LOCKED))
          add_reset (area,'D',pRoom->vnum,dir,2,-1);
        else
          add_reset (area,'D',pRoom->vnum,dir,1,-1);
      }
    }
  }
  return reset_count;
}
 ** OLD DOOR CODE ***/

int add_door_resets ( AREA_DATA *area )
{
  ROOM_INDEX_DATA *pRoom;
  sh_int vnum,count = 0;

  for (vnum = area->min_vnum_room;
       vnum <= area->max_vnum_room;  vnum++) {
    pRoom = get_room_index (vnum);
    if (pRoom) {
      count += add_door_resets_room (pRoom,area);
    }
  }

  return count;
}

void edit_reset_room ( CHAR_DATA *ch )
{
  ROOM_INDEX_DATA *pRoom;
  RESET_DATA *reset,*prev_reset,*next_reset;
  AREA_DATA *area;
  bool remove,mark;
  int count;
  char buf[MAX_STRING_LENGTH];

  pRoom = ch->pcdata->edit.room;
  area = pRoom->area;
  reset = area->reset_first;
  prev_reset = NULL;
  mark = FALSE;
  while (reset) {
    next_reset = reset->next;
    remove = FALSE;
    switch (reset->command) {
      case 'M':
        if (reset->arg3 == pRoom->vnum)
          mark = remove = TRUE;
         else
          mark = FALSE;
        break;
      case 'G':
      case 'E':
        remove = mark;
        break;
      case 'D':
        if (reset->arg1 == pRoom->vnum)
          remove = TRUE;
        mark = FALSE;
        break;
      case 'O':
        if (reset->arg3 == pRoom->vnum)
          mark = remove = TRUE;
        else
          mark = FALSE;
        break;
      case 'P':
        remove = mark;
        break;
      default:
        mark = FALSE;
    }
    if (remove) {
      if (reset == area->reset_last) {
        area->reset_last = prev_reset;

      }
      if (reset == area->reset_first) {
        area->reset_first = reset->next;
        free_reset (reset);
        top_reset--;
      } else {
        prev_reset->next = reset->next;
        free_reset (reset);
        top_reset--;
      }
    } else {
      prev_reset = reset;
    }
    reset = next_reset;
  }

  count = add_mob_resets_room (pRoom,area);
  sprintf (buf,"Added %d mob resets.\n\r",count);
  send_to_char (buf,ch);

  count = add_object_resets_room (pRoom,area);
  sprintf (buf,"Added %d object resets.\n\r",count);
  send_to_char (buf,ch);

  count = add_door_resets_room (pRoom,area);
  sprintf (buf,"Added %d door resets.\n\r",count);
  send_to_char (buf,ch);

}

void edit_reset_main ( CHAR_DATA *ch, int num )
{
  AREA_DATA *pArea;
  char buf[MAX_STRING_LENGTH];
  int count = 0;

  pArea = ch->pcdata->edit.area;
  if (!pArea) {
    send_to_char ("Select an area first.\n\r>  ",ch);
    return;
  }
  switch (num) {
    case ID_EDIT_RESET_ROOM:
      if (!check_range (ch,RANGE_AREA,-1)) return;
      edit_reset_room (ch);
      send_to_char ("\n\r>  ",ch);
      sprintf (buf,"Room resets for room [%d].",ch->pcdata->edit.room->vnum);
      olc_log_string (ch,buf);
      break;
    case ID_EDIT_RESET_MOB:
      if (!check_range (ch,RANGE_AREA,-1)) return;
      remove_resets (pArea,'M');
      remove_resets (pArea,'G');
      remove_resets (pArea,'E');
      count = add_mob_resets (pArea);
      sprintf (buf,"Added %d mob resets.\n\r>  ",count);
      send_to_char (buf,ch);
      sprintf (buf,"Mob resets for area %s.",ch->pcdata->edit.area->name);
      olc_log_string (ch,buf);
      break;
    case ID_EDIT_RESET_OBJECT:
      if (!check_range (ch,RANGE_AREA,-1)) return;
      remove_resets (pArea,'P');
      remove_resets (pArea,'O');
      count = add_object_resets (pArea);
      sprintf (buf,"Added %d object resets.\n\r>  ",count);
      send_to_char (buf,ch);
      sprintf (buf,"Object resets for area %s.",ch->pcdata->edit.area->name);
      olc_log_string (ch,buf);
      break;
    case ID_EDIT_RESET_DOOR:
      if (!check_range (ch,RANGE_AREA,-1)) return;
      remove_resets (pArea,'D');
      count = add_door_resets (pArea);
      sprintf (buf,"Added %d door resets.\n\r>  ",count);
      send_to_char (buf,ch);
      sprintf (buf,"Door resets for area %s.",ch->pcdata->edit.area->name);
      olc_log_string (ch,buf);
      break;
    case ID_EDIT_RESET_ALL:
      if (!check_range (ch,RANGE_AREA,-1)) return;
      remove_resets (pArea,'M');
      remove_resets (pArea,'G');
      remove_resets (pArea,'E');
      remove_resets (pArea,'P');
      remove_resets (pArea,'D');
      remove_resets (pArea,'O');
      count = add_mob_resets (pArea);
      sprintf (buf,"Added %d mob resets.\n\r",count);
      send_to_char (buf,ch);

      count = add_object_resets (pArea);
      sprintf (buf,"Added %d object resets.\n\r",count);
      send_to_char (buf,ch);

      count = add_door_resets (pArea);
      sprintf (buf,"Added %d door resets.\n\r>  ",count);
      send_to_char (buf,ch);

      sprintf (buf,"All resets for area %s.",ch->pcdata->edit.area->name);
      olc_log_string (ch,buf);
      break;
  }
}


void edit_goto_main ( CHAR_DATA *ch, int num )
{
  ch->pcdata->menu = &edit_menu;
  do_menu (ch,NULL);
}

void edit_exit ( CHAR_DATA *ch, int num )
{
  ch->pcdata->menu = NULL;
  ch->pcdata->interp_fun = NULL;
  REMOVE_BIT (ch->comm,COMM_IN_OLC);
}


MOB_INDEX_DATA *get_first_mob (ROOM_INDEX_DATA *room)
{
  CHAR_DATA *gch,*next_char;

  for ( gch = room->people; gch != NULL; gch = next_char ) {
    next_char = gch->next_in_room;
    if (IS_NPC(gch) && gch->pIndexData) {
      return gch->pIndexData;
    }
  }
  return NULL;
}

void do_edit ( CHAR_DATA *ch, char *argument )
{
  if (IS_NPC(ch)) {
    send_to_char ("NPC's can't edit.\n\r",ch);
    return;
  }

  if(ch->pcdata->ref_help)
  {
    send_to_char("You need to exit edithelp before using edit.\n\r", ch);
    return;
  }

  SET_BIT (ch->comm,COMM_IN_OLC);
  ch->pcdata->menu = &edit_menu;
  ch->pcdata->interp_fun = do_menu;
  if (ch->pcdata->edit.area == NULL) {
    ch->pcdata->edit.per_flags = EDIT_DEFAULT_ROOM|EDIT_DEFAULT_OBJ|EDIT_DEFAULT_MOB|
      EDIT_CREATE_MINIMAL|EDIT_DOUBLE_DOOR;  /* kris - take out after saving to char file */
  }
  ch->pcdata->edit.area = ch->in_room->area;

  if (IS_SET(ch->pcdata->edit.per_flags,EDIT_DEFAULT_ROOM) ||
     (ch->pcdata->edit.room == NULL))
    ch->pcdata->edit.room = ch->in_room;
  if (IS_SET(ch->pcdata->edit.per_flags,EDIT_DEFAULT_OBJ) ||
     (ch->pcdata->edit.obj == NULL)) {
    OBJ_DATA *obj,*prev_obj;

    obj = ch->carrying;
    prev_obj = NULL;
    while (obj && (obj->wear_loc != -1)) {
      prev_obj = obj;
      obj = obj->next_content;
    }
    if (obj) {
      ch->pcdata->edit.obj = obj->pIndexData;
    } else if (prev_obj) {
      ch->pcdata->edit.obj = prev_obj->pIndexData;
    } else
      ch->pcdata->edit.obj = NULL;
  }
  if (IS_SET(ch->pcdata->edit.per_flags,EDIT_DEFAULT_MOB) ||
     (ch->pcdata->edit.mob == NULL)) {
    MOB_INDEX_DATA *pData;

    pData = get_first_mob (ch->in_room);
    if (pData != NULL)
    ch->pcdata->edit.mob = pData;
  }
  do_menu (ch, argument);
}

