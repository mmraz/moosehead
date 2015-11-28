/* "$Id: db.h,v 1.7 2001/12/08 02:08:53 poquah Exp $*/
/* files used in db.c */

typedef struct area_name_data AREA_NAME_DATA;
struct area_name_data {
  AREA_NAME_DATA *next;
  char *name;
};

/* vals from db.c */
extern bool fBootDb;
extern int    newmobs;
extern int    newobjs;
extern MOB_INDEX_DATA   * mob_index_hash          [MAX_KEY_HASH];
extern OBJ_INDEX_DATA   * obj_index_hash          [MAX_KEY_HASH];
extern ROOM_INDEX_DATA  * room_index_hash         [MAX_KEY_HASH];

extern int     top_affect;
extern int     top_area;
extern int     top_ed;
extern int     top_exit;
extern int     top_help;
extern int     top_recipe;
extern int     top_mob_index;
extern int     top_obj_index;
extern int     top_reset;
extern int     top_room;
extern int     top_shop;
extern int     mobile_count;
extern int     newmobs;
extern int     newobjs;
extern AREA_DATA  * area_first, * area_last;

extern AREA_NAME_DATA  *area_name_first,*area_name_last;

/* from db2.c */
extern int  social_count;

/* conversion from db.h */
void  convert_mob(MOB_INDEX_DATA *mob);
void  convert_obj(OBJ_INDEX_DATA *obj);

/* macro for flag swapping */
#define GET_UNSET(flag1,flag2)  (~(flag1)&((flag1)|(flag2)))
#define MAGIC_NUM 52571214

/* db3.c */
void do_collate(CHAR_DATA *ch, char *argument);
void save_area ( CHAR_DATA *ch, AREA_DATA *pArea );
void update_area_list ( CHAR_DATA *ch, char *strArea );
