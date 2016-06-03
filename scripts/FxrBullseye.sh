#!/bin/bash
set -x

. scripts/GlobalDefinition.sh
. scripts/SimicsDefinition.sh
. scripts/bullseye_define.sh

# stop FM to prepare unloading the driver
stop_fm() {
	SSH_CMD="ssh -p${viper0} root@localhost"
	${SSH_CMD} "service --skip-redirect opafm stop"
}

# wrap up bullseye
wrapup_bullseye() {
	for viper in ${viper0} ${viper1}; do
		SSH_CMD="ssh -p${viper} root@localhost"
		${SSH_CMD} "
			export COVFILE=${COVFILE}; ${BULLSEYE_DIR}/bin/covgetkernel
			service --skip-redirect opa2_hfi stop
			rmmod libcov-lkm
			${BULLSEYE_DIR}/bin/cov01 -0
		"
	done
}

# error exit
error_exit() {
	# stop FM to prepare unloading the driver
	stop_fm
	wrapup_bullseye
	cleanup_simics
	exit $1
}

# main start here
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

for viper in ${viper0} ${viper1}; do
	SSH_CMD="ssh -p${viper} root@localhost"
	# update date and time to eliminate the warning, "..has modification time ...in the future"
	${SSH_CMD} "date `date +%m%d%H%M%y`"

	# clean up
	rm -rf cov.test /tmp/fxr-driver
	${SSH_CMD} "
		rm -rf ${BULLSEYE_WORK}
		cd /opt; rm -rf BullseyeCoverage
		rm -rf .BullseyeCoverage
		dmesg -C; # clean up dmesg
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
			--exclude="cscope.out" \
			${CURRENT_DIR} | \
			${SSH_CMD} "cd ${HOME_DIR}; tar xf -" 2>/dev/null
	popd

	# build and install bullseye kernel module and driver
	${SSH_CMD} "\
		sed -i -e'/^export COVFILE=/d' /root/.bashrc
		echo export COVFILE=${COVFILE} >>/root/.bashrc
		export COVFILE=${COVFILE}
		cd ${BULLSEYE_DIR}/run/linuxKernel
		make -C /lib/modules/\`uname -r\`/build M=\`pwd\`
		insmod ${BULLSEYE_DIR}/run/linuxKernel/libcov-lkm.ko
		grep -q cov_ /lib/modules/\`uname -r\`/build/Module.symvers ||
			cat Module.symvers >>/lib/modules/\`uname -r\`/build/Module.symvers
		cd ${BULLSEYE_WORK}/opa-headers.git
		./autogen.sh
		./configure
		echo '--symbolic' >${BULLSEYE_DIR}/bin/covc.cfg
		${BULLSEYE_DIR}/bin/covselect -q --deleteAll
		${BULLSEYE_DIR}/bin/covselect -q --add ./
		${BULLSEYE_DIR}/bin/covselect -q --add '!hfi2_vnic/vnic.git/opa_vnic/opa_vnic_debug.c'
		${BULLSEYE_DIR}/bin/covselect -q --add '!opa-headers.git/test/'
		${BULLSEYE_DIR}/bin/covselect -q --add '!kfi/kfi_main.c'
		${BULLSEYE_DIR}/bin/covselect -q --add '!hfi2/verbs/rdmavt/'
		${BULLSEYE_DIR}/bin/covselect -q --add '!hfi2/snoop.c'
		${BULLSEYE_DIR}/bin/cov01 -1
		export PATH=${BULLSEYE_DIR}/bin:${PATH}
		cd ${BULLSEYE_WORK}
		make -C /lib/modules/\`uname -r\`/build M=\`pwd\` || exit \$?
		cp \`find . -name "*.ko"\` /lib/modules/\`uname -r\`/updates
		make -C opa-headers.git || exit \$?
		make -C opa-headers.git install || exit \$?
		exit 0
	"
	res=$?
	[ ${res} != 0 ] && break
done
if [ ${res} != 0 ]; then
	error_exit ${res}
fi

# run harness
pushd opa-headers.git/test
./harness.py --nodelist=viper0,viper1 --type=${test_type}
res=$?
popd
if [ ${res} != 0 ]; then
	error_exit ${res}
fi

# stop FM to prepare unloading the driver
stop_fm

# wrap up bullseye
wrapup_bullseye

# transfer to host
for viper in ${viper0} ${viper1}; do
	SSH_CMD="ssh -p${viper} root@localhost"
	${SSH_CMD} "cat ${COVFILE}" >cov.${test_type}.${viper}
done

# merge 2 covfiles into one, .e.g. cov.quick.4022 cov.quick.5022 -> cov.quick
export COVFILE=`pwd`/cov.${test_type}
${BULLSEYE_DIR}/bin/covmerge --create cov.${test_type}.*
rm -f cov.${test_type}.*

# to view code coverage result
# export COVFILE=`pwd`/cov.${test_type}
# ${BULLSEYE_DIR}/bin/CoverageBrowser

cleanup_simics

exit ${res}
