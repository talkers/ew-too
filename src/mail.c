/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  mail.c
*/

#include <stdlib.h>
#include <ctype.h>
#include <sys/time.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/time.h>

#include "fix.h"
#include "config.h"
#include "player.h"

/* externs */


extern char *first_char(),*next_space();
extern char *store_string(),*store_int(),*store_word();
extern char *get_string(),*get_int(),*get_word(),*end_string();
extern saved_player *find_saved_player();
extern struct command mail_list[],news_list[];
extern player *find_player_global(),*find_player_absolute_quiet();
extern void pager(player *,char *,int),sync_banished_files(int);

#ifdef MIPS
extern char *strstr(char *,char *);
#endif

#ifndef S_IRUSR
#define S_IRUSR 00400
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200
#endif

 /* interns */

note *n_hash[NOTE_HASH_SIZE];
int nhash_update[NOTE_HASH_SIZE];
int unique=1;
int news_start=0,news_count=0;

void unlink_mail();

 /* find note in hash */

note *find_note(int id)
{
    note *scan;
    if (!id) return 0;
    for(scan=n_hash[id%NOTE_HASH_SIZE];scan;scan=scan->hash_next)
	if (scan->id==id) return scan;
    return 0;
}

 /* create a new note */

note *create_note()
{
    note *n;
    int num;
    n=(note *)MALLOC(sizeof(note));
    memset(n,0,sizeof(note));
    n->flags = NOT_READY;
    n->date = time(0);
    n->next_sent=0;
    n->text.where=0;
    n->text.length=0;
    strcpy(n->header,"DEFUNCT");
    strcpy(n->name,"system");
    while(find_note(unique)) unique++;
    n->id=unique++;
    unique &= 65535;
    num=(n->id)%NOTE_HASH_SIZE;
    n->hash_next=n_hash[num];
    n_hash[num]=n;
    nhash_update[num]=1;
    return n;
}

 /* remove a note */

void remove_note(note *n)
{
   note *scan,*prev;
   int num;
   if (n->text.where) FREE(n->text.where);
   num=(n->id)%NOTE_HASH_SIZE;
   scan=n_hash[num];
   if (scan==n) n_hash[num]=n->hash_next;
   else {
     do {
       prev=scan;
       scan=scan->hash_next;
     } while(scan!=n);
     prev->hash_next=n->hash_next;
   }
   FREE(n);
   nhash_update[num]=1;
 }

 /* remove various types of note */

void remove_any_note(note *n) 
{
    saved_player *sp;
    note *scan=0;
    char *oldstack;
    int *change;
    int recursive=0;
    oldstack=stack;
    if (n->flags&NEWS_ARTICLE) {
	scan=find_note(news_start);
	change=&news_start;
	while(scan && scan!=n) {
	    change=&(scan->next_sent);
	    scan=find_note(scan->next_sent);
	}
	if (scan==n) news_count--;
    }
    else {
	strcpy(stack,n->name);
	lower_case(stack);
	stack=end_string(stack);
	sp=find_saved_player(oldstack);
	if (!sp) {
	    log("error","Bad owner name in mail(1)");
	    log("error",oldstack);
	    recursive=n->next_sent;
	    scan=0;
	}
	else {
	    change=&(sp->mail_sent);
	    scan=find_note(sp->mail_sent);
	    while(scan && scan!=n) {
		change=&(scan->next_sent);
		scan=find_note(scan->next_sent);
	    }
	}
    }
    if (scan==n) (*change)=n->next_sent;
    remove_note(n);
    if (recursive) unlink_mail(find_note(recursive));
    stack=oldstack;
}



 /* dump one note onto the stack */

int save_note(note *d)
{
    if (d->flags&NOT_READY) return 0;

    stack=store_int(stack,d->id);
    stack=store_int(stack,d->flags);
    stack=store_int(stack,d->date);
    stack=store_int(stack,d->read_count);
    stack=store_int(stack,d->next_sent);
    stack=store_string(stack,d->header);
    stack=store_int(stack,d->text.length);
    memcpy(stack,d->text.where,d->text.length);
    stack+=d->text.length;
    stack=store_string(stack,d->name);
    return 1;
}


 /* sync one hash bucket to disk */

