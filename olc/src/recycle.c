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
 
static char rcsid[] = "$Id: recycle.c,v 1.8 2001/04/09 18:33:00 mud Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include "gc.h"
#include <stdlib.h>
#include "merc.h"
#include "recycle.h"

/* stuff for recyling notes */
NOTE_DATA *note_free = NULL;

NOTE_DATA *new_note()
{
    NOTE_DATA *note;

    if (note_free == NULL)
#ifdef OLC_VERSION
   note = alloc_perm(sizeof(*note));
#else
  note = GC_MALLOC(sizeof(*note));
#endif
    else
    { 
  note = note_free;
  note_free = note_free->next;
    }
    VALIDATE(note);
    return note;
}

void free_note(NOTE_DATA *note)
{
    if (!IS_VALID(note))
  return;

    free_string( note->text    );
    free_string( note->subject );
    free_string( note->to_list );
    free_string( note->date    );
    free_string( note->sender  );
    INVALIDATE(note);

    note->next = note_free;
    note_free   = note;
}

/* stuff for recycling dns structures */
DNS_DATA *dns_free = NULL;

DNS_DATA *new_dns(void)
{
    static DNS_DATA dns_zero;
    DNS_DATA *dns;

    if (dns_free == NULL)
#ifdef OLC_VERSION
  dns = alloc_perm(sizeof(*dns));
#else
  dns = GC_MALLOC(sizeof(*dns));
#endif
    else
    {
  dns = dns_free;
  dns_free = dns_free->next;
    }

    *dns = dns_zero;
    VALIDATE(dns);
    dns->name = &str_empty[0];
    return dns;
}

void free_dns(DNS_DATA *dns)
{
    if (!IS_VALID(dns))
  return;

    free_string(dns->name);
    INVALIDATE(dns);

    dns->next = dns_free;
    dns_free = dns;
}
    
/* stuff for recycling ban structures */
BAN_DATA *ban_free = NULL;

BAN_DATA *new_ban(void)
{
    static BAN_DATA ban_zero;
    BAN_DATA *ban;

    if (ban_free == NULL)
#ifdef OLC_VERSION
  ban = alloc_perm(sizeof(*ban));
#else
  ban = GC_MALLOC(sizeof(*ban));
#endif
    else
    {
  ban = ban_free;
  ban_free = ban_free->next;
    }

    *ban = ban_zero;
    VALIDATE(ban);
    ban->name = &str_empty[0];
    return ban;
}

void free_ban(BAN_DATA *ban)
{
    if (!IS_VALID(ban))
  return;

    free_string(ban->name);
    INVALIDATE(ban);

    ban->next = ban_free;
    ban_free = ban;
}

/* stuff for recycling descriptors */
DESCRIPTOR_DATA *descriptor_free = NULL;

DESCRIPTOR_DATA *new_descriptor(void)
{
    static DESCRIPTOR_DATA d_zero;
    DESCRIPTOR_DATA *d;

    if (descriptor_free == NULL)
#ifdef OLC_VERSION
  d = alloc_perm(sizeof(*d));
#else
  d = GC_MALLOC(sizeof(*d));
#endif
    else
    {
  d = descriptor_free;
  descriptor_free = descriptor_free->next;
    }
  
    *d = d_zero;
    VALIDATE(d);
    return d;
}

void free_descriptor(DESCRIPTOR_DATA *d)
{
  DESCRIPTOR_DATA *desc;
  extern DESCRIPTOR_DATA *   descriptor_list;
  
    if (!IS_VALID(d))
  return;
    
    for (desc = descriptor_list; desc != NULL; desc = desc->next ) {
      if (d == desc) {
        bug ("free_descriptor: multiple descriptor",0);
        return;
      }
    }
  

    free_string( d->host );
    free_string( d->name );
    free_mem( d->outbuf, d->outsize );
    INVALIDATE(d);
    d->next = descriptor_free;
    descriptor_free = d;
}

/* stuff for recycling gen_data */
GEN_DATA *gen_data_free = NULL;

