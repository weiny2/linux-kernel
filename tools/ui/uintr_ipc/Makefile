PTHREAD_LIBS = -lpthread
WARNINGS = -Wall -Wextra
CFLAGS = $(WARNINGS) -g -muli -mgeneral-regs-only -I../include
LDFLAGS = $(PTHREAD_LIBS)

all: uintr_ipc
%: %.c
        $(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) uintr_ipc
