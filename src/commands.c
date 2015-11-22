/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* 
  commands.c
*/

#include <ctype.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <memory.h>

#include "config.h"
#include "player.h"
#include "fix.h"

/* externs */

extern int check_password(char *,char *,player *);
extern char *crypt(char *,char *);
extern void check_list_resident(player *);
extern void check_list_newbie(char *);
extern void destroy_player(),save_player(),password_mode_on(),
  password_mode_off(),sub_command(),extract_pipe_global(),tell_room(),
  extract_pipe_local(),pstack_mid();
extern player *find_player_global(),*find_player_absolute_quiet();
extern char *end_string(),*tag_string(),*next_space(),*do_pipe(),*full_name(),
  *caps(),*sys_time();
extern int global_tag();
extern file idle_string_list[];
extern saved_player *find_saved_player();
extern char *idlingstring();
extern struct command check_list[];

extern char *gstring_possessive(player *);
extern char *gstring(player *);
extern void su_wall(char *);
extern char *number2string(int);
extern char *get_gender_string(player *);
extern char *word_time(int);
#ifdef MIPS
typedef int time_t;
#endif
extern char *convert_time(time_t);
extern char *time_diff(int),*time_diff_sec(time_t,int);
extern int quick_banish_check(char *,int);

 /* birthday and age stuff */

 void set_age(player *p,char *str)
 {
   char *oldstack;
   int new_age;
   oldstack=stack;

   if (!*str) {
     tell_player(p,"Format: age <number>\n");
     return;
   }
   new_age=atoi(str);
   if (new_age<0) {
     tell_player(p,"You can't be of a negative age !\n");
     return;
   }

   p->age=new_age;
   if (p->age) {
     sprintf(oldstack,"Your age is now set to %d years old.\n",p->age);
     stack=end_string(oldstack);
     tell_player(p,oldstack);
   }
   else
     tell_player(p,"You have turned off your age so no-one can see it.\n");
   stack=oldstack;
 }


 /* set birthday */

 void set_birthday(player *p,char *str)
 {
   char *oldstack;
   struct tm bday;
   int t;
   oldstack=stack;

   if (!*str) {
     tell_player(p,"Format: birthday <day>/<month>(/<year>)\n");
     return;
   }
   memset((char *)&bday,0,sizeof(struct tm));
   bday.tm_year=93;
   bday.tm_mday=atoi(str);

   if (!bday.tm_mday) {
     tell_player(p,"Birthday cleared.\n");
     p->birthday=0;
     return;
   }
   if (bday.tm_mday<0 || bday.tm_mday>31) {
     tell_player(p,"Not a valid day of the month.\n");
     return;
   }
   while(isdigit(*str)) str++;
   str++;
   bday.tm_mon=atoi(str);
   if (bday.tm_mon<=0 || bday.tm_mon>12) {
     tell_player(p,"Not a valid month.\n");
     return;
   }
   bday.tm_mon--;

   while(isdigit(*str)) str++;
   str++;
   bday.tm_year=atoi(str);
   if (bday.tm_year==0) {
     bday.tm_year=93;
     p->birthday = TIMELOCAL(&bday);
   }
   else {
     p->birthday = TIMELOCAL(&bday);
     t=time(0)-(p->birthday);
     if (t>0) p->age=t/31536000;
   }

   sprintf(oldstack,"Your birthday is set to the %s.\n",
	   birthday_string(p->birthday));
   stack=end_string(oldstack);
   tell_player(p,oldstack);
   stack=oldstack;
 }


 /* show what somone can do */

