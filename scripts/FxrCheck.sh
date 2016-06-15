#!/bin/bash

. scripts/GlobalDefinition.sh

myname=$0
PWD=`pwd`

# checkpatch
DIRS="hfi2 hfi2_vnic hfi_core user"
CHECKPATCH_COMMAND=/lib/modules/${KERNEL_VERSION}/build/scripts/checkpatch.pl
CHECKPATCH_OPTIONS="--file --no-tree --terse --strict"

# FRXTODO Exclude generated headers in hfi2/fxr/* from style check, those files
# do conform to kernel coding standard and generate ~70K errors/warnings
if [ "$1" == "" ]; then
    find ${DIRS} -type f -name "*.h" -o -name "*.c" | grep -v "hfi2/fxr/" |\
    while read file; do
	    ${CHECKPATCH_COMMAND} ${CHECKPATCH_OPTIONS} ${file}
    done # just report
else
    ${CHECKPATCH_COMMAND} ${CHECKPATCH_OPTIONS} $1
    exit $?
fi

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
