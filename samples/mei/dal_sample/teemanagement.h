/* SPDX-License-Identifier: Apache-2.0 */
/*
 *  Copyright 2010-2019, Intel Corporation
 */
#ifndef __TEE_MANAGEMENT_H__
#define __TEE_MANAGEMENT_H__

#include "typedefs.h"
#include "dal_tee_metadata.h"
#include "dal_acp_encryption.h"

typedef void *SD_SESSION_HANDLE; //SD session handle
#define UUID_STR_LEN 33
/* Represents a UUID as string with NULL terminator */
typedef char UUID_STR[UUID_STR_LEN];

typedef struct {
	uint8_t uuidCount;
	UUID_STR *uuids;
} UUID_LIST; // Represents a list of UUIDs

/*** TEE Management return codes ***/
typedef enum _TEE_STATUS
{
	// General errors
	TEE_STATUS_SUCCESS               = 0x0000,  // Operation completed successfully
	TEE_STATUS_INTERNAL_ERROR        = 0x2001,  // Something went wrong with DAL itself
	TEE_STATUS_INVALID_PARAMS        = 0x2002,  // An operation was called with illegal arguments, for example, a null pointer.
	TEE_STATUS_INVALID_HANDLE        = 0x2003,  // Invalid Security Domain (SD) handle
	TEE_STATUS_INVALID_UUID          = 0x2004,  // The Security Domain UUID is invalid
	TEE_STATUS_NO_FW_CONNECTION      = 0x2005,  // JHI service can't communicate with the VM in the FW. This might be a JHI configuration error, a HECI driver problem or a FW problem
	TEE_STATUS_UNSUPPORTED_PLATFORM  = 0x2006,  // The desired operation is not supported by the current platform

	// Service errors
	TEE_STATUS_SERVICE_UNAVAILABLE   = 0x2100,  // The application cannot connect to the JHI service. The service might be down.
	TEE_STATUS_REGISTRY_ERROR        = 0x2101,  // An error occurred during a registry access attempt or registry corruption detected
	TEE_STATUS_REPOSITORY_ERROR      = 0x2102,  // Cannot find the applets repository directory on the file system
	TEE_STATUS_SPOOLER_MISSING       = 0x2103,  // Cannot find the SpoolerApplet.dalp file
	TEE_STATUS_SPOOLER_INVALID       = 0x2104,  // The Spooler applet was found, but an error occurred while trying to install it in the VM and communicate with it
	TEE_STATUS_PLUGIN_MISSING        = 0x2105,  // teePlugin.dll, bhPlugin.dll or bhPluginV2.dll was not found. Should be in the same folder as jhi_service.exe.
	TEE_STATUS_PLUGIN_VERIFY_FAILED  = 0x2106,  // The signature or publisher name of teePlugin.dll, bhPlugin.dll or bhPluginV2.dll are not valid

	// Package errors
	TEE_STATUS_INVALID_PACKAGE   = 0x2200,  // Invalid Admin Command Package
	TEE_STATUS_INVALID_SIGNATURE = 0x2201,  // Signature verification failed
	TEE_STATUS_MAX_SVL_RECORDS   = 0x2202,  // Max records allowed in security version list (SVL) exceeded

	// Install / uninstall TA errors:
	TEE_STATUS_CMD_FAILURE_SESSIONS_EXISTS = 0x2300,  // Operation cannot be executed because there are open sessions
	TEE_STATUS_CMD_FAILURE                 = 0x2301,  // Failed to load Admin Command Package to the FW
	TEE_STATUS_MAX_TAS_REACHED             = 0x2302,  // Max number of allowed applets exceeded, an applet has to be uninstalled
	TEE_STATUS_MISSING_ACCESS_CONTROL      = 0x2303,  // The Admin Command Package needs more permissions in order to be loaded. It is not allowed to use a needed JAVA class or package
	TEE_STATUS_TA_DOES_NOT_EXIST           = 0x2304,  // The Admin Command Package (ACP) file path is incorrect
	TEE_STATUS_INVALID_TA_SVN              = 0x2305,  // ACP loading failed due to a failed on Security Version Number (SVN) check
	TEE_STATUS_IDENTICAL_PACKAGE           = 0x2306,  // The loaded package is identical to an existing one
	TEE_STATUS_ILLEGAL_PLATFORM_ID         = 0x2307,  // The provided platform ID is invalid
	TEE_STATUS_SVL_CHECK_FAIL              = 0x2308,  // Install failed due to an svl check
	TEE_STATUS_TA_PLATFORM_MISMATCH        = 0x2309,  // TA incompatible for platform.

	// SD errors
	TEE_STATUS_SD_INTERFCE_DISABLED          = 0x2400,  // OEM singing is disabled
	TEE_STATUS_SD_PUBLICKEY_HASH_VERIFY_FAIL = 0x2401,  // Mismatch in public key hash of an SD
	TEE_STATUS_SD_DB_NO_FREE_SLOT            = 0x2402,  // No free slot to install SD
	TEE_STATUS_SD_TA_INSTALLATION_UNALLOWED  = 0x2403,  // TA installation is not allowed for SD
	TEE_STATUS_SD_TA_DB_NO_FREE_SLOT         = 0x2404,  // No free slot to install TA for SD
	TEE_STATUS_SD_INVALID_PROPERTIES         = 0x2405,  // Incorrect properties in the SD manifest
	TEE_STATUS_SD_SD_DOES_NOT_EXIST          = 0x2406,  // Tried to use an SD that is not installed
	TEE_STATUS_SD_SD_INSTALL_UNALLOWED       = 0x2407,  // Tried to install a SD that is not pre-allowed in the FW

	// Applet encryption errors
	TEE_STATUS_PLATFORM_AFTER_EOM        = 0x2500,  // Operation is not allowed after the End of Manufacturing is set in the FW
	TEE_STATUS_MAX_INVOCATIONS           = 0x2501,  // Operation invocation quota exceeded
	TEE_STATUS_COUNTER_MISMATCH          = 0x2502,  // Obsolete counter value was used
	TEE_STATUS_TA_ENCRYPTION_KEY_NOT_SET = 0x2503,  // Can't install/create session to an encrypted applet without setting a TA encryption key
	TEE_STATUS_OMK_NOT_PROVISIONED       = 0x2504,  // Can't set TA encryption key before provisioning an OEM Master Key
	TEE_STATUS_TA_ENCRYPTION_KEY_INVALID = 0x2505   // The TA encryption key was rejected by the FW (wrong structure/encryption)

} TEE_STATUS;

