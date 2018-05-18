/*
 * Copyright 2017, Linaro Ltd and contributors
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef __S96AT_H
#define __S96AT_H

#include <stdint.h>

#include <s96at_private.h>

#define S96AT_VERSION				PROJECT_VERSION

#define S96AT_STATUS_OK				0x00
#define S96AT_STATUS_CHECKMAC_FAIL		0x01
#define S96AT_STATUS_EXEC_ERROR			0x0f
#define S96AT_STATUS_READY			0x11
#define S96AT_STATUS_PADDING_ERROR		0x98
#define S96AT_STATUS_BAD_PARAMETERS		0x99

#define S96AT_WATCHDOG_TIME			1700 /* msec */

#define S96AT_CHALLENGE_LEN			32
#define S96AT_DEVREV_LEN			4
#define S96AT_GENDIG_INPUT_LEN			4
#define S96AT_HMAC_LEN				32
#define S96AT_KEY_LEN				32
#define S96AT_MAC_LEN				32
#define S96AT_NONCE_INPUT_LEN			20
#define S96AT_RANDOM_LEN			32
#define S96AT_READ_CONFIG_LEN			4
#define S96AT_READ_DATA_LEN			32
#define S96AT_READ_OTP_LEN			4
#define S96AT_SERIAL_NUMBER_LEN			9
#define S96AT_SHA_LEN				32
#define S96AT_WRITE_CONFIG_LEN			4
#define S96AT_WRITE_DATA_LEN			32
#define S96AT_WRITE_OTP_LEN			4
#define S96AT_ZONE_CONFIG_LEN			88
#define S96AT_ZONE_DATA_LEN			512
#define S96AT_ZONE_OTP_LEN			64

#define S96AT_FLAG_NONE				0x00
#define S96AT_FLAG_TEMPKEY_SOURCE_INPUT		0x01
#define S96AT_FLAG_TEMPKEY_SOURCE_RANDOM	0x02
#define S96AT_FLAG_USE_OTP_64_BITS		0x04
#define S96AT_FLAG_USE_OTP_88_BITS		0x08
#define S96AT_FLAG_USE_SN			0x10
#define S96AT_FLAG_ENCRYPT			0x12

#define	S96AT_ZONE_LOCKED			0x00
#define S96AT_ZONE_UNLOCKED			0x55

#define S96AT_OTP_MODE_LEGACY			0x00
#define S96AT_OTP_MODE_CONSUMPTION		0x55
#define S96AT_OTP_MODE_READONLY			0xAA

enum s96at_device {
	S96AT_ATSHA204A,
};

enum s96at_io_interface_type {
	S96AT_IO_I2C_LINUX
};

enum s96at_zone {
	S96AT_ZONE_CONFIG,
	S96AT_ZONE_OTP,
	S96AT_ZONE_DATA
};

struct s96at_check_mac_data {
	const uint8_t *challenge;
	uint8_t slot;
	uint32_t flags;
	const uint8_t *otp;
	const uint8_t *sn;
};

enum s96at_mac_mode {
	S96AT_MAC_MODE_0, /* 1st 32 bytes: Slot, 2nd 32 bytes: Input Challenge */
	S96AT_MAC_MODE_1, /* 1st 32 bytes: Slot, 2nd 32 bytes: TempKey */
	S96AT_MAC_MODE_2, /* 1st 32 bytes: TempKey, 2nd 32 bytes: Input Challenge */
	S96AT_MAC_MODE_3  /* 1st 32 bytes: TempKey, 2nd 32 bytes: TempKey */
};

enum s96at_nonce_mode {
	S96AT_NONCE_MODE_RANDOM,
	S96AT_NONCE_MODE_RANDOM_NO_SEED,
	S96AT_NONCE_MODE_PASSTHROUGH = 0x03
};

enum s96at_random_mode {
	S96AT_RANDOM_MODE_UPDATE_SEED,
	S96AT_RANDOM_MODE_UPDATE_NO_SEED
};

/* Check a MAC generated by another device
 *
 * Generates a MAC and compares it with the value stored in the mac buffer.
 * This is normally used to verify a MAC generated using s96at_get_mac()
 * during a challenge-response communication between a host and a client,
 * where a host sends a challenge and the client responds with a MAC that
 * is subsequently verified by the host.
 * The s96at_check_mac_data structure contains the parameters used by the
 * client to generate the response. For more information see s96at_get_mac().
 * The slot parameter specifies the slot that contains the key to use by the
 * host when calculating the response. In the flags parameter it is required
 * to specify input source of TempKey.
 *
 * Returns S96AT_STATUS_OK on success. A failed comparison returns
 * S96_STATUS_CHECKMAC_FAIL. Other errors return S96AT_EXECUTION_ERROR.
 */
