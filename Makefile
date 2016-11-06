
# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   CFLAGS : Specify compiler options to use
#

CFLAGS += -I. -DPACKAGE_VERSION=\"1.0.0\"

LDFLAGS =
LDLIBS = -ltidal -ldali -lslink -lmseed -lm

LIB_HDRS = firfilter.h crex.h
LIB_SRCS = firfilter.c crex.c
LIB_OBJS = $(LIB_SRCS:.c=.o)

all: slcrex mscrex

slcrex: slcrex.o $(LIB_HDRS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ slcrex.o $(LIB_OBJS) $(LDFLAGS) $(LDLIBS)

mscrex: mscrex.o $(LIB_HDRS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ mscrex.o $(LIB_OBJS) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f slcrex.o slcrex mscrex.o mscrex $(LIB_OBJS)

# Implicit rule for building object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

install:
	@echo
	@echo "No install target, copy the executable(s) yourself"
	@echo
