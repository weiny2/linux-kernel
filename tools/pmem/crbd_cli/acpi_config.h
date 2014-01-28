#ifndef ACPI_CONFIG_H_
#define ACPI_CONFIG_H_

#include <linux/nvdimm.h>

#define LOG_NORM(fmt, ...)	\
	printf("%s: " fmt "\n", LEVEL, ## __VA_ARGS__)
#define LOG_ERR(fmt, ...)	\
	printf("%s-ERR:%s:%d: " fmt "\n", LEVEL, __FILE__, \
	    __LINE__, ## __VA_ARGS__)
#ifdef DEBUG
#define LOG_DBG(fmt, ...)    \
	printf("%s-DBG:%s:%d: " fmt "\n", LEVEL, __FILE__, \
	    __LINE__, ## __VA_ARGS__)
#else
#define LOG_DBG(fmt, ...)
#endif

/* XXX: estimate the size needed for the structure */
#define NVDIMM_CONFIG_SIZE 16384

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

void *create_test_fit(char *fit_str[]);
void *create_default_fit(void *dev_info);
void *read_nvdimm_fit_file(char *);

#endif
