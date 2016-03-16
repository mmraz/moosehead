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

static char rcsid[] = "$Id: act_wiz.c,v 1.418 2004/10/25 02:48:44 boogums Exp $";
#if defined(macintosh)
#include <types.h>
#include <time.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include "merc.h"
#include "recycle.h"
#include "tables.h"
#include "lookup.h"

/* command procedures needed */
DECLARE_DO_FUN(do_rstat);
DECLARE_DO_FUN(do_mstat);
DECLARE_DO_FUN(do_ostat);
DECLARE_DO_FUN(do_rset);
DECLARE_DO_FUN(do_mset);
DECLARE_DO_FUN(do_oset);
DECLARE_DO_FUN(do_sset);
DECLARE_DO_FUN(do_mfind);
DECLARE_DO_FUN(do_ofind);
DECLARE_DO_FUN(do_slookup);
DECLARE_DO_FUN(do_mload);
DECLARE_DO_FUN(do_oload);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_quit);
DECLARE_DO_FUN(do_save);
DECLARE_DO_FUN(do_look);
DECLARE_DO_FUN(do_force);
DECLARE_DO_FUN(do_stand);
DECLARE_DO_FUN(do_disconnect);
DECLARE_DO_FUN(do_help);
DECLARE_DO_FUN(do_clantalk);

//bool override;
/*
 * Local functions.
 */
ROOM_INDEX_DATA * find_locationargs( CHAR_DATA *ch, char *arg );
bool check_parse_nameargs( char *name );
void remove_highlanderargs( CHAR_DATA *ch,CHAR_DATA *victim );
char * format_obj_to_charargs( OBJ_DATA *obj, CHAR_DATA *ch,
				bool fShort );

char * const wear_name[] = { "<{Wused as light{x>     ",
		"<{Wworn on finger{x>    ", "<{Wworn on finger{x>    ",
		"<{Wworn around neck{x>  ", "<{Wworn around neck{x>  ",
		"<{Wworn on torso{x>     ", "<{Wworn on head{x>      ",
		"<{Wworn on legs{x>      ", "<{Wworn on feet{x>      ",
		"<{Wworn on hands{x>     ", "<{Wworn on arms{x>      ",
		"<{Wworn as shield{x>    ", "<{Wworn about body{x>   ",
		"<{Wworn about waist{x>  ", "<{Wworn around wrist{x> ",
		"<{Wworn around wrist{x> ", "<{Wwielded{x>           ",
		"<{Wheld{x>              ", "<{Wfloating nearby{x>   ",
		"<{Wsecondary weapon{x>  " };

