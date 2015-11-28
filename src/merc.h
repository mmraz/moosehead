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
 *  M uch time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/
/* $Id: merc.h,v 1.406 2004/10/25 02:48:45 boogums Exp $"; */
#define unix

#if     defined(_AIX)
#if     !defined(const)
#define const
#endif
typedef int                             sh_int;
typedef int                             bool;
#define unix
#else
typedef short   int                     sh_int;
typedef unsigned char                   bool;
#endif

/*
 * Function types.
 */
#define args( list )                    list
typedef struct  char_data               CHAR_DATA;
typedef void DO_FUN     args( ( CHAR_DATA *ch, char *argument ) );
typedef bool SPEC_FUN   args( ( CHAR_DATA *ch ) );
typedef void SPELL_FUN  args( ( int sn, int level, CHAR_DATA *ch, void *vo,
        int target ) );
typedef void MENU_FUN   args( ( CHAR_DATA *ch, int menu_id ) );        
/*
 *  Accommodate old non-Ansi compilers.
 */
#if defined(TRADITIONAL)
#define const
#define args( list )                    ( )
#define DECLARE_DO_FUN( fun )           void fun( )
#define DECLARE_SPEC_FUN( fun )         bool fun( )
#define DECLARE_SPELL_FUN( fun )        void fun( )
#else
#define DECLARE_DO_FUN( fun )           DO_FUN    fun
#define DECLARE_SPEC_FUN( fun )         SPEC_FUN  fun
#define DECLARE_SPELL_FUN( fun )        SPELL_FUN fun
#endif

/* system calls */
int unlink();
int system();



/*
 * Short scalar types.
 * Diavolo reports AIX compiler has bugs with short types.
 */
#if     !defined(FALSE)
#define FALSE    0
#endif

#if     !defined(TRUE)
#define TRUE     1
#endif


/*
 * Structure types.
 */
typedef struct plan_exit_data PLAN_EXIT_DATA;
typedef struct alliance_data ALLIANCE_DATA;
typedef struct clan_data CLAN_DATA;
typedef struct clan_char CLAN_CHAR;
typedef struct  merit_tracker           MERIT_TRACKER;
typedef struct  plan_data               PLAN_DATA;
typedef struct  damage_data             DAMAGE_DATA;
typedef struct  affect_data             AFFECT_DATA;
typedef struct  area_data               AREA_DATA;
typedef struct  ban_data                BAN_DATA;
typedef struct  dns_data                DNS_DATA;
typedef struct  buf_type                BUFFER;
typedef struct  cstat_data              CSTAT_DATA;
typedef struct  descriptor_data         DESCRIPTOR_DATA;
typedef struct  exit_data               EXIT_DATA;
typedef struct  extra_descr_data        EXTRA_DESCR_DATA;
typedef struct  help_data               HELP_DATA;
typedef struct  help_tracker            HELP_TRACKER;// New help code
typedef struct  kill_data               KILL_DATA;
typedef struct  mob_index_data          MOB_INDEX_DATA;
typedef struct  note_data               NOTE_DATA;
typedef struct  trade_data		TRADE_DATA;
typedef struct  obj_data                OBJ_DATA;
typedef struct  obj_index_data          OBJ_INDEX_DATA;
typedef struct  pc_data                 PC_DATA;
typedef struct  gen_data                GEN_DATA;
typedef struct  reset_data              RESET_DATA;
typedef struct  room_index_data         ROOM_INDEX_DATA;
typedef struct  shop_data               SHOP_DATA;
typedef struct  time_info_data          TIME_INFO_DATA;
typedef struct  weather_data            WEATHER_DATA;
typedef struct  menu_item               MENU_ITEM;
typedef struct  menu_item               MENU_DATA;
typedef struct  edit_data               EDIT_DATA;
typedef struct  macro_data              MACRO_DATA;
typedef struct  line_data               LINE_DATA; 
typedef struct  line_edit_data          LINE_EDIT_DATA; 
/* typedef struct  mem_data                MEM_DATA; */
typedef struct  vnum_range_data         VNUM_RANGE_DATA;
typedef struct  card_data		CARD_DATA;
typedef struct  recipe_data             RECIPE_DATA;


/* UID of the user that the game runs as */
#define MUD_UID	1001

/*
 * String and memory management parameters.
 */
#define MAX_KEY_HASH             1024
#define MAX_STRING_LENGTH        4608
#define MAX_INPUT_LENGTH          256
#define PAGELEN                    22

/* Color equivalency definitions for escape sequences */
/*
Set Graphics Rendition:
         ESC[#;#;....;#m                Set display attributes where # is
                                            0 for normal display
                                            1 for bold on
                                            4 underline (mono only)
                                            5 blink on
                                            7 reverse video on
                                            8 nondisplayed (invisible)
                                            30 black foreground
                                            31 red foreground
                                            32 green foreground
                                            33 yellow foreground
                                            34 blue foreground
                                            35 magenta foreground
                                            36 cyan foreground
                                            37 white foreground
                                            40 black background
                                            41 red background
                                            42 green background
                                            43 yellow background
                                            44 blue background
                                            45 magenta background
                                            46 cyan background
                                            47 white background
*/
#define NORMAL		"[0m"
#define BOLD		"[1m"
#define BLINK		"[5m"
#define BLACK		"[30m"
#define RED		"[31m"
#define GREEN		"[32m"
#define YELLOW		"[33m"
#define BLUE		"[34m"
#define MAGENTA		"[35m"
#define CYAN		"[36m"
#define WHITE		"[37m"


#define CLEAR           "[0m"          /* Resets Colour        */
#define C_RED           "[0;31m"       /* Normal Colours       */
#define C_GREEN         "[0;32m"
#define C_YELLOW        "[0;33m"
#define C_BLUE          "[0;34m"
#define C_MAGENTA       "[0;35m"
#define C_CYAN          "[0;36m"
#define C_WHITE         "[0;37m"
#define C_D_GREY        "[1;30m"       /* Light Colors         */
#define C_B_RED         "[1;31m"
#define C_B_GREEN       "[1;32m"
#define C_B_YELLOW      "[1;33m"
#define C_B_BLUE        "[1;34m"
#define C_B_MAGENTA     "[1;35m"
#define C_B_CYAN        "[1;36m"
#define C_B_WHITE       "[1;37m"

/*
 * Game parameters.
 * Increase the max'es if you add more of something.
 * Adjust the pulse numbers to suit yourself.
 */
#define MAX_SOCIALS               256
#define MAX_SKILL                 450
#define MAX_GROUP                  67 
#define MAX_IN_GROUP               30
#define MAX_ALIAS                  15
#define MAX_RANK		    5
#define MAX_CLASS                  15
#define MAX_GIFTS		   16
#define MAX_GIFT		    3
#define MAX_PC_RACE                17 
#define MAX_CLAN                   22 
#define MAX_DEITY                  11
#define MAX_SAC_PNTS              300
#define MAX_OBJ_SIZE		    6
#define MAX_LEVEL                  60
#define MAX_DISABLE		    5
#define MAX_LOOT_ITEMS		    4
#define LEVEL_HERO                 (MAX_LEVEL - 9)
#define LEVEL_IMMORTAL             (MAX_LEVEL - 8)
#define MAX_IN_RECIPE              20
#define MAX_LIQUIDS                38

#define PULSE_PER_SECOND            5
#define PULSE_VIOLENCE            ( 3 * PULSE_PER_SECOND)
#define PULSE_MOBILE              ( 4 * PULSE_PER_SECOND)
#define PULSE_MUSIC               ( 6 * PULSE_PER_SECOND)

#define PULSE_DOT		   (6 * PULSE_PER_SECOND)
#define PULSE_TICK                (40 * PULSE_PER_SECOND)
#define PULSE_AREA                (120 * PULSE_PER_SECOND)

#define IMPLEMENTOR             MAX_LEVEL
#define CREATOR                 (MAX_LEVEL - 1)
#define SUPREME                 (MAX_LEVEL - 2)
#define DEITY                   (MAX_LEVEL - 3)
#define GOD                     (MAX_LEVEL - 4)
#define IMMORTAL                (MAX_LEVEL - 5)
#define DEMI                    (MAX_LEVEL - 6)
#define ANGEL                   (MAX_LEVEL - 7)
#define AVATAR                  (MAX_LEVEL - 8)
#define HERO                    LEVEL_HERO

#define PC_LOWER_KILLS		0
#define PC_EQUAL_KILLS	 	1	
#define PC_GREATER_KILLS	2 
#define PC_DEATHS		3

#define PC_STOLEN_ITEMS		0 
#define PC_STOLEN_GOLD		1 
#define PC_SLICES     		2 

#define ALL_KILLS       0
#define REAL_KILLS      1

#define CLASS_MAGE      0
#define CLASS_CLERIC    1
#define CLASS_THIEF     2
#define CLASS_WARRIOR   3

#define GLADIATOR_VICTORIES 0
#define GLADIATOR_KILLS 1
#define GLADIATOR_TEAM_VICTORIES 2
#define GLADIATOR_TEAM_KILLS 3
#define GLADIATOR_PLAYS 4 
#define GLADIATOR_TEAM_PLAYS 5 

#define ICG_ADMIN	4
#define ICG_JUDGE	3
#define ICG_BUILD	2
#define ICG_QUEST	1
#define ICG_NONE	0

/* Pulse types */

#define PULSE_RECALL 1
#define PULSE_STEALMERIT 2

/* Item rarity */

#define RARITY_STOCK 1
#define RARITY_COMMON 2
#define RARITY_UNCOMMON 4
#define RARITY_RARE 8
#define RARITY_GEM 16
#define RARITY_IMPOSSIBLE 32
#define RARITY_ALL 63
#define RARITY_LOOTED 64 /* Just needs to be highest */

/* Live edit defines */

#define PLAN_ROOM    1
#define PLAN_MOB     2
#define PLAN_ITEM    4
#define PLAN_EXIT    8
#define PLAN_MASK_TYPE 15
#define PLAN_PLANNED 16
#define PLAN_PLACED 32
#define PLAN_PREVIEWED 64

/* Room type flags */
#define PLAN_ROOM_REGEN 1
#define PLAN_ROOM_LAB 2
#define PLAN_ROOM_ALTAR 4
#define PLAN_ROOM_OUTDOORS 8
#define PLAN_ROOM_MARKED 16 /* Used for removal marking */
#define PLAN_ROOM_DARK 32

/* Mob type flags */
#define PLAN_MOB_MERCHANT 1
#define PLAN_MOB_HEALER 2
#define PLAN_MOB_GOOD 4/* Neutral is default */
#define PLAN_MOB_EVIL 8
#define PLAN_MOB_NEUTER 16 /* Male is default */
#define PLAN_MOB_FEMALE 32

/* Item type flags */
#define PLAN_ITEM_PORTAL 1
#define PLAN_ITEM_DRINK 2
#define PLAN_ITEM_FURNITURE 4
#define PLAN_ITEM_DOODAD 8
#define PLAN_ITEM_FOUNTAIN 16
#define PLAN_ITEM_PIT 32
#define PLAN_MASK_OBJ 63
#define PLAN_ITEM_HIDDEN 64

#define PLAN_EXIT_CLOSABLE 1
#define PLAN_EXIT_LOCKABLE 2
#define PLAN_EXIT_NOPICK 4
#define PLAN_EXIT_HIDDEN 8
#define PLAN_EXIT_CLOSED 16
#define PLAN_EXIT_LOCKED 32
#define PLAN_EXIT_NOPASS 64

/* To add new defines, any ranges need to be continuous.  Otherwise they may be
 * added at the end regardless of what the value is for. */
/* Existing values may be changed, but be VERY CAREFUL incrementing all the
 * others - they must not overlap, and they must line up with the table define
 * in const.c */ 
#define PRICE_ROOM        0
#define PRICE_R_REGEN     1
/* Regen values: 100-200% at 25% per = 4
 * 200%-300% at 10% per = 10.  14 */
#define PRICE_R_REGEN_MID 4 /* Cutoff for calculation shift */
#define PRICE_R_REGEN_END 14
#define PRICE_R_REGEN_COUNT 14 /* Count of them, not index locked */
#define PRICE_LAB         15
#define PRICE_LAB_END     19 /* 5 lab types */
#define PRICE_LAB_COUNT   3 //5 /* Temporary lowering */
#define PRICE_ALTAR       20
#define PRICE_ALTAR_END   24 /* 5 altar types */
#define PRICE_ALTAR_COUNT 5
#define PRICE_ITEM        25 /* Generic item, probably cost 0 */
#define PRICE_FURNITURE   26
#define PRICE_F_REGEN     27
#define PRICE_F_REGEN_END 30
#define PRICE_F_REGEN_COUNT 4 /* 4 regen upgrades to furniture */
#define PRICE_FOUNTAIN    31
#define PRICE_PIT         32
#define PRICE_PORTAL      33
#define PRICE_DOODAD      34
#define PRICE_DRINK       35
#define PRICE_MOB         36 /* Generic mob, probably cost 0 */
#define PRICE_HEALER      37
#define PRICE_H_LEVEL     38
#define PRICE_H_LEVEL_END 52 /* 15 levels of healer upgrades */
#define PRICE_H_LEVEL_COUNT 15
#define PRICE_MERCHANT    53
#define PRICE_M_ITEM      54
#define PRICE_M_DISCOUNT  55
#define PRICE_M_DISCOUNT_END 59 /* 5 levels of discounts */
#define PRICE_M_DISCOUNT_COUNT 5
#define PRICE_EXIT        60 /* Basic exit price */
#define PRICE_E_CLOSABLE  61 /* Exit settings */
#define PRICE_E_LOCKABLE  62
#define PRICE_E_NO_PICK   63
#define PRICE_E_HIDDEN    64 /* Price to hide an exit */
#define PRICE_LINK  65 /* Price to add a link between rooms - standard room price only includes one exit */
#define PRICE_TOTAL       66 /* Make sure this is kept up to date or the PRICE_STR macro breaks */
/* PRICE_TOTAL should be the final value + 1 -- ie, the count */

#define EDITMODE_HALL     (A)
#define EDITMODE_PERSONAL (B)
#define EDITMODE_DESC     (C)
#define EDITMODE_EDIT     (D)
#define EDITMODE_RULES    (E)
#define EDITMODE_CHARTER  (F)

/* Portal vnums */
#define VNUM_PORTAL_HOAN 6102
#define VNUM_PORTAL_DROW 5100
#define VNUM_PORTAL_SANDS 5001
#define VNUM_PORTAL_THALOS 5300
#define VNUM_PORTAL_CANYON 9209
#define VNUM_PORTAL_PYRAMIDS 8701
#define VNUM_PORTAL_EMERALD 3521
#define VNUM_PORTAL_ATLANTIS 2318
#define VNUM_PORTAL_CAMELOT 17676
#define VNUM_PORTAL_THIEVES 6023
#define VNUM_PORTAL_NEW_THALOS 9606

#define HALL_TYPE_NONE 0
#define HALL_TYPE_BASIC 1
#define HALL_TYPE_ADVANCED 2 /* These are flags */
#define BASE_BONUS_TRIBUTE 2000000 /* 20000 tribute */

#define CLAN_FILE_VERSION 2

/* Clan type defines */
#define CLAN_ENEMY_SMALL 1
#define CLAN_ENEMY_MEDIUM 2
#define CLAN_ENEMY_LARGE 3
#define CLAN_ENEMY_FIGHTER 4
#define CLAN_ENEMY_CASTER 5
#define CLAN_ENEMY_FLAGGED 6
#define CLAN_ENEMY_UNFLAGGED 7
#define CLAN_ENEMY_REMORTED 8
#define CLAN_ENEMY_UNREMORTED 9
#define CLAN_ENEMY_LAW 10
#define CLAN_ENEMY_FAITH 11
#define CLAN_ENEMY_GREED 12
#define CLAN_ENEMY_MALICE 13
#define CLAN_ENEMY_PEACE 14
#define CLAN_ENEMY_CHAOS 15
#define CLAN_ENEMY_GANG 16

#define CLAN_TYPE_GANG 0
#define CLAN_TYPE_LAW 1
#define CLAN_TYPE_FAITH 2
#define CLAN_TYPE_GREED 4
#define CLAN_TYPE_MALICE 8
#define CLAN_TYPE_PEACE 16
#define CLAN_TYPE_CHAOS 32

#define CLAN_LONER 1
#define CLAN_OUTCAST 2

/*
 * Site ban structure.
 */
 
struct vnum_range_data 
{
  int min;
  int max;
  VNUM_RANGE_DATA *next;
}; 

#define BAN_SUFFIX              A
#define BAN_PREFIX              B
#define BAN_NEWBIES             C
#define BAN_ALL                 D       
#define BAN_PERMIT              E
#define BAN_PERMANENT           F

struct  ban_data
{
    BAN_DATA *  next;
    bool        valid;
    sh_int      ban_flags;
    sh_int      level;
    char *      name;
};

struct  dns_data
{
    DNS_DATA *  next;
    bool        valid;
    char *      name;
};

struct buf_type
{
    BUFFER *    next;
    bool        valid;
    sh_int      state;  /* error state of the buffer */
    sh_int      size;   /* size in k */
    char *      string; /* buffer's string */
};

/* for playing blackjack and other games */
struct card_data
{
  sh_int	suit;
  sh_int	value;
  int		deck;
};

#define	SUIT_CLUBS	0
#define	SUIT_HEARTS	1
#define	SUIT_SPADES	2
#define	SUIT_DIAMONDS	3


/*
 * Time and weather stuff.
 */
#define SUN_DARK                    0
#define SUN_RISE                    1
#define SUN_LIGHT                   2
#define SUN_SET                     3

#define SKY_CLOUDLESS               0
#define SKY_CLOUDY                  1
#define SKY_RAINING                 2
#define SKY_LIGHTNING               3

struct  time_info_data
{
    int         hour;
    int         day;
    int         month;
    int         year;
};

struct  weather_data
{
    int         mmhg;
    int         change;
    int         sky;
    int         sunlight;
};

/* Note and text edit related defines */

#define MAX_CUSTOM_DESC 1500
#define MAX_NOTE_DESC 4096

/* Break these up based on max length allowed - these are NOT flags */
#define LONG_EDIT_DESC 1 /* pedit or hedit long edit */
#define LONG_EDIT_NOTE 2 /* Any kind of note */
#define EDIT_TYPES 2 /* Update this as more are added, should be a count */

#define LONG_EDIT_EDIT 1
#define LONG_EDIT_NEWLINE 2
#define LONG_EDIT_PARAGRAPH 4
#define LONG_EDIT_DELETE 8


/*
 * Connected state for a channel.
 */
