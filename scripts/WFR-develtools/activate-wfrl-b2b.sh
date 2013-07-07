#!/bin/sh

# load ib_wfr_lite driver
# Use infiniband-diags tools to activate back to back ports with assigned lids
# Finally configure both drivers for VL15 over VL0

# for now hardcode all these.
# change these for your environment
local_port=2
local_lid=1

remote_node=phbppriv03
remote_port=2
remote_lid=2
# </change>

echo "Reloading remote ib_wfr_lite"
ssh root@$remote_node "rmmod ib_qib"
ssh root@$remote_node "rmmod ib_wfr_lite"
ssh root@$remote_node "rmmod ib_umad"
ssh root@$remote_node "rmmod ib_mad"
ssh root@$remote_node "modprobe ib_wfr_lite"
ssh root@$remote_node "modprobe ib_umad"

echo "Waiting for port to Initialize..."
state=""
while [ "$state" != "State: Initializing" ]; do
	sleep 1
	state=`ssh root@$remote_node "ibstat wfr_lite0 $remote_port | grep State:"`
	echo $state
done

echo "Reloading local ib_wfr_lite"
rmmod ib_qib
rmmod ib_wfr_lite
rmmod ib_umad
rmmod ib_mad
modprobe ib_wfr_lite
modprobe ib_umad

echo "Waiting for ports to Initialize..."
state=""
while [ "$state" != "State: Initializing" ]; do
	sleep 1
	state=`ibstat wfr_lite0 $local_port | grep State:`
	echo $state
done

echo "Bringing up ports: local $local_port <==> remote $remote_port"
ibportstate -D 0 $local_port lid $local_lid arm > /dev/null
ibportstate -D 0,$local_port $remote_port lid $remote_lid arm > /dev/null
ibportstate -D 0 $local_port active > /dev/null
ibportstate -D 0,$local_port $remote_port active > /dev/null

echo "configuring wfr_lite for VL15 over VL0"
ssh root@$remote_node "echo $local_lid > /sys/module/ib_wfr_lite/parameters/vl15_ovl0"
echo $remote_lid > /sys/module/ib_wfr_lite/parameters/vl15_ovl0

