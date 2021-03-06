/*
 * Copyright 2017, Linaro Ltd and contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __CMD_H
#define __CMD_H
#include <stdint.h>
#include <stdbool.h>

#include <io.h>

/* Zone encoding, this is typically param1 */
enum {
	ZONE_CONFIG = 0,
	ZONE_OTP,
	ZONE_DATA,
	ZONE_END
};

#define ZONE_CONFIG_SIZE	88
#define ZONE_OTP_SIZE		64
#define ZONE_DATA_SIZE		512

#define ZONE_CONFIG_NUM_WORDS  22
#define ZONE_DATA_NUM_SLOTS    16
#define ZONE_OTP_NUM_WORDS     16

#define LOCK_CONFIG_LOCKED	0x0
#define LOCK_CONFIG_UNLOCKED	0x55
#define LOCK_DATA_LOCKED	0x0
#define LOCK_DATA_UNLOCKED	0x55

#define TEMPKEY_SOURCE_RANDOM	0
#define TEMPKEY_SOURCE_INPUT	1

#define HMAC_LEN		32
#define RANDOM_LEN		32
#define DEVREV_LEN		4
#define SERIALNUM_LEN		9
#define MAC_LEN			32
#define SHA_LEN			32

#define WORD_SIZE		4
#define MAX_READ_SIZE		32 /* bytes */
#define MAX_WRITE_SIZE		32

#define MAC_MODE_TEMPKEY_SOURCE_SHIFT  2
#define MAC_MODE_USE_OTP_88_BITS_SHIFT 4
#define MAC_MODE_USE_OTP_64_BITS_SHIFT 5
#define MAC_MODE_USE_SN_SHIFT          6

#define NONCE_MODE_RANDOM		0
#define NONCE_MODE_RANDOM_NO_SEED	1
#define NONCE_MODE_PASSTHROUGH		3

#define SHA_MODE_INIT          0x00
#define SHA_MODE_COMPUTE       0x01

/* OP-codes for each command, see section 8.5.4 in spec */
#define OPCODE_DERIVEKEY	0x1c
#define OPCODE_DEVREV		0x30
#define OPCODE_GENDIG		0x15
#define OPCODE_HMAC		0x11
#define OPCODE_CHECKMAC		0x28
#define OPCODE_LOCK		0x17
#define OPCODE_MAC		0x08
#define OPCODE_NONCE		0x16
#define OPCODE_PAUSE		0x01
#define OPCODE_RANDOM		0x1b
#define OPCODE_READ		0x02
#define OPCODE_SHA		0x47
#define OPCODE_UPDATEEXTRA	0x20
#define OPCODE_WRITE		0x12

/* Addresses etc for the configuration zone. */
#define OTP_CONFIG_ADDR		0x4
#define OTP_CONFIG_OFFSET	0x2
#define OTP_CONFIG_SIZE		0x1

#define SERIALNBR_ADDR0_3	0x0
#define SERIALNBR_OFFSET0_3	0x0
#define SERIALNBR_SIZE0_3	0x4

#define SERIALNBR_ADDR4_7	0x2
#define SERIALNBR_OFFSET4_7	0x0
#define SERIALNBR_SIZE4_7	0x4

#define SERIALNBR_ADDR8		0x3
#define SERIALNBR_OFFSET8	0x0
#define SERIALNBR_SIZE8		0x1

#define LOCK_DATA_ADDR		0x15
#define LOCK_DATA_OFFSET	0x2
#define LOCK_DATA_SIZE		0x1

#define LOCK_CONFIG_ADDR	0x15
#define LOCK_CONFIG_OFFSET	0x3
#define LOCK_CONFIG_SIZE	0x1

/*
 * Base address for slot configuration starts at 0x5. Each word contains slot
 * configuration for two slots.
 */
uint8_t SLOT_CONFIG_ADDR(uint8_t slotnbr);

#define SLOT_DATA_SIZE         32

#define SLOT_ADDR(id) (8 * id)
#define SLOT_CONFIG_OFFSET(slotnbr) (slotnbr % 2 ? 2 : 0)
#define SLOT_CONFIG_SIZE 0x2

#define OTP_ADDR(addr) (4 * addr)

uint8_t cmd_read(struct io_interface *ioif, uint8_t zone, uint8_t addr,
		 uint8_t offset, size_t size, void *data, size_t data_size);

uint8_t cmd_derive_key(struct io_interface *ioif, uint8_t random, uint8_t slotnbr,
		       uint8_t *buf, size_t size);

uint8_t cmd_check_mac(struct io_interface *ioif, uint8_t *in, size_t in_size,
		      uint8_t mode, uint16_t slotnbr, uint8_t *out, size_t out_size);

uint8_t cmd_get_devrev(struct io_interface *ioif, uint8_t *buf, size_t size);

uint8_t cmd_get_hmac(struct io_interface *ioif, uint8_t mode, uint16_t slotnbr,
		     uint8_t *hmac);

uint8_t cmd_lock_zone(struct io_interface *ioif, uint8_t zone,
		      const uint16_t *expected_crc);

uint8_t cmd_get_mac(struct io_interface *ioif, const uint8_t *in, size_t in_size,
		    uint8_t mode, uint16_t slotnbr, uint8_t *out, size_t out_size);

uint8_t cmd_get_nonce(struct io_interface *ioif, const uint8_t *in, size_t in_size,
		      uint8_t mode, uint8_t *out, size_t out_size);

uint8_t cmd_get_random(struct io_interface *ioif, uint8_t mode, uint8_t *buf,
		       size_t size);

uint8_t cmd_pause(struct io_interface *ioif, uint8_t selector);

uint8_t cmd_sha(struct io_interface *ioif, uint8_t mode, const uint8_t *in,
		size_t in_size, uint8_t *out, size_t out_size);

uint8_t cmd_update_extra(struct io_interface *ioif, uint8_t mode, uint8_t value);

uint8_t cmd_gen_dig(struct io_interface *ioif, const uint8_t *in, size_t in_size,
		    uint8_t zone, uint16_t slotnbr);

uint8_t cmd_write(struct io_interface *ioif, uint8_t zone, uint8_t addr,
		  bool encrypted, const uint8_t *data, size_t size);
#endif
