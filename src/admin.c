/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* 
  admin.c
*/

#ifndef MIPS
#include <malloc.h>
#endif

#include <string.h>
#include <memory.h>
#include <ctype.h>
#include <time.h>

#include "config.h"
#include "player.h"

#ifdef MIPS
typedef int time_t;
#endif





/* externs */
extern void swap_list_names(char *,char *);
extern void lower_case(char *);
extern char *do_crypt(char *,player *);
extern char *end_string(),*next_space(),*tag_string(),*bit_string();
extern player *find_player_global(),*find_player_global_quiet(char *),
    *create_player();
extern saved_player *find_saved_player();
extern int remove_player_file(),set_update();
extern int get_flag();
extern void hard_load_one_file(),sync_to_file(),remove_entire_list(),
    destroy_player();
extern player *find_player_absolute_quiet(char *);
extern file newban_msg,nonewbies_msg,connect_msg,motd_msg,banned_msg,
    banish_file,banish_msg,full_msg,newbie_msg,newpage1_msg,newpage2_msg,
    disclaimer_msg,splat_msg,load_file(char *),load_file_verbose(char *,int),
    banished_p;
 extern int match_banish();
 extern void soft_eject(player *,char *);
 extern player *find_player_absolute_quiet(char *);
 extern int remove_banish_name(char *,char *),quick_banish_check(char *,int);
 /* interns */

int check_privs(int,int);


 flag_list permission_list[]={
     {"base",BASE},
     {"list",LIST},
     {"echo",ECHO_PRIV},
     {"no_timeout",NO_TIMEOUT},
     {"mail",MAIL},
     {"build",BUILD},
     {"session",SESSION},
     {"off_duty",OFF_DUTY},
     {"residency",BASE|BUILD|LIST|ECHO_PRIV|MAIL|SESSION},
     {"lower_admin",LOWER_ADMIN},
     {"lower_su",(OFF_DUTY|LOWER_SU)},
     {"upper_su",(OFF_DUTY|UPPER_SU)},
     {"admin",ADMIN},
     {0,0} };


#if defined(MIPS) || defined(LINUX)
void show_malloc(player *p,char *str)
{
  tell_player(p,"barf !, you can't do that on this platform\n");
}
#else
     
 /* malloc data */

void show_malloc(player *p,char *str)
{
    char *oldstack;
    struct mallinfo i;

    oldstack=stack;
    i=mallinfo();

    sprintf(stack,"Total arena space\t%d\n"
	    "Ordinary blocks\t\t%d\n"
	    "Small blocks\t\t%d\n"
	    "Holding blocks\t\t%d\n"
	    "Space in headers\t\t%d\n"
	    "Small block use\t\t%d\n"
	    "Small blocks free\t%d\n"
	    "Ordinary block use\t%d\n"
	    "Ordinary block free\t%d\n"
 #if defined( ULTRIX ) || defined( SOLARIS )
	    "Keep cost\t\t%d\n",
 #else
	    "Keep cost\t\t%d\n"
	    "Small block size\t\t%d\n"
	    "Small blocks in holding\t%d\n"
	    "Rounding factor\t\t%d\n"
	    "Ordinary block space\t%d\n"
	    "Ordinary blocks alloc\t%d\n"
	    "Tree overhead\t%d\n",
 #endif
	    i.arena,i.ordblks,i.smblks,i.hblks,i.hblkhd,i.usmblks,
 #if defined( ULTRIX ) || defined ( SOLARIS )
	    i.fsmblks,i.uordblks,i.fordblks,i.keepcost);
 #else
    i.fsmblks,i.uordblks,i.fordblks,i.keepcost,
    i.mxfast,i.nlblks,i.grain,i.uordbytes,i.allocated,i.treeoverhead);
 #endif
 stack=end_string(stack);
 tell_player(p,oldstack);
 stack=oldstack;
 }

#endif


 /* view logs */

 void vlog(player *p,char *str)
 {
     char *oldstack;
     file log;
     oldstack=stack;

     switch(*str) {
     case 0:
     case '?':
	 tell_player(p,"Log files you can view: angel, boot, connection, ejects, error, super, dump, timeouts, sync, site, stack, bug, help\n");
	 return;
     case '.':
	 tell_player(p,"uh-uh, you can't do that !\n");
	 return;
     }
     sprintf(stack,"logs/%s.log",str);
     stack=end_string(stack);
     log=load_file_verbose(oldstack,0);
     if (log.where) {
	 if (*(log.where)) 
	     pager(p,log.where,1);
	 else {
	     sprintf(oldstack,"Couldn't find logfile 'logs/%s.log'\n",str);
	     stack=end_string(oldstack);
	     tell_player(p,oldstack);
	 }
	 free(log.where);
     }
     stack=oldstack;
 }


 /* net stats */

void netstat(player *p,char *str)
{
     char *oldstack;
     oldstack=stack;

     sprintf(stack,"Total bytes:\t\t(I) %d\t(O) %d\n"
	     "Average bytes:\t\t(I) %d\t\t(O) %d\n"
	     "Bytes per second:\t(I) %d\t\t(O) %d\n"
	     "Total packets:\t\t(I) %d\t(O) %d\n"
	     "Average packets:\t(I) %d\t\t(O) %d\n"
	     "Packets per second:\t(I) %d\t\t(O) %d\n",
	     in_total,out_total,in_average,out_average,in_bps,out_bps,
	     in_pack_total,out_pack_total,in_pack_average,
	     out_pack_average,in_pps,out_pps);
     stack=end_string(stack);
     tell_player(p,oldstack);
     stack=oldstack;
 }


 /* warn someone */

void warn(player *p,char *str)
{
    char *oldstack,*warning;
    player *p2;

    command_type=PERSONAL;

    if (p->saved_flags&BLOCK_TELLS) {
	tell_player(p,"You can't warn people when you yourself are blocking"
		    " tells.\n");
	return;
    }

    warning=next_space(str);
    if (*warning) *warning++=0;
    if (!*warning) {
	tell_player(p,"Format: warn <player> <message>\n");
	return;
    }

    p2=find_player_global(str);
    if (!p2) return;

    if (p2==p) {
	tell_player(p,"Bye, you sponge ... \n");
	quit(p,0);
	return;
    }
    tell_player(p2,"\007\n");
    oldstack=stack;
    sprintf(stack,"%s warns you: %s\n\n",p->name,warning);
    stack=end_string(stack);
    non_block_tell(p2,oldstack);

    command_type=0;
    stack=oldstack;
    sprintf(stack,"You warn %s: %s\n",p2->name,warning);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
    sprintf(oldstack,"-=> %s warns %s: %s\n",p->name,p2->name,warning);
    stack=end_string(stack);
    p->flags|=NO_SU_WALL;
    su_wall(oldstack);
    stack=oldstack;
    sprintf(oldstack,"%s warns %s: %s",p->name,p2->name,warning);
    stack=end_string(stack);
    log("warn",oldstack);
    stack=oldstack;
}


 /* trace someone and check against email */

void trace(player *p,char *str)
{
    char *oldstack;
    player *p2;
    oldstack=stack;

    if (!*str) {
	tell_player(p,"Format: trace <person>\n");
	return;
    }
    p2=find_player_global(str);
    if (!p2) return;

    if (p2->residency==NON_RESIDENT)
	sprintf(stack,"%s is non resident.\n",p2->name);
    else 
	if (p2->email[0]) {
	    if (p2->email[0]==-1) 
		sprintf(stack,"%s has declared no email address.\n",p2->name);
	    else {
		sprintf(stack,"%s [%s]\n",p2->name,p2->email);
		if (p2->saved_flags&PRIVATE_EMAIL) {
		    while(*stack!='\n') stack++;
		    strcpy(stack," (private)\n");
		}
	    }
	}
	else
	    sprintf(stack,"%s has not set an email address.\n",p2->name);
    stack=strchr(stack,0);
    sprintf(stack,"%s has connected from %s.\n",p2->name,p2->inet_addr);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}


 /* list people who are from the same site */

