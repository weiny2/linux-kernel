#!/bin/bash
function print_help
{
	echo "Usage: `basename $0` -k kernel_src -b branch -t [nostrict|upstream]"
	echo "-k: path to kernel src (optional)"
	echo "-b: branch to check against (optional: default is master)"
	echo "-t: type of checkpatch to do"
	echo "    nostrict: do not use -strict"
	echo "    upstream: use checkpatch.pl found in kernel_src"
	exit 1
}

script_dir=`readlink -f ../../scripts`

while getopts "k:b:t:" opt
do
        case $opt in
        k)
                kernel_build=$OPTARG
                ;;
	b)
		branch=$OPTARG
		;;
	t)
		check_type=$OPTARG
		;;
        *)
                print_help
                ;;
        esac
done

if [[ -z $kernel_build ]]; then
	kernel_build=`pwd`
	if [[ $check_type == "upstream" ]]; then
		echo "Can not run upstream on out-of-tree driver"
		print_help
	fi
fi

if [[ -z $branch ]]; then
	branch="master"
fi

# Run check patch on current git commits
# Feed in kernel_build dir or use default.

if [[ -z $check_type ]]; then
	check_patch="$script_dir/checkpatch.pl --no-tree --strict"
	echo "Checking in git repo: $kernel_build with: $check_patch against origin/$branch"
	cd $kernel_build
	if [[ $? -ne 0 ]]; then
		echo "Could not chdir to: $kernel_build"
		exit 1
	fi
elif [[ $check_type == "nostrict" ]]; then
	check_patch="$script_dir/checkpatch.pl --no-tree"
	echo "Checking in git repo: $kernel_build with: $check_patch against origin/$branch"
	cd $kernel_build
	if [[ $? -ne 0 ]]; then
		echo "Could not chdir to: $kernel_build"
		exit 1
	fi

elif [[ $check_type == "upstream" ]]; then
	check_patch="$kernel_build/scripts/checkpatch.pl --strict"
	echo "Checking in git repo: $kernel_build with: $check_patch against origin/$branch"
	cd $kernel_build
	if [[ $? -ne 0 ]]; then
		echo "Could not chdir to: $kernel_build"
		exit 1
	fi
else
	echo "Wrong check type"
	exit 1
fi

summary=".checkpatch_summary_temp_file"
rm -f $summary
for i in \
    $(git log --reverse origin/${branch}..$(git symbolic-ref HEAD) --pretty=format:'%H' ); do
    	echo "---------------------------------------------------"
	echo Checking patch $i
    	echo "---------------------------------------------------"
	patch=$(git format-patch -1 $i)
	# We could just pipe the check_patch command to tee but that will strip
	# the color formatting of the ERROR, CHECK, and WARNING tags so just do
	#the command twice.
	$check_patch $patch
	$check_patch $patch > $patch.temp
	lines_checked=`grep "lines checked" $patch.temp`
	echo "$lines_checked checked in $patch" >> $summary
	rm -f $patch
	rm -f $patch.temp
	echo ""
done

echo "--------"
echo "Summary:"
echo "--------"
cat $summary
rm -f $summary
