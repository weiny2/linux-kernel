// SPDX-License-Identifier: Apache-2.0
/*
 *  Copyright 2010-2019, Intel Corporation
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <errno.h>
#include "jhi.h"
#include "teemanagement.h"
#include "kdi_cmd_defs.h"
#include "dal_err.h"

#define INTEL_SD_UUID "BD2FBA36A2D64DAB9390FF6DA2FEF31C"
#define APP_ID "00000000000000000000000000000001"
#define APP_ACP_FILE "Bist.acp"

static const char *test_dal_dev = "/dev/dal_test0";

#ifdef INSTALL_APPLET
static bool check_tee(JHI_RET status, const char *step)
{
	if (status != TEE_STATUS_SUCCESS) {
		fprintf(stderr, " %s failed. Status: %s\n",
			step, tee_err_to_str(status));
		return false;
	}

	printf(" %s succeeded.\n", step);
	return true;
}

static bool check_jhi(JHI_RET status, const char *step)
{
	if (status != JHI_SUCCESS) {
		fprintf(stderr, " %s failed. Status: %s\n",
			step, dal_err_to_str(status));
		return false;
	}
	printf(" %s succeeded.\n", step);
	return true;
}

static int install_applet(const uint8_t *app_blob, size_t app_blob_size)
{
	SD_SESSION_HANDLE sd_session_handle = NULL;
	JHI_RET status = JHI_UNKNOWN_ERROR;

	printf("Opening Intel SD session...");
	status = TEE_OpenSDSession(INTEL_SD_UUID, &sd_session_handle);
	if (!check_tee(status, "TEE_OpenSDSession"))
		return status;

	printf("Installing the production BIST applet...");
	status = TEE_SendAdminCmdPkg(sd_session_handle,
				     app_blob, app_blob_size);
	if (!check_tee(status, "TEE_SendAdminCmdPkg"))
		return status;

	printf("Closing the Intel SD session...");
	status = TEE_CloseSDSession(&sd_session_handle);
	if (!check_tee(status, "TEE_CloseSDSession"))
		return status;

	return 0;
}

static int uninstall_applet(void)
{
	JHI_HANDLE jhi_handle;
	JHI_RET status = JHI_UNKNOWN_ERROR;

	printf("Initializing JHI...");
	status = JHI_Initialize(&jhi_handle, 0, 0);
	if (!check_jhi(status, "JHI_Initialize"))
		return status;

	printf("Uninstalling...");
	status = JHI_Uninstall(jhi_handle, APP_ID);
	if (!check_jhi(status, "JHI_Uninstall"))
		return status;

	printf("Deinitilizing JHI...");
	status = JHI_Deinit(jhi_handle);
	if (!check_jhi(status, "JHI_Deinit"))
		return status;

	return 0;
}
#else

static int install_applet(const uint8_t *app_blob __attribute__((unused)),
			  size_t app_blob_size  __attribute__((unused)))
{
	return -1;
}

static int uninstall_applet(void)
{
	return 0;
}

#endif

static bool check_kdi(int status, const char *step)
{
	if (status) {
		printf(" %s failed. Status: %s\n",
		       step, kdi_err_to_str(status));
		return false;
	}

	printf(" %s succeeded.\n", step);
	return true;
}

static ssize_t read_app_blob(const char *fname, uint8_t **app_blob)
{
	int fd;
	struct stat st;
	ssize_t ret;
	uint8_t *blob = NULL;

	fd = open(fname, O_RDONLY);
	if (fd < 0)
		return -errno;

	if (fstat(fd, &st) != 0) {
		ret  = -errno;
		goto out;
	}

	blob = calloc(st.st_size, sizeof(uint8_t));
	if (!blob) {
		ret = -ENOMEM;
		goto out;
	}

	ret = read(fd, blob, st.st_size);
	if (ret != st.st_size) {
		ret = -errno;
		goto out;
	}

	*app_blob = blob;
	blob = NULL;

out:
	free(blob);
	close(fd);
	return ret;
}

static int open_session(int *test_mod_fd, uint64_t *session_handle,
			uint8_t *app_blob, size_t app_blob_size)
{
	struct session_create_cmd *session_create_cmd;
	struct session_create_resp session_create_resp;
	struct kdi_test_command *cmd;
	ssize_t bytes;
	int offset = 0;
	int status = 0;
	int fd;
	uint8_t *buff;
	uint32_t buff_size = sizeof(struct kdi_test_command)
					+ sizeof(*session_create_cmd)
					+ strlen(APP_ID) + 1
					+ app_blob_size;

	fd = open(test_dal_dev, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "Failed to open dal test module. errno=%d\n",
			errno);
		return -errno;
	}

	buff = (uint8_t *)malloc(buff_size);
	if (!buff) {
		status = -errno;
		goto out;
	}

	memset(buff, 0, buff_size);
	cmd = (struct kdi_test_command *)buff;
	cmd->cmd_id = KDI_SESSION_CREATE;

	session_create_cmd = (struct session_create_cmd *)cmd->data;
	session_create_cmd->is_session_handle_ptr = 1;
	session_create_cmd->app_id_len = strlen(APP_ID) + 1;
	session_create_cmd->acp_pkg_len = app_blob_size;
	session_create_cmd->init_param_len = 0;

	memcpy(session_create_cmd->data + offset, APP_ID, strlen(APP_ID) + 1);
	offset += strlen(APP_ID) + 1;
	memcpy(session_create_cmd->data + offset, app_blob, app_blob_size);
	offset += app_blob_size;

	printf("Send open session command...");
	bytes = write(fd, buff, buff_size);
	if (bytes < 0) {
		printf("Failed to write messagee. errno=%d\n", errno);
		status = -errno;
		goto out;
	}
	printf("succeeded.\n");

	// receive open session response
	printf("Receive open session response...");
	bytes = read(fd, &session_create_resp, sizeof(session_create_resp));
	if (bytes < 0) {
		printf("Failed to read message. errno=%d\n", errno);
		status = errno;
		goto out;
	}
	if (session_create_resp.test_mod_status) {
		status = session_create_resp.test_mod_status;
		printf("test_mod_status=%d\n", status);
		goto out;
	}

	status = session_create_resp.status;
	if (!check_kdi(status, "create session"))
		goto out;

	*session_handle = session_create_resp.session_handle;
	*test_mod_fd = fd;
	status = 0;

out:
	if (status)
		close(fd);
	free(buff);
	return status;
}

int send_and_receive(int test_mod_fd, uint64_t session_handle)
{
	int status = 0;
	const char *send_buffer = "0123456";
	size_t output_size = strlen(send_buffer);

	uint8_t *rcv_buffer = NULL;
	uint8_t *buff = NULL;
	uint32_t buff_size = sizeof(struct kdi_test_command) +
			     sizeof(struct send_and_rcv_cmd) +
			     output_size;

	struct send_and_rcv_resp *send_and_rcv_resp;
	struct send_and_rcv_cmd *send_and_rcv_cmd;
	struct kdi_test_command *cmd;

	buff = (uint8_t *)malloc(buff_size);
	if (!buff) {
		fprintf(stderr, "malloc failed\n");
		return 1;
	}

	cmd = (struct kdi_test_command *)buff;
	cmd->cmd_id = KDI_SEND_AND_RCV;

	send_and_rcv_cmd = (struct send_and_rcv_cmd *)cmd->data;
	send_and_rcv_cmd->session_handle = session_handle;
	send_and_rcv_cmd->command_id = 1; /* Command Id 1 for echo test */
	send_and_rcv_cmd->output_buf_len = output_size;
	send_and_rcv_cmd->is_output_buf = 1;
	send_and_rcv_cmd->is_output_len_ptr = 1;
	send_and_rcv_cmd->is_response_code_ptr = 1;
	memcpy(send_and_rcv_cmd->input, send_buffer, output_size);

	rcv_buffer = (uint8_t *)malloc(100);
	if (!rcv_buffer) {
		fprintf(stderr, "malloc failed\n");
		status = 1;
		goto out;
	}

	// Communicate with the applet
	// send msg to the applet
	printf("Send massage to the applet...");
	status = write(test_mod_fd, buff, buff_size);
	if (status < 0) {
		printf("Failed to write messagee. errno=%d\n", errno);
		goto out;
	}
	printf("succeeded\n");

	// get response from the applet
	printf("Get response from the applet...");
	status = read(test_mod_fd, rcv_buffer, 100);
	if (status < 0) {
		printf("Failed to read messagee. errno=%d\n", errno);
		goto out;
	}

	send_and_rcv_resp = (struct send_and_rcv_resp *)rcv_buffer;
	if (send_and_rcv_resp->test_mod_status) {
		status = send_and_rcv_resp->test_mod_status;
		printf("test_mod_status=%d\n", status);
		goto out;
	}
	status = send_and_rcv_resp->status;
	if (!check_kdi(status, "send and receive"))
		goto out;

	if (send_and_rcv_resp->output_len != output_size) {
		printf("did not get the expected message from the applet\n");
		status = 1;
		goto out;
	}

	if (memcmp(send_buffer, send_and_rcv_resp->output,
		   send_and_rcv_resp->output_len)) {
		printf("did not get the expected message from the applet\n");
		status = 1;
		goto out;
	}
	status = 0;

