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

static char rcsid[] = "$Id: editor.c,v 1.3 2000/04/18 20:33:49 mud Exp $";
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
#include "recycle.h"


#define ID_LINE_CLEAR     1
#define ID_LINE_SHOW      2
#define ID_LINE_APPEND    3
#define ID_LINE_INSERT    4
#define ID_LINE_DELETE    5
#define ID_LINE_REPLACE   6
#define ID_LINE_NUM       7
#define ID_LINE_CANCEL    8
#define ID_LINE_EXIT      9

#define FLA_LINE_NUM      1

/* void edit_line  ( CHAR_DATA *ch, int num ); */
void edit_line_show ( CHAR_DATA *ch, bool show_pos );
void insert_line (CHAR_DATA *ch,char *mode,int ins_after,bool show);
int count_lines ( CHAR_DATA *ch );

/* MENU_DATA line_menu = {
  {"Line Editor","",0,NULL},
  {"[Clear] Buffer","clear",ID_LINE_CLEAR,edit_line},
  {"[Show] Buffer","show",ID_LINE_SHOW,edit_line},
  {"[Append] Lines","append",ID_LINE_APPEND,edit_line},
  {"[Insert] After Line","insert",ID_LINE_INSERT,edit_line},
  {"[Delete] Lines","object",ID_LINE_DELETE,edit_line},
  {"[Replace] Line","replace",ID_LINE_REPLACE,edit_line},
  {"Toggle Line [Numbers]","numbers",ID_LINE_NUM,edit_line},
  {"[Cancel] Changes","cancel",ID_LINE_CANCEL,edit_line},
  {"[Exit] Editor","exit",ID_LINE_EXIT,edit_line},
  {NULL,"",0,NULL}
}; */

char *get_line_from_buf (char *arg,char *buf)
{   
  int idx;
  char c;  
  
  idx = 0;  
  buf[0] = NULL;
  if (!arg || !arg[0]) return NULL;  
  while (idx < MAX_STRING_LENGTH) {
    c = arg[idx];
    switch (c) {          
      case '\n':
        if (arg[idx+1] == '\r') {          
          buf[idx] = NULL;
          idx++;
        }        
      case '\r':
        idx++;
      case '\0':        
        buf[idx] = NULL;
        return arg+idx;        
      default:
        buf[idx] = c;
    }    
    idx++;
  }
  return NULL;  
} 

void do_line_editor ( CHAR_DATA *ch, char *arg, DO_FUN *call_back )
{
  LINE_EDIT_DATA *editor;
  char buf[MAX_STRING_LENGTH];
  CHAR_DATA *prev,*temp;
    
  act("$n enters the line editor.",ch,NULL,NULL,TO_ROOM,TRUE);
  ch->was_in_room = ch->in_room;
  char_from_room (ch);
  temp = ch;
  if (ch->pcdata) {
    if ( ch == char_list )
       char_list = ch->next;
    else {
      for ( prev = char_list; prev != NULL; prev = prev->next ){
          if ( prev->next == ch )  {
        prev->next = ch->next;
        break;
          }
      }
    }
    ch = temp;
    ch->desc->connected = CON_EDITOR;
    
    editor       = new_edit ();
    editor->line = NULL;
    if (arg) {
      LINE_DATA *line,*new_line;      
      
      arg  = get_line_from_buf (arg,buf);
      line = NULL;
      while (arg) {
        new_line = new_line_data ();
        new_line->next = NULL;
        new_line->text = str_dup (buf);
        if (line) 
          line->next = new_line;
        else
          editor->line = new_line;
        line = new_line;
        arg = get_line_from_buf (arg,buf);
      }      
    }
    editor->cur_line   = 0;    
    editor->prev_menu  = ch->pcdata->menu;
    /* ch->pcdata->menu = &line_menu;  */
    editor->call_back  = call_back;    
    ch->pcdata->no_out = TRUE;
    SET_BIT(editor->flags,FLA_LINE_NUM);
    ch->pcdata->line_edit = editor;        
    insert_line (ch,"Type '/help' for a list of commands.",count_lines(ch),TRUE);
  } else {
    return;
  }  
  /* do_menu (ch,NULL); */
}

LINE_DATA *get_line_num ( CHAR_DATA *ch, int num )
{ 
  LINE_DATA *line;
  int idx;
  
  if (num == 0) return NULL;
  idx = 1;
  line = ch->pcdata->line_edit->line;  
  while (line) {
    if (num == idx) 
      return line;
    line = line->next;
    idx++;  
  }
  return NULL;
}

