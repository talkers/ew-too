/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* 
  config.h
*/

/* define these for ULTRIX */

#undef  ULTRIX

/* define this for Solaris 2.1 upwards */

#undef SOLARIS

/* define this for MIPS (Prime/SGI machines) */

#undef MIPS

/* and this for Linux */

#undef LINUX

/* for Ultrix and Linux only, this defines the absolute
   maximum number of players on simultaneously */

#define HARD_PLAYER_LIM 250


/* the system equivalent of the timelocal command */

/* this for SunOS  
#define TIMELOCAL(x) timelocal(x) */

/* this for ULTRIX, Solaris and Linux */
#define TIMELOCAL(x) mktime(x) 


/* default port no */

#define DEFAULT_PORT 2010

/* Root directory */

#define ROOT "/your/home/directory/EW-too"

/* path for the test alive socket */

#define SOCKET_PATH "junk/alive_socket"

/* this is the room where people get chucked to when they enter the
   program */

#define ENTRANCE_ROOM "ewtoo.entrance"
#define ENTRANCE_NAME "ewtoo"

/* this is the size of the stack for normal functions

   Due to historical reasons, this value must be larger than
   the largest data file (ie the largest player or notes file).
   This is a pain in the bottom.
   With a large database you will have to increase this value
   enormously, and most of the space will be wasted. If your data
   files are growing, and you suddenly start to get mysterious crashes,
   then this is the first thing to check */

#define STACK_SIZE 50000

/* largest permitted log size */

#define MAX_LOG_SIZE 75000

/* saved hash table size */

#define HASH_SIZE 64

/* note hash table size */

#define NOTE_HASH_SIZE 40

/* speed of the prog (number of clicks a second) */

#define TIMER_CLICK 5

/* speed of the virtual timer (unused) */

#define VIRTUAL_CLICK 10000

/* time in seconds between every player file sync */

#define SYNC_TIME 60

/* time in seconds between every full note sync */

#define NOTE_SYNC_TIME 1800

/* defines how many lines EW-three thinks a terminal has
   (this should really be made a variable) */

#define TERM_LINES 20

/* enable or disable malloc debugging */

#undef MALLOC_DEBUG 

/* timeout on news articles (seconds) */

#define NEWS_TIMEOUT 2592000

/* timeout on a mail article (seconds) */

#define MAIL_TIMEOUT 2592000

/* timeout on players (seconds) */

#define PLAYER_TIMEOUT 2592000


/* how many names can be included in a pipe */

#define NAME_MAX_IN_PIPE 10

/* maximum number of you and yours and stuff in a pipe */

#define YOU_MAX_IN_PIPE 3


/* which malloc routines to use */

#define MALLOC malloc

#define FREE free

/* maximum resident memory size of the program */

#define MAX_RES 1048576

/* max players on system limits, these are offsets
         backwards from the max player limit */
 
#define PLAYER_MAX_SUPER 5
#define PLAYER_MAX_RES   15
#define PLAYER_MAX_NEW   35

/* player limits on single login rooms */

#define MAX_IN_ROOM_LIMIT 40


/* this stuff for testing on a PC, it probably wont work anymore */

#undef PC

#ifdef PC
#undef tolower
#define tolower(x) mytolower(x)
#ifndef PC_FILE
extern char mytolower();
#endif
#endif

