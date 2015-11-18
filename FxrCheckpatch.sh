#!/bin/sh

DIRS="opa2 opa2_vnic opa_core user verbs"
KERNELRELEASE=4.3.0
CHECKPATCH_COMMAND=/lib/modules/${KERNELRELEASE}/build/scripts/checkpatch.pl
CHECKPATCH_OPTIONS="--file --no-tree --terse --strict"

find ${DIRS} -type f -name "*.h" -o -name "*.c" |\
while read file; do
	${CHECKPATCH_COMMAND} ${CHECKPATCH_OPTIONS} ${file}
done

exit 0
