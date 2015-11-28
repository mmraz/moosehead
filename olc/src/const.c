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

/*
 * HEY DUDES there's two structs of the skill and spell table in this file
 * do  a search for SS_SCRIBE and that'll put you in that second one.  The
 * second one is the current one.  It's near the ned of the file.
 */
//CLASS TABLE IS SETUP LIKE THIS:
  /*
    M  C  T  W  M  P  B  D  A  S  E  W  C  R  B
    A  L  H  A  O  A  E  R  S  A  L  I  R  O  L
    G  E  I  R  N  L  R  U  S  M  E  Z  U  G  A
*/


static char rcsid[] = "$Id: const.c,v 1.460 2004/09/01 02:35:01 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include "merc.h"
#include "magic.h"
#include "interp.h"
/*
 #include "imc-mercbase.h"
 */

/* for deities */
const struct deity_type deity_table[] =
{
    {	"mojo",		"Mojo",		{"recall"},
	ALIGN_NONE,	FALSE	},
    {	"matook",	"Matook",	{"recall","patience"},
	ALIGN_NONE,	FALSE	},
    {	"bolardari",	"Bolardari",	{"recall","random"},
	ALIGN_NEUTRAL,	FALSE	},
    {	"megena",	"Megena",	{"recall","nurture","meld"},
	ALIGN_GOOD,	FALSE	},
    {	"huitzilopochtli", "Huitzilopochtli", {"recall","reanimation"},
	ALIGN_EVIL,	FALSE	},
    {	"xochipili",	"Xochipili",	{"recall","opiate"},
	ALIGN_NONE,	FALSE	},
    {	"paladine",	"Paladine",	{"recall","bravery","stature"},
	ALIGN_GOOD,	TRUE	},
    {   "loki",		"Loki",		{"recall","distraction","meld"},
	ALIGN_NEUTRAL,	TRUE	},
    {	"cthon",	"Cthon",	{"recall","fear","speed"},
	ALIGN_EVIL,	TRUE	},
    {	"hermes",	"Hermes",	{"recall","transport"},
	ALIGN_NONE,	TRUE	},
    {   "almighty", "The Almighty", {"recall", "banishment" },
	ALIGN_NONE,     TRUE    }
};

/* item type list */
const struct item_type    item_table  []  =
{
    { ITEM_LIGHT, "light"   },
    { ITEM_SCROLL,  "scroll"  },
    { ITEM_WAND,  "wand"    },
    {   ITEM_STAFF, "staff"   },
    {   ITEM_WEAPON,  "weapon"  },
    {   ITEM_TREASURE,  "treasure"  },
    {   ITEM_ARMOR, "armor"   },
    { ITEM_POTION,  "potion"  },
    { ITEM_CLOTHING,  "clothing"  },
    {   ITEM_FURNITURE, "furniture" },
    { ITEM_TRASH, "trash"   },
    { ITEM_CONTAINER, "container" },
    { ITEM_DRINK_CON, "drink"   },
    { ITEM_KEY, "key"   },
    { ITEM_FOOD,  "food"    },
    { ITEM_MONEY, "money"   },
    { ITEM_BOAT,  "boat"    },
    { ITEM_CORPSE_NPC,"npc_corpse"  },
    { ITEM_CORPSE_PC, "pc_corpse" },
    {   ITEM_FOUNTAIN,  "fountain"  },
    { ITEM_PILL,  "pill"    },
    { ITEM_PROTECT, "protect" },
    { ITEM_MAP, "map"   },
    { ITEM_PORTAL,  "portal"  },
    { ITEM_WARP_STONE,"warp_stone"  },
    { ITEM_ROOM_KEY,  "room_key"  },
    { ITEM_GEM, "gem"   },
    { ITEM_JEWELRY, "jewelry" },
    {   ITEM_JUKEBOX, "jukebox" },
    { ITEM_TRAP,        "trap_part" },
    { ITEM_GRENADE,	"grenade" },
    { ITEM_SPELL_PAGE,	"spell_page" },
    { ITEM_PART,        "item_part" },
    { ITEM_FORGE,       "forge"     },
    { ITEM_HERB,	"herb"	},
    { ITEM_CAPSULE,        "capsule"  },
    {   0,    NULL    }
};


/* weapon selection table */
const struct  weapon_type weapon_table  []  =
{
   { "sword", OBJ_VNUM_SCHOOL_SWORD,  WEAPON_SWORD, &gsn_sword  },
   { "mace",  OBJ_VNUM_SCHOOL_MACE, WEAPON_MACE,  &gsn_mace   },
   { "dagger",  OBJ_VNUM_SCHOOL_DAGGER, WEAPON_DAGGER,  &gsn_dagger },
   { "axe", OBJ_VNUM_SCHOOL_AXE,  WEAPON_AXE, &gsn_axe  },
   { "staff", OBJ_VNUM_SCHOOL_STAFF,  WEAPON_SPEAR, &gsn_spear  },
   { "flail", OBJ_VNUM_SCHOOL_FLAIL,  WEAPON_FLAIL, &gsn_flail  },
   { "whip",  OBJ_VNUM_SCHOOL_WHIP, WEAPON_WHIP,  &gsn_whip },
   { "polearm", OBJ_VNUM_SCHOOL_POLEARM,WEAPON_POLEARM, &gsn_polearm  },
   { "garotte", OBJ_VNUM_SCHOOL_WHIP,WEAPON_GAROTTE, &gsn_garotte  },
   { NULL,  0,        0,  NULL    }
};


/* pnet table and prototype for future flag setting */
const   struct pnet_type      pnet_table    []              =
{
   {  "on",           PNET_ON,         1 },
   {  "prefix", PNET_PREFIX, 1 },
   {  "deaths",       PNET_DEATHS,     1 },
   {  "bounty",       PNET_BOUNTY,     1 },
   {  "level",  PNET_LEVELS,  1 },   
   {  "logins",       PNET_LOGINS,     1 },
   {  "links",       PNET_LINKS,     1 },
   {  "notes", PNET_NOTES, 1 },
   {  "matook", PNET_MATOOK, 1 },
   {  NULL,   0,    0  }
};
 
/* wiznet table and prototype for future flag setting */
const   struct wiznet_type      wiznet_table    []              =
{
   {  "on",           WIZ_ON,         IM },
   {  "prefix", WIZ_PREFIX, IM },
   {  "ticks",        WIZ_TICKS,      IM },
   {  "logins",       WIZ_LOGINS,     IM },
   {  "sites",        WIZ_SITES,      L4 },
   {  "links",        WIZ_LINKS,      L7 },
   {  "newbies",  WIZ_NEWBIE, IM },
   {  "spam",   WIZ_SPAM, L5 },
   {  "deaths",       WIZ_DEATHS,     IM },
   {  "resets",       WIZ_RESETS,     L4 },
   {  "mobdeaths",    WIZ_MOBDEATHS,  L4 },
   {  "flags",  WIZ_FLAGS,  L5 },
   {  "transgress", WIZ_TRANSGRESSION, L5 },
   {  "penalties",  WIZ_PENALTIES,  L5 },
   {  "saccing",  WIZ_SACCING,  L5 },   
   {  "levels", WIZ_LEVELS, IM },
   {  "load",   WIZ_LOAD, L2 },
   {  "restore",  WIZ_RESTORE,  L2 },
   {  "snoops", WIZ_SNOOPS, L2 },
   {  "switches", WIZ_SWITCHES, L2 },
   {  "secure", WIZ_SECURE, L1 },   
   {  "notes", WIZ_NOTES, L7 },
   {  "debug", WIZ_DEBUG, L1 },
   {  "allnotes", WIZ_ALLNOTES, L2 },
   { "olc", WIZ_OLC, L2 },
   { "deityfavor", WIZ_DEITYFAVOR, L5 },
   /*
   {    "imc",          WIZ_IMC,        L4 },
   {    "imc-debug",    WIZ_IMCDEBUG,   L1 },
    */
   {  NULL,   0,    0  }
};

/* attack table  -- not very organized :( */
const   struct attack_type  attack_table  []    =
{
    {   "none",   "hit",    -1    },  /*  0 */
    { "slice",  "slice",  DAM_SLASH },  
    {   "stab",   "stab",   DAM_PIERCE  },
    { "slash",  "slash",  DAM_SLASH },
    { "whip",   "whip",   DAM_SLASH },
    {   "claw",   "claw",   DAM_SLASH },  /*  5 */
    { "blast",  "blast",  DAM_BASH  },
    {   "pound",  "pound",  DAM_BASH  },
    { "crush",  "crush",  DAM_BASH  },
    {   "grep",   "grep",   DAM_SLASH },
    { "bite",   "bite",   DAM_PIERCE  },  /* 10 */
    {   "pierce", "pierce", DAM_PIERCE  },
    {   "suction",  "suction",  DAM_BASH  },
    { "beating",  "beating",  DAM_BASH  },
    {   "digestion",  "digestion",  DAM_ACID  },
    { "charge", "charge", DAM_BASH  },  /* 15 */
    {   "slap",   "slap",   DAM_BASH  },
    { "punch",  "punch",  DAM_BASH  },
    { "wrath",  "wrath",  DAM_ENERGY  },
    { "magic",  "magic",  DAM_ENERGY  },
    {   "divine", "divine power", DAM_HOLY  },  /* 20 */
    { "cleave", "cleave", DAM_SLASH },
    { "scratch",  "scratch",  DAM_PIERCE  },
    {   "peck",   "peck",   DAM_PIERCE  },
    {   "peckb",  "peck",   DAM_BASH  },
    {   "chop",   "chop",   DAM_SLASH },  /* 25 */
    {   "sting",  "sting",  DAM_PIERCE  },
    {   "smash",   "smash", DAM_BASH  },
    {   "shbite", "shocking bite",DAM_LIGHTNING },
    { "flbite", "flaming bite", DAM_FIRE  },
    { "frbite", "freezing bite", DAM_COLD },  /* 30 */
    { "acbite", "acidic bite",  DAM_ACID  },
    { "chomp",  "chomp",  DAM_PIERCE  },
    {   "drain",  "life drain", DAM_NEGATIVE  },
    {   "thrust", "thrust", DAM_PIERCE  },
    {   "slime",  "slime",  DAM_ACID  }, /* 35 */
    { "shock",  "shock",  DAM_LIGHTNING }, 
    {   "thwack", "thwack", DAM_BASH  },
    {   "flame",  "flame",  DAM_FIRE  },
    {   "chill",  "chill",  DAM_COLD  },
    {   "splash", "splash", DAM_DROWNING}, /* 40 */
    {   "mblast", "mental blast", DAM_MENTAL},
    {   "light", "light burst", DAM_LIGHT},
    {   "essence", "essence drain", DAM_NEUTRAL},
    {   "shatter", "shatter", DAM_VULN}, // This one should ALWAYS be last - used for blind gladiators
    {   NULL,   NULL,   0   }
};

/* race table */
const   struct  race_type race_table  []    =
{
/*
    {
  name,   pc_race?,
  act bits, aff_by bits,  off bits,
  imm,    res,    vuln,
  form,   parts 
    },
*/
    { "unique",   FALSE, 0, 0, 0, 0, 0, 0, 0, 0 },

    {   "dragon", TRUE, 0, AFF_INFRARED|AFF_FLYING, 0,
  0,    RES_FIRE|RES_BASH|RES_CHARM, VULN_PIERCE|VULN_COLD,
   A|H|Z,   A|C|D|E|F|G|H|I|J|K|P|Q|U|V|X
    },

    { 
  "human",    TRUE, 
  0,    0,    0,
  0,    0,    0,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "elf",      TRUE,
  0,    AFF_INFRARED, 0,
  0,    RES_MENTAL,  VULN_IRON,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "dwarf",    TRUE,
  0,    AFF_INFRARED, 0,
  0,    RES_POISON|RES_DISEASE, VULN_DROWNING,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "giant",    TRUE,
  0,    0,    0,
  0,    RES_FIRE|RES_COLD,  VULN_LIGHTNING,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "ogre",    TRUE,
  0,    0,    0,
  0,    RES_FIRE|RES_COLD,  VULN_MENTAL,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
   "lasher", TRUE,
   0,   AFF_FLYING,     0,
   0,   RES_FIRE|RES_LIGHTNING, VULN_HOLY,
   A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
   "kender",    TRUE,
   0, 0, 0,
   0, RES_BASH, VULN_COLD|VULN_POISON,
   A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "goblin",   TRUE,
  0,    AFF_INFRARED, 0,
  IMM_POISON,    RES_DISEASE,  VULN_LIGHTNING,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "half-elf",  TRUE,
  0,  AFF_INFRARED, 0,
  0,  RES_CHARM, VULN_IRON,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "yinn",  TRUE, 
  0, AFF_INFRARED, 0,
  0, RES_COLD|RES_LIGHTNING, VULN_FIRE,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "volare",  TRUE,
  0, AFF_FLYING, 0,
  0, RES_HOLY|RES_CHARM, VULN_NEGATIVE,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K
    },
    {
  "rockbiter", TRUE,
  0, 0, 0,
  0, RES_WEAPON|RES_LIGHTNING, VULN_MENTAL,
  E|J|M|ee, A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "faerie",	TRUE, 
  0, AFF_FLYING|AFF_INFRARED|AFF_DETECT_INVIS|AFF_DETECT_HIDDEN, 0,
  0, RES_MAGIC, VULN_BASH,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "gargoyle",  TRUE,
  0, AFF_FLYING|AFF_INFRARED|AFF_DETECT_INVIS, 0,
  0, 0, 0,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K|X
    },

      {
  "mutant",	FALSE, 
  0, 0, 0,
  0, 0, 0,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "smurf",	FALSE, 
  0, 0, 0,
  0, 0, 0,
  A|H|M|V, A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "bat",      FALSE,
  0,    AFF_FLYING|AFF_INFRARED, OFF_DODGE|OFF_FAST,
  0,    0,    VULN_LIGHT,
  A|G|V,    A|C|D|E|F|H|J|K|P
    },

    {
  "bear",     FALSE,
  0,    0,    OFF_CRUSH|OFF_DISARM|OFF_BERSERK,
  0,    RES_BASH|RES_COLD,  0,
  A|G|V,    A|B|C|D|E|F|H|J|K|U|V
    },

    {
  "cat",      FALSE,
  0,    AFF_INFRARED,  OFF_FAST|OFF_DODGE,
  0,    0,    0,
  A|G|V,    A|C|D|E|F|H|J|K|Q|U|V
    },

    {
  "centipede",    FALSE,
  0,    AFF_INFRARED,  0,
  0,    RES_PIERCE|RES_COLD,  VULN_BASH,
  A|B|G|O,    A|C|K 
    },

    {
  "dog",      FALSE,
  0,    0,    OFF_FAST,
  0,    0,    0,
  A|G|V,    A|C|D|E|F|H|J|K|U|V
    },

    {
  "doll",     FALSE,
  0,    0,    0,
  IMM_COLD|IMM_POISON|IMM_HOLY|IMM_NEGATIVE|IMM_MENTAL|IMM_DISEASE
  |IMM_DROWNING,  RES_BASH|RES_LIGHT,
  VULN_SLASH|VULN_FIRE|VULN_ACID|VULN_LIGHTNING|VULN_ENERGY,
  E|J|M|cc, A|B|C|G|H|K
    },

    {
  "fido",     FALSE,
  0,    0,    OFF_DODGE|ASSIST_RACE,
  0,    0,      VULN_MAGIC,
  A|B|G|V,  A|C|D|E|F|H|J|K|Q|V
    },    
   
    {
  "fox",      FALSE,
  0,    AFF_INFRARED,  OFF_FAST|OFF_DODGE,
  0,    0,    0,
  A|G|V,    A|C|D|E|F|H|J|K|Q|V
    },

    {
  "hobgoblin",    FALSE,
  0,    AFF_INFRARED, 0,
  0,    RES_DISEASE|RES_POISON, 0,
  A|H|M|V,        A|B|C|D|E|F|G|H|I|J|K|Y
    },

    {
  "kobold",   FALSE,
  0,    AFF_INFRARED, 0,
  0,    RES_POISON, VULN_MAGIC,
  A|B|H|M|V,  A|B|C|D|E|F|G|H|I|J|K|Q
    },

    {
  "lizard",   FALSE,
  0,    0,    0,
  0,    RES_POISON, VULN_COLD,
  A|G|X|cc, A|C|D|E|F|H|K|Q|V
    },

    {
  "modron",   FALSE,
  0,    AFF_INFRARED,   ASSIST_RACE|ASSIST_ALIGN,
  IMM_CHARM|IMM_DISEASE|IMM_MENTAL|IMM_HOLY|IMM_NEGATIVE,
      RES_FIRE|RES_COLD|RES_ACID, 0,
  H,    A|B|C|G|H|J|K
    },

    {
  "orc",      FALSE,
  0,    AFF_INFRARED, 0,
  0,    RES_DISEASE,  VULN_LIGHT,
  A|H|M|V,  A|B|C|D|E|F|G|H|I|J|K
    },

    {
  "pig",      FALSE,
  0,    0,    0,
  0,    0,    0,
  A|G|V,    A|C|D|E|F|H|J|K
    },  

    {
  "rabbit",   FALSE,
  0,    0,    OFF_DODGE|OFF_FAST,
  0,    0,    0,
  A|G|V,    A|C|D|E|F|H|J|K
    },
    
    {
  "school monster", FALSE,
  ACT_NOALIGN,    0,    0,
  IMM_CHARM|IMM_SUMMON, 0,    VULN_MAGIC,
  A|M|V,    A|B|C|D|E|F|H|J|K|Q|U
    },  

    {
  "snake",    FALSE,
  0,    0,    0,
  0,    RES_POISON, VULN_COLD,
  A|G|X|Y|cc, A|D|E|F|K|L|Q|V|X
    },
 
    {
  "song bird",    FALSE,
  0,    AFF_FLYING,   OFF_FAST|OFF_DODGE,
  0,    0,    0,
  A|G|W,    A|C|D|E|F|H|K|P
    },

    {
  "troll",    FALSE,
  0,    AFF_REGENERATION|AFF_INFRARED|AFF_DETECT_HIDDEN,
  OFF_BERSERK,
  0,  RES_CHARM|RES_BASH, VULN_FIRE|VULN_ACID,
  A|B|H|M|V,    A|B|C|D|E|F|G|H|I|J|K|U|V
    },

    {
  "water fowl",   FALSE,
  0,    AFF_SWIM|AFF_FLYING,  0,
  0,    RES_DROWNING,   0,
  A|G|W,    A|C|D|E|F|H|K|P
    },    
  
    {
  "wolf",     FALSE,
  0,    AFF_INFRARED,  OFF_FAST|OFF_DODGE,
  0,    0,    0,  
  A|G|V,    A|C|D|E|F|J|K|Q|V
    },

    {
  "wyvern",   FALSE,
  0,    AFF_FLYING|AFF_DETECT_INVIS|AFF_DETECT_HIDDEN,
  OFF_BASH|OFF_FAST|OFF_DODGE,
  IMM_POISON, 0,  VULN_LIGHT,
  A|B|G|Z,    A|C|D|E|F|H|J|K|Q|V|X
    },

    {
  "unique",   FALSE,
  0,    0,    0,
  0,    0,    0,    
  0,    0
    },


    {
  NULL, 0, 0, 0, 0, 0, 0
    }
};

// struct pc_race_type
// {
//     char *      name;                   /* MUST be in race_type */
//     char        who_name[7];
//     sh_int      points;                 /* cost in points of the race */
//     sh_int      starting_hmv[3];	/* starting Hp Mana moVe */
//     sh_int      class_mult[MAX_CLASS];  /* exp multiplier for class, * 100 */
//     char *      skills[5];              /* bonus skills for the race */
//     sh_int      stats[MAX_STATS];       /* starting stats */
//     sh_int      max_stats[MAX_STATS];   /* maximum stats */
//     sh_int      size;                   /* aff bits for the race */
// };

