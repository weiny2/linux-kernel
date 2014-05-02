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
mkdir -p kgrafttmp
$TOOLPATH/extract-syms.sh $@
mv extracted.o kgrafttmp
cd kgrafttmp
$TOOLPATH/create-stub.sh $@ > kgrstub.c
cat <<EOF > Makefile
obj-m = kgrmodule.o
kgrmodule-y += kgrstub.o extracted.o

all:
	make -C .. M=\$(PWD) modules
EOF
make
cd ..
ls -l kgrafttmp/kgrmodule.ko
