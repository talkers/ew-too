/*************************************************************************/
/*     EW-too                                     (c) Simon Marsh 1994   */
/*************************************************************************/
/*
/* Ew-too guardian angel  ( the newer one)
/*
/* The job of the guardian angel is to keep the server up and running.
/* If the server disappears (ie it crashes) then the angel will log why
/* and try and boot another one.
*/

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#if !defined(MIPS) && !defined(LINUX)
#include <sys/filio.h>
#endif

#ifndef S_IRUSR
#define S_IRUSR 00400
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200
#endif

#ifdef MIPS  		/* dumb MIPS */
extern int errno;
#endif


char *angel_name="-=> EW-too <=- Guardian Angel watching port";
char *server_name="-=> EW-too <=- Server on port";

char *stack,*stack_start;
int ewtoo=0,die=0,crashes=0,syncing=0;
long int time_out=0,t=0;

/* return a string of the system time */

char *sys_time()
{
    time_t t;
    static char time_string[25];
    t=time(0);
    strftime(time_string,25,"%H:%M:%S - %d/%m/%y",localtime(&t));
    return time_string;
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
	     log("angel",str);
	     exit(-1);
	   }

	   void sigpipe() { error("Sigpipe received."); }
	   void sighup() { kill(ewtoo,SIGHUP); die=1; }
	   void sigquit() { error("Quit signal received."); }
	   void sigill() { error("Illegal instruction."); }
	   void sigfpe() { error("Floating Point Error."); }
	   void sigbus() { error("Bus Error."); }
	   void sigsegv() { error("Segmentation Violation."); }
	   void sigsys() { error("Bad system call."); }
	   void sigterm() { error("Terminate signal received."); }
	   void sigxfsz() { error("File descriptor limit exceeded."); }
	   void sigchld() { log("angel","Received SIGCHLD"); return; }



main(int argc,char *argv[])
{
    int status;
    int length,alive_fd,sock_fd,dieing;
    struct sockaddr_un sa;
    char dummy;
    fd_set fds;
    struct timeval timeout;


    stack_start=(char *)malloc(1000);
    stack=stack_start;

    if (chdir(ROOT)<0) error("Can't change to root directory.\n");

    if (strcmp(angel_name,argv[0])) {
	if (!argv[1]) {
	    sprintf(stack,"%d",DEFAULT_PORT);
	    execlp("bin/angel",angel_name,stack,0);
	}
	else {
	    argv[0]=angel_name;
	    execvp("bin/angel",argv);
	}
	error("exec failed");
    }

    if (nice(5)<0) error("Failed to renice");

    t=time(0);
    time_out=t+60;

    signal(SIGPIPE,sigpipe);
    signal(SIGHUP,sighup);
    signal(SIGQUIT,sigquit);
    signal(SIGILL,sigill);
    signal(SIGFPE,sigfpe);
    signal(SIGBUS,sigbus);
    signal(SIGSEGV,sigsegv);
#ifndef LINUX
    signal(SIGSYS,sigsys);
#endif    
    signal(SIGTERM,sigterm);
    signal(SIGXFSZ,sigxfsz);
    signal(SIGCHLD,sigchld);

    while(!die) {
	t=time(0);
	if (crashes>=4 && time_out>=t) {
	    log("error","Total barf.. crashing lots... Giving up");
	    log("error","Question is, what now ??");
	    exit(-1);
	}
	else if (time_out<t) {
	    time_out=t+45;
	    crashes=0;
	}
	crashes++;
	log("angel","Forking to boot server");
	dieing=0;
	ewtoo=fork();
	switch(ewtoo) {
	case 0:
#ifndef MIPS
	    setsid();
#endif
	    argv[0]=server_name;
	    execvp("bin/ew-too",argv);
	    error("failed to exec server");
	    break;
	case -1:
	    error("Failed to fork()");
	    break;
	default:

	    unlink(SOCKET_PATH);

	    sock_fd=socket(PF_UNIX,SOCK_STREAM,0);
	    if (sock_fd<0) error("failed to create socket");

	    sa.sun_family=AF_UNIX;
	    strcpy(sa.sun_path,SOCKET_PATH);

	    if (bind(sock_fd,(struct sockaddr *)&sa,sizeof(sa))<0) 
		error("failed to bind");

	    if (listen(sock_fd,1)<0) error("failed to listen");

	    timeout.tv_sec=120;
	    timeout.tv_usec=0;
	    FD_ZERO(&fds);
	    FD_SET(sock_fd,&fds);
	    if (select(FD_SETSIZE,&fds,0,0,&timeout)<=0) {
		kill(ewtoo,SIGKILL);
		log("angel","Killed server before connect");
#ifdef MIPS
                wait(&status);
#else
		waitpid(ewtoo,&status,0);
#endif
	    }
	    else {

		length=sizeof(sa);
		alive_fd=accept(sock_fd,(struct sockaddr *)&sa,&length);
		if (alive_fd<0) error("bad accept");
		close(sock_fd);

#ifdef MIPS
		while(wait3(&status,WNOHANG,0)<=0) {
#else
		while(waitpid(ewtoo,&status,WNOHANG)<=0) {
#endif
		    timeout.tv_sec=300;
		    timeout.tv_usec=0;
		    FD_ZERO(&fds);
		    FD_SET(alive_fd,&fds);

		    if (select(FD_SETSIZE,&fds,0,0,&timeout)<=0) {
			if (errno!=EINTR) {
			    if (dieing) {
				kill(ewtoo,SIGKILL);
				log("angel","Server KILLED");
			    }
			    else {
				kill(ewtoo,SIGTERM);
				log("angel","Server TERMINATED");
				dieing=1;
			    }
			}
		    }
		    else {
			if (ioctl(alive_fd,FIONREAD,&length)<0) 
			    error("bad FIONREAD");
			if (!length) {
			    kill(ewtoo,SIGKILL);
			    log("angel","Server disconnected");
			    dieing=1;
			}
			else for(;length;length--) read(alive_fd,&dummy,1);
	    
		    }
		}
	    }
      
	    close(alive_fd);

	    switch((status&255)) {
	    case 0:
		log("angel","Server exited safely");
		break;
	    case 127:
		sprintf(stack,"Server stopped due to signal %d.",(status>>8)&255);
		while(*stack) stack++;
		stack++;
		log("angel",stack_start);
		stack=stack_start;
		break;
	    default:
		sprintf(stack,"Server terminated due to signal %d.",status&127);
		while(*stack) stack++;
		stack++;
		log("angel",stack_start);
		stack=stack_start;
		if (status&128) log("angel","Core dump produced");
		break;
	    }
	    break;
	}
    }
}
