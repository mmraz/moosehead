/***
Copyright 1996  Michael "Rusty" Mraz  rusty@moosehead.com

All rights reserved.  No part of this code shall be reproduced, stored in
a retrieval system, or transmitted by any means electronic, mechanical or
otherwise without written permission from the author.  No liability is
assumed with respect to the use of this code.

This header file is not to be removed.
***/
static char rcsid[] = "$Id: ident.c,v 1.4 2002/03/09 19:14:28 rusty Exp $";

/* Yes, I know some of these includes may not be necessary, but
   hey...  I like to be thorough :)
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
// #include <sys/msg.h>
#include "merc.h"
#include "recycle.h"


void do_ident( CHAR_DATA *ch, char *argument )
{
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *victim;
  int fd, len, i;
  struct hostent *hp;
  struct sockaddr_in addr;
  static struct sockaddr_in b_addr;
  int addrlen;
  FILE *fp_in, *fp_out;
  int lport, fport;
  char host[200],name[56];
  char buffer[1024];
  char stder[1024];
  char buf[1024];

  strcpy(stder, "\n\r");
    one_argument( argument, arg );
    if ( arg[0] == '\0' )
    {
  send_to_char( "Ident whom?\n\r", ch );
  return;
    }

    if ( ((victim = get_char_world( ch, arg ) ) == NULL)
         || (victim->level > ch->level) )
    {
  send_to_char( "They aren't here.\n\r", ch );
  return;
    }

    if ( victim->desc == NULL )
    {
  act( "$N doesn't have a descriptor.", ch, NULL, victim, TO_CHAR ,TRUE);
  return;
    }

  /* Put character names you want to hide from do_ident() here */
    if ( !strcmp(victim->name, "Rusty") ) return;

  signal( SIGPIPE, SIG_IGN );    
 
	strcpy(name,victim->name);
	strcpy(host,victim->desc->host);
	lport = victim->desc->port;
	fport = 4000;
/* Uncomment for debugging *
  sprintf( stder, "IdentInfo: %s %s %d %d", name,host,lport,fport);
  log_string( stder );
*/
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
      sprintf (stder,"Ident:  socket_error");
      log_string( stder );
      return;
    }

    addr = b_addr;
    addr.sin_family = AF_INET;
    if (isdigit(host[0]))
      addr.sin_addr.s_addr = inet_addr(host);
    else {
	printf("GOOBER 1");
      if((hp = gethostbyname(host)) == NULL) {
  /*      sprintf(stder, "Host not found (%s)\n\r", host);
	send_to_char( stder, ch);*/
        return;
      }      
      memcpy(&addr.sin_addr, hp->h_addr, sizeof(addr.sin_addr));
    }
    addr.sin_port = htons(113);
    addrlen = sizeof(addr);

    if (connect(fd, (struct sockaddr *)&addr, addrlen) < 0) {
    sprintf(stder, "Connection failed for host: %s\n\r",host);
    log_string(stder);
    send_to_char( stder, ch);
    return;
    }     


    fp_in  = fdopen(fd, "r");
    fp_out = fdopen(fd, "w");
    if (!fp_in || !fp_out) {
      sprintf(stder,"Error fdopen for host %s.\n\r",host); 
      send_to_char(stder, ch);
      return;
    }     

    fprintf(fp_out, "%d, %d\n", lport, fport);
    fflush(fp_out);

    if (fgets(buffer, sizeof(buffer)-1, fp_in) == NULL) {
      sprintf(stder,"Error fgets for host %s.\n\r",host);
      send_to_char(stder, ch);
      return;
    }
    
    len = strlen(buffer);
    while ((buffer[len] != ' ') && (buffer[len] != ':') && (len > 0)) {
      len--;
    }
    strncpy (name,&buffer[len+1],25);
    for ( i=0 ; i <= 23 ; i++ )
    {
     if ( name[i] == '\n' || name[i] == '\r' )
     {
      name[i] = ' ';
      name[i+1] = '\0';
     }
    }
    /* Uncomment for all return values on a single line */
    /*name[24] = '\0';*/
    sprintf(buf,"%s\n\r@%s",name,host);

    sprintf(stder, "%s = %s@%s\n\r",victim->name,name,host);
    log_string(stder);

    strcat(buf,"\n\r");
    send_to_char(buf, ch);

    fclose(fp_out);
    fclose(fp_in);
    close (fd);

	return;
  }
