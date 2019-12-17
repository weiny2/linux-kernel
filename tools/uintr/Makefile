CC		= $(CROSS_COMPILE)gcc
BUILD_OUTPUT	:= $(CURDIR)
PREFIX		?= /usr
DESTDIR		?=

ifeq ("$(origin O)", "command line")
	BUILD_OUTPUT := $(O)
endif

uintr_syscall : uintr_syscall.c

CFLAGS +=       -Wall -g -O0 -ggdb -static -muli -mgeneral-regs-only
LDFLAGS +=      -lpthread
%: %.c
	@mkdir -p $(BUILD_OUTPUT)
	$(CC) $(CFLAGS) $< $(LDFLAGS) -o $(BUILD_OUTPUT)/$@

all: uintr_syscall

.PHONY : clean
clean :
	@rm -f $(BUILD_OUTPUT)/uintr_syscall

install : uintr_sycall
	install -d  $(DESTDIR)$(PREFIX)/bin
	install $(BUILD_OUTPUT)/uintr_syscall $(DESTDIR)$(PREFIX)/bin/uintr_syscall
