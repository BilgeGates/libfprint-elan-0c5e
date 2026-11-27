/* SPDX-License-Identifier: LGPL-2.1-or-later */
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#define ELAN_LEGACY_INFO_SIZE 64
#define ELAN_LEGACY_INFO_FULL_SIZE 71
#define ELAN_LEGACY_UID_MAX 57
#define ELAN_LEGACY_DELETE_SIZE 72

/* Frame: opcode, four zero bytes, UID length, FP1 UID, zero padding. */
static inline int
elan_legacy_build_identity (uint8_t opcode, const uint8_t *uid, size_t uid_size,
                            uint8_t output[ELAN_LEGACY_DELETE_SIZE])
{
  if ((opcode != 0x11 && opcode != 0x13) || uid_size < 24 ||
      uid_size > ELAN_LEGACY_UID_MAX || memcmp (uid, "FP1-", 4) ||
      memchr (uid, 0, uid_size))
    return -1;
  memset (output, 0, ELAN_LEGACY_DELETE_SIZE);
  output[0] = 0x40;
  output[1] = 0xff;
  output[2] = opcode;
  output[7] = (uint8_t) uid_size;
  memcpy (output + 8, uid, uid_size);
  return 0;
}
