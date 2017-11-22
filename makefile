SRCPATH = src
OBJPATH = obj
TARPATH = bin

CC = gcc
CFLAGS = -Wall -g -Og
LDFLAGS =

PROXY = proxy
TARPROXY = $(TARPATH)/$(PROXY)
LIBPROXY = main
OBJPROXY = $(patsubst %,$(OBJPATH)/$(PROXY)/%.o,$(LIBPROXY))

all: $(TARPROXY)

$(TARPROXY) : $(OBJPROXY)
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJPATH)/%.o : $(SRCPATH)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $^
