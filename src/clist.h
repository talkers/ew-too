/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/*
  clist.h
*/

extern void dynamic_defrag_rooms(),dynamic_validate_rooms();
extern void dynamic_test_func_blocks(),dynamic_test_func_keys();
extern void dynamic_dfstats();
extern char *check_legal_entry(player *,char *,int);
extern char *list_lines(list_ent *);
extern void to_room1(),to_room2(),room_link();
extern void say(),quit(),pulldown(),change_password(),change_email(),do_save(),
    wall(),tell(),grant(),remove(),nuke_player(),view_saved_lists(),
    banish_player(),sync_files(),
    restore_files(),check(),make_new_character(),room_command(),
    change_room_limit(),test_fn(),unlink_from_room(),look(),trans_to(),
    validate_email(),go_room(),remote(),recho(),whisper(),emote(),echo(),
    gender(),shout(),exclude(),change_exit_limit(),change_auto_limit(),
    remove_shout(),set_title(),set_description(),set_plan(),set_prompt(),
    set_converse_prompt(),set_term_width(),set_word_wrap(),blank_password(),
    pemote(),premote(),converse_mode_on(),converse_mode_off(),swho(),
    set_pretitle(),clear_list(),change_list_absolute(),change_list_limit(),
    set_list(),reset_list(),toggle_list(),noisy(),ignore(),inform(),grab(),
    friend(),bar(),invite(),key(),listfind(),set_enter_msg(),set_ignore_msg(),
    view_commands(),qwho(),beep(),blocktells(),earmuffs(),move_to(),
    change_room_entermsg(),hide(),check_idle(),set_idle_msg(),view_ip(),
    view_player_email(),public_com(),do_grab(),examine(),go_quiet(),
    view_time(),toggle_tags(),straight_home(),close_to_newbies(),su(),
    suemote(),emergency(),finger(),see_echo(),dump_com(),news_command(),
    mail_command(),list_notes(),list_all_notes(),change_mail_limit(),
    toggle_news_inform(),toggle_mail_inform(),recap(),soft_eject(),
    show_malloc(),help(),reload(),hitells(),block(),privs(),boot_out(),
    wake(),view_note(),dest_note(),squish(),resident(),join(),crash(),port(),
    motd(),trace(),trans_fn(),remove_move(),relink_note(),add_lag(),
    toggle_iacga(),recount_news(),banish_edit(),same_site(),set_age(),
    set_birthday(),warn(),netstat(),nopager(),bounce(),vlog(),reset_squish(),
    splat_player(),block_su(),unsplat(),invites_list(),rename_player(),
    quiet_rename(),mindseye(),think(),set_session(),set_comment(),
    view_session(),super_help(),set_yes_session(),reply(),prison_player(),
    twho(),reset_session(),set_login_room(),room_entry(),assist_player(),
    on_duty(),barge(),report_error(),clear_screen(),confirm_password(),
    inform_room_enter(),show_exits(),blank_email(),view_banished(),
    test_name(),leave_command(),entrance();

extern void edit_quit(),edit_end(),edit_wipe(),edit_view(),edit_view_line(),
  edit_back_line(),edit_forward_line(),edit_view_commands(),edit_goto_top(),
  edit_goto_bottom(),edit_stats(),edit_delete_line(),edit_goto_line(),
  toggle_quiet_edit(),change_auto_base();

extern void check_wrap(),check_updates(),check_email(),check_rooms(),check_exits(),
  view_list(),view_check_commands(),check_info(),check_room(),check_entry(),
  check_main();

extern void exit_room_mode(),create_new_room(),change_room_id(),change_room_name(),
  delete_room(),add_exit(),remove_exit(),add_auto(),remove_auto(),check_autos(),
  autos_com(),view_room_commands(),view_room_key_commands(),who(),where(),room_lock(),room_bolt(),room_lockable(),
  room_open(),set_home(),go_home(),visit(),hilltop(),room_edit(),with(),grabable(),
  transfer_room(),room_linkable();

