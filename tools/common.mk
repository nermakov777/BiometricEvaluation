#
# This software was developed at the National Institute of Standards and
# Technology (NIST) by employees of the Federal Government in the course
# of their official duties. Pursuant to title 17 Section 105 of the
# United States Code, this software is not subject to copyright protection
# and is in the public domain. NIST assumes no responsibility  whatsoever for
# its use by other parties, and makes no guarantees, expressed or implied,
# about its quality, reliability, or any other characteristic.
#
# This set of directories is where the header files, libraries, programs,
# and man pages are to be installed.
INCPATH := /usr/local/include
LIBPATH := /usr/local/lib
BINPATH := /usr/local/bin
MANPATH := /usr/local/man/man1

#
# Files and directories that are created during the build process, that
# are to be removed during 'make clean'.
DISPOSABLEFILES = *.o *.exe .gdb_history
DISPOSABLEDIRS  = *.dSYM

CP := cp -f
RM := rm -f
PWD := $(shell pwd)
OS := $(shell uname -s)
ARCH := $(shell arch)
CC = gcc
CXX = g++

ifeq ($(findstring CYGWIN,$(OS)), CYGWIN)
        ROOT = Administrator
else
        ROOT = root
endif
ifeq ($(findstring x86_64,$(ARCH)), x86_64)
        ARCHOPT = -fPIC
endif


#
# If there are any 'non-standard' include or lib directories that need to
# be searched prior to the 'standard' libraries, add the to the CFLAGS
# variable.

CFLAGS := -g -std=c99 $(ARCHOPT) $(COMMONINCOPT) -I$(LOCALINC) -I$(INCPATH) $(COMMONLIBOPT) -L$(LIBPATH)
CXXFLAGS := -g $(ARCHOPT) $(COMMONINCOPT) -I$(LOCALINC) -I$(INCPATH) $(COMMONLIBOPT) -L$(LIBPATH)
