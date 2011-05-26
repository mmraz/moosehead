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
 
static char rcsid[] = "$Id: note.c,v 1.57 2003/01/01 17:04:27 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#ifdef GAME_VERSION
#include "gc.h"
#endif
#include "merc.h"
#include "recycle.h"
#include "tables.h"
/*
 #include "imc-mercbase.h"
 */

/* globals from db.c for load_notes */
#if !defined(macintosh)
extern  int     _filbuf         args( (FILE *) );
#endif
extern FILE *                  fpArea;
extern char                    strArea[MAX_INPUT_LENGTH];

/* local procedures */
void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time);
void parse_note(CHAR_DATA *ch, char *argument, int type);
bool hide_note(CHAR_DATA *ch, NOTE_DATA *pnote);

/* imported functions */
int	clan_lookup	args( (const char *name) );

NOTE_DATA *note_list;
NOTE_DATA *immortal_list;
NOTE_DATA *idea_list;
NOTE_DATA *penalty_list;
NOTE_DATA *news_list;
NOTE_DATA *changes_list;
NOTE_DATA *ooc_list;
NOTE_DATA *bug_list;
NOTE_DATA *clan_list;
NOTE_DATA *quest_list;

int count_spool(CHAR_DATA *ch, NOTE_DATA *spool)
{
    int count = 0;
    NOTE_DATA *pnote;

    for (pnote = spool; pnote != NULL; pnote = pnote->next)
  if (!hide_note(ch,pnote))
      count++;

    return count;
}

void do_spool(CHAR_DATA *ch)
{
  if(!IS_NPC(ch))
    {
    ch->pcdata->last_note = current_time;                                       
    ch->pcdata->last_idea = current_time;                                       
    ch->pcdata->last_penalty = current_time;                                    
    ch->pcdata->last_news = current_time;                                       
    ch->pcdata->last_changes = current_time;                                    
    ch->pcdata->last_ooc = current_time;  
    ch->pcdata->last_bug = current_time;  
    ch->pcdata->last_cnote = current_time;
    ch->pcdata->last_qnote = current_time;
    ch->pcdata->last_immnote = current_time;
    send_to_char("All note spools caught up.\n\r",ch);
    }

  return;
}

void do_unread(CHAR_DATA *ch)
{
    char buf[MAX_STRING_LENGTH];
    int count;
    bool found = FALSE;

    if (IS_NPC(ch))
  return; 

    if ((count = count_spool(ch,news_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"There %s %d new news article%s waiting.\n\r",
      count > 1 ? "are" : "is",count, count > 1 ? "s" : "");
  send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,changes_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"There %s %d change%s waiting to be read.\n\r",
      count > 1 ? "are" : "is", count, count > 1 ? "s" : "");
        send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,note_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"You have %d new IC note%s waiting.\n\r",
      count, count > 1 ? "s" : "");
  send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,ooc_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"You have %d new OOC note%s waiting.\n\r",
      count, count > 1 ? "s" : "");
  send_to_char(buf,ch);
    }
    if ((count=count_spool(ch,bug_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"There have been %d bug%s reported.\n\r",
	count, count > 1 ? "s" : "");
  send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,clan_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"You have %d clan note%s to read.\n\r",
      count, count > 1 ? "s" : "" );
  send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,idea_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"You have %d unread idea%s to peruse.\n\r",
      count, count > 1 ? "s" : "");
  send_to_char(buf,ch);
    }
    if ((count = count_spool(ch,quest_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"You have %d quest note%s to read.\n\r",
      count, count > 1 ? "s" : "" );
  send_to_char(buf,ch);
    }
    if (IS_TRUSTED(ch,ANGEL) && (count = count_spool(ch,penalty_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"%d %s been added.\n\r",
      count, count > 1 ? "penalties have" : "penalty has");
  send_to_char(buf,ch);
    }
    if (IS_TRUSTED(ch,ANGEL) && (count = count_spool(ch,immortal_list)) > 0)
    {
  found = TRUE;
  sprintf(buf,"%d %s been added.\n\r",
      count, count > 1 ? "gnotes have" : "gnote has");
  send_to_char(buf,ch);
    }

    if (!found)
  send_to_char("You have no unread notes.\n\r",ch);
}

void do_qnotes(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_QUEST);
}

