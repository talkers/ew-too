S/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/

/* Ew-too moved message server */

/*  from time to time, for one reason or another, you will have to
   shut the server down or move it, for extended periods of time
   this little prog can be used to leave a message on the old
   socket */

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>



#include "config.h"
#include "player.h"


char *stack,*stack_start;
file message;
int main_descriptor;

/* return a string of the system time */

char *sys_time()
{
  time_t t;
  static char time_string[25];
  t=time(0);
  strftime(time_string,25,"%H:%M:%S - %d/%m/%y",localtime(&t));
  return time_string;
}

char *end_string(char *str)
{
  while(*str) str++;
  str++;
  return str;
}



/* log errors and things to file */

void log(char *file,char *string)
{
  int fd,length;

  sprintf(stack,"logs/%s.log",file);
  fd = open(stack,O_CREAT|O_WRONLY|O_SYNC,S_IRUSR|S_IWUSR);

  length=lseek(fd,0,SEEK_END);
  if (length>MAX_LOG_SIZE) {
    close(fd);
    fd=open(stack,O_CREAT|O_WRONLY|O_SYNC|O_TRUNC,S_IRUSR|S_IWUSR);
  }
  sprintf(stack,"%s - %s\n",sys_time(),string);
  printf(stack);
  write(fd,stack,strlen(stack));
  close(fd);
}


void error(char *str)
{
  log("mms",str);
  exit(-1);
}

/* load a file into memory */

file load_file(char *filename)
{
  file f;
  int d,e;
  char *oldstack;

  oldstack=stack;

  d=open(filename,O_RDONLY);
  if (d<0) {
    sprintf(oldstack,"Can't find file:%s",filename);
    stack=end_string(oldstack);
    log("mms",oldstack);
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
    log("mms",oldstack);
    f.where=(char *)MALLOC(1);
    *(char *)f.where=0;
    f.length=0;
    stack=oldstack;
    return f;
  }
  close(d);
  stack=oldstack;
  *(f.where+f.length)=0;
  return f;
}



void sigpipe() { error("Sigpipe received."); }
void sighup() { error("Hangup received."); }
void sigquit() { error("Quit signal received."); }
void sigill() { error("Illegal instruction."); }
void sigfpe() { error("Floating Point Error."); }
void sigbus() { error("Bus Error."); }
void sigsegv() { error("Segmentation Violation."); }
void sigsys() { error("Bad system call."); }
void sigterm() { error("Terminate signal received."); }
void sigxfsz() { error("File descriptor limit exceeded."); }

void init_socket(int port)
{
  struct sockaddr_in main_socket;
  int dummy=1;
  char *oldstack;

  oldstack=stack;
  
/* grab the main socket */
  
  main_descriptor=socket(PF_INET,SOCK_STREAM,0);
  if (main_descriptor<0) error("Can't grab a socket.");
  if (setsockopt(main_descriptor,SOL_SOCKET,SO_REUSEADDR,&dummy,sizeof(dummy))<0)
    error("Bad socket operation.");
  
  main_socket.sin_family=AF_INET;
  main_socket.sin_port=htons(port);
  main_socket.sin_addr.s_addr= INADDR_ANY;
  
  if (bind(main_descriptor,&main_socket,sizeof(main_socket))<0)
    error("Can't bind socket.");

  if (listen(main_descriptor,5)<0) error("Listen refused");
  
  sprintf(oldstack,"Socket bound and listening on port %d",port);
  stack=end_string(oldstack);
  log("mms",oldstack);

  stack=oldstack;
}




main(int argc,char *argv[])
{
  int status,new_socket,count=0,length,port;
  struct sockaddr_in incoming;
  stack_start=(char *)malloc(1000);
  stack=stack_start;
  
  if (chdir(ROOT)<0) error("Can't change to root directory.\n");
  
  if (*(argv[0])!='-') {
    if (!argv[1]) {
      sprintf(stack,"%d",DEFAULT_PORT);
      execlp("bin/mmserver","-=> EW-too <=- MMS counted (00000) on",stack,0);
    }
    else {
      argv[0]="-=> EW-too <=- MMS counted (00000) on";
      execvp("bin/mmserver",argv);
    }
    error("exec failed");
  }
  
  if (nice(5)<0) error("Failed to renice");
  
  signal(SIGPIPE,sigpipe);
  signal(SIGHUP,sighup);
  signal(SIGQUIT,sigquit);
  signal(SIGILL,sigill);
  signal(SIGFPE,sigfpe);
  signal(SIGBUS,sigbus);
  signal(SIGSEGV,sigsegv);
  signal(SIGSYS,sigsys);
  signal(SIGTERM,sigterm);
  signal(SIGXFSZ,sigxfsz);
  
  if (argc==2) port=atoi(argv[1]);
  else port=DEFAULT_PORT;

  message=load_file("files/moved.msg");

  init_socket(port);

  length=sizeof(incoming);

  while(1) {
    new_socket=accept(main_descriptor,&incoming,&length);
    write(new_socket,message.where,message.length);
    close(new_socket);
    count++;
    sprintf(argv[0],"-=> EW-too <=- MMS counted (%5d) on",count);
  }

}
