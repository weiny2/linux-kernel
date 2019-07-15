#!/bin/bash
#Script to regenerate config files based on dcg*.config and intel*.config
#run script from kernel root under intel-next master branch
#Will check changes into repo but will not push unless -p is given
set -e
CONFIG_FOLDER="arch/x86/configs"
LOG_FILE="regen_configs.log"
RULE="==================================================================="
function create_defconfig {
#arguments $1 input_defconfig $2 output config file name to copy to git 
# $3 list of fragments to apply 
	input_config_name=$1
	output_config_name=$2
	fragment_list=$3

	if [ -f ".config" ]; then
		rm .config
	fi
	echo "Running $input_config_name $fragment_list"
	make $input_config_name $fragment_list
	echo "$output_config_name" >>$LOG_FILE
	echo $RULE >>$LOG_FILE
	scripts/diffconfig  "$CONFIG_FOLDER/$output_config_name" .config >>$LOG_FILE
	echo $RULE >> $LOG_FILE
	cp -v .config new_configs/$output_config_name
}

rm -rf new_configs
mkdir new_configs
#RUN from master
#overwrite log file if it is there
echo "regen_configs diff log " >$LOG_FILE
echo $RULE >>$LOG_FILE
#intel-next ubuntu config
create_defconfig intel_next_generic_defconfig intel_next_generic_defconfig  "intel_next_fragments/intel.*.config"
#intel-next clear config base off ubuntu with few options set to y
create_defconfig intel_next_generic_defconfig intel_next_clear_generic_defconfig  "intel_next_fragments/intel.*.config clear_fragments/clear.*.config"
#intel-next dcg rpm config  with dbg off
create_defconfig dcg_x86_64_defconfig  dcg_x86_64_defconfig "intel_next_fragments/intel.*.config dcg_fragments/dcg.*.config clear_fragments/clear.*.config" 
#intel-next ubuntu based fedora rpm config with dbg off 
create_defconfig intel_next_generic_defconfig intel_next_rpm_defconfig  "intel_next_fragments/intel.*.config fedora_fragments/fedora.*.config clear_fragments/clear.*.config"



#add new configs to repo 
git fetch
git checkout configs -f
git reset --hard origin/configs

#copy generated configs to correct folder
cp -rfv new_configs/* $CONFIG_FOLDER

#add to git
git add arch/x86/configs
git commit -s -m "Regen configs based on fragments"

echo "Done push changes with: git push origin configs:configs"
if [ "$1" == "-p" ]; then
	git push origin configs:configs
fi

#since this is ran from buildbot, just print out log file to screen 
#to save a step
cat $LOG_FILE
