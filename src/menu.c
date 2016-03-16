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

static char rcsid[] = "$Id: menu.c,v 1.2 1999/07/14 18:14:01 mud Exp $";
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

void do_menu ( CHAR_DATA *ch, char *arg )
{
  int choice, t;
  MENU_DATA *menu; 
  char buf[100];

  menu = ch->pcdata->menu;
  if (menu == NULL) {
      ch->pcdata->interp_fun = NULL;
      bug ("NULL menu in do_menu",0);
      return;
  }
  
  if (arg && (arg[0] == '/' || arg[0] == '#')) {
    ch->pcdata->interp_fun = NULL;
    interpret (ch,&arg[1]);
    ch->pcdata->interp_fun = &do_menu;
    send_to_char ("\n\r>  ",ch);
    return;
  }    

  ch->pcdata->interp_fun = &do_menu;
  if (arg) one_argument(arg,arg);  
    
  if (!arg || !arg[0] ) {
    if (ch->pcdata->macro_count) return;
    sprintf (buf,"\n\r-= %s =-\n\r",*menu[0].text);
    send_to_char (buf,ch);
    if (*menu[0].menu_fun)
      (**menu[0].menu_fun) (ch,0);
    for (t = 0;; t++) {
      if ( *menu[t].text == NULL ) {
        break;
      }
    }    
    if ((arg == NULL) && IS_SET (ch->comm,COMM_BRIEF_MENUS)) {
      send_to_char (">  ",ch);
      return;
    }
    if (menu[0].id > 0) {  /* multi-column */
      char format[50];
      sprintf (format,"%%d.  %%-%ds%%s",menu[0].id);

      for (choice = 1;; choice++) {      
        if (( *menu[choice].text == NULL ) || (choice > t/2)) {
          send_to_char (">  ", ch);  
          return;
        } else {
          sprintf (buf,format,choice,*menu[choice].text,
            (choice < 10) ? " ":"");
          send_to_char (buf,ch);
          if (*menu[choice+t/2].text) {
            sprintf (buf,"%d.  %s\n\r",choice+t/2,*menu[choice+t/2].text);
            send_to_char (buf,ch);
          } else  {
            send_to_char ("\n\r>  ", ch);  
            return;
          }
        }
      }
    } else {                /* single column */
      for (choice = 1;; choice++) {
        if ( *menu[choice].text == NULL ) {
          send_to_char (">  ", ch);  
          return;
        } else {
          sprintf (buf,"%d.  %s\n\r",choice,*menu[choice].text);
          send_to_char (buf,ch);
        }
      }
    }
    return;                                            
  }
  if (is_number (arg)) {
    choice = atoi (arg);
    for (t = 1; t <= choice; t++) {
      if (*menu[t].text == NULL) {
        send_to_char ("Invalid choice.\n\r>  ", ch);  
        return;
      }
    }
  } else {
    for (choice = 1;; choice++) {
      if ( *menu[choice].text == NULL ) {
        send_to_char ("Invalid choice.\n\r>  ", ch);  
        return;
      } else {
        if (!str_prefix (arg, *menu[choice].context)) {
          break;
        }
      }
    }
  }
  if (*menu[choice].menu_fun)
    (**menu[choice].menu_fun) (ch, menu[choice].id);
  else
    bug ("NULL for menu fuction in do_menu",0);
}
