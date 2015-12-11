CC      ?= gcc
CFLAGS  ?= -std=c99 -pedantic -Wall
LDFLAGS ?=

OBJ = main.o
PROGNAME = i2ctool

all: $(OBJ)
	$(CC) $(CFLAGS) -o $(PROGNAME) $(OBJ) $(LDFLAGS)

clean: $(OBJ)
	@echo "Clean object files"
	@rm -f $<
	@rm -f $(PROGNAME)

%.o: %.c
	$(CC) $(CFLAGS) -c $<
