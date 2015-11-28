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

static char rcsid[] = "$Id: dns.c,v 1.5 1999/11/24 15:57:43 mud Exp $";
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
#include "merc.h"
#include "recycle.h"

DNS_DATA *dns_list = NULL;

void save_dns(void)
{
    DNS_DATA *pdns;
    FILE *fp;
    bool found = FALSE;

//    fclose( fpReserve ); 
    if ( ( fp = fopen( DNS_FILE, "w" ) ) == NULL )
    {
        perror( NOTE_FILE );
    }

    for (pdns = dns_list; pdns != NULL; pdns = pdns->next)
    {
      fprintf(fp,"%-20s\n",pdns->name); 
      found = TRUE;
     }

     fclose(fp);
//     fpReserve = fopen( NULL_FILE, "r" );
     if (!found)
  unlink(DNS_FILE);
}

void load_dns(void)
{
    FILE *fp;
    DNS_DATA *dns_last;
 
    if ( ( fp = fopen( DNS_FILE, "r" ) ) == NULL )
        return;
 
    dns_last = NULL;
    for ( ; ; )
    {
        DNS_DATA *pdns;
        if ( feof(fp) )
        {
            fclose( fp );
            return;
        }
 
        pdns = new_dns();
 
        pdns->name = str_dup(fread_word(fp));
  	fread_to_eol(fp);

        if (dns_list == NULL)
      dns_list = pdns;
  else
      dns_last->next = pdns;
  dns_last = pdns;
    }
}

bool check_dns(char *site)
{
    DNS_DATA *pdns;

    for ( pdns = dns_list; pdns != NULL; pdns = pdns->next ) 
    {
     if(!str_prefix(pdns->name,site))
      return TRUE;
    }

    return FALSE;
}


void dns_site(CHAR_DATA *ch, char *argument)
{
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char *name;
    BUFFER *buffer;
    DNS_DATA *pdns, *prev;

    argument = one_argument(argument,arg1);

    if ( arg1[0] == '\0' )
    {
  if (dns_list == NULL)
  {
      send_to_char("No sites dns'ed at this time.\n\r",ch);
      return;
    }
  buffer = new_buf();

        add_buf(buffer,"DNS'ed sites:\n\r");
        for (pdns = dns_list;pdns != NULL;pdns = pdns->next)
        {
      sprintf(buf,"%-20s\n\r", pdns->name);
      add_buf(buffer,buf);
        }

    if( ch != NULL )
        page_to_char( buf_string(buffer), ch );
  free_buf(buffer);
        return;
    }

    name = arg1;

    prev = NULL;
    for ( pdns = dns_list; pdns != NULL; prev = pdns, pdns = pdns->next )
    {

     if (!str_cmp(name,pdns->name))
      {
      if (prev == NULL)
        dns_list = pdns->next;
      else
        prev->next = pdns->next;
      free_dns(pdns);
      }
    }

    pdns = new_dns();
    pdns->name = str_dup(name);
    pdns->next  = dns_list;
    dns_list    = pdns;
    save_dns();
    sprintf(buf,"%s has been dns'ed.\n\r",pdns->name);
    if( ch != NULL )
      send_to_char( buf, ch );
    return;
}

void do_dns(CHAR_DATA *ch, char *argument)
{
    dns_site(ch, argument);
}

void do_undns( CHAR_DATA *ch, char *argument )                        
{
    char arg[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    DNS_DATA *prev;
    DNS_DATA *curr;

    one_argument( argument, arg );

    if ( arg[0] == '\0' )
    {
        send_to_char( "Remove which site from the dns list?\n\r", ch );
        return;
    }

    prev = NULL;
    for ( curr = dns_list; curr != NULL; prev = curr, curr = curr->next )
    {
        if ( !str_cmp( arg, curr->name ) )
        {
            if ( prev == NULL )
                dns_list   = dns_list->next;
            else
                prev->next = curr->next;

            free_dns(curr);
      sprintf(buf,"DNS on %s lifted.\n\r",arg);
            send_to_char( buf, ch );
      save_dns();
            return;
        }
    }

    send_to_char( "Site is not dns'ed\n\r", ch );
    return;
}


