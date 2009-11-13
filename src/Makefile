#----------------------------------------------------------------------
# Compiler settins (usually there is no need to change these)
# C compiler and linker
#
# Remark: Normally we link using a standard C-compiler invocation
#
CC     := gcc
LD     := gcc

# Compiler and linker flags
CFLAGS  := -Wall -std=gnu99 -O1 -g
LDFLAGS := -g
LDLIBS  := -lm

#----------------------------------------------------------------------
# Sources for the tiniweb server
#
# Add your own source-files to this variable.
#
TINIWEB_SRC = \
	tiniweb.c \
	secmem.c \
	md5.c \
    envvar.c

# Sources for the MD5 demo application
MD5_SRC = \
	md5main.c \
	md5.c

# Binaries to be build by default
PROGRAMS := md5main tiniweb

#----------------------------------------------------------------------
# Automatically inferred object files (usually there is no need to
# touch these variables)
#

TINIWEB_OBJS := $(TINIWEB_SRC:%.c=%.o)
MD5_OBJS     := $(MD5_SRC:%.c=%.o)

# ----------------------------------------------------------------------
# Make all target (default target)
#
all: $(PROGRAMS)

# ----------------------------------------------------------------------
# Target to compile the tiniweb server
#
tiniweb: $(TINIWEB_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# ----------------------------------------------------------------------
# Target to compile the md5main demo application
#
md5main: $(MD5_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

#----------------------------------------------------------------------
# Target to clean build output
#
clean:
	-rm -f $(PROGRAMS) $(TINIWEB_OBJS) $(MD5_OBJS)

#----------------------------------------------------------------------
# Compilation of source files
#
%.o: %.c
	$(CC) $(CFLAGS) -o$@ -c $<
