/***************************************************************************
 *  Original Diku Mud copyright (C) 1990, 1991 by Sebastian Hammer,        *
 *  Michael Seifert, Hans Henrik St{rfeldt, Tom Madsen, and Katja Nyboe.   *
 *                                                                         *
 *  Merc Diku vMud improvments copyright (C) 1992, 1993 by Michael          *
 *  Chastain, Michael Quan, and Mitchell Tse.                              *
 *                                                                         *
 *  In order to use any part of this Merc Diku Mud, you must comply with   *
 *  both the original Diku license in 'license.doc' as well the Merc       *
 *  license in 'license.txt'.  In particular, you may not remove either of *
 *  these copyright notices.                                               *
 *                                                                         *
 *  Thanks to abaddon for proof-reading our comm.c and pointing out bugs.  *
 *  Any remaining bugs are, of course, our work, not his.  :)              *
 *                                                                         *
 *  Much time and thought has gone into this software and you are          *
 *  benefitting.  We hope that you share your changes too.  What goes      *
 *  around, comes around.                                                  *
 ***************************************************************************/


/*
 * This file contains all of the OS-dependent stuff:
 *   startup, signals, BSD sockets for tcp/ip, i/o, timing.
 *
 * The data flow for input is:
 *    Game_loop ---> Read_from_descriptor ---> Read
 *    Game_loop ---> Read_from_buffer
 *
 * The data flow for output is:
 *    Game_loop ---> Process_Output ---> Write_to_descriptor -> Write
 *
 * The OS-dependent functions are Read_from_descriptor and Write_to_descriptor.
 * -- Furey  26 Jan 1993
 */

static char rcsid[] = "$Id: comm.c,v 1.311 2004/10/20 18:59:00 rusty Exp $";
#if defined(macintosh)
#include <types.h>
#else
#include <sys/types.h>
#include <sys/time.h>
#endif
#include "gc.h"
#include <unistd.h>
#include <signal.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
/*
#include <sys/ipc.h>
#include <sys/msg.h>
*/
#include <sys/time.h>
#include <sys/resource.h>
#include "merc.h"
#include "recycle.h"
#include "gladiator.h"
#include "tables.h"
/* #include "imc.h"
 * #include "imc-mercbase.h"
 * #include "icec.h"
 * #include "icec-mercbase.h"
 */
/* command procedures needed */
DECLARE_DO_FUN	( action_wraithform );
DECLARE_DO_FUN (action_zealot_convert);
DECLARE_DO_FUN   ( action_ambush );
DECLARE_DO_FUN(do_help          );
DECLARE_DO_FUN(do_look          );
DECLARE_DO_FUN(do_skills        );
DECLARE_DO_FUN(do_outfit        );
DECLARE_DO_FUN(do_count		);
DECLARE_DO_FUN(do_unread        );
DECLARE_DO_FUN(do_gossip);
DECLARE_DO_FUN(do_ooc);
DECLARE_DO_FUN(do_question);
DECLARE_DO_FUN(do_answer);
DECLARE_DO_FUN(do_clantalk);
DECLARE_DO_FUN(do_auction);
DECLARE_DO_FUN(do_music);
DECLARE_DO_FUN(do_quest);
DECLARE_DO_FUN(do_reply);
DECLARE_DO_FUN(do_tell);
DECLARE_DO_FUN(do_grats);
DECLARE_DO_FUN(do_gtell);

/* External Functions */
int	nonclan_lookup	args( (const char *name) );
#ifdef OLC_VERSION
void edit_help_file args ((CHAR_DATA *ch, char *buf));
#endif

/*
 * Malloc debugging stuff.
 */
#if defined(sun)
#undef MALLOC_DEBUG
#endif

#if defined(MALLOC_DEBUG)
#include <malloc.h>
extern  int     malloc_debug    args( ( int  ) );
extern  int     malloc_verify   args( ( void ) );
#endif
int clanner_count = 0;


/*
 * Signal handling.
 * Apollo has a problem with __attribute(atomic) in signal.h,
 *   I dance around it.
 */
#if defined(apollo)
#define __attribute(x)
#endif

#if defined(unix)
#include <signal.h>
#include <sys/ipc.h>
#endif

#if defined(apollo)
#undef __attribute
#endif



/*
 * Socket and TCP/IP stuff.
 */
#if     defined(macintosh) || defined(MSDOS)
const   char    echo_off_str    [] = { '\0' };
const   char    echo_on_str     [] = { '\0' };
const   char    go_ahead_str    [] = { '\0' };
#endif

#if     defined(unix)
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
const   char    echo_off_str    [] = { IAC, WILL, TELOPT_ECHO, '\0' };
const   char    echo_on_str     [] = { IAC, WONT, TELOPT_ECHO, '\0' };
const   char    go_ahead_str    [] = { IAC, GA, '\0' };
#endif



/*
 * OS-dependent declarations.
 */
