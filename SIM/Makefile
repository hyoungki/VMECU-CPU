# ------------------------------------------
#	TARGET º° NFS Á¤º¸ 
# 	centos : /opt/eldk.8xx/ppc_8xx/home/hslee
# 	Fedora :/opt/eldk.4.1.6xx/ppc_6xx/home/mfcu
# ------------------------------------------


LINUXPATH   = /opt/eldk/ppc_6xx/usr
HPATH       = $(LINUXPATH)/include
INCLUDEDIRS +=  -I$(HPATH)

#CROSS_COMPILE   :=/opt/eldk/usr/bin/ppc_6xx-
CROSS_COMPILE   =ppc_6xx-

USER_INC = $(LOCAL_HOME)/inc
USER_LIB = $(LOCAL_HOME)/lib
USER_BIN = $(LOCAL_HOME)/bin

INCLUDEDIRS +=  -I$(TOPDIR)/include  -I. -I$(HPATH)

CXX  =$(CROSS_COMPILE)g++
AS  =$(CROSS_COMPILE)as
LD  =$(CROSS_COMPILE)ld
#CC  =$(CROSS_COMPILE)gcc 
CC  =$(CROSS_COMPILE)gcc -I$(USER_INC)
CPP =$(CC) -E
AR  =$(CROSS_COMPILE)ar
NM  =$(CROSS_COMPILE)nm
STRIP   =$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump

CFLAGS   += $(INCLUDEDIRS) -Wall -O2 

#
# --- Makefile rules
#
DEFINES =
CCOPTS = -c -O2 $(DEFINES) -Wall
LDOPTS = $(DEFINES) -L$(USER_LIB)

CC = $(CROSS_COMPILE)gcc $(CCOPTS) -I$(USER_INC)
LINK = $(CROSS_COMPILE)gcc $(LDOPTS) -lpthread

.c.o:	; $(CC) $<


%.o: %.c
#	$(CC) -c -o $@ $<
	$(CC) -c -o $@ $<
#
# Object files for this demo
#
OBJS =	simMain.o simThread.o

BAKS = *.bak

LIBS = -lLocal

#
# Library files used for dependency tests
#
LIB_DEPS =

#
# Target Process
#
TARGET = SIM

#
# Make the executable program
#
all:	$(TARGET)

default:	$(TARGET)

$(TARGET):	$(OBJS)
#	$(CC) ${CFLAGS} -o $@ $(OBJS)
	$(LINK) $(OBJS) -o $@ $(LIBS)
	@cp -f  $@ $(TARGET_BIN)	
#	@cp -f  $@ $(NFS_HOME)

clean:
	@/bin/rm -fr core $(TARGET) $(OBJS) $(BAKS)
