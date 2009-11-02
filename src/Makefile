CC ?= gcc
CFLAGS ?= -Wall -O1 -g

TINIWEB_OBJS = tiniweb.o
MD5_OBJS = md5main.o \
	   md5.o

all: md5main tiniweb

tiniweb: $(TINIWEB_OBJS)

md5main: $(MD5_OBJS)
	$(CC) $(LDFLAGS) -lm $^ -o $@

clean:
	rm -f $(MD5_OBJS) md5main $(TINIWEB_OBJS) tiniweb
