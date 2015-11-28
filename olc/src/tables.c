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
 
static char rcsid[] = "$Id: tables.c,v 1.49 2003/10/08 00:49:28 ndagger Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#endif
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "merc.h"
#include "tables.h"

/* for clans */
const struct clan_type clan_table[] =
{
    {   "",             "",             ROOM_VNUM_ALTAR,  FALSE, TRUE, FALSE  },
    {   "loner",   "[ {DLoner {x", ROOM_VNUM_MATOOK, FALSE, TRUE, TRUE },
    {	"outcast", "[{DOutcast{x",  	ROOM_VNUM_MATOOK, FALSE, TRUE, TRUE   },
    {   "demise",  "[{DDemise {x",    ROOM_VNUM_DEMISE,   FALSE, FALSE,TRUE   },
    {   "honor",   "[ {DHonor {x",    ROOM_VNUM_HONOR,    FALSE, FALSE,TRUE   },
    {   "posse",   "[ {DPosse {x",    ROOM_VNUM_POSSE,    FALSE, FALSE,TRUE   },
    {   "avarice", "[{DAvarice{x",   ROOM_VNUM_AVARICE,  FALSE, FALSE, TRUE  },
    {   "zealot",  "[{DZealot {x",   ROOM_VNUM_ZEALOT,   FALSE, FALSE, TRUE  },
    {   "warlock", "[{DWarlock{x",   ROOM_VNUM_WARLOCK,  FALSE, FALSE, TRUE },
    {   "valor",   "[{D Valor {x",   ROOM_VNUM_VALOR,   FALSE, FALSE, TRUE },
    {   "phoenix", "[{DPhoenix{x",    ROOM_VNUM_PHOENIX,  FALSE, FALSE, TRUE },
    {   "riders",  "[{DRiders {x",   ROOM_VNUM_RIDERS,	  FALSE, FALSE, TRUE },
    {   "mcdugal", "[{DMcDugal{x",   ROOM_VNUM_RIDERS,    FALSE, FALSE, TRUE },
    {   "hunter",  "[{DHunter {x",   ROOM_VNUM_HUNTER,    FALSE, FALSE, TRUE },
    {   "temp",    "[{DTemp   {x",     ROOM_VNUM_ALTAR,	  FALSE, FALSE, TRUE },
    {   "smurf",   "[ {DSmurf {x",   ROOM_VNUM_MATOOK,  FALSE, FALSE, TRUE },
    {   "matook",  "({BMatook{x)",    ROOM_VNUM_MATOOK,  FALSE, TRUE,FALSE   },
    {   "newbie",  "({cNewbie{x)",    ROOM_VNUM_ALTAR,  FALSE, TRUE,FALSE   },
    {   "circle",  "({DCircle{x)",    ROOM_VNUM_ALTAR,  FALSE, TRUE,FALSE   },
    {   "camorra",  "({DCamorra{x)",    ROOM_VNUM_ALTAR,  FALSE, TRUE,FALSE   },
    {   "legion",  "({DLegion{x)",    ROOM_VNUM_ALTAR,  FALSE, TRUE,FALSE   },
    {   "macleod",  "({DMacLeod{x)",    ROOM_VNUM_ALTAR,  FALSE, TRUE,FALSE   }
};