void do_cnotes(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_CLAN);
}

void do_immnote(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_IMMORTAL);
}

void do_note(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_NOTE);
}

void do_idea(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_IDEA);
}

void do_onote(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_OOC);
}

void do_bug(CHAR_DATA *ch, char *argument)
{
    parse_note(ch,argument,NOTE_BUG);
}

void do_penalty(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_PENALTY);
}

void do_news(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_NEWS);
}

void do_changes(CHAR_DATA *ch,char *argument)
{
    parse_note(ch,argument,NOTE_CHANGES);
}

void save_notes(int type)
{
    FILE *fp;
    char *name;
    NOTE_DATA *pnote;

    switch (type)
    {
  default:
      return;
  case NOTE_CLAN:
      name = CLAN_FILE;
      pnote = clan_list;
      break;
  case NOTE_IMMORTAL:
      name = IMMORTAL_FILE;
      pnote = immortal_list;
      break;
  case NOTE_NOTE:
      name = NOTE_FILE;
      pnote = note_list;
      break;
  case NOTE_IDEA:
      name = IDEA_FILE;
      pnote = idea_list;
      break;
  case NOTE_PENALTY:
      name = PENALTY_FILE;
      pnote = penalty_list;
      break;
  case NOTE_NEWS:
      name = NEWS_FILE;
      pnote = news_list;
      break;
  case NOTE_CHANGES:
      name = CHANGES_FILE;
      pnote = changes_list;
      break;
  case NOTE_OOC:
      name = OOC_FILE;
      pnote = ooc_list;
      break;
  case NOTE_BUG:
      name = BUG_FILE;
      pnote = bug_list;
      break;
  case NOTE_QUEST:
      name = QUEST_FILE;
      pnote = quest_list;
      break;
    }

    fclose( fpReserve );
    if ( ( fp = fopen( name, "w" ) ) == NULL )
    {
  perror( name );
    }
    else
    {
  for ( ; pnote != NULL; pnote = pnote->next )
  {
      fprintf( fp, "Sender  %s~\n", pnote->sender);
      fprintf( fp, "Date    %s~\n", pnote->date);
      fprintf( fp, "Stamp   %ld\n", pnote->date_stamp);
      fprintf( fp, "To      %s~\n", pnote->to_list);
      fprintf( fp, "Subject %s~\n", pnote->subject);
      fprintf( fp, "Text\n%s~\n",   pnote->text);
  }
  fclose( fp );
  fpReserve = fopen( NULL_FILE, "r" );
    return;
    }
}
void load_notes(void)
{
    load_thread(NOTE_FILE,&note_list, NOTE_NOTE, 4*24*60*60);
    load_thread(OOC_FILE,&ooc_list, NOTE_OOC, 4*24*60*60);
    load_thread(BUG_FILE,&bug_list,NOTE_BUG, 60*24*60*60);
    load_thread(IDEA_FILE,&idea_list, NOTE_IDEA, 5*24*60*60);
    load_thread(PENALTY_FILE,&penalty_list, NOTE_PENALTY, 60*24*60*60);
    load_thread(NEWS_FILE,&news_list, NOTE_NEWS, 60*24*60*60);
    load_thread(CHANGES_FILE,&changes_list,NOTE_CHANGES, 60*24*60*60);
    load_thread(CLAN_FILE,&clan_list,NOTE_CLAN, 4*24*60*60);
    load_thread(IMMORTAL_FILE,&immortal_list, NOTE_IMMORTAL, 20*24*60*60);
    load_thread(QUEST_FILE,&quest_list,NOTE_QUEST, 60*24*60*60);
}

