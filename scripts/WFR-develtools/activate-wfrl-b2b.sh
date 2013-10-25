#!/bin/bash

# load ib_wfr_lite driver
# Use infiniband-diags tools to activate back to back ports with assigned lids
# Finally configure both drivers for VL15 over VL0

local_port=2
local_lid=1

remote_node=""
remote_port=2
remote_lid=2

ibmode="false"
reload_drivers="false"

BOLD='\033[1;31m'
DBG='\033[1;35m'
NORMAL='\033[0m'

fake_mtu=0
function usage
{
	cat <<EOL
usage:
	${0##*/} [-h] [-p <local_port>] [-r <remote_port>] -n <remote_node>

Options:

-h         - h)elp
-p port    - l)ocal port to operate on (default $local_port)
-r port    - r)emote port to operate on (default $remote_port)
-i         - i)gnore VL15 over VL0 set up (run in default (i)B mode)
-n node    - n)ode remote node to configure
-d         - d)o reload drivers
-m mtu     - m)tu change the default MTU of the wfr-lite driver
EOL
}

while getopts "hr:p:n:idm:" opt; do
	case "$opt" in
	r)	remote_port="$OPTARG"
		;;
	p)	local_port="$OPTARG"
		;;
	n)	remote_node="$OPTARG"
		;;
	i)	ibmode="true"
		;;
	d)	reload_drivers="true"
		;;
	m)	fake_mtu="$OPTARG"
		;;
	h)	usage
		exit 0
		;;
	*)	usage
		exit 1
		;;
	esac
done

if [ "$remote_node" == "" ]; then
	echo "ERROR: remote node must be specified"
	usage
	exit 1
fi

if [ "$reload_drivers" == "true" ]; then
	printf "${BOLD}Reloading ib_wfr_lite on \"$remote_node\"\n"
	printf "${NORMAL}"

	echo "   removing modules ..."
	ssh root@$remote_node "modprobe -r rdma_ucm"
	ssh root@$remote_node "modprobe -r ib_ipoib"
	ssh root@$remote_node "modprobe -r ib_umad"
	ssh root@$remote_node "modprobe -r ib_usa"
	ssh root@$remote_node "modprobe -r ib_ucm"
	ssh root@$remote_node "modprobe -r ib_uverbs"
	ssh root@$remote_node "modprobe -r ib_qib"
	ssh root@$remote_node "modprobe -r ib_wfr_lite"

	echo "   loading modules ..."
	ssh root@$remote_node "modprobe ib_wfr_lite fake_mtu=$fake_mtu"
	ssh root@$remote_node "modprobe ib_umad"
	ssh root@$remote_node "modprobe ib_uverbs"
	ssh root@$remote_node "modprobe ib_usa"
	ssh root@$remote_node "modprobe ib_ipoib"
	ssh root@$remote_node "modprobe rdma_ucm"
	sleep 5
fi
ssh root@$remote_node "iba_portconfig --ib -w2 -s4 -p $remote_port enable" || exit 1

if [ "$reload_drivers" == "true" ]; then
	printf "${BOLD}Reloading ib_wfr_lite on \"localhost\"\n"
	printf "${NORMAL}"
	echo "   removing modules ..."
	modprobe -r rdma_ucm
	modprobe -r ib_ipoib
	modprobe -r ib_umad
	modprobe -r ib_usa
	modprobe -r ib_ucm
	modprobe -r ib_uverbs
	modprobe -r ib_qib
	modprobe -r ib_wfr_lite

	echo "   loading modules ..."
	modprobe ib_wfr_lite fake_mtu=$fake_mtu
	modprobe ib_umad
	modprobe ib_uverbs
	modprobe ib_usa
	modprobe ib_ipoib
	modprobe rdma_ucm
fi

iba_portconfig --ib -w2 -s4 -p $local_port enable || exit 1

if [ "$ibmode" == "true" ]; then
	exit 0
fi

# continue with STL coonfiguration below

printf "${BOLD}Waiting for remote port to Initialize...\n"
printf "${NORMAL}"
state=""
while [ "$state" != "2: INIT" ]; do
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

printf "${BOLD}Waiting for local port to Initialize...\n"
printf "${NORMAL}"
state=""
while [ "$state" != "2: INIT" ]; do
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

printf "${BOLD}Arming local port.\n"
printf "${NORMAL}"
iba_portconfig --ib -L ${local_lid} -S3 -p $local_port
state=""
while [ "$state" != "3: ARMED" ]; do
	echo "Waiting for local port to arm..."
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

printf "${BOLD}Arming Remote port.\n"
printf "${NORMAL}"
ssh root@$remote_node "iba_portconfig --ib -L ${remote_lid} -S3 -p $remote_port"
state=""
while [ "$state" != "3: ARMED" ]; do
	echo "Waiting for remote port to arm..."
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

sleep 1

printf "${BOLD}Activating local port.\n"
printf "${NORMAL}"
iba_portconfig --ib -S4 -p $local_port
state=""
while [ "$state" != "4: ACTIVE" ]; do
	echo "Waiting for local port to go active..."
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

printf "${BOLD}Activating Remote port.\n"
printf "${NORMAL}"
ssh root@$remote_node "iba_portconfig --ib -S4 -p $remote_port"
state=""
while [ "$state" != "4: ACTIVE" ]; do
	echo "Waiting for remote port to go active..."
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

exit 0
