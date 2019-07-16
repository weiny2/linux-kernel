#!/bin/bash
set -f

MANIFEST_IN="manifest.in"
MANIFEST="manifest"
LINUS_GIT="git://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git"

exit_usage() {
        cat <<EOF >&2

USAGE:
	$0 [ -m MANIFEST_IN ] [ -o MANIFEST_OUT ]

	m = maninfest in file to use
	o = maninfest out file to use
	s = skip fetch of all branches except linus
	h|? = help (this screen)

EOF
exit 1
}

OPT_exit_only=false
while getopts "m:o:hs?" OPTION; do
	case $OPTION in
		m)
			MANIFEST_IN=$OPTARG
			;;
		o)
			MANIFEST=$OPTARG
			;;
		s)
			skip_fetch=true
			;;
		h|?)
			OPT_exit_only=true
			;;
	esac
done
$OPT_exit_only && exit_usage

if [ ! -r $MANIFEST_IN ] ; then
	echo no manifest in file found
	exit_usage
fi

rule() {
	echo "------------------------------------------------------------------------"
}

fetch() {
	grep -ne $1 fetch_cache
	if [ $? == 0 ]; then
		echo "$1 already fetched"
		return 0
	fi

	git fetch -p --tags $1
	if [ $? -ne 0 ]; then
		echo -e "\nERROR: Failed to fetch $1" >&2
		echo -e "Notify branch owner of the failure"  >&2
		return 1
	fi
	echo $1 >>fetch_cache
	return 0
}

# $1 URL
setup_remote() {
	URL=$1
	if [ -z "$URL" ]; then
		return 1
	fi

	REMOTE=$(git remote -v | grep -e "$URL.*(fetch)$" | awk '{print $1}')
	if [ -z "$REMOTE" ]; then
		REMOTE=$(echo -n $URL | tr -cs '[[:alnum:]]' '_')
	fi

	CUR_URL=$(git remote -v | grep -e "^$REMOTE\s.*(fetch)$" | awk '{print $2}')
	if [ -n "$CUR_URL" ]; then
		if [ ! "$CUR_URL" = "$URL" ]; then
			return 1
		fi
	else
		git remote add --tags $REMOTE $URL
		if [ $? -ne 0 ]; then
			return 1
		fi
	fi
	echo "$REMOTE"
}
rm -f fetch_cache
touch fetch_cache
echo -n "Determining latest version from Linus ... "
REMOTE=$(setup_remote $LINUS_GIT)
if [ $? -ne 0 ]; then
	echo "FAILED"
	exit 1
fi
fetch $REMOTE
if [ $? -ne 0 ]; then
	echo "FAILED"
	exit 1
fi

BASE_TAG=$(git ls-remote --tags $REMOTE | awk '{ print $2 }' | awk -F/ ' { print $3 } ' | sort -Vr | head -1 | sed 's/\^{}//')
echo $BASE_TAG
rule

echo "Generate $MANIFEST from the latest refs"
echo "# Latest mainline release tag" > $MANIFEST
echo "$LINUS_GIT $BASE_TAG $(git rev-parse $BASE_TAG)" >> $MANIFEST
while read -r URL REF; do
	# Pass comments, empty lines, and custom scripts through exactly
	if [ -z "$URL" ] || [ "${URL:0:1}" = "#" ] || [ "${URL:0:6}" = "custom" ]; then
		echo $URL $REF >> $MANIFEST
		continue
	fi

	echo -n "Setup remote $URL..."
	REMOTE=$(setup_remote $URL)
	if [ $? -ne 0 ]; then
		echo "FAILED"
		exit 1
	fi
	echo "$REMOTE"

	echo -n "Fetch $REMOTE..."
	if [ -z $skip_fetch ]; then
		fetch $REMOTE
	fi
	echo "done"

	echo -n "Calculate $REMOTE/$REF commit..."
	COMMIT=$(git rev-parse $REMOTE/$REF 2>/dev/null)
	if [ $? -ne 0 ]; then
		echo -e "\nERROR: $REMOTE/$REF failed 'git rev-parse'"
		exit 1
	fi
	echo "${COMMIT:0:10}"
	echo "$URL $REF $COMMIT" >> $MANIFEST
done < $MANIFEST_IN
rule
echo $MANIFEST:
cat $MANIFEST
rule
echo "To use different COMMITS than those automatically identified, edit
the manifest before running merge.sh"
