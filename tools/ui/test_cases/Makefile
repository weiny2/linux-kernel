PTHREAD_LIBS = -lpthread
WARNINGS = -Wall -Wextra
CFLAGS = $(WARNINGS) -g -muli -mgeneral-regs-only -I../include
LDFLAGS = $(PTHREAD_LIBS)

all: testcase1 testcase2 testcase3 testcase4 testcase5 testcase6 testcase7 testcase11 testcase12 testcase15 testcase16 
%: %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	$(RM) testcase1 testcase2 testcase3 testcase4 testcase5 testcase6 testcase7 testcase11 testcase12 testcase15 testcase16
