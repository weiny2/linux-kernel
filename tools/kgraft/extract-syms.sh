#!/bin/bash
TOOLPATH=`dirname $0`
if ! test -f vmlinux.o; then
    echo "vmlinux.o needs to exist in cwd"
    exit 1
fi
if test -z "$1"; then
    echo "usage: $0 [list of symbols to extract]"
    exit 2
fi

rm -f symlist symlist.rename extracted.o
for i in $@; do
    echo $i >> symlist
    echo $i new_$i >> symlist.rename
done
$TOOLPATH/objcopy-hacked --strip-unneeded -j .doesntexist. --keep-symbols symlist --redefine-syms symlist.rename vmlinux.o extracted.o
nm extracted.o
