#!/bin/bash

make distclean
rm -rf BUILD RPMS SOURCES SPECS SRPMS BUILDROOT

make dist

mkdir -p ./{BUILD,RPMS,SOURCES,SPECS,SRPMS,BUILDROOT}
cp ./hfi1-*.tgz SOURCES
cp hfi.spec SPECS
rpmbuild -bs --define "_topdir $PWD" --nodeps SPECS/hfi.spec