void same_site(player *p,char *str)
{
    char *oldstack,*text;
    player *p2;
    oldstack=stack;
    if (isalpha(*str)) {
	p2=find_player_global(str);
	if (!p2) {
	    stack=oldstack;
	    return;
	}
	str=stack;
	text=p2->num_addr;
	while(isdigit(*text)) *stack++=*text++;
	*stack++='.';
	*text++;
	while(isdigit(*text)) *stack++=*text++;
	*stack++='.';
	*stack++='*';
	*stack++='.';
	*stack++='*';
	*stack++=0;
    }
    if (!isdigit(*str)) {
	tell_player(p,"Format: site <inet_number> or site <person>\n");
	stack=oldstack;
	return;
    }
    text=stack;
    sprintf(stack,"People from .. %s\n",str);
    stack=strchr(stack,0);
    for(p2=flatlist_start;p2;p2=p2->flat_next) 
	if (match_banish(p2,str)) {
	    sprintf(stack,"(%s) %s : %s ",p2->num_addr,p2->inet_addr,p2->name);
	    stack=strchr(stack,0);
	    if (p2->residency==NON_RESIDENT) {
		strcpy(stack,"non resident.\n");
		stack=strchr(stack,0);
	    } 
	    else 
		if (p2->email[0]) {
		    if (p2->email[0]==-1) 
			strcpy(stack,"No email address.\n");
		    else {
			if (p->residency&ADMIN || 
			    !(p2->saved_flags&PRIVATE_EMAIL)) {
			    sprintf(stack,"[%s]",p2->email);
			    stack=strchr(stack,0);
			}
			if (p2->saved_flags&PRIVATE_EMAIL) {
			    strcpy(stack," (private)");
			    stack=strchr(stack,0);
			}
		    }
		    *stack++='\n';
		}
		else {
		    strcpy(stack,"Email not set\n");
		    stack=strchr(stack,0);
		}
	}
    *stack++=0;
    tell_player(p,text);
    stack=oldstack;
}


 /* crash ! */

void crash(player *p,char *str)
{
    char *flop=0;
    *flop=-1;
}

 /* reload everything */

void reload(player *p,char *str)
{
    tell_player(p,"Loading help\n");
    init_help();
    tell_player(p,"Loading messages\n");
    if (newban_msg.where) FREE(newban_msg.where);
    if (nonewbies_msg.where) FREE(nonewbies_msg.where);
    if (connect_msg.where) FREE(connect_msg.where);
    if (motd_msg.where) FREE(motd_msg.where);
    if (banned_msg.where) FREE(banned_msg.where);
    if (banish_file.where) FREE(banish_file.where);
    if (banish_msg.where) FREE(banish_msg.where);
    if (banished_p.where) FREE(banished_p.where);
    if (full_msg.where) FREE(full_msg.where);
    if (newbie_msg.where) FREE(newbie_msg.where);
    if (newpage1_msg.where) FREE(newpage1_msg.where);
    if (newpage2_msg.where) FREE(newpage2_msg.where);
    if (disclaimer_msg.where) FREE(disclaimer_msg.where);
    if (splat_msg.where) FREE(splat_msg.where);
#ifdef PC
    newban_msg=load_file("files\\newban.msg");
    nonewbies_msg=load_file("files\\nonew.msg");
    connect_msg=load_file("files\\connect.msg");
    motd_msg=load_file("files\\motd.msg");
    banned_msg=load_file("files\\banned.msg");
#else
    newban_msg=load_file("files/newban.msg");
    nonewbies_msg=load_file("files/nonew.msg");
    connect_msg=load_file("files/connect.msg");
    motd_msg=load_file("files/motd.msg");
    banned_msg=load_file("files/banned.msg");
#endif
    banish_file=load_file("files/banish");
    banish_msg=load_file("files/banish.msg");
    banished_p=load_file("files/players/banished_players");
    full_msg=load_file("files/full.msg");
    newbie_msg=load_file("files/newbie.msg");
    newpage1_msg=load_file("files/newpage1.msg");
    newpage2_msg=load_file("files/newpage2.msg");
    disclaimer_msg=load_file("files/disclaimer.msg");
    splat_msg=load_file("files/splat.msg");
    tell_player(p,"Done\n");
}


/* edit the banish file from the program */

void quit_banish_edit(player *p)
{
    tell_player(p,"Leaving without changes.\n");
}

void end_banish_edit(player *p)
{
    if (banish_file.where) FREE(banish_file.where);
    banish_file.length=p->edit_info->size;
    banish_file.where=(char *)MALLOC(banish_file.length);
    memcpy(banish_file.where,p->edit_info->buffer,banish_file.length);
    tell_player(p,"Banish file temp changed.\n");
}

void banish_edit(player *p,char *str)
{
    start_edit(p,10000,end_banish_edit,quit_banish_edit,banish_file.where);
}
/* the eject command , muhahahahaa */



void squish(player *p,char *str)
{
    time_t t;
    int nologin=0;
    char *oldstack,*text,*num;
    player *e;
    
    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: squish <person/s> or squish <person> <time>\n");
	return;
    }
    t=time(0);
    if (num=strrchr(str,' '))
	nologin=atoi(num);
    if (nologin>(60*10)) {
	tell_player(p,"That amount of time is too harsh, set to 10 mins now "
		    "...\n");
	nologin=(60*10);
    }
    if (!nologin) 
	nologin=60;
    else
	*num=0;
    while(*str) {
	while(*str && *str!=',') *stack++=*str++;
	if (*str) str++;
	*stack++=0;
	if (*oldstack) {
	    e=find_player_global(oldstack);
	    if (e) {
		text=stack;
		if (!check_privs(p->residency,e->residency)) {
		    tell_player(p,"No way pal !!!\n");
		    sprintf(stack,"-=> %s tried to squish you.\n",p->name);
		    stack=end_string(stack);
		    tell_player(e,text);
		    stack=text;
		    sprintf(stack,"%s failed to squish %s",p->name,e->name);
		    stack=end_string(stack);
		    log("ejects",text);
		    stack=text;
		}
		else {
		    strcpy(stack,"\n\nA huge hand reaches down from the "
			   "skies and pokes you viciously with a wet "
			   "sponge\n\n");
		    stack=end_string(stack);
		    tell_player(e,text);
		    e->squished=t+nologin;
		    stack=text;
		    quit(e,0);
		    sprintf(stack,"-=> %s is squished, and not at all "
			    "happy about it.\n",e->name);
		    stack=end_string(stack);
		    tell_room(e->location,text);
		    stack=text;
		    sprintf(stack,"-=> %s squishes %s.\n",p->name,e->name);
		    stack=end_string(stack);
		    su_wall(text);
		    stack=text;
		    sprintf(text,"%s squishes %s",p->name,e->name);
		    stack=end_string(text);
		    log("ejects",text);
		    stack=text;
		}
	    }
	}
	stack=oldstack;
    }
}

/* reset person (in case the su over does it (which wouldn't be like an su
   at all.. nope no no))
   */

void reset_squish(player *p,char *str)
{
    char *oldstack;
    player *dummy;
    
    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: reset_squish <person>\n");
	return;
    }
    dummy=(player *)MALLOC(sizeof(player));
    memset(dummy,0,sizeof(player));
    strcpy(dummy->lower_name,str);
    lower_case(dummy->lower_name);
    dummy->fd=p->fd;
    if (!load_player(dummy)) {
	tell_player(p,"No such person in saved files.\n");
	FREE(dummy);
	return;
    }
    switch (dummy->residency) {
    case BANISHED:
    case STANDARD_ROOMS:
	tell_player(p,"Not a player.\n");
	FREE(dummy);
	return;
    default:
	break;
    }
    dummy->squished=0;
    dummy->location=(room *)-1;
    save_player(dummy);
    sprintf(stack," Reset the squish time on %s ...\n",dummy->name);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
    FREE(dummy);
}

