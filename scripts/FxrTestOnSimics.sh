#!/bin/sh
set -x

myname=`basename $0 .sh`
tmp_dir=/tmp
test_type=default
cleanup_simics=true

. scripts/GlobalDefinition.sh

while getopts "ilhcd:t:" opt
do
	case $opt in
	i)
		only_install=true
		;;
	l)
		only_load_driver=true
		;;
	c)
		cleanup_simics=false
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
		echo "   -c     don\'t stop Simics"
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

. scripts/FxrInstall.sh

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

	# display simics console logs
	echo simics console logs
	cat ${fxr}/simics/workspace/KnightsHill0.log
	echo "----- Failed harness test(s) -----"
	grep "^\[FAIL\]" /tmp/${myname}.$$ || echo None.
fi
rm -f /tmp/${myname}.$$

if [ ${cleanup_simics} = "true" ]; then
	cleanup_simics
fi

exit $res
