#!/bin/sh
set -x

. scripts/GlobalDefinition.sh
. scripts/SimicsDefinition.sh

if [ ${ByJenkins} == yes ] ; then
	lock_simics
	res=$?
	if [ ! ${res} ]; then
		echo Simics keeps running: ${LOCK_TIMEOUT}
		exit ${res}
	fi

	# Jenkins likes virgin simics
	killall simics-common # make sure no simics running
	start_simics
else # manual invocation
    # start simics if not yet
    if [ -z `pidof simics-common` ]; then
		start_simics
    fi
fi

# show which version/commit of Simics, craff file and others I am using
pwd; git log -n1 | head -1
${fxr}/simics/workspace/bin/simics --version
( cd ${fxr}/simics/workspace/; git log -n1 | head -1 )
ls -l ${fxr}/simics/workspace/${DISK_IMAGE}
ls -l ${fxr}/simics/SynopsisInstructionSetSimulator