void do_changepassword(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	char arg[MAX_INPUT_LENGTH];
	char *pArg, *p;
	char cEnd;
	char buf[256];
	char *pwdnew;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax: changepassword <player> <new>.\n\r", ch);
		return;
	}

	if ((victim = get_char_online(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	/** Person is legitimately logged on... was not ploaded.
	 * Why are you changing the password of someone who already knows it?
	 */
	if (victim->desc != NULL) {
		send_to_char(
				"That player was not ploaded, leave their password alone.\n\r",
				ch);
		return;
	}

	pArg = arg;
	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	if (arg[0] == '\0') {
		send_to_char("Syntax: changepassword <player> <new>.\n\r", ch);
		return;
	}
	if (strlen(arg) < 5) {
		send_to_char("New password must be at least five characters long.\n\r",
				ch);
		return;
	}

	/*
	 * No tilde allowed because of player file format.
	 */
	pwdnew = crypt(arg, victim->name);
	for (p = pwdnew; *p != '\0'; p++) {
		if (*p == '~') {
			send_to_char("New password not acceptable, try again.\n\r", ch);
			return;
		}
	}

	sprintf(buf, "Log %s: %s changing password to %s", victim->name, ch->name,
			arg);
	log_string(buf);

	free_string(victim->pcdata->pwd);
	victim->pcdata->pwd = str_dup(pwdnew);
	save_char_obj(victim);

	sprintf(buf, "%s's password has been changed.\n\r", victim->name);
	send_to_char(buf, ch);
}

void do_ctalk(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	int clan;
	CLAN_DATA *newclan = NULL;

	if (IS_NPC(ch))
		return; /* switched imms can't ctalk now, sorry switched imms */

	argument = one_argument(argument, arg1);
	strcpy(arg2, argument);
	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax: ctalk <clan> <message>\n\r", ch);
		return;
	}

	if ((newclan = clan_lookup(arg1)) != NULL && newclan->default_clan) {
		send_to_char("I doubt anyone would be listening.\n\r", ch);
		return;
	}

	if (!newclan) {
		if ((clan = nonclan_lookup(arg1)) == 0) {
			send_to_char("No such clan exists.\n\r", ch);
			return;
		}

		if (clan_table[clan].hidden && get_trust(ch) < 58) {
			send_to_char("No such clan exists.\n\r", ch);
			return;
		}

		if (clan_table[clan].independent && clan_table[clan].true_clan) {
			send_to_char("I doubt anyone would be listening.\n\r", ch);
			return;
		}
	}
	/* Ok, if they've gotten this far, they're talking on a real channel */

	/* If they have listen off, turn it on */

	if (!IS_SET(ch->mhs, MHS_LISTEN)) {
		SET_BIT(ch->mhs, MHS_LISTEN);
		send_to_char("LISTEN flagged {CON{x, so you hear responses.\n\r", ch);
	}

	if (newclan) {/* Dirty trick to get them onto the channel */
		sprintf(buf, "<%s>:", newclan->name);
		send_to_char(buf, ch);

		CLAN_CHAR *old_cchar = ch->pcdata->clan_info;
		ch->pcdata->clan_info = newclan->members;
		do_clantalk(ch, arg2);
		ch->pcdata->clan_info = old_cchar;
	} else {
		sprintf(buf, "[%s]:", capitalize(clan_table[clan].name));
		send_to_char(buf, ch);

		ch->clan = clan;
		do_clantalk(ch, arg2);
		ch->clan = 0;
	}
	return;
}

void do_listen(CHAR_DATA *ch, char *argument) {
	if (IS_SET(ch->mhs, MHS_LISTEN)) {
		REMOVE_BIT(ch->mhs, MHS_LISTEN);
		send_to_char("LISTEN flagged {ROFF{x.\n\r", ch);
	} else {
		SET_BIT(ch->mhs, MHS_LISTEN);
		send_to_char("LISTEN flagged {CON{x.\n\r", ch);
	}
	return;
}

#define RANGE_UNLIMITED 1
void do_reward(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	char arg2[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	int xp, range = 0;

	argument = one_argument(argument, arg);
	one_argument(argument, arg2);

	if (arg[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax: reward <char> <amount of XP>\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all") && (get_trust(ch) >= 58 || override != 0)) {
		DESCRIPTOR_DATA *d;

		if (!is_number(arg2)) {
			send_to_char("XP value must be numeric.\n\r", ch);
			return;
		}

		if (atoi(arg2) > 10000) {
			send_to_char(
					"I'm sorry Captain, I do not think that would be a good idea\n\r",
					ch);
			return;
		}

		for (d = descriptor_list; d; d = d->next) {
			if ((victim = d->character) == NULL || d->connected != CON_PLAYING)
				continue;

			if (d->character->level <= 50) {
				gain_exp(victim, atoi(arg2));
				act("You have been rewarded by the gods, curtesy of $n today.",
						ch, NULL, victim, TO_VICT, FALSE);
			}
		}
		send_to_char("All Characters rewarded.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("Cannot locate that person.\n\r", ch);
		return;
	}

	if (!is_number(arg2)) {
		send_to_char("XP value must be numeric.\n\r", ch);
		return;
	}

	xp = atoi(arg2);

	/* base range of reward on IMM's level */
	switch (get_trust(ch)) {
	case 52:
	case 53:
	case 54:
		range = 250;
		break;
	case 55:
	case 56:
		range = 500;
		break;
	case 57:
		range = 1000;
		break;
	case 58:
		range = 2500;
		break;
	case 59:
	case 60:
		range = RANGE_UNLIMITED;
		break;
	default:
		return;
	}

	if (range != RANGE_UNLIMITED && (xp > range || xp < (-1 * range))) {
		sprintf(arg, "Your range is limited to +/- %d.\n\r", range);
		send_to_char(arg, ch);
		return;
	}

	gain_exp(victim, atoi(arg2));
	send_to_char("Character rewarded.\n\r", ch);
	send_to_char("You have been rewarded by the gods.\n\r", victim);
	return;
}

void do_spreward(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	char arg2[MAX_STRING_LENGTH];
	char arg[MAX_STRING_LENGTH];
	int sp, range = 0;

	argument = one_argument(argument, arg);
	one_argument(argument, arg2);

	if (arg[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax: spreward <char> <amount of Skill Points>\n\r",
				ch);
		return;
	}

	if (!str_cmp(arg, "all") && (get_trust(ch) >= 58 || override != 0)) {
		DESCRIPTOR_DATA *d;

		if (!is_number(arg2)) {
			send_to_char("SP value must be numeric.\n\r", ch);
			return;
		}

		if (atoi(arg2) > 1000) {
			send_to_char(
					"I'm sorry Captain, I do not think that would be a good idea\n\r",
					ch);
			return;
		}

		for (d = descriptor_list; d; d = d->next) {
			if ((victim = d->character) == NULL || d->connected != CON_PLAYING)
				continue;

			victim->skill_points += atoi(arg2);
			act(
					"The gods have favored you with more skill, curtesy of $n today.",
					ch, NULL, victim, TO_VICT, FALSE);
		}
		send_to_char("All Characters SP rewarded.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("Cannot locate that person.\n\r", ch);
		return;
	}

	if (!is_number(arg2)) {
		send_to_char("SP value must be numeric.\n\r", ch);
		return;
	}

	sp = atoi(arg2);

	/* base range of reward on IMM's level */
	switch (get_trust(ch)) {
	case 52:
	case 53:
	case 54:
		range = 25;
		break;
	case 55:
	case 56:
		range = 50;
		break;
	case 57:
		range = 100;
		break;
	case 58:
		range = 250;
		break;
	case 59:
	case 60:
		range = RANGE_UNLIMITED;
		break;
	default:
		return;
	}

	if (range != RANGE_UNLIMITED && (sp > range || sp < (-1 * range))) {
		sprintf(arg, "Your range is limited to +/- %d.\n\r", range);
		send_to_char(arg, ch);
		return;
	}

	victim->skill_points += atoi(arg2);
	send_to_char("Character SP rewarded.\n\r", ch);
	send_to_char("The gods have favored you with more skill.\n\r", victim);
	return;
}
#undef RANGE_UNLIMITED

void do_pload(CHAR_DATA *ch, char *argument) {
	DESCRIPTOR_DATA d;
	bool isChar = FALSE;
	char name[MAX_INPUT_LENGTH];

	if (argument[0] == '\0') {
		send_to_char("Load who?\n\r", ch);
		return;
	}

	argument[0] = UPPER(argument[0]);
	argument = one_argument(argument, name);

	/* Don't want to load a second copy of a player who's already online! */
	if (get_char_world(ch, name) != NULL) {
		send_to_char("That person is already connected!\n\r", ch);
		return;
	}

	isChar = load_char_obj(&d, name); /* char pfile exists? */

	if (!isChar) {
		send_to_char("No such pfile (be sure to use the full name).\n\r", ch);
		return;
	}

	d.character->desc = NULL;
	d.character->next = char_list;
	char_list = d.character;
	d.connected = CON_PLAYING;
	reset_char(d.character);

	if (d.character->level >= ch->level) {
		send_to_char("No ploading imms higher level than you.\n\r", ch);
		do_quit(d.character, "none");
		return;
	}

	/* bring player to imm */
	if (d.character->in_room != NULL) {
		if (d.character->was_in_room == NULL)
			d.character->was_in_room = d.character->in_room;

		char_from_room(d.character);
		char_to_room(d.character, ch->in_room);
	}

	act("You pull $N from the void.", ch, NULL, d.character, TO_CHAR, TRUE);

	act("$n pulls $N from the void.", ch, NULL, d.character, TO_ROOM, FALSE);

	if (d.character->pet != NULL) {
		char_to_room(d.character->pet, d.character->in_room);
		act("$n has entered the game.", d.character->pet, NULL, NULL, TO_ROOM,
				FALSE);
	}

} /* end do_pload */

void do_punload(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;
	char who[MAX_INPUT_LENGTH];

	argument = one_argument(argument, who);

	if ((victim = get_char_world(ch, who)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	/** Person is legitimatly logged on... was not ploaded.
	 * Can also be used to log out link-dead chars
	 */
	if (victim->desc != NULL) {
		send_to_char("That player was not pulled.\n\r", ch);
		return;
	}

	if (victim->was_in_room != NULL) /* return player and pet to orig room */
	{
		char_from_room(victim);
		char_to_room(victim, victim->was_in_room);

		if (victim->pet != NULL) {
			char_from_room(victim->pet);
			char_to_room(victim->pet, victim->was_in_room);
		}

		ch->was_in_room = NULL;
	}

	act("You release $N to the void.", ch, NULL, victim, TO_CHAR, TRUE);
	act("$n releases $N to the void.", ch, NULL, victim, TO_ROOM, FALSE);

	do_quit(victim, "none");
}

void do_pnet(CHAR_DATA *ch, char *argument) {
	int flag;
	char buf[MAX_STRING_LENGTH];

	if (argument[0] == '\0') {
		if (IS_SET(ch->pnet, PNET_ON)) {
			send_to_char("Signing off of Player-net.\n\r", ch);
			REMOVE_BIT(ch->pnet, PNET_ON);
		} else {
			send_to_char("Welcome to Player-net!\n\r", ch);
			SET_BIT(ch->pnet, PNET_ON);
		}
		return;
	}

	if (!str_prefix(argument, "on")) {
		send_to_char("Welcome to Player-net!\n\r", ch);
		SET_BIT(ch->pnet, PNET_ON);
		return;
	}

	if (!str_prefix(argument, "off")) {
		send_to_char("Signing off of Player-net.\n\r", ch);
		REMOVE_BIT(ch->pnet, PNET_ON);
		return;
	}

	/* show pnet status */
	if (!str_prefix(argument, "status") || !str_prefix(argument, "show")) {
		bool odd = FALSE;
		buf[0] = '\0';

		sprintf(buf, "Player-net:  %s\n\r",
				IS_SET(ch->pnet,PNET_ON) ? "ON " : "OFF");
		send_to_char(buf, ch);

		for (flag = 1; pnet_table[flag].name != NULL; flag++) {
			if (pnet_table[flag].level <= get_trust(ch)) {
				sprintf(buf, "  %-10s - %s%s",
						capitalize(pnet_table[flag].name),
						IS_SET(ch->pnet,pnet_table[flag].flag) ? "ON " : "OFF",
						odd ? "\n\r" : "");
				odd = !odd;
				send_to_char(buf, ch);
			}
		}
		send_to_char("\n\r", ch);
		return;
	}

	flag = pnet_lookup(argument);

	if (flag == -1 || get_trust(ch) < pnet_table[flag].level) {
		send_to_char("No such option.\n\r", ch);
		return;
	}

	if (IS_SET(ch->pnet, pnet_table[flag].flag)) {
		sprintf(buf, "You will no longer see %s on Player-net.\n\r",
				pnet_table[flag].name);
		send_to_char(buf, ch);
		REMOVE_BIT(ch->pnet, pnet_table[flag].flag);
		return;
	} else {
		sprintf(buf, "You will now see %s on Player-net.\n\r",
				pnet_table[flag].name);
		send_to_char(buf, ch);
		SET_BIT(ch->pnet, pnet_table[flag].flag);
		return;
	}

}

void do_wiznet(CHAR_DATA *ch, char *argument) {
	int flag;
	char buf[MAX_STRING_LENGTH];

	if (argument[0] == '\0') {
		if (IS_SET(ch->wiznet, WIZ_ON)) {
			send_to_char("Signing off of Wiznet.\n\r", ch);
			REMOVE_BIT(ch->wiznet, WIZ_ON);
		} else {
			send_to_char("Welcome to Wiznet!\n\r", ch);
			SET_BIT(ch->wiznet, WIZ_ON);
		}
		return;
	}

	if (!str_prefix(argument, "on")) {
		send_to_char("Welcome to Wiznet!\n\r", ch);
		SET_BIT(ch->wiznet, WIZ_ON);
		return;
	}

	if (!str_prefix(argument, "off")) {
		send_to_char("Signing off of Wiznet.\n\r", ch);
		REMOVE_BIT(ch->wiznet, WIZ_ON);
		return;
	}

	/* show wiznet status */
	if (!str_prefix(argument, "status") || !str_prefix(argument, "show")) {
		bool odd = FALSE;
		buf[0] = '\0';

		sprintf(buf, "Wiznet:  %s\n\r",
				IS_SET(ch->wiznet,WIZ_ON) ? "ON " : "OFF");
		send_to_char(buf, ch);

		for (flag = 1; wiznet_table[flag].name != NULL; flag++) {
			if (wiznet_table[flag].level <= get_trust(ch)) {
				sprintf(buf, "  %-10s - %s%s",
						capitalize(wiznet_table[flag].name),
						IS_SET(ch->wiznet,wiznet_table[flag].flag) ?
								"ON " : "OFF", odd ? "\n\r" : "");
				odd = !odd;
				send_to_char(buf, ch);
			}
		}
		send_to_char("\n\r", ch);
		return;
	}

	flag = wiznet_lookup(argument);

	if (flag == -1 || get_trust(ch) < wiznet_table[flag].level) {
		send_to_char("No such option.\n\r", ch);
		return;
	}

	if (IS_SET(ch->wiznet, wiznet_table[flag].flag)) {
		sprintf(buf, "You will no longer see %s on wiznet.\n\r",
				wiznet_table[flag].name);
		send_to_char(buf, ch);
		REMOVE_BIT(ch->wiznet, wiznet_table[flag].flag);
		return;
	} else {
		sprintf(buf, "You will now see %s on wiznet.\n\r",
				wiznet_table[flag].name);
		send_to_char(buf, ch);
		SET_BIT(ch->wiznet, wiznet_table[flag].flag);
		return;
	}

}

void wiznet(char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag,
		long flag_skip, int min_level) {
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d != NULL) {
			if ((d->connected == CON_PLAYING) && (IS_IMMORTAL(d->character))
					&& (IS_SET(d->character->wiznet, WIZ_ON))
					&& (!flag || IS_SET(d->character->wiznet, flag))
					&& (!flag_skip || !IS_SET(d->character->wiznet, flag_skip))
					&& (get_trust(d->character) >= min_level)
					&& (d->character != ch)) {
				send_timestamp(d->character, TRUE, TRUE);
				if (IS_SET(d->character->wiznet, WIZ_PREFIX)) {
					send_to_char("{G--> {x", d->character);
				}
				act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD,
						FALSE);
			}
		} else {
			if (d->character->name != NULL)
				sprintf(buf, "%s had a NULL descriptor", d->character->name);
			log_string(buf);
		}
	}

	return;
}

void pnet(char *string, CHAR_DATA *ch, OBJ_DATA *obj, long flag, long flag_skip,
		int min_level) {
	DESCRIPTOR_DATA *d;
	char buf[MAX_STRING_LENGTH];

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d != NULL) {
			if ((d->connected == CON_PLAYING)
					&& (IS_SET(d->character->pnet, PNET_ON))
					&& (!flag || IS_SET(d->character->pnet, flag))
					&& (!flag_skip || !IS_SET(d->character->pnet, flag_skip))
					&& (get_trust(d->character) >= min_level)
					&& (d->character != ch)) {
				if (IS_SET(d->character->pnet, PNET_PREFIX)) {
					send_to_char("{C--> {x", d->character);
				}
				act_new(string, d->character, obj, ch, TO_CHAR, POS_DEAD, TRUE);
			}
		} else {
			if (d->character->name != NULL)
				sprintf(buf, "%s had a NULL descriptor", d->character->name);
			log_string(buf);
		}
	}

	return;
}

#ifdef OLC_VERSION
void do_matlog( CHAR_DATA *ch, char *argument )
{
	char buf[MAX_STRING_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_obj_index is fast, and I don't feel like threading another link.
	 */
	for ( vnum = 0; vnum < 33000; vnum++ )
	{
		if ( ( pObjIndex = get_obj_index( vnum ) ) != NULL )
		{
			sprintf( buf, "%d %s", pObjIndex->vnum, pObjIndex->material );
			log_string(buf);
		}
	}
	send_to_char("Done sending all material types to the logfile.\n\r",ch);
	return;
}
#endif

void do_name(CHAR_DATA *ch, char *argument) {
	char arg[MAX_STRING_LENGTH];

	if (IS_NPC(ch)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Syntax:  name <who_name>\n\r", ch);
		return;
	}
	if (strlen(argument) > 13) {
		send_to_char("Name cannot be longer than 13 characters.\n\r", ch);
		return;
	}
	if (ch->pcdata->who_name)
		free_string(ch->pcdata->who_name);
	ch->pcdata->who_name = str_dup(argument);
	send_to_char("Ok.\n\r", ch);
}

void do_dweeb(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);

	if (arg1[0] == '\0') {
		send_to_char("Syntax: dweeb <player>\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't playing.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (victim->level >= ch->level && victim != ch) {
		send_to_char("You can't DWEEB someone your level or higher.\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_DWEEB)) {
		send_to_char("They are no longer a DWEEB.\n\r", ch);
		send_to_char("You are no longer a DWEEB.\n\r", victim);
		REMOVE_BIT(victim->act, PLR_DWEEB);
		save_char_obj(victim);
		return;
	}

	else {
		SET_BIT(victim->act, PLR_DWEEB);
		send_to_char("They have been DWEEB'ed.\n\r", ch);
		send_to_char("You are now an official DWEEB!\n\r", victim);
		save_char_obj(victim);
	}
	return;
}

/*void do_rank( CHAR_DATA *ch, char *argument )
 {
 char arg1[MAX_INPUT_LENGTH],arg2[MAX_INPUT_LENGTH];
 CHAR_DATA *victim;
 int num;

 if (ch->pcdata->rank < MAX_RANK && !IS_IMMORTAL(ch))
 {
 send_to_char("Not until you are of the highest rank.\n\r",ch);
 return;
 }

 argument = one_argument( argument, arg1 );
 argument = one_argument( argument, arg2 );
 if ( arg1[0] == '\0' || arg2[0] == '\0' )
 {
 send_to_char( "Syntax: rank <char> <number>\n\r",ch);
 return;
 }
 if ( ( victim = get_char_online( ch, arg1 ) ) == NULL )
 {
 send_to_char( "They aren't playing.\n\r", ch );
 return;
 }

 if ( !is_same_clan(ch,victim) && !IS_IMMORTAL(ch) ) return;

 if ( !is_number(arg2) )
 {
 send_to_char("Ranks need to be numbers.\n\r",ch);
 return;
 }
 else
 {
 num = atoi(arg2);
 if ( num < 0 || num > MAX_RANK )
 {
 send_to_char("That's not a valid rank.\n\r",ch);
 return;
 }
 }

 if ( victim->pcdata->rank == MAX_RANK && !IS_IMMORTAL(ch) )
 {
 send_to_char("Only the immortals can demote a leader in rank.\n\r",ch);
 return;
 }

 if ( IS_SET(victim->pcdata->clan_flags, CLAN_ALLOW_SANC) && num < 4 )
 {
 REMOVE_BIT(victim->pcdata->clan_flags, CLAN_ALLOW_SANC);
 }

 victim->pcdata->rank = num;
 return;
 }*/

void do_join(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CLAN_DATA *clan;
	CHAR_DATA *leader;
	bool leader_found = FALSE;

	if (IS_NPC(ch))
		return;/* NPCs may not join clans */

	if (IS_SET(ch->mhs, MHS_GLADIATOR)) {
		send_to_char("Gladiators can't join guilds.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax: join <clan name>\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "Matook")) {
		if (is_clan(ch)) {
			send_to_char("You may not join Matook from a clan.\n\r", ch);
			return;
		}
		ch->pcdata->old_join = nonclan_lookup(arg);
		if (ch->pcdata->old_join)
			send_to_char("You are awaiting entrance into Matook.\n\r", ch);
		else
			send_to_char("Sorry, that clan was not found.\n\r", ch);
		return;
	}

	if (ch->pcdata->clan_info && !ch->pcdata->clan_info->clan->default_clan) {
		send_to_char("You can't join a clan while in another.\n\r", ch);
		return;
	}

	/* Level check on join here */

	clan = clan_lookup(arg);
	if (clan == NULL) {
		send_to_char("No such clan exists.\n\r", ch);
		return;
	}

	if (ch->pcdata->clan_info && ch->pcdata->clan_info->merit > 0)
		sprintf(buf,
				"%s wishes to join your clan and is bringing %d merit.\n\r",
				ch->name, ch->pcdata->clan_info->merit);
	else
		sprintf(buf, "%s wishes to join your clan.\n\r", ch->name);
	leader_found = notify_clan_leaders(buf, clan, FALSE);

	if (!leader_found) {
		sprintf(buf,
				"There is nobody available to recruit you into %s currently.\n\r",
				clan->name);
		send_to_char(buf, ch);
		return;
	}
	ch->join = clan;
	sprintf(buf, "You are now awaiting entrance into %s.\n\r", clan->name);
	send_to_char(buf, ch);
}

void do_loner(CHAR_DATA *ch, char *argument) {
	CLAN_DATA *loner;
	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch)) {
		send_to_char("Use guild.\n\r", ch);
		return;
	}

	if (IS_SET(ch->mhs, MHS_GLADIATOR)) {
		send_to_char("Gladiators can't loner.\n\r", ch);
		return;
	}

	if (ch->pcdata->clan_info
			&& (!ch->pcdata->clan_info->clan->default_clan
					|| !str_cmp(ch->pcdata->clan_info->clan->name, "loner"))) {
		send_to_char("You can not loner from your current clan.\n\r", ch);
		return;
	}

	if (!is_clan(ch) && (ch->level < 5 || ch->level > 20)) {
		if (ch->level < 5)
			send_to_char("You must be at least level 5 to loner.\n\r", ch);
		else
			send_to_char("You are too high level to loner.\n\r", ch);
		return;
	}

	if (argument[0] != 0) {
		if (ch->pcdata->confirm_loner) {
			ch->pcdata->confirm_loner = FALSE;
			send_to_char("You are no longer considering lonering.\n\r", ch);
		} else
			send_to_char("Use loner with no arguments to loner yourself.\n\r",
					ch);
		return;
	}

	for (loner = clan_first; loner != NULL; loner = loner->next) {
		if (loner->default_clan && !str_cmp(loner->name, "loner"))
			break;
	}
	if (!loner) {/* There must be a loner clan. */
		bug("No loner clan created for loner.", 0);
		loner = new_clan();
		loner->default_clan = TRUE;
		loner->name = str_dup("Loner");
		if (clan_first && !str_cmp(clan_first->name, "outcast")) {/* Loner goes after Outcast */
			loner->next = clan_first->next;
			clan_first->next = loner;
		} else {
			loner->next = clan_first;
			clan_first = loner;
		}
	}

	if (ch->pcdata->clan_info
			&& ch->pcdata->clan_info->clan->default_clan == CLAN_OUTCAST) {
		if (ch->pcdata->confirm_loner) {
			ch->pcdata->confirm_loner = FALSE;
			send_to_char("You are now a loner.\n\r", ch);
			if (!ch->pcdata->clan_info->award_merit) {
				if (ch->pcdata->clan_info->merit < 50000)
					bug("Outcast is lonering without enough merit.", 0);
				ch->pcdata->clan_info->merit = UMAX(0,
						ch->pcdata->clan_info->merit - 50000);
				send_to_char(
						"50000 merit has been charged for the conversion from outcast.\n\r",
						ch);
			}
			add_clan_member(loner, ch, 0);
		} else {
			/* TEST CODE HERE - Remember to re-enable cost */
			if (ch->pcdata->clan_info->award_merit) {
				send_to_char(
						"Type loner again to loner yourself. This is free due to starter tribute.\n\r",
						ch);
				ch->pcdata->confirm_loner = TRUE;
				wiznet("$N is contemplating loner.", ch, NULL, 0, 0,
						get_trust(ch));
				return;
			} else {
				if (ch->pcdata->clan_info->merit >= 50000) {
//        send_to_char("Type loner again to loner yourself. This will cost 50000 tribute.\n\r", ch);
					send_to_char(
							"Type loner again to loner yourself. This will cost 50000 merit.\n\r",
							ch);
					ch->pcdata->confirm_loner = TRUE;
					wiznet("$N is contemplating loner.", ch, NULL, 0, 0,
							get_trust(ch));
					return;
				} else {
					char buf[256];
					sprintf(buf,
							"You need 50000 merit to loner from outcast, you only have %d.\n\r",
							ch->pcdata->clan_info->merit);
					send_to_char(buf, ch);
				}
			}
		}
		return;
	}

	if (ch->pcdata->confirm_loner) {
		send_to_char("You are now a loner.\n\r", ch);
		if (!is_clan(ch))/* Not a conversion from an old clan */
		{
			send_to_char("{WYou may attack other players in 3 ticks.{x\n\r",
					ch);
			ch->pcdata->start_time = 3;
		}
		add_clan_member(loner, ch, 0);
		return;
	}
	send_to_char("Type loner again to confirm this command.\n\r", ch);
	send_to_char("WARNING: this command is irreversible.\n\r", ch);
	send_to_char("Typing loner with an argument will undo loner status.\n\r",
			ch);
	ch->pcdata->confirm_loner = TRUE;
	wiznet("$N is contemplating loner.", ch, NULL, 0, 0, get_trust(ch));
}

void do_lone(CHAR_DATA *ch, char *argument) {
	send_to_char("You must type the full command to loner yourself.\n\r", ch);
	return;
}

void do_outcas(CHAR_DATA *ch, char *argument) {
	send_to_char("You must type the full command to outcast yourself.\n\r", ch);
	return;
}

void do_outcast(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	CLAN_CHAR *target;
	CLAN_DATA *outcast;

	if (IS_SET(ch->mhs, MHS_GLADIATOR)) {
		send_to_char("Gladiators can't outcast.\n\r", ch);
		return;
	}

	for (outcast = clan_first; outcast != NULL; outcast = outcast->next) {
		if (outcast->default_clan == CLAN_OUTCAST)
			break;
	}
	if (!outcast) {/* There must be an outcast clan. */
		bug("No outcast clan created for remove.", 0);
		outcast = new_clan();
		outcast->default_clan = CLAN_OUTCAST;
		outcast->name = str_dup("Outcast");
		outcast->next = clan_first;
		clan_first = outcast;
	}

	if (IS_NPC(ch))
		return;

	if (IS_IMMORTAL(ch)) {
		send_to_char("Immortals may not use outcast, use guild instead.\n\r",
				ch);
		return;
	} else if (ch->pcdata->clan_info
			&& ch->pcdata->clan_info->clan->default_clan) {
		send_to_char("You are not in a clan you can outcast from.\n\r", ch);
		return;
	}

	if (argument[0] != '\0') {
		if (ch->pcdata->confirm_outcast) {
			ch->pcdata->confirm_outcast = FALSE;
			send_to_char("Outcasting cleared.\n\r", ch);
			return;
		}
		if (!ch->pcdata->clan_info
				|| ch->pcdata->clan_info->clan->default_clan) {
			send_to_char(
					"You must be a leader in a clan to use outcast on someone.\n\r",
					ch);
			return;
		}
		if (ch->pcdata->clan_info->rank != 5) {
			send_to_char(
					"Use outcast with no arguments to outcast yourself.\n\r",
					ch);
			return;
		}
		for (target = ch->pcdata->clan_info->clan->members; target != NULL;
				target = target->next) {
			if (!str_cmp(target->name, argument))
				break;
		}
		if (!target) {
			send_to_char("They are not a member of your clan.\n\r", ch);
			return;
		} else {
			CHAR_DATA *victim = get_char_online(ch, target->name);
			if (victim == NULL) {
				send_to_char("They must be online to outcast.\n\r", ch);
				return;
			}
			/* Notify victim of who outcast them, set their rebellion timer */
			/* Have to clear their tribute somehow */
			ch->pcdata->confirm_outcast = FALSE;
			char buf[256];
			sprintf(buf, "You have been outcast by %s.\n\r", ch->name);
			notify_clan_char(buf, target, TRUE);/* Tell them even if they're offline */
			sprintf(buf, "%s has been outcast.\n\r", target->name);
			send_to_char(buf, ch);
			add_clan_member(outcast, victim, 0);
			target->banked_merit += target->merit;
			target->merit = 0;
		}
		return;
	}

	if (ch->pcdata->clan_info) {
		if (ch->pcdata->confirm_outcast) {
			ch->pcdata->confirm_outcast = FALSE;
			ch->pcdata->clan_info->banked_merit += ch->pcdata->clan_info->merit;
			ch->pcdata->clan_info->merit = 0;
			send_to_char("You are now an outcast.\n\r", ch);
			add_clan_member(outcast, ch, 0);
		} else {
			send_to_char(
					"Enter OUTCAST again with no arguments to outcast yourself.\n\r",
					ch);
			if (ch->pcdata->clan_info->clan->members->next != NULL)
				send_to_char(
						"You will not be allowed to rebel after outcasting yourself.\n\r",
						ch);
			else
				send_to_char(
						"{RYou are the last member, the clan will disband if you outcast.{x\n\r",
						ch);
			ch->pcdata->confirm_outcast = TRUE;
		}
	} else if (is_clan(ch) && !ch->pcdata->clan_info) {/* Outcast into loner */
		if (ch->pcdata->confirm_outcast) {
			ch->pcdata->confirm_outcast = FALSE;
			CLAN_DATA *loner = clan_first;
			while (loner) {
				if (loner->default_clan == CLAN_LONER)
					break;
				loner = loner->next;
			}
			if (loner) {
				send_to_char(
						"Welcome to being a loner!  See 'help clan' for details on the new clan world.",
						ch);
				add_clan_member(loner, ch, 0);
			} else {
				send_to_char(
						"Sorry, loner clan not found, please try again later.\n\r",
						ch);
				bug("Loner clan not found on outcast from old system.", 0);
			}
		} else {
			send_to_char(
					"You will become a loner in the new system if you type outcast again.\n\r",
					ch);
			ch->pcdata->confirm_outcast = TRUE;
		}
	} else
		send_to_char("You must be in a clan to use outcast.\n\r", ch);

	return;
	if (ch->clan == nonclan_lookup("smurf")) {
		send_to_char("You are a smurf, always a smurf.\n\r", ch);
		return;
	}

	if ((argument[0] != '\0'
			&& (ch->pcdata->rank != 5 || !is_clan(ch)
					|| clan_table[ch->clan].independent)) && !IS_IMMORTAL(ch)) {
		if (ch->pcdata->confirm_outcast == TRUE) {
			ch->pcdata->confirm_outcast = FALSE;
			send_to_char("Outcast status cleared.\n\r", ch);
			return;
		}
		send_to_char("You must be a rank 5 Clan leader to Outcast others.\n\r",
				ch);
		send_to_char(
				"Just type 'outcast' with no argument to Outcast yourself.\n\r",
				ch);
		return;
	}

	if ((argument[0] != '\0' && is_clan(ch) && !clan_table[ch->clan].independent
			&& ch->pcdata->rank == 5) && !IS_IMMORTAL(ch)) {
		one_argument(argument, arg);
		victim = get_char_online(ch, arg);
		if (victim != NULL && is_same_clan(ch, victim)
				&& victim->pcdata->rank < 5) {
			/*        if (victim->clan == clan_lookup("avarice"))
			 victim->pcdata->learned[skill_lookup("cure vision")] = 0;
			 if (victim->clan == clan_lookup("demise"))
			 victim->pcdata->learned[skill_lookup("confusion")] = 0;
			 if (victim->clan == clan_lookup("demise"))
			 victim->pcdata->learned[skill_lookup("aura of cthon")] = 0;
			 if (victim->clan == clan_lookup("posse"))
			 victim->pcdata->learned[skill_lookup("cuffs of justice")] = 0;
			 if (victim->clan == clan_lookup("zealot"))
			 {
			 victim->pcdata->learned[skill_lookup("annointment")] = 0;
			 victim->pcdata->deity = deity_lookup("mojo");
			 }
			 if (victim->clan == clan_lookup("honor"))
			 victim->pcdata->learned[skill_lookup("honor guard")] = 0;
			 */

			victim->clan = nonclan_lookup("outcast");
			victim->pcdata->rank = 0;
			victim->pcdata->outcT = 2700;
			victim->pcdata->node = 0;
			if (IS_SET(victim->pcdata->clan_flags, CLAN_ALLOW_SANC)) {
				REMOVE_BIT(victim->pcdata->clan_flags, CLAN_ALLOW_SANC);
			}
			if (victim->in_room->clan != NULL
					&& !clan_table[victim->in_room->clan].independent) {
				char_from_room(victim);
				char_to_room(victim, get_room_index(ROOM_VNUM_MATOOK));
				clear_mount(ch);
				do_look(victim, "auto");
			}
			send_to_char("You are now an Outcast.\n\r", victim);
			act("$N is now an Outcast.", ch, NULL, victim, TO_CHAR, TRUE);
			return;
		} else {
			send_to_char("Attempt to Outcast failed.\n\r", ch);
			return;
		}
	}

	if (argument[0] == '\0' && is_clan(ch) && !clan_table[ch->clan].independent
			&& ch->pcdata->confirm_outcast) {
		send_to_char("You are now an Outcast.\n\r", ch);
		/*        if (ch->clan == clan_lookup("avarice"))
		 ch->pcdata->learned[skill_lookup("cure vision")] = 0;
		 if (ch->clan == clan_lookup("demise"))
		 ch->pcdata->learned[skill_lookup("confusion")] = 0;
		 if (ch->clan == clan_lookup("demise"))
		 ch->pcdata->learned[skill_lookup("aura of cthon")] = 0;
		 if (ch->clan == clan_lookup("zealot"))
		 {
		 ch->pcdata->learned[skill_lookup("annointment")] = 0;
		 ch->pcdata->deity = deity_lookup("mojo");
		 }
		 if (ch->clan == clan_lookup("posse"))
		 ch->pcdata->learned[skill_lookup("cuffs of justice")] = 0;

		 if (ch->clan == clan_lookup("honor"))
		 ch->pcdata->learned[skill_lookup("honor guard")] = 0;
		 */
		ch->clan = nonclan_lookup("outcast");
		ch->pcdata->rank = 0;
		ch->pcdata->outcT = 900;
		ch->pcdata->node = 0;
		if (IS_SET(ch->pcdata->clan_flags, CLAN_ALLOW_SANC)) {
			REMOVE_BIT(ch->pcdata->clan_flags, CLAN_ALLOW_SANC);
		}
		if (!ch->in_room->clan
				&& !clan_table[ch->in_room->clan].independent) {
			char_from_room(ch);
			char_to_room(ch, get_room_index(ROOM_VNUM_MATOOK));
			clear_mount(ch);
			do_look(ch, "auto");
		}

		return;
	} else {
		if (ch->pcdata->confirm_outcast == FALSE) {
			send_to_char("Type outcast again to confirm this command.\n\r", ch);
			send_to_char("WARNING: this command is irreversible.\n\r", ch);
			send_to_char(
					"Typing outcast with an argument will undo outcast status.\n\r",
					ch);
			ch->pcdata->confirm_outcast = TRUE;
			wiznet("$N is contemplating outcast.", ch, NULL, 0, 0,
					get_trust(ch));
		} else {
			ch->pcdata->confirm_outcast = FALSE;
			send_to_char("Outcast status removed.\n\r", ch);
			return;
		}
	}

	return;
}

void do_icg(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int immc;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char(
				"Syntax: icg <char> <group|command|remove> <group/command name>\n\r",
				ch);
		return;
	}
	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't playing.\n\r", ch);
		return;
	}

	if (!str_prefix(arg2, "group")) {
		switch (LOWER(arg3[0])) {
		case 'a':
			victim->icg = ICG_ADMIN;
			send_to_char("Admin command group assigned.\n\r", ch);
			send_to_char("You've been assigned the admin command group.\n\r",
					victim);
			break;
		case 'b':
			victim->icg = ICG_BUILD;
			send_to_char("Builder command group assigned.\n\r", ch);
			send_to_char("You've been assigned the Builder command group.\n\r",
					victim);
			break;
		case 'j':
			victim->icg = ICG_JUDGE;
			send_to_char("Judge command group assigned.\n\r", ch);
			send_to_char("You've been assigned the Judge command group.\n\r",
					victim);
			break;
		case 'q':
			victim->icg = ICG_QUEST;
			send_to_char("Quest command group assigned.\n\r", ch);
			send_to_char("You've been assigned the Quest command group.\n\r",
					victim);
			break;
		case 'n':
			victim->icg = 0;
			send_to_char("No command group assigned.\n\r", ch);
			send_to_char("You've been stripped of your IMM command group.\n\r",
					victim);
			break;
		default:
			send_to_char(
					"You must choose a group from: quest, judge, builder," "admin or none.\n\r",
					ch);
			break;
		}
		return;
	}

	if (!str_prefix(arg2, "command")) {
		immc = immc_lookup(arg3);
		if (immc == -1) {
			send_to_char("That is not an assignable IMM command.\n\r", ch);
			return;
		}
		if (IS_SET(victim->icg_bits, imm_command_table[immc].bit)) {
			send_to_char("They already have that command assigned.\n\r", ch);
			return;
		}
		SET_BIT(victim->icg_bits, imm_command_table[immc].bit);
		return;
	}

	if (!str_prefix(arg2, "remove")) {
		immc = immc_lookup(arg3);
		if (immc == -1) {
			send_to_char("That is not an assignable IMM command.\n\r", ch);
			return;
		}

		if (IS_SET(victim->icg_bits, imm_command_table[immc].bit))
			REMOVE_BIT(victim->icg_bits, imm_command_table[immc].bit);
		send_to_char("Command removed.\n\r", ch);

		return;
	}

	send_to_char(
			"Syntax: icg <char> <group|command|remove> <group/command name>\n\r",
			ch);
	return;
}

void do_highlander(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	send_to_char("This command temporarily disabled.\n\r", ch);
	return;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Put Highlander List here.\n\r", ch);
		return;
	}

	if (IS_IMMORTAL(ch)) {

		if ((victim = get_char_world(ch, arg1)) == NULL) {
			send_to_char("They aren't playing.\n\r", ch);
			return;
		}

		if (!str_prefix("win", arg2) && IS_SET(victim->mhs, MHS_HIGHLANDER)) {
			send_to_char(
					"You have won The Prize! Congratulations Highlander!\n\r",
					victim);
			remove_highlander(ch, victim);
			return;
		}

		if (IS_SET(victim->mhs, MHS_HIGHLANDER)) {
			send_to_char("They are no longer a Highlander.\n\r", ch);
			send_to_char(
					"The Quickening no longer runs through your veins.\n\r",
					victim);
			remove_highlander(ch, victim);
			return;
		}

		if (!IS_SET(victim->mhs, MHS_HIGHLANDER)) {
			send_to_char("They are now a Highlander.\n\r", ch);
			send_to_char(
					"The Quickening runs through your veins, you are now a Highlander!\n\r",
					victim);
			act("$n goes to take some heads in Highlander Combat!", victim,
					NULL, NULL, TO_ROOM, FALSE);
			char_from_room(victim);
			char_to_room(victim, get_room_index(16502));
			SET_BIT(victim->mhs, MHS_HIGHLANDER);
			die_follower(victim);
			victim->pcdata->highlander_data[ALL_KILLS] = 0;
			victim->pcdata->highlander_data[REAL_KILLS] = 0;
			victim->pcdata->save_clan = victim->clan;
			if (!is_clan(victim))
				victim->clan = clan_lookup("temp");
			sprintf(buf,
					"%s adds %s to Highlander, %d/%d/%d/%d/%d/%d with all=%d and real=%d",
					ch->name, victim->name, victim->pcdata->perm_hit,
					victim->pcdata->perm_mana, victim->pcdata->perm_move,
					victim->max_hit, victim->max_mana, victim->max_move,
					victim->pcdata->highlander_data[ALL_KILLS],
					victim->pcdata->highlander_data[REAL_KILLS]);
			log_string(buf);
			return;
		}
	}

	return;
}

void remove_highlander(CHAR_DATA *ch, CHAR_DATA *victim) {
	char buf[MAX_STRING_LENGTH];

	sprintf(buf,
			"%s removes %s from Highlander, before %d/%d/%d/%d/%d/%d with all=%d and real=%d",
			ch->name, victim->name, victim->pcdata->perm_hit,
			victim->pcdata->perm_mana, victim->pcdata->perm_move,
			victim->max_hit, victim->max_mana, victim->max_move,
			victim->pcdata->highlander_data[ALL_KILLS],
			victim->pcdata->highlander_data[REAL_KILLS]);
	log_string(buf);

	victim->pcdata->perm_hit -= (victim->pcdata->highlander_data[ALL_KILLS])
			* 100;
	victim->pcdata->perm_mana -= (victim->pcdata->highlander_data[ALL_KILLS])
			* 100;
	victim->pcdata->perm_move -= (victim->pcdata->highlander_data[ALL_KILLS])
			* 100;
	victim->max_hit -= (victim->pcdata->highlander_data[ALL_KILLS]) * 100;
	victim->max_mana -= (victim->pcdata->highlander_data[ALL_KILLS]) * 100;
	victim->max_move -= (victim->pcdata->highlander_data[ALL_KILLS]) * 100;
	victim->pcdata->highlander_data[ALL_KILLS] = 0;
	victim->pcdata->highlander_data[REAL_KILLS] = 0;
	victim->clan = victim->pcdata->save_clan;
	victim->pcdata->save_clan = 0;
	victim->pcdata->quit_time = 0;
	REMOVE_BIT(victim->mhs, MHS_HIGHLANDER);

	sprintf(buf,
			"%s removes %s from Highlander, after %d/%d/%d/%d/%d/%d with all=%d and real=%d",
			ch->name, victim->name, victim->pcdata->perm_hit,
			victim->pcdata->perm_mana, victim->pcdata->perm_move,
			victim->max_hit, victim->max_mana, victim->max_move,
			victim->pcdata->highlander_data[ALL_KILLS],
			victim->pcdata->highlander_data[REAL_KILLS]);
	log_string(buf);

	return;
}

