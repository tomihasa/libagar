#
# Typical BSDBuild Makefile for an Agar application.
#
# Get BSDBuild from: http://bsdbuild.hypertriton.com.
# See: build.prog.mk(5), build.common.mk(5).
#
# In a finished Makefile please strip the overly verbose comments below:
# 

# Set to the base directory containing mk/ (required).
TOP=.
include ${TOP}/Makefile.config

# For recursion, use SUBDIR:
# SUBDIR=mydir1 mydir2 ...

# Base executable name (subject to prefixing, suffixing and transforms).
PROG=agarhello

# Type of program (GUI|CLI|DAEMON, default CLI).
PROG_TYPE="GUI"

# Program is installable? (Yes|No, default Yes)
# PROG_INSTALL="Yes"

# Apply a sed(1) expression to the installed executable.
# PROG_TRANSFORM="s,x,x,"

# Unique identifier. Generate this using uuidgen:
PROG_GUID=""

#
# Source files. See build.prog.mk(5) for the full list of recognized
# languages and file extensions.
#
SRCS=	agarhello.c
#SRCS+=	foo.cc foo.m foo.asm

#
# Add the compilation flags and library required by Agar-GUI (which
# implies Agar-Core). The variables ${AGAR_CFLAGS} and ${AGAR_LIBS}
# are set by the configure code generated by the test module "agar"
# (invoked by configure.in using "check(agar)" or "require(agar)").
#
CFLAGS+=${AGAR_CFLAGS}
LIBS+=	${AGAR_LIBS}

# This is required since we are building a program.
include ${TOP}/mk/build.prog.mk
