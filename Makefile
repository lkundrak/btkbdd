# Bluetooth software keyboard
# Lubomir Rintel <lkundrak@v3.sk>
# License: GPL

VERSION = 1.4

CFLAGS += -Wall -g3
override LDFLAGS += $(shell pkg-config bluez --libs)
override LDFLAGS += -Wl,--as-needed

PREFIX = /usr/local

BINS = btkbdd
MAN = btkbdd.8
all: $(BINS) $(MAN)

btkbdd: main.o keyb.o sdp.o l2cap.o hci.o

keyb.o: btkbdd.h hid.h linux2hid.h
l2cap.o: btkbdd.h
main.o: btkbdd.h
sdp.o: btkbdd.h apple.h

$(BINS):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

%.8: %.pod
	pod2man --center 'System management commands' --release $(VERSION) $< >$@

%.html: %.pod
	pod2html $< >$@

%.ps: %.8
	groff -mman $< >$@

clean:
	rm -f *.o $(BINS) $(MAN)

install: $(BINS)
	mkdir -p $(DESTDIR)$(PREFIX)/sbin
	install -p $(BINS) $(DESTDIR)$(PREFIX)/sbin
	mkdir -p $(DESTDIR)/etc/udev/rules.d
	install -p -m644 90-btkbdd.rules $(DESTDIR)/etc/udev/rules.d
	mkdir -p $(DESTDIR)$(PREFIX)/lib/systemd/system
	install -p -m644 btkbdd@.service $(DESTDIR)$(PREFIX)/lib/systemd/system
	mkdir -p $(DESTDIR)/var/lib/btkbdd
	mkdir -p $(DESTDIR)$(PREFIX)/share/man/man8
	install -p -m644 $(MAN) $(DESTDIR)$(PREFIX)/share/man/man8
