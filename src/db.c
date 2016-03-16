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

static char rcsid[] = "$Id: db.c,v 1.186 2004/08/26 01:30:26 boogums Exp $";
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

#include <gc.h>
#include "merc.h"
#include "db.h"
#include "recycle.h"
#include "music.h"
#include "lookup.h"
#include "gladiator.h"

extern int bounty_available[];
extern int bounty_vnum;
extern int bounty_type;
extern int bounty_desc;
extern int bounty_item;
extern int bounty_room;
extern int bounty_timer;
extern bool bounty_downgrade;

/* a test to see if this acually works */
#define calloc(m,n) GC_MALLOC((m)*(n))

#if defined(unix)
extern int getrlimit(int resource, struct rlimit *rlp);
/*extern int setrlimit(int resource, struct rlimit *rlp);*/
#endif

#if !defined(macintosh)
extern  int _filbuf   args( (FILE *) );
#endif

#if !defined(OLD_RAND)
long random();
/*void srandom(int seed);*/
int getpid();
time_t time(time_t *tloc);
#endif

bool new_helps = FALSE;// New help code
void load_new_helps(HELP_DATA **first, HELP_DATA **last, int *counter);
void create_help_tracks(void);

/* externals for counting purposes */
extern  OBJ_DATA  *obj_free;
extern  CHAR_DATA *char_free;
extern  DESCRIPTOR_DATA *descriptor_free;
extern  PC_DATA   *pcdata_free;
extern  AFFECT_DATA *affect_free;

/*
 * Globals.
 */
RECIPE_DATA * recipe_first;
RECIPE_DATA * recipe_last;

CSTAT_DATA *  cstat_first;
CSTAT_DATA *  cstat_last;

HELP_DATA *   help_first;
HELP_DATA *   help_last;

HELP_TRACKER * help_track_first = NULL;// New help code
HELP_TRACKER *  help_tracks[28]; // A-Z and #

SHOP_DATA *   shop_first;
SHOP_DATA *   shop_last;

NOTE_DATA *   note_free;

char      bug_buf   [2*MAX_INPUT_LENGTH];
char      dns_buf   [20];
CHAR_DATA *   char_list;
char *      help_greeting;
char      log_buf   [2*MAX_INPUT_LENGTH];
KILL_DATA   kill_table  [MAX_LEVEL];
NOTE_DATA *   note_list;
OBJ_DATA *    object_list;
TIME_INFO_DATA    time_info;
WEATHER_DATA    weather_info;
int	weapons_popped = 0;
GLADIATOR_INFO_DATA    gladiator_info;

sh_int          gsn_light_blast;
sh_int          gsn_shaded_room;
sh_int          gsn_sunburst;
sh_int		gsn_arcantic_lethargy;
sh_int		gsn_arcantic_alacrity;
sh_int		gsn_clarity;
sh_int		gsn_midnight_cloak;
sh_int		gsn_shield_of_thorns;
sh_int          gsn_spell_restrain;
sh_int		gsn_shield_of_brambles;
sh_int		gsn_shield_of_spikes;
sh_int		gsn_shield_of_blades;
sh_int		gsn_herbalism;
sh_int		gsn_spirit_of_boar;
sh_int		gsn_spirit_of_bear;
sh_int		gsn_spirit_of_cat;
sh_int		gsn_spirit_of_owl;
sh_int		gsn_spirit_of_wolf;
sh_int		gsn_stone_skin;
sh_int		gsn_symbol_1;
sh_int		gsn_symbol_2;
sh_int		gsn_symbol_3;
sh_int		gsn_symbol_4;
sh_int		gsn_hamstring;
sh_int		gsn_acclimate;
sh_int          gsn_honor_guard;
sh_int		gsn_enhanced_critical;
sh_int		gsn_cleave;
sh_int		gsn_bludgeon;
sh_int		gsn_steel_skin;
sh_int		gsn_diamond_skin;
sh_int		gsn_adamantite_skin;
sh_int		gsn_shield_of_faith;
sh_int		gsn_alchemy;
sh_int		gsn_convert;
sh_int		gsn_annointment;
sh_int          gsn_aura_of_valor;
sh_int          gsn_guardian;
sh_int          gsn_hemorrhage;
sh_int		gsn_riding;
sh_int		gsn_bladesong;
sh_int		gsn_rage;
sh_int		gsn_cutpurse;
sh_int		gsn_garotte;
sh_int		gsn_espionage;
sh_int		gsn_dust_storm;
sh_int		gsn_holy_chant;
sh_int		gsn_ninjitsu;
sh_int		gsn_kurijitsu;
sh_int		gsn_imbue;
sh_int		gsn_endurance;
sh_int		gsn_barbarian_rage;
sh_int		gsn_forget;
sh_int		gsn_focus;
sh_int		gsn_enervation;
sh_int		gsn_fumble;
sh_int		gsn_blur;
sh_int		gsn_spellcraft;
sh_int		gsn_sacred_guardian;
sh_int		gsn_dual_wield;
sh_int		gsn_infiltrate;
sh_int		gsn_tumbling;
sh_int      gsn_connive;
sh_int	    gsn_dae_tok;
sh_int      gsn_backstab;
sh_int      gsn_dodge;
sh_int      gsn_envenom;
sh_int      gsn_hide;
sh_int      gsn_peek;
sh_int      gsn_pick_lock;
sh_int      gsn_sneak;
sh_int      gsn_scan;
sh_int	    gsn_slice;
/* Line Below Added 29-AUG-00 by Boogums */
sh_int      gsn_kcharge;
sh_int      gsn_scribe;
sh_int      gsn_infuse;
sh_int      gsn_endow;
/* 2  Lines Below Added 23SEP00 by Boogums */
sh_int      gsn_call_mount;
sh_int      gsn_charm_animal;
/* Line below added 07OCT00 by Boogums */
sh_int      gsn_sharpen;
sh_int      gsn_swarm;
sh_int      gsn_steal;
sh_int      gsn_bump;
sh_int      gsn_snatch;
sh_int	    gsn_trap;

sh_int      gsn_disarm;
sh_int      gsn_enhanced_damage;
sh_int      gsn_kick;
sh_int      gsn_parry;
sh_int      gsn_rescue;
sh_int      gsn_second_attack;
sh_int      gsn_third_attack;
sh_int		gsn_fourth_attack;

sh_int      gsn_blindness;
sh_int      gsn_charm_person;
sh_int      gsn_curse;
sh_int      gsn_invis;
sh_int      gsn_mass_invis;
sh_int      gsn_poison;
sh_int      gsn_plague;
sh_int      gsn_sleep;
sh_int      gsn_sanctuary;
sh_int      gsn_withstand_death;
/* new gsns */

sh_int      gsn_axe;
sh_int      gsn_dagger;
sh_int      gsn_flail;
sh_int      gsn_mace;
sh_int      gsn_polearm;
sh_int      gsn_shield_block;
sh_int      gsn_spear;
sh_int      gsn_sword;
sh_int      gsn_whip;
sh_int      gsn_cuffs_of_justice;
 
sh_int      gsn_bash;
sh_int      gsn_grab;
sh_int      gsn_berserk;
sh_int	    gsn_bite;
sh_int	    gsn_bleed;
sh_int	    gsn_breathe;
sh_int      gsn_dirt;
sh_int	    gsn_fear;
sh_int      gsn_hand_to_hand;
sh_int	    gsn_hex;
sh_int      gsn_trip;
 
sh_int      gsn_fast_healing;
sh_int      gsn_haggle;
sh_int      gsn_lore;
sh_int      gsn_meditation;
sh_int      gsn_morph;
sh_int	    gsn_communion;
sh_int		gsn_nethermancy;
sh_int          gsn_nether_shield;
sh_int		gsn_magic_resistance;
sh_int		gsn_fade;
sh_int		gsn_vision;
sh_int      gsn_scrolls;
sh_int      gsn_staves;
sh_int      gsn_wands;
sh_int      gsn_swim;
sh_int      gsn_recall;
sh_int      gsn_chi;
sh_int      gsn_throw;
sh_int      gsn_kailindo;
sh_int      gsn_weave_resistance;
sh_int      gsn_insanity;
sh_int      gsn_vorpal;
sh_int	    gsn_wound_transfer;
sh_int	    gsn_protect_neutral;
sh_int	    gsn_water_breathing;
sh_int	    gsn_stonefist;
sh_int	    gsn_earthbind;
sh_int	    gsn_irradiate;
sh_int	    gsn_asphyxiate;
sh_int      gsn_confusion;
sh_int      gsn_cone_of_silence;
sh_int      gsn_blade_barrier;
sh_int	    gsn_wall_fire;
sh_int      gsn_wall_ice;
sh_int      gsn_hydrophilia;


/*
 * Locals.
 */
MOB_INDEX_DATA *  mob_index_hash    [MAX_KEY_HASH];
OBJ_INDEX_DATA *  obj_index_hash    [MAX_KEY_HASH];
ROOM_INDEX_DATA * room_index_hash   [MAX_KEY_HASH];
char *        string_hash       [MAX_KEY_HASH]; 

AREA_DATA *   area_first;
AREA_DATA *   area_last;

char *      string_space;
char *      top_string;
char      str_empty [1];

int     top_affect;
int     top_area;
int     top_cstat;
int     top_ed;
int     top_exit;
int     top_help;
int     top_recipe;
int     top_mob_index;
int     top_obj_index;
int     top_reset;
int     top_room;
int     top_shop;
int     mobile_count = 0;
int     newmobs = 0;
int     newobjs = 0;
long    temp_mem_count = 0;
long    perm_mem_count = 0;


/*
 * Memory management.
 * Increase MAX_STRING if you have too.
 * Tune the others only if you understand what you're doing.
 */
#define     MAX_STRING  4500000
#define     MAX_PERM_BLOCK  131072
#define     MAX_MEM_LIST  12

void *      rgFreeList  [MAX_MEM_LIST];
const long   rgSizeList  [MAX_MEM_LIST]  =
{
    16, 32, 64, 128, 256, 1024, 2048, 4096, 8192, 16384, 32768, 65536-64
};

int     nAllocString;
int     sAllocString;
int     nAllocPerm;
int     sAllocPerm;



/*
 * Semi-locals.
 */
bool      fBootDb;
FILE *      fpArea;
char      strArea[MAX_INPUT_LENGTH];



/*
 * Local booting procedures.
*/
void    init_mm         args( ( void ) );
void  load_area args( ( FILE *fp, char *file_name ) );
void  load_cstat  args( ( FILE *fp ) );
void  load_helps  args( ( FILE *fp ) );
void  load_old_mob  args( ( FILE *fp ) );
void  load_mobiles  args( ( FILE *fp ) );
void  load_old_obj  args( ( FILE *fp ) );
void  load_objects  args( ( FILE *fp ) );
void  load_recipes args( ( FILE *fp )  );
void  load_resets args( ( FILE *fp ) );
void  load_rooms  args( ( FILE *fp ) );
void  load_shops  args( ( FILE *fp ) );
void  load_socials  args( ( FILE *fp ) );
void  load_specials args( ( FILE *fp ) );
void  load_notes  args( ( void ) );
void  load_bans args( ( void ) );
void  load_dns args( ( void ) );

void  fix_exits args( ( void ) );

void  reset_area  args( ( AREA_DATA * pArea ) );

#if defined(unix)
/* RT max open files fix */
 
void maxfilelimit()
{
    struct rlimit r;
 
    getrlimit(RLIMIT_NOFILE, &r);
    r.rlim_cur = r.rlim_max;
    setrlimit(RLIMIT_NOFILE, &r);
}
#endif

AREA_NAME_DATA *area_name_first,*area_name_last;

// New help code
void delete_helps(HELP_DATA **list_file)
{
  HELP_DATA *pHelp_next;
  HELP_DATA *pHelp = *list_file;
  for(pHelp = help_first; pHelp; pHelp = pHelp_next)
  {
    pHelp_next = pHelp->next;
    clear_string(&pHelp->keyword, NULL);
    clear_string(&pHelp->text, NULL);
    clear_string(&pHelp->related, NULL);
    clear_string(&pHelp->editor, NULL);
    GC_FREE(pHelp);
  }
}

void do_synchelps( CHAR_DATA *ch, char *argument )
{
#ifdef OLC_VERSION
  send_to_char("synchelps pulls from OLC to where you are, you can't use it here.\n\r", ch);
  return;
#else
  int i;
  FILE *oldfp, *newfp;
  char bak_name[255];
  char *old_greeting;
  HELP_TRACKER *pTracker, *pTracker_next;
  HELP_DATA *first_new = NULL, *last_new = NULL;
  int top_new = 0;
  oldfp = fopen(HELP_FILE_OLC, "r");
  if(!oldfp)
  {
    send_to_char("There is no new helpfile saved on OLC to sync with.\n\r", ch);
    return; 
  }

  newfp = fopen(HELP_FILE, "r");
  if(newfp)
  {/* Check its current existance -- okay if it's not there, just no backup */
    while(!feof(oldfp) && !feof(newfp) && fgetc(newfp) == fgetc(oldfp))
      continue; 
    if(feof(oldfp) || feof(newfp))
    {
      fclose(oldfp);
      fclose(newfp);
      send_to_char("The two files are identical, there is no need to sync.\n\r", ch);
      return;
    }
    fclose(newfp);// Done with current file
    sprintf(bak_name, "../area/livebak/new_helps_%ld.bak", current_time);
    rename(HELP_FILE, bak_name);
    fseek(oldfp, 0, SEEK_SET);// Reset the OLC file to beginning
  }
  newfp = fopen(HELP_FILE, "w");
  if(newfp)
  {
    char transfer = fgetc(oldfp);
    while(!feof(oldfp))
    {
      fputc(transfer, newfp);
      transfer = fgetc(oldfp);/* trigger eof without a duplicate last-char write */
    }
    fclose(oldfp);
    fclose(newfp);
  }
  else
  {
    send_to_char("Failed to open new file for writing.  Sync did not occur.\n\r", ch);
    fclose(oldfp);
    return;
  }
  // Live reload now
  old_greeting = help_greeting;
  help_greeting = NULL;// Ensures it's available in the new helps
  load_new_helps(&first_new, &last_new, &top_new);
  if(!help_greeting) // BIG PROBLEM IF THIS HAPPENS - game can't start
  {// Little has been added to memory at this point
    delete_helps(&first_new);
    help_greeting = old_greeting;
    unlink(HELP_FILE);
    rename(bak_name, HELP_FILE);// Restore the old one
    send_to_char("help greeting not found, sync {RABORTED{x.\n\r", ch);
    return;
  }
  // Clear out the old help trackers then the help files
  for(i = 0; i < 27; i++)
  {
    help_tracks[i] = NULL;
  }
  for(pTracker = help_track_first; pTracker; pTracker = pTracker_next)
  {
    pTracker_next = pTracker->next;
    clear_string(&pTracker->keyword, NULL);
    GC_FREE(pTracker);
  }
  help_track_first = NULL;

  delete_helps(&help_first);

  // All memory has been cleared from the old ones, point to the new ones
  help_first = first_new;
  help_last = last_new;
  top_help = top_new;
  create_help_tracks();// Link the tracks for the new ones

  send_to_char("Helps have been updated and reloaded, no reboot is needed.\n\r", ch);
  sprintf(bak_name, "Backup file name is new_helps_%ld.bak\n\r", current_time);
  send_to_char(bak_name, ch);
#endif
}

void do_syncarea( CHAR_DATA *ch, char *argument )
{
#ifdef OLC_VERSION
  send_to_char("syncarea pulls from OLC to where you are, you can't use it here.\n\r", ch);
  return;
#else
  FILE *oldfp, *newfp;
  char new_name[255], bak_name[255], olc_name[255];

  sprintf(new_name, "../area/%s.are", argument);
  newfp = fopen(new_name, "r");
  if(!newfp)
  {/* Check its current existance */
    send_to_char("File does not exist in current files.\n\r", ch);
    return;
  }
  /* Good to copy */
  sprintf(bak_name, "../area/livebak/%s_%ld.bak", argument, current_time);
  sprintf(olc_name, "/mud/moosehead/olc/olcarea/%s.are", argument);
  oldfp = fopen(olc_name, "r");
  if(!oldfp)
  {
    sprintf(olc_name, "/mud/moosehead/newareas/%s.are", argument);
    oldfp = fopen(olc_name, "r");
  }
  if(oldfp)
  {
    // newfp is still holding open the original file at this point
    while(!feof(oldfp) && !feof(newfp) && fgetc(newfp) == fgetc(oldfp))
      continue; 
    if(feof(oldfp) || feof(newfp))
    {
      fclose(oldfp);
      fclose(newfp);
      send_to_char("The two files are identical, there is no need to sync.\n\r", ch);
      return;
    }
    fseek(oldfp, 0, SEEK_SET);// Reset the OLC file to beginning
    fclose(newfp);// Done with current file
    rename(new_name, bak_name);
    newfp = fopen(new_name, "w");
    if(newfp)
    {
      char transfer = fgetc(oldfp);
      while(!feof(oldfp))
      {
        fputc(transfer, newfp);
        transfer = fgetc(oldfp);/* trigger eof without a duplicate last-char write */
      }
      fclose(oldfp);
      fclose(newfp);
    }
    else
    {
      send_to_char("Failed to open new file for writing.  This may result in a failed reboot.\n\r", ch);
      fclose(oldfp);
      return;
    }
  }
  else
  {
    fclose(newfp);
    send_to_char("Failed to locate area file in olc's directory.\n\r", ch);
    return;
  }
  send_to_char("Area has been updated, you must reboot to bring the changes live.\n\r", ch);
  sprintf(bak_name, "Backup file name is %s_%ld.bak\n\r", argument, current_time);
  send_to_char(bak_name, ch);
#endif
}

