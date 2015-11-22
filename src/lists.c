/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  lists.c
*/

#include <ctype.h>
#include <string.h>
#ifndef MIPS
#include <malloc.h>
#endif
#include <fcntl.h>
#include <memory.h>


#include "config.h"
#include "player.h"
#include "fix.h"

/* externs */

extern char *upper_from_saved(saved_player *sp);
extern void check_list_newbie(char *lowername);
extern void check_list_resident(player *p);
extern char *check_legal_entry(player *,char *,int);
extern char *list_lines(list_ent *);
extern char *store_string(),*store_int();
extern char *get_string(),*get_int();
extern char *gstring_possessive(),*end_string(),*next_space();
extern player *find_player_global_quiet(char *),*find_player_global(char *);
extern player *find_player_absolute_quiet(char *);
extern saved_player *find_saved_player(char *);
extern void log(char *,char *);
extern void tell_player(player *,char *);
extern char *number2string(int);
extern int get_flag(flag_list *,char *);
extern char *full_name(player *);
extern char *get_gender_string(player *);
extern void save_player(player *);
extern void pager(player *,char *,int);
extern int global_tag(player *,char *);
extern void cleanup_tag(player **,int);

extern saved_player **saved_hash[];

/* interns */

flag_list list_flags[] = {
   {"noisy",NOISY},
   {"ignore",IGNORE},
   {"inform",INFORM},
   {"grab",GRAB},
   {"grabme",GRAB},
   {"friend",FRIEND},
   {"bar",BAR},
   {"invite",INVITE},
   {"beep",BEEP},
   {"block",BLOCK},
   {"key",KEY},
   {"find",FIND},
   {"nsy",NOISY},
   {"ign",IGNORE},
   {"inf",INFORM},
   {"grb",GRAB},
   {"fnd",FRIEND},
   {"inv",INVITE},
   {"bep",BEEP},
   {"blk",BLOCK},
   {"key", KEY},
   {"find",FIND},
    {0,0} };

flag_list show_flags[]={
 {"echo",TAG_ECHO},
 {"echos",TAG_ECHO},
 {"room",TAG_ROOM},
 {"local",TAG_ROOM},
 {"shout",TAG_SHOUT},
 {"shouts",TAG_SHOUT},
 {"tell",TAG_PERSONAL},
 {"tells",TAG_PERSONAL},
 {"autos",TAG_AUTOS},
 {"auto",TAG_AUTOS},
  {0,0} };

char *message_string;

/* delete and entry from someones list */

void delete_entry(saved_player *sp,list_ent *l)
{
  list_ent *scan;
  if (!sp) return;
  scan=sp->list_top;
  if (scan==l) {
      sp->list_top=l->next;
      FREE(l);
      return;
  }
  while(scan) 
      if (scan->next==l) {
	  scan->next=l->next;
	  FREE(l);
	  return;
      }
      else scan=scan->next;
  log("error","Tried to delete list entry that wasn't there.\n");
}


/* compress list */

void tmp_comp_list(saved_player *sp)
{
  char *oldstack;
  list_ent *l,*next;
  
  l=sp->list_top;

  oldstack=stack;
  stack=store_int(stack,0);
  
  while(l) {
    next=l->next;
    if (!l->name[0]) {
      log("error","Bad list entry on compress .. auto deleted.\n");
      delete_entry(sp,l);
    }
    else {
      stack=store_string(stack,l->name);
      stack=store_int(stack,l->flags);
    }
    l=next;
  }
  store_int(oldstack,((int)stack-(int)oldstack));
}

void compress_list(saved_player *sp)
{
  char *oldstack;
  int length;
  list_ent *new,*l,*next;
  if (sp->saved_flags&COMPRESSED_LIST) return;
  sp->saved_flags|=COMPRESSED_LIST;
  oldstack=stack;
  tmp_comp_list(sp);
  length=(int)stack-(int)oldstack;
  if (length==4) {
    sp->list_top=0;
    stack=oldstack;
    return;
  }
  new=(list_ent *)MALLOC(length);
  memcpy(new,oldstack,length);
  
  l=sp->list_top;
  while(l) {
    next=l->next;
    FREE(l);
    l=next;
  }
  sp->list_top=new;
  stack=oldstack;
}

/* decompress list */

void decompress_list(saved_player *sp)
{
  list_ent *l;
  char *old,*end,*start;
  int length;

  if (!(sp->saved_flags&COMPRESSED_LIST)) return;
  sp->saved_flags &= ~COMPRESSED_LIST;
  
  old=(char *)sp->list_top;
  start=old;
  if (!old) return;
  old=get_int(&length,old);
  end=old+length-4;
  sp->list_top=0;
  while(old<end) {
    l=(list_ent *)MALLOC(sizeof(list_ent));
    old=get_string(stack,old);
    strncpy(l->name,stack,MAX_NAME-2);
    old=get_int(&(l->flags),old);
    l->next=sp->list_top;
    sp->list_top=l;
  }
  FREE(start);
}



/* save list */

