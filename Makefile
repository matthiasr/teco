#
#  TECO for ANSI/POSIX UNIX
#
CFLAGS = -O
CC = gcc -ansi -D_POSIX_SOURCE
TERMCAP = termcap
DESTDIR ?= /usr/local
bindir = /bin
mandir = /man

OBJS = te_data.o te_utils.o te_subs.o te_main.o te_rdcmd.o te_exec0.o \
		te_exec1.o te_exec2.o te_srch.o te_chario.o te_window.o \
		te_fxstub.o

te: $(OBJS)
	$(CC) -o te $(OBJS) -l$(TERMCAP)

te_chario.o: te_defs.h te_chario.c
te_data.o: te_defs.h te_data.c
te_exec0.o: te_defs.h te_exec0.c
te_exec1.o: te_defs.h te_exec1.c
te_exec2.o: te_defs.h te_exec2.c
te_fxstub.o: te_defs.h te_fxstub.c
te_main.o: te_defs.h te_main.c
te_rdcmd.o: te_defs.h te_rdcmd.c
te_srch.o: te_defs.h te_srch.c
te_subs.o: te_defs.h te_subs.c
te_utils.o: te_defs.h te_utils.c
te_window.o: te_defs.h te_window.c

clean:
	rm -f $(OBJS) te core *~ teco.tar

tar:
	tar -cvf teco.tar `cat MANIFEST`

install: te te.1
	install -c -m 755 te $(DESTDIR)$(bindir)/te
	install -c -m 644 te.1 $(DESTDIR)$(mandir)/man1/te.1
	@echo
	@echo "You may also want to copy sample.tecorc or the more extensive sample.tecorc2 to ~/.tecorc"