/*** Export APIs ***/

//------------------------------------------------------------------------------
// Function: TEE_OpenSDSession
//               First interface to be called to perform operations with an SD.
// IN     : sdId - a string representation of a UUID without the '-' delimiters.
// OUT    : sdHandle - Reference to an SD session handle that will be initialized and returned to the caller to be used in future calls.
// RETURN : TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------
TEE_STATUS TEE_OpenSDSession (
	IN const char *sdId,
	OUT SD_SESSION_HANDLE* sdHandle
);

//------------------------------------------------------------------------------
// Function: TEE_CloseSDSession
//           This interface closes the SD session.
// INOUT : sdHandle - Reference to the SD session handle.
// RETURN : TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------
TEE_STATUS TEE_CloseSDSession (
	INOUT SD_SESSION_HANDLE* sdHandle
);

//------------------------------------------------------------------------------
// Function: TEE_SendAdminCmdPkg
//           This interface send an admin command package to a specific SD session.
// IN     : sdHandle - The SD session handle.
// IN     : cmdPkg - a buffer containing the ACP.
// IN     : cmdPkgSize - The package size.
// RETURN : TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------
TEE_STATUS TEE_SendAdminCmdPkg (
	IN const SD_SESSION_HANDLE sdHandle,
	IN const uint8_t *cmdPkg,
	IN uint32_t cmdPkgSize
);

