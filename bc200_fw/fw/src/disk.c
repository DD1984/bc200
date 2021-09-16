#include <zephyr.h>
#include <devicetree.h>
#include <logging/log.h>
#include <fs/fs.h>
#include <storage/flash_map.h>
#include <ff.h>

LOG_MODULE_REGISTER(setup_disk, CONFIG_FS_LOG_LEVEL);

static struct fs_mount_t fs_mnt;

static int setup_flash(struct fs_mount_t *mnt)
{
	int rc = 0;
	unsigned int id;
	const struct flash_area *pfa;

	mnt->storage_dev = (void *)FLASH_AREA_ID(ext_flash);
	id = (uintptr_t)mnt->storage_dev;

	rc = flash_area_open(id, &pfa);
	LOG_INF("area %u at 0x%x on %s for %u bytes",
		id,
		(unsigned int)pfa->fa_off,
		pfa->fa_dev_name,
		(unsigned int)pfa->fa_size);

	if (rc < 0)
		flash_area_close(pfa);

	return rc;
}

static int mount_app_fs(struct fs_mount_t *mnt)
{
	int rc;

	static FATFS fat_fs;

	mnt->type = FS_FATFS;
	mnt->fs_data = &fat_fs;
	mnt->mnt_point = "/NAND:";

	rc = fs_mount(mnt);

	return rc;
}

void setup_disk(void)
{
	struct fs_mount_t *mp = &fs_mnt;
	struct fs_dir_t dir;
	struct fs_statvfs sbuf;
	int rc;

	fs_dir_t_init(&dir);

	rc = setup_flash(mp);
	if (rc < 0) {
		LOG_ERR("Failed to setup flash area");
		return;
	}

	rc = mount_app_fs(mp);
	if (rc < 0) {
		LOG_ERR("Failed to mount filesystem");
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




	rc = fs_opendir(&dir, mp->mnt_point);
	printk("%s opendir: %d\n", mp->mnt_point, rc);

	if (rc < 0)
		LOG_ERR("Failed to open directory");

	while (rc >= 0) {
		struct fs_dirent ent = { 0 };

		rc = fs_readdir(&dir, &ent);
		if (rc < 0) {
			LOG_ERR("Failed to read directory entries");
			break;
		}
		if (ent.name[0] == 0) {
			printk("End of files\n");
			break;
		}
		printk("  %c %u %s\n",
		       (ent.type == FS_DIR_ENTRY_FILE) ? 'F' : 'D',
		       ent.size,
		       ent.name);
	}

	(void)fs_closedir(&dir);

	return;
}