GEN_DATA *new_gen_data(void)
{
    static GEN_DATA gen_zero;
    GEN_DATA *gen;

    if (gen_data_free == NULL)
#ifdef OLC_VERSION
  gen = alloc_perm(sizeof(*gen));
#else
  gen = GC_MALLOC(sizeof(*gen));
#endif
    else
    {
  gen = gen_data_free;
  gen_data_free = gen_data_free->next;
    }
    *gen = gen_zero;
    VALIDATE(gen);
    return gen;
}

void free_gen_data(GEN_DATA *gen)
{
    if (!IS_VALID(gen))
  return;

    INVALIDATE(gen);

    gen->next = gen_data_free;
    gen_data_free = gen;
} 

/* stuff for recycling extended descs */
EXTRA_DESCR_DATA *extra_descr_free = NULL;

EXTRA_DESCR_DATA *new_extra_descr(void)
{
    EXTRA_DESCR_DATA *ed;

    if (extra_descr_free == NULL)
#ifdef OLC_VERSION
  ed = alloc_perm(sizeof(*ed));
#else
  ed = GC_MALLOC(sizeof(*ed));
#endif
    else
    {
  ed = extra_descr_free;
  extra_descr_free = extra_descr_free->next;
    }

    ed->keyword = &str_empty[0];
    ed->description = &str_empty[0];
    VALIDATE(ed);
    return ed;
}

void free_extra_descr(EXTRA_DESCR_DATA *ed)
{
    if (!IS_VALID(ed))
  return;

    free_string(ed->keyword);
    free_string(ed->description);
    INVALIDATE(ed);
    
    ed->next = extra_descr_free;
    extra_descr_free = ed;
}

/* stuff for recycling clan data */
CLAN_DATA *clan_free = NULL;

CLAN_DATA *new_clan(void)
{
    static CLAN_DATA clan_zero;
    CLAN_DATA *clan;

    if (clan_free == NULL)
    {
#ifdef OLC_VERSION
   clan = alloc_perm(sizeof(*clan));
#else
  clan = GC_MALLOC(sizeof(*clan));
#endif
    }
    else
    {
  clan = clan_free;
  clan_free = clan_free->next;
    }

    *clan = clan_zero;

    VALIDATE(clan);

    return clan;
}

void free_clan(CLAN_DATA *clan)
{
    if (!IS_VALID(clan))
  return;

 clear_string(&clan->name, NULL);
 clear_string(&clan->charter, NULL);
 clear_string(&clan->rules, NULL);
    INVALIDATE(clan);
    clan->next = clan_free;
    clan_free = clan;
  while(clan->to_match)
  {
    MERIT_TRACKER *temp = clan->to_match->next;
    free_merit(clan->to_match);
    clan->to_match = temp;
  }
}

/* stuff for recycling damage tracking */
CLAN_CHAR *clan_char_free = NULL;

CLAN_CHAR *new_clan_char(void)
{
    static CLAN_CHAR cc_zero;
    CLAN_CHAR *cchar;

    if (clan_char_free == NULL)
#ifdef OLC_VERSION
   cchar = alloc_perm(sizeof(*cchar));
#else
  cchar = GC_MALLOC(sizeof(*cchar));
#endif
    else
    {
  cchar = clan_char_free;
  clan_char_free = clan_char_free->next;
    }

    *cchar = cc_zero;


    VALIDATE(cchar);

    return cchar;
}

void free_clan_char(CLAN_CHAR *cchar)
{
    if (!IS_VALID(cchar))
  return;

    INVALIDATE(cchar);
    cchar->next = clan_char_free;
    clan_char_free = cchar;
  clear_string(&cchar->messages, NULL);
  while(cchar->delay_merit)
  {
    MERIT_TRACKER *temp = cchar->delay_merit->next;
    free_merit(cchar->delay_merit);
    cchar->delay_merit = temp;
  }
}

/* stuff for recycling damage tracking */
ALLIANCE_DATA *ally_free = NULL;

ALLIANCE_DATA *new_ally(void)
{
    static ALLIANCE_DATA ad_zero;
    ALLIANCE_DATA *ally;

    if (ally_free == NULL)
#ifdef OLC_VERSION
   ally = alloc_perm(sizeof(*ally));
#else
  ally = GC_MALLOC(sizeof(*ally));
#endif
    else
    {
  ally = ally_free;
  ally_free = ally_free->next;
    }

    *ally = ad_zero;


    VALIDATE(ally);

    return ally;
}