/* SPLAT!!!! Wibble plink, if I do say so myself */

void soft_splat(player *p,char *str)
{
    char *oldstack,*reason;
    player *dummy;
    int no1,no2,no3,no4;
    
    if (!(reason=strchr(str,' '))) {
	tell_player(p,"Format: splat <person> <reason>\n");
	return;
    }
    oldstack=stack;
    *reason++=0;
    dummy=find_player_global(str);
    if (!dummy) return;
    sprintf(stack,"%s SPLAT: %s",str,reason);
    stack=end_string(stack);
    soft_eject(p,oldstack);
    *reason=' ';
    stack=oldstack;
    if (!(dummy->flags&CHUCKOUT)) return;
    soft_timeout=time(0)+(5*60);    
    sscanf(dummy->num_addr,"%d.%d.%d.%d",&no1,&no2,&no3,&no4);
    soft_splat1=no1;
    soft_splat2=no2;
    sprintf(stack,"-=> Site %d.%d.*.* banned to newbies for 5 minutes.\n",
	    no1,no2);
    stack=end_string(stack);
    su_wall(oldstack);
    stack=oldstack;
}

void splat_player(player *p,char *str)
{
    time_t t;
    char *oldstack,*space;
    player *dummy;
    int no1,no2,no3,no4,tme=0;
    
    if (!(p->residency&(UPPER_SU|LOWER_ADMIN|ADMIN))) {
	soft_splat(p,str);
	return;
    }

    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: splat <person> <time>\n");
	return;
    }
    if (space=strchr(str,' ')) {
	*space++=0;
	tme=atoi(space);
    }
    if (tme<1 || tme>10) {
	tell_player(p,"That's not a very nice amount of time.  Set to 10 "
		    "minutes ...\n");
	tme=10;
    }
    dummy=find_player_global(str);
    if (!dummy) return ;
    squish(p,dummy->lower_name);
    if (!(dummy->flags&CHUCKOUT)) return;
    t=time(0);
    splat_timeout=t+(tme*60);
    sscanf(dummy->num_addr,"%d.%d.%d.%d",&no1,&no2,&no3,&no4);
    splat1=no1;
    splat2=no2;
    sprintf(stack,"-=> Site %d.%d.*.* banned for %d minutes.\n",no1,
	    no2,tme);
    stack=end_string(stack);
    su_wall(oldstack);
    stack=oldstack;
}


void unsplat(player *p,char *str)
{
    char *oldstack;
    time_t t;
    int number;
    
    oldstack=stack;
    
    t=time(0);
    if (!*str || !(number=atoi(str))) {
	number=splat_timeout-(int)t;
	if (number<=0) {
	    tell_player(p,"No site banned atm.\n");
	    return;
	}
	sprintf(stack,"Site %d.%d.*.* is banned for %d more seconds.\n",
		splat1,splat2,number);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    sprintf(stack,"Site %d.%d.*.* now banned for a further %d seconds.\n",
	    splat1,splat2,number);
    splat_timeout=(int)t+number;
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}


/* the eject command (too) , muhahahahaa */

void soft_eject(player *p,char *str)
{
    char *oldstack,*text,*reason;
    player *e;
    
    oldstack=stack;
    reason=next_space(str);
    if (*reason) *reason++=0;
    if (!*reason) {
	tell_player(p,"Format: eject <person> <reason>\n");
	return;
    }
    e=find_player_global(str);
    if (e) {
	text=stack;
	if (e->residency!=NON_RESIDENT) {
	    tell_player(p,"Yeah, dream on.. They're resident.\n");
	    sprintf(stack,"%s tried to eject %s",p->name,e->name);
	    stack=end_string(stack);
	    log("ejects",text);
	    stack=text;
	}
	else {
	    strcpy(stack,"\n\nYou have been ejected !!!\n\nWhat a bummer!\n\n");
	    stack=end_string(stack);
	    tell_player(e,text);
	    stack=text;
	    quit(e,0);
	    sprintf(stack,"-=> %s has been ejected by some superuser !!\n",
		    e->name);
	    stack=end_string(stack);
	    tell_room(e->location,text);
	    stack=text;
	    sprintf(stack,"-=> %s ejects %s\n",p->name,e->name);
	    stack=end_string(stack);
	    su_wall(text);
	    stack=text;
	    sprintf(stack,"%s - %s : %s",p->name,e->name,reason);
	    stack=end_string(stack);
	    log("ejects",text);
	    stack=text;
	}
    }
    stack=oldstack;
}

/* similar to shout but only goes to super users (eject and higher) */

void su(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    
    command_type =0;
    
    if (!*str) {
	tell_player(p,"Format: su <message>\n");
	return;
    }
    if (p->flags&BLOCK_SU) {
	tell_player(p,"You can't do sus when you're ignoring them.\n");
	return;
    }
    
    if (*str==';') {
	str++;
	while(*str==' ') str++;
	sprintf(stack,"<%s %s>\n",p->name,str);
    }
    else
	sprintf(stack,"<%s> %s\n",p->name,str);
    stack=end_string(stack);
    su_wall(oldstack);
    stack=oldstack;
}

/* su-emote.. it's spannerish, I know, but what the hell */

void suemote(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    
    command_type = 0;
    
    if (!*str) {
	tell_player(p,"Format: se <message>\n");
	return;
    }
    sprintf(stack,"<%s %s>\n",p->name,str);
    stack=end_string(stack);
    su_wall(oldstack);
    stack=oldstack;
}

 /* toggle whether the program is globally closed to newbies */

void close_to_newbies(player *p,char *str)
{
    char *oldstack;
    int wall=0;
    oldstack=stack;

    if (!strcasecmp("off",str) && sys_flags&CLOSED_TO_NEWBIES) {
	sys_flags &= ~CLOSED_TO_NEWBIES;
	wall=1;
    }
    else 
	if (!strcasecmp("on",str) && !(sys_flags&CLOSED_TO_NEWBIES)) {
	    sys_flags |= CLOSED_TO_NEWBIES;
	    wall=1;
	}
	else 
	    wall=0;

    if (sys_flags&CLOSED_TO_NEWBIES) {
	if (!wall)
	    tell_player(p,"Program is closed to all newbies.\n");
	sprintf(oldstack,"\n<%s closes the prog to newbies>\n\n",p->name);
    }
    else {
	if (!wall)
	    tell_player(p,"Program is open to newbies.\n");
	sprintf(oldstack,"\n<%s opens the prog to newbies>\n\n",p->name);
    }
    stack=end_string(oldstack);
    if (wall) su_wall(oldstack);
    stack=oldstack;
}

 /* command to list lots of info about a person */

void check_info(player *p,char *str)
{
    player *dummy,*p2;
    char *oldstack;

    if (!*str) {
	tell_player(p,"Format check info <player>\n");
	return;
    }
    if (quick_banish_check(str,1)) return;
    oldstack=stack;

    dummy=(player *)MALLOC(sizeof(player));
    memset(dummy,0,sizeof(player));

    p2=find_player_global(str);
    if (p2) 
	memcpy(dummy,p2,sizeof(player));
    else {
	strcpy(dummy->lower_name,str);
	lower_case(dummy->lower_name);
	dummy->fd=p->fd;
	if (!load_player(dummy)) {
	    tell_player(p,"No such person in saved files.\n");
	    FREE(dummy);
	    return;
	}
    }

    switch(dummy->residency) {
    case BANISHED:
	tell_player(p,"BANISHED.\n");
	FREE(dummy);
	return;
    case STANDARD_ROOMS:
	tell_player(p,"Standard rooms file\n");
	FREE(dummy);
	return;
    default:
	sprintf(stack,"          BN     LB       IE EG    A\n"
		"Residency %s\n",bit_string(dummy->residency));
	break;
    }
    stack=strchr(stack,0);

    sprintf(stack,"%s %s %s\n%s\n%s\n%s %s\nEMAIL:%s\n",
	    dummy->pretitle,dummy->name,dummy->title,dummy->description,
	    dummy->plan,dummy->name,dummy->enter_msg,dummy->email);
    stack=strchr(stack,0);
    switch(dummy->gender) {
    case MALE:
	strcpy(stack,"Gender set to male.\n");
	break;
    case FEMALE:
	strcpy(stack,"Gender set to female.\n");
	break;
    case OTHER:
	strcpy(stack,"Gender set to something.\n");
	break;
    case VOID_GENDER:
	strcpy(stack,"Gender not set.\n");
	break;  
    }
    stack=strchr(stack,0);
    if ((dummy->password[0])<=0) {
	strcpy(stack,"NO PASSWORD SET\n");
	stack=strchr(stack,0);
    }
    sprintf(stack,"            CLTSHPQEPRSHEMA\n"         
	    "Saved flags %s\n",bit_string(dummy->saved_flags));
    stack=strchr(stack,0);
    sprintf(stack,"      PRNREPCPT\n"         
	    "flags %s\n",bit_string(dummy->flags));
    stack=strchr(stack,0);
    sprintf(stack,"Max: rooms %d, exits %d, autos %d, list %d\n",
	    dummy->max_rooms,dummy->max_exits,dummy->max_autos,dummy->max_list);
    stack=strchr(stack,0);
    sprintf(stack,"Term: width %d, wrap %d\n",
	    dummy->term_width,dummy->word_wrap);
    stack=strchr(stack,0);
    if (dummy->script) {
	sprintf(stack,"Scripting on for another %s.\n",word_time(dummy->script));
	stack=strchr(stack,0);
    }
    *stack++=0;
    tell_player(p,oldstack);
    FREE(dummy);
    stack=oldstack;
}


 /* command to check IP addresses */

void view_ip(player *p,char *str)
{
    player *scan;
    char *oldstack,middle[80];
    int page,pages,count;
    oldstack=stack;
    if (isalpha(*str)) {
	scan=find_player_global(str);
	stack=oldstack;
	if (!scan) return;
	sprintf(stack,"%s is logged in from %s.\n",scan->name,scan->inet_addr);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }

    page=atoi(str);
    if (page<=0) page=1;
    page--;

    pages=(current_players-1)/(TERM_LINES-2);
    if (page>pages) page=pages;

    if (current_players==1) 
	strcpy(middle,"There is only you on the program at the moment");
    else
	sprintf(middle,"There are %s people on the program",number2string(current_players));
    pstack_mid(middle);

    count=page*(TERM_LINES-2);
    for(scan=flatlist_start;count;scan=scan->flat_next)
	if (!scan) {
	    tell_player(p,"Bad where listing, abort.\n");
	    log("error","Bad where list");
	    stack=oldstack;
	    return;
	}
	else if (scan->name[0]) count--;
   
    for(count=0;(count<(TERM_LINES-1) && scan);scan=scan->flat_next)
	if (scan->name[0] && scan->location) {
	    if (scan->flags&SITE_LOG) *stack++='*';
	    else *stack++=' ';
	    sprintf(stack,"%s is logged in from %s.\n",scan->name,scan->inet_addr);
	    stack=strchr(stack,0);
	    count++;
	}
    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);

    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}


 /* command to view email status about people on the prog */

