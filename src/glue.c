/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/
/* 
  glue.c

  holds the program together ...
  This is where you will find main()
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <memory.h>
#ifdef MIPS
extern int errno;
#endif

#include "config.h"
#include "player.h"
#include "fix.h"
#include "dynamic.h"


#define ABS(x) (((x)>0)?(x):-(x))

/* extern definitions */

extern void raw_wall(char *);
extern void lower_case(char *);
extern char *sys_errlist[];
/* extern int strftime(char *,int,char *,struct tm*); */
extern void alive_connect();

extern void close_down_socket(),scan_sockets(),process_players(),
    init_parser(),init_rooms(),init_plist(),init_socket(),save_player(),
    sync_all(),actual_timer(),init_notes(),sync_notes(),init_help(),
    do_update(),timer_function(),fork_the_thing_and_sync_the_playerfiles(),
    sync_banished_files();
extern dfile *dynamic_init();
extern dfile *room_df;

extern int total_processing;

void close_down();

#ifndef S_IRUSR
#define S_IRUSR 00400
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200
#endif



/* interns */

char *tens_words[]={ "","ten","twenty","thirty","forty","fifty",
		     "sixty","seventy","eighty","ninety" };

char *units_words[]={ "none","one","two","three","four","five","six",
			"seven","eight","nine" };

char *teens[]={ "ten","eleven","twelve","thirteen","fourteen","fifteen",
		  "sixteen","seventeen","eighteen","nineteen"};


char *months[12]={ "January","February","March","April","May","June",
		   "July","August","September","October","November",
		   "December" };


/* print up birthday */

char *birthday_string(time_t bday)
{
  static char bday_string[50];
  struct tm *t;
  t=localtime(&bday);
  if ((t->tm_mday)>10 && (t->tm_mday)<20) 
    sprintf(bday_string,"%dth of %s",t->tm_mday,months[t->tm_mon]);
  else 
    switch((t->tm_mday)%10) {
    case 1:
      sprintf(bday_string,"%dst of %s",t->tm_mday,months[t->tm_mon]);
      break;
    case 2:
      sprintf(bday_string,"%dnd of %s",t->tm_mday,months[t->tm_mon]);
      break;
    case 3:
      sprintf(bday_string,"%drd of %s",t->tm_mday,months[t->tm_mon]);
      break;
    default:
      sprintf(bday_string,"%dth of %s",t->tm_mday,months[t->tm_mon]);
      break;
    }
  return bday_string;
}

/* return a string of the system time */

char *sys_time()
{
  time_t t;
  static char time_string[25];
  t=time(0);
  strftime(time_string,25,"%H:%M:%S - %d/%m/%y",localtime(&t));
  return time_string;
}

/* returns converted user time */

char *convert_time(time_t t)
{
  static char time_string[50];
  strftime(time_string,49,"%I.%M:%S %p - %a, %d %B",localtime(&t));
  return time_string;
}

/* get local time for all those americans :) */

char *time_diff(int diff)
{
    time_t t;
    static char time_string[50];

    t=time(0)+3600*diff;
    strftime(time_string,49,"%I.%M:%S %p - %a, %d %B",localtime(&t));
    return time_string;
}

char *time_diff_sec(time_t last_on,int diff)
{
    static char time_string[50];
    time_t sec_diff;
    
    sec_diff=(3600*diff)+last_on;
    strftime(time_string,49,"%I.%M:%S %p - %a, %d %B",localtime(&sec_diff));
    return time_string;
}



/* converts time into words */

