/*
 * IMC2 - an inter-mud communications protocol
 *
 * imc-interp.c: packet interpretation code
 *
 * Copyright (C) 1996,1997 Oliver Jowett <oliver@jowett.manawatu.planet.co.nz>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program (see the file COPYING); if not, write to the
 * Free Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include "imc.h"

/* rignore'd people */
imc_ignore_data *imc_ignore_list;
/* prefixes for all data files */
char *imc_prefix;

/* called when a keepalive has been received */
void imc_recv_keepalive(const char *from, const char *version, const char *flags)
{
  imc_reminfo *p;

  if (!strcasecmp(from, imc_name))
    return;
  
  /*  this should never fail, imc.c should create an
   *  entry if one doesn't exist (in the path update
   *  code)
   */
  p=imc_find_reminfo(from, 0);
  if (!p)		    /* boggle */
    return;

  if (imc_hasname(flags, "hide"))
    p->hide=1;
  else
    p->hide=0;
  
  /* lower-level code has already updated p->alive */

  if (strcasecmp(version, p->version))    /* remote version has changed? */
  {
    imc_strfree(p->version);              /* if so, update it */
    p->version=imc_strdup(version);
  }

  /* Only routers should ping - and even then, only directly connected muds */
  if (imc_is_router && imc_getinfo(from))
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    imc_send_ping(from, tv.tv_sec, tv.tv_usec);
  }
}

/* called when a ping request is received */
void imc_recv_ping(const char *from, int time_s, int time_u, const char *path)
{
  /* ping 'em back */
  imc_send_pingreply(from, time_s, time_u, path);
}

/* called when a ping reply is received */
void imc_recv_pingreply(const char *from, int time_s, int time_u, const char *pathto, const char *pathfrom)
{
  imc_reminfo *p;
  struct timeval tv;

  p = imc_find_reminfo(from, 0);   /* should always exist */
  if (!p)			   /* boggle */
    return;

  gettimeofday(&tv, NULL);      /* grab the exact time now and calc RTT */
  p->ping = (tv.tv_sec - time_s) * 1000 + (tv.tv_usec - time_u) / 1000;

  /* check for pending traceroutes */
  imc_traceroute(p->ping, pathto, pathfrom);
}

/* send a standard 'you are being ignored' rtell */
void imc_sendignore(const char *to)
{
  char buf[IMC_DATA_LENGTH];

  if (strcmp(imc_nameof(to), "*"))
  {
    sprintf(buf, "%s is ignoring you.", imc_name);
    imc_send_tell(NULL, to, buf, 1);
  }
}

/* imc_char_data representation:
 *
 *  Levels are simplified: >0 is a mortal, <0 is an immortal. The 'see' and
 *  'invis' fields are no longer used.
 *
 *  d->wizi  is -1 if the character is invisible to mortals (hidden/invis or
 *           wizi)
 *  d->level is the level of the character (-1=imm, 1=mortal)
 *
 *  also checks rignores for a 'notrust' flag which makes that person a
 *  level 1 mortal for the purposes of wizi visibility checks, etc
 *
 *  Default behavior is now: trusted.
 *  If there's a notrust flag, untrusted. If there's also a trust flag, trusted
 */

/* convert from the char data in 'p' to an internal representation in 'd' */
static void getdata(const imc_packet *p, imc_char_data *d)
{
  int trust=1;

  if (imc_findignore(p->from, IMC_NOTRUST))
    trust=0;
  if (imc_findignore(p->from, IMC_TRUST))
    trust=1;

  strcpy(d->name, p->from);
  d->wizi = trust ? imc_getkeyi(&p->data, "wizi", 0) : 0;
  d->level = trust ? imc_getkeyi(&p->data, "level", 0) : 0;
  d->invis = 0;
}

/* convert back from 'd' to 'p' */
static void setdata(imc_packet *p, const imc_char_data *d)
{
  imc_initdata(&p->data);

  if (!d)
  {
    strcpy(p->from, "*");
    imc_addkeyi(&p->data, "level", -1);
    return;
  }

  strcpy(p->from, d->name);

  if (d->wizi)
    imc_addkeyi(&p->data, "wizi", d->wizi);
  imc_addkeyi(&p->data, "level", d->level);
}

