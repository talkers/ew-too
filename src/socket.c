/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* 
  socket.c
*/

#include "config.h"
#include "player.h"
#include "fix.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <arpa/telnet.h>
#include <ctype.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <netdb.h>
#if !defined(MIPS) && !defined(LINUX)
#include <sys/filio.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/un.h>

/* braindead MIPS doesnt have this in errno.h */

#ifdef MIPS
extern int errno;
#endif

#define IPROTO 0

static char SccsId[] = "%W%     %G%";

/* externs */

#if !defined(SOLARIS) && !defined(LINUX)
extern int close(int);
extern int socket(int,int,int);
extern int getsockopt(int,int,int,char *,int *);
extern int setsockopt(int,int,int,char *,int);
extern int bind(int,struct sockaddr *,int);
extern int listen(int,int);
extern int write(int,char *,int);
extern int read(int,char *,int);
extern void perror(char *);
extern void bzero(char *,int);
/* extern int select(int,fd_set*,fd_set*,fd_set*,struct timval*); */
extern char *sprintf(char *,char *,...);
#endif

extern void quit(player *,char *);
extern file load_file();
extern char *end_string();
extern player *create_player();
extern void destroy_player(),connect_to_prog();
extern int test_receive();
extern void handle_error(char *);
extern void log(char *,char *);

/* interns */

int main_descriptor,alive_descriptor;
file banish_file,banish_msg,full_msg,splat_msg,banished_p;
void tell_player();


/* terminal defintitions */

struct terminal terms[] = {
  {"xterm","\033[1m","\033[m","\033[H\033[2J"}, 
  {"vt220","\033[1m","\033[m","\033[H\033[J"},
  {"vt100","\033[1m","\033[m","50\033[;H\0332J"},
  {"vt102","\033[1m","\033[m","50\033["},
  {"ansi","\033[1m","\033[0m","50\033[;H\0332J"},
  {"wyse-30","\033G4","\033G0",""},
  {"tvi912","\033l","\033m","\032"},
  {"sun","\033[1m","\033[m","\014"},
  {"adm","\033)","\033(","1\032"},
  {"hp2392", "\033&dB","\033&d@", "\033H\033J"}
};

void clear_screen(player *p,char *str)
{
    if (p->term)
	tell_player(p,terms[(p->term)-1].cls);
    else
	tell_player(p,"You have to have a termtype set for this command to"
		    " work. Use the command    hitells <termtype>\n");
}


/* the hitells command */

void hitells(player *p,char *str)
{
  char *oldstack;
  int i;
  oldstack=stack;
  if (!*str && !(p->term)) {
    tell_player(p,"Format: Hitells <termtype/?/off>\n");
    return;
  }
  if (!*str) {
    sprintf(stack,"Hitells is on, with terminal set to %s.\n",
	    terms[(p->term)-1].name);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
    return;
  }
  if (*str=='?') {
    strcpy(stack,"Current terminal types available : ");
    stack=strchr(stack,0);
    for(i=0;i<9;i++) {
      sprintf(stack,"%s, ",terms[i].name);
      stack=strchr(stack,0);;
    }
    stack-=2;
    *stack++='.';
    *stack++='\n';
    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
    return;
  }
  if (!strcmp("off",str)) {
    p->term=0;
    tell_player(p,"Hitells turned off.\n");
    return;
  }
  for(i=0;i<9;i++) 
    if (!strcmp(str,terms[i].name)) {
      p->term=i+1;
      sprintf(stack,"Hitells turned on, with terminal set to %s.\n",terms[i].name);
      stack=end_string(stack);
      tell_player(p,oldstack);
      stack=oldstack;
      return;
    }
  sprintf(stack,"Terminal type '%s' not supported.\n"
	  "Do hitells '?' to list currently supported terminals.\n",str);
  stack=end_string(stack);
  tell_player(p,oldstack);
  stack=oldstack;
  return;
}

/* close down sockets after use */

void close_down_socket()
{
  close(main_descriptor);
}


/* grab the main socket */