/* for assigning IMM commands */
const struct imm_command_type imm_command_table[] =
{
    { "sockets",	ICG_JUDGE,	A },
    { "violate",	ICG_ADMIN,	B },
    { "allow",		ICG_JUDGE,	C },
    { "ban",		ICG_JUDGE,	D },
    { "deny",		ICG_JUDGE,	E },
    { "bflag",		ICG_ADMIN,	F },
    { "disconnect",	ICG_JUDGE,	G },
    { "dweeb",		ICG_JUDGE,	H },
    { "flag",		ICG_QUEST,	I },
    { "freeze",		ICG_JUDGE,	J },
    { "fuck",		ICG_JUDGE,	K },
    { "ident",		ICG_JUDGE,	L },
    { "permban",	ICG_JUDGE,	M },
    { "set",		ICG_QUEST,	N },
    { "wizlock",	ICG_ADMIN,	O },
    { "load",		ICG_QUEST,	P },
    { "newlock",	ICG_JUDGE,	Q },
    { "pardon",		ICG_JUDGE,	S },
    { "restore",	ICG_QUEST,	T },
    { "slay",		ICG_JUDGE,	U },
    { "gecho",		ICG_ADMIN,	V },
    { "log",		ICG_JUDGE,	W },
    { "peace",		ICG_QUEST,	X },
    { "return",		ICG_QUEST,	Y },
    { "snoop",		ICG_JUDGE,	Z },
    { "switch",		ICG_QUEST,	Y },
    { "clone",		ICG_QUEST,	P }

};


/* for deities Remove and add to const.c for comm.c 
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
	ALIGN_NONE,	TRUE	}
};
*/

/* don't forget to give these a function in give_gift() deity.c */
const struct gift_type gift_table[] =
{
    {	"recall",	75	},
    {	"random",	50	},
    {	"patience",	100	},
    {	"nurture",	100	},
    {	"meld",		100	},
    {	"reanimation",	200	},
    {	"opiate",	50	},
    {	"bravery",	100	},
    {	"stature",	100	},
    {	"distraction",	100	},
    {	"fear",		150	},
    {	"speed",	200	},
    {	"transport",	150	},
    {   "knowledge",    25      },
    {   "banishment",   200     },
    {	"",		0	}
};

/* for position */
const struct position_type position_table[] =
{
    {   "dead",                 "dead"  },
    {   "mortally wounded",     "mort"  },
    {   "incapacitated",        "incap" },
    {   "stunned",              "stun"  },
    {   "sleeping",             "sleep" },
    {   "resting",              "rest"  },
    {   "sitting",              "sit"   },
    {   "fighting",             "fight" },
    {   "standing",             "stand" },
    {   NULL,                   NULL    }
};

/* for sex */
const struct sex_type sex_table[] =
{
   {    "none"          },
   {    "male"          },
   {    "female"        },
   {    "either"        },
   {    NULL            }
};

/* for sizes */
const struct size_type size_table[] =
{ 
    {   "tiny"          },
    {   "small"         },
    {   "medium"        },
    {   "large"         },
    {   "huge"          },
    {   "giant"         },
    {   NULL            }
};

/* for obj sizes */
const struct obj_size_type obj_size_table[] =
{ 
    {   "one size fits all"  },
    {   "tiny"          },
    {   "small"         },
    {   "medium"        },
    {   "large"         },
    {   "huge"          },
    {   "giant"         },
    {   NULL            }
};

/* various flag tables */
const struct flag_type mhs_flags[] =
{
    {   "old-reclass",          A,      TRUE    },
    {   "mutant",               B,      TRUE    },
    {   "highlander",           C,      TRUE    },
    {   "savant",               D,      TRUE    },
    {   "shapeshifter",         E,      TRUE    },
    {   "shapemorph",           F,      TRUE    },
    {   "norescue",             G,      TRUE    },
    {   NULL,                   0,      0       }
};

const struct flag_type act_flags[] =
{
    {   "npc",                  A,      FALSE   },
    {   "sentinel",             B,      TRUE    },
    {   "scavenger",            C,      TRUE    },
    {   "aggressive",           F,      TRUE    },
    {   "stay_area",            G,      TRUE    },
    {   "wimpy",                H,      TRUE    },
    {   "pet",                  I,      TRUE    },
    {   "train",                J,      TRUE    },
    {   "practice",             K,      TRUE    },
    {   "undead",               O,      TRUE    },
    {   "weaponsmith",          P,      TRUE    },
    {   "cleric",               Q,      TRUE    },
    {   "mage",                 R,      TRUE    },
    {   "thief",                S,      TRUE    },
    {   "warrior",              T,      TRUE    },
    {   "noalign",              U,      TRUE    },
    {   "nopurge",              V,      TRUE    },
    {   "outdoors",             W,      TRUE    },
    {   "armourer",             X,      TRUE    },
    {   "indoors",              Y,      TRUE    },
    {	"mount",		Z,	TRUE	},
    {   "healer",               aa,     TRUE    },
    {   "gain",                 bb,     TRUE    },
    {   "update_always",        cc,     TRUE    },
    {   "changer",              dd,     TRUE    },
    {   "notrans",		ee,	TRUE	}, 
    {   NULL,                   0,      FALSE   }
};