#define CON_PLAYING                      0
#define CON_GET_NAME                     1
#define CON_GET_OLD_PASSWORD             2
#define CON_CONFIRM_NEW_NAME             3
#define CON_GET_NEW_PASSWORD             4
#define CON_CONFIRM_NEW_PASSWORD         5
#define CON_GET_NEW_RACE                 6
#define CON_GET_NEW_SEX                  7
#define CON_GET_NEW_CLASS                8
#define CON_GET_ALIGNMENT                9
#define CON_DEFAULT_CHOICE              10 
#define CON_GEN_GROUPS                  11 
#define CON_PICK_WEAPON                 12
#define CON_READ_IMOTD                  13
#define CON_READ_MOTD                   14
#define CON_BREAK_CONNECT               15
#define CON_EDITOR                      16
#define CON_PICK_STATS			17
#define CON_PICK_DEITY                  18
#define CON_GET_OLD_CLASS               19 
#define CON_PREFRESH_CHAR               20 
#define CON_TEMP_SMURF_PASSWORD         21
#define CON_GET_SURNAME		        22
#define CON_PICK_STATS_DEFAULT          23

/* prepended to alloced memory */ 
/* struct mem_data {
   MEM_DATA *next, *prev;
   char perm;
}; */

/*
 * Descriptor (channel) structure.
 */
struct  descriptor_data
{
    DESCRIPTOR_DATA *   next;
    DESCRIPTOR_DATA *   snoop_by;
    CHAR_DATA *         character;
    CHAR_DATA *         original;
    bool                valid;
    char *              host;
    sh_int              descriptor;
    sh_int              connected;
    bool                fcommand;
    char                inbuf           [4 * MAX_INPUT_LENGTH];
    char                incomm          [MAX_INPUT_LENGTH];
    char                inlast          [MAX_INPUT_LENGTH];
    bool                input_received;
    int                 repeat;
    int                 port;
    char *              outbuf;
    char *              name;
    int                 outsize;
    int                 outtop;
    char *              showstr_head;
    char *              showstr_point;
};


struct	improve_type
{
	sh_int	cost;
};

/*
 * Attribute bonus structures.
 */
struct  str_app_type
{
    sh_int      tohit;
    sh_int      todam;
    sh_int      carry;
    sh_int      wield;
};

struct  int_app_type
{
    sh_int      learn;
};

struct  wis_app_type
{
    sh_int      practice;
};

struct  dex_app_type
{
    sh_int      defensive;
};

struct  con_app_type
{
    sh_int      hitp;
    sh_int      shock;
};

struct  agt_app_type
{
    sh_int      reflexive;
};

struct  end_app_type
{
    sh_int      stamina;
};

struct  soc_app_type
{
    sh_int      influence;
};



/*
 * TO types for act.
 */
#define TO_ROOM             0
#define TO_NOTVICT          1
#define TO_VICT             2
#define TO_CHAR             3
#define TO_ALL              4

/*
 * Recipes to create items
 */

struct recipe_data
{
	RECIPE_DATA *next;
        int	     skill_sn;
	int	     difficulty;
	int          recipe_num;
	int	     vnum_container;
	int	     vnum_complete;
	int          vnum_parts[MAX_IN_RECIPE];
};

/*
 * Clan Stat types.
 */
struct cstat_data
{
    CSTAT_DATA * next; 
    sh_int	clan;
    sh_int	kills;
};
    
// New help code
#define HELP_STAT_LEGACY 1
#define HELP_STAT_DRAFT 2
#define HELP_STAT_REVIEW 4
#define HELP_STAT_FINAL 8
#define HELP_STAT_BASICS  15 // For use before saving to strip out temp OLC flags
// Gap for use in future saved stats if needed
#define HELP_STAT_MODIFIED 64
#define HELP_STAT_EDITING 128
#define HELP_STAT_EXTRAS 192 // The ones not included in HELP_STAT_BASICS

/*
 * Help table types.
 */
struct  help_data
{
    HELP_DATA * next;
    sh_int      level;
    char *      keyword;
    char *      text;
    char *      related;
    int         status;
    time_t      modified;
    char *      editor;
};

struct  help_tracker
{
    char *keyword;
    HELP_DATA *help;
    HELP_TRACKER *prev, *next;
};



/*
 * Shop types.
 */
#define MAX_TRADE        5

struct  shop_data
{
    SHOP_DATA * next;                   /* Next shop in list            */
    sh_int      keeper;                 /* Vnum of shop keeper mob      */
    sh_int      buy_type [MAX_TRADE];   /* Item types shop will buy     */
    sh_int      profit_buy;             /* Cost multiplier for buying   */
    sh_int      profit_sell;            /* Cost multiplier for selling  */
    sh_int      open_hour;              /* First opening hour           */
    sh_int      close_hour;             /* First closing hour           */
};

/**
 ** HERBALIST KIT
 **
 ** Herbs have a frequency and source terrain type.
 ** Frequencies are COMMON, UNCOMMON, and RARE.
 ** Terrains where herbs can be gathered are
 ** plains, mountains, hills, desert, and forest.
 ** They also have an addiction rating, 0 being non
 ** addictive, 100 being super-addictive.
 **/

#define	FREQ_COMMON	 0
#define FREQ_UNCOMMON 1
#define FREQ_RARE	 2

struct	herb_type
{
    char	*name;
    int		vnum;
    int		terrain;
    int		frequency;
    int		addiction;
};


/*
 * Per-class stuff.
 */

#define MAX_GUILD       2
#define MAX_STATS       8
#define MAX_SAVES	3
#define SAVE_FORTITUDE	0
#define SAVE_REFLEX	1
#define SAVE_WILLPOWER	2
#define STAT_STR        0
#define STAT_INT        1
#define STAT_WIS        2
#define STAT_DEX        3
#define STAT_CON        4
#define STAT_AGT        5
#define STAT_END        6
#define STAT_SOC        7

struct  class_type
{
    char *      name;                   /* the full name of the class */
    char        who_name        [4];    /* Three-letter name for 'who'  */
    sh_int      attr_prime;             /* Prime attribute              */
    sh_int	attr_second;		/* Secondary attribute 		*/
    sh_int      weapon;                 /* First weapon                 */
    sh_int      guild[MAX_GUILD];       /* Vnum of guild rooms          */
    sh_int      skill_adept;            /* Maximum skill level          */
    sh_int      thac0_00;               /* Thac0 for level  0           */
    sh_int      thac0_32;               /* Thac0 for level 32           */
    sh_int      dam_mod;                /* Damage modifier              */
    sh_int      hp_min;                 /* Min hp gained on leveling    */
    sh_int      hp_max;                 /* Max hp gained on leveling    */
    sh_int      fMana;                  /* Class gains mana on level    */
    bool        reclass;                /* Class only second time around*/
    sh_int	allowed[2];		/* Permitted classes 		*/
    char *      base_group;             /* base skills gained           */
    char *      default_group;          /* default skills gained        */
};

struct deity_type
{
    char	*name;
    char	*pname;
    char	*gifts[MAX_GIFT];
    sh_int	align;
    bool	clan;
};

struct item_type
{
    int         type;
    char *      name;
};

struct material_type
{
    char * 	name;
    sh_int	toughness;
    long	flags;
};

struct weapon_type
{
    char *      name;
    sh_int      vnum;
    sh_int      type;
    sh_int      *gsn;
};

struct wiznet_type
{
    char *      name;
    long        flag;
    int         level;
};

struct pnet_type
{
    char *      name;
    long        flag;
    int         level;
};

struct attack_type
{
    char *      name;                   /* name */
    char *      noun;                   /* message */
    int         damage;                 /* damage class */
};

struct race_type
{
    char *      name;                   /* call name of the race */
    bool        pc_race;                /* can be chosen by pcs */
    long        act;                    /* act bits for the race */
    long        aff;                    /* aff bits for the race */
    long        off;                    /* off bits for the race */
    long        imm;                    /* imm bits for the race */
    long        res;                    /* res bits for the race */
    long        vuln;                   /* vuln bits for the race */
    long        form;                   /* default form flag for the race */
    long        parts;                  /* default parts for the race */
};


struct pc_race_type  /* additional data for pc races */
{
    char *      name;                   /* MUST be in race_type */
    char        who_name[7];
    sh_int      points;                 /* cost in points of the race */
    sh_int      starting_hmv[3];	/* starting Hp Mana moVe */
    sh_int      class_mult[MAX_CLASS];  /* exp multiplier for class, * 100 */
    char *      skills[5];              /* bonus skills for the race */
    sh_int      stats[MAX_STATS];       /* starting stats */
    sh_int      max_stats[MAX_STATS];   /* maximum stats */
    sh_int      size;                   /* aff bits for the race */
};


struct spec_type
{
    char *      name;                   /* special function name */
    SPEC_FUN *  function;               /* the function */
    int         bounty_difficulty;      /* For assigning a bounty to this mob */
};



/*
 * Data structure for notes.
 */

#define NOTE_NOTE       0
#define NOTE_IDEA       1
#define NOTE_PENALTY    2
#define NOTE_NEWS       3
#define NOTE_CHANGES    4
#define NOTE_OOC	5
#define NOTE_BUG	6
#define NOTE_CLAN	7
#define NOTE_IMMORTAL   8	
#define NOTE_QUEST	9

struct  note_data
{
    NOTE_DATA * next;
    bool        valid;
    sh_int      type;
    char *      sender;
    char *      date;
    char *      to_list;
    char *      subject;
    char *      text;
    time_t      date_stamp;
};


/*
 * Damage dealt
 */ 
struct  damage_data
{
  DAMAGE_DATA *      next;
  bool               valid;
  sh_int             duration;
  sh_int             damage;
  sh_int             type;
  /* Add maladiction tracking here eventually */
  char *             source;
  CHAR_DATA          *temp_target;
};

/*
 * An affect.
 */
struct  affect_data
{
    AFFECT_DATA *       next;
    bool                valid;
    sh_int              where;
    sh_int              type;
    sh_int              level;
    sh_int              duration;
    sh_int              location;
    sh_int              modifier;
    long                bitvector;
    long		caster_id;
};

/* where definitions */
#define TO_AFFECTS      0
#define TO_OBJECT       1
#define TO_IMMUNE       2
#define TO_RESIST       3
#define TO_VULN         4
#define TO_WEAPON       5
#define DAMAGE_OVER_TIME	6
#define TO_AFFECTS_EXT	7

/*
 * A kill structure (indexed by level).
 */
struct  kill_data
{
    sh_int              number;
    sh_int              killed;
};



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (Start of section ... start here)                     *
 *                                                                         *
 ***************************************************************************/

/*
 * Well known mob virtual numbers.
 * Defined in #MOBILES.
 */
/** SPELL COMPONENT VNUMS **/
#define VNUM_SPELLCOMP_SILENCE	11147

/** HERBALISM VNUMS **/
#define VNUM_HERB_SILK		0
#define VNUM_HERB_COWSLIP		0
#define VNUM_HERB_HENBANE		0
#define VNUM_HERB_MOSS		0
#define VNUM_HERB_MANTLE		0
#define VNUM_HERB_NETTLE		0
#define VNUM_HERB_CLARY		0
#define VNUM_HERB_VALLEY		0
#define VNUM_HERB_MANDRAKE		0
#define VNUM_HERB_MOUNTAIN		0
#define VNUM_HERB_LEEK		0
#define VNUM_HERB_FLAX		0
#define VNUM_HERB_NEEDLE		0
#define VNUM_HERB_JOSHUA		0
#define VNUM_HERB_NIGHTSHADE		0

#define MOB_VNUM_CLAN_GUARDIAN 14
#define MOB_VNUM_INSANE_MIME 12
#define MOB_VNUM_RAINBOW 13
#define MOB_VNUM_POISON_EATER 3723
#define MOB_VNUM_BOUNTY_ADMIN 8808
#define MOB_VNUM_HASSAN 3011

#define MOB_VNUM_FIDO              3090
#define MOB_VNUM_CITYGUARD         3060
#define MOB_VNUM_VAMPIRE           3404

#define MOB_VNUM_PATROLMAN         2106
#define GROUP_VNUM_TROLLS          2100
#define GROUP_VNUM_OGRES           2101

#define MOB_VNUM_CORPSE            2
#define MOB_VNUM_SKEL_WAR          3
#define MOB_VNUM_MIRROR_IMAGE      4
#define MOB_VNUM_ELEM_FIRE         5
#define MOB_VNUM_ELEM_AIR          6
#define MOB_VNUM_ELEM_WATER        7
#define MOB_VNUM_ELEM_EARTH        8
#define MOB_VNUM_ELEM_SPIRIT       9
#define MOB_VNUM_ELEM_ENERGY       10
/* Line below added 23-AUG-00 for summon_mount spell */
#define MOB_VNUM_WARHORSE          11
/* Line below added 28SEP00 by Boogums for summon_mouse spell */
#define MOB_VNUM_SPYMOUSE          12
#define MOB_VNUM_KING_FIRE         22500 
#define MOB_VNUM_FIRE_1		   22506
#define MOB_VNUM_FIRE_2		   22508
#define MOB_VNUM_FIRE_3		   22509
#define MOB_VNUM_FIRE_SALAMANDER	   22510
#define MOB_VNUM_MAGMAN		   22511
#define MOB_VNUM_WATER_1           22553

#define MOB_VNUM_WATER_2           22554
#define MOB_VNUM_WATER_3           22555
#define MOB_VNUM_ICE_DRAKE         22556
#define MOB_VNUM_WATER_WIERD       22557

// Seven deadly sins, maybe eventually includes cardinal virtues
#define VNUM_SINS_START            20000
#define VNUM_SINS_END              20030

/* RT ASCII conversions -- used so we can have letters in this file */

#define A                       1
#define B                       2
#define C                       4
#define D                       8
#define E                       16
#define F                       32
#define G                       64
#define H                       128

#define I                       256
#define J                       512
#define K                       1024
#define L                       2048
#define M                       4096
#define N                       8192
#define O                       16384
#define P                       32768

#define Q                       65536
#define R                       131072
#define S                       262144
#define T                       524288
#define U                       1048576
#define V                       2097152
#define W                       4194304
#define X                       8388608

#define Y                       16777216
#define Z                       33554432
#define aa                      67108864        /* doubled due to conflicts */
#define bb                      134217728
#define cc                      268435456    
#define dd                      536870912
#define ee                      1073741824

/*
 * SPELL SKILL BIT VECTORS
 * SS_ starts em, enjoy
*/
#define SS_SCRIBE		(A)
#define SS_OUT_OF_ROOM          (B)
#define SS_WAND                 (C)
#define SS_STAFF		(D)





/*
 * Material bits for material table 
 */
#define MAT_ORGANIC		(A)
#define MAT_MINERAL		(B)
#define MAT_METAL		(C)
#define MAT_WOOD		(D)
#define MAT_COMPOSITE		(E)
#define MAT_TEXTILE		(F)
#define MAT_NONCORPOREAL	(G)
#define MAT_LIQUID		(H)

#define MAT_VULN_SOUND		(I)
#define MAT_FLAMMABLE		(J)
#define MAT_CONDUCTOR		(K)
#define MAT_VULN_SHATTER	(L)
#define MAT_VULN_COLD		(M)
#define MAT_VULN_RUST		(N)
#define MAT_VULN_SLICE		(O)

#define MAT_FIREPROOF		(P)
#define MAT_RUSTPROOF		(Q)
#define MAT_SLICEPROOF		(R)
#define MAT_ACIDPROOF		(S)



/*
 * ACT bits for mobs.
 * Used in #MOBILES.
 */
#define ACT_IS_NPC              (A)             /* Auto set for mobs    */
#define ACT_SENTINEL            (B)             /* Stays in one room    */
#define ACT_SCAVENGER           (C)             /* Picks up objects     */
#define ACT_AGGRESSIVE          (F)             /* Attacks PC's         */
#define ACT_STAY_AREA           (G)             /* Won't leave area     */
#define ACT_WIMPY               (H)
#define ACT_PET                 (I)             /* Auto set for pets    */
#define ACT_TRAIN               (J)             /* Can train PC's       */
#define ACT_PRACTICE            (K)             /* Can practice PC's    */
#define ACT_UNDEAD              (O)     
#define ACT_IS_WEAPONSMITH      (P)
#define ACT_CLERIC              (Q)
#define ACT_MAGE                (R)
#define ACT_THIEF               (S)
#define ACT_WARRIOR             (T)
#define ACT_NOALIGN             (U)
#define ACT_NOPURGE             (V)
#define ACT_OUTDOORS            (W)
#define ACT_IS_ARMOURER         (X)
#define ACT_INDOORS             (Y)
#define ACT_MOUNT		(Z)
#define ACT_IS_HEALER           (aa)
#define ACT_GAIN                (bb)
#define ACT_UPDATE_ALWAYS       (cc)
#define ACT_IS_CHANGER          (dd)
#define ACT_NOTRANS		(ee)

/* damage classes */
#define DAM_NONE                0
#define DAM_BASH                1
#define DAM_PIERCE              2
#define DAM_SLASH               3
#define DAM_FIRE                4
#define DAM_COLD                5
#define DAM_LIGHTNING           6
#define DAM_ACID                7
#define DAM_POISON              8
#define DAM_NEGATIVE            9
#define DAM_HOLY                10
#define DAM_ENERGY              11
#define DAM_MENTAL              12
#define DAM_DISEASE             13
#define DAM_DROWNING            14
#define DAM_LIGHT               15
#define DAM_OTHER               16
#define DAM_HARM                17
#define DAM_CHARM               18
#define DAM_SOUND               19
#define DAM_NEUTRAL             20
#define DAM_VULN                21 /* Keep this at the end of the legal damages */

/* alignment ranges for deities */
#define ALIGN_NONE		0
#define ALIGN_EVIL		1
#define ALIGN_NEUTRAL		2
#define ALIGN_GOOD		3

/* OFF bits for mobiles */
#define OFF_AREA_ATTACK         (A)
#define OFF_BACKSTAB            (B)
#define OFF_BASH                (C)
#define OFF_BERSERK             (D)
#define OFF_DISARM              (E)
#define OFF_DODGE               (F)
#define OFF_FADE                (G)
#define OFF_FAST                (H)
#define OFF_KICK                (I)
#define OFF_KICK_DIRT           (J)
#define OFF_PARRY               (K)
#define OFF_RESCUE              (L)
#define OFF_TAIL                (M)
#define OFF_TRIP                (N)
#define OFF_CRUSH               (O)
#define ASSIST_ALL              (P)
#define ASSIST_ALIGN            (Q)
#define ASSIST_RACE             (R)
#define ASSIST_PLAYERS          (S)
#define ASSIST_GUARD            (T)
#define ASSIST_VNUM             (U)
#define OFF_CHARGE              (V)
#define ASSIST_ELEMENT          (W)
#define OFF_BANE_TOUCH          (X)