extern void view_news_commands(),exit_news_mode(),list_news(),post_news(),
  followup(),remove_article(),read_article(),reply_article();

extern void view_mail_commands(),exit_mail_mode(),view_sent(),view_received(),
  send_letter(),read_letter(),reply_letter(),delete_received(),delete_sent(),
  toggle_anonymous(),lsu(),lnew(),ignoreprefix(),set_time_delay();

extern void player_flags_verbose();

#ifdef PC
  extern void psuedo_person(),switch_person();
#endif

/* dummy commands for stack checks */

struct command input_to = { "input_to fn",0,0,0,0 };
struct command timer = { "timer fn",0,0,0,0 };

/* command list for editor */

struct command editor_list[]={
  {"del",edit_delete_line,0,1,0},
  {"-",edit_back_line,0,1,0},
  {"+",edit_forward_line,0,1,0},
  {"view",edit_view,0,1,0},
  {"l",edit_view_line,0,1,0},
  {"g",edit_goto_line,0,1,0},
  {"top",edit_goto_top,0,1,0},
  {"bot",edit_goto_bottom,0,1,0},
  {"end",edit_end,0,1,0},
  {"quit",edit_quit,0,1,0},
  {"wipe",edit_wipe,0,1,0},
  {"stats",edit_stats,0,1,0},
  {"quiet",toggle_quiet_edit,0,1,0},
  {"commands",edit_view_commands,0,1,0},
  {"?",help,0,0,0},
  {"help",help,0,1,0},
  {0,0,0,0,0} 
};

/* command list for the room function */

struct command keyroom_list[]={
    {"check",check_rooms,BUILD,1,0},
    {"commands",view_room_key_commands,BUILD,1,0},
    {"end",exit_room_mode,BUILD,1,0},
    {"entermsg",change_room_entermsg,BUILD,1,0},
    {"exits",check_exits,BUILD,1,0},
    {"+exit",add_exit,BUILD,1,0},
    {"-exit",remove_exit,BUILD,1,0},
    {"go",go_room,BUILD,1,0},
    {"?",help,0,0,0},
    {"help",help,0,0,0},
    {"info",check_room,BUILD,1,0},
    {"lock",room_lock,BUILD,1,0},
    {"lockable",room_lockable,BUILD,1,0},
    {"look",look,BUILD,1,0},
    {"linkable",room_linkable,BUILD,1,0},
    {"name",change_room_name,BUILD,1,0},
    {"open",room_open,BUILD,1,0},
    {"trans",trans_fn,BUILD,1,0},
    {0,0,0,0,0}
};



struct command room_list[]={
    {"bolt",room_bolt,BUILD,1,0},
    {"edit",room_edit,BUILD,1,0},
    {"sethome",set_home,BUILD,1,0},
    {"lock",room_lock,BUILD,1,0},
    {"lockable",room_lockable,BUILD,1,0},
    {"linkable",room_linkable,BUILD,1,0},
    {"open",room_open,BUILD,1,0},
    {"entrance",room_entry,BUILD,1,0},
    {"entermsg",change_room_entermsg,BUILD,1,0},
    {"+exit",add_exit,BUILD,1,0},
    {"-exit",remove_exit,BUILD,1,0},
    {"link",room_link,BUILD,1,0},
    {"exits",check_exits,BUILD,1,0},
    {"id",change_room_id,BUILD,1,0},
    {"name",change_room_name,BUILD,1,0},
    {"notify",inform_room_enter,BUILD,1,0},
    {"end",exit_room_mode,BUILD,1,0},
    {"info",check_room,BUILD,1,0},
    {"check",check_rooms,BUILD,1,0},
    {"+auto",add_auto,BUILD,1,0},
    {"-auto",remove_auto,BUILD,1,0},
    {"speed",change_auto_base,BUILD,1,0},
    {"autos",autos_com,BUILD,1,0},
    {"delete",delete_room,BUILD,1,0},
    {"create",create_new_room,BUILD,1,0},
    {"go",go_room,BUILD,1,0},
    {"look",look,BUILD,1,0},
    {"trans",trans_fn,BUILD,1,0},
    {"home",go_home,BUILD,1,0},
    {"commands",view_room_commands,BUILD,1,0},
    {"?",help,0,0,0},
    {"help",help,0,1,0},
    {"transfer",transfer_room,ADMIN,1,0},
    {0,0,0,0,0} 
};


