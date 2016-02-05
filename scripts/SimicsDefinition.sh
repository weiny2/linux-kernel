# for Simics
viper0=4022
viper1=5022
fxr=/mnt/fabric/fxr
export LD_LIBRARY_PATH=${fxr}/simics/SynopsisInstructionSetSimulator/lib
export SNPSLMD_LICENSE_FILE="26586@synopsys03p.elic.intel.com"
export LM_PROJECT=”FDO”
LOCK_DIR=/run/lock
LOCK_FILE=${LOCK_DIR}/${myname}.lock
LOCK_TIMEOUT=900 # 900 = 60 * 15 = 15min
MAX_SIMICS_WAIT=180 # = 3 * 60 = 3min
DISK_IMAGE="../FxrRhel7.craff"

function cleanup_lock {
	rm -f ${LOCK_FILE}
}

function lock_simics {
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
			return 15
		fi
		sleep 1
	done
	trap cleanup_lock INT TERM
	touch ${LOCK_FILE}
	return 0
}

function cleanup_simics {
	if [ ${ByJenkins} == yes ]; then
		# stop simics
		killall simics-common
		cleanup_lock
	fi
}

function start_simics {
    # start simics
    pushd ${fxr}/simics/workspace
    ./simics -no-win -e '$disk_image='${DISK_IMAGE} \
		FXR.simics >../simics.log &
    popd
}