/* return values for check_imm */
#define IS_NORMAL               0
#define IS_IMMUNE               1
#define IS_RESISTANT            2
#define IS_VULNERABLE           3

/* IMM bits for mobs */
#define IMM_SUMMON              (A)
#define IMM_CHARM               (B)
#define IMM_MAGIC               (C)
#define IMM_WEAPON              (D)
#define IMM_BASH                (E)
#define IMM_PIERCE              (F)
#define IMM_SLASH               (G)
#define IMM_FIRE                (H)
#define IMM_COLD                (I)
#define IMM_LIGHTNING           (J)
#define IMM_ACID                (K)
#define IMM_POISON              (L)
#define IMM_NEGATIVE            (M)
#define IMM_HOLY                (N)
#define IMM_ENERGY              (O)
#define IMM_MENTAL              (P)
#define IMM_DISEASE             (Q)
#define IMM_DROWNING            (R)
#define IMM_LIGHT               (S)
#define IMM_SOUND               (T)
#define IMM_WOOD                (X)
#define IMM_SILVER              (Y)
#define IMM_IRON                (Z)
 
/* RES bits for mobs */
#define RES_SUMMON              (A)
#define RES_CHARM               (B)
#define RES_MAGIC               (C)
#define RES_WEAPON              (D)
#define RES_BASH                (E)
#define RES_PIERCE              (F)
#define RES_SLASH               (G)
#define RES_FIRE                (H)
#define RES_COLD                (I)
#define RES_LIGHTNING           (J)
#define RES_ACID                (K)
#define RES_POISON              (L)
#define RES_NEGATIVE            (M)
#define RES_HOLY                (N)
#define RES_ENERGY              (O)
#define RES_MENTAL              (P)
#define RES_DISEASE             (Q)
#define RES_DROWNING            (R)
#define RES_LIGHT               (S)
#define RES_SOUND               (T)
#define RES_WOOD                (X)
#define RES_SILVER              (Y)
#define RES_IRON                (Z)
#define RES_DELAY		(bb)
 
/* VULN bits for mobs */
#define VULN_SUMMON             (A)
#define VULN_CHARM              (B)
#define VULN_MAGIC              (C)
#define VULN_WEAPON             (D)
#define VULN_BASH               (E)
#define VULN_PIERCE             (F)
#define VULN_SLASH              (G)
#define VULN_FIRE               (H)
#define VULN_COLD               (I)
#define VULN_LIGHTNING          (J)
#define VULN_ACID               (K)
#define VULN_POISON             (L)
#define VULN_NEGATIVE           (M)
#define VULN_HOLY               (N)
#define VULN_ENERGY             (O)
#define VULN_MENTAL             (P)
#define VULN_DISEASE            (Q)
#define VULN_DROWNING           (R)
#define VULN_LIGHT              (S)
#define VULN_SOUND              (T)
#define VULN_WOOD               (X)
#define VULN_SILVER             (Y)
#define VULN_IRON               (Z)
#define VULN_DISTRACTION        (aa)
 
/* body form */
#define FORM_EDIBLE             (A)
#define FORM_POISON             (B)
#define FORM_MAGICAL            (C)
#define FORM_INSTANT_DECAY      (D)
#define FORM_OTHER              (E)  /* defined by material bit */
 
/* actual form */
#define FORM_ANIMAL             (G)
#define FORM_SENTIENT           (H)
#define FORM_UNDEAD             (I)
#define FORM_CONSTRUCT          (J)
#define FORM_MIST               (K)
#define FORM_INTANGIBLE         (L)
 
#define FORM_BIPED              (M)
#define FORM_CENTAUR            (N)
#define FORM_INSECT             (O)
#define FORM_SPIDER             (P)
#define FORM_CRUSTACEAN         (Q)
#define FORM_WORM               (R)
#define FORM_BLOB               (S)
 
#define FORM_MAMMAL             (V)
#define FORM_BIRD               (W)
#define FORM_REPTILE            (X)
#define FORM_SNAKE              (Y)
#define FORM_DRAGON             (Z)
#define FORM_AMPHIBIAN          (aa)
#define FORM_FISH               (bb)
#define FORM_COLD_BLOOD         (cc)    
#define FORM_MINERAL		(dd) 

/* body parts */
#define PART_HEAD               (A)
#define PART_ARMS               (B)
#define PART_LEGS               (C)
#define PART_HEART              (D)
#define PART_BRAINS             (E)
#define PART_GUTS               (F)
#define PART_HANDS              (G)
#define PART_FEET               (H)
#define PART_FINGERS            (I)
#define PART_EAR                (J)
#define PART_EYE                (K)
#define PART_LONG_TONGUE        (L)
#define PART_EYESTALKS          (M)
#define PART_TENTACLES          (N)
#define PART_FINS               (O)
#define PART_WINGS              (P)
#define PART_TAIL               (Q)
/* for combat */
#define PART_CLAWS              (U)
#define PART_FANGS              (V)
#define PART_HORNS              (W)
#define PART_SCALES             (X)
#define PART_TUSKS              (Y)

/*
 * Bits for 'affected_by_ext'.
 */
#define AFF_EXT_BLOODY          (A)
#define AFF_EXT_ANNOINTMENT     (B)
#define AFF_EXT_SHADED          (N)

/*
 * Bits for 'affected_by'.
 * Used in #MOBILES.
 */
#define AFF_BLIND               (A)
#define AFF_INVISIBLE           (B)
#define AFF_DETECT_EVIL         (C)
#define AFF_DETECT_GOOD         (C)
#define AFF_DETECT_ALIGN        (C)
#define AFF_DETECT_INVIS        (D)
#define AFF_DETECT_MAGIC        (E)
#define AFF_DETECT_HIDDEN       (F)
#define AFF_FAERIE_FOG          (G)
#define AFF_SANCTUARY           (H)
#define AFF_FAERIE_FIRE		(I)
#define AFF_INFRARED            (J)
#define AFF_CURSE               (K)
#define AFF_MORPH		(L)
#define AFF_POISON              (M)
#define AFF_PROTECT_EVIL        (N)
#define AFF_PROTECT_GOOD        (O)
#define AFF_SNEAK               (P)
#define AFF_HIDE                (Q)
#define AFF_SLEEP               (R)
#define AFF_CHARM               (S)
#define AFF_FLYING              (T)
#define AFF_PASS_DOOR           (U)
#define AFF_HASTE               (V)
#define AFF_CALM                (W)
#define AFF_PLAGUE              (X)
#define AFF_WEAKEN              (Y)
#define AFF_WEAPONRY		(Z)
#define AFF_BERSERK             (aa)
#define AFF_SWIM                (bb)
#define AFF_REGENERATION        (cc)
#define AFF_SLOW                (dd)
#define AFF_WITHSTAND_DEATH     (ee)

// Bits for room affects (Don't save over reboot)
#define RAFF_NATURE             (A)
#define RAFF_SHADED             (B)

/*
 * Sex.
 * Used in #MOBILES.
 */
#define SEX_NEUTRAL                   0
#define SEX_MALE                      1
#define SEX_FEMALE                    2

/* AC types */
#define AC_PIERCE                       0
#define AC_BASH                         1
#define AC_SLASH                        2
#define AC_EXOTIC                       3

/* dice */
#define DICE_NUMBER                     0
#define DICE_TYPE                       1
#define DICE_BONUS                      2

/* size */
#define SIZE_TINY                       0
#define SIZE_SMALL                      1
#define SIZE_MEDIUM                     2
#define SIZE_LARGE                      3
#define SIZE_HUGE                       4
#define SIZE_GIANT                      5

/* object size */
#define OBJ_OSFA		0
#define OBJ_TINY		1
#define OBJ_SMALL		2
#define OBJ_MEDIUM		3
#define OBJ_LARGE		4
#define OBJ_HUGE		5
#define OBJ_GIANT		6

/*
 * Well known object virtual numbers.
 * Defined in #OBJECTS.
 */
#define OBJ_VNUM_SHADE_STONE 8808
#define OBJ_VNUM_BOUNTY_EAR 8812
#define OBJ_VNUM_SPIRIT_LINK 3298
#define OBJ_VNUM_CUTTING_1 3295
#define OBJ_VNUM_CUTTING_2 3296
#define OBJ_VNUM_CUTTING_3 3297
#define OBJ_VNUM_SCROLL_SILENCE 33
#define OBJ_VNUM_TINYFAVOR 3727
#define OBJ_VNUM_LESSERFAVOR 3294
#define OBJ_VNUM_NORMALFAVOR 9608
#define OBJ_VNUM_GREATERFAVOR 11148
#define OBJ_VNUM_FRACTAL                1206
#define OBJ_VNUM_FEATHER		12063
#define OBJ_VNUM_ASHES			12064
#define OBJ_VNUM_AMULET			12065
#define OBJ_VNUM_NETHER_SHIELD        3037
#define OBJ_VNUM_ROBES			1236
#define OBJ_VNUM_SILVER_ONE           1
#define OBJ_VNUM_GOLD_ONE             2
#define OBJ_VNUM_GOLD_SOME            3
#define OBJ_VNUM_SILVER_SOME          4
#define OBJ_VNUM_COINS                5
#define OBJ_VNUM_FOUNTAIN	      6
#define OBJ_VNUM_FOG		      7
#define OBJ_VNUM_GRAIL                2405
#define OBJ_VNUM_CORPSE_NPC          10
#define OBJ_VNUM_CORPSE_PC           11
#define OBJ_VNUM_SEVERED_HEAD        12
#define OBJ_VNUM_TORN_HEART          13
#define OBJ_VNUM_SLICED_ARM          14
#define OBJ_VNUM_SLICED_LEG          15
#define OBJ_VNUM_GUTS                16
#define OBJ_VNUM_BRAINS              17

#define OBJ_VNUM_MUSHROOM            20
#define OBJ_VNUM_LIGHT_BALL          21
#define OBJ_VNUM_SPRING              22
#define OBJ_VNUM_DISC                23
#define OBJ_VNUM_PORTAL              25
#define OBJ_VNUM_WALL_FIRE           31
#define OBJ_VNUM_WALL_ICE            32
#define OBJ_VNUM_PLAT_BRICK          3142
#define OBJ_VNUM_HOLY_OIL            1266

#define OBJ_VNUM_POTION_ACCURACY	26
#define OBJ_VNUM_POTION_SPEED		27
#define OBJ_VNUM_POTION_MAGIC_RESISTANCE	28
#define OBJ_VNUM_POTION_VISION		3034
#define OBJ_VNUM_POTION_RESTORATION		29
#define OBJ_VNUM_POTION_MOLOTOV		3019

#define OBJ_VNUM_ROSE              1001

#define OBJ_VNUM_BLOOD			34

#define OBJ_VNUM_EGG			3007
#define OBJ_VNUM_BRICK			3142
#define OBJ_VNUM_SHARD	           22044		
#define OBJ_VNUM_DIAMOND		3377
#define OBJ_VNUM_PIT               3010
#define OBJ_VNUM_MATOOK_PIT		15113
#define OBJ_VNUM_DEMISE_PIT		31400
#define OBJ_VNUM_HONOR_PIT		31500
#define OBJ_VNUM_POSSE_PIT		31600
#define OBJ_VNUM_ZEALOT_PIT		31800
#define OBJ_VNUM_WARLOCK_PIT		31900
#define OBJ_VNUM_FLUZEL_PIT		16514

#define OBJ_VNUM_SCHOOL_MACE       3700
#define OBJ_VNUM_SCHOOL_DAGGER     3701
#define OBJ_VNUM_SCHOOL_SWORD      3702
#define OBJ_VNUM_SCHOOL_SPEAR      3717
#define OBJ_VNUM_SCHOOL_STAFF      3718
#define OBJ_VNUM_SCHOOL_AXE        3719
#define OBJ_VNUM_SCHOOL_FLAIL      3720
#define OBJ_VNUM_SCHOOL_WHIP       3721
#define OBJ_VNUM_SCHOOL_POLEARM    3722

#define OBJ_VNUM_SCHOOL_VEST       3703
#define OBJ_VNUM_SCHOOL_SHIELD     3704
#define OBJ_VNUM_SCHOOL_BANNER     3716
#define OBJ_VNUM_MAP               2445 
#define OBJ_VNUM_MAP_BOINGA        2444  

#define OBJ_VNUM_WHISTLE           2116

#define VNUM_FIRE_SEGMENT	   22516
#define VNUM_WATER_SEGMENT         22558
/*
 *Scrolls and inks for the scribe kit
*/
#define OBJ_VNUM_SCRIBE_PARCHMENT  3056
#define OBJ_VNUM_SCRIBE_VELLUM     3055
#define OBJ_VNUM_SCRIBE_PAPYRUS    3048
#define OBJ_VNUM_SCRIBE_RICEPAPER  3038
#define OBJ_VNUM_SCRIBE_KOZO       3049
#define OBJ_VNUM_SCRIBE_INDIGO     3057
#define OBJ_VNUM_SCRIBE_BISTRE    3058
#define OBJ_VNUM_SCRIBE_SEPIA      3059
#define OBJ_VNUM_SCRIBE_DBLOOD     3078

/*wands for wand maker kit*/
#define OBJ_VNUM_WAND_PINE         3128
#define OBJ_VNUM_WAND_APPLE        3141
#define OBJ_VNUM_WAND_OAK          8147
#define OBJ_VNUM_WAND_WILLOW       2397
#define OBJ_VNUM_WAND_YEW          7827

/*staves for the staffmaker kit*/
#define OBJ_VNUM_POOR_STAFF        9604
#define OBJ_VNUM_MEDIUM_STAFF      9605
#define OBJ_VNUM_HIGH_STAFF        9606

#define OBJ_VNUM_WHETSTONE         33
       
/* Maladiction damage values */
#define MALA_LESSER_DAMAGE 4
#define MALA_NORMAL_DAMAGE 7
#define MALA_GREATER_DAMAGE 10

/* Deity favor values */
#define FAVOR_RARITY    4

#define DEITY_FAVOR_ACTIVATE  0
#define DEITY_TRIAL_ACTIVATE  1
#define DEITY_TRIAL_SUCCESS 2
#define DEITY_TRIAL_FAIL_DEATH  3
#define DEITY_TRIAL_FAIL_TIMER  4
#define DEITY_TRIAL_FAIL_PK 5
#define DEITY_TRIAL_FAIL_QUIT 6
#define DEITY_TRIAL_FAIL_ABANDON 7
#define DEITY_TRIAL_FAIL_REQS 8

/* Clan guardian NPC settings */
#define GUARDIAN_ATTACK    1
#define GUARDIAN_ASSIST    2
#define GUARDIAN_FOLLOW    4
#define GUARDIAN_SNEAK     8
#define GUARDIAN_PEACE    16
#define GUARDIAN_SENTINEL 32

/* Quest defines.  Not all quest functions use all defines. */
#define QSTEP_MOVE 1
#define QSTEP_WIN 2
#define QSTEP_DRAW 3
#define QSTEP_LOSE 4
#define QSTEP_QUIT 5
#define QSTEP_NPC_DEAD 6

// Tasks are negative values but use the quest handler - DO NOT STORE IN quest_wins !!!
#define TASK_POISON_EATER -1 
#define TASK_RAINBOW -2
#define TASK_CLAN_GUARDIAN -3

// Quests, increment QUEST_COUNT as more are added
#define QUEST_MIME 0
#define QUEST_BOUNTY_KILL 1
#define QUEST_BOUNTY_CLAIM 2 
#define QUEST_COUNT 3 /* NEVER DECREMENT THIS OR YOU WILL BREAK LOADING */
#define QUEST_UNKNOWN QUEST_COUNT // Yes, this changes - it just means we don't know which quest it is

// These are referenced in this order (Including some logic/math) so if any
// are added be sure to fix/update as needed
#define BOUNTY_MOB_NAME 1
#define BOUNTY_ROOM_NAME 2
#define BOUNTY_ITEM_NAME 3
#define BOUNTY_MOB_DESC 4
#define BOUNTY_ROOM_DESC 5
#define BOUNTY_ITEM_DESC 6

#define BOUNTY_AWARD_STONE 1
#define BOUNTY_AWARD_UPGRADED 2
#define BOUNTY_AWARD_HARD 4
#define BOUNTY_AWARD_DOWNGRADED 8

/* DEBUG CODE - COMMENT OUT BEFORE LIVE */
//#define DEITY_TRIAL_DEBUG_CODE 

#define LINK_MAX 6
#define LINK_NORMAL 5

/*
 * Item types.
 * Used in #OBJECTS.
 */
#define ITEM_LIGHT                    1
#define ITEM_SCROLL                   2
#define ITEM_WAND                     3
#define ITEM_STAFF                    4
#define ITEM_WEAPON                   5
#define ITEM_TREASURE                 8
#define ITEM_ARMOR                    9
#define ITEM_POTION                  10
#define ITEM_CLOTHING                11
#define ITEM_FURNITURE               12
#define ITEM_TRASH                   13
#define ITEM_CONTAINER               15
#define ITEM_DRINK_CON               17
#define ITEM_KEY                     18
#define ITEM_FOOD                    19
#define ITEM_MONEY                   20
#define ITEM_BOAT                    22
#define ITEM_CORPSE_NPC              23
#define ITEM_CORPSE_PC               24
#define ITEM_FOUNTAIN                25
#define ITEM_PILL                    26
#define ITEM_PROTECT                 27
#define ITEM_MAP                     28
#define ITEM_PORTAL                  29
#define ITEM_WARP_STONE              30
#define ITEM_ROOM_KEY                31
#define ITEM_GEM                     32
#define ITEM_JEWELRY                 33
#define ITEM_JUKEBOX                 34
#define ITEM_TRAP                    35
#define ITEM_GRENADE		     36
#define ITEM_MIXER		     37
#define ITEM_COMPONENT		     38
#define ITEM_SPELL_PAGE		     39
#define ITEM_PART                    40
#define ITEM_FORGE		     41
#define ITEM_HERB		     42
#define ITEM_CAPSULE                 43

/*
 * Extra flags.
 * Used in #OBJECTS.
 */
