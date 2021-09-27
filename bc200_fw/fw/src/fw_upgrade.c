#include <stdio.h>
#include <stdlib.h>
#include <zephyr.h>
#include <devicetree.h>
#include <logging/log.h>
#include <storage/flash_map.h>
#include <usb/usb_device.h>
#include <ff.h>
#include <fs/fs.h>
//#include <sys/crc.h> todo: need to use system crc func
#include <sys/reboot.h>

extern void uprg_progress(const char *text);

#include "decode_utils.h"
#include "nrf_dfu_types.h"

LOG_MODULE_REGISTER(fw_upgrade, CONFIG_FS_LOG_LEVEL);

#define UPGR_DIR "Upgrade"
#define UPGR_FILE_SIGN "BC200_Upgrade"

#define PRIORITY 7

K_THREAD_STACK_DEFINE(fw_upgrade_thread_stack_area, 4096);
static struct k_thread fw_upgrade_thread_data;
static k_tid_t fw_upgrade_tid;
static struct fs_mount_t fs_mnt;

static uint32_t crc32_compute(uint8_t const * p_data, uint32_t size, uint32_t const * p_crc)
{
	uint32_t crc;

	crc = (p_crc == NULL) ? 0xFFFFFFFF : ~(*p_crc);
	for (uint32_t i = 0; i < size; i++)
	{
		crc = crc ^ p_data[i];
		for (uint32_t j = 8; j > 0; j--)
		{
			crc = (crc >> 1) ^ (0xEDB88320U & ((crc & 1) ? 0xFFFFFFFF : 0));
		}
	}
	return ~crc;
}

static int setup_flash(struct fs_mount_t *mnt)
{
	mnt->storage_dev = (void *)FLASH_AREA_ID(ext_flash);

#if 0
	//check partition, may be not needed

	int rc = 0;
	unsigned int id;
	const struct flash_area *pfa;

	id = (uintptr_t)mnt->storage_dev;

	rc = flash_area_open(id, &pfa);
	LOG_INF("area %u at 0x%x on %s for %u bytes",
		id,
		(unsigned int)pfa->fa_off,
		pfa->fa_dev_name,
		(unsigned int)pfa->fa_size);

	if (rc < 0)
		flash_area_close(pfa);
#endif

	return 0;
}

static void fs_init(struct fs_mount_t *mnt)
{
	static FATFS fat_fs;

	mnt->type = FS_FATFS;
	mnt->fs_data = &fat_fs;
	mnt->mnt_point = "/NAND:";
}

static int upgrade(const char *file)
{
	int ret;
	uint8_t buf[BLOCK_SIZE];

	// encode fw and copy it to bank_1
	unsigned short diff = START_DIFF;

	struct fs_file_t f;
	fs_file_t_init(&f);

	ret = fs_open(&f, file, FS_O_READ);
	if (ret < 0)
		return -1;

	const struct flash_area *img1_fa;
	flash_area_open(FLASH_AREA_ID(image_1), &img1_fa);

	flash_area_erase(img1_fa, 0, img1_fa->fa_size);

	ssize_t target_file_len = 0;
	uint32_t crc32 = 0;

	ssize_t len;
	while ((len = fs_read(&f, buf, BLOCK_SIZE)) > 0) {
		decode_block(&diff, buf, len);

		flash_area_write(img1_fa, target_file_len, buf, len - 4);

		target_file_len += len - 4; // 4 - block cksum

		crc32 = crc32_compute(buf, len - 4, &crc32);

		char uprg_buf[64];
		sprintf(uprg_buf, "Loading %zu bytes\n", target_file_len);
		uprg_progress(uprg_buf);
	}

	flash_area_close(img1_fa);
	fs_close(&f);

	// calc all crc
	const struct flash_area *bs_fa;
	flash_area_open(FLASH_AREA_ID(boot_settings), &bs_fa);

	nrf_dfu_settings_t bs;
	flash_area_read(bs_fa, 0, &bs, sizeof(nrf_dfu_settings_t));

	bs.bank_1.image_size = target_file_len;
	bs.bank_1.image_crc = crc32;
	bs.bank_1.bank_code = NRF_DFU_BANK_VALID_APP;
	bs.crc = crc32_compute((uint8_t *)&bs + 4, DFU_SETTINGS_INIT_COMMAND_OFFSET - 4, NULL);

	bs.boot_validation_app.type = VALIDATE_CRC;
	*(uint32_t *)&bs.boot_validation_app.bytes = crc32;
	bs.boot_validation_crc = crc32_compute((const uint8_t *)&bs.boot_validation_softdevice,
											DFU_SETTINGS_BOOT_VALIDATION_SIZE - 4, NULL);

	flash_area_erase(bs_fa, 0, bs_fa->fa_size);
	flash_area_write(bs_fa, 0, &bs, sizeof(nrf_dfu_settings_t));

	flash_area_close(bs_fa);

	return 0;
}

