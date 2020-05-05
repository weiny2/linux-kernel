// SPDX-License-Identifier: GPL-2.0-only
/*
 * keylocker.c, validating the CPU-internal key management
 */
#undef _GNU_SOURCE
#define _GNU_SOURCE 1
#undef __USE_GNU
#define __USE_GNU 1
#undef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 64
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <err.h>
#include <inttypes.h>
#include <sched.h>
#include <setjmp.h>
#include <signal.h>
#include <unistd.h>

#define MSR_IA32_COPY_STATUS		0x990
#define MSR_IA32_IWKEY_BACKUP_STATUS	0x991
#define KEY_SIZE			32
#define HANDLE_SIZE			64

/* Test stage */
static bool after_suspension;

/* Number of active CPUs in a system */
static long cpus;

static int nerrs;

static jmp_buf jmpbuf;

static void sethandler(int sig, void (*handle)(int, siginfo_t *, void *),
		       int flags)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = handle;
	sa.sa_flags = SA_SIGINFO | flags;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, 0))
		err(1, "sigaction");
}

static void clearhandler(int sig)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = SIG_DFL;
	sigemptyset(&sa.sa_mask);
	if (sigaction(sig, &sa, 0))
		err(1, "sigaction");
}

static void sigill(int sig, siginfo_t *si, void *ctx_void)
{
	siglongjmp(jmpbuf, 1);
}

static void rdmsr_on_cpu(uint32_t reg, int cpu, uint64_t *data)
{
	char msr_file_name[64];
	int fd;

	sprintf(msr_file_name, "/dev/cpu/%d/msr", cpu);

	fd = open(msr_file_name, O_RDONLY);
	if (fd < 0)
		err(1, "To open MSR file, need sudo privilege and msr module");

	if (pread(fd, data, sizeof(*data), reg) != sizeof(*data))
		err(1, "Read MSR file.");

	close(fd);
}

static const char key[] = "1234567890qwertyuiopasdfghjklzx";

/* Encode the 128-bit AES key to a handle */
static inline void __encode_key(char *handler)
{
	/* Load the AES key to the implicit operand */
	__asm__ __volatile__("movdqu %0, %%xmm0" :: "m" (key[0]) :);

	/* Set no restriction to the handle */
	__asm__ __volatile__("mov $0, %%eax;" :::);

	/* ENCODEKEY128 %EAX */
	__asm__ __volatile__(".byte 0xf3, 0xf, 0x38, 0xfa, 0xc0" :::);

	/* Store the handle */
	__asm__ __volatile__("movdqu %%xmm0, %0;movdqu %%xmm1, %1;\n"
		"movdqu %%xmm2, %2;movdqu %%xmm3, %3;"
		: "=m" (handler[0]), "=m" (handler[0x10]),
		  "=m" (handler[0x20]), "=m" (handler[0x30]) ::);
}

static int encode_key(char *key_handle)
{
	int ret = 0;

	sethandler(SIGILL, sigill, 0);

	if (!sigsetjmp(jmpbuf, 1))
		__encode_key(key_handle);
	else
		ret = 1;

	clearhandler(SIGILL);
	return ret;
}

static char cpu0_handle[HANDLE_SIZE] = { 0 };

/*
 * This test makes sure the CPU-internal key is the same across CPUs. While
 * the value is not readable, check it indirectly by comparing handles with
 * the same AES key.
 */
static int test_iwkey(void)
{
	int cpu;

	printf("Testing the CPU-internal key consistency between CPUs\n");

	for (cpu = 0; cpu < cpus; ++(cpu)) {
		char this_handle[HANDLE_SIZE] = { 0 };
		cpu_set_t mask;
		uint64_t data;

		CPU_ZERO(&mask);
		CPU_SET(cpu, &mask);
		sched_setaffinity(0, sizeof(cpu_set_t), &mask);

		if (encode_key(this_handle)) {
			if (after_suspension) {
				printf("[FAIL]\tKeyLocker disabled\n");
				nerrs++;
			} else {
				printf("\tKeyLocker disabled\n");
			}
			return 1;
		}

		if (!cpu && !after_suspension) {
			/* We save the first handle */
			memcpy(cpu0_handle, this_handle, HANDLE_SIZE);
		} else if (memcmp(cpu0_handle, this_handle, HANDLE_SIZE)) {
			/* Any mismatch from the first handle is an error */
			printf("[FAIL]\tInconsistent IWKey at CPU%d\n", cpu);
			nerrs++;
			return 1;
		}

		rdmsr_on_cpu(MSR_IA32_IWKEY_BACKUP_STATUS, cpu, &data);
		if (!(data & 0x1)) {
			printf("[FAIL]\tInvalid saved internal key status "
			       "at CPU%d\n", cpu);
			nerrs++;
			return 1;
		}

		rdmsr_on_cpu(MSR_IA32_COPY_STATUS, cpu, &data);
		if (!(data & 0x1)) {
			printf("[FAIL]\tInvalid CPU-internal key status at"
			       " CPU%d\n", cpu);
			nerrs++;
			return 1;
		}
	}

	printf("[OK]\tCPU-internal key looks to be okay\n");
	return 0;
}

static void switch_to_sleep(void)
{
	int fd;

	printf("Transition to Suspend-To-RAM state\n");

	fd = open("/sys/power/mem_sleep", O_RDWR);
	if (fd < 0)
		err(1, "Open /sys/power/mem_sleep");

	if (write(fd, "deep", strlen("deep")) != strlen("deep"))
		err(1, "Write /sys/power/mem_sleep");
	close(fd);

	fd = open("/sys/power/state", O_RDWR);
	if (fd < 0)
		err(1, "Open /sys/power/state");

	if (write(fd, "mem", strlen("mem")) != strlen("mem"))
		err(1, "Write /sys/power/state");
	close(fd);

	printf("Wake up from Suspend-To-RAM state\n");
}

int main(void)
{
	cpus = sysconf(_SC_NPROCESSORS_ONLN);

	printf("%ld CPUs in the system\n", cpus);

	/*
	 * During the boot, the kernel loads and saves a new CPU-internal
	 * key and restores it to all the other CPUs.
	 */
	after_suspension = false;
	if (test_iwkey())
		goto out;

	switch_to_sleep();

	/*
	 * When the system wakes up, the kernel restores the internal key
	 * for all the CPUs.
	 */
	after_suspension = true;
	test_iwkey();

out:
	return nerrs ? 1 : 0;
}