/* handle a packet destined for us, or a broadcast */
void imc_recv(const imc_packet *p)
{
  imc_char_data d;
  int bcast;

  bcast=!strcmp(imc_mudof(p->i.to), "*") ? 1 : 0;
  
  getdata(p, &d);

  /* chat: message to a channel (broadcast) */
  if (!strcasecmp(p->type, "chat") && !imc_isignored(p->from))
    imc_recv_chat(&d, imc_getkeyi(&p->data, "channel", 0),
		  imc_getkey(&p->data, "text", ""));

  /* emote: emote to a channel (broadcast) */
  else if (!strcasecmp(p->type, "emote") && !imc_isignored(p->from))
    imc_recv_emote(&d, imc_getkeyi(&p->data, "channel", 0),
		   imc_getkey(&p->data, "text", ""));

  /* tell: tell a player here something */
  else if (!strcasecmp(p->type, "tell"))
    if (imc_isignored(p->from))
      imc_sendignore(p->from);
    else
      imc_recv_tell(&d, p->to, imc_getkey(&p->data, "text", ""),
		    imc_getkeyi(&p->data, "isreply", 0));

  /* who-reply: receive a who response */
  else if (!strcasecmp(p->type, "who-reply"))
    imc_recv_whoreply(p->to, imc_getkey(&p->data, "text", ""),
		      imc_getkeyi(&p->data, "sequence", -1));

  /* who: receive a who request */
  else if (!strcasecmp(p->type, "who"))
    if (imc_isignored(p->from))
      imc_sendignore(p->from);
    else
      imc_recv_who(&d, imc_getkey(&p->data, "type", "who"));

  /* whois-reply: receive a whois response */
  else if (!strcasecmp(p->type, "whois-reply"))
    imc_recv_whoisreply(p->to, imc_getkey(&p->data, "text", ""));

  /* whois: receive a whois request */
  else if (!strcasecmp(p->type, "whois"))
    imc_recv_whois(&d, p->to);

  /* beep: beep a player */
  else if (!strcasecmp(p->type, "beep"))
    if (imc_isignored(p->from))
      imc_sendignore(p->from);
    else
      imc_recv_beep(&d, p->to);

  /* is-alive: receive a keepalive (broadcast) */
  else if (!strcasecmp(p->type, "is-alive"))
    imc_recv_keepalive(imc_mudof(p->from),
		       imc_getkey(&p->data, "versionid", "unknown"),
		       imc_getkey(&p->data, "flags", ""));

  /* ping: receive a ping request */
  else if (!strcasecmp(p->type, "ping"))
    imc_recv_ping(imc_mudof(p->from), imc_getkeyi(&p->data, "time-s", 0),
		  imc_getkeyi(&p->data, "time-us", 0), p->i.path);

  /* ping-reply: receive a ping reply */
  else if (!strcasecmp(p->type, "ping-reply"))
    imc_recv_pingreply(imc_mudof(p->from), imc_getkeyi(&p->data, "time-s", 0),
		       imc_getkeyi(&p->data, "time-us", 0),
		       imc_getkey(&p->data, "path", NULL), p->i.path);

  /* mail: mail something to a local player */
  else if (!strcasecmp(p->type, "mail"))
    imc_recv_mail(imc_getkey(&p->data, "from", "error@hell"),
		  imc_getkey(&p->data, "to", "error@hell"),
		  imc_getkey(&p->data, "date", "(IMC error: bad date)"),
		  imc_getkey(&p->data, "subject", "no subject"),
		  imc_getkey(&p->data, "id", "bad_id"),
		  imc_getkey(&p->data, "text", ""));

  /* mail-ok: remote confirmed that they got the mail ok */
  else if (!strcasecmp(p->type, "mail-ok"))
    imc_recv_mailok(p->from, imc_getkey(&p->data, "id", "bad_id"));

  /* mail-reject: remote rejected our mail, bounce it */
  else if (!strcasecmp(p->type, "mail-reject"))
    imc_recv_mailrej(p->from, imc_getkey(&p->data, "id", "bad_id"),
		     imc_getkey(&p->data, "reason",
				"(IMC error: no reason supplied"));

  else if (!strcasecmp(p->type, "info-request"))
    imc_recv_inforequest(p->from, imc_getkey(&p->data, "category", ""));

  /* call catch-all fn if present */
  else
  {
    imc_packet out;

    if (imc_recv_hook)
      if ((*imc_recv_hook)(p, bcast))
	return;

    if (bcast || !strcasecmp(p->type, "reject"))
      return;
    
    /* reject packet */
      
    strcpy(out.type, "reject");
    strcpy(out.to, p->from);
    strcpy(out.from, p->to);

    imc_clonedata(&p->data, &out.data);
    imc_addkey(&out.data, "old-type", p->type);

    imc_send(&out);
    imc_freedata(&out.data);
  }
}