void init_socket(int port)
{
  struct sockaddr_in main_socket;
  int dummy=1; 
  char *oldstack;

  oldstack=stack;
  
/* drag in all the msg files */

  banish_file=load_file("files/banish");
  banish_msg=load_file("files/banish.msg");
  full_msg=load_file("files/full.msg");
  banished_p=load_file("files/players/banished_players");

/* grab the main socket */
  
  main_descriptor=socket(PF_INET,SOCK_STREAM,IPROTO);
  if (main_descriptor<0) handle_error("Can't grab a socket.");
  if (setsockopt(main_descriptor,SOL_SOCKET,SO_REUSEADDR,(char *)&dummy,sizeof(dummy))<0)
    handle_error("Bad socket operation.");
  
  if (ioctl(main_descriptor,FIONBIO,&dummy)<0)
    handle_error("Can't set non-blocking");

  main_socket.sin_family=AF_INET;
  main_socket.sin_port=htons(port);
  main_socket.sin_addr.s_addr= INADDR_ANY;
  
  if (bind(main_descriptor,(struct sockaddr *)&main_socket,sizeof(main_socket))<0)
    handle_error("Can't bind socket.");
    

  if (listen(main_descriptor,5)<0) handle_error("Listen refused");
  
  sprintf(oldstack,"Main socket bound and listening on port %d",port);
  stack=end_string(oldstack);
  log("boot",oldstack);

  stack=oldstack;
}

/* tell the angel the server is alive */

void do_alive_ping()
{
  static int count=5;

  count--;
  if (!count && alive_descriptor>0) {
    count=5;
    write(alive_descriptor,"SpAng!",6);
  }
}


/* connect to the alive socket */

void alive_connect()
{
  struct sockaddr_un sa;


  alive_descriptor=socket(PF_UNIX,SOCK_STREAM,0);
  if (alive_descriptor<0) handle_error("failed to make socket");
  
  sa.sun_family=AF_UNIX;
  strcpy(sa.sun_path,SOCKET_PATH);
  
  if (connect(alive_descriptor,(struct sockaddr *)&sa,sizeof(sa))<0) {
    close(alive_descriptor);
    alive_descriptor=-1;
    log("error","Failed to connect to alive socket - ignoring");
    return;
  }

  do_alive_ping();

  log("boot","Alive and kicking");

}



/* work out whether a player is site banished or not */

int match_banish(player *p,char *line)
{
  char *addr;  
  for(addr=p->num_addr;*addr;addr++,line++)
    if (*line=='*') {
      while(isdigit(*addr)) addr++;
      line++;
    }
    else if (*addr!=*line) return 0;
  return 1;
}

int do_banish(player *p)
{
  char *scan;
  int i;
  
  scan=banish_file.where;
  for(i=banish_file.length;i;) {
    if (*scan!='#' && match_banish(p,scan)) {
      while(*scan!='L' && *scan!='C' && *scan!='N' && *scan!='\n') scan++;
      switch(*scan) {
      case 'C':
	return 1;
      case 'N':
	p->flags |= CLOSED_TO_NEWBIES;
	break;
      case 'L':
	p->flags |= SITE_LOG;
	break;
      }
      return 0;
    }
    while(i && *scan!='\n') {
      scan++;
      i--;
    }
    if (i) {
      scan++;
      i--;
    }
  }
  return 0;
}

/* accept new connection on the main socket */

