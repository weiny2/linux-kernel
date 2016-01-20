#!/bin/bash

DEFAULT_KERNEL_VERSION=""
DEFAULT_USER=$USER
DEFAULT_URL_PREFIX="ssh://"
DEFAULT_URL_SUFFIX="@git-amr-2.devtools.intel.com:29418/wfr-linux-devel"
DEFAULT_BRANCH="for-wfr-ifs-rhel7.2"
wfrconfig="arch/x86/configs/wfr-config"
kerneldir="drivers/infiniband/core"

# NOTE: qib, mlx4, mthca, and ib_cm are not needed for STL
#       However, they have changes[*] within them we are actively trying to get
#       upstream.  As soon as these changes are accepted we should be able to
#       drop those modules.
#       [*] new ioctl registration and URMPP flags
headers_to_copy="
	uverbs.h
	core_priv.h
"
sources_to_copy="
	uverbs_main.c
	uverbs_cmd.c
	uverbs_marshall.c
	$headers_to_copy
"
files_to_copy="
	Makefile
"

# ridiculously long to encourage good names later
rpmname="ifs-kernel-updates"

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
tardir=$workdir/stage
mkdir -p "$tardir" || exit 1

echo "Working in $workdir"

# check out or copy our sources
if [ "$gitfetch" != "false" ]; then
	rm -rf ksrc
	echo "Checking out source"
	time git clone --single-branch --branch $branch $url ksrc
	srcdir="ksrc"
else
	rm -rf ksrc; mkdir -p ksrc
	echo "Copying source from $srcdir"
	(cd "$srcdir";cd "$kerneldir"; tar cf - ${sources_to_copy}) | (cd ksrc; tar xf -)
fi


# create the Makefile
echo "Creating Makefile ($tardir/Makefile)"
cat > $tardir/Makefile <<'EOF'
#
# uverbs module
#
#
# Called from the kernel module build system.
#
ifneq ($(KERNELRELEASE),)
#kbuild part of makefile
obj-$(CONFIG_INFINIBAND_USER_ACCESS) += ib_uverbs.o

ib_uverbs-y := uverbs_main.o uverbs_cmd.o uverbs_marshall.o
else
#normal makefile
KDIR ?= /lib/modules/`uname -r`/build

default:
	$(MAKE) -C $(KDIR) M=$$PWD

clean:
	$(MAKE) -C $(KDIR) M=$$PWD clean

install:
	$(MAKE) INSTALL_MOD_DIR=updates -C $(KDIR) M=$$PWD modules_install

endif

EOF

DEFAULT_KERNEL_VERSION=$(uname -r)

if [ "$DEFAULT_KERNEL_VERSION" == "" ]; then
	echo "Unable to generate the kernel version"
	exit 1
fi

rpmrelease=$(cd "$srcdir"; git rev-list "origin..HEAD" | wc -l)
echo "rpmrelease = $rpmrelease"

echo "Setting up RPM build area"
mkdir -p rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# make sure rpm component strings are clean, should be no-ops
rpmname=$(echo "$rpmname" | sed -e 's/[.]/_/g')
rpmversion=$(echo "$DEFAULT_KERNEL_VERSION" | sed -e 's/-/_/g')
rpmrequires=$(echo "$DEFAULT_KERNEL_VERSION" | sed -e 's/.[^.]*$//')

# build the tarball
echo "Copy the working files from $srcdir/$kerneldir"
echo "Copy the working files to $tardir"
pushd $srcdir/$kerneldir
cp ${sources_to_copy} $tardir
popd
echo "Building tar file"
(cd $tardir; tar cfz - --transform="s,^,${rpmname}-${rpmversion}/," *) > \
	rpmbuild/SOURCES/$rpmname-$rpmversion.tgz
cd $workdir

# create the spec file
echo "Creating spec file"
cat > rpmbuild/SPECS/$rpmname.spec <<EOF
Name:           $rpmname
Group:		System Environment/Kernel
Summary:        Extra kernel modules for IFS
Version:        $rpmversion
Release:        $rpmrelease
License:        GPL v2
Source:         %{name}-%{version}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root
Requires:	kernel = $rpmrequires
Provides:	$rpmname-$DEFAULT_KERNEL_VERSION

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
Updated kernel modules for OPA IFS

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
# make -C %kbuild M=\$(pwd)/drivers/infiniband/core ib_uverbs.ko
make

# Then build drivers...

# NOTE: the following not are required for STL but we are carrying patches for them
#       which require a rebuild
#       See NOTE in build script under sources to copy
#cp \$(pwd)/drivers/infiniband/core/Module.symvers \$(pwd)/drivers/infiniband/hw/qib
#make -C %kbuild M=\$(pwd)/drivers/infiniband/hw/qib

%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/lib/modules/%kver/updates

cp ib_uverbs.ko \$RPM_BUILD_ROOT/lib/modules/%kver/updates

%post
depmod -a $DEFAULT_KERNEL_VERSION

%postun
depmod -a $DEFAULT_KERNEL_VERSION

%clean
rm -rf \${RPM_BUILD_ROOT}

%files
%defattr(-, root, root)
%dir /lib/modules/%kver/updates
/lib/modules/%kver/updates/*

EOF

# moment of truth, run rpmbuild
rm -rf ksrc
echo "Building SRPM"
cd rpmbuild
rpmbuild -bs --define "_topdir $(pwd)" SPECS/${rpmname}.spec

exit 0
