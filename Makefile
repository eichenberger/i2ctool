CC      ?= gcc
CFLAGS  ?= -std=c99 -pedantic -Wall
LDFLAGS ?=

OBJ = main.o
PROGNAME = i2ctool

exec_prefix ?= /usr
bindir ?= $(exec_prefix)/bin

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OBJ) $(LDFLAGS)

install: all
	install -d $(DESTDIR)$(bindir)
	install -m 0755 $(PROGNAME) $(DESTDIR)$(bindir)

clean: $(OBJ)
	@echo "Clean object files"
	@rm -f $<
	@rm -f $(PROGNAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