#define ITEM_GLOW               (A)
#define ITEM_HUM                (B)
/*
#define ITEM_DARK               (C)
*/
#define ITEM_IMM_LOAD           (C)
#define ITEM_LOCK               (D)
#define ITEM_EVIL               (E)
#define ITEM_INVIS              (F)
#define ITEM_MAGIC              (G)
#define ITEM_NODROP             (H)
#define ITEM_BLESS              (I)
#define ITEM_ANTI_GOOD          (J)
#define ITEM_ANTI_EVIL          (K)
#define ITEM_ANTI_NEUTRAL       (L)
#define ITEM_NOREMOVE           (M)
#define ITEM_INVENTORY          (N)
#define ITEM_NOPURGE            (O)
#define ITEM_ROT_DEATH          (P)
#define ITEM_VIS_DEATH          (Q)
#define ITEM_NOSAC              (R)
#define ITEM_NONMETAL           (S)
#define ITEM_NOLOCATE           (T)
#define ITEM_MELT_DROP          (U)
#define ITEM_HAD_TIMER          (V)
#define ITEM_SELL_EXTRACT       (W)
#define ITEM_WEAR_TIMER         (X)
#define ITEM_BURN_PROOF         (Y)
#define ITEM_NOUNCURSE          (Z)
#define ITEM_CLAN_CORPSE        (aa)
#define ITEM_WARPED	        (bb)
#define ITEM_TELEPORT		(cc)
#define ITEM_NOIDENTIFY		(dd)

#define ITEM2_TEMP_UNCURSED     (A)

/*
 * Wear flags.
 * Used in #OBJECTS.
 */
#define ITEM_TAKE               (A)
#define ITEM_WEAR_FINGER        (B)
#define ITEM_WEAR_NECK          (C)
#define ITEM_WEAR_BODY          (D)
#define ITEM_WEAR_HEAD          (E)
#define ITEM_WEAR_LEGS          (F)
#define ITEM_WEAR_FEET          (G)
#define ITEM_WEAR_HANDS         (H)
#define ITEM_WEAR_ARMS          (I)
#define ITEM_WEAR_SHIELD        (J)
#define ITEM_WEAR_ABOUT         (K)
#define ITEM_WEAR_WAIST         (L)
#define ITEM_WEAR_WRIST         (M)
#define ITEM_WIELD              (N)
#define ITEM_HOLD               (O)
#define ITEM_NO_SAC             (P)
#define ITEM_WEAR_FLOAT         (Q)
#define ITEM_DRAGGABLE          (R)

/* weapon class */
#define WEAPON_EXOTIC           0
#define WEAPON_SWORD            1
#define WEAPON_DAGGER           2
#define WEAPON_SPEAR            3
#define WEAPON_MACE             4
#define WEAPON_AXE              5
#define WEAPON_FLAIL            6
#define WEAPON_WHIP             7       
#define WEAPON_POLEARM          8
#define WEAPON_GAROTTE          9

/* weapon types */
#define WEAPON_FLAMING          (A)
#define WEAPON_FROST            (B)
#define WEAPON_VAMPIRIC         (C)
#define WEAPON_SHARP            (D)
#define WEAPON_VORPAL           (E)
#define WEAPON_TWO_HANDS        (F)
#define WEAPON_SHOCKING         (G)
#define WEAPON_POISON           (H)
#define WEAPON_STUN             (I)
#define WEAPON_HOLY		(J)
#define WEAPON_FAVORED		(K)
#define WEAPON_NETHER           (L)
#define WEAPON_SCION		(M)
#define WEAPON_RESIST_ENCHANT	(N)

/* gate flags */
#define GATE_NORMAL_EXIT        (A)
#define GATE_NOCURSE            (B)
#define GATE_GOWITH             (C)
#define GATE_BUGGY              (D)
#define GATE_RANDOM             (E)
#define GATE_RANDOM_ROOM        (F)

/* furniture flags */
#define STAND_AT                (A)
#define STAND_ON                (B)
#define STAND_IN                (C)
#define SIT_AT                  (D)
#define SIT_ON                  (E)
#define SIT_IN                  (F)
#define REST_AT                 (G)
#define REST_ON                 (H)
#define REST_IN                 (I)
#define SLEEP_AT                (J)
#define SLEEP_ON                (K)
#define SLEEP_IN                (L)
#define PUT_AT                  (M)
#define PUT_ON                  (N)
#define PUT_IN                  (O)
#define PUT_INSIDE              (P)




/*
 * Apply types (for affects).
 * Used in #OBJECTS.
 */
#define APPLY_NONE                    0
#define APPLY_STR                     1
#define APPLY_DEX                     2
#define APPLY_INT                     3
#define APPLY_WIS                     4
#define APPLY_CON                     5
#define APPLY_SEX                     6
#define APPLY_CLASS                   7
#define APPLY_LEVEL                   8
#define APPLY_AGE                     9
#define APPLY_HEIGHT                 10
#define APPLY_WEIGHT                 11
#define APPLY_MANA                   12
#define APPLY_HIT                    13
#define APPLY_MOVE                   14
#define APPLY_GOLD                   15
#define APPLY_EXP                    16
#define APPLY_AC                     17
#define APPLY_HITROLL                18
#define APPLY_DAMROLL                19
#define APPLY_SAVES                  20
#define APPLY_SAVING_PARA            20
#define APPLY_SAVING_ROD             21
#define APPLY_SAVING_PETRI           22
#define APPLY_SAVING_BREATH          23
#define APPLY_SAVING_SPELL           24
#define APPLY_SPELL_AFFECT           25
#define APPLY_SIZE		     26
#define APPLY_REFLEX_SAVE	     27
#define APPLY_FORTITUDE_SAVE	     28
#define APPLY_WILLPOWER_SAVE	     29
#define APPLY_AGT		     30
#define APPLY_END		     31
#define APPLY_SOC		     32

/*
 * Values for containers (value[1]).
 * Used in #OBJECTS.
 */
#define CONT_CLOSEABLE                1
#define CONT_PICKPROOF                2
#define CONT_CLOSED                   4
#define CONT_LOCKED                   8
#define CONT_PUT_ON                  16



/*
 * Well known room virtual numbers.
 * Defined in #ROOMS.
 */
#define ROOM_VNUM_BOUNTY_PULL     22911
#define ROOM_VNUM_BOUNTY_ROOM      8808
#define ROOM_VNUM_BOUNTY_WATCH     3352
#define ROOM_VNUM_LIMBO               2
#define ROOM_VNUM_HUNTER	   3054
#define ROOM_VNUM_CHAT             1200
#define ROOM_VNUM_DEAD             1212
#define ROOM_VNUM_TEMPLE           3001
#define ROOM_VNUM_ALTAR            3054
#define ROOM_VNUM_SCHOOL           3700
#define ROOM_VNUM_MATOOK    16504
#define ROOM_VNUM_AVARICE	   31700
#define ROOM_VNUM_ZEALOT	   31816
#define ROOM_VNUM_PHOENIX   3001
#define ROOM_VNUM_VALOR     3001
#define ROOM_VNUM_BALANCE          4500
#define ROOM_VNUM_CIRCLE           4400
#define ROOM_VNUM_DEMISE           31422
/* #define ROOM_VNUM_DEMISE           4201 */
#define ROOM_VNUM_HONOR            31520
#define ROOM_VNUM_HONOR_INDEX      31500
#define ROOM_VNUM_ZEALOT_INDEX     31800
#define ROOM_VNUM_WARLOCK_INDEX    31900
#define ROOM_VNUM_POSSE_INDEX      31600
/* #define ROOM_VNUM_HONOR            4300 */
#define ROOM_VNUM_MCLEOD           4600
#define ROOM_VNUM_RIDERS           6900
#define ROOM_VNUM_POSSE            31614
/* #define ROOM_VNUM_POSSE            4701 */
#define ROOM_VNUM_LEGION           4900
#define ROOM_VNUM_WARLOCK          31918
#define ROOM_VNUM_NTFOUNTAIN       3001
#define ROOM_VNUM_SINGLE_GLADIATOR 10820 
#define ROOM_VNUM_TEAM_GLADIATOR 10821 
#define ROOM_VNUM_TEAM_BARBARIAN 10824 


/*
 * Room flags.
 * Used in #ROOMS.
 */
#define ROOM_DARK               (A)
#define ROOM_NO_MOB             (C)
#define ROOM_INDOORS            (D)

#define ROOM_PRIVATE            (J)
#define ROOM_SAFE               (K)
#define ROOM_SOLITARY           (L)
#define ROOM_PET_SHOP           (M)
#define ROOM_NO_RECALL          (N)
#define ROOM_IMP_ONLY           (O)
#define ROOM_GODS_ONLY          (P)
#define ROOM_HEROES_ONLY        (Q)
#define ROOM_NEWBIES_ONLY       (R)
#define ROOM_LAW                (S)
#define ROOM_NOWHERE            (T)
#define ROOM_NODIE              (U)
#define ROOM_NOCLAN             (V)
#define ROOM_NOCOMBAT           (W)
#define ROOM_HOLY_GROUND        (X)
#define ROOM_CLANONLY           (Y)
#define ROOM_ISOLATED           (Z)// New isolated code
#define ROOM_NOHALL             (aa)


/*
 * Directions.
 * Used in #ROOMS.
 */
#define DIR_NORTH                     0
#define DIR_EAST                      1
#define DIR_SOUTH                     2
#define DIR_WEST                      3
#define DIR_UP                        4
#define DIR_DOWN                      5



/*
 * Exit flags.
 * Used in #ROOMS.
 */
#define EX_ISDOOR                     (A)
#define EX_CLOSED                     (B)
#define EX_LOCKED                     (C)
#define EX_PICKPROOF                  (F)
#define EX_NOPASS                     (G)
#define EX_EASY                       (H)
#define EX_HARD                       (I)
#define EX_INFURIATING                (J)
#define EX_NOCLOSE                    (K)
#define EX_NOLOCK                     (L)
#define EX_CONCEALED		      (M)
#define EX_SECRET		      (N)
#define EX_NEW_FORMAT			(O)
#define EX_BLOCKED		      (P)

/*
 * Sector types.
 * Used in #ROOMS.
 */
#define SECT_INSIDE                   0
#define SECT_CITY                     1
#define SECT_FIELD                    2
#define SECT_FOREST                   3
#define SECT_HILLS                    4
#define SECT_MOUNTAIN                 5
#define SECT_WATER_SWIM               6
#define SECT_WATER_NOSWIM             7
#define SECT_UNUSED                   8
#define SECT_AIR                      9
#define SECT_DESERT                  10
#define SECT_MAGELAB_SIMPLE	     11
#define SECT_MAGELAB_INTERMEDIATE    12
#define SECT_MAGELAB_ADVANCED	     13
#define SECT_MAGELAB_SUPERIOR	     14
#define SECT_ALTAR_BASIC  	     15
#define SECT_ALTAR_BLESSED 	     16
#define SECT_ALTAR_ANNOINTED         17
#define SECT_ALTAR_HOLY_GROUND       18
#define SECT_FIRE_PLANE		     19
#define SECT_WATER_PLANE             20
#define SECT_OBS_ROOM		     21
#define SECT_MAGELAB_INCREDIBLE 22
#define SECT_ALTAR_INCREDIBLE 23
#define SECT_MAX		     24


/*
 * Equpiment wear locations.
 * Used in #RESETS.
 */
#define WEAR_HIDDEN                  -2
#define WEAR_NONE                    -1
#define WEAR_LIGHT                    0
#define WEAR_FINGER_L                 1
#define WEAR_FINGER_R                 2
#define WEAR_NECK_1                   3
#define WEAR_NECK_2                   4
#define WEAR_BODY                     5
#define WEAR_HEAD                     6
#define WEAR_LEGS                     7
#define WEAR_FEET                     8
#define WEAR_HANDS                    9
#define WEAR_ARMS                    10
#define WEAR_SHIELD                  11
#define WEAR_ABOUT                   12
#define WEAR_WAIST                   13
#define WEAR_WRIST_L                 14
#define WEAR_WRIST_R                 15
#define WEAR_WIELD                   16
#define WEAR_HOLD                    17
#define WEAR_FLOAT                   18
#define WEAR_SECOND		     19
#define MAX_WEAR                     20



/***************************************************************************
 *                                                                         *
 *                   VALUES OF INTEREST TO AREA BUILDERS                   *
 *                   (End of this section ... stop here)                   *
 *                                                                         *
 ***************************************************************************/

/*
 * Conditions.
 */
#define COND_DRUNK                    0
#define COND_FULL                     1
#define COND_THIRST                   2
#define COND_HUNGER                   3



/*
 * Positions.
 */
#define POS_DEAD                      0
#define POS_MORTAL                    1
#define POS_INCAP                     2
#define POS_STUNNED                   3
#define POS_SLEEPING                  4
#define POS_RESTING                   5
#define POS_SITTING                   6
#define POS_FIGHTING                  7
#define POS_STANDING                  8

/*
 * Race Colors
 */
#define RACE_GREY	1
#define RACE_LIGHT	1
#define RACE_DARK	0

/*
 * Display bits
 */
#define DISP_COMPACT		(A)
#define DISP_BRIEF_DESCR	(B)
#define DISP_BRIEF_COMBAT	(C)
#define DISP_BRIEF_WHOLIST	(D)
#define DISP_BRIEF_EQLIST	(E)
#define DISP_PROMPT		(F)
#define DISP_SHOW_AFFECTS	(G)
#define DISP_DISP_VNUM		(H)
#define DISP_COLOR		(I)
#define DISP_COMBINE		(J)
#define DISP_CODER		(K)
#define DISP_BRIEF_SCAN         (L)
#define DISP_BRIEF_CHAR_DESCR   (M)
#define DISP_SURNAME 		(N)
#define DISP_NOTITLES		(O)
#define DISP_DAMAGE		(P)

/*
 * MHS bits 
 */
#define MHS_OLD_RECLASS		(A)
#define MHS_MUTANT		(B)
#define MHS_HIGHLANDER          (C)
#define MHS_SAVANT		(D)
/* Savant flag is for a Guerrand quest */
#define MHS_SHAPESHIFTED        (E)
#define MHS_ADVISORY_BOARD	(F)
#define MHS_ELEMENTAL           (G)
#define MHS_FADE		(H)
#define MHS_LISTEN		(I)
#define MHS_MOUNT		(J)
#define MHS_NOMOUNT		(K)
#define MHS_SHAPEMORPHED        (L)
#define MHS_PREFRESHED		(M)
#define MHS_NORESCUE            (N)
#define MHS_GLADIATOR           (O)
#define MHS_BANISH              (P)
/* added for call warhorse spell */
#define MHS_WARHORSE            (Q)
#define MHS_KAETH_CLEAN         (R)
#define MHS_WARLOCK_ENEMY       (S)
#define MHS_ZEALOT_ENEMY        (T)
#define MHS_POSSE_ENEMY         (U)
#define MHS_HONOR_ENEMY         (V)
/*2 lines added for necromancer kit by Boogums */
#define MHS_VAMPIRIC_TOUCH      (W)
#define MHS_FULL_SAC            (X)
#define MHS_OLC    	        (Z)
//highest we can go is ee so doing noregen first
#define MHS_CURSE             (aa)
#define MHS_MATOOK_COUNCIL      (bb)

/*
 * Clan flags for clan leader command "sanction"
 */
#define CLAN_NO_HALL            (A)
#define CLAN_NO_REGEN           (B)
#define CLAN_NO_HEALER          (C)
#define CLAN_NO_STORE           (D)
#define CLAN_NO_CHANNEL         (E)
#define CLAN_NO_SKILL_1         (F)
#define CLAN_NO_SKILL_2         (G)
#define CLAN_NO_PORTALS         (H)
#define CLAN_ALLOW_SANC		(I)
#define CLAN_PAB		(J)
#define CLAN_BEEN_ZEALOT        (K)
/* Fine grain control over clan hall building permissions */
#define CLAN_CAN_BUILD  (L) /* Set if any other permissions are set, or to just allow viewing */
#define CLAN_CAN_CREATE (M) /* Can only edit a planned item they just created */
#define CLAN_CAN_EDIT   (N) /* Can edit or delete existing planned items */
#define CLAN_CAN_PLACE  (O) /* Can place a planned item */
#define CLAN_CAN_REMOVE (P) /* Can remove a placed item */

/* Extra option bits */
#define OPT_NOGPROMPT           (A)
#define OPT_REPLYLOCK           (B)
#define OPT_CHANNELSTAMP        (C)
#define OPT_NOSAFELINK            (D)

/*
 * ACT bits for players.
 */
#define PLR_IS_NPC              (A)             /* Don't EVER set.      */

/* RT auto flags */
#define PLR_AUTOPEEK            (B)
#define PLR_AUTOASSIST          (C)
#define PLR_AUTOEXIT            (D)
#define PLR_AUTOLOOT            (E)
#define PLR_AUTOSAC             (F)
#define PLR_AUTOGOLD            (G)
#define PLR_AUTOSPLIT           (H)

/* remort flags */
#define PLR_VAMP		(I)
#define PLR_WERE		(J)
#define PLR_MUMMY               (O)

#define PLR_NOOUTOFRANGE	(K)
#define PLR_WEAPONRY		(L)

/* RT personal flags */
#define PLR_NOWAKE		(M)
#define PLR_HOLYLIGHT           (N)
#define PLR_CANLOOT             (P)
#define PLR_NOSUMMON            (Q)
#define PLR_NOFOLLOW            (R)
#define PLR_NOCANCEL            (S)
#define PLR_NOAUTORECALL	(T)
#define PLR_RECLASS		(cc)
#define PLR_CANCLAN		(dd)
/* penalty flags */
#define PLR_PERMIT              (U)
#define PLR_DWEEB               (V)
#define PLR_LOG                 (W)
#define PLR_DENY                (X)
#define PLR_FREEZE              (Y)
#define PLR_THIEF               (Z)
#define PLR_KILLER              (aa)
#define PLR_THUG		(bb)

/* RT comm flags -- may be used on both mobs and chars */
#define COMM_QUIET              (A)
#define COMM_DEAF               (B)
#define COMM_NOWIZ              (C)
#define COMM_NOAUCTION          (D)
#define COMM_NOGOSSIP           (E)
#define COMM_NOQUESTION         (F)
#define COMM_NOMUSIC            (G)
#define COMM_NOCLAN             (H)
#define COMM_NOQUOTE            (I)
#define COMM_SILENCE            (J)
#define COMM_NOOOC		(cc)
#define COMM_NOGLADIATOR	(L)

/* display flags */
#define COMM_TRUE_TRUST         (K)
#define COMM_TELNET_GA          (P)
#define COMM_NOGRATS            (R)
#define COMM_TELL_BEEP          (S)
#define COMM_BRIEF_MENUS        (aa)
#define COMM_NOBITCH            (T)

/* penalties */
#define COMM_NOEMOTE            (U)
#define COMM_NOSHOUT            (V)
#define COMM_NOTELL             (W)
#define COMM_NOCHANNELS         (X)
#define COMM_NONOTES		(dd)
#define COMM_SNOOP_PROOF        (Y)
#define COMM_AFK                (Z)
#define COMM_IN_OLC             (bb)
#define COMM_NOTITLE		(M)

