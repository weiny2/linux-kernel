#!/bin/bash
BRANCH="eywa"
MANIFEST="manifest"
PATCH_MANIFEST="patch-manifest"
README="README.intel"
LINUS_GIT="git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"
EYWA_GIT="ssh://git-amr-2.devtools.intel.com:29418/otc_eywa-linux-eywa"
LOGPREFIX="merge-$(date +%F)"
FALSE_ERRORS="Checking[[:space:]]out[[:space:]]files:[[:space:]]\{1,3\}[[:digit:]]\{1,3\}.* \
             Auto[[:space:]]packing[[:space:]]the[[:space:]]repository[[:space:]]in[[:space:]]background[[:space:]]for[[:space:]]optimum[[:space:]]performance\.  \
             See[[:space:]]\"git[[:space:]]help[[:space:]]gc\"[[:space:]]for[[:space:]]manual[[:space:]]housekeeping\. \
             Staged.*previous.*.*resolution"

exit_usage() {
        cat <<EOF >&2

USAGE:
	$0 [ -c | -h | -? ] [ -m MANIFEST ]

	c = continue
	f = use patch-manifest file with -c option to speed up merge
	m = maninfest file to use
	h|? = help (this screen)

EOF
exit 1
}

OPT_continue=false
OPT_exit_only=false
OPT_fast_check=false
while getopts "cfm:h?" OPTION; do
	case $OPTION in
		c)
			OPT_continue=true
			;;
		m)
			MANIFEST=$OPTARG
			;;
		f)
			OPT_continue=true
			OPT_fast_check=true
			;;

		h|?)
			OPT_exit_only=true
			;;
	esac
done
$OPT_exit_only && exit_usage

if [ ! -r $MANIFEST ] ; then
	echo no manifest file found
	exit_usage
fi

rule() {
	echo "------------------------------------------------------------------------"
}

err_log_contains_errors() {
	# git often prints normal operating messages to stderr with mac
	# line endings. First we convert the file and then parse out some
	# expected strings to ensure we are actually dealing with errors.
	dos2unix -c mac $LOGPREFIX.err
	if [ $? -ne 0 ]; then
		echo 1
	fi
	cp $LOGPREFIX.err /tmp/$LOGPREFIX.err.org
	for error in $FALSE_ERRORS; do
		sed -i /$error/d $LOGPREFIX.err
	done
	if [ -s $LOGPREFIX.err ]; then
		echo 1
	else
		echo 0
	fi
}

if [ -e $LOGPREFIX.log ] && [ "$OPT_continue" = false ]; then
	echo "ERROR: Log file $LOGPREFIX.log exists, aborting"
	exit 1
fi

if [ -e $LOGPREFIX.err ] && [ "$OPT_continue" = false ]; then
	echo "ERROR: Log file $LOGPREFIX.err exists, aborting"
	exit 1
fi

do_backup() {
	# If -c (continue) is passed, skip the backup and reset step
	if [ "$OPT_continue" = false ]; then
		echo "Backing up master to master-old"
		git checkout master >/dev/null 2>&1
		git branch master-old >/dev/null 2>&1
		if [ $? -ne 0 ]; then
			echo "ERROR: master-old branch exists, aborting"
			exit 1
		fi
		rule
	fi
}

