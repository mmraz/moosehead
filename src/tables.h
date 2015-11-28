/* $Id: tables.h,v 1.10 2000/08/17 14:29:30 mud Exp $" */

struct flag_type
{
    char *name;
    int bit;
    bool settable;
};

struct imm_command_type
{
    char *name;
    sh_int icg;
    int bit;
};

struct clan_type
{
    char 	*name;
    char 	*who_name;
    sh_int 	hall;
    bool	hidden;      /* hidden clans */
    bool	independent; /* true for loners */
    bool	true_clan;   /* true for pk clans */
};

struct gift_type
{
    char	*name;
    sh_int	cost;
};

struct position_type
{
    char *name;
    char *short_name;
};

struct sex_type
{
    char *name;
};

struct size_type
{
    char *name;
};

struct obj_size_type
{
    char *name;
};

extern	const	struct	clan_type	clan_table[];
extern	const	struct	imm_command_type imm_command_table[];
extern	const	struct	gift_type	gift_table[];
extern	const	struct	position_type	position_table[];
extern	const	struct	sex_type	sex_table[];
extern 	const 	struct	size_type 	size_table[];
extern 	const 	struct	obj_size_type 	obj_size_table[];

/* flag tables */
extern  const   struct  flag_type       mhs_flags[];
extern	const	struct	flag_type	act_flags[];
extern	const	struct	flag_type	plr_flags[];
extern	const	struct	flag_type	affect_flags[];
extern	const	struct	flag_type	off_flags[];
extern	const	struct	flag_type	imm_flags[];
extern	const	struct	flag_type	form_flags[];
extern	const	struct	flag_type	part_flags[];
extern	const	struct	flag_type	comm_flags[];
extern	const	struct	flag_type	extra_flags[];
extern	const	struct	flag_type	wear_flags[];
extern	const	struct	flag_type	weapon_flags[];
extern	const	struct	flag_type	container_flags[];
extern	const	struct	flag_type	portal_flags[];
extern	const	struct	flag_type	room_flags[];
extern	const	struct	flag_type	exit_flags[];
extern  const   struct  flag_type       clan_flags[];