/* Pnet flags */
#define PNET_ON			(A)
#define PNET_PREFIX		(B)
#define PNET_DEATHS		(C)
#define PNET_BOUNTY		(D)
#define PNET_LEVELS		(E)
#define PNET_LOGINS		(F)
#define PNET_LINKS		(G)
#define PNET_NOTES		(H)
#define PNET_MATOOK		(I)
#define PNET_SHADEBOUNTY        (J)

/* WIZnet flags */
#define WIZ_ON                  (A)
#define WIZ_TICKS               (B)
#define WIZ_LOGINS              (C)
#define WIZ_SITES               (D)
#define WIZ_LINKS               (E)
#define WIZ_DEATHS              (F)
#define WIZ_RESETS              (G)
#define WIZ_MOBDEATHS           (H)
#define WIZ_FLAGS               (I)
#define WIZ_PENALTIES           (J)
#define WIZ_SACCING             (K)
#define WIZ_LEVELS              (L)
#define WIZ_SECURE              (M)
#define WIZ_SWITCHES            (N)
#define WIZ_SNOOPS              (O)
#define WIZ_RESTORE             (P)
#define WIZ_LOAD                (Q)
#define WIZ_NEWBIE              (R)
#define WIZ_PREFIX              (S)
#define WIZ_SPAM                (T)
#define WIZ_TRANSGRESSION       (U)
#define WIZ_OLC                 (V)
#define PLR_RUFFIAN		(W)
#define WIZ_NOTES		(X)
#define WIZ_DEBUG		(aa)
#define WIZ_ALLNOTES		(bb)
#define WIZ_DEITYFAVOR		(cc)
#define WIZ_SHADEBOUNTY         (dd)// 30

// Difficulty descriptions for mob bounties
#define BOUNTY_EASY             1
#define BOUNTY_MEDIUM           2
#define BOUNTY_HARD             3
#define BOUNTY_VERYHARD         4
#define BOUNTY_INSANE           5
#define MAX_BOUNTY_LEVEL        5

/*
 * Prototype for a mob.
 * This is the in-memory version of #MOBILES.
 */
struct  mob_index_data
{
    MOB_INDEX_DATA *    next;
    SPEC_FUN *          spec_fun;
    SHOP_DATA *         pShop;
    AREA_DATA *         area;
    sh_int              vnum;
    sh_int              group;
    bool                new_format;
    sh_int              count;
    sh_int              killed;
    char *              player_name;
    char *              short_descr;
    char *              long_descr;
    char *              description;
    char *              spec_words[3];
    char *              label;
    long                act;
    long                affected_by;
    long                affected_by_ext;
    int                 bounty_level;
    sh_int              alignment;
    sh_int              level;
    sh_int              hitroll;
    sh_int              hit[3];
    sh_int              mana[3];
    sh_int              damage[3];
    sh_int              ac[4];
    sh_int              dam_type;
    long                off_flags;
    long                imm_flags;
    long                res_flags;
    long                vuln_flags;
    sh_int              start_pos;
    sh_int              default_pos;
    sh_int              sex;
    sh_int              race;
    long                wealth;
    sh_int              gold;
    long                form;
    long                parts;
    sh_int              size;
    char *              material;
};



/* memory settings */
#define MEM_CUSTOMER    A       
#define MEM_SELLER      B
#define MEM_HOSTILE     C
#define MEM_AFRAID      D

/* memory for mobs */

/*
 * One character (PC or NPC).
 */
struct  char_data
{
    CHAR_DATA *         next;
    CHAR_DATA *         next_in_room;
    CHAR_DATA *         master;
    CHAR_DATA *         leader;
    CHAR_DATA *         fighting;
    CHAR_DATA *		riding;
    CHAR_DATA *		passenger;
    CHAR_DATA *         reply;
    CHAR_DATA *         pet;    
    CHAR_DATA *         ignoring;    
    CHAR_DATA *         grab_by;    
    CHAR_DATA *         grabbing;    
    SPEC_FUN *          spec_fun;
    MOB_INDEX_DATA *    pIndexData;
    DESCRIPTOR_DATA *   desc;
    AFFECT_DATA *       affected;
    AFFECT_DATA *       flash_affected;
    NOTE_DATA *         pnote;
    OBJ_DATA *          carrying;
    OBJ_DATA *          on;
    ROOM_INDEX_DATA *   in_room;
    ROOM_INDEX_DATA *   was_in_room;
    AREA_DATA *         zone;
    TRADE_DATA *	trade;
    PC_DATA *           pcdata;
    GEN_DATA *          gen_data;
    /*
    int			redid;
    */
    bool                valid;
    char *              name;
    long                id;
    sh_int              version;
    char *              short_descr;
    char *              long_descr;
    char *              description;
    char *              prompt;    
    sh_int              group;
    sh_int		icg;
    long		icg_bits;
    CLAN_DATA *		join;
    sh_int		kit;
    sh_int		species_enemy; /** ranger stuff */
    sh_int		home_terrain;  /*** ranger stuff */
    sh_int              sex;
    sh_int              class;
    sh_int              race;
    sh_int              level;
    sh_int              trust;
    int                 played;
    int                 lines;  /* for the pager */
    time_t              logon;
    sh_int              timer;
    sh_int              wait;
    sh_int              daze;
    sh_int              hit;
    sh_int              max_hit;
    sh_int              mana;
    sh_int              max_mana;
    sh_int              move;
    sh_int              max_move;
    long                gold;
    long                silver;
    int			in_bank;
    int                 exp;
    long                act;
    long		mhs;
    long		display; /* BS display flags */
    long                comm;   /* RT added to pad the vector */
    long                wiznet; /* wiz stuff */
    long                pnet; /* player info stuff */
    long                imm_flags;
    long                res_flags;
    long                vuln_flags;
    sh_int              invis_level;
    sh_int              incog_level;
    long                affected_by;
    long                affected_by_ext;
    sh_int              position;
    sh_int              practice;
    sh_int              train;
    sh_int		skill_points;
    sh_int              carry_weight;
    sh_int              carry_number;
    sh_int              saving_throw;
    sh_int              alignment;
    sh_int              hitroll;
    sh_int              damroll;
    sh_int              armor[4];
    sh_int              wimpy;
    sh_int              life_timer;
    sh_int              trumps;
    sh_int              clan;
    CARD_DATA *		hand[4];
    /* stats */
    sh_int              perm_stat[MAX_STATS];
    sh_int              mod_stat[MAX_STATS];
    /* parts stuff */
    long                form;
    long                parts;
    sh_int              size;
    char*               material;
    /* mobile stuff */
    long                off_flags;
    sh_int              damage[3];
    sh_int              dam_type;
    sh_int              start_pos;
    sh_int              default_pos;
    CHAR_DATA *         lAb;
    /* Shapeshifter Stuff */
    sh_int              save_stat[MAX_STATS];
    sh_int		save_race;
    sh_int		save_con_mod;
    sh_int		saves[MAX_SAVES]; /* 0 = fort, 1 = reflex, 2 = willpower */
    int        qnum; /* For saving script details - sometimes saving state is needed */
    int        qnum2;/* Being in all NPCs allows far more complexity in quests */
    sh_int last_move;
    int alert;
    CHAR_DATA *qchar;
    CHAR_DATA *follower;
    CHAR_DATA *next_groupmate;
    DAMAGE_DATA *damaged;
};


/* edit flags */

struct menu_item 
{
  char *text;
  char *context;
  int id;
  MENU_FUN *menu_fun;
};

#define EDIT_DEFAULT_ROOM     1   /* Default to current or use old */
#define EDIT_DEFAULT_OBJ      2
#define EDIT_DEFAULT_MOB      4
#define EDIT_CREATE_NOTHING   8   /* How much to prompt when creating */
#define EDIT_CREATE_MINIMAL   16
#define EDIT_CREATE_ALL       32
#define EDIT_AUTO_CREATE      64  /* Create rooms as the char walks into walls */
#define EDIT_DOUBLE_DOOR     128  /* Make double sided doors */
#define EDIT_BRIEF_MENUS     256 

struct edit_data {
  AREA_DATA *area;

  ROOM_INDEX_DATA *room;
  OBJ_INDEX_DATA  *obj;
  MOB_INDEX_DATA  *mob;  
  sh_int           exit;
  int              per_flags;       /* personal flags */
  long            *mod_flags;       /* pointer to flags to be modified */
  char           **flag_table;  
  MENU_DATA       *prev_menu;  
  VNUM_RANGE_DATA *range;
};

struct macro_data {
  MACRO_DATA   *next;
  char         *name;
  char         *text;
  sh_int        mark;
};

/* One line in the editor */
struct line_data {
  LINE_DATA *next;
  char      *text;
};

/* Main editor data structure */
struct line_edit_data {
  LINE_EDIT_DATA *next;
  int         cur_line, mode, flags;
  LINE_DATA   *line;  
  MENU_DATA   *prev_menu;
  DO_FUN      *call_back;
};

/*
 * Data which only PC's have.
 */
struct  pc_data
{
    PC_DATA *           next;
    BUFFER *            buffer;
    DO_FUN  *           interp_fun;    
    MENU_DATA *         menu;
    EDIT_DATA           edit;    
    MACRO_DATA *        macro;    
    LINE_EDIT_DATA *    line_edit;
    CHAR_DATA *		req_trade;
    bool                no_out;
    char *		email;
    sh_int              macro_count;
    sh_int		node;
    sh_int		capped;
    sh_int		specialize;
    sh_int		old_class;
    bool                valid;
    char *              pwd;
    char *              bamfin;
    char *              bamfout;
    char *              title;
    char *              who_name;
    char *              last_kill;
    char *		last_attacked_by;
    sh_int              last_attacked_by_timer;
    char *		last_killed_by;
    time_t              last_note;
    time_t              last_idea;
    time_t              last_penalty;
    time_t              last_news;
    time_t              last_changes;
    time_t		last_ooc;
    time_t		last_bug;
    time_t		last_cnote;
    time_t		last_qnote;
    time_t		last_immnote;
    sh_int              perm_hit;
    sh_int              perm_mana;
/*  sh_int              perm_move; */
    long                perm_move;
    sh_int              true_sex;
    sh_int              quit_time;   /* time until quit for flee-quiters */
    sh_int		barbarian;   /* barbarian attack timer */
    sh_int		mutant_timer; 
    sh_int		skill_point_timer; 
    sh_int		skill_point_tracker; 
    sh_int		logout_tracker; 
    sh_int		gladiator_attack_timer; 
    sh_int		gladiator_team; 
    sh_int		savant;
    int                 last_level;
    sh_int              condition       [4];
    sh_int              old_learned     [MAX_SKILL];
    sh_int              learned         [MAX_SKILL];
    sh_int		last_learned	[MAX_SKILL]; /* for color thing */
    bool                group_known     [MAX_GROUP];
    sh_int              points;
    bool                confirm_delete;
    bool		confirm_loner;
    bool 		confirm_outcast;
    char *    alias[MAX_ALIAS];
    char *    alias_sub[MAX_ALIAS];
    sh_int              deity;
    sh_int		new_deity;
    int			switched;
    sh_int              sac;
    sh_int		rank;
    sh_int		debit_level;
    sh_int		outcT;
    sh_int		ruffT;
    sh_int		matookT;
    char		hostmask[80];
#ifdef NEVER_DEFINED
    long                imc_deaf;    /* IMC deaf-to-channel flags */
    long                imc_allow;   /* IMC channel allow overrides */
    long                imc_deny;    /* IMC channel deny overrides */
    char *              rreply;      /* IMC reply-to */
    char *              rreply_name; /* IMC reply-to shown to char */
    char *              ice_listen;  /* ICE listening-to-channels list */
#endif
    time_t		created_date;
    char *		last_host;
    sh_int              second_hitroll;
    sh_int              second_damroll;
    sh_int              save_clan;
    long                clan_flags;
    sh_int		afk_counter;
    char *		surname;
    int			abolish_timer;
    int			wraith_timer;
    int			ambush_timer;
    int			trap_timer;
    int                 deity_timer;
    long                new_opt_flags;
    long		bounty;
/* used in pkill, no kr or stats change if death timer is not = 0 */
    sh_int		last_death_timer; 
    sh_int		combats_since_last_login;
    time_t		last_combat_date;
    sh_int		logins_without_combat;
    time_t		last_kill_date;
    sh_int		logins_without_kill;
    time_t		last_death_date;
    sh_int		logins_without_death;
    bool		died_today;
    bool		killed_today;
    int			killer_data[4];
    long		steal_data[3];
    int                 merit_stolen;
    int			gladiator_data[6];
    int			highlander_data[2];
    CHAR_DATA *  	glad_bet_on;
    int			glad_bet_amt;
    int			glad_tot_bet;
    sh_int		deity_favor;
    sh_int		deity_favor_timer;
    sh_int		deity_trial;
    sh_int		deity_trial_timer;
    sh_int		pref_stat;
    sh_int    half_train; // Half a train
    sh_int    retrain; // Converted trains that can be bought back
    sh_int    half_retrain; // You don't lose converted trains by remorting
    sh_int    trained_hit; // For future use with stat balancing
    sh_int    trained_mana; // For future use with stat balancing
    sh_int    trained_move; // For future use with stat balancing
    sh_int fast_h;
    sh_int fast_m;
    OBJ_DATA *linked[LINK_MAX];/* Set to NULL */
    long edit_flags;
    PLAN_DATA *edit_obj;
    CLAN_CHAR *clan_info;
    PLAN_DATA *rev_obj;
    int rev_type;
    CLAN_DATA *rev_clan;
    bool edits_made;
    int bank_gold;
    bool bypass_blind;/* Blind bypass code */
    int quest_wins[QUEST_COUNT];
    int quest_count;// Number of active quests
    sh_int edit_type;
    int edit_limit;
    int edit_len; /* Index for end of string */
    int edit_count; /* Index for last line entered */
    char *edit_str;
    int timestamps;
    char *timestamp_color;
    int rlock_time;
    int pulse_timer;
    int pulse_type;
    void *pulse_target;
    bool weapon_too_heavy;
    int corpse_timer;
    sh_int old_join;
    int old_c_clan;/* Old clan recording in case of reworking bonus merit */
    int old_c_hours;
    int old_c_rank;
    int bank_eggs;
    int bank_bricks;
 
    /*start_time is to combat ppl using IMs to scout*/
    int                 start_time;
    /*convert_timer is used by the zealot convert skill*/
    int			convert_timer;
};

/* Data for generating characters -- only used during generation */
struct gen_data
{
    GEN_DATA    *next;
    bool        valid;
    bool        skill_chosen[MAX_SKILL];
    bool        group_chosen[MAX_GROUP];
    int         points_chosen;
    int		bonus_points;
};

struct merit_tracker
{
  MERIT_TRACKER *next;
  bool valid;
  int amount;
  int expire;
};

/* No next because this is only attached to a room */
struct plan_exit_data
{
  bool valid;
  PLAN_EXIT_DATA *next; /* For recycling */
  PLAN_DATA *exit;/* The actual exit object if it exists, default exit otherwise */
  PLAN_DATA *link; /* The room it's linked to */
  ROOM_INDEX_DATA *outside;/* Generally NULL */
};

/* All one type because the convenience is worth it for sorting through lists */
/* Some of the variables could be compressed, but the code stays a little more
 * readable by having settings a little more spread out */
struct plan_data
{
  PLAN_DATA *next;
  CLAN_DATA *clan;
  bool valid;
  long type;// Room, mob, item, exit. Planned, placed
  int plan_index; /* Unique for within this hall, locked on creation */
  int cost;
  bool editing; /* Someone is editing it */
  bool reviewed;
  bool flagged;
  char *label; /* Unique identifier */
  char *name;
  char *short_d;
  char *long_d;
  char *desc;
  char *previewer;
  int opt[2];/* Varies based on type */
  long flags;/* Varies based on type */
  void *to_place;/* The actual object if it has been placed */
  int loc;/* If it's placed, the room's index - mob and item only */
  PLAN_EXIT_DATA *exits;/* Array of exits, set ONLY if this is a placed room */
};

struct alliance_data
{
  CLAN_DATA *clan;
  int duration;
  ALLIANCE_DATA *next;
  bool valid;
  bool pending;
  int cost;
  int bribe;
  int to_pay;
  int offer_duration;
  bool split_pay;
};

/* There needs to be a loner and outcast clan tracking so that personal rooms
 * are never lost, unless they completely leave the clan system (Or delete) */
struct clan_data
{
  char *name;
  CLAN_DATA *        next;
  int tribute;// Total unspent
  int max_tribute;// Total ever earned
  int hall_tribute;// Amount built into the hall
  int type;
  bool valid;
  int vnum_min;
  int vnum_max;
  int leaders;
  SPELL_FUN *        clan_skill_1;
  SPELL_FUN *        clan_skill_2;
  SPELL_FUN *        clan_skill_3;
  CLAN_CHAR *        members;
  PLAN_DATA *  hall;
  int hall_index;
  int version;
  PLAN_DATA *        planned;
  char * charter; /* The public charter of the clan */
  char * rules; /* The rules that only clan members may read */
  bool hall_mod; /* Modified is used for backing up the hall */
  bool member_mod; /* Used for backing up the member list if it has changed */
  int default_clan; /* loner/outcast are default clans */
  MERIT_TRACKER *to_match;
  int initiation;
  time_t init_date;
  time_t creation;
  int enemy;
  char color[3];
  long hall_type; /* Can have special types for certain features being added */ 
  int awarded_tribute; /* Awards from imms, doesn't transfer if you merge clans */
  int kills;
  int assists;
  bool inactive;
  ALLIANCE_DATA *allies;
};

struct clan_char
{
  CLAN_CHAR *        next;
  char *             name;
  int                rank;
  time_t             join_date;
  time_t             last_login;
  time_t             invited;
  bool               valid;
  int                merit;
  int                banked_merit; /* Merit in a loner's bank */
  int                donated;// Tribute added from this source
  CHAR_DATA *        player; // Set if the character is online
  long               flags;// Sanctions and permissions are here now -- can be set offline!
  CLAN_DATA       *  clan;
  ROOM_INDEX_DATA *  personal;
  PLAN_DATA *        pers_plan;
  CLAN_DATA *old_clan;
  char *messages;
  int old_donated;
  int rebel_timer;
  int primary_merit;
  int secondary_merit;
  int award_merit;
  MERIT_TRACKER *delay_merit;
  int lost_merit;
};

/* Hall pricing */

struct hall_pricing
{
  sh_int point_cost;
  sh_int egg_cost;
  sh_int value;
};

/*
 * Liquids.
 */
#define LIQ_WATER        0

struct  liq_type
{
    char *      liq_name;
    char *      liq_color;
    sh_int      liq_affect[5];
};



/*
 * Extra description data for a room or object.
 */
