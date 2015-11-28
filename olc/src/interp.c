/***************************************************************************
 *  Or iginal Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
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

static char rcsid[] = "$Id: interp.c,v 1.198 2004/04/25 00:32:05 boogums Exp $";
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
#include "interp.h"
#include "lookup.h"
#include "tables.h"
/*
 #include "imc-mercbase.h"
 #include "icec-mercbase.h"
 */


bool  check_social  args( ( CHAR_DATA *ch, char *command,
          char *argument ) );



/*
 * Command logging types.
 */
#define LOG_NORMAL  0
#define LOG_ALWAYS  1
#define LOG_NEVER   2



/*
 * Log-all switch.
 */
bool        fLogAll   = FALSE;



/*
 * Command table.
 */
const struct  cmd_type  cmd_table [] =
{
    /*
     * Common movement commands.
     */
    { "north",    do_north, POS_STANDING,    0,  LOG_NEVER, 0, 1 },
    { "east",   do_east,  POS_STANDING,  0,  LOG_NEVER, 0, 1 },
    { "south",    do_south, POS_STANDING,  0,  LOG_NEVER, 0, 1 },
    { "west",   do_west,  POS_STANDING,  0,  LOG_NEVER, 0, 1 },
    { "up",   do_up,    POS_STANDING,  0,  LOG_NEVER, 0, 1 },
    { "down",   do_down,  POS_STANDING,  0,  LOG_NEVER, 0, 1 },

    /*
     * Common other commands.
     * Placed here so one and two letter abbreviations work.
     */
    { "at",             do_at,          POS_DEAD,       L6,  LOG_NORMAL, 1, 0 },
    { "cast",   do_cast,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "chant", do_chant,	POS_FIGHTING, 0, LOG_NORMAL, 1, 1 },
    { "cgossip",        do_auction,     POS_SLEEPING,    1,  LOG_NORMAL, 1, 1 },
    { "create",		do_create,	POS_STANDING,	 1,  LOG_NORMAL,	1,1},
    { "buy",    do_buy,   POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "bounty",	do_bounty,	POS_STANDING,	1,	LOG_NORMAL, 1, 1 },
    { "channels",       do_channels,    POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "exits",    do_exits, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "focus", do_focus,	POS_FIGHTING, 0, LOG_NORMAL, 1, 1 },
    { "get",    do_get,   POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "goto",           do_goto,        POS_DEAD,       L8,  LOG_NORMAL, 1, 0 },
    { "group",          do_group,       POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "gprompt",          do_gprompt,       POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "grenade",	do_grenade,	POS_FIGHTING,	 0,	LOG_NORMAL, 1, 1 },
    { "guild",    do_guild, POS_DEAD, 25,  LOG_ALWAYS, 1, 0 },
    { "hit",    do_kill,  POS_FIGHTING,  0,  LOG_NORMAL, 0, 1 },
    { "hamstring", do_hamstring,POS_STANDING,0,LOG_NORMAL, 1, 1 },   
    { "highlander",    do_highlander, POS_DEAD, 0,  LOG_NORMAL, 1, 0 },
    { "inventory",  do_inventory, POS_RESTING,  0,  LOG_NORMAL, 1, 1 },
    { "look",   do_look,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "loot",    do_loot,   POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
    { "level",    do_level,	POS_SLEEPING, 0, LOG_NORMAL, 1, 0 },
    { "clan",   do_clantalk,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "clist",   do_clist,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "chelp",   do_chelp,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "cinfo",   do_cinfo,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "cmembers",   do_cmembers,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "callies",   do_callies,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "mount",	do_mount,	POS_STANDING, 0, LOG_NORMAL, 1, 1 },
    { "movehall",do_movehall,	POS_STANDING, L3, LOG_NORMAL, 1, 1 },
    { "music",          do_music,     POS_SLEEPING,    1,  LOG_NORMAL, 1, 1 }, 
    { "order",    do_order, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "offer",    do_offer, POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
    { "ooc",	do_ooc,	POS_SLEEPING,    0,  LOG_NORMAL, 1, 1 },
    { "practice",       do_practice,  POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "rest",   do_rest,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "sit",    do_sit,   POS_SLEEPING,    0,  LOG_NORMAL, 1, 1 },
    { "stand",    do_stand, POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
/*  { "spy",	do_spy,	POS_RESTING, 0, LOG_NORMAL, 1, 1 }, */
    { "surname",	do_surname,POS_SLEEPING,0,LOG_ALWAYS,1,0 },
    { "tell",   do_tell,  POS_SLEEPING,   0,  LOG_NORMAL, 1, 1 },
    { "unlock",         do_unlock,      POS_RESTING,     0,  LOG_NORMAL, 1, 2 },
    { "unwraith", do_wraithform_return, POS_STANDING, 0, LOG_NORMAL, 1, 0 },
    { "wield",    do_wear,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "wizhelp",  do_wizhelp, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },

    /*
     * Informational commands.
     */
    { "affects",  do_affects, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "abolish",  do_abolish, POS_STANDING, 0, LOG_NORMAL, 1, 1 },
    { "areas",    do_areas, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "attributes", do_attributes, POS_SLEEPING,0,LOG_NORMAL,1,1 }, 
    { "attack", do_attack, POS_FIGHTING, 0, LOG_NORMAL, 1, 1 }, 
    { "bug",	do_bug,	POS_DEAD,	0,	LOG_NORMAL, 1, 0 },
    { "changes",  do_changes, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "color",  do_color, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "commands", do_commands,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "compare",  do_compare, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "consider", do_consider,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "count",    do_count, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "credits",  do_credits, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "date",   do_time,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
//    { "enemy",    do_enemy, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "donate",  do_donate, POS_STANDING,  0,  LOG_NORMAL, 1, 1 },
    { "equipment",  do_equipment, POS_RESTING,  0,  LOG_NORMAL, 1, 1 },
    { "establish",  do_establish, POS_STANDING,  0,  LOG_NORMAL, 1, 1 },
    { "cstat",   do_cstat,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "examine",  do_examine, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "glance",   do_glance,  POS_RESTING,  0,  LOG_NORMAL, 1, 1 },
    { "hd",    do_hd, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "help",   do_help,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "hedit",   do_hedit,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "hostmask",   do_hostmask,  POS_DEAD,  L1,  LOG_NEVER, 1, 0 },
    { "ignore",   do_ignore,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "idea",   do_idea,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "info",   do_groups,      POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "motd",   do_motd,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "name",	do_name,  POS_DEAD,	L5, LOG_NORMAL, 1, 0},
    { "news",   do_news,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "newbie",   do_newbie,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "onote",   do_onote,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "peek",   do_peek,  POS_RESTING,  0,  LOG_NORMAL, 1, 1 },
    { "pedit",   do_pedit,  POS_RESTING,  0,  LOG_NORMAL, 1, 1 },
    { "read",   do_read,  POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "report",   do_report,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "repair",   do_repair,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "rules",    do_rules, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "rank",	do_rank,	POS_DEAD, 25,	LOG_NORMAL, 1, 0 },
    { "join",	do_join,	POS_DEAD, 5,	LOG_NORMAL, 1, 0 },
    { "outcas", do_outcas,	POS_DEAD, 5,	LOG_NORMAL, 0, 0 },
    { "outcast", do_outcast,	POS_DEAD, 5,	LOG_ALWAYS, 1, 0 },
    { "score",    do_score, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "cscore",    do_cscore, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "skills",   do_skills,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "socials",  do_socials, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "show",   do_show,  POS_DEAD,  0,  LOG_NORMAL, 1, 1 },
    { "spells",	do_spells,	POS_DEAD,	0, LOG_NORMAL, 1, 0 },
    { "specializ", do_specializ, POS_DEAD, 0, LOG_NORMAL, 0, 0 }, 
    { "specialize", do_specialize, POS_DEAD, 0, LOG_NORMAL, 1, 0 },
    { "species",	do_species, POS_DEAD, 0, LOG_NORMAL, 1, 0 },
    { "story",    do_story, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "time",   do_time,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "typo",   do_typo,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "weather",  do_weather, POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "who",    do_who,   POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "whois",    do_whois, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "wizlist",  do_wizlist, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "worth",    do_worth, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
 
    { "deposit", do_deposit, POS_RESTING, 0, LOG_NORMAL, 1, 0 },
    { "withdraw", do_withdraw, POS_RESTING, 0, LOG_NORMAL, 1, 0 }, 
  
  /*
     * Configuration commands.
     */
    
    { "alia",   do_alia,  POS_DEAD,  0,  LOG_NORMAL, 0, 0 },
    { "alias",    do_alias, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "ally",   do_ally,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "autolist", do_autolist,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "autoassist", do_autoassist,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autoexit", do_autoexit,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autogold", do_autogold,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autoloot", do_autoloot,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autopeek", do_autopeek,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autosac",  do_autosac, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "autosplit",  do_autosplit, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "beeptell",  do_beeptell, POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },    
    { "brief",    do_brief, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "combine",  do_combine, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "compact",  do_compact, POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "description",  do_description, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "delet",    do_delet, POS_DEAD,  0,  LOG_ALWAYS, 0, 0 },
    { "delete",   do_delete,  POS_STANDING,  0,  LOG_ALWAYS, 1, 0 },    
/*  { "display", do_display, POS_SLEEPING,	0, LOG_NORMAL, 1, 0 }, */
    { "fullsac", do_full_sac,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "gladiator", do_gladiator, POS_STANDING, 0, LOG_NORMAL, 1, 0 }, 
    { "gbet",    do_gbet,    POS_SLEEPING,     0, LOG_NORMAL, 1, 0},
    { "gscore",    do_gscore,    POS_SLEEPING,     0, LOG_NORMAL, 1, 0},
    { "gtell",    do_gtell, POS_DEAD,  0,  LOG_NORMAL, 1, 1 },
    { "gtscore",    do_gtscore,    POS_SLEEPING,     0, LOG_NORMAL, 1, 0},
    { "gstatus",   do_gstatus,    POS_SLEEPING,     0, LOG_NORMAL, 1, 0},
    { "longeq", do_longeq,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "loner",  do_loner,      POS_DEAD,    5,  LOG_ALWAYS, 1, 0 },
    { "nocancel", do_nocancel,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "nooutofrange", do_nooutofrange,  POS_DEAD, 0,  LOG_NORMAL, 1, 0 },
    { "norecall", do_norecall,	POS_DEAD,	0, LOG_NORMAL, 1, 0 },
    { "norescue", do_norescue,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "nofollow", do_nofollow,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "nowake", do_nowake,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "noloot",   do_noloot,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "nosummon", do_nosummon,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "nogladiator", do_nogladiator,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "odds", do_odds,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "outfit",   do_outfit,  POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "password", do_password,  POS_DEAD,  0,  LOG_NEVER,  1, 0 },
    { "prompt",   do_prompt,  POS_DEAD,        0,  LOG_NORMAL, 1, 0 },
    { "scroll",   do_scroll,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "unmacro",        do_unmacro,     POS_DEAD, L7,  LOG_NORMAL, 1, 0 },
    { "title",    do_title, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "unalias",  do_unalias, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "wimpy",    do_wimpy, POS_DEAD,  0,  LOG_NORMAL, 1, 0 },

    /*
     * Communication commands.
     */
    { "afk",    do_afk,   POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "answer",   do_answer,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "deaf",   do_deaf,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "emote",    do_emote, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "pmote",    do_pmote, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
  /*  { "bitch",  do_bitch, POS_RESTING, 1, LOG_NORMAL, 1, 1}, */
    { ".",    do_gossip,  POS_SLEEPING,  1,  LOG_NORMAL, 0, 1 },
    { "]",	do_ooc,	POS_SLEEPING,    1,  LOG_NORMAL, 1, 1 },
    { "gossip",   do_gossip,  POS_SLEEPING,  1,  LOG_NORMAL, 1, 1 },
    { ",",    do_emote, POS_RESTING,   0,  LOG_NORMAL, 0, 1 },
    { "grats",    do_grats, POS_SLEEPING,  1,  LOG_NORMAL, 1, 1 },
    { ";",    do_gtell, POS_DEAD,  0,  LOG_NORMAL, 0, 1 },
    { "inote",   do_note,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "cnote",  do_cnotes, POS_SLEEPING, 0, LOG_NORMAL, 1, 0 },
    { "pose",   do_pose,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "?"  , do_question,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "quest",    do_quest, POS_SLEEPING,  1,  LOG_NORMAL, 1, 1 },
    { "quiet",    do_quiet, POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "qnote",  do_qnotes, POS_SLEEPING, 0, LOG_NORMAL, 1, 0 },
    { "reply",    do_reply, POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "replay",   do_replay,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "rlock",   do_rlock,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "whisper",    do_whisper,   POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "say",    do_say,   POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "sayto",    do_sayto,   POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "text",   do_text,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 0 },
    { "'",    do_say,   POS_RESTING,   0,  LOG_NORMAL, 0, 1 },


/*    { "shout",    do_shout, POS_RESTING,   3,  LOG_NORMAL, 1, 1 }, */
    { "silence", do_silence, POS_SLEEPING, 0, LOG_NORMAL, 1, 0 },  
    { "unread",   do_unread,  POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "spool",   do_spool,  POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "yell",   do_yell,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },

    /*
     * Object manipulation commands.
     */
    { "assemble", do_assemble, POS_STANDING, 0, LOG_NORMAL, 1,0 },
    { "brandish", do_brandish,  POS_FIGHTING,   0,  LOG_NORMAL, 1, 2 },
    { "close",    do_close, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "drink",    do_drink, POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "drop",   do_drop,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "drag",    do_drag, POS_RESTING,   0,  LOG_ALWAYS, 1, 2 },
    { "eat",    do_eat,   POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "endow", do_endow, POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
    { "envenom",  do_envenom, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "fill",   do_fill,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "forge",   do_forge,  POS_STANDING,   0,  LOG_NORMAL, 1, 0 },
    { "getvoucher", do_get_voucher, POS_STANDING, 0, LOG_NORMAL, 1, 0 },
    { "give",   do_give,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "glick",  do_lick,  POS_RESTING,	 0,  LOG_NORMAL, 1, 1 }, 
    { "heal",   do_heal,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 }, 
    { "hold",   do_wear,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "infuse", do_infuse, POS_STANDING, 0,  LOG_NORMAL, 1, 1 },
    { "list",   do_list,  POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "lock",   do_lock,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "morph",   do_morph,  POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
    { "open",   do_open,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "pick",   do_pick,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "pour",   do_pour,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "put",    do_put,   POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "quaff",    do_quaff, POS_FIGHTING,   0,  LOG_NORMAL, 1, 2 },
    { "recite",   do_recite,  POS_FIGHTING,   0,  LOG_NORMAL, 1, 2 },
    { "remove",   do_remove,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "search",   do_search,  POS_STANDING,   0,  LOG_NORMAL, 1, 0 },
    { "sell",   do_sell,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "take",   do_get,   POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "sacrifice",  do_sacrifice, POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "scribe", do_scribe, POS_STANDING,    0,  LOG_NORMAL, 1, 0},
//    { "shapeshift",   do_shapeshift,  POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
//    { "shapemorph",   do_shapemorph,  POS_STANDING,   0,  LOG_ALWAYS, 1, 1 },
    { "junk",           do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 0, 0 },
    { "tap",        do_sacrifice,   POS_RESTING,     0,  LOG_NORMAL, 0, 0 },   
    { "value",    do_value, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "wear",   do_wear,  POS_RESTING,   0,  LOG_NORMAL, 1, 2 },
    { "zap",    do_zap,   POS_FIGHTING,   0,  LOG_NORMAL, 1, 2 },

    /*
     * Combat commands.
     */
    { "backstab", do_backstab,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "bash",   do_bash,  POS_FIGHTING,    0,  LOG_NORMAL, 1, 1 },
    { "bs",   do_backstab,  POS_FIGHTING,  0,  LOG_NORMAL, 0, 1 },
    { "berserk",  do_berserk, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "bite",  do_bite, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "bleed",  do_bleed, POS_FIGHTING,  0,  LOG_NORMAL, 1, 0 },
    { "bladesong", do_bladesong, POS_FIGHTING, 0, LOG_NORMAL, 1, 1 },
    { "breathe",  do_breathe, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "dbite", do_dbite, POS_FIGHTING, 0, LOG_NORMAL, 0, 1 },  
    { "daetok", do_dae_tok, POS_STANDING, 0, LOG_NORMAL, 0, 1 },
    { "dbite",	do_dbite, POS_FIGHTING,  0, LOG_NORMAL, 0, 1 }, 
    { "dirt",   do_dirt,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "disarm",   do_disarm,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "fear",  do_fear, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "flee",   do_flee,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "grab",   do_grab,  POS_FIGHTING,    0,  LOG_NORMAL, 1, 1 },
    { "garotte",   do_garotte,  POS_STANDING,  0,  LOG_NORMAL, 1, 1 },
    { "hex",  do_hex, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "insanity",   do_insanity,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "kick",   do_kick,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
/* 2 Lines below added 03SEP00 by Boogums */
    { "kcharge",   do_kcharge, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "kc",   do_kcharge, POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "knock", do_knock, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "kill",   do_kill,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "murde",    do_murde, POS_FIGHTING,  0,  LOG_NORMAL, 0, 0 },
    { "murder",   do_murder,  POS_FIGHTING,  1,  LOG_NORMAL, 1, 1 },
    { "rage",	do_rage,	POS_FIGHTING,	0, LOG_NORMAL, 1, 1 },
    { "rescue",   do_rescue,  POS_FIGHTING,  0,  LOG_NORMAL, 0, 1 },
    { "throw",   do_throw,  POS_FIGHTING,    0,  LOG_NORMAL, 1, 1 },
    { "trip",   do_trip,  POS_FIGHTING,    0,  LOG_NORMAL, 1, 1 },
    { "trap",	do_trap,  POS_STANDING,	0,	LOG_NORMAL, 1, 1 },
    { "tail", do_tail_slap, POS_FIGHTING, 0, LOG_NORMAL, 1, 1 }, 
    /*
     * Miscellaneous commands.
     */
    { "brew",	do_brew,	POS_STANDING,	0,	LOG_NORMAL, 1, 1 },
    { "die",   do_die,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "dismount", do_dismount, POS_STANDING, 0, LOG_NORMAL, 1, 0 }, 
    { "display", do_display, POS_SLEEPING,      0, LOG_NORMAL, 1, 0 }, 
    { "enter",    do_enter,   POS_STANDING,  0,  LOG_NORMAL, 1, 1 },
    { "fade",	do_fade,	POS_RESTING, 0, LOG_NORMAL, 0, 1 },
    { "follow",   do_follow,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "gain",   do_gain,  POS_STANDING,  0,  LOG_NORMAL, 1, 0 },
    { "groups",   do_groups,  POS_SLEEPING,    0,  LOG_NORMAL, 1, 0 },
    { "hide",   do_hide,  POS_STANDING,   0,  LOG_NORMAL, 1, 1 },
    { "kr",   do_kr,      POS_RESTING,    5,  LOG_NORMAL, 1, 0 },
    { "kit",	do_kit,	POS_SLEEPING,	 1,	LOG_ALWAYS, 1, 0 },
    { "link",   do_link,  POS_STANDING,   0,  LOG_NORMAL, 1, 0 },
    { "linksafe",   do_linksafe,  POS_STANDING,   0,  LOG_NORMAL, 1, 0 },
    { "unlink",   do_unlink,  POS_STANDING,   0,  LOG_NORMAL, 1, 0 },
    { "pray",   do_pray,  POS_FIGHTING,   0,  LOG_NORMAL, 1, 0 },
    { "pledg",  do_pledg, POS_DEAD,    0, LOG_NORMAL, 1, 0},
    { "pledge",   do_pledge,  POS_DEAD,   0,  LOG_NORMAL, 1, 0 },
    { "qui",    do_qui,   POS_DEAD,  0,  LOG_NORMAL, 0, 0 },
    { "quit",   do_quit_command,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "quicken",   do_quicken,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "roar",   do_roar,  POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "recall",   do_recall,  POS_FIGHTING,  0,  LOG_NORMAL, 1, 1 },
    { "/",    do_recall,  POS_FIGHTING,  0,  LOG_NORMAL, 0, 1 },
    { "rent",   do_rent,  POS_DEAD,  0,  LOG_NORMAL, 0, 1 },
    { "ritual", do_ritual, POS_STANDING, 0, LOG_NORMAL, 1, 0 },
    { "save",   do_save,  POS_DEAD,  0,  LOG_NORMAL, 1, 0 },
    { "sanction", do_sanction, POS_DEAD, 0, LOG_NORMAL, 1, 0 },
    { "sharpen", do_sharpen,POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "sleep",    do_sleep, POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "slice",	do_slice, POS_STANDING,	   0,  LOG_NORMAL, 1,  1 },
    { "sneak",    do_sneak, POS_STANDING,  0,  LOG_NORMAL, 1, 1 },
    { "snatch", do_snatch, POS_STANDING, 0, LOG_NORMAL, 1, 0 },
    { "split",    do_split, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "steal",    do_steal, POS_STANDING,  0,  LOG_NORMAL, 1, 0 },
    { "bump",    do_bump, POS_FIGHTING,  0,  LOG_NORMAL, 1, 0 },
    { "study", do_copyspell, POS_STANDING, 0, LOG_NORMAL, 1, 0 },
    { "trade",	do_trade,	POS_RESTING, 0, LOG_NORMAL, 1, 0 },
    { "train",    do_train, POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "visible",  do_visible, POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "wake",   do_wake,  POS_SLEEPING,  0,  LOG_NORMAL, 1, 1 },
    { "where",    do_where, POS_RESTING,   0,  LOG_NORMAL, 1, 0 },
    { "scan",    do_where, POS_RESTING,   0,  LOG_NORMAL, 1, 1 },
    { "shieldbash", do_shieldbash, POS_FIGHTING, 0, LOG_NORMAL, 1, 1 },



    /*
     * Immortal commands.
     */
    { "advance",  do_advance, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "dump",   do_dump,  POS_DEAD, ML,  LOG_ALWAYS, 0, 0 },
    { "eqlist",   do_eqlist,  POS_DEAD, L1,  LOG_ALWAYS, 0, 0 },
    { "trust",    do_trust, POS_DEAD, ML,  LOG_ALWAYS, 1, 0 },
    { "violate",  do_violate, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "allow",    do_allow, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "ban",    do_ban,   POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "dns",    do_dns,   POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "undns",    do_undns,   POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "deny",   do_deny,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
#ifdef OLC_VERSION
    { "recipe",    do_recipe,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "edit",    do_edit,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "matlog",    do_matlog,  POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "ftick", do_ftick, POS_DEAD, L5, LOG_ALWAYS, 1, 0 },
    { "edithelp",    do_edithelp,  POS_DEAD, L4,  LOG_ALWAYS, 1, 0 },// New help code
    { "ehelp",    do_edithelp,  POS_DEAD, L4,  LOG_ALWAYS, 1, 0 },
#endif
    { "bflag",   do_bflag, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "disconnect", do_disconnect,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "doublexp", do_doublexp, POS_DEAD, L3, LOG_ALWAYS, 1,0 },  
    { "dweeb",    do_dweeb, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "explode", do_explode, POS_DEAD, 55, LOG_ALWAYS, 1, 0 },
    { "flag",   do_flag,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "freeze",   do_freeze,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "fuck",	do_fuck,    POS_DEAD,	L3,  LOG_ALWAYS, 1, 0 },
    { "ident",	do_ident,  POS_DEAD,	ML,  LOG_ALWAYS, 1, 0 },
    { "listen", do_listen, POS_DEAD,	L3, LOG_NORMAL, 1,  0 },
    { "changepassword",  do_changepassword, POS_DEAD,   L1, LOG_ALWAYS, 1,  0 },
    { "ctalk",  do_ctalk, POS_DEAD,     L3, LOG_NORMAL, 1,  0 },
    { "permban",  do_permban, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "pload", do_pload, POS_DEAD, L1, LOG_ALWAYS, 1, 0 },
    { "pnet",   do_pnet,  POS_DEAD, 1,  LOG_NORMAL, 1, 0 },
    { "punload", do_punload, POS_DEAD, L1, LOG_ALWAYS, 1,  0 },
    { "protect",  do_protect, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "remort",    do_remort, POS_DEAD, 51,  LOG_ALWAYS, 1, 0 },
    { "reclas",    do_reclas, POS_DEAD, 26,  LOG_NORMAL, 1, 0 },
    { "reclass",    do_reclass, POS_DEAD,  1,  LOG_ALWAYS, 1, 0 },
    { "reboo",    do_reboo, POS_DEAD, L1,  LOG_NORMAL, 0, 0 },
    { "reboot",   do_reboot,  POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "rename", do_rename, POS_DEAD, L1, LOG_ALWAYS, 1, 0 },
    { "review", do_review, POS_DEAD,	L3, LOG_NORMAL, 1,  0 },
    { "set",    do_set,   POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "shutdow",  do_shutdow, POS_DEAD, L1,  LOG_NORMAL, 0, 0 },
    { "shutdown", do_shutdown,  POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
/*    { "socket",   do_socke,     POS_DEAD, L1,  LOG_NORMAL, 1, 0 }, */
#if defined ANDARONDEV || defined OLC_VERSION
    { "syncarea",   do_syncarea,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "synchelps",   do_synchelps,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
#else
    { "syncarea",   do_syncarea,  POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "synchelps",   do_synchelps,  POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
#endif
    { "sockets",        do_sockets, POS_DEAD,       L1,  LOG_NORMAL, 1, 0 },
    { "reward", do_reward,	 POS_DEAD, L5, LOG_ALWAYS, 1, 0 },
    { "spreward", do_spreward,	 POS_DEAD, L5, LOG_ALWAYS, 1, 0 },
    { "wizlock",  do_wizlock, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "dispvnum", do_dispvnum,    POS_DEAD, L7,  LOG_NORMAL, 1, 0 },
    { "force",    do_force, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "lag",	do_lag,	POS_DEAD, L3, LOG_ALWAYS, 1, 0 }, 
    { "load",   do_load,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "newlock",  do_newlock, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "nochannels", do_nochannels,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "no-dns", do_no_dns, POS_DEAD, L1, LOG_ALWAYS, 1, 0 },
    { "noemote",  do_noemote, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "nonotes",  do_nonotes, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 }, 
    { "noshout",  do_noshout, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "notel",   do_notel,  POS_DEAD, L3,  LOG_NORMAL, 0, 0 },
    { "notell",   do_notell,  POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "notitle",  do_notitle, POS_DEAD, L3, LOG_ALWAYS, 1, 0 },
    { "pecho",    do_pecho, POS_DEAD, L5,  LOG_ALWAYS, 1, 0 }, 
    { "pardon",   do_pardon,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "pur",      do_pur, POS_DEAD, L1,  LOG_ALWAYS, 1, 0 },
    { "purge",    do_purge, POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "restore",  do_restore, POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "sla",    do_sla,   POS_DEAD, L5,  LOG_NORMAL, 0, 0 },
    { "slay",   do_slay,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "teleport", do_transfer,    POS_DEAD, L5,  LOG_ALWAYS, 1, 0 }, 
    { "transfer", do_transfer,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "poofin",   do_bamfin,  POS_DEAD, L8,  LOG_NORMAL, 1, 0 },
    { "poofout",  do_bamfout, POS_DEAD, L8,  LOG_NORMAL, 1, 0 },
    { "gecho",    do_echo,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "holylight",  do_holylight, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "incognito",  do_incognito, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "invis",    do_invis, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "log",    do_log,   POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "memory",   do_memory,  POS_DEAD, L1,  LOG_NORMAL, 1, 0 },
    { "mwhere",   do_mwhere,  POS_DEAD, L5,  LOG_NORMAL, 1, 0 },
    { "noreply",  do_noreply, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "olist",    do_olist, POS_DEAD, L5, LOG_ALWAYS, 1,0 },
    { "owhere",   do_owhere,  POS_DEAD, L5,  LOG_NORMAL, 1, 0 },
    { "peace",    do_peace, POS_DEAD, L5,  LOG_NORMAL, 1, 0 },
    { "penalty",  do_penalty, POS_DEAD, L5,  LOG_NORMAL, 1, 0 }, 
    { "echo",   do_recho, POS_DEAD, L7,  LOG_ALWAYS, 1, 0 },
    { "return",   do_return,   POS_DEAD,  L5,  LOG_NORMAL, 1, 0 },
    { "skstat",   do_skstat, POS_DEAD, 25, LOG_ALWAYS, 1, 0},
    { "snoop",    do_snoop, POS_DEAD, L3,  LOG_ALWAYS, 1, 0 },
    { "stat",   do_stat,  POS_DEAD, L7,  LOG_ALWAYS, 1, 0 },
    { "startgladiator",do_startgladiator,POS_DEAD,L5,LOG_ALWAYS, 1, 0 },
    { "endgladiator",do_endgladiator,POS_DEAD,L5,LOG_ALWAYS, 1, 0 },
    { "skipbet",do_skipbet,POS_DEAD,L5,LOG_ALWAYS, 1, 0 },
    { "removegladiator",do_removegladiator,POS_DEAD,L5,LOG_ALWAYS, 1, 0 },
    { "setcouncil", do_setcouncil, POS_DEAD, L1, LOG_ALWAYS, 1, 0},
    { "string",   do_string,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "switch",   do_switch,  POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "wizinvis", do_invis, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "vnum",   do_vnum,  POS_DEAD, L7,  LOG_NORMAL, 1, 0 },
    { "zecho",    do_zecho, POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "clone",    do_clone, POS_DEAD, L5,  LOG_ALWAYS, 1, 0 },
    { "wiznet",   do_wiznet,  POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "immtalk",  do_immtalk, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "gnote",   do_immnote,  POS_SLEEPING,  L7,  LOG_NORMAL, 1, 0 },
    { "immload",   do_imm_loads,  POS_DEAD, L1,  LOG_ALWAYS, 0, 0 },
    { "imotd", do_imotd, POS_DEAD,       IM,  LOG_NORMAL, 1, 0 },
    { ":",    do_immtalk, POS_DEAD, IM,  LOG_NORMAL, 0, 0 },
    { "icg", do_icg, POS_DEAD,   L1,  LOG_ALWAYS, 1, 0 },
    { "smote",    do_smote, POS_DEAD, IM,  LOG_NORMAL, 1, 0 },
    { "macro",    do_macro, POS_DEAD,  L8,  LOG_NORMAL, 1, 0 },    

    /*
     * IMC commands

    { "rtell",          do_rtell,       POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "rreply",         do_rreply,      POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "rwho",           do_rwho,        POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "rwhois",         do_rwhois,      POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "rquery",         do_rquery,      POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "rfinger",        do_rfinger,     POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "imc",            do_imc,         POS_DEAD,      L1,   LOG_ALWAYS, 1, 0 },
    { "imclist",        do_imclist,     POS_DEAD,       0,   LOG_NORMAL, 1, 0 },
    { "rbeep",          do_rbeep,       POS_SLEEPING,   0,   LOG_NORMAL, 1, 0 },
    { "istats",         do_istats,      POS_DEAD,       0,   LOG_NORMAL, 1, 0 },
    { "rchannels",      do_rchannels,   POS_DEAD,       0,   LOG_NORMAL, 1, 0 },

     */
    /* IMC imm commands

    { "rinfo",          do_rinfo,       POS_DEAD,       0,   LOG_NORMAL, 1, 0 },
    { "rsockets",       do_rsockets,    POS_DEAD,      IM,   LOG_NORMAL, 1, 0 },
    { "rconnect",       do_rconnect,    POS_DEAD,      L3,   LOG_ALWAYS, 1, 0 },
    { "rdisconnect",    do_rdisconnect, POS_DEAD,      L3,   LOG_ALWAYS, 1, 0 },
    { "rignore",        do_rignore,     POS_DEAD,      L3,   LOG_ALWAYS, 1, 0 },
    { "rchanset",       do_rchanset,    POS_DEAD,      L5,   LOG_ALWAYS, 1, 0 },
    { "mailqueue",      do_mailqueue,   POS_DEAD,      L4,   LOG_NORMAL, 1, 0 },
    { "icommand",       do_icommand,    POS_DEAD,      IM,   LOG_NORMAL, 1, 0 },
    { "isetup",         do_isetup,      POS_DEAD,      IM,   LOG_NORMAL, 1, 0 },
    { "ilist",          do_ilist,       POS_DEAD,       0,   LOG_NORMAL, 1, 0 },
    { "ichannels",      do_ichannels,   POS_DEAD,       0,   LOG_NORMAL, 1, 0 },
    { "rping",          do_rping,       POS_DEAD,      L3,   LOG_NORMAL, 1, 0 },
     */
    /*
     * End of list.
     */
    { "",   0,    POS_DEAD,  0,  LOG_NEVER, 0, 0 }
};




/*
 * The main entry point for executing commands.
 * Can be recursively called from 'at', 'order', 'force'.
 */
void interpret( CHAR_DATA *ch, char *argument )
{
    char command[MAX_INPUT_LENGTH];
    char logline[MAX_INPUT_LENGTH];
    int cmd,trust,immc=-1,icgb=0;
    sh_int icg=0;
    bool found;
    int strings = nAllocString;
    int permc = nAllocPerm;
    char commc[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];

  if (ch->pcdata != NULL && ch->pcdata->interp_fun != NULL) {
      ch->pcdata->interp_fun (ch, argument);
      return;
  }     
  
    /*
     * Strip leading spaces.
     */
    while ( isspace(*argument) )
  argument++;
    if ( argument[0] == '\0' )
  return;

    /*
     * Implement freeze command.
     */
    if ( !IS_NPC(ch) && IS_SET(ch->act, PLR_FREEZE) )
    {
  send_to_char( "You're totally frozen!\n\r", ch );
  return;
    }

    /*
     * Grab the command word.
     * Special parsing so ' can be a command,
     *   also no spaces needed after punctuation.
     */
    strcpy( logline, argument );
    if ( !isalpha(argument[0]) && !isdigit(argument[0]) )
    {
  command[0] = argument[0];
  command[1] = '\0';
  argument++;
  while ( isspace(*argument) )
      argument++;
    }
    else
    {
  argument = one_argument( argument, command );
    }

    /*
     * Look for command in command table.
     */
    found = FALSE;
    trust = get_trust( ch );
#ifdef OLC_VERSION
    if(ch->icg == ICG_BUILD)
	trust = MAX_LEVEL - 1;
#else
    if( IS_NPC(ch) && !str_prefix(command, "return") )
	trust = MAX_LEVEL -1;
#endif
    immc = immc_lookup(command);
    icgb = ch->icg_bits;
    icg = ch->icg;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
  if ( command[0] == cmd_table[cmd].name[0]
  &&   !str_prefix( command, cmd_table[cmd].name )
  &&   ( cmd_table[cmd].level <= trust  
	|| ( immc != -1 
		&& (  imm_command_table[immc].icg == icg
		   || IS_SET(icgb,imm_command_table[immc].bit) )
	   )
	)
     )
  {
      found = TRUE;
      break;
  }
    }

    /*
     * Log and snoop.
     *
    if ( cmd_table[cmd].log == LOG_NEVER )
  strcpy( logline, "" );
     */

    if ( ( !IS_NPC(ch) && IS_SET(ch->act, PLR_LOG) )
    ||   fLogAll
    ||   (cmd_table[cmd].log == LOG_ALWAYS && ch->level != ML) )
    {
    char    s[2*MAX_INPUT_LENGTH],*ps;
    int     i;
 
    ps=s; 
    sprintf( log_buf, "Log %s: %s", ch->name, logline );
    /* Make sure that was is displayed is what is typed */
    for (i=0;log_buf[i];i++) { 
        *ps++=log_buf[i];  
        if (log_buf[i]=='$')
            *ps++='$';
        if (log_buf[i]=='{')
            *ps++='{';
    }
    *ps=0;
/* Lets see if this works...NIGHTDAGGER */
if (cmd_table[cmd].log != LOG_NEVER)
  wiznet(log_buf,ch,NULL,WIZ_SECURE,0,get_trust(ch));
  log_string( log_buf );
    }
/* Disabling the "temporary" log all
    else
    {
	sprintf( log_buf, "Log %s: %s", ch->name, logline );
	log_string( log_buf );
    } */

    if ( ch->desc != NULL && ch->desc->snoop_by != NULL )
    {
  write_to_buffer( ch->desc->snoop_by, "% ",    2 );
  write_to_buffer( ch->desc->snoop_by, logline, 0 );
  write_to_buffer( ch->desc->snoop_by, "\n\r",  2 );
    }

    if ( !found )
    {
  /*
   * Look for command in socials table.
   */
  if ( !check_social( ch, command, argument ) )
       /*
	&& !icec_command_hook(ch, command, argument) )*/
      send_to_char( "Huh?\n\r", ch );
  return;
    }

    /*
     * Character not in position for command?
     */
    if ( ch->position < cmd_table[cmd].position )
    {
  switch( ch->position )
  {
  case POS_DEAD:
      send_to_char( "Lie still; you are DEAD.\n\r", ch );
      break;

  case POS_MORTAL:
  case POS_INCAP:
      send_to_char( "You are hurt far too bad for that.\n\r", ch );
      break;

  case POS_STUNNED:
      send_to_char( "You are too stunned to do that.\n\r", ch );
      break;

  case POS_SLEEPING:
      send_to_char( "In your dreams, or what?\n\r", ch );
      break;

  case POS_RESTING:
      send_to_char( "Nah... You feel too relaxed...\n\r", ch);
      break;

  case POS_SITTING:
      send_to_char( "Better stand up first.\n\r",ch);
      break;

  case POS_FIGHTING:
      send_to_char( "No way!  You are still fighting!\n\r", ch);
      break;

  }
  return;
    }

     /* Wiznet memory leak debugging */
    strcpy(commc,argument);

    /* DEstroy pending actions here */
    if ( !IS_NPC(ch))
    {
      if(ch->pcdata->wraith_timer > 0)
      {
	ch->pcdata->wraith_timer = 0;
	act("You failed to go to wraithform!",ch,NULL,NULL,TO_CHAR,FALSE);
	act("$n failed to go to wraithform!",ch,NULL,NULL,TO_ROOM,FALSE);
      }
      if(ch->pcdata->pulse_timer > 0)
      {
        end_pulse_command(ch, FALSE, FALSE);
      }
    }
	
    if ( is_affected(ch,gsn_acclimate) )
    {
        AFFECT_DATA *paf;

        paf = affect_find(ch->affected, gsn_acclimate);
        affect_remove(ch,paf,APPLY_BOTH);
        act("You no longer are attuned with your environment.",ch,NULL,NULL,TO_CHAR,FALSE);
        act("$n is no longer attuned with $s environment.",ch,NULL,NULL,TO_ROOM,FALSE);
    }

    /*
     * Dispatch the command.
     */
    (*cmd_table[cmd].do_fun) ( ch, argument );

    tail_chain( );

    if ( strings < nAllocString )
    {
	sprintf(buf,"Memory: String allocation increase (%s/%s)",
		ch->name, commc);
	wiznet(buf,NULL,NULL,WIZ_DEBUG,0,0);
    }

    if ( permc < nAllocPerm )
    {
	sprintf(buf,"Memory: nAllocerm increase (%s/%s)",
		ch->name, commc);
	wiznet(buf,NULL,NULL,WIZ_DEBUG,0,0);
    }

/*
    if ( is_affected(ch,gsn_acclimate) )
    {
	AFFECT_DATA *paf;

	paf = affect_find(ch->affected, gsn_acclimate);
	affect_remove(ch,paf,APPLY_BOTH);
	act("You no longer are attuned with your environment.",ch,NULL,NULL,TO_CHAR,FALSE);
	act("$n is no longer attuned with $s environment.",ch,NULL,NULL,TO_ROOM,FALSE);
    }
*/
    tail_chain(); /* Never disturb the sleeping giant */
    return;
}



bool check_social( CHAR_DATA *ch, char *command, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
    int cmd;
    bool found;

    found  = FALSE;
    for ( cmd = 0; social_table[cmd].name[0] != '\0'; cmd++ )
    {
  if ( command[0] == social_table[cmd].name[0]
  &&   !str_prefix( command, social_table[cmd].name ) )
  {
      found = TRUE;
      break;
  }
    }

    if ( !found )
  return FALSE;

    if ( !IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE) )
    {
  send_to_char( "You are anti-social!\n\r", ch );
  return TRUE;
    }

    switch ( ch->position )
    {
    case POS_DEAD:
  send_to_char( "Lie still; you are DEAD.\n\r", ch );
  return TRUE;

    case POS_INCAP:
    case POS_MORTAL:
  send_to_char( "You are hurt far too bad for that.\n\r", ch );
  return TRUE;

    case POS_STUNNED:
  send_to_char( "You are too stunned to do that.\n\r", ch );
  return TRUE;

    case POS_SLEEPING:
  /*
   * I just know this is the path to a 12" 'if' statement.  :(
   * But two players asked for it already!  -- Furey
   */
  if ( !str_cmp( social_table[cmd].name, "snore" ) )
      break;
  send_to_char( "In your dreams, or what?\n\r", ch );
  return TRUE;

    }

    one_argument( argument, arg );
    victim = NULL;
    if ( arg[0] == '\0' )
    {
  act( social_table[cmd].others_no_arg, ch, NULL, victim, TO_ROOM    ,FALSE);
  act( social_table[cmd].char_no_arg,   ch, NULL, victim, TO_CHAR    ,FALSE);
    }
    else if ( ( victim = get_char_room( ch, arg ) ) == NULL )
    {
  send_to_char( "They aren't here.\n\r", ch );
    }
    else if ( victim == ch )
    {
  act( social_table[cmd].others_auto,   ch, NULL, victim, TO_ROOM    ,FALSE);
  act( social_table[cmd].char_auto,     ch, NULL, victim, TO_CHAR    ,FALSE);
    }
    else
    {
  act( social_table[cmd].others_found,  ch, NULL, victim, TO_NOTVICT ,FALSE);
  act( social_table[cmd].char_found,    ch, NULL, victim, TO_CHAR    ,FALSE);
  act( social_table[cmd].vict_found,    ch, NULL, victim, TO_VICT    ,FALSE);

  if ( !IS_NPC(ch) && IS_NPC(victim)
  &&   !IS_AFFECTED(victim, AFF_CHARM)
  &&   IS_AWAKE(victim) 
  &&   victim->desc == NULL)
  {
	if(victim->pIndexData->vnum == MOB_VNUM_INSANE_MIME)
	{// Special mime handling
		if(!victim->qchar || victim->qchar != ch)
			return TRUE;// He ignores socials from people he's not duelling 
		if(cmd == victim->qnum2)
		{// Continue the duel
			quest_handler(victim, ch, NULL, QUEST_MIME, QSTEP_WIN);
		}
		else if(victim->qnum > 0)
		{// Violent failure, the duel is in progress and the player got it wrong
			quest_handler(victim, ch, NULL, QUEST_MIME, QSTEP_LOSE);
		}
		else
		{// Shrug and leave, the player didn't accept the duel but did social back
			quest_handler(victim, ch, NULL, QUEST_MIME, QSTEP_MOVE);
		}
		return TRUE;
	}

      switch ( number_bits( 4 ) )
      {
      case 0:

      case 1: case 2: case 3: case 4:
      case 5: case 6: case 7: case 8:
    act( social_table[cmd].others_found,
        victim, NULL, ch, TO_NOTVICT ,FALSE);
    act( social_table[cmd].char_found,
        victim, NULL, ch, TO_CHAR    ,FALSE);
    act( social_table[cmd].vict_found,
        victim, NULL, ch, TO_VICT    ,FALSE);
    break;

      case 9: case 10: case 11: case 12:
    act( "$n slaps $N.",  victim, NULL, ch, TO_NOTVICT ,FALSE);
    act( "You slap $N.",  victim, NULL, ch, TO_CHAR    ,FALSE);
    act( "$n slaps you.", victim, NULL, ch, TO_VICT    ,FALSE);
    break;
      }
  }
    }

    return TRUE;
}



/*
 * Return true if an argument is completely numeric.
 */
bool is_number ( char *arg )
{
 
    if ( *arg == '\0' )
        return FALSE;
 
    if ( *arg == '+' || *arg == '-' )
        arg++;
 
    for ( ; *arg != '\0'; arg++ )
    {
        if ( !isdigit( *arg ) )
            return FALSE;
    }
 
    return TRUE;
}



/*
 * Given a string like 14.foo, return 14 and 'foo'
 */
int number_argument( char *argument, char *arg )
{
    char *pdot;
    int number;
    
    for ( pdot = argument; *pdot != '\0'; pdot++ )
    {
  if ( *pdot == '.' )
  {
      *pdot = '\0';
      number = atoi( argument );
      *pdot = '.';
      strcpy( arg, pdot+1 );
      return number;
  }
    }

    strcpy( arg, argument );
    return 1;
}

/* 
 * Given a string like 14*foo, return 14 and 'foo'
*/
int mult_argument(char *argument, char *arg)
{
    char *pdot;
    int number;

    for ( pdot = argument; *pdot != '\0'; pdot++ )
    {
        if ( *pdot == '*' )
        {
            *pdot = '\0';
            number = atoi( argument );
            *pdot = '*';
            strcpy( arg, pdot+1 );
            return number;
        }
    }
 
    strcpy( arg, argument );
    return 1;
}


char *one_argument_cs( char *argument, char *arg_first )
{
   char cEnd;

   while( isspace(*argument) )
      argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
    cEnd = *argument++;

    while ( *argument != '\0' )
    {
      if ( *argument == cEnd )
      {
         argument++;
	 break;
      }
	*arg_first = *argument;
        arg_first++;
        argument++;
    }
    *arg_first = '\0';

    while ( isspace(*argument) )
   	argument++;

      return argument;
}


/*
 * Pick off one argument from a string and return the rest.
 * Understands quotes.
 */
char *one_argument( char *argument, char *arg_first )
{
    char cEnd;

    while ( isspace(*argument) )
  argument++;

    cEnd = ' ';
    if ( *argument == '\'' || *argument == '"' )
  cEnd = *argument++;

    while ( *argument != '\0' )
    {
  if ( *argument == cEnd )
  {
      argument++;
      break;
  }
  *arg_first = LOWER(*argument);
  arg_first++;
  argument++;
    }
    *arg_first = '\0';

    while ( isspace(*argument) )
  argument++;

    return argument;
}

/*
 * Contributed by Alander.
 */
void do_commands( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cmd;
    int col;
 
    col = 0;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
        if ( cmd_table[cmd].level <  LEVEL_HERO
        &&   cmd_table[cmd].level <= get_trust( ch ) 
  &&   cmd_table[cmd].show)
  {
      sprintf( buf, "%-12s", cmd_table[cmd].name );
      send_to_char( buf, ch );
      if ( ++col % 6 == 0 )
    send_to_char( "\n\r", ch );
  }
    }
 
    if ( col % 6 != 0 )
  send_to_char( "\n\r", ch );
    return;
}

void do_wizhelp( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    int cmd,col,immc;
 
    col = 0;
    for ( cmd = 0; cmd_table[cmd].name[0] != '\0'; cmd++ )
    {
	immc = immc_lookup(cmd_table[cmd].name);
        if ( cmd_table[cmd].level >= LEVEL_HERO
        &&   (cmd_table[cmd].level <= get_trust(ch) 
	      || ch->icg == imm_command_table[immc].icg
#ifdef OLC_VERSION
	      || ch->icg == ICG_BUILD
#endif
	      || IS_SET(ch->icg_bits,imm_command_table[immc].bit) )
        &&   cmd_table[cmd].show)
  {
      sprintf( buf, "%-15s", cmd_table[cmd].name );
      send_to_char( buf, ch );
      if ( ++col % 5 == 0 )
    send_to_char( "\n\r", ch );
  }
    }
 
    if ( col % 6 != 0 )
  send_to_char( "\n\r", ch );
    return;
}