void construct_list_save(saved_player *sp)
{
  int length;
  char *where;
  
  if (!(sp->saved_flags&COMPRESSED_LIST) &&
      (!find_player_absolute_quiet(sp->lower_name))) 
    compress_list(sp);
  
  if (sp->saved_flags&COMPRESSED_LIST) {
    if (sp->list_top) {
      where=(char *)sp->list_top;
      (void)get_int(&length,where);
      memcpy(stack,where,length);
      stack+=length;
    }
    else stack=store_int(stack,4);
  }
  else tmp_comp_list(sp);
}

/* retrieve list */

char *retrieve_list_data(saved_player *sp,char *where)
{
  int length;
  (void)get_int(&length,where);
  if (length==4) sp->list_top=0;
  else {
    sp->saved_flags |= COMPRESSED_LIST;
    sp->list_top=(list_ent *)MALLOC(length);
    memcpy(sp->list_top,where,length);
  }
  where+=length;
  return where;
}

/* general routine to output a flag field */

char *bit_string(int flags)
{
  static char out[33];
  int i;
  for(i=0;i<32;i++,flags>>=1)
    if (flags&1) out[i]='*';
    else out[i]=' ';
  out[32]=0;
  return out;
}


/* count list entries */

int count_list(player *p)
{
  list_ent *l;
  int count=0;
  if (!p->saved) return 0;
  for(l=p->saved->list_top;l;l=l->next) count++;
  return count;
}

/* find list entry for a person */

list_ent *find_list_entry(player *p,char *name)
{
  list_ent *l;
  if (!p->saved) return 0;
  decompress_list(p->saved);
  l=p->saved->list_top;
  while(l) 
    if (!strcasecmp(l->name,name)) return l;
    else l=l->next;
  return 0;
}

/* create a list entry */

list_ent *create_entry(player *p,char *name)
{
  list_ent *l,*e;
  if (!p->saved) return 0;
  if ((count_list(p))>=(p->max_list)) {
    tell_player(p,"Can't create new list entry, because your list is full.\n");
    return 0;
  }
  l=(list_ent *)MALLOC(sizeof(list_ent));
  strncpy(l->name,name,MAX_NAME-2);
  e=find_list_entry(p,"everyone");
  if (e) l->flags=e->flags;
  else l->flags=0;
  l->next=p->saved->list_top;
  p->saved->list_top=l;
  return l;
}


/* find list entry for a person from saved file */

list_ent *fle_from_save(saved_player *sp,char *name)
{
  list_ent *l;
  decompress_list(sp);
  l=sp->list_top;
  while(l) 
    if (!strcasecmp(l->name,name)) return l;
    else l=l->next;
  return 0;
}


/* clear bits of list */

void clear_list(player *p,char *str)
{
  char *oldstack,*text,msg[50];
  int count=0;
  list_ent *l;
  
  oldstack=stack;

  if (!p->saved) {
    tell_player(p,"You do not have a list to alter, since you have no save file\n");
    return;
  }

  if (!*str) {
      tell_player(p,"Format: clist <person>\n");
      return;
  }

  while(*str) {
    while(*str && *str!=',' && *str!=' ') *stack++=*str++;
    if (*str) str++;
    *stack++=0;
    if (*oldstack) {
      l=find_list_entry(p,oldstack);
      if (!l) {
	text=stack;
	sprintf(stack,"Can't find any list entry for '%s'.\n",oldstack);
	stack=end_string(stack);
	tell_player(p,text);
      }
      else {
	count++;
	sprintf(msg,"Entry removed for '%s'\n",l->name);
	tell_player(p,msg);
	delete_entry(p->saved,l);
      }
    }
    stack=oldstack;
  }
  if (!count) tell_player(p,"No entries removed.\n");
  else {
      if (count!=1)
	  sprintf(stack,"Deleted %s entries.\n",number2string(count));
      else
	  strcpy(stack,"Deleted 1 entry.\n");
    stack=end_string(stack);
    tell_player(p,oldstack);
  }
  stack=oldstack;
}


/* turn a flag list into an int */

int get_flags(player *p,char *flags,flag_list *list)
{
  int flag=0,new;
  char *oldstack,*text;

  oldstack=stack;
  while(*flags && *flags==' ') flags++;
  while(*flags) {
    while(*flags && *flags!=',') *stack++=*flags++;
    if (*flags) flags++;
    *stack++=0;
    new=get_flag(list,oldstack);
    if (!new) {
      text=stack;
      sprintf(stack,"Can't find flag '%s'.\n",oldstack);
      stack=end_string(stack);
      tell_player(p,text);
    }
    else flag |= new;
    stack=oldstack;
  }
  return flag;
}


/* force change on list */

