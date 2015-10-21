#!/bin/bash

# Display performance data for a given date in the format of
# MM-DD-YY

if [[ -z $1 ]]; then
	date=`date "+%m-%d-%y"`
else
	date=$1
fi

base_dir="/nfs/sc/disks/fabric_mirror/scratch/wfr_reg_test/daily_logs/RHEL7"

mpi_dir="mpi_perf.$date/no-turbo/summary"
ipostl_dir="ipostl_perf.$date/ipostl-summary.txt"
verbs_dir="ibverbs.$date/VerbsLZSummary.csv"

echo Base dir: $base_dir
echo mpi: $mpi_dir
echo ipostl: $ipostl_dir
echo verbs: $verbs_dir
echo ""
echo ""
cat $base_dir/$mpi_dir
echo ""
cat $base_dir/$ipostl_dir
echo ""
cat $base_dir/$verbs_dir
