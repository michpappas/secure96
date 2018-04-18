#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <cmd.h>
#include <crc.h>
#include <debug.h>
#include <io.h>
#include <personalize.h>
#include <status.h>

/* Generated by ATSHA204A slot config generator */
struct slot_config {
     uint8_t address;
     uint8_t value[4];
};

static struct slot_config slot_configs[] = {
	/* SlotConfig[0]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[1]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x0a */
	{ 0x05, {  0x80, 0x80,   0x80, 0xa0 } },

	/* SlotConfig[2]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[3]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x0b */
	{ 0x06, {  0x80, 0x80,   0x80, 0xb0 } },

	/* SlotConfig[4]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[5]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x0a */
	{ 0x07, {  0x80, 0x80,   0x80, 0xa0 } },

	/* SlotConfig[6]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[7]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x0b */
	{ 0x08, {  0x80, 0x80,   0x80, 0xb0 } },

	/* SlotConfig[8]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x08, WriteConfig=0x04 */
	/* SlotConfig[9]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=1, IsSecret=1, WriteKey=0x09, WriteConfig=0x04 */
	{ 0x09, {  0x80, 0x48,   0xc0, 0x49 } },

	/* SlotConfig[10]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[11]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=1, WriteKey=0x00, WriteConfig=0x08 */
	{ 0x0a, {  0x80, 0x80,   0x80, 0x80 } },

	/* SlotConfig[12]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=0, WriteKey=0x00, WriteConfig=0x00 */
	/* SlotConfig[13]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=0, WriteKey=0x00, WriteConfig=0x00 */
	{ 0x0b, {  0x00, 0x00,   0x00, 0x00 } },

	/* SlotConfig[14]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=0, WriteKey=0x00, WriteConfig=0x08 */
	/* SlotConfig[15]: ReadKey=0x00, CheckOnly=0, SingleUse=0, EncryptedRead=0, IsSecret=0, WriteKey=0x00, WriteConfig=0x08 */
	{ 0x0c, {  0x00, 0x80,   0x00, 0x80 } },
};

static bool is_configuration_locked(struct io_interface *ioif)
{
	uint8_t lock_config;
	int ret = cmd_get_lock_config(ioif, &lock_config);
	if (ret != STATUS_OK) {
		loge("Couldn't get lock config\n");
		return false;
	}

	return lock_config == LOCK_CONFIG_LOCKED;
}

static bool is_data_zone_locked(struct io_interface *ioif)
{
	uint8_t lock_data;
	int ret = cmd_get_lock_data(ioif, &lock_data);
	if (ret != STATUS_OK) {
		loge("Couldn't get lock data\n");
		return false;
	}

	return lock_data == LOCK_DATA_LOCKED;
}

/*
 * This programs all data slots, beware that this should NOT be used in real use
 * cases, since it uses fixed keys where the key is the same hex number as the
 * slot. I.e, slot[0]=000000.., slot[1]=111111..., ..., slot[31]=ffffff....
 */
static int program_data_slots(struct io_interface *ioif, uint16_t *crc)
{
	int i;
	int ret = STATUS_EXEC_ERROR;
	uint8_t data[32] = { 0 };

	for (i = 0; i < 16; i++) {
		/* 00...00, 11...11, 22...22, ..., ff..ff */
		memset(&data, i << 4 | i, sizeof(data));

		/*
		 * We must update CRC in each loop to be able to return the CRC
		 * for the entire data area.
		 */
		*crc = calculate_crc16(data, sizeof(data), *crc);

		logd("Storing: %u bytes, 0x%02x...0x%02x (running CRC: 0x%04x)\n", sizeof(data), data[0], data[31], *crc);
		ret = cmd_write(ioif, ZONE_DATA, SLOT_ADDR(i), false, data, sizeof(data));
		if (ret != STATUS_OK) {
			loge("Failed to program data slot: %d\n", i);
			break;
		}
	}

	return ret;
}