void privs(player *p,char *str)
{
    char *oldstack,name[20],*first;
    int priv,who=0;
    saved_player *sp;
    player *p2;

    oldstack=stack;

/*    if (*str && quick_banish_check(str,0)) {
	sprintf(stack,"%s is a banished name ...\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    } */
    strcpy(name,"You");

    if (*str && p->residency&(LOWER_SU|UPPER_SU|LOWER_ADMIN|ADMIN)) {
	p2=find_player_global(str);
	if (!p2) {
	    sp=find_saved_player(str);
	    if (!sp) {
		tell_player(p,"Couldn't find player in save files.\n");
		stack=oldstack;
		return;
	    }
	    else {
		tell_player(p,"Found in save file ...\n");
		priv=sp->residency;
		strcpy(name,sp->lower_name);
		who=1;
	    }
	}
	else {
	    strcpy(name,p2->name);
	    priv=p2->residency;
	    who=1;
	    sprintf(stack,"Permissions for %s.\n",name);
	    stack=strchr(stack,0);
	}
    }
    else {
	priv=p->residency;
    }
    name[0]=toupper(name[0]);
  
    if (priv==NON_RESIDENT) { 
	sprintf(stack,"%s will not be saved... not a resident, you see..\n",
		name);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    } 
    if (priv==BANISHED) {
	sprintf(stack,"%s has been banished ...\n",name);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    if (who==0) {
	if (priv&LIST) strcpy(stack,"You have a list.\n");
	else strcpy(stack,"You do not have a list.\n");
	stack=strchr(stack,0);
      
	if (priv&ECHO_PRIV) strcpy(stack,"You can echo.\n");
	else strcpy(stack,"You cannot echo.\n");
	stack=strchr(stack,0);
      
	if (priv&BUILD) strcpy(stack,"You can use room commands.\n");
	else strcpy(stack,"You can't use room commands.\n");
	stack=strchr(stack,0);
      
	if (priv&MAIL) strcpy(stack,"You can send mail.\n");
	else strcpy(stack,"You cannot send any mail.\n");
	stack=strchr(stack,0);

	if (priv&SESSION) strcpy(stack,"You can set sessions.\n");
	else strcpy(stack,"You cannot set sessions.\n");
	stack=strchr(stack,0);

	if (priv&LOWER_SU && !(priv&(UPPER_SU|LOWER_ADMIN|ADMIN))) {
	    strcpy(stack,"You are a basic superuser.  (ie. You can do sod "
		   "all)\n");
	    stack=strchr(stack,0);
	}
	if (priv&UPPER_SU) {
	    strcpy(stack,"You are an advanced superuser\n");
	    stack=strchr(stack,0);
	}
	if (priv&LOWER_ADMIN) {
	    strcpy(stack,"You are a lowly little admin thingie.\n");
	    stack=strchr(stack,0);
	}
	if (priv&ADMIN) {
	    strcpy(stack,"You are an administrator.\n");
	    stack=strchr(stack,0);
	}
    }
    if (who==1) {
	if (priv&LIST) sprintf(stack,"%s has a list.\n",name);
	else sprintf(stack,"%s does not has a list.\n",name);
	stack=strchr(stack,0);
      
	if (priv&ECHO_PRIV) sprintf(stack,"%s can echo.\n",name);
	else sprintf(stack,"%s cannot echo.\n",name);
	stack=strchr(stack,0);
      
	if (priv&BUILD) sprintf(stack,"%s can use room commands.\n",name);
	else sprintf(stack,"%s can't use room commands.\n",name);
	stack=strchr(stack,0);
      
	if (priv&MAIL) sprintf(stack,"%s can send mail.\n",name);
	else sprintf(stack,"%s cannot send any mail.\n",name);
	stack=strchr(stack,0);

	if (priv&SESSION) sprintf(stack,"%s can set sessions.\n",name);
	else sprintf(stack,"%s cannot set sessions.\n",name);
	stack=strchr(stack,0);
      
	/*
	  if (priv&SEE_ECHO) {
	  sprintf(stack,"%s can see who echos what.\n",name);
	  stack=strchr(stack,0);
	  }
	  if (priv&SOFT_EJECT) {
	  sprintf(stack,"%s has the ability to eject.\n",name);
	  stack=strchr(stack,0);
	  }
	  if (priv&SQUISH) {
	  sprintf(stack,"%s has the power to squish.\n",name);
	  stack=strchr(stack,0);
	  }
	  if (priv&R_ONLY) {
	  sprintf(stack,"%s has power to grant residency.\n",name);
	  stack=strchr(stack,0);
	  }
	  */
	if (priv&LOWER_SU) {
	    sprintf(stack,"%s is a basic superuser."
		    "\n",name,name);
	    stack=strchr(stack,0);
	}
	if (priv&UPPER_SU) {
	    sprintf(stack,"%s is an advanced superuser\n",name);
	    stack=strchr(stack,0);
	}
	if (priv&LOWER_ADMIN) {
	    sprintf(stack,"%s is a lower admin.\n",name);
	    stack=strchr(stack,0);
	}
	if (priv&ADMIN) {
	    sprintf(stack,"%s is an administrator.\n",name);
	    stack=strchr(stack,0);
	}
    }
    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}



/* recap someones name */

void recap(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;
  if (!*str) {
    tell_player(p,"Format: recap <name>\n");
    return;
  }
  strcpy(stack,str);
  if (strcasecmp(stack,p->lower_name)) {
    tell_player(p,"But that isn't your name !!\n");
    return;
  }
  strcpy(p->name,str);
  sprintf(stack,"Name changed to '%s'\n",p->name);
  stack=end_string(stack);
  tell_player(p,oldstack);
  stack=oldstack;
}

void friend_finger(player *p)
{
    char *oldstack,*temp;
    list_ent *l;
    saved_player *sp;
    player *dummy,*p2;
    int jettime,friend=0;
    
    dummy=(player *)malloc(sizeof(player));
    
    oldstack=stack;
    if (!p->saved) {
	tell_player(p,"You have no save information, and therefore no "
		    "friends ...\n");
	return;
    }
    sp=p->saved;
    l=sp->list_top;
    if (!l) {
	tell_player(p,"You have no list ...\n");
	return;
    }
    strcpy(stack,"\nYour friends were last seen...\n");
    stack=strchr(stack,0);
    do {
	if (l->flags&FRIEND && strcasecmp(l->name,"everyone")) {
/*	    if (quick_banish_check(l->name,0)) {
		sprintf(stack,"%s is a banished name.\n",l->name);
		stack=strchr(stack,0);
	    } 
	    else*/ { 
		p2=find_player_absolute_quiet(l->name);
		friend=1;
		if (p2) {
		    sprintf(stack,"%s is logged on.\n",p2->name);
		    stack=strchr(stack,0);
		}
		else {
		    temp=stack;
		    strcpy(temp,l->name);
		    lower_case(temp);
		    strcpy(dummy->lower_name,temp);
		    dummy->fd=p->fd;
		    if (load_player(dummy))
			switch(dummy->residency) {
			case BANISHED:
			    sprintf(stack,"%s is banished\n",dummy->lower_name);
			    stack=strchr(stack,0);
			    break;
			case STANDARD_ROOMS:
			    sprintf(stack,"%s is a system room ...\n",dummy->name);
			    stack=strchr(stack,0);
			    break;
			default:
			    if (dummy->saved)
				jettime=dummy->saved->last_on+(p->jetlag*3600);
			    else
				jettime=dummy->saved->last_on;
			    sprintf(stack,"%s was last seen at %s.\n",dummy->name,
				    convert_time(jettime));
			    stack=strchr(stack,0);
			    break;
			}
		    else {
			sprintf(stack,"%s doesn't exist.\n",l->name);
			stack=strchr(stack,0);
		    }
		}
	    }
	}
	l=l->next;
    } while(l);
    if (!friend) {
	tell_player(p,"But you have no friends !!\n");
	stack=oldstack;
	return;
    }
    stack=strchr(stack,0);
    *stack++='\n';
    *stack++=0;
    pager(p,oldstack,0);
    stack=oldstack;
    return;
}

/* finger command */

void finger(player *p,char *str)
{
  player *dummy,*p2;
  char *oldstack;
  int jettime;
  oldstack=stack;


  if (!*str) {
    tell_player(p,"Format: finger <player>\n");
    return;
  }
  
  if (!strcmp(str,"friends")) {
      friend_finger(p);
      return;
  }
  
  dummy=(player *)MALLOC(sizeof(player));
  
/*  if (quick_banish_check(str,1)) return; */
  p2=find_player_absolute_quiet(str);
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
    tell_player(p,"That player has been banished from this program.\n");
    FREE(dummy);
    return;
  case STANDARD_ROOMS:
    tell_player(p,"That is where some of the standard rooms are stored.\n");
    FREE(dummy);
    return;
  }
  
  sprintf(stack,"---------------------------------------------------"
	  "-------------------\n"
	  "%s %s\n"
	  "---------------------------------------------------------"
	  "-------------\n",
	  dummy->name,dummy->title);
  stack=strchr(stack,0);
  
  if (dummy->saved)
      jettime=dummy->saved->last_on+(p->jetlag*3600);
  else
      jettime=0;
  if (p2) 
      sprintf(stack,"%s has been logged in for %s since\n%s.\n",
	      full_name(dummy),word_time(time(0)-(dummy->on_since)),
	      convert_time(dummy->on_since));
  else if (dummy->saved) 
      if (p->jetlag)
	  sprintf(stack,"%s was last seen at %s. (Your time)\n",
		  dummy->name,convert_time(jettime));
      else
	  sprintf(stack,"%s was last seen at %s.\n",full_name(dummy),
		  convert_time(dummy->saved->last_on));
  stack=strchr(stack,0);
  
  if (dummy->age && dummy->birthday) 
    sprintf(stack,"%s is %d years old and %s birthday is on the %s.\n",
	    dummy->name,dummy->age,gstring_possessive(dummy),
	    birthday_string(dummy->birthday));
  if (dummy->age && !(dummy->birthday)) 
    sprintf(stack,"%s is %d years old.\n",dummy->name,dummy->age);
  if (!(dummy->age) && dummy->birthday) 
    sprintf(stack,"%s birthday is on the %s.\n",
	    caps(gstring_possessive(dummy)),birthday_string(dummy->birthday));
  stack=strchr(stack,0);

  if (dummy->saved_flags&NEW_MAIL) {
    sprintf(stack,"%s has new mail.\n",caps(gstring(dummy)));
    stack=strchr(stack,0);	
  }
  if (!(dummy->saved_flags&PRIVATE_EMAIL) && dummy->email[0]!=-1) {
    sprintf(stack,"%s email address is ... %s\n",
	    caps(gstring_possessive(dummy)),dummy->email);
    stack=strchr(stack,0);
  }
  if (dummy->plan[0]) {
    pstack_mid("plan");
    sprintf(stack,"%s\n",dummy->plan);
    stack=strchr(stack,0);
  }
  strcpy(stack,"----------------------------------------------------------------------\n");
  stack=end_string(stack);
  tell_player(p,oldstack);
  FREE(dummy);
  stack=oldstack;
}