const struct  pc_race_type  pc_race_table []  =
{
    { "null race", "", 0, {100, 100, 100}, 
    { 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 100 },
    { "" }, { 13, 13, 13, 13, 13, 13, 13, 13 }, 
    { 18, 18, 18, 18, 18, 18, 18, 18 }, 0 },
 
    {
  "dragon", "Dragon", 31, {1000, 600, 400}, 
  {110,110,140,140,125,125,140,110,125,125,110,110,110,140,140},
  {"enhanced damage", "hand to hand", "draconian" }, 
  {14,11,11,14,14,11,14,11}, {25,25,25,25,25,25,25,18}, SIZE_HUGE
    },

    {
  "human",  "Human",  5, {750, 750, 500}, 
  { 100,100,100,100,100,100,100,100,100,100,100,100,100,100,100 },
  { "" },
  { 10, 10, 10, 10, 10, 10, 10, 10 }, { 18, 18, 18, 18, 18, 18, 18, 25 }, SIZE_MEDIUM
    },

    {   
  "elf",    " Elf ",  14, {700, 800, 500},  
  { 100,125,100,120,112,130,112,100,112,110,100,100,125,100,120 }, 
  { "sneak", "hide" },
  {  9, 11, 10, 13,  8, 11, 12, 10 }, { 17, 20, 18, 21, 16, 21, 23, 24 }, SIZE_SMALL
    },

    {
  "dwarf",  "Dwarf",  9, {900, 500, 600}, 
  { 150,100,125,100,112,100,112,125,137,125,175,150,100,125,100 },
  { "berserk", "sharpen" },
  { 11,  9, 11,  8, 13, 9, 13, 18 }, { 20, 16, 19, 16, 21, 17, 24, 20 }, SIZE_MEDIUM
    },

    {
  "giant",  "Giant",  5, {1050, 400, 550}, 
  { 200,150,150,105,150,127,127,175,175,152,200,200,150,150,105 },
  { "bash", "fast healing", "grab" },
  { 14,  8,  9,  8, 14, 9, 13, 10 }, { 23, 15, 18, 16, 22, 17, 22, 20 }, SIZE_HUGE
    },

    {
  "ogre",  "Ogre ",  11, {1050, 400, 550}, 
  { 200,150,150,105,150,127,127,175,175,152,200,200,150,150,105 },
  { "bash", "fast healing" },
  { 13,  9, 10,  9, 13, 11, 14, 10 }, { 21, 16, 18, 17, 21, 17, 24, 17 }, SIZE_LARGE
    },

    {
  "lasher", "Lasher", 14, {900, 500, 600}, 
  {115,115,115,115,115,115,115,115,115,115,115,115,115,115,115 },
  { "fast healing", "meditation" },
  { 13,  10,  10,  10, 14, 10, 11, 12 }, { 22, 20, 20, 17, 22, 21, 23, 22 }, SIZE_LARGE
    },

    {
  "kender",  "Kender",  6, {650, 700, 700}, 
  { 110,110,100,130,105,120,115,110,105,120,110,110,110,100,130 },
  { "dodge" },
  { 10, 10,  9,  15, 9, 15, 11, 15 }, { 18, 17, 17, 23, 17, 25, 19, 25 }, SIZE_SMALL
    },

    {
  "goblin", "Goblin", 9, {750, 750, 500}, 
  { 100,100,120,150,110,125,135,100,110,125,100,100,100,120,150 },
  { "" }, 
  { 10, 10, 10, 11,  9, 11, 11, 8 }, { 18, 18, 18, 19, 17, 19, 19, 16 }, SIZE_MEDIUM
    },
    {
  "half-elf", "H-Elf", 8, {750, 750, 550}, 
  { 105,115,105,115,110,115,110,110,105,110,110,105,115,105,115 },
  { "sneak", "fast healing" },
  { 10, 10, 10, 10, 10, 11, 11, 12 }, { 17, 19, 18, 19, 17, 20, 22, 24 }, SIZE_MEDIUM
    },
    {
  "yinn", "Yinn ",  23, {850, 550, 600}, 
  { 110,110,140,140,125,125,140,110,125,125,110,110,110,140,140 },
  { "second attack", "daetok",  "hand to hand" },
  { 14,  9, 10, 14, 14, 14, 14, 12 }, { 22, 18, 19, 22, 22, 22, 22, 21 }, SIZE_LARGE
    },

    {
  "volare", "Volare", 10, {700, 800, 500}, 
  { 100,100,150,125,125,125,150,100,125,125,100,100,100,150,125 },
  { "creation", "curative", "combat" },
  { 10, 10, 11, 10, 10, 10, 11, 11 }, { 17, 19, 20, 18, 18, 18, 19, 22 }, SIZE_MEDIUM
    },
   
   {
  "rockbiter", "RckBtr", 16, {1250, 250, 500},
  {160,160,160,160,160,160,160,160,160,160,160,160,160,160,160},
  {  "" }, 
  { 17, 8,10,7,17,8,15,11 }, { 25,16,18,14,25,16,25,19 }, SIZE_GIANT
    },

    {
  "faerie", "Faerie", 13, {400, 1200, 400},
  {120,120,240,240,180,180,240,120,180,180,120,120,120,240,240 },
  { "dodge" },
  {  5,17,17,17,5,17,8,11 }, { 13,25,25,25,13,25,16,22 }, SIZE_TINY
    },

    {
  "gargoyle", "Garg ", 6, {750, 750, 500},
  {135,135,135,135,135,135,135,135,135,135,135,135,135,135,135 },
  { "stone skin" },
  { 11,11,11,11,11,11,11,10 }, { 19,19,19,19,19,19,19,18 }, SIZE_MEDIUM
    },

    {
  "smurf", "Smurf", 6, {650, 650, 650},
  {120,120,240,240,180,180,240,120,180,180,120,120,120,240,240 },
  { "" },
  {  5,20,20,20,5,20,8,20 }, { 8,25,25,25,8,25,14,25 }, SIZE_TINY
    },

    {
  "mutant", "Mutant", 30, {750,750,500},
  {100,100,100,100,100,100,100,100,100,100,100,100,100,100,100 },
  { "" },
  { 10,10,10,10,10,10,10,10 }, { 18,18,18,18,18,18,18,18 }, SIZE_MEDIUM
    }

};

 
/* Kit table */
/* More detailed Explination
{    Name    Cost (*10 will give you number of pracs)
   { "null", 0,
      S  I  W  D  C
      T  R  I  E  O
      R  T  S  X  N
   { -1,-1,-1,-1,-1 },

     M  C  T  W  M  P  B  D  A  S  E  W  C  R  B
     A  L  H  A  O  A  E  R  S  A  L  I  R  O  L
     G  E  I  R  N  L  R  U  S  M  E  Z  U  G  A
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },

     D H E D G O L K G H Y V R F M G
     R U L W I G A E O E I O O A U A
     A M F A A R S N B L N L C E T R
     G A   R N E H D L F N A K R A G
   { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1 },


   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0 },

       SEX
     M  F  O 
   { 0, 0, 0 }, 
Avail        skill name
   0,      { "" }
   },

*/
const struct	kit_type	kit_table	[] =
{
   { "null", 0,	
   /* S  I  W  D  C  A  E  S */
   /* T  N  I  E  O  G  N  O */
   /* R  T  S  X  N  T  D  C */
   { -1,-1,-1,-1,-1,-1,-1,-1 },
   /*M  C  T  W  1  2  3  4  5  6  E  W  C  R  B */
   { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
   { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 0, 0, 0 }, 0,	{ "" }
   },
   
   { "cavalier", 6,
   { 16,-1,15,-1,16,-1,16,17},
   { 0, 0, 0, 1, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
   { 1,1,1}, 
     1,   {"call mount"}
   },

   { "acrobat", 7,
   { 14, -1, -1, 18, 13, 18, 14, -1 },
   { 0,0,1,0,1,0,0,0,1,0,0,0,0,1,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "tumbling" }
   },

   { "alchemist", 6,
   { -1,17,13,15,-1,15,-1,-1 },
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 }, 
   1, 	{ "alchemy" }
   },

   { "amazon", 7,
   { 17, -1, -1, 16, 16, 16, 16, -1 },
   { 0,1,0,1,1,0,1,0,0,0,0,0,1,0,1 },
   { 0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0 },
   { 0,0,1 },
   0, { NULL }
   },
   
   { "nature friend", 10,
   { -1, 18, 18, -1, -1, -1, -1, 18 },
   {  0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   1, { "charm animal","swarm" },
   },

   { "archer", 7,
   { 13,-1,-1,19,-1,18,-1,-1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   0,	{ "archery" }
   },

   { "barbarian", 8,
   { 18, -1, -1, -1, 17, -1, 18, -1 },
   { 0,0,0,1,0,0,1,0,0,0,0,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "barbarian rage", "endurance" }
   },

   { "battlerager", 8,
   { 18,-1,-1,-1,18, -1,19,-1 },
   { 0,1,0,1,0,1,0,0,0,0,0,0,1,0,0 },
   { 0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0 },
   { 1, 1, 1 },
   1, { "axe", "killing rage" }
   },

   { "bishop", 8,
   { -1, 14, 18, -1, -1, -1, -1, 17 },
   { 0,1,0,0,0,1,0,0,0,0,0,0,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { "holy chant" }
   },

   { "bladesinger", 8,
   { 15, 16, -1, 18, -1, 18, -1, -1 },
   { 1,0,0,1,0,0,0,0,0,1,0,1,0,0,1 },
   { 0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0 },
   { 1, 1, 1 } ,
   1, { "sword", "dagger", "bladesong" }
   },

   { "brawler", 3,
   { -1, -1, -1, -1, -1, -1, 17, -1 },
   { 0,0,1,1,0,0,1,0,0,1,0,0,0,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1,1,1 },
   1, { NULL }
   },

   { "buffy", 10,
   { 16, -1,16, -1, 16, -1, 16, -1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   0, { NULL }
   },
   { "archeologist", 10,
   { 16, -1,16, -1, 16, -1, 16, -1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1,1,1 },
   1, { NULL }
   },
   { "lycanthrope hunter", 10,
   { 16, -1,16, -1, 16, -1, 16, -1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1,1,1 },
   1, { NULL }
   },
   { "vampyre hunter", 10,
   { 16, -1,16, -1, 16, -1, 16, -1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1,1,1 },
   1, { NULL }
   },

   { "enchanter", 7,
   { -1, 20, -1, -1, -1, -1, -1, -1 },
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { NULL }
   },

   { "medium", 4,
   { -1,-1,-1,-1,-1,-1,-1,-1 },
   { 1,1,0,0,1,0,0,1,0,0,1,1,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   1, { NULL }
   },

   { "shaman", 10,
   { -1, 18, 18, -1, -1, -1, -1, 18 },
   { 1,1,0,0,0,0,0,0,0,0,0,1,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { "disease" }
   },

   { "herbalist", 7,
   { -1, 15, 16, 15, -1, -1, 16, -1 },
   { 0,1,0,0,0,0,0,1,0,0,1,0,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 }, 
   0, { "herbalism" }
   },
 
   { "fence", 6,
   { -1,14,14,15,-1,-1,-1,19 },
{ 0,0,1,0,0,0,0,0,1,0,0,0,0,1,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1 },
   { 1, 1, 1}, 1, { "connive" },
   },


   { "shogun", 9,
   { 16, -1, -1, 16, 16, 16, 17, -1 },
   { 0,0,0,1,0,0,0,0,0,1,0,0,0,0,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   0, { "parry", "hai-ruki" }
   },
   
   { "knight", 9,
   { 18, -1, -1, -1, 16, 16, 16, 18 },
   { 0,1,0,1,0,1,0,0,0,0,0,0,0,0,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1  },
   1, { "kcharge","call mount"  }
   },

   { "myrmidon", 8,
   { 18, -1, -1, -1 , 18, -1, 18, -1 },
   { 0,0,0,1,0,0,0,0,0,0,0,0,0,0,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1 },
   { 1, 1, 1 },
   1, { "fourth attack" }
   },

   { "necromancer", 8,
   { -1, 18, 15, -1, -1, -1, -1, -1},
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { "wraithform", "vampiric touch" }
   },

   { "nethermancer", 15,
   { -1, 19, 18, -1, -1, -1, -1, 18 },
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { "nethermancy", "fade" }
   },

   { "ninja", 8,
   { 17, -1, -1, 20, 16, 18, 16, -1 },
   { 0,0,1,0,0,0,0,0,1,0,0,0,0,1,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1, { "ninjitsu", "kurijitsu" }
   },

   { "prophet", 8,
   { -1, 15,19, -1, -1, -1, -1, -1 },
   { 0,1,0,0,1,1,0,0,0,0,0,0,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "imbue", }
   },

   { "ranger", 7,
   { 16, -1, 14, 16, 14, -1, 17, -1 },
   /*M C T W 1 2 3 4 5 6 E W C R B */
   { 0,0,1,1,0,0,0,0,0,0,0,0,0,1,1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1, 1, 1 },
   1,	{ "dual wield" }
   },

   { "scribe", 6,
   { -1,17,14,15,-1,-1,-1,16 },
   /*M C T W 1 2 3 4 5 6 E W C R B */
   { 1,1,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,  { "scribe" }
   },

   { "shadowstalker", 4,
   { 12, 14, -1, 20, -1, 18, -1, -1 },
   { 0,0,1,0,0,0,0,0,0,0,0,0,0,1,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "stalk" }
   },

   { "shapeshifter" , 8,
   { -1, 13, 16, -1, 16, -1, 16, -1 },
   { 0,0,0,0,0,0,0,1,0,0,1,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1 },
   { 1, 1, 1 },
   0, { "shapemorph","shapeshift" }
   },

   { "spirit caller", 8,
   { -1, 18, 18, -1, -1, -1, -1, -1},
   { 0,0,0,0,0,0,0,1,0,0,0,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   1, { "spiritcaller" }
   },

   { "specialist", 10,
   { -1, 18, 18, -1, -1, -1, -1, -1},
   { 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1,1,1 },
   0, { "element specialization" }
   },

   { "staff crafter", 6,
   { -1, -1, 18, 18, -1, -1, -1, -1 },
   { 0,1,0,0,0,0,0,1,0,0,0,0,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,   { "endow" }
   },

   { "summoner", 8,
   { -1, 18, 18, -1, -1, -1, -1, 17},
   { 0,0,0,0,0,0,0,0,0,0,1,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,  {"summon elemental" }
   },

   { "vindicator", 8,
   { 17, -1, 15, 15, 17, 16, 16, -1 },
   { 0,1,0,0,0,1,0,0,0,0,0,0,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "third attack", "wrath" }
   },

   { "warlock", 8,
   { -1, 18, -1, -1, -1, -1, -1, -1 },
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 0, 1, 0 },
   1, { "immolate", "lacerate", "arcantic alacrity", "arcantic lethargy", "clarity" }
   },

   { "witch", 8,
   { -1, 18, -1, -1, -1, -1, -1, -1 },
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 0,0,1 },
   1, { "immolate", "lacerate", "arcantic alacrity", "arcantic lethargy", "clarity" }
   },

   { "wu jen", 7,
   { 17, 17, -1, 17, -1, 17, -1, -1 },
   { 1,0,0,0,1,0,0,0,0,1,0,0,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "focus", "enhance" }
   },

   { "wyrmslayer", 6,
   { 16, 15, -1, -1, 16, 16, 16, -1 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,	{ "dagger", "spear" }
   },

   { "seer", 7,
   { -1, 19, 18, -1, -1, -1, -1, 18 },
   { 1,1,0,0,1,1,0,1,1,1,1,1,1,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   0,	{ "farsee" }
   },

   { "wand maker", 6,
   { -1,17,14,15,-1,-1,-1,16 },
   /*M C T W 1 2 3 4 5 6 E W C R B */
   { 1,0,0,0,0,0,0,0,0,0,0,1,0,0,0 },
   { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
   { 1, 1, 1 },
   1,  { "infuse" }
   },

   { NULL, 0,
   { -1,-1,-1,-1,-1,-1,-1,-1 },
   { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
   { 0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
 { 1, 1, 1 },
 0,	{ "" }
   }

};




/*
 * Class table.
 */
const struct  class_type  class_table [MAX_CLASS] =
{
    {
  "mage", "Mag",  STAT_INT, STAT_WIS,  OBJ_VNUM_SCHOOL_DAGGER,
  { 3018, 9618 },  75,  20, 6, 87,  6,  8, 2, FALSE,
  { 0, 0 },
  "mage basics", "mage default"
    },

    {
  "cleric", "Cle",  STAT_WIS, STAT_STR,  OBJ_VNUM_SCHOOL_MACE,
  { 3003, 9619 },  75,  20, 2, 100,  7, 10, 2, FALSE,
  { 0, 0 },
  "cleric basics", "cleric default"
    },

    {
  "thief", "Thi",  STAT_DEX, STAT_INT,  OBJ_VNUM_SCHOOL_DAGGER,
  { 3028, 9639 },  75,  20,  -4, 112, 8, 13, 0, FALSE,
  { 0, 0 },  
  "thief basics", "thief default"
    },

    {
  "warrior", "War",  STAT_STR, STAT_CON,  OBJ_VNUM_SCHOOL_SWORD,
  { 3022, 9633 },  75,  20,  -10, 120,  11, 15, 0, FALSE,
  { 0, 0 }, 
  "warrior basics", "warrior default"
    },

    {
  "monk", "Mon",  STAT_DEX, STAT_WIS,  OBJ_VNUM_SCHOOL_MACE,
  { 3028, 9619 },  75,  20,  -4, 105,  8, 13, 1, TRUE,
  { CLASS_THIEF, CLASS_CLERIC },
  "monk basics", "thiefmonk default"
    },

    {
  "paladin", "Pal",  STAT_STR, STAT_WIS, OBJ_VNUM_SCHOOL_SWORD,
  { 3022, 9619 },  75,  20,  -7, 110, 10, 15, 1, TRUE,
   { CLASS_WARRIOR, CLASS_CLERIC },
  "paladin basics", "clericpaladin default"
    },

    {
  "berzerker", "Ber",  STAT_STR, STAT_DEX,  OBJ_VNUM_SCHOOL_SWORD,
  { 3022, 9639 },  75,  20,  -7, 115,  10, 15, 0, TRUE,
  { CLASS_WARRIOR, CLASS_THIEF },
  "berzerker basics", "thiefberzerker default"
    },

    {
  "druid", "Dru",  STAT_WIS, STAT_INT,  OBJ_VNUM_SCHOOL_MACE,
  { 3003, 9618 },  75,  20,  4, 90,  7, 10, 2, TRUE,
  { CLASS_MAGE, CLASS_CLERIC },
  "druid basics", "magedruid default"
    },

    {
  "assassin", "Ass",  STAT_DEX, STAT_INT,  OBJ_VNUM_SCHOOL_DAGGER,
  { 3018, 9639 },  75,  20,  1, 95,  8, 13, 1, TRUE,
  { CLASS_MAGE, CLASS_THIEF }, 
  "assassin basics", "thiefassassin default"
    },

    {
  "samurai", "Sam",  STAT_INT, STAT_STR,  OBJ_VNUM_SCHOOL_SWORD,
  { 3018, 9639 },  75,  20,  2, 105,  9, 15, 1, TRUE,
  { CLASS_MAGE, CLASS_WARRIOR },
  "samurai basics", "warriorsamurai default"
    },

    {
  "elementalist", "Ele", STAT_INT, STAT_WIS, OBJ_VNUM_SCHOOL_DAGGER,
  { 3768, 0 }, 75, 20, 4, 100, 6, 10, 2, FALSE,
  { 0, 0 }, 
  "elementalist basics", "elementalist default"
    },

   {
  "wizard", "Wiz",   STAT_INT, STAT_WIS,  OBJ_VNUM_SCHOOL_DAGGER,
  { 3018, 9618 },  75,  20, 6, 85,  6,  8, 2, TRUE, 
  { CLASS_MAGE, -1 },
  "mage basics", "wizard default"
  },
     
  {
 "crusader", "Cru", STAT_WIS, STAT_STR, OBJ_VNUM_SCHOOL_MACE,
 { 3003, 9619 },  75,  20, 2, 105,  8, 11, 2, TRUE, 
 { CLASS_CLERIC, -1 },
 "cleric basics", "crusader default"
  },

  {
 "rogue", "Rog", STAT_DEX,  STAT_INT, OBJ_VNUM_SCHOOL_DAGGER,
 { 3028, 9639 }, 75, 20, -4, 105, 8, 13, 0, TRUE, 
 { CLASS_THIEF, -1 },
 "thief basics", "rogue default" 
 },

 {
 "blademaster", "Bla", STAT_STR, STAT_CON, OBJ_VNUM_SCHOOL_SWORD,
 { 3022, 9633 }, 75, 20, -10, 115, 11, 15, 0, TRUE, 
 { CLASS_WARRIOR, -1 },
 "warrior basics", "blademaster default"
 }

};



/*
 * Titles.
 *
char *  const     title_table [MAX_CLASS][MAX_LEVEL+1][2] =
{
    {
  { "Man",      "Woman"       },

  { "Apprentice of Magic",  "Apprentice of Magic"   },
  { "Spell Student",    "Spell Student"     },
  { "Scholar of Magic",   "Scholar of Magic"    },
  { "Delver in Spells",   "Delveress in Spells"   },
  { "Medium of Magic",    "Medium of Magic"   },

  { "Scribe of Magic",    "Scribess of Magic"   },
  { "Seer",     "Seeress"     },
  { "Sage",     "Sage"        },
  { "Illusionist",    "Illusionist"     },
  { "Abjurer",      "Abjuress"      },

  { "Invoker",      "Invoker"     },
  { "Enchanter",      "Enchantress"     },
  { "Conjurer",     "Conjuress"     },
  { "Magician",     "Witch"       },
  { "Creator",      "Creator"     },

  { "Savant",     "Savant"      },
  { "Magus",      "Craftess"      },
  { "Wizard",     "Wizard"      },
  { "Warlock",      "War Witch"     },
  { "Sorcerer",     "Sorceress"     },

  { "Elder Sorcerer",   "Elder Sorceress"   },
  { "Grand Sorcerer",   "Grand Sorceress"   },
  { "Great Sorcerer",   "Great Sorceress"   },
  { "Golem Maker",    "Golem Maker"     },
  { "Greater Golem Maker",  "Greater Golem Maker"   },

  { "Maker of Stones",    "Maker of Stones",    },
  { "Maker of Potions",   "Maker of Potions",   },
  { "Maker of Scrolls",   "Maker of Scrolls",   },
  { "Maker of Wands",   "Maker of Wands",   },
  { "Maker of Staves",    "Maker of Staves",    },

  { "Demon Summoner",   "Demon Summoner"    },
  { "Greater Demon Summoner", "Greater Demon Summoner"  },
  { "Dragon Charmer",   "Dragon Charmer"    },
  { "Greater Dragon Charmer", "Greater Dragon Charmer"  },
  { "Master of all Magic",  "Master of all Magic"   },

  { "Master Mage",    "Master Mage"     },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },

	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },

	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },
	{ "Master Mage",                "Master Mage"                   },

  { "Mage Hero",      "Mage Heroine"      },
  { "Avatar of Magic",    "Avatar of Magic"   },
  { "Angel of Magic",   "Angel of Magic"    },
  { "Demigod of Magic",   "Demigoddess of Magic"    },
  { "Immortal of Magic",    "Immortal of Magic"   },
  { "God of Magic",   "Goddess of Magic"    },
  { "Deity of Magic",   "Deity of Magic"    },
  { "Supremity of Magic",   "Supremity of Magic"    },
  { "Creator",      "Creator"     },
  { "Implementor",    "Implementress"     }
    },

    {
  { "Man",      "Woman"       },

  { "Believer",     "Believer"      },
  { "Attendant",      "Attendant"     },
  { "Acolyte",      "Acolyte"     },
  { "Novice",     "Novice"      },
  { "Missionary",     "Missionary"      },

  { "Adept",      "Adept"       },
  { "Deacon",     "Deaconess"     },
  { "Vicar",      "Vicaress"      },
  { "Priest",     "Priestess"     },
  { "Minister",     "Lady Minister"     },

  { "Canon",      "Canon"       },
  { "Levite",     "Levitess"      },
  { "Curate",     "Curess"      },
  { "Monk",     "Nun"       },
  { "Healer",     "Healess"     },

  { "Chaplain",     "Chaplain"      },
  { "Expositor",      "Expositress"     },
  { "Bishop",     "Bishop"      },
  { "Arch Bishop",    "Arch Lady of the Church" },
  { "Patriarch",      "Matriarch"     },

  { "Elder Patriarch",    "Elder Matriarch"   },
  { "Grand Patriarch",    "Grand Matriarch"   },
  { "Great Patriarch",    "Great Matriarch"   },
  { "Demon Killer",   "Demon Killer"      },
  { "Greater Demon Killer", "Greater Demon Killer"    },

  { "Cardinal of the Sea",  "Cardinal of the Sea"   },
  { "Cardinal of the Earth",  "Cardinal of the Earth"   },
  { "Cardinal of the Air",  "Cardinal of the Air"   },
  { "Cardinal of the Ether",  "Cardinal of the Ether"   },
  { "Cardinal of the Heavens",  "Cardinal of the Heavens" },

  { "Avatar of an Immortal",  "Avatar of an Immortal"   },
  { "Avatar of a Deity",    "Avatar of a Deity"   },
  { "Avatar of a Supremity",  "Avatar of a Supremity"   },
  { "Avatar of an Implementor", "Avatar of an Implementor"  },
  { "Master of all Divinity", "Mistress of all Divinity"  },

  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },  
  { "Master Cleric",    "Master Cleric"     },

  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },

  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },
  { "Master Cleric",    "Master Cleric"     },

  { "Holy Hero",      "Holy Heroine"      },
  { "Holy Avatar",    "Holy Avatar"     },
  { "Angel",      "Angel"       },
  { "Demigod",      "Demigoddess",      },
  { "Immortal",     "Immortal"      },
  { "God",      "Goddess"     },
  { "Deity",      "Deity"       },
  { "Supreme Master",   "Supreme Mistress"    },
	{ "Creator",                    "Creator"                       },
  { "Implementor",    "Implementress"     }
    },

    {
  { "Man",      "Woman"       },

  { "Pilferer",     "Pilferess"     },
  { "Footpad",      "Footpad"     },
  { "Filcher",      "Filcheress"      },
  { "Pick-Pocket",    "Pick-Pocket"     },
  { "Sneak",      "Sneak"       },

  { "Pincher",      "Pincheress"      },
  { "Cut-Purse",      "Cut-Purse"     },
  { "Snatcher",     "Snatcheress"     },
  { "Sharper",      "Sharpress"     },
  { "Rogue",      "Rogue"       },

  { "Robber",     "Robber"      },
  { "Magsman",      "Magswoman"     },
  { "Highwayman",     "Highwaywoman"      },
  { "Burglar",      "Burglaress"      },
  { "Thief",      "Thief"       },

  { "Knifer",     "Knifer"      },
  { "Quick-Blade",    "Quick-Blade"     },
  { "Killer",     "Murderess"     },
  { "Brigand",      "Brigand"     },
  { "Cut-Throat",     "Cut-Throat"      },

  { "Spy",      "Spy"       },
  { "Grand Spy",      "Grand Spy"     },
  { "Master Spy",     "Master Spy"      },
  { "Assassin",     "Assassin"      },
  { "Greater Assassin",   "Greater Assassin"    },

  { "Master of Vision",   "Mistress of Vision"    },
  { "Master of Hearing",    "Mistress of Hearing"   },
  { "Master of Smell",    "Mistress of Smell"   },
  { "Master of Taste",    "Mistress of Taste"   },
  { "Master of Touch",    "Mistress of Touch"   },

  { "Crime Lord",     "Crime Mistress"    },
  { "Infamous Crime Lord",  "Infamous Crime Mistress" },
  { "Greater Crime Lord",   "Greater Crime Mistress"  },
  { "Master Crime Lord",    "Master Crime Mistress"   },
  { "Godfather",      "Godmother"     },

	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },

	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },

	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },
	{ "Master Thief",               "Master Thief"                  },

  { "Assassin Hero",    "Assassin Heroine"    },
  { "Avatar of Death",    "Avatar of Death",    },
  { "Angel of Death",   "Angel of Death"    },
  { "Demigod of Assassins", "Demigoddess of Assassins"  },
  { "Immortal Assasin",   "Immortal Assassin"   },
  { "God of Assassins",   "God of Assassins",   },
  { "Deity of Assassins",   "Deity of Assassins"    },
  { "Supreme Master",   "Supreme Mistress"    },
	{ "Creator",                    "Creator"                       },
  { "Implementor",    "Implementress"     }
    },

    {
  { "Man",      "Woman"       },

  { "Swordpupil",     "Swordpupil"      },
  { "Recruit",      "Recruit"     },
  { "Sentry",     "Sentress"      },
  { "Fighter",      "Fighter"     },
  { "Soldier",      "Soldier"     },

  { "Warrior",      "Warrior"     },
  { "Veteran",      "Veteran"     },
  { "Swordsman",      "Swordswoman"     },
  { "Fencer",     "Fenceress"     },
  { "Combatant",      "Combatess"     },

  { "Hero",     "Heroine"     },
  { "Myrmidon",     "Myrmidon"      },
  { "Swashbuckler",   "Swashbuckleress"   },
  { "Mercenary",      "Mercenaress"     },
  { "Swordmaster",    "Swordmistress"     },

  { "Lieutenant",     "Lieutenant"      },
  { "Champion",     "Lady Champion"     },
  { "Dragoon",      "Lady Dragoon"      },
  { "Cavalier",     "Lady Cavalier"     },
  { "Knight",     "Lady Knight"     },

  { "Grand Knight",   "Grand Knight"      },
  { "Master Knight",    "Master Knight"     },
  { "Paladin",      "Paladin"     },
  { "Grand Paladin",    "Grand Paladin"     },
  { "Demon Slayer",   "Demon Slayer"      },

  { "Greater Demon Slayer", "Greater Demon Slayer"    },
  { "Dragon Slayer",    "Dragon Slayer"     },
  { "Greater Dragon Slayer",  "Greater Dragon Slayer"   },
  { "Underlord",      "Underlord"     },
  { "Overlord",     "Overlord"      },

  { "Baron of Thunder",   "Baroness of Thunder"   },
  { "Baron of Storms",    "Baroness of Storms"    },
  { "Baron of Tornadoes",   "Baroness of Tornadoes"   },
  { "Baron of Hurricanes",  "Baroness of Hurricanes"  },
  { "Baron of Meteors",   "Baroness of Meteors"   },

  { "Master Warrior",   "Master Warrior"    },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },

	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },

	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },
	{ "Master Warrior",             "Master Warrior"                },

  { "Knight Hero",    "Knight Heroine"    },
  { "Avatar of War",    "Avatar of War"     },
  { "Angel of War",   "Angel of War"      },
  { "Demigod of War",   "Demigoddess of War"    },
  { "Immortal Warlord",   "Immortal Warlord"    },
  { "God of War",     "God of War"      },
  { "Deity of War",   "Deity of War"      },
  { "Supreme Master of War",  "Supreme Mistress of War" },
	{ "Creator",                    "Creator"                       },
  { "Implementor",    "Implementress"     }
    }
};
 */

const struct	improve_type	improve_table	[26] 	=
{
    {	1	},
    {   1	}, /*  1 */
    {   1	},
    {	1	},
    {	1	},
    {	1	}, /*  5 */
    {	1	},
    {	1	},
    {	1	},
    {	1	},
    { 	1	}, /* 10 */
    {	1	},
    {	1	},
    {	1	},
    {	1	},
    {	2	}, /* 15 */
    {	2	},
    {	2	},
    {	2	}, /* 18 */
    {	3	},
    {	3	}, /* 20 */
    {	3	},
    {	4	},
    {	4	},
    {	5	},
    {	6	} /* 25 */
};


/*
 * Attribute bonus tables.
 */
const struct  str_app_type  str_app   [26]    =
{
    { -5, -4,   0,  0 },  /* 0  */
    { -5, -4,   3,  1 },  /* 1  */
    { -3, -2,   3,  2 },
    { -3, -1,  10,  3 },  /* 3  */
    { -2, -1,  25,  4 },
    { -2, -1,  55,  5 },  /* 5  */
    { -1,  0,  80,  6 },
    { -1,  0,  90,  7 },
    {  0,  0, 100,  8 },
    {  0,  0, 100,  9 },
    {  0,  0, 115, 10 }, /* 10  */
    {  0,  0, 115, 11 },
    {  0,  0, 130, 12 },
    {  0,  0, 130, 13 }, /* 13  */
    {  0,  1, 140, 14 },
    {  1,  1, 150, 15 }, /* 15  */
    {  1,  2, 165, 16 },
    {  2,  3, 180, 22 },
    {  2,  3, 200, 25 }, /* 18  */
    {  3,  4, 225, 30 },
    {  3,  5, 250, 35 }, /* 20  */
    {  4,  6, 300, 40 },
    {  4,  6, 350, 45 },
    {  5,  7, 400, 50 },
    {  5,  8, 450, 55 },
    {  6,  9, 500, 60 }  /* 25   */
};



const struct  int_app_type  int_app   [26]    =
{
    {  3 }, /*  0 */
    {  5 }, /*  1 */
    {  7 },
    {  8 }, /*  3 */
    {  9 },
    { 10 }, /*  5 */
    { 11 },
    { 12 },
    { 13 },
    { 15 },
    { 17 }, /* 10 */
    { 19 },
    { 22 },
    { 25 },
    { 28 },
    { 31 }, /* 15 */
    { 34 },
    { 37 },
    { 40 }, /* 18 */
    { 44 },
    { 49 }, /* 20 */
    { 55 },
    { 60 },
    { 70 },
    { 80 },
    { 85 }  /* 25 */
};



const struct  wis_app_type  wis_app   [26]    =
{
    { 0 },  /*  0 */
    { 0 },  /*  1 */
    { 0 },
    { 0 },  /*  3 */
    { 0 },
    { 1 },  /*  5 */
    { 1 },
    { 1 },
    { 1 },
    { 1 },
    { 1 },  /* 10 */
    { 1 },
    { 1 },
    { 1 },
    { 1 },
    { 2 },  /* 15 */
    { 2 },
    { 2 },
    { 3 },  /* 18 */
    { 3 },
    { 3 },  /* 20 */
    { 3 },
    { 4 },
    { 4 },
    { 4 },
    { 5 } /* 25 */
};



const struct  dex_app_type  dex_app   [26]    =
{
    {   60 },   /* 0 */
    {   50 },   /* 1 */
    {   50 },
    {   40 },
    {   30 },
    {   20 },   /* 5 */
    {   10 },
    {    0 },
    {    0 },
    {    0 },
    {    0 },   /* 10 */
    {    0 },
    {    0 },
    {    0 },
    {    0 },
    { - 10 },   /* 15 */
    { - 20 },
    { - 30 },
    { - 40 },
    { - 50 },
    { - 60 },   /* 20 */
    { - 70 },
    { - 85 },
    { - 90 },
    { -100 },
    { -110 }    /* 25 */
};


const struct  con_app_type  con_app   [26]    =
{
    { -4, 20 },   /*  0 */
    { -3, 25 },   /*  1 */
    { -2, 30 },
    { -2, 35 },   /*  3 */
    { -1, 40 },
    { -1, 45 },   /*  5 */
    { -1, 50 },
    {  0, 55 },
    {  0, 60 },
    {  0, 65 },
    {  0, 70 },   /* 10 */
    {  0, 75 },
    {  0, 80 },
    {  0, 85 },
    {  0, 88 },
    {  1, 90 },   /* 15 */
    {  2, 95 },
    {  2, 97 },
    {  3, 99 },   /* 18 */
    {  3, 99 },
    {  4, 99 },   /* 20 */
    {  4, 99 },
    {  5, 99 },
    {  6, 99 },
    {  7, 99 },
    {  8, 99 }    /* 25 */
};


/*
 * MHS material table by Ben
 */
const struct	material_type	material_table	[] =
{
    { "unknown",	75,	MAT_NONCORPOREAL			},
    { NULL,		0,	0					}
};


/*
 * Liquid properties.
 * Used in world.obj.
 * Remember to increase MAX_LIQUIDS if you ever add any
 */
const struct  liq_type  liq_table []  =
{
/*    name      color proof, full, thirst, food, ssize */
    { "water",      "clear",  {   0, 1, 10, 0, 16 } },
    { "beer",     "amber",  {  12, 1,  8, 1, 12 } },
    { "red wine",   "burgundy", {  30, 1,  8, 1,  5 } },
    { "ale",      "brown",  {  15, 1,  8, 1, 12 } },
    { "dark ale",   "dark",   {  16, 1,  8, 1, 12 } },

    { "whisky",     "golden", { 120, 1,  5, 0,  2 } },
    { "lemonade",   "pink",   {   0, 1,  9, 2, 12 } },
    { "firebreather",   "boiling",  { 190, 0,  4, 0,  2 } },
    { "local specialty",  "clear",  { 151, 1,  3, 0,  2 } },
    { "slime mold juice", "green",  {   0, 2, -8, 1,  2 } },

    { "milk",     "white",  {   0, 2,  9, 3, 12 } },
    { "tea",      "tan",    {   0, 1,  8, 0,  6 } },
    { "coffee",     "black",  {   0, 1,  8, 0,  6 } },
    { "blood",      "red",    {   0, 2, -1, 2,  6 } },
    { "salt water",   "clear",  {   0, 1, -2, 0,  1 } },

    { "pepsi",     "brown",  {   0, 2,  9, 2, 12 } }, 
    { "root beer",    "brown",  {   0, 2,  9, 2, 12 }   },
    { "elvish wine",    "green",  {  35, 2,  8, 1,  5 }   },
    { "white wine",   "golden", {  28, 1,  8, 1,  5 }   },
    { "champagne",    "golden", {  32, 1,  8, 1,  5 }   },

    { "mead",     "honey-colored",{  34, 2,  8, 2, 12 }   },
    { "rose wine",    "pink",   {  26, 1,  8, 1,  5 } },
    { "benedictine wine", "burgundy", {  40, 1,  8, 1,  5 }   },
    { "vodka",      "clear",  { 130, 1,  5, 0,  2 }   },
    { "cranberry juice",  "red",    {   0, 1,  9, 2, 12 } },

    { "orange juice",   "orange", {   0, 2,  9, 3, 12 }   }, 
    { "absinthe",   "green",  { 200, 1,  4, 0,  2 } },
    { "brandy",     "golden", {  80, 1,  5, 0,  4 } },
    { "aquavit",    "clear",  { 140, 1,  5, 0,  2 } },
    { "schnapps",   "clear",  {  90, 1,  5, 0,  2 }   },

    { "icewine",    "purple", {  50, 2,  6, 1,  5 } },
    { "amontillado",    "burgundy", {  35, 2,  8, 1,  5 } },
    { "sherry",     "red",    {  38, 2,  7, 1,  5 }   },  
    { "framboise",    "red",    {  50, 1,  7, 1,  5 }   },
    { "rum",      "amber",  { 151, 1,  4, 0,  2 } },

    { "cordial",    "clear",  { 100, 1,  5, 0,  2 }   },
    { "pus",     "beige",  {   0, 1,  10, 1,  6 } },
    { NULL,     NULL,   {   0, 0,  0, 0,  0 } }
};



/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.
 */
#define SLOT(n) n

/*Put the Game def here first */
#ifdef NEVER_VERSION
/*END of  Old skill table  put endifdef here*/
#endif

const   struct  group_type      group_table     [MAX_GROUP]     =
{

    {
  "rom basics", { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 },
  { "scrolls", "staves", "wands", "recall", "swimming", "scan" }
    },

    {
  "mage basics", { 0,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  { "dagger", }
    },

    {
  "cleric basics",  { -1, 0, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "mace" }
    },
   
    {
  "thief basics",   { -1, -1, 0, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "dagger", "steal" }
    },

    {
  "warrior basics", { -1, -1, -1, 0, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "sword", "second attack" }
    },

    {
  "elementalist basics", {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,0,-1,-1,-1,-1 },
  { "mace", "dagger" }
    },

    {
  "monk basics", { -1, -1, -1, -1, 0, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "dagger", "mace", "steal" }
    },

    {
  "paladin basics", { -1, -1, -1, -1, -1, 0, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "sword", "mace", "second attack" }
    },

    {
  "berzerker basics", { -1, -1, -1, -1, -1, -1, 0, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "dagger", "steal", "sword", "second attack" }
    },

    {
  "druid basics", { -1, -1, -1, -1, -1, -1, -1, 0, -1, -1,-1,-1,-1,-1,-1 },
  { "dagger", "mace" }
    },

    {
  "assassin basics", { -1, -1, -1, -1, -1, -1, -1, -1, 0, -1,-1,-1,-1,-1,-1 },
  { "dagger", "steal" }
    },

    {
  "samurai basics", { -1, -1, -1, -1, -1, -1, -1, -1, -1, 0,-1,-1,-1,-1,-1 },
  { "sword", "second attack", "dagger" }
    },

    {
  "mage default",   { 35, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather" }
    },

    {
  "cleric default", { -1, 35, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions",  
    "detection", "healing", "maladictions", "protective", "shield block", 
    "transportation", "weather" },
    },
 
    {
  "thief default",  { -1, -1, 35, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock", "sneak" }
    },

    {
  "warrior default",  { -1, -1, -1, 35, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
    "parry", "rescue", "third attack" }
    },

/*
    {
  "monk default", { -1, -1, -1, -1, 60,  -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions",
    "detection", "healing", "maladictions", "protective", "shield block",
    "transportation", "weather", "trip", "hide", "peek", "pick lock",
    "sneak", "sword", "backstab", "disarm", "dodge", "second attack" }
    },
    */

    {
  "clericmonk default", { -1,-1,-1,-1,50,-1,-1,-1, -1, -1,-1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions", 
    "detection", "healing", "maladictions", "protective", "shield block", 
    "transportation", "weather", "kailindo" },
    },

    {
  "thiefmonk default",  { -1,-1,-1,-1,50,-1, -1, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock", "sneak", "kailindo" }
    },

/*
    {
  "paladin default", { -1, -1, -1, -1, -1, 60, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions",
    "detection", "healing", "maladictions", "protective", "shield block",
    "transportation", "weather", "bash", "disarm", "enhanced damage",
    "weaponsmaster", "parry", "rescue", "third attack" }
    },
    */

    {
  "clericpaladin default", { -1,-1,-1,-1,-1,50,-1,-1,-1, -1,-1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions", 
    "detection", "healing", "maladictions", "protective", "shield block", 
    "transportation", "weather", "piety" },
    },

    {
  "warriorpaladin default",  { -1,-1,-1,-1,-1,50,-1,-1,-1, -1,-1,-1,-1,-1,-1 },
  { "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
    "parry", "rescue", "third attack", "piety" }
    },

/*
    {
  "berzerker default", { -1, -1, -1, -1, -1, -1, 60, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "backstab", "disarm", "dodge", "trip", "hide", "peek", "pick lock", "sneak",
    "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage",
    "parry", "rescue", "third attack" }
    },
    */

    {
  "warriorberzerker default",  { -1,-1,-1,-1,-1,-1, 50, -1, -1, -1,-1,-1,-1,-1,-1 },
  { "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
    "parry", "rescue", "third attack", "insanity", "weave resistance" }
    },
    
    {
  "thiefberzerker default",  { -1,-1,-1,-1,-1,-1,50,-1,-1,-1,-1,-1,-1,-1,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock","sneak","insanity","weave resistance" }
    },

/*
    {
  "druid default", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather",
    "flail", "attack", "creation", "curative",  "benedictions", "healing",
    "shield block" }
    },
    */

    {
  "magedruid default",  { -1,-1,-1,-1, -1, -1, -1, 50, -1, -1,-1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather",
    "orbs", "harmful"}
    },

    {
  "clericdruid default", { -1,-1,-1,-1,-1, -1, -1, 50, -1, -1,-1,-1,-1,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions", 
    "detection", "healing", "maladictions", "protective", "shield block", 
    "transportation", "weather", "orbs" },
    },

/*
    {
  "assassin default", { -1, -1, -1, -1, -1, -1, -1, -1, 60, -1, -1,-1,-1,-1,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock", "sneak", "meditation", "beguiling",
    "combat", "detection", "enhancement", "illusion", "maladictions",
    "protective", "transportation", "weather" }
    },
    */

    {
  "mageassassin default", { -1,-1,-1,-1,-1,-1,-1,-1,50,-1,-1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather",
    "vorpal", "garotte", "trap"}
    },

    {
  "thiefassassin default",  { -1,-1,-1,-1,-1,-1,-1,-1,50,-1,-1,-1,-1,-1,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock", "sneak", "garotte", "vorpal","trap" }
    },

/*
    {
  "samurai default", { -1, -1, -1, -1, -1, -1, -1, -1, -1, 60, -1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather",
    "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage",
    "parry", "rescue", "third attack" }
    },
    */

    {
  "magesamurai default",   { -1,-1,-1,-1,-1,-1,-1,-1,-1,50,-1,-1,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather", 
    "throw", "chee"}
    },

    {
  "warriorsamurai default",  { -1,-1,-1,-1,-1,-1,-1,-1,-1,50,-1,-1,-1,-1,-1 },
  { "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
    "parry", "rescue", "third attack", "throw", "chee" }
    },

    {
  "elementalist default", {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,35,-1,-1,-1,-1 },
  { "element air", "element fire", "element spirit", "element water" }
    },

    {
  "wizard default",   { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,50,-1,-1,-1 },
  { "meditation", "beguiling", "combat", "detection", "enhancement",
    "illusion", "maladictions", "protective", "transportation", "weather",
    "necromancy"}
    },

    {
  "crusader default", { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,50,-1,-1 },
  { "flail", "attack", "creation", "curative",  "benedictions", 
    "detection", "healing", "maladictions", "protective", "shield block", 
    "transportation", "weather" },
    },
 
    {
  "rogue default",  { -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,50,-1 },
  { "mace", "sword", "backstab", "disarm", "dodge", "second attack",
    "trip", "hide", "peek", "pick lock", "sneak", "slice", "cutpurse" , 
    "infiltrate" },
    },

    {
  "blademaster default",  { -1, -1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,50 },
  { "weaponsmaster", "shield block", "bash", "disarm", "enhanced damage", 
    "parry", "rescue", "third attack", "dual wield" }
    },

    {
  "weaponsmaster",  { 40, 35, 30, 20, 35, 30, 30, 35, 35, 30, 40,40,35,30,25 },
  { "axe", "dagger", "flail", "mace", "polearm", "spear", "sword","whip" }
    },

    {
  "element air", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,12,-1,-1,-1,-1 },
  { "asphyxiate", "dust storm", "fly", "mistform", "thunderclap",
    "traveler fog", "electricsword", "electric shield", "wall of wind" }
    },

    {
  "attack", { -1, 5, -1, 8, 5, 7, 8, 5, -1, 8, -1,-1,7,-1,8 },
  { "hold person",  "demonfire", "dispel evil", "dispel good", "earthquake", 
    "flamestrike", "heat metal", "ray of truth" }
    },

    {
  "beguiling", { 4, -1, 6, -1, 6, -1, 6, 4, 5, 4, -1,4,-1,6,-1 },
  { "calm", "charm person", "sleep" }
    },

    {
  "benedictions",   { -1, 4, -1, 8, 4, 6, 8, 4, -1, 8, -1,-1,6,-1,8 },
  { "aid", "feast", "bless", "calm", "frenzy", "holy word", "remove curse",
    "bestow holiness" }
    },

    {
  "combat", { 6, -1, 10, 9, 10, 9, 9, 6, 8, 8, -1,6,-1,10,9 },
  { "acid blast", "burning hands", "chain lightning", "chill touch",
    "colour spray", "fireball", "lightning bolt", "magic missile",
    "shocking grasp"  }
    },

    {
  "creation", { 4, 4, 8, 8, 6, 6, 8, 4, 6, 6, -1, 4, 4, 8, 8 },
  { "continual light", "create food", "create spring", "create water",
    "create rose", "floating disc", "warp", "create fountain" }
    },

    {
  "curative", { -1, 4, -1, 8, 4, 6, 8, 4, -1, 6, -1, -1, 4, -1, 8 },
  { "cure blindness", "cure disease", "cure poison" }
    }, 

    {
  "detection", { 4, 3, 6, -1, 5, 3, 6, 3, 5, 4, -1, 4, 3, 6, -1 },
  { "detect evil", "detect good", "detect hidden", "detect invis", 
    "detect magic", "detect poison", "farsight", "identify", 
    "know alignment", "locate object" } 
    },

    {
  "draconian", { 8, -1, -1, -1, -1, -1, -1, 8, 8, 8, -1, 8, -1, -1, -1 },
  { "acid breath", "fire breath", "frost breath", "gas breath",
    "lightning breath"  }
    },

    {
  "element earth", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,13,-1,-1,-1,-1 },
  { "acid blast", "body of stone", "earthbind", "earthquake", "rust armor",
    "rust weapon", "stonefist", "stone skin" }
    },

    {
  "enchantment", { 6, -1, -1, -1, -1, -1, -1, 6, 6, 6, -1,6,-1,-1,-1 },
  { "enchant armor", "enchant weapon", "fireproof", "recharge",
    "warp" }
    },

    {
  "element energy", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,12,-1,-1,-1,-1 },
  { "create node", "call lightning", "chain lightning", "irradiate",
    "lightning bolt", "control weather", "shocking grasp", "visit node" }
    },

    { 
  "enhancement", { 5, -1, 9, 9, 9, 9, 9, 5, 7, 7, -1, 5,-1,9,9 },
  { "giant strength", "haste", "infravision", "refresh" }
    },

    {
  "element fire", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,9,-1,-1,-1,-1 },
  { "burning hands", "continual light", "faerie fire", "fireball",
    "fireproof", "flamesword",  "heat metal", "infravision",
    "flame shield" }
    },

    {
  "harmful", { -1, 3, -1, 6, 3, 5, 6, 3, -1, 6, -1,-1,3,-1,6 },
  { "cause critical", "cause light", "cause serious", "harm" }
    },

    {   
  "healing", { -1, 3, -1, 6, 3, 5, -1, 3, -1, 6, -1,-1,3,-1,6 },
  { "cure critical", "cure light", "cure serious", "heal", 
    "mass healing", "refresh" }
    },

    {
  "illusion", { 4, -1, 7, -1, 7, -1, 7, 4, 6, 4, -1,4,-1,7,-1 },
  { "invis", "blur", "mass invis", "ventriloquate" }
    },
  
    {
  "maladictions", { 5, 5, 9, 9, 7, 7, 9, 5, 7, 7, -1,5,5,9,9 },
  { "blindness", "change sex", "curse", "energy drain", "plague", 
    "poison", "slow", "weaken", "famine", "fumble", "forget",
    "feeblemind" }
    },
    
    {
  "necromancy", { 8, 6, -1, -1, 6, 6, -1, 7, 8, 8, -1,8,6,-1,-1 },
  { "animate dead","summon dead","cryogenesis","draw life","turn undead",
    "withstand death", "enervation", "wound transfer", "make bag" }
    },    

    {
  "orbs", {-1, -1, -1, -1, -1, -1, -1, 4, -1, -1, -1,-1,-1,-1,-1 },
  { "orb of awakening", "orb of surprise", "orb of touch", "orb of turning"}
    },

    {
  "piety", {-1, -1, -1, -1, -1, 4, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "crusade", "holy silence", "see soul", "bless", "curse", "indulgence" }
    },

    { 
  "protective",   { 4, 4, 7, 8, 6, 6, 7, 4, 6, 6, -1,4,8,7,8 },
  { "armor", "cancellation", "dispel magic", "fireproof",
    "protection evil", "protection good", "sanctuary", "shield", 
    "stone skin", "protection neutral", "endure cold", "endure heat",
    "sacred guardian" }
    },

    {
  "psionics", { -1, 4, -1, -1, 4, 4, -1, 4, -1, -1, -1,-1,4,-1,-1 },
  { "lay on hands","psionic blast","tower of iron will","body weaponry",
    "mirror image" }
    },    

    {
  "pyrotechnics", { -1,-1,-1,-1,2,-1,-1,-1,2,3,-1,-1,-1,-1,-1 },
  { "small bomb","high explosive", "smoke screen" }
    },    

    {
  "element spirit", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,10,-1,-1,-1,-1 },
  { "bless", "cancellation", "curse", "feast", "famine", "pass door",
    "sanctuary","word of recall" }
    },

    {
  "transportation", { 4, 4, 8, 9, 6, 7, 9, 4, 6, 7, -1,4,4,8,9 },
  { "fly", "gate", "nexus", "pass door", "portal", "summon", "teleport", 
    "knock", "word of recall" }
    },
    {
  "element water", { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,12,-1,-1,-1,-1 },
  { "create spring", "create water", "ice storm", "tsunami",
    "water breathing", "frostsword", "frost shield", "refresh" }
    },

    {
  "weather",    { 4, 4, 8, 8, 6, 6, 8, 4, 6, 6, -1,4,4,8,8 },
  { "call lightning", "control weather", "faerie fire", "faerie fog",
    "lightning bolt" }
    },

    {
  "nosferatu",  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "bleed", "entrance", "feast", "famine" }
    },

    {
  "garou",      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "bite", "morph", "feast", "famine" }
    },

    {
  "mummy",      {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,-1,-1,-1,-1 },
  { "breathe", "fear", "hex","feast", "famine" }
    },

    {
  "disease",	{-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  { "contagion", "scourge", "boiling blood", "blight" }
    },

    {
  "spiritcaller", {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
   { "spirit of boar", "spirit of bear", "spirit of cat", "spirit of owl", "spirit of wolf" }
    }
};

/*
 * The skill and spell table.
 * Slot numbers must never be changed as they appear in #OBJECTS sections.

/SSSSSSSSSSSSSEEEEEEEEEEEEEEEEECCCCCCCCCCCCCCCOOOOOOOOOOOONNNNNNNNNNNNNDDDDDDDDDDD*/
/*Put the Game def here second */
/*HEY CODERZ THIS IS THE NEW STRUCT TABLE*/
const struct  skill_type  skill_table [MAX_SKILL] =
{

/*
 * Magic spells.
 */

/* "spell name",
 * {level you get the spell},
 * { difficulty of the spell },
 * spell_function, TARGET, MIN position,
 * gsn_spell, SLOT#, min mana cost, delay in pulses,
 * damage message, wear off message, wear off object message,bitvector
 */
/* "skill name",
 * {level you get the skill},
 * {CPs for the skill (later used by gain)},
 * spell_null, TARGET, MIN postion,
 * gsn_skill, SLOT#(SLOT(0)), min mana cost, delay in pulses,
 * damage message, wear off message, wear off object message, bitvector
 */
    {
  "reserved", 
  /*
    M  C  T  W  M  P  B  D  A  S  E  W  C  R  B
    A  L  H  A  O  A  E  R  S  A  L  I  R  O  L
    G  E  I  R  N  L  R  U  S  M  E  Z  U  G  A
*/
  /* what level each class gets the spell */
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  /* creation points per class */
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
/*fun name what it does  what pos they have to be in to the targ */
  spell_null,      TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT( 0),  0,  0,
/*dam msg, wear off msg    wear off obj message, ss_bitvectors */
  "",     "",   "", 0 
    },

    {
  "convert",
  { 5 , 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_convert, TAR_IGNORE, POS_STANDING,
  &gsn_convert,    SLOT(0), 5, 6,
  "convert", "!Convert!", "", 0
    },

    {
  "opiate",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
//  {99,99,99,99,99,99,99,99,99,99,99,99,99,99,99}, 
  spell_null,    TAR_IGNORE,     POS_STANDING,
  NULL, SLOT( 0),       0,      0,
  "",   "You feel less enlightened.", "", 0
    },

    {
  "light blast",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
//  {99,99,99,99,99,99,99,99,99,99,99,99,99,99,99},
  spell_null,    TAR_IGNORE,     POS_STANDING,
  &gsn_light_blast, SLOT( 0),       0,      0,
  "",   "You regain your shadowy form.", "", 0,
  0
    },

    {
  "shaded room",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
//  {99,99,99,99,99,99,99,99,99,99,99,99,99,99,99},
  spell_null,    TAR_IGNORE,     POS_STANDING,
  &gsn_shaded_room, SLOT( 0),       0,      0,
  "",   "The darkness shrouding the room lifts.", "", 0,
  0
    },

    {
  "accuracy",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_accuracy, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,	SLOT(1025), 20, 24,
  "", "You have lost your eye for accuracy.", "",0
    },

    {
  "acid blast", 
  {28,53,35,32,43,42,33,40,31,30,22,28,53,35,32}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_acid_blast, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(70), 20, 12,
  "acid blast",   "!Acid Blast!", "",SS_WAND
    },

    {
  "acclimate",
  {53,53,53,53,53,53,53,27,53,53,53,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_acclimate, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_acclimate, SLOT(1330), 200, 60,
  "", "You are no longer attuned to your environment.", "",0
    },

    {
  "adamantite skin",
  {50,53,53,53,53,53,53,53,53,53,53,50,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_adamantite_skin, TAR_CHAR_SELF, POS_STANDING,
  &gsn_adamantite_skin, SLOT(1323), 150, 60,
  "", "Your skin becomes flesh again.", "",0
    },

    {
  "aid",
  { 53,53,53,53,53,53,53,53,53,53,53,53, 5,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_aid,	TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,	SLOT(912), 100, 24,
  "",	"The gods turn their eyes from your health.", "",0
    },
    {
  "annointment",
  { 5 , 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_annointment, TAR_CHAR_SELF, POS_STANDING,
  &gsn_annointment,    SLOT(551), 50, 12,
  "annointment", "Your holy annointment fades.", "", 0
    },

    { 
  "aura of valor",
  { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_aura_of_valor, TAR_CHAR_SELF, POS_STANDING,
  &gsn_aura_of_valor,  SLOT(558), 50, 12,
  "aura of valor", "Your aura of valor fades into nothingness.", "", 0
    },
 

    {
  "arcantic alacrity",
  { 29,53,53,53,53,53,53,53,53,53,53,29,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_arcantic_alacrity, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_arcantic_alacrity,     SLOT(841), 250, 60,
  "",     "Your magic powers return to normal.", "", 0
    },

    {
  "arcantic lethargy",
  { 31,53,53,53,53,53,53,53,53,53,53,31,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_arcantic_lethargy, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_arcantic_lethargy,     SLOT(842), 100, 36,
  "",     "Your magic powers return to normal.", "", 0
    },

    {
  "armor", 
  { 7, 2,10, 5, 6, 4, 8, 5, 9, 6,53, 7, 2,10, 5}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_armor,    TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT( 1),  5, 12,
  "",     "You feel less armored.", "", SS_SCRIBE|SS_STAFF
    },
/*I am slowly converting all teh 1's to SS_SCRIBE per Rages
  suggestion
*/
    {
  "asphyxiate",
  {53,53,53,53,53,53,53,53,53,53,30,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_asphyxiate,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING, 
  &gsn_asphyxiate,	SLOT(766),	75,	24,
  "suffocation",	"Your breathing becomes easy once more.",	"", 0
     },
    {
  "aura of cthon",
  { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_aura_cthon,     TAR_CHAR_SELF,     POS_STANDING,
  NULL,      SLOT(921),      50,     24,
  "",        "You no longer feel the aura of Cthon around you.",       "", 0
    },

    {
  "bestow holiness",
  {53,53,53,53,53,53,53,53,53,53,53,53,45,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_bestow_holiness, TAR_OBJ_INV,	POS_STANDING,
  NULL,	SLOT(916), 100, 24,
  "", "", "The holy aura fades from $p.", 0
    },

    {
  "betray",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_betray, TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(756), 100, 24,
  "", "", "", 0
    },


    {
  "blade barrier",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_blade_barrier,  TAR_CHAR_SELF, POS_STANDING,
  &gsn_blade_barrier,   SLOT(757), 100, 12,
  "spinning blades", "The spinning blades sink back into the earth.", "", 0
    },

    {
  "bless", 
  {53, 7,53, 8,28, 7,30,28,53,30, 7,53, 7,53, 8 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_bless,    TAR_OBJ_CHAR_DEF, POS_STANDING,
  NULL,     SLOT( 3),  5, 12,
  "",     "You feel less righteous.     ", ""
  "$p's holy aura fades.", SS_SCRIBE|SS_WAND|SS_STAFF
    },

    {
  "blight",
  {28,28,52,52,52,52,52,52,52,52,52,28,28,52,52},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4},
  spell_blight,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL, SLOT(918),  75, 24,
  "blight", "Your breathing returns to normal as the Blight fades.", "", 0
    },

    {
  "blindness", 
  {12, 8,17,15,13,11,16,10,15,14,53,12, 8,17,15}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_blindness,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_blindness,   SLOT( 4),  20, 12,
  "",     "You can see again.", "", SS_SCRIBE|SS_WAND
    },

    {
  "blur",
  {53,53,53,53,53,53,53,53,53,53,53,20,53,53,53},
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_blur,	TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  &gsn_blur,	SLOT(920), 25, 12,
  "", "Your body becomes clear and solid.", "", SS_SCRIBE
    },

   {
  "body of stone",
  {53,53,53,53,53,53,53,53,53,53, 2,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_body_of_stone, 	TAR_CHAR_SELF,	POS_STANDING,
  NULL,	SLOT(700), 	20,	18,
  "",	"Your body returns to its regular form.",	"", 0
    },

    {
  "body weaponry", 
  {53,19,53,53,30,30,53,30,53,53,53,53,19,53,53}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_body_weaponry,    TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT( 304),  15, 12,
  "",     "You feel less equipped for battle.", "", SS_SCRIBE|SS_STAFF
    },

    {
  "boiling blood",
  {38,38,52,52,52,52,52,52,52,52,52,38,38,52,52},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4},
  spell_boiling_blood,	TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,	SLOT(919),  75, 24,
  "blistering assault", "Your blood stops boiling.", "", 0
    },

    {
  "burning hands", 
  { 7,53,10, 9,31,30,10,29, 9, 8, 7, 7,53,10, 9}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_burning_hands,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT( 5), 15, 12,
  "burning hands",  "!Burning Hands!",  "", SS_WAND
    },

    {
  "call lightning", 
  {26,18,31,22,25,20,27,22,29,24,26,26,18,31,22}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_call_lightning, TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT( 6), 15, 12,
  "lightning bolt", "!Call Lightning!", "", SS_WAND 
    },

    {
  "call mount",
  {52,10,52,10,52,10,52,52,52,10,52,52,52,52,10},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_call_mount, TAR_IGNORE, POS_STANDING,
  NULL,     SLOT(524), 100,  12,
  "","","",0
    },
    
    {
  "calm", 
  {48,16,50,20,33,18,35,32,49,34,53,48,16,50,20}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_calm,   TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT(509),  30, 12,
  "",     "You have lost your peace of mind.",  "", SS_SCRIBE
    },

    {
  "cancellation", 
  {18,26,34,34,30,30,34,22,26,26,31,18,26,34,34}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_cancellation, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(507),  20, 12,
  "" ,     "!cancellation!", "", SS_SCRIBE|SS_STAFF
    },

    {
  "cause critical", 
  {53,13,53,19,32,16,35,32,53,35,53,53,13,53,19}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_cause_critical, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(63), 20, 12,
  "spell",    "!Cause Critical!", "", SS_SCRIBE|SS_WAND
    },

    {
  "cause light", 
  {53, 1,53, 3,26, 2,27,26,53,27,53,53, 1,53, 6}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_cause_light,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(62), 15, 12,
  "spell",    "!Cause Light!",  "", SS_SCRIBE
    },

    {
  "cause serious", 
  {53, 7,53,10,29, 9,31,29,53,31,53,53, 7,53,10}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_cause_serious,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(64), 17, 12,
  "spell",    "!Cause Serious!",  "", 0
    },

    {
  "chain lightning",
  {33,53,39,36,45,43,38,42,36,35,33,33,53,39,36 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_chain_lightning,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(500),  30, 24,
  "lightning",    "!Chain Lightning!",  "", SS_WAND
    },

    {
  "change sex", 
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_change_sex, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(82), 15, 12,
  "",     "Your body feels familiar again.",  "", SS_SCRIBE
    },

    {
  "charm person", 
  {20,53,25,53,38,53,38,36,23,36,53,20,53,25,53 }, 
  { 1, 1, 2, 2, 1 ,1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_charm_person, TAR_CHAR_OFFENSIVE, POS_STANDING,
  &gsn_charm_person,  SLOT( 7),  20, 12,
  "",     "You feel more self-confident.",  "", SS_SCRIBE
    },

/* Added charm animal 24SEP00 - Boogums */
    {
  "charm animal",
  {53,53,53,53,53,53,53,10,53,53,53,53,53,53,53},
  { 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2},
  spell_charm_animal, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL, SLOT(525), 25, 12,
  "","","", 0
    },
    {
  "chill touch", 
  { 4,53, 6, 6,29,29, 6,28, 5, 5,53, 4,53, 6, 6 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_chill_touch,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT( 8), 15, 12,
  "chilling touch", "You feel less cold.",  "", SS_WAND
    },

    {
  "clarity",
  { 25,53,53,53,53,53,53,53,53,53,53,25,53,53,53 },
  {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  spell_clarity,	TAR_CHAR_DEFENSIVE,	POS_STANDING,
  &gsn_clarity,   SLOT(812), 50, 24,
  "", "Your clarity of thought is gone.", "", 0
    },

    {
  "colour spray", 
  {16,53,22,20,37,36,21,34,19,18,53,16,53,22,20 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_colour_spray, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(10), 15, 12,
  "colour spray",   "!Colour Spray!", "", SS_WAND
    },

  /*
    M  C  T  W  M  P  B  D  A  S  E  W  C  R  B
    A  L  H  A  O  A  E  R  S  A  L  I  R  O  L
    G  E  I  R  N  L  R  U  S  M  E  Z  U  G  A
*/
  /* what level each class gets the spell */
  //{99,99,99,99,99,99,99,99,99,99,99,99,99,99,99},
  /* creation points per class */
  //{99,99,99,99,99,99,99,99,99,99,99,99,99,99,99},

    {
  "cone of silence",
    {20, 20, 53, 53, 53, 53, 53, 20, 53, 53, 53, 20, 20, 53, 53},
    { 5 , 5,  0,  0,  7,  7,  0,  7, 10,  0,  0,  5,  6,  0, 0 }, 
    spell_cone_of_silence, TAR_CHAR_OFFENSIVE, POS_STANDING,
    &gsn_cone_of_silence, SLOT(553),100,16,
    "cone of silence", "You can hear again.","",0
    },
    {
  "confusion",
  { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_confusion, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_confusion,    SLOT(550), 50, 12,
  "confusion", "You regain your ability to think clearly.", "", 0
    },

    {
   "contagion",
  {18,18,52,52,52,52,52,52,52,52,52,18,18,52,52},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4},
  spell_contagion,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL, SLOT(917),  75, 24,
  "contagion", "You regain your strength as the Contagion leaves you.", "", 0
    },

    {
  "continual light", 
  { 6, 4, 6, 9, 5, 7, 8, 5, 6, 8, 6, 6, 4, 6, 9 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_continual_light,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(57),  7, 12,
  "",     "!Continual Light!",  "", SS_SCRIBE
    },

    {
  "control weather", 
  { 15,19,28,22,24,21,25,17,22,19,15,15,19,28,22 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_control_weather,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(11), 25, 12,
  "",     "!Control Weather!",  "", SS_SCRIBE
    },

    {
  "create food", 
  {10, 5,11,12, 8, 9,12, 8,11,11,53,10, 5,11,12 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_create_food,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(12),  5, 12,
  "",     "!Create Food!",  "", SS_SCRIBE
    },

    {
  "create fountain", 
  {28,34,48,40,40,38,44,32,38,34,53,28,34,48,40},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_create_fountain,        TAR_IGNORE,     POS_STANDING,
  NULL, SLOT(488), 40, 12,
  "", "!Create Fountain!", "", SS_SCRIBE
    },

    {
  "create node",
  {53,53,53,53,53,53,53,53,53,53,28,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_create_node,    TAR_IGNORE,     POS_STANDING,
  NULL, SLOT(715),      200,    24,
  "",   "",     "", 0
    },

    {
  "create rose", 
  {16,11,10,24,11,18,17,14,13,20,53,16,11,10,24 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_create_rose,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(511),  30,   12,
  "",     "!Create Rose!",  "", SS_SCRIBE
    },  

    {
  "create spring", 
  {14,17,23,20,20,19,22,16,19,17,14,14,17,23,20 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_create_spring,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(80), 20, 12,
  "",     "!Create Spring!",  "",  SS_SCRIBE
    },

    {
  "create water", 
  { 8, 3,12,11, 8, 7,12, 6,10,10, 8, 8, 3,12,11 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_create_water, TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(13),  5, 12,
  "",     "!Create Water!", "", SS_SCRIBE
    },

    {
  "creeping doom",
  { 52, 40, 52, 52, 52, 52, 52, 52, 52, 52, 52, 52, 40, 52, 52 },
  {  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_creeping_doom, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL, SLOT(785), 35, 12,
  "creeping doom", "!Creeping Doom!",    "", 0
    },

  {
  "crusade",
  {53,53,53,53,53,25,53,53,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2},
  spell_crusade, TAR_CHAR_OFFENSIVE, POS_STANDING,
  NULL,    SLOT(420),  30, 12,
  "",  "", "", 0
  },

  {
  "crushing grip",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2},
  spell_crushing_grip, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,    SLOT(755),  30, 12,
  "crushing grip",  "", "", 0
  },
    {
  "cuffs of justice",
  { 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_cuffs_of_justice, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_cuffs_of_justice,    SLOT(670), 50, 12,
  "cuffs of justice", "You no longer feel bound by the cuffs of justice.", "", 0
    },


    {
  "cure blindness", 
  {53, 5,53,10,28, 8,31,28,53,31,53,53, 5,53,10 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_blindness, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(14),  5, 12,
  "",     "!Cure Blindness!", "", SS_SCRIBE|SS_STAFF
    },

    {
  "cure critical", 
  {53,12,53,20,32,16,53,32,53,36,53,53,12,53,20 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_critical,  TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(15), 20, 12,
  "",     "!Cure Critical!",  "", SS_STAFF
    },

    {
  "cure disease", 
  {53,12,53,16,32,14,34,32,53,34,53,53,12,53,16 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_disease, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(501),  20, 12,
  "",     "!Cure Disease!", "", SS_SCRIBE|SS_STAFF
    },

    {
  "cure light", 
  { 53, 1,53, 3,26, 2,53,26,53,27,53,53, 1,53, 3 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_light, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(16), 10, 12,
  "",     "!Cure Light!",   "", SS_SCRIBE|SS_STAFF
    },

    {
  "cure poison", 
  {53,13,53,18,32,16,35,32,53,35,53,53,13,53,18 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_poison,  TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(43),  5, 12,
  "",     "!Cure Poison!",  "", SS_SCRIBE|SS_STAFF
    },

    {
  "cure serious", 
  {53, 7,53, 9,29, 8,53,29,53,30,53,53, 7,53, 9 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_serious, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(61), 15, 12,
  "",     "!Cure Serious!", "", SS_SCRIBE|SS_STAFF
    },

  {
  "cure vision",
  { 53,53,20,53,20,53,20,53,20,53,53,53,53,20,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cure_vision, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(14),  20, 12,
  "",     "!Cure Vision!", "", 0
  },


    {
  "curse", 
  {18,18,26,22,22,20,24,18,22,20,18,18,18,26,22 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_curse,    TAR_OBJ_CHAR_OFF, POS_FIGHTING,
  &gsn_curse,   SLOT(17), 20, 12,
  "curse",    "The curse wears off.", 
  "$p is no longer impure.", SS_SCRIBE|SS_WAND
    },

    {
  "demonfire", 
  {53,34,53,45,43,40,48,43,53,48,53,53,34,53,45 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_demonfire,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(505),  20, 12,
  "torments",   "!Demonfire!",    "", SS_WAND 
    },  

    {
  "detect evil", 
  {11, 4,12,53, 8,28,32, 8,12,31,53,11, 4,12,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_evil,  TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(18),  5, 12,
  "",     "The red in your vision disappears.", "", SS_SCRIBE
    },

    {
  "detect good", 
  { 11, 4,12,53, 8,28,32, 8,12,31,53,11, 4,12,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_good,      TAR_CHAR_SELF,          POS_STANDING,
  NULL,                   SLOT(513),        5,     12,
  "",                     "The gold in your vision disappears.",  "", SS_SCRIBE
    },

    {
  "detect hidden", 
  {15,11,12,53,12,31,32,13,14,33,53,15,11,12,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_hidden,  TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(44),  5, 12,
  "",     "You feel less aware of your surroundings.",  "", SS_SCRIBE
    },

    {
  "detect invis", 
  { 3, 8, 6,53, 7,30,29, 6, 5,27,53, 3, 8, 6,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_invis, TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(19),  5, 12,
  "",     "You no longer see invisible objects.", "", SS_SCRIBE
    },

    {
  "detect magic", 
  { 2, 6, 5,53, 6,29,28, 4, 4,27,53, 2, 6, 5,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_magic, TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(20),  5, 12,
  "",     "The detect magic wears off.",  "", SS_SCRIBE
    },

    {
  "detect poison", 
  {15, 7, 9,53, 8,29,30,11,12,33,53,15, 7, 9,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_detect_poison,  TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(21),  5, 12,
  "",     "!Detect Poison!",  "", SS_SCRIBE
    },

    {
  "diamond skin",
  {45,53,53,53,53,53,53,53,53,53,53,45,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_diamond_skin, TAR_CHAR_SELF, POS_STANDING,
  &gsn_diamond_skin, SLOT(1223), 100, 48,
  "", "Your skin becomes flesh again.", "", 0
    },

    {
  "dispel evil", 
  {53,15,53,21,33,18,36,33,53,36,53,53,15,53,21 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_dispel_evil,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(22), 15, 12,
  "dispel evil",    "!Dispel Evil!",  "", SS_WAND
    },

    {
  "dispel good", 
  {53,15,53,21,33,18,36,33,53,36,53,53,15,53,21 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_dispel_good,      TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
  NULL,                   SLOT(512),      15,     12,
  "dispel good",          "!Dispel Good!",  "", SS_WAND 
    },

    {
  "dispel magic", 
  {16,24,30,30,27,27,30,20,23,23,53,16,24,30,30 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 2, 2 },
  spell_dispel_magic, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(59), 15, 12,
  "",     "!Dispel Magic!", "", SS_SCRIBE|SS_WAND
    },
    
   {
  "dispel wall", 
  {16,24,30,30,27,27,30,20,23,23,53,16,24,30,30 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 2, 2, 2 },
  spell_dispel_wall, TAR_IGNORE, POS_FIGHTING,
  NULL,     SLOT(760), 75, 12,
  "",     "!Dispel Magic!", "", SS_SCRIBE
    },
    
    {
  "dust storm",
  {53,53,53,53,53,53,53,53,53,53, 4,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_dust_storm,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_dust_storm,	SLOT(702), 40,	12,
  "dust storm",	"You clear your eyes of dust.", "", 0
    },

    {
  "earthbind",	
  {53,53,53,53,53,53,53,53,53,53,17,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_earthbind,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_earthbind,	SLOT(706),	50,	12,
  "",	"Your feet are free to move about.", "", 0
    },

    {
  "earthquake", 
  {53,10,53,14,31,12,33,31,53,33,10,53,10,53,14 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_earthquake, TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT(23), 15, 12,
  "earthquake",   "!Earthquake!",   "", SS_WAND
    },

    {
  "electricsword",
  { 53,53,53,53,53,53,53,53,53,53,27,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4  },
  spell_electricsword,	TAR_OBJ_INV,	POS_STANDING,
  NULL,	SLOT(950), 100, 24,
  "", "Your strength returns.", "$p flashes as the lightning aura fades away.", 0
    },

    {
  "electric shield",
  { 53,53,53,53,53,53,53,53,53,53,30,53,53,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_electric_shield,	TAR_CHAR_SELF,	POS_STANDING,
  NULL, SLOT(959), 100, 24,
  "", "Your electric shield dissolves and vanishes.", "", 0
    },

    {
  "enchant armor", 
  {16,53,53,53,53,53,53,34,34,34,53,16,53,53,53 }, 
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_enchant_armor,  TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(510),  100,  24,
  "",     "!Enchant Armor!",  "", SS_SCRIBE
    },

    {
  "enchant weapon", 
  {17,53,53,53,53,53,53,35,35,35,53,17,53,53,53 }, 
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_enchant_weapon, TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(24), 100,  24,
  "",     "!Enchant Weapon!", "", SS_SCRIBE
    },

    {
  "endure cold",
  {53,53,53,53,53,53,53,53,53,53,53,53,25,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_endure_cold,	TAR_CHAR_DEFENSIVE,	POS_STANDING,
  NULL,	SLOT(910), 30, 24,
  "", "Your cold resistance leaves you.", "", SS_SCRIBE|SS_STAFF
    },

    {
 "endure heat",
 {53,53,53,53,53,53,53,53,53,53,53,53,25,53,53 },
 { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
 spell_endure_heat,	TAR_CHAR_DEFENSIVE,	POS_STANDING,
 NULL,	SLOT(911), 30, 24,
 "", "Your heat resistance leaves you.", "", SS_SCRIBE|SS_STAFF
    },
  {
  "enhance",
  {5,5,5,5,5,5,5,5,5,5,5,5,5,5,5},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  spell_enhance, TAR_CHAR_SELF, POS_STANDING,
  NULL,    SLOT(424),  30, 12,
  "",  "You feel less enhanced.", "", 0
  },


    {
  "energy drain", 
  {19,22,26,23,24,23,25,21,23,21,53,19,22,26,23 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_energy_drain, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(25), 35, 12,
  "energy drain",   "!Energy Drain!", "", SS_WAND
    },

    {
  "enervation",
  {53,53,53,53,53,53,53,53,53,53,53,48,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_enervation,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_enervation,	SLOT(954), 	50,	12,
  "",	"Your magical ability returns to you.", "", 0
    },

    {
  "entrance",
  {25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_entrance, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(625), 10, 24,
  "entrance",   "!Entrance!", "", 0
    },

    {
  "faerie fire", 
  { 6, 3, 5, 8, 4, 6, 7, 5, 6, 7, 6, 6, 3, 5, 8 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_faerie_fire,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(72),  5, 12,
  "faerie fire",    "The pink aura around you fades away.", "", SS_SCRIBE|SS_WAND
    },

    {
  "faerie fog", 
  {14,21,16,24,19,23,20,18,15,19,53,14,21,16,24 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1,53, 1, 1, 2, 2 },
  spell_faerie_fog, TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT(73), 12, 12,
  "faerie fog",   "The purple cloud around you dissolves.",   "", SS_SCRIBE|SS_WAND
    },

    {
  "famine", 
  { 21,21,25,25,23,23,23,23,23,23,21,21,21,25,25 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_famine, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL, SLOT(414), 10, 12,
  "famine", "!Famine!", "", 0
     },

    {
  "farsight", 
  {14,16,16,53,16,34,34,15,15,33,53,14,16,16,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_farsight,   TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(521),  36, 20,
  "farsight",   "!Farsight!",   "", SS_SCRIBE
    },  

  {
  "farsee",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  {-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2},
  spell_farsee, TAR_IGNORE, POS_STANDING,
  NULL,    SLOT(423),  20, 12,
  "",  "", "", 1
  },

    {
  "feast", 
  { 21,21,25,25,23,23,23,23,23,23,21,21,21,25,25 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_feast,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  NULL,     SLOT(415), 10, 12,
  "feast", "!Feast!", "", 0
    },
    
    {
  "feeblemind",
  {53,53,53,53,53,53,53,53,53,53,53,24,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_feeblemind,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(951), 20, 12,
  "", "Your mind is your own once more.", "", 0 
    },

    {
  "fireball", 
  { 22,53,30,26,41,39,28,37,26,24,22,22,53,30,26 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_fireball,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(26), 15, 12,
  "fireball",   "!Fireball!",   "", SS_WAND
    },
  
    {
  "fireproof", 
  { 13,12,19,18,16,15,19,13,16,16,13,13,12,19,18 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_fireproof,  TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(523),  10, 12,
  "",     "", "$p's protective aura fades.", SS_SCRIBE
    },

    {
  "flamestrike", 
  { 53,20,53,27,36,24,39,36,53,39,53,53,20,53,27 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_flamestrike,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(65), 20, 12,
  "flamestrike",    "!Flamestrike!",    "", SS_WAND
    },

    {
  "flamesword",
  { 53,53,53,53,53,53,53,53,53,53,27,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4  },
  spell_flamesword,	TAR_OBJ_INV,	POS_STANDING,
  NULL,	SLOT(950), 100, 24,
  "", "Your strength returns.", "$p flashes as the fire aura fades away.", 0
    },

    {
  "flame shield",
  { 53,53,53,53,53,53,53,53,53,53,30,53,53,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_flame_shield,	TAR_CHAR_SELF,	POS_STANDING,
  NULL, SLOT(959), 75, 24,
  "", "Your flame shield flickers and vanishes.", "", 0
    },

    {
  "flameseek",
  { 40,40,40,40,40,40,40,40,40,40,40,40,40,40,40 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_flameseek,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL, SLOT(759), 75, 24,
  "", "You no longer feel drawn to the flame.", "", 0
    },

    {
  "floating disc", 
  { 4,10, 7,16, 9,13,12, 7, 6,10,53, 4,10, 7,16 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_floating_disc,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(522),  40, 24,
  "",     "!Floating disc!",  "", SS_SCRIBE
    },

    {
  "fly",
  { 10,18,20,22,19,20,21,14,15,16,10,10,18,20,22 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_fly,    TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(56), 10, 18,
  "",     "You slowly float to the ground.",  "", SS_SCRIBE|SS_STAFF|SS_STAFF
    },

    {
  "forget",
  {53,53,53,53,53,53,53,53,53,53,53,32,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_forget,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_forget,	SLOT(952), 35, 12,
  "",	"Your knowledge of your skills returns to you.", "", SS_SCRIBE
    },

    {
  "frenzy", 
  {53,24,53,26,38,25,39,38,53,39,53,53,24,53,26 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_frenzy,           TAR_CHAR_DEFENSIVE,     POS_STANDING,
  NULL,                   SLOT(504),      30,     24,
  "",                     "Your rage ebbs.",  "", SS_SCRIBE
    },

    {
  "frostsword",
  { 53,53,53,53,53,53,53,53,53,53,27,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4  },
  spell_frostsword,	TAR_OBJ_INV,	POS_STANDING,
  NULL,	SLOT(950), 100, 24,
  "", "Your strength returns.", "$p flashes as the ice aura fades away.", 0
    },

    {
  "frost shield",
  { 53,53,53,53,53,53,53,53,53,53,30,53,53,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_frost_shield,	TAR_CHAR_SELF,	POS_STANDING,
  NULL, SLOT(959), 100, 24,
  "", "Your frost shield melts and vanishes.", "", 0
    },

    {
  "fumble",
  {53,53,53,53,53,53,53,53,53,53,53,40,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_fumble,		TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_fumble,		SLOT(953),	40,	12,
  "", 	"You regain your agility.", "", SS_SCRIBE
     },

    {
  "gate", 
  { 27,17,32,28,25,23,30,22,30,29,53,27,17,32,28 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_gate,   TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT(83), 80, 12,
  "",     "!Gate!",   "", 0
    },

    {
  "giant strength", 
  { 11,53,22,20,37,36,21,31,17,16,53,11,53,22,20 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_giant_strength, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(39), 20, 12,
  "",     "You feel weaker.", "", SS_SCRIBE|SS_STAFF
    },
    {
  "guardian",
  { 5 , 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_guardian, TAR_IGNORE, POS_STANDING,
  &gsn_guardian,    SLOT(0), 5, 12,
  "guardian", "!Guardian!", "", 0
    },

    {
  "harm", 
  { 53,23,53,28,37,26,40,37,53,40,53,53,23,53,28 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_harm,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(27), 35, 12,
  "harm spell",   "!Harm!",    "", SS_WAND
    },
  
    {
  "haste", 
  { 21,53,26,29,39,40,28,36,24,25,53,21,53,26,29 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_haste,    TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(502),  30, 12,
  "",     "You feel yourself slow down.", "", SS_SCRIBE|SS_WAND|SS_STAFF
    },

    {
  "heal", 
  { 53,20,33,31,37,26,53,36,42,41,53,53,20,33,31 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_heal,   TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(28), 50, 12,
  "",     "!Heal!",   "", 0
    },
  
    {
  "heat metal", 
  { 53,16,53,23,34,20,37,34,53,37,16,53,16,53,23 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_heat_metal, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(516),  25, 18,
  "spell",    "Your skin cools from the burns.",   "", SS_SCRIBE|SS_WAND
    },

    {
  "hemorrhage",
  { 5 , 5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5,  5},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_hemorrhage, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_hemorrhage,    SLOT(0), 30, 24,
  "hemorrhage", "Your blood thickens to normal.", "", 0
    },

    {
  "hold person",
  {53,53,53,53,53,53,53,53,53,53,53,53,35,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_hold_person,	TAR_CHAR_OFFENSIVE,  POS_STANDING,
  NULL,	SLOT(915), 40, 12,
  "",	"Your muscles are yours once again.", "", 0
    },

  {
  "holy silence",
  {53,53,53,53,53,35,53,53,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2},
  spell_holy_silence, TAR_IGNORE, POS_STANDING,
  NULL,    SLOT(421),  20, 12,
  "",  "Your deity is with you again.", "", 0
  },

    {
  "holy word", 
  { 53,36,53,42,44,39,47,44,53,47,53,53,36,53,42 }, 
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_holy_word,  TAR_IGNORE, POS_FIGHTING,
  NULL,     SLOT(506),  200,  24,
  "divine wrath",   "!Holy Word!",    "", 0
    },
    {
  "honor guard",
  { 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 10},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_honor_guard, TAR_CHAR_SELF, POS_STANDING,
  &gsn_honor_guard,    SLOT(555), 50, 12,
  "honor guard", "Your honor guard dissapates.", "", 0
    },
  
  {
  "hydrophilia",
  { 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40, 40},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_hydrophilia, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_hydrophilia,    SLOT(778), 50, 24,
  "", "You no longer feel drawn to the waves.", "", 0
    },
 
   {
  "ice storm",
  { 53,53,53,53,53,53,53,53,53,53,35,53,53,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_ice_storm,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(900), 40, 12,
  "storm of ice", "!Ice Storm!", "", 0
    }, 

    {
  "identify", 
  { 15,16,18,53,17,34,35,16,17,33,53,15,16,18,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_identify,   TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(53), 12, 24,
  "",     "!Identify!",   "", SS_SCRIBE
    },

    {
  "imbue",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_imbue,	TAR_CHAR_SELF,     	POS_FIGHTING,
  &gsn_imbue,	SLOT(994), 50,	24,
  "",	"Your holy enhancement fades away.", "", 0
     },
 
    {
  "immolate",
  {43,53,53,53,53,53,53,53,53,53,53,43,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_immolate, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(898), 75, 12,
  "immolation",   "!Immolate!", "", 0
    },
 
    {
  "incinerate",
  {38,53,53,53,53,53,53,53,53,53,53,38,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_incinerate, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(998), 50, 12,
  "incineration",   "!Incinerate!", "", 0
    },
 {
  "indulgence",
  {53,53,53,53,53,25,53,53,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2},
  spell_indulgence, TAR_CHAR_SELF, POS_STANDING,
  NULL,    SLOT(423),  35, 12,
  "",  "Your indulgence has ended.", "", 0
  },

  
    {
  "infravision", 
  { 9,13,10,16,12,15,13,11,10,13, 9, 9,13,10,16 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_infravision,  TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(77),  5, 18,
  "",     "You no longer see in the dark.", "", SS_SCRIBE|SS_STAFF
    },

    {
  "invisibility", 
  { 5,53, 9,53,30,53,30,28, 7,28,53, 5,53, 9,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_invis,    TAR_OBJ_CHAR_DEF, POS_STANDING,
  &gsn_invis,   SLOT(29),  5, 12,
  "",     "You are no longer invisible.",  "$p fades into view.", SS_SCRIBE|SS_STAFF
    },

    {
  "irradiate",
  {53,53,53,53,53,53,53,53,53,53,21,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_irradiate,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_irradiate,	SLOT(708),  100,	12,
  "",	"Some of the radiation sickness leaves your body.",	"", 0
    },

    {
  "knock",
  {53,53,53,53,53,53,53,53,53,53,53, 8,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_knock,	TAR_IGNORE,	POS_FIGHTING,
  NULL,	SLOT(955), 20, 12,
  "", "", "", SS_SCRIBE
    },

    {
  "know alignment", 
  { 12, 9,20,53,15,30,36,11,16,32,53,12, 9,20,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_know_alignment, TAR_CHAR_DEFENSIVE, POS_FIGHTING,
  NULL,     SLOT(58),  9, 12,
  "",     "!Know Alignment!", "", SS_SCRIBE
    },

    {
  "lacerate",
  {47,53,53,53,53,53,53,53,53,53,53,47,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_lacerate, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(998), 50, 12,
  "laceration",   "!Laceration!", "", 0
    },

    {
  "lay on hands", 
  { 53,29,53,53,40,40,53,40,53,53,53,53,29,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_lay_on_hands,   TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(300), 50, 12,
  "",     "!LoHands!",   "", 0
    },

    {
  "learned knowledge",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_scribe_knowledge, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL, SLOT(1001), 20, 12,
  "", "", "", SS_SCRIBE
    },
  
    {
  "lightning bolt", 
  { 13,23,18,16,21,20,17,18,16,15,13,13,23,18,16 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_lightning_bolt, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(30), 15, 12,
  "lightning bolt", "!Lightning Bolt!", "", SS_WAND
    },

    {
  "locate object", 
  { 9,15,11,53,13,33,31,12,10,30,53, 9,15,11,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_locate_object,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(31), 20, 18,
  "",     "!Locate Object!",  "", SS_SCRIBE
    },

    {
  "magic missile",  
  { 1,53, 2, 2,26,26, 2,26, 2, 2,53, 1,53, 2, 2 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_magic_missile,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(32), 15, 12,
  "magic missile",  "!Magic Missile!",  "", SS_WAND
    },

    {
  "magic resistance",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_magic_resistance, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_magic_resistance,	SLOT(1028), 40, 24,
  "", "The magic resistance leaves you.", "", 0
     },
    {
  "magical rest",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_scribe_rest, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL, SLOT(1003), 20, 12,
  "", "", "", SS_SCRIBE
    },


    {
  "mass healing", 
  { 53,38,53,46,45,42,53,45,53,49,53,53,38,53,46 }, 
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_mass_healing, TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(508),  100,  36,
  "",     "!Mass Healing!", "", 0
    },

    {
  "mass invis", 
  { 22,25,31,53,28,38,41,24,27,37,53,22,25,31,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_mass_invis, TAR_IGNORE,   POS_STANDING,
  &gsn_mass_invis,  SLOT(69), 20, 24,
  "",     "You are no longer invisible.",   "", SS_SCRIBE|SS_STAFF
    },

    {
  "midnight cloak",
  { 49,49,53,53,53,53,53,53,53,53,53,49,49,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_midnight_cloak, TAR_CHAR_SELF,   POS_STANDING,
  &gsn_midnight_cloak,  SLOT(1069), 200, 72,
  "",     "Your midnight cloak is gone.",   "", 0
    },

    {
  "mirror image", 
  { 53,37,53,53,44,44,53,44,53,53,53,53,37,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_mirror_image,   TAR_IGNORE, POS_FIGHTING,
  NULL,     SLOT(301), 50, 6,
  "",     "!MirroImage!",   "", 0
    },

    {
  "mistform",
  { 53,53,53,53,53,53,53,53,53,53,38,53,53,53,53 },
  {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_mistform,	TAR_CHAR_SELF,	POS_STANDING,
  NULL,	SLOT(725), 50, 24,
  "", "Your body becomes substantial once more.", "", SS_SCRIBE
    },

    {
    "nether shield",
    {25, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 25, 53, 53, 53},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    spell_nether_shield, TAR_IGNORE, POS_STANDING,
    &gsn_nether_shield, SLOT(623), 100, 36,
    "", "!Nether Shield!", "", 0
    },

    {
  "nexus", 
  { 40,35,50,45,43,40,48,38,45,43,53,40,35,50,45 }, 
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_nexus,            TAR_IGNORE,             POS_STANDING,
  NULL,                   SLOT(520),       150,   36,
  "",                     "!Nexus!",    "", SS_SCRIBE
    },

    {
  "orb of awakening",
  {53,53,53,53,53,53,53,10,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 2, 2},
  spell_orb_of_awakening,  TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(410),  10, 12,
  "",     "Your orb fades.", "", SS_STAFF
    },

    {
  "orb of surprise",
  {53,53,53,53,53,53,53,20,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 2, 2},
  spell_orb_of_surprise,  TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(411),  10, 12,
  "",     "Your orb fades.", "", 0
    },

    {
  "orb of touch",
  {53,53,53,53,53,53,53,15,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 2, 2},
  spell_orb_of_touch,  TAR_CHAR_SELF,      POS_STANDING,
  NULL,     SLOT(412),  50, 12,
  "",     "Your orb fades.", "", 0
    },

    {
  "orb of turning",
  {53,53,53,53,53,53,53,25,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 2, 2},
  spell_orb_of_turning,  TAR_CHAR_SELF,      POS_STANDING,
  NULL,     SLOT(410),  10, 12,
  "",     "Your orb fades.", "", 0
    },

    {
  "pass door", 
  { 24,32,25,37,29,35,31,28,25,31,18,24,32,25,37 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_pass_door,  TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(74), 20, 12,
  "",     "You feel solid again.",  "", 0
    },

    
    {
  "plague", 
  { 23,17,36,26,27,22,31,20,30,25,53,23,17,36,26 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_plague,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_plague,    SLOT(503),  20, 12,
  "sickness",   "Your sores vanish.", "", SS_SCRIBE|SS_WAND
    },

    
    {
  "poison", 
  { 17,12,15,21,14,17,18,15,16,19,53,17,12,15,21 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1,53, 1, 1, 2, 2 },
  spell_poison,   TAR_OBJ_CHAR_OFF, POS_FIGHTING,
  &gsn_poison,    SLOT(33), 10, 12,
  "poison",   "You feel less sick.",  "The poison on $p dries up.", SS_SCRIBE|SS_WAND
    },

    {
  "portal", 
  { 35,30,45,40,38,35,43,33,40,38,53,35,30,45,40 }, 
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 1, 1, 2, 2 }, 
  spell_portal,           TAR_IGNORE,             POS_STANDING,
  NULL,                   SLOT(519),       100,     24,
  "",                     "!Portal!",   "", SS_SCRIBE
    },

    {
  "pox",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_scribe_pox,    TAR_CHAR_OFFENSIVE, POS_STANDING,
  NULL,     SLOT(1000),  15, 12,
  "wretched putrid disease",     "...ashes ashes they all fall down! Wow, it worked.", "", SS_SCRIBE
    },

    {
  "protection evil", 
  { 12, 9,17,11,13,10,14,11,15,12,53,12, 9,17,11 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_protection_evil,  TAR_CHAR_SELF,    POS_STANDING,
  NULL,     SLOT(34),   5,  12,
  "",     "You feel less protected.", "", SS_SCRIBE
    },

    {
  "protection good", 
  { 12, 9,17,11,13,10,14,11,15,12,53,12, 9,17,11 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_protection_good,  TAR_CHAR_SELF,          POS_STANDING,
  NULL,                   SLOT(514),       5,     12,
  "",                     "You feel less protected.", "", SS_SCRIBE
    },

    {
  "protection neutral", 
  {12, 9,17,11,13,10,14,11,15,12,53,12, 9,17,11 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },        
  spell_protection_neutral, TAR_CHAR_SELF,        POS_STANDING,
  &gsn_protect_neutral,    SLOT(498),     5, 12,
  "",     "You feel less protected.", "", SS_SCRIBE
    },


    {
  "psionic blast", 
  { 53, 6,53,53,29,29,53,29,53,53,53,53, 6,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1 ,1, 1, 1, 1, 2, 2 },
  spell_psionic_blast,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
  NULL,                   SLOT(302),      15,     12,
  "psionic blast",         "!PSionic Blast!", "", 0
     },
/*
    {
  "scion storm",
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 },
  {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  spell_scion_storm,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(345),	20, 12,
  "scionic storm",	"!Scion Storm!", "", 0
    },
 */   
    {
   "ray of truth", 
   { 53,35,53,47,43,41,49,43,53,49,53,53,35,53,47 }, 
   {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
   spell_ray_of_truth,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
   NULL,                   SLOT(518),      20,     12,
   "ray of truth",         "!Ray of Truth!", "", 0
    },

    {
  "recharge", 
  { 9,53,53,53,53,53,53,30,30,30,53, 9,53,53,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_recharge,   TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(517),  60, 24,
  "",     "!Recharge!",   "", SS_SCRIBE
    },

    {
  "refresh", 
  { 8, 5,12, 9, 9, 7,11, 7,10, 9, 8, 8, 5,12, 9 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_refresh,    TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(81), 12, 18,
  "refresh",    "!Refresh!",    "", SS_SCRIBE|SS_STAFF
    },

    {
  "regeneration",
  {53,20,53,53,39,39,53,28,53,53,53,53,20,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_regeneration, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,	SLOT(1023), 150, 48,
  "", "You have stopped regenerating.", "", 0
    },

    {
  "remove curse", 
  { 53,18,53,22,35,20,37,35,53,37,53,53,18,53,22 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_remove_curse, TAR_OBJ_CHAR_DEF, POS_STANDING,
  NULL,     SLOT(35),  5, 12,
  "",     "!Remove Curse!", "", SS_SCRIBE|SS_WAND|SS_STAFF
    },

    {
  "remove disease",
  { 53,22,53,53,53,37,53,29,53,53,53,53,22,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_remove_disease, TAR_OBJ_CHAR_DEF, POS_STANDING,
  NULL,     SLOT(1035),  75, 24,
  "",     "!Remove Disease", "", SS_SCRIBE|SS_STAFF
    },

    {
  "remove poison",
  { 53,27,53,53,53,42,53,34,53,53,53,53,27,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_remove_poison, TAR_OBJ_CHAR_DEF, POS_STANDING,
  NULL,     SLOT(1036),  75, 24,
  "",     "!Remove Poison", "", SS_SCRIBE|SS_STAFF
    },

    {
  "restore mana",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  spell_restore_mana, TAR_CHAR_DEFENSIVE,	POS_STANDING,
  NULL,	SLOT(1024), 30, 24,
  "", "", "", 0
    },
    {
  "restrain",
  { 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53},
  { 1 , 1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1 },
  spell_restrain, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  &gsn_spell_restrain,    SLOT(965), 50, 12,
  "restrain", "You no longer feel restrained.", "", 0
    },
    
    {
  "rust armor",
  {53,53,53,53,53,53,53,53,53,53,23,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_rust_armor,	TAR_CHAR_OFFENSIVE,  	POS_FIGHTING,
  NULL,	SLOT(710),	80,	24,
  "", "", "", 0
    },

    {
  "rust weapon",
  {53,53,53,53,53,53,53,53,53,53,25,53,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_rust_weapon,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(711),	80,	24,
  "", "", "",0 
    },

    {
  "sacred guardian",
  {53,53,53,53,53,53,53,53,53,53,53,53,15,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_sacred_guardian, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_sacred_guardian, SLOT(913), 50, 24,
  "", "You are not longer being watched by the gods.", "", 0
    },

    {
  "sanctuary", 
  { 36,20,42,30,31,25,36,28,39,33,20,36,20,42,30 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_sanctuary,  TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_sanctuary,   SLOT(36), 75, 12,
  "",     "The white aura around your body fades.", "", SS_SCRIBE|SS_WAND|SS_STAFF
    },

     {
    "scourge",
  {48,48,52,52,52,52,52,52,52,52,52,48,48,52,52},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4},
  spell_scourge, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL, SLOT(918),  75, 24,
  "scourge", "You stop sweating as the Scourge fades.", "", 0
    },

    {
  "shield", 
  { 20,35,35,40,35,38,38,28,28,30,53,20,35,35,40 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_shield,   TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(67), 12, 18,
  "",     "Your force shield shimmers then fades away.", "", SS_SCRIBE|SS_STAFF|SS_STAFF
    },
    {
  "shield of blades",
  { 53,53,53,53,53,53,53,40,53,53,45,53,53,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_shield_of_blades, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_shield_of_blades,     SLOT(1531),  250, 72,
  "",     "Your shield of blades is gone.", "", 0
    },

    {
  "shield of brambles",
  { 53,53,53,53,53,53,53,20,53,53,25,53,53,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_shield_of_brambles, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_shield_of_brambles,     SLOT(1529),  150, 72,
  "",     "Your shield of brambles is gone.", "", 0
    },

    {
  "shield of faith",
  { 53,13,53,53,25,22,53,25,53,53,53,53,13,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_shield_of_faith, TAR_CHAR_SELF, POS_STANDING,
  &gsn_shield_of_faith,     SLOT(1508),  50, 36,
  "",     "Your shield of faith is gone.", "", 0
    },

    {
  "shield of spikes",
  { 53,53,53,53,53,53,53,30,53,53,35,53,53,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_shield_of_spikes, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_shield_of_spikes,     SLOT(1530),  200, 72,
  "",     "Your shield of spikes is gone.", "", 0
    },

    {
  "shield of thorns",
  { 53,53,53,53,53,53,53,10,53,53,15,53,53,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 1, 2, 2, 4, 4 },
  spell_shield_of_thorns, TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_shield_of_thorns,     SLOT(1528),  100, 72,
  "",     "Your shield of thorns is gone.", "", 0
    },

    {
  "shocking grasp", 
  { 10,53,14,13,33,32,14,31,12,12,10,10,53,14,13 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_shocking_grasp, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(53), 15, 12,
  "shocking grasp", "!Shocking Grasp!", "", SS_WAND
    },

    {
  "sleep", 
  { 10,53,11,53,31,53,31,31,11,31,53,10,53,11,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_sleep,    TAR_CHAR_OFFENSIVE, POS_STANDING,
  &gsn_sleep,   SLOT(38), 15, 24,
  "",     "You feel less tired.", "", SS_SCRIBE|SS_WAND
    },

    {                                
  "slow", 
  { 23,30,29,32,30,31,30,27,26,28,53,23,30,29,32 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_slow,             TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
  NULL,                   SLOT(515),      35,     12,
  "",                     "You feel yourself speed up.",  "", SS_SCRIBE|SS_WAND 
    },
   {
  "smoke screen",
  {53,53,53,53,30,53,53,53,30,30,53,53,53,53,53},
  { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2},
  spell_smoke_screen,   TAR_IGNORE, POS_FIGHTING,
  NULL,     SLOT(408), 40, 6,
  "",     "You can see again.",   "", 0
    },

  {
  "see soul",
  {53,53,53,53,53,15,53,53,53,53,53,53,53,53,53},
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 1, 2, 2},
  spell_see_soul, TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,    SLOT(422),  10, 12,
  "",  "", "", 0
  },

    {
  "sonic blast",
  {45,53,53,53,53,53,53,50,53,53,50,45,53,53,53},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2,4, 4 },
  spell_sonic_blast,	TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,	SLOT(1245), 75, 12,
  "sonic blast",	"!Sonic Blast!", "", 0
    },

    {
  "speed",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_speed,	TAR_CHAR_DEFENSIVE,	POS_STANDING,
  NULL,	SLOT(1026), 30, 24,
  "", "Your movements slow down.", "", 0
     },

    {
  "spirit of bear",
  {53,53,53,53,53,53,53,34,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_spirit_of_bear,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  &gsn_spirit_of_bear, SLOT(1087), 1, 2,
  "", "The spirit of bear leaves you.", "", 0
     },
    {
  "spirit of boar",
  {53,53,53,53,53,53,53,33,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_spirit_of_boar,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  &gsn_spirit_of_boar, SLOT(1086), 1, 2,
  "", "The spirit of boar leaves you.", "", 0
     },

    {
  "spirit of cat",
  {53,53,53,53,53,53,53,35,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_spirit_of_cat,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  &gsn_spirit_of_cat, SLOT(1088), 1, 2,
  "", "The spirit of cat leaves you.", "", 0
     },

    {
  "spirit of owl",
  {53,53,53,53,53,53,53,36,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_spirit_of_owl,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  &gsn_spirit_of_owl, SLOT(1089), 1, 2,
  "", "The spirit of owl leaves you.", "", 0
     },

    {
  "spirit of phoenix",
  { 29,22,53,53,37,37,53,26,40,40,53,29,22,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_spirit_phoenix,      TAR_CHAR_SELF,    POS_STANDING,
  NULL,                   SLOT(552),      250,      36,
  "",             "The Spirit of Phoenix leaves you.", "", 0
    },

    {
  "spirit of wolf",
  {53,53,53,53,53,53,53,37,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_spirit_of_wolf,  TAR_CHAR_DEFENSIVE,     POS_FIGHTING,
  &gsn_spirit_of_wolf, SLOT(1085), 1, 2,
  "", "The spirit of wolf leaves you.", "", 0
     },

    {
  "stalk",
    { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
    {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
    spell_stalk, TAR_IGNORE, POS_STANDING,
    NULL, SLOT(1090),100, 12,
    "","","",0
    },

    {
  "steel skin",
  {35,53,53,53,53,53,53,53,53,53,53,35,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_steel_skin,	TAR_CHAR_SELF,	POS_STANDING,
  &gsn_steel_skin,	SLOT(1222),  100, 48,
  "", "Your skin becomes flesh again.", "", 0
    },

    {
  "stonefist",
  {53,53,53,53,53,53,53,53,53,53,15,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_stonefist, 	TAR_CHAR_SELF,	POS_STANDING,
  &gsn_stonefist,	SLOT(705),	30,	18,
  "",	"Your hands become flesh and blood once more.", "", 0
    },

    {
  "stone skin", 
  { 25,40,40,45,40,43,43,33,33,35,25,25,40,40,45 }, 
  {  1, 1, 2 ,2 ,1 ,1 ,2 ,1 ,1 ,1, 1, 1, 1, 2, 2 },
  spell_stone_skin, TAR_CHAR_SELF,    POS_STANDING,
  &gsn_stone_skin,     SLOT(66), 12, 18,
  "",     "Your skin feels soft again.",  "", SS_SCRIBE
    },

    {
  "summon", 
  { 24,12,29,22,21,17,26,18,27,23,53,24,12,29,22 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_summon,   TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(40), 50, 12,
  "",     "!Summon!",   "", SS_SCRIBE
    },

    {
  "sunburst",
  { 44,53,44,44,53,53,53,49,53,53,49,44,53,53,53 },
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_sunburst,	TAR_CHAR_OFFENSIVE, 	POS_FIGHTING,
  &gsn_sunburst,	SLOT(1243), 65, 12,
  "sunburst", "Your vision returns.", "", 0
    },

/*Added swarm 08OCT00 - Boogums */
    {
      "swarm",
      {53,53,53,53,53,53,53,10,53,53,53,53,53,53,53},
      { 2, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 2},
      spell_swarm, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
      NULL, SLOT(527), 100, 12,
      "attacking swarm","","", 0
    },


    {
  "symbol i",
  { 53,16,53,53,53,53,53,53,53,53,53,53,16,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_symbol_1, 	TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_symbol_1,	SLOT(1511), 100, 72,
  "", "The glowing symbol fades from your vision.", "", 0
    },

    {
  "symbol ii",
  { 53,26,53,53,53,53,53,53,53,53,53,53,26,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_symbol_2,       TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_symbol_2, SLOT(1512), 200, 72,
  "", "The glowing symbol fades from your vision.", "", 0
    },

    {
  "symbol iii",
  { 53,36,53,53,53,53,53,53,53,53,53,53,36,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_symbol_3,       TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_symbol_3, SLOT(1513), 400, 72,
  "", "The glowing symbol fades from your vision.", "", 0
    },

    {
  "symbol iv",
  { 53,46,53,53,53,53,53,53,53,53,53,53,46,53,53 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_symbol_4,       TAR_CHAR_DEFENSIVE, POS_STANDING,
  &gsn_symbol_4, SLOT(1514), 600, 72,
  "", "The glowing symbol fades from your vision.", "", 0
    },

    {
  "teleport", 
  { 13,22,25,36,24,29,31,18,19,25,53,13,22,25,36 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_teleport,   TAR_CHAR_OFFENSIVE,   POS_FIGHTING,
  NULL,     SLOT( 2), 35, 12,
  "",     "!Teleport!",   "", SS_SCRIBE
    },

    {
  "thunderclap",
  {53,53,53,53,53,53,53,53,53,53,49,53,53,53,53},
  { 2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4},
  spell_thunderclap,     TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
  NULL, SLOT(752), 100, 24,
  "thunderclap",   "The ringing in your ears finally stops.", "", 0
     },

    {
  "tower of iron will", 
  {53,14,53,53,33,33,53,33,53,53,53,53,14,53,53}, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_tower_of_iron_will,   TAR_CHAR_DEFENSIVE, POS_STANDING,
  NULL,     SLOT(303), 12, 18,
  "",     "Your will has faded.", "", SS_SCRIBE|SS_STAFF
    },

    {
  "traveler fog", 
  {53,53,53,53,53,53,53,53,53,53,19,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_travel_fog,	TAR_IGNORE,	POS_STANDING,
  NULL,	SLOT(707), 20, 6,
  "",	"", "", 0
     },

    {
  "tsunami", 
  {53,53,53,53,53,53,53,53,53,53,29,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_tsunami,	TAR_IGNORE,	POS_FIGHTING,
  NULL,	SLOT(709), 50, 12,
  "tsunami",	"", "", 0
     },

    {
  "venom of vhan",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53 },
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  spell_venom_of_vhan,	TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL, SLOT(472), 75, 24,
  "venom of Vhan", "The venom has run its course.", "", 0
   },

    {
  "ventriloquate", 
  { 1,53, 2,53,27,53,27,26, 2,26,53, 1,53, 2,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_ventriloquate,  TAR_IGNORE,   POS_STANDING,
  NULL,     SLOT(41),  5, 12,
  "",     "!Ventriloquate!",  "", SS_SCRIBE
    },

    {
  "vision",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_vision,	TAR_CHAR_DEFENSIVE,	POS_STANDING,
  &gsn_vision,	SLOT(1027),	10,	18,
  "", "Your vision fades.", "", 0
    },

    {
  "visit node",
  {53,53,53,53,53,53,53,53,53,53,28,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_visit_node,	TAR_IGNORE,	POS_FIGHTING,
  NULL,	SLOT(716),	40,	18,
  "",	"",	"", 0
     },

    {
  "wall of ice",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_wall_of_ice,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_wall_ice,	SLOT(758),	150,	24,
  "wall of ice",	"The wall of ice slowly melts away.", 	"", 0
    },

    {
  "wall of fire",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_wall_of_fire,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  &gsn_wall_fire,	SLOT(758),	30,	24,
  "wall of fire",	"!Wall of Fire!", 	"", 0
    },
    {
  "wall of wind",
  {53,53,53,53,53,53,53,53,53,53,11,53,53,53,53 },
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_wall_of_wind,   TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
  NULL, SLOT(704),      30,     24,
  "wall of wind",       "!Wall of Wind!",       "", 0
    },

    {
  "warp", 
  { 8, 3, 8,12, 6, 8,10, 6, 8,10,53, 8, 3, 8,12 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_warp, TAR_OBJ_INV,    POS_STANDING,
  NULL,     SLOT(626),  10, 12,
  "",     "!Warp!", "", SS_SCRIBE
    },

    {
  "water breathing",
  {53,53,53,53,53,53,53,53,53,53, 1,53,53,53,53},
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_water_breathing,	TAR_CHAR_SELF,	POS_STANDING,
  &gsn_water_breathing,	SLOT(701),	10,	12,
  "",	"Your lungs spasm briefly.", "", 0
    },
    {
  "weaken", 
  { 11,14,16,17,15,16,17,13,14,14,53,11,14,16,17 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_weaken,   TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(68), 20, 12,
  "spell",    "You feel stronger.", "", SS_SCRIBE
    },

    {
  "word of recall", 
  { 9, 5,11,10, 8, 8,11, 7,10,10, 9, 9, 5,11,10 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_word_of_recall, TAR_CHAR_DEFENSIVE,    POS_RESTING,
  NULL,     SLOT(42),  5, 12,
  "",     "!Word of Recall!", "", SS_SCRIBE
    },

    {
  "wrath",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_wrath,	TAR_CHAR_OFFENSIVE,	POS_FIGHTING,
  NULL,	SLOT(939), 25, 12,
  "spiritual wrath", "The wrath of the gods leaves you.", "", 0
    },

    {
  "wrath of the pen",
  {52,52,52,52,52,52,52,52,52,52,52,52,52,52,52},
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
  spell_scribe_maguswrath, TAR_CHAR_OFFENSIVE, POS_STANDING,
  NULL, SLOT(1002), 20, 12,
  "wrath of the pen", "", "", SS_SCRIBE
    },

/*
 * Dragon breath
 */
    {
  "acid breath", 
  { 31,32,33,34,33,33,34,32,32,33,31,31,32,33,34 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_acid_breath,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(200),  100,  24,
  "blast of acid",  "!Acid Breath!",  "", 0
    },

    {
  "fire breath", 
  { 40,45,50,51,48,48,51,43,45,46,40,40,45,50,51 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_fire_breath,  TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(201),  200,  24,
  "blast of flame", "The smoke leaves your eyes.",  "", 0
    },

    {
  "frost breath", 
  { 34,36,38,40,37,38,39,35,36,37,34,34,36,38,40 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_frost_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(202),  125,  24,
  "blast of frost", "!Frost Breath!", "", 0
    },

    {
  "gas breath", 
  { 39,43,47,50,45,47,49,41,43,45,39,39,42,47,50 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_gas_breath, TAR_IGNORE,   POS_FIGHTING,
  NULL,     SLOT(203),  175,  24,
  "blast of gas",   "!Gas Breath!",   "", 0
    },

    {
  "lightning breath", 
  { 37,40,43,46,42,43,45,39,40,42,37,37,40,43,46 },     
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_lightning_breath, TAR_CHAR_OFFENSIVE, POS_FIGHTING,
  NULL,     SLOT(204),  150,  24,
  "blast of lightning", "!Lightning Breath!", "", 0
    },

/*
 * Spells for mega1.are from Glop/Erkenbrand.
 */
    {
     "small bomb", 
     {53,53,53,53,20,53,53,53,20,20,53,53,53,53,53}, 
     { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2},
	spell_general_purpose,  TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(406),      10,      12,
	"small powder bomb", "!General Purpose Ammo!", "", 0
    },
 
    {
      "high explosive", 
      {53,53,53,53,40,53,53,53,40,40,53,53,53,53,53}, 
      { 0, 0, 0, 0, 1, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2},
	spell_high_explosive,   TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	NULL,                   SLOT(407),      10,      12,
	"high explosive bomb",  "!High Explosive Ammo!",  "", 0
    },

/* ALtirin */
    {
      "frostbite",
      {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        spell_frostbite,        TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
        NULL,                   SLOT(1407),      100, 	24,
        "frostbite", "The frost melts away.", "", 0
    },

    {
      "blistering skin",
      {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        spell_blistering_skin,   TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
        NULL,                   SLOT(1408),      100,      24,
	"blistering skin",	"The heat wave fades.", "", 0
    },

    {
      "electrocution",
      {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
      { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1},
        spell_electrocution,   TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
        NULL,                   SLOT(1409),      100,      24,
        "electrocution",	"The electricity exits your body.", "", 0
    },

/*
 *  Necromantic spells.. from Kris
 */
    {
  "animate dead", 
  { 32,25,53,53,38,38,53,29,42,42,53,32,25,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_animate_dead,     TAR_IGNORE,     POS_STANDING,
  NULL,                   SLOT(400),      75,     18,
  "",             "!Animate Dead!", "", SS_SCRIBE
    },
    
    {
  "summon dead", 
  { 23,18,53,53,35,35,53,21,37,37,53,28,18,53,53 },     
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_summon_dead,     TAR_IGNORE,     POS_STANDING,
  NULL,                   SLOT(401),      100,     18, 
  "",             "!Summon Dead!", "", SS_SCRIBE
    },
  {
  "summon elemental",
  {53,53,53,53,53,53,53,53,53,53,20,53,53,53,53},
  {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  spell_summon_elemental, TAR_IGNORE, POS_STANDING,
  NULL,    SLOT(425),   100, 12,
  "", "", "", 0
  },


    {
  "cryogenesis", 
  { 8, 5,53,53,28,28,53, 7,30,30,53, 8, 5,53,53 }, 
  { 1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_cryogenesis,      TAR_OBJ_INV,    POS_STANDING,
  NULL,                   SLOT(402),      5,      8, 
  "",             "!Cryogenesis!", "", SS_SCRIBE
    },

    {
  "draw life", 
  { 25,20,53,53,36,36,53,23,38,38,53,25,20,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_draw_life,      TAR_OBJ_INV,    POS_STANDING,
  NULL,                   SLOT(403),      50,      12,
  "",             "!Draw Life!", "", SS_WAND
    },

    {
  "make bag", 
  { 25,20,53,53,36,36,53,23,38,38,53,25,20,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_make_bag,      TAR_OBJ_INV,    POS_STANDING,
  NULL,                   SLOT(560),     100,      24,
  "",             "!Make Bag!", "", 0
    },
  /*here goes the wraithform spell for the necro kit -Boogums*/
{
  "wraithform",
      { 10,53,53,53,53,53,53,53,53,53,53,10,53,53,53 },
      {  1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 1, 1, 2, 2 },
      spell_wraithform,  TAR_CHAR_SELF,    POS_STANDING,
      NULL,     SLOT(528), 75, 36,
      "",     "You are no longer a shadowy character.",  "", 0
    },
    {
  "turn undead",
  { 20,15,53,53,33,33,53,18,36,36,53,20,15,53,53},
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2},
  spell_turn_undead,      TAR_IGNORE,    POS_FIGHTING,
  NULL,                   SLOT(404),      35,      8,
  "turn undead",             "!Turn Undead!", "",  SS_WAND
    },


/* by Ben 
    {
  "feign death", 
  { 23,19,53,53,36,36,53,38,38,38,53,23,19,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_feign_death,    TAR_CHAR_SELF,  POS_STANDING,
  NULL,         SLOT(499),      20,     12,
  "",   "Your flesh looks alive again.", "", 0
    },
*/
    {
  "withstand death", 
  { 29,22,53,53,37,37,53,26,40,40,53,29,22,53,53 }, 
  {  1, 1, 2, 2, 1, 1, 2, 1, 1, 1, 1, 1, 1, 2, 2 },
  spell_withstand_death,      TAR_CHAR_SELF,    POS_STANDING,
  &gsn_withstand_death,       SLOT(405),      50,      12,
  "",             "You no longer feel more powerful than death.", "", 0
    },

/* By Ben */
    {
  "wound transfer", 
  { 39,34,53,53,46,46,53,37,46,46,53,39,34,53,53 },
  {  2, 2, 4, 4, 2, 2, 4, 2, 2, 2, 2, 2, 2, 4, 4 },
  spell_wound_transfer, TAR_CHAR_SELF,  POS_STANDING,
  &gsn_wound_transfer,  SLOT(417),      75,     18,
  "",   "Your soul is fully vulnerable once more.", "", 0
    },

/* combat and weapons skills */

    {
  "axe",      
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },  
  { 6,6,5,4,6,5,5,6,6,5,9,6,6,5,4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_axe,             SLOT( 0),       0,      0,
	"",                     "!Axe!",    "", 0
    },

    {
  "dagger", 
  {  1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  {  2,3,2,2,3,3,2,3,2,2,2,2,3,2,2 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_dagger,            SLOT( 0),       0,      0,
	"",                     "!Dagger!",   "", 0
    },
 
    {
  "flail",    
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 6,3,6,4,5,4,5,5,6,5,3,6,3,6,4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_flail,             SLOT( 0),       0,      0,
	"",                     "!Flail!",    "", 0
    },

    {
  "mace",     
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },  
  { 5,2,3,3,3,3,3,4,4,4,2,5,2,3,3 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_mace,              SLOT( 0),       0,      0,
	"",                     "!Mace!",   "", 0
    },

    {
  "polearm", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },  
  { 6,6,6,4,6,5,5,6,6,5,9,6,6,6,4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_polearm,           SLOT( 0),       0,      0,
	"",                     "!Polearm!",    "", 0
    },
   /*** 
    {
  "shield block", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 6,4,6,2,5,3,4,5,6,4,5,6,4,6,2 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_shield_block,  SLOT(0),  0,  0,
  "",     "!Shield!",   "", 0
    },
    ***/
    {
  "spear", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 4,4,4,3,4,4,4,4,4,4,4,4,4,4,3 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_spear,             SLOT( 0),       0,      0,
	"",                     "!Spear!",    "", 0
    },

    {
  "sword", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },  
  { 5,6,3,2,5,4,3,6,4,4,6,5,6,3,2 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_sword,             SLOT( 0),       0,      0,
	"",                     "!sword!",    "", 0
    },

    {
  "whip", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },  
  { 6,5,5,4,5,5,5,6,6,5,7,6,5,5,4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_whip,              SLOT( 0),       0,      0,
	"",                     "!Whip!", "", 0
    },

    {
  "backstab", 
  { 53,53, 1,53,26,53,26,53,26,53,53,53,53, 1,53 }, 
  {  0, 0, 5, 0, 5, 0, 5, 0, 5, 0, 0, 0, 0, 5, 0 },
	spell_null,             TAR_IGNORE,             POS_STANDING,
	&gsn_backstab,          SLOT( 0),        0,     24,
	"backstab",             "!Backstab!",   "", 0
    },

    {
  "throw",     
  { 53,53,53,53,53,53,53,53,53, 1,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0 ,0 ,0 ,0 ,4, 0, 0, 0, 0, 0 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_throw,              SLOT( 0),       0,      24,
	"throw",                 "!Throw!",   "", 0
    },

    {
  "barbarian rage",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4 },
  spell_null,	TAR_IGNORE, 	POS_FIGHTING,
  &gsn_barbarian_rage,	SLOT( 0),	0,	0,
  "",	"Your blood cools as your primal rage leaves you.", "", 0
    },

    {
  "bash",     
  { 53,53,53, 1,53,26,26,53,53,26,53,53,53,53, 1 }, 
  {  0, 0, 0, 4, 0, 4, 4, 0, 0, 4, 0, 0, 0, 0, 4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_bash,              SLOT( 0),       0,      24,
	"bash",                 "!Bash!",   "", 0
    },

    {
  "berserk", 
  { 45, 45, 45,18, 45,35,13, 45, 45,35, 45,45,45,45,18 }, 
  {-10,-10,-10, 5,-10, 5, 5,-10,-10, 5,-10,-10,-10,-10,5 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_berserk,         SLOT( 0),       0,      24,
	"",                     "You feel your pulse slow down.", "", 0
    },

    {
  "bite", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -9,-9,-8,-8,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9,-9 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_bite,    SLOT( 0), 0,  24,
  "bite",    "!Bite!", "", 0
    },

    {
  "bladesong",
  { 20,20,20,20,20,20,20,20,20,20,20,20,20,20,20 },
  { -4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_bladesong,	SLOT( 0), 0, 12,
  "", "Your blades stop dancing.", "", 0
    },

    {
  "bleed", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -8,-8,-9,-9,-9,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_bleed,    SLOT( 0), 0,  12,
  "bleed",    "!Bleed!", "", 0
    },

    {
  "bludgeon",
  { 53,53,53,19,53,39,39,53,53,39,53,53,53,53,19 },
  {  0, 0, 0, 7, 0,10,10, 0, 0,10, 0, 0, 0, 0, 7 },
  spell_null,	TAR_IGNORE, POS_FIGHTING,
  &gsn_bludgeon, SLOT(0), 0, 0,
  "",	"!Bludgeon!", "", 0
    },

    {
  "breathe", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -9,-9,-8,-8,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9,-9 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_breathe,    SLOT( 0), 0,  24,
  "breathe",    "!Breathe!", "", 0
    },

    {
  "cleave",
  { 53,53,53,23,53,43,43,53,53,43,53,53,53,53,23 },
  {  0, 0, 0, 7, 0,10,10, 0, 0,10, 0, 0, 0, 0, 7 },
  spell_null,   TAR_IGNORE, POS_FIGHTING,
  &gsn_cleave, SLOT(0), 0, 12, 
  "", "!Cleave!", "", 0
    },
    {
 "connive",
 { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
 { -5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_connive,  SLOT( 0), 0,  0,
  "",     "!connive!", "", 0
    },


    {
  "cutpurse",
  {53,53,53,53,53,53,53,53,53,53,53,53,53,20,53},
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0},
  spell_null, TAR_IGNORE,     POS_FIGHTING,
  &gsn_cutpurse,    SLOT( 0),       0,      0,
  "", "!Cutpurse!", "", 0
    },

    {
  "daetok",  
  { 15,15,15,15,15,15,15,15,15,15,15,15,15,15,15 },
  { -6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_dae_tok,	SLOT( 0), 0, 24,
  "", "You relax from your state of combat readiness.", "", 0
    },

    {
  "dirt kicking", 
  { 53,53, 3, 3,27,27, 3,53,27,27,53,53,53, 3, 3 }, 
  {  0, 0, 4, 4, 4, 4, 4, 0, 4, 4, 0, 0, 0, 4, 4 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_dirt,    SLOT( 0), 0,  24,
  "kicked dirt",    "You rub the dirt out of your eyes.", "", 0
    },

    {
  "disarm", 
  { 53,53,12,11,32,31,12,53,32,31,53,53,53,12,11 }, 
  {  0, 0, 6, 4, 6, 4, 5, 0, 6, 4, 0, 0, 0, 6, 4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_disarm,            SLOT( 0),        0,     24,
	"",                     "You regain strength in your hand.",   "", 0
    },
 
    {
   "dodge", 
   { 20,22, 1,13,12,18, 7,21,11,17,21,20,22, 1,13 }, 
   {  8, 8, 4, 6, 6, 7, 5, 8, 6, 7, 8, 8, 8, 4, 6 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_dodge,             SLOT( 0),        0,     0,
	"",                     "!Dodge!",    "", 0
    },

    {
    "dual wield",
    {53,53,36,24,53,53,53,53,53,53,53,53,53,36,24},
    { 0, 0,-9,-7, 0, 0, 0, 0, 0, 0, 0, 0, 0,-9, 7},
	spell_null,	TAR_IGNORE,	POS_FIGHTING,
	&gsn_dual_wield,	SLOT( 0),	0,	0,
	"",	"!Dual Wield!",	"", 0
    },
  
    {
    "endurance",
    { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
    { -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9 },
    spell_null,	TAR_IGNORE, POS_FIGHTING,
    &gsn_endurance,	SLOT( 0), 0, 0,
    "", "", "", 0
    },

    {
    "eye gouge",
    {53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
    spell_null, TAR_IGNORE, POS_FIGHTING,
    NULL, SLOT( 0), 0, 0,
    "gouge", "You can see again.", "", 0
    },

    {
  "fear", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -9,-9,-8,-8,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9,-9 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_fear,    SLOT( 0), 0,  12,
  "fear",    "!fear!", "", 0
    },

    {
  "grab",  
  { 5,5,5,5,5,5,5,5,5,5,5,5,5,5,5 },
  { -3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3,-3 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_grab,	SLOT( 0), 0, 24,
  "grab", "You let go.", "", 0
    },

    {
  "hai-ruki",
  { 53,53,53,12,53,53,53,53,53,12,53,53,53,53,12 },
  { -9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  NULL,	SLOT( 0 ), 0, 0,
  "", "!hai-ruki!", "", 0
    },

    {
  "hex", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -9,-9,-8,-8,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9,-9 }, 
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_hex,    SLOT( 0), 0,  12,
  "hex",    "!hex!", "", 0
    },
     
     {
    "infiltrate",
    {53,53,53,53,53,53,53,53,53,53,53,53,53,10,53},
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0},
    spell_null,	TAR_IGNORE,	POS_STANDING,
    &gsn_infiltrate,	SLOT( 0),	0,	0,
    "",	"!Infiltrate!", "", 0
    },

    {
  "kailindo", 
   { 53,53,53,53,8,53,53,53,53,53,53,53,53,53,53 }, 
   { 0, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_kailindo,             SLOT( 0),        0,     0,
	"",                     "!Kailindo!",    "", 0
    },
   
    {
 "killing rage",
 { 20,20,20,20,20,20,20,20,20,20,20,20,20,20,20 },
 { -4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4 },
 spell_null,	TAR_IGNORE,	POS_FIGHTING,
 &gsn_rage,	SLOT( 0), 0, 12,
 "", "", "", 0
    },

    {
  "enhanced critical",
  { 53,53,53,16,53,36,36,53,53,36,53,53,53,53,16 },
  {  0, 0, 0, 7, 0,10,10, 0, 0,10, 0, 0, 0, 0, 7 },
  spell_null,   TAR_IGNORE, POS_FIGHTING,
  &gsn_enhanced_critical, SLOT(0), 0, 12,
  "", "!Enhanced Critical!", "", 0
    },


    {
 "enhanced damage", 
 { 45,30,25, 1,28,16,13,38,35,23,30,45,30,25, 1 }, 
 { 10, 9, 5, 3, 7, 6, 4,10, 8, 7, 9,10, 9, 5, 3 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_enhanced_damage,   SLOT( 0),        0,     0,
	"",                     "!Enhanced Damage!",  "", 0
    },

    {
  "espionage",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
  spell_null,	TAR_IGNORE, POS_FIGHTING,
  &gsn_espionage,	SLOT( 0), 0, 0,
  "", "", "", 0
    },

    {
  "tumbling",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6,-6 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_tumbling,	SLOT( 0),	0,	0,
  "",	"!Tumbling!",	"", 0
    },

    {
  "weave resistance", 
  { 53,53,53,53,53,53, 8,53,53,53,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0 },
	spell_null,             TAR_IGNORE,             POS_SLEEPING,
	&gsn_weave_resistance,           SLOT( 0),        0,     0,
	"",                     "!Weave Resistance!",    "", 0
    },

    {
  "insanity", 
  { 53,53,53,53,53,53,16,53,53,53,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_insanity,           SLOT( 0),        0,     12,
	"insane attack",         "!Insanity!",    "", 0
    },
 
    {
  "envenom", 
  { 53,53,10,53,31,53,31,53,31,53,53,53,53,10,53 }, 
  {  0, 0, 4 ,0 ,4 ,0 ,4 ,0 ,4 ,0, 0, 0, 0, 4, 0 },
  spell_null,   TAR_IGNORE,     POS_RESTING,
  &gsn_envenom,   SLOT(0),  0,  36,
  "",     "!Envenom!",    "", 0
    },

    {
  "garotte", 
  { 53,53,53,53,53,53,53,53, 8,53,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0 },
	spell_null,             TAR_CHAR_OFFENSIVE,     POS_STANDING,
	&gsn_garotte,              SLOT( 0),        0,     12,
	"garotte",                 "You regain consciousness.",   "", 0
    },

    {
 "hamstring",
 { 53,53,31,53,53,53,53,53,39,53,53,53,53,31,53 },
 {  0, 0, 8, 0, 0, 0, 0, 0, 8, 0, 0, 0, 0,  8, 0 },
 spell_null, TAR_IGNORE, POS_STANDING,
 &gsn_hamstring, SLOT(0), 0, 12,
 "", "You can walk normally again.", "", 0
    },

    {
 "hand to hand", 
 { 25,10,15, 6,13, 8,11,18,20,16,18,25,10,15, 6 }, 
 {  8, 5, 6, 4, 6, 5, 5, 7, 7 ,6, 6, 8, 5, 6, 4 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_hand_to_hand,  SLOT( 0), 0,  0,
  "",     "!Hand to Hand!", "", 0
    },

    {
 "herbalism",
 { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
 { -5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_herbalism,  SLOT( 0), 0,  0,
  "",     "!Herbalism!", "", 0
    },

    {
 "kcharge",
 {52,52,52,10,52,10,52,52,52,52,52,52,52,52,10},
 {-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},
    spell_null, TAR_IGNORE, POS_STANDING,
    &gsn_kcharge,     SLOT(0), 0,  24,
    "charge","","", 0
 },
    {
  "kick", 
  { 53,12,14, 8,13,10,11,32,33,30,12,53,12,14, 8 }, 
  {  0, 4, 6, 3, 5, 4, 5, 4, 6, 3, 6, 0, 4, 6, 3 },
	spell_null,             TAR_CHAR_OFFENSIVE,     POS_FIGHTING,
	&gsn_kick,              SLOT( 0),        0,     6,
	"kick",                 "!Kick!",   "", 0
    },

    {
  "kurijitsu",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_kurijitsu,	SLOT( 0),	0,	0,
  "",	"!Kurijitsu!",	"", 0
    },

    {
  "morph", 
  { 25,25,25,25,25,25,25,25,25,25,25,25,25,25,25 }, 
  { -9,-9,-8,-8,-9,-9,-8,-9,-9,-9,-9,-9,-9,-9,-9 },
	spell_null,             TAR_IGNORE,     POS_STANDING,
	&gsn_morph,              SLOT( 0),        0,     12,
	"",                 "You transform back to your original form.",   "", 0
    },

    {
  "ninjitsu",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  &gsn_ninjitsu,	SLOT( 0), 0, 0,
  "",	"!Ninjitsu!",	"", 0
     },

    {
  "parry", 
  { 22,20,13, 1,17,11, 7,21,18,12,20,22,20,13, 1 }, 
  {  8, 8, 6, 4, 7, 6, 5, 8, 7, 6, 8, 8, 8, 6, 4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_parry,             SLOT( 0),        0,     0,
	"",                     "!Parry!",    "", 0
    },

    {
  "rescue", 
  { 53,53,53, 1,53,26,26,53,53,26,53,53,53,53, 1 }, 
  {  0, 0, 0, 4, 0, 4, 4, 0, 0, 4, 0, 0, 0, 0, 4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_rescue,            SLOT( 0),        0,     12,
	"",                     "!Rescue!",   "", 0
    },

    {
  "trip", 
  { 53,53, 1,15,26,33, 8,53,26,33,53,53,53, 1,15 }, 
  {  0, 0, 4, 8, 4, 8, 6, 0, 4, 8, 0, 0, 0, 4, 8 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_trip,    SLOT( 0), 0,  24,
  "trip",     "!Trip!",   "", 0
    },


    {
  "shield bash",
  {53,53,53,18,53,29,38,53,53,38,53,53,53,53,18 },
  { 0, 0, 0, 7, 0, 9, 9, 0, 0, 9, 0, 0, 0, 0, 7 },
  spell_null,	TAR_IGNORE,	POS_FIGHTING,
  NULL,	SLOT(0), 0, 0,
  "shield bash",	"You shake off the affects of the shiled bash.", "", 0
    },

    {
  "shield block",
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
  { 6,4,6,2,5,3,4,5,6,4,5,6,4,6,2 },
  spell_null,   TAR_IGNORE,   POS_FIGHTING,
  &gsn_shield_block,  SLOT(0),  0,  0,
  "",     "!Shield!",   "", 0
    },

    {
  "scan", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
	spell_null,             TAR_IGNORE,             POS_STANDING,
	&gsn_scan,              SLOT( 0),       0,      12,
	"",                     "!Scan!", "", 0
    },

    {
    "second attack", 
    { 30,24,12, 5,17,15, 9,27,21,18,26,30,24,12, 5 }, 
    { 10, 8, 5, 3, 7, 6, 4 ,9 ,8 ,7, 9,10, 8, 5, 3 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_second_attack,     SLOT( 0),        0,     0,
	"",                     "!Second Attack!",  "", 0
    },

    {
  "third attack", 
  { 53,30,24,12,38,32,18,53,38,51,53,53,30,24,12 }, 
  {  0,-9,10, 4,10, 4, 7, 0,10, 4, 0, 0,-9,10, 4 },
	spell_null,             TAR_IGNORE,             POS_FIGHTING,
	&gsn_third_attack,      SLOT( 0),        0,     0,
	"",                     "!Third Attack!", "", 0
    },

    {
    "fourth attack",
  { 53,40,34,22,48,42,28,53,48,42,53,53,40,34,22 },
  {  0,-9,-10, -4,-10, -4, -7, 0,-10, -4, 0, 0,-9,-10, -4 },
	  spell_null,             TAR_IGNORE,             POS_FIGHTING,
		  &gsn_fourth_attack,      SLOT( 0),        0,     0,
			  "",                     "!Fourth Attack!", "", 0
			      },

/* non-combat skills */

    {
  "alchemy",
  { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
  { -5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5,-5 },
  spell_null,	TAR_IGNORE,	POS_STANDING,
  &gsn_alchemy,	SLOT( 0),	0,	0,
  "",	"",	"", 0
    },

    {
  "chee", 
  { -1,-1,-1,-1,-1,-1,-1,-1,-1, 2,-1,-1,-1,-1,-1 }, 
  {  0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0 },
  spell_null,   TAR_IGNORE,   POS_SLEEPING,
  &gsn_chi,  SLOT( 0), 0,  0,
  "",     "Chi",   "", 0
    },

    {
  "communion",
  { 53, 1,53,53,53,53,53,53,53,53,53,53, 1,53,53 },
  {  0, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0 },
  spell_null,	TAR_IGNORE,	POS_SLEEPING,
  &gsn_communion,	SLOT( 0), 0, 0,
  "",	"!Communion!",	"", 0
    },

    {
  "fade",
  {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2 },
  spell_null,	TAR_IGNORE,	POS_SLEEPING,
  &gsn_fade,	SLOT( 0),	0,	0,
	"",	"",	"", 0
    },

    { 
  "fast healing", 
  { 15, 9,16, 6,13, 8,11,12,16,11,13,15, 9,16, 6 }, 
  {  8, 5, 6, 4, 6, 5, 5, 7, 7, 6, 6, 8, 5, 6, 4 },
  spell_null,   TAR_IGNORE,   POS_SLEEPING,
  &gsn_fast_healing,  SLOT( 0), 0,  0,
  "",     "!Fast Healing!", "", 0
    },

    {
 "focus",
 { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
 { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
 spell_null,	TAR_IGNORE,	POS_FIGHTING,
 &gsn_focus,	SLOT( 0), 0, 0,
 "",	"!Focus!", "", 0
    },

    {
  "haggle", 
  { 7,18, 1,14,10,16, 8,13, 4,11,12, 7,18, 1,14 }, 
  { 5, 8, 3, 6, 6, 7, 5, 7, 4, 6, 6, 5, 8, 3, 6 },
  spell_null,   TAR_IGNORE,   POS_RESTING,
  &gsn_haggle,    SLOT( 0), 0,  0,
  "",     "!Haggle!",   "", 0
    },

    {
  "hide", 
  { 45,45, 1,12,26,32, 7,45,26,32,45,45,45, 1,12 }, 
  { -8,-8, 4, 6, 4, 6, 5,-8, 4, 6,-8,-8,-8, 4, 6 },
  spell_null,   TAR_IGNORE,   POS_RESTING,
  &gsn_hide,    SLOT( 0),  0, 12,
  "",     "!Hide!",   "", 0
    },

    {
 "holy chant",
 { 10,10,10,10,10,10,10,10,10,10,10,10,10,10,10 },
 { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1 },
 spell_null,	TAR_IGNORE,	POS_FIGHTING,
 &gsn_holy_chant,	SLOT( 0), 0, 0,
 "",	"!Holy Chant!", "", 0
    },

    {
  "infuse",
  { 10,53,53,53,53,53,53,53,53,53,53,10,53,53,53 },
  {-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},
    spell_null, TAR_IGNORE, POS_STANDING,
    &gsn_infuse,     SLOT(0), 0,  48,
    "","","", 0
    },


    {
  "lore", 
  { 10,10, 6,20, 8,15,13,10, 8,15,10,10,10, 6,20 }, 
  {  6, 6, 4, 8, 5, 7, 6, 6, 5, 7, 6, 6, 6, 4, 8 },
  spell_null,   TAR_IGNORE,   POS_RESTING,
  &gsn_lore,    SLOT( 0), 0,  36,
  "",     "!Lore!",   "", 0
    },

    {
  "meditation", 
  { 6,6,15,15,11,11,15,6,11,11,6,6,6,15,15 }, 
  { 5,5, 8, 8, 7, 7, 8,5, 7, 7,5,5,5, 8, 8 },
  spell_null,   TAR_IGNORE,   POS_SLEEPING,
  &gsn_meditation,  SLOT( 0), 0,  0,
  "",     "Meditation",   "", 0
    },

    {
  "nethermancy",
  {20,20,20,20,20,20,20,20,20,20,20,20,20,20,20 },
  {-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9,-9 },
  spell_null,	TAR_IGNORE, POS_DEAD,
  &gsn_nethermancy,	SLOT( 0),	0,	0,
  "", "", "", 0
    },

    {
  "peek", 
  { 8,21, 1,14,11,18, 8,15, 5,11,14, 8,21, 1,14 }, 
  { 5, 7,3, 6, 5, 7,5, 6,4, 6, 6,5, 7,3, 6 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_peek,    SLOT( 0),  0,  0,
  "",     "!Peek!",   "", 0
    },

    {
  "pick lock", 
  { 25,25, 7,25,16,25,16,25,16,25,25,25,25, 7,25 }, 
  {  8, 8, 4, 8, 6, 8, 6, 8, 6, 8, 8, 8, 8, 4, 8 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_pick_lock,   SLOT( 0),  0, 12,
  "",     "!Pick!",   "", 0
    },

    {
  "riding",
  { 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  {12, 9, 5, 4, 7, 4, 4,11, 7, 6,10,12, 9, 5, 4 },
  spell_null,	TAR_IGNORE,	POS_STANDING,
  &gsn_riding,	SLOT( 0),	0,	0,
  "", 	"",	"", 0
    },
    {
  "sharpen",
  {  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
  { -2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2,-2 },
  spell_null,   TAR_IGNORE,     POS_SLEEPING,
  &gsn_sharpen,    SLOT( 0),       0,      8,
        "",     "",     "", 96
    },
    {
  "slice",
  { 53,53,53,53,53,53,53,53,53,53,53,53,53,10,53 },
  {  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0 },
  spell_null,	TAR_IGNORE,	POS_STANDING,
  &gsn_slice,	SLOT( 0 ),	0, 12,
  "",	"",	"", 0
    },
    {
  "sneak", 
  { 45,45, 4,10,28,31, 6,45,28,31,45,45,45, 4, 10 }, 
  { -8,-8,4, 6, 4, 6,5,-8, 4, 6,-8, -8, -8,4,6 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_sneak,   SLOT( 0),  0, 12,
  "",     "You no longer feel stealthy.", "", 0
    },

    {
  "spellcraft",
  { 10,53,53,53,53,53,53,53,53,53,53,10,53,53,53 },
  {  5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 0, 0 },
  spell_null,	TAR_IGNORE,	POS_STANDING,
  &gsn_spellcraft,	SLOT( 0), 0, 0,
  "", "", "", 0
     },
    {
  "endow",
  { 53,10,53,53,53,53,53,10,53,53,53,53,10,53,53 },
  {-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},
    spell_null, TAR_IGNORE, POS_STANDING,
    &gsn_endow,     SLOT(0), 0,  48,
    "","","", 0
    },
    {
  "snatch",
    { 53,53,53,53,53,53,53,53,53,53,53,53,53,53,53 },
    {  -4, -4,-4,-4, -4, -4, -4, -4, -4, -4, -4, -4, -4,-4,-4 },
    spell_null,   TAR_IGNORE,   POS_STANDING,
    &gsn_snatch,   SLOT( 0),  0, 24,
    "",     "!Steal!",    "", 0
    },
    {
  "bump", 
  { 53,53,5,53,53,53,53,53,53,53,53,53,53,53,53 }, 
  {  0, 0,4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,0, 0 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_bump,   SLOT( 0),  0, 18, 
  "",     "!Bump!",    "", 0
    },
    {
  "steal", 
  { 53,53,5,53,28,53,28,53,28,53,53,53,53,5,53 }, 
  {  0, 0,4, 0, 4, 0, 4, 0, 4, 0, 0, 0, 0,4, 0 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_steal,   SLOT( 0),  0, 18, 
  "",     "!Steal!",    "", 0
    },
    {
  "scribe",
  { 10,10,53,53,53,53,53,53,53,53,53,10,53,53,53 },
  {-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4,-4},
    spell_null, TAR_IGNORE, POS_STANDING,
    &gsn_scribe,     SLOT(0), 0,  24,
    "","","", 0
    },
    {
  "scrolls", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 2,3,5,8,4,6,7,3,4,5,2,2,3,5,8 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_scrolls,   SLOT( 0), 0,  24,
  "",     "!Scrolls!",    "", 0
    },

    {
  "staves", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 2,3,5,8,4,6,7,3,4,5,2,2,3,5,8 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_staves,    SLOT( 0), 0,  12,
  "",     "!Staves!",   "", 0
    },
    
    {
  "wands", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 2,3,5,8,4,6,7,3,4,5,2,2,3,5,8 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_wands,   SLOT( 0), 0,  12,
  "",     "!Wands!",    "", 0
    },
    
    {
  "swimming", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 5,4,3,2,4,3,3,5,4,4,4,5,4,3,2 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_swim,   SLOT( 0), 0,  12,
  "",     "!Swim!",    "", 0
    },

    {
  "vorpal", 
  { 53,53,53,53,53,53,53,53,10,53,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 },
  spell_null,   TAR_IGNORE,   POS_SLEEPING,
  &gsn_vorpal,  SLOT( 0), 0,  0,
  "",     "Vorpal",   "", 0
    },

/* Traps */
    {
  "trap",   
  { 53,53,53,53,53,53,53,53,19,53,53,53,53,53 }, 
  {  0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0, 0 },
 spell_null,   TAR_IGNORE,    POS_SLEEPING,
  &gsn_trap,    SLOT( 0), 0, 0,
  "",   "A snare trap disentigrates into dust.", "", 0
    },

    {
  "recall", 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, 
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1 },
  spell_null,   TAR_IGNORE,   POS_STANDING,
  &gsn_recall,    SLOT( 0), 0,  12,
  "",     "!Recall!",   "", 0
    },

/* Trade skills */
	{
    "research",
  { 53,53,53,53,53,53,53,53,53,53,53,53,53,53,53},
  { -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
  spell_null,   TAR_IGNORE,   POS_STANDING,
  NULL,    SLOT( 0), 0,  12,
  "",     "!Research!",   "", 0
    }

};

/*END of skill table  put endifdef here*/

#ifdef WTFHERB
const struct herb_type	herb_table [] =
{
    { "mistletoe", VNUM_HERB_SILK, SECT_FIELD, FREQ_COMMON, 0 },
    { "cowslip",   VNUM_HERB_COWSLIP, SECT_FIELD, FREQ_UNCOMMON, 25 },
    { "henbane",   VNUM_HERB_HENBANE, SECT_FIELD, FREQ_RARE, 50 },
    { "sphagnum moss", VNUM_HERB_MOSS, SECT_FOREST, FREQ_COMMON, 5 },
    { "lady's mantle", VNUM_HERB_MANTLE, SECT_FOREST, FREQ_UNCOMMON, 35 },
    { "stinging nettle", VNUM_HERB_NETTLE, SECT_FOREST, FREQ_RARE, 70 },
    { "clary", VNUM_HERB_CLARY, SECT_HILLS, FREQ_COMMON, 10 },
    { "lilly of the valley", VNUM_HERB_VALLEY, SECT_HILLS, FREQ_UNCOMMON, 40 },
    { "mandrake root", VNUM_HERB_MANDRAKE, SECT_HILLS, FREQ_RARE, 90 },
    { "mountain lilly", VNUM_HERB_MOUNTAIN, SECT_MOUNTAIN, FREQ_COMMON, 0 },
    { "houseleek", VNUM_HERB_LEEK, SECT_MOUNTAIN, FREQ_UNCOMMON, 10 },
    { "flax", VNUM_HERB_FLAX, SECT_MOUNTAIN, FREQ_RARE, 20 },
    { "cactus needle", VNUM_HERB_NEEDLE, SECT_DESERT, FREQ_COMMON, 0 },
    { "josua tree root", VNUM_HERB_JOSHUA, SECT_DESERT, FREQ_UNCOMMON, 5 },
    { "nightshade", VNUM_HERB_NIGHTSHADE, SECT_DESERT, FREQ_RARE, 90 },
    { NULL, 0, 0, 0, 0 }
};
#endif

/* Index must line up with the #defines, be careful when adding more */
/* Data is verified to ensure continuous values and the proper number */
const struct hall_pricing price_table[] =
{/* Point cost (Halls), egg cost (In shards) (Personal rooms), value */
  {1000, 1, PRICE_ROOM},
  {750, 1, PRICE_R_REGEN},
  {750, 1, PRICE_R_REGEN + 1},
  {750, 1, PRICE_R_REGEN + 2},
  {750, 1, PRICE_R_REGEN + 3},
  {1250, 1, PRICE_R_REGEN + 4},
  {1250, 1, PRICE_R_REGEN + 5},
  {1250, 1, PRICE_R_REGEN + 6},
  {1250, 1, PRICE_R_REGEN + 7},
  {1250, 1, PRICE_R_REGEN + 8},
  {1250, 1, PRICE_R_REGEN + 9},
  {1250, 1, PRICE_R_REGEN + 10},
  {1250, 1, PRICE_R_REGEN + 11},
  {1250, 1, PRICE_R_REGEN + 12},
  {1250, 1, PRICE_R_REGEN_END},
  {1000, 1, PRICE_LAB},
  {2000, 1, PRICE_LAB + 1},
  {4000, 1, PRICE_LAB + 2},
  {16000, 1, PRICE_LAB + 3},
  {32000, 1, PRICE_LAB_END},
  {1000, 1, PRICE_ALTAR},
  {2000, 1, PRICE_ALTAR + 1},
  {4000, 1, PRICE_ALTAR + 2},
  {16000, 1, PRICE_ALTAR + 3},
  {32000, 1, PRICE_ALTAR_END},
  {0, 1, PRICE_ITEM},
  {2000, 1, PRICE_FURNITURE},
  {500, 1, PRICE_F_REGEN},
  {500, 1, PRICE_F_REGEN + 1},
  {500, 1, PRICE_F_REGEN + 2},
  {500, 1, PRICE_F_REGEN_END},
  {2000, 1, PRICE_FOUNTAIN},
  {2000, 1, PRICE_PIT},
  {2000, 1, PRICE_PORTAL},
  {500, 1, PRICE_DOODAD},
  {1000, 1, PRICE_DRINK},
  {0, 1, PRICE_MOB},
  {3000, 1, PRICE_HEALER},
  {2000, 1, PRICE_H_LEVEL},
  {2000, 1, PRICE_H_LEVEL + 1},
  {2000, 1, PRICE_H_LEVEL + 2},
  {2000, 1, PRICE_H_LEVEL + 3},
  {2000, 1, PRICE_H_LEVEL + 4},
  {5000, 1, PRICE_H_LEVEL + 5},
  {5000, 1, PRICE_H_LEVEL + 6},
  {5000, 1, PRICE_H_LEVEL + 7},
  {5000, 1, PRICE_H_LEVEL + 8},
  {5000, 1, PRICE_H_LEVEL + 9},
  {10000, 1, PRICE_H_LEVEL + 10},
  {10000, 1, PRICE_H_LEVEL + 11},
  {10000, 1, PRICE_H_LEVEL + 12},
  {10000, 1, PRICE_H_LEVEL + 13},
  {10000, 1, PRICE_H_LEVEL_END},
  {3000, 1, PRICE_MERCHANT},
  {3000, 1, PRICE_M_ITEM},
  {3000, 1, PRICE_M_DISCOUNT},
  {3000, 1, PRICE_M_DISCOUNT + 1},
  {3000, 1, PRICE_M_DISCOUNT + 2},
  {3000, 1, PRICE_M_DISCOUNT + 3},
  {3000, 1, PRICE_M_DISCOUNT_END},
  {0, 1, PRICE_EXIT},
  {0, 1, PRICE_E_CLOSABLE},
  {0, 1, PRICE_E_LOCKABLE},
  {0, 1, PRICE_E_NO_PICK},
  {0, 1, PRICE_E_HIDDEN},
  {100, 1, PRICE_LINK},
  {-3, -3, PRICE_TOTAL} /*-1 and -2 are errors returned from bad _STR index calls */
};

