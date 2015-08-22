CC=gcc
RM=rm -f
CFLAGS=-Wall -Wextra -O2
LDFLAGS=
LIBS=-lssl -lcrypto

SRCS=sha1appender.c
OBJS=$(subst .c,.o,$(SRCS))
BIN=sha1appender

.PHONY: all
all: $(BIN)

$(BIN): $(OBJS) Makefile
	gcc $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

%.o: %.c Makefile
	$(CC) $(CFLAGS) -o $@ -c $<


.PHONY: clean mrproper
clean:
	$(RM) $(OBJS)


mrproper: clean
	$(RM) $(BIN)