#if     defined(_AIX)
#include <sys/select.h>
int     accept          args( ( int s, struct sockaddr *addr, int *addrlen ) );
int     bind            args( ( int s, struct sockaddr *name, int namelen ) );
void    bzero           args( ( char *b, int length ) );
int     getpeername     args( ( int s, struct sockaddr *name, int *namelen ) );
int     getsockname     args( ( int s, struct sockaddr *name, int *namelen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
int     listen          args( ( int s, int backlog ) );
int     setsockopt      args( ( int s, int level, int optname, void *optval,
          int optlen ) );
int     socket          args( ( int domain, int type, int protocol ) );
#endif

#if     defined(apollo)
#include <unistd.h>
void    bzero           args( ( char *b, int length ) );
#endif

#if     defined(__hpux)
int     accept          args( ( int s, void *addr, int *addrlen ) );
int     bind            args( ( int s, const void *addr, int addrlen ) );
void    bzero           args( ( char *b, int length ) );
int     getpeername     args( ( int s, void *addr, int *addrlen ) );
int     getsockname     args( ( int s, void *name, int *addrlen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
int     listen          args( ( int s, int backlog ) );
int     setsockopt      args( ( int s, int level, int optname,
        const void *optval, int optlen ) );
int     socket          args( ( int domain, int type, int protocol ) );
#endif

#if     defined(interactive)
#include <net/errno.h>
#include <sys/fnctl.h>
#endif

#if     defined(linuxXYZZY)
int     accept          args( ( int s, struct sockaddr *addr, int *addrlen ) );
int     bind            args( ( int s, struct sockaddr *name, int namelen ) );
int     close           args( ( int fd ) );
int     getpeername     args( ( int s, struct sockaddr *name, int *namelen ) );
int     getsockname     args( ( int s, struct sockaddr *name, int *namelen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
int     listen          args( ( int s, int backlog ) );
int     read            args( ( int fd, char *buf, int nbyte ) );
int     select          args( ( int width, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout ) );
int     socket          args( ( int domain, int type, int protocol ) );
int     write           args( ( int fd, char *buf, int nbyte ) );
#endif

#if     defined(macintosh)
#include <console.h>
#include <fcntl.h>
#include <unix.h>
struct  timeval
{
  time_t  tv_sec;
  time_t  tv_usec;
};
#if     !defined(isascii)
#define isascii(c)              ( (c) < 0200 )
#endif
static  long                    theKeys [4];

int     gettimeofday            args( ( struct timeval *tp, void *tzp ) );
#endif

#if     defined(MIPS_OS)
extern  int             errno;
#endif

#if     defined(MSDOS)
int     gettimeofday    args( ( struct timeval *tp, void *tzp ) );
int     kbhit           args( ( void ) );
#endif

#if     defined(NeXT)
int     close           args( ( int fd ) );
int     fcntl           args( ( int fd, int cmd, int arg ) );
#if     !defined(htons)
u_short htons           args( ( u_short hostshort ) );
#endif
#if     !defined(ntohl)
u_long  ntohl           args( ( u_long hostlong ) );
#endif
int     read            args( ( int fd, char *buf, int nbyte ) );
int     select          args( ( int width, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout ) );
int     write           args( ( int fd, char *buf, int nbyte ) );
#endif

#if     defined(sequent)
int     accept          args( ( int s, struct sockaddr *addr, int *addrlen ) );
int     bind            args( ( int s, struct sockaddr *name, int namelen ) );
int     close           args( ( int fd ) );
int     fcntl           args( ( int fd, int cmd, int arg ) );
int     getpeername     args( ( int s, struct sockaddr *name, int *namelen ) );
int     getsockname     args( ( int s, struct sockaddr *name, int *namelen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
#if     !defined(htons)
u_short htons           args( ( u_short hostshort ) );
#endif
int     listen          args( ( int s, int backlog ) );
#if     !defined(ntohl)
u_long  ntohl           args( ( u_long hostlong ) );
#endif
int     read            args( ( int fd, char *buf, int nbyte ) );
int     select          args( ( int width, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout ) );
int     setsockopt      args( ( int s, int level, int optname, caddr_t optval,
          int optlen ) );
int     socket          args( ( int domain, int type, int protocol ) );
int     write           args( ( int fd, char *buf, int nbyte ) );
#endif

/* This includes Solaris Sys V as well */
#if defined(sun)
int     accept          args( ( int s, struct sockaddr *addr, int *addrlen ) );
int     bind            args( ( int s, struct sockaddr *name, int namelen ) );
void    bzero           args( ( char *b, int length ) );
int     close           args( ( int fd ) );
int     getpeername     args( ( int s, struct sockaddr *name, int *namelen ) );
int     getsockname     args( ( int s, struct sockaddr *name, int *namelen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
int     listen          args( ( int s, int backlog ) );
int     read            args( ( int fd, char *buf, int nbyte ) );
int     select          args( ( int width, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout ) );
#if defined(SYSV)
int setsockopt          args( ( int s, int level, int optname,
          const char *optval, int optlen ) );
#else
int     setsockopt      args( ( int s, int level, int optname, void *optval,
          int optlen ) );
#endif
int     socket          args( ( int domain, int type, int protocol ) );
int     write           args( ( int fd, char *buf, int nbyte ) );
#endif

#if defined(ultrix)
int     accept          args( ( int s, struct sockaddr *addr, int *addrlen ) );
int     bind            args( ( int s, struct sockaddr *name, int namelen ) );
void    bzero           args( ( char *b, int length ) );
int     close           args( ( int fd ) );
int     getpeername     args( ( int s, struct sockaddr *name, int *namelen ) );
int     getsockname     args( ( int s, struct sockaddr *name, int *namelen ) );
int     gettimeofday    args( ( struct timeval *tp, struct timezone *tzp ) );
int     listen          args( ( int s, int backlog ) );
int     read            args( ( int fd, char *buf, int nbyte ) );
int     select          args( ( int width, fd_set *readfds, fd_set *writefds,
          fd_set *exceptfds, struct timeval *timeout ) );
int     setsockopt      args( ( int s, int level, int optname, void *optval,
          int optlen ) );
int     socket          args( ( int domain, int type, int protocol ) );
int     write           args( ( int fd, char *buf, int nbyte ) );
#endif



/*
 * Global variables.
 */
DESCRIPTOR_DATA *   descriptor_list;    /* All open descriptors         */
DESCRIPTOR_DATA *   d_next;             /* Next descriptor in loop      */
//FILE *              fpReserve;          /* Reserved file handle         */
bool                god;                /* All new chars are gods!      */
bool                merc_down;          /* Shutdown                     */
bool                wizlock;            /* Game is wizlocked            */
bool                newlock;            /* Game is newlocked            */
bool		    no_dns;
char                str_boot_time[MAX_INPUT_LENGTH];
time_t              current_time;       /* time of this pulse */        
bool		    telnet;		/* if we're binding to the telnet port*/
sh_int		    avarice_kills;
sh_int		    demise_kills;
sh_int		    honor_kills;
sh_int		    posse_kills;
sh_int		    warlock_kills;
sh_int		    zealot_kills;
sh_int              honor_demise_kills;
sh_int		    posse_killer_kills;
sh_int		    posse_thief_kills;
sh_int		    posse_thug_kills;
sh_int		    posse_ruffian_kills;

/*
 * OS-dependent local functions.
 */
#if defined(macintosh) || defined(MSDOS)
void    game_loop_mac_msdos     args( ( void ) );
bool    read_from_descriptor    args( ( DESCRIPTOR_DATA *d ) );
bool    write_to_descriptor     args( ( int desc, char *txt, int length ) );
#endif

#if defined(unix)
#define PERMS  0666
key_t msg_key;
int   msg_id;

void    game_loop_unix          args( ( int control[] ) );
int     init_socket             args( ( int port ) );
void    init_descriptor         args( ( int control ) );
bool    read_from_descriptor    args( ( DESCRIPTOR_DATA *d ) );
bool    write_to_descriptor     
	args( ( int desc, char *txt, int length, DESCRIPTOR_DATA *d ) );
#endif




/*
 * Other local functions (OS-independent).
 */
bool    check_parse_name        args( ( char *name ) );
bool    check_reconnect         args( ( DESCRIPTOR_DATA *d, char *name,
            bool fConn ) );
bool    check_playing           args( ( DESCRIPTOR_DATA *d, char *name ) );
int     main                    args( ( int argc, char **argv ) );
void    nanny                   args( ( DESCRIPTOR_DATA *d, char *argument ) );
void    show_stats		args( ( DESCRIPTOR_DATA *d ) );
int 	calc_stat_cost		args( ( CHAR_DATA *ch, int attr_type ) );
bool	can_use_points		args( ( CHAR_DATA *ch, int points ) );
bool    process_output          args( ( DESCRIPTOR_DATA *d, bool fPrompt ) );
void    read_from_buffer        args( ( DESCRIPTOR_DATA *d ) );
void    stop_idling             args( ( CHAR_DATA *ch ) );
void    bust_a_prompt           args( ( CHAR_DATA *ch ) );
bool    check_mob_name          args( ( char *name, bool old_char ) );
void creation_input(DESCRIPTOR_DATA *d, char *argument);
void creation_finalize(DESCRIPTOR_DATA *d, bool def);
void creation_message(DESCRIPTOR_DATA *d, bool forward);
int creation_step(DESCRIPTOR_DATA *d, bool forward, bool accept);
bool is_creation(DESCRIPTOR_DATA *d);

int main( int argc, char **argv )
{
    struct timeval now_time;
    int port;

#if defined(unix)
    int control[2] = {-1,-1};
#endif

    /*
     * Memory debugging if needed.
     */
#if defined(MALLOC_DEBUG)
    malloc_debug( 2 );
#endif

    /*
     * Init time.
     */
    gettimeofday( &now_time, NULL );
    current_time        = (time_t) now_time.tv_sec;
    strcpy( str_boot_time, ctime( &current_time ) );

    /*
     * Macintosh console initialization.
     */
#if defined(macintosh)
    console_options.nrows = 31;
    cshow( stdout );
    csetmode( C_RAW, stdin );
    cecho2file( "log file", 1, stderr );
#endif

    /* Set our eUID to that of the user 'mud' */
    seteuid(MUD_UID);

    /*
     * Reserve one channel for our use.
     */
/*    if ( ( fpReserve = fopen( NULL_FILE, "r" ) ) == NULL )
    {
  perror( NULL_FILE );
  exit( 1 );
    } */

    /*
     * Get the port number.
     */
    port = 4000;
    if ( argc > 1 )
    {
  if ( !is_number( argv[1] ) )
  {
      fprintf( stderr, "Usage: %s [port #]\n", argv[0] );
      exit( 1 );
  }

  port = atoi( argv[1] );
  /*
  else if ( ( port = atoi( argv[1] ) ) <= 1024 )
  {
      fprintf( stderr, "Port number must be above 1024.\n" );
      exit( 1 );
  }
   */
    }

    /*
     * Run the game.
     */
#if defined(macintosh) || defined(MSDOS)
    boot_db( );
    log_string( "Merc is ready to rock." );
    game_loop_mac_msdos( );
#endif

#if defined(unix)
    telnet=FALSE;
    control[0] = init_socket( port );
    if( seteuid(0) == 0 )
    {
      telnet = TRUE;
      control[1] = init_socket( 23 );
    }
    if( telnet ) seteuid(MUD_UID);
    boot_db( );
/****
#ifdef IMC_GAME_VERSION
    imc_startup("/home/chris/moosehead/imc");
    #imc_startup("/mud/moosehead/imc/");  
    icec_init();          
#endif
 ****/
    sprintf( log_buf, "MHS is ready on port %d.", port );
    log_string( log_buf );
#ifdef GAME_VERSION
    /* renice to 4 because we run on a shared machine 
    setpriority(PRIO_PROCESS,0,4);
     */
#else
    /* OLC version needs even less CPU time allocated to it */
    setpriority(PRIO_PROCESS,0,9);
#endif
    game_loop_unix( control );
/****
#ifdef IMC_GAME_VERSION
    imc_shutdown();
#endif
 ****/
    close (control[0]);
    if( telnet )
      close (control[1]);
#endif

    /*
     * That's all, folks.
     */
    log_string( "Normal termination of game." );
    exit( 0 );
    return 0;
}



#if defined(unix)
int init_socket( int port )
{
    static struct sockaddr_in sa_zero;
    struct sockaddr_in sa;
    int x = 1;
    int fd;

    if ( ( fd = socket( AF_INET, SOCK_STREAM, 0 ) ) < 0 )
    {
  perror( "Init_socket: socket" );
  exit( 1 );
    }

    if ( setsockopt( fd, SOL_SOCKET, SO_REUSEADDR,
    (char *) &x, sizeof(x) ) < 0 )
    {
  perror( "Init_socket: SO_REUSEADDR" );
  shutdown( fd, 2);
  close(fd);
  exit( 1 );
    }

/*if defined(SO_DONTLINGER) && !defined(SYSV)*/
    {
  struct  linger  ld;


  ld.l_onoff  = 1;
  ld.l_linger = 0;

  if ( setsockopt( fd, SOL_SOCKET, SO_LINGER,
  (char *) &ld, sizeof(ld) ) < 0 )
  {
      perror( "Init_socket: SO_LINGER" );
      shutdown( fd, 2);
      close(fd);
      exit( 1 );
  }
    }
/*endif*/

    sa              = sa_zero;
    sa.sin_family   = AF_INET;
    sa.sin_port     = htons( port );

    if ( bind( fd, (struct sockaddr *) &sa, sizeof(sa) ) < 0 )
    {
  perror("Init socket: bind" );
  shutdown( fd, 2 );
  close(fd);
  exit(1);
    }


    if ( listen( fd, 3 ) < 0 )
    {
  perror("Init socket: listen");
  shutdown( fd, 2 );
  close(fd);
  exit(1);
    }

    return fd;
}
#endif



#if defined(macintosh) || defined(MSDOS)
void game_loop_mac_msdos( void )
{
    struct timeval last_time;
    struct timeval now_time;
    static DESCRIPTOR_DATA dcon;

    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /*
     * New_descriptor analogue.
     */
    dcon.descriptor     = 0;
    dcon.connected      = CON_GET_NAME;
    dcon.host           = str_dup( "localhost" );
    dcon.outsize        = 2000;
#ifdef OLC_VERSION
    dcon.outbuf         = alloc_mem( dcon.outsize );
#else  /*game version*/
    dcon.outbuf         = GC_MALLOC( dcon.outsize );
#endif
    dcon.next           = descriptor_list;
    dcon.showstr_head   = NULL;
    dcon.showstr_point  = NULL;
    descriptor_list     = &dcon;

    /*
     * Send the greeting.
     */
    {
  extern char * help_greeting;
  if ( help_greeting[0] == '.' )
      write_to_buffer( &dcon, help_greeting+1, 0 );
  else
      write_to_buffer( &dcon, help_greeting  , 0 );
    }

    /* Main loop */
    while ( !merc_down )
    {
  DESCRIPTOR_DATA *d;

  /*
   * Process input.
   */
  for ( d = descriptor_list; d != NULL; d = d_next )
  {
      d_next      = d->next;
      d->fcommand = FALSE;

#if defined(MSDOS)
      if ( kbhit( ) )
#endif
      {
    if ( d->character != NULL )
        d->character->timer = 0;
    if ( !read_from_descriptor( d ) )
    {
        if ( d->character != NULL)
      save_char_obj( d->character );
        d->outtop   = 0;
        close_socket( d );
        continue;
    }
      }

      if (d->character != NULL && d->character->daze > 0)
    --d->character->daze;

      if ( d->character != NULL && d->character->wait > 0 )
      {
    --d->character->wait;
    continue;
      }

      read_from_buffer( d );

      if ( d->incomm[0] != '\0' )
      {
    d->fcommand     = TRUE;
    stop_idling( d->character );
    
    if (d->showstr_point) {
        show_string(d,d->incomm);
    } else  
    if ( d->connected == CON_PLAYING ) {
       if (!check_macro (d->character, d->incomm)) {
        if (d->character->pcdata && d->character->pcdata->macro_count > 0) {
          /* inside a macro, so show command */
          send_to_char (d->incomm,d->character); 
          send_to_char ("\n\r",d->character);
        }          
        substitute_alias( d, d->incomm );
       }
    } else {
      if ( d->connected == CON_EDITOR )
        interpret (d->character, d->incomm);
      else
        nanny( d, d->incomm );
    }

    d->incomm[0]    = '\0';
      }
  }



  /*
   * Autonomous game motion.
   */
  update_handler( );


  /*
   * Output.
   */
  for ( d = descriptor_list; d != NULL; d = d_next )
  {
      d_next = d->next;

      if ( ( d->fcommand || d->outtop > 0 ) )
      {
    if ( !process_output( d, TRUE ) )
    {
        if ( d->character != NULL && d->character->level > 1)
      save_char_obj( d->character );
        d->outtop   = 0;
        close_socket( d );
    }
      }
  }



  /*
   * Synchronize to a clock.
   * Busy wait (blargh).
   */
  now_time = last_time;
  for ( ; ; )
  {
      int delta;

#if defined(MSDOS)
      if ( kbhit( ) )
#endif
      {
    if ( dcon.character != NULL )
        dcon.character->timer = 0;
    if ( !read_from_descriptor( &dcon ) )
    {
        if ( dcon.character != NULL && d->character->level > 1)
      save_char_obj( d->character );
        dcon.outtop = 0;
        close_socket( &dcon );
    }
#if defined(MSDOS)
    break;
#endif
      }

      gettimeofday( &now_time, NULL );
      delta = ( now_time.tv_sec  - last_time.tv_sec  ) * 1000 * 1000
      + ( now_time.tv_usec - last_time.tv_usec );
      if ( delta >= 1000000 / PULSE_PER_SECOND )
    break;
  }
  last_time    = now_time;
  current_time = (time_t) last_time.tv_sec;
    }

    return;
}
#endif



#if defined(unix)

struct my_mesg_buf {
  long mtype;
  char text[1024];
};
/*
void get_ident_info ( CHAR_DATA *ch )
{   
  struct my_mesg_buf buf;  
  int len;
  
  if (!ch->desc || !ch->desc->host) return;
  
  if ( msg_id >= 0 ) {
    sprintf (buf.text,"%s %d %s %d 4000",ch->name,ch->id,ch->desc->host, ch->desc->port );
    buf.mtype = 1;    
    len = strlen (buf.text)+1;
    msgsnd (msg_id, (struct msgbuf *)&buf, len, IPC_NOWAIT );
  }
}

void check_ident_info ( )
{   
  struct my_mesg_buf buf;  
  int id, len;
  char name[1024];
  DESCRIPTOR_DATA *d;
  
  if ( msg_id >= 0 ) {
    len = msgrcv (msg_id, (struct msgbuf *)&buf, 1024, 2, IPC_NOWAIT);
    if ( len > 0 ) {
      sscanf (buf.text,"%d %s",&id,&name[0]);
      if (id > 0)
      for ( d = descriptor_list; d; d = d->next ) {
        if (d->character) {
          if (d->character->id == id) {
            d->name = str_dup ( name );
            sprintf( log_buf, "Ident: %s  %s@%s.", d->character->name, 
              d->name, d->host );
            log_string( log_buf );
            wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(d->character));
          }
        }
      }
    }    
  }
}
*/

  /* for a while we're not going to save characters on a crash
   * This should avoid the CPU looping problems.
   *
   Put this back in to save pfiles on our crashes - Poquah
void sig_crash ()
{
  CHAR_DATA *ch, *ch_next;
  
  signal ( SIGSEGV, SIG_DFL );
  signal ( SIGBUS,  SIG_DFL );
  signal ( SIGHUP,  SIG_DFL );  
  signal ( SIGFPE,  SIG_DFL );
  signal ( SIGILL,  SIG_DFL );
  signal ( SIGIOT,  SIG_DFL );
  signal ( SIGKILL,  SIG_DFL );
  signal ( SIGTERM,  SIG_DFL );
           
  for ( ch = char_list; ch != NULL; ch = ch_next ) {
    ch_next = ch->next;
    if (!IS_NPC (ch) ) {
      save_char_obj( ch );
    }
  }
  
  abort ();
}

   */

void dummy()
{
	char log_buf[MAX_STRING_LENGTH];
	sprintf( log_buf, "Alarm set off");
	log_string( log_buf );
	dns_site(NULL,dns_buf);
	return;
}

void game_loop_unix( int control[] )
{
    static struct timeval null_time;
    struct timeval last_time;
    /* int child_pid; */
    
    /*signal ( SIGCLD, SIG_IGN );
    signal ( SIGSEGV, sig_crash );
    signal ( SIGBUS,  sig_crash );
    signal ( SIGHUP,  sig_crash );    
    signal ( SIGFPE,  sig_crash );
    signal ( SIGILL,  sig_crash );
    signal ( SIGIOT,  sig_crash );
    signal ( SIGKILL,  sig_crash );
    signal ( SIGTERM,  sig_crash );
    */
    /* Put above back in to save pfiles for crashes? - Poquah
     */
    signal ( SIGALRM, dummy );
    
	/*
    msg_key = ftok("/mud/moosehead/src/ident.base", 1);
    msg_key = 666666;
    if ( (msg_id = msgget (msg_key, PERMS | IPC_CREAT) ) < 0 )
      bug ("Could not create message queue.",0);
	*/

    signal( SIGPIPE, SIG_IGN );
    gettimeofday( &last_time, NULL );
    current_time = (time_t) last_time.tv_sec;

    /* Main loop */
    while ( !merc_down )
    {
  fd_set in_set;
  fd_set out_set;
  fd_set exc_set;
  DESCRIPTOR_DATA *d;
  int maxdesc=-1;

#if defined(MALLOC_DEBUG)
  if ( malloc_verify( ) != 1 )
      abort( );
#endif

  /*
   * Poll all active descriptors.
   */
  FD_ZERO( &in_set  );
  FD_ZERO( &out_set );
  FD_ZERO( &exc_set );
  FD_SET( control[0], &in_set );
  if(control[0] > maxdesc)
    maxdesc = control[0];
  if( telnet )
  {
  FD_SET( control[1], &in_set );
  if(control[1] > maxdesc)
    maxdesc = control[1];
  }
  for ( d = descriptor_list; d; d = d->next )
  {
      maxdesc = UMAX( maxdesc, d->descriptor );
      FD_SET( d->descriptor, &in_set  );
      FD_SET( d->descriptor, &out_set );
      FD_SET( d->descriptor, &exc_set );
  }

      /* IMC 
       maxdesc=imc_fill_fdsets(maxdesc, &in_set, &out_set, &exc_set);
       */

  if ( select( maxdesc+1, &in_set, &out_set, &exc_set, &null_time ) < 0 )
  {
      perror( "Game_loop: select: poll" );
      exit( 1 );
  }
      /*
       imc_idle_select(&in_set, &out_set, &exc_set, current_time);
       */

  /*
   * New connection?
   */
  if ( FD_ISSET( control[0], &in_set ) )
      init_descriptor( control[0] );
  if( telnet )
  {
  if ( FD_ISSET( control[1], &in_set ) )
      init_descriptor( control[1] );
  }

  /*
   * Kick out the freaky folks.
   */
  for ( d = descriptor_list; d != NULL; d = d_next )
  {
      d_next = d->next;   
      if ( FD_ISSET( d->descriptor, &exc_set ) )
      {
    FD_CLR( d->descriptor, &in_set  );
    FD_CLR( d->descriptor, &out_set );
    if ( d->character && d->character->level > 1)
        save_char_obj( d->character );
    d->outtop       = 0;
    close_socket( d );
      }
  }

  /*
   * Process input.
   */

  for ( d = descriptor_list; d != NULL; d = d_next )
  {
      d_next      = d->next;
      d->fcommand = FALSE;

      if ( FD_ISSET( d->descriptor, &in_set ) )
      {
    if ( d->character != NULL )
        d->character->timer = 0;
    if ( !read_from_descriptor( d ) )
    {
        FD_CLR( d->descriptor, &out_set );
        if ( d->character != NULL && d->character->level > 1)
      save_char_obj( d->character );
        d->outtop   = 0;
        close_socket( d );
        continue;
    }
      }

      /* Now some commands can act through daze */
      read_from_buffer( d );
	/* Here we handle pulse-delayed actions like wraith form */
	if(d->character != NULL)
	{
    if (d->character->pcdata != NULL)
    {
      if(d->character->pcdata->wraith_timer > 0 )
      {
  	    if ( --d->character->pcdata->wraith_timer <= 0 )
  		action_wraithform(d->character,"");
  	    else
  	    if ( d->character->pcdata->wraith_timer % 12 == 0 )
  		act("Your form shimmers and fades...",d->character,NULL,NULL,TO_CHAR,FALSE);
      }
    }
    tick_pulse_command(d->character);
    
      if (d->character->daze > 0)
    --d->character->daze;

      if (d->character->wait > 0 )
      {
    --d->character->wait;
    continue;
      }
    }

      if ( d->incomm[0] != '\0' )
      {
    d->fcommand     = TRUE;
    stop_idling( d->character );
    
    if (d->showstr_point) {
        show_string(d,d->incomm);
    } else  
    if ( d->connected == CON_PLAYING ) {
       if (!check_macro (d->character, d->incomm)) {
        if (d->character->pcdata && d->character->pcdata->macro_count > 0) {
          /* inside a macro, so show command */
          send_to_char (d->incomm,d->character); 
          send_to_char ("\n\r",d->character);
        }          
        substitute_alias( d, d->incomm );
       }
    } else {
      if ( d->connected == CON_EDITOR )
        interpret (d->character, d->incomm);
      else
        nanny( d, d->incomm );
    }

    d->incomm[0]    = '\0';
      }
  }



  /*
   * Autonomous game motion.
   */
  update_handler( );

  for ( d = descriptor_list ; d ; d = d->next )
  {
      if ( d->character != NULL &&
	   HAS_KIT(d->character,"nethermancer") )
	   fade( d->character, "" );
  }

/*
  check_ident_info( );
*/

  /*
   * Output.
   */

  for ( d = descriptor_list; d != NULL; d = d_next )
  {
      d_next = d->next;

      if ( ( (d->fcommand) || (d->outtop > 0) )
      &&   (FD_ISSET(d->descriptor, &out_set)) )
      {
    if ( !process_output( d, TRUE ) )
    {
        if ( d->character != NULL && d->character->level > 1)
      save_char_obj( d->character );
        d->outtop   = 0;
        close_socket( d );
    }
      }
  }



  /*
   * Synchronize to a clock.
   * Sleep( last_time + 1/PULSE_PER_SECOND - now ).
   * Careful here of signed versus unsigned arithmetic.
   */
  {
      struct timeval now_time;
      long secDelta;
      long usecDelta;

      gettimeofday( &now_time, NULL );
      usecDelta   = ((int) last_time.tv_usec) - ((int) now_time.tv_usec)
      + 1000000 / PULSE_PER_SECOND;
      secDelta    = ((int) last_time.tv_sec ) - ((int) now_time.tv_sec );
      while ( usecDelta < 0 )
      {
    usecDelta += 1000000;
    secDelta  -= 1;
      }

      while ( usecDelta >= 1000000 )
      {
    usecDelta -= 1000000;
    secDelta  += 1;
      }

      if ( secDelta > 0 || ( secDelta == 0 && usecDelta > 0 ) )
      {
    struct timeval stall_time;

    stall_time.tv_usec = usecDelta;
    stall_time.tv_sec  = secDelta;
    if ( select( 0, NULL, NULL, NULL, &stall_time ) < 0 )
    {
        perror( "Game_loop: select: stall" );
        exit( 1 );
    }
      }
  }

  gettimeofday( &last_time, NULL );
  current_time = (time_t) last_time.tv_sec;
    }
    
    signal ( SIGSEGV, SIG_DFL );
    signal ( SIGBUS,  SIG_DFL );
    signal ( SIGHUP,  SIG_DFL );    
    signal ( SIGFPE,  SIG_DFL );
    signal ( SIGILL,  SIG_DFL );
    signal ( SIGIOT,  SIG_DFL );
    signal ( SIGKILL,  SIG_DFL );
    signal ( SIGTERM,  SIG_DFL );    

    return;
}
#endif

void count_clanners(void)
{
  clanner_count = 0;
  DESCRIPTOR_DATA *d;
  for ( d = descriptor_list; d != NULL; d = d->next )
    if ( d->connected == CON_PLAYING && !IS_IMMORTAL(d->character) )
  {
    if(is_clan(d->character))
      clanner_count++;
  }

}

#if defined(unix)
void init_descriptor( int control )
{
    char buf[MAX_STRING_LENGTH];
    DESCRIPTOR_DATA *dnew;
    struct sockaddr_in sock;
    struct hostent *from = NULL;
    int desc;
    int size;

    size = sizeof(sock);
    getsockname( control, (struct sockaddr *) &sock, &size );
    if ( ( desc = accept( control, (struct sockaddr *) &sock, &size) ) < 0 )
    {
  perror( "New_descriptor: accept" );
  return;
    }

#if !defined(FNDELAY)
#define FNDELAY O_NDELAY
#endif

    if ( fcntl( desc, F_SETFL, FNDELAY ) == -1 )
    {
  perror( "New_descriptor: fcntl: FNDELAY" );
  return;
    }

    /*
     * Cons a new descriptor.
     */
    dnew = new_descriptor();

    dnew->descriptor    = desc;
    dnew->connected     = CON_GET_NAME;
    dnew->showstr_head  = NULL;
    dnew->showstr_point = NULL;
    dnew->outsize       = 2048;
#ifdef OLC_VERSION
    dnew->outbuf        = alloc_mem( dnew->outsize );
#else  /*game version*/
    dnew->outbuf        = GC_MALLOC( dnew->outsize );
#endif

    size = sizeof(sock);
    if ( getpeername( desc, (struct sockaddr *) &sock, &size ) < 0 )
    {
  perror( "New_descriptor: getpeername" );
  dnew->host = str_dup( "(unknown)" );
    }
    else
    {
  /*
   * Would be nice to use inet_ntoa here but it takes a struct arg,
   * which ain't very compatible between gcc and system libraries.
   */
  int addr;

  addr = ntohl( sock.sin_addr.s_addr );
  sprintf( buf, "%d.%d.%d.%d",
      ( addr >> 24 ) & 0xFF, ( addr >> 16 ) & 0xFF,
      ( addr >>  8 ) & 0xFF, ( addr       ) & 0xFF
      );
log_string("checkpoint a");
  sprintf( log_buf, "Sock.sinaddr:  %s", buf );
log_string("checkpoint b");
  log_string( log_buf );

log_string("checkpoint c");
  if ( 0) { // !no_dns ) {
  alarm(30);
  strcpy(dns_buf,buf);
	log_string("GOOBER 2");
  if(!check_dns(buf))
{
	log_string("GOOBER 1 ");
  from = gethostbyaddr( (char *) &sock.sin_addr,
      sizeof(sock.sin_addr), AF_INET );
	log_string("GOOBER 2");
}
  alarm(0);
  strcpy(dns_buf,"");
  }

      dnew->port = ntohs (sock.sin_port);
      dnew->host = str_dup( from ? from->h_name : buf );
    }
  
    /*
     * Swiftest: I added the following to ban sites.  I don't
     * endorse banning of sites, but Copper has few descriptors now
     * and some people from certain sites keep abusing access by
     * using automated 'autodialers' and leaving connections hanging.
     *
     * Furey: added suffix check by request of Nickel of HiddenWorlds.
     */
    if ( check_ban(dnew->host,BAN_ALL))
    {
      sprintf( log_buf, "Ban denied %s.", dnew->host );
      log_string( log_buf );
  write_to_descriptor( desc,
      "Your site has been banned from this Sled.\n\r", 0, dnew );
 /* write_to_descriptor( desc,
      "Email rusty@mraz.org for more information.\n\r", 0, dnew );*/
  write_to_descriptor( desc,
      "Bans are not given lightly.  You probobly earned it.  Go away.\n\r", 0, dnew );
  write_to_descriptor( desc, 
      "If you are connecting from mudconnector.com, please use a different IP to connect to our SLED from.  Thank you The MHS Admin Team.\n\r", 0, dnew);
#if defined (GAME_VERSION)
  shutdown( desc, 2 );
  close( desc );
  free_descriptor(dnew);
  return;
#endif
    }
    /*
     * Init descriptor data.
     */
    dnew->next                  = descriptor_list;
    descriptor_list             = dnew;

    /*
     * Send the greeting.
     */
    {
  extern char * help_greeting;
  if ( help_greeting[0] == '.' )
      write_to_buffer( dnew, help_greeting+1, 0 );
  else
      write_to_buffer( dnew, help_greeting  , 0 );
    }

    return;
}
#endif



void close_socket( DESCRIPTOR_DATA *dclose )
{
    CHAR_DATA *ch;

    if ( dclose->outtop > 0 )
  process_output( dclose, FALSE );

    if ( dclose->snoop_by != NULL )
    {
  write_to_buffer( dclose->snoop_by,
      "Your victim has left the game.\n\r", 0 );
    }

    {
  DESCRIPTOR_DATA *d;

  for ( d = descriptor_list; d != NULL; d = d->next )
  {
      if ( d->snoop_by == dclose )
    d->snoop_by = NULL;
  }
    }

    if ( ( ch = dclose->character ) != NULL )
    {
  sprintf( log_buf, "Closing link to %s.", ch->name );
  log_string( log_buf );
  if ( dclose->connected == CON_PLAYING && !merc_down)
  {
      act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM ,FALSE);
      wiznet("Net death has claimed $N.",ch,NULL,WIZ_LINKS,0,0);
//      if(!IS_IMMORTAL(ch))
 //     {
 //       pnet("Net death has claimed $N.",ch,NULL,PNET_LINKS,0,0);
 //     }
      ch->desc = NULL;
  }
#ifdef OLC_VERSION
  else if(dclose->connected == CON_EDITOR && !merc_down)
  {// If in the middle of editing a help file we don't want it just killing it
    if(ch->pcdata && ch->pcdata->line_edit != NULL)//== &edit_help_file)
    {// Store your current work
      insert_line_callback (ch, "/done");
      act( "$n has lost $s link.", ch, NULL, NULL, TO_ROOM ,FALSE);
      wiznet("Net death has claimed $N.",ch,NULL,WIZ_LINKS,0,0);
      ch->desc = NULL;
      ch->timer = 0; /* Prevent instant-voids by losing link after idling a while */
    }
    else// Handling for any other text editing
      free_char( dclose->original ? dclose->original : dclose->character );
  }
#endif
  else
  {
      free_char( dclose->original ? dclose->original : dclose->character );
  }
    }

    if ( d_next == dclose )
  d_next = d_next->next;   

    if ( dclose == descriptor_list )
    {
  descriptor_list = descriptor_list->next;
    }
    else
    {
  DESCRIPTOR_DATA *d;

  for ( d = descriptor_list; d && d->next != dclose; d = d->next )
      ;
  if ( d != NULL )
      d->next = dclose->next;
  else
      bug( "Close_socket: dclose not found.", 0 );
    }
    shutdown( dclose->descriptor, 2 );
    close( dclose->descriptor );
    free_descriptor(dclose);
#if defined(MSDOS) || defined(macintosh)
    exit(1);
#endif
    return;
}



bool read_from_descriptor( DESCRIPTOR_DATA *d )
{
    int iStart;
    d->input_received = TRUE;
    /* Hold horses if pending command already. */
//    if ( d->incomm[0] != '\0' )
//  return TRUE;

    /* Check for overflow. */
    iStart = strlen(d->inbuf);
    if ( iStart >= sizeof(d->inbuf) - 10 )
    {
  sprintf( log_buf, "%s input overflow!", d->host );
  log_string( log_buf );
  write_to_descriptor( d->descriptor,
      "\n\r*** PUT A LID ON IT!!! ***\n\r", 0, d );
  return FALSE;
    }

    /* Snarf input. */
#if defined(macintosh)
    for ( ; ; )
    {
  int c;
  c = getc( stdin );
  if ( c == '\0' || c == EOF )
      break;
  putc( c, stdout );
  if ( c == '\r' )
      putc( '\n', stdout );
  d->inbuf[iStart++] = c;
  if ( iStart > sizeof(d->inbuf) - 10 )
      break;
    }
#endif

#if defined(MSDOS) || defined(unix)
    for ( ; ; )
    {
  int nRead;

  nRead = read( d->descriptor, d->inbuf + iStart,
      sizeof(d->inbuf) - 10 - iStart );
  if ( nRead > 0 )
  {
      iStart += nRead;
      if ( d->inbuf[iStart-1] == '\n' || d->inbuf[iStart-1] == '\r' )
    break;
  }
  else if ( nRead == 0 )
  {
      log_string( "EOF encountered on read." );
      return FALSE;
  }
  else if ( errno == EWOULDBLOCK )
      break;
  else
  {
      sprintf(log_buf,"RFD: connection timed out on %s::%d",d->host,errno);
      wiznet(log_buf,NULL,NULL,WIZ_DEBUG,0,0);
      perror( "Read_from_descriptor" );
      return FALSE;
  }
    }
#endif

    d->inbuf[iStart] = '\0';
    return TRUE;
}

/* Function for my need - a specialized str_prefix */
bool check_word_match(char *base, char *word, char *word2, int needed)
{
  int i = 0;
  while(base[i] != '\0' && (base[i] == word[i] || base[i] == word2[i]))
  {
    i++;
  }
  if(i >= needed && (base[i] == word[i] || base[i] == word2[i] || base[i] == ' ' || base[i] == '\n' || base[i] == '\r'))
  {
    return TRUE;
  }
  // base == '\0' is a failure, they haven't hit enter yet
  return FALSE;
}

/* Move out the current line, leave the rest alone - drop the first word */
bool move_line_from_buffer(char *base, char *transfer)
{
  int i = 0, j = 0;
  /* Eat the first word */
  while(base[i] == ' ')
    i++;// Go past any spaces
  if(base[i] != ':' && base[i] != ';' && base[i] != ']' && base[i] != '.' && base[i] != '?')
  {
    while(base[i] != ' ' && base[i] != '\n' && base[i] != '\r' && base[i] != '\0')
      i++;// Eat the word itself
  }
  else
    i++;
  while(base[i] == ' ')
    i++;// Past any more spaces
  /* Now start copying */
  while(base[i] != '\n' && base[i] != '\r' && base[i] != '\0')
  {
    transfer[j] = base[i];
    i++;
    j++;
  }
  transfer[j] = '\0';// Close off transfer
  while(base[i] == '\n' || base[i] == '\r')
    i++;// Past the CR
  while(base[i] != '\0')
  {//Overwrite this line
    *base = base[i];
    base++;
  }
  *base = '\0';
  return i;// How many were yanked
}

/* Transfer one ooc command out of buffer to input line */
bool read_ooc_from_buffer( DESCRIPTOR_DATA *d )
{
  int i, j, k;
//  bool newline = TRUE;
  bool check_tilde = TRUE;
  char buf[MAX_INPUT_LENGTH];

  if(!d->input_received)
	return FALSE;


  // Replaced NULL with '\0', if there's a problem with input ending that may be it
  while(check_tilde)
  {
   check_tilde = FALSE;
   for ( i = 0; d->inbuf[i] != '\0'; i++ ) {
    if (d->inbuf[i] == '~' ) {
/*        d->inbuf[0] = '\r';
      for (t = 1; d->inbuf[t] != NULL; t++)
        d->inbuf[t] = d->inbuf[t+i];  
      d->inbuf[1] = NULL;
      d->incomm[0] = NULL;*/
      /* Change of plan, now we only eat up to the ~ */
      check_tilde = TRUE;// Need to circle again, otherwise a second tilde could sneak in from one input blast
      j = 0;
      i++;
      for(; d->inbuf[i] != '\0'; i++)
      {
        d->inbuf[j] = d->inbuf[i];// Automatically ends itself
        j++;
      }
      d->inbuf[j] = '\0';
      d->incomm[0] = '\n';// Clear the input command as well - To restore input command, yank this
      d->incomm[1] = '\0';
      /* Still kill the macro */
      if (d->character && d->character->pcdata) {
        d->character->pcdata->macro_count = 0;
        clear_macro_marks (d->character);          
      }
      if(d->inbuf[0] == '\0')
        return FALSE;// Don't return after unless it's done, more responsivei
      break;// Done with the for this round
    }      
   }
  }
/* Input needs cleanup, not ready for live yet */
  if(1 || !d->character || !d->character->in_room)
    return FALSE;// Nothing to be done, no need to check

  /* Current commands parsed for: (Unfortunately very hard coded)
   * answer
   * clan
   * cgossip                              
   * gtell / ;
   * grats   
   * gossip / .
   * music
   * ooc / ]
   * quest
   * question / ?
   * reply
   * tell 
   * immtalk :*/

  i = 0;
  while(d->inbuf[i] != '\0')
  {
    j = i;
    while(d->inbuf[j] == ' ')
      j++;
    k = j;
    while(d->inbuf[k] != '\n' && d->inbuf[k] != '\r' && d->inbuf[k] != '\0')
      k++;
    if(d->inbuf[k] == '\0')
      return FALSE;// Nothing to do here
    switch(d->inbuf[j])
    {
      case ':': if(IS_IMMORTAL(d->character))
		{
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_immtalk(d->character, buf);
                  return TRUE;
		}
                break;
      case ';': move_line_from_buffer(d->inbuf + i, buf);
                do_gtell(d->character, buf);
                return TRUE;
      case '.': move_line_from_buffer(d->inbuf + i, buf);
                do_gossip(d->character, buf);
                return TRUE;
      case ']': move_line_from_buffer(d->inbuf + i, buf);
                do_ooc(d->character, buf);
                return TRUE;
      case '?': move_line_from_buffer(d->inbuf + i, buf);
                do_question(d->character, buf);
                return TRUE;
      case 'a':
      case 'A': if(check_word_match(d->inbuf + j + 1, "nswer", "NSWER", 1))
                {//answer
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_answer(d->character, buf);
                  return TRUE;
                }
                break;
      case 'c':
      case 'C':  if(check_word_match(d->inbuf + j + 1, "lan", "LAN", 1))
                {//clan
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_clantalk(d->character, buf);
                  return TRUE;
                }
                if(check_word_match(d->inbuf + j + 1, "gossip", "GOSSIP", 1))
                {//cgossip
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_auction(d->character, buf);
                  return TRUE;
                }
                break;
      case 'g':
      case 'G':  if(check_word_match(d->inbuf + j + 1, "rats", "RATS", 2))
                {//grats
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_grats(d->character, buf);
                  return TRUE;
                }
                if(check_word_match(d->inbuf + j + 1, "tell", "TELL", 0))
                {//gtell
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_gtell(d->character, buf);
                  return TRUE;
                }
                if(check_word_match(d->inbuf + j + 1, "ossip", "OSSIP", 2))
                {//gossip
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_gossip(d->character, buf);
                  return TRUE;
                }
                break;
      case 'i':
      case 'I': if(IS_IMMORTAL(d->character))
		{
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_immtalk(d->character, buf);
                  return TRUE;
                }
                break;
      case 'm':
      case 'M': if(check_word_match(d->inbuf + j + 1, "usic", "USIC", 1))
                {//music
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_music(d->character, buf);
                  return TRUE;
                }
                break;
      case 'o':
      case 'O': if(check_word_match(d->inbuf + j + 1, "oc", "OC", 1))
                {//ooc
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_ooc(d->character, buf);
                  return TRUE;
                }
                break;
      case 'q':
      case 'Q':  if(check_word_match(d->inbuf + j + 1, "uest", "UEST", 0))
                {//quest
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_quest(d->character, buf);
                  return TRUE;
                }
                if(check_word_match(d->inbuf + j + 1, "uestion", "UESTION", 5))
                {//question
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_question(d->character, buf);
                  return TRUE;
                }
                break;
      case 'r':
      case 'R': if(check_word_match(d->inbuf + j + 1, "eply", "EPLY", 3))
                {//reply
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_reply(d->character, buf);
                  return TRUE;
                }
                break;
      case 't':
      case 'T': if(check_word_match(d->inbuf + j + 1, "ell", "ELL", 3))
                {//tell
                  move_line_from_buffer(d->inbuf + i, buf);
                  do_tell(d->character, buf);
                  return TRUE;
                }
                break;
    }
    i = k;// Advance to the newline
    while(d->inbuf[i] == '\n' || d->inbuf[i] == '\r')
      i++;
  }
  return FALSE;// No new command found
}

/*
 * Transfer one line from input buffer to input line.
 */
void read_from_buffer( DESCRIPTOR_DATA *d )
{
    int i, j, k, t;

    /*
     * Hold horses if pending command already.
     */
    if((d->input_received = read_ooc_from_buffer(d)) == TRUE)
	return;

    if ( d->incomm[0] != '\0' )
  return;
  

    /*
     * Look for at least one new line.
     */
    for ( i = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
      if (d->inbuf[i] == 1) {                  /* signals end of macro */        
        for (t = i; d->inbuf[t] != '\0'; t++)  /* shift over inbuff */
          d->inbuf[t] = d->inbuf[t+1];
        if (d->character && d->character->pcdata) {
          d->character->pcdata->macro_count--;
          if (d->character->pcdata->macro_count < 1) {
            d->character->pcdata->macro_count = 0;
            clear_macro_marks (d->character);
          }
        }        
      }
    
      if ( d->inbuf[i] == '\0' ) {
        if (d->character && d->character->pcdata) {
          if (d->character->pcdata->macro_count > 0) {   /* fixes weird situations */          
            d->character->pcdata->macro_count = 0;
            clear_macro_marks (d->character);          
          }          
        }
        return;
      }
    }

    /*
     * Canonical input processing.
     */
    for ( i = 0, k = 0; d->inbuf[i] != '\n' && d->inbuf[i] != '\r'; i++ )
    {
  if ( k >= MAX_INPUT_LENGTH - 2 )
  {
      write_to_descriptor( d->descriptor, "Input too long.\n\r", 0, d );

      /* skip the rest of the line */
      for ( ; d->inbuf[i] != '\0'; i++ )
      {
    if ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
        break;
      }
      d->inbuf[i]   = '\n';
      d->inbuf[i+1] = '\0';
      break;
  }

  if ( d->inbuf[i] == '\b' && k > 0 )
      --k;
  else if ( isascii(d->inbuf[i]) && isprint(d->inbuf[i]) )
      d->incomm[k++] = d->inbuf[i];
    }

    /*
     * Finish off the line.
     */
    if ( k == 0 )
  d->incomm[k++] = ' ';
    d->incomm[k] = '\0';

    /*
     * Deal with bozos with #repeat 1000 ...
     */

    if ( k > 1 || d->incomm[0] == '!' )
    {
  if ( d->incomm[0] != '!' && strcmp( d->incomm, d->inlast ) )
  {
      d->repeat = 0;
  }
  else
  {
      if ( ++d->repeat >= 25 && d->character && d->connected == CON_PLAYING)
      {
    sprintf( log_buf, "%s@%s input spamming!", d->character->name, d->host );
    log_string( log_buf );
    wiznet("Spam spam spam $N spam spam spam spam spam!",
           d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
    if (d->incomm[0] == '!')
    {
        wiznet(d->inlast,d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
    sprintf( log_buf, "%s", d->inlast );
    log_string( log_buf );
    }
    else
    {
        wiznet(d->incomm,d->character,NULL,WIZ_SPAM,0,get_trust(d->character));
    sprintf( log_buf, "%s", d->incomm );
    log_string( log_buf );
    }

    d->repeat = 0;
/*
    write_to_descriptor( d->descriptor,
        "\n\r*** PUT A LID ON IT!!! ***\n\r", 0 );
    strcpy( d->incomm, "quit" );
*/
      }
  }
    }


    /*
     * Do '!' substitution.
     */
    if ( d->incomm[0] == '!' )
  strcpy( d->incomm, d->inlast );
    else
  strcpy( d->inlast, d->incomm );

    /*
     * Shift the input buffer.
     */
    while ( d->inbuf[i] == '\n' || d->inbuf[i] == '\r' )
  i++;
    for ( j = 0; ( d->inbuf[j] = d->inbuf[i+j] ) != '\0'; j++ )
  ;
    return;
}



/*
 * Low level output function.
 */
bool process_output( DESCRIPTOR_DATA *d, bool fPrompt )
{
    extern bool merc_down;

    /*
     * Bust a prompt.
     */
    if (fPrompt && !merc_down && d->showstr_point)
  write_to_buffer(d,"[Hit Return to continue]\n\r",0);
    else if ((fPrompt && !merc_down && d->connected == CON_PLAYING ) &&
        ((d->character->pcdata && !d->character->pcdata->interp_fun) || 
        IS_NPC (d->character))) 
    {
  CHAR_DATA *ch;
  CHAR_DATA *victim;

  ch = d->character;

  /* battle prompt */
  if ((victim = ch->fighting) != NULL && can_see(ch,victim,FALSE))
  {
      int percent;
      char wound[100];
      char buf[MAX_STRING_LENGTH];
 
      if (victim->max_hit > 0)
    percent = victim->hit * 100 / victim->max_hit;
      else
    percent = -1;
 
      if (percent >= 100)
    sprintf(wound,"is in excellent condition.");
      else if (percent >= 90)
    sprintf(wound,"has a few scratches.");
      else if (percent >= 75)
    sprintf(wound,"has some small wounds and bruises.");
      else if (percent >= 50)
    sprintf(wound,"has quite a few wounds.");
      else if (percent >= 30)
    sprintf(wound,"has some big nasty wounds and scratches.");
      else if (percent >= 15)
    sprintf(wound,"looks pretty hurt.");
      else if (percent >= 0)
    sprintf(wound,"is in awful condition.");
      else
    sprintf(wound,"is bleeding to death.");
 
      if(IS_SET(victim->mhs,MHS_GLADIATOR) && gladiator_info.blind )
      {
         sprintf(buf,"%s %s \n\r", 
           IS_NPC(victim) ? victim->short_descr : victim->long_descr,wound);
      }
      else
      {
         sprintf(buf,"%s %s \n\r", 
           IS_NPC(victim) ? victim->short_descr : victim->name,wound);
      }
      buf[0] = UPPER(buf[0]);
      write_to_buffer( d, buf, 0);
  }


  ch = d->original ? d->original : d->character;
  if (!IS_SET(ch->display, DISP_COMPACT) )
      write_to_buffer( d, "\n\r", 2 );


  if ( IS_SET(ch->display, DISP_PROMPT) )
      bust_a_prompt( d->character );

  if (IS_SET(ch->comm,COMM_TELNET_GA))
      write_to_buffer(d,go_ahead_str,0);
    }

    /*
     * Short-circuit if nothing to write.
     */
    if ( d->outtop == 0 )
  return TRUE;

    /*
     * Snoop-o-rama.
     */
    if ( d->snoop_by != NULL )
    {
  if (d->character != NULL)
      write_to_buffer( d->snoop_by, d->character->name,0);
  write_to_buffer( d->snoop_by, "> ", 2 );
  write_to_buffer( d->snoop_by, d->outbuf, d->outtop );
    }

    /*
     * OS-dependent output.
     */
    if ( !write_to_descriptor( d->descriptor, d->outbuf, d->outtop, d ) )
    {
  d->outtop = 0;
  return FALSE;
    }
    else
    {
  d->outtop = 0;
  return TRUE;
    }
}

/* Prompt generation command */
void bust_a_prompt( CHAR_DATA *ch )
{
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    const char *str;
    const char *i;
    char *point;
    int j;
    char doors[MAX_INPUT_LENGTH];
    EXIT_DATA *pexit;
    bool found;
    const char *dir_name[] = {"N","E","S","W","U","D"};
    register int door;
 
    point = buf;
  if(!IS_NPC(ch))
  {
#ifdef OLC_VERSION
    if(ch->pcdata->ref_help)
    {
      send_to_char("ehelp: ", ch);
      send_to_char(ch->pcdata->edit_help.keyword, ch);
      if(IS_SET(ch->pcdata->edit_help.status, HELP_STAT_MODIFIED))
        send_to_char(" (Modified)", ch);
      send_to_char("\n\r", ch);
    }
#endif

    if(ch->pcdata->pulse_timer > 0)
      prompt_pulse_command(ch);

    if(IS_SET(ch->pcdata->edit_flags, (EDITMODE_HALL | EDITMODE_PERSONAL)))
    {
      if(ch->pcdata->edit_obj)
      {
        switch(ch->pcdata->edit_obj->type & PLAN_MASK_TYPE)
        {
          case PLAN_ROOM: sprintf(buf, "<Editing room {W%s{x>\n\r", ch->pcdata->edit_obj->label); break;
          case PLAN_ITEM: sprintf(buf, "<Editing item {Y%s{x>\n\r", ch->pcdata->edit_obj->label); break;
          case PLAN_MOB: sprintf(buf, "<Editing mob {B%s{x>\n\r", ch->pcdata->edit_obj->label); break;
          case PLAN_EXIT: sprintf(buf, "<Editing exit {G%s{x>\n\r", ch->pcdata->edit_obj->label); break;
        }
        send_to_char(buf, ch);   
      }
      else if(IS_SET(ch->pcdata->edit_flags, EDITMODE_RULES))
        send_to_char("<Editing clan rules>\n\r", ch);
      else if(IS_SET(ch->pcdata->edit_flags, EDITMODE_CHARTER))
        send_to_char("<Editing clan charter>\n\r", ch);
      else if(IS_SET(ch->pcdata->edit_flags, EDITMODE_HALL))
        send_to_char("<Editing clan hall>\n\r", ch);
      else
        send_to_char("<Editing personal rooms>\n\r", ch);
    }
  }

   if (IS_SET(ch->comm,COMM_AFK))
   {
     if(!IS_NPC(ch))
     {
	sprintf( buf, "{x<{CAFK{x - %d> ", ch->pcdata->afk_counter);
     }
     else
     {
	strcpy( buf, "{x<{CAFK{x> ");
     }
     send_to_char(buf,ch);
     return;
   }
    else
   if ( !IS_NPC(ch) && ch->pcdata->wraith_timer > 0 )
   {
	send_to_char(" < .. Fading to Wraithform .. > ",ch );
	return;
   }

   if(!IS_NPC(ch) && !IS_SET(ch->pcdata->new_opt_flags, OPT_NOGPROMPT)
    && (ch->leader || ch->follower))
   {/* They've got a group.  List up to 8 other members of it */
       CHAR_DATA *group;
      if(ch->leader)
        group = ch->leader;
      else
        group = ch;
      if(group->follower)
      {// This check is just paranoia
	char transfer[50];
        int hit_perc;
        char hit_color;
	int count = 0;
        bool first = TRUE;
        buf[0] = '>';
        buf[1] = '\0';
        while(group)
        {
          count++;
          if(count > 9)
            break;// Only a row across, for now at least.
          if(group != ch)
          {// Everyone except this player
            hit_perc = group->hit * 100 / (group->max_hit == 0 ? 1 : group->max_hit);
            if(hit_perc > 75)
              hit_color = 'W';
            else if(hit_perc > 50)
              hit_color = 'Y';
            else if(hit_perc > 25)
              hit_color = 'R';
            else
              hit_color = 'r';
            /* First four of their name, then their % hp */
            sprintf(transfer, "%.4s:{%c%3d%%{x ", capitalize( PERS(group,ch,TRUE) ), hit_color, hit_perc);
            strcat(buf, transfer);
          }
          if(first)
          {
            group = group->follower;
            first = FALSE;
          }
          else
          group = group->next_groupmate;
        }
	buf[strlen(buf) - 1] = '\0';// Chop off the last space
	strcat(buf, "\n\r");
        send_to_char(buf, ch);
      }
   }

    str = ch->prompt;
    if (str == NULL || str[0] == '\0')
    {
	sprintf( buf, "{x<%dhp %dm %dmv> ",
	    ch->hit,ch->mana,ch->move);
	send_to_char(buf,ch);
	return;
    }

   while( *str != '\0' )
   {
      if( *str != '%' )
      {
	 *point++ = *str++;
	 continue;
      }
      ++str;
      switch( *str )
      {
	 default :
	    i = " "; break;
	case 'e':
	    found = FALSE;
	    doors[0] = '\0';
	    if ( IS_AFFECTED(ch,AFF_BLIND) )
		strcat(doors,"(blinded)");
	    else
	    for (door = 0; door < 6 ; door++ )
	    {
		if ((pexit = ch->in_room->exit[door]) != NULL
		&&  pexit ->u1.to_room != NULL
		&&  (can_see_room(ch,pexit->u1.to_room)
		||   (IS_AFFECTED(ch,AFF_INFRARED) 
		&&    !IS_AFFECTED(ch,AFF_BLIND)))
		&&  !IS_SET(pexit->exit_info,EX_CLOSED))
		{
		    found = TRUE;
		    strcat(doors,dir_name[door]);
		}
	    }
	    if (!found)
		strcat(buf,"none");
	    sprintf(buf2,"%s",doors);
	    i = buf2; break;
    	case 'b' :
	    if ( is_affected(ch,gsn_rage) )
            {
               sprintf( buf2, "???" );
               i = buf2;
               break;
            }
	    else
	    {
	    sprintf(buf2,"[HP:");
	    if (ch->max_hit <= 0)
            {
                i = buf2;
		break;// Don't even try
            }
	    if ( ch->hit * 100 / ch->max_hit > 75 )
		strcat(buf2,"{W");
	    else
	    if ( ch->hit * 100 / ch->max_hit > 50 )
		strcat(buf2,"{Y");
	    else
	    if ( ch->hit * 100 / ch->max_hit > 25 )
		strcat(buf2,"{R");
	    else
		strcat(buf2,"{r");
	    for( j = 0 ; j < ( ch->hit* 100 / ch->max_hit )/ 5 ; j ++ )
		    strcat(buf2,"*");
 	    strcat(buf2,"{x");
	    for( j = 0; j < 20 - (ch->hit*100/ch->max_hit)/ 5 ; j++ )
		    strcat(buf2,"-");
	    strcat(buf2,"]");
	    }
	    i = buf2; break;
	 case 'B' :
            sprintf(buf2,"[M:");
	    if (ch->max_mana <= 0)
		break;// Don't even try
	    if ( ch->mana * 100 / ch->max_mana > 75 )
		strcat(buf2,"{W");
	    else
	    if ( ch->mana * 100 / ch->max_mana > 50 )
		strcat(buf2,"{Y");
	    else
	    if ( ch->mana * 100 / ch->max_mana > 25 )
		strcat(buf2,"{R");
	    else
		strcat(buf2,"{r");	
	for( j = 0 ; j < ( ch->mana * 100 / ch->max_mana)/ 5 ; j ++ )
		strcat(buf2,"*");
	strcat(buf2,"{x");
	for( j = 0; j < 20 - (ch->mana*100/ch->max_mana)/5 ; j++ )
		strcat(buf2,"-");
	strcat(buf2,"]");
	i = buf2; break;
	 case 'c' :
	    sprintf(buf2,"%s","\n\r");
	    i = buf2; break;
	 case 'p' :
	    if ( is_affected(ch,gsn_rage) )
            {
               sprintf( buf2, "???" );
               i = buf2;
               break;
            }
	    else
		if(ch->max_hit <= 0)
                {
                    sprintf(buf2, "{R%d%%{x", ch->max_hit);
                    i = buf2;
		    break;
                }
           sprintf( buf2, "{%s%d%%{x", 
	          ch->hit < ch->max_hit /2 ? "R" :
		  ch->hit < ch->max_hit * 3 / 4 ? "Y" : "W", (ch->hit*100)/ch->max_hit );
	    i = buf2; break;
	 case 'h' :
	    if ( is_affected(ch,gsn_rage) )
	       sprintf( buf2, "???" );
	    else
	    {
              if(ch->max_hit > 0)
              {
	    	if ( ch->hit * 100 / ch->max_hit > 75 )
		    strcpy(buf2,"{W");
		else
		if ( ch->hit * 100 / ch->max_hit > 50 )
		    strcpy(buf2,"{Y");
		else
		if ( ch->hit * 100 / ch->max_hit > 25 )
		    strcpy(buf2,"{R");
		else
		    strcpy(buf2,"{r");
	         sprintf( buf2 + 2, "%d{x", ch->hit );
               }
               else
	         sprintf( buf2, "%d", ch->hit );
	    }
	    i = buf2; break;
	 case 'H' :
	    if ( is_affected(ch,gsn_rage) )
               sprintf( buf2, "???" );
	    else
	       sprintf( buf2, "{W%d{x", ch->max_hit );
	    i = buf2; break;
	 case 'P' :
		if(ch->max_mana <= 0)
		    break;
	    sprintf( buf2, "{%s%d%%{x",
		ch->mana < ch->max_mana / 2 ? "R" : 
		ch->mana < ch->max_mana * 3 / 4 ? "Y" : "W", (ch->mana*100)/ch->max_mana );
	    i = buf2; break;
	case 'm' :
		if(ch->max_mana <= 0)
                {
                    sprintf(buf2, "%d", ch->mana);
                    i = buf2;
		    break;
                }
	    	if ( ch->mana * 100 / ch->max_mana > 75 )
		    strcpy(buf2,"{W");
		else
		if ( ch->mana * 100 / ch->max_mana > 50 )
		    strcpy(buf2,"{Y");
		else
		if ( ch->mana * 100 / ch->max_mana > 25 )
		    strcpy(buf2,"{R");
		else
		    strcpy(buf2,"{r");
	    sprintf( buf2 + 2, "%d{x", ch->mana );
	    i = buf2; break;
	 case 'M' :
	    sprintf( buf2, "{W%d{x", ch->max_mana );
	    i = buf2; break;
	 case 'v' :
	    if ( ch->max_move < 1 )
	    {
              bug("bad max_move",0);
	    }
            else
	    	if ( ch->move * 100 / ch->max_move > 75 )
		    strcpy(buf2,"{W");
		else
		if ( ch->move * 100 / ch->max_move > 50 )
		    strcpy(buf2,"{Y");
		else
		if ( ch->move * 100 / ch->max_move > 25 )
		    strcpy(buf2,"{R");
		else
		    strcpy(buf2,"{r");
	    sprintf( buf2 + 2, "%d{x", ch->move );
	     i = buf2; break;
 	 case 'w' :
	    sprintf( buf2, "{W%d{x", get_carry_weight(ch) / 10);
//           sprintf( buf2, "%ld%%", (get_carry_weight(ch)*100)/can_carry_w(ch) );
	   i = buf2; break;
 	 case 'W' :
	    sprintf( buf2, "{W%d{x", can_carry_w(ch) / 10);
//           sprintf( buf2, "%ld%%", (get_carry_weight(ch)*100)/can_carry_w(ch) );
	   i = buf2; break;
	 case 'V' :
		if(ch->max_move <= 0)
			break;
	    sprintf( buf2, "{W%d{x", ch->max_move );
	    i = buf2; break;
/*            sprintf(buf2,"[MV:");// Remove % mv
	    if ( ch->move * 100 / ch->max_move > 75 )
		strcat(buf2,"{W");
	    else
	    if ( ch->move * 100 / ch->max_move > 50 )
		strcat(buf2,"{Y");
	    else
	    if ( ch->move * 100 / ch->max_move > 25 )
		strcat(buf2,"{R");
	    else
		strcat(buf2,"{r");	
	for( j = 0 ; j < ( ch->move * 100 / ch->max_move)/ 20 ; j++ )
		strcat(buf2,"*");
	strcat(buf2,"{x");
	for( j = 0; j < 5 - (ch->move*100/ch->max_move)/20 ; j++ )
		strcat(buf2,"-");
	strcat(buf2,"]");
	i = buf2; break;*/
	 case 'n' :
	    sprintf( buf2, "%d.%d", ch->carry_number/10,ch->carry_number%10 );
		i = buf2; break;
	 case 'N' :
		sprintf( buf2, "%d", can_carry_n(ch) / 10 );
		i = buf2; break;
	 case 't' :
	 	sprintf( buf2, "%d", time_info.hour );
		i = buf2; break;
	/*
	 case 'V' :
	    sprintf( buf2, "{W%d{x", ch->max_move );
	    i = buf2; break;
	 */
	 case 'x' :
	    sprintf( buf2, "%d", ch->exp );
	    i = buf2; break;
	 case 'X' :
	    sprintf(buf2, "%d", IS_NPC(ch) ? 0 :
	    (ch->level + 1) * exp_per_level(ch,ch->pcdata->points) - ch->exp);
	    i = buf2; break;
	 case 'g' :
	    sprintf( buf2, "%ld", ch->gold);
	    i = buf2; break;
	 case 's' :
	    sprintf( buf2, "%ld", ch->silver);
	    i = buf2; break;
	 case 'a' :
	       sprintf( buf2, "%s", IS_GOOD(ch) ? "good" : IS_EVIL(ch) ?
		"evil" : "neutral" );
	    i = buf2; break;
	 case 'r' :
	    if( ch->in_room != NULL )
	       sprintf( buf2, "%s", 
		((!IS_NPC(ch) && IS_SET(ch->act,PLR_HOLYLIGHT)) ||
		 (!IS_AFFECTED(ch,AFF_BLIND) && !room_is_dark( ch->in_room )))
		? ch->in_room->name : "darkness");
	    else
	       sprintf( buf2, " " );
	    i = buf2; break;
	 case 'R' :
	    if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
	       sprintf( buf2, "%d", ch->in_room->vnum );
	    else
	       sprintf( buf2, " " );
	    i = buf2; break;
	 case 'z' :
	    if( IS_IMMORTAL( ch ) && ch->in_room != NULL )
	       sprintf( buf2, "%s", ch->in_room->area->name );
	    else
	       sprintf( buf2, " " );
	    i = buf2; break;
         case 'S' :
	   sprintf(buf2,"{W%d{x",IS_NPC(ch)?0:ch->pcdata->sac);
//	   sprintf(buf2,"%d%%",IS_NPC(ch)?0:(ch->pcdata->sac*100)/MAX_SAC_PNTS);
           i = buf2; break;
	 case 'y' :
	    if( IS_IMMORTAL( ch ) && ch->invis_level > 0)
    	        sprintf(buf2," ({WWizi{x@{W%d{x)", ch->invis_level);
	    else
		buf2[0] = '\0';
	    i = buf2; break;
	 case 'Y' : 
	     if ( IS_IMMORTAL( ch ) && ch->invis_level > 0)
		sprintf(buf2,"%d", ch->invis_level); 
	     else buf2[0] = '\0'; 
	     i = buf2; break;
	 case 'i' :
	    if( IS_IMMORTAL( ch ) && ch->incog_level > 0)
    	        sprintf(buf2," ({WIncog{x@{W%d{x)", ch->incog_level);
	    else
		buf2[0] = '\0';
	    i = buf2; break;
	 case 'I' :
	    if( IS_IMMORTAL( ch ) && ch->incog_level > 0)
    	        sprintf(buf2,"%d", ch->incog_level);
	    else
		buf2[0] = '\0';
	    i = buf2; break;
	 case '%' :
	    sprintf( buf2, "%%" );
	    i = buf2; break;
      }
      ++str;
      while( (*point = *i) != '\0' )
	 ++point, ++i;
}
   buf2[0] = '\0';
   i = buf2;
      while( (*point = *i) != '\0' )
	 ++point, ++i;
   
   point = &buf[0];

   send_to_char("{x",ch);
   write_to_buffer( ch->desc, point, 0 );

   return;
}


/*
 * Append onto an output buffer.
 */
void write_to_buffer( DESCRIPTOR_DATA *d, const char *txt, int length )
{
  if (!d) return;
    /*
     * Find length in case caller didn't.
     */
    if ( length <= 0 )
  length = strlen(txt);
    /*
     * Initial \n\r if needed.
     */
    if ( d->outtop == 0 && !d->fcommand )
    {
  d->outbuf[0]    = '\n';
  d->outbuf[1]    = '\r';
  d->outtop       = 2;
    }

    /*
     * Expand the buffer as needed.
     */
    while ( d->outtop + length >= d->outsize )
    {
  char *outbuf;

  if (d->outsize >= 32000)
  {
      bug("Buffer overflow. Closing.\n\r",0);
      close_socket(d);
      return;
  }
#ifdef OLC_VERSION
  outbuf      = alloc_mem( 2 * d->outsize );
#else /*game version*/
  outbuf      = GC_MALLOC( 2 * d->outsize );
#endif
  strncpy( outbuf, d->outbuf, d->outtop );
  free_mem( d->outbuf, d->outsize );
  d->outbuf   = outbuf;
  d->outsize *= 2;
    }

    /*
     * Copy.
     */
    strcpy( d->outbuf + d->outtop, txt );
    d->outtop += length;
    return;
}


#if defined (GAME_VERSION)
/* Color compliant version */
bool write_to_descriptor( int desc, char *str, int length, DESCRIPTOR_DATA *d )
{
   int iStart;
   int nWrite;
   int nBlock;
   BUFFER *txt;
   char *point;
   char buf[2];
   char buf2[MAX_STRING_LENGTH];
#if defined(macintosh) || defined(MSDOS)
   if ( desc == 0 )
     desc = 1;
#endif

  if( d->character == NULL || !IS_SET(d->character->mhs,MHS_OLC) )
  {
   txt = new_buf();
   buf[1] = '\0';
   for(point = str; *point; point++)
     {  
	if( *point == '{')
	  {
	     ++point;
	     if ( ( d->character != NULL &&
		    IS_SET(d->character->display,DISP_COLOR) )
		    || *point == '!' )
	       {   
		  switch( *point )
		    {
		     default: /* fixed to accout for terminals */
		       sprintf( buf2, C_WHITE CLEAR );
		       break;
		     case 'x':
		       sprintf( buf2, C_WHITE CLEAR );
		       break;
		     case 'b':
		       sprintf( buf2, C_BLUE );
		       break;
		     case 'c':
		       sprintf( buf2, C_CYAN );
		       break;
		     case 'g':
		       sprintf( buf2, C_GREEN );
		       break;
		     case 'm':
		       sprintf( buf2, C_MAGENTA );
		       break;
		     case 'r':
		       sprintf( buf2, C_RED );
		       break;
		     case 'w':
		       sprintf( buf2, C_WHITE );
		       break;
		     case 'y':
		       sprintf( buf2, C_YELLOW );
		       break;
		     case 'B':
		       sprintf( buf2, C_B_BLUE );
		       break;
		     case 'C':
		       sprintf( buf2, C_B_CYAN );
		       break;
		     case 'G':
		       sprintf( buf2, C_B_GREEN );
		       break;
		     case 'M':
		       sprintf( buf2, C_B_MAGENTA );
		       break;
		     case 'R':
		       sprintf( buf2, C_B_RED );
		       break;
		     case 'W':
		       sprintf( buf2, C_B_WHITE );
		       break;
		     case 'Y':
		       sprintf( buf2, C_B_YELLOW );
		       break;
		     case 'D':
		       sprintf( buf2, C_D_GREY );
		       break;
		     case '!':
	       if (d->character != NULL && IS_IMMORTAL(d->character) )
			 {
			    sprintf(buf2, "%c", '\a');
			 }
		       else buf2[0] = '\0';
		       break;
		     case '{':
		       sprintf( buf2, "%c", '{' );
		       break;
		     case '\n':
		       sprintf( buf2, "\n" );
		       break;
		    }             
		  add_buf(txt, buf2);
		  continue;
	       }
  	       else if(*point == '\n' || *point == '{')
	       {
	          sprintf(buf2, "%c", *point);
	       	  add_buf(txt, buf2);
	       }

	     continue;
	  }
	buf[0] = *point;
	add_buf(txt,buf);
     }
/*   if ( strlen(buf_string(txt)) != length )*/
     length = strlen(buf_string(txt));
  }
  else
  {
    if ( length <= 0 ) length = strlen(str);
    txt = new_buf();
    add_buf(txt,str);
  }

   for ( iStart = 0; iStart < length; iStart += nWrite )
     {
	nBlock = UMIN( length - iStart, 4096 );
	if ( ( nWrite = write( desc, buf_string(txt) + iStart, nBlock ) ) < 0 )
	  { perror( "Write_to_descriptor" ); return FALSE; }
     } 
   clear_buf(txt);
   free_buf(txt);
   return TRUE;
}
#endif

/*
 * Lowest level output function.
 * Write a block of text to the file descriptor.
 * If this gives errors on very long blocks (like 'ofind all'),
 *   try lowering the max block size.
*
 * Replaced with color-compliant version *
 */

#if defined (OLC_VERSION)
bool write_to_descriptor( int desc, char *txt, int length, DESCRIPTOR_DATA *d )
{
    int iStart;
    int nWrite;
    int nBlock;

#if defined(macintosh) || defined(MSDOS)
    if ( desc == 0 )
  desc = 1;
#endif

    if ( length <= 0 )
  length = strlen(txt);

    for ( iStart = 0; iStart < length; iStart += nWrite )
    {
  nBlock = UMIN( length - iStart, 4096 );
  if ( ( nWrite = write( desc, txt + iStart, nBlock ) ) < 0 )
      { perror( "Write_to_descriptor" ); return FALSE; }
    } 

    return TRUE;
}
#endif

/* Let's do this here - character creation management */
/* To add a new step: Add it to is_creation, add it to which steps lead to it
	 in creation_step and which it leads back to, add its message to creation_message
	 and add its processing to creation_input */
bool is_creation(DESCRIPTOR_DATA *d)
{
	switch(d->connected)
	{
		case CON_GET_SURNAME:
		case CON_GET_NEW_RACE:
		case CON_GET_NEW_SEX:
		case CON_GET_NEW_CLASS:
		case CON_DEFAULT_CHOICE:
		case CON_GET_OLD_CLASS:
		case CON_GET_ALIGNMENT:
		case CON_GEN_GROUPS:
		case CON_PICK_WEAPON:
		case CON_PICK_STATS:
		case CON_PICK_DEITY:
		case CON_PICK_STATS_DEFAULT: return TRUE;
	}
	return FALSE;
}

int creation_step(DESCRIPTOR_DATA *d, bool forward, bool accept)
{// accept is optional - only relevant for certain steps
	CHAR_DATA *ch = d->character;
  if(forward)
	{
		switch(d->connected)
		{
			case CON_GET_SURNAME: return CON_GET_NEW_RACE;
			case CON_GET_NEW_RACE: if(IS_SET(ch->mhs,MHS_PREFRESHED))
					return CON_GET_NEW_CLASS;
				return CON_GET_NEW_SEX;
			case CON_GET_NEW_SEX:	return CON_GET_NEW_CLASS;
			case CON_GET_NEW_CLASS: return CON_DEFAULT_CHOICE;
			case CON_DEFAULT_CHOICE: if(accept)
				{// Accept means you want to customize
				  int count = 0;// This is just for information - The oldclass must be set elsewhere
				  int i = 0;
          for ( i = 0 ; i < MAX_CLASS ; i++ )
          {
             if ( class_table[i].reclass )
                continue;
    
             if (class_table[ch->class].allowed[0] == i ||
                 class_table[ch->class].allowed[1] == i )
                count++;
          }

					if(count > 1 && !IS_SET(ch->act,PLR_RECLASS))
						return CON_GET_OLD_CLASS;
					else
						return CON_GET_ALIGNMENT;
				}
				else
					return CON_PICK_STATS_DEFAULT; // Skip everything else.
			case CON_GET_OLD_CLASS: return CON_GET_ALIGNMENT;
			case CON_GET_ALIGNMENT: return CON_GEN_GROUPS;
			case CON_GEN_GROUPS: return CON_PICK_WEAPON;
			case CON_PICK_WEAPON: return CON_PICK_STATS;
			case CON_PICK_STATS: if(IS_SET(ch->mhs,MHS_PREFRESHED))
				{
					creation_finalize(d, FALSE);
					return CON_READ_MOTD;// Done!
				}
				return CON_PICK_DEITY;
			case CON_PICK_DEITY: creation_finalize(d, FALSE);
				return CON_READ_MOTD;// Done!
			case CON_PICK_STATS_DEFAULT: creation_finalize(d, TRUE);
				return CON_READ_MOTD;// Done!
			default: return CON_GET_SURNAME;// Something went wrong, restart - safer than ending it
		}
	}
	else
	{// Unfortunately, no great way to avoid having a separate list -- at least it adds flexibility
		switch(d->connected)
		{
			case CON_GET_NEW_RACE: return CON_GET_SURNAME;
			case CON_GET_NEW_CLASS: if(IS_SET(ch->mhs, MHS_PREFRESHED))
					return CON_GET_NEW_RACE;
				return CON_GET_NEW_SEX;
			case CON_GET_NEW_SEX:	return CON_GET_NEW_RACE;
			case CON_DEFAULT_CHOICE: if(IS_SET(ch->act,PLR_RECLASS))
					return d->connected;// Can't back up past here
				else
					return CON_GET_NEW_CLASS;
			case CON_GET_ALIGNMENT:	if(class_table[ch->class].reclass && !IS_SET(ch->act,PLR_RECLASS))
			      	{
				  int count = 0;// This is just for information - The oldclass must be set elsewhere
				  int i = 0;
			          for ( i = 0 ; i < MAX_CLASS ; i++ )
			          {
			             if ( class_table[i].reclass )
			                continue;
    
			             if (class_table[ch->class].allowed[0] == i ||
			                 class_table[ch->class].allowed[1] == i )
			                count++;
			          }

				  if(count > 1)
				  	return CON_GET_OLD_CLASS;
				}
				return CON_DEFAULT_CHOICE;
			case CON_GET_OLD_CLASS: return CON_DEFAULT_CHOICE;
			case CON_GEN_GROUPS: return CON_GET_ALIGNMENT;
			case CON_PICK_WEAPON: return CON_GEN_GROUPS;
			case CON_PICK_STATS: return CON_PICK_WEAPON;
			case CON_PICK_DEITY: return CON_PICK_STATS;
			case CON_PICK_STATS_DEFAULT: return CON_DEFAULT_CHOICE;
			default: return d->connected;// Don't go anywhere back if it's not a handled step - can't back all the way out
		}
	}
} 

void creation_message(DESCRIPTOR_DATA *d, bool forward)
{// Forward will sometimes show more details than backwards, reduces spam
	char buf[256];
	CHAR_DATA *ch = d->character;
	int i = 0;
  switch(d->connected)
	{
  case CON_GET_SURNAME:
    if(forward)
      sprintf( buf, "\n\rNew character. Enter back to redo a previous step during creation.\n\rChoose a surname for %s (Leave blank for none): ", ch->name);
    else
      sprintf(buf, "\n\rChoose a surname for %s (Leave blank for none): ", ch->name);
    write_to_buffer( d, buf, 0 );
    break;
	case CON_GET_NEW_RACE:
  write_to_buffer(d,"\n\rThe following races are available:\n\r  ",0);
  for ( i = 1; race_table[i].name != NULL; i++ )
  {
   if (!race_table[i].pc_race)
    break;
   write_to_buffer(d," * ",0);
   write_to_buffer(d,race_table[i].name,0);
   write_to_buffer(d,"\n\r",0);
   write_to_buffer(d," ",1);
  }

  write_to_buffer(d,"\n\r",0);
  write_to_buffer(d,"What is your race (help for more information)? ",0);
   break;
  case CON_GET_NEW_SEX:
    write_to_buffer( d, "\n\rWhat is your sex (M/F)? ", 0 );
    break;
  case CON_GET_NEW_CLASS:
  strcpy( buf, "\n\rSelect a class (Help <class> for more information)\n\r [" );
  for ( i = 0; i < MAX_CLASS; i++ )
  {
    if (i > 0)
      strcat( buf, " " );
    strcat( buf, class_table[i].name );
  }
  strcat( buf, "]: " );
  write_to_buffer( d, buf, 0 );
  break;
	case CON_DEFAULT_CHOICE:
    write_to_buffer(d,"\n\rDo you wish to customize this character?\n\r",0);
    write_to_buffer(d,"Customization takes time, but allows a wider range of skills and abilities.\n\r",0);
    write_to_buffer(d,"\n\rIf you are new to the game, it is suggested you do not customize now.\n\r\n\r",0);
    write_to_buffer(d,"Customize (Y/N)? ",0);
    break;
	case CON_GET_OLD_CLASS:
     strcpy( buf, "\n\rSelect an old class [" );
     for ( i = 0 ; i < MAX_CLASS ; i++ )
     {
        if (i > 0)
           strcat( buf, " " );

        if ( class_table[i].reclass )
           continue;

        if (class_table[ch->class].allowed[0] == i ||
            class_table[ch->class].allowed[1] == i )
           strcat( buf, class_table[i].name );
     }
     strcat( buf, "]: " );
     write_to_buffer( d, buf, 0 );
     break;
	case CON_GET_ALIGNMENT:
    write_to_buffer( d, "\n\rYou may be good, neutral, or evil.\n\r",0);
    write_to_buffer( d, "Which alignment (G/N/E)? ",0);
    break;
	case CON_GEN_GROUPS:
	  write_to_buffer(d, "\n\r", 0);
    do_help(ch,"group header");
    list_group_costs(ch);
    write_to_buffer(d,"You already have the following skills:\n\r",0);
    do_skills(ch,"");
    send_to_char("\n\r", ch);
    do_help(ch,"menu choice");
    break;
	case CON_PICK_WEAPON:
      write_to_buffer(d,
    "\n\rPlease pick a weapon from the following choices:\n\r",0);
      buf[0] = '\0';
      for ( i = 0; weapon_table[i].name != NULL; i++)
    if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
    {
        strcat(buf,weapon_table[i].name);
        strcat(buf," ");
    }
    strcat(buf,"\n\rYour choice? ");
    write_to_buffer(d,buf,0);
    break;
	case CON_PICK_DEITY:
	 write_to_buffer(d, "\n\r", 0);
   write_to_buffer(d,
    "Please pick a Deity (help deity for more information)\n\r",0);
   write_to_buffer(d,"Your choice? ",0);
   break;
	case CON_PICK_STATS:
	case CON_PICK_STATS_DEFAULT:
	 write_to_buffer(d, "\n\rHow do you identify yourself? (help for more information)\n\r",0);
	 write_to_buffer(d, "Strength (Str) - I am strong in combat.\n\r",0);
	 write_to_buffer(d, "Dexterity (Dex) - I am good with precise actions.\n\r", 0);
	 write_to_buffer(d, "Intelligence (Int) - I am a quick learner.\n\r", 0);
	 write_to_buffer(d, "Wisdom (Wis) - I have many insights into the world.\n\r", 0);
	 write_to_buffer(d, "Constitution (Con) - I am tough and fit.\n\r", 0);
   write_to_buffer(d, "Charisma (Cha) - I can convince people to do what I want.\n\r", 0);
   break; 
  default:
      write_to_buffer( d, "Please answer (Y/N)? ", 0 );
      return;
  }
}

void clear_picked_groups(DESCRIPTOR_DATA *d)
{
	int i;
	CHAR_DATA *ch = d->character;
  if(ch->gen_data == NULL)
  {
    ch->gen_data = new_gen_data();// Do this once here
    ch->gen_data->bonus_points = 50;
    bug("Bad gen data - None present.", 0);
  }
  /* Default, drop everything in case they customized then backed out */
  for (i = 0; i < MAX_GROUP; i++)
  {
    if (group_table[i].name == NULL)
      break;
    ch->gen_data->group_chosen[i] = FALSE;
    gn_remove(ch,i);
  }
  for (i = 0; i < MAX_SKILL; i++)
	{
    if (skill_table[i].name == NULL)
        break;

    ch->gen_data->skill_chosen[i] = FALSE;
    ch->pcdata->learned[i] = 0;
  }
  
  for (i = 0; i < 5; i++)
  {
      if (pc_race_table[ch->race].skills[i] == NULL)
    break;
      group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
  }
  /* add cost */
  ch->pcdata->points = pc_race_table[ch->race].points;

  group_add(ch,"rom basics",FALSE);
  group_add(ch,class_table[ch->class].base_group,FALSE);
  ch->pcdata->learned[gsn_recall] = 50;
  ch->pcdata->learned[gsn_scan] = 50;
  ch->pcdata->learned[gsn_swim] = 50;

  ch->gen_data->points_chosen = ch->pcdata->points;
}

void creation_finalize(DESCRIPTOR_DATA *d, bool def)
{// Fill in everything else
  int i;
  CHAR_DATA *ch = d->character;
  int count = 0;
  int oldClass = 0;
  if (!IS_SET(ch->mhs,MHS_PREFRESHED) && !IS_SET(ch->act,PLR_RECLASS))
  {
     sprintf( log_buf, "%s@%s new player.", ch->name, d->host );
     log_string( log_buf );
     wiznet("Newbie alert!  $N sighted.",ch,NULL,WIZ_NEWBIE,0,0);
     wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));

    SET_BIT(ch->comm,COMM_NOBITCH);
    SET_BIT(ch->comm,COMM_NOAUCTION);
    SET_BIT(ch->comm,COMM_NOMUSIC);
//    SET_BIT(ch->comm,COMM_NOQUOTE);
//    SET_BIT(ch->comm,COMM_NOGRATS);
    SET_BIT(ch->act,PLR_NOOUTOFRANGE);

    /* Stamp Time of Creation */
    ch->pcdata->created_date = current_time;
  
    /* Set all new characters as Newbies */
    ch->clan = nonclan_lookup("newbie");
  }
  else
  {
     int newLevel;
     sprintf( log_buf, "%s@%s pfresh player.", ch->name, d->host );
     log_string( log_buf );
     wiznet("Pfresh alert!  $N sighted.",ch,NULL,WIZ_NEWBIE,0,0);
     wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
      /* calculate number of debit levels to give refresh chars */
  	newLevel = ch->exp / exp_per_level(ch,ch->pcdata->points);
  	/* remove the free level "1" */
  	newLevel -= 1;
  	if( ( ch->level < 11 ) && (newLevel > ch->level ) )
  	{
  	  newLevel = ch->level ;
  	}
  	if(   IS_SET(ch->act,PLR_VAMP) ||
  	      IS_SET(ch->act,PLR_WERE) ||
  	      IS_SET(ch->act,PLR_MUMMY) 
            )
  	{
  	  if ( newLevel > 76 )
  	  {
  	    newLevel = 76;
  	  }
  	  ch->pcdata->debit_level = newLevel;
  	}
  	else
  	{
  	 /* not a remort, max it at 50 */
  	 if(newLevel > 50 )
  	 {
  	   newLevel =50;
           }
  	 ch->pcdata->debit_level = newLevel;

  	ch->exp = exp_per_level(ch,ch->pcdata->points) ;
  	 /* Remove Remort Status */
  	 if (IS_SET(ch->act,PLR_VAMP))
  	    REMOVE_BIT(ch->act,PLR_VAMP);
  	 if (IS_SET(ch->act,PLR_WERE))
  	    REMOVE_BIT(ch->act,PLR_WERE);
  	 if (IS_SET(ch->act,PLR_MUMMY))
  	    REMOVE_BIT(ch->act,PLR_MUMMY);
        }
  }
  if(IS_SET(ch->act,PLR_RECLASS))
	REMOVE_BIT(ch->act,PLR_RECLASS);
  if(def)
  {
    if(ch->pcdata->old_class == 0)
    {// Hasn't been set yet, set now
    /* Safety check - check auto-set if it got missed somehow */
      if (!class_table[ch->class].reclass )
        ch->pcdata->old_class = ch->class; 
      else
      {
        for ( i = 0 ; i < MAX_CLASS ; i++ )
        {
           if ( class_table[i].reclass )
              continue;
    
           if (class_table[ch->class].allowed[0] == i ||
               class_table[ch->class].allowed[1] == i )
               {
                oldClass = i;
                count++;
               }
        }
        if(count == 1)
          ch->pcdata->old_class = oldClass;
        else
        {// Needs setting based on attribute
          int stat = ch->pcdata->pref_stat;// Saving code space
          if(ch->class == class_lookup("monk"))
          {
            if(stat == STAT_STR || stat == STAT_DEX || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("thief");
            else// int, wis, cha -> cleric
              ch->pcdata->old_class = class_lookup("cleric");
          }
          else if(ch->class == class_lookup("paladin"))
          {
            if(stat == STAT_STR || stat == STAT_DEX || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("warrior");
            else// int, wis, cha -> cleric
              ch->pcdata->old_class = class_lookup("cleric");
          }
          else if(ch->class == class_lookup("berzerker"))
          {
            if(stat == STAT_STR || stat == STAT_WIS || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("warrior");
            else// dex, int, cha -> thief
              ch->pcdata->old_class = class_lookup("thief");
          }
          else if(ch->class == class_lookup("druid"))
          {
            if(stat == STAT_STR || stat == STAT_WIS || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("cleric");
            else// dex, int, cha -> mage
              ch->pcdata->old_class = class_lookup("mage");
          }
          else if(ch->class == class_lookup("assassin"))
          {
            if(stat == STAT_STR || stat == STAT_DEX || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("thief");
            else// int, wis, cha -> mage
              ch->pcdata->old_class = class_lookup("mage");
          }
          else if(ch->class == class_lookup("samurai"))
          {
            if(stat == STAT_STR || stat == STAT_DEX || stat == STAT_CON)
              ch->pcdata->old_class = class_lookup("warrior");
            else// int, wis, cha -> mage
              ch->pcdata->old_class = class_lookup("mage");
          }
          else
            ch->pcdata->old_class = ch->class;
        }
      }
    }
      /* Give them some skills, clear in case they backed out after customizing */
      clear_picked_groups(d);
	    group_add(ch,class_table[ch->class].default_group,TRUE);
      
	    /* With an oldclass and skills set, let's give them a weapon */
	    count = 0;
	    
	    for ( i = 0; weapon_table[i].name != NULL; i++)
	      if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
	      {
	      	ch->pcdata->learned[*weapon_table[i].gsn] = 1; // Reset this in case it got set up somehow
	        count++;
	      }
	    if(!count)
	    {// This is bad, no weapon available - just bug it, it shouldn't happen but can be corrected by an imm after
	      bug("No weapon found after creation.", 0);
	    }
	    else
	    {
	      count = number_range(1, count);
	      for ( i = 0; weapon_table[i].name != NULL; i++)
	        if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
	        {
	          count--;
	          if(!count)
	          {
	            ch->pcdata->learned[*weapon_table[i].gsn] = 80;
	            break;
	          }
	        }
	    }
	    ch->alignment = 0;// Set in case they set it then backed out
	  }

    /* initialize stats */
    for (i = 0; i < MAX_STATS; i++)
    {
	if(i == STAT_AGT || i == STAT_END)
	{
		ch->perm_stat[i] = pc_race_table[ch->race].stats[i];
		continue;// Removed
	}
        ch->perm_stat[i] = pc_race_table[ch->race].stats[i] + 3;// Simulate even distribution
    }
    ch->perm_stat[class_table[ch->pcdata->old_class].attr_prime] += 3;
    ch->perm_stat[class_table[ch->pcdata->old_class].attr_second] += 2;
    ch->perm_stat[ch->pcdata->pref_stat]++;// How you view your character sets your first train spent
    
    ch->affected_by = ch->affected_by|race_table[ch->race].aff;
    ch->imm_flags   = ch->imm_flags|race_table[ch->race].imm;
    ch->res_flags   = ch->res_flags|race_table[ch->race].res;
    ch->vuln_flags  = ch->vuln_flags|race_table[ch->race].vuln;
    ch->form        = race_table[ch->race].form;
    ch->parts       = race_table[ch->race].parts;
    ch->hit     = 20;
    ch->max_hit = 20;
    ch->pcdata->perm_hit = 20;
    ch->mana    = 100;
    ch->max_mana = 100;
    ch->pcdata->perm_mana = 100;
    ch->move    = 100;
    ch->max_move = 100;
    ch->pcdata->perm_move = 100;
    SET_BIT(ch->mhs,MHS_KAETH_CLEAN);// No old kaethan gear

    /* add racial skills back on incase they got dropped in customizing*/
    for (i = 0; i < MAX_SKILL; i++)
    {
        if (pc_race_table[ch->race].skills[i] == NULL)
           break;
        group_add(ch,pc_race_table[ch->race].skills[i],FALSE);
    }


    ch->level = 0;    
    ch->train = 1;// Four trains initially, 3 are added later
}

void creation_input(DESCRIPTOR_DATA *d, char *argument)
{
	char arg[256];
	  int i = 0, race = 0;
	char buf[256];
  CHAR_DATA *ch = d->character;
  one_argument(argument,arg);
  if(d->connected != CON_GET_SURNAME && !strcmp(arg,"back")
    && !IS_SET(d->character->mhs,MHS_PREFRESHED) )// Prefresh chars can't go back atm
  {// Accept the surname Back, they can go back if it was unintended
    d->connected = creation_step(d, FALSE, FALSE);
    creation_message(d, FALSE);
    return;
  }
  switch(d->connected)
  {
	case CON_GET_SURNAME:
	  if(strlen(arg) > 0)// They entered a surname
		  do_surname(ch,argument);
    break;//End CON_GET_SURNAME
	case CON_GET_NEW_RACE:
    one_argument(argument,arg);

    if (!strcmp(arg,"help"))
    {
        argument = one_argument(argument,arg);
        if (argument[0] == '\0')
      do_help(ch,"race help");
        else
      do_help(ch,argument);
        write_to_buffer(d,
      "What is your race (help for more information)? ",0);
        return;
    }
  
    race = race_lookup(argument);
  
    if (race == 0 || !race_table[race].pc_race || race == race_lookup("mutant") || race == race_lookup("smurf"))
    {
        if (race == race_lookup("mutant"))
        {
  	 write_to_buffer(d,"Sorry Mutants are Out of Order, Pick Again.\n\r",0);
  	 write_to_buffer(d,"\n\r",0);
        }
  
        if (race == race_lookup("smurf"))
        {
           write_to_buffer(d,"Blue and two apples tall?  You wouldn't last two minutes.  Pick again please.\n\r",0);
           write_to_buffer(d,"\n\r",0);
        }
  
  
        write_to_buffer(d,"That is not a valid race.\n\r",0);
      creation_message(d, FALSE);
      return;// Error message sent, ask them to choose a race
    }
  
  /*  if (race == race_lookup("smurf"))
    {
        write_to_buffer(d,"This is a Beta Test Race right now. You must have the password.\n\r",0);
        write_to_buffer(d, "What is the password? ",0); 
        ch->race = race;
        d->connected = CON_TEMP_SMURF_PASSWORD;
        break;
  
    }
  */
    ch->race = race;
  
    if ( race == race_lookup("mutant") )
    {
  	SET_BIT(ch->mhs,MHS_MUTANT);
  	ch->pcdata->mutant_timer = 500;
    }
  
    /* add skills */
    for (i = 0; i < 5; i++)
    {
        if (pc_race_table[race].skills[i] == NULL)
      break;
        group_add(ch,pc_race_table[race].skills[i],FALSE);
    }
    /* add cost */
    ch->pcdata->points = pc_race_table[race].points;
    ch->size = pc_race_table[race].size;

      break;// End CON_GET_NEW_RACE
  case CON_GET_NEW_SEX:
  switch ( argument[0] )
  {
  case 'm': case 'M': ch->sex = SEX_MALE;    
          ch->pcdata->true_sex = SEX_MALE;
          break;
  case 'f': case 'F': ch->sex = SEX_FEMALE; 
          ch->pcdata->true_sex = SEX_FEMALE;
          break;
  default:
      write_to_buffer( d, "That's not a sex.\n\rWhat IS your sex (M/F)? ", 0 );
      return;
  }
  break;
  case CON_GET_NEW_CLASS:
  one_argument(argument,arg);

  if (!strcmp(arg,"help"))
  {
      argument = one_argument(argument,arg);
    if (argument[0] == '\0')
      do_help(ch,"class help");
    else
      do_help(ch,argument);
    creation_message(d, FALSE);
    return;
  }

  i = class_lookup(argument);

  if ( i == -1 )
  {
      write_to_buffer( d,
    "That's not a class.\n\rWhat IS your class? ", 0 );
      return;
  }

  ch->class = i;

  if (!class_table[i].reclass )
    ch->pcdata->old_class = i; 
  else
  {
	  int count = 0;// Set oldclass now if they have no options
	  int oldClass = 0;
    for ( i = 0 ; i < MAX_CLASS ; i++ )
    {
       if ( class_table[i].reclass )
          continue;

       if (class_table[ch->class].allowed[0] == i ||
           class_table[ch->class].allowed[1] == i )
           {
             oldClass = i;
             count++;
           }
    }
    if(count == 1)
      ch->pcdata->old_class = oldClass;
    else
      ch->pcdata->old_class = 0;// Set later
  }
  break;
	case CON_DEFAULT_CHOICE:
  switch ( argument[0] )
  {
  case 'y': case 'Y': 
    clear_picked_groups(d);
    d->connected = creation_step(d, TRUE, TRUE);
    creation_message(d, TRUE);
    break;
  case 'n': case 'N': 
    d->connected = creation_step(d, TRUE, FALSE);
    creation_message(d, TRUE);
    break;
  default:
    write_to_buffer( d, "Please answer (Y/N)? ", 0 );
    break;
  }
  /* Special cases - clarifies if accept is TRUE or FALSE */
    return; // end CON_DEFAULT_CHOICE
	case CON_GET_OLD_CLASS:
    i = class_lookup(argument);
  
    if ( i == -1 || class_table[i].reclass 
         || (class_table[ch->class].allowed[0] != i &&
  	   class_table[ch->class].allowed[1] != i))
    {
        write_to_buffer( d,
      "That's not a valid old class.\n\rWhat IS your old class? ", 0 );
        return;
    }
  
    ch->pcdata->old_class = i;
     break;
	case CON_GET_ALIGNMENT:
  switch( argument[0])
  {
      case 'g' : case 'G' : ch->alignment = 750;  break;
      case 'n' : case 'N' : ch->alignment = 0;    break;
      case 'e' : case 'E' : ch->alignment = -750; break;
      default:
    write_to_buffer(d,"That's not a valid alignment.\n\r",0);
    write_to_buffer(d,"Which alignment (G/N/E)? ",0);
    return;
  }
    break;
	case CON_GEN_GROUPS:
  if (!str_cmp(argument,"done"))
  {
      char buf[256];
      /* Check for no weapons on finishing, else they get stuck */
      bool weapon_found = FALSE;
      for ( i = 0; weapon_table[i].name != NULL; i++)
      {
         if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
            weapon_found = TRUE;
      }
      if(!weapon_found)
      {
         strcat(buf,"\n\rYou have no weapons to choose from. Please add one. ");
         write_to_buffer(d,buf,0);
         return;
      }

      sprintf(buf,"Creation points: %d\n\r",ch->pcdata->points);
      send_to_char(buf,ch);
      sprintf(buf,"Experience per level: %d\n\r",
        exp_per_level(ch,ch->gen_data->points_chosen));
      send_to_char(buf,ch);
     
      /* Pfresh Chars dont pick a new deity */
      if (IS_SET(ch->mhs,MHS_PREFRESHED))
      {// Notify them of how many debit levels they have at this point
	 int newLevel = ch->exp / exp_per_level(ch,ch->pcdata->points);
	 /* remove the free level "1" */
	 newLevel -= 1;
	 if( ( ch->level < 11 ) && (newLevel > ch->level ) )
	   newLevel = ch->level ;
	 if(   IS_SET(ch->act,PLR_VAMP) ||
	       IS_SET(ch->act,PLR_WERE) ||
	      IS_SET(ch->act,PLR_MUMMY) 
           )
    	 {
    	  if ( newLevel > 76 )
    	    newLevel = 76;
    	 }
    	 else
    	 {
        if(newLevel > 50 )
    	   newLevel =50;
    	 }
         sprintf(buf,"Debit levels: %d\n\r",newLevel);
         send_to_char(buf,ch);
      }
      break;
  }

    if (!parse_gen_groups(ch,argument))
    send_to_char(
    "Choices are: list,learned,premise,add,drop,info,help, and done.\n\r",ch);
  
    send_to_char("\n\r", ch);
    do_help(ch,"menu choice");
    return;
	case CON_PICK_WEAPON:
   {
   int weapon = weapon_lookup(argument);
   if (weapon == -1 || ch->pcdata->learned[*weapon_table[weapon].gsn] <= 0)
   {
      write_to_buffer(d,
    "That's not a valid selection. Choices are:\n\r",0);
      buf[0] = '\0';
      for ( i = 0; weapon_table[i].name != NULL; i++)
    if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
    {
        strcat(buf,weapon_table[i].name);
        strcat(buf," ");
    }
      strcat(buf,"\n\rYour choice? ");
      write_to_buffer(d,buf,0);
      return;
   }
    for ( i = 0; weapon_table[i].name != NULL; i++)
    {// Reset all weapons to 1 so you can't get multiple at 80 by going back
       if (ch->pcdata->learned[*weapon_table[i].gsn] > 0)
       {
          ch->pcdata->learned[*weapon_table[i].gsn] = 1;
       }
    }
    ch->pcdata->learned[*weapon_table[weapon].gsn] = 80;
   }
	 break;
	case CON_PICK_DEITY:
     {
      int deity = 0;
      one_argument(argument,arg);
      write_to_buffer( d, "\n\r", 2 );
      if (!strcmp(arg,"help"))
      {
         argument = one_argument(argument,arg);
         if (argument[0] == '\0')
            do_help(ch,"deity");
         else
            do_help(ch,argument);
         write_to_buffer(d,
            "Please pick a Deity (help deity for more information)\n\r",0);
         write_to_buffer(d,"Your choice? ",0);
         return;
      }

      deity = deity_lookup(argument);

      if ( deity == -1)                  
      {                                                         
         write_to_buffer(d,"Not an existing Deity.\n\r",0);
         write_to_buffer(d,
            "Please pick a Deity (help deity for more information)\n\r",0);
         write_to_buffer(d,"Your choice? ",0);
         return;
      }

      /* smurfs can pick clanned deitied at creation */
      if (ch->race != race_lookup("smurf"))
      {
         if (deity_table[deity].clan && !is_clan(ch))
         {
            write_to_buffer(d,"Don't get ahead of yourself, you arent Clanned yet.\n\r",0);
            write_to_buffer(d,
               "Please pick a Non-Clan-Deity (help deity for more information):\n\r",0);
            write_to_buffer(d,"Your choice?\n\r",0);
            return;
         }
      }
      
      ch->pcdata->deity = deity;
      ch->pcdata->new_deity = deity; 
      ch->pcdata->sac = 0;
   }
   break;
	case CON_PICK_STATS:
	case CON_PICK_STATS_DEFAULT:
    one_argument(argument,arg);
    write_to_buffer( d, "\n\r", 2 );
    if (!strcmp(arg,"help"))
    {
    	do_help(ch,"pickstats");
	creation_message(d, FALSE);
    	write_to_buffer(d,"Your choice -> ",0);
    }
    if ( !str_cmp(argument,"str") ) {
      ch->pcdata->pref_stat = STAT_STR; break;
    }
    if ( !str_cmp(argument,"dex") ) {
      ch->pcdata->pref_stat = STAT_DEX; break;      
    }
    if ( !str_cmp(argument,"int") ) {
      ch->pcdata->pref_stat = STAT_INT; break;      
    }
    if ( !str_cmp(argument,"wis") ) {
      ch->pcdata->pref_stat = STAT_WIS; break;
    }
    if ( !str_cmp(argument,"con") ) {
      ch->pcdata->pref_stat = STAT_CON; break;
    }
    if ( !str_cmp(argument,"cha") ) {
      ch->pcdata->pref_stat = STAT_SOC; break;
    }
    write_to_buffer(d, "That is not an attribute. How do you identify yourself?\n\r", 0);
    return;
  }// END SWITCH
  /* Generic next step - return from switch instead of break if this isn't appropriate */
  d->connected = creation_step(d, TRUE, FALSE);
  if(is_creation(d))
    creation_message(d, TRUE);
}

void refund_skill(CHAR_DATA *ch, char *skill)
{
  char buf[256];
  int sn = skill_lookup(skill);
  if(sn < 0)
  {
    sprintf(buf, "Skill not found to refund: %s", skill);
    bug(skill, 0);
    return;
  }

  if(ch->pcdata->learned[sn] > 0)
  {
    int refund = 0, prac_amount;
    if(ch->pcdata->learned[sn] > 75)
    {/* 1 practice per 10% */
      refund += (ch->pcdata->learned[sn] - 65) / 10;
      ch->pcdata->learned[sn] = 75;
    }
    prac_amount = UMAX(1, abs(skill_table[sn].rating[ch->class]));
    prac_amount = UMAX(1, int_app[get_max_train(ch,STAT_INT)].learn / prac_amount);
    refund += (ch->pcdata->learned[sn] + prac_amount - 1) / prac_amount;
    sprintf(buf, "You are refunded %d pracs for the removal of %s.\n\r", refund, skill);
    send_to_char(buf, ch);
    ch->practice += refund;
    ch->pcdata->learned[sn] = 0;
  }
}

void do_version_up(CHAR_DATA *ch)
{
  char buf[256];
  int i;
  if(ch->version < 32)
  {
    if(ch->perm_stat[STAT_END] != pc_race_table[ch->race].stats[STAT_END])
    {
  	int refund = ch->perm_stat[STAT_END] - pc_race_table[ch->race].stats[STAT_END];
  	if(refund > 0)
  	{
	  sprintf(buf, "$N is removing endurance. Old value: %d, refund: %d", ch->perm_stat[STAT_END], refund);
	  wiznet(buf,ch,NULL,WIZ_NOTES,WIZ_SECURE,get_trust(ch));
  	  ch->train += refund;
  	  sprintf(buf, "\n\r{W*** Endurance has been removed. You are refunded %d train%s. ***{x", refund, refund == 1 ? "" : "s");
	  if(ch->perm_stat[STAT_AGT] == pc_race_table[ch->race].stats[STAT_AGT])
	     strcat(buf, "\n\r");// Get nice spacing
  	  send_to_char(buf, ch);
  	}
  	ch->perm_stat[STAT_END] = pc_race_table[ch->race].stats[STAT_END];
    }

    if(ch->perm_stat[STAT_AGT] != pc_race_table[ch->race].stats[STAT_AGT])
    {
  	int refund = ch->perm_stat[STAT_AGT] - pc_race_table[ch->race].stats[STAT_AGT];
  	if(refund > 0)
  	{
	  sprintf(buf, "$N is removing agility. Old value: %d, refund: %d", ch->perm_stat[STAT_AGT], refund);
	  wiznet(buf,ch,NULL,WIZ_NOTES,WIZ_SECURE,get_trust(ch));
  	  ch->train += refund;
  	  sprintf(buf, "\n\r{W*** Agility has been removed. You are refunded %d train%s. ***{x\n\r\n\r", refund, refund == 1 ? "" : "s");
  	  send_to_char(buf, ch);
  	}
        ch->perm_stat[STAT_AGT] = pc_race_table[ch->race].stats[STAT_AGT];
    }
  }/* End version 32 */
  if(ch->version < 33)
  {
    /* Remove cone of silence and snatch, and swap slice for bump on thieves */
    if(ch->class == class_lookup("thief"))
    {
      int sn = skill_lookup("slice");
      i = skill_lookup("bump");
      if(sn >= 0 && i >= 0 && ch->pcdata->learned[sn] > 0)
      {
        send_to_char("Slice has been replaced with Bump.\n\r", ch);
        ch->pcdata->learned[i] = ch->pcdata->learned[sn];
        ch->pcdata->learned[sn] = 0;
      }
    }
    refund_skill(ch, "snatch");
    refund_skill(ch, "cone of silence");
    if(ch->pcdata->clan_info && ch->pcdata->clan_info->award_merit > 0)
    {
      int previous = ch->pcdata->clan_info->award_merit;
      ch->pcdata->clan_info->award_merit = calculate_bonus_merit(ch, FALSE);
      if(ch->pcdata->clan_info->award_merit > previous &&
        ch->pcdata->clan_info->award_merit > 100)
      {
        sprintf(buf, "Your starter tribute has been increased to %d from %d.\n\r", ch->pcdata->clan_info->award_merit / 100, previous / 100);
        send_to_char(buf, ch);
      }
      if(!ch->pcdata->clan_info->clan->default_clan)
        calculate_award_tribute(ch->pcdata->clan_info->clan);
    }
    if(IS_SET(ch->act, PLR_DENY))
    {
      sprintf(buf, "Removing deny on %s", ch->name);
      log_string(buf);
      REMOVE_BIT(ch->act, PLR_DENY);
    }
    REMOVE_BIT(ch->act, PLR_FREEZE);
  }
}
/*
 * Deal with sockets that haven't logged in yet.
 */
void nanny( DESCRIPTOR_DATA *d, char *argument )
{
    DESCRIPTOR_DATA *d_old, *d_next;
    char buf[MAX_STRING_LENGTH];
    char arg[MAX_INPUT_LENGTH];
    CHAR_DATA *ch;
    char *pwdnew;
    char *p;
    int race, i;
    bool fOld;
    OBJ_DATA *obj;
    OBJ_DATA *obj2;
    OBJ_DATA *obj_next;
    OBJ_DATA *obj_next2;

    while ( isspace(*argument) )
  argument++;

    ch = d->character;

    if(is_creation(d))
    {// Intercept, the creation steps don't go in the main loop any more
        creation_input(d, argument);
	if(is_creation(d))
	        return;// Catch the end of creation and go back
    }

    switch ( d->connected )
    {

    default:
  bug( "Nanny: bad d->connected %d.", d->connected );
  close_socket( d );
  return;
    case CON_GET_NAME:
  if ( argument[0] == '\0' )
  {
      close_socket( d );
      return;
  }


  argument[0] = UPPER(argument[0]);
  if (!check_parse_name (argument)) {
    write_to_buffer ( d, "Illegal name, try another.\n\rName: ", 0 );
    return;
  }

  fOld = load_char_obj ( d, argument );

  if ( !check_mob_name( argument, fOld ) && !fOld )
  {
      write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
      return;
  }

  ch   = d->character;

/*  argument[0] = UPPER(argument[0]);
  fOld = load_char_obj ( d, argument );

  if ( !fOld && !check_parse_name( argument ) )
  {
      write_to_buffer( d, "Illegal name, try another.\n\rName: ", 0 );
      return;
  }

  ch   = d->character;
*/
  if (IS_SET(ch->act, PLR_DENY) && (ch->version > 32 || IS_IMMORTAL(ch)))
  {
      sprintf( log_buf, "Denying access to %s@%s.", argument, d->host );
      log_string( log_buf );
      write_to_buffer( d, "You are denied access.\n\r", 0 );
      close_socket( d );
      return;
  }
//COREY BAN CHEKC GOES HERE
  if (check_ban(d->host,BAN_PERMIT) && !IS_SET(ch->act,PLR_PERMIT))
  {
      sprintf( log_buf, "Permit ban, denied %s@%s.", argument, d->host );
      log_string( log_buf );
      write_to_buffer(d,"Your site has been banned from this Sled.\n\r",0);
      close_socket(d);
      return;
  }

  if ( check_reconnect( d, argument, FALSE ) )
  {
      fOld = TRUE;
  }
  else
  {
#ifdef OLC_VERSION
     if ( !IS_SET(ch->act, PLR_FREEZE) && !IS_SET(ch->mhs,MHS_PREFRESHED) && !IS_IMMORTAL(ch)) 
#else /*game version*/
      if ( wizlock && !IS_IMMORTAL(ch)) 
      /* if ( wizlock || (IS_IMMORTAL(ch) && !IS_SET(ch->act,PLR_PERMIT))) */
#endif
      {
    write_to_buffer( d, "The game is wizlocked.\n\r", 0 );
    close_socket( d );
    return;
      }
  }

  if ( fOld )
  {
      /* Old player */

	/* not sure if you want them left in or not, Rusty */

      if(ch->pcdata->hostmask[0] != '\0')
      ch->desc->host = str_dup(ch->pcdata->hostmask);
      write_to_buffer( d, "Password: ", 0 );
      write_to_buffer( d, echo_off_str, 0 );
      d->connected = CON_GET_OLD_PASSWORD;
      return;
  }
  else
  {
      /* New player */
      if (newlock)
      {
    write_to_buffer( d, "The game is newlocked.\n\r", 0 );
    close_socket( d );
    return;
      }

      if (check_ban(d->host,BAN_NEWBIES))
      {
    write_to_buffer(d,
        "New players are not allowed from your site.\n\r",0);
    close_socket(d);
    return;
      }

 write_to_buffer(d,
"Note that some names are unacceptible.  We are fairly lenient on names\n\r",0);
 write_to_buffer(d,
"but nevertheless, some people manage to think of things that even the\n\r",0);
 write_to_buffer(d,
"hardy MHS Imm staff cannot stomach.  You'll be made to delete if you\n\r",0);
 write_to_buffer(d,
"manage to come up with something that falls into that category.\n\r",0);
      sprintf( buf, "Are you sure that %s is reasonable? (Y/N) ",argument );
      write_to_buffer( d, buf, 0 );
      d->connected = CON_CONFIRM_NEW_NAME;
      return;
  }
  break;

    case CON_GET_OLD_PASSWORD:
#if defined(unix)
  write_to_buffer( d, "\n\r", 2 );
#endif

  if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ))
  /*&&  strcmp( crypt(argument,"AltJOjLwtP8NE"),"AlHVvwOVMBOs6"))*/
  {
      write_to_buffer( d, "Wrong password.\n\r", 0 );

      if (d->character->pet) {
          CHAR_DATA *pet=d->character->pet;

          char_to_room(pet,get_room_index( ROOM_VNUM_LIMBO));
          stop_follower(pet);
          extract_char(pet,TRUE);
      }
      close_socket( d );
      return;
  }
 
  write_to_buffer( d, echo_on_str, 0 );

  if (check_playing(d,ch->name))
      return;

  if ( check_reconnect( d, ch->name, TRUE ) )
      return;

  sprintf( log_buf, "%s@%s has connected.", ch->name, d->host );
  log_string( log_buf );
  wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));

  if (ch->race == race_lookup("smurf"))
  {
          close_socket( d );
      return;
  }


  if ( ch->kit== kit_lookup("buffy") )
  {
    for ( i = 0 ; i < 5 ; i++ )
      if ( kit_table[ch->kit].skills[i] == NULL )
         break;
      else
         group_remove(ch,kit_table[ch->kit].skills[i]);

       sprintf(log_buf,"Taking buffy kit from %s and giving them 100 pracs .",ch->name);
       log_string(log_buf);

    ch->kit = 0;
    ch->practice += 100;

  }

/* BEGIN THE REMOVAL OF CLAN AVARICE */
/*if (ch->clan == clan_lookup("avarice") )
{ 
  ch->clan = clan_lookup("loner");;
  ch->pcdata->rank = 0;
}*/
/*remove shapeshifterkit*/
if ( ch->kit == kit_lookup("shapeshifter") )
{
  ch->practice += 100;
  ch->kit = 0;
}

/*BEGIN THE REMOVAL OF RESEARCH SKILL*/
if ( ch->pcdata->learned[skill_lookup("research")] > 1 )
{
  ch->pcdata->learned[skill_lookup("research")] = 0;
  ch->practice += 10;
}
if ( ch->pcdata->learned[skill_lookup("research")]  == 1 )
{
  ch->pcdata->learned[skill_lookup("research")] = 0;
}




/*BEGIN THE REMOVAL OF CLAN SKILLS */
/*if (ch->clan == clan_lookup("honor")
&&  ch->pcdata->learned[skill_lookup("honor guard")] > 0 )
{
  ch->pcdata->learned[skill_lookup("honor guard")] = 0;
}
if (ch->clan == clan_lookup("demise")
&&  ch->pcdata->learned[skill_lookup("confusion")] > 0 )
{
  ch->pcdata->learned[skill_lookup("confusion")] = 0;
}

if (ch->clan == clan_lookup("demise")
&& ch->pcdata->learned[skill_lookup("aura of cthon")] > 0)
{
  ch->pcdata->learned[skill_lookup("aura of cthon")] = 0;
}

if (ch->clan == clan_lookup("posse")
&& ch->pcdata->learned[skill_lookup("cuffs of justice")] > 0)
{
  ch->pcdata->learned[skill_lookup("cuffs of justice")] = 0;
}*/

/*END THE REMOVAL OF CLAN SKILLS*/

/* COREY put the kaethan check here*/
/*#ifdef COREY_VERSION*/
if( !IS_SET(ch->mhs,MHS_KAETH_CLEAN) && !IS_IMMORTAL(ch) )
{ /*BEGIN KAETH CHECK*/
  /* Remove all EQ Worn just for good measure */
  remove_all_objs(ch, TRUE);

  /* Remove all ITEM_IMM_LOAD and empty out the container
  if its a container */

/*having it loop thru a few more times to get containers within containers*/
  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
    obj_next = obj->next_content;

    if(obj->item_type == ITEM_CONTAINER)
    {
      for ( obj2 = obj->contains; obj2 != NULL; obj2 = obj_next2 )
      {
        obj_next2 = obj2->next_content;
        /*get_obj( ch, obj2, obj );*/

	/*putting this in*/
    if(
         obj2->pIndexData->vnum == 12046
      || obj2->pIndexData->vnum == 12047
      || obj2->pIndexData->vnum == 12048
      || obj2->pIndexData->vnum == 12049
      || obj2->pIndexData->vnum == 12050
      || obj2->pIndexData->vnum == 12051
      || obj2->pIndexData->vnum == 12055
      || obj2->pIndexData->vnum == 12056
      || obj2->pIndexData->vnum == 12057
      || obj2->pIndexData->vnum == 12058

      )
      {
       log_buf[0]='\0';
       sprintf(log_buf,"Taking %d from %s .",obj2->pIndexData->vnum,ch->name);
       log_string(log_buf);
       extract_obj(obj2);
//       obj_to_char(create_object(get_obj_index(OBJ_VNUM_SHARD),0,FALSE),ch);
//       obj_to_char(create_object(get_obj_index(OBJ_VNUM_SHARD),0,FALSE),ch);
      }
	/*taking this out*/
      }
    }
  }

  for ( obj = ch->carrying; obj != NULL; obj = obj_next )
  {
    obj_next = obj->next_content;
    if(
         obj->pIndexData->vnum == 12046
      || obj->pIndexData->vnum == 12047
      || obj->pIndexData->vnum == 12048
      || obj->pIndexData->vnum == 12049
      || obj->pIndexData->vnum == 12050
      || obj->pIndexData->vnum == 12051
      || obj->pIndexData->vnum == 12055
      || obj->pIndexData->vnum == 12056
      || obj->pIndexData->vnum == 12057
      || obj->pIndexData->vnum == 12058 

      )
      {
       log_buf[0]='\0';
       sprintf(log_buf,"Taking %d from %s and giving them two shards.",obj->pIndexData->vnum,ch->name);
       log_string(log_buf);
       extract_obj(obj);
       obj_to_char(create_object(get_obj_index(OBJ_VNUM_SHARD),0,FALSE),ch);
       obj_to_char(create_object(get_obj_index(OBJ_VNUM_SHARD),0,FALSE),ch);
      }
  }


   SET_BIT(ch->mhs,MHS_KAETH_CLEAN);
}/*end of KAETH CHECK */
/*#endif*/
/* Poquah temp remove come back to this later */


/*COREY COREY COREY put the remove matook stuff here */
/*

  if (is_clan(ch))
  {
  if(ch->pcdata->logins_without_death > 10 ||
     ch->pcdata->logins_without_kill > 10 ||
     ch->pcdata->logins_without_combat > 10)
  {
     sprintf( log_buf, "%s is a possible storage character.", ch->name);
     log_string( log_buf );
     wiznet(log_buf,NULL,NULL,WIZ_SITES,0,get_trust(ch));
  }
  }
*/

/*  get_ident_info ( ch ); */

      if( IS_SET(ch->mhs,MHS_PREFRESHED) )
      {
         do_help( ch, "pfreshmotd" );
         write_to_buffer(d,"\n\r",0);
         write_to_buffer(d,"'Continue' or 'Quit' Which would you like to do? ",0);
         d->connected = CON_PREFRESH_CHAR;
         break;
      }

      if( IS_SET(ch->act,PLR_RECLASS) )
      {
/*	REMOVE_BIT(ch->act,PLR_RECLASS);
	write_to_buffer( d, "\n\r", 2 );
	write_to_buffer( d, "You may be good, neutral, or evil.\n\r",0);
	write_to_buffer( d, "Which alignment (G/N/E)? ",0);*/
	/* perm stat based on old class */
/*	ch->perm_stat[class_table[ch->pcdata->old_class].attr_prime] += 3;
  	ch->perm_stat[class_table[ch->pcdata->old_class].attr_second] += 2;

	d->connected = CON_GET_ALIGNMENT;*/
	ch->gen_data = new_gen_data();
	ch->gen_data->bonus_points = 50;

	write_to_buffer(d, "Beginning reclass.\n\r", 0);
	d->connected = CON_DEFAULT_CHOICE;
	creation_message(d, TRUE);
	break;
      }

  if ( IS_IMMORTAL(ch) )
  {
      do_help( ch, "imotd" );
      d->connected = CON_READ_IMOTD;
  }
  else
  {
      do_help( ch, "motd" );
      d->connected = CON_READ_MOTD;
  }
  break;

/* RT code for breaking link */
 
    case CON_BREAK_CONNECT:
  switch( *argument )
  {
  case 'y' : case 'Y': default:
      for ( d_old = descriptor_list; d_old != NULL; d_old = d_next )
      {
    d_next = d_old->next;
    if (d_old == d || d_old->character == NULL)
        continue;

    if (str_cmp(ch->name,d_old->original ?
        d_old->original->name : d_old->character->name))
        continue;

    close_socket(d_old);
      }
      if (check_reconnect(d,ch->name,TRUE))
    return;
      write_to_buffer(d,"Reconnect attempt failed.\n\rName: ",0);
      if ( d->character != NULL )
      {
    free_char( d->character );
    d->character = NULL;
      }
      d->connected = CON_GET_NAME;
      break;
    }
/*
  case 'n' : case 'N':
      write_to_buffer(d,"Name: ",0);
      if ( d->character != NULL )
      {
    free_char( d->character );
    d->character = NULL;
      }
      d->connected = CON_GET_NAME;
      break;

  default:
      write_to_buffer(d,"Please type Y or N? ",0);
      break;
  }*/
  break;

    case CON_CONFIRM_NEW_NAME:
  switch ( *argument )
  {
  case 'y': case 'Y':
      sprintf( buf, "Please choose a password: %s", echo_off_str );
      write_to_buffer( d, buf, 0 );
      d->connected = CON_GET_NEW_PASSWORD;
      break;

  case 'n': case 'N':
      write_to_buffer( d, "Ok, what IS it, then? ", 0 );
      free_char( d->character );
      d->character = NULL;
      d->connected = CON_GET_NAME;
      break;

  default:
      write_to_buffer( d, "Please type Yes or No? ", 0 );
      break;
  }
  break;

    case CON_PREFRESH_CHAR:
  one_argument(argument,arg);

  if (!strcmp(arg,"quit"))
  {
      write_to_buffer( d, "Character Not Refreshed.n\r", 0 );
      close_socket( d );
      return;
  }
  else
  if (!strcmp(arg,"continue"))
  {
     /* Move all skills to old skills and reset skills */
     for ( i = 0; i < MAX_SKILL; i++ )
     {
        ch->pcdata->old_learned[i] = ch->pcdata->learned[i];
        ch->pcdata->learned[i] = 0;
     }

     /* Clear Groups Known so they can pick them again */
     for (i = 0; i < MAX_GROUP; i++)
     {
        if (group_table[i].name == NULL)
           break;
        ch->pcdata->group_known[i] = 0;
     }

     /* Set the Logout tracker */
     ch->pcdata->logout_tracker = 6;

     /* Reset Skill Points */
     ch->skill_points = 0;
     ch->pcdata->skill_point_timer = 0;
     ch->pcdata->skill_point_tracker = 0;

     /* Remove Killer,Thief and Trumps */
     if (IS_SET(ch->act,PLR_KILLER))
        REMOVE_BIT(ch->act,PLR_KILLER); 
     if (IS_SET(ch->act,PLR_THIEF))
        REMOVE_BIT(ch->act,PLR_THIEF);
     if (IS_SET(ch->wiznet,PLR_RUFFIAN))
        REMOVE_BIT(ch->wiznet,PLR_RUFFIAN);
     ch->trumps = 0;

     /* Reset Fight, Outcast and Ruffian Timers */
     ch->timer = 0;
     ch->pcdata->outcT = 0;
     ch->pcdata->ruffT = 0;

     /* Reset Clan and Rank */
     if (is_clan(ch))
     {
        ch->clan = 0;
        ch->pcdata->rank = 0;
     }

     /* calculate gained xp for refresh purposes */
     if( IS_SET(ch->act, PLR_VAMP) || 
         IS_SET(ch->act, PLR_WERE ) ||
         IS_SET(ch->act, PLR_MUMMY ) )
     {
         ch->exp += 26 * exp_per_level(ch,ch->pcdata->points);
     }

     /* set pcdata points to zero */
     ch->pcdata->points = 0;

     /* Remove Kit */
     /* Remove specialization if any */
     ch->pcdata->specialize = 0;
     ch->species_enemy = 0;
     ch->kit = 0;

     /* Remove Nodes if Any */
     if (ch->pcdata->node != 0)
        ch->pcdata->node = 0;

     /* Remove Shapeshifted or Shapemorphed just in case */
     if (IS_SET(ch->mhs,MHS_SHAPESHIFTED))
        REMOVE_BIT(ch->mhs,MHS_SHAPESHIFTED);
     if (IS_SET(ch->mhs,MHS_SHAPEMORPHED))
        REMOVE_BIT(ch->mhs,MHS_SHAPEMORPHED);
     if (IS_SET(ch->mhs,MHS_MUTANT))
        REMOVE_BIT(ch->mhs,MHS_MUTANT);

     /* Remove Affects and Resists/Vulns/Imms */
     while ( ch->flash_affected )
        flash_affect_remove( ch, ch->flash_affected,APPLY_BOTH );
     while ( ch->affected )
        affect_remove( ch, ch->affected,APPLY_BOTH );
     ch->affected_by = 0;

    if (ch->imm_flags)
       ch->imm_flags = 0; 
    if (ch->res_flags)
       ch->res_flags = 0;
    if (ch->vuln_flags)
       ch->vuln_flags = 0;

    /* Remove all EQ Worn just for good measure */
    remove_all_objs(ch, TRUE);

    /* Remove all ITEM_IMM_LOAD and empty out the container
       if its a container */
   
    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
       obj_next = obj->next_content;
       if(obj->item_type == ITEM_CONTAINER)
       {
          for ( obj2 = obj->contains; obj2 != NULL; obj2 = obj_next2 )
          {
             obj_next2 = obj2->next_content;
             get_obj( ch, obj2, obj );
          }
       }
    }

    for ( obj = ch->carrying; obj != NULL; obj = obj_next )
    {
       obj_next = obj->next_content;
       if (IS_SET(obj->extra_flags,ITEM_IMM_LOAD))
          extract_obj(obj);
    }

    /* Clear Last Attacked By */
    ch->pcdata->last_attacked_by = str_dup("no one");
    ch->pcdata->last_attacked_by_timer = 0;
    ch->pcdata->last_death_timer = 0;
    ch->pcdata->logins_without_combat = 0;
    ch->pcdata->logins_without_death = 0;
    ch->pcdata->logins_without_kill = 0;
    ch->pcdata->last_combat_date = current_time;
    ch->pcdata->last_death_date = current_time;
    ch->pcdata->last_kill_date = current_time;

    /* Reset Report Stats */
    ch->pcdata->killer_data[PC_DEATHS] = 0;
    ch->pcdata->killer_data[PC_LOWER_KILLS] = 0;
    ch->pcdata->killer_data[PC_EQUAL_KILLS] = 0;
    ch->pcdata->killer_data[PC_GREATER_KILLS] = 0;
    ch->pcdata->steal_data[PC_STOLEN_ITEMS] = 0;
    ch->pcdata->steal_data[PC_STOLEN_GOLD] = 0;
    ch->pcdata->steal_data[PC_SLICES] = 0;
    ch->pcdata->last_kill = str_dup("no one");
    ch->pcdata->last_killed_by = str_dup("no one");

    /* Clear Trains and Pracs */
    ch->train = 0;
    ch->practice = 0;

    write_to_buffer(d,"The following races are available:\n\r  ",0);
    for ( race = 1; race_table[race].name != NULL; race++ )
    {
       if (!race_table[race].pc_race)
          break;
       write_to_buffer(d," * ",0);
       write_to_buffer(d,race_table[race].name,0);
       write_to_buffer(d,"\n\r",0);
       write_to_buffer(d," ",1);
    }

    write_to_buffer(d,"\n\r",0);
    write_to_buffer(d,"What is your race (help for more information)? ",0);
    d->connected = CON_GET_NEW_RACE;
    break;
 }
 else
 {
    write_to_buffer(d,"\n\r",0);
    write_to_buffer(d,"'Continue' or 'Quit' Which would you like to do? ",0);
    break;
 }
 break;

    case CON_GET_NEW_PASSWORD:
#if defined(unix)
  write_to_buffer( d, "\n\r", 2 );
#endif

  if ( strlen(argument) < 5 )
  {
      write_to_buffer( d,
    "Password must be at least five characters long.\n\rPassword: ",
    0 );
      return;
  }

  pwdnew = crypt( argument, ch->name );
  for ( p = pwdnew; *p != '\0'; p++ )
  {
      if ( *p == '~' )
      {
    write_to_buffer( d,
        "New password not acceptable, try again.\n\rPassword: ",
        0 );
    return;
      }
  }

  free_string( ch->pcdata->pwd );
  ch->pcdata->pwd = str_dup( pwdnew );
  write_to_buffer( d, "Please retype password: ", 0 );
  d->connected = CON_CONFIRM_NEW_PASSWORD;
  break;

    case CON_CONFIRM_NEW_PASSWORD:
#if defined(unix)
  write_to_buffer( d, "\n\r", 2 );
#endif

  if ( strcmp( crypt( argument, ch->pcdata->pwd ), ch->pcdata->pwd ) )
  {
      write_to_buffer( d, "Passwords don't match.\n\rRetype password: ",
    0 );
      d->connected = CON_GET_NEW_PASSWORD;
      return;
  }

  ch->gen_data = new_gen_data();// Do this once here
  ch->gen_data->bonus_points = 50;

  write_to_buffer( d, echo_on_str, 0 );
  d->connected = CON_GET_SURNAME;
  creation_message(d, TRUE);
  break;

/*    case CON_TEMP_SMURF_PASSWORD:
  one_argument(argument,arg);

  if (!strcmp(arg,"simpy"))
  {
     race = ch->race;
     for (i = 0; i < MAX_STATS; i++)
         ch->perm_stat[i] = pc_race_table[race].stats[i];
     ch->affected_by = ch->affected_by|race_table[race].aff;
     ch->imm_flags   = ch->imm_flags|race_table[race].imm;
     ch->res_flags   = ch->res_flags|race_table[race].res;
     ch->vuln_flags  = ch->vuln_flags|race_table[race].vuln;
     ch->form        = race_table[race].form;
     ch->parts       = race_table[race].parts;
     for (i = 0; i < 5; i++)
     {
         if (pc_race_table[race].skills[i] == NULL)
            break;
         group_add(ch,pc_race_table[race].skills[i],FALSE);
     }
     ch->pcdata->points = pc_race_table[race].points;
     ch->size = pc_race_table[race].size;
     ch->pcdata->created_date = current_time;
     ch->clan = clan_lookup("smurf");
     ch->pcdata->surname = str_dup("Smurf"); 
     ch->pcdata->rank = 0;
     write_to_buffer( d, "What is your sex (M/F)? ", 0 );
     d->connected = CON_GET_NEW_SEX;
  }
  else
  {
     write_to_buffer(d,"That password is not correct.\n\r  ",0);
     write_to_buffer(d,"The following races are available:\n\r  ",0);
     for ( race = 1; race_table[race].name != NULL; race++ )
     {
         if (!race_table[race].pc_race)
            break;
         write_to_buffer(d," * ",0);
         write_to_buffer(d,race_table[race].name,0);
         write_to_buffer(d,"\n\r",0);
         write_to_buffer(d," ",1);
     }

     write_to_buffer(d,"\n\r",0);
     write_to_buffer(d,"What is your race (help for more information)? ",0);
     d->connected = CON_GET_NEW_RACE;
  }
  break;*/

    case CON_READ_IMOTD:
  write_to_buffer(d,"\n\r",2);
  do_help( ch, "motd" );
  d->connected = CON_READ_MOTD;
  break;

    case CON_READ_MOTD:
  if ( ch->pcdata == NULL || ch->pcdata->pwd[0] == '\0')
  {
      write_to_buffer( d, "Warning! Null password!\n\r",0 );
      write_to_buffer( d, "Please report old password with bug.\n\r",0);
      write_to_buffer( d,
    "Type 'password null <new password>' to fix.\n\r",0);
  }

  write_to_buffer( d, 
    "\n\rWelcome to MooseHead Sled.  Please do not feed the mobiles.\n\r",
      0 );
  ch->next        = char_list;
  char_list       = ch;
  d->connected    = CON_PLAYING;
  reset_char(ch);
  ch->pcdata->clan_info = find_char_clan(ch->name);
  if (is_clan(ch) && !IS_IMMORTAL(ch) )
  {
     ch->pcdata->start_time = 3;
     if(ch->pcdata->clan_info && ch->pcdata->clan_info->clan->type == CLAN_TYPE_FAITH
       && ch->pcdata->deity != ch->pcdata->clan_info->clan->enemy)
     {
       ch->pcdata->deity = ch->pcdata->clan_info->clan->enemy;
       ch->pcdata->deity_timer = 0;
       sprintf(buf,"To follow your clan's declared faith, you are now a follower of %s.\n\r",deity_table[ch->pcdata->deity].pname);
       send_to_char(buf,ch);
     }
    set_clan_skills(ch);
  }

  if (ch->pcdata->last_level == 0 || ch->pcdata->last_level == NULL)
     ch->pcdata->last_level = 1; /* Fixes symbol bugs */
  if ( ch->level == 0 )
  {

      ch->level   = 1;
      ch->train    += 3;
      ch->practice = 5;

      if (IS_SET(ch->mhs,MHS_PREFRESHED))
      {
      /* Fill any last minute things in here */
         ch->hit     = 20;
         ch->max_hit = 20;
         ch->pcdata->perm_hit = 20;
         ch->mana    = 100;
         ch->max_mana= 100;
         ch->pcdata->perm_mana= 100;
         ch->move    = 100;
         ch->max_move= 100;
         ch->pcdata->perm_move= 100;
         do_outfit(ch,"");

         REMOVE_BIT(ch->mhs,MHS_PREFRESHED);
      }
      else
      {
         ch->hit     = ch->max_hit;
         ch->mana    = ch->max_mana;
         ch->move    = ch->max_move;
         ch->exp     = exp_per_level(ch,ch->pcdata->points);
         set_title( ch, "the confused" );

         ch->gold = number_range( 20,40 );
         ch->silver = number_range( 300,600 );
       sprintf (buf,"SMURF: %s outfit",ch->name);
       log_string(buf); 
         do_outfit(ch,"");
       sprintf (buf,"SMURF: %s after outfit",ch->name);
       log_string(buf); 
         obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP),0,FALSE),ch);
         obj_to_char(create_object(get_obj_index(OBJ_VNUM_MAP_BOINGA),0,FALSE),ch);
      }

#ifdef OLC_VERSION
      char_to_room( ch, get_room_index( ROOM_VNUM_DEAD ) );
#else /*game version*/
      char_to_room( ch, get_room_index( ROOM_VNUM_SCHOOL ) );
#endif

      send_to_char("\n\r",ch);
      do_help(ch,"NEWBIE INFO");
      send_to_char("\n\r",ch);
  }
  else if ( ch->in_room != NULL )
  {
      char_to_room( ch, ch->in_room );
  }
  else if ( IS_IMMORTAL(ch) )
  {
      char_to_room( ch, get_room_index( ROOM_VNUM_CHAT ) );
  }
  else
  {
      char_to_room( ch, get_room_index( ROOM_VNUM_TEMPLE ) );
  }
  if(ch->in_room->vnum < 0 && ch->pcdata->clan_info && !ch->pcdata->clan_info->clan->default_clan &&
    (ch->pcdata->clan_info->clan->vnum_min < abs(ch->in_room->vnum) ||
    ch->pcdata->clan_info->clan->vnum_max > abs(ch->in_room->vnum)))
    {/* Send them elsewhere, this is not their hall */
      char_from_room(ch);
      if(!ch->pcdata->clan_info->clan->hall || !ch->pcdata->clan_info->clan->hall->to_place)
        char_to_room(ch,get_room_index(ROOM_VNUM_MATOOK));
      else
        char_to_room(ch, (ROOM_INDEX_DATA*)ch->pcdata->clan_info->clan->hall->to_place);
    }

  act( "$n has entered the game.", ch, NULL, NULL, TO_ROOM ,FALSE);

  do_look( ch, "auto" );

  wiznet("$N has left real life behind.",ch,NULL,
    WIZ_LOGINS,WIZ_SITES,get_trust(ch));
    if( (!IS_IMMORTAL(ch) || (ch->incog_level == 0 && ch->invis_level == 0))
	&& ch->desc != NULL )
      {
  //pnet("$N has entered Boinga.",ch,NULL,PNET_LOGINS,NULL,get_trust(ch));
  pnet("$N has entered Boinga.",ch,NULL,PNET_LOGINS,NULL,IS_IMMORTAL(ch) ? get_trust(ch) : 1);
      }
  do_version_up(ch);


/* All Highlanders(with certain kills) feel a Highlander enter */
if (IS_SET(ch->mhs,MHS_HIGHLANDER))
{
   sprintf(buf, "You feel the presence of a Highlander entering Boinga.\n\r");

   for ( d = descriptor_list; d != NULL; d = d->next )
   {
       CHAR_DATA *victm;

       victm = d->original ? d->original : d->character;

       if ( d->connected == CON_PLAYING &&
            d->character != ch &&
            IS_SET(victm->mhs,MHS_HIGHLANDER) &&
	    (victm->pcdata->highlander_data[ALL_KILLS] >= 6))
          send_to_char(buf, victm);
   }
}

  count_clanners();

  ch->logon	= current_time;

  if (ch->pet != NULL)
  {
      char_to_room(ch->pet,ch->in_room);
      act("$n has entered the game.",ch->pet,NULL,NULL,TO_ROOM,FALSE);
  }
#ifdef GAME_VERSION
  do_unread(ch,"");
  do_count(ch,"");
#endif
  if(!IS_NPC(ch) && ch->pcdata->start_time > 0)
  {
    sprintf(buf, "\n\r{WYou may not attack other players for %d ticks.{x\n\r", ch->pcdata->start_time);
    send_to_char(buf, ch);
  }

  break;
    }

    return;
}

bool check_parse_surname( char *name )
{
    int clan;

    /*
     * Reserved words.
     */
  if ( is_name( name, 
      "all auto immortal self someone something the you outcast loner on off") )
    return FALSE;
  
    /* check clans */
    for (clan = 0; clan < MAX_CLAN; clan++)
    {
    	if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
    	&&  !str_cmp(name,clan_table[clan].name))
    	   return FALSE;
    }

    if (str_cmp(capitalize(name),"Rusty") && (!str_prefix("rusty",name)
        || !str_suffix("rusty",name)))
    return FALSE;

    /*
     * Length restrictions.
     */
     
    if ( strlen(name) <  2 )
  return FALSE;

    if ( strlen(name) > 12 )
  return FALSE;

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
  char *pc;
  bool fIll,adjcaps = FALSE,cleancaps = FALSE;
  int total_caps = 0;

  fIll = TRUE;
  for ( pc = name; *pc != '\0'; pc++ )
  {
      if ( !isalpha(*pc) && *pc != 0x27 )
    return FALSE;

      if ( isupper(*pc)) /* ugly anti-caps hack */
      {
    if (adjcaps)
        cleancaps = TRUE;
    total_caps++;
    adjcaps = TRUE;
      }
      else
    adjcaps = FALSE;

      if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
    fIll = FALSE;
  }
  if ( fIll )
      return FALSE;

  if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
      return FALSE;
    }

    return TRUE;
}



/*
 * Parse a name for acceptability.
 */
bool check_parse_name( char *name )
{
    int clan,count;
    DESCRIPTOR_DATA *d,*dnext;

    /*
     * Reserved words.
     */
    if ( is_name( name, 
        "all auto immortal self someone something the you outcast loner on off") )
    return FALSE;
  
    /*
     * check names of people playing. Yes, this is necessary for multiple
     * newbies with the same name
     */
    if (descriptor_list) {
        count=0;
        for (d = descriptor_list; d != NULL; d = dnext) {
            dnext=d->next;
            if (d->connected!=CON_PLAYING
		&& d->character
		&& d->character->name
                && d->character->name[0] 
		&& !str_cmp(d->character->name,name)) {
                count++;
                close_socket(d);
            }
        }
        if (count) {
            sprintf(log_buf,"Double newbie alert (%s)",name);
            wiznet(log_buf,NULL,NULL,WIZ_LOGINS,0,0);

            return FALSE;
        }
    }

    if( !str_cmp(capitalize(name),"Matook") )
      return TRUE;

    /* check clans */
    for (clan = 0; clan < MAX_CLAN; clan++)
    {
    	if (LOWER(name[0]) == LOWER(clan_table[clan].name[0])
    	&&  !str_cmp(name,clan_table[clan].name))
    	   return FALSE;
    }

    if (str_cmp(capitalize(name),"Rusty") && (!str_prefix("rusty",name)
        || !str_suffix("rusty",name)))
    return FALSE;

    /*
     * Length restrictions.
     */
     
    if ( strlen(name) <  2 )
  return FALSE;

#if defined(MSDOS)
    if ( strlen(name) >  8 )
  return FALSE;
#endif

#if defined(macintosh) || defined(unix)
    if ( strlen(name) > 12 )
  return FALSE;
#endif

    /*
     * Alphanumerics only.
     * Lock out IllIll twits.
     */
    {
  char *pc;
  bool fIll,adjcaps = FALSE,cleancaps = FALSE;
  int total_caps = 0;

  fIll = TRUE;
  for ( pc = name; *pc != '\0'; pc++ )
  {
      if ( !isalpha(*pc) )
    return FALSE;

      if ( isupper(*pc)) /* ugly anti-caps hack */
      {
    if (adjcaps)
        cleancaps = TRUE;
    total_caps++;
    adjcaps = TRUE;
      }
      else
    adjcaps = FALSE;

      if ( LOWER(*pc) != 'i' && LOWER(*pc) != 'l' )
    fIll = FALSE;
  }
  if ( fIll )
      return FALSE;

  if (cleancaps || (total_caps > (strlen(name)) / 2 && strlen(name) < 3))
      return FALSE;
    }

    /*
     * Prevent players from naming themselves after mobs.
     *
    {
  extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
  MOB_INDEX_DATA *pMobIndex;
  int iHash;

  for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
  {
      for ( pMobIndex  = mob_index_hash[iHash];
      pMobIndex != NULL;
      pMobIndex  = pMobIndex->next )
      {
    if ( is_name( name, pMobIndex->player_name ) )
        return FALSE;
      }
  }
    }*/

    return TRUE;
}


/* in comm.c.. after check_parse_name */

bool check_mob_name (char *name,bool old_char)
{
  extern MOB_INDEX_DATA *mob_index_hash[MAX_KEY_HASH];
  MOB_INDEX_DATA *pMobIndex;
  int iHash;

  for ( iHash = 0; iHash < MAX_KEY_HASH; iHash++ )
  {
      for ( pMobIndex  = mob_index_hash[iHash];
      pMobIndex != NULL;
      pMobIndex  = pMobIndex->next )
      {
    if ( is_name( name, pMobIndex->player_name ) ) {
        if (old_char) {
          char buf[MAX_STRING_LENGTH];

          sprintf (buf,"Warning:  name conflict for name '%s'",
            name);
          log_string (buf);
        }
        return FALSE;
    }
      }
  }

    return TRUE;
} 


/*
 * Look for link-dead player to reconnect.
 */
bool check_reconnect( DESCRIPTOR_DATA *d, char *name, bool fConn )
{
    CHAR_DATA *ch;
    bool found=FALSE;

    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
  if ( !IS_NPC(ch)
  &&   (!fConn || ch->desc == NULL)
  &&   !str_cmp( d->character->name, ch->name ) )
  {
      if ( fConn == FALSE )
      {
    free_string( d->character->pcdata->pwd );
    d->character->pcdata->pwd = str_dup( ch->pcdata->pwd );
      }
      else
      {
    OBJ_DATA *obj;

   if (d->character->pet) {
      CHAR_DATA *pet=d->character->pet;

      char_to_room(pet,get_room_index( ROOM_VNUM_LIMBO));
      stop_follower(pet);
      extract_char(pet,TRUE);
    }
    free_char( d->character );
    d->character = ch;
    ch->desc         = d;
    ch->timer        = 0;
    ch->pcdata->interp_fun = NULL;
/*    get_ident_info ( ch ); */
    for ( ch = char_list; ch != NULL; ch = ch->next )
    {
      if( ch == d->character )
	{ 
	  found = TRUE;
	  break;
	}
    }
    if( !found )
    {
      sprintf( log_buf, "%s not found in char_list on reconnect",
		d->character->name );
      log_string( log_buf );
      d->character->next = char_list;
      char_list = d->character;
    }
    send_to_char(
        "Reconnecting. Type replay to see missed tells.\n\r", ch );
    act( "$n has reconnected.", ch, NULL, NULL, TO_ROOM ,FALSE);
    if ((obj = get_eq_char(ch,WEAR_LIGHT)) != NULL
    &&  obj->item_type == ITEM_LIGHT && obj->value[2] != 0)
        --ch->in_room->light;

    sprintf( log_buf, "%s@%s reconnected.", ch->name, d->host );
    log_string( log_buf );
    wiznet("$N groks the fullness of $S link.",
        ch,NULL,WIZ_LINKS,0,get_trust(ch));
   //pnet("$N groks the fullness of $S link.",ch,NULL,PNET_LINKS,0,get_trust(ch));
   if(!IS_IMMORTAL(ch))
     pnet("$N groks the fullness of $S link.",ch,NULL,PNET_LINKS,0,1);
    d->connected = CON_PLAYING;

      }
      return TRUE;
  }
    }

    return FALSE;
}



/*
 * Check if already playing.
 */
bool check_playing( DESCRIPTOR_DATA *d, char *name )
{
    DESCRIPTOR_DATA *dold;

    for ( dold = descriptor_list; dold; dold = dold->next )
    {
  if ( dold != d
  &&   dold->character != NULL
  &&   dold->connected != CON_GET_NAME
  &&   dold->connected != CON_GET_OLD_PASSWORD
  &&   !str_cmp( name, dold->original
     ? dold->original->name : dold->character->name ) )
  {
      write_to_buffer( d, "That character is already playing.\n\r",0);
	strcpy( d->incomm, "\n" );
      /*write_to_buffer( d, "Do you wish to connect anyway (Y/N)?",0);*/
	     for ( dold = descriptor_list; dold != NULL; dold = d_next )
      {
    d_next = dold->next;
    if (dold == d || dold->character == NULL)
        continue;

    if (str_cmp(d->character->name,dold->original ?
        dold->original->name : dold->character->name))
        continue;

    close_socket(dold);
      }
      if (check_reconnect(d,d->character->name,TRUE))
    return TRUE;

      d->connected = CON_PLAYING;
      return TRUE;
  }
    }

    return FALSE;
}



void stop_idling( CHAR_DATA *ch )
{
    if ( ch == NULL
    ||   ch->desc == NULL
    ||   ch->desc->connected != CON_PLAYING
    ||   ch->was_in_room == NULL 
    ||   ch->in_room != get_room_index(ROOM_VNUM_LIMBO))
  return;

    ch->timer = 0;
    char_from_room( ch );
    char_to_room( ch, ch->was_in_room );
    if ( is_mounted(ch) )
    {
	char_from_room( ch->riding );
	char_to_room( ch->riding, ch->was_in_room );
    }
    ch->was_in_room     = NULL;
    act( "$n has returned from the void.", ch, NULL, NULL, TO_ROOM ,FALSE);
    return;
}

bool can_use_points( CHAR_DATA *ch, int points )
{
    int i;

    if ( points <= 0 ) return FALSE;
    for ( i = 0 ; i < MAX_STATS ; i++ )
    {
	if(i == STAT_END || i == STAT_AGT)
	  continue;
       /* if ( improve_table[ch->perm_stat[i]].cost <= points */
        if ( calc_stat_cost(ch,i) <= points 
	  && get_max_train(ch,i) > ch->perm_stat[i] )
		return TRUE;
    }
    return FALSE;
}

int calc_stat_cost( CHAR_DATA *ch, int attr_type )
{

/*    *** new code
      go to the race table and compare stats to current stats
      the difference between the two cost is the cost to increase
      that stat , add one since cost is
	always one more than the difference. example:
	str is currently 10, original is 10. difference is 0
	cost is 1 to increase str to 11 .  another mod for
	primary and secondary attrs -3 and -2 respectively
*/

  int cost;
  cost = ch->perm_stat[attr_type] - pc_race_table[ch->race].stats[attr_type];
  cost +=1; /*  mod because of difference */

  /*  check to see if this attr type is a primary or secondary stat
      always check based on old class , since bonus is based on old class */
  if (class_table[ch->pcdata->old_class].attr_prime == attr_type)
  {
      /*  it is the primary stat, decrease cost by 3 */
      cost -=3;
  }
  if ( class_table[ch->pcdata->old_class].attr_second == attr_type)
  {
     /*  it is a secondary stat, decrease cost by 2 */
      cost -=2;
  }

  return cost;
}


void show_stats( DESCRIPTOR_DATA *d )
{
    CHAR_DATA *ch;
    int i,statCost;
    char buf[MAX_STRING_LENGTH];
    static char *attrib_name[] = {
	"Str", "Int", "Wis", "Dex", "Con", "Agt", "End", "Cha" };

    if ( ( ch = d->character ) == NULL )
    {
	bug("display_stats : NULL ch in d",0);
	return;
    }

    send_to_char("\n\r",ch);
    for ( i = 0 ; i < MAX_STATS ; i++ )
    {
    if(i == STAT_AGT || i == STAT_END)
	continue;
    statCost = calc_stat_cost(ch,i);
    sprintf(buf,"%s: Current: %2d, spend %d point%s to raise to %2d.\n\r",
        attrib_name[i],
	ch->perm_stat[i],
	/* improve_table[ch->perm_stat[i]].cost,
 	improve_table[ch->perm_stat[i]].cost == 1 ? "" : "s",
	*/
	statCost,
	statCost == 1 ? "" : "s",
	ch->perm_stat[i]+1 );
    send_to_char(buf,ch);
    }

    sprintf(buf,"You have %d points.  Improve which attribute? -> ",
	ch->gen_data->bonus_points );
    send_to_char(buf,ch);
}
/*
 * Write something to a given room
 * Hybred between stc and act, only works if awake
 */
void send_to_room( const char *txt, ROOM_INDEX_DATA *room )
{
    CHAR_DATA *ch;

    for ( ch = room->people ; ch!= NULL ; ch = ch->next_in_room )
    {
	if ( !IS_AWAKE(ch))
	    continue;

	send_to_char(txt,ch);
    }

    return;
}

/*
 * Write to one char.
 */
void send_to_char( const char *txt, CHAR_DATA *ch )
{
  
    if ( (txt != NULL) && (ch->desc != NULL) )
  write_to_buffer( ch->desc, txt, strlen(txt) );
    return;
}

/*
 * Send a page to one char.
 */
void page_to_char( const char *txt, CHAR_DATA *ch )
{
    if ( (txt == NULL) || (ch->desc == NULL) ) 
	return;

    if (ch->lines == 0 )
    {
  send_to_char(txt,ch);
  return;
    }
  
#if defined(macintosh)
  send_to_char(txt,ch);
#else
    if (ch->desc->showstr_head &&
       (strlen(txt)+strlen(ch->desc->showstr_head)+1) < 32000)
    {
#ifdef OLC_VERSION
      char *temp=alloc_mem(strlen(txt) + strlen(ch->desc->showstr_head) + 1);
#else
      char *temp=GC_MALLOC(strlen(txt) + strlen(ch->desc->showstr_head) + 1);
#endif
      strcpy(temp, ch->desc->showstr_head);
      strcat(temp, txt);
      ch->desc->showstr_point = temp +
       (ch->desc->showstr_point - ch->desc->showstr_head);
      free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head) + 1);
      ch->desc->showstr_head=temp;
    }
    else
    {
      if (ch->desc->showstr_head)
      free_mem(ch->desc->showstr_head, strlen(ch->desc->showstr_head)+1);
#ifdef OLC_VERSION
      ch->desc->showstr_head = alloc_mem(strlen(txt) + 1);
#else
      ch->desc->showstr_head = GC_MALLOC(strlen(txt) + 1);
#endif
      strcpy(ch->desc->showstr_head,txt);
      ch->desc->showstr_point = ch->desc->showstr_head;
      show_string(ch->desc,"");
    }
    return;
#endif
}