void view_player_email(player *p,char *str)
{
    player *scan;
    char *oldstack,middle[80];
    int page,pages,count;
    oldstack=stack;

    page=atoi(str);
    if (page<=0) page=1;
    page--;

    pages=(current_players-1)/(TERM_LINES-2);
    if (page>pages) page=pages;

    if (current_players==1) 
	strcpy(middle,"There is only you on the program at the moment");
    else
	sprintf(middle,"There are %s people on the program",number2string(current_players));
    pstack_mid(middle);

    count=page*(TERM_LINES-2);
    for(scan=flatlist_start;count;scan=scan->flat_next)
	if (!scan) {
	    tell_player(p,"Bad where listing, abort.\n");
	    log("error","Bad where list");
	    stack=oldstack;
	    return;
	}
	else if (scan->name[0]) count--;

    for(count=0;(count<(TERM_LINES-1) && scan);scan=scan->flat_next)
	if (scan->name[0] && scan->location) {
	    if (scan->residency==NON_RESIDENT)
		sprintf(stack,"%s is non resident.\n",scan->name);
	    else 
		if (scan->email[0]) {
		    if (scan->email[0]==-1) 
			sprintf(stack,"%s has declared no email address.\n",scan->name);
		    else {
			sprintf(stack,"%s [%s]\n",scan->name,scan->email);
			if (scan->saved_flags&PRIVATE_EMAIL) {
			    while(*stack!='\n') stack++;
			    strcpy(stack," (private)\n");
			}
		    }
		}
		else
		    sprintf(stack,"%s has not set an email address.\n",scan->name);
	    stack=strchr(stack,0);
	    count++;
	}
    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);

    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}


 /* command to validate lack of email */

 void validate_email(player *p,char *str)
 {
   player *p2;
   p2=find_player_global(str);
   if (!p2) return;
   p2->email[0]=-1;
   p2->email[1]=0;
   tell_player(p,"Set player as having no email address.\n");
 }

 /* blank password of a player */

void blank_password(player *p,char *str)
{
    player *dummy;
    char *space;
    
    space=0;
    space=strchr(str,' ');
    if (space) {
	*space++=0;
	if (strlen(space)>9 || strlen(space)<3) {
	    tell_player(p,"Try a reasonable password.\n");
	    return;
	}
    }
/*    if (quick_banish_check(str,1)) return; */
    dummy=(player *)MALLOC(sizeof(player));
    memset(dummy,0,sizeof(player));
    strcpy(dummy->lower_name,str);
    lower_case(dummy->lower_name);
    dummy->fd=p->fd;

    if (!load_player(dummy)) {
	tell_player(p,"No such person in saved files.\n");
	FREE(dummy);
	return;
    }
    if (!check_privs(p->residency,dummy->residency)) {
	tell_player(p,"You can't do that !!\n");
	FREE(dummy);
	return;
    }
    if (space) 
	strcpy(dummy->password,do_crypt(space,dummy));
    else
	dummy->password[0]=-1;
    dummy->location=(room *)-1;
    tell_player(p,"Password blanked.\n");
    save_player(dummy);
    FREE(dummy);
    return;
}



 /* a test fn to test things */

void test_fn(player *p,char *str)
{
  tell_player(p,"Splat !\n");
}


 /* give someone lag ... B-) */

 void add_lag(player *p,char *str)
 {
   char *size;
   int new_size;
   char *oldstack;
   player *p2;

   oldstack=stack;
   size=next_space(str);
   *size++=0;
   new_size=atoi(size);
   if (!new_size && strcmp("perm",size)) {
     tell_player(p,"Format: lag <player> <for how long/perm>\n");
     return;
   }
   p2=find_player_global(str);
   if (!p2) return;
   if (!check_privs(p->residency,p2->residency)) {
     tell_player(p,"You can't do that !!\n");
     sprintf(oldstack,"%s tried to lag you.\n",p->name);
     stack=end_string(oldstack);
     tell_player(p2,oldstack);
     stack=oldstack;
     return;
   }
   if (new_size) {
       p2->lagged=new_size;
       p2->flags&=~PERM_LAG;
   }
   else p2->flags ^= PERM_LAG;
   tell_player(p,"Tis Done ..\n");
   if (p2->flags&PERM_LAG)
       tell_player(p,"Now perminently lagged.\n");
   else
       tell_player(p,"Not perminently lagged.\n");
   stack=oldstack;
 }



 /* remove shout from someone for a period */