/* emergency command */

void emergency(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;
  
  if (p->script) {
    if (!strcmp(str,"off") || !strcmp(str,"stop")) {
      sprintf(stack,"Time is now %s.\n"
	      "Scripting stopped at your request.\n",convert_time(time(0)));
      stack=end_string(stack);
      tell_player(p,oldstack);
      p->script=0;
      stack=oldstack;
      return;
    }
    tell_player(p,"You are already scripting ... use 'emergency stop' to "
		"stop.\n");
    return;
  }
  
  if (!*str) {
    tell_player(p,"You must give a reason for starting emergency scripting"
		" as an argument.\n"
		"(And the reason better be good ...)\n");
    return;
  }
  
  command_type = PERSONAL|EMERGENCY;
#ifdef PC	
  sprintf(stack,"logs\\%s_emergency.log",p->lower_name);
#else
  sprintf(stack,"logs/%s_emergency.log",p->lower_name);
#endif
  unlink(stack);
  p->script=60;
  sprintf(stack,"Emergency scripting started for 60 seconds.\n"
	  "Remember, any previous scripts will be deleted\n"
	  "Reason given : %s\n"
	  "Time is now %s.\n",str,convert_time(time(0)));
  stack=end_string(stack);
  tell_player(p,oldstack);
  sprintf(oldstack,"%s has started emergency scripting with reason %s.\n",
	  p->name,str);
  stack=end_string(oldstack);
  su_wall(oldstack);
  stack=oldstack;
}


/* see the time */

void view_time(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    if (p->jetlag) 
	sprintf(stack,"Your local time is %s.\n"
		"EW-too time is %s.\n"
		"The program has been up for %s.\nThat is from %s.\n"
		"Total number of logins in that time is %s.\n",
		time_diff(p->jetlag),sys_time(),word_time(time(0)-up_date),
		convert_time(up_date),number2string(logins));
    else 
	sprintf(stack,"EW-too time is %s.\n"
		"The program has been up for %s.\nThat is from %s.\n"
		"Total number of logins in that time is %s.\n",
		sys_time(),word_time(time(0)-up_date),
		convert_time(up_date),number2string(logins)); 
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;	
}

 /* go quiet */

void go_quiet(player *p,char *str)
{
    earmuffs(p,str);
    blocktells(p,str);
}


 /* the examine command */

void examine(player *p,char *str)
{
    player *p2;
    char *oldstack;
    oldstack=stack;
    if (!strcmp("me",str)) p2=p;
    else p2=find_player_global(str);
    if (!p2) return;
    sprintf(stack,"---------------------------------------------------"
	    "-------------------\n"
	    "%s %s\n"
	    "---------------------------------------------------------"
	    "-------------\n",p2->name,p2->title);
    stack=strchr(stack,0);
    if (p2->description[0]) {
	sprintf(stack,"%s\n---------------------------------------------"
		"-------------------------\n",p2->description);
	stack=strchr(stack,0);
    }

    if (p->jetlag)
    sprintf(stack,"%s has been logged in for %s\nThat is from %s. (Your time)"
	    "\n",full_name(p2),word_time(time(0)-(p2->on_since)),
	    convert_time((p2->on_since+(p->jetlag*3600))));
    else
    sprintf(stack,"%s has been logged in for %s\nThat is from %s.\n",
	    full_name(p2),word_time(time(0)-(p2->on_since)),
	    convert_time(p2->on_since));

    stack=strchr(stack,0);
    sprintf(stack,"%s total login time is %s.\n",caps(gstring_possessive(p2)),
	    word_time(p2->total_login));
    stack=strchr(stack,0);
    if (p2->saved_flags&BLOCK_TELLS && p2->saved_flags&BLOCK_SHOUT)
	sprintf(stack,"%s is ignoring shouts and tells.\n",caps(gstring(p2)));
    else {
	if (p2->saved_flags&BLOCK_TELLS)
	    sprintf(stack,"%s is ignoring tells.\n",caps(gstring(p2)));
	if (p2->saved_flags&BLOCK_SHOUT)
	    sprintf(stack,"%s is ignoring shouts.\n",caps(gstring(p2)));
    }
    stack=strchr(stack,0);

    if (p2->age && p2->birthday) 
	sprintf(stack,"%s is %d years old and %s birthday is on the %s.\n",
		p2->name,p2->age,gstring_possessive(p2),
		birthday_string(p2->birthday));
    if (p2->age && !(p2->birthday)) 
	sprintf(stack,"%s is %d years old.\n",p2->name,p2->age);
    if (!(p2->age) && p2->birthday) 
	sprintf(stack,"%s birthday is on the %s.\n",
		caps(gstring_possessive(p2)),birthday_string(p2->birthday));
    stack=strchr(stack,0);

    if (p2->residency!=NON_RESIDENT && (!(p2->saved_flags&PRIVATE_EMAIL)
					|| p->residency&(LOWER_ADMIN|ADMIN)
					|| p==p2)) {
	if (p2->email[0]==-1) strcpy(stack,"Declared no email address.\n"); 
	else sprintf(stack,"Email: %s\n",p2->email);
	stack=strchr(stack,0);
    }
    strcpy(stack,"----------------------------------------------------"
	   "------------------\n");
    stack=strchr(stack,0);
    if (p2==p || p->residency&(LOWER_ADMIN|ADMIN)) {
	sprintf(stack,"Your entermsg is set to ...\n%s %s\n",
		p2->name,p2->enter_msg);
	stack=strchr(stack,0);
	strcpy(stack,"------------------------------------------------"
	       "----------------------\n");
	stack=strchr(stack,0);
    }
    if ((p2==p || p->residency&(LOWER_ADMIN|ADMIN)) && p2->residency&BASE) {
	if (strlen(p2->ignore_msg)>0)
	    sprintf(stack,"Your ignoremsg is set to ...\n%s\n",p2->ignore_msg);
	else
	    strcpy(stack,"You have set no ignoremsg.\n");
	stack=strchr(stack,0);
	strcpy(stack,"------------------------------------------------"
	       "----------------------\n");
	stack=strchr(stack,0);
    }
    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}


 /* the idle command */