void change_list_absolute(player *p,char *str)
{
    char *flag_list,*oldstack,msg[50],*dummyname;
    int change,count=0;
    list_ent *l;
    player *tell;

    if (!p->saved) {
	tell_player(p,"You haven't got a list since you have no save file.\n");
	return;
    }
    oldstack=stack;
    flag_list=next_space(str);
    if (!*flag_list) {
	tell_player(p,"Format: flist <player list> <flag list>\n");
	return;
    }
    *flag_list++=0;
    change=get_flags(p,flag_list,list_flags);
    if (!change) {
	tell_player(p,"Bad flag list, no changes made.\n");
	return;
    }
  
    while(*str) {
	while(*str && *str!=',') *stack++=*str++;
	*stack++=0;
	if (*str) str++;
	if (*oldstack && (dummyname=check_legal_entry(p,oldstack,0))) {
	    sprintf(msg,"Flag set for '%s'\n",dummyname);
	    tell_player(p,msg);
	    l=find_list_entry(p,dummyname);
	    if (!l)
		l=create_entry(p,dummyname);
	    if (l) {
		l->flags=change;
		count++;
		if (message_string) {
		    tell=find_player_absolute_quiet(oldstack);
		    if (tell) tell_player(tell,message_string);
		}
	    }
	}
	stack=oldstack;
    }
    if (count)
	tell_player(p,"List entries force changed.\n");
    else
	tell_player(p,"No changes to make.\n");
}



/* set change on list */

void set_list(player *p,char *str)
{
    char *flag_list,*oldstack,msg[50],*dummyname;
    int change,count=0;
    list_ent *l;
    player *tell;
    if (!p->saved) {
	tell_player(p,"You haven't got a list since you have no save file.\n");
	return;
    }
    oldstack=stack;
    flag_list=next_space(str);
    if (!*flag_list) {
	tell_player(p,"Format: slist <player list> <flag list>\n");
	return;
    }
    *flag_list++=0;
    change=get_flags(p,flag_list,list_flags);
    if (!change) {
	tell_player(p,"Bad flag list, no changes made.\n");
	return;
    }
  
    while(*str) {
	while(*str && *str!=',') *stack++=*str++;
	*stack++=0;
	if (*str) str++;
	if (*oldstack && (dummyname=check_legal_entry(p,oldstack,0))) {
	    sprintf(msg,"Flag set for '%s'\n",dummyname);
	    tell_player(p,msg);
	    l=find_list_entry(p,dummyname);
	    if (!l)
		l=create_entry(p,dummyname);
	    if (l) {
		l->flags |= change;
		count++;
		if (message_string) {
		    tell=find_player_absolute_quiet(oldstack);
		    if (tell) tell_player(tell,message_string);
		}
	    }
	}
	stack=oldstack;
    }
    if (count)
	tell_player(p,"List entries set.\n");
    else
	tell_player(p,"No changes to make.\n");
}


/* reset change on list */

void reset_list(player *p,char *str)
{
    char *flag_list,*oldstack,msg[50],*dummyname;
    int change,count=0;
    list_ent *l;
    player *tell;

    if (!p->saved) {
	tell_player(p,"You haven't got a list since you have no save file.\n");
	return;
    }
    oldstack=stack;
    flag_list=next_space(str);
    if (!*flag_list) {
	tell_player(p,"Format: rlist <player list> <flag list>\n");
	return;
    }
    *flag_list++=0;
    change=get_flags(p,flag_list,list_flags);
    if (!change) {
	tell_player(p,"Bad flag list, no changes made.\n");
	return;
    }
  
    while(*str) {
	while(*str && *str!=',') *stack++=*str++;
	*stack++=0;
	if (*str) str++;
	if (*oldstack && (dummyname=check_legal_entry(p,oldstack,0))) {
	    sprintf(msg,"Flags reset for '%s'\n",dummyname);
	    tell_player(p,msg);
	    l=find_list_entry(p,dummyname);
	    if (!l) l=create_entry(p,dummyname);
	    if (l) {
		l->flags &= ~change;
		if (!l->flags) delete_entry(p->saved,l);
		count++;
		if (message_string) {
		    tell=find_player_absolute_quiet(oldstack);
		    if (tell) tell_player(tell,message_string);
		}
	    }
	}
	stack=oldstack;
    }
    if (count)
	tell_player(p,"List entries reset.\n");
    else
	tell_player(p,"No changes to make.\n");
}

/* toggle change on list */

void toggle_list(player *p,char *str)
{
    char *flag_list,*oldstack,msg[50],*dummyname;
    int change,count=0;
    list_ent *l;
    player *tell;

    if (!p->saved) {
	tell_player(p,"You haven't got a list since you have no save file.\n");
	return;
    }
    oldstack=stack;
    flag_list=next_space(str);
    if (!*flag_list) {
	tell_player(p,"Format: tlist <player list> <flag list>\n");
	return;
    }
    *flag_list++=0;
    change=get_flags(p,flag_list,list_flags);
    if (!change) {
	tell_player(p,"Bad flag list, no changes made.\n");
	return;
    }
  
    while(*str) {
	while(*str && *str!=',') *stack++=*str++;
	*stack++=0;
	if (*str) str++;
	lower_case(oldstack);
	if (*oldstack && (dummyname=check_legal_entry(p,oldstack,0))) {
	    sprintf(msg,"Flag toggled for '%s'\n",dummyname);
	    tell_player(p,msg);
	    l=find_list_entry(p,dummyname);
	    if (!l) l=create_entry(p,dummyname);
	    if (l) {
		l->flags ^= change;
		if (!l->flags) delete_entry(p->saved,l);
		count++;
		if (message_string && l->flags&change) {
		    tell=find_player_absolute_quiet(oldstack);
		    if (tell) tell_player(tell,message_string);
		}
	    }
	}
	stack=oldstack;
    }
    if (count)
	tell_player(p,"List entries toggled.\n");
    else
	tell_player(p,"No changes to make.\n");
}



