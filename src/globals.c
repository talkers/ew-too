/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  globals.c
*/

#define GLOBAL_FILE

#include "config.h"
#include "player.h"

/* boot thangs */

int up_date;
int logins=0;

/* sizes */

int max_players,current_players=0;

int in_total=0,out_total=0,in_current=0,out_current=0,
  in_average=0,out_average=0,net_count=10,
  in_bps=0,out_bps=0,
  in_pack_total=0,out_pack_total=0,
  in_pack_current=0,out_pack_current=0,
  in_pps=0,out_pps=0,in_pack_average=0,out_pack_average=0;


/* One char for splat sites */

int splat1,splat2;
int splat_timeout;
int soft_splat1,soft_splat2,soft_timeout=0;

/* sessions!  */

char session[MAX_SESSION];
int session_reset=0;
player *p_sess=0;

/* flags */

int sys_flags=0;
int command_type=0;

/* pointers */

char *action;
char *stack,*stack_start;
player *flatlist_start;
player *hashlist[27];
player *current_player;
room *current_room;
player *stdout_player;

player **pipe_list;
int pipe_matches;

room *entrance_room,*prison;
int no_of_entrance_rooms;


/* lists for use with idle times 
   its here for want of a better place to put it */

file idle_string_list[]={
  {"has just hit return.\n",0},
  {"is typing merrily away.\n",10},
  {"hesitates slightly.\n",15},
  {"is thinking about what to type next.\n",25},
  {"appears to be stuck for words.\n",40},
  {"ponders thoughtfully about what to say.\n",60},
  {"stares oblivious into space.\n",200},
  {"is on the road to idledom.\n",300},
  {"is off to the loo ?\n",600},
  {"appears to be doing something else.\n",900},
  {"is slipping into unconsciousness.\n",1200},
  {"has fallen asleep at the keyboard.\n",1800},
  {"snores loudly.\n",2400},
  {"moved !! .... no sorry, false alarm.\n",3000},
  {"seems to have passed away.\n",3600},
  {"is dead and buried.\n",5400},
  {"passed away a long time ago.\n",7200},
  {0,0} };