/* Commands called by the interface layer */

/* return mud information.
 * yes, this is protocol level, and -required-
 */
void imc_recv_inforequest(const char *from, const char *category)
{
  imc_packet reply;

  strcpy(reply.to, from);
  strcpy(reply.from, "*");

  imc_initdata(&reply.data);

  if (imc_isignored(from))
  {
    strcpy(reply.type, "info-unavailable");
    imc_send(&reply);
  }
  else if (!strcasecmp(category, "site"))
  {
    strcpy(reply.type, "info-reply");
    
    imc_addkey(&reply.data, "name",    imc_siteinfo.name);
    imc_addkey(&reply.data, "host",    imc_siteinfo.host);
    imc_addkey(&reply.data, "email",   imc_siteinfo.email);
    imc_addkey(&reply.data, "imail",   imc_siteinfo.imail);
    imc_addkey(&reply.data, "www",     imc_siteinfo.www);
    imc_addkey(&reply.data, "version", IMC_VERSIONID);
    imc_addkey(&reply.data, "details", imc_siteinfo.details);
    imc_addkey(&reply.data, "flags",   imc_siteinfo.flags);

    imc_send(&reply);
  }
  else
  {
    strcpy(reply.type, "info-unavailable");
    imc_send(&reply);
  }

  imc_freedata(&reply.data);
}

/* send a message out on a channel */
void imc_send_chat(const imc_char_data *from, int channel,
		   const char *argument, const char *to)
{
  imc_packet out;
  char tobuf[IMC_MNAME_LENGTH];

  if (imc_active<IA_UP)
    return;

  setdata(&out, from);

  strcpy(out.type, "chat");
  strcpy(out.to, "*@*");
  imc_addkey(&out.data, "text", argument);
  imc_addkeyi(&out.data, "channel", channel);

  to=imc_getarg(to, tobuf, IMC_MNAME_LENGTH);
  while (tobuf[0])
  {
    if (!strcmp(tobuf, "*") || !strcasecmp(tobuf, imc_name) ||
	imc_find_reminfo(tobuf, 0))
    {
      strcpy(out.to, "*@");
      strcat(out.to, tobuf);
      imc_send(&out);
    }

    to=imc_getarg(to, tobuf, IMC_MNAME_LENGTH);
  }

  imc_freedata(&out.data);
}

/* send an emote out on a channel */
void imc_send_emote(const imc_char_data *from, int channel,
		    const char *argument, const char *to)
{
  imc_packet out;
  char tobuf[IMC_MNAME_LENGTH];

  if (imc_active<IA_UP)
    return;

  setdata(&out, from);

  strcpy(out.type, "emote");
  imc_addkeyi(&out.data, "channel", channel);
  imc_addkey(&out.data, "text", argument);

  to=imc_getarg(to, tobuf, IMC_MNAME_LENGTH);
  while (tobuf[0])
  {
    if (!strcmp(tobuf, "*") || !strcasecmp(tobuf, imc_name) ||
	imc_find_reminfo(tobuf, 0))
    {
      strcpy(out.to, "*@");
      strcat(out.to, tobuf);
      imc_send(&out);
    }

    to=imc_getarg(to, tobuf, IMC_MNAME_LENGTH);
  }

  imc_freedata(&out.data);
}

/* send a tell to a remote player */
void imc_send_tell(const imc_char_data *from, const char *to,
		   const char *argument, int isreply)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (!strcmp(imc_mudof(to), "*"))
    return; /* don't let them do this */

  setdata(&out, from);

  imc_sncpy(out.to, to, IMC_NAME_LENGTH);
  strcpy(out.type, "tell");
  imc_addkey(&out.data, "text", argument);
  if (isreply)
    imc_addkeyi(&out.data, "isreply", isreply);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* send a who-request to a remote mud */
void imc_send_who(const imc_char_data *from, const char *to, const char *type)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (!strcmp(imc_mudof(to), "*"))
    return; /* don't let them do this */

  setdata(&out, from);

  sprintf(out.to, "*@%s", to);
  strcpy(out.type, "who");

  imc_addkey(&out.data, "type", type);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* respond to a who request with the given data */