void free_ally(ALLIANCE_DATA *ally)
{
    if (!IS_VALID(ally))
  return;

    INVALIDATE(ally);
    ally->next = ally_free;
    ally_free = ally;
}

/* stuff for recycling damage tracking */
MERIT_TRACKER *merit_free = NULL;

MERIT_TRACKER *new_merit(void)
{
    static MERIT_TRACKER mt_zero;
    MERIT_TRACKER *merit;

    if (merit_free == NULL)
#ifdef OLC_VERSION
  merit = alloc_perm(sizeof(*merit));
#else
  merit = GC_MALLOC(sizeof(*merit));
#endif
    else
    {
  merit = merit_free;
  merit_free = merit_free->next;
    }

    *merit = mt_zero;


    VALIDATE(merit);

    return merit;
}

void free_merit(MERIT_TRACKER *merit)
{
  if (!IS_VALID(merit))
    return;

  INVALIDATE(merit);
  merit->next = merit_free;
  merit_free = merit;
}


/* stuff for recycling plan data */
PLAN_DATA *plan_free = NULL;

PLAN_DATA *new_plan(void)
{
    static PLAN_DATA plan_zero;
    PLAN_DATA *plan;

    if (plan_free == NULL)
#ifdef OLC_VERSION
   plan = alloc_perm(sizeof(*plan));
#else
  plan = GC_MALLOC(sizeof(*plan));
#endif
    else
    {
  plan = plan_free;
  plan_free = plan_free->next;
    }

    *plan = plan_zero;

    VALIDATE(plan);
    plan->label = NULL;
    return plan;
}

void free_plan(PLAN_DATA *plan)
{
    if (!IS_VALID(plan))
  return;
  
  clear_string(&plan->label, NULL);
  clear_string(&plan->name, NULL);
  clear_string(&plan->short_d, NULL);
  clear_string(&plan->long_d, NULL);
  clear_string(&plan->desc, NULL);
  clear_string(&plan->previewer, NULL);

  if(plan->exits != NULL)
  {
    free_p_exit(plan->exits);
    plan->exits = NULL;
  }
  
  INVALIDATE(plan);
  plan->next = plan_free;
  plan_free = plan;
}

/* stuff for recycling plan exit data */
PLAN_EXIT_DATA *p_exit_free = NULL;

PLAN_EXIT_DATA *new_p_exit(void)
{
    int i;
    static PLAN_EXIT_DATA p_exit_zero;
    PLAN_EXIT_DATA *p_exit;

    if (p_exit_free == NULL)
#ifdef OLC_VERSION
   p_exit = alloc_perm(sizeof(*p_exit) * 6);
#else
  p_exit = GC_MALLOC(sizeof(*p_exit) * 6);
#endif
    else
    {
  p_exit = p_exit_free;
  p_exit_free = p_exit_free->next;
    }
    for(i = 0; i < 6; i++)
      p_exit[i] = p_exit_zero;

    VALIDATE(p_exit);
    return p_exit;
}

void free_p_exit(PLAN_EXIT_DATA *p_exit)
{
    if (!IS_VALID(p_exit))
  return;
  
  INVALIDATE(p_exit);
  p_exit->next = p_exit_free;
  p_exit_free = p_exit;
}

/* stuff for recycling damage tracking */
DAMAGE_DATA *damage_free = NULL;

DAMAGE_DATA *new_damage(void)
{
    static DAMAGE_DATA da_zero;
    DAMAGE_DATA *da;

    if (damage_free == NULL)
#ifdef OLC_VERSION
   da = alloc_perm(sizeof(*da));
#else
  da = GC_MALLOC(sizeof(*da));
#endif
    else
    {
  da = damage_free;
  damage_free = damage_free->next;
    }

    *da = da_zero;


    VALIDATE(da);
    return da;
}

void free_damage(DAMAGE_DATA *da)
{
    if (!IS_VALID(da))
  return;

    INVALIDATE(da);
    clear_string(&da->source, NULL);
    da->next = damage_free;
    damage_free = da;
}

