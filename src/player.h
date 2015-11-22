/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  player.h
*/

/* kludgy macro, there must be a better way to do this */

#define align(p) p=(void *)(((int)p+3)&-4)

/* gender types */

#define MALE 1
#define FEMALE 2
#define OTHER 0
#define VOID_GENDER -1

/* residency types */

#define STANDARD_ROOMS -2
#define BANISHED -1
#define NON_RESIDENT 0
#define BASE 1
#define NO_SYNC 2
#define ECHO_PRIV 4
#define NO_TIMEOUT 8
#define MAIL 64
#define LIST 128
#define BUILD 256
#define SESSION 512

#define OFF_DUTY (1<<14)

#define WARN (1<<15)

#define LOWER_ADMIN (1<<26)
#define BARF  (1<<27)
#define LOWER_SU (1<<28)
#define UPPER_SU (1<<29)
#define ADMIN (1<<30)



/* #define lengths */

#define MAX_NAME 20
#define MAX_INET_ADDR 40
#define IBUFFER_LENGTH 256
#define MAX_PROMPT 15
#define MAX_ID 15
#define MAX_EMAIL 60
#define MAX_PASSWORD 20
#define MAX_TITLE 60
#define MAX_DESC 160
#define MAX_PLAN 160
#define MAX_PRETITLE 18
#define MAX_ENTER_MSG 60
#define MAX_IGNOREMSG 55
#define MAX_SESSION 60
#define MAX_COMMENT 50

/* an un-subtle size, but... */
#define MAX_REPLY 200
#define MAX_ROOM_CONNECT 35

#define MAX_ROOM_NAME 50
#define MAX_AUTOMESSAGE 160
#define MAX_ROOM_SIZE 1500
#define MAX_ARTICLE_SIZE 5000
#define MAX_SNEWS 200

/* system flag definitiosn */

#define PANIC (1<<0)
#define VERBOSE (1<<1)
#define SHUTDOWN (1<<2)
/*  missing flag 8 */
#define EVERYONE_TAG (1<<3)
#define FAILED_COMMAND (1<<4)
#define CLOSED_TO_NEWBIES (1<<5)
#define PIPE (1<<6)
#define ROOM_TAG (1<<7)
#define FRIEND_TAG (1<<8)
#define DO_TIMER (1<<9)
#define UPDATE (1<<10)
#define NO_PRINT_LOG (1<<11)
#define NO_PRETITLES (1<<12)
#define UPDATEROOMS (1<<13)
#define UPDATEFLAGS (1<<14)
#define NEWBIE_TAG (1<<15)
#define ENFORCE_NO_NEW (1<<16)
#define SECURE_DYNAMIC (1<<17)


/* player flag defs */

/* keep PANIC as 1 */
#define INPUT_READY (1<<1)
#define LAST_CHAR_WAS_N (1<<2)
#define LAST_CHAR_WAS_R (1<<3)
#define DO_LOCAL_ECHO (1<<4)
#define PASSWORD_MODE (1<<5)
/* keep closed to newbies at 64 */
#define PROMPT (1<<7)
#define TAGGED (1<<8)
#define LOGIN (1<<9)
#define CHUCKOUT (1<<10)
#define EOR_ON (1<<11)
#define IAC_GA_DO (1<<12)
#define SITE_LOG (1<<13)
#define DONT_CHECK_PIPE (1<<14)
#define RECONNECTION (1<<15)
#define NO_UPDATE_L_ON (1<<16)
#define BLOCK_SU (1<<17)
#define NO_SAVE_LAST_ON (1<<18)
#define NO_SU_WALL (1<<19)
#define ASSISTED (1<<20)

/* ones that get saved */

#define CONVERSE (1<<0)
#define LONG_DESCS (1<<1)
#define BLOCK_TELLS (1<<2)
#define BLOCK_SHOUT (1<<3)
#define HIDING (1<<4)
#define PRIVATE_EMAIL (1<<5)
#define QUIET_EDIT (1<<6)
#define TAG_ECHO (1<<7)
#define TAG_PERSONAL (1<<8)
#define TAG_ROOM (1<<9)
#define TAG_SHOUT (1<<10)
#define TRANS_TO_HOME (1<<11)
#define SEEECHO (1<<12)
#define NEW_MAIL (1<<13)
#define NO_ANONYMOUS (1<<14)
#define MAIL_INFORM (1<<15)
#define NEWS_INFORM (1<<16)
#define TAG_AUTOS (1<<17)
#define PERM_LAG (1<<18)
#define IAC_GA_ON (1<<19)
#define COMPRESSED_LIST (1<<20)
#define NO_PAGER (1<<21)
#define AGREED_DISCLAIMER (1<<22)
#define NOPREFIX (1<<23)
#define SAVENOSHOUT (1<<24)
#define YES_SESSION (1<<26)
#define LOG_CONNECTION (1<<27)
#define ROOM_ENTER (1<<28)
#define SHOW_EXITS (1<<29)
#define NEW_SITE (1<<30)

/* list flags */

#define NOISY 1
#define IGNORE 2
#define INFORM 4
#define GRAB 8
#define FRIEND 16
#define BAR 32
#define INVITE 64 
#define BEEP 128
#define BLOCK 256
#define KEY 512
#define FIND 1024

/* command types */

