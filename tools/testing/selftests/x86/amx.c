// SPDX-License-Identifier: GPL-2.0

#define _GNU_SOURCE
#include <err.h>
#include <elf.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#include <malloc.h>
#include <unistd.h>

#include <linux/futex.h>

#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/ptrace.h>
#include <sys/shm.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/uio.h>

#include <x86intrin.h>

#ifndef __x86_64__
# error This test is 64-bit only
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

#define CTXT_SW_ITERATIONS		50
#define NUM_THREADS			100

#define XSAVE_CPUID			0x1
#define XSAVE_ECX_BIT			26

#define XSTATE_CPUID			0xd
#define XSTATE_USER_STATE_SUBLEAVE	0x0
#define TILE_CPUID			0x1d
#define TILE_PALETTE_CPUID_SUBLEAVE	0x1

#define XFEATURE_XTILE_CFG  17
#define XFEATURE_XTILE_DATA 18
#define XFEATURE_MASK_XTILE ((1 << XFEATURE_XTILE_DATA) | \
			    (1 << XFEATURE_XTILE_CFG))

#define TILE_SIZE			1024
#define NUM_TILES			8
#define PAGE_SIZE			(1<<12)
#define XSAVE_SIZE			(NUM_TILES * TILE_SIZE + PAGE_SIZE)
#define MAX_TILES			16
#define RESERVED_BYTES			14

struct xsave_data {
	u8 area[XSAVE_SIZE];
} __attribute__((aligned(64)));

struct tile_config {
	u8  palette_id;
	u8  start_row;
	u8  reserved[RESERVED_BYTES];
	u16 colsb[MAX_TILES];
	u8  rows[MAX_TILES];
};

struct xtile_info {
	struct {
		u16 bytes_per_tile;
		u16 bytes_per_row;
		u16 max_names;
		u16 max_rows;
	} config;

	struct {
		u32 xsave_offset;
		u32 xsave_size;
	} data;
};

struct tile_data {
	u8 data[NUM_TILES * TILE_SIZE];
};

static inline u64 __xgetbv(u32 index)
{
	u32 eax, edx;

	asm volatile(".byte 0x0f,0x01,0xd0"
		     : "=a" (eax), "=d" (edx)
		     : "c" (index));
	return eax + ((u64)edx << 32);
}

static inline void __cpuid(u32 *eax, u32 *ebx, u32 *ecx, u32 *edx)
{
	asm volatile("cpuid;"
		     : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
		     : "0" (*eax), "2" (*ecx));
}

static inline void __ldtilecfg(void *cfg)
{
	asm volatile(".byte 0xc4,0xe2,0x78,0x49,0x00"
		     : : "a"(cfg));
}

static inline void __tileloadd(void *tile)
{
	asm volatile(".byte 0xc4,0xe2,0x7b,0x4b,0x04,0x10"
		     : : "a"(tile), "d"(0));
}

static inline void __xsave(void *area, u32 lo, u32 hi)
{
	asm volatile(".byte 0x48,0x0f,0xae,0x27"
		     : : "D" (area), "a" (lo), "d" (hi)
		     : "memory");
}

static inline void __xrstor(void *area, u32 lo, u32 hi)
{
	asm volatile(".byte 0x48,0x0f,0xae,0x2f"
		     : : "D" (area), "a" (lo), "d" (hi));
}

static inline void __tilerelease(void)
{
	asm volatile(".byte 0xc4, 0xe2, 0x78, 0x49, 0xc0" ::);
}

static int nerrs;
static struct xtile_info xtile;

static int __enum_xtile_config(void)
{
	u32 eax, ebx, ecx, edx;
	u16 bytes_per_tile;

	eax = TILE_CPUID;
	ecx = TILE_PALETTE_CPUID_SUBLEAVE;

	__cpuid(&eax, &ebx, &ecx, &edx);
	if (!eax || !ebx || !ecx)
		return 1;

	xtile.config.max_names = ebx >> 16;
	if (xtile.config.max_names != NUM_TILES)
		return 1;

	bytes_per_tile = eax >> 16;
	if (bytes_per_tile != TILE_SIZE)
		return 1;

	xtile.config.bytes_per_row = ebx;
	xtile.config.max_rows = ecx;

	return 0;
}

static int __enum_xsave_tile(void)
{
	u32 eax, ebx, ecx, edx;

	eax = XSTATE_CPUID;
	ecx = XFEATURE_XTILE_DATA;

	__cpuid(&eax, &ebx, &ecx, &edx);
	if (!eax || !ebx)
		return 1;

	xtile.data.xsave_offset = ebx;
	xtile.data.xsave_size = eax;

	return 0;
}