void noisy(player *p,char *str)
{
    char *oldstack,*cmd;
    oldstack=stack;
    
    if (!*str) {
	tell_player(p,"Format: noisy <player(s)> on/off/[blank]\n");
	return;
    }

    if (!check_legal_entry(p,str,1)) return;
    
    while(*str && *str!=' ') *stack++=*str++;
    if (*str) str++;
    strcpy(stack," noisy");
    stack=strchr(stack,0);
    *stack++=0;
    cmd=stack;
    message_string=cmd;
    if (!strcmp("off",str)) {
	sprintf(stack,"%s removes you from %s noisy list.\n",full_name(p),gstring_possessive(p));
	stack=end_string(stack);
	reset_list(p,oldstack);
	stack=oldstack;
	message_string=0;
	return;
    }
    if (!strcmp("on",str)) {
	sprintf(stack,"%s adds you to %s noisy list.\n",full_name(p),gstring_possessive(p));
	stack=end_string(stack);
	set_list(p,oldstack);
	stack=oldstack;
	message_string=0;
	return;
    }
    sprintf(stack,"%s adds you to %s noisy list.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    toggle_list(p,oldstack);
    message_string=0;
    stack=oldstack;
}


void ignore(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: ignore <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," ignore");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s starts listening to you again.\n",full_name(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s ignores you.\n",full_name(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s ignores you.\n",full_name(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}


void block(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: block <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," block");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s starts listening to you again.\n",full_name(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s blocks you off from doing tells to %s.\n",
	    full_name(p),get_gender_string(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s blocks you off from doing tells to %s\n",
	  full_name(p),get_gender_string(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void inform(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: inform <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," inform");
  stack=strchr(stack,0);
  *stack++=0;
  if (!strcmp("off",str)) {
    reset_list(p,oldstack);
    stack=oldstack;
    return;
  }
  if (!strcmp("on",str)) {
    set_list(p,oldstack);
    stack=oldstack;
    return;
  }
  toggle_list(p,oldstack);
  stack=oldstack;
}

void grab(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: grab <player(s)> on/off/[blank]\n");
    return;
  }
  
  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," grab");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s stops you from being able to grab %s.\n",full_name(p),get_gender_string(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s allows you to grab %s.\n",full_name(p),get_gender_string(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s allows you to grab %s.\n",full_name(p),get_gender_string(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void friend(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: friend <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;
	
  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," friend");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s doesn't like you any more.\n",full_name(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s makes you %s friend.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s makes you %s friend.\n",full_name(p),gstring_possessive(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void bar(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: bar <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;
	
  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," bar");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s allows you back into %s rooms.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s bars you from %s rooms.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s bars you from %s rooms.\n",full_name(p),gstring_possessive(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void invite(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: invite <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;
	
  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," invite");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s strikes you off %s invite list.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s enters you onto %s invite list.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s enters you onto %s invite list.\n",full_name(p),gstring_possessive(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void beep(player *p,char *str)
{
  char *oldstack;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: beep <player(s)> on/off/[blank]\n");
    return;
  }
	
  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," beep");
  stack=strchr(stack,0);
  *stack++=0;
  if (!strcmp("off",str)) {
      reset_list(p,oldstack);
      stack=oldstack;
      return;
  }
  if (!strcmp("on",str)) {
      set_list(p,oldstack);
      stack=oldstack;
      return;
  }
  toggle_list(p,oldstack);
  stack=oldstack;
}

