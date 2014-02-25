
#ifndef CRBD_IOCTL_H_
#define CRBD_IOCTL_H_

#include <linux/types.h>
#include <linux/nvdimm_ioctl.h>
#include <linux/cr_ioctl.h>

int check_root(void);
int crbd_ioctl_translate_addr(__u64 *spa, __u64 *rdpa, __u16 pid,
	__u8 direction, __u8 region_type);
int crbd_ioctl_load_fit(char *nvdimm_fit_file);
int crbd_ioctl_dimm_init();
int crbd_ioctl_get_topology(__u8 count, struct nvdimm_topology *dimm_topo);
int crbd_identify_dimm(int dimm_handle);
int crbd_get_security(int dimm_handle, char *security_status);
int crbd_set_nonce(int dimm_handle);
int crbd_set_passphrase(int dimm_handle, char *curr_ph, char *new_ph);
int crbd_disable_passphrase(int dimm_handle, char *curr_ph);
int crbd_unlock_unit(int dimm_handle, char *curr_ph);
int crbd_erase_prepare(int dimm_handle);
int crbd_erase_unit(int dimm_handle, char *curr_ph);
int crbd_freeze_lock(int dimm_handle);
int crbd_ioctl_pass_thru(struct fv_fw_cmd *fw_cmd);
int crbd_ioctl_topology_count(void);
#endif /* CRBD_IOCTL_H_ */
