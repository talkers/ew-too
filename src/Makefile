#/*************************************************************************/
#/*     EW-too                                     (c) Simon Marsh 1994   */
#/*************************************************************************/
#
# The build produces two binaries 'swap' and 'aswap'
#
# These are intended to be renamed to 'ew-too' and 'angel' respectively
# at some opportune moment. (ie while the server is down, you can really
# screw yourself if you try swapping the binaries while the server is
# still running)
#
# If you aren't using GNU make and GCC, then good luck ...
#

BIN 			= ../bin
EW2BINARY		= swap
ABINARY			= aswap
MBINARY			= mmserver
OBJECT-DIR		= ../junk
CC      		= gcc
CFLAGS  		= -O2
HDRS          		= 
DEFS			= 
LIBS    		= 
#
# You'll need these libs for a Solaris build
#
#LIBS    		= -lmalloc -lsocket -lnsl


OBJS          = $(OBJECT-DIR)/glue.o \
		$(OBJECT-DIR)/socket.o \
		$(OBJECT-DIR)/globals.o \
		$(OBJECT-DIR)/plists.o \
		$(OBJECT-DIR)/parse.o \
		$(OBJECT-DIR)/lists.o \
		$(OBJECT-DIR)/commands.o \
		$(OBJECT-DIR)/admin.o \
		$(OBJECT-DIR)/room.o \
		$(OBJECT-DIR)/mail.o \
		$(OBJECT-DIR)/tag.o \
		$(OBJECT-DIR)/editor.o \
		$(OBJECT-DIR)/session.o \
		$(OBJECT-DIR)/dynamic.o
#		/usr/lib/debug/malloc.o

all:   			$(BIN)/$(EW2BINARY) mangel
			echo 'Made all'

loligo:			
			touch config.h
			gmake DEFS=-DLOLIGO EW2BINARY=swap

langel:		
			touch angel.c
			gmake DEFS=-DLOLIGO ABINARY=aswap angel

mangel:			$(BIN)/$(ABINARY)

lmms: 			
			touch mms.c
			gmake DEFS=-DLOLIGO MBINARY=mswap mms

mms:			$(BIN)/$(MBINARY)

$(BIN)/$(ABINARY):	angel.c Makefile
		$(CC) $(CFLAGS) -o $(BIN)/$(ABINARY) $(DEFS) angel.c $(LIBS)
		strip $(BIN)/$(ABINARY)

$(BIN)/$(MBINARY):	mms.c Makefile
		$(CC) $(CFLAGS) -o $(BIN)/$(MBINARY) $(DEFS) mms.c $(LIBS)
		strip $(BIN)/$(MBINARY)

$(BIN)/$(EW2BINARY):   	$(OBJS) Makefile
		$(CC) $(CFLAGS) -o $(BIN)/$(EW2BINARY) $(DEFS) $(OBJS) $(LIBS)
		strip $(BIN)/$(EW2BINARY)

install:        	$(BIN)/$(EW2BINARY) angel mms
			echo 'Made install'

program:        	$(BIN)/$(EW2BINARY)
			echo 'Made program'

$(OBJECT-DIR)/%.o:	config.h player.h
		$(CC) $(CFLAGS) $(DEFS) -c $*.c -o $(OBJECT-DIR)/$*.o

###
$(OBJECT-DIR)/glue.o: glue.c 
$(OBJECT-DIR)/socket.o: socket.c 
$(OBJECT-DIR)/globals.o: globals.c 
$(OBJECT-DIR)/plists.o: plists.c compaction.c
$(OBJECT-DIR)/parse.o: parse.c clist.h
$(OBJECT-DIR)/lists.o: lists.c 
$(OBJECT-DIR)/commands.o: commands.c 
$(OBJECT-DIR)/admin.o: admin.c
$(OBJECT-DIR)/room.o: room.c
$(OBJECT-DIR)/mail.o: mail.c
$(OBJECT-DIR)/tag.o: tag.c
$(OBJECT-DIR)/malloc.o: malloc.c
$(OBJECT-DIR)/editor.o: editor.c
$(OBJECT-DIR)/session.o: session.c
$(OBJECT-DIR)/dynamic.o: dynamic.c
