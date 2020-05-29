#!/bin/bash

#set -x

BRANCH="eywa"
MANIFEST="manifest"
PATCH_MANIFEST="patch-manifest"
README="README.intel"
DATE=$(date +%F)
LOGNAME="merge-$DATE.log"
tmpdir=$(mktemp -d)
baseversion=$(grep "Resetting master to" $LOGNAME | uniq | sed "s/Resetting master to //g")

die() {
	echo "ERROR: $@"
	if [ -e $tmpdir/commitmsg ]; then
		rm $tmpdir/commitmsg
	fi
	rmdir $tmpdir
	exit 1
}

[  ! -z $baseversion ] || die "Unable to determine base version"

[ -f $MANIFEST ] || die "manifest is not available"

[ -f $PATCH_MANIFEST ] || die "patch manifest is not available"

[ -f $README ] || die "${README} is not available"

[ -f $LOGNAME ] || die "merge log is not available"

pushupstream=0
kernelcommit=""

while getopts ":po:" option
do
	case $option in
	p) pushupstream=1;;
	o) kernelcommit=$OPTARG;;
	\?) die "Invalid option -$OPTARG";;
	:) die "Option -$OPTARG requires a parameter";;
	esac
done

shift $((OPTIND-1))

if [ "z$kernelcommit" != "z" ]; then
	if [ -e $kernelcommit ]; then
		die "$kernelcommit already exists"
	fi
	if [ ! -d $(dirname $kernelcommit) ]; then
		mkdir -p $(dirname $kernelcommit) || \
			die "Unable to create $(dirname $kernelcommit)"
	fi
else
	kernelcommit=$(mktemp)
fi

mv $MANIFEST $BRANCH || die "Unable to move manifest"
mv $PATCH_MANIFEST $BRANCH || die "Unable to move patch manifest"
mv $LOGNAME $BRANCH || die "Unable to move log $LOGNAME"

git add $BRANCH/$MANIFEST || die "Unable to add manifest to repo"
git add $BRANCH/$PATCH_MANIFEST || die "Unable to add patch manifest to repo"
git add $BRANCH/$LOGNAME || die "Unable to add $LOGNAME to repo"
git add ${README} || die "Unable to add ${README} to repo"

cat > $tmpdir/commitmsg << EOF
Intel Next: Add release files for $baseversion $DATE

Add ${README}, ${MANIFEST}, ${PATCH_MANIFEST}, and the merge log.

manifest:
EOF

grep -v -e "^\#" -e "^$" $BRANCH/$MANIFEST >> $tmpdir/commitmsg
echo " " >> $tmpdir/commitmsg

git commit -s -F $tmpdir/commitmsg || die "Unable to commit merge commit"

rm $tmpdir/commitmsg
rmdir $tmpdir

cat > $kernelcommit << EOF
Intel Next: Update to $baseversion $DATE

manifest:
EOF

grep -v -e "^\#" -e "^$" $BRANCH/$MANIFEST >> $kernelcommit
echo " " >> $kernelcommit

baseversion=$(echo $baseversion | sed "s/v//g")
LOGNAME=$(echo $LOGNAME | sed "s/.log//g")

if [ $pushupstream -eq 0 ]; then
	echo "success you can now tag and push the release with: "
	echo "git tag intel-$baseversion-$DATE"
	echo "git push origin -f master:master intel-$baseversion-$DATE"
	echo ""
	echo "git checkout linus"
	echo "git merge --ff-only v$baseversion"
	echo "git push origin linus:linus"
	echo "git push origin linus:linus v$baseversion"
	echo ""
else
	git tag intel-$baseversion-$DATE || die "Unable to tag repo"
	echo "Pushing master branch upstream ..."
	git push origin -f master:master intel-$baseversion-$DATE || \
		die "Unable to push kernel master branch upstream"
	echo "Switching to linus branch ..."
	git checkout linus || die "Unable to switch to linus branch"
	git merge --ff-only v$baseversion || die "Unable to update linus branch"
	echo "Pushing linus branch upstream ..."
	git push origin linus:linus || die "Unable to push linus branch upstream"
	git push origin linus:linus v$baseversion || die "Unable to push latest linus tag upstream"
fi

echo "file containing text for kernel commit can be found in $kernelcommit"
exit 0