/* string pager */
void show_string(struct descriptor_data *d, char *input)
{
    char buffer[4*MAX_STRING_LENGTH];
    char buf[MAX_INPUT_LENGTH];
    register char *scan, *chk;
    int lines = 0, toggle = 1;
    int show_lines;

    one_argument(input,buf);
    if (buf[0] != '\0')
    {
	  if (d->showstr_head)
  	  {
      	   free_mem(d->showstr_head,strlen(d->showstr_head));
     	   d->showstr_head = 0;
  	  }
  	  d->showstr_point  = 0;
 	  return;
    }

    if (d->character)
	show_lines = d->character->lines;
    else
	show_lines = 0;

    for (scan = buffer; ; scan++, d->showstr_point++)
    {
	if (((*scan = *d->showstr_point) == '\n' || *scan == '\r')
	   && (toggle = -toggle) < 0)
	   lines++;

  else if (!*scan || (show_lines > 0 && lines >= show_lines))
  {
      *scan = '\0';
      write_to_buffer(d,buffer,strlen(buffer));
      for (chk = d->showstr_point; isspace(*chk); chk++);
      {
	if (!*chk)
	{
           if (d->showstr_head)
           {
		free_mem(d->showstr_head,strlen(d->showstr_head));
		d->showstr_head = 0;
        }
        d->showstr_point  = 0;
    }
      }
      return;
  }
    }
    return;
}
  

