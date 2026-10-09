CC      ?= clang
PODMAN  ?= podman

VERSION   = v0.1.0
BINDIR    = bin
INCDIR    = include
BINARY    = nelvana
BINARYCTL = nelvanactl
CFLAGS  = -g -std=c2x -Wall -Wextra -fpic \
          -I/usr/local/include \
          -Dbin_name=$(BINARY) \
          -D$(BINARY)_version=$(VERSION) \
		  -Dgit_sha=$(shell git rev-parse HEAD)
LDFLAGS = -L/usr/local/lib \
          -lrattler \
          -lpapago \
          -lmaple \
          -lsqlite3 \
          -lssl \
          -lcrypto \
          -ljansson \
          -llogger \
          -lpthread \
		  -lcurl
PREFIX = /usr/local

MACOS_MANPAGE_LOC = /usr/share/man
LINUX_MANPAGE_LOC = /usr/share/man/man1

.PHONY: nelvana-keys
nelvana-keys: $(BINDIR)
	rm -f  $(BINDIR)/$(BINARY)-keys
	$(CC) $(CFLAGS) nelvana-keys.c db.c -o $(BINDIR)/$(BINARY)-keys $(LDFLAGS)

.PHONY: nelvana-exec
nelvana-exec: $(BINDIR)
	rm -f  $(BINDIR)/$(BINARY)-exec
	$(CC) $(CFLAGS) nelvana-exec.c db.c -o $(BINDIR)/$(BINARY)-exec $(LDFLAGS)

.PHONY: nelvana-server
nelvana-server: $(BINDIR)
	rm -f  $(BINDIR)/$(BINARY)-server
	$(CC) $(CFLAGS) server.c db.c -o $(BINDIR)/$(BINARY)-server $(LDFLAGS)
	
$(BINDIR):
	mkdir -p $(BINDIR)

.PHONY: install
install: $(BINDIR)/$(BINARY)
	install $(BINDIR)/$(BINARY) $(PREFIX)/$(BINDIR)/$(BINARY)
ifeq ($(UNAME_S),Darwin)
	cp $(BINARY).1 $(MACOS_MANPAGE_LOC)/$(BINARY).1
else
	cp $(BINARY).1 $(LINUX_MANPAGE_LOC)/$(BINARY).1
endif

.PHONY: uninstall
uninstall: 
	rm -f $(PREFIX)/$(BINDIR)/$(BINARY)*
ifeq ($(UNAME_S),Darwin)
	rm -f $(MACOS_MANPAGE_LOC)/$(BINARY).1
else
	rm -f $(LINUX_MANPAGE_LOC)/$(BINARY).1
endif

.PHONY: image
image:
	$(PODMAN) build -t briandowns/$(BINARY):latest .

.PHONY: push
push:
	$(PODMAN) push briandowns/$(BINARY):latest

.PHONY: clean
clean:
	rm -f $(BINDIR)/*