int count_lines ( CHAR_DATA *ch )
{
  LINE_DATA *line;
  int cnt;
  
  cnt  = 0;  
  line = ch->pcdata->line_edit->line;
  while (line) {
    cnt++;
    line = line->next;
  }
  return cnt;
}

LINE_DATA *prev_line ( CHAR_DATA *ch, LINE_DATA *line )
{
  LINE_DATA *head,*prev;
  
  head = ch->pcdata->line_edit->line;
  prev = NULL;
  while (head) {
    if (head == line) return prev;
    prev = line;
    line = line->next;    
  }
  
  return NULL;
}

void edit_line_show ( CHAR_DATA *ch, bool show_pos )
{ 
  LINE_DATA      *line; 
  char buf[MAX_STRING_LENGTH];
  int idx = 1;
    
  line = ch->pcdata->line_edit->line;
  if (line) {    
    while (line) {
      if (IS_SET(ch->pcdata->line_edit->flags,FLA_LINE_NUM))
        sprintf (buf,"%2d:%s\n\r",idx,line->text);
      else
        sprintf (buf,"%s\n\r",line->text);
      send_to_char (buf,ch);
      if (show_pos) {
        if (idx == ch->pcdata->line_edit->cur_line) {
          send_to_char ("-->\n\r",ch);
        }
      }
      idx++;
      line = line->next;
    }  
  } else {
    send_to_char ("No lines in buffer.\n\r",ch);
  }    
}

void clear_buffer (CHAR_DATA *ch)
{
  LINE_DATA *line, *next;

  line = ch->pcdata->line_edit->line;    
  next = NULL;
  while (line) {
    free_string (line->text);
    next = line->next;
    free_line_data (line);
    line = next;
  }
  ch->pcdata->line_edit->line = NULL;        
}

void insert_info (CHAR_DATA *ch, char *mode, bool marker)
{
  char buf[MAX_STRING_LENGTH];

  if (marker && mode)  {
    if (IS_SET(ch->pcdata->line_edit->flags,FLA_LINE_NUM)) {
      sprintf (buf,"%-75s75|\n\r",mode);
    } else {
      sprintf (buf,"%-72s75|\n\r",mode);
    }
    send_to_char (buf,ch);
  } else {  
    if (IS_SET(ch->pcdata->line_edit->flags,FLA_LINE_NUM)) {
      sprintf (buf,"%2d:",ch->pcdata->line_edit->cur_line);
    } else {
      sprintf (buf,":");
    }      
    send_to_char (buf,ch);
  }  
}

void get_insert_line_num (CHAR_DATA *ch,char *arg)
{  
  int line_num;
  
  if (!is_number(arg)) {
    send_to_char ("Invalid line num.\n\r",ch);
    insert_info (ch,NULL,FALSE);
    return;
  } else {
    line_num = atoi (arg);
    
    if (line_num > count_lines (ch)) {
      line_num = count_lines (ch);
    }
    if (line_num < 0) {
      line_num = 0;
    }
    insert_line (ch,NULL,line_num,FALSE);
  }  
}

void get_delete_line_2 (CHAR_DATA *ch, char *arg, char *arg2)
{
  int num1,num2,t;
  LINE_DATA *line,*temp;
  
  num1 = atoi (arg);
  if (arg2) num2 = atoi(arg2);
  else num2 = num1;
  if (num2 < num1) {
    send_to_char ("Second line number must be greater than the first.\n\r",ch);
    insert_info (ch,NULL,FALSE);
    return;    
  } 
  if (num1 < 1) {
    send_to_char ("First line must be 1 or greater.\n\r",ch);
    insert_info (ch,NULL,FALSE);
    return;        
  }
  
  for (t = 0; t <= (num2 - num1); t++) {
    line = get_line_num (ch,num1-1);
    if (line) {
      temp = line->next;
      if (temp) {
        line->next = temp->next;
        free_string (temp->text);
        free_line_data (temp);
      } else {
        line->next = NULL;
        break;
      }
      ch->pcdata->line_edit->cur_line--;
    } else {
      if (num1 == 1) {
        line = ch->pcdata->line_edit->line;
        if (line) {
          temp = line->next;
          free_string (line->text);
          free_line_data (line);
          ch->pcdata->line_edit->line = temp;
          ch->pcdata->line_edit->cur_line--;
        } else {
          send_to_char ("Line number does not exist.\n\r",ch);
          break;        
        }        
      } else {
        send_to_char ("Line number does not exist.\n\r",ch);
        break;
      }
    }      
  }
  t = count_lines (ch);
  if (ch->pcdata->line_edit->cur_line > t) 
    ch->pcdata->line_edit->cur_line = t + 1;
  insert_info (ch,NULL,FALSE);    
}