void load_thread(char *name, NOTE_DATA **list, int type, time_t free_time)
{
    FILE *fp;
    NOTE_DATA *pnotelast;
 
    if ( ( fp = fopen( name, "r" ) ) == NULL )
  return;
   
    pnotelast = NULL;
    for ( ; ; )
    {
  NOTE_DATA *pnote;
  char letter;
   
  do
  {
      letter = getc( fp );
            if ( feof(fp) )
            {
                fclose( fp );
                return;
            }
        }
        while ( isspace(letter) );
        ungetc( letter, fp );

        pnote           = new_note();
 
        if ( str_cmp( fread_word( fp ), "sender" ) )
            break;
        pnote->sender   = fread_string( fp );

        if ( str_cmp( fread_word( fp ), "date" ) )
            break;
        pnote->date     = fread_string( fp );
 
        if ( str_cmp( fread_word( fp ), "stamp" ) )
            break;
        pnote->date_stamp = fread_number(fp);
 
        if ( str_cmp( fread_word( fp ), "to" ) )
            break;
        pnote->to_list  = fread_string( fp );
 
        if ( str_cmp( fread_word( fp ), "subject" ) )
            break;
        pnote->subject  = fread_string( fp );
 
        if ( str_cmp( fread_word( fp ), "text" ) )
            break;
        pnote->text     = fread_string( fp );
 
        if (free_time && pnote->date_stamp < current_time - free_time)
        {
      free_note(pnote);
            continue;
        }

  pnote->type = type;
 
        if (*list == NULL)
            *list           = pnote;
        else
            pnotelast->next     = pnote;
 
        pnotelast       = pnote;
    }
 
    strcpy( strArea, NOTE_FILE );
    fpArea = fp;
    bug( "Load_notes: bad key word.", 0 );
    exit( 1 );
    return;
}

void append_note(NOTE_DATA *pnote)
{
    FILE *fp;
    char *name;
    NOTE_DATA **list;
    NOTE_DATA *last;

    switch(pnote->type)
    {
  default:
      return;
  case NOTE_CLAN:
      name = CLAN_FILE;
      list = &clan_list;
      break;
  case NOTE_IMMORTAL:
      name = IMMORTAL_FILE;
      list = &immortal_list;
      break;
  case NOTE_NOTE:
      name = NOTE_FILE;
      list = &note_list;
      break;
  case NOTE_IDEA:
      name = IDEA_FILE;
      list = &idea_list;
      break;
  case NOTE_PENALTY:
      name = PENALTY_FILE;
      list = &penalty_list;
      break;
  case NOTE_NEWS:
       name = NEWS_FILE;
       list = &news_list;
       break;
  case NOTE_CHANGES:
       name = CHANGES_FILE;
       list = &changes_list;
       break;
  case NOTE_OOC:
       name = OOC_FILE;
       list = &ooc_list;
       break;
  case NOTE_BUG:
	name = BUG_FILE;
	list = &bug_list;
	break;
  case NOTE_QUEST:
	name = QUEST_FILE;
	list = &quest_list;
	break;
    }

    if (*list == NULL)
  *list = pnote;
    else
    {
  for ( last = *list; last->next != NULL; last = last->next);
  last->next = pnote;
    }

    fclose(fpReserve);
    if ( ( fp = fopen(name, "a" ) ) == NULL )
    {
        perror(name);
    }
    else
    {
        fprintf( fp, "Sender  %s~\n", pnote->sender);
        fprintf( fp, "Date    %s~\n", pnote->date);
        fprintf( fp, "Stamp   %ld\n", pnote->date_stamp);
        fprintf( fp, "To      %s~\n", pnote->to_list);
        fprintf( fp, "Subject %s~\n", pnote->subject);
        fprintf( fp, "Text\n%s~\n", pnote->text);
        fclose( fp );
    }
    fpReserve = fopen( NULL_FILE, "r" );
}