/* MM player guild*/
void do_guild(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int clan;

	if (IS_NPC(ch))
		return;

	if (ch->level < MAX_LEVEL - 4
			&& (!ch->pcdata->clan_info
					|| ch->pcdata->clan_info->clan->default_clan
					|| ch->pcdata->clan_info->rank < MAX_RANK - 1)
			&& (str_cmp(clan_table[ch->clan].name, "matook"))) {
		send_to_char("Sorry, you can't guild.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || (!ch->pcdata->clan_info && arg2[0] == '\0')) {
		if (!ch->pcdata->clan_info)
			send_to_char("Syntax: guild <char> <clan name>\n\r", ch);
		else
			send_to_char("Syntax: guild <char>\n\r", ch);
		return;
	}
	if ((victim = get_char_online(ch, arg1)) == NULL) {
		send_to_char("They aren't playing.\n\r", ch);
		return;
	}
	if (IS_NPC(victim)) {
		send_to_char("An NPC in a clan? I don't think so.\n\r", ch);
		return;
	}
	if (IS_IMMORTAL(ch)) {
		CLAN_DATA *clan;
		if (arg2[0] == '\0') {/* Guild them into the immortal's current clan */
			clan = ch->pcdata->clan_info->clan;
		} else if (!str_cmp(arg2, "matook")) {
			if (victim->pcdata->clan_info) {
				send_to_char(
						"They must be removed from the clan system first.\n\r",
						ch);
				return;
			}
			victim->clan = nonclan_lookup(arg2);
			send_to_char("You are now a member of Matook.\n\r", victim);
			sprintf(buf, "%s is now a member of Matook.\n\r", victim->name);
			send_to_char(buf, ch);
			return;
		} else if (!str_cmp(arg2, "none")) {
			if (victim->pcdata->clan_info) {
				remove_clan_member(victim->pcdata->clan_info);
				free_clan_char(victim->pcdata->clan_info);
				victim->pcdata->clan_info = NULL;
			} else if (victim->clan)
				victim->clan = 0;
			send_to_char("They are no longer in a clan.\n\r", ch);
			send_to_char("You have been removed from your clan.\n\r", ch);
			return;
		} else if ((clan = clan_lookup(arg2)) == NULL) {
			send_to_char("You would have to establish that clan first.\n\r",
					ch);
			return;
		}
		add_clan_member(clan, victim, 0);
		send_to_char("Ok.", ch);
		sprintf(buf, "You are now a member of %s.\n\r", clan->name);
		send_to_char(buf, victim);
		save_clan(victim, TRUE, FALSE, TRUE);
		return;
	}
	/* Temporary removal of level limits */
	if (ch->pcdata->clan_info) {
		if (((!is_clan(victim) && victim->level <= 20 && victim->level >= 5)
				|| (is_clan(victim)
						&& (!victim->pcdata->clan_info
								|| victim->pcdata->clan_info->clan->default_clan)))
				&& victim->join == ch->pcdata->clan_info->clan) {
			if (victim->pcdata->clan_info
					&& !str_cmp(victim->pcdata->clan_info->clan->name,
							"outcast")) {/* Prevent accidents */
				if (str_cmp(arg2, "outcast")) {
					send_to_char(
							"Use 'guild <target> outcast' if you want to guild an outcast.\n\rThis will cost your clan 1000 points.\n\r",
							ch);
					return;
				}
				if (ch->pcdata->clan_info->clan->tribute < 100000) {
					sprintf(buf,
							"Guilding an outcast costs 1000 points, your clan only has %d points.\n\r",
							ch->pcdata->clan_info->clan->tribute / 100);
					send_to_char(buf, ch);
					return;
				} else {
					sprintf(buf, "Your clan pays 1000 points to guild %s.\n\r",
							victim->name);
					send_to_char(buf, ch);
					ch->pcdata->clan_info->clan->tribute -= 1000;
				}
			}
			add_clan_member(ch->pcdata->clan_info->clan, victim, 0);
			send_to_char("They are now a member of your clan.", ch);
			sprintf(buf, "You are now a member of %s.\n\r",
					ch->pcdata->clan_info->clan->name);
			send_to_char(buf, victim);
			save_clan(victim, TRUE, FALSE, TRUE);
		} else
			send_to_char(
					"They are not eligible to join your clan.  Have they asked to join?\n\r",
					ch);
		return;
	} else {
		if (victim->pcdata->old_join == ch->clan) {
			/* Matook only at this point */
			victim->clan = ch->clan;
			send_to_char("They are now a member of your clan.\n\r", ch);
			sprintf(buf, "You are now a member of %s.\n\r",
					capitalize(clan_table[victim->clan].name));
			send_to_char(buf, victim);
			return;
		}
	}
}

/* equips a character */
void do_outfit(CHAR_DATA *ch, char *argument) {
	OBJ_DATA *obj;
	int i, sn, vnum;

	if ((ch->level > 5 || IS_NPC(ch)) && !HAS_KIT(ch, "nethermancer")) {
		send_to_char("Find it yourself!\n\r", ch);
		return;
	}

	if (HAS_KIT(ch, "nethermancer")) {
		obj = create_object(get_obj_index(OBJ_VNUM_ROBES), 0, FALSE);
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_BODY);
		return;
	}

	if (ch->carry_number + 3 > can_carry_n(ch)) {
		send_to_char("You can't carry any more stuff.\n\r", ch);
		return;
	}

	if ((obj = get_eq_char(ch, WEAR_LIGHT)) == NULL) {
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_BANNER), 0, FALSE);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_LIGHT);
	}

	if ((obj = get_eq_char(ch, WEAR_BODY)) == NULL) {
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_VEST), 0, FALSE);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_BODY);
	}

	/* do the weapon thing */
	if ((obj = get_eq_char(ch, WEAR_WIELD)) == NULL) {
		sn = 0;
		vnum = OBJ_VNUM_SCHOOL_SWORD; /* just in case! */

		for (i = 0; weapon_table[i].name != NULL; i++) {
			if (ch->pcdata->learned[sn]
					< ch->pcdata->learned[*weapon_table[i].gsn]) {
				sn = *weapon_table[i].gsn;
				vnum = weapon_table[i].vnum;
			}
		}

		obj = create_object(get_obj_index(vnum), 0, FALSE);
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_WIELD);
	}

	if (((obj = get_eq_char(ch, WEAR_WIELD)) == NULL
			|| !IS_WEAPON_STAT(obj, WEAPON_TWO_HANDS))
			&& (obj = get_eq_char(ch, WEAR_SHIELD)) == NULL) {
		obj = create_object(get_obj_index(OBJ_VNUM_SCHOOL_SHIELD), 0, FALSE);
		obj->cost = 0;
		obj_to_char(obj, ch);
		equip_char(ch, obj, WEAR_SHIELD);
	}

	send_to_char("You have been equipped by Mojo.\n\r", ch);
}

void do_nonotes(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Nonote whom?", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NONOTES)) {
		REMOVE_BIT(victim->comm, COMM_NONOTES);
		send_to_char("The gods have restored your writing priviliges.\n\r",
				victim);
		send_to_char("NONOTES removed.\n\r", ch);
		sprintf(buf, "$N restores notes to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NONOTES);
		send_to_char("The gods have revoked your writing priviliges.\n\r",
				victim);
		send_to_char("NONOTES set.\n\r", ch);
		sprintf(buf, "$N revokes %s's notes.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;

}

void do_setcouncil(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Set council bit on whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Very funny, a mob as Matook Council?\n\r", ch);
		return;
	}

	if (IS_SET(victim->mhs, MHS_MATOOK_COUNCIL)) {
		REMOVE_BIT(victim->mhs, MHS_MATOOK_COUNCIL);
		send_to_char("Matook council bit UNSET.\n\r", ch);
		send_to_char("You are no longer Matook council.\n\r", victim);
		sprintf(buf, "$N removes Matook council flag from %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_FLAGS, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->mhs, MHS_MATOOK_COUNCIL);
		send_to_char("You have been flagged Matook council.\n\r", victim);
		send_to_char("Matook council bit SET.\n\r", ch);
		sprintf(buf, "$N sets Matook council flag on %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_FLAGS, WIZ_SECURE, 0);
	}
	return;
}

/* RT nochannels command, for those spammers */
void do_nochannels(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Nochannel whom?", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOCHANNELS)) {
		REMOVE_BIT(victim->comm, COMM_NOCHANNELS);
		send_to_char("The gods have restored your channel priviliges.\n\r",
				victim);
		send_to_char("NOCHANNELS removed.\n\r", ch);
		sprintf(buf, "$N restores channels to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOCHANNELS);
		send_to_char("The gods have revoked your channel priviliges.\n\r",
				victim);
		send_to_char("NOCHANNELS set.\n\r", ch);
		sprintf(buf, "$N revokes %s's channels.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

void do_smote(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *vch;
	char *letter, *name;
	char last[MAX_INPUT_LENGTH], temp[MAX_STRING_LENGTH];
	int matches = 0;

	if (!IS_NPC(ch) && IS_SET(ch->comm, COMM_NOEMOTE)) {
		send_to_char("You can't show your emotions.\n\r", ch);
		return;
	}

	if (argument[0] == '\0') {
		send_to_char("Emote what?\n\r", ch);
		return;
	}

	if (strstr(argument, ch->name) == NULL) {
		send_to_char("You must include your name in an smote.\n\r", ch);
		return;
	}

	send_to_char(argument, ch);
	send_to_char("\n\r", ch);

	for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
		if (vch->desc == NULL || vch == ch)
			continue;

		if ((letter = strstr(argument, vch->name)) == NULL) {
			send_to_char(argument, vch);
			send_to_char("\n\r", vch);
			continue;
		}

		strcpy(temp, argument);
		temp[strlen(argument) - strlen(letter)] = '\0';
		last[0] = '\0';
		name = vch->name;

		for (; *letter != '\0'; letter++) {
			if (*letter == '\'' && matches == strlen(vch->name)) {
				strcat(temp, "r");
				continue;
			}

			if (*letter == 's' && matches == strlen(vch->name)) {
				matches = 0;
				continue;
			}

			if (matches == strlen(vch->name)) {
				matches = 0;
			}

			if (*letter == *name) {
				matches++;
				name++;
				if (matches == strlen(vch->name)) {
					strcat(temp, "you");
					last[0] = '\0';
					name = vch->name;
					continue;
				}
				strncat(last, letter, 1);
				continue;
			}

			matches = 0;
			strcat(temp, last);
			strncat(temp, letter, 1);
			last[0] = '\0';
			name = vch->name;
		}

		send_to_char(temp, vch);
		send_to_char("\n\r", vch);
	}

	return;
}

void do_bamfin(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];

	if (!IS_NPC(ch)) {
		smash_tilde(argument);

		if (argument[0] == '\0') {
			sprintf(buf, "Your poofin is %s\n\r", ch->pcdata->bamfin);
			send_to_char(buf, ch);
			return;
		}

		if (strstr(argument, ch->name) == NULL) {
			send_to_char("You must include your name.\n\r", ch);
			return;
		}

		free_string(ch->pcdata->bamfin);
		ch->pcdata->bamfin = str_dup(argument);

		sprintf(buf, "Your poofin is now %s\n\r", ch->pcdata->bamfin);
		send_to_char(buf, ch);
	}
	return;
}

void do_bamfout(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];

	if (!IS_NPC(ch)) {
		smash_tilde(argument);

		if (argument[0] == '\0') {
			sprintf(buf, "Your poofout is %s\n\r", ch->pcdata->bamfout);
			send_to_char(buf, ch);
			return;
		}

		if (strstr(argument, ch->name) == NULL) {
			send_to_char("You must include your name.\n\r", ch);
			return;
		}

		free_string(ch->pcdata->bamfout);
		ch->pcdata->bamfout = str_dup(argument);

		sprintf(buf, "Your poofout is now %s\n\r", ch->pcdata->bamfout);
		send_to_char(buf, ch);
	}
	return;
}

void do_deny(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Deny whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (!IS_SET(victim->act, PLR_DENY)) {
		SET_BIT(victim->act, PLR_DENY);
		send_to_char("You are denied access!\n\r", victim);
		sprintf(buf, "$N denies access to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
		send_to_char("OK.\n\r", ch);
		save_char_obj(victim);
		do_quit(victim, "");
	} else {
		REMOVE_BIT(victim->act, PLR_DENY);
		send_to_char("You are allowed access once more.\n\r", victim);
		sprintf(buf, "$N permits access to %s", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
		send_to_char("OK.\n\r", ch);
		save_char_obj(victim);
	}
	return;
}

void do_disconnect(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Disconnect whom?\n\r", ch);
		return;
	}

	if (is_number(arg)) {
		int desc;

		desc = atoi(arg);
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->descriptor == desc) {
				if (d->character != NULL && d->character->level >= ch->level) {
					send_to_char("You looking to get Smacked?\n\r", ch);
					return;
				}

				close_socket(d);
				send_to_char("Ok.\n\r", ch);
				return;
			}
		}
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc == NULL) {
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR, TRUE);
		return;
	}

	if (victim->level >= ch->level) {
		send_to_char("You looking to get Smacked?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d == victim->desc) {
			close_socket(d);
			send_to_char("Ok.\n\r", ch);
			return;
		}
	}

	bug("Do_disconnect: desc not found.", 0);
	send_to_char("Descriptor not found!\n\r", ch);
	return;
}

void do_pardon(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0') {
		send_to_char("Syntax: pardon <character> <killer|thief>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "killer")) {
		victim->trumps = 0;
		victim->pcdata->ruffT = 0;
		if (IS_SET(victim->wiznet, PLR_RUFFIAN))
			REMOVE_BIT(victim->wiznet, PLR_RUFFIAN);
		if (IS_SET(victim->act, PLR_KILLER)) {
			REMOVE_BIT(victim->act, PLR_KILLER);
			send_to_char("Killer flag removed.\n\r", ch);
			send_to_char("You are no longer a KILLER.\n\r", victim);
		}
		return;
	}

	if (!str_cmp(arg2, "thief")) {
		if (IS_SET(victim->act, PLR_THIEF)) {
			REMOVE_BIT(victim->act, PLR_THIEF);
			send_to_char("Thief flag removed.\n\r", ch);
			send_to_char("You are no longer a THIEF.\n\r", victim);
		}
		return;
	}

	send_to_char("Syntax: pardon <character> <killer|thief>.\n\r", ch);
	return;
}

void do_echo(CHAR_DATA *ch, char *argument) {
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		send_to_char("Global echo what?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING) {
			if (get_trust(d->character) >= get_trust(ch))
				send_to_char("global> ", d->character);
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}

	return;
}

void do_recho(CHAR_DATA *ch, char *argument) {
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		send_to_char("Local echo what?\n\r", ch);

		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING
				&& d->character->in_room == ch->in_room) {
			if (get_trust(d->character) >= get_trust(ch))
				send_to_char("local> ", d->character);
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}

	return;
}

void do_zecho(CHAR_DATA *ch, char *argument) {
	DESCRIPTOR_DATA *d;

	if (argument[0] == '\0') {
		send_to_char("Zone echo what?\n\r", ch);
		return;
	}

	for (d = descriptor_list; d; d = d->next) {
		if (d->connected == CON_PLAYING && d->character->in_room != NULL
				&& ch->in_room != NULL
				&& d->character->in_room->area == ch->in_room->area) {
			if (get_trust(d->character) >= get_trust(ch))
				send_to_char("zone> ", d->character);
			send_to_char(argument, d->character);
			send_to_char("\n\r", d->character);
		}
	}
}

void do_pecho(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg);

	if (argument[0] == '\0' || arg[0] == '\0') {
		send_to_char("Personal echo what?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("Target not found.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch) && get_trust(ch) != MAX_LEVEL)
		send_to_char("personal> ", victim);

	send_to_char(argument, victim);
	send_to_char("\n\r", victim);
	send_to_char("personal> ", ch);
	send_to_char(argument, ch);
	send_to_char("\n\r", ch);
}

ROOM_INDEX_DATA *find_location(CHAR_DATA *ch, char *arg) {
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	if (is_number(arg))
		return get_room_index(atoi(arg));

	if ((victim = get_char_world(ch, arg)) != NULL)
		return victim->in_room;

	if ((obj = get_obj_world(ch, arg)) != NULL)
		return obj->in_room;

	return NULL;
}

void do_transfer(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0') {
		send_to_char("Transfer whom (and where)?\n\r", ch);
		return;
	}

	if (!str_cmp(arg1, "all")) {
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->connected == CON_PLAYING && d->character != ch
					&& d->character->in_room != NULL
					&& can_see(ch, d->character, TRUE)) {
				char buf[MAX_STRING_LENGTH];
				sprintf(buf, "%s %s", d->character->name, arg2);
				do_transfer(ch, buf);
			}
		}
		return;
	}

	/*
	 * Thanks to Grodyn for the optional location parameter.
	 */
	if (arg2[0] == '\0') {
		location = ch->in_room;
	} else {
		if ((location = find_location(ch, arg2)) == NULL) {
			send_to_char("No such location.\n\r", ch);
			return;
		}

		if (!is_room_owner(ch, location) && room_is_private(ch, location )
		&& get_trust(ch) < MAX_LEVEL) {
			send_to_char("That room is private right now.\n\r", ch);
			return;
		}
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (ch->level < victim->level && !IS_NPC(victim)) {
		send_to_char("They may not want to come. Why not ask them to goto?\n\r",
				ch);
		return;
	}

	if (victim->in_room == NULL) {
		send_to_char("They are in limbo.\n\r", ch);
		return;
	}

	if (victim->fighting != NULL)
		stop_fighting(victim, TRUE);
	act("$n disappears in a mushroom cloud.", victim, NULL, NULL, TO_ROOM,
			FALSE);
	char_from_room(victim);
	clear_mount(victim);
	char_to_room(victim, location);
	act("$n arrives from a puff of smoke.", victim, NULL, NULL, TO_ROOM, FALSE);
	if (ch != victim)
		act("$n has transferred you.", ch, NULL, victim, TO_VICT, FALSE);
	do_look(victim, "auto");
	send_to_char("Ok.\n\r", ch);

}

void do_at(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	ROOM_INDEX_DATA *original;
	OBJ_DATA *on;
	CHAR_DATA *wch;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("At where what?\n\r", ch);
		return;
	}

	if ((location = find_location(ch, arg)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, location) && room_is_private(ch, location )
	&& get_trust(ch) < MAX_LEVEL) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	original = ch->in_room;
	on = ch->on;
	char_from_room(ch);
	clear_mount(ch);
	char_to_room(ch, location);
	interpret(ch, argument);

	/*
	 * See if 'ch' still exists before continuing!
	 * Handles 'at XXXX quit' case.
	 */
	for (wch = char_list; wch != NULL; wch = wch->next) {
		if (wch == ch) {
			char_from_room(ch);
			clear_mount(ch);
			char_to_room(ch, original);
			ch->on = on;
			break;
		}
	}

	return;
}

void do_goto(CHAR_DATA *ch, char *argument) {
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;
	int count = 0;

	if (argument[0] == '\0') {
		send_to_char("Goto where?\n\r", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	count = 0;
	for (rch = location->people; rch != NULL; rch = rch->next_in_room)
		count++;

	if (!is_room_owner(ch, location) && room_is_private(ch, location)
			&& (count > 1 || get_trust(ch) < MAX_LEVEL)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0') {
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT, FALSE);
			} else {
				if (get_trust(ch) < 60)
					act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT,
							FALSE);
			}
		}
	}

	char_from_room(ch);
	char_to_room(ch, location);
	clear_mount(ch);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0') {
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT, FALSE);
			} else {
				if (get_trust(ch) < 60)
					act("$n appears in a swirling mist.", ch, NULL, rch,
							TO_VICT, FALSE);
			}
		}
	}

	do_look(ch, "auto");
	return;
}

void do_violate(CHAR_DATA *ch, char *argument) {
	ROOM_INDEX_DATA *location;
	CHAR_DATA *rch;

	if (argument[0] == '\0') {
		send_to_char("Goto where?\n\r", ch);
		return;
	}

	if ((location = find_location(ch, argument)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!room_is_private(ch, location)) {
		send_to_char("That room isn't private, use goto.\n\r", ch);
		return;
	}

	if (ch->fighting != NULL)
		stop_fighting(ch, TRUE);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfout[0] != '\0')
				act("$t", ch, ch->pcdata->bamfout, rch, TO_VICT, FALSE);
			else
				act("$n leaves in a swirling mist.", ch, NULL, rch, TO_VICT,
						FALSE);
		}
	}

	char_from_room(ch);
	char_to_room(ch, location);
	clear_mount(ch);

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (get_trust(rch) >= ch->invis_level) {
			if (ch->pcdata != NULL && ch->pcdata->bamfin[0] != '\0')
				act("$t", ch, ch->pcdata->bamfin, rch, TO_VICT, FALSE);
			else
				act("$n appears in a swirling mist.", ch, NULL, rch, TO_VICT,
						FALSE);
		}
	}

	do_look(ch, "auto");
	return;
}

/* RT to replace the 3 stat commands */

void do_stat(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char *string;
	/*
	 OBJ_DATA *obj;
	 ROOM_INDEX_DATA *location;
	 CHAR_DATA *victim;
	 */

	string = one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  stat obj <name>\n\r", ch);
		send_to_char("  stat mob <name>\n\r", ch);
		send_to_char("  stat room <number>\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "room")) {
		do_rstat(ch, string);
		return;
	}

	if (!str_cmp(arg, "obj")) {
		do_ostat(ch, string);
		return;
	}

	if (!str_cmp(arg, "char") || !str_cmp(arg, "mob")) {
		do_mstat(ch, string);
		return;
	}

	send_to_char("Syntax:\n\r", ch);
	send_to_char("  stat obj <name>\n\r", ch);
	send_to_char("  stat mob <name>\n\r", ch);
	send_to_char("  stat room <number>\n\r", ch);
	return;

	/* the old way
	 *

	 obj = get_obj_world(ch,argument);
	 if (obj != NULL)
	 {
	 do_ostat(ch,argument);
	 return;
	 }

	 victim = get_char_world(ch,argument);
	 if (victim != NULL)
	 {
	 do_mstat(ch,argument);
	 return;
	 }

	 location = find_location(ch,argument);
	 if (location != NULL)
	 {
	 do_rstat(ch,argument);
	 return;
	 }

	 send_to_char("Nothing by that name found anywhere.\n\r",ch);
	 *
	 */
}

