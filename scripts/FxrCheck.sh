#!/bin/bash

. scripts/GlobalDefinition.sh

myname=$0
PWD=`pwd`

# sparse
export PATH=$PATH:/usr/local/bin
# check whether sparse is installed
if ! type sparse 2>/dev/null; then
	echo Have you installed sparse?
	exit 99
fi
# run sparse
make C=2 KVER=${KERNEL_VERSION}
res=$?
if [ ${res} != 0 ]; then
	printf "error on sparse: %d\n" ${res}
	exit ${res}
fi

exit 0