void get_delete_line_1 (CHAR_DATA *ch,char *arg)
{
  if (arg[0] == NULL) {
    send_to_char ("Deletion cancelled.\n\r",ch);
    insert_info (ch,NULL,FALSE);
    return;
  }
  get_delete_line_2 (ch,arg,NULL);
}

void insert_line_callback (CHAR_DATA *ch, char *arg)
{  
  char arg2[MAX_STRING_LENGTH/2];
  char arg3[MAX_STRING_LENGTH/2];

  if (arg[0] == '/') {    
    arg = one_argument (arg,arg2);
    if (!str_prefix (&arg2[1],"help") || !arg2[1] || (arg2[1] == '?')) {
      send_to_char ("Available options:\n\r",ch);
      send_to_char ("  /help        - this menu\n\r",ch);
      send_to_char ("  /back        - deletes the previous line\n\r",ch);
      send_to_char ("  /clear       - clears the current buffer\n\r",ch);
      send_to_char ("  /insert #    - insert before line #\n\r",ch);
      send_to_char ("  /delete # #  - deletes the specified line #'s\n\r",ch);
      send_to_char ("  /linenum     - toggles line numbers\n\r",ch);
      send_to_char ("  /show        - shows current buffer\n\r",ch);
      send_to_char ("  /cancel      - cancels editing\n\r",ch);
      send_to_char ("  /done        - quits and saves\n\r",ch);
      insert_info (ch,NULL,FALSE);
      return;
    }
    if (!str_prefix (&arg2[1],"done")) {
      char bigbuf[4*MAX_STRING_LENGTH];
      LINE_DATA *line;
      
      ch->next        = char_list;
      char_list       = ch;      
      ch->desc->connected = CON_PLAYING;
      char_to_room (ch,ch->was_in_room);
      act("$n comes back from the line editor.",ch,NULL,NULL,TO_ROOM,TRUE);

      line = ch->pcdata->line_edit->line;
      bigbuf[0] = NULL;
      while (line) {
        strcat (bigbuf,line->text);
        strcat (bigbuf,"\n\r");
        line = line->next;
      } 
      (*ch->pcdata->line_edit->call_back) (ch,bigbuf);
      ch->pcdata->menu = ch->pcdata->line_edit->prev_menu;      
      clear_buffer (ch);
      free_edit (ch->pcdata->line_edit);
      ch->pcdata->line_edit = NULL;
      ch->pcdata->no_out = FALSE;
      do_menu (ch,NULL);
      return;
    }
    if (!str_prefix (&arg2[1],"show")) {
      edit_line_show (ch, FALSE);
      insert_info (ch,NULL,FALSE);
      return;
    }
    if (!str_prefix (&arg2[1],"back")) {
      LINE_DATA *line,*temp;
      
      line = get_line_num (ch,ch->pcdata->line_edit->cur_line-2);
      if (line) {
        temp = line->next;
        line->next = line->next->next;        
        free_string (temp->text);
        free_line_data (temp);
        ch->pcdata->line_edit->cur_line--; 
      } else {        
        if (ch->pcdata->line_edit->cur_line == 2) {          
          line = ch->pcdata->line_edit->line;
          temp = line->next;
          free_string (line->text);
          free_line_data (line);
          ch->pcdata->line_edit->line = temp;
          ch->pcdata->line_edit->cur_line--;
        } else {
          send_to_char ("No previous lines.\n\r",ch);          
        }
      }
      insert_info (ch,NULL,FALSE);
      return;
    }
    if (!str_prefix (&arg2[1],"linenum")) {
      TOGGLE_BIT (ch->pcdata->line_edit->flags,FLA_LINE_NUM);
      send_to_char ("Line Numbers:  ",ch);
      if (IS_SET(ch->pcdata->line_edit->flags,FLA_LINE_NUM))
        send_to_char ("On\n\r",ch);
      else
        send_to_char ("Off\n\r",ch);        
      insert_info (ch,NULL,FALSE);
      return;
    }
    if (!str_prefix (&arg2[1],"cancel")) {
      ch->next        = char_list;
      char_list       = ch;      
      ch->desc->connected = CON_PLAYING;
      char_to_room (ch,ch->was_in_room);
      act("$N comes back from the line editor.",ch,NULL,NULL,TO_ROOM,TRUE);
    
      ch->pcdata->menu = ch->pcdata->line_edit->prev_menu;      
      clear_buffer (ch);
      free_edit (ch->pcdata->line_edit);
      ch->pcdata->line_edit = NULL;
      ch->pcdata->no_out = FALSE;
      do_menu (ch,NULL);
      return;        
    }
    if (!str_prefix (&arg2[1],"clear")) {
      clear_buffer (ch);
      ch->pcdata->line_edit->cur_line = 1;
      send_to_char ("Buffer cleared.\n\r",ch);
      insert_info (ch,NULL,FALSE);
      return;          
    }
    if (!str_prefix (&arg2[1],"delete")) {
      arg = one_argument (arg,arg2);
      if (arg2[0] == NULL) {
        send_to_char ("Enter line to delete:  ",ch);
        ch->pcdata->interp_fun = &get_delete_line_1;
        return;
      } else {
        arg = one_argument (arg,arg3);
        if (arg3[0] == NULL) {
          get_delete_line_2 (ch,arg2,NULL);
          return;
        } else {
          get_delete_line_2 (ch,arg2,arg3);
          return;
        }
      }      
    
    }
    if (!str_prefix (&arg2[1],"insert")) {
      
      arg = one_argument (arg,arg2);
      if (arg2[0] == NULL) {
        send_to_char ("Insert after line:  ",ch);
        ch->pcdata->interp_fun = &get_insert_line_num;
        return;
      } else {
        get_insert_line_num (ch,arg2);
        return;
      }      
    }
    send_to_char ("Invalid option.  Type '/help' for a list of commands.\n\r",ch);    
    insert_info (ch,NULL,FALSE);
    return;
  } else {
    LINE_DATA *line,*new_line;
    
    line = get_line_num (ch,ch->pcdata->line_edit->cur_line-1);
    new_line = new_line_data ();
    new_line->next = NULL;
    new_line->text = str_dup (arg);
    if (line) {
      new_line->next = line->next;
      line->next = new_line;
    } else {
      if (count_lines(ch) > 0) {
        send_to_char ("Error getting line number.\n\r",ch);
        free_string (new_line->text);
        free_line_data (new_line);
        return;
      } else {
        ch->pcdata->line_edit->line = new_line;
      }      
    }
    ch->pcdata->line_edit->cur_line++;
  }  
  insert_info (ch,NULL,FALSE);
}

