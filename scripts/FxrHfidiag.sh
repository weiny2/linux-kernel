#!/bin/sh
set -x

. scripts/GlobalDefinition.sh

cd ${JENKINS_WORKSPACE}
if [ ! -d ${DIAGTOOL_REPO}/.git ]; then
	if [ ${ByJenkins} == yes ] ; then
		git clone ssh://lab_fxrdrvbld@git-amr-2.devtools.intel.com:29418/wfr-diagtools-sw \
			${DIAGTOOL_REPO}
	else
		git clone ssh://cyokoyam@git-amr-2.devtools.intel.com:29418/wfr-diagtools-sw \
			${DIAGTOOL_REPO}
	fi
fi
cd ${DIAGTOOL_REPO}
git checkout fxr_v2
git clean -f
git reset --hard origin/fxr_v2
make specfile RPM_NAME=hfidiags-hfi2
make dist RPM_NAME=hfidiags-hfi2
rm -rf rpmbuild
mkdir -p rpmbuild/SOURCES
mv hfidiags-hfi2-[0-9]*.tar.gz rpmbuild/SOURCES
rpmbuild -bb --nodeps \
	--define "_topdir ${PWD}/rpmbuild" \
	./hfidiags-hfi2.spec
