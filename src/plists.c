/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/


/*
   Plists.c
*/

#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <memory.h>

#include "fix.h"
#include "config.h"
#include "player.h"
#include "compaction.c"

#ifndef S_IRUSR
#define S_IRUSR 00400
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200
#endif

#ifdef MIPS
extern char *strstr(char *,char *);
#endif

 /* externs */

extern player *find_player_absolute_quiet(char *);
extern char *gstring(player *);
extern void do_inform(player *,char *);
extern void do_prompt(player *,char *);
extern void quit(player *,char *);
extern void finish_edit(player *);
extern int restore_player_title(player *,char *,char *);
extern void decompress_list(saved_player *);
#ifndef LINUX
extern char *crypt(char *,char *);
#endif
extern void player_flags(player *),player_flags_verbose(player *,char *);
extern file load_file();
extern char *end_string(),*convert_time(),*retrieve_room_data(),
  *retrieve_list_data(),*bit_string(),*retrieve_mail_data();
extern room *create_room(),*convert_room();
extern void password_mode_on(),password_mode_off(),trans_to(),leave_sub();
extern void construct_room_save(),construct_list_save(),construct_mail_save();
extern void decompress_room(room *);
extern void log(char *,char *);
extern void handle_error(char *);
extern void free_room_data(saved_player *);
extern void tell_player(player *,char *);
extern void tell_room(room *,char *);
extern void swho(player *,char *);
extern void pager(player *,char *,int);
extern int possible_move(player *,room *,int);

extern player *p_sess;
extern file banished_p,full_msg;

 /* interns */

file motd_msg,connect_msg,newban_msg,banned_msg,nonewbies_msg,
  newbie_msg,newpage1_msg,newpage2_msg,disclaimer_msg,splat_msg;

saved_player **saved_hash[26];
int update[26];
void save_player(),newbie_check_name(),link_to_program();
int restore_player();
int banished_check(char *);

saved_player **birthday_list=0;

 /* update functions .. a complete database traversal */

/*  crypts the password */

char *update_password(char *oldpass)
{
    char key[9];
    strncpy(key,oldpass,8);
    return crypt(key,"SP");
}

void players_update_function(player *p)
{    
    if (p->residency&ADMIN)
	printf("%-18s -- %-40s >(ADMIN)\n",p->name,p->email);
    else
	if (p->residency&UPPER_SU)
	    printf("%-18s -- %-40s >(UPPER)\n",p->name,p->email);
	else
	    if (p->residency&LOWER_SU)
		printf("%-18s -- %-40s >(BASIC)\n",p->name,p->email);
	    else
		printf("%-18s -- %s\n",p->name,p->email); 
}

void flags_update_function(player *p)
{
	printf("%-18s -- %-40s\n",p->name,bit_string(p->residency));
}

void rooms_update_function(player *p)
{
  room *r;
  saved_player *sp;
  sp=p->saved;
  r=sp->rooms;
  while(r) {
    if (r->flags&OPEN) {
      decompress_room(r);
      printf("-=> %s.%s (%s)\n",r->owner->lower_name,r->id,r->name);
      if (r->exits.where) printf(r->exits.where);
    }
    r=r->next;
  }
}


void do_update(int rooms)
{
    player *p;
    saved_player *scan,**hash;
    int i,j,fd;
  
    fd=open("/dev/null",O_WRONLY);
  
    p=(player *)MALLOC(sizeof(player));
  
    for(j=0;j<26;j++) {
	hash=saved_hash[j];
	for(i=0;i<HASH_SIZE;i++,hash++)
	    for(scan=*hash;scan;scan=scan->next) {
	      p->fd=fd;
	      p->script=0;
	      p->location=(room *)-1;
	      restore_player(p,scan->lower_name);
	      players_update_function(p);
	    }
    }
    close(fd);
}
/*
		if (scan->residency!=STANDARD_ROOMS &&
		    (scan->residency!=BANISHED)) {
		    memset((char *)p,0,sizeof(player));
		    p->fd=fd;
		    p->script=0;
		    p->location=(room *)-1;
		    restore_player(p,scan->lower_name);
		    if (rooms)
			rooms_update_function(p);
		    else if (sys_flags&UPDATE)
			players_update_function(p);
		    else
			flags_update_function(p);	
		}
    }
    close(fd);
}
*/

/* return the top player in a hash list */

saved_player *find_top_player(char c,int h)
{
  if ((c<0) || (c>25)) return 0;
  if ((h<0) || (h>HASH_SIZE)) return 0;
  return (*(saved_hash[c]+h));
}


/* birthdays !!! (which are not implemented) 

void do_birthdays()
{
  player *p;
  saved_player *scan,**hash,**list;
  int i,j,fd;
  time_t t;
  struct tm *date,*bday;
  char *oldstack;
  
  fd=open("/dev/null",O_WRONLY);
  
  oldstack=stack;
  align(stack);
  list=(saved_player **)stack;
  
  t=time(0);
  date=localtime(&t);
  
  p=(player *)MALLOC(sizeof(player));
  
  for(j=0;j<26;j++) {
    hash=saved_hash[j];
    for(i=0;i<HASH_SIZE;i++,hash++)
      for(scan=*hash;scan;scan=scan->next)
	if (scan->residency!=STANDARD_ROOMS &&
	    (scan->residency!=BANISHED)) {
	  memset((char *)p,0,sizeof(player *));
 	  restore_player(p,scan->lower_name);
	  bday=localtime(&(p->birthday));
	  if ((bday->tm_mon==date->tm_mon) &&
	      (bday->tm_mday==date->tm_mday)) {
	    *((saved_player **)stack)=scan;
	    stack+=sizeof(saved_player *);
	    p->age++;
	    save_player(p);
	  }
	}
  }
  *((saved_player **)stack)=0;
  stack+=sizeof(saved_player *);
  if (birthday_list) FREE(birthday_list);
  
  i=(int)stack-(int)list;
  if (i>4) {
    birthday_list=(saved_player **)MALLOC(i);
    memcpy(birthday_list,list,i);
  }
   else birthday_list=0;
  
  close(fd);
}
*/

 /* saved player stuff */

 /* see if a saved player exists (given lower case name) */

saved_player *find_saved_player(char *name)
{
    saved_player **hash,*list;
    int sum=0,h;
    char *c;
    if (!isalpha(*name)) return 0;
    hash=saved_hash[((int)(tolower(*name))-(int)'a')];
    for(c=name;*c;c++) {
	if (isalpha(*c)) sum+=(int)(tolower(*c))-(int)'a';
	else return 0;
    }
    list=*(hash+(sum%HASH_SIZE));
    for(;list;list=list->next) if (!strcmp(name,list->lower_name)) return list;
    return 0;
}


 /* hard load and save stuff (ie to disk and back) */


 /* extract one player */

