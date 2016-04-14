#!/bin/bash

# Pull commits from internal repo and prep them for inclusion upstream

function print_help
{
	echo "Usage: `basename $0` -k kernel_src -b branch -r remote [-s series|COMMIT, COMMIT]"
	echo " Specify a list of commits to cherry pick or use the -s for a consecutive series"
	echo "-k: path to kernel src"
	echo "-b: branch to checkout"
	echo "-r: remote (default: origin)"
	echo "-s: first~1..last (you probably want the ~1)"
	exit 1
}

datestamp=`date "+%s"`
top_dir=$(git rev-parse --show-toplevel)
new_branch="for-a-stream-$datestamp"
log_dir="/nfs/sc/disks/fabric_mirror/scratch/upstream_logs"
orig_branch="a-stream-master"

while getopts "k:b:s:r:" opt
do
        case $opt in
        k)
                kernel_build=$OPTARG
                ;;
	b)
		branch=$OPTARG
		;;
	r)
		remote=$OPTARG
		;;
	s)
		series=$OPTARG
		;;
        *)
                print_help
                ;;
        esac
done

shift $(($OPTIND - 1))

if [[ -z $branch ]]; then
	print_help
fi

if [[ -z $kernel_build ]]; then
	print_help
fi

if [[ ! -d $kernel_build ]]; then
	print_help
fi

if [[ -z $remote ]]; then
	remote="origin"
fi

cd $kernel_build
if [[ $? -ne 0 ]]; then
	echo "Could not chdir to $kernel_build"
	exit 1
fi

#----------------------------------------------------
# Create a working branch and get the commits into it
#----------------------------------------------------

echo "Pulling a branch from $branch in the git tree at $kernel_build called $new_branch"

echo "Updating remote: $remote"
git remote update $remote
if [[ $? -ne 0 ]]; then
	echo "Could not update remote"
	exit 1
fi

git checkout -b $new_branch $remote/$branch
if [[ $? -ne 0 ]]; then
	echo "Could not checkout branch"
	exit 1
fi

echo "Processing commits..."
cnt=0
if [[ -z $series ]]; then
    for commit in $@; do
	    echo $commit;
	    git cherry-pick $commit | tee $log_dir/cherry_pick_${datestamp}.log
	    if [[ $? -ne 0 ]]; then
		    echo "Cherry pick did not go cleanly"
		    exit 1
	    fi
	    cnt=$((cnt+1))
    done
else
	cnt=`git rev-list $series --count`
	echo "Cherry picking a series of $cnt commits"
	git cherry-pick $series | tee $log_dir/cherry_pick_${datestamp}.log
fi

echo "All commits cherry-picked, changing to stgit"
stg init
if [[ $? -ne 0 ]]; then
	echo "Could not stg init the branch"
	exit 1
fi

echo "Making stg patches"
stg uncommit -n $cnt
if [[ $? -ne 0 ]]; then
	echo "Could not stg uncommit right number"
	exit 1
fi

#-------------------------------------------------------------------
# Validate the commits look OK to post. Run checkpatch and diff them
#-------------------------------------------------------------------

echo "Checking patches..."
cd $top_dir/test/tests
./checkpatch.sh -k $kernel_build -b $branch -t upstream | tee $log_dir/check_patch_${datestamp}.log
echo "Press ENTER to examine patches:"
read blah

cd $kernel_build
if [[ $? -ne 0 ]]; then
	echo "Could not change back to $kernel_build"
	exit 1
fi

# Popping all commits
stg pop --all

while :
do
	stg push
	if [[ $? -ne 0 ]]; then
		echo "No more patches"
		break
	fi

	stg show -O -U5 | vim -c "set filetype=gitcommit" -
	if [[ $? -ne 0 ]]; then
		echo "Could not edit commit message"
		exit 1
	fi
done

#---------------
# Kick off tests
#---------------

echo "Press ENTER to run presubmit checks and regression tests"
read blah

cd $top_dir/test/tests
./presubmit_checks.sh -k $kernel_build | tee $log_dir/presubmit_${datestamp}.log
if [[ $? -ne 0 ]]; then
	echo "Did not pass presubmit checks"
	exit 1
fi

echo "Running tests"
cd ..
./queue_test.sh -k $kernel_build -m hfi1 | tee $log_dir/hfi1_test_${datestamp}.log
if [[ $? -ne 0 ]]; then
	echo "Test failed! Do not submit"
	exit 1
fi

#------------------------------------------
# Looks good if we got to here time to push
#------------------------------------------

cd $kernel_build
if [[ $? -ne 0 ]]; then
	echo "Could not change back to $kernel_build"
	exit 1
fi


echo "Logs are located at:"
echo "                    $log_dir/cherry_pick_${datestamp}.log"
echo "                    $log_dir/check_patch_${datestamp}.log"
echo "			  $log_dir/presubmit_${datestamp}.log"
echo "                    $log_dir/hfi1_test_${datestamp}.log"

cmd="git push $remote $new_branch:$branch"

echo "Press Enter to commit this with: $cmd otherwise exit with ctrl+c"
read blah
$cmd
if [[ $? -ne 0 ]]; then
	echo "Could not push"
	exit 1
fi

#-------------
# Notification
#-------------

echo "Sending mail notification:"

echo "This is a note to confirm that the following commits have been added to the branch: $branch" > mail.tmp
echo "" >> mail.tmp
git log -n $cnt --oneline >> mail.tmp
echo "" >> mail.tmp
echo "" >> mail.tmp
echo "These will posted upstream soon" >> mail.tmp

mailx -S smtp=smtp://smtp.intel.com -s "[$branch] Processed pull requests" -r dennis.dalessandro@intel.com hfi1-kernel@eclists.intel.com < mail.tmp
echo "Just sent:"
cat mail.tmp
rm mail.tmp

echo ""
echo "If user requested their branch can now be removed with: git push origin --delete <branch>"

echo "-------------------------------------------------------------"
echo "Add these commits to the spreadsheet:"
git --no-pager log -n $cnt --pretty=format:'%s'; echo ""
echo "-------------------------------------------------------------"
echo ""

#--------------------
# Update local branch
#--------------------
echo "To return to $orig_branch and pull updates press ENTER"
read blah
git checkout $orig_branch
git pull --rebase

#-----------------------
# Remove working branch?
#-----------------------
echo ""
echo "Remove temporary branch: $new_branch press ENTER?"
read blarg
stg branch --delete --force $new_branch

exit 0
