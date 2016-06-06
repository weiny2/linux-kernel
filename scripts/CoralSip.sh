#!/bin/sh

c_source_code_dirs="common/rdma hfi2 hfi2_vnic hfi_core user"
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
	mv ${prologue} ${file}
	copyright_statement | sed -e "s/^/ */" >>${file}
	cat ${epilogue} >>${file}
}

add_sharp_copyright() {
	file=$1
	mv ${file} ${tmpfile}
	echo "#" >${file}
	copyright_statement | sed -e "s/^/#/" >>${file}
	echo "#" >>${file}
	cat ${tmpfile} >>${file}
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

exit 0