void remove_shout(player *p,char *str)
{
    char *oldstack,*size=0;
    int new_size=0;
    player *p2;

    if (!*str) {
	tell_player(p,"Format: remove_shout <player> <for how long>\n");
	return;
    }
    oldstack=stack;
    size=strchr(str,' ');
    if (size) {
	*size++=0;
	new_size=atoi(size);
    }
    p2=find_player_global(str);
    if (!p2) return;
    if (!check_privs(p->residency,p2->residency)) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to remove shout from you.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    p2->saved_flags&=~SAVENOSHOUT;
    if (new_size)
	tell_player(p2,"You suddenly find yourself with a sore throat ...\n");
    else
	tell_player(p2,"Someone hands you a cough sweet ...\n");
    if (new_size>30) new_size=5;
    switch (new_size) {
    case -1:
	sprintf(stack,"-=> %s just remove shouted %s. (permanently!))\n",
		p->name,p2->name);
	p2->saved_flags|=SAVENOSHOUT;
	break;
    case 0:
	sprintf(stack,"-=> %s just allowed %s to shout again.\n",p->name,
		p2->name);
	break;
    case 1:
	sprintf(stack,"-=> %s just remove shouted %s for 1 minute.\n",
		p->name,p2->name);;
	break;
    default:
	sprintf(stack,"-=> %s just remove shouted %s for %d minutes.\n",
		p->name,p2->name,new_size);
	break;
    }
    new_size*=60;
    if (new_size>=0)
	p2->no_shout=new_size;
    stack=end_string(stack);
    su_wall(oldstack);
    stack=oldstack;
}


    
/* remove trans movement from someone for a period */

void remove_move(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;
    
    if (!*str) {
	tell_player(p,"Format: remove_move <player> <for how long>\n");
	return;
    }
    oldstack=stack;
    size=strchr(str,' ');
    if (size) {
	*size++=0;
	new_size=atoi(size);
    }
    if (!new_size) {
	    tell_player(p,"Format: remove_move <player> <for how long>\n");
	    return;
	}
    p2=find_player_global(str);
    if (!p2) return;
    if (!check_privs(p->residency,p2->residency)) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to remove move from you.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    if (new_size)
	tell_player(p2,"You step on some chewing-gum, and you suddenly "
		    "find it very hard to move ...\n");
    else
        tell_player(p2,"Someone hands you a new pair of shoes ...\n");
    if (new_size>30) new_size=5;
    new_size*=60;
    if (new_size>=0)
	p2->no_move=new_size;
    stack=oldstack;
}

/* change someones max mail limit */

void change_mail_limit(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;

    oldstack=stack;
    size=next_space(str);
    *size++=0;
    new_size=atoi(size);
    if (!new_size) {
	tell_player(p,"Format: change_mail_limit <player> <new size>\n");
	return;
	 }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency>p->residency) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to change your mail limit.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    p2->max_mail=new_size;
    sprintf(oldstack,"%s has changed you mail limit to %d.\n",p->name,
	    new_size);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    tell_player(p,"Tis Done ..\n");
    stack=oldstack;
}

/* change someones max list limit */

void change_list_limit(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;
    
    oldstack=stack;
    size=next_space(str);
    *size++=0;
    new_size=atoi(size);
    if (!new_size) {
	tell_player(p,"Format: change_list_limit <player> <new size>\n");
	return;
    }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency>p->residency) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to change your list limit.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    p2->max_list=new_size;
    sprintf(oldstack,"%s has changed you list limit to %d.\n",p->name,
	    new_size);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    tell_player(p,"Tis Done ..\n");
    stack=oldstack;
}

/* change someones max room limit */

void change_room_limit(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;
    
    oldstack=stack;
    size=next_space(str);
    *size++=0;
    new_size=atoi(size);
    if (!new_size) {
	tell_player(p,"Format: change_room_limit <player> <new size>\n");
	return;
    }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency>p->residency) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to change your room limit.\n",p->name);
	stack=end_string(oldstack);
	   tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    p2->max_rooms=new_size;
    sprintf(oldstack,"%s has changed you room limit to %d.\n",p->name,
	    new_size);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    tell_player(p,"Tis Done ..\n");
    stack=oldstack;
}


/* change someones max exit limit */

void change_exit_limit(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;
    
    oldstack=stack;
    size=next_space(str);
    *size++=0;
    new_size=atoi(size);
	 if (!new_size) {
	     tell_player(p,"Format: change_exit_limit <player> <new size>\n");
	     return;
	 }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency>p->residency) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to change your exit limit.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
	 }
    p2->max_exits=new_size;
    sprintf(oldstack,"%s has changed your exit limit to %d.\n",p->name,new_size);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    tell_player(p,"Tis Done ..\n");
    stack=oldstack;
}


/* change someones max autos limit */

void change_auto_limit(player *p,char *str)
{
    char *size;
    int new_size;
    char *oldstack;
    player *p2;
    
    oldstack=stack;
    size=next_space(str);
    *size++=0;
    new_size=atoi(size);
    if (!new_size) {
	   tell_player(p,"Format: change_auto_limit <player> <new size>\n");
	   return;
       }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency>p->residency) {
	tell_player(p,"You can't do that !!\n");
	sprintf(oldstack,"%s tried to change your automessage limit.\n",
		p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	return;
    }
    p2->max_autos=new_size;
    sprintf(oldstack,"%s has changed your automessage limit to %d.\n",p->name,
	    new_size);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    tell_player(p,"Tis Done ..\n");
    stack=oldstack;
}

/* manual command to sync files to disk */

void sync_files(player *p,char *str)
{
    if (!isalpha(*str)) {
	tell_player(p,"Argument must be a letter.\n");
	return;
    }
    sync_to_file(tolower(*str),1);
    tell_player(p,"Sync succesful.\n");
}

/* manual retrieve from disk */

void restore_files(player *p,char *str)
{
    if (!isalpha(*str)) {
	tell_player(p,"Argument must be a letter.\n");
	return;
    }
    remove_entire_list(tolower(*str));
    hard_load_one_file(tolower(*str));
    tell_player(p,"Restore succesful.\n");
}


/* shut down the program */

void pulldown(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    tell_player(p,"\nShutting down server.\n\n");
    sprintf(oldstack,"%s shut the program down.",p->name);
    stack=end_string(oldstack);
    log("super",oldstack);
    sys_flags |= SHUTDOWN;
    stack=oldstack;
}


/* wall to everyone, non blockable */

void wall(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: wall <arg>\n");
	return;
    }
    sprintf(oldstack,"%s screams -=> %s <=-\007\n",p->name,str);
    stack=end_string(oldstack);
    raw_wall(oldstack);
    stack=oldstack;
}


/* permission changes routines */

/* the resident command command */

void resident(player *p,char *str)
{
    player *p2;
    char *oldstack;
    
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: resident <whoever>\n");
	return;
    }
    sprintf(oldstack,"%s tries to resident %s",p->name,str);
    stack=end_string(oldstack);
    log("super",oldstack);
    
    p2=find_player_global(str);
    if (!p2) {
	stack=oldstack;
	return;
    }
    if (p2->residency!=NON_RESIDENT) {
	tell_player(p,"That person is resident already\n");
	stack=oldstack;
	return;
    }
    sprintf(oldstack,"\n\n%s has made you a resident.\n"
	    "For this to take effect, you MUST set an email address"
	    " and password NOW.\nIf you don't you will still not be able"
	    " to save, and next time you log in, you will be no longer"
	    " resident.\nTo set an email address, simply type 'email"
	    " <whatever>' as a command.\nTo set your password, simply type"
	    " 'password' as a command, and follow the prompts.\n\n",p->name);
    stack=end_string(oldstack);
    tell_player(p2,oldstack);
    p2->residency |= get_flag(permission_list,"residency");
    tell_player(p,"Residency granted ...\n");
    stack=oldstack;
    
    sprintf(oldstack,"-=> %s grants residency to %s\n",p->name,p2->name);
    stack=end_string(oldstack);
    su_wall(oldstack);
    stack=oldstack;
    p2->saved_residency=p2->residency;
}

/* the grant command */