uint8_t s96at_check_mac(struct s96at_desc *desc, enum s96at_mac_mode mode,
			uint8_t slot, uint32_t flags, struct s96at_check_mac_data *data,
			const uint8_t *mac);

/* Clean up a device descriptor
 *
 * Unregisters the device from the io interface.
 */
uint8_t s96at_cleanup(struct s96at_desc *desc);

/* Calculate a CRC value
 *
 * Calculates the CRC of the data stored in buf, using the same CRC-16
 * polynomial used by the device. If a non-zero value is passed to the
 * current_crc parameter, the algorithm will use that as the initial value.
 * This is useful if the CRC cannot be calculated in one go.
 *
 * Returns the calculated CRC value.
 */
uint16_t s96at_crc(const uint8_t *buf, size_t buf_len, uint16_t current_crc);

/* Derive a key
 *
 * Derives a new key by combining the value stored in a slot with a nonce
 * and storing its SHA256 hash into the target slot. Depending on how the
 * target slot is configured, this function will use the appropriate input
 * key:
 *
 * For slots configured into Create mode, the parent key is used.
 * For slots configured into Rolling mode, the current key is used.
 *
 * If required by the configuration, an authorizing MAC can be sent along,
 * through the mac buffer.
 *
 * The flags parameter must specify the input source of TempKey as defined
 * when executing the Nonce command.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_derive_key(struct s96at_desc *desc, uint8_t slot, uint8_t *mac,
			 uint32_t flags);

/* Get Device Revision
 *
 * Retrieves the device revision and stores it into the buffer pointed by
 * buf. The buffer length must be S96AT_DEVREV_LEN.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_devrev(struct s96at_desc *desc, uint8_t *buf);

/* Generate a digest
 *
 * Generate a SHA-256 digest combining the value stored in TempKey with
 * a value stored in the device. The input value is defined by the zone
 * and slot parameters. The value of data must be NULL.
 *
 * For keys configured as CheckOnly, it is possible to generate the
 * digest using the value stored in the data buffer instead of a value
 * stored in the device. This is normally used to generate euphemeral
 * keys. When this operation is required, a pointer must be passed to the
 * data parameter pointing to a buffer that contains the input data. In
 * this case, the zone and slot values are ignored.
 *
 * In both cases, the generated digest is stored in TempKey and it can
 * be used to combine the Nonce with an additional value before executing
 * the MAC / CheckMAC / HMAC commands.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_gen_digest(struct s96at_desc *desc, enum s96at_zone zone,
			 uint8_t slot, uint8_t *data);

/* Generate an HMAC-SHA256
 *
 * Generates an HMAC. To generate an HMAC, it is first required to load
 * an input challenge into TempKey using Nonce and optionally GenDigest.
 * The value in TempKey is then combined along with the key stored in slot
 * to generate an HMAC.
 *
 * Flags control whether other intput values should be included in
 * the input message. These are:
 *
 * S96AT_FLAG_USE_OTP_64_BITS	Include OTP[0:7]
 * S96AT_FLAG_USE_OTP_88_BITS	Include OTP[0:10]
 * S96AT_FLAG_USE_SN		Include SN[2:3] and SN[4:7]
 *
 * The resulting HMAC is written into the hmac buffer.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_hmac(struct s96at_desc *desc, uint8_t slot,
		       uint32_t flags, uint8_t *hmac);

/* Get the lock status of the Config Zone
 *
 * Reads the lock status of the Config Zone into lock_config.
 * Possible values are:
 *
 *  S96AT_LOCK_CONFIG_LOCKED
 *  S96AT_LOCK_CONFIG_UNLOCKED
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_lock_config(struct s96at_desc *desc, uint8_t *lock_config);

/* Get the lock status of the Data / OTP zone
 *
 * Reads the lock status of the Data / OTP zone into lock_data.
 * Possible values are:
 *
 * S96AT_LOCK_DATA_LOCKED
 * S96AT_LOCK_DATA_UNLOCKED
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_lock_data(struct s96at_desc *desc, uint8_t *lock_data);

/* Generate a MAC
 *
 * Generates a MAC. To generate a MAC, it is first required to load
 * an input challenge into TempKey using Nonce and optionally GenDigest.
 * The value in TempKey is then combined along with the key stored in slot
 * to generate a MAC.
 *
 * The mode parameter specifies which fields to use to form the input message:
 *
 * S96AT_MAC_MODE_0	1st 32 bytes from a slot, 2nd 32 bytes from the input challenge
 * S96AT_MAC_MODE_1	1st 32 bytes from a slot, 2nd 32 bytes from TempKey
 * S96AT_MAC_MODE_2	1st 32 bytes from TempKey 2nd 32 bytes from the input challenge
 * S96AT_MAC_MODE_3	1st 32 bytes from TempKey 2nd 32 bytes from TempKey
 *
 * When S96AT_MAC_MODE_2 or S96AT_MAC_MODE_3 is used, the slot parameter is ignored.
 * When S96AT_MAC_MODE_1 or S96AT_MAC_MODE_3 is used, the input challenge parameter
 * is ignored.
 *
 * Flags control whether other intput values should be included in
 * the input message. These are:
 *
 * S96AT_FLAG_USE_OTP_64_BITS	Include OTP[0:7]
 * S96AT_FLAG_USE_OTP_88_BITS	Include OTP[0:10]
 * S96AT_FLAG_USE_SN		Include SN[2:3] and SN[4:7]
 *
 * The resulting MAC is written into the mac buffer.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_mac(struct s96at_desc *desc, enum s96at_mac_mode mode, uint8_t slot,
		      const uint8_t *challenge, uint32_t flags, uint8_t *mac);

/* Generate a nonce
 *
 * Generates a nonce. The operation mode is specified by the mode
 * parameter. When operating in Passthrough mode, the input value
 * in the data buffer is stored directly in TempKey.
 * When operating in Random Mode, an input value is combined with
 * a random number and the resulting hash is stored in TempKey.
 *
 * When Random Mode is used, the default behaviour is to update
 * the seed before generating the random number. This behaviour
 * can be overriden when using S96AT_REANDOM_MODE_NO_SEED.
 *
 * The value produced by the RNG is stored into buf.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_gen_nonce(struct s96at_desc *desc, enum s96at_nonce_mode mode,
			uint8_t *data, uint8_t *random);

/* Get the OTP Mode
 *
 * Reads the OTP Mode into otp_mode. Possible values are:
 *
 * S96AT_OTP_MODE_CONSUMPTION
 * S96AT_OTP_MODE_LEGACY
 * S96AT_OTP_MODE_READ_ONLY
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_otp_mode(struct s96at_desc *desc, uint8_t *opt_mode);

/* Send the Pause command
 *
 * Upon receiving the Pause command, devices with Selector byte in the
 * configuration that do NOT match the selector parameter, will enter the
 * Idle State. This is useful for avoiding conflicts when multiple devices
 * are used on the same bus.
 *
 * A device that does not enter the idle state returns S96AT_STATUS_OK.
 */