void accept_new_connection()
{
  struct sockaddr_in incoming;
  struct hostent *hp;
  int length,new_socket,socket;
  char *resolved_address;
  player *p;
  int dummy=1,no1,no2,no3,no4;
  char *oldstack;
/* MARBLE_HOSTFIX defined in config.h */
#ifdef MARBLE_HOSTFIX
  long haddr;
  char *nullptr;
#endif
  oldstack=stack;
  
  length=sizeof(incoming);
  socket=accept(main_descriptor,(struct sockaddr *)&incoming,&length);
  if ((socket<0) && errno==EINTR) {
    log("error","EINTR accept trap");
    return;
  }
  if (socket<0) handle_error("Error accepting new connection.");

/* this makes sure we get the lowest fd available */

  new_socket=dup(socket);
  close(socket);

  if (ioctl(new_socket,FIONBIO,&dummy)<0)
    handle_error("Can't set non-blocking");

  if (new_socket>=max_players) {
    write(new_socket,full_msg.where,full_msg.length);
    out_current+=full_msg.length;
    out_pack_current++;
    close(new_socket);
    return;
  }
  p=create_player();
  current_player=p;
  p->fd=new_socket;

  strncpy(p->num_addr,inet_ntoa(incoming.sin_addr),MAX_INET_ADDR-2);
  strncpy(p->inet_addr,p->num_addr,MAX_INET_ADDR-2);

#ifdef MARBLE_HOSTFIX
  haddr = inet_addr(p->num_addr);
  if(nullptr = strchr(p->num_addr, '\n'))
     *nullptr = '\0';
  if(haddr != -1) {
     sprintf(oldstack,"Got connection: %s",p->num_addr);
     stack=end_string(oldstack);
     log("RESOLVE",oldstack);
     hp = gethostbyaddr(&haddr, 4, AF_INET);
     if(hp) resolved_address=hp->h_name;
     else resolved_address=p->num_addr;
  }
  strncpy(p->inet_addr, resolved_address, MAX_INET_ADDR-2);
  sprintf(oldstack,"Resolved connect: %s",p->inet_addr);
  stack=end_string(oldstack);
  log("RESOLVE",oldstack);
#endif
/*
  sprintf(oldstack,"Got connection: %s",p->num_addr);
  stack=end_string(oldstack);
  log("connection.dump",oldstack);

  hp=gethostbyaddr((char *)&(incoming.sin_addr.s_addr),
		   sizeof(incoming.sin_addr.s_addr),
		   incoming.sin_family); 
  if (hp) resolved_address=hp->h_name;
  else resolved_address=p->num_addr;

  strncpy(p->inet_addr,resolved_address,MAX_INET_ADDR-2);

  sprintf(oldstack,"Resolved connection: %s",p->inet_addr);
  stack=end_string(oldstack);
  log("connection.dump",oldstack);
*/
  sscanf(p->num_addr,"%d.%d.%d.%d",&no1,&no2,&no3,&no4);
  if (do_banish(p)) {
      write(new_socket,banish_msg.where,banish_msg.length);
      out_current+=banish_msg.length;
      out_pack_current++;
      destroy_player(p);
      return;
  }
  else
      if (time(0)<splat_timeout && no1==splat1 && no2==splat2) {
	  write(new_socket,splat_msg.where,splat_msg.length);
	  out_current+=banish_msg.length;
	  out_pack_current++;
	  destroy_player(p);
	  return;
      }
      else connect_to_prog(p);
  current_player=0;
}

/* turn on and off echo for passwords */

void password_mode_on(player *p)
{
  p->flags |= PASSWORD_MODE;
  if (!(p->flags&DO_LOCAL_ECHO)) 
    tell_player(p,"\377\373\001");
}

void password_mode_off(player *p)
{
  p->flags &= ~PASSWORD_MODE;
  if (!(p->flags&DO_LOCAL_ECHO)) 
    tell_player(p,"\377\374\001");
}

/* do a backspace */

void backspace(player *p)
{
  p->ibuffer[p->ibuff_pointer]=0;
  if (p->ibuff_pointer>0) p->ibuff_pointer--;
}

/* handle telnet control codes */

void telnet_options(player *p)
{
  unsigned char c;
  if (read(p->fd,&c,1)!=1) return;
  switch(c) {
  case EC:
    backspace(p);
    break;
  case EL:
    p->ibuff_pointer=0;
    break;
  case IP:
    quit(p,0);
    break;
  case DO:
    if (read(p->fd,&c,1)!=1) return;
    switch(c) {
    case TELOPT_ECHO:
      if (!(p->flags&PASSWORD_MODE)) p->flags |= DO_LOCAL_ECHO; /* start local echo */
      break;
    case TELOPT_SGA:
      break;
    case TELOPT_EOR:
      p->flags |= EOR_ON;
      p->flags &= ~IAC_GA_DO;
      tell_player(p,"\377\031");
      break;
    }
    break;
  case DONT:
    if (read(p->fd,&c,1)!=1) return;
    switch(c) {
    case TELOPT_ECHO:
      p->flags &= ~DO_LOCAL_ECHO; /* stop local echo */
      break;
    case TELOPT_SGA:
      break;
    case TELOPT_EOR:
      p->flags &= ~EOR_ON;
      if (p->saved_flags&IAC_GA_ON) p->flags |= IAC_GA_DO;
      break;
    }
    break;
  }
}


/* gets any input from one player */

