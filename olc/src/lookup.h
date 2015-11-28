/* $Id: lookup.h,v 1.8 2000/02/08 02:11:42 mud Exp $ */
#define MAX_IMM_COMM 27
/* various lookup procedures that use table.h */

CLAN_DATA*	clan_lookup	args( (const char *name) );
/* moved to merc.h for comm.c use
int	deity_lookup	args( (const char *name) );
*/
int     nonclan_lookup  args( (const char *name) );
int	gift_lookup	args( (const char *name) );
int	position_lookup	args( (const char *name) );
int 	sex_lookup	args( (const char *name) );
int 	immc_lookup	args( (const char *name) );
int 	size_lookup	args( (const char *name) );
