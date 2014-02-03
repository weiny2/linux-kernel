#!/bin/bash

DEFAULT_KERNEL_VERSION="3.9.2-wfr+"
DEFAULT_USER=$USER
DEFAULT_URL_PREFIX="ssh://"
DEFAULT_URL_SUFFIX="@git-amr-2.devtools.intel.com:29418/wfr-linux-devel"
DEFAULT_BRANCH="wfr-for-ifs"

# NOTE: qib, mlx4, mthca, and ib_cm are not needed for STL
#       However, they have changes[*] within them we are actively trying to get
#       upstream.  As soon as these changes are accepted we should be able to
#       drop those modules.
#       [*] new ioctl registration and URMPP flags
sources_to_copy="
	scripts/WFR-develtools/activate-wfrl-b2b.sh
	scripts/WFR-develtools/umad-trace.stp
	drivers/infiniband/core
	drivers/infiniband/ulp/ipoib
	drivers/infiniband/hw/wfr-lite
	include/rdma
	include/uapi/rdma
	drivers/infiniband/hw/qib
	drivers/infiniband/hw/mthca
	drivers/infiniband/hw/mlx4
"

# ridiculously long to encourage good names later
rpmname="ifs-kernel-updates"
rpmversion="1" # this is appended to later to make the complete version string

set -e

function usage
{
	cat <<EOL
usage:
	${0##*/} -h
	${0##*/} [-G] [-b branch] [-w dirname] [-u user] [-U URL]
	${0##*/} -S srcdir [-w dirname]

Options:

-G         - fetch source from a git repository [DEFAULT]
-S srcdir  - fetch source directly from a specified directory

-b branch  - branch of URL to check out [$DEFAULT_BRANCH]
-w dirname - work directory, defaults to a mktemp directory
-h         - this help text
-u user    - user for the defaut URL [$DEFAULT_USER]
-U URL	   - source checkout url [$DEFAULT_URL_PREFIX$USER$DEFAULT_URL_SUFFIX]
EOL
}

gitfetch="maybe"
srcdir=""
workdir=""
url=""
branch=$DEFAULT_BRANCH
user=$DEFAULT_USER
while getopts "GS:U:b:hu:w:" opt; do
    	case "$opt" in
	G)	[ "$gitfetch" = "false" ] && usage && exit 1
		gitfetch="true"
		;;
	S)	[ "$gitfetch" = "true" ] && usage && exit 1
		srcdir="$OPTARG"
		gitfetch="false"
		[ ! -e "$srcdir" ] && echo "srcdir $srcdir not found" && exit 1
		srcdir=$(readlink -f "$srcdir")
		;;
	U)	[ "$gitfetch" = "false" ] && usage && exit 1
		url="$OPTARG"
		gitfetch="true"
		;;
	b)	[ "$gitfetch" = "false" ] && usage && exit 1
		branch="$OPTARG"
		gitfetch="true"
		;;
	h)	usage
		exit 0
		;;
	u)	[ "$gitfetch" = "false" ] && usage && exit 1
		user="$OPTARG"
		gitfetch="true"
		;;
	w)	workdir="$OPTARG"
		;;

    	esac
done

# create final version of the variables
if [ -n "$workdir" ]; then
	mkdir -p "$workdir" || exit 1
else
	workdir=$(mktemp -d --tmpdir=$(pwd) build.XXXX)
	[ ! $? ] && exit 1
fi
[ -z "$url" ] && url="$DEFAULT_URL_PREFIX$user$DEFAULT_URL_SUFFIX"

# after cd, where are we *really*
cd -P "$workdir"; workdir=$(pwd)

echo "Working in $workdir"

# check out or copy our sources
if [ "$gitfetch" != "false" ]; then
	rm -rf ksrc
	echo "Checking out source"
	time git clone --branch $branch $url ksrc
	srcdir="ksrc"
else
	rm -rf ksrc; mkdir -p ksrc
	echo "Copying source from $srcdir"
	(cd "$srcdir"; tar cf - ${sources_to_copy}) | (cd ksrc; tar xf -)
fi
rpmversion="$rpmversion".$(cd "$srcdir"; git rev-list "v3.9.2^..HEAD" | wc -l)

echo "Setting up RPM build area"
mkdir -p rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# patch Makefile to use local include files first
# kind of a hack, perfect thing to put in SOURCES as a real patch
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../include -I\$(M)/../../../include/uapi' >> ksrc/drivers/infiniband/core/Makefile
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../../include -I\$(M)/../../../../include/uapi' >> ksrc/drivers/infiniband/hw/wfr-lite/Makefile