void do_rstat(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	OBJ_DATA *obj;
	CHAR_DATA *rch;
	int door;
	AFFECT_DATA *paf;

	one_argument(argument, arg);
	location = (arg[0] == '\0') ? ch->in_room : find_location(ch, arg);
	if (location == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, location) && ch->in_room != location
	&& room_is_private(ch, location ) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	sprintf(buf, "Name: '%s'\n\rArea: '%s'\n\r", location->name,
			location->area ? location->area->name : "None");
	send_to_char(buf, ch);

	sprintf(buf, "Vnum: %d  Sector: %d  Light: %d  Healing: %d  Mana: %d\n\r",
			location->vnum, location->sector_type, location->light,
			location->heal_rate, location->mana_rate);
	send_to_char(buf, ch);

	sprintf(buf, "Room flags: %ld.\n\rDescription:\n\r%s", location->room_flags,
			location->description);
	send_to_char(buf, ch);

	if (location->extra_descr != NULL) {
		EXTRA_DESCR_DATA *ed;

		send_to_char("Extra description keywords: '", ch);
		for (ed = location->extra_descr; ed; ed = ed->next) {
			send_to_char(ed->keyword, ch);
			if (ed->next != NULL)
				send_to_char(" ", ch);
		}
		send_to_char("'.\n\r", ch);
	}

	send_to_char("Characters:", ch);
	for (rch = location->people; rch; rch = rch->next_in_room) {
		if (can_see(ch, rch, TRUE)) {
			send_to_char(" ", ch);
			one_argument(rch->name, buf);
			send_to_char(buf, ch);
		}
	}

	send_to_char(".\n\rObjects:   ", ch);
	for (obj = location->contents; obj; obj = obj->next_content) {
		send_to_char(" ", ch);
		one_argument(obj->name, buf);
		send_to_char(buf, ch);
	}
	send_to_char(".\n\r", ch);

	for (door = 0; door <= 5; door++) {
		EXIT_DATA *pexit;

		if ((pexit = location->exit[door]) != NULL) {
			sprintf(buf,
					"Door: %d.  To: %d.  Key: %d.  Exit flags: %s.\n\rKeyword: '%s'.\n\r",

					door,
					(pexit->u1.to_room == NULL ? -1 : pexit->u1.to_room->vnum),
					pexit->key, exit_bit_name(pexit->exit_info),
					pexit->keyword);
			send_to_char(buf, ch);
		}
	}

	for (paf = location->affected; paf != NULL; paf = paf->next) {
		sprintf(buf, "Affect: '%s' level %d, duration %d\n\r",
				skill_table[(int) paf->type].name, paf->level, paf->duration);
		send_to_char(buf, ch);
	}

	return;
}

void do_ostat(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	OBJ_DATA *obj;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Stat what?\n\r", ch);
		return;
	}

	if ((obj = get_obj_world(ch, argument)) == NULL) {
		send_to_char("Nothing like that in hell, earth, or heaven.\n\r", ch);
		return;
	}

	sprintf(buf, "Name(s): %s\n\r", obj->name);
	send_to_char(buf, ch);

	sprintf(buf, "Vnum: %d  Format: %s  Type: %s  Resets: %d\n\r",
			obj->pIndexData->vnum, obj->pIndexData->new_format ? "new" : "old",
			item_type_name(obj), obj->pIndexData->reset_num);
	send_to_char(buf, ch);

	sprintf(buf, "Short description: %s\n\rLong description: %s\n\r",
			obj->short_descr, obj->description);
	send_to_char(buf, ch);

	sprintf(buf, "Wear bits: %s\n\rExtra bits: %s\n\r Extra2 bits: %s\n\r",
			wear_bit_name(obj->wear_flags), extra_bit_name(obj->extra_flags),
			extra2_bit_name(obj->extra_flags2));
	send_to_char(buf, ch);

	if (obj->pIndexData->new_format) {
		sprintf(buf, "Material type: %s\n\r", obj->material);
		send_to_char(buf, ch);
	}
	sprintf(buf, "Number: %d/%d  Weight: %d/%d/%d (10th pounds) Damage: %d\n\r",
			1, get_obj_number(obj), obj->weight, get_obj_weight(obj),
			get_true_weight(obj), obj->damaged);
	send_to_char(buf, ch);

	sprintf(buf,
			"Level: %d  Cost: %d  Condition: %d  Timer: %d  Wear Timer: %d\n\r",
			obj->level, obj->cost, obj->condition, obj->timer, obj->wear_timer);
	send_to_char(buf, ch);

	sprintf(buf, "In room: %d  In object: %s  Carried by: %s  Wear_loc: %d\n\r",
			obj->in_room == NULL ? 0 : obj->in_room->vnum,
			obj->in_obj == NULL ? "(none)" : obj->in_obj->short_descr,
			obj->carried_by == NULL ? "(none)" :
			can_see(ch, obj->carried_by, TRUE) ?
					obj->carried_by->name : "someone", obj->wear_loc);
	send_to_char(buf, ch);

	sprintf(buf, "Values: %d %d %d %d %d\n\r", obj->value[0], obj->value[1],
			obj->value[2], obj->value[3], obj->value[4]);
	send_to_char(buf, ch);

	sprintf(buf, "Last owned by: %s\n\r",
			obj->prev_owner != NULL ? obj->prev_owner : "no one");
	send_to_char(buf, ch);

	/* now give out vital statistics as per identify */

	switch (obj->item_type) {
	case ITEM_SCROLL:
	case ITEM_POTION:
	case ITEM_PILL:
		sprintf(buf, "Level %d spells of:", obj->value[0]);
		send_to_char(buf, ch);

		if (obj->value[1] >= 0 && obj->value[1] < MAX_SKILL) {
			send_to_char(" '", ch);
			send_to_char(skill_table[obj->value[1]].name, ch);
			send_to_char("'", ch);
		}

		if (obj->value[2] >= 0 && obj->value[2] < MAX_SKILL) {
			send_to_char(" '", ch);
			send_to_char(skill_table[obj->value[2]].name, ch);
			send_to_char("'", ch);
		}

		if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
			send_to_char(" '", ch);
			send_to_char(skill_table[obj->value[3]].name, ch);
			send_to_char("'", ch);
		}

		send_to_char(".\n\r", ch);
		break;

	case ITEM_WAND:
	case ITEM_STAFF:
		sprintf(buf, "Has %d(%d) charges of level %d", obj->value[1],
				obj->value[2], obj->value[0]);
		send_to_char(buf, ch);

		if (obj->value[3] >= 0 && obj->value[3] < MAX_SKILL) {
			send_to_char(" '", ch);
			send_to_char(skill_table[obj->value[3]].name, ch);
			send_to_char("'", ch);
		}

		send_to_char(".\n\r", ch);
		break;

	case ITEM_DRINK_CON:
		sprintf(buf, "It holds %s-colored %s.\n\r",
				liq_table[obj->value[2]].liq_color,
				liq_table[obj->value[2]].liq_name);
		send_to_char(buf, ch);
		break;

	case ITEM_WEAPON:
		send_to_char("Weapon type is ", ch);
		switch (obj->value[0]) {
		case (WEAPON_EXOTIC):
			send_to_char("exotic\n\r", ch);
			break;
		case (WEAPON_SWORD):
			send_to_char("sword\n\r", ch);
			break;
		case (WEAPON_DAGGER):
			send_to_char("dagger\n\r", ch);
			break;
		case (WEAPON_SPEAR):
			send_to_char("spear/staff\n\r", ch);
			break;
		case (WEAPON_MACE):
			send_to_char("mace/club\n\r", ch);
			break;
		case (WEAPON_AXE):
			send_to_char("axe\n\r", ch);
			break;
		case (WEAPON_FLAIL):
			send_to_char("flail\n\r", ch);
			break;
		case (WEAPON_WHIP):
			send_to_char("whip\n\r", ch);
			break;
		case (WEAPON_POLEARM):
			send_to_char("polearm\n\r", ch);
			break;
		case (WEAPON_GAROTTE):
			send_to_char("garotte\n\r", ch);
			break;
		default:
			send_to_char("unknown\n\r", ch);
			break;
		}
		if (obj->pIndexData->new_format)
			sprintf(buf, "Damage is %dd%d (average %d)\n\r", obj->value[1],
					obj->value[2], (1 + obj->value[2]) * obj->value[1] / 2);
		else
			sprintf(buf, "Damage is %d to %d (average %d)\n\r", obj->value[1],
					obj->value[2], (obj->value[1] + obj->value[2]) / 2);
		send_to_char(buf, ch);

		sprintf(buf, "Damage noun is %s.\n\r",
				attack_table[obj->value[3]].noun);
		send_to_char(buf, ch);

		if (obj->value[4]) /* weapon flags */
		{
			sprintf(buf, "Weapons flags: %s\n\r",
					weapon_bit_name(obj->value[4]));
			send_to_char(buf, ch);
		}
		break;

	case ITEM_ARMOR:
		sprintf(buf,
				"Armor class is %d pierce, %d bash, %d slash, and %d vs. magic\n\rSize: %s\n\r",
				obj->value[0], obj->value[1], obj->value[2], obj->value[3],
				obj_size_table[obj->value[4] <= MAX_OBJ_SIZE ? obj->value[4] : 0].name);
		send_to_char(buf, ch);
		break;

	case ITEM_CONTAINER:
		sprintf(buf, "Capacity: %d#  Maximum weight: %d#  flags: %s\n\r",
				obj->value[0], obj->value[3], cont_bit_name(obj->value[1]));
		send_to_char(buf, ch);
		if (obj->value[4] != 100) {
			sprintf(buf, "Weight multiplier: %d%%\n\r", obj->value[4]);
			send_to_char(buf, ch);
		}
		break;
	}

	if (obj->extra_descr != NULL || obj->pIndexData->extra_descr != NULL) {
		EXTRA_DESCR_DATA *ed;

		send_to_char("Extra description keywords: '", ch);

		for (ed = obj->extra_descr; ed != NULL; ed = ed->next) {
			send_to_char(ed->keyword, ch);
			/* if ( ed->next != NULL ) */
			send_to_char(" ", ch);
		}

		for (ed = obj->pIndexData->extra_descr; ed != NULL; ed = ed->next) {
			send_to_char(ed->keyword, ch);
			if (ed->next != NULL)
				send_to_char(" ", ch);
		}

		send_to_char("'\n\r", ch);
	}

	for (paf = obj->affected; paf != NULL; paf = paf->next) {
		sprintf(buf, "Affects %s by %d, level %d",
				affect_loc_name(paf->location), paf->modifier, paf->level);
		send_to_char(buf, ch);
		if (paf->duration > -1)
			sprintf(buf, ", %d hours.\n\r", paf->duration);
		else
			sprintf(buf, ".\n\r");
		send_to_char(buf, ch);
		if (paf->bitvector) {
			switch (paf->where) {
			case TO_AFFECTS:
				sprintf(buf, "Adds %s affect.\n",
						affect_bit_name(paf->bitvector));
				break;
			case TO_WEAPON:
				sprintf(buf, "Adds %s weapon flags.\n",
						weapon_bit_name(paf->bitvector));
				break;
			case TO_OBJECT:
				sprintf(buf, "Adds %s object flag.\n",
						extra_bit_name(paf->bitvector));
				break;
			case TO_IMMUNE:
				sprintf(buf, "Adds immunity to %s.\n",
						imm_bit_name(paf->bitvector));
				break;
			case TO_RESIST:
				sprintf(buf, "Adds resistance to %s.\n\r",
						imm_bit_name(paf->bitvector));
				break;
			case TO_VULN:
				sprintf(buf, "Adds vulnerability to %s.\n\r",
						imm_bit_name(paf->bitvector));
				break;
			default:
				sprintf(buf, "Unknown bit %d: %ld\n\r", paf->where,
						paf->bitvector);
				break;
			}
			send_to_char(buf, ch);
		}
	}

	if (!obj->enchanted)
		for (paf = obj->pIndexData->affected; paf != NULL; paf = paf->next) {
			sprintf(buf, "Affects %s by %d, level %d.\n\r",
					affect_loc_name(paf->location), paf->modifier, paf->level);
			send_to_char(buf, ch);
			if (paf->bitvector) {
				switch (paf->where) {
				case TO_AFFECTS:
					sprintf(buf, "Adds %s affect.\n",
							affect_bit_name(paf->bitvector));
					break;
				case TO_OBJECT:
					sprintf(buf, "Adds %s object flag.\n",
							extra_bit_name(paf->bitvector));
					break;
				case TO_IMMUNE:
					sprintf(buf, "Adds immunity to %s.\n",
							imm_bit_name(paf->bitvector));
					break;
				case TO_RESIST:
					sprintf(buf, "Adds resistance to %s.\n\r",
							imm_bit_name(paf->bitvector));
					break;
				case TO_VULN:
					sprintf(buf, "Adds vulnerability to %s.\n\r",
							imm_bit_name(paf->bitvector));
					break;
				default:
					sprintf(buf, "Unknown bit %d: %ld\n\r", paf->where,
							paf->bitvector);
					break;
				}
				send_to_char(buf, ch);
			}
		}

	if (obj->item_type == ITEM_FORGE && ch->level >= CREATOR) {
		RECIPE_DATA *recipe;
		int i, j;
		OBJ_INDEX_DATA *t_obj;
		char buf2[MAX_STRING_LENGTH];

		send_to_char("RECIPES:\n\r\n\r", ch);

		for (i = 0; i <= 4; i++) {
			recipe = NULL;
			if (obj->value[i] != 0) {
				recipe = get_recipe_data(obj->value[i]);
			}

			if (recipe == NULL)
				continue;

			sprintf(buf, "Recipe vnum %d: \n\rComplete Item: %d\n\r",
					recipe->recipe_num, recipe->vnum_complete);
			send_to_char(buf, ch);
			buf2[0] = '\0';
			for (j = 0; j < MAX_IN_RECIPE; j++) {
				if (recipe->vnum_parts[j] == 0)
					break;

				t_obj = get_obj_index(recipe->vnum_parts[j]);
				if (t_obj == NULL)
					bug("Null recipe item %d", recipe->vnum_parts[j]);
				else {
					sprintf(buf, ", %s  ", t_obj->short_descr);
					strcat(buf2, buf);
				}
			}
			send_to_char(buf2 + 1, ch); /* +1 to skip the first comma */
			send_to_char("\n\r", ch);
		}
		send_to_char("\n\r", ch);
	}
	return;
}

