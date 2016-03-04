#!/bin/bash
set -x

. scripts/GlobalDefinition.sh
. scripts/SimicsDefinition.sh
. scripts/bullseye_define.sh

myname=$0
PWD=`pwd`
CURRENT_DIR=`basename ${PWD}`
BULLSEYE_WORK=${HOME_DIR}/${CURRENT_DIR}

test_type=default
if [ $# -gt 0 ]; then
	test_type=$1
fi
COVFILE=${BULLSEYE_WORK}/cov.${test_type}

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
git submodule update
pushd ..
tar cf - \
	--exclude=".*" \
	--exclude="rpmbuild" \
	--exclude="scripts" \
	--exclude="opa2_hfi*" \
	--exclude="cov.*" \
	${CURRENT_DIR} | \
	${SSH_CMD} "cd ${HOME_DIR}; tar xf -" 2>/dev/null
popd

# build and install bullseye kernel module and driver
${SSH_CMD} "\
	sed -i -e'/^export COVFILE=/d' /root/.bashrc; \
	echo export COVFILE=${COVFILE} >>/root/.bashrc
	export COVFILE=${COVFILE}; \
	${BULLSEYE_DIR}/bin/cov01 -1; \
	${BULLSEYE_DIR}/bin/covselect -q --deleteAll; \
	${BULLSEYE_DIR}/bin/covselect -q --add ./; \
	${BULLSEYE_DIR}/bin/covselect -q --add '!hfi2_vnic/vnic.git/opa_vnic/opa_vnic_debug.c'; \
	${BULLSEYE_DIR}/bin/covselect -q --add '!opa-headers.git/test/'
	${BULLSEYE_DIR}/bin/covselect -q --add '!kfi/kfi_main.c'
	${BULLSEYE_DIR}/bin/covselect -q --add '!hfi2/snoop.c'
	cd ${BULLSEYE_DIR}/run/linuxKernel; \
	make -C /lib/modules/\`uname -r\`/build M=\`pwd\`; \
	insmod ${BULLSEYE_DIR}/run/linuxKernel/libcov-lkm.ko; \
	grep -q cov_ /lib/modules/\`uname -r\`/build/Module.symvers || \
		cat Module.symvers >>/lib/modules/\`uname -r\`/build/Module.symvers; \
	cd ${BULLSEYE_WORK}/opa-headers.git; \
	./autogen.sh; \
	./configure; \
	echo '--symbolic' >${BULLSEYE_DIR}/bin/covc.cfg
	export PATH=${BULLSEYE_DIR}/bin:${PATH}; \
	cd ${BULLSEYE_WORK}; \
	make -C /lib/modules/\`uname -r\`/build M=\`pwd\`; \
	cp \`find . -name "*.ko"\` /lib/modules/\`uname -r\`/updates; \
	make -C opa-headers.git; \
	make -C opa-headers.git install; \
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
${SSH_CMD} "cat ${COVFILE}" >cov.${test_type}

# to view code coverage result
# export COVFILE=`pwd`/cov.${test_type}
# ${BULLSEYE_DIR}/bin/CoverageBrowser

cleanup_simics
