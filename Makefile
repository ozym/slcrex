
# Build environment can be configured the following
# environment variables:
#   CC : Specify the C compiler to use
#   CFLAGS : Specify compiler options to use
#

CFLAGS += -I. -DPACKAGE_VERSION=\"1.0.2\" -DFIRFILTERS=\"/etc/filters.fir\"

LDFLAGS =
LDLIBS = -lcrex -ltidal -ldali -lslink -lmseed -lm

all: slcrex mscrex

slcrex: slcrex.o
	$(CC) $(CFLAGS) -o $@ slcrex.o $(LDFLAGS) $(LDLIBS)

mscrex: mscrex.o
	$(CC) $(CFLAGS) -o $@ mscrex.o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f slcrex.o slcrex mscrex.o mscrex

# Implicit rule for building object files
%.o: %.c
	$(CC) $(CFLAGS) -c $<

install:
	@echo
	@echo "No install target, copy the executable(s) yourself"
	@echo
