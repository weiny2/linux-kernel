#!/bin/bash

GIT_COMPAT_RDMA_URL="ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-compat-rdma.git"
GIT_COMPAT_RDMA_BRANCH="alpha0"
GIT_COMPAT_URL="ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-compat.git"
GIT_COMPAT_BRANCH="alpha0"
GIT_LINUX_NEXT_URL="ssh://$USER@git-amr-2.devtools.intel.com:29418/wfr-linux-devel.git"
GIT_LINUX_NEXT_BRANCH="for-wfr-ifs-3.12-alpha0"

base_dir=${PWD}
echo $base_dir

work_dir=${base_dir}/compat
echo $work_dir
mkdir -p ${work_dir}
cd ${work_dir}

# Bring in all the working directories from their GIT homes.
if [[ ! -d wfr-compat ]]; then
	git clone --verbose --depth 1 --branch $GIT_COMPAT_BRANCH $GIT_COMPAT_URL
fi
if [[ ! -d wfr-compat-rdma ]]; then 
	git clone --verbose --branch $GIT_COMPAT_RDMA_BRANCH $GIT_COMPAT_RDMA_URL
fi
if [[ ! -d wfr-linux-devel ]]; then
	git clone --verbose --depth 1 --branch $GIT_LINUX_NEXT_BRANCH $GIT_LINUX_NEXT_URL
fi

# Kick off the compat-rdma process

export GIT_TREE="${work_dir}/wfr-linux-devel"
export GIT_COMPAT_TREE="${work_dir}/wfr-compat"

KVERSION=$(uname -r)
_kversion=$(uname -r | sed -e 's/-/_/g')
K_SRC=/lib/modules/${KVERSION}/build
LIB_MOD_DIR=/lib/modules/${KVERSION}/updates
_release=1
rpm_name=compat-rdma

echo =============================
echo "KVERSION = ${KVERSION}"
echo "K_SRC = ${K_SRC}"
echo "LIB_MOD_DIR = ${LIB_MOD_DIR}"
echo "GIT_TREE = ${GIT_TREE}"
echo "GIT_COMPAT_TREE = ${GIT_COMPAT_TREE}"
echo ==============================

rm -rf  ${work_dir}/rpmbuild

echo "Create rpmbuild base dirs..."
mkdir -p ${work_dir}/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS} 
if [[ $? != 0 ]]; then
	echo "failed to create rpmbuild dir"
	exit 1
fi

cd $work_dir
if [[ -d compat-rdma-${_kversion} ]]; then 
	rm -rf compat-rdma-${_kversion}
fi

cp -r wfr-compat-rdma compat-rdma-${_kversion}
cd compat-rdma-${_kversion}

echo "Run admin_rdma....."

./scripts/admin_rdma.sh

cd $work_dir

echo "Run tar ..."
tar czvf ${work_dir}/rpmbuild/SOURCES/compat-rdma-${_kversion}-${_release}.tgz --exclude=.git --exclude=.gitignore compat-rdma-${_kversion}

echo "Run spec copy... "
cp compat-rdma-${_kversion}/compat-rdma.spec  ${work_dir}/rpmbuild/SPECS/compat-rdma.spec

echo "Run rpmbuild ...."
conf_opts="--with-core-mod --with-ipoib-mod --with-user_mad-mod --with-user_access-mod --with-addr_trans-mod --with-hfi1-mod"

cd ${work_dir}/rpmbuild
rpmbuild -ba \
        --define "_topdir $(pwd)" \
        --target $(uname -m) \
        --define "_name ${rpm_name}" \
        --define "_prefix /usr" \
        --define "KVERSION $(uname -r)" \
        --define "build_kernel_ib 1" \
        --define "build_kernel_ib_devel 1" \
        --define "configure_options ${conf_opts}" \
        --define "_release ${_release}" \
        SPECS/${rpm_name}.spec

if [[ $? = 0 ]]; then
	echo 'DONE compat-rdma'
else
	echo "Fail compat-rdma"
fi

cd $base_dir