#define VOID 0
#define SEE_ERROR 1
#define PERSONAL 2
#define ROOM 4
#define EVERYONE 8
#define ECHO_COM 16
#define EMERGENCY 32
#define AUTO 64
#define HIGHLIGHT 128
#define NO_P_MATCH 256
#define TAG_INFORM 512
#define LIST_EVERYONE 1024
#define ADMIN_BARGE 2048
#define SORE 4096

/* files'n'things */

typedef struct {
  char *where;
  int length;
} file;

/* just simple struct to hold the super help pages */

struct super_n {
    char text[MAX_SNEWS];
    int ident;
    struct super_n *next;
};

typedef struct super_n snews;

/* room definitions */

#define HOME_ROOM 1
#define COMPRESSED 2
#define AUTO_MESSAGE 4
#define AUTOS_TAG 8
#define LOCKABLE 16
#define LOCKED 32
#define OPEN 64
#define LINKABLE 128
#define KEYLOCKED 256
#define CONFERENCE 512
#define ROOM_UPDATED 1024

struct r_struct {
  char name[MAX_ROOM_NAME];
  char id[MAX_ID];
  int flags;
  int data_key;
  int auto_count;
  int auto_base;
  file text;
  file exits;
  file automessage;
  struct s_struct *owner;
  struct p_struct *players_top;
  struct r_struct *next;
  char enter_msg[MAX_ENTER_MSG];
};

typedef struct r_struct room;

/* note defs */

#define NEWS_ARTICLE 1
#define ANONYMOUS 2
#define NOT_READY 4
#define SUPRESS_NAME 8

struct n_struct {
  int id;
  int flags;
  int date;
  file text;
  int next_sent;
  int read_count;
  struct n_struct *hash_next;
  char header[MAX_TITLE];
  char name[MAX_NAME];
};

typedef struct n_struct note;

/* list defs */

struct l_struct {
  char name[MAX_NAME];
  int flags;
  struct l_struct *next;
};

typedef struct l_struct list_ent;

/* saved player defs */

struct s_struct {
  char lower_name[MAX_NAME];
  char last_host[MAX_INET_ADDR];
  int last_on;
  int residency;
  int saved_flags;
  file data;
  struct l_struct *list_top;
  struct r_struct *rooms;
  int mail_sent;
  int *mail_received;
  struct s_struct *next;
};

typedef struct s_struct saved_player;

/* editor info structure */

typedef struct {
  char *buffer;
  char *current;
  int max_size;
  int size;
  void *finish_func;
  void *quit_func;
  int flag_copy;
  int sflag_copy;
  void *input_copy;
  void *misc;
} ed_info;


/* terminal defs */

struct terminal {
  char *name;
  char *bold;
  char *off;
  char *cls;
};



/* the player structure */

struct p_struct {
  int fd;
  int performance;
  int hash_top;
  int flags;
  int term;
  int saved_flags;
  int anticrash;
  int antipipe;
  int residency;
  int saved_residency;
  int term_width;
  int column;
  int word_wrap;
  int idle;
  int gender;
  int no_shout;
  int shout_index;
  int jail_timeout;
  int no_move;
  int lagged;
  int script;
  int jetlag;   /* This has just become time zone difference */
  int squished;    /* wibble! */
  int birthday;
  int age;
  int last_newsb;
  struct p_struct *hash_next;
  struct p_struct *flat_next;
  struct p_struct *flat_previous;
  struct p_struct *room_next;
  saved_player *saved;
  room *location;
  int max_rooms;
  int max_exits;
  int max_autos;
  int max_list;
  int max_mail;
  int on_since;
  int total_login;
  ed_info *edit_info;
  char inet_addr[MAX_INET_ADDR];
  char num_addr[MAX_INET_ADDR];
  char name[MAX_NAME];
  char title[MAX_TITLE];
  char pretitle[MAX_PRETITLE];
  char description[MAX_DESC];
  char plan[MAX_PLAN];
  char lower_name[MAX_NAME];
  char idle_msg[MAX_TITLE];
  
  char ignore_msg[MAX_IGNOREMSG];
  char comment[MAX_COMMENT];
  char reply[MAX_REPLY];
  char room_connect[MAX_ROOM_CONNECT];
  int reply_time;

  void *input_to_fn;
  void *timer_fn;
  int timer_count;
  char ibuffer[IBUFFER_LENGTH];
  int ibuff_pointer;
  char prompt[MAX_PROMPT];
  char converse_prompt[MAX_PROMPT];
  char email[MAX_EMAIL];
  char password[MAX_PASSWORD];
  char password_cpy[MAX_PASSWORD];
  char enter_msg[MAX_ENTER_MSG];
};

typedef struct p_struct player;

/* flag list def */

typedef struct {
  char *text;
  int change;
} flag_list;

/* structure for commands */

struct command {
  char *text;
  void *function;
  int level;
  int space;
  char *help;
};

/* global definitions */

#ifndef GLOBAL_FILE

char *action;
extern char *stack,*stack_start;
extern int sys_flags,max_players,current_players,command_type,
  up_time,up_date,logins;
extern player *flatlist_start,*hashlist[],*current_player,*stdout_player,
	*input_player;
extern room *current_room,*entrance_room,*prison;
extern player **pipe_list;
extern int pipe_matches;
extern int splat1,splat2,splat_timeout;
extern int soft_splat1,soft_splat2,soft_timeout;
#endif


extern int in_total,out_total,in_current,out_current,
  in_average,out_average,net_count,
  in_bps,out_bps,
  in_pack_total,out_pack_total,
  in_pack_current,out_pack_current,
  in_pps,out_pps,in_pack_average,out_pack_average;
