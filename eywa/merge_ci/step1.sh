#!/bin/bash
#Automates setup and running of gen-manifest.sh

function clean_build {
	set -e
	git reset master --hard
	git checkout --detach
	echo tracking; git branch -f master origin/master ; git branch -f eywa origin/eywa; git branch -f linus origin/linus
	git fetch
	git checkout eywa -f
	git reset --hard origin/eywa
}

function run_blacklist {
	#./blacklist.py
	return
}

function append_test_branch {
	pushd .
	cd eywa

cat >>manifest.in << EOF
###############################################################################
# Single Test Branch (NOT COMMITTED)
###############################################################################
EOF

	echo $1 >>manifest.in
	popd
}
function create_single_branch {
	pushd .
	cd eywa
	cat >manifest.in << EOF
###############################################################################
# INTEL NEXT INFRASTRUCTURE
###############################################################################

# Intel Next scripts
ssh://git-amr-1.devtools.intel.com:29418/otc_intel_next-linux.git eywa

# Intel Next kernel configs
ssh://git-amr-1.devtools.intel.com:29418/otc_intel_next-linux.git configs

# Intel Next packaging
ssh://git-amr-1.devtools.intel.com:29418/otc_intel_next-linux.git packaging

###############################################################################
# Single Test Branch (NOT COMMITED)
###############################################################################
EOF
	echo $1 >>manifest.in
	popd

}

function gen_manifest {
	if [ -z $skip_fetch ]; then
		./merge.py -g
		if [ $? != 0 ]; then
                   echo "merge.py -g failed to generate manifest. look at intel-next-merge*.log"
		fi
	else
		./merge.py -g -s
		if [ $? != 0 ]; then
                   echo "merge.py -g failed to generate manifest. look at intel-next-merge*.log"
		fi

	fi
	rm README*
	#RUN replace.py script to replace any branches
}

exit_usage() {
        cat <<EOF >&2

USAGE:
        $0 [ -t <repo branch>| -f <repo branch>| -a <manifest file to append> |-h|?

	t = (optional) generate manifest with only given brach/repo + eywa branches
	f = (optional) append test repo branch to end of manifest.in
	a = (optional) append anonther manifest file into manifest.in
	s = (optional) skip fetch of all branches of manifest file except linus
        h|? = help (this screen)

EOF
exit 1
}
while getopts "f:t:a:hs?" OPTION; do
        case $OPTION in
                f)
                        merge_test_branch="$OPTARG"
                        ;;
                t)
                        merge_single_branch="$OPTARG"
                        ;;
		a)
			append_manifest_file="$OPTARG"
                        ;;
		s)
			skip_fetch=true
                        ;;
		h|?)
                        OPT_exit_only=true
                        ;;
        esac
done

if [[ -v OPT_exit_only ]]; then
	exit_usage
fi
#if both test parameters are given exit
if [[ ! -z "$merge_test_branch" ]]  && [[ ! -z "$merge_single_branch"  ]]; then
	exit_usage
fi

#first run blacklist
clean_build


#append another mnaifest file if given
if [[ ! -z "$append_manifest_file" ]]; then
	cat $append_manifest_file >>eywa/manifest.in
fi

#check to see if both options are set
if [[ ! -z "$merge_test_branch" ]]; then
	append_test_branch "$merge_test_branch"
elif [[ ! -z "$merge_single_branch" ]]; then
	create_single_branch "$merge_single_branch"
fi
gen_manifest
