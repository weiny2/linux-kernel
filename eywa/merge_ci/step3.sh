#!/bin/bash

depkg="$1"
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
                echo "build passed"
        else
                echo "build failed"
                cat $LOGFILE
                cleanup 1
	fi
}

set -e
rm -f ../*.deb
make clean
echo "Running build test"
eywa/build-test.sh OUTPUT_DIR

if [[ "$depkg" == "-y" ]] ; then 
	#use fragment to disable DBG symbols for the CI build
	echo "Building deb pkg"
	make intel_next_generic_defconfig fedora_fragments/fedora.disable_dbg.config
	LOGFILE=$(mktemp)
	echo "Starting debpkg build log file is in $LOGFILE"
	QUICKDEB_BUILD=1 make -j48 bindeb-pkg >>$LOGFILE 2>&1 || result 1
        cleanup 0
fi

#run kernel commit script
#rm -f kernelcommit
#eywa/merge-commit.sh -o kernelcommit
#cat kernelcommit