void do_mstat(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char strtime[25];
	char arg[MAX_INPUT_LENGTH];
	AFFECT_DATA *paf;
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Stat whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	sprintf(buf, "Name: %s %s\n\r", victim->name,
			!IS_NPC(victim) ? victim->pcdata->surname : "(Mob)");
	send_to_char(buf, ch);

	if (!IS_NPC(victim)) {
		strcpy(strtime, ctime(&victim->pcdata->created_date));
		strtime[strlen(strtime) - 1] = '\0';
		sprintf(buf,
				"Clan: %s  Rank: %d  Host: %s Created: %s\n\rStart Timer: %d \r\n",
				is_clan(victim) ? clan_table[victim->clan].name : "(none)",
				is_clan(victim) ? victim->pcdata->rank : 0,
				victim->pcdata->last_host, strtime,
				is_clan(victim) ? victim->pcdata->start_time : 0);
		send_to_char(buf, ch);
	}

	sprintf(buf,
			"Vnum: %d  Format: %s  Race: %s  Group: %d  Sex: %s  Room: %d\n\r",
			IS_NPC(victim) ? victim->pIndexData->vnum : 0,
			IS_NPC(victim) ?
					victim->pIndexData->new_format ? "new" : "old" : "pc",
			race_table[victim->race].name,
			IS_NPC(victim) ? victim->group : 0, sex_table[victim->sex].name,
			victim->in_room == NULL ? 0 : victim->in_room->vnum);
	send_to_char(buf, ch);

	if (IS_NPC(victim)) {
		sprintf(buf, "Count: %d  Killed: %d  -  Life Timer:  %d\n\r",
				victim->pIndexData->count, victim->pIndexData->killed,
				victim->life_timer);
		send_to_char(buf, ch);
	}

	sprintf(buf,
			"Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)  Con: %d(%d)  Cha: %d(%d)\n\r",
//    "Str: %d(%d)  Int: %d(%d)  Wis: %d(%d)  Dex: %d(%d)\n\rCon: %d(%d)  Agt: %d(%d)  End: %d(%d)  Soc: %d(%d)\n\r",
			victim->perm_stat[STAT_STR], get_curr_stat(victim, STAT_STR),
			victim->perm_stat[STAT_INT], get_curr_stat(victim, STAT_INT),
			victim->perm_stat[STAT_WIS], get_curr_stat(victim, STAT_WIS),
			victim->perm_stat[STAT_DEX], get_curr_stat(victim, STAT_DEX),
			victim->perm_stat[STAT_CON], get_curr_stat(victim, STAT_CON),
//  victim->perm_stat[STAT_AGT],
//  get_curr_stat(victim,STAT_AGT),
//  victim->perm_stat[STAT_END],
//  get_curr_stat(victim,STAT_END),
			victim->perm_stat[STAT_SOC], get_curr_stat(victim, STAT_SOC));
	send_to_char(buf, ch);

	sprintf(buf, "Hp: %d/%d  Mana: %d/%d  Move: %d/%d  Prac: %d  Train: %d\n\r",
			victim->hit, victim->max_hit, victim->mana, victim->max_mana,
			victim->move, victim->max_move,
			IS_NPC(victim) ? 0 : victim->practice,
			IS_NPC(victim) ? 0 : victim->train);
	send_to_char(buf, ch);
	if (!IS_NPC(victim)) {
		sprintf(buf,
				"TrainedStats: %d %d %d  Retrain: %d  Half Train: %d  Half Retrain: %d\n\r",
				victim->pcdata->trained_hit, victim->pcdata->trained_mana,
				victim->pcdata->trained_move, victim->pcdata->retrain,
				victim->pcdata->half_train, victim->pcdata->half_retrain);
		send_to_char(buf, ch);
	}

	sprintf(buf,
			"Lv: %d(%d)  Cls: %s %s  Align: %d  Go: %ld  Si: %ld  XP: %d\n\r",
			victim->level,
			IS_NPC(victim) ? 0 : victim->pcdata->debit_level,
			IS_NPC(victim) ? "" : class_table[victim->pcdata->old_class].name,
			IS_NPC(victim) ? "mobile" : class_table[victim->class].name,
			victim->alignment, victim->gold, victim->silver, victim->exp);
	send_to_char(buf, ch);

	sprintf(buf, "Armor: pierce: %d  bash: %d  slash: %d  magic: %d\n\r",
			GET_AC(victim, AC_PIERCE), GET_AC(victim, AC_BASH),
			GET_AC(victim, AC_SLASH), GET_AC(victim, AC_EXOTIC));
	send_to_char(buf, ch);

	if (!IS_NPC(victim)) {
		sprintf(buf,
				"Hit: %d/%d  Dam: %d/%d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\n\r",
				GET_HITROLL(victim), GET_SECOND_HITROLL(victim),
				GET_DAMROLL(victim), GET_SECOND_DAMROLL(victim),
				victim->saving_throw, size_table[victim->size].name,
				position_table[victim->position].name, victim->wimpy);
	} else {
		sprintf(buf,
				"Hit: %d  Dam: %d  Saves: %d  Size: %s  Position: %s  Wimpy: %d\n\r",
				GET_HITROLL(victim), GET_DAMROLL(victim), victim->saving_throw,
				size_table[victim->size].name,
				position_table[victim->position].name, victim->wimpy);
	}
	send_to_char(buf, ch);

	if (!IS_NPC(victim) && is_clan(victim)) {
		sprintf(buf,
				"Kills: Lw: %d Eq: %d Gr: %d Deaths: %d Stolen: (%ld/%ld) Slices: %ld\n\r",
				victim->pcdata->killer_data[PC_LOWER_KILLS],
				victim->pcdata->killer_data[PC_EQUAL_KILLS],
				victim->pcdata->killer_data[PC_GREATER_KILLS],
				victim->pcdata->killer_data[PC_DEATHS],
				victim->pcdata->steal_data[PC_STOLEN_ITEMS],
				victim->pcdata->steal_data[PC_STOLEN_GOLD],
				victim->pcdata->steal_data[PC_SLICES]);
		send_to_char(buf, ch);
		sprintf(buf, "Outcast: %d  Ruffian: %d\n\r", victim->pcdata->outcT,
				victim->pcdata->ruffT);
		send_to_char(buf, ch);
	}

	if (!IS_NPC(victim) && IS_SET(victim->mhs, MHS_HIGHLANDER)) {
		sprintf(buf, "Highlander kills: %d  Real Kills: %d\n\r",
				victim->pcdata->highlander_data[ALL_KILLS],
				victim->pcdata->highlander_data[REAL_KILLS]);
		send_to_char(buf, ch);
	}
	if (!IS_NPC(victim)) {
		sprintf(buf,
				"Single Events: %d  Victories: %d  Kills: %d  Team Events: %d  Victories: %d  Kills: %d\n\r",
				victim->pcdata->gladiator_data[GLADIATOR_PLAYS],
				victim->pcdata->gladiator_data[GLADIATOR_VICTORIES],
				victim->pcdata->gladiator_data[GLADIATOR_KILLS],
				victim->pcdata->gladiator_data[GLADIATOR_TEAM_PLAYS],
				victim->pcdata->gladiator_data[GLADIATOR_TEAM_VICTORIES],
				victim->pcdata->gladiator_data[GLADIATOR_TEAM_KILLS]);
		send_to_char(buf, ch);
	}

	if (IS_NPC(victim) && victim->pIndexData->new_format) {
		sprintf(buf, "Damage: %dd%d  Message:  %s\n\r",
				victim->damage[DICE_NUMBER], victim->damage[DICE_TYPE],
				attack_table[victim->dam_type].noun);
		send_to_char(buf, ch);
	}
	sprintf(buf, "Fighting: %s  Riding: %s  Passenger: %s\n\r",
			victim->fighting ? victim->fighting->name : "(none)",
			victim->riding ? victim->riding->name : "(none)",
			victim->passenger ? victim->passenger->name : "(none)");
	if (!IS_NPC(victim)) {
		sprintf(buf2,
				"Remort: %s  Deity: %s (%d / %d)  Skill Points: (%d / %d / %d)\n\r",
				IS_SET(victim->act, PLR_WERE) ? "garou" :
				IS_SET(victim->act, PLR_MUMMY) ? "mummy" :
				IS_SET(victim->act, PLR_VAMP) ? "nosferatu" : "(none)",
				deity_table[victim->pcdata->deity].pname, victim->pcdata->sac,
				(int) (((victim->played + (int) (current_time - victim->logon))
						- victim->pcdata->switched) / 3600),
				victim->skill_points, victim->pcdata->skill_point_tracker,
				victim->pcdata->skill_point_timer);
		strcat(buf, buf2);
	} else {
		strcat(buf, "\n\r");
	}
	send_to_char(buf, ch);

	if (!IS_NPC(victim)) {
		sprintf(buf,
				"Thirst: %d  Hunger: %d  Full: %d  Drunk: %d   Casting Level: %d\n\r",
				victim->pcdata->condition[COND_THIRST],
				victim->pcdata->condition[COND_HUNGER],
				victim->pcdata->condition[COND_FULL],
				victim->pcdata->condition[COND_DRUNK],
				compute_casting_level(victim, 0));
		send_to_char(buf, ch);
//       if (victim->pcdata->logout_tracker > 0)
//       {
		sprintf(buf, "PFresh Logouts Remaining: %d\n\r",
				victim->pcdata->logout_tracker);
//  send_to_char( buf, ch );
//       }
	}

	sprintf(buf, "Carry number: %d  Carry weight: %ld   Kit: %s\n\r",
			victim->carry_number, get_carry_weight(victim) / 10,
			victim->kit ? kit_table[victim->kit].name : "none");
	send_to_char(buf, ch);

	/*
	 (int) (victim->played + victim->redid + current_time - victim->logon) / 3600,
	 */
	if (!IS_NPC(victim)) {
		sprintf(buf,
				"Age: %d  Played: %d  Last Level: %d  Timer: %d  Barb: %d  Mutate: %d\n\r",
				get_age(victim),
				(int) (victim->played + current_time - victim->logon) / 3600,
				victim->pcdata->last_level, victim->timer,
				victim->pcdata->barbarian, victim->pcdata->mutant_timer);
		send_to_char(buf, ch);
	}

	sprintf(buf, "MHS: %s\n\r", mhs_bit_name(victim->mhs));
	send_to_char(buf, ch);

	if (!IS_NPC(victim) && is_clan(victim)) {
		sprintf(buf, "Clan Flags: %s\n\r",
				clan_bit_name(victim->pcdata->clan_flags));
		send_to_char(buf, ch);
	}

	if (!IS_NPC(victim)) {
		sprintf(buf, " Matook: %d\n\r", (int) (victim->pcdata->matookT));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Act: %s\n\r", act_bit_name(victim->act));
	send_to_char(buf, ch);

	if (victim->comm) {
		sprintf(buf, "Comm: %s\n\r", comm_bit_name(victim->comm));
		send_to_char(buf, ch);
	}

	if (IS_NPC(victim) && victim->off_flags) {
		sprintf(buf, "Offense: %s\n\r", off_bit_name(victim->off_flags));
		send_to_char(buf, ch);
	}

	if (victim->imm_flags) {
		sprintf(buf, "Immune: %s\n\r", imm_bit_name(victim->imm_flags));
		send_to_char(buf, ch);
	}

	if (victim->res_flags) {
		sprintf(buf, "Resist: %s\n\r", imm_bit_name(victim->res_flags));
		send_to_char(buf, ch);
	}

	if (victim->vuln_flags) {
		sprintf(buf, "Vulnerable: %s\n\r", imm_bit_name(victim->vuln_flags));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Form: %s\n\rParts: %s\n\r", form_bit_name(victim->form),
			part_bit_name(victim->parts));
	send_to_char(buf, ch);

	if (victim->affected_by) {
		sprintf(buf, "Affected by %s\n\r",
				affect_bit_name(victim->affected_by));
		send_to_char(buf, ch);
	}

	sprintf(buf, "Master: %s  Leader: %s  Pet: %s MOB-lAb: %s\n\r",
			victim->master ? victim->master->name : "(none)",
			victim->leader ? victim->leader->name : "(none)",
			victim->pet ? victim->pet->name : "(none)",
			victim->lAb ? victim->lAb->name : "(none)");
	send_to_char(buf, ch);

	sprintf(buf, "People: %s  Next In Room: %s\n\r",
			victim->in_room->people ? victim->in_room->people->name : "(none)",
			victim->next_in_room ? victim->next_in_room->name : "(none)");
	send_to_char(buf, ch);

	if (!IS_NPC(victim)) {
		sprintf(buf,
				"Fighting: %s  Last Attacked By: %s  LAB Timer: %d Last Death Timer: %d  Trumps: %d\n\r",
				victim->fighting ? victim->fighting->name : "(none)",
				victim->pcdata->last_attacked_by,
				victim->pcdata->last_attacked_by_timer,
				victim->pcdata->last_death_timer, victim->trumps);
		send_to_char(buf, ch);
		strcpy(strtime, ctime(&victim->pcdata->last_combat_date));
		strtime[strlen(strtime) - 1] = '\0';
		sprintf(buf,
				"Login Information:\n\r"
						"Combats Since: %d Without Combats: %d Without Kill: %d Without Death: %d\n\r"
						"Last Combat Date: %s\n\r",
				victim->pcdata->combats_since_last_login,
				victim->pcdata->logins_without_combat,
				victim->pcdata->logins_without_kill,
				victim->pcdata->logins_without_death, strtime);
		send_to_char(buf, ch);
		strcpy(strtime, ctime(&victim->pcdata->last_kill_date));
		strtime[strlen(strtime) - 1] = '\0';
		sprintf(buf, "Last Kill Date: %s\n\r", strtime);
		send_to_char(buf, ch);
		strcpy(strtime, ctime(&victim->pcdata->last_death_date));
		strtime[strlen(strtime) - 1] = '\0';
		sprintf(buf, "Last Death Date: %s\n\r", strtime);
		send_to_char(buf, ch);
	}

	sprintf(buf, "Short description: %s\n\rLong  description: %s\n\r",
			victim->short_descr,
			victim->long_descr[0] != '\0' ? victim->long_descr : "(none)");
	send_to_char(buf, ch);

	if ( IS_NPC(victim) && victim->spec_fun != 0) {
		sprintf(buf,
				"Mobile has special procedure %s.\n\rWords[0]: %s\n\rWords[1]: %s\n\rWords[2]: %s\n\r",
				spec_name(victim->spec_fun), victim->pIndexData->spec_words[0],
				victim->pIndexData->spec_words[1],
				victim->pIndexData->spec_words[2]);
		send_to_char(buf, ch);
	}

	if (victim->qnum || victim->qnum2 || victim->qchar) {
		if (victim->qchar)
			sprintf(buf, "QValues: %s character, %d val1, %d val2\n\r",
					victim->qchar->name, victim->qnum, victim->qnum2);
		else
			sprintf(buf, "QValues: <null> character, %d val1, %d val2\n\r",
					victim->qnum, victim->qnum2);
		send_to_char(buf, ch);
	}

	if (!IS_NPC(victim)) {
		int i;
		char buf2[256];
		sprintf(buf, "Participating in %d NPC quests.\n\r",
				victim->pcdata->quest_count);
		send_to_char(buf, ch);
		sprintf(buf, "Quest wins: %d", victim->pcdata->quest_wins[0]);
		for (i = 1; i < QUEST_COUNT; i++) {
			sprintf(buf2, "%d ", victim->pcdata->quest_wins[i]);
			strcat(buf, buf2);
		}
		strcat(buf, "\n\r");
		send_to_char(buf, ch);
	}

	for (paf = victim->affected; paf != NULL; paf = paf->next) {
		sprintf(buf,
				"Spell: '%s' modifies %s by %d for %d hours with bits %s, level %d.\n\r",
				skill_table[(int) paf->type].name,
				affect_loc_name(paf->location), paf->modifier, paf->duration,
				affect_bit_name(paf->bitvector), paf->level);
		send_to_char(buf, ch);
	}
	if (IS_SET(victim->act, PLR_FREEZE))
		send_to_char("{RPlayer has been frozen{x.\n\r", ch);
	if (IS_SET(victim->act, PLR_DENY))
		send_to_char("{RPlayer has been denied{x.\n\r", ch);
	return;
}

/* ofind and mfind replaced with vnum, vnum skill also added */

void do_vnum(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char *string;
	int vnum;

	string = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  vnum obj <name>\n\r", ch);
		send_to_char("  vnum mob <name>\n\r", ch);
		send_to_char("  vnum skill <skill or spell>\n\r", ch);
		send_to_char("  vnum #\n\r", ch);
		return;
	}

	if (is_number(arg)) {
		MOB_INDEX_DATA *pMobIndex;
		OBJ_INDEX_DATA *pObjIndex;
		char buf[MAX_STRING_LENGTH];

		vnum = atoi(arg);

		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
			sprintf(buf, "[%5d] %s\n\r", pMobIndex->vnum,
					pMobIndex->short_descr);
			send_to_char(buf, ch);
		} else {
			send_to_char("No mobs with that vnum.\n\r", ch);
		}

		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			sprintf(buf, "[%5d] %s\n\r", pObjIndex->vnum,
					pObjIndex->short_descr);
			send_to_char(buf, ch);
		} else {
			send_to_char("No objects with that vnum.\n\r", ch);
		}

		return;
	}

	if (!str_cmp(arg, "obj")) {
		do_ofind(ch, string);
		return;
	}

	if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) {
		do_mfind(ch, string);
		return;
	}

	if (!str_cmp(arg, "skill") || !str_cmp(arg, "spell")) {
		do_slookup(ch, string);
		return;
	}
	/* do both */
	do_mfind(ch, argument);
	do_ofind(ch, argument);
}

void do_mfind(CHAR_DATA *ch, char *argument) {
	extern int top_mob_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Find whom?\n\r", ch);
		return;
	}

	fAll = FALSE; /* !str_cmp( arg, "all" ); */
	found = FALSE;
	nMatch = 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_mob_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for (vnum = 0; nMatch < top_mob_index; vnum++) {
		if ((pMobIndex = get_mob_index(vnum)) != NULL) {
			nMatch++;
			if (fAll || is_name(argument, pMobIndex->player_name)) {
				found = TRUE;
				sprintf(buf, "[%5d] %s\n\r", pMobIndex->vnum,
						pMobIndex->short_descr);
				send_to_char(buf, ch);
			}
		}
	}

	if (!found)
		send_to_char("No mobiles by that name.\n\r", ch);

	return;
}

void do_ofind(CHAR_DATA *ch, char *argument) {
	extern int top_obj_index;
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	int vnum;
	int nMatch;
	bool fAll;
	bool found;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Find what?\n\r", ch);
		return;
	}

	fAll = FALSE; /* !str_cmp( arg, "all" ); */
	found = FALSE;
	nMatch = 0;

	/*
	 * Yeah, so iterating over all vnum's takes 10,000 loops.
	 * Get_obj_index is fast, and I don't feel like threading another link.
	 * Do you?
	 * -- Furey
	 */
	for (vnum = 0; nMatch < top_obj_index; vnum++) {
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			nMatch++;
			if (fAll || is_name(argument, pObjIndex->name)) {
				found = TRUE;
				sprintf(buf, "[%5d] %s\n\r", pObjIndex->vnum,
						pObjIndex->short_descr);
				send_to_char(buf, ch);
			}
		}
	}

	if (!found)
		send_to_char("No objects by that name.\n\r", ch);

	return;
}

void do_owhere(CHAR_DATA *ch, char *argument) {
	char buf[MAX_INPUT_LENGTH];
	BUFFER *buffer;
	OBJ_DATA *obj;
	OBJ_DATA *in_obj;
	bool found;
	int number = 0, max_found;

	found = FALSE;
	number = 0;
	max_found = 200;

	buffer = new_buf();

	if (argument[0] == '\0') {
		send_to_char("Find what?\n\r", ch);
		return;
	}

	for (obj = object_list; obj != NULL; obj = obj->next) {
		if (!can_see_obj(ch, obj) || !is_name(argument, obj->name)
				|| ch->level < obj->level)
			continue;

		for (in_obj = obj; in_obj->in_obj != NULL; in_obj = in_obj->in_obj)
			;

		if (in_obj->carried_by != NULL && in_obj->carried_by->in_room != NULL) {
			if (!can_see(ch, in_obj->carried_by, TRUE))
				continue; // Don't report on imms you can't see
			sprintf(buf, "%3d) %s is carried by %s [Room %d]\n\r", number,
					obj->short_descr, PERS(in_obj->carried_by, ch, TRUE),
					in_obj->carried_by->in_room->vnum);
		} else if (in_obj->in_room != NULL && can_see_room(ch, in_obj->in_room))
			sprintf(buf, "%3d) %s is in %s [Room %d]\n\r", number,
					obj->short_descr, in_obj->in_room->name,
					in_obj->in_room->vnum);
		else
			sprintf(buf, "%3d) %s is somewhere\n\r", number, obj->short_descr);

		buf[0] = UPPER(buf[0]);
		add_buf(buffer, buf);

		found = TRUE;
		number++;

		if (number >= max_found)
			break;
	}

	if (!found)
		send_to_char("Nothing like that in heaven or earth.\n\r", ch);
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);
}

void do_mwhere(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	BUFFER *buffer;
	CHAR_DATA *victim;
	bool found;
	int count = 0;

	if (argument[0] == '\0') {
		DESCRIPTOR_DATA *d;

		/* show characters logged */

		buffer = new_buf();
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->character != NULL && d->connected == CON_PLAYING
					&& d->character->in_room != NULL
					&& can_see(ch, d->character, TRUE)
					&& can_see_room(ch, d->character->in_room)) {
				victim = d->character;
				count++;
				if (d->original != NULL)
					sprintf(buf,
							"%3d) %s (in the body of %s) is in %s [%d]\n\r",
							count, d->original->name, victim->short_descr,
							victim->in_room->name, victim->in_room->vnum);
				else
					sprintf(buf, "%3d) %s is in %s [%d]\n\r", count,
							victim->name, victim->in_room->name,
							victim->in_room->vnum);
				add_buf(buffer, buf);
			}
		}

		page_to_char(buf_string(buffer), ch);
		free_buf(buffer);
		return;
	}

	found = FALSE;
	buffer = new_buf();
	for (victim = char_list; victim != NULL; victim = victim->next) {
		if (victim->in_room != NULL && is_name(argument, victim->name)
				&& can_see(ch, victim, TRUE)) {
			found = TRUE;
			count++;
			sprintf(buf, "%3d) [%5d] %-28s [%5d] %s\n\r", count,
			IS_NPC(victim) ? victim->pIndexData->vnum : 0,
			IS_NPC(victim) ? victim->short_descr : victim->name,
					victim->in_room->vnum, victim->in_room->name);
			add_buf(buffer, buf);
		}
	}

	if (!found)
		act("You didn't find any $T.", ch, NULL, argument, TO_CHAR, FALSE);
	else
		page_to_char(buf_string(buffer), ch);

	free_buf(buffer);

	return;
}

void do_reboo(CHAR_DATA *ch, char *argument) {
	send_to_char("If you want to REBOOT, spell it out.\n\r", ch);
	return;
}

void do_reboot(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH]; /* Poquah - For Note Check */
	extern bool merc_down;
	DESCRIPTOR_DATA *d, *d_next;

	/* Poquah Down - Check for Players writing notes */
	one_argument(argument, arg);

	if (str_cmp(arg, "force")) {
		bool fAny = FALSE;
		for (d = descriptor_list; d != NULL; d = d->next)
			if (d->connected == CON_PLAYING && d->character->pnote != NULL) {
				fAny = TRUE;
				sprintf(buf, "%s is currently working on a note.\n\r",
						d->character->name);
				send_to_char(buf, ch);
			}
		if (fAny) {
			send_to_char("Use 'reboot force' to reboot anyway.\n\r", ch);
			return;
		}
	}
	/* Poquah Up */
#ifdef OLC_VERSION
	if(ch->icg == ICG_BUILD)
	return;
#endif
	if (ch->invis_level < LEVEL_HERO) {
		sprintf(buf, "Reboot by %s.", ch->name);
		do_echo(ch, buf);
	}
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->character)
			edit_stop(d->character);
	}
	do_force(ch, "all save");
	do_save(ch, "");
	save_pits();
	merc_down = TRUE;
	for (d = descriptor_list; d != NULL; d = d_next) {
		d_next = d->next;
		close_socket(d);
	}

	return;
}

void do_shutdow(CHAR_DATA *ch, char *argument) {
	send_to_char("If you want to SHUTDOWN, spell it out.\n\r", ch);
	return;
}

void do_shutdown(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	extern bool merc_down;
	DESCRIPTOR_DATA *d, *d_next;

#ifdef OLC_VERSION
	if(ch->icg == ICG_BUILD)
	return;
#endif

	if (ch->invis_level < LEVEL_HERO)
		sprintf(buf, "Shutdown by %s.", ch->name);
	append_file(ch, SHUTDOWN_FILE, buf);
	strcat(buf, "\n\r");
	if (ch->invis_level < LEVEL_HERO)
		do_echo(ch, buf);
	for (d = descriptor_list; d != NULL; d = d->next) {
		if (d->character)
			edit_stop(d->character);
	}
	do_force(ch, "all save");
	do_save(ch, "");
	save_pits();
	merc_down = TRUE;
	for (d = descriptor_list; d != NULL; d = d_next) {
		d_next = d->next;
		close_socket(d);
	}
	return;
}

void do_protect(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *victim;

	if (argument[0] == '\0') {
		send_to_char("Protect whom from snooping?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("You can't find them.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_SNOOP_PROOF)) {
		act_new("$N is no longer snoop-proof.", ch, NULL, victim, TO_CHAR,
				POS_DEAD, TRUE);
		send_to_char("Your snoop-proofing was just removed.\n\r", victim);
		REMOVE_BIT(victim->comm, COMM_SNOOP_PROOF);
	} else {
		act_new("$N is now snoop-proof.", ch, NULL, victim, TO_CHAR, POS_DEAD,
				TRUE);
		send_to_char("You are now immune to snooping.\n\r", victim);
		SET_BIT(victim->comm, COMM_SNOOP_PROOF);
	}
}

void do_snoop(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	DESCRIPTOR_DATA *d;
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Snoop whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc == NULL) {
		send_to_char("No descriptor to snoop.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Cancelling all snoops.\n\r", ch);
		wiznet("$N stops being such a snoop.", ch, NULL, WIZ_SNOOPS, WIZ_SECURE,
				get_trust(ch));
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->snoop_by == ch->desc)
				d->snoop_by = NULL;
		}
		return;
	}

	if (victim->desc->snoop_by != NULL) {
		send_to_char("Busy already.\n\r", ch);
		return;
	}
	/*
	 if (!is_room_owner(ch,victim->in_room) &&  !IS_TRUSTED(ch,IMPLEMENTOR))
	 {
	 send_to_char("That character is in a private room.\n\r",ch);
	 return;
	 }
	 */
	if (get_trust(victim) >= get_trust(ch)
			|| (IS_SET(victim->comm, COMM_SNOOP_PROOF)
					&& !IS_TRUSTED(ch, IMPLEMENTOR))) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (ch->desc != NULL) {
		for (d = ch->desc->snoop_by; d != NULL; d = d->snoop_by) {
			if (d->character == victim || d->original == victim) {
				send_to_char("No snoop loops.\n\r", ch);
				return;
			}
		}
	}

	victim->desc->snoop_by = ch->desc;
	sprintf(buf, "$N starts snooping on %s",
			(IS_NPC(ch) ? victim->short_descr : victim->name));
	wiznet(buf, ch, NULL, WIZ_SNOOPS, WIZ_SECURE, get_trust(ch));
	send_to_char("Ok.\n\r", ch);
	return;
}

void do_switch(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Switch into whom?\n\r", ch);
		return;
	}

	if (ch->desc == NULL)
		return;

	if (ch->desc->original != NULL) {
		send_to_char("You are already switched.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Ok.\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {
		send_to_char("You can only switch into mobiles.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, victim->in_room) && ch->in_room != victim->in_room
	&& room_is_private(ch,victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
		send_to_char("That character is in a private room.\n\r", ch);
		return;
	}

	if (victim->desc != NULL) {
		send_to_char("Character in use.\n\r", ch);
		return;
	}

	sprintf(buf, "$N switches into %s", victim->short_descr);
	wiznet(buf, ch, NULL, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));

	ch->desc->character = victim;
	ch->desc->original = ch;
	victim->desc = ch->desc;
	ch->desc = NULL;
	/* change communications to match */
	if (ch->prompt != NULL) {
		free_string(victim->prompt);
		victim->prompt = str_dup(ch->prompt);
	}
	victim->comm = ch->comm;
	victim->lines = ch->lines;
	send_to_char("Ok.\n\r", victim);
	return;
}

void do_return(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];

	if (ch->desc == NULL)
		return;

	if (ch->desc->original == NULL) {
		send_to_char("You aren't switched.\n\r", ch);
		return;
	}

	send_to_char(
			"You return to your original body. Type replay to see any missed tells.\n\r",
			ch);
	if (ch->prompt != NULL) {
		free_string(ch->prompt);
		ch->prompt = NULL;
	}

	sprintf(buf, "$N returns from %s.", ch->short_descr);
	wiznet(buf, ch->desc->original, 0, WIZ_SWITCHES, WIZ_SECURE, get_trust(ch));
	ch->desc->character = ch->desc->original;
	ch->desc->original = NULL;
	ch->desc->character->desc = ch->desc;
	ch->desc = NULL;
	return;
}

/* trust levels for load and clone */
bool obj_check(CHAR_DATA *ch, OBJ_DATA *obj) {
	if (IS_TRUSTED(ch, GOD)
			|| (IS_TRUSTED(ch,IMMORTAL) && obj->level <= 20 && obj->cost <= 1000)
			|| (IS_TRUSTED(ch,DEMI) && obj->level <= 10 && obj->cost <= 500)
			|| (IS_TRUSTED(ch,ANGEL) && obj->level <= 5 && obj->cost <= 250)
			|| (IS_TRUSTED(ch,AVATAR) && obj->level == 0 && obj->cost <= 100))
		return TRUE;
	else
		return FALSE;
}