const struct flag_type plr_flags[] =
{
    {   "npc",                  A,      FALSE   },
    {   "autoassist",           C,      FALSE   },
    {   "autoexit",             D,      FALSE   },
    {   "autoloot",             E,      FALSE   },
    {   "autosac",              F,      FALSE   },
    {   "autogold",             G,      FALSE   },
    {   "autosplit",            H,      FALSE   },
    {   "honor",                I,      FALSE   },
    {   "circle",               J,      FALSE   },
    {   "demise",               K,      FALSE   },
    {   "balance",              L,      FALSE   },
    {   "loner",                M,      FALSE   },
    {   "holylight",            N,      FALSE   },
    {   "can_loot",             P,      FALSE   },
    {   "nosummon",             Q,      FALSE   },
    {   "nofollow",             R,      FALSE   },
    {   "permit",               U,      TRUE    },
    {   "dweeb",                V,      FALSE   },
    {   "log",                  W,      FALSE   },
    {   "deny",                 X,      FALSE   },
    {   "freeze",               Y,      FALSE   },
    {   "thief",                Z,      FALSE   },
    {   "killer",               aa,     FALSE   },
    {   "thug",                 bb,     FALSE   },
    {   NULL,                   0,      0       }
};

const struct flag_type affect_flags[] =
{
    {   "blind",                A,      TRUE    },
    {   "invisible",            B,      TRUE    },
    {   "detect_evil",          C,      TRUE    },
    {   "detect_invis",         D,      TRUE    },
    {   "detect_magic",         E,      TRUE    },
    {   "detect_hidden",        F,      TRUE    },
    {   "detect_good",          G,      TRUE    },
    {   "sanctuary",            H,      TRUE    },
    {   "faerie_fire",          I,      TRUE    },
    {   "infrared",             J,      TRUE    },
    {   "curse",                K,      TRUE    },
    {   "poison",               M,      TRUE    },
    {   "protect_evil",         N,      TRUE    },
    {   "protect_good",         O,      TRUE    },
    {   "sneak",                P,      TRUE    },
    {   "hide",                 Q,      TRUE    },
    {   "sleep",                R,      TRUE    },
    {   "charm",                S,      TRUE    },
    {   "flying",               T,      TRUE    },
    {   "pass_door",            U,      TRUE    },
    {   "haste",                V,      TRUE    },
    {   "calm",                 W,      TRUE    },
    {   "plague",               X,      TRUE    },
    {   "weaken",               Y,      TRUE    },
    {   "dark_vision",          Z,      TRUE    },
    {   "berserk",              aa,     TRUE    },
    {   "swim",                 bb,     TRUE    },
    {   "regeneration",         cc,     TRUE    },
    {   "slow",                 dd,     TRUE    },
    {   NULL,                   0,      0       }
};

