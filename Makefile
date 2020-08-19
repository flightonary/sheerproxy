#
# Makefile for sheerproxy (requires GNU make), stolen from proxychains
#
# Use config.mak to override any of the following variables.
# Do not make changes here.
#

srcdir = sheerproxy/
testdir = tests/

SRCS = $(sort $(wildcard sheerproxy/*.c))
OBJS = $(SRCS:.c=.o)
TSRCS = $(sort $(wildcard $(testdir)/*.c))
TOBJS = $(TSRCS:.c=.out)
TLOBJS = $(filter-out %libsheerproxy.o %getnameinfo.o, $(OBJS))

CFLAGS  += -Wall -O2 -std=c11 -D_GNU_SOURCE -pipe
NO_AS_NEEDED = -Wl,--no-as-needed
LIBDL   = -ldl
LDFLAGS = -fPIC $(NO_AS_NEEDED) $(LIBDL) -lpthread
INC     = -I$(realpath $(srcdir))
PIC     = -fPIC
AR      = $(CROSS_COMPILE)ar
RANLIB  = $(CROSS_COMPILE)ranlib
SOCKET_LIBS =

LDSO_SUFFIX = so
LD_SET_SONAME = -Wl,-soname=
INSTALL = ./tools/install.sh

LDSO_PATHNAME = libsheerproxy.$(LDSO_SUFFIX)

ALL_LIBS = $(LDSO_PATHNAME)
ALL_CONFIGS = sheerproxy.conf

-include config.mak

CFLAGS+=$(USER_CFLAGS) $(MAC_CFLAGS)
CFLAGS_MAIN=-DLIB_DIR=\"$(libdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -DDLL_NAME=\"$(LDSO_PATHNAME)\"

INSTALL_LIBS = $(ALL_LIBS:%=$(libdir)/%)
INSTALL_CONF = $(ALL_CONFIGS:%=$(sysconfdir)/%)


all: $(ALL_LIBS)

install: $(INSTALL_LIBS) $(INSTALL_CONF)

$(libdir)/%: %
	$(INSTALL) -D -m 644 $< $@
	./tools/ld_preload.sh install $@

$(sysconfdir)/%: %
	$(INSTALL) -D -m 644 $< $@

uninstall:
	./tools/ld_preload.sh uninstall $(INSTALL_LIBS)
	rm -f $(INSTALL_LIBS)
	rm -f $(INSTALL_CONF)

clean:
	rm -f $(ALL_LIBS)
	rm -f $(OBJS)
	rm -f $(TOBJS)

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_MAIN) $(INC) $(PIC) -c -o $@ $<

$(LDSO_PATHNAME): $(OBJS)
	$(CC) $(LDFLAGS) $(LD_SET_SONAME)$(LDSO_PATHNAME) $(USER_LDFLAGS) \
		-shared -o $@ $(OBJS) $(SOCKET_LIBS)

$(testdir)/%.out: $(testdir)/%.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(CFLAGS_MAIN) $(INC) $(LDFLAGS) -o $@ $< $(TLOBJS)
	chmod +x $@

test: $(OBJS) $(TOBJS)

.PHONY: all clean install install-config install-libs test
