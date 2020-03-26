#!/bin/sh

rule()
{
	echo "------------------------------------------------------------------------"
}

CPUS=$(nproc)
LOGFILE=$(mktemp)
BUILDDIR=$1
SKIP_ALLMOD=false
SKIP_ALLYES=false
if [ "$2" = "allyesconfig" ]; then
     SKIP_ALLMOD=true
     echo "skipping allmodconfig"
fi

if [ "$2" = "allmodconfig" ]; then
     SKIP_ALLYES=true
     echo "skipping allyesconfig"
fi


cleanup() {
	date
	if [ $1 -eq 1 ]; then
		echo "Log file available at $LOGFILE"
	else
		echo "Removing $LOGFILE ..."
		rm $LOGFILE
	fi
	exit $1
}

result() {
	if [ $1 -eq 0 ]; then
		echo "pass"
		echo "Warnings:"
		grep "warning:" $LOGFILE
	else
		echo "fail"
		cat $LOGFILE
		cleanup 1
	fi
}

allyesconfig() {
	echo -n "Building with allyesconfig ... "
	rule >> $LOGFILE
	echo "allyesconfig" >> $LOGFILE
	make mrproper >>$LOGFILE 2>&1 || result 1
	make $BUILDDIR allyesconfig >>$LOGFILE 2>&1 || result 1
        make $BUILDDIR build_test_exemptions/intel.disable.*.config >> $LOGFILE 2>&1 || result 1
	make $BUILDDIR syncconfig >>$LOGFILE 2>&1 || result 1
	make -j $CPUS $BUILDDIR >>$LOGFILE 2>&1 || result 1
	result $?
}

allmodconfig() {
	echo -n "Building with allmodconfig ..."
	rule >> $LOGFILE
	echo "allmodconfig" >> $LOGFILE
	make mrproper >>$LOGFILE 2>&1 || result 1
	make $BUILDDIR allmodconfig >>$LOGFILE 2>&1 || result 1
        make $BUILDDIR build_test_exemptions/intel.disable.*.config >> $LOGFILE 2>&1 || result 1
	make $BUILDDIR syncconfig >>$LOGFILE 2>&1 || result 1
	make -j $CPUS $BUILDDIR >>$LOGFILE 2>&1 || result 1
	make -j $CPUS $BUILDDIR  modules >>$LOGFILE 2>&1 || result 1
	result $?
}

if [ "z$BUILDDIR" != "z" ]; then
	echo "Will send build output to $BUILDDIR"
	echo -n "Creating $BUILDDIR if it does not exist ..."
	mkdir -p $BUILDDIR || result 1
	echo "success"
	BUILDDIR=" O=$BUILDDIR "
fi

echo "Build test...(can be followed in $LOGFILE)"
if [ "$SKIP_ALLYES" = false ]; then
	date
	allyesconfig
fi
if [ "$SKIP_ALLMOD" = false ]; then
	date
	allmodconfig
fi
echo "ALL TESTS PASSED"
cleanup 0
