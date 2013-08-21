
CFLAGS += --std=gnu99 -g -Wall -Wextra -Wno-unused -Wno-unused-parameter

CFLAGS += $(shell pkg-config --cflags upower-glib)

STATIC_LIBS = /usr/lib/libixp.a

COMMON_OBJS = statusbar.o
CLOCK_OBJS = wmii-clock.o proc_stat.o proc_meminfo.o
UPOWER_OBJS = wmii-upower.o
BINS = wmii-clock wmii-upower

default: $(BINS)

clean:
	rm -f $(BINS) $(CLOCK_OBJS) $(UPOWER_OBJS) $(COMMON_OBJS)

wmii-clock: $(CLOCK_OBJS) $(COMMON_OBJS)
	$(CC) -o wmii-clock  $(CLOCK_OBJS) $(COMMON_OBJS) $(STATIC_LIBS)

wmii-upower: $(UPOWER_OBJS) $(COMMON_OBJS)
	$(CC) -o wmii-upower $(shell pkg-config --libs upower-glib) $(UPOWER_OBJS) $(COMMON_OBJS) $(STATIC_LIBS)
