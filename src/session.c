/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  session.c
*/

#include <string.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/types.h>


#include "config.h"
#include "player.h"
#include "fix.h"

extern char session[];
extern int  session_reset;
extern player *p_sess;
extern char *end_string(char *);

#ifdef MIPS
extern char *strstr(char *,char *);
#endif

extern char *word_time(int);

void set_session(player *p,char *str)
{
    char *oldstack;
    time_t t;
    player *scan;
    int wait,yessu=0;
    
    oldstack=stack;

    t=time(0);
    if (session_reset)
	wait=session_reset-(int)t;
    else
	wait=0;
    if (strlen(session)==0) 
	strncpy(session,"not set",MAX_SESSION-2);
    if (!*str) {
	sprintf(stack,"The session is currently '%s'\n",session);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    if (wait>0 && p!=p_sess) {
	sprintf(stack,"Session can be reset in %s\n",word_time(wait));
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    if (strlen(str)>55) {
	tell_player(p,"Too longer session name ...\n");
	stack=oldstack;
	return;
    }
    strcpy(session,str);
    sprintf(stack,"You reset the session message to be '%s'\n",str);
    stack=end_string(stack);
    tell_player(p,oldstack);

    /* reset comments */
    for (scan=flatlist_start;scan;scan=scan->flat_next)
	strncpy(scan->comment,"",MAX_COMMENT-2);

    stack=oldstack;
    sprintf(stack,"%s sets the session to be '%s'\n",p->name,str);
    stack=end_string(stack);
    
    command_type|=EVERYONE;
    
    for (scan=flatlist_start;scan;scan=scan->flat_next)
	if (scan!=p && !(scan->saved_flags&YES_SESSION))
	    tell_player(scan,oldstack);
    
    stack=oldstack;
 
    if (p!=p_sess || wait<=0)
	session_reset=t+(60*15);
    p_sess=p;
    
    sprintf(stack,"%s- %s",p->name,session);
    stack=end_string(stack);
    log("session",oldstack);
    stack=oldstack;
}



void reset_session(player *p,char *str)
{
    session_reset=0;
    tell_player(p,"Session timer reset\n");
}

void set_comment(player *p,char *str)
{
    char *oldstack;
    
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"You reset your session comment.\n");
	strncpy(p->comment,"",MAX_COMMENT-2);
	return;
    }
    strncpy(p->comment,str,MAX_COMMENT-2);
    sprintf(stack,"You set your session comment to be '%s'\n",p->comment);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

/*
void view_session(player *p,char *str)
{
    char *oldstack,middle[80];
    player *scan;
    int page,pages=0,count;
    oldstack=stack;

    page=atoi(str);
    if (page<=0) page=1;
    page--;
  
comment out line        pages=(current_players-1)/(TERM_LINES-2); 

    for (scan=flatlist_start;scan;scan=scan->flat_next) 
	if (scan->comment[0]!=0 || scan==p_sess) pages++;
	
    if (page>pages) page=pages;
  
    if (strlen(session)==0) 
	strncpy(session,"not set",MAX_SESSION-2);

    strcpy(middle,session);

    pstack_mid(middle);
    
    count=page*(TERM_LINES-2);
    for(scan=flatlist_start;count;scan=scan->flat_next)
	if (!scan) {
	    tell_player(p,"Bad who listing, abort.\n");
	    log("error","Bad who list (session.c)");
	    stack=oldstack;
	    return;
	}
	else if (scan->name[0]) count--;
    for(count=0;(count<(TERM_LINES-1) && scan);scan=scan->flat_next)
	if ((scan->name[0] && scan->location) && (scan==p_sess || 
						  scan->comment[0]!=0)) {
	    if (scan==p_sess)
		sprintf(stack,"%-20s* %s\n",scan->name,scan->comment);
	    else
		sprintf(stack,"%-20s- %s\n",scan->name,scan->comment);
	    stack=strchr(stack,0);
	    count++;
	}  
   
    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);
    *stack++=0;
    if (count)
	tell_player(p,oldstack);
    else {
	stack=oldstack;
	sprintf(stack,"Noone has commented on the session '%s'\n",session);
	stack=end_string(stack);
	tell_player(p,oldstack);
    }
    stack=oldstack;
}
*/

void view_session(player *p,char *str)
{
    char *oldstack,middle[80];
    player *scan;
    int page,pages,count;
    oldstack=stack;

    page=atoi(str);
    if (page<=0) page=1;
    page--;
  
    pages=(current_players-1)/(TERM_LINES-2);
    if (page>pages) page=pages;
  
    if (strlen(session)==0) 
	strncpy(session,"not set",MAX_SESSION-2);

    strcpy(middle,session);

    pstack_mid(middle);

    count=page*(TERM_LINES-2);
    for(scan=flatlist_start;count;scan=scan->flat_next)
	if (!scan) {
	    tell_player(p,"Bad who listing, abort.\n");
	    log("error","Bad who list (tag.c)");
	    stack=oldstack;
	    return;
	}
	else if (scan->name[0]) count--;
    for(count=0;(count<(TERM_LINES-1) && scan);scan=scan->flat_next)
	if (scan->name[0] && scan->location) {
	    if (scan==p_sess)
		sprintf(stack,"%-20s* ",scan->name);
	    else
		sprintf(stack,"%-20s- ",scan->name);
	    stack=strchr(stack,0);
	    strcpy(stack,scan->comment);
	    stack=strchr(stack,0);
	    *stack++='\n';
	    count++;
	}  
    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);
    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}