void sync_note_hash(int number)
{
    char *oldstack;
    note *scan,*check;
    int length,count=0,fd,t,tout;
    oldstack=stack;

    if (sys_flags&VERBOSE) {
	sprintf(stack,"Syncing note hash %d.",number);
	stack=end_string(stack);
	log("sync",oldstack);
	stack=oldstack;
    }

    t=time(0);

    for (scan=n_hash[number];scan;scan=check) {
      if (scan->flags&NEWS_ARTICLE) 
      	tout=NEWS_TIMEOUT;
      else
        tout=MAIL_TIMEOUT;
      if ((t-(scan->date))>tout) {
	remove_any_note(scan);
	check=n_hash[number];
      }
      else check=scan->hash_next;
    }

    stack=store_int(stack,0);

    for(scan=n_hash[number];scan;scan=scan->hash_next)
      count+=save_note(scan);

    store_int(oldstack,count);
    length=(int)stack-(int)oldstack;

#ifdef PC
    sprintf(stack,"files\\notes\\hash%d",number);
    fd=open(stack,O_CREAT|O_WRONLY|O_TRUNC|O_BINARY);
#else
    sprintf(stack,"files/notes/hash%d",number);
    fd=open(stack,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
#endif
    if (fd<0) handle_error("Failed to open note file.");
    if (write(fd,oldstack,length)<0)
	handle_error("Failed to write note file.");
    close(fd);

    nhash_update[number]=0;
    stack=oldstack;
}

 /* throw all the notes to disk */

void sync_notes(int background)
{
    int n,fd;
    struct itimerval new;
    char *oldstack;
    oldstack=stack;

    if (background && fork()) return;

    if (sys_flags&VERBOSE || sys_flags&PANIC)
	log("sync","Dumping notes to disk");
    for(n=0;n<NOTE_HASH_SIZE;n++) sync_note_hash(n);
    if (sys_flags&VERBOSE || sys_flags&PANIC)    
	log("sync","Note dump completed");
    
#ifdef PC
    fd=open("files\\notes\\track",O_CREAT|O_WRONLY|O_TRUNC|O_BINARY);
#else
    fd=open("files/notes/track",O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
#endif
    if (fd<0) handle_error("Failed to open track file.");
    stack=store_int(stack,unique);
    stack=store_int(stack,news_start);
    stack=store_int(stack,news_count);
    if (write(fd,oldstack,12)<0)
	handle_error("Failed to write track file.");
    close(fd);
    
    stack=oldstack;
    
    if (background) exit(0);
}

/* su command to dump all or single notes to disk */

void dump_com(player *p,char *str)
{
    int num;
    char *oldstack;
    oldstack=stack;
    if (!isdigit(*str)) {
	tell_player(p,"Dumping all notes to disk.\n");
/*	sync_banished_files(0); */
	sync_notes(1);
	return;
    }
    num=atoi(str);
    if (num<0 || num>=NOTE_HASH_SIZE) {
	tell_player(p,"Argument not in hash range !\n");
	return;
    }
    sprintf(stack,"Dumping hash %d to disk.\n",num);
    stack=end_string(stack);
    tell_player(p,oldstack);
    sync_note_hash(num);
    stack=oldstack;
}


 /* throw away a hash of notes */

 void discard_hash(int num)
 {
   note *scan,*next;
   for(scan=n_hash[num];scan;scan=next) {
     next=scan->hash_next;
     if (scan->text.where)  FREE(scan->text.where);
     FREE(scan);
   }
 }


 /* extracts one note into the hash list */

 /*
#define DEBUG(x,y) printf(x,y);  \
   get_string(stack,where);   \
   printf("where = %s\n",stack);
*/

#define DEBUG(x,y)

 
 char *extract_note(char *where)
 {
   int num;
   note *d;
   d=(note *)MALLOC(sizeof(note));
DEBUG("d=%d\n",d)
   memset(d,0,sizeof(note));
   where=get_int(&d->id,where);
DEBUG("id=%d\n",d->id)
   where=get_int(&d->flags,where);
DEBUG("flags=%d\n",d->flags)
   where=get_int(&d->date,where);
DEBUG("date=%d\n",d->date)
   where=get_int(&d->read_count,where);
DEBUG("rcount=%d\n",d->read_count)
   where=get_int(&d->next_sent,where);
DEBUG("next_sent=%d\n",d->next_sent)
   where=get_string(d->header,where);
DEBUG("header=%s\n",d->header)
   where=get_int(&d->text.length,where);
DEBUG("text length=%d\n",d->text.length)
   d->text.where=(char *)MALLOC(d->text.length);
DEBUG("text where=%d\n",d->text.where)
   memcpy(d->text.where,where,d->text.length);
   where+=d->text.length;
   where=get_string(d->name,where);
DEBUG("name=%s\n",d->name)

   if (d->id == d->next_sent) {
     FREE(d->text.where);
     FREE(d);
   }
   else {
     num=(d->id)%NOTE_HASH_SIZE;
     if (n_hash[num]) d->hash_next=n_hash[num];
     else d->hash_next=0;
     n_hash[num]=d;
   }

/*   get_string(stack,d->text);
   printf("%s:%s\n%s\n",d->name,d->header,stack); */
   
   
   return where;
 }

 /* load all notes from disk
    this should be changed for arbitary hashes */

 void init_notes()
 {
   int n,length,fd,count;
   char *oldstack,*where,*scan;
   oldstack=stack;

   if (sys_flags&VERBOSE || sys_flags&PANIC)
     log("boot","Loading notes from disk");

#ifdef PC
   fd=open("files\\notes\\track",O_RDONLY|O_BINARY);
#else
   fd=open("files/notes/track",O_RDONLY|O_NDELAY);
#endif
   if (fd<0) {
     sprintf(oldstack,"Failed to load track file");
     stack=end_string(oldstack);
     log("error",oldstack);
     unique=1;
     news_start=0;
     stack=oldstack;
   }
   else {
     if (read(fd,oldstack,12)<0) handle_error("Can't read track file.");
     stack=get_int(&unique,stack);
     stack=get_int(&news_start,stack);
     stack=get_int(&news_count,stack);
     close(fd);
     stack=oldstack;
   }


   for(n=0;n<NOTE_HASH_SIZE;n++) {

     discard_hash(n);
     nhash_update[n]=0;

#ifdef PC
     sprintf(oldstack,"files\\notes\\hash%d",n);
     fd=open(oldstack,O_RDONLY|O_BINARY);
#else
     sprintf(oldstack,"files/notes/hash%d",n);
     fd=open(oldstack,O_RDONLY|O_NDELAY);
#endif
     if (fd<0) {
       sprintf(oldstack,"Failed to load note hash%d",n);
       stack=end_string(oldstack);
       log("error",oldstack);
       stack=oldstack;
     }
     else {
       length=lseek(fd,0,SEEK_END);
       lseek(fd,0,SEEK_SET);
       if (length) {
	 where=(char *)MALLOC(length);
	 if (read(fd,where,length)<0) handle_error("Can't read note file.");
	 scan=get_int(&count,where);
	 for(;count;count--) scan=extract_note(scan);
	 FREE(where);
       }
       close(fd);
     }
   }
   stack=oldstack;
 }



 /* store info for a player save */

 void construct_mail_save(saved_player *sp)
 {
   int count=0,*scan;
   char *oldstack;
   stack=store_int(stack,sp->mail_sent);
   if (!(sp->mail_received)) stack=store_int(stack,0);
   else {
     oldstack=stack;
     stack=store_int(oldstack,0);
     for(scan=sp->mail_received;*scan;scan++,count++)
       stack=store_int(stack,*scan);
     store_int(oldstack,count);
   }
 }


 /* get info back from a player save */

 char *retrieve_mail_data(saved_player *sp,char *where)
 {
   int count=0,*fill;
   where=get_int(&sp->mail_sent,where);
   where=get_int(&count,where);
   if (count) {
     fill=(int *)MALLOC((count+1)*sizeof(int));
     sp->mail_received=fill;
     for(;count;count--,fill++) where=get_int(fill,where);
     *fill++=0;
   }
   else sp->mail_received=0;
   return where;
 }


 /* su command to check out note hashes */

 void list_notes(player *p,char *str)
 {
   int num;
   note *scan;
   char *oldstack;

   oldstack=stack;

   num=atoi(str);
   if (num<0 || num>=NOTE_HASH_SIZE) {
     tell_player(p,"Number not in range.\n");
     return;
   }

   strcpy(stack,"Notes:\n");
   stack=strchr(stack,0);
   for(scan=n_hash[num];scan;scan=scan->hash_next) {
     if (scan->flags&NEWS_ARTICLE) 
       sprintf(stack,"[%d] %s + %s",scan->id,
	       scan->name,scan->header);
     else 
       sprintf(stack,"[%d] %s - %s",scan->id,scan->name,
	       bit_string(scan->flags));
     stack=strchr(stack,0);
     if (scan->flags&NOT_READY) {
       strcpy(stack,"(DEFUNCT)\n");
       stack=strchr(stack,0);
     }
     else *stack++='\n';
   }
   strcpy(stack," --\n");
   stack=strchr(stack,0);
   stack++;
   tell_player(p,oldstack);
   stack=oldstack;
 }


void list_all_notes(player *p,char *str)
{
    char *oldstack;
    int count=0,hash;
    note *scan;

    oldstack=stack;

    strcpy(stack," All notes --\n");
    stack=strchr(stack,0);
    for (hash=0;hash<=NOTE_HASH_SIZE;count=0,hash++) {
	for (scan=n_hash[hash];scan;scan=scan->hash_next) count++;
	sprintf(stack,"%3d notes in bucket %2d",count,hash);
	stack=strchr(stack,0);
	if (hash&1)
	    strcpy(stack,"   --   ");
	else
	    strcpy(stack,"\n");
	stack=strchr(stack,0);
    }
    strcpy(stack," --\n");
    stack=strchr(stack,0);
    stack++;
    tell_player(p,oldstack);
    stack=oldstack;
}

 /* the news command */

void news_command(player *p,char *str)
{
    if (p->edit_info) {
	tell_player(p,"Can't do news commands whilst in editor.\n");
	return;
    }

    if ((*str=='/') && (p->input_to_fn==news_command)) {
	match_commands(p,str+1);
	if (!(p->flags&PANIC) && (p->input_to_fn==news_command))
	    do_prompt(p,"News Mode >");
	return;
    }
    if (!*str) 
	if (p->input_to_fn==news_command) {
	    tell_player(p,"Format: news <action>\n");
	    if (!(p->flags&PANIC) && (p->input_to_fn==news_command))
		do_prompt(p,"News Mode >");
	    return;
	}
	else {
	    tell_player(p,"Entering news mode. Use 'end' to leave.\n"
			"'/<command>' does normal commands.\n");
	    p->flags &= ~PROMPT;
	    p->input_to_fn=news_command;
	}
    else sub_command(p,str,news_list);
    if (!(p->flags&PANIC) && (p->input_to_fn==news_command))
	do_prompt(p,"News Mode >");
}


 /* the mail command */

void mail_command(player *p,char *str)
{
    if (p->edit_info) {
	tell_player(p,"Can't do mail commands whilst in editor.\n");
	return;
    }

    if (!p->saved) {
	tell_player(p,"You have no save information, and so can't use "
		    "mail.\n");
	return;
    }

    if ((*str=='/') && (p->input_to_fn==mail_command)) {
	match_commands(p,str+1);
	if (!(p->flags&PANIC) && (p->input_to_fn==mail_command))
	    do_prompt(p,"Mail Mode >");
	return;
    }
    if (!*str) 
	if (p->input_to_fn==mail_command) {
	    tell_player(p,"Format: mail <action>\n");
	    if (!(p->flags&PANIC) && (p->input_to_fn==mail_command))
		do_prompt(p,"Mail Mode >");
	    return;
	}
	else {
	    tell_player(p,"Entering mail mode. Use 'end' to leave.\n"
			"'/<command>' does normal commands.\n");
	    p->flags &= ~PROMPT;
	    p->input_to_fn=mail_command;
	}
    else sub_command(p,str,mail_list);
    if (!(p->flags&PANIC) && (p->input_to_fn==mail_command))
	do_prompt(p,"Mail Mode >");
}

 /* view news commands */

void view_news_commands(player *p,char *str)
{
    view_sub_commands(p,news_list);
}

 /* view mail commands */

void view_mail_commands(player *p,char *str)
{
    view_sub_commands(p,mail_list);
}

 /* exit news mode */

void exit_news_mode(player *p,char *str)
{
    if (p->input_to_fn != news_command) return;
    tell_player(p,"Leaving news mode.\n");
    p->input_to_fn=0;
    p->flags|=PROMPT;
}

 /* exit mail mode */

void exit_mail_mode(player *p,char *str)
{
    if (p->input_to_fn != mail_command) return;
    tell_player(p,"Leaving mail mode.\n");
    p->input_to_fn=0;
    p->flags|=PROMPT;
}

 /* finds news article number x */

note *find_news_article(int n)
{
    int integrity=0;
    note *scan;
    if (n<1 || n>news_count) return 0;

    for(n--,scan=find_note(news_start);n;n--,integrity++)
	if (scan) scan=find_note(scan->next_sent);
	else {
	    news_count=integrity;
	    return 0;
	}
    return scan;
}

 /* list news articles */

void list_news(player *p,char *str)
{
    char *oldstack,middle[80];
    int page,pages,count,article,ncount=1;
    note *scan;
    oldstack=stack;
    
    if (!news_count) {
	tell_player(p,"No news articles to view.\n");
	return;
    }
    
    page=atoi(str);
    if (page<=0) page=1;
    page--;
    
    pages=(news_count-1)/(TERM_LINES-2);
    if (page>pages) page=pages;
    
    if (news_count==1) strcpy(middle,"There is one news article");
    else sprintf(middle,"There are %s articles",
		 number2string(news_count));
    pstack_mid(middle);

    count=page*(TERM_LINES-2);

    for(article=news_start;count;count--,ncount++) {
	scan=find_note(article);
	if (!scan) {
	    tell_player(p,"Bad news listing, aborted.\n");
	    log("error","Bad news list");
	    stack=oldstack;
	    return;
	}
	article=scan->next_sent;
    }
    for(count=0;(count<(TERM_LINES-1));count++,ncount++) {
	scan=find_note(article);
	if (!scan) break;
	if (p->residency&(LOWER_ADMIN|ADMIN))
	    sprintf(stack,"(%d) [%d] ",scan->id,ncount);
	else sprintf(stack,"[%d] ",ncount);
	while(*stack) *stack++;
	if (ncount<10) *stack++=' ';
	strcpy(stack,scan->header);
	while(*stack) *stack++;
	if (scan->flags&ANONYMOUS) {
	    if (p->residency&(LOWER_ADMIN|ADMIN)) sprintf(stack," <%s>\n",scan->name);
	    else strcpy(stack,"\n");
	}
	else sprintf(stack," (%s)\n",scan->name);
	stack=strchr(stack,0);
	article=scan->next_sent;
    }  

    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);

    *stack++=0;
    tell_player(p,oldstack);

    stack=oldstack;
}


 /* post a news article */

void quit_post(player *p)
{
    tell_player(p,"Article NOT posted.\n");
    remove_note((note *)p->edit_info->misc);
}


void end_post(player *p,char *str)
{
    note *article;
    char *oldstack;
    oldstack=stack;

    article=(note *)p->edit_info->misc;
    stack=store_string(oldstack,p->edit_info->buffer);
    article->text.length=(int)stack-(int)oldstack;
    article->text.where=(char *)MALLOC(article->text.length);
    memcpy(article->text.where,oldstack,article->text.length);
    stack=oldstack;

    article->next_sent=news_start;
    news_start=article->id;
    news_count++;
    article->flags &= ~NOT_READY;
    article->read_count=0;
    tell_player(p,"Article posted....\n");
    if (p->edit_info->input_copy==news_command) do_prompt(p,"News Mode >");
}

void post_news(player *p,char *str)
{
    note *article;
    char *scan,*first;
    if (!*str) {
	tell_player(p,"Format: post <header>\n");
	return;
    }
    article=create_note();
    strncpy(article->header,str,MAX_TITLE-2);
    article->flags |= NEWS_ARTICLE;
    first=first_char(p);
    scan=strstr(first,"post");
    if (scan && (scan!=first_char(p)) && (*(scan-1)=='a'))
	article->flags |= ANONYMOUS;
    strcpy(article->name,p->name);
    tell_player(p,"Now enter the main body text for the article.\n");
    *stack=0;
    start_edit(p,MAX_ARTICLE_SIZE,end_post,quit_post,stack);
    if (p->edit_info) p->edit_info->misc=(void *)article;
    else FREE(article);
}

 /* follow up an article */

void followup(player *p,char *str)
{
    char *oldstack,*body,*indent,*scan,*first;
    note *article,*old;
    oldstack=stack;

    old=find_news_article(atoi(str));
    if (!old) {
	sprintf(stack,"No such news article '%s'\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }

    article=create_note();
    if (strstr(old->header,"Re: ")==old->header) 
	strcpy(article->header,old->header);
    else {
	sprintf(stack,"Re: %s",old->header);
	strncpy(article->header,stack,MAX_TITLE-2);
    }
    article->flags |= NEWS_ARTICLE;

    first=first_char(p);
    scan=strstr(first,"followup");
    if (scan && (scan!=first_char(p)) && (*(scan-1)=='a'))
	article->flags |= ANONYMOUS;

    strcpy(article->name,p->name);

    indent=stack;
    get_string(stack,old->text.where);
    stack=end_string(stack);
    body=stack;

    if (old->flags&ANONYMOUS)
	sprintf(stack,"From annonymous article written on %s ...\n",
		convert_time(old->date));
    else sprintf(stack,"On %s, %s wrote ...\n",
		 convert_time(old->date),old->name);
    stack=strchr(stack,0);

    while(*indent) {
	*stack++='>';
	*stack++=' ';
	while(*indent && *indent!='\n') *stack++=*indent++;
	*stack++='\n';
	indent++;
    }
    *stack++='\n';
    *stack++=0;

    tell_player(p,"Please trim article as much as is possible ....\n");
    start_edit(p,MAX_ARTICLE_SIZE,end_post,quit_post,body);
    if (p->edit_info) p->edit_info->misc=(void *)article;
    else FREE(article);
    stack=oldstack;
}

 /* wipe a news article */

void remove_article(player *p,char *str)
{
    note *article,*prev,*scan;
    char *oldstack;
    oldstack=stack;
    article=find_news_article(atoi(str));
    if (!article) {
	sprintf(stack,"No such news article '%s'\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    strcpy(stack,article->name);
    lower_case(stack);
    if (!(p->residency&(LOWER_ADMIN|ADMIN)) && strcmp(stack,p->lower_name)) {
	tell_player(p,"You can't remove an article that isn't yours.\n");
	return;
    }

    scan=find_note(news_start);
    if (scan==article) news_start=article->next_sent;
    else {
	do {
	    prev=scan;
	    scan=find_note(scan->next_sent);
	} while(scan!=article);
	prev->next_sent=article->next_sent;
    }
    news_count--;
    remove_note(article);
    tell_player(p,"Article removed.\n");
    stack=oldstack;
}

 /* read an article */

void read_article(player *p,char *str)
{
    char *oldstack;
    note *article;
    oldstack=stack;

    if (!*str) {
	tell_player(p,"Format: read <article-number>\n");
	return;
    }

    article=find_news_article(atoi(str));
    if (!article) {
	sprintf(stack,"No such news article '%s'\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    article->read_count++;
    if (article->flags&ANONYMOUS) {
	if (p->residency&(LOWER_ADMIN|ADMIN))
	    sprintf(stack,"Subject: %s\nPosted annonymously on %s by %s.\n",
		    article->header,convert_time(article->date),article->name);
	else sprintf(stack,"Subject: %s\nPosted annonymously on %s.\n",
		     article->header,convert_time(article->date));
    }
    else sprintf(stack,"Subject: %s\nPosted by %s on %s.\n",article->header,
		 article->name,convert_time(article->date));
    stack=strchr(stack,0);
    if (article->read_count==1) strcpy(stack,"Article read once only.\n\n");
    else sprintf(stack,"Article has been read %s times.\n\n",
		 number2string(article->read_count));
    stack=strchr(stack,0);
    get_string(stack,article->text.where);
    stack=end_string(stack);
    pager(p,oldstack,0);
    stack=oldstack;
}



 /* mail stuff */



 /* count how many mails have been posted */

 int posted_count(player *p)
 {
   int scan,count=0;
   note *mail,*next ;

   if (!p->saved) return 0;
   scan=p->saved->mail_sent;
   mail=find_note(scan);
   if (!mail) {
     p->saved->mail_sent=0;
     return 0;
   }
   while(mail) {
     count++;
     scan=mail->next_sent;
     next=find_note(scan);
     if (!next && scan) {
       mail->next_sent=0;
       mail=0;
     }
     else mail=next;
   }
   return count;
 }



 /* view mail that has been sent */

void view_sent(player *p,char *str)
{
    char *oldstack;
    note *mail,*next;
    int count=1,scan;
    saved_player *sp;
    oldstack=stack;

    if (*str && p->residency&(LOWER_ADMIN|ADMIN)) {
	sp=find_saved_player(str);
	if (!sp) {
	    tell_player(p,"No such player in save files.\n");
	    return;
	}
	else {
	    scan=sp->mail_sent;
	    mail=find_note(scan);
	    if (!mail) {
		sp->mail_sent=0;
		tell_player(p,"You have sent no mail.\n");
		return;
	    }
	}
    }
    else {
	if (!p->saved) {
	    tell_player(p,"You have no save file, and therefore no mail "
			"either.\n");
	    return;
	}
	scan=p->saved->mail_sent;
	mail=find_note(scan);
	if (!mail) {
	    p->saved->mail_sent=0;
	    tell_player(p,"You have sent no mail.\n");
	    return;
	}
    }
    strcpy(stack,"Listing mail sent...\n");
    stack=strchr(stack,0);
    while(mail) {
	if (p->residency&(LOWER_ADMIN|ADMIN))
	    sprintf(stack,"(%d) [%d] %d - %s\n",mail->id,count,
		    mail->read_count,mail->header);
	else 
	    sprintf(stack,"[%d] %d - %s\n",count,mail->read_count,
		    mail->header);
	stack=strchr(stack,0);
	scan=mail->next_sent;
	count++;
	next=find_note(scan);
	if (!next && scan) {
	    mail->next_sent=0;
	    mail=0;
	}
	else mail=next;
    }
    sprintf(stack,"You have sent %d out of your maximum of %d mails.\n",
	    count-1,p->max_mail);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;		
}


 /* this fn goes through the received list and removes dud note pointers */

void reconfigure_received_list(saved_player *sp)
{
    int count=0;
    int *scan,*fill;
    char *oldstack;
    oldstack=stack;

    align(stack);
    fill=(int *)stack;
    scan=sp->mail_received;
    if (!scan) return;
    for(;*scan;scan++)
	if (((*scan)<0) || ((*scan)>65536)) {
	    count=0;
	    break;
	}
	else if (find_note(*scan)) {
	    *(int *)stack=*scan;
	    stack+=sizeof(int);
	    count++;
	}
    FREE(sp->mail_received);
    if (count) {
	*(int *)stack=0;
	stack+=sizeof(int);
	count++;
	sp->mail_received=(int *)MALLOC(count*sizeof(int));
	memcpy(sp->mail_received,oldstack,count*sizeof(int));
    }
    else sp->mail_received=0;
    stack=oldstack;
}


 /* view mail that has been received */

void view_received(player *p,char *str)
{
    char *oldstack,middle[80],*page_input;
    int page,pages,*scan,*scan_count,mcount=0,ncount=1,n;
    note *mail;
    saved_player *sp;
    oldstack=stack;
    
    if (*str && !isdigit(*str) && p->residency&(LOWER_ADMIN|ADMIN)) {
	page_input=next_space(str);
	if (*page_input) *page_input++=0;
	sp=find_saved_player(str);
	if (!sp) {
	    tell_player(p,"No such player in save files.\n");
	    return;
	}
	else scan=sp->mail_received;
	str=page_input;
    }
    else {
	if (!p->saved) {
	    tell_player(p,"You have no save file, and therefore no mail either.\n");
	    return;
	}
	sp=p->saved;
	scan=sp->mail_received;
    }

    if (!scan) {
	tell_player(p,"You have received no mail.\n");
	return;
    }

    p->saved_flags &= ~NEW_MAIL;

    for(scan_count=scan;*scan_count;scan_count++) mcount++;

    page=atoi(str);
    if (page<=0) page=1;
    page--;

    pages=(mcount-1)/(TERM_LINES-2);
    if (page>pages) page=pages;

    if (mcount==1) strcpy(middle,"You have received one letter");
    else sprintf(middle,"You have received %s letters",
		 number2string(mcount));
    pstack_mid(middle);

    ncount=page*(TERM_LINES-2);

    scan += ncount;

    for(n=0,ncount++;n<(TERM_LINES-1);n++,ncount++,scan++) {
	if (!*scan) break;
	mail=find_note(*scan);
	if (!mail) {
	    stack=oldstack;
	    tell_player(p,"Found mail that owner had deleted ...\n");
	    reconfigure_received_list(sp);
	    if (sp!=p->saved) {
		tell_player(p,"Reconfigured ... try again\n");
		return;
	    }
	    view_received(p,str);
	    return;
	}
	if (p->residency&(LOWER_ADMIN|ADMIN)) sprintf(stack,"(%d) [%d] ",mail->id,ncount);
	else sprintf(stack,"[%d] ",ncount);
	while(*stack) *stack++;
	if (ncount<10) *stack++=' ';
	strcpy(stack,mail->header);
	while(*stack) *stack++;
	if (mail->flags&ANONYMOUS) {
	    if (p->residency&(LOWER_ADMIN|ADMIN)) sprintf(stack," <%s>\n",mail->name);
	    else strcpy(stack,"\n");
	}
	else sprintf(stack," (%s)\n",mail->name);
	while(*stack) *stack++;
    }  

    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);

    *stack++=0;
    tell_player(p,oldstack);

    stack=oldstack;
}


 /* send a letter */


 void quit_mail(player *p)
 {
   tell_player(p,"Letter NOT posted.\n");
   remove_note((note *)p->edit_info->misc);
 }

 void end_mail(player *p,char *str)
 {
   note *mail;
   char *oldstack,*name_list,*body,*tcpy,*comp,*text;
   saved_player **player_list,**pscan,**pfill;
   player *on;
   int receipt_count,n,m,*received_lists,*r;
   oldstack=stack;

   mail=(note *)p->edit_info->misc;

   align(stack);
   player_list=(saved_player **)stack;
   receipt_count=saved_tag(p,mail->text.where);
   if (mail->text.where) FREE(mail->text.where);
   mail->text.where=0;
   if (!receipt_count) {
     tell_player(p,"No one to send the letter to !\n");
     remove_note(mail);
     stack=oldstack;
     if (p->edit_info->input_copy==mail_command) do_prompt(p,"Mail Mode >");
     return;
   }

   if (!p->saved) {
     tell_player(p,"Eeek, no save file !\n");
     remove_note(mail);
     stack=oldstack;
     if (p->edit_info->input_copy==mail_command) do_prompt(p,"Mail Mode >");
     return;
   }

   pscan=player_list;
   pfill=player_list;
   m=receipt_count;
   for(n=0;n<m;n++) 
     if (((*pscan)->residency==STANDARD_ROOMS) ||
	 ((*pscan)->residency==BANISHED)) {
       tell_player(p,"You can't send mail to a room, or someone who is banished.\n");
       receipt_count--;
       pscan++;
     }
     else {
       if ((mail->flags&ANONYMOUS) && ((*pscan)->saved_flags&NO_ANONYMOUS)) {
	 text=stack;
	 sprintf(stack,"%s is not receiving anonymous mail.\n",(*pscan)->lower_name);
	 stack=end_string(stack);
	 tell_player(p,text);
	 stack=text;
	 receipt_count--;
	 pscan++;
       }
       else *pfill++=*pscan++;
     }

   if (receipt_count>0) {
     pscan=player_list;
     name_list=stack;
     if (mail->flags&SUPRESS_NAME) {
       strcpy(stack,"Anonymous");
       stack=end_string(stack);
     }
     else {
       strcpy(stack,(*pscan)->lower_name);
       stack=strchr(stack,0);
       if (receipt_count>1) {
	 pscan++;
	 for(n=2;n<receipt_count;n++,pscan++) {
	   sprintf(stack,", %s",(*pscan)->lower_name);
	   stack=strchr(stack,0); 
	 }
	 sprintf(stack," and %s",(*pscan)->lower_name);
	 stack=strchr(stack,0); 
       }
       *stack++=0;
     }

     body=stack;
     sprintf(stack,"Sending mail to %s.\n",name_list);
     stack=end_string(stack);
     tell_player(p,body);
     stack=body;

     if (mail->flags&ANONYMOUS) 
       sprintf(stack,"Anonymous mail dated %s.\nSubject: %s\nTo: %s\n\n",
	       convert_time(mail->date),mail->header,name_list);
     else
       sprintf(stack,"Mail dated %s.\nSubject: %s\nTo: %s\nFrom: %s\n\n",
	       convert_time(mail->date),mail->header,name_list,mail->name);
     stack=strchr(stack,0);

     tcpy=p->edit_info->buffer;
     for(n=0;n<p->edit_info->size;n++) *stack++=*tcpy++;

     comp=stack;
     stack=store_string(stack,body);
     mail->text.length=(int)stack-(int)comp;
     mail->text.where=(char *)MALLOC(mail->text.length);
     memcpy(mail->text.where,comp,mail->text.length);

     mail->next_sent=p->saved->mail_sent;
     p->saved->mail_sent=mail->id;

     text=stack;
     command_type |= HIGHLIGHT;

     if (mail->flags&ANONYMOUS) 
       sprintf(stack,"     -=>  New mail, '%s' sent anonymously\n\n",
	       mail->header);
     else
       sprintf(stack,"     -=>  New mail, '%s' from %s.\n\n",
	       mail->header,mail->name);
     stack=end_string(stack);

     align(stack);
     received_lists=(int *)stack;

     pscan=player_list;
     for(n=0;n<receipt_count;n++,pscan++) {
       stack=(char *)received_lists;
       r=(*pscan)->mail_received;
       if (!r) m=2*sizeof(int);
       else {
	 for(m=2*sizeof(int);*r;r++,m+=sizeof(int)) {
	   *(int *)stack=*r;
	   stack+=sizeof(int);
	 }
       }
       *(int *)stack=mail->id;
       stack+=sizeof(int);
       *(int *)stack=0;
       if ((*pscan)->mail_received) FREE((*pscan)->mail_received);
       r=(int *)MALLOC(m);
       memcpy(r,received_lists,m);
       (*pscan)->mail_received=r;
       on=find_player_absolute_quiet((*pscan)->lower_name);
       if (on && on->saved_flags&MAIL_INFORM) {
	 tell_player(on,"\007\n");
	 tell_player(on,text);
       }
       else (*pscan)->saved_flags |= NEW_MAIL;
     }

     command_type &= ~HIGHLIGHT;

     mail->read_count=receipt_count;
     mail->flags &= ~NOT_READY;
     tell_player(p,"Mail posted....\n");
   }
   else tell_player(p,"No mail posted.\n");
   if (p->edit_info->input_copy==mail_command) do_prompt(p,"Mail Mode >");
   stack=oldstack;
 }


 void send_letter(player *p,char *str)
 {
   note *mail;
   char *subject,*scan,*first;
   int length;

   if (posted_count(p)>=p->max_mail) {
     tell_player(p,"Sorry, you have reached your mail limit.\n");
     return;
   }

   subject=next_space(str);

   if (!*subject) {
     tell_player(p,"Format: post <character(s)> <subject>\n");
     return;
   }

   *subject++=0;

   mail=create_note();

   first=first_char(p);
   scan=strstr(first,"post");
   if (scan && (scan!=first_char(p)) && (*(scan-1)=='a'))
     mail->flags |= ANONYMOUS;

   strcpy(mail->name,p->name);
   strncpy(mail->header,subject,MAX_TITLE-2);

   tell_player(p,"Enter main text of the letter...\n");
   *stack=0;	
   start_edit(p,MAX_ARTICLE_SIZE,end_mail,quit_mail,stack);
   if (p->edit_info) {
     p->edit_info->misc=(void *)mail;
     length=strlen(str)+1;
     mail->text.where=(char *)MALLOC(length);
     memcpy(mail->text.where,str,length);
   }
   else FREE(mail);
 }


 /* find the note corresponing to letter number */

 note *find_received(saved_player *sp,int n)
 {
   int *scan,count=1;
   note *mail;
   scan=sp->mail_received;
   if (!scan) return 0;
   for(;*scan;count++,scan++)
     if (count==n) {
       mail=find_note(*scan);
       if (!mail) {
	 reconfigure_received_list(sp);
	 return find_received(sp,n);
       }
       return mail;
     }
   return 0;
 }

 /* view a letter */

 void read_letter(player *p,char *str)
 {
   char *oldstack,*which;
   saved_player *sp;
   note *mail;
   oldstack=stack;

   which=str;
   sp=p->saved;
   if (!sp) {
     tell_player(p,"You have no save info.\n");
     return;
   }

   mail=find_received(sp,atoi(which));
   if (!mail) {
     sprintf(stack,"No such letter '%s'\n",which);
     stack=end_string(stack);
     tell_player(p,oldstack);
     stack=oldstack;
     return;
   }
   get_string(stack,mail->text.where);
   stack=end_string(stack);
   pager(p,oldstack,0);
   p->saved_flags &= ~NEW_MAIL;
   stack=oldstack;	
 }


 /* reply to a letter */

 void reply_letter(player *p,char *str)
 {
   note *mail,*old;
   char *indent,*body,*oldstack,*scan,*first;
   int length;

   oldstack=stack;

   if (!*str) {
     tell_player(p,"Format: reply <number>\n");
     return;
   }
   if (posted_count(p)>=p->max_mail) {
     tell_player(p,"Sorry, you have reached your mail limit.\n");
     return;
   }
   if (!p->saved) {
     tell_player(p,"Eeek, no saved bits.\n");
     return;
   }
   old=find_received(p->saved,atoi(str));
   if (!old) {
     sprintf(stack,"Can't find letter '%s'\n",str);
     stack=end_string(stack);
     tell_player(p,oldstack);
     stack=oldstack;
     return;
   }
   mail=create_note();

   first=first_char(p);
   scan=strstr(first,"reply");
   if (scan && (scan!=first_char(p)) && (*(scan-1)=='a'))
     mail->flags |= ANONYMOUS;

   if (old->flags&ANONYMOUS) mail->flags |= SUPRESS_NAME;
   strcpy(mail->name,p->name);

   if (strstr(old->header,"Re: ")==old->header) 
     strcpy(mail->header,old->header);
   else {
     sprintf(stack,"Re: %s",old->header);
     strncpy(mail->header,stack,MAX_TITLE-2);
   }

   indent=stack;
   get_string(stack,old->text.where);
   stack=end_string(stack);
   body=stack;

   sprintf(stack,"In reply to '%s'\n",old->header);
   stack=strchr(stack,0);

   while(*indent) {
     *stack++='>';
     *stack++=' ';
     while(*indent && *indent!='\n') *stack++=*indent++;
     *stack++='\n';
     indent++;
   }
   *stack++='\n';
   *stack++=0;

   tell_player(p,"Please trim letter as much as possible...\n");
   start_edit(p,MAX_ARTICLE_SIZE,end_mail,quit_mail,body);
   if (p->edit_info) {
     p->edit_info->misc=(void *)mail;
     length=strlen(old->name)+1;
     mail->text.where=(char *)MALLOC(length);
     memcpy(mail->text.where,old->name,length);
   }
   else FREE(mail);
   stack=oldstack;
 }


 /* reply to an article */

 void reply_article(player *p,char *str)
 {
   note *mail,*old;
   char *indent,*body,*oldstack,*scan,*first;
   int length;

   oldstack=stack;

   if (!*str) {
     tell_player(p,"Format: reply <no>\n");
     return;
   }
   if (posted_count(p)>=p->max_mail) {
     tell_player(p,"Sorry, you have reached your mail limit.\n");
     return;
   }
   if (!p->saved) {
     tell_player(p,"Eeek, no saved bits.\n");
     return;
   }
   old=find_news_article(atoi(str));
   if (!old) {
     sprintf(stack,"Can't find letter '%s'\n",str);
     stack=end_string(stack);
     tell_player(p,oldstack);
     stack=oldstack;
     return;
   }
   mail=create_note();

   first=first_char(p);
   scan=strstr(first,"reply");
   if (scan && (scan!=first_char(p)) && (*(scan-1)=='a'))
     mail->flags |= ANONYMOUS;

   if (old->flags&ANONYMOUS) mail->flags |= SUPRESS_NAME;
   strcpy(mail->name,p->name);

   if (strstr(old->header,"Re: ")==old->header) 
     strcpy(mail->header,old->header);
   else {
     sprintf(stack,"Re: %s",old->header);
     strncpy(mail->header,stack,MAX_TITLE-2);
   }

   indent=stack;
   get_string(stack,old->text.where);
   stack=end_string(stack);
   body=stack;

   sprintf(stack,"In your article '%s' you wrote ...\n",old->header);
   stack=strchr(stack,0);

   while(*indent) {
     *stack++='>';
     *stack++=' ';
     while(*indent && *indent!='\n') *stack++=*indent++;
     *stack++='\n';
     indent++;
   }
   *stack++='\n';
   *stack++=0;

   tell_player(p,"Please trim letter as much as possible...\n");
   start_edit(p,MAX_ARTICLE_SIZE,end_mail,quit_mail,body);
   if (p->edit_info) {
     p->edit_info->misc=(void *)mail;
     length=strlen(old->name)+1;
     mail->text.where=(char *)MALLOC(length);
     memcpy(mail->text.where,old->name,length);
   }
   else FREE(mail);
   stack=oldstack;
 }


 /* unlink and remove a mail note */

void unlink_mail(note *m)
{
    char *oldstack,*temp;
    saved_player *sp;
    int *change,recursive=0;
    note *scan,*tmp;
    if (!m) return;
    oldstack=stack;
    strcpy(stack,m->name);
    lower_case(stack);
    stack=end_string(stack);
    sp=find_saved_player(oldstack);
    if (!sp) {
	temp=stack;
	sprintf(stack,"(2) mail: %s - current: %s",m->name,
		current_player->name);
	stack=end_string(stack);
	log("error",temp);
	stack=temp;
	recursive=m->next_sent; 
    }
    else {
	change=&(sp->mail_sent);
	scan=find_note(sp->mail_sent);
	while(scan && scan!=m) {
	    change=&(scan->next_sent);
	    scan=find_note(scan->next_sent);
	}
	tmp=find_note(m->next_sent);
	if (tmp) *change=tmp->id;
	else *change=0;
    }
    remove_note(m);
    if (recursive) unlink_mail(find_note(recursive));
    stack=oldstack;
}


 /* remove sent mail */

 void delete_sent(player *p,char *str) 
 {
   int number;
   note *scan;
   number=atoi(str);
   if (number<=0) {
     tell_player(p,"Format: remove <mail number>\n");
     return;
   }
   if (!(p->saved)) {
     tell_player(p,"You have no save information, and therefore no mail either.\n");
     return;
   }
   scan=find_note(p->saved->mail_sent);
   for(number--;number;number--)
     if (scan) scan=find_note(scan->next_sent);
     else break;
   if (!scan) {
     tell_player(p,"You haven't sent that many mails.\n");
     return;
   }
   unlink_mail(scan);
   tell_player(p,"Removed mail ...\n");
 }


 /* remove received mail */

 void delete_received(player *p,char *str)
 {
   int number,count=0;
   int *make,*scan;
   char *oldstack;
   note *deleted;
   number=atoi(str);
   if (number<=0) {
     tell_player(p,"Format: delete <mail number>\n");
     return;
   }
   if (!(p->saved)) {
     tell_player(p,"You have no save information, and therefore no mail either.\n");
     return;
   }
   scan=p->saved->mail_received;
   if (!scan) {
     tell_player(p,"You have recieved no mail to delete.\n");
     return;
   }
   oldstack=stack;
   align(stack);
   make=(int *)stack;
   for(number--;number;number--) 
     if (*scan) {
       *make++=*scan++;
       count++;
     }
     else break;
   if (!*scan) {
     tell_player(p,"Not that many pieces of mail.\n");
     stack=oldstack;
     return;
   }
   deleted=find_note(*scan++);
   while(*scan) {
     *make++=*scan++;
     count++;
   }
   *make++=0;
   count++;
   if (p->saved->mail_received) FREE(p->saved->mail_received);
   if (count!=1) {
     p->saved->mail_received=(int *)MALLOC(sizeof(int)*count);
     memcpy(p->saved->mail_received,stack,sizeof(int)*count);
   }
   else p->saved->mail_received=0;
   if (deleted) {
     deleted->read_count--;
     if (!(deleted->read_count)) unlink_mail(deleted);
   }
   tell_player(p,"Mail deleted....\n");
   stack=oldstack;
 }




 /* admin ability to view notes */

 void view_note(player *p,char *str)
 {
   note *n;
   char *oldstack;
   oldstack=stack;
   if (!*str) {
     tell_player(p,"Format: view_note <number>\n");
     return;
   }
   n=find_note(atoi(str));
   if (!n) {
     tell_player(p,"Can't find note with that number.\n");
     return;
   }
   if (n->flags&NEWS_ARTICLE) 
       strcpy(stack,"News Article.\n");
   else
       strcpy(stack,"Mail (?).\n");
   stack=strchr(stack,0);
   sprintf(stack,"Posted on %s, by %s.\nRead count: %d\n",
	   convert_time(n->date),n->name,n->read_count);
   stack=strchr(stack,0);
   if (n->flags&ANONYMOUS) {
     strcpy(stack,"Posted anonymously.\n");
     stack=strchr(stack,0);
   }
   if (n->flags&NOT_READY) {
     strcpy(stack,"Not ready flag set.\n");
     stack=strchr(stack,0);
   }
   if (n->flags&SUPRESS_NAME) {
     strcpy(stack,"Name suppressed.\n");
     stack=strchr(stack,0);
  }
  sprintf(stack,"Next link -> %d\n",n->next_sent);
  stack=end_string(stack);
  pager(p,oldstack,0);
  stack=oldstack;
}



/* remove a note from */

void dest_note(player *p,char *str)
{
  note *n;

  if (!*str) {
    tell_player(p,"Format: dest_note <number>\n");
    return;
  }
  n=find_note(atoi(str));
  if (!n) {
    tell_player(p,"Can't find note with that number.\n");
    return;
  }
  remove_any_note(n);
  tell_player(p,"Note removed\n");
}



/* relink emergancy command */

void relink_note(player *p,char *str)
{
  char *to;
  int id1,id2;
  note *n;
  to=next_space(str);
  *to++=0;
  id1=atoi(str);
  id2=atoi(to);
/*  if (!id1 || !id2) {
    tell_player(p,"Format : relink <number> <number>\n");
    return;
  } */
  n=find_note(id1);
  if (!n) {
    tell_player(p,"Can't find first note\n");
    return;
  }
  n->next_sent=id2;
  tell_player(p,"next_sent pointer twiddled.\n");
  return;
}


/* recount emergancy command */

void recount_news(player *p,char *str)
{
  int article;
  note *scan;
  news_count=0;
  scan=find_note(news_start);
  if (scan) {
    article=scan->next_sent;
    for(;scan;article=scan->next_sent,news_count++) {
      scan=find_note(article);
      if (!scan) break;
    }
    news_count++;
  }
  tell_player(p,"Tis Done ...\n");
}