/* command list for the check function */

struct command check_list[]={
  {"main",check_main,0,1,0},
  {"flags",player_flags_verbose,0,1,0},
  {"mail",view_received,MAIL,1,0},
  {"sent",view_sent,MAIL,1,0},
  {"news",list_news,0,1,0},
  {"exits",check_exits,0,1,0},
  {"entry",check_entry,0,1,0},
  {"list",view_list,LIST,1,0},
  {"autos",check_autos,BUILD,1,0},
  {"room",check_room,0,1,0},
  {"rooms",check_rooms,BUILD,1,0},
  {"email",check_email,0,1,0},
  {"wrap",check_wrap,0,1,0},
  {"res_list",view_saved_lists,ADMIN,1,0},
  {"updates",check_updates,(LOWER_ADMIN|ADMIN),1,0},
  {"info",check_info,(LOWER_ADMIN|ADMIN),1,0},
  {"commands",view_check_commands,0,1,0},
  {"ip",view_ip,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"mails",view_player_email,ADMIN,1,0},
  {"?",help,0,0,0},
  {"help",help,0,1,0},
  {0,0,0,0,0} };

/* command list for the news sub command */

struct command news_list[]={
  {"check",list_news,0,1,0},
  {"read",read_article,0,1,0},
  {"view",list_news,0,1,0},
  {"reply",reply_article,MAIL,1,0},
  {"areply",reply_article,MAIL,1,0},
  {"post",post_news,MAIL,1,0},
  {"apost",post_news,MAIL,1,0},
  {"followup",followup,MAIL,1,0},
  {"afollowup",followup,MAIL,1,0},
  {"remove",remove_article,MAIL,1,0},
  {"commands",view_news_commands,0,1,0},
  {"inform",toggle_news_inform,0,1,0},
  {"end",exit_news_mode,0,1,0},
  {"?",help,0,0,0},
  {"help",help,0,1,0},
  {0,0,0,0,0} };

/* command list for the mail sub command */

struct command mail_list[]={
  {"check",view_received,0,1,0},
  {"read",read_letter,MAIL,1,0},
  {"post",send_letter,MAIL,1,0},
  {"apost",send_letter,MAIL,1,0},
  {"reply",reply_letter,MAIL,1,0},
  {"areply",reply_letter,MAIL,1,0},
  {"delete",delete_received,MAIL,1,0},
  {"remove",delete_sent,MAIL,1,0},
  {"commands",view_mail_commands,MAIL,1,0},
  {"end",exit_mail_mode,MAIL,1,0},
  {"sent",view_sent,MAIL,1,0},
  {"view",view_received,MAIL,1,0},
  {"inform",toggle_mail_inform,0,1,0},
  {"noanon",toggle_anonymous,0,1,0},
  {"?",help,0,0,0},
  {"help",help,0,1,0},
  {0,0,0,0,0} };


/* restricted command list for naughty peoples */

struct command restricted_list[]={
  {"'",say,0,0,0},
  {"`",say,0,0,0},
  {"\"",say,0,0,0},
  {"::",pemote,0,0,0},
  {":;",pemote,0,0,0},
  {";;",pemote,0,0,0},
  {";:",pemote,0,0,0},
  {";",emote,0,0,0},
  {":",emote,0,0,0},
  {"=",whisper,0,0,0},
  {"emote",emote,0,1,0},
  {"say",say,0,1,0},
  {"pemote",pemote,0,1,0},
  {"whisper",whisper,0,1,0},
  {"look",look,0,1,0},
  {"l",look,0,1,0},
  {"?",help,0,1,0},
  {"help",help,0,1,0},
  {0,0,0,0,0} };

/* this is the main command list */