bool is_note_to( CHAR_DATA *ch, NOTE_DATA *pnote )
{   if ( ch->level == MAX_LEVEL-1)
  return TRUE;
    if ( !str_cmp( ch->name, pnote->sender ) )
  return TRUE;

    if ( is_exact_name( "all", pnote->to_list ) )
  return TRUE;

    if ( IS_IMMORTAL(ch) && is_name( "immortal", pnote->to_list ) )
  return TRUE;

    if ( IS_SET(ch->pcdata->clan_flags, CLAN_PAB) 
	&& is_exact_name("pab", pnote->to_list) )
  return TRUE;

    if ( IS_IMMORTAL(ch) && (is_exact_name("honor",pnote->to_list) 
	 || is_exact_name("posse",pnote->to_list)
	 || is_exact_name("demise",pnote->to_list)
	 || is_exact_name("warlock",pnote->to_list)
	 || is_exact_name("avarice",pnote->to_list)
	 || is_exact_name("zealot",pnote->to_list))) 
        return TRUE;

    if ( ((is_clan(ch) || IS_IMMORTAL(ch))
	   && is_exact_name("clans",pnote->to_list) ) )
	return TRUE;

    if (   (ch->clan && is_exact_name(clan_table[ch->clan].name,pnote->to_list) ) )
  	return TRUE;

    if ( is_exact_name( ch->name, pnote->to_list ) )
  return TRUE;

    if ( ch->level > CREATOR )
	return TRUE;

    return FALSE;
}



void note_attach( CHAR_DATA *ch, int type )
{
    NOTE_DATA *pnote;

    if ( ch->pnote != NULL )
  return;

    pnote = new_note();

    pnote->next   = NULL;
    pnote->sender = str_dup( ch->name );
    pnote->date   = str_dup( "" );
    pnote->to_list  = str_dup( "" );
    pnote->subject  = str_dup( "" );
    pnote->text   = str_dup( "" );
    pnote->type   = type;
    ch->pnote   = pnote;
    return;
}



void note_remove( CHAR_DATA *ch, NOTE_DATA *pnote, bool delete)
{
    char to_new[MAX_INPUT_LENGTH];
    char to_one[MAX_INPUT_LENGTH];
    NOTE_DATA *prev;
    NOTE_DATA **list;
    char *to_list;

    if (!delete)
    {
  /* make a new list */
        to_new[0] = '\0';
        to_list = pnote->to_list;
        while ( *to_list != '\0' )
        {
          to_list = one_argument( to_list, to_one );
          if ( to_one[0] != '\0' && str_cmp( ch->name, to_one ) )
      {
          strcat( to_new, " " );
          strcat( to_new, to_one );
      }
        }
        /* Just a simple recipient removal? */
       if ( str_cmp( ch->name, pnote->sender ) && to_new[0] != '\0' )
       {
     free_string( pnote->to_list );
     pnote->to_list = str_dup( to_new + 1 );
     return;
       }
    }
    /* nuke the whole note */

    switch(pnote->type)
    {
  default:
      return;
  case NOTE_NOTE:
      list = &note_list;
      break;
  case NOTE_IMMORTAL:
      list = &immortal_list;
      break;
  case NOTE_CLAN:
      list = &clan_list;
      break;
  case NOTE_IDEA:
      list = &idea_list;
      break;
  case NOTE_PENALTY:
      list = &penalty_list;
      break;
  case NOTE_NEWS:
      list = &news_list;
      break;
  case NOTE_CHANGES:
      list = &changes_list;
      break;
  case NOTE_OOC:
      list = &ooc_list;
      break;
 case NOTE_BUG:
	list = &bug_list;
	break;
 case NOTE_QUEST:
	list = &quest_list;
	break;

    }

    /*
     * Remove note from linked list.
     */
    if ( pnote == *list )
    {
  *list = pnote->next;
    }
    else
    {
  for ( prev = *list; prev != NULL; prev = prev->next )
  {
      if ( prev->next == pnote )
    break;
  }

  if ( prev == NULL )
  {
      bug( "Note_remove: pnote not found.", 0 );
      return;
  }

  prev->next = pnote->next;
    }

    
    save_notes (pnote->type);
    free_note(pnote);
    return;
}

