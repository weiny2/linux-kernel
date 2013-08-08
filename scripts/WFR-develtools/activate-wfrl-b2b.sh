#!/bin/sh

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
EOL
}

while getopts "hr:p:n:id" opt; do
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
	echo "Reloading ib_wfr_lite on \"$remote_node\""
	ssh root@$remote_node "rmmod ib_qib"
	ssh root@$remote_node "rmmod ib_wfr_lite"
	ssh root@$remote_node "rmmod ib_umad"
	ssh root@$remote_node "rmmod ib_usa"
	ssh root@$remote_node "rmmod ib_sa"
	ssh root@$remote_node "rmmod ib_mad"
	ssh root@$remote_node "rmmod ib_uverbs"
	ssh root@$remote_node "modprobe ib_wfr_lite"
	ssh root@$remote_node "modprobe ib_umad"
	ssh root@$remote_node "modprobe ib_uverbs"
	ssh root@$remote_node "modprobe ib_usa"
	sleep 5
fi
ssh root@$remote_node "iba_portenable -w2 -s4 -p $remote_port"

if [ "$reload_drivers" == "true" ]; then
	echo "Reloading ib_wfr_lite on \"localhost\""
	rmmod ib_qib
	rmmod ib_wfr_lite
	rmmod ib_umad
	rmmod ib_usa
	rmmod ib_sa
	rmmod ib_mad
	rmmod ib_uverbs
	modprobe ib_wfr_lite
	modprobe ib_umad
	modprobe ib_uverbs
	modprobe ib_usa
fi

iba_portenable -w2 -s4 -p $local_port

if [ "$ibmode" == "true" ]; then
	exit 0
fi

# continue with STL coonfiguration below

echo "Waiting for remote port to Initialize..."
state=""
while [ "$state" != "2: INIT" ]; do
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

state=""
while [ "$state" != "2: INIT" ]; do
	echo "Waiting for local port to Initialize..."
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

echo "Arming local port."
iba_portconfig -L ${local_lid} -S3 -p $local_port
state=""
while [ "$state" != "3: ARMED" ]; do
	echo "Waiting for local port to arm..."
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

echo "Arming Remote port."
ssh root@$remote_node "iba_portconfig -L ${remote_lid} -S3 -p $remote_port"
state=""
while [ "$state" != "3: ARMED" ]; do
	echo "Waiting for remote port to arm..."
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

sleep 1

echo "Activating local port."
iba_portconfig -S4 -p $local_port
state=""
while [ "$state" != "4: ACTIVE" ]; do
	echo "Waiting for local port to go active..."
	sleep 1
	state=$(cat /sys/class/infiniband/wfr_lite0/ports/${local_port}/state)
	echo $state
done

sleep 1

echo "Activating Remote port."
ssh root@$remote_node "iba_portconfig -S4 -p $remote_port"
state=""
while [ "$state" != "4: ACTIVE" ]; do
	echo "Waiting for remote port to go active..."
	sleep 1
	state=`ssh root@$remote_node "cat /sys/class/infiniband/wfr_lite0/ports/${remote_port}/state"`
	echo $state
done

exit 0
