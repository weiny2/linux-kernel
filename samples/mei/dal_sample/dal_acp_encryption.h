/*
   Copyright 2010-2019 Intel Corporation

   This software is licensed to you in accordance
   with the agreement between you and Intel Corporation.

   Alternatively, you can use this file in compliance
   with the Apache license, Version 2.


   Apache License, Version 2.0

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

   http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#ifndef _DAL_ACP_ENCRYPTION_H_
#define _DAL_ACP_ENCRYPTION_H_

#if !defined(_WIN32) && !defined(__linux__)
#include <cse_basic_types.h>
#endif

#define TEE_RSA_2048_KEY_LEN_BYTES      (256)
#define TEE_RSA_3072_KEY_LEN_BYTES      (384)
#define TEE_RSA_4096_KEY_LEN_BYTES      (512)

#define TEE_RSA_MAX_KEY_LEN_BYTES       (512)
#define TEE_RSA_MAX_SIG_LEN_BYTES       (TEE_RSA_MAX_KEY_LEN_BYTES)
#define TEE_RSA_EXPONENT_LEN_BYTES	    (sizeof(uint32_t))

#define TEE_KEY_MAGIC_LEN               (4)
#define TEE_KEY_MAGIC_VALUE             ("DAEK")
#define TEE_KEY_MAX_LEN_BYTES           (64)

#define TEE_MIN_KEY_ID                  (1)
#define TEE_MAX_KEY_ID                  (5)
#define TEE_INVALD_KEY_ID               (0)

#define TEE_RESERVED_BYTE               (0)
#define TEE_SDID_LENGTH                 (16)

#define TEE_MIN_BYTECODE_LENGTH         (6)

#define TEE_AES_GCM_256_KEY_LEN_BYTES       (32)
#define TEE_AES_GCM_256_IV_LEN_BYTES        (12)
#define TEE_AES_GCM_256_MIN_TAG_LEN_BYTES   (12)
#define TEE_AES_GCM_256_MAX_TAG_LEN_BYTES   (16)


#pragma pack(1)

typedef enum _tee_encryption_alg_types_t
{
    tee_encryption_alg_unknown   = 0,
    tee_encryption_alg_pkcs      = 1

} tee_encryption_alg_types_t;

typedef enum _tee_cipher_types_t
{
    tee_cipher_unknown          = 0,
    tee_cipher_aes_gcm_256      = 1

} tee_cipher_types_t;

typedef struct _tee_asym_key_material
{
    uint32_t                    initial_counter; // the initial counter value for AR protection
    tee_encryption_alg_types_t  alg_type; // the RSA algorithm type associated with the key
    uint32_t                    key_length; // the size of the key modulus in bytes
    uint32_t                    public_exponent; // the RSA public exponent E
    uint8_t                     modulus[TEE_RSA_MAX_KEY_LEN_BYTES]; // the RSA modulus N
    uint8_t                     private_exponent[TEE_RSA_MAX_KEY_LEN_BYTES]; // the RSA private exponent D

} tee_asym_key_material;

typedef struct _tee_key_material
{
    uint8_t                     sig_sd_id[TEE_SDID_LENGTH]; // signer security domain ID
    uint32_t                    key_id; // the ID of the symmetric DEKn key
    uint32_t                    counter; // the value of the AR counter
    tee_cipher_types_t          cipher_type; // the DEKn cipher type
    uint8_t                     reserved[8]; // reserved for future use
    uint8_t                     encrypted_key[TEE_RSA_MAX_KEY_LEN_BYTES]; // the DEKn encrypted with OMK
    uint8_t                     signature[TEE_RSA_MAX_SIG_LEN_BYTES]; // the signature of the Security Domain on all the above fields

} tee_key_material;

typedef struct _tee_plaintext_key
{
    uint8_t                     magic[TEE_KEY_MAGIC_LEN]; // the magic header
    uint8_t                     key[TEE_KEY_MAX_LEN_BYTES]; // the DEKn symmetric key

} tee_plaintext_key;


#pragma pack()

#ifdef _WIN32
C_ASSERT( ( sizeof(tee_asym_key_material) % 4 ) == 0 );
C_ASSERT( ( sizeof(tee_key_material) % 4 ) == 0 );
#endif

#endif      // _DAL_ACP_ENCRYPTION_H_

