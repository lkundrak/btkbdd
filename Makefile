# Bluetooth software keyboard
# Lubomir Rintel <lkundrak@v3.sk>
# License: GPL

VERSION = 1.4

CFLAGS += -Wall -g3
override LDFLAGS += $(shell pkg-config bluez --libs)
override LDFLAGS += -Wl,--as-needed

PREFIX = /usr/local

BINS = btkbdd/btkbdd
MAN = btkbdd/btkbdd.8
UDEV_RULES = btkbdd/90-btkbdd.rules
SERVICES = btkbdd/btkbdd@.service
all: $(BINS) $(MAN)

btkbdd/btkbdd: btkbdd/main.o btkbdd/keyb.o btkbdd/sdp.o btkbdd/l2cap.o btkbdd/hci.o

btkbbdd/keyb.o: btkbdd/btkbdd.h btkbdd/hid.h btkbdd/linux2hid.h
btkbbdd/l2cap.o: btkbdd/btkbdd.h
btkbbdd/main.o: btkbdd/btkbdd.h
btkbbdd/sdp.o: btkbdd/btkbdd.h btkbdd/apple.h

$(BINS):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.8: %.pod
	pod2man --center 'System management commands' --release $(VERSION) $< >$@

%.html: %.pod
	pod2html $< >$@

%.ps: %.8
	groff -mman $< >$@

clean:
	rm -f */*.o $(BINS) $(MAN)

install: $(BINS)
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	install -p $(BINS) $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)/etc/udev/rules.d
	install -p -m644 $(UDEV_RULES) $(DESTDIR)/etc/udev/rules.d
	mkdir -p $(DESTDIR)$(PREFIX)/lib/systemd/system
	install -p -m644 $(SERVICES) $(DESTDIR)$(PREFIX)/lib/systemd/system
	mkdir -p $(DESTDIR)/var/lib/btkbdd
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man8
	install -p -m644 $(MAN) $(DESTDIR)$(PREFIX)/share/man/man8
