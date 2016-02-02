#!/bin/bash

. scripts/GlobalDefinition.sh

myname=$0
PWD=`pwd`

# checkpatch
DIRS="opa2 opa2_vnic opa_core user verbs"
CHECKPATCH_COMMAND=/lib/modules/${KERNEL_VERSION}/build/scripts/checkpatch.pl
CHECKPATCH_OPTIONS="--file --no-tree --terse --strict"

find ${DIRS} -type f -name "*.h" -o -name "*.c" |\
while read file; do
	${CHECKPATCH_COMMAND} ${CHECKPATCH_OPTIONS} ${file}
done # just report

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
