#!/bin/sh
set -x

myname=`basename $0 .sh`
tmp_dir=/tmp
test_type=default

. scripts/GlobalDefinition.sh

while getopts "ilhd:t:" opt
do
	case $opt in
	i)
		only_install=true
		;;
	l)
		only_load_driver=true
		;;
	d)
		if !(ssh -p${viper0} root@localhost "[ -d ${OPTARG} ]"); then
			echo "Directory $OPTARG does not exist"
			exit 1;
		fi
		tmp_dir=$OPTARG
		;;
	t)
		test_type=$OPTARG
		;;
	h)
		set +x
		echo "Usage: `basename $0` [-i] [-l] [-d <path to temp directory>] [-t <test_type argument>"
		echo "By Default this script installs the necessary driver"
		echo "and runs the harness test suite(default) "
		echo "   -i     Only installs the appropriate drivers"
		echo "          built in rpmbuild"
		echo "   -l     Installs and only runs ModuleLoad test"
		echo "          (When -l option is used -i is ignored)"
		echo "   -d     Specify path to temporary directory in viperx host"
		echo "          to copy and extract drivers"
		echo "   -t     test_type argument sent to harness.py"
		exit 0
		;;
	\?)
		echo "Invalid option: -$OPTARG"
		exit 1
		;;
	esac
done
shift $(($OPTIND - 1))

# Am I invoked by Jenkins?
ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes

. scripts/StartSimics.sh
. scripts/WaitSimics.sh

# show which version/commit of Simics, craff file and others I am using
pwd; git log -n1 | head -1
${fxr}/simics/workspace/bin/simics --version
( cd ${fxr}/simics/workspace/; git log -n1 | head -1 )
ls -l ${fxr}/simics/FxrRhel7.craff
ls -l ${fxr}/simics/SynopsisInstructionSetSimulator

ssh -p${viper0} root@localhost "service --skip-redirect opafm stop"

for viper in ${viper0} ${viper1}; do
    ssh_cmd="ssh -p${viper} root@localhost"
    scp_cmd="scp -P${viper}"

    ${ssh_cmd} "dmesg -c > /dev/null"
    # stop opa2_hfi daemon to release the driver
    ${ssh_cmd} "service --skip-redirect opa2_hfi stop"
    if [ ! $? ]; then
	echo fail on stoping opa2_hfi.
	exit 12
    fi
    # update the driver
    ${ssh_cmd} "rm -f ${tmp_dir}/opa*.x86_64.rpm"
    ${scp_cmd} \
	rpmbuild/RPMS/x86_64/opa2_hfi-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
	opa-headers.git/opa-headers-[0-9]*.[0-9]*-[0-9]*.x86_64.rpm \
	root@localhost:${tmp_dir}
    if [ ! $? ]; then
	echo fail on copying rpm files.
	exit 13
    fi
    ${ssh_cmd} "rpm -e opa2_hfi" 2>/dev/null
    ${ssh_cmd} "rpm -e opa-headers" 2>/dev/null
    ${ssh_cmd} "rpm -i ${tmp_dir}/opa*.x86_64.rpm"
    if [ ! $? ]; then
	echo fail on the installation of rpm files.
	exit 14
    fi
done

if [ $only_install ] && ! [ $only_load_driver ]; then
	exit 0
fi

if [ $only_load_driver ]; then
	cd opa-headers.git/test
	./harness.py --nodelist=viper0,viper1 --testlist=ModuleLoad
	exit 0
fi

# run default tests
cd opa-headers.git/test
./harness.py --nodelist=viper0,viper1 --type=${test_type} | tee /tmp/${myname}.$$
res=$?
if [ ! ${res} ]; then
    echo fail on harness.
fi
./harness.py --nodelist=viper0,viper1 --testlist=ModuleLoad | tee -a /tmp/${myname}.$$
res=$?
if [ ! ${res} ]; then
    echo fail on ModuleReload of harness.
fi

set +x
if [ ${ByJenkins} == yes ] ; then
	for viper in ${viper0} ${viper1}; do
		ssh_cmd="ssh -p${viper} root@localhost"

		echo dmesg on viper with port ${viper} start
		${ssh_cmd} "dmesg -c"
		echo dmesg on viper with port ${viper} end
	done
	# stop simics
	killall simics-common

	# display simics console logs
	echo simics console logs
	cat ${fxr}/simics/workspace/KnightsHill0.log
	echo "----- Failed harness test(s) -----"
	grep "^\[FAIL\]" /tmp/${myname}.$$ || echo None.

	cleanup_lock
fi
rm -f /tmp/${myname}.$$

exit $res
