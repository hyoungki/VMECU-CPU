# ------------------------------------------
#	TARGET º° NFS Á¤º¸ 
# 	centos : /opt/eldk.8xx/ppc_8xx/home/hslee
# 	Fedora :/opt/eldk.4.1.6xx/ppc_6xx/home/mfcu
# ------------------------------------------
#NFS_HOME=/opt/eldk/ppc_82xx/home/hslee

#NFS_HOME=/opt/eldk.4.1.6xx/ppc_6xx/home/vmecu

#LOCAL_HOME=/mnt/hgfs/shared/ACE-VMECU-NET-V11

#LINUXPATH   = /opt/eldk.4.1.6xx/ppc_6xx/usr
LINUXPATH   = /opt/eldk/ppc_6xx/usr
HPATH       = $(LINUXPATH)/include
INCLUDEDIRS +=  -I$(HPATH)

#CROSS_COMPILE   :=/opt/eldk/usr/bin/ppc_6xx-

CROSS_COMPILE   =ppc_6xx-

#CROSS_COMPILE   :=/opt/eldk.4.1.6xx/usr/bin/ppc_6xx-

USER_INC = $(LOCAL_HOME)/inc
USER_LIB = $(LOCAL_HOME)/lib
USER_BIN = $(LOCAL_HOME)/bin


INCLUDEDIRS +=  -I$(TOPDIR)/include  -I. -I$(HPATH)

CXX  =$(CROSS_COMPILE)g++
AS  =$(CROSS_COMPILE)as
LD  =$(CROSS_COMPILE)ld
#CC  =$(CROSS_COMPILE)gcc 
CC  =$(CROSS_COMPILE)gcc -I$(USER_INC)  -Wall
CPP =$(CC) -E

ARFLAG = rcv
AR  =$(CROSS_COMPILE)ar
NM  =$(CROSS_COMPILE)nm
STRIP   =$(CROSS_COMPILE)strip
OBJCOPY =$(CROSS_COMPILE)objcopy
OBJDUMP =$(CROSS_COMPILE)objdump

CFLAGS   += $(INCLUDEDIRS) -Wall -O2 

%.o: %.c
#	$(CC) -c -o $@ $<
	$(CC) -c -o $@ $<
#
# Object files for this demo
#
OBJS =	libLocal.o libDnpCRC.o libDnpTIME.o libNetFUNC.o lib_logFile.o  configini.o parse_config.o tk_net_lib.o

BAKS = *.bak

#
# Target Process
#
TARGET = libLocal.a


#
# Make the executable program
#

all:	$(TARGET)

default:	$(TARGET)

$(TARGET):	$(OBJS)
#	$(CC) ${CFLAGS} $(OBJS)
	$(AR) $(ARFLAG) $@ $(OBJS)
#	@mv $@ $(USER_LIB)
#	@cp -f  $@ $(NFS_HOME)

clean:
	echo "clean...lib"
	@/bin/rm -fr core $(TARGET) $(OBJS) $(BAKS)
