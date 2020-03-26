// SPDX-License-Identifier: GPL-2.0
/*
 * Tests x86 MKTME Multi-Key Memory Protection APIs
 *
 * COMPILE w keyutils library ==>  cc -o mktest mktme_test.c -lkeyutils
 *
 * Test requires capability of CAP_SYS_RESOURCE, or CAP_SYS_ADMIN.
 * $ sudo setcap 'CAP_SYS_RESOURCE+ep' mktest
 *
 * Tests that need to reset the garbage collection delay (gc_delay)
 * require root privileges. See remove_gc_delay().
 *
 * There are examples in here of:
 *  * how to use the Kernel Key Service MKTME API to allocate keys
 *  * how to use the MKTME Memory Encryption API to encrypt memory
 *
 * Adding Tests:
 *	o Each test should run independently and clean up after itself.
 *	o There should be no dependencies among tests.
 *	o Tests that use a lot of keys, should consider adding sleep(),
 *	  so that the next test isn't key-starved.
 *	o Make no assumptions about the order in which tests will run.
 *	o There are shared defines that can be used for setting
 *	  payload options.
 */
#define _GNU_SOURCE
#include <errno.h>
#include <sys/syscall.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <keyutils.h>

#define mktme_assert(condition) do {					\
	if (!(condition)) {						\
		printf("%s: Line:%d Error: %s \n",		\
		       __func__, __LINE__, strerror(errno));			\
	}								\
} while (0)

#define ARRAY_SIZE(x) (sizeof(x) / sizeof(*(x)))
#define PAGE_SIZE sysconf(_SC_PAGE_SIZE)

/*  TODO Perhaps get this from new keyctl describe_type option
 *       or, perhaps trying to get max keys is not valid selftest
 */
int max_keyids = 63;

key_serial_t testmaster;	/*
				 * Useful for most encrypted key adds
				 * Added in main()
				 */
/*
 * Let caller decide if failure to add the key is really an error.
 * Callers reference this key by the returned serial number.
 */
key_serial_t add_test_key()
{

	char name[23];
	char *mktme_options = "new mktme user:testmaster 36";

	sprintf(name, "mktme_%d", rand() % 1000000);

	/* Add an encrypted key with format MKTME */
	return add_key("encrypted", name, mktme_options,
			  strlen(mktme_options), KEY_SPEC_THREAD_KEYRING);
}

/* Do not use this helper if error return is needed. */
void invalidate_test_key(key_serial_t key)
{
	int ret = 0;

	if (key > 0)
		ret = syscall(SYS_keyctl, KEYCTL_INVALIDATE, key);
	mktme_assert(!ret);
}

/*
 * Set the garbage collection delay to 0, so that keys are quickly
 * available for re-use while running the selftests.
 *
 * Most tests use INVALIDATE to remove a key, which has no delay by
 * design. Using REVOKE will lead to this delay.
 *
 * TODO - maybe don't need this unless doing REVOKE testing
 */
