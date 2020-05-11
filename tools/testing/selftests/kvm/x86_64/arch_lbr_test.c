// SPDX-License-Identifier: GPL-2.0
/*
 * Power-on test for Architectural LBRs VMCS Fields support.
 *
 * Copyright 2018, Intel Inc.
 *
 * Author:
 *  Like Xu <like.xu@intel.com>
 */
#define _GNU_SOURCE /* for program_invocation_short_name */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/syscall.h>

#include "test_util.h"

#include "kvm_util.h"
#include "processor.h"

#define MSR_ARCH_LBR_CTL		0x000014ce
#define VCPU_ID	      1

uint64_t rdmsr_on_cpu(uint32_t reg, int cpu)
{
	uint64_t data;
	int fd;
	char msr_file[64];

	sprintf(msr_file, "/dev/cpu/%d/msr", cpu);
	fd = open(msr_file, O_RDONLY);
	if (fd < 0)
		exit(KSFT_SKIP);

	if (pread(fd, &data, sizeof data, reg) != sizeof data)
		exit(KSFT_SKIP);

	close(fd);
	return data;
}

void guest_code(void)
{
        unsigned long i, a = 0;

        wrmsr(MSR_ARCH_LBR_CTL, 0xf);

        for (i = 0; i < 99; i++)
                a++;

        /* trigger vm-exit */
        GUEST_SYNC(0);

        ASSERT_EQ(a, 99);

        /* check "Load Guest IA32_LBR_CTL" entry control.*/
        ASSERT_EQ(rdmsr(MSR_ARCH_LBR_CTL), 0xf);

        GUEST_DONE();
}

int main(int argc, char *argv[])
{
        struct kvm_cpuid_entry2 *entry;
        struct kvm_vm *vm;
        struct ucall uc;
        int cpu = syscall(SYS_getcpu);

        entry = kvm_get_supported_cpuid_index(0x7, 0);
        if (!((entry->edx >> 19) & 1ULL)) {
                printf("Architectural LBR not supported, skipping test.\n");
                return 0;
        }

        vm = vm_create_default(VCPU_ID, 0, guest_code);
        vcpu_set_cpuid(vm, VCPU_ID, kvm_get_supported_cpuid());

        for (;;) {
                ASSERT_EQ(rdmsr_on_cpu(MSR_ARCH_LBR_CTL, cpu), 0);

                vcpu_run(vm, VCPU_ID);

                /* check "Guest IA32_LBR_CTL" state field. */
                ASSERT_EQ(vcpu_get_msr(vm, VCPU_ID, MSR_ARCH_LBR_CTL), 0xf);

                switch (get_ucall(vm, VCPU_ID, &uc)) {
                case UCALL_SYNC:
                        /* check "Clear IA32_LBR_CTL" exit control support*/
                        ASSERT_EQ(rdmsr_on_cpu(MSR_ARCH_LBR_CTL, cpu), 0);
			continue;
                case UCALL_DONE:
                        goto done;
                default:
                        TEST_ASSERT(false, "Unknown ucall 0x%lx.", uc.cmd);
                }
        }

done:
        kvm_vm_free(vm);
        return 0;
}