/*
 * This programs OTP, beware that this should be tweaked for the use case in
 * mind. This just programs OTP with the same value as the word address of the
 * OTP (for testing puprpose).
 * I.e, OTP[0]=000000.., OTP[1]=111111..., ..., OTP[16]=ffffff....
 */
static int program_otp_zone(struct io_interface *ioif, uint16_t *crc)
{
	int i, j;
	int ret = STATUS_EXEC_ERROR;
	uint8_t data[32] = { 0 };

	/*
	 * Before Data/OTP zones are locked, only 32-byte values
	 * can be written (section 8.5.18). We therefore need to
	 * program the OTP in terms of blocks, which correspond
	 * to address 0x00 and 0x10.
	 */
	for (i = 0; i < 2; i++) {
		/* 00...00, 11...11, 22...22, ..., ff..ff */
		for (j = 0; j < 8; j++) {
			memset(data + j * 4, (i * 8 + j) << 4 |
			       (i * 8 + j), 4);
		}

		/*
		 * We must update CRC in each loop to be able to return the CRC
		 * for the entire OTP area.
		 */
		*crc = calculate_crc16(data, sizeof(data), *crc);
		logd("Storing: %u bytes, 0x%02x...0x%02x (running CRC: 0x%04x)\n", sizeof(data), data[0], data[31], *crc);
		ret = cmd_write(ioif, ZONE_OTP, i * 0x10, false, data, sizeof(data));
		if (ret != STATUS_OK) {
			loge("Failed to program OTP address: 0x%02x\n", i * 0x10);
			break;
		}
	}

	return ret;
}

static int program_slot_configs(struct io_interface *ioif)
{
	int i;
	int ret = STATUS_EXEC_ERROR;

	for (i = 0; i < sizeof(slot_configs) / sizeof(struct slot_config); i++) {
		logd("addr: 0x%02x, config[%02d]: 0x%02x 0x%02x, config[%02d]: 0x%02x 0x%02x\n",
		     slot_configs[i].address,
		     2*i, slot_configs[i].value[0], slot_configs[i].value[1],
		     (2*i)+1, slot_configs[i].value[2], slot_configs[i].value[3]);

		ret = cmd_write(ioif, ZONE_CONFIG, slot_configs[i].address, false,
				slot_configs[i].value, sizeof(slot_configs[i].value));

		if (ret != STATUS_OK) {
			loge("Failed to program slot config: %d/%d\n", 2*i, (2*i) + 1);
			break;
		}
	}

	return ret;
}

static int lock_config_zone(struct io_interface *ioif)
{
	uint16_t crc = 0;
	uint8_t config_zone[ZONE_CONFIG_SIZE] = { 0 };
	int ret = STATUS_EXEC_ERROR;

	ret = cmd_get_config_zone(ioif, config_zone, sizeof(config_zone));
	if (ret != STATUS_OK)
		goto out;

	hexdump("config_zone", config_zone, ZONE_CONFIG_SIZE);

	crc = calculate_crc16(config_zone, sizeof(config_zone), crc);

	ret = cmd_lock_zone(ioif, ZONE_CONFIG, &crc);
out:
	return ret;
}

int atsha204a_personalize(struct io_interface *ioif)
{
	int ret = STATUS_OK;

	if (is_configuration_locked(ioif)) {
		logd("Device config already locked\n");
	} else {
		ret = program_slot_configs(ioif);
		if (ret != STATUS_OK)
			goto out;

		ret = lock_config_zone(ioif);
		if (ret != STATUS_OK)
			goto out;
	}

	if (is_data_zone_locked(ioif)) {
		logd("Device data already locked\n");
	} else {
		uint16_t crc = 0;

		ret = program_data_slots(ioif, &crc);
		if (ret != STATUS_OK)
			goto out;

		logd("Intermediate CRC: 0x%04x\n", crc);
		ret = program_otp_zone(ioif, &crc);
		if (ret != STATUS_OK)
			goto out;

		logd("Final CRC: 0x%04x\n", crc);
		ret = cmd_lock_zone(ioif, ZONE_DATA, &crc);
	}
out:
	return ret;
}