/* stuff for recycling affects */
AFFECT_DATA *affect_free = NULL;

AFFECT_DATA *new_affect(void)
{
    static AFFECT_DATA af_zero;
    AFFECT_DATA *af;

    if (affect_free == NULL)
#ifdef OLC_VERSION
   af = alloc_perm(sizeof(*af));
#else
  af = GC_MALLOC(sizeof(*af));
#endif
    else
    {
  af = affect_free;
  affect_free = affect_free->next;
    }

    *af = af_zero;


    VALIDATE(af);
    return af;
}

void free_affect(AFFECT_DATA *af)
{
    if (!IS_VALID(af))
  return;

    INVALIDATE(af);
    af->next = affect_free;
    affect_free = af;
}

/* MudTrader
*/

TRADE_DATA *trade_free = NULL;

TRADE_DATA *new_trade(void)
{
    static TRADE_DATA trade_zero;
    TRADE_DATA *trade;

    if (trade_free == NULL)
#ifdef OLC_VERSION
  trade = alloc_perm(sizeof(*trade));
#else
  trade = GC_MALLOC(sizeof(*trade));
#endif
  else
    {
  trade = trade_free;
  trade_free = trade_free->next;
    }
   *trade = trade_zero;
   VALIDATE(trade);

   return trade;
}

void	free_trade( TRADE_DATA *trade )
{
      INVALIDATE(trade);

      trade->next   = trade_free;
      trade_free    = trade; 
}

/* stuff for recycling objects */
OBJ_DATA *obj_free = NULL;

OBJ_DATA *new_obj(void)
{
    static OBJ_DATA obj_zero;
    OBJ_DATA *obj;

    if (obj_free == NULL)
#ifdef OLC_VERSION
  obj = alloc_perm(sizeof(*obj));
#else
  obj = GC_MALLOC(sizeof(*obj));
#endif
    else
    {
  obj = obj_free;
  obj_free = obj_free->next;
    }
    *obj = obj_zero;
    VALIDATE(obj);

    return obj;
}

void free_obj(OBJ_DATA *obj)
{
    AFFECT_DATA *paf, *paf_next;
    EXTRA_DESCR_DATA *ed, *ed_next;
    DAMAGE_DATA *ddata;

    if (!IS_VALID(obj))
  return;

    for (paf = obj->affected; paf != NULL; paf = paf_next)
    {
  paf_next = paf->next;
  free_affect(paf);
    }
    obj->affected = NULL;

    for (ed = obj->extra_descr; ed != NULL; ed = ed_next )
    {
  ed_next = ed->next;
  free_extra_descr(ed);
     }
     obj->extra_descr = NULL;

    for(ddata = obj->loot_track; ddata; ddata = obj->loot_track)
    {
      obj->loot_track = obj->loot_track->next;
      free_damage(ddata);
    }
    obj->loot_track = NULL;   

    clear_string( &obj->name, NULL);
    clear_string( &obj->description, NULL );
    clear_string( &obj->short_descr, NULL );
    clear_string( &obj->owner, NULL);
    clear_string( &obj->material, NULL);
    if(obj->link_name)
    {
      free_string(obj->link_name);
      obj->link_name = NULL;
    }
    INVALIDATE(obj);

    obj->next   = obj_free;
    obj_free    = obj; 
}


/* stuff for recyling characters */
CHAR_DATA *char_free = NULL;

CHAR_DATA *new_char (void)
{
    static CHAR_DATA ch_zero;
    CHAR_DATA *ch;
    int i;

    if (char_free == NULL)
#ifdef OLC_VERSION
  ch = alloc_perm(sizeof(*ch));
#else
  ch = GC_MALLOC(sizeof(*ch));
#endif
    else
    {
  ch = char_free;
  char_free = char_free->next;
    }

    *ch       = ch_zero;
    VALIDATE(ch);
    ch->name                    = str_dup("");//&str_empty[0];
    ch->short_descr             = str_dup("");//&str_empty[0];
    ch->long_descr              = str_dup("");//&str_empty[0];
    ch->description             = str_dup("");//&str_empty[0];
    ch->prompt                  = NULL;//str_dup("");//&str_empty[0];
    ch->logon                   = current_time;
    ch->lines                   = PAGELEN;
    for (i = 0; i < 4; i++)
        ch->armor[i]            = 100;
    ch->position                = POS_STANDING;
    ch->hit                     = 20;
    ch->max_hit                 = 20;
    ch->mana                    = 100;
    ch->max_mana                = 100;
    ch->move                    = 100;
    ch->max_move                = 100;
    for (i = 0; i < MAX_STATS; i ++)
    {
        ch->perm_stat[i] = 13;
        ch->mod_stat[i] = 0;
    }

    return ch;
}