void rename_area (char *strArea)
{
  char buf2[MAX_STRING_LENGTH],
       strBak[MAX_STRING_LENGTH],
       sbuf[MAX_STRING_LENGTH];
  char *pstr,*pstr2;
  FILE *fp;
  char *strtime;
  
  fp = fopen(strArea,"r");
  if (fp != NULL) {    /* if the file exists, rename it */
    fclose (fp);
    strcpy (buf2,strArea);
    pstr = buf2;
    pstr2 = NULL;
    while (pstr) {
      pstr2 = ++pstr;
      pstr = strchr (pstr,'.');
    }
    if (pstr2) *pstr2 = (char)NULL;
    strcat (buf2,"old");
    fp = fopen (buf2,"r");
    if (fp != NULL) {
      fclose (fp);
      unlink (buf2);
    }
    rename (strArea,buf2);
    sprintf (sbuf,"Renaming '%s' as '%s'.",strArea,buf2);
    log_string (sbuf);    
  }
  
  strcpy (strBak,strArea);
  pstr = strBak;
  pstr2 = NULL;
  while (pstr) {
    pstr2 = ++pstr;
    pstr = strchr (pstr,'.');
  }  
  if (pstr2) *pstr2 = (char)NULL;
  strcat (strBak,"bak");  
  
  fp = fopen (strArea,"r");
  if (fp != NULL) {
    fclose (fp);
    unlink (strArea);
    sprintf (sbuf,"Deleting '%s'",strArea);
    log_string (sbuf);
  }
  
  fp = fopen (strBak,"r");
  if (fp != NULL) {
    fclose (fp);
  
    rename (strBak,strArea);
    sprintf (sbuf,"Renaming '%s' as '%s'.\n",strBak,strArea);
    log_string (sbuf);
  } else {
    log_string ("Could not find backup to recopy over.");
  }
  
  strtime       = ctime( &current_time );
  strtime[strlen(strtime)-1]  = '\0';  
  
//  if (fpReserve) fclose(fpReserve);
  if ( ( fp = fopen( NOTE_FILE, "a" ) ) == NULL ) {
    perror (NOTE_FILE);
  } else {  
    fprintf( fp, "Sender  MHS II~\n" );
    fprintf( fp, "Date    %s~\n", strtime );
    fprintf( fp, "Stamp   %ld\n", current_time );
    fprintf( fp, "To      immortal~\n" );
    fprintf( fp, "Subject Failed to boot '%s'~\n", strArea );
    fprintf( fp, "Text\n" );
    fprintf( fp, "Renamed '%s' as '%s'\n",strArea,buf2 );
    fprintf( fp, "If backup existed, '%s' was replaced.\n",strArea );
    fprintf( fp, "\nMHS\n~\n" );
    fclose ( fp );
  }  
  
//  fpReserve = fopen( NULL_FILE, "r" );
  
  

}