char *word_time(int t)
{
  static char time_string[100],*fill;
  int days,hrs,mins,secs;
  if (!t) return "no time at all";
  days=t/86400;
  hrs=(t/3600)%24;
  mins=(t/60)%60;
  secs=t%60;
  fill=time_string;
  if (days) {
    sprintf(fill,"%d day",days);
    while(*fill) fill++;
    if (days!=1) *fill++='s';
    if (hrs || mins || secs) {
      *fill++=',';
      *fill++=' ';
    }
  }
  if (hrs) {
    sprintf(fill,"%d hour",hrs);
    while(*fill) fill++;
    if (hrs!=1) *fill++='s';
    if (mins && secs) {
      *fill++=',';
      *fill++=' ';
    }
    if ((mins && !secs) || (!mins && secs)) {
      strcpy(fill," and ");
      while(*fill) fill++;
    }
  }
  if (mins) {
    sprintf(fill,"%d minute",mins);
    while(*fill) fill++;
    if (mins!=1) *fill++='s';
    if (secs) {
      strcpy(fill," and ");
      while(*fill) fill++;
    }
  }
  if (secs) {
    sprintf(fill,"%d second",secs);
    while(*fill) fill++;
    if (secs!=1) *fill++='s';
  }
  *fill++=0;
  return time_string;
}

/* returns a number in words */

char *number2string(int n)
{
  int hundreds,tens,units;
  static char words[50];
  char *fill;
  if (n>=1000) {
    sprintf(words,"%d",n);
    return words;
  }
  if (!n) return "none";
  hundreds=n/100;
  tens=(n/10)%10;
  units=n%10;
  fill=words;
  if (hundreds) {
    sprintf(fill,"%s hundred",units_words[hundreds]);
    while(*fill) fill++;
  }
  if (hundreds && (units || tens)) {
    strcpy(fill," and ");
    while(*fill) fill++;
  }
  if (tens && tens!=1) {
    strcpy(fill,tens_words[tens]);
    while(*fill) fill++;
  }
  if (tens!=1 && tens && units) *fill++=' ';
  if (units && tens!=1) {
    strcpy(fill,units_words[units]);
    while(*fill) fill++;
  }
  if (tens==1) {
    strcpy(fill,teens[(n%100)-10]);
    while(*fill) fill++;
  }
  *fill++=0;
  return words;
}

/* point to after a string  */

char *end_string(char *str)
{
    while(*str) str++;
    str++;
    return str;
}


/* get gender string function */

char *get_gender_string(player *p)
{
  switch(p->gender) {
  case MALE:
    return "him";
    break;
  case FEMALE:
    return "her";
    break;
  case OTHER:
    return "it";
    break;
  case VOID_GENDER:
    return "it";
    break;
  }
  return "(this is frogged)";
}

/* get gender string for possessives */

char *gstring_possessive(player *p)
{
  switch(p->gender) {
  case MALE:
    return "his";
    break;
  case FEMALE:
    return "her";
    break;
  case OTHER:
    return "its";
    break;
  case VOID_GENDER:
    return "its";
    break;
  }
  return "(this is frogged)";
}


/* more gender strings */

char *gstring(player *p)
{
  switch(p->gender) {
  case MALE:
    return "he";
    break;
  case FEMALE:
    return "she";
    break;
  case OTHER:
    return "it";
    break;
  case VOID_GENDER:
    return "it";
    break;
  }
  return "(this is frogged)";
}

/* returns the 'full' name of someone, that is their pretitle and name */

char *full_name(player *p)
{
  static char fname[MAX_PRETITLE+MAX_NAME];
  if ((!(sys_flags&NO_PRETITLES)) && (p->residency&BASE) && p->pretitle[0]) {
    sprintf(fname,"%s %s",p->pretitle,p->name);
    return fname;
  }
  return p->name;
}



/* log errors and things to file */