void check_idle(player *p,char *str)
{
    player *scan;
    char *oldstack,middle[80],namestring[40];
    file *is_scan;
    int page,pages,count,not_idle=0;
    oldstack=stack;
    if (isalpha(*str)) {
	scan=find_player_global(str);
	stack=oldstack;
	if (!scan) return;

	if (p->saved_flags&NOPREFIX)
	    strcpy(namestring,scan->name);
	else
	    if (*scan->pretitle)
		sprintf(namestring,"%s %s",scan->pretitle,scan->name);
	    else
		strcpy(namestring,scan->name);
	if (!scan->idle) 
	    sprintf(stack,"%s has just hit return.\n",namestring);
	else {
	    if (scan->idle_msg[0])
		sprintf(stack,"%s %s\n%s is %s idle\n",namestring,
			scan->idle_msg,caps(gstring(scan)),
			word_time(scan->idle));
	    else
		sprintf(stack,"%s is %s idle.\n",namestring,
			word_time(scan->idle));
	}
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

    for(scan=flatlist_start;scan;scan=scan->flat_next) 
	if (scan->name[0] && scan->location && (scan->idle)<300)
	    not_idle++;

    if (current_players==1) 
	strcpy(middle,"There is only you on the program at the moment");
    else
	if (not_idle==1) 
	    sprintf(middle,"There are %d people here, only one of whom "
		    "appears to be awake",current_players);
	else
	    sprintf(middle,"There are %d people here, %d of which appear "
		    "to be awake",current_players,not_idle);
    pstack_mid(middle);

    count=page*(TERM_LINES-2);
    for(scan=flatlist_start;count;scan=scan->flat_next)
	if (!scan) {
	    tell_player(p,"Bad idle listing, abort.\n");
	    log("error","Bad idle list");
	    stack=oldstack;
	    return;
	}
	else if (scan->name[0]) count--;

    for(count=0;(count<(TERM_LINES-1) && scan);scan=scan->flat_next)
	if (scan->name[0] && scan->location) {
	    if (p->saved_flags&NOPREFIX)
		strcpy(namestring,scan->name);
	    else
		sprintf(namestring,"%s %s",scan->pretitle,scan->name);
	    if (scan->idle_msg[0])
		sprintf(stack,"%s %s\n",namestring,scan->idle_msg);
	    else {
		for(is_scan=idle_string_list;is_scan->where;is_scan++) 
		    if (is_scan->length>=scan->idle) break;
		if (!is_scan->where) is_scan--;
		sprintf(stack,"%s %s",namestring,is_scan->where);
	    }
	    stack=strchr(stack,0);
	    count++;
	}
    sprintf(middle,"Page %d of %d",page+1,pages+1);
    pstack_mid(middle);

    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}




 /* converse mode on and off */

 void converse_mode_on(player *p,char *str)
 {
   if (p->saved_flags&CONVERSE) {
     tell_player(p,"But you are already in converse mode !\n");
     return;
   }
   p->saved_flags |= CONVERSE;
   tell_player(p,"Entering 'converse' mode. Everything you type will get said.\n"
	       "Start the line with '/' to use normal commands, and /end to\n"
	       "leave this mode.\n");
   return;
 }

 void converse_mode_off(player *p,char *str)
 {
   if (!(p->saved_flags&CONVERSE)) {
     tell_player(p,"But you are not in converse mode !\n");
     return;
   }
   p->saved_flags &= ~CONVERSE;
   tell_player(p,"Ending converse mode.\n");
   return;
 }




 /* set things */


 void set_idle_msg(player *p,char *str)
 {
   char *oldstack;
   oldstack=stack;
   if (*str) {
       strncpy(p->idle_msg,str,MAX_TITLE-2);
       sprintf(stack,"Idle message set to ....\n%s %s\nuntil you type a"
	       " new command.\n",full_name(p),p->idle_msg);
   } else
       strcpy(stack,"Format: idlemsg <message>\n");
   stack=end_string(stack);
   tell_player(p,oldstack);
   stack=oldstack;
 }


void set_enter_msg(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    if (!*str) {
	tell_player(p,"Format: entermsg <message>\n");
	return;
    }
    strncpy(p->enter_msg,str,MAX_ENTER_MSG-2);
    sprintf(stack,"This is what people will see when you enter the "
	    "room.\n%s %s\n",p->name,p->enter_msg);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

  void set_title(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    strncpy(p->title,str,MAX_TITLE-2);
    sprintf(stack,"You change your title so that now you are known as "
	    "...\n%s %s\n",p->name,p->title);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }

  void set_pretitle(player *p,char *str)
  {
    char *oldstack,*scan;
    oldstack=stack;

    for(scan=str;*scan;scan++) 
      switch(*scan) {
      case ' ':
	tell_player(p,"You may not have spaces in your pretitle, please "
		     "change it to something else.\n");
	return;
	break;
      case ',':
	tell_player(p,"You may not have commas in you pretitle, "
		    "please change it to something else.\n");
	return;
	break;
      }

    strncpy(p->pretitle,str,MAX_PRETITLE-2);
    sprintf(stack,"You change your pretitle so you become: %s %s\n",
	    p->pretitle,p->name);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }

  void set_description(player *p,char *str)
  {
    char *oldstack;
    oldstack=stack;
    strncpy(p->description,str,MAX_DESC-2);
    sprintf(stack,"You change your description to read...\n%s\n",
	    p->description);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }

  void set_plan(player *p,char *str)
  {
    char *oldstack;
    oldstack=stack;
    strncpy(p->plan,str,MAX_PLAN-2);
    sprintf(stack,"You set your plan to read ...\n%s\n",p->plan);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }

void set_prompt(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    strncpy(p->prompt,str,MAX_PROMPT-2);
    sprintf(stack,"You change your prompt to %s\n",p->prompt);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

void set_converse_prompt(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    strncpy(p->converse_prompt,str,MAX_PROMPT-2);
    sprintf(stack,"You change your converse prompt to %s\n",p->converse_prompt);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

  void set_term_width(player *p,char *str)
  {
    char *oldstack;
    int n;
    oldstack=stack;
    if (!strcmp("off",str)) {
      tell_player(p,"Linewrap turned off.\n");
      p->term_width=0;
      return;
    }
    n=atoi(str);
    if (!n) {
      tell_player(p,"Format: linewrap off/<terminal_width>\n");
      return;
    }
    if (n<=((p->word_wrap)<<1)) {
      tell_player(p,"Can't set terminal width that small compared to word wrap.\n");
      return;
    }
    if (n<10) {
      tell_player(p,"Nah, you haven't got a terminal so small !!\n");
      return;
    }
    p->term_width=n;
    sprintf(stack,"Linewrap set on, with terminal width %d.\n",p->term_width);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }


  void set_word_wrap(player *p,char *str)
  {
    char *oldstack;
    int n;
    oldstack=stack;
    if (!strcmp("off",str)) {
      tell_player(p,"wordwrap turned off.\n");
      p->word_wrap=0;
      return;
    }
    n=atoi(str);
    if (!n) {
      tell_player(p,"Format: wordwrap off/<max_word_size>\n");
      return;
    }
    if (n>=((p->term_width)>>1)) {
      tell_player(p,"Can't set max word length that big compared to term width.\n");
      return;
    }
    p->word_wrap=n;
    sprintf(stack,"Wordwrap set on, with max word size set to %d.\n",p->word_wrap);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
  }

  /* quit from the program */

void quit(player *p,char *str)
{
    if (!str) p->flags |= PANIC;
    p->flags |= CHUCKOUT;

/* Clean up lists */

    if (p->residency==NON_RESIDENT)
	check_list_newbie(p->lower_name);
    else
	check_list_resident(p);
}

  /* command to change gender */

  void gender(player *p,char *str)
  {
    *str=tolower(*str);
    switch(*str) {
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
      tell_player(p,"No gender set, Format : gender m/f/n\n");
      break;
    }
  }


  /* say command */

void say(player *p,char *str)
{
    char *oldstack,*text,*mid,*scan,*pipe,*prepipe;
    player *s;
    oldstack=stack;

    command_type=ROOM;

    if (!*str) {
	tell_player(p,"Format: say <msg>\n");
	return;
    }
    extract_pipe_local(str);
    if (sys_flags&FAILED_COMMAND) {
	sys_flags &= ~FAILED_COMMAND;
	return;  
    }
    for(scan=str;*scan;scan++);
    switch(*(--scan)) {
    case '?':
	mid="asks";
	break;
    case '!':
	mid="exclaims";
	break;
    default:
	mid="says";
	break;
    }

    for(s=p->location->players_top;s;s=s->room_next) 
	if (s!=current_player) {
	    prepipe=stack;
	    pipe=do_pipe(s,str);
	    if (!pipe) {
		cleanup_tag(pipe_list,pipe_matches);
		stack=oldstack;
		return;
	    }
	    text=stack;
	    if (s->saved_flags&NOPREFIX)
		sprintf(stack,"%s %s '%s'\n",p->name,mid,pipe);
	    else
		sprintf(stack,"%s %s '%s'\n",full_name(p),mid,pipe);
	    stack=end_string(stack);
	    tell_player(s,text);
	    stack=prepipe;
	}

    pipe=do_pipe(p,str);
    if (!pipe) {
	cleanup_tag(pipe_list,pipe_matches);
	stack=oldstack;
	return;
    }
    switch(*scan) {
    case '?':
	mid="ask";
	break;
    case '!':
	mid="exclaim";
	break;
    default:
	mid="say";
	break;
    }
    text=stack;
    sprintf(stack,"You %s '%s'\n",mid,pipe);
    stack=end_string(stack);
    tell_player(p,text);
    cleanup_tag(pipe_list,pipe_matches);
    stack=oldstack;
    sys_flags &= ~PIPE;
}

 /* Count  !!s and caps in a str */

int count_caps(char *str)
{
    int count=-2;
    char *mark;

    for (mark=str;*mark;mark++)
	if (isupper(*mark) || *mark=='!') count++;
    return (count>0?count:0);
}

  /* shout command command */

void shout(player *p,char *str)
{
    char *oldstack,*text,*mid,*scan,*pipe,*prepipe;
    player *s;
    oldstack=stack;

    command_type=EVERYONE;

    if (!*str) {
	tell_player(p,"Format: shout <msg>\n");
	return;
    }

    if (p->saved_flags&BLOCK_SHOUT) {
	tell_player(p,"You can't shout whilst ignoring shouts yourself.\n");
	return;
    }

    if (p->no_shout || p->saved_flags&SAVENOSHOUT) {
	if (p->no_shout>0) 
	    sprintf(stack,"You have been prevented from shouting for the "
		    "next %s.\n",word_time(p->no_shout));
	else
	    strcpy(stack,"You have been prevented from shouting for the "
		   "forseable future.\n");
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }

    if (p->shout_index>60 || strstr(str,"SFSU")) {
	tell_player(p,"You seem to have gotten a sore throat by shouting too"
		    " much!\n");
	return;
    }
    p->shout_index+=(count_caps(str)*2)+20;
    
    extract_pipe_global(str);
    if (sys_flags&FAILED_COMMAND) {
	sys_flags &= ~FAILED_COMMAND;
	stack=oldstack;
	return;  
    }
    for(scan=str;*scan;scan++);
    switch(*(--scan)) {
    case '?':
	mid="shouts, asking";
	break;
    case '!':
	mid="yells";
	break;
    default:
	mid="shouts";
	break;
    }

    for(s=flatlist_start;s;s=s->flat_next) 
	if (s!=current_player) {
	    prepipe=stack;
	    pipe=do_pipe(s,str);
	    if (!pipe) {
		cleanup_tag(pipe_list,pipe_matches);
		stack=oldstack;
		return;
	    }
	    text=stack;
	    if (s->saved_flags&NOPREFIX)
		sprintf(stack,"%s %s '%s'\n",p->name,mid,pipe);
	    else
		sprintf(stack,"%s %s '%s'\n",full_name(p),mid,pipe);
	    stack=end_string(stack);
	    tell_player(s,text);
	    stack=prepipe;
	}

    pipe=do_pipe(p,str);
    if (!pipe) {
	cleanup_tag(pipe_list,pipe_matches);
	stack=oldstack;
	return;
    }
    switch(*scan) {
    case '?':
	mid="shout, asking";
	break;
    case '!':
	mid="yell";
	break;
    default:
	mid="shout";
	break;
    }
    text=stack;
    sprintf(stack,"You %s '%s'\n",mid,pipe);
    stack=end_string(stack);
    tell_player(p,text);
    cleanup_tag(pipe_list,pipe_matches);
    stack=oldstack;
    sys_flags &= ~PIPE;
}



 /* emote command */

 void emote(player *p,char *str)
 {
   char *oldstack,*text,*mid,*pipe,*prepipe;
   player *s;
   oldstack=stack;

   command_type=ROOM;

   if (!*str) {
     tell_player(p,"Format: emote <msg>\n");
     return;
   }
   extract_pipe_local(str);
   if (sys_flags&FAILED_COMMAND) {
     sys_flags &= ~FAILED_COMMAND;
     return;  
   }

   for(s=p->location->players_top;s;s=s->room_next) 
     if (s!=current_player) {
       prepipe=stack;
       pipe=do_pipe(s,str);
       if (!pipe) {
	 cleanup_tag(pipe_list,pipe_matches);
	 stack=oldstack;
	 return;
       }
       text=stack;
       if (*str==39)
	 sprintf(stack,"%s%s\n",p->name,pipe);
       else
	 sprintf(stack,"%s %s\n",p->name,pipe);
       stack=end_string(stack);
       tell_player(s,text);
       stack=prepipe;
     }

   pipe=do_pipe(p,str);
   if (!pipe) {
     cleanup_tag(pipe_list,pipe_matches);
     stack=oldstack;
     return;
   }
   text=stack;
   if (*str==39)
     sprintf(stack,"You emote : %s%s\n",p->name,pipe);
   else
     sprintf(stack,"You emote : %s %s\n",p->name,pipe);
   stack=end_string(stack);
   tell_player(p,text);
   cleanup_tag(pipe_list,pipe_matches);
   stack=oldstack;
   sys_flags &= ~PIPE;
 }


 /* echo command */

 void echo(player *p,char *str)
 {
   char *oldstack,*text,*mid,*pipe,*prepipe;
   player *s;
   oldstack=stack;

   command_type=ROOM|ECHO_COM;

   if (!*str) {
     tell_player(p,"Format: echo <msg>\n");
     return;
   }
   extract_pipe_local(str);
   if (sys_flags&FAILED_COMMAND) {
     sys_flags &= ~FAILED_COMMAND;
     return;  
   }

   for(s=p->location->players_top;s;s=s->room_next) 
     if (s!=current_player) {
       prepipe=stack;
       pipe=do_pipe(s,str);
       if (!pipe) {
	 cleanup_tag(pipe_list,pipe_matches);
	 stack=oldstack;
	 return;
       }
       text=stack;
       sprintf(stack,"%s\n",pipe);
       stack=end_string(stack);
       tell_player(s,text);
       stack=prepipe;
     }

   pipe=do_pipe(p,str);
   if (!pipe) {
     cleanup_tag(pipe_list,pipe_matches);
     stack=oldstack;
     return;
   }
   text=stack;
   sprintf(stack,"You echo : %s\n",pipe);
   stack=end_string(stack);
   tell_player(p,text);
   cleanup_tag(pipe_list,pipe_matches);
   stack=oldstack;
   sys_flags &= ~PIPE;
 }


 /* the tell command */

void tell(player *p,char *str)
{
    char *msg,*pstring,*final,*mid,*scan;
    char *oldstack;
    player **list,**step;
    int i,n;

    command_type=PERSONAL|SEE_ERROR;

    if (p->saved_flags&BLOCK_TELLS) {
	tell_player(p,"You can't tell other people when you yourself "
		    "are blocking tells.\n");
	return;
    }  
    
    oldstack=stack;
    align(stack);
    list=(player **)stack;

    msg=next_space(str);
    if (!*msg) { 
	tell_player(p,"Format : tell <player(s)> <msg>\n");
	return;
    }
    for(scan=msg;*scan;scan++);
    switch(*(--scan)) {
    case '?':
	mid="asks of";
	break;
    case '!':
	mid="exclaims to";
	break;
    default:
	mid="tells";
	break;
    }
    *msg++=0;
    command_type|=SORE;
    n=global_tag(p,str);
    if (!n) {
	stack=oldstack;
	return;
    }

    /* for reply */
    
    if (strcasecmp(str,"everyone") && strcasecmp(str,"room") &&
	strcasecmp(str,"friends"))
	make_reply_list(p,list,n);
    else
	p->shout_index+=count_caps(str)+17;

    for(step=list,i=0;i<n;i++,step++)
	if (*step!=p) {
	    pstring=tag_string(*step,list,n);
	    final=stack;
	    if ((*step)->saved_flags&NOPREFIX)
		sprintf(stack,"%s %s %s '%s'\n",p->name,mid,pstring,msg);
	    else
		sprintf(stack,"%s %s %s '%s'\n",full_name(p),mid,pstring,msg);
	    stack=end_string(stack);
	    tell_player(*step,final);
	    stack=pstring;
	}

    if (sys_flags&EVERYONE_TAG || sys_flags&FRIEND_TAG || sys_flags&ROOM_TAG ||
	!(sys_flags&FAILED_COMMAND)) {
	switch(*scan) {
	case '?':
	    mid="ask of";
	    break;
	case '!':
	    mid="exclaim to";
	    break;
	default:
	    mid="tell";
	    break;
	}
	pstring=tag_string(p,list,n);
	final=stack;
	sprintf(stack,"You %s %s '%s'\n",mid,pstring,msg);
	stack=strchr(stack,0);
	if (idlingstring(p,list,n)) 
	    strcpy(stack,idlingstring(p,list,n));
	stack=end_string(stack);
	tell_player(p,final);
    }
    cleanup_tag(list,n);
    stack=oldstack;
}


 /* the wake command */

void wake(player *p,char *str)
{
    char *oldstack;
    player *p2;

    command_type=PERSONAL|SEE_ERROR;
    oldstack=stack;

    if (p->saved_flags&BLOCK_TELLS) {
      tell_player(p,"You can't wake other people when you yourself are blocking tells.\n");
      return;
    }  
    if (!*str) {
      tell_player(p,"Format : wake <player>\n");
      return;
    }
    p2=find_player_global(str);
    if (!p2) return;

    if (p2->saved_flags&NOPREFIX)
	sprintf(stack,"!!!!!!!!!! OI !!!!!!!!!!! WAKE UP, %s wants to speak to you.\007\n",p->name);
    else
	sprintf(stack,"!!!!!!!!!! OI !!!!!!!!!!! WAKE UP, %s wants to speak to you.\007\n",full_name(p));
    stack=end_string(stack);
    tell_player(p2,oldstack);

    if (sys_flags&FAILED_COMMAND) {
      stack=oldstack;
      return;
    }
    stack=oldstack;
    sprintf(stack,"You scream loudly at %s in an attempt to wake %s up.\n",
	    full_name(p2),get_gender_string(p2));
    stack=strchr(stack,0);
    if (p2->idle_msg[0]!=0) 
	sprintf(stack," Idling> %s %s\n",p2->name,p2->idle_msg);
    stack=end_string(stack);
    tell_player(p,oldstack);

    stack=oldstack;
  }

  /* remote command */

void remote(player *p,char *str)
{
    char *msg,*pstring,*final;
    char *oldstack;
    player **list,**step;
    int i,n;

    command_type=PERSONAL|SEE_ERROR;

    if (p->saved_flags&BLOCK_TELLS) {
	tell_player(p,"You can't remote to other people when you yourself are blocking tells.\n");
	return;
    }  

    oldstack=stack;
    align(stack);
    list=(player **)stack;

    msg=next_space(str);
    if (!*msg) { 
	tell_player(p,"Format : remote <player(s)> <msg>\n");
	stack=oldstack;
	return;
    }
    *msg++=0;
    command_type|=SORE;
    n=global_tag(p,str);
    if (!n) {
	stack=oldstack;
	return;
    }
    for(step=list,i=0;i<n;i++,step++) 
	if (*step!=p) {
	    final=stack;
	    if (*msg=='\'')
		sprintf(stack,"%s%s\n",p->name,msg);
	    else
		sprintf(stack,"%s %s\n",p->name,msg);
	    stack=end_string(stack);
	    tell_player(*step,final);
	    stack=final;
	}

    if (sys_flags&EVERYONE_TAG || !(sys_flags&FAILED_COMMAND)) {
	pstring=tag_string(p,list,n);
	final=stack;
	if (*msg==39)
	    sprintf(stack,"You emote '%s%s' to %s.\n",p->name,msg,pstring);
	else
	    sprintf(stack,"You emote '%s %s' to %s.\n",p->name,msg,pstring);
	stack=strchr(stack,0);
	if (idlingstring(p,list,n))
	    strcpy(stack,idlingstring(p,list,n));
	stack=end_string(stack);
	tell_player(p,final);
    }
    cleanup_tag(list,n);
    stack=oldstack;
}

  /* recho command */

  void recho(player *p,char *str)
  {
    char *msg,*pstring,*final;
    char *oldstack;
    player **list,**step;
    int i,n;

    command_type=PERSONAL|ECHO_COM|SEE_ERROR;

    if (p->saved_flags&BLOCK_TELLS) {
      tell_player(p,"You can't recho to other people when you yourself are blocking tells.\n");
      return;
    }  

    oldstack=stack;
    align(stack);
    list=(player **)stack;

    msg=next_space(str);
    if (!*msg) { 
      tell_player(p,"Format : recho <player(s)> <msg>\n");
      stack=oldstack;
      return;
    }
    *msg++=0;
    command_type|=SORE;
    n=global_tag(p,str);
    if (!n) {
      stack=oldstack;
      return;
    }
    for(step=list,i=0;i<n;i++,step++) 
      if (*step!=p) {
	final=stack;
	sprintf(stack,"+ %s\n",msg);
	stack=end_string(stack);
	tell_player(*step,final);
	stack=final;
      }

    if (sys_flags&EVERYONE_TAG || !(sys_flags&FAILED_COMMAND)) {
      pstring=tag_string(p,list,n);
      final=stack;
      sprintf(stack,"You echo '+ %s' to %s\n",msg,pstring);
      while (*stack) stack++;
      if (idlingstring(p,list,n))
	  strcpy(stack,idlingstring(p,list,n));
      stack=end_string(stack);
      tell_player(p,final);
    }
    cleanup_tag(list,n);
    stack=oldstack;
  }

  /* whisper command */

  void whisper(player *p,char *str)
  {
    char *oldstack,*msg,*everyone,*text,*pstring,*mid,*s;
    player **list,*scan;
    int n;

    if (p->saved_flags&BLOCK_TELLS) {
      tell_player(p,"You can't whisper to other people when you yourself are blocking tells.\n");
      return;
    }  

    command_type=ROOM|SEE_ERROR;

    oldstack=stack;
    align(stack);
    list=(player **)stack;

    msg=next_space(str);
    if (!*msg) {
      tell_player(p,"Format whisper <person(s)> <msg>\n");
      stack=oldstack;
      return;
    }
    *msg++=0;
    for(s=msg;*s;s++);
    switch(*(--s)) {
    case '?':
      mid="asks in a whisper";
      break;
    case '!':
      mid="exclaims in a whisper";
	break;
    default:
      mid="whispers";
      break;
    }
    n=local_tag(p,str);
    if (!n) return;
    everyone=tag_string(0,list,n);
    for(scan=p->location->players_top;scan;scan=scan->room_next) 
      if (p!=scan) 
	if (scan->flags&TAGGED) {
	  pstring=tag_string(scan,list,n);
	  text=stack;
	  if (scan->saved_flags&NOPREFIX)
	      sprintf(stack,"%s %s '%s' to %s.\n",p->name,mid,msg,pstring);
	  else
	      sprintf(stack,"%s %s '%s' to %s.\n",full_name(p),mid,msg,pstring);
	  stack=end_string(stack);
	  tell_player(scan,text);
	  stack=pstring;
	}
	else {
	  text=stack;
	  if (scan->saved_flags&NOPREFIX)
	      sprintf(stack,"%s whispers something to %s.\n",p->name,everyone);
	  else
	      sprintf(stack,"%s whispers something to %s.\n",full_name(p),everyone);
	  stack=end_string(stack);
	  tell_player(scan,text);
	  stack=text;
	}
    if (!(sys_flags&FAILED_COMMAND)) {
      switch(*s) {
      case '?':
	mid="ask in a whisper";
	break;
      case '!':
	mid="exclaim in a whisper";
	break;
      default:
	mid="whisper";
	break;
      } 
      pstring=tag_string(p,list,n);
      text=stack;
      sprintf(stack,"You %s '%s' to %s.\n",mid,msg,pstring);
      stack=strchr(stack,0);
      if (idlingstring(p,list,n))
	  strcpy(stack,idlingstring(p,list,n));
      stack=end_string(stack);
      tell_player(p,text);
  }
    cleanup_tag(list,n);
    stack=oldstack;
}

/* exclude command */

void exclude(player *p,char *str)
{
    char *oldstack,*msg,*everyone,*text,*pstring,*mid,*s;
    player **list,*scan;
    int n;
    
    command_type=ROOM|SEE_ERROR;
    
    oldstack=stack;
    align(stack);
    list=(player **)stack;
    
    msg=next_space(str);
    if (!*msg) {
	tell_player(p,"Format exclude <person(s)> <msg>\n");
	stack=oldstack;
	return;
    }
    *msg++=0;
    for(s=msg;*s;s++);
    switch(*(--s)) {
    case '?':
	mid="asks";
	break;
    case '!':
	mid="exclaims to";
	break;
    default:
	mid="tells";
	break;
    }
    n=local_tag(p,str);
    if (!n) return;
    everyone=tag_string(0,list,n);
    for(scan=p->location->players_top;scan;scan=scan->room_next) 
	if (p!=scan) 
	    if (scan->flags&TAGGED) {
		pstring=tag_string(scan,list,n);
		text=stack;
		sprintf(stack,"%s tells everyone something about %s\n",
			full_name(p),pstring);
		stack=end_string(stack);
		tell_player(scan,text);
		stack=pstring;
	    }
	    else {
		text=stack;
		sprintf(stack,"%s %s everyone but %s '%s'\n",
			full_name(p),mid,everyone,msg);
		stack=end_string(stack);
		tell_player(scan,text);
		stack=text;
	    }
    if (!(sys_flags&FAILED_COMMAND)) {
	switch(*s) {
	case '?':
	    mid="ask";
	    break;
	case '!':
	    mid="exclaim to";
	    break;
	default:
	    mid="tell";
	    break;
	} 
	pstring=tag_string(p,list,n);
	text=stack;
	sprintf(stack,"You %s everyone but %s '%s'\n",mid,pstring,msg);
	stack=end_string(stack);
	tell_player(p,text);
    }
    cleanup_tag(list,n);
    stack=oldstack;
}


/* pemote command */

void pemote(player *p,char *str)
{
    char *oldstack,*scan;
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: pemote <msg>\n");
	return;
    }
    
    for(scan=p->lower_name;*scan;scan++);
    if (*(scan-1)=='s') *stack++=39;
    else {
	*stack++=39;
	*stack++='s';
    }
    *stack++=' ';
    while(*str) *stack++=*str++;
    *stack++=0;
    emote(p,oldstack);
    stack=oldstack;
}


/* premote command */

void premote(player *p,char *str)
{
    char *oldstack,*scan;
    oldstack=stack;
    
    scan=next_space(str);
    if (!*scan) {
      tell_player(p,"Format: pemote <person> <msg>\n");
      return;
    }

    while(*str && *str!=' ') *stack++=*str++;
    *stack++=' ';
    if (*str) str++;
    for(scan=p->lower_name;*scan;scan++);
    if (*(scan-1)=='s') *stack++=39;
    else {
      *stack++=39;
      *stack++='s';
    }
    *stack++=' ';
    while(*str) *stack++=*str++;
    *stack++=0;
    remote(p,oldstack);
    stack=oldstack;
  }


  /* save command */

void do_save(player *p,char *str)
{
    if (p->residency==NON_RESIDENT) {
	log("error","Tried to save a non-resi, (chris)");
	return;
    }
    save_player(p);
}

  /* show email */

  void check_email(player *p,char *str)
  {
    char *oldstack;
    oldstack=stack;
    if (p->residency==NON_RESIDENT) {
      tell_player(p,"You are non resident and so cannot set an email address.\n"
		  "Please ask a super user to make you resident.\n");
      return;
    }
    if (p->email[0]==-1) 
      tell_player(p,"You have declared that you have no email address.\n");
    else {
      sprintf(stack,"Your email address is set to :\n%s\n",p->email);
      stack=end_string(oldstack);
      tell_player(p,oldstack);
    }
    if (p->saved_flags&PRIVATE_EMAIL) tell_player(p,"Your email is private.\n");
    else tell_player(p,"Your email is public for all to read.\n");
    stack=oldstack;
  }

  /* change whether an email is public or private */

  void public_com(player *p,char *str)
  {
    if (!strcmp("on",str)) p->saved_flags &= ~PRIVATE_EMAIL;
     else if (!strcmp("off",str)) p->saved_flags |= PRIVATE_EMAIL;
     else p->saved_flags ^= PRIVATE_EMAIL;

     if (p->saved_flags&PRIVATE_EMAIL) 
       tell_player(p,"Your email is private, only the admin will be able to see it.\n");
     else
       tell_player(p,"Your email address is public, so everyone can see it.\n");
   }

   /* email command */

void change_email(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    if (p->residency==NON_RESIDENT) {
	tell_player(p,"You may only use the email command once resident.\n"
		    "Please ask a superuser to become to grant you residency.\n");
	return;
    }
    if (!*str) {
	check_email(p,str);
	return;
    }
    if (!strlen(p->email)) {
	sprintf(stack,"%-18s %s (New)",p->name,str);
	stack=end_string(stack);
	log("help",oldstack);
	stack=oldstack;
    }
    strncpy(p->email,str,MAX_EMAIL-2);
    sprintf(oldstack,"Email address has been changed to: %s\n",p->email);
    stack=end_string(oldstack);
    tell_player(p,oldstack);
    p->residency &= ~NO_SYNC;
    save_player(p);
    stack=oldstack;
    return;
}

   /* password changing routines */

char *do_crypt(char *entered,player *p)
{
    char key[9];
    strncpy(key,entered,8);
    return crypt(key,p->lower_name);
}


void got_password2(player *p,char *str)
{
    password_mode_off(p);
    p->input_to_fn=0;
    p->flags |= PROMPT;
    if (strcmp(p->password_cpy,str)) {
	tell_player(p,"\nBut that doesn't match !!!\n"
		    "Password not changed ...\n");
    }
    else {
	strcpy(p->password,do_crypt(str,p));
	tell_player(p,"\nPassword has now been changed.\n");
	p->residency &= ~NO_SYNC;
	save_player(p);
    }
}

void got_password1(player *p,char *str)
{
    if (strlen(str)>(MAX_PASSWORD-2)) {
	do_prompt(p,"\nPassword too long, please try again.\n"
		  "Please enter a shorter password:");
	p->input_to_fn=got_password1;
    }
    else {
	strcpy(p->password_cpy,str);
	do_prompt(p,"\nEnter password again to verify:");
	p->input_to_fn=got_password2;
    }
}

void validate_password(player *p,char *str)
{
    if (!check_password(p->password,str,p)) {
	tell_player(p,"\nHey ! thats the wrong password !!\n");
	password_mode_off(p);
	p->input_to_fn=0;
	p->flags |= PROMPT;
    }
    else {
	do_prompt(p,"\nNow enter a new password:");
	p->input_to_fn=got_password1;
    }
}

void change_password(player *p,char *str)
{
    if (p->residency==NON_RESIDENT) {
	tell_player(p,"You may only set a password once resident.\n"
		    "To become a resident, please ask a superuser.\n");
	return;
    }
    password_mode_on(p);
    p->flags &= ~PROMPT;
    if (p->password[0] && p->password[0]!=-1) {
	do_prompt(p,"Please enter your current password:");
	p->input_to_fn=validate_password;
    }
    else {
	do_prompt(p,"You have no password.\n"
		  "Please enter a password:");
	p->input_to_fn=got_password1;
    }
}


   /* the 'check' command */

void check(player *p,char *str)
{
    if (!*str) {
	tell_player(p,"Format: check <sub command>\n");
	return;
    }
    sub_command(p,str,check_list);
}

   /* view check commands */

   void view_check_commands(player *p,char *str)
   {
     view_sub_commands(p,check_list);
   }





   /* show wrap info */

void check_wrap(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    if (p->term_width) {
	sprintf(stack,"Line wrap on, with terminal width set to %d characters.\n",
		p->term_width);
	stack=strchr(stack,0);
	if (p->word_wrap) 
	    sprintf(stack,"Word wrap is on, with biggest word size set to %d characters.\n",
		    p->word_wrap);
	else
	    strcpy(stack,"Word wrap is off.\n");
	stack=end_string(stack);
	tell_player(p,oldstack);
    }
    else tell_player(p,"Line wrap and word wrap turned off.\n");
    stack=oldstack;
}

 /* Toggle the ignoreprefix flag (saved_flags) */

void ignoreprefix(player *p,char *str)
{
    if (!strcmp("on",str)) p->saved_flags &= ~NOPREFIX;
    else if (!strcmp("off",str)) p->saved_flags |= NOPREFIX;
    else p->saved_flags ^= NOPREFIX;
    
    if (p->saved_flags&NOPREFIX) 
	tell_player(p,"You are now ignoring prefixes.\n");
    else
	tell_player(p,"You are now seeing prefixes again.\n");
}

void set_time_delay(player *p,char *str)
{
    int time_diff;
    char *oldstack;
    
    oldstack=stack;
    if (!*str) {
	if (p->jetlag)
	    sprintf(stack,"Your time difference is currently set at %d hours.\n",p->jetlag);
	else
	    sprintf(stack,"Your time difference is not currently set.\n");
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return;
    }
    time_diff=atoi(str);
    if (!time_diff) {
	tell_player(p,"Time difference of 0 hours set. ( that was worth it, wasn't it... )\n");
	p->jetlag=0;
	return;
    }
    if (time_diff<-23 || time_diff>23) {
	tell_player(p,"That's a bit silly, isn't it?\n");
	return;
    }
    p->jetlag=time_diff;
    
    oldstack=stack;
    if (p->jetlag==1) 
	strcpy(stack,"Time Difference set to 1 hour.\n");
    else
	sprintf(stack,"Time Difference set to %d hours.\n",p->jetlag);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

void set_ignore_msg(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"You reset your ignore message.\n");
	strcpy(p->ignore_msg,"");
	return;
    }
    
    strncpy(p->ignore_msg,str,MAX_IGNOREMSG-2);
    strcpy(stack,"Ignore message now set to ...\n");
    stack=strchr(stack,0);
    sprintf(stack,"%s\n",p->ignore_msg);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}
	

void think(player *p,char *str)
{
    char *oldstack;
    player *scan;
    room *r;
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: think <msg>\n");
	return;
    }
    sprintf(stack,"%s thinks . o O ( %s )\n",p->name,str);
    stack=end_string(stack);
    r=p->location;
    if (!r) {
	tell_player(p,"You don't appear to be anywhere !\n");
	return;
    }
    scan=r->players_top;
    if (!scan) {
	log("error","sominks buggered (think)");
	return;
    }
    command_type|=ROOM;
    for (;scan;scan=scan->room_next) 
	if (scan!=p)
	    tell_player(scan,oldstack);

    command_type&=~ROOM;
    stack=oldstack;
    sprintf(stack,"You emote: %s thinks . o O ( %s )\n",p->name,str);
    stack=end_string(stack);
    tell_player(p,oldstack);
    stack=oldstack;
}

