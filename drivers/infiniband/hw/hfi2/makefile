# Copyright (c) 2013, 2014. Intel Corporation. All rights reserved.
# Copyright (c) 2007, 2008, 2009. QLogic Corp. All rights reserved.
# Copyright (c) 2003, 2004, 2005. PathScale, Inc. All rights reserved.
#
# This software is available to you under a choice of one of two
# licenses.  You may choose to be licensed under the terms of the GNU
# General Public License (GPL) Version 2, available from the file
# COPYING in the main directory of this source tree, or the
# OpenIB.org BSD license below:
#
#     Redistribution and use in source and binary forms, with or
#     without modification, are permitted provided that the following
#     conditions are met:
#
#      - Redistributions of source code must retain the above
#        copyright notice, this list of conditions and the following
#        disclaimer.
#
#      - Redistributions in binary form must reproduce the above
#        copyright notice, this list of conditions and the following
#        disclaimer in the documentation and/or other materials
#        provided with the distribution.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
# BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
# ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
#
# Patent licenses, if any, provided herein do not apply to
# combinations of this program with other software, or any other
# product whatsoever.
#

NAME := hfi2

TOP_SRC=$(PWD)/../../../../

# Newer Git has faster tag lookup
GIT_VERSION := $(shell git --version | cut -b 13)
ifeq (2,$(GIT_VERSION))
GIT_EXTRA_FLAGS="--first-parent"
endif

# The desired version number comes from the most recent tag starting with "v"
VERSION := $(shell if [ -e $(TOP_SRC)/.git ] ; then git describe --tags $(GIT_EXTRA_FLAGS) --abbrev=0 --match="*fxr_v*" | sed -e 's/^fxr_v//' -e 's/-/_/'; else echo "version" ; fi)
ifeq (,$(VERSION))
VERSION := 0.99
endif
# The desired release number comes the git describe following the version which
# is the number of commits since the version tag was planted suffixed by the g<commitid>
RELEASE := $(shell if [ -e $(TOP_SRC)/.git ] ; then git describe --tags $(GIT_EXTRA_FLAGS) --long --match="*fxr_v*" | sed -e 's/fxr_v[0-9.]*-\([0-9]*\)/\1/' | sed 's/-g.*$$//'; else echo "release" ; fi)
ifeq (,$(RELEASE))
RELEASE := 0
endif
MVERSION = $(VERSION)-$(RELEASE)
# This is for changelog in specfile
BASEVERSION :=

EXCLUDES := --exclude-vcs --exclude-backups --exclude='*.patch' --exclude='*.swp' \
	--exclude='series' --exclude='*.orig' --exclude='$(NAME).spec*' \
	--exclude="cov.*" \
	--exclude="rpmbuild" \
	--exclude="*.tgz" \
	--exclude="*~" \
	--exclude="*.rpm" \
	--exclude=scripts

PWD := $(shell pwd)
VERBOSE := 0
RDMAVT_PATH ?= $(PWD)/../../sw/rdmavt
KRPING_PATH ?= $(PWD)/../../ulp/krping

# if KVER not specified, set KBUILD for in-tree build
ifeq (,$(KVER))
KBUILD ?= $(TOP_SRC)
KVER = $(shell cat $(KBUILD)/include/config/kernel.release)
else
# else if have KVER, use it to set KBUILD to installed kernel
KBUILD ?= /lib/modules/$(KVER)/build
endif

hfi2:
	$(MAKE) -C $(KBUILD) M=$(PWD) V=$(VERBOSE) MVERSION=$(MVERSION)

hfi2-ifs:
	$(MAKE) -C $(KBUILD) M=$(PWD) V=$(VERBOSE) MVERSION=$(MVERSION) \
	CONFIG_INFINIBAND_HFI2=m CONFIG_HFI2_STLNP=m

.PHONY: rdmavt krping
rdmavt:
	$(MAKE) -C $(KBUILD) M=$(RDMAVT_PATH) V=$(VERBOSE) MVERSION=$(MVERSION)
krping:
	$(MAKE) -C $(KBUILD) M=$(KRPING_PATH) V=$(VERBOSE) MVERSION=$(MVERSION)

# these targets assume you have write permission to $(KBUILD)
prep:
	install --mode=644 $(KBUILD)/arch/x86/configs/hfi2-stlnp.config $(KBUILD)/.config
	install --mode=644 $(PWD)/build/Module-simics-symvers $(KBUILD)/Module.symvers
	$(MAKE) -C $(KBUILD) modules_prepare
