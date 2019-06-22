all: dal_sample
CFLAGS := -Wall -Wextra
#CFLAGS += -DINSTALL_APPLET
objs := main.o dal_err.o
dal_sample: $(objs)
	$(CC) -o $@ $(objs)
clean:
	$(RM) dal_sample *.o
