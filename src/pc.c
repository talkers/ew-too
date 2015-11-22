/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* 
  pc.c

  During early development EW-too used to be able to run on a DOS
  PC with djgpp in single user mode. This code is pretty useless now.
*/

#define PC_FILE

#include <ctype.h>

#include "config.h"
#include "player.h"

/* externs */

extern file load_file();
extern char *end_string(),*do_pipe();
extern player *create_player();
extern void destroy_player(),connect();
extern int test_receive();

/* interns */

int main_descriptor;
file banish_file,banish_msg,full_msg;
void tell_player();

player *input_player;


/* a tolower kludge */

char mytolower(char c)
{
  static change='a'-'A';
  if (isupper(c)) c+=change;
  return c;
}


/* close down sockets after use */

void close_down_socket()
{
/*  close(main_descriptor); */
}


/* grab the main socket */

void init_socket(int port)
{

/* drag in all the msg files */

  banish_file=load_file("files\\banish");
  banish_msg=load_file("files\\banish.msg");
  full_msg=load_file("files\\full.msg");

}

/* accept new connection on the main socket */

void accept_new_connection()
{
  player *p;
  int new_socket;

  new_socket=current_players+1;
  
  if (current_players==max_players) {
    write(new_socket,full_msg.where,full_msg.length);
    return;
  }
  p=create_player();
  input_player=p;
  current_player=p;
  p->fd=new_socket;
  strncpy(p->inet_addr,"Internal Test",MAX_INET_ADDR);
  connect(p);
  current_player=0;
}

/* dummy functions for echoing */

void password_mode_on(player *p)
{
}

void password_mode_off(player *p)
{
}


/* this routine is called when idle */

void scan_sockets()
{
  player *p;
  p=input_player;

  gets(stack);
  strncpy(p->ibuffer,stack,IBUFFER_LENGTH);
  p->anticrash=0;
  p->flags |= INPUT_READY;
  p->column=0;
}


/* turns a string said to a player into something ready to go
   down the telnet connection */

file process_output(player *p,char *str)
{
  int i;
  file o;
    
  o.where=stack;
  o.length=0;

	if (command_type&ECHO_COM && p->saved_flags&TAG_ECHO) *stack++='+';
	if (command_type&PERSONAL && p->saved_flags&TAG_PERSONAL) *stack++='>';
  if (command_type&EVERYONE && p->saved_flags&TAG_SHOUT) *stack++='!';	
  if (command_type&ROOM && p->saved_flags&TAG_ROOM) *stack++='-';
  if (stack!=o.where) {
  	*stack++=' ';
  	o.length+=2;
  }

  if (p!=current_player) {
    sprintf(stack,"<%d> ",p->fd);
    while(*stack) {
			stack++;
			o.length++;
		}
  }
  
 	if (command_type&ECHO_COM && (command_type&PERSONAL ||
			(p->saved_flags&SEEECHO && p->residency&SEE_ECHO))) {
		sprintf(stack,"[%s] ",current_player->name);
		while(*stack) {
			stack++;
			o.length++;
		}				
	}
	
  while(*str) {
    if (p->term_width && (p->column>=p->term_width)) {
      for(i=0;i<p->word_wrap;i++,stack--,o.length--) if (isspace(*(--str))) break;
      if (i!=p->word_wrap) {
	*stack++='\r';
	*stack++='\n';
	p->column=0;
	str++;
	o.length+=2;
      }
      else {
	for(;i;stack++,str++,o.length++) i--;
	*stack++='\r';
	*stack++='\n';
	p->column=0;
	o.length+=2;
      }
    }
    switch(*str) {
    case '\n':
      *stack++='\r';
      *stack++='\n';
      p->column=0;
      str++;
      o.length+=2;
      break;
    default:
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
  if (((p->fd)<0) || (p->flags&PANIC)) return;
  if (!(sys_flags&PANIC)) if (!test_receive(p)) return;
  output=process_output(p,str);
  if (p->script) {
  	script=stack;
  	sprintf(stack,"%s_emergency",p->lower_name);
  	stack=end_string(stack);
  	log(script,str);
		stack=script;  	
  }
  if (write(0,output.where,output.length)<0) {
    p->flags |= PANIC;
    log("error","PANIC trying to write to player.");
  }
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
  if (write(0,output.where,output.length)<0) {
    p->flags |= PANIC;
    log("error","PANIC trying to write to player.");
  }
  stack=oldstack;
}


/* create and switch pseudo players */

void psuedo_person(player *p,char *str)
{
  accept_new_connection();  
}


void switch_person(player *p,char *str)
{
  int new=0;
  player *scan;
  new=atoi(str);
  if (!new) {
    tell_player(p,"Argument is a number.\n");
    return;
  }
  for(scan=flatlist_start;scan;scan=scan->flat_next)
    if (scan->fd==new) {
      tell_player(p,"Switching to player.\n");
      input_player=scan;
      return;
    }
  tell_player(p,"No such player.\n");
}
