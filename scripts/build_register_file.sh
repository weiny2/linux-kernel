#!/bin/bash

# You probably do not want to run this. If you don't know what you are doing
# then STOP HERE.

echo "Really want to run this script? You probably want refresh_register_file.sh"
sleep 30

# This script builds the register header file.
echo "Removing old chip register file"
rm -f ../chip_registers.h

echo "Building list of all possible registers"
rm -f register_val_list.txt
rm -f register_name_list.txt
rm -f register_used.txt
rm -f register_unused.txt
awk '/#define/ {if (NF > 2) { $1=""; print $0}}' ../include/wfr/* > register_val_list.txt
awk '/#define/ {if (NF > 2) { $1=""; print $2}}' ../include/wfr/* > register_name_list.txt

echo "Generating unique list of used registers"
rm -f register_uniq.txt
cat register_name_list.txt | sort | uniq > register_uniq.txt

echo "Removing base registers so we don't get duplicate entries"
sed -i '/^WFR_CORE$/d' register_uniq.txt
sed -i '/^WFR_CCE$/d' register_uniq.txt
sed -i '/^WFR_ASIC$/d' register_uniq.txt
sed -i '/^WFR_MISC$/d' register_uniq.txt
sed -i '/^DC_TOP_CSRS$/d' register_uniq.txt
sed -i '/^WFR_DEBUG$/d' register_uniq.txt
sed -i '/^WFR_RXE$/d' register_uniq.txt
sed -i '/^WFR_TXE$/d' register_uniq.txt
sed -i '/^DCC_CSRS$/d' register_uniq.txt
sed -i '/^DC_LCB_CSRS$/d' register_uniq.txt
sed -i '/^DC_8051_CSRS$/d' register_uniq.txt
sed -i '/^WFR_PCIE$/d' register_uniq.txt

echo "Pre-processing files to handle any macros"
for file in $(ls ../*.[ch]); do
	echo "Preprocessing $file..."
	gcc -E $file > $file.prepro 2>/dev/null
done

echo "Limiting register set to those used"
lines=`cat register_uniq.txt | wc -l`
count=0
for reg in `cat register_uniq.txt`; do
	found=0
	count=$((count+1))
	should_print=`echo "$count%500" | bc`
	if [[ $should_print -eq 0 ]]; then
		echo "($count / $lines)"
	fi
	for file in $(ls ../*.prepro); do
		grep $reg $file > /dev/null
		if [ $? -eq 0 ]; then
			echo $reg >> register_used.txt
			#echo "$reg first used in $file"
			found=1
			break
		fi
	done

	if [ $found -eq 0 ]; then
		echo $reg >> register_unused.txt
	fi
done

echo "Removing pre-processed files"
rm -f ../*.prepro

echo "Generating minimal register/value set"
rm -f register_minimal_set.txt
for reg in `cat register_used.txt`; do 
	egrep "^ $reg\s+" register_val_list.txt >> register_minimal_set.txt; 
done

./refresh_register_file.sh

