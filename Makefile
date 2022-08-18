################################################################
#
# $Id:$
#
# $Log:$
#

ifeq ($(V),1)
	VB=''
else
	VB=@
endif

CC=clang


CFLAGS+=-O3 -nostdlib -target wasm32 -I./newlib-cygwin/newlib/libc/include -I./include -D__IEEE_LITTLE_ENDIAN -DMINECRAFT=1

CFLAGS+=-Dmalloc_getpagesize="(65536)" -D_GNU_SOURCE=1 -DNO_FLOATING_POINT=1
CFLAGS+=-ggdb3 -Wall # -DNORMALUNIX -DLINUX -DSNDSERV # -DUSEASM


# Normally, Wasmcraft programs that need external files will include the files in the binary in the form of a C array.
# This means that we can "mmap" it in as a no-op to be more efficient.
# However, we don't *really* have mmap, so we only define HAVE_MMAP when compiling user functions.
CFLAGS_N = $(CFLAGS) -DHAVE_MMAP=0
CFLAGS_M = $(CFLAGS) -DHAVE_MMAP=1

LDFLAGS+=-Wl,--lto-O3,--gc-sections,--import-undefined 
LIBS+=

# subdirectory for objects
OBJDIR=build
OBJDIR2=build2
OUTPUT=newlib_example.wasm

OBJS_NEWLIB+=errno.o sysfstat.o

include Makefile.string.inc
include Makefile.stdlib.inc
include Makefile.stdio.inc
include Makefile.reent.inc
include Makefile.ctype.inc
include Makefile.posix.inc
include Makefile.locale.inc
include Makefile.libmcommon.inc

OBJS2 += $(addprefix $(OBJDIR2)/, $(OBJS_NEWLIB))

LIBC_DIR = ./newlib-cygwin/newlib/libc
LIBM_DIR = ./newlib-cygwin/newlib/libm

#SRC_NEWLIB_RAW = stdio/printf.c stdio/fprintf.c
#SRC_NEWLIB += $(addprefix ./newlib-cygwin/newlib/libc/, $(SRC_NEWLIB_RAW))

SRC_USER = main.o stubs.o
OBJS += $(addprefix $(OBJDIR)/, $(SRC_USER))

all:	 $(OUTPUT) newlib_example.wat

clean:
	rm -rf $(OBJDIR)
	rm -rf $(OBJDIR2)
	rm -f $(OUTPUT)
	rm -f $(OUTPUT).gdb
	rm -f $(OUTPUT).map

newlib_example.wat: $(OUTPUT)
	wasm2wat $(OUTPUT) -o newlib_example.wat

$(OUTPUT):	$(OBJS) $(OBJS2)
	@echo [Linking $@]
	$(VB)$(CC) $(CFLAGS_M) $(LDFLAGS) $(OBJS) $(OBJS2) \
	-o $(OUTPUT) $(LIBS) -Wl
#	@echo [Size]
#	-$(CROSS_COMPILE)size $(OUTPUT)

$(OBJS): | $(OBJDIR)

$(OBJS2): | $(OBJDIR2)

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(OBJDIR2):
	mkdir -p $(OBJDIR2)

$(OBJDIR)/%.o:	%.c
	@echo [Compiling $<]
	$(VB)$(CC) $(CFLAGS_M) -Werror-implicit-function-declaration -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/stdio/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/string/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/stdlib/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/reent/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/ctype/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/posix/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/locale/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/errno/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBC_DIR)/syscalls/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

$(OBJDIR2)/%.o: $(LIBM_DIR)/common/%.c
	@echo [Compiling newlib $<]
	$(VB)$(CC) $(CFLAGS_N) -c $< -o $@

print:
	@echo OBJS: $(OBJS)