out:
	free(rcv_buffer);
	free(buff);
	return status;
}

int close_session(int test_mod_fd, uint64_t session_handle)
{
	int status;
	uint8_t buff[sizeof(struct kdi_test_command) +
		     sizeof(struct session_close_cmd)];
	uint32_t buff_size = sizeof(struct kdi_test_command) +
			     sizeof(struct session_close_cmd);
	struct session_close_resp session_close_resp;

	struct kdi_test_command *cmd = (struct kdi_test_command *)buff;
	struct session_close_cmd *session_close_cmd;

	session_close_cmd = (struct session_close_cmd *)cmd->data;

	cmd->cmd_id = KDI_SESSION_CLOSE;
	session_close_cmd->session_handle = session_handle;

	printf("Send close session command...");
	status = write(test_mod_fd, buff, buff_size);
	if (status < 0) {
		printf("Failed to write messagee. errno=%d\n", errno);
		return status;
	}
	printf("succeeded\n");

	printf("Receive close session command...");
	status = read(test_mod_fd, &session_close_resp,
		      sizeof(session_close_resp));
	if (status < 0) {
		printf("Failed to read message. errno=%d\n", errno);
		return status;
	}

	if (session_close_resp.test_mod_status) {
		printf("test_mod_status=%d\n",
		       session_close_resp.test_mod_status);
		return session_close_resp.test_mod_status;
	}

	status = session_close_resp.status;
	check_kdi(status, "close session");

	return status;
}

