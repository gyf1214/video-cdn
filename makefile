SRCPATH = src
OBJPATH = obj
TARPATH = bin
LOGPATH = log

CC = gcc
CFLAGS = -Wall -g -Iinclude
LDFLAGS = -lm

PROXY = proxy
TARPROXY = $(TARPATH)/$(PROXY)
LIBPROXY = main config io server buffer client util
OBJPROXY = $(patsubst %,$(OBJPATH)/$(PROXY)/%.o,$(LIBPROXY))

DNS = nameserver
TARDNS = $(TARPATH)/$(DNS)
LIBDNS = main config server
OBJDNS = $(patsubst %,$(OBJPATH)/$(DNS)/%.o,$(LIBDNS))

all: $(TARPROXY) $(TARDNS)
	mkdir -p $(LOGPATH)

clean:
	rm -fr $(OBJPATH) $(TARPATH)

rebuild: clean all

$(TARPROXY) : $(OBJPROXY)
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

$(TARDNS) : $(OBJDNS)
	mkdir -p $(dir $@)
	$(CC) $(LDFLAGS) -o $@ $^

$(OBJPATH)/%.o : $(SRCPATH)/%.c makefile
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

.PHONY: all clean rebuild
