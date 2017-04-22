# OPTFLAGS = -g3
OPTFLAGS = -Ofast -DNDEBUG
CFLAGS = -std=c99 -Wno-implicit-int $(OPTFLAGS)
LDFLAGS = -pthread
UTILS = fsa.o executor.c filesys.c parser.c buffer.c random.c logger.c multiproc.c filestream.c
CUTILS = $(patsubst %.c, %.o, $(UTILS))
all:; $(MAKE) -j$(shell getconf _NPROCESSORS_ONLN) _all
_all: fsa_dump shellsplit runcmds runparallelcmds argvp
CFLAGS+=-DSTACSCHECK
fsa_dump:fsa_dump.c $(CUTILS)
shellsplit:shellsplit.c $(CUTILS)
runcmds:runcmds.c $(CUTILS)
runparallelcmds:runparallelcmds.c $(CUTILS)
argvp:argvp.c
clean:
	rm -vf argvp fsa_dump shellsplit runcmds runparallelcmds *.o
	rm -rf *.dSYM
