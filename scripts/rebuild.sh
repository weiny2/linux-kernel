#!/bin/sh
s=$PWD/$1
p=$PWD/$2
rpm_name=compat-rdma
conf_opts="--with-core-mod --with-ipoib-mod --with-user_mad-mod --with-user_access-mod --with-addr_trans-mod --with-hfi1-mod"
rm -rf compat
mkdir compat
cd compat
topdir=$PWD
rpm -ivh --define "_topdir $(pwd)" $s
srpm=`basename $s`
_release=${srpm%.src.rpm}
_release=${_release##compat-*-*-}
if [ -n "$2" ]
then
	cd $topdir/SOURCES
	tar -zxf compat-rdma-3.10.0_123.el7.x86_64-${_release}.tgz
	cd compat-rdma-3.10.0_123.el7.x86_64/drivers/infiniband/hw/hfi1
	patch < $p
	sed -i 's/#define HFI_DRIVER_VERSION_BASE "\(.*\)"/#define HFI_DRIVER_VERSION_BASE "\1-SB"/' common.h
	cd $topdir/SOURCES
	tar -zcf compat-rdma-3.10.0_123.el7.x86_64-${_release}.tgz compat-rdma-3.10.0_123.el7.x86_64
	rm -rf compat-rdma-3.10.0_123.el7.x86_64
	cd $topdir
fi
rpmbuild -ba --noclean \
	--define "_topdir $(pwd)" \
        --target $(uname -m) \
        --define "_name ${rpm_name}" \
        --define "_prefix /usr" \
        --define "KVERSION $(uname -r)" \
        --define "build_kernel_ib 1" \
        --define "build_kernel_ib_devel 1" \
        --define "configure_options ${conf_opts}" \
        --define "_release ${_release}" \
	SPECS/compat-rdma.spec