void log(char *file,char *string)
{
  int fd,length;
  char *oldstack, *newname;
  oldstack=stack;

  sprintf(oldstack,"logs/%s.log",file);
  fd = open(oldstack,O_CREAT|O_WRONLY|O_SYNC,S_IRUSR|S_IWUSR);

  length=lseek(fd,0,SEEK_END);
  if (length>MAX_LOG_SIZE) {
    close(fd);
    stack=end_string(oldstack);
    newname=stack;
    sprintf(newname,"logs/%s.old",file);
    rename(oldstack,newname);
    stack=oldstack;
    fd=open(oldstack,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
  }
  sprintf(stack,"%s - %s\n",sys_time(),string);
  if (!(sys_flags&NO_PRINT_LOG)) printf(stack);
  write(fd,stack,strlen(stack));
  close(fd);
  stack=oldstack;
}

/* what happens when *shriek* an error occurs */

void handle_error(char *error_msg)
{
  char dump[80];

/*  
  if (errno==EINTR) {
    log("error","EINTR trap");
    log("error",error_msg);
    return;
  }
*/
  if (sys_flags&PANIC) {
    stack=stack_start;
    log("error","Immediate PANIC shutdown.");
    exit(-1);
  }
  sys_flags |= PANIC;

/*
  sprintf(dump,"gcore %d",getpid());
  system(dump);
*/

  stack=stack_start;

  sys_flags &= ~NO_PRINT_LOG;

  log("error",error_msg);
  log("boot","Abnormal exit from error handler");
  
/* dump possible useful info */

  log("dump","------------ Starting dump");

  sprintf(stack_start,"Errno set to %d, %s",errno,sys_errlist[errno]);
  stack=end_string(stack_start);
  log("dump",stack_start);

  if (current_player) {
    log("dump",current_player->name);
    if (current_player->location) {
      sprintf(stack_start,"player %s.%s",
	      current_player->location->owner->lower_name,
	      current_player->location->id);
      stack=end_string(stack_start);
      log("dump",stack_start);
    } 
    else log("dump","No room of current player");  

    sprintf(stack_start,"flags %d saved %d residency %d",current_player->flags,
	    current_player->saved_flags,current_player->residency);
    stack=end_string(stack_start);
    log("dump",stack_start);
    log("dump",current_player->ibuffer);
  }
  else log("dump","No current player !");
  if (current_room) {
    sprintf(stack_start,"current %s.%s",current_room->owner->lower_name,
	    current_room->id);
    stack=end_string(stack_start);
    log("dump",stack_start);
  } 
  else log("dump","No current room");
  
  sprintf(stack_start,"global flags %d, players %d",sys_flags,current_players);
  stack=end_string(stack_start);
  log("dump",stack_start);

  sprintf(stack_start,"action %s",action);
  stack=end_string(stack_start);
  log("dump",stack_start);
  
  log("dump","---------- End of dump info");

  raw_wall("\n\n    ---====>>>> CRASH ! Attempting to sync player files <<<<"
	   "====---\007\n\n\n");
  
  close_down();
  exit(-1);
}


/* function to convert seamlessly to caps (ish) */

char *caps(char *str)
{
  static char buff[500];
  strncpy(buff,str,498);
  buff[0]=toupper(buff[0]);
  return buff;
}


/* load a file into memory */

file load_file_verbose(char *filename,int verbose)
{
  file f;
  int d;
  char *oldstack;

  oldstack=stack;

  d=open(filename,O_RDONLY);
  if (d<0) {
    sprintf(oldstack,"Can't find file:%s",filename);
    stack=end_string(oldstack);
    if (verbose)
	log("error",oldstack);
    f.where=(char *)MALLOC(1);
    *(char *)f.where=0;
    f.length=0;
    stack=oldstack;
    return f;
  }
  f.length=lseek(d,0,SEEK_END);
  lseek(d,0,SEEK_SET);
  f.where=(char *)MALLOC(f.length+1);
  memset(f.where,0,f.length+1);
  if (read(d,f.where,f.length)<0) {
    sprintf(oldstack,"Error reading file:%s",filename);
    stack=end_string(oldstack);
    log("error",oldstack);
    f.where=(char *)MALLOC(1);
    *(char *)f.where=0;
    f.length=0;
    stack=oldstack;
    return f;
  }
  close(d);
  if (sys_flags&VERBOSE) {
    sprintf(oldstack,"Loaded file:%s",filename);
    stack=end_string(oldstack);
    log("boot",oldstack);
    stack=oldstack;
  }
  stack=oldstack;
  *(f.where+f.length)=0;
  return f;
}

file load_file(char *filename)
{
    return load_file_verbose(filename,1);
}

/* convert a string to lower case */

void lower_case(char *str)
{
  while(*str) *str++=tolower(*str);
}

/* fns to block signals */

void sigpipe() { return; }
void sighup() 
{ 
    log("boot","Terminated by hangup signal");
    close_down();
    exit(0);
}
void sigquit() { handle_error("Quit signal received."); }
void sigill()  { handle_error("Illegal instruction."); }
void sigfpe()  { handle_error("Floating Point Error."); }
void sigbus()  { handle_error("Bus Error."); }
void sigsegv() { handle_error("Segmentation Violation."); }
void sigsys()  { handle_error("Bad system call."); }
void sigterm() { handle_error("Terminate signal received."); }

/* in the case of an fd limit exceeded, clear out as many fd's as
   possible, by junking all the players. This allows a few fds
   spare to log stuff */

void sigxfsz() 
{
  player *scan;
  int len;
  char warning[]="\n\n\n--==>> Immediate emergency shutdown.\n\n\n\n\007";
  
  len=strlen(warning);

  for(scan=flatlist_start;scan;scan=scan->flat_next) {
      write(scan->fd,warning,len);
      close(scan->fd);
      scan->fd = 0;
  }
  handle_error("File descriptor limit exceeded.");
}
void sigusr1() { fork_the_thing_and_sync_the_playerfiles(); }

/* close down sequence */

void close_down()
{
  player *scan;
#ifndef PC
  struct itimerval new,old;
#endif
  raw_wall("\007\n\n");
  command_type |= HIGHLIGHT;
  raw_wall("          ---====>>>> Program shutting down NOW <<<<====---"
	   "\n\n\n");
  command_type &= ~HIGHLIGHT;

#ifndef PC
  new.it_interval.tv_sec=0;
  new.it_interval.tv_usec=0;
  new.it_value.tv_sec=0;
  new.it_value.tv_usec=new.it_interval.tv_usec;
  if (setitimer(ITIMER_REAL,&new,&old)<0) handle_error("Can't set timer.");
  if (sys_flags&VERBOSE || sys_flags&PANIC) log("boot","Timer Stopped");
#endif
  
  if (sys_flags&VERBOSE || sys_flags&PANIC) log("boot","Saving all players.");
  for(scan=flatlist_start;scan;scan=scan->flat_next)
    save_player(scan);
  if (sys_flags&VERBOSE || sys_flags&PANIC) log("boot","Syncing to disk.");
  sync_all();

  sync_notes(0);
  sync_banished_files();
	
  if (sys_flags&PANIC)
    raw_wall("\n\n              ---====>>>> Files sunc (phew !) <<<<====---"
	     "\007\n\n\n");
  for(scan=flatlist_start;scan;scan=scan->flat_next)
      close(scan->fd);
  
  close_down_socket();
  
#ifdef PC
  chdir("src");
#endif
  
  if (!(sys_flags&PANIC)) {
      log("boot","Program exited normally.");
      exit(0);
  }
}


/* the boot sequence */

void boot(int port)
{
    char *oldstack;
    int i;
#ifndef PC
    struct rlimit rlp;
    struct itimerval new,old;
#endif

    oldstack=stack;
    log("boot","-=> EW-three <=- Boot Started");
    
    up_date=time(0);
    
#ifndef PC

#if !defined(ULTRIX) && !defined(LINUX)
    
    getrlimit(RLIMIT_NOFILE,&rlp);
    rlp.rlim_cur=rlp.rlim_max;
    setrlimit(RLIMIT_NOFILE,&rlp);
    max_players=(rlp.rlim_cur)-20;
    
    if (sys_flags&VERBOSE) {
	sprintf(oldstack,"Got %d file descriptors, Allocated %d for players",
		rlp.rlim_cur,max_players);
    stack=end_string(oldstack);
	log("boot",oldstack);
	stack=oldstack;
    }
    
#else
    max_players=HARD_PLAYER_LIM;
    
    if (sys_flags&VERBOSE) {
    sprintf(oldstack,"Set max players to %d.",max_players);
    stack=end_string(oldstack);
    log("boot",oldstack);
    stack=oldstack;
}
#endif
    
#ifndef SOLARIS
/*    getrlimit(RLIMIT_RSS,&rlp);
    rlp.rlim_cur=MAX_RES;
    setrlimit(RLIMIT_RSS,&rlp); */
#endif
    
#else
    max_players=10;
#endif
    
  flatlist_start=0;
    for(i=0;i<27;i++) hashlist[i]=0;
    
    stdout_player=(player *)MALLOC(sizeof(player));
    memset(stdout_player,0,sizeof(player));
    
    srand(time(0));

    room_df=dynamic_init("rooms",256);
     
    init_plist();
    init_parser();
    init_rooms();
    init_notes();
    init_help();
        
#ifndef PC
    if (!(sys_flags&SHUTDOWN)) {
    new.it_interval.tv_sec=0;
    new.it_interval.tv_usec=(1000000/TIMER_CLICK);
    new.it_value.tv_sec=0;
    new.it_value.tv_usec=new.it_interval.tv_usec;
    if ((int)signal(SIGALRM,actual_timer)<0) handle_error("Can't set timer signal.");
    if (setitimer(ITIMER_REAL,&new,&old)<0) handle_error("Can't set timer.");
    if (sys_flags&VERBOSE) 
      log("boot","Timer started.");
  }
  signal(SIGPIPE,sigpipe);
  signal(SIGHUP,sighup);
  signal(SIGQUIT,sigquit);
  signal(SIGILL,sigill);
  signal(SIGFPE,sigfpe);
  signal(SIGBUS,sigbus);
  signal(SIGSEGV,sigsegv);
#ifndef LINUX
  signal(SIGSYS,sigsys);
#endif
  signal(SIGTERM,sigterm);
  signal(SIGXFSZ,sigxfsz);
  signal(SIGUSR1,sigusr1);
#endif
  
  if (!(sys_flags&SHUTDOWN)) {
      init_socket(port);
      alive_connect();
  }

  current_players=0;

  stack=oldstack;
}

/* got to have a main to control everything */

void main(int argc,char *argv[])
{
    int port=0;
  
    action="boot";
    /*  if (mallopt(M_MXFAST,1024)) {
	perror("spoon:");
	exit(0);
	}*/


#ifdef MALLOC_DEBUG
    malloc_debug(2);
#endif
  
    stack_start=(char *)MALLOC(STACK_SIZE);
    memset(stack_start,0,STACK_SIZE);
    stack=stack_start;
  

    if (argc==3) 
	if (!strcmp("update",argv[1])) {
	    if (!strcmp("rooms",argv[2])) {
		log("boot","Program booted for file rooms update.");
		sys_flags |= SHUTDOWN|UPDATEROOMS;
	    }
	    else if (!strcmp("flags",argv[2])) {
		log("boot","Program booted for flags update");
		sys_flags |= SHUTDOWN|UPDATEFLAGS;
	    }
	    else {
		log("boot","Program booted for file players update.");
 		sys_flags |= SHUTDOWN|UPDATE;
	    }
	}
    if (argc==2) 
	port=atoi(argv[1]);
    
    if (!port) port=DEFAULT_PORT;
    
    if (chdir(ROOT)) {
	printf("Can't change to root directory.\n");
	exit(1);
    }
  
    boot(port);
  
#ifdef PC
    accept_new_connection();
#endif
  
    if (sys_flags&UPDATE) do_update(0);
    else 
	if (sys_flags&UPDATEFLAGS) do_update(0);
	else
	    if (sys_flags&UPDATEROOMS) do_update(1);
    sys_flags |= NO_PRINT_LOG;
    
    while(!(sys_flags&SHUTDOWN)) {
    
	errno=0;

	if (stack!=stack_start) {
	    sprintf(stack_start,"Lost stack reclaimed %d bytes\n",
		    (int)stack-(int)stack_start);
	    stack=end_string(stack_start);
	    log("stack",stack_start);
	    stack=stack_start;
	}
    
	action="scan sockets";
	scan_sockets();
	action="processing players";
	process_players();
	action="";

	timer_function();
	sigpause(0); 

	do_alive_ping();

    }
  
    close_down();
}