struct command complete_list[]={               /* non alphabetic */
  {"'",say,0,0,0},
  {"`",say,0,0,0},
  {"\"",say,0,0,0},
  {"::",pemote,0,0,0},
  {":;",pemote,0,0,0},
  {";;",pemote,0,0,0},
  {";:",pemote,0,0,0},
  {";",emote,0,0,0},
  {":",emote,0,0,0},
  {">",tell,0,0,0},
  {"<:",premote,0,0,0},
  {"<;",premote,0,0,0},
  {"<",remote,0,0,0},
  {"=",whisper,0,0,0},
  {"!",shout,0,0,0},
  {"?",help,0,0,0},
  {"~",think,0,0,0},
  {0,0,0,0,0},

  {"age",set_age,0,1,0},                             /* A */ 
  {"assist",assist_player,OFF_DUTY,1,0},
  {0,0,0,0,0},

  {"bar",bar,LIST,1,0},                              /* B */
  {"barge",barge,ADMIN,1,0},
  {"beep",beep,LIST,1,0},
  {"block",block,LIST,1,0},
  {"boot",boot_out,BUILD,1,0},
  {"birthday",set_birthday,0,1,0},
  {"blocktells",blocktells,0,1,0},
  {"bounce",bounce,0,1,0},
  {"banish",banish_player,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0}, 
  {"blankpass",blank_password,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"blank_email",blank_email,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"bedit",banish_edit,(LOWER_ADMIN|ADMIN),1,0},
  {"bug",report_error,OFF_DUTY,1,0},
  {0,0,0,0,0},
    
  {"check",check,0,1,0},                           /* C */ 
  {"cls",clear_screen,0,1,0},
  {"clist",clear_list,LIST,1,0},
  {"comment",set_comment,0,1,0},
  {"confirm",confirm_password,OFF_DUTY,1,0},
  {"connect_room",set_login_room,BASE,1,0},
  {"converse",converse_mode_on,0,1,0},
  {"cprompt",set_converse_prompt,0,1,0},
  {"commands",view_commands,0,1,0},
  {"change_mail_limit",change_mail_limit,ADMIN,1,0},
  {"change_room_limit",change_room_limit,ADMIN,1,0},
  {"change_exit_limit",change_exit_limit,ADMIN,1,0},
  {"change_auto_limit",change_auto_limit,ADMIN,1,0},
  {"change_list_limit",change_list_limit,ADMIN,1,0},
  {"crash",crash,ADMIN,1,0},
  {0,0,0,0,0},

  {"description",set_description,0,1,0},           /* D */
  {"dump",dump_com,(LOWER_ADMIN|ADMIN),1,0},
  {"dtb",dynamic_test_func_blocks,(ADMIN),1,0},
  {"dtk",dynamic_test_func_keys,(ADMIN),1,0},
  {"dfstats",dynamic_dfstats,(ADMIN),1,0},
  {"defrag",dynamic_defrag_rooms,(ADMIN),1,0},
  {0,0,0,0,0},

  {"emote",emote,0,1,0},                           /* E */
  {"echo",echo,ECHO_PRIV,1,0},
  {"exits",check_exits,0,1,0},
  {"exclude",exclude,0,1,0},
  {"examine",examine,0,1,0},
  {"exam",examine,0,1,0},
  {"ex",examine,0,1,0},
  {"earmuffs",earmuffs,0,1,0},
  {"entermsg",set_enter_msg,0,1,0},
  {"end",converse_mode_off,0,1,0},
  {"email",change_email,0,1,0},
  {"entrance",entrance,0,1,0},
  {"emergency",emergency,0,1,0},
  {"eject",soft_eject,LOWER_SU,1,0},
  {0,0,0,0,0},

  {"friend",friend,LIST,1,0},                      /* F */
  {"fwho",qwho,0,1,0},
  {"find",listfind,LIST,1,0},
  {"finger",finger,0,1,0},
  {"flist",change_list_absolute,LIST,1,0},
  {0,0,0,0,0},

