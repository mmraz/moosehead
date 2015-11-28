/* $Id: recycle.h,v 1.6 1999/11/23 20:36:49 mud Exp $" */
/* externs */
extern char str_empty[1];
extern int mobile_count;

/* stuff for providing a crash-proof buffer */

#define MAX_BUF   16384
#define MAX_BUF_LIST  10
#define BASE_BUF  1024

/* valid states */
#define BUFFER_SAFE 0
#define BUFFER_OVERFLOW 1
#define BUFFER_FREED  2

/* note recycling */
#define ND NOTE_DATA
ND  *new_note args( (void) );
void  free_note args( (NOTE_DATA *note) );
#undef ND

/* DNS data recycling */
#define DD DNS_DATA
DD  *new_dns args( (void) );
void  free_dns args( (DNS_DATA *ban) );
#undef DD

/* ban data recycling */
#define BD BAN_DATA
BD  *new_ban args( (void) );
void  free_ban args( (BAN_DATA *ban) );
#undef BD

/* descriptor recycling */
#define DD DESCRIPTOR_DATA
DD  *new_descriptor args( (void) );
void  free_descriptor args( (DESCRIPTOR_DATA *d) );
#undef DD

/* char gen data recycling */
#define GD GEN_DATA
GD  *new_gen_data args( (void) );
void  free_gen_data args( (GEN_DATA * gen) );
#undef GD

/* extra descr recycling */
#define ED EXTRA_DESCR_DATA
ED  *new_extra_descr args( (void) );
void  free_extra_descr args( (EXTRA_DESCR_DATA *ed) );
#undef ED

/*clan recycling */
CLAN_DATA *new_clan args((void));
void free_clan args((CLAN_DATA *clan));

/*clan char recycling */
CLAN_CHAR *new_clan_char args((void));
void free_clan_char args((CLAN_CHAR *cchar));

/*ally recycling */
ALLIANCE_DATA *new_ally args((void));
void free_ally args((ALLIANCE_DATA *ally));

/* merit tracker recycling */
MERIT_TRACKER *new_merit args((void));
void free_merit args((MERIT_TRACKER *merit));

/* plan data recycling */
PLAN_DATA *new_plan args((void));
void free_plan args((PLAN_DATA *plan));

/* plan exit data recycling */
PLAN_EXIT_DATA *new_p_exit args((void));
void free_p_exit args((PLAN_EXIT_DATA *p_exit));

/* damage tracking recycling */
#define DAD DAMAGE_DATA
DAD  *new_damage args( (void) );
void  free_damage args( (DAMAGE_DATA *da) );
#undef DAD

/* affect recycling */
#define AD AFFECT_DATA
AD  *new_affect args( (void) );
void  free_affect args( (AFFECT_DATA *af) );
#undef AD

/* MudTrader */
#define TRAD TRADE_DATA
TRAD	*new_trade	args( (void) );
void	free_trade	args( (TRADE_DATA *trade) );
#undef TRAD

/* object recycling */
#define OD OBJ_DATA
OD  *new_obj args( (void) );
void  free_obj args( (OBJ_DATA *obj) );
#undef OD

/* character recyling */
#define CD CHAR_DATA
#define PD PC_DATA
CD  *new_char args( (void) );
void  free_char args( (CHAR_DATA *ch) );
PD  *new_pcdata args( (void) );
void  free_pcdata args( (PC_DATA *pcdata) );
#undef PD
#undef CD

/* misc recycles */
EXIT_DATA        *new_exit ();
void              free_exit (EXIT_DATA *exit);
MACRO_DATA       *new_macro ();
void              free_macro (MACRO_DATA *macro);
RESET_DATA       *new_reset ();
void              free_reset (RESET_DATA *reset);
AREA_DATA        *new_area ();
LINE_EDIT_DATA   *new_edit ();
void              free_edit (LINE_EDIT_DATA *edit);
LINE_DATA        *new_line_data ();
void              free_line_data (LINE_DATA *line);
VNUM_RANGE_DATA  *new_range ();
void              free_range (VNUM_RANGE_DATA *range);


/* mob id and memory procedures */
#define MD MEM_DATA
long  get_pc_id args( (void) );
long  get_mob_id args( (void) );
/* MD  *new_mem_data args( (void) );
void  free_mem_data args( ( MEM_DATA *memory) );
MD  *find_memory args( (MEM_DATA *memory, long id) );*/
#undef MD

/* buffer procedures */

BUFFER  *new_buf args( (void) );
BUFFER  *new_buf_size args( (int size) );
void  free_buf args( (BUFFER *buffer) );
bool  add_buf args( (BUFFER *buffer, char *string) );
void  clear_buf args( (BUFFER *buffer) );
char  *buf_string args( (BUFFER *buffer) );