/* for clone, to insure that cloning goes many levels deep */
void recursive_clone(CHAR_DATA *ch, OBJ_DATA *obj, OBJ_DATA *clone) {
	OBJ_DATA *c_obj, *t_obj;

	for (c_obj = obj->contains; c_obj != NULL; c_obj = c_obj->next_content) {
		if (obj_check(ch, c_obj)) {
			t_obj = create_object(c_obj->pIndexData, 0, FALSE);
			clone_object(c_obj, t_obj);
			obj_to_obj(t_obj, clone);
			recursive_clone(ch, c_obj, t_obj);
		}
	}
}

/* command that is similar to load */
void do_clone(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	char *rest;
	CHAR_DATA *mob;
	OBJ_DATA *obj;
	int count, number;

	rest = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Clone what?\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "object")) {
		mob = NULL;
		number = mult_argument(rest, arg);
		obj = get_obj_here(ch, arg);
		if (obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
		obj = NULL;
		number = mult_argument(rest, arg);
		mob = get_char_room(ch, arg);
		if (mob == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	} else /* find both */
	{
		number = mult_argument(argument, arg);
		mob = get_char_room(ch, arg);
		obj = get_obj_here(ch, arg);
		if (mob == NULL && obj == NULL) {
			send_to_char("You don't see that here.\n\r", ch);
			return;
		}
	}

	/* clone an object */
	if (obj != NULL) {
		OBJ_DATA *clone;

		clone = NULL;
		if (!obj_check(ch, obj)) {
			send_to_char(
					"Your powers are not great enough for such a task.\n\r",
					ch);
			return;
		}

		if (number > 50) {
			send_to_char("Whoa.. not more than 50.\n\r", ch);
			return;
		}

		if (number <= 0) {
			send_to_char("Did you mistype that number before the '*'?\n\r", ch);
			return;
		}

		for (count = 0; count < number; count++) {
			clone = create_object(obj->pIndexData, 0, FALSE);
			clone_object(obj, clone);
			if (obj->carried_by != NULL)
				obj_to_char(clone, ch);
			else
				obj_to_room(clone, ch->in_room);
			recursive_clone(ch, obj, clone);
		}

		sprintf(buf, "$n has created [%d]$p.", number);
		act(buf, ch, clone, NULL, TO_ROOM, FALSE);
		sprintf(buf, "You clone [%d]$p.", number);
		act(buf, ch, clone, NULL, TO_CHAR, FALSE);
		sprintf(buf, "$N clones [%d]$p.", number);
		wiznet(buf, ch, clone, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
		return;
	} else if (mob != NULL) {
		CHAR_DATA *clone;
		OBJ_DATA *new_obj;

		clone = NULL;
		if (!IS_NPC(mob)) {
			send_to_char("You can only clone mobiles.\n\r", ch);
			return;
		}

		if ((mob->level > 20 && !IS_TRUSTED(ch, GOD))
				|| (mob->level > 10 && !IS_TRUSTED(ch, IMMORTAL))
				|| (mob->level > 5 && !IS_TRUSTED(ch, DEMI))
				|| (mob->level > 0 && !IS_TRUSTED(ch, ANGEL))
				|| !IS_TRUSTED(ch, AVATAR)) {
			send_to_char(
					"Your powers are not great enough for such a task.\n\r",
					ch);
			return;
		}

		if (number > 50) {
			send_to_char("Whoa.. not more than 50.\n\r", ch);
			return;
		}

		for (count = 0; count < number; count++) {
			clone = create_mobile(mob->pIndexData);
			clone_mobile(mob, clone);

			for (obj = mob->carrying; obj != NULL; obj = obj->next_content) {
				if (obj_check(ch, obj)) {
					new_obj = create_object(obj->pIndexData, 0, FALSE);
					clone_object(obj, new_obj);
					recursive_clone(ch, obj, new_obj);
					obj_to_char(new_obj, clone);
					new_obj->wear_loc = obj->wear_loc;
				}
			}
			char_to_room(clone, ch->in_room);
		}

		sprintf(buf, "$n has created [%d]$N.", number);
		act(buf, ch, NULL, clone, TO_ROOM, FALSE);
		sprintf(buf, "You clone [%d]$N.", number);
		act(buf, ch, NULL, clone, TO_CHAR, FALSE);
		sprintf(buf, "$N clones [%d]%s.", number, clone->short_descr);
		wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
		return;
	}
}

/* RT to replace the two load commands */

void do_load(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  load mob <vnum>\n\r", ch);
		send_to_char("  load obj <vnum> <level>\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "mob") || !str_cmp(arg, "char")) {
		do_mload(ch, argument);
		return;
	}

	if (!str_cmp(arg, "obj")) {
		do_oload(ch, argument);
		return;
	}
	/* echo syntax */
	do_load(ch, "");
}

void do_mload(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	MOB_INDEX_DATA *pMobIndex;
	CHAR_DATA *victim;
	char buf[MAX_STRING_LENGTH];

	one_argument(argument, arg);

	if (arg[0] == '\0' || !is_number(arg)) {
		send_to_char("Syntax: load mob <vnum>.\n\r", ch);
		return;
	}

	if ((pMobIndex = get_mob_index(atoi(arg))) == NULL) {
		send_to_char("No mob has that vnum.\n\r", ch);
		return;
	}

	victim = create_mobile(pMobIndex);
	char_to_room(victim, ch->in_room);
	act("$n has created $N!", ch, NULL, victim, TO_ROOM, FALSE);
	sprintf(buf, "$N loads %s.", victim->short_descr);
	wiznet(buf, ch, NULL, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
	send_to_char("Ok.\n\r", ch);
	return;
}

void do_oload(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
	OBJ_INDEX_DATA *pObjIndex;
	OBJ_DATA *obj;
	int level;

	argument = one_argument(argument, arg1);
	one_argument(argument, arg2);

	if (arg1[0] == '\0' || !is_number(arg1)) {
		send_to_char("Syntax: load obj <vnum> <level>.\n\r", ch);
		return;
	}

	level = get_trust(ch); /* default */

	if (arg2[0] != '\0') /* load with a level */
	{
		if (!is_number(arg2)) {
			send_to_char("Syntax: oload <vnum> <level>.\n\r", ch);
			return;
		}
		level = atoi(arg2);
		if (level < 0 || level > get_trust(ch)) {
			send_to_char("Level must be be between 0 and your level.\n\r", ch);
			return;
		}
	}

	if ((pObjIndex = get_obj_index(atoi(arg1))) == NULL) {
		send_to_char("No object has that vnum.\n\r", ch);
		return;
	}

	obj = create_object(pObjIndex, level, FALSE);
	if (CAN_WEAR(obj, ITEM_TAKE))
		obj_to_char(obj, ch);
	else
		obj_to_room(obj, ch->in_room);
	act("$n has created $p!", ch, obj, NULL, TO_ROOM, FALSE);
	wiznet("$N loads $p.", ch, obj, WIZ_LOAD, WIZ_SECURE, get_trust(ch));
	send_to_char("Ok.\n\r", ch);
	return;
}

void do_pur(CHAR_DATA *ch, char *argument) {
	if (ch->level >= AVATAR)
		send_to_char("Was that 'purr' or 'purge'? Spell it out.\n\r", ch);
	return;
}

void do_purge(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char buf[100];
	CHAR_DATA *victim;
	OBJ_DATA *obj;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		/* 'purge' */
		CHAR_DATA *vnext;
		OBJ_DATA *obj_next;

		for (victim = ch->in_room->people; victim != NULL; victim = vnext) {
			vnext = victim->next_in_room;
			if ( IS_NPC(victim) && !IS_SET(victim->act, ACT_NOPURGE)
					&& victim != ch /* safety precaution */)
				extract_char(victim, TRUE);
		}

		for (obj = ch->in_room->contents; obj != NULL; obj = obj_next) {
			obj_next = obj->next_content;
			if (!IS_OBJ_STAT(obj, ITEM_NOPURGE))
				extract_obj(obj);
		}

		act("$n purges the room!", ch, NULL, NULL, TO_ROOM, FALSE);
		send_to_char("Ok.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (!IS_NPC(victim)) {

		if (ch == victim) {
			send_to_char("Ho ho ho.\n\r", ch);
			return;
		}

		if (get_trust(ch) <= get_trust(victim)) {
			send_to_char("Maybe that wasn't a good idea...\n\r", ch);
			sprintf(buf, "%s tried to purge you!\n\r", ch->name);
			send_to_char(buf, victim);
			return;
		}

		act("$n disintegrates $N.", ch, 0, victim, TO_NOTVICT, FALSE);

		if (victim->level > 1)
			save_char_obj(victim);
		d = victim->desc;
		extract_char(victim, TRUE);
		if (d != NULL)
			close_socket(d);

		return;
	}

	act("$n purges $N.", ch, NULL, victim, TO_NOTVICT, FALSE);
	extract_char(victim, TRUE);
	return;
}

void do_advance(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int level;
	int iLevel;

#ifdef OLC_VERSION
	if(ch->icg == ICG_BUILD)
	{
		send_to_char("Very funny buttmunch.\n\r",ch);
		return;
	}
#endif

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		send_to_char("Syntax: advance <char> <level>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("That player is not here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if ((level = atoi(arg2)) < 1 || level > MAX_LEVEL) {
		sprintf(buf, "Level must be 1 to %d.\n\r", MAX_LEVEL);
		send_to_char(buf, ch);
		return;
	}

	if (level > get_trust(ch)) {
		send_to_char("Limited to your trust level.\n\r", ch);
		return;
	}

	if (ch->level == victim->level) {
		send_to_char("Now now, play nicely.\n\r", ch);
		return;
	}

	/*
	 * Lower level:
	 *   Reset to level 1.
	 *   Then raise again.
	 *   Currently, an imp can lower another imp.
	 *   -- Swiftest
	 */
	if (level <= victim->level) {
		int temp_prac;

		send_to_char("Lowering a player's level!\n\r", ch);
		send_to_char("**** OOOOHHHHHHHHHH  NNNNOOOO ****\n\r", victim);
		temp_prac = victim->practice;
		victim->level = 1;
		victim->exp = exp_per_level(victim, victim->pcdata->points);
		victim->max_hit = 10;
		victim->max_mana = 100;
		victim->max_move = 100;
		victim->practice = 0;
		victim->pcdata->perm_hit = victim->hit = victim->max_hit;
		victim->pcdata->perm_mana = victim->mana = victim->max_mana;
		victim->pcdata->perm_move = victim->move = victim->max_move;
		victim->icg = ICG_NONE;
		victim->icg_bits = 0;
		victim->wiznet = 0;
		victim->incog_level = 0;
		victim->invis_level = 0;
		if (IS_SET(victim->act, PLR_HOLYLIGHT))
			REMOVE_BIT(victim->act, PLR_HOLYLIGHT);
		advance_level(victim);
		victim->practice = temp_prac;
	} else {
		send_to_char("Raising a player's level!\n\r", ch);
		send_to_char("**** OOOOHHHHHHHHHH  YYYYEEEESSS ****\n\r", victim);
	}

	for (iLevel = victim->level; iLevel < level; iLevel++) {
		send_to_char("You raise a level!!  ", victim);
		victim->level += 1;
		advance_level(victim);
	}
	victim->exp = exp_per_level(victim,
			victim->pcdata->points) * UMAX( 1, victim->level );
	victim->trust = 0;
	save_char_obj(victim);
	return;
}

void do_trust(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	int level;

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (arg1[0] == '\0' || arg2[0] == '\0' || !is_number(arg2)) {
		send_to_char("Syntax: trust <char> <level>.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("That player is not here.\n\r", ch);
		return;
	}

	if ((level = atoi(arg2)) < 0 || level > MAX_LEVEL) {
		sprintf(buf, "Level must be 0 (reset) or 1 to %d.\n\r", MAX_LEVEL);
		send_to_char(buf, ch);
		return;
	}

	if (level > get_trust(ch)) {
		send_to_char("Limited to your trust.\n\r", ch);
		return;
	}

	victim->trust = level;
	return;
}

void do_restore(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;
	CHAR_DATA *vch;
	DESCRIPTOR_DATA *d;

	one_argument(argument, arg);
	if (arg[0] == '\0' || !str_cmp(arg, "room")) {
		/* cure room */

		for (vch = ch->in_room->people; vch != NULL; vch = vch->next_in_room) {
			affect_strip(vch, gsn_plague);
			affect_strip(vch, gsn_poison);
			affect_strip(vch, gsn_blindness);
			affect_strip(vch, gsn_sleep);
			affect_strip(vch, gsn_curse);

			vch->hit = vch->max_hit;
			vch->mana = vch->max_mana;
			vch->move = vch->max_move;
			update_pos(vch);
			act("$n has restored you.", ch, NULL, vch, TO_VICT, FALSE);
		}

		sprintf(buf, "$N restored room %d.", ch->in_room->vnum);
		wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));

		send_to_char("Room restored.\n\r", ch);
		return;

	}

	if ((get_trust(ch) >= 58 || override != 0)
			&& (!str_cmp(arg, "nonclan") || !str_cmp(arg, "all"))) {
		/* cure all */

		for (d = descriptor_list; d != NULL; d = d->next) {
			victim = d->character;

			if (victim == NULL || IS_NPC(victim)
					|| (victim->pcdata && victim->pcdata->quit_time > 0))
				continue;

			if (!str_cmp(arg, "nonclan") && is_clan(victim))
				continue;

			affect_strip(victim, gsn_plague);
			affect_strip(victim, gsn_poison);
			affect_strip(victim, gsn_blindness);
			affect_strip(victim, gsn_sleep);
			affect_strip(victim, gsn_curse);

			victim->hit = victim->max_hit;
			victim->mana = victim->max_mana;
			victim->move = victim->max_move;
			victim->pcdata->sac =
					(victim->class == class_lookup("paladin")) ? 600 : 300;
			if (victim->class == class_lookup("crusader")) {
				victim->pcdata->sac = 400;
			}

			if (HAS_KIT(victim, "bishop"))
				victim->pcdata->sac += 100;

			update_pos(victim);
			if (victim->in_room != NULL)
				act("$n has restored you.", ch, NULL, victim, TO_VICT, FALSE);
		}

		if (!str_cmp(arg, "nonclan"))
			send_to_char("All active nonclanners restored.\n\r", ch);
		else
			send_to_char("All active players restored.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	affect_strip(victim, gsn_plague);
	affect_strip(victim, gsn_poison);
	affect_strip(victim, gsn_blindness);
	affect_strip(victim, gsn_sleep);
	affect_strip(victim, gsn_curse);
	victim->hit = victim->max_hit;
	victim->mana = victim->max_mana;
	victim->move = victim->max_move;
	update_pos(victim);
	act("$n has restored you.", ch, NULL, victim, TO_VICT, FALSE);
	sprintf(buf, "$N restored %s",
	IS_NPC(victim) ? victim->short_descr : victim->name);
	wiznet(buf, ch, NULL, WIZ_RESTORE, WIZ_SECURE, get_trust(ch));
	send_to_char("Ok.\n\r", ch);
	return;
}

void do_freeze(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Freeze whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->act, PLR_FREEZE)) {
		REMOVE_BIT(victim->act, PLR_FREEZE);
		send_to_char("You can play again.\n\r", victim);
		send_to_char("FREEZE removed.\n\r", ch);
		sprintf(buf, "$N thaws %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->act, PLR_FREEZE);
		send_to_char("You can't do ANYthing!\n\r", victim);
		send_to_char("FREEZE set.\n\r", ch);
		sprintf(buf, "$N puts %s in the deep freeze.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	save_char_obj(victim);

	return;
}

void do_log(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Log whom?\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		if (fLogAll) {
			fLogAll = FALSE;
			send_to_char("Log ALL off.\n\r", ch);
		} else {
			fLogAll = TRUE;
			send_to_char("Log ALL on.\n\r", ch);
		}
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	/*
	 * No level check, gods can log anyone.
	 */
	if (IS_SET(victim->act, PLR_LOG)) {
		REMOVE_BIT(victim->act, PLR_LOG);
		send_to_char("LOG removed.\n\r", ch);
	} else {
		SET_BIT(victim->act, PLR_LOG);
		send_to_char("LOG set.\n\r", ch);
	}

	return;
}

void do_noemote(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Noemote whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOEMOTE)) {
		REMOVE_BIT(victim->comm, COMM_NOEMOTE);
		send_to_char("You can emote again.\n\r", victim);
		send_to_char("NOEMOTE removed.\n\r", ch);
		sprintf(buf, "$N restores emotes to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOEMOTE);
		send_to_char("You can't emote!\n\r", victim);
		send_to_char("NOEMOTE set.\n\r", ch);
		sprintf(buf, "$N revokes %s's emotes.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

void do_noshout(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Noshout whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOSHOUT)) {
		REMOVE_BIT(victim->comm, COMM_NOSHOUT);
		send_to_char("You can shout again.\n\r", victim);
		send_to_char("NOSHOUT removed.\n\r", ch);
		sprintf(buf, "$N restores shouts to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOSHOUT);
		send_to_char("You can't shout!\n\r", victim);
		send_to_char("NOSHOUT set.\n\r", ch);
		sprintf(buf, "$N revokes %s's shouts.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

void do_notel(CHAR_DATA *ch, char *argument) {
	send_to_char("You must type the full command to notell someone.\n\r", ch);
	return;
}

void do_notitle(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Notitle whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("Not on-line.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPCs.\n\r", ch);
		return;
	}

	if (get_trust(victim) > get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOTITLE)) {
		REMOVE_BIT(victim->comm, COMM_NOTITLE);
		sprintf(buf, "$N restores %s's title.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
		send_to_char("Notitle removed.\n\r", ch);
		return;
	} else {
		SET_BIT(victim->comm, COMM_NOTITLE);
		sprintf(buf, "$N disables %s's title.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
		send_to_char("Notitles set.\n\r", ch);
		return;
	}

	bug("End of functino: do_notitle", 0);
	return;
}

void do_lag(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	argument = one_argument(argument, arg);
	one_argument(argument, arg2);

	if (arg[0] == '\0' || arg2[0] == '\0') {
		do_help(ch, "lag");
		return;
	}

	if (!is_number(arg2)) {
		send_to_char("Second argument msut be numeric.\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("That person isn't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) > get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	WAIT_STATE(victim, atoi(arg2) * 4);
	send_to_char("The lag beast strikes!\n\r", ch);
	return;
}

void do_notell(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Notell whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (get_trust(victim) >= get_trust(ch)) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (IS_SET(victim->comm, COMM_NOTELL)) {
		REMOVE_BIT(victim->comm, COMM_NOTELL);
		send_to_char("You can tell again.\n\r", victim);
		send_to_char("NOTELL removed.\n\r", ch);
		sprintf(buf, "$N restores tells to %s.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	} else {
		SET_BIT(victim->comm, COMM_NOTELL);
		send_to_char("You can't tell!\n\r", victim);
		send_to_char("NOTELL set.\n\r", ch);
		sprintf(buf, "$N revokes %s's tells.", victim->name);
		wiznet(buf, ch, NULL, WIZ_PENALTIES, WIZ_SECURE, 0);
	}

	return;
}

void do_peace(CHAR_DATA *ch, char *argument) {
	CHAR_DATA *rch;

	for (rch = ch->in_room->people; rch != NULL; rch = rch->next_in_room) {
		if (rch->fighting != NULL)
			stop_fighting(rch, TRUE);
		if (IS_NPC(rch) && IS_SET(rch->act, ACT_AGGRESSIVE))
			REMOVE_BIT(rch->act, ACT_AGGRESSIVE);
	}

	send_to_char("Ok.\n\r", ch);
	return;
}

void do_wizlock(CHAR_DATA *ch, char *argument) {
	extern bool wizlock;
	wizlock = !wizlock;

	if (wizlock) {
		wiznet("$N has wizlocked the game.", ch, NULL, 0, 0, 0);
		send_to_char("Game wizlocked.\n\r", ch);
	} else {
		wiznet("$N removes wizlock.", ch, NULL, 0, 0, 0);
		send_to_char("Game un-wizlocked.\n\r", ch);
	}

	return;
}

/* RT anti-newbie code */

void do_newlock(CHAR_DATA *ch, char *argument) {
	extern bool newlock;
	newlock = !newlock;

	if (newlock) {
		wiznet("$N locks out new characters.", ch, NULL, 0, 0, 0);
		send_to_char("New characters have been locked out.\n\r", ch);
	} else {
		wiznet("$N allows new characters back in.", ch, NULL, 0, 0, 0);
		send_to_char("Newlock removed.\n\r", ch);
	}

	return;
}

void do_slookup(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	int sn;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Lookup which skill or spell?\n\r", ch);
		return;
	}

	if (!str_cmp(arg, "all")) {
		for (sn = 0; sn < MAX_SKILL; sn++) {
			if (skill_table[sn].name == NULL)
				break;
			sprintf(buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn,
					skill_table[sn].slot, skill_table[sn].name);
			send_to_char(buf, ch);
		}
	} else {
		if ((sn = skill_lookup(arg)) < 0) {
			send_to_char("No such skill or spell.\n\r", ch);
			return;
		}

		sprintf(buf, "Sn: %3d  Slot: %3d  Skill/spell: '%s'\n\r", sn,
				skill_table[sn].slot, skill_table[sn].name);
		send_to_char(buf, ch);
	}

	return;
}

/* RT set replaces sset, mset, oset, and rset */

void do_set(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	/*    if (ch->level < 58)
	 {
	 send_to_char("You must be at least level 58 to use set.\n\r",ch);
	 return;
	 }*/

	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  set mob   <name> <field> <value>\n\r", ch);
		send_to_char("  set obj   <name> <field> <value>\n\r", ch);
		send_to_char("  set room  <room> <field> <value>\n\r", ch);
		send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
		return;
	}

	if (!str_prefix(arg, "mobile") || !str_prefix(arg, "character")) {
		do_mset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "skill") || !str_prefix(arg, "spell")) {
		do_sset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "object")) {
		do_oset(ch, argument);
		return;
	}

	if (!str_prefix(arg, "room")) {
		do_rset(ch, argument);
		return;
	}
	/* echo syntax */
	do_set(ch, "");
}

void do_sset(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	int value;
	int sn;
	bool fAll;

	if (ch->level < MAX_LEVEL - 2 && ch->icg != ICG_ADMIN) {
		send_to_char("You are not permitted to set skills.\n\r", ch);
		return;
	}

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	argument = one_argument(argument, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  set skill <name> <spell or skill> <value>\n\r", ch);
		send_to_char("  set skill <name> all <value>\n\r", ch);
		send_to_char("   (use the name of the skill, not the number)\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Not on NPC's.\n\r", ch);
		return;
	}

	fAll = !str_cmp(arg2, "all");
	sn = 0;
	if (!fAll && (sn = skill_lookup(arg2)) < 0) {
		send_to_char("No such skill or spell.\n\r", ch);
		return;
	}
	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3)) {
		send_to_char("Value must be numeric.\n\r", ch);
		return;
	}

	value = atoi(arg3);
	if (value < 0 || value > 100) {
		send_to_char("Value range is 0 to 100.\n\r", ch);
		return;
	}

	if (fAll) {
		for (sn = 0; sn < MAX_SKILL; sn++) {
			if (skill_table[sn].name != NULL)
				victim->pcdata->learned[sn] = value;
		}
	} else {
		victim->pcdata->learned[sn] = value;
	}

	return;
}

void do_mset(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char buf[100];
	CHAR_DATA *victim;
	int value;

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  set char <name> <field> <value>\n\r", ch);
		send_to_char("  Field being one of:\n\r", ch);
		send_to_char("    str int wis dex con cha\n\r", ch);
		send_to_char("    surname sex class level sac race\n\r", ch);
		send_to_char("    group gold silver hp mana move prac logins\n\r", ch);
		send_to_char("    align train thirst hunger drunk full bounty\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg1)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	/* clear zones for mobs */
	victim->zone = NULL;

	/*
	 * Snarf the value (which need not be numeric).
	 */
	value = is_number(arg3) ? atoi(arg3) : -1;

	/*
	 * Set something.
	 */

	if (!str_cmp(arg2, "surname")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on an NPC.\n\r", ch);
			return;
		}

		if (strlen(arg3) > 12) {
			send_to_char("Name too long.\n\r", ch);
			return;
		}

		if (!str_cmp(arg3, "clear")) {
			if (victim->pcdata->surname != NULL)
				free_string(victim->pcdata->surname);
			victim->pcdata->surname = NULL;
			send_to_char("Surname cleared.\n\r", ch);
			return;
		} else {
			if (victim->pcdata->surname != NULL)
				free_string(victim->pcdata->surname);
			victim->pcdata->surname = str_dup(arg3);
			send_to_char("Surname set.\n\r", ch);
			return;
		}
	}

	if (!str_cmp(arg2, "logins")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on an NPC.\n\r", ch);
			return;
		}
		if (value < 0) {
			send_to_char("Value must be more than 0.\n\r", ch);
			return;
		}
		victim->pcdata->logins_without_combat = value;
		victim->pcdata->logins_without_death = value;
		victim->pcdata->logins_without_kill = value;
		send_to_char("Logins have been cleared.\n\r", ch);
		return;

	}

	if (!str_cmp(arg2, "bounty")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on an NPC.\n\r", ch);
			return;
		}

		if (value < 0) {
			send_to_char("Value must be more than 0.\n\r", ch);
			return;
		}

		victim->pcdata->bounty = value;
		send_to_char("Bounty set.\n\r", ch);
		return;
	}

	if (!str_cmp(arg2, "mhs")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on an NPC.\n\r", ch);
			return;
		}

		if (IS_SET(victim->mhs, MHS_OLD_RECLASS)) {
			REMOVE_BIT(victim->mhs, MHS_OLD_RECLASS);
			send_to_char("MHS bit removed.\n\r", ch);
			return;
		} else {
			SET_BIT(victim->mhs, MHS_OLD_RECLASS);
			send_to_char("MHS bit set.\n\r", ch);
			return;
		}
	}

	if (!str_cmp(arg2, "board")) {
		if (get_trust(ch) < 59) {
			send_to_char("Only 59's can set the board flag.\n\r", ch);
			return;
		}

		if (IS_NPC(victim)) {
			send_to_char("Not on an NPC.\n\r", ch);
			return;
		}

		if (IS_SET(victim->mhs, MHS_ADVISORY_BOARD)) {
			REMOVE_BIT(victim->mhs, MHS_ADVISORY_BOARD);
			send_to_char("Board bit removed.\n\r", ch);
			return;
		} else {
			SET_BIT(victim->mhs, MHS_ADVISORY_BOARD);
			send_to_char("Board bit set.\n\r", ch);
			return;
		}
	}

	if (!str_cmp(arg2, "hours")) {
		if (value < 1) {
			send_to_char("You may not set a player to less than 1 hour.\n", ch);
			return;
		}
		victim->played = value * 3600;
		send_to_char("Ok.\n", ch);
		return;
	}

	if (!str_cmp(arg2, "str")) {
		if (value < 3 || value > get_max_train(victim, STAT_STR)) {
			sprintf(buf, "Strength range is 3 to %d\n\r.",
					get_max_train(victim, STAT_STR));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_STR] = value;
		return;
	}

	if (!str_cmp(arg2, "int")) {
		if (value < 3 || value > get_max_train(victim, STAT_INT)) {
			sprintf(buf, "Intelligence range is 3 to %d.\n\r",
					get_max_train(victim, STAT_INT));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_INT] = value;
		return;
	}

	if (!str_cmp(arg2, "wis")) {
		if (value < 3 || value > get_max_train(victim, STAT_WIS)) {
			sprintf(buf, "Wisdom range is 3 to %d.\n\r",
					get_max_train(victim, STAT_WIS));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_WIS] = value;
		return;
	}

	if (!str_cmp(arg2, "dex")) {
		if (value < 3 || value > get_max_train(victim, STAT_DEX)) {
			sprintf(buf, "Dexterity range is 3 to %d.\n\r",
					get_max_train(victim, STAT_DEX));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_DEX] = value;
		return;
	}

	if (!str_cmp(arg2, "con")) {
		if (value < 3 || value > get_max_train(victim, STAT_CON)) {
			sprintf(buf, "Constitution range is 3 to %d.\n\r",
					get_max_train(victim, STAT_CON));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_CON] = value;
		return;
	}

	/*    if ( !str_cmp( arg2, "agt" ) )
	 {
	 if ( value < 3 || value > get_max_train(victim,STAT_AGT) )
	 {
	 sprintf(buf,
	 "Agility range is 3 to %d\n\r.",
	 get_max_train(victim,STAT_AGT));
	 send_to_char(buf,ch);
	 return;
	 }

	 victim->perm_stat[STAT_AGT] = value;
	 return;
	 }

	 if ( !str_cmp( arg2, "end" ) )
	 {
	 if ( value < 3 || value > get_max_train(victim,STAT_END) )
	 {
	 sprintf(buf,
	 "Endurance range is 3 to %d\n\r.",
	 get_max_train(victim,STAT_END));
	 send_to_char(buf,ch);
	 return;
	 }

	 victim->perm_stat[STAT_END] = value;
	 return;
	 }
	 */

	if (!str_cmp(arg2, "cha")) {
		if (value < 3 || value > get_max_train(victim, STAT_SOC)) {
			sprintf(buf, "Charisma range is 3 to %d\n\r.",
					get_max_train(victim, STAT_SOC));
			send_to_char(buf, ch);
			return;
		}

		victim->perm_stat[STAT_SOC] = value;
		return;
	}

	if (!str_prefix(arg2, "sac")) {
		if (value < 0 || value > 300) {
			send_to_char("Sac range is 0 to 300.\n\r", ch);
			return;
		}
		victim->pcdata->sac = value;
		return;
	}

	if (!str_prefix(arg2, "sex")) {
		if (value < 0 || value > 2) {
			send_to_char("Sex range is 0 to 2.\n\r", ch);
			return;
		}
		victim->sex = value;
		if (!IS_NPC(victim))
			victim->pcdata->true_sex = value;
		return;
	}

	if (!str_prefix(arg2, "oldclass")) {
		int class;

		if (IS_NPC(victim)) {
			send_to_char("Mobs don't have any class.\n\r", ch);
			return;
		}

		if ((class = class_lookup(arg3)) == -1) {
			send_to_char("That's not a class.\n\r", ch);
			return;
		}

		if (class_table[class].reclass) {
			send_to_char("OLD CLASS, brain child.  Old class.\n\r", ch);
			return;
		}

		victim->pcdata->old_class = class;
		return;

	}

	if (!str_prefix(arg2, "class")) {
		int class;

		if (IS_NPC(victim)) {
			send_to_char("Mobiles have no class.\n\r", ch);
			return;
		}

		class = class_lookup(arg3);
		if (class == -1) {
			char buf[MAX_STRING_LENGTH];

			strcpy(buf, "Possible classes are: ");
			for (class = 0; class < MAX_CLASS; class++) {
				if (class > 0)
					strcat(buf, " ");
				strcat(buf, class_table[class].name);
			}
			strcat(buf, ".\n\r");

			send_to_char(buf, ch);
			return;
		}

		victim->class = class;
		return;
	}

	if (!str_prefix(arg2, "level")) {
		if (!IS_NPC(victim)) {
			send_to_char("Not on PC's.\n\r", ch);
			return;
		}

		if (value < 0 || value > MAX_LEVEL) {
			sprintf(buf, "Level range is 0 to %d.\n\r", MAX_LEVEL);
			send_to_char(buf, ch);
			return;
		}
		victim->level = value;
		return;
	}

	if (!str_prefix(arg2, "gold")) {
		victim->gold = value;
		return;
	}

	if (!str_prefix(arg2, "silver")) {
		victim->silver = value;
		return;
	}

	if (!str_prefix(arg2, "hp")) {
		if (value < -10 || value > 30000) {
			send_to_char("Hp range is -10 to 30,000 hit points.\n\r", ch);
			return;
		}
		victim->max_hit = value;
		if (!IS_NPC(victim))
			victim->pcdata->perm_hit = value;
		return;
	}

	if (!str_prefix(arg2, "mana")) {
		if (value < 0 || value > 30000) {
			send_to_char("Mana range is 0 to 30,000 mana points.\n\r", ch);
			return;
		}
		victim->max_mana = value;
		if (!IS_NPC(victim))
			victim->pcdata->perm_mana = value;
		return;
	}

	if (!str_prefix(arg2, "move")) {
		if (value < 0 || value > 30000) {
			send_to_char("Move range is 0 to 30,000 move points.\n\r", ch);
			return;
		}
		victim->max_move = value;
		if (!IS_NPC(victim))
			victim->pcdata->perm_move = value;
		return;
	}

	if (!str_prefix(arg2, "practice")) {
		if (value < 0 || value > 250) {
			send_to_char("Practice range is 0 to 250 sessions.\n\r", ch);
			return;
		}
		victim->practice = value;
		return;
	}

	if (!str_prefix(arg2, "train")) {
		if (value < 0 || value > 50) {
			send_to_char("Training session range is 0 to 50 sessions.\n\r", ch);
			return;
		}
		victim->train = value;
		return;
	}

	if (!str_prefix(arg2, "align")) {
		if (value < -1000 || value > 1000) {
			send_to_char("Alignment range is -1000 to 1000.\n\r", ch);
			return;
		}
		victim->alignment = value;
		return;
	}

	if (!str_prefix(arg2, "thirst")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (value < -1 || value > 100) {
			send_to_char("Thirst range is -1 to 100.\n\r", ch);
			return;
		}

		victim->pcdata->condition[COND_THIRST] = value;
		return;
	}

	if (!str_prefix(arg2, "drunk")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (value < -1 || value > 100) {
			send_to_char("Drunk range is -1 to 100.\n\r", ch);
			return;
		}

		victim->pcdata->condition[COND_DRUNK] = value;
		return;
	}

	if (!str_prefix(arg2, "full")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (value < -1 || value > 100) {
			send_to_char("Full range is -1 to 100.\n\r", ch);
			return;
		}

		victim->pcdata->condition[COND_FULL] = value;
		return;
	}

	if (!str_prefix(arg2, "hunger")) {
		if (IS_NPC(victim)) {
			send_to_char("Not on NPC's.\n\r", ch);
			return;
		}

		if (value < -1 || value > 100) {
			send_to_char("Full range is -1 to 100.\n\r", ch);
			return;
		}

		victim->pcdata->condition[COND_HUNGER] = value;
		return;
	}

	if (!str_prefix(arg2, "race")) {
		int race;

		race = race_lookup(arg3);

		if (race == 0) {
			send_to_char("That is not a valid race.\n\r", ch);
			return;
		}

		if (!IS_NPC(victim) && !race_table[race].pc_race) {
			send_to_char("That is not a valid player race.\n\r", ch);
			return;
		}

		victim->race = race;
		victim->size = pc_race_table[race].size;
		return;
	}

	if (!str_prefix(arg2, "group")) {
		if (!IS_NPC(victim)) {
			send_to_char("Only on NPCs.\n\r", ch);
			return;
		}
		victim->group = value;
		return;
	}

	/*
	 * Generate usage message.
	 */
	do_mset(ch, "");
	return;
}

void do_string(CHAR_DATA *ch, char *argument) {
	char type[MAX_INPUT_LENGTH];
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;
	OBJ_DATA *obj;

	smash_tilde(argument);
	argument = one_argument(argument, type);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (type[0] == '\0' || arg1[0] == '\0' || arg2[0] == '\0'
			|| arg3[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  string char <name> <field> <string>\n\r", ch);
		send_to_char("    fields: name short long desc title surname spec\n\r",
				ch);
		send_to_char("  string obj  <name> <field> <string>\n\r", ch);
		send_to_char("    fields: name short long extended\n\r", ch);
		return;
	}

	if (!str_prefix(type, "character") || !str_prefix(type, "mobile")) {
		if ((victim = get_char_world(ch, arg1)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		/* clear zone for mobs */
		victim->zone = NULL;

		/* string something */

		if (!str_prefix(arg2, "name")) {
			if (!IS_NPC(victim)) {
				send_to_char("Not on PC's.\n\r", ch);
				return;
			}
			free_string(victim->name);
			victim->name = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "description")) {
			free_string(victim->description);
			victim->description = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "short")) {
			free_string(victim->short_descr);
			victim->short_descr = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "long")) {
			free_string(victim->long_descr);
			strcat(arg3, "\n\r");
			victim->long_descr = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "title")) {
			if (IS_NPC(victim)) {
				send_to_char("Not on NPC's.\n\r", ch);
				return;
			}

			set_title(victim, arg3);
			return;
		}

		if (!str_prefix(arg2, "spec")) {
			if (!IS_NPC(victim)) {
				send_to_char("Not on PC's.\n\r", ch);
				return;
			}

			if ((victim->spec_fun = spec_lookup(arg3)) == 0) {
				send_to_char("No such spec fun.\n\r", ch);
				return;
			}
		}

		if (!str_prefix(arg2, "surname")) {
			if (IS_NPC(victim)) {
				send_to_char("Not on an NPC.\n\r", ch);
				return;
			}

			if (strlen(arg3) > 12) {
				send_to_char("Name too long.\n\r", ch);
				return;
			}

			if (!str_cmp(arg3, "clear")) {
				if (victim->pcdata->surname != NULL)
					free_string(victim->pcdata->surname);
				victim->pcdata->surname = NULL;
				send_to_char("Surname cleared.\n\r", ch);
				return;
			} else {
				if (victim->pcdata->surname != NULL)
					free_string(victim->pcdata->surname);
				victim->pcdata->surname = str_dup(arg3);
				send_to_char("Surname set.\n\r", ch);
				return;
			}
		}

		return;
	}

	if (!str_prefix(type, "object")) {
		/* string an obj */

		if ((obj = get_obj_world(ch, arg1)) == NULL) {
			send_to_char("Nothing like that in heaven or earth.\n\r", ch);
			return;
		}

		if (!str_prefix(arg2, "name")) {
			free_string(obj->name);
			obj->name = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "short")) {
			free_string(obj->short_descr);
			obj->short_descr = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "long")) {
			free_string(obj->description);
			obj->description = str_dup(arg3);
			return;
		}

		if (!str_prefix(arg2, "ed") || !str_prefix(arg2, "extended")) {
			EXTRA_DESCR_DATA *ed;

			argument = one_argument(argument, arg3);
			if (argument == NULL) {
				send_to_char("Syntax: oset <object> ed <keyword> <string>\n\r",
						ch);
				return;
			}

			strcat(argument, "\n\r");

			ed = new_extra_descr();

			ed->keyword = str_dup(arg3);
			ed->description = str_dup(argument);
			ed->next = obj->extra_descr;
			obj->extra_descr = ed;
			return;
		}
	}

	/* echo bad use message */
	do_string(ch, "");
}

void do_oset(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	OBJ_DATA *obj;
	int value;

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  set obj <object> <field> <value>\n\r", ch);
		send_to_char("  Field being one of:\n\r", ch);
		send_to_char("    value0 value1 value2 value3 value4 (v1-v4)\n\r", ch);
		send_to_char("    extra wear level weight cost timer\n\r", ch);
		return;
	}

	if ((obj = get_obj_world(ch, arg1)) == NULL) {
		send_to_char("Nothing like that in heaven or earth.\n\r", ch);
		return;
	}

	/*
	 * Snarf the value (which need not be numeric).
	 */
	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_cmp(arg2, "value0") || !str_cmp(arg2, "v0")) {
		if (obj->item_type != ITEM_CAPSULE)
			obj->value[0] = UMIN(50, value);
		else
			obj->value[0] = value;
		set_rarity(obj);
		return;
	}

	if (!str_cmp(arg2, "value1") || !str_cmp(arg2, "v1")) {
		obj->value[1] = value;
		set_rarity(obj);
		return;
	}

	if (!str_cmp(arg2, "value2") || !str_cmp(arg2, "v2")) {
		obj->value[2] = value;
		set_rarity(obj);
		return;
	}

	if (!str_cmp(arg2, "value3") || !str_cmp(arg2, "v3")) {
		obj->value[3] = value;
		set_rarity(obj);
		return;
	}

	if (!str_cmp(arg2, "value4") || !str_cmp(arg2, "v4")) {
		obj->value[4] = value;
		set_rarity(obj);
		return;
	}

	if (!str_prefix(arg2, "extra")) {
		obj->extra_flags = value;
		set_rarity(obj);
		return;
	}

	if (!str_prefix(arg2, "wear")) {
		obj->wear_flags = value;
		return;
	}

	if (!str_prefix(arg2, "level")) {
		obj->level = value;
		return;
	}

	if (!str_prefix(arg2, "weight")) {
		obj->weight = value;
		return;
	}

	if (!str_prefix(arg2, "cost")) {
		obj->cost = value;
		return;
	}

	if (!str_prefix(arg2, "timer")) {
		obj->timer = value;
		return;
	}

	/*
	 * Generate usage message.
	 */
	do_oset(ch, "");
	return;
}