void get_player_input(player *p)
{
  int chars_ready=0;
  char *oldstack,c;
  
  oldstack=stack;
  
  if (ioctl(p->fd,FIONREAD,&chars_ready) == -1) {
    quit(p,0);
    log("error","PANIC on FIONREAD ioctl");
    perror("SpoooN (socket.c)");
    return;
  }
  if (!chars_ready) {
    if (sys_flags&VERBOSE) 
      if (p->lower_name[0]) {
	sprintf(oldstack,"%s went netdead.",p->name);
	stack=end_string(oldstack);
	log("connection",oldstack);
      }
      else
	log("connection","Connection went netdead on login.");
    quit(p,0);
    stack=oldstack;
    return;
  }

  in_current+=chars_ready;
  in_pack_current++;

  stack=oldstack;
  
  for(;!(p->flags&PANIC) && chars_ready;chars_ready--)
    if (read(p->fd,&c,1)!=1) {
      log("error","Read error on player");
      quit(p,0);
      return;
    }
    else 
      switch(c) {
      case -1:
	p->flags &= ~(LAST_CHAR_WAS_R|LAST_CHAR_WAS_N);
	telnet_options(p);
	return;
	break;
      case '\n':
	if (!(p->flags&LAST_CHAR_WAS_R)) {
	  p->flags |= LAST_CHAR_WAS_N;
	  p->anticrash=0;
	  p->flags |= INPUT_READY;
	  p->ibuffer[p->ibuff_pointer]=0;
	  p->ibuff_pointer=0;
	  p->column=0;
	  return;
	}
	break;
      case '\r':
	if (!(p->flags&LAST_CHAR_WAS_N)) {
	  p->flags |= LAST_CHAR_WAS_R;
	  p->anticrash=0;
	  p->flags |= INPUT_READY;
	  p->ibuffer[p->ibuff_pointer]=0;
	  p->ibuff_pointer=0;
	  p->column=0;
	  return;
	}
	break;
      default:
	p->flags &= ~(LAST_CHAR_WAS_R|LAST_CHAR_WAS_N);
	if (c==8 || c==127 || c==-9) {
	  backspace(p);
	  break;
	}
	if ((c>31) && (p->ibuff_pointer<(IBUFFER_LENGTH-3))) {
	  p->ibuffer[p->ibuff_pointer]=c;
	  p->ibuff_pointer++;
	  if ((!(p->flags&PASSWORD_MODE)) && (p->flags&DO_LOCAL_ECHO)) {
	    if (write(p->fd,&c,1)<0 && errno!=EINTR) {
	      log("error","Echoing back to player.\n");
	      quit(p,0);
	      return;
	    }
	    out_current++;
	    out_pack_current++;
	  }
	}
	else {
	  p->anticrash++;
	  if (p->anticrash>100) {
	    tell_player(p,"\n\nAuto chuckout to prevent piping.\n\n");
	    quit(p,"");
	    return;
	  }
	}
	break;
      }
}


/* this routine is called when idle */

void scan_sockets()
{
  fd_set fset;
  player *scan;
  
  FD_ZERO(&fset);
 
  FD_SET(main_descriptor,&fset);
  for(scan=flatlist_start;scan;scan=scan->flat_next) {
    if (!((scan->fd<0) || (scan->flags&PANIC))) 
      FD_SET(scan->fd,&fset);
  }
  
  if (select(FD_SETSIZE,&fset,0,0,0) == -1) return;
  
  if (FD_ISSET(main_descriptor,&fset)) 
      accept_new_connection();
  for(scan=flatlist_start;scan;scan=scan->flat_next)
    if (!(scan->fd<=0 || scan->flags&(PANIC|INPUT_READY))
	&& FD_ISSET(scan->fd,&fset))
      get_player_input(scan);
}


/* turns a string said to a player into something ready to go
   down the telnet connection */