void free_char (CHAR_DATA *ch)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj_next;
    AFFECT_DATA *paf;
    AFFECT_DATA *paf_next;

    if (!IS_VALID(ch))
  return;

    if (IS_NPC(ch))
  mobile_count--;

    for (obj = ch->carrying; obj != NULL; obj = obj_next)
    {
  obj_next = obj->next_content;
  extract_obj(obj);
    }

    while(ch->flash_affected)
      flash_affect_remove(ch, ch->flash_affected, APPLY_BOTH);

    for (paf = ch->affected; paf != NULL; paf = paf_next)
    {
  paf_next = paf->next;
  affect_remove(ch,paf,APPLY_BOTH);
    }

    free_string(ch->name);
    free_string(ch->short_descr);
    free_string(ch->long_descr);
    free_string(ch->description);
    free_string(ch->prompt);

    if (ch->pnote != NULL)
      free_note(ch->pnote);

    if (ch->pcdata != NULL)
      free_pcdata(ch->pcdata);

    ch->next = char_free;
    char_free  = ch;

    INVALIDATE(ch);
    return;
}

PC_DATA *pcdata_free = NULL;

PC_DATA *new_pcdata(void)
{       
    int alias;

    static PC_DATA pcdata_zero;
    PC_DATA *pcdata;
    static EDIT_DATA edit_zero;

    if (pcdata_free == NULL)
#ifdef OLC_VERSION
  pcdata = alloc_perm(sizeof(*pcdata));
#else
  pcdata = GC_MALLOC(sizeof(*pcdata));
#endif
    else
    {
  pcdata = pcdata_free;
  pcdata_free = pcdata_free->next;
    }

    *pcdata = pcdata_zero;
    pcdata->macro = NULL;

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
        pcdata->alias[alias] = NULL;
        pcdata->alias_sub[alias] = NULL;
    }

    pcdata->buffer = new_buf();
    
    pcdata->edit = edit_zero;
    pcdata->edit.per_flags = EDIT_DEFAULT_ROOM|EDIT_DEFAULT_OBJ|EDIT_DEFAULT_MOB|
      EDIT_CREATE_MINIMAL|EDIT_DOUBLE_DOOR;  /* kris - take out after saving to char file */    
    
    VALIDATE(pcdata);
    return pcdata;
}
  

void free_pcdata(PC_DATA *pcdata)
{       
    MACRO_DATA *macro,*temp;
    int alias;

    if (!IS_VALID(pcdata))
  return;

    free_string(pcdata->pwd);
    free_string(pcdata->bamfin);
    free_string(pcdata->bamfout);
    free_string(pcdata->title);
    free_string(pcdata->who_name);
    free_buf(pcdata->buffer);
    
    macro = pcdata->macro;
    
    while (macro) {
      temp = macro->next;
      free_macro (macro);
      macro = temp;
    }
    
    pcdata->macro = NULL;    

    for (alias = 0; alias < MAX_ALIAS; alias++)
    {
        free_string(pcdata->alias[alias]);
        free_string(pcdata->alias_sub[alias]);
    }

    
    INVALIDATE(pcdata);
    pcdata->next = pcdata_free;
    pcdata_free = pcdata;

    return;
}

  


/* stuff for setting ids */
long  last_pc_id;
long  last_mob_id;

long get_pc_id(void)
{
    int val;

    val = (current_time <= last_pc_id) ? last_pc_id + 1 : current_time;
    last_pc_id = val;
    return val;
}

long get_mob_id(void)
{
    last_mob_id++;
    return last_mob_id;
}


/* procedures and constants needed for buffering */

BUFFER *buf_free = NULL;