struct  extra_descr_data
{
    EXTRA_DESCR_DATA *next;     /* Next in list                     */
    bool valid;
    char *keyword;              /* Keyword in look/examine          */
    char *description;          /* What to see                      */
};

/*
 * Prototype for an object.
 */
struct  obj_index_data
{
    OBJ_INDEX_DATA *    next;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    AREA_DATA *         area;
    bool                new_format;
    char *              name;
    char *              short_descr;
    char *              description;
    char *              label;
    sh_int              vnum;
    sh_int              reset_num;
    char *              material;
    sh_int              item_type;
    long                extra_flags;
    long                extra_flags2;
    long                wear_flags;
    sh_int              level;
    sh_int              condition;
    sh_int              count;
    sh_int              weight;
    sh_int		wear_timer;
    int                 cost;
    int                 value[5];
};


/*
 * Kits 
 */
 struct kit_type
 {
     char		*	name;
     sh_int			cost;
     int			min_stat[MAX_STATS];
     bool			class[MAX_CLASS];
     bool			race[MAX_PC_RACE];
     bool			sex[3];
     long			flags;
     char		*	skills[5];
 };

/*
 * Trade information for MudTrader 1.0
 */
 struct trade_data
 {
     TRADE_DATA *                next;
     bool			 valid;
     CHAR_DATA *                 trader[2];
     OBJ_DATA *                  items[2];
     int                         gold[2];
     int                         silver[2];
     bool                        approved[2];
};


/*
 * One object.
 */
struct  obj_data
{
    OBJ_DATA *          next;
    OBJ_DATA *          next_content;
    OBJ_DATA *          contains;
    OBJ_DATA *          in_obj;
    OBJ_DATA *          on;
    OBJ_DATA *		next_in_trade;
    CHAR_DATA *         carried_by;
    EXTRA_DESCR_DATA *  extra_descr;
    AFFECT_DATA *       affected;
    OBJ_INDEX_DATA *    pIndexData;
    ROOM_INDEX_DATA *   in_room;
    bool                valid;
    bool                enchanted;
    int			warps;
    int			damaged;
    char *              prev_owner;
    char *              owner;
    char *              name;
    char *              short_descr;
    char *              description;
    sh_int              item_type;
    int                 extra_flags;
    int                 extra_flags2;
    int                 wear_flags;
    sh_int              wear_loc;
    sh_int              weight;
    sh_int              wear_timer;
    int                 cost;
    sh_int              level;
    sh_int              condition;
    sh_int              stolen_timer;
    int rarity;
    DAMAGE_DATA         *loot_track;
    OBJ_DATA            *loot_next;
    char                *link_name;
    char *              material;
    sh_int              timer;
    int                 value   [5];
    bool                original;// Hasn't moved from its start spot
};



/*
 * Exit data.
 */
struct  exit_data
{
  EXIT_DATA *next;
  
    union
    {
  ROOM_INDEX_DATA *       to_room;
  sh_int                  vnum;
    } u1;
    long                exit_info;
    sh_int              key;
    char *              keyword;  
};



/*
 * Reset commands:
 *   '*': comment
 *   'M': read a mobile 
 *   'O': read an object
 *   'P': put object in object
 *   'G': give object to mobile
 *   'E': equip object to mobile
 *   'D': set state of door
 *   'R': randomize room exits
 *   'S': stop (end of list)
 */

/*
 * Area-reset definition.
 */
struct  reset_data
{
    RESET_DATA *        next;
    char                command;
    sh_int              arg1;
    sh_int              arg2;
    sh_int              arg3;
    sh_int              arg4;
};



/*
 * Area definition.
 */
struct  area_data
{
    AREA_DATA *         next;
    RESET_DATA *        reset_first;
    RESET_DATA *        reset_last;
    char *              file_name;
    char *              name;
    char *              credits;
    sh_int              age;
    sh_int              nplayer;
    sh_int              low_range;
    sh_int              high_range;    
    bool                empty;
    bool                freeze;
    bool                new_area;
    bool                under_develop;
    bool		no_transport; /* BS for obstacle courses */
    sh_int              min_vnum_room;
    sh_int              max_vnum_room;
    sh_int              min_vnum_obj;
    sh_int              max_vnum_obj;
    sh_int              min_vnum_mob;
    sh_int              max_vnum_mob;    
};



/*
 * Room type.
 */
struct  room_index_data
{
    ROOM_INDEX_DATA *   next;
    CHAR_DATA *         people;
    OBJ_DATA *          contents;
    EXTRA_DESCR_DATA *  extra_descr;
    AREA_DATA *         area;
    AFFECT_DATA *	affected;
    EXIT_DATA *         exit    [6];
    sh_int              obs_target;
    char *              name;
    char *              description;
    char *              owner;
    sh_int              vnum;
    long                room_flags;
    sh_int              light;
    sh_int              sector_type;
    sh_int              heal_rate;
    sh_int              mana_rate;
    sh_int              clan;
    char *              label;
    long                room_affects;
};



/*
 * Types of attacks.
 * Must be non-overlapping with spell/skill types,
 * but may be arbitrary beyond that.
 */
#define TYPE_DOT			-3
 #define TYPE_SECONDARY			-2
#define TYPE_UNDEFINED               -1
#define TYPE_HIT                     1000


/*
 * Trap types
 */
#define TRAP_SNARE			0
#define TRAP_ALARM			1
#define TRAP_CLAW			2


/*
 * Apply Types
 */
#define APPLY_PRIMARY			0
#define APPLY_SECONDARY			1
#define APPLY_BOTH		 	2	

/*
 *  Target types.
 */
#define TAR_IGNORE                  0
#define TAR_CHAR_OFFENSIVE          1
#define TAR_CHAR_DEFENSIVE          2
#define TAR_CHAR_SELF               3
#define TAR_OBJ_INV                 4
#define TAR_OBJ_CHAR_DEF            5
#define TAR_OBJ_CHAR_OFF            6

#define TARGET_CHAR                 0
#define TARGET_OBJ                  1
#define TARGET_ROOM                 2
#define TARGET_NONE                 3

#ifdef NEVER_VERSION

/*
 * Skills include spells as a particular case.
 */
struct  skill_type
{
    char *      name;                   /* Name of skill                */
    sh_int      skill_level[MAX_CLASS]; /* Level needed by class        */
    sh_int      rating[MAX_CLASS];      /* How hard it is to learn      */      
    SPELL_FUN * spell_fun;              /* Spell pointer (for spells)   */
    sh_int      target;                 /* Legal targets                */
    sh_int      minimum_position;       /* Position for caster / user   */
    sh_int *    pgsn;                   /* Pointer to associated gsn    */
    sh_int      slot;                   /* Slot for #OBJECT loading     */
    sh_int      min_mana;               /* Minimum mana used            */
    sh_int      beats;                  /* Waiting time after use       */
    char *      noun_damage;            /* Damage message               */
    char *      msg_off;                /* Wear off message             */
    char *      msg_obj;                /* Wear off message for obects  */
};
#endif

/*
 * Skills include spells as a particular case.
 */
struct  skill_type
{
    char *      name;                   /* Name of skill                */
    sh_int      skill_level[MAX_CLASS]; /* Level needed by class        */
    sh_int      rating[MAX_CLASS];      /* How hard it is to learn      */
    SPELL_FUN * spell_fun;              /* Spell pointer (for spells)   */
    sh_int      target;                 /* Legal targets                */
    sh_int      minimum_position;       /* Position for caster / user   */
    sh_int *    pgsn;                   /* Pointer to associated gsn    */
    sh_int      slot;                   /* Slot for #OBJECT loading     */
    sh_int      min_mana;               /* Minimum mana used            */
    sh_int      beats;                  /* Waiting time after use       */
    char *      noun_damage;            /* Damage message               */
    char *      msg_off;                /* Wear off message             */
    char *      msg_obj;                /* Wear off message for obects  */
    long        bitvector;              /* For the Bit Vectors          */ 
    double      flags;                  /* Affect flags */
};


struct  group_type
{
    char *      name;
    sh_int      rating[MAX_CLASS];
    char *      spells[MAX_IN_GROUP];
};

#define VOIDSIG void
extern VOIDSIG dummy();

/*
 * These are skill_lookup return values for common skills and spells.
 */

extern  sh_int  gsn_light_blast;
extern  sh_int  gsn_shaded_room;
extern  sh_int  gsn_sunburst;
extern	sh_int	gsn_arcantic_lethargy;
extern	sh_int	gsn_arcantic_alacrity;
extern	sh_int	gsn_clarity;
extern	sh_int	gsn_spirit_of_boar;
extern	sh_int	gsn_midnight_cloak;
extern  sh_int          gsn_shield_of_thorns;
extern  sh_int          gsn_shield_of_brambles;
extern  sh_int          gsn_shield_of_spikes;
extern  sh_int          gsn_shield_of_blades;
extern	sh_int	gsn_herbalism;
extern	sh_int	gsn_spirit_of_bear;
extern	sh_int	gsn_spirit_of_cat;
extern	sh_int	gsn_spirit_of_owl;
extern	sh_int	gsn_spirit_of_wolf;
 extern sh_int  gsn_symbol_1;
 extern sh_int  gsn_symbol_2;
 extern sh_int  gsn_symbol_3;
 extern sh_int   gsn_symbol_4;
 extern sh_int	gsn_shield_of_faith;
 extern sh_int	gsn_stone_skin;
extern sh_int	gsn_hamstring;
extern	sh_int	gsn_acclimate;
 extern sh_int	gsn_enhanced_critical;
 extern sh_int	gsn_cleave;
extern sh_int   gsn_connive;
 extern sh_int  gsn_bludgeon;
 extern sh_int	gsn_steel_skin;
 extern sh_int	gsn_diamond_skin;
 extern sh_int	gsn_adamantite_skin;
 extern sh_int	gsn_alchemy;
 extern sh_int	gsn_convert;
 extern sh_int	gsn_annointment;
 extern sh_int	gsn_guardian;
 extern sh_int	gsn_hemorrhage;
 extern sh_int  gsn_aura_of_valor;
 extern sh_int	gsn_riding;
 extern sh_int	gsn_rage;
 extern sh_int	gsn_bladesong;
 extern	sh_int	gsn_cutpurse;
 extern sh_int	gsn_espionage;
 extern sh_int  gsn_dust_storm;
 extern sh_int	gsn_holy_chant;
 extern	sh_int	gsn_ninjitsu;
 extern sh_int	gsn_kurijitsu;
 extern	sh_int	gsn_imbue;
 extern sh_int	gsn_endurance;
 extern sh_int	gsn_barbarian_rage;
 extern sh_int	gsn_forget;
 extern sh_int 	gsn_enervation;
 extern sh_int	gsn_fumble;
extern sh_int   gsn_honor_guard;
 extern sh_int	gsn_focus;
 extern sh_int	gsn_blur;
 extern	sh_int	gsn_spellcraft;
 extern	sh_int	gsn_garotte;
extern sh_int	gsn_grab;
extern sh_int	gsn_sacred_guardian;
extern sh_int	gsn_dual_wield;
extern sh_int	gsn_infiltrate;
extern sh_int	gsn_tumbling;
extern sh_int   gsn_cutpurse;
extern	sh_int	gsn_dae_tok;
extern sh_int   gsn_infuse;
extern  sh_int  gsn_endow;
extern  sh_int  gsn_backstab;
extern  sh_int  gsn_dodge;
extern  sh_int  gsn_envenom;
extern  sh_int  gsn_hide;
extern  sh_int  gsn_peek;
extern  sh_int  gsn_pick_lock;
extern  sh_int  gsn_sneak;
extern  sh_int  gsn_scan;
extern  sh_int  gsn_snatch;
extern	sh_int  gsn_slice;
extern  sh_int  gsn_steal;
extern  sh_int  gsn_bump;
extern  sh_int	gsn_trap;
/* Line below added 29AUG00 by Boogums */
extern  sh_int  gsn_kcharge;
extern sh_int   gsn_sharpen;
extern sh_int   gsn_scribe;
extern sh_int   gsn_wall_fire;
extern sh_int   gsn_wall_ice;
extern sh_int   gsn_hydrophilia;

extern  sh_int  gsn_disarm;
extern  sh_int  gsn_enhanced_damage;
extern  sh_int  gsn_kick;
extern  sh_int  gsn_parry;
extern  sh_int  gsn_rescue;
extern  sh_int  gsn_second_attack;
extern  sh_int  gsn_third_attack;
extern  sh_int	gsn_fourth_attack;

extern  sh_int  gsn_blindness;
extern  sh_int  gsn_charm_person;
extern  sh_int  gsn_curse;
extern  sh_int  gsn_invis;
extern  sh_int  gsn_mass_invis;
extern  sh_int  gsn_plague;
extern  sh_int  gsn_poison;
extern  sh_int  gsn_sleep;
extern  sh_int  gsn_sanctuary;
extern  sh_int  gsn_withstand_death;

/* new gsns */
extern sh_int  gsn_axe;
extern sh_int  gsn_dagger;
extern sh_int  gsn_flail;
extern sh_int  gsn_mace;
extern sh_int  gsn_polearm;
extern sh_int  gsn_shield_block;
extern sh_int  gsn_spear;
extern sh_int  gsn_sword;
extern sh_int  gsn_whip;
 
extern sh_int  gsn_bash;
extern sh_int  gsn_berserk;
extern sh_int  gsn_bite;
extern sh_int  gsn_bleed;
extern sh_int  gsn_breathe;
extern sh_int  gsn_dirt;
extern sh_int  gsn_fear;
extern sh_int  gsn_hand_to_hand;
extern sh_int  gsn_hex;
extern sh_int  gsn_trip;
 
extern sh_int  gsn_fast_healing;
extern sh_int  gsn_haggle;
extern sh_int  gsn_lore;
extern sh_int  gsn_meditation;
extern sh_int  gsn_morph;
extern sh_int  gsn_communion;
extern sh_int	gsn_nethermancy;
extern sh_int   gsn_nether_shield;
extern sh_int	gsn_fade;
extern sh_int   gsn_cuffs_of_justice;
extern sh_int  gsn_spell_restrain;
extern sh_int	gsn_magic_resistance;
extern sh_int	gsn_vision;

extern sh_int  gsn_scrolls;
extern sh_int  gsn_staves;
extern sh_int  gsn_wands;
extern sh_int  gsn_swim;
extern sh_int  gsn_recall;
extern sh_int  gsn_chi;
extern sh_int  gsn_throw;
extern sh_int  gsn_kailindo;
extern sh_int  gsn_weave_resistance;
extern sh_int  gsn_insanity;
extern sh_int  gsn_vorpal;
extern sh_int	gsn_wound_transfer;
extern sh_int	gsn_protect_neutral;
extern sh_int	gsn_water_breathing;
extern sh_int	gsn_stonefist;
extern sh_int	gsn_earthbind;
extern sh_int	gsn_irradiate;
extern sh_int	gsn_asphyxiate;
extern sh_int   gsn_confusion;
extern sh_int   gsn_cone_of_silence;
extern sh_int gsn_blade_barrier;
/*
 * Utility macros.
 */
#define IS_VALID(data)          ((data) != NULL && (data)->valid)
#define VALIDATE(data)          ((data)->valid = TRUE)
#define INVALIDATE(data)        ((data)->valid = FALSE)
#define UMIN(a, b)              ((a) < (b) ? (a) : (b))
#define UMAX(a, b)              ((a) > (b) ? (a) : (b))
#define URANGE(a, b, c)         ((b) < (a) ? (a) : ((b) > (c) ? (c) : (b)))
#define LOWER(c)                ((c) >= 'A' && (c) <= 'Z' ? (c)+'a'-'A' : (c))
#define UPPER(c)                ((c) >= 'a' && (c) <= 'z' ? (c)+'A'-'a' : (c))
#define IS_SET(flag, bit)       ((flag) & (bit))
#define SET_BIT(var, bit)       ((var) |= (bit))
#define REMOVE_BIT(var, bit)    ((var) &= ~(bit))
#define TOGGLE_BIT(var,bit)     ((var) ^= (bit))



/*
 * Character macros.
 */
#define HAS_MHS(ch,flag)	(IS_SET((ch)->mhs,(flag)))
#define HAS_KIT(ch,ikit)	((ch)->kit == (kit_lookup(ikit)))
#define IS_NPC(ch)              (IS_SET((ch)->act, ACT_IS_NPC))
#define IS_IMMORTAL(ch)         (get_trust(ch) >= LEVEL_IMMORTAL)
#define IS_HERO(ch)             (get_trust(ch) >= LEVEL_HERO)
#define IS_TRUSTED(ch,level)    (get_trust((ch)) >= (level))
#define IS_AFFECTED(ch, sn)     (IS_SET((ch)->affected_by, (sn)))

#define GET_AGE(ch)             ((int) (17 + ((ch)->played \
            + current_time - (ch)->logon )/72000))

#define COLOR_ON(ch)		(IS_SET((ch)->display,DISP_COLOR))
#define IS_CAPPED(ch)		(IS_NPC((ch)) ? FALSE : (ch)->pcdata->capped)
#define IS_GOOD(ch)             (ch->alignment >= 350)
#define IS_EVIL(ch)             (ch->alignment <= -350)
#define IS_NEUTRAL(ch)          (!IS_GOOD(ch) && !IS_EVIL(ch))

#define IS_AWAKE(ch)            (ch->position > POS_SLEEPING)
#define GET_AC(ch,type)         ((ch)->armor[type]                          \
      + ( IS_AWAKE(ch)                            \
      ? dex_app[get_curr_stat(ch,STAT_DEX)].defensive : 0 ))  
#define GET_HITROLL(ch) \
    ((ch)->hitroll+str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_DAMROLL(ch) \
    ((ch)->damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)
#define GET_SECOND_HITROLL(ch) \
    ((ch)->pcdata->second_hitroll+str_app[get_curr_stat(ch,STAT_STR)].tohit)
#define GET_SECOND_DAMROLL(ch) \
    ((ch)->pcdata->second_damroll+str_app[get_curr_stat(ch,STAT_STR)].todam)

#define IS_OUTSIDE(ch)          (!IS_SET(                                   \
            (ch)->in_room->room_flags,              \
            ROOM_INDOORS))

/* #define WAIT_STATE(ch, npulse)  ((ch)->wait = UMAX((ch)->wait, (npulse))) */
#define WAIT_STATE	wait_state
#define DAZE_STATE(ch, npulse)  ((ch)->daze = UMAX((ch)->daze, (npulse)))
#define get_carry_weight(ch)    ((ch)->carry_weight + (ch)->silver/10 +  \
                  (ch)->gold * 2 / 5)



/*
 * Object macros.
 */