void imc_send_whoreply(const char *to, const char *data, int sequence)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (!strcmp(imc_mudof(to), "*"))
    return; /* don't let them do this */

  imc_initdata(&out.data);

  imc_sncpy(out.to, to, IMC_NAME_LENGTH);
  strcpy(out.type, "who-reply");
  strcpy(out.from, "*");
  imc_addkey(&out.data, "text", data);
  if (sequence!=-1)
    imc_addkeyi(&out.data, "sequence", sequence);
  
  imc_send(&out);
  imc_freedata(&out.data);
}

/* special handling of whoreply construction for sequencing */
static char *wr_to;
static char *wr_buf;
static int wr_sequence;

void imc_whoreply_start(const char *to)
{
  wr_sequence=0;
  wr_to=imc_strdup(to);
  wr_buf=imc_getsbuf(IMC_DATA_LENGTH);
}

void imc_whoreply_add(const char *text)
{
  /* give a bit of a margin for error here */
  if (strlen(wr_to) + strlen(text) >= IMC_DATA_LENGTH-500)
  {
    imc_send_whoreply(wr_to, wr_buf, wr_sequence);
    wr_sequence++;
    imc_sncpy(wr_buf, text, IMC_DATA_LENGTH);
    return;
  }

  strcat(wr_buf, text);
}

void imc_whoreply_end(void)
{
  imc_send_whoreply(wr_to, wr_buf, -(wr_sequence+1));
  imc_strfree(wr_to);
  wr_buf[0]=0;
  imc_shrinksbuf(wr_buf);
}

/* send a whois-request to a remote mud */
void imc_send_whois(const imc_char_data *from, const char *to)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (strchr(to, '@'))
    return;

  setdata(&out, from);

  sprintf(out.to, "%s@*", to);
  strcpy(out.type, "whois");

  imc_send(&out);
  imc_freedata(&out.data);
}

/* respond with a whois-reply */
void imc_send_whoisreply(const char *to, const char *data)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (!strcmp(imc_mudof(to), "*"))
    return; /* don't let them do this */

  imc_initdata(&out.data);

  imc_sncpy(out.to, to, IMC_NAME_LENGTH);
  strcpy(out.type, "whois-reply");
  strcpy(out.from, "*");
  imc_addkey(&out.data, "text", data);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* beep a remote player */
void imc_send_beep(const imc_char_data *from, const char *to)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  if (!strcmp(imc_mudof(to), "*"))
    return; /* don't let them do this */

  setdata(&out, from);
  strcpy(out.type, "beep");
  imc_sncpy(out.to, to, IMC_NAME_LENGTH);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* send a keepalive to everyone */
void imc_send_keepalive(void)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  imc_initdata(&out.data);
  strcpy(out.type, "is-alive");
  strcpy(out.from, "*");
  strcpy(out.to, "*@*");
  imc_addkey(&out.data, "versionid", IMC_VERSIONID);
  if (imc_siteinfo.flags[0])
    imc_addkey(&out.data, "flags", imc_siteinfo.flags);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* send a ping with a given timestamp */
void imc_send_ping(const char *to, int time_s, int time_u)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  imc_initdata(&out.data);
  strcpy(out.type, "ping");
  strcpy(out.from, "*");
  strcpy(out.to, "*@");
  imc_sncpy(out.to+2, to, IMC_MNAME_LENGTH-2);
  imc_addkeyi(&out.data, "time-s", time_s);
  imc_addkeyi(&out.data, "time-us", time_u);

  imc_send(&out);
  imc_freedata(&out.data);
}

/* send a pingreply with the given timestamp */
void imc_send_pingreply(const char *to, int time_s, int time_u, const char *path)
{
  imc_packet out;

  if (imc_active<IA_UP)
    return;

  imc_initdata(&out.data);
  strcpy(out.type, "ping-reply");
  strcpy(out.from, "*");
  strcpy(out.to, "*@");
  imc_sncpy(out.to+2, to, IMC_MNAME_LENGTH-2);
  imc_addkeyi(&out.data, "time-s", time_s);
  imc_addkeyi(&out.data, "time-us", time_u);
  imc_addkey(&out.data, "path", path);

  imc_send(&out);
  imc_freedata(&out.data);
}
