
CFLAGS += --std=gnu99 -g -Wall -Wextra -Wno-unused -Wno-unused-parameter

CFLAGS += \
	$(shell pkg-config --cflags upower-glib)

PREFIX ?= /usr/local

STATIC_LIBS = /usr/lib/libixp.a

COMMON_OBJS = statusbar.o signals.o
CLOCK_OBJS = wmii-clock.o proc_stat.o proc_meminfo.o sysfs.o
BLOCK_OBJS = wmii-block.o sysfs.o
UPOWER_OBJS = wmii-upower.o
BINS = wmii-clock wmii-upower wmii-block

default: $(BINS)

clean:
	rm -f $(BINS) $(CLOCK_OBJS) $(UPOWER_OBJS) $(COMMON_OBJS)

install: $(BINS)
	mkdir -p $(PREFIX)/bin
	install -m 755 wmii-block $(PREFIX)/bin
	install -m 755 wmii-clock $(PREFIX)/bin
	install -m 755 wmii-upower $(PREFIX)/bin

wmii-clock: $(CLOCK_OBJS) $(COMMON_OBJS)
	$(CC) -o wmii-clock  $(CLOCK_OBJS) $(COMMON_OBJS) $(STATIC_LIBS)

wmii-block: $(BLOCK_OBJS) $(COMMON_OBJS)
	$(CC) -o wmii-block $(BLOCK_OBJS) $(COMMON_OBJS) $(STATIC_LIBS)

wmii-upower: $(UPOWER_OBJS) $(COMMON_OBJS)
	$(CC) -o wmii-upower $(shell pkg-config --libs upower-glib) $(UPOWER_OBJS) $(COMMON_OBJS) $(STATIC_LIBS)
