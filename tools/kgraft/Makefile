CC	= $(CROSS_COMPILE)gcc
CFLAGS	= -g

all: dwarf-inline-tree it2rev.pl

dwarf-inline-tree: dwarf-inline-tree.o dwarf_names.o
	$(CC) -o $@ $^ -ldwarf -lelf

dwarf-inline-tree.o: dwarf_names.h
dwarf_names.o: dwarf_names.h

clean:
	rm -f dwarf-inline-tree.o dwarf_names.o dwarf-inline-tree

.PHONY: all clean