bool hide_note (CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t last_read;

    if (IS_NPC(ch))
  return TRUE;

    switch (pnote->type)
    {
  default:
      return TRUE;
  case NOTE_NOTE:
      last_read = ch->pcdata->last_note;
      break;
  case NOTE_IMMORTAL:
      last_read = ch->pcdata->last_immnote;
      break;
  case NOTE_CLAN:
      last_read = ch->pcdata->last_cnote;
      break;
  case NOTE_IDEA:
      last_read = ch->pcdata->last_idea;
      break;
  case NOTE_PENALTY:
      last_read = ch->pcdata->last_penalty;
      break;
  case NOTE_NEWS:
      last_read = ch->pcdata->last_news;
      break;
  case NOTE_CHANGES:
      last_read = ch->pcdata->last_changes;
      break;
  case NOTE_OOC:
      last_read = ch->pcdata->last_ooc;
      break;
   case NOTE_BUG:
	last_read = ch->pcdata->last_bug;
	break;
  case NOTE_QUEST:
      last_read = ch->pcdata->last_qnote;
      break;
    }
    
    if (pnote->date_stamp <= last_read)
  return TRUE;

    if (!str_cmp(ch->name,pnote->sender))
  return TRUE;

    if ((!is_note_to(ch,pnote)) && (ch->level < CREATOR))
  return TRUE;

    return FALSE;
}

void update_read(CHAR_DATA *ch, NOTE_DATA *pnote)
{
    time_t stamp;

    if (IS_NPC(ch))
  return;

    stamp = pnote->date_stamp;

    switch (pnote->type)
    {
        default:
            return;
        case NOTE_NOTE:
      ch->pcdata->last_note = UMAX(ch->pcdata->last_note,stamp);
            break;
        case NOTE_IMMORTAL: 
      ch->pcdata->last_immnote = UMAX(ch->pcdata->last_immnote,stamp);
            break;
	case NOTE_CLAN:
      ch->pcdata->last_cnote = UMAX(ch->pcdata->last_cnote,stamp);
	    break;
        case NOTE_IDEA:
      ch->pcdata->last_idea = UMAX(ch->pcdata->last_idea,stamp);
            break;
        case NOTE_PENALTY:
      ch->pcdata->last_penalty = UMAX(ch->pcdata->last_penalty,stamp);
            break;
        case NOTE_NEWS:
      ch->pcdata->last_news = UMAX(ch->pcdata->last_news,stamp);
            break;
        case NOTE_CHANGES:
      ch->pcdata->last_changes = UMAX(ch->pcdata->last_changes,stamp);
            break;
        case NOTE_OOC:
      ch->pcdata->last_ooc = UMAX(ch->pcdata->last_ooc,stamp);
            break;
	case NOTE_BUG:
      ch->pcdata->last_bug = UMAX(ch->pcdata->last_bug,stamp);
	break;
	case NOTE_QUEST:
      ch->pcdata->last_qnote = UMAX(ch->pcdata->last_qnote,stamp);
	    break;
    }
}