static int __check_xsave_size(void)
{
	u32 eax, ebx, ecx, edx;

	eax = XSTATE_CPUID;
	ecx = XSTATE_USER_STATE_SUBLEAVE;

	__cpuid(&eax, &ebx, &ecx, &edx);
	if (!ebx || (ebx > XSAVE_SIZE))
		return 1;

	return 0;
}

static inline void *__get_xsave_tile_data_addr(void *data)
{
	return data + xtile.data.xsave_offset;
}

static inline bool check_availability(void)
{
	u32 eax, ebx, ecx, edx;

	eax = XSAVE_CPUID;
	ecx = 0;

	__cpuid(&eax, &ebx, &ecx, &edx);
	if (!(ecx & 1<<XSAVE_ECX_BIT))
		return false;

	if (__xgetbv(0) & XFEATURE_MASK_XTILE)
		return true;

	return false;
}

static int config_tiles(struct tile_config *cfg)
{
	int i;

	memset(cfg, 0, sizeof(*cfg));
	cfg->palette_id = 1;
	for (i = 0; i < xtile.config.max_names; i++) {
		cfg->colsb[i] = xtile.config.bytes_per_row;
		cfg->rows[i] = xtile.config.max_rows;
	}

	return 0;
}

static void activate_tiles(struct tile_config *cfg, void *data)
{
	__ldtilecfg(cfg);

	memset(data, 0xff, xtile.data.xsave_size);
	__tileloadd(data);
}

static void deactivate_tiles(void)
{
	__tilerelease();
}

static void load_tiles(void *data)
{
	__xrstor(data, -1, -1);
}

static void store_tiles(void *data)
{
	__xsave(data, -1, -1);
}

static void generate_tile_data(void *tiles)
{
	u32 seed, iterations;
	u32 *ptr = tiles;
	int i;

	iterations = xtile.data.xsave_size / sizeof(u32);
	seed = time(NULL);

	for (i = 0, ptr = tiles; i < iterations; i++, ptr++)
		*ptr  = (u32)rand_r(&seed);
}

static void copy_tile_data(void *data, void *tiles)
{
	void *dst = __get_xsave_tile_data_addr(data);

	memcpy(dst, tiles, xtile.data.xsave_size);
}

static int validate(void *data, void *tiles)
{
	void *tile_data = __get_xsave_tile_data_addr(data);

	if (memcmp(tile_data, tiles, xtile.data.xsave_size))
		return 1;

	return 0;
}

static int enum_xtile_info(void)
{
	int ret;

	ret = __check_xsave_size();
	if (ret)
		return 1;

	ret = __enum_xsave_tile();
	if (ret)
		return 1;

	ret = __enum_xtile_config();
	if (ret)
		return 1;

	if (sizeof(struct tile_data) < xtile.data.xsave_size)
		return 1;

	return 0;
}

#define WAIT(cur) \
	do { sched_yield(); } \
	while (syscall(SYS_futex, finfo->futex, \
		       FUTEX_WAIT, cur, 0, 0, 42))

#define WAKE(next) \
	do { \
		*(finfo->futex) = next; \
		while (!syscall(SYS_futex, finfo->futex, \
				FUTEX_WAKE, 1, 0, 0, 42)) \
			sched_yield(); \
	} while (0)

#define GET_ITERATIVE_NUM(id) ((id << 1) & ~0x1)

#define GET_ENDING_NUM(id) ((id << 1) | 0x1)

struct futex_info {
	int current;
	int next;
	int *futex;
};

static void *child(void *info)
{
	struct futex_info *finfo = (struct futex_info *)info;
	struct xsave_data data;
	struct tile_data tiles;
	struct tile_config cfg;
	int i;

	if (config_tiles(&cfg))
		err(1, "tile configuration");

	activate_tiles(&cfg, &tiles);
	store_tiles(&data);

	generate_tile_data(&tiles);
	copy_tile_data(&data, &tiles);
	load_tiles(&data);

	for (i = 0; i < CTXT_SW_ITERATIONS; i++) {
		WAIT(GET_ITERATIVE_NUM(finfo->current));

		memset(&data, 0, sizeof(data));
		store_tiles(&data);
		nerrs += validate(&data, &tiles);

		generate_tile_data(&tiles);
		copy_tile_data(&data, &tiles);
		load_tiles(&data);

		WAKE(GET_ITERATIVE_NUM(finfo->next));
	}

	WAIT(GET_ENDING_NUM(finfo->current));

	deactivate_tiles();
	return NULL;
}