// 1-2 qualifier gain for finding a bounty, -1 for missing
// At 15+ can start getting rank 5's and the stones after which it resets
void select_bounty(int qualifier)
{
  CHAR_DATA *mob;
  MOB_INDEX_DATA *pMobIndex;
  CHAR_DATA *mob_found = NULL;
  int i, j;
  int level = 1;
  bounty_downgrade = FALSE;
  bounty_timer = 0;
  bounty_desc = number_range(4, 102);// Below 4 gets too many of the same type
  if(qualifier > 0)
  {// If qualifier is 0 just go with the minimum level of 0
    int selection_mod = 0;
    int type_mod = number_range(0, UMIN(number_range(5, 10), qualifier));
    // 0-1: mob name. 2,4: room name. 3,5: item name.
    // 6,7: room desc. 8,9: mob desc. 9: item desc
    switch(type_mod)
    {
      default:
      case 1: bounty_type = BOUNTY_MOB_NAME; break;
      case 2:
      case 4: bounty_type = BOUNTY_ITEM_NAME; break;
      case 3:
      case 5: bounty_type = BOUNTY_ROOM_NAME; break;
      case 6:
      case 7: bounty_type = BOUNTY_ROOM_DESC; break;
      case 8:
      case 9: bounty_type = BOUNTY_MOB_DESC; break;
      case 10: bounty_type = BOUNTY_ITEM_DESC; break;
    }
    qualifier /= number_range(3, 7);
    selection_mod = number_percent();
    if(qualifier > 0 && selection_mod < 90)
    {
      if(selection_mod < 70)
        qualifier = 0;
      else
        qualifier = number_range(1, UMIN(MAX_BOUNTY_LEVEL - 1, qualifier));
    }
    level = UMIN(MAX_BOUNTY_LEVEL, qualifier + 1);
  }
  else// Easiest
    bounty_type = BOUNTY_MOB_NAME;

  j = number_range(1, bounty_available[level - 1]);
  i = 0;
  do
  {
    pMobIndex = mob_index_hash[i];
    if(!pMobIndex)
      i++;
  } while(!pMobIndex && i < MAX_KEY_HASH);
  if(i == MAX_KEY_HASH)
  {
    bounty_vnum = bounty_type = bounty_desc = -2;
    bug("Error: Can't find a first mob to select a bounty from.", 0);
    return;
  }
  for(; pMobIndex; )
  {
    if(pMobIndex->bounty_level == level)
    {
      j--;
      if(j <= 0)
        break;
    }
    pMobIndex = pMobIndex->next;
    while(!pMobIndex)
    {
      i++;
      if(i >= MAX_KEY_HASH)
      {
        bounty_vnum = bounty_type = bounty_desc = -2;
        bug("Error: Failed to find an expected mob index for a bounty.", 0);
        return;
      }
      pMobIndex = mob_index_hash[i];
    }
  }
  // Found a mob from the appropriate level, see if it's a valid target. If not
  // continue on from it. If that fails, panic. No, don't panic.  Use a hard
  // coded standby then -- MOB_VNUM_HASSAN should do as a default
  bounty_vnum = -1;
  while(bounty_vnum < 0 && i < MAX_KEY_HASH)
  {
    for ( mob = char_list; mob != NULL; mob = mob->next )
    {
      if(IS_NPC(mob) && mob->pIndexData == pMobIndex)
      {
        if (mob->in_room == NULL || mob->zone != mob->in_room->area)
          continue;
        if(!mob_found)
          mob_found = mob;
        if(bounty_type == BOUNTY_ITEM_NAME || bounty_type == BOUNTY_ITEM_DESC)
        {// See if they've got any original items on them
          OBJ_DATA *obj;
          for ( obj = mob->carrying; obj != NULL; obj = obj->next_content )
          {
            if(obj->original && !IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
            {
              int num_objects = 0;
              bounty_item = obj->pIndexData->vnum;
              bounty_vnum = pMobIndex->vnum;
              bounty_room = mob->in_room->vnum;
              for(obj = mob->carrying; obj != NULL; obj = obj->next_content)
              {
                if(obj->original && !IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
                  num_objects++;
              }
              num_objects = number_range(1, num_objects);
              for(obj = mob->carrying; obj != NULL; obj = obj->next_content)
              {
                if(obj->original && !IS_SET(obj->extra_flags,ITEM_VIS_DEATH))
                {
                  num_objects--;
                  if(num_objects <= 0)
                  {// This is the bounty item
                    bounty_item = obj->pIndexData->vnum;
                  }
                }
              }
              return;
            }
          }
        }
        else
        {// Good to go, everything has a name or a room
          bounty_vnum = pMobIndex->vnum;
          bounty_room = mob->in_room->vnum;
          return;
        }
      }
    }
    if(mob_found)
    {// Mob was found but item didn't match
      bounty_vnum = pMobIndex->vnum;
      bounty_room = mob_found->in_room->vnum;
      bounty_type -= number_range(1, 2);// Fall back to a different type
      return;
    }
    if(bounty_vnum < 0)
    {// Next in the list and try again
      for(; pMobIndex; )
      {
        if(pMobIndex->bounty_level == level)
          break;
        pMobIndex = pMobIndex->next;
        while(!pMobIndex)
        {
          i++;
          if(i >= MAX_KEY_HASH)
            break;
          pMobIndex = mob_index_hash[i];
        }
      }
    }
  }
  // It's Hassan time
  bounty_vnum = MOB_VNUM_HASSAN;
}

void mark_mob_bounties(void)
{
  int i, j;
  CHAR_DATA *mob;
  MOB_INDEX_DATA *pMobIndex;
  
  for(i = 0; i < MAX_KEY_HASH; i++)
  {
    pMobIndex = mob_index_hash[i];
    for(; pMobIndex; pMobIndex = pMobIndex->next)
    {
      pMobIndex->bounty_level = 0;
      if(!pMobIndex->area || pMobIndex->area->under_develop)
        continue;
      if(pMobIndex->pShop)
        continue;
      if (IS_SET(pMobIndex->act,ACT_TRAIN)
         ||  IS_SET(pMobIndex->act,ACT_PRACTICE)
         ||  IS_SET(pMobIndex->act,ACT_IS_HEALER)
         ||  IS_SET(pMobIndex->act,ACT_IS_CHANGER)
         ||  IS_SET(pMobIndex->act, ACT_IS_ARMOURER)
         ||  IS_SET(pMobIndex->act, ACT_IS_WEAPONSMITH))
         continue;
      if(pMobIndex->vnum >= VNUM_SINS_START && pMobIndex->vnum <= VNUM_SINS_END)
        continue;
      for ( mob = char_list; mob != NULL; mob = mob->next )
      {
        if(IS_NPC(mob) && mob->pIndexData == pMobIndex)
        {
          int iClass, iGuild;
          if (mob->in_room == NULL || mob->zone != mob->in_room->area)
            continue;
          if(room_is_private(mob, mob->in_room))
            continue;
          if(IS_SET(mob->in_room->room_flags,(ROOM_GODS_ONLY|ROOM_HEROES_ONLY|
                                             ROOM_IMP_ONLY|ROOM_NEWBIES_ONLY|
                                             ROOM_NOCLAN|ROOM_CLANONLY|ROOM_SAFE
                                             |ROOM_NODIE|ROOM_NOCOMBAT
                                             )))
            continue;// No mobs in rooms with special flags
          for ( iClass = 0; iClass < MAX_CLASS; iClass++ )
          {
              for ( iGuild = 0; iGuild < MAX_GUILD; iGuild ++)
              {
                if(mob->in_room->vnum == class_table[iClass].guild[iGuild])
                  break;
              }
              if(iGuild != MAX_GUILD)
                break;
          }
          if(iClass != MAX_CLASS)
            continue;// No good, in a guild room
          break;// Mob is good
        }
      }
      if(!mob)// Make sure it's got a spawn point
        continue;
      if(pMobIndex->spec_fun)
      {
         for ( j = 0; spec_table[j].name != NULL; j++)
         {
            if (spec_table[j].function == pMobIndex->spec_fun)
            {
              if(spec_table[j].bounty_difficulty > 0)
                pMobIndex->bounty_level = spec_table[j].bounty_difficulty;
              break;
            }
         }
         if(!pMobIndex->bounty_level)
          continue;// Not a suitable spec_fun for a bounty
      }
      else
        pMobIndex->bounty_level = 1;//Base level is 1
      if(pMobIndex->level >= 50)
        pMobIndex->bounty_level++;
      if(pMobIndex->level > 55)
        pMobIndex->bounty_level++;
      if(IS_SET(pMobIndex->off_flags, OFF_BANE_TOUCH))
        pMobIndex->bounty_level++; 
      pMobIndex->bounty_level = UMIN(pMobIndex->bounty_level, MAX_BOUNTY_LEVEL);
      bounty_available[pMobIndex->bounty_level - 1]++;
    }
  }
  // Now select the first bounty
  select_bounty(0);
}

void boot_db( void )
{
  /* needed to keep track of area file names */
  /* help.are and a few others don't have an AREA_DATA */
  AREA_NAME_DATA *area_name;
  char buf[MAX_STRING_LENGTH];

#if defined(unix)
    /* open file fix */
    maxfilelimit();
#endif

    /*
     * Init some data space stuff.
     */
    {
#ifdef GAME_VERSION
  if ( ( string_space = calloc( 1, MAX_STRING ) ) == NULL )
#else
  if ( ( string_space = GC_MALLOC( MAX_STRING ) ) == NULL )
#endif
  {
      bug( "Boot_db: can't alloc %d string space.", MAX_STRING );
      exit( 1 );
  }
  top_string  = string_space;
  fBootDb   = TRUE;
    }    

 sprintf(log_buf,"here1 ");  

    /*
     * Init random number generator.
     */
    {
        init_mm( );
    }

    /*
     * Set time and weather.
     */
    {
  long lhour, lday, lmonth;

  lhour   = (current_time - 650336715)
      / (PULSE_TICK / PULSE_PER_SECOND);
  time_info.hour  = lhour  % 24;
  lday    = lhour  / 24;
  time_info.day = lday   % 35;
  lmonth    = lday   / 35;
  time_info.month = lmonth % 17;
  time_info.year  = lmonth / 17;

       if ( time_info.hour <  5 ) weather_info.sunlight = SUN_DARK;
  else if ( time_info.hour <  6 ) weather_info.sunlight = SUN_RISE;
  else if ( time_info.hour < 19 ) weather_info.sunlight = SUN_LIGHT;
  else if ( time_info.hour < 20 ) weather_info.sunlight = SUN_SET;
  else                            weather_info.sunlight = SUN_DARK;

  weather_info.change = 0;
  weather_info.mmhg = 960;
  if ( time_info.month >= 7 && time_info.month <=12 )
      weather_info.mmhg += number_range( 1, 50 );
  else
      weather_info.mmhg += number_range( 1, 80 );

       if ( weather_info.mmhg <=  980 ) weather_info.sky = SKY_LIGHTNING;
  else if ( weather_info.mmhg <= 1000 ) weather_info.sky = SKY_RAINING;
  else if ( weather_info.mmhg <= 1020 ) weather_info.sky = SKY_CLOUDY;
  else                                  weather_info.sky = SKY_CLOUDLESS;

    }

    /* Set Gladiator Settings */
    gladiator_info.started = FALSE;
    gladiator_info.time_left = 0;
    gladiator_info.min_level = 0;
    gladiator_info.max_level = 0;
    gladiator_info.type = 0;
    gladiator_info.playing = 0;
    gladiator_info.team_counter = 0;
    gladiator_info.gladiator_score = 0;
    gladiator_info.barbarian_score = 0;
    gladiator_info.bet_counter = 0;

    /*
     * Assign gsn's for skills which have them.
     */
    {
  int sn;

  for ( sn = 0; sn < MAX_SKILL; sn++ )
  {
      if ( skill_table[sn].pgsn != NULL )
    *skill_table[sn].pgsn = sn;
  }
    }

    /*
     * Read in all the area files.
     */
    {
  FILE *fpList;

  load_new_helps(&help_first, &help_last, &top_help);//New help code

  if ( ( fpList = fopen( AREA_LIST, "r" ) ) == NULL )
  {
      perror( AREA_LIST );
      exit( 1 );
  }
  
  area_name_first = NULL;  
    
  for ( ; ; )
  {
    int min_vnum;
    
      strcpy( strArea, fread_word( fpList ) );
log_string(strArea);
      if ( strArea[0] == '$' )
    break;

      if ( strArea[0] == '-' )
      {
    fpArea = stdin;
      }
      else
      {
    if ( ( fpArea = fopen( strArea, "r" ) ) == NULL )
    {
        perror( strArea );
        sprintf (buf,"Skipping area '%s'",strArea);
        log_string (buf);        
        continue;
    }
      }      
     
#ifdef OLC_VERSION
      area_name = alloc_perm (sizeof (AREA_NAME_DATA));
#else /*game version*/
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

      for ( ; ; )
      {
    char *word;

    if ( fread_letter( fpArea ) != '#' )
    {
        bug( "Boot_db: # not found.", 0 );
        if ( fpArea != stdin )
          fclose( fpArea );
        fpArea = NULL;
        rename_area (strArea);        
        exit( 1 );
    }

    word = fread_word( fpArea );

         if ( word[0] == '$'               )                 break;
    else if ( !str_cmp( word, "AREA"     ) ) load_area    (fpArea,strArea);
    else if ( !str_cmp( word, "CSTAT"    ) ) load_cstat   (fpArea);
    else if ( !str_cmp( word, "HELPS"    ) )
    {// New help code
      if(!new_helps)
        load_helps   (fpArea);
      else
        break;
    }
    else if ( !str_cmp( word, "MOBOLD"   ) ) load_old_mob (fpArea);
    else if ( !str_cmp( word, "MOBILES"  ) ) load_mobiles (fpArea);
    else if ( !str_cmp( word, "OBJOLD"   ) ) load_old_obj (fpArea);
      else if ( !str_cmp( word, "OBJECTS"  ) ) load_objects (fpArea);
    else if ( !str_cmp( word, "RESETS"   ) ) load_resets  (fpArea);
    else if ( !str_cmp( word, "RECIPES"  ) ) load_recipes (fpArea);
    else if ( !str_cmp( word, "ROOMS"    ) ) load_rooms   (fpArea);
    else if ( !str_cmp( word, "SHOPS"    ) ) load_shops   (fpArea);
    else if ( !str_cmp( word, "SOCIALS"  ) ) load_socials (fpArea);
    else if ( !str_cmp( word, "SPECIALS" ) ) load_specials(fpArea);
    else
    {
        bug( "Boot_db: bad section name.", 0 );
        if ( fpArea != stdin )
          fclose( fpArea );
        fpArea = NULL;
        rename_area (strArea);                
        exit( 1 );
    }
      }

      if ( fpArea != stdin )
        fclose( fpArea );
      fpArea = NULL;
      min_vnum = get_area_min_vnum (area_last);
      if (!area_last->min_vnum_room) area_last->min_vnum_room = min_vnum;
      if (!area_last->max_vnum_room) area_last->max_vnum_room = min_vnum;
      if (!area_last->min_vnum_obj)  area_last->min_vnum_obj  = min_vnum;
      if (!area_last->max_vnum_obj)  area_last->max_vnum_obj  = min_vnum;
      if (!area_last->min_vnum_mob)  area_last->min_vnum_mob  = min_vnum;
      if (!area_last->max_vnum_mob)  area_last->max_vnum_mob  = min_vnum;
        
  }
  fclose( fpList );
    }

  area_last = NULL;

    /*
     * Fix up exits.
     * Declare db booting over.
     * Reset all areas once.
     * Load up the songs, notes and ban files.
     */
    {
  fix_exits( );
  fBootDb = FALSE;
  area_update( );
  load_notes( );
  load_bans();
  load_dns();
  load_clans();
  load_pits();
  verify_price_table();
  /* load_songs(); */
  // Link the help files regardless of load method
  create_help_tracks();
  mark_mob_bounties();
    }

    return;
}



/*
 * Snarf an 'area' header line.
 */
void load_area( FILE *fp, char *file_name )
{
    AREA_DATA *pArea,*temp_last;     

#ifdef OLC_VERSION
    pArea   = alloc_perm( sizeof(*pArea) );
#else /*game version*/
    pArea   = GC_MALLOC( sizeof(*pArea) );    
#endif
    temp_last = area_last;
    area_last = pArea;
    pArea->file_name     = str_dup (file_name); 
    pArea->reset_first  = NULL;
    pArea->reset_last = NULL;
    fread_string(fp);
    pArea->name   = fread_string( fp );
    pArea->credits  = fread_string( fp );
    fread_number(fp);
    fread_number(fp);
    pArea->age    = 100;
    pArea->nplayer  = 0;
    pArea->empty  = FALSE;
    pArea->min_vnum_room = 
    pArea->max_vnum_room = 
    pArea->min_vnum_obj  = 
    pArea->max_vnum_obj  =     
    pArea->min_vnum_mob  = 
    pArea->max_vnum_mob  = 0;
    pArea->freeze        = FALSE;
    pArea->new_area      = FALSE;
    pArea->under_develop = (strstr (file_name,"newareas") != NULL);
    pArea->no_transport	 = FALSE;

    if ( area_first == NULL )
  area_first = pArea;
    if ( temp_last  != NULL )
  temp_last->next = pArea;
    
    pArea->next = NULL;
    top_area++;
    return;
}

/*
 * Snarf a clan stat section.
 */
void load_cstat( FILE *fp )
{
    CSTAT_DATA *cstat;
    char buf[MAX_STRING_LENGTH];
    char *word;

    for ( ; ; )
    {
#ifdef OLC_VERSION
       cstat   = alloc_perm( sizeof(*cstat) );
#else /*game version*/
       cstat   = GC_MALLOC( sizeof(*cstat) );
#endif

       word = fread_word(fp);
       if ( word[0] == '$' )
          break;
       cstat->clan = nonclan_lookup(word);
       cstat->kills = fread_number ( fp );

 sprintf(buf,"Cstat: %d kills %d",cstat->clan,cstat->kills);
 log_string(buf);

      if (cstat_first == NULL)
          cstat_first = cstat;
       if (cstat_last  != NULL )
          cstat_last->next = cstat;

       cstat_last = cstat;
       cstat->next = NULL;

       top_cstat++;
    }
    return;
}

void load_recipes( FILE *fp)
{
	RECIPE_DATA *recipe;
 	int i, num;
 	char test;
	
	for ( ; ; )
	{
#ifdef OLC_VERSION
		recipe = alloc_perm ( sizeof (*recipe));
#else
		recipe = GC_MALLOC( sizeof (*recipe) );
#endif
		test = fread_letter(fp);
		if (test == '$')
			break;
	
		recipe->recipe_num = fread_number(fp);
	        recipe->skill_sn = skill_lookup( fread_word(fp) );
		recipe->difficulty = fread_number(fp);
		recipe->vnum_container = fread_number(fp);
		recipe->vnum_complete = fread_number(fp);
		for ( i = 0 ; i < MAX_IN_RECIPE ; i++ )
		{
			num = fread_number(fp);
			if (num != 0 )
				recipe->vnum_parts[i] = num;
			else
				break;
		}

                if (recipe_first == NULL)
                     recipe_first = recipe;
                if (recipe_last  != NULL )
                   recipe_last->next = recipe;

                recipe_last = recipe;
		recipe->next = NULL;
		
		top_recipe++;
	}
	return;
}

// New help code
//Note: mishandles high # symbols "{|}~" but these shouldn't be showing up 
bool alphabet_before(char *first, char *second)
{// Returns TRUE if first is before second alphabetically. Numbers are early.
  int i = 0;
  while(first[i] && second[i])
  {
    if(UPPER(first[i]) == UPPER(second[i]))
      i++;
    else
    {
      if(UPPER(first[i]) < UPPER(second[i]))
        return TRUE;
      else
        return FALSE;
    }
  }
  if(!first[i])// Ran out of first one first, so it should go in front
    return TRUE;
  return FALSE;
}

void create_help_tracks(void)
{
  int i;
  HELP_DATA *pHelp;
  HELP_TRACKER *phTracker, *phWalker;
  char *base, part[255];

  for(pHelp = help_first; pHelp; pHelp = pHelp->next)
  {
    base = one_argument(pHelp->keyword, part);
    
    while(part[0] != '\0')
    {// Break it down into the various keywords
    //#ifdef OLC_VERSION
    //  phTracker   = alloc_perm( sizeof(*phTracker) );
    //#else /*game version*/
      phTracker   = GC_MALLOC( sizeof(*phTracker) );
    //#endif
      phTracker->help = pHelp;
      clear_string(&phTracker->keyword, part);
      if(!help_track_first ||
        !alphabet_before(help_track_first->keyword, phTracker->keyword))
      {// First in the list
        if(!help_track_first)
          help_track_first = phTracker;
        else
        {
          phTracker->next = help_track_first;
          help_track_first->prev = phTracker;
          help_track_first = phTracker;
        }
      }
      else
      {// Find where in the list it goes
        phWalker = help_track_first;
        while(phWalker->next && 
          alphabet_before(phWalker->next->keyword, phTracker->keyword))
          phWalker = phWalker->next;
        if(phWalker->next)
          phWalker->next->prev = phTracker;
        phTracker->next = phWalker->next;
        phTracker->prev = phWalker;
        phWalker->next = phTracker;
      }
      base = one_argument(base, part);
    }
  }
  // Go through and assign the shortcuts for each of the trackers
  i = 0;
  for(i = 0; i < 27; i++)
  {
    help_tracks[i] = NULL;
  }
  for(phWalker = help_track_first; phWalker; phWalker = phWalker->next)
  {
    if(UPPER(phWalker->keyword[0]) >= 'A' && UPPER(phWalker->keyword[0]) <= 'Z')
    {
      if(!help_tracks[UPPER(phWalker->keyword[0]) - 'A' + 1])
        help_tracks[UPPER(phWalker->keyword[0]) - 'A' + 1] = phWalker;
    }
    else if(!help_tracks[0])// Anything not A-Z goes into 0
      help_tracks[0] = phWalker;
  }
}

void load_new_helps(HELP_DATA **first, HELP_DATA **last, int *counter)
{
  HELP_DATA *pHelp;
  char buf[255];
  FILE *fp = fopen(HELP_FILE, "r");
  if(!fp)
  {// Load the old help files
    bug("Unable to open new help file.\n\r", 0);
    return;
  }
  new_helps = TRUE;
  for ( ; ; )
  {
  //#ifdef OLC_VERSION
  //  pHelp   = alloc_perm( sizeof(*pHelp) );
  //#else /*game version*/
    pHelp   = GC_MALLOC( sizeof(*pHelp) );
  //#endif
    pHelp->level  = fread_number( fp );
    pHelp->keyword  = fread_string( fp );
    if ( pHelp->keyword[0] == '$' )
        break;
    
    pHelp->related = fread_string( fp );
    if(pHelp->related)
    {
      int i;
      pHelp->related[0] = UPPER(pHelp->related[0]);
      for(i = 1; i < strlen(pHelp->related); i++)
      {
        if(pHelp->related[i - 1] == ' ' || pHelp->related[i - 1] == 39)
          pHelp->related[i] = UPPER(pHelp->related[i]);
        else
          pHelp->related[i] = LOWER(pHelp->related[i]);
      }
    }
    
    fscanf(fp, "%ld %d %s",  &pHelp->modified, &pHelp->status, buf);

    if(!pHelp->status)
      pHelp->status = HELP_STAT_LEGACY;

    if(str_cmp(buf, "none"))
      pHelp->editor = str_dup(buf);
    else
      pHelp->editor = NULL;

    pHelp->text = fread_string( fp );

    if ( !str_cmp( pHelp->keyword, "greeting" ) )
        help_greeting = pHelp->text;
  
    if ( *first == NULL )
        *first = pHelp;
    if ( *last  != NULL )
        (*last)->next = pHelp;
  
    *last = pHelp;
    pHelp->next = NULL;
    *counter++;
  }
  fclose(fp);
}

/*
 * Snarf a help section.
 */
void load_helps( FILE *fp )
{
    HELP_DATA *pHelp;

  if(new_helps)// New help code
  {
    return;// If it's already loaded the new help file, don't load any oldstyle
  }

    for ( ; ; )
    {
#ifdef OLC_VERSION
  pHelp   = alloc_perm( sizeof(*pHelp) );
#else /*game version*/
  pHelp   = GC_MALLOC( sizeof(*pHelp) );
#endif
  pHelp->level  = fread_number( fp );
  pHelp->keyword  = fread_string( fp );
  if ( pHelp->keyword[0] == '$' )
      break;
  pHelp->text = fread_string( fp );

  if ( !str_cmp( pHelp->keyword, "greeting" ) )
      help_greeting = pHelp->text;

  if ( help_first == NULL )
      help_first = pHelp;
  if ( help_last  != NULL )
      help_last->next = pHelp;

  help_last = pHelp;
  pHelp->next = NULL;
  top_help++;
    }

    return;
}



/*
 * Snarf a mob section.  old style 
 */
void load_old_mob( FILE *fp )
{
    MOB_INDEX_DATA *pMobIndex;
    /* for race updating */
    int race;
    char name[MAX_STRING_LENGTH];

    for ( ; ; )
    {
  sh_int vnum;
  char letter;
  int iHash;

  letter        = fread_letter( fp );
  if ( letter != '#' )
  {
      bug( "Load_old_mobiles: # not found.", 0 );
      if ( fp != stdin ) fclose( fp );
      fp = NULL;
      if (area_last)
        rename_area (area_last->file_name);              
      exit( 1 );
  }

  vnum        = fread_number( fp );
  if ( vnum == 0 )
      break;

  fBootDb = FALSE;
  if ( get_mob_index( vnum ) != NULL )
  {
      bug( "Load_old_mobiles: vnum %d duplicated.", vnum );
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
  pMobIndex     = alloc_perm( sizeof(*pMobIndex) );
#else /*game version*/
  pMobIndex     = GC_MALLOC( sizeof(*pMobIndex) );
#endif
  pMobIndex->vnum     = vnum;
  pMobIndex->new_format   = FALSE;
  pMobIndex->player_name    = fread_string( fp );
  pMobIndex->short_descr    = fread_string( fp );
  pMobIndex->long_descr   = fread_string( fp );
  pMobIndex->description    = fread_string( fp );

  pMobIndex->long_descr[0]  = UPPER(pMobIndex->long_descr[0]);
  pMobIndex->description[0] = UPPER(pMobIndex->description[0]);

  pMobIndex->act      = fread_flag( fp ) | ACT_IS_NPC;
  pMobIndex->affected_by    = fread_flag( fp );
  pMobIndex->pShop    = NULL;
  pMobIndex->alignment    = fread_number( fp );
  letter        = fread_letter( fp );
  pMobIndex->level    = fread_number( fp );

  /*
   * The unused stuff is for imps who want to use the old-style
   * stats-in-files method.
   */
            fread_number( fp ); /* Unused */
            fread_number( fp ); /* Unused */
            fread_number( fp ); /* Unused */
  /* 'd'    */      fread_letter( fp ); /* Unused */
            fread_number( fp ); /* Unused */
  /* '+'    */      fread_letter( fp ); /* Unused */
            fread_number( fp ); /* Unused */
            fread_number( fp ); /* Unused */
  /* 'd'    */      fread_letter( fp ); /* Unused */
            fread_number( fp ); /* Unused */
  /* '+'    */      fread_letter( fp ); /* Unused */
            fread_number( fp ); /* Unused */
        pMobIndex->wealth               = fread_number( fp )/20;  
  /* xp can't be used! */     fread_number( fp ); /* Unused */
  pMobIndex->start_pos    = fread_number( fp ); /* Unused */
  pMobIndex->default_pos    = fread_number( fp ); /* Unused */

    if (pMobIndex->start_pos < POS_SLEEPING)
      pMobIndex->start_pos = POS_STANDING;
  if (pMobIndex->default_pos < POS_SLEEPING)
      pMobIndex->default_pos = POS_STANDING;

  /*
   * Back to meaningful values.
   */
  pMobIndex->sex      = fread_number( fp );

      /* compute the race BS */
    one_argument(pMobIndex->player_name,name);
 
    if (name[0] == '\0' || (race =  race_lookup(name)) == 0)
    {
            /* fill in with blanks */
            pMobIndex->race = race_lookup("human");
            pMobIndex->off_flags = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_VNUM;
            pMobIndex->imm_flags = 0;
            pMobIndex->res_flags = 0;
            pMobIndex->vuln_flags = 0;
            pMobIndex->form = FORM_EDIBLE|FORM_SENTIENT|FORM_BIPED|FORM_MAMMAL;
            pMobIndex->parts = PART_HEAD|PART_ARMS|PART_LEGS|PART_HEART|
                               PART_BRAINS|PART_GUTS;
      }
      else
      {
            pMobIndex->race = race;
            pMobIndex->off_flags = OFF_DODGE|OFF_DISARM|OFF_TRIP|ASSIST_RACE|
                                   race_table[race].off;
            pMobIndex->imm_flags = race_table[race].imm;
            pMobIndex->res_flags = race_table[race].res;
            pMobIndex->vuln_flags = race_table[race].vuln;
            pMobIndex->form = race_table[race].form;
            pMobIndex->parts = race_table[race].parts;
      }

  if ( letter != 'S' )
  {
      bug( "Load_old_mobiles: vnum %d non-S.", vnum );
      if ( fp != stdin ) fclose( fp );
      fp = NULL; 
      if (area_last)
        rename_area (area_last->file_name);                    
      exit( 1 );
  }

  iHash     = vnum % MAX_KEY_HASH;
  pMobIndex->next   = mob_index_hash[iHash];
  mob_index_hash[iHash] = pMobIndex;
  top_mob_index++;
  kill_table[URANGE(0, pMobIndex->level, MAX_LEVEL-1)].number++;
    }

    return;
}

/*
 * Snarf an obj section.  old style 
 */
void load_old_obj( FILE *fp )
{
    OBJ_INDEX_DATA *pObjIndex;

    for ( ; ; )
    {
  sh_int vnum;
  char letter;
  int iHash;

  letter        = fread_letter( fp );
  if ( letter != '#' )
  {
      bug( "Load_objects: # not found.", 0 );
      if ( fp != stdin ) fclose( fp );
      fp = NULL; 
      if (area_last)
        rename_area (area_last->file_name);                    
      exit( 1 );
  }

  vnum        = fread_number( fp );
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
  pObjIndex     = alloc_perm( sizeof(*pObjIndex) );
#else /*game version*/
  pObjIndex     = GC_MALLOC( sizeof(*pObjIndex) );
#endif
  pObjIndex->vnum     = vnum;
  pObjIndex->new_format   = FALSE;
  pObjIndex->reset_num    = 0;
  pObjIndex->name     = fread_string( fp );
  pObjIndex->short_descr    = fread_string( fp );
  pObjIndex->description    = fread_string( fp );
  /* Action description */    fread_string( fp );

  pObjIndex->short_descr[0] = LOWER(pObjIndex->short_descr[0]);
  pObjIndex->description[0] = UPPER(pObjIndex->description[0]);
  pObjIndex->material   = str_dup("");

  pObjIndex->item_type    = fread_number( fp );
  pObjIndex->extra_flags    = fread_flag( fp );
  pObjIndex->wear_flags   = fread_flag( fp );
  pObjIndex->value[0]   = fread_number( fp );
  pObjIndex->value[1]   = fread_number( fp );
  pObjIndex->value[2]   = fread_number( fp );
  pObjIndex->value[3]   = fread_number( fp );
  pObjIndex->value[4]   = 0;
  pObjIndex->level    = 0;
  pObjIndex->condition    = 100;
  pObjIndex->weight   = fread_number( fp );
  pObjIndex->cost     = fread_number( fp ); /* Unused */
  /* Cost per day */      fread_number( fp );


  if (pObjIndex->item_type == ITEM_WEAPON)
  {
      if (is_name("two",pObjIndex->name) 
      ||  is_name("two-handed",pObjIndex->name) 
      ||  is_name("claymore",pObjIndex->name))
    SET_BIT(pObjIndex->value[4],WEAPON_TWO_HANDS);
  }

  for ( ; ; )
  {
      char letter;

      letter = fread_letter( fp );

      if ( letter == 'A' )
      {
    AFFECT_DATA *paf;

#ifdef OLC_VERSION
    paf     = alloc_perm( sizeof(*paf) );
#else
    paf     = GC_MALLOC( sizeof(*paf) );
#endif
    paf->where    = TO_OBJECT;
    paf->type   = -1;
    paf->level    = 20; /* RT temp fix */
    paf->duration   = -1;
    paf->location   = fread_number( fp );
    paf->modifier   = fread_number( fp );
    paf->bitvector    = 0;
    paf->next   = pObjIndex->affected;
    pObjIndex->affected = paf;
    top_affect++;
      }

      else if ( letter == 'E' )
      {
    EXTRA_DESCR_DATA *ed;

#ifdef OLC_VERSION
    ed      = alloc_perm( sizeof(*ed) );
#else
    ed      = GC_MALLOC( sizeof(*ed) );
#endif
    ed->keyword   = fread_string( fp );
    ed->description   = fread_string( fp );
    ed->next    = pObjIndex->extra_descr;
    pObjIndex->extra_descr  = ed;
    top_ed++;
      }

      else
      {
    ungetc( letter, fp );
    break;
      }
  }

        /* fix armors */
        if (pObjIndex->item_type == ITEM_ARMOR)
        {
            pObjIndex->value[1] = pObjIndex->value[0];
            pObjIndex->value[2] = pObjIndex->value[1];
        }

  /*
   * Translate spell "slot numbers" to internal "skill numbers."
   */
  switch ( pObjIndex->item_type )
  {
  case ITEM_PILL:
  case ITEM_POTION:
  case ITEM_SCROLL:
      pObjIndex->value[1] = slot_lookup( pObjIndex->value[1] );
      pObjIndex->value[2] = slot_lookup( pObjIndex->value[2] );
      pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
      pObjIndex->value[4] = slot_lookup( pObjIndex->value[4] );
      break;

  case ITEM_STAFF:
  case ITEM_WAND:
      pObjIndex->value[3] = slot_lookup( pObjIndex->value[3] );
      break;
  }

  iHash     = vnum % MAX_KEY_HASH;
  pObjIndex->next   = obj_index_hash[iHash];
  obj_index_hash[iHash] = pObjIndex;
  top_obj_index++;
    }

    return;
}





/*
 * Snarf a reset section.
 */
void load_resets( FILE *fp )
{
    RESET_DATA *pReset;
    bool error_found;

    if ( area_last == NULL )
    {
  bug( "Load_resets: no #AREA seen yet.", 0 );  
  exit( 1 );
    }

    for ( ; ; )
    {
  char letter;

  if ( ( letter = fread_letter( fp ) ) == 'S' )
      break;

  if ( letter == '*' )
  {
      fread_to_eol( fp );
      continue;
  }

#ifdef OLC_VERSION
  pReset    = alloc_perm( sizeof(*pReset) );
#else
  pReset    = GC_MALLOC( sizeof(*pReset) );
#endif
  pReset->command = letter;
  /* if_flag */   fread_number( fp );
  pReset->arg1  = fread_number( fp );
  pReset->arg2  = fread_number( fp );
  pReset->arg3  = (letter == 'G' || letter == 'R')
          ? 0 : fread_number( fp );
  pReset->arg4  = (letter == 'P' || letter == 'M')
          ? fread_number(fp) : 0;
        fread_to_eol( fp );
        
  error_found = FALSE;        
  /*
   * Validate parameters.
   * We're calling the index functions for the side effect.
   */
  switch ( letter )
  {
  default:
      bug( "Load_resets: bad command '%c'.", letter );
      error_found = TRUE;      
      break;
  case 'M':  
  case 'O':
  case 'P':
  case 'G':
  case 'E':
      break; 
  case 'D':
/* This is obsolete *
      if ( pReset->arg3 < 0 || pReset->arg3 > 2 )
      {
    bug( "Load_resets: 'D': bad 'locks': %d.", pReset->arg3 );
    error_found = TRUE;          
      }
 */
      break;
  case 'R':
      if ( pReset->arg2 < 0 || pReset->arg2 > 6 )
      {
    bug( "Load_resets: 'R': bad exit %d.", pReset->arg2 );
    error_found = TRUE;
      }
      break;
  }

  if (!error_found) {
    if ( area_last->reset_first == NULL )
        area_last->reset_first  = pReset;
    if ( area_last->reset_last  != NULL )
        area_last->reset_last->next = pReset;
        
    area_last->reset_last = pReset;
    pReset->next    = NULL;
    top_reset++;
  }
    }

    return;
}



/*
 * Snarf a room section.
 */
void load_rooms( FILE *fp )
{
    ROOM_INDEX_DATA *pRoomIndex;

    if ( area_last == NULL )
    {
  bug( "Load_resets: no #AREA seen yet.", 0 );
  exit( 1 );
    }

    for ( ; ; )
    {
  sh_int vnum;
  char letter;
  int door;
  int iHash;

  letter        = fread_letter( fp );
  if ( letter != '#' )
  {
      bug( "Load_rooms: # not found.", 0 );      
      if ( fp != stdin ) fclose( fp );
      fp = NULL; 
      if (area_last)
        rename_area (area_last->file_name);      
      exit( 1 );
  }

  vnum        = fread_number( fp );
  if ( vnum == 0 )
      break;

  fBootDb = FALSE;
  if ( get_room_index( vnum ) != NULL )
  {
      bug( "Load_rooms: vnum %d duplicated.", vnum );
      if ( fp != stdin ) fclose( fp );
      fp = NULL; 
      if (area_last)
        rename_area (area_last->file_name);              
      exit( 1 );
  }
  fBootDb = TRUE;
  
  if (area_last->min_vnum_room == 0) {
    area_last->min_vnum_room = vnum;
    area_last->max_vnum_room = vnum;  
  } else {  
    if (vnum > area_last->max_vnum_room)
      area_last->max_vnum_room = vnum;
    if (vnum < area_last->min_vnum_room)
      area_last->min_vnum_room = vnum;
  }      

#ifdef OLC_VERSION
  pRoomIndex      = alloc_perm( sizeof(*pRoomIndex) );
#else
  pRoomIndex      = GC_MALLOC( sizeof(*pRoomIndex) );
#endif
  pRoomIndex->owner   = str_dup("");
  pRoomIndex->people    = NULL;
  pRoomIndex->contents    = NULL;
  pRoomIndex->extra_descr   = NULL;
  pRoomIndex->area    = area_last;
  pRoomIndex->vnum    = vnum;
  pRoomIndex->name    = fread_string( fp );
  pRoomIndex->description   = fread_string( fp );
  /* Area number */     fread_number( fp );
  pRoomIndex->room_flags    = fread_flag( fp );
  /* horrible hack */
    if ( 3000 <= vnum && vnum < 3400)
     SET_BIT(pRoomIndex->room_flags,ROOM_LAW);
  pRoomIndex->sector_type   = fread_number( fp );
  pRoomIndex->light   = 0;
  for ( door = 0; door <= 5; door++ )
      pRoomIndex->exit[door] = NULL;

  /* defaults */
  pRoomIndex->heal_rate = 100;
  pRoomIndex->mana_rate = 100;
  pRoomIndex->obs_target = 0;

  for ( ; ; )
  {
      letter = fread_letter( fp );

      if ( letter == 'S' )
    break;

      if ( letter == 'H') /* healing room */
    pRoomIndex->heal_rate = fread_number(fp);
  
      else if ( letter == 'B') /* observation room */
	pRoomIndex->obs_target = fread_number(fp);

      else if ( letter == 'M') /* mana room */
    pRoomIndex->mana_rate = fread_number(fp);

     else if ( letter == 'C') /* clan */
     {
    if (pRoomIndex->clan)
      {
        bug("Load_rooms: duplicate clan fields.",0);        
      }
    pRoomIndex->clan = nonclan_lookup(fread_string(fp));
      }
  

      else if ( letter == 'D' )
      {
    EXIT_DATA *pexit;
    int locks;

    door = fread_number( fp );
    if ( door < 0 || door > 5 )
    {
        bug( "Fread_rooms: vnum %d has bad door number.", vnum );        
    }

#ifdef OLC_VERSION
    pexit     = alloc_perm( sizeof(*pexit) );
#else
    pexit     = GC_MALLOC( sizeof(*pexit) );
#endif
    		fread_string( fp );
    pexit->keyword    = fread_string( fp );
    pexit->exit_info  = 0;
    locks     = fread_number( fp );
    pexit->key    = fread_number( fp );
    pexit->u1.vnum    = fread_number( fp );

    if (!IS_SET(locks,EX_NEW_FORMAT) )
    {
        switch ( locks ) 
        {
        case 1: pexit->exit_info = EX_ISDOOR;                break;
        case 2: pexit->exit_info = EX_ISDOOR | EX_PICKPROOF; break;
        case 3: pexit->exit_info = EX_ISDOOR | EX_NOPASS;    break;
        case 4: pexit->exit_info = EX_ISDOOR|EX_NOPASS|EX_PICKPROOF;
          break;
        }
    }
    else
	pexit->exit_info = locks;
	
    pRoomIndex->exit[door]  = pexit;    
    top_exit++;
      }
      else if ( letter == 'E' )
      {
    EXTRA_DESCR_DATA *ed;

#ifdef OLC_VERSION
    ed      = alloc_perm( sizeof(*ed) );
#else
    ed      = GC_MALLOC( sizeof(*ed) );
#endif
    ed->keyword   = fread_string( fp );
    ed->description   = fread_string( fp );
    ed->next    = pRoomIndex->extra_descr;
    pRoomIndex->extra_descr = ed;
    top_ed++;
      }

      else if (letter == 'O')
      {
    if (pRoomIndex->owner[0] != '\0')
    {
        bug("Load_rooms: duplicate owner.",0);        
    }

    pRoomIndex->owner = fread_string(fp);
      }

      else
      {
    bug( "Load_rooms: vnum %d has flag not 'DES'.", vnum );    
      }
  }

  iHash     = vnum % MAX_KEY_HASH;
  pRoomIndex->next  = room_index_hash[iHash];
  room_index_hash[iHash]  = pRoomIndex;
  top_room++;
    }

    return;
}



/*
 * Snarf a shop section.
 */
void load_shops( FILE *fp )
{
    SHOP_DATA *pShop;

    for ( ; ; )
    {
  MOB_INDEX_DATA *pMobIndex;
  int iTrade;

#ifdef OLC_VERSION
  pShop     = alloc_perm( sizeof(*pShop) );
#else
  pShop     = GC_MALLOC( sizeof(*pShop) );
#endif
  pShop->keeper   = fread_number( fp );
  if ( pShop->keeper == 0 )
      break;
  for ( iTrade = 0; iTrade < MAX_TRADE; iTrade++ )
      pShop->buy_type[iTrade] = fread_number( fp );
  pShop->profit_buy = fread_number( fp );
  pShop->profit_sell  = fread_number( fp );
  pShop->open_hour  = fread_number( fp );
  pShop->close_hour = fread_number( fp );
          fread_to_eol( fp );
  pMobIndex   = get_mob_index( pShop->keeper );
  pMobIndex->pShop  = pShop;

  if ( shop_first == NULL )
      shop_first = pShop;
  if ( shop_last  != NULL )
      shop_last->next = pShop;

  shop_last = pShop;
  pShop->next = NULL;
  top_shop++;
    }

    return;
}


/*
 * Snarf spec proc declarations.
 */
void load_specials( FILE *fp )
{
    for ( ; ; )
    {
  MOB_INDEX_DATA *pMobIndex;
  char letter;

  switch ( letter = fread_letter( fp ) )
  {
  default:
      bug( "Load_specials: letter '%c' not *MS.", letter );
      if ( fp != stdin ) fclose( fp );
      fp = NULL; 
      if (area_last)
        rename_area (area_last->file_name);              
      exit( 1 );

  case 'S':
      return;

  case '*':
      break;

  case 'M':
      pMobIndex   = get_mob_index ( fread_number ( fp ) );
      pMobIndex->spec_fun = spec_lookup ( fread_word   ( fp ) );
      if ( pMobIndex->spec_fun == 0 )
      {
    bug( "Load_specials: 'M': vnum %d.", pMobIndex->vnum );
    if ( fp != stdin ) fclose( fp );
    fp = NULL;
    if (area_last)
      rename_area (area_last->file_name);            
    exit( 1 );
      }
      break;
  }

  fread_to_eol( fp );
    }
}


/*
 * Translate all room exits from virtual to real.
 * Has to be done after all rooms are read in.
 * Check for bad reverse exits.
 */
void fix_exits( void )
{
    /*extern const sh_int rev_dir [];*/
   /* char buf[MAX_STRING_LENGTH];*/
    ROOM_INDEX_DATA *pRoomIndex;
   /* ROOM_INDEX_DATA *to_room; */
    EXIT_DATA *pexit;
   /* EXIT_DATA *pexit_rev; */
    int iHash;
    int door;

    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
  for ( pRoomIndex  = room_index_hash[iHash];
        pRoomIndex != NULL;
        pRoomIndex  = pRoomIndex->next )
  {
      bool fexit;

      fexit = FALSE;
      for ( door = 0; door <= 5; door++ )
      {
    if ( ( pexit = pRoomIndex->exit[door] ) != NULL )
    {
        if ( pexit->u1.vnum <= 0 
        || get_room_index(pexit->u1.vnum) == NULL) {
      pexit->u1.to_room = NULL;
      pRoomIndex->exit[door] = NULL;
        } else {
      fexit = TRUE; 
      pexit->u1.to_room = get_room_index( pexit->u1.vnum );
        }
    }
      }
      if (!fexit)
    SET_BIT(pRoomIndex->room_flags,ROOM_NO_MOB);
  }
    }

/*
    for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
    {
  for ( pRoomIndex  = room_index_hash[iHash];
        pRoomIndex != NULL;
        pRoomIndex  = pRoomIndex->next )
  {
      for ( door = 0; door <= 5; door++ )
      {
    if ( ( pexit     = pRoomIndex->exit[door]       ) != NULL
    &&   ( to_room   = pexit->u1.to_room            ) != NULL
    &&   ( pexit_rev = to_room->exit[rev_dir[door]] ) != NULL
    &&   pexit_rev->u1.to_room != pRoomIndex 
    &&   (pRoomIndex->vnum < 1200 || pRoomIndex->vnum > 1299))
    {
        sprintf( buf, "Fix_exits: %d:%d -> %d:%d -> %d.",
      pRoomIndex->vnum, door,
      to_room->vnum,    rev_dir[door],
      (pexit_rev->u1.to_room == NULL)
          ? 0 : pexit_rev->u1.to_room->vnum );
        bug( buf, 0 );
    }
      }
  }
    }
 */
    return;
}



/*
 * Repopulate areas periodically.
 */
void area_update( void )
{
    AREA_DATA *pArea;
    char buf[MAX_STRING_LENGTH];

    for ( pArea = area_first; pArea != NULL; pArea = pArea->next )
    {

  if ( ++pArea->age < 3 )
      continue;

  /*
   * Check age and reset.
   * Note: Mud School resets every 3 minutes (not 15).
   */
   
  if ( ((!pArea->empty && (pArea->nplayer == 0 || pArea->age >= 15))
  ||    pArea->age >= 31) && !pArea->freeze) 
  {
      ROOM_INDEX_DATA *pRoomIndex;
      ROOM_INDEX_DATA *honorIndex;
      ROOM_INDEX_DATA *posseIndex;
      ROOM_INDEX_DATA *posse2Index;
      ROOM_INDEX_DATA *warlockIndex;
      ROOM_INDEX_DATA *zealotIndex;
      ROOM_INDEX_DATA *zealot2Index;

      reset_area( pArea );
      sprintf(buf,"%s has just been reset.",pArea->name);
      wiznet(buf,NULL,NULL,WIZ_RESETS,0,0);

      pArea->age = number_range( 0, 3 );
      pRoomIndex = get_room_index( ROOM_VNUM_SCHOOL );
      honorIndex = get_room_index( ROOM_VNUM_HONOR_INDEX );
      posseIndex = get_room_index( ROOM_VNUM_POSSE_INDEX );
      posse2Index = get_room_index( 31603 );
      warlockIndex = get_room_index( ROOM_VNUM_WARLOCK_INDEX);
      zealotIndex = get_room_index( ROOM_VNUM_ZEALOT_INDEX );
      zealot2Index = get_room_index( 31806 );


      if (
         (pRoomIndex != NULL && pArea == pRoomIndex->area)
      || (honorIndex != NULL && pArea == honorIndex->area)
      || (posseIndex != NULL && pArea == posseIndex->area)
      || (posse2Index != NULL && pArea == posse2Index->area)
      || (warlockIndex != NULL && pArea == warlockIndex->area)
      || (zealotIndex != NULL && pArea == zealotIndex->area)
      || (zealot2Index != NULL && pArea == zealot2Index->area )
       )
        pArea->age = 15 - 2;
      else if (pArea->nplayer == 0)
        pArea->empty = TRUE;
  }


    }

    return;
}



/*
 * Reset one area.
 */
void reset_area( AREA_DATA *pArea )
{
    RESET_DATA *pReset;
    CHAR_DATA *mob;
    bool last;
    int level;

    mob   = NULL;
    last  = TRUE;
    level = 0;
    for ( pReset = pArea->reset_first; pReset != NULL; pReset = pReset->next )
    {
  ROOM_INDEX_DATA *pRoomIndex;
  MOB_INDEX_DATA *pMobIndex;
  OBJ_INDEX_DATA *pObjIndex;
  OBJ_INDEX_DATA *pObjToIndex;
  EXIT_DATA *pexit;
  OBJ_DATA *obj;
  OBJ_DATA *obj_to;
  int count, limit;

  switch ( pReset->command )
  {
  default:
      bug( "Reset_area: bad command %c.", pReset->command );
      break;

  case 'M':
      if ( ( pMobIndex = get_mob_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'M': bad vnum %d.", pReset->arg1 );
    continue;
      }

      if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
      {
    bug( "Reset_area: 'R': bad vnum %d.", pReset->arg3 );
    continue;
      }

      if ( pMobIndex->count >= pReset->arg2 )
      {
    last = FALSE;
    break;
      }

      count = 0;
      for (mob = pRoomIndex->people; mob != NULL; mob = mob->next_in_room)
    if (mob->pIndexData == pMobIndex)
    {
        count++;
        if (count >= pReset->arg4)
        {
          last = FALSE;
          break;
        }
    }

      if (count >= pReset->arg4)
    break;

      mob = create_mobile( pMobIndex );      

      /*
       * Check for pet shop.
       */
      {
    ROOM_INDEX_DATA *pRoomIndexPrev;
    pRoomIndexPrev = get_room_index( pRoomIndex->vnum - 1 );
    if ( pRoomIndexPrev != NULL
    &&   IS_SET(pRoomIndexPrev->room_flags, ROOM_PET_SHOP) )
        SET_BIT(mob->act, ACT_PET);
      }

      /* set area */
      mob->zone = pRoomIndex->area;

      char_to_room( mob, pRoomIndex );
      level = URANGE( 0, mob->level - 2, LEVEL_HERO - 1 );
      last  = TRUE;
      break;

  case 'O':
      if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'O': bad vnum %d.", pReset->arg1 );
    continue;
      }

      if ( ( pRoomIndex = get_room_index( pReset->arg3 ) ) == NULL )
      {
    bug( "Reset_area: 'R': bad vnum %d.", pReset->arg3 );
    continue;
      }

      if ( (pArea->nplayer > 0 && !pArea->freeze)
      ||   count_obj_list( pObjIndex, pRoomIndex->contents ) > 0 )
      {
    last = FALSE;
    break;
      }

      if (pReset->arg2 > 50) /* old format */
          limit = 6;
      else if (pReset->arg2 == -1) /* no limit */
          limit = 999;
      else
          limit = pReset->arg2;
      if ( pObjIndex->count >= limit )
      {
	last = FALSE;
	break;
      }

      obj       = create_object( pObjIndex, UMIN(number_fuzzy(level),
                   LEVEL_HERO - 1), TRUE );
      obj->cost = 0;
      obj_to_room( obj, pRoomIndex );
      last = TRUE;
      break;

  case 'P':
      if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'P': bad vnum %d.", pReset->arg1 );
    continue;
      }

      if ( ( pObjToIndex = get_obj_index( pReset->arg3 ) ) == NULL )
      {
    bug( "Reset_area: 'P': bad vnum %d.", pReset->arg3 );
    continue;
      }

            if (pReset->arg2 > 50) /* old format */
                limit = 6;
            else if (pReset->arg2 == -1) /* no limit */
                limit = 999;
            else
                limit = pReset->arg2;

      if ((pArea->nplayer > 0  && !pArea->freeze)
      || (obj_to = get_obj_type( pObjToIndex ) ) == NULL
      || (obj_to->in_room == NULL && !last)
      || ( pObjIndex->count >= limit && (number_range(0,2) != 0  && !pArea->freeze))
      || (count = count_obj_list(pObjIndex,obj_to->contains)) 
    > pReset->arg4 )
      {
    last = FALSE;
    break;
      }

      while (count < pReset->arg4)
      {
          obj = create_object( pObjIndex, number_fuzzy(obj_to->level), TRUE );
        obj_to_obj( obj, obj_to );
    count++;
    if (pObjIndex->count >= limit)
        break;
      }
      /* fix object lock state! */
      obj_to->value[1] = obj_to->pIndexData->value[1];
      last = TRUE;
      break;

  case 'G':
  case 'E':
      if ( ( pObjIndex = get_obj_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'E' or 'G': bad vnum %d.", pReset->arg1 );
    continue;
      }

      if ( !last )
    break;

      if ( mob == NULL )
      {
    bug( "Reset_area: 'E' or 'G': null mob for vnum %d.",
        pReset->arg1 );
    last = FALSE;
    break;
      }

      if ( mob->pIndexData->pShop != NULL )
      {
    int olevel = 0,i,j;

    if (!pObjIndex->new_format)
        switch ( pObjIndex->item_type )
    {
    case ITEM_PILL:
    case ITEM_POTION:
    case ITEM_SCROLL:
        olevel = 53;
        for (i = 1; i < 5; i++)
        {
      if (pObjIndex->value[i] > 0)
      {
              for (j = 0; j < MAX_CLASS; j++)
          {
        olevel = UMIN(olevel,
                 skill_table[pObjIndex->value[i]].
                 skill_level[j]);
          }
      }
        }
       
        olevel = UMAX(0,(olevel * 3 / 4) - 2);
        break;
    case ITEM_WAND:   olevel = number_range( 10, 20 ); break;
    case ITEM_STAFF:  olevel = number_range( 15, 25 ); break;
    case ITEM_ARMOR:  olevel = number_range(  5, 15 ); break;
    case ITEM_WEAPON: olevel = number_range(  5, 15 ); break;
    case ITEM_TREASURE: olevel = number_range( 10, 20 ); break;
    }

    obj = create_object( pObjIndex, olevel, TRUE );
    if( obj->item_type != ITEM_TRASH )
    SET_BIT( obj->extra_flags, ITEM_INVENTORY );
      }

      else
      {
    if (pReset->arg2 > 50) /* old format */
        limit = 6;
    else if (pReset->arg2 == -1) /* no limit */
        limit = 999;
    else
        limit = pReset->arg2;

    if (pObjIndex->count < limit || number_range(0,2) == 0 || pArea->freeze)
    {
        obj=create_object(pObjIndex,UMIN(number_fuzzy(level),
        LEVEL_HERO - 1), TRUE);
        /* error message if it is too high */
/*        if (obj->level > mob->level + 3
        ||  (obj->item_type == ITEM_WEAPON 
        &&   pReset->command == 'E' 
        &&   obj->level < mob->level -5 && obj->level < 45))
      fprintf(stderr,
          "Err: obj %s (%d) -- %d, mob %s (%d) -- %d\n",
          obj->short_descr,obj->pIndexData->vnum,obj->level,
          mob->short_descr,mob->pIndexData->vnum,mob->level); */
    }
    else
        break;
      }
      obj_to_char( obj, mob );
      if ( pReset->command == 'E' )
    equip_char( mob, obj, pReset->arg3 );
      last = TRUE;
      break;

  case 'D':
      if ( ( pRoomIndex = get_room_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'D': bad vnum %d.", pReset->arg1 );
    continue;
      }

      if ( ( pexit = pRoomIndex->exit[pReset->arg2] ) == NULL )
    break;

    /* may need to convert here */
    if ( IS_SET(pReset->arg3,EX_NEW_FORMAT) )
    	pexit->exit_info = pReset->arg3; /* set up exit flags */
    else
      switch ( pReset->arg3 )
      {
      case 0:
    REMOVE_BIT( pexit->exit_info, EX_CLOSED );
    REMOVE_BIT( pexit->exit_info, EX_LOCKED );
    break;

      case 1:
    SET_BIT(    pexit->exit_info, EX_CLOSED );
    REMOVE_BIT( pexit->exit_info, EX_LOCKED );
    break;

      case 2:
    SET_BIT(    pexit->exit_info, EX_CLOSED );
    SET_BIT(    pexit->exit_info, EX_LOCKED );
    break;
      }

      last = TRUE;
      break;

  case 'R':
      if ( ( pRoomIndex = get_room_index( pReset->arg1 ) ) == NULL )
      {
    bug( "Reset_area: 'R': bad vnum %d.", pReset->arg1 );
    continue;
      }

      {
    int d0;
    int d1;

    for ( d0 = 0; d0 < pReset->arg2 - 1; d0++ )
    {
        d1                   = number_range( d0, pReset->arg2-1 );
        pexit                = pRoomIndex->exit[d0];
        pRoomIndex->exit[d0] = pRoomIndex->exit[d1];
        pRoomIndex->exit[d1] = pexit;
    }
      }
      break; 
  }
    }

    return;
}

void MobIndexToInstance ( CHAR_DATA *mob, MOB_INDEX_DATA *pMobIndex )
{
  int i;
  AFFECT_DATA af;
  
  while ( mob->flash_affected )
    flash_affect_remove( mob, mob->flash_affected, APPLY_PRIMARY);
  while ( mob->affected )
    affect_remove( mob, mob->affected, APPLY_PRIMARY);


    mob->pIndexData = pMobIndex;

    mob->name   = str_dup(pMobIndex->player_name);
    mob->id   = get_mob_id();
    mob->short_descr  = str_dup(pMobIndex->short_descr);
    mob->long_descr = str_dup(pMobIndex->long_descr);
    mob->description  = str_dup(pMobIndex->description);
    mob->spec_fun = pMobIndex->spec_fun;
    mob->prompt   = NULL;

    if (pMobIndex->wealth == 0)
    {
  mob->silver = 0;
  mob->gold   = 0;
    }
    else
    {
  long wealth;

  wealth = number_range(pMobIndex->wealth/2, 3 * pMobIndex->wealth/2);
  mob->gold = number_range(wealth/200,wealth/100);
  mob->silver = wealth - (mob->gold * 100);
    } 

    if (pMobIndex->new_format)
    /* load in new style */
    {
  /* read from prototype */
  mob->group    = pMobIndex->group;
  mob->act    = pMobIndex->act;
  mob->comm   = COMM_NOCHANNELS|COMM_NOSHOUT|COMM_NOTELL;
  mob->affected_by  = pMobIndex->affected_by;
  mob->alignment    = pMobIndex->alignment;
  mob->level    = pMobIndex->level;
  mob->hitroll    = pMobIndex->hitroll;
  mob->damroll    = pMobIndex->damage[DICE_BONUS];
  mob->max_hit    = dice(pMobIndex->hit[DICE_NUMBER],
               pMobIndex->hit[DICE_TYPE])
          + pMobIndex->hit[DICE_BONUS];
  mob->hit    = mob->max_hit;
  mob->max_mana   = dice(pMobIndex->mana[DICE_NUMBER],
               pMobIndex->mana[DICE_TYPE])
          + pMobIndex->mana[DICE_BONUS];
  mob->mana   = mob->max_mana;
  mob->damage[DICE_NUMBER]= pMobIndex->damage[DICE_NUMBER];
  mob->damage[DICE_TYPE]  = pMobIndex->damage[DICE_TYPE];
  mob->dam_type   = pMobIndex->dam_type;
        if (mob->dam_type == 0)
          switch(number_range(1,3))
            {
                case (1): mob->dam_type = 3;        break;  /* slash */
                case (2): mob->dam_type = 7;        break;  /* pound */
                case (3): mob->dam_type = 11;       break;  /* pierce */
            }
  for (i = 0; i < 4; i++)
      mob->armor[i] = pMobIndex->ac[i]; 
  mob->off_flags    = pMobIndex->off_flags;
  mob->imm_flags    = pMobIndex->imm_flags;
  mob->res_flags    = pMobIndex->res_flags;
  mob->vuln_flags   = pMobIndex->vuln_flags;
  mob->start_pos    = pMobIndex->start_pos;
  mob->default_pos  = pMobIndex->default_pos;
  mob->sex    = pMobIndex->sex;
        if (mob->sex == 3) /* random sex */
            mob->sex = number_range(1,2);
  mob->race   = pMobIndex->race;
  mob->form   = pMobIndex->form;
  mob->parts    = pMobIndex->parts;
  mob->size   = pMobIndex->size;
  mob->material   = str_dup(pMobIndex->material);

  /* computed on the spot */

      for (i = 0; i < MAX_STATS; i ++)
            mob->perm_stat[i] = UMIN(25,11 + mob->level/4);
            
        if (IS_SET(mob->act,ACT_WARRIOR))
        {
            mob->perm_stat[STAT_STR] += 3;
            mob->perm_stat[STAT_INT] -= 1;
            mob->perm_stat[STAT_CON] += 2;
        }
        
        if (IS_SET(mob->act,ACT_THIEF))
        {
            mob->perm_stat[STAT_DEX] += 3;
            mob->perm_stat[STAT_INT] += 1;
            mob->perm_stat[STAT_WIS] -= 1;
        }
        
        if (IS_SET(mob->act,ACT_CLERIC))
        {
            mob->perm_stat[STAT_WIS] += 3;
            mob->perm_stat[STAT_DEX] -= 1;
            mob->perm_stat[STAT_STR] += 1;
        }
        
        if (IS_SET(mob->act,ACT_MAGE))
        {
            mob->perm_stat[STAT_INT] += 3;
            mob->perm_stat[STAT_STR] -= 1;
            mob->perm_stat[STAT_DEX] += 1;
        }
        
        if (IS_SET(mob->off_flags,OFF_FAST))
            mob->perm_stat[STAT_DEX] += 2;
            
        mob->perm_stat[STAT_STR] += mob->size - SIZE_MEDIUM;
        mob->perm_stat[STAT_CON] += (mob->size - SIZE_MEDIUM) / 2;

  /* let's get some spell action */
  if (IS_AFFECTED(mob,AFF_SANCTUARY))
  {
      af.where   = TO_AFFECTS;
      af.type      = skill_lookup("sanctuary");
      af.level     = mob->level;
      af.duration  = -1;
      af.location  = APPLY_NONE;
      af.modifier  = 0;
      af.bitvector = AFF_SANCTUARY;
      affect_to_char( mob, &af );
  }

  if (IS_AFFECTED(mob,AFF_HASTE))
  {
      af.where   = TO_AFFECTS;
      af.type      = skill_lookup("haste");
          af.level     = mob->level;
            af.duration  = -1;
          af.location  = APPLY_DEX;
          af.modifier  = 1 + (mob->level >= 18) + (mob->level >= 25) + 
         (mob->level >= 32);
          af.bitvector = AFF_HASTE;
          affect_to_char( mob, &af );
  }

  if (IS_AFFECTED(mob,AFF_PROTECT_EVIL))
  {
      af.where   = TO_AFFECTS;
      af.type  = skill_lookup("protection evil");
      af.level   = mob->level;
      af.duration  = -1;
      af.location  = APPLY_SAVES;
      af.modifier  = -1;
      af.bitvector = AFF_PROTECT_EVIL;
      affect_to_char(mob,&af);
  }

        if (IS_AFFECTED(mob,AFF_PROTECT_GOOD))
        {
      af.where   = TO_AFFECTS;
            af.type      = skill_lookup("protection good");
            af.level     = mob->level;
            af.duration  = -1;
            af.location  = APPLY_SAVES;
            af.modifier  = -1;
            af.bitvector = AFF_PROTECT_GOOD;
            affect_to_char(mob,&af);
        }
    }
    else /* read in old format and convert */
    {
  mob->act    = pMobIndex->act;
  mob->affected_by  = pMobIndex->affected_by;
  mob->alignment    = pMobIndex->alignment;
  mob->level    = pMobIndex->level;
  mob->hitroll    = pMobIndex->hitroll;
  mob->damroll    = 0;
  mob->max_hit    = mob->level * 8 + number_range(
          mob->level * mob->level/4,
          mob->level * mob->level);
  mob->max_hit *= .9;
  mob->hit    = mob->max_hit;
  mob->max_mana   = 100 + dice(mob->level,10);
  mob->mana   = mob->max_mana;
  switch(number_range(1,3))
  {
      case (1): mob->dam_type = 3;  break;  /* slash */
      case (2): mob->dam_type = 7;  break;  /* pound */
      case (3): mob->dam_type = 11; break;  /* pierce */
  }
  for (i = 0; i < 3; i++)
      mob->armor[i] = interpolate(mob->level,100,-100);
  mob->armor[3]   = interpolate(mob->level,100,0);
  mob->race   = pMobIndex->race;
  mob->off_flags    = pMobIndex->off_flags;
  mob->imm_flags    = pMobIndex->imm_flags;
  mob->res_flags    = pMobIndex->res_flags;
  mob->vuln_flags   = pMobIndex->vuln_flags;
  mob->start_pos    = pMobIndex->start_pos;
  mob->default_pos  = pMobIndex->default_pos;
  mob->sex    = pMobIndex->sex;
  mob->form   = pMobIndex->form;
  mob->parts    = pMobIndex->parts;
  mob->size   = SIZE_MEDIUM;
  mob->material   = str_dup ("");

  for (i = 0; i < MAX_STATS; i ++)
    mob->perm_stat[i] = 11 + mob->level/4;
  }
  
  mob->position = mob->start_pos;

}

/*
 * Create an instance of a mobile.
 */
CHAR_DATA *create_mobile( MOB_INDEX_DATA *pMobIndex )
{
    CHAR_DATA *mob;

    mobile_count++;

    if ( pMobIndex == NULL )
    {
  bug( "Create_mobile: NULL pMobIndex.", 0 );
  exit( 1 );
    }

    mob = new_char();
    MobIndexToInstance (mob,pMobIndex);

    /* link the mob to the world list */
    mob->next   = char_list;
    char_list   = mob;
    pMobIndex->count++;
    return mob;
}

/* duplicate a mobile exactly -- except inventory */
void clone_mobile(CHAR_DATA *parent, CHAR_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;

    if ( parent == NULL || clone == NULL || !IS_NPC(parent))
  return;
    
    /* start fixing values */ 
    clone->name   = str_dup(parent->name);
    clone->version  = parent->version;
    clone->short_descr  = str_dup(parent->short_descr);
    clone->long_descr = str_dup(parent->long_descr);
    clone->description  = str_dup(parent->description);
    clone->group  = parent->group;
    clone->sex    = parent->sex;
    clone->class  = parent->class;
    clone->race   = parent->race;
    clone->level  = parent->level;
    clone->trust  = 0;
    clone->timer  = parent->timer;
    clone->wait   = parent->wait;
    clone->hit    = parent->hit;
    clone->max_hit  = parent->max_hit;
    clone->mana   = parent->mana;
    clone->max_mana = parent->max_mana;
    clone->move   = parent->move;
    clone->max_move = parent->max_move;
    clone->gold   = parent->gold;
    clone->silver = parent->silver;
    clone->exp    = parent->exp;
    clone->act    = parent->act;
    clone->comm   = parent->comm;
    clone->imm_flags  = parent->imm_flags;
    clone->res_flags  = parent->res_flags;
    clone->vuln_flags = parent->vuln_flags;
    clone->invis_level  = parent->invis_level;
    clone->affected_by  = parent->affected_by;
    clone->position = parent->position;
    clone->practice = parent->practice;
    clone->train  = parent->train;
    clone->saving_throw = parent->saving_throw;
    clone->alignment  = parent->alignment;
    clone->hitroll  = parent->hitroll;
    clone->damroll  = parent->damroll;
    clone->wimpy  = parent->wimpy;
    clone->form   = parent->form;
    clone->parts  = parent->parts;
    clone->size   = parent->size;
    clone->material = str_dup(parent->material);
    clone->off_flags  = parent->off_flags;
    clone->dam_type = parent->dam_type;
    clone->start_pos  = parent->start_pos;
    clone->default_pos  = parent->default_pos;
    clone->spec_fun = parent->spec_fun;
    
    for (i = 0; i < 4; i++)
      clone->armor[i] = parent->armor[i];

    for (i = 0; i < MAX_STATS; i++)
    {
  clone->perm_stat[i] = parent->perm_stat[i];
  clone->mod_stat[i]  = parent->mod_stat[i];
    }

    for (i = 0; i < 3; i++)
  clone->damage[i]  = parent->damage[i];

    /* now add the affects */
    for (paf = parent->affected; paf != NULL; paf = paf->next)
        affect_to_char(clone,paf);

}


void ObjIndexToInstance ( OBJ_DATA *obj, OBJ_INDEX_DATA *pObjIndex, int level, bool favored )
{
  int i;
  AFFECT_DATA *paf;

  while ( obj->affected )
    affect_remove_obj ( obj, obj->affected );
  
    obj->enchanted  = FALSE;
    obj->warps 	    = 0;
    obj->damaged = 0;
    if (pObjIndex->new_format)
  obj->level = pObjIndex->level;
    else
  obj->level    = UMAX(0,level);
    obj->wear_loc = -1;

    obj->name   = str_dup(pObjIndex->name);
    obj->short_descr  = str_dup(pObjIndex->short_descr);
    obj->description  = str_dup(pObjIndex->description);
    obj->material = str_dup(pObjIndex->material);
    obj->item_type  = pObjIndex->item_type;
    obj->extra_flags  = pObjIndex->extra_flags;
    obj->wear_flags = pObjIndex->wear_flags;
    obj->value[0] = pObjIndex->value[0];
    obj->value[1] = pObjIndex->value[1];
    obj->value[2] = pObjIndex->value[2];
    obj->value[3] = pObjIndex->value[3];
    obj->value[4] = pObjIndex->value[4];
    obj->weight   = pObjIndex->weight;

    if (level == -1 || pObjIndex->new_format)
  obj->cost = pObjIndex->cost;
    else
      obj->cost = number_fuzzy( 10 )
      * number_fuzzy( level ) * number_fuzzy( level );

    /*
     * Mess with object properties.
     */
    switch ( obj->item_type )
    {
    default:
  bug( "Read_object: vnum %d bad type.", pObjIndex->vnum );
  break;

    case ITEM_LIGHT:
  if (obj->value[2] == 999)
    obj->value[2] = -1;
  break;

    case ITEM_FURNITURE:
    case ITEM_TRASH:
    case ITEM_CONTAINER:
    case ITEM_DRINK_CON:
    case ITEM_KEY:
    case ITEM_FOOD:
    case ITEM_BOAT:
    case ITEM_CORPSE_NPC:
    case ITEM_CORPSE_PC:
    case ITEM_FOUNTAIN:
    case ITEM_MAP:
    case ITEM_CLOTHING:
    case ITEM_TRAP:
    case ITEM_PORTAL:
  if (!pObjIndex->new_format)
      obj->cost /= 5;
  break;

    case ITEM_TREASURE:
    case ITEM_WARP_STONE:
    case ITEM_ROOM_KEY:
    case ITEM_GEM:
    case ITEM_JEWELRY:
    case ITEM_GRENADE:
    case ITEM_MIXER:
    case ITEM_COMPONENT:
    case ITEM_SPELL_PAGE:
    case ITEM_PART:
    case ITEM_FORGE:
  break;

    case ITEM_JUKEBOX:
  for (i = 0; i < 5; i++)
     obj->value[i] = -1;
  break;

    case ITEM_SCROLL:
  if (level != -1 && !pObjIndex->new_format)
      obj->value[0] = number_fuzzy( obj->value[0] );
  break;

    case ITEM_WAND:
    case ITEM_STAFF:
  if (level != -1 && !pObjIndex->new_format)
  {
      obj->value[0] = number_fuzzy( obj->value[0] );
      obj->value[1] = number_fuzzy( obj->value[1] );
      obj->value[2] = obj->value[1];
  }
  if (!pObjIndex->new_format)
      obj->cost *= 2;
  break;

    case ITEM_WEAPON:
    weapons_popped++;
  if (level != -1 && !pObjIndex->new_format)
  {
      obj->value[1] = number_fuzzy( number_fuzzy( 1 * level / 4 + 2 ) );
      obj->value[2] = number_fuzzy( number_fuzzy( 3 * level / 4 + 6 ) );
  }
  /*change from 6 to 4 - poquah */
  if ( favored && (number_percent() * number_percent()) < 3 )
    {
      switch ( dice(1,10) )
      {
      case 1:
      case 2: 
      case 3:
	  if ( obj->value[4] == 0  && ( obj->value[1] > 1 && obj->value[2] > 1))
		SET_BIT( obj->value[4], WEAPON_VORPAL );
		sprintf(log_buf,"Vorpal: %s vnum %d after %d",
			obj->name,obj->pIndexData->vnum,weapons_popped);
		break;
      case 4:
      case 5:
      case 6:
      case 7:
      case 8:
		SET_BIT( obj->value[4], WEAPON_HOLY );
		sprintf(log_buf,"Holy: %s vnum %d after %d",
			obj->name,obj->pIndexData->vnum,weapons_popped);
		break;
      default:
      	SET_BIT( obj->value[4], WEAPON_FAVORED );
        sprintf(log_buf,"Favored: %s vnum %d after %d",
        obj->name,obj->pIndexData->vnum,weapons_popped);


      }
      log_string(log_buf);
      weapons_popped = 0;
    }

  break;

    case ITEM_ARMOR:
  if (level != -1 && !pObjIndex->new_format)
  {
      obj->value[0] = number_fuzzy( level / 5 + 3 );
      obj->value[1] = number_fuzzy( level / 5 + 3 );
      obj->value[2] = number_fuzzy( level / 5 + 3 );
  }
  break;

    case ITEM_POTION:
    case ITEM_PILL:
  if (level != -1 && !pObjIndex->new_format)
      obj->value[0] = number_fuzzy( number_fuzzy( obj->value[0] ) );
  break;

    case ITEM_MONEY:
  if (!pObjIndex->new_format)
      obj->value[0] = obj->cost;
  break;
    }
  
    for (paf = pObjIndex->affected; paf != NULL; paf = paf->next) 
  if ( paf->location == APPLY_SPELL_AFFECT )
      affect_to_obj(obj,paf);
}

/*
 * Create an instance of an object.
 */
OBJ_DATA *create_object( OBJ_INDEX_DATA *pObjIndex, int level, bool favored )
{
    OBJ_DATA *obj;

    if ( pObjIndex == NULL )
    {
  bug( "Create_object: NULL pObjIndex.", 0 );
  exit( 1 );
    }

    obj = new_obj();
    obj->pIndexData = pObjIndex;
    obj->in_room  = NULL;    
    obj->stolen_timer  = 0;    
    ObjIndexToInstance (obj,pObjIndex,level,favored);
 
    obj->original = TRUE;
 
    obj->next   = object_list;
    object_list   = obj;
    pObjIndex->count++;

    set_rarity(obj);

    return obj;
}

/* duplicate an object exactly -- except contents */
void clone_object(OBJ_DATA *parent, OBJ_DATA *clone)
{
    int i;
    AFFECT_DATA *paf;
/*    EXTRA_DESCR_DATA *ed,*ed_new; */

    if (parent == NULL || clone == NULL)
  return;

    /* start fixing the object */
    clone->name   = str_dup(parent->name);
    clone->short_descr  = str_dup(parent->short_descr);
    clone->description  = str_dup(parent->description);
    clone->item_type  = parent->item_type;
    clone->extra_flags  = parent->extra_flags;
    clone->wear_flags = parent->wear_flags;
    clone->weight = parent->weight;
    clone->cost   = parent->cost;
    clone->level  = parent->level;
    clone->stolen_timer  = parent->stolen_timer;
    clone->condition  = parent->condition;
    clone->material = str_dup(parent->material);
    clone->timer  = parent->timer;

    for (i = 0;  i < 5; i ++)
  clone->value[i] = parent->value[i];

    /* affects */
    clone->enchanted  = parent->enchanted;
  
    for (paf = parent->affected; paf != NULL; paf = paf->next) 
  affect_to_obj(clone,paf);

    /* extended desc */
/*
    for (ed = parent->extra_descr; ed != NULL; ed = ed->next);
    {
        ed_new                  = new_extra_descr();
        ed_new->keyword     = str_dup( ed->keyword);
        ed_new->description     = str_dup( ed->description );
        ed_new->next            = clone->extra_descr;
        clone->extra_descr    = ed_new;
    }
*/

}



/*
 * Clear a new character.
 */
void clear_char( CHAR_DATA *ch )
{
    static CHAR_DATA ch_zero;
    int i;

    *ch       = ch_zero;
    ch->name      = &str_empty[0];
    ch->short_descr   = &str_empty[0];
    ch->long_descr    = &str_empty[0];
    ch->description   = &str_empty[0];
    ch->prompt                  = &str_empty[0];
    ch->logon     = current_time;
    ch->lines     = PAGELEN;
    for (i = 0; i < 4; i++)
      ch->armor[i]    = 100;
    ch->position    = POS_STANDING;
    ch->hit     = 20;
    ch->max_hit     = 20;
    ch->mana      = 100;
    ch->max_mana    = 100;
    ch->move      = 100;
    ch->max_move    = 100;
    ch->on      = NULL;
    for (i = 0; i < MAX_STATS; i ++)
    {
  ch->perm_stat[i] = 13; 
  ch->mod_stat[i] = 0;
    }
    return;
}

/*
 * Get an extra description from a list.
 */
char *get_extra_descr( const char *name, EXTRA_DESCR_DATA *ed )
{
    for ( ; ed != NULL; ed = ed->next )
    {
  if ( is_name( (char *) name, ed->keyword ) )
      return ed->description;
    }
    return NULL;
}



/*
 * Translates mob virtual number to its mob index struct.
 * Hash table lookup.
 */
MOB_INDEX_DATA *get_mob_index( int vnum )
{
    MOB_INDEX_DATA *pMobIndex;

    if (vnum <= 0) return NULL;

    for ( pMobIndex  = mob_index_hash[vnum % MAX_KEY_HASH];
    pMobIndex != NULL;
    pMobIndex  = pMobIndex->next )
    {
  if ( pMobIndex->vnum == vnum )
      return pMobIndex;
    }

    if ( fBootDb )
    {
  bug( "Get_mob_index: bad vnum %d while booting.", vnum );  
    }

    return NULL;
}

RECIPE_DATA *get_recipe_data ( int recipe_number )
{
	RECIPE_DATA *recipe;

	for ( recipe = recipe_first ; recipe != NULL ; recipe = recipe->next)
	{
		if (recipe->recipe_num == recipe_number)
		   return recipe;
	}
	return NULL;
}

/*
 * Translates mob virtual number to its obj index struct.
 * Hash table lookup.
 */
OBJ_INDEX_DATA *get_obj_index( int vnum )
{
    OBJ_INDEX_DATA *pObjIndex;

    if (vnum <= 0) return NULL;

    for ( pObjIndex  = obj_index_hash[vnum % MAX_KEY_HASH];
    pObjIndex != NULL;
    pObjIndex  = pObjIndex->next )
    {
  if ( pObjIndex->vnum == vnum )
      return pObjIndex;
    }

    if ( fBootDb )
    {
  bug( "Get_obj_index: bad vnum %d while booting.", vnum );  
    }

    return NULL;
}



/*
 * Translates mob virtual number to its room index struct.
 * Hash table lookup.
 */
ROOM_INDEX_DATA *get_room_index( int vnum )
{
    ROOM_INDEX_DATA *pRoomIndex;

    if (vnum <= 0)
    {
      CLAN_DATA *clan = clan_first;
      PLAN_DATA *obj;
      vnum = abs(vnum);
      for(; clan != NULL; clan = clan->next)
      {
        if(vnum >= clan->vnum_min && vnum <= clan->vnum_max)
          break;
      }
      if(clan)
      {
        for(obj = clan->planned; obj != NULL; obj = obj->next)
        {
          if(IS_SET(obj->type, PLAN_ROOM) && obj->plan_index == vnum)
          {
            if(IS_SET(obj->type, (PLAN_PLACED | PLAN_PREVIEWED)) && obj->to_place)
              return (ROOM_INDEX_DATA*)obj->to_place;
            return NULL;
          }
        }
      }
      return NULL;
    }

    for ( pRoomIndex  = room_index_hash[vnum % MAX_KEY_HASH];
    pRoomIndex != NULL;
    pRoomIndex  = pRoomIndex->next )
    {
  if ( pRoomIndex->vnum == vnum )
      return pRoomIndex;
    }

    if ( fBootDb )
    {
  bug( "Get_room_index: bad vnum %d while booting.", vnum );  
    }

    return NULL;
}



/*
 * Read a letter from a file.
 */
char fread_letter( FILE *fp )
{
    char c;

    do
    {
  c = getc( fp );
    }
    while ( isspace(c) );

    return c;
}



/*
 * Read a number from a file.
 */
int fread_number( FILE *fp )
{
    int number;
    bool sign;
    char c;

    do
    {
  c = getc( fp );
    }
    while ( isspace(c) );

    number = 0;

    sign   = FALSE;
    if ( c == '+' )
    {
  c = getc( fp );
    }
    else if ( c == '-' )
    {
  sign = TRUE;
  c = getc( fp );
    }

    if ( !isdigit(c) )
    {
  bug( "Fread_number: bad format.", 0 );
  if ( fp != stdin ) fclose( fp );
  fp = NULL; 
  if (area_last)
    rename_area (area_last->file_name);          
  exit( 1 );
    }

    while ( isdigit(c) )
    {
  number = number * 10 + c - '0';
  c      = getc( fp );
    }

    if ( sign )
  number = 0 - number;

    if ( c == '|' )
  number += fread_number( fp );
    else if ( c != ' ' )
  ungetc( c, fp );

    return number;
}

long fread_flag( FILE *fp)
{
    int number;
    char c;
    bool negative = FALSE;

    do
    {
  c = getc(fp);
    }
    while ( isspace(c));

    if (c == '-')
    {
  negative = TRUE;
  c = getc(fp);
    }

    number = 0;

    if (!isdigit(c))
    {
  while (('A' <= c && c <= 'Z') || ('a' <= c && c <= 'z'))
  {
      number += flag_convert(c);
      c = getc(fp);
  }
    }

    while (isdigit(c))
    {
  number = number * 10 + c - '0';
  c = getc(fp);
    }

    if (c == '|')
  number += fread_flag(fp);

    else if  ( c != ' ')
  ungetc(c,fp);

    if (negative)
  return -1 * number;

    return number;
}

long flag_convert(char letter )
{
    long bitsum = 0;
    char i;

    if ('A' <= letter && letter <= 'Z') 
    {
  bitsum = 1;
  for (i = letter; i > 'A'; i--)
      bitsum *= 2;
    }
    else if ('a' <= letter && letter <= 'z')
    {
  bitsum = 67108864; /* 2^26 */
  for (i = letter; i > 'a'; i --)
      bitsum *= 2;
    }

    return bitsum;
}

/*
 * Read one word (into static buffer).
 */
char *fread_word( FILE *fp )
{
    static char word[MAX_INPUT_LENGTH];
    char *pword;
    char cEnd;

    do
    {
  cEnd = getc( fp );
    }
    while ( isspace( cEnd ) );

    if ( cEnd == '\'' || cEnd == '"' )
    {
  pword   = word;
    }
    else
    {
  word[0] = cEnd;
  pword   = word+1;
  cEnd    = ' ';
    }

    for ( ; pword < word + MAX_INPUT_LENGTH; pword++ )
    {
  *pword = getc( fp );
  if ( cEnd == ' ' ? isspace(*pword) : *pword == cEnd )
  {
      if ( cEnd == ' ' )
    ungetc( *pword, fp );
      *pword = '\0';
      return word;
  }
    }

    bug( "Fread_word: word too long.", 0 );
    if ( fp != stdin ) fclose( fp );
    fp = NULL; 
    if (area_last)
      rename_area (area_last->file_name);            
    exit( 1 );
    return NULL;
}


char *fread_string( FILE *fp )
{
    char *plast;
    char c;

    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
  bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
  exit( 1 );
    }

    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
  c = getc( fp );
    }
    while ( isspace(c) );

    if ( ( *plast++ = c ) == '~' )
  return &str_empty[0];

    for ( ;; )
    {
        /*
         * Back off the char type lookup,
         *   it was too dirty for portability.
         *   -- Furey
         */

  switch ( *plast = getc(fp) )
  {
        default:
            plast++;
            break;
 
        case EOF:
  /* temp fix */
            bug( "Fread_string: EOF", 0 );
      return NULL;
            /* exit( 1 ); */
            break;
 
        case '\n':
            plast++;
            *plast++ = '\r';
            break;
 
        case '\r':
            break;
 
        case '~':
            plast++;
      {
    union
    {
        char *  pc;
        char  rgc[sizeof(char *)];
    } u1;
    int ic;
    int iHash;
    char *pHash;
    char *pHashPrev;
    char *pString;

    plast[-1] = '\0';
    iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
    for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
    {
        for ( ic = 0; ic < sizeof(char *); ic++ )
      u1.rgc[ic] = pHash[ic];
        pHashPrev = u1.pc;
        pHash    += sizeof(char *);

        if ( top_string[sizeof(char *)] == pHash[0]
        &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
      return pHash;
    }

    if ( fBootDb )
    {
        pString   = top_string;
        top_string    = plast;
        u1.pc   = string_hash[iHash];
        for ( ic = 0; ic < sizeof(char *); ic++ )
      pString[ic] = u1.rgc[ic];
        string_hash[iHash]  = pString;

        nAllocString += 1;
        sAllocString += top_string - pString;
        return pString + sizeof(char *);
    }
    else
    {
        return str_dup( top_string + sizeof(char *) );
    }
      }
  }
    }
}

char *fread_string_eol( FILE *fp )
{
    static bool char_special[256-EOF];
    char *plast;
    char c;
 
    if ( char_special[EOF-EOF] != TRUE )
    {
        char_special[EOF -  EOF] = TRUE;
        char_special['\n' - EOF] = TRUE;
        char_special['\r' - EOF] = TRUE;
    }
 
    plast = top_string + sizeof(char *);
    if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
    {
        bug( "Fread_string: MAX_STRING %d exceeded.", MAX_STRING );
        exit( 1 );
    }
 
    /*
     * Skip blanks.
     * Read first char.
     */
    do
    {
        c = getc( fp );
    }
    while ( isspace(c) );
 
    if ( ( *plast++ = c ) == '\n')
        return &str_empty[0];
 
    for ( ;; )
    {
        if ( !char_special[ ( *plast++ = getc( fp ) ) - EOF ] )
            continue;
 
        switch ( plast[-1] )
        {
        default:
            break;
 
        case EOF:
            bug( "Fread_string_eol  EOF", 0 );
            if ( fp != stdin ) fclose( fp );
            fp = NULL;
            if (area_last)
              rename_area (area_last->file_name);            
            exit( 1 );
            break;
 
        case '\n':  case '\r':
            {
                union
                {
                    char *      pc;
                    char        rgc[sizeof(char *)];
                } u1;
                int ic;
                int iHash;
                char *pHash;
                char *pHashPrev;
                char *pString;
 
                plast[-1] = '\0';
                iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
                for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
                {
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        u1.rgc[ic] = pHash[ic];
                    pHashPrev = u1.pc;
                    pHash    += sizeof(char *);
 
                    if ( top_string[sizeof(char *)] == pHash[0]
                    &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
                        return pHash;
                }
 
                if ( fBootDb )
                {
                    pString             = top_string;
                    top_string          = plast;
                    u1.pc               = string_hash[iHash];
                    for ( ic = 0; ic < sizeof(char *); ic++ )
                        pString[ic] = u1.rgc[ic];
                    string_hash[iHash]  = pString;
 
                    nAllocString += 1;
                    sAllocString += top_string - pString;
                    return pString + sizeof(char *);
                }
                else
                {
                    return str_dup( top_string + sizeof(char *) );
                }
            }
        }
    }
}



/*
 * Read to end of line (for comments).
 */
void fread_to_eol( FILE *fp )
{
    char c;

    do
    {
  c = getc( fp );
    }
    while ( c != '\n' && c != '\r' );

    do
    {
  c = getc( fp );
    }
    while ( c == '\n' || c == '\r' );

    ungetc( c, fp );
    return;
}

/*
void free_mem( void *pMem, int sMem )
{
    int iList;

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
  if ( sMem <= rgSizeList[iList] )
      break;
    }

    if ( iList == MAX_MEM_LIST )
    {
  bug( "Free_mem: size %d too large.", sMem );
  exit( 1 );
    }

    * ((void **) pMem) = rgFreeList[iList];
    rgFreeList[iList]  = pMem;

    return;
} */

void free_mem( void *pMem, int sMem )
{
#ifdef OOLC_VERSION
    int iList;
    int *magic;

    pMem -= sizeof(*magic);
    magic = (int *) pMem;

    if (*magic != MAGIC_NUM)
    {
        bug("Attempt to recyle invalid memory of size %d.",sMem);
        bug((char*) pMem + sizeof(*magic),0);
        return;
    }

    *magic = 0;
    sMem += sizeof(*magic);

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }
    
    if ( iList == MAX_MEM_LIST )
    {
        bug( "Free_mem: size %d too large.", sMem );
        exit( 1 );
    }
    
    * ((void **) pMem) = rgFreeList[iList];
    rgFreeList[iList]  = pMem;
#else
	GC_FREE(pMem);     
#endif
    return;
}    


/*
 * Allocate some ordinary memory,
 *   with the expectation of freeing it someday.
 *
void *alloc_mem( int sMem )
{
    void *pMem;
    int *magic;
    int iList;

    sMem += sizeof(*magic);

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
  if ( sMem <= rgSizeList[iList] )
      break;
    }

    if ( iList == MAX_MEM_LIST )
    {
  bug( "Alloc_mem: size %d too large.", sMem );
  exit( 1 );
    }

    if ( rgFreeList[iList] == NULL )
    {
  pMem              = alloc_perm( rgSizeList[iList] );
    }
    else
    {
  pMem              = rgFreeList[iList];
  rgFreeList[iList] = * ((void **) rgFreeList[iList]);
    }

    return pMem;
}
*/

/*
 * Old memory code, use GC_MALLOC instead, more stable
 *
 */
#ifdef OOLC_VERSION
void *alloc_mem( int sMem )
{
    void *pMem;
    int *magic;
    int iList;

    sMem += sizeof(*magic);

    for ( iList = 0; iList < MAX_MEM_LIST; iList++ )
    {
        if ( sMem <= rgSizeList[iList] )
            break;
    }

    if ( iList == MAX_MEM_LIST )
    {
        bug( "Alloc_mem: size %d too large.", sMem );
        exit( 1 );
    }

    if ( rgFreeList[iList] == NULL )
    { 
        pMem              = alloc_perm( rgSizeList[iList] );
    } 
    else
    { 
        pMem              = rgFreeList[iList];
        rgFreeList[iList] = * ((void **) rgFreeList[iList]);
    } 
    
    magic = (int *) pMem;
    *magic = MAGIC_NUM;
    pMem += sizeof(*magic);
      
    return pMem;
}     
#endif

/*
 * Old memory code, use GC_MALLOC instead (see the gc/ directory 
 * for more information.
 *
 */
#ifdef OLC_VERSION
void *alloc_perm( int sMem )
{
    static char *pMemPerm;
    static int iMemPerm;
    void *pMem;

    while ( sMem % sizeof(long) != 0 )
  sMem++;
    if ( sMem > MAX_PERM_BLOCK )
    {
  bug( "Alloc_perm: %d too large.", sMem );
  exit( 1 );
    }

    if ( pMemPerm == NULL || iMemPerm + sMem > MAX_PERM_BLOCK )
    {
  iMemPerm = 0;
  if ( ( pMemPerm = GC_MALLOC( MAX_PERM_BLOCK ) ) == NULL )
  {
      perror( "Alloc_perm" );
      exit( 1 );
  }
    }

    pMem        = pMemPerm + iMemPerm;
    iMemPerm   += sMem;
    nAllocPerm += 1;
    sAllocPerm += sMem;
    return pMem;
}

#endif

/*
 * Duplicate a string into dynamic memory.
 * Fread_strings are read-only and shared.
 */
char *str_dup( const char *str )
{
    char *str_new;
    
    if (str == NULL)
  return &str_empty[0];

    if ( str[0] == '\0' )
  return &str_empty[0];

    if ( str >= string_space && str < top_string )
  return (char *) str;

#ifdef OLC_VERSION
    str_new = alloc_mem( strlen(str) + 1 );
#else
    str_new = GC_MALLOC( strlen(str) );
#endif
    strncpy( str_new, str, strlen(str) );
    return str_new;
}

/*
 * Old memory code, use str_dup()  (I hope)
 *
 */
#ifdef OLC_VERSION
char *str_dup_perm( const char *str )
{       
  int len;
  char *plast;
  int ic;
  int iHash;
  char *pHash;
  char *pHashPrev;
  char *pString;  
  
  if (str == NULL)
    return NULL;

  if ( str[0] == '\0' )
    return &str_empty[0];

  if ( str >= string_space && str < top_string )
    return (char *) str;  
  
  len = strlen (str);
  
  plast = top_string + sizeof(char *);
  if ( plast > &string_space[MAX_STRING - MAX_STRING_LENGTH] )
  {
    bug( "Str_dup_perm: MAX_STRING %d exceeded.", MAX_STRING );
    exit( 1 );
  }
 
  strcpy (plast,str);
  plast += strlen (str) + 2;
  {
      union
      {
          char *      pc;
          char        rgc[sizeof(char *)];
      } u1;
 
      plast[-1] = '\0';
      iHash     = UMIN( MAX_KEY_HASH - 1, plast - 1 - top_string );
      for ( pHash = string_hash[iHash]; pHash; pHash = pHashPrev )
      {
          for ( ic = 0; ic < sizeof(char *); ic++ )
              u1.rgc[ic] = pHash[ic];
          pHashPrev = u1.pc;
          pHash    += sizeof(char *);
 
          if ( top_string[sizeof(char *)] == pHash[0]
          &&   !strcmp( top_string+sizeof(char *)+1, pHash+1 ) )
              return pHash;
      }
 
      pString             = top_string;
      top_string          = plast;
      u1.pc               = string_hash[iHash];
      for ( ic = 0; ic < sizeof(char *); ic++ )
          pString[ic] = u1.rgc[ic];
      string_hash[iHash]  = pString;
 
      nAllocString += 1;
      sAllocString += top_string - pString;
      return pString + sizeof(char *);
  }  
}
#endif

/*
 * Free a string.
 * Null is legal here to simplify callers.
 * Read-only shared strings are not touched.
 */

void free_string( char *pstr )
{
    if ( pstr == NULL
    ||   pstr == &str_empty[0]
    || ( pstr >= string_space && pstr < top_string ) )
  return;

    free_mem( pstr, strlen(pstr) + 1 );
    return;
}

int get_area_min_vnum (AREA_DATA *area)
{
  int min;
  
  min = area->min_vnum_room;
  if (area->min_vnum_obj)
    if (area->min_vnum_obj < min)
       min = area->min_vnum_obj;
  if (area->min_vnum_mob)       
    if (area->min_vnum_mob < min)
       min = area->min_vnum_mob;
     
  return min;     
}

int get_area_max_vnum (AREA_DATA *area)
{
  int max;
  
  max = area->max_vnum_room;
  if (area->max_vnum_obj > max)
     max = area->max_vnum_obj;
  if (area->max_vnum_mob > max)
     max = area->max_vnum_mob;
     
  return max;     
}


void do_areas( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];
    char bigbuf[4*MAX_STRING_LENGTH];
    AREA_DATA *area;
    bool odd;


    if (argument[0] != '\0')
    {
  send_to_char("No argument is used with this command.\n\r",ch);
  return;
    }
  
  if (IS_IMMORTAL (ch) && IS_SET (ch->display,DISP_DISP_VNUM)) 
   {
    area = area_first;
    bigbuf[0] = '\0';
    while (area) 
    {
      int min,max;
      
      min = get_area_min_vnum (area);
      max = get_area_max_vnum (area);
      sprintf(buf,"%-39s [%5d] - [%5d]\n\r",area->name,min,max);
      strcat (bigbuf,buf);
      area = area->next;
    }
    page_to_char (bigbuf,ch);  
   }
  else
   {
    bigbuf[0] = '\0';
    area = area_first;
    odd = FALSE;
    if (IS_IMMORTAL (ch))
    send_to_char ("* denotes areas under development - IMM only.\n\r",ch);

    while (area)
     {
      if (!area->under_develop || IS_IMMORTAL(ch))
       {
        sprintf (buf,"{%s%-38s",area->under_develop ? "*":"",area->credits);
        strcat(bigbuf, buf);
        if (odd) strcat(bigbuf,"\n\r");
        odd = !odd;
       }
      area = area->next;    
     }
    page_to_char (bigbuf,ch);
   }

  return;
 }  


void do_memory( CHAR_DATA *ch, char *argument )
{
    char buf[MAX_STRING_LENGTH];

    sprintf( buf, "Affects %5d\n\r", top_affect    ); send_to_char( buf, ch );
    sprintf( buf, "Areas   %5d\n\r", top_area      ); send_to_char( buf, ch );
    sprintf( buf, "Cstats  %5d\n\r", top_cstat      ); send_to_char( buf, ch );
    sprintf( buf, "ExDes   %5d\n\r", top_ed        ); send_to_char( buf, ch );
    sprintf( buf, "Exits   %5d\n\r", top_exit      ); send_to_char( buf, ch );
    sprintf( buf, "Helps   %5d\n\r", top_help      ); send_to_char( buf, ch );
    sprintf( buf, "Socials %5d\n\r", social_count  ); send_to_char( buf, ch );
    sprintf( buf, "Mobs    %5d(%d new format)\n\r", top_mob_index,newmobs ); 
    send_to_char( buf, ch );
    sprintf( buf, "(in use)%5d\n\r", mobile_count  ); send_to_char( buf, ch );
    sprintf( buf, "Objs    %5d(%d new format)\n\r", top_obj_index,newobjs ); 
    send_to_char( buf, ch );
    sprintf( buf, "Resets  %5d\n\r", top_reset     ); send_to_char( buf, ch );
    sprintf( buf, "Rooms   %5d\n\r", top_room      ); send_to_char( buf, ch );
    sprintf( buf, "Shops   %5d\n\r", top_shop      ); send_to_char( buf, ch );

    sprintf( buf, "Strings %5d strings of %7d bytes (max %d).\n\r",
  nAllocString, sAllocString, MAX_STRING );
    send_to_char( buf, ch );

#ifdef OLC_VERSION
    sprintf( buf, "Perms   %5d blocks  of %7d bytes.\n\r", 
	     nAllocPerm, sAllocPerm );
    send_to_char( buf, ch );
#endif
}

void do_eqlist( CHAR_DATA *ch, char *argument )
{
   OBJ_INDEX_DATA *pObjIndex;
   AFFECT_DATA *paf;
   FILE *fp;
   int vnum,nMatch,wcount = 0;
   char wtype[12];
   char arg1[MAX_INPUT_LENGTH];
   char buf[MAX_STRING_LENGTH];
   char buf2[MAX_STRING_LENGTH];
   char fname[20];

   argument = one_argument(argument, arg1);

   if (arg1[0] == '\0')
   {
      send_to_char("Arguement missing see 'help eqlist'.\n\r",ch);
      return;
   }

//   fclose(fpReserve);

   strcpy(fname, "eqlist.eql");

   if (!str_prefix("armor",arg1))
   {
      strcpy(fname, "armor.eql");
   }
   if (!str_prefix("weapons",arg1))
   {
      strcpy(fname, "weapons.eql");
   }

   /* open file */
   fp = fopen(fname,"w");

   if (!str_prefix("armor",arg1) || !str_prefix("all",arg1))
   {
      nMatch = 0;
      for (vnum = 0; nMatch < top_obj_index; vnum++)
         if ((pObjIndex = get_obj_index(vnum)) != NULL)
         {
            nMatch++;
if( !IS_OBJ_STAT(pObjIndex,ITEM_NOIDENTIFY))
{
            if ((pObjIndex->item_type == ITEM_ARMOR ||
		 pObjIndex->item_type == ITEM_LIGHT ||
                 pObjIndex->item_type == ITEM_CLOTHING ||
                 pObjIndex->item_type == ITEM_JEWELRY) &&
                 pObjIndex->level < 52)
	    {

   if (pObjIndex->wear_flags & ITEM_WEAR_FINGER ) strcpy(wtype, "finger");
   if (pObjIndex->wear_flags & ITEM_WEAR_NECK ) strcpy(wtype, "neck");
   if (pObjIndex->wear_flags & ITEM_WEAR_BODY ) strcpy(wtype, "torso");
   if (pObjIndex->wear_flags & ITEM_WEAR_HEAD ) strcpy(wtype, "head");
   if (pObjIndex->wear_flags & ITEM_WEAR_LEGS ) strcpy(wtype, "legs");
   if (pObjIndex->wear_flags & ITEM_WEAR_FEET ) strcpy(wtype, "feet");
   if (pObjIndex->wear_flags & ITEM_WEAR_HANDS  ) strcpy(wtype, "hands");
   if (pObjIndex->wear_flags & ITEM_WEAR_ARMS ) strcpy(wtype, "arms");
   if (pObjIndex->wear_flags & ITEM_WEAR_SHIELD ) strcpy(wtype, "shield");
   if (pObjIndex->wear_flags & ITEM_WEAR_ABOUT  ) strcpy(wtype, "body");
   if (pObjIndex->wear_flags & ITEM_WEAR_WAIST  ) strcpy(wtype, "waist");
   if (pObjIndex->wear_flags & ITEM_WEAR_WRIST  ) strcpy(wtype, "wrist");
   if (pObjIndex->wear_flags & ITEM_WIELD   ) strcpy(wtype, "wield");
   if (pObjIndex->wear_flags & ITEM_HOLD    ) strcpy(wtype, "hold");
   if (pObjIndex->wear_flags & ITEM_WEAR_FLOAT  ) strcpy(wtype, "float");
   if (pObjIndex->item_type == ITEM_LIGHT) strcpy(wtype,"light");

   strcpy(buf," ");
    for ( paf = pObjIndex->affected; paf != NULL; paf = paf->next )
    {
       if (paf->modifier > 0)
	  strcat( buf, "+");
       sprintf(buf2, "%s%d",buf,paf->modifier);
       strcpy( buf, buf2);

       strcpy(buf2,affect_loc_name( paf->location ));
       if (!str_prefix("strength",buf2))
          strcpy( buf2,"str");
       if (!str_prefix("intelligence",buf2))
          strcpy( buf2,"int");
       if (!str_prefix("wisdom",buf2))
          strcpy( buf2,"wis");
       if (!str_prefix("dexterity",buf2))
          strcpy( buf2,"dex");
       if (!str_prefix("constitution",buf2))
          strcpy( buf2,"con");
       if (!str_prefix("damage roll",buf2))
          strcpy( buf2,"dam");
       if (!str_prefix("hit roll",buf2))
          strcpy( buf2,"hit");
       if (!str_prefix("armor class",buf2))
	  strcpy( buf2,"ac");
       strcat( buf, buf2);
       strcat( buf, " ");
    }
    fprintf(fp,"%d %d \"%s\" wear=%s w=%d v=%d EF(%s) %d/%d/%d/%d AF(%s) a=\"%s\"\n", 
    pObjIndex->vnum,pObjIndex->level,pObjIndex->short_descr,
    wtype,pObjIndex->weight,pObjIndex->cost,
    extra_bit_name(pObjIndex->extra_flags),
    pObjIndex->value[0],pObjIndex->value[1],
    pObjIndex->value[2],pObjIndex->value[3],
    buf,
    pObjIndex->area->name);
            }
	 }
   }

   if (!str_prefix("weapons",arg1) || !str_prefix("all",arg1))
   {
     for (wcount = 0; wcount < 10; wcount++)
     {
       switch (wcount)
       {
          case(WEAPON_EXOTIC):
          strcpy(wtype,"exotic");
          break;
          case(WEAPON_SWORD):
          strcpy(wtype,"sword");
          break;
          case(WEAPON_DAGGER):
          strcpy(wtype,"dagger");
          break;
          case(WEAPON_SPEAR):
          strcpy(wtype,"spear");
          break;
          case(WEAPON_MACE):
          strcpy(wtype,"mace");
          break;
          case(WEAPON_AXE):
          strcpy(wtype,"axe");
          break;
          case(WEAPON_FLAIL):
          strcpy(wtype,"flail");
          break;
          case(WEAPON_WHIP):
          strcpy(wtype,"whip");
          break;
          case(WEAPON_POLEARM):
          strcpy(wtype,"polearm");
          break;
          case(WEAPON_GAROTTE):
          strcpy(wtype,"garotte");
          break;
       }
       fprintf(fp,"####\n");
       fprintf(fp,"%s\n",wtype);
      nMatch = 0;         
      for (vnum = 0; nMatch < top_obj_index; vnum++)
         if ((pObjIndex = get_obj_index(vnum)) != NULL)
	 {
            nMatch++;
            if (pObjIndex->item_type == ITEM_WEAPON &&
		pObjIndex->value[0] == wcount &&
	         pObjIndex->level < 52)
	    {
      strcpy(buf," ");
      for ( paf = pObjIndex->affected; paf != NULL; paf = paf->next )
      {
         if (paf->modifier > 0)
            strcat( buf, "+");
         sprintf(buf2, "%s%d",buf,paf->modifier);
         strcpy( buf, buf2);

       strcpy(buf2,affect_loc_name( paf->location ));
       if (!str_prefix("strength",buf2))
          strcpy( buf2,"str");
       if (!str_prefix("intelligence",buf2))
	  strcpy( buf2,"int");
       if (!str_prefix("wisdom",buf2))
          strcpy( buf2,"wis");
       if (!str_prefix("dexterity",buf2))
          strcpy( buf2,"dex");
       if (!str_prefix("constitution",buf2))
          strcpy( buf2,"con");
       if (!str_prefix("damage roll",buf2))
          strcpy( buf2,"dam");
       if (!str_prefix("hit roll",buf2))
          strcpy( buf2,"hit");
       if (!str_prefix("armor class",buf2))
	  strcpy( buf2,"ac");
       strcat( buf, buf2);
         strcat( buf, " ");
      }
        fprintf(fp,"%d %d \"%s\" w=%d v=%d madeof=\"%s\" damtype=%d EF(%s) AF(%s) WF(%s) %dD%d(%d) a=\"%s\"\n",
	   pObjIndex->vnum,pObjIndex->level,pObjIndex->short_descr,
	   pObjIndex->weight,pObjIndex->cost,
	   pObjIndex->material, pObjIndex->value[3],
	   extra_bit_name(pObjIndex->extra_flags),
	   buf,
	   weapon_bit_name(pObjIndex->value[4]),
	   pObjIndex->value[1],pObjIndex->value[2],
	   ((1 + pObjIndex->value[2]) * pObjIndex->value[1] / 2),
	   pObjIndex->area->name);
	    }
	 }
} /* IF NO_INDENT */ 
     }       
   }

   /* close file */
   fclose(fp);
}

void do_dump( CHAR_DATA *ch, char *argument )
{ 
    int count,count2,num_pcs,aff_count;
    CHAR_DATA *fch;
    MOB_INDEX_DATA *pMobIndex;
    PC_DATA *pc;
    OBJ_DATA *obj;
    OBJ_INDEX_DATA *pObjIndex;
    ROOM_INDEX_DATA *room;
    EXIT_DATA *exit;
    DESCRIPTOR_DATA *d;
    AFFECT_DATA *af;
    FILE *fp;
    int vnum,nMatch = 0;

    /* open file */
//    fclose(fpReserve);
    fp = fopen("mem.dmp","w");

    /* report use of data structures */
    
    num_pcs = 0;
    aff_count = 0;

    /* mobile prototypes */
    fprintf(fp,"MobProt %4d (%8d bytes)\n",
  top_mob_index, top_mob_index * (sizeof(*pMobIndex))); 

    /* mobs */
    count = 0;  count2 = 0;
    for (fch = char_list; fch != NULL; fch = fch->next)
    {
  count++;
  if (fch->pcdata != NULL)
      num_pcs++;
  for (af = fch->affected; af != NULL; af = af->next)
      aff_count++;
    }
    for (fch = char_free; fch != NULL; fch = fch->next)
  count2++;

    fprintf(fp,"Mobs  %4d (%8d bytes), %2d free (%d bytes)\n",
  count, count * (sizeof(*fch)), count2, count2 * (sizeof(*fch)));

    /* pcdata */
    count = 0;
    for (pc = pcdata_free; pc != NULL; pc = pc->next)
  count++; 

    fprintf(fp,"Pcdata  %4d (%8d bytes), %2d free (%d bytes)\n",
  num_pcs, num_pcs * (sizeof(*pc)), count, count * (sizeof(*pc)));

    /* descriptors */
    count = 0; count2 = 0;
    for (d = descriptor_list; d != NULL; d = d->next)
  count++;
    for (d= descriptor_free; d != NULL; d = d->next)
  count2++;

    fprintf(fp, "Descs  %4d (%8d bytes), %2d free (%d bytes)\n",
  count, count * (sizeof(*d)), count2, count2 * (sizeof(*d)));

    /* object prototypes */
    for ( vnum = 0; nMatch < top_obj_index; vnum++ )
        if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
        {
      for (af = pObjIndex->affected; af != NULL; af = af->next)
    aff_count++;
            nMatch++;
        }

    fprintf(fp,"ObjProt %4d (%8d bytes)\n",
  top_obj_index, top_obj_index * (sizeof(*pObjIndex)));


    /* objects */
    count = 0;  count2 = 0;
    for (obj = object_list; obj != NULL; obj = obj->next)
    {
  count++;
  for (af = obj->affected; af != NULL; af = af->next)
      aff_count++;
    }
    for (obj = obj_free; obj != NULL; obj = obj->next)
  count2++;

    fprintf(fp,"Objs  %4d (%8d bytes), %2d free (%d bytes)\n",
  count, count * (sizeof(*obj)), count2, count2 * (sizeof(*obj)));

    /* affects */
    count = 0;
    for (af = affect_free; af != NULL; af = af->next)
  count++;

    fprintf(fp,"Affects %4d (%8d bytes), %2d free (%d bytes)\n",
  aff_count, aff_count * (sizeof(*af)), count, count * (sizeof(*af)));

    /* rooms */
    fprintf(fp,"Rooms %4d (%8d bytes)\n",
  top_room, top_room * (sizeof(*room)));

     /* exits */
    fprintf(fp,"Exits %4d (%8d bytes)\n",
  top_exit, top_exit * (sizeof(*exit)));

    fclose(fp);

    /* start printing out mobile data */
    fp = fopen("mob.dmp","w");

    fprintf(fp,"\nMobile Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_mob_index; vnum++)
  if ((pMobIndex = get_mob_index(vnum)) != NULL)
  {
      nMatch++;
      fprintf(fp,"#%-4d %3d active %3d killed     %s\n",
    pMobIndex->vnum,pMobIndex->count,
    pMobIndex->killed,pMobIndex->short_descr);
  }
    fclose(fp);

    /* start printing out object data */
    fp = fopen("obj.dmp","w");

    fprintf(fp,"\nObject Analysis\n");
    fprintf(fp,  "---------------\n");
    nMatch = 0;
    for (vnum = 0; nMatch < top_obj_index; vnum++)
  if ((pObjIndex = get_obj_index(vnum)) != NULL)
  {
      nMatch++;
      fprintf(fp,"#%-4d %3d active %3d reset      %s\n",
    pObjIndex->vnum,pObjIndex->count,
    pObjIndex->reset_num,pObjIndex->short_descr);
  }

    /* close file */
    fclose(fp);
//    fpReserve = fopen( NULL_FILE, "r" );
}


/*
 * Stick a little fuzz on a number.
 */
int number_fuzzy( int number )
{
    switch ( number_bits( 2 ) )
    {
    case 0:  number -= 1; break;
    case 3:  number += 1; break;
    }

    return UMAX( 1, number );
}



/*
 * Generate a random number.
 */
int number_range( int from, int to )
{
    int power;
    int number;

    if (from == 0 && to == 0)
  return 0;

    if ( ( to = to - from + 1 ) <= 1 )
  return from;

    for ( power = 2; power < to; power <<= 1 )
  ;

    while ( ( number = number_mm() & (power -1 ) ) >= to )
  ;

    return from + number;
}



/*
 * Generate a percentile roll.
 */
int number_percent( void )
{
    /* Going to try just a remainder()
    int percent;

    while ( (percent = number_mm() & (128-1) ) > 99 );

    return 1 + percent;
     */

//  int foo=50;

//  foo = remainder((random()>>6),100) + 50;
 
// return foo;
  return number_range(0, 99);
}



/*
 * Generate a random door.
 */
int number_door( void )
{
    int door;

    while ( ( door = number_mm() & (8-1) ) > 5)
  ;

    return door;
}

int number_bits( int width )
{
    return number_mm( ) & ( ( 1 << width ) - 1 );
}




/*
 * I've gotten too many bad reports on OS-supplied random number generators.
 * This is the Mitchell-Moore algorithm from Knuth Volume II.
 * Best to leave the constants alone unless you've read Knuth.
 * -- Furey
 */
#if defined (OLD_RAND)
static  int     rgiState[2+55];
#endif
 
void init_mm( )
{
#if defined (OLD_RAND)
    int *piState;
    int iState;
 
    piState     = &rgiState[2];
 
    piState[-2] = 55 - 55;
    piState[-1] = 55 - 24;
 
    piState[0]  = ((int) current_time) & ((1 << 30) - 1);
    piState[1]  = 1;
    for ( iState = 2; iState < 55; iState++ )
    {
        piState[iState] = (piState[iState-1] + piState[iState-2])
                        & ((1 << 30) - 1);
    }
#else
    srandom(time(NULL)^getpid());
#endif
    return;
}
 
 
 
long number_mm( void )
{
#if defined (OLD_RAND)
    int *piState;
    int iState1;
    int iState2;
    int iRand;
 
    piState             = &rgiState[2];
    iState1             = piState[-2];
    iState2             = piState[-1];
    iRand               = (piState[iState1] + piState[iState2])
                        & ((1 << 30) - 1);
    piState[iState1]    = iRand;
    if ( ++iState1 == 55 )
        iState1 = 0;
    if ( ++iState2 == 55 )
        iState2 = 0;
    piState[-2]         = iState1;
    piState[-1]         = iState2;
    return iRand >> 6;
#else
    return random() >> 6;
#endif
}


/*
 * Roll some dice.
 */
int dice( int number, int size )
{
    int idice;
    int sum;

    switch ( size )
    {
    case 0: return 0;
    case 1: return number;
    }

    for ( idice = 0, sum = 0; idice < number; idice++ )
  sum += number_range( 1, size );

    return sum;
}



/*
 * Simple linear interpolation.
 */
int interpolate( int level, int value_00, int value_32 )
{
    return value_00 + level * (value_32 - value_00) / 32;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void smash_tilde( char *str )
{
    for ( ; *str != '\0'; str++ )
    {
  if ( *str == '~' )
      *str = '-';
    }

    return;
}



/*
 * Compare strings, case insensitive.
 * Return TRUE if different
 *   (compatibility with historical functions).
 */
bool str_cmp( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
  char buf[256];
  sprintf(buf, "Str_cmp: null astr. bstr: ");
  if(bstr != NULL)
    strcat(buf, bstr);
  bug( buf, 0 );
  return TRUE;
    }

    if ( bstr == NULL )
    {
  char buf[256];
  sprintf(buf, "Str_cmp: null bstr. astr: %s", astr);
  bug( buf, 0 );
  return TRUE;
    }

    for ( ; *astr || *bstr; astr++, bstr++ )
    {
  if ( LOWER(*astr) != LOWER(*bstr) )
      return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool str_prefix( const char *astr, const char *bstr )
{
    if ( astr == NULL )
    {
  bug( "Strn_cmp: null astr.", 0 );
  return TRUE;
    }

    if ( bstr == NULL )
    {
  char buf[256];
  sprintf(buf, "Strn_cmp: null bstr. astr: %s", astr);
  bug(buf, 0);
//  bug( "Strn_cmp: null bstr.", 0 );
  return TRUE;
    }

    for ( ; *astr; astr++, bstr++ )
    {
  if ( LOWER(*astr) != LOWER(*bstr) )
      return TRUE;
    }

    return FALSE;
}



/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool str_infix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if ( ( c0 = LOWER(astr[0]) ) == '\0' )
  return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for ( ichar = 0; ichar <= sstr2 - sstr1; ichar++ )
    {
  if ( c0 == LOWER(bstr[ichar]) && !str_prefix( astr, bstr + ichar ) )
      return FALSE;
    }

    return TRUE;
}



/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool str_suffix( const char *astr, const char *bstr )
{
    int sstr1;
    int sstr2;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if ( sstr1 <= sstr2 && !str_cmp( astr, bstr + sstr2 - sstr1 ) )
  return FALSE;
    else
  return TRUE;
}



/*
 * Returns an initial-capped string.
 */
char *capitalize( const char *str )
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for ( i = 0; str[i] != '\0'; i++ )
  strcap[i] = LOWER(str[i]);
    strcap[i] = '\0';
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}


/*
 * Append a string to a file.
 */
void append_file( CHAR_DATA *ch, char *file, char *str )
{
    FILE *fp;

    if ( IS_NPC(ch) || str[0] == '\0' )
  return;

//    fclose( fpReserve );
    if ( ( fp = fopen( file, "a" ) ) == NULL )
    {
  perror( file );
  send_to_char( "Could not open the file!\n\r", ch );
    }
    else
    {
  fprintf( fp, "[%5d] %s: %s\n",
      ch->in_room ? ch->in_room->vnum : 0, ch->name, str );
  fclose( fp );
    }

//    fpReserve = fopen( NULL_FILE, "r" );
    return;
}



/*
 * Reports a bug.
 */
void bug( const char *str, int param )
{
    char buf[MAX_STRING_LENGTH];

    if ( fpArea != NULL )
    {
  int iLine;
  int iChar;

  if ( fpArea == stdin )
  {
      iLine = 0;
  }
  else
  {
      iChar = ftell( fpArea );
      fseek( fpArea, 0, 0 );
      for ( iLine = 0; ftell( fpArea ) < iChar; iLine++ )
      {
    while ( getc( fpArea ) != '\n' )
        ;
      }
      fseek( fpArea, iChar, 0 );
  }

  sprintf( buf, "[*****] FILE: %s LINE: %d", strArea, iLine );
  log_string( buf );
/*
  if ( ( fp = fopen( "shutdown.txt", "a" ) ) != NULL )
  {
      fprintf( fp, "[*****] %s\n", buf );
      fclose( fp );
  }
*/
    }

    strcpy( buf, "[*****] BUG: " );
    sprintf( buf + strlen(buf), str, param );
    log_string( buf );
/* RT removed due to bug-file spamming 
//    fclose( fpReserve );
    if ( ( fp = fopen( BUG_FILE, "a" ) ) != NULL )
    {
  fprintf( fp, "%s\n", buf );
  fclose( fp );
    }
//    fpReserve = fopen( NULL_FILE, "r" );
*/

    return;
}



/*
 * Writes a string to the log.
 */
void log_string( const char *str )
{
    char *strtime;

    strtime                    = ctime( &current_time );
    strtime[strlen(strtime)-1] = '\0';
    fprintf( stderr, "%s :: %s\n", strtime, str );
    return;
}



/*
 * This function is here to aid in debugging.
 * If the last expression in a function is another function call,
 *   gcc likes to generate a JMP instead of a CALL.
 * This is called "tail chaining."
 * It hoses the debugger call stack for that call.
 * So I make this the last call in certain critical functions,
 *   where I really need the call stack to be right for debugging!
 *
 * If you don't understand this, then LEAVE IT ALONE.
 * Don't remove any calls to tail_chain anywhere.
 *
 * -- Furey
 */
void tail_chain( void )
{
    return;
}