void insert_line (CHAR_DATA *ch,char *mode,int ins_after,bool show)
{
  if (show) {
    insert_info (ch,mode,TRUE);
    edit_line_show (ch,FALSE);
  }
  ch->pcdata->interp_fun = insert_line_callback;
  ch->pcdata->line_edit->cur_line = ins_after+1;
  insert_info (ch,NULL,FALSE);
}

/* void edit_line  ( CHAR_DATA *ch, int num )
{
  char bigbuf[4*MAX_STRING_LENGTH];
  LINE_DATA *line;
  
  switch (num) {
    case ID_LINE_CLEAR:      
      clear_buffer (ch);
      ch->pcdata->line_edit->cur_line = 0;
      send_to_char ("Buffer cleared.\n\r>  ",ch);
      break;
    case ID_LINE_SHOW:
      edit_line_show (ch, FALSE);
      send_to_char (">  ",ch);
      break;
    case ID_LINE_APPEND:      
      insert_line (ch,"Type '/help' for a list of commands.",count_lines(ch),FALSE);
      break;
    case ID_LINE_INSERT:
      break;
    case ID_LINE_DELETE:
      break;
    case ID_LINE_REPLACE:
      break;  
    case ID_LINE_NUM:
      TOGGLE_BIT (ch->pcdata->line_edit->flags,FLA_LINE_NUM);
      send_to_char ("Line Numbers:  ",ch);
      if (IS_SET(ch->pcdata->line_edit->flags,FLA_LINE_NUM))
        send_to_char ("On\n\r>  ",ch);
      else
        send_to_char ("Off\n\r>  ",ch);        
      break;      
    case ID_LINE_CANCEL:
      ch->pcdata->menu = ch->pcdata->line_edit->prev_menu;      
      clear_buffer (ch);
      free_edit (ch->pcdata->line_edit);
      ch->pcdata->no_out = FALSE;
      do_menu (ch,NULL);
      return;    
    case ID_LINE_EXIT:
      line = ch->pcdata->line_edit->line;
      bigbuf[0] = NULL;
      while (line) {
        strcat (bigbuf,line->text);
        strcat (bigbuf,"\n\r");
        line = line->next;
      } 
      (*ch->pcdata->line_edit->call_back) (ch,bigbuf);
      ch->pcdata->menu = ch->pcdata->line_edit->prev_menu;      
      clear_buffer (ch);
      free_edit (ch->pcdata->line_edit);
      ch->pcdata->no_out = FALSE;
      do_menu (ch,NULL);
      return;
  }
} */