  {"go",go_room,0,1,0},                            /* G */ 
  {"grab",do_grab,0,1,0},
  {"grabme",grab,LIST,1,0},
  {"grabable",grabable,0,1,0},
  {"gender",gender,0,1,0},
  {"ghome",straight_home,BUILD,1,0},
  {"grant",grant,ADMIN,1,0},  
  {"goto",trans_to,ADMIN,1,0},
  {0,0,0,0,0},

  {"help",help,0,1,0},                              /* H */ 
  {"home",go_home,BUILD,1,0},
  {"hitells",hitells,0,1,0},
  {"hide",hide,0,1,0},
  {0,0,0,0,0},

  {"idle",check_idle,0,1,0},                        /* I */ 
  {"idlemsg",set_idle_msg,0,1,0},
  {"ignore",ignore,LIST,1,0},
  {"ignoremsg",set_ignore_msg,LIST,1,0},
  {"inform",inform,LIST,1,0},
  {"invite",invite,LIST,1,0},
  {"invites",invites_list,0,1,0},
  {"iacga",toggle_iacga,0,1,0},
  {0,0,0,0,0},
  
  {"jail",prison_player,(LOWER_ADMIN|ADMIN),1,0},
  {"jetlag",set_time_delay,0,1,0},
  {"join",join,0,1,0},                             /* J */ 
  {0,0,0,0,0},        
  
  {"key",key,LIST,1,0},
  {0,0,0,0,0},                                     /* K */ 
  
  {"look",look,0,1,0},                             /* L */ 
  {"list",view_list,LIST,1,0},
  {"leave",leave_command,0,1,0},
  {"lock",room_lock,0,1,0},
  {"linewrap",set_term_width,0,1,0},
  {"list_res",view_saved_lists,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"list_su",lsu,0,1,0},
  {"list_new",lnew,OFF_DUTY,1,0},
  {"list_notes",list_notes,(LOWER_ADMIN|ADMIN),1,0},
  {"list_all_notes",list_all_notes,(LOWER_ADMIN|ADMIN),1,0},  
  {"lag",add_lag,(LOWER_ADMIN|ADMIN),1,0},
  {"l",look,0,1,0},
  {0,0,0,0,0},
  
  {"mail",mail_command,MAIL,1,0},  		 /* M */ 
  {"motd",motd,0,1,0},
  {"make",make_new_character,(LOWER_ADMIN|ADMIN),1,0},
  {"malloc",show_malloc,(LOWER_ADMIN|ADMIN),1,0},
  {"mindseye",mindseye,BUILD,1,0},
  {0,0,0,0,0},