char current_gc_delay[10] = {0};
static inline int remove_gc_delay(void)
{
	int fd;

	fd = open("/proc/sys/kernel/keys/gc_delay",
		  O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("Failed open /proc/sys/kernel/keys/gc_delay");
		return -1;
	}
	if (read(fd, current_gc_delay, sizeof(current_gc_delay)) <= 0) {
		perror("Failed read /proc/sys/kernel/keys/gc_delay");
		close(fd);
		return -1;
	}
	lseek(fd, 0, SEEK_SET);
	if (write(fd, "0", sizeof(char)) != sizeof(char)) {
		perror("Failed write temp_gc_delay to gc_delay\n");
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
}

static inline void restore_gc_delay(void)
{
	int fd;

	fd  = open("/proc/sys/kernel/keys/gc_delay",
		   O_RDWR | O_NONBLOCK);
	if (fd < 0) {
		perror("Failed open /proc/sys/kernel/keys/gc_delay");
		return;
	}
	if (write(fd, current_gc_delay, strlen(current_gc_delay)) !=
	    strlen(current_gc_delay)) {
		perror("Failed to restore gc_delay\n");
		close(fd);
		return;
	}
	close(fd);
}

void test_encrypted_keys(void)
{
	char *mktme_options = "new mktme user:themaster 36";
	char *master_payload = "nothing";
	key_serial_t master_key;
	key_serial_t enc_key;
	key_serial_t enc_key_reload;
	void *read_blob_1;
	void *read_blob_2;
	char *reload_blob;
	int size;

	/* Add a master key */
	master_key = add_key("user", "themaster", master_payload,
			     strlen(master_payload), KEY_SPEC_THREAD_KEYRING);
	if (master_key < 0) {
		fprintf(stderr, "add master_key failed: %s\n", strerror(errno));
		return;
	}

	/* Add an encrypted key with format MKTME */
	enc_key = add_key("encrypted", "mktme_enc", mktme_options,
			  strlen(mktme_options), KEY_SPEC_THREAD_KEYRING);
	if (enc_key < 0) {
		fprintf(stderr, "add_key failed: %s\n", strerror(errno));
		goto out_master;
	}

	/* Save the encrypted blob */
	size = keyctl_read_alloc(enc_key, &read_blob_1);
	if (size == -1) {
		fprintf(stderr, "keyctl_read_alloc failed: %s\n",
			strerror(errno));
		goto out_enc_key;
	}

	/* Prepare the saved blob for reloading */
	size += strlen("load ");
	reload_blob = malloc(size);
	memcpy(reload_blob, "load ", strlen("load "));
	strcat(reload_blob, read_blob_1);

	/* Reload the encrypted key using the saved blob */
	enc_key_reload = add_key("encrypted", "mktme_enc_reload", reload_blob,
				 size, KEY_SPEC_THREAD_KEYRING);
	if (enc_key_reload < 0) {
		fprintf(stderr, "reload key failed: %s\n", strerror(errno));
		goto out_reload_blob;
	}

	/* Save the 2nd blob */
	size = keyctl_read_alloc(enc_key_reload, &read_blob_2);
	if (size == -1) {
		fprintf(stderr, "keyctl_read_alloc failed: %s\n",
			strerror(errno));
		goto out;
	}

	/* The blobs should match */
	if (strncmp(read_blob_1, read_blob_2, size))
		fprintf(stderr, "Blobs don't match\n");

out:
	free(read_blob_2);
	invalidate_test_key(enc_key_reload);
out_reload_blob:
	free(reload_blob);
	free(read_blob_1);
out_enc_key:
	invalidate_test_key(enc_key);
out_master:
	invalidate_test_key(master_key);
}

struct tlist {
	const char *name;
	void (*func)();
};

static const struct tlist mtests[] = {
{"Keys: Add,Store,Reload Encrypted Key",test_encrypted_keys		},
};

void print_usage(void)
{
	fprintf(stderr, "Usage: mktme_test [options]...\n"
		"  -a			Run all tests\n"
		"  -t <testnum>		Run one <testnum> test\n"
		"  -i <iterations>	Repeat test(s) iterations\n"
		"  -l			List available tests\n"
		"  -h, -?		Show this help\n"
	       );
}

int main(int argc, char *argv[])
{
	int test_selected = -1;
	int iterations = 1;
	int trace = 0;
	int i, c;
	char *test, *iter;

	/*
	 * TODO: Default case needs to run 'selftests' -  a
	 * curated set of tests that validate functionality but
	 * don't fall over if keys are not available.
	 */
	while ((c = getopt(argc, argv, "at:i:lph?")) != -1) {
		switch (c) {
		case 'a':
			test_selected = -1;
			printf("Test Selected [ALL]\n");
			break;
		case 't':
			test_selected = strtoul(optarg, &test, 10);
			printf("Test Selected [%d]\n", test_selected);
			break;
		case 'l':
			for (i = 0; i < ARRAY_SIZE(mtests); i++)
				printf("[%2d] %s\n", i + 1,
				       mtests[i].name);
			exit(EXIT_SUCCESS);
			break;
		case 'p':
			trace = 1;
			break;
		case 'i':
			iterations = strtoul(optarg, &iter, 10);
			printf("Iterations [%d]\n", iterations);
			break;
		case 'h':
		case '?':
		default:
			print_usage();
			exit(EXIT_FAILURE);
		}
	}

/*
 *	if (!cpu_has_mktme()) {
 *		printf("MKTME not supported on this system.\n");
 *		exit(EXIT_FAILURE);
 *	}
 */
	/* Remove the garbage collection delay for all tests */
	if (remove_gc_delay()) {
		fprintf(stderr, "Cannot set gc_delay.\n");
		exit(EXIT_FAILURE);
	}

	if (trace) {
		printf("Pausing: start trace on PID[%d]\n",
		       (int)getpid());
		getchar();
	}

	srand((unsigned int)time(NULL) - getpid());
	
	/* Use testmaster key for most encrypted keys */
	char *master_payload = "nothing";
	testmaster = add_key("user", "testmaster", master_payload,
			     strlen(master_payload), KEY_SPEC_THREAD_KEYRING);
        if (testmaster < 0)
                fprintf(stderr, "add master_key failed: %s\n", strerror(errno));

	while (iterations-- > 0) {
		if (test_selected == -1) {
			for (i = 0; i < ARRAY_SIZE(mtests); i++) {
				printf("[%2d] %s\n", i + 1,
				       mtests[i].name);
				mtests[i].func();
			}
			printf("\nTests Complete\n");

		} else {
			if (test_selected <= ARRAY_SIZE(mtests)) {
				printf("[%2d] %s\n", test_selected,
				       mtests[test_selected - 1].name);
				mtests[test_selected - 1].func();
				printf("\nTest Complete\n");
			}
		}
	printf("Iterations Remaining: %d\n\n", iterations);
	sleep(1);
	}
	restore_gc_delay();
	exit(EXIT_SUCCESS);
}
