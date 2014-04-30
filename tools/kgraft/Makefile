CC=gcc
CFLAGS=-g

all: objcopy-hacked dwarf-inline-tree it2rev.pl

objcopy-hacked: objcopy.diff
	echo "Build by hand!"
	exit 1

dwarf-inline-tree: dwarf-inline-tree.o dwarf_names.o
	gcc -o $@ $^ -ldwarf -lelf

dwarf-inline-tree.o: dwarf_names.h
dwarf_names.o: dwarf_names.h

check: app.o symlist all
	@echo "inline tree"
	./dwarf-inline-tree app.o
	@echo "inline pairs"
	./dwarf-inline-tree app.o | perl it2rev.pl
	@echo "extract stuff"
	./objcopy-hacked --strip-unneeded -j .doesntexist. --keep-symbols symlist app.o app-extract.o
	@echo "symbols"
	readelf -sW app.o app-extract.o

app.c: app.h
app.o: CFLAGS=-g -ffunction-sections -fdata-sections

clean:
	rm -f dwarf-inline-tree.o dwarf_names.o dwarf-inline-tree app.o app-extract.o
