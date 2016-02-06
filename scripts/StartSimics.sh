#!/bin/sh
set -x

. scripts/GlobalDefinition.sh

function cleanup_lock {
	rm -f ${LOCK_FILE}
}

if [ ${ByJenkins} == yes ] ; then
	umask 022
	# Only one Simics instance is allowed because no way to access individual
	# Simics. Remember, ports are 4022 and 5022.
	#
	# There is a gap between done and touch. So that this is not a perfect
	# mutual exclusion, but practically works.
	#
	# Wait until Simics dies
	declare -i iterate=0
	while [ -e ${LOCK_FILE} ]; do
		iterate=$((iterate + 1))
		if [ ${iterate} -gt ${LOCK_TIMEOUT} ]; then
			echo Simics is running: ${LOCK_TIMEOUT}
			exit 15
		fi
		sleep 1
	done
	trap cleanup_lock INT TERM
	touch ${LOCK_FILE}

    # make sure no simics process running
    killall simics-common

    # start simics
    pushd ${fxr}/simics/workspace
    ./simics -no-win -e '$disk_image=../FxrRhel7.craff' \
	FXR.simics >../simics.log &
    popd
else # manual invocation
    # start simics if not yet
    if [ -z `pidof simics-common` ]; then
	pushd ${fxr}/simics/workspace
	./simics -no-win -e '$disk_image=../FxrRhel7.craff' \
	    FXR.simics >../simics.log &
	popd
    fi
fi

# show which version/commit of Simics, craff file and others I am using
pwd; git log -n1 | head -1
${fxr}/simics/workspace/bin/simics --version
( cd ${fxr}/simics/workspace/; git log -n1 | head -1 )
ls -l ${fxr}/simics/FxrRhel7.craff
ls -l ${fxr}/simics/SynopsisInstructionSetSimulator