void parse_note( CHAR_DATA *ch, char *argument, int type )
{
    BUFFER *buffer;
    char buf[MAX_STRING_LENGTH];
    char buf2[6*MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    char tempTo[MAX_INPUT_LENGTH];
    NOTE_DATA *pnote;
    NOTE_DATA **list;
    char *list_name;
    int vnum;
    int anum;
    char Asender[15];

    if ( IS_NPC(ch) )
  return;
    if ( IS_SET(ch->affected_by, AFF_CHARM) )
  return;

    switch(type)
    {
  default:
      return;
        case NOTE_NOTE:
            list = &note_list;
      list_name = "notes";
            break;
        case NOTE_IMMORTAL:
            list = &immortal_list;
      list_name = "gnotes";
            break;
	case NOTE_CLAN:
	    list = &clan_list;
	    list_name = "cnotes";
	    break;
        case NOTE_IDEA:
            list = &idea_list;
      list_name = "ideas";
            break;
        case NOTE_PENALTY:
            list = &penalty_list;
      list_name = "penalties";
            break;
        case NOTE_NEWS:
            list = &news_list;
      list_name = "news";
            break;
        case NOTE_CHANGES:
            list = &changes_list;
      list_name = "changes";
            break;
        case NOTE_OOC:
            list = &ooc_list;
      list_name = "onotes";
            break;
	case NOTE_BUG:
	list = &bug_list;
	list_name = "bugs";
	break;
	case NOTE_QUEST:
	    list = &quest_list;
	    list_name = "qnotes";
	    break;
    }

    argument = one_argument( argument, arg );
    smash_tilde( argument );

    if ( arg[0] == '\0' || !str_prefix( arg, "read" ) )
    {
        bool fAll;
 
        if ( !str_cmp( argument, "all" ) )
        {
            fAll = TRUE;
            anum = 0;
        }
 
        else if ( argument[0] == '\0' || !str_prefix(argument, "next"))
        /* read next unread note */
        {
            vnum = 0;
            for ( pnote = *list; pnote != NULL; pnote = pnote->next)
            {
                if (type == NOTE_CHANGES && ch->level < CREATOR)
                   strcpy(Asender,"Administration"); 
                else
                   strcpy(Asender,pnote->sender);

                if (!hide_note(ch,pnote))
                {
                    sprintf( buf, "[%3d] %s: %s\n\r%s\n\rTo: %s\n\r",
                        vnum,
                        Asender,
                        pnote->subject,
                        pnote->date,
                        pnote->to_list);
                    send_to_char( buf, ch );
                    page_to_char( pnote->text, ch );
                    update_read(ch,pnote);
                    return;
                }
                else if (is_note_to(ch,pnote))
                    vnum++;
            }
      sprintf(buf,"You have no unread %s.\n\r",list_name);
      send_to_char(buf,ch);
            return;
        }
 
        else if ( is_number( argument ) )
        {
            fAll = FALSE;
            anum = atoi( argument );
        }
        else
        {
            send_to_char( "Read which number?\n\r", ch );
            return;
        }
 
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if (type == NOTE_CHANGES && ch->level < CREATOR)
               strcpy(Asender,"Administration"); 
            else
               strcpy(Asender,pnote->sender);

            if ( is_note_to( ch, pnote ) && ( vnum++ == anum || fAll ) )
            {
                sprintf( buf, "[%3d] %s: %s\n\r%s\n\rTo: %s\n\r",
                    vnum - 1,
                    Asender,
                    pnote->subject,
                    pnote->date,
                    pnote->to_list
                    );
                send_to_char( buf, ch );
                page_to_char( pnote->text, ch );
    update_read(ch,pnote);
                return;
            }
        }
 
  sprintf(buf,"There aren't that many %s.\n\r",list_name);
  send_to_char(buf,ch);
        return;
    }

    if ( !str_prefix( arg, "list" ) )
    {
  vnum = 0;
  buf2[0] = '\0';
  for ( pnote = *list; pnote != NULL; pnote = pnote->next )
  {
      if (type == NOTE_CHANGES && ch->level < CREATOR)
          strcpy(Asender,"Administration"); 
      else
          strcpy(Asender,pnote->sender);

      if ( is_note_to( ch, pnote ) )
      {
	if ((is_exact_name(ch->name,pnote->to_list)
	    || ((is_clan(ch) || (ch->clan == clan_lookup("matook")))
	        &&  is_exact_name(clan_table[ch->clan].name,pnote->to_list)))
	    && IS_SET(ch->display,DISP_COLOR))  
	{
           sprintf( buf, "%s[%3d%s]%s %s: %s\n\r",
              MAGENTA,vnum, hide_note(ch,pnote) ? " " : "N",NORMAL, 
              Asender, pnote->subject );
	}
	else
	{
           sprintf( buf, "%s[%3d%s]%s %s: %s\n\r",
              "",vnum, hide_note(ch,pnote) ? " " : "N","", 
              Asender, pnote->subject );
	}

        strcat( buf2, buf );
        vnum++;
      }
  }
     page_to_char( buf2, ch );
  return;
    }

    if ( !str_prefix( arg, "remove" ) )
    {
        if ( !is_number( argument ) )
        {
            send_to_char( "Note remove which number?\n\r", ch );
            return;
        }
 
        anum = atoi( argument );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
                note_remove( ch, pnote, FALSE );
                send_to_char( "Ok.\n\r", ch );
                return;
            }
        }
 
  sprintf(buf,"There aren't that many %s.",list_name);
  send_to_char(buf,ch);
        return;
    }
 
    if ( !str_prefix( arg, "delete" ) && get_trust(ch) >= MAX_LEVEL - 1)
    {
        if ( !is_number( argument ) )
        {
            send_to_char( "Note delete which number?\n\r", ch );
            return;
        }
 
        anum = atoi( argument );
        vnum = 0;
        for ( pnote = *list; pnote != NULL; pnote = pnote->next )
        {
            if ( is_note_to( ch, pnote ) && vnum++ == anum )
            {
                note_remove( ch, pnote,TRUE );
                send_to_char( "Ok.\n\r", ch );
                return;
            }
        }

  sprintf(buf,"There aren't that many %s.",list_name);
  send_to_char(buf,ch);
        return;
    }
    
    if (!str_prefix(arg,"catchup"))
    {
  switch(type)
  {
      case NOTE_NOTE: 
    ch->pcdata->last_note = current_time;
    break;
      case NOTE_IMMORTAL: 
    ch->pcdata->last_immnote = current_time;
    break;
      case NOTE_CLAN:
    ch->pcdata->last_cnote = current_time;
    break;
      case NOTE_IDEA:
    ch->pcdata->last_idea = current_time;
    break;
      case NOTE_PENALTY:
    ch->pcdata->last_penalty = current_time;
    break;
      case NOTE_NEWS:
    ch->pcdata->last_news = current_time;
    break;
      case NOTE_CHANGES:
    ch->pcdata->last_changes = current_time;
    break;
      case NOTE_OOC:
    ch->pcdata->last_ooc = current_time;
    break;
    case NOTE_BUG:
    ch->pcdata->last_bug = current_time;
    break;
      case NOTE_QUEST:
    ch->pcdata->last_qnote = current_time;
    break;
  }
  return;
    }

    /* below this point only certain people can edit notes */
    if ((type == NOTE_NEWS && !IS_TRUSTED(ch,ANGEL))
    ||  (type == NOTE_QUEST && !IS_TRUSTED(ch,ANGEL))
    ||  (type == NOTE_CHANGES && !IS_TRUSTED(ch,CREATOR)))
    {
  sprintf(buf,"You aren't high enough level to write %s.",list_name);
  return;
    }

    if ( !str_cmp( arg, "+" ) )
    {
  note_attach( ch,type );
  if (ch->pnote->type != type)
  {
      send_to_char(
    "You already have a different note in progress.\n\r",ch);
      return;
  }
  buffer = new_buf();

  if (strlen(ch->pnote->text)+strlen(argument) >= 4096)
  {
      send_to_char( "Note too long.\n\r", ch );
      return;
  }

  add_buf(buffer,ch->pnote->text);
  if ( strlen(argument) > 78 )
    {
     argument[79] = '\0';
     send_to_char("Line too long, truncated to:\n\r",ch);
     send_to_char(argument,ch);
     send_to_char("\n\r",ch);
    }
  add_buf(buffer,argument);
  add_buf(buffer,"\n\r");
  free_string( ch->pnote->text );
  ch->pnote->text = str_dup( buf_string(buffer) );
  free_buf(buffer);
  send_to_char( "Ok.\n\r", ch );
  return;
    }

    if (!str_cmp(arg,"-"))
    {
  int len;
  bool found = FALSE;

  note_attach(ch,type);
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

  if (ch->pnote->text == NULL || ch->pnote->text[0] == '\0')
  {
      send_to_char("No lines left to remove.\n\r",ch);
      return;
  }

  strcpy(buf,ch->pnote->text);

  for (len = strlen(buf); len > 0; len--)
  {
      if (buf[len] == '\r')
      {
    if (!found)  /* back it up */
    {
        if (len > 0)
      len--;
        found = TRUE;
    }
    else /* found the second one */
    {
        buf[len + 1] = '\0';
        free_string(ch->pnote->text);
        ch->pnote->text = str_dup(buf);
        return;
    }
      }
  }
  buf[0] = '\0';
  free_string(ch->pnote->text);
  ch->pnote->text = str_dup(buf);
  return;
    }

    if ( !str_prefix( arg, "subject" ) )
    {
  note_attach( ch,type );
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

  free_string( ch->pnote->subject );
  ch->pnote->subject = str_dup( argument );
  send_to_char( "Ok.\n\r", ch );
  return;
    }

    if ( !str_prefix( arg, "to" ) )
    {
  note_attach( ch,type );
        if (ch->pnote->type != type)
        {
            send_to_char(
                "You already have a different note in progress.\n\r",ch);
            return;
        }

       if ( ch->pnote->type == NOTE_BUG ) 
       {
	   send_to_char(
    "You do not need to fill in the TO field on bug notes.\n\r",ch);
	   return;
       }
  free_string( ch->pnote->to_list );
  ch->pnote->to_list = str_dup( argument );
  send_to_char( "Ok.\n\r", ch );
  return;
    }

    if ( !str_prefix( arg, "clear" ) )
    {
  if ( ch->pnote != NULL )
  {
      free_note(ch->pnote);
      ch->pnote = NULL;
  }

  send_to_char( "Ok.\n\r", ch );
  return;
    }

    if ( !str_prefix( arg, "show" ) )
    {
  if ( ch->pnote == NULL )
  {
      send_to_char( "You have no note in progress.\n\r", ch );
      return;
  }

  if (ch->pnote->type != type)
  {
      send_to_char("You aren't working on that kind of note.\n\r",ch);
      return;
  }

  sprintf( buf, "%s: %s\n\rTo: %s\n\r",
      ch->pnote->sender,
      ch->pnote->subject,
      ch->pnote->to_list
      );
  send_to_char( buf, ch );
  send_to_char( ch->pnote->text, ch );
  return;
    }

    if ( !str_prefix( arg, "post" ) || !str_prefix(arg, "send"))
    { 
	char *strtime;

/*
      if( ch->level <= 2)
	{
	 send_to_char("You must be level 3 to post notes.\n\r",ch);
	 return;
	}
 */
      if( IS_SET(ch->comm, COMM_NONOTES ) )
	{
	 send_to_char("The gods don't wish to hear from you.\n\r",ch);
	 return;
	}


  if ( ch->pnote == NULL )
  {
      send_to_char( "You have no note in progress.\n\r", ch );
      return;
  }

        if (ch->pnote->type != type)
        {
            send_to_char("You aren't working on that kind of note.\n\r",ch);
            return;
        }

  if ( ch->pnote->type == NOTE_BUG ) 
      {
         free_string( ch->pnote->to_list );
	 sprintf(tempTo, "%s immortal", ch->name);
         ch->pnote->to_list = str_dup( tempTo );
      }

  if (!str_cmp(ch->pnote->to_list,""))
  {
      send_to_char(
    "You need to provide a recipient (name, all, clan or immortal).\n\r",
    ch);
      return;
  }

  if ( is_exact_name( ch->pnote->to_list, "imm" ) ) 
  {
      send_to_char("Notes to 'IMM' don't go anywhere, genius.\n\r",ch);
      return;
  }

        if (strchr(ch->pnote->to_list, '@')!=NULL && ch->level<15)
        {
          send_to_char("You need to be at least level 15 to send notes to other muds.\n\r", ch);
          return;
        }

  if (!str_cmp(ch->pnote->subject,""))
  {
      send_to_char("You need to provide a subject.\n\r",ch);
      return;
  }

  ch->pnote->next     = NULL;
  strtime       = ctime( &current_time );
  strtime[strlen(strtime)-1]  = '\0';
  ch->pnote->date     = str_dup( strtime );
  ch->pnote->date_stamp   = current_time;

       /* handle IMC notes 

       if (strchr(ch->pnote->to_list, '@')!=NULL)
         imc_post_mail(ch, ch->pnote->sender,
                       ch->pnote->to_list,
                       ch->pnote->date,
                       ch->pnote->subject,
                       ch->pnote->text);
	*/

  append_note(ch->pnote);
  if ( is_exact_name( "all", ch->pnote->to_list ) )
  {
    pnet("New post by $N.",ch,NULL,PNET_NOTES,0,get_trust(ch));
  }
  ch->pnote = NULL;

  wiznet("New post by $N.",ch,NULL,WIZ_NOTES,0,get_trust(ch));
  send_to_char ("Thank you for contributing.\n\r",ch);
  return;
    }

    send_to_char( "You can't do that.\n\r", ch );
    return;
}

