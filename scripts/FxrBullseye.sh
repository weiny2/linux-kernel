#!/bin/bash
set -x

. scripts/GlobalDefinition.sh
. scripts/bullseye_define.sh

myname=$0
PWD=`pwd`
CURRENT_DIR=`basename ${PWD}`
BULLSEYE_WORK=${HOME_DIR}/${CURRENT_DIR}
COVFILE=${BULLSEYE_WORK}/cov.test

test_type=quick
if [ $# -gt 0 ]; then
	test_type=$1
fi

# Am I invoked by Jenkins?
export ByJenkins=no
[ `id -un` == root ] && pwd | grep --quiet jenkins && ByJenkins=yes

. scripts/FxrInstall.sh

# update date and time to eliminate the warning, "..has modification time ...in the future"
${SSH_CMD} "date `date +%m%d%H%M%y`"

# clean up
rm -rf cov.test /tmp/fxr-driver
${SSH_CMD} "\
	rm -rf ${BULLSEYE_WORK}; \
	cd /opt; rm -rf BullseyeCoverage; \
	rm -rf .BullseyeCoverage; \
	dmesg -C; # clean up dmesg \
"

# make sure to unload bullseye kernel module
${SSH_CMD} "if lsmod | grep -q cov; then rmmod libcov_lkm; fi"

# copy /opt/BullseyeCoverage
pushd ${BULLSEYE_SRC_DIR}
tar cf - BullseyeCoverage | ${SSH_CMD} "cd /opt; tar xf -" 2>/dev/null
popd

# transfer driver source code.
git clean -xfd # remove garbage
git submodule update
pushd ..
tar cf - --exclude=".*" ${CURRENT_DIR} | ${SSH_CMD} "cd ${HOME_DIR}; tar xf -" 2>/dev/null
popd

# build and install bullseye kernel module and driver
${SSH_CMD} "\
	${BULLSEYE_DIR}/bin/cov01 -1; \
	export COVFILE=${COVFILE}; \
	cd ${BULLSEYE_DIR}/run/linuxKernel; \
	make -C /lib/modules/\`uname -r\`/build M=\`pwd\`; \
	insmod ${BULLSEYE_DIR}/run/linuxKernel/libcov-lkm.ko; \
	grep -q cov_ /lib/modules/\`uname -r\`/build/Module.symvers || \
		cat Module.symvers >>/lib/modules/\`uname -r\`/build/Module.symvers; \
	export PATH=${BULLSEYE_DIR}/bin:${PATH}; \
	cd ${BULLSEYE_WORK}; \
	make -C /lib/modules/\`uname -r\`/build M=\`pwd\`; \
	cp \`find . -name "*.ko"\` /lib/modules/\`uname -r\`/updates; \
"

# run harness
pushd opa-headers.git/test
./harness.py --nodelist=viper0,viper1 --type=${test_type}
popd

# wrap up bullseye
${SSH_CMD} "\
	service --skip-redirect opafm stop; \
	export COVFILE=${COVFILE}; ${BULLSEYE_DIR}/bin/covgetkernel; \
	service --skip-redirect opa2_hfi stop; \
	rmmod libcov-lkm; \
	${BULLSEYE_DIR}/bin/cov01 -0; \
"

# transfer to host
${SSH_CMD} "cd ${HOME_DIR}; tar cf - ${CURRENT_DIR}" | (cd /tmp; tar xf -)
${SSH_CMD} "cat ${COVFILE}" >cov.test

# to view code coverage result
# export COVFILE=`pwd`/cov.test
# ${BULLSEYE_DIR}/bin/CoverageBrowser

# epilogue
if [ ${ByJenkins} == yes ] ; then
	# stop simics
	killall simics-common

	rm -f ${LOCK_FILE}
fi