const struct flag_type off_flags[] =
{
    {   "area_attack",          A,      TRUE    },
    {   "backstab",             B,      TRUE    },
    {   "bash",                 C,      TRUE    },
    {   "berserk",              D,      TRUE    },
    {   "disarm",               E,      TRUE    },
    {   "dodge",                F,      TRUE    },
    {   "fade",                 G,      TRUE    },
    {   "fast",                 H,      TRUE    },
    {   "kick",                 I,      TRUE    },
    {   "dirt_kick",            J,      TRUE    },
    {   "parry",                K,      TRUE    },
    {   "rescue",               L,      TRUE    },
    {   "tail",                 M,      TRUE    },
    {   "trip",                 N,      TRUE    },
    {   "crush",                O,      TRUE    },
    {   "assist_all",           P,      TRUE    },
    {   "assist_align",         Q,      TRUE    },
    {   "assist_race",          R,      TRUE    },
    {   "assist_players",       S,      TRUE    },
    {   "assist_guard",         T,      TRUE    },
    {   "assist_vnum",          U,      TRUE    },
    {   NULL,                   0,      0       }
};

const struct flag_type imm_flags[] =
{
    {   "summon",               A,      TRUE    },
    {   "charm",                B,      TRUE    },
    {   "magic",                C,      TRUE    },
    {   "weapon",               D,      TRUE    },
    {   "bash",                 E,      TRUE    },
    {   "pierce",               F,      TRUE    },
    {   "slash",                G,      TRUE    },
    {   "fire",                 H,      TRUE    },
    {   "cold",                 I,      TRUE    },
    {   "lightning",            J,      TRUE    },
    {   "acid",                 K,      TRUE    },
    {   "poison",               L,      TRUE    },
    {   "negative",             M,      TRUE    },
    {   "holy",                 N,      TRUE    },
    {   "energy",               O,      TRUE    },
    {   "mental",               P,      TRUE    },
    {   "disease",              Q,      TRUE    },
    {   "drowning",             R,      TRUE    },
    {   "light",                S,      TRUE    },
    {   "sound",                T,      TRUE    },
    {   "wood",                 X,      TRUE    },
    {   "silver",               Y,      TRUE    },
    {   "iron",                 Z,      TRUE    },
    {   NULL,                   0,      0       }
};

const struct flag_type form_flags[] =
{
    {   "edible",               FORM_EDIBLE,            TRUE    },
    {   "poison",               FORM_POISON,            TRUE    },
    {   "magical",              FORM_MAGICAL,           TRUE    },
    {   "instant_decay",        FORM_INSTANT_DECAY,     TRUE    },
    {   "other",                FORM_OTHER,             TRUE    },
    {   "animal",               FORM_ANIMAL,            TRUE    },
    {   "sentient",             FORM_SENTIENT,          TRUE    },
    {   "undead",               FORM_UNDEAD,            TRUE    },
    {   "construct",            FORM_CONSTRUCT,         TRUE    },
    {   "mist",                 FORM_MIST,              TRUE    },
    {   "intangible",           FORM_INTANGIBLE,        TRUE    },
    {   "biped",                FORM_BIPED,             TRUE    },
    {   "centaur",              FORM_CENTAUR,           TRUE    },
    {   "insect",               FORM_INSECT,            TRUE    },
    {   "spider",               FORM_SPIDER,            TRUE    },
    {   "crustacean",           FORM_CRUSTACEAN,        TRUE    },
    {   "worm",                 FORM_WORM,              TRUE    },
    {   "blob",                 FORM_BLOB,              TRUE    },
    {   "mammal",               FORM_MAMMAL,            TRUE    },
    {   "bird",                 FORM_BIRD,              TRUE    },
    {   "reptile",              FORM_REPTILE,           TRUE    },
    {   "snake",                FORM_SNAKE,             TRUE    },
    {   "dragon",               FORM_DRAGON,            TRUE    },
    {   "amphibian",            FORM_AMPHIBIAN,         TRUE    },
    {   "fish",                 FORM_FISH ,             TRUE    },
    {   "cold_blood",           FORM_COLD_BLOOD,        TRUE    },
    {   NULL,                   0,                      0       }
};