static int create_children(int num, struct futex_info *finfo)
{
	const int shm_id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0666);
	int *futex = shmat(shm_id, NULL, 0);
	pthread_t thread;
	int i;

	for (i = 0; i < num; i++) {
		finfo[i].futex = futex;
		finfo[i].current = i + 1;
		finfo[i].next = (i + 2) % (num + 1);

		if (pthread_create(&thread, NULL, child, &finfo[i])) {
			err(1, "pthread_create");
			return 1;
		}
	}
	return 0;
}

static int test_context_switch(void)
{
	struct futex_info *finfo;
	cpu_set_t cpuset;
	int i;

	printf("[RUN]\t%u context switches of AMX states in %d threads\n",
			CTXT_SW_ITERATIONS * NUM_THREADS, NUM_THREADS);

	nerrs = 0;
	CPU_ZERO(&cpuset);
	CPU_SET(0, &cpuset);

	finfo = malloc(sizeof(*finfo) * NUM_THREADS);

	if (create_children(NUM_THREADS, finfo))
		return 1;

	for (i = 0; i < CTXT_SW_ITERATIONS; i++) {
		WAKE(GET_ITERATIVE_NUM(1));
		WAIT(GET_ITERATIVE_NUM(0));
	}

	for (i = 1; i <= NUM_THREADS; i++)
		WAKE(GET_ENDING_NUM(i));

	if (nerrs) {
		printf("[FAIL]\t%u incorrect AMX states seen\n", nerrs);
		return 1;
	}

	printf("[OK]\tall AMX states are correct\n");
	return 0;
}

#define GET_AMX_STATE(child, data) \
	ptrace(PTRACE_GETREGSET, child, (u32)NT_X86_XSTATE, &data)

#define SET_AMX_STATE(child, data) \
	ptrace(PTRACE_SETREGSET, child, (u32)NT_X86_XSTATE, &data)

static int request_amx_state_write(bool load_tilecfg, pid_t child)
{
	struct xsave_data data;
	struct tile_data tiles;
	struct iovec iov;

	generate_tile_data(&tiles);

	iov.iov_base = &data;
	iov.iov_len = sizeof(data);

	if (GET_AMX_STATE(child, iov))
		err(1, "PTRACE_GETREGSET");

	copy_tile_data(&data, &tiles);

	if (SET_AMX_STATE(child, iov))
		err(1, "PTRACE_SETREGSET");

	memset(&data, 0, sizeof(data));

	if (GET_AMX_STATE(child, iov))
		err(1, "PTRACE_GETREGSET");

	if (!load_tilecfg)
		memset(&tiles, 0, sizeof(tiles));

	return validate(&data, &tiles);
}

static void test_amx_state_write(bool load_tilecfg)
{
	pid_t child;
	int status;

	child = fork();
	if (child < 0)
		err(1, "fork");

	if (!child) {
		printf("[RUN]\tPTRACE writes AMX state,");

		if (ptrace(PTRACE_TRACEME, 0, NULL, NULL))
			err(1, "PTRACE_TRACEME");

		if (load_tilecfg) {
			struct tile_data tiles;
			struct tile_config cfg;

			if (config_tiles(&cfg))
				err(1, "tile configuration");
			activate_tiles(&cfg, &tiles);
			printf("\twith tile configuration\n");
		} else {
			printf("\twithout tile configuration\n");
		}

		raise(SIGTRAP);
		_exit(0);
	}

	do {
		wait(&status);
	} while (WSTOPSIG(status) != SIGTRAP);


	if (request_amx_state_write(load_tilecfg, child)) {
		nerrs += 1;
		printf("[FAIL]\t");
		if (load_tilecfg)
			printf("incorrect write\n");
		else
			printf("unexpected write\n");
	} else {
		printf("[OK]\t");
		if (load_tilecfg)
			printf("correct write\n");
		else
			printf("no write\n");
	}

	ptrace(PTRACE_DETACH, child, NULL, NULL);
}

static void test_ptrace(void)
{
	bool load_tilecfg;

	load_tilecfg = true;
	test_amx_state_write(load_tilecfg);
	load_tilecfg = false;
	test_amx_state_write(load_tilecfg);
}

int main(void)
{
	int ret;

	if (!check_availability()) {
		printf("\tAMX state is not available\n");
		return 0;
	}

	ret = enum_xtile_info();
	if (ret) {
		printf("\tAMX state size is different from what expected\n");
		return 0;
	}

	test_context_switch();
	test_ptrace();

	return nerrs ? 1 : 0;
}