/* buffer sizes */
const int buf_size[MAX_BUF_LIST] =
{
    16,32,64,128,256,1024,2048,4096,8192,16384
};

/* local procedure for finding the next acceptable size */
/* -1 indicates out-of-boundary error */
int get_size (int val)
{
    int i;

    for (i = 0; i < MAX_BUF_LIST; i++)
  if (buf_size[i] >= val)
  {
      return buf_size[i];
  }
    
    return -1;
}

BUFFER *new_buf()
{
    BUFFER *buffer;

    if (buf_free == NULL) 
#ifdef OLC_VERSION
  buffer = alloc_perm(sizeof(*buffer));
#else
  buffer = GC_MALLOC(sizeof(*buffer));
#endif
    else
    {
  buffer = buf_free;
  buf_free = buf_free->next;
    }

    buffer->next  = NULL;
    buffer->state = BUFFER_SAFE;
    buffer->size  = get_size(BASE_BUF);

#ifdef OLC_VERSION
    buffer->string  = alloc_mem(buffer->size);
#else
    buffer->string  = GC_MALLOC(buffer->size);
#endif
    buffer->string[0] = '\0';
    VALIDATE(buffer);

    return buffer;
}

BUFFER *new_buf_size(int size)
{
    BUFFER *buffer;
 
    if (buf_free ==  NULL)
#ifdef OLC_VERSION
        buffer = alloc_perm(sizeof(*buffer));
#else
        buffer = GC_MALLOC(sizeof(*buffer));
#endif
    else
    {
        buffer = buf_free;
        buf_free = buf_free->next;
    }
 
    buffer->next        = NULL;
    buffer->state       = BUFFER_SAFE;
    buffer->size        = get_size(size);
    if (buffer->size == -1)
    {
        bug("new_buf: buffer size %d too large.",size);
        exit(1);
    }
#ifdef OLC_VERSION
    buffer->string      = alloc_mem(buffer->size);
#else
    buffer->string      = GC_MALLOC(buffer->size);
#endif
    buffer->string[0]   = '\0';
    VALIDATE(buffer);
 
    return buffer;
}


void free_buf(BUFFER *buffer)
{
    if (!IS_VALID(buffer))
  return;

    free_mem(buffer->string,buffer->size);
    buffer->string = NULL;
    buffer->size   = 0;
    buffer->state  = BUFFER_FREED;
    INVALIDATE(buffer);

    buffer->next  = buf_free;
    buf_free      = buffer;
}


bool add_buf(BUFFER *buffer, char *string)
{
    int len;
    char *oldstr;
    int oldsize;

    oldstr = buffer->string;
    oldsize = buffer->size;

    if (buffer->state == BUFFER_OVERFLOW) /* don't waste time on bad strings! */
  return FALSE;

    len = strlen(buffer->string) + strlen(string) + 1;

    while (len >= buffer->size) /* increase the buffer size */
    {
  buffer->size  = get_size(buffer->size + 1);
  {
      if (buffer->size == -1) /* overflow */
      {
    buffer->size = oldsize;
    buffer->state = BUFFER_OVERFLOW;
    bug("buffer overflow past size %d",buffer->size);
    return FALSE;
      }
    }
    }

    if (buffer->size != oldsize)
    {
#ifdef OLC_VERSION
  buffer->string  = alloc_mem(buffer->size);
#else
  buffer->string  = GC_MALLOC(buffer->size);
#endif

  strcpy(buffer->string,oldstr);
  free_mem(oldstr,oldsize);
    }

    strcat(buffer->string,string);
    return TRUE;
}


void clear_buf(BUFFER *buffer)
{
    buffer->string[0] = '\0';
    buffer->state     = BUFFER_SAFE;
}


char *buf_string(BUFFER *buffer)
{
    return buffer->string;
}

EXIT_DATA *exit_free = NULL;  
    
EXIT_DATA *new_exit ()
{
  EXIT_DATA *exit;
  static EXIT_DATA z_exit;
  
  if ( exit_free == NULL )
  {
#ifdef OLC_VERSION
    exit            = alloc_perm( sizeof(*exit) );
#else
    exit            = GC_MALLOC( sizeof(*exit) );
#endif
  }
  else
  {
    exit            = exit_free;
    exit_free       = exit_free->next;
  }
  
  exit->u1.to_room = NULL;  
  *exit = z_exit;  
  return exit;
}