#define CAN_WEAR(obj, part)     (IS_SET((obj)->wear_flags,  (part)))
#define IS_OBJ_STAT(obj, stat)  (IS_SET((obj)->extra_flags, (stat)))
#define IS_WEAPON_STAT(obj,stat)(IS_SET((obj)->value[4],(stat)))
#define WEIGHT_MULT(obj)        ((obj)->item_type == ITEM_CONTAINER ? \
  (obj)->value[4] : 100)


/*
 * Description macros.
 */
#define PERS(ch, looker, ooc)  ( can_see( looker, ch, ooc ) ? (IS_NPC(ch) ? ch->short_descr : ch->name) : (IS_IMMORTAL(ch) ? "an Immortal" : "someone") )

/*
 * Structure for a social in the socials table.
 */
struct  social_type
{
    char      name[20];
    char *    char_no_arg;
    char *    others_no_arg;
    char *    char_found;
    char *    others_found;
    char *    vict_found;
    char *    char_not_found;
    char *      char_auto;
    char *      others_auto;
};



/*
 * Global constants.
 */

extern	const	struct	hall_pricing	price_table     []; 
extern  const   struct  str_app_type    str_app         [26];
extern  const   struct  int_app_type    int_app         [26];
extern  const   struct  wis_app_type    wis_app         [26];
extern  const   struct  dex_app_type    dex_app         [26];
extern  const   struct  con_app_type    con_app         [26];
extern  const   struct  agt_app_type    agt_app         [26];
extern  const   struct  end_app_type    end_app         [26];
extern  const   struct  soc_app_type    soc_app         [26];


extern	const	struct	improve_type	improve_table	[];
extern	const	struct  kit_type	kit_table	[];
extern  const   struct  class_type      class_table     [MAX_CLASS];
extern	const	struct	deity_type	deity_table     [];
extern  const   struct  weapon_type     weapon_table    [];
extern  const   struct  item_type       item_table      [];
extern  const   struct  wiznet_type     wiznet_table    [];
extern  const   struct  pnet_type       pnet_table      [];
extern  const   struct  attack_type     attack_table    [];
extern  const   struct  race_type       race_table      [];
extern  const   struct  pc_race_type    pc_race_table   [];
extern  const   struct  material_type	material_table  [];
extern  const   struct  spec_type       spec_table      [];
extern  const   struct  liq_type        liq_table       [];
extern  const   struct  skill_type      skill_table     [MAX_SKILL];
extern  const   struct  group_type      group_table     [MAX_GROUP];
extern          struct social_type      social_table    [MAX_SOCIALS];
/* extern  char *  const                title_table     [MAX_CLASS] [MAX_LEVEL+1] [2];
*/


/*
 * Global variables.
 */
extern CLAN_DATA *clan_first;
extern          CSTAT_DATA        *     cstat_first;
extern          HELP_DATA         *     help_first;
extern          HELP_DATA         *     help_last;// New help code
extern          HELP_TRACKER      *     help_track_first;
extern          HELP_TRACKER      *     help_tracks     [];
extern          SHOP_DATA         *     shop_first;

extern          CHAR_DATA         *     char_list;
extern          DESCRIPTOR_DATA   *     descriptor_list;
extern          OBJ_DATA          *     object_list;

extern          char                    bug_buf         [];
extern          char                    dns_buf         [];
extern          time_t                  current_time;
extern          bool                    fLogAll;
//extern          FILE *                  fpReserve;
extern          KILL_DATA               kill_table      [];
extern          char                    log_buf         [];
extern          TIME_INFO_DATA          time_info;
extern          WEATHER_DATA            weather_info;
extern          int			cfunds[MAX_CLAN];
extern		int			override;

extern sh_int      posse_kills;
extern sh_int      honor_kills;
extern sh_int      warlock_kills;
extern sh_int      demise_kills;
extern sh_int      avarice_kills;
extern sh_int      zealot_kills;
extern sh_int      honor_demise_kills;
extern sh_int      posse_killer_kills;
extern sh_int      posse_thief_kills;
extern sh_int      posse_ruffian_kills;
extern sh_int      posse_thug_kills;

/*
 * OS-dependent declarations.
 * These are all very standard library functions,
 *   but some systems have incomplete or non-ansi header files.
 */
#if     defined(_AIX)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(apollo)
int     atoi            args( ( const char *string ) );
void *  calloc          args( ( unsigned nelem, size_t size ) );
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(hpux)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(linux)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(macintosh)
#define NOCRYPT
#if     defined(unix)
#undef  unix
#endif
#endif

#if     defined(MIPS_OS)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(MSDOS)
#define NOCRYPT
#if     defined(unix)
#undef  unix
#endif
#endif

#if     defined(NeXT)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif

#if     defined(sequent)
char *  crypt           args( ( const char *key, const char *salt ) );
int     fclose          args( ( FILE *stream ) );
int     fprintf         args( ( FILE *stream, const char *format, ... ) );
/*int     fread           args( ( void *ptr, int size, int n, FILE *stream 
) );*/
int     fseek           args( ( FILE *stream, long offset, int ptrname ) );
void    perror          args( ( const char *s ) );
int     ungetc          args( ( int c, FILE *stream ) );
#endif

#if     defined(sun)
char *  crypt           args( ( const char *key, const char *salt ) );
int     fclose          args( ( FILE *stream ) );
int     fprintf         args( ( FILE *stream, const char *format, ... ) );
#if     defined(SYSV)
/*siz_t   fread           args( ( void *ptr, size_t size, size_t n, 
          FILE *stream) );*/
#else/*
int     fread           args( ( void *ptr, int size, int n, FILE *stream 
) );*/ 
#endif
int     fseek           args( ( FILE *stream, long offset, int ptrname ) );
void    perror          args( ( const char *s ) );
int     ungetc          args( ( int c, FILE *stream ) );
#endif

#if     defined(ultrix)
char *  crypt           args( ( const char *key, const char *salt ) );
#endif



/*
 * The crypt(3) function is not available on some operating systems.
 * In particular, the U.S. Government prohibits its export from the
 *   United States to foreign countries.
 * Turn on NOCRYPT to keep passwords in plain text.
 */
#if     defined(NOCRYPT)
#define crypt(s1, s2)   (s1)
#endif



/*
 * Data files used by the server.
 *
 * AREA_LIST contains a list of areas to boot.
 * All files are read in completely at bootup.
 * Most output files (bug, idea, typo, shutdown) are append-only.
 *
 * The NULL_FILE is held open so that we have a stream handle in reserve,
 *   so players can go ahead and telnet to all the other descriptors.
 * Then we close it whenever we need to open a file (e.g. a save file).
 */
#if defined(macintosh)
#define PLAYER_DIR      ""              /* Player files                 */
#define TEMP_FILE       "romtmp"
#define TEMP_FILE2      "romtmp2"
#define NULL_FILE       "proto.are"             /* To reserve one stream        */
#endif

#define alloc_mem GC_MALLOC

#if defined(MSDOS)
#define PLAYER_DIR      "../player/"    /* Player files */
#define GOD_DIR         "../gods/"      /* list of gods */
#define COLL_DIR        "../collated/"  /* collated areas output */
#define NEW_DIR         "../newareas/"  /* new areas */
#define TEMP_FILE       "../player/romtmp"
#define NULL_FILE       "nul"           /* To reserve one stream        */
#endif

#if defined(unix)
#define PLAYER_DIR      "../player/"    /* Player files */
#define GOD_DIR         "../gods/"      /* list of gods */
#define COLL_DIR        "../collated/"  /* collated areas output */
#define CLAN_DIR        "../clan/"      /* clan stuff */
#define CLAN_BAK_DIR        "../clanbak/"      /* clan stuff */
#define NEW_DIR         "../newareas/"  /* new areas */
#ifdef OLC_VERSION
#define TEMP_FILE       "../player/mhstmp"
#define TEMP_FILE2      "../player/mhstmp2"
#else
#define TEMP_FILE       "../player/romtmp"
#define TEMP_FILE2      "../player/mhstmp2"
#endif
#define NULL_FILE       "/dev/null"     /* To reserve one stream        */
#endif

#ifdef OLC_VERSION // New help code
#define HELP_FILE   "../olc/olcarea/new_helps.are"
#define HELP_BAK    "../olc/olcarea/new_helps.bak"
#else
#define HELP_FILE   "../area/new_helps.are"
#define HELP_BAK   "../area/new_helps.bak"
#ifdef ANDARONDEV
#define HELP_FILE_OLC "/mud/moosehead/olc/olcarea/new_helps.are"
#else
#define HELP_FILE_OLC "../olc/olcarea/new_helps.are"
#endif
#endif

#ifdef OLC_VERSION
#define AREA_LIST       "../area/area.lst"  /* List of areas*/
#else
#define AREA_LIST       "../area/area.act"  /* List of areas*/
#endif
#define BUG_FILE        "../area/bugs.not" /* For 'bug' and bug()*/
#define TYPO_FILE       "../area/typos.txt" /* For 'typo'*/
#define NOTE_FILE       "../area/notes.not"/* For 'notes'*/
#define IDEA_FILE       "../area/ideas.not"
#define PENALTY_FILE    "../area/penal.not"
#define CLAN_FILE	"../area/clan.not"
#define IMMORTAL_FILE	"../area/immortal.not"
#define NEWS_FILE       "../area/news.not"
#define CHANGES_FILE    "../area/chang.not"
#define OOC_FILE	"../area/ooc.not"
#define QUEST_FILE	"../area/quest.not"
#define SHUTDOWN_FILE   "../area/shutdown.txt"/* For 'shutdown'*/
#define BAN_FILE        "../area/ban.txt" /* for bans */
#define DNS_FILE        "../area/dns.txt" /* for bad DNS servers */
#define MUSIC_FILE      "../area/music.txt" /* for lyrics */
#define OLC_LOG_FILE    "../log/olc.log"

/*
 * Our function prototypes.
 * One big lump ... this is every function in Merc.
 */
#define CD      CHAR_DATA
#define MID     MOB_INDEX_DATA
#define OD      OBJ_DATA
#define OID     OBJ_INDEX_DATA
#define RID     ROOM_INDEX_DATA
#define SF      SPEC_FUN
#define AD      AFFECT_DATA

#define TIMESTAMP_MASK 63
#define TIMESTAMP_GLOBAL 64
#define TIMESTAMP_TELLS 128
#define TIMESTAMP_SHOW  256
#define TIMESTAMP_UPPERMASK 64 | 128 | 256

/* act_comm.c */
bool    send_timestamps args( ( CHAR_DATA *ch, bool send_now, bool global) );
bool	check_parse_name	args( ( char * arg ) );
bool	check_parse_surname	args( ( char * arg ) );
void    check_sex       args( ( CHAR_DATA *ch) );
void    add_follower    args( ( CHAR_DATA *ch, CHAR_DATA *master ) );
void    stop_follower   args( ( CHAR_DATA *ch ) );
void    nuke_pets       args( ( CHAR_DATA *ch ) );
void    die_follower    args( ( CHAR_DATA *ch ) );
void    die_ignore      args( ( CHAR_DATA *ch ) );
bool    is_same_group   args( ( CHAR_DATA *ach, CHAR_DATA *bch ) );
bool	group_has_crusader args( ( CHAR_DATA *ch ) );
int	group_has_how_many_crusader args( ( CHAR_DATA *ch ) );
bool    group_has_cavalier      args( ( CHAR_DATA *ch ) ); 
void	channel_vis_status	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void remove_from_group args((CHAR_DATA *ch));
void add_to_group args((CHAR_DATA *leader, CHAR_DATA *victim));

/* act_enter.c */
RID  *get_random_room   args ( (CHAR_DATA *ch) );
RID  *get_random_room_obj(void);

/* act_info.c */
void    set_title       args( ( CHAR_DATA *ch, char *title ) );
void    show_list_to_char       args( ( OBJ_DATA *list, CHAR_DATA *ch,
             bool fShort, bool fShowNothing , bool fExpand) );
void	do_surname	args( ( CHAR_DATA *ch, char *argument) );
void do_look( CHAR_DATA *ch, char *argument );

/* act_move.c */
bool	is_abolishable	args( ( AFFECT_DATA *af ) );
void    move_char       args( ( CHAR_DATA *ch, int door, bool follow ) );
extern	char	kludge_string[MAX_STRING_LENGTH];
void 	fade	args( ( CHAR_DATA *ch, char *argument ) );

/* act_obj.c */
void unlink_item args((CHAR_DATA *ch, OBJ_DATA *obj));
void do_link args((CHAR_DATA *ch, char *argument));
void do_unlink args((CHAR_DATA *ch, char *argument));
void do_linksafe args((CHAR_DATA *ch, char *argument));
void  steal	args( ( CHAR_DATA *ch, char *arg1, CHAR_DATA *victim) );
bool can_loot           args( (CHAR_DATA *ch, OBJ_DATA *obj, bool loot_check) );
void  remove_all_objs  args( (CHAR_DATA *ch, bool verbose) );
void    get_obj         args( ( CHAR_DATA *ch, OBJ_DATA *obj,
          OBJ_DATA *container ) );
void remove_bonuses( CHAR_DATA *ch, OBJ_DATA *obj);
void add_bonuses( CHAR_DATA *ch, OBJ_DATA *obj);

/* act_wiz.c */
void wiznet             args( (char *string, CHAR_DATA *ch, OBJ_DATA *obj,
             long flag, long flag_skip, int min_level ) );
void pnet             args( (char *string, CHAR_DATA *ch, OBJ_DATA *obj,
             long flag, long flag_skip, int min_level ) );

/* alias.c */
void    substitute_alias args( (DESCRIPTOR_DATA *d, char *input) );

/* ban.c */
bool    check_ban       args( ( char *site, int type) );

/* dns.c */
bool	check_dns	args( ( char *site) );
void	dns_site	args( (CHAR_DATA *ch, char *site) );

/* comm.c */
void    show_string     args( ( struct descriptor_data *d, char *input) );
void    close_socket    args( ( DESCRIPTOR_DATA *dclose ) );
void    write_to_buffer args( ( DESCRIPTOR_DATA *d, const char *txt,
          int length ) );
void    send_to_char    args( ( const char *txt, CHAR_DATA *ch ) );
void	send_to_room	args( ( const char *txt, ROOM_INDEX_DATA *room ) );
void    page_to_char    args( ( const char *txt, CHAR_DATA *ch ) );
void    act             args( ( const char *format, CHAR_DATA *ch,
          const void *arg1, const void *arg2, int type, bool ooc ) );
void    act_new         args( ( const char *format, CHAR_DATA *ch, 
          const void *arg1, const void *arg2, int type,
          int min_pos, bool ooc) );

/* deity.c */
void do_deity_msg(char *msg, CHAR_DATA *ch);
void	give_gift	args( (CHAR_DATA *ch, int gift) );
void	reanimation	args( (CHAR_DATA *ch) );
bool	has_gift	args( (CHAR_DATA *ch, int gift) );
bool	is_aligned	args( (CHAR_DATA *ch) );
void log_deity_favor(CHAR_DATA *ch, CHAR_DATA *alt, int type);
bool deity_enchant_armor(CHAR_DATA *ch, int amount);
bool deity_enchant_weapon(CHAR_DATA *ch, OBJ_DATA *obj, int amount);
int do_favor_error(CHAR_DATA *ch, int rarity, int index, int xp, int favor_strength);
int do_favor_reward(CHAR_DATA *ch, CHAR_DATA *victim, int rarity, int index, int xp, int favor_strength); 
int deity_favor_message(CHAR_DATA *ch, CHAR_DATA *victim, int xp);
int deity_trial_kill(CHAR_DATA *ch, CHAR_DATA *victim, int xp);

/* db.c */
char *  print_flags     args( ( long flag ));
void    boot_db         args( ( void ) );
void    area_update     args( ( void ) );
void    MobIndexToInstance  args( ( CHAR_DATA *mob, MOB_INDEX_DATA *pMobIndex ) );
CD *    create_mobile   args( ( MOB_INDEX_DATA *pMobIndex ) );
void    clone_mobile    args( ( CHAR_DATA *parent, CHAR_DATA *clone) );
void    ObjIndexToInstance  args( ( OBJ_DATA *obj, OBJ_INDEX_DATA *pObjIndex, int level, bool favored ) );
OD *    create_object   args( ( OBJ_INDEX_DATA *pObjIndex, int level, bool favored ) );
void    clone_object    args( ( OBJ_DATA *parent, OBJ_DATA *clone ) );
void    clear_char      args( ( CHAR_DATA *ch ) );
char *  get_extra_descr args( ( const char *name, EXTRA_DESCR_DATA *ed ) );
MID *   get_mob_index   args( ( int vnum ) );
OID *   get_obj_index   args( ( int vnum ) );
RID *   get_room_index  args( ( int vnum ) );
char    fread_letter    args( ( FILE *fp ) );
int     fread_number    args( ( FILE *fp ) );
long    fread_flag      args( ( FILE *fp ) );
char *  fread_string    args( ( FILE *fp ) );
char *  fread_string_eol args(( FILE *fp ) );
void    fread_to_eol    args( ( FILE *fp ) );
char *  fread_word      args( ( FILE *fp ) );
long    flag_convert    args( ( char letter) );
void *  alloc_mem       args( ( int sMem ) );
void *  alloc_perm      args( ( int sMem ) );
void    free_mem        args( ( void *pMem, int sMem ) );
char *  str_dup         args( ( const char *str ) );
char *  str_dup_perm    args( ( const char *str ) );
void    free_string     args( ( char *pstr ) );
int     number_fuzzy    args( ( int number ) );
int     number_range    args( ( int from, int to ) );
int     number_percent  args( ( void ) );
int     number_door     args( ( void ) );
int     number_bits     args( ( int width ) );
long     number_mm       args( ( void ) );
int     dice            args( ( int number, int size ) );
int     interpolate     args( ( int level, int value_00, int value_32 ) );
void    smash_tilde     args( ( char *str ) );
bool    str_cmp         args( ( const char *astr, const char *bstr ) );
bool    str_prefix      args( ( const char *astr, const char *bstr ) );
bool    str_infix       args( ( const char *astr, const char *bstr ) );
bool    str_suffix      args( ( const char *astr, const char *bstr ) );
char *  capitalize      args( ( const char *str ) );
void    append_file     args( ( CHAR_DATA *ch, char *file, char *str ) );

void    bug             args( ( const char *str, int param ) );
void    log_string      args( ( const char *str ) );
int     get_area_min_vnum  args( (AREA_DATA *area) );
int     get_area_max_vnum  args( (AREA_DATA *area) );
RECIPE_DATA * get_recipe_data args ( (int recipe_number ));
void    tail_chain      args( ( void ) );