void extract_player(char *where,int length)
{
    int len,sum,rlength,llength;
    char *oldstack,*c,*copy;
    saved_player *old,*sp,**hash;
    oldstack=stack;
    where=get_int(&len,where);
    where=get_string(oldstack,where);
    stack=end_string(oldstack);
    old=find_saved_player(oldstack);
    sp=old;
    if (!old) {
	sp=(saved_player *)MALLOC(sizeof(saved_player));
	memset((char *)sp,0,sizeof(saved_player));
	strncpy(sp->lower_name,oldstack,MAX_NAME);
	sp->rooms=0;
	sp->mail_sent=0;
	sp->mail_received=0;
	sp->list_top=0;
	hash=saved_hash[((int)sp->lower_name[0]-(int)'a')];
	for(sum=0,c=sp->lower_name;*c;c++) sum+=(int)(*c)-'a';
	hash=(hash+(sum%HASH_SIZE));
	sp->next=*hash;
	*hash=sp;
    }
    where=get_int(&sp->last_on,where);
    where=get_int(&sp->saved_flags,where);
    where=get_int(&sp->residency,where);

    if (sp->residency==BANISHED) {
	sp->last_host[0]=0;
	sp->data.where=0;
	sp->data.length=0;
	stack=oldstack;
	return;
    }

/*
      This bit controls when idle players get wiped.
*/

    if (((time(0)-(sp->last_on))>PLAYER_TIMEOUT) && 
	!(sp->residency&NO_TIMEOUT)) {
	log("timeouts",sp->lower_name);
       	remove_player_file(sp->lower_name);  
	stack=oldstack;
	return;
    }

    where=get_string(sp->last_host,where);
    where=get_int(&sp->data.length,where);
    sp->data.where=(char *)MALLOC(sp->data.length);
    memcpy(sp->data.where,where,sp->data.length);
    where+=sp->data.length;
    copy=where;
    where=retrieve_room_data(sp,where);
    rlength=(int)where-(int)copy;
    copy=where;
    where=retrieve_list_data(sp,where);
    llength=(int)where-(int)copy;
    where=retrieve_mail_data(sp,where);

/*    printf("%d\t%d\t%d\t\t%s\n",length,rlength,llength,sp->lower_name); */
    stack=oldstack;
}

 /* hard load in on player file */

void hard_load_one_file(char c)
{
    char *oldstack,*where,*scan;
    int fd,length,len2,i;
    oldstack=stack;

    if (sys_flags&VERBOSE) {
	sprintf(oldstack,"Loading player file '%c'.",c);
	stack=end_string(oldstack);
	log("boot",oldstack);
	stack=oldstack;
    }
#ifdef PC
    sprintf(oldstack,"files\\players\\%c",c);
    fd=open(oldstack,O_RDONLY|O_BINARY);
#else
    sprintf(oldstack,"files/players/%c",c);
    fd=open(oldstack,O_RDONLY|O_NDELAY);
#endif
    if (fd<0) {
	sprintf(oldstack,"Failed to load player file '%c'",c);
	stack=end_string(oldstack);
	log("error",oldstack);
    }
    else {
	length=lseek(fd,0,SEEK_END);
	lseek(fd,0,SEEK_SET);
	if (length) {
	    where=(char *)MALLOC(length);
	    if (read(fd,where,length)<0) 
		handle_error("Can't read player file.");
	    for(i=0,scan=where;i<length;) {
		get_int(&len2,scan);
		extract_player(scan,len2);
		i+=len2;
		scan+=len2;
	    }
	    FREE(where);
	}
	close(fd);
    }
    stack=oldstack;
}


 /* load in all the player files */

void hard_load_files()
{
    char c;
    int i,hash_length;
    char *oldstack;
    oldstack=stack;
    hash_length=HASH_SIZE*sizeof(saved_player *);
    for(i=0;i<26;i++) {
	saved_hash[i]=(saved_player **)MALLOC(hash_length);
	memset((void *)saved_hash[i],0,hash_length);
    }
    for(c='a';c<='z';c++) hard_load_one_file(c); 
}


 /* write one player file out */

void write_to_file(saved_player *sp)
{
    char *oldstack;
    int length;
    oldstack=stack;
    if (sys_flags&VERBOSE && sys_flags&PANIC) {
	sprintf(oldstack,"Attempting to write player '%s'.",sp->lower_name);
	stack=end_string(oldstack);
	log("sync",oldstack);
	stack=oldstack;
    }
    stack+=4;
    stack=store_string(stack,sp->lower_name);
    stack=store_int(stack,sp->last_on);
    stack=store_int(stack,sp->saved_flags|COMPRESSED_LIST);
    stack=store_int(stack,sp->residency);
    if (sp->residency!=BANISHED) {
	stack=store_string(stack,sp->last_host);
	stack=store_int(stack,sp->data.length);
	memcpy(stack,sp->data.where,sp->data.length);
	stack+=sp->data.length;
	construct_room_save(sp);
	construct_list_save(sp);
	construct_mail_save(sp);
    }
    length=(int)stack-(int)oldstack;
    (void) store_int(oldstack,length);

}

 /* sync player files corresponding to one letter */