void free_exit (EXIT_DATA *exit)
{
  exit->next = exit_free;
  exit_free  = exit;
}

RESET_DATA *reset_free = NULL;

RESET_DATA *new_reset ()
{
  RESET_DATA *reset;
  static RESET_DATA z_reset;

  if ( reset_free == NULL )
  {
#ifdef OLC_VERSION
    reset             = alloc_perm( sizeof(RESET_DATA) );
#else
    reset             = GC_MALLOC( sizeof(RESET_DATA) );
#endif
  }
  else
  {
    reset             = reset_free;
    reset_free        = reset_free->next;
  }
  
  *reset = z_reset;  
  return reset;  
}

void free_reset (RESET_DATA *reset)
{
  reset->next = reset_free;
  reset_free = reset;
}

AREA_DATA *new_area ()
{ 
  AREA_DATA *area;
  static AREA_DATA z_area;
  
#ifdef OLC_VERSION
  area = alloc_perm( sizeof(AREA_DATA) );
#else
  area = GC_MALLOC( sizeof(AREA_DATA) );
#endif
  
  *area = z_area;  
  return area;
}

LINE_EDIT_DATA *edit_free = NULL;

LINE_EDIT_DATA *new_edit ()
{
  LINE_EDIT_DATA *edit;
  
  if ( edit_free == NULL )
  {
#ifdef OLC_VERSION
    edit            = alloc_perm( sizeof (LINE_EDIT_DATA) );
#else
    edit            = GC_MALLOC( sizeof (LINE_EDIT_DATA) );
#endif
  }
  else
  {
    edit            = edit_free;
    edit_free       = edit_free->next;
  }
    
  return edit;
}

void free_edit (LINE_EDIT_DATA *edit)
{
  edit->next = edit_free;
  edit_free  = edit;
}

LINE_DATA *line_free = NULL;

LINE_DATA *new_line_data ()
{
  LINE_DATA *line;
    
  if ( line_free == NULL )
  {
#ifdef OLC_VERSION
    line            = alloc_perm( sizeof (LINE_DATA) );
#else
    line            = GC_MALLOC( sizeof (LINE_DATA) );
#endif
  }
  else
  {
    line            = line_free;
    line_free       = line_free->next;
  }
    
  return line;  
}
 
void free_line_data (LINE_DATA *line)
{
  line->next = line_free;
  line_free  = line;
}

MACRO_DATA *macro_free;

MACRO_DATA *new_macro ()
{
  MACRO_DATA *macro;
  static MACRO_DATA z_macro;
    
  if ( macro_free == NULL )
  {
#ifdef OLC_VERSION
    macro            = alloc_perm( sizeof (MACRO_DATA) );
#else
    macro            = GC_MALLOC( sizeof (MACRO_DATA) );
#endif
  }
  else
  {
    macro            = macro_free;
    macro_free       = macro_free->next;
  }
  
  *macro = z_macro;
  return macro;  
}


void free_macro (MACRO_DATA *macro)
{
  if (macro->name)
    free_string (macro->name);
  if (macro->text)
    free_string (macro->text);
    
  macro->name = NULL;
  macro->text = NULL;
  
  macro->next = macro_free;
  macro_free  = macro;
}

VNUM_RANGE_DATA *range_free;

VNUM_RANGE_DATA *new_range ()
{
  VNUM_RANGE_DATA *range;
  static VNUM_RANGE_DATA z_range;
    
  if ( range_free == NULL )
  {
#ifdef OLC_VERSION
    range            = alloc_perm( sizeof (VNUM_RANGE_DATA) );
#else
    range            = GC_MALLOC( sizeof (VNUM_RANGE_DATA) );
#endif
  }
  else
  {
    range            = range_free;
    range_free       = range_free->next;
  }
  
  *range = z_range;
  return range;
}

void free_range (VNUM_RANGE_DATA *range)
{
  range->min = 0;
  range->max = 0;
  
  range->next = range_free;
  range_free = range;
}