file process_output(player *p,char *str)
{
  int i,hi=0;
  char *save;
  file o;
    
  o.where=stack;
  o.length=0;
  
  if (p!=current_player) {
    
    if ((command_type&PERSONAL || p->flags&TAGGED) && p->term && 
	!((sys_flags&EVERYONE_TAG) || (sys_flags&ROOM_TAG))) {
      strcpy(stack,terms[(p->term)-1].bold);
      while(*stack) {
	stack++;
	o.length++;
      }
      hi=1;
    }
    
    save=stack;
    if (command_type&ECHO_COM && p->saved_flags&TAG_ECHO) {
      *stack++='+';
      o.length++;
    }
    if ((command_type&PERSONAL ||
	 p->flags&TAGGED) && p->saved_flags&TAG_PERSONAL && 
	!((sys_flags&EVERYONE_TAG) || (sys_flags&ROOM_TAG))) {
	*stack++='>';
	o.length++;
    }
    if ((command_type&EVERYONE || sys_flags&EVERYONE_TAG) && 
	p->saved_flags&TAG_SHOUT) {
      *stack++='!';
      o.length++;
    }
    if ((command_type&ROOM || sys_flags&ROOM_TAG) 
	&& p->saved_flags&TAG_ROOM) {
      *stack++='-';
      o.length++;
    }
    if (command_type&AUTO && p->saved_flags&TAG_AUTOS) {
      *stack++='#';
      o.length++;
    }
    if (stack!=save) {
      *stack++=' ';
      o.length++;
    }
    
    if (command_type&ECHO_COM && p->saved_flags&SEEECHO &&
	(command_type&PERSONAL || (p->location && p->location->flags&OPEN))) {
      sprintf(stack,"[%s] ",current_player->name);
      while(*stack) {
	stack++;
	o.length++;
      }                               
    }

  }

  if ((!hi) && (command_type&HIGHLIGHT) && (p->term)) {
    strcpy(stack,terms[(p->term)-1].bold);
    while(*stack) {
      stack++;
      o.length++;
    }
    hi=1;
  }

  p->column+=o.length;
  
  while(*str) {
    switch(*str) {
    case '\n':
      if (hi) {
	strcpy(stack,terms[(p->term)-1].off);
	while(*stack) {
	  stack++;
	  o.length++;
	}
	hi=0;
      }
      *stack++='\r';
      *stack++='\n';
      p->column=0;
      str++;
      o.length+=2;
      break;
    default:
      if (p->term_width && (p->column>=p->term_width)) {
	for(i=0;i<p->word_wrap;i++,stack--,o.length--) if (isspace(*(--str))) break;
	if (i!=p->word_wrap) {
	  *stack++='\r';
	  *stack++='\n';
	  *stack++=' ';
	  *stack++=' ';
	  *stack++=' ';
	  p->column=3;
	  str++;
	  o.length+=5;
	}
	else {
	  for(;i;stack++,str++,o.length++) i--;
	  *stack++='\r';
	  *stack++='\n';
	  *stack++=' ';
	  *stack++=' ';
	  *stack++=' ';
	  p->column=3;
	  o.length+=5;
	}
      }
      *stack++=*str++;
      o.length++;
      p->column++;
      break;
    }
  }
  return o;
}

/* generic routine to write to one player */

void tell_player(player *p,char *str)
{
  file output;
  char *oldstack,*script;
  oldstack=stack;
  if (((p->fd)<0) || (p->flags&PANIC) || 
      (!(p->location) && current_player!=p)) return;
  if (!(sys_flags&PANIC)) if (!test_receive(p)) return;
  output=process_output(p,str);
  if (p->script) {
    script=stack;
    sprintf(stack,"%s_emergency",p->lower_name);
    stack=end_string(stack);
    log(script,str);
    stack=script;  	
  }
  if (write(p->fd,output.where,output.length)<0 && errno!=EINTR) 
    quit(p,0);
  out_current+=output.length;
  out_pack_current++;
  stack=oldstack;
}

/* small derivative of tell player to save typing */

void tell_current(char *str)
{
  if (!current_player) return;
  tell_player(current_player,str);
}

/* non blockable raw tell */

void non_block_tell(player *p,char *str)
{
  file output;
  char *script,*oldstack;
  oldstack=stack;
  if (((p->fd)<0) || (p->flags&PANIC)) return;
  output=process_output(p,str);
  if (p->script) {
    script=stack;
    sprintf(stack,"%s_emergency",p->lower_name);
    stack=end_string(stack);
    log(script,str);
    stack=script;  	
  }
  if (write(p->fd,output.where,output.length)<0 && errno!=EINTR) 
    quit(p,0);
  out_current+=output.length;
  out_pack_current++;
  stack=oldstack;
}


/* general routine to send a prompt */

void do_prompt(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;
  strcpy(stack,str);
  stack=strchr(stack,0);;
  if (p->flags&IAC_GA_DO) {
    *stack++=IAC;
    *stack++=GA;
  }
  if (p->flags&EOR_ON) {
    *stack++=IAC;
    *stack++=EOR;
  }
  *stack++=0;
  tell_player(p,oldstack);
  stack=oldstack;
}