int main(int argc, char *argv[])
{
	int status;
	int test_mod_fd = -1;
	uint64_t session_handle = 0;
	ssize_t app_blob_size;
	uint8_t *app_blob = NULL;

	app_blob_size = read_app_blob(APP_ACP_FILE, &app_blob);
	if (app_blob_size < 0)
		goto out;

	/*
	 * OPEN SESSION OVER KDI
	 */
	status = open_session(&test_mod_fd, &session_handle,
			      app_blob, app_blob_size);
	if (status) {
		// If failed, maybe the applet is not installed
		/*
		 * INSTALL APPLET USING JHI
		 */
		status = install_applet(app_blob, app_blob_size);
		if (status)
			return status;
		/*
		 * OPEN SESSION OVER KDI
		 */
		status = open_session(&test_mod_fd, &session_handle,
				      app_blob, app_blob_size);
		if (status)
			goto out;
	}

	/*
	 * COMMUNICATE WITH THE APPLET
	 */
	status = send_and_receive(test_mod_fd, session_handle);
	/*
	 * CLOSE THE SESSION
	 */
	status = close_session(test_mod_fd, session_handle);

out:
	if (test_mod_fd != -1)
		close(test_mod_fd);

	/*
	 * OPTIONAL: UNINSTALL THE APPLET USING JHI
	 */
	uninstall_applet();

	return status;
}
