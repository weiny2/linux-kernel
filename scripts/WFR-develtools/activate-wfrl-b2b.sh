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

function usage
{
	cat <<EOL
usage:
	${0##*/} [-h] [-p <local_port>] [-r <remote_port>] -n <remote_node>

Options:

-h         - this help text
-p port    - local port to operate on (default $local_port)
-r port    - remote port to operate on (default $remote_port)
-i         - ignore VL15 over VL0 set up (run in default (i)B mode)
-n node    - remote node to operate on
EOL
}
while getopts "hr:p:n:i" opt; do
	case "$opt" in
	h)	usage
		exit 0
		;;
	r)	remote_port="$OPTARG"
		;;
	p)	local_port="$OPTARG"
		;;
	n)	remote_node="$OPTARG"
		;;
	i)	ibmode="true"
		;;

	esac
done

if [ "$remote_node" == "" ]; then
	echo "ERROR: remote node must be specified"
	usage
	exit 1
fi

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

echo "Waiting for port to Initialize..."
state=""
while [ "$state" != "State: Initializing" ]; do
	sleep 1
	state=`ssh root@$remote_node "ibstat wfr_lite0 $remote_port | grep State:"`
	echo $state
done

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

echo "Waiting for ports to Initialize..."
state=""
while [ "$state" != "State: Initializing" ]; do
	sleep 1
	state=`ibstat wfr_lite0 $local_port | grep State:`
	echo $state
done

if [ "$ibmode" == "false" ]; then
	echo "Bringing up ports: local $local_port <==> remote $remote_port"
	ibportstate -D 0 $local_port lid $local_lid arm > /dev/null
	ibportstate -D 0,$local_port $remote_port lid $remote_lid arm > /dev/null
	ibportstate -D 0 $local_port active > /dev/null
	ibportstate -D 0,$local_port $remote_port active > /dev/null

	echo "configuring wfr_lite for VL15 over VL0"
	ssh root@$remote_node "echo $local_lid > /sys/module/ib_wfr_lite/parameters/vl15_ovl0"
	echo $remote_lid > /sys/module/ib_wfr_lite/parameters/vl15_ovl0
fi

exit 0