void do_rset(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	ROOM_INDEX_DATA *location;
	int value;

	smash_tilde(argument);
	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);
	strcpy(arg3, argument);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  set room <location> <field> <value>\n\r", ch);
		send_to_char("  Field being one of:\n\r", ch);
		send_to_char("    flags sector\n\r", ch);
		return;
	}

	if ((location = find_location(ch, arg1)) == NULL) {
		send_to_char("No such location.\n\r", ch);
		return;
	}

	if (!is_room_owner(ch, location) && ch->in_room != location
	&& room_is_private(ch,location) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
		send_to_char("That room is private right now.\n\r", ch);
		return;
	}

	/*
	 * Snarf the value.
	 */
	if (!is_number(arg3)) {
		send_to_char("Value must be numeric.\n\r", ch);
		return;
	}
	value = atoi(arg3);

	/*
	 * Set something.
	 */
	if (!str_prefix(arg2, "flags")) {
		location->room_flags = value;
		return;
	}

	if (!str_prefix(arg2, "sector")) {
		location->sector_type = value;
		return;
	}

	/*
	 * Generate usage message.
	 */
	do_rset(ch, "");
	return;
}

/*  removed by Nightdagger, 1/18/03, don't need it
 void do_socke( CHAR_DATA *ch, char *argument )
 {
 send_to_char("You must use the full command to check sockets.\n\r",ch);
 }
 */

void do_sockets(CHAR_DATA *ch, char *argument) {
	char buf[4 * MAX_STRING_LENGTH];
	char buf2[MAX_STRING_LENGTH];
	char cdbuf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	char *name;
	DESCRIPTOR_DATA *d;
	int count;
	bool suffix, prefix, disp;
	static char *connect_table[] = { "[ 0] Playing", "[ 1] Get Name",
			"[ 2] Get Old Pass", "[ 3] Confirm Name", "[ 4] Get New Pass",
			"[ 5] Confirm Pass", "[ 6] Get New Race", "[ 7] Get New Sex",
			"[ 8] Get New Class", "[ 9] Get Alignment", "[10] Customize?",
			"[11] Gen Groups", "[12] Pick Weapon", "[13] Read IMOTD",
			"[14] Read MOTD", "[15] Break Connect", "[16] Line Editor",
			"[17] Pick Stats", "[18] Login Menu", "[19] Creation",
			"[20] P-Refresh Character", "[21] Smurf Password",
			"[22} Pick Surname", "" };

	count = 0;
	buf[0] = '\0';
	buf2[0] = '\0';

	one_argument(argument, arg);
	if (arg[0] == '@') {
		name = &arg[1];
		prefix = suffix = FALSE;
		if (name[0] == '*') {
			prefix = TRUE;
			name++;
		}
		if (name[strlen(name) - 1] == '*') {
			suffix = TRUE;
			name[strlen(name) - 1] = '\0';
		}

		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->character != NULL && can_see(ch, d->character, TRUE)) {
				disp = FALSE;
				if ((prefix && suffix) && strstr(d->host, name) != NULL)
					disp = TRUE;
				if (prefix && !str_suffix(name, d->host))
					disp = TRUE;
				if (suffix && !str_prefix(name, d->host))
					disp = TRUE;
				if (!suffix && !prefix && !str_cmp(name, d->host))
					disp = TRUE;
				if (disp) {
					count++;
					sprintf(buf2, "[%2d] %-18s %s@%s:%d\n\r", d->descriptor,
							connect_table[d->connected],
							d->original ? d->original->name :
							d->character ? d->character->name : "(none)",
							d->host, d->port);
					strcat(buf, buf2);
				}
			}
		}
	} else {
		for (d = descriptor_list; d != NULL; d = d->next) {
			if (d->character == NULL) {
				cdbuf[0] = '\0';

				if (d->descriptor != NULL && d->connected != NULL
						&& d->host != NULL && d->port != NULL) {
					sprintf(buf2, "[%2d] Nobody yet @%s:%d\r\n", d->descriptor,
							d->host, d->port);
					strcat(buf, buf2);
				} else {
					sprintf(cdbuf,
							"SOCKET ERROR Desc: %2d Conn: %-18s Host: %s Port: %d",
							(d->descriptor != NULL) ? d->descriptor : 0,
							(d->connected != NULL) ?
									connect_table[d->connected] : "NULL",
							(d->host != NULL) ? d->host : "NULL",
							(d->port != NULL) ? d->port : 0);
					log_string(cdbuf);
				}

			} else if (d->character != NULL && can_see(ch, d->character, TRUE)
					&& (arg[0] == '\0' || is_name(arg, d->character->name)
							|| (d->original && is_name(arg, d->original->name)))) {
				count++;
				sprintf(buf2, "[%2d] %-18s %-15s %s:%d\n\r", d->descriptor,
						connect_table[d->connected],
						d->original ? d->original->name :
						d->character ? d->character->name : "(none)", d->host,
						d->port);
				strcat(buf, buf2);
			}
		}
	}
	if (count == 0) {
		send_to_char("No one by that name is connected.\n\r", ch);
		return;
	}

	sprintf(buf2, "%d user%s\n\r", count, count == 1 ? "" : "s");
	strcat(buf, buf2);
	page_to_char(buf, ch);
	return;
}

