#!/bin/sh

c_source_code_dirs="common/rdma hfi2 hfi2_vnic hfi_core kfi opa-headers.git user"
leading_sharp_files="makefile opa2_hfi.spec.in"

myname=`basename $0 .sh`
tmpfile=/tmp/${myname}.$$
prologue=/tmp/${myname}.head.$$
epilogue=/tmp/${myname}.trail.$$

copyright_statement() {
	cat <<EOF
 "INTEL CONFIDENTIAL"
 Copyright (c) 2016 Intel Corporation

 The source code contained or described herein and all documents related to
 the source code ("Material") are owned by Intel Corporation or its suppliers
 or licensors. Title to the Material remains with Intel Corporation or its
 suppliers and licensors. The Material contains trade secrets and proprietary
 and confidential information of Intel or its suppliers and licensors. The
 Material is protected by worldwide copyright and trade secret laws and
 treaty provisions. No part of the Material may be used, copied, reproduced,
 modified, published, uploaded, posted, transmitted, distributed, or disclosed
 in any way without Intel's prior express written permission.

 No license under any patent, copyright, trade secret or other intellectual
 property right is granted to or conferred upon you by disclosure or delivery
 of the Materials, either expressly, by implication, inducement, estoppel or
 otherwise. Any license under such intellectual property rights must be
 express and approved by Intel in writing.
EOF
}

add_copyright() {
	file=$1
	sed -e "/^ \* Copyright(c) 2016 Intel Corporation./,//d" ${file} |\
		sed -e "/^ \* Copyright (c) 2014 Intel Corporation.  All rights reserved./,//d" |\
		sed -e "/^ \* Intel Omni-Path Architecture (OPA) Host Fabric Software/,//d" |\
		sed -e "/^ \* This file is provided under a dual BSD\/GPLv2 license.  When using or/,//d" >${prologue}
	sed -e "1,/^ \* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE./d" ${file} >${epilogue}
	sed -e "1,/^ \* SOFTWARE./d" ${file} >>${epilogue}
	if [ -s ${epilogue} ]; then # ${epilogue} is empty when there is no copyright notice in the file
		mv ${prologue} ${file}
		copyright_statement | sed -e "s/^/ */" >>${file}
		cat ${epilogue} >>${file}
	fi
	rm -f ${prologue} ${epilogue}
}

add_sharp_copyright() {
	file=$1
	mv ${file} ${tmpfile}
	echo "#" >${file}
	copyright_statement | sed -e "s/^/#/" >>${file}
	echo "#" >>${file}
	cat ${tmpfile} >>${file}
}

create_PRE_RELEASE_NOTICE() {
	cat <<EOF >$1
NOTICE
This software is provided as "pre-release" software for the purpose of your
testing and evaluation to enable your qualification schedules to align
closely with the final Intel public release. Pre-release means the software
which has not completed final validation, may contain bugs or functional
deficiencies, and may be modified by or for Intel prior to final release.

The provided software is not intended for public disclosure, use in a
production environment, or use by any end customer or anyone other
than your internal software evaluation personnel.

You understand, acknowledge and agree that:
(i) pre-release software may not be fully tested and may contain bugs,
errors or functional deficiencies;
(ii) pre-release software is not suitable for commercial release in
its current state;
(iii) regulatory approvals for pre-release software (such as UL or FCC)
have not been obtained, and pre-release software may therefore not be
certified for use in certain countries or environments and
(iv) Intel can provide no assurance that it will ever produce or make
generally available a production version of the pre-release software.

Intel is not under any obligation to develop and/or release or offer
for sale or license a final product based upon the pre-release software and
may unilaterally elect to abandon the pre-release software or any such
development platform at any time and without any obligation or liability
whatsoever to You or any other person.

Intel Corporation requests that you do not release, publish, or distribute
this pre-release software until you are specifically authorized. These terms
are deleted upon publication of this software.
EOF
}

# main start here
find ${c_source_code_dirs} -name "*.[h|c]" | \
while read file; do
	echo ${file}:
	add_copyright ${file}
done

# makefile
file=makefile
sed -i -e "1,37d" ${file}

# opa2_hfi.spec.in"
file=opa2_hfi.spec.in
sed -i -e "/^#.*$/d" ${file}

find ${leading_sharp_files} | \
while read file; do
	echo ${file}:
	add_sharp_copyright ${file}
done

# create PRE_RELEASE_NOTICE in every directory
find . -type d |\
	grep --invert-match "/.git" |\
	grep --invert-match "rpmbuild" |\
while read dir; do
	echo create ${dir}/PRE_RELEASE_NOTICE
	create_PRE_RELEASE_NOTICE ${dir}/PRE_RELEASE_NOTICE
done

exit 0
