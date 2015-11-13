#!/bin/sh
#
# run checkpatch for commit, i.e.
# 1. create a patch file between HEAD and HEAD-1 of git repository
# 2. run checkpatch
# 3. when there is an error on checkpatch, bit 0 of the exit value of this
#    script is on
# 4. clean-up temporary files.
#
set -x

debug=false

KERNELRELEASE=4.3.0-rc4
CHECKPATCH_COMMAND=/lib/modules/${KERNELRELEASE}/build/scripts/checkpatch.pl
CHECKPATCH_OPTIONS="--patch --no-tree --terse --no-signoff --strict"

declare -i ret=0

if [ ${debug} == true ]; then
	diff_file=FxrDiff.diff
	checkpatch_out=FxrCheckpatch.out
	git diff bbb bbb~ >${diff_file}
else
	diff_file=/tmp/FxrDiff$$
	checkpatch_out=/tmp/FxrCheckpatch$$
	git diff HEAD~ >${diff_file}
fi

${CHECKPATCH_COMMAND} ${CHECKPATCH_OPTIONS} ${diff_file} | \
	tee ${checkpatch_out}
grep -q ": ERROR: " ${checkpatch_out} && echo $((ret += 1)) >/dev/null

rm -f ${diff_file} ${checkpatch_out}

exit ${ret}