void grant(player *p,char *str)
{
    char *permission;
    player *p2;
    saved_player *sp;
    int change;
    char *oldstack;
    
    oldstack=stack;
    
    permission=next_space(str);
    if (!*permission) {
	tell_player(p,"Format: grant <whoever> <whatever>\n");
	return;
    }
    *permission++=0;
    
    change=get_flag(permission_list,permission);
    if (!change) {
	tell_player(p,"Can't find that permission.\n");
	return;
    }
    if (!(p->residency&change)) {
	tell_player(p,"You can't give out permissions you haven't got "
		    "yourself.\n");
	return;
    }
    
    p2=find_player_global(str);
    if (!p2) {
/*	if (quick_banish_check(str,1)) return; */
	sp=find_saved_player(str);
	if (!sp) {
	    tell_player(p,"Couldn't find player.\n");
	    stack=oldstack;
	    return;
	}
	if (sp->residency>p->residency) {
	    tell_player(p,"You can't alter that save file\n");
	    sprintf(oldstack,"%s failed to grant %s to %s\n",p->name,
		    permission,str);
	    stack=end_string(oldstack);
	    log("super",oldstack);
	    stack=oldstack;
	    return;
	}
	tell_player(p,"Permission changed in player files.\n");
	stack=oldstack;
	sprintf(stack,"%-18s (%s)",sp->lower_name,permission);
	stack=end_string(stack);
	log("help",oldstack);
	sp->residency |= change;
	set_update(*str);
	stack=oldstack;
	return;
    }
    else {
	if (p2->residency>p->residency) {
	    tell_player(p,"No Way Pal !!\n");
	    sprintf(oldstack,"%s tried to grant your permissions.\n"
		    ,p->name);
	    stack=end_string(oldstack);
	    tell_player(p2,oldstack);
	    stack=oldstack;
	    return;
	}
	sprintf(oldstack,"\n%s has changed your permissions.\n",p->name);
	p2->saved_residency |= change;
	p2->residency=p2->saved_residency;	
	stack=strchr(stack,0);
	if (p2->residency&LOWER_SU)
	    strcpy(stack,"Read the appropriate files please ( shelp "
		   "basic or shelp advanced )\n\n");
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	stack=oldstack;
	sprintf(stack,"%-18s %s (%s)",p2->name,p2->email,permission);
	stack=end_string(stack);
	log("help",oldstack);
	save_player(p2);
	tell_player(p,"Permissions changed ...\n");
    }
    stack=oldstack;
}

/* the remove command */

void remove(player *p,char *str)
{
    char *permission;
    player *p2;
    saved_player *sp;
    int change;
    char *oldstack;
    
    oldstack=stack;
    
    permission=next_space(str);
    if (!*permission) {
	tell_player(p,"Format: remove <whoever> <whatever>\n");
	return;
    }
    *permission++=0;
    
    change=get_flag(permission_list,permission);
    if (!change) {
	tell_player(p,"Can't find that permission.\n");
	return;
    }
    if (!(p->residency&change)) {
	tell_player(p,"You can't remove permissions you haven't got "
		    "yourself.\n");
	return;
    }
    
    sprintf(oldstack,"%s tries to remove %s from %s",p->name,permission,str);
    stack=end_string(oldstack);
    log("super",oldstack);
    
    p2=find_player_global(str);
    if (!p2) {
	sp=find_saved_player(str);
	if (!sp) {
	    tell_player(p,"Couldn't find player.\n");
	    return;
	}
	if (!check_privs(p->residency,sp->residency)) {
	    tell_player(p,"You cant change that save file !!!\n");
	    sprintf(oldstack,"%s failed to remove %s from %s",p->name,
		    permission,str);
	    stack=end_string(oldstack);
	    log("super",oldstack);
	    stack=oldstack;
	    return;
	}
	sp->residency &= ~change;
	if (sp->residency==NON_RESIDENT) remove_player_file(sp->lower_name);
	set_update(*str);
	tell_player(p,"Permissions changed in save files.\n");
	stack=oldstack;
	return;
    }
    else {
	if (p2->residency>p->residency) {
	    tell_player(p,"No Way Pal !!\n");
	    sprintf(oldstack,"%s tried to remove your permissions.\n",p->name);
	    stack=end_string(oldstack);
	    tell_player(p2,oldstack);
	    stack=oldstack;
	    return;
	}
	p2->residency &= ~change;
	p2->saved_residency=p2->residency;
	sprintf(oldstack,"%s has changed your permissions.\n",p->name);
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	if (p2->residency!=NON_RESIDENT) save_player(p2);
	else remove_player_file(p2->lower_name);
	tell_player(p,"Permissions changed ...\n");
    }
    stack=oldstack;
}

/* remove player completely from the player files */