/* REPLYS */

/* save a list of who sent you the last list of names, for reply  */

void make_reply_list(player *p,player **list,int matches,int friend)
{
    char *oldstack,*send,*mark,*scan;
    player **step;
    time_t t;
    int i,count,timeout;

    oldstack=stack;

    t=time(0);
    timeout=t+(2*60);

    if (matches<2) 
	return;

    sprintf(stack,"%s.,",p->lower_name);
    count=strlen(stack);
    stack=strchr(stack,0);
    for (step=list,i=0;i<matches;i++,step++) 
	if (*step!=p) {
	    count+=(strlen((*step)->lower_name)+1);
	    if (count<(MAX_REPLY-2)) {
		sprintf(stack,"%s.,",(*step)->lower_name);
		stack=strchr(stack,0);
	    }
	    else
		log("reply","Too longer reply string !!!");
	}
    stack=end_string(stack);
    
    /* should have string in oldstack */
    
    send=stack;
    for (step=list,i=0;i<matches;i++,step++,mark=0) {
	mark=strstr(oldstack,(*step)->lower_name);
	if (!mark) {
	    log("reply","Can't find player in reply string!!");
	    return;
	}
	for (scan=oldstack;scan!=mark;)
	    *stack++=*scan++;
	while (*scan!=',') scan++;
	scan++;
	while (*scan)
	    *stack++=*scan++;
	*stack=0;
	strcpy((*step)->reply,send);
	(*step)->reply_time=timeout;
	stack=send;
    }
}

/* Reply command itself */

void reply(player *p,char *str)
{
    char *oldstack;

    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: reply <msg>\n");
	return;
    }
    if (!*(p->reply) || (p->reply_time<(int)time(0))) {
	tell_player(p,"You don't have anyone to reply to!\n");
	return;
    }
    sprintf(stack,"%s ",p->reply);
    stack=strchr(stack,0);
    strcpy(stack,str);
    stack=end_string(stack);
    tell(p,oldstack);
    stack=oldstack;
}
  

void report_error(player *p,char *str)
{
    char *oldstack;
    if (!*str) {
	tell_player(p,"Format: bug <whatever the sodding bug is>\n");
	return;
    }
    if (strlen(str)>60) {
	tell_player(p,"Make it a little smaller.\n");
	return;
    }
    tell_player(p,"Done.\n");
    oldstack=stack;
    sprintf(stack,"%s: %s",p->name,str);
    stack=end_string(stack);
    log("bug",oldstack);
    stack=oldstack;
}    

void show_exits(player *p,char *str)
{
    if (p->saved_flags&SHOW_EXITS) {
	tell_player(p,"You won't see exits when you enter a room now.\n");
	p->saved_flags&=~SHOW_EXITS;
    }
    else {
	tell_player(p,"When you enter a room you will now see the exits.\n");
	p->saved_flags|=SHOW_EXITS;
    }
}