do_merge() {
	while read -r URL REF COMMIT; do
		# Skip comments and blank lines
		if [ -z "$URL" ] || [ "${URL:0:1}" = "#" ]; then continue; fi

		# Assume custom scripts will be in manifest after $BRANCH 
		# branch has been mounted since this is where custom
		# scripts are stored.
		# Custom scripts are always run - whether this is a
		# continuation of failed merge or not
		if [ "${URL:0:6}" = "custom" ]; then
			echo "Executing custom merge script $REF"
			$BRANCH/custom/$REF
			if [ $? -ne 0 ]; then
				echo "ERROR: failure during custom script $REF"
				exit 1
			fi
			# Assume script generates one commit that should be
			# added to patch manifest.
			echo "Merge custom from $REF archive" >>\
				$PATCH_MANIFEST
			git log -1 --pretty=oneline >> $PATCH_MANIFEST
			rule >> $PATCH_MANIFEST
			rule
			continue
		fi

		REMOTE=$(git remote -v | grep "$URL.*(fetch)" | awk '{print $1}')
		if [ -z "$REMOTE" ]; then
			echo "ERROR: no remote for $URL"
			exit 1
		fi

		echo "$URL $REF $COMMIT"
		if [ "$URL" = "$LINUS_GIT" ] && [[ "${REF:0:3}" == v[3456789]. ]]; then
			kernelversion=$REF
		fi
		if [ "$OPT_continue" = false ] && [ "$URL" = "$LINUS_GIT" ] && [[ "${REF:0:3}" == v[3456789]. ]]; then
			echo "Resetting master to $REF"
			#remove patch-manifest file in case of incomplete merge
			rm $PATCH_MANIFEST 2>/dev/null
			touch $PATCH_MANIFEST
			git reset --hard $REF
			rule
		else
			# Skip any topics/commits that have allready been merged
			if [ "$OPT_fast_check" = true ]; then
				egrep -q "^Merge $COMMIT from" $PATCH_MANIFEST
				if [ $? -eq 0 ]; then continue; fi
			elif [ "$OPT_continue" = true ]; then
				git branch --contains $COMMIT | egrep -q "\* master"
				if [ $? -eq 0 ]; then continue; fi
			fi
			echo "Merging $REMOTE $REF $COMMIT"
			git merge -m "Intel Next: Merge commit $COMMIT from $URL $REF" --no-ff --log $COMMIT --rerere-autoupdate
			if [ $? -ne 0 ]; then
				output=`git rerere diff`
				if [ "$output" = "" ]; then
					echo "Rerere handled conflicts.. Printing log and continuing merge"
					echo "merge log"
					git diff --stat --stat-width=200 --cached
					git commit --no-edit
				else
					echo "Merge has failed. Check git status to see conflicts. Fix them and then run '$0 -c or (-f)' to continue"
					exit 1
				fi
			fi
			if [ "$URL" != "$EYWA_GIT" ]; then
				echo "Merge $COMMIT from $URL $REF" >> $PATCH_MANIFEST
				#Disable patch manifest print
				#git log -1 --merges --pretty=format:%P |\
				#	awk '{system("git log --pretty=oneline --no-merges --abbrev-commit "$1".."$2)}' >> $PATCH_MANIFEST
				rule >> $PATCH_MANIFEST
			fi
			rule
		fi
	done < $MANIFEST
}

do_readme() {
	echo "INTEL CONFIDENTIAL. FOR INTERNAL USE ONLY." > ${README}
	rule >> ${README}
	echo "" >> ${README}
	echo "README for ${LOGPREFIX}" >> ${README}
	echo "" >> ${README}
	echo "" >> ${README}

	for readme in ${README}.*; do
		if [ ! -r ${readme} ]; then
			echo "(No READMEs provided by topic branch maintainers.)" >> ${README}
			echo "" >> ${README}
			continue
		fi

		echo "${readme}:" >> ${README}
		echo "" >> ${README}
		cat ${readme} >> ${README}
		echo "" >> ${README}
	done

	rule >> ${README}
	echo "Intel Next Maintainers <intel-next-maintainers@eclists.intel.com>" >> ${README}
}

do_backup > >(tee -a $LOGPREFIX.log) 2> >(tee -a $LOGPREFIX.err >&2)
do_merge > >(tee -a $LOGPREFIX.log) 2> >(tee -a $LOGPREFIX.err >&2)
do_readme

if [ -s $LOGPREFIX.err ]; then
	result=$(err_log_contains_errors)
	if [ $result -eq 1 ]; then
		echo "ERROR: merge completed with warnings/errors. Please see $LOGPREFIX.err"
		exit 1
	else
		rm $LOGPREFIX.err
	fi
elif [ -e $LOGPREFIX.err ]; then
	rm $LOGPREFIX.err
fi

if [ -s /tmp/$LOGPREFIX.err.org ]; then
	echo "Merge completed with false positives. These can be reviewed in /tmp/$LOGPREFIX.err.org. Please remove file after review."
fi

echo "Complete. Log output can be found in $LOGPREFIX.log"