const struct flag_type part_flags[] =
{
    {   "head",                 PART_HEAD,              TRUE    },
    {   "arms",                 PART_ARMS,              TRUE    },
    {   "legs",                 PART_LEGS,              TRUE    },
    {   "heart",                PART_HEART,             TRUE    },
    {   "brains",               PART_BRAINS,            TRUE    },
    {   "guts",                 PART_GUTS,              TRUE    },
    {   "hands",                PART_HANDS,             TRUE    },
    {   "feet",                 PART_FEET,              TRUE    },
    {   "fingers",              PART_FINGERS,           TRUE    },
    {   "ear",                  PART_EAR,               TRUE    },
    {   "eye",                  PART_EYE,               TRUE    },
    {   "long_tongue",          PART_LONG_TONGUE,       TRUE    },
    {   "eyestalks",            PART_EYESTALKS,         TRUE    },
    {   "tentacles",            PART_TENTACLES,         TRUE    },
    {   "fins",                 PART_FINS,              TRUE    },
    {   "wings",                PART_WINGS,             TRUE    },
    {   "tail",                 PART_TAIL,              TRUE    },
    {   "claws",                PART_CLAWS,             TRUE    },
    {   "fangs",                PART_FANGS,             TRUE    },
    {   "horns",                PART_HORNS,             TRUE    },
    {   "scales",               PART_SCALES,            TRUE    },
    {   "tusks",                PART_TUSKS,             TRUE    },
    {   NULL,                   0,                      0       }
};

const struct flag_type disp_flags[]=
{
    {   "compact",		DISP_COMPACT,		TRUE	},
    {   "brief_desc",		DISP_BRIEF_DESCR,	TRUE	},
    {   "brief_combat",		DISP_BRIEF_COMBAT,	TRUE	},
    {	"brief_wholist",	DISP_BRIEF_WHOLIST,	TRUE	},
    {	"brief_eqlist",		DISP_BRIEF_EQLIST,	TRUE	},
    {	"prompt",		DISP_PROMPT,		TRUE	},
    {	"show_affected",	DISP_SHOW_AFFECTS,	TRUE	},  
    {	"disp_vnum",		DISP_DISP_VNUM,		TRUE	},
    {   "color",		DISP_COLOR,		TRUE	},
    {   "combine",		DISP_COMBINE,		TRUE    },
    {   "scan",                 DISP_BRIEF_SCAN,        TRUE    },
    {	NULL,			0,			0	}
};

const struct flag_type comm_flags[] =
{
    {   "quiet",                COMM_QUIET,             TRUE    },
    {   "deaf",                 COMM_DEAF,              TRUE    },
    {   "nowiz",                COMM_NOWIZ,             TRUE    },
    {   "nobitch",              COMM_NOBITCH,           TRUE    },
    {   "noclangossip",         COMM_NOAUCTION,         TRUE    },
    {   "nogossip",             COMM_NOGOSSIP,          TRUE    },
    {	"noooc",		COMM_NOOOC,		TRUE	},
    {   "noquestion",           COMM_NOQUESTION,        TRUE    },
    {   "nomusic",              COMM_NOMUSIC,           TRUE    },
    {   "noclan",               COMM_NOCLAN,            TRUE    },
    {   "noquest",              COMM_NOQUOTE,           TRUE    },
    /**
    {   "shoutsoff",            COMM_SHOUTSOFF,         TRUE    },
     **/
    {   "true_trust",           COMM_TRUE_TRUST,        TRUE    },
    {   "telnet_ga",            COMM_TELNET_GA,         TRUE    },
    {   "nograts",              COMM_NOGRATS,           TRUE    },
    {   "noemote",              COMM_NOEMOTE,           FALSE   },
    {   "noshout",              COMM_NOSHOUT,           FALSE   },
    {   "notell",               COMM_NOTELL,            FALSE   },
    {   "nochannels",           COMM_NOCHANNELS,        FALSE   },
    {   "nonotes",		COMM_NONOTES,		FALSE   },
    {   "snoop_proof",          COMM_SNOOP_PROOF,       FALSE   },
    {   "afk",                  COMM_AFK,               TRUE    },
    {   NULL,                   0,                      0       }
};