void key(player *p,char *str)
{
  char *oldstack,*cmd;
  oldstack=stack;

  if (!*str) {
    tell_player(p,"Format: key <player(s)> on/off/[blank]\n");
    return;
  }

  if (!check_legal_entry(p,str,1)) return;

  while(*str && *str!=' ') *stack++=*str++;
  if (*str) str++;
  strcpy(stack," key");
  stack=strchr(stack,0);
  *stack++=0;
  cmd=stack;
  message_string=cmd;
  if (!strcmp("off",str)) {
    sprintf(stack,"%s snatches %s key back off of you.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    reset_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  if (!strcmp("on",str)) {
    sprintf(stack,"%s presents you with the key to %s rooms.\n",full_name(p),gstring_possessive(p));
    stack=end_string(stack);
    set_list(p,oldstack);
    stack=oldstack;
    message_string=0;
    return;
  }
  sprintf(stack,"%s presents you with the key to %s rooms.\n",full_name(p),gstring_possessive(p));
  stack=end_string(stack);
  toggle_list(p,oldstack);
  stack=oldstack;
  message_string=0;
}

void listfind(player *p,char *str)
{
    char *oldstack;
    oldstack=stack;

    if (!*str) {
	tell_player(p,"Format: find <player(s)> on/off/[blank]\n");
	return;
    }

    if (!check_legal_entry(p,str,1)) return;
	
    while(*str && *str!=' ') *stack++=*str++;
    if (*str) str++;
    strcpy(stack," find");
    stack=strchr(stack,0);
    *stack++=0;
    if (!strcmp("off",str)) {
	reset_list(p,oldstack);
	stack=oldstack;
	return;
    }
    if (!strcmp("on",str)) {
	sprintf(stack,"%s lets you see where %s is, even when %s is hidden.\n",
		p->name,gstring_possessive(p),gstring_possessive(p));
	stack=end_string(stack);
	tell_player(p,stack);
	set_list(p,oldstack);
	stack=oldstack;
	return;
    }
    toggle_list(p,oldstack);
    stack=oldstack;
}


/* toggle whether iacga's are sent or not */

void toggle_iacga(player *p,char *str)
{
  if (!strcmp("off",str)) {
    p->saved_flags &= ~IAC_GA_ON;
    p->flags &= ~IAC_GA_DO;
  }
  else if (!strcmp("on",str)) {
    p->saved_flags |= IAC_GA_ON;
    if (p->flags&EOR_ON) p->flags |= IAC_GA_DO;
  }
  else {
    p->saved_flags ^= IAC_GA_ON;
    p->flags ^= IAC_GA_DO;
    if (p->flags&EOR_ON) p->flags &= ~IAC_GA_DO;
  }
  
  if (p->saved_flags&IAC_GA_ON) {
    if (p->flags&IAC_GA_DO) 
      tell_player(p,"You will get IAC GA controls after each prompt.\n");
    else
      tell_player(p,"You will get IAC GA's on prompts if IAC EOR is unavailable.\n");
  }
  else
    tell_player(p,"You will not get IAC GA controls after each prompt.\n");
}


/* toggle earmuffs and blocktells on and off */

void earmuffs(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~BLOCK_SHOUT;
  else if (!strcmp("on",str)) p->saved_flags |= BLOCK_SHOUT;
  else p->saved_flags ^= BLOCK_SHOUT;
  
  if (p->saved_flags&BLOCK_SHOUT) 
    tell_player(p,"You are wearing earmuffs.\n");
  else
    tell_player(p,"You are not wearing earmuffs.\n");
}

/* toggle the pager on and off */

void nopager(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~NO_PAGER;
  else if (!strcmp("on",str)) p->saved_flags |= NO_PAGER;
  else p->saved_flags ^= NO_PAGER;
  if (p->saved_flags&NO_PAGER) 
    tell_player(p,"You will not get paged output.\n");
  else
    tell_player(p,"You will get paged output.\n");
}


void blocktells(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~BLOCK_TELLS;
  else if (!strcmp("on",str)) p->saved_flags |= BLOCK_TELLS;
  else p->saved_flags ^= BLOCK_TELLS;
  
  if (p->saved_flags&BLOCK_TELLS) 
    tell_player(p,"Tells to you will get blocked.\n");
  else
    tell_player(p,"Tells to you will not get blocked.\n");
  
}

/* toggle whether someone is hiding or not */


void hide(player *p,char *str)
{
    
  if (!strcmp("off",str)) p->saved_flags &= ~HIDING;
  else if (!strcmp("on",str)) p->saved_flags |= HIDING;
  else p->saved_flags ^= HIDING;
  
  if (p->saved_flags&HIDING) 
    tell_player(p,"You hide yourself away from prying eyes.\n");
  else
    tell_player(p,"People can find out where you are.\n");
}

/* toggle whether someone gets trans straight home */

void straight_home(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~TRANS_TO_HOME;
  else if (!strcmp("on",str)) p->saved_flags |= TRANS_TO_HOME;
  else p->saved_flags ^= TRANS_TO_HOME;
  
  if (p->saved_flags&TRANS_TO_HOME) 
      tell_player(p,"You will be taken to your home when you log in.\n");
  else
      tell_player(p,"You get taken to the entrance room when you log in.\n");
  strcpy(p->room_connect,"");
}

/* toggle see echo on and off */

void see_echo(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~SEEECHO;
  else if (!strcmp("on",str)) p->saved_flags |= SEEECHO;
  else p->saved_flags ^= SEEECHO;
  
  if (p->saved_flags&SEEECHO) 
    if (p->residency&(UPPER_SU|LOWER_ADMIN|ADMIN))
	tell_player(p,"You will now see who echos what.\n");
    else tell_player(p,"You will now see who echos what in public rooms.\n");
  else
    tell_player(p,"You will no longer see who echos what.\n");
}

/* toggle whether someone can receive anonymous mail */

void toggle_anonymous(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~NO_ANONYMOUS;
  else if (!strcmp("on",str)) p->saved_flags |= NO_ANONYMOUS;
  else p->saved_flags ^= NO_ANONYMOUS;
  
  if (p->saved_flags&NO_ANONYMOUS) 
    tell_player(p,"You will not be able to receive anonymous mail.\n");
  else
    tell_player(p,"You will be able to receive anonymous mail.\n");
  save_player(p);
}


/* toggle whether someone gets informed of news */

void toggle_news_inform(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~NEWS_INFORM;
  else if (!strcmp("on",str)) p->saved_flags |= NEWS_INFORM;
  else p->saved_flags ^= NEWS_INFORM;
  
  if (p->saved_flags&NEWS_INFORM) 
    tell_player(p,"You will be informed of new news.\n");
  else
    tell_player(p,"You will not be informed of new news.\n");
}


/* toggle whether someone gets informed of mail */

void toggle_mail_inform(player *p,char *str)
{
  if (!strcmp("off",str)) p->saved_flags &= ~MAIL_INFORM;
  else if (!strcmp("on",str)) p->saved_flags |= MAIL_INFORM;
  else p->saved_flags ^= MAIL_INFORM;
  
  if (p->saved_flags&MAIL_INFORM) 
    tell_player(p,"You will be informed when you receive mail.\n");
  else
    tell_player(p,"You will not be informed when you receive mail.\n");
}


/* see single entry */

void view_single_list(player *p,char *str)
{
    char *oldstack;
    list_ent *scan;
    int wibble=0;
    
    oldstack=stack;

    for (scan=p->saved->list_top;scan && !wibble;scan=scan->next) 
	if (!strcasecmp(scan->name,str)) {
	    sprintf(stack,"Your list entry for '%s'\n\n - Name -         - "
		    "Nsy. Ignr Infm Grab Frnd Bar. Invt Beep Bloc Key. "
		    "Find\n\n",scan->name);
	    stack=strchr(stack,0);
	    strcpy(stack,list_lines(scan));
	    wibble=1;
	}
    if (!wibble) {
	tell_player(p,"You dont have that name on your list ...\n");
	stack=oldstack;
	return;
    }
    stack=strchr(stack,0);
    *stack++='\n';
    *stack++=0;
    tell_player(p,oldstack);
    stack=oldstack;
}

/* view list */

void view_list(player *p,char *str)
{
  char *oldstack;
  list_ent *l,*every=0;
  int count;
  player *p2=0;

  oldstack=stack;
  if (*str && p->residency&ADMIN) {
      p2=find_player_global(str);
      if (p2 && !p2->saved) {
	tell_player(p,"They have no list to view.\n");
	return;
      }
  }
  if (!p2)
      p2=p;

  if (!p2->saved) {
    tell_player(p2,"You have no list to view.\n");
    return;
  }

  if (!p2 && *str) {
      view_single_list(p,str);
      return;
  }

  count=count_list(p2);
  sprintf(stack,"You are using %d of your maximum %d list entries.\n",
	  count,p2->max_list);
  stack=strchr(stack,0);
  if (count) {
      strcpy(stack,"\n - Name -         - Nsy. Ignr Infm Grab Frnd Bar. "
	     "Invt Beep Bloc Key. Find\n\n");
      stack=strchr(stack,0);
      for(l=p2->saved->list_top;l;l=l->next)
	  if (strcmp(l->name,"everyone")) {
	      strcpy(stack,list_lines(l));
	      stack=strchr(stack,0);
	      *stack++='\n';
	  } else
	      every=l;
      if (every) {
	  strcpy(stack,"---------------------------------------------"
		 "-----------------------------\n");
	  stack=strchr(stack,0);
	  strcpy(stack,list_lines(every));
	  stack=strchr(stack,0);
	  *stack++='\n';
      }
  }
  *stack++=0;
  pager(p,oldstack,0);
  stack=oldstack;
}

/* Print out flags for the list entries */

char *list_lines(list_ent *l)
{
    int count;
    static char string[80];
    string[0]=0;
    sprintf(string,"%-18s-",l->name);
    for(count=1;count<2048;count<<=1) {
	if (l->flags&count) strcat(string," YES!");
	else strcat(string,"     ");
    }
    return string;
}


/* view list of someone else */


void do_entry(player *p,player *p2)
{
  int count;
  list_ent *l;

  *stack++='\n';

  strcpy(stack,p2->name);
  for(count=MAX_NAME+1;*stack;count--) stack++;
  for(;count>3;count--) *stack++=' ';
  
  if (!p2->saved) {
    strcpy(stack,"    - No list");
    stack=strchr(stack,0);
    return;
  }
  
  l=find_list_entry(p2,p->lower_name);
  if (!l) {
    l=find_list_entry(p2,"everyone");
    if (!l) {
      strcpy(stack,"    - No entry for you");
      stack=strchr(stack,0);
      return;
    }
    *stack++='(';
    *stack++='E';
    *stack++=')';
    *stack++=' ';
  }
  else {
    *stack++=' ';
    *stack++=' ';
    *stack++=' ';
    *stack++=' ';
  }

  *stack++='-';
  for(count=1;count<2048;count<<=1) {
    if (l->flags&count) strcpy(stack," YES!");
    else strcpy(stack,"     ");
    stack=strchr(stack,0);
  }
}



void check_entry(player *p,char *str)
{
  char *oldstack,*text;
  player **list,**scan;
  int n,i;

  oldstack=stack;
  command_type=SEE_ERROR;
  
  if (!*str) {
    tell_player(p,"Format: check entry <player(list)>\n");
    return;
  }
  
  align(stack);
  list=(player **)stack;
  
  n=global_tag(p,str);
  if (!n) {
    stack=oldstack;
    return;
  }

  text=stack;

  strcpy(stack," - Name -             - Nsy. Ignr Infm Grab Frnd Bar."
	 " Invt Beep Bloc Key. Find");
  stack=strchr(stack,0);
  for(scan=list,i=0;i<n;i++,scan++) 
    if (*scan!=p) do_entry(p,*scan);
  *stack++='\n';
  *stack++=0;
  cleanup_tag(list,n);
  pager(p,text,0);
  stack=oldstack;
}


/*

void construct_list_save(saved_player *sp)
{
  list_ent *l,*next;

  l=sp->list_top;
  while(l) {
    next=l->next;
    if (!l->name[0]) {
      log("error","Bad list entry on save .. auto deleted.\n");
      delete_entry(sp,l);
    }
    else {
      stack=store_string(stack,l->name);
      stack=store_int(stack,l->flags);
    }
    l=next;
  }
  stack=store_string(stack,"");
}
*/

/* retrieve list */
/*
char *retrieve_list_data(saved_player *sp,char *where)
{
  list_ent *l;
  *stack=1;
  sp->list_top=0;
  while(*stack) {
    where=get_string(stack,where);
    if (*stack) {
      l=(list_ent *)MALLOC(sizeof(list_ent));
      strncpy(l->name,stack,MAX_NAME-2);
      where=get_int(&(l->flags),where);
      l->next=sp->list_top;
      sp->list_top=l;
    }
  }
  return where;
}
*/

/* see if a particular player can recieve a particular message */

int test_receive(player *p)
{
    int cerror=1;
    list_ent *l;
    char *oldstack;
    oldstack=stack;
    if (!current_player) return 1;
    if (p==current_player) return 1;
    if (current_player->residency&ADMIN && command_type&PERSONAL &&
	(!(sys_flags&EVERYONE_TAG))) return 1;

    if (command_type&EMERGENCY) return 1;
  
    if (command_type&PERSONAL) {
	l=find_list_entry(current_player,p->lower_name);
	if (!l) l=find_list_entry(current_player,"everyone");
	if (l && (l->flags&IGNORE || l->flags&BLOCK) && !(l->flags&NOISY)) {
	    if (command_type&SEE_ERROR) {
		sprintf(stack,"You can't get to %s because you "
			"are ignoring %s.\n",p->name,get_gender_string(p));
		stack=end_string(stack);
		tell_current(oldstack);
	    }
	    sys_flags |= FAILED_COMMAND;
	    stack=oldstack;
	    return 0;
	}	
    }

    l=find_list_entry(p,current_player->lower_name);
    if (!l) l=find_list_entry(p,"everyone");
  
    if (p->location != current_player->location) {
	if ((p->saved_flags&BLOCK_TELLS) && (!l || !(l->flags&NOISY)) &&
	    (command_type&PERSONAL)) {
	    if (command_type&SEE_ERROR) {
		sprintf(stack,"%s foils you with a blocktell.\n",p->name);
		stack=end_string(stack);
		tell_current(oldstack);
		stack=oldstack;
	    }
	    sys_flags |= FAILED_COMMAND;
	    return 0;
	}
	if (p->saved_flags&BLOCK_SHOUT && (command_type&EVERYONE || 
					   sys_flags&EVERYONE_TAG)
	    && (!l || !(l->flags&NOISY))) return 0;
    }
    if (!l) return 1;
  
    if (l->flags&BLOCK && !(l->flags&NOISY)
	&& command_type&PERSONAL) {
	if (command_type&SEE_ERROR) {
	    sprintf(stack,"You are blocked by %s.\n",p->name);
	    stack=end_string(stack);
	    tell_current(oldstack);
	}
	sys_flags |= FAILED_COMMAND;
	stack=oldstack;
	return 0;
    }
    
    if (sys_flags&ROOM_TAG || sys_flags&EVERYONE_TAG || sys_flags&FRIEND_TAG)
	cerror=0;
    
    if (l->flags&IGNORE && !(l->flags&NOISY)) {
	if (command_type&SEE_ERROR && cerror) {
	    if (strlen(p->ignore_msg)>0)
		sprintf(stack,"%s\n",p->ignore_msg);
	    else
		sprintf(stack,"You are ignored by %s.\n",p->name);
	    stack=end_string(stack);
	    tell_current(oldstack);
	}
	sys_flags |= FAILED_COMMAND;
	stack=oldstack;
	return 0;
    }
    return 1;
}




/* do inform bits */

void do_inform(player *p,char *msg)
{
  player *scan;
  char *oldstack;
  list_ent *l;

  oldstack=stack;

  if (!(p->location) || !(p->name[0])) return;

  for(scan=flatlist_start;scan;scan=scan->flat_next) 
    if (scan!=p && !(scan->flags&PANIC) && scan->name[0] && 
	scan->location) {
	l=find_list_entry(scan,p->lower_name);
	if (!l) {
	    l=find_list_entry(scan,"everyone");
	    command_type=LIST_EVERYONE;
	}
	if (l && l->flags&INFORM) {
	    command_type=0;
	    if (scan->residency&(LOWER_ADMIN|UPPER_SU|ADMIN))
		sprintf(stack,msg,p->name,p->inet_addr);
	    else
		sprintf(stack,msg,p->name,"");
	    stack=strchr(stack,0);
	    if (l->flags&BEEP) *stack++=7;
	    *stack++='\n';
	    *stack++=0;
	    if (l->flags&FRIEND) command_type=PERSONAL;
	    tell_player(scan,oldstack);
	    command_type=0;
	    stack=oldstack;	  
	}
    }
}


/* toggle what someone sees */
	  
void toggle_tags(player *p,char *str)
{
  char *oldstack;
  int change;
  
  oldstack=stack;
  if (!*str) {
    change=p->saved_flags&(TAG_ECHO|TAG_PERSONAL|TAG_ROOM|TAG_SHOUT|TAG_AUTOS);
    if (!change) tell_player(p,"You have no show flags enabled.\n");
    else {
      strcpy(stack,"You will get shown : ");
      stack=strchr(stack,0);
      if (change&TAG_ECHO) {
	strcpy(stack,"echos, ");
	stack=strchr(stack,0);
      }
      if (change&TAG_PERSONAL) {
	strcpy(stack,"tells to you, ");
	stack=strchr(stack,0);
      }
      if (change&TAG_ROOM) {
	strcpy(stack,"room commands, ");
	stack=strchr(stack,0);
      }
      if (change&TAG_SHOUT) {
	strcpy(stack,"shouts, ");
	stack=strchr(stack,0);
      }
      if (change&TAG_AUTOS) {
	strcpy(stack,"autos, ");
	stack=strchr(stack,0);
      }
      *stack=0;
      *(--stack)='\n';
      *(--stack)='.';
      stack=end_string(stack);
      tell_player(p,oldstack);
    }
   stack=oldstack;
    return;
  }
  change=get_flags(p,str,show_flags);
  if (!change) {
    tell_player(p,"Bad flag list, no changes made. \n"
		"Possible flags are echo,tell,room,shout,autos\n");
    return;
  }
  p->saved_flags ^= change;
  tell_player(p,"Show flags entries toggled.\n");
}

/* checks whether str is a person on the program (inc newbie)
   or if they are in the playerfiles  Returns their uppercase
   name
*/

char *check_legal_entry(player *p,char *str,int verbose)
{
    char *oldstack,*store;
    static char wibble[20];
    player *plyr;
    saved_player *splyr;
    oldstack=stack;
    store=str;
    while(*store) *store++=tolower(*store);
    if (store=strchr(str,' '))
	*store=0;
    if (!strcmp(str,"everyone")) {
	strcpy(wibble,"everyone");
	return wibble;
    }
    if (strlen(str)>20) {
	sprintf(stack,"Too long a name '%s'\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return 0;
    }
    if ((plyr=find_player_global_quiet(str))) {
	strcpy(wibble,plyr->name);
	return wibble;
    }
    if (!(splyr=find_saved_player(str))) {
	sprintf(stack,"There is no such person '%s'\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return 0;
    }
    if (!splyr) {
	log("error","Spooned again chris. check_legal_entry");
	return 0;
    }
    if (splyr->residency==STANDARD_ROOMS) {
	sprintf(stack,"Sorry, %s is a system name.\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return 0;
    }
    if (splyr->residency==BANISHED) {
	sprintf(stack,"Sorry, %s has been banished.\n",str);
	stack=end_string(stack);
	tell_player(p,oldstack);
	stack=oldstack;
	return 0;
    }
    strcpy(wibble,upper_from_saved(splyr));

    return wibble;
}

/* Look up a saved player, and return his/her uppercase name */

char *upper_from_saved(saved_player *sp)
{
    char *r;
    static char uname[20];
    r=sp->data.where;
    get_string(uname,r);
    return uname;
}


/*  Need some stuff to delete entries for newbies who have logged off
    and to wipe newbies from residents lists when they log off 
*/


/* This one is for when a resident logs off... all the newbies are stripped
   from their list
*/


void check_list_resident(player  *p)
{
    player *scan;
    list_ent *l;

	for (scan=flatlist_start;scan;scan=scan->flat_next) {
	    l=find_list_entry(p,scan->lower_name);
	    if (l && scan->residency==NON_RESIDENT)
		delete_entry(p->saved,l);
	}
}

/* 
  Now to strip all the entries of a newbie, when (s)he logs off
*/

void check_list_newbie(char *lowername)
{
    player *scan;
    list_ent *l;

    for (scan=flatlist_start;scan;scan=scan->flat_next) {
	l=find_list_entry(scan,lowername);
	if (l)
	    delete_entry(scan->saved,l);
    }
}