void nuke_player(player *p,char *str)
{
    char *oldstack;
    player *p2;
    saved_player *sp;
    oldstack=stack;
    sprintf(oldstack,"%s blancmanges %s.",p->name,str);
    stack=end_string(oldstack);
    log("super",oldstack);
    p2=find_player_absolute_quiet(str);
    if (!p2) 
	tell_player(p,"No such person on the program.\n");
    if (p2) {
	if (!check_privs(p->residency,p2->residency)) {
	    tell_player(p,"You can't nuke them !\n");
	    sprintf(oldstack,"%s tried to nuke you.\n",p->name);
	    stack=end_string(oldstack);
	    tell_player(p2,oldstack);
	    stack=oldstack;
	    return;
	}
	strcpy(oldstack,"\n\n -=> You have been nuked !!.\n\n\n");
	stack=end_string(oldstack);
	tell_player(p2,oldstack);
	p2->saved=0;
	p2->residency=0;
	quit(p2,0);
	sprintf(oldstack,"-=> %s nukes %s, oh what joy\n",p->name,p2->name);
	stack=end_string(oldstack);
	su_wall(oldstack);
    }
    strcpy(oldstack,str);
    lower_case(oldstack);
    stack=end_string(oldstack);
/*    if (remove_banish_name(oldstack,banished_p.where)) {
	tell_player(p,"Wheeeeeee.. Nuked banished name.\n");
	stack=oldstack;
	return;
    } */
    sp=find_saved_player(oldstack);
    if (!sp) {
	sprintf(oldstack,"Couldn't find saved player '%s'.\n",str);
	stack=end_string(oldstack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    if (sp->residency>=p->residency) {
	tell_player(p,"You can't nuke that save file !\n");
	stack=oldstack;
	return;
    }
    remove_player_file(oldstack);
    tell_player(p,"Files succesfully nuked.\n");
    stack=oldstack;
}


/* banish a player from the program */

void banish_player(player *p,char *str)
{
    char *oldstack;
    player *p2;
    saved_player *sp;
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: banish <player>\n");
	return;
    }

    if (strchr(str,' ')) {
	tell_player(p,"Try that one without a space\n");
	return;
    }
    
    if (*str=='*' && !(p->residency&(ADMIN|LOWER_ADMIN))) {
	tell_player(p,"You can't wildcard banish!\n");
	return;
    }

    if (banished_check(str)) {
	tell_player(p,"That name is already banished.\n");
	return;
    }

    sprintf(oldstack,"%s is trying to banish %s.",p->name,str);
    stack=end_string(oldstack);
    log("super",oldstack);
    p2=find_player_absolute_quiet(str);
    if (!p2)
	tell_player(p,"No such person on the program.\n");
    if (p2) {
	if (!check_privs(p->residency,p2->residency)) {
	    tell_player(p,"You can't banish them !\n");
	    sprintf(oldstack,"%s tried to banish you.\n",p->name);
	    stack=end_string(oldstack);
	    tell_player(p2,oldstack);
	    stack=oldstack;
	    return;
	}
	sprintf(oldstack,"\n\n -=> You have been banished !!!.\n\n\n");
	stack=end_string(oldstack);
	tell_player(p2,oldstack);   
	p2->saved=0;
	p2->residency=0;
	quit(p2,0);
	sprintf(oldstack,"-=> %s banishes %s\n",p->name,p2->name);
	stack=end_string(oldstack);
	su_wall(oldstack);
    }
    strcpy(oldstack,str);
    lower_case(oldstack);
    stack=end_string(oldstack);
    sp=find_saved_player(oldstack);
    if (sp && (sp->residency>=p->residency)) {
	tell_player(p,"You can't banish that save file !\n");
	stack=oldstack;
	return;
    }

    remove_player_file(oldstack);
/*    add_name_banish(oldstack); */

    create_banish_file(oldstack); 

    tell_player(p,"Name succesfully banished.\n");
    stack=oldstack;
}

/* create a new character */

void make_new_character(player *p,char *str)
{
    char *oldstack,*cpy,*email;
    player *np;
    int length=0;
    oldstack=stack;
    email=next_space(str);
    if (!*str || !*email) {
	tell_player(p,"Format: make <character name> <email addr>\n");
	return;
    }
    *email++=0;
    for(cpy=str;*cpy;cpy++) if (isalnum(*cpy)) {
	*stack++=*cpy;
	length++;
    }
    *stack++=0;
    if (length>(MAX_NAME-2)) {
	tell_player(p,"Name too long.\n");
	stack=oldstack;
	return;
    }
/*    if (quick_banish_check(str,1)) return; */
    if (find_saved_player(oldstack)) {
	tell_player(p,"That player already exists.\n");
	stack=oldstack;
	return;
    }
    np=create_player();
    np->fd=p->fd;
    np->location=(room *)-1;
    restore_player(np,oldstack);
    np->residency=get_flag(permission_list,"residency");
    np->saved_residency=np->residency;

    /* Crypt that password, why don't you */

    strcpy(np->password,do_crypt(oldstack,np));

/*    strncpy(np->password,oldstack,(MAX_PASSWORD-2)); */

    strncpy(np->email,email,(MAX_EMAIL-2));
    save_player(np);
    np->fd=0;
    np->location=0;
    destroy_player(np);
    cpy=stack;
    sprintf(cpy,"%s creates %s.",p->name,oldstack);
    stack=end_string(cpy);
    log("super",cpy);
    tell_player(p,"Player created.\n");
    stack=oldstack;
    return;
}




/* port from EW dump file */

void port(player *p,char *str)
{
    char *oldstack,*scan;
    player *np;
    file old;
    oldstack=stack;
    
    old=load_file("files/old.players");
    scan=old.where;
    
    while(old.length>0) {
	while(*scan!=' ') {
	    *stack++=*scan++;
	    old.length--;
	}
	scan++;
	*stack++=0;
	strcpy(stack,oldstack);
	lower_case(stack);
	if (!find_saved_player(stack)) {
	    np=create_player();
	    np->fd=p->fd;
	    restore_player(np,oldstack);
	    np->residency=get_flag(permission_list,"residency");
	    stack=oldstack;
	     while(*scan!=' ') {
		 *stack++=*scan++;
		 old.length--;
	     }
	    *stack++=0;
	    scan++;
	    strncpy(np->password,oldstack,MAX_PASSWORD-2);
	    stack=oldstack;
	    while(*scan!='\n') {
		*stack++=*scan++;
		old.length--;
	    }
	    *stack++=0;
	    scan++;
	    strncpy(np->email,oldstack,MAX_EMAIL-2);
	    sprintf(oldstack,"%s [%s] %s\n",np->name,np->password,np->email);
	    stack=end_string(oldstack);
	    tell_player(p,oldstack);
	    stack=oldstack;      
	    save_player(np);
	    np->fd=0;
	    destroy_player(np);
	}
	else {
	    while(*scan!='\n') {
		scan++;
		old.length--;
	    }
	    scan++;
	}
    }
    if (old.where) FREE(old.where);
    stack=oldstack;
}


void lsu(player *p,char *str)
{
    int count=0;
    char *oldstack;
    player *scan;
    oldstack=stack;
    
    strcpy(stack,"----------------------------- Supers on "
	   "------------------------------\n");
    stack=strchr(stack,0);
    for(scan=flatlist_start;scan;scan=scan->flat_next)
	if (p->residency&OFF_DUTY) {
	    if (scan->residency&OFF_DUTY && scan->location) {
		count++;
		*stack=' ';
		stack++;
		sprintf(stack,"%-20s",scan->name);
		stack=strchr(stack,0);
		if (scan->saved_residency&ADMIN)
		    strcpy(stack,"< Admin >       ");
		else 
		    if (scan->saved_residency&UPPER_SU)
			strcpy(stack,"< Advanced su > ");
		    else
			if (scan->saved_residency&LOWER_ADMIN)
			    strcpy(stack,"< Lower Admin > ");
			else
			    if (scan->saved_residency&LOWER_SU)
				strcpy(stack,"< Basic su >    ");
		stack=strchr(stack,0);
		if (scan->flags&BLOCK_SU) {
		    strcpy(stack,"      Off Duty atm.");
		    stack=strchr(stack,0);
		}
		*stack++='\n';
	    }
	}
	else {
	    if (scan->residency&OFF_DUTY && scan->location &&
		!(scan->flags&BLOCK_SU)) {
		count++;
		sprintf(stack,"%-20s ",scan->name);
		stack=strchr(stack,0);
		if (count%3==0) {
		    *stack='\n';
		    stack++;
		}
	    }
	}
    if (count==0) {
	tell_player(p,"There are no superusers connected right now.. tough\n");
	stack=oldstack;
	return;
    }
    if (!(p->residency&OFF_DUTY) && count%3!=0) {
	*stack='\n';
	stack++;
    }
    if (count>1)
	sprintf(stack,"---------------- There are %s Super Users connected "
		"------------------\n",number2string(count));
    else
	sprintf(stack,"----------------- There is one Super User connected "
		"------------------\n",count);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

void lnew(player *p,char *str)
{
    char *oldstack;
    int i=0;
    player *scan;
    oldstack=stack;   
    command_type = EVERYONE;    
    strcpy(stack,"----------------------------- Newbies on ----------------"
	   "-------------\n");
    stack=strchr(stack,0);
    for(scan=flatlist_start;scan;scan=scan->flat_next)
	if (scan->residency==NON_RESIDENT && scan->location) {
	    i=1;
	    sprintf(stack,"%-20s",scan->name);
	    stack=strchr(stack,0);
	    if (p->residency&(UPPER_SU|UPPER_SU|ADMIN)) {
		strcpy(stack,scan->inet_addr);
		stack=strchr(stack,0);
	    }
	    *stack++='\n';
	}
    strcpy(stack,"---------------------------------------------------------"
	   "-------------\n");
    stack=end_string(stack);
    if (i!=1)
	tell_player(p," No newbies on at the moment.\n");
    else
	tell_player(p,oldstack);
    stack=oldstack;
}


    /* rename a person (yeah, right, like this is going to work .... ) 
       
    */

void do_rename(player *p,char *str,int verbose)
{
    char *oldstack,*firspace,name[21],*letter,*oldlist;
    int *oldmail;
    int hash;
    player *oldp,*scan,*previous;
    saved_player *sp;
    room *oldroom;

    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: rename <person> <newp-name>\n");
	return;
    }
    if (!(firspace=strchr(str,' ')))  return;
    *firspace=0;
    firspace++;
    letter=firspace;
    if (!(oldp=find_player_global(str)))  return;
    if (oldp->residency&BASE) {
	sprintf(stack,"But %s is not a newbie.\n",oldp->name);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    scan=find_player_global_quiet(firspace);
    if (scan) {
	sprintf(stack,"There is already a person with the name '%s' "
		"logged on.\n",scan->name);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    strcpy(name,firspace);
    lower_case(name);
    sp=find_saved_player(name);
    if (sp) {
	sprintf(stack,"There is already a person with the name '%s' "
		"in the player files.\n",sp->lower_name);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }

    /* Test for a nice inputted name */
    
    if (strlen(letter)>MAX_NAME-2 || strlen(letter)<3) {
	tell_player(p,"Try picking a name of a decent length.\n");
	stack=oldstack;
	return;
    }
    while (*letter) {
	if (!isalpha(*letter)) {
	    tell_player(p,"Letters in names only, please ...\n");
	    stack=oldstack;
	    return;
	}
	letter++;
    }

    /* right, newname doesn't exist then, safe to make a change (I hope) */
    /* Remove oldp from hash list */

    scan=hashlist[oldp->hash_top];
    previous=0;
    while(scan && scan!=oldp) {
	previous=scan;
	scan=scan->hash_next;
    }
    if (!scan) log("error","Bad hash list (rename)");
    else 
	if (!previous)
	    hashlist[oldp->hash_top]=oldp->hash_next;
	else
	    previous->hash_next=oldp->hash_next;

    strcpy(name,oldp->lower_name);
    strncpy(oldp->name,firspace,MAX_NAME-2);
    lower_case(firspace);
    strncpy(oldp->lower_name,firspace,MAX_NAME-2);

    /* now place oldp back into named hashed lists */
  
    hash=((int)(oldp->lower_name[0])-(int)'a'+1);
    oldp->hash_next=hashlist[hash];
    hashlist[hash]=oldp;
    oldp->hash_top=hash;

    oldp->saved=0;
    save_player(oldp);
    stack=oldstack;
    if (verbose) {
	sprintf(stack,"%s dissolves in front of your eyes, and "
		"rematerialises as %s ...\n",name,oldp->name);
	stack=end_string(stack);
	
	/* tell room */
	
	scan=oldp->location->players_top;
	while (scan) {
	    if (scan!=oldp && scan!=p) 
		tell_player(scan,oldstack);
	    scan=scan->room_next;
	}
	stack=oldstack;
	sprintf(stack,"\n%s has just changed your name to be '%s' ...\n\n",
		p->name,oldp->name);
	stack=end_string(stack);
	tell_player(oldp,oldstack);
    }
    
    tell_player(p," Tis done ...\n");
    stack=oldstack;

    /* log it */

    sprintf(stack,"Rename by %s - %s to %s",p->name,name,oldp->name);
    stack=end_string(stack);
    log("super",oldstack);
    stack=oldstack;
}

void rename_player(player *p,char *str)
{
    do_rename(p,str,1);
}

void quiet_rename(player *p,char *str)
{
    do_rename(p,str,0);
}

void on_duty(player *p,char *str)
{
    char *oldstack;
    p->flags&= ~BLOCK_SU;
    tell_player(p,"You return to duty.\n");
    p->residency=p->saved_residency;
    oldstack=stack;
    sprintf(stack,"%s goes on duty.",p->name);
    stack=end_string(stack);
    log("duty",oldstack);
    stack=oldstack;
}


void block_su(player *p,char *str)
{
    char *oldstack;
    if (p->flags&BLOCK_SU) {
	tell_player(p,"You're already off duty.\n");
	return;
    }
    p->flags|=BLOCK_SU;
    tell_player(p,"You're now off duty ... I mean, why though,"
		" what the hell is the point of being a superuser if"
		" you're going to go off duty the whole time?\n");
    p->saved_residency=p->residency;
    p->residency&=~(LOWER_SU|UPPER_SU|LOWER_ADMIN|ADMIN);
    oldstack=stack;
    sprintf(stack,"%s goes off duty.",p->name);
    stack=end_string(stack);
    log("duty",oldstack);
    stack=oldstack;
}

/* help for superusers */

void super_help(player *p,char *str)
{
    char *oldstack;
    file help;

    oldstack=stack;
    if (!*str || (!strcmp(str,"admin") && !(p->residency&ADMIN))) {
	tell_player(p,"SuperUser help files that you can read are: basic, "
		    "advanced.\n");
	return;
    }
    if (*str=='.') {
	tell_player(p,"Uh-uh, cant do that ...\n");
	return;
    }
    sprintf(stack,"doc/%s.doc",str);
    stack=end_string(stack);
    help=load_file_verbose(oldstack,0);
    if (help.where) {
	if (*(help.where)) 
	    pager(p,help.where,1);
	else
	    tell_player(p,"Couldn't find that help file ...\n");
	free(help.where);
    }
    stack=oldstack;
}

    /*  assist command */

void assist_player(player *p,char *str)
{
    char *oldstack,*comment;
    player *p2;

    if (!*str) {
	tell_player(p,"Format: assist <person>\n");
	return;
    }
    p2=find_player_global(str);
    if (!p2) return;
    if (p2->residency!=NON_RESIDENT) {
	tell_player(p,"That person isn't a newbie though ...\n");
	return;
    }
    if (p2->flags&ASSISTED) {
	tell_player(p,"That person has already been assisted!\n");
	return;
    }
    p2->flags|=ASSISTED;
    oldstack=stack;
    sprintf(stack,"\n%s is a superuser, and would be more than happy to "
	    "assist you in any problems you may have.  To talk to %s, "
	    "type: tell %s <whatever>\n\n",p->name,get_gender_string(p),
	    p->lower_name);
    stack=end_string(stack);
    tell_player(p2,oldstack);
    stack=oldstack;
    sprintf(stack,"-=> %s assists %s.\n",p->name,p2->name);
    stack=end_string(stack);
    p->flags|=NO_SU_WALL;
    su_wall(oldstack);
    stack=oldstack;
    sprintf(stack,"You assist %s.\n",p2->name);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
    sprintf(stack,"%s assists %s",p->name,p2->name);
    stack=end_string(stack);
    log("assist",oldstack);
    stack=oldstack;
}


void confirm_password(player *p,char *str)
{
    char *oldstack;
    player *p2;

    if (!*str) {
	tell_player(p,"Format: confirm <name>\n");
	return;
    }
    p2=find_player_global(str);
    if (!p2) return;
    
    if (p2->residency==NON_RESIDENT) {
	tell_player(p,"That person is not a resident.\n");
	return;
    }
    oldstack=stack;

    /* check email */

    if (p2->email[0]==-1)
	strcpy(stack,"Email validated set.\n");
    else 
	if (!strstr(p2->email,"@") || !strstr(p2->email,"."))
	    strcpy(stack,"Probably not a correct email.\n");
	else
	    strcpy(stack,"Email set.\n");
    stack=strchr(stack,0);
    
    /* password */
    
    if (p2->password[0] && p2->password[0]!=-1) 
	strcpy(stack,"Password set.\n");
    else
	strcpy(stack,"Password NOT-set.\n");
    stack=strchr(stack,0);
    
    if (p2->residency&NO_SYNC) 
	sprintf(stack,"Character '%s' won't be saved.\n",p2->name);
    else
	sprintf(stack,"Character '%s' will be saved.\n",p2->name);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}
	    

 /* reset email of a player */

void blank_email(player *p,char *str)
{
	player *dummy;
	char *space;

	space=0;
	space=strchr(str,' ');
	if (space) {
		*space++=0;
		if (strlen(space)<3) {
			tell_player(p,"Try a reasonable email address.\n");
			return;
		}
	}
	dummy=(player *)MALLOC(sizeof(player));
	memset(dummy,0,sizeof(player));
	strcpy(dummy->lower_name,str);
	lower_case(dummy->lower_name);
	dummy->fd=p->fd;

	if (!load_player(dummy)) {
		tell_player(p,"No such person in saved files.\n");
		FREE(dummy);
		return;
	}
	if (dummy->residency>=p->residency) {
		tell_player(p,"You can't do that !!\n");
		FREE(dummy);
		return;
	}
	if (space)
		strcpy(dummy->email,space);
	else
		dummy->email[0]=-1;
		dummy->location=(room *)-1;
		tell_player(p,"Email reset.\n");
		save_player(dummy);
		FREE(dummy);
		return;
}

/* 1 yes, 0 no. p=doer  p2=bloke getting done */

int check_privs(int p,int p2)
{
    if (p&(ADMIN|LOWER_ADMIN)) return 1;
    if (p2&(ADMIN|LOWER_ADMIN)) return 0;
    if (p&UPPER_SU) return ((p2&UPPER_SU)?0:1);
    if (p&LOWER_SU) return ((p2&LOWER_SU)?0:1);
    return 0;
}
