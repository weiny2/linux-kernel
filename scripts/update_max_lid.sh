#!/bin/bash
# Script to enable and update the maximum LID field in the FM config file
# Takes a hostname of the FM node.
#Assumes that the fm config file is always present in /etc/sysconfig/ in the FM node
conf_file=/etc/sysconfig/opafm.xml
max_lid=0xEFFFFF
MaximumLID=`ssh root@$1 "grep \"<MaximumLID\" $conf_file"`
MaximumLID=`echo $MaximumLID | sed 's/<!--.*-->//g'`

#An empty string here means we dont have a MaximumLID field or it was
#commented out.
#If there was already a valid MaximumLID field, the script leaves it alone
if [[ -z $MaximumLID ]];then
	line_number=`ssh root@$1 "awk '/<Sm>/{ print NR; exit }' $conf_file"`
	maxLID="\ \ \ \ <MaximumLID>$max_lid</MaximumLID>"
	ssh root@$1 "sed -i \"$line_number a $maxLID\" $conf_file"
	MaximumLID=`ssh root@$1 "grep \"<MaximumLID\" $conf_file"`
	MaximumLID=`echo $MaximumLID | sed 's/<!--.*-->//g'`
fi