/* quick sex fixer */
void fix_sex(CHAR_DATA *ch)
{
    if (ch->sex < 0 || ch->sex > 2)
  ch->sex = IS_NPC(ch) ? 0 : ch->pcdata->true_sex;
}

void act(const char *format, CHAR_DATA *ch, const void *arg1, const void *arg2,
    int type, bool ooc)
{
    /* to be compatible with older code */
    act_new(format,ch,arg1,arg2,type,POS_RESTING,ooc);
}

void act_new( const char *format, CHAR_DATA *ch, const void *arg1, 
        const void *arg2, int type, int min_pos, bool ooc)
{
    static char * const he_she  [] = { "it",  "he",  "she" };
    static char * const him_her [] = { "it",  "him", "her" };
    static char * const his_her [] = { "its", "his", "her" };
 
    char buf[2*MAX_STRING_LENGTH];
    char fname[MAX_INPUT_LENGTH];
    CHAR_DATA *to;
    CHAR_DATA *vch = (CHAR_DATA *) arg2;
    OBJ_DATA *obj1 = (OBJ_DATA  *) arg1;
    OBJ_DATA *obj2 = (OBJ_DATA  *) arg2;
    const char *str;
    const char *i='\0';
    char *point;
 
    /*
     * Discard null and zero-length messages.
     */
    if ( format == NULL || format[0] == '\0' )
  return;

    /* discard null rooms and chars */
    if (ch == NULL || ch->in_room == NULL)
  return;

    to = ch->in_room->people;
    if ( type == TO_VICT )
    {
  if ( vch == NULL )
  {
      bug( "Act: null vch with TO_VICT.", 0 );
      return;
  }

  if (vch->in_room == NULL)
      return;

  to = vch->in_room->people;
    }
 
    for ( ; to != NULL; to = to->next_in_room )
    {
  if ( (to->desc == NULL) || (to->position < min_pos) )
      continue;
 
  if ( (type == TO_CHAR) && to != ch )
      continue;
  if ( type == TO_VICT && ( to != vch || to == ch ) )
      continue;
  if ( type == TO_ROOM && to == ch )
      continue;
  if ( type == TO_NOTVICT && (to == ch || to == vch) )
      continue;
 
  point   = buf;
  str     = format;
  while ( *str != '\0' )
  {
      if ( *str != '$' )
      {
    *point++ = *str++;
    continue;
      }
      ++str;
 
      if ( arg2 == NULL && *str >= 'A' && *str <= 'Z' )
      {
    bug( "Act: missing arg2 for code %d.", *str );
    i = " <@@@> ";
      }
      else
      {
    switch ( *str )
    {
    default:  bug( "Act: bad code %d.", *str );
        i = " <@@@> ";                                break;
    /* Thx alex for 't' idea */
    case 't': if(arg1) i = (char *) arg1;	break;
    case 'T': if(arg2) i = (char *) arg2;	break;
    case 'l':
    case 'n': if(ch) {
        if(IS_SET(ch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE )
	{
          i = (!IS_NPC(to) && IS_SET(to->act,PLR_HOLYLIGHT)) ?
       		 PERS(ch,to,ooc): ch->long_descr;
	}
	else
	{
	  i = PERS( ch, to, ooc );
	}
    }
	  break;
    case 'L':
    case 'N': if(vch) {
    if(IS_SET(vch->mhs,MHS_GLADIATOR) && gladiator_info.blind == TRUE )
	{
          i = (!IS_NPC(to) && IS_SET(to->act,PLR_HOLYLIGHT)) ?
       		 PERS(vch,to,ooc): vch->long_descr;
	}
	else
	{
	  i = PERS( vch, to, ooc );
	}
    }
	  break;
    case 'e': if(ch) i = he_she  [URANGE(0, ch  ->sex, 2)];        break;
    case 'E': if(vch) i = he_she  [URANGE(0, vch ->sex, 2)];        break;
    case 'm': if(ch) i = him_her [URANGE(0, ch  ->sex, 2)];        break;
    case 'M': if(vch) i = him_her [URANGE(0, vch ->sex, 2)];        break;
    case 's': if(ch) i = his_her [URANGE(0, ch  ->sex, 2)];        break;
    case 'S': if(vch) i = his_her [URANGE(0, vch ->sex, 2)];        break;
 
    case 'r': if(arg2) i = (char *) arg2;		    break;

    case 'p':
        if(obj1) i = can_see_obj( to, obj1 )
          ? obj1->short_descr
          : "something";
        break;
 
    case 'P':
        if(obj2) i = can_see_obj( to, obj2 )
          ? obj2->short_descr
          : "something";
        break;
 
    case 'd':
        if ( arg2 == NULL || ((char *) arg2)[0] == '\0' )
        {
      i = "door";
        }
        else
        {
      one_argument( (char *) arg2, fname );
      i = fname;
        }
        break;

    }
      }
 
      ++str;
      while ( ( *point = *i ) != '\0' )
    ++point, ++i;
  }
 
  *point++ = '\n';
  *point++ = '\r';
  *point = '\0';
  buf[0]   = UPPER(buf[0]);
  write_to_buffer( to->desc, buf, point - buf);
    }
 
    return;
}

/*
 * Macintosh support functions.
 */
#if defined(macintosh)
int gettimeofday( struct timeval *tp, void *tzp )
{
    tp->tv_sec  = time( NULL );
    tp->tv_usec = 0;
}
#endif