/*
 void do_sockets( CHAR_DATA *ch, char *argument )
 {
 char buf[2 * MAX_STRING_LENGTH];
 char buf2[MAX_STRING_LENGTH];
 char arg[MAX_INPUT_LENGTH];
 DESCRIPTOR_DATA *d;
 int count;

 count = 0;
 buf[0]  = '\0';

 one_argument(argument,arg);
 for ( d = descriptor_list; d != NULL; d = d->next )
 {
 if ( d->character != NULL && can_see( ch, d->character, TRUE )
 && (arg[0] == '\0' || is_name(arg,d->character->name)
 || (d->original && is_name(arg,d->original->name))))
 {
 count++;
 sprintf( buf + strlen(buf), "[%3d %2d] %s@%s\n\r",
 d->descriptor,
 d->connected,
 d->original  ? d->original->name  :
 d->character ? d->character->name : "(none)",
 d->host
 );
 }
 }
 if (count == 0)
 {
 send_to_char("No one by that name is connected.\n\r",ch);
 return;
 }

 sprintf( buf2, "%d user%s\n\r", count, count == 1 ? "" : "s" );
 strcat(buf,buf2);
 page_to_char( buf, ch );
 return;
 }
 */

/*
 * Thanks to Grodyn for pointing out bugs in this function.
 */
void do_force(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	char arg[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg);

	if (arg[0] == '\0' || argument[0] == '\0') {
		send_to_char("Force whom to do what?\n\r", ch);
		return;
	}

	one_argument(argument, arg2);

	if (!str_cmp(arg2, "delete")) {
		send_to_char("That will NOT be done.\n\r", ch);
		return;
	}

	sprintf(buf, "$n forces you to '%s'.", argument);

	if (!str_cmp(arg, "all")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 3) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust(vch) < get_trust(ch)) {
				act(buf, ch, NULL, vch, TO_VICT, FALSE);
				interpret(vch, argument);
			}
		}
	} else if (!str_cmp(arg, "players")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
			&& vch->level < LEVEL_HERO) {
				act(buf, ch, NULL, vch, TO_VICT, FALSE);
				interpret(vch, argument);
			}
		}
	} else if (!str_cmp(arg, "gods")) {
		CHAR_DATA *vch;
		CHAR_DATA *vch_next;

		if (get_trust(ch) < MAX_LEVEL - 2) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		for (vch = char_list; vch != NULL; vch = vch_next) {
			vch_next = vch->next;

			if (!IS_NPC(vch) && get_trust( vch ) < get_trust( ch )
			&& vch->level >= LEVEL_HERO) {
				act(buf, ch, NULL, vch, TO_VICT, FALSE);
				interpret(vch, argument);
			}
		}
	} else {
		CHAR_DATA *victim;

		if ((victim = get_char_world(ch, arg)) == NULL) {
			send_to_char("They aren't here.\n\r", ch);
			return;
		}

		if (victim == ch) {
			send_to_char("Aye aye, right away!\n\r", ch);
			return;
		}

		if (!is_room_owner(ch,
				victim->in_room) && ch->in_room != victim->in_room
				&& room_is_private(ch,victim->in_room) && !IS_TRUSTED(ch,IMPLEMENTOR)) {
			send_to_char("That character is in a private room.\n\r", ch);
			return;
		}

		if (get_trust(victim) >= get_trust(ch)) {
			send_to_char("Do it yourself!\n\r", ch);
			return;
		}

		if (!IS_NPC(victim) && get_trust(ch) < MAX_LEVEL - 3) {
			send_to_char("Not at your level!\n\r", ch);
			return;
		}

		act(buf, ch, NULL, victim, TO_VICT, FALSE);
		interpret(victim, argument);
	}

	send_to_char("Ok.\n\r", ch);
	return;
}

/*
 * New routines by Dionysos.
 */
void do_invis(CHAR_DATA *ch, char *argument) {
	int level;
	char arg[MAX_STRING_LENGTH];

	/* RT code for taking a level argument */
	one_argument(argument, arg);

	if (arg[0] == '\0')
		/* take the default path */

		if (ch->invis_level) {
			ch->invis_level = 0;
			act("$n slowly fades into existence.", ch, NULL, NULL, TO_ROOM,
					FALSE);
			send_to_char("You slowly fade back into existence.\n\r", ch);
		} else {
			ch->invis_level = get_trust(ch);
			act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM,
					FALSE);
			sprintf(arg, "You slowly vanish into thin air at level %d.\n\r",
					ch->invis_level);
			send_to_char(arg, ch);
		}
	else
	/* do the level thing */
	{
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch)) {
			send_to_char("Invis level must be between 2 and your level.\n\r",
					ch);
			return;
		} else {
			ch->reply = NULL;
			ch->invis_level = level;
			act("$n slowly fades into thin air.", ch, NULL, NULL, TO_ROOM,
					FALSE);
			sprintf(arg, "You slowly vanish into thin air at level %d.\n\r",
					ch->invis_level);
			send_to_char(arg, ch);
		}
	}

	return;
}

void do_incognito(CHAR_DATA *ch, char *argument) {
	int level;
	char arg[MAX_STRING_LENGTH];

	/* RT code for taking a level argument */
	one_argument(argument, arg);

	if (arg[0] == '\0')
		/* take the default path */

		if (ch->incog_level) {
			ch->incog_level = 0;
			act("$n is no longer cloaked.", ch, NULL, NULL, TO_ROOM, FALSE);
			send_to_char("You are no longer cloaked.\n\r", ch);
		} else {
			ch->incog_level = get_trust(ch);
			act("$n cloaks $s presence", ch, NULL, NULL, TO_ROOM, FALSE);
			send_to_char("You cloak your presence.\n\r", ch);
		}
	else
	/* do the level thing */
	{
		level = atoi(arg);
		if (level < 2 || level > get_trust(ch)) {
			send_to_char("Incog level must be between 2 and your level.\n\r",
					ch);
			return;
		} else {
			ch->reply = NULL;
			ch->incog_level = level;
			act("$n cloaks $s presence.", ch, NULL, NULL, TO_ROOM, FALSE);
			send_to_char("You cloak your presence.\n\r", ch);
		}
	}

	return;
}

void do_dispvnum(CHAR_DATA *ch, char *argument) {
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->display, DISP_DISP_VNUM)) {
		send_to_char("Vnums will no longer be displayed.\n\r", ch);
		REMOVE_BIT(ch->display, DISP_DISP_VNUM);
	} else {
		send_to_char("Vnums will be displayed.\n\r", ch);
		SET_BIT(ch->display, DISP_DISP_VNUM);
	}
}

void do_holylight(CHAR_DATA *ch, char *argument) {
	if (IS_NPC(ch))
		return;

	if (IS_SET(ch->act, PLR_HOLYLIGHT)) {
		REMOVE_BIT(ch->act, PLR_HOLYLIGHT);
		send_to_char("Holy light mode off.\n\r", ch);
	} else {
		SET_BIT(ch->act, PLR_HOLYLIGHT);
		send_to_char("Holy light mode on.\n\r", ch);
	}

	return;
}

void do_explode(CHAR_DATA *ch, char *argument) {
	char arg[MAX_STRING_LENGTH];
	CHAR_DATA *victim, *next_vict;
	OBJ_DATA *obj, *obj_content, *obj_next;
	ROOM_INDEX_DATA *pRoomIndex;
	int min, max, count = 0, spin, num, retry;

	argument = one_argument(argument, arg);

	if (arg[0] == '\0') {
		send_to_char("Syntax:\n\r", ch);
		send_to_char("  explode room\n\r", ch);
		send_to_char("  explode <name of container> <mob_min_level> "
				"<mob_max_level>\n\r", ch);
		return;
	}

	if (!strcmp(arg, "room")) {
		victim = ch->in_room->people;
		while (victim) {
			next_vict = victim->next_in_room;
			if (IS_NPC(victim)) {
				pRoomIndex = get_random_room(victim);
				act("$n vanishes!", victim, NULL, NULL, TO_ROOM, FALSE);
				char_from_room(victim);
				char_to_room(victim, pRoomIndex);
				clear_mount(victim);
				act("$n slowly fades into existence.", victim, NULL,
				NULL, TO_ROOM, FALSE);
			}
			victim = next_vict;
		}
		return;
	}

	obj = get_obj_list(ch, arg, ch->carrying);
	if (obj == NULL) {
		send_to_char("Item not found.\n\r", ch);
		return;
	}
	if (obj->item_type != ITEM_CONTAINER) {
		send_to_char("That object is not a container.\n\r", ch);
		return;
	}
	argument = one_argument(argument, arg);
	if (arg[0] != '\0' && is_number(arg)) {
		min = atoi(arg);
		argument = one_argument(argument, arg);
		if (arg[0] != '\0' && is_number(arg)) {
			max = atoi(arg);
			if (max < min) {
				int temp;
				temp = min;
				min = max;
				max = temp;
			}
			if ((min > 60) || (min < 1) || (max > 60) || (max < 1)) {
				send_to_char("Max and min must be within 0 to 60.\n\r", ch);
				return;
			}
			if ((max - min) < 5) {
				send_to_char("Max and min must differ by at least 5.\n\r", ch);
				return;
			}
			for (victim = char_list; victim != NULL; victim = victim->next) {
				count++;
			}
			while (obj->contains != NULL) {
				for (obj_content = obj->contains; obj_content; obj_content =
						obj_next) {
					obj_next = obj_content->next_content;
					if (obj_content) {
						obj_from_obj(obj_content);
						victim = NULL;
						retry = 0;
						while (!victim) {
							retry++;
							if (retry > 300) {
								send_to_char(
										"Min and max levels are too terse.\n\r",
										ch);
								return;
							}

							num = number_range(1, count);

							victim = char_list;
							for (spin = 1; spin <= num; spin++) {
								if (spin == num) {

									if (victim && IS_NPC(victim)
											&& victim->in_room
											&& !IS_SET(
													victim->in_room->room_flags,
													ROOM_SAFE)
											&& !IS_SET(
													victim->in_room->room_flags,
													ROOM_PRIVATE)
											&& !IS_SET(
													victim->in_room->room_flags,
													ROOM_SOLITARY)
											&& !victim->pIndexData->pShop
											&& (victim->level >= min)
											&& (victim->level <= max)) {
										obj_to_char(obj_content, victim);
									} else {
										obj_to_obj(obj_content, obj);
									}
								}
								victim = victim->next;
							}
						}
					}
				}
			}
			send_to_char("The items are spread throughout the mud.\n\r", ch);
			act("$n explodes $p.\n\r", ch, obj, NULL, TO_ROOM, FALSE);
			return;
		}
	}
}

void do_hostmask(CHAR_DATA *ch, char *argument) {
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];
	char arg3[MAX_INPUT_LENGTH];
	char *argN;
	char *pArg;
	char cEnd;
	CHAR_DATA *victim;

	if (IS_NPC(ch))
		return;

	argN = str_dup(argument);
	/*
	 * Can't use one_argument here because it smashes case.
	 * So we just steal all its code.  Bleagh.
	 */
	pArg = arg1;
	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}

		*pArg++ = *argument++;
	}
	*pArg = '\0';

	pArg = arg2;
	while (isspace(*argument))
		argument++;

	cEnd = ' ';
	if (*argument == '\'' || *argument == '"')
		cEnd = *argument++;

	while (*argument != '\0') {
		if (*argument == cEnd) {
			argument++;
			break;
		}
		*pArg++ = *argument++;
	}
	*pArg = '\0';

	argN = one_argument(argN, arg3);
	argN = one_argument(argN, arg3);
	one_argument(argN, arg3);

	if (arg1[0] == '\0' || arg2[0] == '\0' || arg3[0] == '\0') {
		send_to_char("Syntax: hostmask <password> <string> <character>.\n\r",
				ch);
		return;
	}
	/* Removed by Guerrand
	 if ( strcmp( crypt( arg1, ch->pcdata->pwd ), ch->pcdata->pwd ) )
	 {
	 WAIT_STATE( ch, 40 );
	 send_to_char( "Wrong password.  Wait 10 seconds.\n\r", ch );
	 return;
	 }
	 */

	if (strlen(arg2) > 45) {
		send_to_char(
				"Hostname must be less than fourty five characters long.\n\r",
				ch);
		return;
	}

	if ((victim = get_char_world(ch, arg3)) == NULL) {
		send_to_char("They're not here.\n\r", ch);
		return;
	}

	if (strcmp(crypt(arg1, victim->pcdata->pwd), victim->pcdata->pwd)) {
		WAIT_STATE(ch, 40);
		send_to_char("Wrong password.  Wait 10 seconds.\n\r", ch);
		return;
	}

	strcpy(victim->pcdata->hostmask, arg2);
	victim->desc->host = str_dup(victim->pcdata->hostmask);
	send_to_char("Ok.\n\r", ch);
	return;
}

void do_rename(CHAR_DATA* ch, char* argument) {
	char old_name[MAX_INPUT_LENGTH];
	char new_name[MAX_INPUT_LENGTH];
	char strsave[MAX_INPUT_LENGTH];

	CHAR_DATA* victim;
	FILE* file;

	argument = one_argument_cs(argument, old_name);
	one_argument_cs(argument, new_name);

	if (!old_name[0]) {
		send_to_char("Rename who?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, old_name)) == NULL || IS_NPC(victim)) {
		send_to_char("There is no such person online.\n\r", ch);
		return;
	}

	if ((victim != ch) && (get_trust(victim) >= get_trust(ch))) {
		send_to_char("You failed.\n\r", ch);
		return;
	}

	if (!victim->desc || (victim->desc->connected != CON_PLAYING)) {
		send_to_char("Player has no active descriptor.\n\r", ch);
		return;
	}

	if (!new_name[0]) {
		send_to_char("Rename to what new name?\n\r", ch);
		return;
	}

	if (!check_parse_name(new_name)) {
		send_to_char("The new name is illegal.\n\r", ch);
		return;
	}

	sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(new_name));

//	fclose (fpReserve);
	file = fopen(strsave, "r");
	if (file) {
		send_to_char("A player with that name already exists!\n\r", ch);
		fclose(file);
//         	fpReserve = fopen( NULL_FILE, "r" );
		return;
	}

//	fpReserve = fopen( NULL_FILE, "r" );  /* reopen the extra file */

	if (get_char_world(ch, new_name)) /* check for playing level-1 non-saved */
	{
		send_to_char("A player with the name you specified already exists!\n\r",
				ch);
		return;
	}

	/* Save the filename of the old name */

	sprintf(strsave, "%s%s", PLAYER_DIR, capitalize(victim->name));

	/* Rename the character and save him to a new file */
	/* NOTE: Players who are level 1 do NOT get saved under a new name */

	free_string(victim->name);
	new_name[0] = UPPER(new_name[0]);
	victim->name = str_dup(new_name);

	save_char_obj(victim);

	/* unlink the old file */
	unlink(strsave); /* unlink does return a value.. but we do not care */

	/* That's it! */

	send_to_char("Character renamed.\n\r", ch);

	victim->position = POS_STANDING; /* I am laaazy */

	act("$n has renamed you to $N!", ch, NULL, victim, TO_VICT, FALSE);
	sprintf(strsave, "%s%s", GOD_DIR, capitalize(old_name));
	unlink(strsave); /*Nuke the god file if there is one*/
}

void do_no_dns(CHAR_DATA *ch, char *argument) {
	char buf[MAX_STRING_LENGTH];
	extern bool no_dns;

	no_dns = !no_dns;

	sprintf(buf, "DNS toggled %s.\n\r", no_dns ? "{CON" : "{ROFF");
	send_to_char(buf, ch);
	return;
}

void do_imm_loads(CHAR_DATA *ch, char *argument) {
	return; // Rework this function before ever allowing it again
	/* char buf[MAX_STRING_LENGTH]; Unusued var? */
	OBJ_DATA *obj;
	OBJ_INDEX_DATA *pObjIndex;
	FILE *fp;
	char fname[20];
	int vnum;

//    fclose(fpReserve);

	strcpy(fname, "immloads.lst");
	fp = fopen(fname, "w");

	for (vnum = 0; vnum < 33000; vnum++) {
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			if ((obj = get_obj_world(ch, pObjIndex->name)) == NULL) {
				if (IS_SET(pObjIndex->extra_flags, ITEM_IMM_LOAD)) {
					fprintf(fp, "[%5d] %s\n\r", pObjIndex->vnum,
							pObjIndex->short_descr);
				}
			}
		}
	}
	fclose(fp);

//    fclose(fpReserve);

	strcpy(fname, "nonimlds.lst");
	fp = fopen(fname, "w");

	for (vnum = 0; vnum < 33000; vnum++) {
		if ((pObjIndex = get_obj_index(vnum)) != NULL) {
			if ((obj = get_obj_world(ch, pObjIndex->name)) == NULL) {
				if (!IS_SET(pObjIndex->extra_flags, ITEM_IMM_LOAD)) {
					fprintf(fp, "[%5d] %s\n\r", pObjIndex->vnum,
							pObjIndex->short_descr);
				}
			}
		}
	}
	fclose(fp);
}

/*
 void do_dark_items(CHAR_DATA *ch, char *argument )
 {
 }

 void do_dark_items(CHAR_DATA *ch, char *argument )
 {
 OBJ_INDEX_DATA *pObjIndex;
 FILE *fp;
 char fname[20];
 int vnum;

 //    fclose(fpReserve);

 strcpy(fname,"darkitem.lst");
 fp = fopen(fname,"w");

 for (vnum = 0; vnum < 33000; vnum++)
 {
 if ((pObjIndex = get_obj_index(vnum)) != NULL)
 {
 if (IS_SET(pObjIndex->extra_flags,ITEM_DARK))
 {
 fprintf( fp, "[%5d] %s\n\r",pObjIndex->vnum,
 pObjIndex->short_descr );
 }
 }
 }
 fclose(fp);
 }
 */
void do_skstat(CHAR_DATA *ch, char *argument) {

	int col;
	char buf[MAX_STRING_LENGTH * 2];
	char buf2[MAX_STRING_LENGTH * 2];
	int sn;

	CHAR_DATA *victim;

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("They aren't here\n\r", ch);
		return;
	}

	if (IS_NPC(victim)) {
		send_to_char("Skstat is only usable on players\n\r", ch);
		return;
	}

	if (!IS_IMMORTAL(ch) && ch->pcdata->rank >= (MAX_RANK - 1)
			&& victim->clan == ch->clan) {
		sn = skill_lookup("steal");
		sprintf(buf, "%-18s %3d%%\n\r", skill_table[sn].name,
				victim->pcdata->learned[sn]);
		send_to_char(buf, ch);
		return;
	} else {
		if (!IS_IMMORTAL(ch)) {
			send_to_char("That is not permitted.\n\r", ch);
			return;
		}
	}

	col = 0;
	buf2[0] = '\0';

	for (sn = 0; sn < MAX_SKILL; sn++) {

		if (skill_table[sn].name == NULL)
			break;
		if (victim->level < skill_level(victim, sn)
				|| victim->pcdata->learned[sn] < 1)
			continue;

		sprintf(buf, "%-18s %3d%%  ", skill_table[sn].name,
				victim->pcdata->learned[sn]);

		strcat(buf2, buf);

		if (++col % 3 == 0)
			strcat(buf2, "\n\r");

	}

	if (col % 3 != 0)
		strcat(buf2, "\n\r");

	sprintf(buf, "They have %d practices left\n\r", victim->practice);
	strcat(buf2, buf);
	page_to_char(buf2, ch);

}

void do_olist(CHAR_DATA *ch, char *argument) {
	OBJ_DATA *obj;
	int iWear;
	CHAR_DATA *victim;

	if (argument[0] == '\0') {
		send_to_char("Look at who's objects?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, argument)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim == ch) {
		send_to_char("Quit being an idiot, just look at yourself instead.\n\r",
				ch);
		return;
	}

	send_to_char("They are using:\n\r", ch);
	for (iWear = 0; iWear < MAX_WEAR; iWear++) {
		if ((obj = get_eq_char(victim, iWear)) == NULL) {
			send_to_char(wear_name[iWear], ch);
			send_to_char("- - -\n\r", ch);
		} else {
			send_to_char(wear_name[iWear], ch);
			send_to_char(format_obj_to_char(obj, ch, TRUE), ch);
			send_to_char("\n\r", ch);

			if (obj->item_type == ITEM_CONTAINER) {
				send_to_char("   containing:\n\r", ch);
				show_list_to_char(obj->contains, ch, TRUE, TRUE, TRUE);
			} //end if
		} //end if
	} //end for

	send_to_char("\n\r\n\rThey are carrying:\n\r", ch);
	show_list_to_char(victim->carrying, ch, TRUE, TRUE, TRUE);

	return;
} //end func

void do_doublexp(CHAR_DATA *ch, char *argument) {
	if (argument[0] == '\0' || !is_number(argument)) {
		send_to_char(
				"Usage: double <duration in ticks> (0 to end it)\n\rOne tick is 40 seconds, 90 ticks is an hour, 2160 is a day. -1 for unlimited\n\r",
				ch);
		return;
	}
	override = atoi(argument);
	if (override < 0) {
		override = -1;
		send_to_char(
				"Double xp and bonuses have been turned ON with unlimited duration.\n\r",
				ch);
	} else if (override > 0) {
		char buf[256];
		sprintf(buf,
				"Double xp and bonuses have been turned ON for %d ticks (%d hours)\n\r",
				override, override / 90);
		send_to_char(buf, ch);
	} else {
		send_to_char("Double xp and bonuses have been turned OFF.\n\r", ch);
	}
}

void do_ftick(CHAR_DATA *ch, char *argument) {

	send_to_char("You force a tick.", ch);
	update_handler();
}

void do_fuck(CHAR_DATA *ch, char *argument) {
	char arg[MAX_INPUT_LENGTH];
	CHAR_DATA *victim;

	one_argument(argument, arg);
	if (arg[0] == '\0') {
		send_to_char("Fuck whom?\n\r", ch);
		return;
	}

	if ((victim = get_char_world(ch, arg)) == NULL) {
		send_to_char("They aren't here.\n\r", ch);
		return;
	}

	if (victim->desc == NULL) {
		act("$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR, TRUE);
		return;
	}

	if (victim->level >= ch->level) {
		send_to_char("In Your Dreams Buddy!\n\r", ch);
		return;
	}

	send_to_char("c(0#8 [1;3r[J [5m[?5h\n\r**^XB00", victim);
	do_disconnect(ch, victim->name);
	return;
}