prep-zebu:
	install --mode=644 $(KBUILD)/arch/x86/configs/hfi2-stlnp.config $(KBUILD)/.config
	install --mode=644 $(PWD)/build/Module-zebu-symvers $(KBUILD)/Module.symvers
	$(MAKE) -C $(KBUILD) modules_prepare
prep-verbs:
	install --mode=644 $(KBUILD)/arch/x86/configs/hfi2.config $(KBUILD)/.config
	install --mode=644 $(PWD)/build/Module-simics-symvers $(KBUILD)/Module.symvers
	$(MAKE) -C $(KBUILD) modules_prepare
prep-debug:
	install --mode=644 $(KBUILD)/arch/x86/configs/hfi2-stlnp-debug.config $(KBUILD)/.config
	install --mode=644 $(PWD)/build/Module-simics-debug-symvers $(KBUILD)/Module.symvers
	$(MAKE) -C $(KBUILD) modules_prepare

clean:
	make -C $(KBUILD) M=${RDMAVT_PATH} clean
	make -C $(KBUILD) M=${KRPING_PATH} clean
	make -C $(KBUILD) M=${PWD} clean

distclean: clean
	rm -f $(NAME).spec cov.* *~
	rm -rf rpmbuild

$(NAME).spec: build/$(NAME).spec.in
	sed \
		-e 's/@VERSION@/'${VERSION}'/g' \
		-e 's/@RELEASE@/'${RELEASE}'/g' \
		-e 's/@NAME@/'${NAME}'/g' build/$(NAME).spec.in > $(NAME).spec
ifneq (,$(BASEVERSION))
	if [ -e .git ]; then \
		echo '%changelog' >> $(NAME).spec; \
		git log --no-merges v$(BASEVERSION)..HEAD --format="* %cd <%ae>%n- %s%n" \
		| sed 's/-[0-9][0-9][0-9][0-9] //' \
		| sed 's/ [0-9][0-9]:[0-9][0-9]:[0-9][0-9]//' \
		>> $(NAME).spec; \
	fi
endif

dist: distclean $(NAME).spec
	rm -rf /tmp/$(NAME)-$(VERSION)
	mkdir -p /tmp/$(NAME)-$(VERSION)
	mkdir -p /tmp/$(NAME)-$(VERSION)/rdmavt
	mkdir -p /tmp/$(NAME)-$(VERSION)/krping
	cp -r . /tmp/$(NAME)-$(VERSION)
	cp -r $(RDMAVT_PATH) /tmp/$(NAME)-$(VERSION)/
	cp -r $(KRPING_PATH) /tmp/$(NAME)-$(VERSION)/
	tar $(EXCLUDES) -C /tmp -zcvf $(PWD)/$(NAME)-$(VERSION).tgz ./$(NAME)-$(VERSION)
	rm -rf /tmp/$(NAME)-$(VERSION)

install:
	mkdir -p $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	mkdir -p $(RPM_BUILD_ROOT)/usr/lib/udev/rules.d
	mkdir -p $(RPM_BUILD_ROOT)/etc/init.d
	mkdir -p $(RPM_BUILD_ROOT)/etc/sysconfig
	mkdir -p $(RPM_BUILD_ROOT)/etc/opa-fm
	install hfi2.ko $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	install rdmavt/rdmavt.ko $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	# ignore errors when these two modules not built by using prep-verbs
	-install user/hfi2_user.ko $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	-install kfi/kfi.ko $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	-install krping/rdma_krping.ko $(RPM_BUILD_ROOT)/lib/modules/$(KVER)/updates
	install --mode=644 build/80-hfi2.rules $(RPM_BUILD_ROOT)/usr/lib/udev/rules.d/
	install build/$(NAME).rc $(RPM_BUILD_ROOT)/etc/init.d/$(NAME)

# these targets used by rpm specfile to package hfi2 + rdmavt
driver: hfi2 rdmavt krping
driver-ifs: hfi2-ifs rdmavt

rpm: dist
	mkdir -p rpmbuild/SOURCES
	mkdir -p rpmbuild/BUILD
	mkdir -p rpmbuild/SRPMS
	mkdir -p rpmbuild/RPMS
	mv hfi2-$(VERSION).tgz rpmbuild/SOURCES
	rpmbuild -ba \
		--define "_topdir ${PWD}/rpmbuild" \
		--define 'require_kver $(KVER)' \
		--define 'have_kdir $(KBUILD)' \
		hfi2.spec

version:
	@echo git $(GIT_VERSION) $(GIT_EXTRA_FLAGS)
	@echo $(KVER) $(KBUILD)
	@echo $(NAME) $(MVERSION)

test: version
