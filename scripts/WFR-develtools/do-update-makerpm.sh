#!/bin/bash

# TODO: find a way to get this programatically from the target and put
# the method _in_ the spec file
full_kernel_rpmversion="3.9.2_wfr+"

DEFAULT_USER=$USER
DEFAULT_URL_PREFIX="ssh://"
DEFAULT_URL_SUFFIX="@git-amr-2.devtools.intel.com:29418/wfr-linux-devel"
DEFAULT_BRANCH="wfr-3.9.y-3.9.2-rmpp-usa"

sources_to_copy="
	drivers/infiniband/core
	include/rdma/ib_sa.h
	include/rdma/ib_usa.h
"

target_kos_to_build="
	drivers/infiniband/core/ib_sa.ko
	drivers/infiniband/core/ib_usa.ko
	drivers/infiniband/core/ib_mad.ko
	drivers/infiniband/core/ib_umad.ko
"

# ridiculously long to encourage good names later
rpmname="ifs-kernel-rmpp-usa-updates"
rpmversion="1.2.3"
rpmrelease="02134zoom"

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
else
	rm -rf ksrc; mkdir -p ksrc
	echo "Copying source from $srcdir"
	(cd "$srcdir"; tar cf - ${sources_to_copy}) | (cd ksrc; tar xf -)
fi

echo "Setting up RPM build area"
mkdir -p rpmbuild/{SOURCES,SPECS}

# patch Makefile to use local include files first
# kind of a hack, perfect thing to put in SOURCES as a real patch
echo 'NOSTDINC_FLAGS := -I\$(M)/../../../include' >> ksrc/drivers/infiniband/core/Makefile

# build the tarball
echo "Building tar file"
(cd ksrc; tar cfz - --transform="s,^,${rpmname}-${rpmversion}/," ${sources_to_copy}) > \
	rpmbuild/SOURCES/$rpmname-$rpmversion-$rpmrelease.tgz

# create the spec file
# section 1 - header through mid build
echo "Creating spec file"
cat > rpmbuild/SPECS/$rpmname.spec <<EOF
Name:           $rpmname
Summary:        Extra kernel modules for IFS
Version:        $rpmversion
Release:        $rpmrelease
License:        GPL v2
Source:         %{name}-%{version}-%{release}.tgz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root

%description
Updated kernel modules from wfr-3.9.y-3.9.2-rmpp-usa branch

%prep
%setup -q

%build
if [ -z "\$ksrc" ]; then
	echo "for now, \\\$ksrc must be explicitly set to kernel build dir" >&2
	exit 1
fi
EOF

# section 2 - create make lines for each .ko
for i in $target_kos_to_build; do
	echo "make -C \$ksrc M=\$(pwd)/$(dirname $i) $(basename $i)" >> rpmbuild/SPECS/$rpmname.spec
done

# section 3 - install
cat >> rpmbuild/SPECS/$rpmname.spec <<EOF

%install
rm -rf \$RPM_BUILD_ROOT
mkdir -p \$RPM_BUILD_ROOT/lib/modules/${full_kernel_rpmversion}/updates
EOF

# sections 4 - create lines for each .ko to copy
for i in $target_kos_to_build; do
	echo "cp $i \$RPM_BUILD_ROOT/lib/modules/${full_kernel_rpmversion}/updates" >> rpmbuild/SPECS/$rpmname.spec
done

# section 5 - finish
cat >> rpmbuild/SPECS/$rpmname.spec <<EOF

%clean
rm -rf \${RPM_BUILD_ROOT}

%files
%defattr(-, root, root)
%dir /lib/modules/${full_kernel_rpmversion}/updates
/lib/modules/${full_kernel_rpmversion}/updates/*
EOF

# moment of truth, run rpmbuild
rm -rf ksrc
echo "Building SRPM"
cd rpmbuild
rpmbuild -bs --define "_topdir $(pwd)" SPECS/${rpmname}.spec

exit 0