  {"news",news_command,0,1,0}, 	                /* N */
  {"noisy",noisy,LIST,1,0},   
  {"nopager",nopager,0,1,0},
  {"noprefix",ignoreprefix,0,1,0},
  {"newbies",close_to_newbies,OFF_DUTY,1,0},
  {"netstat",netstat,(LOWER_ADMIN|ADMIN),1,0},
  {"nuke",nuke_player,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {0,0,0,0,0},
  
  {"off_duty",block_su,OFF_DUTY,1,0},
  {"on_duty",on_duty,OFF_DUTY,1,0},
  {0,0,0,0,0},                                     /* O */ 
  
  {"password",change_password,0,1,0},              /* P */ 
  {"prefix",set_pretitle,BASE,1,0},
  {"pemote",pemote,0,1,0},
  {"premote",premote,0,1,0},
  {"plan",set_plan,BASE,1,0},
  {"prompt",set_prompt,0,1,0},
  {"privs",privs,BASE,1,0},
  {"public",public_com,0,1,0},
#ifdef PC
  {"pseudo",psuedo_person,0,1,0},
#endif
  {"port",port,ADMIN,1,0},
  {0,0,0,0,0},
  
  {"quit",quit,0,1,0},                             /* Q */ 
  {"qwho",qwho,0,1,0},
  {"quiet",go_quiet,0,1,0},
  {0,0,0,0,0},

  {"recap",recap,0,1,0},                           /* R */
  {"remote",remote,0,1,0},
  {"reply",reply,0,1,0},
  {"recho",recho,ECHO_PRIV,1,0},
  {"reset_squish",reset_squish,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"reset_session",reset_session,OFF_DUTY,1,0},
  {"room",room_command,BUILD,1,0},
  {"rlist",reset_list,LIST,1,0},
  {"resident",resident,OFF_DUTY,1,0},
  {"remove",remove,ADMIN,1,0},    
  {"remove_shout",remove_shout,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"remove_move",remove_move,(LOWER_ADMIN|ADMIN),1,0},
  {"remove_note",dest_note,(LOWER_ADMIN|ADMIN),1,0},
  {"rename",rename_player,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"restore",restore_files,(LOWER_ADMIN|ADMIN),1,0},
  {"reload",reload,(LOWER_ADMIN|ADMIN),1,0},
  {"relink",relink_note,(LOWER_ADMIN|ADMIN),1,0},
  {"recount",recount_news,(LOWER_ADMIN|ADMIN),1,0},
  {0,0,0,0,0},

  {"say",say,0,1,0},                               /* S */ 
  {"shout",shout,0,1,0},
  {"save",do_save,BASE,1,0},
  {"seesess",view_session,0,1,0},
  {"seetitle",set_yes_session,0,1,0},
  {"session",set_session,SESSION,1,0},
  {"se",suemote,OFF_DUTY,1,0},
  {"swho",swho,0,1,0},
  {"shelp",super_help,OFF_DUTY,1,0},
  {"show",toggle_tags,0,1,0},
  {"slist",set_list,LIST,1,0},
  {"seeecho",see_echo,0,1,0},
  {"su",su,OFF_DUTY,1,0},
  {"su:",suemote,OFF_DUTY,1,0},
  {"splat",splat_player,OFF_DUTY,1,0},
  {"squish",squish,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"site",same_site,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {"showexits",show_exits,BASE,1,0},
  {"shutdown",pulldown,(LOWER_ADMIN|ADMIN),1,0},
  {"sync",sync_files,(LOWER_ADMIN|ADMIN),1,0},
  
#ifdef PC
  {"switch",switch_person,0,1,0},
#endif
  {0,0,0,0,0},

  {"tell",tell,0,1,0},                             /* T */ 
  {"test",test_name,OFF_DUTY,1,0},
  {"think",think,0,1,0},
  {"tlist",toggle_list,LIST,1,0},
  {"title",set_title,0,1,0},
  {"time",view_time,0,1,0},
  {"trans",trans_fn,0,1,0},
  {"trace",trace,(LOWER_ADMIN|ADMIN),1,0},
  {"twho",twho,0,1,0},
  {0,0,0,0,0},

/*  {"unlink",unlink_from_room,ADMIN,1,0},            U     */
  {"unsplat",unsplat,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0},
  {0,0,0,0,0},            

  {"visit",visit,0,1,0},                           /* V */
/*  {"validate_email",validate_email,(UPPER_SU|LOWER_ADMIN|ADMIN),1,0}, */
/*  {"view_banished",view_banished,OFF_DUTY,1,0}, */
  {"view_note",view_note,(LOWER_ADMIN|ADMIN),1,0},
  {"vlog",vlog,(LOWER_ADMIN|ADMIN),1,0},
  {"validate_rooms",dynamic_validate_rooms,ADMIN,1,0},
  {0,0,0,0,0},

  {"who",who,0,1,0},                               /* W */ 
  {"whisper",whisper,0,1,0},
  {"where",where,0,1,0},
  {"with",with,0,1,0},
  {"wake",wake,0,1,0},
  {"wordwrap",set_word_wrap,0,1,0},
  {"warn",warn,OFF_DUTY,1,0},
  {"wall",wall,(LOWER_ADMIN|ADMIN),1,0},  
  {0,0,0,0,0},

  {"x",examine,0,1,0},                             /* X */
  {0,0,0,0,0},

  {0,0,0,0,0},                                     /* Y */ 

  {0,0,0,0,0}                                      /* Z */ 
};

struct command *coms[27];
