#!/bin/bash

echo "Building list of all possible registers and their value"
rm -f register_val_list.txt
awk '/#define/ {if (NF > 2) { $1=""; print $0}}' ../include/wfr/* > register_val_list.txt

echo "Generating header file"
echo "#ifndef DEF_CHIP_REG" > chip_registers.h
echo "#define DEF_CHIP_REG" >> chip_registers.h

# Add copyright
cat >> chip_registers.h << 'COPYRIGHT'

/*
 *
 * This file is provided under a dual BSD/GPLv2 license.  When using or
 * redistributing this file, you may do so under either license.
 *
 * GPL LICENSE SUMMARY
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * BSD LICENSE
 *
 * Copyright(c) 2015 Intel Corporation.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *  - Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *  - Neither the name of Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

COPYRIGHT

egrep "#define\s+\S+\s+" ../include/wfr/wfr_core_defs.h >> chip_registers.h
echo "" >> chip_registers.h

# Look up the register values for the registers we want.
for reg in `cat register_minimal_set.txt`; do
	val=`egrep "^ $reg\s+" register_val_list.txt`
	if [[ $? -ne 0 ]]; then
		echo "ERROR: $reg not defined!"
		exit 1
	fi

	echo $val | awk '{ if (length($0) > 71) { 
				print "#define " $1 " \\";
				print "\t\t"$2;
			   } else { 
			    	print "#define " $0; 
			   }
			}' >> chip_registers.h
done

echo "" >> chip_registers.h
echo "#endif          /* DEF_CHIP_REG */" >> chip_registers.h

echo "Removing WFR_ from the header file"
# Need to rename WFR_DEBUG becuase if we do not then we end up with
# a define for DEBUG which will screw up the tracing for DEBUG level
sed -i 's/WFR_DEBUG/CHIP_DEBUG/' chip_registers.h

# Other than the above special case(s) blow away WFR in the file.
sed -i 's/WFR_//g' chip_registers.h

echo "Moving chip_register.h to build dir"
mv chip_registers.h ../



