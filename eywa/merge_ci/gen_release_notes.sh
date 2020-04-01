#!/bin/bash
#This script is for release notes geneartion of intel next
#It is meant to run from buildbot after the daily tag has been pushed

latest_linus_tag=$(git describe --tags origin/linus)
latest_tag=$(git describe --tags)
previous_tag=$(git for-each-ref --sort=committerdate | grep refs/tags/intel | sed "s/.*refs\/tags\///g"  |tail -2 | head -1)
release_notes_file=$latest_tag.release-notes

#get major version of the kernel to find out if this is first release
major_version=$(echo $latest_tag | sed "s/intel-//g" | sed "s/-.*//g")
nth_release=$(git tag | grep intel-$major_version  |wc -l)

email_subject="Intel Next $latest_tag release notes"
from_email="intel-next-maintainers@eclists.intel.com"
freindly_name="Intel Next Project"
to_email="kyle.d.pelton@intel.com"
rule="------------------------------------------------------------------------"

echo "Intel Next $latest_tag release notes " >$release_notes_file
echo $rule >>$release_notes_file

echo "This release is based on upstream $latest_linus_tag and is release number $nth_release of this cycle." >>$release_notes_file
echo  >>$release_notes_file
echo "git repo: ssh://git@gitlab.devtools.intel.com:29418/intel-next/intel-next-kernel.git" >>$release_notes_file
echo  >>$release_notes_file
echo "branch: master"  >>$release_notes_file
echo "tag: $latest_tag"  >>$release_notes_file
echo >>$release_notes_file
cat >> $release_notes_file << EOF
For more information about intel next visit: http://goto.intel.com/intelnext

Links to Intel Next binaries are avaliable on gitlab: https://gitlab.devtools.intel.com/intel-next/intel-next-kernel/-/releases
Intel Next validation results: http://mozart.sh.intel.com:8080/dashboard/quick_entrance/LTP-DDT/Lab/intel-next_clr
$rule
EOF
#We want to skip the patch list on the first release because it will contain 
#all upstream code and will be too big
if [ $nth_release -ne 1 ]; then
        cp eywa/manifest.json eywa/manifest_new.json
        git checkout $previous_tag -- eywa/manifest.json
        mv eywa/manifest.json eywa/manifest_previous.json
        mv eywa/manifest_new.json eywa/manifest.json
        echo "Summary of changes:"  >>$release_notes_file     
	echo  >>$release_notes_file
        eywa/merge_ci/diff_manifest.py eywa/manifest.json eywa/manifest_previous.json >> $release_notes_file
	echo >>$release_notes_file
        echo "Patches added since previous tag $previous_tag:" >>$release_notes_file
	echo >>$release_notes_file
	git shortlog $previous_tag..$latest_tag  --no-merges -e >>$release_notes_file
fi

echo "Branch manifest for tag:$latest_tag" >>$release_notes_file
echo $rule >>$release_notes_file
grep -v -e "^\#" -e "^$" eywa/manifest |sort >>$release_notes_file
echo >>$release_notes_file
echo "README.intel" >>$release_notes_file
echo >>$release_notes_file
cat README.intel >>$release_notes_file

echo "Sending release email to $to_email"
#sending release notes
cat $release_notes_file | mailx  -s "$email_subject"  -a "From: $freindly_name <$from_email>" -r $from_email $to_email 