char *find_upgrade_file(struct fs_mount_t *mnt)
{
	int ret;

	struct fs_dir_t dir;
	fs_dir_t_init(&dir);

	char buf[MAX_FILE_NAME];
	sprintf(buf, "%s/"UPGR_DIR,  mnt->mnt_point);

	ret = fs_opendir(&dir, buf);
	if (ret < 0)
		LOG_ERR("Failed to open directory");

	struct fs_dirent ent = { 0 };

	while (ret >= 0) {
		ret = fs_readdir(&dir, &ent);
		if (ret < 0) {
			LOG_ERR("Failed to read directory entries");
			break;
		}
		if (ent.name[0] == 0)
			break;
		if (strstr(ent.name, UPGR_FILE_SIGN) && ent.type == FS_DIR_ENTRY_FILE)
			break;
	}

	char *file = NULL;
	if (*ent.name) {
		file = malloc(strlen(mnt->mnt_point) + strlen(UPGR_DIR) + strlen(ent.name) + 2 + 1);
		sprintf(file, "%s/"UPGR_DIR"/%s", mnt->mnt_point, ent.name);
	}

	fs_closedir(&dir);

	return file;
}

static int check_upgrade(void)
{
	int ret;

	ret = fs_mount(&fs_mnt);
	if (ret < 0) {
		LOG_ERR("Failed to mount filesystem");
		return -1;
	}

	ret = -1;
	char *f = find_upgrade_file(&fs_mnt);
	if (f) {
		LOG_INF("upgrade file: %s", log_strdup(f));

		ret = upgrade(f);
		if (ret == 0)
			fs_unlink(f);

		free(f);
	}

	fs_unmount(&fs_mnt);

	return ret;
}

static void usb_status_cb(enum usb_dc_status_code cb_status, const uint8_t *param)
{
	switch (cb_status) {
		case USB_DC_DISCONNECTED:
			LOG_INF("USB device disconnected");
			uprg_progress("USB disconnected");

			k_wakeup(fw_upgrade_tid);
		break;
		case USB_DC_CONNECTED:
			LOG_INF("USB device connected!!!");
			uprg_progress("USB connected!\n");
		break;
		default:
		break;
	}
}

#if 0
void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;
	struct fs_statvfs sbuf;
	int rc;

	rc = setup_flash(mp);
	if (rc < 0) {
		LOG_ERR("Failed to setup flash area");
		return;
	}

	/* Allow log messages to flush to avoid interleaved output */
	k_sleep(K_MSEC(50));

	LOG_INF("Mount \"%s\": %d", fs_mnt.mnt_point, rc);

	rc = fs_statvfs(mp->mnt_point, &sbuf);
	if (rc < 0) {
		LOG_ERR("FAIL: statvfs: %d\n", rc);
		return;
	}

	LOG_INF("\"%s\": bsize = %lu ; frsize = %lu ; blocks = %lu ; bfree = %lu",
		mp->mnt_point,
		sbuf.f_bsize, sbuf.f_frsize,
		sbuf.f_blocks, sbuf.f_bfree);

	return;
}
#endif


void fw_upgrade_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	int ret;

	ret = setup_flash(&fs_mnt);
	if (ret < 0) {
		LOG_ERR("Failed to setup flash area");
		return;
	}

	ret = usb_enable(usb_status_cb);
	if (ret != 0) {
		LOG_ERR("Failed to enable USB");
		return;
	}

	LOG_INF("The device is put in USB mass storage mode.");

	fs_init(&fs_mnt);

	while (1) {
		k_sleep(K_FOREVER);

		if (check_upgrade() == 0) {
			printk("reboot\n");
			sys_reboot(SYS_REBOOT_WARM);
		}
	}
}

void fw_upgrade_task_start(void)
{
	fw_upgrade_tid = k_thread_create(&fw_upgrade_thread_data, fw_upgrade_thread_stack_area, K_THREAD_STACK_SIZEOF(fw_upgrade_thread_stack_area),
				fw_upgrade_thread, NULL, NULL, NULL, PRIORITY, 0, K_FOREVER);
	k_thread_name_set(&fw_upgrade_thread_data, "fw_upgrade");

	k_thread_start(&fw_upgrade_thread_data);
}