/* effect.c */
void    acid_effect     args( (void *vo, int level, int dam, int target) );
void    cold_effect     args( (void *vo, int level, int dam, int target) );
void    fire_effect     args( (void *vo, int level, int dam, int target) );
void	holy_effect	args( (CHAR_DATA *victim, int level, int align, CHAR_DATA *ch ) );
void    poison_effect   args( (void *vo, int level, int dam, int target) );
void    shock_effect    args( (void *vo, int level, int dam, int target) );
void	trap_effect	args( (CHAR_DATA *ch, AFFECT_DATA *paf ) );

/* fight.c */
void sort_clanner_items args((CHAR_DATA *ch, OBJ_DATA *container, OBJ_DATA **loot_start, bool do_linked));
CHAR_DATA *check_is_online args((char *name));
bool    is_safe         args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    is_safe_steal   args( (CHAR_DATA *ch, CHAR_DATA *victim ) );
bool    is_safe_spell   args( (CHAR_DATA *ch, CHAR_DATA *victim, bool area , int sn) );
void    violence_update args( ( void ) );
void    multi_hit       args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dt ) );
bool    damage          args( ( CHAR_DATA *ch, CHAR_DATA *victim, int dam,
        int dt, int class, bool show, bool iOld ) );
int	myrm_pen	args( ( CHAR_DATA *ch, CHAR_DATA *victim ) );
void    update_pos      args( ( CHAR_DATA *victim ) );
void    stop_fighting   args( ( CHAR_DATA *ch, bool fBoth ) );
void    check_killer    args( ( CHAR_DATA *ch, CHAR_DATA *victim) );
void	raw_kill	args( ( CHAR_DATA *victim, CHAR_DATA *ch) );
bool    is_clan_guard   args( ( CHAR_DATA *victim) );
void make_corpse( CHAR_DATA *ch, CHAR_DATA *killer );

/* handler.c */
void prompt_pulse_command args((CHAR_DATA *ch));
void end_pulse_command args((CHAR_DATA *ch, bool success, bool violent));
bool damage_pulse_command args((CHAR_DATA *ch));
void damage_add args((CHAR_DATA *ch, CHAR_DATA *victim, int amount, int duration));
void damage_remove args((CHAR_DATA *ch, DAMAGE_DATA *dam));
DAMAGE_DATA *damage_find args((CHAR_DATA *victim, char *source));
void damage_decrement args((CHAR_DATA *ch));
int	room_has_medium	args( ( CHAR_DATA * ) );
bool	check_hai_ruki	args( ( CHAR_DATA * ) );
bool	shogun_in_group args( ( CHAR_DATA * ) );
int	count_groupies_in_room args( ( CHAR_DATA * ) );
int     smurf_group_count args( ( CHAR_DATA * ) );
bool	check_is_dot	args( ( int dt ) );
CHAR_DATA *get_char_by_id	args( ( long id ) );
int	get_sac_points	args( ( CHAR_DATA *ch, int points  ) );
void	clear_mount	args( ( CHAR_DATA *ch ) );
bool	is_mounted	args( (CHAR_DATA *ch) );
AD      *affect_find args( (AFFECT_DATA *paf, int sn));
void    affect_check    args( (CHAR_DATA *ch, int where, int vector) );
int     count_users     args( (OBJ_DATA *obj) );
void    deduct_cost     args( (CHAR_DATA *ch, int cost) );
void	wait_state	args( (CHAR_DATA *ch, int npulse) );
void    affect_enchant  args( (OBJ_DATA *obj) );
int     check_immune    args( (CHAR_DATA *ch, int dam_type) );
int     liq_lookup      args( ( const char *name) );
int     material_lookup args( ( const char *name) );
int     weapon_lookup   args( ( const char *name) );
int     weapon_type     args( ( const char *name) );
int	deity_lookup	args( (const char *name) );
int     deity_type      args( ( const char *name) );
char    *weapon_name    args( ( int weapon_Type) );
int     item_lookup     args( ( const char *name) );
char    *item_name      args( ( int item_type) ); 
int     attack_lookup   args( ( const char *name) );
int     race_lookup     args( ( const char *name) );
long    pnet_lookup   args( ( const char *name) );
int     class_lookup    args( ( const char *name) );
int	kit_lookup	args( ( const char *name) );
bool    is_clan         args( (CHAR_DATA *ch) );
bool    is_same_clan    args( (CHAR_DATA *ch, CHAR_DATA *victim));
bool    is_old_mob      args ( (CHAR_DATA *ch) );
bool	out_of_element	args( (CHAR_DATA *ch) );
int     get_skill       args( ( CHAR_DATA *ch, int sn ) );
int     get_weapon_sn   args( ( CHAR_DATA *ch, bool fSecondary ) );
int     get_weapon_skill args(( CHAR_DATA *ch, int sn ) );
int     get_age         args( ( CHAR_DATA *ch ) );
void    reset_char      args( ( CHAR_DATA *ch )  );
int     get_trust       args( ( CHAR_DATA *ch ) );
int     get_curr_stat   args( ( CHAR_DATA *ch, int stat ) );
int     get_max_train   args( ( CHAR_DATA *ch, int stat ) );
int     can_carry_n     args( ( CHAR_DATA *ch ) );
int     can_carry_w     args( ( CHAR_DATA *ch ) );
int     apply_chi       args( ( CHAR_DATA *ch, int num ) );
bool    is_name         args( ( char *str, char *namelist ) );
bool	is_exact_name	args( ( char *str, char *namelist ) );
void    flash_affect_to_char  args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void    affect_to_char  args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void	affect_to_room  args( ( ROOM_INDEX_DATA *room, AFFECT_DATA *paf ) );
void    affect_to_obj   args( ( OBJ_DATA *obj, AFFECT_DATA *paf ) );
void	raffect_remove	args( ( ROOM_INDEX_DATA *room, AFFECT_DATA *paf ) );
bool    check_trap	args( ( ROOM_INDEX_DATA *room, int trap ) );
void    flash_affect_remove   args( ( CHAR_DATA *ch, AFFECT_DATA *paf, int AppType) );
void    affect_remove   args( ( CHAR_DATA *ch, AFFECT_DATA *paf, int AppType) );
void    affect_remove_obj args( (OBJ_DATA *obj, AFFECT_DATA *paf ) );
void    affect_strip    args( ( CHAR_DATA *ch, int sn ) );
bool    is_flash_affected     args( ( CHAR_DATA *ch, int sn ) );
bool    is_affected     args( ( CHAR_DATA *ch, int sn ) );
bool    is_room_affected args( ( ROOM_INDEX_DATA *room, int sn) );
void    affect_join     args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
void    char_from_room  args( ( CHAR_DATA *ch ) );
void    char_to_room    args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
void    obj_to_char     args( ( OBJ_DATA *obj, CHAR_DATA *ch ) );
void    obj_from_char   args( ( OBJ_DATA *obj ) );
int     apply_ac        args( ( OBJ_DATA *obj, int iWear, int type ) );
OD *    get_eq_char     args( ( CHAR_DATA *ch, int iWear ) );
void    equip_char      args( ( CHAR_DATA *ch, OBJ_DATA *obj, int iWear ) );
void    unequip_char    args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
int     count_obj_list  args( ( OBJ_INDEX_DATA *obj, OBJ_DATA *list ) );
void    obj_from_room   args( ( OBJ_DATA *obj ) );
void    obj_to_room     args( ( OBJ_DATA *obj, ROOM_INDEX_DATA *pRoomIndex ) );
void    obj_to_obj      args( ( OBJ_DATA *obj, OBJ_DATA *obj_to ) );
void    obj_from_obj    args( ( OBJ_DATA *obj ) );
void    extract_obj     args( ( OBJ_DATA *obj ) );
void    extract_char    args( ( CHAR_DATA *ch, bool fPull ) );
CD *    get_char_room   args( ( CHAR_DATA *ch, char *argument ) );
CD *    get_char_online args( ( CHAR_DATA *ch, char *argument ) );
CD *    get_char_world  args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_type    args( ( OBJ_INDEX_DATA *pObjIndexData ) );
OD *    get_obj_list    args( ( CHAR_DATA *ch, char *argument,
          OBJ_DATA *list ) );
OD *	find_obj_carry	args( ( CHAR_DATA *ch, CHAR_DATA *vc, char *arg ) );
OD *	find_obj_wear	args( ( CHAR_DATA *ch, CHAR_DATA *vc, char *arg ) );
OD *    get_obj_carry   args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_wear    args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_here    args( ( CHAR_DATA *ch, char *argument ) );
OD *    get_obj_world   args( ( CHAR_DATA *ch, char *argument ) );
OD *    create_money    args( ( int gold, int silver ) );
int     get_obj_number  args( ( OBJ_DATA *obj ) );
int     get_obj_weight  args( ( OBJ_DATA *obj ) );
int     get_true_weight args( ( OBJ_DATA *obj ) );
bool    room_is_dark    args( ( ROOM_INDEX_DATA *pRoomIndex ) );
bool    is_room_owner   args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *room) );
bool    room_is_private args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex ) );
bool    can_see         args( ( CHAR_DATA *ch, CHAR_DATA *victim, bool ooc ) );
bool    can_see_obj     args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool	is_room_clan    args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
bool    can_see_room    args( ( CHAR_DATA *ch, ROOM_INDEX_DATA *pRoomIndex) );
bool    can_drop_obj    args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
bool    can_wear_obj    args( ( CHAR_DATA *ch, OBJ_DATA *obj ) );
char *  item_type_name  args( ( OBJ_DATA *obj ) );
char *  item_type_name_num  args ( ( int num ) );
char *  affect_loc_name args( ( int location ) );
char *  affect_bit_name args( ( int vector ) );
char *  extra_bit_name  args( ( int extra_flags ) );
char *  extra_bit_name2 args( ( int extra_flags2 ) );
char *  wear_bit_name   args( ( int wear_flags ) );
char *  act_bit_name    args( ( int act_flags ) );
char *  mhs_bit_name	args( ( int bit_name ) );
char *  off_bit_name    args( ( int off_flags ) );
char *  imm_bit_name    args( ( int imm_flags ) );
char *  form_bit_name   args( ( int form_flags ) );
char *  part_bit_name   args( ( int part_flags ) );
char *  weapon_bit_name args( ( int weapon_flags ) );
char *  comm_bit_name   args( ( int comm_flags ) );
char *  cont_bit_name   args( ( int cont_flags) );
char *  clan_bit_name   args( ( int clan_flags) );
char*	exit_bit_name	args( ( int exit_flags ) );


/* interp.c */
void    interpret       args( ( CHAR_DATA *ch, char *argument ) );
bool    is_number       args( ( char *arg ) );
int     number_argument args( ( char *argument, char *arg ) );
int     mult_argument   args( ( char *argument, char *arg) );
char *  one_argument    args( ( char *argument, char *arg_first ) );
char *  one_argument_cs args( ( char *argument, char *arg_first ) );
bool	check_social	args( ( CHAR_DATA *ch, char *command, char *argument ));

/* clan.c */
int calculate_bonus_merit args((CHAR_DATA *ch, bool new_join));
void set_clan_skills args((CHAR_DATA *ch));
bool clan_kill_type args((CHAR_DATA *killer, CHAR_DATA *victim));
void remove_clan_member args((CLAN_CHAR *cchar));
void add_clan_member args((CLAN_DATA *clan, CHAR_DATA *ch, int rank));
CLAN_CHAR *find_char_clan args((char *name));

/* live_edit.c */
void verify_pricing_table args((void));
void edit_stop args((CHAR_DATA *ch));
int count_edit_obj args((CHAR_DATA *ch, int flag, bool verbose, bool hedit));
PLAN_DATA *find_edit_obj args((CHAR_DATA *ch, char *arg, bool hedit));
PLAN_DATA *find_edit_obj_by_index args((PLAN_DATA *start, int type, int plan_index));
PLAN_DATA *find_char_room_obj args((CHAR_DATA *ch, bool hedit));
// New help code
void blast_punctuation args((char *arg, bool leave_quote, bool capitalize));
void lower_only args((char *arg));
int calc_cost_range args((int start, int count, bool hedit));
void set_obj_cost args((CHAR_DATA *ch, PLAN_DATA *obj, bool hedit, bool override));
bool check_can_edit args((CHAR_DATA *ch, int action, bool hedit));
bool pay_hall_cost args((CHAR_DATA *ch, int amount, bool do_buy, bool hedit));
void swap_rooms args((CHAR_DATA *ch, PLAN_DATA *old_room, PLAN_DATA *new_room, bool hedit));
void load_plan_obj args((PLAN_DATA *obj, bool strings));
void load_room_obj args((PLAN_DATA *obj, bool strings));
void load_mob_obj args((PLAN_DATA *obj, bool strings));
void load_item_obj args((PLAN_DATA *obj, bool strings));
void load_exit_obj args((PLAN_DATA *obj, bool strings));
void place_hall_obj args((CHAR_DATA *ch, PLAN_DATA *obj, bool hedit));
int get_refund_amount args((CHAR_DATA *ch, int amount, bool hedit));
int remove_hall_obj args((CHAR_DATA *ch, PLAN_DATA *obj, bool hedit));
void reset_hall_obj args((CHAR_DATA *ch, PLAN_DATA *obj, bool reset_all));
bool save_hall args((char *clan_name, PLAN_DATA *plans, bool save_immediately));
void save_clan_list args((void));
void load_clan_list args((void));
void load_clan args((char *clan_name, int def_clan));
void do_save_clan args((CLAN_DATA *clan));
void save_clan args((CHAR_DATA *ch, bool save_c, bool save_h, bool hedit));
bool fread_plan_obj args((FILE *fp, PLAN_DATA *obj));
bool fread_plan_exit args((FILE *fp, PLAN_DATA *first, CLAN_DATA *clan));
void fread_clan_hall args((FILE *fp, PLAN_DATA **head, CLAN_DATA *clan));
void fread_clan_messages args((FILE *fp, CLAN_DATA *clan));
void respawn_plan_obj args((PLAN_DATA *obj, PLAN_DATA *start, bool show_creation));
int get_arg_dir args((char *arg));
void clear_string args((char **str, char *new_str));
void search_linked_rooms args((PLAN_DATA *start));
int new_obj_index args((CHAR_DATA *ch, int type, bool hedit));
void player_edit args((CHAR_DATA *ch, char *argument, bool hedit));
void do_hedit args((CHAR_DATA *ch, char *argument));
void do_pedit args((CHAR_DATA *ch, char *argument));


/* mag2.c */
void	blow_orb	args( ( CHAR_DATA *victim,int sn ) );

/* magic.c */
bool check_annointment args((CHAR_DATA *victim, CHAR_DATA *ch));
void apply_mala_damage args((CHAR_DATA *ch, CHAR_DATA *victim, int amount));
int     find_spell      args( ( CHAR_DATA *ch, const char *name) );
int      mana_cost 	args( (CHAR_DATA *ch, int min_mana, int level, int sn) );
int     skill_lookup    args( ( const char *name ) );
int     slot_lookup     args( ( int slot ) );
bool    saves_spell     args( ( int level, CHAR_DATA *victim, int dam_type ) );
int     compute_casting_level args( ( CHAR_DATA *ch, int sn ) );
void    obj_cast_spell  args( ( int sn, int level, CHAR_DATA *ch,
            CHAR_DATA *victim, OBJ_DATA *obj ) );
bool	check_dispel	args( ( int dis_level, CHAR_DATA *victim, int sn) );
extern char *target_name;
void	dot		args( ( CHAR_DATA *ch, AFFECT_DATA *paf ) );
bool	reup_affect	args( (CHAR_DATA *ch, int sn, int duration, int level) );

/* note.c */
void peek_argument args((char *argument, char *arg_first));
bool start_long_edit args((CHAR_DATA *ch, int limit, int type, char *base_str));
void do_long_edit args((CHAR_DATA *ch, char *arg, int type, int edit_type));
void end_long_edit args((CHAR_DATA *ch, char **result));


/* save.c */
void    save_char_obj   args( ( CHAR_DATA *ch ) );
bool    load_char_obj   args( ( DESCRIPTOR_DATA *d, char *name ) );
void    save_pits       args( (void) );
void    load_pits       args( (void) );

/* skills.c */
int     skill_level	args( ( CHAR_DATA *ch, int sn ) );
bool    parse_gen_groups args( ( CHAR_DATA *ch,char *argument ) );
void    list_group_costs args( ( CHAR_DATA *ch ) );
void    list_group_known args( ( CHAR_DATA *ch ) );
int     exp_per_level   args( ( CHAR_DATA *ch, int points ) );
void    check_improve   args( ( CHAR_DATA *ch, int sn, bool success, 
            int multiplier ) );
int     group_lookup    args( (const char *name) );
void    gn_add          args( ( CHAR_DATA *ch, int gn) );
void    gn_remove       args( ( CHAR_DATA *ch, int gn) );
void    group_add       args( ( CHAR_DATA *ch, const char *name, bool deduct) );
void    group_remove    args( ( CHAR_DATA *ch, const char *name) );

/* special.c */
bool is_bounty_target args ( (CHAR_DATA *victim, bool kill) );
void describe_mob_bounty args ( (CHAR_DATA *target, CHAR_DATA *teller, bool just_started) );
bool is_shaded args ( (CHAR_DATA *shade) );
void remove_shaded_room args ( (CHAR_DATA *shade) );
SF *    spec_lookup     args( ( const char *name ) );
char *  spec_name       args( ( SPEC_FUN *function ) );
void quest_handler(CHAR_DATA *quest_npc, CHAR_DATA *ch, OBJ_DATA *obj, int quest, int update);
void log_quest_detail(char *buf, int quest);

/* teleport.c 
RID *   room_by_name    args( ( char *target, int level, bool error) );
*/
/* update.c */
bool spawn_rainbow args((void));
void    advance_level   args( ( CHAR_DATA *ch ) );
void    gain_exp        args( ( CHAR_DATA *ch, long gain ) );
void    gain_condition  args( ( CHAR_DATA *ch, int iCond, int value ) );
void    update_handler  args( ( void ) ); 
/* menu.c */
void do_menu ( CHAR_DATA *ch, char *arg );
/* editor.c */
void do_line_editor       args( ( CHAR_DATA *ch, char *arg, DO_FUN *call_back ) );
void insert_line_callback args ( (CHAR_DATA *ch, char *arg) );
/* macro. c */
void clear_macro_marks args ( ( CHAR_DATA *ch ) );
bool check_macro       args ( ( CHAR_DATA *ch, char *argument ) );
/* olc.c    */
bool create_room      ( CHAR_DATA *ch, ROOM_INDEX_DATA *room, int dir, int move_char );

#undef  CD
#undef  MID
#undef  OD
#undef  OID
#undef  RID
#undef  SF
#undef AD
