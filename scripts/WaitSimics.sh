#!/bin/sh
xtrace_state=`set -o | grep xtrace | sed -e"s/ *//g" | sed -e"s/\t/ /" | cut -d" " -f2`
set +x

. scripts/GlobalDefinition.sh
. scripts/SimicsDefinition.sh

declare -i iterate=0

while [ -z `pidof simics-common` ]; do
	if [ $((iterate++)) -gt ${MAX_SIMICS_WAIT} ]; then
		printf "No simics process for %d seconds\n" ${MAX_SIMICS_WAIT}
		exit 1
	fi
	sleep 1
done

ssh_cmd="ssh -p${viper0} root@localhost -oConnectTimeout=1"

while ! ${ssh_cmd} ls 2>/dev/null >/dev/null; do
	if [ $((iterate++)) -gt ${MAX_SIMICS_WAIT} ]; then
		printf "No simics process for %d seconds\n" ${MAX_SIMICS_WAIT}
		exit 1
	fi
	sleep 1
done

if [ ${xtrace_state} == "on" ]; then
	set -x
fi