uint8_t s96at_pause(struct s96at_desc *desc, uint8_t selector);

/* Generate a random number
 *
 * Random numbers are generated by combining the output of a hardware RNG
 * with an internally stored seed value. The generated number is stored in
 * the buffer pointed by buf. The length of the buffer must be at least
 * S96AT_RANDOM_LEN.
 *
 * Before generating a new number, the interal seed is updated by default.
 * This can be overriden by using S96AT_RANDOM_MODE_UPDATE_NO_SEED.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_random(struct s96at_desc *desc, enum s96at_random_mode mode,
			 uint8_t *buf);

/* Get the device's Serial Number
 *
 * Reads the device's Serial Number into buf. The buffer must be
 * at least S96AT_SERIALNUM_LEN long.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_serialnbr(struct s96at_desc *desc, uint8_t *serial);

/* Generate a hash (SHA-256)
 *
 * Generates a SHA-256 hash of the input message contained in buf.
 * The message length is specified in msg_len. The input buffer's length
 * must be a multiple of S96AT_SHA_BLOCK_LEN and it is modified
 * in place to contain the SHA padding, as defined in FIPS 180-2. The
 * input buffer is therefore required to have enough room for the padding.
 *
 * The resulting hash is stored in the hash buffer.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_get_sha(struct s96at_desc *desc, uint8_t *buf,
		      size_t buf_len, size_t msg_len, uint8_t *hash);

/* Initialize a device descriptor
 *
 * Selects a device and registers with an io interface. Upon successful initialization,
 * the descriptor can be used in subsequent operations.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_init(enum s96at_device device_type, enum s96at_io_interface_type iface,
		   struct s96at_desc *desc);

/* Lock a zone
 *
 * Locks a zone specified by the zone parameter. Device personalization requires
 * the following steps: The configuraton zone is first programmed and locked. After
 * the configuration zone is locked, the Data and OTP areas are programmed.
 * The Data and OTP zones are then locked in a single operation, by setting the zone
 * parameter to S96AT_ZONE_DATA. Specifying S96AT_ZONE_OTP returns an error.
 *
 * Notice that locking a zone is a one-time operation.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_lock_zone(struct s96at_desc *desc, enum s96at_zone zone, uint16_t crc);

/* Read from a zone
 *
 * The Config zone consists of 88 bytes divided into 22x 4-byte words. The id
 * parameter specifies which word to be read, in the range of 0-21. The buffer
 * length must be S96AT_READ_CONFIG_LEN. Reading from the Config zone is always
 * permitted.
 *
 * The Data zone consists of 512 bytes divided into 16x 32-byte slots. The id
 * parameter specifies which slot to be read, in the range of 0-15. The buffer
 * length must be S96AT_READ_DATA_LEN. Reading is permitted only after the Data
 * zone has been locked. The permissions to read depends on the slot's configuration.
 * Reads from the Data zone can be encrypted if the EncryptedRead bit is set
 * in the slot's configuration. To perform an encrypted read, GenDig must be run
 * before reading data to set the encryption key.
 *
 * The OTP zone consists of 64 bytes divided into 16x 4-byte words. The id
 * parameter specifies which word to be read in the range of 0 - 15. The buffer
 * length must be S96AT_READ_OTP_LEN. Reading is permitted only after the OTP
 * zone has been locked. If the OTP zone is configured into Legacy mode, reading
 * from words 0 or 1 returns an error.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_read(struct s96at_desc *desc, enum s96at_zone zone, uint8_t id, uint8_t *buf);

/* Wake up the device
 *
 * Wakes up the device by sending the wake-up sequence. Upon wake up, a watchdog
 * counter is triggered, which keeps the device awake for S96AT_WATCHDOG_TIME.
 * Once the counter reaches zero, the device enters sleep mode, regardless of its
 * current command execution or IO state. It is therefore required that all operations
 * are completed within S96AT_WATCHDOG_TIME.
 *
 * Returns S96AT_STATUS_READY on device wake, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_wake(struct s96at_desc *desc);

/* Write to a zone
 *
 * The Config zone consists of 88 bytes divided into 22x 4-byte words. The id
 * parameter specifies which word to be written, in the range of 0-21. The buffer
 * length must be S96AT_WRITE_CONFIG_LEN. Writing to the Config zone is only
 * permitted before the zone is locked.
 *
 * The Data zone consists of 512 bytes divided into 16x 32-byte slots. The id
 * parameter specifies which slot to be write, in the range of 0-15. The buffer
 * length must be S96AT_WRITE_DATA_LEN. Writing to the Data zone is allowed once
 * the Configuration zone has been locked, and before the Data zone has been locked.
 * After the Data zone is locked, writing to a slot depends on the permissions set
 * on the slot's configuration. Writes to the Data zone can be encrypted, if
 * S96AT_FLAG_ENCRYPT is set.
 *
 * The OTP zone consists of 64 bytes divided into 16x 4-byte words. The id
 * parameter specifies which word to be written, in the range of 0 - 15. The buffer
 * length must be S96AT_WRITE_OTP_LEN. Writing is permitted only after the Configuration
 * zone has been locked and before the OTP zone has been locked. NOTICE THAT WRITING
 * INTO THE OTP ZONE IS A ONE-TIME OPERATION. Once the OTP area has been locked, writing
 * is only permitted if the zone has been configured in Consumption mode, in which case
 * bits can only be changed from zero to one.
 *
 * Returns S96AT_STATUS_OK on success, otherwise S96AT_STATUS_EXEC_ERROR.
 */
uint8_t s96at_write(struct s96at_desc *desc, enum s96at_zone zone, uint8_t id,
		    uint32_t flags, const uint8_t *buf);

#endif
