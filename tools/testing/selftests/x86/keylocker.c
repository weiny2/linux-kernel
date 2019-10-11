// SPDX-License-Identifier: GPL-2.0-only
/*
 * keylocker.c, validating the internal wrapping key management
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

static int nerrs;
static long proc_nb;
static bool after_sleep;

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
		err(1, "Open msr file. Make sure sudo and 'msr' module");

	if (pread(fd, data, sizeof(*data), reg) != sizeof(*data))
		err(1, "Read msr file.");

	close(fd);
}

static const char key[] = "1234567890qwertyuiopasdfghjklzx";

/* Wrap the 128-bit AES key to a handle */
static inline void __encode_key(char *handler)
{
	/* Load the AES key to the implicit operand */
	__asm__ __volatile__("movdqu %0, %%xmm0" :: "m" (key[0]) :);

	/* Set no restriction on the handle */
	__asm__ __volatile__("mov $0, %%eax;" :::);

	/* ENCODEKEY128 %EAX */
	__asm__ __volatile__(".byte 0xf3, 0xf, 0x38, 0xfa, 0xc0" :::);

	/* Store the produced handle output */
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
 * This test makes sure the internal wrapping key is the same across
 * CPUs. While the value is not readable, check it indirectly by
 * comparing handle values with the same AES key.
 */
static int test_internal_key(void)
{
	int proc_id;

	printf("Testing the internal key copy\n");

	for (proc_id = 0; proc_id < proc_nb; ++(proc_id)) {
		char this_handle[HANDLE_SIZE] = { 0 };
		cpu_set_t mask;
		uint64_t data;

		CPU_ZERO(&mask);
		CPU_SET(proc_id, &mask);
		sched_setaffinity(0, sizeof(cpu_set_t), &mask);

		if (encode_key(this_handle)) {
			if (after_sleep) {
				printf("[FAIL]\tKey Locker disabled\n");
				nerrs++;
			} else {
				printf("\tKey Locker disabled\n");
			}
			return 1;
		}

		if (!proc_id && !after_sleep) {
			/* We save the first handle */
			memcpy(cpu0_handle, this_handle, HANDLE_SIZE);
		} else if (memcmp(cpu0_handle, this_handle, HANDLE_SIZE)) {
			/* Any mismatch from the first handle is a copy error */
			printf("[FAIL]\tWrong value at CPU%d\n", proc_id);
			nerrs++;
			return 1;
		}

		rdmsr_on_cpu(MSR_IA32_IWKEY_BACKUP_STATUS, proc_id, &data);
		if (!(data & 0x1)) {
			printf("[FAIL]\tInvalid backup status at CPU%d\n",
			       proc_id);
			nerrs++;
			return 1;
		}

		rdmsr_on_cpu(MSR_IA32_COPY_STATUS, proc_id, &data);
		if (!(data & 0x1)) {
			printf("[FAIL]\tInvalid copy status at CPU%d\n",
			       proc_id);
			nerrs++;
			return 1;
		}
	}

	printf("[OK]\tthe internal key looks okay\n");
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
	proc_nb = sysconf(_SC_NPROCESSORS_ONLN);

	printf("%ld processors in the system\n", proc_nb);

	/*
	 * During the boot, the kernel loads a new internal
	 * wrapping key and copies it to all the other CPUs.
	 */
	after_sleep = false;
	if (test_internal_key())
		goto out;

	switch_to_sleep();

	/*
	 * At wake-up, the kernel restores the key in all
	 * CPUs.
	 */
	after_sleep = true;
	test_internal_key();

out:
	return nerrs ? 1 : 0;
}
