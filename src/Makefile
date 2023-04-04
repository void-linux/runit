-include ../config.mk

PROGS=chpst runit runit-init runsv runsvchdir runsvdir sv svlogd utmpset

.PHONY: all check install clean

all: $(PROGS)

check: $(PROGS)
	./check-local $(PROGS)

install: all
	install -d $(DESTDIR)$(SBINDIR)
	@for prog in $(PROGS); do \
		install -m 755 $$prog $(DESTDIR)$(SBINDIR); \
	done

clean:
	rm -f *.o *.a $(PROGS) *.local

runit: runit.o unix.a byte.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) -static -o $@

runit-init: runit-init.o unix.a byte.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) -static -o $@

runsv: runsv.o unix.a byte.a time.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

runsvdir: runsvdir.o unix.a byte.a time.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

sv: sv.o unix.a byte.a time.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

utmpset: utmpset.o unix.a byte.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

runsvchdir: runsvchdir.o unix.a byte.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

svlogd: svlogd.o pmatch.o fmt_ptime.o unix.a byte.a time.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) $(SOCKETLIB_LDFLAGS) -o $@

chpst: chpst.o uidgid.o unix.a byte.a
	@printf " [CCLD]\t\t$@\n"
	${SILENT}$(CC) $^ $(LDFLAGS) $(BUILD_STATIC) -o $@

byte.a: byte_chr.o byte_copy.o byte_cr.o byte_diff.o byte_rchr.o \
fmt_uint.o fmt_uint0.o fmt_ulong.o scan_ulong.o str_chr.o \
str_diff.o str_len.o str_start.o
	@printf " [AR]\t\t$@\n"
	${SILENT}$(AR) $@ $^

time.a: iopause.o tai_now.o tai_pack.o tai_sub.o tai_unpack.o \
taia_add.o taia_approx.o taia_frac.o taia_less.o taia_now.o \
taia_pack.o taia_sub.o taia_uint.o
	@printf " [AR]\t\t$@\n"
	${SILENT}$(AR) $@ $^

unix.a: alloc.o alloc_re.o buffer.o buffer_0.o buffer_1.o buffer_2.o \
buffer_get.o buffer_put.o buffer_read.o buffer_write.o coe.o env.o \
error.o error_str.o fd_copy.o fd_move.o fifo.o lock_ex.o lock_exnb.o \
ndelay_off.o ndelay_on.o open_append.o open_read.o \
open_trunc.o open_write.o openreadclose.o pathexec_env.o \
pathexec_run.o readclose.o seek_set.o sgetopt.o sig.o \
sig_block.o sig_catch.o sig_pause.o stralloc_cat.o stralloc_catb.o \
stralloc_cats.o stralloc_eady.o stralloc_opyb.o stralloc_opys.o \
stralloc_pend.o strerr_die.o strerr_sys.o subgetopt.o wait_nohang.o \
wait_pid.o
	@printf " [AR]\t\t$@\n"
	${SILENT}$(AR) $@ $^

%.o: %.c
	@printf " [CC]\t\t$@\n"
	${SILENT}$(CC) $(CFLAGS) $(CPPFLAGS) -c $<