//------------------------------------------------------------------------------
// Function: TEE_ListInstalledTAs
//          This interface send an admin command package to a specific SD session.
// IN       : sdHandle - The SD session handle.
// OUT      : uuid_list - The structure containing the UUIDs as a string representations without the '-' delimiters..
//          The user should pass a reference to an existing struct and the library will fill it.
//          In order to free the inner array containing the UUIDs (uuid_list.uuids), the user should use the TEE_DEALLOC API.
// RETURN : TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------

TEE_STATUS TEE_ListInstalledTAs (
	IN const SD_SESSION_HANDLE sdHandle,
	OUT UUID_LIST *uuid_list
);

//------------------------------------------------------------------------------
// Function: TEE_ListInstalledSDs
//              This interface send an admin command package to a specific SD session.
// IN           : sdHandle - The SD session handle.
// OUT          : uuid_list - The structure containing the UUIDs as a string representations without the '-' delimiters..
//              The user should pass a reference to an existing struct and the library will fill it.
//              In order to free the inner array containing the UUIDs (uuid_list.uuids), the user should use the TEE_DEALLOC API.
// RETURN       : TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------

TEE_STATUS TEE_ListInstalledSDs(
	IN const SD_SESSION_HANDLE sdHandle,
	OUT UUID_LIST *uuid_list
);

//------------------------------------------------------------------------------
// Function: TEE_QueryTEEMetadata
//          This interface is used to retrieve version numbers and general info on the DAL VM from the FW.
// IN        : sdHandle - The SD session handle. Currently not used.
// OUT     `   : metadata - A struct that will hold the result
// RETURN `: TEE_STATUS - success or any failure returns
//------------------------------------------------------------------------------

TEE_STATUS TEE_QueryTEEMetadata (
	IN const SD_SESSION_HANDLE sdHandle,
	OUT dal_tee_metadata *metadata
);

//------------------------------------------------------------------------------
// Function: TEE_ProvisionOemMasterKey
// This interface is used to provision an OEM Master Key (OMK).
// IN : sdHandle - The SD session handle. Currently not used.
// IN : key - A pointer to a struct containing the RSA key exponent.
// RETURN : TEE_STATUS_SUCCESS - Success.
//          TEE_STATUS_SD_INTERFACE_DISABLED - DAL OEM signing is not enabled on the platform.
//          TEE_STATUS_PLATFORM_AFTER_EOM - The End Of Manufacturing Field Programmable Fuse is already set on the platform.
//------------------------------------------------------------------------------

TEE_STATUS TEE_ProvisionOemMasterKey(
	IN const SD_SESSION_HANDLE sdHandle,
	IN const tee_asym_key_material *key
);

//------------------------------------------------------------------------------
// Function: TEE_SetTAEncryptionKey
//    This interface is used to set a DAL Encryption Key (DEK).
// IN  : sdHandle - The SD session handle. Currently not used.
// OUT  : metadata - A struct that will hold the result
// RETURN : TEE_STATUS_SUCCESS - Success.
//     TEE_STATUS_SD_INTERFACE_DISABLED: DAL OEM Signing is not enabled on the platform.
//     TEE_STATUS_MAX_INVOCATIONS: The command has been invoked more than allowed by throttling.
//     TEE_STATUS_COUNTER_MISMATCH: The command has an obsolete counter value.
//     TEE_STATUS_SIGNATURE_VERIFY_FAILED: The command signature is incorrect.
//     TEE_STATUS_OMK_NOT_PROVISIONED: Need to call TEE_ProvisionOemMasterKey first.
//     TEE_STATUS_TA_ENCRYPTION_KEY_INVALID: Wrong key structure/encryption. Key rejected by the FW.
//------------------------------------------------------------------------------

TEE_STATUS TEE_SetTAEncryptionKey(
	IN  const SD_SESSION_HANDLE  sdHandle,
	IN const tee_key_material *key
);

//------------------------------------------------------------------------------
// Function: TEE_DEALLOC
//    This interface is used to free memory that was allocated by the TEE management library.
// IN  : handle - A handle to the memory buffer.
//------------------------------------------------------------------------------

void TEE_DEALLOC(void *handle);

#endif //__TEE_MANAGEMENT_H__