# See NOTE above under sources to copy.
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../../include -I\$(M)/../../../../include/uapi' >> ksrc/drivers/infiniband/hw/qib/Makefile
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../../include -I\$(M)/../../../../include/uapi' >> ksrc/drivers/infiniband/hw/mlx4/Makefile
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../../include -I\$(M)/../../../../include/uapi' >> ksrc/drivers/infiniband/hw/mthca/Makefile


# make sure rpm component strings are clean, should be no-ops
rpmname=$(echo "$rpmname" | sed -e 's/[.]/_/g')
rpmversion=$(echo "$rpmversion" | sed -e 's/-/_/g')
rpmrelease=$(echo "$DEFAULT_KERNEL_VERSION" | sed -e 's/-/_/g')

# build the tarball
echo "Building tar file"
(cd ksrc; tar cfz - --transform="s,^,${rpmname}-${rpmversion}/," ${sources_to_copy}) > \
	rpmbuild/SOURCES/$rpmname-$rpmversion-$rpmrelease.tgz

# create the spec file
echo "Creating spec file"
cat > rpmbuild/SPECS/$rpmname.spec <<EOF
Name:           $rpmname
Group:		System Environment/Kernel
Summary:        Extra kernel modules for IFS
Version:        $rpmversion
Release:        $rpmrelease
License:        GPL v2
Source:         %{name}-%{version}-%{release}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root
Requires:	kernel >= $rpmrelease

# find the number of cpus on this system
%global num_cpus %(
which nproc > /dev/null
if [ "$?" == "0" ]; then
	nproc
else
	echo "1"
fi
)

# find our target version
%global kbuild %(
if [ -z "\$kbuild" ]; then 
	echo "/lib/modules/$DEFAULT_KERNEL_VERSION/build"
else 
	echo "\$kbuild"
fi
)

%global kver %(
if [ -f "%{kbuild}/include/config/kernel.release" ]; then
	cat %{kbuild}/include/config/kernel.release
else
	echo "fail"
fi
)


%description
Updated kernel modules for STL IFS

%prep
%setup -q

%build
if [ "%kver" = "fail" ]; then
        if [ -z "%kbuild" ]; then
                echo "The default target kernel, $DEFAULT_KERNEL_VERSION, is not installed" >&2
                echo "To build, set \\\$kbuild to your target kernel build directory" >&2
        else
                echo "Cannot find kernel version in %kbuild" >&2
        fi
        exit 1
fi
echo "Kernel version is %kver"
echo "Kernel source directory is \"%kbuild\""

# Build Core support first...
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_mad.ko
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_umad.ko
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_sa.ko
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_usa.ko

# NOTE: the following not are required for STL but we are carrying patches for them
#       which require a rebuild
#       See NOTE in build script under sources to copy
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_cm.ko


# Then build drivers...
cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/hw/wfr-lite
export CONFIG_INFINIBAND_WFR_LITE=m; make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/hw/wfr-lite

# NOTE: the following not are required for STL but we are carrying patches for them
#       which require a rebuild
#       See NOTE in build script under sources to copy
cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/hw/qib
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/hw/qib
cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/hw/mlx4
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/hw/mlx4
cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/hw/mthca
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/hw/mthca
cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/ulp/ipoib
make -j %num_cpus -C %kbuild M=\$(pwd)/drivers/infiniband/ulp/ipoib


%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/lib/modules/%kver/updates
mkdir -p \$RPM_BUILD_ROOT/sbin

cp scripts/WFR-develtools/activate-wfrl-b2b.sh \$RPM_BUILD_ROOT/sbin
cp scripts/WFR-develtools/umad-trace.stp \$RPM_BUILD_ROOT/sbin
cp drivers/infiniband/core/ib_mad.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/core/ib_umad.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/core/ib_sa.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/core/ib_usa.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/hw/wfr-lite/ib_wfr_lite.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates

# See NOTE above
cp drivers/infiniband/core/ib_cm.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/hw/qib/ib_qib.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/hw/mlx4/mlx4_ib.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/hw/mthca/ib_mthca.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates
cp drivers/infiniband/ulp/ipoib/ib_ipoib.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates

%post
depmod -a

%postun
depmod -a

%clean
rm -rf \${RPM_BUILD_ROOT}

%files
%defattr(-, root, root)
%dir /lib/modules/%kver/updates
/lib/modules/%kver/updates/*
%dir /sbin
/sbin/*
EOF

# moment of truth, run rpmbuild
rm -rf ksrc
echo "Building SRPM"
cd rpmbuild
rpmbuild -bs --define "_topdir $(pwd)" SPECS/${rpmname}.spec

exit 0