void sync_to_file(char c,int background)
{
  saved_player *scan,**hash;
  char *oldstack;
  int fd,i,length;
  
  if (background && fork()) return;
    
  oldstack=stack;
  if (sys_flags&VERBOSE) {
    sprintf(oldstack,"Syncing File '%c'.",c);
     stack=end_string(oldstack);
    log("sync",oldstack);
    stack=oldstack;
  }
  hash=saved_hash[((int)c-(int)'a')];
  for(i=0;i<HASH_SIZE;i++,hash++)
    for(scan=*hash;scan;scan=scan->next)
      if (scan->residency!=STANDARD_ROOMS &&
	  (!(scan->residency&NO_SYNC) || scan->residency==BANISHED))
	write_to_file(scan);
  length=(int)stack-(int)oldstack;


  /* test that you can write out a file ok */

  strcpy(stack,"files/players/backup_write");
  fd=open(stack,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
  if (fd<0) handle_error("Primary open failed (player back)");
  if (write(fd,oldstack,length)<0) handle_error("Primary write failed "
						"(playerback)");
  close(fd);

#ifdef PC
  sprintf(stack,"files\\players\\%c",c);
  fd=open(stack,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY);
#else
  sprintf(stack,"files/players/%c",c);
  fd=open(stack,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
#endif
  if (fd<0) handle_error("Failed to open player file.");
  if (write(fd,oldstack,length)<0)
    handle_error("Failed to write player file.");
   close(fd);
  update[(int)c-(int)'a']=0;
  stack=oldstack;

  if (background) exit(0);

}


 /* sync everything to disk */

void sync_all()
{
    char c,*oldstack;
    oldstack=stack;
    for(c='a';c<='z';c++) sync_to_file(c,0);
    log("sync","Full Sync Completed.");
}

 /* fork and sync the playerfiles */

void fork_the_thing_and_sync_the_playerfiles(void)
{
    int fl;
    fl=fork();
    fl==0;
    if (fl==-1) {
	log("error","Forked up!");
	return;
    }
    if (fl>0) return;
    sync_all();
    exit(0);
}

 /* flicks on the update flag for a particular player hash */

void set_update(char c)
{
    update[(int)c-(int)'a']=1;
}


 /* removes an entry from the saved player lists */

int remove_player_file(char *name)
{
    saved_player *previous=0,**hash,*list;
    char *c;
    int sum=0;

    if (!isalpha(*name)) {
	log("error","Tried to remove non-player from save files.");
	return 0;
    }
    strcpy(stack,name);
    lower_case(stack);

    hash=saved_hash[((int)(*stack)-(int)'a')];
    for(c=stack;*c;c++) 
	if (isalpha(*c)) sum+=(int)(*c)-'a';
	else {
	    log("error","Remove bad name from save files");
	    return 0;
	}

    hash+=(sum%HASH_SIZE);
    list=*hash;
    for(;list;previous=list,list=list->next) 
	if (!strcmp(stack,list->lower_name)) {
	    if (previous) previous->next=list->next;
	    else *hash=list->next;
	    if (list->data.where) FREE(list->data.where);
	    if (list->mail_received) FREE(list->mail_received);
	    free_room_data(list);
	    FREE((void *)list);
	    set_update(*stack);
	    return 1;
	}
    return 0;
}

 /* remove an entire hash of players */

void remove_entire_list(char c)
{
    saved_player **hash,*sp,*next;
    int i;
    if (!isalpha(c)) return;
    hash=saved_hash[((int)(c)-(int)'a')];
    for(i=0;i<HASH_SIZE;i++,hash++) { 
	sp=*hash;
	while(sp) {
	    next=sp->next;
	    if (sp->data.where) FREE(sp->data.where);
	    free_room_data(sp);
	    FREE((void *)sp);
	    sp=next;
	}
	*hash=0;
    }
    set_update(c);
}

 /* routines to save a player to the save files */

 /* makes the save data onto the stack */

file construct_save_data(player *p)
{
    file d;
    d.where=stack;

    stack=store_string(stack,p->name);
    stack=store_string(stack,p->prompt);
    stack=store_string(stack,p->converse_prompt);

/* make sure this doesnt break */

    if (p->email[0]==-1) p->email[1]==0;

    stack=store_string(stack,p->email);
    if (p->password[0]==-1) p->password[0]=0;
    stack=store_string(stack,p->password);
    stack=store_string(stack,p->title);
    stack=store_string(stack,p->plan);
    stack=store_string(stack,p->description);
    stack=store_string(stack,p->enter_msg);
    stack=store_string(stack,p->pretitle);

    stack=store_string(stack,p->ignore_msg);   
    stack=store_string(stack,p->room_connect);

    stack=store_int(stack,p->term_width);
    stack=store_int(stack,p->word_wrap);
    stack=store_int(stack,p->max_rooms);
    stack=store_int(stack,p->max_exits);
    stack=store_int(stack,p->max_autos);
    stack=store_int(stack,p->max_list);
    stack=store_int(stack,p->max_mail);
    stack=store_int(stack,p->gender);
    stack=store_int(stack,p->no_shout);
    stack=store_int(stack,p->total_login);
    stack=store_int(stack,p->term);
    stack=store_int(stack,p->birthday);
    stack=store_int(stack,p->age); 

/*   Here goes with adding to the playerfiles */

    stack=store_int(stack,p->jetlag);  
    stack=store_int(stack,p->squished); 

    d.length=(int)stack-(int)d.where;
    stack=d.where;
    return d;
}

 /* the routine that sets everything up for the save */

void save_player(player *p)
{
    saved_player *old,**hash,*sp;
    int sum;
    file data;
    char *c;

    if (!(p->location) || !(p->name[0]) || 
	p->residency==NON_RESIDENT) return;
   
    if (sys_flags&PANIC) {
	c=stack;
	sprintf(c,"Attempting to save player %s.",p->name);
	stack=end_string(c);
	log("boot",c);
	stack=c;
    }

    if (!(isalpha(p->lower_name[0]))) {
	log("error","Tried to save non-player.");
	return ;
    }
    if (!(p->password[0])) {
	tell_player(p,"Tried to save character but failed ...\n"
		    "Your character will not save until you set a password.\n"
		    "Simply type 'password' whilst in command mode to set one.\n");
	p->residency |= NO_SYNC;
    } 
    if (!(p->email[0])) {
	tell_player(p,"Tried to save character but failed ...\n"
		    "Your character will not save until you set an email address.\n"
		    "To set this just type 'email <whatever>', where <whatever> is your\n"
		    "email address.\n"
		    "If you do not have an email, please speak to one of the superusers.\n");
	p->residency |= NO_SYNC;
    }
  
    old=p->saved;
    sp=old;
    if (!old) {
	sp=(saved_player *)MALLOC(sizeof(saved_player));
	memset((char *)sp,0,sizeof(saved_player));
	strncpy(sp->lower_name,p->lower_name,MAX_NAME);
	sp->rooms=0;
	sp->mail_sent=0;
	sp->mail_received=0;
	sp->list_top=0;
	hash=saved_hash[((int)p->lower_name[0]-(int)'a')];
	for(sum=0,c=p->lower_name;*c;c++) 
	    if (isalpha(*c)) sum+=(int)(*c)-'a';
	    else {
		tell_player(p,"Eeek, trying to save bad player name !!\n");
		FREE(sp);
		return;
	    }
	hash=(hash+(sum%HASH_SIZE));
	sp->next=*hash;
	*hash=sp;
	p->saved=sp;
	sp->saved_flags = p->saved_flags;
	create_room(p);
    }
    data=construct_save_data(p);
    if (!data.length) {
	log("error","Bad construct save.");
	return;
    }
    if (old && sp->data.where) FREE((void *)sp->data.where);
    sp->data.where=(char *)MALLOC(data.length);
    sp->data.length=data.length;
    memcpy(sp->data.where,data.where,data.length);
    sp->residency=p->saved_residency;
    sp->saved_flags=p->saved_flags;
    strncpy(sp->last_host,p->inet_addr,MAX_INET_ADDR);
    set_update(*(sp->lower_name));
    p->saved=sp;
    if (!(p->flags&NO_SAVE_LAST_ON))
	sp->last_on=time(0); 
    tell_player(p,"Character Saved ...\n");
}

/* the routine that sets everything up for the save */

void create_banish_file(char *name)
{
  saved_player **hash,*sp,*scan;
  int sum;
  char *c;

  sp=(saved_player *)MALLOC(sizeof(saved_player));
  memset((char *)sp,0,sizeof(saved_player));
  strcpy(stack,name);
  lower_case(stack);
  strncpy(sp->lower_name,stack,MAX_NAME-2);
  sp->rooms=0;
  sp->mail_sent=0;
  sp->mail_received=0;
  hash=saved_hash[((int)name[0]-(int)'a')];
  for(sum=0,c=name;*c;c++) 
    if (isalpha(*c)) sum+=(int)(*c)-'a';
    else {
      log("error","Tried to banish bad player");
      FREE(sp);
      return;
    }
  hash=(hash+(sum%HASH_SIZE));
  scan=*hash;
  while(scan) {
    hash=&(scan->next);
    scan=scan->next;
  }
  *hash=sp;
  sp->residency=BANISHED;
  sp->saved_flags=0;
  sp->last_host[0]=0;
  sp->last_on=time(0);
  sp->next=0;
  set_update(tolower(*(sp->lower_name)));
}


/* load from a saved player into a current player */

/* actually do load */

int load_player(player *p)
{
  saved_player *sp;
  char *r;
  sp=find_saved_player(p->lower_name);
  p->saved=sp;
  if (!sp) return 0;
 
  p->residency=sp->residency;

  p->saved_residency=p->residency;
  p->saved_flags=sp->saved_flags;
  if (sp->residency==BANISHED || sp->residency==STANDARD_ROOMS) return 1;

  r=sp->data.where;
  r=get_string(p->name,r);
  r=get_string(p->prompt,r);
  r=get_string(p->converse_prompt,r);
  r=get_string(p->email,r);
  r=get_string(p->password,r);
  r=get_string(p->title,r);
  r=get_string(p->plan,r);
  r=get_string(p->description,r);
  r=get_string(p->enter_msg,r);
  r=get_string(p->pretitle,r);

  r=get_string(p->ignore_msg,r);     
  r=get_string(p->room_connect,r); 

  r=get_int(&p->term_width,r);
  r=get_int(&p->word_wrap,r);
  r=get_int(&p->max_rooms,r);
  r=get_int(&p->max_exits,r);
  r=get_int(&p->max_autos,r);
  r=get_int(&p->max_list,r);
  r=get_int(&p->max_mail,r);
  r=get_int(&p->gender,r);
  r=get_int(&p->no_shout,r);
  r=get_int(&p->total_login,r);
  r=get_int(&p->term,r);
  r=get_int(&p->birthday,r);
  r=get_int(&p->age,r); 

  r=get_int(&p->jetlag,r);  
  r=get_int(&p->squished,r);  

  if (((p->term_width)>>1)<=(p->word_wrap)) p->word_wrap=(p->term_width)>>1;

  decompress_list(sp);
  p->saved_flags=sp->saved_flags;
  return 1;  
}

/* load and do linking */

int restore_player(player *p,char *name)
{
    return restore_player_title(p,name,0);
}

int restore_player_title(player *p,char *name,char *title)
{
  int did_load;
  
  strncpy(p->name,name,MAX_NAME-2);
  strncpy(p->lower_name,name,MAX_NAME-2);
  lower_case(p->lower_name);
  if (!strcmp(p->name,p->lower_name)) p->name[0]=toupper(p->name[0]);
    strncpy(p->prompt,"->",MAX_PROMPT);
  strncpy(p->converse_prompt,"(Converse) ->",MAX_PROMPT);
  
  strcpy(p->enter_msg,"enters in a standard kind of way.");

  
  p->term_width=79;
  p->column=0;
  p->word_wrap=10;

  p->total_login=0;
  
  p->gender=VOID_GENDER;
  p->no_shout=180;

  p->saved_flags=PRIVATE_EMAIL|TAG_ECHO|MAIL_INFORM|IAC_GA_ON;

  strncpy(p->title,"the newbie, so treat me nicely.",MAX_TITLE);
  strncpy(p->description,"Isn't it time I wrote my own description ?",
	  MAX_DESC);
  strncpy(p->plan,"I must write myself a proper plan sometime ...",MAX_PLAN);

  p->max_rooms=3;
  p->max_exits=10;
  p->max_autos=10;
  p->max_list=20;
  p->max_mail=10;

  p->birthday=0;
  p->age=0;
  p->jail_timeout=0;

  p->script=0;

  p->residency=0;

  strcpy(p->ignore_msg,"");
  p->jetlag=0;

  did_load=load_player(p);

  if (title && *title) {
      strncpy(p->title,title,MAX_TITLE);
      p->title[MAX_TITLE]=0;
  }
  
  if ((p->saved_flags&IAC_GA_ON) && (!(p->flags&EOR_ON))) 
    p->flags |= IAC_GA_DO;
  else p->flags &= ~IAC_GA_DO;

  if (p->residency!=BANISHED && p->residency!=STANDARD_ROOMS) {
      p->residency&=~((1<<16)|(1<<17)|(1<<18)|(1<<19)|(1<<21)|(1<<24));
      if (p->residency&UPPER_SU)
	  p->residency&=~LOWER_SU;
      if (p->residency&ADMIN)
	  p->residency&=~(LOWER_SU|UPPER_SU);
  }

  /* got to have someone here I'm afraid..  */

  if (!strcmp("admin",p->lower_name)) 
      p->residency=(-1^(1<<31))&(~NO_SYNC);
      
/* integrity .. sigh */

  p->saved_residency=p->residency;

/*  if (p->max_rooms>50 || p->max_rooms<0) p->max_rooms=3;
  if (p->max_exits>50 || p->max_exits<0) p->max_exits=10;
  if (p->max_autos>50 || p->max_autos<0) p->max_autos=10;
  if (p->max_list>100 || p->max_list<0) p->max_list=20;
  if (p->max_mail>100 || p->max_mail<0) p->max_mail=10;
  if (p->term_width>200 || p->term_width<0) p->term_width=79; */
  if ((p->word_wrap)>((p->term_width)>>1) || p->word_wrap<0) 
      p->word_wrap=(p->term_width)>>1;
  if (p->term>10) p->term=0;
  
  return did_load;
}


/* current player stuff */

/* create an abstract player into the void hash list */

player *create_player()
{
  player *p;

  p=(player *)MALLOC(sizeof(player));
  memset((char *)p,0,sizeof(player));

  if (flatlist_start)
      flatlist_start->flat_previous=p;
  p->flat_next=flatlist_start;
  flatlist_start=p;

  p->hash_next=hashlist[0];
  hashlist[0]=p;
  p->hash_top=0;
  p->timer_fn=0;
  p->timer_count=-1;
  return p;
}

/* unlink p from all the lists */

void punlink(player *p)
{
  player *previous,*scan;

/* reset the session p */

  p_sess=0;
  
/* first remove from the hash list */
  
  scan=hashlist[p->hash_top];
  previous=0;
  while(scan && scan!=p) {
    previous=scan;
    scan=scan->hash_next;
  }
  if (!scan) log("error","Bad hash list");
  else 
    if (!previous)
      hashlist[p->hash_top]=p->hash_next;
    else
      previous->hash_next=p->hash_next;
  
  /* then remove from the flat list */
  
  if (p->flat_previous) 
    p->flat_previous->flat_next=p->flat_next;
  else
    flatlist_start=p->flat_next;
  if (p->flat_next)
    p->flat_next->flat_previous=p->flat_previous;
  
  /* finally remove from the location list */
  
  if (p->location) {
    previous=0;
    scan=p->location->players_top;
    while(scan && scan!=p) {
      previous=scan;
      scan=scan->room_next;
    }
    if (!scan) log("error","Bad Location list");
    else 
      if (!previous)
	p->location->players_top=p->room_next;
      else
	previous->room_next=p->room_next;
  }
}

/* remove a player from the current hash lists */

void destroy_player(player *p)
{
#ifndef PC
  if ((p->fd)>0) close(p->fd);
#endif
  if (p->name[0] && p->location) current_players--;
  punlink(p);
  if (p->edit_info) finish_edit(p);
  FREE(p);
#ifdef PC
  if (p==input_player) {
    input_player=flatlist_start;
    if (!input_player) sys_flags |= SHUTDOWN;
  }
#endif
}

/* get person to agree to disclaimer */

void agree_disclaimer(player *p,char *str)
{
  p->input_to_fn=0;
  if ((!strcmp(str,"continue")) || (!strcmp(str,"CONTINUE"))) {
    p->saved_flags |= AGREED_DISCLAIMER;
    if (p->saved) p->saved->saved_flags |= AGREED_DISCLAIMER;
    link_to_program(p);
    return;
  }
  tell_player(p,"\nDisconnecting from program.\n\n");
  quit(p,"");
}

/* links a person into the program properly */

void link_to_program(player *p)
{
    char *oldstack;
    saved_player *sp;
    player *search,*previous,*scan;
    room *r,*rm;
    int hash;
    time_t t;
    struct tm *log_time;
    oldstack=stack;
  
  
    search=hashlist[((int)(p->lower_name[0]))-(int)'a'+1];
    for(;search;search=search->hash_next) 
	if (!strcmp(p->lower_name,search->lower_name)) 
	    if (p->residency==NON_RESIDENT) {
		tell_player(p,"\nSorry there is already someone on the "
			    "program with that name.\nPlease try again, "
			    "but use a different name.\n\n");
		quit(p,"");
		return;
	    }
	    else {
		tell_player(p,"\nYou were already on the program !!\n\n"
			    "Ejecting old character.\n\n");
		p->total_login=search->total_login;
		search->flags |= RECONNECTION;
		p->flags |= RECONNECTION;
	  
		if (search->location) {
		    previous=0;
		    scan=search->location->players_top;
		    while(scan && scan!=search) {
			previous=scan;
			scan=scan->room_next;
		    }
 		    if (!scan) log("error","Bad Location list");
		    else 
			if (!previous)
			    search->location->players_top=search->room_next;
			else
			    previous->room_next=search->room_next;
		}
		search->location=0;
		quit(search,0);
	    }
  
  
    /* do the disclaimer biz  */

    if (!(p->saved_flags&AGREED_DISCLAIMER)) {
	if (!(p->saved && p->password[0]==0)) {
	    tell_player(p,disclaimer_msg.where);
	    do_prompt(p,"Enter 'continue' or 'end':");
	    p->input_to_fn=agree_disclaimer;
	    return;
	}
    }

    /* remove player from non name hash list */
  
    previous=0;
    scan=hashlist[0];
    while(scan && scan!=p) {
	previous=scan;
	scan=scan->hash_next;
    }
    if (!scan) log("error","Bad non-name hash list");
    else 
	if (!previous)
	    hashlist[0]=p->hash_next;
	else
	    previous->hash_next=p->hash_next;  

    /* now place into named hashed lists */
  
    hash=(int)(p->lower_name[0])-(int)'a'+1;
    p->hash_next=hashlist[hash];
    hashlist[hash]=p;
    p->hash_top=hash;
    
    if (p->saved_flags&LOG_CONNECTION) {
	sprintf(oldstack,"Login %s @ %s",p->name,p->inet_addr);
	stack=end_string(oldstack);
	log("connection",oldstack);
	stack=oldstack;
    }
    if (p->flags&SITE_LOG) log("site",oldstack);

    t=time(0);
    log_time=localtime(&t);

/*
    if ((log_time->tm_hour>22) || (log_time->tm_hour<5))
	log("night",oldstack);
*/
    p->flags |= PROMPT;
    p->timer_fn=0;
    p->timer_count=-1;

    if (p->residency!=NON_RESIDENT) {
	sp=p->saved;
	tell_player(p,motd_msg.where);
    }
    else 
	tell_player(p,newbie_msg.where);
    current_players++;
    p->on_since=time(0);
    logins++;

    p->shout_index=50;
    if (p->residency!=NON_RESIDENT) 
	player_flags(p);

    if (p->saved_flags&TRANS_TO_HOME || *p->room_connect) {
	sp=p->saved;
	if (!sp)
	    tell_player(p,"Double Eeek (room_connect)!\n");
	else {
	    if (p->saved_flags&TRANS_TO_HOME) {
		for(r=sp->rooms;r;r=r->next)
		    if (r->flags&HOME_ROOM) {
			sprintf(oldstack,"%s.%s",r->owner->lower_name,r->id);
			stack=end_string(oldstack);
			trans_to(p,oldstack);
			break;
		    }
	    }
	    else {
		rm=convert_room(p,p->room_connect);
		if (rm && rm->flags&CONFERENCE && possible_move(p,rm,1))
		    trans_to(p,p->room_connect);
	    }
	    if (!(p->location))
		tell_player(p,"-=> Tried to connect you to a room, but failed"
			    " !!\n\n");
	}
    }

    if (!(p->location)) leave_sub(p,1);

    if (p->flags&RECONNECTION) {
	do_inform(p,"[%s reconnects] %s");
	sprintf(oldstack,"%s shimmers for a moment as %s re-connects.\n",
		p->name,gstring(p));
	stack=end_string(oldstack);
	tell_room(p->location,oldstack);
	p->flags &= ~RECONNECTION;
    }
    else {
	do_inform(p,"[%s has connected] %s");
	sprintf(oldstack,"%s appears flying through the air and lands "
		"in a heap on the ground.\n",p->name);
	stack=end_string(oldstack);
	tell_room(p->location,oldstack);
    }
    if (p->saved) {
	decompress_list(p->saved);
	p->saved_flags = p->saved->saved_flags;
    }
    if (p->residency&OFF_DUTY) sys_flags&=~ENFORCE_NO_NEW;
    stack=oldstack;
}

/* get new gender */

void enter_gender(player *p,char *str)
{
  switch(tolower(*str)) {
  case 'm':
    p->gender=MALE;
    tell_player(p,"Gender set to Male.\n");
    break;
  case 'f':
    p->gender=FEMALE;
    tell_player(p,"Gender set to Female.\n");
    break;
  case 'n':
    p->gender=OTHER;
    tell_player(p,"Gender set to well, erm, something.\n");
    break;
  default:
    tell_player(p,"No gender set.\n");
    break;
  }
  p->input_to_fn=0;
  link_to_program(p);
}


/* time out */

void login_timeout(player *p)
{
  tell_player(p,"\n\nConnection Timed Out ...\n\n");
  quit(p,0);
}


/* newbie stuff */

void newbie_get_gender(player *p,char *str)
{
  tell_player(p,"\n\nThe program requires that you enter your gender.\n"
	      "This is used solely for the purposes of correct english and grammar.\n"
	      "If you object to this, then simply type 'n' for not applicable.\n\n");
  do_prompt(p,"Enter (M)ale, (F)emale or (N)ot applicable:");
  p->input_to_fn=enter_gender;
}

void got_new_name(player *p,char *str)
{
  char *oldstack,*cpy;
  int length=0;
  oldstack=stack;
  
  for(cpy=str;*cpy;cpy++)
      if (isalpha(*cpy)) {
	  *stack++=*cpy;
	  length++;
      }
  *stack++=0;
  length++;
  if (length>(MAX_NAME-2)) {
      tell_player(p,"Sorry, that name is too long, please enter something "
		  "shorter.\n\n");
    do_prompt(p,"Please enter a name:");
    p->input_to_fn=got_new_name;
    if (sys_flags&VERBOSE) {
      cpy=stack;
      sprintf(cpy,"Name too long : %s\n",str);
      stack=end_string(cpy);
      log("connection",cpy);
      stack=cpy;
    }
    stack=oldstack;
    return;
  }
  if (length<3) {
    tell_player(p,"Thats a bit short, try something longer.\n\n");
    do_prompt(p,"Please enter different name:");
    p->input_to_fn=got_new_name;
    stack=oldstack;
    return;
  }
  
  if (restore_player_title(p,oldstack,0)) {
    tell_player(p,"Sorry, there is already someone who uses the program with that name.\n\n");
    do_prompt(p,"Please enter different name:");
    p->input_to_fn=got_new_name;
    stack=oldstack;
    return;
  } 
  newbie_get_gender(p,str);
  stack=oldstack;
}


void newbie_got_name_answer(player *p,char *str)
{
  switch(tolower(*str)) {
  case 'y':
    newbie_get_gender(p,str);
    break;
  case 'n':
    tell_player(p,"\n\nOk, then, please enter a new name ...\n\n");
    do_prompt(p,"Enter a name:");
    p->input_to_fn=got_new_name;
    break;
  default:
    tell_player(p,"Please answer with Y or N.\n");
    newbie_check_name(p,str);
    break;
  }
}

void newbie_check_name(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;

  sprintf(stack,"\n\nYou entered the name '%s' when you first logged in.\n"
	  "Is this the name that you wish to be known as on the program ?\n\n",
	  p->name);
  stack=end_string(stack);
  tell_player(p,oldstack);
  do_prompt(p,"Answer Y or N:");
  p->input_to_fn=newbie_got_name_answer;
  stack=oldstack;
}

void newbie_start(player *p,char *str)
{
  tell_player(p,newpage2_msg.where);
  do_prompt(p,"Hit return to continue:");
  p->input_to_fn=newbie_check_name;
}


/* test password */

int check_password(char *password,char *entered,player *p)
{
    char key[9];
    strncpy(key,entered,8);
    return (!strncmp(crypt(key,p->lower_name),password,11));
}
	   

void got_password(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;
  p->input_to_fn=0;
  password_mode_off(p);
  
  if (!check_password(p->password,str,p)) {
    tell_player(p,"\n\nHey !! that ain't right !\n"
		"Wrong password .... closing connection.\n\n");
    sprintf(stack,"Password fail: %s - %s",p->inet_addr,p->name);
    stack=end_string(stack);
    log("connection",oldstack);
    stack=oldstack;
    p->flags|=NO_SAVE_LAST_ON;
    quit(p,"");
    return;
  }

  if (p->gender<0) {
    p->input_to_fn=enter_gender;
    tell_player(p,"\nYou have no gender set.\n");
    do_prompt(p,"Please choose M(ale), F(female) or N(ot applicable):");
    return;
  }
  stack=oldstack;
  link_to_program(p);
}

/* check for soft splats */

int site_soft_splat(player *p)
{
    int no1=0,no2=0,no3=0,no4=0,out;
    
    out=time(0);
    sscanf(p->num_addr,"%d.%d.%d.%d",&no1,&no2,&no3,&no4);
    if (out<=soft_timeout && no1==soft_splat1 && no2==soft_splat2) 
	return 1;
    else
	return 0;
}

/* calls here when the player has entered their name */

void got_name(player *p,char *str)
{
    int t;
    char *oldstack,*cpy,*space;
    int length=0,isspace=0,nologin;
    player *search;

    oldstack=stack;

    for(cpy=str;*cpy && *cpy!=' ';cpy++)
	if (isalpha(*cpy)) {
	    *stack++=*cpy;
	    length++;
	}
    if (*cpy==' ') isspace=1;
    *stack++=0;
    space=stack;
    length++;
    if (length>(MAX_NAME-2)) {
	tell_player(p,"Sorry, that name is too long, please enter something shorter.\n\n");
	do_prompt(p,"Please enter a name:");
	p->input_to_fn=got_name;
	if (sys_flags&VERBOSE) {
	    cpy=stack;
	    sprintf(cpy,"Name too long : %s\n",str);
	    stack=end_string(cpy);
	    log("connection",cpy);
	    stack=cpy;
	}
	stack=oldstack;
	return;
    }
    if (length<3) {
	tell_player(p,"Thats a bit short, try something longer.\n\n");
	do_prompt(p,"Please enter a name:");
	p->input_to_fn=got_name;
	stack=oldstack;
	return;
    }

    if (!strcmp("who",oldstack)) {
	swho(p,0);
	p->input_to_fn=got_name;
	do_prompt(p,"\nPlease enter a name:");
	stack=oldstack;
	return;
    }
  
    if (!strcmp("quit",oldstack)) {
	quit(p,"");
	stack=oldstack;
	return;
    }
    if (isspace) while(*++cpy==' ');
/*
    if (banished_check(oldstack)) {
	tell_player(p,banned_msg.where);
	quit(p,"");
	stack=oldstack;
	return;
    }
*/	
    if (restore_player_title(p,oldstack,isspace?cpy:0)) {
	    
	if (p->residency==BANISHED) {
	    tell_player(p,banned_msg.where);
	    quit(p,"");
	    stack=oldstack;
	    return;
	}
	if (p->residency==STANDARD_ROOMS) {
	    tell_player(p,"\nSorry but that name is reserved.\n"
			"Choose a different name ...\n\n");
	    do_prompt(p,"Please enter a name:");
	    p->input_to_fn=got_name;
	    stack=oldstack;
	    return;
	}

	if (((p->residency<LOWER_SU) && 
	     (p->fd>=(max_players-PLAYER_MAX_RES))) ||
	     (!(p->residency&(ADMIN|LOWER_ADMIN)) && 
	      (p->fd>=(max_players-PLAYER_MAX_SUPER)))) {
          tell_player(p,full_msg.where);
          quit(p,"");
          stack=oldstack;
          return;
        }

	t=time(0);
	if (p->squished>t) {
	    nologin=p->squished-t;
	    stack=oldstack;
	    sprintf(stack,"\nSorry, you have been prevented from logging on for another %d seconds (and counting ...)\n\n",nologin);
	    stack=end_string(stack);
	    tell_player(p,oldstack);
	    quit(p,"");
	    stack=oldstack;
	    return;
	} else p->squished=0;
	sprintf(oldstack,"\n Password needed for '%s'\n\n",p->name);
	stack=end_string(oldstack);
	tell_player(p,oldstack);
	stack=oldstack;
    
	if (p->password[0]) {
	    password_mode_on(p);
	    do_prompt(p,"Please enter your password:");
	    p->input_to_fn=got_password; 
	    p->timer_count=60;  
	    p->timer_fn=login_timeout;
	    stack=oldstack;
	    return;
	}

	stack=oldstack;
	link_to_program(p);
	tell_player(p,"\nYou have no password !!!\n"
		    "Please set one as soon as possible with the 'password' command.\n"
		    "If you don't your character will not save\n");
	p->input_to_fn=0;
	return;
    }
  
    p->input_to_fn=0;

    if (p->fd>=(max_players-PLAYER_MAX_NEW)) {
      tell_player(p,full_msg.where);
      quit(p,"");
      stack=oldstack;
      return;
    }

    if (p->flags&CLOSED_TO_NEWBIES) {
	tell_player(p,newban_msg.where);
	quit(p,"");
	stack=oldstack;
	return;
    }

    if (site_soft_splat(p)) {
	tell_player(p,newban_msg.where);
	quit(p,"");
	stack=oldstack;
	return;
    }

/*
    You may wish to change the following if, so that when there are no
    superusers on the program, newbies are locked out. This is acheived
    by changing it with this commented out version.

    if (sys_flags&(CLOSED_TO_NEWBIES|ENFORCE_NO_NEW)) {
*/

    if (sys_flags&CLOSED_TO_NEWBIES) {    
	tell_player(p,nonewbies_msg.where);
	quit(p,"");
	stack=oldstack;
	return;
    }

    search=hashlist[((int)(p->lower_name[0]))-(int)'a'+1];
    for(;search;search=search->hash_next) 
	if (!strcmp(p->lower_name,search->lower_name)) {
	    tell_player(p,"\nSorry there is already someone on the program "
			"with that name.\nPlease try again, but use a "
			"different name.\n\n");
	    quit(p,"");
	    return;
	}
  
    tell_player(p,newpage1_msg.where);
    do_prompt(p,"Hit return to continue:");
    p->input_to_fn=newbie_start;
    p->timer_count=1800;
    stack=oldstack;
}

/* a new player has connected */

void connect_to_prog(player *p)
{
  tell_player(p,"\377\373\031");                     /* send will EOR */
  tell_player(p,connect_msg.where);
  do_prompt(p,"Please enter your name:");
  p->input_to_fn=got_name;
  p->timer_count=60;
  p->timer_fn=login_timeout;
}

/* tell player motd */

void motd(player *p,char *str)
{
  if (p->residency) tell_player(p,motd_msg.where);
  else tell_player(p,newbie_msg.where);
}


/* init everything needed for the plist file */

void init_plist()
{
  char *oldstack;
  int i;

  oldstack=stack;
 
#ifdef PC
  newban_msg=load_file("files\\newban.msg");
  nonewbies_msg=load_file("files\\nonew.msg");
  connect_msg=load_file("files\\connect.msg");
  motd_msg=load_file("files\\motd.msg");
  banned_msg=load_file("files\\banned.msg");
  newbie_msg=load_file("files\\newbie.msg");
  newpage1_msg=load_file("files\\newpage1.msg");
  newpage2_msg=load_file("files\\newpage2.msg");
#else
  newban_msg=load_file("files/newban.msg");
  nonewbies_msg=load_file("files/nonew.msg");
  connect_msg=load_file("files/connect.msg");
  motd_msg=load_file("files/motd.msg");
  banned_msg=load_file("files/banned.msg");
  newbie_msg=load_file("files/newbie.msg");
  newpage1_msg=load_file("files/newpage1.msg");
  newpage2_msg=load_file("files/newpage2.msg");
  disclaimer_msg=load_file("files/disclaimer.msg");

  splat_msg=load_file("files/splat.msg");
  
#endif

  hard_load_files();
  for(i=0;i<26;i++) update[i]=0;
 
  stack=oldstack;
}


/* various associated routines */

/* find one player given a (partial) name */

/* little subroutinette */

int match_player(char *str1,char *str2)
{
  for(;*str2;str1++,str2++) {
    if (*str1!=*str2 && *str2!=' ') {
      if (!(*str1) && *str2=='.' && !(*(str2+1))) return 1;
      return 0;
    }
  }
  if (*str1) return -1;
  return 1;
}


/* command to view res files */

void view_saved_lists(player *p,char *str)
{
  saved_player *scan,**hash;
  int i,j;
  char *oldstack;
  oldstack=stack;
  if (!*str || !isalpha(*str)) {
    tell_player(p,"Argument is a letter.\n");
    return;
  }
  strcpy(stack,"[HASH] [NAME]               12345678901234567890123456789012\n");
  stack=strchr(stack,0);
  hash=saved_hash[((int)(tolower(*str))-(int)'a')];
  for(i=0;i<HASH_SIZE;i++,hash++) 
    for(scan=*hash;scan;scan=scan->next) {
      sprintf(stack,"[%d]",i);
      j=strlen(stack);
      stack=strchr(stack,0);
      for (j=7-j;j;j--) *stack++=' ';
      strcpy(stack,scan->lower_name);
      j=strlen(stack);
      stack=strchr(stack,0);
      for (j=21-j;j;j--) *stack++=' ';
      switch(scan->residency) {
      case BANISHED:
        strcpy(stack,"BANISHED");
        break;
      case STANDARD_ROOMS:
        strcpy(stack,"Standard room file.");
        break;
      default:
	strcpy(stack,bit_string(scan->residency));
	break;
      }
      stack=strchr(stack,0);
      *stack++='\n';
    }
  *stack++=0;
  pager(p,oldstack,1);
  stack=oldstack;
}

/* external routine to check updates */

void check_updates(player *p,char *str)
{
  char *oldstack;
  int i;
  oldstack=stack;
  strcpy(stack,"abcdefghijklmnopqrstuvwxyz\n");
  stack=strchr(stack,0);
  for(i=0;i<26;i++) 
    if (update[i]) *stack++='*';
    else *stack++=' ';
  *stack++='\n';
  *stack++=0;
  tell_player(p,oldstack);
  stack=oldstack;
}


void player_flags(player *p)
{
    char *oldstack,*str;
    oldstack=stack;
    str=stack;
    if (p->residency==NON_RESIDENT) {
	log("error","You've sponged Chris. Tried to player_flags a non-resi");
	return;
    }
    if (!p->saved) 
	tell_player(p,"Eeeeeeek ! No saved bits !\n");
    else {
	sprintf(oldstack,"\n --\nLast logged in %s from %s.\n",
		convert_time(p->saved->last_on),p->saved->last_host);
	    stack=strchr(stack,0);
	strcpy(stack,"You are ");
	    stack=strchr(stack,0);
	if (p->saved_flags&HIDING) {
	    strcpy(stack,"in hiding, ");
	    stack=strchr(stack,0);
	}
	if (p->saved_flags&BLOCK_SHOUT) {
	    strcpy(stack,"ignoring shouts, ");
	    stack=strchr(stack,0);
	}
	if (p->saved_flags&BLOCK_TELLS) {
	    strcpy(stack,"ignoring tells, ");
	    stack=strchr(stack,0);
	}
	if (p->saved_flags&CONVERSE) {
	    strcpy(stack,"in converse mode, ");
	    stack=strchr(stack,0);
	}
	if (p->saved_flags&NOPREFIX) {
	    strcpy(stack,"ignoring prefixes, ");
	    stack=strchr(stack,0);
	}
	if (str=strrchr(oldstack,',')) {
	    *str++='.';
	    *str++='\n';
	    *str++=0;
	    stack=strchr(stack,0);
	    *stack++;
	    tell_player(p,oldstack);
	}
    }
    if (p->saved_flags&NEW_MAIL) {
	command_type |= HIGHLIGHT;
	tell_player(p,"You have unread mail\n");
	p->saved_flags &= ~NEW_MAIL;
	p->saved->saved_flags&=~NEW_MAIL;
	command_type &= ~HIGHLIGHT;
    }
    tell_player(p," --\n");
    stack=oldstack;
}

void player_flags_verbose(player *p,char *str)
{
    char *oldstack,*wibble,*argh;
    player *p2;
    oldstack=stack;

    if (*str && p->residency&LOWER_SU) {
	p2=find_player_absolute_quiet(str);
	if (!p2) {
	    tell_player(p,"No-one on of that name.\n");
	    return;
	}
    }
    else
	p2=p;

    if (p2->residency==NON_RESIDENT) {
	tell_player(p,"You aren't a resident, so your character won't be "
		    "saved when you log off.\n");
	return;
    }
    strcpy(stack,"\n --\n");
    stack=strchr(stack,0);
    argh=stack;
    strcpy(stack,"You are ");
    stack=strchr(stack,0);
    if (p2->saved_flags&HIDING) {
	strcpy(stack,"in hiding, ");
	stack=strchr(stack,0);
    }
    if (p2->saved_flags&BLOCK_SHOUT) {
	strcpy(stack,"ignoring shouts, ");
	stack=strchr(stack,0);
    }
    if (p2->saved_flags&BLOCK_TELLS) {
	strcpy(stack,"ignoring tells, ");
	stack=strchr(stack,0);
    }
    if (p2->saved_flags&CONVERSE) {
	strcpy(stack,"in converse mode, ");
	stack=strchr(stack,0);
    }
    if (p2->saved_flags&NOPREFIX) {
	strcpy(stack,"ignoring prefixes, ");
	stack=strchr(stack,0);
    }
    if (wibble=strrchr(oldstack,',')) {
	*wibble++='.';
	*wibble++='\n';
	*wibble=0;
    }
    else
	stack=argh;
    if (p2->saved_flags&PRIVATE_EMAIL)
	strcpy(stack,"Your email is private.\n");
    else
	strcpy(stack,"Your email is public for all to see.\n");
    stack=strchr(stack,0);
    if (p2->saved_flags&TRANS_TO_HOME) {
	strcpy(stack,"You will be taken to your home when you log in.\n");
	stack=strchr(stack,0);
    }
    else
	if (*p2->room_connect) {
	    sprintf(stack,"You will try to connect to room '%s' when you log"
		    " in\n",p->room_connect);
	    stack=strchr(stack,0);
	}
    if (p2->saved_flags&NO_ANONYMOUS) 
	strcpy(stack,"You won't receive anonymous mail.\n");
    else
	strcpy(stack,"You are currently able to receive anonymous mail.\n");
    stack=strchr(stack,0);
    if (p2->saved_flags&IAC_GA_ON) 
	strcpy(stack,"Iacga prompting is turned on.\n");
    else
	strcpy(stack,"Iacga prompting is turned off.\n");
    stack=strchr(stack,0);
    if (p2->saved_flags&NO_PAGER) 
	strcpy(stack,"You are not recieving paged output.\n");
    else
	strcpy(stack,"You are recieving paged output.\n");
    stack=strchr(stack,0);
    if (p2->flags&BLOCK_SU && p->residency&LOWER_SU) {
	strcpy(stack,"You are ignoring sus.\n");
	stack=strchr(stack,0);
    }
    strcpy(stack," --\n");
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

int banished_check(char *name)
{
/*    char *oldstack,*scan,*mark,*mark2;
    oldstack=stack;
    strcpy(stack,name);
    for (scan=stack;*scan;scan++) *scan=tolower(*scan);
    
    scan=banished_p.where;
    
    mark=strchr(scan,'\n');
    while (mark) {
	mark2=mark;
	mark2++;
	*mark=0;
	if (*scan=='*') {
	    scan++;
	    if (strstr(stack,scan)) {
		*mark='\n';
		return 1;
	    }
	}
	else {
	    if (!strcmp(stack,scan)) {
		*mark='\n';
		return 1;
	    }
	}
	*mark='\n';
	scan=mark2;
	mark=strchr(mark2,'\n');
    }*/
    return 0;
}
	    
void add_name_banish(char *name)
{
/*    char *oldstack;

    oldstack=stack;
    strcpy(oldstack,banished_p.where);
    free(banished_p.where);
    oldstack=strchr(oldstack,0);
    sprintf(oldstack,"%s\n",name);

    banished_p.where=(char *)malloc(strlen(stack)+1);
    banished_p.length=strlen(stack);
    strcpy(banished_p.where,stack); */
}

void sync_banished_files(int background)
{
    int fd;
    
/*    if (background && fork()) return;  
    
    if (sys_flags&PANIC) 
	log("sync","Dumping banished file to disk");
    
    fd=open("files/players/banished_players",O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if (fd<0) {
	log("sync","Fuck, couldn't open banished players file.");
	return;
    }
    if (write(fd,banished_p.where,banished_p.length)<0)
	log("sync","Failed to write banished file.");
    close(fd); */
/*    if (background) exit(0); */
}

  /* remove from list (must be lowercase!!!!) */

int remove_banish_name(char *name,char *start)
{
#ifdef 0
    char *oldstack,*mark,*scan;

    oldstack=stack;

    mark=strstr(start,name);
    if (!mark) return 0;
    scan=mark;
    if (!*(--mark) || *mark=='\n') {
	for (mark++;*mark && *mark!='\n';mark++) if (*mark=='*') return 0;
	if (!*mark) return 0;
	/* check for total match */
	*mark=0;
	if (strlen(name)!=strlen(scan)) {
	    *mark='\n';
	    if (remove_banish_name(name,++mark))
		return 1;
	    else
		return 0;
	}
	*scan=0;
	strcpy(stack,banished_p.where);
	if (*(++mark)) {
	    stack=strchr(stack,0);
	    strcpy(stack,mark);
	}
	free(banished_p.where);
	banished_p.where=(char *)malloc(strlen(oldstack)+1);
	strcpy(banished_p.where,oldstack);
	banished_p.length=strlen(oldstack);
	stack=oldstack;
	return 1;
    }
    stack=oldstack;
#endif
    return 0;
}

int quick_banish_check(char *lowername,int verbose)
{
/*    char *begin,*end,*oldstack;
    int len;
    oldstack=stack;
    strcpy(stack,lowername);
    lower_case(stack);
    len=strlen(stack);
    if (!strncmp(stack,banished_p.where,len) && 
	banished_p.where[len]=='\n') {
	if (verbose) tell_current("That name is banished.\n");
	return 1;
    }
    begin=strstr(banished_p.where,stack);
    if (!begin) return 0;
    end=begin;
    if (*(--begin) && *begin!='\n') return 0;
    for (;len;len--) {
	if (*end=='*') return 0;
	end++;
    }
    if (*end!='\n') return 0;
    if (verbose) tell_current("That name is banished.\n");
    return 1; */
  return 0;
}

void view_banished(player *p,char *str)
{
/*    pager(p,banished_p.where,1); */
}

void test_name(player *p,char *str)
{
/*    if (!*str) {
	tell_player(p,"Format: test <name>\n");
	return;
    }
    if (!banished_check(str))
	tell_player(p,"That name is not banished ...\n");
    else 
	tell_player(p,"That name is banished ...\n"); */
}

